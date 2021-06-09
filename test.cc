#define BOOST_COROUTINES_NO_DEPRECATION_WARNING 
#include <iostream>
#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/use_future.hpp>

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
        handler(asio::error::operation_aborted, 0);

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

int main() {
    asio::io_service svc;

    spawn(svc, using_yield_ec);
    spawn(svc, using_yield_catch);
    std::thread work([] {
            using_future();
            using_handler();
        });

    svc.run();
    work.join();
}

