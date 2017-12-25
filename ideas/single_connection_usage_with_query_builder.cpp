struct Sum {
    int value;
};

boost::asio::io_service io_service;

boost::asio::spawn(io_service, [&] (boost::asio::yield_context yield) {
    auto connection_provider = apq::make_connection_provider(io_service);
    apq::cursor<Sum> cursor;
    auto query = "SELECT ("_SQL + 13 + "::integer + "_SQL + 42 + "::integer) value"_SQL;
    apq::binary_protocol::request(connection_provider, query, cursor, yield);
    Sum sum;
    apq::binary_protocol::fetch(cursor, sum, yield);
    std::cout << sum.value << '\n';
});

io_service.run();
