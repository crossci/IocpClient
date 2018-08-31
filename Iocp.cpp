#include "Iocp.h"
#include "IOContext.h"
#pragma comment(lib,"ws2_32.lib")

bool Iocp::QueryInterface(const GUID& guid, void **ppvObject)
{
	QUERYINTERFACE(Iocp);
	IF_TRUE(QUERYINTERFACE_PARENT(CUnknownEx));
	return false;
}
Iocp::Iocp() :m_completion_port(INVALID_HANDLE_VALUE)
{
	WORD wVersionRequested;
	WSADATA wsaData;
	wVersionRequested = MAKEWORD(2, 2);
	WSAStartup(wVersionRequested, &wsaData);
}
Iocp::~Iocp()
{
	destory();
	WSACleanup();
}
bool Iocp::init(int worker_num)
{
	destory();
	m_completion_port = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (m_completion_port == NULL)
	{
		return false;
	}
	HANDLE thread_handle = INVALID_HANDLE_VALUE;
	for (int i = 0; i < worker_num; i++)
	{
		thread_handle = CreateThread(0, 0, WorkerThreadProc, (void *)this, 0, 0);
	}
}
void Iocp::destory()
{
	if (m_completion_port != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_completion_port);
	}
}
void Iocp::on_error(CPtrHelper<ClientContext> cc)
{
	cc->close();

}
bool Iocp::on_recive(CPtrHelper<ClientContext> cc, IOContext* ioc, int len)
{
	cc->on_recieve(ioc->m_wsa_buf.buf, len);
	if (!post_recieve(cc, *ioc))
	{
		cc->close();
		return false;
	}
	return true;
}
bool Iocp::post_recieve(CPtrHelper<ClientContext> cc, IOContext& ioc)
{
	DWORD dwFlags = 0, dwBytes = 0;
	ioc.reset();
	ioc.m_io_type = RECV_POSTED;
	ioc.m_socket = cc->m_socket;
	int nBytesRecv = WSARecv(ioc.m_socket, &ioc.m_wsa_buf, 1, &dwBytes, &dwFlags, &ioc.m_overLapped, NULL);
	// 如果返回值错误，并且错误的代码并非是Pending的话，那就说明这个重叠请求失败了
	if ((SOCKET_ERROR == nBytesRecv) && (WSA_IO_PENDING != WSAGetLastError()))
	{
		cc->close();
		return false;
	}
	return true;
}
bool Iocp::bind_iocp(CPtrHelper<ClientContext> cc)
{
	if (NULL == CreateIoCompletionPort((HANDLE)cc->m_socket, m_completion_port, (DWORD)(cc.operator->()), 0))
	{
		DWORD dwErr = WSAGetLastError();
		if (dwErr != ERROR_INVALID_PARAMETER)
		{
			cc->close();
			return false;
		}
	}
}
DWORD Iocp::WorkerThreadProc(LPVOID lpParam)
{
	CPtrHelper<Iocp> piocp = Iocp::CreateInstance();
	if (!piocp)
		return 0;
	OVERLAPPED *ol = NULL;
	DWORD dwBytes = 0;
	IOContext* ioContext = NULL;
	ClientContext* client_context = NULL;
	while (1)
	{
		BOOL bRet = GetQueuedCompletionStatus(piocp->m_completion_port, &dwBytes, (PULONG_PTR)&client_context, &ol, INFINITE);

		// 读取传入的参数
		ioContext = CONTAINING_RECORD(ol, IOContext, m_overLapped);

		// 收到退出标志
		if (-1 == (DWORD)client_context)
		{
			break;
		}

		if (!bRet)
		{
			DWORD dwErr = GetLastError();

			// 可能是客户端异常退出了(64)
			if (ERROR_NETNAME_DELETED == dwErr)
			{
				piocp->on_error(client_context);
				continue;
			}
			else
			{
				piocp->on_error(client_context);
				continue;
			}
		}
		else
		{
			// 判断是否有客户端断开
			if ((0 == dwBytes) && (RECV_POSTED == ioContext->m_io_type || SEND_POSTED == ioContext->m_io_type))
			{
				piocp->on_error(client_context);
				continue;
			}
			else
			{
				switch (ioContext->m_io_type)
				{
				case CONNECT:
					client_context->on_connect();
					break;
				case RECV_POSTED:
					piocp->on_recive(client_context, ioContext, dwBytes);
					break;
				case SEND_POSTED:
					client_context->on_send_complete();
					break;
				default:
					break;
				}
			}
		}
	}
	return 0;
}