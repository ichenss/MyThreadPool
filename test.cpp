#include <iostream>
#include <chrono>
#include <thread>

#include "threadpool.h"

class MyTask : public Task
{
public:
	MyTask(int begin, int end) 
		: begin_(begin), end_(end)
	{} 

	Any run()
	{
		std::cout << "tid: " << std::this_thread::get_id() << "begin!" << std::endl;
		// std::this_thread::sleep_for(std::chrono::seconds(2));
		int sum = 0;
		for (int i = begin_; i <= end_; ++i)
		{
			sum += i;
		}
		std::cout << "tid: " << std::this_thread::get_id() << "end!" << std::endl;
	}

private:
	int begin_;
	int end_;
};

int main()
{
	ThreadPool pool;
	pool.start(4);

	pool.submitTask(std::make_shared<MyTask>());
	pool.submitTask(std::make_shared<MyTask>());
	pool.submitTask(std::make_shared<MyTask>());

	getchar();
	return 0;
}
