# Networking introduction project
This project is based on the video series of javidx9 (https://www.youtube.com/watch?v=2hNdkYInj4g&t=485s) to familiarize myself with networking in C++ and get a deeper understanding how to program networking applications on a very low level of abstraction. Also the most common issues are elaborated as well as discussed, how they could be solved, using the ```boost::asio``` library. The main takeaways are written down here in the ```README.md```.
The difference between my project and the one of javidx9 is, that he uses a Windows setup by incooperating Visual Studio and its build system, whereas I using a CMake project as my build system and working completly under linux (Ubuntu 20.04 LTS). So lets get started. ;-)  

## General Information about ```boost::asio```
+ boost::asio is a purely header based library and you can use the boost version of asio as well as the boost independent version of asio by downloading it from https://think-async.com/Asio/ 
+ asio could handle not only networking, but also a lot of other forms of input-output operations for external ressources (which is everything that is not associated with the process that the main executable of the programm) 
+ asio is very good at handly errors --> Error-Codes are thrown whenever something went wrong and they have a very descriptive form so you can get a good intention about what was gone wrong

## Common mistakes in networking
### main_client.cpp
* Between the request from the client and the response from the server to the client, it takes time to transfer the bytes over the internet/network. So if we execute the read out for the socket immediatly after we sended the request to the server, there will be propably no or not all data available 
    - One possible opportunity is to create a time delay between the request and the readout for the response in the code. But this is not good practice!
    - Another possability is to block the calling thread for the socket to wait until there is data arrived. The problem with this is, that the execution of the socket readout is triggered as soon as the first packet of data is arrived, which results in an incomplete readout of the data.
***The cental problem with blocking the execution before read out the socket is, that we do NOT know how long the transport of the takes. We do not know, how much time the response needs to complete, since the request is out of our computer as well as out of our control! Also, we do not know how much data we need to receive in order to get the complete data from the server. This problem is twofold: First, we do not know when the data transmission is finish, second, we do not know what size our data buffer needs to be!***
+ Possible solutions: 
    - Make a resonably large data buffer for the aimed data that we expect to receive
    - Use the asynchronous functionalities of boost::asio --> grapSomeData function in ```main_client.cpp```

### main_client_async.cpp
+ In order to not block the main function from execution while waiting for the full data to be arrived, we can use the asynchronous functions of ```boost::asio```. But caution: Thinking asynchronously is not that easy! All ```async_``` functions of asio return immediatly after they were called in the code. In the background, the OS will do the asynchronous tasks that are associated with the so called "I/O objects" that are encoded by the ```boost::asio async_``` commands. They all want a handler (which is a callback function/function pointer that follows some rules) which is called by the I/O service object, when its .run() method is called. 
    - CAUTION: The .run() method is a blocking method for the thread that is calling the ```ioserv.run()``` in the code. 
+ Good practice: We do not want the main thread to be blocked. So the ```ioserv.run()``` will be executed in its own thread, which solves the described problem
    - Problem: After .run() was called and the I/O service object has nothing to do (i.e. all work that is defined by I/O objects like ```socket.async_read_some(...)```), the I/O service object ends its execution capability. So if we receive multiple data packages over the network over time, just the first packed will be received and the I/O service object will end. 
    - Solution: Give the I/O service object some idle work to do so that it does not end its execution capability until we say ```ioserv.stop()```.
+ You can see the code in ```main_client_async.cpp``` especially in line 49. 
+ Interesting remark: If we adjust the size of the buffer element where the received data is stored, we can define how many bytes of data every ```read_some()``` command should read from the socket
+ Asynchronous programming (in its common meaning as concurrent programming as well as with boost::asio) brings a lot of possabilities but also comes with the cost of more complexity, since things do not execute in the order that is shown in the source code
    - Underlying Idea: Define something that should do work for us in the future 

## Requirements
* CMake: >= Version 3.11.4
* gcc: >= Version 5.5.0
* boost: >=  Version 1.72.0 (install a defined version of Boost: https://stackoverflow.com/questions/12578499/how-to-install-boost-on-ubuntu - the "build from source" explaination)
* GTest: Version >= 1.11.0

## Networking Library Project
Besides the ```boost::asio``` explaination files, I want to develop my own version of the networking protocol from the javidx9 videos. Therefore, I implement a library, called custom_net_lib, which can be found in /src/common_net_lib and is the accordingly named library in the ```CMakeLists.txt``` file.
To make the library accessable from outside the project, the interface headers are stored within the ```/include/custom_net_lib``` folder.

You can include the header based library in your own project by including ```#include "custom_net_lib.hpp"```

### Message structure
The message structure is defined as it is show in the following picture: (ref: https://www.youtube.com/watch?v=2hNdkYInj4g&t=485s)
![message_structure](/images/message_structure.png) 
(***image by javidx9***)
In the structure, the ID is meant to be defining the kind of the payload data. It should be an enum class, so the compiler can check,if we send valid IDs at compile time rather then on runtime. In order to make the message framework as flexible as possible, we define that enum class as a template, so the user can define its own IDs. 
The header has always the same size in form of number of bytes that are associated with the header. By doing so, we can make it possible to divide the payload from the header easily from the payload within the code. 
Since sockets can only transfer serialized data, the message type must have functions that can serialize and deserialize the stored data in the message instances. 

### Architecture
* Client: 
    - Is in its infinite loop and does the work it is supposed to do. During that time (i.e. until the .run() method of the I/O service object is called and so the handler of an endpoint is incooperated), the client could receive multiple messages from the server, so it need a queue to store the incoming messages before processing them.
    - At the same time, the client could send requests to the server at any time. 
    - Since clients could decide to do multiple requests to the server in a shorter amount of time then the request could be send throu the socket, the sending process also needs a queue for the outgoing requests.
* Server: 
    - Since the client could send requests to the server at any time and one server could talk to multiple clients, it also needs a queue for storing incoming messages that needs to be processed, while it is working on the latest request in the queue. 
    - Since the server need to answer multiple clients or multiple requests in a shorter amount of time then the socket could process, a queue for the sending process is also needed ***for every connection*** (due to the fact, that the listening for incoming connections is on a different socket then the data exchange between the client and the server.) 
    - In this example, the server should be capable to answer multiple/all clients at one time. Alternativly it can answer just one client. 
    - All incoming requests to the server should be processed by just one server instance object, so all requests from multiple clients needs to be stored in ***one*** queue. 
Out of this, the following Software structure is established (***image by javidx9***) 
![server_client_architecture](/images/client_server_architecture.png)

Overall components: 
* We need thread save queues, since
* The connections can be abstracted by a class in C++, since they are all of the same structure. (if you are looking from the server ***OR** the client instances)
    - Also this object should abstract everything networking related away from the client and server applications
* A client class needs to be implemented, since we want to create multiple clients for different use cases
* A server class needs to be implented in order to realize the server functionallies
* ***The template that is used in all template implementations is defining the ID datatype for the messages! (except within the thread safe queue, where the template describes the custom message datatype)***

#### Server
* The server always runs on a machine (in an infinite loop)
* Two tasks: 
    - Listening for new connections from a client
    - Handle the application logic
* Since clients can connect to the server at _any_ time, the connection listening should be done in a asynchronous manner (So the appication logic can be executed in parallel and the connection establishment is not blocking the application)
    - --> Asynchronous connection handler is needed with asio
* Also, the disconnection needs to be handled by the server and the client can disconnect at _any_ time!
    - --> Asynchronous disconnection handler needs to be implemented with asio
* ***We want to create an abstract base class of the server from which a CustomServer class can be derived. The base class will implement the networking interface, whereas the CustomServer implements the Server logic***

![server_overview](/images/server_overview.png)

***Asio implementation of the Server Networking part***
* The asio I/O service object does not need idle work in the server implementation, since it always listens for new connections. 
* Workflow of the networking implementation with asio: 
    - The asio I/O service object (i.e. ```boost::asio::io_service```) is listening for connections on a defined port of the machine where the server is running on
    - If a client asks for a connection to the server, a data-exchange socket will be opened (which is a different, OS defined port of the system, so the server can still listen to other new incoming connections) --> The creation of new connections needs to be in a different thread then the "listening for new connections"
    - After the socket is created and the connection is established, a Connection-Instance of our library implementation needs to be created in order to exchange messages
        - The connection object needs to check if the client is allowed to connect to the server, so that malicious clients/connection approaches can be rejected (e.g. banned IP-addresses)
    - If the connection is not rejected, we want the connection object be placed in a container (e.g. std::vector) of established, valid connection that can be processed by the server (shown in red in the image)
    - The arrow back to the "accept connection" block from the "establish with server" should describe the logic of the program!
![server_networking_logic](/images/server_net_logic.png)
* The connection object will be handled by a shared_ptr, so if the connection is rejected, no shared_ptr will be associated with the connection object and as soon as the shared pointer is going out of scope, the connection object gets deleted.
    - Remark: In asynchronous programms, smartpointers are very useful, since things happening in parallel. Also the rule of five/move semantics plays a key role!
* IMPORTANT: The I/O service object can handle multiple things in parallel. We can think of it like a sheduler for parallel tasks, but things could actually happen in parallel
* *How to deal with reading messages:*
    - Since the I/O service object can handle multiple asynchronous tasks, the Connection instances in the (red marked) container can ask it for processing the header of their associated request. 
    - The method that reads/process the header can ask then the I/O service object for processing the payload
    - After the payload was processed, it is stored in the "Incoming Queue", where the payload as well as the associated owner (i.e. the client) is saved for processing with the server logic
![server_async_proc](/images/server_async_processing.png)

* For Server, ```boost::asio``` has an ```boost::asio::ip::tcp::acceptor``` object, that needs an endpoint for its initialization and then realizes the functionallity of listening for new connections and create new socket objects where the data exchange can happen. This a a kind of "implicitly defining a socket" for the listening to new connections.

## CAUTION with the custom message definition
Since this is a custom (non RFC standardized) networking message protocol, we can _NOT_ use networking debug tools like PUTTY or Postman. We need to implement a custom server in order to test the communication interface between the client and the server. 

## Cyber Security 
Especially the server is a dedicated aim for cyber attacks. Therefore, a client-server handshake is implemented. The main issue with the server code in its plain working form is, that he listens for new connections and if he receives a message, it trys to read the header of the message. Right after reading the ID, the size of the payload is read out. If there is a malicious message with a much to big size of the payload, the size will be allocated on the system which can course memory issues. Also there is a possability for injecting malicious instructions after a buffer overflow and other security vulnerabilities.

In order to solve that problem, a validation process after requesting a connction from the client to the server needs to be established. The basic princaple of doing such a handshake is to send the client a message with a puzzle that the client has to solve. Then, the client solves the puzzle and sends its solution back to the client and if the server can confirm that the client successfully solved the puzzle, it will aknowlage the connection and the message exchange can take place. 
This process is visualized within the following picture (extracted from https://www.youtube.com/watch?v=hHowZ3bWsio&t=272s):
![client_validation](/images/client_validation.png)
If a client can not sucessfully validate the connection for multiple times, it is possable to put its IP-adress on a blacklist from which all attempts will be immediatly get rejected. (Maybe just internally, so that the attacker will not recognize it.)
If you want to be even more secure, we can implement that the validation process should be renewed all _N_ messages or all _t_ seconds.

A secure validation process looks as the following: (also extracted from https://www.youtube.com/watch?v=hHowZ3bWsio&t=272s)
![secure_validation](/images/secure_validation.png)
The aim of a secure handshake is to make it as hard as possable to try out, what the protocol does behind the scenes, since we can read the messages in clear text during the handshake. Modern appraches use randomly generated data/numbers. The randomly generated data is then processed by the server with a function that is also known to the client (but ***NOT*** to any other programmers/hackers). The client receives the random data, calculates the same function on the client side and sends its result back to the server. The server looks if the result that he received from the client is equal to his calculation and if this is the case, the client is a validated communication partner.   

The benefit of working with random data that is generated on the server side of the communication is, that an attacker/hacker could not reproduce the inputs that easy so he could not look what happens, if he changes a bit/bit shifts, additions, ... in the handshake messages. Current reseach is working on good encryption functions ```f(x)``` as the "puzzle" that gets solved by the client (as well as the server).

Tipp: Within the encryption, you can define a digit of one of the hexadecimal constants with the release version number of the code. By doing this, you can prevent old versions of the client from exchanging data with a newer/other version of the server.  

## ToDos for the Udacity capstone 
* Implement a Message interface where the clients can send user inputs to each other and to all in the chat
* Use ```boost::program_options``` to setup a flexible port where the server should listen for new connections via the command line
* Document the work within a clean ```README.md``` and clean up some comments within the code in order to hand the project in 
