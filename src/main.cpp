#include <asiolib/all.hpp>
#include <chrono>
#include <iostream>

int main()
{
    
    asio::io_context ioContext;
    asio::steady_timer timer(ioContext);
    timer.expires_after(std::chrono::seconds(3));
    
    std::cout << "START\n";
    
    timer.wait();
    
    std::cout << "END\n";
    
}
