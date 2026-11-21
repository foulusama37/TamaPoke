// Estado de la intro de primera partida. Existe por la misma razon que
// battle_ui.h: los .ino se concatenan en orden y TamaPoke.ino necesita
// nombrar esto antes de que intro.ino lo defina.
#pragma once
#include <stdint.h>
#include "art.h"

extern bool introOpen;

void startIntro();
void renderIntro();
void introTap(int16_t x, int16_t y);
// Pinta un retrato RLE anclado por los pies, como los bichos. `clipY` corta
// por abajo (466 = sin recorte).
void drawArt(const Art &a, int cx, int baseY, int s, int clipY = 466);
// El mismo dibujo de un solo color, para siluetas y contornos.
void drawArtSolid(const Art &a, int cx, int baseY, int s, uint16_t col,
                  int clipY = 466);
// El dibujo a CUALQUIER ancho y centrado en (cx, cy), no anclado por los
// pies ni atado a escalas enteras. Para cosas que crecen.
void drawArtZoom(const Art &a, int cx, int cy, int destW);
