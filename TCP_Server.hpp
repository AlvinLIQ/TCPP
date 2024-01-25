#pragma once

#include "TCP.hpp"
#include "TCP_Client.hpp"
#include "Socket.hpp"

namespace TCP
{
	class TCP_Server
	{
	public:
		TCP_Server(const char* ip, int port);
		~TCP_Server();

		void Listen(void(*ConnectionCallback)(TCP_Client* client), bool sync = false);
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
		std::vector<TCP_Client> connections{};

		ServerStates state = ServerStates::Stopped;
		std::thread* pServerThread = nullptr;
	};
}