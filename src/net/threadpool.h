#pragma once

#include "concurrent_queue.h"
#include "noncopyable.h"

#include <chrono>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <future>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <unordered_map>

/*
    �߳��������ȷ��:
        CPU �ܼ�������: �����߳�������CPU����������Ȼ��Զ�.��Ϊÿ���߳���ҪCPUʱ��Ƭ,
            ����߳���������,���ܻ�����Ƶ�����������л�,������������
        IO�ܼ�������: �������������ļ���д��.�߳����������ӵ�CPU����������������������,
            ������ΪIO����ͨ���������߳�,�����߳̿������ϵͳ�Ĳ�������

    �������ִ��ʱ��϶�,��ô���ý��ٵ��߳���
    ���ִ������ʱ�䳤,������֮��û��̫������,���������߳�����߲��д�������.
    ���鷨��:
        �߳��� = CPU������ + 1(�������ܼ���) ����
        �߳��� = CPU������ * 2(IO�ܼ���)
*/

const int kTaskMaxThreshold = 1024;
const int kThreadMaxThreshold = 1024;
const int kThreadMaxIdleTime = 60;


enum class PoolMode
{
	MODE_FIXED,     // �̶��߳�����ģʽ
	MODE_CACHED     // ��̬�߳�����ģʽ
};


// �߳�
class Thread
{
	using Func = std::function<void(int)>;
public:
	Thread(Func func);
	~Thread() {}
	void Start();
	int GetId() const;
private:
	Func func_;
	static int generate_id_;
	int thread_id_;
};


// �̳߳�
class ThreadPool : private NonCopyable
{
	using Task = std::function<void()>;
public:
	ThreadPool();
	~ThreadPool();
	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;
	void Exit();
	/**
	* @brief �����̳߳ع���ģʽ: �̶��߳���������̬�߳�����
	*
	* @param mode
	*/
	void SetMode(PoolMode mode);
	/**
	* @brief �����߳������������. Ĭ��1024
	*
	* @param threshold
	*/
	void SetThreadMaxThreshold(int threshold = kThreadMaxThreshold);
	/**
	* @brief ���������������. Ĭ��512
	*
	* @param threshold
	*/
	void SetTaskMaxThreshold(int threshold = kTaskMaxThreshold);
	template <typename Func, typename... Args>
	auto SubmitTask(Func&& func, Args&&... args) -> std::future<decltype(func(args...))>
	{
		if (running_ == false)
			std::runtime_error("Threadpool is no running!!!");

		using RType = decltype(func(args...));
		auto task = std::make_shared<std::packaged_task<RType()>>(
			std::bind(std::forward<Func>(func), std::forward<Args>(args)...));
		std::future<RType> result = task->get_future();
		int ret = 0;
		_SubmitTask<RType>(task, ret);
		if (ret != 0)
		{
			auto retNull = std::make_shared<std::packaged_task<RType()>>(
				[]()->RType { return RType(); });
			(*retNull)();
			return retNull->get_future();
		}
		return result;
	}
	/**
	* @brief �ύ����.�ύ��Ա����
	*
	* @tparam Func ��Ա����ǩ��
	* @tparam Obj ��������
	* @tparam Args ������������
	* @param func ��Ա������ַ
	* @param obj �����ַ
	* @param args �����õ��Ĳ���
	* @return std::future<typename std::result_of<Func(Obj, Args...)>::type>
	*/
	template <typename Func, typename Obj, typename... Args>
	std::future<typename std::result_of<Func(Obj, Args...)>::type>
		SubmitTask(Func&& func, Obj&& obj, Args&&... args)
	{
		if (running_ == false)
			std::runtime_error("Threadpool is no running!!!");

		using RType = typename std::result_of<Func(Obj, Args...)>::type;
		auto task = std::make_shared<std::packaged_task<RType()>>(
			std::bind(std::forward<Func>(func), std::forward<Obj>(obj),
				std::forward<Args>(args)...)
		);
		std::future<RType> result = task->get_future();
		int ret = 0;
		_SubmitTask<RType>(task, ret);
		if (ret != 0)
		{
			auto ret_null = std::make_shared<std::packaged_task<RType()>>(
				[]()->RType { return RType(); }
			);
			(*ret_null)();
			return ret_null->get_future();
		}
		return result;
	}
	/**
	* @brief �����̳߳�
	*
	* @param initThreadCnt
	*/
	void Start(int initThreadCnt = std::thread::hardware_concurrency() + 10);
private:
	void ThreadFunc(int thread_id); // �߳�ִ�к���
	template <typename RType>
	void _SubmitTask(std::shared_ptr<std::packaged_task<RType()>> task, int& ret)
	{
		std::unique_lock<std::mutex> lock(mutex_);
		auto task_lambda = [task]() { (*task)(); };
		if (!task_queue_.TryEmplace(task_lambda))
		{
			// �������ʧ��,��ô�ȴ�10ms�ڲ���
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			if (!task_queue_.TryEmplace(task_lambda))
			{
				ret = 1;    // ����ʧ���򷵻�
				return;
			}
		}
		not_empty_cond_.notify_all();   // ֪ͨ�����߳���������
		if (mode_ == PoolMode::MODE_CACHED
			&& task_cnt_ > (idle_thread_cnt_ * 2) /* �����������ڿ������������� */
			&& curr_thread_cnt_ < thread_max_threshold_)
		{
			// ���������߳�
			int create_task_cnt = (task_cnt_ - idle_thread_cnt_) / 2;
			for (int i = 0; i < create_task_cnt; i++)
			{
				auto ptr = new Thread(std::bind(&ThreadPool::ThreadFunc, this,
					std::placeholders::_1));
				int thread_id = ptr->GetId();
				threads_.emplace(thread_id, std::move(ptr));
				threads_[thread_id]->Start();
			}

			curr_thread_cnt_ += create_task_cnt;
			idle_thread_cnt_ += create_task_cnt;
		}

	}
private:
	std::unordered_map<int, std::unique_ptr<Thread>> threads_;  // �߳��б�
	int init_thread_cnt_;                               // �̳߳�ʼ����
	int thread_max_threshold_ = kThreadMaxThreshold;    // �߳������������
	std::atomic<int> curr_thread_cnt_ = 0;              // ��ǰ�߳�����
	std::atomic<int> idle_thread_cnt_ = 0;              // �����߳�����
	PoolMode mode_;                                     // �̳߳ع���ģʽ
	std::atomic<bool> running_ = false;                 // �̳߳ع���״̬
	std::atomic<int> task_cnt_ = 0;
	int task_max_threadshold_ = kTaskMaxThreshold;
	ConcurrentQueue<Task> task_queue_;
	std::mutex mutex_;
	std::condition_variable not_full_cond_;     // ���з�����������
	std::condition_variable not_empty_cond_;    // ���зǿ���������
	std::condition_variable exit_cond_;         // �̳߳��˳���������
};
