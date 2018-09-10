#pragma once

#include <ozo/impl/async_request.h>

namespace ozo {

/**
 * @brief Send request to a database and provides query result (time-out version).
 * @ingroup group-requests
 *
 * The function sends request to a database and provides result via out parameter. The function can
 * be called as any of Boost.Asio asynchronous function with #CompletionToken.
 *
 * ###Basic usage
 *
 * @code
#include <ozo/request.h>
#include <ozo/connection_info.h>
#include <ozo/shortcuts.h>
#include <boost/asio.hpp>
int main() {
    boost::asio::io_context io;
    ozo::rows_of<std::int64_t, std::optional<std::string>> rows;
    ozo::connection_info<> conn_info("host=... port=...");

    using namespace ozo::literals;
    const auto query = "SELECT id, name FROM users_info WHERE amount>="_SQL + std::int64_t(25);

    ozo::request(ozo::make_connector(conn_info, io, 500ms), query, 100ms, ozo::into(rows),
            [&](ozo::error_code ec, auto conn) {
        if (ec) {
            std::cerr << ec.message()
                      << " | " << error_message(conn)
                      << " | " << get_error_context(conn);
            return;
        };
        assert(ozo::connection_good(conn));
        std::cout << "id" << '\t' << "name" << std::endl;
        for(auto& row: res) {
            std::cout << std::get<0>(row) << '\t'
                    << std::get<1>(row) << std::endl;
        }
    });
    io.run();
}
 * @endcode
 * @param provider --- #ConnectionProvider to get connection from.
 * @param query --- #Query or `ozo::query_builder` object to send to a database.
 * @param timeout --- request timeout.
 * @param out --- output object like Iterator, #InsertIterator or `ozo::result`.
 * @param token --- any valid of #CompletionToken.
 * @return depends on #CompletionToken.
 */
template <typename P, typename Q, typename Out, typename CompletionToken, typename = Require<ConnectionProvider<P>>>
inline auto request(P&& provider, Q&& query, const time_traits::duration& timeout, Out out, CompletionToken&& token) {
    using signature_t = void (error_code, connection_type<P>);
    async_completion<CompletionToken, signature_t> init(token);

    impl::async_request(std::forward<P>(provider), std::forward<Q>(query), timeout, std::move(out),
            init.completion_handler);

    return init.result.get();
}

/**
 * @brief Send request to a database and provides query result.
 * @ingroup group-requests
 *
 * This is time-out free version of the `ozo::request`.
 * @param provider --- #ConnectionProvider to get connection from.
 * @param query --- #Query or `ozo::query_builder` object to send to a database.
 * @param out --- output object like Iterator, #InsertIterator or `ozo::result`.
 * @param token --- any valid of #CompletionToken.
 * @return depends on #CompletionToken.
 */
template <typename P, typename Q, typename Out, typename CompletionToken, typename = Require<ConnectionProvider<P>>>
inline auto request(P&& provider, Q&& query, Out out, CompletionToken&& token) {
    return request(
        std::forward<P>(provider),
        std::forward<Q>(query),
        time_traits::duration::max(),
        std::move(out),
        std::forward<CompletionToken>(token)
    );
}

} // namespace ozo
