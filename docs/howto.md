# How to

Here are some examples of how to use OZO API.

<!-- TOC -->
- [How To Make A Very Simple Request](#making-a-request-to-a-postgresql-database)
- [How To Handle Error Properly](#How-To-Handle-Error-Properly)
- [How To Map Column Names to Column Numbers At Compile Time](#How-To-Map-Column-Names-to-Column-Numbers-At-Compile-Time)
- [How To Determine Which Type Do I Need To Use For The PostgreSQL Type](#How-To-Determine-Which-Type-Do-I-Need-To-Use-For-The-PostgreSQL-Type)
- [How To Bind One More PostgreSQL Type For C++ Type With Existing Binding](#How-To-Bind-One-More-PostgreSQL-Type-For-C-Type-With-Existing-Binding)

## Making a request to a database

This section explains how to execute an SQL query without using custom types.

Sample table contains three fields: `id`, `name` (it can contain the _NULL_ value), and `amount`:

```sql
CREATE TABLE users_info(
    id          bigint  NOT NULL,
    name        text,
    amount      bigint  NOT NULL
);
```

Follow these steps to make a request:

1. [Define the result type](#step-1-define-the-result-type)
2. [Create a connection](#step-2-create-a-connection)
3. [Specify an SQL query](#step-3-specify-an-sql-query)
4. [Make a request to database](#step-4-make-a-request-and-save-the-result)

### Step 1. Define the result type

Define the type to write into the query result:

```cpp
// Add the container which accepts an integer and nullable string columns.
ozo::rows_of<std::int64_t, std::optional<std::string>> rows;
```

Pay attention to the following:

- `rows_of` is an alias for `std::vector<std::tuple<...>>` to store data rows.
- Use the `std::optional<std::string>` wrapper type for the nullable type. It helps to avoid a deserialization error if a _NULL_ value is retrieved from the database.

> It is important to preserve the same field order in the request and the result type.
>
> You can also use structure adaptation. Read more about adapting in the [Boost.Hana](https://www.boost.org/doc/libs/1_66_0/libs/hana/doc/html/index.html#tutorial-introspection-adapting) and [Boost.Fusion](https://www.boost.org/doc/libs/1_66_0/libs/fusion/doc/html/fusion/adapted.html) documentation.

### Step 2. Create a connection

Create a connection to a database using the `make_connection_info()` function:

```cpp
// Specify the connection info with host and port to connect to.
auto conn_info = ozo::make_connection_info("host=... port=...");
```
> You can also pass a connection URI string as an argument.

### Step 3. Specify an SQL query 

Specify the SQL query as a string with the `_SQL` suffix. The suffix converts query string to OZO query data type.

Add the `std::int64_t(25)` parameter to the query. The parameter is not a part of the query text.

```cpp
// Add namespace for the _SQL literal.
using namespace ozo::literals;
// Specify the SQL query with the parameter.
const auto query = "SELECT id, name FROM users_info WHERE amount>="_SQL + std::int64_t(25);
```


### Step 4. Make a request and save the result

#### Specify the request parameters

Invoke the `request()` function to make an asynchronous request. The function adds data from the response to the vector of tuples row-by-row.

```cpp
ozo::request(conn_info[io],  query, ozo::into(res),
        [&](ozo::error_code ec, auto conn) {
        //...
});
```

Pass the following parameters to the `request()` function:
- `conn_info[io]` — the connection info. **Connection** is a **ConnectionProvider** since it can provide itself. So the query request is performed within connection obtained for the first argument.
- `query` — the SQL query.
- `ozo::into(rows)`— the output parameter. `ozo::into` is an alias for `std::back_inserter`. You should handle the lifetime of the output parameter manually.  
   In the example, the parameter is placed on stack since its lifetime overlaps `io.run()` call. You can also use `std::make_shared` and then store the resulting shared pointer in your callback function. In this case, the memory (that the `ozo::into` function writes to) stays valid until the callback function finishes.
   > The query output parameter can be an iterator with an appropriate value type or an `ozo::result` object which provides access to raw binary data. It is not recommended since the user needs to implement binary protocol parsing.

- `[&](ozo::error_code ec, auto conn)` — completion function parameter. It is a callback lambda. The arguments of the callback are an error code `ec` (`boost::system::error_code`) and the connection `conn`.
  > Instead of a callback lambda, you can also use [boost::asio::use_future](https://www.boost.org/doc/libs/1_67_0/doc/html/boost_asio/reference/use_future.html), [boost::asio::yield_context](https://www.boost.org/doc/libs/1_67_0/doc/html/boost_asio/reference/yield_context.html) or any other compatible concept, such as: [boost::asio::async_result](https://www.boost.org/doc/libs/1_67_0/doc/html/boost_asio/reference/async_result.html), [Completion Token](https://www.boost.org/doc/libs/1_67_0/doc/html/boost_asio/reference/async_completion.html).

#### Handle the error

Handle the errors:

```cpp
// Handle the error and output error messages:
if (ec) {
    // Output error code message.
    std::cerr << ec.message();
    // Output error message from underlying libpq.
    std::cerr << " | " << ozo::error_message(conn);
    // Output additional error context from OZO.
    if (!ozo::is_null_recursive(conn)) {
        std::cerr << " | " << ozo::get_error_context(conn);
    }
    return;
};
```

The sample also outputs the error messages from the libpq and ozo libraries.

#### Output the result

Output the query result row-by-row using the `for` loop:

```cpp
// Output obtained results using the `for` loop.
std::cout << "id" << '\t' << "name" << std::endl;
for(auto& row: res) {
    std::cout << std::get<0>(row) << '\t' << std::get<1>(row) << std::endl;
}
```

### Code sample

```cpp
#include <ozo/request.h>
#include <ozo/connection_info.h>
#include <ozo/shortcuts.h>
#include <boost/asio.hpp>

int main() {
    // Define the `io_context` to perform asyncronous operations.
    boost::asio::io_context io;

    // Add the container which accepts an integer and nullable string columns.
    ozo::rows_of<std::int64_t, std::optional<std::string>> rows;

    // Set the connection info: the host and port to connect to.
    auto conn_info = ozo::make_connection_info("host=... port=...");

    // Add a namespace for _SQL literal.
    using namespace ozo::literals;
    // Specify the SQL query.
    const auto query = "SELECT id, name FROM users_info WHERE amount>="_SQL + std::int64_t(25);

    // Make a request with connection provider, query and callback.
    ozo::request(conn_info[io], query, ozo::into(rows),
            [&](ozo::error_code ec, auto conn) {
        // Handle the error and output error messages:
        if (ec) {
            // Output error code message.
            std::cerr << ec.message();
            // Output error message from underlying libpq.
            std::cerr << " | " << ozo::error_message(conn);
            // Output additional error context from OZO.
            if (!ozo::is_null_recursive(conn)) {
                std::cerr << " | " << ozo::get_error_context(conn);
            }
            return;
        };

        // Check the connection using the `connection_good` function.
        assert(ozo::connection_good(conn));

        // Output obtained results using the `for` loop.
        std::cout << "id" << '\t' << "name" << std::endl;
        for(auto& row: rows) {
            std::cout << std::get<0>(row) << '\t' << std::get<1>(row) << std::endl;
        }
    });

    io.run();
}
```
---

## How To Handle Error Properly

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

//...

const auto query = "SELECT id, name FROM users_info WHERE amount>="_SQL + std::int64_t(25);
```

If we interchange `id` and `name` in the query text we will get a run-time error. To be more robust to changes in the ordering of columns returned from the database, we can use Boost.Fusion to map column names to positions.

```cpp
BOOST_FUSION_DEFINE_STRUCT((), my_row,
    (std::int64_t, id)
    (std::optional<std::string>, name)
)

int main() {

//...

    std::vector<my_row> res;

//...

    const auto query = "SELECT id, name FROM users_info WHERE amount>="_SQL + std::int64_t(25);

    ozo::request(ozo::make_connector(io, conn_info), query, ozo::into(res),
            [&](ozo::error_code ec, auto conn) {
    //...
    });

}
```

In this example, you can change the order of the fields in either `my_row` or your database query freely, as the fields are identified based on their name, and not based on their index in the tuple.

---

## How To Determine Which Type Do I Need To Use For The PostgreSQL Type

It is a good question! Since we are using binary protocol and have serialization/deserialization system we also have a type system. It is easy and extandable. For build-in types you can look at [ozo/pg/types](../include/ozo/pg/types) for difinitions like:

```cpp
#include <ozo/pg/definitions.h>
#include <string>

OZO_PG_BIND_TYPE(std::string, "text")
```

`OZO_PG_BIND_TYPE(CPP_TYPE, PG_TYPE)` defines a C++ to built-in PostgreSQL type mapping. It's arguments are:

* `CPP_TYPE` - C++ type.
* `PG_TYPE` - PostgreSQL type name.

It defines C++ to built-in PostgreSQL type mapping with it's array. The current array represantation in OZO are std::vector, std::array, std::list, so for `std::string` type it could be `std::vector<std::string>`, `std::array<std::string, N>`, `std::list<std::string>`.

Since the mapping is many **C++** types to one **PostgreSQL** type, you can always map another C++ type as PostgreSQL type.

E.g. we have an in-library `text` type definition like:

```cpp
OZO_PG_BIND_TYPE(std::string, "text")
```

You have your own brilliant string representation `Stroka` compatible with `const char* data(const Stroka&)` and `const char* size(const Stroka&)` functions (it is needed to use default introspection mechanisms). It can be included in type system and used as `text` representation like this:

```cpp
OZO_PG_BIND_TYPE(Stroka, "text")
```

Now you can get `text` into `Stroka` type, and put `Stroka` object like text in queries.

---

## How To Bind One More PostgreSQL Type For C++ Type With Existing Binding

This is very common problem. A lot of PostgreSQL types can be represented via the same C++ types. E.g. `text`, `name`, `varchar` can be easily and convenient represented via `std::string`. But it can not be done, due to relation "one to many" between PostgreSQL type and C++ types. You can map `text` to several C++ types but you can not map several PostgreSQL types to a single C++ type. There is a solution for the problem in the library - `OZO_STRONG_TYPEDEF`. This macro allows you to define an alias on a type with strong type definition guarantee.

E.g. alias and definition for `bytea` type may looks like this:

```cpp
OZO_STRONG_TYPEDEF(std::string, bytea)
//...
OZO_PG_BIND_TYPE(bytea, "bytea")
```

You can access for the underlying type via conversion operator or `get()` method:

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
