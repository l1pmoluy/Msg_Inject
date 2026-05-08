# Msg_Inject

`Msg_Inject` is a Windows desktop tool built with C++, Win32, Direct3D 11, and Dear ImGui. It provides a simple GUI for selecting a target process, organizing payload files, and triggering an injection workflow from a single interface.

This repository contains the main `Msg_Inject` project only. Experimental code that was not intended for publication has been excluded from the solution and from version control.

## Features

- Win32 desktop UI with a Dear ImGui front end and Direct3D 11 renderer
- Process picker that can list all processes or only visible window applications
- Drag-and-drop and file-dialog based payload management
- Basic file type detection for `EXE`, `DLL`, and other files
- Optional reflect mode that builds a payload through the `PEReflact` path before execution
- Summary dialog showing per-file execution results

## Project Structure

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

## Requirements

- Windows
- Visual Studio 2022
- MSVC toolset `v143`
- Windows SDK for the configured target platform
- Direct3D 11 development libraries

## Dependencies

This project currently vendors Dear ImGui source code through the `imgui-master` directory and builds against:

- `imgui.cpp`
- `imgui_draw.cpp`
- `imgui_tables.cpp`
- `imgui_widgets.cpp`
- `backends/imgui_impl_dx11.cpp`
- `backends/imgui_impl_win32.cpp`

## Build

1. Open [Msg_Inject.sln](E:\Code\Msg_Inject\Msg_Inject.sln) in Visual Studio 2022.
2. Select `Debug` or `Release`.
3. Select `Win32` or `x64`.
4. Build the `Msg_Inject` project.

Notes:

- The project file currently sets `OutDir` to `..\\..\\GameCheats\\cs2`.
- If that path does not exist on your machine, change the Output Directory in project properties before building.

## Usage

1. Launch the built application.
2. Click the process selection button and choose a target process.
3. Add one or more files through the file dialog or by dragging them into the window.
4. Enable or disable reflect mode depending on your test scenario.
5. Click the inject button to start execution.
6. Review the summary dialog for success or failure details.

## Implementation Notes

- `MainWindow` owns the main UI, process list, and file management flow.
- `FileRepository` stores the selected payload files as a linked structure.
- `FileTypeDetector` distinguishes between `EXE`, `DLL`, and non-PE files.
- `Executor` prepares payloads and dispatches execution.
- `ProcessInject` contains the injection entry point used by the UI workflow.
- `PEReflact` contains the payload-building path used when reflect mode is enabled.

## Current Status

This is an experimental project and the repository mainly preserves the current codebase in a shareable form. Some logic is still oriented toward local research and testing rather than polished release usage.

## Disclaimer

This repository is provided for research, learning, and controlled testing only. Make sure you understand the legal, ethical, and security implications before running process injection code on any system.
