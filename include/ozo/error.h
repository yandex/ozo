#pragma once

#include <ozo/detail/base36.h>
#include <boost/system/system_error.hpp>

namespace ozo {

using error_code = boost::system::error_code;
using system_error = boost::system::system_error;
using error_category = boost::system::error_category;
using error_condition = boost::system::error_condition;

// OZO related errors
namespace error {

enum code {
    ok, // to do not use error code 0
    pq_connection_start_failed, // PQConnectionStart function failed
    pq_socket_failed, // PQSocket returned -1 as fd - it seems like there is no connection
    pq_connection_status_bad, // PQstatus returned CONNECTION_BAD
    pq_connect_poll_failed, // PQconnectPoll function failed
    oid_type_mismatch, // no conversion possible from oid to user-supplied type
    unexpected_eof, // unecpected EOF while data read
    pg_send_query_params_failed, // PQsendQueryParams function failed
    pg_consume_input_failed, // PQconsumeInput function failed
    pg_set_nonblocking_failed, // PQsetnonblocking function failed
    pg_flush_failed, // PQflush function failed
    bad_result_process, // error while processing or converting result from the database
    no_sql_state_found, // no sql state has been found in database query execution error reply
    result_status_unexpected, // got unexpected status from query result
    result_status_empty_query, // the string sent to the server was empty
    result_status_bad_response, // the server's response was not understood
    oid_request_failed, // error during request oids from a database
};

const error_category& category() noexcept;

} // namespace error

// SQL state related errors and conditions
namespace sqlstate {

/**
* This set of error conditions can not be complete, since new versions
* of PostgreSQL can add another ones and user can create new ones inside
* a DB logic. So this set can be used to determine most of sql states
* but not all.
*/
enum code {
    successful_completion = 0, // 00000
    warning = 46656, // 01000
    dynamic_result_sets_returned = 46668, // 0100C
    implicit_zero_bit_padding = 46664, // 01008
    null_value_eliminated_in_set_function = 46659, // 01003
    privilege_not_granted = 46663, // 01007
    privilege_not_revoked = 46662, // 01006
    string_data_right_truncation_warning = 46660, // 01004
    deprecated_feature = 79057, // 01P01
    no_data = 93312, // 02000
    no_additional_dynamic_result_sets_returned = 93313, // 02001
    sql_statement_not_yet_complete = 139968, // 03000
    connection_exception = 373248, // 08000
    connection_does_not_exist = 373251, // 08003
    connection_failure = 373254, // 08006
    sqlclient_unable_to_establish_sqlconnection = 373249, // 08001
    sqlserver_rejected_establishment_of_sqlconnection = 373252, // 08004
    transaction_resolution_unknown = 373255, // 08007
    protocol_violation = 405649, // 08P01
    triggered_action_exception = 419904, // 09000
    feature_not_supported = 466560, // 0A000
    invalid_transaction_initiation = 513216, // 0B000
    locator_exception = 699840, // 0F000
    invalid_locator_specification = 699841, // 0F001
    invalid_grantor = 979776, // 0L000
    invalid_grant_operation = 1012177, // 0LP01
    invalid_role_specification = 1166400, // 0P000
    diagnostics_exception = 1632960, // 0Z000
    stacked_diagnostics_accessed_without_active_handler = 1632962, // 0Z002
    case_not_found = 3359232, // 20000
    cardinality_violation = 3405888, // 21000
    data_exception = 3452544, // 22000
    array_subscript_error = 3452630, // 2202E
    character_not_in_repertoire = 3452617, // 22021
    datetime_field_overflow = 3452552, // 22008
    division_by_zero = 3452582, // 22012
    error_in_assignment = 3452549, // 22005
    escape_character_conflict = 3452555, // 2200B
    indicator_overflow = 3452618, // 22022
    interval_field_overflow = 3452585, // 22015
    invalid_argument_for_logarithm = 3452594, // 2201E
    invalid_argument_for_ntile_function = 3452584, // 22014
    invalid_argument_for_nth_value_function = 3452586, // 22016
    invalid_argument_for_power_function = 3452595, // 2201F
    invalid_argument_for_width_bucket_function = 3452596, // 2201G
    invalid_character_value_for_cast = 3452588, // 22018
    invalid_datetime_format = 3452551, // 22007
    invalid_escape_character = 3452589, // 22019
    invalid_escape_octet = 3452557, // 2200D
    invalid_escape_sequence = 3452621, // 22025
    nonstandard_use_of_escape_character = 3484950, // 22P06
    invalid_indicator_parameter_value = 3452580, // 22010
    invalid_parameter_value = 3452619, // 22023
    invalid_regular_expression = 3452591, // 2201B
    invalid_row_count_in_limit_clause = 3452612, // 2201W
    invalid_row_count_in_result_offset_clause = 3452613, // 2201X
    invalid_time_zone_displacement_value = 3452553, // 22009
    invalid_use_of_escape_character = 3452556, // 2200C
    most_specific_type_mismatch = 3452560, // 2200G
    null_value_not_allowed = 3452548, // 22004
    null_value_no_indicator_parameter = 3452546, // 22002
    numeric_value_out_of_range = 3452547, // 22003
    string_data_length_mismatch = 3452622, // 22026
    string_data_right_truncation = 3452545, // 22001
    substring_error = 3452581, // 22011
    trim_error = 3452623, // 22027
    unterminated_c_string = 3452620, // 22024
    zero_length_character_string = 3452559, // 2200F
    floating_point_exception = 3484945, // 22P01
    invalid_text_representation = 3484946, // 22P02
    invalid_binary_representation = 3484947, // 22P03
    bad_copy_file_format = 3484948, // 22P04
    untranslatable_character = 3484949, // 22P05
    not_an_xml_document = 3452565, // 2200L
    invalid_xml_document = 3452566, // 2200M
    invalid_xml_content = 3452567, // 2200N
    invalid_xml_comment = 3452572, // 2200S
    invalid_xml_processing_instruction = 3452573, // 2200T
    integrity_constraint_violation = 3499200, // 23000
    restrict_violation = 3499201, // 23001
    not_null_violation = 3505682, // 23502
    foreign_key_violation = 3505683, // 23503
    unique_violation = 3505685, // 23505
    check_violation = 3505720, // 23514
    exclusion_violation = 3531601, // 23P01
    invalid_cursor_state = 3545856, // 24000
    invalid_transaction_state = 3592512, // 25000
    active_sql_transaction = 3592513, // 25001
    branch_transaction_already_active = 3592514, // 25002
    held_cursor_requires_same_isolation_level = 3592520, // 25008
    inappropriate_access_mode_for_branch_transaction = 3592515, // 25003
    inappropriate_isolation_level_for_branch_transaction = 3592516, // 25004
    no_active_sql_transaction_for_branch_transaction = 3592517, // 25005
    read_only_sql_transaction = 3592518, // 25006
    schema_and_data_statement_mixing_not_supported = 3592519, // 25007
    no_active_sql_transaction = 3624913, // 25P01
    in_failed_sql_transaction = 3624914, // 25P02
    invalid_sql_statement_name = 3639168, // 26000
    triggered_data_change_violation = 3685824, // 27000
    invalid_authorization_specification = 3732480, // 28000
    invalid_password = 3764881, // 28P01
    dependent_privilege_descriptors_still_exist = 3872448, // 2B000
    dependent_objects_still_exist = 3904849, // 2BP01
    invalid_transaction_termination = 3965760, // 2D000
    sql_routine_exception = 4059072, // 2F000
    function_executed_no_return_statement = 4059077, // 2F005
    modifying_sql_data_not_permitted = 4059074, // 2F002
    prohibited_sql_statement_attempted = 4059075, // 2F003
    reading_sql_data_not_permitted = 4059076, // 2F004
    invalid_cursor_name = 5225472, // 34000
    external_routine_exception = 5412096, // 38000
    containing_sql_not_permitted = 5412097, // 38001
    modifying_sql_data_not_permitted_external = 5412098, // 38002
    prohibited_sql_statement_attempted_external = 5412099, // 38003
    reading_sql_data_not_permitted_external = 5412100, // 38004
    external_routine_invocation_exception = 5458752, // 39000
    invalid_sqlstate_returned = 5458753, // 39001
    null_value_not_allowed_external = 5458756, // 39004
    trigger_protocol_violated = 5491153, // 39P01
    srf_protocol_violated = 5491154, // 39P02
    savepoint_exception = 5552064, // 3B000
    invalid_savepoint_specification = 5552065, // 3B001
    invalid_catalog_name = 5645376, // 3D000
    invalid_schema_name = 5738688, // 3F000
    transaction_rollback = 6718464, // 40000
    transaction_integrity_constraint_violation = 6718466, // 40002
    serialization_failure = 6718465, // 40001
    statement_completion_unknown = 6718467, // 40003
    deadlock_detected = 6750865, // 40P01
    syntax_error_or_access_rule_violation = 6811776, // 42000
    syntax_error = 6819553, // 42601
    insufficient_privilege = 6818257, // 42501
    cannot_coerce = 6822294, // 42846
    grouping_error = 6822147, // 42803
    windowing_error = 6844248, // 42P20
    invalid_recursion = 6844221, // 42P19
    invalid_foreign_key = 6822252, // 42830
    invalid_name = 6819554, // 42602
    name_too_long = 6819626, // 42622
    reserved_name = 6823557, // 42939
    datatype_mismatch = 6822148, // 42804
    indeterminate_datatype = 6844220, // 42P18
    collation_mismatch = 6844249, // 42P21
    indeterminate_collation = 6844250, // 42P22
    wrong_object_type = 6822153, // 42809
    undefined_column = 6820851, // 42703
    undefined_function = 6822435, // 42883
    undefined_table = 6844177, // 42P01
    undefined_parameter = 6844178, // 42P02
    undefined_object = 6820852, // 42704
    duplicate_column = 6820849, // 42701
    duplicate_cursor = 6844179, // 42P03
    duplicate_database = 6844180, // 42P04
    duplicate_function = 6820923, // 42723
    duplicate_prepared_statement = 6844181, // 42P05
    duplicate_schema = 6844182, // 42P06
    duplicate_table = 6844183, // 42P07
    duplicate_alias = 6820886, // 42712
    duplicate_object = 6820884, // 42710
    ambiguous_column = 6820850, // 42702
    ambiguous_function = 6820925, // 42725
    ambiguous_parameter = 6844184, // 42P08
    ambiguous_alias = 6844185, // 42P09
    invalid_column_reference = 6844212, // 42P10
    invalid_column_definition = 6819589, // 42611
    invalid_cursor_definition = 6844213, // 42P11
    invalid_database_definition = 6844214, // 42P12
    invalid_function_definition = 6844215, // 42P13
    invalid_prepared_statement_definition = 6844216, // 42P14
    invalid_schema_definition = 6844217, // 42P15
    invalid_table_definition = 6844218, // 42P16
    invalid_object_definition = 6844219, // 42P17
    with_check_option_violation = 6905088, // 44000
    insufficient_resources = 8538048, // 53000
    disk_full = 8539344, // 53100
    out_of_memory = 8540640, // 53200
    too_many_connections = 8541936, // 53300
    configuration_limit_exceeded = 8543232, // 53400
    program_limit_exceeded = 8584704, // 54000
    statement_too_complex = 8584705, // 54001
    too_many_columns = 8584741, // 54011
    too_many_arguments = 8584779, // 54023
    object_not_in_prerequisite_state = 8631360, // 55000
    object_in_use = 8631366, // 55006
    cant_change_runtime_param = 8663762, // 55P02
    lock_not_available = 8663763, // 55P03
    operator_intervention = 8724672, // 57000
    query_canceled = 8724712, // 57014
    admin_shutdown = 8757073, // 57P01
    crash_shutdown = 8757074, // 57P02
    cannot_connect_now = 8757075, // 57P03
    database_dropped = 8757076, // 57P04
    system_error = 8771328, // 58000
    io_error = 8771436, // 58030
    undefined_file = 8803729, // 58P01
    duplicate_file = 8803730, // 58P02
    config_file_error = 25194240, // F0000
    lock_file_exists = 25194241, // F0001
    fdw_error = 29999808, // HV000
    fdw_column_name_not_found = 29999813, // HV005
    fdw_dynamic_parameter_value_needed = 29999810, // HV002
    fdw_function_sequence_error = 29999844, // HV010
    fdw_inconsistent_descriptor_information = 29999881, // HV021
    fdw_invalid_attribute_value = 29999884, // HV024
    fdw_invalid_column_name = 29999815, // HV007
    fdw_invalid_column_number = 29999816, // HV008
    fdw_invalid_data_type = 29999812, // HV004
    fdw_invalid_data_type_descriptors = 29999814, // HV006
    fdw_invalid_descriptor_field_identifier = 30000133, // HV091
    fdw_invalid_handle = 29999819, // HV00B
    fdw_invalid_option_index = 29999820, // HV00C
    fdw_invalid_option_name = 29999821, // HV00D
    fdw_invalid_string_length_or_buffer_length = 30000132, // HV090
    fdw_invalid_string_format = 29999818, // HV00A
    fdw_invalid_use_of_null_pointer = 29999817, // HV009
    fdw_too_many_handles = 29999848, // HV014
    fdw_out_of_memory = 29999809, // HV001
    fdw_no_schemas = 29999833, // HV00P
    fdw_option_name_not_found = 29999827, // HV00J
    fdw_reply_handle = 29999828, // HV00K
    fdw_schema_not_found = 29999834, // HV00Q
    fdw_table_not_found = 29999835, // HV00R
    fdw_unable_to_create_execution = 29999829, // HV00L
    fdw_unable_to_create_reply = 29999830, // HV00M
    fdw_unable_to_establish_connection = 29999831, // HV00N
    plpgsql_error = 41990400, // P0000
    raise_exception = 41990401, // P0001
    no_data_found = 41990402, // P0002
    too_many_rows = 41990403, // P0003
    internal_error = 56966976, // XX000
    data_corrupted = 56966977, // XX001
    index_corrupted = 56966978, // XX002
};

const error_category& category() noexcept;

} // namespace sqlstate

// Common error conditions
namespace errc {

enum code {
    ok,
    sql_error,
    connect_error,
    transport_error,
    database_readonly,
};

const error_category& category() noexcept;

} // namespace errc

} // namespace ozo

namespace boost {
namespace system {

template <>
struct is_error_code_enum<ozo::error::code> : std::true_type {};

template <>
struct is_error_condition_enum<ozo::sqlstate::code> : std::true_type {};

template <>
struct is_error_condition_enum<ozo::errc::code> : std::true_type {};

} // namespace system
} // namespace boost

namespace ozo {
namespace error {

inline auto make_error_code(const code e) {
    return error_code(static_cast<int>(e), category());
}

namespace impl {

class category : public error_category {
public:
    const char* name() const noexcept override { return "ozo::error::category"; }

    std::string message(int value) const override {
        switch (code(value)) {
            case ok:
                return "no error";
            case pq_connection_start_failed:
                return "pq_connection_start_failed - PQConnectionStart function failed";
            case pq_socket_failed:
                return "pq_socket_failed - PQSocket returned -1 as fd - it seems like there is no connection";
            case pq_connection_status_bad:
                return "pq_connection_status_bad - PQstatus returned CONNECTION_BAD";
            case pq_connect_poll_failed:
                return "pq_connect_poll_failed - PQconnectPoll function failed";
            case oid_type_mismatch:
                return "no conversion possible from oid to user-supplied type";
            case unexpected_eof:
                return "unecpected EOF while data read";
            case pg_send_query_params_failed:
                return "pg_send_query_params_failed - PQsendQueryParams function failed";
            case pg_consume_input_failed:
                return "pg_consume_input_failed - PQconsumeInput function failed";
            case pg_set_nonblocking_failed:
                return "pg_set_nonblocking_failed - PQsetnonblocking function failed";
            case pg_flush_failed:
                return "pg_flush_failed - PQflush function failed";
            case bad_result_process:
                return "bad_result_process - error while processing or converting result from the database";
            case no_sql_state_found:
                return "no_sql_state_found - no sql state has been found in database query execution error reply";
            case result_status_unexpected:
                return "result_status_unexpected - got unexpected status from query result";
            case result_status_empty_query:
                return "result_status_empty_query - the string sent to the server was empty";
            case result_status_bad_response:
                return "result_status_bad_response - the server's response was not understood";
            case oid_request_failed:
                return "error during request oids from a database";
        }
        return "no message for value: " + std::to_string(value);
    }
};

} // namespace impl

inline const error_category& category() noexcept {
    static impl::category instance;
    return instance;
}

} // namespace error

namespace sqlstate {

inline auto make_error_code(const int e) {
    return error_code(e, category());
}

inline auto make_error_condition(const code e) {
    return error_condition(static_cast<int>(e), category());
}

namespace impl {

class category : public error_category {
public:
    const char* name() const noexcept override { return "ozo::sqlstate::category"; }

    std::string message(int value) const override {
        #define __OZO_SQLSTATE_NAME(value) case value: return std::string(#value) + "(" + detail::ltob36(value) + ")";
        switch (value) {
            __OZO_SQLSTATE_NAME(successful_completion)
            __OZO_SQLSTATE_NAME(warning)
            __OZO_SQLSTATE_NAME(dynamic_result_sets_returned)
            __OZO_SQLSTATE_NAME(implicit_zero_bit_padding)
            __OZO_SQLSTATE_NAME(null_value_eliminated_in_set_function)
            __OZO_SQLSTATE_NAME(privilege_not_granted)
            __OZO_SQLSTATE_NAME(privilege_not_revoked)
            __OZO_SQLSTATE_NAME(string_data_right_truncation_warning)
            __OZO_SQLSTATE_NAME(deprecated_feature)
            __OZO_SQLSTATE_NAME(no_data)
            __OZO_SQLSTATE_NAME(no_additional_dynamic_result_sets_returned)
            __OZO_SQLSTATE_NAME(sql_statement_not_yet_complete)
            __OZO_SQLSTATE_NAME(connection_exception)
            __OZO_SQLSTATE_NAME(connection_does_not_exist)
            __OZO_SQLSTATE_NAME(connection_failure)
            __OZO_SQLSTATE_NAME(sqlclient_unable_to_establish_sqlconnection)
            __OZO_SQLSTATE_NAME(sqlserver_rejected_establishment_of_sqlconnection)
            __OZO_SQLSTATE_NAME(transaction_resolution_unknown)
            __OZO_SQLSTATE_NAME(protocol_violation)
            __OZO_SQLSTATE_NAME(triggered_action_exception)
            __OZO_SQLSTATE_NAME(feature_not_supported)
            __OZO_SQLSTATE_NAME(invalid_transaction_initiation)
            __OZO_SQLSTATE_NAME(locator_exception)
            __OZO_SQLSTATE_NAME(invalid_locator_specification)
            __OZO_SQLSTATE_NAME(invalid_grantor)
            __OZO_SQLSTATE_NAME(invalid_grant_operation)
            __OZO_SQLSTATE_NAME(invalid_role_specification)
            __OZO_SQLSTATE_NAME(diagnostics_exception)
            __OZO_SQLSTATE_NAME(stacked_diagnostics_accessed_without_active_handler)
            __OZO_SQLSTATE_NAME(case_not_found)
            __OZO_SQLSTATE_NAME(cardinality_violation)
            __OZO_SQLSTATE_NAME(data_exception)
            __OZO_SQLSTATE_NAME(array_subscript_error)
            __OZO_SQLSTATE_NAME(character_not_in_repertoire)
            __OZO_SQLSTATE_NAME(datetime_field_overflow)
            __OZO_SQLSTATE_NAME(division_by_zero)
            __OZO_SQLSTATE_NAME(error_in_assignment)
            __OZO_SQLSTATE_NAME(escape_character_conflict)
            __OZO_SQLSTATE_NAME(indicator_overflow)
            __OZO_SQLSTATE_NAME(interval_field_overflow)
            __OZO_SQLSTATE_NAME(invalid_argument_for_logarithm)
            __OZO_SQLSTATE_NAME(invalid_argument_for_ntile_function)
            __OZO_SQLSTATE_NAME(invalid_argument_for_nth_value_function)
            __OZO_SQLSTATE_NAME(invalid_argument_for_power_function)
            __OZO_SQLSTATE_NAME(invalid_argument_for_width_bucket_function)
            __OZO_SQLSTATE_NAME(invalid_character_value_for_cast)
            __OZO_SQLSTATE_NAME(invalid_datetime_format)
            __OZO_SQLSTATE_NAME(invalid_escape_character)
            __OZO_SQLSTATE_NAME(invalid_escape_octet)
            __OZO_SQLSTATE_NAME(invalid_escape_sequence)
            __OZO_SQLSTATE_NAME(nonstandard_use_of_escape_character)
            __OZO_SQLSTATE_NAME(invalid_indicator_parameter_value)
            __OZO_SQLSTATE_NAME(invalid_parameter_value)
            __OZO_SQLSTATE_NAME(invalid_regular_expression)
            __OZO_SQLSTATE_NAME(invalid_row_count_in_limit_clause)
            __OZO_SQLSTATE_NAME(invalid_row_count_in_result_offset_clause)
            __OZO_SQLSTATE_NAME(invalid_time_zone_displacement_value)
            __OZO_SQLSTATE_NAME(invalid_use_of_escape_character)
            __OZO_SQLSTATE_NAME(most_specific_type_mismatch)
            __OZO_SQLSTATE_NAME(null_value_not_allowed)
            __OZO_SQLSTATE_NAME(null_value_no_indicator_parameter)
            __OZO_SQLSTATE_NAME(numeric_value_out_of_range)
            __OZO_SQLSTATE_NAME(string_data_length_mismatch)
            __OZO_SQLSTATE_NAME(string_data_right_truncation)
            __OZO_SQLSTATE_NAME(substring_error)
            __OZO_SQLSTATE_NAME(trim_error)
            __OZO_SQLSTATE_NAME(unterminated_c_string)
            __OZO_SQLSTATE_NAME(zero_length_character_string)
            __OZO_SQLSTATE_NAME(floating_point_exception)
            __OZO_SQLSTATE_NAME(invalid_text_representation)
            __OZO_SQLSTATE_NAME(invalid_binary_representation)
            __OZO_SQLSTATE_NAME(bad_copy_file_format)
            __OZO_SQLSTATE_NAME(untranslatable_character)
            __OZO_SQLSTATE_NAME(not_an_xml_document)
            __OZO_SQLSTATE_NAME(invalid_xml_document)
            __OZO_SQLSTATE_NAME(invalid_xml_content)
            __OZO_SQLSTATE_NAME(invalid_xml_comment)
            __OZO_SQLSTATE_NAME(invalid_xml_processing_instruction)
            __OZO_SQLSTATE_NAME(integrity_constraint_violation)
            __OZO_SQLSTATE_NAME(restrict_violation)
            __OZO_SQLSTATE_NAME(not_null_violation)
            __OZO_SQLSTATE_NAME(foreign_key_violation)
            __OZO_SQLSTATE_NAME(unique_violation)
            __OZO_SQLSTATE_NAME(check_violation)
            __OZO_SQLSTATE_NAME(exclusion_violation)
            __OZO_SQLSTATE_NAME(invalid_cursor_state)
            __OZO_SQLSTATE_NAME(invalid_transaction_state)
            __OZO_SQLSTATE_NAME(active_sql_transaction)
            __OZO_SQLSTATE_NAME(branch_transaction_already_active)
            __OZO_SQLSTATE_NAME(held_cursor_requires_same_isolation_level)
            __OZO_SQLSTATE_NAME(inappropriate_access_mode_for_branch_transaction)
            __OZO_SQLSTATE_NAME(inappropriate_isolation_level_for_branch_transaction)
            __OZO_SQLSTATE_NAME(no_active_sql_transaction_for_branch_transaction)
            __OZO_SQLSTATE_NAME(read_only_sql_transaction)
            __OZO_SQLSTATE_NAME(schema_and_data_statement_mixing_not_supported)
            __OZO_SQLSTATE_NAME(no_active_sql_transaction)
            __OZO_SQLSTATE_NAME(in_failed_sql_transaction)
            __OZO_SQLSTATE_NAME(invalid_sql_statement_name)
            __OZO_SQLSTATE_NAME(triggered_data_change_violation)
            __OZO_SQLSTATE_NAME(invalid_authorization_specification)
            __OZO_SQLSTATE_NAME(invalid_password)
            __OZO_SQLSTATE_NAME(dependent_privilege_descriptors_still_exist)
            __OZO_SQLSTATE_NAME(dependent_objects_still_exist)
            __OZO_SQLSTATE_NAME(invalid_transaction_termination)
            __OZO_SQLSTATE_NAME(sql_routine_exception)
            __OZO_SQLSTATE_NAME(function_executed_no_return_statement)
            __OZO_SQLSTATE_NAME(modifying_sql_data_not_permitted)
            __OZO_SQLSTATE_NAME(prohibited_sql_statement_attempted)
            __OZO_SQLSTATE_NAME(reading_sql_data_not_permitted)
            __OZO_SQLSTATE_NAME(invalid_cursor_name)
            __OZO_SQLSTATE_NAME(external_routine_exception)
            __OZO_SQLSTATE_NAME(containing_sql_not_permitted)
            __OZO_SQLSTATE_NAME(modifying_sql_data_not_permitted_external)
            __OZO_SQLSTATE_NAME(prohibited_sql_statement_attempted_external)
            __OZO_SQLSTATE_NAME(reading_sql_data_not_permitted_external)
            __OZO_SQLSTATE_NAME(external_routine_invocation_exception)
            __OZO_SQLSTATE_NAME(invalid_sqlstate_returned)
            __OZO_SQLSTATE_NAME(null_value_not_allowed_external)
            __OZO_SQLSTATE_NAME(trigger_protocol_violated)
            __OZO_SQLSTATE_NAME(srf_protocol_violated)
            __OZO_SQLSTATE_NAME(savepoint_exception)
            __OZO_SQLSTATE_NAME(invalid_savepoint_specification)
            __OZO_SQLSTATE_NAME(invalid_catalog_name)
            __OZO_SQLSTATE_NAME(invalid_schema_name)
            __OZO_SQLSTATE_NAME(transaction_rollback)
            __OZO_SQLSTATE_NAME(transaction_integrity_constraint_violation)
            __OZO_SQLSTATE_NAME(serialization_failure)
            __OZO_SQLSTATE_NAME(statement_completion_unknown)
            __OZO_SQLSTATE_NAME(deadlock_detected)
            __OZO_SQLSTATE_NAME(syntax_error_or_access_rule_violation)
            __OZO_SQLSTATE_NAME(syntax_error)
            __OZO_SQLSTATE_NAME(insufficient_privilege)
            __OZO_SQLSTATE_NAME(cannot_coerce)
            __OZO_SQLSTATE_NAME(grouping_error)
            __OZO_SQLSTATE_NAME(windowing_error)
            __OZO_SQLSTATE_NAME(invalid_recursion)
            __OZO_SQLSTATE_NAME(invalid_foreign_key)
            __OZO_SQLSTATE_NAME(invalid_name)
            __OZO_SQLSTATE_NAME(name_too_long)
            __OZO_SQLSTATE_NAME(reserved_name)
            __OZO_SQLSTATE_NAME(datatype_mismatch)
            __OZO_SQLSTATE_NAME(indeterminate_datatype)
            __OZO_SQLSTATE_NAME(collation_mismatch)
            __OZO_SQLSTATE_NAME(indeterminate_collation)
            __OZO_SQLSTATE_NAME(wrong_object_type)
            __OZO_SQLSTATE_NAME(undefined_column)
            __OZO_SQLSTATE_NAME(undefined_function)
            __OZO_SQLSTATE_NAME(undefined_table)
            __OZO_SQLSTATE_NAME(undefined_parameter)
            __OZO_SQLSTATE_NAME(undefined_object)
            __OZO_SQLSTATE_NAME(duplicate_column)
            __OZO_SQLSTATE_NAME(duplicate_cursor)
            __OZO_SQLSTATE_NAME(duplicate_database)
            __OZO_SQLSTATE_NAME(duplicate_function)
            __OZO_SQLSTATE_NAME(duplicate_prepared_statement)
            __OZO_SQLSTATE_NAME(duplicate_schema)
            __OZO_SQLSTATE_NAME(duplicate_table)
            __OZO_SQLSTATE_NAME(duplicate_alias)
            __OZO_SQLSTATE_NAME(duplicate_object)
            __OZO_SQLSTATE_NAME(ambiguous_column)
            __OZO_SQLSTATE_NAME(ambiguous_function)
            __OZO_SQLSTATE_NAME(ambiguous_parameter)
            __OZO_SQLSTATE_NAME(ambiguous_alias)
            __OZO_SQLSTATE_NAME(invalid_column_reference)
            __OZO_SQLSTATE_NAME(invalid_column_definition)
            __OZO_SQLSTATE_NAME(invalid_cursor_definition)
            __OZO_SQLSTATE_NAME(invalid_database_definition)
            __OZO_SQLSTATE_NAME(invalid_function_definition)
            __OZO_SQLSTATE_NAME(invalid_prepared_statement_definition)
            __OZO_SQLSTATE_NAME(invalid_schema_definition)
            __OZO_SQLSTATE_NAME(invalid_table_definition)
            __OZO_SQLSTATE_NAME(invalid_object_definition)
            __OZO_SQLSTATE_NAME(with_check_option_violation)
            __OZO_SQLSTATE_NAME(insufficient_resources)
            __OZO_SQLSTATE_NAME(disk_full)
            __OZO_SQLSTATE_NAME(out_of_memory)
            __OZO_SQLSTATE_NAME(too_many_connections)
            __OZO_SQLSTATE_NAME(configuration_limit_exceeded)
            __OZO_SQLSTATE_NAME(program_limit_exceeded)
            __OZO_SQLSTATE_NAME(statement_too_complex)
            __OZO_SQLSTATE_NAME(too_many_columns)
            __OZO_SQLSTATE_NAME(too_many_arguments)
            __OZO_SQLSTATE_NAME(object_not_in_prerequisite_state)
            __OZO_SQLSTATE_NAME(object_in_use)
            __OZO_SQLSTATE_NAME(cant_change_runtime_param)
            __OZO_SQLSTATE_NAME(lock_not_available)
            __OZO_SQLSTATE_NAME(operator_intervention)
            __OZO_SQLSTATE_NAME(query_canceled)
            __OZO_SQLSTATE_NAME(admin_shutdown)
            __OZO_SQLSTATE_NAME(crash_shutdown)
            __OZO_SQLSTATE_NAME(cannot_connect_now)
            __OZO_SQLSTATE_NAME(database_dropped)
            __OZO_SQLSTATE_NAME(system_error)
            __OZO_SQLSTATE_NAME(io_error)
            __OZO_SQLSTATE_NAME(undefined_file)
            __OZO_SQLSTATE_NAME(duplicate_file)
            __OZO_SQLSTATE_NAME(config_file_error)
            __OZO_SQLSTATE_NAME(lock_file_exists)
            __OZO_SQLSTATE_NAME(fdw_error)
            __OZO_SQLSTATE_NAME(fdw_column_name_not_found)
            __OZO_SQLSTATE_NAME(fdw_dynamic_parameter_value_needed)
            __OZO_SQLSTATE_NAME(fdw_function_sequence_error)
            __OZO_SQLSTATE_NAME(fdw_inconsistent_descriptor_information)
            __OZO_SQLSTATE_NAME(fdw_invalid_attribute_value)
            __OZO_SQLSTATE_NAME(fdw_invalid_column_name)
            __OZO_SQLSTATE_NAME(fdw_invalid_column_number)
            __OZO_SQLSTATE_NAME(fdw_invalid_data_type)
            __OZO_SQLSTATE_NAME(fdw_invalid_data_type_descriptors)
            __OZO_SQLSTATE_NAME(fdw_invalid_descriptor_field_identifier)
            __OZO_SQLSTATE_NAME(fdw_invalid_handle)
            __OZO_SQLSTATE_NAME(fdw_invalid_option_index)
            __OZO_SQLSTATE_NAME(fdw_invalid_option_name)
            __OZO_SQLSTATE_NAME(fdw_invalid_string_length_or_buffer_length)
            __OZO_SQLSTATE_NAME(fdw_invalid_string_format)
            __OZO_SQLSTATE_NAME(fdw_invalid_use_of_null_pointer)
            __OZO_SQLSTATE_NAME(fdw_too_many_handles)
            __OZO_SQLSTATE_NAME(fdw_out_of_memory)
            __OZO_SQLSTATE_NAME(fdw_no_schemas)
            __OZO_SQLSTATE_NAME(fdw_option_name_not_found)
            __OZO_SQLSTATE_NAME(fdw_reply_handle)
            __OZO_SQLSTATE_NAME(fdw_schema_not_found)
            __OZO_SQLSTATE_NAME(fdw_table_not_found)
            __OZO_SQLSTATE_NAME(fdw_unable_to_create_execution)
            __OZO_SQLSTATE_NAME(fdw_unable_to_create_reply)
            __OZO_SQLSTATE_NAME(fdw_unable_to_establish_connection)
            __OZO_SQLSTATE_NAME(plpgsql_error)
            __OZO_SQLSTATE_NAME(raise_exception)
            __OZO_SQLSTATE_NAME(no_data_found)
            __OZO_SQLSTATE_NAME(too_many_rows)
            __OZO_SQLSTATE_NAME(internal_error)
            __OZO_SQLSTATE_NAME(data_corrupted)
            __OZO_SQLSTATE_NAME(index_corrupted)
        }
    #undef __OZO_SQLSTATE_NAME
        return "sql state " + detail::ltob36(value);
    }
};

} // namespace impl

inline const error_category& category() noexcept {
    static impl::category instance;
    return instance;
}

} // namespace sqlstate

} // namespace ozo
