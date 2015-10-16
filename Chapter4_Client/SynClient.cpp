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
	ip::tcp::endpoint g_ep(ip::address::from_string("127.0.0.1"), 8001);
	struct TalkToSvr
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
	public:
		TalkToSvr(const std::string &username);
		void Connect(ip::tcp::endpoint ep);
		void Loop();
		std::string Username() const;
	private:
		void WriteRequest();
		void ReadAnswer();
		void ProcessMsg();
		void OnLogin();
		void OnPing(const std::string &msg);
		void OnClients(const std::string &msg);
		void doAskClients();
		void Write(const std::string &msg);
		size_t ReadComplete(const error_code &ec, size_t bytes);
	};

	TalkToSvr::TalkToSvr(const std::string &username)
		: m_socket(g_service), m_started(true), m_username(username){}

	void TalkToSvr::Connect(ip::tcp::endpoint ep)
	{
		m_socket.connect(ep);
	}

	void TalkToSvr::Loop()
	{
		Write(std::string("login ") + m_username + "\n");
		ReadAnswer();
		while (m_started)
		{
			WriteRequest();
			ReadAnswer();
			//this_thread::sleep(posix_time::millisec( rand() % 7000 ));
		}
	}

	std::string TalkToSvr::Username() const
	{
		return m_username;
	}

	void TalkToSvr::WriteRequest()
	{
		Write("ping\n");
	}

	void TalkToSvr::ReadAnswer()
	{
		m_already_read = 0;
		read(m_socket, buffer(m_buffer), bind(&TalkToSvr::ReadComplete, this, _1, _2));
		ProcessMsg();
	}

	void TalkToSvr::ProcessMsg()
	{
		std::string msg(m_buffer, m_already_read);
		if (msg.find("login ") == 0)
			OnLogin();
		else if (msg.find("ping") == 0)
			OnPing(msg);
		else if (msg.find("clients ") == 0)
			OnClients(msg);
		else
			std::cerr << "invalid msg " << msg << std::endl;
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
	}

	void TalkToSvr::OnClients(const std::string &msg)
	{
		std::string clients = msg.substr(8);
		std::cout << m_username << ", new client list:" << clients;
	}

	void TalkToSvr::doAskClients()
	{
		Write("ask_clients\n");
		ReadAnswer();
	}

	void TalkToSvr::Write(const std::string &msg)
	{
		m_socket.write_some(buffer(msg));
	}

	size_t TalkToSvr::ReadComplete(const error_code &ec, size_t bytes)
	{
		if (ec)
			return 0;
		bool found = (std::find(m_buffer, m_buffer + bytes, '\n') < m_buffer + bytes);
		return found ? 0 : 1;
	}

	void RunClient(const std::string &client_name)
	{
		TalkToSvr client(client_name);
		try
		{
			client.Connect(g_ep);
			client.Loop();
		}
		catch (system::system_error &e)
		{
			std::cout << "Client terminated. "
				<< e.code() <<" "<< e.what() << std::endl;
		}
	}
}

void SynClient()
{
	std::string user_name;
	std::cout << "input user name : " << std::endl;
	std::cin >> user_name;
	RunClient(user_name);
}