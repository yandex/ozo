boost::asio::io_service io_service;

boost::asio::spawn(io_service, [&] (boost::asio::yield_context yield) {
    auto connection_provider = apq::make_connection_provider(io_service);
    apq::cursor<apq::row> cursor;
    apq::text_protocol::request(connection_provider, "SELECT unnest(ARRAY[1, 2, 3])", cursor, yield);
    for (const auto& row : apq::range(cursor, yield)) {
        std::cout << row.at(0) << '\n';
    }
});

io_service.run();
