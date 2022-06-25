#include "pch.h"
#include "CGameServer.h"


unsigned int CGameServer::ThreadProc()
{
	WSANETWORKEVENTS events;

	DWORD dwReturn = 0, dwRet = 0;

	while (m_bRun)
	{
		dwReturn = WSAWaitForMultipleEvents(1, &m_hEvent, FALSE, WSA_INFINITE, FALSE);

		if (dwReturn != WSA_WAIT_FAILED)
		{
			if (m_hSocket)
			{
				dwRet = WSAEnumNetworkEvents(m_hSocket, m_hEvent, &events);
				if (dwRet == 0)
				{
					if ((events.lNetworkEvents & FD_READ) == FD_READ)
					{
						Recv();
					}
					if ((events.lNetworkEvents & FD_WRITE) == FD_WRITE)
					{
						FlushSendBuffer();
					}
					if ((events.lNetworkEvents & FD_CLOSE) == FD_CLOSE)
					{
						DisConnect();
					}
				}
			}
		}
		Sleep(1);
	}
	return 1;
}



CGameServer::CGameServer(void)
{
	m_bConnect = false;
	m_hSocket = NULL;

	memset(m_RecvBuffer, 0x00, sizeof(m_RecvBuffer));
	memset(m_SendBuffer, 0x00, sizeof(m_SendBuffer));

	m_RecvWrite = 0;
	m_SendSize = 0;

	m_hEvent = NULL;
	m_IP = 0;
	m_Port = 0;
	m_Sel = -1;
}

CGameServer::~CGameServer(void)
{
	Clear();
}

void CGameServer::Clear()
{
	m_bConnect = false;
	SAFE_CLOSESOCK(m_hSocket);

	memset(m_RecvBuffer, 0x00, sizeof(m_RecvBuffer));
	memset(m_SendBuffer, 0x00, sizeof(m_SendBuffer));

	m_RecvWrite = 0;
	m_SendSize = 0;

	if (m_hEvent)
	{
		WSACloseEvent(m_hEvent);
		m_hEvent = NULL;
	}

	m_IP = 0;
	m_Port = 0;
}

void CGameServer::SetAddr(char* ip, int port, int sel)
{
	m_IP = inet_addr(ip);
	m_Port = port;
	m_Sel = sel;
}

bool CGameServer::Connect()
{
	sockaddr_in ServerAddress;
	m_hSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (m_hSocket == INVALID_SOCKET)
	{
		return false;
	}

	ServerAddress.sin_family = AF_INET;
	ServerAddress.sin_addr.s_addr = m_IP;
	ServerAddress.sin_port = htons(m_Port);

	int error = connect(m_hSocket, (struct sockaddr*)&ServerAddress, sizeof(ServerAddress));
	if (error == SOCKET_ERROR)
	{
		SAFE_CLOSESOCK(m_hSocket);

		return false;
	}

	m_hEvent = WSACreateEvent();

	WSAEventSelect(m_hSocket, m_hEvent, FD_READ | FD_WRITE | FD_CLOSE);

	BOOL opt_val = TRUE;
	error = setsockopt(m_hSocket, IPPROTO_TCP, TCP_NODELAY, (char*)&opt_val, sizeof(opt_val));

	DWORD size = 65535;
	setsockopt(m_hSocket, SOL_SOCKET, SO_SNDBUF, (const char*)&size, sizeof(size));
	setsockopt(m_hSocket, SOL_SOCKET, SO_RCVBUF, (const char*)&size, sizeof(size));

	m_bConnect = true;

	CThread::Begin();

	return true;
}

void CGameServer::DisConnect()
{
	SAFE_CLOSESOCK(m_hSocket);
	m_bConnect = false;

	memset(m_RecvBuffer, 0x00, sizeof(m_RecvBuffer));
	memset(m_SendBuffer, 0x00, sizeof(m_SendBuffer));

	m_RecvWrite = 0;
	m_SendSize = 0;

	if (m_hEvent)
	{
		WSACloseEvent(m_hEvent);
		m_hEvent = NULL;
	}

	CThread::End();
}

void CGameServer::EmptyRecvBuffer()
{
	memset(m_RecvBuffer, 0x00, sizeof(m_RecvBuffer));
	m_RecvWrite = 0;
}

void CGameServer::Send(char* buff, int size)
{
	if (m_hSocket == NULL)
	{
		return;
	}

	if (buff == NULL)
		return;

	int sendsize, error = 0;
	if (m_SendSize <= 0) // 큐가 비어있는 경우만 호출
	{
		do
		{
			sendsize = send(m_hSocket, buff, size, 0);

			if (sendsize < 0)
			{
				AddSendBuffer(buff, size);
				break;
			}
			else
			{
				buff = buff + sendsize; // 버퍼의 위치를 보낸만큼 뒤로 뷁
				size -= sendsize;		// 패킷 사이즈를 보낸만큼 빼준다.
			}
		} while (size);//Size가 0이 될때까지..보낸다
	}
	else // 큐가 비어있지 않다면 보낼 데이터를 큐에 쌓고, 버퍼를 초과하지 않았다면 FlushBuffer();를 호출해서 처리한다.
	{
		if (AddSendBuffer(buff, size))
		{
			FlushSendBuffer();
		}
		else
		{
			FlushSendBuffer();
		}
	}
}

bool CGameServer::AddSendBuffer(char* buff, int size)
{
	if (buff == NULL)
		return false;
	if (m_SendSize + size >= MAX_SEND)
	{
		return false;
	}

	memcpy(&m_SendBuffer[m_SendSize], buff, size);
	m_SendSize += size;
	return true;
}

int CGameServer::FlushSendBuffer()
{

	int sendsize = 0;
	do
	{
		sendsize = send(m_hSocket, m_SendBuffer, m_SendSize, 0);
		if (sendsize == SOCKET_ERROR)
		{
			unsigned int erro = WSAGetLastError();
			return erro;
		}
		else
		{
			memmove(m_SendBuffer, &m_SendBuffer[sendsize], m_SendSize - sendsize);
			m_SendSize -= sendsize;
		}
	} while (m_SendSize);

	return m_SendSize;
}

void CGameServer::Recv()
{
	if (m_hSocket == NULL)
		return;

	int size = 0;
	if (m_RecvWrite < MAX_RECV)
		size = recv(m_hSocket, &m_RecvBuffer[m_RecvWrite], MAX_RECV - m_RecvWrite, 0);

	if (size > 0)
	{
		//현재 리시브버퍼의 길이에 더한다
		m_RecvWrite += size;

		if (m_RecvWrite >= MAX_RECV)
		{
			EmptyRecvBuffer();
			return;
		}

		while (m_RecvWrite >= HEADSIZE)
		{
			stHeader header;
			memcpy(&header, m_RecvBuffer, HEADSIZE);

			if (header.iID >= TCP_PROTOCOL_END || header.iID <= TCP_PROTOCOL_START)
			{
				EmptyRecvBuffer();
				return;
			}
			if (header.iLength <= 0)
			{
				EmptyRecvBuffer();
				return;
			}
			int iCheckSum = header.Type.iType + header.iLength + header.iID;
			if (header.iCheckSum != iCheckSum)
			{
				EmptyRecvBuffer();

				return;
			}

			if (m_RecvWrite >= header.iLength)
			{
				Parse(header.iID, m_RecvBuffer);
				memmove(m_RecvBuffer, &m_RecvBuffer[header.iLength], m_RecvWrite);
				m_RecvWrite -= header.iLength;

			}
			else
			{
				break;
			}
		}
	}
}

void CGameServer::Parse(int protocol, char* packet)
{
	switch (protocol)
	{
	case prTOOL_UUC_Ack: RecvTOOL_UUC_Ack(packet);	break;
	default: break;
	}
}

void CGameServer::SendTOOL_UUC_Req()
{
	stTOOL_UUC_Req req;
	char buffer[128];	memset(buffer, 0x00, sizeof(buffer));
	memcpy(buffer, &req, sizeof(stTOOL_UUC_Req));
	Send(buffer, sizeof(stTOOL_UUC_Req));
}

void CGameServer::RecvTOOL_UUC_Ack(char* packet)
{
	stTOOL_UUC_Ack ack;
	memcpy(&ack, packet, sizeof(stTOOL_UUC_Ack));

	CServerLookDlg* pDlg = (CServerLookDlg*)GetMainDlg();

	if (pDlg)
	{
		pDlg->m_List.SetItemTextFmt(m_Sel, 2, "%d", ack.nCount);
	}
}


