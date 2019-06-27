#pragma once

//
// WARNING!!! THIS FILE WAS GENERATED. DO NOT MODIFY IT.
//
// This file contains information about embedded PostgreSQL types.
//     Generated from: "contrib/postgres/src/include/catalog/pg_type.dat".
//

#include <ozo/type_traits.h>

#include <boost/hana/string.hpp>

/**
 * @brief Helper macro to define type mapping between C++ type and PostgreSQL type
 *
 * In general type mapping is provided via `ozo::definitions::type` and
 * `ozo::definitions::array` specialization. The library has all the necessary information
 * (like OID, size and so on) about built-in types of PostgreSQL. To make C++ types binding
 * easer and reduce a boilerplate code the macro exists.
 *
 * @note This macro should be called in the global namespace only
 *
 * @param CppType --- C++ type to be mapped to database type
 * @param PgTypeName --- string with name of database type
 *
 * ### Example
 *
 * E.g. a definition of `std::string` as PostgreSQL `text` type may look like this:
@code
#include <ozo/pg/definitions.h>
#include <string>

OZO_PG_BIND_TYPE(std::string, "text")
@endcode
 *
 * @sa OZO_PG_DEFINE_CUSTOM_TYPE
 *
 * ### Known Types
 *
 * | Type | Size | Has Array | Short Description |
 * |------|------|-----------|-------------------|
 * | null | none | no        | OZO pseudo-type definition for null representation |
 * | aclitem | 12 bytes | yes | access control list |
 * | any | 4 bytes | no | pseudo-type representing any type |
 * | anyarray | dynamic size | no | pseudo-type representing a polymorphic array type |
 * | anyelement | 4 bytes | no | pseudo-type representing a polymorphic base type |
 * | anyenum | 4 bytes | no | pseudo-type representing a polymorphic base type that is an enum |
 * | anynonarray | 4 bytes | no | pseudo-type representing a polymorphic base type that is not an array |
 * | anyrange | dynamic size | no | pseudo-type representing a polymorphic base type that is a range |
 * | bit | dynamic size | yes | fixed-length bit string |
 * | bool | 1 bytes | yes | boolean, 'true'/'false' |
 * | box | 32 bytes | yes | geometric box '(lower left,upper right)' |
 * | bpchar | dynamic size | yes | char(length), blank-padded string, fixed storage length |
 * | bytea | dynamic size | yes | variable-length string, binary values escaped |
 * | char | 1 bytes | yes | single character |
 * | cid | 4 bytes | yes | command identifier type, sequence in transaction id |
 * | cidr | dynamic size | yes | network IP address/netmask, network address |
 * | circle | 24 bytes | yes | geometric circle '(center,radius)' |
 * | cstring | dynamic size | yes | C-style string |
 * | date | 4 bytes | yes | date |
 * | daterange | dynamic size | yes | range of dates |
 * | event_trigger | 4 bytes | no | pseudo-type for the result of an event trigger function |
 * | fdw_handler | 4 bytes | no | pseudo-type for the result of an FDW handler function |
 * | float4 | 4 bytes | yes | single-precision floating point number, 4-byte storage |
 * | float8 | 8 bytes | yes | double-precision floating point number, 8-byte storage |
 * | gtsvector | dynamic size | yes | GiST index internal text representation for text search |
 * | index_am_handler | 4 bytes | no | pseudo-type for the result of an index AM handler function |
 * | inet | dynamic size | yes | IP address/netmask, host address, netmask optional |
 * | int2 | 2 bytes | yes | -32 thousand to 32 thousand, 2-byte storage |
 * | int2vector | dynamic size | yes | array of int2, used in system tables |
 * | int4 | 4 bytes | yes | -2 billion to 2 billion integer, 4-byte storage |
 * | int4range | dynamic size | yes | range of integers |
 * | int8 | 8 bytes | yes | ~18 digit integer, 8-byte storage |
 * | int8range | dynamic size | yes | range of bigints |
 * | interval | 16 bytes | yes | @ <number> <units>, time interval |
 * | json | dynamic size | yes | JSON stored as text |
 * | jsonb | dynamic size | yes | Binary JSON |
 * | jsonpath | dynamic size | yes | JSON path |
 * | language_handler | 4 bytes | no | pseudo-type for the result of a language handler function |
 * | line | 24 bytes | yes | geometric line |
 * | lseg | 32 bytes | yes | geometric line segment '(pt1,pt2)' |
 * | macaddr | 6 bytes | yes | XX:XX:XX:XX:XX:XX, MAC address |
 * | macaddr8 | 8 bytes | yes | XX:XX:XX:XX:XX:XX:XX:XX, MAC address |
 * | money | 8 bytes | yes | monetary amounts, $d,ddd.cc |
 * | name | dynamic size | yes | 63-byte type for storing system identifiers |
 * | numeric | dynamic size | yes | numeric(precision, decimal), arbitrary precision number |
 * | numrange | dynamic size | yes | range of numerics |
 * | oid | 4 bytes | yes | object identifier(oid), maximum 4 billion |
 * | oidvector | dynamic size | yes | array of oids, used in system tables |
 * | opaque | 4 bytes | no | obsolete, deprecated pseudo-type |
 * | path | dynamic size | yes | geometric path '(pt1,...)' |
 * | pg_attribute | dynamic size | no |  |
 * | pg_class | dynamic size | no |  |
 * | pg_dependencies | dynamic size | no | multivariate dependencies |
 * | pg_lsn | 8 bytes | yes | PostgreSQL LSN datatype |
 * | pg_mcv_list | dynamic size | no | multivariate MCV list |
 * | pg_ndistinct | dynamic size | no | multivariate ndistinct coefficients |
 * | pg_node_tree | dynamic size | no | string representing an internal node tree |
 * | pg_proc | dynamic size | no |  |
 * | pg_type | dynamic size | no |  |
 * | point | 16 bytes | yes | geometric point '(x, y)' |
 * | polygon | dynamic size | yes | geometric polygon '(pt1,...)' |
 * | record | dynamic size | yes | pseudo-type representing any composite type |
 * | refcursor | dynamic size | yes | reference to cursor (portal name) |
 * | regclass | 4 bytes | yes | registered class |
 * | regconfig | 4 bytes | yes | registered text search configuration |
 * | regdictionary | 4 bytes | yes | registered text search dictionary |
 * | regnamespace | 4 bytes | yes | registered namespace |
 * | regoper | 4 bytes | yes | registered operator |
 * | regoperator | 4 bytes | yes | registered operator (with args) |
 * | regproc | 4 bytes | yes | registered procedure |
 * | regprocedure | 4 bytes | yes | registered procedure (with args) |
 * | regrole | 4 bytes | yes | registered role |
 * | regtype | 4 bytes | yes | registered type |
 * | table_am_handler | 4 bytes | no |  |
 * | text | dynamic size | yes | variable-length string, no limit specified |
 * | tid | 6 bytes | yes | (block, offset), physical location of tuple |
 * | time | 8 bytes | yes | time of day |
 * | timestamp | 8 bytes | yes | date and time |
 * | timestamptz | 8 bytes | yes | date and time with time zone |
 * | timetz | 12 bytes | yes | time of day with time zone |
 * | trigger | 4 bytes | no | pseudo-type for the result of a trigger function |
 * | tsm_handler | 4 bytes | no | pseudo-type for the result of a tablesample method function |
 * | tsquery | dynamic size | yes | query representation for text search |
 * | tsrange | dynamic size | yes | range of timestamps without time zone |
 * | tstzrange | dynamic size | yes | range of timestamps with time zone |
 * | tsvector | dynamic size | yes | text representation for text search |
 * | txid_snapshot | dynamic size | yes | txid snapshot |
 * | unknown | dynamic size | no | pseudo-type representing an undetermined type |
 * | uuid | 16 bytes | yes | UUID datatype |
 * | varbit | dynamic size | yes | variable-length bit string |
 * | varchar | dynamic size | yes | varchar(length), non-blank-padded string, variable storage length |
 * | void | 4 bytes | no | pseudo-type for the result of a function with no real result |
 * | xid | 4 bytes | yes | transaction id |
 * | xml | dynamic size | yes | XML content |
 * @ingroup group-type_system-mapping
 */
#ifdef OZO_DOCUMENTATION
#define OZO_PG_BIND_TYPE(CppType, PgTypeName)
#else
#define OZO_PG_BIND_TYPE(CppType, PgTypeName) \
    namespace ozo::definitions {\
    template <>\
    struct type<CppType> : ozo::pg::type_definition<decltype(PgTypeName##_s)> {\
        static_assert(std::is_same_v<typename type::size, dynamic_size>\
            || type::size::value == null_state_size\
            || static_cast<size_type>(sizeof(CppType)) == type::size::value,\
            #CppType " type size does not match to declared size");\
    };\
    template <>\
    struct array<CppType> : ozo::pg::array_definition<decltype(PgTypeName##_s)>{};\
    }
#endif

namespace ozo::pg {

namespace hana = boost::hana;
using namespace hana::literals;

template <typename Name>
struct type_definition {
    static_assert(std::is_void_v<Name>,
    "Unknown PostgreSQL built-in type, should be completely defined "
    "via ozo::definitions::type specialization.");
};

template <typename Name, typename = hana::when<true>>
struct array_definition {};

template <typename Name>
struct array_definition<Name, hana::when<
        !std::is_void_v<typename type_definition<Name>::array_oid>>> {
    using oid = typename type_definition<Name>::array_oid;
    using size = dynamic_size;
    using name = decltype(typename type_definition<Name>::name{} + "[]"_s);
    using value_type = typename type_definition<Name>::name;
};

template <>
struct type_definition<decltype("null"_s)> {
    using oid = null_oid_t;
    using size = decltype(null_state_size);
    using name = decltype("null"_s);
};

template <>
struct array_definition<decltype("record"_s)> {
    using oid = oid_constant<2287>;
    using value_type = decltype("record"_s);
    using size = dynamic_size;
    using name = decltype("_record"_s);
};

template <>
struct type_definition<decltype("aclitem"_s)> {
    using oid = oid_constant<1033>;
    using array_oid = oid_constant<1034>;
    using size = bytes<12>;
    using name = decltype("aclitem"_s);
};

template <>
struct type_definition<decltype("any"_s)> {
    using oid = oid_constant<2276>;
    using size = bytes<4>;
    using name = decltype("any"_s);
};

template <>
struct type_definition<decltype("anyarray"_s)> {
    using oid = oid_constant<2277>;
    using size = dynamic_size;
    using name = decltype("anyarray"_s);
};

template <>
struct type_definition<decltype("anyelement"_s)> {
    using oid = oid_constant<2283>;
    using size = bytes<4>;
    using name = decltype("anyelement"_s);
};

template <>
struct type_definition<decltype("anyenum"_s)> {
    using oid = oid_constant<3500>;
    using size = bytes<4>;
    using name = decltype("anyenum"_s);
};

template <>
struct type_definition<decltype("anynonarray"_s)> {
    using oid = oid_constant<2776>;
    using size = bytes<4>;
    using name = decltype("anynonarray"_s);
};

template <>
struct type_definition<decltype("anyrange"_s)> {
    using oid = oid_constant<3831>;
    using size = dynamic_size;
    using name = decltype("anyrange"_s);
};

template <>
struct type_definition<decltype("bit"_s)> {
    using oid = oid_constant<1560>;
    using array_oid = oid_constant<1561>;
    using size = dynamic_size;
    using name = decltype("bit"_s);
};

template <>
struct type_definition<decltype("bool"_s)> {
    using oid = oid_constant<16>;
    using array_oid = oid_constant<1000>;
    using size = bytes<1>;
    using name = decltype("bool"_s);
};

template <>
struct type_definition<decltype("box"_s)> {
    using oid = oid_constant<603>;
    using array_oid = oid_constant<1020>;
    using value_type = decltype("point"_s);
    using size = bytes<32>;
    using name = decltype("box"_s);
};

template <>
struct type_definition<decltype("bpchar"_s)> {
    using oid = oid_constant<1042>;
    using array_oid = oid_constant<1014>;
    using size = dynamic_size;
    using name = decltype("bpchar"_s);
};

template <>
struct type_definition<decltype("bytea"_s)> {
    using oid = oid_constant<17>;
    using array_oid = oid_constant<1001>;
    using size = dynamic_size;
    using name = decltype("bytea"_s);
};

template <>
struct type_definition<decltype("char"_s)> {
    using oid = oid_constant<18>;
    using array_oid = oid_constant<1002>;
    using size = bytes<1>;
    using name = decltype("char"_s);
};

template <>
struct type_definition<decltype("cid"_s)> {
    using oid = oid_constant<29>;
    using array_oid = oid_constant<1012>;
    using size = bytes<4>;
    using name = decltype("cid"_s);
};

template <>
struct type_definition<decltype("cidr"_s)> {
    using oid = oid_constant<650>;
    using array_oid = oid_constant<651>;
    using size = dynamic_size;
    using name = decltype("cidr"_s);
};

template <>
struct type_definition<decltype("circle"_s)> {
    using oid = oid_constant<718>;
    using array_oid = oid_constant<719>;
    using size = bytes<24>;
    using name = decltype("circle"_s);
};

template <>
struct type_definition<decltype("cstring"_s)> {
    using oid = oid_constant<2275>;
    using array_oid = oid_constant<1263>;
    using size = dynamic_size;
    using name = decltype("cstring"_s);
};

template <>
struct type_definition<decltype("date"_s)> {
    using oid = oid_constant<1082>;
    using array_oid = oid_constant<1182>;
    using size = bytes<4>;
    using name = decltype("date"_s);
};

template <>
struct type_definition<decltype("daterange"_s)> {
    using oid = oid_constant<3912>;
    using array_oid = oid_constant<3913>;
    using size = dynamic_size;
    using name = decltype("daterange"_s);
};

template <>
struct type_definition<decltype("event_trigger"_s)> {
    using oid = oid_constant<3838>;
    using size = bytes<4>;
    using name = decltype("event_trigger"_s);
};

template <>
struct type_definition<decltype("fdw_handler"_s)> {
    using oid = oid_constant<3115>;
    using size = bytes<4>;
    using name = decltype("fdw_handler"_s);
};

template <>
struct type_definition<decltype("float4"_s)> {
    using oid = oid_constant<700>;
    using array_oid = oid_constant<1021>;
    using size = bytes<4>;
    using name = decltype("float4"_s);
};

template <>
struct type_definition<decltype("float8"_s)> {
    using oid = oid_constant<701>;
    using array_oid = oid_constant<1022>;
    using size = bytes<8>;
    using name = decltype("float8"_s);
};

template <>
struct type_definition<decltype("gtsvector"_s)> {
    using oid = oid_constant<3642>;
    using array_oid = oid_constant<3644>;
    using size = dynamic_size;
    using name = decltype("gtsvector"_s);
};

template <>
struct type_definition<decltype("index_am_handler"_s)> {
    using oid = oid_constant<325>;
    using size = bytes<4>;
    using name = decltype("index_am_handler"_s);
};

template <>
struct type_definition<decltype("inet"_s)> {
    using oid = oid_constant<869>;
    using array_oid = oid_constant<1041>;
    using size = dynamic_size;
    using name = decltype("inet"_s);
};

template <>
struct type_definition<decltype("int2"_s)> {
    using oid = oid_constant<21>;
    using array_oid = oid_constant<1005>;
    using size = bytes<2>;
    using name = decltype("int2"_s);
};

template <>
struct type_definition<decltype("int2vector"_s)> {
    using oid = oid_constant<22>;
    using array_oid = oid_constant<1006>;
    using value_type = decltype("int2"_s);
    using size = dynamic_size;
    using name = decltype("int2vector"_s);
};

template <>
struct type_definition<decltype("int4"_s)> {
    using oid = oid_constant<23>;
    using array_oid = oid_constant<1007>;
    using size = bytes<4>;
    using name = decltype("int4"_s);
};

template <>
struct type_definition<decltype("int4range"_s)> {
    using oid = oid_constant<3904>;
    using array_oid = oid_constant<3905>;
    using size = dynamic_size;
    using name = decltype("int4range"_s);
};

template <>
struct type_definition<decltype("int8"_s)> {
    using oid = oid_constant<20>;
    using array_oid = oid_constant<1016>;
    using size = bytes<8>;
    using name = decltype("int8"_s);
};

template <>
struct type_definition<decltype("int8range"_s)> {
    using oid = oid_constant<3926>;
    using array_oid = oid_constant<3927>;
    using size = dynamic_size;
    using name = decltype("int8range"_s);
};

template <>
struct type_definition<decltype("interval"_s)> {
    using oid = oid_constant<1186>;
    using array_oid = oid_constant<1187>;
    using size = bytes<16>;
    using name = decltype("interval"_s);
};

template <>
struct type_definition<decltype("json"_s)> {
    using oid = oid_constant<114>;
    using array_oid = oid_constant<199>;
    using size = dynamic_size;
    using name = decltype("json"_s);
};

template <>
struct type_definition<decltype("jsonb"_s)> {
    using oid = oid_constant<3802>;
    using array_oid = oid_constant<3807>;
    using size = dynamic_size;
    using name = decltype("jsonb"_s);
};

template <>
struct type_definition<decltype("jsonpath"_s)> {
    using oid = oid_constant<4072>;
    using array_oid = oid_constant<4073>;
    using size = dynamic_size;
    using name = decltype("jsonpath"_s);
};

template <>
struct type_definition<decltype("language_handler"_s)> {
    using oid = oid_constant<2280>;
    using size = bytes<4>;
    using name = decltype("language_handler"_s);
};

template <>
struct type_definition<decltype("line"_s)> {
    using oid = oid_constant<628>;
    using array_oid = oid_constant<629>;
    using value_type = decltype("float8"_s);
    using size = bytes<24>;
    using name = decltype("line"_s);
};

template <>
struct type_definition<decltype("lseg"_s)> {
    using oid = oid_constant<601>;
    using array_oid = oid_constant<1018>;
    using value_type = decltype("point"_s);
    using size = bytes<32>;
    using name = decltype("lseg"_s);
};

template <>
struct type_definition<decltype("macaddr"_s)> {
    using oid = oid_constant<829>;
    using array_oid = oid_constant<1040>;
    using size = bytes<6>;
    using name = decltype("macaddr"_s);
};

template <>
struct type_definition<decltype("macaddr8"_s)> {
    using oid = oid_constant<774>;
    using array_oid = oid_constant<775>;
    using size = bytes<8>;
    using name = decltype("macaddr8"_s);
};

template <>
struct type_definition<decltype("money"_s)> {
    using oid = oid_constant<790>;
    using array_oid = oid_constant<791>;
    using size = bytes<8>;
    using name = decltype("money"_s);
};

template <>
struct type_definition<decltype("name"_s)> {
    using oid = oid_constant<19>;
    using array_oid = oid_constant<1003>;
    using value_type = decltype("char"_s);
    using size = dynamic_size;
    using name = decltype("name"_s);
};

template <>
struct type_definition<decltype("numeric"_s)> {
    using oid = oid_constant<1700>;
    using array_oid = oid_constant<1231>;
    using size = dynamic_size;
    using name = decltype("numeric"_s);
};

template <>
struct type_definition<decltype("numrange"_s)> {
    using oid = oid_constant<3906>;
    using array_oid = oid_constant<3907>;
    using size = dynamic_size;
    using name = decltype("numrange"_s);
};

template <>
struct type_definition<decltype("oid"_s)> {
    using oid = oid_constant<26>;
    using array_oid = oid_constant<1028>;
    using size = bytes<4>;
    using name = decltype("oid"_s);
};

template <>
struct type_definition<decltype("oidvector"_s)> {
    using oid = oid_constant<30>;
    using array_oid = oid_constant<1013>;
    using value_type = decltype("oid"_s);
    using size = dynamic_size;
    using name = decltype("oidvector"_s);
};

template <>
struct type_definition<decltype("opaque"_s)> {
    using oid = oid_constant<2282>;
    using size = bytes<4>;
    using name = decltype("opaque"_s);
};

template <>
struct type_definition<decltype("path"_s)> {
    using oid = oid_constant<602>;
    using array_oid = oid_constant<1019>;
    using size = dynamic_size;
    using name = decltype("path"_s);
};

template <>
struct type_definition<decltype("pg_attribute"_s)> {
    using oid = oid_constant<75>;
    using size = dynamic_size;
    using name = decltype("pg_attribute"_s);
};

template <>
struct type_definition<decltype("pg_class"_s)> {
    using oid = oid_constant<83>;
    using size = dynamic_size;
    using name = decltype("pg_class"_s);
};

template <>
struct type_definition<decltype("pg_dependencies"_s)> {
    using oid = oid_constant<3402>;
    using size = dynamic_size;
    using name = decltype("pg_dependencies"_s);
};

template <>
struct type_definition<decltype("pg_lsn"_s)> {
    using oid = oid_constant<3220>;
    using array_oid = oid_constant<3221>;
    using size = bytes<8>;
    using name = decltype("pg_lsn"_s);
};

template <>
struct type_definition<decltype("pg_mcv_list"_s)> {
    using oid = oid_constant<5017>;
    using size = dynamic_size;
    using name = decltype("pg_mcv_list"_s);
};

template <>
struct type_definition<decltype("pg_ndistinct"_s)> {
    using oid = oid_constant<3361>;
    using size = dynamic_size;
    using name = decltype("pg_ndistinct"_s);
};

template <>
struct type_definition<decltype("pg_node_tree"_s)> {
    using oid = oid_constant<194>;
    using size = dynamic_size;
    using name = decltype("pg_node_tree"_s);
};

template <>
struct type_definition<decltype("pg_proc"_s)> {
    using oid = oid_constant<81>;
    using size = dynamic_size;
    using name = decltype("pg_proc"_s);
};

template <>
struct type_definition<decltype("pg_type"_s)> {
    using oid = oid_constant<71>;
    using size = dynamic_size;
    using name = decltype("pg_type"_s);
};

template <>
struct type_definition<decltype("point"_s)> {
    using oid = oid_constant<600>;
    using array_oid = oid_constant<1017>;
    using value_type = decltype("float8"_s);
    using size = bytes<16>;
    using name = decltype("point"_s);
};

template <>
struct type_definition<decltype("polygon"_s)> {
    using oid = oid_constant<604>;
    using array_oid = oid_constant<1027>;
    using size = dynamic_size;
    using name = decltype("polygon"_s);
};

template <>
struct type_definition<decltype("record"_s)> {
    using oid = oid_constant<2249>;
    using size = dynamic_size;
    using name = decltype("record"_s);
};

template <>
struct type_definition<decltype("refcursor"_s)> {
    using oid = oid_constant<1790>;
    using array_oid = oid_constant<2201>;
    using size = dynamic_size;
    using name = decltype("refcursor"_s);
};

template <>
struct type_definition<decltype("regclass"_s)> {
    using oid = oid_constant<2205>;
    using array_oid = oid_constant<2210>;
    using size = bytes<4>;
    using name = decltype("regclass"_s);
};

template <>
struct type_definition<decltype("regconfig"_s)> {
    using oid = oid_constant<3734>;
    using array_oid = oid_constant<3735>;
    using size = bytes<4>;
    using name = decltype("regconfig"_s);
};

template <>
struct type_definition<decltype("regdictionary"_s)> {
    using oid = oid_constant<3769>;
    using array_oid = oid_constant<3770>;
    using size = bytes<4>;
    using name = decltype("regdictionary"_s);
};

template <>
struct type_definition<decltype("regnamespace"_s)> {
    using oid = oid_constant<4089>;
    using array_oid = oid_constant<4090>;
    using size = bytes<4>;
    using name = decltype("regnamespace"_s);
};

template <>
struct type_definition<decltype("regoper"_s)> {
    using oid = oid_constant<2203>;
    using array_oid = oid_constant<2208>;
    using size = bytes<4>;
    using name = decltype("regoper"_s);
};

template <>
struct type_definition<decltype("regoperator"_s)> {
    using oid = oid_constant<2204>;
    using array_oid = oid_constant<2209>;
    using size = bytes<4>;
    using name = decltype("regoperator"_s);
};

template <>
struct type_definition<decltype("regproc"_s)> {
    using oid = oid_constant<24>;
    using array_oid = oid_constant<1008>;
    using size = bytes<4>;
    using name = decltype("regproc"_s);
};

template <>
struct type_definition<decltype("regprocedure"_s)> {
    using oid = oid_constant<2202>;
    using array_oid = oid_constant<2207>;
    using size = bytes<4>;
    using name = decltype("regprocedure"_s);
};

template <>
struct type_definition<decltype("regrole"_s)> {
    using oid = oid_constant<4096>;
    using array_oid = oid_constant<4097>;
    using size = bytes<4>;
    using name = decltype("regrole"_s);
};

template <>
struct type_definition<decltype("regtype"_s)> {
    using oid = oid_constant<2206>;
    using array_oid = oid_constant<2211>;
    using size = bytes<4>;
    using name = decltype("regtype"_s);
};

template <>
struct type_definition<decltype("table_am_handler"_s)> {
    using oid = oid_constant<269>;
    using size = bytes<4>;
    using name = decltype("table_am_handler"_s);
};

template <>
struct type_definition<decltype("text"_s)> {
    using oid = oid_constant<25>;
    using array_oid = oid_constant<1009>;
    using size = dynamic_size;
    using name = decltype("text"_s);
};

template <>
struct type_definition<decltype("tid"_s)> {
    using oid = oid_constant<27>;
    using array_oid = oid_constant<1010>;
    using size = bytes<6>;
    using name = decltype("tid"_s);
};

template <>
struct type_definition<decltype("time"_s)> {
    using oid = oid_constant<1083>;
    using array_oid = oid_constant<1183>;
    using size = bytes<8>;
    using name = decltype("time"_s);
};

template <>
struct type_definition<decltype("timestamp"_s)> {
    using oid = oid_constant<1114>;
    using array_oid = oid_constant<1115>;
    using size = bytes<8>;
    using name = decltype("timestamp"_s);
};

template <>
struct type_definition<decltype("timestamptz"_s)> {
    using oid = oid_constant<1184>;
    using array_oid = oid_constant<1185>;
    using size = bytes<8>;
    using name = decltype("timestamptz"_s);
};

template <>
struct type_definition<decltype("timetz"_s)> {
    using oid = oid_constant<1266>;
    using array_oid = oid_constant<1270>;
    using size = bytes<12>;
    using name = decltype("timetz"_s);
};

template <>
struct type_definition<decltype("trigger"_s)> {
    using oid = oid_constant<2279>;
    using size = bytes<4>;
    using name = decltype("trigger"_s);
};

template <>
struct type_definition<decltype("tsm_handler"_s)> {
    using oid = oid_constant<3310>;
    using size = bytes<4>;
    using name = decltype("tsm_handler"_s);
};

template <>
struct type_definition<decltype("tsquery"_s)> {
    using oid = oid_constant<3615>;
    using array_oid = oid_constant<3645>;
    using size = dynamic_size;
    using name = decltype("tsquery"_s);
};

template <>
struct type_definition<decltype("tsrange"_s)> {
    using oid = oid_constant<3908>;
    using array_oid = oid_constant<3909>;
    using size = dynamic_size;
    using name = decltype("tsrange"_s);
};

template <>
struct type_definition<decltype("tstzrange"_s)> {
    using oid = oid_constant<3910>;
    using array_oid = oid_constant<3911>;
    using size = dynamic_size;
    using name = decltype("tstzrange"_s);
};

template <>
struct type_definition<decltype("tsvector"_s)> {
    using oid = oid_constant<3614>;
    using array_oid = oid_constant<3643>;
    using size = dynamic_size;
    using name = decltype("tsvector"_s);
};

template <>
struct type_definition<decltype("txid_snapshot"_s)> {
    using oid = oid_constant<2970>;
    using array_oid = oid_constant<2949>;
    using size = dynamic_size;
    using name = decltype("txid_snapshot"_s);
};

template <>
struct type_definition<decltype("unknown"_s)> {
    using oid = oid_constant<705>;
    using size = dynamic_size;
    using name = decltype("unknown"_s);
};

template <>
struct type_definition<decltype("uuid"_s)> {
    using oid = oid_constant<2950>;
    using array_oid = oid_constant<2951>;
    using size = bytes<16>;
    using name = decltype("uuid"_s);
};

template <>
struct type_definition<decltype("varbit"_s)> {
    using oid = oid_constant<1562>;
    using array_oid = oid_constant<1563>;
    using size = dynamic_size;
    using name = decltype("varbit"_s);
};

template <>
struct type_definition<decltype("varchar"_s)> {
    using oid = oid_constant<1043>;
    using array_oid = oid_constant<1015>;
    using size = dynamic_size;
    using name = decltype("varchar"_s);
};

template <>
struct type_definition<decltype("void"_s)> {
    using oid = oid_constant<2278>;
    using size = bytes<4>;
    using name = decltype("void"_s);
};

template <>
struct type_definition<decltype("xid"_s)> {
    using oid = oid_constant<28>;
    using array_oid = oid_constant<1011>;
    using size = bytes<4>;
    using name = decltype("xid"_s);
};

template <>
struct type_definition<decltype("xml"_s)> {
    using oid = oid_constant<142>;
    using array_oid = oid_constant<143>;
    using size = dynamic_size;
    using name = decltype("xml"_s);
};

} // namespace ozo::pg
