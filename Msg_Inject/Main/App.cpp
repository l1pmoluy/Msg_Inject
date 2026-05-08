#include "App.h"
#include "Common.h"
#include "MainWindow.h"

int App::Run(HINSTANCE instance, int nCmdShow)
{
    INITCOMMONCONTROLSEX icc{};
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_STANDARD_CLASSES | ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icc);

    MainWindow window;
    if (!window.Create(instance, nCmdShow))
    {
        MessageBoxW(nullptr, L"窗口创建失败。", L"错误", MB_OK | MB_ICONERROR);
        return -1;
    }
    return window.Run();
}
