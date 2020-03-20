#include "mock_async_sql.h"

#include <chrono>
#include <thread>
#include <mutex>
#include <future>

namespace mock_async_sql {

	class mock_sql_runner {

	private:	
		std::mutex m_lock;
		std::vector<std::future<void>> m_tasks;

		/*
			Mock connection/query failure: randomize a failure in 5% of the cases
		*/
		static bool failed() noexcept {
			static const int dummy = [] {
				std::srand(::time(nullptr));
				return 0;
			}();

			const auto randomized_num = std::rand() % 100;
			return randomized_num <= 5;
		}

	public:
		mock_sql_runner() noexcept = default;
		
		~mock_sql_runner() noexcept {
			std::unique_lock<std::mutex> lock(m_lock);
			for (auto& task : m_tasks) {
				try {
					task.get();
				}
				catch (...) {
					//do nothing.
				}
			}

		}

		void mock_connection(
			std::shared_ptr<db_connection> conn_ptr,
			std::unique_ptr<connection_callback_base> cb) {
			auto future = std::async(std::launch::async, [conn_ptr, cb = std::move(cb)]() mutable {
				std::this_thread::sleep_for(std::chrono::seconds(2));
			
				if (failed()) {
					auto error_ptr = std::make_exception_ptr(std::runtime_error("db_connection::connect - can't connect."));
					return cb->on_connection(error_ptr, nullptr);
				}

				cb->on_connection(nullptr, conn_ptr);
			});

			std::unique_lock<std::mutex> lock(m_lock);
			m_tasks.emplace_back(std::move(future));
		}

		void mock_query_result(
			std::shared_ptr<db_connection> conn_ptr,
			std::unique_ptr<query_callback_base> cb) {
			auto future = std::async(std::launch::async, [conn_ptr, cb = std::move(cb)]() mutable {
				std::this_thread::sleep_for(std::chrono::seconds(2));

				if (failed()) {
					auto error_ptr = std::make_exception_ptr(std::runtime_error("db_connection::connect - can't connect."));
					return cb->on_query(error_ptr, nullptr, {});
				}

				std::vector<std::vector<std::any>> results;
				results.emplace_back(std::vector<std::any>{ "Daniel", "Williams", 34 });
				results.emplace_back(std::vector<std::any>{ "Sarah", "Jones", 31 });
				results.emplace_back(std::vector<std::any>{ "Mark", "Williams", 40 });
				results.emplace_back(std::vector<std::any>{ "Jack", "Davis", 37 });

				cb->on_query(nullptr, conn_ptr, std::move(results));
			});

			std::unique_lock<std::mutex> lock(m_lock);
			m_tasks.emplace_back(std::move(future));
		}
	};	

	mock_sql_runner global_runner;
}

void mock_async_sql::db_connection::connect(std::unique_ptr<connection_callback_base> connection_callback){
	global_runner.mock_connection(shared_from_this(), std::move(connection_callback));
}

void mock_async_sql::db_connection::query(std::string query_string, std::unique_ptr<query_callback_base> query_callback){
	global_runner.mock_query_result(shared_from_this(), std::move(query_callback));
}
