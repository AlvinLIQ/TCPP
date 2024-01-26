#include "TCP_Client.hpp"

using namespace TCP;
using namespace Socket;

TCP_Client::TCP_Client(const SOCKET& c_fd)
{
	s_fd = c_fd;
	state = ConnectionStates::Connected;
}

TCP_Client::TCP_Client(const char* host, int port)
{
	s_fd = initSocket();
	sockAddr = initAddr(host, port);
}

TCP_Client::~TCP_Client()
{
}

int TCP_Client::Connect()
{
	int result = sockConn(&s_fd, &sockAddr);
	if (!result)
	{
		ioctlsocket(s_fd, FIONBIO, (u_long*)&OptVal);
		state = ConnectionStates::Connected;
	}
	else
		state = ConnectionStates::Disconnected;

	return result;
}

int TCP_Client::Recv(const int bufSize)
{
	int len = recv(s_fd, buf, bufSize, 0);
	if (!len || SocketShouldClose())
		Close();
	bufLen = len <= 0 ? 0 : len;
	buf[bufLen] = '\0';

	return len;
}

int TCP_Client::Send(const char* data, int len)
{
	int result = send(s_fd, data, len, 0);
	if (SocketShouldClose())
		Close();

	return result;
}

int TCP_Client::Close()
{
	closesocket(s_fd);
	state = ConnectionStates::Closed;
	return 0;
}