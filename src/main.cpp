// Copyright 2022 Eugen Hartmann.
// Licensed under the MIT License (MIT).

#include <Windows.h>

#include <format>

#include "d3d11-present-hook.h"
#include "d3d11-renderer.h"
#include "d3d12-present-hook.h"
#include "d3d12-renderer.h"

#include "black-box-dx-window.h"

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

template<class RendererT, class HookT>
int WithAppWindow(std::wstring executableName) {
    std::wstring outputFolder = L"";
    LPCWSTR testWindowTitle = L"DirectX Black Box Window";
    BlackBoxDXWindow bbWindow = PrepareWindow<RendererT>(testWindowTitle);

    HWND targetWindowHandle = getWindowHandleByTitle(executableName.c_str());

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
        WithAppWindow<D3D12Renderer, D3D12PresentHook>(executableName):
        WithAppWindow<D3D11Renderer, D3D11PresentHook>(executableName);
}
