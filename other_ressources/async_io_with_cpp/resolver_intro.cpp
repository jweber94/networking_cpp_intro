#include <iostream>
#include <boost/asio.hpp>

namespace io = boost::asio; 
namespace ip = io::ip; 

using tcp = ip::tcp; 
using error_code = boost::system::error_code; 

int main(){

    io::io_service ioserv; 
    error_code ec; 
    
    tcp::resolver resolver_instance(ioserv); 

    tcp::resolver::results_type resolve_results = resolver_instance.resolve("google.com", "80", ec);

    std::size_t counter = 1;
    for (tcp::endpoint const & endpt : resolve_results){
        std::cout << "The " << counter << " address is: " << endpt << "\n"; 
        counter++; 
    }

    std::cout << "Finished program.\n"; 

    return 0; 
}

/*
* Compile Command:
*   $ g++ resolver_intro.cpp -pthread --std=c++17 -I/usr/local/include -L/usr/local/lib -lboost_coroutine -lboost_context -lboost_system
*/