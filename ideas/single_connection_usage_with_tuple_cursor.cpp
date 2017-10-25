boost::asio::io_service io_service;

boost::asio::spawn(io_service, [&] (boost::asio::yield_context yield) {
    auto connection_provider = apq::make_connection_provider(io_service);
    apq::cursor<std::tuple<std::string>> cursor;
    apq::binary_protocol::request(connection_provider, "SELECT pg_is_in_recovery()", cursor, yield);
    std::tuple<std::string> data;
    apq::binary_protocol::fetch(cursor, data, yield);
    std::cout << std::get<std::string>(data) << '\n';
});

io_service.run();
