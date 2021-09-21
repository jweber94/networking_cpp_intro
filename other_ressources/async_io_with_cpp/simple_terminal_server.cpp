#include <iostream>
#include <optional>
#include <queue>
#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/date_time.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>

namespace io = boost::asio; 
namespace ip = io::ip; 

using tcp = ip::tcp; 
using error_code_type = boost::system::error_code; 
using namespace std::placeholders; // for _1 in std::bind()

using string_group = std::vector<std::string>; 

using dispatcher_type = std::map<std::string, dispatcher_entry>; 

struct DispatcherEntry
{
    // Dispatcher (= Verteiler in german) entry which describes, what a connected client can request from the server
    std::string const description; 
    std::size_t const args; 
    std::function<std::string (string_group const &)> const handler; 
};

// free functions
string_group split(std::string const & string){
    using separator = boost::char_separator<char>; 
    using tokenizer = boost::tokenizer<separator>;

    string_group group; 

    for (auto && str : tokenizer(string, separator(" "))){
        // str is an rvalue due to the && reference operator, so we can emplace_back it (which does move semantics under the hood)
        group.emplace_back(str); 
    }

    return group; 
}

// Session implementation - needs to be defined (or at least declared) before the server class implementation, since the server incooperates the session class
class Session : public std::enable_shared_from_this<Session>
{
    public: 
        // constructor
        Session(tcp::socket && socket, dispatcher_type const & dispatcher) : data_socket_(std::move(socket)), dispatcher_ref_(dispatcher) {
            std::cout << "[SESSION]: A new session was initialized\n"; 
        } 

        // member methods
        void start(){
            write("Welcome to my terminal server!"); 
            wrtie(); // equals \n if the argument was left empty
            read(); // start listening for client requests
        }

    private:

        void write(std::string const & string_to_write){
            outgoing_.push(string + "\r\n> "); // the > is the client interface-look during the active connection 
        }

        void read(){
            io::async_read_until(data_socket_, incoming_buffer_, "\n", std::bind(&Session::on_read, shared_from_this(), _1, _2));
                // here we define that the server should read until it receives a new line commend (which is in C style the \n character pair) 
        }

        void on_read(error_code_type ec, std::size_t bytes_transferred){
            if (!ec}
            {
                std::istream stream(&incoming_buffer_);
                std::string line; 
                std::getline(stream, line);

                incoming_buffer_.consume(bytes_transferred);

                boost::algorithm::trim(line); 

                if (!line.empty()){
                    // if we receive something from the socket which is NOT empty
                    dispatch(line); 
                }  

                read(); // schedule the next reading for the I/O service object

            } else {
                std::cerr << "[SESSION]: An error occured during reading\n"; 
                data_socket_.close(); 
            }
        }

        void dispatch(std::string const & line){
            auto command = parse(line);
            std::stringstream response; 

            if (auto it = dispatcher_ref_.find(command.first); it != dispatcher_ref_.cend()){
                // check if the first element of the received (from the client) command is within the hash map of the dispatcher
                // .cend() is the last iterator of the keys in the hash map of std::map<key_type, value_type>
                auto const & entry = it->second; // ->first of a map iterator element is the key of the map element and ->second is the value of the corresponding key
                    // entry is the value of the hash map which is a DispatcherEntry instance (a self defined struct that has a public member args)
                if (entry.args == command.second.size()){   
                    // check if the given commend has a valid number of arguments (the valid number of arguments is defined within the dispatcher instance)
                    try
                    {
                        response << entry.handler(command.second); 
                    }
                    catch(const std::exception& e)
                    {
                        response << "An error has occured: " << e.what();
                    }
                    
                } else {
                    // key of the dispatcher was found but the wrong number of arguments for the key was given
                    response << "Wrong arguments count. Expected: " << entry.args << " , provided: " << commend.second.size(); 
                }

            } else {
                // the key and the from the client delivered command is not found in the dispatcher
                response << "Command not found. Available commands:\n\r"; 

                for (auto const & [cmd, entry] : dispatcher){
                    response << "- " << cmd << ": " << entry.description << "\n\r"; 
                }
            }

            write(response.str()); // push the response in the outgoing_buffer_

            if (outgoing_buffer_.size() == 1){
                // check if the outgoing queue is empty before that and then write to the client
                write(); 
            } 
        }

        static std::pair<std::string, string_group> parse(std::string const & string){
            string_group args = split(string); 
            
            auto name = std::move(args[0]);

            args.erase(args.begin());

            return {name, args}; // implicitly calling the std::pair<std::string, string_group> constructor with the curly brackets 
        }

        void write(){
            io::async_write(data_socket_, io::buffer(outgoing_.front()), std::bind(&Session::on_write, shared_from_this(), _1, _2)); 
        }

        void on_write(error_code_type ec, std::size_t bytes_transferred){
            if (!ec){   
                outgoing_.pop(); // release the previously sended element from the out queue

                if (!outgoing.empty()){
                    // if there are still more elements in the outgoing queue, we want to send them until all elements were sent and the queue is empty
                    write(); 
                } 

            } else {
                std::cerr << "[SESSION]: An error occured during writeing\n"; 
            }
        }

        tcp::socket data_socket_;
        dispatcher_type const & dispatcher_ref_; // const reference to a superordinate dispatcher instance that does the dispatching for the server-client session
            // dispatcher_type is a hash map that is the same for all session objects. Therefore, a reference to a superordinate dispatcher instance is reasonable
        io::streambuf incoming_buffer_; 
        std::queue<std::string> outgoing_;            
};

class Server {
    public: 
        Server(io::io_service & ioserv, std::uint16_t port) : ioserv_(ioserv), acceptor_(ioserv_, tcp::endpoint(tcp::v4(), port)) 
        {
            std::cout << "[SERVER]: Server initialized\n"; 
            accept(); // start listening for new connections from clients
        }

        template <typename F>
        void attach(std::string const & pattern, std::string const & description, F && f){
            // add a new command to the dispatcher of the server logic
            // f is a function pointer or a lambda function in the .attach() call, that will be executed if the client requests a dispatcher entry
            auto cmd = split(pattern); 

            dispatcher.emplace(cmd[0], dispatcher_entry{
                description, 
                cmd.size() - 1, 
                std::move(f)
            }); 
        }

    private: 

        void accept()
        {
            socket_.emplace(ioserv_); 

            acceptor_.async_accept(*socket_, [&](error_code_type ec){
                // create new session and then start listening for the next connection
                std::make_shared<Session>(std::move(*socket), dispatcher)->start();
                accept(); 
            }); 
        }

        io::io_service ioserv_;
        tcp::acceptor acceptor_; 
        std::optional<tcp::socket> socket_; 
        dispatcher_type dispatcher_;  
}; 

int main(int argc, char * argv[]){
    
    // server start input check
    if (argc != 2){
        std::cout << "Usage: server [Port]\n";
        return 0; 
    }

    io::io_service ioserv; 

    Server srv(ioserv, boost::lexical_cast<std::uint16_t>(argv[1])); // get the first calling argument as the port number 

    // create the dispatcher entrys
    srv.attach("date", "Print current date and time", [](string_group const & args){
        auto now = boost::posix_time::second_clock::local_time(); 
        return to_simple_string(now); 
    }); 

    srv.attach("pwd", "Print working directory", [](string_group const & args){
        return boost::filesystem::current_path().string(); 
    }); 

    srv.attach("dir", "Print working directorys content", [](string_group const & args){
        std::stringstream result; 

        std::copy(
            boost::filesystem::directory_iterator(boost::filesystem::current_path()), 
            boost::filesystem::directory_iterator(), 
            std::ostream_iterator<boost::filesystem::directory_entry>(result, "\n\r")
        ); 

        return result; 
    }); 

    srv.attach("cd", "Change current working directory", [](string_group const & args){
        boost::filesystem::current_path(args[0]);
        return "Working directory: " + boost::filesystem::current_path().string(); 
    }); 

    src.attach("mul % %", "Multiply two numbers", [](string_group const & args){
        double a = boost::lexical_cast<double>(args[0]);
        double b = boost::lexical_cast<double>(args[1]);

        return std::to_string(a * b); 
    }); 

    ioserv.run(); 

    return 0; 
}

/*
* Compile Command:
*   $ g++ simple_terminal_server.cpp I/usr/local/include -L/usr/local/lib -lboost_coroutine -lboost_context -lboost_system
*
* Connect to the server with telnet
*
* Reference: 
*   https://dens.website/tutorials/cpp-asio/simple-terminal-server
*/