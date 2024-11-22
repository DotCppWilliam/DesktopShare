#pragma once

#include "channel.h"
#include "event_loop.h"
#include "buffer.h"
#include <functional>
#include <atomic>
#include <mutex>

class TcpConnection;
using CloseCallback = std::function<void(TcpConnection* conn)>;
using DisconnectCallback = std::function<void(TcpConnection* conn)>;
using ReadCallback = std::function<bool(TcpConnection* conn, Buffer& buf)>;

class TcpConnection : public std::enable_shared_from_this<TcpConnection>
{
public:
	TcpConnection(EventLoop* event_loop, SOCKET sock);
	~TcpConnection();
public:
	void SetReadCallback(const ReadCallback& cb)
	{
		read_callback_ = cb;
	}

	void SetCloseCallback(const CloseCallback& cb)
	{
		close_callback_ = cb;
	}

	void Send(std::shared_ptr<char> data, uint32_t size);
	void Recv();
	void Disconnect();
protected:
	virtual void HandlRead(IOContext* io_ctx);
	virtual void HandleWrite(IOContext* io_ctx);
	virtual void HandleClose();
	void SetDisconnectCallbakck(const DisconnectCallback& cb)
	{
		disconn_callback_ = cb;
	}
private:
	void Close();
protected:
	EventLoop* event_loop_;
	std::shared_ptr<Channel> channel_;
	ReadCallback read_callback_;
	CloseCallback close_callback_;
	DisconnectCallback disconn_callback_;
	Buffer recv_buf_;
	std::atomic<bool> is_close_ = false;
	std::mutex mutex_;
	SOCKET socket_;
};