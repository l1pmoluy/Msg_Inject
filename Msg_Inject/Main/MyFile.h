#pragma once
#include "Common.h"
#include "FileType.h"

struct MY_FILE
{
    HANDLE fileHandle = INVALID_HANDLE_VALUE;
    std::wstring path;
    FileType type = FileType::Other;
    bool selected = false;
    std::unique_ptr<MY_FILE> next;
};
