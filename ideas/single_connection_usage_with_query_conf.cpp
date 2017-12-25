// query_conf.sql content:
// -- name: sum_integers
// SELECT :first::integer + :second::integer
//

using namespace boost::hana::literals;

struct Summands {
    int first;
    int second;
};

struct SumIntegers {
    static constexpr auto name = "sum_integers"_s;
    using arguments_type = Summands;
};

const auto query_repository = apq::register_queries<SumIntegers>(std::ifstream("query_conf.sql"));

boost::asio::io_service io_service;

boost::asio::spawn(io_service, [&] (boost::asio::yield_context yield) {
    auto connection_provider = apq::make_connection_provider(io_service);
    apq::cursor cursor;
    Summands summands;
    summands.first = 13;
    summands.second = 42;
    auto query = apq::make_query<SumIntegers>(query_repository, summands);
    apq::text_protocol::request(connection_provider, cursor, query, yield);
    apq::row row;
    apq::text_protocol::fetch(cursor, row, yield);
    std::cout << row.at(0) << '\n';
});

io_service.run();
