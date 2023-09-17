#include "concurrencpp/concurrencpp.h"

#include <iostream>

#include "concurrencpp/io/impl/win32/io_engine.h"
#include "concurrencpp/io/file_stream.h"

using namespace concurrencpp;

result<void> read_file_async(std::shared_ptr<thread_pool_executor> tpe) {
    auto engine = std::make_shared<concurrencpp::io_engine>();
    concurrencpp::file_stream fs(engine, R"(C:\Users\David\Downloads\46305_BoundAndGagged.mp4)", concurrencpp::file_open_mode::read);
    concurrencpp::file_stream fs1(engine, R"(C:\Users\David\Desktop\bng.mp4)", concurrencpp::file_open_mode::write);
    
    while (!(co_await fs.eof_reached(tpe))) {
        std::byte buffer[1024 * 16];
        auto read = co_await fs.read(tpe, buffer);
        co_await fs1.write(tpe, buffer, read);    
    }
    
    engine->shutdown();
}

int main() {
    concurrencpp::runtime r;
    auto tpe = r.thread_pool_executor();
    read_file_async(tpe).get();
    return 0;
}
