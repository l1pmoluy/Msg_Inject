#pragma once
#include "Common.h"
#include "MyFile.h"

class FileRepository
{
public:
    struct DisplayItem
    {
        std::wstring line;
        bool selected = false;
    };

    FileRepository() = default;
    ~FileRepository();

    bool AddFile(const std::wstring& path, std::wstring& message);
    void Clear();
    bool ReadFileContent(const MY_FILE& file, std::unique_ptr<uint8_t[]>& buffer, size_t& size, std::wstring& message) const;

    MY_FILE* Head() const { return m_head.get(); }
    size_t Count() const { return m_count; }
    std::vector<DisplayItem> ToDisplayItems() const;
    bool ToggleSelectedAt(size_t index);
    bool SetSelectedAt(size_t index, bool selected);
    size_t RemoveSelected();

private:
    MY_FILE* GetAt(size_t index) const;
    bool ContainsPath(const std::wstring& path) const;

    std::unique_ptr<MY_FILE> m_head;
    MY_FILE* m_tail = nullptr;
    size_t m_count = 0;
};
