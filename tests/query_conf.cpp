#include <ozo/query_conf.h>

#include <boost/hana/adapt_struct.hpp>
#include <boost/hana/contains.hpp>
#include <boost/hana/string.hpp>
#include <boost/hana/tuple.hpp>

#include <string_view>
#include <cstring>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace ozo {
namespace detail {

static bool operator ==(const query_text_part& lhs, const query_text_part& rhs) {
    return lhs.value == rhs.value;
}

static bool operator ==(const query_parameter_name& lhs, const query_parameter_name& rhs) {
    return lhs.value == rhs.value;
}

static bool operator ==(const parsed_query& lhs, const parsed_query& rhs) {
    return lhs.name == rhs.name && lhs.text == rhs.text;
}

static std::ostream& operator <<(std::ostream& stream, const query_text_part& value) {
    return stream << "query_text_part {\"" << value.value << "\"}";
}

static std::ostream& operator <<(std::ostream& stream, const query_parameter_name& value) {
    return stream << "query_parameter_name {\"" << value.value << "\"}";
}

static std::ostream& operator <<(std::ostream& stream, const query_text_element& value) {
    return boost::apply_visitor([&] (const auto& v) -> std::ostream& { return stream << v; }, value);
}

static std::ostream& operator <<(std::ostream& stream, const parsed_query& value) {
    stream << "parsed_query {\"" << value.name << "\", {";
    boost::for_each(value.text, [&] (const auto& v) { stream << v << ", "; });
    return stream << "}}";
}

static bool operator ==(const query_description& lhs, const query_description& rhs) {
    return lhs.name == rhs.name && lhs.text == rhs.text;
}

static std::ostream& operator <<(std::ostream& stream, const query_description& value) {
    return stream << "query_description {\"" << value.name << "\", \"" << value.text << "\"}";
}

} // namespace detail

namespace impl {

template < class ... ParamsT>
static bool operator ==(const query<ParamsT ...>& lhs, const query<ParamsT ...>& rhs) {
    return lhs.text == rhs.text && lhs.params == rhs.params;
}

} // namespace impl

namespace tests {

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

struct non_default_constructible_struct_parameters {
    std::reference_wrapper<std::string> string;
    std::reference_wrapper<int> number;
};

struct query_with_non_default_constructible_struct_parameters {
    static constexpr auto name = "query with non default-constructible struct parameters"_s;
    using parameters_type = non_default_constructible_struct_parameters;
};

struct query_with_typo_in_name {
    static constexpr auto name = "qeury with typo in name"_s;
    using parameters_type = std::tuple<>;
};

struct prohibit_copy_parameter {
    prohibit_copy_parameter() = default;
    prohibit_copy_parameter(const prohibit_copy_parameter&) = delete;
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

} // namespace tests
} // namespace ozo

BOOST_HANA_ADAPT_STRUCT(
    ozo::tests::struct_parameters,
    string,
    number
);

BOOST_HANA_ADAPT_STRUCT(
    ozo::tests::non_default_constructible_struct_parameters,
    string,
    number
);

BOOST_HANA_ADAPT_STRUCT(
    ozo::tests::prohibit_copy_struct,
    v
);

BOOST_HANA_ADAPT_STRUCT(
    ozo::tests::require_copy_struct,
    v
);

namespace {

using namespace testing;
using namespace ozo::tests;

namespace hana = boost::hana;

using namespace ozo::detail;
using namespace ozo::detail::text_parser;

using qtp = query_text_part;
using qpn = query_parameter_name;

TEST(parse_query_conf, should_for_empty_const_char_array_return_empty_description) {
    EXPECT_THAT(ozo::detail::parse_query_conf(""), ElementsAre());
}

TEST(parse_query_conf, should_for_empty_std_string_view_returns_empty_description) {
    EXPECT_THAT(ozo::detail::parse_query_conf(std::string_view("")), ElementsAre());
}

TEST(parse_query_conf, should_for_empty_std_string_returns_empty_descriptions) {
    EXPECT_THAT(ozo::detail::parse_query_conf(std::string("")), ElementsAre());
}

TEST(parse_query_conf, should_for_empty_iterators_range_return_empty_description) {
    const std::string_view content("");
    EXPECT_THAT(ozo::detail::parse_query_conf(content.begin(), content.end()), ElementsAre());
}

TEST(parse_query_conf, should_for_invalid_input_throw_exception) {
    EXPECT_THROW(
        ozo::detail::parse_query_conf(std::string("foo")),
        std::invalid_argument
    );
}

TEST(parse_query_conf, should_for_one_query_statement_return_one_parsed_query) {
    EXPECT_THAT(
        ozo::detail::parse_query_conf(
            "-- name: query without parameters\n"
            "SELECT 1"
        ),
        ElementsAre(parsed_query {"query without parameters", {qtp {"SELECT 1"}}})
    );
}

TEST(parse_query_conf, should_for_two_query_statements_return_two_parsed_queries) {
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

TEST(parse_query_conf, should_for_two_query_statements_with_multiline_separator_return_two_parsed_queries) {
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

TEST(parse_query_conf, should_for_one_query_statement_with_one_parameter_returns_parsed_query_into_text_parts_and_parameter) {
    EXPECT_THAT(
        ozo::detail::parse_query_conf(
            "-- name: query with one parameter\n"
            "SELECT :0"
        ),
        ElementsAre(parsed_query {"query with one parameter", {qtp {"SELECT "}, qpn {"0"}}})
    );
}

TEST(parse_query_conf, should_support_parameters_name_with_ascii_letters_number_and_underscore) {
    EXPECT_THAT(
        ozo::detail::parse_query_conf(
            "-- name: query with one parameter\n"
            "SELECT :abcXYZ_012"
        ),
        ElementsAre(parsed_query {"query with one parameter", {qtp {"SELECT "}, qpn {"abcXYZ_012"}}})
    );
}

TEST(parse_query_conf, should_for_one_query_statement_with_parameters_return_parsed_query_with_parameters) {
    EXPECT_THAT(
        ozo::detail::parse_query_conf(
            "-- name: query with one parameter\n"
            "SELECT :a + :b"
        ),
        ElementsAre(parsed_query {"query with one parameter", {qtp {"SELECT "}, qpn {"a"}, qtp {" + "}, qpn {"b"}}})
    );
}

TEST(parse_query_conf, should_for_one_query_with_a_parameter_and_explicit_cast_return_parsed_query_with_cast) {
    EXPECT_THAT(
        ozo::detail::parse_query_conf(
            "-- name: query with one parameter\n"
            "SELECT :a::integer"
        ),
        ElementsAre(parsed_query {"query with one parameter", {qtp {"SELECT "}, qpn {"a"}, qtp {"::integer"}}})
    );
}

TEST(parse_query_conf, should_for_query_containing_eol_return_same_text) {
    EXPECT_THAT(
        ozo::detail::parse_query_conf(
            "-- name: query without parameters\n"
            "SELECT\n1"
        ),
        ElementsAre(parsed_query {"query without parameters", {qtp {"SELECT\n"}, qtp {"1"}}})
    );
}

TEST(parse_query_conf, should_for_two_queries_containing_eol_return_same_text) {
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

TEST(parse_query_conf, should_for_comment_in_query_statement_text_return_text_without) {
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

TEST(parse_query_conf, should_support_assignment_operator) {
    EXPECT_THAT(
        ozo::detail::parse_query_conf(
            "-- name: query with one parameter\n"
            "SELECT function(a := :a)"
        ),
        ElementsAre(parsed_query {"query with one parameter", {qtp {"SELECT function(a := "}, qpn {"a"}, qtp {")"}}})
    );
}

TEST(check_for_duplicates, should_not_throw_for_empty_queries) {
    EXPECT_NO_THROW(ozo::detail::check_for_duplicates(hana::tuple<>()));
}

TEST(check_for_duplicates, should_not_throw_for_single_query) {
    EXPECT_NO_THROW(ozo::detail::check_for_duplicates(hana::tuple<query_without_parameters>()));
}

TEST(check_for_duplicates, should_not_throw_for_two_different_queries) {
    const hana::tuple<query_without_parameters, query_without_parameters_2> queries;
    EXPECT_NO_THROW(ozo::detail::check_for_duplicates(queries));
}

TEST(check_for_duplicates, should_throw_for_two_equal_queries) {
    const hana::tuple<query_without_parameters, query_without_parameters> queries;
    EXPECT_THROW(ozo::detail::check_for_duplicates(queries), std::invalid_argument);
}

TEST(check_for_duplicates, should_throw_for_multiple_queries_with_two_equal) {
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

TEST(check_for_duplicates, should_return_empty_set_for_empty_queries) {
    const auto result = ozo::detail::check_for_duplicates({});
    EXPECT_THAT(result, UnorderedElementsAre());
}

TEST(check_for_duplicates, should_return_empty_set_with_query_name_for_one_query) {
    const std::vector<parsed_query> queries({parsed_query {"name", {}}});
    EXPECT_THAT(ozo::detail::check_for_duplicates(queries), UnorderedElementsAre("name"));
}

TEST(check_for_duplicates, should_return_empty_set_with_queries_names_for_two_different_queries) {
    const std::vector<parsed_query> queries({parsed_query {"foo", {}}, parsed_query {"bar", {}}});
    EXPECT_THAT(ozo::detail::check_for_duplicates(queries), UnorderedElementsAre("foo", "bar"));
}

TEST(check_for_duplicates, should_throw_exception_for_two_equal_queries) {
    const std::vector<parsed_query> queries({parsed_query {"foo", {}}, parsed_query {"foo", {}}});
    EXPECT_THROW(ozo::detail::check_for_duplicates(queries), std::invalid_argument);
}

TEST(check_for_duplicates, should_throw_exception_for_multiple_queries_with_two_equal) {
    const std::vector<parsed_query> queries({
        parsed_query {"foo", {}},
        parsed_query {"bar", {}},
        parsed_query {"baz", {}},
        parsed_query {"foo", {}},
    });
    EXPECT_THROW(ozo::detail::check_for_duplicates(queries), std::invalid_argument);
}

TEST(check_for_duplicates, should_not_throw_for_empty_declarations_and_definitions) {
    const hana::tuple<> declarations;
    const std::unordered_set<std::string_view> definitions;
    EXPECT_NO_THROW(ozo::detail::check_for_undefined(declarations, definitions));
}

TEST(check_for_duplicates, should_throw_for_not_empty_declarations_and_empty_definitions) {
    const hana::tuple<query_without_parameters> declarations;
    const std::unordered_set<std::string_view> definitions;
    EXPECT_THROW(ozo::detail::check_for_undefined(declarations, definitions), std::invalid_argument);
}

TEST(check_for_duplicates, should_not_throw_for_empty_declarations_and_not_empty_definitions) {
    const hana::tuple<> declarations;
    std::string name("foo");
    const std::unordered_set<std::string_view> definitions({name});
    EXPECT_NO_THROW(ozo::detail::check_for_undefined(declarations, definitions));
}

TEST(check_for_duplicates, should_not_throw_for_matching_declarations_and_definitions) {
    const hana::tuple<query_without_parameters> declarations;
    std::string name("query without parameters");
    const std::unordered_set<std::string_view> definitions({name});
    EXPECT_NO_THROW(ozo::detail::check_for_undefined(declarations, definitions));
}


TEST(query_part_visitor, should_append_text_as_is_forquery_text_part) {
    query_description result;
    query_part_visitor<query_with_one_parameter> visitor(result);
    visitor(query_text_part {"foo"});
    visitor(query_text_part {"bar"});
    EXPECT_EQ(result.text, "foobar");
}

TEST(query_part_visitor, should_append_libpq_placeholder_for_query_with_tuple_parameters_according_to_order) {
    query_description result;
    query_part_visitor<query_with_one_parameter> visitor(result);
    visitor(query_parameter_name {"0"});
    EXPECT_EQ(result.text, "$1");
}

TEST(query_part_visitor, should_append_libpq_placeholder_for_query_with_struct_parameters_according_to_name) {
    query_description result;
    query_part_visitor<query_with_struct_parameters> visitor(result);
    visitor(query_parameter_name {"number"});
    EXPECT_EQ(result.text, "$2");
}

TEST(query_part_visitor, should_append_libpq_placeholder_for_query_with_non_default_constructible_struct_parameters_according_to_name) {
    query_description result;
    query_part_visitor<query_with_non_default_constructible_struct_parameters> visitor(result);
    visitor(query_parameter_name {"number"});
    EXPECT_EQ(result.text, "$2");
}

TEST(query_part_visitor, should_throw_for_greater_than_maximum_numeric_parameter) {
    query_description result;
    query_part_visitor<query_with_one_parameter> visitor(result);
    EXPECT_THROW(visitor(query_parameter_name {"1"}), std::out_of_range);
}

TEST(query_part_visitor, should_throw_with_not_numeric_parameter_for_query_with_tuple_parameters) {
    query_description result;
    query_part_visitor<query_with_one_parameter> visitor(result);
    EXPECT_THROW(visitor(query_parameter_name {"foo"}), std::invalid_argument);
}

TEST(query_part_visitor, should_throw_with_with_undeclared_named_parameter) {
    query_description result;
    query_part_visitor<query_with_struct_parameters> visitor(result);
    EXPECT_THROW(visitor(query_parameter_name {"foo"}), std::invalid_argument);
}


TEST(make_query_description, should_set_name_and_concat_text_into_string_for_single_query) {
    const query_with_one_parameter query;
    const parsed_query parsed {"query with one parameter", {query_text_part {"SELECT "}, query_parameter_name {"0"}}};
    const auto result = ozo::detail::make_query_description(query, parsed);
    EXPECT_EQ(result.name, "query with one parameter");
    EXPECT_EQ(result.text, "SELECT $1");
}

TEST(make_query_description, should_trim_query_text_for_single_query) {
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

TEST(make_query_description, should_set_name_and_concat_text_into_string_for_multiply_queries) {
    const hana::tuple<query_with_struct_parameters, query_with_one_parameter> queries;
    const parsed_query parsed {"query with one parameter", {query_text_part {"SELECT "}, query_parameter_name {"0"}}};
    const auto result = ozo::detail::make_query_description(queries, parsed);
    EXPECT_EQ(result.name, "query with one parameter");
    EXPECT_EQ(result.text, "SELECT $1");
}

TEST(make_query_description, should_thow_for_parsed_query_name_not_present_in_queries) {
    const hana::tuple<query_with_struct_parameters, query_with_one_parameter> queries;
    const parsed_query parsed {"foo", {}};
    EXPECT_THROW(ozo::detail::make_query_description(queries, parsed), std::invalid_argument);
}


using qtp = query_text_part;
using qpn = query_parameter_name;

TEST(make_query_descriptions, should_set_name_and_concat_text_into_string_for_each_parsed_query) {
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


TEST(make_query_conf, should_return_empty_descriptions_and_querie_for_empty_data) {
    const auto result = ozo::detail::make_query_conf({});
    EXPECT_THAT(result->descriptions, ElementsAre());
    EXPECT_THAT(result->queries, ElementsAre());
}

TEST(make_query_conf, should_return_one_description_and_one_query_for_one_description) {
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

TEST(make_query_conf, should_return_two_descriptions_and_two_queries_for_two_descriptions_with_different_names) {
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


TEST(get_query_name, should_return_std_string_view_of_static_field_name_for_query_type) {
    EXPECT_EQ(ozo::get_query_name<query_without_parameters>(),
              std::string_view("query without parameters"));
}

TEST(make_query_repository, should_return_query_repository_for_empty_query_conf_and_no_types) {
    EXPECT_NO_THROW(ozo::make_query_repository(std::string_view()));
}

TEST(query_repository_make_query, should_return_query_for_query_conf_with_single_query_without_parameters) {
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

TEST(query_repository_make_query, should_return_query_for_query_conf_with_single_query_with_one_parameter) {
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

TEST(query_repository_make_query, should_return_query_for_query_conf_with_single_query_with_one_parameter_passed_in_tuple) {
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

TEST(query_repository_make_query, should_return_query_for_query_conf_with_single_query_with_one_const_reference_parameter) {
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

TEST(query_repository_make_query, should_return_query_for_query_conf_with_two_queries) {
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

TEST(query_repository_make_query, should_return_query_for_query_conf_with_single_query_with_struct_parameters) {
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

TEST(query_repository_make_query, should_return_query_for_query_conf_with_single_query_with_struct_parameters_with_different_fields_order) {
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

TEST(query_repository_make_query, should_return_query_for_struct_parameters_passed_by_const_reference) {
    const auto repository = ozo::make_query_repository(
        "-- name: query with struct parameters\n"
        "SELECT :string::text || :number::text",
        hana::tuple<query_with_struct_parameters>()
    );
    const struct_parameters parameters {std::string_view("42"), 13};
    EXPECT_EQ(
        repository.make_query<query_with_struct_parameters>(static_cast<const struct_parameters&>(parameters)),
        ozo::make_query("SELECT $1::text || $2::text", std::string_view("42"), 13)
    );
}

TEST(query_repository_make_query, should_return_query_for_struct_parameters_passed_by_reference) {
    const auto repository = ozo::make_query_repository(
        "-- name: query with struct parameters\n"
        "SELECT :string::text || :number::text",
        hana::tuple<query_with_struct_parameters>()
    );
    struct_parameters parameters {std::string_view("42"), 13};
    EXPECT_EQ(
        repository.make_query<query_with_struct_parameters>(static_cast<struct_parameters&>(parameters)),
        ozo::make_query("SELECT $1::text || $2::text", std::string_view("42"), 13)
    );
}

TEST(query_repository_make_query, should_not_copy_parameter_passed_by_rvalue_reference) {
    const auto repository = ozo::make_query_repository(
        "-- name: prohibit copy query\n"
        "SELECT 1",
        hana::tuple<prohibit_copy_query>()
    );
    repository.make_query<prohibit_copy_query>(prohibit_copy_parameter {});
}

TEST(query_repository_make_query, should_not_copy_struct_parameters_passed_by_rvalue_reference) {
    const auto repository = ozo::make_query_repository(
        "-- name: prohibit copy struct query\n"
        "SELECT 1",
        hana::tuple<prohibit_copy_struct_query>()
    );
    repository.make_query<prohibit_copy_struct_query>(prohibit_copy_struct {prohibit_copy_parameter {}});
}

TEST(query_repository_make_query, should_copy_parameter_passed_by_const_reference) {
    const auto repository = ozo::make_query_repository(
        "-- name: require copy query\n"
        "SELECT 1",
        hana::tuple<require_copy_query>()
    );
    const require_copy_parameter parameter {};
    repository.make_query<require_copy_query>(parameter);
}

TEST(query_repository_make_query, should_copy_struct_parameters_passed_by_const_reference) {
    const auto repository = ozo::make_query_repository(
        "-- name: require copy struct query\n"
        "SELECT 1",
        hana::tuple<require_copy_struct_query>()
    );
    const require_copy_struct parameters {require_copy_parameter {}};
    repository.make_query<require_copy_struct_query>(parameters);
}

} // namespace
