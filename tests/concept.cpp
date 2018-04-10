#include <ozo/concept.h>

#include <GUnit/GTest.h>

GTEST("ozo::ForwardIterator<T>", "[returns true for a std iterator]") {
    EXPECT_TRUE(ozo::ForwardIterator<std::vector<int>::iterator>);
}

namespace iterability_test {

struct foo {
    class iterator
        : public std::iterator<std::forward_iterator_tag, foo, long, const foo*, foo&>{
    };

    foo::iterator begin() { return foo::iterator(); }
    foo::iterator end() { return foo::iterator(); }
};

struct bar {};

}

GTEST("ozo::ForwardIterator<T>", "[returns true for a suitable custom iterator]") {
    EXPECT_TRUE(ozo::ForwardIterator<iterability_test::foo::iterator>);
}

GTEST("ozo::ForwardIterator<T>", "[returns false for an unsuitable type]") {
    EXPECT_FALSE(ozo::ForwardIterator<iterability_test::bar>);
}

GTEST("ozo::Iterable<T>", "[returns true for a std container]") {
    EXPECT_TRUE(ozo::Iterable<std::vector<int>>);
}

GTEST("ozo::Iterable<T>", "[returns true for suitable custom types]") {
    EXPECT_TRUE(ozo::Iterable<iterability_test::foo>);
}

GTEST("ozo::Iterable<T>", "[returns false for unsuitable custom types]") {
    EXPECT_FALSE(ozo::Iterable<iterability_test::bar>);
}