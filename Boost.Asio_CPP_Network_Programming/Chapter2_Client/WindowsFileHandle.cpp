#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/ref.hpp>
#include <windows.h>

using namespace boost;
using namespace boost::asio;
using namespace boost::system;

namespace{
	io_service g_service;
	void OnRead(streambuf &buf, const error_code &ec, size_t sz)
	{
		std::istream in(&buf);
		std::string line;
		std::getline(in, line);
		std::cout << "first line: " << line << std::endl;
	}
}

void WindowsFileHandle()
{
	HANDLE file = ::CreateFile(L"Readme.txt", GENERIC_READ, 0, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, 0);
	windows::stream_handle h(g_service, file);
	streambuf buf;
	async_read(h, buf, transfer_exactly(256), bind(OnRead, boost::ref(buf), _1, _2));
	g_service.run();
}