#include "benchmark.h"

#include <ozo/connection_info.h>
#include <ozo/connection_pool.h>
#include <ozo/request.h>
#include <ozo/query_builder.h>
#include <ozo/shortcuts.h>

#include <boost/asio/io_service.hpp>
#include <boost/asio/spawn.hpp>

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

struct benchmark_params {
    std::string conn_string;
    std::size_t coroutines = 0;
    std::size_t threads_number = 0;
    std::size_t queue_capacity = 0;
    std::size_t connections = 0;
};

template <typename Row, typename Query>
void reopen_connection(const benchmark_params& params, Query query) {
    std::cout << '\n' << __func__ << std::endl;

    benchmark_t benchmark(1);
    asio::io_context io(1);
    ozo::connection_info connection_info(params.conn_string);

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
    ozo::connection_info connection_info(params.conn_string);

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
    const ozo::connection_info connection_info(params.conn_string);
    ozo::connection_pool_config config;
    config.capacity = params.coroutines + 1;
    config.queue_capacity = params.queue_capacity;
    ozo::connection_pool pool(connection_info, config);

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

} // namespace

int main(int argc, char **argv) {
    using namespace ozo::benchmark;
    using namespace ozo::literals;
    using namespace hana::literals;

    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <conninfo>\n";
        return 1;
    }

    const auto simple_query = "SELECT 1"_SQL.build();

    std::cout << "\nquery: " << ozo::to_const_char(ozo::get_text(simple_query)) << std::endl;

    benchmark_params params;
    params.conn_string = argv[1];

    reopen_connection<void>(params, simple_query);
    reuse_connection<void>(params, simple_query);
    params.coroutines = 1;
    use_connection_pool<void>(params, simple_query);
    params.coroutines = 2;
    use_connection_pool<void>(params, simple_query);
    params.threads_number = 2;
    params.coroutines = 2;
    params.connections = 4;
    params.queue_capacity = 0;
    use_connection_pool_mult_threads<void>(params, simple_query);
    params.threads_number = 2;
    params.coroutines = 2;
    params.connections = 2;
    params.queue_capacity = 4;
    use_connection_pool_mult_threads<void>(params, simple_query);
    params.coroutines = 1;
    use_connection_pool<std::tuple<std::int32_t>>(params, simple_query);

    const auto complex_query = (
        "SELECT typname, typnamespace, typowner, typlen, typbyval, typcategory, "_SQL +
        "typispreferred, typisdefined, typdelim, typrelid, typelem, typarray "_SQL +
        "FROM pg_type WHERE typtypmod = "_SQL + -1 + " AND typisdefined = "_SQL + true
    ).build();

    std::cout << "\nquery: " << ozo::to_const_char(ozo::get_text(complex_query)) << std::endl;
    params.coroutines = 1;
    use_connection_pool<void>(params, complex_query);
    params.coroutines = 1;
    use_connection_pool<pg_type>(params, complex_query);
    params.coroutines = 2;
    use_connection_pool<void>(params, complex_query);
    params.coroutines = 4;
    use_connection_pool<void>(params, complex_query);
    params.coroutines = 8;
    use_connection_pool<void>(params, complex_query);
    params.coroutines = 16;
    use_connection_pool<void>(params, complex_query);
    params.coroutines = 32;
    use_connection_pool<void>(params, complex_query);
    params.coroutines = 64;
    use_connection_pool<void>(params, complex_query);
    params.coroutines = 2;
    use_connection_pool<pg_type>(params, complex_query);
    params.coroutines = 4;
    use_connection_pool<pg_type>(params, complex_query);
    params.coroutines = 8;
    use_connection_pool<pg_type>(params, complex_query);
    params.threads_number = 2;
    params.coroutines = 8;
    params.connections = 16;
    params.queue_capacity = 0;
    use_connection_pool_mult_threads<void>(params, complex_query);
    params.threads_number = 2;
    params.coroutines = 8;
    params.connections = 8;
    params.queue_capacity = 16;
    use_connection_pool_mult_threads<void>(params, complex_query);
    params.threads_number = 4;
    params.coroutines = 8;
    params.connections = 32;
    params.queue_capacity = 0;
    use_connection_pool_mult_threads<void>(params, complex_query);
    params.threads_number = 4;
    params.coroutines = 8;
    params.connections = 16;
    params.queue_capacity = 32;
    use_connection_pool_mult_threads<void>(params, complex_query);
    params.threads_number = 8;
    params.coroutines = 8;
    params.connections = 64;
    params.queue_capacity = 0;
    use_connection_pool_mult_threads<void>(params, complex_query);
    params.threads_number = 8;
    params.coroutines = 8;
    params.connections = 32;
    params.queue_capacity = 64;
    use_connection_pool_mult_threads<void>(params, complex_query);

    return 0;
}
