# Introduction to ```boost::beast```

## Intro
+ ```boost::beast``` is a header based library that is used for creating networking ***libraries*** that provide HTTP, Websocket and networking protocol functionallities. It is NOT meant to be a library for an application programmer, since we want to abstract away all networking I/O functionallities in application code. Therefore, we need to program libraries that are able doing this. 
    - Since boost::asio is only capable of socket programming in an TCP/IP context and not providing any networking protocol functionallities, ```boost::beast``` fills the gap of beeing a library that has low level of abstraction of the socket programming interface but also delivers the functionallity of talking in a networking protocol language like HTTP or WebSocket.
+ ```boost::beast``` builds on top of boost::asio, so a socket programmer will be able to understand ```beast``` code
    - Also, a basic knowlage of concurrent programming as well as asynchronous programming is needed in order to build libraries on top of ```beast```.
+ The aim of ```boost::beast``` is to deliver a library that is able to be the basis of an application level library. So the ```beast``` components are well-suited for building upon them. 
+ Also, ```boost::beast``` is designed for handling multiple (thousands) of connections in server code and is optimized for runtime performance. 
+ All low level code like the socket interfaces for TCP (instead of UDP) and all other low level stuff, that was explained in the "networking in C" tutorial (https://medium.com/from-the-scratch/http-server-what-do-you-need-to-know-to-build-a-simple-http-server-from-scratch-d1ef8945e4fa) and protocol details are abstracted away by ```boost::beast```. This makes to code that is build upon ```beast``` better readable, maintainable and even faster.
