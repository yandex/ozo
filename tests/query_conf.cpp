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

bool operator ==(const query_text_part& lhs, const query_text_part& rhs) {
    return lhs.value == rhs.value;
}

bool operator ==(const query_parameter_name& lhs, const query_parameter_name& rhs) {
    return lhs.value == rhs.value;
}

bool operator ==(const parsed_query& lhs, const parsed_query& rhs) {
    return lhs.name == rhs.name && lhs.text == rhs.text;
}

std::ostream& operator <<(std::ostream& stream, const query_text_part& value) {
    return stream << "query_text_part {\"" << value.value << "\"}";
}

std::ostream& operator <<(std::ostream& stream, const query_parameter_name& value) {
    return stream << "query_parameter_name {\"" << value.value << "\"}";
}

std::ostream& operator <<(std::ostream& stream, const query_text_element& value) {
    return boost::apply_visitor([&] (const auto& v) -> std::ostream& { return stream << v; }, value);
}

std::ostream& operator <<(std::ostream& stream, const parsed_query& value) {
    stream << "parsed_query {\"" << value.name << "\", {";
    boost::for_each(value.text, [&] (const auto& v) { stream << v << ", "; });
    return stream << "}}";
}

bool operator ==(const query_description& lhs, const query_description& rhs) {
    return lhs.name == rhs.name && lhs.text == rhs.text;
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

struct query_without_parameters_2 {
    static constexpr auto name = "query without parameters 2"_s;
    using parameters_type = std::tuple<>;
};

struct query_with_one_parameter {
    static constexpr auto name = "query with one parameter"_s;
    using parameters_type = std::tuple<std::int32_t>;
};

struct struct_parameters {
    std::string_view string;
    int number;
};

struct query_with_struct_parameters {
    static constexpr auto name = "query with struct parameters"_s;
    using parameters_type = struct_parameters;
};

struct query_with_typo_in_name {
    static constexpr auto name = "qeury with typo in name"_s;
    using parameters_type = std::tuple<>;
};

struct prohibit_copy_parameter {
    prohibit_copy_parameter() = default;

    prohibit_copy_parameter(const prohibit_copy_parameter&) {
        EXPECT_TRUE(false);
    }

    prohibit_copy_parameter(prohibit_copy_parameter&& other) = default;
};

struct prohibit_copy_query {
    static constexpr auto name = "prohibit copy query"_s;
    using parameters_type = std::tuple<prohibit_copy_parameter>;
};

struct prohibit_copy_struct {
    prohibit_copy_parameter v;
};

struct prohibit_copy_struct_query {
    static constexpr auto name = "prohibit copy struct query"_s;
    using parameters_type = prohibit_copy_struct;
};

struct require_copy_parameter {
    std::shared_ptr<bool> copied = std::make_shared<bool>(false);

    require_copy_parameter() = default;

    ~require_copy_parameter() {
        EXPECT_TRUE(*copied);
    }

    require_copy_parameter(const require_copy_parameter& other) : copied(other.copied) {
        *copied = true;
    }

    require_copy_parameter(require_copy_parameter&& other) : copied(other.copied) {}
};

struct require_copy_query {
    static constexpr auto name = "require copy query"_s;
    using parameters_type = std::tuple<require_copy_parameter>;
};

struct require_copy_struct {
    require_copy_parameter v;
};

struct require_copy_struct_query {
    static constexpr auto name = "require copy struct query"_s;
    using parameters_type = require_copy_struct;
};

} // namespace testing
} // namespace ozo

BOOST_HANA_ADAPT_STRUCT(
    ozo::testing::struct_parameters,
    string,
    number
);

BOOST_HANA_ADAPT_STRUCT(
    ozo::testing::prohibit_copy_struct,
    v
);

BOOST_HANA_ADAPT_STRUCT(
    ozo::testing::require_copy_struct,
    v
);

GTEST("ozo::detail::parse_query_conf()") {
    using namespace testing;
    using namespace ozo::testing;
    using namespace ozo::detail;
    using namespace ozo::detail::text_parser;

    using qtp = query_text_part;
    using qpn = query_parameter_name;

    namespace hana = boost::hana;

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

    SHOULD("for invalid input throws exception") {
        EXPECT_THROW(
            ozo::detail::parse_query_conf(std::string("foo")),
            std::invalid_argument
        );
    }

    SHOULD("for one query returns one parsed query") {
        EXPECT_THAT(
            ozo::detail::parse_query_conf(
                "-- name: query without parameters\n"
                "SELECT 1"
            ),
            ElementsAre(parsed_query {"query without parameters", {qtp {"SELECT 1"}}})
        );
    }

    SHOULD("for two queries returns two parsed query") {
        EXPECT_THAT(
            ozo::detail::parse_query_conf(
                "-- name: query without parameters\n"
                "SELECT 1\n"
                "-- name: query without parameters 2\n"
                "SELECT 2"
            ),
            ElementsAre(
                parsed_query {"query without parameters", {qtp {"SELECT 1\n"}}},
                parsed_query {"query without parameters 2", {qtp {"SELECT 2"}}}
            )
        );
    }

    SHOULD("for two queries with multiline separator returns two parsed query") {
        EXPECT_THAT(
            ozo::detail::parse_query_conf(
                "-- name: query without parameters\n"
                "SELECT 1\n\n\n"
                "-- name: query without parameters 2\n"
                "SELECT 2"
            ),
            ElementsAre(
                parsed_query {"query without parameters", {qtp {"SELECT 1\n"}, qtp {"\n"}, qtp {"\n"}}},
                parsed_query {"query without parameters 2", {qtp {"SELECT 2"}}}
            )
        );
    }

    SHOULD("for one query with one parameter returns parsed query into text parts and parameter") {
        EXPECT_THAT(
            ozo::detail::parse_query_conf(
                "-- name: query with one parameter\n"
                "SELECT :0"
            ),
            ElementsAre(parsed_query {"query with one parameter", {qtp {"SELECT "}, qpn {"0"}}})
        );
    }

    SHOULD("support parameters name with ascii letters, number and underscore") {
        EXPECT_THAT(
            ozo::detail::parse_query_conf(
                "-- name: query with one parameter\n"
                "SELECT :abcXYZ_012"
            ),
            ElementsAre(parsed_query {"query with one parameter", {qtp {"SELECT "}, qpn {"abcXYZ_012"}}})
        );
    }

    SHOULD("for one query with one parameter returns parsed query with all parameters") {
        EXPECT_THAT(
            ozo::detail::parse_query_conf(
                "-- name: query with one parameter\n"
                "SELECT :a + :b"
            ),
            ElementsAre(parsed_query {"query with one parameter", {qtp {"SELECT "}, qpn {"a"}, qtp {" + "}, qpn {"b"}}})
        );
    }

    SHOULD("for one query with one parameter and explicit cast returns parsed query with cast") {
        EXPECT_THAT(
            ozo::detail::parse_query_conf(
                "-- name: query with one parameter\n"
                "SELECT :a::integer"
            ),
            ElementsAre(parsed_query {"query with one parameter", {qtp {"SELECT "}, qpn {"a"}, qtp {"::integer"}}})
        );
    }

    SHOULD("for query containing eol returns same text") {
        EXPECT_THAT(
            ozo::detail::parse_query_conf(
                "-- name: query without parameters\n"
                "SELECT\n1"
            ),
            ElementsAre(parsed_query {"query without parameters", {qtp {"SELECT\n"}, qtp {"1"}}})
        );
    }

    SHOULD("for two queries containing eol returns same text") {
        EXPECT_THAT(
            ozo::detail::parse_query_conf(
                "-- name: query without parameters\n"
                "SELECT\n1\n"
                "-- name: query without parameters 2\n"
                "SELECT\n2"
            ),
            ElementsAre(
                parsed_query {"query without parameters", {qtp {"SELECT\n"}, qtp {"1\n"}}},
                parsed_query {"query without parameters 2", {qtp {"SELECT\n"}, qtp {"2"}}}
            )
        );
    }

    SHOULD("for comment in query text returns text without") {
        EXPECT_THAT(
            ozo::detail::parse_query_conf(
                "-- name: query without parameters\n"
                "SELECT\n"
                "-- comment\n"
                "1\n"
            ),
            ElementsAre(
                parsed_query {"query without parameters", {qtp {"SELECT\n"}, qtp {"1\n"}}}
            )
        );
    }
}

GTEST("ozo::detail::check_for_duplicates queries") {
    using namespace testing;
    using namespace ozo::testing;

    namespace hana = boost::hana;

    SHOULD("for empty queries do not throw") {
        EXPECT_NO_THROW(ozo::detail::check_for_duplicates(hana::tuple<>()));
    }

    SHOULD("for one query do not throw") {
        EXPECT_NO_THROW(ozo::detail::check_for_duplicates(hana::tuple<query_without_parameters>()));
    }

    SHOULD("for two different queries do not throw") {
        const hana::tuple<query_without_parameters, query_without_parameters_2> queries;
        EXPECT_NO_THROW(ozo::detail::check_for_duplicates(queries));
    }

    SHOULD("for two equal queries throws exception") {
        const hana::tuple<query_without_parameters, query_without_parameters> queries;
        EXPECT_THROW(ozo::detail::check_for_duplicates(queries), std::invalid_argument);
    }

    SHOULD("for multiple queries with two equal throws exception") {
        const auto queries = hana::tuple<
            query_with_one_parameter,
            query_without_parameters,
            query_without_parameters_2,
            query_with_struct_parameters,
            query_with_typo_in_name,
            query_with_one_parameter
        >();
        EXPECT_THROW(ozo::detail::check_for_duplicates(queries), std::invalid_argument);
    }
}

GTEST("ozo::detail::check_for_duplicates parsed queries") {
    using namespace testing;
    using namespace ozo::testing;
    using namespace ozo::detail;

    namespace hana = boost::hana;

    SHOULD("for empty queries returns empty set") {
        const auto result = ozo::detail::check_for_duplicates({});
        EXPECT_THAT(result, UnorderedElementsAre());
    }

    SHOULD("for one query returns set with query name") {
        const std::vector<parsed_query> queries({parsed_query {"name", {}}});
        EXPECT_THAT(ozo::detail::check_for_duplicates(queries), UnorderedElementsAre("name"));
    }

    SHOULD("for two different queries set with queries names") {
        const std::vector<parsed_query> queries({parsed_query {"foo", {}}, parsed_query {"bar", {}}});
        EXPECT_THAT(ozo::detail::check_for_duplicates(queries), UnorderedElementsAre("foo", "bar"));
    }

    SHOULD("for two equal queries throws exception") {
        const std::vector<parsed_query> queries({parsed_query {"foo", {}}, parsed_query {"foo", {}}});
        EXPECT_THROW(ozo::detail::check_for_duplicates(queries), std::invalid_argument);
    }

    SHOULD("for multiple queries with two equal throws exception") {
        const std::vector<parsed_query> queries({
            parsed_query {"foo", {}},
            parsed_query {"bar", {}},
            parsed_query {"baz", {}},
            parsed_query {"foo", {}},
        });
        EXPECT_THROW(ozo::detail::check_for_duplicates(queries), std::invalid_argument);
    }
}

GTEST("ozo::detail::check_for_undefined") {
    using namespace testing;
    using namespace ozo::testing;
    using namespace ozo::detail;

    namespace hana = boost::hana;

    SHOULD("for empty declarations and definitions do not throw") {
        const hana::tuple<> declarations;
        const std::unordered_set<std::string_view> definitions;
        EXPECT_NO_THROW(ozo::detail::check_for_undefined(declarations, definitions));
    }

    SHOULD("for not empty declarations and empty definitions throws exception") {
        const hana::tuple<query_without_parameters> declarations;
        const std::unordered_set<std::string_view> definitions;
        EXPECT_THROW(ozo::detail::check_for_undefined(declarations, definitions), std::invalid_argument);
    }

    SHOULD("for empty declarations and not empty definitions do not throw") {
        const hana::tuple<> declarations;
        std::string name("foo");
        const std::unordered_set<std::string_view> definitions({name});
        EXPECT_NO_THROW(ozo::detail::check_for_undefined(declarations, definitions));
    }

    SHOULD("for matching declarations and definitions do not throw") {
        const hana::tuple<query_without_parameters> declarations;
        std::string name("query without parameters");
        const std::unordered_set<std::string_view> definitions({name});
        EXPECT_NO_THROW(ozo::detail::check_for_undefined(declarations, definitions));
    }
}

GTEST("ozo::detail::query_part_visitor") {
    using namespace testing;
    using namespace ozo::testing;
    using namespace ozo::detail;

    SHOULD("for query_text_part append text as is") {
        query_description result;
        query_part_visitor<query_with_one_parameter> visitor(result);
        visitor(query_text_part {"foo"});
        visitor(query_text_part {"bar"});
        EXPECT_EQ(result.text, "foobar");
    }

    SHOULD("for query_parameter_name append libpq placeholder for parameter according to order") {
        query_description result;
        query_part_visitor<query_with_one_parameter> visitor(result);
        visitor(query_parameter_name {"0"});
        EXPECT_EQ(result.text, "$1");
    }

    SHOULD("for query_parameter_name append converted from name libpq placeholder for parameter according to order") {
        query_description result;
        query_part_visitor<query_with_struct_parameters> visitor(result);
        visitor(query_parameter_name {"number"});
        EXPECT_EQ(result.text, "$2");
    }

    SHOULD("for query_parameter_name with greater than maximum numeric parameter throws exception") {
        query_description result;
        query_part_visitor<query_with_one_parameter> visitor(result);
        EXPECT_THROW(visitor(query_parameter_name {"1"}), std::out_of_range);
    }

    SHOULD("for query_parameter_name not numeric parameter for query with tuple parameters throws exception") {
        query_description result;
        query_part_visitor<query_with_one_parameter> visitor(result);
        EXPECT_THROW(visitor(query_parameter_name {"foo"}), std::invalid_argument);
    }

    SHOULD("for query_parameter_name with undeclared named parameter throws exception") {
        query_description result;
        query_part_visitor<query_with_struct_parameters> visitor(result);
        EXPECT_THROW(visitor(query_parameter_name {"foo"}), std::invalid_argument);
    }
}

GTEST("ozo::detail::make_query_description for single query") {
    using namespace testing;
    using namespace ozo::testing;
    using namespace ozo::detail;

    SHOULD("set name and concat text into string") {
        const query_with_one_parameter query;
        const parsed_query parsed {"query with one parameter", {query_text_part {"SELECT "}, query_parameter_name {"0"}}};
        const auto result = ozo::detail::make_query_description(query, parsed);
        EXPECT_EQ(result.name, "query with one parameter");
        EXPECT_EQ(result.text, "SELECT $1");
    }

    SHOULD("trim query text") {
        const query_with_one_parameter query;
        const parsed_query parsed {"query with one parameter", {
            query_text_part {"\t \n"},
            query_text_part {"SELECT "},
            query_parameter_name {"0"},
            query_text_part {"\t \n"}},
        };
        const auto result = ozo::detail::make_query_description(query, parsed);
        EXPECT_EQ(result.name, "query with one parameter");
        EXPECT_EQ(result.text, "SELECT $1");
    }
}

GTEST("ozo::detail::make_query_description for multiple queries") {
    using namespace testing;
    using namespace ozo::testing;
    using namespace ozo::detail;

    namespace hana = boost::hana;

    SHOULD("set name and concat text into string") {
        const hana::tuple<query_with_struct_parameters, query_with_one_parameter> queries;
        const parsed_query parsed {"query with one parameter", {query_text_part {"SELECT "}, query_parameter_name {"0"}}};
        const auto result = ozo::detail::make_query_description(queries, parsed);
        EXPECT_EQ(result.name, "query with one parameter");
        EXPECT_EQ(result.text, "SELECT $1");
    }

    SHOULD("for parsed query name not present in queries throws exception") {
        const hana::tuple<query_with_struct_parameters, query_with_one_parameter> queries;
        const parsed_query parsed {"foo", {}};
        EXPECT_THROW(ozo::detail::make_query_description(queries, parsed), std::invalid_argument);
    }
}

GTEST("ozo::detail::make_query_descriptions") {
    using namespace testing;
    using namespace ozo::testing;
    using namespace ozo::detail;

    namespace hana = boost::hana;

    using qtp = query_text_part;
    using qpn = query_parameter_name;

    SHOULD("set name and concat text into string for each parsed query") {
        const hana::tuple<query_with_struct_parameters, query_with_one_parameter> queries;
        const std::vector<parsed_query> parsed({
            parsed_query {"query with one parameter", {qtp {"SELECT "}, qpn {"0"}}},
            parsed_query {"query with struct parameters", {qtp {"SELECT "}, qpn {"string"}, qtp {", "}, qpn {"number"}}},
        });
        const auto result = ozo::detail::make_query_descriptions(queries, parsed);
        EXPECT_THAT(result, ElementsAre(
            query_description {"query with one parameter", "SELECT $1"},
            query_description {"query with struct parameters", "SELECT $1, $2"}
        ));
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
            ozo::detail::query_description {"query without parameters", "SELECT 1"},
        });
        EXPECT_THAT(
            result->descriptions,
            ElementsAre(ozo::detail::query_description {"query without parameters", "SELECT 1"})
        );
        EXPECT_THAT(
            result->queries,
            ElementsAre(std::make_pair(std::string_view("query without parameters"), std::string_view("SELECT 1")))
        );
    }

    SHOULD("for two descriptions with different names returns tow descriptions and two queries") {
        const auto result = ozo::detail::make_query_conf({
            ozo::detail::query_description {"query without parameters 1", "SELECT 1"},
            ozo::detail::query_description {"query without parameters 2", "SELECT 2"},
        });
        EXPECT_THAT(
            result->descriptions,
            ElementsAre(
                ozo::detail::query_description {"query without parameters 1", "SELECT 1"},
                ozo::detail::query_description {"query without parameters 2", "SELECT 2"}
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
    EXPECT_EQ(ozo::get_query_name<ozo::testing::query_without_parameters>(),
              std::string_view("query without parameters"));
}

GTEST("ozo::detail::make_query_repository") {
    using namespace testing;

    SHOULD("for empty query conf and no types returns query repository") {
        EXPECT_NO_THROW(ozo::make_query_repository(std::string_view()));
    }
}

GTEST("ozo::detail::query_repository::make_query") {
    using namespace testing;
    using namespace ozo::testing;

    namespace hana = boost::hana;

    SHOULD("for query conf with one query without parameters returns appropriate query") {
        const auto repository = ozo::make_query_repository(
            "-- name: query without parameters\n"
            "SELECT 1",
            hana::tuple<query_without_parameters>()
        );
        EXPECT_EQ(
            repository.make_query<query_without_parameters>(),
            ozo::make_query("SELECT 1")
        );
    }

    SHOULD("for query conf with one query with one parameter returns appropriate query") {
        const auto repository = ozo::make_query_repository(
            "-- name: query with one parameter\n"
            "SELECT :0::integer",
            hana::tuple<query_with_one_parameter>()
        );
        EXPECT_EQ(
            repository.make_query<query_with_one_parameter>(42),
            ozo::make_query("SELECT $1::integer", 42)
        );
    }

    SHOULD("for query conf with one query with one parameter passing in tuple returns appropriate query") {
        using parameters_type = query_with_one_parameter::parameters_type;
        const auto repository = ozo::make_query_repository(
            "-- name: query with one parameter\n"
            "SELECT :0::integer",
            hana::tuple<query_with_one_parameter>()
        );
        EXPECT_EQ(
            repository.make_query<query_with_one_parameter>(parameters_type(42)),
            ozo::make_query("SELECT $1::integer", 42)
        );
    }

    SHOULD("for query conf with one query with one parameter passing in parameters type by const reference returns appropriate query") {
        using parameters_type = query_with_one_parameter::parameters_type;
        const auto repository = ozo::make_query_repository(
            "-- name: query with one parameter\n"
            "SELECT :0::integer",
            hana::tuple<query_with_one_parameter>()
        );
        const parameters_type parameters(42);
        EXPECT_EQ(
            repository.make_query<query_with_one_parameter>(parameters),
            ozo::make_query("SELECT $1::integer", 42)
        );
    }

    SHOULD("for query conf with two queries returns appropriate queries") {
        const auto repository = ozo::make_query_repository(
            "-- name: query without parameters\n"
            "SELECT 1\n"
            "-- name: query with one parameter\n"
            "SELECT :0::integer",
            hana::tuple<query_without_parameters, query_with_one_parameter>()
        );
        EXPECT_EQ(
            repository.make_query<query_without_parameters>(),
            ozo::make_query("SELECT 1")
        );
        EXPECT_EQ(
            repository.make_query<query_with_one_parameter>(42),
            ozo::make_query("SELECT $1::integer", 42)
        );
    }

    SHOULD("for query conf with one query with struct parameters returns appropriate query") {
        const auto repository = ozo::make_query_repository(
            "-- name: query with struct parameters\n"
            "SELECT :string::text || :number::text",
            hana::tuple<query_with_struct_parameters>()
        );
        EXPECT_EQ(
            repository.make_query<query_with_struct_parameters>(
                struct_parameters {std::string_view("42"), 13}
            ),
            ozo::make_query("SELECT $1::text || $2::text", std::string_view("42"), 13)
        );
    }

    SHOULD("for query conf with one query with struct parameters with different fields order returns appropriate query") {
        const auto repository = ozo::make_query_repository(
            "-- name: query with struct parameters\n"
            "SELECT :number::text || :string::text",
            hana::tuple<query_with_struct_parameters>()
        );
        EXPECT_EQ(
            repository.make_query<query_with_struct_parameters>(struct_parameters {"42", 13}),
            ozo::make_query("SELECT $2::text || $1::text", std::string_view("42"), 13)
        );
    }

    SHOULD("for struct parameters passing by const reference returns appropriate query") {
        const auto repository = ozo::make_query_repository(
            "-- name: query with struct parameters\n"
            "SELECT :string::text || :number::text",
            hana::tuple<query_with_struct_parameters>()
        );
        const struct_parameters parameters {std::string_view("42"), 13};
        EXPECT_EQ(
            repository.make_query<query_with_struct_parameters>(parameters),
            ozo::make_query("SELECT $1::text || $2::text", std::string_view("42"), 13)
        );
    }

    SHOULD("do not copy parameter passing by rvalue reference") {
        const auto repository = ozo::make_query_repository(
            "-- name: prohibit copy query\n"
            "SELECT 1",
            hana::tuple<prohibit_copy_query>()
        );
        repository.make_query<prohibit_copy_query>(prohibit_copy_parameter {});
    }

    SHOULD("do not copy struct parameters passing by rvalue reference") {
        const auto repository = ozo::make_query_repository(
            "-- name: prohibit copy struct query\n"
            "SELECT 1",
            hana::tuple<prohibit_copy_struct_query>()
        );
        repository.make_query<prohibit_copy_struct_query>(prohibit_copy_struct {prohibit_copy_parameter {}});
    }

    SHOULD("copy parameter passing by const reference") {
        const auto repository = ozo::make_query_repository(
            "-- name: require copy query\n"
            "SELECT 1",
            hana::tuple<require_copy_query>()
        );
        const require_copy_parameter parameter {};
        repository.make_query<require_copy_query>(parameter);
    }

    SHOULD("copy struct parameters passing by const reference") {
        const auto repository = ozo::make_query_repository(
            "-- name: require copy struct query\n"
            "SELECT 1",
            hana::tuple<require_copy_struct_query>()
        );
        const require_copy_struct parameters {require_copy_parameter {}};
        repository.make_query<require_copy_struct_query>(parameters);
    }
}
