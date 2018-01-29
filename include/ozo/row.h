#pragma once

#include "ozo/value.h"
#include "ozo/error.h"

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

} // namespace detail

template <typename RowData>
using row = basic_row<RowData, detail::pg_value_converter<decltype(ozo::register_types<>())>>;

} // namespace ozo