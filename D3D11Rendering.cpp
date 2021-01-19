//
// D3D11Rendering.cpp :
// Utility that explains how to render to a window using D3D11.
// As a first step will render a Red color on window. Later will
// use a shader to convert from NV12 to ARGB and render onto
// window.
//

#include <iostream>

// Basic Direct3D header files.
#include <windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <dxgi.h>

// include the Direct3D Library file
#pragma comment (lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

// Initialization of Variables
ID3D11Device* device = nullptr;
ID3D11DeviceContext* context = nullptr;
IDXGISwapChain1* swapchain = nullptr;
IDXGIFactory2* factory = nullptr;
ID3D11RenderTargetView* rtv = nullptr;

// Callback function for WindowProc
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

// Creates D3D11Device and swapchain if window is not null
bool Init(HWND& hwnd)
{
  if (hwnd == NULL)
    return false;

  // DxGI Factory creation that is used to generate other dxgi objects
  HRESULT hr = CreateDXGIFactory1(__uuidof(IDXGIFactory2), (void**)&factory);
  if (FAILED(hr)) {
    std::cout << "Failed to create dxgi factory hr: " << hr;
    return false;
  }

  UINT createFlags = 0;
#ifdef _DEBUG
  createFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

  // Create a device that represents the display adapter
  hr = D3D11CreateDevice(/*IDXGIAdapter* */ NULL,
                         D3D_DRIVER_TYPE_HARDWARE,
                         NULL, createFlags, NULL,
                         0, D3D11_SDK_VERSION,
                         &device, NULL, &context);
  if (FAILED(hr)) {
    std::cout << "Failed to create d3d11 device hr: " << hr;
    return false;
  }

  DXGI_SWAP_CHAIN_DESC1 desc;
  ZeroMemory(&desc, sizeof(DXGI_SWAP_CHAIN_DESC1));
  desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  // Setting width and height to zero so that width & height from the output window
  desc.Height = 0;
  desc.Width = 0;
  desc.Scaling = DXGI_SCALING_STRETCH;
  desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  desc.SampleDesc.Count = 1;    // multisampling setting
  desc.SampleDesc.Quality = 0;  // vendor-specific flag
  desc.BufferCount = 2;
  desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
  desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
  desc.Stereo = false;

  // Creates a swap chain that is associated with an HWND handle to the
  // output window for the swap chain.
  hr = factory->CreateSwapChainForHwnd(
      device, (HWND)hwnd, &desc, NULL, NULL,
      reinterpret_cast<IDXGISwapChain1**>(&swapchain));
  if (FAILED(hr)) {
    std::cout << "Failed to create swapchain with error hr:" << hr;
    return false;
  }

  // Getting the address of BackBuffer
  ID3D11Texture2D* backbuffer;
  hr = swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backbuffer);
  if (FAILED(hr)) {
    std::cout << "Failed to get the backbuffer with error hr:" << hr;
    return false;
  }

  // Creates a render - target view for accessing resource data.
  // use the back buffer address to create the render target
  hr = device->CreateRenderTargetView(backbuffer, NULL, &rtv);
  if (FAILED(hr)) {
    std::cout << "Failed to create rendertargetview with error hr:" << hr;
    return false;
  }

  backbuffer->Release();

  // set the render target as the back buffer
  context->OMSetRenderTargets(1, &rtv, NULL);

  return true;
}

// Function to deinitialize and clean up Direct3D
void DeInit()
{
  swapchain->Release();
  swapchain = NULL;
  rtv->Release();
  rtv = NULL;
  device->Release();
  device = NULL;
  context->Release();
  context = NULL;
  factory->Release();
  factory = NULL;

  return;
}

// Function to Render a frame to window
void Render()
{
  FLOAT color[4] = { 1.0f, 0.0f, 0.0f, 1.0f };

  // clear the back buffer to a red color
  context->ClearRenderTargetView(rtv, color);

  // switch the back buffer and the front buffer
  swapchain->Present(0, 0);
}

// this is the main message handler for the program
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
    case WM_DESTROY:
    {
      PostQuitMessage(0);
      return 0;
    }
    break;
  }

   return DefWindowProc(hWnd, message, wParam, lParam);
}

int main(void)
{
  HWND hWnd;
  WNDCLASS wc;

  ZeroMemory(&wc, sizeof(WNDCLASS));

  wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.hInstance = NULL;
  wc.lpfnWndProc = WindowProc;
  wc.lpszClassName = L"D3DRendering";
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.hbrBackground = (HBRUSH)COLOR_WINDOW;

  if (!RegisterClass(&wc)) {
    std::cout << "Could not register class";
    return false;
  }
  RECT wr = { 0, 0, 800, 600 };
  AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

  // Create a Window
  hWnd = CreateWindow(
      L"D3DRendering",
      L"D3D11 Render Window",
      WS_OVERLAPPEDWINDOW,
      50,
      50,
      wr.right - wr.left,
      wr.bottom - wr.top,
      NULL,
      NULL,
      NULL,
      NULL);
  if (hWnd == NULL) {
    std::cout << "Creation of Window failed";
    return false;
  }
  ShowWindow(hWnd, SW_RESTORE);

  // Initialize Direct3D objects
  if (!Init(hWnd)) {
    DeInit();
    return false;
  }

  // enter the main loop:
  MSG msg;
  while (TRUE)
  {
    if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
      TranslateMessage(&msg);
      DispatchMessage(&msg);

      if (msg.message == WM_QUIT)
        break;
    }
    Render();
  }
  // DeInitialize D3D11 objects
  DeInit();

  // Destroy the window
  DestroyWindow(hWnd);

  return msg.wParam;
}
