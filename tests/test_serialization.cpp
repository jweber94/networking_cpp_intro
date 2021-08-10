#include <gtest/gtest.h>
#include "net_message.hpp"

TEST(sample_test_case, sample_test)
{
    
    enum class my_ids {
        entry_one, 
        entry_two
    };

    float test_payload = 5.5; 

    custom_netlib::message<my_ids> test_msg;
    test_msg.header.id = my_ids::entry_one; 
    test_msg << test_payload; 
    
    std::cout << "The payload is: " << test_msg << "\n"; 

    EXPECT_EQ(1, 1);
}