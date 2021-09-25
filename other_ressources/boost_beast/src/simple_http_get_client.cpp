#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

#include <boost/asio.hpp>
//#include <boost/asio/connect.hpp>
//#include <boost/asio/ip/tcp.hpp>

#include <boost/lexical_cast.hpp>

#include <cstdlib>
#include <iostream>
#include <string>

//namespace beast = boost::beast; 
//namespace http = beast::http; 

namespace io = boost::asio; 
namespace ip = io::ip; 

using tcp = ip::tcp;  

int main(int argc, char * argv []){
    
    try {
        // check if the client was validly called from the command line
        if (argc != 4){
            std::cerr <<
                "Usage: http-client-sync <host> <port> <target>\n" <<
                "Example:\n" <<
                "    http-client-sync www.example.com 80 /\n" <<
                "    http-client-sync www.example.com 80 / 1.0\n";
            return EXIT_FAILURE;
        }

        std::string host = boost::lexical_cast<std::string>(argv[1]);
        std::string port = boost::lexical_cast<std::string>(argv[2]); 
        std::string query = boost::lexical_cast<std::string>(argv[3]);
        int version = 10; // equals HTTP version 1.0, if we do 11 then it would be HTTP version 1.1
        
        // boost::asio stuff
        io::io_service ioserv; // create an I/O service object from boost asio in order to call the I/O service objects 
        boost::system::error_code ec; 

        tcp::resolver resolver(ioserv);
        auto const domain_results = resolver.resolve(host, port, ec);

        // boost::beast connection instance
        boost::beast::tcp_stream stream(ioserv); // is a wrapper class like the Session class that we have created within the async_io tutorial of dens website 
            // https://www.boost.org/doc/libs/develop/libs/beast/doc/html/beast/ref/boost__beast__tcp_stream.html
        stream.connect(domain_results); // the resolver gives us a list of possible endpoints back to which we can connect. The .connect() method creates a connection to the first endpoint that accepts the connection in the result list of the resolver 

        // create the HTTP request for the connected server
        boost::beast::http::request<boost::beast::http::string_body> request_instance(boost::beast::http::verb::get, query, version); // setting up the main information for the header as well as the payload with the "query" parameter
        request_instance.set(boost::beast::http::field::host, host); // Set the host information of the http header for the request: https://www.tutorialspoint.com/de/http/http_header_fields.htm (look for host)
        request_instance.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING); // setting the user agent: https://www.geeksforgeeks.org/http-headers-user-agent/
            // you can look up the most HTTP header details here: https://www.tutorialspoint.com/de/http/http_header_fields.htm
        
        // send the HTTP request to the connected server
        boost::beast::http::write(stream, request_instance);
        
        // receive the answer from the connected server
            // create a buffer instance for saving the received information from the server
        boost::beast::flat_buffer buffer_instance; // A flat_buffer is a continious part of memory that is used to store the received information from the socket. (https://www.boost.org/doc/libs/1_67_0/libs/beast/doc/html/beast/ref/boost__beast__flat_buffer.html) It is the equivivalent of the boost::asio::buffer instances and it OWNS the memory that is associated with it (TODO: Is the correct?)
            // create a container that holds the response after receiving it within the buffer instance
        boost::beast::http::response<boost::beast::http::dynamic_body> response_instance; // The dynamic_body means that we use a dynamic buffer for storing the payload (https://www.boost.org/doc/libs/develop/libs/beast/doc/html/beast/ref/boost__beast__http__dynamic_body.html)
            // http::response https://www.boost.org/doc/libs/develop/libs/beast/doc/html/beast/ref/boost__beast__http__response.html
        boost::beast::http::read(stream, buffer_instance, response_instance); // is a free read function, like the boost::asio::read() and boost::asio::write() functions 

        // print out the http response from the server
        std::cout << "The response from the server is:\n\n" << response_instance << "\n";

        // close the connection of the stream object
        
        stream.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec); 
            // access the reference to the underlying socket by calling tcp_stream.socket() -> then it is basic boost::asio syntax
        

    } catch (std::exception const & e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        return EXIT_FAILURE;
    }

   std::cout << "Finished program.\n"; 
   return 0;  
}

/*
* Compile Command: 
*   $ g++ simple_http_get_client.cpp -I/usr/local/include -L/usr/local/lib -lboost_coroutine -lboost_context -lboost_system --std=c++17 -pthread
*/