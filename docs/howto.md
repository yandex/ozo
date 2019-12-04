# How to

Here are some examples of how to use OZO API.

<!-- TOC -->
- [How to](#how-to)
  - [How To Make A Very Simple Request](#how-to-make-a-very-simple-request)
  - [How To Handle Error Properly](#how-to-handle-error-properly)
  - [How To Map Column Names to Column Numbers At Compile Time](#how-to-map-column-names-to-column-numbers-at-compile-time)
  - [How To Determine Which Type Do I Need To Use For The PostgreSQL Type](#how-to-determine-which-type-do-i-need-to-use-for-the-postgresql-type)
  - [How To Bind One More PostgreSQL Type For C++ Type With Existing Binding](#how-to-bind-one-more-postgresql-type-for-c-type-with-existing-binding)

## How To Make A Very Simple Request

E.g. you have _very_ simple table.

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
    ozo::connection_info conn_info("host=... port=...");

    // For _SQL literal
    using namespace ozo::literals;
    // Our query statement
    const auto query = "SELECT id, name FROM users_info WHERE amount>="_SQL + std::int64_t(25);

    // Request with connection provider, query and callback.
    ozo::request(ozo::make_connector(conn_info, io), query, ozo::into(rows),
            [&](ozo::error_code ec, auto conn) {
        // Here we got an error, so we can get:
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
        // typically you do not need to check it manually
        assert(ozo::connection_good(conn));

        // We got results, let's do something with them, e.g. print them out
        std::cout << "id" << '\t' << "name" << std::endl;
        for(auto& row: res) {
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

Here we define a result type. `ozo::rows_of` is an alias of `std::vector<std::tuple<...>>`. And `ozo::into` is an alias of `std::back_inserter`. So `ozo::request()` function will fill this vector of tuples by back inserting data from database's response, row by row. Please read the documentation for more details.

It is _very important_ to preserve the same order of fields in the request and the types in the tuple (it is a little bit annoying, but there is a way to avoid it via [Boost.Hana](https://www.boost.org/doc/libs/1_66_0/libs/hana/doc/html/index.html#tutorial-introspection-adapting) or [Boost.Fusion](https://www.boost.org/doc/libs/1_66_0/libs/fusion/doc/html/fusion/adapted.html) structure adaptation).

Notice that the second position of the tuple is `std::optional<std::string>`.This is because the `name` field of the table can be _NULL_. Empty optional represents a _NULL_ value (you can learn more about Nullable concept from the documentation). If if a _NULL_ value is retrieved from the database for a type that is not an `std::optional`, then a deserialization error will occur.

Note that in the example, the table is defined with the first (id) and third (amount) columns as _NOT NULL_. This means that for queries retrieving those columns, it's not necessary to use `std::optional` for those fields. However, the second (name) column is _NULL_, and therefore must be an `std::optional` as explained in the paragraph above.

Note that it's acceptable to provide an `std::optional` for a _NOT NULL_ field, but it is not acceptable to omit the `std::optional` for a field that is not _NOT NULL_, unless you can gaurentee the retrieved data will not be _NULL_. Failure to properly use `std::optional` will lead to a run-time error in case of a _NULL_ value received from a database.

```cpp
ozo::connection_info conn_info("host=... port=...");
```

Now we need to create a connection information for database to connect to. This is our connection source which can create a connection for us as it will be needed (you can learn more about ConnectionSource and ConnectionProvider concepts from the documentation). It's also acceptable to provide a connection URI string, instead of the comma seperated version.

```cpp
const auto query = "SELECT id, name FROM users_info WHERE amount>="_SQL + std::int64_t(25);
```

Here's our database query. The `_SQL` suffix is a user defined literal that converts the string that it is attached to into OZO's query data type. The parameter `std::int64_t(25)` is then added to the query accordingly. Note that the parameter will be passed as a separate binary parameter, but not as part of the query text.

Here is the asynchrounous function call `request()`.

```cpp
ozo::request(ozo::make_connector(conn_info, io), query, ozo::into(res),
        [&](ozo::error_code ec, auto conn) {
//...
});
```

`ozo::make_connector(conn_info, io)` - the first parameter is a **ConnectionProvider** or a **Connection**. **ConnectionProvider** is an entity from which you can get a new (or already established) connection. **Connection** is a **ConnectionProvider** since it can provide itself. So the query request will be performed within connection obtained for the first argument.

`query` - the next argument is query which we discussed above.

`ozo::into(res)` - the output parameter. In this case the out parameter is a back insert iterator to the result vector. Note, that the life time of the output parameter should be managed by a user. In this example it correctly placed on stack since its lifetime overlaps `io.run()` call. Another way you can do this is to use `std::make_shared`, and then store the resulting shared pointer in your callback function. That way the memory that `ozo::into` is writing into will stay valid until the callback function finishes.

The query output parameter can be an iterator with appropriate value type, or an `ozo::result` object which provides access to raw binary data. The second variant is not recommended since user would need to implement binary protocol parsing.

`[&](ozo::error_code ec, auto conn)` - completion function parameter, in this example it is a callback lambda. In other cases it can be [boost::asio::use_future](https://www.boost.org/doc/libs/1_67_0/doc/html/boost_asio/reference/use_future.html), [boost::asio::yield_context](https://www.boost.org/doc/libs/1_67_0/doc/html/boost_asio/reference/yield_context.html) or any other compatible concept, such as: [boost::asio::async_result](https://www.boost.org/doc/libs/1_67_0/doc/html/boost_asio/reference/async_result.html), [Completion Token](https://www.boost.org/doc/libs/1_67_0/doc/html/boost_asio/reference/async_completion.html). The arguments of the call back are an error code `ec` (which is namely `boost::system::error_code` for now) and the connection `conn` with which the query was made.

`for(auto& row: res)` - This portion of the example executes if there is no error, and stands for the operations that you want your code to do when there is no error, such as printing out the contents of the output container.

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
struct my_row {
    std::int64_t id;
    std::optional<std::string> name;
};

BOOST_HANA_ADAPT_STRUCT(my_row, id, name);

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

 > **Note:** you may use `BOOST_FUSION_ADAPT_STRUCT` and all of the `Boost.Fusion` adaptation mechanisms for structures, but it is recommended to use `Boost.Hana` for a faster and more modern approach.

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
