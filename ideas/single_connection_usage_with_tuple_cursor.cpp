boost::asio::io_service io_service;

boost::asio::spawn(io_service, [&] (boost::asio::yield_context yield) {
    auto connection_provider = apq::make_connection_provider(io_service);
    apq::cursor<std::tuple<std::string>> cursor;
    apq::binary_protocol::request(connection_provider, cursor, "SELECT pg_is_in_recovery()", yield);
    std::cout << std::get<0>(cursor.next(yield)) << '\n';
});

io_service.run();
