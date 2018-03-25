#include "result_mock.h"

namespace {

using namespace ::testing;

using ::ozo::testing::pg_result_mock;

GTEST("ozo::value::oid()") {
    StrictGMock<pg_result_mock> mock{};
    ::ozo::value<pg_result_mock> value({object(mock), 1, 2});

    SHOULD("call field_type() with column") {
        EXPECT_INVOKE(mock, field_type, 2).WillOnce(Return(0));
        value.oid();
    }

    SHOULD("return field_type() result") {
        EXPECT_INVOKE(mock, field_type, _).WillOnce(Return(66));
        EXPECT_EQ(value.oid(), 66);
    }
}

GTEST("ozo::value::is_text()") {
    StrictGMock<pg_result_mock> mock{};
    ::ozo::value<pg_result_mock> value({object(mock), 1, 2});

    SHOULD("call field_format() with column") {
        EXPECT_INVOKE(mock, field_format, 2).WillOnce(Return(::ozo::impl::result_format::text));
        value.is_text();
    }

    SHOULD("return true if field_format() results result_format::text") {
        EXPECT_INVOKE(mock, field_format, 2).WillOnce(Return(::ozo::impl::result_format::text));
        EXPECT_TRUE(value.is_text());
    }

    SHOULD("return false if field_format() results result_format::binary") {
        EXPECT_INVOKE(mock, field_format, 2).WillOnce(Return(::ozo::impl::result_format::binary));
        EXPECT_FALSE(value.is_text());
    }
}

GTEST("ozo::value::is_binary()") {
    StrictGMock<pg_result_mock> mock{};
    ::ozo::value<pg_result_mock> value({object(mock), 1, 2});

    SHOULD("call field_format() with column") {
        EXPECT_INVOKE(mock, field_format, 2).WillOnce(Return(::ozo::impl::result_format::text));
        value.is_binary();
    }

    SHOULD("return false if field_format() results result_format::text") {
        EXPECT_INVOKE(mock, field_format, _).WillOnce(Return(::ozo::impl::result_format::text));
        EXPECT_FALSE(value.is_binary());
    }

    SHOULD("return true if field_format() results result_format::binary") {
        EXPECT_INVOKE(mock, field_format, _).WillOnce(Return(::ozo::impl::result_format::binary));
        EXPECT_TRUE(value.is_binary());
    }
}

GTEST("ozo::value::data()") {
    StrictGMock<pg_result_mock> mock{};
    ::ozo::value<pg_result_mock> value({object(mock), 1, 2});

    SHOULD("call get_value() with row and column") {
        EXPECT_INVOKE(mock, get_value, 1, 2).WillOnce(Return(nullptr));
        value.data();
    }

    SHOULD("return get_value() result") {
        const char* foo = "foo";
        EXPECT_INVOKE(mock, get_value, _, _).WillOnce(Return(foo));
        EXPECT_EQ(value.data(), foo);
    }
}

GTEST("ozo::value::size()") {
    StrictGMock<pg_result_mock> mock{};
    ::ozo::value<pg_result_mock> value({object(mock), 1, 2});

    SHOULD("call get_length() with row and column") {
        EXPECT_INVOKE(mock, get_length, 1, 2).WillOnce(Return(0));
        value.size();
    }

    SHOULD("return get_length() result") {
        EXPECT_INVOKE(mock, get_length, _, _).WillOnce(Return(777));
        EXPECT_EQ(value.size(), 777);
    }
}

GTEST("ozo::value::is_null()") {
    StrictGMock<pg_result_mock> mock{};
    ::ozo::value<pg_result_mock> value({object(mock), 1, 2});

    SHOULD("call get_isnull() with row and column") {
        EXPECT_INVOKE(mock, get_isnull, 1, 2).WillOnce(Return(false));
        value.is_null();
    }

    SHOULD("return true if get_isnull() returns true") {
        EXPECT_INVOKE(mock, get_isnull, _, _).WillOnce(Return(true));
        EXPECT_TRUE(value.is_null());
    }

    SHOULD("return false if get_isnull() returns false") {
        EXPECT_INVOKE(mock, get_isnull, _, _).WillOnce(Return(false));
        EXPECT_FALSE(value.is_null());
    }
}

GTEST("ozo::row::empty()") {
    StrictGMock<pg_result_mock> mock{};
    ::ozo::row<pg_result_mock> row({object(mock), 0, 1});

    SHOULD("return true if nfields() returns 0") {
        EXPECT_INVOKE(mock, nfields).WillOnce(Return(0));

        EXPECT_TRUE(row.empty());
    }

    SHOULD("return false if nfields() returns not 0") {
        EXPECT_INVOKE(mock, nfields).WillOnce(Return(1));

        EXPECT_FALSE(row.empty());
    }
}

GTEST("ozo::row::size()") {
    StrictGMock<pg_result_mock> mock{};
    ::ozo::row<pg_result_mock> row({object(mock), 0, 1});

    SHOULD("return nfields() result") {
        EXPECT_INVOKE(mock, nfields).WillOnce(Return(3));

        EXPECT_EQ(row.size(), 3);
    }
}

GTEST("ozo::row::begin()") {
    StrictGMock<pg_result_mock> mock{};
    ::ozo::row<pg_result_mock> row({object(mock), 0, 0});

    SHOULD("return end() if nfields() returns 0") {
        EXPECT_INVOKE(mock, nfields).WillOnce(Return(0));

        EXPECT_EQ(row.begin(), row.end());
    }

    SHOULD("return iterator on start column") {
        EXPECT_INVOKE(mock, field_type, 0).WillOnce(Return(0));
        row.begin()->oid();
    }
}

GTEST("ozo::row::find()") {
    StrictGMock<pg_result_mock> mock{};
    ::ozo::row<pg_result_mock> row({object(mock), 0, 0});

    SHOULD("call field_number() with field name") {
        const char* name = "foo";
        EXPECT_INVOKE(mock, field_number, Eq("foo")).WillOnce(Return(0));

        row.find(name);
    }

    SHOULD("return end() if field_number() returns -1") {
        EXPECT_INVOKE(mock, field_number, _).WillOnce(Return(-1));
        EXPECT_INVOKE(mock, nfields).WillRepeatedly(Return(100500));
        EXPECT_EQ(row.find("foo"), row.end());
    }

    SHOULD("return iterator on found column if field_number() returns not -1") {
        EXPECT_INVOKE(mock, field_number, _).WillOnce(Return(555));
        EXPECT_INVOKE(mock, nfields).WillRepeatedly(Return(100500));
        const auto i = row.find("foo");
        EXPECT_TRUE(i != row.end());
        EXPECT_INVOKE(mock, field_type, 555).WillOnce(Return(0));
        i->oid();
    }
}


GTEST("ozo::row::operator[] (int)") {
    StrictGMock<pg_result_mock> mock{};
    ::ozo::row<pg_result_mock> row({object(mock), 0, 0});

    SHOULD("return value proxy with column equal to argument") {
        EXPECT_INVOKE(mock, get_value, 0, 42).WillOnce(Return(nullptr));
        row[42].data();
    }
}

GTEST("ozo::row::at(int)") {
    StrictGMock<pg_result_mock> mock{};
    ::ozo::row<pg_result_mock> row({object(mock), 0, 0});

    SHOULD("return value proxy if column number valid") {
        EXPECT_INVOKE(mock, nfields).WillRepeatedly(Return(100500));
        EXPECT_INVOKE(mock, get_value, 0, 42).WillOnce(Return(nullptr));
        row.at(42).data();
    }

    SHOULD("throw std::out_of_range if column number less than 0") {
        EXPECT_INVOKE(mock, nfields).WillRepeatedly(Return(100500));
        EXPECT_THROW(row.at(-1).data(), std::out_of_range);
    }

    SHOULD("throw std::out_of_range if column number equals to nfields") {
        EXPECT_INVOKE(mock, nfields).WillRepeatedly(Return(10));
        EXPECT_THROW(row.at(10).data(), std::out_of_range);
    }

    SHOULD("throw std::out_of_range if column number greater than nfields") {
        EXPECT_INVOKE(mock, nfields).WillRepeatedly(Return(10));
        EXPECT_THROW(row.at(42).data(), std::out_of_range);
    }
}

GTEST("ozo::row::at(const char*)") {
    StrictGMock<pg_result_mock> mock{};
    ::ozo::row<pg_result_mock> row({object(mock), 0, 0});

    SHOULD("return value proxy if column name found") {
        EXPECT_INVOKE(mock, field_number, _).WillOnce(Return(42));
        EXPECT_INVOKE(mock, nfields).WillRepeatedly(Return(100500));
        EXPECT_INVOKE(mock, get_value, 0, 42).WillOnce(Return(nullptr));
        row.at("FOO").data();
    }

    SHOULD("throw std::out_of_range if column name not found") {
        EXPECT_INVOKE(mock, field_number, _).WillOnce(Return(-1));
        EXPECT_INVOKE(mock, nfields).WillRepeatedly(Return(100500));
        EXPECT_THROW(row.at("FOO").data(), std::out_of_range);
    }
}


GTEST("ozo::basic_result::empty()") {
    StrictGMock<pg_result_mock> mock{};
    ::ozo::basic_result<pg_result_mock*> result(object(mock));

    SHOULD("return true if pg_ntuples() returns 0") {
        EXPECT_INVOKE(mock, ntuples).WillOnce(Return(0));

        EXPECT_TRUE(result.empty());
    }

    SHOULD("return false if pg_ntuples() returns not 0") {
        EXPECT_INVOKE(mock, ntuples).WillOnce(Return(1));

        EXPECT_FALSE(result.empty());
    }
}

GTEST("ozo::basic_result::size()") {
    StrictGMock<pg_result_mock> mock{};
    ::ozo::basic_result<pg_result_mock*> result(object(mock));

    SHOULD("return pg_ntuples() result") {
        EXPECT_INVOKE(mock, ntuples).WillOnce(Return(43));

        EXPECT_EQ(result.size(), 43);
    }
}

GTEST("ozo::basic_result::begin()") {
    StrictGMock<pg_result_mock> mock{};
    ::ozo::basic_result<pg_result_mock*> result(object(mock));

    SHOULD("return end() if ntuples() returns 0") {
        EXPECT_INVOKE(mock, ntuples).WillOnce(Return(0));

        EXPECT_EQ(result.begin(), result.end());
    }

    SHOULD("return iterator on start column") {
        EXPECT_INVOKE(mock, get_value, 0, _).WillOnce(Return(nullptr));
        result.begin()->begin()->data();
    }
}

GTEST("ozo::basic_result::operator[] (int)") {
    StrictGMock<pg_result_mock> mock{};
    ::ozo::basic_result<pg_result_mock*> result(object(mock));

    SHOULD("return value proxy with row equal to argument") {
        EXPECT_INVOKE(mock, get_value, 42, _).WillOnce(Return(nullptr));
        result[42][0].data();
    }
}

GTEST("ozo::basic_result::at(int)") {
    StrictGMock<pg_result_mock> mock{};
    ::ozo::basic_result<pg_result_mock*> result(object(mock));

    SHOULD("return row if row number valid") {
        EXPECT_INVOKE(mock, ntuples).WillRepeatedly(Return(100500));
        result.at(42);
    }

    SHOULD("throw std::out_of_range if row number less than 0") {
        EXPECT_INVOKE(mock, ntuples).WillRepeatedly(Return(100500));
        EXPECT_THROW(result.at(-1), std::out_of_range);
    }

    SHOULD("throw std::out_of_range if column number equals to ntuples") {
        EXPECT_INVOKE(mock, ntuples).WillRepeatedly(Return(10));
        EXPECT_THROW(result.at(10), std::out_of_range);
    }

    SHOULD("throw std::out_of_range if column number greater than ntuples") {
        EXPECT_INVOKE(mock, ntuples).WillRepeatedly(Return(10));
        EXPECT_THROW(result.at(42), std::out_of_range);
    }
}

} // namespace
