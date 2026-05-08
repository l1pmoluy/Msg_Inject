#pragma once
#include "Common.h"
#include <Windows.h> 
#include <iostream>

typedef  NTSTATUS(NTAPI* pfnNtMapViewOfSection) (
    HANDLE SectionHandle,
    HANDLE ProcessHandle,
    PVOID* BaseAddress,
    ULONG_PTR ZeroBits,
    SIZE_T CommitSize,
    PLARGE_INTEGER SectionOffset,
    PSIZE_T ViewSize,
    DWORD InheritDisposition,
    ULONG AllocationType,
    ULONG Win32Protect
    );

class ProcessInject
{
public:
    static bool Run(DWORD pid, const uint8_t* payload, size_t payloadSize, bool isReflact, std::wstring& message);
};
