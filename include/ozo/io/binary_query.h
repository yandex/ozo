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

class binary_query {
public:
    template <class Text, class Params, class OidMap, class Allocator = std::allocator<char>>
    binary_query(Text text, const Params& params, const OidMap& oid_map, const Allocator& allocator = Allocator{},
            Require<ozo::QueryText<Text> && ozo::HanaSequence<Params>>* = nullptr)
    : impl{std::allocate_shared<impl_type<Text, Params, OidMap, Allocator>>(allocator, std::move(text), params, oid_map, allocator)} {}

    template <typename Query, class OidMap, class Allocator = std::allocator<char>>
    binary_query(const Query& query, const OidMap& oid_map, const Allocator& allocator = Allocator{},
            Require<ozo::Query<Query>>* = nullptr)
    : binary_query(get_query_text(query), get_query_params(query), oid_map, allocator) {}

    const char* text() const noexcept {
        return impl->text();
    }

    const oid_t* types() const noexcept {
        return impl->types();
    }

    const int* formats() const noexcept {
        return impl->formats();
    }

    const int* lengths() const noexcept {
        return impl->lengths();
    }

    const char* const* values() const noexcept {
        return impl->values();
    }

    std::ptrdiff_t params_count() const noexcept {
        return impl->params_count();
    }

private:
    static constexpr auto binary_format = 1;

    struct interface {
        virtual const char* text() const noexcept = 0;
        virtual const oid_t* types() const noexcept = 0;
        virtual const int* formats() const noexcept = 0;
        virtual const int* lengths() const noexcept = 0;
        virtual const char* const* values() const noexcept = 0;
        virtual std::ptrdiff_t params_count() const noexcept = 0;
        virtual ~interface() = default;
    };

    template <class Text, class Params, class OidMap, class Allocator = std::allocator<char>>
    struct impl_type final : interface {
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

        impl_type(Text text, const Params& params,
            const OidMap& oid_map, const Allocator& allocator)
        : text_(std::move(text)), buffer_(allocator) {
            formats_.fill(binary_format);

            const auto range = hana::to_tuple(hana::make_range(hana::size_c<0>, hana::size_c<params_count_>));

            hana::for_each(range, [&] (auto i) {
                lengths_[i] = std::max(0, size_of(params[i]));
                types_[i] = type_oid(oid_map, params[i]);
            });

            buffer_.reserve(hana::unpack(lengths_, [](auto ...x) {return (x + ... + 0);}));

            ozo::detail::ostreambuf osbuf(buffer_);
            ozo::ostream os(&osbuf);

            hana::for_each(params, [&] (auto& param) { send(os, oid_map, param);});

            std::size_t offset = 0;
            hana::for_each(range, [&] (auto i) {
                values_[i] = lengths_[i] ? std::data(buffer_) + offset : nullptr;
                offset += lengths_[i];
            });
        }

        impl_type(const impl_type&) = delete;
        impl_type(impl_type&&) = delete;

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

    std::shared_ptr<const interface> impl;
};

} // namespace ozo
