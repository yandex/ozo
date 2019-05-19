#pragma once

#include <utility>

namespace ozo::detail::functional {

struct forward {
    template <typename T>
    constexpr static decltype(auto) apply(T&& v) noexcept {
        return std::forward<T>(v);
    }
};

struct dereference {
    template <typename T>
    constexpr static decltype(auto) apply(T&& v) noexcept(noexcept(*v)) {
        return *v;
    }
};

struct operator_not {
    template <typename T>
    constexpr static decltype(auto) apply(T&& v) noexcept(noexcept(!v)) {
        return !v;
    }
};

struct always_true {
    template <typename T>
    constexpr static std::true_type apply(T&&) noexcept { return {};}
};

struct always_false {
    template <typename T>
    constexpr static std::false_type apply(T&&) noexcept { return {};}
};

} // namespace ozo::detail::functional

namespace ozo::detail {

template <template<typename...> typename Functional, typename T, typename ...Ts>
constexpr Functional<std::decay_t<T>> make_functional(T&&, Ts&&...) { return {};}

template <template<typename...> typename Functional, typename ...Ts>
using functional_type = decltype(make_functional<Functional>(std::declval<Ts>()...));

struct is_applicable_impl {
    template <typename ...Ts>
    using true_type = std::is_void<std::void_t<Ts...>>;

    template <typename Functional, typename ...Ts>
    static constexpr auto get(Functional, Ts&&... args) ->
            true_type<decltype(Functional::apply(std::forward<Ts>(args)...))>;

    static constexpr std::false_type get(...);
};

template <template<typename...> typename Functional, typename ...Ts>
constexpr auto is_applicable(Ts&&... args) {
    return decltype(
        is_applicable_impl::get(
            make_functional<Functional>(std::forward<Ts>(args)...),
            std::forward<Ts>(args)...)
     ){};
}

template <template<typename...> typename Functional, typename ...Ts>
constexpr auto IsApplicable = decltype(is_applicable<Functional>(std::declval<Ts>()...)){};

template <template<typename...> typename Functional, typename ...Ts>
using result_of = decltype(functional_type<Functional, Ts...>::apply(std::declval<Ts>()...));

template <template<typename...> typename Functional, typename ...Ts>
constexpr bool is_noexcept = noexcept(functional_type<Functional, Ts...>::apply(std::declval<Ts>()...));

template <template<typename...> typename Functional, typename ...Ts>
constexpr result_of<Functional, Ts...> apply(Ts&&... args) noexcept(is_noexcept<Functional, Ts...>) {
    return functional_type<Functional, Ts...>::apply(std::forward<Ts>(args)...);
}

} // namespace ozo::detail

