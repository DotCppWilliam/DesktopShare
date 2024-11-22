#include "acceptor.h"
#include "async_socket.h"
#include "channel.h"
#include <mswsock.h>
#include <QDebug>

#pragma comment(lib, "Mswsock.lib")
#pragma warning(disable: 4996)

Acceptor::Acceptor(EventLoop* event_loop)
	: event_loop_(event_loop)
{
	sock_ = new AsyncSocket();
}

Acceptor::~Acceptor()
{
	if (sock_)
		delete sock_;
}

int Acceptor::Listen(uint16_t port)
{
	int ret = 0;
	if (sock_->Init() == 0)
	{
		if (sock_->Bind(port) == 0)
		{
			if (sock_->Listen(1024) == 0)
			{
			// 将listen socket与iocp关联
				if (CreateIoCompletionPort((HANDLE)sock_->GetSocket(), event_loop_->GetIocp(),
					(ULONG_PTR)channel_.get(), 0) == nullptr)
				{
					sock_->Close();
					return -1;
				}

				channel_.reset(new Channel(sock_->GetSocket()));
				channel_->SetReadCallback([this](IOContext* io_ctx) {
					this->OnAccept(io_ctx); });
				channel_->EnableReading();
				event_loop_->UpdateChannel(channel_);
				return 0;
			}
		}
	}

	return -1;
}

int Acceptor::Accept()
{
	// 为新的客户端创建套接字
	SOCKET client_sock = WSASocket(AF_INET, SOCK_STREAM, 0, nullptr, 0, WSA_FLAG_OVERLAPPED);
	if (client_sock == INVALID_SOCKET)
		return -1;

	IOContext* io_ctx = new IOContext();
	ZeroMemory(&io_ctx->overlapped, 0, sizeof(OVERLAPPED));
	io_ctx->data_ptr.reset(new char[2 * sizeof(sockaddr_in)] + 16);
	io_ctx->buffer.buf = io_ctx->data_ptr.get();
	io_ctx->buffer.len = 2 * (sizeof(sockaddr_in) + 16);
	io_ctx->serv_sock = sock_->GetSocket();
	io_ctx->type = EVENT_IN;

	DWORD byte_received = 0;
	BOOL ret = AcceptEx(sock_->GetSocket(),
		client_sock,
		io_ctx->buffer.buf,
		0,
		sizeof(sockaddr_in) + 16,
		sizeof(sockaddr_in) + 16,
		&byte_received,
		&io_ctx->overlapped);
	
	if (!ret && WSAGetLastError() != ERROR_IO_PENDING)
	{
		qDebug() << "AcceptEx failed: " << WSAGetLastError();
		sock_->Close();
		return -1;
	}
}

int Acceptor::OnAccept(IOContext* io_ctx)
{
	if (io_ctx == nullptr || io_ctx->client_sock == INVALID_SOCKET)
	{
		qDebug() << "Invalid acccept context";
		return -1;
	}

	// 获取客户端地址信息
	sockaddr_in* local_addr = nullptr;
	sockaddr_in* remote_addr = nullptr;
	int local_addr_len = sizeof(sockaddr_in);
	int remote_addr_len = sizeof(sockaddr_in);
	
	// 使用 AcceptEx 提供的函数解析地址
	GetAcceptExSockaddrs(
		io_ctx->buffer.buf,
		0,
		sizeof(sockaddr_in) + 16,
		sizeof(sockaddr_in) + 16,
		(sockaddr**)&local_addr,
		&local_addr_len,
		(sockaddr**)&remote_addr,
		&remote_addr_len
	);

	qDebug() << "Accepted new connection from: " << inet_ntoa(remote_addr->sin_addr) 
		<< ":" << ntohs(remote_addr->sin_port);

	SOCKET client_sock = io_ctx->client_sock;
	if (new_conn_cb_)
	{
		delete io_ctx;
		new_conn_cb_(client_sock);
	}

	return 0;
}
