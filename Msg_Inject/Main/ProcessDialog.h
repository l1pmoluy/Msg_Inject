#pragma once
#include "Common.h"

class ProcessDialog
{
public:
    explicit ProcessDialog(HWND owner);
    bool Show(HANDLE& selectedHandle, std::wstring& selectedName, DWORD& selectedPid);

private:
    struct ProcessItem
    {
        DWORD pid = 0;
        std::wstring exeName;
        std::wstring exePath;
    };

    enum class FilterMode
    {
        AllProcesses,
        WindowApps
    };

    static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    static BOOL CALLBACK EnumWindowProc(HWND hWnd, LPARAM lParam);

    void OnCreate(HWND hWnd);
    void OnCommand(HWND hWnd, WPARAM wParam);
    void OnNotify(LPARAM lParam);
    void ReloadProcessList();
    void LoadAllProcesses();
    void LoadWindowProcesses();
    void ConfirmSelection();
    bool IsAppWindow(HWND hWnd) const;
    int AddProcessIcon(const ProcessItem& item);
    void CreateFonts();
    void DestroyFonts();
    void ApplyFonts() const;

    HWND m_owner = nullptr;
    HWND m_hWnd = nullptr;
    HWND m_hTitle = nullptr;
    HWND m_hSubtitle = nullptr;
    HWND m_hRadioAll = nullptr;
    HWND m_hRadioApps = nullptr;
    HWND m_hList = nullptr;
    HWND m_hPathHint = nullptr;
    HIMAGELIST m_hImageList = nullptr;

    HFONT m_hTitleFont = nullptr;
    HFONT m_hBodyFont = nullptr;
    HFONT m_hSmallFont = nullptr;

    HANDLE* m_selectedHandle = nullptr;
    std::wstring* m_selectedName = nullptr;
    DWORD* m_selectedPid = nullptr;

    bool m_confirmed = false;
    FilterMode m_mode = FilterMode::AllProcesses;
    std::vector<ProcessItem> m_processes;
};
