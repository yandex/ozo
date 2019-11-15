#include "benchmark.h"

#include <ozo/connection_info.h>
#include <ozo/connection_pool.h>
#include <ozo/request.h>
#include <ozo/query_builder.h>
#include <ozo/shortcuts.h>

#include <nlohmann/json.hpp>

#include <boost/asio/io_service.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/program_options.hpp>

#include <cassert>
#include <condition_variable>
#include <thread>

namespace {

namespace asio = boost::asio;

using benchmark_t = ozo::benchmark::time_limit_benchmark;

constexpr const std::chrono::seconds connect_timeout(1);
constexpr const std::chrono::seconds request_timeout(1);

std::mutex cerr_mutex;

template <typename T>
void spawn(asio::io_context& io, std::size_t token, T&& coroutine) {
    asio::spawn(io, [token, coroutine = std::forward<T>(coroutine)] (asio::yield_context yield) {
        try {
            {
                const std::lock_guard lock(cerr_mutex);
                std::cerr << "coroutine " << token << " started" << std::endl;
            }
            coroutine(yield);
        } catch (const boost::coroutines::detail::forced_unwind&) {
            throw;
        } catch (const std::exception& e) {
            const std::lock_guard lock(cerr_mutex);
            std::cerr << "coroutine " << token << " failed: " << e.what() << std::endl;
            std::abort();
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
    std::chrono::steady_clock::duration duration;
    std::size_t coroutines = 0;
    std::size_t threads_number = 0;
    std::size_t queue_capacity = 0;
    std::size_t connections = 0;
    bool parse_result = false;
    bool verbose = false;
};

struct benchmark_report {
    std::string name;
    std::string query;
    ozo::benchmark::output output;
    ozo::benchmark::stats stats;
    OZO_STD_OPTIONAL<std::size_t> coroutines;
    OZO_STD_OPTIONAL<std::size_t> threads_number;
    OZO_STD_OPTIONAL<std::size_t> queue_capacity;
    OZO_STD_OPTIONAL<std::size_t> connections;
    OZO_STD_OPTIONAL<bool> parse_result;
};

std::ostream& operator <<(std::ostream& stream, const benchmark_report& value) {
    stream << "benchmark: " << value.name << '\n';
    stream << "query: " << value.query << '\n';
    if (value.coroutines) {
        stream << "threads_number: " << *value.coroutines << '\n';
    }
    if (value.threads_number) {
        stream << "threads_number: " << *value.threads_number << '\n';
    }
    if (value.queue_capacity) {
        stream << "queue_capacity: " << *value.queue_capacity << '\n';
    }
    if (value.connections) {
        stream << "connections: " << *value.connections << '\n';
    }
    if (value.parse_result) {
        stream << "parse_result: " << *value.parse_result << '\n';
    }
    stream << value.stats << '\n';
    return stream;
}

template <typename Row, typename Query>
benchmark_report reopen_connection(const benchmark_params& params, Query query) {
    constexpr bool parse_result = !std::is_same_v<Row, void>;

    assert(parse_result == params.parse_result);

    benchmark_report report;
    report.name = __func__;
    report.query = ozo::to_const_char(ozo::get_text(query));
    report.parse_result = parse_result;

    benchmark_t benchmark(1, params.duration);
    benchmark.set_print_progress(params.verbose);

    asio::io_context io(1);
    ozo::connection_info connection_info(params.conn_string);

    spawn(io, 0, [&] (asio::yield_context yield) {
        while (true) {
            std::conditional_t<parse_result, std::vector<Row>, ozo::result> result;
            ozo::error_code ec;
            const auto connection = ozo::request(connection_info[io], query, request_timeout, ozo::into(result), yield[ec]);
            if (ec) {
                std::cerr << ec.message() << '\n';
                if (connection) {
                    std::cerr << ozo::get_error_context(connection) << '\n';
                    std::cerr << ozo::error_message(connection) << '\n';
                }
                std::abort();
            }
            if (!benchmark.step(result.size())) {
                break;
            }
        }
    });

    io.run();

    report.output = benchmark.get_output();
    report.stats = benchmark.get_stats();

    return report;
}

template <typename Row, typename Query>
benchmark_report reuse_connection(const benchmark_params& params, Query query) {
    constexpr bool parse_result = !std::is_same_v<Row, void>;

    assert(parse_result == params.parse_result);

    benchmark_report report;
    report.name = __func__;
    report.query = ozo::to_const_char(ozo::get_text(query));
    report.parse_result = parse_result;

    benchmark_t benchmark(1, params.duration);
    benchmark.set_print_progress(params.verbose);

    asio::io_context io(1);
    ozo::connection_info connection_info(params.conn_string);

    spawn(io, 0, [&] (asio::yield_context yield) {
        auto connection = ozo::get_connection(connection_info[io], connect_timeout, yield);
        while (true) {
            std::conditional_t<parse_result, std::vector<Row>, ozo::result> result;
            ozo::error_code ec;
            connection = ozo::request(std::move(connection), query, request_timeout, ozo::into(result), yield[ec]);
            if (ec) {
                std::cerr << ec.message() << '\n';
                if (connection) {
                    std::cerr << ozo::get_error_context(connection) << '\n';
                    std::cerr << ozo::error_message(connection) << '\n';
                }
                std::abort();
            }
            if (!benchmark.step(result.size())) {
                break;
            }
        }
    });

    io.run();

    report.output = benchmark.get_output();
    report.stats = benchmark.get_stats();

    return report;
}

template <typename Row, typename Query>
benchmark_report use_connection_pool(const benchmark_params& params, Query query) {
    constexpr bool parse_result = !std::is_same_v<Row, void>;

    assert(parse_result == params.parse_result);

    benchmark_report report;
    report.name = __func__;
    report.query = ozo::to_const_char(ozo::get_text(query));
    report.coroutines = params.coroutines;
    report.queue_capacity = params.queue_capacity;
    report.parse_result = parse_result;

    benchmark_t benchmark(params.coroutines, params.duration);
    benchmark.set_print_progress(params.verbose);

    asio::io_context io(1);
    const ozo::connection_info connection_info(params.conn_string);
    ozo::connection_pool_config config;
    config.capacity = params.coroutines + 1;
    config.queue_capacity = params.queue_capacity;
    ozo::connection_pool pool(connection_info, config, !ozo::thread_safe);

    for (std::size_t token = 0; token < params.coroutines; ++token) {
        spawn(io, token, [&, token] (asio::yield_context yield) {
            while (true) {
                std::conditional_t<parse_result, std::vector<Row>, ozo::result> result;
                ozo::error_code ec;
                const auto connection = ozo::request(pool[io], query, request_timeout, ozo::into(result), yield[ec]);
                if (ec) {
                    std::cerr << ec.message() << '\n';
                    if (connection) {
                        std::cerr << ozo::get_error_context(connection) << '\n';
                        std::cerr << ozo::error_message(connection) << '\n';
                    }
                    std::abort();
                }
                if (!benchmark.step(result.size(), token)) {
                    break;
                }
            }
        });
    }

    io.run();

    report.output = benchmark.get_output();
    report.stats = benchmark.get_stats();

    return report;
}

struct context {
    asio::io_context io;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> guard = boost::asio::make_work_guard(io);
    std::thread thread {[this] { this->io.run(); }};

    context() = default;
};

template <typename Row, typename Query>
benchmark_report use_connection_pool_mult_threads(const benchmark_params& params, Query query) {
    constexpr bool parse_result = !std::is_same_v<Row, void>;

    assert(parse_result == params.parse_result);

    benchmark_report report;
    report.name = __func__;
    report.query = ozo::to_const_char(ozo::get_text(query));
    report.coroutines = params.coroutines;
    report.queue_capacity = params.queue_capacity;
    report.threads_number = params.threads_number;
    report.connections = params.connections;
    report.parse_result = parse_result;

    benchmark_t benchmark(params.coroutines * params.threads_number, params.duration);
    benchmark.set_print_progress(params.verbose);

    const ozo::connection_info connection_info(params.conn_string);
    ozo::connection_pool_config config;
    config.capacity = params.connections;
    config.queue_capacity = params.queue_capacity;
    std::vector<std::unique_ptr<context>> contexts;
    ozo::connection_pool pool(connection_info, config);
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
                    std::conditional_t<parse_result, std::vector<Row>, ozo::result> result;
                    ozo::error_code ec;
                    {
                        const auto connection = ozo::request(pool[io], query, request_timeout, ozo::into(result), yield[ec]);
                        if (ec) {
                            const std::lock_guard lock(cerr_mutex);
                            std::cerr << "coroutine " << token << ": " << ec.message() << '\n';
                            if (connection) {
                                std::cerr << "coroutine " << token << ": " << ozo::get_error_context(connection) << '\n';
                                std::cerr << "coroutine " << token << ": " << ozo::error_message(connection) << '\n';
                            }
                            std::abort();
                        }
                    }
                    if (!benchmark.thread_safe_step(result.size(), token)) {
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

    report.output = benchmark.get_output();
    report.stats = benchmark.get_stats();

    return report;
}

template <typename Row, typename Query>
benchmark_report run_benchmark(const std::string& name, const benchmark_params& params, Query query) {
    std::map<std::string, std::function<benchmark_report ()>> scenarios {{
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

benchmark_report run_benchmark(const std::string& name, const benchmark_params& params) {
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

enum class format {
    text,
    json
};

std::ostream& operator <<(std::ostream& stream, format value) {
    switch (value) {
        case format::text:
            return stream << "text";
        case format::json:
            return stream << "json";
    }
    return stream;
}

std::istream& operator >>(std::istream& stream, format& value) {
    std::string token;
    stream >> token;
    if (token == "text") {
        value = format::text;
    } else if (token == "json") {
        value = format::json;
    } else {
        throw std::invalid_argument("Invalid format: \"" + token + "\"");
    }
    return stream;
}

} // namespace

namespace nlohmann {

template <>
struct adl_serializer<std::chrono::steady_clock::duration> {
    static void to_json(json& j, const std::chrono::steady_clock::duration& value) {
        j = value.count();
    }

    static void from_json(const json&, std::chrono::steady_clock::duration&) {
        throw std::logic_error("std::chrono::steady_clock::duration is not implemented");
    }
};

template <>
struct adl_serializer<ozo::benchmark::stats> {
    static void to_json(json& j, const ozo::benchmark::stats& value) {
        if (value.mean_request_time) {
            j["mean_request_time"] = *value.mean_request_time;
        }
        if (value.median_request_time) {
            j["median_request_time"] = *value.median_request_time;
        }
        if (value.q90_request_time) {
            j["q90_request_time"] = *value.q90_request_time;
        }
        if (value.min_request_time) {
            j["min_request_time"] = *value.min_request_time;
        }
        if (value.max_request_time) {
            j["max_request_time"] = *value.max_request_time;
        }
        j["mean_request_speed"] = value.mean_request_speed;
        if (value.median_request_speed) {
            j["median_request_speed"] = *value.median_request_speed;
        }
        if (value.min_request_speed) {
            j["min_request_speed"] = *value.min_request_speed;
        }
        if (value.max_request_speed) {
            j["max_request_speed"] = *value.max_request_speed;
        }
        j["mean_read_rows_speed"] = value.mean_read_rows_speed;
        if (value.median_read_rows_speed) {
            j["median_read_rows_speed"] = *value.median_read_rows_speed;
        }
        if (value.min_read_rows_speed) {
            j["min_read_rows_speed"] = *value.min_read_rows_speed;
        }
        if (value.max_read_rows_speed) {
            j["max_read_rows_speed"] = *value.max_read_rows_speed;
        }
    }

    static void from_json(const json&, ozo::benchmark::stats&) {
        throw std::logic_error("ozo::benchmark::stats serialization is not implemented");
    }
};

template <>
struct adl_serializer<ozo::benchmark::step> {
    static void to_json(json& j, const ozo::benchmark::step& value) {
        j["duration"] = value.duration;
        j["rows_count"] = value.rows_count;
        j["requests_count"] = value.requests_count;
    }

    static void from_json(const json&, ozo::benchmark::step&) {
        throw std::logic_error("ozo::benchmark::step serialization is not implemented");
    }
};

template <>
struct adl_serializer<ozo::benchmark::output> {
    static void to_json(json& j, const ozo::benchmark::output& value) {
        j["steps"] = value.steps;
        j["requests"] = value.requests;
    }

    static void from_json(const json&, ozo::benchmark::output&) {
        throw std::logic_error("ozo::benchmark::output serialization is not implemented");
    }
};

template <>
struct adl_serializer<benchmark_report> {
    static void to_json(json& j, const benchmark_report& value) {
        j["name"] = value.name;
        j["query"] = value.query;
        if (value.coroutines) {
            j["coroutines"] = *value.coroutines;
        }
        if (value.connections) {
            j["connections"] = *value.connections;
        }
        if (value.queue_capacity) {
            j["queue_capacity"] = *value.queue_capacity;
        }
        if (value.threads_number) {
            j["threads_number"] = *value.threads_number;
        }
        if (value.parse_result) {
            j["parse_result"] = *value.parse_result;
        }
        j["output"] = value.output;
        j["stats"] = value.stats;
    }

    static void from_json(const json&, benchmark_report&) {
        throw std::logic_error("benchmark_report serialization is not implemented");
    }
};

} // namespace nlohmann

int main(int argc, char **argv) {
    using namespace ozo::benchmark;

    namespace po = boost::program_options;

    try {
        po::options_description options;

        options.add_options()
            ("help,h", "print help message")
            ("benchmark,b", po::value<std::string>(), "benchmark name to run")
            ("conninfo", po::value<std::string>()->default_value(""), "psql-like database connection info")
            ("query", po::value<query_type>()->default_value(query_type::simple), "query type (simple or complex)")
            ("coroutines", po::value<std::size_t>()->default_value(1), "number of parallel coroutines")
            ("queue", po::value<std::size_t>()->default_value(0), "connection pool queue capacity")
            ("threads", po::value<std::size_t>()->default_value(1), "number of threads")
            ("connections", po::value<std::size_t>(), "number of parallel coroutines (default: equal to coroutines + 1)")
            ("parse,p", "parse query result")
            ("verbose,v", "use verbose output")
            ("duration,d", po::value<std::uint64_t>()->default_value(31), "benchmark duration in seconds")
            ("format,f", po::value<format>()->default_value(format::text), "benchmark report format (text, json)")
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

        const auto name = variables.at("benchmark").as<std::string>();

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
        params.parse_result = variables.count("parse") > 0;
        params.verbose = variables.count("verbose") > 0;
        params.duration = std::chrono::seconds(variables.at("duration").as<std::uint64_t>());

        const auto report = run_benchmark(name, params);

        switch (variables.at("format").as<format>()) {
            case format::text:
                std::cout << report << std::endl;
                break;
            case format::json:
                std::cout << nlohmann::json(report) << std::endl;
                break;
        }

        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return -1;
    }
}
