#pragma once
#include "Common.h"
#include "MyFile.h"

class FileTypeDetector
{
public:
    static FileType Detect(HANDLE fileHandle);
    static std::wstring ToString(FileType type);
};
