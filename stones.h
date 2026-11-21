// Las evoluciones por OBJETO.
//
// La tabla ya no vive aqui: la genera tools/gen_evos.py leyendo el PBS del
// Anil (evos.h). Estaba escrita a mano y se invento la mitad — un "Linking
// Cord" generico para todo lo que en los juegos es intercambio, cuando el
// Anil dice el objeto exacto ("Evolutions = STEELIX,TradeItem,METALCOAT"),
// y piedras para Espeon y Umbreon cuando dice amistad de dia y de noche.
//
// Este fichero queda como los NOMBRES de siempre, para no tocar las ocho
// llamadas repartidas por pet.cpp, el .ino y los tests.
#pragma once
#include <stdint.h>

#include "dex.h"
#include "evos.h"

#define T_SUN EVOI_SUN             // compatibilidad: era el nombre viejo
#define STONE_EVO_COUNT EVO_ITEM_COUNT
#define STONE_EVO EVO_ITEM
#define STONE_MANY EVO_ITEM_MANY

// OJO, cambio de significado: ahora es "le sirve ALGUN objeto", no "solo
// puede evolucionar con objeto". Son cosas distintas y antes se confundian:
// el Anil da a Onix DOS caminos (nivel 42 y Metal Coat), y tratarlo como
// solo-objeto le quitaba el de nivel. Quien quiera bloquear el nivel tiene
// que mirar ADEMAS que no haya `evolveLevel`.
static inline bool needsStone(int16_t dex) { return evoHasItem(dex); }

// Solo con objeto: no hay camino por nivel.
static inline bool onlyByItem(int16_t dex) {
  return evoHasItem(dex) && dex >= 1 && dex <= DEX_COUNT &&
         DEX_TBL[dex].evolveLevel == 0;
}

static inline uint8_t stoneTypeFor(int16_t dex) { return evoItemFor(dex); }
// `hembra` por Snorunt, que con la Piedra Alba da Froslass solo si lo es.
static inline int16_t stoneTarget(int16_t dex, uint8_t item,
                                  bool hembra = true) {
  return evoItemTarget(dex, item, hembra);
}
