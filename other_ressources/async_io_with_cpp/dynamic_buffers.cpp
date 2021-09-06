#include <iostream>
#include <string>
#include <vector>
#include <boost/asio.hpp>
#include <algorithm> // std::min

int main(){

    // create a vector and a string buffer instance
        // create the data storage
    std::string string_data; 
    std::vector<int> int_vec_data; 

        // create the buffer instances and their view
    auto str_buf_dyn = boost::asio::dynamic_buffer(string_data); 
    auto vec_buf_dyn = boost::asio::dynamic_buffer(int_vec_data); 

    vec_buf_dyn.prepare(10); // Makes only a difference in the output sequence, .size() shows the size of the input sequence
    std::cout << "This is the size before filling it with data: " << vec_buf_dyn.size() << "\n"; 

    // insert data to the output sequence
    int_vec_data.push_back(1); 
    int_vec_data.push_back(2);
    int_vec_data.push_back(3);

    std::cout << "This is the size after filling it with data: " << vec_buf_dyn.size() << "\n";

    // convert the output sequence to the input sequence
    vec_buf_dyn.commit(3); 

    std::cout << "This is the size after commiting the data from the output sequence: " << vec_buf_dyn.size() << "\n";  

    auto data = vec_buf_dyn.data(); // .data() returns a const buffer sequence from which we can iterate 
    
    auto begin = boost::asio::buffers_begin(data); 
    auto end = boost::asio::buffers_end(data);
    for (auto it = begin; it != end; ++it){
        std::cout << *it << "\n"; // FIXME: Why does this not work?
    }
    
    vec_buf_dyn.consume(3); 
    std::cout << "Size after consuming: " << vec_buf_dyn.size() << "\n";
    

    std::cout << "Finished program\n"; 
    return 0; 
}

/*
* Compile Command:
*   $ g++ dynamic_buffers.cpp -pthread --std=c++17 -I/usr/local/include -L/usr/local/lib -lboost_coroutine -lboost_context -lboost_system
*/