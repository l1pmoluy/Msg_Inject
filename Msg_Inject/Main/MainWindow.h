#pragma once
#include "Common.h"
#include "FileRepository.h"
#include <d3d11.h>

class MainWindow
{
public:
    ~MainWindow();

    bool Create(HINSTANCE instance, int nCmdShow);
    int Run();

private:
    struct ProcessItem
    {
        DWORD pid = 0;
        std::wstring exeName;
        std::wstring exePath;
    };

    enum class ProcessFilterMode
    {
        AllProcesses,
        WindowApps
    };

    static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    static BOOL CALLBACK EnumWindowProc(HWND hWnd, LPARAM lParam);

    bool InitializeD3D();
    void CleanupD3D();
    void CreateRenderTarget();
    void CleanupRenderTarget();
    bool InitializeImGui();
    void ShutdownImGui();
    bool ProcessMessages(bool& running);
    void RenderFrame();
    void RenderUi();
    void RenderHeader();
    void RenderProcessPanel();
    void RenderFilesPanel();
    void RenderFooter();
    void RenderProcessPicker();
    void ReloadProcessList();
    void LoadAllProcesses();
    void LoadWindowProcesses();
    bool IsAppWindow(HWND hWnd) const;
    void ConfirmSelectedProcess();
    void OnDropFiles(HDROP hDrop);
    void AddFilesFromDialog();
    void AddDroppedFile(const std::wstring& path);
    void RemoveSelectedFiles();
    void StartExecution();
    void ClearProcessHandle();
    void SetStatusMessage(const std::wstring& message, bool isError);
    std::string ToUtf8(const std::wstring& value) const;

    HINSTANCE m_instance = nullptr;
    HWND m_hWnd = nullptr;

    ID3D11Device* m_d3dDevice = nullptr;
    ID3D11DeviceContext* m_d3dDeviceContext = nullptr;
    IDXGISwapChain* m_swapChain = nullptr;
    ID3D11RenderTargetView* m_mainRenderTargetView = nullptr;
    UINT m_resizeWidth = 0;
    UINT m_resizeHeight = 0;
    bool m_swapChainOccluded = false;

    FileRepository m_files;
    HANDLE m_selectedProcessHandle = nullptr;
    DWORD m_selectedPid = 0;
    std::wstring m_selectedProcessName;

    std::vector<ProcessItem> m_processes;
    ProcessFilterMode m_processFilterMode = ProcessFilterMode::AllProcesses;
    bool m_openProcessPicker = false;
    int m_selectedProcessIndex = -1;
    bool m_useReflect = true;

    std::wstring m_statusMessage = L"\u5c31\u7eea\u3002";
    bool m_statusIsError = false;
};
