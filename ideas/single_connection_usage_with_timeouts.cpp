boost::asio::io_service io_service;

boost::asio::spawn(io_service, [&] (boost::asio::yield_context yield) {
    auto connection = apq::make_connection_provider(io_service);
    apq::cursor<apq::row> cursor;
    connection_timeouts timeouts;
    timeouts.connect = 100ms;
    timeouts.request = 200ms;
    apq::text_protocol::request(connection, apq::make_query("SELECT pg_is_in_recovery()", timeouts), cursor, yield);
    apq::row row;
    apq::text_protocol::fetch(cursor, row, yield);
    std::cout << row.at(0) << '\n';
});

io_service.run();
