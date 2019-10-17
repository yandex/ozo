#include "benchmark.h"

#include <ozo/connection_info.h>
#include <ozo/connection_pool.h>
#include <ozo/request.h>
#include <ozo/query_builder.h>
#include <ozo/shortcuts.h>

#include <boost/asio/io_service.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/program_options.hpp>

#include <condition_variable>
#include <thread>

namespace {

namespace asio = boost::asio;

using benchmark_t = ozo::benchmark::time_limit_benchmark;

constexpr const std::chrono::seconds connect_timeout(1);
constexpr const std::chrono::seconds request_timeout(1);

template <typename T>
void spawn(asio::io_context& io, std::size_t token, T&& coroutine) {
    asio::spawn(io, [token, coroutine = std::forward<T>(coroutine)] (asio::yield_context yield) {
        try {
            coroutine(yield);
        } catch (const boost::coroutines::detail::forced_unwind&) {
            throw;
        } catch (const std::exception& e) {
            std::cout << "coroutine " << token << " failed: " << e.what() << std::endl;
        }
    });
}

enum class query_type {
    simple,
    complex,
};

std::ostream& operator <<(std::ostream& stream, query_type value) {
    switch (value) {
        case query_type::simple:
            return stream << "simple";
        case query_type::complex:
            return stream << "complex";
    }
    return stream;
}

std::istream& operator >>(std::istream& stream, query_type& value) {
    std::string token;
    stream >> token;
    if (token == "simple") {
        value = query_type::simple;
    } else if (token == "complex") {
        value = query_type::complex;
    } else {
        throw std::invalid_argument("Invalid query type: \"" + token + "\"");
    }
    return stream;
}

struct benchmark_params {
    std::string conn_string;
    ::query_type query_type = ::query_type::simple;
    std::size_t coroutines = 0;
    std::size_t threads_number = 0;
    std::size_t queue_capacity = 0;
    std::size_t connections = 0;
    bool parse_result = false;
};

template <typename Row, typename Query>
void reopen_connection(const benchmark_params& params, Query query) {
    std::cout << '\n' << __func__ << std::endl;

    benchmark_t benchmark(1);
    asio::io_context io(1);
    ozo::connection_info<> connection_info(params.conn_string);

    spawn(io, 0, [&] (asio::yield_context yield) {
        while (true) {
            std::conditional_t<std::is_same_v<Row, void>, ozo::result, std::vector<Row>> result;
            ozo::request(connection_info[io], query, request_timeout, ozo::into(result), yield);
            if (!benchmark.step(result.size())) {
                break;
            }
        }
    });

    io.run();
}

template <typename Row, typename Query>
void reuse_connection(const benchmark_params& params, Query query) {
    std::cout << '\n' << __func__ << std::endl;

    benchmark_t benchmark(1);
    asio::io_context io(1);
    ozo::connection_info<> connection_info(params.conn_string);

    spawn(io, 0, [&] (asio::yield_context yield) {
        auto connection = ozo::get_connection(connection_info[io], connect_timeout, yield);
        while (true) {
            std::conditional_t<std::is_same_v<Row, void>, ozo::result, std::vector<Row>> result;
            ozo::request(connection, query, request_timeout, ozo::into(result), yield);
            if (!benchmark.step(result.size())) {
                break;
            }
        }
    });

    io.run();
}

template <typename Row, typename Query>
void use_connection_pool(const benchmark_params& params, Query query) {
    std::cout << '\n' << __func__ << " coroutines=" << params.coroutines << std::endl;

    benchmark_t benchmark(params.coroutines);
    asio::io_context io(1);
    const ozo::connection_info<> connection_info(params.conn_string);
    ozo::connection_pool_config config;
    config.capacity = params.coroutines + 1;
    config.queue_capacity = params.queue_capacity;
    auto pool = ozo::make_connection_pool(connection_info, config);

    for (std::size_t token = 0; token < params.coroutines; ++token) {
        spawn(io, token, [&, token] (asio::yield_context yield) {
            while (true) {
                std::conditional_t<std::is_same_v<Row, void>, ozo::result, std::vector<Row>> result;
                ozo::request(pool[io], query, request_timeout, ozo::into(result), yield);
                if (!benchmark.step(result.size(), token)) {
                    break;
                }
            }
        });
    }

    io.run();
}

struct context {
    asio::io_context io;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> guard = boost::asio::make_work_guard(io);
    std::thread thread {[this] { this->io.run(); }};

    context() = default;
};

template <typename Row, typename Query>
void use_connection_pool_mult_threads(const benchmark_params& params, Query query) {
    std::cout << '\n' << __func__
        << " threads_number=" << params.threads_number
        << " coroutines_per_thread=" << params.coroutines
        << " connections=" << params.connections
        << " queue_capacity=" << params.queue_capacity << std::endl;

    benchmark_t benchmark(params.coroutines * params.threads_number);
    const ozo::connection_info<> connection_info(params.conn_string);
    ozo::connection_pool_config config;
    config.capacity = params.connections;
    config.queue_capacity = params.queue_capacity;
    std::vector<std::unique_ptr<context>> contexts;
    auto pool = ozo::make_connection_pool(connection_info, config);
    std::atomic_size_t finished_coroutines {0};
    std::mutex mutex;
    std::unique_lock<std::mutex> lock(mutex);
    std::condition_variable coroutine_finished;

    for (std::size_t i = 0; i < params.threads_number; ++i) {
        contexts.emplace_back(std::make_unique<context>());
        auto& io = contexts.back()->io;
        for (std::size_t j = 0; j < params.coroutines; ++j) {
            const auto token = params.coroutines * i + j;
            spawn(io, token, [&, token] (asio::yield_context yield) {
                while (true) {
                    std::conditional_t<std::is_same_v<Row, void>, ozo::result, std::vector<Row>> result;
                    boost::system::error_code ec;
                    ozo::request(pool[io], query, request_timeout, ozo::into(result), yield[ec]);
                    if (!benchmark.thread_safe_step(result.size(), token)) {
                        break;
                    }
                    if (ec) {
                        break;
                    }
                }
                ++finished_coroutines;
                coroutine_finished.notify_all();
            });
        }
    }

    if (params.coroutines * params.threads_number > 0) {
        coroutine_finished.wait(lock, [&] {
            return finished_coroutines >= params.coroutines * params.threads_number;
        });
    }

    std::for_each(contexts.begin(), contexts.end(), [] (const auto& v) { v->guard.reset(); });
    std::for_each(contexts.begin(), contexts.end(), [] (const auto& v) { v->thread.join(); });
}

template <typename Row, typename Query>
void run_benchmark(const std::string& name, const benchmark_params& params, Query query) {
    std::map<std::string, std::function<void ()>> scenarios {{
        {
            "reopen_connection",
            [&] {
                if (params.parse_result) {
                    return reopen_connection<Row>(params, query);
                } else {
                    return reopen_connection<void>(params, query);
                }
            }
        },
        {
            "reuse_connection",
            [&] {
                if (params.parse_result) {
                    return reuse_connection<Row>(params, query);
                } else {
                    return reuse_connection<void>(params, query);
                }
            }
        },
        {
            "use_connection_pool",
            [&] {
                if (params.parse_result) {
                    return use_connection_pool<Row>(params, query);
                } else {
                    return use_connection_pool<void>(params, query);
                }
            }
        },
        {
            "use_connection_pool_mult_threads",
            [&] {
                if (params.parse_result) {
                    return use_connection_pool_mult_threads<Row>(params, query);
                } else {
                    return use_connection_pool_mult_threads<void>(params, query);
                }
            }
        },
    }};

    const auto scenario = scenarios.find(name);

    if (scenario == scenarios.end()) {
        throw std::invalid_argument("Invalid benchmark name: \"" + name + "\"");
    }

    return scenario->second();
}

void run_benchmark(const std::string& name, const benchmark_params& params) {
    using namespace ozo::literals;

    const auto simple_query = "SELECT 1"_SQL.build();
    const auto complex_query = (
        "SELECT typname, typnamespace, typowner, typlen, typbyval, typcategory, "_SQL +
        "typispreferred, typisdefined, typdelim, typrelid, typelem, typarray "_SQL +
        "FROM pg_type WHERE typtypmod = "_SQL + -1 + " AND typisdefined = "_SQL + true
    ).build();

    switch (params.query_type) {
        case query_type::simple:
            return run_benchmark<std::tuple<std::int32_t>>(name, params, simple_query);
        case query_type::complex:
            return run_benchmark<ozo::benchmark::pg_type>(name, params, complex_query);
    }

    throw std::invalid_argument("Invalid query type: \"" + std::to_string(static_cast<int>(params.query_type)) + "\"");
}

} // namespace

int main(int argc, char **argv) {
    using namespace ozo::benchmark;

    namespace po = boost::program_options;

    try {
        po::options_description options;

        options.add_options()
            ("help,h", "print help message")
            ("benchmark,b", po::value<std::string>(), "benchmark name to run")
            ("parse,p", "parse query result")
            ("coroutines", po::value<std::size_t>()->default_value(1), "number of parallel coroutines")
            ("connections", po::value<std::size_t>(), "number of parallel coroutines (default: equal to coroutines + 1)")
            ("threads", po::value<std::size_t>()->default_value(1), "number of threads")
            ("queue", po::value<std::size_t>()->default_value(0), "connection pool queue capacity")
            ("conninfo", po::value<std::string>()->default_value(""), "psql-like database connection info")
            ("query", po::value<query_type>()->default_value(query_type::simple), "query type (simple or complex)")
        ;

        po::variables_map variables;
        po::store(po::parse_command_line(argc, argv, options), variables);
        po::notify(variables);

        if (variables.count("help")) {
            std::cout << options << std::endl;
            return 0;
        }

        if (!variables.count("benchmark")) {
            std::cerr << "Nothing to run: benchmark is not set" << std::endl;
            return -1;
        }

        benchmark_params params;
        params.conn_string = variables.at("conninfo").as<std::string>();
        params.query_type = variables.at("query").as<query_type>();
        params.coroutines = variables.at("coroutines").as<std::size_t>();
        params.queue_capacity = variables.at("queue").as<std::size_t>();
        params.threads_number = variables.at("threads").as<std::size_t>();
        if (variables.count("connections")) {
            params.connections = variables.at("connections").as<std::size_t>();
        } else {
            params.connections = params.coroutines;
        }

        run_benchmark(variables.at("benchmark").as<std::string>(), params);

        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return -1;
    }
}
