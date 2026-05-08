#include "FileTypeDetector.h"

FileType FileTypeDetector::Detect(HANDLE fileHandle)
{
    if (fileHandle == nullptr || fileHandle == INVALID_HANDLE_VALUE)
    {
        return FileType::Other;
    }

    LARGE_INTEGER zero{};
    if (!SetFilePointerEx(fileHandle, zero, nullptr, FILE_BEGIN))
    {
        return FileType::Other;
    }

    IMAGE_DOS_HEADER dosHeader{};
    DWORD bytesRead = 0;
    if (!ReadFile(fileHandle, &dosHeader, sizeof(dosHeader), &bytesRead, nullptr) ||
        bytesRead != sizeof(dosHeader) ||
        dosHeader.e_magic != IMAGE_DOS_SIGNATURE)
    {
        SetFilePointerEx(fileHandle, zero, nullptr, FILE_BEGIN);
        return FileType::Other;
    }

    LARGE_INTEGER ntOffset{};
    ntOffset.QuadPart = dosHeader.e_lfanew;
    if (!SetFilePointerEx(fileHandle, ntOffset, nullptr, FILE_BEGIN))
    {
        SetFilePointerEx(fileHandle, zero, nullptr, FILE_BEGIN);
        return FileType::Other;
    }

    DWORD ntSignature = 0;
    if (!ReadFile(fileHandle, &ntSignature, sizeof(ntSignature), &bytesRead, nullptr) ||
        bytesRead != sizeof(ntSignature) ||
        ntSignature != IMAGE_NT_SIGNATURE)
    {
        SetFilePointerEx(fileHandle, zero, nullptr, FILE_BEGIN);
        return FileType::Other;
    }

    IMAGE_FILE_HEADER fileHeader{};
    if (!ReadFile(fileHandle, &fileHeader, sizeof(fileHeader), &bytesRead, nullptr) ||
        bytesRead != sizeof(fileHeader))
    {
        SetFilePointerEx(fileHandle, zero, nullptr, FILE_BEGIN);
        return FileType::Other;
    }

    SetFilePointerEx(fileHandle, zero, nullptr, FILE_BEGIN);

    if ((fileHeader.Characteristics & IMAGE_FILE_DLL) != 0)
    {
        return FileType::Dll;
    }

    return FileType::Exe;
}

std::wstring FileTypeDetector::ToString(FileType type)
{
    switch (type)
    {
    case FileType::Exe:
        return L"EXE";
    case FileType::Dll:
        return L"DLL";
    default:
        return L"OTHER";
    }
}
