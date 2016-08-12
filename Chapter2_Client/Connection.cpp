#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>

using namespace boost;
using namespace boost::asio;
using namespace boost::system;

namespace{
	io_service g_service;
	struct Connection : enable_shared_from_this<Connection>
	{
		typedef error_code ErrorCode;
		typedef shared_ptr<Connection> ConnectionPtr;
	private:
		ip::tcp::socket m_socket;
		enum{
			MAXMSG = 1024,
		};
		char m_read_buffer[MAXMSG];
		char m_write_buffer[MAXMSG];
		bool m_started;
	public:
		Connection() : m_socket(g_service), m_started(true){}
		void Start(ip::tcp::endpoint ep);
		void Stop();
		bool Started();
	private:
		void OnConnect(const ErrorCode &ec);
		void OnRead(const ErrorCode &ec, size_t bytes);
		void OnWrite(const ErrorCode &ec, size_t bytes);
		void doRead();
		void doWrite(const std::string &msg);
		void ProcessData(const std::string &msg);
	};

	void Connection::Start(ip::tcp::endpoint ep)
	{
		m_socket.async_connect(ep, bind(&Connection::OnConnect, shared_from_this(), _1));
	}

	void Connection::Stop()
	{
		if (!m_started)
			return;
		m_started = false;
		m_socket.close();
	}

	bool Connection::Started()
	{
		return m_started;
	}

	void Connection::OnConnect(const ErrorCode &ec)
	{
		if (!ec)
			doRead();
		else
			Stop();
	}

	void Connection::OnRead(const ErrorCode &ec, size_t bytes)
	{
		if (!Started())
			return;
		std::string msg(m_read_buffer, bytes);
		if (msg == "can_login")
			doWrite("access_data");
		else if (msg.find("data") == 0)
			ProcessData(msg);
		else if (msg == "login_fail")
			Stop();
	}

	void Connection::OnWrite(const ErrorCode &ec, size_t bytes)
	{
		doRead();
	}

	void Connection::doRead()
	{
		m_socket.async_read_some(buffer(m_read_buffer), bind(&Connection::OnRead, shared_from_this(), _1, _2));
	}

	void Connection::doWrite(const std::string &msg)
	{
		if (!Started())
			return;
		std::copy(msg.begin(), msg.end(), m_write_buffer);
		m_socket.async_write_some(buffer(m_write_buffer, msg.size()), bind(&Connection::OnWrite, shared_from_this(), _1, _2));
	}

	void Connection::ProcessData(const std::string &msg)
	{
		std::cout << "Processed!" << std::endl;
	}
}

void TestConnection()
{
	ip::tcp::endpoint ep(ip::address::from_string("127.0.0.1"), 8001);
	Connection::ConnectionPtr(new Connection)->Start(ep);
}