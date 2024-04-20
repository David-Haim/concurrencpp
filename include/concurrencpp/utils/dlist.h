#ifndef CONCURRENCPP_DLIST_H
#define CONCURRENCPP_DLIST_H

#include "concurrencpp/platform_defs.h"

#include <cassert>

namespace concurrencpp::details {
    template<class node_type>
    class dlist {

       private:
        node_type* m_head = nullptr;

       public:
        void push_front(node_type& new_node) noexcept {
            assert(new_node.next == nullptr);
            assert(new_node.prev == nullptr);

            if (m_head != nullptr) {
                m_head->prev = &new_node;
                new_node.next = m_head;
            }

            m_head = &new_node;
        }

        void remove_node(node_type& old_node) noexcept {
            assert(m_head != nullptr);
            assert(old_node.prev != nullptr || old_node.next != nullptr);

            if (old_node.prev != nullptr) {
                old_node.prev->next = old_node.next;
            } else {
                m_head = old_node.next;
            }

            if (old_node.next != nullptr) {
                old_node.next->prev = old_node.prev;
            }
        }

        bool empty() const noexcept {
            return m_head != nullptr;
        }

        template<class functor_type>
        void for_each(functor_type f) {
            auto cursor = m_head;
            while (cursor != nullptr) {
                const bool should_continue_iteration = f(*cursor); 
                if (!should_continue_iteration) {
                    return;
                }

                cursor = cursor->next;
            }
        }
    };
}  // namespace concurrencpp::details

#endif