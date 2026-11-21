// Que escenario de /bg va en cada sitio. Vive aparte porque lo necesitan los
// DOS lados: el combate (battle_ui.ino) y la pantalla de casa (TamaPoke.ino,
// que se concatena antes y por tanto no puede leer una tabla del otro).
//
// Los numeros son los de tools/pack_bg.py y, una vez publicados, NO se
// cambian: los ficheros de una tarjeta vieja saldrian en el sitio de otro.
#pragma once
#include <stdint.h>

#include "dex.h"
#include "gyms.h"

#define BG_FIELD 0
#define BG_FOREST 1
#define BG_CAVE 2
#define BG_BEACH 3
#define BG_CITY 4
#define BG_MOUNTAIN 5
#define BG_NIGHT 6
#define BG_VOLCANO 7
#define BG_SNOW 8
#define BG_LAB 41   // el laboratorio: intro del profesor y eleccion de inicial

// Una casa por lider, en el orden de KANTO_GYMS. Brock pelea en la cueva y
// Blaine en el volcan (que ya estan por el bioma); los demas tienen el suyo.
// Giovanni va a la base de Rocket, que es de donde sale de verdad.
//   BROCK MISTY SURGE ERIKA KOGA SABRINA BLAINE GIOVANNI
//   LORELEI BRUNO AGATHA LANCE AZUL
static const uint8_t KANTO_BG[GYM_TOTAL] = {
  BG_CAVE, 9, 10, 11, 12, 13, BG_VOLCANO, 14,
  15, 16, 17, 18, 19,
};

// Para un salvaje (y para la pantalla de casa) manda el `biome` que ya trae
// la dex: es la misma casilla que decide donde aparece, asi que el fondo y el
// encuentro no se pueden contradecir.
//   0 pradera 1 playa 2 bosque 3 volcan 4 montana 5 nieve
//
// 42.. son de otros fangames (leva 71, tools/pack_bg.py FONDOS_EXTRA).
// Cada bioma tiene VARIAS: un unico paisaje cansa enseguida, y la pantalla
// de casa esta siempre encendida. Quien elige entre ellas es bgPick().
struct BgSet { const uint8_t *ids; uint8_t n; };

static const uint8_t BGS_FIELD[] = { BG_FIELD, 20, 21, 22,
    42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55 };
static const uint8_t BGS_BEACH[] = { BG_BEACH, 23, 24,
    56, 57, 58, 59, 60, 61, 62, 63 };
static const uint8_t BGS_FOREST[] = { BG_FOREST, 25, 26, 27,
    64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75 };
static const uint8_t BGS_VOLC[] = { BG_VOLCANO, 28,
    76, 77, 78, 79 };
static const uint8_t BGS_MOUNT[] = { BG_MOUNTAIN, 29, 30, 31,
    80, 81, 82, 83, 84, 85, 86, 87, 88, 89 };
static const uint8_t BGS_SNOW[] = { BG_SNOW, 32, 33, 34,
    90, 91, 92, 93, 94, 95 };
static const uint8_t BGS_NIGHT[] = { BG_NIGHT, 35, 36,
    96, 97, 98, 99, 100, 101, 102, 103, 104 };
static const uint8_t BGS_CAVE[] = { BG_CAVE, 37, 38,
    105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116 };
static const uint8_t BGS_CITY[] = { BG_CITY, 39, 40,
    117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127 };

#define BGSET(a) { a, (uint8_t)(sizeof(a) / sizeof(a[0])) }

static const BgSet BIOME_BGS[6] = {
  BGSET(BGS_FIELD), BGSET(BGS_BEACH), BGSET(BGS_FOREST),
  BGSET(BGS_VOLC),  BGSET(BGS_MOUNT), BGSET(BGS_SNOW),
};
static const BgSet NIGHT_BGS = BGSET(BGS_NIGHT);
static const BgSet CAVE_BGS = BGSET(BGS_CAVE);
static const BgSet CITY_BGS = BGSET(BGS_CITY);

// `semilla` decide cual toca. No se usa random() a proposito: el escenario de
// casa se repinta 30 veces por segundo y con un dado cada fotograma seria un
// estroboscopio. Quien llama pasa algo que cambie despacio (el combate, en
// cambio, si sortea, porque solo elige una vez al empezar).
static inline uint8_t bgPick(const BgSet &s, uint32_t semilla) {
  return s.n ? s.ids[semilla % s.n] : BG_FIELD;
}

static inline const BgSet &bgSetOfBiome(uint8_t b) {
  return BIOME_BGS[b < 6 ? b : 0];
}
