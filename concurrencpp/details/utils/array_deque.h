#ifndef CONCURRENCPP_ARRAY_DEQUE_H
#define CONCURRENCPP_ARRAY_DEQUE_H

#include <memory>
#include <algorithm>
#include <type_traits>

#include <cassert>

#include <iterator>

namespace concurrencpp::details {

	template<class type> class array_deque;
	template<class type> class array_deque_iterator;
	template<class type> class const_array_deque_iterator;

	template<class type>
	class array_deque_iterator {

	private:
		friend class const_array_deque_iterator<type>;

	public:
		using iterator_category = std::forward_iterator_tag;
		using value_type        = type;
		using difference_type   = ptrdiff_t;
		using pointer           = type *;
		using reference         = type &;

	private:
		type* m_elements;
		size_t m_cursor, m_capacity;

		void inc() noexcept { m_cursor = (m_cursor + 1) & (m_capacity - 1); }

	public:
		array_deque_iterator() noexcept : array_deque_iterator(nullptr, 0, 0) {}

		array_deque_iterator(type* elements, size_t cursor, size_t capacity) noexcept :
			m_elements(elements),
			m_cursor(cursor),
			m_capacity(capacity) {}

		array_deque_iterator(const array_deque_iterator& rhs) noexcept = default;

		array_deque_iterator& operator = (const array_deque_iterator& rhs) noexcept {
			m_elements = rhs.m_elements;
			m_cursor = rhs.m_cursor;
			m_capacity = rhs.m_capacity;
			return *this;
		}

		array_deque_iterator& operator ++ () noexcept {
			inc();
			return *this;
		}

		array_deque_iterator& operator ++ (int) noexcept {
			inc();
			return *this;
		}

		bool operator == (const array_deque_iterator& rhs) const noexcept {
			if (this == &rhs) {
				return true;
			}

			return (m_elements == rhs.m_elements) && (m_cursor == rhs.m_cursor);
		}

		bool operator != (const array_deque_iterator& rhs) const noexcept {
			return !(*this == rhs);
		}

		type& operator * () noexcept { return m_elements[m_cursor]; }
		const type& operator * () const noexcept { return m_elements[m_cursor]; }

		type* operator -> () noexcept { return std::addressof(m_elements[m_cursor]); }
		const type* operator -> () const noexcept { return std::addressof(m_elements[m_cursor]); }
	};

	template<class type>
	class const_array_deque_iterator {

	public:
		using iterator_category = std::forward_iterator_tag;
		using value_type        = type;
		using difference_type   = ptrdiff_t;
		using pointer           = const type*;
		using reference         = const type &;

	private:
		const type* m_elements;
		size_t m_cursor, m_capacity;

		void inc() noexcept { m_cursor = (m_cursor + 1) & (m_capacity - 1); }

	public:
		const_array_deque_iterator() noexcept : const_array_deque_iterator(nullptr, 0, 0) {}

		const_array_deque_iterator(const type* elements, size_t cursor, size_t capacity) noexcept :
			m_elements(elements),
			m_cursor(cursor),
			m_capacity(capacity) {}

		const_array_deque_iterator(array_deque_iterator<type> it) noexcept :
			m_elements(it.m_elements),
			m_cursor(it.m_cursor),
			m_capacity(it.m_capacity){}

		const_array_deque_iterator(const const_array_deque_iterator& rhs) noexcept = default;

		const_array_deque_iterator& operator = (const const_array_deque_iterator& rhs) noexcept {
			m_elements = rhs.m_elements;
			m_cursor = rhs.m_cursor;
			m_capacity = rhs.m_capacity;
			return *this;
		}

		const_array_deque_iterator& operator = (const array_deque_iterator<type>& rhs) noexcept {
			m_elements = rhs.m_elements;
			m_cursor = rhs.m_cursor;
			m_capacity = rhs.m_capacity;
			return *this;
		}

		const_array_deque_iterator& operator ++ () noexcept {
			inc();
			return *this;
		}

		const_array_deque_iterator& operator ++ (int) noexcept {
			inc();
			return *this;
		}

		bool operator == (const const_array_deque_iterator& rhs) const noexcept {
			if (this == &rhs) {
				return true;
			}

			return (m_elements == rhs.m_elements) && (m_cursor == rhs.m_cursor);
		}

		bool operator != (const const_array_deque_iterator& rhs) const noexcept {
			return !(*this == rhs);
		}

		const type& operator * () const noexcept { return m_elements[m_cursor]; }
		const type* operator -> () const noexcept { return std::addressof(m_elements[m_cursor]); }
	};

	template<class type>
	class array_deque {

		static_assert(
			std::is_nothrow_default_constructible_v<type>,
			"concurrencpp::details::array_deque<type> - type must be no-throw default-constructible.");

		static_assert(
			std::is_nothrow_move_constructible_v<type>,
			"concurrencpp::details::array_deque<type> - type must be no-throw move-constructible.");

	private:
		std::unique_ptr<type[]> m_elements;
		size_t m_capacity, m_head, m_tail;

		constexpr static size_t k_initial_capacity = 16;

		void clear_impl() noexcept {
			m_capacity = 0;
			m_head = 0;
			m_tail = 0;
			m_elements.reset();
		}

		void shrink_impl() {
			assert(size() <= (m_capacity >> 3));
			const auto length = size();
			if (length == 0) {
				return clear_impl();
			}

			const auto new_capacity = m_capacity >> 2;
			auto new_buffer = std::make_unique<type[]>(new_capacity);
			auto new_buffer_ptr = new_buffer.get();
			auto elements = m_elements.get();

			if (m_head < m_tail) {
				std::move(elements + m_head, elements + m_head + length, new_buffer_ptr);
			}
			else if (m_head > m_tail) {
				const auto right_count = m_capacity - m_head;
				std::move(elements + m_head, elements + m_head + right_count, new_buffer_ptr);
				std::move(elements, elements + m_tail, new_buffer_ptr + right_count);
			}

			std::swap(m_elements, new_buffer);
			m_head = 0;
			m_tail = length;
			m_capacity = new_capacity;
		}

		void grow_impl() {
			assert(m_head == m_tail);
			const auto new_capacity = m_capacity << 1;
			auto new_buffer = std::make_unique<type[]>(new_capacity);
			auto new_buffer_ptr = new_buffer.get();
			auto elements = m_elements.get();
			std::move(elements + m_head, elements + m_capacity, new_buffer_ptr);
			std::move(elements, elements + m_head, new_buffer_ptr + m_capacity - m_head);
			std::swap(m_elements, new_buffer);
			m_head = 0;
			m_tail = m_capacity;
			m_capacity = new_capacity;
		}

		void ensure_initial_capacity() {
			if (m_capacity != 0) {
				assert(static_cast<bool>(m_elements));
				return;
			}

			m_elements = std::make_unique<type[]>(k_initial_capacity);
			m_capacity = k_initial_capacity;
		}

		void grow_if_needed() {
			if (m_head == m_tail) {
				grow_impl();
			}
		}

		void shrink_if_needed() {
			if (size() <= (m_capacity >> 3)) {
				shrink_impl();
			}
		}

	public:
		array_deque() noexcept : m_capacity(0), m_head(0), m_tail(0) {}

		array_deque(array_deque&& rhs) noexcept :
			m_elements(std::move(rhs.m_elements)),
			m_capacity(rhs.m_capacity),
			m_head(rhs.m_head),
			m_tail(rhs.m_tail) {
			rhs.m_capacity = 0;
			rhs.m_head = 0;
			rhs.m_tail = 0;
		}

		~array_deque() noexcept = default;

		size_t capacity() const noexcept { return m_capacity; }
		size_t size() const noexcept { return (m_tail - m_head) & (m_capacity - 1); }
		bool empty() const noexcept { return m_head == m_tail; }

		template<class ... argument_type>
		void emplace_front(argument_type&& ... arguments) {
			static_assert(
				std::is_constructible_v<type, argument_type&&...>,
				"concurrencpp::details::array_deque::emplace_front - can't build <<type>> from <<argument_types...>>");

			ensure_initial_capacity();

			auto elements = m_elements.get();
			m_head = (m_head - 1) & (m_capacity - 1);
			elements[m_head] = type(std::forward<argument_type>(arguments)...);
			grow_if_needed();
		}

		template<class ... argument_type>
		void emplace_back(argument_type&& ... arguments) {
			static_assert(
				std::is_constructible_v<type, argument_type...>,
				"concurrencpp::details::array_deque::emplace_front - can't build <<type>> from <<argument_types...>>");

			ensure_initial_capacity();

			auto elements = m_elements.get();
			elements[m_tail] = type(std::forward<argument_type>(arguments)...);;
			m_tail = (m_tail + 1) & (m_capacity - 1);
			grow_if_needed();
		}

		type pop_front() {
			assert(!empty());
			auto elements = m_elements.get();
			auto result = std::move(elements[m_head]);
			m_head = (m_head + 1) & (m_capacity - 1);
			shrink_if_needed();
			return result;
		}

		type pop_back() {
			assert(!empty());
			auto elements = m_elements.get();
			m_tail = (m_tail - 1) & (m_capacity - 1);
			auto result = std::move(elements[m_tail]);
			shrink_if_needed();
			return result;
		}

		void clear() noexcept { clear_impl(); }

		array_deque_iterator<type> begin() noexcept {
			return array_deque_iterator<type>(m_elements.get(), m_head, m_capacity);
		}

		const_array_deque_iterator<type> begin() const noexcept {
			return const_array_deque_iterator<type>(m_elements.get(), m_head, m_capacity);
		}

		array_deque_iterator<type> end() noexcept {
			return array_deque_iterator<type>(m_elements.get(), m_tail, m_capacity);
		}

		const_array_deque_iterator<type> end() const noexcept {
			return const_array_deque_iterator<type>(m_elements.get(), m_tail, m_capacity);
		}

		bool operator == (const array_deque& rhs) const noexcept {
			if (this == &rhs) {
				return true;
			}

			if (size() != rhs.size()) {
				return false;
			}

			return std::equal(begin(), end(), rhs.begin());
		}

		bool operator != (const array_deque& rhs) const noexcept {
			return !(*this == rhs);
		}
	};
}

#endif
