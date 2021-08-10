#include <iostream>
#include <boost/asio/io_service.hpp>


int main(){

    boost::asio::io_service ioserv; // I/O servive object to manage the I/O services that correspond to the I/O objects
    std::cout << "Hello World\n"; 

    return 0; 
}