#ifndef TCPSERVER_H
#define TCPSERVER_H

#include "Server.h"

#include <netinet/in.h>
#include <string>
#include <vector>


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

   std::vector<int> client_sockets;
   std::vector<std::string> client_commands;


};


#endif
