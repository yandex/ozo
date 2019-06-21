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

#include <libpq-fe.h>

#include <array>
#include <iterator>
#include <memory>
#include <string_view>
#include <vector>

namespace ozo {

template <class Text, class Params, class OidMap, class Allocator = std::allocator<char>>
class binary_query {
public:
    static_assert(ozo::HanaSequence<Params>, "Params should be Hana.Sequence");
    static_assert(ozo::OidMap<OidMap>, "OidMap should model ozo::OidMap");
    static_assert(ozo::QueryText<Text>, "Text should model ozo::QueryText concept");

    using allocator_type = std::conditional_t<
                                    std::is_same_v<typename Allocator::value_type, char>,
                                        Allocator,
                                        typename Allocator::template rebind<char>::other>;
    using oid_map_type = OidMap;
    using text_type = std::decay_t<Text>;
    using params_type = Params;

    static constexpr auto params_count = decltype(hana::length(std::declval<params_type>()))::value;

    binary_query(Text text, const Params& params, const OidMap& oid_map, const Allocator& allocator = Allocator{},
            Require<ozo::QueryText<Text> && ozo::HanaSequence<Params>>* = nullptr)
    : impl(make_impl(std::move(text), params, oid_map, allocator)) {}

    template <typename Query>
    binary_query(const Query& query, const OidMap& oid_map, const Allocator& allocator = Allocator{},
            Require<ozo::Query<Query>>* = nullptr)
    : binary_query(get_query_text(query), get_query_params(query), oid_map, allocator) {}

    constexpr const char* text() const noexcept {
        return to_const_char(impl->text);
    }

    constexpr const oid_t* types() const noexcept {
        return std::data(impl->types);
    }

    constexpr const int* formats() const noexcept {
        return std::data(impl->formats);
    }

    constexpr const int* lengths() const noexcept {
        return std::data(impl->lengths);
    }

    constexpr const char* const* values() const noexcept {
        return std::data(impl->values);
    }

private:
    static constexpr auto binary_format = 1;

    using buffer_type = std::vector<char, allocator_type>;

    struct impl_type {
        text_type text;
        buffer_type buffer;
        std::array<oid_t, params_count> types;
        std::array<int, params_count> formats;
        std::array<int, params_count> lengths;
        std::array<const char*, params_count> values;

        impl_type(text_type text, const allocator_type& allocator)
            : text(std::move(text)), buffer(allocator) {}

        impl_type(const impl_type&) = delete;
        impl_type(impl_type&&) = delete;
    };

    template <std::size_t field>
    class field_proxy {
    public:
        field_proxy(impl_type& result, ostream& os) : result(result), os(os) {}

        constexpr void set_type(::Oid value) noexcept {
            result.types[field] = value;
        }

        constexpr void set_format(int value) noexcept {
            result.formats[field] = value;
        }

        constexpr void set_length(int value) noexcept {
            result.lengths[field] = value;
        }

        constexpr auto& stream() noexcept {
            return os;
        }

    private:
        impl_type& result;
        ozo::ostream& os;
    };

    std::shared_ptr<impl_type> impl;

    static auto make_impl(text_type text, const params_type& params,
            const oid_map_type& oid_map, const allocator_type& allocator) {

        auto result = std::make_shared<impl_type>(std::move(text), allocator);

        ozo::detail::ostreambuf osbuf(result->buffer);
        ozo::ostream os(&osbuf);

        const auto range = hana::to<hana::tuple_tag>(
            hana::make_range(hana::size_c<0>, hana::size_c<params_count>));

        hana::for_each(range, [&] (auto field) {
            write_meta(oid_map, params[field], field_proxy<field>(*result, os));
        });

        std::size_t offset = 0;
        hana::for_each(range, [&] (auto field) {
                const auto size = result->lengths[field];
                if (size && size != null_state_size) {
                    result->values[field] = std::data(result->buffer) + offset;
                    offset += size;
                } else {
                    result->values[field] = nullptr;
                }
            }
        );
        return result;
    }

    template <class T, std::size_t field>
    static void write_meta(const oid_map_type& oid_map, const T& value, field_proxy<field> result) {
        using ozo::send;
        using ozo::size_of;
        result.set_type(type_oid(oid_map, value));
        result.set_format(binary_format);
        send(result.stream(), oid_map, value);
        result.set_length(size_of(value));
    }
};

template <class Query, class OidMap, class Allocator = std::allocator<char>>
binary_query(const Query& q, const OidMap&, const Allocator& = Allocator{}, Require<ozo::Query<Query>>* = nullptr) ->
    binary_query<decltype(get_query_text(q)), decltype(get_query_params(q)), OidMap, Allocator>;

} // namespace ozo
