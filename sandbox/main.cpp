#include "concurrencpp/concurrencpp.h"

#include <iostream>
#include <concurrencpp/io/impl/win32/io_engine.h>
#include <concurrencpp/io/socket.h>

using namespace concurrencpp;
using namespace concurrencpp::io;

result<void> run_socket(std::shared_ptr<io_engine> engine, std::shared_ptr<worker_thread_executor> ex) {
    concurrencpp::socket sock(engine, address_family::ip_v6, socket_type::stream, protocol_type::tcp);

    ip_endpoint ep;
    ep.address = ip_v6("2a00:1450:4028:802::2004");
    ep.port = 80;

    co_await sock.connect(ex, ep);
    std::cout << "socket is connected" << std::endl;

    co_await sock.write(ex, "GET / HTTP/1.1\r\nHost: host:port\r\nConnection: close\r\n\r\n");

    std::cout << "request was written!" << std::endl;

    char buffer[100];
    std::string req;
    while (!co_await sock.eof_reached(ex)) {
        auto read = co_await sock.read(ex, buffer);
        req.append(buffer, buffer + read);
    }

    std::cout << req << std::endl;
}

null_result request_handler(executor_tag, std::shared_ptr<worker_thread_executor> ex, concurrencpp::socket sock) {
    char buffer[1024];
    std::string req;
    auto read = co_await sock.read(ex, buffer);
    req.append(buffer, buffer + read);
   
    std::cout << req << std::endl;
}

result<void> run_socket_server(std::shared_ptr<io_engine> engine, std::shared_ptr<worker_thread_executor> ex) {
    concurrencpp::socket sock(engine, address_family::ip_v4, socket_type::stream, protocol_type::tcp);
    co_await sock.bind(ex, {ip_v4::any, 8080});
    co_await sock.listen(ex);

    while (true) {
        auto new_sock = co_await sock.accept(ex);
        std::cout << "new socket received " << std::endl;

        request_handler({}, ex, std::move(new_sock));
    }
}

int main() {
    concurrencpp::runtime runtime;
    auto engine = std::make_shared<io_engine>();
    auto ex = runtime.make_worker_thread_executor();

    //    auto result = run_socket(engine, ex);
    auto result = run_socket_server(engine, ex);

    try {
        result.get();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }

    engine->shutdown();

    return 0;
}
