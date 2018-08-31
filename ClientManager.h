#pragma once
#include "ClientContext.h"
#include <map>
static const GUID GUID_OF(ClientManager) =
{ 0x53be7794, 0x2dfd, 0x436e, { 0xa3, 0xa2, 0xed, 0x44, 0x31, 0x6b, 0x18, 0xea } };
class ClientManager :public CUnknownEx
{
public:
	typedef std::map<int, CPtrHelper<ClientContext>> CLIENT_MAP;
	__QueryInterface;
	STATIC_CREATE(ClientManager);
	CLIENT_MAP m_client_map;
	CLock m_client_lock;
	void init();
	void clear();
	

	void add_client(CPtrHelper<ClientContext> client);

	void remove_client(int id);

};