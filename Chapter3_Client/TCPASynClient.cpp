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

#define MEM_FN(x)         bind(&self_type::x, shared_from_this())
#define MEM_FUN1(x, y)    bind(&self_type::x, shared_from_this(), y)
#define MEM_FUN2(x, y, z) bind(&self_type::x, shared_from_this(), y, z)

namespace{
	io_service g_service;
	class TalkToSvr : public enable_shared_from_this<TalkToSvr>,
		asio::detail::noncopyable
	{
	private:
		typedef TalkToSvr self_type;
		ip::tcp::socket m_socket;
		enum{
			MAXMSG = 1024,
		};
		char m_read_buffer[MAXMSG];
		char m_write_buffer[MAXMSG];
		bool m_started;
		std::string m_message;
	private:
		TalkToSvr(const std::string &message);
		void Start(ip::tcp::endpoint ep);
	public:
		typedef error_code ErrorCode;
		typedef shared_ptr<TalkToSvr> TalkToSvrPtr;
		static TalkToSvrPtr Start(ip::tcp::endpoint ep, const std::string &message);
		void Stop();
		bool Started();
	private:
		void doRead();
		void doWrite(const std::string &msg);
		size_t ReadComplete(const error_code &ec, size_t bytes);
		void OnConnect(const error_code &ec);
		void OnRead(const error_code &ec, size_t bytes);
		void OnWrite(const error_code &ec, size_t bytes);
	};

	TalkToSvr::TalkToSvr(const std::string &message)
		: m_socket(g_service), m_started(true), m_message(message){}

	void TalkToSvr::Start(ip::tcp::endpoint ep)
	{
		m_socket.async_connect(ep, MEM_FUN1(OnConnect, _1));
	}

	TalkToSvr::TalkToSvrPtr TalkToSvr::Start(ip::tcp::endpoint ep, const std::string &message)
	{
		TalkToSvrPtr ptr(new TalkToSvr(message));
		ptr->Start(ep);
		return ptr;
	}

	void TalkToSvr::Stop()
	{
		if (!m_started)
			return;
		m_started = false;
		m_socket.close();
	}

	bool TalkToSvr::Started()
	{
		return m_started;
	}

	void TalkToSvr::doRead()
	{
		async_read(m_socket, buffer(m_read_buffer), MEM_FUN2(ReadComplete, _1, _2), MEM_FUN2(OnRead, _1, _2));
	}

	void TalkToSvr::doWrite(const std::string &msg)
	{
		if (!Started())
			return;
		std::copy(msg.begin(), msg.end(), m_write_buffer);
		m_socket.async_write_some(buffer(m_write_buffer, msg.size()), MEM_FUN2(OnWrite, _1, _2));
	}

	size_t TalkToSvr::ReadComplete(const error_code &ec, size_t bytes)
	{
		if (ec)
			return 0;
		bool found = (std::find(m_read_buffer, m_read_buffer + bytes, '\n') < m_read_buffer + bytes);
		return found ? 0 : 1;
	}

	void TalkToSvr::OnConnect(const error_code &ec)
	{
		if (!ec)
			doWrite(m_message + "\n");
		else
			Stop();
	}

	void TalkToSvr::OnRead(const error_code &ec, size_t bytes)
	{
		if (!ec)
		{
			std::string cpy(m_read_buffer, bytes - 1);
			std::cout << "server echoed our " << m_message << " : " << (cpy == m_message ? "OK" : "FAIL") << std::endl;
		}
		Stop();
	}

	void TalkToSvr::OnWrite(const error_code &ec, size_t bytes)
	{
		doRead();
	}
}

void TCPASynClient()
{
	try
	{
		ip::tcp::endpoint ep(ip::address::from_string("127.0.0.1"), 8001);
		char *messages[] = { "John say hi", "so does James", "Lucy just got home", "Boost.Asio is fun!", 0 };
		for (char **msg = messages; *msg; ++msg)
		{
			TalkToSvr::Start(ep, *msg);
			this_thread::sleep(posix_time::microsec(100));
		}
		g_service.run();
	}
	catch (system::system_error &e)
	{
		std::cout << e.code() << std::endl
			<< e.what() << std::endl;
	}
}