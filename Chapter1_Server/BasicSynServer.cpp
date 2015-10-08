#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>

using namespace boost;
using namespace boost::asio;

namespace{
	typedef shared_ptr<ip::tcp::socket> SocketPtr;
	const int SIZE = 512;

	void ClientSession(SocketPtr sock)
	{
		char data[SIZE];
		size_t len = sock->read_some(buffer(data));
		if (len > 0)
			sock->write_some(buffer("ok"));
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