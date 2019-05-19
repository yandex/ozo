#pragma once

#include <ozo/native_result_handle.h>
#include <ozo/type_traits.h>
#include <libpq-fe.h>

namespace ozo::impl {

enum class result_format : int {
    text = 0,
    binary = 1,
};

namespace pq {

inline oid_t pq_field_type(const PGresult& res, int column) noexcept {
    return PQftype(std::addressof(res), column);
}

inline result_format pq_field_format(const PGresult& res, int column) noexcept {
    return static_cast<result_format>(PQfformat(std::addressof(res), column));
}

inline const char* pq_get_value(const PGresult& res, int row, int column) noexcept {
    return PQgetvalue(std::addressof(res), row, column);
}

inline std::size_t pq_get_length(const PGresult& res, int row, int column) noexcept {
    return static_cast<std::size_t>(PQgetlength(std::addressof(res), row, column));
}

inline bool pq_get_isnull(const PGresult& res, int row, int column) noexcept {
    return PQgetisnull(std::addressof(res), row, column);
}

inline int pq_field_number(const PGresult& res, const char* name) noexcept {
    return PQfnumber(std::addressof(res), name);
}

inline int pq_nfields(const PGresult& res) noexcept {
    return PQnfields(std::addressof(res));
}

inline int pq_ntuples(const PGresult& res) noexcept {
    return PQntuples(std::addressof(res));
}

} // namespace pq

template <typename T>
inline oid_t field_type(T&& res, int column) noexcept {
    using pq::pq_field_type;
    return pq_field_type(std::forward<T>(res), column);
}

template <typename T>
inline result_format field_format(T&& res, int column) noexcept {
    using pq::pq_field_format;
    return pq_field_format(std::forward<T>(res), column);
}

template <typename T>
inline const char* get_value(T&& res, int row, int column) noexcept {
    using pq::pq_get_value;
    return pq_get_value(std::forward<T>(res), row, column);
}

template <typename T>
inline std::size_t get_length(T&& res, int row, int column) noexcept {
    using pq::pq_get_length;
    return pq_get_length(std::forward<T>(res), row, column);
}

template <typename T>
inline bool get_isnull(T&& res, int row, int column) noexcept {
    using pq::pq_get_isnull;
    return pq_get_isnull(std::forward<T>(res), row, column);
}

template <typename T>
inline int field_number(T&& res, const char* name) noexcept {
    using pq::pq_field_number;
    return pq_field_number(std::forward<T>(res), name);
}

template <typename T>
inline int nfields(T&& res) noexcept {
    using pq::pq_nfields;
    return pq_nfields(std::forward<T>(res));
}

template <typename T>
inline int ntuples(T&& res) noexcept {
    using pq::pq_ntuples;
    return pq_ntuples(std::forward<T>(res));
}

} // namespace ozo::impl

