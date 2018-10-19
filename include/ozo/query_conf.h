#pragma once

#include <ozo/query.h>

#include <boost/algorithm/string/trim.hpp>
#include <boost/fusion/algorithm/iteration/for_each.hpp>
#include <boost/hana/concat.hpp>
#include <boost/hana/core/to.hpp>
#include <boost/hana/for_each.hpp>
#include <boost/hana/members.hpp>
#include <boost/hana/set.hpp>
#include <boost/hana/size.hpp>
#include <boost/lexical_cast/try_lexical_convert.hpp>
#include <boost/range/algorithm/for_each.hpp>
#include <boost/range/algorithm/transform.hpp>
#include <boost/range/numeric.hpp>
#include <boost/spirit/home/x3.hpp>

#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <unordered_set>

namespace ozo {

template <class QueryT>
constexpr auto get_raw_query_name(const QueryT& = QueryT {}) noexcept {
    return QueryT::name;
}

template <class QueryT>
constexpr std::string_view get_query_name(const QueryT& query = QueryT {}) noexcept {
    return std::string_view(hana::to<const char*>(get_raw_query_name(query)));
}

namespace detail {

namespace x3 = boost::spirit::x3;

struct query_line_comment {
    std::string value;
};

struct query_line_text {
    std::string value;
};

using query_line = boost::variant<query_line_comment, query_line_text>;

struct to_string_visitor {
    std::string& result;

    template <class ... T>
    void operator ()(const boost::fusion::deque<T ...>& value) {
        boost::fusion::for_each(value, [&] (const auto& v) { (*this)(v); });
    }

    template <class ... T>
    void operator ()(const boost::variant<T ...>& value) {
        boost::apply_visitor(*this, value);
    }

    void operator ()(const std::string& value) {
        result += value;
    }
};

namespace lines_parser {

using x3::char_;
using x3::eoi;
using x3::eol;
using x3::lexeme;
using x3::string;

inline const x3::rule<class end, std::string> end("end");
inline const x3::rule<class line, query_line> line("line");
inline const x3::rule<class conf, std::vector<query_line>> conf("conf");

inline const auto on_end = [] (const auto& ctx) {
    boost::apply_visitor([&] (const auto& v) { x3::_val(ctx) = v; }, x3::_attr(ctx));
};

inline const auto on_comment = [] (const auto& ctx) {
    std::string value;
    to_string_visitor visitor {value};
    visitor(x3::_attr(ctx));
    x3::_val(ctx) = query_line_comment {std::move(value)};
};

inline const auto on_text = [] (const auto& ctx) {
    std::string value;
    to_string_visitor visitor {value};
    visitor(x3::_attr(ctx));
    x3::_val(ctx) = query_line_text {std::move(value)};
};

inline const auto end_def = lexeme[string("\r\n") | string("\r") | string("\n")][on_end];
inline const auto line_def = lexeme[string("--") >> *(char_ - eol) >> (end | eoi)][on_comment]
    | lexeme[*(char_ - eol) >> end][on_text]
    | (lexeme[+(char_ - eol)][on_text] >> eoi);
inline const auto conf_def = *line >> eoi;

BOOST_SPIRIT_DEFINE(end, line, conf)

} // namespace lines_parser

namespace header_parser {

using x3::char_;
using x3::eoi;
using x3::eol;
using x3::lexeme;
using x3::space;

struct query_header {
    std::string name;
};

inline const x3::rule<class header, query_header> header("header");

inline const auto on_name = [] (const auto& ctx) {
    x3::_val(ctx).name = x3::_attr(ctx);
};

inline const auto header_def = "--" >> *space >> "name" >> *space >> ':' >> *space >> lexeme[+(char_ - eol)][on_name] >> -eol >> eoi;

BOOST_SPIRIT_DEFINE(header)

} // namespace header_parser

struct query_text_part {
    std::string value;
};

struct query_parameter_name {
    std::string value;
};

using query_text_element = boost::variant<query_text_part, query_parameter_name>;

namespace text_parser {

using x3::char_;
using x3::eoi;
using x3::lexeme;
using x3::lit;
using x3::string;

struct text_value {
    std::vector<query_text_element> text;
};

inline const x3::rule<class text_part, std::string> text_part("text_part");
inline const x3::rule<class parameter_name, std::string> parameter_name("parameter_name");
inline const x3::rule<class text, text_value> text("text");

inline const auto on_text_part = [] (const auto& ctx) {
    to_string_visitor visitor {x3::_val(ctx)};
    visitor(x3::_attr(ctx));
};

inline const auto on_text = [] (const auto& ctx) {
    x3::_val(ctx).text.push_back(query_text_part {x3::_attr(ctx)});
};

inline const auto on_parameter_name = [] (const auto& ctx) {
    x3::_val(ctx).text.push_back(query_parameter_name {x3::_attr(ctx)});
};

inline const auto text_part_def = +lexeme[string("::") | string(":=") | +(char_ - char_(':') - char_('\0'))][on_text_part];
inline const auto parameter_name_def = lit(':') >> lexeme[+char_("_0-9A-Za-z")];
inline const auto text_def = *(parameter_name[on_parameter_name] | text_part[on_text]) >> eoi;

BOOST_SPIRIT_DEFINE(text_part, parameter_name, text)

} // namespace text_parser

template <class T>
constexpr auto HasMembers = hana::Struct<typename hana::tag_of<T>::type>::value;

struct parsed_query {
    std::string name;
    std::vector<query_text_element> text;
};

enum class state_type {
    initial,
    header_parsed,
};

class line_visitor {
public:
    void operator ()(const query_line_comment& comment) {
        switch (state) {
            case state_type::initial:
            case state_type::header_parsed:
                parse_header(comment);
                break;
        }
    }

    void operator ()(const query_line_text& text) {
        switch (state) {
            case state_type::initial:
                throw std::invalid_argument("Failed to parse query conf: expected comment");
            case state_type::header_parsed:
                parse_text(text);
                break;
        }
    }

    std::vector<parsed_query>& get_queries() {
        return queries;
    }

private:
    state_type state = state_type::initial;
    std::string text_value;
    std::vector<parsed_query> queries;

    void parse_header(const query_line_comment& comment) {
        header_parser::query_header header;
        if (x3::phrase_parse(comment.value.begin(), comment.value.end(), header_parser::header, x3::char_('\0'), header)) {
            state = state_type::header_parsed;
            queries.push_back(parsed_query {std::move(header.name), {}});
        }
    }

    void parse_text(const query_line_text& text) {
        text_parser::text_value parsed_text;
        if (!x3::phrase_parse(text.value.begin(), text.value.end(), text_parser::text, x3::lit('\0'), parsed_text)) {
            throw std::invalid_argument("Failed to parse query text: " + queries.back().name);
        }
        queries.back().text.insert(queries.back().text.end(), std::make_move_iterator(parsed_text.text.begin()),
                                   std::make_move_iterator(parsed_text.text.end()));
        text_value.clear();
    }
};

template <class ForwardIteratorT>
std::vector<query_line> parse_query_conf_lines(const ForwardIteratorT begin, const ForwardIteratorT end) {
    std::vector<query_line> result;
    if (!x3::phrase_parse(begin, end, lines_parser::conf, x3::char_('\0'), result)) {
        throw std::invalid_argument("Failed to parse query conf lines");
    }
    return result;
}

inline std::vector<parsed_query> parse_query_conf_elements(const std::vector<query_line>& lines) {
    line_visitor visitor;
    boost::for_each(lines, [&] (const auto& line) { boost::apply_visitor(visitor, line); });
    return std::move(visitor.get_queries());
}

template <class ForwardIteratorT>
std::vector<parsed_query> parse_query_conf(const ForwardIteratorT begin, const ForwardIteratorT end) {
    return parse_query_conf_elements(parse_query_conf_lines(begin, end));
}

template <class ForwardIteratorRangeT>
std::vector<parsed_query> parse_query_conf(const ForwardIteratorRangeT& range) {
    return parse_query_conf(std::begin(range), std::end(range));
}

template <class ... QueriesT>
void check_for_duplicates(const hana::tuple<QueriesT ...>& queries) {
    hana::fold_left(
        queries, hana::make_set(),
        [] (auto set, auto query) {
            const auto result = hana::insert(set, query.name);
            if (set == result) {
                throw std::invalid_argument(hana::to<const char*>("Duplicate declaration for query: "_s + query.name));
            }
            return result;
        }
    );
}

inline std::unordered_set<std::string_view> check_for_duplicates(const std::vector<parsed_query>& descriptions) {
    std::unordered_set<std::string_view> names;
    boost::for_each(descriptions, [&] (const auto& description) {
        if (!names.insert(std::string_view(description.name)).second) {
            throw std::invalid_argument("Duplicate definition for query: " + description.name);
        }
    });
    return names;
}

template <class ... QueriesT>
void check_for_undefined(const hana::tuple<QueriesT ...>& declarations, const std::unordered_set<std::string_view>& definitions) {
    hana::for_each(declarations, [&] (const auto& query) {
        if (!definitions.count(std::string_view(hana::to<const char*>(query.name), hana::size(query.name)))) {
            throw std::invalid_argument(hana::to<const char*>("Query is not defined in query conf: "_s + query.name));
        }
    });
}

struct query_description {
    std::string name;
    std::string text;
};

template <class QueryT>
class query_part_visitor {
public:
    query_part_visitor(detail::query_description& query_description)
        : query_description(query_description) {}

    void operator ()(const query_text_part& value) {
        query_description.text += value.value;
    }

    void operator ()(const query_parameter_name& value) {
        using parameters_type = typename QueryT::parameters_type;
        std::size_t number = 0;
        if constexpr (HasMembers<parameters_type>) {
            bool found = false;
            hana::for_each(hana::transform(hana::accessors<parameters_type>(), hana::first), [&] (const auto& key) {
                if (std::string_view(hana::to<const char*>(key), hana::size(key)) == value.value) {
                    found = true;
                } else if (!found) {
                    ++number;
                }
            });
            if (!found) {
                throw std::invalid_argument(
                    hana::to<const char*>("Parameter is not found in query \""_s + QueryT::name + "\": "_s)
                    + value.value
                );
            }
        } else {
            if (!boost::conversion::try_lexical_convert(value.value, number)) {
                throw std::invalid_argument(
                    hana::to<const char*>("Only valid numeric names supported for not adapted query parameters types,"_s
                                          " but query \""_s + QueryT::name + "\" has parameter with name: "_s)
                    + value.value
                );
            }
            if (number >= std::tuple_size_v<parameters_type>) {
                throw std::out_of_range(
                    hana::to<const char*>("Query has numeric parameter greater than maximum: "_s)
                    + value.value + " (" + std::to_string(std::tuple_size_v<parameters_type>) + ")"
                );
            }
        }
        query_description.text += "$" + std::to_string(number + 1);
    }

private:
    detail::query_description& query_description;
};

template <class QueryT>
query_description make_query_description(const QueryT& query, const parsed_query& parsed) {
    query_description result;
    result.name = parsed.name;
    boost::for_each(parsed.text, [&] (const auto& part) {
        query_part_visitor<std::decay_t<decltype(query)>> visitor(result);
        boost::apply_visitor(visitor, part);
    });
    boost::trim(result.text);
    return result;
}

template <class ... QueriesT>
ozo::detail::query_description make_query_description(const hana::tuple<QueriesT ...>& queries,
                                                      const parsed_query& parsed) {
    boost::optional<ozo::detail::query_description> opt_query_description;
    hana::for_each(queries, [&] (const auto& query) {
        if (std::string_view(hana::to<const char*>(query.name), hana::size(query.name)) == parsed.name) {
            opt_query_description = make_query_description(query, parsed);
        }
    });
    if (!opt_query_description) {
        throw std::invalid_argument("Query is not declared: " + parsed.name);
    }
    return *opt_query_description;
}

template <class ... QueriesT>
std::vector<ozo::detail::query_description> make_query_descriptions(const hana::tuple<QueriesT ...>& queries,
        const std::vector<parsed_query>& parsed) {
    std::vector<ozo::detail::query_description> result;
    result.reserve(parsed.size());
    boost::transform(parsed, std::back_inserter(result),
        [&] (const auto& v) { return make_query_description(queries, v); });
    return result;
}

struct query_conf {
    std::vector<ozo::detail::query_description> descriptions;
    std::unordered_map<std::string_view, std::string_view> queries;

    query_conf(std::vector<ozo::detail::query_description> descriptions)
            : descriptions(std::move(descriptions)) {}
};

inline std::shared_ptr<query_conf> make_query_conf(std::vector<ozo::detail::query_description> descriptions) {
    using description_pair = std::unordered_map<std::string_view, std::string_view>::value_type;
    const auto result = std::make_shared<query_conf>(std::move(descriptions));
    std::transform(result->descriptions.cbegin(), result->descriptions.cend(),
        std::inserter(result->queries, result->queries.end()),
        [] (const auto& description) { return description_pair(description.name, description.text); });
    return result;
}

} // namespace detail

template <class ... QueriesT>
class query_repository {
public:
    query_repository(std::shared_ptr<detail::query_conf> query_conf)
        : query_conf(std::move(query_conf)) {}

    template <class QueryT>
    auto make_query() const {
        return ozo::make_query(get_description<QueryT>());
    }

    template <class QueryT, class ... ParametersT>
    auto make_query(ParametersT&& ... parameters) const {
        static_assert(std::is_same_v<typename QueryT::parameters_type, std::tuple<std::decay_t<ParametersT> ...>>,
                      "parameters types differ from QueryT::parameters_type");
        return make_query<QueryT>(std::make_tuple(std::forward<ParametersT>(parameters) ...));
    }

    template <class QueryT>
    auto make_query(const typename QueryT::parameters_type& parameters) const {
        const auto description = get_description<QueryT>();
        if constexpr (detail::HasMembers<typename QueryT::parameters_type>) {
            return hana::unpack(
                hana::members(parameters),
                [&] (const auto& ... parameters) { return ozo::make_query(description, parameters ...); }
            );
        } else {
            return std::apply(
                [&] (const auto& ... parameters) { return ozo::make_query(description, parameters ...); },
                parameters
            );
        }
    }

    template <class QueryT>
    auto make_query(typename QueryT::parameters_type&& parameters) const {
        const auto description = get_description<QueryT>();
        if constexpr (detail::HasMembers<typename QueryT::parameters_type>) {
            return hana::unpack(
                hana::members(std::move(parameters)),
                [&] (auto&& ... parameters) { return ozo::make_query(description, std::move(parameters) ...); }
            );
        } else {
            return std::apply(
                [&] (auto&& ... parameters) { return ozo::make_query(description, std::move(parameters) ...); },
                std::move(parameters)
            );
        }
    }

private:
    std::shared_ptr<detail::query_conf> query_conf;

    template <class QueryT>
    decltype(auto) get_description() const {
        return query_conf->queries.at(get_query_name(std::get<QueryT>(std::tuple<QueriesT ...>())));
    }
};

template <class ForwardIteratorT, class ... QueriesT>
auto make_query_repository(ForwardIteratorT begin, ForwardIteratorT end,
                           const hana::tuple<QueriesT ...>& queries = hana::tuple<QueriesT ...>()) {
    detail::check_for_duplicates(queries);
    const auto parsed = detail::parse_query_conf(begin, end);
    detail::check_for_undefined(queries, detail::check_for_duplicates(parsed));
    return query_repository<QueriesT ...>(detail::make_query_conf(detail::make_query_descriptions(queries, parsed)));
}

template <class ForwardIteratorRangeT, class ... QueriesT>
auto make_query_repository(const ForwardIteratorRangeT& range,
                           const hana::tuple<QueriesT ...>& queries = hana::tuple<QueriesT ...>()) {
    return make_query_repository(std::begin(range), std::end(range), queries);
}

} // namespace ozo
