struct Summands {
    int first;
    int second;
};

boost::asio::io_service io_service;

boost::asio::spawn(io_service, [&] (boost::asio::yield_context yield) {
    auto connection_provider = apq::make_connection_provider(io_service);
    apq::cursor cursor;
    Summand summands;
    summands.first = 13;
    summands.second = 42;
    auto query = apq::query("SELECT :first + :second", summands);
    apq::text_protocol::request(connection_provider, cursor, query, yield);
    std::cout << cursor.next(yield).at(0) << '\n';
});

io_service.run();
