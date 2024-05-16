#include "threadpool.h"

#include <functional>

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

// 给线程池提交任务
void ThreadPool::submitTask(std::shared_ptr<Task> sp)
{
}

// 开启线程池
void ThreadPool::start(int initThreadSize)
{
	// 记录线程初始个数
	initThreadSize_ = initThreadSize;

	// 创建线程对象
	for (int i = 0; i < threads_.size(); i++)
	{
		// 创建thread线程对象的时候，把线程函数给到线程对象
		threads_.emplace_back(new Thread(std::bind(ThreadPool::threadFunc(), this)));
	}

	// 启动所有线程
	for (int i = 0; i < threads_.size(); i++)
	{
		threads_[i]->start();
	}
}

void ThreadPool::threadFunc()
{
}

/// <summary>
/// 线程方法实现
/// </summary>

void Thread::start()
{
}
