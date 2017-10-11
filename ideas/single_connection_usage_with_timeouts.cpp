boost::asio::io_service io_service;

boost::asio::spawn(io_service, [&] (boost::asio::yield_context yield) {
    auto connection = apq::make_connection_provider(io_service);
    connection_timeouts timeouts;
    timeouts.connect = 100ms;
    timeouts.request = 200ms;
    auto result = apq::text_protocol::request(connection, timeouts, "SELECT pg_is_in_recovery()", yield);
    std::cout << "is it replica? " << result.begin()->at(0) << '\n';
});

io_service.run();
