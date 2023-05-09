#pragma once

#include "TCP.hpp"
#include "Socket.hpp"

namespace TCP
{
	class TCP_Server
	{
	public:
		TCP_Server(const char* ip, int port);
		~TCP_Server();

		void Listen(void(*f)(SOCKET c_fd));
		void Stop();

		static void ExitSignal();

		const ConnectionStates GetConnectionState()
		{
			return state;
		}

		const SOCKET& GetSocket()
		{
			return s_fd;
		}
		const sockaddr_in& GetServerAddress()
		{
			return serverAddr;
		}

		const int& GetClientCount()
		{
			return clientCount;
		}
	private:
		SOCKET s_fd;
		sockaddr_in serverAddr;
		int clientCount = 0;

		ConnectionStates state;
	};
}