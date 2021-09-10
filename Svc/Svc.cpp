
#include <windows.h>
#include <tchar.h>
#include <strsafe.h>

#include "SvcWorker.h"
#include "resource.h"

#pragma comment(lib, "advapi32.lib")

#define SVCNAME TEXT("MonService")

SERVICE_STATUS g_svcStatus;
SERVICE_STATUS_HANDLE g_svcStatusHdl;
HANDLE g_svcStopEvent = NULL;

VOID SvcInstall(VOID);
VOID WINAPI SvcCtrlHandler(DWORD);
VOID WINAPI SvcMain(DWORD, LPTSTR*);

VOID ReportSvcStatus(DWORD, DWORD, DWORD);
VOID SvcInit(DWORD, LPTSTR*);
VOID SvcReportEvent(LPTSTR);

int _cdecl _tmain(int argc, TCHAR* argv[])
{
	if (lstrcmpi(argv[1], TEXT("install")) == 0)
	{
		SvcInstall();
		return 0;
	}

	TCHAR svcname[100] = SVCNAME;

	//TODO add aditional service needed

	SERVICE_TABLE_ENTRY DispatchTable[] =
	{
		{svcname,(LPSERVICE_MAIN_FUNCTION)SvcMain},
		{NULL,NULL}
	};

	if (!StartServiceCtrlDispatcher(DispatchTable))
	{
		TCHAR temp[30] = TEXT("StartServiceCtrlDispatcher");
		SvcReportEvent(temp);
	}
}

VOID SvcInstall(VOID)
{
	SC_HANDLE schSCManager;
	SC_HANDLE schSCService;
	TCHAR szPath[MAX_PATH];
	if (!GetModuleFileName(NULL, szPath, MAX_PATH))
	{
		printf("Cannot install service (%d)\n", GetLastError());
		return;
	}

	schSCManager = OpenSCManager(
		NULL, NULL, SC_MANAGER_ALL_ACCESS
	);

	if (schSCManager == NULL)
	{
		printf("OpenSCManager failed (%d)\n", GetLastError());
		return;
	}

	schSCService = CreateService(
		schSCManager,
		SVCNAME,
		SVCNAME,
		SERVICE_ALL_ACCESS,
		SERVICE_WIN32_OWN_PROCESS,
		SERVICE_DEMAND_START,
		SERVICE_ERROR_NORMAL,
		szPath,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL
	);

	if (schSCService == NULL)
	{
		printf("CreateService failed (%d)\n", GetLastError());
		CloseServiceHandle(schSCManager);
		return;
	}
	else printf("Service installed successfully\n");

	CloseServiceHandle(schSCManager);
	CloseServiceHandle(schSCService);
}

VOID WINAPI SvcMain(DWORD dwArgc, LPTSTR* lpszArgv)
{
	g_svcStatusHdl = RegisterServiceCtrlHandler(
		SVCNAME, SvcCtrlHandler
	);

	if (!g_svcStatusHdl)
	{
		TCHAR msg[100] = TEXT("RegisterServiceCtrlHandler");
		SvcReportEvent(msg);
	}

	g_svcStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	g_svcStatus.dwServiceSpecificExitCode = 0;

	ReportSvcStatus(SERVICE_START_PENDING, NO_ERROR, 3000);

	SvcInit(dwArgc, lpszArgv);
}

VOID SvcInit(DWORD dwArgc, LPTSTR* lpszArgv)
{
	//TODO DECLARE AND SET ANY VARIABLE call periodically reportStatus with startPending, or service_stopped if initialisation failed
	InitializeServiceWorker();

	g_svcStopEvent = CreateEvent(
		NULL,
		TRUE,
		FALSE,
		NULL
	);

	if (g_svcStopEvent == NULL)
	{
		ReportSvcStatus(SERVICE_STOPPED, GetLastError(), 0);
		return;
	}

	ReportSvcStatus(SERVICE_RUNNING, NO_ERROR, 0);

	//TO_DO : do the stuff here
	
	//start thread for file
	HANDLE hthread = CreateThread(NULL, 0, RunServiceWoker, NULL, 0, NULL);
	
	while (1)
	{
		WaitForSingleObject(g_svcStopEvent, INFINITE);

		// clean up code (call reportStatus with stop_pending ?)
		DeleteServiceWOrker();

		ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);
		return;
	}
}

VOID ReportSvcStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint)
{
	static DWORD dwCheckPoint = 1;

	g_svcStatus.dwCurrentState = dwCurrentState;
	g_svcStatus.dwWin32ExitCode = dwWin32ExitCode;
	g_svcStatus.dwWaitHint = dwWaitHint;

	if (dwCurrentState == SERVICE_START_PENDING)
		g_svcStatus.dwControlsAccepted = 0;
	else g_svcStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;

	if ((dwCurrentState == SERVICE_RUNNING) || (dwCurrentState == SERVICE_STOPPED))
		g_svcStatus.dwCheckPoint = 0;
	else g_svcStatus.dwCheckPoint = dwCheckPoint++;

	SetServiceStatus(g_svcStatusHdl, &g_svcStatus);
}

VOID WINAPI SvcCtrlHandler(DWORD dwCtrl)
{
	switch (dwCtrl)
	{
	case SERVICE_CONTROL_STOP:
		ReportSvcStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);

		//signal service to stop
		SetEvent(g_svcStopEvent);
		ReportSvcStatus(g_svcStatus.dwCurrentState, NO_ERROR, 0);
		return;
	case SERVICE_CONTROL_INTERROGATE:
		break;
	default:
		break;
	}
}

VOID SvcReportEvent(LPTSTR szFunction)
{
	HANDLE hEventSource;
	LPCTSTR  lpszStrings[2];
	TCHAR Buffer[80];

	hEventSource = RegisterEventSource(NULL, SVCNAME);

	if (hEventSource != NULL)
	{
		StringCchPrintf(Buffer, 80, TEXT("%s failed with %d"), szFunction, GetLastError());

		lpszStrings[0] = SVCNAME;
		lpszStrings[1] = Buffer;

		ReportEvent(
			hEventSource,
			EVENTLOG_ERROR_TYPE,
			0,
			SVC_ERROR,
			NULL,
			2,
			0,
			lpszStrings,
			NULL
		);
		DeregisterEventSource(hEventSource);
	}
}