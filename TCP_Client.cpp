#include "TCP_Client.hpp"

using namespace TCP;
using namespace Socket;

int clientCount = 0;

TCP_Client::TCP_Client(const SOCKET& c_fd)
{
	s_fd = c_fd;
	state = ConnectionStates::Connected;
}

TCP_Client::TCP_Client(const char* host, int port)
{
	s_fd = initSocket();
	sockAddr = initAddr(host, port);

	++clientCount;
}

TCP_Client::~TCP_Client()
{
	--clientCount;
}

int TCP_Client::Connect()
{
	int result = sockConn(&s_fd, &sockAddr);
	if (!result)
		state = ConnectionStates::Connected;
	else
		state = ConnectionStates::Disconnected;

	return result;
}

int TCP_Client::Recv()
{
	int len = recv(s_fd, buf, TCP_BUFFER_SIZE, 0);
	bufLen = len <= 0 ? 0 : len;
	return len;
}

int TCP_Client::Send(const char* data, int len)
{
	return send(s_fd, data, len, 0);
}

int TCP_Client::Close()
{
	closeSocket(&s_fd);
	state = ConnectionStates::Closed;
	return 0;
}