#pragma once

#include "channel.h"
#include "socket.h"
#include "event_type.h"
#include "event_loop.h"
#include <functional>

class EventLoop;
using NewConnCallback = std::function<void(SOCKET sock)>;

class Acceptor
{
public:
	Acceptor(EventLoop* event_loop);
	~Acceptor();
public:
	void SetNewConnCallback(const NewConnCallback& cb)
	{
		new_conn_cb_ = cb;
	}
	int Listen(uint16_t port);
	int Accept();
private:
	int OnAccept(IOContext* io_ctx);
private:
	EventLoop* event_loop_ = nullptr;
	std::shared_ptr<Channel> channel_;
	NewConnCallback new_conn_cb_;
	SocketBase* sock_ = nullptr;
};