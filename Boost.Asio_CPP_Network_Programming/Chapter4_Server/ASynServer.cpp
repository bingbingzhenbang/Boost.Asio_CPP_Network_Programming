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
	class TalkToClient;
	typedef shared_ptr<TalkToClient> TalkToClientPtr;
	typedef std::vector<TalkToClientPtr> ClientsArray;
	ClientsArray g_clients;

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
		std::string m_username;
		deadline_timer m_timer;
		bool m_clients_changed;
		posix_time::ptime m_last_ping;
	private:
		TalkToClient();
	public:
		typedef error_code ErrorCode;
		void Start();
		static TalkToClientPtr New();
		void Stop();
		bool Started() const;
		ip::tcp::socket& Sock();
		std::string Username() const;
		void SetClientsChanged();
	private:
		void UpdateClientsChanged();
		void OnRead(const error_code &ec, size_t bytes);
		void OnLogin(const std::string &msg);
		void OnPing();
		void OnClients();
		void OnWrite(const error_code &ec, size_t bytes);
		void OnCheckPing();
		void doRead();
		void doPing();
		void doAskClients();
		void doWrite(const std::string &msg);
		void PostCheckPing();
		size_t ReadComplete(const error_code &ec, size_t bytes);	
	};

	TalkToClient::TalkToClient()
		: m_socket(g_service), m_started(false), m_timer(g_service), m_clients_changed(false){}

	void TalkToClient::Start()
	{
		m_started = true;
		g_clients.push_back(shared_from_this());
		m_last_ping = posix_time::microsec_clock::local_time();
		doRead();
	}

	TalkToClientPtr TalkToClient::New()
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
		TalkToClientPtr this_talk = shared_from_this();
		ClientsArray::iterator itr = std::find(g_clients.begin(), g_clients.end(), this_talk);
		g_clients.erase(itr);
		UpdateClientsChanged();
	}

	bool TalkToClient::Started() const
	{
		return m_started;
	}

	ip::tcp::socket& TalkToClient::Sock()
	{
		return m_socket;
	}

	std::string TalkToClient::Username() const
	{
		return m_username;
	}

	void TalkToClient::SetClientsChanged()
	{
		m_clients_changed = true;
	}

	void TalkToClient::UpdateClientsChanged()
	{
		for (ClientsArray::iterator itr = g_clients.begin();
			itr != g_clients.end(); ++itr)
			itr->get()->SetClientsChanged();
	}

	void TalkToClient::OnRead(const error_code &ec, size_t bytes)
	{
		if (ec)
			Stop();
		if (!Started())
			return;
		std::string msg(m_read_buffer, bytes);
		if (msg.find("login ") == 0)
			OnLogin(msg);
		else if (msg.find("ping") == 0)
			OnPing();
		else if (msg.find("ask_clients") == 0)
			OnClients();
	}

	void TalkToClient::OnLogin(const std::string &msg)
	{
		std::istringstream in(msg);
		in >> m_username >> m_username;
		doWrite("login ok\n");
		UpdateClientsChanged();
	}

	void TalkToClient::OnPing()
	{
		doWrite(m_clients_changed ? "ping client_list_changed\n" : "ping ok\n");
		m_clients_changed = false;
	}

	void TalkToClient::OnClients()
	{
		std::string msg;
		for (ClientsArray::iterator itr = g_clients.begin();
			itr != g_clients.end(); ++itr)
		{
			msg += itr->get()->Username() + " ";
		}
		doWrite("clients " + msg + "\n");
	}

	void TalkToClient::OnWrite(const error_code &ec, size_t bytes)
	{
		doRead();
	}

	void TalkToClient::OnCheckPing()
	{
		posix_time::ptime now = posix_time::microsec_clock::local_time();
		if ((now - m_last_ping).total_milliseconds() > 5000)
			Stop();
		m_last_ping = posix_time::microsec_clock::local_time();
	}

	void TalkToClient::doRead()
	{
		async_read(m_socket, buffer(m_read_buffer), MEM_FUN2(ReadComplete, _1, _2), MEM_FUN2(OnRead, _1, _2));
		PostCheckPing();
	}

	void TalkToClient::doPing()
	{
		doWrite("ping\n");
	}

	void TalkToClient::doAskClients()
	{
		doWrite("ask_clients\n");
	}

	void TalkToClient::doWrite(const std::string &msg)
	{
		if (!Started())
			return;
		std::copy(msg.begin(), msg.end(), m_write_buffer);
		m_socket.async_write_some(buffer(m_write_buffer, msg.size()), MEM_FUN2(OnWrite, _1, _2));
	}

	void TalkToClient::PostCheckPing()
	{
		m_timer.expires_from_now(posix_time::microsec(5000));
		m_timer.async_wait(MEM_FN(OnCheckPing));
	}

	size_t TalkToClient::ReadComplete(const error_code &ec, size_t bytes)
	{
		if (ec)
			return 0;
		bool found = (std::find(m_read_buffer, m_read_buffer + bytes, '\n') < m_read_buffer + bytes);
		return found ? 0 : 1;
	}

	ip::tcp::endpoint g_ep(ip::tcp::v4(), 8001);
	ip::tcp::acceptor g_accp(g_service, g_ep);

	void HandleAccept(TalkToClientPtr client, const error_code &ec)
	{
		client->Start();
		TalkToClientPtr new_client = TalkToClient::New();
		g_accp.async_accept(new_client->Sock(), boost::bind(HandleAccept, new_client, _1));
	}
}

void ASynServer()
{
	try
	{
		TalkToClientPtr client = TalkToClient::New();
		g_accp.async_accept(client->Sock(), boost::bind(HandleAccept, client, _1));
		g_service.run();
	}
	catch (system::system_error &e)
	{
		std::cout << "Client terminated. "
			<< e.code() << " " << e.what() << std::endl;
	}
}