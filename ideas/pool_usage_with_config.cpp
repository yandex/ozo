boost::asio::io_service io_service;

apq::connection_pool_config config;
config.connection.conninfo = "host=mydb01 user=poller";
config.idle_timeout = 100s;

apq::connection_pool pool(config);

boost::asio::spawn(io_service, [&] (boost::asio::yield_context yield) {
    while (true) {
        auto connection_provider = apq::make_connection_provider(io_service, pool);
        auto result = apq::text_protocol::request(connection_provider, "SELECT pg_is_in_recovery()", yield);
        std::cout << "is it replica? " <<  result.begin()->at(0) << '\n';
        boost::asio::deadline_timer timer(io_service, 1s);
        timer.async_wait(yield);
    }
});

io_service.run();
