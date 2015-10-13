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

	void HandleConnections()
	{
		try
		{
			ip::tcp::acceptor accp(g_service, g_ep);
			char buf[BUFSIZE];
			while (true)
			{
				ip::tcp::socket sock(g_service);
				accp.accept(sock);
				int bytes = read(sock, buffer(buf), bind(ReadComplete, buf, _1, _2));
				std::string msg(buf, bytes);
				sock.write_some(buffer(msg));
				sock.close();
			}
		}
		catch (system::system_error &e)
		{
			std::cout << e.code() << std::endl
				<< e.what() << std::endl;
		}
	}
}

void TCPSynServer()
{
	HandleConnections();
}