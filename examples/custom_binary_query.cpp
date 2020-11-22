#include <ozo/connection_info.h>
#include <ozo/execute.h>
#include <ozo/request.h>
#include <ozo/shortcuts.h>

#include <boost/asio/io_service.hpp>
#include <boost/asio/spawn.hpp>

#include <algorithm>
#include <array>
#include <iostream>
#include <random>

namespace asio = boost::asio;

namespace {

struct Account {
    std::int64_t id;
    std::string name;
    std::int64_t balance;
};

struct InsertAccounts {
    const std::vector<Account>& accounts;
};

// Custom implementation for ozo::binary_query. It stores all data in a prepeared for libpq format.
class InsertAccountsBinaryQuery : public ozo::binary_query::implementation {
public:
    // Constructor converts vector of accounts into a binary query format.
    template <typename OidMap, typename Allocator>
    InsertAccountsBinaryQuery(const std::vector<Account>& accounts, const OidMap& oid_map, const Allocator& allocator)
    : text_("INSERT INTO accounts (id, name, balance) VALUES", allocator),
      params_count_(std::size(accounts) * 3),
      buffer_(allocator),
      types_(params_count_, ozo::oid_t{}, allocator),
      formats_(params_count_, ozo::binary_query::binary_format, allocator),
      lengths_(params_count_, 0, allocator),
      values_(params_count_, nullptr, allocator) {
        ozo::ostream os(buffer_);

        for (std::size_t i = 0, n = accounts.size(); i < n; ++i) {
            // We define length for each query paramter. std::max is used because ozo::size_of may return
            // a negative number due to libpq requirements.
            lengths_[3 * i + 0] = std::max(0, ozo::size_of(accounts[i].id));
            lengths_[3 * i + 1] = std::max(0, ozo::size_of(accounts[i].name));
            lengths_[3 * i + 2] = std::max(0, ozo::size_of(accounts[i].balance));

            // Using provided oid_map we set OID for each query parameter.
            types_[3 * i + 0] = ozo::type_oid(oid_map, accounts[i].id);
            types_[3 * i + 1] = ozo::type_oid(oid_map, accounts[i].name);
            types_[3 * i + 2] = ozo::type_oid(oid_map, accounts[i].balance);

            // Here we serialized Account fields into binary format and write it into a single buffer.
            ozo::send(os, oid_map, accounts[i].id);
            ozo::send(os, oid_map, accounts[i].name);
            ozo::send(os, oid_map, accounts[i].balance);

            // For each parameter query text should contain a placeholder starting with $1
            // grouped by rows like ($1, $2, $3).
            text_ += " ($" + std::to_string(3 * i + 1) + ", $" + std::to_string(3 * i + 2) + ", $" + std::to_string(3 * i + 3) + ")";
            if (i < n - 1) {
                text_ += ",";
            }
        }

        // Now text_ should contain a string like "INSERT INTO accounts (id, name, balance) VALUES ($1, $2, $3), ($4, $5, $6)"

        // buffer_ contains data but also we need to have a pointer for each query parameter
        std::size_t offset = 0;
        for (std::size_t i = 0; i < params_count_; ++i) {
            values_[i] = lengths_[i] ? std::data(buffer_) + offset : nullptr;
            offset += lengths_[i];
        }
    }

    // Overriden ozo::binary_query::implementation functions just return already prepared values.

    const char* text() const noexcept final {
        return text_.c_str();
    }

    const ozo::oid_t* types() const noexcept final {
        return types_.data();
    }

    const int* formats() const noexcept final {
        return formats_.data();
    }

    const int* lengths() const noexcept final {
        return lengths_.data();
    }

    const char* const* values() const noexcept final {
        return values_.data();
    }

    std::ptrdiff_t params_count() const noexcept final {
        return static_cast<std::ptrdiff_t>(params_count_);
    }

private:
    std::string text_;
    std::size_t params_count_;
    std::vector<char> buffer_;
    std::vector<ozo::oid_t> types_;
    std::vector<int> formats_;
    std::vector<int> lengths_;
    std::vector<const char*> values_;
};

const auto throw_if_error = [](ozo::error_code ec, const auto& conn) {
    if (ec) {
        std::ostringstream s;
        if (!ozo::is_null_recursive(conn)) {
            s << "libpq error message: \"" << ozo::error_message(conn)
                << "\", error context: \"" << ozo::get_error_context(conn) << "\"";
        }
        throw ozo::system_error(ec, s.str());
    }
};

template <class RandomDevice>
std::string generate_name(RandomDevice& random) {
    constexpr const char* first_names[] = {
        "Alice",
        "Bob",
        "John",
        "Mary",
        "Peter",
        "Patricia",
    };
    constexpr const char* last_names[] = {
        "Anderson",
        "Garcia",
        "Johnson",
        "Miller",
        "Smith",
        "Williams",
    };
    std::string result;
    result += first_names[std::uniform_int_distribution<>(0, std::size(first_names) - 1)(random)];
    result += " ";
    result += last_names[std::uniform_int_distribution<>(0, std::size(last_names) - 1)(random)];
    return result;
}

}

namespace ozo {

// Custom converter for InsertAccounts query into ozo::binary_query. This template specialization will be used
// by ozo::to_binary_query function.
template <>
struct to_binary_query_impl<InsertAccounts> {
    template <typename OidMap, typename Allocator>
    static binary_query apply(const InsertAccounts& query, const OidMap& oid_map, const Allocator& allocator) {
        return binary_query{std::allocate_shared<InsertAccountsBinaryQuery>(allocator, query.accounts, oid_map, allocator)};
    }
};

} // namespace ozo

int main(int argc, char **argv) {
    std::cout << "OZO custom binary query example" << std::endl;

    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <connection string> <number of rows>\n";
        return 1;
    }

    asio::io_context io;
    auto conn_info = ozo::connection_info(argv[1]);
    const auto accounts_number = std::atol(argv[2]);

    using namespace ozo::literals;
    using namespace std::chrono_literals;

    const auto coroutine = [&] (asio::yield_context yield) {
        try {
            auto conn = ozo::execute(conn_info[io], "DROP TABLE IF EXISTS accounts;"_SQL, yield);

            conn = ozo::execute(std::move(conn),
                "CREATE TABLE accounts (\
                        id INT8,\
                        name TEXT NOT NULL,\
                        balance INT8 NOT NULL,\
                        PRIMARY KEY(id)\
                );"_SQL, yield);

            // Generate records to insert, the number of records is not known at compile time
            std::minstd_rand0 random;
            std::vector<Account> accounts;
            for (std::int64_t id = 1; id <= accounts_number; ++id) {
                accounts.push_back(Account { id, generate_name(random), std::uniform_int_distribution(-10, 10)(random) });
            }

            ozo::error_code ec;
            // Insert generated records using a custom query. InsertAccounts is just a wrapper type over a vector
            // to be used for customized conversion into ozo::binary_query.
            conn = ozo::execute(std::move(conn), InsertAccounts {accounts}, yield[ec]);
            throw_if_error(ec, conn);

            using rows_type = ozo::rows_of<ozo::pg::int8, ozo::pg::text, ozo::pg::int8>;
            const auto select_all = "SELECT id, name, balance FROM accounts"_SQL;
            rows_type result;

            const auto print_result = [](const rows_type& rows) {
                std::cout << "id\tname\tbalance\n";
                boost::for_each(rows, [](auto& x) {
                    boost::hana::for_each(x, [&](auto& v) {
                        std::cout << v << '\t';
                    });
                    std::cout << '\n';
                });
            };

            conn = ozo::request(std::move(conn), select_all, 1s, ozo::into(result), yield);

            print_result(result);
        } catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;
        }
    };

    asio::spawn(io, coroutine);

    io.run();

    return 0;
}
