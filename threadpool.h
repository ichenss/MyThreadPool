#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <vector>
#include <queue>
#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>

/// <summary>
/// ����������
/// </summary>
class Task
{
public:
	// �û������Զ��������������ͣ��̳���Task����дrun������ʵ���Զ���������
	virtual void run() = 0;
};

// �̳߳�֧�ֵ�ģʽ
enum class PoolMode
{
	MODE_FIXED,	// �̶��������߳�
	MODE_CACHED,// �߳������ɶ�̬����
};

/// <summary>
/// �߳�����
/// </summary>
class Thread 
{
public:
	// �����߳�
	void start();

private:

};

/// <summary>
/// �̳߳�����
/// </summary>
class ThreadPool
{
public:
	// �̳߳ع���
	ThreadPool();
	// �̳߳�����
	~ThreadPool();

	// �����̳߳ع���ģʽ
	void setMode(PoolMode mode);

	// �����������������ֵ
	void setTaskQueMaxThreshHold(int threshhold);
	
	// ���̳߳��ύ����
	void submitTask(std::shared_ptr<Task> sp);

	// �����̳߳�
	void start(int initThreadSize = 4);

	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;

private:
	// �����̺߳���
	void threadFunc();

private:
	std::vector<Thread*> threads_; // �߳��б�
	size_t initThreadSize_; // ��ʼ���߳�����

	std::queue<std::shared_ptr<Task>> taskQue_; // �������
	std::atomic_uint taskSize_; // ���������
	int taskQueMaxThreshHold_; // ������е���ֵ

	std::mutex taskQueMtx_; // ��֤��������̰߳�ȫ
	std::condition_variable notFull_; // ��ʾ������в���
	std::condition_variable notEmpty_; // ��ʾ������в���

	PoolMode poolMode_; // ��ǰ�̳߳ع���ģʽ
};


#endif // !THREADPOOL_H