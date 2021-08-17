#ifndef NETMESSAGE
#define NETMESSAGE

#include "common_net_includes.hpp"
//#include "connection_net_interface.hpp" // We can not do this, since this
// would be a cycle dependency - we solve this by commenting this line out and
// then forward-declare the ConnectionInterface for the OwnedMessage struct
//(ref.:
// https://stackoverflow.com/questions/2133250/x-does-not-name-a-type-error-in-c)

namespace custom_netlib {

/*
 * The template class T is meant to be an "enum class" to make your own custom
 * IDs and to let the compiler check for valid IDs in your code while using the
 * message framework
 */

// forward declaration for the header datatype, since it is used in the complete
// message data structure
template <typename T> struct message_header {
  T id{};
  uint32_t size = 0; // do NOT use size_t, since size_t is platform dependent,
                     // but we want an platform independent message protocol
}; // Remark: In structs, everything is public unless it is defined differend
   // (in opposite of classes, where everything is private by default)

/*
 * Usage for storing data within the payload of a message instance:
 *   msg_instance << payload_data_1 << payload_data_2 ;
 *
 * Usage for reading out the payload data from a message instance
 *   msg_instance >> aimed_datatype_2 >> aimed_datatype_1;
 */

// actual message datastructure
template <typename T> struct message {
  // data
  message_header<T> header{};
  std::vector<uint32_t>
      payload; // the message has no notion of the datatype in the payload,
               // since it is a serialized form of the data that should be send.
               // Therefore, for every serializabel datatype, uint32_t can be
               // used

  // methods
  size_t size() const {
    // making it const in order to make clear, that the size request should not
    // edit the data in the message
    return payload.size(); // sizeof(message_header<T>) + payload.size(); //
                           // std::vector has a method called size, which
                           // returns the number of bytes of the vector element
  }

  // external access
  // interaction with std::ostream
  friend std::ostream &operator<<(std::ostream &ostream,
                                  const message<T> &msg) {
    ostream << "ID: " << (int)msg.header.id << " Size: " << msg.header.size;
    return ostream;
  }

  // pushing data to the payload of the message in serialized form
  template <typename PayloadType> // the template datatype PayloadType is used
                                  // to make the saving of the payload
                                  // applicable to every serializable datatype,
                                  // so PayloadType is the variable for a
                                  // seriablizable datatype
  friend message<T> &operator<<(message<T> &msg, const PayloadType &data) {
    // makeing sure, that the given data is serializable
    static_assert(std::is_standard_layout<PayloadType>::value,
                  "Data is not serializable in a common way. Could not save "
                  "the data in the payload!");

    size_t current_size =
        msg.payload.size(); // cache current size of the payload

    msg.payload.resize(current_size + sizeof(PayloadType));

    memcpy(
        msg.payload.data() + current_size, &data,
        sizeof(
            PayloadType)); // copy the data in the newly allocated vector space
                           // std::memcopy(destination_ptr, source_ptr,
                           // number_of_bytes_to_copy)
                           // https://www.cplusplus.com/reference/cstring/memcpy/
                           //      Copies the given number of bytes to copy
                           //      directly to the destination memory location
                           //      and therefore, it is implicitly serialized to
                           //      uint8_t
                           // explaination of msg.payload.data() + current_size:
                           //  msg.payload.data() gives us a pointer to the
                           //  address of the first vector element back. Then we
                           //  need to add the cached size in elements of the
                           //  payload to get the pointer on which we want to
                           //  add the new data and then copy the number of byes
                           //  from the source data to the vector

    msg.header.size =
        msg.size(); // update the size of the message in the header

    return msg; // by returning the message reference, we make chaining data
                // together possible, e.g. msg_instance << data_1 << data_2;
  }

  // read out payload of the message
  template <typename PayloadType>
  friend message<T> &operator>>(message<T> &msg, PayloadType &data) {
    // extract the data from the last element to the first element
    static_assert(std::is_standard_layout<PayloadType>::value,
                  "Data is not serializable in a common way. Could not save "
                  "the data in the payload!");

    // calculate the beginning of the last element, given by the datatype that
    // the serialized data should be casted to
    size_t start_idx_last_element = msg.payload.size() - sizeof(PayloadType);

    // copy the data in byte form to the given aimed data
    memcpy(&data, msg.payload.data() + start_idx_last_element,
           sizeof(PayloadType));
    // This is the same principal as before: by copying directly in form of
    // chars in the memory blocks, the data is implicitly deserialized

    // resize the vector after copying the extracted data
    msg.payload.resize(start_idx_last_element);

    // save the new size of the vector in the header and return the reference to
    // the message
    msg.header.size = msg.size();
    return msg;
  }
};

// forward declaration
template <typename T> class ConnectionInterface;

template <typename T> struct OwnedMessage {
  std::shared_ptr<ConnectionInterface<T>> remote =
      nullptr; // create a new message type that contains the message as well as
               // the connection interface object that the message was or will
               // be send by doing this, every message on the server can be
               // associated with the client that was sending it the request
               // message and so it can send an answer message to the correct
               // client, since every connection corresponds to a client on the
               // server implementation. On the client side, this information is
               // not necessary needed.
  message<T> msg;

  friend std::ostream &operator<<(std::ostream &ostr,
                                  const OwnedMessage<T> &msg) {
    ostr << msg.msg;
    return ostr;
  }
};

} // namespace custom_netlib

#endif /* NETMESSAGE */