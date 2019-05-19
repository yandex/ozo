#pragma once

#include <ozo/core/concept.h>
#include <ozo/error.h>
#include <ozo/detail/ostream.h>
#include <ozo/detail/endian.h>
#include <ozo/detail/float.h>
#include <ozo/detail/typed_buffer.h>
#include <boost/hana/for_each.hpp>
#include <boost/hana/members.hpp>
#include <boost/hana/tuple.hpp>

namespace ozo::impl {

using detail::ostream;

template <typename T>
inline Require<Integral<T> && sizeof(T) == 1, ostream&>
write(ostream& out, T in) {
    out.put(static_cast<char>(in));
    if (!out) {
        throw system_error(error::unexpected_eof);
    }
    return out;
}

template <typename T>
inline Require<RawDataReadable<T>, ostream&> write(ostream& out, const T& in) {
    using std::data;
    using std::size;
    out.write(data(in), size(in));
    if (!out) {
        throw system_error(error::unexpected_eof);
    }
    return out;
}

template <typename T>
inline Require<Integral<T> && sizeof(T) != 1, ostream&> write(ostream& out, T in) {
    detail::typed_buffer<T> buf;
    buf.typed = detail::convert_to_big_endian(in);
    return write(out, buf);
}

template <typename T>
inline Require<FloatingPoint<T>, ostream&> write(ostream& out, T in) {
    return write(out, detail::to_integral(in));
}

inline ostream& write(ostream& out, bool in) {
    return write(out, (in ? char(1) : char(0)));
}

template <typename T>
inline Require<FusionSequence<T> && !HanaSequence<T> && !HanaStruct<T>, ostream&>
write(ostream& out, const T& in) {
    fusion::for_each(in, [&out](auto& member) { write(out, member); });
    return out;
}

template <typename T>
inline Require<HanaSequence<T>, ostream&> write(ostream& out, const T& in) {
    hana::for_each(in, [&out](auto& item) { write(out, item); });
    return out;
}

template <typename T>
inline Require<HanaStruct<T>, ostream&> write(ostream& out, const T& in) {
    return write(out, hana::members(in));
}

} // namespace ozo::impl

