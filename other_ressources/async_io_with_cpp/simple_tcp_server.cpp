#include <iostream>
#include <boost/asio.hpp>
#include <optional>

class Session : public std::enable_shared_from_this<Session>
{
    public: 
        Session(boost::asio::ip::tcp::socket && socket) : sock_(std::move(socket)) {
            std::cout << "Session initialized\n"; 
        }

        void start(){
            boost::asio::async_read_until(sock_, streambuff_, '\n', [self = shared_from_this()](boost::system::error_code ec, std::size_t length_message_in_bytes){
                    // CAUTION: We need the self = shared_from_this() object in order to not delete the shared pointer from the Server::async_accept() function
                std::cout << std::istream(&self->streambuff_).rdbuf();
                    // boost::asio::streambuf is a buffer object that contains char arrays of one or more characters
                    // We needed to inherit the enable_shared_from_this() in order to access the this pointer from inside the lambda
                    // .rbbuf() returns the reference fom the underlying buffer object of the istream object (which is a wrapper around the underlying buffer object) 
            }); 
            // Read asynchronously until \n is read out from the message and print it out to the std::cout channel
        }

    private: 
        boost::asio::ip::tcp::socket sock_; 
        boost::asio::streambuf streambuff_;
}; 

class Server 
{
    public:
        Server (boost::asio::io_service & io_serv_obj, std::uint16_t port) : ioserv_(io_serv_obj), acceptor_(ioserv_, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)){
            std::cout << "Initializing Server successful.\n"; 
        }

        void async_accept(){
            socket_.emplace(ioserv_);
            acceptor_.async_accept(*socket_, [&](boost::system::error_code ec){
                std::make_shared<Session>(std::move(*socket_))->start(); // Start the session that 
                    // Whenever a new socket is optionally created, the I/O service object is associated with it and then moved (and so the reference to the I/O service object) to the individual session instance
                    // The shared pointer is captured by the lambda function from Session::Start from the lambda function async_read_until. So the implicit (this) shared pointer gets not deleted until the read handler function is not terminated. When this happens, the Session instance gets automatically destroyed since noting points to it (also not the this pointer) and it does not block any new connections, since the Session::start() function does not block the accept handler of the server  
                async_accept(); // recurive call of the acceptance function for new clients that try to connect to the server 
            }); 
        }

    private:
        boost::asio::io_service & ioserv_;
        boost::asio::ip::tcp::acceptor acceptor_; 
        std::optional<boost::asio::ip::tcp::socket> socket_; 
            // std::optional is an object that may or may not be present (Ref: https://en.cppreference.com/w/cpp/utility/optional)
            // since the socket private member is optional, we can create multiple instances at one time, as long as we std::move them away
};


int main(){

    boost::asio::io_service ioserv; 
    Server srv(ioserv, 15007); 
    srv.async_accept(); 
    ioserv.run(); 

    std::cout << "Finished program\n"; 
    return 0; 
}

/*
* Compile command: 
*   $ g++ -pthread --std=c++17 simple_tcp_server.cpp -I/usr/local/include -L/usr/local/lib -lboost_coroutine -lboost_context -lboost_system
*/