#pragma once

#include "ozo/row.h"
#include "ozo/error.h"

namespace ozo {

namespace detail {

template <typename Rows, typename Iterator, typename RowConverter, typename RowFactory, typename Enable = void>
struct result_converter
{
    inline error_code operator()(Rows&& rows, Iterator result, RowConverter converter, RowFactory factory);
};

template <typename Rows, typename Iterator, typename RowConverter, typename RowFactory>
struct result_converter<Rows, Iterator, RowConverter, RowFactory, typename std::enable_if<
    is_iterable<Rows>::value && is_iterator<Iterator>::value
>::type>
{
    inline error_code operator()(Rows&& rows, Iterator result,
            RowConverter converter, RowFactory factory)
    {
        error_code ec = error::ok;
        for (auto& row : rows) {
            if constexpr (is_forward_iterator<Iterator>::value) {
                (void)factory; // prevent compiler from complaining about unused variable
                ec = converter(row, *result++);
            } else if constexpr (is_output_iterator<Iterator>::value) {
                auto result_row = factory();
                ec = converter(row, result_row);
                if (!ec) {
                    result = std::move(result_row);
                }
            }
            if (ec) {
                return ec;
            }
        }
        return ec;
    }
};

} // namespace detail

template <typename Rows, typename ResultIterator, typename RowConverter, typename RowFactory = void(*)()>
error_code convert_rows(Rows&& rows, ResultIterator result, RowConverter converter, RowFactory factory = nullptr)
{
    return detail::result_converter<Rows, ResultIterator, RowConverter, RowFactory>{}(
        std::forward<Rows>(rows), result, converter, factory);
}

} // namespace ozo