#include <boost/asio/io_service.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <iostream>
#include <list>
#include <string>
#include <ctime>

#include <thread>
#include <chrono>

boost::asio::io_service ioserv; 
boost::asio::ip::tcp::endpoint tcp_endpt{boost::asio::ip::tcp::v4(), 2015};
boost::asio::ip::tcp::acceptor tcp_acceptor {ioserv, tcp_endpt};  
std::list<boost::asio::ip::tcp::socket> tcp_sockets; // list of data exchange sockets for interaction with multiple clients at the same time

// with coroutines, there is no need for a globaly stored data storage that is waiting to be send

void do_write(boost::asio::ip::tcp::socket &tcp_socket, boost::asio::yield_context yield){

    std::time_t now = std::time(nullptr);
    std::string data = std::ctime(&now);

    boost::asio::async_write(tcp_socket, boost::asio::buffer(data), yield);
    tcp_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_send);   

    std::this_thread::sleep_for(std::chrono::seconds(7)); // simulate more work

}


void do_accept(boost::asio::yield_context yield){
    // function that should be used as the coroutine

    /*
    * boost::asio::yield_context yield could be used instead of a handler (i.e. function pointer)
    *   --> The program goes on, where it is meant to be in the sequential manner that the code can be read. We just do not get anything back from the asynchronous, external ressource, since it is just sending the data to the client, this is not necessary in our case here!
    *   Also the program continues the execution on the position where the asynchronous function was called
    */

    for (int i = 0; i < 2; ++i){
        tcp_sockets.emplace_back(ioserv); // calls implicitly the constructor of the template class object, which is in this case a boost::asio::ip::tcp::socket
        // tcp_sockets.emplace_back(boost::asio::ip::tcp::socket{ioserv}); // equivalent to the line above
        tcp_acceptor.async_accept(tcp_sockets.back(), yield); // choose the lastly added element of the list from the emplace_back() call above 
        std::cout << "Answering client\n"; 
        boost::asio::spawn(ioserv, [](boost::asio::yield_context yield){ 
            std::cout << "Start sending data to the client...\n"; 
            do_write(tcp_sockets.back(), yield); 
            std::cout << "sending data to the client finished!\n"; 
        }); 

        std::cout << "Next client incoming!!!\n"; 
    }

    // the execution order while we are spawning new asynchronous tasks will be the same. There is virtually just one thread for executing the 
    // spaned function. This aims to make the code executing more like it was partly programmed sequentially, even if asynchronous functions are called!
}


int main(){

    tcp_acceptor.listen(); // non blocking - opening the socket for incoming connections from clients

    boost::asio::spawn(ioserv, do_accept); 
        // TODO: Make a tutorial on boost::coroutines

    std::cout << "Main execution goes on!\n"; 
    std::this_thread::sleep_for(std::chrono::seconds(1)); 

    ioserv.run(); 

    return 0; 
}

/*
* Compile command: 
*   $ g++ -pthread --std=c++17 time_server_with_coroutines.cpp -I/usr/local/include -L/usr/local/lib -lboost_coroutine -lboost_context -lboost_system
*/

// TODO: Look after platform depended asio programming: https://dieboostcppbibliotheken.de/boost.asio-plattformspezifische-objekte