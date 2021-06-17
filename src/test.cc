#define BOOST_COROUTINES_NO_DEPRECATION_WARNING 
#include <iostream>
#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/use_future.hpp>
#include <thread>
#include <chrono>
#include <future>

using boost::system::error_code;
namespace asio = boost::asio;

template <typename Token>
auto async_meaning_of_life(bool success, Token&& token)
{
#if BOOST_VERSION >= 106600
    using result_type = typename asio::async_result<std::decay_t<Token>, void(error_code, int)>;
    typename result_type::completion_handler_type handler(std::forward<Token>(token));

    result_type result(handler);
#else
    typename asio::handler_type<Token, void(error_code, int)>::type
                 handler(std::forward<Token>(token));

    asio::async_result<decltype (handler)> result (handler);
#endif

    if (success)
        handler(error_code{}, 42);
    else
        handler(asio::error::operation_aborted, -2);

    return result.get ();
}

void using_yield_ec(asio::yield_context yield) {
    for (bool success : { true, false }) {
        boost::system::error_code ec;
        auto answer = async_meaning_of_life(success, yield[ec]);
        std::cout << __FUNCTION__ << ": Result: " << ec.message() << "\n";
        std::cout << __FUNCTION__ << ": Answer: " << answer << "\n";
    }
}

void using_yield_catch(asio::yield_context yield) {
    for (bool success : { true, false }) 
    try {
        auto answer = async_meaning_of_life(success, yield);
        std::cout << __FUNCTION__ << ": Answer: " << answer << "\n";
    } catch(boost::system::system_error const& e) {
        std::cout << __FUNCTION__ << ": Caught: " << e.code().message() << "\n";
    }
}

void using_future() {
    for (bool success : { true, false }) 
    try {
        auto answer = async_meaning_of_life(success, asio::use_future);
        std::cout << __FUNCTION__ << ": Answer: " << answer.get() << "\n";
    } catch(boost::system::system_error const& e) {
        std::cout << __FUNCTION__ << ": Caught: " << e.code().message() << "\n";
    }
}

void using_handler() {
    for (bool success : { true, false })
        async_meaning_of_life(success, [](error_code ec, int answer) {
            std::cout << "using_handler: Result: " << ec.message() << "\n";
            std::cout << "using_handler: Answer: " << answer << "\n";
        });
}

template<typename CompletionToken>
auto async_operate(CompletionToken token) {
    boost::asio::async_completion<CompletionToken, void(error_code, std::string)> init(token);
    
    std::cout << __func__ << " this_thread id=" << std::this_thread::get_id() << std::endl;
    // asio::async_completion<asio::yield_context, void()> init(token);

    // auto op = asio::bind_executor(ex, [](boost::system::error_code ec, std::string s) {
    //     std::cout << s << std::endl;
    // });
    // op(boost::system::error_code(), "aaps");
    
    // auto handler = init.completion_handler;
    // static std::thread tr1([&handler]() {
    std::thread tr1([handler = init.completion_handler]() {
        std::cout << "thread tr1 id=" << std::this_thread::get_id() << std::endl;
        using namespace std::chrono_literals;
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

void async_op(asio::yield_context yield) {
    using namespace std::chrono;
    auto start = system_clock::now();
    boost::system::error_code ec;
    auto s = async_operate(yield[ec]);
    auto delta = system_clock::now() - start;
    std::cout << s << " elapsed " << duration_cast<milliseconds>(delta).count() <<  std::endl;
}


int main() {
    // asio::io_service svc;

    // spawn(svc, using_yield_ec);
    // spawn(svc, using_yield_catch);
    // std::thread work([] {
    //         using_future();
    //         using_handler();
    //     });

    // svc.run();
    // work.join();
    using namespace std;
    asio::io_context ioc;
    spawn(ioc, async_op);
    using Executor = asio::io_context::executor_type;
    asio::executor_work_guard<Executor> work(asio::make_work_guard(ioc));
    std::thread tr([&ioc]() {
        ioc.run();
    });
    this_thread::sleep_for(5s);
    work.reset();
    tr.join();
}

