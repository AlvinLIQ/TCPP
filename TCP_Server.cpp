#include "TCP_Server.hpp"

using namespace TCP;
using namespace Socket;

TCP_Server::TCP_Server(const char* ip, int port)
{
	s_fd = initSocket();
	serverAddr = initAddr(ip, port);
}

TCP_Server::~TCP_Server()
{
	Stop();
	if (pServerThread != nullptr)
	{
		if (pServerThread->joinable())
			pServerThread->join();
		delete pServerThread;
		pServerThread = nullptr;
	}
	closesocket(s_fd);
	while (!connections.empty())
	{
		connections.begin()->Close();
		connections.erase(connections.begin());
	}
}
void TCP_Server::Listen(void(*ConnectionCallback)(TCP_Client* client), bool sync)
{
	if (!sync)
	{
		if (pServerThread != nullptr)
		{
			if (pServerThread->joinable())
				pServerThread->join();
			delete pServerThread;
			pServerThread = nullptr;
		}
		pServerThread = new std::thread([this, ConnectionCallback]()
		{
			Listen(ConnectionCallback, true);
		});
		return;
	}
	setsockopt(s_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&OptVal, sizeof(char));
	ioctlsocket(s_fd, FIONBIO, (u_long*)&OptVal);
	socklen_t addrLen = sizeof(serverAddr);
	if (listenSocket(s_fd, &serverAddr, addrLen) == -1)
		return;
	
	state = ServerStates::Listening;
	SOCKET c_fd;
	while (state == ServerStates::Listening)
	{
		while (connections.size() < MaxClientCount && (c_fd = accept(s_fd, (struct sockaddr*)&serverAddr, &addrLen)) != (SOCKET)-1)
		{
			connections.push_back(TCP_Client(c_fd));
			//std::thread([ConnectionCallback, this, conn = connections.rbegin()]()
			//{
			//ConnectionCallback(c_fd);
				//connections.erase(conn.base());
			//}).detach();
		}
		for (auto conn = connections.begin(); conn < connections.end(); conn++)
		{
			ConnectionCallback((TCP_Client*)&(*conn));
			if (conn->GetState() != Connected)
			{
				connections.erase(conn);
				break;
			}
		}
	}
	state = ServerStates::Stopped;
}

void TCP_Server::Stop()
{
	state = ServerStates::Stopped;
}