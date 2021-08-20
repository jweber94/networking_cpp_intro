#include <boost/asio/io_service.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <string>
#include <ctime>
#include <iostream>

boost::asio::io_service ioserv; 
boost::asio::ip::tcp::endpoint tcp_endpt{boost::asio::ip::tcp::v4(), 2014}; // TCP/IPv4 on port 2014 --> implicitly calls AF_INET and SOCK_STREAM which can be seen in the acutal implementation
    // this creates implicitly an address --> in the C basic tutorial this is the equivalent to the sockaddr_in
boost::asio::ip::tcp::acceptor tcp_acceptor {ioserv, tcp_endpt}; // does the bind of the previously defined socked address and is a wrapper around the accept function of the C tutorial

boost::asio::ip::tcp::socket tcp_socket {ioserv}; // creates implicitly a socket with AF_INET (for IP) and SOCK_STREAM (for TCP)  
    // this is a socket, that is just used for the data exchange 

std::string data; // storage for the data that should be send 


void write_handler(const boost::system::error_code &ec, std::size_t bytes_transferred) {
    // requirements for a write handler: https://www.boost.org/doc/libs/1_75_0/doc/html/boost_asio/reference/WriteHandler.html
    if (!ec){
        std::cout << "Finished processing the request. End the connection to the client\n";
        tcp_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_send); // end the TCP/IP connection for the data exchange with the client
    }

}


void accept_handler(const boost::system::error_code &ec){
    // requirements for a valid accept_handler function: https://www.boost.org/doc/libs/1_75_0/doc/html/boost_asio/reference/AcceptHandler.html
    
    std::cout << "New client request was recognized. Processing the request...\n";
    
    if (!ec){
        std::time_t now = std::time(nullptr); 
        data = std::ctime(&now);
        boost::asio::async_write(tcp_socket, boost::asio::buffer(data), write_handler); // writing to a given socket (must NOT be the calling socket - here we use the global data exchange socket for it)
            // an alternative is the use of the boost::asio::async_write_some() function - then it must be called again in the write_handler function like it was done in the networking_client.cpp with async_read_some(...)
            // async_write needs a write_handler to finish the data exchange properly

            // CAUTION: data needs to be global since the boost::asio::buffer object hands the data over by reference.
            // If data would not be global, it could go out of scope, before the data is completly write out to the socket 
            // since the async_accept method of the tcp::acceptor object is NOT blocking the calling thread
    }

}

int main(){

    tcp_acceptor.listen(); // listen on the tcp acceptor associated socket for new connections
        // listen is a NON-blocking function 

    tcp_acceptor.async_accept(tcp_socket, accept_handler); 
        // a data exchange tcp socket is given to the acceptor object which is used to establish a data exchange TCP connection between the client and the server on a new port (which is not equal to the listening port)
        // this is the equivivalent to the "new_socket" and "address_ex" in the C networking tutorial, just that here we need to hand over an TCP socket object to the acceptor object --> it is just a little more abstracted and in a object oriented manner  
    
        // is a blocking function, so the execution stops here until a new client request is received on the listening port
    

    ioserv.run(); // join all asynchronouos tasks and make the output of their handlers available 

    return 0; 
}

/*
* Compile command: (gcc version 9.3.0 and boost libraries installed on the system) 
*   $ g++ -pthread --std=c++17 networking_time_server.cpp
* 
* Call the server with 
*   $ telnet localhost 2014
*
* or type in your Browser
*   localhost:2014
*/