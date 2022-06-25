#pragma once
#include "CMonitor.h"

class CThread : public CMonitor
{
public:
	DWORD m_ThredID;
	bool m_bRun;
	HANDLE m_ThreadHandle;

public:
	CThread(void);
	~CThread(void);
	bool Begin();
	void End();
	bool IsRun();
	virtual unsigned int ThreadProc() { return 0; };
};