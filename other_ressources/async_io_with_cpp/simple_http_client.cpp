#include <iostream>
#include <boost/asio.hpp>
#include <string>

namespace io = boost::asio; 
namespace ip = io::ip; 

using tcp = ip::tcp; 
using error_code_type = boost::system::error_code; 

using namespace std::placeholders;

class ClientApplication
{
    public: 
        ClientApplication(io::io_service & ioserv, std::string hostname) : resolver_(ioserv), socket_(ioserv){
            // Create the HTTP request in a plain string form by creating a HTTP formated string
            request_ =  "GET / HTTP/1.1\n"
                        "Host: " + hostname + "\n"
                        "Connection: close\n\n"; 
            
            // resolve the given hostname with the http service with the resolver instance
            resolver_.async_resolve(hostname, "http", std::bind(&ClientApplication::on_resolve, this, _1, _2)); 
        } 

        void on_resolve(error_code_type ec, tcp::resolver::results_type resolve_results){
            std::cout << "Resolve: " << ec.message() << "\n";
            io::async_connect(socket_, resolve_results, std::bind(&ClientApplication::on_connect, this, _1, _2)); 
        }

        void on_connect(error_code_type ec, tcp::endpoint const & endpt){
            std::cout << "Connect: " << ec.message() << ", endpoint: " << endpt << "\n"; 
            io::async_write(socket_, io::buffer(request_), std::bind(&ClientApplication::on_write, this, _1, _2)); 
        }

        void on_write(error_code_type ec, std::size_t bytes_transferred){
            std::cout << "Write: " << ec.message() << ", bytes transferred: " << bytes_transferred << "\n"; 
            io::async_read(socket_, response_buffer_, std::bind(&ClientApplication::on_read, this, _1, _2)); 
        }

        void on_read(error_code_type ec, std::size_t bytes_received){
            std::cout << "Read: " << ec.message() << ", bytes received: " << bytes_received << "\n";
            
            // print the response to the terminal
            std::cout << std::istream(&response_buffer_).rdbuf() << "\n"; 
        }

    private:
        tcp::resolver resolver_; 
        tcp::socket socket_; 
        std::string request_;
        io::streambuf response_buffer_;  
}; 


int main(){
    io::io_service ioserv; 
    std::string input_domain; 

    std::cout << "Please insert the domain that you want the GET request: "; 
    std::cin >> input_domain; 
    ClientApplication app(ioserv, input_domain); 

    ioserv.run();

    std::cout << "\n\nFinished program.\n"; 
    return 0; 
}

/*
* Compile Command: 
*   $ g++ simple_http_client.cpp -pthread --std=c++17 -I/usr/local/include -L/usr/local/lib -lboost_coroutine -lboost_context -lboost_system
*/