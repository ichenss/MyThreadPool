#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <iostream>
#include <vector>
#include <queue>
#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <unordered_map>
#include <thread>
#include <future>

const int TASK_MAX_THRESHHOLD = 2; // INT32_MAX;
const int THREAD_MAX_THRESHHOLD = 1024;
const int THREAD_MAX_IDLE_TIME = 60;

/// <summary>
/// 线程池支持的模式
/// </summary>
enum class PoolMode
{
	MODE_FIXED,	 // 固定数量的线程
	MODE_CACHED, // 线程数量可动态增长
};

/// <summary>
/// 线程类型
/// </summary>
class Thread
{
public:
	// 线程函数对象类型
	using ThreadFunc = std::function<void(int)>;
	// 线程构造
	Thread(ThreadFunc func)
		: func_(func)
		, threadId_(generateId_++)
	{}
	// 线程析构
	~Thread() = default;
	// 启动线程
	void start()
	{
		// 创建一个线程来执行线程函数
		std::thread t(func_, threadId_);
		t.detach(); // 设置分离线程
	}

	int getId() const
	{
		return threadId_;
	}

private:
	ThreadFunc func_;
	static int generateId_;
	int threadId_;
};

int Thread::generateId_ = 0;

/// <summary>
/// 线程池类型
/// </summary>
class ThreadPool
{
public:
	// 线程池构造
	ThreadPool()
		: initThreadSize_(0)
		, taskSize_(0)
		, idleThreadSize_(0)
		, curThreadSize_(0)
		, taskQueMaxThreshHold_(TASK_MAX_THRESHHOLD)
		, threadSizeThreshHold_(THREAD_MAX_THRESHHOLD)
		, poolMode_(PoolMode::MODE_FIXED)
		, isPoolRunning_(false)
	{}
	// 线程池析构
	~ThreadPool()
	{
		isPoolRunning_ = false;

		// 等待线程池里所有线程返回
		std::unique_lock<std::mutex> lock(taskQueMtx_);
		notEmpty_.notify_all();
		exitCond_.wait(lock, [&]() -> bool
					{ return threads_.size() == 0; });
	}
 
	// 设置线程池工作模式
	void setMode(PoolMode mode)
	{
		if (checkRunningState())
			return;
		poolMode_ = mode;
	}

	// 设置任务队列上限阈值
	void setTaskQueMaxThreshHold(int threshhold)
	{
		if (checkRunningState())
			return;
		taskQueMaxThreshHold_ = threshhold;
	}

	// 设置线程池cached模式下线程阈值
	void setThreadSizeThreshHold(int threshhold)
	{
		if (checkRunningState())
			return;
		if (poolMode_ == PoolMode::MODE_CACHED)
		{
			threadSizeThreshHold_ = threshhold;
		}
	}

	// 给线程池提交任务
	// 使用可变参数模板编程，让submitTask可以接收任意函数和任意数量参数
	template<typename Func, typename... Args>
	auto submitTask(Func&& func, Args&&... args) -> std::future<decltype(func(args...))>
	{
		// 打包任务，放入任务队列
		using RType = decltype(func(args...));
		auto task = std::make_shared<std::packaged_task<RType()>>(
			std::bind(std::forward<Func>(func), std::forward<Args>(args)...)
		);
		std::future<RType> result = task->get_future();

		// 获取锁
		std::unique_lock<std::mutex> lock(taskQueMtx_);

		// 用户提交任务，最长不能阻塞超过1s，否则判断任务提交
		if (!notFull_.wait_for(lock, std::chrono::seconds(1),
							[&]() -> bool
							{ return taskQue_.size() < (size_t)taskQueMaxThreshHold_; }))
		{
			// 等待1s,条件依然没满足
			std::cerr << "task queue is full, submit task fail" << std::endl;
			auto task = std::make_shared<std::packaged_task<RType()>>(
				[]()->RType{ return RType(); }
			);
			(*task)();
			return task->get_future();
		}

		// 如果有空余，把任务放在任务队列中
		// taskQue_.emplace(sp);
		taskQue_.emplace([task](){ (*task)(); });
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

		return result;
	}

	// 开启线程池
	void start(int initThreadSize = std::thread::hardware_concurrency())
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

	ThreadPool(const ThreadPool &) = delete;
	ThreadPool &operator=(const ThreadPool &) = delete;

private:
	// 定义线程函数
	void threadFunc(int threadid)
	{
		auto lastTime = std::chrono::high_resolution_clock().now();
		for (;;)
		{
			Task task;
			{
				// 先获取锁
				std::unique_lock<std::mutex> lock(taskQueMtx_);

				std::cout << "tid: " << std::this_thread::get_id() << "tring task..." << std::endl;

				while (taskQue_.size() == 0)
				{
					if (!isPoolRunning_)
					{
						threads_.erase(threadid);
						std::cout << "thread id: " << std::this_thread::get_id() << "exit!" 
							<< std::endl;
						exitCond_.notify_all();
						return;	// 线程函数结束，线程结束
					}

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
				task();	// 执行function<void()>
			}
			idleThreadSize_++;
			lastTime = std::chrono::high_resolution_clock().now();
		}
	}

	bool checkRunningState() const
	{
		return isPoolRunning_;
	}

private:
	// std::vector<std::unique_ptr<Thread>> threads_;
	std::unordered_map<int, std::unique_ptr<Thread>> threads_; // 线程列表
	size_t initThreadSize_;									   // 初始的线程数量
	std::atomic_int curThreadSize_;							   // 当前线程池线程数量
	int threadSizeThreshHold_;								   // 线程数量上限阈值
	std::atomic_int idleThreadSize_;						   // 空闲线程数量

	// Task任务 =》 函数对象
	using Task = std::function<void()>;
	std::queue<Task> taskQue_; 					// 任务队列
	std::atomic_uint taskSize_;					// 任务的数量
	int taskQueMaxThreshHold_;					// 任务队列的阈值

	std::mutex taskQueMtx_;			   // 保证任务队列线程安全
	std::condition_variable notFull_;  // 表示任务队列不满
	std::condition_variable notEmpty_; // 表示任务队列不空
	std::condition_variable exitCond_; // 等待线程资源全部回收

	PoolMode poolMode_;				 // 当前线程池工作模式
	std::atomic_bool isPoolRunning_; // 线程池运行状态
};

#endif // !THREADPOOL_H
