/*
Standard
*/

#pragma once

#include "RainWSA2Include.h"

#include <string>

namespace Rain
{
	int InitWinsock (WSADATA &wsaData);

	//client side functions
	int GetClientAddr (std::string host, std::string port, struct addrinfo **result);
	int CreateClientSocket (struct addrinfo **ptr, SOCKET &ConnectSocket);
	int ConnToServ (struct addrinfo **ptr, SOCKET &ConnectSocket); //never frees ptr; we might need it again

	int QuickClientInit (WSADATA &wsaData, std::string host, std::string port, struct addrinfo **paddr, SOCKET &connect);

	//server side functions
	int GetServAddr (std::string port, struct addrinfo **result);
	int CreateServLSocket (struct addrinfo **ptr, SOCKET &ListenSocket);
	int BindServLSocket (struct addrinfo **ptr, SOCKET &ListenSocket); //frees ptr
	int ListenServSocket (SOCKET &ListenSocket);
	int ServAcceptClient (SOCKET &ClientSocket, SOCKET &ListenSocket);

	int QuickServerInit (WSADATA &wsaData, std::string port, struct addrinfo **paddr, SOCKET &listener);

	//both sides can use this function
	int ShutdownSocketSend (SOCKET &ConnectSocket);

	//uility
	std::string GetClientNumIP (SOCKET &clientsock);
}