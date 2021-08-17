#include "custom_net_lib.hpp"
#include <iostream>

enum class CustomMsgTypes : uint32_t {
  ServerAccept,
  ServerDeny,
  ServerPing,
  MessageAll,
  ServerMessage
};

class CustomServerLogic
    : public custom_netlib::ServerInterfaceClass<CustomMsgTypes> {
public:
  CustomServerLogic(uint16_t n_port)
      : custom_netlib::ServerInterfaceClass<CustomMsgTypes>(n_port) {
    // Initializer list constructs the server interface with the given port
    // number
  }

protected:
  // Implementations for the virtual methods of the server interface base class
  virtual bool onClientConnect(
      std::shared_ptr<custom_netlib::ConnectionInterface<CustomMsgTypes>>
          client_ptr) {
    return true; // accept all requested client connections
  }

  virtual void onClientDisconnect(
      std::shared_ptr<custom_netlib::ConnectionInterface<CustomMsgTypes>>
          client_ptr) {}

  virtual void
  onMessage(std::shared_ptr<custom_netlib::ConnectionInterface<CustomMsgTypes>>
                client,
            custom_netlib::message<CustomMsgTypes> &msg_input) {
    switch (msg_input.header.id) {
    case CustomMsgTypes::ServerPing: {
      std::cout << "[" << client->getID() << "]: Server ping\n";
      client->SendData(msg_input);
      break;
    }

    case CustomMsgTypes::ServerDeny: {
      break;
    }

    default: { break; }
    }
  }
};

int main() {

  CustomServerLogic server_test(60000);
  server_test.Start();

  custom_netlib::message<CustomMsgTypes> msg_test;

  std::cout << "Server was successfully created and already started!\n";

  while (true) {
    server_test.update();
  }

  return 0;
}