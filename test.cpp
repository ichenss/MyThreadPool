#include <iostream>
#include <chrono>
#include <thread>

#include "threadpool.h"

using uLong = unsigned long long;
class MyTask : public Task
{
public:
	MyTask(int begin, int end)
		: begin_(begin), end_(end)
	{
	}

	Any run()
	{
		std::cout << "tid: " << std::this_thread::get_id() << "begin!" << std::endl;
		std::this_thread::sleep_for(std::chrono::seconds(3));
		uLong sum = 0;
		for (uLong i = begin_; i <= end_; ++i)
		{
			sum += i;
		}
		std::cout << "tid: " << std::this_thread::get_id() << "end!" << std::endl;
		return sum;
	}

private:
	int begin_;
	int end_;
};

int main()
{
	{
		ThreadPool pool;
		pool.setMode(PoolMode::MODE_CACHED);
		pool.start(2);

		Result res1 = pool.submitTask(std::make_shared<MyTask>(1, 100000000));
		Result res2 = pool.submitTask(std::make_shared<MyTask>(100000000, 200000000));
		pool.submitTask(std::make_shared<MyTask>(100000000, 200000000));
		pool.submitTask(std::make_shared<MyTask>(100000000, 200000000));
		pool.submitTask(std::make_shared<MyTask>(100000000, 200000000));

		uLong sum1 = res1.get().cast_<uLong>();
		std::cout << sum1 << std::endl;
	}
	std::cout << "main over" << std::endl;
	getchar();
#if 0
	{
		ThreadPool pool;
		pool.setMode(PoolMode::MODE_CACHED);
		pool.start(4);

		Result res1 = pool.submitTask(std::make_shared<MyTask>(1, 100000000));
		Result res2 = pool.submitTask(std::make_shared<MyTask>(100000001, 200000000));
		Result res3 = pool.submitTask(std::make_shared<MyTask>(200000001, 300000000));
		pool.submitTask(std::make_shared<MyTask>(200000001, 300000000));
		pool.submitTask(std::make_shared<MyTask>(200000001, 300000000));
		pool.submitTask(std::make_shared<MyTask>(200000001, 300000000));
		uLong r1 = res1.get().cast_<uLong>();
		uLong r2 = res2.get().cast_<uLong>();
		uLong r3 = res3.get().cast_<uLong>();

		/*
		Master - Slave线程模型
		分解-执行-合并
		*/
		std::cout << (r1 + r2 + r2) << std::endl;
	}

	getchar();
#endif
	// return 0;
}
