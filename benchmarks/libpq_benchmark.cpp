#include "benchmark.h"

#include <libpq-fe.h>

#include <thread>

template <class Benchmark>
void run_benchmark(Benchmark& benchmark, const char* conninfo) {
    using namespace std::string_literals;

    const auto connection = PQconnectdb(conninfo);

    if (PQstatus(connection) != CONNECTION_OK) {
        PQfinish(connection);
        throw std::runtime_error("Connection to database failed: "s + PQerrorMessage(connection));
    }

    while (true) {
        const int typtypmod = -1;
        const bool typisdefined = true;
        const char *parameters[2] = {
            reinterpret_cast<const char*>(&typtypmod),
            reinterpret_cast<const char*>(&typisdefined)
        };
        const int lengths[2] = {sizeof(typtypmod), sizeof(typisdefined)};
        const int binary_format = 1;
        const int formats[2] = {binary_format, binary_format};

        const auto result = PQexecParams(
            connection,
            R"SQL(
                SELECT typname, typnamespace, typowner, typlen, typbyval, typcategory,
                    typispreferred, typisdefined, typdelim, typrelid, typelem, typarray
                FROM pg_type
                WHERE typtypmod = $1::int AND typisdefined = $2::boolean
            )SQL",
            2,
            nullptr,
            parameters,
            lengths,
            formats,
            binary_format
        );

        if (PQresultStatus(result) != PGRES_TUPLES_OK) {
            PQclear(result);
            PQfinish(connection);
            throw std::runtime_error("Query failed: "s + PQresultErrorMessage(result));
        }

        if (!benchmark.step(PQntuples(result))) {
            PQclear(result);
            break;
        }

        PQclear(result);
    }

    PQfinish(connection);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <conninfo>" << std::endl;
        return 1;
    }

    using namespace ozo::benchmark;

    rows_count_limit_benchmark<std::mutex> benchmark(10000000);

    benchmark.start();

    std::vector<std::thread> threads;
    for (int i = 0; i < 8; ++i) {
        threads.emplace_back([&, i] {
            try {
                run_benchmark(benchmark, argv[1]);
            } catch (const std::exception& e) {
                std::cout << "Thread " << i << " failed: " << e.what() << '\n';
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    return 0;
}
