#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>

using namespace boost;
using namespace boost::asio;

namespace{
	io_service g_service;

	void func(int i)
	{
		std::cout << "func called, i = " << i << "/"
			<<this_thread::get_id()<< std::endl;
	}

	void WorkerThread()
	{
		g_service.run();
	}

	void RunDispatchAndPost()
	{
		for (int i = 0; i < 10; i += 2)
		{
			g_service.dispatch(bind(func, i));
			g_service.post(bind(func, i + 1));
		}
	}
}

void ASynWork()
{
	//io_service::strand srand_one(g_service), srand_two(g_service);
	//for (int i = 0; i < 5; ++i)
	//{
	//	g_service.post(srand_one.wrap(bind(func, i)));
	//}
	//for (int i = 5; i < 10; ++i)
	//{
	//	g_service.post(srand_two.wrap(bind(func, i)));
	//}
	//thread_group threads;
	//for (int i = 0; i < 3; ++i)
	//{
	//	threads.create_thread(WorkerThread);
	//}
	//this_thread::sleep(posix_time::millisec(500));
	//threads.join_all();

	g_service.post(RunDispatchAndPost);
	g_service.run();
}