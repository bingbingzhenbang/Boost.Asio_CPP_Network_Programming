#include <iostream>
#include <vector>
#include <string>
#include <boost/asio.hpp>

using namespace boost;
using namespace boost::asio;

namespace{
	io_service g_service;

	struct Person
	{
		std::string m_first_name;
		std::string m_last_name;
		int m_age;
	};

	std::ostream& operator<<(std::ostream &out, const Person &p)
	{
		return out << p.m_first_name << " " << p.m_last_name << " " << p.m_age;
	}

	std::istream& operator>>(std::istream &in, Person &p)
	{
		return in >> p.m_first_name >> p.m_last_name >> p.m_age;
	}
}

void StreamClient()
{
	ip::tcp::endpoint ep(ip::address::from_string("127.0.0.1"), 8001);
	ip::tcp::socket sock(g_service);
	sock.connect(ep);
	streambuf buf;
	std::ostream out(&buf);
	Person p;
	std::cin >> p;
	out << p << std::endl;
	write(sock, buf);
}
