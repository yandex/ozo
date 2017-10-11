boost::asio::io_service io_service;

boost::asio::spawn(io_service, [&] (boost::asio::yield_context yield) {
    auto connection = apq::make_connection_provider(io_service);
    apq::cursor cursor;
    connection_timeouts timeouts;
    timeouts.connect = 100ms;
    timeouts.request = 200ms;
    apq::text_protocol::request(connection, cursor, timeouts, "SELECT pg_is_in_recovery()", yield);
    std::cout << cursor.next(yield).at(0) << '\n';
});

io_service.run();
