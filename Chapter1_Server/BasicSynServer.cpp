#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <iostream>

using namespace boost;
using namespace boost::asio;

namespace{
	typedef shared_ptr<ip::tcp::socket> SocketPtr;
	const int SIZE = 512;
	void ClientSession(SocketPtr sock)
	{
		try
		{
			char data[SIZE] = {'\0'};
			size_t len = sock->read_some(buffer(data));
			std::cout << data << std::endl;
			if (len > 0)
				write(*sock, (buffer("ok")));
		}
		catch (std::exception &e)
		{
			std::cout << e.what() << std::endl;
		}
	}
}

void BasicSynServer()
{
	try
	{
		io_service service;
		ip::tcp::endpoint ep(ip::tcp::v4(), 2001);
		ip::tcp::acceptor acc(service, ep);
		while (true)
		{
			SocketPtr sock(new ip::tcp::socket(service));
			acc.accept(*sock);
			thread(bind(ClientSession, sock));
		}
	}
	catch (std::exception &e)
	{
		std::cout << e.what() << std::endl;
	}
}