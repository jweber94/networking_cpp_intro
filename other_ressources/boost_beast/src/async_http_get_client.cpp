#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

#include <boost/asio.hpp>

#include <boost/lexical_cast.hpp>

#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>

// utility functions
void fail(boost::system::error_code ec, char const * what){
    std::cerr << "ERROR: " << what << " : " << ec.message() << "\n"; 
}

class Session : public std::enable_shared_from_this<Session>
{
    public: 
        Session(boost::asio::io_service & ioserv) : resolver_(boost::asio::make_strand(ioserv)), stream_(boost::asio::make_strand(ioserv)){
            std::cout << "[SESSION]: Session initialized.\n"; 
        }

        void run (std::string & host, std::string & port, std::string & target, int version){
            // setup the request
            request_.version(version);
            request_.method(boost::beast::http::verb::get);
            request_.target(target);
            
            request_.set(boost::beast::http::field::host, host);
            request_.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);  
        
            // resolve the server
            resolver_.async_resolve(host, port, std::bind(&Session::on_resolve, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
                // calls the on_resolve completion handler after successfully resolving the server connection via OS-DNS lookup
                // on_resolve() calls implicitly the on_connect and on_read as well as on_write methods
        }

        void on_resolve(boost::system::error_code ec, boost::asio::ip::tcp::resolver::results_type results){
            // the async_resolve method hands over the results_type to the completion handler
            if (!ec){
                stream_.expires_after(std::chrono::seconds(30)); // set the time after the NEXT async operation expires with an timeout error
                    // https://www.boost.org/doc/libs/develop/libs/beast/doc/html/beast/ref/boost__beast__basic_stream/expires_after.html
                stream_.async_connect(results, std::bind(&Session::on_connect, shared_from_this(), std::placeholders::_1, std::placeholders::_2)); 

            } else {
                std::cerr << "[SESSION]: Error during resolving the given host.\n"; 
            }
        }

        void on_connect(boost::system::error_code ec, boost::asio::ip::tcp::resolver::results_type::endpoint_type endpt){
            if (!ec){
                stream_.expires_after(std::chrono::seconds(30)); 
                boost::beast::http::async_write(stream_, request_, std::bind(&Session::on_write, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
                    // write the member variable request_ to the beast::tcp_stream member stream_ and call the on_write completion handler method 
            } else {
                std::cerr << "[SESSION]: Error during connection\n"; 
            }
        }

        void on_write(boost::system::error_code ec, std::size_t bytes_transferred){
            boost::ignore_unused(bytes_transferred);
            if (!ec){
                boost::beast::http::async_read(stream_, receive_buffer_, response_, std::bind(&Session::on_read, shared_from_this(), std::placeholders::_1, std::placeholders::_2)); 
            } else {
                std::cerr << "[SESSION]: Error during writeing\n"; 
            }
        }

        void on_read (boost::system::error_code ec, std::size_t bytes_transferred){
            boost::ignore_unused(bytes_transferred); 

            if (!ec){
                std::cout << "This is the message from the server: \n\n" << response_ << "\n";
                stream_.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
                    // get the socket from the beast::tcp stream and then use the common boost::asio syntax
                    // shut down on a synchronous manner
                std::cout << "[SESSION]: Successfully closed the socket\n";
            } else {
                std::cerr << "[SESSION]: Error during reading out\n"; 
            }
        }

    private: 
        boost::asio::ip::tcp::resolver resolver_; 
        boost::beast::tcp_stream stream_; 
        boost::beast::flat_buffer receive_buffer_; 
        boost::beast::http::request<boost::beast::http::empty_body> request_;
        boost::beast::http::response<boost::beast::http::string_body> response_;
}; 


int main (int argc, char * argv []){

    if (argc != 4){
        std::cerr << "Usage: http-client-async <host> <port> <target>\n" <<
                     "Example:\n" <<
                     "    http-client-async www.example.com 80 /\n";

        return EXIT_FAILURE; 
    }

    std::string host = boost::lexical_cast<std::string>(argv[1]);
    std::string port = boost::lexical_cast<std::string>(argv[2]);
    std::string target = boost::lexical_cast<std::string>(argv[3]); 

    int version = 10; // if we do 11, we would request the HTTP Version 1.1, if we stay at 10, we request the usage of HTTP Version 1.0

    boost::asio::io_service ioserv; 

    std::make_shared<Session>(ioserv)->run(host, port, target, version); // call it as an rvalue - since the Session class points to itself until the connection is expired, there will be a shared pointer to the memory on the heap and the Session instance wont get deleted until the data exchange socket is closed

    // run the I/O service object in order to execute the asynchronous tasks
    ioserv.run(); 

    std::cout << "Finished program\n"; 
    return EXIT_SUCCESS; 
}

/*
* Compile Command: 
*   $ g++ async_http_get_client.cpp -I/usr/local/include -L/usr/local/lib -lboost_coroutine -lboost_context -lboost_system --std=c++17 -pthread
*
* Call:
*   $ ./a.out google.de 80 "/"
*/