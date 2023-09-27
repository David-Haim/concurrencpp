#include "concurrencpp/io/socket.h"
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

lazy_result<size_t> socket::read_impl(std::shared_ptr<executor> resume_executor, void* buffer, size_t count) {
    throw_if_empty("concurrencpp::socket::read - socket is empty.");

    if (!static_cast<bool>(resume_executor)) {
        throw std::invalid_argument("concurrencpp::socket::read() - resume_executor is null.");
    }

    auto engine = m_state->get_engine("concurrencpp::socket::read() - engine has been shut down.");

    return socket_state::read(m_state, std::move(resume_executor), std::move(engine), buffer, static_cast<uint32_t>(count));
}

lazy_result<size_t> socket::read_impl(std::shared_ptr<executor> resume_executor,
                                      std::stop_token stop_token,
                                      void* buffer,
                                      size_t count) {
    throw_if_empty("concurrencpp::socket::read - socket is empty.");

    if (!static_cast<bool>(resume_executor)) {
        throw std::invalid_argument("concurrencpp::socket::read() - resume_executor is null.");
    }

    auto engine = m_state->get_engine("concurrencpp::socket::read() - engine has been shut down.");

    return socket_state::read(m_state,
                              std::move(resume_executor),
                              std::move(engine),
                              buffer,
                              static_cast<uint32_t>(count),
                              &stop_token);
}

lazy_result<size_t> socket::write_impl(std::shared_ptr<executor> resume_executor, const void* buffer, size_t count) {
    throw_if_empty("concurrencpp::socket::write() - socket is empty.");

    if (!static_cast<bool>(resume_executor)) {
        throw std::invalid_argument("concurrencpp::socket::write() - resume_executor is null.");
    }

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

    if (!static_cast<bool>(resume_executor)) {
        throw std::invalid_argument("socket::write() - resume_executor is null.");
    }

    if ((buffer == nullptr) && (count != 0)) {
        throw std::invalid_argument("concurrencpp::socket::write() - fill error msg here.");
    }

    if (!stop_token.stop_possible()) {
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

    if (!static_cast<bool>(resume_executor)) {
        throw std::invalid_argument("socket::eof_reached() - resume_executor is null.");
    }

    return socket_state::eof_reached(m_state, std::move(resume_executor));
}

lazy_result<void> socket::connect(std::shared_ptr<concurrencpp::executor> resume_executor, concurrencpp::ip_endpoint remote_endpoint) {
    throw_if_empty("concurrencpp::socket::connect() - socket is empty");

    if (!static_cast<bool>(resume_executor)) {
        throw std::invalid_argument("socket::connect() - resume_executor is null.");
    }

    auto engine = m_state->get_engine("concurrencpp::socket::connect() - engine has been shut down.");

    return socket_state::connect(m_state, std::move(resume_executor), std::move(engine), remote_endpoint);
}

lazy_result<void> socket::connect(std::shared_ptr<concurrencpp::executor> resume_executor,
                                  concurrencpp::ip_endpoint remote_endpoint,
                                  std::stop_token stop_token) {
    throw_if_empty("concurrencpp::socket::connect() - socket is empty");

    if (!static_cast<bool>(resume_executor)) {
        throw std::invalid_argument("concurrencpp::socket::connect() - resume_executor is null.");
    }

    if (!stop_token.stop_possible()) {
        throw std::invalid_argument("...");
    }

    auto engine = m_state->get_engine("concurrencpp::socket::connect() - engine has been shut down.");

    return socket_state::connect(m_state, std::move(resume_executor), std::move(engine), remote_endpoint, &stop_token);
}

lazy_result<void> socket::listen(std::shared_ptr<concurrencpp::executor> resume_executor, std::uint32_t backlog) {
    throw_if_empty("concurrencpp::socket::listen() - socket is empty.");

    if (!static_cast<bool>(resume_executor)) {
        throw std::invalid_argument("concurrencpp::socket::listen() - resume_executor is null.");
    }

    return socket_state::listen(m_state, std::move(resume_executor), backlog);
}

lazy_result<socket> socket::accept(std::shared_ptr<concurrencpp::executor> resume_executor) {
    throw_if_empty("concurrencpp::socket::accept() - socket is empty.");

    if (!static_cast<bool>(resume_executor)) {
        throw std::invalid_argument("concurrencpp::socket::accept() - resume_executor is null.");
    }

    auto engine = m_state->get_engine("concurrencpp::socket::accept() - engine has been shut down.");

    return socket_state::accept(m_state, std::move(resume_executor), std::move(engine));
}

lazy_result<socket> socket::accept(std::shared_ptr<concurrencpp::executor> resume_executor, std::stop_token stop_token) {
    throw_if_empty("concurrencpp::socket::accept() - socket is empty.");

    if (!static_cast<bool>(resume_executor)) {
        throw std::invalid_argument("concurrencpp::socket::accept() - resume_executor is null.");
    }

    if (!stop_token.stop_possible()) {
        throw std::invalid_argument("...");
    }

    auto engine = m_state->get_engine("concurrencpp::socket::accept() - engine has been shut down.");

    return socket_state::accept(m_state, std::move(resume_executor), std::move(engine));
}

lazy_result<void> socket::bind(std::shared_ptr<concurrencpp::executor> resume_executor, concurrencpp::ip_endpoint local_endpoint) {
    throw_if_empty("concurrencpp::socket::bind() - socket is empty.");

    if (!static_cast<bool>(resume_executor)) {
        throw std::invalid_argument("concurrencpp::socket::bind() - resume_executor is null.");
    }

    return socket_state::bind(m_state, std::move(resume_executor), local_endpoint);
}

bool socket::keep_alive() const {
    throw_if_empty("concurrencpp::socket::keep_alive() - socket is empty.");
    return socket_state::keep_alive(m_state, );
    return m_state->keep_alive();
}

void socket::keep_alive(bool enable) {
    throw_if_empty("concurrencpp::socket::keep_alive() - socket is empty.");
    m_state->keep_alive(enable);
}

bool socket::broadcast_enabled() const {
    throw_if_empty("concurrencpp::socket::broadcast_enabled() - socket is empty.");
    return m_state->broadcast_enabled();
}

void socket::broadcast_enabled(bool enable) {
    throw_if_empty("concurrencpp::socket::broadcast_enabled() - socket is empty.");
    m_state->broadcast_enabled(enable);
}

bool socket::reuse_port() const {
    throw_if_empty("concurrencpp::socket::reuse_port() - socket is empty.");
    return m_state->reuse_port();
}

void socket::reuse_port(bool enable) {
    throw_if_empty("concurrencpp::socket::reuse_port() - socket is empty.");
    m_state->reuse_port(enable);
}

bool socket::linger_mode() const {
    throw_if_empty("concurrencpp::socket::linger_mode() - socket is empty.");
    return m_state->linger_mode();
}

void socket::linger_mode(bool enable) {
    throw_if_empty("concurrencpp::socket::linger_mode() - socket is empty.");
    m_state->linger_mode(enable);
}

bool socket::no_delay() const {
    throw_if_empty("concurrencpp::socket::no_delay() - socket is empty.");
    return m_state->no_delay();
}

void socket::no_delay(bool enable) {
    throw_if_empty("concurrencpp::socket::no_delay() - socket is empty.");
    m_state->no_delay(enable);
}

uint32_t socket::receive_buffer_size() const {
    throw_if_empty("concurrencpp::socket::receive_buffer_size() - socket is empty.");
    return m_state->receive_buffer_size();
}

void socket::receive_buffer_size(uint32_t size) {
    throw_if_empty("concurrencpp::socket::receive_buffer_size() - socket is empty.");
    m_state->receive_buffer_size(size);
}

uint32_t socket::send_buffer_size() const {
    throw_if_empty("concurrencpp::socket::send_buffer_size() - socket is empty.");
    return m_state->send_buffer_size();
}

void socket::send_buffer_size(uint32_t size) {
    throw_if_empty("concurrencpp::socket::send_buffer_size() - socket is empty.");
    m_state->send_buffer_size(size);
}

std::chrono::milliseconds socket::receive_timeout() const {
    throw_if_empty("concurrencpp::socket::receive_timeout() - socket is empty.");
    return m_state->receive_timeout();
}

void socket::receive_timeout(std::chrono::milliseconds ms) {
    throw_if_empty("concurrencpp::socket::send_buffer_size() - socket is empty.");
    m_state->receive_timeout(ms);
}

std::chrono::milliseconds socket::send_timeout() const {
    throw_if_empty("concurrencpp::socket::send_timeout() - socket is empty.");
    return m_state->send_timeout();
}

void socket::send_timeout(std::chrono::milliseconds ms) {
    throw_if_empty("concurrencpp::socket::send_timeout() - socket is empty.");
    m_state->send_timeout(ms);
}
