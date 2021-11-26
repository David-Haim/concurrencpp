#include <fstream>
#include <iostream>
#include <filesystem>

#include "concurrencpp/concurrencpp.h"

concurrencpp::generator<std::string_view> read_lines(std::string_view text) {
    std::string_view::size_type pos = 0;
    std::string_view::size_type prev = 0;

    while ((pos = text.find('\n', prev)) != std::string::npos) {
        co_yield text.substr(prev, pos - prev);
        prev = pos + 1;
    }

    co_yield text.substr(prev);
}

concurrencpp::result<void> read_file_lines(const std::filesystem::path& path,
                                           std::shared_ptr<concurrencpp::thread_pool_executor> background_executor,
                                           std::shared_ptr<concurrencpp::thread_pool_executor> thread_pool_executor) {
    // make sure we don't block in a thread that is used for cpu-processing
    co_await concurrencpp::resume_on(background_executor);

    std::ifstream stream(path.c_str(), std::ios::binary | std::ios::in);
    std::string file_content(std::istreambuf_iterator<char>(stream), {});

    // make sure we don't process cpu-bound tasks on the background executor
    co_await concurrencpp::resume_on(thread_pool_executor);

    for (const auto& line : read_lines(file_content)) {
        std::cout << "read a new line. size: " << line.size() << std::endl;
        std::cout << "line: " << std::endl;
        std::cout << line;
        std::cout << "\n==============" << std::endl;
    }
}

int main(const int argc, const char* argv[]) {
    if (argc < 2) {
        const auto help_msg = "please pass all necessary arguments\n argv[1] - the file to be read\n";
        std::cerr << help_msg << std::endl;
        return -1;
    }

    const auto file_path = std::string(argv[1]);

    concurrencpp::runtime runtime;
    const auto thread_pool_executor = runtime.thread_pool_executor();
    const auto background_executor = runtime.background_executor();

    read_file_lines(file_path, thread_pool_executor, background_executor).get();
    return 0;
}