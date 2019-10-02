#include <ozo/detail/begin_statement_builder.h>
#include <ozo/query_builder.h>

#include <gtest/gtest.h>

namespace {

namespace hana = boost::hana;

TEST(begin_statement_builder, should_build_query_according_to_options) {
    using namespace ozo;
    using namespace ozo::detail;
    using namespace hana::literals;

    EXPECT_EQ(get_text(begin_statement_builder::build(make_options(transaction_options::isolation_level = isolation_level::serializable,
                                                                   transaction_options::mode = transaction_mode::read_write,
                                                                   transaction_options::deferrability = deferrable))),
              "BEGIN ISOLATION LEVEL SERIALIZABLE READ WRITE DEFERRABLE"_s);
    EXPECT_EQ(get_text(begin_statement_builder::build(make_options(transaction_options::isolation_level = isolation_level::serializable,
                                                                   transaction_options::mode = transaction_mode::read_write,
                                                                   transaction_options::deferrability = !deferrable))),
              "BEGIN ISOLATION LEVEL SERIALIZABLE READ WRITE NOT DEFERRABLE"_s);
    EXPECT_EQ(get_text(begin_statement_builder::build(make_options(transaction_options::isolation_level = isolation_level::serializable,
                                                                   transaction_options::mode = transaction_mode::read_write))),
              "BEGIN ISOLATION LEVEL SERIALIZABLE READ WRITE"_s);

    EXPECT_EQ(get_text(begin_statement_builder::build(make_options(transaction_options::isolation_level = isolation_level::serializable,
                                                                   transaction_options::mode = transaction_mode::read_only,
                                                                   transaction_options::deferrability = deferrable))),
              "BEGIN ISOLATION LEVEL SERIALIZABLE READ ONLY DEFERRABLE"_s);
    EXPECT_EQ(get_text(begin_statement_builder::build(make_options(transaction_options::isolation_level = isolation_level::serializable,
                                                                   transaction_options::mode = transaction_mode::read_only,
                                                                   transaction_options::deferrability = !deferrable))),
              "BEGIN ISOLATION LEVEL SERIALIZABLE READ ONLY NOT DEFERRABLE"_s);
    EXPECT_EQ(get_text(begin_statement_builder::build(make_options(transaction_options::isolation_level = isolation_level::serializable,
                                                                   transaction_options::mode = transaction_mode::read_only))),
              "BEGIN ISOLATION LEVEL SERIALIZABLE READ ONLY"_s);

    EXPECT_EQ(get_text(begin_statement_builder::build(make_options(transaction_options::isolation_level = isolation_level::serializable,
                                                                   transaction_options::deferrability = deferrable))),
              "BEGIN ISOLATION LEVEL SERIALIZABLE DEFERRABLE"_s);
    EXPECT_EQ(get_text(begin_statement_builder::build(make_options(transaction_options::isolation_level = isolation_level::serializable,
                                                                   transaction_options::deferrability = !deferrable))),
              "BEGIN ISOLATION LEVEL SERIALIZABLE NOT DEFERRABLE"_s);
    EXPECT_EQ(get_text(begin_statement_builder::build(make_options(transaction_options::isolation_level = isolation_level::serializable))),
              "BEGIN ISOLATION LEVEL SERIALIZABLE"_s);


    EXPECT_EQ(get_text(begin_statement_builder::build(make_options(transaction_options::isolation_level = isolation_level::repeatable_read,
                                                                   transaction_options::mode = transaction_mode::read_write,
                                                                   transaction_options::deferrability = deferrable))),
              "BEGIN ISOLATION LEVEL REPEATABLE READ READ WRITE DEFERRABLE"_s);
    EXPECT_EQ(get_text(begin_statement_builder::build(make_options(transaction_options::isolation_level = isolation_level::repeatable_read,
                                                                   transaction_options::mode = transaction_mode::read_write,
                                                                   transaction_options::deferrability = !deferrable))),
              "BEGIN ISOLATION LEVEL REPEATABLE READ READ WRITE NOT DEFERRABLE"_s);
    EXPECT_EQ(get_text(begin_statement_builder::build(make_options(transaction_options::isolation_level = isolation_level::repeatable_read,
                                                                   transaction_options::mode = transaction_mode::read_write))),
              "BEGIN ISOLATION LEVEL REPEATABLE READ READ WRITE"_s);

    EXPECT_EQ(get_text(begin_statement_builder::build(make_options(transaction_options::isolation_level = isolation_level::repeatable_read,
                                                                   transaction_options::mode = transaction_mode::read_only,
                                                                   transaction_options::deferrability = deferrable))),
              "BEGIN ISOLATION LEVEL REPEATABLE READ READ ONLY DEFERRABLE"_s);
    EXPECT_EQ(get_text(begin_statement_builder::build(make_options(transaction_options::isolation_level = isolation_level::repeatable_read,
                                                                   transaction_options::mode = transaction_mode::read_only,
                                                                   transaction_options::deferrability = !deferrable))),
              "BEGIN ISOLATION LEVEL REPEATABLE READ READ ONLY NOT DEFERRABLE"_s);
    EXPECT_EQ(get_text(begin_statement_builder::build(make_options(transaction_options::isolation_level = isolation_level::repeatable_read,
                                                                   transaction_options::mode = transaction_mode::read_only))),
              "BEGIN ISOLATION LEVEL REPEATABLE READ READ ONLY"_s);

    EXPECT_EQ(get_text(begin_statement_builder::build(make_options(transaction_options::isolation_level = isolation_level::repeatable_read,
                                                                   transaction_options::deferrability = deferrable))),
              "BEGIN ISOLATION LEVEL REPEATABLE READ DEFERRABLE"_s);
    EXPECT_EQ(get_text(begin_statement_builder::build(make_options(transaction_options::isolation_level = isolation_level::repeatable_read,
                                                                   transaction_options::deferrability = !deferrable))),
              "BEGIN ISOLATION LEVEL REPEATABLE READ NOT DEFERRABLE"_s);
    EXPECT_EQ(get_text(begin_statement_builder::build(make_options(transaction_options::isolation_level = isolation_level::repeatable_read))),
              "BEGIN ISOLATION LEVEL REPEATABLE READ"_s);


    EXPECT_EQ(get_text(begin_statement_builder::build(make_options(transaction_options::isolation_level = isolation_level::read_committed,
                                                                   transaction_options::mode = transaction_mode::read_write,
                                                                   transaction_options::deferrability = deferrable))),
              "BEGIN ISOLATION LEVEL READ COMMITTED READ WRITE DEFERRABLE"_s);
    EXPECT_EQ(get_text(begin_statement_builder::build(make_options(transaction_options::isolation_level = isolation_level::read_committed,
                                                                   transaction_options::mode = transaction_mode::read_write,
                                                                   transaction_options::deferrability = !deferrable))),
              "BEGIN ISOLATION LEVEL READ COMMITTED READ WRITE NOT DEFERRABLE"_s);
    EXPECT_EQ(get_text(begin_statement_builder::build(make_options(transaction_options::isolation_level = isolation_level::read_committed,
                                                                   transaction_options::mode = transaction_mode::read_write))),
              "BEGIN ISOLATION LEVEL READ COMMITTED READ WRITE"_s);

    EXPECT_EQ(get_text(begin_statement_builder::build(make_options(transaction_options::isolation_level = isolation_level::read_committed,
                                                                   transaction_options::mode = transaction_mode::read_only,
                                                                   transaction_options::deferrability = deferrable))),
              "BEGIN ISOLATION LEVEL READ COMMITTED READ ONLY DEFERRABLE"_s);
    EXPECT_EQ(get_text(begin_statement_builder::build(make_options(transaction_options::isolation_level = isolation_level::read_committed,
                                                                   transaction_options::mode = transaction_mode::read_only,
                                                                   transaction_options::deferrability = !deferrable))),
              "BEGIN ISOLATION LEVEL READ COMMITTED READ ONLY NOT DEFERRABLE"_s);
    EXPECT_EQ(get_text(begin_statement_builder::build(make_options(transaction_options::isolation_level = isolation_level::read_committed,
                                                                   transaction_options::mode = transaction_mode::read_only))),
              "BEGIN ISOLATION LEVEL READ COMMITTED READ ONLY"_s);

    EXPECT_EQ(get_text(begin_statement_builder::build(make_options(transaction_options::isolation_level = isolation_level::read_committed,
                                                                   transaction_options::deferrability = deferrable))),
              "BEGIN ISOLATION LEVEL READ COMMITTED DEFERRABLE"_s);
    EXPECT_EQ(get_text(begin_statement_builder::build(make_options(transaction_options::isolation_level = isolation_level::read_committed,
                                                                   transaction_options::deferrability = !deferrable))),
              "BEGIN ISOLATION LEVEL READ COMMITTED NOT DEFERRABLE"_s);
    EXPECT_EQ(get_text(begin_statement_builder::build(make_options(transaction_options::isolation_level = isolation_level::read_committed))),
              "BEGIN ISOLATION LEVEL READ COMMITTED"_s);


    EXPECT_EQ(get_text(begin_statement_builder::build(make_options(transaction_options::isolation_level = isolation_level::read_uncommitted,
                                                                   transaction_options::mode = transaction_mode::read_write,
                                                                   transaction_options::deferrability = deferrable))),
              "BEGIN ISOLATION LEVEL READ UNCOMMITTED READ WRITE DEFERRABLE"_s);
    EXPECT_EQ(get_text(begin_statement_builder::build(make_options(transaction_options::isolation_level = isolation_level::read_uncommitted,
                                                                   transaction_options::mode = transaction_mode::read_write,
                                                                   transaction_options::deferrability = !deferrable))),
              "BEGIN ISOLATION LEVEL READ UNCOMMITTED READ WRITE NOT DEFERRABLE"_s);
    EXPECT_EQ(get_text(begin_statement_builder::build(make_options(transaction_options::isolation_level = isolation_level::read_uncommitted,
                                                                   transaction_options::mode = transaction_mode::read_write))),
              "BEGIN ISOLATION LEVEL READ UNCOMMITTED READ WRITE"_s);

    EXPECT_EQ(get_text(begin_statement_builder::build(make_options(transaction_options::isolation_level = isolation_level::read_uncommitted,
                                                                   transaction_options::mode = transaction_mode::read_only,
                                                                   transaction_options::deferrability = deferrable))),
              "BEGIN ISOLATION LEVEL READ UNCOMMITTED READ ONLY DEFERRABLE"_s);
    EXPECT_EQ(get_text(begin_statement_builder::build(make_options(transaction_options::isolation_level = isolation_level::read_uncommitted,
                                                                   transaction_options::mode = transaction_mode::read_only,
                                                                   transaction_options::deferrability = !deferrable))),
              "BEGIN ISOLATION LEVEL READ UNCOMMITTED READ ONLY NOT DEFERRABLE"_s);
    EXPECT_EQ(get_text(begin_statement_builder::build(make_options(transaction_options::isolation_level = isolation_level::read_uncommitted,
                                                                   transaction_options::mode = transaction_mode::read_only))),
              "BEGIN ISOLATION LEVEL READ UNCOMMITTED READ ONLY"_s);

    EXPECT_EQ(get_text(begin_statement_builder::build(make_options(transaction_options::isolation_level = isolation_level::read_uncommitted,
                                                                   transaction_options::deferrability = deferrable))),
              "BEGIN ISOLATION LEVEL READ UNCOMMITTED DEFERRABLE"_s);
    EXPECT_EQ(get_text(begin_statement_builder::build(make_options(transaction_options::isolation_level = isolation_level::read_uncommitted,
                                                                   transaction_options::deferrability = !deferrable))),
              "BEGIN ISOLATION LEVEL READ UNCOMMITTED NOT DEFERRABLE"_s);
    EXPECT_EQ(get_text(begin_statement_builder::build(make_options(transaction_options::isolation_level = isolation_level::read_uncommitted))),
              "BEGIN ISOLATION LEVEL READ UNCOMMITTED"_s);


    EXPECT_EQ(get_text(begin_statement_builder::build(make_options(transaction_options::mode = transaction_mode::read_write,
                                                                   transaction_options::deferrability = deferrable))),
              "BEGIN READ WRITE DEFERRABLE"_s);
    EXPECT_EQ(get_text(begin_statement_builder::build(make_options(transaction_options::mode = transaction_mode::read_write,
                                                                   transaction_options::deferrability = !deferrable))),
              "BEGIN READ WRITE NOT DEFERRABLE"_s);
    EXPECT_EQ(get_text(begin_statement_builder::build(make_options(transaction_options::mode = transaction_mode::read_write))),
              "BEGIN READ WRITE"_s);

    EXPECT_EQ(get_text(begin_statement_builder::build(make_options(transaction_options::mode = transaction_mode::read_only,
                                                                   transaction_options::deferrability = deferrable))),
              "BEGIN READ ONLY DEFERRABLE"_s);
    EXPECT_EQ(get_text(begin_statement_builder::build(make_options(transaction_options::mode = transaction_mode::read_only,
                                                                   transaction_options::deferrability = !deferrable))),
              "BEGIN READ ONLY NOT DEFERRABLE"_s);
    EXPECT_EQ(get_text(begin_statement_builder::build(make_options(transaction_options::mode = transaction_mode::read_only))),
              "BEGIN READ ONLY"_s);

    EXPECT_EQ(get_text(begin_statement_builder::build(make_options(transaction_options::deferrability = deferrable))),
              "BEGIN DEFERRABLE"_s);
    EXPECT_EQ(get_text(begin_statement_builder::build(make_options(transaction_options::deferrability = !deferrable))),
              "BEGIN NOT DEFERRABLE"_s);
    EXPECT_EQ(get_text(begin_statement_builder::build(make_options())),
              "BEGIN"_s);
}

TEST(begin_statement_builder, should_treat_none_like_non_existent_parameters) {
    using namespace ozo;
    using namespace ozo::detail;

    EXPECT_EQ(get_text(begin_statement_builder::build(make_options(transaction_options::isolation_level = ozo::none))),
              get_text(begin_statement_builder::build(make_options())));

    EXPECT_EQ(get_text(begin_statement_builder::build(make_options(transaction_options::deferrability = ozo::none,
                                                                   transaction_options::mode = transaction_mode::read_only))),
              get_text(begin_statement_builder::build(make_options(transaction_options::mode = transaction_mode::read_only))));
}

TEST(begin_statement_builder, should_allow_integral_constants_for_deferrability) {
    using namespace ozo;
    using namespace ozo::detail;

    EXPECT_EQ(get_text(begin_statement_builder::build(make_options(transaction_options::deferrability = deferrable))),
              get_text(begin_statement_builder::build(make_options(transaction_options::deferrability = std::true_type{}))));

    EXPECT_EQ(get_text(begin_statement_builder::build(make_options(transaction_options::deferrability = !deferrable))),
              get_text(begin_statement_builder::build(make_options(transaction_options::deferrability = std::false_type{}))));
}

} // namespace
