#ifndef CONCURRENCPP_IO_READER_WRITER_H
#define CONCURRENCPP_IO_READER_WRITER_H

#include "concurrencpp/results/lazy_result.h"

#include <span>
#include <array>
#include <vector>
#include <string>
#include <stop_token>

namespace concurrencpp::io::details {
    template<class derived_type, class state_type>
    class reader_writer {

       protected:
        std::shared_ptr<state_type> m_state;

       public:
        reader_writer(std::shared_ptr<state_type> state) noexcept : m_state(std::move(state)) {}

        /*
            read overloads
        */
        lazy_result<size_t> read(std::shared_ptr<executor> resume_executor, void* buffer, size_t count) {
            return static_cast<derived_type*>(this)->read_impl(std::move(resume_executor), buffer, count);
        }

        lazy_result<size_t> read(std::shared_ptr<executor> resume_executor, std::stop_token stop_token, void* buffer, size_t count) {
            return static_cast<derived_type*>(this)->read_impl(std::move(resume_executor), stop_token, buffer, count);
        }

        template<class byte_type>
        lazy_result<size_t> read(std::shared_ptr<executor> resume_executor, std::span<byte_type> buffer) {
            static_assert(sizeof(byte_type) == 1, "concurrencpp::stream::read - sizeof(byte_type) is not 1.");
            return read(std::move(resume_executor), buffer.data(), buffer.size());
        }

        template<class byte_type>
        lazy_result<size_t> read(std::shared_ptr<executor> resume_executor, std::stop_token stop_token, std::span<byte_type> buffer) {
            static_assert(sizeof(byte_type) == 1, "concurrencpp::stream::read - sizeof(byte_type) is not 1.");
            return read(std::move(resume_executor), stop_token, buffer.data(), buffer.size());
        }

        template<class byte_type, size_t count>
        lazy_result<size_t> read(std::shared_ptr<executor> resume_executor, byte_type (&buffer)[count]) {
            static_assert(sizeof(byte_type) == 1, "concurrencpp::stream::read - sizeof(byte_type) is not 1.");
            return read(std::move(resume_executor), buffer, count);
        }

        template<class byte_type, size_t count>
        lazy_result<size_t> read(std::shared_ptr<executor> resume_executor, std::stop_token stop_token, byte_type (&buffer)[count]) {
            static_assert(sizeof(byte_type) == 1, "concurrencpp::stream::read - sizeof(byte_type) is not 1.");
            return read(std::move(resume_executor), stop_token, buffer, count);
        }

        template<class byte_type, size_t count>
        lazy_result<size_t> read(std::shared_ptr<executor> resume_executor, std::array<byte_type, count>& buffer) {
            static_assert(sizeof(byte_type) == 1, "concurrencpp::stream::read - sizeof(byte_type) is not 1.");
            return read(std::move(resume_executor), buffer.data(), buffer.size());
        }

        template<class byte_type, size_t count>
        lazy_result<size_t> read(std::shared_ptr<executor> resume_executor,
                                 std::stop_token stop_token,
                                 std::array<byte_type, count>& buffer) {
            static_assert(sizeof(byte_type) == 1, "concurrencpp::stream::read - sizeof(byte_type) is not 1.");
            return read(std::move(resume_executor), stop_token, buffer.data(), buffer.size());
        }

        template<class byte_type, size_t count>
        lazy_result<size_t> read(std::shared_ptr<executor> resume_executor, std::vector<byte_type>& buffer) {
            static_assert(sizeof(byte_type) == 1, "concurrencpp::stream::read - sizeof(byte_type) is not 1.");
            return read(std::move(resume_executor), buffer.data(), buffer.size());
        }

        template<class byte_type, size_t count>
        lazy_result<size_t> read(std::shared_ptr<executor> resume_executor,
                                 std::stop_token stop_token,
                                 std::vector<byte_type>& buffer) {
            static_assert(sizeof(byte_type) == 1, "concurrencpp::stream::read - sizeof(byte_type) is not 1.");
            return read(std::move(resume_executor), stop_token, buffer.data(), buffer.size());
        }

        lazy_result<size_t> read(std::shared_ptr<executor> resume_executor, std::string& buffer) {
            return read(std::move(resume_executor), buffer.data(), buffer.size());
        }

        lazy_result<size_t> read(std::shared_ptr<executor> resume_executor, std::stop_token stop_token, std::string& buffer) {
            return read(std::move(resume_executor), stop_token, buffer.data(), buffer.size());
        }

        /*
            write overloads
        */

        lazy_result<size_t> write(std::shared_ptr<executor> resume_executor, const void* buffer, size_t count) {
            return static_cast<derived_type*>(this)->write_impl(std::move(resume_executor), buffer, count);
        }

        lazy_result<size_t> write(std::shared_ptr<executor> resume_executor,
                                  std::stop_token stop_token,
                                  const void* buffer,
                                  size_t count) {
            return static_cast<derived_type*>(this)->write_impl(std::move(resume_executor), stop_token, buffer, count);
        }

        template<class byte_type>
        lazy_result<size_t> write(std::shared_ptr<executor> resume_executor, std::span<const byte_type> buffer) {
            static_assert(sizeof(byte_type) == 1, "concurrencpp::stream::write - sizeof(byte_type) is not 1.");
            return write(std::move(resume_executor), buffer.data(), buffer.size());
        }

        template<class byte_type>
        lazy_result<size_t> write(std::shared_ptr<executor> resume_executor,
                                  std::stop_token stop_token,
                                  std::span<const byte_type> buffer) {
            static_assert(sizeof(byte_type) == 1, "concurrencpp::stream::write - sizeof(byte_type) is not 1.");
            return write(std::move(resume_executor), stop_token, buffer.data(), buffer.size());
        }

        template<class byte_type, size_t count>
        lazy_result<size_t> write(std::shared_ptr<executor> resume_executor, const byte_type (&buffer)[count]) {
            static_assert(sizeof(byte_type) == 1, "concurrencpp::stream::write - sizeof(byte_type) is not 1.");
            return write(std::move(resume_executor), buffer, count);
        }

        template<class byte_type, size_t count>
        lazy_result<size_t> write(std::shared_ptr<executor> resume_executor,
                                  std::stop_token stop_token,
                                  const byte_type (&buffer)[count]) {
            static_assert(sizeof(byte_type) == 1, "concurrencpp::stream::write - sizeof(byte_type) is not 1.");
            return write(std::move(resume_executor), stop_token, buffer, count);
        }

        template<class byte_type, size_t count>
        lazy_result<size_t> write(std::shared_ptr<executor> resume_executor, const std::array<byte_type, count>& buffer) {
            static_assert(sizeof(byte_type) == 1, "concurrencpp::stream::write - sizeof(byte_type) is not 1.");
            return write(std::move(resume_executor), buffer.data(), buffer.size());
        }

        template<class byte_type, size_t count>
        lazy_result<size_t> write(std::shared_ptr<executor> resume_executor,
                                  std::stop_token stop_token,
                                  const std::array<byte_type, count>& buffer) {
            static_assert(sizeof(byte_type) == 1, "concurrencpp::stream::write - sizeof(byte_type) is not 1.");
            return write(std::move(resume_executor), stop_token, buffer.data(), buffer.size());
        }

        template<class byte_type, size_t count>
        lazy_result<size_t> write(std::shared_ptr<executor> resume_executor, const std::vector<byte_type>& buffer) {
            static_assert(sizeof(byte_type) == 1, "concurrencpp::stream::write - sizeof(byte_type) is not 1.");
            return write(std::move(resume_executor), buffer.data(), buffer.size());
        }

        template<class byte_type, size_t count>
        lazy_result<size_t> write(std::shared_ptr<executor> resume_executor,
                                  std::stop_token stop_token,
                                  const std::vector<byte_type>& buffer) {
            static_assert(sizeof(byte_type) == 1, "concurrencpp::stream::write - sizeof(byte_type) is not 1.");
            return write(std::move(resume_executor), stop_token, buffer.data(), buffer.size());
        }

        lazy_result<size_t> write(std::shared_ptr<executor> resume_executor, const std::string& buffer) {
            return write(std::move(resume_executor), buffer.data(), buffer.size());
        }

        lazy_result<size_t> write(std::shared_ptr<executor> resume_executor, std::stop_token stop_token, const std::string& buffer) {
            return write(std::move(resume_executor), stop_token, buffer.data(), buffer.size());
        }
    };
}  // namespace concurrencpp::io::details

#endif