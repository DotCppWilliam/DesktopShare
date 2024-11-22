#pragma once

#include "socket.h"

class AsyncSocket : public SocketBase
{
public:
	int Init() override;

	int Bind(uint16_t port) override;
	int Listen(int backlog) override;
	int Connect(std::string& ip, uint16_t port) override;
	int Connect(std::string& ip, uint16_t port, int timeout_ms, int max_attempts) override;
	void Close() override;
};