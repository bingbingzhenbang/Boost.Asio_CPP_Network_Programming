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

	typedef shared_ptr<ip::tcp::socket> SocketPtr;
}

void StreamServer()
{
	try
	{
		ip::tcp::endpoint ep(ip::tcp::v4(), 8001);
		ip::tcp::acceptor acc(g_service, ep);
		while (true)
		{
			SocketPtr sock(new ip::tcp::socket(g_service));
			acc.accept(*sock);
			streambuf buf;
			if (read_until(*sock, buf, "\n"))
			{
				std::istream in(&buf);
				Person p;
				in >> p;
				std::cout << p << std::endl;
			}
		}
	}
	catch (std::exception &e)
	{
		std::cout << e.what() << std::endl;
	}
}
