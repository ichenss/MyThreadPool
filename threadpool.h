#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <vector>
#include <queue>
#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <functional>

/// <summary>
/// Any类型，可以接收任意数据的类型
/// </summary>
class Any
{
public:
	Any() = default;
	~Any() = default;
	Any(const Any&) = delete;
	Any& operator=(const Any&) = delete;
	Any(Any&&) = default;
	Any& operator=(Any&&) = default;

	template<typename T>
	Any(T data) : base_(std::make_unique<Derive<T>>(data))
	{}

	// 提取data_数据
	template<typename T>
	T cast_()
	{
		// 基类指针  -》 派生类指针    RTTI
		Derive<T>* pd = dynamic_cast<Derive<T>*>(base_.get());
		if (pd == nullptr)
		{
			throw "type is unmatch!";
		}
		return pd->data_;
	}
private:
	// 基类类型
	class Base
	{
	public:
		virtual ~Base() = default;
	};

	// 派生类类型
	template<typename T>
	class Derive : public Base
	{
	public:
		Derive(T data) : data_(data)
		{}

	private:
		T data_;	// 保存了任意的其他类型
	};
private:
	// 基类指针
	std::unique_ptr<Base> base_;
};

/// <summary>
/// 任务抽象基类
/// </summary>
class Task
{
public:
	// 用户可以自定义任意任务类型，继承于Task，重写run方法，实现自定义任务处理
	virtual Any run() = 0;
};

/// <summary>
/// 线程池支持的模式
/// </summary>
enum class PoolMode
{
	MODE_FIXED,	// 固定数量的线程
	MODE_CACHED,// 线程数量可动态增长
};

/// <summary>
/// 线程类型
/// </summary>
class Thread 
{
public:
	// 线程函数对象类型
	using ThreadFunc = std::function<void()>;
	// 线程构造
	Thread(ThreadFunc func);
	// 线程析构
	~Thread();
	// 启动线程
	void start();

private:
	ThreadFunc func_;
};

/// <summary>
/// 线程池类型
/// </summary>
class ThreadPool
{
public:
	// 线程池构造
	ThreadPool();
	// 线程池析构
	~ThreadPool();

	// 设置线程池工作模式
	void setMode(PoolMode mode);

	// 设置任务队列上限阈值
	void setTaskQueMaxThreshHold(int threshhold);
	
	// 给线程池提交任务
	void submitTask(std::shared_ptr<Task> sp);

	// 开启线程池
	void start(int initThreadSize = 4);

	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;

private:
	// 定义线程函数
	void threadFunc();

private:
	std::vector<std::unique_ptr<Thread>> threads_; // 线程列表
	size_t initThreadSize_; // 初始的线程数量

	std::queue<std::shared_ptr<Task>> taskQue_; // 任务队列
	std::atomic_uint taskSize_; // 任务的数量
	int taskQueMaxThreshHold_; // 任务队列的阈值

	std::mutex taskQueMtx_; // 保证任务队列线程安全
	std::condition_variable notFull_; // 表示任务队列不满
	std::condition_variable notEmpty_; // 表示任务队列不空

	PoolMode poolMode_; // 当前线程池工作模式
};


#endif // !THREADPOOL_H