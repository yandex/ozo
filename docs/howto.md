# How to

Here are some examples of how to use OZO API.

## How To Make A Very Simple request

E.g. you have _very_ simple table.

```sql
CREATE TABLE users_info(
    id          bigint  NOT NULL,
    name        text,
    amount      bigint  NOT NULL
);
```

If you want execute just a single query with no custom types or something else then the simplest way for you is:

```cpp
#include <ozo/request.h>
#include <ozo/connection_info.h>
#include <ozo/shortcuts.h>
#include <boost/asio.hpp>

int main() {
    // We need io_context for IO, this is alias on boost::asio::io_context
    boost::asio::io_context io;

    // Rows which accepts integer and nullable string columns in the sequence
    // It is an alias on std::vector of std::tuple
    ozo::rows_of<std::int64_t, std::optional<std::string>> rows;

    // Connection info with host and port to coonect to
    auto conn_info = ozo::make_connection_info("host=... port=...");

    // For _SQL literal
    using namespace ozo::literals;
    // Our query statement
    const auto query = "SELECT id, name FROM users_info WHERE amount>="_SQL + std::int64_t(25);

    // Request with connection provider, query and callback.
    // Provider binds how to get connection with io_context.
    ozo::request(ozo::make_connector(io, conn_info), query, ozo::into(rows),
            [&](ozo::error_code ec, auto conn) {
        if (ec) {
            // Here we got an error, so we can get:
            //           error code's message
            std::cerr << ec.message()
            //           error message from underlying libpq
                    << " | " << error_message(conn)
            //           and error context from OZO
                    << " | " << get_error_context(conn);
            return;
        };

        // Connection must be in good state here,
        // typically you do not need to check it manually
        assert(ozo::connection_good(conn));

        // We got results, let's handle, e.g. print it out
        std::cout << "id" << '\t' << "name" << std::endl;
        for(auto& row: res) {
            std::cout << std::get<0>(row) << '\t'
                    << std::get<1>(row) << std::endl;
        }
    });

    io.run();
}
```

Let's look a little bit closer on this pretty simple asynchronous code.

```cpp
ozo::rows_of<std::int64_t, std::optional<std::string>> rows;
```

Here we define result type as we want to see it. Practically `ozo::rows_of` is an alias on `std::vector<std::tuple<...>>`. And `ozo::into` is an alias on `std::back_inserter`. So `request()` function will fill this vector of tuples according to the database response rows.

It is _very important_ to preserve the same order of fields in request and types in the tuple (it is a little bit annoying, but there is a way to avoid it via [Boost.Hana](https://www.boost.org/doc/libs/1_66_0/libs/hana/doc/html/index.html#tutorial-introspection-adapting) or [Boost.Fusion](https://www.boost.org/doc/libs/1_66_0/libs/fusion/doc/html/fusion/adapted.html) structure adaptation).

There is `std::optional<std::string>` at the second position of the tuple. This is because `name` field of the table can be _NULL_. Empty optional represens the _NULL_ (learn more about Nullable concept). If you ommit an `std::optional` then in case of _NULL_ value run-time result deserialization error will happend.

The first argument of the tuple can not be a _NULL_, so here we do not to bother about it and can ommit `std::optional`.

There is no mistake to expect nullable type for non-nullable result, but the opposite leads to run-time error in case of _NULL_ value result from database.

```cpp
auto conn_info = ozo::make_connection_info("host=... port=...");
```

Now we need to create a connection information for database to connect to. This is our connection provider which can create a connection for us as it will be needed (see more information about connection provider and how to get connection).

```cpp
const auto query = "SELECT id, name FROM users_info WHERE amount>="_SQL + std::int64_t(25);
```

Here our query for database. There is a text with `_SQL` literal and single parameter `std::int64_t(25)`. Note, what the parameter will be passed as a separate binary parameter but not as a part of query text.

Here is `request()` asynchronous function call.

```cpp
ozo::request(ozo::make_connector(io, conn_info), query, ozo::into(res),
        [&](ozo::error_code ec, auto conn) {
//...
});
```

`ozo::make_provider(io, conn_info)` - the first parameter is a **Connection Provider** or **Connection**. **Connection Provider** is an entity from which you can get a new (or may be already established) connection. **Connection** is a **Connection Provider** since it can provide itself. So the query request will be performed within connection obtained for the first argument.

`query` - the next argument is query which we discussed earler.

`ozo::into(res)` - the output perameter. In this case out parameter is back inserter iterator to the result vector. Note, what the life time of the output parameter managed by the user. In this case it correctly placed on stack since its lifetime overlaps `io.run()` call. But in more sophisticated code with callbacks it needs to be stored e.g. in shared pointer or something like this. The query output parameter can be iterator on container with appropriated data items, or it can be `ozo::result` which provides access to raw binary data. The second variant is not recommended since user must implement binary protocol parsing by self, but if it needed it can be used.

`[&](ozo::error_code ec, auto conn)` - completion token parameter, in this case is callback lambda. In other cases it can be [boost::asio::use_future](https://www.boost.org/doc/libs/1_67_0/doc/html/boost_asio/reference/use_future.html), [boost::asio::yield_context](https://www.boost.org/doc/libs/1_67_0/doc/html/boost_asio/reference/yield_context.html) or any other [boost::asio::async_result](https://www.boost.org/doc/libs/1_67_0/doc/html/boost_asio/reference/async_result.html) compatible [Completion Token](https://www.boost.org/doc/libs/1_67_0/doc/html/boost_asio/reference/async_completion.html). The arguments of the call back are error code `ec` (which is namely `boost::system::error_code` for now) and the connection `conn` with which the query was made. Even if you got an error it is possible what there is an additional error context in the `conn`. Since there is no rooms to place context depended information about connection error into error code the context depended information provided via `error_message()` and `get_error_context()` functions. The first one returns error message from `libpq`, the second - additional context from `OZO`.

`for(auto& row: res)` - so if there is no error we can handle result from the output container.

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

In this case you can change fields' places free. Fields and structure members are bound with thier names.

## How To Determine Which Type I need To Use For The PostgreSQL Type

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
