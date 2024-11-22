#include "event_loop.h"
#include "channel.h"
#include <QDebug>
#include <stdexcept>

#pragma comment(lib, "ws2_32.lib")

EventLoop::EventLoop(ThreadPool* thread_pool)
	: thread_pool_(thread_pool)
{
	iocp_ = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
	if (!iocp_)
	{
		qDebug() << "failed to create iocp";
		throw std::runtime_error("Create IOCP failed");
	}
}

EventLoop::~EventLoop()
{
	if (iocp_)
		CloseHandle(iocp_);
}

void EventLoop::UpdateChannel(std::shared_ptr<Channel> channel)
{
	SOCKET sock = channel->GetSocket();
	channels_[sock] = channel;
	if (!CreateIoCompletionPort((HANDLE)sock, iocp_, (ULONG_PTR)channel.get(), 0))
	{
		qDebug() << "Failed to associate socket with IOCP";
		throw std::runtime_error("Failed to associate socket with IOCP");
	}
}

void EventLoop::RemoveChannel(std::shared_ptr<Channel> channel)
{
	channels_.erase(channel->GetSocket());
}

void EventLoop::Poll(int timeout_ms)
{
	thread_ = std::thread(&EventLoop::ThreadFunc, this, timeout_ms);
	thread_.detach();
}

void EventLoop::Quit()
{
	quit_ = true;
}


void EventLoop::ThreadFunc(int timeout_ms)
{
	DWORD bytes_transferred = 0;
	ULONG_PTR completion_key = 0;
	OVERLAPPED* overlapped = nullptr;

	while (!quit_)
	{
		BOOL ret = GetQueuedCompletionStatus(iocp_, &bytes_transferred, &completion_key,
			&overlapped, timeout_ms);
		if (!ret)
		{
			DWORD err = GetLastError();
			if (err == WSA_WAIT_TIMEOUT)
				continue;
			else
			{
				qDebug() << "GetQueuedCompletionStatus failed: " << err;
				return;
			}
		}
		Channel* channel = (Channel*)completion_key;
		IOContext* io_ctx = (IOContext*)overlapped;
		qDebug() << "EventLoop type: " << io_ctx->type << " bytes: " << io_ctx->bytes_transferred << " addr: " << io_ctx;
		fflush(stdout);
		// 提交给线程池去处理具体的事件
		thread_pool_->SubmitTask([=] {
			channel->HandleEvents(io_ctx, io_ctx->type); });
		
		bytes_transferred = completion_key = 0;
		overlapped = nullptr;
	}
}
