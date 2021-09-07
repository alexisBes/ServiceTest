#include <windows.h>
#include <tchar.h>
#include <strsafe.h>
#include <AclAPI.h>
#include <stdio.h>

#pragma comment(lib,"advapi32.lib")

TCHAR szCommand[10];
TCHAR szSvcName[80];

SC_HANDLE schSvcManager;
SC_HANDLE schService;

VOID __stdcall DisplayUsage(void);

VOID __stdcall DoStartSvc(void);
VOID __stdcall DoUpdatSvcDacl(void);
VOID __stdcall DoStopSvc(void);

BOOL __stdcall StopDependentService(void);

void _tmain(int argc, TCHAR* argv[])
{
	printf("\n");
	if (argc != 3)
	{
		printf("ERROR: Incorrect number of arguments\n\n");
		DisplayUsage();
		return;
	}
	StringCchCopy(szCommand, 10, argv[1]);
	StringCchCopy(szSvcName, 80, argv[2]);

	if (lstrcmpi(szCommand, TEXT("start")) == 0)
		DoStartSvc();
	else if (lstrcmpi(szCommand, TEXT("dacl")) == 0)
		DoUpdatSvcDacl();
	else if (lstrcmpi(szCommand, TEXT("stop")) == 0)
		DoStopSvc();
	else
	{
		_tprintf(TEXT("Unknown command (%s)\n\n"), szCommand);
		DisplayUsage();
	}
}

VOID __stdcall DisplayUsage()
{
	printf("Description:\n");
	printf("\tCommand-line tool that controls a service.\n\n");
	printf("Usage:\n");
	printf("\tsvccontrol [command] [service_name]\n\n");
	printf("\t[command]\n");
	printf("\t  start\n");
	printf("\t  dacl\n");
	printf("\t  stop\n");
}

//starrt the service
VOID __stdcall DoStartSvc(void)
{
	SERVICE_STATUS_PROCESS ssStatus;
	DWORD dwOldCheckPoint;
	DWORD dwStartTickCount;
	DWORD dwWaitTime;
	DWORD dwBytesNeeded;

	schSvcManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (schSvcManager == NULL)
	{
		printf("OpenSCManager failed (%d)\n", GetLastError());
		return;
	}

	schService = OpenService(schSvcManager, szSvcName, SERVICE_ALL_ACCESS);
	if (schService == NULL)
	{
		printf("OpenService failed (%d)\n", GetLastError());
		CloseServiceHandle(schSvcManager);
		return;
	}

	//check if the service is stop
	if (!QueryServiceStatusEx(schService, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssStatus, sizeof(SERVICE_STATUS_PROCESS), &dwBytesNeeded))
	{
		printf("QueryServiceStatusEx failed (%d)\n", GetLastError());
		CloseServiceHandle(schService);
		CloseServiceHandle(schSvcManager);
		return;
	}

	//if the service still running, stop the function
	//it's possible to stop the service and continue this function for a restart
	if (ssStatus.dwCurrentState != SERVICE_STOPPED && ssStatus.dwCurrentState != SERVICE_STOP_PENDING)
	{
		printf("Cannot start the service because it is already running\n");
		CloseServiceHandle(schService);
		CloseServiceHandle(schSvcManager);
		return;
	}

	// save tick and checkpoint
	dwStartTickCount = GetTickCount64();// GetTickCount()
	dwOldCheckPoint = ssStatus.dwCheckPoint;

	// wait for the signal to stop
	while (ssStatus.dwCurrentState == SERVICE_STOP_PENDING)
	{
		// wait tenth of the service wait time
		dwWaitTime = ssStatus.dwWaitHint / 10;
		if (dwWaitTime < 1000)
			dwWaitTime = 1000;
		else if (dwWaitTime > 10000)
			dwWaitTime = 10000;
		Sleep(dwWaitTime);

		if (!QueryServiceStatusEx(schService, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssStatus, sizeof(SERVICE_STATUS_PROCESS), &dwBytesNeeded))
		{
			printf("QueryServiceStatusEx failed (%d)\n", GetLastError());
			CloseServiceHandle(schService);
			CloseServiceHandle(schSvcManager);
			return;
		}

		if (ssStatus.dwCheckPoint > dwOldCheckPoint)
		{
			// continue to wait
			dwStartTickCount = GetTickCount64();// GetTickCount()
			dwOldCheckPoint = ssStatus.dwCheckPoint;
		}
		else
		{
			if (GetTickCount64() - dwStartTickCount > ssStatus.dwWaitHint)
			{
				printf("Timeout waiting to service to stop\n");
				CloseServiceHandle(schService);
				CloseServiceHandle(schSvcManager);
				return;
			}
		}
	}

	// start the service
	if (!StartService(schService, 0, NULL))
	{
		printf("StartService failed (%d)\n", GetLastError());
		CloseServiceHandle(schService);
		CloseServiceHandle(schSvcManager);
		return;
	}
	else printf("Service start pending ...\n");

	//check the status until it not pending
	if (!QueryServiceStatusEx(schService, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssStatus, sizeof(SERVICE_STATUS_PROCESS), &dwBytesNeeded))
	{
		printf("QueryServiceStatusEx failed (%d)\n", GetLastError());
		CloseServiceHandle(schService);
		CloseServiceHandle(schSvcManager);
		return;
	}

	// save tick & checkpoint
	dwStartTickCount = GetTickCount64();// GetTickCount()
	dwOldCheckPoint = ssStatus.dwCheckPoint;
	while (ssStatus.dwCurrentState == SERVICE_START_PENDING)
	{
		dwWaitTime = ssStatus.dwWaitHint / 10;
		if (dwWaitTime < 1000)
			dwWaitTime = 1000;
		else if (dwWaitTime > 10000)
			dwWaitTime = 10000;

		Sleep(dwWaitTime);

		if (!QueryServiceStatusEx(schService, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssStatus, sizeof(SERVICE_STATUS_PROCESS), &dwBytesNeeded))
		{
			printf("QueryServiceStatusEx failed (%d)\n", GetLastError());
			CloseServiceHandle(schService);
			CloseServiceHandle(schSvcManager);
			return;
		}

		if (ssStatus.dwCheckPoint > dwOldCheckPoint)
		{
			// continue to wait
			dwStartTickCount = GetTickCount64();// GetTickCount()
			dwOldCheckPoint = ssStatus.dwCheckPoint;
		}
		else
		{
			if (GetTickCount64() - dwStartTickCount > ssStatus.dwWaitHint)
			{
				// no progress has been made
				break;
			}
		}
	}

	if (ssStatus.dwCurrentState == SERVICE_RUNNING)
	{
		printf("Service Started successfully. \n");
	}
	else
	{
		printf("Service not started. \n");
		printf("  Current State: %d \n", ssStatus.dwCurrentState);
		printf("  Exit Code: %d\n", ssStatus.dwWin32ExitCode);
		printf("  Check Point: %d\n", ssStatus.dwCheckPoint);
		printf("  Wait Hint: %d\n", ssStatus.dwWaitHint);
	}
	CloseServiceHandle(schService);
	CloseServiceHandle(schSvcManager);
}

// update DACL for guest account
VOID __stdcall DoUpdatSvcDacl(void)
{
	EXPLICIT_ACCESS ea;
	SECURITY_DESCRIPTOR sd;
	PSECURITY_DESCRIPTOR psd = NULL;
	PACL pacl = NULL;
	PACL pNewACL = NULL;
	BOOL bDaclPresent = FALSE;
	BOOL bDaclDefaulted = FALSE;
	DWORD dwError = 0;
	DWORD dwSize = 0;
	DWORD dwBytesNeeded = 0;

	TCHAR temp[6] = TEXT("GUEST"); // only use is for the line 279, intialised here beaucse of the GOTO

	schSvcManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (schSvcManager == NULL)
	{
		printf("OpenSCManager failed (%d)\n", GetLastError());
		return;
	}

	schService = OpenService(schSvcManager, szSvcName, READ_CONTROL | WRITE_DAC);
	if (schService == NULL)
	{
		printf("OpenService failed (%d)\n", GetLastError());
		CloseServiceHandle(schSvcManager);
		return;
	}

	// get security descriptor
	if (!QueryServiceObjectSecurity(schService, DACL_SECURITY_INFORMATION, &psd, 0, &dwBytesNeeded))
	{
		if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
		{
			dwSize = dwBytesNeeded;
			psd = (PSECURITY_INFORMATION)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwSize);
			if (psd == NULL)
			{
				printf("heap allocated failed\n");
				goto dacl_cleanup;
			}

			if (!QueryServiceObjectSecurity(schService, DACL_SECURITY_INFORMATION, psd, dwSize, &dwBytesNeeded))
			{
				printf("QueryServiceObjectSecurity failed (%d)\n", GetLastError());
				goto dacl_cleanup;
			}
		}
		else
		{
			printf("QueryServiceObjectSecurity failed (%d)\n", GetLastError());
			goto dacl_cleanup;
		}
	}
	// get the dacl
	if (!GetSecurityDescriptorDacl(psd, &bDaclPresent, &pacl, &bDaclDefaulted))
	{
		printf("GetSecurityDescriptorDacl failed (%d)\n", GetLastError());
		goto dacl_cleanup;
	}

	//build acces
	BuildExplicitAccessWithName(&ea, temp, SERVICE_START | SERVICE_STOP | READ_CONTROL | DELETE, SET_ACCESS, NO_INHERITANCE);

	dwError = SetEntriesInAcl(1, &ea, pacl, &pNewACL);
	if (dwError != ERROR_SUCCESS)
	{
		printf("SetEntriesInAcl failed (%d)\n", dwError);
		goto dacl_cleanup;
	}

	//intialised new security descriptor
	if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION))
	{
		printf("InitializeSecurityDescriptor failed (%d) \n", GetLastError());
		goto dacl_cleanup;
	}
	//set the new dacl in the security descriptor
	if (!SetSecurityDescriptorDacl(&sd, TRUE, pNewACL, FALSE))
	{
		printf("SetSecurityDescriptorDacl failed (%d)\n", GetLastError());
		goto dacl_cleanup;
	}

	// set the new DACL for the service object
	if (!SetServiceObjectSecurity(schService, DACL_SECURITY_INFORMATION, &sd))
	{
		printf("SetServiceObjectSecurity faield (%d)\n", GetLastError());
		goto dacl_cleanup;
	}
	else printf("Service DACL updated sucessfuly ! \n");

dacl_cleanup:
	CloseServiceHandle(schSvcManager);
	CloseServiceHandle(schService);

	if (pNewACL != NULL)
		LocalFree((HLOCAL)pNewACL);
	if (psd != NULL)
		HeapFree(GetProcessHeap(), 0, (LPVOID)psd);
}

// stop the service
VOID __stdcall DoStopSvc(void)
{
	SERVICE_STATUS_PROCESS ssp;
	DWORD dwStartTime = GetTickCount64();
	DWORD dwBytesNedded;
	DWORD dwTimeOut = 30000;
	DWORD dwWaitTime;

	schSvcManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (schSvcManager == NULL)
	{
		printf("OpenSCManager failed (%d)\n", GetLastError());
		return;
	}

	schService = OpenService(schSvcManager, szSvcName, SERVICE_STOP | SERVICE_QUERY_STATUS | SERVICE_ENUMERATE_DEPENDENTS);
	if (schService == NULL)
	{
		printf("OpenService failed (%d)\n", GetLastError());
		CloseServiceHandle(schSvcManager);
		return;
	}

	if (!QueryServiceStatusEx(schService, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssp, sizeof(SERVICE_STATUS_PROCESS), &dwBytesNedded))
	{
		printf("QueryServiceStatusEx failed (%d)\n", GetLastError());
		goto stop_cleanup;
	}

	if (ssp.dwCurrentState == SERVICE_STOPPED)
	{
		printf("servicei is already stopped. \n");
		goto stop_cleanup;
	}

	// if a stop is pending, wait for it
	while (ssp.dwCurrentState == SERVICE_STOP_PENDING)
	{
		printf("service stop pending ...");

		dwWaitTime = ssp.dwWaitHint / 10;
		if (dwWaitTime < 1000)
			dwWaitTime = 1000;
		else if (dwWaitTime > 10000)
			dwWaitTime = 10000;

		Sleep(dwWaitTime);

		if (!QueryServiceStatusEx(schService, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssp, sizeof(SERVICE_STATUS_PROCESS), &dwBytesNedded))
		{
			printf("QueryServiceStatusEx failed (%d)\n", GetLastError());
			goto stop_cleanup;
		}

		if (ssp.dwCurrentState == SERVICE_STOPPED)
		{
			printf("Service stop successfully !\n");
			goto stop_cleanup;
		}

		if (GetTickCount64() - dwStartTime > dwTimeOut)
		{
			printf("Service stopped time out.\n");
			goto stop_cleanup;
		}
	}

	//if the service is running, stop dependencies
	StopDependentService();

	// stop the service
	if (!ControlService(schService, SERVICE_CONTROL_STOP, (LPSERVICE_STATUS)&ssp))
	{
		printf("ControlService failed (%d)\n", GetLastError());
		goto stop_cleanup;
	}

	while (ssp.dwCurrentState != SERVICE_STOPPED)
	{
		Sleep(ssp.dwWaitHint);

		if (!QueryServiceStatusEx(schService, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssp, sizeof(SERVICE_STATUS_PROCESS), &dwBytesNedded))
		{
			printf("QueryServiceStatusEx failed (%d)\n", GetLastError());
			goto stop_cleanup;
		}

		if (ssp.dwCurrentState == SERVICE_STOPPED)break;

		if (GetTickCount64() - dwStartTime > dwTimeOut)
		{
			printf("Wait timed out.\n");
			goto stop_cleanup;
		}
	}
	printf("Service stop successfully !\n");

stop_cleanup:
	CloseServiceHandle(schService);
	CloseServiceHandle(schSvcManager);
}

BOOL __stdcall StopDependentService(void)
{
	DWORD i;
	DWORD dwBytesNeeded;
	DWORD dwCount;

	LPENUM_SERVICE_STATUS lpDependencies = NULL;
	ENUM_SERVICE_STATUS ess;
	SC_HANDLE hDepService;
	SERVICE_STATUS_PROCESS ssp;

	DWORD dwStartTime = GetTickCount64();
	DWORD dwTimeout = 30000;

	// get riquired buffer size
	if (EnumDependentServices(schService, SERVICE_ACTIVE, lpDependencies, 0, &dwBytesNeeded, &dwCount))
	{
		// there is no dependencies
		return TRUE;
	}
	else
	{
		if (GetLastError() == ERROR_MORE_DATA)
			return FALSE;//UNEXPECTED ERROR

		lpDependencies = (LPENUM_SERVICE_STATUS)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwBytesNeeded);

		if (!lpDependencies)
			return FALSE;

		__try {
			
			if (!EnumDependentServices(schService,SERVICE_ACTIVE,lpDependencies,dwBytesNeeded,&dwBytesNeeded,&dwCount))
				return FALSE;

			for (i = 0; i < dwCount; i++)
			{
				ess = *(lpDependencies + i);
				hDepService = OpenService(schSvcManager, ess.lpServiceName, SERVICE_STOP | SERVICE_QUERY_STATUS);
				if (!hDepService)
					return FALSE;

				__try {
					if (!ControlService(hDepService, SERVICE_CONTROL_STOP, (LPSERVICE_STATUS)&ssp))
						return FALSE;
					while (ssp.dwCurrentState != SERVICE_STOPPED)
					{
						Sleep(ssp.dwWaitHint);
						if (!QueryServiceStatusEx(hDepService, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssp, sizeof(LPSERVICE_STATUS_PROCESS), &dwBytesNeeded))
							return FALSE;
						if (ssp.dwCurrentState == SERVICE_STOPPED)
							break;

						if (GetTickCount64() - dwStartTime > dwTimeout)
							return FALSE;
					}
				}
				__finally {
					// always release service handle
					CloseServiceHandle(hDepService);
				}
			}
		}
		__finally {
			// free the enumerate
			HeapFree(GetProcessHeap(), 0, lpDependencies);
		}
	}
	return TRUE;
}

