#include "custom_net_lib.hpp"
#include <iostream>

enum class CustomMsgTypes : uint32_t {
  ServerAccept,
  ServerDeny,
  ServerPing,
  MessageAll,
  ServerMessage
};

class CustomClient : public custom_netlib::ClientBaseInterface<CustomMsgTypes>
{
    public: 

    private: 

};

int main(){

    CustomClient client; 
    client.Connect("127.0.0.1", 60000); 

    std::cout << "Finished client program.\n"; 

    return 0; 
}