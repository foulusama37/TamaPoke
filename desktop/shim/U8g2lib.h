// El firmware solo usa U8g2 para las fuentes CJK (japones/coreano). Desde la
// leva 79 el simulador carga LAS MISMAS (desktop/shim/u8g2_cjk_dados.inc, que
// tools/gen_u8g2_cjk_sim.py copia de la biblioteca U8g2) y gfx.cpp las
// decodifica como Arduino_GFX: la previa en japones ya no sale con basura y
// los tests miden el ancho de verdad.
#pragma once
#include <stdint.h>

typedef uint8_t u8g2_font_t;
extern const uint8_t u8g2_font_unifont_t_japanese3[];
extern const uint8_t u8g2_font_unifont_t_korean2[];
