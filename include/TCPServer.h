#ifndef TCPSERVER_H
#define TCPSERVER_H

#include "Server.h"

#include <netinet/in.h>

class TCPServer : public Server 
{
public:
   TCPServer();
   ~TCPServer();

   void bindSvr(const char *ip_addr, unsigned short port);
   void listenSvr();
   void shutdown();

private:
   //stores socket file descriptor
   int socketFD = 0;

   //data structure needed for bind & accept functions
    struct sockaddr_in address; 



};


#endif
