// Test file to check, whether -Wno-gnu-string-literal-operator-template -Wno-gnu-zero-variadic-macro-arguments are properly passed to external projects

// -Wno-gnu-string-literal-operator-template
struct S { constexpr S() = default; };

template<typename C, C...>
auto operator ""_somesuffix() { return S{}; }

// -Wno-gnu-zero-variadic-macro-arguments
#define VARIADIC_MACRO(x, ...) "asdf"

char const* test_va_string()
{
    return VARIADIC_MACRO();
}
