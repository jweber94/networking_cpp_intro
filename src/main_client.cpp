#include <iostream>
#include <string>
#include <thread>
#include <chrono>

#include <boost/asio/io_service.hpp>
#include <boost/asio/ts/buffer.hpp>
#include <boost/asio/ts/internet.hpp>

int main(){
    /*
    * Synchronous http request to a server using boost::asio
    */

    boost::asio::io_service ioserv; // I/O servive object to manage the I/O services that correspond to the I/O objects
    boost::system::error_code ec; 
    
    boost::asio::ip::tcp::endpoint tcp_endpt(boost::asio::ip::make_address("93.184.216.34", ec), 80);  // create a socket on port 80
    //boost::asio::ip::tcp::endpoint tcp_endpt(boost::asio::ip::make_address("127.0.0.1", ec), 80);  // does not work, since the localhost has no server on port 80
        // create an endpoint based on an IP address on port 80 (http port)
        // basically an endpoint is an address that can access multiple different ressources. It is used to connect to a server. --> Difference between "endpoint" and ressource: https://dev.to/asrar7787/rest-api-what-is-the-difference-between-endpoint-and-resource-3l1p
        // an endpoint can be described throu multiple kinds of addresses. IPv4 is just one of them
        // you can give boost::asio::ip::make_address only an IP address! If you want to hand it a domain, you need a boost::asio::ip::tcp::resolver object (ref: documentation)

    boost::asio::ip::tcp::socket sock(ioserv); // create a socket for the data exchange 
    
    // connect to the endpoint
    sock.connect(tcp_endpt, ec); 
    if (!ec){ // check the error code, if the connection to the endpoint was successful
        std::cout << "Connected - continue programm\n"; 
    } else {
        std::cerr << "An error occured: " << ec.message() << "\n";
        sock.close(); 
        return 1; 
    }

    // sending a http request to the server
    std::string http_request = "GET /index.html HTTP/1.1\r\n"
                                "Host: example.com\r\n"
                                "Connection: close\r\n\r\n";
        // \r is a repositioning to the beginning of the current screen line
    if (sock.is_open()){   
        // check if the socket could connect to the defined endpoint --> we have an active and alive connection to the other end(-point) of the server
        sock.write_some(boost::asio::buffer(http_request.data(), http_request.size()), ec); // send as much data of the given data as possible
            // We need to use asio buffers in order to read and write with sockets. This is an interface for the asio framework
    }  

    // need to wait for the bytes to arrive 
    std::this_thread::sleep_for(std::chrono::milliseconds(200)); // extremly worse practice
    //sock.wait(sock.wait_read); // wait until there is data arrived to read from the socket
        // CAUTION: This waits only until the first packet of data is arrived and then triggers the readout. This results in an incomplete transmission of the requested data in this example

    // receiving data from the server
    size_t bytes_server_response = sock.available(); 
    std::vector<char> response_buffer(bytes_server_response); // create a datastructure for saving the received data from the server

    std::cout << "We have " << bytes_server_response << " available from the server response!\n";
    if (bytes_server_response > 0){
        sock.read_some(boost::asio::buffer(response_buffer.data(), response_buffer.size()), ec); // read as much of the data from the socket as possible
    } else {
        std::cerr << "There was no data from the server to be received!\n";
        sock.close(); 
        return 1; 
    }
    sock.close(); 

    // print out the received data
    for (auto it : response_buffer){
        std::cout << it; 
    }

    return 0; 
}