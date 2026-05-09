# Msg_Inject

`Msg_Inject` 是一个基于 C++、Win32、Direct3D 11 和 Dear ImGui 开发的 Windows 桌面工具。它提供了一个图形界面，用来选择目标进程、整理载荷文件，并从统一界面触发注入流程。

## 功能特性

- 基于 Win32 + Dear ImGui + Direct3D 11 的桌面界面
- 支持选择全部进程或仅显示有窗口的进程
- 支持通过文件选择器或拖拽方式添加载荷文件
- 可对文件进行基础类型识别，区分 `EXE`、`DLL` 和其他文件
- 支持可选的反射加载模式，通过 `PEReflact` 路径构建载荷

## 项目结构

```text
Msg_Inject.sln
Msg_Inject/
  Main/
    App.cpp
    MainWindow.cpp
    Executor.cpp
    ProcessInject.cpp
    PEReflact.cpp
imgui-master/
```

## 依赖说明

当前项目直接将 Dear ImGui 源码放在 `imgui-master` 目录中，并编译以下文件：

- `imgui.cpp`
- `imgui_draw.cpp`
- `imgui_tables.cpp`
- `imgui_widgets.cpp`
- `backends/imgui_impl_dx11.cpp`
- `backends/imgui_impl_win32.cpp`

## 编译方法

1. 使用 Visual Studio 2022 打开 [Msg_Inject.sln](E:\Code\Msg_Inject\Msg_Inject.sln)。
2. 选择 `Debug` 或 `Release` 配置。
3. 选择 `Win32` 或 `x64` 平台。
4. 编译 `Msg_Inject` 项目。

说明：

- 当前工程文件里的 `OutDir` 被设置成了 `..\\..\\GameCheats\\cs2`。
- 如果你的电脑上没有这个路径，需要先在项目属性中修改输出目录再编译。

## 使用方法

1. 启动程序。
2. 点击选择进程按钮，选中目标进程。
3. 通过文件选择框或直接拖拽的方式添加一个或多个文件。
4. 根据测试需求决定是否启用反射加载模式。
5. 点击注入按钮开始执行。
6. 查看执行摘要窗口中的成功或失败信息。

## 代码说明

- `MainWindow` 负责主界面、进程列表和文件管理流程。
- `FileRepository` 负责保存已选择的载荷文件。
- `FileTypeDetector` 用于区分 `EXE`、`DLL` 和非 PE 文件。
- `Executor` 负责准备载荷并调度执行。
- `ProcessInject` 包含界面触发时使用的注入入口。
- `PEReflact` 负责在启用反射模式时构建对应载荷。

## 当前状态

这个项目目前仍然偏实验性质。这个仓库的主要作用是把当前代码整理成一个可分享、可继续维护的版本，因此部分实现仍然更偏向本地研究和测试，而不是完整的发布版工具。

## 免责声明

本仓库仅用于研究、学习和受控测试。运行涉及进程注入的代码前，请自行确认相关行为在法律、伦理和安全层面上的风险与边界。
