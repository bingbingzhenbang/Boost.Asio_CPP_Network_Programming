#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/function.hpp>

using namespace boost;
using namespace boost::asio;

namespace{
	io_service g_service;

	void DispatchedFunc1()
	{
		std::cout << "Dispatched 1" << std::endl;
	}

	void DispatchedFunc2()
	{
		std::cout << "Dispatched 2" << std::endl;
	}

	void test(function<void()> func)
	{
		std::cout << "test !" << std::endl;
		g_service.dispatch(DispatchedFunc1);
		func();
	}

	void ServicesRun()
	{
		g_service.run();
	}
}

void Dispatch()
{
	test(g_service.wrap(DispatchedFunc2));
	thread th(ServicesRun);
	this_thread::sleep(posix_time::microsec(500));
	th.join();
}