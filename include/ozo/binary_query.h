#pragma once

#include <ozo/binary_serialization.h>
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

template <class T>
constexpr auto Nullable = is_nullable<std::decay_t<T>>::value;

template <class BufferAllocatorT, class OidMapImplT, class ... ParamsT>
class binary_query {
public:
    static constexpr std::size_t params_count = sizeof ... (ParamsT);

    using buffer_allocator_type = BufferAllocatorT;
    using oid_map_type = oid_map_t<OidMapImplT>;

    binary_query(std::string_view text)
        : impl(make_impl(buffer_allocator_type(), text, oid_map_type())) {}

    binary_query(std::string_view text, const oid_map_type& oid_map, const ParamsT& ... params)
        : impl(make_impl(buffer_allocator_type(), text, oid_map, params ...)) {}

    binary_query(const buffer_allocator_type& buffer_allocator, std::string_view text, const oid_map_type& oid_map, const ParamsT& ... params)
        : impl(make_impl(buffer_allocator, text, oid_map, params ...)) {}

    constexpr const char* text() const noexcept {
        return std::data(impl->text);
    }

    constexpr const ::Oid* types() const noexcept {
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
        std::string_view text;
        buffer_type buffer;
        std::array<::Oid, params_count> types;
        std::array<int, params_count> formats;
        std::array<int, params_count> lengths;
        std::array<const char*, params_count> values;

        impl_type(std::string_view text, const buffer_allocator_type& buffer_allocator)
            : text(text), buffer(buffer_allocator) {}

        impl_type(const impl_type&) = delete;
        impl_type(impl_type&&) = delete;
    };

    template <std::size_t field>
    class field_proxy {
    public:
        field_proxy(impl_type& result) : result(result) {}

        constexpr void set_type(::Oid value) noexcept {
            result.types[field] = value;
        }

        constexpr void set_format(int value) noexcept {
            result.formats[field] = value;
        }

        constexpr void set_length(int value) noexcept {
            result.lengths[field] = value;
        }

        constexpr void set_value(const char* value) noexcept {
            result.values[field] = value;
        }

        constexpr auto& buffer() noexcept {
            return result.buffer;
        }

    private:
        impl_type& result;
    };

    std::shared_ptr<impl_type> impl;

    static constexpr auto make_impl(const buffer_allocator_type& buffer_allocator, std::string_view text, const oid_map_type& oid_map, const ParamsT& ... params) {
        const auto tuple = hana::tuple<const ParamsT& ...>(params ...);
        auto result = std::make_shared<impl_type>(text, buffer_allocator);
        hana::for_each(
            hana::to<hana::tuple_tag>(hana::make_range(hana::size_c<0>, hana::size_c<params_count>)),
            [&] (auto field) {
                field_proxy<field> proxy(*result);
                write_meta(oid_map, tuple[field], proxy);
            }
        );
        std::size_t offset = 0;
        hana::for_each(
            hana::to<hana::tuple_tag>(hana::make_range(hana::size_c<0>, hana::size_c<params_count>)),
            [&] (auto field) {
                if (const auto size = result->lengths[field]) {
                    field_proxy<field>(*result).set_value(std::data(result->buffer) + offset);
                    offset += size;
                } else {
                    field_proxy<field>(*result).set_value(nullptr);
                }
            }
        );
        return result;
    }

    template <class T, std::size_t field>
    static constexpr Require<Nullable<T>> write_meta(const oid_map_type& oid_map, const T& value, field_proxy<field>& result) {
        if (is_null(value)) {
            write_null_meta(type_oid<std::decay_t<decltype(*value)>>(oid_map), result);
        } else {
            write_meta(oid_map, *value, result);
        }
    }

    template <class T, std::size_t field>
    static constexpr Require<!Nullable<T>> write_meta(const oid_map_type& oid_map, const T& value, field_proxy<field>& result) {
        using ozo::send;
        result.set_type(type_oid(oid_map, value));
        result.set_format(binary_format);
        const auto current_size = std::size(result.buffer());
        send(value, oid_map, std::back_inserter(result.buffer()));
        result.set_length(int(std::size(result.buffer()) - current_size));
    }

    template <class T, std::size_t field>
    static constexpr Require<!Nullable<T>> write_meta(const oid_map_type& oid_map, const std::reference_wrapper<T>& value, field_proxy<field>& result) {
        write_meta(oid_map, value.get(), result);
    }

    template <class T, std::size_t field>
    static constexpr void write_meta(const oid_map_type& oid_map, const std::weak_ptr<T>& value, field_proxy<field>& result) {
        write_meta(oid_map, value.lock(), result);
    }

    template <std::size_t field>
    static constexpr void write_meta(const oid_map_type&, std::nullptr_t, field_proxy<field>& result) noexcept {
        write_null_meta(null_oid_t::value, result);
    }

    template <std::size_t field>
    static constexpr void write_meta(const oid_map_type&, __OZO_NULLOPT_T, field_proxy<field>& result) noexcept {
        write_null_meta(null_oid_t::value, result);
    }

    template <std::size_t field>
    static constexpr void write_null_meta(::Oid oid, field_proxy<field>& result) noexcept {
        result.set_type(oid);
        result.set_format(binary_format);
        result.set_length(0);
    }
};

template <class ... ParamsT>
auto make_binary_query(std::string_view text, const ParamsT& ... params) {
    using binary_query_type = binary_query<std::allocator<char>, empty_oid_map::impl_type, const ParamsT& ...>;
    return binary_query_type(text, register_types<>(), params ...);
}

template <class OidMapImplT, class ... ParamsT>
auto make_binary_query(std::string_view text, const oid_map_t<OidMapImplT>& oid_map, const ParamsT& ... params) {
    using binary_query_type = binary_query<std::allocator<char>, OidMapImplT, const ParamsT& ...>;
    return binary_query_type(text, oid_map, params ...);
}

template <class BufferAllocatorT, class OidMapImplT, class ... ParamsT>
auto make_binary_query(std::string_view text, const oid_map_t<OidMapImplT>& oid_map, const ParamsT& ... params) {
    using binary_query_type = binary_query<BufferAllocatorT, OidMapImplT, const ParamsT& ...>;
    return binary_query_type(text, oid_map, params ...);
}

template <class BufferAllocatorT, class OidMapImplT, class ... ParamsT>
auto make_binary_query(const BufferAllocatorT& buffer_allocator, std::string_view text, const oid_map_t<OidMapImplT>& oid_map, const ParamsT& ... params) {
    using binary_query_type = binary_query<BufferAllocatorT, OidMapImplT, const ParamsT& ...>;
    return binary_query_type(buffer_allocator, text, oid_map, params ...);
}

template <class T, class = Require<Query<T>>>
auto make_binary_query(const T& query) {
    return hana::unpack(
        get_params(query),
        [&] (const auto& ... params) {
            return make_binary_query(get_text(query), empty_oid_map(), params ...);
        }
    );
}

template <class OidMapImplT, class T, class = Require<Query<T>>>
auto make_binary_query(const oid_map_t<OidMapImplT>& oid_map, const T& query) {
    return hana::unpack(
        get_params(query),
        [&] (const auto& ... params) {
            return make_binary_query(get_text(query), oid_map, params ...);
        }
    );
}

template <class BufferAllocatorT, class OidMapImplT, class T, class = Require<Query<T>>>
auto make_binary_query(const oid_map_t<OidMapImplT>& oid_map, const T& query) {
    return hana::unpack(
        get_params(query),
        [&] (const auto& ... params) {
            return make_binary_query<BufferAllocatorT>(get_text(query), oid_map, params ...);
        }
    );
}

template <class BufferAllocatorT, class OidMapImplT, class T, class = Require<Query<T>>>
auto make_binary_query(const BufferAllocatorT& buffer_allocator, const oid_map_t<OidMapImplT>& oid_map, const T& query) {
    return hana::unpack(
        get_params(query),
        [&] (const auto& ... params) {
            return make_binary_query(buffer_allocator, get_text(query), oid_map, params ...);
        }
    );
}

template <typename T, typename ...Ts>
inline decltype(auto) make_binary_query(const oid_map_t<T>&, binary_query<Ts...> query) {
    return std::move(query);
}

} // namespace ozo
