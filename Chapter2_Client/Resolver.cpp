#include <iostream>
#include <string>
#include <boost/asio.hpp>

using namespace boost;
using namespace boost::asio;
using namespace boost::system;

void Resolver()
{
	io_service service;
	ip::tcp::resolver rlv(service);
	ip::tcp::resolver::query qry("www.baidu.com", "80");
	ip::tcp::resolver::iterator iter = rlv.resolve(qry), end;
	if (iter != end)
		std::cout << iter->endpoint().address().to_string() << std::endl;
}