#include <iostream>
#include <boost/asio.hpp>
#include <string>
#include <array>
#include <vector>

namespace io = boost::asio; 
namespace ip = io::ip; 

using tcp = ip::tcp; 
using error_code_type = boost::system::error_code;

template <typename... Args>
auto const_buffer_sequence(Args&&... args)
{
    return std::array<io::const_buffer, sizeof...(Args)>{io::buffer(args)...};
}

template <typename... Args>
auto buffer_sequence(Args&&... args)
{
    return std::array<io::mutable_buffer, sizeof...(Args)>{io::buffer(args)...};
}

int main(){
    std::cout << "Program started.\n"; 

    // creating some disturbed data
    std::string data_1 = "Hello World\n"; 
    uint32_t size_1 = data_1.size(); 
    std::string * data_2 = new std::string("This is the second data.\n"); 
    uint32_t size_2 = data_2->size();  

    // creating the buffer sequence
    auto buffer_sequence_view = const_buffer_sequence(data_1, *data_2, io::buffer(&size_1, sizeof(size_1)), io::buffer(&size_2, sizeof(size_2))); //, data_2, size_2 
        // boost::asio::buffer(address_of_memory, size_of_memory) but there is an overload for std::string, so you can hand over a std::string to a boost::asio::buffer(std::string input_str) and the size will be called implicitly 

    // accessing the indivudal buffer elements within the buffer sequence
    auto begin = boost::asio::buffer_sequence_begin(buffer_sequence_view);
    auto end = boost::asio::buffer_sequence_end(buffer_sequence_view);  
    std::size_t it_counter = 0; 
    for (auto iterator = begin; iterator != end; ++iterator){ 
        std::cout << "The " << it_counter << " is: " << iterator->data() << " with the size: " << iterator->size()<< "\n";
        it_counter++; 
    }

    // accessing the buffer sequence byte by byte
    auto begin_bs = boost::asio::buffers_begin(buffer_sequence_view); 
    auto end_bs = boost::asio::buffers_end(buffer_sequence_view); 
    std::size_t byte_counter = 0;
    for (auto iter_byte = begin_bs; iter_byte != end_bs; ++iter_byte){
        std::cout << "The " << byte_counter << " is: " << *iter_byte << "\n";
        byte_counter++;  
    } 

    char * receive_memory = new char [4];
    char receive_memory_2; 
    auto buffer_sequ_receive = buffer_sequence(io::buffer(receive_memory, sizeof(receive_memory)), boost::asio::buffer(&receive_memory_2, sizeof(receive_memory_2))); 
        // buffer sequence has 4*1 + 1 bytes = 4*4+1*4 = 20 bit
    std::cout << "[DEBUG]: The size is: " << sizeof(receive_memory) << std::endl; 

    // copying 
    //boost::asio::buffer_copy(buffer_sequ_receive, buffer_sequence_view); // Copies as much as the target buffer instance is available to store
    boost::asio::buffer_copy(buffer_sequ_receive, buffer_sequence_view, 3); // Copies just 3 bytes from the beginning of the source buffer sequence to the beginning of the target buffer sequence

    auto begin_test = boost::asio::buffers_begin(buffer_sequ_receive);
    auto end_test = boost::asio::buffers_end(buffer_sequ_receive);
    std::cout << "This is the copy test:\n"; 
    for (auto it = begin_test; it != end_test; ++it){
        std::cout << *it << "\n"; 
    }

    std::cout << "Program finished.\n";
    return 0;  
}

/*
* Further examples: https://cpp.hotexamples.com/de/examples/-/ConstBufferSequence/-/cpp-constbuffersequence-class-examples.html
*/

/*
* Compile Command: 
*   $ g++ working_with_buffer_sequences.cpp -pthread --std=c++17 -I/usr/local/include -L/usr/local/lib -lboost_coroutine -lboost_context -lboost_system
*/