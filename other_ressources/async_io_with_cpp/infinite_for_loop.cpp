#include <iostream>
#include <thread>
#include <chrono>

int main(){

	for (;;){
		std::cout << "For loop executed\n"; 
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}

	std::cout << "Finished program\n"; 

	return 0; 
}
