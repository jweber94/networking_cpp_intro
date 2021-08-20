/*
* Compile command:
*   $ g++ --std=c++17 user_input.cpp -pthread
* 
* This is my implementation to make an asynchronous user input via the keyboard, using a thread that is listening for new chars that are entered within the terminal. 
*/

#include <stdio.h>
#include <iostream>
#include <future>
#include <thread>
#include <chrono>
#include <set>

std::set<char> possable_inputs{'a', 'd', 'p', 'm', 's', 'q', ''}; // The last element is ctrl+c

template<typename R>
  bool is_ready(std::future<R> const& f)
  { return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready; }

void get_keyboard_input(std::promise<char> * promObj)
{
    char input = 'a';
    while(true)//(input != 'q') && (input != '^C'))
    {
        system("stty raw");
        input = getchar();
        system("stty cooked");

        if ( possable_inputs.find(input) != possable_inputs.end() ){
          if (input == ''){
            promObj->set_value(input);
            return;
          } else {
            std::cout << " was your input\n";
            promObj->set_value(input);
            return;
          }

        }
        else {
          std::cout << " was your input - no action triggered.\n";
        }
    }
}

int main(){
  std::cout << "Programm started\n";

  char input_character = 'y'; // default value to not get randomly blocked since we are not under contol whats in the memory before we allocate it
  std::promise<char> prmsChar;
  std::thread input_listening_thr;
  
  input_listening_thr = std::thread(get_keyboard_input, &prmsChar);
  std::future<char> ftrChar = prmsChar.get_future(); // start future

  while (true)
  {        
        if (is_ready(ftrChar)){
          // there is a new character given
          input_character = ftrChar.get(); // blocks the main thread until the user gives the program some input
          if (input_character == 'q'){
            std::cout << "quit commend accepted!\n";
            input_listening_thr.join();
            return 0;
          }

          switch (input_character){
                      case 'a':
                          std::cout << "server accept\n";
                          break;
                      case 'd':
                          std::cout << "server deny\n";
                          break;
                      case 'p':
                          std::cout << "server ping\n";
                          break;
                      case 'm':
                          std::cout << "message all\n";
                          break;
                      case 's':
                          std::cout << "server message\n";
                          break;
                      case '':
                          input_listening_thr.join();
                          return 0;
                          break;
                      default:
                          std::cout << input_character << " is not a valid input. Please try again with a, d, p, m or s to interact with the server or quit the program by pressing q.\n";
            }
            input_listening_thr.join();

            // setting up a new listening thread for the user input
            prmsChar = std::promise<char>(); 
            ftrChar = prmsChar.get_future();
            input_listening_thr = std::thread(get_keyboard_input, &prmsChar);
        } else {
          // do some work
          std::this_thread::sleep_for(std::chrono::milliseconds(500)); // simulate work
          //std::cout << "..."; 
        }
    }
  return 0;
}
