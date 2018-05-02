#include <ozo/connection_info.h>
#include <ozo/async_request.h>

#include <boost/fusion/adapted/struct/adapt_struct.hpp>
#include <boost/hana/adapt_struct.hpp>
#include <boost/hana/members.hpp>
#include <boost/hana/size.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/spawn.hpp>

#include <iostream>

struct attach {
    std::string filename;
    std::string type;
    std::int64_t size;
};

BOOST_HANA_ADAPT_STRUCT(attach, filename, type, size);
BOOST_FUSION_ADAPT_STRUCT(attach, filename, type, size)

std::size_t size_of(const attach& value) {
    using ozo::size_of;
    namespace hana = boost::hana;
    constexpr const auto header_size = sizeof(std::int32_t);
    constexpr const auto field_oid_size = sizeof(ozo::oid_t);
    constexpr const auto field_len_size = sizeof(std::int32_t);
    constexpr const auto field_meta_size = field_oid_size + field_len_size;
    const auto fields_number = hana::value(hana::size(hana::members(value)));
    const auto data_size = hana::fold(hana::members(value), std::size_t(0),
        [&] (auto r, const auto& v) { return r + size_of(v); });
    return header_size + fields_number * field_meta_size + data_size;
}

OZO_PG_DEFINE_CUSTOM_TYPE(attach, "ozo_test.attach", dynamic_size)
OZO_PG_DEFINE_CUSTOM_TYPE(std::vector<attach>, "ozo_test.attach[]", dynamic_size)

template <class OidMapT, class OutIteratorT>
constexpr OutIteratorT send(const attach& value, const OidMapT& map, OutIteratorT out) {
    using ozo::send;
    using ozo::size_of;
    out = send(std::int32_t(boost::hana::size(boost::hana::members(value))), map, out);
    boost::hana::for_each(boost::hana::members(value), [&] (const auto& v) {
        out = send(ozo::type_oid(map, v), map, out);
        out = send(std::int32_t(size_of(v)), map, out);
        out = send(v, map, out);
    });
    return out;
}

namespace ozo {

template <>
struct recv_impl<attach> {
    template <typename M>
    static istream& apply(istream& in, int32_t, const oid_map_t<M>& oid_map, attach& out) {
        using ozo::size_of;
        std::int32_t fields;
        recv(in, sizeof(fields), oid_map, fields);
        boost::fusion::for_each(out, [&] (auto& v) {
            ozo::oid_t oid;
            recv(in, sizeof(oid), oid_map, oid);
            std::int32_t len;
            recv(in, sizeof(len), oid_map, len);
            recv(in, len, oid_map, v);
        });
        return in;
    }
};

template <typename P, typename Q, typename Out, typename CompletionToken, typename = Require<ConnectionProvider<P>>>
auto request(P&& provider, Q&& query, Out&& out, CompletionToken&& token) {
    using signature_t = void (error_code, connectable_type<P>);
    async_completion<CompletionToken, signature_t> init(token);

    async_request(std::forward<P>(provider), std::forward<Q>(query), std::forward<Out>(out),
                  init.completion_handler);

    return init.result.get();
}

namespace impl {

template <typename ImplT>
struct transaction {
    ImplT impl_;
};

template <typename ImplT>
inline const auto& get_connection_handle(const transaction<ImplT>& ctx) noexcept {
    return get_connection_handle(unwrap_connection(ctx.impl_));
}

template <typename ImplT>
inline auto& get_connection_handle(transaction<ImplT>& ctx) noexcept {
    return get_connection_handle(unwrap_connection(ctx.impl_));
}

template <typename ImplT>
inline const auto& get_connection_socket(const transaction<ImplT>& ctx) noexcept {
    return get_connection_socket(unwrap_connection(ctx.impl_));
}

template <typename ImplT>
inline auto& get_connection_socket(transaction<ImplT>& ctx) noexcept {
    return get_connection_socket(unwrap_connection(ctx.impl_));
}

template <typename ImplT>
inline const auto& get_connection_oid_map(const transaction<ImplT>& ctx) noexcept {
    return get_connection_oid_map(unwrap_connection(ctx.impl_));
}

template <typename ImplT>
inline auto& get_connection_oid_map(transaction<ImplT>& ctx) noexcept {
    using impl::get_connection_oid_map;
    return get_connection_oid_map(unwrap_connection(ctx.impl_));
}

template <typename ImplT>
inline const auto& get_connection_statistics(const transaction<ImplT>& ctx) noexcept {
    return get_connection_statistics(unwrap_connection(ctx.impl_));
}

template <typename ImplT>
inline auto& get_connection_statistics(transaction<ImplT>& ctx) noexcept {
    return get_connection_statistics(unwrap_connection(ctx.impl_));
}

template <typename ImplT>
inline auto& get_connection_error_context(transaction<ImplT>& ctx) {
    return get_connection_error_context(unwrap_connection(ctx.impl_));
}

template <typename ImplT>
inline const auto& get_connection_error_context(const transaction<ImplT>& ctx) {
    return get_connection_error_context(unwrap_connection(ctx.impl_));
}

} // namespace impl


template <typename ImplT>
auto make_transaction(ImplT&& impl) {
    return impl::transaction<ImplT> {std::forward<ImplT>(impl)};
}

template <typename P, typename Out, typename CompletionToken, typename = Require<ConnectionProvider<P>>>
auto transaction(P&& provider, Out&& out, CompletionToken&& token) {
    using namespace ozo::literals;
    using signature_t = void (error_code, impl::transaction<connectable_type<P>>);
    async_completion<CompletionToken, signature_t> init(token);

    async_request(std::forward<P>(provider), "BEGIN"_SQL, std::forward<Out>(out),
        [handler = init.completion_handler] (error_code ec, auto conn) mutable {
            handler(std::move(ec), make_transaction(std::move(conn)));
        });

    return init.result.get();
}

template <typename ImplT, typename Query, typename Out, typename CompletionToken, typename = Require<Connectable<ImplT>>>
auto end_transaction(impl::transaction<ImplT>&& transaction, Query&& query, Out&& out, CompletionToken&& token) {
    using signature_t = void (error_code, std::decay_t<ImplT>);
    async_completion<CompletionToken, signature_t> init(token);

    async_request(std::move(transaction), std::forward<Query>(query), std::forward<Out>(out),
        [handler = init.completion_handler] (error_code ec, auto transaction) mutable {
            handler(std::move(ec), std::move(transaction.impl_));
        });

    return init.result.get();
}

template <typename ImplT,  typename Out, typename CompletionToken, typename = Require<Connectable<ImplT>>>
auto commit(impl::transaction<ImplT>&& transaction, Out&& out, CompletionToken&& token) {
    using namespace ozo::literals;
    return end_transaction(std::move(transaction), "COMMIT"_SQL,
                           std::forward<Out>(out), std::forward<CompletionToken>(token));
}

template <typename ImplT,  typename Out, typename CompletionToken, typename = Require<Connectable<ImplT>>>
auto rollback(impl::transaction<ImplT>&& transaction, Out&& out, CompletionToken&& token) {
    using namespace ozo::literals;
    return end_transaction(std::move(transaction), "ROLLBACK"_SQL,
                           std::forward<Out>(out), std::forward<CompletionToken>(token));
}

} // namespace ozo

std::ostream& operator <<(std::ostream& stream, const attach& value) {
    stream << "attach {";
    boost::hana::for_each(boost::hana::members(value), [&] (const auto& v) {
        stream << v << ", ";
    });
    return stream << "}";
}

template <class T>
std::ostream& operator <<(std::ostream& stream, const std::vector<T>& value) {
    stream << "{";
    boost::for_each(value, [&] (const auto& v) {
        stream << v << ", ";
    });
    return stream << "}";
}

template <class ... T>
std::ostream& operator <<(std::ostream& stream, const boost::hana::tuple<std::reference_wrapper<T> ...>& value) {
    stream << "{";
    boost::hana::for_each(value, [&] (const auto& v) {
        stream << v.get() << ", ";
    });
    return stream << "}";
}

template <class ... T>
std::ostream& operator <<(std::ostream& stream, const boost::hana::tuple<T ...>& value) {
    stream << "{";
    boost::hana::for_each(value, [&] (const auto& v) {
        stream << v << ", ";
    });
    return stream << "}";
}

template <class ... T>
std::ostream& operator <<(std::ostream& stream, const std::tuple<T ...>& value) {
    stream << "{";
    boost::hana::for_each(value, [&] (const auto& v) {
        stream << v << ", ";
    });
    return stream << "}";
}

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <conninfo>\n";
        return 1;
    }

    namespace asio = ::boost::asio;
    namespace hana = ::boost::hana;

    asio::io_context io(1);
    asio::spawn(io, [&] (auto yield) {
        try {
            using namespace ozo::literals;

            auto oid_map = ozo::register_types<attach, std::vector<attach>>();
            ozo::connection_info<decltype(oid_map)> connection_info(io, argv[1]);

            ozo::result result;
            boost::system::error_code ec;
            auto transaction = ozo::transaction(connection_info, result, yield[ec]);

            if (ec) {
                throw std::runtime_error("Request failed: " + ec.message()
                                         + ", error context: " + ozo::get_error_context(transaction));
            }

            {
                constexpr auto init_queries = hana::make_tuple(
                    "DROP SCHEMA IF EXISTS ozo_test CASCADE;"_SQL,
                    "CREATE SCHEMA ozo_test;"_SQL,
                    "CREATE TYPE ozo_test.attach AS (filename text, type text, size bigint);"_SQL,
                    "CREATE TABLE ozo_test.messages (uid bigint, mid bigint, attaches ozo_test.attach[]);"_SQL
                );

                hana::for_each(init_queries, [&] (const auto& query_builder) {
                    const auto query = query_builder.build();

                    std::cout << "Perform request with query: " << ozo::get_text(query) << std::endl;

                    ozo::result result;
                    boost::system::error_code ec;
                    ozo::request(transaction, query, result, yield[ec]);

                    if (ec) {
                        throw std::runtime_error("Request failed: " + ec.message()
                                                 + ", error context: " + ozo::get_error_context(transaction));
                    }
                });

                const auto query = "SELECT oid, typarray FROM pg_type WHERE typname = 'attach';"_SQL.build();

                std::cout << "Perform request with query: " << ozo::get_text(query) << std::endl;

                std::vector<std::tuple<ozo::oid_t, ozo::oid_t>> oids;
                boost::system::error_code ec;
                ozo::request(transaction, query, std::back_inserter(oids), yield[ec]);

                if (ec) {
                    throw std::runtime_error("Request failed: " + ec.message()
                                             + ", error context: " + ozo::get_error_context(transaction));
                }

                if (oids.empty()) {
                    throw std::runtime_error("Oid for type attach is not found");
                }

                ozo::set_type_oid<attach>(get_oid_map(transaction), std::get<0>(oids.front()));
                ozo::set_type_oid<std::vector<attach>>(get_oid_map(transaction), std::get<1>(oids.front()));
            }

            {
                std::int64_t uid;
                std::int64_t mid;
                std::vector<attach> attaches;
                const auto query_builder = "INSERT INTO ozo_test.messages (uid, mid, attaches) VALUES ("_SQL
                        + std::cref(uid) + ", "_SQL + std::cref(mid) + ", "_SQL + std::cref(attaches) + ")"_SQL;

                const auto values = {
                    std::make_tuple(std::int64_t(1), std::int64_t(1), std::vector<attach>({})),
                    std::make_tuple(std::int64_t(1), std::int64_t(2), std::vector<attach>({
                        attach {"foo.jpeg", "image/jpeg", 13124}
                    })),
                    std::make_tuple(std::int64_t(1), std::int64_t(3), std::vector<attach>({
                        attach {"report.txt", "text/plain", 5344},
                        attach {"doc.txt", "text/plain", 3434}
                    })),
                    std::make_tuple(std::int64_t(2), std::int64_t(1), std::vector<attach>({})),
                    std::make_tuple(std::int64_t(2), std::int64_t(2), std::vector<attach>({})),
                };

                boost::for_each(values, [&] (const auto& value) {
                    ozo::result result;
                    boost::system::error_code ec;
                    std::tie(uid, mid, attaches) = value;
                    const auto query = query_builder.build();
                    std::cout << "Perform request with query: " << ozo::get_text(query)
                              << ", params: " << ozo::get_params(query)
                              << std::endl;
                    ozo::request(transaction, query, result, yield[ec]);

                    if (ec) {
                        throw std::runtime_error("Request failed: " + ec.message()
                                                 + ", error context: " + ozo::get_error_context(transaction));
                    }
                });
            }

            {
                const auto query = (
                    "SELECT mid, attaches "_SQL +
                      "FROM ozo_test.messages "_SQL +
                     "WHERE uid = "_SQL + std::int64_t(1) +
                      " AND mid = ANY("_SQL + std::vector<std::int64_t>({2, 3}) + ")"_SQL
                ).build();

                std::vector<std::tuple<std::int64_t, std::vector<attach>>> result;
                boost::system::error_code ec;
                std::cout << "Perform request with query: " << ozo::get_text(query)
                          << ", params: " << ozo::get_params(query)
                          << std::endl;
                ozo::request(transaction, query, std::back_inserter(result), yield[ec]);

                if (ec) {
                    throw std::runtime_error("Request failed: " + ec.message()
                                             + ", error context: " + ozo::get_error_context(transaction));
                }

                std::cout << "Selected attaches:" << std::endl;
                boost::for_each(result, [&] (const auto& v) {
                    std::cout << std::get<0>(v) << ", " << std::get<1>(v) << std::endl;
                });
            }

            auto connection = ozo::rollback(std::move(transaction), result, yield[ec]);

            if (ec) {
                throw std::runtime_error("Request failed: " + ec.message()
                                         + ", error context: " + ozo::get_error_context(connection));
            }
        } catch (const std::exception& e) {
            std::cout << e.what() << std::endl;
        }
    });

    io.run();

    return 0;
}
