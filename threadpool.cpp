#include "threadpool.h"

#include <iostream>
#include <thread>

/// <summary>
/// �̳߳ط���ʵ��
/// </summary>

const int TASK_MAX_THRESHHOLD = 1024;

// ����
ThreadPool::ThreadPool()
	: initThreadSize_(0)
	, taskSize_(0)
	, taskQueMaxThreshHold_(TASK_MAX_THRESHHOLD)
	, poolMode_(PoolMode::MODE_FIXED)
{
}

// ����
ThreadPool::~ThreadPool()
{
}

// �����̳߳ع���ģʽ
void ThreadPool::setMode(PoolMode mode)
{
	poolMode_ = mode;
}

// �����������������ֵ
void ThreadPool::setTaskQueMaxThreshHold(int threshhold)
{
	taskQueMaxThreshHold_ = threshhold;
}

// ���̳߳��ύ����	�û����øýӿڣ��������������������
void ThreadPool::submitTask(std::shared_ptr<Task> sp)
{
	// ��ȡ��
	std::unique_lock<std::mutex> lock(taskQueMtx_);

	// �߳�ͨ��  �ȴ���������п���
	notFull_.wait(lock, [&]()->bool { return taskQue_.size() < taskQueMaxThreshHold_; });

	// ����п��࣬������������������
	taskQue_.emplace(sp);
	taskSize_++;

	// �·�������������п϶����գ�notEmpty֪ͨ
	notEmpty_.notify_all();
}

// �����̳߳�
void ThreadPool::start(int initThreadSize)
{
	// ��¼�̳߳�ʼ����
	initThreadSize_ = initThreadSize;

	// �����̶߳���
	for (int i = 0; i < initThreadSize_; i++)
	{
		// ����thread�̶߳����ʱ�򣬰��̺߳��������̶߳���
		auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this));
		threads_.emplace_back(std::move(ptr));
	}

	// ���������߳�
	for (int i = 0; i < threads_.size(); i++)
	{
		threads_[i]->start();
	}
}

// �����̺߳���	�̳߳ص������̴߳��������������������
void ThreadPool::threadFunc()
{
	std::cout << "begin threadFunc tid: " << std::this_thread::get_id() << std::endl;
	std::cout << "end threadFunc tid: " << std::this_thread::get_id() << std::endl;
}

/// <summary>
/// �̷߳���ʵ��
/// </summary>

Thread::Thread(ThreadFunc func)
	: func_(func)
{}

Thread::~Thread()
{}

void Thread::start()
{
	// ����һ���߳���ִ���̺߳���
	std::thread t(func_);
	t.detach(); // ���÷����߳�
}
