#ifndef CONCURRENCPP_FORWARD_DECLERATIONS_H
#define CONCURRENCPP_FORWARD_DECLERATIONS_H

namespace concurrencpp {
	template<class type> class result;
	template<class type> class result_promise;

	class runtime;

	class timer_queue;
	class timer;

	struct executor;
	class inline_executor;
	class thread_pool_executor;
	class parallel_executor;
	class background_executor;
	class thread_executor;
	class worker_thread_executor;
	class manual_executor;
}

namespace concurrencpp::details {
	class thread_pool;
	class thread_group;
	class single_worker_thread;
}

#endif //FORWARD_DECLERATIONS_H
