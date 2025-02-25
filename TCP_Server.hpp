#pragma once

#include "TCP.hpp"
#include "TCP_Client.hpp"
#include "Socket.hpp"
#include <thread>

#define DEFAULT_MAX_CLIENT_COUNT 32

namespace TCP
{
	class TCP_Server
	{
	public:
		TCP_Server(const char* ip, int port)
		{
			s_fd = Socket::initSocket();
			serverAddr = Socket::initAddr(ip, port);
		}
		
		~TCP_Server()
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
		void Listen(void(*ConnectionCallback)(TCP_Client* client), bool sync)
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
			setsockopt(s_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&Socket::OptVal, sizeof(char));
			ioctlsocket(s_fd, FIONBIO, (u_long*)&Socket::OptVal);
			socklen_t addrLen = sizeof(serverAddr);
			if (Socket::listenSocket(s_fd, &serverAddr, addrLen) == -1)
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
		
		void Stop()
		{
			state = ServerStates::Stopped;
		}

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

		size_t MaxClientCount = DEFAULT_MAX_CLIENT_COUNT;
	private:
		SOCKET s_fd;
		sockaddr_in serverAddr;
		std::vector<TCP_Client> connections{};

		ServerStates state = ServerStates::Stopped;
		std::thread* pServerThread = nullptr;
	};
}