#pragma once
#include "Common.h"

#define ROL4(value, count) (((value) << (count)) | ((value) >> (32 - (count))))

extern "C" {
    __declspec(selectany) char __mycode_begin = 0;
}

#pragma comment(linker, "/SECTION:.mycode,ERW")

namespace PEReflact
{
    bool BuildReflectPayload(
        const uint8_t* fileBuffer,
        size_t fileSize,
        std::unique_ptr<uint8_t[]>& outputBuffer,
        size_t& outputSize,
        std::wstring& message);

    bool BuildRawPayload(
        const uint8_t* fileBuffer,
        size_t fileSize,
        std::unique_ptr<uint8_t[]>& outputBuffer,
        size_t& outputSize,
        std::wstring& message);
}

HMODULE FindModuleByPEB(unsigned int moduleHash);
FARPROC FindFuncByMODULE(HMODULE hModule, unsigned int funcHash);
int Loader(char* fileAddress);
void CallLoader();
