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

template <std::size_t coroutines>
using benchmark_t = ozo::benchmark::time_limit_benchmark<coroutines>;

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

template <typename Row, typename Query>
void reopen_connection(const std::string& conn_string, Query query) {
    std::cout << '\n' << __func__ << std::endl;

    benchmark_t<1> benchmark;
    asio::io_context io(1);
    ozo::connection_info connection_info(conn_string);

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
void reuse_connection(const std::string& conn_string, Query query) {
    std::cout << '\n' << __func__ << std::endl;

    benchmark_t<1> benchmark;
    asio::io_context io(1);
    ozo::connection_info connection_info(conn_string);

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
void use_connection_pool(const std::string& conn_string, Query query) {
    std::cout << '\n' << __func__ << std::endl;

    benchmark_t<1> benchmark;
    asio::io_context io(1);
    const ozo::connection_info connection_info(conn_string);
    ozo::connection_pool_config config;
    config.capacity = 2;
    config.queue_capacity = 0;
    ozo::connection_pool pool(connection_info, config);

    spawn(io, 0, [&] (asio::yield_context yield) {
        while (true) {
            std::conditional_t<std::is_same_v<Row, void>, ozo::result, std::vector<Row>> result;
            ozo::request(pool[io], query, request_timeout, ozo::into(result), yield);
            if (!benchmark.step(result.size())) {
                break;
            }
        }
    });

    io.run();
}

template <std::size_t coroutines, typename Row, typename Query>
void use_connection_pool_mult_connection(const std::string& conn_string, Query query) {
    std::cout << '\n' << __func__ << " coroutines=" << coroutines << std::endl;

    benchmark_t<coroutines> benchmark;
    asio::io_context io(1);
    const ozo::connection_info connection_info(conn_string);
    ozo::connection_pool_config config;
    config.capacity = coroutines + 1;
    config.queue_capacity = 0;
    ozo::connection_pool pool(connection_info, config);

    for (std::size_t token = 0; token < coroutines; ++token) {
        spawn(io, 0, [&, token] (asio::yield_context yield) {
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

template <std::size_t threads_number, std::size_t coroutines, typename Row, typename Query>
void use_connection_pool_mult_threads(const std::string& conn_string, Query query,
        std::size_t connections, std::size_t queue_capacity) {
    std::cout << '\n' << __func__
        << " threads_number=" << threads_number
        << " coroutines_per_thread=" << coroutines
        << " connections=" << connections
        << " queue_capacity=" << queue_capacity << std::endl;

    benchmark_t<coroutines * threads_number> benchmark;
    const ozo::connection_info connection_info(conn_string);
    ozo::connection_pool_config config;
    config.capacity = connections;
    config.queue_capacity = queue_capacity;
    std::vector<std::unique_ptr<context>> contexts;
    ozo::connection_pool pool(connection_info, config);
    std::atomic_size_t finished_coroutines {0};
    std::mutex mutex;
    std::unique_lock<std::mutex> lock(mutex);
    std::condition_variable coroutine_finished;

    for (std::size_t i = 0; i < threads_number; ++i) {
        contexts.emplace_back(std::make_unique<context>());
        auto& io = contexts.back()->io;
        for (std::size_t j = 0; j < coroutines; ++j) {
            const auto token = coroutines * i + j;
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

    if (coroutines * threads_number > 0) {
        coroutine_finished.wait(lock, [&] { return finished_coroutines >= coroutines * threads_number; });
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

    const std::string conn_string(argv[1]);

    const auto simple_query = "SELECT 1"_SQL.build();

    std::cout << "\nquery: " << ozo::to_const_char(ozo::get_text(simple_query)) << std::endl;
    reopen_connection<void>(conn_string, simple_query);
    reuse_connection<void>(conn_string, simple_query);
    use_connection_pool<void>(conn_string, simple_query);
    use_connection_pool_mult_connection<2, void>(conn_string, simple_query);
    use_connection_pool_mult_threads<2, 2, void>(conn_string, simple_query, 4, 0);
    use_connection_pool_mult_threads<2, 2, void>(conn_string, simple_query, 2, 4);
    use_connection_pool<std::tuple<std::int32_t>>(conn_string, simple_query);

    const auto complex_query = (
        "SELECT typname, typnamespace, typowner, typlen, typbyval, typcategory, "_SQL +
        "typispreferred, typisdefined, typdelim, typrelid, typelem, typarray "_SQL +
        "FROM pg_type WHERE typtypmod = "_SQL + -1 + " AND typisdefined = "_SQL + true
    ).build();

    std::cout << "\nquery: " << ozo::to_const_char(ozo::get_text(complex_query)) << std::endl;
    use_connection_pool<void>(conn_string, complex_query);
    use_connection_pool<pg_type>(conn_string, complex_query);
    use_connection_pool_mult_connection<2, void>(conn_string, complex_query);
    use_connection_pool_mult_connection<4, void>(conn_string, complex_query);
    use_connection_pool_mult_connection<8, void>(conn_string, complex_query);
    use_connection_pool_mult_connection<16, void>(conn_string, complex_query);
    use_connection_pool_mult_connection<32, void>(conn_string, complex_query);
    use_connection_pool_mult_connection<64, void>(conn_string, complex_query);
    use_connection_pool_mult_connection<2, pg_type>(conn_string, complex_query);
    use_connection_pool_mult_connection<4, pg_type>(conn_string, complex_query);
    use_connection_pool_mult_connection<8, pg_type>(conn_string, complex_query);
    use_connection_pool_mult_threads<2, 8, void>(conn_string, complex_query, 16, 0);
    use_connection_pool_mult_threads<2, 8, void>(conn_string, complex_query, 8, 16);
    use_connection_pool_mult_threads<4, 8, void>(conn_string, complex_query, 32, 0);
    use_connection_pool_mult_threads<4, 8, void>(conn_string, complex_query, 16, 32);
    use_connection_pool_mult_threads<8, 8, void>(conn_string, complex_query, 64, 0);
    use_connection_pool_mult_threads<8, 8, void>(conn_string, complex_query, 32, 64);

    return 0;
}
