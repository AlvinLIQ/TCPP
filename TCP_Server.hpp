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

		void Listen(void(*ConnectionCallback)(SOCKET c_fd));
		void Stop();

		static void ExitSignal();

		const ServerStates GetConnectionState()
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

		const size_t GetConnectionsCount()
		{
			return connections.size();
		}
	private:
		SOCKET s_fd;
		sockaddr_in serverAddr;
		std::vector<SOCKET> connections{};

		ServerStates state;
	};
}