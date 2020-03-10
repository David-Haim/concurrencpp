#ifndef CONCURRENCPP_TASK_H
#define CONCURRENCPP_TASK_H

#include <typeinfo>
#include <exception>
#include <type_traits>
 
#include <cassert>

namespace concurrencpp::details {
	class task_context;
	struct task_interface;	
	template<class functor_type> class task_impl;
}

namespace concurrencpp::details {
	struct task_traits {
		constexpr static size_t k_inline_buffer_size = sizeof(void*) * 5;

		using buffer_type = typename std::aligned_storage_t<k_inline_buffer_size, alignof(std::max_align_t)>;
	};

	template<class functor_type>
	class functor_traits {

	private:
		template<class type, class ignored = void>
		struct has_cancel_impl : std::false_type {};

		template<class type>
		struct has_cancel_impl<type,
			typename std::void_t<
			decltype(std::declval<type>().cancel(std::declval<std::exception_ptr>())),
			typename std::enable_if_t<std::is_same_v<decltype(std::declval<type>().cancel(std::declval<std::exception_ptr>())), void>>
			>> : std::bool_constant<noexcept(std::declval<type>().cancel(std::declval<std::exception_ptr>()))> {};

	public:
		using decayed_type = typename std::decay_t<functor_type>;

		constexpr static auto is_inlinable = 
			sizeof(decayed_type) <= task_traits::k_inline_buffer_size &&
			std::is_nothrow_move_constructible_v<decayed_type>;

		constexpr static auto is_nothrow_movable = std::is_nothrow_move_constructible_v<decayed_type>;

		using has_cancel = has_cancel_impl<decayed_type>;	
	};

	struct task_interface {
		virtual ~task_interface() noexcept = default;
		virtual void execute() = 0;
		virtual void move_to(task_context& dest) noexcept = 0;
		virtual void cancel(std::exception_ptr cancel_reason) noexcept = 0;
		virtual const std::type_info& type() const noexcept = 0;
	};

	class task_context {

	private:
		task_interface* m_pointer;
		typename task_traits::buffer_type m_buffer;

		task_interface* buffer_ptr() noexcept {
			return reinterpret_cast<task_interface*>(std::addressof(m_buffer));
		}

		const task_interface* buffer_ptr() const noexcept {
			return reinterpret_cast<const task_interface*>(std::addressof(m_buffer));
		}

	public:
		task_context() noexcept : m_pointer(nullptr) {}
		~task_context() noexcept { clear(); }

		task_context(task_context&& rhs) noexcept;
		task_context& operator = (task_context&& rhs) noexcept;

		task_interface* pointer() noexcept { return m_pointer; }
		const task_interface* pointer() const noexcept { return m_pointer; }

		bool is_inlined() const noexcept { 
			return m_pointer == nullptr || m_pointer == buffer_ptr();
		}

		operator bool() const noexcept { return pointer() != nullptr; }

		void clear() noexcept;

		const std::type_info& type() const noexcept;

		template<class functor_type>
		void build(functor_type&& functor) {
			using decayed_functor_type = std::decay_t<functor_type>;

			static_assert(
				std::is_base_of_v<task_interface, decayed_functor_type>,
				"concurrencpp::details::task_context::build - <<functor_type>> doesn't inherit from <<task_interface>>.");

			assert(!static_cast<bool>(*this));

			if (functor_traits<functor_type>::is_inlinable) {
				new (buffer_ptr()) decayed_functor_type(std::forward<functor_type>(functor));
				m_pointer = buffer_ptr();
				return;
			}

			m_pointer = new decayed_functor_type(std::forward<functor_type>(functor));
		}
	};

	/*
		wraps <<functor_type>> polymorphically, injecting default <<void cancel(std::exc_ptr) noexcept>>
		if no cancel function was provided.
	*/
	template<class functor_type>
	class task_impl final : public task_interface {

		using has_cancel = typename functor_traits<functor_type>::has_cancel;

	private:
		functor_type m_functor;

		void cancel_impl(std::exception_ptr e, std::true_type /*has_cancel*/) noexcept {
			m_functor.cancel(e);
		}

		void cancel_impl(std::exception_ptr e, std::false_type /*has_cancel*/) const noexcept {
			(void)e;
		}

	public:
		template<class undecayed_functor_type>
		task_impl(undecayed_functor_type&& functor) :
			m_functor(std::forward<undecayed_functor_type>(functor)) {}

		task_impl(task_impl&&) noexcept(functor_traits<functor_type>::is_nothrow_movable) = default;
	
		void execute() { m_functor(); }

		void cancel(std::exception_ptr reason) noexcept { 
			cancel_impl(reason, has_cancel()); 
		}

		virtual void move_to(task_context& dest) noexcept {
			dest.build<task_impl<functor_type>>(std::move(*this));
		}

		virtual const std::type_info& type() const noexcept {
			return typeid(functor_type);
		}
	};
}

namespace concurrencpp {
	class task {

	private:
		details::task_context m_ctx;

	public:
		task() noexcept = default;
		~task() noexcept = default;

		task(std::nullptr_t) noexcept : task(){}

		template<class functor_type>
		task(functor_type&& functor) : task() {
			using decayed_type = std::decay_t<functor_type>;

			static_assert(std::is_invocable_v<decayed_type>,
				"concurrencpp::task - <<functor_type>> doesn't supply operator(), or the supplied operator does not take void as only argument.");

			m_ctx.build<details::task_impl<decayed_type>>(std::forward<functor_type>(functor));
		}

		task(task&& rhs) noexcept = default;
		task& operator =(task&& rhs) noexcept = default;

		void operator()();

		void clear() noexcept;
		void cancel(std::exception_ptr reason) noexcept;

		operator bool() const noexcept { return static_cast<bool>(m_ctx); }
	
		const std::type_info& type() const noexcept { return m_ctx.type(); }

		bool uses_sbo() const noexcept { return m_ctx.is_inlined(); }
	};
}

#endif // !CONCURRENCPP_TASK_H
