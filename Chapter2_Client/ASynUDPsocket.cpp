#include <iostream>
#include <string>
#include <boost/asio.hpp>

using namespace boost;
using namespace boost::asio;
using namespace boost::system;

namespace{
	io_service g_service;
	ip::udp::socket g_sock(g_service);
	ip::udp::endpoint g_sender_ep;
	char g_buf[512];
	void OnRead(const error_code &ec, size_t readbytes)
	{
		std::cout << "read" << readbytes << std::endl;
		g_sock.async_receive_from(buffer(g_buf), g_sender_ep, OnRead);
	}
}

void ASynUDPsocket()
{
	try
	{
		ip::udp::endpoint ep(ip::address::from_string("127.0.0.1"), 8001);
		g_sock.open(ep.protocol());
		g_sock.set_option(ip::udp::socket::reuse_address(true));
		g_sock.bind(ep);
		g_sock.async_receive_from(buffer(g_buf), g_sender_ep, OnRead);
		g_service.run();
	}
	catch (system::system_error &e)
	{
		std::cout << e.code() << std::endl
			<< e.what() << std::endl;
	}
}