#include <ozo/connection_info.h>
#include <ozo/request.h>
#include <ozo/execute.h>
#include <ozo/transaction.h>

#include <boost/fusion/adapted/struct/adapt_struct.hpp>
#include <boost/hana/adapt_struct.hpp>
#include <boost/hana/members.hpp>
#include <boost/hana/size.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/spawn.hpp>

#include <iostream>

namespace asio = boost::asio;
namespace hana = boost::hana;

struct attach {
    std::string filename;
    std::string type;
    std::int64_t size;
};

BOOST_HANA_ADAPT_STRUCT(attach, filename, type, size);
BOOST_FUSION_ADAPT_STRUCT(attach, filename, type, size)

std::size_t size_of(const attach& value) {
    using ozo::size_of;
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

namespace ozo {

template <>
struct send_impl<attach> {
    template <typename M>
    static ostream& apply(ostream& out, const oid_map_t<M>& oid_map, const attach& in) {
        using ozo::size_of;
        send(out, oid_map, std::int32_t(hana::size(hana::members(in))));
        hana::for_each(hana::members(in), [&] (const auto& v) {
            send(out, oid_map, ozo::type_oid(oid_map, v));
            send(out, oid_map, std::int32_t(size_of(v)));
            send(out, oid_map, v);
        });
        return out;
    }
};

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

} // namespace ozo

std::ostream& operator <<(std::ostream& stream, const attach& value) {
    stream << "attach {";
    hana::for_each(hana::members(value), [&] (const auto& v) {
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
std::ostream& operator <<(std::ostream& stream, const hana::tuple<std::reference_wrapper<T> ...>& value) {
    stream << "{";
    hana::for_each(value, [&] (const auto& v) {
        stream << v.get() << ", ";
    });
    return stream << "}";
}

template <class ... T>
std::ostream& operator <<(std::ostream& stream, const hana::tuple<T ...>& value) {
    stream << "{";
    hana::for_each(value, [&] (const auto& v) {
        stream << v << ", ";
    });
    return stream << "}";
}

template <class ... T>
std::ostream& operator <<(std::ostream& stream, const std::tuple<T ...>& value) {
    stream << "{";
    hana::for_each(value, [&] (const auto& v) {
        stream << v << ", ";
    });
    return stream << "}";
}

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <conninfo>\n";
        return 1;
    }

    asio::io_context io(1);
    asio::spawn(io, [&] (asio::yield_context yield) {
        try {
            using namespace ozo::literals;

            {
                ozo::connection_info<> connection_info(argv[1]);

                constexpr auto init_queries = hana::make_tuple(
                    "DROP SCHEMA IF EXISTS ozo_test CASCADE;"_SQL,
                    "CREATE SCHEMA ozo_test;"_SQL,
                    "CREATE TYPE ozo_test.attach AS (filename text, type text, size bigint);"_SQL,
                    "CREATE TABLE ozo_test.messages (uid bigint, mid bigint, attaches ozo_test.attach[]);"_SQL
                );

                auto connection = ozo::get_connection(ozo::make_connector(connection_info, io), yield);

                hana::for_each(init_queries, [&] (const auto& query_builder) {
                    const auto query = query_builder.build();

                    std::cout << "Perform request with query: " << ozo::to_const_char(ozo::get_text(query)) << std::endl;

                    ozo::error_code ec;
                    ozo::execute(connection, query, yield[ec]);

                    if (ec) {
                        throw std::runtime_error("Request failed: " + ec.message()
                                                 + ", error context: " + ozo::get_error_context(connection));
                    }
                });
            }

            auto oid_map = ozo::register_types<attach, std::vector<attach>>();
            ozo::connection_info<decltype(oid_map)> connection_info(argv[1]);
            ozo::error_code ec;
            auto connection = ozo::get_connection(ozo::make_connector(connection_info, io), yield);
            auto transaction = ozo::begin(connection, yield[ec]);

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
                    ozo::error_code ec;
                    std::tie(uid, mid, attaches) = value;
                    const auto query = query_builder.build();
                    std::cout << "Perform request with query: " << ozo::to_const_char(ozo::get_text(query))
                              << ", params: " << ozo::get_params(query)
                              << std::endl;
                    ozo::execute(transaction, query, yield[ec]);

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
                ozo::error_code ec;
                std::cout << "Perform request with query: " << ozo::to_const_char(ozo::get_text(query))
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

            connection = ozo::rollback(std::move(transaction), yield[ec]);

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
