#pragma once

#include <ozo/ext/std.h>
#include <ozo/ext/boost.h>
#include <ozo/io/send.h>
#include <ozo/io/array.h>
#include <ozo/io/composite.h>
#include <ozo/core/concept.h>
#include <ozo/query.h>
#include <ozo/type_traits.h>
#include <ozo/optional.h>
#include <ozo/pg/types.h>

#include <boost/hana/for_each.hpp>
#include <boost/hana/tuple.hpp>
#include <boost/hana/ext/std/array.hpp>

#include <libpq-fe.h>

#include <array>
#include <iterator>
#include <memory>
#include <string_view>
#include <vector>

namespace ozo {

/**
 * @brief Binary protocol query representation.
 *
 * The `binary_query` being used for query sending to a database.
 *
 * @models{BinaryQueryConvertible}
 *
 * @ingroup group-query-types
 */
class binary_query {
public:
    static constexpr auto binary_format = 1;

    struct implementation {
        /**
          * Get raw query text buffer.
          *
          * @return `const char*` --- pointer to the buffer.
          */
        virtual const char* text() const noexcept = 0;

        /**
          * Get query parameter types array.
          *
          * @return `const oid_t*` --- the query parameter types array.
          */
        virtual const oid_t* types() const noexcept = 0;

        /**
          * Get query parameter formats array. For the `binary_array` all the formats are `binary_format`.
          *
          * @return `const int*` --- the query parameter formats array.
          */
        virtual const int* formats() const noexcept = 0;

        /**
          * Get query parameter lengths array. Each element represents length of the respective parameter
          * binary representation from `values()`.
          *
          * @return `const int*` --- the query parameter lengths array.
          */
        virtual const int* lengths() const noexcept = 0;

        /**
          * Get query parameter binary representations array.
          *
          * @return `const char* const*` --- an array with pointers to the binary representations.
          */
        virtual const char* const* values() const noexcept = 0;

        /**
          * Get query parameters count.
          *
          * @return `std::ptrdiff_t` --- the query parameters count.
          */
        virtual std::ptrdiff_t params_count() const noexcept = 0;

        virtual ~implementation() = default;
    };

    /**
     * Construct a new binary query object with known parameters at compile time
     *
     * @param text      --- query text object, should model `QueryText` concept.
     * @param params    --- query parameters object, should model `HanaSequence` concept.
     * @param oid_map   --- `OidMap` which is used within connection.
     * @param allocator --- allocator object which should be used to allocate internal data,
     *                      default is `std::allocator<char>`.
     */
    template <class Text, class Params, class OidMap, class Allocator = std::allocator<char>>
    binary_query(Text text, const Params& params, const OidMap& oid_map, const Allocator& allocator = Allocator{})
    : impl{std::allocate_shared<fixed_params<Text, Params, OidMap, Allocator>>(
        allocator, std::move(text), params, oid_map, allocator
    )} {}

    /**
     * Construct a new binary query object with a user defined implementation
     *
     * @param impl --- binary query implementation
     */
    explicit binary_query(std::shared_ptr<const implementation> impl)
    : impl(std::move(impl)) {}

    /**
     * Get raw query text buffer.
     *
     * @return `const char*` --- pointer to the buffer.
     */
    const char* text() const noexcept {
        return impl->text();
    }

    /**
     * Get query parameter types array.
     *
     * @return `const oid_t*` --- the query parameter types array.
     */
    const oid_t* types() const noexcept {
        return impl->types();
    }

    /**
     * Get query parameter formats array. For the `binary_array` all the formats are `binary_format`.
     *
     * @return `const int*` --- the query parameter formats array.
     */
    const int* formats() const noexcept {
        return impl->formats();
    }

    /**
     * Get query parameter lengths array. Each element represents length of the respective parameter
     * binary representation from `values()`.
     *
     * @return `const int*` --- the query parameter lengths array.
     */
    const int* lengths() const noexcept {
        return impl->lengths();
    }

    /**
     * Get query parameter binary representations array.
     *
     * @return `const char* const*` --- an array with pointers to the binary representations.
     */
    const char* const* values() const noexcept {
        return impl->values();
    }

    /**
     * Get query parameters count.
     *
     * @return `std::ptrdiff_t` --- the query parameters count.
     */
    std::ptrdiff_t params_count() const noexcept {
        return impl->params_count();
    }

private:
    template <class Text, class Params, class OidMap, class Allocator = std::allocator<char>>
    struct fixed_params final : implementation {
        static_assert(ozo::HanaSequence<Params>, "Params should be Hana.Sequence");
        static_assert(ozo::OidMap<OidMap>, "OidMap should model ozo::OidMap");
        static_assert(ozo::QueryText<Text>, "Text should model ozo::QueryText concept");

        using allocator_type = std::conditional_t<
                                std::is_same_v<typename Allocator::value_type, char>,
                                    Allocator,
                                    typename Allocator::template rebind<char>::other>;
        using buffer_type = std::vector<char, allocator_type>;
        using oid_map_type = OidMap;
        using text_type = std::decay_t<Text>;
        using params_type = Params;

        static constexpr auto params_count_ = decltype(hana::length(std::declval<params_type>()))::value;

        text_type text_;
        buffer_type buffer_;
        std::array<oid_t, params_count_> types_;
        std::array<int, params_count_> formats_;
        std::array<int, params_count_> lengths_;
        std::array<const char*, params_count_> values_;

        fixed_params(Text text, const Params& params,
            const OidMap& oid_map, const Allocator& allocator)
        : text_(std::move(text)), buffer_(allocator) {
            formats_.fill(binary_format);

            const auto range = hana::to_tuple(hana::make_range(hana::size_c<0>, hana::size_c<params_count_>));

            hana::for_each(range, [&] (auto i) {
                lengths_[i] = std::max(0, size_of(params[i]));
                types_[i] = type_oid(oid_map, params[i]);
            });

            buffer_.reserve(hana::unpack(lengths_, [](auto ...x) {return (x + ... + 0);}));

            ozo::ostream os(buffer_);

            hana::for_each(params, [&] (auto& param) { send(os, oid_map, param);});

            std::size_t offset = 0;
            hana::for_each(range, [&] (auto i) {
                values_[i] = lengths_[i] ? std::data(buffer_) + offset : nullptr;
                offset += lengths_[i];
            });
        }

        fixed_params(const fixed_params&) = delete;
        fixed_params(fixed_params&&) = delete;

        const char* text() const noexcept override {
            return to_const_char(text_);
        }

        const oid_t* types() const noexcept override {
            return std::data(types_);
        }

        const int* formats() const noexcept override {
            return std::data(formats_);
        }

        const int* lengths() const noexcept override {
            return std::data(lengths_);
        }

        const char* const* values() const noexcept override {
            return std::data(values_);
        }

        std::ptrdiff_t params_count() const noexcept override {
            return params_count_;
        }
    };

    std::shared_ptr<const implementation> impl;
};

namespace detail {
struct no_binary_query_conversion {};
} // namespace detail

template <typename T, typename = hana::when<true>>
struct to_binary_query_impl : detail::no_binary_query_conversion {
    template <typename OidMap, typename Alloc>
    static auto apply(const T&, const OidMap&, const Alloc&) {
        static_assert(std::is_void_v<to_binary_query_impl>, "no conversion to the binary_query is defined");
    }
};

template <typename T>
using is_binary_query_convertible = typename std::negation<typename std::is_base_of<detail::no_binary_query_conversion, T>::type>::type;

template <typename T>
inline constexpr auto is_binary_query_convertible_v = is_binary_query_convertible<T>::value;

/**
 * @brief Concept of a type that is convertible to `ozo::binary_query`
 *
 * To be convertible to the `ozo::binary_query` type should model `Query`
 * concept or should have a valid `ozo::to_binary_query_impl<T>` overload.
 *
 * ### Customization
 *
 * A particular query object type may become `BinaryQueryConvertible` via the
 * specialization of `ozo::to_binary_query_impl<T>::apply()` template function.
 * By default, it handles any `Query` model object or `binary_query` object.
 *
 * @code
template <typename T>
struct to_binary_query_impl<T> {
    template <typename OidMap, typename Allocator>
    static binary_query apply(const T&, const OidMap&, const Allocator&);
};
 * @endcode
 *
 * ### Example
 *
 * E.g., a custom library contains type-erasure interface for its queries.
 *
 * @code
namespace demo {

constexpr auto oidMap = ozo::register_types<...>();

using OidMap = decltype(oidMap);

struct Query {
    virtual ozo::binary_query toBinaryQuery(const OidMap&, const std::allocator<char>&) const = 0;
    virtual ~Query() = 0;
};

} // namespace demo
 * @endcode
 * Then the adaptation should look like this:
 * @code
namespace ozo {
template <>
struct to_binary_query_impl<demo::Query> {
    template <typename OidMap, typename Alloc>
    static binary_query apply(const demo::Query& query, const demo::OidMap& oid_map, const std::allocator<char>& alloc) {
        return query.toBinaryQuery(oid_map, alloc);
    }
};
} // namespace ozo
 * @endcode
 *
 *
 * @ingroup group-query-concepts
 * @concept{ozo::BinaryQueryConvertible}
 */
//! @cond
template <typename T>
inline constexpr auto BinaryQueryConvertible = is_binary_query_convertible_v<std::decay_t<T>>;
//! @endcond

template <typename T>
struct to_binary_query_impl<T, hana::when<Query<T>>> {
    template <typename OidMap, typename Alloc>
    static binary_query apply(const T& query, const OidMap& oid_map, const Alloc& allocator) {
        return binary_query(get_query_text(query), get_query_params(query), oid_map, allocator);
    }
};

template <>
struct to_binary_query_impl<binary_query> {
    template <typename OidMap, typename Alloc>
    static binary_query apply(const binary_query& query, const OidMap&, const Alloc&) {
        return query;
    }
};

/**
 * @brief Convert a query object to the binary representation.
 *
 * This function provides an ability to convert a query object to the protocol compatible
 * binary representation to send it to the PostgreSQL database. A user may call this
 * function to reuse the `binary_query` and eliminate unnecessarily conversion of the
 * query object to its binary representation each operation. E.g., this may be useful
 * with the `failover` micro-framework.
 *
 * @param query     --- a query object to convert to the binary representation.
 * @param oid_map   --- `OidMap` to type OIDs for the binary representation.
 * @param allocator --- allocator to use for the data of `ozo::binary_query`.
 *
 * @return `ozo::binary_query` --- the binary representation.
 *
 * @ingroup group-query-functions
 */
template <typename BinaryQueryConvertible, typename OidMap, typename Allocator = std::allocator<char>>
inline binary_query to_binary_query(const BinaryQueryConvertible& query,
        const OidMap& oid_map, const Allocator& allocator = Allocator{}) {
    return to_binary_query_impl<BinaryQueryConvertible>::apply(query, oid_map, allocator);
}

} // namespace ozo
