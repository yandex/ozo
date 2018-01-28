#include <ozo/type_traits.h>

#include <boost/make_shared.hpp>
#include <boost/fusion/adapted/std_tuple.hpp>
#include <boost/fusion/adapted/struct/define_struct.hpp>
#include <boost/fusion/include/define_struct.hpp>

#include <boost/hana/adapt_adt.hpp>

#include <GUnit/GTest.h>

namespace hana = ::boost::hana;

GTEST("ozo::is_null<nullable>()", "[for non initialized optional returns true]") {
    EXPECT_TRUE(ozo::is_null(boost::optional<int>{}));
}

GTEST("ozo::is_null<nullable>()", "[for initialized optional returns false]") {
    EXPECT_TRUE(!ozo::is_null(boost::optional<int>{0}));
}


GTEST("ozo::is_null<std::weak_ptr>()", "[for valid pointer returns false]") {
    auto ptr = std::make_shared<int>();
    EXPECT_TRUE(!ozo::is_null(std::weak_ptr<int>{ptr}));
}

GTEST("ozo::is_null<std::weak_ptr>()", "[for expired pointer returns false]") {
    std::weak_ptr<int> ptr;
    {
        ptr = std::make_shared<int>();
    }
    EXPECT_TRUE(ozo::is_null(ptr));
}

GTEST("ozo::is_null<std::weak_ptr>()", "[for uninitialized pointer returns false]") {
    std::weak_ptr<int> ptr;
    EXPECT_TRUE(ozo::is_null(ptr));
}


GTEST("ozo::is_null<boost::weak_ptr>()", "[for valid pointer returns false]") {
    auto ptr = boost::make_shared<int>();
    EXPECT_TRUE(!ozo::is_null(boost::weak_ptr<int>(ptr)));
}

GTEST("ozo::is_null<boost::weak_ptr>()", "[for expired pointer returns false]") {
    boost::weak_ptr<int> ptr;
    {
        ptr = boost::make_shared<int>();
    }
    EXPECT_TRUE(ozo::is_null(ptr));
}

GTEST("ozo::is_null<boost::weak_ptr>()", "[for uninitialized pointer returns false]") {
    boost::weak_ptr<int> ptr;
    EXPECT_TRUE(ozo::is_null(ptr));
}


GTEST("ozo::is_null<not nullable>()", "[for int returns false]") {
    EXPECT_TRUE(!ozo::is_null(int()));
}

namespace ozo {
namespace testing {
    struct some_type {
        std::size_t size() const {
            return 1000;
        }
    };
    struct builtin_type {};
}
}

OZO_PG_DEFINE_CUSTOM_TYPE(ozo::testing::some_type, "some_type", dynamic_size)
OZO_PG_DEFINE_TYPE(ozo::testing::builtin_type, "builtin_type", 5, bytes<8>)

auto oid_map = ozo::register_types<ozo::testing::some_type>();

GTEST("ozo::type_name()", "[for defined type returns name from traits]") {
    using namespace std::string_literals;
    EXPECT_EQ(ozo::type_name(ozo::testing::some_type{}), "some_type"s);
}

GTEST("ozo::size_of()", "[for static size type returns size from traits]") {
    EXPECT_EQ(ozo::size_of(ozo::testing::builtin_type{}), 8);
}

GTEST("ozo::size_of()", "[for dynamic size type returns size from objects method size()]") {
    EXPECT_EQ(ozo::size_of(ozo::testing::some_type{}), 1000);
}

GTEST("ozo::type_oid()", "[for defined builtin type returns oid from traits]") {
    EXPECT_EQ(ozo::type_oid(oid_map, ozo::testing::builtin_type{}), 5);
}

GTEST("ozo::type_oid()", "[for defined custom type returns oid from map]") {
    const auto val = ozo::testing::some_type{};
    ozo::set_type_oid<ozo::testing::some_type>(oid_map, 333);
    EXPECT_EQ(ozo::type_oid(oid_map, val), 333);
}

GTEST("ozo::accepts_oid()", "[for type with oid in map and same oid argument returns true]") {
    const auto val = ozo::testing::some_type{};
    ozo::set_type_oid<ozo::testing::some_type>(oid_map, 222);
    EXPECT_TRUE(ozo::accepts_oid(oid_map, val, 222));
}

GTEST("ozo::accepts_oid()", "[for type with oid in map and different oid argument returns false]") {
    const auto val = ozo::testing::some_type{};
    ozo::set_type_oid<ozo::testing::some_type>(oid_map, 222);
    EXPECT_TRUE(!ozo::accepts_oid(oid_map, val, 0));
}

GTEST("ozo::is_composite<std::string>", "[returns false]") {
    EXPECT_TRUE(!ozo::is_composite<std::string>::value);
}

GTEST("ozo::is_composite<std::tuple<T...>>", "[returns true]") {
    using tuple = std::tuple<int,std::string,double>;
    EXPECT_TRUE(ozo::is_composite<tuple>::value);
}

BOOST_FUSION_DEFINE_STRUCT(
    (testing), fusion_adapted,
    (std::string, name)
    (int, age))

GTEST("ozo::is_composite<adapted>", "[with Boost.Fusion adapted structure returns true]") {
    EXPECT_TRUE(ozo::is_composite<testing::fusion_adapted>::value);
}

namespace testing {

struct hana_adapted {
  BOOST_HANA_DEFINE_STRUCT(hana_adapted,
    (std::string, brand),
    (std::string, model)
  );
};

} // namespace testing

GTEST("ozo::is_composite<adapted>", "[with Boost.Hana adapted structure returns true]") {
    EXPECT_TRUE(ozo::is_composite<testing::hana_adapted>::value);
}
