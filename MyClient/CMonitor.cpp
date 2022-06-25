#include "pch.h"
#include "CMonitor.h"

CMonitor::CMonitor(void)
{
	d_mutex = CreateMutex(NULL, false, NULL);
	if (d_mutex == NULL)
		throw MessageBox(NULL, "Monitor() - Mutex creation failed.", NULL, MB_OK);

}
CMonitor::CMonitor(char* name)
{
	d_mutex = CreateMutex(NULL, false, name);
	if (d_mutex == NULL)
		throw MessageBox(NULL, "Monitor() - Mutex creation failed.", NULL, MB_OK);

}

CMonitor::~CMonitor(void)
{
	if (d_mutex != NULL)
	{
		CloseHandle(d_mutex);
		d_mutex = NULL;
	}
}

//lock
void CMonitor::On() const
{
	WaitForSingleObject(d_mutex, INFINITE);
}

//unlock
void CMonitor::Off() const
{
	ReleaseMutex(d_mutex);
}