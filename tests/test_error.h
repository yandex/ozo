#pragma once

#include <ozo/error.h>

#include <boost/asio/steady_timer.hpp>

#include <sstream>

namespace ozo::tests::error {

enum code {
    ok, // to do no use error code 0
    error, // error
};

namespace detail {

class category : public error_category {
public:
    const char* name() const noexcept override { return "ozo::tests::error::detail::category"; }

    std::string message(int value) const override {
        switch (code(value)) {
            case ok:
                return "no error";
            case error:
                return "test error";
        }
        std::ostringstream error;
        error << "no message for value: " << value;
        return error.str();
    }
};

} // namespace detail

inline const error_category& get_category() {
    static detail::category instance;
    return instance;
}

} // namespace ozo::tests::error

namespace boost::system {

template <>
struct is_error_code_enum<ozo::tests::error::code> : std::true_type {};

} // namespace boost::system

namespace ozo::tests::error {

inline auto make_error_code(const code e) {
    return boost::system::error_code(static_cast<int>(e), get_category());
}

} // namespace ozo::tests::error
