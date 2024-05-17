#include "threadpool.h"

#include <iostream>
#include <thread>

/// <summary>
/// 线程池方法实现
/// </summary>

const int TASK_MAX_THRESHHOLD = 1024;

// 构造
ThreadPool::ThreadPool()
	: initThreadSize_(0)
	, taskSize_(0)
	, taskQueMaxThreshHold_(TASK_MAX_THRESHHOLD)
	, poolMode_(PoolMode::MODE_FIXED)
{
}

// 析构
ThreadPool::~ThreadPool()
{
}

// 设置线程池工作模式
void ThreadPool::setMode(PoolMode mode)
{
	poolMode_ = mode;
}

// 设置任务队列上限阈值
void ThreadPool::setTaskQueMaxThreshHold(int threshhold)
{
	taskQueMaxThreshHold_ = threshhold;
}

// 给线程池提交任务	用户调用该接口，传入任务对象，生产任务
void ThreadPool::submitTask(std::shared_ptr<Task> sp)
{
	// 获取锁
	std::unique_lock<std::mutex> lock(taskQueMtx_);

	// 线程通信  等待任务队列有空余
	notFull_.wait(lock, [&]()->bool { return taskQue_.size() < taskQueMaxThreshHold_; });

	// 如果有空余，把任务放在任务队列中
	taskQue_.emplace(sp);
	taskSize_++;

	// 新放了任务，任务队列肯定不空，notEmpty通知
	notEmpty_.notify_all();
}

// 开启线程池
void ThreadPool::start(int initThreadSize)
{
	// 记录线程初始个数
	initThreadSize_ = initThreadSize;

	// 创建线程对象
	for (int i = 0; i < initThreadSize_; i++)
	{
		// 创建thread线程对象的时候，把线程函数给到线程对象
		auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this));
		threads_.emplace_back(std::move(ptr));
	}

	// 启动所有线程
	for (int i = 0; i < threads_.size(); i++)
	{
		threads_[i]->start();
	}
}

// 定义线程函数	线程池的所有线程从任务队列里面消费任务
void ThreadPool::threadFunc()
{
	std::cout << "begin threadFunc tid: " << std::this_thread::get_id() << std::endl;
	std::cout << "end threadFunc tid: " << std::this_thread::get_id() << std::endl;
}

/// <summary>
/// 线程方法实现
/// </summary>

Thread::Thread(ThreadFunc func)
	: func_(func)
{}

Thread::~Thread()
{}

void Thread::start()
{
	// 创建一个线程来执行线程函数
	std::thread t(func_);
	t.detach(); // 设置分离线程
}
