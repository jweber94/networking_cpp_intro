#include "server_logic.hpp"
#include "networking_logic.hpp"

#include <iostream>
#include <boost/program_options.hpp>
#include <boost/asio.hpp>

int main(int argc, char * argv [])
{
    std::cout << "Started REST server.\n"; 

    NetworkLogic network_instance; 


    std::cout << "Finished program\n";
    return 0; 
}