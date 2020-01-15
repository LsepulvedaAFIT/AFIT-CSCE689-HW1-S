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
   void sendCommandPrompt(int socket);

   std::string getClientIP(const int inputSocketFD);
   std::string getClientPort(const int inputSocketFD);

   void printDisconnectedClientInfo(const int sd);
   void checkForIntCommand(char *readCommand, int socket);

private:
   //stores socket file descriptor
   int socketFD = 0;

   //data structure needed for bind & accept functions
   struct sockaddr_in address; 

   std::vector<int> client_sockets; 


};


#endif
