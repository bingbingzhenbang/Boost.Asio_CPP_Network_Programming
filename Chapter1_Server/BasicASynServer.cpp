#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>
#include <iostream>

using namespace boost;
using namespace boost::asio;

namespace{
	typedef shared_ptr<ip::tcp::socket> SocketPtr;
	const int SIZE = 512;
	io_service g_service;
	ip::tcp::endpoint g_ep(ip::tcp::v4(), 6688);
	ip::tcp::acceptor g_acc(g_service, g_ep);

	void StartAccept(SocketPtr);
	void HandleAccept(const system::error_code &, SocketPtr);

	void StartAccept(SocketPtr sock)
	{
		g_acc.async_accept(*sock, bind(HandleAccept, boost::asio::placeholders::error, sock));
	}

	void HandleAccept(const system::error_code &err, SocketPtr sock)
	{
		if (err)
			return;
		char data[SIZE] = { '\0' };
		system::error_code errorcode;
		size_t len = sock->read_some(buffer(data), errorcode);
		if (errorcode == error::eof)
		{
			std::cout << "EOF !" << std::endl;
		}
		else
		{
			std::cout << "Client : " << sock->remote_endpoint().address() << std::endl;
			std::cout << data << std::endl;
		}
		sock.reset(new ip::tcp::socket(g_service));
		StartAccept(sock);
	}
}

void BasicASynServer()
{
	try
	{
		SocketPtr sock(new ip::tcp::socket(g_service));
		StartAccept(sock);
		g_service.run();
	}
	catch (system::system_error &e)
	{
		std::cout << e.code() << std::endl;
	}
}