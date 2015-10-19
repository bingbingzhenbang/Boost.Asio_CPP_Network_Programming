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
		std::string m_username;
		deadline_timer m_timer;
	private:
		TalkToSvr(const std::string &username);
		void Start(ip::tcp::endpoint ep);
	public:
		typedef error_code ErrorCode;
		typedef shared_ptr<TalkToSvr> TalkToSvrPtr;
		static TalkToSvrPtr Start(ip::tcp::endpoint ep, const std::string &username);
		void Stop();
		bool Started();
	private:
		void doPing();
		void PostponePing();
		void doAskClients();
		void doRead();
		void doWrite(const std::string &msg);
		size_t ReadComplete(const error_code &ec, size_t bytes);
		void OnConnect(const error_code &ec);
		void OnRead(const error_code &ec, size_t bytes);
		void OnLogin();
		void OnPing(const std::string &msg);
		void OnClients(const std::string &msg);
		void OnWrite(const error_code &ec, size_t bytes);
	};

	TalkToSvr::TalkToSvr(const std::string &username)
		: m_socket(g_service), m_started(true), m_username(username), m_timer(g_service){}

	void TalkToSvr::Start(ip::tcp::endpoint ep)
	{
		m_socket.async_connect(ep, MEM_FUN1(OnConnect, _1));
	}

	TalkToSvr::TalkToSvrPtr TalkToSvr::Start(ip::tcp::endpoint ep, const std::string &username)
	{
		TalkToSvrPtr ptr(new TalkToSvr(username));
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

	void TalkToSvr::doPing()
	{
		doWrite("ping\n");
	}

	void TalkToSvr::PostponePing()
	{
		m_timer.expires_from_now(posix_time::millisec(rand() % 7000));
		m_timer.async_wait(MEM_FN(doPing));
	}

	void TalkToSvr::doAskClients()
	{
		doWrite("ask_clients\n");
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
			doWrite(std::string("login ") + m_username + "\n");
		else
			Stop();
	}

	void TalkToSvr::OnRead(const error_code &ec, size_t bytes)
	{
		if (ec)
			Stop();
		if (!Started())
			return;
		std::string msg(m_read_buffer, bytes);
		if (msg.find("login ") == 0)
			OnLogin();
		else if (msg.find("ping") == 0)
			OnPing(msg);
		else if (msg.find("clients ") == 0)
			OnClients(msg);
	}

	void TalkToSvr::OnLogin()
	{
		doAskClients();
	}

	void TalkToSvr::OnPing(const std::string &msg)
	{
		std::istringstream in(msg);
		std::string answer;
		in >> answer >> answer;
		if (answer == "client_list_changed")
			doAskClients();
		else
			PostponePing();
	}

	void TalkToSvr::OnClients(const std::string &msg)
	{
		std::string clients = msg.substr(8);
		std::cout << m_username << ", new clients list: " << clients;
		PostponePing();
	}

	void TalkToSvr::OnWrite(const error_code &ec, size_t bytes)
	{
		doRead();
	}
}

void ASynClient()
{
	try
	{
		ip::tcp::endpoint ep(ip::address::from_string("127.0.0.1"), 8001);
		//char *messages[] = { "John say hi", "so does James", "Lucy just got home", "Boost.Asio is fun!", 0 };
		//for (char **msg = messages; *msg; ++msg)
		//{
		//	TalkToSvr::Start(ep, *msg);
		//	this_thread::sleep(posix_time::microsec(100));
		//}
		std::string user_name;
		std::cout << "input user name : " << std::endl;
		std::cin >> user_name;
		TalkToSvr::Start(ep, user_name);
		g_service.run();
	}
	catch (system::system_error &e)
	{
		std::cout << e.code() << std::endl
			<< e.what() << std::endl;
	}
}