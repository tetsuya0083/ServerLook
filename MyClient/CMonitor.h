#pragma once

#include <Windows.h>

class CMonitor
{
protected:
	HANDLE d_mutex;

public:
	CMonitor(void);
	CMonitor(char* name);
	~CMonitor(void);

	void On() const;
	void Off() const;

};

