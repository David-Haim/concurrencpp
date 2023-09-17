#include "concurrencpp/io/memory_stream.h"

using concurrencpp::memory_stream;

memory_stream::memory_stream(memory_stream&& rhs) noexcept :
    m_nodes(std::exchange(rhs.m_nodes, {})), m_total_size(std::exchange(rhs.m_total_size, 0)),
    m_start_pos(std::exchange(rhs.m_start_pos, 0)), m_end_pos(std::exchange(rhs.m_end_pos, 0)) {}

memory_stream& memory_stream::operator=(memory_stream&& rhs) noexcept {
    if (&rhs == this) {
        return *this;
    }

    m_nodes = std::exchange(rhs.m_nodes, {});
    m_total_size = std::exchange(rhs.m_total_size, 0);
    m_start_pos = std::exchange(rhs.m_start_pos, 0);
    m_end_pos = std::exchange(rhs.m_end_pos, 0);
    return *this;
}

void memory_stream::add_new_chunk() {
    assert((m_end_pos == k_node_size) || m_nodes.empty());

    std::unique_ptr<char, free_deleter> new_memory(static_cast<char*>(::malloc(k_node_size)));
    if (new_memory.get() == nullptr) {
        throw std::bad_alloc();
    }

    m_nodes.emplace_back(std::move(new_memory));
    m_end_pos = 0;
}

void memory_stream::remote_first_chunk() noexcept {
    assert(m_start_pos == k_node_size);
    m_nodes.erase(m_nodes.begin());
    m_start_pos = 0;
}

size_t memory_stream::read_impl(char* dst, const size_t count) {
    if (count == 0) {
        return count;
    }

    if (dst == nullptr) {
        throw std::invalid_argument(details::consts::k_memory_stream_read_nullptr_dst);
    }

    auto remaining_bytes = count;
    auto read_length = count;
    if (remaining_bytes > m_total_size) {
        read_length = remaining_bytes = m_total_size;
    }

    while (remaining_bytes != 0) {
        assert(!m_nodes.empty());
        auto& first_node = m_nodes.front();
        const auto remaining_capacity = k_node_size - m_start_pos;
        const auto to_read = std::min(remaining_bytes, remaining_capacity);

        assert(m_start_pos < k_node_size);
        ::memcpy(dst, first_node.get() + m_start_pos, to_read);
        remaining_bytes -= to_read;
        dst += to_read;
        m_total_size -= to_read;
        m_start_pos += to_read;

        if (m_start_pos == k_node_size) {
            remote_first_chunk();
        }
    }

    return read_length;
}

size_t memory_stream::write_impl(const char* bytes, const size_t count) {
    if (count == 0) {
        return 0;
    }

    if (bytes == nullptr) {
        throw std::invalid_argument(details::consts::k_memory_stream_write_nullptr_src);
    }

    if (m_nodes.empty()) {
        add_new_chunk();
    }

    auto remaining_bytes = count;
    while (remaining_bytes != 0) {
        auto& last_node = m_nodes.back();
        const auto remaining_capacity = k_node_size - m_end_pos;
        const auto to_write = std::min(remaining_bytes, remaining_capacity);

        assert(m_end_pos < k_node_size);
        ::memcpy(last_node.get() + m_end_pos, bytes, to_write);
        remaining_bytes -= to_write;
        bytes += to_write;
        m_total_size += to_write;
        m_end_pos += to_write;

        if (m_end_pos == k_node_size) {
            add_new_chunk();
        }
    }

    return count;
}