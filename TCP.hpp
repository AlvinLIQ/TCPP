#pragma once

#define DEFAULT_HOST "127.0.0.1"
#define DEFAULT_PORT 4399

#ifdef _WIN32
#define ssize_t int
#endif

namespace TCP
{
	enum ConnectionStates
	{
		Connected, Disconnected, Closed
	};
	enum ServerStates
	{
		Listening, Stopped
	};
}
