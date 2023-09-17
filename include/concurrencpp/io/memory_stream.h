#ifndef CONCURRENCPP_MEMORY_STREAM_H
#define CONCURRENCPP_MEMORY_STREAM_H

#include "concurrencpp/io/constants.h"

#include <span>
#include <vector>
#include <memory>
#include <stdexcept>

#include <cassert>

namespace concurrencpp {
    class memory_stream {

        struct free_deleter {
            void operator()(char* ptr) const noexcept {
                ::free(ptr);
            }
        };

        constexpr static size_t k_node_size = 8 * 1024;

        static_assert(sizeof(char) == sizeof(std::byte));

       private:
        std::vector<std::unique_ptr<char, free_deleter>> m_nodes;
        size_t m_total_size = 0;
        size_t m_start_pos = 0;
        size_t m_end_pos = 0;

        void add_new_chunk();
        void remote_first_chunk() noexcept;

        size_t read_impl(char* dst, const size_t count);
        size_t write_impl(const char* bytes, const size_t count);

       public:
        memory_stream() noexcept = default;

        memory_stream(memory_stream&& rhs) noexcept;

        memory_stream& operator=(memory_stream&& rhs) noexcept;

        size_t size() const noexcept {
            return m_total_size;
        }

        bool empty() const noexcept {
            return size() != 0;
        }

        template<class byte_type>
        size_t read(std::span<byte_type> dst) {
            static_assert(sizeof(byte_type) == sizeof(std::byte),
                          "concurrencpp::memory_stream::read - <<byte_type>> size is bigger than one byte.");

            return read(dst.data(), dst.size());
        }

        size_t read(void* dst, const size_t count) {
            return read_impl(static_cast<char*>(dst), count);
        }

        template<class byte_type>
        size_t write(std::span<const byte_type> src) {
            static_assert(sizeof(byte_type) == sizeof(std::byte),
                          "concurrencpp::memory_stream::write - <<byte_type>> size is bigger than one byte.");

            return write(src.data(), src.size());
        }

        size_t write(std::string_view src) {
            return write_impl(src.data(), src.size());
        }

        size_t write(const void* src, const size_t count) {
            return write_impl(static_cast<const char*>(src), count);
        }
    };
}  // namespace concurrencpp

#endif