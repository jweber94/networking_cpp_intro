#include <gtest/gtest.h>
#include "net_message.hpp"

TEST(sample_test_case, sample_test)
{
    
    enum class CustomMsgTypes : uint32_t {
        MsgID1, 
        MsgID2
    };

    float test_payload_1 = 5.5;
    bool test_payload_2 = true; 
    int test_payload_3 = 3;

    // create the message based on the test payload of different types    
    custom_netlib::message<CustomMsgTypes> test_msg;
    test_msg.header.id = CustomMsgTypes::MsgID1; //my_ids::entry_one; 
    test_msg << test_payload_1 << test_payload_2 << test_payload_3; 
    
    // create variables with known wrong entrys and then save their new values based on the message
    float test_returned_payload_1 = 1.1;
    bool test_returned_payload_2 = false; 
    int test_returned_payload_3 = 1;

    test_msg >> test_returned_payload_3 >> test_returned_payload_2 >> test_returned_payload_1;  

    EXPECT_EQ(test_payload_1, test_returned_payload_1);
    EXPECT_EQ(test_payload_2, test_returned_payload_2);
    EXPECT_EQ(test_payload_3, test_returned_payload_3);
}