#include <ozo/connection_info.h>
#include <ozo/request.h>
#include <ozo/shortcuts.h>

#include <boost/asio/io_service.hpp>
#include <boost/asio/spawn.hpp>

#include <iostream>

namespace asio = boost::asio;

int main(int argc, char **argv) {
    std::cout << "OZO request example" << std::endl;

    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <connection string>\n";
        return 1;
    }

    // Ozo perform all IO using Boost.Asio, so first thing we need to do is setup asio::io_context
    asio::io_context io;

    // To make a request we need to make a ConnectionSource. It knows how to connect to database using
    // connection string. See https://www.postgresql.org/docs/9.4/static/libpq-connect.html#LIBPQ-CONNSTRING
    // how to make a connection string.
    ozo::connection_info<> connection_info(argv[1]);

    const auto coroutine = [&] (asio::yield_context yield) {
        // The next step is bind asio::io_context with ConnectionSource to setup executor for all
        // callbacks. Default connection is a ConnectionProvider. If there is some problem with network
        // or database we don't want to wait indefinetely, so we establish connect timeout.
        const std::chrono::seconds connect_timeout(1);
        const auto connector = ozo::make_connector(connection_info, io, connect_timeout);

        // Request result is always set of rows. Client should take care of output object lifetime.
        ozo::rows_of<int> result;

        // Request operation require ConnectionProvider, query, output object for result and CompletionToken.
        // Also we setup request timeout and reference for error code to avoid throwing exceptions.
        // Function returns connection which can be used as ConnectionProvider for futher requests or to
        // get additional inforation about error through error context.
        const std::chrono::seconds request_timeout(1);
        boost::system::error_code ec;
        // This allows to use _SQL literals
        using namespace ozo::literals;
        const auto connection = ozo::request(
            connector,
            "SELECT 1"_SQL,
            request_timeout,
            ozo::into(result),
            yield[ec]
        );

        // When request is completed we check is there an error. This example should not produce any errors
        // if there are no problems with target database, network or permissions for given user in connection
        // string.
        if (ec) {
            std::cout << "Request failed with error: " << ec.message();
            // Here we should check if the connection is in null state to avoid UB.
            if (!ozo::is_null_recursive(connection)) {
                // Let's check libpq native error message and if so - print it out
                if (auto msg = ozo::error_message(connection); !msg.empty()) {
                    std::cout << ", error message: " << msg;
                }
                // Sometimes libpq native error message is not enough, so let's check
                // the additional error context from OZO
                if (auto ctx = ozo::get_error_context(connection); !ctx.empty()) {
                    std::cout << ", error context: " << ctx;
                }
            }
            std::cout << std::endl;
            return;
        }

        // Just print request result
        std::cout << "Selected:" << std::endl;
        for (auto value : result) {
            std::cout << std::get<0>(value) << std::endl;
        }
    };

    // All IO is asynchronous, therefore we have a choice here, what should be our CompletionToken.
    // We use Boost.Coroutines to write asynchronouse code in synchronouse style. Coroutine will be
    // called after io.run() is called.
    asio::spawn(io, coroutine);

    io.run();

    return 0;
}
