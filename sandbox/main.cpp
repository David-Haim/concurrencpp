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

const uint16_t port = 12345;

result<void> udp_reciever(std::shared_ptr<io_engine> engine, std::shared_ptr<worker_thread_executor> ex) {
    concurrencpp::socket sock(engine, address_family::ip_v4, socket_type::datagram, protocol_type::udp);

    co_await sock.bind(ex, {ip_v4::loop_back, port});
    
    char buffer[1024];
    std::string req;
    auto res = co_await sock.receive_from(ex, buffer, sizeof buffer);
   
//    auto res = co_await sock.read(ex, buffer, 1024);
    req.append(buffer, buffer + res.read);
    std::cout << "New message from " << std::get<0>(res.remote_endpoint.address).to_string() << " , " 
        << res.remote_endpoint.port << " : " 
        << req << std::endl;
}

result<void> udp_sender(std::shared_ptr<io_engine> engine, std::shared_ptr<worker_thread_executor> ex) {
    concurrencpp::socket sock(engine, address_family::ip_v4, socket_type::datagram, protocol_type::udp);

    co_await sock.bind(ex, {ip_v4::any, 8090});
    ip_endpoint ep;
    ep.address = ip_v4::loop_back;
    ep.port = port;
    auto res = co_await sock.send_to(ex, ep,(void*) "hello world", 11);
    std::cout << "sent : " << res;
}

#include "concurrencpp/io/file_stream.h"

int main() {
    concurrencpp::runtime runtime;
    auto engine = std::make_shared<io_engine>();
    auto ex = runtime.make_worker_thread_executor();

    concurrencpp::file_stream fs(engine, R"(C:\Users\David\Desktop\langs.txt)",file_open_mode::read);
    
    char buff[1024];
    fs.read(ex, buff).run().get();

    //    auto result = run_socket(engine, ex);
//    auto result = run_socket_server(engine, ex);

    auto result = udp_reciever(engine, ex);

    std::this_thread::sleep_for(std::chrono::seconds(1));

    auto result1 = udp_sender(engine, ex);
    try {
        result.get();
        result1.get();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }

    engine->shutdown();

    return 0;
}
