## concurrencpp, the C++ concurrency library

(note: documentation is still under development)

concurrencpp allows applications to write asynchronous code easily and safetly by using executors and coroutines.
By using concurrencpp applications can break down big procedures that need to be processed asynchronously into smaller tasks that run concurrently and work in a co-operative manner to achieve the wanted result. 
concurrencpp also allows applications to write parallel algorithms more easily by using parallel coroutines.

concurrencpp main advantages are: 
* Being able to write non blocking, asynchronous code easily by using the C++20 coroutines and the `co_await` keyword.
* Being able to write modern concurrent code without having to rely on low-level concurrency primitives like locks and condition variables .
* The concurrency runtime manages all low-level resources such as threads automatically
* concurrencpp provides various types of commonly used executors with a complete coroutines integration. 
* Applications can extend the library by using their own provided executors.

concurrencpp is task-centric. A task is an asynchronous operation. Tasks provide higher level of abstraction for concurrent code than traditional thread-centric approaches. Tasks can be chained together, meaning that tasks pass their asynchronous result 
from one to another, where the result of one task is used as if it were a parameter or an intermediate  value of another ongoing task.
In concurrencpp, the concept of tasks is represented by coroutines. This allows tasks to be suspended, waiting for other tasks to finish, and chained using `co_await`, and thus solving the consumer-producer problem elegantly by giving the concurrent code a synchronous look. 

**Executors**

In the context of concurrencpp, an executor is an object that is able to schedule and run coroutines.
Executors simplify the work of managing resources such as threads, thread pools and task queues by decoupling them away from application code.
Executors provide a unified way of scheduling and executing coroutines, since they all extend `concurrencpp::executor`.
The main power of concurrencpp executors comes from their ability to schedule user provided callables by turning them into coroutines first, scheduling them to run asynchronously, and finally marshal the asynchronous result (or exception) back to the caller.

`concurrencpp::executor` interface

```cpp
class executor {
	/*
		Initializes a new executor and gives it  a name.
	*/
	executor(std::string_view name);

	virtual ~executor() noexcept = default;

	/*
		The name of the executor, used for logging and debugging.
	*/
	const std::string name;

	/*
		Schedules a suspended coroutine to run in this executor. 
		Throws concurrencpp::errors::executor_shutdown exception if shutdown was called before.
	*/
	virtual void enqueue(std::experimental::coroutine_handle<> task) = 0;

	/*
		Schedules a range of suspended coroutines to run in this executor. 
		Throws concurrencpp::errors::executor_shutdown exception if shutdown was called before.
	*/	
	virtual void enqueue(std::span<std::experimental::coroutine_handle<>> tasks) = 0;

	/*
		The maximum amount of real OS threads this executor supports. 
		The actual number might be smaller than this. 
		returns numeric_limits<int>::max if the executor does not have a limit for OS threads 
	*/
	virtual int max_concurrency_level() const noexcept = 0;

	/* 
		Returns true if shutdown was called before, false otherwise 
	*/ 
	virtual bool shutdown_requested() const noexcept = 0;

	/* 
		Shuts down the executor:
		- Tells underlying threads to exit their work loop
		- Destroyed unexecuted coroutines
		- Makes subsequent calls to enqueue, post, submit, bulk_post and 
			bulk_submit to throw concurrencpp::errors::executor_shutdown exception
		- Makes shutdown_requested return true.
	*/
	virtual void shutdown() noexcept = 0;

	/*
		Turns a callable into a suspended coroutine and schedules it to run in this executor using enqueue.
	*/
	template<class callable_type>
	void post(callable_type&& callable);

	/*
		Like post, but returns a result object that marshals the asynchronous result.
	*/
	template<class callable_type>
	result<type> submit(callable_type&& callable);

	/*
		Turns an array of callables into an array of suspended coroutines and schedules them to run in this executor using enqueue.
	*/
	template<class callable_type>
	void bulk_post(std::span<callable_type> callable_list);

	/*
		Like bulk_post, but returns an array of result objects that marshal the asynchronous results.
		Order is preserved. 
	*/	
	template<class callable_type>
	std::vector<concurrencpp::result<type>> bulk_submit(std::span<callable_type> callable_list);
};
```

***Executor types:***

As mentioned above, concurrencpp provides commonly used executors. These excutore types are:

* **thread pool executor** - a general purpose executor that maintains a pool of threads. 
The thread pool executor is suitable for relatively short cpu-bound tasks that don't block. Applications are encouraged to use this executor as the default executor for non blocking functions. 
The concurrencpp thread pool provides dynamic thread injection and dynamic work balancing for its enqueued tasks.

* **blocking executor** - a thredpool executor with a larger pool of threads. Suitable for launching short blocking tasks like file io and db queries.

* **thread executor** - an executor that launches each enqueued task to run on a new thread of execution. Threads are not reused. 
This executor is good for long running tasks, like objects that run a work loop, or long blocking operations.

* **worker thread executor** - a single thread executor that maintains a single task queue. Suitable when applications want a dedicated thread that executes many related tasks.

* **manual executor** - an executor that does not execute coroutines by itself. Application code can execute previously enqueued tasks by manually invoking its execution methods.

* **inline executor** - mainly used to override the behavior of other executors. Enqueuing a task is equivalent to invoking it inline.

**Result objects**

Asynchronous values and exceptions can be consumed using the concurrencpp result objects.
A result object is a conduit for the asynchronous coroutine result, like `std::future`. 
When a coroutine finishes execution, it either returns a valid value or throws an exception. 
In either case, this asynchronous result is marshaled to the consumer of the result object.
The result state therefore, very from `idle` (the asynchronous result or exception aren't ready yet) to `value` (the coroutine terminated by returning a valid value) to `exception` (the coroutine terminated by throwing an exception).  
Result objects can be polled for their state, waited, resolved or awaited. 
Awaiting a result object by using `co_await` (and by doing so, turning the current function into a coroutine as well) is the preferred way of consuming result objects.  
concurrencpp also provide the `null_result` type. in this case, any returned value or thrown exception will be dropped. 

`concurrencpp::result` API
	
```cpp
class result{
	/*
		Creates an empty result that isn't associated with any task.
	*/
	result() noexcept = default;

	/*
		Destroyes the result. associated tasks are not cancelled.
	*/	
	~result() noexcept = default;

	/*
		Moves the content of rhs to *this. After this call, rhs is empty. 
	*/
	result(result&& rhs) noexcept = default;

	/*
		Moves the content of rhs to *this. After this call, rhs is empty. Returns *this.		
	*/
	result& operator = (result&& rhs) noexcept = default;

	/*
		Returns true if this is a non empty result. Applications must not use this object if static_cast<bool>(*this) is false. 
	*/
	operator bool() const noexcept;

	/*
		Queries the status of *this. The return value might be any of result_status::idle,
		result_status::value or result_status::exception.
		Throws concurrencpp::errors::empty_result if *this is empty.		
	*/
	result_status status() const ;

	/*
		Blocks the current thread of execution until this result is ready, when its status is either  result_status::value or result_status::exception.
		Throws concurrencpp::errors::empty_result if *this is empty.					
	*/
	void wait();

	/*
		Blocks until this result is ready or duration has passed. Returns the status of this result after unblocking.
		Throws concurrencpp::errors::empty_result if *this is empty.					
	*/
	template<class duration_unit, class ratio>
	result_status wait_for(std::chrono::duration<duration_unit, ratio> duration);

	/*
		Blocks until this result is ready or timeout_time is reached. Returns the status of this resut after unblocking.
		Throws concurrencpp::errors::empty_result if *this is empty.					
	*/
	template< class clock, class duration >
	result_status wait_until(std::chrono::time_point<clock, duration> timeout_time);

	/*
		Blocks the current thread of execution until this result is ready, when its status is either result_status::value or result_status::exception.
		If the result is a valid value, it is returned, otherwise, get rethrows the asynchronous exception.		
		Throws concurrencpp::errors::empty_result if *this is empty.					
	*/
	type get();

	/*
		Returns an awaitable used to await this result.
		If the reasult is already ready - the current coroutine resumes immediatly in the calling thread of execution.
		If the result is not ready yet, the current coroutine is suspended and resumed when the asynchronous result is ready, by the thread which had set the asynchronous value or exception.
		In either way, after resuming, if the result is a valid value, it is returned. 
		Otherwise, operator co_await rethrows the asynchronous exception.
		Throws concurrencpp::errors::empty_result if *this is empty.							
	*/
	auto operator co_await();

	/*
		Returns an awaitable used to await this result.
		If the result is not ready yet, the current coroutine is suspended and resumed when the asynchronous result is ready, by scheduling the current coroutine via executor.
		If the reasult is already ready - the behaviour depends on the value of force_rescheduling:
			if force_rescheduling = true, then the current coroutine is forcefully suspended and resumed via execution
			if force_rescheduling = false, then the current coroutine resumes immediatly in the calling thread of execution.

		In either way, after resuming, if the result is a valid value, it is returned. 
		Otherwise, operator co_await rethrows the asynchronous exception.
		throws concurrencpp::errors::empty_result if *this is empty.		
		If this result is ready and force_rescheduling=true, throws any exception that executor::enqueue may throw.	
	*/
	auto await_via(
		std::shared_ptr<concurrencpp::executor> executor,
		bool force_rescheduling = true);

	/*
		Returns an awaitable used to resolve this result.
		Resolving a result means awaiting it to become ready, but unlike awaiting, resolving does not return or re-throw  the asynchronous result. 
		*this is returned in a non empty form and guaranteed that its status is not result_status::idle. 
		Throws concurrencpp::errors::empty_result if *this is empty.
	*/	
	auto resolve();

	/*
		Returns an awaitable used to resolve this result.
		If the result is not ready yet, the current coroutine is suspended and resumed when the asynchronous result is ready, by scheduling the current coroutine via executor.
		If the reasult is already ready - the behaviour depends on the value of force_rescheduling:
			if force_rescheduling = true, then the current coroutine is forcefully suspended and resumed via execution
			if force_rescheduling = false, then the current coroutine resumes immediatly in the calling thread of execution.
		In either way, after resuming, *this is returned in a non empty form and guaranteed that its status is not result_status::idle.
		throws concurrencpp::errors::empty_result if *this is empty.		
		If this result is ready and force_rescheduling=true, throws any exception that executor::enqueue may throw.					
	*/
	auto resolve_via(
		std::shared_ptr<concurrencpp::executor> executor, bool force_rescheduling = true);
};
```

**Parallel coroutines**
In concurrencpp, a parallel coroutine is a function that:

1. Returns any of `concurrencpp::result<T>` / `concurrencpp::null_result` . 
1. Gets `concurrencpp::executor_tag` as its first argument .
1. Gets any of `type*` / `type&` / `std::shared_ptr<type>`, where `type` is a concrete class of `concurrencpp::executor` as its second argument.
1. Contains any of `co_await` or `co_return` in its body.

In this case, the function is a parallel coroutine: 
concurrencpp will start the coroutine suspended and immediately re-schedule it to run in the provided executor. 
Applications can then consume the result of the parallel coroutine by using the returned result object. 
This technique is especially good for parallel algorithms that require recursion, or applications that use the fork-join concurrency model.

**Creating and scheduling coroutines**

There are few ways to create and schedule coroutines in concurrencpp

-  By calling any of `executor::post`, `executor::submit`, `executor::bulk_post` or `executor::bulk_submit`. 
In this case the application provides a callable and depending on the selected method, might return a result object to the caller. The callable is then turned into a coroutine and scheduled to run in the executor.

-  By using a parallel coroutine.

-  By retunring a result object.
When a function returns any of `concurrencpp::result` / `concurrencpp::null_result` and has at least on of `co_await`/`co_return`, then this function is a coroutine that starts executing immediately in the caller thread without suspension. 

- By using a `result_promise<type>` 
It is also possible to manually marshal asynchronous results using `result_promise`. 
`result_promise` is similiar to `std::promise` - applications can pass results from producers to consumers without having the producer function being a coroutine. In this case the result object is associated with a `result_promise` object and its usage is the same as a regular result object that's associated with a coroutine. 

**Timers and Timer queues**

concurrencpp also provides timers and timer queues. 
Timers are objects that schedule actions to run on an executor within a well defined interval of time.
A timer queue is a concurrencpp worker that manages a collection of timers and processes them in just one thread of execution.

Timers have four properties that describe them:

1. A callable (and potentially its arguments) - a callable that will be scheduled to run periodically.
1. An executor - in which the callable will be scheduled to run as a coroutine
1. due time - from the time of creation, the interval in milliseconds in which the timer will be scheduled to run for the first time 
1. frequency - from the time the timer callable was scheduled for the first time, the interval in milliseconds the callable will be schedule to run periodically, until the timer is destructed or cancelled.

There are two more flavors of timers in concurrencpp: oneshot timers and delay objects. 

*  A oneshot timer is a one-time timer with only a due time - after it schedules its callable to run once it never reschedule it to run again.  

* A delay object is a result object that becomes ready when its due time is reached. Applications can co_await on this result object for a non-blocking way to delay the execution of a coroutine. 

```cpp   
class timer_queue {
	/*
		Destroyes *this and cancels all associated timers.
	*/
	~timer_queue() noexcept;

	/*
		Creates a new running timer where *this is associated timer_queue
	*/
	template<class callable_type>
	timer make_timer(
		size_t due_time,
		size_t frequency,
		std::shared_ptr<concurrencpp::executor> executor,
		callable_type&& callable);

	/*
		Creates a new running timer where *this is associated timer_queue
	*/
	template<class callable_type, class ... argumet_types>
	timer make_timer(
		size_t due_time,
		size_t frequency,
		std::shared_ptr<concurrencpp::executor> executor,
		callable_type&& callable,
		argumet_types&& ... arguments);

	/*
		Creates a new one shot timer where *this is associated timer_queue
	*/
	template<class callable_type>
	timer make_one_shot_timer(
		size_t due_time,
		std::shared_ptr<concurrencpp::executor> executor,
		callable_type&& callable);

	/*
		Creates a new one shot timer where *this is associated timer_queue
	*/
	template<class callable_type, class ... argumet_types>
	timer make_one_shot_timer(
		size_t due_time,
		std::shared_ptr<concurrencpp::executor> executor,
		callable_type&& callable,
		argumet_types&& ... arguments);

	/*
		Creates a new delay object where *this is associated timer_queue
	*/
	result<void> make_delay_object(size_t due_time, std::shared_ptr<concurrencpp::executor> executor);
	};

	class timer {
	/*
		Creates an empty timer that does not run.
	*/
	timer() noexcept = default;

	/*
		Cancels the timer, if not empty.
	*/
	~timer() noexcept;

	/*
		Moves the content of rhs to *this.
		rhs is empty afte this call.
	*/
	timer(timer&& rhs) noexcept = default;

	/*
		Moves the content of rhs to *this.
		rhs is empty afte this call.
		Returns *this
	*/
	timer& operator = (timer&& rhs) noexcept;

	/*
		Cancels this timer. after this call, the associated timer_queue will not schedule *this to run again.
		After this call, *this is empty.
		Has no affect if *this is empty or the timer_queue is expired. 
	*/
	void cancel();

	/*
		Returns the due time in millesconds the timer was defined with.
		Throws concurrencpp::errors::empty_timer is *this is empty.
	*/
	size_t get_due_time() const;

	/*
		Returns the executor the timer was defined with.	
		Throws concurrencpp::errors::empty_timer is *this is empty.
	*/
	std::shared_ptr<executor> get_executor() const;

	/*
		Returns the timer_queue the timer was defined with.
		Throws concurrencpp::errors::empty_timer is *this is empty.
	*/
	std::weak_ptr<timer_queue> get_timer_queue() const;

	/*
		Returns the frequency in millesconds the timer was defined with.	
		Throws concurrencpp::errors::empty_timer is *this is empty.
	*/
	size_t get_frequency() const;

	/*
		Sets new frequency for this timer. Callables that have been scheduled to run at this point are not affected.	
		Throws concurrencpp::errors::empty_timer is *this is empty.
	*/
	void set_frequency(size_t new_frequency);

	/*
		Returns true is *this is not an empty timer, false otherwise.	
	*/
	operator bool() const noexcept;
};
```

**The runtime object**

The concurrencpp runtime object is the glue that sticks all the components above to a complete and cohesive mechanism of managing asynchronous actions. 
The concurrencpp runtime object is the agent used to acquire, store and create new executors. The runtime must be created as a value type as soon as the main function starts to run. 
When the concurrencpp runtime gets out of scope, it iterates over its stored executors and shuts them down one by one by calling `executor::shutdown`. Executors then exit their inner work loop and any subsequent attempt to schedule a new coroutine will throw a `concurrencpp::executor_shutdown` exception. The runtime also contains the global timer queue used to create timers and delay objects.

`concurrencpp::runtime` API

```cpp
class runtime {
	/*
		Creates a runtime object with default options.	
	*/
	runtime();

	/*
		Creates a runtime object with user defined options.
	*/
	runtime(const concurrencpp::runtime_options& options);

	/*
		Destroys this runtime object. 
		Calls executor::shutdown on each monitored executor.
	*/
	~runtime() noexcept;

	/*
		Returns this runtime timer queue used to create new times.
	*/
	std::shared_ptr<concurrencpp::timer_queue> timer_queue() const noexcept;

	/*
		Returns this runtime concurrencpp::inline_executor
	*/
	std::shared_ptr<concurrencpp::inline_executor> inline_executor() const noexcept;

	/*
		Returns this runtime concurrencpp::thread_pool_executor
	*/
	std::shared_ptr<concurrencpp::thread_pool_executor> thread_pool_executor() const noexcept;

	/*
		Returns this runtime concurrencpp::background_executor
	*/
	std::shared_ptr<concurrencpp::thread_pool_executor> background_executor() const noexcept;

	/*
		Returns this runtime concurrencpp::thread_executor
	*/
	std::shared_ptr<concurrencpp::thread_executor> thread_executor() const noexcept;

	/*
		Creates a new concurrencpp::worker_thread_executor and registers it in this runtime.
	*/
	std::shared_ptr<concurrencpp::worker_thread_executor> make_worker_thread_executor();

	/*
		Creates a new concurrencpp::manual_executor and registers it in this runtime.
	*/
	std::shared_ptr<concurrencpp::manual_executor> make_manual_executor();

	/*
		Creates a new user defined executor and registers it in this runtime.
		executor_type must be a valid concrete class of concurrencpp::executor 
	*/
	template<class executor_type, class ... argument_types>
	std::shared_ptr<executor_type> make_executor(argument_types&& ... arguments);

	/*
		returns the version of concurrencpp that the library was built with.
	*/
	static std::tuple<unsigned int, unsigned int, unsigned int> version() noexcept;
};
```
