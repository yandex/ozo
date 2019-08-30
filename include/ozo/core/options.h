#pragma once

#include <boost/hana/is_empty.hpp>
#include <boost/hana/tuple.hpp>
#include <boost/hana/fold.hpp>
#include <boost/hana/concat.hpp>

namespace ozo {

/**
 * @brief Option class
 *
 * Option is a class like `boost::hana::type` but with syntactic sugar of
 * `operator = ` to produce `boost::hana::pair` with natural equation as
 * it shown in the example.
 *
 * ### Example
 *
 * @code
constexpr ozo::option<class foo_tag> foo;

static_assert(foo=0L == boost::hana::make_pair(foo, 0L));
 *
 * @endcode
 *
 * @tparam Key --- type to tag option
 * @ingroup group-core-types
 */
template <typename Key>
struct option : hana::type<Key> {
    template <typename T>
    constexpr auto operator = (T&& v) const {
        return hana::make_pair(*this, std::forward<T>(v));
    }
};

namespace hana = boost::hana;

/**
 * @brief Get the option object from Hana.Map
 *
 * This function is similar to `map[op]` call, but it makes a static assert that map should
 * contain an option. This produces human readable compile error in case of no option found.
 *
 * @param map --- map to get option from
 * @param op --- the option
 * @return option value reference
 * @ingroup group-core-functions
 */
template <typename Map, typename Key>
constexpr decltype(auto) get_option(Map&& map, ozo::option<Key> op) {
    static_assert(decltype(hana::is_a<hana::map_tag>(map))::value, "Map should be boost::hana::map");
    static_assert(decltype(hana::contains(map, op))::value, "option has not been found");
    return map[op];
}

/**
 * @brief Get the option object from Hana.Map
 *
 * This function is similar to `map[op]` call, but returns default value if no such option has been found.
 *
 * @param map --- map to get option from
 * @param op --- the option
 * @param default_ --- the default value for the option which is not found
 * @return option value reference or forwarded default value
 * @ingroup group-core-functions
 */
template <typename Map, typename Key, typename T>
constexpr decltype(auto) get_option(Map&& map, ozo::option<Key> op, T&& default_) {
    static_assert(decltype(hana::is_a<hana::map_tag>(map))::value, "Map should be boost::hana::map");
    if constexpr (decltype(hana::contains(map, op))::value) {
        return map[op];
    } else {
        return std::forward<T>(default_);
    }
}

/**
 * @brief Constructor for options map
 *
 * This function is a semantic wrapper for `boost::hana::make_map`.
 *
 * ### Example
 *
 * @code
namespace demo {

constexpr ozo::option<class foo_tag> foo;
constexpr ozo::option<class buzz_tag> buzz;

}

using namespace std::literals;

auto options = ozo::make_options(demo::foo="Foo"s, demo::buzz=777);

 * @endcode
 *
 * @param opts --- pairs of options and its' values
 * @return --- constructed `boost::hana::map`
 * @ingroup group-core-functions
 */
template <typename ...Options>
constexpr auto make_options(Options&& ...opts) {
    return hana::make_map(std::forward<Options>(opts)...);
}

/**
 * @brief Base class for options factories
 *
 * This class is CRTP base for classes are representing different
 * kinds of option factories. See `ozo::options_factory`,
 * `ozo::failover::retry_strategy` as examples of usage.
 *
 * @tparam RealType --- the real type child type of a factory
 * @tparam Options --- initial options set
 * @ingroup group-core-types
 */
template <typename RealType, typename Options>
class options_factory_base {
    Options v_;

    template <typename OtherOptions>
    constexpr static auto rebind(OtherOptions&& v) {
        return RealType::rebind_options(std::forward<OtherOptions>(v));
    }

public:
    /**
     * Construct a new options factory base object.
     *
     * @param v --- default options
     */
    constexpr options_factory_base(Options v) : v_(std::move(v)) {
        static_assert(decltype(hana::is_a<hana::map_tag>(v))::value, "Options should be boost::hana::map");
    }

    /**
     * Set number of options with values.
     *
     * This method updates or inserts option with a value specified.
     * If option exists - it will be updated, in this case value should be at least implicitly
     * convertible to the existant option value type. If option does not exist - it will be added.
     *
     * @param options --- pairs of option and its value
     * @return options_factory_base with updated or inserted options
     *
     *
     * ### Example
     *
     * @code
     * namespace demo {
     *
     * constexpr ozo::option<class foo_tag> foo;
     * constexpr ozo::option<class buzz_tag> buzz;
     *
     * }
     *
     * using namespace std::literals;
     *
     * ozo::options_factory options{};
     *
     * auto updated_options = options.set(demo::foo="Foo"s, demo::buzz=777);
     * @endcode
     */
#ifdef OZO_DOCUMENTATION
    template <typename Options>
    constexpr decltype(auto) set(Options&& ...options) &&;
#endif
    template <typename Key, typename T>
    constexpr decltype(auto) set(hana::pair<ozo::option<Key>, T> v) && {
        if constexpr (decltype(has(hana::first(v)))::value) {
            v_[hana::first(v)] = std::move(hana::second(std::move(v)));
            return static_cast<RealType&&>(*this);
        } else {
            return rebind(hana::insert(std::move(v_), std::move(v)));
        }
    }

    template <typename Key1, typename T1, typename Key2, typename T2, typename ...Ts>
    constexpr decltype(auto) set(hana::pair<ozo::option<Key1>, T1> v1, hana::pair<ozo::option<Key2>, T2> v2, Ts ...vs) && {
        return hana::fold(
            hana::make_tuple(std::move(v1), std::move(v2), std::move(vs)...),
            std::move(*this),
            [](auto self, auto v) { return std::move(self).set(v);}
        );
    }

    template <typename Key, typename T, typename ...Ts>
    constexpr decltype(auto) set(hana::pair<ozo::option<Key>, T> v, Ts ...vs) const & {
        auto copy = *this;
        return std::move(copy).set(std::move(v), std::move(vs)...);
    }

    template <typename Key, typename T>
    constexpr decltype(auto) set(ozo::option<Key> op, T&& value) && {
        return std::move(*this).set(hana::make_pair(op, std::forward<T>(value)));
    }

    template <typename Key, typename T>
    constexpr decltype(auto) set(ozo::option<Key> op, T&& value) const & {
        return set(hana::make_pair(op, std::forward<T>(value)));
    }

    /**
     * Indicates if option exists.
     *
     * @param op --- option to check
     * @return `boost::hana::true_` or `boost::hana::false_`
     *
     * ### Example
     *
     * @code
     * namespace demo {
     *
     * constexpr ozo::option<class foo_tag> foo;
     * constexpr ozo::option<class buzz_tag> buzz;
     *
     * }
     *
     * using namespace std::literals;
     *
     * ozo::options_factory options{demo::foo="Foo"s};
     *
     * static_assert(decltype(options.has(demo::foo))::value, "");
     * static_assert(!decltype(options.has(demo::buzz))::value, "");
     *
     * @endcode
     */
    template <typename Key>
    constexpr auto has(ozo::option<Key> op) const {
        return hana::contains(options(), op);
    }

    /**
     * Returns current option value. If no option found compile error will occur.
     *
     * @param op --- option to get value of
     * @return const reference on the value
     */
    template <typename Key>
    constexpr decltype(auto) get(ozo::option<Key> op) const {
        return get_option(options(), op);
    }

    /**
     * Constructed options accessor
     */
    constexpr const Options& options() const & { return v_;}
    /**
     * Constructed options accessor
     */
    constexpr Options& options() & { return v_;}
    /**
     * Constructed options accessor
     */
    constexpr Options&& options() && { return std::move(v_);}
};

/**
 * @brief Generic options factory
 *
 * Compile-time factory of options map.
 *
 * @tparam `Options` --- initial options set, default is empty `boost::hana::map`.
 * @ingroup group-core-types
 */
template <typename Options = decltype(hana::make_map())>
class options_factory : public ozo::options_factory_base<options_factory<Options>, Options> {
    using base = ozo::options_factory_base<options_factory<Options>, Options>;
    friend class ozo::options_factory_base<options_factory<Options>, Options>;

    template <typename OtherOptions>
    constexpr static auto rebind_options(OtherOptions&& options) {
        return options_factory<std::decay_t<OtherOptions>>(std::forward<OtherOptions>(options));
    }

public:
    /**
     * Construct a new options factory object
     *
     * @param options --- `boost::hana::map` with default options, default value is empty map.
     */
    constexpr options_factory(Options options = Options{}) : base(std::move(options)) {
        options_factory(decltype(hana::is_a<hana::map_tag>(options))::value, "Options should be boost::hana::map");
    }
};

} // namespace ozo
