#include "pch.h"
#include "CloudManager.h"

CloudManager::CloudManager()
{
	connectingThread = NULL;
}

CloudManager::~CloudManager()
{
	for (auto cloudThread : cloudThreads) {
		cloudThread->stopThread();
		delete cloudThread;
	}
}

CloudThread* CloudManager::addThread(int port, char* server, char* code, CONNECTIONTYPE ctType)
{
    std::lock_guard<std::mutex> guard(mtx);
	CloudThread* cloudThread = new CloudThread(this);
	cloudThreads.push_back(cloudThread);
	cloudThread->startThread(port, vncPort, server, code, ctType);
	connectingThread = cloudThread;
	return cloudThread;
}

void CloudManager::deleteThread(CloudThread* cloudThread)
{
    std::lock_guard<std::mutex> guard(mtx);
	cloudThreads.erase(std::remove(cloudThreads.begin(), cloudThreads.end(), cloudThread), cloudThreads.end());
	cloudThread->stopThread();
	delete cloudThread;
}

void CloudManager::setVNcPort(int port)
{
	vncPort = port;
}

bool CloudManager::isUdpConnecting()
{
	return connectingThread != NULL ? connectingThread->isThreadRunning() : false;
}
char* CloudManager::getExternalIpAddress()
{
	return connectingThread != NULL ? connectingThread->getExternalIpAddress() : (char*)"";
}
int CloudManager::getStatus()
{
	return connectingThread != NULL ? connectingThread->getStatus() : 0;
}

void CloudManager::stopActiveThread()
{
	if (connectingThread != NULL)
		deleteThread(connectingThread);
	connectingThread = NULL;
}

void CloudManager::isConnected()
{
	connectingThread = NULL;
}


