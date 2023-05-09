#include "TCP_Server.hpp"

using namespace TCP;
using namespace Socket;

short runningState = 1;

TCP::TCP_Server* currentTCPServer;

TCP_Server::TCP_Server(const char* ip, int port)
{
	s_fd = initSocket();
	serverAddr = initAddr(ip, port);

	currentTCPServer = this;
}

TCP_Server::~TCP_Server()
{
	closesocket(s_fd);
}
void TCP_Server::Listen(void(*f)(SOCKET c_fd))
{
	Server(f, &runningState, &clientCount, s_fd, serverAddr);
}

void TCP_Server::Stop()
{
	runningState = 0;
}