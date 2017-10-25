boost::asio::io_service io_service;
apq::connection_pool pool;

boost::asio::spawn(io_service, [&] (boost::asio::yield_context yield) {
    while (true) {
        auto connection_provider = apq::make_connection_provider(io_service, pool);
        apq::cursor cursor;
        connection_pool_timeouts timeouts;
        timeouts.queue = 100ms;
        timeouts.connection.connect = 100ms;
        timeouts.connection.request = 200ms;
        apq::text_protocol::request(connection_provider, cursor, timeouts, "SELECT pg_is_in_recovery()", yield);
        apq::row row;
        apq::text_protocol::fetch(cursor, row, yield);
        std::cout << row.at(0) << '\n';
        boost::asio::deadline_timer timer(io_service, 1s);
        timer.async_wait(yield);
    }
});

io_service.run();
