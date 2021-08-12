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
* ***The template that is used in all template implementations is defining the ID datatype for the messages!***