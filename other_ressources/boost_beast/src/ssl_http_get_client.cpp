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
    boost::asio::

    std::cout << "Finished program\n";
    return 0; 
}
// https://www.boost.org/doc/libs/1_77_0/libs/beast/example/http/client/async-ssl/http_client_async_ssl.cpp
// https://www.boost.org/doc/libs/1_77_0/libs/beast/doc/html/beast/examples.html#beast.examples.clients
// https://medium.com/platform-engineer/web-api-design-35df8167460