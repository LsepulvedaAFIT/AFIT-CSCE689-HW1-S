#ifndef TCPSERVER_H
#define TCPSERVER_H

#include "Server.h"

#include <netinet/in.h>
#include <string>
#include <vector>
#include <memory>

//client socket object helps keep commands and sockets together for cleaner code
//this object is only used by TCPServer
class socket_obj
{
public:
   socket_obj();
   ~socket_obj();      
   int socketObjFD = 0;
   std::string command = "";

};

class TCPServer : public Server 
{
public:
   TCPServer();
   ~TCPServer();

   void bindSvr(const char *ip_addr, unsigned short port);
   void listenSvr();
   void shutdown();

   void errorCheck(int input, std::string errMess);

   std::string getClientIP(const int inputFD);
   std::string getClientPort(const int inputFD);

   void sendMessageToClient(int inputClientFD, std::string message);
   void closeClient(int inputClientFD, int index);
   void printDisconnectedClientInfo(const int sd);
   void checkForIntCommand(char *readCommand, int socket);

private:
   //stores socket file descriptor
   int socket_FD = 0;

   //data structure needed for bind & accept functions
   struct sockaddr_in address; 

   //testing
   std::vector<std::unique_ptr<socket_obj>> clientObj_sockets;

};

#endif
