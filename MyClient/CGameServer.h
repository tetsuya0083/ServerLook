#pragma once
#include "CThread.h"

class CGameServer : public CThread
{
public:

	SOCKET m_hSocket;

	int m_RecvWrite;
	char m_RecvBuffer[MAX_RECV];

	int m_SendSize;
	char m_SendBuffer[MAX_SEND];

	bool m_bConnect;
	WSAEVENT m_hEvent;

	int m_IP;
	int m_Port;

	int m_Sel;
public:
	CGameServer(void);
	~CGameServer(void);

	void Clear();

	void SetAddr(char* ip, int port, int sel);

	bool Connect();
	void DisConnect();

	void EmptyRecvBuffer();


	void Recv();
	void Parse(int protocol, char* packet);

	void Send(char* buff, int size);
	int FlushSendBuffer();
	bool AddSendBuffer(char* buff, int size);

	void RecvTOOL_UUC_Ack(char* packet);
	void SendTOOL_UUC_Req();
public:
	unsigned int ThreadProc();

};

