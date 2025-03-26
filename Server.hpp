#pragma once

#include "TCP.hpp"
#include "Client.hpp"
#include "Socket.hpp"
#include <thread>
#include <mutex>

#define DEFAULT_MAX_CLIENT_COUNT 32

namespace TCP
{
	class Server
	{
	public:
		Server()
		{

		}

		Server(Server&& server) noexcept
		{
			s_fd = server.s_fd;
			pServerThread = server.pServerThread;
			connections = server.connections;
			MaxClientCount = server.MaxClientCount;
			state = server.state;
			serverAddr = server.serverAddr;
			server.pServerThread = nullptr;
			server.s_fd = (SOCKET)-1;
			server.connections.clear();
		}

		Server &operator=(Server&& server) noexcept
		{
			if (&server != this)
			{
				s_fd = server.s_fd;
				pServerThread = server.pServerThread;
				connections = server.connections;
				MaxClientCount = server.MaxClientCount;
				state = server.state;
				serverAddr = server.serverAddr;
				server.pServerThread = nullptr;
				server.s_fd = (SOCKET)-1;
				server.connections.clear();
			}

			return *this;
		}

		Server(const char* ip, int port)
		{
			s_fd = Socket::initSocket();
			serverAddr = Socket::initAddr(ip, port);
		}
		
		~Server()
		{
			Close();
		}
		void Listen(void(*ConnectionCallback)(Client* client, void* sender), void* sender, bool sync)
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
				pServerThread = new std::thread([this, sender, ConnectionCallback]()
				{
					Listen(ConnectionCallback, sender, true);
				});
				return;
			}
			setsockopt(s_fd, SOL_SOCKET, SO_REUSEADDR, (void*)&Socket::OptVal, sizeof(Socket::OptVal));
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
					connections.push_back(Client(c_fd));
					//std::thread([ConnectionCallback, this, conn = connections.rbegin()]()
					//{
					//ConnectionCallback(c_fd);
						//connections.erase(conn.base());
					//}).detach();
				}
				for (auto conn = connections.begin(); conn < connections.end(); conn++)
				{
					ConnectionCallback((Client*)&(*conn), sender);
					if (conn->GetState() != Connected)
					{
						_mutex.lock();
						connections.erase(conn);
						_mutex.unlock();
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

		void Close()
		{
			Stop();
			while (!connections.empty())
			{
				_mutex.lock();
				connections.begin()->Close();
				connections.erase(connections.begin());
				_mutex.unlock();
			}
			if (pServerThread != nullptr)
			{
				if (pServerThread->joinable())
					pServerThread->join();
				delete pServerThread;
				pServerThread = nullptr;
			}
			closesocket(s_fd);
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
		std::vector<Client> connections{};
		std::mutex _mutex;

		ServerStates state = ServerStates::Stopped;
		std::thread* pServerThread = nullptr;
	};
}

namespace UDP
{
	class Server
	{
	public:
		Server()
		{
			
		}
		Server(const char* ip, int port)
		{
			s_fd = Socket::initSocket(AF_INET, SOCK_DGRAM);
			serverAddr = Socket::initAddr(ip, port);
			conn = Client(s_fd);
		}
		
		~Server()
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
		}
		void Listen()
		{
			setsockopt(s_fd, SOL_SOCKET, SO_REUSEADDR, &Socket::OptVal, sizeof(Socket::OptVal));
			ioctlsocket(s_fd, FIONBIO, (u_long*)&Socket::OptVal);
			socklen_t addrLen = sizeof(serverAddr);
			if (bind(s_fd, (struct sockaddr*)&serverAddr, addrLen) == -1)
				return;
			
			state = TCP::ServerStates::Listening;
		}

		void Stop()
		{
			state = TCP::ServerStates::Stopped;
		}

		const SOCKET& GetSocket()
		{
			return s_fd;
		}
		const sockaddr_in& GetServerAddress()
		{
			return serverAddr;
		}
		Client& GetConnection()
		{
			return conn;
		}
	private:
		SOCKET s_fd;
		sockaddr_in serverAddr;

		Client conn;
		TCP::ServerStates state = TCP::ServerStates::Stopped;
		std::thread* pServerThread = nullptr;
	};
}