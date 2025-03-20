#pragma once

#include "TCP.hpp"
#include "Socket.hpp"
#include <fstream>

namespace TCP
{
	class Client
	{
	public:
		Client()
		{

		}
		Client(const SOCKET& c_fd)
		{
			s_fd = c_fd;
			state = ConnectionStates::Connected;
		}
		
		Client(const char* host, int port)
		{
			s_fd = Socket::initSocket();
			sockAddr = Socket::initAddr(host, port);
		}

		Client(const struct sockaddr_in& addr) : sockAddr(addr)
		{
			s_fd = Socket::initSocket();
		}

		~Client()
		{
		}
		
		int Connect()
		{
			if (state == ConnectionStates::Closed)
			{
				s_fd = Socket::initSocket();
				state = ConnectionStates::Disconnected;
			}
			int result = Socket::sockConn(&s_fd, &sockAddr);
			if (!result)
			{
				ioctlsocket(s_fd, FIONBIO, (u_long*)&Socket::OptVal);
				state = ConnectionStates::Connected;
			}
			else
				state = ConnectionStates::Disconnected;
		
			return result;
		}
		
		ssize_t Recv(const size_t bufSize, bool block = false, void* buffer = nullptr)
		{
			if (state != ConnectionStates::Connected)
				return -1;
			ssize_t len;
			size_t recvd = 0;
			bufLen = 0;
			if (!buffer)
				buffer = buf;
			
			do
			{
				len = recv(s_fd, (char*)buffer + recvd, bufSize - recvd, 0);
				if (len == (ssize_t)-1)
				{	
					if (Socket::SocketShouldClose())
					{
						Close();
						return len;
					}
					len = 0;
				}
				else if (len == 0)
				{
					Close();
					return 0;
				}
				
				recvd += len;
				bufLen += len;
			} while (block && recvd < bufSize);
			buf[bufLen] = '\0';
			return len;
		}
		
		ssize_t Send(const char* data, size_t len, bool block = true)
		{
			if (state != ConnectionStates::Connected)
				return -1;
			ssize_t result;
			size_t sent = 0;
			do
			{
				result = send(s_fd, &data[sent], len - sent, 0);
				if (result == (ssize_t)-1)
				{
					if (Socket::SocketShouldClose())
					{
						Close();
						break;
					}
				}
				else
				{
					sent += (size_t)result;
				}
			} while (block && sent < len);
			return result;
		}

		void Disconnect()
		{
			Close();
			state = ConnectionStates::Disconnected;
			s_fd = Socket::initSocket();
		}
		
		int Close()
		{
			closesocket(s_fd);
			state = ConnectionStates::Closed;
			return 0;
		}

		struct FileInfo
		{
			char name[4096];
			uint32_t namePrefix;
			uint64_t size;
		};
		typedef void(*ValueCallback)(int, void*);
		int SendFile(std::string filename, ValueCallback callback = nullptr, void* sender = nullptr, ssize_t totalSize = 0, uint32_t namePrefix = 0)
		{
			std::vector<char> data(BUFFER_SIZE);
			std::fstream fs(filename, std::ios_base::in | std::ios_base::ate | std::ios_base::binary);
			ssize_t filesize = fs.tellg();
			if (!totalSize)
				totalSize = filesize;
			FileInfo info;
			memcpy(info.name, filename.c_str(), filename.length() + 1);
			info.namePrefix = namePrefix;
			info.size = filesize;
			if (Send((char*)&info, sizeof(info), true) == -1)
				return -1;
			fs.seekg(0);
			for (ssize_t sent = 0; sent < filesize;)
			{
				fs.read(data.data(), data.size());
				ssize_t result;
				if ((result = Send(data.data(), fs.gcount(), true)) == -1)
					return -1;
				sent += fs.gcount();
				if (callback)
					callback(sent * 100 / totalSize, sender);
				else
					printf("%ld\n", sent * 100 / totalSize);
			}

			return 0;
		}

		int RecvFile(std::string& path, ValueCallback callback = nullptr, void* sender = nullptr, ssize_t totalSize = 0)
		{
			FileInfo info;
			Recv(sizeof(FileInfo), true, &info);
			if (state != TCP::ConnectionStates::Connected)
				return -1;
			if (!totalSize)
				totalSize = info.size;
			if (path.empty())
				path = ".";
			if (path.back() != '/')
				path += '/';
			path += &info.name[info.namePrefix];

			std::string dir = path.substr(0, path.find_last_of('/'));
			struct stat sb;
			if (stat(dir.c_str(), &sb) != 0)
				pclose(popen(("mkdir -p " + dir).c_str(), "r"));
			std::fstream fs(path, std::ios_base::out | std::ios_base::binary);
			ssize_t cur;
			for (size_t recvd = 0, remaining = info.size, bufferSize; recvd < info.size; )
			{
				bufferSize = std::min(remaining, (size_t)BUFFER_SIZE);
				while ((cur = Recv(bufferSize)) == (ssize_t)-1)
				{
					if (Socket::SocketShouldClose())
						return -1;
				}
				fs.write(GetBuffer(), cur);
				recvd += cur;
				remaining -= cur;
				if (callback)
					callback(recvd * 100 / totalSize, sender);
				else
				 	printf("%ld\n", recvd * 100 / totalSize);
			}

			return 0;
		}

		const char* GetBuffer()
		{
			return buf;
		}
		const int GetBufferLength()
		{
			return bufLen;
		}
		const ConnectionStates GetState()
		{
			return state;
		}
		SOCKET& GetSocket()
		{
			return s_fd;
		}
	protected:
		SOCKET s_fd;
		sockaddr_in sockAddr;

		ConnectionStates state = ConnectionStates::Disconnected;
		char buf[BUFFER_SIZE + 1] = "";
		ssize_t bufLen = 0;
	};
}

namespace UDP
{
	class Client
	{
	public:
		Client()
		{
		}
		Client(const SOCKET& c_fd)
		{
			s_fd = c_fd;
			state = TCP::ConnectionStates::Connected;
		}

		Client(const SOCKET& c_fd, const sockaddr_in& addr)
		{
			s_fd = c_fd;
			sockAddr = addr;
			state = TCP::ConnectionStates::Connected;
		}
		
		Client(const char* host, int port)
		{
			s_fd = Socket::initSocket(AF_INET, SOCK_DGRAM);
			sockAddr = Socket::initAddr(host, port);
		}
		
		~Client()
		{
		}
		
		size_t Recv(const int bufSize)
		{
			if (state == TCP::ConnectionStates::Closed)
				return -1;
			size_t len = recvfrom(s_fd, buf, bufSize, 0, (struct sockaddr*)&sourceAddr, &sourceAddrLen);
			if (!len || Socket::SocketShouldClose())
				Close();
			bufLen = len == (size_t)-1 ? 0 : len;
			buf[bufLen] = '\0';
		
			return len;
		}
		
		int Send(const char* data, size_t len)
		{
			if (state == TCP::ConnectionStates::Closed)
				return -1;
			int result = sendto(s_fd, data, len, 0, (struct sockaddr*)&sockAddr, sizeof(sockAddr));
			if (Socket::SocketShouldClose())
				Close();
		
			return result;
		}

		void Connect()
		{
			state = TCP::ConnectionStates::Connected;
		}
		
		int Close()
		{
			closesocket(s_fd);
			state = TCP::ConnectionStates::Closed;
			return 0;
		}

		void Disconnect()
		{
			state = TCP::ConnectionStates::Disconnected;
		}

		const char* GetBuffer()
		{
			return buf;
		}
		const int GetBufferLength()
		{
			return bufLen;
		}
		const TCP::ConnectionStates GetState()
		{
			return state;
		}
		SOCKET& GetSocket()
		{
			return s_fd;
		}
		struct sockaddr_in GetSourceAddr()
		{
			return sourceAddr;
		}
	protected:
		SOCKET s_fd;
		sockaddr_in sockAddr;
		sockaddr_in sourceAddr;
		socklen_t sourceAddrLen = sizeof(sourceAddr);

		TCP::ConnectionStates state = TCP::ConnectionStates::Disconnected;
		char buf[BUFFER_SIZE + 1] = "";
		int bufLen = 0;
	};
}