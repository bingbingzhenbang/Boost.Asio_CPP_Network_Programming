#include <iostream>
#include <string>
#include <boost/asio.hpp>

using namespace boost;
using namespace boost::asio;
using namespace boost::system;

void SynTCPsocket()
{
	try
	{
		io_service serivce;
		ip::tcp::endpoint ep(ip::address::from_string("127.0.0.1"), 6688);
		ip::tcp::socket sock(serivce);
		sock.connect(ep);
		sock.write_some(buffer("GET /index.html\r\n"));
		std::cout << "bytes available" << sock.available() << std::endl;
		char buf[512];
		error_code ec;
		size_t read = sock.read_some(buffer(buf), ec);
		if (ec == error::eof)
			std::cout << "EOF" << std::endl;
		else
			std::cout << buf << std::endl;
	}
	catch (system::system_error &e)
	{
		std::cout << e.code() <<std::endl
			<<e.what()<< std::endl;
	}
}