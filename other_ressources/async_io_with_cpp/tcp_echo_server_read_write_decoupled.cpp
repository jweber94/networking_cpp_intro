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
        Session(tcp::socket && socket) : socket_(std::move(socket)), instreambuffer_(65536){
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
            if (!ec && bytes_transferred){
                // if bytes_transferred == 0 then the instreambuffer_ is completly filled with data
                // We consider an exceeded buffer as a client request violation. This should be documented within the API documentation 
                if (!writing_){
                    // the writing back to the client is scheduled if we are currently not writing
                    write();
                }
                // schedule the next read operation after we write the sended data back to the client
                read(); 
            } else {
                close(); 
                std::cerr << "[SESSION]: An error occured or the incoming buffer instance is completly full. Droped the session.\n";  
            }
        }

        void write(){
            // setting the flag in order to hand back the status of writing to the asynchonous reading completion handler
            writing_ = true; 
            
            // manage the streambuffer for writing out 
            auto tmp_buffer_handle = outstreambuffer_.prepare(instreambuffer_.size());
            io::buffer_copy(tmp_buffer_handle, instreambuffer_.data());
            outstreambuffer_.commit(tmp_buffer_handle.size());  

            // write from the out buffer
            io::async_write(socket_, outstreambuffer_, std::bind(&Session::on_write, shared_from_this(), std::placeholders::_1, std::placeholders::_2)); 
        }

        void on_write(error_code_type ec, std::size_t bytes_transferred){
            writing_ = false;

            // We do not schedule a new write operation inside the on_write write-completion handler, so we are not shure that all received data is written back to the client, see the in-depth explaination from https://dens.website/tutorials/cpp-asio/tcp-echo-server-2
            if (ec){
                std::cerr << "[SESSION]: An error occured during writeing. Dropped the session!\n"; 
                close(); // ping pong between read and write, no queue for read and write necessary
            } 

        void close(){
            error_code_type ec; 
            socket_.close(ec); 
        }

        tcp::socket socket_; 
        // separate the buffers for reading and writing in order to decouple the read and write operation 
        io::streambuf instreambuffer_;
        io::streambuf outstreambuffer_; // The limit of bytes of the outstreambuffer_ is determined by the limit of the instreambuffer_ that is set within the initializer list of the Session constructor
        bool writing_ = false; // flag for checking if there is currently a write process
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