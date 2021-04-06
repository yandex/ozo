#pragma once

#include <ozo/pg/definitions.h>
#include <ozo/io/send.h>
#include <ozo/io/recv.h>

#include <string>

namespace ozo::pg {

class ltree {
    friend send_impl<ltree>;
    friend recv_impl<ltree>;
    friend size_of_impl<ltree>;

public:
    ltree() = default;

    ltree(std::string raw_string) noexcept
        : value(std::move(raw_string)) {}

    const std::string& raw_string() const & noexcept {
        return value;
    }

    std::string raw_string() && noexcept {
        return std::move(value);
    }

    friend bool operator ==(const ltree& lhs, const ltree& rhs) {
        return lhs.value == rhs.value;
    }

private:
    std::string value;
};

} // namespace ozo::pg

namespace ozo {

template <>
struct size_of_impl<pg::ltree> {
    static auto apply(const pg::ltree& v) noexcept {
        return std::size(v.value) + 1;
    }
};

template <>
struct send_impl<pg::ltree> {
    template <typename OidMap>
    static ostream& apply(ostream& out, const OidMap&, const pg::ltree& in) {
        const std::int8_t version = 1;
        write(out, version);
        return write(out, in.value);
    }
};

template <>
struct recv_impl<pg::ltree> {
    template <typename OidMap>
    static istream& apply(istream& in, size_type size, const OidMap&, pg::ltree& out) {
        if (size < 1) {
            throw std::range_error("data size " + std::to_string(size) + " is too small to read ltree");
        }
        std::int8_t version;
        read(in, version);
        out.value.resize(static_cast<std::size_t>(size - 1));
        return read(in, out.value);
    }
};

} // namespace ozo

OZO_PG_DEFINE_CUSTOM_TYPE(ozo::pg::ltree, "ltree")
