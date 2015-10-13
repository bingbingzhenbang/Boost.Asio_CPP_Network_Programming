#include <iostream>
#include <vector>
#include <string>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>

using namespace boost;
using namespace boost::asio;
using namespace boost::system;

#define MEM_FN(x)         bind(&self_type::x, shared_from_this())
#define MEM_FUN1(x, y)    bind(&self_type::x, shared_from_this(), y)
#define MEM_FUN2(x, y, z) bind(&self_type::x, shared_from_this(), y, z)

namespace{
	io_service g_service;
	class TalkToClient : public enable_shared_from_this<TalkToClient>,
		asio::detail::noncopyable
	{
	private:
		typedef TalkToClient self_type;
		ip::tcp::socket m_socket;
		enum{
			MAXMSG = 1024,
		};
		char m_read_buffer[MAXMSG];
		char m_write_buffer[MAXMSG];
		bool m_started;
	private:
		TalkToClient();
	public:
		typedef error_code ErrorCode;
		typedef shared_ptr<TalkToClient> TalkToClientPtr;
		void Start();
		static TalkToClientPtr New();
		void Stop();
		ip::tcp::socket& Sock();
	private:
		void doRead();
		void doWrite(const std::string &msg);
		size_t ReadComplete(const error_code &ec, size_t bytes);
		void OnRead(const error_code &ec, size_t bytes);
		void OnWrite(const error_code &ec, size_t bytes);
	};

	TalkToClient::TalkToClient()
		: m_socket(g_service), m_started(false){}

	void TalkToClient::Start()
	{
		m_started = true;
		doRead();
	}

	TalkToClient::TalkToClientPtr TalkToClient::New()
	{
		TalkToClientPtr ptr(new TalkToClient());
		return ptr;
	}

	void TalkToClient::Stop()
	{
		if (!m_started)
			return;
		m_started = false;
		m_socket.close();
	}

	ip::tcp::socket& TalkToClient::Sock()
	{
		return m_socket;
	}

	void TalkToClient::doRead()
	{
		async_read(m_socket, buffer(m_read_buffer), MEM_FUN2(ReadComplete, _1, _2), MEM_FUN2(OnRead, _1, _2));
	}

	void TalkToClient::doWrite(const std::string &msg)
	{
		if (!m_started)
			return;
		std::copy(msg.begin(), msg.end(), m_write_buffer);
		m_socket.async_write_some(buffer(m_write_buffer, msg.size()), MEM_FUN2(OnWrite, _1, _2));
	}

	size_t TalkToClient::ReadComplete(const error_code &ec, size_t bytes)
	{
		if (ec)
			return 0;
		bool found = (std::find(m_read_buffer, m_read_buffer + bytes, '\n') < m_read_buffer + bytes);
		return found ? 0 : 1;
	}

	void TalkToClient::OnRead(const error_code &ec, size_t bytes)
	{
		if (!ec)
		{
			std::string cpy(m_read_buffer, bytes);
			doWrite(cpy + "\n");
		}
		Stop();
	}

	void TalkToClient::OnWrite(const error_code &ec, size_t bytes)
	{
		doRead();
	}

	ip::tcp::endpoint g_ep(ip::tcp::v4(), 8001);
	ip::tcp::acceptor g_accp(g_service, g_ep);

	void HandleAccept(TalkToClient::TalkToClientPtr client, const error_code &ec)
	{
		client->Start();
		TalkToClient::TalkToClientPtr new_client = TalkToClient::New();
		g_accp.async_accept(new_client->Sock(), bind(HandleAccept, new_client, _1));
	}
}

void TCPASynServer()
{
	try
	{
		TalkToClient::TalkToClientPtr client = TalkToClient::New();
		g_accp.async_accept(client->Sock(), bind(HandleAccept, client, _1));
		g_service.run();
	}
	catch (system::system_error &e)
	{
		std::cout << e.code() << std::endl
			<< e.what() << std::endl;
	}
}