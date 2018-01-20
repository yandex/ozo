#pragma once

#include <boost/system/error_code.hpp>

namespace apq {

// TODO: it's a stub, we haven't decided what to use yet
using boost::system::error_code;

enum errc
{
    success = 0,
    type_mismatch,
    size_mismatch,
    index_out_of_range
};

namespace {

struct error_category : boost::system::error_category
{
    const char* name() const noexcept override
    {
        return "apq::error_category";
    }

    std::string message(int ev) const override
    {
        switch (ev)
        {
        case success: return "success";
        case type_mismatch: return "type_mismatch";
        case size_mismatch: return "size_mismatch";
        case index_out_of_range: return "index out of range";
        default: return "unknown";
        }
    }
};

const error_category error_category_ {};

} // namespace

} // namespace apq

namespace boost {
namespace system {
  template <>
    struct is_error_code_enum<apq::errc> : std::true_type {};
}
}

namespace apq {

inline error_code make_error_code(errc e)
{
    return error_code(e, error_category_);
}

} // namespace apq
