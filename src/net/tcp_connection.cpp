#include "tcp_connection.h"
#include "event_type.h"
#include "socket_util.h"
#include <QDebug>



TcpConnection::TcpConnection(EventLoop* event_loop, SOCKET sock)
	: event_loop_(event_loop),
	socket_(sock),
	channel_(new Channel(sock))
{
	channel_->SetReadCallback([this](IOContext* io_ctx) { this->HandlRead(io_ctx); });
	channel_->SetWriteCallback([this](IOContext* io_ctx) { this->HandleWrite(io_ctx); });
	channel_->SetCloseCallback([this] { this->HandleClose(); });

	SetSendBufSize(sock, 100 * 1024);
	SetRecvBufSize(sock, 100 * 1024);
	SetNoDelay(sock);
	recv_buf_.Resize(100 * 1024);

	channel_->EnableReading();
	channel_->EnableWriting();
	event_loop_->UpdateChannel(channel_);
}

TcpConnection::~TcpConnection()
{
	SOCKET fd = channel_->GetSocket();
	if (fd > 0)
		closesocket(fd);
}

void TcpConnection::Send(std::shared_ptr<char> data, uint32_t size)
{
	if (is_close_)
		return;

	IOContext* io_ctx = new IOContext;
	memset(io_ctx, 0, sizeof(IOContext));
	io_ctx->data_ptr = data;
	io_ctx->buffer.buf = data.get();
	io_ctx->buffer.len = size;
	io_ctx->type = EVENT_OUT;
	qDebug() << "Send io_ctx: " << io_ctx;
	int result = WSASend(socket_, &io_ctx->buffer, 1, &io_ctx->bytes_transferred, io_ctx->flags, &io_ctx->overlapped, nullptr);
	if (result == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
	{
		qDebug() << "WSASend failed with error";
		HandleClose();	// 如果发送失败,关闭连接
	}
}


void TcpConnection::Disconnect()
{
	if (!is_close_)
	{
		std::unique_lock<std::mutex> lock(mutex_);
		auto conn = shared_from_this();
		event_loop_->GetThreadPool()->SubmitTask([conn] {
			conn->Close(); });
	}
}

void TcpConnection::Recv()
{
	if (is_close_)
		return;
	IOContext* io_ctx = new IOContext;
	qDebug() << "Recv io_ctx: " << io_ctx;
	memset(io_ctx, 0, sizeof(IOContext));
	io_ctx->buffer.buf = recv_buf_.WriteBegin();
	io_ctx->buffer.len = recv_buf_.WritableBytes();
	io_ctx->bytes_transferred = 0;
	io_ctx->type = EVENT_IN;
	io_ctx->serv_sock = socket_;

	int result = WSARecv(socket_, &io_ctx->buffer, 1, &io_ctx->bytes_transferred, &io_ctx->flags, &io_ctx->overlapped, nullptr);
	if (result == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
	{
	// 如果接收失败,且错误不是 WSA_IO_PENDING(表示立刻收到了数据), 则关闭连接
		qDebug() << "WSARecv failed with error";
		HandleClose();
	}
}


void TcpConnection::HandlRead(IOContext* io_ctx)
{
	if (is_close_)
		return;

	qDebug() << "HandleRead type: " << io_ctx->type << " bytes: " << io_ctx->bytes_transferred;

	DWORD bytes_read = 0;
	if (io_ctx->bytes_transferred > 0)
	{
		recv_buf_.Advance(io_ctx->bytes_transferred);

		if (read_callback_)
			read_callback_(this, recv_buf_);
	}
	Recv();
}

void TcpConnection::HandleWrite(IOContext* io_ctx)
{
	qDebug() << "HandleWrite io_ctx addr: " << &io_ctx << " type: " << io_ctx->type;
	// 处理发送数据后的逻辑
	if (is_close_)
		return;
	delete io_ctx;
}

void TcpConnection::HandleClose()
{
	std::unique_lock<std::mutex> lock(mutex_);
	this->Close();
}


void TcpConnection::Close()
{
	if (!is_close_)
	{
		is_close_ = true;
		event_loop_->RemoveChannel(channel_);
		if (close_callback_)
			close_callback_(this);

		if (disconn_callback_)
			disconn_callback_(this);
	}
}
