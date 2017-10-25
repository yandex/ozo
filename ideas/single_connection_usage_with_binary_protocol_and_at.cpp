boost::asio::io_service io_service;

boost::asio::spawn(io_service, [&] (boost::asio::yield_context yield) {
    auto connection_provider = apq::make_connection_provider(io_service);
    apq::cursor<apq::row> cursor;
    auto query = apq::query("SELECT ($1::integer + $2::integer) value", 13, 42);
    apq::binary_protocol::request(connection_provider, query, cursor, yield);
    apq::row row;
    apq::binary_protocol::fetch(cursor, row, yield);
    std::cout << row.at<int>(0) << '\n';
});

io_service.run();
