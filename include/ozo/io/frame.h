#pragma once

#include <ozo/type_traits.h>
#include <ozo/result.h>

namespace ozo {

template <typename Parent>
class data_frame {
public:
    using parent_type = Parent;

    data_frame(const Parent& parent, size_type size)
    : parent_{parent}, size_(size) {}

    stream_type& stream() const noexcept { return parent().stream();}
    oid_type oid() const noexcept { return parent().oid();}
    const oid_map_type& oid_map() const noexcept { return parent().oid_map();}
    size_type size() const noexcept { return size_;}

    const parent_type& parent() const noexcept { return parent_;}
    std::string name() const {return parent().name();}

private:
    parent_type& parent_;
    size_type size_;
};

template <typename ParentFrame, typename Out>
inline auto& recv_data_frame(const ParentFrame& parent, Out& out) {
    return recv(data_frame{parent, read<size_type>(parent.stream())}, out);
}

template <typename Stream>
class base_frame {
public:
    using stream_type = Stream;
    base_frame(Stream& stream, const oid_map_type& oid_map, oid_type oid, std::string_view name)

    stream_type& stream() const noexcept { return stream_;}
    oid_type oid() const noexcept { return oid_;}
    size_type size() const noexcept { return size_;}
    const oid_map_type& oid_map() const noexcept { return oid_map_;}

    std::string_view name() const { return name_;}

private:
    stream_type& stream_;
    const oid_map_type& oid_map_;
    oid_type oid_;
    std::string_view name_;
};

template <typename T, typename OidMap, typename Out>
auto recv(const value<T>& in, const OidMap& oids, Out& out) {
    istream s(in.data(), in.size());
    const base_frame frame{s, oids, in.oid(), in.name()};
    const auto size = in.is_null() ? null_state_size : in.size();
    return recv(data_frame{frame, size}, out);
}

template <typename Parent, typename GetName>
struct frame {
    frame(const Parent& parent, oid_type oid, GetName get_name)

    stream_type& stream() const { return parent().stream();}
    oid_type oid() const { return oid_;}
    const oid_map_type& oid_map() const { return parent().oid_map();}

    const parent_type& parent() const() { return parent_;}
    std::string name() const {return get_name_();}

    const Parent& parent_;
    oid_type oid_;
    GetName get_name_;
};

template <typename ParentFrame, typename GetNeme, typename Out>
inline auto& recv_frame(const ParentFrame& parent, GetName&& get_name, Out& out) {
    return recv_data_frame(frame{parent, read<oid_t>(parent.stream()), std::forward<GetName>(get_name)}, out);
}


template <class OidMap, class In>
inline ostream& send(ostream& out, const OidMap& oid_map, const In& in) {
    return ozo::is_null(in) ? out : detail::get_send_impl<In>::apply(out, oid_map, ozo::unwrap(in));
}

template <class OidMap, class In>
inline ostream& send_data_frame(ostream& out, const OidMap& oid_map, const In& in) {
    write(out, size_of(in));
    return send(out, oid_map, in);
}

template <class OidMap, class In>
inline ostream& send_frame(ostream& out, const OidMap& oid_map, const In& in) {
    write(out, type_oid(oid_map, in));
    return send_data_frame(out, oid_map, in);
}
} // namespace ozo