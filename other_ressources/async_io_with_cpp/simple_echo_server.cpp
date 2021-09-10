#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <functional>
#include <optional>


namespace io = boost::asio; 
namespace ip = io::ip; 

using tcp = ip::tcp; 
using error_code_type = boost::system::error_code; 

class Session : public std::enable_shared_from_this<Session>
{   
    public:
        Session(tcp::socket && socket) : socket_(std::move(socket)){
            std::cout << "[SERVER]: Session initialized.\n"; 
        }

        void start(){
            read();
        }
    private: 

        void read(){
            io::async_read(socket_, streambuffer_, io::transfer_at_least(1), std::bind(&Session::on_read, shared_from_this(), std::placeholders::_1, std::placeholders::_2)); 
        }

        void on_read(error_code_type ec, std::size_t bytes_transferred){
            // boost asio read handler
            if (!ec){
                write();
            } else {
                std::cerr << "[SESSION]: An error occured during data receiving.\n"; 
            }
        }

        void write(){
            io::async_write(socket_, streambuffer_, std::bind(&Session::on_write, shared_from_this(), std::placeholders::_1, std::placeholders::_2)); 
        }

        void on_write(error_code_type ec, std::size_t bytes_transferred){
            if (!ec){
                read(); // ping pong between read and write, no queue for read and write necessary
            } else {
                std::cerr << "[SESSION]: An error occured during writing\n"; 
            }
        }

        tcp::socket socket_; 
        io::streambuf streambuffer_; 

}; 


class Server 
{
    public: 
        Server(io::io_service & ioserv, std::uint16_t port) : ioserv_(ioserv), acceptor_(ioserv, tcp::endpoint(tcp::v4(), port)){
            accept(); 
        }

    private: 
        // member methods
        void accept(){
            socket_.emplace(ioserv_); 

            acceptor.async_accept(*socket_, [&](error_code_type ec){
                std::make_shared<Session>(std::move(*socket))->start(); 
            }); 

        }

        // member variables
        io::io_service & ioserv_;
        tcp::acceptor acceptor_;
        std::optional<tcp::socket> socket_;  
};


int main(int argc, char * argv[]){

    if (argc != 2){
        // If you do not hand over any command line arguments, we want to say how to use the server properly and then end the program
        std::cout << "Usage: Server [PORT]\n"; 
        return 0; 
    } else {
        io::io_service ioserv; 

        Server srv(ioserv, boost::lexical_cast<std::uint16_t>(argv[1]));
            // The argv[0] argument is the name of the program and the following arguments are the given command line parameters that are space-separated
            //std::cout << "This is argv[0]: " << argv[0] << std::endl;  
            //std::cout << "This is argv[1]: " << argv[1] << std::endl;
    }

    return 0; 
}


/*
* Compile Command:
*   $ g++ simple_echo_server.cpp I/usr/local/include -L/usr/local/lib -lboost_coroutine -lboost_context -lboost_system
*/