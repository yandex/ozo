#include <ozo/async_request.h>
#include <ozo/connection_info.h>
#include <ozo/connection_pool.h>

#include <boost/asio/io_service.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/fusion/adapted/struct.hpp>
#include <boost/hana/for_each.hpp>
#include <boost/hana/tuple.hpp>

#include <iostream>

namespace ozo {

template <typename P, typename Q, typename Out, typename CompletionToken, typename = Require<ConnectionProvider<P>>>
auto request(P&& provider, Q&& query, Out&& out, CompletionToken&& token) {
    using signature_t = void (error_code, connectable_type<P>);
    async_completion<CompletionToken, signature_t> init(token);

    async_request(std::forward<P>(provider), std::forward<Q>(query), std::forward<Out>(out),
                  init.completion_handler);

    return init.result.get();
}

} // namespace ozo

namespace asio = ::boost::asio;
namespace hana = ::boost::hana;

constexpr const auto max_duration = std::chrono::seconds(5);

class Benchmark {
public:
    bool step() {
        if (++step_count % modulo == 0) {
            const auto finish = std::chrono::steady_clock::now();
            if (finish >= next_print) {
                using double_s = std::chrono::duration<double, std::ratio<1>>;
                const auto duration = finish - step_start;
                const auto request_per_second = step_count / std::chrono::duration_cast<double_s>(duration).count();
                modulo = std::size_t(std::round(request_per_second * 0.25));
                total_count += step_count;
                const auto total_duration = finish - start;
                std::cout << total_count << " requests done in "
                          << std::chrono::duration_cast<double_s>(total_duration).count() << "s, "
                          << request_per_second << " r/s\n";
                if (total_duration > max_duration) {
                    return false;
                }
                step_count = 0;
                step_start = finish;
                next_print += std::chrono::seconds(1);
            }
        }
        return true;
    }

private:
    std::size_t total_count = 0;
    std::size_t modulo = 1;
    std::size_t step_count = 0;
    const std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point next_print = start + std::chrono::seconds(1);
    std::chrono::steady_clock::time_point step_start = start;
};

struct pg_type {
    ozo::name_oid typname;
    ozo::oid_t typnamespace;
    ozo::oid_t typowner;
    std::int16_t typlen;
    bool typbyval;
    char typcategory;
    bool typispreferred;
    bool typisdefined;
    char typdelim;
    ozo::oid_t typrelid;
    ozo::oid_t typelem;
    ozo::oid_t typarray;
};

BOOST_FUSION_ADAPT_STRUCT(pg_type, typname, typnamespace, typowner, typlen, typbyval, typcategory, typispreferred,
                          typisdefined, typdelim, typrelid, typelem, typarray)

template <class MakeQuery>
void reuse_connection_info(const std::string& conn_string, MakeQuery&& make_query) {
    std::cout << __func__ << '\n';

    Benchmark benchmark;
    asio::io_context io(1);
    ozo::connection_info<> connection_info(io, conn_string);
    const auto query = make_query();

    asio::spawn(io, [&] (auto yield) {
        try {
            while (benchmark.step()) {
                ozo::result result;
                ozo::request(connection_info, query, result, yield);
            }
        } catch (const std::exception& e) {
            std::cout << e.what() << '\n';
        }
    });

    io.run();
}

template <class Result, class MakeQuery>
void reuse_connection_info_and_parse_result(const std::string& conn_string, MakeQuery&& make_query) {
    std::cout << __func__ << '\n';

    Benchmark benchmark;
    asio::io_context io(1);
    ozo::connection_info<> connection_info(io, conn_string);
    const auto query = make_query();

    asio::spawn(io, [&] (auto yield) {
        try {
            while (benchmark.step()) {
                std::vector<Result> result;
                ozo::request(connection_info, query, std::back_inserter(result), yield);
            }
        } catch (const std::exception& e) {
            std::cout << e.what() << '\n';
        }
    });

    io.run();
}

template <class MakeQuery>
void reuse_connection(const std::string& conn_string, MakeQuery&& make_query) {
    std::cout << __func__ << '\n';

    Benchmark benchmark;
    asio::io_context io(1);
    ozo::connection_info<> connection_info(io, conn_string);
    const auto query = make_query();

    asio::spawn(io, [&] (auto yield) {
        try {
            auto connection = ozo::get_connection(connection_info, yield);
            while (benchmark.step()) {
                ozo::result result;
                ozo::request(connection, query, result, yield);
            }
        } catch (const std::exception& e) {
            std::cout << e.what() << '\n';
        }
    });

    io.run();
}

template <class Result, class MakeQuery>
void reuse_connection_and_parse_result(const std::string& conn_string, MakeQuery&& make_query) {
    std::cout << __func__ << '\n';

    Benchmark benchmark;
    asio::io_context io(1);
    ozo::connection_info<> connection_info(io, conn_string);
    const auto query = make_query();

    asio::spawn(io, [&] (auto yield) {
        try {
            auto connection = ozo::get_connection(connection_info, yield);
            while (benchmark.step()) {
                std::vector<Result> result;
                ozo::request(connection, query, std::back_inserter(result), yield);
            }
        } catch (const std::exception& e) {
            std::cout << e.what() << '\n';
        }
    });

    io.run();
}

template <class MakeQuery>
void use_connection_pool(const std::string& conn_string, MakeQuery&& make_query) {
    std::cout << __func__ << '\n';

    Benchmark benchmark;
    asio::io_context io(1);
    const ozo::connection_info<> connection_info(io, conn_string);
    auto pool = ozo::make_connection_pool(connection_info, 2, 0);
    auto provider = ozo::make_provider(pool, io);
    const auto query = make_query();

    asio::spawn(io, [&] (auto yield) {
        try {
            while (benchmark.step()) {
                ozo::result result;
                ozo::error_code ec;
                ozo::request(provider, query, result, yield);
            }
        } catch (const std::exception& e) {
            std::cout << e.what() << '\n';
        }
    });

    io.run();
}

template <class Result, class MakeQuery>
void use_connection_pool_and_parse_result(const std::string& conn_string, MakeQuery&& make_query) {
    std::cout << __func__ << '\n';

    Benchmark benchmark;
    asio::io_context io(1);
    const ozo::connection_info<> connection_info(io, conn_string);
    auto pool = ozo::make_connection_pool(connection_info, 2, 0);
    auto provider = ozo::make_provider(pool, io);
    const auto query = make_query();

    asio::spawn(io, [&] (auto yield) {
        try {
            while (benchmark.step()) {
                std::vector<Result> result;
                ozo::request(provider, query, std::back_inserter(result), yield);
            }
        } catch (const std::exception& e) {
            std::cout << e.what() << '\n';
        }
    });

    io.run();
}

int main(int argc, char **argv) {
    using namespace ozo::literals;
    using namespace hana::literals;

    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <conninfo> <connections>\n";
        return 1;
    }

    const std::string conn_string(argv[1]);

    constexpr auto make_simple_query = [] {
        return "SELECT 1"_SQL.build();
    };

    constexpr auto make_complex_query = [] {
        return ("SELECT typname, typnamespace, typowner, typlen, typbyval, typcategory, "_SQL +
                "typispreferred, typisdefined, typdelim, typrelid, typelem, typarray "_SQL +
                "FROM pg_type WHERE typtypmod = "_SQL +
                -1 + " AND typisdefined = "_SQL + true).build();
    };

    hana::for_each(
        hana::make_tuple(
            hana::make_tuple(std::tuple<std::int32_t> {}, make_simple_query),
            hana::make_tuple(pg_type {}, make_complex_query)
        ),
        [&] (const auto& v) {
            std::cout << "query: " << ozo::get_text(v[1_c]()) << '\n';
            reuse_connection_info(conn_string, v[1_c]);
            reuse_connection_info_and_parse_result<std::decay_t<decltype(v[0_c])>>(conn_string, v[1_c]);
            reuse_connection(conn_string, v[1_c]);
            reuse_connection_and_parse_result<std::decay_t<decltype(v[0_c])>>(conn_string, v[1_c]);
            use_connection_pool(conn_string, v[1_c]);
            use_connection_pool_and_parse_result<std::decay_t<decltype(v[0_c])>>(conn_string, v[1_c]);
        }
    );

    return 0;
}
