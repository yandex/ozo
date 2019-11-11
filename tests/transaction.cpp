#include <ozo/transaction.h>

namespace {

// Test `constexpr`ness of transaction-related stuff here (so accidental breaking changes to the interface can be noticed)
constexpr auto custom_opt = ozo::make_options(ozo::transaction_options::isolation_level = ozo::isolation_level::serializable);
[[maybe_unused]] constexpr auto custom_begin = ozo::begin.with_transaction_options(custom_opt);

}
