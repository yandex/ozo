#pragma once

#include "apq/value.h"

namespace apq {

template <typename RowData, typename TypeMap = decltype(libapq::register_types<>())>
class row
{
public:
    row() = default;
    row(const RowData& row_data) : row_data_(row_data) {}
    row(RowData&& row_data) : row_data_(std::forward<RowData>(row_data)) {}


    /**
     * How to handle errors here? We can:
     * - follow Boost and provide throwing/non-throwing versions
     * - ditch throwing version entirely, annoying anybody who may use it
     * - follow CppCoreGuidelines and return a pair or a custom struct
     * - follow CppCoreGuidelines even more and throw a custom exception
     * 
     * For now - will throw an exception
     * */
    template <typename T>
    T at(std::size_t i)
    {
        if (i < 0 || i >= row_data_.size()) {
            throw std::runtime_error("invalid pg row index " + std::to_string(i));
        }

        const auto& value_data = row_data_.at(i);

        T ret;
        convert_value(
            value_data.oid(),
            value_data.bytes(),
            value_data.size(),
            type_map_,
            ret
        );
        return ret;
    }

private:
    RowData row_data_;
    TypeMap type_map_;
};

}