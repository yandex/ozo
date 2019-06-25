#include <ozo/connection_info.h>
#include <ozo/request.h>
#include <ozo/shortcuts.h>
#include <ozo/failover/retry.h>

#include <boost/asio/io_service.hpp>
#include <boost/asio/spawn.hpp>

#include <iostream>

namespace asio = boost::asio;

const auto print_error = [](ozo::error_code ec, const auto& conn) {
    std::cout << "error code message: \"" << ec.message();
    // Here we should check if the connection is in null state to avoid UB.
    if (!ozo::is_null_recursive(conn)) {
        std::cout << "\", libpq error message: \"" << ozo::error_message(conn)
            << "\", error context: \"" << ozo::get_error_context(conn);
    }
    std::cout << "\"" << std::endl;
};

int main(int argc, char **argv) {
    std::cout << "OZO request example" << std::endl;

    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <connection string>\n";
        return 1;
    }

    asio::io_context io;

    auto conn_info = ozo::connection_info(argv[1]);

    asio::spawn(io, [&] (asio::yield_context yield) {
        ozo::rows_of<int> result;
        ozo::error_code ec;
        using namespace ozo::literals;
        using namespace std::chrono_literals;
        namespace failover = ozo::failover;
        using retry_options = failover::retry_options;
        // Here we will retry operation no more than 3 times on connection errors
        // Each try will have its own time constraint
        //   1st try will be limited by 1/3 sec.
        //   2nd try will be limited by  (1 - t(1st try)) / 2 sec, which is not less than 1/3 sec.
        //   3rd try will be limited by  1 - (t(1st try) + t(2nd try)), which is not less than 1/3 sec too.
        auto retry = 3*failover::retry(ozo::errc::connection_error)
        // We want to print out information about retries
                        .set(retry_options::on_retry, print_error);
        // Here a request call with retry fallback strategy
        auto conn = ozo::request[retry](conn_info[io], "SELECT 1"_SQL, 1s, ozo::into(result), yield[ec]);

        // When request is completed we check is there an error.
        if (ec) {
            std::cout << "Request failed; ";
            print_error(ec, conn);
            return;
        }

        // Just print request result
        std::cout << "Selected:" << std::endl;
        for (auto value : result) {
            std::cout << std::get<0>(value) << std::endl;
        }
    });

    io.run();

    return 0;
}
