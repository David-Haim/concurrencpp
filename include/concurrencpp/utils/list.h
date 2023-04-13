#ifndef CONCURRENCPP_LIST_H
#define CONCURRENCPP_LIST_H

#include <utility>

#include <cassert>

namespace concurrencpp::details {
    template<class node_type>
    class list {

       private:
        node_type* m_head = nullptr;
        node_type* m_tail = nullptr;
        size_t m_size = 0;

       public:
        list() noexcept = default;
        
        list(list&& rhs) noexcept :
            m_head(std::exchange(rhs.m_head, nullptr)), m_tail(std::exchange(rhs.m_tail, nullptr)),
            m_size(std::exchange(rhs.m_size, 0)) {}

        list& operator=(list&& rhs) noexcept {
            if (this == &rhs) {
                return *this;
            }

            m_head = std::exchange(rhs.m_head, nullptr);
            m_tail = std::exchange(rhs.m_tail, nullptr);
            m_size = std::exchange(rhs.m_size, 0);
            return *this;
        }

        size_t size() const noexcept {
            return m_size;
        }

        bool empty() const noexcept {
            return m_size == 0;
        }

        void push_front(node_type* node) noexcept {
            ++m_size;

            if (m_head == nullptr) {
                assert(m_tail == nullptr);
                m_head = node;
                m_tail = node;
                return;
            }

            assert(m_tail != nullptr);
            node->prev = nullptr;
            node->next = m_head;
            m_head->prev = node;
            m_head = node;
        }

        void push_back(node_type* node) noexcept {
            ++m_size;

            if (m_tail == nullptr) {
                assert(m_head == nullptr);
                m_head = node;
                m_tail = node;
                return;
            }

            assert(m_head != nullptr);
            node->prev = m_tail;
            m_tail->next = node;
            m_tail = node;
        }

        void push_back(node_type* head, node_type* tail, size_t count) noexcept {
            if (count == 0) {
                return;
            }

            assert(head != nullptr);
            assert(tail != nullptr);
            assert(head->prev == nullptr);
            assert(tail->next == nullptr);

            if (empty()) {
                m_head = head;
                m_tail = tail;
                m_size = count;
                return;
            }

            m_size += count;
            assert(m_tail->next == nullptr);
            m_tail->next = head;
            head->prev = m_tail;
            m_tail = tail;
        }

        node_type* pop_front() noexcept {
            assert(!empty());

            --m_size;
            auto result = m_head;

            if (m_size == 0) {
                m_head = nullptr;
                m_tail = nullptr;
                return result;
            }

            if (m_head->next != nullptr) {
                m_head->next->prev = nullptr;
            }

            m_head = m_head->next;
            return result;
        }

        std::pair<node_type*, node_type*> pop_front(size_t count) noexcept {
            assert(m_size >= count);

            if (count == 0) {
                return {nullptr, nullptr};
            }

            m_size -= count;
            if (m_size == 0) {
                auto head = m_head;
                auto tail = m_tail;

                m_head = nullptr;
                m_tail = nullptr;
                return {head, tail};
            }

            auto old_head = m_head;
            auto tail_cursor = m_head;
            for (size_t i = 0; i < count - 1; i++) {
                tail_cursor = tail_cursor->next;
            }

            auto new_head = tail_cursor->next;
            assert(new_head != nullptr);
            new_head->prev = nullptr;
            m_head = new_head;

            tail_cursor->next = nullptr;
            return {old_head, tail_cursor};
        }

        node_type* pop_back() noexcept {
            assert(!empty());

            --m_size;
            auto result = m_tail;

            if (m_size == 0) {
                m_head = nullptr;
                m_tail = nullptr;
                return result;
            }

            if (m_tail->prev != nullptr) {
                m_tail->prev->next = nullptr;
            }

            m_tail = m_tail->prev;
            return result;
        }
    };
}  // namespace concurrencpp::details

#endif