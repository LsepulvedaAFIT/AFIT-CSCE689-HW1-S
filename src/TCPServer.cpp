#include "TCPServer.h"

//STL
#include <iostream>
#include <unistd.h>
#include <vector>
#include <string.h>
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

#define MAX_CLIENTS 2


TCPServer::TCPServer() {
    //initializes client vector to zeros;
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        this->client_sockets.push_back(0);
    }
}


TCPServer::~TCPServer() {
}

/**********************************************************************************************
 * bindSvr - Creates a network socket and sets it nonblocking so we can loop through looking for
 *           data. Then binds it to the ip address and port
 *
 *    Throws: socket_error for recoverable errors, runtime_error for unrecoverable types
 **********************************************************************************************/

void TCPServer::bindSvr(const char *ip_addr, short unsigned int port) {
    
    //reference: https://www.geeksforgeeks.org/socket-programming-cc/
    //creates socket: socketFD -> File Handler
    //Domain: AF_INET -> IPv4 Protocol
    //Type: SOCK_STREAM -> TCP Connection (relicalbe two-way connection)
    //Protocol: 0 -> default Internet Protocol
    this->socketFD = socket(AF_INET, SOCK_STREAM, 0);
    //std::cout << "socket: " << this->socket_fd << "\n"; //testing
    //this->socketFD = -1; //error testing
    //making sure socket was create without errors
    errorCheck(this->socketFD, "Server socket failed");
    /********************************************************************
    * The variable serv_addr is a structure of type struct sockaddr_in. 
    * This structure has four fields. The first field is short sin_family, 
    * which contains a code for the address family. It should always be 
    * set to the symbolic constant AF_INET. 
    *********************************************************************/
    this->address.sin_family = AF_INET;

    /********************************************************************
    * The third field of sockaddr_in is a structure of type struct in_addr 
    * which contains only a single field unsigned long s_addr. This field 
    * contains the IP address of the host. For server code, this will always
    * be the IP address of the machine on which the server is running, and 
    * there is a symbolic constant INADDR_ANY which gets this address. 
    *********************************************************************/
    //this->address.sin_addr.s_addr = INADDR_ANY;
    this->address.sin_addr.s_addr = inet_addr( ip_addr ); 

    /********************************************************************
    * The second field of serv_addr is unsigned short sin_port , which
    * contain the port number. However, instead of simply copying the port
    * number to this field, it is necessary to convert this to network byte 
    * order using the function htons() which converts a port number in host 
    * byte order to a port number in network byte order.
    *********************************************************************/
    this->address.sin_port = htons( port );

    //c prog method
    //int bindCheck = bind(this->socketFD, (struct sockaddr *)&address, sizeof(address) );
    //c++ method
    int bindCheck = bind(this->socketFD, reinterpret_cast<struct sockaddr *>(&address), sizeof(address) );
    //error checking
    errorCheck(bindCheck, "Server bind failed");

    //Non-blocking declaration
    //fcntl: manipulate file descriptor
    //cmd: F_SETFL -> set file status flag
    //flag: o_NONBLOCK -> non-blocking socket
    fcntl(this->socketFD, F_SETFL, O_NONBLOCK);
   
}

/**********************************************************************************************
 * listenSvr - Performs a loop to look for connections and create TCPConn objects to handle
 *             them. Also loops through the list of connections and handles data received and
 *             sending of data. 
 *
 *    Throws: socket_error for recoverable errors, runtime_error for unrecoverable types
 **********************************************************************************************/

void TCPServer::listenSvr() {
    int max_sd = 0;
    
    //buffer for read and write communications
    char buffer[1024] = {0}; 

    //sets socket to listen with max queue size of 3
    int lisCheck = listen(this->socketFD, 3);
    //checks for errors
    errorCheck(lisCheck, "Server listen failed");
    //sets the size for addrlen to pass as a parameter
    //into socket accept function
    int addrlen = sizeof(address);

    //container for select function
    fd_set readset;
    
    while(true)
    {
        FD_ZERO(&readset);
        FD_SET(this->socketFD, &readset);
        int max_sd = this->socketFD;
        int sd = 0;

        for (int i = 0; i < MAX_CLIENTS; i++){
            sd = client_sockets.at(i);
            if(sd > 0)
            {   
                FD_SET( sd , &readset);   
            }    
            //highest file descriptor number, need it for the select function  
            if(sd > max_sd)
            {
                max_sd = sd;
            }
        }

        int activity = select( max_sd + 1 , &readset , NULL , NULL , NULL);   

        if ((activity < 0) && (errno!=EINTR))
        {
            std::cout << "error with select function\n";
        }

        if (FD_ISSET(this->socketFD, &readset))
        {
            int setSocket = accept(this->socketFD, reinterpret_cast<struct sockaddr *>(&address), reinterpret_cast<socklen_t*>(&addrlen));
            errorCheck(setSocket, "Server acceot failed");

            std::cout << "New connection created: socket " << setSocket << std::endl;
            
            const char* helloMessage = "Hello Client!\n\nMENU\n1: Current IP Address\n2: Current Port\n3: displays graphic\n4: displays graphic\n5: displays graphic\npasswd: Change Password\nexit: Disconnect From Server\nmenu: Displays Menu\n";
            send(setSocket, helloMessage, strlen(helloMessage) , 0);
            sendCommandPrompt(setSocket);

            std::cout << "Hello message sent to socket: " << setSocket << std::endl; 

            for (int i = 0; i < MAX_CLIENTS; i++)   
            {   
                //if position is empty  
                if( client_sockets.at(i) == 0 )   
                {   
                    //add new client to vector
                    client_sockets.at(i) = setSocket;   
                    printf("Adding to list of sockets as %d\n" , i);   
                         
                    break;   
                }   
            }
        }
        
        for (int i = 0; i < MAX_CLIENTS; i++)   
        {   
            sd = client_sockets.at(i);   
                
            if (FD_ISSET( sd , &readset))   
            {   
                //Check if it was for closing , and also read the  
                //incoming message  
                int valread = read( sd , buffer, 1024);
                if (valread == 0)   
                {   
                    //Somebody disconnected , get his details and print  
                    printDisconnectedClientInfo(sd);
                        
                    //Close the socket and mark as 0 in list for reuse  
                    close( sd );   
                    client_sockets.at(i) = 0;   
                }
                    
                //Echo back the message that came in  
                else 
                {   
                    //set the string terminating NULL byte on the end  
                    //of the data read  
                    buffer[valread] = '\0';
                    std::cout << "socket "<< sd << ": " << buffer << "\n";//testing
                    
                    std::string readCommandStr(buffer);
                    clrNewlines(readCommandStr);
                    if (readCommandStr == "hello")
                    {
                        std::string helloMessage = "(>n_n)> Hello Client\n";
                        const char* helloCharStar = helloMessage.c_str();
                        send(sd , helloCharStar, strlen(helloCharStar) , 0 );
                    }
                    else if (readCommandStr == "exit")
                    {
                        close( sd );   
                        client_sockets.at(i) = 0;   
                    }
                    else if (readCommandStr == "passwd")
                    {
                        std::string pwMessage = "TODO: Implement in HW2\n";
                        const char* pwCharStar = pwMessage.c_str();
                        send(sd , pwCharStar, strlen(pwCharStar) , 0 );  
                    }
                    else if (readCommandStr == "menu")
                    {
                        std::string menu = "MENU\n1: Current IP Address\n2: Current Port\n3: displays graphic\n4: displays graphic\n5: displays graphic\npasswd: Change Password\nexit: Disconnect from Server\nmenu: displays Menu\n";
                        const char* menuCharStar = menu.c_str();
                        send(sd , menuCharStar, strlen(menuCharStar) , 0 );  
                    }
                    else
                    {
                        checkForIntCommand(buffer, sd);
                    }
                    sendCommandPrompt(sd);
                }   
            }   
        }  
    }
}

/**********************************************************************************************
 * shutdown - Cleanly closes the socket FD.
 *
 *    Throws: socket_error for recoverable errors, runtime_error for unrecoverable types
 **********************************************************************************************/

void TCPServer::shutdown() {
    //closes the socket
    close(this->socketFD);
}

void TCPServer::errorCheck(int input, std::string errMess){
    if (input < 0)
    {
        throw socket_error(errMess);
    }
}

void TCPServer::sendCommandPrompt(int socket){
    const char* cmdPromptMessage = "\nCOMMAND:";
    send(socket, cmdPromptMessage, strlen(cmdPromptMessage) , 0);
}

std::string TCPServer::getClientIP(int inputSocketFD)
{
    int addrlen = sizeof(this->address);
    getpeername(inputSocketFD, (struct sockaddr*)&address , (socklen_t*)&addrlen);   
    //printf("Host disconnected , ip %s , port %d \n" , inet_ntoa(address.sin_addr) , ntohs(address.sin_port)); 
    std::stringstream ss;
    ss << inet_ntoa(address.sin_addr);
    return ss.str();
}

void TCPServer::printDisconnectedClientInfo(const int sd)
{
    int addrlen = sizeof(this->address);
    getpeername(sd , (struct sockaddr*)&address , (socklen_t*)&addrlen);   
    std::cout << "Client disconnected , ip " << getClientIP(sd) << ", port " << ntohs(address.sin_port) << std::endl;;      
}

std::string TCPServer::getClientPort(const int inputSocketFD)
{
    int addrlen = sizeof(this->address);
    getpeername(inputSocketFD, (struct sockaddr*)&address , (socklen_t*)&addrlen);   
    printf("Host disconnected , ip %s , port %d \n" , inet_ntoa(address.sin_addr) , ntohs(address.sin_port)); 
    std::stringstream ss;
    ss << ntohs(address.sin_port);
    return ss.str();
}


void TCPServer::checkForIntCommand(char *inputCommand, int socket){
    int readCommandInt = atoi(inputCommand);
    switch (readCommandInt)
    {
        case 1:
        {
            std::string clientIP = getClientIP(socket) + '\n';
            const char* sendIPBuffer = clientIP.c_str();
            send(socket, sendIPBuffer, strlen(sendIPBuffer) , 0 );
            break;
        }
        case 2:
        {
            std::string clientPort = getClientPort(socket) + '\n';
            const char* sendIPBuffer = clientPort.c_str();
            send(socket , sendIPBuffer, strlen(sendIPBuffer) , 0 );
            break;
        }
        case 3:
        {
            std::string cmd3Message = "__m_OO_m__\n";
            const char* sendIPBuffer = cmd3Message.c_str();
            send(socket , sendIPBuffer, strlen(sendIPBuffer) , 0 );
            break;
        }
        case 4:
        {
            std::string cmd4Message = "m_(-___-)_m\n";
            const char* sendIPBuffer = cmd4Message.c_str();
            send(socket, sendIPBuffer, strlen(sendIPBuffer) , 0 );
            break;
        }
        case 5:
        {
            std::string cmd5Message = "d[ o_O ]b\n";
            const char* sendIPBuffer = cmd5Message.c_str();
            send(socket , sendIPBuffer, strlen(sendIPBuffer) , 0 );
            break;
        }
        default:
        {
            std::stringstream ssTemp;
            ssTemp << "Unknown Command: " << inputCommand << "\n";
            std::string uknMessStr = ssTemp.str();
            const char* uknMessChar = uknMessStr.c_str();
            send(socket , uknMessChar , strlen(uknMessChar) , 0 );
        }
    }
}