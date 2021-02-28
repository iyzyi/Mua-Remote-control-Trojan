#pragma once

#include "pch.h"
#include "Misc.h"
#include "Crypto.h"


class CSocketClient;
class CClient;


// 这里的包头长度不包含表示封包长度的那4个字节
// 不能用(sizeof(PACKET_HEAD))，结构体元素不一定连续
#define PACKET_HEAD_LENGTH 7

// 封包长度受HP-Socket的限制，最大为0x3FFFFF
#define PACKET_MAX_LENGTH 0x3FFFFF

// 包体最大长度
#define PACKET_BODY_MAX_LENGTH ((PACKET_MAX_LENGTH) - (PACKET_HEAD_LENGTH) - sizeof(DWORD) - 0x1000)
// 定义成(PACKET_MAX_LENGTH) - (PACKET_HEAD_LENGTH) - sizeof(DWORD)会有奇妙的溢出，保险起见减去个0x1000



typedef struct _PACKET_HEAD {
	WORD		wCommandId;					// 命令号
	DWORD		dwCheckSum;					// 序列号
	BYTE		bySplitNum;					// 封包分片数量, 最多255个分片，使得最大能够传输将近1G的数据。
											//	 BYTE就够用了，更大的数据量的话还是换个协议吧，这个通信协议没有校验机制。

	_PACKET_HEAD(PBYTE pbData) {
		wCommandId = GetWordFromBuffer(pbData, 0);
		dwCheckSum = GetDwordFromBuffer(pbData, 2);
		bySplitNum = GetByteFromBuffer(pbData, 6);
	}

	_PACKET_HEAD() {
		wCommandId = 0;
		dwCheckSum = 0;
		bySplitNum = 0;
	}

	VOID StructToBuffer(PBYTE pbOutBuffer) {
		WriteWordToBuffer(pbOutBuffer, wCommandId, 0);
		WriteDwordToBuffer(pbOutBuffer, dwCheckSum, 2);
		WriteByteToBuffer(pbOutBuffer, bySplitNum, 6);
	}

}PACKET_HEAD, *PPACKET_HEAD;




class CPacket {

public:

	CPacket(CSocketClient* pSocketClient);
	CPacket();

	BOOL PacketParse(PBYTE pbData, DWORD dwLength);
	VOID PacketCombine(COMMAND_ID wCommandId, PBYTE pbPacketBody, DWORD dwPacketBodyLength);


	~CPacket();

public:
	CONNID				m_dwConnId;
	CSocketClient*		m_pSocketClient;			// 所属socket
	CClient*			m_pClient;					// 所属客户端
	
	DWORD				m_dwPacketLength;			// 整个封包的长度(包括包头和包体，但不包括封包中表示长度的4个字节)
	PACKET_HEAD			m_PacketHead;				// 包头
	PBYTE				m_pbPacketBody;				// 包体
	
	DWORD				m_dwPacketBodyLength;		// 包体长度

	PBYTE				m_pbPacketPlaintext;
	PBYTE				m_pbPacketCiphertext;
};