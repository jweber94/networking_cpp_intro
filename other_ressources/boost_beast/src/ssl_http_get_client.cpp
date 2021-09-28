// FIXME: Try to compile and run it
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>

#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>

#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>

class Session : public std::enable_shared_from_this()
{
    public: 
        Session(boost::asio::io_service ioserv, boost::asio::ssl::context & ssl_context) : resolver_(ioserv), stream_(ioserv, ssl_context)
        {
            std::cout << "[SESSION]: Client session initialized!\n";
        }

        void run(std::string & host, std::string & port, std::string & target, int version){
            // Try to set the SNI hostname - this is a SSL functionallity (from the SSL library - ) and is needed to do a successful handshake for the most of the SSL-server configurations (Ref.: https://stackoverflow.com/questions/5113333/how-to-implement-server-name-indication-sni) 
            if (!SSL_set_tlsext_host_name(stream_.native_handle(), host){
                boost::beast::error_code ec(static_cast<int>(::ERR_get_error()), boost::asio::error::get_ssl_category());
                std::cerr << "[SESSION]: Error on .run()\n";  
                return; 
            }

            // create the request like we have done in the no-SSL example
            request_.version(version); 
            request_.method(boost::beast::http::verb::get);
            request_.target(target);

            request_.set(boost::beast::http::field::host, host);
            request_.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);

            // resolve the host
            resolver_.async_resolve(host, port, std::bind(&Session::on_resolve, shared_from_this())); 
                // the async_resolve function needs the on_resolve completion handler that needs to be implemented according to https://www.boost.org/doc/libs/1_75_0/doc/html/boost_asio/reference/ResolveHandler.html
                // It receives the resolving results as an iterable list from the asynchronous function
        }

        void on_resolve(boost::system::error_code ec, boost::asio::ip::tcp::resolver::results_type results){
            if (!ec){
                boost::beast::get_lowest_layer(stream_).expires_after(std::chrono::seconds(30)); // setting the timeout for the next logical operation 
                boost::beast::get_lowest_layer(stream_).async_connect(results, std::bind(&Session::on_connect, shared_from_this())); 

            } else {
                std::cerr << "[SESSION]: Error on on_resolve()\n"; 
                return; 
            }
        }

        void on_connect(boost::system::error_code ec, boost::asio::ip::tcp::resolver::results_type::endpoint_type endpoint_data_exchange){
            if (!ec){
                // in the case of a SSL connection, we do NOT send the request after the connection is established! INSTEAD we do the SSL/TLS handshake
                stream_.async_handshake(boost::asio::ssl::stream_base::client, std::bind(&Session::on_handshake, shared_from_this())); 
                
            } else {
                std::cerr << "[SESSION]: Error on on_connect()\n"; 
                return; 
            }
        }

        void on_handshake(boost::system::error_code ec){
            if (!ec){
                beast::get_lowest_layer(stream_).expires_after(std::chrono::seconds(30)); 
                boost::beast::http::async_write(stream_, request_, std::bind(&Session::on_write, shared_from_this())); 

            } else {
                std::cerr << "[SESSION]: Error on on_handshake()\n"; 
                return; 
            }
        }

        void on_write(boost::system::error_code ec, std::size_t bytes_transferred){
            if (!ec){
                boost::ignore_unused(bytes_transferred); // To avoid compiler moaning

                boost::beast::http::async_read(stream_, buffer_, result_, std::bind(&Session::on_read, shared_from_this()));  

            } else {
                std::cerr << "[SESSION]: Error on on_write()\n"; 
                return; 
            }
        }

        void on_read(boost::system::error_code ec, std::size_t bytes_transferred){
            boost::ignore_unused(bytes_transferred) // To avoid compiler moaning

            if (!ec){
                // print out the result
                std::cout << "This is the result of the server request:\n\n" << result_ << "\n\n";  
                
                // shutdown the connection after the message from the server was received - and try a soft shutdown for 30 seconds
                boost::beast::get_lowest_layer(stream_).expires_after(sd::chrono::seconds(30));
                stream_.async_shutdown(std::bind(&Session::on_shutdown, shared_from_this()));  

            } else {
                std::cerr << "[SESSION]: Error on on_read()\n"; 
                return; 
            }

        }

        void on_shutdown(boost::system::error_code ec){
            if (ec == boost::asio::error::eof){
                ec = {}; 
            } else {
                return fail(ec, "shutdown"); 
            }
        }

    private: 
        boost::asio::ip::tcp::resolver resolver_; 
        boost::beast::ssl_stream<boost::beast::tcp_stream> stream_; 
        boost::beast::flat_buffer buffer_; 
        boost::beast::http::request<boost::beast::http::empty_body> request_; 
        boost::beast::http::response<boost::beast::http::string_body> response_; 
}; 


int main(int argc, char * argv []){

    // command line parser
    if (argc != 4){
        std::cerr << "Usage: http-client-async-ssl <host> <port> <target> [<HTTP version: 1.0 or 1.1(default)>]\n" <<
                     "Example:\n" <<
                     "    http-client-async-ssl www.example.com 443 /\n" <<
                     "    http-client-async-ssl www.example.com 443 / 1.0\n";

        return 1; 
    }

    std::string host = boost::lexical_cast<std::string>(argv[1]);
    std::string port = boost::lexical_cast<std::string>(argv[2]);
    std::string target = boost::lexical_cast<std::string>(argv[3]);
    int version = 10; 

    // create contexts
    boost::asio::io_service ioserv; 
    ssl::context ctx(ssl::context::tlsv12_client); // holds the TLS (Transport Layer Security) certificate for the TLS Handshake 
    
    // SSL settings
    load_root_certificate(ctx); 
    ctx.set_verify_mode(boost::asio::ssl::verify_peer); 

    // create a Session instance for the data exchange between this client and the server 
    std::make_shared<Session>(boost::asio::make_strand(ioserv, ctx))->run(host, port, target, version); 

    // run the I/O service object for running the asynchonous tasks
    ioserv.run(); 

    std::cout << "Finished program\n";
    return 0; 
}

/*
* Compile Command:
*   $ g++ ssl_http_get_client.cpp -I/usr/local/include -L/usr/local/lib -lboost_coroutine -lboost_context -lboost_system --std=c++17 -pthread -lcrypto -lssl
*/