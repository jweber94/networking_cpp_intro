#include <iostream>
#include <boost/asio.hpp>

namespace io = boost::asio; 
namespace ip = io::ip; 
using tcp = ip::tcp; 
using error_code_type = boost::system::error_code; 

int main(){

    io::io_service ioserv; 
    error_code_type ec; 

    tcp::resolver resolver_instance(ioserv); 

    resolver_instance.async_resolve("google.com", "80", [&](error_code_type ec, tcp::resolver::results_type results){
        if (!ec){
            std::size_t counter = 1; 
            for (tcp::endpoint const & endpt : results){
                std::cout << "The " << counter << " address is: " << endpt << "\n"; 
                counter++;  
            }
        } else {
            std::cerr << "[RESOLVER]: Could not resolve the given address.\n";
        }
    }); 

    ioserv.run(); // running the asynchronous tasks and block the main thread until the I/O service queue of the I/O service object is empty

    std::cout << "Finished program!\n";
    return 0; 
}

/*
* Compile Command
*   $ g++ resolver_intro_async.cpp -pthread --std=c++17 -I/usr/local/include -L/usr/local/lib -lboost_coroutine -lboost_context -lboost_system
*/