#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>

using namespace boost;
using namespace boost::asio;
using namespace boost::system;

namespace{
	io_service g_service;
	typedef shared_ptr<ip::tcp::socket> SocketPtr;
	typedef shared_ptr<streambuf> BufPtr;
	struct Client
	{
		SocketPtr m_socket;
		BufPtr m_buff;
		Client() : m_socket(new ip::tcp::socket(g_service)), m_buff(new streambuf){}
	};
	std::vector<Client> g_clients;

	void OnRead(Client &c, const error_code &ec, size_t read_bytes)
	{
		std::istream in(&*c.m_buff);
		std::string msg;
		std::getline(in, msg);
		if (msg == "request_login")
		{
		}
		async_read_until(*c.m_socket, *c.m_buff, '\n', bind(OnRead, c, _1, _2));
	}

	void HandleClientsThread()
	{
		g_service.run();
	}
}

void ASynHandleClients()
{
	for (int i = 0; i < g_clients.size(); ++i)
	{
		async_read_until(*g_clients[i].m_socket, *g_clients[i].m_buff, '\n', bind(OnRead, g_clients[i], _1, _2));
	}
	for (int i = 0; i < 10; ++i)
	{
		thread(HandleClientsThread);
	}
}