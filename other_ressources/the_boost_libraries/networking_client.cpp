#include <boost/asio/io_service.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <array>
#include <string>
#include <iostream>

#include <thread>
#include <chrono>

// global variables, since they are needed even outside of the main function (= entrance point)
boost::asio::io_service ioserv; 
boost::asio::ip::tcp::resolver resolv{ioserv}; // DNS resolver I/O object that makes calls to the system DNS server 
boost::asio::ip::tcp::socket tcp_socket{ioserv}; // create a TCP socket --> implicitly creating a socket with AF_INET and SOCK_STREAM (ref: c_network_programming)

std::array<char, 4096> bytes; 

void read_handler(const boost::system::error_code &ec, std::size_t bytes_transferred){
    // read handler requirements: https://www.boost.org/doc/libs/1_75_0/doc/html/boost_asio/reference/ReadHandler.html
    // continuing the socket read out that was started from the connect_handler
    // bytes_transferred shows how many bytes already were received
    if (!ec){
        std::cout.write(bytes.data(), bytes_transferred);
        tcp_socket.async_read_some(boost::asio::buffer(bytes), read_handler); // recursive call of the read handler, so that the complete response from the server is read and printed out
            // this read out must come after the cout operation in order to recursivly print everything that is received out in the correct order
    }
    // the error code is set to != 0 from tcp_socket.async_read_some(...) just if the connection to the server is broken up (e.g. the data was completly send) 

}

void connect_handler(const boost::system::error_code &ec){
    // connect handler requirements: https://www.boost.org/doc/libs/1_75_0/doc/html/boost_asio/reference/ConnectHandler.html
    if (!ec){
        std::string r = "GET / HTTP/1.1\r\nHost: theboostcpplibraries.com\r\n\r\n";
        // write the GET request to the socket in order to make a server request from this client program
        write(tcp_socket, boost::asio::buffer(r)); // is a standart C function (ref: c_network_programming) 
        // receive the answer from the server
        tcp_socket.async_read_some(boost::asio::buffer(bytes), read_handler); // starting the socket read out
    }

}

void resolve_handler(const boost::system::error_code &ec, boost::asio::ip::tcp::resolver::iterator it){
    // neccessary requirements for a valid RESOLVE_HANDLER: https://www.boost.org/doc/libs/1_75_0/doc/html/boost_asio/reference/ResolveHandler.html 

    if (!ec){ // if the DNS resolving was successful, the boost::system::error_code is set to 0, so we need to inverse the errorcode to connect 
        tcp_socket.async_connect(*it, connect_handler); 
    } 
}

int main(){

    boost::asio::ip::tcp::resolver::query q {"theboostcpplibraries.com", "80"}; // give it the host as a string that needs to be resolved to an IP address and then the port that the server is listening on for new connections
        // URL: https://de.wikipedia.org/wiki/Uniform_Resource_Locator
    resolv.async_resolve(q, resolve_handler); // give the resolver the query and the resolve handler (aka function pointer that is called after the asynchronous operation) 
        // The handler functions call different asynchronous methods nested in each other. This is needed, since they are logically dependend on each other. 
        // e.g. to open the socket, a successful resolved ressource address is needed and without a successful created socket, no data could be received from the socket
        // In order to understand the code properly, we need to read the code from the main --> resolve_handler --> connect_handler --> read_handler
    
    // the following three lines of code are used to make the intention of the .run(); call of the I/O service object clear. All handles get called only if the .run() method is called in the calling 
    // thread that started the asynchronous operations. This is in contrast to the use of coroutines (ref: time_server_with_coroutines.cpp), where the handles get executed every time an asynchronous task is finished
    std::cout << "Asynchronous operation started already\n"; 
    std::this_thread::sleep_for(std::chrono::seconds(5)); 
    std::cout << "The io_service object .run() method is called now\n";

    ioserv.run(); 

    return 0;
}

/*
* Compile command:
*   $ g++ -pthread --std=c++17 networking_client.cpp
*/