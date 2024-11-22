#include "socket.h"


SocketBase::SocketBase()
{
}

SocketBase::~SocketBase()
{
	WSACleanup();
}

int SocketBase::Init()
{
	return 0;
}

int SocketBase::Bind(uint16_t port)
{
	return 0;
}

int SocketBase::Listen(int backlog)
{
	return 0;
}

int SocketBase::Connect(std::string& ip, uint16_t port)
{
	return 0;
}

int SocketBase::Connect(std::string& ip, uint16_t port, int timeout_ms, int max_attempts)
{
	return 0;
}

void SocketBase::Close()
{
}
