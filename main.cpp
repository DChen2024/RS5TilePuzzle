#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>
#include <math.h>
#include <d2d1.h>
#pragma comment(lib, "d2d1") /* Graphics */

#include "basewin.hpp"
#include "resource.h"

static constexpr float BOARD = 2.f/3;
static constexpr float TILES = 15.f/16;
static constexpr float INFOS = 1.f/9;
static constexpr float SPACE = 1.f/16;

template <class T> void SafeRelease(T** ppT) {
  if (*ppT != NULL) {
    (*ppT)->Release();
    *ppT = NULL;
  }
}

class DPIScale {
  static float scale;

public:
  static void Initialize(HWND hwnd) {
    float dpi = GetDpiForWindow(hwnd);
    scale = dpi/96.0f;
  }

  static float PixelsToDips(float f) {
    return f/scale;
  }

  static D2D1_RECT_F PixelsToDips(float left, float top, float right, float bottom) {
    return D2D1::RectF(left/scale, top/scale, right/scale, bottom/scale);
  }
};
float DPIScale::scale;

class MainWindow : public BaseWindow<MainWindow> {
  ID2D1Factory*               pFactory;
  ID2D1HwndRenderTarget*      pRenderTarget;
  ID2D1SolidColorBrush*       pSolidColorBrush;
  UINT16                      b[4];
  UINT16                      board, flags;
  SHORT                       w, h;

  HRESULT CreateGraphicsResources();
  void    DiscardGraphicsResources();
  void    OnClose();
  void    OnCommand(WPARAM wParam, LPARAM lParam);
  void    OnCreate(LPARAM lParam);
  void    OnPaint();
  void    OnSize(WPARAM wParam, LPARAM lParam);

public:
  MainWindow() : pFactory(), pRenderTarget(), pSolidColorBrush(), b(), board(), flags(), w(), h() {}

  LPCTSTR ClassName() const { return TEXT("Window Class"); }
  LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
};

HRESULT MainWindow::CreateGraphicsResources() {
  HRESULT hr = S_OK;
  if (pRenderTarget == NULL) {
    D2D1_SIZE_U size = D2D1::SizeU(w, h);
    hr = pFactory->CreateHwndRenderTarget(
      D2D1::RenderTargetProperties(),
      D2D1::HwndRenderTargetProperties(m_hwnd, size),
      &pRenderTarget
    );

    if (SUCCEEDED(hr)) {
      D2D1_COLOR_F color = D2D1::ColorF(D2D1::ColorF::SkyBlue);
      hr = pRenderTarget->CreateSolidColorBrush(color, &pSolidColorBrush);
    }
  }
  return hr;
}

void MainWindow::DiscardGraphicsResources() {
  SafeRelease(&pSolidColorBrush);
  SafeRelease(&pRenderTarget);
}

void MainWindow::OnClose() {
  if (!(~flags & 0x0003)) {
    if (MessageBox(m_hwnd, TEXT("You found the hidden Easter egg!"), TEXT("My application"), MB_OKCANCEL) == IDOK)
      DestroyWindow(m_hwnd);
  }
  else if (MessageBox(m_hwnd, TEXT("Are you sure you want to quit?"), TEXT("My application"), MB_OKCANCEL) == IDOK)
    if (MessageBox(m_hwnd, TEXT("Really quit?"), TEXT("My application"), MB_OKCANCEL) == IDOK)
      DestroyWindow(m_hwnd);
}

void MainWindow::OnCommand(WPARAM wParam, LPARAM lParam) {
  switch (LOWORD(wParam)) {
  case 0:
    board ^= 1<<0 | 1<<1 | 1<<3;
  break;
  case 1:
    board ^= 1<<0 | 1<<1 | 1<<2 | 1<<4;
  break;
  case 2:
    board ^= 1<<1 | 1<<2 | 1<<5;
  break;
  case 3:
    board ^= 1<<0 | 1<<3 | 1<<4 | 1<<6;
  break;
  case 4:
    board ^= 1<<1 | 1<<3 | 1<<4 | 1<<5 | 1<<7;
  break;
  case 5:
    board ^= 1<<2 | 1<<4 | 1<<5 | 1<<8;
  break;
  case 6:
    board ^= 1<<3 | 1<<6 | 1<<7;
  break;
  case 7:
    board ^= 1<<4 | 1<<6 | 1<<7 | 1<<8;
  break;
  case 8:
    board ^= 1<<5 | 1<<7 | 1<<8;
  break;
  }
  for (int k = 0; k < 4; ++k)
    if (board == b[k])
      flags |= 1<<k+12;
  if (!(~flags & 0xF000)) { // puzzle complete
    static auto EnumChildProc = [](HWND hwnd, LPARAM lParam) -> BOOL {
      DestroyWindow(hwnd);
      return TRUE;
    };
    EnumChildWindows(m_hwnd, EnumChildProc, NULL);
  }
  InvalidateRect(m_hwnd, NULL, FALSE);
  // easter egg
  if (LOWORD(wParam) != 4)
    flags &= ~0x000F;
  else if (~flags & 0x000F)
    flags += 1;
}

void MainWindow::OnCreate(LPARAM lParam) {
  HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtr(m_hwnd, GWLP_HINSTANCE);
  SetClassLongPtr(m_hwnd, GCLP_HICON, (LONG_PTR)LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1)));
  SetClassLongPtr(m_hwnd, GCLP_HCURSOR, (LONG_PTR)LoadCursor(NULL, IDC_ARROW));
  for (int i = 0; i < 9; ++i) {
    CreateWindowEx(WS_EX_LAYERED, TEXT("BUTTON"), NULL,
      WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
      CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
      m_hwnd, (HMENU)i, hInstance, NULL
    );
  }
  static auto EnumChildProc = [](HWND hwnd, LPARAM lParam) -> BOOL {
    SetLayeredWindowAttributes(hwnd, RGB(255, 255, 255), 0, LWA_COLORKEY); // transparent button
    return TRUE;
  };
  EnumChildWindows(m_hwnd, EnumChildProc, NULL);
  DPIScale::Initialize(m_hwnd);
  b[0] = 1<<1 | 1<<6 | 1<<7;
  b[1] = 1<<3 | 1<<4 | 1<<6 | 1<<7;
  b[2] = 1<<4;
  b[3] = 1<<2 | 1<<3 | 1<<5;
}

void MainWindow::OnPaint() {
  HRESULT hr = CreateGraphicsResources();
  if (FAILED(hr))
    return;

  PAINTSTRUCT ps;
  BeginPaint(m_hwnd, &ps);
  pRenderTarget->BeginDraw();
  pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::SkyBlue));

  // large border
  const int n = min(w, h);
  float x = w/2.f;
  float y = h/2.f;
  float r = BOARD/2*n;
  D2D1_RECT_F rect = DPIScale::PixelsToDips(x-r, y-r, x+r, y+r);
  D2D1_COLOR_F color = D2D1::ColorF(D2D1::ColorF::Black);
  pSolidColorBrush->SetColor(color);
  pRenderTarget->FillRectangle(&rect, pSolidColorBrush);

  // large board
  float p[4*3];
  float _r = TILES/3*r;
  for (int k = 0; k < 3; ++k) {
    float _x = x+(3+TILES)/(3+1)*BOARD/3*n*(k-(3-1)/2.f);
    float _y = y+(3+TILES)/(3+1)*BOARD/3*n*(k-(3-1)/2.f);
    p[k*4+0] = _x-_r;
    p[k*4+1] = _y-_r;
    p[k*4+2] = _x+_r;
    p[k*4+3] = _y+_r;
  }
  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j) {
      rect = DPIScale::PixelsToDips(p[j*4+0], p[i*4+1], p[j*4+2], p[i*4+3]);
      if (!(~flags & 0xF000))
        color = D2D1::ColorF(D2D1::ColorF::Gray);
      else if (board & 1<<i*3+j)
        color = D2D1::ColorF(D2D1::ColorF::Blue);
      else
        color = D2D1::ColorF(D2D1::ColorF::Red);
      pSolidColorBrush->SetColor(color);
      pRenderTarget->FillRectangle(&rect, pSolidColorBrush);
    }
  }

  // small border and board
  y = h/2.f+(BOARD+1)/4*n;
  r = INFOS*BOARD/2*n;
  _r = TILES/3*r;
  for (int i = 0; i < 3; ++i) {
    float _y = y+(3+TILES)/(3+1)*INFOS*BOARD/3*n*(i-(3-1)/2.f);
    p[i*4+1] = _y-_r;
    p[i*4+3] = _y+_r;
  }
  for (int k = 0; k < 4; ++k) {
    x = w/2.f+(SPACE+1)*INFOS*BOARD*n*(k-(4-1)/2.f);
    rect = DPIScale::PixelsToDips(x-r, y-r, x+r, y+r);
    color = D2D1::ColorF(D2D1::ColorF::Black);
    pSolidColorBrush->SetColor(color);
    pRenderTarget->FillRectangle(&rect, pSolidColorBrush);
    for (int j = 0; j < 3; ++j) {
      float _x = x+(3+TILES)/(3+1)*INFOS*BOARD/3*n*(j-(3-1)/2.f);
      p[j*4+0] = _x-_r;
      p[j*4+2] = _x+_r;
    }
    for (int i = 0; i < 3; ++i) {
      for (int j = 0; j < 3; ++j) {
        rect = DPIScale::PixelsToDips(p[j*4+0], p[i*4+1], p[j*4+2], p[i*4+3]);
        if (b[k] & 1<<i*3+j)
          color = D2D1::ColorF(D2D1::ColorF::Blue);
        else if (flags & 1<<k+12)
          color = D2D1::ColorF(D2D1::ColorF::Red);
        else
          color = D2D1::ColorF(D2D1::ColorF::DarkRed);
        pSolidColorBrush->SetColor(color);
        pRenderTarget->FillRectangle(&rect, pSolidColorBrush);
      }
    }
  }

  hr = pRenderTarget->EndDraw();
  if (hr == D2DERR_RECREATE_TARGET)
    DiscardGraphicsResources();
  EndPaint(m_hwnd, &ps);
}

void MainWindow::OnSize(WPARAM wParam, LPARAM lParam) {
  w = LOWORD(lParam);
  h = HIWORD(lParam);
  const float n = min(w, h);
  const float r = BOARD/2*TILES/3*n;
  short p[4*3];
  for (int k = 0; k < 3; ++k) {
    float x = w/2.f+(3+TILES)/(3+1)*BOARD/3*n*(k-(3-1)/2.f);
    float y = h/2.f+(3+TILES)/(3+1)*BOARD/3*n*(k-(3-1)/2.f);
    p[k*4+0] = roundf(x-r);
    p[k*4+1] = roundf(y-r);
    p[k*4+2] = roundf(x+r);
    p[k*4+3] = roundf(y+r);
  }

  static auto EnumChildProc = [](HWND hwnd, LPARAM lParam) -> BOOL {
    int k = GetWindowLongPtr(hwnd, GWLP_ID);
    int i = k/3, j = k%3;
    short* p = (short*)lParam;
    short left = p[j*4+0];
    short top = p[i*4+1];
    short right = p[j*4+2];
    short bottom = p[i*4+3];
    MoveWindow(hwnd, left, top, right-left, bottom-top, FALSE);
    return TRUE;
  };
  EnumChildWindows(m_hwnd, EnumChildProc, (LPARAM)p);
  InvalidateRect(m_hwnd, NULL, FALSE);

  if (pRenderTarget == NULL)
    return;

  D2D1_SIZE_U size = D2D1::SizeU(w, h);
  pRenderTarget->Resize(size);
}

int WINAPI _tWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPTSTR pCmdLine, _In_ int nCmdShow) {
  MainWindow win;

  if (!win.Create(TEXT("Riddle School 5: Tile Puzzle"), WS_OVERLAPPEDWINDOW))
    return 0;
  ShowWindow(win.Window(), nCmdShow);

  MSG msg;
  while (GetMessage(&msg, NULL, 0, 0) != 0) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  return 0;
}

LRESULT MainWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {
  case WM_CLOSE:
    OnClose();
  break;
  case WM_COMMAND:
    OnCommand(wParam, lParam);
    SetFocus(m_hwnd);
  break;
  case WM_CREATE:
    if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pFactory)))
      return -1;
    OnCreate(lParam);
  break;
  case WM_DESTROY:
    DiscardGraphicsResources();
    SafeRelease(&pFactory);
    PostQuitMessage(0);
  break;
  case WM_GETMINMAXINFO: {
    LPMINMAXINFO lpMMI = (LPMINMAXINFO)lParam;
    lpMMI->ptMinTrackSize.x = 300;
    lpMMI->ptMinTrackSize.y = 300;
  }
  break;
  case WM_PAINT:
    OnPaint();
  break;
  case WM_SIZE:
    OnSize(wParam, lParam);
  break;
  default:
    return DefWindowProc(m_hwnd, uMsg, wParam, lParam); // default window procedure
  break;
  }
  return 0;
}
