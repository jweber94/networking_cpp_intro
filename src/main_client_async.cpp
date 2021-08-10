#include <iostream>
#include <string>
#include <thread>
#include <chrono>

#include <boost/asio/io_service.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/tcp.hpp>



std::vector<char> global_receive_storage(1*1024); // storage of 1 kilobyte 


void readout_handle(boost::system::error_code & ec, std::size_t length){
    if (!ec){
        std::cout << "\n\nRead " << length << " bytes\n\n"; 

        for (int i = 0; i < length; i++){
            std::cout << global_receive_storage[i]; 
        }
    }
}

void grapSomeData(boost::asio::ip::tcp::socket & socket){
    socket.async_read_some(boost::asio::buffer(global_receive_storage.data(), global_receive_storage.size()), 
        // here comes the read handle as a lambda function, so we can avoid to define the socked globally 
        [&](boost::system::error_code ec, std::size_t length){
            if (!ec){
                std::cout << "\n\nRead " << length << " bytes\n\n"; 
                
                for (int i = 0; i < length; i++){
                    std::cout << global_receive_storage[i];
                }
                grapSomeData(socket);
            }
        } // end of the lambda function (i.e. head handler)
    ); // end of async_read_some(buffer, read_handler)
}


int main(){
    /*
    * Asynchronous http request to a server using boost::asio
    */

    boost::asio::io_service ioserv; // the I/O service object lets the program end as soon as there are no I/O services (and/or associated I/O objects) registered in the I/O service object
    
    boost::asio::io_service::work idle_work(ioserv); // give the I/O service object some idle work (ref: CAUTION 2) 
    // starting a thread that runs the I/O service object to run in parallel to the main thread
    std::thread thr([&](){
        ioserv.run(); 
        // Explaination: https://stackoverflow.com/questions/15568100/confused-when-boostasioio-service-run-method-blocks-unblocks
    }); 
        // CAUTION: .run() blocks the thread that calls the .run() method of the I/O service object
        // In order to not block the main thread by the I/O service object, we run the I/O service object in its own thread
        // CAUTION2: As soon as the I/O service object has no I/O services registered with it, it ends its lifetime. So if we did not 
        // associate the idle work to the I/O service object, it would end (and can not awake again!) before the actual asynchronous 
        // async_read_some(...) function is invoced! 
    

    
    boost::system::error_code ec; 
    boost::asio::ip::address ip_address = boost::asio::ip::address::from_string("51.38.81.49", ec);
    // boost::asio::ip::tcp::endpoint tcp_endpt(boost::asio::ip::make_address("93.184.216.34", ec), 80);
    boost::asio::ip::tcp::endpoint tcp_endpt(ip_address, 80); // DeepL IP address  
    boost::asio::ip::tcp::socket sock(ioserv); 
    
    // connect to the endpoint
    sock.connect(tcp_endpt, ec); 
    if (!ec){ 
        std::cout << "Connected - continue programm\n"; 
    } else {
        std::cerr << "An error occured: " << ec.message() << "\n";
        sock.close(); 
        return 1; 
    }

    grapSomeData(sock); // listening to the socket for new data arrived before the request to the server was started 

    // sending a http request to the server
    std::string http_request = "GET /index.html HTTP/1.1\r\n"
                                "Host: example.com\r\n"
                                "Connection: close\r\n\r\n";
        // \r is a repositioning to the beginning of the current screen line
    if (sock.is_open()){   
        // check if the socket could connect to the defined endpoint --> we have an active and alive connection to the other end(-point) of the server
        sock.write_some(boost::asio::buffer(http_request.data(), http_request.size()), ec); // send as much data of the given data as possible
            // We need to use asio buffers in order to read and write with sockets. This is an interface for the asio framework
    }  

    // grapSomeData(sock); // wrong place, since we want to read something immediatly after we sended the request. 
    // The asynchrounous functions of boost::asio are immediatly returning after they were called (since they are asynchronously) 
    // and therefore, the main function will end before any data was arrived on the socked 
    
    std::this_thread::sleep_for(std::chrono::milliseconds(2000)); // wait a resonable amount of time at the end of the program, to receive the full data 
    // (in a real world scenario, this would be an infinite loop or something like this, so there would be no reason to wait) but we achived by incooperating
    // some fake idle_work, that we can receive the full data and do not block the main thread
    ioserv.stop(); // hard stop the I/O service object that is doing the idle_work after receiving the full website  
    
    // join the main thread to avoid an segfault at the end of the program
    if (thr.joinable()){
        thr.join(); 
    }

    std::cout << "Finished reading!\n";

    return 0; 
}