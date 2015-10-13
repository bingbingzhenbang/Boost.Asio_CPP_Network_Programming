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
	ip::tcp::endpoint g_ep(ip::address::from_string("127.0.0.1"), 8001);

	size_t ReadComplete(char *buf, const error_code &ec, size_t bytes)
	{
		if (ec)
			return 0;
		bool found = (std::find(buf, buf + bytes, '\n') < buf + bytes);
		return found ? 0 : 1;
	}

	void SyncEcho(std::string msg)
	{
		msg += '\n';
		ip::tcp::socket sock(g_service);
		sock.connect(g_ep);
		sock.write_some(buffer(msg));
		char buf[BUFSIZE];
		int bytes = read(sock, buffer(buf), bind(ReadComplete, buf, _1, _2));
		std::string cpy(buf, bytes - 1);
		msg = msg.substr(0, msg.size() - 1);
		std::cout << "server echoed our " << msg << " : " << (cpy == msg ? "OK" : "FAIL") << std::endl;
		sock.close();
	}
}

void TCPSynClient()
{
	try
	{
		char *messages[] = {"John say hi", "so does James", "Lucy just got home", "Boost.Asio is fun!", 0};
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