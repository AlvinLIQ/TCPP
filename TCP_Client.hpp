#pragma once

#include "TCP.hpp"
#include "Socket.hpp"

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
		
		int Send(const char* data, int len)
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
		char buf[TCP_BUFFER_SIZE] = "";
		int bufLen = 0;
	};
}