// Reimplementacion minima de Arduino_GFX para el simulador.
//
// Por que no reusar la libreria real: su Arduino_DataBus.h arrastra headers del
// ESP-IDF (soc/*, hal/*, esp_lcd_*) y el include va entre comillas, asi que
// Arduino_GFX.cpp siempre encuentra el suyo antes que cualquier stub. Sale mas
// barato reimplementar los ~20 primitivos que usa el firmware.
//
// Lo que SI se conserva identico: la tabla de la fuente 5x7 (extraida de la
// propia libreria) y el formato RGB565, para que texto y colores caigan en los
// mismos pixeles que en la placa.
#pragma once
#include <string>
#include <vector>
#include <Arduino.h>

#include <vector>

#define GFX_NOT_DEFINED -1

#define RGB565_BLACK 0x0000
#define RGB565_WHITE 0xFFFF
#define RGB565_RED 0xF800
#define RGB565_GREEN 0x07E0
#define RGB565_BLUE 0x001F
#define RGB565_YELLOW 0xFFE0
#define RGB565_CYAN 0x07FF
#define RGB565_MAGENTA 0xF81F
#define RGB565(r, g, b) ((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3))

class Arduino_DataBus {};
class Arduino_ESP32QSPI : public Arduino_DataBus {
public:
  Arduino_ESP32QSPI(int, int, int, int, int, int) {}
};
class Arduino_CO5300 {
public:
  Arduino_CO5300(Arduino_DataBus *, int, int, int, int, int, int, int, int) {}
  void setBrightness(uint8_t b) { _bright = b; }
  uint8_t brightness() const { return _bright; }

private:
  uint8_t _bright = 255;
};

// Las de verdad, no un typedef vacio: el firmware ya no dibuja con la 5x7
// sino con una GFXfont propia (font_redonda.h), y si el simulador la
// ignorase las capturas enseñarian una pantalla que no existe.
typedef struct {
  uint16_t bitmapOffset;
  uint8_t width, height;
  uint8_t xAdvance;
  int8_t xOffset, yOffset;
} GFXglyph;

typedef struct {
  uint8_t *bitmap;
  GFXglyph *glyph;
  uint16_t first, last;
  uint8_t yAdvance;
} GFXfont;

class Arduino_Canvas : public Print {
public:
  Arduino_Canvas(int16_t w, int16_t h, void *output);
  ~Arduino_Canvas();

  bool begin(int32_t speed = GFX_NOT_DEFINED);
  uint16_t *getFramebuffer() { return _fb; }
  int16_t width() const { return _w; }
  int16_t height() const { return _h; }

  void flush();  // en el simulador: cuenta un frame y, si toca, lo vuelca a disco

  void fillScreen(uint16_t color);
  void drawPixel(int16_t x, int16_t y, uint16_t color);
  void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
  // vuelca un bloque RGB565 tal cual: el fondo de combate que viene de la SD
  void draw16bitRGBBitmap(int16_t x, int16_t y, uint16_t *bitmap, int16_t w, int16_t h);
  void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
  void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
  void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
  void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
  void fillCircle(int16_t x, int16_t y, int16_t r, uint16_t color);
  void drawCircle(int16_t x, int16_t y, int16_t r, uint16_t color);
  void fillEllipse(int16_t x, int16_t y, int16_t rx, int16_t ry, uint16_t color);
  void fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color);
  void drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color);
  void fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2,
                    uint16_t color);

  void setFont(const GFXfont *f = nullptr) { _font = f; }
  void setUTF8Print(bool b) { (void)b; }
  void setTextSize(uint8_t s) { _size = s ? s : 1; }
  void setTextColor(uint16_t c) { _fg = c; _bg = c; _opaqueBg = false; }
  void setTextColor(uint16_t c, uint16_t bg) { _fg = c; _bg = bg; _opaqueBg = true; }
  // Deja de ser inline: cada setCursor CIERRA el run de texto anterior,
  // que es lo que el metro del texto usa para saber donde empieza y
  // acaba cada frase (ver mockTextoFuera).
  void setCursor(int16_t x, int16_t y);
  int16_t getCursorX() const { return _cx; }
  int16_t getCursorY() const { return _cy; }
  void getTextBounds(const char *s, int16_t x, int16_t y, int16_t *x1, int16_t *y1,
                     uint16_t *w, uint16_t *h);
  void getTextBounds(const String &s, int16_t x, int16_t y, int16_t *x1, int16_t *y1,
                     uint16_t *w, uint16_t *h) {
    getTextBounds(s.c_str(), x, y, x1, y1, w, h);
  }
  size_t write(uint8_t c) override;

  // --- solo del simulador ---
  uint32_t frames() const { return _frames; }
  void cierraRun();        // cierra el run de texto en curso
  void savePPM(const char *path) const;

private:
  void drawChar(int16_t x, int16_t y, unsigned char c);
  const GFXfont *_font = nullptr;
  int16_t _w, _h;
  uint16_t *_fb = nullptr;
  uint16_t _fg = 0xFFFF, _bg = 0;
  bool _opaqueBg = false;
  uint8_t _size = 1;
  int16_t _cx = 0, _cy = 0;
  uint32_t _frames = 0;
  // el run de texto en curso, para el metro del texto
  std::string _run;
  int16_t _runX = 0, _runY = 0;
  bool _runMal = false;
  int _runLejos = 0;       // cuanto se sale, en pixeles
};

// ---- EL METRO DEL TEXTO (solo del simulador) ----
//
// Las cadenas cambian de largo con el idioma —— el aleman hace palabras
// del doble que el ingles —— y no hay forma humana de mirar 110 frases
// por 8 idiomas en cada pantalla. Esto apunta cada letra que se pinta y
// dice cual se sale del panel redondo.
struct TextoSalido {
  std::string texto;   // la frase entera, no una letra suelta
  int x, y;            // donde empezaba
  int lejos;           // cuantos pixeles se sale del circulo
};
void mockTextoReset();
// Todos los textos que se han salido desde el ultimo reset, el que mas
// se sale primero.
const std::vector<TextoSalido> &mockTextoFuera();
// todo o texto escrito desde mockTextoTodoReset() (os bytes, como sairam)
void mockTextoTodoReset();
const std::string &mockTextoTodo();

