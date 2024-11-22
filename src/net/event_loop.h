#pragma once

#include "channel.h"
#include "threadpool.h"
#include <unordered_map>
#include <thread>


class EventLoop
{
public:
	EventLoop(ThreadPool* thread_pool);
	~EventLoop();
public:
	void UpdateChannel(std::shared_ptr<Channel> channel);
	void RemoveChannel(std::shared_ptr<Channel> channel);
	void Poll(int timeout_ms);
	ThreadPool* GetThreadPool()
	{
		return thread_pool_;
	}
	HANDLE GetIocp()
	{
		return iocp_;
	}
	void Quit();
private:
	void ThreadFunc(int timeout_ms);
private:
	HANDLE iocp_ = nullptr;
	std::unordered_map<SOCKET, std::shared_ptr<Channel>> channels_;
	ThreadPool* thread_pool_;
	bool quit_ = false;
	std::thread thread_;
};