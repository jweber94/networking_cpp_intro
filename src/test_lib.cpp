#include <iostream>
#include "custom_net_lib.hpp"

enum class CustomMsgTypes : uint32_t {
    ServerAccept, 
    ServerDeny, 
    ServerPing, 
    MessageAll, 
    ServerMessage
}; 

class CustomServerLogic : public custom_netlib::ServerInterfaceClass<CustomMsgTypes>
{
    public: 

    private:

}; 


int main(){

    // TODO: Test the server code here --> https://www.youtube.com/watch?v=UbjxGvrDrbw (22:13 min)
    std::cout << "Hello World!\n"; 

    return 0; 
}