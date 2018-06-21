#include <GUnit/GTest.h>

#include <ozo/binary_query.h>

#include <iterator>

namespace hana = ::boost::hana;

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
