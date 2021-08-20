#include "custom_net_lib.hpp"
#include <iostream>
#include <boost/program_options.hpp>

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
    custom_netlib::message<CustomMsgTypes> msg;
    msg.header.id = CustomMsgTypes::ServerAccept;
    client_ptr->SendData(msg);
    return true; // accept all requested client connections
  }

  virtual void onClientDisconnect(
      std::shared_ptr<custom_netlib::ConnectionInterface<CustomMsgTypes>>
          client_ptr) {
    std::cout << "Removing client [" << client_ptr->getID() << "]\n";
  }

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

    case CustomMsgTypes::MessageAll: {
      std::cout << "[" << client->getID() << "]: Message all\n";
      custom_netlib::message<CustomMsgTypes> msg;
      msg.header.id = CustomMsgTypes::MessageAll;
      msg << client->getID();
      sendMessageToAllClients(msg, client); // just return the header
      break;
    }

    default: { break; }
    }
  }
};

int main(int argc, const char *argv[]) {

  // command line parser
  uint32_t port_num = 0;
  boost::program_options::options_description desc{
      "Options for the chatbot server."};
  boost::program_options::variables_map vm;

  desc.add_options()(
        "help,h", "Help and overview of all possible command line options")(
        "port,p",
        boost::program_options::value<uint32_t>()->default_value(60000),
        "Port where the server should listen for new connections from clients.");

    boost::program_options::store(
        boost::program_options::parse_command_line(argc, argv, desc), vm);
    boost::program_options::notify(vm);

    port_num = vm["port"].as<uint32_t>();

  //CustomServerLogic server_test(60000);
  CustomServerLogic server_test(port_num); 
  server_test.Start();

  custom_netlib::message<CustomMsgTypes> msg_test;

  std::cout << "Server was successfully created and already started!\n";

  while (true) {
    server_test.update(-1, true);
  }

  return 0;
}