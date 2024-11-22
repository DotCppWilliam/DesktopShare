#include "socket_util.h"

void SetNonBlock(SOCKET sock)
{
	unsigned long on = 1;
	ioctlsocket(sock, FIONBIO, &on);
}

void SetReuseAddr(SOCKET sock)
{
	int on = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on));
}

void SetNoDelay(SOCKET sock)
{
	int on = 1;
	int ret = setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char*)&on, sizeof(on));
}

void SetKeepAlive(SOCKET sock)
{
	int on = 1;
	setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (char*)&on, sizeof(on));
}

void SetSendBufSize(SOCKET sock, size_t size)
{
	setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char*)&size, sizeof(size));
}

void SetRecvBufSize(SOCKET sock, size_t size)
{
	setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char*)&size, sizeof(size));
}