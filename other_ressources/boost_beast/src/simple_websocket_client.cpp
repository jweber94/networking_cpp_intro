#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>

#include <cstdlib>
#include <iostream>
#include <string>



int main(int argc, char * argv []){

    try 
    {
        if (argc != 4){
            std::cerr << "Usage: websocket-client-sync <host> <port> <text>\n" <<
                            "Example:\n" <<
                            "    websocket-client-sync echo.websocket.org 80 \"Hello, world!\"\n";

            return 1; 
        }

        // parse the commend line arguments
        std::string host = boost::lexical_cast<std::string>(argv[1]);
        std::string port = boost::lexical_cast<std::string>(argv[2]); 
        std::string text = boost::lexical_cast<std::string>(argv[3]); 

        // create the asio interface
        boost::asio::io_service ioserv; 
        boost::asio::ip::tcp::resolver resolver(ioserv); 
        boost::system::error_code ec; 

        boost::asio::ip::tcp::resolver::results_type results = resolver.resolve(host, port, ec); 

        // create the beast interface
        boost::beast::websocket::stream<boost::asio::ip::tcp::socket> websocket_stream_instance(ioserv);

        // synchronously connect to the server
        boost::asio::ip::tcp::endpoint data_ep = boost::asio::connect(websocket_stream_instance.next_layer(), results); 
            // .next_layer() returns a reference to the next underlying stream instance of the websocket stream instance 
            // returns the first endpoint to which the connect function could establish a connection from the results of the resolver (which delivers a list of possable endpoints)

        // update the host string
        host += ':' + std::to_string(data_ep.port()); // The port of the data exchange is a different one then the port where the client trys to connect to the server  

        // Set a decorator to change the user-agent 
        websocket_stream_instance.set_option(boost::beast::websocket::stream_base::decorator(
            [](boost::beast::websocket::request_type & req){
                req.set(boost::beast::http::field::user_agent, std::string(BOOST_BEAST_VERSION_STRING) + " websocket-client-coro"); 
            }
        )); 
            // stream.set_option() is a global option for the data exchange stream and NOT for the message itself. It impacts ALL messages that are send throu the (socket-)stream object
            // Decorator: https://www.boost.org/doc/libs/1_70_0/libs/beast/doc/html/beast/using_websocket/decorator.html
            // A decorator is a function (-pointer) that is executed immediatly before the HTTP or WebSocket message is send throu the socket

        // upgrade the client-server connection from HTTP to WebSocket
        websocket_stream_instance.handshake(host, "/"); // https://www.boost.org/doc/libs/1_66_0/libs/beast/doc/html/beast/ref/boost__beast__websocket__stream/handshake/overload1.html
            // the second argument is the request target, which may or may not be empty. It defines a (virtual) ressource on the server side. In this example, the root directory is requested

        // Send the WebSocket request to the server
        websocket_stream_instance.write(boost::asio::buffer(text)); 

        // create a beast buffer and receive the server answer into it
        boost::beast::flat_buffer receive_buffer; 
        websocket_stream_instance.read(receive_buffer);  
    
        // close the connection to the server
        websocket_stream_instance.close(boost::beast::websocket::close_code::normal);
        std::cout << "Connection successfully closed!\n"; 

        // print out the received message from the server
        std::cout << "This is the received message from the server:\n\n" << boost::beast::make_printable(receive_buffer.data()) << "\n"; 

    } 
    catch (std::exception const & e) 
    {
        std::cerr << "An error occured: " << e.what() << "\n"; 
        return 1; 
    }

    std::cout << "Finished program!\n"; 

    return 0; 
}

/*
* Compile Command: 
*   $ g++ simple_websocket_client.cpp -I/usr/local/include -L/usr/local/lib -lboost_coroutine -lboost_context -lboost_system --std=c++17 -pthread
*/