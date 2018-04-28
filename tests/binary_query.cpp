#include <GUnit/GTest.h>

#include <ozo/binary_query.h>

#include <iterator>

namespace hana = ::boost::hana;

namespace {

struct fixed_size_struct {
    BOOST_HANA_DEFINE_STRUCT(fixed_size_struct,
        (std::int64_t, value)
    );
};

struct dynamic_size_struct {
    BOOST_HANA_DEFINE_STRUCT(dynamic_size_struct,
        (std::string, value)
    );
};

} // namespace

namespace ozo {

template <>
struct send_impl<fixed_size_struct> {
    template <typename M>
    static ostream& apply(ostream& out, const oid_map_t<M>& oid_map, const fixed_size_struct& in) {
        using ozo::size_of;
        send(out, oid_map, std::int32_t(1));
        send(out, oid_map, ozo::type_oid(oid_map, in.value));
        send(out, oid_map, std::int32_t(sizeof(in.value)));
        send(out, oid_map, in.value);
        return out;
    }
};

template <>
struct send_impl<dynamic_size_struct> {
    template <typename M>
    static ostream& apply(ostream& out, const oid_map_t<M>& oid_map, const dynamic_size_struct& in) {
        using ozo::size_of;
        send(out, oid_map, std::int32_t(1));
        send(out, oid_map, ozo::type_oid(oid_map, in.value));
        send(out, oid_map, std::int32_t(size_of(in.value)));
        send(out, oid_map, in.value);
        return out;
    }
};

} // namespace ozo

GTEST("ozo::binary_query::params_count", "without parameters should be equal to 0") {
    const auto query = ozo::make_binary_query("", hana::make_tuple());
    EXPECT_EQ(query.params_count, 0);
}

GTEST("ozo::query") {
    SHOULD("with more than 0 parameters should be equal to that number") {
        const auto query = ozo::make_binary_query("", hana::make_tuple(true, 42, std::string("text")));
        EXPECT_EQ(query.params_count, 3);
    }

    SHOULD("from query concept with more than 0 parameters should be equal to that number") {
        const auto query = ozo::make_binary_query(ozo::make_query("", true, 42, std::string("text")));
        EXPECT_EQ(query.params_count, 3);
    }
}

GTEST("ozo::binary_query::text", "query text should be equal to input") {
    const auto query = ozo::make_binary_query("query", hana::make_tuple());
    EXPECT_STREQ(query.text(), "query");
}

GTEST("ozo::binary_query::types") {
    SHOULD("type of the param should be equal to type oid") {
        const auto query = ozo::make_binary_query("", hana::make_tuple(std::int16_t()));
        EXPECT_EQ(query.types()[0], ozo::type_traits<std::int16_t>::oid());
    }

    SHOULD("nullptr type should be equal to 0") {
        const auto query = ozo::make_binary_query("", hana::make_tuple(nullptr));
        EXPECT_EQ(query.types()[0], 0);
    }

    SHOULD("null std::optional<std::int32_t> type should be equal to std::int32_t oid") {
        const auto query = ozo::make_binary_query("", hana::make_tuple(__OZO_STD_OPTIONAL<std::int32_t>()));
        EXPECT_EQ(query.types()[0], ozo::type_traits<std::int32_t>::oid());
    }

    SHOULD("null std::shared_ptr<std::int32_t> type should be equal to std::int32_t oid") {
        const auto query = ozo::make_binary_query("", hana::make_tuple(std::shared_ptr<std::int32_t>()));
        EXPECT_EQ(query.types()[0], ozo::type_traits<std::int32_t>::oid());
    }

    SHOULD("null std::unique_ptr<std::int32_t> type should be equal to std::int32_t oid") {
        const auto query = ozo::make_binary_query("", hana::make_tuple(std::vector<std::int32_t>()));
        EXPECT_EQ(query.types()[0], ozo::type_traits<std::vector<std::int32_t>>::oid());
    }

    SHOULD("null std::weak_ptr<std::int32_t> type should be equal to std::int32_t oid") {
        const auto query = ozo::make_binary_query("", hana::make_tuple(std::weak_ptr<std::int32_t>()));
        EXPECT_EQ(query.types()[0], ozo::type_traits<std::int32_t>::oid());
    }

    SHOULD("custom struct type should be equal to oid from map") {
        auto oid_map = ozo::register_types<fixed_size_struct>();
        ozo::set_type_oid<fixed_size_struct>(oid_map, 100500);
        const auto query = ozo::make_binary_query("", hana::make_tuple(fixed_size_struct {42}), oid_map);
        EXPECT_EQ(query.types()[0], 100500);
    }
}

GTEST("ozo::binary_query::formats", "format of the param should be equal to 1") {
    const auto query = ozo::make_binary_query("", hana::make_tuple(std::int16_t()));
    EXPECT_EQ(query.formats()[0], 1);
}

GTEST("ozo::binary_query::lengths") {
    SHOULD("length of the param should be equal to its binary serialized data size") {
        const auto query = ozo::make_binary_query("", hana::make_tuple(std::int16_t()));
        EXPECT_EQ(query.lengths()[0], sizeof(std::int16_t));
    }

    SHOULD("string length should be equal to number of chars") {
        const auto query = ozo::make_binary_query("", hana::make_tuple(std::string("std::string")));
        EXPECT_EQ(query.lengths()[0], 11);
    }

    SHOULD("fixed struct length should be equal to size of type") {
        auto oid_map = ozo::register_types<fixed_size_struct>();
        ozo::set_type_oid<fixed_size_struct>(oid_map, 100500);
        const auto query = ozo::make_binary_query("", hana::make_tuple(fixed_size_struct {42}), oid_map);
        EXPECT_EQ(query.lengths()[0], 20);
    }

    SHOULD("dynamic struct length should be equal to size of content") {
        auto oid_map = ozo::register_types<dynamic_size_struct>();
        ozo::set_type_oid<dynamic_size_struct>(oid_map, 100500);
        const dynamic_size_struct value {"Lorem ipsum dolor sit amet, consectetur adipiscing elit"};
        const auto query = ozo::make_binary_query("", hana::make_tuple(value), oid_map);
        EXPECT_EQ(query.lengths()[0], 67);
    }
}

GTEST("ozo::binary_query::values") {

    SHOULD("string value should be equal to input") {
        using namespace ::testing;
        const auto query = ozo::make_binary_query("", hana::make_tuple(std::string("string")));
        EXPECT_THAT(std::vector<char>(query.values()[0], query.values()[0] + 6),
            ElementsAre('s', 't', 'r', 'i', 'n', 'g'));
    }

    SHOULD("nullptr value should be equal to nullptr") {
        const auto query = ozo::make_binary_query("", hana::make_tuple(nullptr));
        EXPECT_EQ(query.values()[0], nullptr);
    }

    SHOULD("nullopt value should be equal to nullptr") {
        const auto query = ozo::make_binary_query("", hana::make_tuple(__OZO_NULLOPT));
        EXPECT_EQ(query.values()[0], nullptr);
    }

    SHOULD("null std::optional<std::int32_t> value should be equal to nullptr") {
        const auto query = ozo::make_binary_query("", hana::make_tuple(__OZO_STD_OPTIONAL<std::int32_t>()));
        EXPECT_EQ(query.values()[0], nullptr);
    }

    SHOULD("not null std::optional<std::string> value should be equal to input") {
        using namespace ::testing;
        const auto query = ozo::make_binary_query("", hana::make_tuple(__OZO_STD_OPTIONAL<std::string>("string")));
        EXPECT_THAT(std::vector<char>(query.values()[0], query.values()[0] + 6),
            ElementsAre('s', 't', 'r', 'i', 'n', 'g'));
    }

    SHOULD("null std::shared_ptr<std::int32_t> value should be equal to nullptr") {
        const auto query = ozo::make_binary_query("", hana::make_tuple(std::shared_ptr<std::int32_t>()));
        EXPECT_EQ(query.values()[0], nullptr);
    }

    SHOULD("not null std::shared_ptr<std::string> value should be equal to input") {
        using namespace ::testing;
        const auto query = ozo::make_binary_query("", hana::make_tuple(std::make_shared<std::string>("string")));
        EXPECT_THAT(std::vector<char>(query.values()[0], query.values()[0] + 6),
            ElementsAre('s', 't', 'r', 'i', 'n', 'g'));
    }

    SHOULD("null std::unique_ptr<std::int32_t> value should be equal to nullptr") {
        const auto query = ozo::make_binary_query("", hana::make_tuple(std::unique_ptr<std::int32_t>()));
        EXPECT_EQ(query.values()[0], nullptr);
    }

    SHOULD("not null std::unique_ptr<std::string> value should be equal to input") {
        using namespace ::testing;
        const auto query = ozo::make_binary_query("", hana::make_tuple(std::make_unique<std::string>("string")));
        EXPECT_THAT(std::vector<char>(query.values()[0], query.values()[0] + 6),
            ElementsAre('s', 't', 'r', 'i', 'n', 'g'));
    }

    SHOULD("null std::weak_ptr<std::int32_t> value should be equal to nullptr") {
        const auto query = ozo::make_binary_query("", hana::make_tuple(std::weak_ptr<std::int32_t>()));
        EXPECT_EQ(query.values()[0], nullptr);
    }

    SHOULD("not null std::weak_ptr<std::string> value should be equal to input") {
        using namespace ::testing;
        const auto ptr = std::make_shared<std::string>("string");
        const auto query = ozo::make_binary_query("", hana::make_tuple(std::weak_ptr<std::string>(ptr)));
        EXPECT_THAT(std::vector<char>(query.values()[0], query.values()[0] + 6),
            ElementsAre('s', 't', 'r', 'i', 'n', 'g'));
    }

    SHOULD("std::reference_wrapper<std::string> value should be equal to input") {
        using namespace ::testing;
        const auto value = std::string("string");
        const auto query = ozo::make_binary_query("", hana::make_tuple(std::cref(value)));
        EXPECT_THAT(std::vector<char>(query.values()[0], query.values()[0] + 6),
            ElementsAre('s', 't', 'r', 'i', 'n', 'g'));
    }
}
