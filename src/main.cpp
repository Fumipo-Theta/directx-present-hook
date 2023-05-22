// Copyright 2022 Eugen Hartmann.
// Licensed under the MIT License (MIT).

#include <Windows.h>
#include <TlHelp32.h>
#include <string>
#include <iostream>

#include <format>

#include "d3d11-present-hook.h"
#include "d3d11-renderer.h"
#include "d3d12-present-hook.h"
#include "d3d12-renderer.h"

#include "black-box-dx-window.h"

HWND g_hwnd = NULL;

template<class RendererT>
BlackBoxDXWindow<RendererT> PrepareWindow(std::wstring_view blackBoxDXWindowTitle) {
    HRESULT hr;
    // Create the black box DirectX window.
    BlackBoxDXWindow<RendererT> blackBoxDXWindow{blackBoxDXWindowTitle};
    hr = blackBoxDXWindow.Initialize(1280, 720);
    if (FAILED(hr)) {
        std::wstring message =
            std::format(L"Could not initialize the black box DirectX window. Error: {:#x}.",
                static_cast<unsigned long>(hr));
        MessageBox(NULL, message.data(), L"Error", MB_OK);
    }
    return blackBoxDXWindow;
}

template<class HookT>
int HookAndAttachWindow(HWND targetWindowHandle, const std::wstring& outputFolder) {
  HRESULT hr;

  // Hook!
  hr = HookT::Get()->Hook();
  if (FAILED(hr)) {
    std::wstring message =
      std::format(L"Could not hook. Error: {:#x}.",
        static_cast<unsigned long>(hr));
    MessageBox(NULL, message.data(), L"Error", MB_OK);
    return 1;
  }

  // Set the hook object to capture some frames.
  hr = HookT::Get()->CaptureFrames(targetWindowHandle, outputFolder, 10);
  if (FAILED(hr)) {
    std::wstring message =
      std::format(L"Could not start frame capturing. Error: {:#x}.",
        static_cast<unsigned long>(hr));
    MessageBox(NULL, message.data(), L"Error", MB_OK);
    return 1;
  }

  MSG msg = {};
  while (msg.message != WM_QUIT) {
    if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }

  // Exit.
  return static_cast<char>(msg.wParam);
}

HWND getWindowHandleByTitle(LPCWSTR windowTitle) {
    return  FindWindow(NULL, windowTitle);
}

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    DWORD lpdwProcessId;
    GetWindowThreadProcessId(hwnd, &lpdwProcessId);
    if (lpdwProcessId == lParam)
    {
        std::cout << "Window handle: " << hwnd << std::endl;
        g_hwnd = hwnd;
        return FALSE;
    }
    return TRUE;
}

void GetWindowHandleByPID(DWORD processId) {
    EnumWindows(EnumWindowsProc, processId);
}

DWORD GetPIDByExeName(const std::wstring& exeName) {
    DWORD pid = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap != INVALID_HANDLE_VALUE)
    {
        PROCESSENTRY32 pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32);
        if (Process32First(hSnap, &pe32))
        {
            do
            {
                if (exeName == pe32.szExeFile)
                {
                    pid = pe32.th32ProcessID;
                    break;
                }
            } while (Process32Next(hSnap, &pe32));
        }
        CloseHandle(hSnap);
    }
    return pid;
}


template<class RendererT, class HookT>
int WithAppWindow2(std::wstring executableName) {
    std::wstring outputFolder = L"";
    LPCWSTR testWindowTitle = L"DirectX Black Box Window";
    BlackBoxDXWindow bbWindow = PrepareWindow<RendererT>(testWindowTitle);
    int res = 1;

    //bbWindow.Show(SW_SHOW);

    DWORD pid = 4344; // GetPIDByExeName(executableName);
    std::cout << "PID" << pid << std::endl;
    if (pid != 0) {
        GetWindowHandleByPID(pid);
        if (g_hwnd != NULL) {
            HWND targetWindowHandle = g_hwnd;
            // Show the window.
            

            res = HookAndAttachWindow<HookT>(targetWindowHandle, outputFolder);

           
        }
        else {
            std::cout << "Window handle is not found!" << std::endl;
            return 1;
        }
    }
    else {
        std::cout << "PID is not found!" << std::endl;
        return 1;
    }

    // Unitialize the window.
    //bbWindow.Uninitialize();
    return res;
}

template<class RendererT, class HookT>
int WithAppWindow(std::wstring executableName) {
    std::wstring outputFolder = L"";
    LPCWSTR testWindowTitle = L"DirectX Black Box Window";
    BlackBoxDXWindow bbWindow = PrepareWindow<RendererT>(testWindowTitle);

    HWND targetWindowHandle = getWindowHandleByTitle(testWindowTitle); // getWindowHandleByTitle(executableName.c_str());

    // Show the window.
    bbWindow.Show(SW_SHOW);

    int res = HookAndAttachWindow<HookT>(targetWindowHandle, outputFolder);

    // Unitialize the window.
    bbWindow.Uninitialize();
    return res;
}

int CALLBACK WinMain(_In_ HINSTANCE instanceHandle, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int cmdShow) {
    int directXVersion = 11;
    std::wstring executableName;

    int argCount = 0;
    LPWSTR* argList = CommandLineToArgvW(GetCommandLine(), &argCount);
    if (argList) {
        if (argCount > 1) {
            try {
                directXVersion = std::stoi(argList[1]);
                if (directXVersion != 11 && directXVersion != 12) {
                    std::wstring message =
                        std::format(L"The application only supports DirectX 11 and DirectX 12.");
                    MessageBox(NULL, message.data(), L"Error", MB_OK);
                    return 1;
                }
            }
            catch (const std::exception& e) {
                std::string message =
                    std::format("Could not parse the command line to get the DirectX version: {}",
                        e.what());
                MessageBoxA(NULL, message.data(), "Error", MB_OK);
                return 1;
            }
        }
        if (argCount > 2) {
            executableName = argList[2];
        }
        LocalFree(argList);
    }

    return (directXVersion == 12)?
        WithAppWindow2<D3D12Renderer, D3D12PresentHook>(executableName):
        WithAppWindow2<D3D11Renderer, D3D11PresentHook>(executableName);
}
