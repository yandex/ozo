boost::asio::io_service io_service;

boost::asio::spawn(io_service, [&] (boost::asio::yield_context yield) {
    auto connection_provider = apq::make_connection_provider(io_service);
    apq::vector<apq::row> buffer;
    apq::text_protocol::request(connection_provider, "SELECT unnset(ARRAY[1, 2, 3])", std::back_inserter(buffer), yield);
    for (const auto& row : buffer) {
        std::cout << row.at(0) << '\n';
    }
});

io_service.run();
