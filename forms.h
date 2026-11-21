// GENERADO por tools/gen_forms.py desde pokemon_forms.txt del Anil.
// No editar.
//
// Una FORMA comparte numero de dex con su especie y cambia tipos,
// stats, habilidad y dibujo. Un Vulpix de Alola es un Vulpix.
//
// Las regionales se asignan por DONDE SALE el bicho: en Alola, un
// Vulpix sale de Alola. Y encadenan solas, porque al evolucionar se
// conserva el numero de forma si el destino lo tiene (formOf).
#pragma once
#include <stdint.h>
#include "dex.h"

#define FORM_NO_REGION 0xFF

struct FormVariant {
  int16_t base;
  uint8_t form;      // el sprite es <slug>_<form> en la tarjeta
  uint8_t region;    // en que region sale, o FORM_NO_REGION
  uint8_t chance;    // con que probabilidad, en % (ver abajo)
  uint16_t bHp;
  uint8_t bAtk, bDef, bSpA, bSpD, bSpe;
  uint8_t type1, type2;
  uint16_t ability;
  uint8_t screenSz;
  const char *name;
};

#define FORM_COUNT 61
static const FormVariant FORM_TBL[FORM_COUNT] = {
  {   19,  1, 6             , 100,  30,  56,  35,  25,  35,  72, T_DARK    , T_NORMAL  ,  80, 100, "A.RATTATA" },  // RATTATA: Forma Alola
  {   20,  1, 6             , 100,  75,  71,  70,  40,  80,  77, T_DARK    , T_NORMAL  ,  80, 115, "A.RATICATE" },  // RATICATE: Forma Alola
  {   26,  1, 6             , 100,  60,  85,  50,  95,  85, 110, T_ELEC    , T_PSYCHIC , 180, 125, "A.RAICHU" },  // RAICHU: Forma Alola
  {   27,  1, 6             , 100,  50,  75,  90,  10,  35,  40, T_ICE     , T_STEEL   , 226,  98, "A.SANDSHREW" },  // SANDSHREW: Forma Alola
  {   28,  1, 6             , 100,  75, 100, 110,  45,  55,  90, T_ICE     , T_STEEL   , 226, 115, "A.SANDSLASH" },  // SANDSLASH: Forma Alola
  {   37,  1, 6             , 100,  38,  41,  40,  60,  65,  65, T_ICE     , T_NONE    , 226, 109, "A.VULPIX" },  // VULPIX: Forma Alola
  {   38,  1, 6             , 100,  73,  67,  75, 100, 100, 109, T_ICE     , T_FAIRY   , 226, 139, "A.NINETALES" },  // NINETALES: Forma Alola
  {   50,  1, 6             , 100,  10,  55,  30,  35,  45,  90, T_GROUND  , T_STEEL   , 205,  82, "A.DIGLETT" },  // DIGLETT: Forma Alola
  {   51,  1, 6             , 100,  35, 100,  60,  50,  70, 110, T_GROUND  , T_STEEL   , 205, 108, "A.DUGTRIO" },  // DUGTRIO: Forma Alola
  {   52,  1, 6             , 100,  40,  35,  35,  50,  40,  90, T_DARK    , T_NONE    , 162, 103, "A.MEOWTH" },  // MEOWTH: Forma Alola
  {   52,  2, 7             , 100,  50,  65,  55,  40,  40,  40, T_STEEL   , T_NONE    , 162, 105, "G.MEOWTH" },  // MEOWTH: Forma Galar
  {   53,  1, 6             , 100,  65,  60,  60,  75,  65, 115, T_DARK    , T_NONE    ,  78, 121, "A.PERSIAN" },  // PERSIAN: Forma Alola
  {   58,  1, 3             ,  12,  60,  75,  45,  65,  50,  55, T_FIRE    , T_ROCK    , 112, 116, "H.GROWLITHE" },  // GROWLITHE: Forma Hisui
  {   59,  1, 3             ,  12,  95, 115,  80,  95,  80,  90, T_FIRE    , T_ROCK    , 112, 138, "H.ARCANINE" },  // ARCANINE: Forma Hisui
  {   74,  1, 6             , 100,  50,  80, 100,  30,  30,  20, T_ROCK    , T_ELEC    , 132, 102, "A.GEODUDE" },  // GEODUDE: Forma Alola
  {   75,  1, 6             , 100,  80,  95, 115,  45,  45,  35, T_ROCK    , T_ELEC    , 132, 119, "A.GRAVELER" },  // GRAVELER: Forma Alola
  {   76,  1, 6             , 100, 110, 120, 130,  55,  65,  45, T_ROCK    , T_ELEC    , 132, 129, "A.GOLEM" },  // GOLEM: Forma Alola
  {   77,  1, 7             , 100,  50,  85,  55,  65,  65,  90, T_PSYCHIC , T_NONE    , 200, 126, "G.PONYTA" },  // PONYTA: Forma Galar
  {   78,  1, 7             , 100,  65, 110,  70,  80,  80, 105, T_PSYCHIC , T_FAIRY   , 106, 137, "G.RAPIDASH" },  // RAPIDASH: Forma Galar
  {   79,  1, 7             , 100,  90,  65,  65,  40,  40,  15, T_PSYCHIC , T_NONE    ,  80, 104, "G.SLOWPOKE" },  // SLOWPOKE: Forma Galar
  {   80,  1, 7             , 100,  95,  65,  80, 110, 110,  30, T_POISON  , T_PSYCHIC , 186, 124, "G.SLOWBRO" },  // SLOWBRO: Forma Galar
  {   83,  1, 7             , 100,  52,  95,  55,  58,  62,  55, T_FIGHT   , T_NONE    , 240, 113, "G.FARFETCHD" },  // FARFETCHD: Forma Galar
  {   88,  1, 6             , 100,  80,  60,  50,  60,  50,  25, T_POISON  , T_DARK    , 171, 106, "A.GRIMER" },  // GRIMER: Forma Alola
  {   89,  1, 6             , 100, 105, 105,  75,  65, 100,  50, T_POISON  , T_DARK    , 171, 129, "A.MUK" },  // MUK: Forma Alola
  {  100,  1, 3             ,  12,  40,  30,  50,  55,  55, 100, T_ELEC    , T_GRASS   ,  60,  88, "H.VOLTORB" },  // VOLTORB: Forma Hisui
  {  101,  1, 3             ,  12,  60,  50,  70,  80,  80, 150, T_ELEC    , T_GRASS   ,  84, 111, "H.ELECTRODE" },  // ELECTRODE: Forma Hisui
  {  103,  1, 6             , 100,  95,  95,  85, 125,  75,  55, T_GRASS   , T_DRAGON  ,  76, 136, "A.EXEGGUTOR" },  // EXEGGUTOR: Forma Alola
  {  105,  1, 6             , 100,  60,  80, 110,  50,  80,  65, T_FIRE    , T_GHOST   ,  41, 119, "A.MAROWAK" },  // MAROWAK: Forma Alola
  {  110,  1, 7             , 100,  65,  90, 120,  95,  70,  60, T_POISON  , T_FAIRY   , 120, 145, "G.WEEZING" },  // WEEZING: Forma Galar
  {  122,  1, 7             , 100,  50,  65,  65,  90,  90, 100, T_ICE     , T_PSYCHIC , 288, 111, "G.MR. MIME" },  // MR. MIME: Forma Galar
  {  128,  1, 8             ,  33,  75, 110, 105,  30,  70, 100, T_FIGHT   , T_NONE    , 112, 117, "P.TAUROS CMB" },  // TAUROS: Forma Paldea (Combatiente)
  {  128,  2, 8             ,  33,  75, 110, 105,  30,  70, 100, T_FIGHT   , T_FIRE    , 112, 118, "P.TAUROS BLZ" },  // TAUROS: Forma Paldea (Ardiente)
  {  128,  3, 8             ,  34,  75, 110, 105,  30,  70, 100, T_FIGHT   , T_WATER   , 112, 118, "P.TAUROS AQU" },  // TAUROS: Forma Paldea (Acuática)
  {  144,  1, 7             , 100, 100,  85,  85, 125,  95,  95, T_PSYCHIC , T_FLYING  ,  33, 122, "G.ARTICUNO" },  // ARTICUNO: Forma Galar
  {  145,  1, 7             , 100, 100, 125,  90,  85,  90, 100, T_FIGHT   , T_FLYING  ,  49, 123, "G.ZAPDOS" },  // ZAPDOS: Forma Galar
  {  146,  1, 7             , 100, 100,  85,  90, 100, 125,  90, T_DARK    , T_FLYING  ,  19, 134, "G.MOLTRES" },  // MOLTRES: Forma Galar
  {  157,  1, 3             ,  12,  73,  74,  78, 119,  85, 105, T_FIRE    , T_GHOST   ,  19, 127, "H.TYPHLOSION" },  // TYPHLOSION: Forma Hisui
  {  194,  1, 8             , 100,  55,  55,  45,  35,  35,  15, T_POISON  , T_GROUND  , 169,  90, "P.WOOPER" },  // WOOPER: Forma Paldea
  {  199,  1, 7             , 100,  95,  65,  80, 110, 110,  30, T_POISON  , T_PSYCHIC ,  63, 117, "G.SLOWKING" },  // SLOWKING: Forma Galar
  {  211,  1, 3             ,  12,  65,  95,  85,  55,  55,  85, T_DARK    , T_POISON  , 169,  94, "H.QWILFISH" },  // QWILFISH: Forma Hisui
  {  215,  1, 3             ,  12,  55,  95,  55,  35,  75, 115, T_FIGHT   , T_POISON  , 110, 104, "H.SNEASEL" },  // SNEASEL: Forma Hisui
  {  222,  1, 7             , 100,  60,  55, 100,  65, 100,  30, T_GHOST   , T_NONE    , 295,  99, "G.CORSOLA" },  // CORSOLA: Forma Galar
  {  263,  1, 7             , 100,  38,  30,  41,  30,  41,  60, T_DARK    , T_NORMAL  , 161, 109, "G.ZIGZAGOON" },  // ZIGZAGOON: Forma Galar
  {  264,  1, 7             , 100,  80,  70,  61,  50,  61, 100, T_DARK    , T_NORMAL  , 161, 114, "G.LINOONE" },  // LINOONE: Forma Galar
  {  503,  1, 3             ,  12,  90, 118,  80,  90,  65,  85, T_WATER   , T_DARK    , 272, 138, "H.SAMUROTT" },  // SAMUROTT: Forma Hisui
  {  549,  1, 3             ,  12,  70, 105,  75,  50,  75, 105, T_GRASS   , T_FIGHT   ,  27, 108, "H.LILLIGANT" },  // LILLIGANT: Forma Hisui
  {  554,  2, 7             , 100,  70,  90,  45,  15,  45,  50, T_ICE     , T_NONE    ,  98,  93, "G.DARUMAKA" },  // DARUMAKA: Forma Galar
  {  555,  2, 7             , 100, 105, 140,  55,  30,  55,  95, T_ICE     , T_NONE    ,  83, 126, "G.DARMANITAN" },  // DARMANITAN: Forma Galar
  {  562,  1, 7             , 100,  38,  55,  85,  30,  65,  30, T_GROUND  , T_GHOST   , 290,  93, "G.YAMASK" },  // YAMASK: Forma Galar
  {  570,  1, 3             ,  12,  35,  60,  40,  85,  40,  70, T_NORMAL  , T_GHOST   , 105, 108, "H.ZORUA" },  // ZORUA: Forma Hisui
  {  571,  1, 3             ,  12,  55, 100,  60, 125,  60, 110, T_NORMAL  , T_GHOST   , 105, 140, "H.ZOROARK" },  // ZOROARK: Forma Hisui
  {  618,  1, 7             , 100, 109,  81,  99,  66,  84,  32, T_GROUND  , T_STEEL   , 136, 102, "G.STUNFISK" },  // STUNFISK: Forma Galar
  {  628,  1, 3             ,  12, 110,  83,  70, 112,  70,  65, T_PSYCHIC , T_FLYING  , 117, 143, "H.BRAVIARY" },  // BRAVIARY: Forma Hisui
  {  705,  1, 3             ,  12,  58,  75,  83,  83, 113,  40, T_STEEL   , T_DRAGON  , 206, 114, "H.SLIGGOO" },  // SLIGGOO: Forma Hisui
  {  706,  1, 3             ,  12,  80, 100, 100, 110, 150,  60, T_STEEL   , T_DRAGON  , 206, 143, "H.GOODRA" },  // GOODRA: Forma Hisui
  {  713,  1, 3             ,  12,  95, 127, 184,  34,  36,  38, T_ICE     , T_ROCK    , 248, 134, "H.AVALUGG" },  // AVALUGG: Forma Hisui
  {  724,  1, 3             ,  12,  88, 117,  75,  80,  75,  95, T_GRASS   , T_FIGHT   , 156, 119, "H.DECIDUEYE" },  // DECIDUEYE: Forma Hisui
  {  801,  1, FORM_NO_REGION, 100,  80,  95, 115, 130, 115,  65, T_STEEL   , T_FAIRY   , 231, 113, "O.MAGEARNA" },  // MAGEARNA: Forma Color Vetusta
  {  849,  1, FORM_NO_REGION, 100,  75,  98,  70, 114,  70,  75, T_ELEC    , T_POISON  , 181, 121, "L.TOXTRICITY" },  // TOXTRICITY: Forma Grave
  {  978,  1, FORM_NO_REGION, 100,  68,  50,  60, 120,  95,  92, T_DRAGON  , T_WATER   ,  32,  84, "D.TATSUGIRI" },  // TATSUGIRI: Forma Lánguida
  {  978,  2, FORM_NO_REGION, 100,  68,  50,  60, 120,  95,  92, T_DRAGON  , T_WATER   ,  32,  84, "S.TATSUGIRI" },  // TATSUGIRI: Forma Recta
};

// La fila de esa forma de esa especie, o -1.
static inline int16_t formRow(int16_t dex, uint8_t form) {
  if (!form) return -1;            // 0 = la forma normal
  for (uint16_t i = 0; i < FORM_COUNT; i++)
    if (FORM_TBL[i].base == dex && FORM_TBL[i].form == form)
      return (int16_t)i;
  return -1;
}

// Que forma le toca a esta especie en esta region (0 = la normal).
//
// `roll` es una tirada 0..99 y hace falta porque no todas las
// formas son seguras: en Alola un Vulpix ES de Alola siempre, pero
// HISUI no es una region del juego — es la Sinnoh antigua — y sus
// formas salen alli solo de vez en cuando. Sin la tirada, en
// Sinnoh no habria un solo Growlithe normal.
//
// Las probabilidades se ACUMULAN porque una region puede traer
// varias variedades del mismo bicho: Paldea tiene tres Tauros, a
// 33/33/34. Volviendo en la primera que casaba salia siempre el
// mismo y los otros dos no existian para el jugador.
//
// Quien no quiera tirar (la ficha, los tests) pasa 0 y se lleva la
// primera forma que exista.
static inline uint8_t formForRegion(int16_t dex, uint8_t region,
                                    uint8_t roll = 0) {
  uint16_t acc = 0;
  for (uint16_t i = 0; i < FORM_COUNT; i++) {
    if (FORM_TBL[i].base != dex || FORM_TBL[i].region != region)
      continue;
    acc += FORM_TBL[i].chance;
    if (roll < acc) return FORM_TBL[i].form;
  }
  return 0;
}

// Al evolucionar se CONSERVA la forma si el destino la tiene: asi
// un Vulpix de Alola da un Ninetales de Alola sin una tabla de
// parejas. Si no la tiene, vuelve a la normal.
static inline uint8_t formAfterEvolve(int16_t to, uint8_t form) {
  return formRow(to, form) >= 0 ? form : 0;
}

// El nombre que hay que PINTAR: el de la forma si la tiene, y si
// no el de la especie en el idioma activo. Un sitio solo, para
// que no haya pantallas que digan "VULPIX" sobre un sprite de
// Alola. (El de la mega lo resuelve battle_ui, que es el unico
// sitio donde una mega existe.)
static inline const char *dexNameForm(int16_t dex, uint8_t form) {
  int16_t f = formRow(dex, form);
  return f >= 0 ? FORM_TBL[f].name : dexName(dex);
}

// Los PS contando la forma. Vive AQUI y no en dex.h porque forms.h
// necesita los T_* y el DEX_COUNT de dex.h: si dex.h mirara
// FORM_TBL, los dos se incluirian en circulo.
static inline uint16_t hpAtLevelForm(int16_t dex, uint16_t level,
                                     uint8_t trHp, uint8_t form) {
  int16_t f = formRow(dex, form);
  if (f < 0) return hpAtLevel(dex, level, trHp);
  return hpAtLevelBase(FORM_TBL[f].bHp, level, trHp);
}

// El stat BASE de una especie contando la forma. `cual` va en el
// orden de BASE_STATS: 0 PS, 1 Atq, 2 Def, 3 AtqEsp, 4 DefEsp, 5 Vel.
static inline uint16_t baseStatForm(int16_t dex, uint8_t form,
                                    uint8_t cual) {
  int16_t f = formRow(dex, form);
  if (f >= 0) {
    const FormVariant &v = FORM_TBL[f];
    switch (cual) {
      case 0: return v.bHp;
      case 1: return v.bAtk;
      case 2: return v.bDef;
      case 3: return v.bSpA;
      case 4: return v.bSpD;
      default: return v.bSpe;
    }
  }
  if (dex < 1 || dex > DEX_COUNT) return 1;
  const DexEntry &d = DEX_TBL[dex];
  switch (cual) {
    case 0: return d.bHp;
    case 1: return d.bAtk;
    case 2: return d.bDef;
    case 3: return d.bSpA;
    case 4: return d.bSpD;
    default: return d.bSpe;
  }
}

