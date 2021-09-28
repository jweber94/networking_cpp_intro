/*
* This is my implementation of a little simpler version of the boost::beast example of a synchronous (!) http server application based on this example: https://www.boost.org/doc/libs/1_77_0/libs/beast/example/http/server/sync/http_server_sync.cpp
*/

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

#include <boost/asio.hpp>
#include <boost/config.hpp>
#include <boost/lexical_cast.hpp>

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

/*
* ---------------------------------------------------------------------------
* Utility functions
* ---------------------------------------------------------------------------
*/

// failure report
void fail(beast::error_code ec, char const* what)
{
    std::cerr << what << ": " << ec.message() << "\n";
}

std::string path_cat(boost::beast::string_view base, boost::beast::string_view path)
{
    if (base.empty())
    {
        return std::string(path); 
    }

    std::string result(base); 

    #ifdef BOOST_MSVC
    // Windows case
    std::cout << "CAUTION: Windows case\n"; 
    char constexpr path_separator = '\\'; 

    if (result.back() == path_separator)
    {
        result.resize(result.resize() - 1); 
    }
    result.append(path.data(), path.size());  

    for (auto & c : result)
    {
        // iterate over all characters of the requested path and change the default path separator to the windows path separator in the case that we are on a windows machine
        if (c == '/'){
            c = path_separator
        }
    }
    #else 
    std::cout << "CAUTION: UNIX case\n"; 
    char constexpr path_separator = '/'; 
    if (result.back() == path_separator)
    {
        result.resize(result.size() - 1); 
    }
    result.append(path.data(), path.size()); 

    #endif

    return result; 
}

boost::beast::string_view mime_type(boost::beast::string_view path)
{
    // MIME = Multi Purpose Internet Mail Extension - allows to exchange diffent types of data between client and servers. This will be written to the header of the HTTP message (which might be important for DPI)
    using boost::beast::iequals; 

    auto const ext = [&path] {
        auto const pos = path.rfind("."); // looking for the file extension within the requested ressource path (i.e. ,jpeg)

        if (pos == boost::beast::string_view::npos)
        {
            return boost::beast::string_view {}; 
        }
        return path.substr(pos); // return everything from the string_view after (and including) the pos index until the end of the string_view 
    } (); // ext is defined as a function pointer

    // define the individual HTTP payload types according to the requested (and valid) data 
    if(iequals(ext, ".htm"))  return "text/html";
    if(iequals(ext, ".html")) return "text/html";
    if(iequals(ext, ".php"))  return "text/html";
    if(iequals(ext, ".css"))  return "text/css";
    if(iequals(ext, ".txt"))  return "text/plain";
    if(iequals(ext, ".js"))   return "application/javascript";
    if(iequals(ext, ".json")) return "application/json";
    if(iequals(ext, ".xml"))  return "application/xml";
    if(iequals(ext, ".swf"))  return "application/x-shockwave-flash";
    if(iequals(ext, ".flv"))  return "video/x-flv";
    if(iequals(ext, ".png"))  return "image/png";
    if(iequals(ext, ".jpe"))  return "image/jpeg";
    if(iequals(ext, ".jpeg")) return "image/jpeg";
    if(iequals(ext, ".jpg"))  return "image/jpeg";
    if(iequals(ext, ".gif"))  return "image/gif";
    if(iequals(ext, ".bmp"))  return "image/bmp";
    if(iequals(ext, ".ico"))  return "image/vnd.microsoft.icon";
    if(iequals(ext, ".tiff")) return "image/tiff";
    if(iequals(ext, ".tif"))  return "image/tiff";
    if(iequals(ext, ".svg"))  return "image/svg+xml";
    if(iequals(ext, ".svgz")) return "image/svg+xml";
    return "application/text";

}

/*
 * ---------------------------------------------------------------------------
 * Handling the HTTP request from the server and create an appropriate answer - mainly boost::beast code
 * ---------------------------------------------------------------------------
 */

// Creating the HTTP response based on the HTTP request - Since the response is defined by the request from the client, therefore a generic lambda for receiving the response is neede
template <class Body, class Allocator, class Send>
void handle_request(boost::beast::string_view doc_root, boost::beast::http::request<Body, boost::beast::http::basic_field<Allocator> && request, Send && send)
{
    // REQUEST HANDLER
    auto const bad_request = [&request](boost::beast::string_view why) {
        // boost::beast::string_view is the equivivalent of std::string that is used with the boost::beast library
        boost::beast::http::response<boost::beast::string_view> response(boost::beast::http::status::bad_request, request.version());
            // create the response instance 

        // setting for the response instance before it is send to the client
        response.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
        response.set(boost::beast::http::field::content_type, "text/html");
        
        response.keep_alive(request.keep_alive());
        response.body() = std::string(why);

        response.prepare_payload(); // setting up the header information about the content (i.e. content length, ...)
            // https://www.boost.org/doc/libs/1_67_0/libs/beast/doc/html/beast/ref/boost__beast__http__message/prepare_payload.html

        return response; 
    };

    auto const not_found = [&request] (boost::beast::string_view target) {
        boost::beast::http::response<boost::beast::http::string_body> response(boost::beast::http::status::not_found, request.version()); 

        response.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
        response.set(boost::beast::http::content_type, "test/html");

        response.keep_alive(request.keep_alive());
        response.body() = "The ressource: '" + std::string(target) + "' was not found."; 

        response.prepare_payload(); 

        return response; 
    };

    auto const server_error = [&request] (boost::beast::string_view what) {
        boost::beast::http::response<boost::beast::http::string_body> response(boost::beast::http::status::internal_server_error, request.version()); 

        response.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
        response.set(boost::beast::http::content_type, "test/html");

        response.keep_alive(request.keep_alive());
        response.body() = "An error occured: '" + std::string(what) + "'"; 

        return response; 
    };

    // CREATEING THE REQUEST

    // ensure that we have received a valid HTTP request from the client and if not we want to directly rejuct it
    if ((request.method() != boost::beast::http::verb::get) && (request.method() != boost::beast::http::verb::head))
    { 
        // send a bad request if we did not received a HTTP GET or HEAD request
        return send(bad_request("Unknown HTTP-method")); 
    }

    // the requested target must be an absolute path and not a relativ path
    if (request.target().empty() || req.target()[0] != '/' || req.target().find("..") != beast::string_view::npos)
    {
        return send(bad_request("Illegal request-target")); 
    }

    // create the response path from the requested target ressource
    std::string path = path_cat(doc_root, request.target());
    if (request.target().back() == '/')
    {
        path.append("index.html"); 
    } 

    // try to open the requested ressource
    boost::system::error_code ec; 
    boost::beast::http::file_body::value_type body;
    body.open(path.c_str(), boost::beast::file_mode::scan, ec);

    // case 1: Requested a not valid ressource
    if (ec == boost::errc::no_such_file_or_directory)
    {
        return send(not_found(request.target())); 
    } 

    // case 2: An unknown error occured
    if (ec)
    {
        return send(server_error(ec.message())); 
    }

    // case 3: A valid request was received
    auto const size = body.size(); 

    // case 3.1: HEAD request
    if (request.method() == boost::beast::http::verb::head)
    {
        boost::beast::http::response<boost::beast::http::empty_body> response(boost::beast::http::status::ok, request.version()); 

        response.set(boost::beast::http::fiel::server, BOOST_BEAST_VERSION_STRING);
        response.set(boost::beast::http::field::content_type, mime_type(path));

        response.content_length(size); 

        response.keep_alive(request.keep_alive()); 

        return send(std::move(response));   
    } 

    // case 3.2: GET request
    boost::beast::http::response:<boost::beast::http::file::file_body> response(std::piecewise_construct, std::make_tuple(std::move(body), std::make_tuple(boost::beast::http::status::ok, request.version())));

    response.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
    response.set(boost::beast::http::field::content_type, mime_type(path));

    response.content_length(size);

    response.keep_alive(request.keep_alive());

    return send(std::move(response));      

} /* Handle request */


/*
 * ---------------------------------------------------------------------------
 * Sending and receiving - mainly boost::asio and a little bit of boost::beast
 * ---------------------------------------------------------------------------
 */

// C++11 implementation of a generic lambda function
template <class Stream>
struct send_lambda{
   Stream & stream_; 
   bool & close_; 
   boost::beast::error_code & ec;


   send_lambda(Stream & stream, bool & close, boost::system::error_code & ec) : stream_(stream), close_(close), ec_(ec)
   {
       std::cout << "[SESSION]: Lambda initialized\n"; 
   } 

   template <bool isRequest, class Body, class Fields>
   void operator()(boost::beast::http::message<isRequest, Body, Fields>&& msg) const
   {
        // determine if all data was received - indicates that no more data can be read out from a datasource for the OS, Ref.: https://en.wikipedia.org/wiki/End-of-file
        close_.msg.need_eof(); 

        // create a serializer in order to make the non-const file body a const message for the boost::beast::http::write (free) function - http::write just can take const 
        // messages but the one that is handed over is not const (since we do not know what is within the message)    
        boost::beast::serializer<isRequest, Body, Fields> serializer(msg); 
            // The serializer works as an output-buffer in the beast syntax (TODO: What is the difference between a buffer and a serializer in boost::beast?)
        boost::beast::http::write(stream_, serializer, ec_); 
   }
};  /* send_lambda */


void do_session(boost::asio::ip::tcp::socket & data_socket, std::shared_ptr<std::string const>( const & doc_root)){
    bool close = false; 
    boost::system::error_code ec; 

    boost::beast::flat_buffer session_buffer; 

    // create a function pointer of the generic send_lambda function (which can be used with any kind of socket/stream since it is a generic template for the used socket or stream instance)
    // this is used for sending th response to the requesting client (it calls the boost::beast::http::write free function, which is a wrapper around the boost::asio::write free function)
    send_lambda<boost::asio::ip::tcp::socket> lambda_send_fcnptr(data_socket, close, ec);

    // read the request from the client
    for (;;) // infinite for loop
    {
        // read message from client
        boost::beast::http::request<boost::beast::http::string_body> request; 
        boost::beast::http::read(data_socket, session_buffer, request, ec);  
            // synchronous and main-thread-blocking read from socket function of boost::beast
    
        if (ec == boost::beast::http::error_code::end_of_stream){
            break; // breake the infinite for loop, since we already received all data from the client because it sends us the end_of_stream signal with the latest data packet
        }

        // write response to client
        handle_request(*doc_root, std::move(request), lambda_send_fcnptr); 

        if (ec) {
            std::cerr << "[SESSION]: Error during read\n"; 
            return fail(ec, "write"); 
        }

        if (close) {
            // close the connection to the client - this is mostly done if the client sends a "Connection: close" within the HTTP header
            break;  
        }
    }

    data_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);
        // synchronously shutdown the connection by closing the data exchange socket 

    std::cout << "[SESSION]: Session finished successfully. Connection gracefully closed.\n"; 
}


int main(int argc, char * argv []){

    try 
    {
        // command line parser
        if (argc != 4){
            std::cerr << "Usage: http-server-sync <address> <port> <doc_root>\n" <<
                         "Example:\n" <<
                         "    http-server-sync 0.0.0.0 8080 .\n";
            return 1; 
        }

        boost::asio::ip::address address = boost::asio::ip::make_address(argv[1]);
        unsigned short port = boost::lexical_cast<unsigned short>(argv[2]);
        std::shared_ptr<std::string> const doc_root = std::make_shared<std::string>(boost::lexical_cast<std::string>(argv[3]));    
            // doc_root is the root of the code for the delivered website which will be noted as "/" for the client ressource

        // boost::asio stuff
        boost::asio::io_service ioserv; 
        boost::asio::ip::tcp::acceptor acceptor(ioserv, {address, port}); 

        for (;;) // infinite for-loop
        {
            // create a socket for the data exchange (must NOT be equal to the acceptor socket of the server)
            boost::asio::ip::tcp::socket data_socket(ioserv);

            // synchronously accept connections from clients
            acceptor.accept(data_socket); 
                // this will block the infinite for loop until a new connection is accepted

            std::thread{std::bind(&do_session, std::move(data_socket), doc_root)}.detach(); 
                // detach the thread for the session execution in order to make it possable to listen for new connections even if the data exchange is currently running
                // therefore, a std::move for the data exchange socket is necessary, since there will be a new socket associated with the data_socket variable 
                // for the next incoming connection
                // Reference for .detach(): https://stackoverflow.com/questions/22803600/when-should-i-use-stdthreaddetach
        }


    } catch (const std::exception & e) 
    {
        std::cerr << "[SERVER]: An exception occured - " << e.what() << "\n"; 
        return 1; 
    }


    return 0; 
}

/*
* Compile Command: 
*   $ g++ simple_http_server.cpp -I/usr/local/include -L/usr/local/lib -lboost_coroutine -lboost_context -lboost_system --std=c++17 -pthread
*/