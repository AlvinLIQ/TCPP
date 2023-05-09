#pragma once

#include "TCP.hpp"
#include "Socket.hpp"

namespace TCP
{
	class TCP_Client
	{
	public:
		TCP_Client(const SOCKET& c_fd);
		TCP_Client(const char* host, int port);
		~TCP_Client();
		int Connect();
		int Recv();
		int Send(const char* data, int len);

		int Close();

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