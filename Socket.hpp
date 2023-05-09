#pragma once
#ifdef _WIN32
#include <WS2tcpip.h>
#include <Windows.h>
#include <WinSock2.h>

#pragma comment(lib, "ws2_32.lib")
#else
//all the linux headers here, fuck
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <string.h>
#include <string>
#include <errno.h>

#define closesocket close
#define ioctlsocket ioctl

typedef unsigned int SOCKET;
#endif

#define TCP_BUFFER_SIZE 512

#include <iostream>
#include <string>

namespace Socket
{
	inline struct sockaddr_in initAddr_shd(unsigned int ip, int port)
	{
		struct sockaddr_in target_addr;
		target_addr.sin_family = AF_INET;
		target_addr.sin_addr.s_addr = ip;
		target_addr.sin_port = htons(port);

		return target_addr;
	}

	inline struct sockaddr_in initAddr(const char* host, int port = 80)
	{
		struct addrinfo hints;
		struct addrinfo* addr = NULL;
		struct sockaddr_in result = {};
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_flags = AI_PASSIVE;
		hints.ai_protocol = IPPROTO_TCP;
		hints.ai_socktype = SOCK_STREAM;

		std::string hostStr = host;
		for (int i = 0; host[i]; i++)
		{
			if (host[i] == ':')
			{
				port = 0;
				hostStr = hostStr.substr(0, i);
				for (i++; host[i]; i++)
					port = port * 10 + (host[i] - '0');
				break;
			}
		}
		if (getaddrinfo(hostStr.c_str(), nullptr, &hints, &addr) == -1)
			exit(-1);

		//		InetPton(AF_INET, host, &addr_p);
		if (addr != NULL)
		{
			result = *(struct sockaddr_in*)addr->ai_addr;
			result.sin_port = htons(port);
		}

		return result;
	}
#ifdef _WIN32
	inline struct sockaddr_in initAddrW(const wchar_t* host, int port)
	{
		ADDRINFOW hints;
		PADDRINFOW addr = NULL;
		struct sockaddr_in* result;
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_flags = AI_PASSIVE;
		hints.ai_protocol = IPPROTO_TCP;
		hints.ai_socktype = SOCK_STREAM;
		if (GetAddrInfoW(host, nullptr, &hints, &addr) == -1)
			exit(-1);

		//		InetPton(AF_INET, host, &addr_p);
		result = (struct sockaddr_in*)addr->ai_addr;
		result->sin_port = htons(port);
		return *result;
	}
#endif
#ifdef _WIN32
	inline int initWinSock()
	{
		WSADATA wsaData;
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
			return -1;
		return 0;
	}
#endif

	inline SOCKET initSocket()
	{
		SOCKET socket_fd;
		socket_fd = socket(AF_INET, SOCK_STREAM, 0);

		return socket_fd;
	}

	inline int sockConn(const SOCKET* s_fd, const struct sockaddr_in* target_addr)
	{
		return connect(*s_fd, (struct sockaddr*)target_addr, sizeof(*target_addr));
	}

	inline SOCKET listenSocket(const SOCKET s_fd, struct sockaddr_in* srv_addr)
	{
		socklen_t s_len = sizeof(*srv_addr);

		if (bind(s_fd, (struct sockaddr*)srv_addr, s_len) < 0)
		{
			std::cout << s_fd << ", " << errno << std::endl;
			goto error;
		}

		return listen(s_fd, 5);
		//	return accept(s_fd, (struct sockaddr*)&srv_addr, &s_len);
	error:
		return -1;
	}

	inline void closeSocket(const SOCKET* s_fd)
	{
		closesocket(*s_fd);
#ifdef _WIN32
		WSACleanup();
#endif
	}

	template <typename F>
	static void Server(F f, short* pState, int* pClientCount, SOCKET& s_fd, struct sockaddr_in& srv_addr)
	{
		SOCKET c_fd;
		struct sockaddr_in cli_addr = {};
		socklen_t cli_len;

		unsigned long mOn = 1;
		setsockopt(s_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&mOn, sizeof(char));
		if (listenSocket(s_fd, &srv_addr) == (SOCKET)-1)
		{
			return;
		}
		ioctlsocket(s_fd, FIONBIO, &mOn);
		while (*pState)
		{
			c_fd = accept(s_fd, (struct sockaddr*)&cli_addr, &cli_len);
			if (c_fd > 0 && c_fd != (SOCKET)-1)
			{
				//while (clients_count > 12);
				f(c_fd);
				//thread(proxy, c_fd).detach();
			}
		}
		while (*pClientCount);
	}

#ifdef _WIN32
	inline std::string wctos(const wchar_t* source)
	{
		size_t sLen = wcslen(source);
		char* c = new char[sLen * 3 + 1];
		WideCharToMultiByte(CP_UTF8, 0, source, (int)(sLen + 1), c, (int)(sLen * 3 + 1), 0, 0);

		std::string result = c;
		delete[] c;
		return result;
	}

	inline wchar_t* ctowc(const char* source)
	{
		wchar_t* result;
		int sLen, rLen = MultiByteToWideChar(CP_UTF8, 0, source, sLen = (int)strlen(source), NULL, 0);
		result = new wchar_t[rLen + 1];
		MultiByteToWideChar(CP_UTF8, 0, source, sLen, result, rLen);
		result[rLen] = '\0';
		return result;
	}
#endif
}