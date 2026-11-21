// GENERADO por tools/gen_formas_combate.py desde pokemon_forms.txt del Anil.
// No editar.
//
// Las formas que se ganan y se pierden DENTRO del combate (Castform al
// sol, Aegislash Filo, Darmanitan Daruma...). No van en FORM_TBL porque
// esa tabla esta en el save, en la Pokedex y en la evolucion. El sprite
// es /mons/c<dex 4 cifras>_<forma>.bin: por dex y forma, nunca por fila.
#pragma once
#include <stdint.h>
#include "dex.h"

struct BattleForm {
  int16_t base;
  uint8_t form;       // el numero de forma del Anil
  uint8_t bHp, bAtk, bDef, bSpA, bSpD, bSpe;
  uint8_t type1, type2;
  uint8_t screenSz;   // alto de cuerpo, como DEX_TBL[].screenSz
};

#define BFORM_COUNT 15
static const BattleForm BFORM_TBL[BFORM_COUNT] = {
  {  351,  1,  90,  70,  90, 130,  90, 100, T_FIRE   , T_NONE   , 104 },  // CASTFORM: Forma Sol
  {  351,  2,  90,  70,  90, 130,  90, 100, T_WATER  , T_NONE   ,  98 },  // CASTFORM: Forma Lluvia
  {  351,  3,  90,  70,  90, 130,  90, 100, T_ICE    , T_NONE   , 108 },  // CASTFORM: Forma Nieve
  {  421,  1,  70,  60,  70, 100,  88, 100, T_GRASS  , T_NONE   ,  96 },  // CHERRIM: Forma Soleada
  {  555,  1, 105,  30, 105, 140, 105,  55, T_FIRE   , T_PSYCHIC, 102 },  // DARMANITAN: Forma Daruma
  {  555,  3, 105, 160,  55,  30,  55, 135, T_ICE    , T_FIRE   , 119 },  // DARMANITAN: Forma Daruma de Galar
  {  681,  1,  60, 150,  50, 150,  50,  60, T_STEEL  , T_GHOST  , 124 },  // AEGISLASH: Forma Filo
  {  746,  1,  45, 140, 130, 140, 135,  30, T_WATER  , T_NONE   , 147 },  // WISHIWASHI: Forma Banco
  {  774,  7,  60, 100,  60, 100,  60, 120, T_ROCK   , T_FLYING ,  91 },  // MINIOR: Forma Núcleo Rojo
  {  778,  1,  55, 105,  80,  50, 105,  96, T_GHOST  , T_FAIRY  ,  94 },  // MIMIKYU: Forma Descubierta
  {  875,  1,  75,  80,  70,  65,  50, 130, T_ICE    , T_NONE   , 106 },  // EISCUE: Forma Cara Deshielo
  {  845,  1,  70,  85,  55,  95,  85,  85, T_FLYING , T_WATER  , 120 },  // CRAMORANT: Forma Tragatodo
  {  845,  2,  70,  85,  55,  95,  85,  85, T_FLYING , T_WATER  , 128 },  // CRAMORANT: Forma Engulletodo
  {  877,  1,  58,  95,  58,  70,  58,  97, T_ELEC   , T_DARK   ,  94 },  // MORPEKO: Forma Voraz
  {  718,  2, 216, 100, 121,  91,  95,  85, T_DRAGON , T_GROUND , 167 },  // ZYGARDE: Forma Completa
};

// La fila de esa forma de combate de esa especie, o -1.
static inline int8_t bformRow(int16_t dex, uint8_t form) {
  for (uint8_t i = 0; i < BFORM_COUNT; i++)
    if (BFORM_TBL[i].base == dex && BFORM_TBL[i].form == form)
      return (int8_t)i;
  return -1;
}
