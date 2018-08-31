#include "ClientManager.h"

bool ClientManager::QueryInterface(const GUID& guid, void **ppvObject)
{
	QUERYINTERFACE(ClientManager);
	IF_TRUE(QUERYINTERFACE_PARENT(CUnknownEx));
	return false;
}
void ClientManager::init()
{
}
void ClientManager::clear()
{
	m_client_lock.Lock();
	m_client_map.clear();
	m_client_lock.Unlock();
}
void ClientManager::add_client(CPtrHelper<ClientContext> client)
{
	int id = client->get_ID();
	m_client_lock.Lock();
	CLIENT_MAP::iterator it = m_client_map.find(id);
	if (it != m_client_map.end())
	{
		m_client_map.erase(it);
	}
	m_client_map.insert(std::make_pair(id,client));
	m_client_lock.Unlock();
}
void ClientManager::remove_client(int id)
{
	m_client_lock.Lock();
	CLIENT_MAP::iterator it = m_client_map.find(id);
	if (it != m_client_map.end())
	{
		m_client_map.erase(it);
	}
	m_client_lock.Unlock();
}