#pragma once

#define DEFAULT_HOST "127.0.0.1"
#define DEFAULT_PORT 4399

namespace TCP
{
	enum ConnectionStates
	{
		Connected, Disconnected, Closed
	};

}