#pragma once

#include <ozo/impl/async_request.h>

namespace ozo {
#ifdef OZO_DOCUMENTATION
/**
 * @brief Request query from a database
 *
 * The function sends request to a database and provides result via out parameter. The function can
 * be called as any of Boost.Asio asynchronous function with #CompletionToken. The request would be
 * cancelled if time constrain is reached while performing.
 *
 * @note The function does not particitate in ADL since could be implemented via functional object.
 *
 * @param provider --- #ConnectionProvider to get connection from.
 * @param query --- #Query or `ozo::query_builder` object to request from a database.
 * @param time_constraint --- request #TimeConstraint; this time constrain <b>includes</b> time for getting connection from provider.
 * @param out --- output object like Iterator, #InsertIterator or `ozo::result`.
 * @param token --- operation #CompletionToken.
 * @return deduced from #CompletionToken.
 *
 * ###Example
 *
 * @code
#include <ozo/request.h>
#include <ozo/connection_info.h>
#include <ozo/shortcuts.h>
#include <boost/asio.hpp>

int main() {
    boost::asio::io_context io;
    ozo::rows_of<std::int64_t, std::optional<std::string>> rows;
    auto conn_info = ozo::connection_info("host=... port=...");

    using namespace ozo::literals;
    using namespace std::chrono_literals;
    const auto query = "SELECT id, name FROM users_info WHERE amount>="_SQL + std::int64_t(25);

    ozo::request(conn_info[io], query, 500ms, ozo::into(rows),
            [&](ozo::error_code ec, auto conn) {
        if (ec) {
            std::cerr << ec.message() << " | " << error_message(conn);
            if (!is_null_recursive(conn)) {
                std::cerr << " | " << get_error_context(conn);
            }
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
 * @ingroup group-requests-functions
 */
template <typename ConnectionProvider, typename Query, typename TimeConstraint, typename Out, typename CompletionToken>
decltype(auto) request (ConnectionProvider&& provider, Query&& query, TimeConstraint time_constraint, Out out, CompletionToken&& token);

/**
 * @brief Request query from a database
 *
 * This function is time constrain free shortcut to `ozo::request()` function.
 * Its call is equal to `ozo::request(provider, query, ozo::none, out, token)` call.
 *
 * @note The function does not particitate in ADL since could be implemented via functional object.
 *
 * @param provider --- #ConnectionProvider to get connection from.
 * @param query --- #Query or `ozo::query_builder` object to request from a database.
 * @param out --- output object like Iterator, #InsertIterator or `ozo::result`.
 * @param token --- operation #CompletionToken.
 * @return deduced from #CompletionToken.
 * @ingroup group-requests-functions
 */
template <typename ConnectionProvider, typename Query, typename Out, typename CompletionToken>
decltype(auto) request (ConnectionProvider&& provider, Query&& query, Out out, CompletionToken&& token);

#else

template <typename Initiator>
struct request_op : base_async_operation <request_op<Initiator>, Initiator> {
    using base = typename request_op::base;
    using base::base;

    template <typename P, typename Q, typename TimeConstraint, typename Out, typename CompletionToken>
    decltype(auto) operator() (P&& provider, Q&& query, TimeConstraint t,
            Out out, CompletionToken&& token) const {
        static_assert(ConnectionProvider<P>, "provider should be a ConnectionProvider");
        static_assert(ozo::TimeConstraint<TimeConstraint>, "should model TimeConstraint concept");
        return async_initiate<CompletionToken, handler_signature<P>>(
            get_operation_initiator(*this), token, std::forward<P>(provider), t, std::forward<Q>(query), std::move(out));
    }

    template <typename P, typename Q, typename Out, typename CompletionToken>
    decltype(auto) operator()(P&& provider, Q&& query, Out out, CompletionToken&& token) const {
        return (*this)(std::forward<P>(provider), std::forward<Q>(query), none, std::move(out),
            std::forward<CompletionToken>(token));
    }

    template <typename OtherInitiator>
    constexpr static auto rebind_initiator(const OtherInitiator& other) {
        return request_op<OtherInitiator>{other};
    }
};

namespace detail {
struct initiate_async_request {
    template <typename Handler, typename P, typename Q, typename TimeConstraint, typename Out>
    constexpr void operator()(Handler&& h, P&& p, TimeConstraint t, Q&& q, Out out) const {
        impl::async_request(std::forward<P>(p), std::forward<Q>(q), t, std::move(out), std::forward<Handler>(h));
    }
};
} // namespace detail

constexpr request_op<detail::initiate_async_request> request;

#endif

} // namespace ozo
