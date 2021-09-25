#include <iostream>
#include <boost/asio.hpp>

namespace io = boost::asio; 
namespace ip = io::ip; 

using tcp = ip::tcp; 

int main(){
	
	io::io_service ioserv; 
	std::cout << "Hello World\n"; 
	tcp::resolver resolver(ioserv); 

	return 0; 
}
