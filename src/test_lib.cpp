#include <iostream>
#include "net_message.hpp"
#include "connection_net_interface.hpp"
#include "net_client.hpp"
#include "net_ts_queue.hpp"

enum class CustomTypes : uint32_t {
    Car, 
    Pedestrian
}; 


class CustomClient : public custom_netlib::ClientBaseInterface<CustomTypes>
    // CAUTION: Here we USE the interface for giving the template of the client a valid ID
{
    public: 
        bool aimOfTheCar(float && x, float && y){
            custom_netlib::message<CustomTypes> request_msg; 
            request_msg.header.id = CustomTypes::Car; // The compiler makes sure, that we just can input valid IDs by using the enum class as the template argument, so we can just use valid elements of the given template argument which is in this case only the two elements of CustomTypes
            // write the payload
            request_msg << x << y; 

            Send(request_msg); 
        }
}; 


int main(){

    
    CustomClient client_instance;
    client_instance.Connect("127.0.0.1", 8000);
    client_instance.aimOfTheCar(5.5, 6.6);   
    


    std::cout << "Hello World!\n"; 

    return 0; 
}