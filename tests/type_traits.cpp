#include <apq/type_traits.h>

#include <boost/make_shared.hpp>
#include <boost/fusion/adapted/std_tuple.hpp>
#include <boost/fusion/adapted/struct/define_struct.hpp>
#include <boost/fusion/include/define_struct.hpp>

#include <boost/hana/adapt_adt.hpp>

#include <GUnit/GTest.h>

namespace hana = ::boost::hana;

GTEST("libapq::is_null<nullable>()", "[for non initialized optional returns true]") {
    EXPECT_TRUE(libapq::is_null(boost::optional<int>{}));
}

GTEST("libapq::is_null<nullable>()", "[for initialized optional returns false]") {
    EXPECT_TRUE(!libapq::is_null(boost::optional<int>{0}));
}


GTEST("libapq::is_null<std::weak_ptr>()", "[for valid pointer returns false]") {
    auto ptr = std::make_shared<int>();
    EXPECT_TRUE(!libapq::is_null(std::weak_ptr<int>{ptr}));
}

GTEST("libapq::is_null<std::weak_ptr>()", "[for expired pointer returns false]") {
    std::weak_ptr<int> ptr;
    {
        ptr = std::make_shared<int>();
    }
    EXPECT_TRUE(libapq::is_null(ptr));
}

GTEST("libapq::is_null<std::weak_ptr>()", "[for uninitialized pointer returns false]") {
    std::weak_ptr<int> ptr;
    EXPECT_TRUE(libapq::is_null(ptr));
}


GTEST("libapq::is_null<boost::weak_ptr>()", "[for valid pointer returns false]") {
    auto ptr = boost::make_shared<int>();
    EXPECT_TRUE(!libapq::is_null(boost::weak_ptr<int>(ptr)));
}

GTEST("libapq::is_null<boost::weak_ptr>()", "[for expired pointer returns false]") {
    boost::weak_ptr<int> ptr;
    {
        ptr = boost::make_shared<int>();
    }
    EXPECT_TRUE(libapq::is_null(ptr));
}

GTEST("libapq::is_null<boost::weak_ptr>()", "[for uninitialized pointer returns false]") {
    boost::weak_ptr<int> ptr;
    EXPECT_TRUE(libapq::is_null(ptr));
}


GTEST("libapq::is_null<not nullable>()", "[for int returns false]") {
    EXPECT_TRUE(!libapq::is_null(int()));
}

namespace libapq {
namespace testing {
    struct some_type {
        std::size_t size() const {
            return 1000;
        }
    };
    struct builtin_type {};
} 
}

LIBAPQ_PG_DEFINE_CUSTOM_TYPE(libapq::testing::some_type, "some_type", dynamic_size)
LIBAPQ_PG_DEFINE_TYPE(libapq::testing::builtin_type, "builtin_type", 5, bytes<8>)

auto oid_map = libapq::register_types<libapq::testing::some_type>();

GTEST("libapq::type_name()", "[for defined type returns name from traits]") {
    using namespace std::string_literals;
    EXPECT_EQ(libapq::type_name(libapq::testing::some_type{}), "some_type"s);
}

GTEST("libapq::size_of()", "[for static size type returns size from traits]") {
    EXPECT_EQ(libapq::size_of(libapq::testing::builtin_type{}), 8);
}

GTEST("libapq::size_of()", "[for dynamic size type returns size from objects method size()]") {
    EXPECT_EQ(libapq::size_of(libapq::testing::some_type{}), 1000);
}

GTEST("libapq::type_oid()", "[for defined builtin type returns oid from traits]") {
    EXPECT_EQ(libapq::type_oid(oid_map, libapq::testing::builtin_type{}), 5);
}

GTEST("libapq::type_oid()", "[for defined custom type returns oid from map]") {
    const auto val = libapq::testing::some_type{};
    libapq::set_type_oid<libapq::testing::some_type>(oid_map, 333);
    EXPECT_EQ(libapq::type_oid(oid_map, val), 333);
}

GTEST("libapq::accepts_oid()", "[for type with oid in map and same oid argument returns true]") {
    const auto val = libapq::testing::some_type{};
    libapq::set_type_oid<libapq::testing::some_type>(oid_map, 222);
    EXPECT_TRUE(libapq::accepts_oid(oid_map, val, 222));
}

GTEST("libapq::accepts_oid()", "[for type with oid in map and different oid argument returns false]") {
    const auto val = libapq::testing::some_type{};
    libapq::set_type_oid<libapq::testing::some_type>(oid_map, 222);
    EXPECT_TRUE(!libapq::accepts_oid(oid_map, val, 0));
}

GTEST("libapq::is_composite<std::string>", "[returns false]") {
    EXPECT_TRUE(!libapq::is_composite<std::string>::value);
}

GTEST("libapq::is_composite<std::tuple<T...>>", "[returns true]") {
    using tuple = std::tuple<int,std::string,double>;
    EXPECT_TRUE(libapq::is_composite<tuple>::value);
}

BOOST_FUSION_DEFINE_STRUCT(
    (testing), fusion_adapted,
    (std::string, name)
    (int, age))

GTEST("libapq::is_composite<adapted>", "[with Boost.Fusion adapted structure returns true]") {
    EXPECT_TRUE(libapq::is_composite<testing::fusion_adapted>::value);
}

namespace testing {

struct hana_adapted {
  BOOST_HANA_DEFINE_STRUCT(hana_adapted,
    (std::string, brand),
    (std::string, model)
  );
};

} // namespace testing

GTEST("libapq::is_composite<adapted>", "[with Boost.Hana adapted structure returns true]") {
    EXPECT_TRUE(libapq::is_composite<testing::hana_adapted>::value);
}
