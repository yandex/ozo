#pragma once

#include <ozo/impl/io.h>
#include <ozo/type_traits.h>
#include <ozo/native_result_handle.h>

#include <libpq-fe.h>

#include <boost/iterator/iterator_facade.hpp>
#include <memory>
#include <vector>


namespace ozo {

template <typename Result>
class value {
public:
    struct coordinates {
        const Result* res;
        int row;
        int col;
    };

    value(const coordinates& v) : v_(v) {}

    oid_t oid() const noexcept {
        return impl::field_type(res(), column());
    }

    bool is_text() const noexcept {
        return impl::field_format(res(), column()) == impl::result_format::text;
    }

    bool is_binary() const noexcept {
        return impl::field_format(res(), column()) == impl::result_format::binary;
    }

    const char* data() const noexcept {
        return impl::get_value(res(), row(), column());
    }

    std::size_t size() const noexcept {
        return impl::get_length(res(), row(), column());
    }

    bool is_null() const noexcept {
        return impl::get_isnull(res(), row(), column());
    }

private:
    int column() const noexcept { return v_.col;}
    int row() const noexcept { return v_.row;}
    const Result& res() const noexcept { return *(v_.res);}

    coordinates v_;
};

template <typename T, typename Result>
inline const T* data(const value<Result>& v) {
    return reinterpret_cast<const T*>(v.data());
}

template <typename Result>
class row {
public:
    using value = ozo::value<Result>;
    using coordinates = typename value::coordinates;

    class const_iterator : public boost::iterator_facade<
        const_iterator,
        value,
        boost::random_access_traversal_tag,
        value,
        int
    > {
    public:
        const_iterator() = default;
        const_iterator(const coordinates& v) : v_{v} {}

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

    using iterator = const_iterator;

    row(const coordinates& first) : first_(first) {}

    const_iterator begin() const noexcept { return {first_}; }

    const_iterator end() const noexcept { return begin() + size(); }

    const_iterator find(const char* name) const noexcept {
        int i = impl::field_number(*(first_.res), name);
        return i == -1 ? end() : begin() + i;
    }

    value operator[] (int i) const noexcept { return *(begin() + i); }

    std::size_t size() const noexcept { return impl::nfields(*(first_.res)); }

    bool empty() const noexcept { return size() == 0; }

    value at(int column_index) const {
        if (column_index < 0 || static_cast<std::size_t>(column_index) >= size()) {
            throw std::out_of_range("ozo::row::at() column index "
                + std::to_string(column_index) + " out of range");
        }
        return (*this)[column_index];
    }

    value at(const char* column_name) const {
        auto i = find(column_name);
        if (i == end()) {
            throw std::out_of_range(std::string("ozo::row::at() no such column name ")
                + column_name);
        }
        return *i;
    }

private:
    coordinates first_;
};

template <typename T>
class basic_result {
public:
    using handle_type = T;
    using row = ozo::row<std::decay_t<decltype(*std::declval<handle_type>())>>;
    using value = typename row::value;
    using coordinates = typename row::coordinates;

    class const_iterator : public boost::iterator_facade<
        const_iterator,
        row,
        boost::random_access_traversal_tag,
        row,
        int
    > {
    public:
        const_iterator() = default;
        const_iterator(const coordinates& v) : v_{v} {}

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

    using iterator = const_iterator;

    basic_result() = default;
    basic_result(handle_type res) : res_(std::move(res)) {}

    const_iterator begin() const noexcept { return {{std::addressof(*res_), 0, 0}}; }

    const_iterator end() const noexcept { return begin() + size(); }

    std::size_t size() const noexcept { return impl::ntuples(*res_);}

    bool empty() const noexcept { return size() == 0; }

    row operator[] (int i) const noexcept { return *(begin() + i); }

    row at(int i) const {
        if (i < 0 || static_cast<std::size_t>(i) >= size()) {
            throw std::out_of_range("ozo::result::at() index " + std::to_string(i) + " out of range");
        }
        return (*this)[i];
    }

    handle_type& handle() {
        return res_;
    }

    const handle_type& handle() const {
        return res_;
    }

private:
    handle_type res_;
};

using result = basic_result<native_result_handle>;
using shared_result = basic_result<shared_native_result_handle>;

} // namespace ozo
