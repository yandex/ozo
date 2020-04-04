#pragma once

#include <ozo/impl/result.h>
#include <boost/iterator/iterator_facade.hpp>
#include <memory>
#include <vector>


namespace ozo {

/**
 * @brief Database request result value proxy
 *
 * Provides access to values of a database request result. The library is designed
 * to do not obligate a user to have a deal with raw and untyped results representation.
 * But sometimes it is really needed to have an access to raw representation of the
 * result. E.g. for reducing memory consumption or performance reason. For that case
 * the library provides this mechanism.
 *
 * @ingroup group-requests-types
 */
#ifndef OZO_DOCUMENTATION
template <typename Result>
#endif
class value {
public:
    struct coordinates {
        const Result* res;
        int row;
        int col;
    };

    value(const coordinates& v) noexcept : v_(v) {}

    /**
     * Get value type OID
     *
     * @return oid_t --- type OID
     */
    oid_t oid() const noexcept {
        return impl::field_type(res(), column());
    }

    /**
     * Determine whether the value is in text format. Should be always false for current
     * implementation.
     *
     * @return `true` --- value in text format
     * @return `false` --- value in binary format
     */
    bool is_text() const noexcept {
        return impl::field_format(res(), column()) == impl::result_format::text;
    }

    /**
     * Determine whether the value is in binary format. Should be always true for current
     * implementation.
     *
     * @return `true` --- value in binary format
     * @return `false` --- value in text format
     */
    bool is_binary() const noexcept {
        return impl::field_format(res(), column()) == impl::result_format::binary;
    }

    /**
     * Get value raw data buffer access.
     *
     * @return const char* --- pointer to raw data buffer
     * @sa ozo::value::size()
     */
    const char* data() const noexcept {
        return impl::get_value(res(), row(), column());
    }

    /**
     * Get value raw data buffer size.
     *
     * @return std::size_t --- size of raw data buffer in bytes.
     * @sa ozo::value::data()
     */
    std::size_t size() const noexcept {
        return impl::get_length(res(), row(), column());
    }

    /**
     * Determine whether the value is in `NULL` state.
     *
     * @return `true` --- if value is `NULL`
     * @return `false` --- value is not `NULL`
     */
    bool is_null() const noexcept {
        return impl::get_isnull(res(), row(), column());
    }

private:
    int column() const noexcept { return v_.col;}
    int row() const noexcept { return v_.row;}
    const Result& res() const noexcept { return *(v_.res);}

    coordinates v_;
};

/**
 * @brief Database request result row proxy
 *
 * @ingroup group-requests-types
 */
#ifndef OZO_DOCUMENTATION
template <typename Result>
#endif
class row {
public:
    using value = ozo::value<Result>;
    using coordinates = typename value::coordinates;

#ifdef OZO_DOCUMENTATION
    /**
     * Constant iterator on value in the row.
     *
     * Random access iterator. Provides access to a `ozo::value` object.
     * Since `ozo::basic_result` provides read-only access to a database
     * request result, all the iterators provide a read-only access to
     * a `ozo::value` object.
     */
    using const_iterator = <implementation defined>;
#else
    class const_iterator : public boost::iterator_facade<
        const_iterator,
        value,
        boost::random_access_traversal_tag,
        value,
        int
    > {
    public:
        const_iterator() = default;
        const_iterator(const coordinates& v) noexcept : v_{v} {}

    private:
        value dereference() const noexcept { return {v_}; }

        bool equal(const const_iterator& rhs) const noexcept {
            return v_.res == rhs.v_.res && v_.col == rhs.v_.col && v_.row == rhs.v_.row;
        }

        void increment() noexcept { advance(1); }
        void decrement() noexcept { advance(-1); }
        void advance(int n) noexcept { v_.col += n; }

        int distance_to(const const_iterator& z) noexcept { return z.col - v_.col; }

        coordinates v_ {nullptr, 0, 0};

        friend class boost::iterator_core_access;
    };
#endif
    /**
     * Iterator on value in the row, alias on `iterator` class.
     */
    using iterator = const_iterator;

    row(const coordinates& first) noexcept : first_(first) {}

    /**
     * Iterator on the first of row values sequence.
     *
     * @return const_iterator --- iterator on the first of row values sequence
     */
    const_iterator begin() const noexcept { return {first_}; }

    /**
     * Iterator on end of row values sequence.
     *
     * @return const_iterator --- iterator on the end of row values sequence
     */
    const_iterator end() const noexcept { return begin() + size(); }

    /**
     * Find value by field name.
     *
     * @param name --- value field name to find.
     * @return `const_iterator` --- iterator on found value.
     * @return `end()` --- in no field with given name found.
     */
    const_iterator find(const char* name) const noexcept {
        int i = impl::field_number(*(first_.res), name);
        return i == -1 ? end() : begin() + i;
    }

    /**
     * Get value by field index.
     *
     * Valid index is in range `[0, size())`. No index-in-range check is performing.
     * @note If given index is out of range the result is UB.
     *
     * @param index --- index of the value field.
     * @return `ozo::value` --- proxy object on the value.
     */
    value operator[] (int index) const noexcept { return *(begin() + index); }

    /**
     * Get count of values in row.
     *
     * @return `std::size_t` --- count of values in row.
     */
    std::size_t size() const noexcept { return impl::nfields(*(first_.res)); }

    /**
     * Indicates if there are no values in row.
     *
     * @return `true` --- no values in the row, `size() == 0`, `begin() == end()`.
     * @return `false` --- row is not empty, `size() > 0`, `begin() != end()`.
     */
    [[nodiscard]] bool empty() const noexcept { return size() == 0; }

    /**
     * Get value by field index with range check
     *
     * If index in not range `[0, size())` throws `std::out_of_range`.
     *
     * @param index --- index of value in the row.
     * @return `ozo::value` --- proxy object to a value.
     */
    value at(int index) const {
        if (index < 0 || static_cast<std::size_t>(index) >= size()) {
            throw std::out_of_range("ozo::row::at() field index "
                + std::to_string(index) + " out of range");
        }
        return (*this)[index];
    }

    /**
     * Get value by field name with range check
     *
     * If value with field name not found throws `std::out_of_range`.
     *
     * @param name --- index of value in the row.
     * @return value --- proxy object to a value.
     */
    value at(const char* name) const {
        auto i = find(name);
        if (i == end()) {
            throw std::out_of_range(std::string("ozo::row::at() no such field name ")
                + name);
        }
        return *i;
    }

private:
    coordinates first_;
};

/**
 * @brief Database raw result representation
 *
 * This class provides access to the raw representation of a database request result. It
 * models range of rows. Each row can be accessed via index or iterator.
 * @tparam T --- underlying native result handler type, in common case `ozo::pg::result`.
 *
 * @ingroup group-requests-types
 */
template <typename T>
class basic_result {
public:
    using handle_type = T;
    using native_handle_type = decltype(std::addressof(*std::declval<handle_type>()));
    using row = ozo::row<std::decay_t<decltype(*std::declval<handle_type>())>>;
    using value = typename row::value;
    using coordinates = typename row::coordinates;

#ifdef OZO_DOCUMENTATION
    /**
     * Constant iterator on row in the result.
     *
     * Random access iterator. Provides access to a `ozo::row` object.
     * Since `ozo::basic_result` provides read-only access to a database
     * request result, all the iterators provide a read-only access to
     * a `ozo::row` object.
     */
    using const_iterator = <implementation defined>;
#else
    class const_iterator : public boost::iterator_facade<
        const_iterator,
        row,
        boost::random_access_traversal_tag,
        row,
        int
    > {
    public:
        const_iterator() = default;
        const_iterator(const coordinates& v) noexcept : v_{v} {}

    private:
        row dereference() const noexcept { return {v_}; }

        bool equal(const const_iterator& rhs) const noexcept {
            return v_.res == rhs.v_.res && v_.row == rhs.v_.row;
        }

        void increment() noexcept { advance(1); }
        void decrement() noexcept { advance(-1); }
        void advance(int n) noexcept { v_.row += n; }

        int distance_to(const const_iterator& z) noexcept { return z.row - v_.row; }

        coordinates v_ {nullptr, 0, 0};

        friend class boost::iterator_core_access;
    };
#endif

    /**
     * Iterator on value in the row, alias on `iterator` class.
     */
    using iterator = const_iterator;

    basic_result() = default;
    basic_result(handle_type res) noexcept(noexcept(handle_type(std::move(res))))
    : res_(std::move(res)) {}

    /**
     * Iterator on the first row of the result.
     *
     * @return const_iterator --- iterator on the first row
     */
    const_iterator begin() const noexcept { return {{std::addressof(*res_), 0, 0}}; }

    /**
     * Iterator on end of row sequence.
     *
     * @return const_iterator --- iterator on end of row sequence
     */
    const_iterator end() const noexcept { return begin() + size(); }

    /**
     * Get count of rows.
     *
     * @return `std::size_t` --- count of rows.
     */
    std::size_t size() const noexcept { return impl::ntuples(*res_);}

    /**
     * Determine whether the result is empty.
     *
     * @return `true` --- no ros in the result, `size() == 0`, `begin() == end()`.
     * @return `false` --- row is not empty, `size() > 0`, `begin() != end()`.
     */
    [[nodiscard]] bool empty() const noexcept { return size() == 0; }

    /**
     * Get row by index.
     *
     * Valid index is in range `[0, size())`. No index-in-range check is performing.
     *
     * @note If given index is out of range the result is UB.
     * @param index --- index of a row.
     * @return `ozo::row` --- proxy object to a row.
     */
    row operator[] (int i) const noexcept { return *(begin() + i); }

    /**
     * Get row by index with range check.
     *
     * If index in not range `[0, size())` throws `std::out_of_range`.
     *
     * @param index --- index of a row.
     * @return `ozo::row` --- proxy object to a row.
     */
    row at(int i) const {
        if (i < 0 || static_cast<std::size_t>(i) >= size()) {
            throw std::out_of_range("ozo::result::at() index " + std::to_string(i) + " out of range");
        }
        return (*this)[i];
    }

    /**
     * Get the native `libpq` handle representation.
     *
     * This function may be used to obtain the underlying representation of the handle.
     * This is intended to allow access to native handle functionality that is not otherwise provided.
     *
     * @return native_handle_type --- native handle representation
     */
    native_handle_type native_handle() const noexcept {
        return std::addressof(*res_);
    }

private:
    handle_type res_;
};

/**
 * @brief Database raw result representation
 *
 * Stores raw request result. The result object is useful then it needs to get an access
 * to raw data representation or the underlying `libpq` handle.
 *
 * @ingroup group-requests-types
 */
using result = basic_result<ozo::pg::result>;

template <typename T>
auto make_result(T&& handle) {
    return ozo::basic_result<std::decay_t<T>>(std::forward<T>(handle));
}

} // namespace ozo
