#include "threadpool.h"

#include <functional>

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

// ���̳߳��ύ����
void ThreadPool::submitTask(std::shared_ptr<Task> sp)
{
}

// �����̳߳�
void ThreadPool::start(int initThreadSize)
{
	// ��¼�̳߳�ʼ����
	initThreadSize_ = initThreadSize;

	// �����̶߳���
	for (int i = 0; i < threads_.size(); i++)
	{
		// ����thread�̶߳����ʱ�򣬰��̺߳��������̶߳���
		threads_.emplace_back(new Thread(std::bind(ThreadPool::threadFunc(), this)));
	}

	// ���������߳�
	for (int i = 0; i < threads_.size(); i++)
	{
		threads_[i]->start();
	}
}

void ThreadPool::threadFunc()
{
}

/// <summary>
/// �̷߳���ʵ��
/// </summary>

void Thread::start()
{
}
