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
#include <boost/hana/fold.hpp>
#include <boost/hana/tuple.hpp>
#include <boost/hana/plus.hpp>
#include <boost/hana/ext/std/array.hpp>

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
    : impl{std::allocate_shared<impl_type>(allocator, std::move(text), params, oid_map, allocator)} {}

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

        impl_type(text_type text, const params_type& params,
            const oid_map_type& oid_map, const allocator_type& allocator)
        : text(std::move(text)), buffer(allocator) {
            formats.fill(binary_format);

            const auto range = hana::to_tuple(hana::make_range(hana::size_c<0>, hana::size_c<params_count>));

            hana::for_each(range, [&] (auto i) {
                lengths[i] = std::max(0, size_of(params[i]));
                types[i] = type_oid(oid_map, params[i]);
            });

            buffer.reserve(hana::unpack(lengths, [](auto ...x) {return (x + ... + 0);}));

            ozo::detail::ostreambuf osbuf(buffer);
            ozo::ostream os(&osbuf);

            hana::for_each(params, [&] (auto& param) { send(os, oid_map, param);});

            std::size_t offset = 0;
            hana::for_each(range, [&] (auto i) {
                values[i] = lengths[i] ? std::data(buffer) + offset : nullptr;
                offset += lengths[i];
            });
        }

        impl_type(const impl_type&) = delete;
        impl_type(impl_type&&) = delete;
    };

    std::shared_ptr<const impl_type> impl;
};

template <class Query, class OidMap, class Allocator = std::allocator<char>>
binary_query(const Query& q, const OidMap&, const Allocator& = Allocator{}, Require<ozo::Query<Query>>* = nullptr) ->
    binary_query<decltype(get_query_text(q)), decltype(get_query_params(q)), OidMap, Allocator>;

} // namespace ozo
