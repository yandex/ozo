#include <ozo/result.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace ozo::tests {

using namespace testing;

struct pg_result_mock {
    MOCK_CONST_METHOD1(field_type, ozo::oid_t(int column));
    MOCK_CONST_METHOD1(field_format, ozo::impl::result_format(int column));
    MOCK_CONST_METHOD2(get_value, const char*(int row, int column));
    MOCK_CONST_METHOD2(get_length, std::size_t(int row, int column));
    MOCK_CONST_METHOD2(get_isnull, bool(int row, int column));
    MOCK_CONST_METHOD1(field_number, int(const char* name));
    MOCK_CONST_METHOD0(nfields, int());
    MOCK_CONST_METHOD0(ntuples, int());

    friend ozo::oid_t pq_field_type(const pg_result_mock& m, int column) {
        return m.field_type(column);
    }

    friend ozo::impl::result_format pq_field_format(const pg_result_mock& m, int column) {
        return m.field_format(column);
    }

    friend const char* pq_get_value(const pg_result_mock& m, int row, int column) {
        return m.get_value(row, column);
    }

    friend std::size_t pq_get_length(const pg_result_mock& m, int row, int column) {
        return m.get_length(row, column);
    }

    friend bool pq_get_isnull(const pg_result_mock& m, int row, int column) {
        return m.get_isnull(row, column);
    }

    friend int pq_field_number(const pg_result_mock& m, const char* name) {
        return m.field_number(name);
    }

    friend int pq_nfields(const pg_result_mock& m) {
        return m.nfields();
    }

    friend int pq_ntuples(const pg_result_mock& m) {
        return m.ntuples();
    }
};

} // namespace ozo::tests
