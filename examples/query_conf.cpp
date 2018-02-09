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
#include <fstream>
#include <string_view>

#include <libpq-fe.h>

static constexpr auto binary_format = 1;

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

using namespace boost::hana::literals;

struct number_by_numeral_word {
    static constexpr auto name = "number by numeral word"_s;
    using parameters_type = std::tuple<std::string>;
};

struct numeral_word_by_number {
    static constexpr auto name = "numeral word by number"_s;
    using parameters_type = std::tuple<std::int64_t>;
};

int main(int argc, char **argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <conninfo> <query_conf>\n";
        return 1;
    }

    std::ifstream query_conf_file(argv[2]);
    if (!query_conf_file.is_open()) {
        std::cerr << "Can't open query conf file: " << argv[2] << '\n';
        return -1;
    }

    std::noskipws(query_conf_file);
    const std::string query_conf_text {std::istream_iterator<char>(query_conf_file), std::istream_iterator<char>()};
    const auto query_repository = ozo::make_query_repository<
        number_by_numeral_word,
        numeral_word_by_number
    >(std::string_view {query_conf_text});

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
            PQfinish(ozo::get_native_handle(connection));
            return;
        }

        PQclear(init_query_result);

        {
            std::cout << "Enter word: ";
            std::string word;
            std::getline(std::cin, word);
            const auto query = query_repository.make_query<number_by_numeral_word>(word);
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

            const auto query = query_repository.make_query<numeral_word_by_number>(number);
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
    });

    return 0;
}
