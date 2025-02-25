#pragma once

#include "TCP.hpp"
#include "Socket.hpp"
#include <fstream>

namespace TCP
{
	class TCP_Client
	{
	public:
		TCP_Client(const SOCKET& c_fd)
		{
			s_fd = c_fd;
			state = ConnectionStates::Connected;
		}
		
		TCP_Client(const char* host, int port)
		{
			s_fd = Socket::initSocket();
			sockAddr = Socket::initAddr(host, port);
		}
		
		~TCP_Client()
		{
		}
		
		int Connect()
		{
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
			int len = recv(s_fd, buf, bufSize, 0);
			if (!len || Socket::SocketShouldClose())
				Close();
			bufLen = len <= 0 ? 0 : len;
			buf[bufLen] = '\0';
		
			return len;
		}
		
		int Send(const char* data, size_t len)
		{
			int result = send(s_fd, data, len, 0);
			if (Socket::SocketShouldClose())
				Close();
		
			return result;
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
			size_t size;
		};

		int SendFile(std::string filename, TCP::TCP_Client& client, int& progress)
		{
			std::vector<char> data(TCP_BUFFER_SIZE);
			std::fstream fs(filename, std::ios_base::in | std::ios_base::ate | std::ios_base::binary);
			size_t filesize = fs.tellg();
			FileInfo info;
			memcpy(info.name, filename.c_str(), filename.length() + 1);
			info.size = filesize;
			if (Send((char*)&info, sizeof(info)) == -1)
				return -1;

			size_t remaining = filesize;
			fs.seekg(0);
			while (remaining > TCP_BUFFER_SIZE)
			{
				fs.read(data.data(), TCP_BUFFER_SIZE);
				if (Send(data.data(), TCP_BUFFER_SIZE) == -1)
					return -1;
				remaining -= TCP_BUFFER_SIZE;
				progress = remaining * 100 / filesize;
				printf("progress:%d\r", progress);
			}
			fs.read(data.data(), remaining);
			return Send(data.data(), remaining);
		}

		int RecvFile(std::string& path, int& progress)
		{
			if (Recv(sizeof(FileInfo)) == -1)
				return -1;
			FileInfo info = *(FileInfo*)GetBuffer();
			if (path.empty())
				path = ".";
			if (path.back() == '/')
				path += '/';
			path += info.name;
			std::fstream fs(path, std::ios_base::out | std::ios_base::binary);

			size_t remaining = info.size;
			while (remaining > TCP_BUFFER_SIZE)
			{
				if (Recv(TCP_BUFFER_SIZE) == -1)
					return -1;
				fs.write(GetBuffer(), GetBufferLength());
				remaining -= GetBufferLength();
			}
			if (Recv(remaining) == -1)
					return -1;
			fs.write(GetBuffer(), remaining);
			fs.close();

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
		char buf[TCP_BUFFER_SIZE + 1] = "";
		int bufLen = 0;
	};
}