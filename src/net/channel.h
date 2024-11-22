#pragma once

#include "event_type.h"
#include <functional>
#include <QDebug>

using EventCallback = std::function<void(IOContext* io_ctx)>;
using CloseEventCallback = std::function<void()>;


#include <QDebug>
class Channel
{
public:
	Channel(SOCKET sock)
		: sock_(sock) { }
	~Channel() 
	{
		qDebug() << "~Channel";
	}
public:
	void SetReadCallback(const EventCallback& cb)
	{
		read_callback_ = cb;
	}

	void SetWriteCallback(const EventCallback& cb)
	{
		write_callback_ = cb;
	}

	void SetCloseCallback(const CloseEventCallback& cb)
	{
		close_callback_ = cb;
	}

	void SetErrorCallback(const EventCallback& cb)
	{
		error_callback_ = cb;
	}

	void EnableReading()
	{
		events_ |= EVENT_IN;
	}

	void EnableWriting()
	{
		events_ |= EVENT_OUT;
	}

	void DisableReading()
	{
		events_ &= ~EVENT_IN;
	}

	void DisableWriting()
	{
		events_ &= ~EVENT_OUT;
	}

	void HandleEvents(IOContext* io_ctx, int revents)
	{
		if ((events_ & (EVENT_PRI | EVENT_IN))
			&& (revents & (EVENT_PRI | EVENT_IN)))
		{
			read_callback_(io_ctx);
		}

		if ((events_ & EVENT_OUT)
			&& (revents & EVENT_OUT))
		{
			write_callback_(io_ctx);
		}

		if ((events_ & EVENT_HUP)
			&& (revents & EVENT_HUP))
		{
			close_callback_();
		}

		if ((events_ & EVENT_ERR)
			&& (revents & EVENT_ERR))
		{
			error_callback_(io_ctx);
		}
	}

	SOCKET GetSocket() { return sock_; }

	// 设置可感兴趣事件
	void SetEvents(int events) { events_ = events; }

private:
	EventCallback read_callback_ = [](IOContext* io_ctx) {};
	EventCallback write_callback_ = [](IOContext* io_ctx) {};
	CloseEventCallback close_callback_ = [] {};
	EventCallback error_callback_ = [](IOContext* io_ctx) {};

	SOCKET sock_ = INVALID_SOCKET;
	int events_ = 0;
};