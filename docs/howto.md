# Table of Contents

Here are some examples of how to use the OZO API.

<!-- TOC -->
- [Table of Contents](#Table-of-Contents)
  - [How To Make A Very Simple Request](#How-To-Make-A-Very-Simple-Request)
  - [How To Properly Handle Errors](#How-To-Properly-Handle-Errors)
  - [How To Map Column Names to Column Numbers At Compile Time](#How-To-Map-Column-Names-to-Column-Numbers-At-Compile-Time)
  - [How To Determine Which C++ Type You Need To Use For A PostgreSQL Type](#How-To-Determine-Which-C++-Type-You-Need-To-Use-For-A-PostgreSQL-Type)
  - [How To Bind Multiple PostgreSQL Types To The Same C++ Type](#How-To-Bind-Multiple-PostgreSQL-Types-To-The-Same-C++-Type)

## How To Make A Very Simple Request

E.g. you have a _very_ simple table.

```sql
CREATE TABLE users_info(
    id          bigint  NOT NULL,
    name        text,
    amount      bigint  NOT NULL
);
```

If you want to execute a query with no custom types or other advanced behavior then the simplest way to do this is:

```cpp
#include <ozo/request.h>
#include <ozo/connection_info.h>
#include <ozo/shortcuts.h>
#include <boost/asio.hpp>

int main() {
    // The boost io_context is the central management object for asyncronous operations.
    boost::asio::io_context io;

    // Container of rows which accepts integer and nullable string columns in the sequence.
    // (This is an alias on std::vector of std::tuple - see the documentation)
    ozo::rows_of<std::int64_t, std::optional<std::string>> rows;

    // Connection info with host and port to connect to
    // This also supports a PostgreSQL connection URI string
    ozo::connection_info<> conn_info = ozo::make_connection_info("host=... port=...");

    // For _SQL literal
    using namespace ozo::literals;

    std::int64_t myParam = ...;

    // Our query statement
    const auto query = "SELECT id, name FROM users_info WHERE amount>="_SQL + myParam;

    // Request with connection provider, query and callback.
    // Note that ozo::request does not manage the lifetime of the destination for the query results.
    // You can use std::shared_ptr, and pass the pointer to your callback function, to get easy lifetime management.
    ozo::request(conn_info[io], query, ozo::into(rows),
            [&](ozo::error_code ec, auto conn) {
        // Check to see if we received an error:
        if (ec) {
            // * Print error code's message
            std::cerr << ec.message();
            // * Print error message from underlying libpq
            std::cerr << " | " << ozo::error_message(conn);
            // * Print additional error context from OZO
            if (!ozo::is_null_recursive(conn)) {
                std::cerr << " | " << ozo::get_error_context(conn);
            }
            return;
        };

        // Connection should be in good state here,
        // typically you do not need to check this.
        assert(ozo::connection_good(conn));

        // We got results, let's do something with them, e.g. print them out
        std::cout << "id" << '\t' << "name" << std::endl;
        for(auto& row : rows) {
            std::cout << std::get<0>(row) << '\t' << std::get<1>(row) << std::endl;
        }
    });

    io.run();
}
```

Let's look a little bit closer at this basic asynchronous query example.

```cpp
ozo::rows_of<std::int64_t, std::optional<std::string>> rows;
```

Here we define a result type. `ozo::rows_of` is an alias of `std::vector<std::tuple<...>>`. And `ozo::into` is an alias of `std::back_inserter`. So the `ozo::request()` function will fill this vector of tuples by back inserting data from database's response, row by row. Please read the documentation for more details.

It is _very important_ to preserve the same order of fields in the request and the types in the tuple. It is a little annoying, but there is a way to avoid this requirement using [Boost.Hana](https://www.boost.org/doc/libs/1_66_0/libs/hana/doc/html/index.html#tutorial-introspection-adapting) or [Boost.Fusion](https://www.boost.org/doc/libs/1_66_0/libs/fusion/doc/html/fusion/adapted.html) structure adaptation. See the section - [How To Map Column Names to Column Numbers At Compile Time](#How-To-Map-Column-Names-to-Column-Numbers-At-Compile-Time) for a brief tutorial on how to do this.

Notice that the second position of the tuple is `std::optional<std::string>`.This is because the `name` field of the table can be _NULL_. An empty optional represents a _NULL_ value (you can learn more about Nullable concept from the  OZO documentation). If if a _NULL_ value is retrieved from the database for a type that is not an `std::optional`, then a deserialization error will occur.

Note that in the example, the table is defined with the first (id) and third (amount) columns as _NOT NULL_. This means that for queries retrieving those columns, it's not necessary to use `std::optional` for those fields. However, the second (name) column is can be _NULL_, and therefore must be an `std::optional` as explained in the paragraph above.

Note that it's acceptable to provide an `std::optional` for a _NOT NULL_ field, but it is not acceptable to omit the `std::optional` for a field that is not _NOT NULL_, unless you can guarantee the retrieved data will not be _NULL_. Failure to properly use `std::optional` will lead to a run-time error in case of a _NULL_ value received from a database.

```cpp
auto conn_info = ozo::make_connection_info("host=... port=...");
```

Now we need to create a connection information for database to connect to. This is our connection source which can create a connection for us as it will be needed (you can learn more about `ConnectionSource` and `ConnectionProvider` concepts from the documentation). It's also acceptable to provide a connection URI string, instead of the comma seperated version.

Here's how to construct a database query.

```cpp
std::int64_t myParam = ...;
using namespace ozo::literals;
const auto query = "SELECT id, name FROM users_info WHERE amount>="_SQL + myParam;
```

The `_SQL` suffix is a user defined literal (from OZO) that converts the string that it is attached to into OZO's query type. Note that the parameter will be passed as a separate binary parameter, never as part of the query text, even if you provide a parameter that is known at compile time.

You can also use `ozo::make_query()`, if you prefer to use parameterized queries with the `$1` style notation to indicate your query parameters:

```cpp
std::int64_t myParam = ...;
static contexpr auto querytext = "SELECT id, name FROM users_info WHERE amount>=$1";
const auto query = ozo::make_query(querytext, myParam);
```

A query is sent to the database using the asynchrounous function call `ozo::request()`.

```cpp
ozo::request(conn_info[io], query, ozo::into(res),
        [&](ozo::error_code ec, auto conn) {
//...
});
```

`conn_info[io]` - This acquires the ozo::request function with the information it needs to connect to the database. The `conn_info` variable provides the connection information, and the `io` variable, passed to `operator[]` provides the `boost::asio::io_context` to use for asyncronous activities. The result is a `ConnectionProvider` object which will provide `ozo::request` with a connection to the database.

There's some details that need to be unpacked here. Notably, ozo has two generic concepts relating to establishing a connection to the database.

- **ConnectionSource** -- A type that knows how to construct or acquire a **ConnectionProvider** when provided with an `io_context` and possibly other paramaters. There is no concrete type **ConnectionSource**, instead it's a set of compile-time requirements, like C++20 Concepts, that provide you the flexibility to provide your own implementation.
- **ConnectionProvider** -- A type that can provide a concrete connection to a database. It can be any entity that's able to provide a new (or already established) connection on demand. This can be an actual, established, connection, since the established connection can simply provide itself.

`query` - the next argument is query which we discussed above.

`ozo::into(res)` - the output parameter. In this case the output parameter is a back insert iterator to the result vector. Note, that the life time of the output parameter should be managed by a user. In this example it correctly placed on stack since its lifetime overlaps `io.run()` call. Another way you can do this is to use `std::make_shared`, and then store the resulting shared pointer in your callback function. That way the memory that `ozo::into` is writing into will stay valid until the callback function finishes.

The query output parameter can be an iterator with appropriate value type, or an `ozo::result` object which provides access to raw binary data. The second variant is not recommended since the user would need to implement binary protocol parsing.

`[&](ozo::error_code ec, auto conn)` - completion function parameter, in this example it is a callback lambda. In other cases it can be [boost::asio::use_future](https://www.boost.org/doc/libs/1_67_0/doc/html/boost_asio/reference/use_future.html), [boost::asio::yield_context](https://www.boost.org/doc/libs/1_67_0/doc/html/boost_asio/reference/yield_context.html) or any other compatible concept, such as: [boost::asio::async_result](https://www.boost.org/doc/libs/1_67_0/doc/html/boost_asio/reference/async_result.html), [Completion Token](https://www.boost.org/doc/libs/1_67_0/doc/html/boost_asio/reference/async_completion.html). The arguments of the call back are an error code `ec` (which is namely `boost::system::error_code` for now) and the connection `conn` with which the query was made.

See here for an example of [boost::asio::yield_context](https://github.com/yandex/ozo/blob/master/examples/request.cpp).

`for(auto& row : rows)` - This portion of the example executes if there is no error, and stands for the operations that you want your code to do when there is no error, such as printing out the contents of the output container. In this case, since `rows` is an `ozo::rows_of`, which is simply a `std::vector<std::tuple<...>>`, we're using a standard range-based for loop to iterate the results, and `std::get<>` to retrieve the columns by number. Nothing special.

---

## How To Properly Handle Errors

OZO uses `boost::system::error_code` to indicate an error and therefore, like `std::error_code`, it cannot provide any context-dependent information at all. So unfortunately `error_code::message()` returns only a static textual description of the code. But this is not enough, especially for sql errors. That's why the additional error information is needed and can be obtained via these two functions:

* `ozo::error_message()` - provides a `std::string_view` with native error message from libpq,
* `ozo::get_error_context()` - provides an additional error context from OZO.

Please learn more about these function in the documentation.

**Example**:

```cpp
void print_error (std::ostream& s, ozo::error_code ec) {
    s << "Request failed with error: " << ec.message();
    // Here we should check if the connection is in null state to avoid UB.
    if (!ozo::is_null_recursive(connection)) {
        // Let's check libpq native error message and if so - print it out
        if (auto msg = ozo::error_message(connection); !msg.empty()) {
            s << ", error message: " << msg;
        }
        // Sometimes libpq native error message is not enough, so let's check
        // the additional error context from OZO
        if (auto ctx = ozo::get_error_context(connection); !ctx.empty()) {
            s << ", additional error context: " << ctx;
        }
    }
}
```

---

## How To Map Column Names to Column Numbers At Compile Time

In in the first example we saw how to do a simple request, but in that example we needed to map data from the database to our container object strictly based on the column order:

```cpp
ozo::rows_of<std::int64_t, std::optional<std::string>> rows;
std::int64_t myParam = ...;
using namespace ozo::literals;
const auto query = "SELECT id, name FROM users_info WHERE amount>="_SQL + myParam;
```

If we interchange `id` and `name` in the query text we will get a run-time error. To be more robust to changes in the ordering of columns returned from the database, we can use Boost.Fusion to map column names to positions.

```cpp
BOOST_FUSION_DEFINE_STRUCT((), my_row,
    (std::int64_t, id)
    (std::optional<std::string>, name)
)

int main() {

    std::vector<my_row> rows;

    std::int64_t myParam = ...;
    // Note: We've switched `name` and `id`!
    const auto query = "SELECT name, id FROM users_info WHERE amount>="_SQL + myParam;

    ozo::request(conn_info[io], query, ozo::into(rows),
            [&](ozo::error_code ec, auto conn) {
        // When this callback is entered, the "rows" variable has been populated with the results
        // of the query, unless there was some problem, as indicated by the error code.
    });

}
```

In this example, you can change the order of the fields in either `my_row` or your database query freely, as the fields are identified based on their name, and not based on their index in the tuple. Of course, if you change the names of the fields in your database, you'll need to change the naming you use in your C++ code as well!

---
## How To Determine Which C++ Type You Need To Use For A PostgreSQL Type

Since we are using a binary protocol and have a serialization/deserialization system we also have a type system. This type system is extensible so you can add your own types at compile time. For built-in types you can look at [ozo/pg/types](../include/ozo/pg/types) for definitions like:

```cpp
#include <ozo/pg/definitions.h>
#include <string>

OZO_PG_BIND_TYPE(std::string, "text")
```

`OZO_PG_BIND_TYPE(CPP_TYPE, PG_TYPE)` defines a C++ to built-in PostgreSQL type mapping. Its arguments are:

* `CPP_TYPE` - C++ type.
* `PG_TYPE` - PostgreSQL type name (as a raw string literal).

It defines a type mapping from the C++ type to the specified built-in PostgreSQL type. This mapping also adds compatibility for the various array representations supported by OZO. The current array represantations in OZO are std::vector, std::array, std::list, so for a type that you bind to `std::string`, you can use `std::vector<std::string>`, `std::array<std::string, N>`, `std::list<std::string>` in your queries.

This mapping is many-to-one, allowing you to map multiple C++ types to the same PostgreSQL type. For example, OZO has the built-in mapping for the `text` type like such:

```cpp
OZO_PG_BIND_TYPE(std::string, "text")
```

Suppose you have a custom string representation, `Stroka`. OZO needs you to define some helper free-functions that can be called via argument-dependent-lookup in order to support OZO's default introspection mechanisms. The signature for these functions looks like the following:

```cpp
const char* data(const Stroka&);
const char* size(const Stroka&);
```

Once you have those helper functions, you can bind your custom type `Stroka` to the PostgreSQL type `"text"` like so:

```cpp
OZO_PG_BIND_TYPE(Stroka, "text")
```

Once you've done that, you can use `Stroka` in your queries directly, without needing to convert to another type (such as `std::string`) first.

---

## How To Bind Multiple PostgreSQL Types To The Same C++ Type

A lot of PostgreSQL types can be represented via the same C++ types. E.g. `text`, `name`, `varchar` can be represented via `std::string`. However, if you defined mappings between `std::string` and multiple postgresql types, OZO can't know out of the box which PostgreSQL data type you mean when you provide an `std::string`. You can map `text` to several C++ types but you can not map several PostgreSQL types to a single C++ type. There is a solution for the problem in the library - `OZO_STRONG_TYPEDEF`. This macro allows you to define an alias on a type with strong type definition guarantee.

E.g. alias and definition for `bytea` type may look like this:

```cpp
OZO_STRONG_TYPEDEF(std::string, bytea)
//...
OZO_PG_BIND_TYPE(bytea, "bytea")
```

You can access the underlying type via conversion operator or `get()` method:

```cpp
bytea wrapped_value;

auto n = static_cast<const std::string&>(wrapped_value).find(' ');
// or
auto n = wrapped_value.get().find(' ');
// or
bool is_good_string(const std::string& v);
//...
bool flag = is_good_string(wrapped_value);
```
