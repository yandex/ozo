#include <GUnit/GTest.h>

#include <ozo/binary_query.h>

#include <iterator>

GTEST("ozo::binary_query::params_count", "without parameters should be equal to 0") {
    const auto query = ozo::make_binary_query("");
    EXPECT_EQ(query.params_count, 0);
}

GTEST("ozo::query") {
    SHOULD("with more than 0 parameters should be equal to that number") {
        const auto query = ozo::make_binary_query("", true, 42, std::string("text"));
        EXPECT_EQ(query.params_count, 3);
    }

    SHOULD("from query concept with more than 0 parameters should be equal to that number") {
        const auto query = ozo::make_binary_query(ozo::make_query("", true, 42, std::string("text")));
        EXPECT_EQ(query.params_count, 3);
    }
}

GTEST("ozo::binary_query::text", "query text should be equal to input") {
    const auto query = ozo::make_binary_query("query");
    EXPECT_STREQ(query.text(), "query");
}

GTEST("ozo::binary_query::types") {
    SHOULD("type of the param should be equal to type oid") {
        const auto query = ozo::make_binary_query("", std::int16_t());
        EXPECT_EQ(query.types()[0], ozo::type_traits<std::int16_t>::oid());
    }

    SHOULD("nullptr type should be equal to 0") {
        const auto query = ozo::make_binary_query("", nullptr);
        EXPECT_EQ(query.types()[0], 0);
    }

    SHOULD("null std::optional<std::int32_t> type should be equal to std::int32_t oid") {
        const auto query = ozo::make_binary_query("", std::optional<std::int32_t>());
        EXPECT_EQ(query.types()[0], ozo::type_traits<std::int32_t>::oid());
    }

    SHOULD("null std::shared_ptr<std::int32_t> type should be equal to std::int32_t oid") {
        const auto query = ozo::make_binary_query("", std::shared_ptr<std::int32_t>());
        EXPECT_EQ(query.types()[0], ozo::type_traits<std::int32_t>::oid());
    }

    SHOULD("null std::unique_ptr<std::int32_t> type should be equal to std::int32_t oid") {
        const auto query = ozo::make_binary_query("", std::vector<std::int32_t>());
        EXPECT_EQ(query.types()[0], ozo::type_traits<std::vector<std::int32_t>>::oid());
    }

    SHOULD("null std::weak_ptr<std::int32_t> type should be equal to std::int32_t oid") {
        const auto query = ozo::make_binary_query("", std::weak_ptr<std::int32_t>());
        EXPECT_EQ(query.types()[0], ozo::type_traits<std::int32_t>::oid());
    }

    SHOULD("std::vector<float> type should be equal to std::vector<float> oid") {
        const auto value = 42.13f;
        const auto query = ozo::make_binary_query("", std::cref(value));
        EXPECT_EQ(*reinterpret_cast<const float*>(query.values()[0]), 42.13f);
    }
}

GTEST("ozo::binary_query::formats", "format of the param should be equal to 1") {
    const auto query = ozo::make_binary_query("", std::int16_t());
    EXPECT_EQ(query.formats()[0], 1);
}

GTEST("ozo::binary_query::lengths") {
    SHOULD("length of the param should be equal to its binary serialized data size") {
        const auto query = ozo::make_binary_query("", std::int16_t());
        EXPECT_EQ(query.lengths()[0], sizeof(std::int16_t));
    }

    SHOULD("string length should be equal to number of chars") {
        const auto query = ozo::make_binary_query("", std::string("std::string"));
        EXPECT_EQ(query.lengths()[0], 11);
    }
}

GTEST("ozo::binary_query::values") {
    SHOULD("value of the floating point type param should be equal to input") {
        const auto query = ozo::make_binary_query("", 42.13f);
        EXPECT_EQ(*reinterpret_cast<const float*>(query.values()[0]), 42.13f);
    }

    SHOULD("string value should be equal to input") {
        using namespace ::testing;
        const auto query = ozo::make_binary_query("", std::string("string"));
        EXPECT_THAT(std::vector<char>(query.values()[0], query.values()[0] + 6),
            ElementsAre('s', 't', 'r', 'i', 'n', 'g'));
    }

    SHOULD("nullptr value should be equal to nullptr") {
        const auto query = ozo::make_binary_query("", nullptr);
        EXPECT_EQ(query.values()[0], nullptr);
    }

    SHOULD("nullopt value should be equal to nullptr") {
        const auto query = ozo::make_binary_query("", std::nullopt);
        EXPECT_EQ(query.values()[0], nullptr);
    }

    SHOULD("null std::optional<std::int32_t> value should be equal to nullptr") {
        const auto query = ozo::make_binary_query("", std::optional<std::int32_t>());
        EXPECT_EQ(query.values()[0], nullptr);
    }

    SHOULD("not null std::optional<std::string> value should be equal to input") {
        using namespace ::testing;
        const auto query = ozo::make_binary_query("", std::optional<std::string>("string"));
        EXPECT_THAT(std::vector<char>(query.values()[0], query.values()[0] + 6),
            ElementsAre('s', 't', 'r', 'i', 'n', 'g'));
    }

    SHOULD("null std::shared_ptr<std::int32_t> value should be equal to nullptr") {
        const auto query = ozo::make_binary_query("", std::shared_ptr<std::int32_t>());
        EXPECT_EQ(query.values()[0], nullptr);
    }

    SHOULD("not null std::shared_ptr<std::string> value should be equal to input") {
        using namespace ::testing;
        const auto query = ozo::make_binary_query("", std::make_shared<std::string>("string"));
        EXPECT_THAT(std::vector<char>(query.values()[0], query.values()[0] + 6),
            ElementsAre('s', 't', 'r', 'i', 'n', 'g'));
    }

    SHOULD("null std::unique_ptr<std::int32_t> value should be equal to nullptr") {
        const auto query = ozo::make_binary_query("", std::unique_ptr<std::int32_t>());
        EXPECT_EQ(query.values()[0], nullptr);
    }

    SHOULD("not null std::unique_ptr<std::string> value should be equal to input") {
        using namespace ::testing;
        const auto query = ozo::make_binary_query("", std::make_unique<std::string>("string"));
        EXPECT_THAT(std::vector<char>(query.values()[0], query.values()[0] + 6),
            ElementsAre('s', 't', 'r', 'i', 'n', 'g'));
    }

    SHOULD("null std::weak_ptr<std::int32_t> value should be equal to nullptr") {
        const auto query = ozo::make_binary_query("", std::weak_ptr<std::int32_t>());
        EXPECT_EQ(query.values()[0], nullptr);
    }

    SHOULD("not null std::weak_ptr<std::string> value should be equal to input") {
        using namespace ::testing;
        const auto ptr = std::make_shared<std::string>("string");
        const auto query = ozo::make_binary_query("", std::weak_ptr<std::string>(ptr));
        EXPECT_THAT(std::vector<char>(query.values()[0], query.values()[0] + 6),
            ElementsAre('s', 't', 'r', 'i', 'n', 'g'));
    }

    SHOULD("std::reference_wrapper<std::int8_t> value should be equal to input") {
        const auto value = 42.13f;
        const auto query = ozo::make_binary_query("", std::cref(value));
        EXPECT_EQ(*reinterpret_cast<const float*>(query.values()[0]), 42.13f);
    }
}
