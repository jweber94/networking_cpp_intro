#include <iostream>
#include <functional>
#include <string>

/*
 * Reference to std::bind: 
 * 	https://en.cppreference.com/w/cpp/utility/functional/bind
 * 		Takes as its first argument a callable - something that we can call with or without parameters
 * 		It returns an unspecifyed type T which depends on the input to the bind function
 *
 * CAUTION: std::bind is very costly when you are using it
 *
 * std::bind can be used to reorder or redefine the number of parameters that a function should have (e.g. the third example)   
 */

void print(const int & i){
	// This will be our callable function
	std::cout << "Hello, my number is: " << i << "\n"; 
}

void multiple_params(int counter, const std::string & input_str){
	std::cout << "Counter: " << counter << " , with string: " << input_str << "\n"; 
}

template<typename T>
void test_template(T i, const std::string & s){
	std::cout << i << " , " << s << "\n"; 
}

int main(){
	int i = 7; 
	const auto result_of_binding = std::bind(&print, i);
       	result_of_binding(); 	
	i = 9; 
	result_of_binding(); 
	// CAUTION: std::bind makes a copy of the arguments that are passed into the bind
	
	const auto result_2 = std::bind(&print, std::ref(i)); 
	result_2(); 
	i = 11; 
	result_2(); 
	// This can be resolved by handing over an explicit reference to the bind statement


	// update number of function parameters
	const auto one_param = std::bind(&multiple_params, std::ref(i), std::placeholders::_1); 
	one_param("hello");

	// reorder the function parameters
	const auto swaped_params = std::bind(&multiple_params, std::placeholders::_2, std::placeholders::_1); 
	swaped_params("hello", 2); 

	// you can not hand std::bind templates
	const auto test_templ = std::bind(&test_template<double>, 4.2, std::placeholders::_1); 
	test_templ("hello"); 

	// a returned function by std::bind can be passed around by using a std::function<T> which makes your code more strongly typed
	std::function<void (const std::string &)> test_fcn(test_templ);
	test_fcn("hello function"); 	

	std::cout << "Finished program!\n"; 
	return 0; 
}
