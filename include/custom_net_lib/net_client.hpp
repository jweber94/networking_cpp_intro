#ifndef NETCLIENT
#define NETCLIENT

#include "net_ts_queue.hpp"
#include "net_message.hpp"
#include "connection_net_interface.hpp"
#include "common_net_includes.hpp"

namespace custom_netlib {

    template <typename T>
    class ClientBaseInterface
    {
        /*
        * Client interface class that is responsible for setting up the connection as well as the asio I/O service object
        */

        public:
            // elements of the rule of five
            ClientBaseInterface() : connection_sock_(ioserv_){
                // associate the I/O service object with the socker, so it can send it I/O objects for execution
                std::cout << "The client I/O context is now active.\n"; 
            }

            ~ClientBaseInterface(){
                this->Disconnect(); 
            }

            // methods
            bool Connect(const std::string & host, const uint16_t port){

                try
                {
                    // Try to establish a connection
                    connection_module_ = std::make_unique<ConnectionInterface<T> >(); 
                    // TODO

                    // the asio resolver can take a URL with a separated port and resolve it to an endpoint
                    boost::asio::ip::tcp::resolver url_res(ioserv_); 
                    end_pt_to_connect_ = url_res.resolve(host, std::to_string(port)); // if we ask the resolver for an invalid URL, then it will throw an exception

                    thr_io_serv_ = std::thread([this](){
                        ioserv_.run(); 
                    }); 
                }
                catch(const std::exception& e)
                {
                    std::cerr << "Client error: " << e.what() << '\n';
                    return false;  
                }                
            }

            void Disconnect(){
                
                // disconnect the connection module and stop the thread where the I/O service object of the client is running in
                if(isConnected()){
                    connection_module_->Disconnect(); // -> since it is a (unique-)pointer 
                }
                ioserv_.stop(); 

                if (thr_io_serv_.joinable()){
                    thr_io_serv_.join(); 
                }

                connection_module_.release(); // release the unique pointer (this is the "manually go out of scope" for a unique_ptr)
                std::cout << "Connection stoped!\n"; 
            }

            bool isConnected(){
                if (connection_module_){ // check if the connection_module_ exists
                    // check if the connection instance is existand. If that is the case, a connection is already established, since the connection object is instanciated only after the connection to the server is already established before
                    return connection_module_->IsConnected(); 
                } else {
                    return false;
                }
            }

            TsNetQueue<OwnedMessage<T> > & Incoming(){
                // Retrieve the received messages from the server 
                return input_message_queue_; 
            } 
        
        private: 
            TsNetQueue<OwnedMessage<T> > input_message_queue_; 

        protected: 
            boost::asio::io_context ioserv_; // io_service is the old version of io_context. Both are the abstracted implementation of the I/O service object, whereas io_service is the deprecated version (https://stackoverflow.com/questions/59753391/boost-asio-io-service-vs-io-context) 
            std::thread thr_io_serv_;  // thread to execute the asio handlers with the io_context
            boost::asio::ip::tcp::socket connection_sock_; // hardware connection to the server that needs to be handed over to the connection object if the connection was sucessfully established, but before a connection is established, the functionallities of the ConnectionInterface class could not be used. Therefore the client object is responsable for this 
            std::unique_ptr<ConnectionInterface<T> > connection_module_; // instance of the clients connection module of the architecture overview
            boost::asio::ip::tcp::endpoint end_pt_to_connect_; 
    };


} /* custom_netlib */

#endif /* NETCLIENT */