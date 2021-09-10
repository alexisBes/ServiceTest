#include "SvcWorker.h"
#include <fstream>
#include <Windows.h>
#include <string>
#include <thread>

#define NBTHREAD 3

std::ofstream logFile;

int nbThreadFinish = 0;
bool needStop = false;

void InitializeServiceWorker()
{
	logFile.open("E:/log.txt");
}

unsigned long __stdcall RunServiceWoker(void* lpParam)
{
	logFile.write("initialise service...\n", 23);
	logFile.flush();
	for (int i = 0; i < NBTHREAD; i++)
	{
		std::thread test(TestMultiThread, i);
		test.detach();
	}
	

	return ERROR_SUCCESS;
}

void DeleteServiceWOrker()
{
	needStop = true;
	while (nbThreadFinish < NBTHREAD)
	{}
	logFile.close();
}

void TestMultiThread(int num)
{
	std::ofstream logFileThread;
	logFileThread.open("E:/log" + std::to_string(num) + ".txt");
	int count = 0;
	do
	{
		Sleep(1000);
		if (needStop)
			break;
		if (count >= 60)
		{
			logFileThread.write("every minute, one minute pass\n", 30);
			logFileThread.flush();
			count = 0;
		}
		count++;
	} while (!needStop);
	logFileThread.close();
	nbThreadFinish++;
}
