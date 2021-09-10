#pragma once

void InitializeServiceWorker();

unsigned long __stdcall RunServiceWoker(void* lpParam);

void DeleteServiceWOrker();

static void TestMultiThread(int num);