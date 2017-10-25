boost::asio::io_service io_service;
connection_config config;
config.conninfo = "host=mydb01 user=poller";
config.timeouts.connect = 100ms;
config.timeouts.request = 200ms;

boost::asio::spawn(io_service, [&] (boost::asio::yield_context yield) {
    auto connection_provider = apq::make_connection_provider(io_service, config);
    apq::cursor<apq::row> cursor;
    apq::text_protocol::request(connection_provider, cursor, "SELECT pg_is_in_recovery()", yield);
    apq::row row;
    apq::text_protocol::fetch(cursor, row, yield);
    std::cout << row.at(0) << '\n';
});

io_service.run();
