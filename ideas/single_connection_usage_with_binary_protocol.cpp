struct Sum {
    int value;
};

boost::asio::io_service io_service;

boost::asio::spawn(io_service, [&] (boost::asio::yield_context yield) {
    auto connection_provider = apq::make_connection_provider(io_service);
    apq::cursor<Sum> cursor;
    auto query = apq::query("SELECT ($1::integer + $2::integer) value", 13, 42);
    apq::binary_protocol::request(connection_provider, cursor, query, yield);
    std::cout << cursor.next(yield).value << '\n';
});

io_service.run();
