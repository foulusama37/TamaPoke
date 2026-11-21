// El firmware solo usa U8g2 para las fuentes CJK (japones/coreano). En el
// simulador no se cargan: applyLangFont() cae en la fuente clasica, que es lo
// que ya pasa en la placa para los seis idiomas latinos.
#pragma once
#include <stdint.h>

typedef uint8_t u8g2_font_t;
#define u8g2_font_unifont_t_japanese3 nullptr
#define u8g2_font_unifont_t_korean1 nullptr
#define u8g2_font_unifont_t_korean2 nullptr
#define u8g2_font_unifont_t_chinese3 nullptr
