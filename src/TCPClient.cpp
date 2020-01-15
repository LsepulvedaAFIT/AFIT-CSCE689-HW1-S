#include "TCPClient.h"

//STL
#include <iostream>
#include <string.h>
#include <unistd.h>
#include <vector>
#include <sstream>

//networking headers
#include <sys/socket.h> // Core BSD socket functions and data structures.
#include <netinet/in.h> // AF_INET and AF_INET6 address families and their corresponding protocol families PF_INET and PF_INET6.
#include <arpa/inet.h>  // Functions for manipulating numeric IP addresses.
#include <netdb.h>

//for non-blocking
#include <fcntl.h>

#include "exceptions.h"

/**********************************************************************************************
 * TCPClient (constructor) - Creates a Stdin file descriptor to simplify handling of user input. 
 *
 **********************************************************************************************/

TCPClient::TCPClient() {
}

/**********************************************************************************************
 * TCPClient (destructor) - No cleanup right now
 *
 **********************************************************************************************/

TCPClient::~TCPClient() {

}

/**********************************************************************************************
 * connectTo - Opens a File Descriptor socket to the IP address and port given in the
 *             parameters using a TCP connection.
 *
 *    Throws: socket_error exception if failed. socket_error is a child class of runtime_error
 **********************************************************************************************/

void TCPClient::connectTo(const char *ip_addr, unsigned short port) {
    this->socketFD = socket(AF_INET, SOCK_STREAM, 0);
    //this->socketFD = -1;//testing
    if (this->socketFD < 0){
        std::cout << "socket failed\n";
        //const char* errSerSock = "Client socket failed";
        //socket_error(errSerSock);
    }
    
    this->servAddress.sin_family = AF_INET;
    this->servAddress.sin_port = htons(port);

    if(inet_pton(AF_INET, "127.0.0.1", &servAddress.sin_addr) <= 0)  
    { 
        printf("\nInvalid address/ Address not supported \n"); 
        exit(EXIT_FAILURE); 
    } 

    int connVal = connect(this->socketFD, (struct sockaddr *)&servAddress, sizeof(servAddress));
    if (connVal < 0) 
    { 
        printf("\nConnection Failed: connVal \n"); 
        exit(EXIT_FAILURE);
    } 

}

/**********************************************************************************************
 * handleConnection - Performs a loop that checks if the connection is still open, then 
 *                    looks for user input and sends it if available. Finally, looks for data
 *                    on the socket and sends it.
 * 
 *    Throws: socket_error for recoverable errors, runtime_error for unrecoverable types
 **********************************************************************************************/

void TCPClient::handleConnection() {
    int valread; 
    //const char* hello = "Hello from client"; 
    std::string command;
    char buffer[1024] = {0}; 
    std::string readBuffer = "";

    //send(this->socketFD, hello, strlen(hello) , 0 ); 

    while (true)
    {
        std::cout.flush();
        valread = read( this->socketFD, buffer, 1024); 
        std::cout << buffer;
        std::cout.flush();
        memset(buffer, 0, 1024);
        std::cin >> command;
        const char *sendCmd = command.c_str();
        send(this->socketFD, const_cast<char *>(sendCmd) , strlen(sendCmd) , 0 ); 
        std::cout << "Command message sent\n\n" ; 
        std::cout.flush();
    }
}

/**********************************************************************************************
 * closeConnection - Your comments here
 *
 *    Throws: socket_error for recoverable errors, runtime_error for unrecoverable types
 **********************************************************************************************/

void TCPClient::closeConn() {
    close(this->socketFD);
}


