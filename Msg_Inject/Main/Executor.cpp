#include "Executor.h"
#include "FileTypeDetector.h"
#include "PEReflact.h"
#include "ProcessInject.h"

void Executor::Run(DWORD pid, HANDLE processHandle, const FileRepository& repository, const MY_FILE* files, bool useReflect, HWND ownerWindow)
{
    if (pid == 0 || !processHandle)
    {
        MessageBoxW(ownerWindow, L"Please select a process first.", L"Info", MB_OK | MB_ICONINFORMATION);
        return;
    }

    if (!files)
    {
        MessageBoxW(ownerWindow, L"Please add at least one file.", L"Info", MB_OK | MB_ICONINFORMATION);
        return;
    }

    int totalCount = 0;
    int peCount = 0;
    int otherCount = 0;

    std::wstringstream summary;
    summary << L"Executor started.\r\n";
    summary << L"Reflect mode = " << (useReflect ? L"ON" : L"OFF") << L"\r\n";
    summary << L"PID = " << pid << L"\r\n";
    summary << L"Process Handle = " << reinterpret_cast<UINT_PTR>(processHandle) << L"\r\n\r\n";

    for (const MY_FILE* current = files; current != nullptr; current = current->next.get())
    {
        ++totalCount;

        std::unique_ptr<uint8_t[]> rawBuffer;
        size_t rawSize = 0;
        std::wstring buildMessage;
        if (!repository.ReadFileContent(*current, rawBuffer, rawSize, buildMessage))
        {
            summary << L"[FAIL] " << current->path << L"\r\n" << buildMessage << L"\r\n\r\n";
            continue;
        }

        std::unique_ptr<uint8_t[]> payloadBuffer;
        size_t payloadSize = 0;
        bool payloadReady = useReflect
            ? PEReflact::BuildReflectPayload(rawBuffer.get(), rawSize, payloadBuffer, payloadSize, buildMessage)
            : PEReflact::BuildRawPayload(rawBuffer.get(), rawSize, payloadBuffer, payloadSize, buildMessage);

        if (!payloadReady)
        {
            summary << L"[FAIL] " << current->path << L"\r\n" << buildMessage << L"\r\n\r\n";
            continue;
        }

        std::wstring branchMessage;
        if (current->type == FileType::Exe || current->type == FileType::Dll)
        {
            ++peCount;
            HandleExecutableOrLibrary(processHandle, payloadBuffer.get(), payloadSize, branchMessage);
            summary << L"[PE] ";
        }
        else
        {
            ++otherCount;
            HandleOtherFile(processHandle, payloadBuffer.get(), payloadSize, branchMessage);
            summary << L"[OTHER] ";
        }

        std::wstring runMessage;
        const bool runOk = ProcessInject::Run(pid, payloadBuffer.get(), payloadSize, useReflect, runMessage);

        summary << current->path
                << L"\r\nType = " << FileTypeDetector::ToString(current->type)
                << L"\r\nPayload Ptr = " << reinterpret_cast<UINT_PTR>(payloadBuffer.get())
                << L"\r\nPayload Size = 0x" << std::hex << payloadSize << std::dec
                << L"\r\nBuild = " << buildMessage
                << L"\r\nBranch = " << branchMessage
                << L"\r\nRun = " << (runOk ? L"OK" : L"FAIL")
                << L"\r\n" << runMessage
                << L"\r\n\r\n";
    }

    summary << L"Total = " << totalCount
            << L"\r\nPE = " << peCount
            << L"\r\nOther = " << otherCount;

    MessageBoxW(ownerWindow, summary.str().c_str(), L"Run Summary", MB_OK | MB_ICONINFORMATION);
}

void Executor::HandleExecutableOrLibrary(HANDLE processHandle, const uint8_t* payload, size_t payloadSize, std::wstring& message)
{
    UNREFERENCED_PARAMETER(processHandle);

    std::wstringstream ss;
    ss << L"PE branch ready"
       << L"; ptr=" << reinterpret_cast<UINT_PTR>(payload)
       << L"; size=0x" << std::hex << payloadSize;
    message = ss.str();
}

void Executor::HandleOtherFile(HANDLE processHandle, const uint8_t* payload, size_t payloadSize, std::wstring& message)
{
    UNREFERENCED_PARAMETER(processHandle);

    std::wstringstream ss;
    ss << L"OTHER branch ready"
       << L"; ptr=" << reinterpret_cast<UINT_PTR>(payload)
       << L"; size=0x" << std::hex << payloadSize;
    message = ss.str();
}
