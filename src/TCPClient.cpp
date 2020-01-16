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
#include "strfuncts.h"

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
    errorCheck(this->socketFD, "socket failed\n");
    
    this->servAddress.sin_family = AF_INET;
    this->servAddress.sin_port = htons(port);

    if(inet_pton(AF_INET, "127.0.0.1", &servAddress.sin_addr) <= 0)  
    { 
        throw socket_error("Invalid address, not supported");
    } 

    int connVal = connect(this->socketFD, reinterpret_cast<struct sockaddr *>(&this->servAddress), sizeof(this->servAddress));
    errorCheck(connVal, "connect failed\n");
}

/**********************************************************************************************
 * handleConnection - Performs a loop that checks if the connection is still open, then 
 *                    looks for user input and sends it if available. Finally, looks for data
 *                    on the socket and sends it.
 * 
 *    Throws: socket_error for recoverable errors, runtime_error for unrecoverable types
 **********************************************************************************************/

void TCPClient::handleConnection() {
    //variable for input/output commands and reading data
    int valread;
    std::string command;
    char buffer[1024] = {0}; 
    std::string readBuffer = "";

    //Main loop for sending and recieving data from server
    while (true)
    {
        //flush for avoid errors
        std::cout.flush();
        //reads and displays message on console
        valread = read( this->socketFD, buffer, 1024);
        std::cout << buffer;
        //flush for avoid errors
        std::cout.flush();
        //clears the buffer
        memset(buffer, 0, 1024);

        //take command and add newline to signifiy complete command
        std::cin >> command;
        command = command + '\n';
        //converts to char star and sends to server
        const char *sendCmd = command.c_str();
        send(this->socketFD, const_cast<char *>(sendCmd) , strlen(sendCmd) , 0 );
        //User message for awarness 
        std::cout << "Command message sent\n\n" ; 
        //checks if user is exiting
        clrNewlines(command);
        if (command == "exit"){
            //exits out of loop and proceeds to shutdown
            break;
        }
    }
}

/**********************************************************************************************
 * closeConnection - Your comments here
 *
 *    Throws: socket_error for recoverable errors, runtime_error for unrecoverable types
 **********************************************************************************************/
void TCPClient::closeConn() {
    std::cout << "Shutting down connection\n";
    close(this->socketFD);
}

//Checks if input is less than 0 and throw an error with message
void TCPClient::errorCheck(int input, std::string errMess){
    if (input < 0)
    {
        throw socket_error(errMess);
    }
}


