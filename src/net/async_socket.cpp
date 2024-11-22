#include <winsock2.h>
#include <ws2tcpip.h>
#include "async_socket.h"
#include "socket_util.h"
#include <QDebug>
#include <thread>

int AsyncSocket::Init()
{
	WSAData data;
	int ret = WSAStartup(MAKEWORD(2, 2), &data);

	sock_ = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
	if (sock_ == INVALID_SOCKET)
		return -1;

	SetReuseAddr(sock_);
	SetNoDelay(sock_);
	return 0;
}

int AsyncSocket::Bind(uint16_t port)
{
	SOCKADDR_IN addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(port);
	if (bind(sock_, (sockaddr*)&addr, sizeof(addr)) != 0)
	{
		closesocket(sock_);
		sock_ = INVALID_SOCKET;
		return -1;
	}

	return 0;
}

int AsyncSocket::Listen(int backlog)
{
	if (listen(sock_, backlog) != 0)
	{
		closesocket(sock_);
		sock_ = INVALID_SOCKET;
		return -1;
	}
	return 0;
}

int AsyncSocket::Connect(std::string& ip, uint16_t port)
{
	if (sock_ == INVALID_SOCKET)
		return -1;

	if (ip.empty())
		return -1;

	sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr);

	int ret = ::connect(sock_, (sockaddr*)&server_addr, sizeof(server_addr));
	if (ret == SOCKET_ERROR)
	{
		qDebug() << "connect failed";
		closesocket(sock_);
		return -1;
	}

	return 0;
}

int AsyncSocket::Connect(std::string& ip, uint16_t port, int timeout_ms, int max_attempts)
{
	WSAEVENT event = WSACreateEvent();
	if (event == WSA_INVALID_EVENT)
	{
		qDebug() << "Failed to create WSA event";
		return -1;
	}

	if (WSAEventSelect(sock_, event, FD_CONNECT) == SOCKET_ERROR)
	{
		qDebug() << "WSAEventSelect failed";
		WSACloseEvent(event);
		return -1;
	}

	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
	int addr_len = sizeof(addr);
	
	for (int attempt = 1; attempt < max_attempts; attempt++)
	{
		if (WSAEventSelect(sock_, event, FD_CONNECT) == SOCKET_ERROR)
		{
			qDebug() << "WSAEventSelect failed";
			WSACloseEvent(event);
			return -1;
		}

		if (::connect(sock_, (sockaddr*)&addr, addr_len) == SOCKET_ERROR)
		{
			if (WSAGetLastError() != WSAEWOULDBLOCK)
			{
				qDebug() << "Connect failed immediately on attempt " << attempt;
				WSACloseEvent(event);
				return -1;
			}
		}

		DWORD wait_result = WSAWaitForMultipleEvents(1, &event, FALSE, timeout_ms, FALSE);
		if (wait_result == WSA_WAIT_TIMEOUT)
		{
			qDebug() << "connect time out on attempt";
		}
		else if (wait_result == WSA_WAIT_FAILED)
		{
			qDebug() << "WSAWaitForMultipleEvents failed on attempt";
			WSACloseEvent(event);
			return -1;
		}
		else
		{
			WSANETWORKEVENTS network_events;
			if (WSAEnumNetworkEvents(sock_, event, &network_events) == SOCKET_ERROR)
			{
				qDebug() << "WSAEnumNetworkEvents failed on attempt ";
				WSACloseEvent(event);
				return -1;
			}

			if (network_events.iErrorCode[FD_CONNECT_BIT] == 0)
			{
				qDebug() << "Connected to the server on attempt!!!!!!!";
				WSACloseEvent(event);
				return 0;
			}
			else
			{
				qDebug() << "Connect error on attempt " << attempt << ": "
					<< network_events.iErrorCode[FD_CONNECT_BIT];
			}
		}
		// 连接失败,重置套接字状态和事件对象,准备下一次尝试
		WSAEventSelect(sock_, nullptr, 0);
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
	WSACloseEvent(event);
	return -1;
}

void AsyncSocket::Close()
{
	if (sock_)
	{
		closesocket(sock_);
		sock_ = INVALID_SOCKET;
	}
}