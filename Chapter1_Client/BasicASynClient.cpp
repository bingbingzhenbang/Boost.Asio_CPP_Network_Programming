#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>
#include <iostream>

using namespace boost;
using namespace boost::asio;

namespace{
	typedef shared_ptr<ip::tcp::socket> SocketPtr;
	void ConnectHandler(const system::error_code &ec, SocketPtr sock)
	{
		if (ec)
			return;
		sock->write_some(buffer("Message form client, Hello server!"));
	}
}


void BasicASynClient()
{
	try
	{
		io_service service;
		ip::tcp::endpoint ep(ip::address::from_string("127.0.0.1"), 6688);
		SocketPtr sock(new ip::tcp::socket(service));
		sock->async_connect(ep, bind(ConnectHandler, boost::asio::placeholders::error, sock));
		service.run();
	}
	catch (system::system_error &e)
	{
		std::cout << e.code() << std::endl;
	}
}