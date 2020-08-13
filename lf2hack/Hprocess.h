#pragma once

#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>
#include <psapi.h>

//THIS FILE SIMPLY DOES MOST OF THE BACKEND WORK FOR US,
//FROM FINDING THE PROCESS TO SETTING UP CORRECT ACCESS FOR US
//TO EDIT MEMORY
//IN MOST GAMES, A SIMPLER VERSION OF THIS CAN BE USED, or if you're injecting then its often not necessary
//This file has been online for quite a while so credits should be shared but im using this from NubTIK
//So Credits to him and thanks

using namespace std;

class CHackProcess
{
public:

	PROCESSENTRY32 __gameProcess;
	HANDLE __HandleProcess;
	HWND __HWNDCss;
	DWORD __dwordClient;
	DWORD __dwordEngine;
	DWORD __dwordOverlay;
	DWORD __dwordVGui;
	DWORD __dwordLibCef;
	DWORD __dwordSteam;
	DWORD FindProcessName(const char *__ProcessName, PROCESSENTRY32 *pEntry)
	{
		PROCESSENTRY32 __ProcessEntry;
		__ProcessEntry.dwSize = sizeof(PROCESSENTRY32);
		HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (hSnapshot == INVALID_HANDLE_VALUE) return 0;        if (!Process32First(hSnapshot, &__ProcessEntry))
		{
			CloseHandle(hSnapshot);
			return 0;
		}
		const size_t cSize = strlen(__ProcessName) + 1;
		wchar_t* wc = new wchar_t[cSize];
		mbstowcs(wc, __ProcessName, cSize);
		do {
			
			if (!wcscmp(__ProcessEntry.szExeFile, wc))
			{
				memcpy((void *)pEntry, (void *)&__ProcessEntry, sizeof(PROCESSENTRY32));
				CloseHandle(hSnapshot);
				return __ProcessEntry.th32ProcessID;
			}
		} while (Process32Next(hSnapshot, &__ProcessEntry));
		CloseHandle(hSnapshot);
		return 0;
	}


	DWORD getThreadByProcess(DWORD __DwordProcess)
	{
		THREADENTRY32 __ThreadEntry;
		__ThreadEntry.dwSize = sizeof(THREADENTRY32);
		HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
		if (hSnapshot == INVALID_HANDLE_VALUE) return 0;

		if (!Thread32First(hSnapshot, &__ThreadEntry)) { CloseHandle(hSnapshot); return 0; }

		do {
			if (__ThreadEntry.th32OwnerProcessID == __DwordProcess)
			{
				CloseHandle(hSnapshot);
				return __ThreadEntry.th32ThreadID;
			}
		} while (Thread32Next(hSnapshot, &__ThreadEntry));
		CloseHandle(hSnapshot);
		return 0;
	}

	DWORD GetModuleNamePointer(LPSTR LPSTRModuleName, DWORD __DwordProcessId)
	{
		MODULEENTRY32 lpModuleEntry = { 0 };
		HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, __DwordProcessId);
		if (!hSnapShot)
			return NULL;
		lpModuleEntry.dwSize = sizeof(lpModuleEntry);
		BOOL __RunModule = Module32First(hSnapShot, &lpModuleEntry);
		const size_t cSize = strlen(LPSTRModuleName) + 1;
		wchar_t* wc = new wchar_t[cSize];
		mbstowcs(wc, LPSTRModuleName, cSize);
		while (__RunModule)
		{
			if (!wcscmp(lpModuleEntry.szModule, wc))
			{
				CloseHandle(hSnapShot);
				return (DWORD)lpModuleEntry.modBaseAddr;
			}
			__RunModule = Module32Next(hSnapShot, &lpModuleEntry);
		}
		CloseHandle(hSnapShot);
		return NULL;
	}


	void runSetDebugPrivs()
	{
		HANDLE __HandleProcess = GetCurrentProcess(), __HandleToken;
		TOKEN_PRIVILEGES priv;
		LUID __LUID;
		const size_t cSize = strlen("seDebugPrivilege") + 1;
		wchar_t* wc = new wchar_t[cSize];
		mbstowcs(wc, "seDebugPrivilege", cSize);
		OpenProcessToken(__HandleProcess, TOKEN_ADJUST_PRIVILEGES, &__HandleToken);
		LookupPrivilegeValue(0, const_cast <LPCWCHAR> (wc), &__LUID);
		priv.PrivilegeCount = 1;
		priv.Privileges[0].Luid = __LUID;
		priv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
		AdjustTokenPrivileges(__HandleToken, false, &priv, 0, 0, 0);
		CloseHandle(__HandleToken);
		CloseHandle(__HandleProcess);
	}

	int PrintModules(DWORD processID)
	{
		HMODULE hMods[1024];
		HANDLE hProcess;
		DWORD cbNeeded;
		unsigned int i;

		// Print the process identifier.

		printf("\nProcess ID: %u\n", processID);

		// Get a handle to the process.

		hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processID);
		if (NULL == hProcess) 
		{
			return 1;
		}
		/*else if (GetLastError() == ERROR_ACCESS_DENIED)
		{
			std::cout << "Can't do that." << std::endl;
		}
		else
		{
			DWORD lastError = GetLastError();
			//You have to cache the value of the last error here, because the call to
			//operator<<(std::ostream&, const char *) may cause the last error to be set
			//to something else.
			std::cout << "General failure. GetLastError returned " << std::hex
				<< lastError << ".";
		}*/
		// Get a list of all the modules in this process.

		if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded))
		{
			for (i = 0; i < (cbNeeded / sizeof(HMODULE)); i++)
			{
				TCHAR szModName[MAX_PATH];

				// Get the full path to the module's file.

				if (GetModuleFileNameEx(hProcess, hMods[i], szModName,
					sizeof(szModName) / sizeof(TCHAR)))
				{
					// Print the module name and handle value.

					_tprintf(TEXT("\t%s (0x%08X)\n"), szModName, hMods[i]);
				}
			}
		}

		// Release the handle to the process.

		CloseHandle(hProcess);

		return 0;
	}

	void RunProcess(const char *ProcessName,LPCWSTR WindowName)
	{
		//commented lines are for non steam versions of the game
		runSetDebugPrivs();
		while (!FindProcessName(ProcessName, &__gameProcess)) Sleep(12);
		while (!(getThreadByProcess(__gameProcess.th32ProcessID))) Sleep(12);
		__HandleProcess = OpenProcess(PROCESS_ALL_ACCESS, false, __gameProcess.th32ProcessID);
		//while (__dwordClient == 0x0) __dwordClient = GetModuleNamePointer((LPSTR)"client.dll", __gameProcess.th32ProcessID);
		////  while (__dwordEngine == 0x0) __dwordEngine = GetModuleNamePointer((LPSTR)"engine.dll", __gameProcess.th32ProcessID);
		//while(__dwordOverlay == 0x0) __dwordOverlay = GetModuleNamePointer((LPSTR)"gameoverlayrenderer.dll", __gameProcess.th32ProcessID);
		////  while (__dwordVGui == 0x0) __dwordVGui = GetModuleNamePointer((LPSTR)"vguimatsurface.dll", __gameProcess.th32ProcessID);
		//while(__dwordLibCef == 0x0) __dwordLibCef = GetModuleNamePointer((LPSTR)"libcef.dll", __gameProcess.th32ProcessID);
		////  while(__dwordSteam == 0x0) __dwordSteam = GetModuleNamePointer("steam.dll", __gameProcess.th32ProcessID);
		//__HWNDCss = FindWindow(NULL, WindowName);
		__HWNDCss = ::FindWindow(0, _T("Counter-Strike: Global Offensive"));
	}
};