// Intro de primera partida: creditos -> profesor -> chico/chica -> inicial.
//
// Solo se ve UNA vez por cacharro (`Pet::introDone`, en la NVS) y la borra
// unicamente el reseteo de fabrica. Va antes de la eleccion de inicial, que
// ya existia: la intro no la sustituye, la presenta.
//
// Los retratos viven en FLASH (art.h, con RLE) y no en la SD a
// proposito: es la primera pantalla que ve alguien que acaba de montar el
// aparato, y no puede ser un error de tarjeta.

#include "intro.h"

#define IN_TITLE 0    // creditos + portada
#define IN_OAK 1      // el profesor habla (introPage 0..3)
#define IN_GENDER 2   // chico o chica
#define OAK_PAGES 5        // a 5a: onde fica o manual (leva 74)

bool introOpen = false;
uint8_t introStep = IN_TITLE;
uint8_t introPage = 0;
uint32_t introSince = 0;

// ---------- dibujo de los retratos ----------

// Descomprime el RLE y lo pinta anclado por los PIES (como los bichos): asi
// dos retratos de alturas distintas se apoyan en la misma linea.
//
// `clipY` corta por abajo, igual que `PetFx.clipY` en la caida del combate.
// Hace falta porque a escala 2 el profesor mide 302 px y la pantalla son
// 466 con una caja de texto en medio: sin recorte habria que dibujarlo
// diminuto, y un retrato de medio cuerpo se lee mucho mejor.
static void drawArtCore(const Art &a, int cx, int baseY, int s, int clipY,
                        bool solido, uint16_t tinta) {
  int x0 = cx - (a.w * s) / 2, y0 = baseY - a.h * s;
  uint32_t px = 0;
  for (uint16_t i = 0; i + 1 < a.rleLen; i += 2) {
    uint8_t n = a.rle[i], idx = a.rle[i + 1];
    if (idx == ART_TRANSP) { px += n; continue; }
    uint16_t col = solido ? tinta : a.pal[idx];
    // Una tirada RLE puede cruzar el final de la fila, asi que se parte.
    while (n) {
      uint16_t r = (uint16_t)(px / a.w), c = (uint16_t)(px % a.w);
      uint16_t cabe = (uint16_t)(a.w - c);
      uint8_t t = n < cabe ? n : (uint8_t)cabe;
      int py = y0 + r * s;
      if (py + s <= clipY) gfx->fillRect(x0 + c * s, py, t * s, s, col);
      px += t;
      n = (uint8_t)(n - t);
    }
  }
}

void drawArt(const Art &a, int cx, int baseY, int s, int clipY) {
  drawArtCore(a, cx, baseY, s, clipY, false, 0);
}

// El mismo dibujo pintado de UN solo color. Existe para la estrella de shiny:
// su paleta es casi blanca y sobre la chapa blanca de la barra de PS se
// perdia. Con cuatro siluetas oscuras alrededor se lee encima de cualquier
// cosa, sea el cielo, la hierba o un panel claro.
void drawArtSolid(const Art &a, int cx, int baseY, int s, uint16_t col, int clipY) {
  drawArtCore(a, cx, baseY, s, clipY, true, col);
}

// El dibujo a cualquier ancho, CENTRADO en (cx, cy). `drawArt` solo sabe
// escalas enteras y ancla por los pies; esto hace falta para lo que crece
// —— el sello de la mega —— donde tres saltos de escala entera se verian
// como tres saltos y no como un crecimiento.
//
// Regla de tres por pixel: el de origen (c, r) ocupa de c*destW/w a
// (c+1)*destW/w. Al encoger salen rectangulos de ancho 0 y se pierden
// columnas; en una animacion de un segundo no se nota.
void drawArtZoom(const Art &a, int cx, int cy, int destW) {
  if (destW <= 0 || a.w == 0 || a.h == 0) return;
  const int destH = (int)a.h * destW / (int)a.w;
  if (destH <= 0) return;
  const int x0 = cx - destW / 2, y0 = cy - destH / 2;
  uint32_t px = 0;
  for (uint16_t i = 0; i + 1 < a.rleLen; i += 2) {
    uint8_t n = a.rle[i], idx = a.rle[i + 1];
    if (idx == ART_TRANSP) { px += n; continue; }
    const uint16_t col = a.pal[idx];
    while (n) {
      uint16_t r = (uint16_t)(px / a.w), c = (uint16_t)(px % a.w);
      uint16_t cabe = (uint16_t)(a.w - c);
      uint8_t t = n < cabe ? n : (uint8_t)cabe;
      const int xa = x0 + (int)c * destW / (int)a.w;
      const int xb = x0 + (int)(c + t) * destW / (int)a.w;
      const int ya = y0 + (int)r * destH / (int)a.h;
      const int yb = y0 + (int)(r + 1) * destH / (int)a.h;
      if (xb > xa && yb > ya) gfx->fillRect(xa, ya, xb - xa, yb - ya, col);
      px += t;
      n = (uint8_t)(n - t);
    }
  }
}

// ---------- portada ----------

// La bola de la portada es el sprite de verdad, no un circulo pintado a mano:
// `drawArt` ancla por los pies, asi que para centrarla en `cy` hay que bajar
// la base media altura.
#define BALL_MARK_S 3

static void drawPokeballMark(int cx, int cy) {
  drawArt(ART_UI_BALL, cx, cy + (ART_UI_BALL.h * BALL_MARK_S) / 2, BALL_MARK_S);
}

static void renderIntroTitle(uint32_t now) {
  gfx->fillScreen(RGB565_BLACK);
  gfx->fillCircle(CX, CY, 231, C565(0x1b, 0x2a, 0x4a));
  // un par de destellos, para que la portada no sea un cartel quieto
  for (auto &st : STARS) gfx->fillRect(st[0], st[1], 4, 4, UI_WHITE);

  // la bola sube y baja despacio
  int bob = (int)(sinf(now * 0.0016f) * 5.0f);
  drawPokeballMark(CX, 116 + bob);

  gfx->setTextColor(C565(0xff, 0xd9, 0x6a));
  setSize(5);
  setCur(centerX("TamaPoke", 5), 196);
  printT("TamaPoke");

  // el subtitulo va en una cinta, que es lo que lo convierte en portada
  const char *sub = T(S_INTRO_SUB);
  int sw = textW(sub, 2) + 28;
  gfx->fillRoundRect(CX - sw / 2, 250, sw, 30, 8, C565(0xe8, 0x4b, 0x43));
  gfx->setTextColor(UI_WHITE);
  setSize(2);
  setCur(CX - textW(sub, 2) / 2, 258);
  printT(sub);

  // el credito al autor original, que es de lo que va esta pantalla
  gfx->setTextColor(C565(0x9f, 0xb4, 0xd8));
  setSize(1);
  setCur(centerX(T(S_INTRO_BASED), 1), 316);
  printT(T(S_INTRO_BASED));
  gfx->setTextColor(UI_WHITE);
  setSize(2);
  setCur(centerX("socquique", 2), 330);
  printT("socquique");
  gfx->setTextColor(C565(0x9f, 0xb4, 0xd8));
  setSize(1);
  setCur(centerX("github.com/socquique/TamaPoke", 1), 358);
  printT("github.com/socquique/TamaPoke");

  // parpadea para que se vea que hay que tocar
  if ((now / 600) % 2 == 0) {
    gfx->setTextColor(C565(0x6f, 0x86, 0xb0));
    setSize(1);
    setCur(centerX(T(S_INTRO_TAP), 1), 396);
    printT(T(S_INTRO_TAP));
  }
}

// ---------- el profesor ----------

static void renderIntroOak() {
  gfx->fillScreen(RGB565_BLACK);
  // El profesor habla en SU laboratorio. Sin tarjeta queda el circulo claro
  // de antes: la intro es lo primero que se ve al montar el cacharro y no
  // puede depender de que la SD este puesta.
  if (!drawSceneBg(BG_LAB)) gfx->fillCircle(CX, CY, 231, UI_BG_DAY);
  // A escala 2 mide 302 px: se ancla arriba y se recorta justo donde empieza
  // la caja, o sea que se ve de la cabeza al muslo. Centrado en 233 ocupa de
  // x=169 a x=297, dentro del circulo incluso arriba del todo.
  drawArt(ART_OAK, CX, 40 + ART_OAK.h * 2, 2, 306);

  // La caja de texto, dimensionada para que la charla quepa A TALLA 2.
  //
  // A talla 1 esto era ilegible en la pantalla de verdad: un caracter
  // mide 6x8 px en una de 466, y con el aparato en la mano no se leia.
  // Subirlo a 2 cambia la cuenta entera, asi que se hizo con numeros:
  //
  //   322 px de ancho util / 12 px por caracter = 26 columnas
  //   4 lineas de 18 px = 72 px de texto
  //
  // CUATRO lineas y no tres porque el ALEMAN las pide: sus dos paginas
  // largas miden 74 y 83 caracteres y se parten en cuatro. En ingles
  // caben en tres de sobra. Medir solo el idioma que uno habla es como
  // se descubre tarde que una frase se corta a la mitad en otro.
  //
  // Y la caja se estrecha de 346 a 322: baja hasta y=400 para que quepa
  // el texto, y a esa altura la cuerda del circulo ya no da 346 px -- la
  // caja se habria salido por las esquinas de abajo.
  drawWindow(72, 306, 322, 94, 14);
  static const StrId PAG[OAK_PAGES] = { S_OAK_1, S_OAK_2, S_OAK_3, S_OAK_4, S_OAK_5 };
  gfx->setTextColor(INK_K);
  drawWrapped(T(PAG[introPage < OAK_PAGES ? introPage : 0]), 322, 26, 18, 4, 2);

  // tres puntitos: cuanto queda de charla
  for (int i = 0; i < OAK_PAGES; i++) {
    int x = CX - (OAK_PAGES * 14) / 2 + i * 14 + 7;
    if (i == introPage) gfx->fillCircle(x, 414, 4, UI_INK);
    else gfx->drawCircle(x, 414, 4, UI_TRACK);
  }
}

// ---------- chico o chica ----------

#define GEN_BTN_Y 330
#define GEN_BTN_H 54
#define GEN_BTN_W 150

static void renderIntroGender() {
  gfx->fillScreen(RGB565_BLACK);
  if (!drawSceneBg(BG_LAB)) gfx->fillCircle(CX, CY, 231, UI_BG_DAY);
  printOutlined(centerX(T(S_ASK_GENDER), 2), 52, T(S_ASK_GENDER), 2, UI_WHITE);

  drawArt(ART_BOY, 142, 314, 1);
  drawArt(ART_GIRL, 324, 314, 1);

  const char *et[2] = { T(S_BOY), T(S_GIRL) };
  for (int i = 0; i < 2; i++) {
    int bx = (i ? 324 : 142) - GEN_BTN_W / 2;
    gfx->fillRoundRect(bx, GEN_BTN_Y, GEN_BTN_W, GEN_BTN_H, 13,
                       i ? C565(0xd4, 0x52, 0x7e) : C565(0x4a, 0x8d, 0xd4));
    gfx->setTextColor(UI_WHITE);
    setSize(2);
    setCur(bx + (GEN_BTN_W - textW(et[i], 2)) / 2, GEN_BTN_Y + 18);
    printT(et[i]);
  }
}

// ---------- pegamento ----------

void startIntro() {
  introOpen = true;
  introStep = IN_TITLE;
  introPage = 0;
  introSince = millis();
}

void renderIntro() {
  uint32_t now = millis();
  if (introStep == IN_TITLE)       renderIntroTitle(now);
  else if (introStep == IN_OAK)    renderIntroOak();
  else                             renderIntroGender();
  gfx->flush();
}

void introTap(int16_t x, int16_t y) {
  // La portada se come el primer segundo de toques: encender el cacharro y
  // saltarse los creditos sin verlos por un toque perdido seria una lastima,
  // y es justo la pantalla que da credito al autor original.
  if (introStep == IN_TITLE) {
    if (millis() - introSince < 1000) return;
    sfxPlay(SFX_TAP);
    introStep = IN_OAK;
    introPage = 0;
    return;
  }
  if (introStep == IN_OAK) {
    sfxPlay(SFX_TAP);
    if (++introPage >= OAK_PAGES) { introStep = IN_GENDER; introPage = 0; }
    return;
  }
  // genero: solo cuentan los dos botones, no el retrato. Un retrato es un
  // dibujo; que sea tambien un boton se descubre tocandolo por accidente.
  if (y < GEN_BTN_Y || y > GEN_BTN_Y + GEN_BTN_H) return;
  for (int i = 0; i < 2; i++) {
    int bx = (i ? 324 : 142) - GEN_BTN_W / 2;
    if (x < bx || x > bx + GEN_BTN_W) continue;
    sfxPlay(SFX_MEDAL);
    pet.finishIntro(i == 1);
    introOpen = false;
    return;
  }
}
