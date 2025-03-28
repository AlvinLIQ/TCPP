#pragma once

#include "TCP.hpp"
#include "Socket.hpp"
#include <fstream>

#define TCP_CONNECTION_CLOSED 8
#define TCP_FAILED_TO_CREATE_OR_OPEN_FILE 4
#define TCP_FAILED_TO_CREATE_DIR 2
#define TCP_FAILED_TO_SEND 1
#define TCP_SUCCEED 0

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
			if (buffer == buf)
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
		typedef void(*StringCallback)(std::string, void*);
		int SendFile(std::string filename, ValueCallback callback = nullptr, void* sender = nullptr, ssize_t totalSize = 0, uint32_t namePrefix = 0, ssize_t filesize = 0, size_t progress = 0)
		{
			int status = TCP_SUCCEED;
			std::vector<char> data(BUFFER_SIZE);
			std::ifstream fs(filename, std::ios_base::in | std::ios_base::binary);
			if (fs.fail())
			{
				status |= TCP_FAILED_TO_CREATE_OR_OPEN_FILE;
				filesize = 0;
			}
			else if (!filesize)
			{
				fs.seekg(0L, std::ios::end);
				filesize = fs.tellg();
				fs.seekg(0L, std::ios::beg);
			}
			if (!totalSize)
				totalSize = filesize;
			FileInfo info;
			memcpy(info.name, filename.c_str(), filename.length() + 1);
			info.namePrefix = namePrefix;
			info.size = filesize;
			if (Send((char*)&info, sizeof(info), true) == -1)
				return -1;
			for (ssize_t sent = 0; sent < filesize;)
			{
				fs.read(data.data(), data.size());
				ssize_t result;
				if ((result = Send(data.data(), fs.gcount(), true)) == -1)
					return TCP_FAILED_TO_SEND;
				sent += fs.gcount();
				if (callback)
					callback(progress + sent * 100 / totalSize, sender);
				else
					printf("%ld\n", progress + sent * 100 / totalSize);
			}

			return status;
		}

		int RecvFile(std::string& path, ValueCallback callback = nullptr, StringCallback nameCallback = nullptr, void* sender = nullptr, ssize_t totalSize = 0, size_t progress = 0)
		{
			int status = TCP_SUCCEED;
			FileInfo info;
			Recv(sizeof(FileInfo), true, &info);
			if (state != TCP::ConnectionStates::Connected)
				return TCP_CONNECTION_CLOSED;

			if (!totalSize)
				totalSize = info.size;
			if (path.empty())
				path = ".";
			if (path.back() != '/' && info.name[info.namePrefix] != '/')
				path += '/';
			path += &info.name[info.namePrefix];
			if (nameCallback)
				nameCallback(path, sender);

			std::string dir = path.substr(0, path.find_last_of('/'));
			struct stat sb;
			if (stat(dir.c_str(), &sb) != 0)
			{
				system(("mkdir -p \"" + dir + "\"").c_str());
				status |= TCP_FAILED_TO_CREATE_DIR;
			}
			std::ofstream fs(path, std::ios_base::out | std::ios_base::binary);
			if (fs.fail())
				status |= TCP_FAILED_TO_CREATE_OR_OPEN_FILE;
			ssize_t cur;
			for (size_t recvd = 0, remaining = info.size, bufferSize; recvd < info.size; )
			{
				bufferSize = std::min(remaining, (size_t)BUFFER_SIZE);
				while ((cur = Recv(bufferSize)) == (ssize_t)-1)
				{
					if (Socket::SocketShouldClose())
						return TCP_FAILED_TO_SEND;
				}
				fs.write(GetBuffer(), cur);
				recvd += cur;
				remaining -= cur;
				if (callback)
					callback(progress + recvd * 100 / totalSize, sender);
				else
				 	printf("%ld\n", progress + recvd * 100 / totalSize);
			}

			return status;
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