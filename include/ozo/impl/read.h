#pragma once

#include <ozo/concept.h>
#include <ozo/detail/istream.h>
#include <ozo/detail/typed_buffer.h>

namespace ozo::impl {

using detail::istream;

template <typename T>
inline Require<RawDataWritable<T>, istream&> read(istream& in, T& out) {
    using std::data;
    using std::size;
    in.read(data(out), size(out));
    if (!in) {
        throw system_error(error::unexpected_eof);
    }
    return in;
}

template <typename T>
inline Require<Integral<T>, istream&> read(istream& in, T& out) {
    detail::typed_buffer<T> buf;
    read(in, buf);
    out = detail::convert_from_big_endian(buf.typed);
    return in;
}

template <typename T>
inline Require<FloatingPoint<T>, istream&> read(istream& in, T& out) {
    detail::floating_point_integral_t<T> tmp;
    read(in, tmp);
    out = detail::to_floating_point(tmp);
    return in;
}

inline istream& read(istream& in, bool& out) {
    char tmp = 0;
    read(in, tmp);
    out = tmp;
    return in;
}

template <typename T>
inline Require<FusionSequence<T>, istream&> read(istream& in, T& out) {
    fusion::for_each(out, [&in](auto& member) { read(in, member); });
    return in;
}

} // namespace ozo::impl
