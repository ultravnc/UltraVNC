#include "FlowControlledSend.h"

HMODULE CFlowControlledSend::s_hIPHlp=0;
 t_GetPerTcpConnectionEStats CFlowControlledSend::s_pGetPerTcpConnectionEStats=0;
 t_SetPerTcpConnectionEStats CFlowControlledSend::s_pSetPerTcpConnectionEStats=0;

const INT   CFlowControlledSend::DEFAULT_SNDBUF=32*1024;
CFlowControlledSend::CFlowControlledSend(void)
{
	m_hDataReady=NULL;
	m_RemoteSock=NULL;
}

CFlowControlledSend::~CFlowControlledSend(void)
{
}
// Method implementations
BOOL CFlowControlledSend::Init(SOCKET RemoteSock)
{
	m_bShutDown=FALSE;
	m_RemoteSock=RemoteSock;
	m_hDataReady=CreateEvent(NULL,FALSE,FALSE,NULL); 
	m_dwPendingBytes=0;
	m_dwOptimalBDP=0;
	//get the BDP value
	DWORD dwOutSize=sizeof(DWORD);

   

	m_bBDPQuerySupported =InitTCPStats();
	if(m_bBDPQuerySupported)
	{
        int optVal=0;
		setsockopt(RemoteSock, SOL_SOCKET, SO_SNDBUF, (char *)&optVal, sizeof(int)); 
	}
	m_pCallBack=NULL;
	m_pData=NULL;
	m_dwBytesRequested=0;

	start_undetached();
	return TRUE;
}

//does initialise to get tcp congestion time
BOOL CFlowControlledSend::InitTCPStats()
{
   if (s_hIPHlp == NULL) //load iphlpi
   {
	  s_hIPHlp = ::LoadLibrary("Iphlpapi.dll");
	  if(s_hIPHlp ){
		s_pGetPerTcpConnectionEStats = (t_GetPerTcpConnectionEStats) ::GetProcAddress(s_hIPHlp, "GetPerTcpConnectionEStats");
		s_pSetPerTcpConnectionEStats = (t_SetPerTcpConnectionEStats) ::GetProcAddress(s_hIPHlp, "SetPerTcpConnectionEStats");
	  }
   }
   //not found api to get cw window
   if(s_pGetPerTcpConnectionEStats==0 || s_pSetPerTcpConnectionEStats==0)return FALSE;

   sockaddr_in skaddr={0};
   int	iSockSize=sizeof(sockaddr_in); 

   //initialise socket row value
   getsockname(m_RemoteSock,(sockaddr*)&skaddr,&iSockSize);
   m_SocketInfo.dwState =MIB_TCP_STATE_ESTAB;
   m_SocketInfo.dwLocalAddr=skaddr.sin_addr.S_un.S_addr;
   m_SocketInfo.dwLocalPort=skaddr.sin_port;
   getpeername(m_RemoteSock,(sockaddr*)&skaddr,&iSockSize);
   m_SocketInfo.dwRemoteAddr=skaddr.sin_addr.S_un.S_addr;
   m_SocketInfo.dwRemotePort=skaddr.sin_port;

   PUCHAR rw = NULL;
   TCP_ESTATS_SND_CONG_RW_v0	  enableInfo={0};
   ULONG iRet, lSize = 0;
   //enable tcp stats 
   rw = (PUCHAR) & enableInfo;
   enableInfo.EnableCollection=TRUE;
   lSize = sizeof (TCP_ESTATS_SND_CONG_RW_v0);
 
   iRet = s_pSetPerTcpConnectionEStats((PMIB_TCPROW) &m_SocketInfo, TcpConnectionEstatsSndCong, rw, 0, lSize, 0);
 
   return iRet==ERROR_SUCCESS; 
}

//method to get congestion window
BOOL CFlowControlledSend::GetOptimalSndBuf(SOCKET socket,DWORD &dwMaxSndBytes)
{
   ULONG							iRet, lSize = 0;
   PUCHAR							rw = NULL;
   TCP_ESTATS_SND_CONG_ROD_v0		 cwInfo={0};

   if(s_pSetPerTcpConnectionEStats==0)return FALSE;

   lSize= sizeof (cwInfo);
   rw = (PUCHAR) & cwInfo;
   iRet = s_pGetPerTcpConnectionEStats(&m_SocketInfo,TcpConnectionEstatsSndCong,NULL,0,0,NULL,0,0,rw,0,lSize);

   if(iRet!=ERROR_SUCCESS)
   {
	   dwMaxSndBytes=	DEFAULT_SNDBUF;
	   return TRUE;
   }

   dwMaxSndBytes= cwInfo.CurCwnd*1.4; //always twice the congestion window will be optimal
   //this doesn't work
/*   INT iRet =  WSAIoctl( socket,SIO_IDEAL_SEND_BACKLOG_QUERY,NULL,0,
				(LPVOID)&dwBDP,sizeof(DWORD),
				(LPDWORD) &dwOutSize,NULL,NULL  // completion routine 
				);*/
	 
	return TRUE;
}
//do resource freeing here
BOOL CFlowControlledSend::Close()
{
	m_bShutDown=TRUE;
	SetEvent(m_hDataReady);//notify shutdown
	void *returnval;//wait for thread exit

	if(m_hDataReady)
	{
		CloseHandle(m_hDataReady); 
		m_hDataReady=NULL;
	}
	m_RemoteSock=NULL;
	join(&returnval);//object destroyed
	return TRUE;
}
//new overlapped send keeps only congestion window size pending
BOOL CFlowControlledSend::sendall(char *buff,unsigned int bufflen,int dummy)
{
	LPOVERLAPPED_SEND_INFO		pInfo=new OVERLAPPED_SEND_INFO;
	WSABUF						DataBuf={0};
    DWORD						dwSentBytes=0;

	if (fShutdownOrdered) return FALSE;

	
	if(!m_bBDPQuerySupported || //no support for BDp query do default
		m_dwPendingBytes>m_dwOptimalBDP)//make blocking send call if greater  than BDP
	{
		return ::sendall(m_RemoteSock,buff,bufflen,dummy);
	}
	//queue the send to the writing thread 
	DataBuf.buf = (CHAR*)pInfo->bySendData;
	DataBuf.len=pInfo->dwLen= bufflen; 
	memcpy(DataBuf.buf,buff,bufflen); 
	pInfo->Overlap.hEvent = WSACreateEvent();
	INT iRet = WSASend(m_RemoteSock,&DataBuf,1,&dwSentBytes,0,&pInfo->Overlap,NULL);
	INT iLasterr = WSAGetLastError();
	if ((iRet == SOCKET_ERROR) &&
         (WSA_IO_PENDING != iLasterr)
			)
	{
		return FALSE;
	}
	if(iRet == ERROR_SUCCESS && dwSentBytes==bufflen )
	{
		//sent completed immmediately
		WSACloseEvent( pInfo->Overlap.hEvent );
		delete pInfo;
		return TRUE;
	}

	{//locked
		omni_mutex_lock				l(m_SendListLock);
		//add to queue pending events
		m_dwPendingBytes+=DataBuf.len; 
		m_Sendlist.push_front(pInfo); 
		SetEvent(m_hDataReady);
	}
	return TRUE;
}


BOOL CFlowControlledSend::IsActive()
{
	return m_bBDPQuerySupported;
}
//check how many bytes can be written
DWORD	CFlowControlledSend::CanWrite(DWORD dwBytesWriteNeeded,LPWRITE_FREE writeCallback,LPVOID lpParam)
{
	GetOptimalSndBuf(m_RemoteSock,m_dwOptimalBDP);
	DWORD dwBytesCanWrite=m_dwOptimalBDP>m_dwPendingBytes?m_dwOptimalBDP-m_dwPendingBytes:0;
	m_pCallBack=writeCallback;
	m_pData= lpParam;
	m_dwBytesRequested=dwBytesWriteNeeded;
	return dwBytesCanWrite>dwBytesWriteNeeded?dwBytesCanWrite:0;
}
// Code to be executed by the thread
void* CFlowControlledSend::run_undetached(void * arg)
{
	DWORD			dwHandleSize=2;	
	HANDLE			*pWaitHandles= new HANDLE[2];
	OVERLAPPED 	    *pBDPChangeOvlap=new OVERLAPPED; 
	DWORD			dwOutSize=sizeof(DWORD);

	pWaitHandles[0]=m_hDataReady;
	pBDPChangeOvlap->hEvent = WSACreateEvent(); 
	pWaitHandles[1]=pBDPChangeOvlap->hEvent;
	
/*	INT iRet =  WSAIoctl( m_RemoteSock,SIO_IDEAL_SEND_BACKLOG_CHANGE,NULL,0,
				(LPVOID)& m_dwOptimalBDP,sizeof(DWORD),
				(LPDWORD) &dwOutSize,pBDPChangeOvlap,NULL
				);*/
	while(!m_bShutDown)
	{
		DWORD	dwWaitRet;

		dwWaitRet=WSAWaitForMultipleEvents(dwHandleSize,pWaitHandles,FALSE,1*1000,FALSE); 
		if(dwWaitRet == WAIT_OBJECT_0)//queued send
		{
			omni_mutex_lock				l(m_SendListLock);
			SendPendingList::iterator	PendItem;

			delete pWaitHandles;
			pWaitHandles = new HANDLE[m_Sendlist.size()+2];
			pWaitHandles[0] = m_hDataReady;
			pWaitHandles[1] = pBDPChangeOvlap->hEvent;
			INT iIndex=2;
			for ( PendItem = m_Sendlist.begin(); PendItem != m_Sendlist.end(); ++PendItem)
			{
				pWaitHandles[iIndex++]=(*PendItem)->Overlap.hEvent;
			}
			dwHandleSize=m_Sendlist.size()+2;
			continue;
		}
		if(dwWaitRet == WAIT_OBJECT_0+1)//bdp changed
		{
/*			WSAIoctl( m_RemoteSock,SIO_IDEAL_SEND_BACKLOG_CHANGE,NULL,0,
				(LPVOID)& m_dwOptimalBDP,sizeof(DWORD),
				(LPDWORD) &dwOutSize,pBDPChangeOvlap,NULL
				);
			DWORD dwBytesCanWrite=m_dwOptimalBDP>m_dwPendingBytes?m_dwBDP-m_dwPendingBytes:0;
			if(dwBytesCanWrite>m_dwBytesRequested && m_pCallBack)
			{
				m_pCallBack(m_pData);		
			}*/
			continue;
		}
		if(dwWaitRet > WAIT_OBJECT_0+1 && dwWaitRet<(WAIT_OBJECT_0+dwHandleSize))//send completed
		{
			omni_mutex_lock				l(m_SendListLock);
			SendPendingList::iterator	PendItem;

			GetOptimalSndBuf(m_RemoteSock,m_dwOptimalBDP);//recalculate
			//find the handle and remove the item from list
			for ( PendItem = m_Sendlist.begin(); PendItem != m_Sendlist.end(); ++PendItem)
			{
				if((*PendItem)->Overlap.hEvent==pWaitHandles[dwWaitRet])
				{
					m_dwPendingBytes-=(* PendItem)->dwLen;
					m_Sendlist.erase(PendItem);  
					break;
				}
			}
			//form new wait handles
			delete pWaitHandles;
			pWaitHandles = new HANDLE[m_Sendlist.size()+2];
			pWaitHandles[0] = m_hDataReady;
			pWaitHandles[1] = pBDPChangeOvlap->hEvent;
			INT iIndex=2;
			for ( PendItem = m_Sendlist.begin(); PendItem != m_Sendlist.end(); ++PendItem)
			{
				pWaitHandles[iIndex++]=(*PendItem)->Overlap.hEvent;
			}
			dwHandleSize=m_Sendlist.size()+2;
			continue;
		}
		if(dwWaitRet==WAIT_TIMEOUT)
		{
			
			if (fShutdownOrdered) break;
		}
	}//while

	WSACloseEvent(pBDPChangeOvlap->hEvent );
	delete pBDPChangeOvlap;

	return 0;
}