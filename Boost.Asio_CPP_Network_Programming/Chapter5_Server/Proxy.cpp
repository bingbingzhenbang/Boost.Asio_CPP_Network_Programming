#include <iostream>
#include <vector>
#include <string>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/ref.hpp>

using namespace boost;
using namespace boost::asio;
using namespace boost::system;

#define MEM_FN(x)            bind(&self_type::x, shared_from_this())
#define MEM_FUN1(x, y)       bind(&self_type::x, shared_from_this(), y)
#define MEM_FUN2(x, y, z)    bind(&self_type::x, shared_from_this(), y, z)
#define MEM_FUN3(x, y, z, t) bind(&self_type::x, shared_from_this(), y, z, t)

namespace{
	io_service g_service;
	class Proxy : public enable_shared_from_this<Proxy>
	{
	private:
		ip::tcp::socket m_client;
		ip::tcp::socket m_server;
		enum{
			MAXMSG = 1024,
		};
		char m_buff_client[MAXMSG];
		char m_buff_server[MAXMSG];
		int m_started;
	private:
		Proxy(ip::tcp::endpoint ep_client, ip::tcp::endpoint ep_server)
			: m_client(g_service), m_server(g_service), m_started(1)
		{
			m_client.connect(ep_client);
			m_server.connect(ep_server);
		}
	public:
		typedef Proxy self_type;
		typedef shared_ptr<Proxy> ProxyPtr;
		static ProxyPtr Start(ip::tcp::endpoint ep_client, ip::tcp::endpoint ep_server)
		{
			ProxyPtr ptr(new Proxy(ep_client, ep_server));
			return ptr;
		}
		void Stop()
		{
			m_started = 1;
			m_client.close();
			m_server.close();
		}
		bool Started()
		{
			return m_started == 2;
		}
	private:
		void OnConnect(const error_code &ec);
		void OnStart();
		void doRead(ip::tcp::socket &sock, char *buff);
		void doWrite(ip::tcp::socket &sock, char *buff, size_t size);
		void OnRead(ip::tcp::socket &sock, const error_code &ec, size_t bytes);
		void OnWrite(ip::tcp::socket &sock, const error_code &ec, size_t bytes);
		size_t ReadComplete(ip::tcp::socket &sock, const error_code &ec, size_t bytes);
	};

	void Proxy::OnConnect(const error_code &ec)
	{
		if (!ec)
		{
			if (++m_started == 2)
				OnStart();
		}
		else
			Stop();
	}

	void Proxy::OnStart()
	{
		doRead(m_client, m_buff_client);
		doRead(m_server, m_buff_server);
	}

	void Proxy::doRead(ip::tcp::socket &sock, char *buff)
	{
		async_read(sock, buffer(buff, MAXMSG), MEM_FUN3(ReadComplete, ref(sock), _1, _2), MEM_FUN3(OnRead, ref(sock), _1, _2));
	}

	void Proxy::doWrite(ip::tcp::socket &sock, char *buff, size_t size)
	{
		sock.async_write_some(buffer(buff, size), MEM_FUN3(OnWrite, ref(sock), _1, _2));
	}

	void Proxy::OnRead(ip::tcp::socket &sock, const error_code &ec, size_t bytes)
	{
		if (&sock == &m_client)
			doWrite(m_server, m_buff_client, bytes);
		else
			doWrite(m_client, m_buff_server, bytes);
	}

	void Proxy::OnWrite(ip::tcp::socket &sock, const error_code &ec, size_t bytes)
	{
		if (&sock == &m_client)
			doRead(m_server, m_buff_server);
		else
			doRead(m_client, m_buff_client);
	}

	size_t Proxy::ReadComplete(ip::tcp::socket &sock, const error_code &ec, size_t bytes)
	{
		if (sock.available() > 0)
			return sock.available();
		return bytes > 0 ? 0 : 1;
	}
}

void TestProxy()
{
	try
	{
		ip::tcp::endpoint ep_c(ip::address::from_string("127.0.0.1"), 8001);
		ip::tcp::endpoint ep_s(ip::address::from_string("127.0.0.1"), 8002);
		Proxy::Start(ep_c, ep_s);
		g_service.run();
	}
	catch (system::system_error &e)
	{
		std::cout << "proxy terminated. "
			<< e.code() << " " << e.what() << std::endl;
	}
}