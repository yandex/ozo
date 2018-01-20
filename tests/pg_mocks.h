#pragma once

#include "apq/error_code.h"
#include "apq/type_traits.h"

#include <boost/hana/map.hpp>
#include <boost/lexical_cast.hpp>

struct mock_pg_value
{
    libapq::oid_t oid_;
    std::string data_;

    libapq::oid_t oid() const { return oid_; }
    const char* bytes() const { return data_.data(); }
    std::size_t size() const { return data_.size(); }
};

template <std::size_t Length>
using mock_pg_row = std::array<mock_pg_value, Length>;

struct mock_pg_converter
{
    static std::size_t& times_called() { static std::size_t t; return t; }
    static apq::error_code& ec() { static apq::error_code e; return e; }

    template <typename TypeMap, typename T>
    inline apq::error_code operator()(libapq::oid_t, const char*, std::size_t, const TypeMap&, T&)
    {
        ++ times_called();
        return ec();
    }
};