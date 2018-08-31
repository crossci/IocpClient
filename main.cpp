#include <iostream>
#include "ClientManager.h"
#include "../PublicLibrary/ConsoleCtrlHandler.h"
#include "Iocp.h"
#include "ClientContext.h"
using namespace std;

int main(int argc,char* argv)
{
	CPtrHelper<Iocp> pIocp = Iocp::CreateInstance();
	if (pIocp)
	{
		pIocp->init(1);
	}

	CPtrHelper<ClientManager> pClientManager = ClientManager::CreateInstance();
	if (pClientManager)
	{
		pClientManager->init();
		CPtrHelper<ClientContext> pclient = NULL;
		for (int i = 0; i < 10000; i++)
		{
			pclient = ClientContext::CreateInstance();
			if (pclient->connect("127.0.0.1", 30000))
			{
				pClientManager->add_client(pclient);
			}
		}
		ConsoleCtrlHandler::WaitConsoleCtrlHandler();
		pClientManager->clear();
	}
	return 0;
}