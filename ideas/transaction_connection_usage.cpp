boost::asio::io_service io_service;

boost::asio::spawn(io_service, [&] (boost::asio::yield_context yield) {
    auto connection_provider = apq::make_connection_provider(io_service);
    auto connection = apq::text_protocol::begin(connection_provider, yield);
    apq::text_protocol::execute(connection, "CREATE TABLE foo (bar integer);", yield);
    apq::text_protocol::commit(connection, yield);
});

io_service.run();
