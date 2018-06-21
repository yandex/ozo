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
    virtual ~connection_mock() = default;
};

struct connection_gmock : connection_mock {
    MOCK_METHOD0(set_nonblocking, int());
    MOCK_METHOD0(send_query_params, int());
    MOCK_METHOD0(consume_input, int());
    MOCK_CONST_METHOD0(is_busy, bool());
    MOCK_METHOD0(flush_output, ozo::impl::query_state());
    MOCK_METHOD0(get_result, boost::optional<pg_result>());
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

inline auto make_connection(connection_mock& mock, io_context& io,
        stream_descriptor_mock& socket_mock) {
    return std::make_shared<connection<>>(connection<>{
            std::make_unique<native_handle>(native_handle::bad),
            stream_descriptor{io, socket_mock},
            empty_oid_map{},
            std::addressof(mock),
            ""
        });
}

} // namespace testing
} // namespace ozo
