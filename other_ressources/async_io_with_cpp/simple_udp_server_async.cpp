#include <iostream>
#include <boost/asio.hpp>

int main(){

    uint16_t listening_port = 15006; 

    boost::asio::io_service ioserv; 
    boost::asio::ip::udp::endpoint listening_endpt(boost::asio::ip::udp::v4(), listening_port);
    boost::asio::ip::udp::socket listening_sock(ioserv, listening_endpt); 

    char buffer_array [65504]; 

    boost::asio::ip::udp::endpoint sender_address;

    listening_sock.async_receive_from(boost::asio::buffer(buffer_array, sizeof(buffer_array)), sender_address, [&](boost::system::error_code ec, std::size_t bytes_transferred){
        std::cout << "Message is received, message size is: " << bytes_transferred << "\n"; 
    }); 

    ioserv.run(); // Synchronize and run the asynchronous I/O services in the thread where the I/O service object is set to .run(); If we do not do this, the asynchronous operation will not be executed

    std::string received_string(buffer_array); 
    std::cout << "This is the received data:\n" << received_string << "\n\n";

    std::cout << "Finished program!\n"; 

    return 0; 
}

/*
* Compile command
*   $ g++ -pthread --std=c++17 simple_udp_server_async.cpp -I/usr/local/include -L/usr/local/lib -lboost_coroutine -lboost_context -lboost_system
* 
* Sending UDP packets to the server:
*   $ echo "This is my data" > /dev/udp/127.0.0.1/15006
*/