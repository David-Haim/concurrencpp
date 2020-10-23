#ifndef MOCK_ASYNC_SQL_H
#define MOCK_ASYNC_SQL_H

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <any>

namespace mock_async_sql {
    class db_connection;

    struct connection_callback_base {
        virtual ~connection_callback_base() noexcept = default;

        virtual void on_connection(std::exception_ptr, std::shared_ptr<db_connection>) = 0;
    };

    struct query_callback_base {
        virtual ~query_callback_base() noexcept = default;

        virtual void on_query(std::exception_ptr, std::shared_ptr<db_connection>, std::vector<std::vector<std::any>>) = 0;
    };

    class db_connection : public std::enable_shared_from_this<db_connection> {

       public:
        db_connection(std::string db_url, std::string username, std::string password) {
            (void)db_url;
            (void)username;
            (void)password;
        }

        void connect(std::unique_ptr<connection_callback_base> connection_callback);
        void query(std::string query_string, std::unique_ptr<query_callback_base> query_callback);
    };
}  // namespace mock_async_sql

#endif
