#include <iostream>
#include <boost/asio.hpp>
#include <chrono>

namespace io = boost::asio; 
using error_code = boost::system::error_code; 

// global variables
io::io_service ioserv; 
io::high_resolution_timer glob_timer_instance(ioserv);

auto now(){
    return std::chrono::high_resolution_clock::now(); 
}

auto begin = now(); 

void my_async_wait(){
    
    glob_timer_instance.expires_after(std::chrono::seconds(1)); 
    
    glob_timer_instance.async_wait([&](error_code ec){
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now() - begin).count();
        std::cout << "Time elapsed since the timer was set: " << elapsed << "\n";
        my_async_wait(); // setting up a new waiting cycle 
    }); 
}

int main(){

    my_async_wait(); 
    ioserv.run(); 

    return 0; 
}

/*
* Compile Command: 
*   $ g++ async_timer_example.cpp -pthread --std=c++17 -I/usr/local/include -L/usr/local/lib -lboost_coroutine -lboost_context -lboost_system 
*/