#pragma once

#include <ozo/query.h>

#include <boost/spirit/home/x3.hpp>
#include <boost/hana/concat.hpp>
#include <boost/hana/members.hpp>
#include <boost/hana/core/to.hpp>

#include <memory>
#include <string_view>
#include <string>
#include <unordered_map>

namespace ozo {
namespace detail {

struct query_description {
    std::string name;
    std::string text;
    std::unordered_map<std::string, std::size_t> parameters;
};

namespace parser {

namespace x3 = boost::spirit::x3;

using x3::blank;
using x3::lit;
using x3::char_;
using x3::eol;
using x3::lexeme;
using x3::string;

x3::rule<class name, std::string> name("name");
x3::rule<class text_part, std::string> text_part("text_part");
x3::rule<class parameter_name, std::string> parameter_name("parameter_name");

struct text_value {
    std::string text;
    std::unordered_map<std::string, std::size_t> parameters;
};

x3::rule<class text, text_value> text("text");
x3::rule<class query_, query_description> query("query");
x3::rule<class conf, std::vector<query_description>> conf("conf");

const auto on_text_part = [] (const auto& ctx) {
    boost::apply_visitor([&] (const auto& value) {
        x3::_val(ctx) += value;
    }, x3::_attr(ctx));
};

const auto on_text = [] (const auto& ctx) {
    x3::_val(ctx).text += x3::_attr(ctx);
};

const auto on_parameter_name = [] (const auto& ctx) {
    const auto parameter = x3::_val(ctx).parameters.find(x3::_attr(ctx));
    if (parameter == x3::_val(ctx).parameters.end()) {
        x3::_val(ctx).text += "$" + std::to_string(x3::_val(ctx).parameters.size() + 1);
        x3::_val(ctx).parameters.emplace(x3::_attr(ctx), x3::_val(ctx).parameters.size());
    } else {
        x3::_val(ctx).text += "$" + std::to_string(parameter->second + 1);
    }
};

const auto on_name = [] (const auto& ctx) {
    x3::_val(ctx).name = x3::_attr(ctx);
};

const auto on_query_text = [] (const auto& ctx) {
    x3::_val(ctx).text = x3::_attr(ctx).text;
    x3::_val(ctx).parameters = x3::_attr(ctx).parameters;
};

const auto prefix = "--" >> *lit(' ') >> "name" >> *lit(' ') >> ':' >> *lit(' ');
const auto name_def = lexeme[+(char_ - eol)];
const auto text_part_def = *lexeme[string("::") | +(char_ - char_(':') - eol)][on_text_part];
const auto parameter_name_def = lit(':') >> lexeme[+char_("_0-9A-Za-z")];
const auto text_def = text_part[on_text] >> *(parameter_name[on_parameter_name] >> text_part[on_text]);
const auto query_def = name[on_name] >> +eol >> text[on_query_text];
const auto conf_def = -(prefix >> query % (+eol >> prefix));

BOOST_SPIRIT_DEFINE(name, text_part, parameter_name, text, query, conf)

} // namespace parser

template <class InputIteratorT>
std::vector<ozo::detail::query_description> parse_query_conf(InputIteratorT begin, InputIteratorT end) {
    namespace x3 = boost::spirit::x3;
    std::vector<query_description> descriptions;
    if (!x3::phrase_parse(begin, end, parser::conf, x3::blank, descriptions)) {
        throw std::runtime_error("failed to parse query conf");
    }
    return descriptions;
}

template <class InputIteratorRangeT>
std::vector<ozo::detail::query_description> parse_query_conf(const InputIteratorRangeT& range) {
    return parse_query_conf(std::begin(range), std::end(range));
}

struct query_conf {
    std::vector<ozo::detail::query_description> descriptions;
    std::unordered_map<std::string_view, std::string_view> queries;

    query_conf(std::vector<ozo::detail::query_description> descriptions)
            : descriptions(std::move(descriptions)) {}
};

std::shared_ptr<query_conf> make_query_conf(std::vector<ozo::detail::query_description> descriptions) {
    using description_pair = std::unordered_map<std::string_view, std::string_view>::value_type;
    const auto result = std::make_shared<query_conf>(std::move(descriptions));
    std::transform(result->descriptions.cbegin(), result->descriptions.cend(),
        std::inserter(result->queries, result->queries.end()),
        [] (const auto& description) { return description_pair(description.name, description.text); });
    return result;
}

} // namespace detail

template <class QueryT>
constexpr std::string_view get_query_name(const QueryT& = QueryT {}) noexcept {
    return std::string_view(hana::to<const char*>(QueryT::name));
}

template <class ... QueriesT>
class query_repository {
public:
    query_repository(std::shared_ptr<detail::query_conf> query_conf)
        : query_conf(std::move(query_conf)) {}

    template <class QueryT>
    auto make_query() const {
        return ozo::make_query(query_conf->queries.at(get_query_name<QueryT>()).data());
    }

    template <class QueryT, class ... TupleValuesT>
    auto make_query(TupleValuesT ... values) const {
        return ozo::make_query(query_conf->queries.at(get_query_name<QueryT>()).data(), values ...);
    }

    template <class QueryT>
    auto make_query(const typename QueryT::parameters_type& parameters) const {
        const auto& description = query_conf->queries.at(get_query_name<QueryT>());
        return hana::apply(
            [&] (auto text, auto ... values) { return make_query<QueryT>(text, values ...); },
            hana::concat(hana::make_tuple(description.data()), hana::members(parameters))
        );
    }

private:
    std::shared_ptr<detail::query_conf> query_conf;
};

template <class InputIteratorT, class ... QueriesT>
auto make_query_repository(InputIteratorT begin, InputIteratorT end) {
    return query_repository<QueriesT ...>(detail::make_query_conf(detail::parse_query_conf(begin, end)));
}

template <class InputIteratorRangeT, class ... QueriesT>
auto make_query_repository(InputIteratorRangeT range) {
    return make_query_repository<delctype(std::begin(range)), QueriesT ...>(std::begin(range), std::end(range));
}

template <class ... QueriesT>
auto make_query_repository(const std::string_view& value) {
    return make_query_repository<std::string_view::const_iterator, QueriesT ...>(value.begin(), value.end());
}

} // namespace ozo
