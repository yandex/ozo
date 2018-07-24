#include <ozo/type_traits.h>

#include <boost/make_shared.hpp>
#include <boost/fusion/adapted/std_tuple.hpp>
#include <boost/fusion/adapted/struct/define_struct.hpp>
#include <boost/fusion/include/define_struct.hpp>

#include <boost/hana/adapt_adt.hpp>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace hana = boost::hana;
using namespace testing;

namespace {

TEST(is_null, should_return_true_for_non_initialized_optional) {
    EXPECT_TRUE(ozo::is_null(boost::optional<int>{}));
}

TEST(is_null, should_return_false_for_initialized_optional) {
    EXPECT_TRUE(!ozo::is_null(boost::optional<int>{0}));
}

TEST(is_null, should_return_false_for_valid_std_weak_ptr) {
    auto ptr = std::make_shared<int>();
    EXPECT_TRUE(!ozo::is_null(std::weak_ptr<int>{ptr}));
}

TEST(is_null, should_return_true_for_expired_std_weak_ptr) {
    std::weak_ptr<int> ptr;
    {
        ptr = std::make_shared<int>();
    }
    EXPECT_TRUE(ozo::is_null(ptr));
}

TEST(is_null, should_return_true_for_non_initialized_std_weak_ptr) {
    std::weak_ptr<int> ptr;
    EXPECT_TRUE(ozo::is_null(ptr));
}


TEST(is_null, should_return_false_for_valid_boost_weak_ptr)  {
    auto ptr = boost::make_shared<int>();
    EXPECT_TRUE(!ozo::is_null(boost::weak_ptr<int>(ptr)));
}

TEST(is_null, should_return_true_for_expired_boost_weak_ptr)  {
    boost::weak_ptr<int> ptr;
    {
        ptr = boost::make_shared<int>();
    }
    EXPECT_TRUE(ozo::is_null(ptr));
}

TEST(is_null, should_return_true_for_non_initialized_boost_weak_ptr)  {
    boost::weak_ptr<int> ptr;
    EXPECT_TRUE(ozo::is_null(ptr));
}

TEST(is_null, should_return_false_for_non_nullable_type) {
    EXPECT_TRUE(!ozo::is_null(int()));
}

TEST(unwrap_nullable, should_unwrap_nullable_type) {
    boost::optional<int> n(7);
    EXPECT_EQ(ozo::unwrap_nullable(n), 7);
}

TEST(unwrap_nullable, should_unwrap_notnullable_type) {
    int n(7);
    EXPECT_EQ(ozo::unwrap_nullable(n), 7);
}

}

namespace ozo {
namespace tests {

struct nullable_mock {
    MOCK_METHOD0(emplace, void());
    MOCK_CONST_METHOD0(negate, bool());
    MOCK_METHOD0(reset, void());
    bool operator!() const { return negate(); }
};

}// namespace tests

template <>
struct is_nullable<StrictMock<ozo::tests::nullable_mock>> : std::true_type {};

template <>
struct is_nullable<ozo::tests::nullable_mock> : std::true_type {};

} // namespace ozo

namespace {

using namespace ozo::tests;

TEST(init_nullable, should_initialize_uninitialized_nullable) {
    StrictMock<nullable_mock> mock{};
    EXPECT_CALL(mock, negate()).WillOnce(Return(true));
    EXPECT_CALL(mock, emplace()).WillOnce(Return());
    ozo::init_nullable(mock);
}

TEST(init_nullable, should_pass_initialized_nullable) {
    StrictMock<nullable_mock> mock{};
    EXPECT_CALL(mock, negate()).WillOnce(Return(false));
    ozo::init_nullable(mock);
}

TEST(init_nullable, should_allocate_std_unique_ptr) {
    std::unique_ptr<int> ptr{};
    ozo::init_nullable(ptr);
    EXPECT_TRUE(ptr);
}

TEST(init_nullable, should_allocate_std_shared_ptr) {
    std::shared_ptr<int> ptr{};
    ozo::init_nullable(ptr);
    EXPECT_TRUE(ptr);
}

TEST(init_nullable, should_allocate_boost_scoped_ptr) {
    boost::scoped_ptr<int> ptr{};
    ozo::init_nullable(ptr);
    EXPECT_TRUE(ptr);
}

TEST(init_nullable, should_allocate_boost_shared_ptr) {
    boost::shared_ptr<int> ptr{};
    ozo::init_nullable(ptr);
    EXPECT_TRUE(ptr);
}

TEST(reset_nullable, should_reset_nullable) {
    StrictMock<nullable_mock> mock{};
    EXPECT_CALL(mock, reset()).WillOnce(Return());
    ozo::reset_nullable(mock);
}

}// namespace

namespace ozo {
namespace tests {

struct some_type {
    std::size_t size() const {
        return 1000;
    }
    decltype(auto) begin() const { return std::addressof(v_);}
    char v_;
};

struct builtin_type { std::int64_t v = 0; };

}
}

OZO_PG_DEFINE_CUSTOM_TYPE(ozo::tests::some_type, "some_type", dynamic_size)
OZO_PG_DEFINE_TYPE(ozo::tests::builtin_type, "builtin_type", 5, bytes<8>)

BOOST_FUSION_DEFINE_STRUCT(
    (ozo)(tests), fusion_adapted,
    (std::string, name)
    (int, age))

namespace {

using namespace ozo::tests;

auto oid_map = ozo::register_types<ozo::tests::some_type>();

TEST(type_name, should_return_type_name_object) {
    using namespace std::string_literals;
    EXPECT_EQ(ozo::type_name(ozo::tests::some_type{}), "some_type"s);
}

TEST(size_of, should_return_size_from_traits_for_static_size_type) {
    EXPECT_EQ(ozo::size_of(ozo::tests::builtin_type{}), 8u);
}

TEST(size_of, should_return_size_from_method_size_for_dynamic_size_objects) {
    EXPECT_EQ(ozo::size_of(ozo::tests::some_type{}), 1000u);
}

TEST(type_oid, should_return_oid_from_traits_for_buildin_type) {
    EXPECT_EQ(ozo::type_oid(oid_map, ozo::tests::builtin_type{}), 5u);
}

TEST(type_oid, should_return_oid_from_oid_map_for_custom_type) {
    const auto val = ozo::tests::some_type{};
    ozo::set_type_oid<ozo::tests::some_type>(oid_map, 333);
    EXPECT_EQ(ozo::type_oid(oid_map, val), 333u);
}

TEST(accepts_oid, should_return_true_for_type_with_oid_in_map_and_same_oid_argument) {
    const auto val = ozo::tests::some_type{};
    ozo::set_type_oid<ozo::tests::some_type>(oid_map, 222);
    EXPECT_TRUE(ozo::accepts_oid(oid_map, val, 222));
}

TEST(accepts_oid, should_return_false_for_type_with_oid_in_map_and_different_oid_argument) {
    const auto val = ozo::tests::some_type{};
    ozo::set_type_oid<ozo::tests::some_type>(oid_map, 222);
    EXPECT_TRUE(!ozo::accepts_oid(oid_map, val, 0));
}

TEST(is_composite, should_return_false_for_std_string) {
    EXPECT_TRUE(!ozo::is_composite<std::string>::value);
}

TEST(is_composite, should_return_true_for_std_tuple) {
    using tuple = std::tuple<int,std::string,double>;
    EXPECT_TRUE(ozo::is_composite<tuple>::value);
}

TEST(is_composite, should_return_true_for_boost_fusion_adapted_struct) {
    EXPECT_TRUE(ozo::is_composite<ozo::tests::fusion_adapted>::value);
}

} // namespace

namespace ozo::tests {

struct hana_adapted {
    BOOST_HANA_DEFINE_STRUCT(hana_adapted,
        (std::string, brand),
        (std::string, model)
    );
};

} // namespace ozo::tests

namespace {

TEST(is_composite, should_return_true_for_boost_hana_adapted_struct) {
    EXPECT_TRUE(ozo::is_composite<ozo::tests::hana_adapted>::value);
}

} //namespace
