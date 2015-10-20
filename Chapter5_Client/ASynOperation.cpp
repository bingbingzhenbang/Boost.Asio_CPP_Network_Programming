#include <iostream>
#include <vector>
#include <string>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <windows.h>

using namespace boost;
using namespace boost::asio;
using namespace boost::system;

namespace{
	io_service g_service;
	struct ASyncOp : enable_shared_from_this<ASyncOp>,
		private asio::detail::noncopyable
	{
	public:
		typedef function<error_code()> OpFunc;
		typedef function<void(error_code)> CompletionFunc;
		typedef shared_ptr<ASyncOp> ASyncOpPtr;
		typedef shared_ptr<io_service::work> WorkPtr;
		struct Operation
		{
			io_service *m_service;
			OpFunc m_op;
			CompletionFunc m_completion;
			WorkPtr m_work;
			Operation() : m_service(0){}
			Operation(io_service &service, OpFunc op, CompletionFunc completion)
				: m_service(&service), m_op(op), m_completion(completion), m_work(new io_service::work(service)){}
		};
	private:
		recursive_mutex m_cs;
		std::vector<Operation> m_ops;
		bool m_started;
		ASyncOpPtr m_self;
		ASyncOp() : m_started(false){}
	public:
		static ASyncOpPtr New()
		{
			return ASyncOpPtr(new ASyncOp);
		}
		void Start()
		{
			{
				recursive_mutex::scoped_lock lk(m_cs);
				if (m_started)
					return;
				m_started = true;
			}
			thread t(bind(&ASyncOp::Run, this));
		}
		void Add(OpFunc op, CompletionFunc completion, io_service &service)
		{
			m_self = shared_from_this();
			recursive_mutex::scoped_lock lk(m_cs);
			m_ops.push_back(Operation(service, op, completion));
			if (!m_started)
				Start();
		}
		void Run()
		{
			while (true)
			{
				{
					recursive_mutex::scoped_lock lk(m_cs);
					if (!m_started)
						break;
				}
				this_thread::sleep(posix_time::microsec(10));
				Operation cur;
				{
					recursive_mutex::scoped_lock lk(m_cs);
					if (!m_ops.empty())
					{
						cur = m_ops[0];
						m_ops.erase(m_ops.begin());
					}
				}
				if (cur.m_service)
					cur.m_service->post(bind(cur.m_completion, cur.m_op()));
			}
			m_self.reset();
		}
	};

	size_t g_checksum = 0;
	std::wstring g_filename = L"readme.txt";
	error_code ComputeFileChecksum(std::wstring file_name)
	{
		HANDLE file = ::CreateFile(file_name.c_str(), GENERIC_READ, 0, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, 0);
		windows::random_access_handle h(g_service, file);
		long buff[1024];
		size_t bytes = 0, at = 0;
		error_code ec;
		while ( (bytes = read_at(h, at, buffer(buff), ec)) > 0 )
		{
			at += bytes;
			bytes /= sizeof(long);
			for (size_t i = 0; i < bytes; ++i)
				g_checksum += buff[i];
		}
		return error_code(0, generic_category());
	}
	void OnChecksum(std::wstring file_name, error_code)
	{
		std::wcout << "checksum for " << file_name << " = " << g_checksum << std::endl;
	}
}

void ASynOperation()
{
	ASyncOp::New()->Add(bind(ComputeFileChecksum, g_filename), bind(OnChecksum, g_filename, _1), g_service);
	g_service.run();
}