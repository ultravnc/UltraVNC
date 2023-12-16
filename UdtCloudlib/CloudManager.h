#pragma once
#include <vector>
#include <condition_variable>
#include "proxy/Cloudthread.h"

class CloudManager
{
private:
	std::mutex mtx;
	std::vector<CloudThread*> cloudThreads;
	int vncPort = 5900;
	CloudThread *connectingThread;
public:
	CloudManager();
	~CloudManager();
	CloudThread* addThread(int port, char* server, char* code, CONNECTIONTYPE ctType);
	void deleteThread(CloudThread* cloudThread);
	void setVNcPort(int port);
	bool isUdpConnecting();
	char* getExternalIpAddress();
	int getStatus();
	void stopActiveThread();
	void isConnected();
};

