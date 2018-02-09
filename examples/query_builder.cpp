#include <ozo/binary_query.h>
#include <ozo/connection_info.h>
#include <ozo/query.h>
#include <ozo/query_builder.h>
#include <ozo/query_conf.h>
#include <ozo/result.h>

#include <boost/fusion/adapted/struct/adapt_struct.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/spawn.hpp>

#include <iostream>

#include <libpq-fe.h>

static constexpr auto binary_format = 1;

struct numeral_row {
    std::int64_t number;
    std::string word;
};

BOOST_FUSION_ADAPT_STRUCT(numeral_row, number, word)

struct word_row {
    std::string word;
};

BOOST_FUSION_ADAPT_STRUCT(word_row, word)

struct number_row {
    std::int64_t number;
};

BOOST_FUSION_ADAPT_STRUCT(number_row, number)

struct pg_value {
    ozo::oid_t oid() const { return oid_; }
    const char* bytes() const { return bytes_; }
    int size() const { return size_; }

    ozo::oid_t oid_;
    const char* bytes_;
    int size_;
};

std::vector<std::vector<pg_value>> make_rows(PGresult* pg_result) {
    int n_tuples = PQntuples(pg_result);
    std::vector<std::vector<pg_value>> rows;
    for (int i = 0; i < n_tuples; ++i) {
        rows.emplace_back();
        int n_fields = PQnfields(pg_result);
        for (int j = 0; j < n_fields; ++j) {
            rows.back().emplace_back(pg_value{
                static_cast<ozo::oid_t>(PQftype(pg_result, j)),
                PQgetvalue(pg_result, i, j),
                PQgetlength(pg_result, i, j)
            });
        }
    }
    return rows;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <conninfo>\n";
        return 1;
    }

    namespace asio = ::boost::asio;

    asio::io_service io;
    asio::spawn(io, [&] (auto yield) {
        using namespace ozo::literals;

        ozo::connection_info<> connection_info(io, argv[1]);
        auto connection = ozo::get_connection(connection_info, yield);

        const auto init_query = (
            "BEGIN;\n"_SQL +
            "DROP TABLE IF EXISTS numerals;\n"_SQL +
            "CREATE TABLE numerals (number bigint, word text);\n"_SQL +
            "INSERT INTO numerals VALUES (1, 'first'), (2, 'second'), (3, 'third');\n"_SQL +
            "COMMIT;"_SQL
        ).build();
        std::cout << "Execute init query: " << ozo::get_text(init_query) << std::endl;
        const auto init_query_result = PQexec(ozo::get_native_handle(connection), ozo::get_text(init_query).data());

        if (PQresultStatus(init_query_result) != PGRES_COMMAND_OK) {
            std::cerr << "Init query failed: " << PQresultErrorMessage(init_query_result) << '\n';
            PQclear(init_query_result);
            return;
        }

        PQclear(init_query_result);

        {
            const auto query = ("SELECT number, word FROM numerals"_SQL).build();
            const auto binary_query = ozo::make_binary_query(query);

            std::cout << "Execute query: " << ozo::get_text(query) << std::endl;
            const auto result = PQexecParams(
                ozo::get_native_handle(connection),
                binary_query.text(),
                0,
                nullptr,
                nullptr,
                nullptr,
                nullptr,
                binary_format
            );
            if (PQresultStatus(result) != PGRES_TUPLES_OK) {
                std::cerr << "Query failed: " << PQresultErrorMessage(result) << '\n';
                PQclear(init_query_result);
                return;
            }

            auto rows = make_rows(result);
            std::vector<numeral_row> user_result;
            ozo::convert_rows(
                rows,
                std::back_inserter(user_result),
                [] (auto&& data, auto& row) { return ozo::convert_row(std::forward<decltype(data)>(data), row); },
                [] () { return numeral_row {}; }
            );

            for (const auto& row : user_result) {
                std::cout << row.number << " " << row.word << std::endl;
            }

            PQclear(result);
        }

        {
            std::cout << "Enter word: ";
            std::string word;
            std::getline(std::cin, word);
            const auto query = ("SELECT number FROM numerals WHERE word = "_SQL + word).build();
            const auto binary_query = ozo::make_binary_query(query);
            std::cout << "Execute query: " << ozo::get_text(query) << std::endl;

            const auto result = PQexecParams(
                ozo::get_native_handle(connection),
                binary_query.text(),
                binary_query.params_count,
                binary_query.types(),
                binary_query.values(),
                binary_query.lengths(),
                binary_query.formats(),
                binary_format
            );
            if (PQresultStatus(result) != PGRES_TUPLES_OK) {
                std::cerr << "Query failed: " << PQresultErrorMessage(result) << '\n';
                PQclear(init_query_result);
                return;
            }

            auto rows = make_rows(result);
            std::vector<number_row> user_result;
            ozo::convert_rows(
                rows,
                std::back_inserter(user_result),
                [] (auto&& data, auto& row) { return ozo::convert_row(std::forward<decltype(data)>(data), row); },
                [] () { return number_row {}; }
            );

            for (const auto& row : user_result) {
                std::cout << row.number << std::endl;
            }

            PQclear(result);
        }

        {
            std::cout << "Enter number: ";
            std::int64_t number;
            std::cin >> number;

            const auto query = ("SELECT word FROM numerals WHERE number >= "_SQL + number).build();
            const auto binary_query = ozo::make_binary_query(query);
            std::cout << "Execute query: " << ozo::get_text(query) << std::endl;

            const auto result = PQexecParams(
                ozo::get_native_handle(connection),
                binary_query.text(),
                binary_query.params_count,
                binary_query.types(),
                binary_query.values(),
                binary_query.lengths(),
                binary_query.formats(),
                binary_format
            );
            if (PQresultStatus(result) != PGRES_TUPLES_OK) {
                std::cerr << "Query failed: " << PQresultErrorMessage(result) << '\n';
                PQclear(init_query_result);
                PQfinish(ozo::get_native_handle(connection));
                return;
            }

            auto rows = make_rows(result);
            std::vector<word_row> user_result;
            ozo::convert_rows(
                rows,
                std::back_inserter(user_result),
                [] (auto&& data, auto& row) { return ozo::convert_row(std::forward<decltype(data)>(data), row); },
                [] () { return word_row {}; }
            );

            for (const auto& row : user_result) {
                std::cout << row.word << std::endl;
            }

            PQclear(result);
        }

        {
            std::cout << "Enter first summand: ";
            std::int32_t first_summand;
            std::cin >> first_summand;
            std::cout << "Enter second summand: ";
            std::int64_t second_summand;
            std::cin >> second_summand;

            const auto query = ("SELECT "_SQL + first_summand + " + "_SQL + second_summand).build();
            const auto binary_query = ozo::make_binary_query(query);
            std::cout << "Execute query: " << ozo::get_text(query) << std::endl;

            const auto result = PQexecParams(
                ozo::get_native_handle(connection),
                binary_query.text(),
                binary_query.params_count,
                binary_query.types(),
                binary_query.values(),
                binary_query.lengths(),
                binary_query.formats(),
                binary_format
            );
            if (PQresultStatus(result) != PGRES_TUPLES_OK) {
                std::cerr << "Query failed: " << PQresultErrorMessage(result) << '\n';
                PQclear(init_query_result);
                return;
            }

            auto rows = make_rows(result);
            std::vector<number_row> user_result;
            ozo::convert_rows(
                rows,
                std::back_inserter(user_result),
                [] (auto&& data, auto& row) { return ozo::convert_row(std::forward<decltype(data)>(data), row); },
                [] () { return number_row {}; }
            );

            for (const auto& row : user_result) {
                std::cout << row.number << std::endl;
            }

            PQclear(result);
        }
    });

    io.run();

    return 0;
}
