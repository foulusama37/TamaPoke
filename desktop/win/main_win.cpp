// Ventana nativa de Windows para el simulador: el firmware corriendo de verdad,
// con el raton haciendo de dedo. Sin dependencias: solo user32/gdi32, que ya
// vienen con w64devkit.
//
// Raton  : pulsar/arrastrar/soltar = tocar/deslizar/levantar
// Teclado: ESC salir | ESPACIO pausa | G guarda captura (PPM)
//          1..9 velocidad del reloj (1 = tiempo real, 9 = 9x)
// Consola: se escribe en la ventana de texto y se pulsa Enter, en cualquier
//          momento (SPEC 25, LVL 30, EGGS, STATS, WIPE...). La lee un hilo
//          aparte; leerla desde el handler de teclado congelaba la ventana.
#include <windows.h>

#include <cstdio>
#include <cstring>
#include <cmath>
#include <string>

#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <TouchDrvCSTXXX.hpp>

void setup();
void loop();
void mockSdRoot(const char *path);

#define LADO 466
static int g_zoom = 1;
static uint32_t *g_bgra = nullptr;   // conversion RGB565 -> BGRA para GDI
static uint8_t *g_dentro = nullptr;  // mascara del circulo, calculada una vez
static Arduino_Canvas *g_canvas = nullptr;
static bool g_pausa = false;
static int g_vel = 1;
static uint32_t g_shots = 0;
static bool g_consola = false;
static std::string g_linea;

// La pantalla de la placa es REDONDA: lo que cae fuera del circulo no existe.
// Pintarlo como bisel en vez de dejar la esquina a la vista es lo que hace que
// el simulador avise de verdad cuando algo se sale por el borde curvo.
#define BISEL 0x141820
#define ARO   0x3A4050

// Consola serie en un hilo aparte. Antes se leia stdin desde el handler de
// teclado, lo que bloqueaba el bucle de mensajes: la ventana se congelaba
// hasta escribir algo. Ahora se puede teclear en cualquier momento.
static CRITICAL_SECTION g_cs;
static std::string g_pendiente;
static DWORD WINAPI hiloConsola(LPVOID) {
  char linea[160];
  while (fgets(linea, sizeof(linea), stdin)) {
    std::string s(linea);
    while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) s.pop_back();
    if (s.empty()) continue;
    EnterCriticalSection(&g_cs);
    g_pendiente = s;
    LeaveCriticalSection(&g_cs);
  }
  return 0;
}
static void drenarConsola() {
  std::string s;
  EnterCriticalSection(&g_cs);
  s.swap(g_pendiente);
  LeaveCriticalSection(&g_cs);
  if (!s.empty()) mockSerialPush(s);
}

// el firmware llama a gfx->flush(); aqui solo se guarda el puntero al lienzo
void simOnFlush(Arduino_Canvas *c) { g_canvas = c; }

// dibuja la linea de consola encima de todo, para no tener que cambiar de
// ventana ni adivinar si esta escuchando
static void pintarConsola(HWND hwnd, HDC hdc) {
  if (!g_consola) return;
  RECT rc;
  GetClientRect(hwnd, &rc);
  RECT caja = {0, rc.bottom - 30, rc.right, rc.bottom};
  HBRUSH fondo = CreateSolidBrush(RGB(20, 22, 28));
  FillRect(hdc, &caja, fondo);
  DeleteObject(fondo);
  SetBkMode(hdc, TRANSPARENT);
  SetTextColor(hdc, RGB(150, 235, 160));
  std::string s = "serial> " + g_linea + "_";
  TextOutA(hdc, 8, rc.bottom - 24, s.c_str(), (int)s.size());
}

static void pintar(HWND hwnd, HDC hdc) {
  if (!g_canvas) return;
  const uint16_t *fb = g_canvas->getFramebuffer();
  if (!fb) return;
  for (int i = 0; i < LADO * LADO; i++) {
    if (g_dentro[i] != 1) { g_bgra[i] = g_dentro[i] == 2 ? ARO : BISEL; continue; }
    uint16_t p = fb[i];
    uint32_t r = (p >> 11) & 0x1F, g = (p >> 5) & 0x3F, b = p & 0x1F;
    g_bgra[i] = ((r << 3 | r >> 2) << 16) | ((g << 2 | g >> 4) << 8) | (b << 3 | b >> 2);
  }
  BITMAPINFO bi = {};
  bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bi.bmiHeader.biWidth = LADO;
  bi.bmiHeader.biHeight = -LADO;  // negativo = primera fila arriba
  bi.bmiHeader.biPlanes = 1;
  bi.bmiHeader.biBitCount = 32;
  bi.bmiHeader.biCompression = BI_RGB;
  RECT rc;
  GetClientRect(hwnd, &rc);
  StretchDIBits(hdc, 0, 0, rc.right, rc.bottom, 0, 0, LADO, LADO, g_bgra, &bi,
                DIB_RGB_COLORS, SRCCOPY);
}

// del cliente (posiblemente escalado) a coordenadas de la pantalla redonda
static void aCoords(HWND hwnd, LPARAM lp, int16_t *x, int16_t *y) {
  RECT rc;
  GetClientRect(hwnd, &rc);
  int cx = LOWORD(lp), cy = HIWORD(lp);
  *x = (int16_t)(rc.right ? cx * LADO / rc.right : cx);
  *y = (int16_t)(rc.bottom ? cy * LADO / rc.bottom : cy);
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
  int16_t x, y;
  switch (msg) {
    case WM_LBUTTONDOWN:
      aCoords(hwnd, lp, &x, &y);
      mockTouchSet(x, y, true);
      SetCapture(hwnd);
      return 0;
    case WM_MOUSEMOVE:
      if (wp & MK_LBUTTON) { aCoords(hwnd, lp, &x, &y); mockTouchSet(x, y, true); }
      return 0;
    case WM_LBUTTONUP:
      aCoords(hwnd, lp, &x, &y);
      mockTouchSet(x, y, false);
      ReleaseCapture();
      return 0;
    // Con la consola abierta NINGUNA tecla es atajo: si no, escribir "SPEC 25"
    // dispara el espacio (pausa) y los digitos (velocidad). Justo lo que
    // pasaba antes, y por eso parecia que se quedaba pausado.
    case WM_CHAR:
      if (!g_consola) {
        if (wp == '/' || wp == '\r') { g_consola = true; g_linea.clear(); }
        return 0;
      }
      if (wp == '\r') {
        if (!g_linea.empty()) { mockSerialPush(g_linea); printf("> %s\n", g_linea.c_str()); }
        g_linea.clear();
        g_consola = false;
      } else if (wp == '\b') {
        if (!g_linea.empty()) g_linea.pop_back();
      } else if (wp == 27) {
        g_linea.clear();
        g_consola = false;
      } else if (wp >= 32 && wp < 127 && g_linea.size() < 40) {
        g_linea.push_back((char)wp);
      }
      return 0;

    case WM_KEYDOWN:
      if (g_consola) return 0;  // la consola se lleva todo el teclado
      if (wp == VK_ESCAPE) PostQuitMessage(0);
      else if (wp == VK_SPACE) { g_pausa = !g_pausa; printf("[sim] %s\n", g_pausa ? "pausa" : "sigue"); }
      else if (wp >= '1' && wp <= '9') { g_vel = (int)(wp - '0'); printf("[sim] velocidad x%d\n", g_vel); }
      else if (wp == 'G' && g_canvas) {
        char p[128];
        snprintf(p, sizeof(p), "captura_%02u.ppm", ++g_shots);
        g_canvas->savePPM(p);
        printf("[sim] guardado %s\n", p);
      }
      return 0;
    case WM_PAINT: {
      PAINTSTRUCT ps;
      HDC hdc = BeginPaint(hwnd, &ps);
      pintar(hwnd, hdc);
      pintarConsola(hwnd, hdc);
      EndPaint(hwnd, &ps);
      return 0;
    }
    case WM_ERASEBKGND: return 1;  // sin parpadeo: se repinta entero igual
    case WM_DESTROY: PostQuitMessage(0); return 0;
  }
  return DefWindowProc(hwnd, msg, wp, lp);
}

int main(int argc, char **argv) {
  std::string sd = "../tools/sdcard";
  for (int i = 1; i < argc; i++) {
    std::string a = argv[i];
    if (a == "--sd" && i + 1 < argc) sd = argv[++i];
    else if (a == "--zoom" && i + 1 < argc) g_zoom = atoi(argv[++i]);
  }
  if (g_zoom < 1) g_zoom = 1;
  mockSdRoot(sd.c_str());
  g_bgra = (uint32_t *)malloc(sizeof(uint32_t) * LADO * LADO);

  // mascara del circulo: 1 pantalla, 2 aro, 0 bisel
  g_dentro = (uint8_t *)malloc(LADO * LADO);
  const double R = LADO / 2.0 - 0.5;
  for (int y = 0; y < LADO; y++)
    for (int x = 0; x < LADO; x++) {
      double dx = x - R, dy = y - R, d = sqrt(dx * dx + dy * dy);
      g_dentro[y * LADO + x] = d <= R - 3 ? 1 : (d <= R ? 2 : 0);
    }

  InitializeCriticalSection(&g_cs);
  CreateThread(nullptr, 0, hiloConsola, nullptr, 0, nullptr);

  WNDCLASS wc = {};
  wc.lpfnWndProc = WndProc;
  wc.hInstance = GetModuleHandle(nullptr);
  wc.lpszClassName = "TamaPokeSim";
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
  RegisterClass(&wc);

  RECT rc = {0, 0, LADO * g_zoom, LADO * g_zoom};
  AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
  HWND hwnd = CreateWindow("TamaPokeSim", "TamaPoke - simulador", WS_OVERLAPPEDWINDOW,
                           CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top,
                           nullptr, nullptr, wc.hInstance, nullptr);
  ShowWindow(hwnd, SW_SHOW);

  printf("[sim] SD = %s\n", sd.c_str());
  printf("[sim] raton = dedo | ESC salir | ESPACIO pausa | G captura | 1-9 velocidad\n");
  printf("[sim] consola: pulsa ENTER (o /) EN LA VENTANA, escribe y ENTER otra vez\n");
  mockSetMillis(0);
  setup();

  MSG msg;
  DWORD anterior = GetTickCount();
  for (;;) {
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
      if (msg.message == WM_QUIT) goto fin;
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
    DWORD ahora = GetTickCount();
    DWORD dt = ahora - anterior;
    anterior = ahora;
    if (dt > 100) dt = 100;  // tras un arrastre de ventana no saltar horas de golpe
    drenarConsola();
    if (!g_pausa) {
      // el reloj del firmware avanza con el de verdad (x velocidad), asi las
      // animaciones y el planificador del bicho van a su ritmo real
      for (DWORD t = 0; t < dt * (DWORD)g_vel; t += 16) {
        mockAdvanceMillis(16);
        mockTouchTick();
        loop();
      }
    }
    InvalidateRect(hwnd, nullptr, FALSE);
    Sleep(8);
  }
fin:
  free(g_bgra);
  free(g_dentro);
  DeleteCriticalSection(&g_cs);
  return 0;
}
