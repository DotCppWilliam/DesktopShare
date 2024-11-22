#pragma once

#include <windows.h>
#include <string>
#include <cstdint>

// ÆÕÍ¨×èÈûÊ½ socket
class SocketBase
{
public:
	SocketBase();
	virtual ~SocketBase();
public:
	virtual int Init();
	virtual int Bind(uint16_t port);
	virtual int Listen(int backlog);
	virtual int Connect(std::string& ip, uint16_t port);
	virtual int Connect(std::string& ip, uint16_t port, int timeout_ms, int max_attempts);
	virtual void Close();
	SOCKET& GetSocket()
	{
		return sock_;
	}
protected:
	SOCKET sock_ = -1;
};