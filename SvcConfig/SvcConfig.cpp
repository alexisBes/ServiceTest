#include <windows.h>
#include <tchar.h>
#include <strsafe.h>
#include <stdio.h>

#pragma comment(lib,"advapi32.lib")

TCHAR szCommand[10];
TCHAR szSvcName[80];

VOID __stdcall DisplayUsage(void);

VOID __stdcall DoQuerySvc(void);
VOID __stdcall DoUpdatSvcDesc(void);
VOID __stdcall DoDisableSvc(void);
VOID __stdcall DoEnableSvc(void);
VOID __stdcall DoDeleteSvc(void);

int __cdecl _tmain(int argc, TCHAR* argv[])
{
	printf("\n");
	if (argc != 3)
	{
		printf("ERROR :\t incorrect number of argment \n\n");
		DisplayUsage();
		return -1;
	}

	StringCchCopy(szCommand, 10, argv[1]);
	StringCchCopy(szSvcName, 80, argv[2]);

	if (lstrcmpi(szCommand, TEXT("query")) == 0)
		DoQuerySvc();
	else if (lstrcmpi(szCommand, TEXT("describe")) == 0)
		DoUpdatSvcDesc();
	else if (lstrcmpi(szCommand, TEXT("disable")) == 0)
		DoDisableSvc();
	else if (lstrcmpi(szCommand, TEXT("enable")) == 0)
		DoEnableSvc();
	else if (lstrcmpi(szCommand, TEXT("delete")) == 0)
		DoDeleteSvc();
	else
	{
		_tprintf(TEXT("Unknown command(% s)\n\n"), szCommand);
		DisplayUsage();
	}
}

VOID __stdcall DisplayUsage(void)
{
	printf("Description:\n");
	printf("\tCommand-line tool that configures a service.\n\n");
	printf("Usage:\n");
	printf("\tsvcconfig [command] [service_name]\n\n");
	printf("\t[command]\n");
	printf("\t  query\n");
	printf("\t  describe\n");
	printf("\t  disable\n");
	printf("\t  enable\n");
	printf("\t  delete\n");
}

VOID __stdcall DoQuerySvc(void)
{
	SC_HANDLE schSCManager;
	SC_HANDLE schService;
	LPQUERY_SERVICE_CONFIG lpsc = NULL;
	LPSERVICE_DESCRIPTION lpsd = NULL;
	DWORD dwBytesNeeded, cbBufSize = 0, dwError;

	schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (schSCManager == NULL)
	{
		printf("OpenSCManager failed (%d)\n", GetLastError());
		return;
	}

	schService = OpenService(schSCManager, szSvcName, SERVICE_QUERY_CONFIG);
	if (schService == NULL)
	{
		printf("OpenService failed (%d)\n", GetLastError());
		CloseServiceHandle(schSCManager);
		return;
	}

	// Get the configuration information.
	if (!QueryServiceConfig(schService, NULL, 0, &dwBytesNeeded))
	{
		dwError = GetLastError();
		if (ERROR_INSUFFICIENT_BUFFER == dwError)
		{
			cbBufSize = dwBytesNeeded;
			lpsc = (LPQUERY_SERVICE_CONFIG)LocalAlloc(LMEM_FIXED, cbBufSize);
		}
		else
		{
			printf("QueryServiceConfig failed (%d)\n", dwError);
			goto cleanup;
			return;
		}
	}

	if (!QueryServiceConfig(schService, lpsc, cbBufSize, &dwBytesNeeded))
	{
		printf("QueryServiceConfig failed (%d)\n", GetLastError());
		goto cleanup;
	}

	if (!QueryServiceConfig2(schService, SERVICE_CONFIG_DESCRIPTION, NULL, 0, &dwBytesNeeded))
	{
		dwError = GetLastError();
		if (dwError == ERROR_INSUFFICIENT_BUFFER)
		{
			cbBufSize = dwBytesNeeded;
			lpsd = (LPSERVICE_DESCRIPTION)LocalAlloc(LMEM_FIXED, cbBufSize);
		}
		else
		{
			printf("QueryServiceConfig2 failed (%d)\n", dwError);
			goto cleanup;
		}
	}

	if (!QueryServiceConfig2(schService, SERVICE_CONFIG_DESCRIPTION, (LPBYTE)lpsd, cbBufSize, &dwBytesNeeded))
	{
		printf("QueryServiceConfig2 failed (%d)\n", GetLastError());
		goto cleanup;
	}

	_tprintf(TEXT("%s configuration : \n"), szSvcName);
	_tprintf(TEXT("  Type: 0x%x \n"), lpsc->dwServiceType);
	_tprintf(TEXT("  Start Type : 0x%x\n"), lpsc->dwStartType);
	_tprintf(TEXT("  Error Control : 0x%x \n"), lpsc->dwErrorControl);
	_tprintf(TEXT("  Binary Path: %s\n"), lpsc->lpBinaryPathName);
	_tprintf(TEXT("  Account : %s\n"), lpsc->lpServiceStartName);

	if (lpsd->lpDescription != NULL && lstrcmp(lpsd->lpDescription, TEXT("")) != 0)
		_tprintf(TEXT("  Description: %s\n"), lpsd->lpDescription);
	if (lpsc->lpLoadOrderGroup != NULL && lstrcmp(lpsc->lpLoadOrderGroup, TEXT("")) != 0)
		_tprintf(TEXT("  Load Order Group: %s\n"), lpsc->lpLoadOrderGroup);
	if (lpsc->dwTagId != 0)
		_tprintf(TEXT("  Tag ID: %d\n"), lpsc->dwTagId);
	if (lpsc->lpDependencies != NULL && lstrcmp(lpsc->lpDependencies, TEXT("")) != 0)
		_tprintf(TEXT("  Dependencies: %s\n"), lpsc->lpDependencies);

	LocalFree(lpsc);
	LocalFree(lpsd);

cleanup:
	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);
}

VOID __stdcall DoUpdatSvcDesc(void)
{
	SC_HANDLE schSCManager;
	SC_HANDLE schService;
	SERVICE_DESCRIPTION sd;
	TCHAR szDesc[100] = TEXT("Test description example");

	schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

	if (schSCManager == NULL)
	{
		printf("OpenSCManager failed (%d)\n", GetLastError());
		return;
	}

	schService = OpenService(schSCManager, szSvcName, SERVICE_CHANGE_CONFIG);
	if (schService == NULL)
	{
		printf("Open service failed (%d)\n", GetLastError());
		CloseServiceHandle(schSCManager);
		return;
	}

	sd.lpDescription = szDesc;

	// Change the service start type.
	if (!ChangeServiceConfig2(schService, SERVICE_CONFIG_DESCRIPTION, &sd))
	{
		printf("ChangeServiceConfig2 failed (%d)\n", GetLastError());
	}
	else printf("Service description updated successfully.\n");

	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);
}

VOID __stdcall DoDisableSvc(void)
{
	SC_HANDLE schSCManager;
	SC_HANDLE schService;

	schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

	if (schSCManager == NULL)
	{
		printf("OpenSCManager failed (%d)\n", GetLastError());
		return;
	}

	schService = OpenService(schSCManager, szSvcName, SERVICE_CHANGE_CONFIG);
	if (schService == NULL)
	{
		printf("Open service failed (%d)\n", GetLastError());
		CloseServiceHandle(schSCManager);
		return;
	}

	// Change the service start type.
	if (!ChangeServiceConfig(schService, SERVICE_NO_CHANGE, SERVICE_DISABLED, SERVICE_NO_CHANGE,
		NULL, NULL, NULL, NULL, NULL, NULL, NULL))
	{
		printf("ChangeServiceConfig failed (%d)\n", GetLastError());
	}
	else printf("Service Disabled successfully\n");

	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);
}

VOID __stdcall DoEnableSvc(void)
{
	SC_HANDLE schSCManager;
	SC_HANDLE schService;

	schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (schSCManager == NULL)
	{
		printf("OpenSCManager failed (%d)\n", GetLastError());
		return;
	}

	schService = OpenService(schSCManager, szSvcName, SERVICE_CHANGE_CONFIG);
	if (schService == NULL)
	{
		printf("Open service failed (%d)\n", GetLastError());
		CloseServiceHandle(schSCManager);
		return;
	}

	if (!ChangeServiceConfig(schService, SERVICE_NO_CHANGE, SERVICE_DEMAND_START, SERVICE_NO_CHANGE,
		NULL, NULL, NULL, NULL, NULL, NULL, NULL))
	{
		printf("ChangeServiceConfig failed (%d)\n", GetLastError());
	}
	else printf("Service Enabled successfully\n");

	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);
}

VOID __stdcall DoDeleteSvc(void)
{
	SC_HANDLE schSCManager;
	SC_HANDLE schService;
	SERVICE_STATUS ssStatus;

	schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (schSCManager == NULL)
	{
		printf("OpenSCManager failed (%d)\n", GetLastError());
		return;
	}

	schService = OpenService(schSCManager, szSvcName, DELETE);
	if (schService == NULL)
	{
		printf("Open service failed (%d)\n", GetLastError());
		CloseServiceHandle(schSCManager);
		return;
	}

	if (!DeleteService(schService))
	{
		printf("DeleteService failed (%d)\n", GetLastError());
	}
	else printf("Service delete successfully\n");

	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);
}
