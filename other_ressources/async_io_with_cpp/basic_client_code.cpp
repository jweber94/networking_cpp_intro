#include <iostream>
#include <boost/asio.hpp>

namespace io = boost::asio; 
namespace ip = io::ip; 
using tcp = ip::tcp; 
using error_code = boost::system::error_code; 


int main(){

    io::io_service ioserv; 
    tcp::socket socket_to_connect(ioserv);

    ip::address addr = ip::make_address("127.0.0.1"); 
    tcp::endpoint endpt(addr, 80); 

    error_code ec;

    socket_to_connect.connect(endpt, ec);

    if (!ec){
        std::cout << "[CLIENT]: Connection established!\n";
    } else {
        std::cerr << "[CLIENT]: Cound not connect to the server!\n"; 
    } 

    return 0; 
}

/*
* Compile command: 
*   $ g++ base_client_code.cpp -pthread --std=c++17 -I/usr/local/include -L/usr/local/lib -lboost_coroutine -lboost_context -lboost_system
*/