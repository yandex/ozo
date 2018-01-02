#include <GUnit/GTest.h>

#include <ozo/query_conf.h>

#include <boost/hana/adapt_struct.hpp>
#include <boost/hana/contains.hpp>
#include <boost/hana/string.hpp>
#include <boost/hana/tuple.hpp>

#include <string_view>
#include <cstring>

namespace ozo {
namespace detail {

bool operator ==(const query_description& lhs, const query_description& rhs) {
    return lhs.name == lhs.name && rhs.text == rhs.text;
}

std::ostream& operator <<(std::ostream& stream, const query_description& value) {
    return stream << "query_description {\"" << value.name << "\", \"" << value.text << "\"}";
}

} // namespace detail

namespace impl {

template < class ... ParamsT>
bool operator ==(const query<ParamsT ...>& lhs, const query<ParamsT ...>& rhs) {
    return lhs.text == rhs.text && lhs.params == rhs.params;
}

} // namespace impl

namespace testing {

using namespace boost::hana::literals;

struct query_without_parameters {
    static constexpr auto name = "query without parameters"_s;
    using parameters_type = std::tuple<>;
};

struct query_with_one_parameter {
    static constexpr auto name = "query with one parameter"_s;
    using parameters_type = std::tuple<std::string>;
};

struct struct_parameters {
    std::string_view string;
    int number;
};

struct query_with_struct_parameters {
    static constexpr auto name = "query with struct parameter"_s;
    using parameters_type = struct_parameters;
};

} // namespace testing
} // namespace ozo

BOOST_HANA_ADAPT_STRUCT(
    ozo::testing::struct_parameters,
    string,
    number
);

GTEST("ozo::detail::parse_query_conf()") {
    using namespace testing;

    SHOULD("for empty const char[] returns empty descriptions") {
        EXPECT_THAT(ozo::detail::parse_query_conf(""), ElementsAre());
    }

    SHOULD("for empty std::string_view returns empty descriptions") {
        EXPECT_THAT(ozo::detail::parse_query_conf(std::string_view("")), ElementsAre());
    }

    SHOULD("for empty std::string returns empty descriptions") {
        EXPECT_THAT(ozo::detail::parse_query_conf(std::string("")), ElementsAre());
    }

    SHOULD("for empty iterators range returns empty descriptions") {
        const std::string_view content("");
        EXPECT_THAT(ozo::detail::parse_query_conf(content.begin(), content.end()), ElementsAre());
    }

    SHOULD("for one query returns one query descriptions") {
        EXPECT_THAT(
            ozo::detail::parse_query_conf("-- name: query without parameters\nSELECT 1"),
            ElementsAre(ozo::detail::query_description {"query without parameters", "SELECT 1", {}})
        );
    }

    SHOULD("for two queries returns two query descriptions") {
        EXPECT_THAT(
            ozo::detail::parse_query_conf("-- name: query without parameters 1\nSELECT 1\n-- name: query without parameters 2\nSELECT 2"),
            ElementsAre(
                ozo::detail::query_description {"query without parameters 1", "SELECT 1", {}},
                ozo::detail::query_description {"query without parameters 2", "SELECT 2", {}}
            )
        );
    }

    SHOULD("for two queries with multiline separator returns two query descriptions") {
        EXPECT_THAT(
            ozo::detail::parse_query_conf("-- name: query without parameters 1\nSELECT 1\n\n\n-- name: query without parameters 2\nSELECT 2"),
            ElementsAre(
                ozo::detail::query_description {"query without parameters 1", "SELECT 1", {}},
                ozo::detail::query_description {"query without parameters 2", "SELECT 2", {}}
            )
        );
    }

    SHOULD("for one query with one parameter returns query description with parameter") {
        EXPECT_THAT(
            ozo::detail::parse_query_conf("-- name: query with parameters\nSELECT :parameter"),
            ElementsAre(ozo::detail::query_description {"query with parameters", "SELECT $1", {{"parameter", 1}}})
        );
    }

    SHOULD("for one query with one parameter and explicit cast returns query description with parameter") {
        EXPECT_THAT(
            ozo::detail::parse_query_conf("-- name: query with parameters\nSELECT :parameter::integer"),
            ElementsAre(ozo::detail::query_description {"query with parameters", "SELECT $1::integer", {{"parameter", 1}}})
        );
    }

    SHOULD("for query containing eol returns same text") {
        EXPECT_THAT(
            ozo::detail::parse_query_conf("-- name: query without parameters\nSELECT\n1"),
            ElementsAre(ozo::detail::query_description {"query without parameters", "SELECT\n1", {}})
        );
    }

    DISABLED_SHOULD("for two queries containing eol returns same text") {
        EXPECT_THAT(
            ozo::detail::parse_query_conf("-- name: query without parameters 1\nSELECT\n1\n-- name: query without parameters 2\nSELECT\n2"),
            ElementsAre(
                ozo::detail::query_description {"query without parameters 1", "SELECT\n1", {}},
                ozo::detail::query_description {"query without parameters 2", "SELECT\n2", {}}
            )
        );
    }
}

GTEST("ozo::detail::make_query_conf") {
    using namespace testing;

    SHOULD("for empty descriptions returns empty descriptions and queries") {
        const auto result = ozo::detail::make_query_conf({});
        EXPECT_THAT(result->descriptions, ElementsAre());
        EXPECT_THAT(result->queries, ElementsAre());
    }

    SHOULD("for one description returns one description and one query") {
        const auto result = ozo::detail::make_query_conf({
            ozo::detail::query_description {"query without parameters", "SELECT 1", {}},
        });
        EXPECT_THAT(
            result->descriptions,
            ElementsAre(ozo::detail::query_description {"query without parameters", "SELECT 1", {}})
        );
        EXPECT_THAT(
            result->queries,
            ElementsAre(std::make_pair(std::string_view("query without parameters"), std::string_view("SELECT 1")))
        );
    }

    SHOULD("for two descriptions with different names returns tow descriptions and two queries") {
        const auto result = ozo::detail::make_query_conf({
            ozo::detail::query_description {"query without parameters 1", "SELECT 1", {}},
            ozo::detail::query_description {"query without parameters 2", "SELECT 2", {}},
        });
        EXPECT_THAT(
            result->descriptions,
            ElementsAre(
                ozo::detail::query_description {"query without parameters 1", "SELECT 1", {}},
                ozo::detail::query_description {"query without parameters 2", "SELECT 2", {}}
            )
        );
        EXPECT_THAT(
            result->queries,
            UnorderedElementsAre(
                std::make_pair(std::string_view("query without parameters 1"), std::string_view("SELECT 1")),
                std::make_pair(std::string_view("query without parameters 2"), std::string_view("SELECT 2"))
            )
        );
    }
}

GTEST("ozo::get_query_name", "for query type returns std::string_view to value of static field name") {
    EXPECT_EQ(ozo::get_query_name<ozo::testing::query_without_parameters>(), std::string_view("query without parameters"));
}

GTEST("ozo::detail::query_repository::make_query") {
    using namespace testing;

    SHOULD("for query conf with one query without parameters returns appropriate query") {
        const auto repository = ozo::make_query_repository<ozo::testing::query_without_parameters>(
            "-- name: query without parameters\n"
            "SELECT 1"
        );
        EXPECT_EQ(
            repository.make_query<ozo::testing::query_without_parameters>(),
            ozo::make_query("SELECT 1")
        );
    }

    SHOULD("for query conf with one query with one parameter returns appropriate query") {
        const auto repository = ozo::make_query_repository<ozo::testing::query_with_one_parameter>(
            "-- name: query with one parameter\n"
            "SELECT :parameter::integer"
        );
        EXPECT_EQ(
            repository.make_query<ozo::testing::query_with_one_parameter>(42),
            ozo::make_query("SELECT $1::integer", 42)
        );
    }

    SHOULD("for query conf with two queries returns appropriate queries") {
        const auto repository = ozo::make_query_repository<
            ozo::testing::query_without_parameters,
            ozo::testing::query_with_one_parameter
        >(
            "-- name: query without parameters\n"
            "SELECT 1\n"
            "-- name: query with one parameter\n"
            "SELECT :parameter::integer"
        );
        EXPECT_EQ(
            repository.make_query<ozo::testing::query_without_parameters>(),
            ozo::make_query("SELECT 1")
        );
        EXPECT_EQ(
            repository.make_query<ozo::testing::query_with_one_parameter>(42),
            ozo::make_query("SELECT $1::integer", 42)
        );
    }

//    SHOULD("for query conf with one query with struct parameters returns appropriate query") {
//        const auto repository = ozo::make_query_repository<ozo::testing::query_with_struct_parameters>(
//            "-- name: query with struct parameters\n"
//            "SELECT :string::text || :number::text"
//        );
//        EXPECT_EQ(
//            repository.make_query<ozo::testing::query_with_struct_parameters>(ozo::testing::struct_parameters {"42", 13}),
//            ozo::make_query("SELECT $1::text || $2::text", std::string("42"), 13)
//        );
//    }

//    SHOULD("for query conf with one query with struct parameters with different fields order returns appropriate query") {
//        const auto repository = ozo::make_query_repository<ozo::testing::query_with_struct_parameters>(
//            "-- name: query with struct parameters\n"
//            "SELECT :number::text || :string::text"
//        );
//        EXPECT_EQ(
//            repository.make_query<ozo::testing::query_with_struct_parameters>(ozo::testing::struct_parameters {"42", 13}),
//            ozo::make_query("SELECT $1::text || $2::text", 13, std::string("42"))
//        );
//    }
}
