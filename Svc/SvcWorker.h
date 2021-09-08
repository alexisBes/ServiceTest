#pragma once

static bool g_isStopAsking = false;

void InitializeServiceWorker();

unsigned long __stdcall RunServiceWoker(void* lpParam);

void deleteServiceWOrker();