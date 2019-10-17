#pragma once

#include <boost/hana/find.hpp>
#include <boost/hana/filter.hpp>
#include <boost/hana/fold.hpp>
#include <boost/hana/not_equal.hpp>
#include <boost/hana/optional.hpp>
#include <boost/hana/plus.hpp>
#include <boost/hana/string.hpp>
#include <boost/hana/tuple.hpp>
#include <ozo/core/none.h>
#include <ozo/core/options.h>
#include <ozo/query_builder.h>
#include <ozo/transaction_options.h>
#include <type_traits>

namespace ozo::detail {

namespace hana = boost::hana;

struct begin_statement_builder {
    using opt = transaction_options;

    template <typename Level>
    constexpr static auto to_string(decltype(opt::isolation_level), Level level) {
        [[maybe_unused]] const auto prefix = BOOST_HANA_STRING(" ISOLATION LEVEL ");
        const auto sql_levels = hana::make_map(
            hana::make_pair(isolation_level::serializable, BOOST_HANA_STRING("SERIALIZABLE")),
            hana::make_pair(isolation_level::repeatable_read, BOOST_HANA_STRING("REPEATABLE READ")),
            hana::make_pair(isolation_level::read_committed, BOOST_HANA_STRING("READ COMMITTED")),
            hana::make_pair(isolation_level::read_uncommitted, BOOST_HANA_STRING("READ UNCOMMITTED"))
        );

        if constexpr (const auto sql = hana::find(sql_levels, level); sql != hana::nothing) {
            return prefix + *sql;
        } else {
            static_assert(std::is_void_v<Level>, "unexpected transaction level");
        }
    }

    template <typename Mode>
    constexpr static auto to_string(decltype(opt::mode), Mode mode) {
        const auto sql_modes = hana::make_map(
            hana::make_pair(transaction_mode::read_write, BOOST_HANA_STRING(" READ WRITE")),
            hana::make_pair(transaction_mode::read_only, BOOST_HANA_STRING(" READ ONLY"))
        );

        if constexpr (const auto sql = hana::find(sql_modes, mode); sql != hana::nothing) {
            return *sql;
        } else {
            static_assert(std::is_void_v<Mode>, "unknown transaction mode");
        }
    }

    template <typename Deferrable>
    constexpr static auto to_string(decltype(opt::deferrability), Deferrable) {
        if constexpr (Deferrable::value) {
            return BOOST_HANA_STRING(" DEFERRABLE");
        } else {
            return BOOST_HANA_STRING(" NOT DEFERRABLE");
        }
    }

    template <typename Options>
    constexpr static auto build(Options const& options) {
        constexpr auto supported_options = hana::make_tuple(opt::isolation_level, opt::mode, opt::deferrability);
        constexpr auto query_prefix = BOOST_HANA_STRING("BEGIN");

        const auto real_options = hana::filter(hana::filter(supported_options, hana::partial(hana::contains, options)),
                                               ([&](const auto& v) { return std::negation<is_none<std::decay_t<decltype(options[v])>>>{}; }));
        const auto strings = hana::transform(real_options, [&](const auto& v) { return to_string(v, options[v]); });
        return make_query(hana::unpack(strings, [query_prefix](const auto& ...s) {
            return query_prefix + (s + ... + hana::string_c<>);
        }));
    }
};

} // ozo::detail
