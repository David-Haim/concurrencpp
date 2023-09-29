#include "concurrencpp/io/socket.h"
#include "concurrencpp/io/constants.h"
#include "concurrencpp/io/impl/win32/socket_state.h"

#include <cstdint>

using concurrencpp::socket;
using concurrencpp::lazy_result;
using concurrencpp::details::win32::socket_state;

socket::socket(std::shared_ptr<details::win32::socket_state> state) noexcept : reader_writer(std::move(state)) {}

socket::socket(std::shared_ptr<io_engine> engine,
               concurrencpp::io::address_family af,
               concurrencpp::io::socket_type type,
               concurrencpp::io::protocol_type protocol) :
    reader_writer(std::make_shared<details::win32::socket_state>(engine, af, type, protocol)) {}

void socket::throw_if_empty(const char* error_msg) const {
    if (!static_cast<bool>(m_state)) {
        throw std::runtime_error(error_msg);
    }
}

void socket::throw_if_resume_executor_empty(const std::shared_ptr<executor>& executor, const char* error_msg) {
    if (!static_cast<bool>(executor)) {
        throw std::invalid_argument(error_msg);
    }
}

void socket::throw_if_stop_token_empty(const std::stop_token& stop_token, const char* error_msg) {
    if (!stop_token.stop_possible()) {
        throw std::invalid_argument(error_msg);
    }
}

lazy_result<size_t> socket::read_impl(std::shared_ptr<executor> resume_executor, void* buffer, size_t count) {
    throw_if_empty(details::consts::k_socket_read_empty_socket_err_msg);
    throw_if_resume_executor_empty(resume_executor, details::consts::k_socket_read_null_resume_executor_err_msg);

    auto engine = m_state->get_engine(details::consts::k_socket_read_engine_shutdown_err_msg);

    if ((buffer == nullptr) && (count != 0)) {
        throw std::invalid_argument("concurrencpp::socket::write() - fill error msg here.");
    }

    return socket_state::read(m_state, std::move(resume_executor), std::move(engine), buffer, static_cast<uint32_t>(count));
}

lazy_result<size_t> socket::read_impl(std::shared_ptr<executor> resume_executor,
                                      std::stop_token stop_token,
                                      void* buffer,
                                      size_t count) {
    throw_if_empty(details::consts::k_socket_read_empty_socket_err_msg);
    throw_if_resume_executor_empty(resume_executor, details::consts::k_socket_read_null_resume_executor_err_msg);

    auto engine = m_state->get_engine(details::consts::k_socket_read_engine_shutdown_err_msg);

    if ((buffer == nullptr) && (count != 0)) {
        throw std::invalid_argument("concurrencpp::socket::write() - fill error msg here.");
    }

    return socket_state::read(m_state,
                              std::move(resume_executor),
                              std::move(engine),
                              buffer,
                              static_cast<uint32_t>(count),
                              &stop_token);
}

lazy_result<size_t> socket::write_impl(std::shared_ptr<executor> resume_executor, const void* buffer, size_t count) {
    throw_if_empty(details::consts::k_socket_write_empty_socket_err_msg);
    throw_if_resume_executor_empty(resume_executor, details::consts::k_socket_write_null_resume_executor_err_msg);

    if ((buffer == nullptr) && (count != 0)) {
        throw std::invalid_argument("concurrencpp::socket::write() - fill error msg here.");
    }

    auto engine = m_state->get_engine("concurrencpp::socket::write() - engine has been shut down.");

    return socket_state::write(m_state, std::move(resume_executor), std::move(engine), buffer, static_cast<uint32_t>(count));
}

lazy_result<size_t> socket::write_impl(std::shared_ptr<executor> resume_executor,
                                       std::stop_token stop_token,
                                       const void* buffer,
                                       size_t count) {
    throw_if_empty("socket::write() - socket is empty.");
    throw_if_resume_executor_empty(resume_executor, "concurrencpp::socket::write() - given resume_executor is null.");
    throw_if_stop_token_empty(stop_token, "concurrencpp::socket::write() - bad stop_token.");

    if ((buffer == nullptr) && (count != 0)) {
        throw std::invalid_argument("concurrencpp::socket::write() - fill error msg here.");
    }

    auto engine = m_state->get_engine("concurrencpp::socket::write() - engine has been shut down.");

    return socket_state::write(m_state,
                               std::move(resume_executor),
                               std::move(engine),
                               buffer,
                               static_cast<uint32_t>(count),
                               &stop_token);
}

void socket::close() noexcept {
    // TODO: try cancel. also in file_stream
    m_state.reset();
}

bool socket::empty() const noexcept {
    return static_cast<bool>(m_state);
}

concurrencpp::io::address_family socket::get_address_family() const {
    throw_if_empty("concurrencpp::socket::get_address_family() - socket is empty");
    return m_state->get_address_family();
}

concurrencpp::io::socket_type socket::get_socket_type() const {
    throw_if_empty("concurrencpp::socket::get_socket_type() - socket is empty");
    return m_state->get_socket_type();
}

concurrencpp::io::protocol_type socket::get_protocol_type() const {
    throw_if_empty("concurrencpp::socket::get_protocol_type() - socket is empty");
    return m_state->get_protocol_type();
}

lazy_result<bool> socket::eof_reached(std::shared_ptr<concurrencpp::executor> resume_executor) {
    throw_if_empty("concurrencpp::socket::eof_reached() - socket is empty");
    throw_if_resume_executor_empty(resume_executor, "concurrencpp::socket::eof_reached() - resume_executor is null.");

    return socket_state::eof_reached(m_state, std::move(resume_executor));
}

lazy_result<void> socket::connect(std::shared_ptr<concurrencpp::executor> resume_executor, concurrencpp::ip_endpoint remote_endpoint) {
    throw_if_empty("concurrencpp::socket::connect() - socket is empty");
    throw_if_resume_executor_empty(resume_executor, "concurrencpp::socket::connect() - resume_executor is null.");

    auto engine = m_state->get_engine("concurrencpp::socket::connect() - engine has been shut down.");

    return socket_state::connect(m_state, std::move(resume_executor), std::move(engine), remote_endpoint);
}

lazy_result<void> socket::connect(std::shared_ptr<concurrencpp::executor> resume_executor,
                                  concurrencpp::ip_endpoint remote_endpoint,
                                  std::stop_token stop_token) {
    throw_if_empty("concurrencpp::socket::connect() - socket is empty");
    throw_if_resume_executor_empty(resume_executor, "concurrencpp::socket::connect() - resume_executor is null.");
    throw_if_stop_token_empty(stop_token, "concurrencpp::socket::connect() - bad stop_token.");

    auto engine = m_state->get_engine("concurrencpp::socket::connect() - engine has been shut down.");

    return socket_state::connect(m_state, std::move(resume_executor), std::move(engine), remote_endpoint, &stop_token);
}

lazy_result<void> socket::listen(std::shared_ptr<concurrencpp::executor> resume_executor, std::uint32_t backlog) {
    throw_if_empty("concurrencpp::socket::listen() - socket is empty.");
    throw_if_resume_executor_empty(resume_executor, "concurrencpp::socket::listen() - resume_executor is null.");

    return socket_state::listen(m_state, std::move(resume_executor), backlog);
}

lazy_result<socket> socket::accept(std::shared_ptr<concurrencpp::executor> resume_executor) {
    throw_if_empty("concurrencpp::socket::accept() - socket is empty.");
    throw_if_resume_executor_empty(resume_executor, "concurrencpp::socket::accept() - resume_executor is null.");

    auto engine = m_state->get_engine("concurrencpp::socket::accept() - engine has been shut down.");

    return socket_state::accept(m_state, std::move(resume_executor), std::move(engine));
}

lazy_result<socket> socket::accept(std::shared_ptr<concurrencpp::executor> resume_executor, std::stop_token stop_token) {
    throw_if_empty("concurrencpp::socket::accept() - socket is empty.");
    throw_if_resume_executor_empty(resume_executor, "concurrencpp::socket::accept() - resume_executor is null.");
    throw_if_stop_token_empty(stop_token, "concurrencpp::socket::accept() - bad stop_token.");

    auto engine = m_state->get_engine("concurrencpp::socket::accept() - engine has been shut down.");

    return socket_state::accept(m_state, std::move(resume_executor), std::move(engine));
}

lazy_result<void> socket::bind(std::shared_ptr<concurrencpp::executor> resume_executor, concurrencpp::ip_endpoint local_endpoint) {
    throw_if_empty("concurrencpp::socket::bind() - socket is empty.");
    throw_if_resume_executor_empty(resume_executor, "concurrencpp::socket::bind() - resume_executor is null.");

    return socket_state::bind(m_state, std::move(resume_executor), local_endpoint);
}

lazy_result<std::optional<concurrencpp::ip_endpoint>> socket::local_endpoint(std::shared_ptr<concurrencpp::executor> resume_executor) {
    throw_if_empty("concurrencpp::socket::local_endpoint() - socket is empty.");
    throw_if_resume_executor_empty(resume_executor, "concurrencpp::socket::local_endpoint() - given resume executor is null.");

    return socket_state::local_endpoint(m_state, std::move(resume_executor));
}

lazy_result<std::optional<concurrencpp::ip_endpoint>> socket::remote_endpoint(
    std::shared_ptr<concurrencpp::executor> resume_executor) {
    throw_if_empty("concurrencpp::socket::remote_endpoint() - socket is empty.");
    throw_if_resume_executor_empty(resume_executor, "concurrencpp::socket::remote_endpoint() - given resume executor is null.");

    return socket_state::local_endpoint(m_state, std::move(resume_executor));
}

lazy_result<uint32_t> socket::available_bytes(std::shared_ptr<concurrencpp::executor> resume_executor) {
    throw_if_empty("concurrencpp::socket::available_bytes() - socket is empty.");
    throw_if_resume_executor_empty(resume_executor, "concurrencpp::socket::available_bytes() - given resume executor is null.");

    return socket_state::available_bytes(m_state, std::move(resume_executor));
}

lazy_result<bool> socket::keep_alive(std::shared_ptr<concurrencpp::executor> resume_executor) {
    throw_if_empty("concurrencpp::socket::keep_alive() - socket is empty.");
    throw_if_resume_executor_empty(resume_executor, "concurrencpp::socket::keep_alive() - given resume executor is null.");

    return socket_state::keep_alive(m_state, std::move(resume_executor));
}

lazy_result<void> socket::keep_alive(std::shared_ptr<concurrencpp::executor> resume_executor, bool enable) {
    throw_if_empty("concurrencpp::socket::keep_alive() - socket is empty.");
    throw_if_resume_executor_empty(resume_executor, "concurrencpp::socket::keep_alive() - given resume executor is null.");

    return socket_state::keep_alive(m_state, std::move(resume_executor), enable);
}

lazy_result<bool> socket::broadcast_enabled(std::shared_ptr<concurrencpp::executor> resume_executor) {
    throw_if_empty("concurrencpp::socket::broadcast_enabled() - socket is empty.");
    throw_if_resume_executor_empty(resume_executor, "concurrencpp::socket::broadcast_enabled() - given resume executor is null.");

    return socket_state::broadcast_enabled(m_state, std::move(resume_executor));
}

lazy_result<void> socket::broadcast_enabled(std::shared_ptr<concurrencpp::executor> resume_executor, bool enable) {
    throw_if_empty("concurrencpp::socket::broadcast_enabled() - socket is empty.");
    throw_if_resume_executor_empty(resume_executor, "concurrencpp::socket::broadcast_enabled() - given resume executor is null.");

    return socket_state::broadcast_enabled(m_state, std::move(resume_executor), enable);
}

lazy_result<bool> socket::reuse_port(std::shared_ptr<concurrencpp::executor> resume_executor) {
    throw_if_empty("concurrencpp::socket::reuse_port() - socket is empty.");
    throw_if_resume_executor_empty(resume_executor, "concurrencpp::socket::reuse_port() - given resume executor is null.");

    return socket_state::reuse_port(m_state, std::move(resume_executor));
}

lazy_result<void> socket::reuse_port(std::shared_ptr<concurrencpp::executor> resume_executor, bool enable) {
    throw_if_empty("concurrencpp::socket::reuse_port() - socket is empty.");
    throw_if_resume_executor_empty(resume_executor, "concurrencpp::socket::reuse_port() - given resume executor is null.");

    return socket_state::reuse_port(m_state, std::move(resume_executor), enable);
}

lazy_result<bool> socket::linger_mode(std::shared_ptr<concurrencpp::executor> resume_executor) {
    throw_if_empty("concurrencpp::socket::linger_mode() - socket is empty.");
    throw_if_resume_executor_empty(resume_executor, "concurrencpp::socket::linger_mode() - given resume executor is null.");

    return socket_state::linger_mode(m_state, std::move(resume_executor));
}

lazy_result<void> socket::linger_mode(std::shared_ptr<concurrencpp::executor> resume_executor, bool enable) {
    throw_if_empty("concurrencpp::socket::linger_mode() - socket is empty.");
    throw_if_resume_executor_empty(resume_executor, "concurrencpp::socket::linger_mode() - given resume executor is null.");

    return socket_state::linger_mode(m_state, std::move(resume_executor), enable);
}

lazy_result<bool> socket::no_delay(std::shared_ptr<concurrencpp::executor> resume_executor) {
    throw_if_empty("concurrencpp::socket::no_delay() - socket is empty.");
    throw_if_resume_executor_empty(resume_executor, "concurrencpp::socket::no_delay() - given resume executor is null.");

    return socket_state::no_delay(m_state, std::move(resume_executor));
}

lazy_result<void> socket::no_delay(std::shared_ptr<concurrencpp::executor> resume_executor, bool enable) {
    throw_if_empty("concurrencpp::socket::no_delay() - socket is empty.");
    throw_if_resume_executor_empty(resume_executor, "concurrencpp::socket::no_delay() - given resume executor is null.");

    return socket_state::no_delay(m_state, std::move(resume_executor), enable);
}

lazy_result<uint32_t> socket::receive_buffer_size(std::shared_ptr<concurrencpp::executor> resume_executor) {
    throw_if_empty("concurrencpp::socket::receive_buffer_size() - socket is empty.");
    throw_if_resume_executor_empty(resume_executor, "concurrencpp::socket::receive_buffer_size() - given resume executor is null.");

    return socket_state::receive_buffer_size(m_state, std::move(resume_executor));
}

lazy_result<void> socket::receive_buffer_size(std::shared_ptr<concurrencpp::executor> resume_executor, uint32_t size) {
    throw_if_empty("concurrencpp::socket::receive_buffer_size() - socket is empty.");
    throw_if_resume_executor_empty(resume_executor, "concurrencpp::socket::receive_buffer_size() - given resume executor is null.");

    return socket_state::receive_buffer_size(m_state, std::move(resume_executor), size);
}

lazy_result<uint32_t> socket::send_buffer_size(std::shared_ptr<concurrencpp::executor> resume_executor) {
    throw_if_empty("concurrencpp::socket::send_buffer_size() - socket is empty.");
    throw_if_resume_executor_empty(resume_executor, "concurrencpp::socket::send_buffer_size() - given resume executor is null.");

    return socket_state::send_buffer_size(m_state, std::move(resume_executor));
}

lazy_result<void> socket::send_buffer_size(std::shared_ptr<concurrencpp::executor> resume_executor, uint32_t size) {
    throw_if_empty("concurrencpp::socket::send_buffer_size() - socket is empty.");
    throw_if_resume_executor_empty(resume_executor, "concurrencpp::socket::send_buffer_size() - given resume executor is null.");

    return socket_state::send_buffer_size(m_state, std::move(resume_executor), size);
}

lazy_result<std::chrono::milliseconds> socket::receive_timeout(std::shared_ptr<concurrencpp::executor> resume_executor) {
    throw_if_empty("concurrencpp::socket::receive_timeout() - socket is empty.");
    throw_if_resume_executor_empty(resume_executor, "concurrencpp::socket::receive_timeout() - given resume executor is null.");

    return socket_state::receive_timeout(m_state, std::move(resume_executor));
}

lazy_result<void> socket::receive_timeout(std::shared_ptr<concurrencpp::executor> resume_executor, std::chrono::milliseconds ms) {
    throw_if_empty("concurrencpp::socket::receive_timeout() - socket is empty.");
    throw_if_resume_executor_empty(resume_executor, "concurrencpp::socket::receive_timeout() - given resume executor is null.");

    return socket_state::receive_timeout(m_state, std::move(resume_executor), ms);
}

lazy_result<std::chrono::milliseconds> socket::send_timeout(std::shared_ptr<concurrencpp::executor> resume_executor) {
    throw_if_empty("concurrencpp::socket::receive_timeout() - socket is empty.");
    throw_if_resume_executor_empty(resume_executor, "concurrencpp::socket::receive_timeout() - given resume executor is null.");

    return socket_state::send_timeout(m_state, std::move(resume_executor));
}

lazy_result<void> socket::send_timeout(std::shared_ptr<concurrencpp::executor> resume_executor, std::chrono::milliseconds ms) {
    throw_if_empty("concurrencpp::socket::receive_timeout() - socket is empty.");
    throw_if_resume_executor_empty(resume_executor, "concurrencpp::socket::receive_timeout() - given resume executor is null.");

    return socket_state::send_timeout(m_state, std::move(resume_executor), ms);
}
