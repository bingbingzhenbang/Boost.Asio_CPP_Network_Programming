#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <boost/asio.hpp>
#include <boost/bind.hpp>

using namespace boost;
using namespace boost::asio;
using namespace boost::system;

namespace{
	const int BUFSIZE = 1024;
	struct Client
	{
		ip::tcp::socket m_socket;
		char m_buff[BUFSIZE];
		int m_already_read;
	};
	std::vector<Client> g_clients;

	void OnReadMsg(Client &c, const std::string &msg)
	{
		if (msg == "request login")
			c.m_socket.write_some(buffer("request ok\n"));
	}

	void OnRead(Client &c)
	{
		int to_read = std::min<int>(BUFSIZE - c.m_already_read, c.m_socket.available());
		c.m_socket.read_some(buffer(c.m_buff + c.m_already_read, to_read));
		c.m_already_read += to_read;
		char *pos_char = std::find(c.m_buff, c.m_buff + c.m_already_read, '\n');
		if (pos_char < c.m_buff + c.m_already_read)
		{
			std::string msg(c.m_buff, pos_char);
			std::copy(pos_char, c.m_buff + BUFSIZE, c.m_buff);
			c.m_already_read -= (pos_char - c.m_buff);
			OnReadMsg(c, msg);
		}
	}
}

void ASynHandleClients()
{
	try
	{
		while (true)
		{
			for (int i = 0; i < g_clients.size(); ++i)
			{
				if (g_clients[i].m_socket.available())
					OnRead(g_clients[i]);
			}
		}
	}
	catch (system::system_error &e)
	{
		std::cout << e.code() << std::endl
			<< e.what() << std::endl;
	}
}