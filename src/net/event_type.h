#pragma once

#include <WinSock2.h>
#include <memory>

enum EventType
{
	EVENT_NONE = 0,
	EVENT_IN = 1,
	EVENT_PRI = 2,
	EVENT_OUT = 4,
	EVENT_ERR = 8,
	EVENT_HUP = 16,
	EVENT_RDHUP = 32
};

enum OptionType
{
	OP_TYPE_RECV,
	OP_TYPE_SEND,
	OP_TYPE_ACCEPT
};

struct IOContext
{
	OVERLAPPED overlapped;
	WSABUF buffer;
	std::shared_ptr<char> data_ptr;	// 共享数据指针,避免拷贝数据
	DWORD bytes_transferred;
	EventType type;
	SOCKET serv_sock;
	SOCKET client_sock;
	DWORD flags;
};
