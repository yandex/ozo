# How to

Here are some examples of how to use OZO API.

<!-- TOC -->
- [How to](#how-to)
  - [How To Make A Very Simple request](#how-to-make-a-very-simple-request)
  - [How To Handle Error Properly](#how-to-handle-error-properly)
  - [How Start To Do Not Bother With Types Sequence In Row But Begin To Bother About Column Names](#how-start-to-do-not-bother-with-types-sequence-in-row-but-begin-to-bother-about-column-names)
  - [How To Determine Which Type Do I Need To Use For The PostgreSQL Type](#how-to-determine-which-type-do-i-need-to-use-for-the-postgresql-type)
  - [How To Bind One More PostgreSQL Type For C++ Type With Existing Binding](#how-to-bind-one-more-postgresql-type-for-c-type-with-existing-binding)

## How To Make A Very Simple request

E.g. you have _very_ simple table.

```sql
CREATE TABLE users_info(
    id          bigint  NOT NULL,
    name        text,
    amount      bigint  NOT NULL
);
```

If you want to execute a query with no custom types or something else then the simplest way for you is:

```cpp
#include <ozo/request.h>
#include <ozo/connection_info.h>
#include <ozo/shortcuts.h>
#include <boost/asio.hpp>

int main() {
    // We need io_context for IO
    boost::asio::io_context io;

    // Container of rows which accepts integer and nullable string columns in the sequence.
    // (This is an alias on std::vector of std::tuple - see the documentation)
    ozo::rows_of<std::int64_t, std::optional<std::string>> rows;

    // Connection info with host and port to connect to
    auto conn_info = ozo::make_connection_info("host=... port=...");

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

        // We got results, let's handle, e.g. print it out
        std::cout << "id" << '\t' << "name" << std::endl;
        for(auto& row: res) {
            std::cout << std::get<0>(row) << '\t' << std::get<1>(row) << std::endl;
        }
    });

    io.run();
}
```

Let's look a little bit closer on this pretty simple but asynchronous code.

```cpp
ozo::rows_of<std::int64_t, std::optional<std::string>> rows;
```

Here we define a result type. Practically `ozo::rows_of` is an alias on `std::vector<std::tuple<...>>`. And `ozo::into` is an alias on `std::back_inserter`. So `ozo::request()` function will fill this vector of tuples according to the database response rows. Please read the documentation for more details.

It is _very important_ to preserve the same order of fields in request and types in the tuple (it is a little bit annoying, but there is a way to avoid it via [Boost.Hana](https://www.boost.org/doc/libs/1_66_0/libs/hana/doc/html/index.html#tutorial-introspection-adapting) or [Boost.Fusion](https://www.boost.org/doc/libs/1_66_0/libs/fusion/doc/html/fusion/adapted.html) structure adaptation).

There is `std::optional<std::string>` at the second position of the tuple. This is because `name` field of the table can be _NULL_. Empty optional represents the _NULL_ (you can learn more about Nullable concept from the documentation). If you omit an `std::optional` then in case of _NULL_ value deserialization error could be.

The first argument of the tuple can not be a _NULL_ due to the schema, so we can omit `std::optional`.

In other words: there is no mistake to expect nullable type for non-nullable result, but the opposite leads to a run-time error in case of _NULL_ value received from a database.

```cpp
auto conn_info = ozo::make_connection_info("host=... port=...");
```

Now we need to create a connection information for database to connect to. This is our connection source which can create a connection for us as it will be needed (you can learn more about ConnectionSource and ConnectionProvider concepts from the documentation).

```cpp
const auto query = "SELECT id, name FROM users_info WHERE amount>="_SQL + std::int64_t(25);
```

Here our query for database. There is a text with `_SQL` literal and a parameter `std::int64_t(25)`. Note that the parameter will be passed as a separate binary parameter, but not as part of the query text.

Here is `request()` asynchronous function call.

```cpp
ozo::request(ozo::make_connector(conn_info, io), query, ozo::into(res),
        [&](ozo::error_code ec, auto conn) {
//...
});
```

`ozo::make_connector(conn_info, io)` - the first parameter is a **ConnectionProvider** or a **Connection**. **ConnectionProvider** is an entity from which you can get a new (or may be already established) connection. **Connection** is a **ConnectionProvider** since it can provide itself. So the query request will be performed within connection obtained for the first argument.

`query` - the next argument is query which we discussed above.

`ozo::into(res)` - the output parameter. In this case the out parameter is a back insert iterator to the result vector. Note, that the life time of the output parameter should be managed by a user. In this example it correctly placed on stack since its lifetime overlaps `io.run()` call. The query output parameter can be iterator with appropriated value type, or `ozo::result` object which provides access to raw binary data. The second variant is not recommended since user should implement binary protocol parsing by self.

`[&](ozo::error_code ec, auto conn)` - completion token parameter, in this case is callback lambda. In other cases it can be [boost::asio::use_future](https://www.boost.org/doc/libs/1_67_0/doc/html/boost_asio/reference/use_future.html), [boost::asio::yield_context](https://www.boost.org/doc/libs/1_67_0/doc/html/boost_asio/reference/yield_context.html) or any other [boost::asio::async_result](https://www.boost.org/doc/libs/1_67_0/doc/html/boost_asio/reference/async_result.html) compatible [Completion Token](https://www.boost.org/doc/libs/1_67_0/doc/html/boost_asio/reference/async_completion.html). The arguments of the call back are error code `ec` (which is namely `boost::system::error_code` for now) and the connection `conn` with which the query was made.

`for(auto& row: res)` - so if there is no error we can handle result from the output container.

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

## How Start To Do Not Bother With Types Sequence In Row But Begin To Bother About Column Names

In previous story we saw how to do a simple request. But there is a tricky thing with sequence of types in result:
```cpp
ozo::rows_of<std::int64_t, std::optional<std::string>> rows;

//...

const auto query = "SELECT id, name FROM users_info WHERE amount>="_SQL + std::int64_t(25);
```

If we interchange `id` and `name` in the query text we will get a run-time error. To be more robust for such changes there is another way - adaptation. E.g. with Boost.Fusion.

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

In this case you can change fields' places free. Fields and structure members are bound with their names.

---

## How To Determine Which Type Do I Need To Use For The PostgreSQL Type

It is a good question! Since we are using binary protocol and have serialization/deserialization system we also have a type system. It is easy and extandable. For build-in types you can look at [ozo/type_traits.h](../include/ozo/type_traits.h) for difinitions like:

```cpp
OZO_PG_DEFINE_TYPE(std::vector<char>, "bytea", BYTEAOID, dynamic_size)

OZO_PG_DEFINE_TYPE_AND_ARRAY(boost::uuids::uuid, "uuid", UUIDOID, 2951, bytes<16>)
```

`OZO_PG_DEFINE_TYPE(CPP_TYPE, PG_TYPE, OID, SIZE)` defines a C++ to built-in PostgreSQL type mapping. It's arguments are:

* `CPP_TYPE` - C++ type.
* `PG_TYPE` - PostgreSQL type name.
* `OID` - type OID in PostgreSQL.
* `SIZE` - type size which can be static for N bytes `bytes(N)`, or dynamic `dynamic_size`, like in this case.

`OZO_PG_DEFINE_TYPE_AND_ARRAY(CPP_TYPE, PG_TYPE, OID, ARRAY_OID SIZE)` defines  C++ to built-in PostgreSQL type mapping with it's array. The current array represantation in OZO is std::vector, so for `uuid` type it will be `std::vector<uuid>`.

Since the mapping is ***C++** to **PostgreSQL***, you can extend it with your own types.

E.g. we have a `text` type definition like:

```cpp
OZO_PG_DEFINE_TYPE_AND_ARRAY(std::string, "text", TEXTOID, TEXTARRAYOID, dynamic_size)
```

You have your own brilliant string representation `Stroka` compatible with `const char* data(const Stroka&)` and `const char* size(const Stroka&)` functions (it is needed to use default introspection mechanisms). It can be included in type system and used as `text` representation like this:

```cpp
OZO_PG_DEFINE_TYPE_AND_ARRAY(Stroka, "text", TEXTOID, TEXTARRAYOID, dynamic_size)
```

Now you can get `text` into `Stroka` type, and put `Stroka` object like text in queries.

---

## How To Bind One More PostgreSQL Type For C++ Type With Existing Binding

This is very common problem. A lot of PostgreSQL types can be represented via the same C++ types. E.g. `text`, `name`, `varchar` can be easily and convenient represented via `std::string`. But it can not be done, due to relation "one to many" between PostgreSQL type and C++ types. You can map `text` to several C++ types but you can not map several PostgreSQL types to a single C++ type. There is a solution for the problem in the library - `OZO_STRONG_TYPEDEF`. This macro allows you to define an alias on a type with strong type definition guarantee.

E.g. alias and definition for `bytea` type may looks like this:

```cpp
OZO_STRONG_TYPEDEF(std::string, bytea)
//...
OZO_PG_DEFINE_TYPE_AND_ARRAY(bytea, "bytea", BYTEAOID, 1001, dynamic_size)
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
