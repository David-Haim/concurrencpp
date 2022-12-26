#ifndef CONCURRENCPP_SLIST_H
#define CONCURRENCPP_SLIST_H

#include <cassert>

namespace concurrencpp::details {
    template<class node_type>
    class slist {

       private:
        node_type* m_head = nullptr;
        node_type* m_tail = nullptr;

        void assert_state() const noexcept {
            if (m_head == nullptr) {
                assert(m_tail == nullptr);
                return;
            }

            assert(m_tail != nullptr);
        }

       public:
        slist() noexcept = default;

        slist(slist&& rhs) noexcept : m_head(rhs.m_head), m_tail(rhs.m_tail) {
            rhs.m_head = nullptr;
            rhs.m_tail = nullptr;
        }

        bool empty() const noexcept {
            assert_state();
            return m_head == nullptr;
        }

        void push_back(node_type& node) noexcept {
            assert_state();

            if (m_head == nullptr) {
                m_head = m_tail = &node;
                return;
            }

            m_tail->next = &node;
            m_tail = &node;
        }

        node_type* pop_front() noexcept {
            assert_state();
            const auto node = m_head;
            if (node == nullptr) {
                return nullptr;
            }

            m_head = m_head->next;
            if (m_head == nullptr) {
                m_tail = nullptr;
            }

            return node;
        }
    };
}  // namespace concurrencpp::details

#endif