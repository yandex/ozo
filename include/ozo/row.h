#pragma once

#include "ozo/value.h"
#include "ozo/error.h"

#include <boost/fusion/adapted.hpp>
#include <boost/fusion/sequence.hpp>
#include <boost/fusion/support/is_sequence.hpp>
#include <boost/fusion/include/is_sequence.hpp>

namespace ozo {

template <typename RowData, typename ValueConverter>
class basic_row
{
public:
    basic_row() = default;
    basic_row(const RowData& row_data, ValueConverter converter = ValueConverter{})
        : row_data_(row_data), value_converter_(converter) {}
    basic_row(RowData&& row_data, ValueConverter converter = ValueConverter{})
        : row_data_(std::move(row_data)), value_converter_(converter) {}

    template <typename T>
    error_code at(std::size_t i, T& value)
    {
        if (i >= row_data_.size()) {
            return error::row_index_out_of_range;
        }

        const auto& value_data = row_data_.at(i);

        return value_converter_(
            value_data.oid(),
            value_data.bytes(),
            value_data.size(),
            value
        );
    }

private:
    RowData row_data_;
    ValueConverter value_converter_;
};

namespace detail {

template <typename TypeMap>
struct pg_value_converter
{
    const TypeMap& type_map;

    template <typename T>
    inline error_code operator()(oid_t oid, const char* bytes, std::size_t size, T& value)
    {
        return convert_value(oid, bytes, size, type_map, value);
    }
};

template <typename RowData, typename Row, typename ValueConverter, typename Enable = void>
struct row_converter
{
    error_code operator()(RowData&& row_data, Row& row, ValueConverter value_converter = ValueConverter{});
};

template <typename RowData, typename Row, typename ValueConverter>
struct row_converter<RowData, Row, ValueConverter,
    typename std::enable_if<
        is_iterable<RowData>::value && boost::fusion::traits::is_sequence<Row>::value
    >::type
>
{
    error_code operator()(RowData&& row_data, Row& row, ValueConverter value_converter = ValueConverter{})
    {
        error_code ec = error::ok;
        auto data_it = begin(row_data);
        auto data_end = end(row_data);

        boost::fusion::for_each(row, [&](auto& value) {
            if (ec) {
                return;
            }

            if (data_it == data_end) {
                ec = error::row_type_mismatch;
                return;
            }

            const auto& value_data = *data_it;

            ec = value_converter(
                value_data.oid(),
                value_data.bytes(),
                value_data.size(),
                value
            );
            ++data_it;
        });
        if (!ec && data_it != data_end) {
            ec = error::row_type_mismatch;
        }
        return ec;
    }
};

} // namespace detail

template <typename RowData>
using row = basic_row<RowData, detail::pg_value_converter<decltype(ozo::register_types<>())>>;

template <typename RowData, typename Row,
    typename ValueConverter = detail::pg_value_converter<decltype(ozo::register_types<>())>>
inline error_code convert_row(RowData&& row_data, Row& row,
    const ValueConverter& value_converter = ValueConverter{})
{
    return detail::row_converter<RowData, Row, ValueConverter>{}(std::forward<RowData>(row_data), row, value_converter);
}

} // namespace ozo