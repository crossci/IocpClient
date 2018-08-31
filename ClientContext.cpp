#include "ClientContext.h"
#include "ClientManager.h"
#include "Iocp.h"
#include <iostream>
using namespace std;
bool ClientContext::QueryInterface(const GUID& guid, void **ppvObject)
{
	QUERYINTERFACE(ClientContext);
	IF_TRUE(QUERYINTERFACE_PARENT(CTimer));
	return false;
}
ClientContext::ClientContext() :m_socket(INVALID_SOCKET)
, m_ID(0)
{
	set_send_index(0);
	set_send_complete(true);
	m_sending_pool[0].reset();
	m_sending_pool[1].reset();
}
ClientContext::~ClientContext()
{
	
}
void ClientContext::on_recieve(const char* buffer, int len)
{
	std::string temp(buffer,len);
	cout << temp << endl;
	post_recieve();
}
bool ClientContext::post_recieve()
{
	DWORD dwFlags = 0, dwBytes = 0;
	m_read_context.reset();
	m_read_context.m_socket = m_socket;
	m_read_context.m_io_type = RECV_POSTED;
	int nBytesRecv = WSARecv(m_read_context.m_socket, &m_read_context.m_wsa_buf, 1, &dwBytes, &dwFlags, &m_read_context.m_overLapped, NULL);
	// 如果返回值错误，并且错误的代码并非是Pending的话，那就说明这个重叠请求失败了
	if ((SOCKET_ERROR == nBytesRecv) && (WSA_IO_PENDING != WSAGetLastError()))
	{
		close();
		return false;
	}
	return true;
}
void ClientContext::write(const char* buffer, int len)
{
	m_sending_pool_lock.Lock();
	int write_index = (m_send_index + 1) % 2;
	m_sending_pool[write_index].write(buffer, len);
	if (get_send_complete())
	{
		_post_send();
	}
	m_sending_pool_lock.Unlock();
	
}
void ClientContext::on_send_complete()
{
	m_sending_pool_lock.Lock();
	set_send_complete(true);
	_post_send();
	m_sending_pool_lock.Unlock();
}
void ClientContext::close()
{
	ClearTimer();
	if (m_socket != INVALID_SOCKET)
	{
		shutdown(m_socket, SD_BOTH);
		closesocket(m_socket);
		m_socket = INVALID_SOCKET;
		set_net_state(NET_STATE_CLOSED);
	}
	CPtrHelper<ClientManager> pClientManager = ClientManager::CreateInstance();
	if (pClientManager)
	{
		pClientManager->remove_client(get_ID());
	}
}
bool ClientContext::connect(const char* ip, int port)
{
	close();
	m_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (m_socket == INVALID_SOCKET)
	{
		return false;
	}
	set_ID(m_socket);
	CPtrHelper<Iocp> pIocp = Iocp::CreateInstance();
	if (pIocp)
	{
		pIocp->bind_iocp(this);
	}
	m_read_context.reset();
	m_read_context.m_socket = m_socket;
	m_read_context.m_io_type = CONNECT;

	sockaddr_in addr; //地址结构
	memset((void *)&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(ip);
	addr.sin_port = htons((u_short)port);

	int dwErr = 0;
	static LPFN_CONNECTEX lpfnconnectex = 0;
	if (!lpfnconnectex)
	{
		unsigned int dwBytes = 0;
		GUID g_GuidConnectEx = WSAID_CONNECTEX;
		dwErr = WSAIoctl(m_socket, SIO_GET_EXTENSION_FUNCTION_POINTER, &g_GuidConnectEx, sizeof(g_GuidConnectEx), &lpfnconnectex, sizeof(lpfnconnectex), (LPDWORD)&dwBytes, NULL, NULL);
		if (dwErr == SOCKET_ERROR)
		{
			return false;
		}
	}
	if (!_bind())
	{
		return false;
	}
	int result = lpfnconnectex(m_socket, (struct sockaddr *)&addr, sizeof(addr), NULL, NULL, NULL, (LPWSAOVERLAPPED)&(m_read_context.m_overLapped));
	if (result == 0)
	{
		dwErr = WSAGetLastError();
		if (dwErr == ERROR_IO_PENDING)
		{
			set_net_state(NET_STATE_CONNECTING);
			return true;
		}
		return false;
	}
	set_net_state(NET_STATE_CONNECTING);
	return true;
}
void ClientContext::on_connect()
{
	set_net_state(NET_STATE_CONNECTED);
	CPtrHelper<ClientManager> pClientManager = ClientManager::CreateInstance();
	if (pClientManager)
	{
		pClientManager->add_client(this);
	}
	post_recieve();
	SetTimer(SEND_MASSAGE_EVENT, 10);
}
bool ClientContext::OnTimer(int e)
{
	bool ret = true;
	switch (e)
	{
	case SEND_MASSAGE_EVENT:
		write("hello", strlen("hello"));
		break;
	default:
		ret = false;
		break;
	}
	return ret;
}
void ClientContext::_post_send()
{
	m_send_index = (m_send_index + 1) % 2;
	int count = m_sending_pool[m_send_index].get_count();
	if (count > 0)
	{
		set_send_complete(false);
		m_sending_pool[m_send_index].m_io_type = SEND_POSTED;
		m_sending_pool[m_send_index].m_socket = m_socket;
		DWORD dwBytes = 0, dwFlags = 0;
		m_sending_pool[m_send_index].m_wsa_buf.buf = m_sending_pool[m_send_index].m_buffer.GetBuffer();
		m_sending_pool[m_send_index].m_wsa_buf.len = count;
		if (::WSASend(m_sending_pool[m_send_index].m_socket, &m_sending_pool[m_send_index].m_wsa_buf, 1, &dwBytes, dwFlags, &m_sending_pool[m_send_index].m_overLapped, NULL) != NO_ERROR)
		{
			if (WSAGetLastError() != WSA_IO_PENDING)
			{
				//todo close the socket;
			}
		}
		m_sending_pool[m_send_index].m_buffer.SetEmpty();
	}
}
bool ClientContext::_bind()
{
	bool ret = true;
	int rc = 0;
	struct sockaddr_in addr;
	ZeroMemory(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = 0;
	rc = bind(m_socket, (SOCKADDR*)&addr, sizeof(addr));
	if (rc != 0)
	{
		rc = WSAGetLastError();
		ret = false;
	}
	return ret;
}