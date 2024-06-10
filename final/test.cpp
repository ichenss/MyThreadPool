#include <iostream>
#include <chrono>
#include <thread>
#include <future>
#include <functional>

#include "threadpool.h"

using namespace std;

int sum1(int a, int b) 
{ 
	this_thread::sleep_for(chrono::seconds(2));
	return a + b; 
}

int main()
{
	ThreadPool pool;
	// pool.setMode(PoolMode::MODE_CACHED);
	pool.start(2);

	future<int> r1 = pool.submitTask(sum1, 100, 20);
	future<int> r2 = pool.submitTask(sum1, 100, 20);
	future<int> r3 = pool.submitTask([]()->int{
		int sum = 0;
		for (int i = 1; i < 101; ++i)
		{
			sum+=i;
		}
		return sum;
	});
	future<int> r4 = pool.submitTask([]()->int{
		int sum = 0;
		for (int i = 1; i < 101; ++i)
		{
			sum+=i;
		}
		return sum;
	});
	future<int> r5 = pool.submitTask([]()->int{
		int sum = 0;
		for (int i = 1; i < 101; ++i)
		{
			sum+=i;
		}
		return sum;
	});

	cout << r1.get() << endl;
	cout << r2.get() << endl;
	cout << r3.get() << endl;
	cout << r4.get() << endl;
	cout << r5.get() << endl;

	// packaged_task<int(int, int)> task(sum1);
	// future<int> res = task.get_future();
	// task(10, 20);

	// thread t1(move(task), 10, 20);
	// t1.detach();
	
	// cout << res.get() << endl;
	return 0;
}
