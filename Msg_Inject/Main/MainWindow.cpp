#include "MainWindow.h"
#include "Executor.h"
#include "FileTypeDetector.h"
#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace
{
    constexpr wchar_t kWindowClassName[] = L"MsgInjectImGuiWindow";
    constexpr float kWindowWidth = 1100.0f;
    constexpr float kWindowHeight = 760.0f;

    void ApplyTheme()
    {
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowRounding = 14.0f;
        style.ChildRounding = 12.0f;
        style.FrameRounding = 10.0f;
        style.PopupRounding = 12.0f;
        style.GrabRounding = 10.0f;
        style.ScrollbarRounding = 10.0f;
        style.FramePadding = ImVec2(12.0f, 10.0f);
        style.ItemSpacing = ImVec2(12.0f, 10.0f);
        style.WindowPadding = ImVec2(18.0f, 18.0f);
        style.ChildBorderSize = 0.0f;
        style.FrameBorderSize = 0.0f;
        style.WindowBorderSize = 0.0f;
        style.PopupBorderSize = 0.0f;

        ImVec4* colors = style.Colors;
        colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.10f, 0.12f, 1.00f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.12f, 0.14f, 0.17f, 1.00f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.10f, 0.12f, 0.15f, 0.98f);
        colors[ImGuiCol_Border] = ImVec4(0.22f, 0.25f, 0.30f, 0.35f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.18f, 0.22f, 1.00f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.24f, 0.28f, 1.00f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.24f, 0.28f, 0.33f, 1.00f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.10f, 0.12f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.08f, 0.10f, 0.12f, 1.00f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.11f, 0.13f, 0.16f, 1.00f);
        colors[ImGuiCol_Button] = ImVec4(0.86f, 0.36f, 0.18f, 1.00f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.94f, 0.43f, 0.22f, 1.00f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.78f, 0.30f, 0.12f, 1.00f);
        colors[ImGuiCol_Header] = ImVec4(0.18f, 0.22f, 0.27f, 1.00f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.22f, 0.27f, 0.33f, 1.00f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.31f, 0.37f, 1.00f);
        colors[ImGuiCol_CheckMark] = ImVec4(1.00f, 0.78f, 0.43f, 1.00f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.94f, 0.43f, 0.22f, 1.00f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.98f, 0.54f, 0.31f, 1.00f);
        colors[ImGuiCol_Text] = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.55f, 0.60f, 0.66f, 1.00f);
        colors[ImGuiCol_Separator] = ImVec4(0.24f, 0.28f, 0.33f, 1.00f);
        colors[ImGuiCol_TableHeaderBg] = ImVec4(0.13f, 0.15f, 0.19f, 1.00f);
        colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.03f);
    }
}

MainWindow::~MainWindow()
{
    ShutdownImGui();
    CleanupD3D();
    ClearProcessHandle();
}

bool MainWindow::Create(HINSTANCE instance, int nCmdShow)
{
    m_instance = instance;

    ImGui_ImplWin32_EnableDpiAwareness();
    const float mainScale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_CLASSDC;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = kWindowClassName;

    if (!RegisterClassExW(&wc))
    {
        return false;
    }

    m_hWnd = CreateWindowW(
        kWindowClassName,
        L"Msg Inject",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        static_cast<int>(kWindowWidth * mainScale),
        static_cast<int>(kWindowHeight * mainScale),
        nullptr,
        nullptr,
        instance,
        this);

    if (!m_hWnd)
    {
        return false;
    }

    DragAcceptFiles(m_hWnd, TRUE);

    if (!InitializeD3D())
    {
        DestroyWindow(m_hWnd);
        m_hWnd = nullptr;
        return false;
    }

    if (!InitializeImGui())
    {
        return false;
    }

    ShowWindow(m_hWnd, nCmdShow);
    UpdateWindow(m_hWnd);
    return true;
}

int MainWindow::Run()
{
    bool running = true;
    while (running)
    {
        if (!ProcessMessages(running))
        {
            break;
        }

        if (m_swapChainOccluded && m_swapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
        {
            Sleep(10);
            continue;
        }
        m_swapChainOccluded = false;

        if (m_resizeWidth != 0 && m_resizeHeight != 0)
        {
            CleanupRenderTarget();
            m_swapChain->ResizeBuffers(0, m_resizeWidth, m_resizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            m_resizeWidth = 0;
            m_resizeHeight = 0;
            CreateRenderTarget();
        }

        RenderFrame();
    }

    return 0;
}

bool MainWindow::InitializeD3D()
{
    DXGI_SWAP_CHAIN_DESC swapChainDesc{};
    swapChainDesc.BufferCount = 2;
    swapChainDesc.BufferDesc.Width = 0;
    swapChainDesc.BufferDesc.Height = 0;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
    swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.OutputWindow = m_hWnd;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.Windowed = TRUE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
    const D3D_FEATURE_LEVEL featureLevels[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };

    HRESULT result = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        createDeviceFlags,
        featureLevels,
        2,
        D3D11_SDK_VERSION,
        &swapChainDesc,
        &m_swapChain,
        &m_d3dDevice,
        &featureLevel,
        &m_d3dDeviceContext);

    if (result == DXGI_ERROR_UNSUPPORTED)
    {
        result = D3D11CreateDeviceAndSwapChain(
            nullptr,
            D3D_DRIVER_TYPE_WARP,
            nullptr,
            createDeviceFlags,
            featureLevels,
            2,
            D3D11_SDK_VERSION,
            &swapChainDesc,
            &m_swapChain,
            &m_d3dDevice,
            &featureLevel,
            &m_d3dDeviceContext);
    }

    if (result != S_OK)
    {
        return false;
    }

    CreateRenderTarget();
    return true;
}

void MainWindow::CleanupD3D()
{
    CleanupRenderTarget();
    if (m_swapChain)
    {
        m_swapChain->Release();
        m_swapChain = nullptr;
    }
    if (m_d3dDeviceContext)
    {
        m_d3dDeviceContext->Release();
        m_d3dDeviceContext = nullptr;
    }
    if (m_d3dDevice)
    {
        m_d3dDevice->Release();
        m_d3dDevice = nullptr;
    }
}

void MainWindow::CreateRenderTarget()
{
    ID3D11Texture2D* backBuffer = nullptr;
    if (m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer)) == S_OK)
    {
        m_d3dDevice->CreateRenderTargetView(backBuffer, nullptr, &m_mainRenderTargetView);
        backBuffer->Release();
    }
}

void MainWindow::CleanupRenderTarget()
{
    if (m_mainRenderTargetView)
    {
        m_mainRenderTargetView->Release();
        m_mainRenderTargetView = nullptr;
    }
}

bool MainWindow::InitializeImGui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();
    ApplyTheme();

    if (!ImGui_ImplWin32_Init(m_hWnd))
    {
        return false;
    }

    if (!ImGui_ImplDX11_Init(m_d3dDevice, m_d3dDeviceContext))
    {
        return false;
    }

    const char* fontPaths[] = {
        "C:\\Windows\\Fonts\\msyh.ttc",
        "C:\\Windows\\Fonts\\msyhbd.ttc",
        "C:\\Windows\\Fonts\\simhei.ttf"
    };

    for (const char* path : fontPaths)
    {
        if (io.Fonts->AddFontFromFileTTF(path, 18.0f, nullptr, io.Fonts->GetGlyphRangesChineseFull()) != nullptr)
        {
            break;
        }
    }

    if (io.Fonts->Fonts.empty())
    {
        io.Fonts->AddFontDefault();
    }

    return true;
}

void MainWindow::ShutdownImGui()
{
    if (ImGui::GetCurrentContext() == nullptr)
    {
        return;
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

bool MainWindow::ProcessMessages(bool& running)
{
    MSG msg{};
    while (PeekMessageW(&msg, nullptr, 0U, 0U, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
        if (msg.message == WM_QUIT)
        {
            running = false;
            return false;
        }
    }

    return true;
}

void MainWindow::RenderFrame()
{
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    RenderUi();

    ImGui::Render();
    const float clearColor[4] = { 0.07f, 0.08f, 0.10f, 1.00f };
    m_d3dDeviceContext->OMSetRenderTargets(1, &m_mainRenderTargetView, nullptr);
    m_d3dDeviceContext->ClearRenderTargetView(m_mainRenderTargetView, clearColor);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    const HRESULT presentResult = m_swapChain->Present(1, 0);
    m_swapChainOccluded = (presentResult == DXGI_STATUS_OCCLUDED);
}

void MainWindow::RenderUi()
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);

    ImGuiWindowFlags windowFlags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoSavedSettings;

    const std::string title = ToUtf8(L"Msg Inject \u5de5\u4f5c\u53f0");
    ImGui::Begin(title.c_str(), nullptr, windowFlags);
    RenderHeader();
    ImGui::Spacing();

    ImGui::Columns(2, nullptr, false);
    RenderProcessPanel();
    ImGui::NextColumn();
    RenderFilesPanel();
    ImGui::Columns(1);

    ImGui::Spacing();
    RenderFooter();
    RenderProcessPicker();
    ImGui::End();
}

void MainWindow::RenderHeader()
{
    const std::string subtitle = ToUtf8(L"\u57fa\u4e8e ImGui \u7684\u8fdb\u7a0b\u9009\u62e9\u3001\u8f7d\u8377\u6574\u7406\u4e0e\u6ce8\u5165\u6267\u884c\u754c\u9762\u3002");

    ImGui::TextColored(ImVec4(1.0f, 0.78f, 0.43f, 1.0f), "Msg Inject");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::TextDisabled("%s", subtitle.c_str());
    ImGui::Separator();
}

void MainWindow::RenderProcessPanel()
{
    const std::string panelTitle = ToUtf8(L"\u76ee\u6807\u8fdb\u7a0b");
    const std::string chooseProcess = ToUtf8(L"\u9009\u62e9\u8fdb\u7a0b");
    const std::string noProcess = ToUtf8(L"\u5f53\u524d\u672a\u9009\u62e9\u8fdb\u7a0b\u3002");
    const std::string handleLabel = ToUtf8(L"\u53e5\u67c4:");
    const std::string reflectLabel = ToUtf8(L"\u542f\u7528\u53cd\u5c04\u5c01\u88c5");
    const std::string reflectTip = ToUtf8(L"\u542f\u7528\u540e\uff0c\u4f1a\u5148\u628a PE_Reflact \u7684 loader shellcode \u62fc\u63a5\u5230\u6587\u4ef6\u5185\u5bb9\u524d\u9762\uff0c\u518d\u4f5c\u4e3a payload \u6ce8\u5165\u3002");
    const std::string startButton = ToUtf8(L"\u5f00\u59cb\u6ce8\u5165");

    ImGui::BeginChild("ProcessPanel", ImVec2(0.0f, 0.0f), true);

    ImGui::TextUnformatted(panelTitle.c_str());
    ImGui::Spacing();

    if (ImGui::Button(chooseProcess.c_str(), ImVec2(-1.0f, 0.0f)))
    {
        m_openProcessPicker = true;
        ReloadProcessList();
    }

    ImGui::Spacing();
    if (m_selectedPid == 0 || m_selectedProcessHandle == nullptr)
    {
        ImGui::TextDisabled("%s", noProcess.c_str());
    }
    else
    {
        ImGui::TextWrapped("%s", ToUtf8(m_selectedProcessName).c_str());
        ImGui::Text("PID: %lu", static_cast<unsigned long>(m_selectedPid));
        ImGui::Text("%s %llu", handleLabel.c_str(), static_cast<unsigned long long>(reinterpret_cast<UINT_PTR>(m_selectedProcessHandle)));
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Checkbox(reflectLabel.c_str(), &m_useReflect);
    ImGui::TextWrapped("%s", reflectTip.c_str());

    ImGui::Spacing();
    if (ImGui::Button(startButton.c_str(), ImVec2(-1.0f, 42.0f)))
    {
        StartExecution();
    }

    ImGui::EndChild();
}

void MainWindow::RenderFilesPanel()
{
    const std::string panelTitle = ToUtf8(L"\u8f7d\u8377\u6587\u4ef6");
    const std::string addFiles = ToUtf8(L"\u6dfb\u52a0\u6587\u4ef6");
    const std::string removeChecked = ToUtf8(L"\u79fb\u9664\u52fe\u9009");
    const std::string dragTip = ToUtf8(L"\u53ef\u4ee5\u628a\u6587\u4ef6\u76f4\u63a5\u62d6\u8fdb\u7a97\u53e3\uff0c\u6216\u901a\u8fc7\u6587\u4ef6\u9009\u62e9\u5668\u6dfb\u52a0\u3002");
    const std::string useColumn = ToUtf8(L"\u542f\u7528");
    const std::string typeColumn = ToUtf8(L"\u7c7b\u578b");
    const std::string pathColumn = ToUtf8(L"\u8def\u5f84");

    ImGui::BeginChild("FilesPanel", ImVec2(0.0f, 0.0f), true);

    ImGui::TextUnformatted(panelTitle.c_str());
    ImGui::Spacing();

    if (ImGui::Button(addFiles.c_str(), ImVec2(140.0f, 0.0f)))
    {
        AddFilesFromDialog();
    }
    ImGui::SameLine();
    if (ImGui::Button(removeChecked.c_str(), ImVec2(160.0f, 0.0f)))
    {
        RemoveSelectedFiles();
    }

    ImGui::Spacing();
    ImGui::TextDisabled("%s", dragTip.c_str());
    ImGui::Spacing();

    if (ImGui::BeginTable("FilesTable", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY))
    {
        ImGui::TableSetupColumn(useColumn.c_str(), ImGuiTableColumnFlags_WidthFixed, 48.0f);
        ImGui::TableSetupColumn(typeColumn.c_str(), ImGuiTableColumnFlags_WidthFixed, 70.0f);
        ImGui::TableSetupColumn(pathColumn.c_str(), ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        auto items = m_files.ToDisplayItems();
        for (size_t i = 0; i < items.size(); ++i)
        {
            const MY_FILE* current = m_files.Head();
            for (size_t index = 0; current != nullptr && index < i; ++index)
            {
                current = current->next.get();
            }

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            bool selected = items[i].selected;
            std::string checkboxId = "##file_" + std::to_string(i);
            if (ImGui::Checkbox(checkboxId.c_str(), &selected))
            {
                m_files.SetSelectedAt(i, selected);
            }

            ImGui::TableSetColumnIndex(1);
            if (current != nullptr)
            {
                ImGui::TextUnformatted(ToUtf8(FileTypeDetector::ToString(current->type)).c_str());
            }

            ImGui::TableSetColumnIndex(2);
            if (current != nullptr)
            {
                ImGui::TextWrapped("%s", ToUtf8(current->path).c_str());
            }
        }

        ImGui::EndTable();
    }

    ImGui::EndChild();
}

void MainWindow::RenderFooter()
{
    ImVec4 color = m_statusIsError ? ImVec4(0.95f, 0.35f, 0.33f, 1.0f) : ImVec4(0.64f, 0.82f, 0.47f, 1.0f);
    ImGui::TextColored(color, "%s", ToUtf8(m_statusMessage).c_str());
}

void MainWindow::RenderProcessPicker()
{
    const std::string popupTitle = ToUtf8(L"\u8fdb\u7a0b\u9009\u62e9");
    const std::string allProcesses = ToUtf8(L"\u5168\u90e8\u8fdb\u7a0b");
    const std::string windowApps = ToUtf8(L"\u4ec5\u7a97\u53e3\u7a0b\u5e8f");
    const std::string refresh = ToUtf8(L"\u5237\u65b0");
    const std::string nameColumn = ToUtf8(L"\u8fdb\u7a0b\u540d");
    const std::string pathColumn = ToUtf8(L"\u8def\u5f84");
    const std::string confirm = ToUtf8(L"\u786e\u5b9a");
    const std::string cancel = ToUtf8(L"\u53d6\u6d88");

    if (m_openProcessPicker)
    {
        ImGui::OpenPopup(popupTitle.c_str());
        m_openProcessPicker = false;
    }

    bool modalOpen = true;
    if (!ImGui::BeginPopupModal(popupTitle.c_str(), &modalOpen, ImGuiWindowFlags_AlwaysAutoResize))
    {
        return;
    }

    int modeValue = (m_processFilterMode == ProcessFilterMode::AllProcesses) ? 0 : 1;
    if (ImGui::RadioButton(allProcesses.c_str(), modeValue == 0))
    {
        m_processFilterMode = ProcessFilterMode::AllProcesses;
        ReloadProcessList();
    }
    ImGui::SameLine();
    if (ImGui::RadioButton(windowApps.c_str(), modeValue == 1))
    {
        m_processFilterMode = ProcessFilterMode::WindowApps;
        ReloadProcessList();
    }

    if (ImGui::Button(refresh.c_str()))
    {
        ReloadProcessList();
    }

    ImGui::Spacing();

    if (ImGui::BeginChild("ProcessList", ImVec2(760.0f, 420.0f), true))
    {
        if (ImGui::BeginTable("ProcessTable", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY))
        {
            ImGui::TableSetupColumn(nameColumn.c_str(), ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("PID", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableSetupColumn(pathColumn.c_str(), ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            for (size_t i = 0; i < m_processes.size(); ++i)
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);

                const bool isSelected = (m_selectedProcessIndex == static_cast<int>(i));
                const std::string label = ToUtf8(m_processes[i].exeName) + "##proc_" + std::to_string(i);
                if (ImGui::Selectable(label.c_str(), isSelected, ImGuiSelectableFlags_SpanAllColumns))
                {
                    m_selectedProcessIndex = static_cast<int>(i);
                }
                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                {
                    m_selectedProcessIndex = static_cast<int>(i);
                    ConfirmSelectedProcess();
                    ImGui::CloseCurrentPopup();
                }

                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%lu", static_cast<unsigned long>(m_processes[i].pid));

                ImGui::TableSetColumnIndex(2);
                ImGui::TextWrapped("%s", ToUtf8(m_processes[i].exePath).c_str());
            }

            ImGui::EndTable();
        }
        ImGui::EndChild();
    }

    if (ImGui::Button(confirm.c_str(), ImVec2(120.0f, 0.0f)))
    {
        ConfirmSelectedProcess();
        if (m_selectedPid != 0)
        {
            ImGui::CloseCurrentPopup();
        }
    }
    ImGui::SameLine();
    if (ImGui::Button(cancel.c_str(), ImVec2(120.0f, 0.0f)))
    {
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
}

void MainWindow::ReloadProcessList()
{
    m_processes.clear();
    m_selectedProcessIndex = -1;

    if (m_processFilterMode == ProcessFilterMode::AllProcesses)
    {
        LoadAllProcesses();
    }
    else
    {
        LoadWindowProcesses();
    }

    std::sort(m_processes.begin(), m_processes.end(), [](const ProcessItem& left, const ProcessItem& right) {
        return _wcsicmp(left.exeName.c_str(), right.exeName.c_str()) < 0;
    });

    if (!m_processes.empty())
    {
        m_selectedProcessIndex = 0;
    }
}

void MainWindow::LoadAllProcesses()
{
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
    {
        SetStatusMessage(L"\u521b\u5efa\u8fdb\u7a0b\u5feb\u7167\u5931\u8d25\u3002", true);
        return;
    }

    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);

    if (Process32FirstW(snapshot, &entry))
    {
        do
        {
            ProcessItem item;
            item.pid = entry.th32ProcessID;
            item.exeName = entry.szExeFile;
            m_processes.push_back(std::move(item));
        } while (Process32NextW(snapshot, &entry));
    }

    CloseHandle(snapshot);
}

void MainWindow::LoadWindowProcesses()
{
    EnumWindows(EnumWindowProc, reinterpret_cast<LPARAM>(this));
}

bool MainWindow::IsAppWindow(HWND hWnd) const
{
    if (!IsWindowVisible(hWnd))
    {
        return false;
    }
    if (GetWindow(hWnd, GW_OWNER) != nullptr)
    {
        return false;
    }
    if (GetWindowTextLengthW(hWnd) == 0)
    {
        return false;
    }

    const LONG_PTR exStyle = GetWindowLongPtrW(hWnd, GWL_EXSTYLE);
    return (exStyle & WS_EX_TOOLWINDOW) == 0;
}

void MainWindow::ConfirmSelectedProcess()
{
    if (m_selectedProcessIndex < 0 || m_selectedProcessIndex >= static_cast<int>(m_processes.size()))
    {
        SetStatusMessage(L"\u8bf7\u5148\u9009\u62e9\u4e00\u4e2a\u8fdb\u7a0b\u3002", true);
        return;
    }

    const ProcessItem& item = m_processes[static_cast<size_t>(m_selectedProcessIndex)];
    HANDLE processHandle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, item.pid);
    if (!processHandle)
    {
        SetStatusMessage(L"OpenProcess \u5931\u8d25\uff0c\u9519\u8bef\u7801=" + std::to_wstring(GetLastError()), true);
        return;
    }

    ClearProcessHandle();
    m_selectedProcessHandle = processHandle;
    m_selectedPid = item.pid;
    m_selectedProcessName = item.exeName;
    SetStatusMessage(L"\u5df2\u9009\u62e9\u8fdb\u7a0b\uff1a" + item.exeName + L"\uff08PID=" + std::to_wstring(item.pid) + L"\uff09", false);
}

void MainWindow::OnDropFiles(HDROP hDrop)
{
    const UINT count = DragQueryFileW(hDrop, 0xFFFFFFFF, nullptr, 0);
    for (UINT i = 0; i < count; ++i)
    {
        wchar_t buffer[MAX_PATH] = {};
        DragQueryFileW(hDrop, i, buffer, MAX_PATH);
        AddDroppedFile(buffer);
    }

    DragFinish(hDrop);
}

void MainWindow::AddFilesFromDialog()
{
    wchar_t buffer[65536] = {};

    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = m_hWnd;
    ofn.lpstrFile = buffer;
    ofn.nMaxFile = static_cast<DWORD>(sizeof(buffer) / sizeof(wchar_t));
    ofn.lpstrFilter = L"All Files\0*.*\0";
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT;

    if (!GetOpenFileNameW(&ofn))
    {
        return;
    }

    std::wstring first = buffer;
    wchar_t* cursor = buffer + first.length() + 1;
    if (*cursor == L'\0')
    {
        AddDroppedFile(first);
        return;
    }

    while (*cursor != L'\0')
    {
        const std::wstring fileName = cursor;
        AddDroppedFile(first + L"\\" + fileName);
        cursor += fileName.length() + 1;
    }
}

void MainWindow::AddDroppedFile(const std::wstring& path)
{
    std::wstring message;
    if (!m_files.AddFile(path, message))
    {
        SetStatusMessage(message, true);
        return;
    }

    SetStatusMessage(message, false);
}

void MainWindow::RemoveSelectedFiles()
{
    const size_t removed = m_files.RemoveSelected();
    if (removed == 0)
    {
        SetStatusMessage(L"\u6ca1\u6709\u52fe\u9009\u9700\u8981\u79fb\u9664\u7684\u6587\u4ef6\u3002", true);
        return;
    }

    SetStatusMessage(L"\u5df2\u79fb\u9664 " + std::to_wstring(removed) + L" \u4e2a\u6587\u4ef6\u3002", false);
}

void MainWindow::StartExecution()
{
    Executor::Run(m_selectedPid, m_selectedProcessHandle, m_files, m_files.Head(), m_useReflect, m_hWnd);
    SetStatusMessage(L"\u6267\u884c\u5b8c\u6210\uff0c\u8be6\u60c5\u8bf7\u67e5\u770b\u7ed3\u679c\u6458\u8981\u7a97\u53e3\u3002", false);
}

void MainWindow::ClearProcessHandle()
{
    if (m_selectedProcessHandle)
    {
        CloseHandle(m_selectedProcessHandle);
        m_selectedProcessHandle = nullptr;
    }

    m_selectedPid = 0;
    m_selectedProcessName.clear();
}

void MainWindow::SetStatusMessage(const std::wstring& message, bool isError)
{
    m_statusMessage = message;
    m_statusIsError = isError;
}

std::string MainWindow::ToUtf8(const std::wstring& value) const
{
    if (value.empty())
    {
        return {};
    }

    const int required = WideCharToMultiByte(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
    std::string result(static_cast<size_t>(required), '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), result.data(), required, nullptr, nullptr);
    return result;
}

BOOL CALLBACK MainWindow::EnumWindowProc(HWND hWnd, LPARAM lParam)
{
    MainWindow* self = reinterpret_cast<MainWindow*>(lParam);
    if (!self->IsAppWindow(hWnd))
    {
        return TRUE;
    }

    DWORD pid = 0;
    GetWindowThreadProcessId(hWnd, &pid);
    if (pid == 0)
    {
        return TRUE;
    }

    for (const auto& item : self->m_processes)
    {
        if (item.pid == pid)
        {
            return TRUE;
        }
    }

    HANDLE processHandle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!processHandle)
    {
        return TRUE;
    }

    wchar_t exePath[MAX_PATH] = {};
    DWORD size = MAX_PATH;
    std::wstring exeName = L"<unknown>";
    std::wstring fullPath;
    if (QueryFullProcessImageNameW(processHandle, 0, exePath, &size) && size > 0)
    {
        fullPath.assign(exePath, size);
        const size_t pos = fullPath.find_last_of(L"\\/");
        exeName = (pos == std::wstring::npos) ? fullPath : fullPath.substr(pos + 1);
    }

    CloseHandle(processHandle);
    self->m_processes.push_back({ pid, exeName, fullPath });
    return TRUE;
}

LRESULT CALLBACK MainWindow::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
    {
        return TRUE;
    }

    MainWindow* self = nullptr;
    if (message == WM_NCCREATE)
    {
        auto* createStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);
        self = reinterpret_cast<MainWindow*>(createStruct->lpCreateParams);
        SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        self->m_hWnd = hWnd;
    }
    else
    {
        self = reinterpret_cast<MainWindow*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
    }

    switch (message)
    {
    case WM_SIZE:
        if (self != nullptr && self->m_swapChain != nullptr && wParam != SIZE_MINIMIZED)
        {
            self->m_resizeWidth = static_cast<UINT>(LOWORD(lParam));
            self->m_resizeHeight = static_cast<UINT>(HIWORD(lParam));
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
        {
            return 0;
        }
        break;
    case WM_DROPFILES:
        if (self != nullptr)
        {
            self->OnDropFiles(reinterpret_cast<HDROP>(wParam));
            return 0;
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hWnd, message, wParam, lParam);
}
