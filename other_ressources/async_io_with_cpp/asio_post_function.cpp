#include <iostream>
#include <boost/asio.hpp>
#include <thread>
#include <chrono>

namespace io = boost::asio; 
using tcp = io::ip::tcp; 
using error_code = boost::system::error_code; 
using work_guard_type = io::executor_work_guard<io::io_context::executor_type>; 


void my_info_function(const std::string & name){
    std::cout << "Function owned by " << name << " was executed asynchronously!\n";
}

int main(){

    io::io_context ioserv;
    work_guard_type work_guard(ioserv.get_executor()); 

    std::thread posting_thread([&](){
        auto my_functor = std::bind(my_info_function, "jens"); // creating a function without input parameters and with void as return type  
        io::post(ioserv, my_functor); 
    }); 

    std::thread watchdog_timeout([&](){
        std::this_thread::sleep_for(std::chrono::seconds(5)); 
        work_guard.reset(); 
    }); 
    ioserv.run(); 
    std::cout << "[TIMEOUT]: No work to do for more then 5 seconds. Exiting the program.\n"; 

    watchdog_timeout.join(); // join the timeout thread in order to avoid a segmentation fault
    posting_thread.join(); 
    return 0; 
}

/*
* Compile command:
*   $ g++ -pthread --std=c++17 asio_post_function.cpp -I/usr/local/include -L/usr/local/lib -lboost_coroutine -lboost_context -lboost_system
*/