#include <iostream>
#include <string>
#include <boost/asio.hpp>

using namespace boost;
using namespace boost::asio;
using namespace boost::system;

void SynUDPsocket()
{
	try
	{
		io_service serivce;
		ip::udp::socket sock(serivce);
		sock.open(ip::udp::v4());
		ip::udp::endpoint receiver_ep(ip::address::from_string("87.248.112.181"), 80);
		sock.send_to(buffer("testing\n"), receiver_ep);
		char buf[512];
		ip::udp::endpoint sender_ep;
		sock.receive_from(buffer(buf), sender_ep);
	}
	catch (system::system_error &e)
	{
		std::cout << e.code() << std::endl
			<< e.what() << std::endl;
	}
}