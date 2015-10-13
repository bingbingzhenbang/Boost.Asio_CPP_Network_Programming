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
	
	void HandleConnections()
	{
		char buf[BUFSIZE];
		ip::udp::socket sock(g_service, ip::udp::endpoint(ip::udp::v4(), 8001));
		while (true)
		{
			ip::udp::endpoint sender_ep;
			int bytes = sock.receive_from(buffer(buf), sender_ep);
			std::string msg(buf, bytes);
			sock.send_to(buffer(msg), sender_ep);
		}
	}
}

void UDPSynServer()
{
	try
	{
		HandleConnections();
	}
	catch (system::system_error &e)
	{
		std::cout << e.code() << std::endl
			<< e.what() << std::endl;
	}
}