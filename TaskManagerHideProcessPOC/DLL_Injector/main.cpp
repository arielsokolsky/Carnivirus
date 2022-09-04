#include <Windows.h>
#include <iostream>

#include <process.h>
#include <Tlhelp32.h>
#include <winbase.h>
#include <string.h>
#include <comdef.h>

#define DLL_NAME L"HideProcessHookDLL.dll"
#define INJECT_INTO "Taskmgr.exe"

HANDLE findProcess(const char* name);

int main()
{
	// Get full path of DLL to inject
	wchar_t dllPath[MAX_PATH] = { 0 };
	DWORD pathLen = GetFullPathNameW(DLL_NAME, MAX_PATH, dllPath, NULL);
	// Get LoadLibrary function address �
	// the address doesn't change at remote process
	PVOID addrLoadLibrary = (PVOID)GetProcAddress(GetModuleHandleA("Kernel32.dll"), "LoadLibraryW");
	if (addrLoadLibrary == NULL)
	{
		std::cout << "Error getting dll function address: " << GetLastError();
		return 0;
	}

	// Open remote process
	HANDLE process = findProcess(INJECT_INTO);
	if (process == NULL)
	{
		std::cout << "process wasn't found";
		return 0;
	}
	// Get a pointer to memory location in remote process,
	// big enough to store DLL path
	PVOID memAddr = (PVOID)VirtualAllocEx(process, NULL, pathLen * sizeof(wchar_t), MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if (NULL == memAddr)
	{
		std::cout << "Error allocating memory: " << GetLastError();
		return 0;
	}
	// Write DLL name to remote process memory
	BOOL check = WriteProcessMemory(process, memAddr, dllPath, pathLen * sizeof(wchar_t), NULL);
	if (0 == check)
	{
		std::cout << "Error writing to memory: " << GetLastError();
		return 0;
	}
	// Open remote thread, while executing LoadLibrary
	// with parameter DLL name, will trigger DLLMain
	HANDLE hRemote = CreateRemoteThread(process, NULL, 0, (LPTHREAD_START_ROUTINE)addrLoadLibrary, memAddr, 0, NULL);
	if (NULL == hRemote)
	{
		std::cout << "Error creating thread: " << GetLastError();
		return 0;
	}

	WaitForSingleObject(hRemote, INFINITE);
	CloseHandle(hRemote);
	CloseHandle(process);
	return 0;
}

// function finds process by it's name and returns a handle
HANDLE findProcess(const char* name)
{
	HANDLE hProcess = NULL;
	int retValue = 0;
	// snapshot for all running processes
	HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPALL, NULL);
	PROCESSENTRY32 pEntry;
	// initializing size - needed for using Process32First
	pEntry.dwSize = sizeof(pEntry);
	BOOL hRes = Process32First(hSnapShot, &pEntry);
	// while first process in pEntry exists and process wasn't found yet
	while (hRes && hProcess == NULL)
	{
		// create const char for string comparison
		_bstr_t b(pEntry.szExeFile);
		if (strcmp(b, name) == 0)
		{
			// get handle
			hProcess = OpenProcess(PROCESS_ALL_ACCESS, 0, (DWORD)pEntry.th32ProcessID);
			std::cout << hProcess << std::endl;
		}
		// go to next process
		hRes = Process32Next(hSnapShot, &pEntry);
	}
	CloseHandle(hSnapShot);

	return hProcess;
}
