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
    static std::size_t& times_called() { static std::size_t t; return t; }
    static ozo::error_code& ec() { static ozo::error_code e; return e; }

    template <typename TypeMap, typename T>
    inline ozo::error_code operator()(ozo::oid_t, const char*, std::size_t, const TypeMap&, T&)
    {
        ++ times_called();
        return ec();
    }
};