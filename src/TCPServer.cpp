#include "TCPServer.h"

//STL
#include <iostream>
#include <unistd.h>
#include <vector>
#include <string.h>
#include <sstream>
#include <memory>
#include <algorithm>

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
    //creates and initializes client vector to the max number of clients
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        std::unique_ptr<socket_obj> newSocket = std::make_unique<socket_obj>();
        this->clientObj_sockets.push_back(std::move(newSocket));
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
    this->socket_FD = socket(AF_INET, SOCK_STREAM, 0);
    //std::cout << "socket: " << this->socket_fd << "\n"; //testing
    //this->socketFD = -1; //error testing
    //making sure socket was create without errors
    errorCheck(this->socket_FD, "Server socket failed");
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
    //this->address.sin_addr.s_addr = INADDR_ANY;//this connects to the system default IP
    this->address.sin_addr.s_addr = inet_addr( ip_addr ); 

    /********************************************************************
    * The second field of serv_addr is unsigned short sin_port , which
    * contain the port number. However, instead of simply copying the port
    * number to this field, it is necessary to convert this to network byte 
    * order using the function htons() which converts a port number in host 
    * byte order to a port number in network byte order.
    *********************************************************************/
    this->address.sin_port = htons( port );

    //c++ method which should be safer code
    int bindCheck = bind(this->socket_FD, reinterpret_cast<struct sockaddr *>(&this->address), sizeof(this->address) );
    //error checking
    errorCheck(bindCheck, "Server bind failed");

    //Non-blocking declaration
    //fcntl: manipulate file descriptor
    //cmd: F_SETFL -> set file status flag
    //flag: o_NONBLOCK -> non-blocking socket
    fcntl(this->socket_FD, F_SETFL, O_NONBLOCK);
   
}

/**********************************************************************************************
 * listenSvr - Performs a loop to look for connections and create TCPConn objects to handle
 *             them. Also loops through the list of connections and handles data received and
 *             sending of data. 
 *
 *    Throws: socket_error for recoverable errors, runtime_error for unrecoverable types
 **********************************************************************************************/

void TCPServer::listenSvr() {
    //index for largest file descriptor
    int maxFD = 0;
    
    //buffer for read and write communications
    char buffer[1024] = {0}; 

    //sets socket to listen with max queue size of 3
    int lisCheck = listen(this->socket_FD, 3);
    //checks for errors
    errorCheck(lisCheck, "Server listen failed");
    //sets the size for addrlen to pass as a parameter into socket accept function
    int addrLen = sizeof(this->address);

    //file descriptor set container for select function
    fd_set readSet;
    
    //main loop that continously reads and sends data 
    while(true)
    {
        //zeroizes readSet set
        FD_ZERO(&readSet);
        //add server socket to readSet
        FD_SET(this->socket_FD, &readSet);
        int maxFD  = this->socket_FD; //initializes max file descripter to server socket
        int currentClientFD = 0; //index while iterating through client

        //adds all exiting clients to readSet
        for (int i = 0; i < MAX_CLIENTS; i++){
            currentClientFD = this->clientObj_sockets.at(i)->socketObjFD;

            //checks if vector has client associated to that index
            if(currentClientFD > 0)
            {   
                //add client file descriptor to readset
                FD_SET( currentClientFD , &readSet);   
            }    
            //highest file descriptor number, need it for the select function  
            if(currentClientFD > maxFD )
            {
                maxFD = currentClientFD;
            }
        }
        //create timeval struct to pass into the last select option
        struct timeval timeOut;
        timeOut.tv_sec = 0;
        timeOut.tv_usec = 500000;

        //indicates which of the specified file descriptors is ready for reading, ready for writing, or has an error condition pending
        int activity = select( maxFD + 1 , &readSet , NULL , NULL , &timeOut);    

        //error checks the select function
        if ((activity < 0) && (errno!=EINTR))
        {
            std::cout << "error with select function\n";
        }

        //checks if any new clients have connected
        if (FD_ISSET(this->socket_FD, &readSet))
        {
            //accepts the connection and error check is conducted
            int setSocket = accept(this->socket_FD, reinterpret_cast<struct sockaddr *>(&this->address), reinterpret_cast<socklen_t*>(&addrLen));
            errorCheck(setSocket, "Server accept failed");

            //Server Admin Alert
            std::cout << "New connection created: socket " << setSocket << "\n";
            
            //Welcome message and menu
            sendMessageToClient(setSocket, "Hello Client!\n\nCOMMAND MENU\nhello: Welcome message\n1: Current IP Address\n2: Current Port\n3: Displays Graphic\n4: Displays Graphic\n5: Displays Graphic\npasswd: Change Password\nexit: Disconnect From Server\nmenu: Displays Menu\n\nCOMMAND:");

            //Server Admin Notification
            std::cout << "Hello message sent to socket: " << setSocket << "\n"; 

            //adds new client to vector
            for (int i = 0; i < MAX_CLIENTS; i++)   
            {   
                //find first position that empty
                if( this->clientObj_sockets.at(i)->socketObjFD == 0 ) 
                {   
                    //add new client socket to vector  
                    this->clientObj_sockets.at(i)->socketObjFD = setSocket;
                    std::cout << "Adding to list of sockets as " << i << "\n";   
                    break;   
                }   
            }
        }

        //iterates through client list to process commands
        for (int currentVectorIndex = 0; currentVectorIndex < MAX_CLIENTS; currentVectorIndex++)   
        {   
            //current client index
            currentClientFD = this->clientObj_sockets.at(currentVectorIndex)->socketObjFD;     

            //checks if client sent a command    
            if (FD_ISSET( currentClientFD , &readSet))   
            {   
                //Check if connection was lost; else reads the incoming message  
                int valRead = read( currentClientFD, buffer, 1024);
                if (valRead == 0)   
                {   
                    //Somebody disconnected , get his details and print  
                    printDisconnectedClientInfo(currentClientFD);
                        
                    //Close the socket and mark as 0 in list for reuse  
                    closeClient(currentClientFD, currentVectorIndex); 
                }
                else 
                {   
                    //set the string terminating NULL byte on the end of the data read  
                    buffer[valRead] = '\0';
                    //Alerting Admin of socket message
                    std::cout << "socket "<< currentClientFD << ": " << buffer;//testing
                    
                    //convert buffer arrar to string for better manipulation
                    std::string readCommandStr(buffer);

                    //adds message to command buffer
                    this->clientObj_sockets.at(currentVectorIndex)->command = this->clientObj_sockets.at(currentVectorIndex)->command + readCommandStr;

                    //if command is incomplete server will continue on to other socket and check this one again in the next iteration
                    size_t newlineCmdCount = std::count(this->clientObj_sockets.at(currentVectorIndex)->command.begin(), this->clientObj_sockets.at(currentVectorIndex)->command.end(), '\n');
                    //if (completeCmd < 1){
                    std::cout << "newlines: " << newlineCmdCount << std::endl;
                    if (newlineCmdCount < 1){
                        //Alert to Server Admin
                        std::cout << "partial cmd from client: " << currentClientFD << "\n";
                        continue;
                    }
                    //variables for multiple command processing
                    size_t pos = 0;
                    std::string token;
                    std::string delimiter = "\n";

                    //loops continue until all commands in a string are processed
                    while((newlineCmdCount > 0) && ((pos = this->clientObj_sockets.at(currentVectorIndex)->command.find(delimiter)) != std::string::npos))
                    {
                        //separates the 1st command from the string if multiple commands are sent at once
                        if (newlineCmdCount > 1)
                        {
                                readCommandStr = this->clientObj_sockets.at(currentVectorIndex)->command.substr(0, pos);
                                //std::cout << readCommandStr << std::endl;//testing
                                //erases the command to be processed for original string
                                this->clientObj_sockets.at(currentVectorIndex)->command.erase(0, pos + delimiter.length());
                        }
                        else{
                            //transfers complete command
                            readCommandStr = this->clientObj_sockets.at(currentVectorIndex)->command;
                            //clears command buffer to avoid errors
                            this->clientObj_sockets.at(currentVectorIndex)->command = "";
                        }

                        //clear away the newline character from command to ensure proper match
                        clrNewlines(readCommandStr);

                        //Sends Hello message
                        if (readCommandStr == "hello")
                        {
                            sendMessageToClient(currentClientFD, "(>n_n)> Hello Client\n\nCOMMAND:");
                        }
                        //closes client's connection
                        else if (readCommandStr == "exit")
                        {
                            closeClient(currentClientFD, currentVectorIndex);
                        }
                        //TODO: HW2
                        else if (readCommandStr == "passwd")
                        {
                            sendMessageToClient(currentClientFD, "TODO: Implement in HW2\n\nCOMMAND:");

                        }
                        //Displays menu
                        else if (readCommandStr == "menu")
                        {
                            sendMessageToClient(currentClientFD, "COMMAND MENU\nhello: Welcome message\n1: Current IP Address\n2: Current Port\n3: Displays Graphic\n4: Displays Graphic\n5: Displays Graphic\npasswd: Change Password\nexit: Disconnect From Server\nmenu: Displays Menu\n\nCOMMAND:");
                        }
                        //Checks if command was an int after string comparisons
                        else
                        {
                            checkForIntCommand(const_cast<char *>(readCommandStr.c_str()), currentClientFD);
                        }
                        newlineCmdCount--;
                    }
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
    //closes the client sockets
    for (int i = 0; i < MAX_CLIENTS; i++){
        //closeClient(this->client_sockets.at(i), i);
        closeClient(this->clientObj_sockets.at(i)->socketObjFD, i);
    }
    //closes server client
    close(this->socket_FD);
}

void TCPServer::closeClient(int inputClientFD, int index){
    std::cout << "Closing client socket: " << inputClientFD << "\n";
    //closes client
    close( inputClientFD );   
    //reset vector tracker
    //client_sockets.at(index) = 0; 
    this->clientObj_sockets.at(index)->socketObjFD = 0;
}

//Throws error if input < 0
void TCPServer::errorCheck(int input, std::string errMess){
    if (input < 0)
    {
        throw socket_error(errMess);
    }
}

void TCPServer::sendMessageToClient(int inputClientFD, std::string message){
    const char* messageCharStar = message.c_str();
    send(inputClientFD , messageCharStar, strlen(messageCharStar) , 0 );  
}

//Return the Client IP in a string
std::string TCPServer::getClientIP(const int inputFD)
{
    int addrLen = sizeof(this->address);
    getpeername(inputFD, reinterpret_cast<struct sockaddr *>(&this->address), reinterpret_cast<socklen_t*>(&addrLen)); 
    std::stringstream ss;
    ss << inet_ntoa(this->address.sin_addr);
    return ss.str();
}

//Displays disconnect info to console
void TCPServer::printDisconnectedClientInfo(const int inputFD)
{
    int addrLen = sizeof(this->address);
    getpeername(inputFD, reinterpret_cast<struct sockaddr *>(&this->address), reinterpret_cast<socklen_t*>(&addrLen)); 
    std::cout << "Client disconnected , ip " << getClientIP(inputFD) << ", port " << ntohs(this->address.sin_port) << std::endl;;      
}

//Return the Client Port in a string
std::string TCPServer::getClientPort(const int inputFD)
{
    int addrLen = sizeof(this->address);
    getpeername(inputFD, reinterpret_cast<struct sockaddr *>(&this->address), reinterpret_cast<socklen_t*>(&addrLen));  
    std::stringstream ss;
    ss << ntohs(this->address.sin_port);
    return ss.str();
}

//this function processess commands which are integers
//If command is not match, unknown command message is sent to client 
void TCPServer::checkForIntCommand(char *inputCommand, int socket){
    int readCommandInt = atoi(inputCommand);
    switch (readCommandInt)
    {
        case 1:
        {
            std::string clientIP = "Current IP: " + getClientIP(socket) + "\n\nCOMMAND:";
            sendMessageToClient(socket, clientIP);
            break;
        }
        case 2:
        {
            std::string clientPort = "Current Port: " + getClientPort(socket) + "\n\nCOMMAND:";
            sendMessageToClient(socket, clientPort);
            break;
        }
        case 3:
        {
            sendMessageToClient(socket, "__m_OO_m__\n\nCOMMAND:");
            break;
        }
        case 4:
        {
            sendMessageToClient(socket, "m_(-___-)_m\n\nCOMMAND:");
            break;
        }
        case 5:
        {
            sendMessageToClient(socket, "d[ o_O ]b\n\nCOMMAND:");
            break;
        }
        default:
        {
            std::string tempString(inputCommand);
            clrNewlines(tempString);
            std::stringstream ss;
            ss << "Unknown Command: \"" << tempString << "\"\n\nCOMMAND:";
            std::string unknownCmd = ss.str();
            sendMessageToClient( socket, unknownCmd );
        }
    }
}


socket_obj::socket_obj(){

}

socket_obj::~socket_obj(){
    
}