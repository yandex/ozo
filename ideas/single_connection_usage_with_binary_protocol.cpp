struct Sum {
    int value;
};

boost::asio::io_service io_service;

boost::asio::spawn(io_service, [&] (boost::asio::yield_context yield) {
    auto connection_provider = apq::make_connection_provider(io_service);
    auto query = apq::query("SELECT $1::integer + $2::integer", 13, 42);
    auto result = apq::binary_protocol::request<Sum>(connection_provider, query, yield);
    std::cout << "sum is " << result.begin()->value << '\n';
});

io_service.run();
