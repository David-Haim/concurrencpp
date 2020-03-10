#ifndef CONCURRENCPP_THREAD_POOL_LISTENER_H
#define CONCURRENCPP_THREAD_POOL_LISTENER_H

#include <thread>

namespace concurrencpp::details {
	struct thread_pool_listener_base {
		virtual ~thread_pool_listener_base() = default;
		virtual void on_thread_created(std::thread::id id) = 0;
		virtual void on_thread_waiting(std::thread::id id) = 0;
		virtual void on_thread_resuming(std::thread::id id) = 0;
		virtual void on_thread_idling(std::thread::id id) = 0;
		virtual void on_thread_destroyed(std::thread::id id) = 0;
	};
}

#endif // !CONCURRENCPP_THREAD_FACTORY_H
