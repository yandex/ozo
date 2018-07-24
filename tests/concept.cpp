#include <ozo/concept.h>

#include <list>
#include <vector>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace {

TEST(ForwardIterator, should_return_true_for_iterator_type) {
    EXPECT_TRUE(ozo::ForwardIterator<std::list<int>::iterator>);
}

TEST(ForwardIterator, should_return_false_for_not_iterator_type) {
    EXPECT_FALSE(ozo::ForwardIterator<int>);
}

TEST(Iterable, should_return_true_for_iterable_type) {
    EXPECT_TRUE(ozo::Iterable<std::vector<int>>);
}

TEST(Iterable, should_return_false_for_not_iterable_type) {
    EXPECT_FALSE(ozo::Iterable<int>);
}

} // namespace
