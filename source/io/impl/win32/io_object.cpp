#include "concurrencpp/io/impl/win32/io_engine.h"
#include "concurrencpp/io/impl/win32/io_awaitable.h"

using concurrencpp::details::win32::io_object;
using concurrencpp::io_engine;

io_object::io_object(void* handle, handle_cleanup_fn cleanup_fn, std::shared_ptr<io_engine> engine) noexcept :
    m_handle(handle, cleanup_fn), m_engine(std::move(engine)) {
    assert(static_cast<bool>(engine));
    assert(handle != reinterpret_cast<void*>(-1));

    engine->register_handle(handle);
}

void* io_object::handle() const noexcept {
    return m_handle.get();
}

std::shared_ptr<io_engine> io_object::get_engine(const char* error_msg) const {
    auto engine = m_engine.lock();
    if (!static_cast<bool>(engine)) {
        throw std::runtime_error(error_msg);
    }

    return engine;
}
