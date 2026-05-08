#pragma once
#include "Common.h"
#include "FileRepository.h"
#include "MyFile.h"

class Executor
{
public:
    static void Run(DWORD pid, HANDLE processHandle, const FileRepository& repository, const MY_FILE* files, bool useReflect, HWND ownerWindow);

private:
    static void HandleExecutableOrLibrary(HANDLE processHandle, const uint8_t* payload, size_t payloadSize, std::wstring& message);
    static void HandleOtherFile(HANDLE processHandle, const uint8_t* payload, size_t payloadSize, std::wstring& message);
};
