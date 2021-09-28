# Introduction to ```boost::beast```

## Intro
+ ```boost::beast``` is a header based library that is used for creating networking ***libraries*** that provide HTTP, Websocket and networking protocol functionallities. It is NOT meant to be a library for an application programmer, since we want to abstract away all networking I/O functionallities in application code. Therefore, we need to program libraries that are able doing this. 
    - Since boost::asio is only capable of socket programming in an TCP/IP context and not providing any networking protocol functionallities, ```boost::beast``` fills the gap of beeing a library that has low level of abstraction of the socket programming interface but also delivers the functionallity of talking in a networking protocol language like HTTP or WebSocket.
+ ```boost::beast``` builds on top of boost::asio, so a socket programmer will be able to understand ```beast``` code
    - Also, a basic knowlage of concurrent programming as well as asynchronous programming is needed in order to build libraries on top of ```beast```.
+ The aim of ```boost::beast``` is to deliver a library that is able to be the basis of an application level library. So the ```beast``` components are well-suited for building upon them. 
+ Also, ```boost::beast``` is designed for handling multiple (thousands) of connections in server code and is optimized for runtime performance. 
+ All low level code like the socket interfaces for TCP (instead of UDP) and all other low level stuff, that was explained in the "networking in C" tutorial (https://medium.com/from-the-scratch/http-server-what-do-you-need-to-know-to-build-a-simple-http-server-from-scratch-d1ef8945e4fa) and protocol details are abstracted away by ```boost::beast```. This makes to code that is build upon ```beast``` better readable, maintainable and even faster.

## HTTP Requests
+ Definition of a HTTP message with ```boost::beast```/a HTTP message in general: 
    - https://www.boost.org/doc/libs/develop/libs/beast/doc/html/beast/using_http/protocol_primer.html
    - A serialized HTTP message has a ```"\r\n"``` statement`at the end of every line.
        - ```"\n"``` is the "new line" character
        - ```\r``` is the carriage return character, which tells the terminal emulator that it should move the cursor to the beginning of the line. (Ref.: https://stackoverflow.com/questions/7372918/whats-the-use-of-r-escape-sequence)

## Websocket
+ ```boost::beast``` can be used to create HTTP connections as well as Websocket connections. 
    - The advantage of Websocket over HTTP is, that in Websocket the server could send the client messages without a previous request from the client. 
    - Websocket, as well as HTTP, uses TCP/IP as its underlying communication protocoll, so it is an established connection  
    - Ref.: https://de.wikipedia.org/wiki/WebSocket
+ Websocket is a refinement/subset of HTTP. At first you have to establish a HTTP connection and then request a websocket update throu the HTTP connection
    - ***BUT*** ```boost::beast::http``` and ```boost::beast::websocket``` are two separate namespaces! 

## HTTPS
+ You need a SSL (Secure Socket Layer) context to run ```boost::beast``` stream instances in an encrypted context.
    - These context will get associated with the data exchange sockets within the ```boost::beast``` library in a similar way like it was done in plain ```boost::asio``` in the async I/O tutorial of dens website (https://dens.website/tutorials/cpp-asio/ssl-tls) 
+ The only implementation difference between a HTTP and a HTTPS connection is
    - You have to wrap the ```boost::beast::tcp_stream``` into a ```boost::beast::ssl_stream``` 
    - You have to set the SSL settings for the SSL-Context object. 
    - Client example: ```ssl_http_get_client.cpp```
    - Server example: ```ssl_simple_http_server.cpp``` 

### SSL 
+ If you are a client, for the most servers you need to set a Server Name Indication (short: SNI).
    - Ref.: https://www.cloudflare.com/de-de/learning/ssl/what-is-sni/
        - 

## Important references
+ ```boost::asio``` documentation:  
    - https://www.boost.org/doc/libs/1_75_0/doc/html/boost_asio/reference.html
+ ```boost::beast``` documentation:
    - TODO