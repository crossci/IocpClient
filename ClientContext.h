#pragma once
#include <WinSock2.h>
#include <Windows.h>
#include <MSWSock.h>
#include <vector>
#include <list>
#include "../PublicLibrary/MemoryPool.h"
#include "../PublicLibrary/Macro.h"
#include "TimerManage/itimer.h"
#include "IOContext.h"
static const GUID GUID_OF(ClientContext) =
{ 0x578e040a, 0x514, 0x4d97, { 0xab, 0x50, 0xaa, 0x83, 0xa9, 0x95, 0x2e, 0x62 } };
class ClientContext :public CTimer, public CCircularMemory
{
public:
	MEMORY_INTERFACE;
	POOL_CREATE(ClientContext);
	DEFINE_VALUE(ID, int);
	DEFINE_STRING(client_IP);
	DEFINE_VALUE(client_port, int);
	DEFINE_VALUE(net_state, int);

	SOCKET m_socket;
	IOContext m_read_context;
	IOContext m_sending_pool[2];
	DEFINE_VALUE(send_index, int);
	DEFINE_VALUE(send_complete, bool);
	CLock m_sending_pool_lock;
public:
	ClientContext();
	~ClientContext();
	IOContext& get_read_context(){ return m_read_context; }
	void on_recieve(const char* buffer,int len);
	bool post_recieve();
	void write(const char* buffer,int len);
	void on_send_complete();
	void close();
	bool connect(const char* ip, int port);
	void on_connect();
	virtual bool OnTimer(int e);
private:
	void _post_send();
	bool _bind();

};