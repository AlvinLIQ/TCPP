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
		
		int Recv(const int bufSize)
		{
			if (state != ConnectionStates::Connected)
				return -1;
			int len = recv(s_fd, buf, bufSize, 0);
			if (len < 0 && Socket::SocketShouldClose())
				Close();
			bufLen = len <= 0 ? 0 : len;
			buf[bufLen] = '\0';
		
			return len;
		}
		
		int Send(const char* data, size_t len)
		{
			if (state != ConnectionStates::Connected)
				return -1;
			int result = send(s_fd, data, len, 0);
			if (result < 0 && Socket::SocketShouldClose())
				Close();
		
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
			char name[256];
			uint32_t namePrefix;
			uint64_t size;
		};
		template<typename T>
		int SendFile(std::string filename, T object, uint32_t namePrefix = 0)
		{
			std::vector<char> data(BUFFER_SIZE);
			std::fstream fs(filename, std::ios_base::in | std::ios_base::ate | std::ios_base::binary);
			size_t filesize = fs.tellg();
			FileInfo info;
			memcpy(info.name, filename.c_str(), filename.length() + 1);
			info.namePrefix = namePrefix;
			info.size = filesize;
			if (Send((char*)&info, sizeof(info)) == -1)
				return -1;
			size_t remaining = filesize;
			size_t bufsize = 0;
			fs.seekg(0);
			for (size_t sent = 0; sent < filesize;)
			{
				bufsize = std::min(remaining, (size_t)BUFFER_SIZE);
				fs.read(data.data(), bufsize);
				if (Send(data.data(), bufsize) == -1)
					return -1;
				remaining -= bufsize;
				sent += BUFFER_SIZE;
				if (object)
					object->Progress(sent / filesize);
			}

			return 0;
		}
		template<typename T>
		int RecvFile(std::string& path, T object)
		{
			while(Recv(sizeof(FileInfo)) == -1)
				if (Socket::SocketShouldClose())
					return -1;
			FileInfo info = *(FileInfo*)GetBuffer();
			if (path.empty())
				path = ".";
			if (path.back() == '/')
				path += '/';
			path += &info.name[info.namePrefix];
			std::fstream fs(path, std::ios_base::out | std::ios_base::binary);
			int cur;
			for (size_t recvd = 0; recvd < info.size; )
			{
				while ((cur = Recv(BUFFER_SIZE)) < 0)
				{
					if (Socket::SocketShouldClose())
						return -1;
				}
				fs.write(GetBuffer(), cur);
				recvd += cur;
				if (object)
					object->Progress(recvd * 100 / info.size);
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
		int bufLen = 0;
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