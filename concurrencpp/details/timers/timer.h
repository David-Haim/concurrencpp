#ifndef CONCURRENCPP_TIMER_H
#define CONCURRENCPP_TIMER_H

#include <atomic>
#include <memory>
#include <chrono>

#include "../forward_declerations.h"

namespace concurrencpp::details {	
	struct timer_traits {
		constexpr static size_t k_oneshot_timer_frequency = static_cast<size_t>(-1);
	};

	enum class timer_action {
		idle,
		fire,
		fire_delete
	};

	class timer_impl_base : public std::enable_shared_from_this<timer_impl_base> {

	public:
		using clock_type = std::chrono::high_resolution_clock;
		using time_point = std::chrono::time_point<clock_type>;
		using milliseconds = std::chrono::milliseconds;

	private:
		const std::weak_ptr<concurrencpp::timer_queue> m_timer_queue;
		const std::shared_ptr<concurrencpp::executor> m_executor;
		const size_t m_due_time;
		std::atomic_size_t m_frequency;
		time_point m_deadline; //set by the c.tor, changed only by the timer_queue thread.
		std::shared_ptr<timer_impl_base> m_next;
		timer_impl_base* m_prev;
	
		static time_point make_deadline(milliseconds diff) noexcept { return clock_type::now() + diff; }

	public:
		timer_impl_base(
			size_t due_time,
			size_t frequency,		
			std::shared_ptr<concurrencpp::executor> executor,
			std::weak_ptr<concurrencpp::timer_queue> timer_queue) noexcept;
		
		virtual ~timer_impl_base() noexcept = default;
		virtual void execute() = 0;	

		timer_action update(const time_point now);
		void schedule();

		time_point get_deadline() const noexcept;
		size_t get_frequency() const noexcept;
		size_t get_due_time() const noexcept;

		std::shared_ptr<concurrencpp::executor> get_executor() const noexcept;
		std::weak_ptr<concurrencpp::timer_queue> get_timer_queue() const noexcept;

		void set_next(std::shared_ptr<timer_impl_base> next) noexcept;
		std::shared_ptr<timer_impl_base> get_next(bool nullify_next = false) noexcept;
		void set_prev(timer_impl_base* prev) noexcept;
		timer_impl_base* get_prev(bool nullify_prev = false) noexcept;

		void set_new_frequency(size_t new_frequency) noexcept;

		bool operator > (const timer_impl_base& rhs) const noexcept { return get_deadline() > rhs.get_deadline(); }
	};

	template<class functor_type>
	class timer_impl final : public timer_impl_base {

	private:
		functor_type m_functor;

	public:
		template<class given_functor_type>
		timer_impl(
			size_t due_time,
			size_t frequency,
			std::shared_ptr<concurrencpp::executor> executor,
			std::weak_ptr<concurrencpp::timer_queue> timer_queue,
			given_functor_type&& functor) :
			timer_impl_base(due_time, frequency, std::move(executor), std::move(timer_queue)),
			m_functor(std::forward<given_functor_type>(functor)) {}

		virtual void execute() override {
			m_functor();
		}
	};
}

namespace concurrencpp {
	class timer {

	private:
		std::shared_ptr<details::timer_impl_base> m_impl;

		void throw_if_empty(const char* error_message) const;

	public:
		timer() noexcept = default;
		timer(std::shared_ptr<details::timer_impl_base> timer_impl) noexcept;
		~timer() noexcept;

		timer(timer&& rhs) noexcept = default;
		timer& operator = (timer&& rhs) noexcept;

		timer(const timer&) = delete;
		timer& operator = (const timer&) = delete;

		void cancel();

		size_t get_due_time() const;	
		size_t get_frequency() const;
		std::shared_ptr<concurrencpp::executor> get_executor() const;
		std::weak_ptr<concurrencpp::timer_queue> get_timer_queue() const;
	
		void set_frequency(size_t new_frequency);
	
		operator bool() const noexcept { return static_cast<bool>(m_impl); }	
	};
}

#endif //CONCURRENCPP_TIMER_H
