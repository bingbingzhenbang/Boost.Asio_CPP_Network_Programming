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

namespace{
	io_service g_service;
	recursive_mutex g_cs;
	struct TalkToClient;
	typedef shared_ptr<TalkToClient> TalkToClientPtr;
	typedef std::vector<TalkToClientPtr> ClientsArray;
	ClientsArray g_clients;

	struct TalkToClient
		: enable_shared_from_this<TalkToClient>
	{
	private:
		ip::tcp::socket m_socket;
		enum{
			MAXMSG = 1024,
		};
		int m_already_read;
		char m_buffer[MAXMSG];
		bool m_started;
		std::string m_username;
		bool m_clients_changed;
		posix_time::ptime m_last_ping;
	public:
		TalkToClient();
		std::string Username() const;
		void AnswerToClient();
		void SetClientsChanged();
		ip::tcp::socket& Sock();
		bool TimedOut() const;
		void Stop();
	private:
		void ReadRequest();
		void ProcessRequest();
		void UpdateClientsChanged();
		void OnLogin(const std::string &msg);
		void OnPing();
		void OnClients();
		void Write(const std::string &msg);
	};

	TalkToClient::TalkToClient()
		: m_socket(g_service), m_already_read(0), m_started(false), m_clients_changed(false){}

	std::string TalkToClient::Username() const
	{
		return m_username;
	}

	void TalkToClient::AnswerToClient()
	{
		try
		{
			m_already_read = 0;
			ReadRequest();
			ProcessRequest();
		}
		catch (system::system_error &e)
		{
			Stop();
		}
		if (TimedOut())
			Stop();
	}

	void TalkToClient::SetClientsChanged()
	{
		m_clients_changed = true;
	}

	ip::tcp::socket& TalkToClient::Sock()
	{
		return m_socket;
	}

	bool TalkToClient::TimedOut() const
	{
		posix_time::ptime now = posix_time::microsec_clock::local_time();
		long long ms = (now - m_last_ping).total_microseconds();
		return ms > 5000;
	}

	void TalkToClient::Stop()
	{
		error_code ec;
		m_socket.close(ec);
	}

	void TalkToClient::ReadRequest()
	{
		if (m_socket.available())
			m_already_read += m_socket.read_some(buffer(m_buffer + m_already_read, MAXMSG - m_already_read));
	}
	
	void TalkToClient::ProcessRequest()
	{
		bool found_enter = std::find(m_buffer, m_buffer + m_already_read, '\n') < m_buffer + m_already_read;
		if (!found_enter)
			return;
		m_last_ping = posix_time::microsec_clock::local_time();
		size_t pos = std::find(m_buffer, m_buffer + m_already_read, '\n') - m_buffer;
		std::string msg(m_buffer, pos);
		std::copy(m_buffer + m_already_read, m_buffer + MAXMSG, m_buffer);
		m_already_read -= pos + 1;
		if (msg.find("login ") == 0)
			OnLogin(msg);
		else if (msg.find("ping") == 0)
			OnPing();
		else if (msg.find("ask_clients") == 0)
			OnClients();
		else
			std::cerr << "invalid msg " << msg << std::endl;
	}

	void TalkToClient::UpdateClientsChanged()
	{

	}

	void TalkToClient::OnLogin(const std::string &msg)
	{
		std::istringstream in(msg);
		in >> m_username >> m_username;
		Write("login ok\n");
		UpdateClientsChanged();
	}

	void TalkToClient::OnPing()
	{
		Write(m_clients_changed ? "ping client_list_changed\n" : "ping ok\n");
		m_clients_changed = false;
	}

	void TalkToClient::OnClients()
	{
		std::string msg;
		{
			recursive_mutex::scoped_lock lk(g_cs);
			for (ClientsArray::iterator b = g_clients.begin();
				b != g_clients.end(); ++b)
			{
				msg += b->get()->Username() + " ";
			}
		}
		Write("clients " + msg + "\n");
	}

	void TalkToClient::Write(const std::string &msg)
	{
		m_socket.write_some(buffer(msg));
	}

	void AcceptThread()
	{
		ip::tcp::acceptor accp(g_service, ip::tcp::endpoint(ip::tcp::v4(), 8001));
		while (true)
		{
			TalkToClientPtr ptr(new TalkToClient);
			accp.accept(ptr->Sock());
			recursive_mutex::scoped_lock lk(g_cs);
			g_clients.push_back(ptr);
		}
	}

	void HandleClientsThread()
	{
		while (true)
		{
			this_thread::sleep(posix_time::millisec(1));
			recursive_mutex::scoped_lock lk(g_cs);
			for (ClientsArray::iterator b = g_clients.begin();
				b != g_clients.end(); ++b)
			{
				b->get()->AnswerToClient();
			}
			g_clients.erase(std::remove_if(g_clients.begin(), g_clients.end(), bind(&TalkToClient::TimedOut, _1)), g_clients.end());
		}
	}
}

void SynServer()
{
	try
	{
		thread_group threads;
		threads.create_thread(AcceptThread);
		threads.create_thread(HandleClientsThread);
		threads.join_all();
	}
	catch (system::system_error &e)
	{
		std::cout << "Client terminated. "
			<< e.code() << " " << e.what() << std::endl;
	}
}