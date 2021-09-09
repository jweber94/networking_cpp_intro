#include <iostream>
#include <boost/asio.hpp>

namespace io = boost::asio; 
namespace ip = io::ip; 

using tcp = ip::tcp; 
using error_code_type = boost::system::error_code; 

// forward declaration
void send(tcp::socket * sock, io::const_buffer data_to_send);

void on_send(error_code_type ec, std::size_t bytes_transferred, io::const_buffer data_to_send, tcp::socket * sock){
    // Must be declared before send in order to fulfill forward declaration
    if(!ec){
        auto rest = data_to_send + bytes_transferred;
        if( rest.size() != 0 ){
            // auto rest = data + bytes_transferred
            // --> You can add an integer to a buffer view instance and receive a pointer to the underlaying data which is shifted by the integer value
            // This formulation is equivivalent to 
            /*
                auto rest = io::buffer(
                    static_cast<std::uint8_t const*>(data.data()) + bytes_transferred,
                    data.size() - bytes_transferred
                );
            */
            send(sock, rest); 
        } else {
            std::cout << "Message delivered\n";  
        }
    } else {
        std::cerr << "ERROR: " << ec.message() << "\n"; 
    }
}

void send(tcp::socket * sock, io::const_buffer data_to_send){
    sock->async_send(data_to_send, std::bind(on_send, std::placeholders::_1, std::placeholders::_2, data_to_send, sock));
}


int main(){

    io::io_service ioserv; 
    tcp::socket socket_for_dataexchange(ioserv);
    tcp::acceptor acceptor_instance(ioserv, tcp::endpoint(tcp::v4(), 52222)); 

    acceptor_instance.async_accept(socket_for_dataexchange, [&](error_code_type ec){
        if (!ec){
            std::cout << "Sending data to the client\n";
            send(&socket_for_dataexchange, io::buffer("Hello, this is a test.")); 
            std::cout << "Finished sending.\n";
            socket_for_dataexchange.close(); 
        } else {
            std::cerr << "ERROR: Error while trying to send data to the client.\n"; 
        }
         
    }); 

    ioserv.run(); 

    std::cout << "Finished server program.\n";
    return 0; 
}

/*
* Compile Command:
*   $ g++ send_recursive_not_recommend.cpp -pthread --std=c++17 -I/usr/local/include -L/usr/local/lib -lboost_coroutine -lboost_context -lboost_system
*/