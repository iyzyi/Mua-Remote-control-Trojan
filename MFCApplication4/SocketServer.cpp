#include "pch.h"
#include "SocketServer.h"
#include "MFCApplication4.h"
#include "MFCApplication4Dlg.h"



CSocketServer::CSocketServer() : m_pServer(this) {
	m_bIsRunning = false;
	//m_pfnMainSocketRecvPacket = NULL;

	// 设置数据包最大长度（有效数据包最大长度不能超过0x3FFFFF字节(4MB-1B)，默认：262144/0x40000 (256KB)
	m_pServer->SetMaxPackSize(PACKET_MAX_LENGTH);
	// 设置心跳检测包发送间隔
	m_pServer->SetKeepAliveTime(60 * 1000);
	// 设置心跳检测重试包发送间隔
	m_pServer->SetKeepAliveInterval(20 * 1000);
}


CSocketServer::~CSocketServer() {

}


// 启动socket服务端
BOOL CSocketServer::StartSocketServer(LPCTSTR lpszIpAddress, USHORT wPort) {

	BOOL bRet = m_pServer->Start(lpszIpAddress, wPort);
	if (!bRet) {
		return false;
	} else {

#ifdef _DEBUG
		USES_CONVERSION;									// 使用A2W之前先声明这个
		printf("Socket服务端启动成功，IP=%s, PORT=%d\n", W2A(lpszIpAddress), wPort);
#endif

		// 设置回调函数
		//m_pfnMainSocketRecvPacket = pfnMainSocketRecvPacket;
		//m_pfnChildSocketRecvPacket = pfnChildSocketRecvPacket;

		// 初始化ClientManage的Client链表
		m_ClientManage = CClientManage();

		m_bIsRunning = true;
		return true;
	}
}


BOOL CSocketServer::StopSocketServer() {
	BOOL bRet = m_pServer->Stop();
	if (bRet) {
		m_bIsRunning = false;
		return true;
	}
	else {
		return false;
	}
}




BOOL CSocketServer::SendPacket(CONNID dwConnectId, COMMAND_ID dwCommandId, PBYTE pbPacketBody, DWORD dwPacketBodyLength) {
	BOOL bRet;
	CClient *pClient = m_ClientManage.SearchClient(dwConnectId);
	if (pClient != NULL) {
		bRet = SendPacket(pClient, dwCommandId, pbPacketBody, dwPacketBodyLength);
	} else {
		bRet = false;
	}
	return bRet;
}


BOOL CSocketServer::SendPacket(CClient* pClient, COMMAND_ID dwCommandId, PBYTE pbPacketBody, DWORD dwPacketBodyLength) {
	// 发包只需要ConnectId就能发，但是通信的密钥在CClient类对象里面，
	// CPacket的封包加密需要CClient里面的密钥，所以必须传入CClient参数。

	CPacket Packet = CPacket(pClient);
	Packet.PacketCombine(dwCommandId, pbPacketBody, dwPacketBodyLength);
	BOOL bRet = m_pServer->Send(pClient->m_dwConnectId, Packet.m_pbPacketCiphertext, Packet.m_dwPacketLength);
	return bRet;
}


VOID CSocketServer::SendPacketToAllClient(COMMAND_ID dwCommandId, PBYTE pbPacketBody, DWORD dwPacketBodyLength) {
	CClient *pClientNode = m_ClientManage.m_pClientListHead;
	while (pClientNode->m_pNextClient != NULL) {
		SendPacket(pClientNode->m_pNextClient, dwCommandId, pbPacketBody, dwPacketBodyLength);
		pClientNode = pClientNode->m_pNextClient;
	}
}




BOOL CSocketServer::IsRunning() {
	return m_bIsRunning;
}




// 回调函数的实现

EnHandleResult CSocketServer::OnPrepareListen(ITcpServer* pSender, SOCKET soListen) {
	printf("OnPrepareListen: \n");
	return HR_OK;
}


EnHandleResult CSocketServer::OnAccept(ITcpServer* pSender, CONNID dwConnID, SOCKET soClient) {
	printf("[Client %d] OnAccept: \n", dwConnID);
	return HR_OK;
}


EnHandleResult CSocketServer::OnHandShake(ITcpServer* pSender, CONNID dwConnID) {
	printf("[Client %d] OnHandShake: \n", dwConnID);
	return HR_OK;
}


EnHandleResult CSocketServer::OnSend(ITcpServer* pSender, CONNID dwConnID, const BYTE* pData, int iLength) {
	printf("[Client %d] OnSend: \n", dwConnID);
	PrintBytes((LPBYTE)pData, iLength);
	return HR_OK;
}


EnHandleResult CSocketServer::OnReceive(ITcpServer* pSender, CONNID dwConnID, const BYTE* pData, int iLength) {
	printf("[Client %d] OnReceive: \n", dwConnID);
	PrintData((PBYTE)pData, iLength);
	
	CClient* pClient = m_ClientManage.SearchClient(dwConnID);
	if (pClient == NULL) {						// 新客户端来啦(可能是新的主socket，也可能是已知客户端的新的子socket)

		// 第一个封包是AES的key和iv，所以长度必须满足条件。否则丢弃该包，以免拒绝服务。
		if (iLength == CRYPTO_KEY_PACKET_LENGTH 
			&& (pData[0] == CRYPTO_KEY_PACKET_TOKEN_FOR_MAIN_SOCKET 
			|| pData[0] == CRYPTO_KEY_PACKET_TOKEN_FOR_CHILD_SOCKET) ) {

			TCHAR lpszIpAddress[20];
			int iIpAddressLen = 20;
			WORD wPort = 0;
			// 通过ConnectId获取IP地址和端口
			m_pServer->GetRemoteAddress(dwConnID, lpszIpAddress, iIpAddressLen, wPort);

			BOOL bIsMainSocketServer = (pData[0] == CRYPTO_KEY_PACKET_TOKEN_FOR_MAIN_SOCKET) ? true : false;
			CClient* pClientNew = new CClient(dwConnID, (LPWSTR)lpszIpAddress, wPort, bIsMainSocketServer);
			m_ClientManage.AddNewClientToList(pClientNew);

			// 设置该Client的密钥
			BYTE pbKey[16];
			BYTE pbIv[16];
			memcpy(pbKey, pData + 1, 16);
			memcpy(pbIv, pData + 17, 16);
			pClientNew->SetCryptoKey(pbKey, pbIv);

			// 告知客户端，我服务端这边已经接收到宁的密钥了
			SendPacket(pClientNew, CRYPTO_KEY, NULL, 0);

		} // if (iLength == FIRST_PACKET_LENGTH)

	} // if (pClient == NULL) 新客户端
	else {

		CPacket* pPacket = new CPacket(pClient);
		BOOL isValidPacket = pPacket->PacketParse((PBYTE)pData, iLength);

		if (isValidPacket) {								// 有效封包

			if (pClient->m_bIsMainSocketServer) {
				//m_pfnMainSocketRecvPacket(pPacket);				// 处理主socket封包的回调函数
				PostMessage(theApp.m_pMainWnd->m_hWnd, WM_RECV_MAIN_SOCKET_CLIENT_PACKET, NULL, (LPARAM)pPacket);
			}
			else {
				//m_pfnChildSocketRecvPacket(pPacket);
				PostMessage(theApp.m_pMainWnd->m_hWnd, WM_RECV_CHILD_SOCKET_CLIENT_PACKET, NULL, (LPARAM)pPacket);
			}
		}

		// TODO： 丢弃的包达到一定次数即判定为拒绝服务
	}

	return HR_OK;
}


EnHandleResult CSocketServer::OnClose(ITcpServer* pSender, CONNID dwConnID, EnSocketOperation enOperation, int iErrorCode) {
	printf("[Client %d] OnClose: \n", dwConnID);

	//m_ClientManage.DeleteClientFromList(dwConnID);

	theApp.m_pMainWnd->PostMessage(WM_CLIENT_DISCONNECT, dwConnID, NULL);

	return HR_OK;
}


EnHandleResult CSocketServer::OnShutdown(ITcpServer* pSender) {
	printf("OnShutdown: \n");
	printf("Socket服务端关闭成功\n");
	return HR_OK;
}