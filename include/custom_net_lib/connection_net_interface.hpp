#ifndef CONNECTIONNET
#define CONNECTIONNET

#include "common_net_includes.hpp"
#include "net_message.hpp"
#include "net_ts_queue.hpp"

namespace custom_netlib {

    template <typename T>
    class ConnectionInterface : public std::enable_shared_from_this<ConnectionInterface<T>>
        // : public std::enable_shared_from_this<ConnectionInterface<T>> enables to create a shared pointer of this class (ConnectionInterface) from within the class itself instead of a raw pointer (by returning 'this')
    {
        public: 
            // parts of the rule of five
            ConnectionInterface(){
                std::cout << "Connection element created!\n"; 
            };

            virtual ~ConnectionInterface(){

            }

            // methods
            bool ConnectToServer(); 
            bool ConnectToClient(); 
            uint16_t getID(); 
            bool Disconnect(); 
            bool IsConnected() const; // returns true, if the data exchange socket is active/connected

            bool SendData(const message<T> & msg_to_send); 

        protected: 
            boost::asio::ip::tcp::socket socket_connection_; 
            boost::asio::io_context & io_service_object_; // we want to have the I/O service object owned by the client or the server itself, so there will just one I/O service object that manages all networking stuff, but especially the server can have multiple connection elements
            TsNetQueue<message<T> > out_msg_queue_; // messages that should be send throu the socket connection
            TsNetQueue<OwnedMessage<T> > in_msg_queue_; // messages that are received from the communication partner of the network connection. Therefore, we need to have the messages associated with the owner (i.e. the communication partner on the other side) of the message
    };

} /* custom_netlib */

#endif /* CONNECTIONNET */