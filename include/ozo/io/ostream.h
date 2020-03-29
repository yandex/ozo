#pragma once

#include <ozo/core/concept.h>
#include <ozo/error.h>
#include <ozo/detail/endian.h>
#include <ozo/detail/float.h>
#include <ozo/detail/typed_buffer.h>

#include <boost/hana/for_each.hpp>
#include <boost/hana/members.hpp>
#include <boost/hana/tuple.hpp>

#include <vector>
#include <ostream>

namespace ozo {

class ostream {
public:
    using traits_type = std::ostream::traits_type;
    using char_type = std::ostream::char_type;

    ostream(std::vector<char_type>& buf) : buf_(buf) {}

    ostream& write(const char_type* s, std::streamsize n) {
        buf_.insert(buf_.end(), s, s + n);
        return *this;
    }

    ostream& put(char_type ch) {
        buf_.push_back(ch);
        return *this;
    }

    template <typename T>
    Require<Integral<T> && sizeof(T) == 1, ostream&> write(T in) {
        return put(static_cast<char_type>(in));
    }

    template <typename T>
    Require<RawDataReadable<T>, ostream&> write(const T& in) {
        using std::data;
        using std::size;
        return write(reinterpret_cast<const char_type*>(data(in)), size(in));
    }

    template <typename T>
    Require<Integral<T> && sizeof(T) != 1, ostream&> write(T in) {
        detail::typed_buffer<T> buf;
        buf.typed = detail::convert_to_big_endian(in);
        return write(buf);
    }

    template <typename T>
    Require<FloatingPoint<T>, ostream&> write(T in) {
        return write(detail::to_integral(in));
    }

    ostream& write(bool in) {
        const char_type value = in ? char_type(1) : char_type(0);
        return write(value);
    }

    template <typename T>
    Require<HanaSequence<T>, ostream&> write(const T& in) {
        hana::for_each(in, [&](auto& item) { write(item); });
        return *this;
    }

    template <typename T>
    Require<HanaStruct<T>, ostream&> write(const T& in) {
        return write(hana::members(in));
    }

private:
    std::vector<char_type>& buf_;
};

template <typename ...Ts>
inline ostream& write(ostream& out, Ts&& ...vs) {
    return out.write(std::forward<Ts>(vs)...);
}

} // namespace ozo
