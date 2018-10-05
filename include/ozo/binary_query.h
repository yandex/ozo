#pragma once

#include <ozo/send.h>
#include <ozo/detail/array.h>
#include <ozo/concept.h>
#include <ozo/query.h>
#include <ozo/type_traits.h>
#include <ozo/optional.h>

#include <boost/hana/for_each.hpp>
#include <boost/hana/tuple.hpp>

#include <libpq-fe.h>

#include <array>
#include <iterator>
#include <memory>
#include <string_view>
#include <vector>

namespace ozo {

template <class TextT, class ParamsT, class OidMapT, class BufferAllocatorT>
class binary_query {
public:
    static_assert(HanaTuple<ParamsT>, "Params must be hana::tuple");
    static_assert(OidMap<OidMapT>, "OidMapT must be ozo::oid_map_t");
    static_assert(QueryText<TextT>, "Text must be QueryText concept");

    using buffer_allocator_type = BufferAllocatorT;
    using oid_map_type = OidMapT;
    using text_type = TextT;
    using params_type = ParamsT;

    static constexpr auto params_count = decltype(hana::length(std::declval<params_type>()))::value;

    binary_query(text_type text, const params_type& params, const buffer_allocator_type& buffer_allocator, const oid_map_type& oid_map)
        : impl(make_impl(std::move(text), params, oid_map, buffer_allocator)) {}

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

    using buffer_type = std::vector<char, buffer_allocator_type>;

    struct impl_type {
        text_type text;
        buffer_type buffer;
        std::array<oid_t, params_count> types;
        std::array<int, params_count> formats;
        std::array<int, params_count> lengths;
        std::array<const char*, params_count> values;

        impl_type(text_type text, const buffer_allocator_type& buffer_allocator)
            : text(std::move(text)), buffer(buffer_allocator) {}

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

        constexpr int stream_pos() noexcept {
            return int(std::size(result.buffer));
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
            const oid_map_type& oid_map, const buffer_allocator_type& buffer_allocator) {

        auto result = std::make_shared<impl_type>(std::move(text), buffer_allocator);

        ozo::detail::ostreambuf osbuf(result->buffer);
        ozo::ostream os(&osbuf);

        const auto range = hana::to<hana::tuple_tag>(
            hana::make_range(hana::size_c<0>, hana::size_c<params_count>));

        hana::for_each(range, [&] (auto field) {
            field_proxy<field> proxy(*result, os);
            write_meta(oid_map, params[field], proxy);
        });

        std::size_t offset = 0;
        hana::for_each(range, [&] (auto field) {
                const auto size = result->lengths[field];
                result->values[field] = size ? std::data(result->buffer) + offset : nullptr;
                offset += size;
            }
        );
        return result;
    }

    template <class T, std::size_t field>
    static void write_meta(const oid_map_type& oid_map, const T& value, field_proxy<field>& result) {
        using ozo::send;
        using ozo::size_of;
        result.set_type(type_oid(oid_map, value));
        result.set_format(binary_format);
        send(result.stream(), oid_map, value);
        result.set_length(size_of(value));
    }
};

template <typename T>
struct is_binary_query : std::false_type {};

template <typename ... Ts>
struct is_binary_query<binary_query<Ts...>> : std::true_type {};

template <typename T>
constexpr auto BinaryQuery = is_binary_query<std::decay_t<T>>::value;

template <class Text, class Params, class M = empty_oid_map, class Alloc = std::allocator<char>,
        class = Require<QueryText<Text> && HanaTuple<Params>>
>
auto make_binary_query(Text&& text, const Params& params,
            const M& oid_map = M{}, const Alloc& buffer_allocator = Alloc{}) {
    using binary_query_type = binary_query<std::decay_t<Text>, Params, M, Alloc>;
    return binary_query_type(std::forward<Text>(text), params, buffer_allocator, oid_map);
}

template <class T, class M = empty_oid_map, class Alloc = std::allocator<char>, class = Require<Query<T>>>
auto make_binary_query(const T& query, const M& oid_map = M{}, const Alloc& buffer_allocator = Alloc{}) {
    return make_binary_query(get_text(query), get_params(query), oid_map, buffer_allocator);
}

template <class Q, class M, class A>
inline Require<BinaryQuery<Q>, Q> make_binary_query(Q query, M&&, A&&) {
    return std::move(query);
}

} // namespace ozo
