#define BOOST_COROUTINES_NO_DEPRECATION_WARNING 
#include <iostream>
#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/use_future.hpp>
#include <thread>
#include <chrono>
#include <future>
#include <random>

using boost::system::error_code;
namespace asio = boost::asio;


template<typename CompletionToken>
auto async_operate(CompletionToken token) {
    asio::async_completion<CompletionToken, void(error_code, std::string)> init(token);
    
    std::cout << __func__ << " thread id=" << std::this_thread::get_id() << std::endl;

    // auto op = asio::bind_executor(ex, [](boost::system::error_code ec, std::string s) {
    //     std::cout << s << std::endl;
    // });
    // op(boost::system::error_code(), "aaps");
    
    // auto handler = init.completion_handler;
    std::thread tr1([handler = init.completion_handler]() {
        std::cout << "tr1 thread id=" << std::this_thread::get_id() << std::endl;
        using namespace std::chrono_literals;
        std::mt19937 mt{std::random_device{}()};
        std::this_thread::sleep_for(3s);
        auto ex = asio::get_associated_executor(handler);
        asio::dispatch(ex, [handler, ex]() mutable {
            std::string s("aaps");
            handler(boost::system::error_code(), s);
            std::cout << "handler this_thread id=" << std::this_thread::get_id() << std::endl;
        });
    });
    tr1.detach();

    return init.result.get();
}

class AsyncTest {
public:
    asio::io_context context_;
    using Executor = asio::io_context::executor_type;
    asio::executor_work_guard<Executor> work_;
    using Signature = void(boost::system::error_code);
    std::thread thread_;
    asio::yield_context* yield_;
    // asio::executor_binder<std::function<Signature>, asio::io_context>* hh;

    AsyncTest()
        :work_(asio::make_work_guard(context_))
    {
        thread_ = std::move(std::thread{[this](){ context_.run(); }});
    }
    ~AsyncTest() {
        thread_.join();
    }

    template<typename ExecutionContext, typename CompletionToken>
    auto async_wait(ExecutionContext&& ctx, CompletionToken&& token) {
        std::cout << __func__ << std::endl;
        asio::async_completion<CompletionToken, void(boost::system::error_code)> init(token);
        auto& handler = init.completion_handler;
        auto ex = asio::get_associated_executor(handler);
        auto h = asio::bind_executor(ex, std::move(handler));
        std::thread tr{[&]() {
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(3s);
            asio::post(ex, std::move(h));
        }};
        tr.detach();
        std::cout << __func__ << " end" << std::endl;
        return init.result.get();
    }
    void test_async_op() {
        spawn(context_, [this](asio::yield_context yield) {
            using namespace std::chrono;
            yield_ = &yield;
            auto start = system_clock::now();
            boost::system::error_code ec;
            auto s = async_operate(yield[ec]);
            auto delta = system_clock::now() - start;
            std::cout << s << " elapsed " << duration_cast<milliseconds>(delta).count() <<  std::endl;
            work_.reset();
        });
    }
};


int main() {
    AsyncTest at;    
    at.test_async_op();
}

