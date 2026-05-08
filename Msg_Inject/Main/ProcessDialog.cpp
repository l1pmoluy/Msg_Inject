#include "ProcessDialog.h"

namespace
{
    constexpr wchar_t kDialogClassName[] = L"MsgInjectProcessDialog";

    constexpr int IDC_RADIO_ALL = 3001;
    constexpr int IDC_RADIO_APPS = 3002;
    constexpr int IDC_LIST_PROCESS = 3003;
    constexpr int IDC_BUTTON_OK = 3004;
    constexpr int IDC_BUTTON_CANCEL = 3005;

    constexpr COLORREF kDialogBgColor = RGB(243, 246, 250);
    constexpr COLORREF kPanelBgColor = RGB(255, 255, 255);
    constexpr COLORREF kListBgColor = RGB(250, 252, 254);
    constexpr COLORREF kTextColor = RGB(33, 42, 54);
    constexpr COLORREF kMutedTextColor = RGB(105, 116, 130);
}

ProcessDialog::ProcessDialog(HWND owner)
    : m_owner(owner)
{
}

bool ProcessDialog::Show(HANDLE& selectedHandle, std::wstring& selectedName, DWORD& selectedPid)
{
    m_selectedHandle = &selectedHandle;
    m_selectedName = &selectedName;
    m_selectedPid = &selectedPid;
    m_confirmed = false;
    m_mode = FilterMode::AllProcesses;

    WNDCLASSW wc{};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(m_owner, GWLP_HINSTANCE));
    wc.lpszClassName = kDialogClassName;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(kDialogBgColor);
    RegisterClassW(&wc);

    m_hWnd = CreateWindowExW(
        WS_EX_DLGMODALFRAME,
        kDialogClassName,
        L"选择进程",
        WS_CAPTION | WS_SYSMENU | WS_POPUP | WS_VISIBLE,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        700,
        520,
        m_owner,
        nullptr,
        wc.hInstance,
        this);

    if (!m_hWnd)
    {
        return false;
    }

    EnableWindow(m_owner, FALSE);

    MSG msg{};
    while (IsWindow(m_hWnd) && GetMessageW(&msg, nullptr, 0, 0))
    {
        if (!IsDialogMessageW(m_hWnd, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    EnableWindow(m_owner, TRUE);
    SetActiveWindow(m_owner);

    return m_confirmed;
}

LRESULT CALLBACK ProcessDialog::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    ProcessDialog* self = nullptr;

    if (message == WM_NCCREATE)
    {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        self = reinterpret_cast<ProcessDialog*>(cs->lpCreateParams);
        SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        self->m_hWnd = hWnd;
    }
    else
    {
        self = reinterpret_cast<ProcessDialog*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
    }

    if (!self)
    {
        return DefWindowProcW(hWnd, message, wParam, lParam);
    }

    switch (message)
    {
    case WM_CREATE:
        self->OnCreate(hWnd);
        return 0;
    case WM_COMMAND:
        self->OnCommand(hWnd, wParam);
        return 0;
    case WM_NOTIFY:
        self->OnNotify(lParam);
        return 0;
    case WM_PAINT:
    {
        PAINTSTRUCT ps{};
        HDC hdc = BeginPaint(hWnd, &ps);

        RECT client{};
        GetClientRect(hWnd, &client);
        HBRUSH bgBrush = CreateSolidBrush(kDialogBgColor);
        FillRect(hdc, &client, bgBrush);
        DeleteObject(bgBrush);

        RECT panel{ 18, 78, 682, 482 };
        HBRUSH panelBrush = CreateSolidBrush(kPanelBgColor);
        FillRect(hdc, &panel, panelBrush);
        DeleteObject(panelBrush);

        HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(223, 229, 236));
        HGDIOBJ oldPen = SelectObject(hdc, borderPen);
        HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
        Rectangle(hdc, panel.left, panel.top, panel.right, panel.bottom);
        SelectObject(hdc, oldBrush);
        SelectObject(hdc, oldPen);
        DeleteObject(borderPen);

        EndPaint(hWnd, &ps);
        return 0;
    }
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN:
    {
        HDC hdc = reinterpret_cast<HDC>(wParam);
        HWND control = reinterpret_cast<HWND>(lParam);
        const bool isPanelControl =
            control == self->m_hRadioAll ||
            control == self->m_hRadioApps ||
            control == self->m_hPathHint;

        SetTextColor(hdc, control == self->m_hSubtitle || control == self->m_hPathHint ? kMutedTextColor : kTextColor);
        SetBkColor(hdc, isPanelControl ? kPanelBgColor : kDialogBgColor);

        static HBRUSH dialogBrush = CreateSolidBrush(kDialogBgColor);
        static HBRUSH panelBrush = CreateSolidBrush(kPanelBgColor);
        return reinterpret_cast<LRESULT>(isPanelControl ? panelBrush : dialogBrush);
    }
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    {
        HDC hdc = reinterpret_cast<HDC>(wParam);
        SetTextColor(hdc, kTextColor);
        SetBkColor(hdc, kListBgColor);
        static HBRUSH listBrush = CreateSolidBrush(kListBgColor);
        return reinterpret_cast<LRESULT>(listBrush);
    }
    case WM_CLOSE:
        DestroyWindow(hWnd);
        return 0;
    case WM_DESTROY:
        self->DestroyFonts();
        if (self->m_hImageList != nullptr)
        {
            ImageList_Destroy(self->m_hImageList);
            self->m_hImageList = nullptr;
        }
        return 0;
    default:
        return DefWindowProcW(hWnd, message, wParam, lParam);
    }
}

BOOL CALLBACK ProcessDialog::EnumWindowProc(HWND hWnd, LPARAM lParam)
{
    auto* self = reinterpret_cast<ProcessDialog*>(lParam);
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

    HANDLE processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
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

void ProcessDialog::OnCreate(HWND hWnd)
{
    CreateFonts();

    m_hTitle = CreateWindowW(
        L"STATIC",
        L"选择目标进程",
        WS_CHILD | WS_VISIBLE,
        18, 16, 180, 26,
        hWnd, nullptr, nullptr, nullptr);

    m_hSubtitle = CreateWindowW(
        L"STATIC",
        L"保留最常用的筛选和列表信息，快速选中目标。",
        WS_CHILD | WS_VISIBLE,
        18, 46, 300, 18,
        hWnd, nullptr, nullptr, nullptr);

    m_hRadioAll = CreateWindowW(
        L"BUTTON",
        L"全部进程",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        36, 96, 110, 24,
        hWnd, (HMENU)(INT_PTR)IDC_RADIO_ALL, nullptr, nullptr);

    m_hRadioApps = CreateWindowW(
        L"BUTTON",
        L"仅窗口程序",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        152, 96, 140, 24,
        hWnd, (HMENU)(INT_PTR)IDC_RADIO_APPS, nullptr, nullptr);

    SendMessageW(m_hRadioAll, BM_SETCHECK, BST_CHECKED, 0);

    m_hList = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        WC_LISTVIEWW,
        L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
        36, 132, 628, 286,
        hWnd, (HMENU)(INT_PTR)IDC_LIST_PROCESS, nullptr, nullptr);

    ListView_SetExtendedListViewStyle(
        m_hList,
        LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_LABELTIP);

    m_hImageList = ImageList_Create(20, 20, ILC_COLOR32 | ILC_MASK, 32, 32);
    ListView_SetImageList(m_hList, m_hImageList, LVSIL_SMALL);

    LVCOLUMNW column{};
    column.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

    column.pszText = const_cast<LPWSTR>(L"进程");
    column.cx = 482;
    ListView_InsertColumn(m_hList, 0, &column);

    column.pszText = const_cast<LPWSTR>(L"PID");
    column.cx = 110;
    column.iSubItem = 1;
    ListView_InsertColumn(m_hList, 1, &column);

    m_hPathHint = CreateWindowW(
        L"STATIC",
        L"双击列表可直接确认，默认优先显示文件名与 PID。",
        WS_CHILD | WS_VISIBLE,
        36, 430, 320, 18,
        hWnd, nullptr, nullptr, nullptr);

    CreateWindowW(
        L"BUTTON",
        L"确定",
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
        470, 436, 88, 32,
        hWnd, (HMENU)(INT_PTR)IDC_BUTTON_OK, nullptr, nullptr);

    CreateWindowW(
        L"BUTTON",
        L"取消",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        576, 436, 88, 32,
        hWnd, (HMENU)(INT_PTR)IDC_BUTTON_CANCEL, nullptr, nullptr);

    ApplyFonts();
    ReloadProcessList();
}

void ProcessDialog::OnCommand(HWND hWnd, WPARAM wParam)
{
    switch (LOWORD(wParam))
    {
    case IDC_RADIO_ALL:
        if (HIWORD(wParam) == BN_CLICKED)
        {
            m_mode = FilterMode::AllProcesses;
            ReloadProcessList();
        }
        break;
    case IDC_RADIO_APPS:
        if (HIWORD(wParam) == BN_CLICKED)
        {
            m_mode = FilterMode::WindowApps;
            ReloadProcessList();
        }
        break;
    case IDC_BUTTON_OK:
        ConfirmSelection();
        break;
    case IDC_BUTTON_CANCEL:
        DestroyWindow(hWnd);
        break;
    }
}

void ProcessDialog::OnNotify(LPARAM lParam)
{
    const auto* hdr = reinterpret_cast<NMHDR*>(lParam);
    if (hdr->idFrom == IDC_LIST_PROCESS && hdr->code == NM_DBLCLK)
    {
        ConfirmSelection();
    }
}

void ProcessDialog::ReloadProcessList()
{
    m_processes.clear();
    ListView_DeleteAllItems(m_hList);
    if (m_hImageList != nullptr)
    {
        ImageList_RemoveAll(m_hImageList);
    }

    if (m_mode == FilterMode::AllProcesses)
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

    for (size_t i = 0; i < m_processes.size(); ++i)
    {
        const int imageIndex = AddProcessIcon(m_processes[i]);

        LVITEMW item{};
        item.mask = LVIF_TEXT | LVIF_IMAGE;
        item.iItem = static_cast<int>(i);
        item.pszText = const_cast<LPWSTR>(m_processes[i].exeName.c_str());
        item.iImage = imageIndex;
        ListView_InsertItem(m_hList, &item);

        const std::wstring pidText = std::to_wstring(m_processes[i].pid);
        ListView_SetItemText(m_hList, static_cast<int>(i), 1, const_cast<LPWSTR>(pidText.c_str()));
    }

    if (!m_processes.empty())
    {
        ListView_SetItemState(m_hList, 0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        ListView_EnsureVisible(m_hList, 0, FALSE);
    }
}

void ProcessDialog::LoadAllProcesses()
{
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
    {
        return;
    }

    PROCESSENTRY32W pe{};
    pe.dwSize = sizeof(pe);

    if (Process32FirstW(snapshot, &pe))
    {
        do
        {
            ProcessItem item;
            item.pid = pe.th32ProcessID;
            item.exeName = pe.szExeFile;
            m_processes.push_back(std::move(item));
        } while (Process32NextW(snapshot, &pe));
    }

    CloseHandle(snapshot);
}

void ProcessDialog::LoadWindowProcesses()
{
    EnumWindows(EnumWindowProc, reinterpret_cast<LPARAM>(this));
}

void ProcessDialog::ConfirmSelection()
{
    const int index = ListView_GetNextItem(m_hList, -1, LVNI_SELECTED);
    if (index < 0 || index >= static_cast<int>(m_processes.size()))
    {
        MessageBoxW(m_hWnd, L"请先选中一个进程。", L"提示", MB_OK | MB_ICONINFORMATION);
        return;
    }

    const auto& item = m_processes[index];
    HANDLE processHandle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, item.pid);
    if (!processHandle)
    {
        std::wstring message = L"OpenProcess 失败，错误码=" + std::to_wstring(GetLastError());
        MessageBoxW(m_hWnd, message.c_str(), L"错误", MB_OK | MB_ICONERROR);
        return;
    }

    *m_selectedHandle = processHandle;
    *m_selectedName = item.exeName;
    *m_selectedPid = item.pid;
    m_confirmed = true;

    DestroyWindow(m_hWnd);
}

bool ProcessDialog::IsAppWindow(HWND hWnd) const
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
    if ((exStyle & WS_EX_TOOLWINDOW) != 0)
    {
        return false;
    }

    return true;
}

int ProcessDialog::AddProcessIcon(const ProcessItem& item)
{
    SHFILEINFOW sfi{};
    HICON hIcon = nullptr;

    if (!item.exePath.empty())
    {
        SHGetFileInfoW(
            item.exePath.c_str(),
            FILE_ATTRIBUTE_NORMAL,
            &sfi,
            sizeof(sfi),
            SHGFI_ICON | SHGFI_SMALLICON);
        hIcon = sfi.hIcon;
    }

    if (hIcon == nullptr)
    {
        hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    }

    const int index = ImageList_AddIcon(m_hImageList, hIcon);

    if (sfi.hIcon != nullptr)
    {
        DestroyIcon(sfi.hIcon);
    }

    return index;
}

void ProcessDialog::CreateFonts()
{
    DestroyFonts();

    m_hTitleFont = CreateFontW(-22, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    m_hBodyFont = CreateFontW(-16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    m_hSmallFont = CreateFontW(-14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
}

void ProcessDialog::DestroyFonts()
{
    if (m_hTitleFont)
    {
        DeleteObject(m_hTitleFont);
        m_hTitleFont = nullptr;
    }
    if (m_hBodyFont)
    {
        DeleteObject(m_hBodyFont);
        m_hBodyFont = nullptr;
    }
    if (m_hSmallFont)
    {
        DeleteObject(m_hSmallFont);
        m_hSmallFont = nullptr;
    }
}

void ProcessDialog::ApplyFonts() const
{
    SendMessageW(m_hTitle, WM_SETFONT, reinterpret_cast<WPARAM>(m_hTitleFont), TRUE);

    const HWND bodyControls[] = {
        m_hSubtitle, m_hRadioAll, m_hRadioApps, m_hList
    };
    for (HWND control : bodyControls)
    {
        SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(m_hBodyFont), TRUE);
    }

    SendMessageW(m_hPathHint, WM_SETFONT, reinterpret_cast<WPARAM>(m_hSmallFont), TRUE);
}
