#include <boost/asio.hpp>
#include <iostream>

using namespace boost;
using namespace boost::asio;

void BasicSynClient()
{
	try
	{
		io_service service;
		ip::tcp::endpoint ep(ip::address::from_string("127.0.0.1"), 2001);
		ip::tcp::socket sock(service);
		sock.connect(ep);
		sock.write_some(buffer("Hello Server"));
	}
	catch (std::exception &e)
	{
		std::cout << e.what() << std::endl;
	}
}