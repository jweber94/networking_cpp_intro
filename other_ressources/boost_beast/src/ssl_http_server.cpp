#include <boost/beast/core.hpp>
#include <boost/beast/version.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/http.hpp>

#include <boost/config.hpp>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>

#include <memory>
#include <iostream>
#include <string>
#include <cstdlib>
#include <thread>
#include <vector>
#include <functional>

#include "common/server_certificate.hpp"

/*
* -------------------------------------------
* Utility functions
* -------------------------------------------
*/

void fail(boost::beast::error_code & ec, char const * what)
{
    /*
    * ssl::error::stream_truncated, also known as an SSL "short read",
    * indicates the peer closed the connection without performing the
    * required closing handshake (for example, Google does this to
    * improve performance). Generally this can be a security issue,
    * but if your communication protocol is self-terminated (as
    * it is with both HTTP and WebSocket) then you may simply
    * ignore the lack of close_notify.
    * 
    * Ref.: https://security.stackexchange.com/questions/91435/how-to-handle-a-malicious-ssl-tls-shutdown    
    */
    if(ec == boost::asio::ssl::error::stream_truncated)
        return;

    std::cerr << what << ": " << ec.message() << "\n";
}


std::string path_cat(boost::beast::string_view base, boost::beast::string_view path)
{
    if (base.empty())
    {
        return std::string(path); // we can convert a boost::beast::string_view easily to a std::string by calling the std::string constructor with the boost::beast::string_view
    }

    // create the relative path based on the server root folder configuration based on the main-command line parser
    std::string result(base); 

    #ifdef BOOST_MSVC
        // if we compile and run the server on a windows machine as the host system for the server code
        char constexpr path_separator = '\\'; 
        if (result.back() == path_separator)
        {
            result.resize(result.size() - 1); 
        }
        result.append(path.data(), path.size());
        
        for (auto & c : result)
        {
            if (c == '/')
            {
                c = path_separator; 
            }
        } 
    #else
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
    // boost::beast::string_view inherits (and therefore extends) from std::string, so all methods of the std::string class are available from the boost::beast::string_view class 
    using boost::beast::iequals; 

    // create a function pointer for finding the (requested) file extension 
    auto const ext = 
        [&path]{
            auto const pos = path.rfind("."); 

            if (pos == boost::beast::string_view::npos)
            {
                return boost::beast::string_view{}; 
            }
            return path.substr(pos); 
        }();  

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

template<class Body, class Allocator, class Send>
void handle_request(boost::beast::string_view doc_root, boost::beast::http::request<Body, boost::beast::http::basic_fields<Allocator>> && request, Send && send)
{
    /* 
    * doc_root = directory where the virtual home directory for the client is placed
    * request = request that was received from the boost::beast::async_read
    * send = send_lambda that was defined within the Session instance
    */

   // define possible responses, based on the received request
   auto const bad_request = 
    [&request](boost::beast::string_view why)
    {
        boost::beast::http::response<boost::beast::http::string_body> response(boost::beast::http::status::bad_request, request.version()); 

        response.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
        response.set(boost::beast::http::field::content_type, "text/html");

        response.keep_alive(request.keep_alive()); 
        response.body() = std::string(why);

        response.prepare_payload(); 

        return response;  
    }; 

    auto const not_found =
    [&request](boost::beast::string_view target)
    {
        boost::beast::http::response<boost::beast::http::string_body> res{boost::beast::http::status::not_found, request.version()};
        res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(boost::beast::http::field::content_type, "text/html");
        res.keep_alive(request.keep_alive());
        res.body() = "The resource '" + std::string(target) + "' was not found.";
        res.prepare_payload();
        return res;
    };

    // Returns a server error response
    auto const server_error =
    [&request](boost::beast::string_view what)
    {
        boost::beast::http::response<boost::beast::http::string_body> res{boost::beast::http::status::internal_server_error, request.version()};
        res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(boost::beast::http::field::content_type, "text/html");
        res.keep_alive(request.keep_alive());
        res.body() = "An error occurred: '" + std::string(what) + "'";
        res.prepare_payload();
        return res;
    };

    // process the request in order to create the correct response

        // catch the obviously mallicious requests and answer them
    // check that we have received a valid HTTP method request from the client
    if ( (request.method() != boost::beast::http::verb::get) && (request.method() != boost::beast::http::verb::head) )
    {
        return send(bad_request("Unsupported HTTP-method")); 
    }

    // requested ressource path must NOT contain a .. - it needs to be an absolute path 
    if ( request.target().empty() || request.target()[0] != '/' || request.target().find("..") != boost::beast::string_view::npos )
    {
        return send(bad_request("Illegal request-target")); 
    }

        // cope with a basically valid request
    // check if the requested ressource is acutally available on the server OS
    std::string path = path_cat(doc_root, request.target()); // boost::beast::request::target() delivers a boost::beast::string_view object back that can be processed with std::string operations
    if (request.target().back() == '/') 
    {
        path.append("index.html"); 
    }

    // try to open the requested file
    boost::beast::error_code ec; 
    boost::beast::http::file_body::value_type body; 
    body.open(path.c_str(), boost::beast::file_mode::scan, ec);

        // catch the error if a not existing file was requested
    if (ec == boost::beast::errc::no_such_file_or_directory)
    {
        return send(server_error(ec.message())); 
    }

    // cache the size of the response body
    std::uint64_t const size = body.size(); 

    // answer to a HTTP HEAD-request
    if (request.method() == boost::beast::http::verb::head)
    {
        boost::beast::http::response<boost::beast::http::empty_body> response(boost::beast::http::status::ok, request.version());

        response.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
        response.set(boost::beast::http::field::content_type, mime_type(path));

        response.content_length(size);
        response.keep_alive(request.keep_alive());

        return send(std::move(response));     
    }

    // answer to a HTTP GET-request
    boost::beast::http::response<boost::beast::http::file_body> response(std::piecewise_construct, std::make_tuple(std::move(body)), std::make_tuple(boost::beast::http::status::ok, request.version())); 

    response.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
    response.set(boost::beast::http::field::content_type, mime_type(path));

    response.content_length(size);
    response.keep_alive(request.keep_alive());

    return send(std::move(response));     
} /* handle_request */

/*
* -------------------------------------------
* Session class (management of the server-client data exchange)
* -------------------------------------------
*/

class Session : public std::enable_shared_from_this<Session>
{

    // generic lambda that is used for sending the HTTP message to the client
        // needs to be in the scope of the Session class, because it needs to access the private members 
        struct send_lambda
        {
            Session & self_; 

            // constructor
            send_lambda(Session & self) : self_(self)
            {
                std::cout << "[SERVER]: send_lambda for the Session initialized.\n"; 
            }

            template<bool isRequest, class Body, class Fields>
            void operator()(boost::beast::http::message<isRequest, Body, Fields> && msg) const {
                // the const says that the () operator should NOT modify the members of the send_lambda (i.e. Session instance reference) instance
                
                // create a shared pointer to a http message instance on the heap and move the rvalue referenced message that was handed over to the () operator to the created shared pointer (to allocate as less memory as possible)
                auto msg_sh_ptr = std::make_shared<boost::beast::http::message<isRequest, Body, Fields> >(std::move(msg)); 
                self_.response_ = msg_sh_ptr; // let the Session.response_ shared pointer point to the created message object on the head as well (so it will not get deleted after the scope of this () operator)

                // write the response to the data_exchange_socket_ of the Session instance
                boost::beast::http::async_write(self_.stream_, *msg_sh_ptr, std::bind(&Session::on_write, self_.shared_from_this(), msg_sh_ptr->need_eof(), std::placeholders::_1, std::placeholders::_2)); 
                    // Caution, we need the self_.shared_from_this() since we need to access the self_ & instance of the send_lambda function
            }
        };

    public: 

        // constructor
        Session(boost::asio::ip::tcp::socket && data_exchange_socket, boost::asio::ssl::context & ssl_context, std::shared_ptr<std::string const> const & doc_root) : stream_(std::move(data_exchange_socket), ssl_context), doc_root_(doc_root), lambda_(*this)
        {
            // The data_exchange_socket is NOT a explicit member of the Session instance, since it is implicitly contained within the boost::beast::ssl_stream instance that IS a member of the Session instance 
            std::cout << "[SESSION]: Session initialized.\n"; // TODO: Create a (static) Session counter
        }
        



        // public member methods (interface for using the Session class)
        void run()
        {
            boost::asio::dispatch(stream_.get_executor(), std::bind(&Session::on_run, shared_from_this()));  
        }

        void on_run()
        {
            // running the Session means that we want the data exchange to happen. Therefore, we need a TLS handshake beforehand 

            // set the timeout for the next logical operation, which is the TLS handshake
            boost::beast::get_lowest_layer(stream_).expires_after(std::chrono::seconds(30)); 

            stream_.async_handshake(
            boost::asio::ssl::stream_base::server,
            boost::beast::bind_front_handler(
                &Session::on_handshake,
                shared_from_this()));
            //stream_.async_handshake(boost::asio::ssl::stream_base::server, std::bind(&Session::on_handshake, shared_from_this())); 
        }

        void on_handshake(boost::beast::error_code ec)
        {
            if (!ec)
            {
                // is the handshake is successfully completed, this completion handler is invoked and we are now able to read the encrypted traffic from the socket
                do_read(); 
            } 
            else 
            {
                fail(ec, "handshake");
                return; 
            }
        }

        void do_read()
        {   
            // initialize the request_ member of the Session instance as empty in order to avoid undefined behaviour
            request_ = {}; 

            // set the timeout for the next logical operation, i.e. the read out from the socket
            boost::beast::get_lowest_layer(stream_).expires_after(std::chrono::seconds(30)); 

            // do the read out asynchronously and call on_read to process the received request during the asynchronously scheduled read operation
            boost::beast::http::async_read(stream_, buffer_, request_, std::bind(&Session::on_read, shared_from_this(), std::placeholders::_1, std::placeholders::_2)); 
        }

        
        void on_read(boost::beast::error_code ec, std::size_t bytes_transferred)
        {
            boost::ignore_unused(bytes_transferred); // to avoid compiler moaning

            // check if an error occured or we received an end_of_stream signal from the client - is this is the case, we want to close the stream_ instance (and therefore the Session instance)
            if (ec == boost::beast::http::error::end_of_stream)
            {
                return do_close(); 
            }
            if (ec) 
            {
                std::cout << "Read fail\n"; 
                return fail(ec, "read"); 
            }

            // if no error occures, we want to respond to the received (and read out) request from the client
            handle_request(*doc_root_, std::move(request_), lambda_); 
                // this will call on_write() from the self_ & instance of the send_lambda member variable lambda_
        }

        void on_write(bool close, boost::beast::error_code ec, std::size_t bytes_transferred)
        {
            // async_write (i.e. write to the data exchange socket) completion handler
            boost::ignore_unused(bytes_transferred); // to avoid compiler moaning

            // catch the error if one occures during the asynchronous write operation
            if (ec)
            {
                return fail(ec, "write"); 
            }

            // the boost::beast::async_write (free) function will set the bool close parameter of this completion handler to true, if the client request says in the HTTP header "Connection: close" 
            if (close)
            {
                return do_close(); 
            }

            // we already processed the response within the handle_request function, which was called from the on_read method of the Session instance, so we can delete it (i.e. setting the last remaining shared_ptr to nullptr and the smartpointer will automatically delete the boost::beast::http::request instance on the heap)
            response_ = nullptr;

            // schedule another asynchronous read operation if the connection is still open and no error occured during the last read
            do_read();   
        }

        void do_close()
        {
            // set a timeout for a soft shutdown of the connection
            boost::beast::get_lowest_layer(stream_).expires_after(std::chrono::seconds(30));

            // schedule the soft shutdown of the connection
            stream_.async_shutdown(std::bind(&Session::on_shutdown, shared_from_this(), std::placeholders::_1));
        }

        void on_shutdown(boost::system::error_code ec)
        {
            if (!ec)
            {
                std::cout << "[SESSION]: successfully shut down.\n"; 
            } 
            else 
            {
                return fail(ec, "shutdown"); 
            }
        }

    private: 

        // private member methods (aka. managing the Session logic)

        // member variables
        boost::beast::ssl_stream<boost::beast::tcp_stream> stream_; 
        boost::beast::flat_buffer buffer_; 
        std::shared_ptr<std::string const> doc_root_; 
        boost::beast::http::request<boost::beast::http::string_body> request_; 
        std::shared_ptr<void> response_; 
        send_lambda lambda_; 
};
/*
* -------------------------------------------
* Listener class (aka. the server logic) 
* -------------------------------------------
*/

class Listener : public std::enable_shared_from_this<Listener>
{
    public: 
        // constructor
        Listener(boost::asio::io_service & ioserv, boost::asio::ssl::context & ssl_context, boost::asio::ip::tcp::endpoint endpt, std::shared_ptr<std::string const> const & doc_root) : ioserv_(ioserv), ssl_context_(ssl_context), acceptor_(ioserv), doc_root_(doc_root)
        {
            // setting up the connection acceptance with the SSL/TLS context and its settings
            boost::beast::error_code ec; 

            // set the data exchange protocol and catch a possible error
            acceptor_.open(endpt.protocol(), ec);
                // with the protocol setting, you can choose if you want to use a stream_protocol, raw_protocol, seq_packet_protocol (TCP) or datagram_protocol (UDP)
                // Ref.: https://www.boost.org/doc/libs/1_65_1/doc/html/boost_asio/overview/networking/other_protocols.html
            if (ec)
            {
                fail(ec, "open"); 
                return; 
            }            

            // allow address reuse for the case that we exit the server program, recofigure it and start it over again, before the network timeout was finished
            // Ref.: https://stackoverflow.com/questions/3229860/what-is-the-meaning-of-so-reuseaddr-setsockopt-option-linux
            acceptor_.set_option(boost::asio::socket_base::reuse_address(true), ec); 
            if (ec)
            {
                fail(ec, "set_option");
                return;  
            }

            // connect the endpoint with the acceptor to make it listen for new client connection requests on the (user defined) port
            acceptor_.bind(endpt, ec);
            if (ec)
            {
                fail(ec, "bind");
                return;  
            } 

            // start listening for connections
            acceptor_.listen(boost::asio::socket_base::max_listen_connections, ec); 
            if (ec)
            {
                fail(ec, "listen"); 
                return; 
            }
            
            std::cout << "[SERVER]: Listener instance initialized.\n"; 
        }

        // member methods public (user interface)
        void run()
        {
            do_accept(); 
        }

    private: 
        // member methods private
        void do_accept()
        {
            acceptor_.async_accept(boost::asio::make_strand(ioserv_), std::bind(&Listener::on_accept, shared_from_this(), std::placeholders::_1, std::placeholders::_2)); 
                // the acceptor creates the data exchange socket while accepting a new client connection and hands it over to the accept completion handler
        }

        void on_accept(boost::beast::error_code ec, boost::asio::ip::tcp::socket data_exchange_socket)
        {
            if (!ec)
            {   
                // if no error on connection occures, we want to establish and hold a connection to the client with a Session instance
                std::make_shared<Session>(std::move(data_exchange_socket), ssl_context_, doc_root_)->run(); 
            } else 
            {
                fail(ec, "accept");
            }

            // schedule the next asynchronous listening for a new connection
            do_accept(); 
        }

        // member variables
        boost::asio::io_service & ioserv_;
        boost::asio::ssl::context & ssl_context_;
        boost::asio::ip::tcp::acceptor acceptor_; 
        std::shared_ptr<std::string const> doc_root_;   
};

int main (int argc, char * argv [])
{
    std::cout << "Server program started\n"; 

    try
    {
        // command line parser
        if (argc != 5)
        {
            std::cerr <<
            "Usage: http-server-async-ssl <address> <port> <doc_root> <threads>\n" <<
            "Example:\n" <<
            "    http-server-async-ssl 0.0.0.0 8080 . 1\n";
            return 1;
        }

        boost::asio::ip::address address = boost::asio::ip::make_address(argv[1]); 
        short unsigned int port = boost::lexical_cast<short unsigned int>(argv[2]);
        std::shared_ptr<std::string> const doc_root = std::make_shared<std::string>(argv[3]);
        int const threads = std::max<int>(1, boost::lexical_cast<int>(argv[4]));      
            // if an invalid number of threads (less then 1) was given, we choose just one thread for execution

        // create the I/O service object as well as the SSL context (which hold the SSL/TLS certificates)
        boost::asio::io_service ioserv; 
        boost::asio::ssl::context ssl_context(boost::asio::ssl::context::tlsv12); 

        // SSL configuration, see ./common/server_certificate.hpp
        load_server_certificate(ssl_context); 

        // create a Listener instance
        std::make_shared<Listener>(ioserv, ssl_context, boost::asio::ip::tcp::endpoint{address, port}, doc_root)->run(); 
            // since the Listener class has a std::enable_shared_from_this() inheritance, there will be a shared pointer to the listener instance as long as no error occures

        // create a thread pool for executing the I/O service object
        std::vector<std::thread> thread_vec; 

        thread_vec.reserve(threads - 1);
        for (auto i = threads - 1; i > 0; --i)
        {
            thread_vec.emplace_back(
                [&ioserv](){
                    ioserv.run(); 
                }
            );
        }

        ioserv.run();  

        return 0; 
    }
    catch (std::exception & e)
    {
        std::cerr << "[SERVER]: An exception occured - " << e.what() << "\n"; 
        return 1; 
    }


    std::cout << "Finished program.\n"; 
    return 0; 
}

/*
* Compile Command:
*   $ g++ ssl_http_server.cpp -I/usr/local/include -L/usr/local/lib -lboost_coroutine -lboost_context -lboost_system --std=c++17 -pthread -lcrypto -lssl
*/