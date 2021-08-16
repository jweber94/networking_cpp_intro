#ifndef NETCLIENT
#define NETCLIENT

#include "common_net_includes.hpp"
#include "connection_net_interface.hpp"
#include "net_message.hpp"
#include "net_ts_queue.hpp"

#include <string>

namespace custom_netlib {

template <typename T> class ClientBaseInterface {
  /*
   * Client interface class that is responsible for setting up the connection as
   * well as the asio I/O service object
   */

public:
  // elements of the rule of five
  ClientBaseInterface() : connection_sock_(ioserv_) {
    // associate the I/O service object with the socker, so it can send it I/O
    // objects for execution
    std::cout << "The client I/O context is now active.\n";
  }

  ~ClientBaseInterface() { this->Disconnect(); }

  // methods
  bool Connect(const std::string &host, const uint16_t port) {

    try {

      // use the system wide DNS server for resolving the IP-address with the given URL
      boost::asio::ip::tcp::resolver url_resolver(ioserv_); 
      boost::asio::ip::tcp::resolver::results_type endpts = url_resolver.resolve(host, std::to_string(port)); 
        // The endpoint of a URL is acutally used to send data between the client and the server and if the resolver could not resolve a given URL, an exception will be thrown and get catched by the try-catch block

      // Try to establish a connection
      connection_module_ = std::make_unique<ConnectionInterface<T>>(ConnectionInterface<T>::Owner::client, ioserv_, boost::asio::ip::tcp::socket(ioserv_), input_message_queue_); // create an connection interface instance
      connection_module_->ConnectToServer(endpts); // try to establish the connection with the connection instance 
      
      thr_io_serv_ = std::thread([this]() { ioserv_.run(); });
    } catch (const std::exception &e) {
      std::cerr << "Client error: " << e.what() << '\n';
      return false;
    }
    return true; 
  }

  void Disconnect() {

    // disconnect the connection module and stop the thread where the I/O
    // service object of the client is running in
    if (isConnected()) {
      connection_module_->Disconnect(); // -> since it is a (unique-)pointer
    }
    ioserv_.stop();

    if (thr_io_serv_.joinable()) {
      thr_io_serv_.join();
    }

    connection_module_
        .release(); // release the unique pointer (this is the "manually go out
                    // of scope" for a unique_ptr)
    std::cout << "Connection stoped!\n";
  }

  bool isConnected() {
    if (connection_module_) { // check if the connection_module_ exists
      // check if the connection instance is existand. If that is the case, a
      // connection is already established, since the connection object is
      // instanciated only after the connection to the server is already
      // established before
      return connection_module_->IsConnected();
    } else {
      return false;
    }
  }

  TsNetQueue<OwnedMessage<T>> &Incoming() {
    // Retrieve the received messages from the server
    return input_message_queue_;
  }

private:
  TsNetQueue<OwnedMessage<T>> input_message_queue_;

protected:
  boost::asio::io_context
      ioserv_; // io_service is the old version of io_context. Both are the
               // abstracted implementation of the I/O service object, whereas
               // io_service is the deprecated version
               // (https://stackoverflow.com/questions/59753391/boost-asio-io-service-vs-io-context)
  std::thread
      thr_io_serv_; // thread to execute the asio handlers with the io_context
  boost::asio::ip::tcp::socket
      connection_sock_; // hardware connection to the server that needs to be
                        // handed over to the connection object if the
                        // connection was sucessfully established, but before a
                        // connection is established, the functionallities of
                        // the ConnectionInterface class could not be used.
                        // Therefore the client object is responsable for this
  std::unique_ptr<ConnectionInterface<T>>
      connection_module_; // instance of the clients connection module of the
                          // architecture overview
  boost::asio::ip::tcp::endpoint end_pt_to_connect_;
};

} // namespace custom_netlib

#endif /* NETCLIENT */