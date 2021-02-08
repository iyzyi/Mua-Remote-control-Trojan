#pragma once

#include "pch.h"
#include "Packet.h"
#include "Crypto.h"
#include "Misc.h"
#include "ClientManage.h"


typedef void (CALLBACK* NOTIFYPROC)(CPacket &Packet);		// NOTIFYPROC: 通知程序，是回调函数



// 继承自CTcpServerListener
class CSocketServer : public CTcpServerListener {
public:

	CSocketServer();
	~CSocketServer();

	BOOL SendPacket(CONNID dwConnectId, COMMAND_ID dwCommandId, PBYTE pbPacketBody, DWORD dwPacketBodyLength);
	VOID SendPacketToAllClient(COMMAND_ID dwCommandId, PBYTE pbPacketBody, DWORD dwLength);

	BOOL StartSocketServer(NOTIFYPROC pfnNotifyProc, LPCTSTR lpszIpAddress, USHORT wPort);
	BOOL StopSocketServer();

	BOOL IsRunning();

public:
	CTcpPackServerPtr			m_pServer;

	NOTIFYPROC					m_pfnManageRecvPacket;	// 回调函数，接收到的封包均传给这个函数处理，
														// 在StartSocketServer的时候，通过参数，把回调函数的地址传进来

	CClientManage				m_ClientManage;

protected:
	BOOL						m_bIsRunning;

protected:
	// 与管理Client连接相关
	VOID ListAddClient(CONNID ConnectId);
	VOID ListDeleteClient(CONNID ConnectId);

	// CTcpServerListener的抽象函数（用于回调）全都得实现，不然会报错：不能实例化抽象类
	virtual EnHandleResult OnPrepareListen(ITcpServer* pSender, SOCKET soListen);
	virtual EnHandleResult OnAccept(ITcpServer* pSender, CONNID dwConnID, SOCKET soClient);
	virtual EnHandleResult OnHandShake(ITcpServer* pSender, CONNID dwConnID);
	virtual EnHandleResult OnSend(ITcpServer* pSender, CONNID dwConnID, const BYTE* pData, int iLength);
	virtual EnHandleResult OnReceive(ITcpServer* pSender, CONNID dwConnID, const BYTE* pData, int iLength);
	virtual EnHandleResult OnClose(ITcpServer* pSender, CONNID dwConnID, EnSocketOperation enOperation, int iErrorCode);
	virtual EnHandleResult OnShutdown(ITcpServer* pSender);
};