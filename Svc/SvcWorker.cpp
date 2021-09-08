#include "SvcWorker.h"
#include <fstream>
#include <Windows.h>

std::ofstream logFile;
void InitializeServiceWorker()
{
	logFile.open("E:/log.txt");
}

unsigned long __stdcall RunServiceWoker(void* lpParam)
{
	logFile.write("initialise service...\n", 23);
	logFile.flush();
	int count = 0;
	do
	{
		Sleep(10000);
		if (g_isStopAsking)
			break;
		if (count >= 6)
		{
			logFile.write("every minute, one minute pass\n", 30);
			logFile.flush();
			count = 0;
		}
		count++;
	} while (!g_isStopAsking);
	g_isStopAsking = false;
	return ERROR_SUCCESS;
}

void deleteServiceWOrker()
{
	while (g_isStopAsking);
	logFile.close();
}
