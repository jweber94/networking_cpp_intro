#ifndef NETSERVER
#define NETSERVER

#include "common_net_includes.hpp"
#include "connection_net_interface.hpp"
#include "net_message.hpp"
#include "net_ts_queue.hpp"

namespace custom_netlib {

template <typename T> // The template Type defines the message type
class ServerInterfaceClass {
public:
  // components of the rule of five
  ServerInterfaceClass(uint16_t port)
      : connection_acceptor_server_(
            ioserv_,
            boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)) {
    /* Initializing the listening socket before anything else happens with the
     * server by creating it with an initializer list This is where the implicit
     * socket for the listening to new connections is created as the acceptor
     * object of boost::asio
     */
  }

  virtual ~ServerInterfaceClass() { Stop(); }

  // methods
  bool Start() {
    try {
      waitForClientConnection(); // create the "work" for the asio I/O service
                                 // object such that it does not end before any
                                 // work comes in
      thr_io_serv_ = std::thread([this]() {
        ioserv_.run(); // start the I/O service object and let it listen to new
                       // connections, since the waitForClientConnection()
                       // method creates a I/O object/service that calls a
                       // handler as soon as a new connection request comes in
      });

    } catch (std::exception &ec) {
      std::cerr << "[SERVER] ERROR: " << ec.what() << "\n";
      return false;
    }

    std::cout << "[SERVER]: Server started.\n";
    return true;
  }

  bool Stop() {
    ioserv_.stop();

    if (thr_io_serv_.joinable()) {
      thr_io_serv_.join(); // wait/block the execution until the I/O service
                           // object has successfully stopped
    }

    std::cout << "[SERVER]: Server stopped.\n";
    return true;
  }

  // asynchronous methods
  void waitForClientConnection() {
    connection_acceptor_server_.async_accept([this](
                                                 boost::system::error_code ec,
                                                 boost::asio::ip::tcp::socket
                                                     sock) {
      if (!ec) {
        std::cout << "[SERVER]: A new connection was established: "
                  << sock.remote_endpoint() << "\n";

        // FIXME: Comment that in again
        /*
        std::shared_ptr<ConnectionInterface<T> > new_connection =
        std::make_shared<ConnectionInterface<T>
        >(ConnectionInterface<T>::owner::server, ioserv_, std::move(sock),
        inMsgQueue_);

        if (onClientConnect(new_connection)){
            // Deny the connection if the onClientConnect method delivers false
        --> A CustomServer class needs to override the onClientConnect method,
        since it delivers always false in its default implementation, so all
        connections get rejected (clean code principals) std::cout << "[SERVER]:
        Connection is valid - connection is active now!\n";
            connectionsQueue_.emplace_back(std::move(new_connection)); // add
        the connection object to the connections queue so there will be a
        shared_ptr after the execution of the lambda function and so, the
        connection object will not get deleted if the lambda function goes out
        of scope

            // assigning the identifyer
            connectionsQueue_.back()->ConnectToClient(idCounter_++);
            std::cout << "[CLIENT " << connectionsQueue_.back()->getID() << "]
        connection approved\n";

        } else {
            std::cout << "[SERVER]: The server denied the connection!\n";
        }*/

      } else {
        // if there is an error code, we want to display it
        std::cerr << "[SERVER]: ERROR with establishing a new connection: "
                  << ec.message() << "\n";
      }
      waitForClientConnection(); // calling the wait for new connection method
                                 // again --> A new asynchronous task will be
                                 // registered by the I/O service object
    });
    // Explaination: Every time when the asynchronous, out of process event is
    // finished/executed, the given handler will be executed JUST ONE SINGLE
    // TIME! The Given lambda function is the implementation of a boost::asio
    // acceptor handler
    // (https://www.boost.org/doc/libs/1_66_0/doc/html/boost_asio/reference/AcceptHandler.html)
  }

  void
  sendMessageToClient(std::shared_ptr<ConnectionInterface<T>> client_connection,
                      const message<T> &msg_to_send) {
    if (client_connection && client_connection->IsConnected()) {
      // check if the client object is valid and if the connection to the data
      // exchange socket is existand. If that is given, we can send the message
      // throu the socket
      client_connection->SendData(msg_to_send);
    } else {
      // we have identified, that the client is not existant in out network
      // anymore
      onClientDisconnect(
          client_connection); // since we could not connect to the client, we
                              // want to do something with that information
                              // (e.g. inform other clients that the client is
                              // not accessable anymore)
      client_connection.reset();

      // delelte the client from the connections queue
      connectionsQueue_.erase(std::remove(connectionsQueue_.begin(),
                                          connectionsQueue_.end(),
                                          client_connection),
                              connectionsQueue_.end());
    }
  }

  void sendMessageToAllClients(
      const message<T> &msg_to_send,
      std::shared_ptr<ConnectionInterface<T>> ignore_client) {
    // send a message to all connected clients except of one client (i.e. if one
    // client does something, that should be messaged to all other client except
    // for itself)

    bool invalidClientExistFlag = false;

    for (auto &connIt : connectionsQueue_) {
      if (connIt && (connIt->IsConnected()) && (connIt != ignore_client)) {
        connIt->Send(msg_to_send);
      } else {
        onClientDisconnect(
            connIt); // since we could not connect to the client, we want to do
                     // something with that information (e.g. inform other
                     // clients that the client is not accessable anymore)
        connIt.reset();
        invalidClientExistFlag = true;
      }
    }

    if (invalidClientExistFlag) {
      // delete the connections that were not accessable for the message
      // sending. We do not want to delete an iterator while looping throu the
      // queue, since this would invalidate the iterator during runtime - this
      // results in undefined behaviour
      connectionsQueue_.erase(std::remove(connectionsQueue_.begin(),
                                          connectionsQueue_.end(), nullptr),
                              connectionsQueue_.end());
    }
  }

  void update(size_t numMaxMessages = -1) {
    // Method to explicitly process messages in the server logic. The parameter
    // numMaxMessages defines the maximum number of messages that will be
    // processed out of the incoming message queue on one call of this method.
    // This is important because if there are always new messages from the
    // clients, the update function will never return and therefor, the server
    // logic will not continue with its work. CAUTION: setting size_t (which is
    // a unsigned int in the background) to -1 result in the maximum number that
    // size_t could handle (ref: integer overflow -
    // https://en.wikipedia.org/wiki/Integer_overflow)

    size_t msg_count = 0;
    while (msg_count < numMaxMessages && !inMsgQueue_.isEmpty()) {
      auto msg = inMsgQueue_.popFront(); // extract the first element of the
                                         // incoming (thread save) message queue
      onMessage(msg.remote, msg.msg);
      msg_count++;
    }
  }

private:
protected:
  virtual bool onClientConnect(std::shared_ptr<ConnectionInterface<T>> client) {
    /*
     * This method will get called, if a new client connects to the server.
     * If you return false, the connection will be rejected
     *
     * Example: Check in this function for banned IP-Addresses or the maximum
     * number of connected clients
     *
     * Needs to be overwritten by the CustomServer class (that inherits from
     * this class)
     */
    return false;
  }

  virtual void
  onClientDisconnect(std::shared_ptr<ConnectionInterface<T>> client) {
    /*
     * Gets called if a client gets disconnected.
     *
     * Example: Remove the client from the list of clients that is shown
     * (messaged) to all other clients
     */
  }

  virtual void onMessage(std::shared_ptr<ConnectionInterface<T>> client,
                         message<T> &msg_input) {
    /*
     * Gets called if a messages was received by the ConnectionInterface (that
     * uses the I/O service object of the server)
     *
     * This method defines, how to deal with the message if it is already
     * arrived and deserialized
     */
  }

  // members
  TsNetQueue<OwnedMessage<T>> inMsgQueue_;
  std::deque<std::shared_ptr<ConnectionInterface<T>>> connectionsQueue_;
  // CAUTION: The order of the initialization is given by the order of the
  // members in the code. The ioserv needs to be initalized BEFORE the thread is
  // initialized where is should run
  boost::asio::io_context ioserv_;
  std::thread thr_io_serv_;
  boost::asio::ip::tcp::acceptor connection_acceptor_server_;
  uint32_t
      idCounter_; // working with identify counters is much easyer then working
                  // with socket-addresses and even more secure (since we do not
                  // need to send socket information over the network)
};

} // namespace custom_netlib

#endif /* NETSERVER */