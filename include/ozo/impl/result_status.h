#pragma once

#include <libpq-fe.h>

namespace ozo::impl {

inline const char* get_result_status_name(ExecStatusType status) {
#define __OZO_CASE_RETURN(item) case item: return #item;
    switch (status) {
        __OZO_CASE_RETURN(PGRES_SINGLE_TUPLE)
        __OZO_CASE_RETURN(PGRES_TUPLES_OK)
        __OZO_CASE_RETURN(PGRES_COMMAND_OK)
        __OZO_CASE_RETURN(PGRES_COPY_OUT)
        __OZO_CASE_RETURN(PGRES_COPY_IN)
        __OZO_CASE_RETURN(PGRES_COPY_BOTH)
        __OZO_CASE_RETURN(PGRES_NONFATAL_ERROR)
        __OZO_CASE_RETURN(PGRES_BAD_RESPONSE)
        __OZO_CASE_RETURN(PGRES_EMPTY_QUERY)
        __OZO_CASE_RETURN(PGRES_FATAL_ERROR)
    }
#undef __OZO_CASE_RETURN
    return "unknown";
}

} // namespace ozo::impl
