#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <functional>
#include <optional>

namespace io = boost::asio; 
namespace ip = io::ip; 

using tcp = ip::tcp; 
using error_code_type = boost::system::error_code; 

// Create a custom dynamic buffer according to https://www.boost.org/doc/libs/1_77_0/doc/html/boost_asio/reference/DynamicBuffer_v1.html in order to create a circular buffer from which we can read and write independendly for read and write operations 
template <std::size_t Capacity>
class CircularBuffer
{
    public: 
        // type definitions needs to be in the public section of a class implementation, since per default everything is private within a class and so the type definitions as well
        using const_buffer_type = boost::container::static_vector<io::const_buffer, 2>; // defining a vector with two elements of the type boost::asio::const_buffer
        using mutable_buffer_type = boost::container::static_vector<io::mutable_buffer, 2>;

        auto prepare(std::size_t n){
            // returns a const_buffer_type or a const_mutable_buffer_type
            if (size() + n > max_size()){
                throw std::length_error("circular_buffer overflow"); 
            }

            return make_sequence<mutable_buffer_type>(buffer_, tail_, tail_ + n); 
        }

        void commit(){
            // put the previously inserted data to the fixed side of the buffer
            tail_ += n; 
        }

        void consume(std::size_t n){
            // free/delete the previously accessed data from the buffer
            head_ += n; // shift the "here begins the data" index n bytes nearer to the "here ends the data" index of the underlying physical memory block
        }

        auto data() const {
            // const does mean that we do not want to modify the data of the buffer with this method, therefore a read only buffer is returned. We do not want to delete or modify the data since the .data() method of a boost::asio::* buffer instance is only used for the write operations. We need to free the memory by consuming it after we wrote it to a socket
            return make_sequence<const_buffer_type>(buffer_, head_, tail_);
        }

        std::size_t size() const {
            // returns the size of the allocated memory in bytes
            return tail_ - head_; 
        }

        constexpr std::size_t max_size() const {
            // constexpr get evaluated during compile time and result in more efficient code but it needs to be well defined during compile time. If not, the compiler will complain
            return Capacity;
        }

        constexpr std::size_t capacity() const {
            return max_size(); 
        }

    private: 
        template <typename Sequence, typename Buffer>
        static Sequence make_sequence(Buffer & buffer, std::size_t begin, std::size_t end){
            // static return type means that we can call the (here private) member function without 
            // an explicit instance (or the this instance) of the class. This comes with the cost of beeing not able to access non-static members of the calling class implementation (ref.: https://stackoverflow.com/questions/15235526/the-static-keyword-and-its-various-uses-in-c) 

            std::size_t size = end - begin; 

            begin %= Capacity; // begin becomes the result of the remainder of the division of begin / Capacity (e.g.: begin = 10 and Capacity = 9, then begin = 1 (which is the remainder of 10/9) after the operation)
            end %= Capacity; 

            if (begin <= end){
                // flat case --> The allocated buffer memory is between the front of the underlying physical memory and its end
                return {
                    typename Sequence::value_type(&buffer[begin], size)
                };
                // We need the curly brackets in order to initialize the sequence type which is defined by the 'using const_buffer_type = ...' and 'using mutable_buffer_type = ...', since the return statement calls implicitly the constructor of the Sequence type and therefore we need to hand over the initializers within curly brackets
            } 
            else {
                // looped case --> The allocated buffer memory ends at the end of the underlying physical memory and starts over again at its front in order to fit completly within the circular buffer
                std::size_t ending = Capacity - begin; 
                return {
                    typename Sequence::value_type(&buffer[begin], ending), 
                    typename Sequence::value_type(&buffer[0], size - ending)
                };
            } // The two cases are visualized in the third image on the tutorial website (ref.: https://dens.website/tutorials/cpp-asio/tcp-echo-server-3)
        }

        std::array<char, Capacity> buffer_;
        std::size_t head_ = 0; // synonym for "Begin" in picture three of the tutorial
        std::size_t tail_ = 0; // synonym for "End" in picture three of the tutorial
}; 


// CircularBufferView is a boost::asio buffer view wrapper according to https://www.boost.org/doc/libs/1_77_0/doc/html/boost_asio/reference/DynamicBuffer_v1.html
// since a buffer instance that is used with the high level boost::asio::* free function must not own the underlying memory of the buffer storage
template <std::size_t Capacity>
class CircularBufferView
{
    public: 
        using buffer_type = CircularBuffer<Capacity>; 
        using const_buffer_type = typename buffer_type::const_buffer_type; 
        using mutable_buffer_type = typename buffer_type::mutable_buffer_type; 

        CircularBufferView(buffer_type& buffer) : buffer_(buffer){
            std::cout << "Initialized CircularBufferView\n"; 
        }

        auto prepare(std::size_t n){
            return buffer_.prepare(n); 
        }

        void commit(std::size_t n){
            buffer_.commit(n); 
        }

        void consume(std::size_t n){
            buffer_.consume(n);
        }

        auto data() const {
            return buffer_.data();
        }

        auto size() const {
            return buffer_.size(); 
        }

        constexpr auto max_size() const {
            return buffer_.max_size(); 
        } 

        constexpr auto capacity() const {
            return buffer_.capacity(); 
        }

    private: 
        buffer_type& buffer_; 
};

template <std::size_t Capacity>
CircularBufferView<Capacity> make_view(CircularBuffer<Capacity>& buffer) {
    // makes a buffer view from the cicular buffer that we have implemented. This is needed since the boost::asio::async_read and async_write free functions do need a reference (!) to the underlying memory and not the memory itself 
    return buffer; 
}

class Session : public std::enable_shared_from_this<Session>
{   
    public:
        Session(tcp::socket && socket) : socket_(std::move(socket)), instreambuffer_(65536){
            std::cout << "[SERVER]: Session initialized.\n"; 
        }

        void start(){
            read();
        }
    private: 

        void read(){
            io::async_read(socket_, make_view(buffer_), io::transfer_at_least(1), std::bind(&Session::on_read, shared_from_this(), std::placeholders::_1, std::placeholders::_2)); 
        }

        void on_read(error_code_type ec, std::size_t bytes_transferred){
            // boost asio read handler
            if (!ec && bytes_transferred){
                // if bytes_transferred == 0 then the instreambuffer_ is completly filled with data
                // We consider an exceeded buffer as a client request violation. This should be documented within the API documentation 
                if (!writing_){
                    // the writing back to the client is scheduled if we are currently not writing
                    write();
                }
                // schedule the next read operation after we write the sended data back to the client
                read(); 
            } else {
                close(); 
                std::cerr << "[SESSION]: An error occured or the incoming buffer instance is completly full. Droped the session.\n";  
            }
        }

        void write(){
            // setting the flag in order to hand back the status of writing to the asynchonous reading completion handler
            writing_ = true; 
            
            /*
            // No need to manage the memory manually if we use our own dynamic buffer 

            // manage the streambuffer for writing out 
            auto tmp_buffer_handle = outstreambuffer_.prepare(instreambuffer_.size());
            io::buffer_copy(tmp_buffer_handle, instreambuffer_.data());
            outstreambuffer_.commit(tmp_buffer_handle.size());  
            */

            // write from the out buffer
            io::async_write(socket_, make_view(buffer_), std::bind(&Session::on_write, shared_from_this(), std::placeholders::_1, std::placeholders::_2)); 
        }

        void on_write(error_code_type ec, std::size_t bytes_transferred){
            writing_ = false;

            // We do not schedule a new write operation inside the on_write write-completion handler, so we are not shure that all received data is written back to the client, see the in-depth explaination from https://dens.website/tutorials/cpp-asio/tcp-echo-server-2
            if (!ec){
                if (buffer.size()){
                    // if size() delivers a value >= 1 the if-statement will be evaluated as true
                    write();  
                }
            } else {
                std::cerr << "[SESSION]: An error occured during writeing. Dropped the session!\n"; 
                close(); // ping pong between read and write, no queue for read and write necessary
            }

        void close(){
            error_code_type ec; 
            socket_.close(ec); 
        }

        tcp::socket socket_; 
        // separate the buffers for reading and writing in order to decouple the read and write operation 
        CircularBuffer<65536> buffer_; 
        bool writing_ = false; // flag for checking if there is currently a write process
}; 


class Server 
{
    public: 
        Server(io::io_service & ioserv, std::uint16_t port) : ioserv_(ioserv), acceptor_(ioserv, tcp::endpoint(tcp::v4(), port)){
            accept(); 
        }

    private: 
        // member methods
        void accept(){
            socket_.emplace(ioserv_); 

            acceptor.async_accept(*socket_, [&](error_code_type ec){
                std::make_shared<Session>(std::move(*socket))->start(); 
            }); 

        }

        // member variables
        io::io_service & ioserv_;
        tcp::acceptor acceptor_;
        std::optional<tcp::socket> socket_;  
};

int main(int argc, char * argv[]){

    if (argc != 2){
        // If you do not hand over any command line arguments, we want to say how to use the server properly and then end the program
        std::cout << "Usage: Server [PORT]\n"; 
        return 0; 
        
    } else {
        io::io_service ioserv; 

        Server srv(ioserv, boost::lexical_cast<std::uint16_t>(argv[1]));
            // The argv[0] argument is the name of the program and the following arguments are the given command line parameters that are space-separated
            //std::cout << "This is argv[0]: " << argv[0] << std::endl;  
            //std::cout << "This is argv[1]: " << argv[1] << std::endl;
    }

    return 0; 
}


/*
* Compile Command:
*   $ g++ simple_echo_server.cpp I/usr/local/include -L/usr/local/lib -lboost_coroutine -lboost_context -lboost_system
*/