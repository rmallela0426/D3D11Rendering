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
#include <vector>
#include <string>

// include the Direct3D Library file
#pragma comment (lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

#define NUM_RGB_INPUT_FILES 2
#define WIDTH 1920
#define HEIGHT 1080

// Initialization of Variables
ID3D11Device* device = nullptr;
ID3D11DeviceContext* context = nullptr;
IDXGISwapChain1* swapchain = nullptr;
IDXGIFactory2* factory = nullptr;
std::vector<ID3D11Texture2D*> surfaces(NUM_RGB_INPUT_FILES, nullptr);
int readIdx = 0;

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
  desc.Height = HEIGHT;
  desc.Width = WIDTH;
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

  // Create staging textures for copying the data from a file
  // texture descriptor that specifics the properties for that texture
  D3D11_TEXTURE2D_DESC tdesc = { 0 };
  tdesc.Width = WIDTH;
  tdesc.Height = HEIGHT;
  tdesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  tdesc.MipLevels = 1;
  tdesc.ArraySize = 1;
  tdesc.SampleDesc.Count = 1;
  tdesc.SampleDesc.Quality = 0;
  tdesc.Usage = D3D11_USAGE_STAGING;
  tdesc.BindFlags = 0;
  tdesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;

  for (int i = 0; i < NUM_RGB_INPUT_FILES; i++) {
    std::string filename = "File" + std::to_string(i+1) + ".rgba";
    FILE* fp = NULL;
    fopen_s(&fp, filename.c_str(), "rb");
    if (fp == NULL) {
      std::cout << "Failed to open File1.rgba";
      return false;
    }

    hr = device->CreateTexture2D(&tdesc, NULL, &surfaces[i]);
    if (FAILED(hr)) {
      std::cout << "Failed while creating staging texture hr:" << hr;
      return false;
    }

    D3D11_MAPPED_SUBRESOURCE sr;
    hr = context->Map(surfaces[i], 0, D3D11_MAP_WRITE, 0, &sr);
    if (FAILED(hr)) {
      std::cout << "Failed to Map the stagting texture hr: " << hr;
      return false;
    }

    // Copy the file data to the buffer
    int numbytes = fread(sr.pData, sizeof(char), tdesc.Width * tdesc.Height * 4, fp);

    context->Unmap(surfaces[i], 0);

    fclose(fp);
  }

  // Initialize the read Index to 0. This is used for accessign the file to copy into
  // back buffer. Once the index reach the NUM_RGB_INPUT_FILES, it will wrap back to 0
  readIdx = 0;

  return true;
}

// Function to deinitialize and clean up Direct3D
void DeInit()
{
  if(swapchain) {
    swapchain->Release();
    swapchain = NULL;
  }

  for (ID3D11Texture2D* & tex : surfaces) {
      tex->Release();
      tex = nullptr;
  }

  if (device) {
    device->Release();
    device = NULL;
  }
  if (context) {
    context->Release();
    context = NULL;
  }
  if (factory) {
    factory->Release();
    factory = NULL;
  }

  return;
}

// Function to Render a frame to window
void Render()
{
  // Getting the address of BackBuffer
  ID3D11Texture2D* backbuffer;
  HRESULT hr = swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backbuffer);
  if (FAILED(hr)) {
    std::cout << "Failed to get the backbuffer with error hr:" << hr;
    return;
  }

  readIdx = readIdx % NUM_RGB_INPUT_FILES;

  context->CopyResource(backbuffer, surfaces[readIdx++]);
  // switch the back buffer and the front buffer
  hr = swapchain->Present(0, 0);
  if (FAILED(hr)) {
      std::cout << "Swapchain of present failed with error hr:" << hr;
  }
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
  RECT wr = { 0, 0, WIDTH, HEIGHT};
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

  // Initialize Direct3D11 & Swapchain objects
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
