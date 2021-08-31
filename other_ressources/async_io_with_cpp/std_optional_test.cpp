#include <iostream>
#include <optional>

class TestOpt
{
    public: 
        void setX(double * input_val){
            my_value_ = std::move(input_val);  
            std::cout << "My value befor reset is: " << my_value_ << "\n";
            *my_value_ = 6.6;  
            std::cout << "My value after reset is: " << my_value_ << "\n"; 
        }   

    private: 
        double * my_value_; 
}; 


int main(){

    std::optional<double> test_value; // Defines a pointer to a double 
    std::cout << "Uninitialized value: " << test_value.has_value() << " with the value: " << *test_value << "\n"; 
    test_value = 4.2; 
    std::cout << "Initialized value: " << test_value.has_value() << " with the value: " << *test_value << "\n";
    
    TestOpt testClass;
    if (test_value.has_value()){
        testClass.setX(test_value); // hand over the pointer that gets moved away within the class
    } else {
        std::cerr << "The optinal value has no value initialied!\n"; 
    }
    
    std::cout << "After moving: " << test_value.has_value() << " with the value: " << *test_value << "\n";
    test_value = 7.7; 
    std::cout << "After setting a new value: " << test_value.has_value() << "\n" ;

    std::cout << "Finished test program\n"; 
    return 0;
}

/*
* Compile command: 
*   $ g++ --std=c++17 
*/