#include <boost/asio.hpp>
#include <optional>
#include <queue>
#include <unordered_set>
#include <iostream>
#include <string>

// type aliases 
    // Definitions and automatical includes for a cleaner code
namespace io = boost::asio; 
using tcp = io::ip::tcp; 
using error_code = boost::system::error_code; 
using namespace std::placeholders; 

    // type aliases for type erasure objects - https://stackoverflow.com/questions/20353210/usage-and-syntax-of-stdfunction these are object where you can store lambda functions in
using message_handler = std::function<void (std::string)>; 
using error_handler = std::function<void ()>; 


class Session : public std::enable_shared_from_this<Session>
{
    public: 
        Session (tcp::socket && socket_for_exchange) : socket_(std::move(socket_for_exchange)){
            error_code ec; 
            std::cout << "New Session with remote endpoint " << socket_.remote_endpoint(ec) << " initialized!\n"; 
        }

        void start(message_handler && on_msg, error_handler && on_error){
            this->on_msg_ = std::move(on_msg);
            this->on_error_ = std::move(on_error);
            async_read();   
        }

        void post(std::string const & message_to_client){
            bool idle = outgoing_msgs_.empty(); 
            outgoing_msgs_.push(message_to_client);

            if (idle){
                async_write();  
            }
        }


    private:
        // private encapsulated methods
        void async_read(){
            io::async_read_until(socket_, streambuffer_, "\n", std::bind(&Session::on_read, shared_from_this(), _1, _2)); 
                // structure of the call: (socket_to_receive, buffer_instance, read_completion_handler)
        }

        void on_read(error_code ec, std::size_t bytes_transferred){
            if (!ec){  
                // successful read out the socket
                std::stringstream message; 
                message << socket_.remote_endpoint(ec) << ": " << std::istream(&streambuffer_).rdbuf(); 
                streambuffer_.consume(bytes_transferred); // delete the already read out buffer instance in order to not allocate unnecessary memory
                on_msg_(message.str()); // .str() converts the complete string stream to a normal string
                async_read();  
            } else {
                // error case
                std::cerr << "Something went wrong by reading out the socket!\n";
                socket_.close(); 
                on_error_(); 
            }
        } 

        void async_write(){
            io::async_write(socket_, io::buffer(outgoing_msgs_.front()), std::bind(&Session::on_write, shared_from_this(), _1, _2)); 
                // structure of the call: (socket_to_send, buffer_instance, write_completion_handler)
                // CAUTION: Here we use a boost::asio::streambuf streambuffer object that does NOT need preallocated memory where it can store the incoming data
        }

        void on_write(error_code ec, std::size_t bytes_transferred){
            if (!ec){
                outgoing_msgs_.pop(); // The latest message is already send, since this is just the completion (!) handler
                
                if (!outgoing_msgs_.empty()){
                    // recursive call of the asyn_write function, until the "message to send list" is empty --> this realizes the implicit async I/O scheduling 
                    async_write(); 
                }

            } else {
                std::cerr << "Something went wrong by sending out messages to one client!\n"; 
                socket_.close(ec); 
                on_error_(); 
            }
        }

        // member variables
        tcp::socket socket_;
        io::streambuf streambuffer_; 
        std::queue<std::string> outgoing_msgs_; 
        message_handler on_msg_; 
        error_handler on_error_;  // is only defined by the calling server by invokeing the client->start() method
};


class Server 
{
    public: 
        Server (io::io_service & io_serv_obj, std::uint16_t port_num) : ioserv_(io_serv_obj), acceptor_(io_serv_obj, tcp::endpoint(tcp::v4(), port_num)){
            std::cout << "Server initialized.\n"; 
        } 

        void async_server_accept(){
            socket_.emplace(ioserv_); 
            
            acceptor_.async_accept(*socket_, [&](error_code ec){
                std::cout << "   new client.\n";
                auto client = std::make_shared<Session>(std::move(*socket_)); // // Moving with std::optional is possable https://stackoverflow.com/questions/17502564/how-to-move-from-stdoptionalt
                
                std::cout <<"Posting to all clients\n"; 
                // distribute messages
                client->post("Welcome to the chat!\n"); // Message just the newly accepted client 
                post("We have a newcomer\n"); // Message all connected clients
                
                // add the new client to the clients list after the other got notifyed so the new client does not receive the "We have a newcomer message"
                clients_.insert(client);

                // 
                client->start(
                    std::bind(&Server::post, this, _1), 
                        [&, weak=std::weak_ptr(client)](){
                            if (auto shared = weak.lock(); shared && clients_.erase(shared)){
                                // create a shared pointer from the consumed weak pointer, then erease the weak pointer instance from the clients_ lists and then post this message to the console. Afterwards, the last, local shared pointer to the client instance gets deleted when the lambda goes out of scope
                                post("We are one less!\n\r"); 
                            }
                        }
                ); 
                    // How to read this expression: 
                    //      The first on_message function is the bind from the Server::post function 
                    //      The second on_error function is the here defined lambda function
                    // [&] in the lambda function does mean that all variabvles are available within the lambda function by reference 
                async_server_accept(); // After the acceptance of the last new client is finished, the I/O service object gets primed by a new asynchronous task that should wait and listen for new connections
            });
        }

        void post(std::string const & message){
            for (auto & client : clients_){
                client->post(message); 
            }
        }

    private:
        io::io_service & ioserv_; 
        tcp::acceptor acceptor_;
        std::optional<tcp::socket> socket_; // std::optional is the possability of having a member/input variable present or not without complicated code like std::pair<DataClass, bool>, whereas bool describes if there is actually a value behind the DataClass. Example and explaination: https://devblogs.microsoft.com/cppblog/stdoptional-how-when-and-why/
        std::unordered_set<std::shared_ptr<Session> > clients_; 
};


int main(){
    io::io_service ioserv;
    Server srv(ioserv, 15002); 

    srv.async_server_accept(); 

    ioserv.run(); // run the asynchronous functions and block the main thread here until everything is finished

    std::cout << "Finished program\n"; 
    return 0; 
}


/*
* Compile command:
*   $ g++ simple_tcp_chat_server.cpp --std=c++17 -pthread -I/usr/local/include -L/usr/local/lib -lboost_coroutine -lboost_context -lboost_system 
*/