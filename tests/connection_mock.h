#pragma once

#include "test_asio.h"
#include <ozo/impl/io.h>

namespace ozo {
namespace testing {

enum class native_handle {bad, good};

inline bool connection_status_bad(const native_handle* h) {
    return *h == native_handle::bad;
}
struct pg_result {
    ExecStatusType status;
    error_code error;
};

inline decltype(auto) pq_result_status(const pg_result& res) noexcept {
    return res.status;
}

inline error_code pq_result_error(const pg_result& res) noexcept {
    return res.error;
}

using ozo::empty_oid_map;

struct connection_mock {
    virtual int set_nonblocking() = 0;
    virtual int send_query_params() = 0;
    virtual int consume_input() = 0;
    virtual bool is_busy() const = 0;
    virtual ozo::impl::query_state flush_output() = 0;
    virtual boost::optional<pg_result> get_result() = 0;

    virtual int connect_poll() const = 0;
    virtual ozo::error_code start_connection(const std::string&) = 0;
    virtual ozo::error_code assign_socket() = 0;
    virtual void async_request() = 0;
    virtual ozo::error_code rebind_io_context() = 0;

    virtual ~connection_mock() = default;
};

struct connection_gmock : connection_mock {
    MOCK_METHOD0(set_nonblocking, int());
    MOCK_METHOD0(send_query_params, int());
    MOCK_METHOD0(consume_input, int());
    MOCK_CONST_METHOD0(is_busy, bool());
    MOCK_METHOD0(flush_output, ozo::impl::query_state());
    MOCK_METHOD0(get_result, boost::optional<pg_result>());

    MOCK_CONST_METHOD0(connect_poll, int());
    MOCK_METHOD1(start_connection, ozo::error_code(const std::string&));
    MOCK_METHOD0(assign_socket, ozo::error_code());
    MOCK_METHOD0(async_request, void());
    MOCK_METHOD0(rebind_io_context, ozo::error_code());

};

inline boost::optional<pg_result> make_pg_result(
        ExecStatusType status, error_code error) {
    return pg_result{status, error};
}

struct fake_query {
    template <typename T>
    friend decltype(auto) make_binary_query(fake_query query,
            const ozo::oid_map_t<T>&) {
        return std::move(query);
    }
};

template <typename OidMap = empty_oid_map>
struct connection {
    using handle_type = std::unique_ptr<native_handle>;

    handle_type handle_;
    stream_descriptor socket_;
    OidMap oid_map_;
    connection_mock* mock_ = nullptr;
    std::string error_context_;

    friend OidMap& get_connection_oid_map(connection& conn) {
        return conn.oid_map_;
    }
    friend const OidMap& get_connection_oid_map(const connection& conn) {
        return conn.oid_map_;
    }
    friend auto& get_connection_socket(connection& conn) {
        return conn.socket_;
    }
    friend const auto& get_connection_socket(const connection& conn) {
        return conn.socket_;
    }
    friend handle_type& get_connection_handle(connection& conn) {
        return conn.handle_;
    }
    friend const handle_type& get_connection_handle(const connection& conn) {
        return conn.handle_;
    }
    friend std::string& get_connection_error_context(connection& conn) {
        return conn.error_context_;
    }
    friend const std::string& get_connection_error_context(const connection& conn) {
        return conn.error_context_;
    }

    friend int pq_set_nonblocking(connection& c) {
        return c.mock_->set_nonblocking();
    }

    template <typename ...Ts>
    friend int pq_send_query_params(connection& c, const fake_query&) noexcept {
        return c.mock_->send_query_params();
    }

    friend int pq_consume_input(connection& c) noexcept {
        return c.mock_->consume_input();
    }

    friend bool pq_is_busy(connection& c) noexcept {
        return c.mock_->is_busy();
    }

    friend ozo::impl::query_state pq_flush_output(connection& c) noexcept {
        return c.mock_->flush_output();
    }

    friend decltype(auto) pq_get_result(connection& c) noexcept {
        return c.mock_->get_result();
    }



    friend int pq_connect_poll(connection& c) {
        return c.mock_->connect_poll();
    }

    friend ozo::error_code pq_start_connection(
            connection& c, const std::string& conninfo) {
        return c.mock_->start_connection(conninfo);
    }

    friend ozo::error_code pq_assign_socket(connection& c) {
        return c.mock_->assign_socket();
    }

    template <typename Q, typename Out, typename Handler>
    friend void async_request(std::shared_ptr<connection>&& provider, Q&&, Out&&, Handler&&) {
        provider->mock_->async_request();
    }

    template <typename IoContext>
    friend ozo::error_code rebind_connection_io_context(connection& c, IoContext&) {
        return c.mock_->rebind_io_context();
    }
};

template <typename ...Ts>
using connection_ptr = std::shared_ptr<connection<Ts...>>;

static_assert(ozo::Connection<connection<>>,
    "connection does not meet Connection requirements");
static_assert(ozo::ConnectionWrapper<connection_ptr<>>,
    "connection_ptr does not meet ConnectionWrapper requirements");
static_assert(ozo::Connectable<connection<>>,
    "connection does not meet Connectable requirements");
static_assert(ozo::Connectable<connection_ptr<>>,
    "connection_ptr does not meet Connectable requirements");

template <typename OidMap = empty_oid_map>
inline auto make_connection(connection_mock& mock, io_context& io,
        stream_descriptor_mock& socket_mock, OidMap oid_map = OidMap{}) {
    return std::make_shared<connection<OidMap>>(connection<OidMap>{
            std::make_unique<native_handle>(native_handle::bad),
            stream_descriptor{io, socket_mock},
            oid_map,
            std::addressof(mock),
            ""
        });
}

} // namespace testing
} // namespace ozo
