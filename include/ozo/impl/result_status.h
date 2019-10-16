#pragma once

#include <libpq-fe.h>

namespace ozo::impl {

inline const char* get_result_status_name(ExecStatusType status) noexcept {
#define OZO_CASE_RETURN(item) case item: return #item;
    switch (status) {
        OZO_CASE_RETURN(PGRES_SINGLE_TUPLE)
        OZO_CASE_RETURN(PGRES_TUPLES_OK)
        OZO_CASE_RETURN(PGRES_COMMAND_OK)
        OZO_CASE_RETURN(PGRES_COPY_OUT)
        OZO_CASE_RETURN(PGRES_COPY_IN)
        OZO_CASE_RETURN(PGRES_COPY_BOTH)
        OZO_CASE_RETURN(PGRES_NONFATAL_ERROR)
        OZO_CASE_RETURN(PGRES_BAD_RESPONSE)
        OZO_CASE_RETURN(PGRES_EMPTY_QUERY)
        OZO_CASE_RETURN(PGRES_FATAL_ERROR)
    }
#undef OZO_CASE_RETURN
    return "unknown";
}

} // namespace ozo::impl
