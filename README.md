[![](https://mermaid.ink/img/pako:eNo9kc1OwzAQhF_FLNcEJWmSBh-QaMqtSAgkDmAO28RJTOMfOY5EqfruOK6obzPfjLyrPUGjWw4U4jhmygk3ckpqbxGhvnnjhFZEopeTkGbkTDVadaKnTBHiBi59eo8Tv8p3tAL3I59CghBjhUR7rPWoLSUMbp_CY8BU-LG3aAaye72kHz8ZvIjmQBzanjuCxjD4InH8QDYe1VoaMXKy3e28falsAq2XouXTRAatD1dYB7gNUHfC3QQCEUhuJYrW731akgzC8AyWEVve4Ty6ZcSzj-Ls9NtRNUCdnXkEVs_9ALTDcfJqNi06vhXoF5FX16D60Fr-V7wEeoIfoOv0Lq1WVVokSZbnaR7BEeiqqBY3TYq8rNbFOYLfUE7uqqzM87JcZ-l9llVJGQFvhdP2-XKzcLrzH8G1i50?type=png)](https://mermaid.live/edit#pako:eNo9kc1OwzAQhF_FLNcEJWmSBh-QaMqtSAgkDmAO28RJTOMfOY5EqfruOK6obzPfjLyrPUGjWw4U4jhmygk3ckpqbxGhvnnjhFZEopeTkGbkTDVadaKnTBHiBi59eo8Tv8p3tAL3I59CghBjhUR7rPWoLSUMbp_CY8BU-LG3aAaye72kHz8ZvIjmQBzanjuCxjD4InH8QDYe1VoaMXKy3e28falsAq2XouXTRAatD1dYB7gNUHfC3QQCEUhuJYrW731akgzC8AyWEVve4Ty6ZcSzj-Ls9NtRNUCdnXkEVs_9ALTDcfJqNi06vhXoF5FX16D60Fr-V7wEeoIfoOv0Lq1WVVokSZbnaR7BEeiqqBY3TYq8rNbFOYLfUE7uqqzM87JcZ-l9llVJGQFvhdP2-XKzcLrzH8G1i50)

# 💉 ProcessAnalyzer

![Visual C++](https://img.shields.io/badge/Built%20With-Visual%20C%2B%2B-blue.svg) ![Qt](https://img.shields.io/badge/Powered%20By-Qt-green.svg)

[Read this README in Russian](README-RU.md)

**ProcessAnalyzer** is a Windows x64 function hooking application written in Qt and C++. It simplifies code flow changes by leveraging the function hooking technique and provides an intuitive GUI for interacting with system processes, their dependencies, and exports.

---

## Key Features
- **Process Inspection**: Enumerate active processes, their dependencies, and exports. Virtual libraries are supported and highlighted.
- **Function Hooking**: Modify the behavior of application functions with custom C++ payloads. No need for bytecode or assembly.
- **Dynamic Library Injection**: Connect your own C++ library with exported payload functions to target processes.
- **Interprocess Communication**: Local TCP protocol for communication between the GUI (Qt) and the injected DLL.
- **Convenient GUI**: Analyze, preview, and manage hooks visually.

---

## Repository Structure
The repository contains two logical projects:

### 1. **ProcessAnalyzer (Qt GUI)**
A Qt-based GUI for managing processes and hooks.

#### Key Files:
- **`detective.cpp`**:  
  - `getRunningProcesses()`: Captures currently running processes.  
  - `getRuntimeDeps(procId)`: Retrieves library dependencies for a process.  
  - `getStaticImport(fileName)`: Extracts static imports of a file.  
  - `getStaticExport(obj)`: Extracts static exports of a library.  

- **`injector.cpp`**:  
  - `apply(procId, dllName)`: Injects a DLL using `WriteProcessMemory` and starts its main function with `CreateRemoteThread`.

- **`mainwindow.cpp`**:  
  - Implements all GUI functionality.

- **`metacompute.cpp`**:  
  - `connectNetwork(procId)`: Injects the DLL, starts a server, and accepts connections from the target process.  
  - `getPreviewTreeItems(args)`: Composes GUI with pre-injection process data.  
  - `hookFunction(args)`: Applies a hook to the target function.  
  - `unhookFunction(args)`: Removes hooks (experimental).

- **`moduleobject.cpp`**:  
  - Represents process module info (name, path, isVirtual, etc.).

- **`network.cpp`**:  
  - Implements a Windows TCP server for communication.

---

### 2. **ProcessAnalyzerDLL**
A C++ DLL for handling the hooking mechanism and interprocess communication.

#### Key Files:
- **`Hooker.cpp`**:  
  - `installHook()`: Installs a hook by building trampoline and relay functions.  
  - `uninstallHook()`: Removes the hook and restores original bytes.  
  - `previewHook()`: Previews the bytes to be replaced, formatted as human-readable instructions.

- **`network.cpp`**:  
  - Implements a Windows TCP client for command reception.

- **`RuntimeDeps.cpp`**:  
  - Retrieves process dependency and export information.

- **`dllmain.cpp`**:  
  - Entry point, manages hook commands from the local TCP server.

---

## Example Usage
This example demonstrates hooking the `GdipSetSolidFillColor` function in `paint.exe`. After successful hooking, the brush color is forced to orange regardless of user input.

https://user-images.githubusercontent.com/55055449/175368939-d37e9666-f379-4499-95d1-e65e654f2f82.mp4

**Payload Code:**
```cpp
extern "C" _declspec(dllexport)
Gdiplus::GpStatus GdipSetSolidFillColorPayload(Gdiplus::GpSolidFill* brush, Gdiplus::ARGB color)
{
    Gdiplus::ARGB orange = 0xffff7700;
    return trampoline(brush, orange);
}
```

**Note**: Replacement functions must be marked as `extern "C" _declspec(dllexport)` and have the same signature as the original function.

---

## Build Instructions
### ProcessAnalyzer (Qt GUI):
- Tested on **Qt 6.1.3** and **MSVC 2019**.

### ProcessAnalyzerDLL and Test:
- Tested on **Microsoft Visual Studio 2019**.

---

## Restrictions
- Does not support 32-bit programs.
- Reverse operation for hooks is unstable.
- Libraries with replacement functions are not unloaded until the process terminates.
- Supports only one trampoline function per hook.

---

## Disclaimer
This project was made by two students for fun and may contain inappropriate language in file names or comments (mostly addressed). Use with caution in professional environments.
