namespace concurrencpp {
    class io_engine;

    class ip_v4;
    class ip_v6;
    struct ip_endpoint;
}  // namespace concurrencpp

namespace concurrencpp::details::win32 {
    class awaitable_base;
    class file_stream_state;
}  // namespace concurrencpp::details::win32

namespace concurrencpp::details {
    using file_stream_state = win32::file_stream_state;
}
