#ifndef CONNECTIONNET
#define CONNECTIONNET

#include "common_net_includes.hpp"
#include "net_message.hpp"
#include "net_ts_queue.hpp"

namespace custom_netlib {

template <typename T>
class ConnectionInterface
    : public std::enable_shared_from_this<ConnectionInterface<T>>
// : public std::enable_shared_from_this<ConnectionInterface<T>> enables to
// create a shared pointer of this class (ConnectionInterface) from within the
// class itself instead of a raw pointer (by returning 'this')
{
public:
  enum class Owner {
    // only defined within the class (ref.: scoping)
    server,
    client
  };

  // parts of the rule of five
  ConnectionInterface(Owner parent, boost::asio::io_context &ioservobj,
                      boost::asio::ip::tcp::socket sock,
                      TsNetQueue<OwnedMessage<T>> &input_queue)
      : io_service_object_(ioservobj), socket_connection_(std::move(sock)),
        in_msg_queue_(input_queue) {
    std::cout << "Connection element created!\n";
    owner_ = parent;
  }

  virtual ~ConnectionInterface() {
    std::cout << "Destructor of the connection interface was called!\n";
  }

  // methods
  bool
  ConnectToServer(const boost::asio::ip::tcp::resolver::results_type &endpts) {
    // Only necessary if the associated owner is a client
    if (owner_ == Owner::client) {
      boost::asio::async_connect(
          socket_connection_, endpts,
          [this](boost::system::error_code ec,
                 boost::asio::ip::tcp::endpoint endpt) {
            if (!ec) {
              ReadHeader(); // if the asynchronous task, that was registered by
                            // the I/O context ioserv_, of connection to the
                            // server is finished, the connection handler will
                            // be executed within the thread where the ioserv_
                            // is running in. Therefore, the ReadHeader() method
                            // is used to prime the I/O service object with
                            // another asynchronous task that should listen for
                            // and read out messages from the server
            }
          }); // boost::asio connect handler
              // (https://www.boost.org/doc/libs/1_71_0/doc/html/boost_asio/reference/ConnectHandler.html)
      std::cout << "[CLIENT]: Connect to server.\n";
    }
    return true;
  }

  bool ConnectToClient(uint32_t u_id = 0) {
    // Only necessary if the associated Owner is a server
    if (owner_ == Owner::server) {
      std::cout << "[SERVER]: Connect to client.\n";
      if (socket_connection_.is_open()) {
        id_ = u_id;
        ReadHeader(); // register the I/O object / I/O service within the I/O
                      // service object of boost::asio --> We want our server to
                      // read messages as soon as they appear on the socket
      }
    }
    return true;
  }

  uint32_t getID() const { return id_; }

  bool Disconnect() {
    if (IsConnected()) {
      boost::asio::post(io_service_object_,
                        [this]() { socket_connection_.close(); });
    }
    return true;
  }

  bool IsConnected() const {
    // Returns true, if the data exchange socket is active/connected
    return socket_connection_.is_open();
  }

  bool SendData(const message<T> &msg_to_send) {
    boost::asio::post(io_service_object_, [this, msg_to_send]() {
      bool writingMsgFlag =
          !out_msg_queue_.isEmpty(); // if the out queue is not empty, we
                                     // assume, that boost::asio is currently
                                     // sending them throu the socket.

      out_msg_queue_.pushBack(msg_to_send);
      if (!writingMsgFlag) { // we do not want to add another WriteHeader
                             // workload to the boost::asio I/O service object,
                             // since it is already within the scheduling. We
                             // only want to prime the I/O service object with
                             // new sending work, if there are currently no
                             // messages to send and then the newly added
                             // message will be sent
        WriteHeader(); // WriteHeader has an intrinsic loop that writes messages
                       // until the out_msg_queue_ is empty!
      }
    });
    return true;
  }

  void AddToIncomingMsgQueue() {
    if (owner_ == Owner::server) {
      in_msg_queue_.pushBack(
          {this->shared_from_this(),
           tmp_input_msg_}); // Transform the incoming message to an owned
                             // message if the owner of the connection interface
                             // instance is a server implicitly calls the
                             // initializer list
    } else {
      // The owner is a client
      in_msg_queue_.pushBack({nullptr, tmp_input_msg_});
      // no owner pointer required, since clients only have one communication
      // partner
    }

    ReadHeader(); // prime the asynchronous asio I/O service object to read
                  // another header --> register a new I/O service / I/O object
                  // in the io_service object
  }

  // Async asio tasks
  void ReadHeader() {
    /*
     * We want to prime the I/O service object with the ReadHeader() method, in
     * order to let the server/client listen for new messages (but we do not
     * know WHEN the message will be available)
     */
    boost::asio::async_read(
        socket_connection_,
        boost::asio::buffer(&tmp_input_msg_.header, sizeof(message_header<T>)),
        [this](boost::system::error_code ec, std::size_t length) {
          if (!ec) {
            if (tmp_input_msg_.header.size >
                0) { // We do not have always a payload attached to our headers,
                     // so we need to check if there is a payload (which is
                     // indicated by a saved size in the header, that is greater
                     // then 0)
              tmp_input_msg_.payload.resize(
                  tmp_input_msg_.header
                      .size); // resize the storage for the payload where the
                              // received data from the socket should be stored
              ReadPayload();
            } else {
              // if we do not have a payload attached, the read out of the
              // socket is already complete and we will add the readed header to
              // our message (thread save) queue
              AddToIncomingMsgQueue(); // this method accesses the
                                       // tmp_input_msg_ member variable to add
                                       // the readed data to the queue
            }

          } else {
            std::cout << "[" << id_ << "]: reading the header failed!\n";
            socket_connection_
                .close(); // closing the connection will be detected by the
                          // MessageClient or MessageAllClients function of the
                          // server base class which will result in deleting the
                          // connection object in the future
          }
        }); // Boost read handler
  }

  void ReadPayload() {
    boost::asio::async_read(
        socket_connection_,
        boost::asio::buffer(tmp_input_msg_.payload.data(),
                            tmp_input_msg_.payload.size()),
        [this](boost::system::error_code ec, std::size_t length) {
          // Another read handler
          if (!ec) {
            AddToIncomingMsgQueue(); // accesses implicitly the tmp_input_msg
                                     // and adds its content to the thread save
                                     // incoming message queue
          } else {
            std::cout << "[" << id_ << "]: reading the payload failed!\n";
            socket_connection_.close();
          }
        });
  }

  void WriteHeader() {
    boost::asio::async_write(
        socket_connection_,
        boost::asio::buffer(&out_msg_queue_.front().header,
                            sizeof(message_header<T>)),
        [this](boost::system::error_code ec, std::size_t length) {
          // boost::asio write handler
          if (!ec) {

            if (out_msg_queue_.front().payload.size() > 0) {
              // The message that we want to asynchronously send has a payload
              // attached
              writePayload();
            } else {
              // The message that we want to asynchronously send has NO payload
              // attached
              out_msg_queue_.popFront();

              // check if there are more messages to send or if the queue of
              // messages that should get sent is empty
              if (!out_msg_queue_.isEmpty()) {
                WriteHeader();
              }
            }

          } else {
            // The connection has failed
            std::cout << "[" << id_ << "]: sending the header failed!\n";
            socket_connection_.close();
          }
        });
  }

  void writePayload() {
    boost::asio::async_write(
        socket_connection_,
        boost::asio::buffer(out_msg_queue_.front().payload.data(),
                            out_msg_queue_.front().payload.size()),
        [this](boost::system::error_code ec, std::size_t length) {
          // write handler
          if (!ec) {
            out_msg_queue_.popFront();

            if (!out_msg_queue_.isEmpty()) {
              // if there are more messages to send in the queue, prime the
              // boost::asio I/O service object (io_service) with another
              // asynchronous task (i.e. I/O service / I/O object)
              WriteHeader();
            }

          } else {
            std::cout << "[" << id_ << "]: Writing the payload failed.\n";
            socket_connection_.close();
          }
        });
  }

protected:
  boost::asio::ip::tcp::socket socket_connection_;
  boost::asio::io_context
      &io_service_object_; // we want to have the I/O service object owned by
                           // the client or the server itself, so there will
                           // just one I/O service object that manages all
                           // networking stuff, but especially the server can
                           // have multiple connection elements
  TsNetQueue<message<T>> out_msg_queue_; // messages that should be send throu
                                         // the socket connection
  TsNetQueue<OwnedMessage<T>> &
      in_msg_queue_; // messages that are received from the communication
                     // partner of the network connection. Therefore, we need to
                     // have the messages associated with the owner (i.e. the
                     // communication partner on the other side) of the message
  Owner owner_ = Owner::server;
  uint32_t id_ = 0; // store the identifyer of the client associated with the
                    // connection object
  message<T> tmp_input_msg_;
};

} // namespace custom_netlib

#endif /* CONNECTIONNET */