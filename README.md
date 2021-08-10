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