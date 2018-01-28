#pragma once

#include "ozo/value.h"
#include "ozo/error.h"

namespace ozo {

template <typename RowData, typename ValueConverter, typename TypeMap>
class basic_row
{
public:
    basic_row() = default;
    basic_row(const RowData& row_data) : row_data_(row_data) {}
    basic_row(RowData&& row_data) : row_data_(std::move(row_data)) {}

    template <typename T>
    error_code at(std::size_t i, T& value)
    {
        if (i >= row_data_.size()) {
            return error::row_index_out_of_range;
        }

        const auto& value_data = row_data_.at(i);

        return ValueConverter{}(
            value_data.oid(),
            value_data.bytes(),
            value_data.size(),
            type_map_,
            value
        );
    }

private:
    RowData row_data_;
    TypeMap type_map_;
};

namespace detail {

struct pg_value_converter
{
    template <typename TypeMap, typename T>
    inline error_code operator()(oid_t oid, const char* bytes, std::size_t size, const TypeMap& type_map, T& value)
    {
        return convert_value(oid, bytes, size, type_map, value);
    }
};

} // namespace detail

template <typename RowData, typename TypeMap = decltype(ozo::register_types<>())>
using row = basic_row<RowData, detail::pg_value_converter, TypeMap>;

} // namespace ozo