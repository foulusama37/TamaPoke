// La forma de un dibujo RLE, SIN los datos.
//
// Vive aparte de art.h porque sdmon.cpp tambien la necesita (carga las
// animaciones de golpe desde la tarjeta) y art.h son 143 KB de `static
// const`: incluirlo en otra unidad de compilacion duplicaria el arte
// entero en flash.
#pragma once
#include <stdint.h>

// 0xFF = pixel transparente (no se dibuja).
#define ART_TRANSP 0xFF

struct Art {
  uint8_t w, h;
  const uint16_t *pal;
  const uint8_t *rle;   // pares (repeticiones, indice de paleta)
  uint16_t rleLen;      // en BYTES, o sea pares*2
};

// Una animacion: sus fotogramas, cuantos son y a que escala se pinta.
struct FxAnim { const Art *const *f; uint8_t n, s; };
