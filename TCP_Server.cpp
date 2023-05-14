#include "TCP_Server.hpp"
#include <memory>

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
void TCP_Server::Listen(void(*ConnectionCallback)(SOCKET c_fd))
{
	setsockopt(s_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&OptVal, sizeof(char));
	ioctlsocket(s_fd, FIONBIO, &OptVal);
	socklen_t addrLen = sizeof(serverAddr);
	if (listenSocket(s_fd, &serverAddr, addrLen) == -1)
		return;
	
	state = ServerStates::Listening;
	SOCKET c_fd;
	while (state == ServerStates::Listening)
	{
		while ((c_fd = accept(s_fd, (struct sockaddr*)&serverAddr, &addrLen)) != (SOCKET)-1)
		{
			Connection conn{c_fd, std::thread(ConnectionCallback, c_fd)};
			conn.connectionThread.detach();
			connections.push_back(std::move(conn));
		}
	}
	state = ServerStates::Stopped;
}

void TCP_Server::Stop()
{
	state = ServerStates::Stopped;
}