#include "FileRepository.h"
#include "FileTypeDetector.h"

FileRepository::~FileRepository()
{
    Clear();
}

bool FileRepository::AddFile(const std::wstring& path, std::wstring& message)
{
    if (ContainsPath(path))
    {
        message = L"文件已经在链表中：" + path;
        return false;
    }

    HANDLE fileHandle = CreateFileW(
        path.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);

    if (fileHandle == INVALID_HANDLE_VALUE)
    {
        std::wstringstream ss;
        ss << L"CreateFile 失败：" << path << L"，错误码=" << GetLastError();
        message = ss.str();
        return false;
    }

    auto node = std::make_unique<MY_FILE>();
    node->fileHandle = fileHandle;
    node->path = path;
    node->type = FileTypeDetector::Detect(fileHandle);

    MY_FILE* rawNode = node.get();
    if (!m_head)
    {
        m_head = std::move(node);
        m_tail = m_head.get();
    }
    else
    {
        m_tail->next = std::move(node);
        m_tail = rawNode;
    }

    ++m_count;

    std::wstringstream ss;
    ss << L"已加入文件链表：" << rawNode->path
       << L" | Handle=" << rawNode->fileHandle
       << L" | Type=" << FileTypeDetector::ToString(rawNode->type);
    message = ss.str();
    return true;
}

void FileRepository::Clear()
{
    MY_FILE* current = m_head.get();
    while (current)
    {
        if (current->fileHandle != nullptr && current->fileHandle != INVALID_HANDLE_VALUE)
        {
            CloseHandle(current->fileHandle);
            current->fileHandle = INVALID_HANDLE_VALUE;
        }
        current = current->next.get();
    }

    m_head.reset();
    m_tail = nullptr;
    m_count = 0;
}

bool FileRepository::ReadFileContent(const MY_FILE& file, std::unique_ptr<uint8_t[]>& buffer, size_t& size, std::wstring& message) const
{
    buffer.reset();
    size = 0;

    HANDLE fileHandle = CreateFileW(
        file.path.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);

    if (fileHandle == INVALID_HANDLE_VALUE)
    {
        std::wstringstream ss;
        ss << L"重新打开文件失败：" << file.path << L"，错误码=" << GetLastError();
        message = ss.str();
        return false;
    }

    LARGE_INTEGER fileSize{};
    if (!GetFileSizeEx(fileHandle, &fileSize) || fileSize.QuadPart <= 0)
    {
        CloseHandle(fileHandle);
        message = L"文件大小无效：" + file.path;
        return false;
    }

    size = static_cast<size_t>(fileSize.QuadPart);
    buffer = std::make_unique<uint8_t[]>(size);

    DWORD bytesRead = 0;
    if (!ReadFile(fileHandle, buffer.get(), static_cast<DWORD>(size), &bytesRead, nullptr) ||
        bytesRead != static_cast<DWORD>(size))
    {
        CloseHandle(fileHandle);
        buffer.reset();
        size = 0;
        std::wstringstream ss;
        ss << L"读取文件失败：" << file.path << L"，错误码=" << GetLastError();
        message = ss.str();
        return false;
    }

    CloseHandle(fileHandle);
    message = L"读取文件成功：" + file.path;
    return true;
}

std::vector<FileRepository::DisplayItem> FileRepository::ToDisplayItems() const
{
    std::vector<DisplayItem> lines;
    const MY_FILE* current = m_head.get();
    while (current)
    {
        std::wstringstream ss;
        ss << current->path
           << L" | Type=" << FileTypeDetector::ToString(current->type)
           << L" | Handle=" << current->fileHandle;
        lines.push_back({ ss.str(), current->selected });
        current = current->next.get();
    }

    return lines;
}

bool FileRepository::ToggleSelectedAt(size_t index)
{
    MY_FILE* node = GetAt(index);
    if (!node)
    {
        return false;
    }

    node->selected = !node->selected;
    return true;
}

bool FileRepository::SetSelectedAt(size_t index, bool selected)
{
    MY_FILE* node = GetAt(index);
    if (!node)
    {
        return false;
    }

    node->selected = selected;
    return true;
}

size_t FileRepository::RemoveSelected()
{
    size_t removed = 0;
    std::unique_ptr<MY_FILE>* currentLink = &m_head;
    m_tail = nullptr;

    while (*currentLink)
    {
        if ((*currentLink)->selected)
        {
            if ((*currentLink)->fileHandle != nullptr && (*currentLink)->fileHandle != INVALID_HANDLE_VALUE)
            {
                CloseHandle((*currentLink)->fileHandle);
                (*currentLink)->fileHandle = INVALID_HANDLE_VALUE;
            }

            *currentLink = std::move((*currentLink)->next);
            ++removed;
            --m_count;
            continue;
        }

        m_tail = currentLink->get();
        currentLink = &((*currentLink)->next);
    }

    return removed;
}

bool FileRepository::ContainsPath(const std::wstring& path) const
{
    const MY_FILE* current = m_head.get();
    while (current)
    {
        if (_wcsicmp(current->path.c_str(), path.c_str()) == 0)
        {
            return true;
        }
        current = current->next.get();
    }

    return false;
}

MY_FILE* FileRepository::GetAt(size_t index) const
{
    MY_FILE* current = m_head.get();
    size_t currentIndex = 0;
    while (current)
    {
        if (currentIndex == index)
        {
            return current;
        }

        current = current->next.get();
        ++currentIndex;
    }

    return nullptr;
}
