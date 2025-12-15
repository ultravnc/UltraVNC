// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 2001 HorizonLive.com, Inc. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999 AT&T Laboratories Cambridge. All Rights Reserved.
//


// VSocket.h

// RFB V3.0

// The VSocket class provides simple socket functionality,
// independent of platform. Hurrah.

//#define FLOWCONTROL

class VSocket;
extern BOOL G_ipv6_allowed;
#if (!defined(_ATT_VSOCKET_DEFINED))
#define _ATT_VSOCKET_DEFINED

#include "vtypes.h"
#include <DSMPlugin/DSMPlugin.h>
////////////////////////////
#include <iphlpapi.h>
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

// Socket implementation

// Create one or more VSocketSystem objects per application
class VSocketSystem
{
public:
	VSocketSystem();
	~VSocketSystem();
	VBool Initialised() {return m_status;};
private:
	VBool m_status;
};

// The main socket class
class VSocket
{
public:
  // Constructor/Destructor
  VSocket();
  virtual ~VSocket();

  // Create
  //        Create a socket and attach it to this VSocket object
  VBool Create(); //IPv4
  VBool CreateConnect(const VString address, const VCard port); //IPv6
  VBool CreateBindConnect(const VString address, const VCard port);//IPv6
  VBool	CreateBindListen(const VCard port, const VBool localOnly = VFalse);//IPv6


  // Shutdown
  //        Shutdown the currently attached socket

  VBool Shutdown(); //IPv6  iPV4
  VBool ShutdownIPV6(); //IPv6  iPV4
  VBool ShutdownIPV4(); //IPv6  iPV4
  VBool Shutdown4(); //IPv6
  VBool Shutdown6(); //IPv6


  // Close
  //        Close the currently attached socket
  VBool Close() ;//IPv6  iPV4
  VBool CloseIPV6();//IPv6  iPV4
  VBool CloseIPV4();//IPv6  iPV4
  VBool Close4(); //IPv6
  VBool Close6(); //IPv6


  // Bind
  //        Bind the attached socket to the specified port
  //		If localOnly is VTrue then the socket is bound only
  //        to the loopback adapter.
  VBool Bind4(const VCard port, const VBool localOnly=VFalse); //IPv6
  VBool Bind6(const VCard port, const VBool localOnly=VFalse); //IPv6
  VBool Bind(const VCard port, const VBool localOnly=VFalse); //IPv6


  // Connect
  //        Make a stream socket connection to the specified port
  //        on the named machine.
  VBool Connect(const VString address, const VCard port); //IPv4


  // Listen
  //        Set the attached socket to listen for connections

  VBool Listen4();//IPv6
  VBool Listen6();//IPv6
  VBool Listen();//IPv6


  // Accept
  //        If the attached socket is set to listen then this
  //        call blocks waiting for an incoming connection, then
  //        returns a new socket object for the new connection

  VSocket *Accept(); // IPv6 IPv4
  VSocket* AcceptIPV6(); // IPv6 IPv4
  VSocket* AcceptIPV4(); // IPv6 IPv4
  VSocket *Accept4(); // IPv6
  VSocket *Accept6(); // IPv6


  // GetPeerName
  //        If the socket is connected then this returns the name
  //        of the machine to which it is connected.
  //        This string MUST be copied before the next socket call...
  VString GetPeerName(bool naam);// IPv6 IPv4
  VString GetPeerNameIPV6(bool naam);// IPv6 IPv4
  VString GetPeerNameIPV4(bool naam);// IPv6 IPv4
  VString GetPeerName4(bool naam);// IPv6
  VString GetPeerName6(bool naam);// IPv6


  // GetSockName
  //		If the socket exists then the name of the local machine
  //		is returned. This string MUST be copied before the next
  //		socket call!
  VString GetSockName();// IPv6 IPv4
  VString GetSockNameIPV6();// IPv6 IPv4
  VString GetSockNameIPV4();// IPv6 IPv4
  VString GetSockName4();// IPv6
  VString GetSockName6();// IPv6

  // Resolve
  //        Uses the Winsock API to resolve the supplied DNS name to
  //        an IP address and returns it as an Int32

  static VCard32 Resolve4(const VString name);// IPv6
  static bool Resolve6(const VString name, in6_addr * addr);// IPv6
  static VCard32 Resolve(const VString name);// IPv4


  // SetTimeout
  //        Sets the socket timeout on reads and writes.

  VBool SetSendTimeout(VCard32 msecs);// IPv6 IPv4
  VBool SetRecvTimeout(VCard32 msecs);// IPv6 IPv4
  VBool SetTimeout(VCard32 msecs);// IPv6 IPv4
  VBool SetSendTimeoutIPV6(VCard32 msecs);// IPv6 IPv4
  VBool SetRecvTimeoutIPV6(VCard32 msecs);// IPv6 IPv4
  VBool SetTimeoutIPV6(VCard32 msecs);// IPv6 IPv4
  VBool SetSendTimeoutIPV4(VCard32 msecs);// IPv6 IPv4
  VBool SetRecvTimeoutIPV4(VCard32 msecs);// IPv6 IPv4
  VBool SetTimeoutIPV4(VCard32 msecs);// IPv6 IPv4



  VBool SetTimeout4(VCard32 msecs);// IPv6
  VBool SetSendTimeout4(VCard32 msecs);// IPv6
  VBool SetRecvTimeout4(VCard32 msecs);// IPv6
  VBool SetTimeout6(VCard32 msecs);// IPv6
  VBool SetSendTimeout6(VCard32 msecs);// IPv6
  VBool SetRecvTimeout6(VCard32 msecs);// IPv6

  // adzm 2010-08

  VBool SetDefaultSocketOptions4();// IPv6
  VBool SetDefaultSocketOptions6();// IPv6
  VBool SetDefaultSocketOptions();// IPv4


  // adzm 2010-08
  static void SetSocketKeepAliveTimeoutDefault(int timeout) { m_defaultSocketKeepAliveTimeout = timeout; }

  bool GetPeerAddress4(char *address, int size);// IPv6
  bool GetPeerAddress6(char *address, int size);// IPv6
  bool GetPeerAddress(char *address, int size);// IPv64



 
  VBool ReadSelect(VCard to);
  VBool ReadSelectIPV4(VCard to);
  VBool ReadSelectIPV6(VCard to);
  VInt Send(const char *buff, const VCard bufflen);
  VInt SendIPV6(const char* buff, const VCard bufflen);
  VInt SendIPV4(const char* buff, const VCard bufflen);
  VInt SendQueued(const char *buff, const VCard bufflen);
  VInt SendQueuedIPV6(const char* buff, const VCard bufflen);
  VInt SendQueuedIPV4(const char* buff, const VCard bufflen);
  VInt Read(char *buff, const VCard bufflen);
  VInt ReadIPV6(char* buff, const VCard bufflen);
  VInt ReadIPV4(char* buff, const VCard bufflen);
  VBool SendExact(const char *buff, const VCard bufflen);
  VBool SendExactIPV6(const char* buff, const VCard bufflen);
  VBool SendExactIPV4(const char* buff, const VCard bufflen);
  VBool SendExact(const char *buff, const VCard bufflen, unsigned char msgType);
  VBool SendExactIPV6(const char* buff, const VCard bufflen, unsigned char msgType);
  VBool SendExactIPV4(const char* buff, const VCard bufflen, unsigned char msgType);
  VBool SendExactQueue(const char *buff, const VCard bufflen);
  VBool SendExactQueueIPV6(const char* buff, const VCard bufflen);
  VBool SendExactQueueIPV4(const char* buff, const VCard bufflen);
  VBool SendExactQueue(const char *buff, const VCard bufflen, unsigned char msgType);
  VBool SendExactQueueIPV6(const char* buff, const VCard bufflen, unsigned char msgType);
  VBool SendExactQueueIPV4(const char* buff, const VCard bufflen, unsigned char msgType);
  VBool ReadExact(char *buff, const VCard bufflen);
  VBool ReadExactIPV4(char* buff, const VCard bufflen);
  VBool ReadExactIPV6(char* buff, const VCard bufflen);
  VBool ClearQueue();
  VBool ClearQueueIPV6();
  VBool ClearQueueIPV4();

  VBool ReadSelectSock(VCard to , SOCKET allsock);
  VInt SendSock(const char *buff, const VCard bufflen, SOCKET allsock);
  VInt SendQueuedSock(const char *buff, const VCard bufflen, SOCKET allsock);
  VInt ReadSock(char *buff, const VCard bufflen, SOCKET allsock);
  VBool SendExactSock(const char *buff, const VCard bufflen, SOCKET allsock);
  VBool SendExactSock(const char *buff, const VCard bufflen, unsigned char msgType, SOCKET allsock);
  VBool SendExactQueueSock(const char *buff, const VCard bufflen, SOCKET allsock);
  VBool SendExactQueueSock(const char *buff, const VCard bufflen, unsigned char msgType, SOCKET allsock);
  VBool ReadExactSock(char *buff, const VCard bufflen, SOCKET allsock);
  VBool ClearQueueSock(SOCKET allsock);

  // sf@2002 - DSMPlugin
  //adzm 2009-06-20
  void SetDSMPluginPointer(CDSMPlugin* pDSMPlugin);
  void EnableUsePlugin(bool fEnable) { m_fUsePlugin = fEnable;};
  bool IsUsePluginEnabled(void) { return m_fUsePlugin;};
  //adzm 2010-05-12 - dsmplugin config
  void SetDSMPluginConfig(char* szDSMPluginConfig);
  // adzm 2010-09
  void SetPluginStreamingIn() { m_fPluginStreamingIn = true; }
  void SetPluginStreamingOut() { m_fPluginStreamingOut = true; }
  bool IsPluginStreamingIn(void) { return m_fPluginStreamingIn; }
  bool IsPluginStreamingOut(void) { return m_fPluginStreamingOut; }

  void SetWriteToNetRectBuffer(bool fEnable) {m_fWriteToNetRectBuf = fEnable;}; 
  bool GetWriteToNetRectBuffer(void) {return m_fWriteToNetRectBuf;};
  int  GetNetRectBufOffset(void) {return m_nNetRectBufOffset;};
  void SetNetRectBufOffset(int nValue) {m_nNetRectBufOffset = nValue;};
  BYTE* GetNetRectBuf(void) {return m_pNetRectBuf;};
  void CheckNetRectBufferSize(int nBufSize);
  VBool SendExactHTTP(const char *buff, const VCard bufflen);
  VBool ReadExactHTTP(char *buff, const VCard bufflen);

  //adzm 2010-05-10
  IIntegratedPlugin* GetIntegratedPlugin() { return m_pIntegratedPluginInterface; };

  //adzm 2010-08-01
  DWORD GetLastSentTick() { return m_LastSentTick; };
  IIntegratedPlugin* m_pIntegratedPluginInterface;
  ////////////////////////////
  // Internal structures
protected:
  // The internal socket id
    SOCKET sock4;
    SOCKET sock6;
    SOCKET sock;


  //adzm 2010-08-01
  DWORD m_LastSentTick;

  CDSMPlugin* m_pDSMPlugin; // sf@2002 - DSMPlugin
  //adzm 2009-06-20
  IPlugin* m_pPluginInterface;
  //adzm 2010-05-10
  bool m_fUsePlugin;
  bool m_fPluginStreamingIn; //adzm 2010-09
  bool m_fPluginStreamingOut; //adzm 2010-09
  omni_mutex m_TransMutex;
  omni_mutex m_RestMutex;
  omni_mutex m_CheckMutex;

  //adzm 2009-06-20
  BYTE* TransformBuffer(BYTE* pDataBuffer, int nDataLen, int* nTransformedDataLen);
  BYTE* RestoreBufferStep1(BYTE* pDataBuffer, int nDataLen, int* nRestoredDataLen);
  BYTE* RestoreBufferStep2(BYTE* pDataBuffer, int nDataLen, int* nRestoredDataLen);

  // All this should be private with accessors -> later
  BYTE* m_pNetRectBuf;
  bool m_fWriteToNetRectBuf;
  int m_nNetRectBufOffset;
  int m_nNetRectBufSize;

  char queuebuffer[9000];
  DWORD queuebuffersize;

  // adzm 2010-08
  static int m_defaultSocketKeepAliveTimeout;

  bool							GetOptimalSndBuf();
  unsigned int							G_SENDBUFFER;
};

#endif // _ATT_VSOCKET_DEFINED
