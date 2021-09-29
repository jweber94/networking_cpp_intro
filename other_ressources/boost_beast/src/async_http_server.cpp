#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

#include <boost/lexical_cast.hpp>
#include <boost/config.hpp>
#include <boost/asio.hpp>

#include <iostream>

/*
* -------------------------------------------------------------
* Utility functions
* -------------------------------------------------------------
*/

// Get the corresponding HTTP payload type to the (from the client) requested file ending (MIME = Multipurose Internet Mail Extension) 
boost::beast::string_view mime_type(boost::beast::string_view path)
{
    // Explaination to MIME: https://de.wikipedia.org/wiki/Multipurpose_Internet_Mail_Extensions
    using boost::beast::iequals;
    auto const ext = [&path]
    {
        auto const pos = path.rfind(".");
        if(pos == boost::beast::string_view::npos)
            return boost::beast::string_view{};
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

// create relative path for the client root-definiton to the requested ressource
std::string path_cat(boost::beast::string_view base, boost::beast::string_view path)
{
    if(base.empty())
        return std::string(path);
    std::string result(base);
    #ifdef BOOST_MSVC
        // for a windows host
        char constexpr path_separator = '\\';
        if(result.back() == path_separator)
            result.resize(result.size() - 1);
        result.append(path.data(), path.size());
        for(auto& c : result)
            if(c == '/')
                c = path_separator;
    #else
        // for a UNIX host
        char constexpr path_separator = '/';
        if(result.back() == path_separator)
            result.resize(result.size() - 1);
        result.append(path.data(), path.size());
    #endif
    return result;
}

void fail(boost::system::error_code ec, char const * what)
{
    std::cerr << what << ": " << ec.message() << "\n";  
}

// This function produces an HTTP response for the given
// request. The type of the response object depends on the
// contents of the request, so the interface requires the
// caller to pass a generic lambda for receiving the response.
template<
    class Body, class Allocator,
    class Send>
void
handle_request(
    boost::beast::string_view doc_root,
    boost::beast::http::request<Body, boost::beast::http::basic_fields<Allocator>>&& req,
    Send&& send)
{
    // Returns a bad request response
    auto const bad_request =
    [&req](boost::beast::string_view why)
    {
        boost::beast::http::response<boost::beast::http::string_body> res{boost::beast::http::status::bad_request, req.version()};
        res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(boost::beast::http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = std::string(why);
        res.prepare_payload();
        return res;
    };

    // Returns a not found response
    auto const not_found =
    [&req](boost::beast::string_view target)
    {
        boost::beast::http::response<http::string_body> res{boost::beast::http::status::not_found, req.version()};
        res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(boost::beast::http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = "The resource '" + std::string(target) + "' was not found.";
        res.prepare_payload();
        return res;
    };

    // Returns a server error response
    auto const server_error =
    [&req](boost::beast::string_view what)
    {
        boost::beast::http::response<boost::beast::http::string_body> res{boost::beast::http::status::internal_server_error, req.version()};
        res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(boost::beast::http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = "An error occurred: '" + std::string(what) + "'";
        res.prepare_payload();
        return res;
    };

    // Make sure we can handle the method
    if( req.method() != boost::beast::http::verb::get &&
        req.method() != boost::beast::http::verb::head)
        return send(bad_request("Unknown HTTP-method"));

    // Request path must be absolute and not contain "..".
    if( req.target().empty() ||
        req.target()[0] != '/' ||
        req.target().find("..") != boost::beast::string_view::npos)
        return send(bad_request("Illegal request-target"));

    // Build the path to the requested file
    std::string path = path_cat(doc_root, req.target());
    if(req.target().back() == '/')
        path.append("index.html");

    // Attempt to open the file
    boost::beast::error_code ec;
    boost::beast::http::file_body::value_type body;
    body.open(path.c_str(), boost::beast::file_mode::scan, ec);

    // Handle the case where the file doesn't exist
    if(ec == boost::beast::errc::no_such_file_or_directory)
        return send(not_found(req.target()));

    // Handle an unknown error
    if(ec)
        return send(server_error(ec.message()));

    // Cache the size since we need it after the move
    auto const size = body.size();

    // Respond to HEAD request
    if(req.method() == boost::beast::http::verb::head)
    {
        boost::beast::http::response<boost::beast::http::empty_body> res{boost::beast::http::status::ok, req.version()};
        res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(boost::beast::http::field::content_type, mime_type(path));
        res.content_length(size);
        res.keep_alive(req.keep_alive());
        return send(std::move(res));
    }

    // Respond to GET request
    boost::beast::http::response<boost::beast::http::file_body> res{
        std::piecewise_construct,
        std::make_tuple(std::move(body)),
        std::make_tuple(boost::beast::http::status::ok, req.version())};
    res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(boost::beast::http::field::content_type, mime_type(path));
    res.content_length(size);
    res.keep_alive(req.keep_alive());
    return send(std::move(res));
}

/*
* -------------------------------------------------------------
* Session class
* -------------------------------------------------------------
*/

class Session : public std::enable_shared_from_this<Session>()
{
    public: 

        // C++ 11 implementation for a generic lambda function - since it needs to access the .response_ member variable of the Session class, it needs to be in the class scope of the Session class
        struct send_lambda
        {
            Session & self_; 

            send_lambda(Session & self) : self_(self)
            {
                std:::cout << "[SENDLAMBDA]: send_lambda initialized\n"; 
            }

            template<bool isRequest, class Body, class Fields>
            void operator()(boost::beast::http::message<isRequest, Body, Fields>&& msg) const 
            {
                auto sp = std::make_shared<boost::beast::http::message<isRequest, Body, Fields>> (std::move(msg));
                self_.response_ = sp;

                boost::beast::http::async_write(self_.stream_, *sp, std::bind(&Session::on_write, self_.shared_from_this(), sp->need_eof()));  
            }
        }; 

        Session(boost::asio::ip::tcp::socket && socket, std::shared_ptr<std::string const> const & doc_root) : stream_(std::move(socket)), doc_root_(doc_root), lambda_(*this)
        {
            std::cout << "[SERVER]: Session initialized.\n"; 
        }

        void run()
        {   
            boost::asio::dispatch(stream_.get_executor(), std::bind(&Session::do_read, shared_from_this()));
            // dispatch primes the I/O service object with the given do_read function. 
            // dispatch is quiet like boost::asio::post, see: https://stackoverflow.com/questions/2326588/boost-asio-io-service-dispatch-vs-post
        }

        void do_read()
        {
            // Make the request empty before reading since if we do not do this, it will result in undefined behaviour
            request_ = {}; 

            // set the timeout for the next logical operation
            stream_.expires_after(std::chrono::seconds(30));

            boost::beast::http::async_read(
                stream_, buffer_, request_, std::bind(&Session::on_read, shared_from_this())
            );  
        }

        void on_read(boost::system::error_code ec, std::size_t bytes_transferred)
        {
            boost::ignore_unused(bytes_transferred); // to avoid compiler moaning

            // if the client sends an end_of_stream
            if (ec == boost::beast::http::error::end_of_stream)
            {
                return do_close(); 
            } 
            
            // catch all other errors that can occure
            if (ec)
            {
                return fail(ec, "read");  
            }

            // if no error is occured, send the response
            handle_request(*doc_root_, std::move(request_), lambda_);
                // calls on_write 
        }

        void on_write(bool close, boost::system::error_code ec, std::size_t bytes_transferred)
        {
            // will get called from the lambda_ (send_lambda) instance from the end of on_read
            boost::ignore_unused(bytes_transferred); 

            if (ec)
            {
                return fail(ec, "write"); 
            }

            if (close)
            {
                return do_close(); 
            }

            response_ = nullptr; 

            // read the next request
            do_read(); 
        }

        void do_close()
        {
            boost::system::error_code ec; 

            stream_.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec); 

        }

    private: 

        boost::beast::tcp_stream stream_; 
        boost::beast::flat_buffer buffer_; 
        std::shared_ptr<std::string const> doc_root_; 
        boost::beast::http::request<boost::beast::http::string_body> request_; 
        std::shared_ptr<void> response_; 
        send_lambda lambda_; 
}; 



/*
* -------------------------------------------------------------
* Listener class
* -------------------------------------------------------------
*/

class Listener : public std::enable_shared_from_this<Listener>()
{
    public:
        Listener(boost::asio::io_service & ioserv, boost::asio::ip::tcp::endpoint endpoint, std::shared_ptr<std::string const> const & doc_root) : ioserv_(ioserv), acceptor_(boost::asio::make_strand(ioserv)), doc_root_(doc_root) 
        {
            boost::system::error_code ec; 

            // open up the acceptor
            acceptor_.open(endpoint.protocol(), ec);
                // endpoint.protocol() describes the internet protocol with which the Listener should listen for connections
                // https://www.boost.org/doc/libs/1_50_0/doc/html/boost_asio/reference/ip__basic_endpoint/protocol.html
                // The acceptor has a method for accepting on a defined protocol (Ref.: https://www.boost.org/doc/libs/1_66_0/doc/html/boost_asio/reference/basic_socket_acceptor/open.html)
            if (ec){
                std::cerr << "[SERVER]: Error on opening up the acceptor.\n"; 
                return; 
            }

            // Setup the Listener settings for the connection handling
                // make address reuse possable 
            acceptor_.set_option(boost::asio::socket_base::reuse_address(true), ec);              
                // https://stackoverflow.com/questions/3229860/what-is-the-meaning-of-so-reuseaddr-setsockopt-option-linux
            if (ec)
            {
                std::cerr << "[SERVER]: Error on setting the listener to reuse_address.\n"; 
                return; 
            }

                // bind the listening address to the acceptor instance
            acceptor_.bind(endpoint, ec);
            if (ec)
            {
                std::cerr << "[SERVER]: Error on binding the endpoint to the acceptor instance.\n"; 

                return; 
            } 

            // Start listening on the defined endpoint for new connections
            acceptor_.listen(boost::asio::socket_base::max_listen_connections, ec); 
            if (ec)
            {
                std::cerr << "[SERVER]: Error on listening for new connections\n"; 
                return; 
            }

            std::cout << "[SERVER]: Listener initialized\n"; 
        }

        void run()
        {
            do_accept(); 
        }

    private: 

        void do_accept()
        {
            // strand explained: https://stackoverflow.com/questions/25363977/what-is-the-advantage-of-strand-in-boost-asio
            // Every new connection gets its own strand, since we want to use the individual data exchange sockets without interrupting the read or write operation
            acceptor_.async_accept(boost::asio::make_strand(ioserv_), std::bind(&Listener::on_accept, shared_from_this())); 
                // the make_strand function delivers implicitly the data exchange socket (? TODO)
        }

        void on_accept(boost::system::error_code ec, boost::asio::ip::tcp::socket){
            if (!ec){
                // If the Client was successfully accepted, we want to start a Session for the data exchange with the data_exchange socket, that is implicitly created by the acceptor::async_accept() function that needs an acceptor competion handler to which the data exchange socket is handed over (see: )
                std::make_shared<Session>(std::move(socket), doc_root_)->run(); 
                    // create a Session instance for managing the data exchange 
            } 
            else 
            {
                std::cerr << "[SERVER]: Error on on_accept().\n"; 
                return; 
            }
        }

        boost::asio::io_service & ioserv_;
        boost::asio::ip::tcp::acceptor acceptor_; 
        std::shared_ptr<std::string const> doc_root_; 
}; 

/*
* -------------------------------------------------------------
* Entrence point
* -------------------------------------------------------------
*/

int main (int argc, char * argv [])
{   
    try 
    {
        // command line parser
        if (argc == 5)
        {   
            auto const address = boost::asio::ip::make_address(argv[1]); 
            int const port = boost::lexical_cast<int>(argv[2]);
            std::string const doc_root = boost::lexical_cast<std::string>(argv[3]);
            int demanded_threads = boost::lexical_cast<int>(argv[4]); 
            int threads; 
            if (demanded_threads < 1)
            {
                threads = 1; 
            } 
            else 
            {
                threads = demanded_threads; 
            }

            boost::asio::io_service ioserv; 

            // create a listener that will call Session instances in case of a connection request from a client
            std::make_shared<Listener>(ioserv, boost::asio::ip::tcp::endpoint(address, port), doc_root)->run(); 
                // calls implicitly the Listener class constructor

            // run the I/O service object on the number of demanded threads
            std::vector<std::thread> thread_vec; 
            thread_vec.reserve(threads - 1); // reserve the storage for all threads that were demanded up front on the command line

            for (auto i = threads -1; i > 0; --i)
            {   
                // go backwards throu the thread vector in order to not allocate more memory
                thread_vec.emplace_back(
                    [&ioserv](){
                        ioserv.run(); 
                    }
                ); 
                
            }

            ioserv.run(); 
            return 0; 
        } 
        else 
        {
            std::cerr << "Usage: http-server-async <address> <port> <doc_root> <threads>\n" <<
                         "Example:\n" <<
                        "    http-server-async 0.0.0.0 8080 . 1\n";
        
            return 1; 
        }
    }
    catch (std::exception & e)
    {
        std::cerr << "An error occured: " << e.what()<< "\n";
        return 1;  
    }


    std::cout << "Finished program.\n"; 
    return 0; 
}

/*
* Compile Command: 
*   $ g++ async_http_server.cpp -I/usr/local/include -L/usr/local/lib -lboost_coroutine -lboost_context -lboost_system --std=c++17 -pthread
*/