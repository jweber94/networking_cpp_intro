#include <iostream>
#include <boost/asio.hpp>
#include <string>
#include <array>

namespace io = boost::asio; 
namespace ip = io::ip; 

using tcp = ip::tcp; 
using error_code_type = boost::system::error_code; 

template <typename... Args>
auto const_buffer_sequence(Args&&... args)
{
    return std::array<io::const_buffer, sizeof...(Args)>{io::buffer(args)...};
}

template <typename... Args>
auto buffer_sequence(Args&&... args)
{
    return std::array<io::mutable_buffer, sizeof...(Args)>{io::buffer(args)...};
}


int main() {
    std::cout << "Starting the server.\n"; 

    io::io_service ioserv; 
    tcp::endpoint connection_listener(tcp::v4(), 51111); // port 80 is the http port
    tcp::acceptor connection_acceptor(ioserv, connection_listener); 
    tcp::socket socket_for_exchange(ioserv); 

    std::string http_header =               "HTTP/1.1 200 OK\n"
                                            "Date: Mon, 06 Sept 2021 08:59:53 GMT\n"
                                            "Server: Ubuntu 16.04 LTS\n"
                                            "Last-Modified: Mon, 06 Sept 2021 08:57:56 GMT\n"
                                            "Content-Length: 88\n"
                                            "Content-Type: text/html\n"
                                            "Connection: Closed\n"
                                            "\n";
            
    std::string payload =                   "<html>\n"
                                            "<body>\n";

    std::string payload2 =                  "<h1>hello, world!</h1>\n"
                                            "</body>\n"
                                            "</html>\n\n"; 

    auto header_buffer_view = io::const_buffer(http_header.data(), http_header.size());
    auto payload_buffer_view = io::const_buffer(payload.data(), payload.size()); 
    auto payload_buffer_view_2 = io::const_buffer(payload2.data(), payload2.size());

    std::cout << "The size of the payload buffer is: " << payload_buffer_view.size() << "\nAnd the pointer to the data is: " << payload_buffer_view.data() << "\n"; 

    auto buffer_sequence_view = const_buffer_sequence(header_buffer_view, payload_buffer_view, payload_buffer_view_2); 

    connection_acceptor.async_accept(socket_for_exchange, [&](boost::system::error_code ec){
        if(!ec){
            std::cout << "Connection successful. Sending the HTTP response.\n"; 
            

            io::async_write(socket_for_exchange, buffer_sequence_view, [&](error_code_type ec, std::size_t bytes_transferred){
                if (!ec){
                    std::cout << "Sending the Web page.\n";
                    socket_for_exchange.close();  
                } else {
                    std::cerr << "[SERVER]: An error occured by sending the http response.\n"; 
                }
            }); 

        } else {
            std::cerr << "[SERVER]: Error on connection.\n"; 
        }
    }); 

    ioserv.run();
    std::cout << "Finished program.\n"; 
    return 0; 
}

/*
* Compile Command: 
*   $ g++ buffer_with_variadic_templates.cpp -pthread --std=c++17 -I/usr/local/include -L/usr/local/lib -lboost_coroutine -lboost_context -lboost_system
*
* Get the website with your browser by typing 
*   127.0.0.1:51111
* into the URL line. 
*/