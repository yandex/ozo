#include <ozo/detail/functional.h>

#include <gtest/gtest.h>

namespace {

template <typename T>
struct test_functional {
    static constexpr int apply(const T&, int v) { return v;}
};

template <typename T>
struct test_dispatch {};

template <>
struct test_dispatch<std::string> {
    static constexpr int apply(const std::string&, int) { return 777;}
};

TEST(IsApplicable, should_return_true_for_applicable_functional_arguments) {
    const bool res = ozo::detail::IsApplicable<test_functional, std::string, int>;
    EXPECT_TRUE(res);
}

TEST(IsApplicable, should_return_false_for_non_applicable_functional_arguments) {
    const bool res = ozo::detail::IsApplicable<test_functional, std::string, std::string>;
    EXPECT_FALSE(res);
}

TEST(result_of, should_return_type_of_functional_result) {
    using type = ozo::detail::result_of<test_functional, std::string, int>;
    const bool res = std::is_same_v<type, int>;
    EXPECT_TRUE(res);
}

TEST(apply, should_invoke_functional_and_return_result) {
    const auto res = ozo::detail::apply<test_functional>(std::string{}, 42);
    EXPECT_EQ(res, 42);
}

TEST(apply, should_dispatch_functional_by_first_argument) {
    const auto res = ozo::detail::apply<test_dispatch>(std::string{}, 42);
    EXPECT_EQ(res, 777);
}

} // namespace
