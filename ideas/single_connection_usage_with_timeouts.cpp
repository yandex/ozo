boost::asio::io_service io_service;

boost::asio::spawn(io_service, [&] (boost::asio::yield_context yield) {
    auto connection = apq::make_connection_provider(io_service);
    apq::data_row row;
    connection_timeouts timeouts;
    timeouts.connect = 100ms;
    timeouts.request = 200ms;
    if (apq::text_protocol::request(connection, apq::make_query("SELECT pg_is_in_recovery()", timeouts), row, yield)) {
    	std::cout << row.at(0) << '\n';
    }
});

io_service.run();
