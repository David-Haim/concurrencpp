/*
        It's the end of February and a company would like to congratulate all of
   its workers who were born in March. To do so, the company uses an
   asynchronous sql library, that uses callbacks to marshal asynchronous results
   and events.

        When using the library vanilla style, the application creates a
   db_connection object, calls db_connection::connect() and provides a callback
   that receives the asynchronous error or an open connection to the database.

        The application then proceeds calling db_connection::query() and
   provides a callback that receives the asynchronous error or the results in
   the form of std::vector< std::vector < std::any > > (a dynamic table).

        This example shows how to marshal asynchronous results and errors using
   concurrencpp::result_promise and concurrencpp::result pair. this allows
   applications to avoid callback-style code flow and allows using coroutines
   instead.

        Do note that this example uses a mock SQL library and no real
   connection/query is going to be executed. The constants are made up and
   results are hard-coded. This example is only good for showing how to use
   third-party libraries with concurrencpp.

        Library's API:

        mock_async_sql::db_connection{
                db_connection(db_url, username, password);
                void connect(connection_cb => (error, connection){ ... });
                void query(query_string, query_cb => (error, results){ ... });
        };

        connection_cb must inherit from mock_async_sql::connection_callback_base
   and override its on_connect method. query_cb must inherit from
   mock_async_sql::query_callback_base and override its on_query method.
*/

#include "mock_async_sql.h"
#include "concurrencpp/concurrencpp.h"

#include <iostream>

using concurrencpp::result;
using concurrencpp::result_promise;

using mock_async_sql::db_connection;
using mock_async_sql::connection_callback_base;
using mock_async_sql::query_callback_base;

result<std::shared_ptr<db_connection>> connect_async() {
    class connection_callback final : public connection_callback_base {

       private:
        result_promise<std::shared_ptr<db_connection>> m_result_promise;

       public:
        connection_callback(result_promise<std::shared_ptr<db_connection>> result_promise) noexcept :
            m_result_promise(std::move(result_promise)) {}

        void on_connection(std::exception_ptr error, std::shared_ptr<db_connection> connection) override {
            if (error) {
                return m_result_promise.set_exception(error);
            }

            m_result_promise.set_result(std::move(connection));
        }
    };

    auto connection = std::make_shared<db_connection>("http://123.45.67.89:8080/db", "admin", "password");
    result_promise<std::shared_ptr<db_connection>> result_promise;
    auto result = result_promise.get_result();
    std::unique_ptr<connection_callback_base> callback(new connection_callback(std::move(result_promise)));

    connection->connect(std::move(callback));

    return result;
}

result<std::vector<std::vector<std::any>>> query_async(std::shared_ptr<db_connection> open_connection) {
    using query_result_type = std::vector<std::vector<std::any>>;

    class query_callback final : public query_callback_base {

       private:
        result_promise<query_result_type> m_result_promise;

       public:
        query_callback(result_promise<query_result_type> result_promise) noexcept : m_result_promise(std::move(result_promise)) {}

        void on_query(std::exception_ptr error, std::shared_ptr<db_connection> connection, query_result_type results) override {
            (void)connection;
            if (error) {
                return m_result_promise.set_exception(error);
            }

            m_result_promise.set_result(std::move(results));
        }
    };

    result_promise<query_result_type> result_promise;
    auto result = result_promise.get_result();
    std::unique_ptr<query_callback_base> callback(new query_callback(std::move(result_promise)));

    open_connection->query("select first_name, last_name, age from db.users where birth_month=3", std::move(callback));
    return result;
}

void print_result(const std::vector<std::vector<std::any>>& results) {
    for (const auto line : results) {
        assert(line.size() == 3);

        assert(line[0].has_value());
        assert(line[0].type() == typeid(const char*));

        const auto first_name = std::any_cast<const char*>(line[0]);

        assert(line[1].has_value());
        assert(line[1].type() == typeid(const char*));

        const auto last_name = std::any_cast<const char*>(line[1]);

        assert(line[2].has_value());
        assert(line[2].type() == typeid(int));

        const auto age = std::any_cast<int>(line[2]);

        std::cout << first_name << " " << last_name << " " << age << std::endl;
    }
}

concurrencpp::result<void> print_workers_born_in_march() {
    auto connection_ptr = co_await connect_async();
    auto results = co_await query_async(connection_ptr);
    print_result(results);
}

int main() {
    try {
        print_workers_born_in_march().get();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
