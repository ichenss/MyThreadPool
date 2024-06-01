#include "threadpool.h"

#include <iostream>
#include <thread>

/// <summary>
/// 线程池方法实现
/// </summary>
const int TASK_MAX_THRESHHOLD = INT32_MAX;
const int THREAD_MAX_THRESHHOLD = 10;
const int THREAD_MAX_IDLE_TIME = 10;

// 构造
ThreadPool::ThreadPool()
	: initThreadSize_(0), taskSize_(0), idleThreadSize_(0),
	  curThreadSize_(0), taskQueMaxThreshHold_(TASK_MAX_THRESHHOLD),
	  threadSizeThreshHold_(THREAD_MAX_THRESHHOLD),
	  poolMode_(PoolMode::MODE_FIXED), isPoolRunning_(false)
{
}

// 析构
ThreadPool::~ThreadPool()
{
	isPoolRunning_ = false;
	notEmpty_.notify_all();
	// 等待线程池里所有线程返回
	std::unique_lock<std::mutex> lock(taskQueMtx_);
	exitCond_.wait(lock, [&]() -> bool
				   { return threads_.size() == 0; });
}

// 设置线程池工作模式
void ThreadPool::setMode(PoolMode mode)
{
	if (checkRunningState())
		return;
	poolMode_ = mode;
}

// 设置任务队列上限阈值
void ThreadPool::setTaskQueMaxThreshHold(int threshhold)
{
	if (checkRunningState())
		return;
	taskQueMaxThreshHold_ = threshhold;
}

// 设置线程池cached模式下线程阈值
void ThreadPool::setThreadSizeThreshHold(int threshhold)
{
	if (checkRunningState())
		return;
	if (poolMode_ == PoolMode::MODE_CACHED)
	{
		threadSizeThreshHold_ = threshhold;
	}
}

// 给线程池提交任务	用户调用该接口，传入任务对象，生产任务
Result ThreadPool::submitTask(std::shared_ptr<Task> sp)
{
	// 获取锁
	std::unique_lock<std::mutex> lock(taskQueMtx_);

	// 线程通信  等待任务队列有空余
	// 用户提交任务，最长不能阻塞超过1s，否则判断任务提交
	if (!notFull_.wait_for(lock, std::chrono::seconds(1),
						   [&]() -> bool
						   { return taskQue_.size() < (size_t)taskQueMaxThreshHold_; }))
	{
		// 等待1s,条件依然没满足
		std::cerr << "task queue is full, submit task fail" << std::endl;
		return Result(sp, false);
	}

	// 如果有空余，把任务放在任务队列中
	taskQue_.emplace(sp);
	taskSize_++;

	// 新放了任务，任务队列肯定不空，notEmpty通知
	notEmpty_.notify_all();

	// cached模式
	if (poolMode_ == PoolMode::MODE_CACHED && taskSize_ > idleThreadSize_ && curThreadSize_ < threadSizeThreshHold_)
	{
		// 创建新线程
		auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1));
		int threadId = ptr->getId();
		threads_.emplace(threadId, std::move(ptr));
		// 启动线程
		threads_[threadId]->start();
		// 修改线程数量变量
		curThreadSize_++;
		idleThreadSize_++;
	}

	return Result(sp);
}

// 开启线程池
void ThreadPool::start(int initThreadSize)
{
	// 设置线程池运行状态
	isPoolRunning_ = true;

	// 记录线程初始个数
	initThreadSize_ = initThreadSize;
	curThreadSize_ = initThreadSize;

	// 创建线程对象
	for (int i = 0; i < initThreadSize_; i++)
	{
		// 创建thread线程对象的时候，把线程函数给到线程对象
		auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1));
		int threadId = ptr->getId();
		threads_.emplace(threadId, std::move(ptr));
		// threads_.emplace_back(std::move(ptr));
	}

	// 启动所有线程
	for (int i = 0; i < threads_.size(); i++)
	{
		threads_[i]->start();
		idleThreadSize_++;
	}
}

// 定义线程函数	线程池的所有线程从任务队列里面消费任务
void ThreadPool::threadFunc(int threadid)
{
	auto lastTime = std::chrono::high_resolution_clock().now();
	while (isPoolRunning_)
	{
		std::shared_ptr<Task> task;
		{
			// 先获取锁
			std::unique_lock<std::mutex> lock(taskQueMtx_);

			std::cout << "tid: " << std::this_thread::get_id() << "tring task..." << std::endl;

			while (taskQue_.size() == 0)
			{
				if (poolMode_ == PoolMode::MODE_CACHED)
				{
					if (std::cv_status::timeout == notEmpty_.wait_for(lock, std::chrono::seconds(1)))
					{
						auto now = std::chrono::high_resolution_clock().now();
						auto dur = std::chrono::duration_cast<std::chrono::seconds>(now - lastTime);
						if (dur.count() >= THREAD_MAX_IDLE_TIME && curThreadSize_ > initThreadSize_)
						{
							// 回收线程
							threads_.erase(threadid);
							curThreadSize_--;
							idleThreadSize_--;
							std::cout << "thread id: " << std::this_thread::get_id() << "exit!" << std::endl;
							return;
						}
					}
				}
				else
				{
					// 等待notEmpty条件
					notEmpty_.wait(lock);
				}

				if (!isPoolRunning_)
				{
					threads_.erase(threadid);
					exitCond_.notify_all();
					std::cout << "thread id: " << std::this_thread::get_id() << "exit!" << std::endl;
					return;
				}
			}

			idleThreadSize_--;

			std::cout << "tid: " << std::this_thread::get_id() << "task OK..." << std::endl;

			// 从任务队列中取一个任务出来
			task = taskQue_.front();
			taskQue_.pop();
			taskSize_--;

			// 如果依然有任务，通知其他线程
			if (taskQue_.size() > 0)
			{
				notEmpty_.notify_all();
			}

			// 取出一个任务，进行通知
			notFull_.notify_all();

		} // 出作用域释放锁

		// 当前线程负责执行这个任务
		if (task != nullptr)
		{
			// task->run();
			task->exec();
		}
		idleThreadSize_++;
		lastTime = std::chrono::high_resolution_clock().now();
	}
	threads_.erase(threadid);
	exitCond_.notify_all();
	std::cout << "thread id: " << std::this_thread::get_id() << "exit!" << std::endl;
}

bool ThreadPool::checkRunningState() const
{
	return isPoolRunning_;
}

/// <summary>
/// 线程方法实现
/// </summary>
int Thread::generateId_ = 0;

Thread::Thread(ThreadFunc func)
	: func_(func), threadId_(generateId_++)
{
}

Thread::~Thread()
{
}

void Thread::start()
{
	// 创建一个线程来执行线程函数
	std::thread t(func_, threadId_);
	t.detach(); // 设置分离线程
}

int Thread::getId() const
{
	return threadId_;
}

/// <summary>
/// Task方法实现
/// </summary>
Task::Task()
	: result_(nullptr)
{
}

void Task::exec()
{
	if (result_ != nullptr)
		result_->setVal(run()); // 这里发生多态调用
}

void Task::setResult(Result *res)
{
	result_ = res;
}

/// <summary>
/// Result方法实现
/// </summary>
Result::Result(std::shared_ptr<Task> task, bool isValid)
	: isValid_(isValid), task_(task)
{
	task_->setResult(this);
}

Any Result::get()
{
	if (!isValid_)
		return "";
	sem_.wait(); // task任务如果没有执行完，这里会阻塞用户的线程
	return std::move(any_);
}

void Result::setVal(Any any)
{
	this->any_ = std::move(any);
	sem_.post(); // 已经获取的任务的返回值，增加信号量资源
}
