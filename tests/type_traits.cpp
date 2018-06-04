#include <ozo/type_traits.h>
#include "concept_mock.h"

#include <boost/make_shared.hpp>
#include <boost/fusion/adapted/std_tuple.hpp>
#include <boost/fusion/adapted/struct/define_struct.hpp>
#include <boost/fusion/include/define_struct.hpp>

#include <boost/hana/adapt_adt.hpp>

#include <GUnit/GTest.h>

namespace hana = ::boost::hana;
using namespace ::testing;
using namespace ::ozo::testing;

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

GTEST("ozo::unwrap_nullable") {
    SHOULD("unwrap boost::optional") {
        boost::optional<int> n(7);
        EXPECT_EQ(ozo::unwrap_nullable(n), 7);
    }
}

GTEST("ozo::allocate_nullable") {
    SHOULD("allocate a boost::scoped_ptr") {
        using ptr_t = boost::scoped_ptr<int>;
        ptr_t p = ozo::allocate_nullable<int, ptr_t>(std::allocator<int>());
        EXPECT_TRUE(p);
    }
    SHOULD("allocate a std::unique_ptr with default deleter") {
        using ptr_t = std::unique_ptr<int>;
        ptr_t p = ozo::allocate_nullable<int, ptr_t>(std::allocator<int>());
        EXPECT_TRUE(p);
    }
    SHOULD("allocate a boost::shared_ptr") {
        using ptr_t = boost::shared_ptr<int>;
        ptr_t p = ozo::allocate_nullable<int, ptr_t>(std::allocator<int>());
        EXPECT_TRUE(p);
    }
    SHOULD("allocate a std::shared_ptr") {
        using ptr_t = std::shared_ptr<int>;
        ptr_t p = ozo::allocate_nullable<int, ptr_t>(std::allocator<int>());
        EXPECT_TRUE(p);
    }
}

using emplaceable_nullable = emplaceable<
    operator_not<
    nullable<
    concept
>>>;

template <typename V>
using swappable_nullable = swappable<
    has_element<V,
    operator_not<
    nullable<
    concept
>>>>;

// This wrapper has no pure virtual methods,
// and therefore can be created in allocate_nullable.
// It simply redirects all calls to a StrictGMock::object
// supplied via static reference.
template <typename V>
class swappable_mock_wrapper : public swappable_nullable<V>
{
public:
    static boost::optional<swappable_nullable<V>&> mock;

    swappable_mock_wrapper() = default;

    virtual bool negate() const override { return mock.get().negate(); }
    virtual void swap(swappable_nullable<V>& rhs) override { mock.get().swap(rhs); }
};

template <typename V>
boost::optional<swappable_nullable<V>&> swappable_mock_wrapper<V>::mock;

namespace ozo {

template <>
inline swappable_mock_wrapper<int> allocate_nullable<
        int,
        swappable_mock_wrapper<int>,
        std::allocator<int>>(
            const std::allocator<int>&) {
    return swappable_mock_wrapper<int>{};
}

} // namespace ozo

GTEST("ozo::init_nullable") {
    SHOULD("initialize an empty Nullable with emplace() if possible") {
        StrictGMock<emplaceable_nullable> mock{};
        EXPECT_INVOKE(mock, emplace);
        EXPECT_INVOKE(mock, negate).WillOnce(Return(true));
        ozo::init_nullable(mock.object());
    }

    SHOULD("do nothing with an emplaceable Nullable that contains a value") {
        StrictGMock<emplaceable_nullable> mock{};
        EXPECT_INVOKE(mock, negate).WillOnce(Return(false));
        ozo::init_nullable(mock.object());
    }

    SHOULD("initialize an empty swappable Nullable with a new nullable containing a default-constructed element") {
        StrictGMock<swappable_nullable<int>> mock{};
        swappable_mock_wrapper<int>::mock.reset(mock.object());
        EXPECT_INVOKE(mock, swap, _);
        EXPECT_INVOKE(mock, negate).WillOnce(Return(true));
        auto mock_wrapper = swappable_mock_wrapper<int>{};
        ozo::init_nullable(mock_wrapper);
    }

    SHOULD("do nothing with a swappable Nullable that contains a value") {
        StrictGMock<swappable_nullable<int>> mock{};
        swappable_mock_wrapper<int>::mock.reset(mock.object());
        EXPECT_INVOKE(mock, negate).WillOnce(Return(false));
        auto mock_wrapper = swappable_mock_wrapper<int>{};
        ozo::init_nullable(mock_wrapper);
    }
}

GTEST("ozo::reset_nullable") {
    SHOULD("reset a nullable that contains a value") {
        StrictGMock<resettable<nullable<concept>>> mock{};
        EXPECT_INVOKE(mock, reset);
        ozo::reset_nullable(mock.object());
    }
}

namespace ozo {
namespace testing {
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
