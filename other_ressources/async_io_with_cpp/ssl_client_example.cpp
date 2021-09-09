#include <iostream>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <string>

namespace io = boost::asio; 
namespace ip = io::ip; 

using tcp = ip::tcp; 
using error_code_type = boost::system::error_code; 

// SSL specific
namespace ssl = io::ssl; 
using ssl_socket = ssl::stream<tcp::socket>; 


int main(){

    io::io_service ioserv;
    ssl::context ssl_context(ssl::context::tls); 

    ssl_socket sock(ioserv, ssl_context); 

    tcp::resolver resolver(ioserv);  

    auto endpt_array = resolver.resolve("google.com", "443");

    io::connect(sock.next_layer(), endpt_array); // synchronous connection
        // accessing the raw tcp socket instead of the ssl socket for establishing the connection, since the handshake has not been done yet
    
    sock.handshake(ssl::stream_base::client);

    std::string request =   "GET / HTTP/1.1\n"
                            "Host: www.google.com\n"
                            "Connection: close\n\n";

    io::write(sock, io::buffer(request)); // synchronous read 

    io::streambuf response_buffer; 
    error_code_type ec; 

    io::read(sock, response_buffer, ec);

    std::cout << "This is the response from the server:\n\n"; 
    std::cout << std::istream(&response_buffer).rdbuf() << "\n";   

    std::cout << "\n\nFinished program.\n"; 
    return 0; 
}

/*
* Compile Command: 
*   $ g++ ssl_client_example.cpp -pthread --std=c++17 -I/usr/local/include -L/usr/local/lib -lboost_coroutine -lboost_context -lboost_system -lcrypto -lssl
*
* You need to install 
*   $ sudo apt-get install libssl-dev
*   Ref.: https://askubuntu.com/questions/211038/cant-find-openssl
* which is not installed nativly on ubuntu
*
* If you are using CMake, you need to add openssl to the required packages and have to link against them!
*/