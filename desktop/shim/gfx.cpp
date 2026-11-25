#include <string>
#include <vector>
#include <cmath>
#include "Arduino_GFX_Library.h"
#include "glcdfont.h"

Arduino_Canvas::Arduino_Canvas(int16_t w, int16_t h, void *) : _w(w), _h(h) {
  _fb = (uint16_t *)calloc((size_t)w * h, 2);
}
Arduino_Canvas::~Arduino_Canvas() { free(_fb); }
bool Arduino_Canvas::begin(int32_t) { return _fb != nullptr; }

void Arduino_Canvas::drawPixel(int16_t x, int16_t y, uint16_t c) {
  if (x < 0 || y < 0 || x >= _w || y >= _h) return;
  _fb[(int32_t)y * _w + x] = c;
}
void Arduino_Canvas::fillScreen(uint16_t c) {
  for (int32_t i = 0; i < (int32_t)_w * _h; i++) _fb[i] = c;
}
void Arduino_Canvas::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t c) {
  if (w <= 0 || h <= 0) return;
  int x0 = x < 0 ? 0 : x, y0 = y < 0 ? 0 : y;
  int x1 = x + w > _w ? _w : x + w, y1 = y + h > _h ? _h : y + h;
  for (int yy = y0; yy < y1; yy++) {
    uint16_t *line = _fb + (int32_t)yy * _w;
    for (int xx = x0; xx < x1; xx++) line[xx] = c;
  }
}
void Arduino_Canvas::draw16bitRGBBitmap(int16_t x, int16_t y, uint16_t *bm,
                                        int16_t w, int16_t h) {
  if (!bm || w <= 0 || h <= 0) return;
  for (int r = 0; r < h; r++) {
    int yy = y + r;
    if (yy < 0 || yy >= _h) continue;
    for (int c = 0; c < w; c++) {
      int xx = x + c;
      if (xx < 0 || xx >= _w) continue;
      _fb[(int32_t)yy * _w + xx] = bm[(int32_t)r * w + c];
    }
  }
}
void Arduino_Canvas::drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t c) {
  fillRect(x, y, w, 1, c);
}
void Arduino_Canvas::drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t c) {
  fillRect(x, y, 1, h, c);
}
void Arduino_Canvas::drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t c) {
  drawFastHLine(x, y, w, c);
  drawFastHLine(x, y + h - 1, w, c);
  drawFastVLine(x, y, h, c);
  drawFastVLine(x + w - 1, y, h, c);
}
void Arduino_Canvas::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t c) {
  int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  int err = dx + dy;
  for (;;) {
    drawPixel(x0, y0, c);
    if (x0 == x1 && y0 == y1) break;
    int e2 = 2 * err;
    if (e2 >= dy) { err += dy; x0 += sx; }
    if (e2 <= dx) { err += dx; y0 += sy; }
  }
}
void Arduino_Canvas::fillCircle(int16_t cx, int16_t cy, int16_t r, uint16_t c) {
  if (r < 0) return;
  for (int y = -r; y <= r; y++)
    for (int x = -r; x <= r; x++)
      if (x * x + y * y <= r * r) drawPixel(cx + x, cy + y, c);
}
void Arduino_Canvas::drawCircle(int16_t cx, int16_t cy, int16_t r, uint16_t c) {
  if (r < 0) return;
  int r2 = r * r, rin = (r - 1) * (r - 1);
  for (int y = -r; y <= r; y++)
    for (int x = -r; x <= r; x++) {
      int d = x * x + y * y;
      if (d <= r2 && d > rin) drawPixel(cx + x, cy + y, c);
    }
}
void Arduino_Canvas::fillEllipse(int16_t cx, int16_t cy, int16_t rx, int16_t ry, uint16_t c) {
  if (rx <= 0 || ry <= 0) return;
  int32_t rx2 = (int32_t)rx * rx, ry2 = (int32_t)ry * ry;
  for (int y = -ry; y <= ry; y++) {
    int32_t t = (int32_t)y * y * rx2;
    for (int x = -rx; x <= rx; x++)
      if ((int32_t)x * x * ry2 + t <= rx2 * ry2) drawPixel(cx + x, cy + y, c);
  }
}
void Arduino_Canvas::fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r,
                                   uint16_t c) {
  if (w <= 0 || h <= 0) return;
  int16_t mx = (w < h ? w : h) / 2;
  if (r > mx) r = mx;
  if (r <= 0) { fillRect(x, y, w, h, c); return; }
  fillRect(x + r, y, w - 2 * r, h, c);
  fillRect(x, y + r, r, h - 2 * r, c);
  fillRect(x + w - r, y + r, r, h - 2 * r, c);
  int r2 = r * r;
  for (int dy = 0; dy < r; dy++)
    for (int dx = 0; dx < r; dx++) {
      int a = r - dx, b = r - dy;
      if (a * a + b * b > r2) continue;
      drawPixel(x + dx, y + dy, c);
      drawPixel(x + w - 1 - dx, y + dy, c);
      drawPixel(x + dx, y + h - 1 - dy, c);
      drawPixel(x + w - 1 - dx, y + h - 1 - dy, c);
    }
}
void Arduino_Canvas::drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r,
                                   uint16_t c) {
  if (w <= 0 || h <= 0) return;
  int16_t mx = (w < h ? w : h) / 2;
  if (r > mx) r = mx;
  drawFastHLine(x + r, y, w - 2 * r, c);
  drawFastHLine(x + r, y + h - 1, w - 2 * r, c);
  drawFastVLine(x, y + r, h - 2 * r, c);
  drawFastVLine(x + w - 1, y + r, h - 2 * r, c);
  int r2 = r * r, rin = (r - 1) * (r - 1);
  for (int dy = 0; dy < r; dy++)
    for (int dx = 0; dx < r; dx++) {
      int a = r - dx, b = r - dy, d = a * a + b * b;
      if (d > r2 || d <= rin) continue;
      drawPixel(x + dx, y + dy, c);
      drawPixel(x + w - 1 - dx, y + dy, c);
      drawPixel(x + dx, y + h - 1 - dy, c);
      drawPixel(x + w - 1 - dx, y + h - 1 - dy, c);
    }
}
void Arduino_Canvas::fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2,
                                  int16_t y2, uint16_t c) {
  int minx = min(x0, min(x1, x2)), maxx = max(x0, max(x1, x2));
  int miny = min(y0, min(y1, y2)), maxy = max(y0, max(y1, y2));
  auto lado = [](int ax, int ay, int bx, int by, int px, int py) {
    return (bx - ax) * (py - ay) - (by - ay) * (px - ax);
  };
  for (int y = miny; y <= maxy; y++)
    for (int x = minx; x <= maxx; x++) {
      int d1 = lado(x0, y0, x1, y1, x, y);
      int d2 = lado(x1, y1, x2, y2, x, y);
      int d3 = lado(x2, y2, x0, y0, x, y);
      bool neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
      bool pos = (d1 > 0) || (d2 > 0) || (d3 > 0);
      if (!(neg && pos)) drawPixel(x, y, c);
    }
}

// ---------------------------------------------------------------- texto
// Con una GFXfont puesta, el glifo va bit a bit desde su bitmap y la Y es
// la LINEA BASE (yOffset es negativo). Es el mismo trato que le da
// Adafruit_GFX, que es de donde el firmware saca su `gFontAscent`.
static const GFXglyph *glifoDe(const GFXfont *f, unsigned char ch) {
  if (!f || ch < f->first || ch > f->last) return nullptr;
  return &f->glyph[ch - f->first];
}

// ---- el metro del texto ----
// El panel es un circulo de 466: lo que caiga fuera no existe para quien
// lo mira. Se comparan los pixeles ENCENDIDOS, no la caja de la letra,
// porque la celda de la fuente lleva columnas en blanco.
static const int TXT_CX = 233, TXT_CY = 233, TXT_R = 233;
static std::vector<TextoSalido> gSalidos;

void mockTextoReset() { gSalidos.clear(); }
// tudo o que se escreveu na tela desde o ultimo reset (leva 73: o teste
// que ve se a tela em portugues ainda tem texto em ingles)
static std::string gTextoTodo;
void mockTextoTodoReset() { gTextoTodo.clear(); }
const std::string &mockTextoTodo() { return gTextoTodo; }
const std::vector<TextoSalido> &mockTextoFuera() { return gSalidos; }

// cuanto se sale del circulo un pixel (0 = dentro)
static int fueraDelCirculo(int x, int y) {
  int dx = x - TXT_CX, dy = y - TXT_CY;
  double d = sqrt((double)(dx * dx + dy * dy));
  int f = (int)(d - TXT_R + 0.5);
  return f > 0 ? f : 0;
}

void Arduino_Canvas::setCursor(int16_t x, int16_t y) {
  cierraRun();
  _cx = x;
  _cy = y;
  _runX = x;
  _runY = y;
}

void Arduino_Canvas::cierraRun() {
  if (_runMal && !_run.empty()) {
    TextoSalido t;
    t.texto = _run;
    t.x = _runX;
    t.y = _runY;
    t.lejos = _runLejos;
    // el que mas se sale, delante
    auto it = gSalidos.begin();
    while (it != gSalidos.end() && it->lejos >= t.lejos) ++it;
    gSalidos.insert(it, t);
  }
  _run.clear();
  _runMal = false;
  _runLejos = 0;
}

void Arduino_Canvas::drawChar(int16_t x, int16_t y, unsigned char ch) {
  if (_font) {
    const GFXglyph *g = glifoDe(_font, ch);
    if (!g || !g->width || !g->height) return;
    const uint8_t *bm = _font->bitmap + g->bitmapOffset;
    uint16_t bit = 0;
    for (uint8_t yy = 0; yy < g->height; yy++) {
      for (uint8_t xx = 0; xx < g->width; xx++, bit++) {
        if (bm[bit >> 3] & (0x80 >> (bit & 7))) {
          int px = x + (g->xOffset + xx) * _size;
          int py = y + (g->yOffset + yy) * _size;
          fillRect(px, py, _size, _size, _fg);
          int f = fueraDelCirculo(px, py);
          int f2 = fueraDelCirculo(px + _size - 1, py + _size - 1);
          if (f2 > f) f = f2;
          if (f > 0) { _runMal = true; if (f > _runLejos) _runLejos = f; }
        }
      }
    }
    return;
  }
  for (int8_t col = 0; col < 6; col++) {
    uint8_t bits = (col < 5) ? GLCDFONT[(int)ch * 5 + col] : 0;
    for (int8_t row = 0; row < 8; row++) {
      bool on = bits & (1 << row);
      if (on) {
        int px = x + col * _size, py = y + row * _size;
        fillRect(px, py, _size, _size, _fg);
        int f = fueraDelCirculo(px, py);
        int f2 = fueraDelCirculo(px + _size - 1, py + _size - 1);
        if (f2 > f) f = f2;
        if (f > 0) { _runMal = true; if (f > _runLejos) _runLejos = f; }
      } else if (_opaqueBg) {
        fillRect(x + col * _size, y + row * _size, _size, _size, _bg);
      }
    }
  }
}
size_t Arduino_Canvas::write(uint8_t c) {
  if (_u8) return writeU8(c);
  if (c == '\n') {
    _cy += (_font ? _font->yAdvance : 8) * _size;
    _cx = 0;
    return 1;
  }
  if (c == '\r') return 1;
  _run += (char)c;
  if (gTextoTodo.size() > (1u << 20)) gTextoTodo.erase(0, 1u << 19);   // a bateria inteira nao enche a memoria
  gTextoTodo += (char)c;
  drawChar(_cx, _cy, c);
  const GFXglyph *g = glifoDe(_font, c);
  _cx += (g ? g->xAdvance : 6) * _size;
  return 1;
}
void Arduino_Canvas::getTextBounds(const char *s, int16_t x, int16_t y, int16_t *x1, int16_t *y1,
                                   uint16_t *w, uint16_t *h) {
  if (_u8) { boundsU8(s, x, y, x1, y1, w, h); return; }
  size_t n = s ? strlen(s) : 0;
  if (_font) {
    // Ancho sumando avances y alto/arriba del PRIMER glifo: es lo que el
    // firmware usa para medir el ascenso (gFontAscent = -y1).
    int ancho = 0, arriba = 0, abajo = 0;
    for (size_t i = 0; i < n; i++) {
      const GFXglyph *g = glifoDe(_font, (unsigned char)s[i]);
      if (!g) continue;
      ancho += g->xAdvance;
      if (g->yOffset < arriba) arriba = g->yOffset;
      if (g->yOffset + g->height > abajo) abajo = g->yOffset + g->height;
    }
    if (x1) *x1 = x;
    if (y1) *y1 = (int16_t)(y + arriba * _size);
    if (w) *w = (uint16_t)(ancho * _size);
    if (h) *h = (uint16_t)((abajo - arriba) * _size);
    return;
  }
  if (x1) *x1 = x;
  if (y1) *y1 = y;
  if (w) *w = (uint16_t)(n * 6 * _size);
  if (h) *h = (uint16_t)(8 * _size);
}

// ---------------------------------------------------------------- volcado
void Arduino_Canvas::savePPM(const char *path) const {
  FILE *f = fopen(path, "wb");
  if (!f) return;
  fprintf(f, "P6\n%d %d\n255\n", _w, _h);
  std::vector<uint8_t> row((size_t)_w * 3);
  for (int y = 0; y < _h; y++) {
    for (int x = 0; x < _w; x++) {
      uint16_t p = _fb[(int32_t)y * _w + x];
      // RGB565 -> RGB888 replicando los bits altos, igual que hace el panel
      uint8_t r = (p >> 11) & 0x1F, g = (p >> 5) & 0x3F, b = p & 0x1F;
      row[x * 3 + 0] = (uint8_t)((r << 3) | (r >> 2));
      row[x * 3 + 1] = (uint8_t)((g << 2) | (g >> 4));
      row[x * 3 + 2] = (uint8_t)((b << 3) | (b >> 2));
    }
    fwrite(row.data(), 1, row.size(), f);
  }
  fclose(f);
}

// el simulador decide fuera cuando guardar; aqui solo se cuenta
extern void simOnFlush(Arduino_Canvas *c);
void Arduino_Canvas::flush() {
  _frames++;
  simOnFlush(this);
}

// ---------------------------------------------------------------- U8g2 (CJK)
// O japones e o coreano: as fontes U8g2 DE VERDADE (u8g2_cjk_dados.inc, que
// tools/gen_u8g2_cjk_sim.py copia da biblioteca), decodificadas com o codigo
// da Arduino_GFX 1.6.8 (Arduino_GFX.cpp: setFont(const uint8_t*), write(),
// drawChar(), charBounds(), u8g2_font_decode_*). Antes o simulador desenhava
// cada BYTE do UTF-8 com a 5x7 e media 6 px por byte: a previa saia com lixo
// e os testes de largura mediam um texto que nao existe.
#include "U8g2lib.h"
#include "u8g2_cjk_dados.inc"

namespace {
struct LeBits {
  const uint8_t *p = nullptr;
  uint8_t pos = 0;
  // u8g2_font_decode_get_unsigned_bits
  uint8_t u(uint8_t cnt) {
    uint8_t val = *p;
    val >>= pos;
    uint8_t pc = (uint8_t)(pos + cnt);
    if (pc >= 8) {
      uint8_t s = (uint8_t)(8 - pos);
      p++;
      val |= (uint8_t)(*p << s);
      pc -= 8;
    }
    val &= (uint8_t)((1U << cnt) - 1);
    pos = pc;
    return val;
  }
  // u8g2_font_decode_get_signed_bits
  int8_t s(uint8_t cnt) {
    int8_t v = (int8_t)u(cnt);
    int8_t d = 1;
    d = (int8_t)(d << (cnt - 1));
    return (int8_t)(v - d);
  }
};
struct GlifoU8 {
  uint8_t w = 0, h = 0;
  int8_t x = 0, y = 0, dx = 0;
};
uint16_t palavraU8(const uint8_t *f, uint8_t off) { return (uint16_t)((f[off] << 8) | f[off + 1]); }

// u8g2_font_get_glyph_data + o cabecalho do glifo. Sem o glifo: false (a
// biblioteca nao desenha nada nem avanca o cursor).
bool achaGlifoU8(const uint8_t *font, uint16_t enc, GlifoU8 &g, LeBits &lb) {
  const uint8_t *f = font + 23;
  const uint8_t *gd = nullptr;
  if (enc <= 255) {
    if (enc >= 'a') f += palavraU8(font, 19);
    else if (enc >= 'A') f += palavraU8(font, 17);
    for (;;) {
      if (f[1] == 0) break;
      if (f[0] == enc) gd = f + 2;
      f += f[1];
    }
  } else {
    f += palavraU8(font, 21);
    const uint8_t *t = f;
    uint16_t e;
    do {
      f += palavraU8(t, 0);
      e = palavraU8(t, 2);
      t += 4;
    } while (e < enc);
    for (;;) {
      e = palavraU8(f, 0);
      if (e == 0) break;
      if (e == enc) { gd = f + 3; break; }
      f += f[2];
    }
  }
  if (!gd) return false;
  lb.p = gd;
  lb.pos = 0;
  g.w = lb.u(font[4]);
  g.h = lb.u(font[5]);
  g.x = lb.s(font[6]);
  g.y = lb.s(font[7]);
  g.dx = lb.s(font[8]);
  return true;
}

// a maquina de UTF-8 da biblioteca: devolve true quando `enc` esta completo
bool utf8Passo(uint8_t c, bool utf8, uint8_t &estado, uint16_t &enc) {
  if (!utf8) { enc = c; estado = 0; return true; }
  if (estado == 0) {
    if (c >= 0xfc) { estado = 5; c &= 1; }
    else if (c >= 0xf8) { estado = 4; c &= 3; }
    else if (c >= 0xf0) { estado = 3; c &= 7; }
    else if (c >= 0xe0) { estado = 2; c &= 15; }
    else if (c >= 0xc0) { estado = 1; c &= 0x01f; }
    enc = c;
  } else {
    estado--;
    enc = (uint16_t)(enc << 6);
    enc |= (uint16_t)(c & 0x03f);
  }
  return estado == 0;
}
}  // namespace

void Arduino_Canvas::setFont(const uint8_t *u8g2) {
  _font = nullptr;
  _u8 = u8g2;
  _u8Estado = 0;
}

void Arduino_Canvas::drawCharU8(int16_t x, int16_t y, uint16_t enc) {
  GlifoU8 g;
  LeBits lb;
  if (!achaGlifoU8(_u8, enc, g, lb)) return;
  const int s = _size;
  const int ty = y - (g.h + g.y) * s;
  const int tx = x + g.x * s;
  if (g.w == 0) return;
  uint8_t dx = 0, dy = 0;
  // u8g2_font_decode_len
  auto corre = [&](uint8_t len, bool frente) {
    uint8_t cnt = len, lx = dx, ly = dy;
    for (;;) {
      uint8_t rem = (uint8_t)(g.w - lx);
      uint8_t cur = cnt < rem ? cnt : rem;
      if (cur) {
        int px = tx + lx * s, py = ty + ly * s;
        if (frente) {
          fillRect(px, py, cur * s, s, _fg);
          int f = fueraDelCirculo(px, py);
          int f2 = fueraDelCirculo(px + cur * s - 1, py + s - 1);
          if (f2 > f) f = f2;
          if (f > 0) { _runMal = true; if (f > _runLejos) _runLejos = f; }
        } else if (_opaqueBg) {
          fillRect(px, py, cur * s, s, _bg);
        }
      }
      if (cnt < rem) break;
      cnt = (uint8_t)(cnt - rem);
      lx = 0;
      ly++;
    }
    lx = (uint8_t)(lx + cnt);
    dx = lx;
    dy = ly;
  };
  for (;;) {
    uint8_t a = lb.u(_u8[2]);
    uint8_t b = lb.u(_u8[3]);
    do {
      corre(a, false);
      corre(b, true);
    } while (lb.u(1) != 0);
    if (dy >= g.h) break;
  }
}

size_t Arduino_Canvas::writeU8(uint8_t c) {
  _run += (char)c;
  if (gTextoTodo.size() > (1u << 20)) gTextoTodo.erase(0, 1u << 19);
  gTextoTodo += (char)c;
  if (!utf8Passo(c, _utf8, _u8Estado, _u8Enc)) return 1;
  if (_u8Enc == '\n') {
    _cx = 0;
    _cy += _size * _u8[10];
    return 1;
  }
  if (_u8Enc == '\r') return 1;
  GlifoU8 g;
  LeBits lb;
  if (!achaGlifoU8(_u8, _u8Enc, g, lb)) return 1;   // sem glifo: nada, nem avanca
  drawCharU8(_cx, _cy, _u8Enc);
  _cx += _size * g.dx;
  return 1;
}

// charBounds + getTextBounds da biblioteca, com a fonte U8g2
void Arduino_Canvas::boundsU8(const char *str, int16_t x, int16_t y, int16_t *x1, int16_t *y1,
                              uint16_t *w, uint16_t *h) {
  const int s = _size;
  *x1 = x;
  *y1 = y;
  *w = *h = 0;
  int minx = _w - 1, miny = _h - 1, maxx = 0, maxy = 0;
  uint8_t estado = 0;
  uint16_t enc = 0;
  for (const unsigned char *p = (const unsigned char *)str; p && *p; p++) {
    if (!utf8Passo(*p, _utf8, estado, enc)) continue;
    if (enc == '\n') { x = 0; y = (int16_t)(y + s * _u8[10]); continue; }
    if (enc == '\r') continue;
    GlifoU8 g;
    LeBits lb;
    if (!achaGlifoU8(_u8, enc, g, lb)) continue;
    int gx1 = x + g.x * s, gy1 = y - (g.h + g.y) * s;
    int gx2 = gx1 + g.w * s - 1, gy2 = gy1 + g.h * s - 1;
    if (gx1 < minx) minx = gx1;
    if (gy1 < miny) miny = gy1;
    if (gx2 > maxx) maxx = gx2;
    if (gy2 > maxy) maxy = gy2;
    x = (int16_t)(x + s * g.dx);
  }
  if (maxx >= minx) { *x1 = (int16_t)minx; *w = (uint16_t)(maxx - minx + 1); }
  if (maxy >= miny) { *y1 = (int16_t)miny; *h = (uint16_t)(maxy - miny + 1); }
}
