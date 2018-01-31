#pragma once

#include "ozo/error.h"
#include "ozo/type_traits.h"

#include <boost/hana/map.hpp>
#include <boost/lexical_cast.hpp>

struct mock_pg_value
{
    ozo::oid_t oid_;
    std::string data_;

    ozo::oid_t oid() const { return oid_; }
    const char* bytes() const { return data_.data(); }
    std::size_t size() const { return data_.size(); }
};

template <std::size_t Length>
using mock_pg_row = std::array<mock_pg_value, Length>;

struct mock_pg_converter
{
    std::size_t times_called = 0;
    ozo::error_code ec;

    template <typename T>
    inline ozo::error_code operator()(ozo::oid_t, const char* bytes, std::size_t size, T& value)
    {
        ++ times_called;
        value = boost::lexical_cast<T>(std::string(bytes, size));
        return ec;
    }
};