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

TCPServer::TCPServer() {

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
    //this->socket_fd = 0; //testing
    //making sure socket was create without errors
    if (this->socketFD < 0)
    {
        //TODO:Throw exception instead per function description
        std::cout << "socket failed\n"; 
        exit(EXIT_FAILURE);
    }
  
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
    this->address.sin_addr.s_addr = INADDR_ANY; 

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
    if (bindCheck < 0)
    {
        //TODO:Throw exception instead per function description
        std::cout << "bind failed\n"; 
        exit(EXIT_FAILURE); 
    }
    
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
    int MAX_CLIENTS = 2; //client can still connect & get greet but no two-way communication
    int max_sd = 0;
    //stores the client socket connections
    std::vector<int> client_sockets(MAX_CLIENTS,0); 
    //buffer for read and write communications
    char buffer[1024] = {0}; 

    //sets socket to listen with max queue size of 3
    int lisCheck = listen(this->socketFD, 3);
    //checks for errors
    if (lisCheck < 0)
    {
        //TODO:Throw exception instead per function description
        std::cout << "listen failed\n"; 
        exit(EXIT_FAILURE); 
    }
    
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
            //int setSocket = accept(this->socketFD, (struct sockaddr *)&address, (socklen_t*)&addrlen);
            int setSocket = accept(this->socketFD, reinterpret_cast<struct sockaddr *>(&address), reinterpret_cast<socklen_t*>(&addrlen));
            if (setSocket < 0)
            {
                //TODO:Throw exception instead per function description
                std::cout << "accept failed\n"; 
                exit(EXIT_FAILURE);
            }
            std::cout << "New connection created: socket " << setSocket << std::endl;
            
            //might need to change lineendings Windows uses "\r\n"
            const char* helloMessage = "Hello Client!\r\n\r\nMENU\r\n1: Username\r\n2: Password\r\n3: Change IP\r\n4: Change Port\r\n5: EXIT\r\n\r\nCOMMAND:";
            send(setSocket, helloMessage, strlen(helloMessage) , 0);

            std::cout << "Hello message sent to socket: " << setSocket << std::endl; 

            for (int i = 0; i < MAX_CLIENTS; i++)   
            {   
                //if position is empty  
                if( client_sockets.at(i) == 0 )   
                {   
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
                    getpeername(sd , (struct sockaddr*)&address , (socklen_t*)&addrlen);   
                    printf("Host disconnected , ip %s , port %d \n" , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));   
                        
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
                    if(atoi(buffer) == 5)
                    {
                        close(sd);
                        client_sockets.at(i) = 0;
                    }
                    else
                    {
                        send(sd , buffer , strlen(buffer) , 0 );//testing: echo recieved message
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
    //closes the socket
    close(this->socketFD);
}
