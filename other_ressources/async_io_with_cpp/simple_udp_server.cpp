#include <boost/asio.hpp>

int main(){
    /*
    * This is the most simple server code for an UDP echo server that works synchronously
    */

    std::uint16_t port = 15002; 

    boost::asio::io_service io_serv; // asio service object for managing the I/O services and their corresponding I/O objects 

    boost::asio::ip::udp::endpoint receiver_socket(boost::asio::ip::udp::v4(), port); // define the address of the socket
        // boost::asio::ip::udp::v4() defines an object that can be used as an interface for a UDP network
    boost::asio::ip::udp::socket sock(io_serv, receiver_socket); // bind the address to the socket (sockets are just like a cable where you can listen for data. Another analogy is a file handle that is designed for the network communication)
        // Ref: https://stackoverflow.com/questions/47458487/difference-between-endpoint-socket-acceptor


    for ( ; ; ){ // infinite for loop
        // Create large enough buffer array
        char buffer_array[65536];

        boost::asio::ip::udp::endpoint sender_socket; 
        
        // receive data from the client
        std::size_t num_bytes_transferred = sock.receive_from(boost::asio::buffer(buffer_array), sender_socket);
            // CAUTION: asio buffers hold pointers/references to the actual memory where the data should be stored. It does NOT own the memory. (We can not hand it over an unique_ptr!)

        // send the received data from the client back to the client that sended us the data
        sock.send_to(boost::asio::buffer(buffer_array, num_bytes_transferred), sender_socket);
            // CAUTION: UDP has a notion where the data packets should be send to  
    } 

    return 0; 
}

/*
* Compile command:
*   $ g++ -pthread --std=c++17 simple_udp_server.cpp -I/usr/local/include -L/usr/local/lib -lboost_coroutine -lboost_context -lboost_system
*/