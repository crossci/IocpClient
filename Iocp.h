#pragma once
#include <WinSock2.h>
#include <Windows.h>
#include <MSWSock.h>
#include "../PublicLibrary/UnknownEx.h"
#include "..\PublicLibrary\Macro.h"
#include "ClientContext.h"
static const GUID GUID_OF(Iocp) =
{ 0x54235d42, 0x24e4, 0x4e7f, { 0x89, 0x95, 0x73, 0xae, 0xe9, 0x3c, 0x5f, 0x5a } };
class Iocp : public CUnknownEx
{
public:
	__QueryInterface;
	STATIC_CREATE(Iocp);
	HANDLE m_completion_port;
public:
	Iocp();
	~Iocp();
	bool init(int worker_num = 1);
	void destory();
	void on_error(CPtrHelper<ClientContext> cc);
	bool on_recive(CPtrHelper<ClientContext> cc, IOContext* ioc, int len);
	bool post_recieve(CPtrHelper<ClientContext> cc, IOContext& ioc);
	bool bind_iocp(CPtrHelper<ClientContext> cc);
	static DWORD WINAPI WorkerThreadProc(LPVOID lpParam);
};