#pragma once

#include <ozo/core/concept.h>
#include <ozo/detail/endian.h>
#include <ozo/detail/float.h>
#include <ozo/detail/typed_buffer.h>

#include <boost/hana/for_each.hpp>

#include <istream>

namespace ozo {

class istream {
    class istreambuf {
        const char* i_;
        const char* last_;
    public:

        constexpr istreambuf(const char* data, size_t len) noexcept
        : i_(data), last_(data + len) {}

        std::streamsize read(char* buf, std::streamsize n) noexcept {
            auto last = std::min(i_ + n, last_);
            std::copy(i_, last, buf);
            n = std::distance(i_, last);
            i_ = last;
            return n;
        }
    };
public:
    using traits_type = std::istream::traits_type;
    using char_type = std::istream::char_type;

    constexpr istream(const char* data, size_t len) noexcept
    : buf_(data, len) {}

    istream& read(char_type* buf, std::streamsize len) noexcept {
        const std::streamsize nbytes = buf_.read(buf, len);
        if (nbytes != len) {
            unexpected_eof_ = true;
        }
        return *this;
    }

    traits_type::int_type get() noexcept {
        char retval;
        if (!read(&retval, 1)) {
            return traits_type::eof();
        }
        return retval;
    }

    operator bool() const noexcept { return !unexpected_eof_;}

    template <typename T>
    Require<RawDataWritable<T>, istream&> read(T& out) {
        using std::data;
        using std::size;
        return read(reinterpret_cast<char_type*>(data(out)), size(out));
    }

    template <typename T>
    Require<Integral<T> && sizeof(T) == 1, istream&> read(T& out) noexcept {
        out = istream::traits_type::to_char_type(get());
        return *this;
    }

    template <typename T>
    Require<Integral<T> && sizeof(T) != 1, istream&> read(T& out) noexcept {
        detail::typed_buffer<T> buf;
        read(buf.raw);
        out = detail::convert_from_big_endian(buf.typed);
        return *this;
    }

    template <typename T>
    Require<FloatingPoint<T>, istream&> read(T& out) noexcept {
        detail::floating_point_integral_t<T> tmp;
        read(tmp);
        out = detail::to_floating_point(tmp);
        return *this;
    }

    istream& read(bool& out) noexcept {
        char_type tmp;
        read(tmp);
        out = (tmp != 0);
        return *this;
    }

    template <typename T>
    Require<HanaStruct<T>, istream&> read(T& out) {
        hana::for_each(hana::keys(out), [&](auto key) {
            read(hana::at_key(out, key));
        });
        return *this;
    }

private:
    istreambuf buf_;
    bool unexpected_eof_ = false;
};

template <typename ...Ts>
inline istream& read(istream& in, Ts&& ...vs) {
    if (!in.read(std::forward<Ts>(vs)...)) {
        throw system_error(error::unexpected_eof);
    }
    return in;
}

} // namespace ozo
