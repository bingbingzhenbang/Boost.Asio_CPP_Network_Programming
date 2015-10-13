#include <iostream>
#include <vector>
#include <string>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>

using namespace boost;
using namespace boost::asio;
using namespace boost::system;

namespace{
	const int BUFSIZE = 1024;
	io_service g_service;
	ip::udp::endpoint g_ep(ip::address::from_string("127.0.0.1"), 8001);

	void SyncEcho(std::string msg)
	{
		ip::udp::socket sock(g_service, ip::udp::endpoint(ip::udp::v4(), 0));
		sock.send_to(buffer(msg), g_ep);
		char buf[BUFSIZE];
		ip::udp::endpoint sender_ep;
		int bytes = sock.receive_from(buffer(buf), sender_ep);
		std::string cpy(buf, bytes);
		std::cout << "server echoed our " << msg << " : " << (cpy == msg ? "OK" : "FAIL") << std::endl;
		sock.close();
	}
}

void UDPSynClient()
{
	try
	{
		char *messages[] = { "John say hi", "so does James", "Lucy just got home", "Boost.Asio is fun!", 0 };
		thread_group threads;
		for (char **msg = messages; *msg; ++msg)
		{
			threads.create_thread(bind(SyncEcho, *msg));
			this_thread::sleep(posix_time::microsec(100));
		}
		threads.join_all();
	}
	catch (system::system_error &e)
	{
		std::cout << e.code() << std::endl
			<< e.what() << std::endl;
	}
}