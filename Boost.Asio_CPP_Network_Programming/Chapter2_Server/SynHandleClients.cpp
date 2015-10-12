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
	const int BUFSIZE = 1024;
	io_service g_service;
	struct Client
	{
	private:
		mutex m_cs;
		bool m_is_reading;
	public:
		ip::tcp::socket m_socket;
		char m_buff[BUFSIZE];
		int m_already_read;
		Client() :m_is_reading(false), m_socket(g_service), m_already_read(0){}
		bool SetReading()
		{
			mutex::scoped_lock lk(m_cs);
			if (m_is_reading)
				return false;
			else
			{
				m_is_reading = true;
				return true;
			}
		}
		void unSetReading()
		{
			mutex::scoped_lock lk(m_cs);
			m_is_reading = false;
		}
	};
	std::vector<Client> g_clients;

	void OnReadMsg(Client &c, const std::string &msg)
	{
		if (msg == "request_login")
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

	void HandleClientsThread()
	{
		try
		{
			while (true)
			{
				for (int i = 0; i < g_clients.size(); ++i)
				{
					if (g_clients[i].m_socket.available())
					{
						if (g_clients[i].SetReading())
						{
							OnRead(g_clients[i]);
							g_clients[i].unSetReading();
						}
					}
				}
			}
		}
		catch (system::system_error &e)
		{
			std::cout << e.code() << std::endl
				<< e.what() << std::endl;
		}
	}
}

void SynHandleClients()
{
	for (int i = 0; i < 10; ++i)
	{
		thread(HandleClientsThread);
	}
}