#pragma once
#include <list>

#include <winsock2.h>
#include <windows.h>
#include "omnithread.h"
#include <Iphlpapi.h>
extern bool			fShutdownOrdered;
#define IDEAL_SEND_BACKLOG_IOCTLS
    #define SIO_IDEAL_SEND_BACKLOG_QUERY    _IOR( 't', 123, ULONG )
    #define SIO_IDEAL_SEND_BACKLOG_CHANGE   _IO( 't', 122 )
bool sendall(SOCKET RemoteSocket,char *buff,unsigned int bufflen,int dummy);
extern "C" {
        typedef ULONG (WINAPI *t_GetPerTcpConnectionEStats)(
                PMIB_TCPROW Row, TCP_ESTATS_TYPE EstatsType,
                PUCHAR Rw, ULONG RwVersion, ULONG RwSize,
                PUCHAR Ros, ULONG RosVersion, ULONG RosSize,
                PUCHAR Rod, ULONG RodVersion, ULONG RodSize);
       typedef ULONG (WINAPI *t_SetPerTcpConnectionEStats)(
                PMIB_TCPROW Row, TCP_ESTATS_TYPE EstatsType,
                PUCHAR Rw, ULONG RwVersion, ULONG RwSize,
				ULONG Offset);
}

class CFlowControlledSend:public omni_thread
{
public:
	typedef DWORD (WINAPI *LPWRITE_FREE)(LPVOID lpCallbackParam);
	CFlowControlledSend(void);
	~CFlowControlledSend(void);
		// Init routine
	BOOL Init(SOCKET socket);
		BOOL Close();
	BOOL		sendall(char *buff,unsigned int bufflen,int dummy);
	DWORD		CanWrite(DWORD dwBytesWriteNeeded,LPWRITE_FREE writeCallback,LPVOID lpParam);
	BOOL		IsActive();
private:
	typedef struct ___OVERLAPPED_SEND_INFO{
		WSAOVERLAPPED	Overlap;
		BYTE			bySendData[1500];
		DWORD			dwLen;
	}OVERLAPPED_SEND_INFO,*LPOVERLAPPED_SEND_INFO;
	typedef std::list<OVERLAPPED_SEND_INFO*> SendPendingList;
	static	const INT   DEFAULT_SNDBUF;
	BOOL		m_bShutDown;
	HANDLE		m_hDataReady;
	omni_mutex  m_SendListLock;	
	SendPendingList	m_Sendlist;
	SOCKET			m_RemoteSock;
	DWORD			m_dwPendingBytes;
	DWORD			m_dwOptimalBDP;
	BOOL			m_bBDPQuerySupported;
	LPWRITE_FREE	m_pCallBack;
	LPVOID			m_pData;
	DWORD			m_dwBytesRequested;
	

	// Code to be executed by the thread
	virtual void* run_undetached(void * arg);
	BOOL									GetOptimalSndBuf(SOCKET socket,DWORD &dwMaxSndBytes);
	static HMODULE							s_hIPHlp;
	static t_GetPerTcpConnectionEStats      s_pGetPerTcpConnectionEStats;
	static t_SetPerTcpConnectionEStats      s_pSetPerTcpConnectionEStats;
	MIB_TCPROW                              m_SocketInfo;
	BOOL									InitTCPStats();	
};
