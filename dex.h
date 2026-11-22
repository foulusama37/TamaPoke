#pragma once
#include <stdint.h>
#include "i18n.h"  // gLang

// GENERADO por tools/gen_dex.py desde tools/dex_data.py - no editar

#define DEX_COUNT 1025
#define DEX_REGIONS 9
static const int16_t DEX_REGION_END[DEX_REGIONS] = { 151, 251, 386, 493, 649, 721, 809, 905, 1025 };
#define DEX_KANTO 151  // compatibilidad: donde acaba la primera
// En que region cae una especie (0 = la primera). Para el techo de
// nivel y para que la hierba de cada region saque lo suyo.
static inline uint8_t dexRegionOf(int16_t dex) {
  for (uint8_t r = 0; r < DEX_REGIONS; r++)
    if (dex <= DEX_REGION_END[r]) return r;
  return DEX_REGIONS - 1;
}
// Bytes del bitmap de "ya lo criaste". Sale de DEX_COUNT para que
// anadir una region no deje el mapa corto en silencio.
#define DEX_REG_BYTES ((DEX_COUNT + 7) / 8)
// Para la cabecera "%s%s Lv.%u" de la ficha: el nombre mas largo
// de cualquier idioma (18 bytes en UTF-8), mas el prefijo,
// la parte fija del formato en el peor idioma (5 bytes), el
// nivel y el NUL. Sale de aqui para que los sitios que la pintan
// no puedan quedarse cortos al entrar una generacion con nombres
// mas largos.
//
// El nivel cuenta CINCO cifras y no tres: es la misma cuenta que
// hace el test de buffers (test/test_tools.py), que da por hecho
// que un %u puede traer un uint16_t entero. Dos cuentas distintas
// para el mismo buffer es justo lo que este #define vino a evitar.
//
// La parte fija se MIDE y no se supone: se dio por hecho que eran
// los cuatro bytes de " Lv." en los ocho idiomas, y en japones y
// coreano son mas, asi que el buffer se quedaba corto justo en los
// dos idiomas donde los nombres ya son los mas largos.
#define DEX_HEAD_BUF 30
// Las que se parten por SEXO: el macho va a otra cosa que la
// hembra, y `evolvesTo` solo guarda una (la de la hembra).
#define DEX_SEXADAS 1
struct DexSexada { int16_t from, male; };
static const DexSexada DEX_SEXADA[DEX_SEXADAS ? DEX_SEXADAS : 1] = { { 412, 414 } };
// En que evoluciona un MACHO de esta especie, o 0 si da igual.
static inline int16_t dexMaleBranch(int16_t dex) {
  for (uint8_t i = 0; i < DEX_SEXADAS; i++)
    if (DEX_SEXADA[i].from == dex) return DEX_SEXADA[i].male;
  return 0;
}

// Ramas de nivel que no decide ni el sexo ni un objeto. Salen
// del PBS: `evolvesTo` solo guarda una y el resto se perdian.
enum : uint8_t {
  RAMA_ATK_MAYOR = 0,  // Tyrogue con mas Ataque que Defensa
  RAMA_DEF_MAYOR,      // ...al reves
  RAMA_IGUALES,        // ...empatados
  RAMA_AZAR,           // Wurmple: a suertes, pero fijo por bicho
  RAMA_EXTRA,          // Nincada: ADEMAS de la normal (Shedinja)
};
struct EvoRama { int16_t from; uint8_t cond; int16_t to; };
#define EVO_RAMAS 4
static const EvoRama EVO_RAMA[EVO_RAMAS ? EVO_RAMAS : 1] = {
  {  236, RAMA_ATK_MAYOR,  106 },
  {  236, RAMA_DEF_MAYOR,  107 },
  {  265, RAMA_AZAR     ,  268 },
  {  290, RAMA_EXTRA    ,  292 },
};

// Lo que lleva encima un SALVAJE, por especie. Sale del PBS
// (WildItemCommon/Uncommon/Rare) y las probabilidades son las
// de los juegos: 50%, 5% y 1%. Un 0 es "nada".
//
// Sin esto, Frisk, Pickpocket, Anticipation, Ladron y Desarme
// solo tenian con que trabajar frente a un lider de gimnasio.
struct WildItem { int16_t dex; uint8_t comun, raro, rarisimo; };
#define WILD_ITEM_COUNT 153
static const WildItem WILD_ITEM[WILD_ITEM_COUNT ? WILD_ITEM_COUNT : 1] = {
  {   12,   0,  23,   0 },
  {   15,   0,  20,   0 },
  {   21,   0,  19,   0 },
  {   22,   0,  19,   0 },
  {   35,   0,  64,   0 },
  {   36,   0,  64,   0 },
  {   37,   0,  12,   0 },
  {   38,   0,  12,   0 },
  {   39,   0,  64,   0 },
  {   40,   0,  64,   0 },
  {   50,   0,  21,   0 },
  {   51,   0,  21,   0 },
  {   52,   0,   9,   0 },
  {   53,   0,   9,   0 },
  {   61,   0,  67,   0 },
  {   62,   0,  67,   0 },
  {   63,   0,  16,   0 },
  {   64,   0,  16,   0 },
  {   65,   0,  16,   0 },
  {   72,   0,  20,   0 },
  {   73,   0,  20,   0 },
  {   74,   0,  11,   0 },
  {   75,   0,  11,   0 },
  {   76,   0,  11,   0 },
  {   80,   0,  67,   0 },
  {   81,   0,  70,   0 },
  {   82,   0,  70,   0 },
  {   84,   0,  19,   0 },
  {   85,   0,  19,   0 },
  {  116,   0,  68,   0 },
  {  117,   0,  68,   0 },
  {  118,   0,  13,   0 },
  {  119,   0,  13,   0 },
  {  125,   0,  79,   0 },
  {  126,   0,  80,   0 },
  {  131,  13,  13,  13 },
  {  143,   1,   1,   1 },
  {  147,   0,  68,   0 },
  {  148,   0,  68,   0 },
  {  149,   0,  68,   0 },
  {  151,   4,   4,   4 },
  {  170,   0,  73,   0 },
  {  171,   0,  73,   0 },
  {  173,   0,  64,   0 },
  {  186,   0,  67,   0 },
  {  199,   0,  67,   0 },
  {  208,   0,  70,   0 },
  {  211,   0,  20,   0 },
  {  215,   0,   9,   0 },
  {  227,   0,  70,   0 },
  {  230,   0,  68,   0 },
  {  239,   0,  79,   0 },
  {  240,   0,  80,   0 },
  {  251,   4,   4,   4 },
  {  263,  28,  33,   0 },
  {  264,  28,  34,   0 },
  {  284,   0,  23,   0 },
  {  290,   0,  21,   0 },
  {  296,   0,  18,   0 },
  {  297,   0,  67,   0 },
  {  299,   0,  15,   0 },
  {  304,   0,  22,   0 },
  {  305,   0,  22,   0 },
  {  306,   0,  22,   0 },
  {  315,   0,  20,   0 },
  {  316,   3,   2,   0 },
  {  317,   3,   2,   0 },
  {  318,   0,  72,   0 },
  {  319,   0,  72,   0 },
  {  324,   0,  12,   0 },
  {  328,   0,  21,   0 },
  {  335,   0,   9,   0 },
  {  337,   0,  64,   0 },
  {  338,   0,  65,   0 },
  {  351,  13,  13,  13 },
  {  353,   0,  24,   0 },
  {  354,   0,  24,   0 },
  {  355,   0,  24,   0 },
  {  356,   0,  24,   0 },
  {  367,   0,  72,   0 },
  {  368,   0,  73,   0 },
  {  369,   0,  73,   0 },
  {  371,   0,  25,   0 },
  {  372,   0,  25,   0 },
  {  373,   0,  25,   0 },
  {  374,   0,  70,   0 },
  {  375,   0,  70,   0 },
  {  376,   0,  70,   0 },
  {  406,   0,  20,   0 },
  {  407,   0,  20,   0 },
  {  413,   0,  23,   0 },
  {  414,   0,  23,   0 },
  {  416,   0,  20,   0 },
  {  420,   0,  14,   0 },
  {  421,   0,  14,   0 },
  {  436,   0,  70,   0 },
  {  437,   0,  70,   0 },
  {  440,  83,   0,   0 },
  {  446,   1,   1,   1 },
  {  451,   0,  20,   0 },
  {  452,   0,  20,   0 },
  {  459,   0,  17,   0 },
  {  460,   0,  17,   0 },
  {  461,   0,   9,   0 },
  {  462,   0,  70,   0 },
  {  466,   0,  79,   0 },
  {  467,   0,  80,   0 },
  {  476,   0,  15,   0 },
  {  477,   0,  24,   0 },
  {  492,   4,   4,   4 },
  {  524,  11,  22,   0 },
  {  525,  11,  22,   0 },
  {  526,  11,  22,   0 },
  {  531,   3,   2,   0 },
  {  538,   0,  18,   0 },
  {  539,   0,  18,   0 },
  {  543,   0,  20,   0 },
  {  544,   0,  20,   0 },
  {  545,   0,  20,   0 },
  {  551,   0,  26,   0 },
  {  552,   0,  26,   0 },
  {  553,  26,   0,   0 },
  {  556,   0,  14,   0 },
  {  557,   0,  22,   0 },
  {  558,   0,  22,   0 },
  {  562,   0,  24,   0 },
  {  563,   0,  24,   0 },
  {  568,   0,  27,   0 },
  {  569,  27,   0,   0 },
  {  582,   0,  17,   0 },
  {  583,   0,  17,   0 },
  {  584,  17,   0,   0 },
  {  615,   0,  17,   0 },
  {  618,   0,  21,   0 },
  {  621,   0,  25,   0 },
  {  637,  23,  23,  23 },
  {  701,   0,  67,   0 },
  {  710,  14,  14,  14 },
  {  711,  14,  14,  14 },
  {  731,   0,   3,   0 },
  {  732,   0,   2,   0 },
  {  747,   0,  20,   0 },
  {  748,   0,  20,   0 },
  {  751,   0,  13,   0 },
  {  752,   0,  13,   0 },
  {  753,   0,  14,   0 },
  {  754,   0,  14,   0 },
  {  769,   0,  24,   0 },
  {  770,   0,  24,   0 },
  {  776,   0,  12,   0 },
  {  819,   0,   3,   0 },
  {  820,   0,   2,   0 },
  {  902,   0,  72,   0 },
};

// Tira el dado: que lleva encima ESTE salvaje (0 = nada).
static inline uint8_t rollWildItem(int16_t dex) {
  for (uint16_t i = 0; i < WILD_ITEM_COUNT; i++) {
    if (WILD_ITEM[i].dex != dex) continue;
    uint8_t t = (uint8_t)random(100);
    if (t < 1 && WILD_ITEM[i].rarisimo) return WILD_ITEM[i].rarisimo;
    if (t < 6 && WILD_ITEM[i].raro) return WILD_ITEM[i].raro;
    if (t < 56) return WILD_ITEM[i].comun;
    return 0;
  }
  return 0;
}

#define DEX_EEVEE 133
// Las ramas de Eevee. Van en una tabla y no como un rango 134..136
// porque con Johto dejaron de ser seguidas: Espeon y Umbreon estan
// en los 190 y con Sinnoh llegaron Leafeon y Glaceon en los 470.
// Quien recorra la linea evolutiva tiene que mirar aqui.
//
// La lista sale del PBS del Anil y NO esta escrita a mano: estaba, y
// se quedo en cinco cuando entraron las dos de Sinnoh.
static const int16_t EEVEE_BRANCH[] = { 134, 135, 136, 196, 197, 470, 471, 700 };
#define EEVEE_BRANCHES (sizeof(EEVEE_BRANCH) / sizeof(EEVEE_BRANCH[0]))

// Tipos de GEN 1 para el combate. OJO: el campo 'tipo' de dex_data.py
// NO sirve para esto: son 14, no existe volador y cada especie tiene uno
// solo, porque se uso para elegir color de UI y bioma de fondo. Estos
// salen de PokeAPI con la tipificacion de gen 1 (gen_types.py).
enum : uint8_t {
  T_NONE, T_NORMAL, T_FIGHT, T_FLYING, T_POISON, T_GROUND, T_ROCK, T_BUG, T_GHOST, T_FIRE, T_WATER, T_GRASS, T_ELEC, T_PSYCHIC, T_ICE, T_DRAGON, T_DARK, T_STEEL, T_FAIRY, T_COUNT
};

// Efectividad x100 (0, 50, 100, 200) de [atacante][defensor].
// Se conservan las rarezas de gen 1 (veneno<->bicho x2, hielo->fuego x1),
// pero NO el bug de fantasma contra psiquico, que hacia cero y dejaba a
// los psiquicos intocables. Ver tools/type_chart.py para cambiarlo.
static const uint8_t TYPE_CHART[T_COUNT][T_COUNT] = {
  { 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },  // none
  { 100, 100, 100, 100, 100, 100,  50, 100,   0, 100, 100, 100, 100, 100, 100, 100, 100,  50, 100 },  // normal
  { 100, 200, 100,  50,  50, 100, 200,  50,   0, 100, 100, 100, 100,  50, 200, 100, 200, 200,  50 },  // fighting
  { 100, 100, 200, 100, 100, 100,  50, 200, 100, 100, 100, 200,  50, 100, 100, 100, 100,  50, 100 },  // flying
  { 100, 100, 100, 100,  50,  50,  50, 100,  50, 100, 100, 200, 100, 100, 100, 100, 100,   0, 200 },  // poison
  { 100, 100, 100,   0, 200, 100, 200,  50, 100, 200, 100,  50, 200, 100, 100, 100, 100, 200, 100 },  // ground
  { 100, 100,  50, 200, 100,  50, 100, 200, 100, 200, 100, 100, 100, 100, 200, 100, 100,  50, 100 },  // rock
  { 100, 100,  50,  50,  50, 100, 100, 100,  50,  50, 100, 200, 100, 200, 100, 100, 200,  50,  50 },  // bug
  { 100,   0, 100, 100, 100, 100, 100, 100, 200, 100, 100, 100, 100, 200, 100, 100,  50, 100, 100 },  // ghost
  { 100, 100, 100, 100, 100, 100,  50, 200, 100,  50,  50, 200, 100, 100, 200,  50, 100, 200, 100 },  // fire
  { 100, 100, 100, 100, 100, 200, 200, 100, 100, 200,  50,  50, 100, 100, 100,  50, 100, 100, 100 },  // water
  { 100, 100, 100,  50,  50, 200, 200,  50, 100,  50, 200,  50, 100, 100, 100,  50, 100,  50, 100 },  // grass
  { 100, 100, 100, 200, 100,   0, 100, 100, 100, 100, 200,  50,  50, 100, 100,  50, 100, 100, 100 },  // electric
  { 100, 100, 200, 100, 200, 100, 100, 100, 100, 100, 100, 100, 100,  50, 100, 100,   0,  50, 100 },  // psychic
  { 100, 100, 100, 200, 100, 200, 100, 100, 100,  50,  50, 200, 100, 100,  50, 200, 100,  50, 100 },  // ice
  { 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 200, 100,  50,   0 },  // dragon
  { 100, 100,  50, 100, 100, 100, 100, 100, 200, 100, 100, 100, 100, 200, 100, 100,  50, 100,  50 },  // dark
  { 100, 100, 100, 100, 100, 100, 200, 100, 100,  50,  50, 100,  50, 100, 200, 100, 100,  50, 200 },  // steel
  { 100, 100, 200, 100,  50, 100, 100, 100, 100,  50, 100, 100, 100, 100, 100, 200, 200,  50, 100 },  // fairy
};

// Multiplicador x100 de un golpe de tipo `atk` contra un bicho de tipos
// d1/d2. Con dos tipos se multiplican, asi que sale 0, 25, 50, 100, 200 o 400.
static inline uint16_t typeEffect(uint8_t atk, uint8_t d1, uint8_t d2) {
  if (atk >= T_COUNT || d1 >= T_COUNT) return 100;
  uint16_t e = TYPE_CHART[atk][d1];
  if (d2 && d2 < T_COUNT) e = (uint16_t)(e * TYPE_CHART[atk][d2] / 100);
  return e;
}

// Etiqueta corta y color de cada tipo, para el chip de la tarjeta de
// golpe. Cortas a proposito: la tarjeta mide 257 px y ya lleva flechas,
// asi que FIGHTING no cabe pero FIGHT si.
static const char *const TYPE_NAME[T_COUNT] = {
  "---",
  "NORMAL",
  "FIGHT",
  "FLY",
  "POISON",
  "GROUND",
  "ROCK",
  "BUG",
  "GHOST",
  "FIRE",
  "WATER",
  "GRASS",
  "ELEC",
  "PSY",
  "ICE",
  "DRAGON",
  "DARK",
  "STEEL",
  "FAIRY",
};
static const uint16_t TYPE_COLOR[T_COUNT] = {
  0x8C51,  // none
  0xAD4F,  // normal
  0xC185,  // fighting
  0xAC9E,  // flying
  0xA214,  // poison
  0xE60D,  // ground
  0xBD07,  // rock
  0xADC4,  // bug
  0x72D3,  // ghost
  0xF406,  // fire
  0x6C9E,  // water
  0x7E4A,  // grass
  0xFE86,  // electric
  0xFAD1,  // psychic
  0x9EDB,  // ice
  0x71DF,  // dragon
  0x72C9,  // dark
  0xBDDA,  // steel
  0xECD5,  // fairy
};
static inline const char *typeName(uint8_t t) {
  return t < T_COUNT ? TYPE_NAME[t] : TYPE_NAME[T_NONE];
}
static inline uint16_t typeColor(uint8_t t) {
  return t < T_COUNT ? TYPE_COLOR[t] : TYPE_COLOR[T_NONE];
}

// rareza: 0 = solo por evolucion, 1 = comun, 2 = raro, 3 = legendario
enum : uint8_t { R_EVO = 0, R_COMUN, R_RARO, R_LEGENDARIO };

struct DexEntry {
  const char *name;
  // uint16_t y no uint8_t: el numero de dex se pasa de 255 en
  // cuanto entre la gen 2, y entonces las evoluciones altas
  // apuntarian a cualquier sitio en silencio. Cuesta 151 bytes de
  // flash y quita un bloqueo de encima.
  uint16_t evolvesTo;   // numero de dex, 0 = forma final
  uint8_t evolveLevel;
  uint8_t rarity;       // sale de huevo si > 0
  uint16_t accent;      // color RGB565 del tipo para la UI
  // los SEIS stats base. Antes solo habia cuatro (faltaban los especiales)
  // y sin ellos no se puede calcular el dano de medio catalogo de golpes.
  uint8_t bHp, bAtk, bDef, bSpA, bSpD, bSpe;
  uint8_t biome;        // 0 pradera 1 playa 2 bosque 3 volcan 4 montana 5 nieve
  // Alto en pantalla, segun la altura real Y LA DENSIDAD del sprite
  // (gen_sizes.py). uint16_t y no uint8_t: la correccion por densidad
  // tiene que pasar de 255 en los sprites muy vacios. Pidgeotto esta
  // dibujado VOLANDO —— 84x116 de lienzo con solo el 30% de tinta ——
  // y necesita 270 para que su CABEZA se vea del tamano de la de
  // Pidgey; con el byte se topaba en 255 justo donde hacia falta.
  // Son 1025 bytes mas de flash sobre 3 MB.
  uint16_t screenSz;
  // El PESO, en decenas de kilo (asi un Groudon de 950 kg cabe
  // en un byte). Lo piden dos golpes y no se podian hacer sin
  // el: Patada Baja pega segun lo que pese el rival y Cuerpo
  // Pesado segun cuanto mas peses tu. Sale del PBS.
  uint8_t weight10;
  uint8_t type1, type2; // tipos de GEN 1 para combatir; type2 = T_NONE si es puro
  // Probabilidad de salir HEMBRA, en %. 255 = SIN SEXO, que no es
  // lo mismo que 0 (0 es "siempre macho"). Sale del GenderRatio
  // del Anil. Hace falta para las evoluciones que piden sexo
  // (Snorunt da Froslass solo si es hembra, Burmy se parte en
  // Wormadam y Mothim) y para elegir el sprite de hembra.
  uint8_t femPct;
};

// Sin sexo: ni macho ni hembra. Son 155 (Magnemite, Voltorb, los
// legendarios...), asi que no es un caso raro.
#define GENDERLESS 255

// Los que son evolucion de alguien Y ADEMAS salen por su cuenta.
// Es una decision, no un descuido: en los juegos tampoco hay que
// criar un Pichu para encontrarse un Pikachu, ni un Happiny para
// ver una Chansey. Se emite para que los tests puedan distinguir
// esto de un agujero de verdad (ALSO_WILD en dex_data.py).
#define DEX_ALSO_WILD_COUNT 15
static const int16_t DEX_ALSO_WILD[DEX_ALSO_WILD_COUNT] = { 25, 35, 39, 113, 122, 124, 125, 126, 143, 183, 185, 202, 226, 315, 358 };
static inline bool dexAlsoWild(int16_t dex) {
  for (uint8_t i = 0; i < DEX_ALSO_WILD_COUNT; i++)
    if (DEX_ALSO_WILD[i] == dex) return true;
  return false;
}

static const DexEntry DEX_TBL[DEX_COUNT + 1] = {
  { "?", 0, 0, 0, 0x2946, 50, 50, 50, 50, 50, 50, 0, 120, 1, T_NONE, T_NONE, 255 },  // 0: sin usar (255 = GENDERLESS)
  { "BULBASAUR", 2, 16, R_COMUN, 0x3C49, 45, 49, 49, 65, 65, 45, 2, 93, 1, T_GRASS, T_POISON, 50 },  // 1 planta 0.7m
  { "IVYSAUR", 3, 32, R_EVO, 0x3C49, 60, 62, 63, 80, 80, 60, 2, 114, 1, T_GRASS, T_POISON, 50 },  // 2 planta 1.0m
  { "VENUSAUR", 0, 0, R_EVO, 0x3C49, 80, 82, 83, 100, 100, 80, 2, 146, 10, T_GRASS, T_POISON, 50 },  // 3 planta 2.0m
  { "CHARMANDER", 5, 16, R_COMUN, 0xEA87, 39, 52, 43, 60, 50, 65, 3, 95, 1, T_FIRE, T_NONE, 50 },  // 4 fuego 0.6m
  { "CHARMELEON", 6, 36, R_EVO, 0xEA87, 58, 64, 58, 80, 65, 80, 3, 110, 2, T_FIRE, T_NONE, 50 },  // 5 fuego 1.1m
  { "CHARIZARD", 0, 0, R_EVO, 0xEA87, 78, 84, 78, 109, 85, 100, 3, 143, 9, T_FIRE, T_FLYING, 50 },  // 6 fuego 1.7m
  { "SQUIRTLE", 8, 16, R_COMUN, 0x4C98, 44, 48, 65, 50, 64, 43, 1, 95, 1, T_WATER, T_NONE, 50 },  // 7 agua 0.5m
  { "WARTORTLE", 9, 36, R_EVO, 0x4C98, 59, 63, 80, 65, 80, 58, 1, 118, 2, T_WATER, T_NONE, 50 },  // 8 agua 1.0m
  { "BLASTOISE", 0, 0, R_EVO, 0x4C98, 79, 83, 100, 85, 105, 78, 1, 129, 9, T_WATER, T_NONE, 50 },  // 9 agua 1.6m
  { "CATERPIE", 11, 9, R_COMUN, 0x7CC4, 45, 40, 35, 20, 20, 45, 2, 88, 1, T_BUG, T_NONE, 50 },  // 10 bicho 0.3m
  { "METAPOD", 12, 16, R_EVO, 0x7CC4, 50, 60, 55, 25, 25, 30, 2, 92, 1, T_BUG, T_NONE, 50 },  // 11 bicho 0.7m
  { "BUTTERFREE", 0, 0, R_EVO, 0x7CC4, 60, 45, 50, 100, 100, 90, 2, 116, 3, T_BUG, T_FLYING, 50 },  // 12 bicho 1.1m
  { "WEEDLE", 14, 9, R_COMUN, 0x7CC4, 40, 45, 30, 20, 20, 50, 2, 84, 1, T_BUG, T_POISON, 50 },  // 13 bicho 0.3m
  { "KAKUNA", 15, 16, R_EVO, 0x7CC4, 45, 65, 50, 25, 25, 35, 2, 85, 1, T_BUG, T_POISON, 50 },  // 14 bicho 0.6m
  { "BEEDRILL", 0, 0, R_EVO, 0x7CC4, 65, 100, 40, 65, 80, 95, 2, 109, 3, T_BUG, T_POISON, 50 },  // 15 bicho 1.0m
  { "PIDGEY", 17, 18, R_COMUN, 0x8C4D, 40, 50, 40, 45, 35, 56, 0, 92, 1, T_NORMAL, T_FLYING, 50 },  // 16 normal 0.3m
  { "PIDGEOTTO", 18, 36, R_EVO, 0x8C4D, 63, 83, 55, 75, 50, 81, 0, 130, 3, T_NORMAL, T_FLYING, 50 },  // 17 normal 1.1m
  { "PIDGEOT", 0, 0, R_EVO, 0x8C4D, 83, 95, 75, 105, 70, 100, 0, 126, 4, T_NORMAL, T_FLYING, 50 },  // 18 normal 1.5m
  { "RATTATA", 20, 20, R_COMUN, 0x8C4D, 30, 56, 35, 25, 35, 72, 0, 91, 1, T_NORMAL, T_NONE, 50 },  // 19 normal 0.3m
  { "RATICATE", 0, 0, R_EVO, 0x8C4D, 65, 100, 60, 50, 70, 110, 0, 114, 2, T_NORMAL, T_NONE, 50 },  // 20 normal 0.7m
  { "SPEAROW", 22, 20, R_COMUN, 0x8C4D, 40, 60, 30, 31, 31, 70, 0, 92, 1, T_NORMAL, T_FLYING, 50 },  // 21 normal 0.3m
  { "FEAROW", 0, 0, R_EVO, 0x8C4D, 65, 115, 65, 61, 61, 100, 0, 133, 4, T_NORMAL, T_FLYING, 50 },  // 22 normal 1.2m
  { "EKANS", 24, 22, R_COMUN, 0x8A73, 35, 60, 44, 40, 54, 55, 0, 101, 1, T_POISON, T_NONE, 50 },  // 23 veneno 2.0m
  { "ARBOK", 0, 0, R_EVO, 0x8A73, 70, 95, 69, 65, 79, 95, 0, 126, 6, T_POISON, T_DARK, 50 },  // 24 veneno 3.5m
  { "PIKACHU", 26, 0, R_COMUN, 0xBCA1, 35, 55, 30, 50, 40, 90, 0, 101, 1, T_ELEC, T_NONE, 50 },  // 25 electrico 0.4m
  { "RAICHU", 0, 0, R_EVO, 0xBCA1, 60, 90, 55, 90, 80, 110, 0, 118, 3, T_ELEC, T_NONE, 50 },  // 26 electrico 0.8m
  { "SANDSHREW", 28, 22, R_COMUN, 0xB447, 50, 75, 85, 20, 30, 40, 4, 94, 1, T_GROUND, T_NONE, 50 },  // 27 tierra 0.6m
  { "SANDSLASH", 0, 0, R_EVO, 0xB447, 75, 110, 110, 45, 55, 70, 4, 112, 3, T_GROUND, T_NONE, 50 },  // 28 tierra 1.0m
  { "NIDORAN H", 30, 16, R_COMUN, 0x8A73, 55, 47, 52, 40, 40, 41, 0, 92, 1, T_POISON, T_NONE, 100 },  // 29 veneno 0.4m
  { "NIDORINA", 31, 0, R_EVO, 0x8A73, 70, 62, 67, 55, 55, 56, 0, 106, 2, T_POISON, T_NONE, 100 },  // 30 veneno 0.8m
  { "NIDOQUEEN", 0, 0, R_EVO, 0x8A73, 90, 92, 87, 75, 85, 76, 0, 131, 6, T_POISON, T_GROUND, 100 },  // 31 veneno 1.3m
  { "NIDORAN M", 33, 16, R_COMUN, 0x8A73, 46, 57, 40, 40, 40, 50, 0, 93, 1, T_POISON, T_NONE, 0 },  // 32 veneno 0.5m
  { "NIDORINO", 34, 0, R_EVO, 0x8A73, 61, 72, 57, 55, 55, 65, 0, 112, 2, T_POISON, T_NONE, 0 },  // 33 veneno 0.9m
  { "NIDOKING", 0, 0, R_EVO, 0x8A73, 81, 102, 77, 85, 75, 85, 0, 134, 6, T_POISON, T_GROUND, 0 },  // 34 veneno 1.4m
  { "CLEFAIRY", 36, 0, R_COMUN, 0xDC76, 70, 45, 48, 60, 65, 35, 0, 103, 1, T_FAIRY, T_NONE, 75 },  // 35 hada 0.6m
  { "CLEFABLE", 0, 0, R_EVO, 0xDC76, 95, 70, 73, 95, 90, 60, 0, 117, 4, T_FAIRY, T_NONE, 75 },  // 36 hada 1.3m
  { "VULPIX", 38, 0, R_COMUN, 0xEA87, 38, 41, 40, 60, 65, 65, 3, 110, 1, T_FIRE, T_NONE, 75 },  // 37 fuego 0.6m
  { "NINETALES", 0, 0, R_EVO, 0xEA87, 73, 76, 75, 100, 100, 100, 3, 140, 2, T_FIRE, T_NONE, 75 },  // 38 fuego 1.1m
  { "JIGGLYPUFF", 40, 0, R_COMUN, 0x8C4D, 115, 45, 20, 45, 25, 20, 0, 94, 1, T_NORMAL, T_FAIRY, 75 },  // 39 normal 0.5m
  { "WIGGLYTUFF", 0, 0, R_EVO, 0x8C4D, 140, 65, 60, 85, 60, 45, 0, 110, 1, T_NORMAL, T_FAIRY, 75 },  // 40 normal 1.0m
  { "ZUBAT", 42, 22, R_COMUN, 0x8A73, 40, 55, 35, 30, 40, 75, 0, 98, 1, T_POISON, T_FLYING, 50 },  // 41 veneno 0.8m
  { "GOLBAT", 169, 35, R_EVO, 0x8A73, 75, 80, 70, 65, 75, 100, 0, 124, 6, T_POISON, T_FLYING, 50 },  // 42 veneno 1.6m
  { "ODDISH", 44, 21, R_COMUN, 0x3C49, 45, 50, 55, 75, 65, 30, 2, 89, 1, T_GRASS, T_POISON, 50 },  // 43 planta 0.5m
  { "GLOOM", 45, 0, R_EVO, 0x3C49, 60, 65, 70, 85, 75, 40, 2, 107, 1, T_GRASS, T_POISON, 50 },  // 44 planta 0.8m
  { "VILEPLUME", 0, 0, R_EVO, 0x3C49, 75, 80, 85, 110, 95, 50, 2, 116, 2, T_GRASS, T_POISON, 50 },  // 45 planta 1.2m
  { "PARAS", 47, 22, R_COMUN, 0x7CC4, 65, 70, 55, 45, 55, 25, 2, 99, 1, T_BUG, T_GRASS, 50 },  // 46 bicho 0.3m
  { "PARASECT", 0, 0, R_EVO, 0x7CC4, 95, 100, 80, 60, 90, 30, 2, 116, 3, T_BUG, T_GRASS, 50 },  // 47 bicho 1.0m
  { "VENONAT", 49, 31, R_COMUN, 0x7CC4, 60, 55, 50, 60, 55, 45, 2, 108, 3, T_BUG, T_POISON, 50 },  // 48 bicho 1.0m
  { "VENOMOTH", 0, 0, R_EVO, 0x7CC4, 70, 65, 60, 100, 90, 90, 2, 119, 1, T_BUG, T_POISON, 50 },  // 49 bicho 1.5m
  { "DIGLETT", 51, 26, R_COMUN, 0xB447, 30, 60, 35, 35, 45, 95, 4, 84, 1, T_GROUND, T_NONE, 50 },  // 50 tierra 0.2m
  { "DUGTRIO", 0, 0, R_EVO, 0xB447, 65, 100, 70, 50, 70, 120, 4, 103, 3, T_GROUND, T_NONE, 50 },  // 51 tierra 0.7m
  { "MEOWTH", 53, 28, R_COMUN, 0x8C4D, 40, 65, 35, 40, 40, 90, 0, 107, 1, T_NORMAL, T_NONE, 50 },  // 52 normal 0.4m
  { "PERSIAN", 0, 0, R_EVO, 0x8C4D, 75, 105, 60, 85, 65, 115, 0, 119, 3, T_NORMAL, T_NONE, 50 },  // 53 normal 1.0m
  { "PSYDUCK", 55, 30, R_COMUN, 0x4C98, 60, 52, 48, 65, 50, 55, 1, 100, 2, T_WATER, T_NONE, 50 },  // 54 agua 0.8m
  { "GOLDUCK", 0, 0, R_EVO, 0x4C98, 80, 92, 78, 95, 80, 85, 1, 113, 8, T_WATER, T_PSYCHIC, 50 },  // 55 agua 1.7m
  { "MANKEY", 57, 28, R_COMUN, 0xA2A5, 40, 80, 35, 35, 45, 70, 0, 109, 3, T_FIGHT, T_NONE, 50 },  // 56 lucha 0.5m
  { "PRIMEAPE", 979, 44, R_EVO, 0xA2A5, 65, 115, 60, 60, 70, 105, 0, 117, 3, T_FIGHT, T_NONE, 50 },  // 57 lucha 1.0m
  { "GROWLITHE", 59, 0, R_RARO, 0xEA87, 55, 70, 45, 70, 50, 60, 3, 107, 2, T_FIRE, T_NONE, 25 },  // 58 fuego 0.7m
  { "ARCANINE", 0, 0, R_EVO, 0xEA87, 90, 110, 80, 100, 80, 95, 3, 136, 16, T_FIRE, T_NONE, 25 },  // 59 fuego 1.9m
  { "POLIWAG", 61, 22, R_COMUN, 0x4C98, 55, 60, 40, 55, 50, 75, 1, 97, 1, T_WATER, T_NONE, 50 },  // 60 agua 0.6m
  { "POLIWHIRL", 62, 0, R_EVO, 0x4C98, 65, 80, 65, 65, 55, 80, 1, 113, 2, T_WATER, T_NONE, 50 },  // 61 agua 1.0m
  { "POLIWRATH", 0, 0, R_EVO, 0x4C98, 90, 95, 95, 70, 90, 70, 1, 127, 5, T_WATER, T_FIGHT, 50 },  // 62 agua 1.3m
  { "ABRA", 64, 16, R_COMUN, 0xD28F, 25, 20, 15, 105, 55, 90, 0, 102, 2, T_PSYCHIC, T_NONE, 25 },  // 63 psiquico 0.9m
  { "KADABRA", 65, 45, R_EVO, 0xD28F, 40, 35, 30, 120, 70, 105, 0, 123, 6, T_PSYCHIC, T_NONE, 25 },  // 64 psiquico 1.3m
  { "ALAKAZAM", 0, 0, R_EVO, 0xD28F, 55, 50, 45, 135, 95, 120, 0, 130, 5, T_PSYCHIC, T_NONE, 25 },  // 65 psiquico 1.5m
  { "MACHOP", 67, 26, R_COMUN, 0xA2A5, 70, 80, 50, 35, 35, 35, 0, 103, 2, T_FIGHT, T_NONE, 25 },  // 66 lucha 0.8m
  { "MACHOKE", 68, 45, R_EVO, 0xA2A5, 80, 100, 70, 50, 60, 45, 0, 121, 7, T_FIGHT, T_NONE, 25 },  // 67 lucha 1.5m
  { "MACHAMP", 0, 0, R_EVO, 0xA2A5, 90, 130, 80, 65, 85, 55, 0, 130, 13, T_FIGHT, T_NONE, 25 },  // 68 lucha 1.6m
  { "BELLSPROUT", 70, 21, R_COMUN, 0x3C49, 50, 75, 35, 70, 30, 40, 2, 93, 1, T_GRASS, T_POISON, 50 },  // 69 planta 0.7m
  { "WEEPINBELL", 71, 0, R_EVO, 0x3C49, 65, 90, 50, 85, 45, 55, 2, 107, 1, T_GRASS, T_POISON, 50 },  // 70 planta 1.0m
  { "VICTREEBEL", 0, 0, R_EVO, 0x3C49, 80, 110, 70, 95, 70, 70, 2, 122, 2, T_GRASS, T_POISON, 50 },  // 71 planta 1.7m
  { "TENTACOOL", 73, 30, R_COMUN, 0x4C98, 40, 40, 35, 50, 100, 70, 1, 105, 5, T_WATER, T_POISON, 50 },  // 72 agua 0.9m
  { "TENTACRUEL", 0, 0, R_EVO, 0x4C98, 80, 70, 65, 80, 120, 100, 1, 123, 6, T_WATER, T_POISON, 50 },  // 73 agua 1.6m
  { "GEODUDE", 75, 25, R_COMUN, 0x9407, 50, 80, 100, 30, 30, 20, 4, 96, 2, T_ROCK, T_GROUND, 50 },  // 74 roca 0.4m
  { "GRAVELER", 76, 45, R_EVO, 0x9407, 80, 95, 115, 45, 45, 35, 4, 115, 10, T_ROCK, T_GROUND, 50 },  // 75 roca 1.0m
  { "GOLEM", 0, 0, R_EVO, 0x9407, 110, 120, 130, 55, 65, 45, 4, 124, 30, T_ROCK, T_GROUND, 50 },  // 76 roca 1.4m
  { "PONYTA", 78, 40, R_RARO, 0xEA87, 50, 85, 55, 65, 65, 90, 3, 111, 3, T_FIRE, T_NONE, 50 },  // 77 fuego 1.0m
  { "RAPIDASH", 0, 0, R_EVO, 0xEA87, 85, 110, 70, 80, 80, 105, 3, 134, 10, T_FIRE, T_NONE, 50 },  // 78 fuego 1.7m
  { "SLOWPOKE", 80, 37, R_COMUN, 0x4C98, 90, 65, 65, 40, 40, 15, 1, 110, 4, T_WATER, T_PSYCHIC, 50 },  // 79 agua 1.2m
  { "SLOWBRO", 0, 0, R_EVO, 0x4C98, 95, 75, 110, 100, 80, 30, 1, 124, 8, T_WATER, T_PSYCHIC, 50 },  // 80 agua 1.6m
  { "MAGNEMITE", 82, 30, R_COMUN, 0xBCA1, 25, 35, 70, 95, 55, 45, 0, 80, 1, T_ELEC, T_STEEL, 255 },  // 81 electrico 0.3m
  { "MAGNETON", 462, 45, R_EVO, 0xBCA1, 50, 60, 95, 120, 70, 70, 0, 116, 6, T_ELEC, T_STEEL, 255 },  // 82 electrico 1.0m
  { "FARFETCHD", 0, 0, R_RARO, 0x8C4D, 85, 100, 75, 60, 85, 95, 0, 107, 2, T_FLYING, T_NORMAL, 50 },  // 83 normal 0.8m
  { "DODUO", 85, 31, R_COMUN, 0x8C4D, 35, 85, 45, 35, 35, 75, 0, 103, 4, T_NORMAL, T_FLYING, 50 },  // 84 normal 1.4m
  { "DODRIO", 0, 0, R_EVO, 0x8C4D, 70, 110, 70, 60, 60, 110, 0, 123, 9, T_NORMAL, T_FLYING, 50 },  // 85 normal 1.8m
  { "SEEL", 87, 34, R_COMUN, 0x4C98, 65, 45, 55, 45, 70, 45, 1, 108, 9, T_WATER, T_NONE, 50 },  // 86 agua 1.1m
  { "DEWGONG", 0, 0, R_EVO, 0x4C98, 90, 90, 80, 90, 95, 70, 1, 126, 12, T_WATER, T_ICE, 50 },  // 87 agua 1.7m
  { "GRIMER", 89, 32, R_RARO, 0x8A73, 80, 60, 50, 60, 50, 25, 0, 104, 3, T_POISON, T_NONE, 50 },  // 88 veneno 0.9m
  { "MUK", 0, 0, R_EVO, 0x8A73, 105, 105, 75, 65, 100, 50, 0, 131, 3, T_POISON, T_NONE, 50 },  // 89 veneno 1.2m
  { "SHELLDER", 91, 0, R_COMUN, 0x4C98, 30, 65, 100, 50, 25, 50, 1, 93, 1, T_WATER, T_NONE, 50 },  // 90 agua 0.3m
  { "CLOYSTER", 0, 0, R_EVO, 0x4C98, 50, 95, 180, 85, 45, 70, 1, 124, 13, T_WATER, T_ICE, 50 },  // 91 agua 1.5m
  { "GASTLY", 93, 25, R_COMUN, 0x6AD3, 30, 35, 30, 100, 35, 80, 0, 119, 1, T_GHOST, T_POISON, 50 },  // 92 fantasma 1.3m
  { "HAUNTER", 94, 45, R_EVO, 0x6AD3, 45, 50, 45, 115, 55, 95, 0, 122, 1, T_GHOST, T_POISON, 50 },  // 93 fantasma 1.6m
  { "GENGAR", 0, 0, R_EVO, 0x6AD3, 60, 65, 60, 130, 75, 110, 0, 119, 4, T_GHOST, T_POISON, 50 },  // 94 fantasma 1.5m
  { "ONIX", 208, 42, R_RARO, 0x9407, 55, 60, 160, 30, 45, 70, 4, 132, 21, T_ROCK, T_GROUND, 50 },  // 95 roca 8.8m
  { "DROWZEE", 97, 26, R_COMUN, 0xD28F, 60, 48, 45, 63, 90, 42, 0, 113, 3, T_PSYCHIC, T_NONE, 50 },  // 96 psiquico 1.0m
  { "HYPNO", 0, 0, R_EVO, 0xD28F, 85, 80, 70, 95, 115, 67, 0, 119, 8, T_PSYCHIC, T_NONE, 50 },  // 97 psiquico 1.6m
  { "KRABBY", 99, 28, R_COMUN, 0x4C98, 30, 105, 90, 40, 35, 50, 1, 102, 1, T_WATER, T_NONE, 50 },  // 98 agua 0.4m
  { "KINGLER", 0, 0, R_EVO, 0x4C98, 75, 130, 115, 50, 50, 75, 1, 126, 6, T_WATER, T_NONE, 50 },  // 99 agua 1.3m
  { "VOLTORB", 101, 30, R_COMUN, 0xBCA1, 40, 30, 50, 55, 55, 100, 0, 88, 1, T_ELEC, T_NONE, 255 },  // 100 electrico 0.5m
  { "ELECTRODE", 0, 0, R_EVO, 0xBCA1, 60, 50, 70, 90, 80, 150, 0, 111, 7, T_ELEC, T_NONE, 255 },  // 101 electrico 1.2m
  { "EXEGGCUTE", 103, 0, R_COMUN, 0x3C49, 60, 40, 80, 60, 45, 40, 2, 108, 1, T_GRASS, T_PSYCHIC, 50 },  // 102 planta 0.4m
  { "EXEGGUTOR", 0, 0, R_EVO, 0x3C49, 95, 95, 85, 125, 80, 55, 2, 129, 12, T_GRASS, T_PSYCHIC, 50 },  // 103 planta 2.0m
  { "CUBONE", 105, 28, R_COMUN, 0xB447, 50, 70, 95, 40, 50, 55, 4, 95, 1, T_GROUND, T_NONE, 50 },  // 104 tierra 0.4m
  { "MAROWAK", 0, 0, R_EVO, 0xB447, 60, 80, 110, 50, 80, 65, 4, 106, 4, T_GROUND, T_NONE, 50 },  // 105 tierra 1.0m
  { "HITMONLEE", 0, 0, R_RARO, 0xA2A5, 60, 120, 53, 35, 110, 87, 0, 110, 5, T_FIGHT, T_NONE, 0 },  // 106 lucha 1.5m
  { "HITMONCHAN", 0, 0, R_RARO, 0xA2A5, 60, 105, 79, 35, 110, 76, 0, 101, 5, T_FIGHT, T_NONE, 0 },  // 107 lucha 1.4m
  { "LICKITUNG", 463, 35, R_RARO, 0x8C4D, 90, 55, 75, 60, 75, 30, 0, 121, 7, T_NORMAL, T_NONE, 50 },  // 108 normal 1.2m
  { "KOFFING", 110, 30, R_COMUN, 0x8A73, 40, 65, 95, 60, 45, 35, 0, 99, 1, T_POISON, T_NONE, 50 },  // 109 veneno 0.6m
  { "WEEZING", 0, 0, R_EVO, 0x8A73, 65, 90, 120, 95, 70, 60, 0, 136, 1, T_POISON, T_NONE, 50 },  // 110 veneno 1.2m
  { "RHYHORN", 112, 42, R_RARO, 0xB447, 80, 85, 95, 30, 30, 25, 4, 124, 12, T_GROUND, T_ROCK, 50 },  // 111 tierra 1.0m
  { "RHYDON", 464, 50, R_EVO, 0xB447, 105, 130, 120, 45, 45, 40, 4, 133, 12, T_GROUND, T_ROCK, 50 },  // 112 tierra 1.9m
  { "CHANSEY", 242, 35, R_RARO, 0x8C4D, 250, 5, 5, 35, 105, 50, 0, 109, 3, T_NORMAL, T_NONE, 100 },  // 113 normal 1.1m
  { "TANGELA", 465, 40, R_RARO, 0x3C49, 65, 55, 115, 100, 40, 60, 2, 106, 4, T_GRASS, T_NONE, 50 },  // 114 planta 1.0m
  { "KANGASKHAN", 0, 0, R_RARO, 0x8C4D, 105, 95, 80, 40, 80, 90, 0, 132, 8, T_NORMAL, T_NONE, 100 },  // 115 normal 2.2m
  { "HORSEA", 117, 32, R_COMUN, 0x4C98, 30, 40, 70, 70, 25, 60, 1, 86, 1, T_WATER, T_NONE, 50 },  // 116 agua 0.4m
  { "SEADRA", 230, 50, R_EVO, 0x4C98, 55, 65, 95, 95, 45, 85, 1, 106, 2, T_WATER, T_NONE, 50 },  // 117 agua 1.2m
  { "GOLDEEN", 119, 33, R_COMUN, 0x4C98, 45, 67, 60, 35, 50, 63, 1, 107, 2, T_WATER, T_NONE, 50 },  // 118 agua 0.6m
  { "SEAKING", 0, 0, R_EVO, 0x4C98, 80, 110, 75, 65, 80, 80, 1, 119, 4, T_WATER, T_NONE, 50 },  // 119 agua 1.3m
  { "STARYU", 121, 0, R_COMUN, 0x4C98, 30, 45, 55, 70, 55, 85, 1, 94, 3, T_WATER, T_NONE, 255 },  // 120 agua 0.8m
  { "STARMIE", 0, 0, R_EVO, 0x4C98, 60, 75, 85, 100, 85, 115, 1, 112, 8, T_WATER, T_PSYCHIC, 255 },  // 121 agua 1.1m
  { "MR. MIME", 866, 0, R_RARO, 0xD28F, 50, 45, 65, 100, 120, 90, 0, 111, 5, T_PSYCHIC, T_FAIRY, 50 },  // 122 psiquico 1.3m
  { "SCYTHER", 212, 45, R_RARO, 0x7CC4, 70, 110, 80, 55, 80, 105, 2, 110, 6, T_BUG, T_FLYING, 50 },  // 123 bicho 1.5m
  { "JYNX", 0, 0, R_RARO, 0xD28F, 65, 50, 35, 115, 95, 105, 0, 122, 4, T_PSYCHIC, T_ICE, 100 },  // 124 psiquico 1.4m
  { "ELECTABUZZ", 466, 50, R_RARO, 0xBCA1, 65, 83, 57, 95, 85, 105, 0, 120, 3, T_ELEC, T_NONE, 25 },  // 125 electrico 1.1m
  { "MAGMAR", 467, 50, R_RARO, 0xEA87, 65, 95, 57, 100, 85, 93, 3, 121, 4, T_FIRE, T_NONE, 25 },  // 126 fuego 1.3m
  { "PINSIR", 0, 0, R_RARO, 0x7CC4, 65, 125, 100, 55, 70, 85, 2, 120, 6, T_BUG, T_NONE, 50 },  // 127 bicho 1.5m
  { "TAUROS", 0, 0, R_RARO, 0x8C4D, 75, 110, 95, 40, 70, 110, 0, 120, 9, T_NORMAL, T_NONE, 0 },  // 128 normal 1.4m
  { "MAGIKARP", 130, 20, R_COMUN, 0x4C98, 20, 10, 55, 15, 20, 80, 1, 107, 1, T_WATER, T_NONE, 50 },  // 129 agua 0.9m
  { "GYARADOS", 0, 0, R_EVO, 0x4C98, 95, 125, 79, 60, 100, 81, 1, 142, 24, T_WATER, T_FLYING, 50 },  // 130 agua 6.5m
  { "LAPRAS", 0, 0, R_RARO, 0x4C98, 130, 75, 80, 95, 95, 60, 1, 123, 22, T_WATER, T_ICE, 50 },  // 131 agua 2.5m
  { "DITTO", 0, 0, R_RARO, 0x8C4D, 48, 48, 48, 48, 48, 48, 0, 90, 1, T_NORMAL, T_NONE, 255 },  // 132 normal 0.3m
  { "EEVEE", 134, 0, R_COMUN, 0x8C4D, 55, 65, 50, 45, 65, 55, 0, 101, 1, T_NORMAL, T_NONE, 50 },  // 133 normal 0.3m
  { "VAPOREON", 0, 0, R_EVO, 0x4C98, 130, 65, 60, 110, 95, 65, 1, 110, 3, T_WATER, T_NONE, 50 },  // 134 agua 1.0m
  { "JOLTEON", 0, 0, R_EVO, 0xBCA1, 65, 65, 60, 110, 95, 130, 0, 106, 2, T_ELEC, T_NONE, 50 },  // 135 electrico 0.8m
  { "FLAREON", 0, 0, R_EVO, 0xEA87, 75, 130, 60, 85, 110, 65, 3, 115, 2, T_FIRE, T_NONE, 50 },  // 136 fuego 0.9m
  { "PORYGON", 233, 30, R_RARO, 0x8C4D, 65, 60, 70, 85, 75, 40, 0, 95, 4, T_NORMAL, T_NONE, 255 },  // 137 normal 0.8m
  { "OMANYTE", 139, 35, R_RARO, 0x9407, 35, 40, 100, 90, 55, 35, 1, 94, 1, T_ROCK, T_WATER, 50 },  // 138 roca 0.4m
  { "OMASTAR", 0, 0, R_EVO, 0x9407, 70, 60, 125, 115, 70, 55, 1, 111, 4, T_ROCK, T_WATER, 12 },  // 139 roca 1.0m
  { "KABUTO", 141, 35, R_RARO, 0x9407, 30, 80, 90, 55, 45, 55, 1, 89, 1, T_ROCK, T_WATER, 12 },  // 140 roca 0.5m
  { "KABUTOPS", 0, 0, R_EVO, 0x9407, 60, 115, 105, 65, 70, 80, 1, 117, 4, T_ROCK, T_WATER, 12 },  // 141 roca 1.3m
  { "AERODACTYL", 0, 0, R_RARO, 0x9407, 80, 105, 65, 60, 75, 130, 4, 126, 6, T_ROCK, T_FLYING, 12 },  // 142 roca 1.8m
  { "SNORLAX", 0, 0, R_RARO, 0x8C4D, 160, 110, 65, 65, 110, 30, 0, 133, 46, T_NORMAL, T_NONE, 12 },  // 143 normal 2.1m
  { "ARTICUNO", 0, 0, R_LEGENDARIO, 0x4DB8, 90, 85, 100, 95, 125, 85, 5, 139, 6, T_ICE, T_FLYING, 255 },  // 144 hielo 1.7m
  { "ZAPDOS", 0, 0, R_LEGENDARIO, 0xBCA1, 90, 90, 85, 125, 90, 100, 0, 133, 5, T_ELEC, T_FLYING, 255 },  // 145 electrico 1.6m
  { "MOLTRES", 0, 0, R_LEGENDARIO, 0xEA87, 90, 100, 90, 125, 85, 90, 3, 131, 6, T_FIRE, T_FLYING, 255 },  // 146 fuego 2.0m
  { "DRATINI", 148, 30, R_RARO, 0x5A98, 41, 64, 45, 50, 50, 50, 1, 94, 1, T_DRAGON, T_NONE, 50 },  // 147 dragon 1.8m
  { "DRAGONAIR", 149, 55, R_EVO, 0x5A98, 61, 84, 65, 70, 70, 70, 1, 113, 2, T_DRAGON, T_NONE, 50 },  // 148 dragon 4.0m
  { "DRAGONITE", 0, 0, R_EVO, 0x5A98, 91, 134, 95, 100, 100, 80, 1, 133, 21, T_DRAGON, T_FLYING, 50 },  // 149 dragon 2.2m
  { "MEWTWO", 0, 0, R_LEGENDARIO, 0xD28F, 110, 110, 100, 154, 130, 130, 0, 120, 12, T_PSYCHIC, T_NONE, 255 },  // 150 psiquico 2.0m
  { "MEW", 0, 0, R_LEGENDARIO, 0xD28F, 100, 100, 100, 100, 100, 100, 0, 97, 1, T_PSYCHIC, T_NONE, 255 },  // 151 psiquico 0.4m
  { "CHIKORITA", 153, 16, R_COMUN, 0x3C49, 45, 49, 65, 49, 65, 45, 2, 97, 1, T_GRASS, T_FAIRY, 50 },  // 152 planta 0.9m
  { "BAYLEEF", 154, 32, R_EVO, 0x3C49, 60, 62, 80, 63, 80, 60, 2, 113, 2, T_GRASS, T_FAIRY, 50 },  // 153 planta 1.2m
  { "MEGANIUM", 0, 0, R_EVO, 0x3C49, 80, 82, 100, 90, 100, 80, 2, 128, 10, T_GRASS, T_FAIRY, 50 },  // 154 planta 1.8m
  { "CYNDAQUIL", 156, 14, R_COMUN, 0xEA87, 39, 52, 43, 60, 50, 65, 3, 94, 1, T_FIRE, T_NORMAL, 50 },  // 155 fuego 0.5m
  { "QUILAVA", 157, 36, R_EVO, 0xEA87, 58, 64, 58, 80, 65, 80, 3, 108, 2, T_FIRE, T_NORMAL, 50 },  // 156 fuego 0.9m
  { "TYPHLOSION", 0, 0, R_EVO, 0xEA87, 78, 74, 78, 109, 85, 110, 3, 124, 8, T_FIRE, T_NORMAL, 50 },  // 157 fuego 1.7m
  { "TOTODILE", 159, 18, R_COMUN, 0x4C98, 50, 65, 64, 44, 48, 43, 1, 91, 1, T_WATER, T_NONE, 50 },  // 158 agua 0.6m
  { "CROCONAW", 160, 30, R_EVO, 0x4C98, 65, 80, 80, 59, 63, 58, 1, 108, 2, T_WATER, T_NONE, 50 },  // 159 agua 1.1m
  { "FERALIGATR", 0, 0, R_EVO, 0x4C98, 85, 105, 100, 79, 83, 78, 1, 131, 9, T_WATER, T_DRAGON, 50 },  // 160 agua 2.3m
  { "SENTRET", 162, 25, R_COMUN, 0x8C4D, 35, 66, 34, 35, 45, 70, 0, 101, 1, T_NORMAL, T_NONE, 50 },  // 161 normal 0.8m
  { "FURRET", 0, 0, R_EVO, 0x8C4D, 85, 90, 64, 45, 65, 110, 0, 110, 3, T_NORMAL, T_NONE, 50 },  // 162 normal 1.8m
  { "HOOTHOOT", 164, 24, R_COMUN, 0x8C4D, 60, 30, 30, 66, 56, 50, 0, 97, 2, T_NORMAL, T_FLYING, 50 },  // 163 normal 0.7m
  { "NOCTOWL", 0, 0, R_EVO, 0x8C4D, 100, 50, 65, 95, 96, 70, 0, 112, 4, T_NORMAL, T_FLYING, 50 },  // 164 normal 1.6m
  { "LEDYBA", 166, 21, R_COMUN, 0x7CC4, 40, 40, 30, 60, 80, 55, 2, 103, 1, T_BUG, T_FLYING, 50 },  // 165 bicho 1.0m
  { "LEDIAN", 0, 0, R_EVO, 0x7CC4, 70, 65, 50, 85, 110, 85, 2, 111, 4, T_BUG, T_FLYING, 50 },  // 166 bicho 1.4m
  { "SPINARAK", 168, 21, R_COMUN, 0x7CC4, 40, 60, 40, 40, 50, 70, 2, 89, 1, T_BUG, T_POISON, 50 },  // 167 bicho 0.5m
  { "ARIADOS", 0, 0, R_EVO, 0x7CC4, 85, 110, 70, 60, 60, 80, 2, 107, 3, T_BUG, T_POISON, 50 },  // 168 bicho 1.1m
  { "CROBAT", 0, 0, R_EVO, 0x8A73, 85, 90, 80, 70, 80, 130, 0, 125, 8, T_POISON, T_FLYING, 50 },  // 169 veneno 1.8m
  { "CHINCHOU", 171, 27, R_COMUN, 0x4C98, 75, 38, 38, 56, 56, 67, 1, 101, 1, T_WATER, T_ELEC, 50 },  // 170 agua 0.5m
  { "LANTURN", 0, 0, R_EVO, 0x4C98, 125, 58, 58, 76, 76, 67, 1, 117, 2, T_WATER, T_ELEC, 50 },  // 171 agua 1.2m
  { "PICHU", 25, 15, R_COMUN, 0xBCA1, 20, 40, 15, 35, 35, 60, 0, 85, 1, T_ELEC, T_NONE, 50 },  // 172 electrico 0.3m
  { "CLEFFA", 35, 15, R_COMUN, 0xDC76, 50, 25, 28, 45, 55, 15, 0, 81, 1, T_FAIRY, T_NONE, 75 },  // 173 hada 0.3m
  { "IGGLYBUFF", 39, 15, R_COMUN, 0x8C4D, 90, 30, 15, 40, 20, 15, 0, 81, 1, T_NORMAL, T_FAIRY, 75 },  // 174 normal 0.3m
  { "TOGEPI", 176, 25, R_COMUN, 0xDC76, 35, 20, 65, 40, 65, 20, 0, 81, 1, T_FAIRY, T_NONE, 12 },  // 175 hada 0.3m
  { "TOGETIC", 468, 0, R_EVO, 0xDC76, 55, 40, 85, 80, 105, 40, 0, 97, 1, T_FAIRY, T_FLYING, 12 },  // 176 hada 0.6m
  { "NATU", 178, 28, R_COMUN, 0xD28F, 40, 50, 45, 70, 45, 70, 0, 84, 1, T_PSYCHIC, T_FLYING, 50 },  // 177 psiquico 0.2m
  { "XATU", 0, 0, R_EVO, 0xD28F, 65, 75, 70, 95, 70, 100, 0, 101, 2, T_PSYCHIC, T_FLYING, 50 },  // 178 psiquico 1.5m
  { "MAREEP", 180, 15, R_COMUN, 0xBCA1, 55, 40, 40, 65, 45, 35, 0, 100, 1, T_ELEC, T_NONE, 50 },  // 179 electrico 0.6m
  { "FLAAFFY", 181, 30, R_EVO, 0xBCA1, 70, 55, 55, 80, 60, 45, 0, 108, 1, T_ELEC, T_NONE, 50 },  // 180 electrico 0.8m
  { "AMPHAROS", 0, 0, R_EVO, 0xBCA1, 90, 75, 85, 115, 90, 55, 0, 115, 6, T_ELEC, T_NONE, 50 },  // 181 electrico 1.4m
  { "BELLOSSOM", 0, 0, R_EVO, 0x3C49, 75, 80, 85, 100, 100, 50, 2, 124, 1, T_GRASS, T_NONE, 50 },  // 182 planta 0.4m
  { "MARILL", 184, 18, R_COMUN, 0x4C98, 70, 20, 50, 20, 50, 40, 1, 96, 1, T_WATER, T_FAIRY, 50 },  // 183 agua 0.4m
  { "AZUMARILL", 0, 0, R_EVO, 0x4C98, 100, 50, 80, 50, 80, 50, 1, 112, 3, T_WATER, T_FAIRY, 50 },  // 184 agua 0.8m
  { "SUDOWOODO", 0, 0, R_RARO, 0x9407, 80, 100, 120, 50, 75, 50, 2, 103, 4, T_ROCK, T_NONE, 50 },  // 185 roca 1.2m
  { "POLITOED", 0, 0, R_EVO, 0x4C98, 90, 75, 75, 90, 100, 70, 1, 141, 3, T_WATER, T_NONE, 50 },  // 186 agua 1.1m
  { "HOPPIP", 188, 18, R_COMUN, 0x3C49, 35, 45, 40, 35, 55, 50, 2, 94, 1, T_GRASS, T_FLYING, 50 },  // 187 planta 0.4m
  { "SKIPLOOM", 189, 27, R_EVO, 0x3C49, 55, 65, 50, 45, 65, 80, 2, 91, 1, T_GRASS, T_FLYING, 50 },  // 188 planta 0.6m
  { "JUMPLUFF", 0, 0, R_EVO, 0x3C49, 75, 90, 70, 55, 95, 110, 2, 110, 1, T_GRASS, T_FLYING, 50 },  // 189 planta 0.8m
  { "AIPOM", 424, 32, R_RARO, 0x8C4D, 55, 70, 55, 40, 55, 85, 0, 97, 1, T_NORMAL, T_NONE, 50 },  // 190 normal 0.8m
  { "SUNKERN", 192, 0, R_COMUN, 0x3C49, 30, 50, 30, 50, 30, 30, 0, 81, 1, T_GRASS, T_NONE, 50 },  // 191 planta 0.3m
  { "SUNFLORA", 0, 0, R_EVO, 0x3C49, 75, 75, 65, 115, 95, 30, 0, 103, 1, T_GRASS, T_FIRE, 50 },  // 192 planta 0.8m
  { "YANMA", 469, 38, R_RARO, 0x7CC4, 65, 65, 45, 75, 45, 95, 2, 99, 4, T_BUG, T_FLYING, 50 },  // 193 bicho 1.2m
  { "WOOPER", 195, 20, R_COMUN, 0x4C98, 55, 55, 45, 35, 35, 15, 1, 90, 1, T_WATER, T_GROUND, 50 },  // 194 agua 0.4m
  { "QUAGSIRE", 0, 0, R_EVO, 0x4C98, 95, 100, 95, 65, 65, 35, 1, 116, 8, T_WATER, T_GROUND, 50 },  // 195 agua 1.4m
  { "ESPEON", 0, 0, R_EVO, 0xD28F, 65, 65, 60, 130, 95, 110, 0, 110, 3, T_PSYCHIC, T_NONE, 50 },  // 196 psiquico 0.9m
  { "UMBREON", 0, 0, R_EVO, 0x49E6, 95, 65, 110, 60, 130, 65, 0, 105, 3, T_DARK, T_NONE, 50 },  // 197 siniestro 1.0m
  { "MURKROW", 430, 0, R_RARO, 0x49E6, 60, 85, 42, 85, 42, 91, 0, 97, 1, T_DARK, T_FLYING, 50 },  // 198 siniestro 0.5m
  { "SLOWKING", 0, 0, R_EVO, 0x4C98, 95, 75, 80, 100, 110, 30, 1, 118, 8, T_WATER, T_PSYCHIC, 50 },  // 199 agua 2.0m
  { "MISDREAVUS", 429, 0, R_RARO, 0x6AD3, 60, 60, 60, 85, 85, 85, 0, 100, 1, T_GHOST, T_FAIRY, 50 },  // 200 fantasma 0.7m
  { "UNOWN", 0, 0, R_RARO, 0xD28F, 80, 72, 48, 80, 80, 48, 0, 76, 1, T_PSYCHIC, T_NONE, 255 },  // 201 psiquico 0.5m
  { "WOBBUFFET", 0, 0, R_RARO, 0xD28F, 190, 33, 58, 33, 58, 33, 0, 110, 3, T_PSYCHIC, T_NONE, 50 },  // 202 psiquico 1.3m
  { "GIRAFARIG", 981, 40, R_RARO, 0x8C4D, 70, 80, 65, 90, 65, 85, 0, 116, 4, T_NORMAL, T_PSYCHIC, 50 },  // 203 normal 1.5m
  { "PINECO", 205, 31, R_COMUN, 0x7CC4, 50, 65, 90, 35, 35, 15, 2, 98, 1, T_BUG, T_NONE, 50 },  // 204 bicho 0.6m
  { "FORRETRESS", 0, 0, R_EVO, 0x7CC4, 75, 90, 140, 60, 60, 40, 2, 116, 13, T_BUG, T_STEEL, 50 },  // 205 bicho 1.2m
  { "DUNSPARCE", 982, 37, R_RARO, 0x8C4D, 100, 70, 70, 65, 65, 45, 0, 97, 1, T_NORMAL, T_NONE, 50 },  // 206 normal 1.5m
  { "GLIGAR", 472, 42, R_RARO, 0xB447, 65, 75, 105, 35, 65, 85, 4, 117, 6, T_GROUND, T_FLYING, 50 },  // 207 tierra 1.1m
  { "STEELIX", 0, 0, R_EVO, 0x7C53, 75, 100, 200, 55, 65, 30, 4, 141, 40, T_STEEL, T_GROUND, 50 },  // 208 acero 9.2m
  { "SNUBBULL", 210, 23, R_COMUN, 0xDC76, 60, 80, 50, 40, 40, 30, 0, 98, 1, T_FAIRY, T_NONE, 75 },  // 209 hada 0.6m
  { "GRANBULL", 0, 0, R_EVO, 0xDC76, 90, 120, 75, 60, 60, 45, 0, 114, 5, T_FAIRY, T_FIGHT, 75 },  // 210 hada 1.4m
  { "QWILFISH", 0, 0, R_RARO, 0x4C98, 65, 95, 85, 55, 55, 85, 1, 95, 1, T_WATER, T_POISON, 50 },  // 211 agua 0.5m
  { "SCIZOR", 0, 0, R_EVO, 0x7CC4, 70, 130, 100, 55, 80, 65, 2, 126, 12, T_BUG, T_STEEL, 50 },  // 212 bicho 1.8m
  { "SHUCKLE", 0, 0, R_RARO, 0x7CC4, 20, 10, 230, 10, 230, 5, 4, 97, 2, T_BUG, T_ROCK, 50 },  // 213 bicho 0.6m
  { "HERACROSS", 0, 0, R_RARO, 0x7CC4, 80, 125, 75, 40, 95, 85, 2, 113, 5, T_BUG, T_FIGHT, 50 },  // 214 bicho 1.5m
  { "SNEASEL", 461, 38, R_RARO, 0x49E6, 55, 95, 55, 35, 75, 115, 0, 106, 3, T_DARK, T_ICE, 50 },  // 215 siniestro 0.9m
  { "TEDDIURSA", 217, 30, R_COMUN, 0x8C4D, 60, 80, 50, 50, 50, 40, 0, 88, 1, T_NORMAL, T_NONE, 50 },  // 216 normal 0.6m
  { "URSARING", 901, 45, R_EVO, 0x8C4D, 90, 110, 65, 75, 65, 55, 0, 128, 13, T_NORMAL, T_NONE, 50 },  // 217 normal 1.8m
  { "SLUGMA", 219, 33, R_COMUN, 0xEA87, 60, 40, 60, 70, 40, 20, 3, 93, 4, T_FIRE, T_NONE, 50 },  // 218 fuego 0.7m
  { "MAGCARGO", 0, 0, R_EVO, 0xEA87, 70, 50, 140, 100, 80, 30, 3, 117, 6, T_FIRE, T_ROCK, 50 },  // 219 fuego 0.8m
  { "SWINUB", 221, 33, R_COMUN, 0x4DB8, 50, 50, 40, 30, 30, 50, 5, 83, 1, T_ICE, T_GROUND, 50 },  // 220 hielo 0.4m
  { "PILOSWINE", 473, 40, R_EVO, 0x4DB8, 100, 100, 80, 60, 60, 50, 5, 117, 6, T_ICE, T_GROUND, 50 },  // 221 hielo 1.1m
  { "CORSOLA", 0, 0, R_RARO, 0x4C98, 85, 85, 85, 95, 85, 35, 1, 99, 1, T_WATER, T_ROCK, 75 },  // 222 agua 0.6m
  { "REMORAID", 224, 25, R_COMUN, 0x4C98, 35, 65, 35, 65, 35, 65, 1, 89, 1, T_WATER, T_NONE, 50 },  // 223 agua 0.6m
  { "OCTILLERY", 0, 0, R_EVO, 0x4C98, 75, 105, 75, 105, 75, 45, 1, 105, 3, T_WATER, T_NONE, 50 },  // 224 agua 0.9m
  { "DELIBIRD", 0, 0, R_RARO, 0x4DB8, 65, 75, 65, 100, 65, 90, 5, 108, 2, T_ICE, T_FLYING, 50 },  // 225 hielo 0.9m
  { "MANTINE", 0, 0, R_RARO, 0x4C98, 85, 40, 70, 80, 140, 70, 1, 117, 22, T_WATER, T_FLYING, 50 },  // 226 agua 2.1m
  { "SKARMORY", 0, 0, R_RARO, 0x7C53, 65, 80, 140, 40, 70, 70, 4, 123, 5, T_STEEL, T_FLYING, 50 },  // 227 acero 1.7m
  { "HOUNDOUR", 229, 24, R_COMUN, 0x49E6, 45, 60, 30, 80, 50, 65, 0, 97, 1, T_DARK, T_FIRE, 50 },  // 228 siniestro 0.6m
  { "HOUNDOOM", 0, 0, R_EVO, 0x49E6, 75, 90, 50, 110, 80, 95, 0, 119, 4, T_DARK, T_FIRE, 50 },  // 229 siniestro 1.4m
  { "KINGDRA", 0, 0, R_EVO, 0x4C98, 75, 95, 95, 95, 95, 85, 1, 124, 15, T_WATER, T_DRAGON, 50 },  // 230 agua 1.8m
  { "PHANPY", 232, 25, R_COMUN, 0xB447, 90, 60, 60, 40, 40, 40, 4, 91, 3, T_GROUND, T_NONE, 50 },  // 231 tierra 0.5m
  { "DONPHAN", 0, 0, R_EVO, 0xB447, 90, 120, 120, 60, 60, 50, 4, 125, 12, T_GROUND, T_NONE, 50 },  // 232 tierra 1.1m
  { "PORYGON2", 474, 50, R_EVO, 0x8C4D, 85, 80, 90, 105, 95, 60, 0, 96, 3, T_NORMAL, T_NONE, 255 },  // 233 normal 0.6m
  { "STANTLER", 899, 40, R_RARO, 0x8C4D, 73, 95, 62, 85, 65, 85, 0, 115, 7, T_NORMAL, T_PSYCHIC, 50 },  // 234 normal 1.4m
  { "SMEARGLE", 0, 0, R_RARO, 0x8C4D, 55, 20, 35, 20, 45, 75, 0, 106, 6, T_NORMAL, T_NONE, 50 },  // 235 normal 1.2m
  { "TYROGUE", 237, 23, R_COMUN, 0xA2A5, 55, 60, 60, 35, 45, 65, 0, 90, 2, T_FIGHT, T_NONE, 0 },  // 236 lucha 0.7m
  { "HITMONTOP", 0, 0, R_EVO, 0xA2A5, 50, 95, 95, 35, 110, 70, 0, 111, 5, T_FIGHT, T_NONE, 0 },  // 237 lucha 1.4m
  { "SMOOCHUM", 124, 30, R_COMUN, 0xD28F, 45, 30, 15, 85, 65, 65, 0, 85, 1, T_PSYCHIC, T_ICE, 100 },  // 238 psiquico 0.4m
  { "ELEKID", 125, 25, R_COMUN, 0xBCA1, 45, 63, 37, 65, 55, 95, 0, 99, 2, T_ELEC, T_NONE, 25 },  // 239 electrico 0.6m
  { "MAGBY", 126, 25, R_COMUN, 0xEA87, 45, 75, 37, 70, 55, 83, 3, 91, 2, T_FIRE, T_NONE, 25 },  // 240 fuego 0.7m
  { "MILTANK", 0, 0, R_RARO, 0x8C4D, 95, 90, 105, 40, 70, 100, 0, 109, 8, T_NORMAL, T_NONE, 100 },  // 241 normal 1.2m
  { "BLISSEY", 0, 0, R_EVO, 0x8C4D, 255, 10, 10, 75, 135, 55, 0, 118, 5, T_NORMAL, T_NONE, 100 },  // 242 normal 1.5m
  { "RAIKOU", 0, 0, R_LEGENDARIO, 0xBCA1, 90, 85, 75, 115, 100, 115, 0, 133, 18, T_ELEC, T_NONE, 255 },  // 243 electrico 1.9m
  { "ENTEI", 0, 0, R_LEGENDARIO, 0xEA87, 115, 115, 85, 90, 75, 100, 3, 133, 20, T_FIRE, T_NONE, 255 },  // 244 fuego 2.1m
  { "SUICUNE", 0, 0, R_LEGENDARIO, 0x4C98, 100, 75, 115, 90, 115, 85, 1, 134, 19, T_WATER, T_NONE, 255 },  // 245 agua 2.0m
  { "LARVITAR", 247, 30, R_RARO, 0x9407, 50, 64, 50, 45, 50, 41, 4, 92, 7, T_ROCK, T_GROUND, 50 },  // 246 roca 0.6m
  { "PUPITAR", 248, 55, R_EVO, 0x9407, 70, 84, 70, 65, 70, 51, 4, 98, 15, T_ROCK, T_GROUND, 50 },  // 247 roca 1.2m
  { "TYRANITAR", 0, 0, R_EVO, 0x9407, 100, 134, 110, 95, 100, 61, 4, 132, 20, T_ROCK, T_DARK, 50 },  // 248 roca 2.0m
  { "LUGIA", 0, 0, R_LEGENDARIO, 0xD28F, 106, 90, 130, 90, 154, 110, 0, 140, 22, T_PSYCHIC, T_FLYING, 255 },  // 249 psiquico 5.2m
  { "HO-OH", 0, 0, R_LEGENDARIO, 0xEA87, 106, 130, 90, 110, 154, 90, 3, 147, 20, T_FIRE, T_FLYING, 255 },  // 250 fuego 3.8m
  { "CELEBI", 0, 0, R_LEGENDARIO, 0xD28F, 100, 100, 100, 100, 100, 100, 0, 88, 1, T_PSYCHIC, T_GRASS, 255 },  // 251 psiquico 0.6m
  { "TREECKO", 253, 16, R_COMUN, 0x3C49, 40, 45, 35, 65, 55, 70, 2, 97, 1, T_GRASS, T_NONE, 50 },  // 252 planta 0.5m
  { "GROVYLE", 254, 36, R_EVO, 0x3C49, 50, 65, 45, 85, 65, 95, 2, 117, 2, T_GRASS, T_NONE, 50 },  // 253 planta 0.9m
  { "SCEPTILE", 0, 0, R_EVO, 0x3C49, 70, 105, 65, 85, 85, 120, 2, 124, 5, T_GRASS, T_DRAGON, 50 },  // 254 planta 1.7m
  { "TORCHIC", 256, 16, R_COMUN, 0xEA87, 45, 60, 40, 70, 50, 45, 3, 86, 1, T_FIRE, T_NONE, 50 },  // 255 fuego 0.4m
  { "COMBUSKEN", 257, 36, R_EVO, 0xEA87, 60, 85, 60, 85, 60, 55, 3, 110, 2, T_FIRE, T_FIGHT, 50 },  // 256 fuego 0.9m
  { "BLAZIKEN", 0, 0, R_EVO, 0xEA87, 80, 120, 70, 110, 70, 80, 3, 125, 5, T_FIRE, T_FIGHT, 50 },  // 257 fuego 1.9m
  { "MUDKIP", 259, 16, R_COMUN, 0x4C98, 50, 70, 50, 50, 50, 40, 1, 94, 1, T_WATER, T_NONE, 50 },  // 258 agua 0.4m
  { "MARSHTOMP", 260, 36, R_EVO, 0x4C98, 70, 85, 70, 60, 70, 50, 1, 109, 3, T_WATER, T_GROUND, 50 },  // 259 agua 0.7m
  { "SWAMPERT", 0, 0, R_EVO, 0x4C98, 100, 110, 90, 85, 90, 60, 1, 132, 8, T_WATER, T_GROUND, 50 },  // 260 agua 1.5m
  { "POOCHYENA", 262, 18, R_COMUN, 0x49E6, 35, 55, 35, 30, 30, 35, 0, 99, 1, T_DARK, T_NONE, 50 },  // 261 siniestro 0.5m
  { "MIGHTYENA", 0, 0, R_EVO, 0x49E6, 70, 110, 70, 60, 60, 90, 0, 122, 4, T_DARK, T_NONE, 50 },  // 262 siniestro 1.0m
  { "ZIGZAGOON", 264, 20, R_COMUN, 0x8C4D, 38, 30, 41, 30, 41, 60, 0, 108, 2, T_NORMAL, T_NONE, 50 },  // 263 normal 0.4m
  { "LINOONE", 0, 0, R_EVO, 0x8C4D, 80, 70, 61, 50, 61, 100, 0, 111, 3, T_NORMAL, T_NONE, 50 },  // 264 normal 0.5m
  { "WURMPLE", 266, 7, R_COMUN, 0x7CC4, 45, 45, 35, 20, 30, 20, 2, 93, 1, T_BUG, T_NONE, 50 },  // 265 bicho 0.3m
  { "SILCOON", 267, 10, R_EVO, 0x7CC4, 50, 35, 55, 25, 25, 15, 2, 98, 1, T_BUG, T_NONE, 50 },  // 266 bicho 0.6m
  { "BEAUTIFLY", 0, 0, R_EVO, 0x7CC4, 70, 70, 60, 110, 75, 85, 2, 109, 3, T_BUG, T_FLYING, 50 },  // 267 bicho 1.0m
  { "CASCOON", 269, 10, R_COMUN, 0x7CC4, 50, 35, 55, 25, 25, 15, 2, 98, 1, T_BUG, T_NONE, 50 },  // 268 bicho 0.7m
  { "DUSTOX", 0, 0, R_EVO, 0x7CC4, 70, 50, 70, 90, 105, 85, 2, 104, 3, T_BUG, T_POISON, 50 },  // 269 bicho 1.2m
  { "LOTAD", 271, 14, R_COMUN, 0x4C98, 40, 30, 30, 40, 50, 30, 1, 92, 1, T_WATER, T_GRASS, 50 },  // 270 agua 0.5m
  { "LOMBRE", 272, 0, R_EVO, 0x4C98, 60, 50, 50, 60, 70, 50, 1, 104, 3, T_WATER, T_GRASS, 50 },  // 271 agua 1.2m
  { "LUDICOLO", 0, 0, R_EVO, 0x4C98, 80, 80, 70, 90, 100, 70, 1, 131, 6, T_WATER, T_GRASS, 50 },  // 272 agua 1.5m
  { "SEEDOT", 274, 14, R_COMUN, 0x3C49, 50, 55, 50, 50, 50, 55, 2, 87, 1, T_GRASS, T_NONE, 50 },  // 273 planta 0.5m
  { "NUZLEAF", 275, 0, R_EVO, 0x3C49, 70, 70, 50, 60, 50, 90, 2, 106, 3, T_GRASS, T_DARK, 50 },  // 274 planta 1.0m
  { "SHIFTRY", 0, 0, R_EVO, 0x3C49, 90, 110, 60, 95, 60, 90, 2, 133, 6, T_GRASS, T_DARK, 50 },  // 275 planta 1.3m
  { "TAILLOW", 277, 22, R_COMUN, 0x8C4D, 40, 65, 30, 30, 30, 85, 0, 95, 1, T_NORMAL, T_FLYING, 50 },  // 276 normal 0.3m
  { "SWELLOW", 0, 0, R_EVO, 0x8C4D, 75, 105, 60, 50, 50, 125, 0, 119, 2, T_NORMAL, T_FLYING, 50 },  // 277 normal 0.7m
  { "WINGULL", 279, 25, R_COMUN, 0x4C98, 40, 30, 30, 55, 30, 85, 1, 90, 1, T_WATER, T_FLYING, 50 },  // 278 agua 0.6m
  { "PELIPPER", 0, 0, R_EVO, 0x4C98, 80, 50, 100, 95, 70, 65, 1, 124, 3, T_WATER, T_FLYING, 50 },  // 279 agua 1.2m
  { "RALTS", 281, 20, R_COMUN, 0xD28F, 28, 25, 25, 45, 35, 40, 0, 87, 1, T_PSYCHIC, T_FAIRY, 50 },  // 280 psiquico 0.4m
  { "KIRLIA", 282, 30, R_EVO, 0xD28F, 55, 35, 45, 75, 75, 50, 0, 99, 2, T_PSYCHIC, T_FAIRY, 50 },  // 281 psiquico 0.8m
  { "GARDEVOIR", 0, 0, R_EVO, 0xD28F, 68, 65, 65, 125, 115, 80, 0, 120, 5, T_PSYCHIC, T_FAIRY, 50 },  // 282 psiquico 1.6m
  { "SURSKIT", 284, 25, R_COMUN, 0x7CC4, 55, 30, 32, 60, 60, 65, 2, 83, 1, T_BUG, T_WATER, 50 },  // 283 bicho 0.5m
  { "MASQUERAIN", 0, 0, R_EVO, 0x7CC4, 70, 60, 62, 100, 90, 80, 2, 112, 1, T_BUG, T_WATER, 50 },  // 284 bicho 0.8m
  { "SHROOMISH", 286, 23, R_COMUN, 0x3C49, 60, 40, 60, 40, 60, 35, 2, 90, 1, T_GRASS, T_NONE, 50 },  // 285 planta 0.4m
  { "BRELOOM", 0, 0, R_EVO, 0x3C49, 60, 130, 80, 60, 60, 70, 2, 113, 4, T_GRASS, T_FIGHT, 50 },  // 286 planta 1.2m
  { "SLAKOTH", 288, 18, R_COMUN, 0x8C4D, 60, 60, 60, 35, 35, 30, 0, 98, 2, T_NORMAL, T_NONE, 50 },  // 287 normal 0.8m
  { "VIGOROTH", 289, 36, R_EVO, 0x8C4D, 80, 80, 80, 55, 55, 90, 0, 117, 5, T_NORMAL, T_NONE, 50 },  // 288 normal 1.4m
  { "SLAKING", 0, 0, R_EVO, 0x8C4D, 150, 160, 100, 95, 65, 100, 0, 134, 13, T_NORMAL, T_NONE, 50 },  // 289 normal 2.0m
  { "NINCADA", 291, 20, R_COMUN, 0x7CC4, 31, 45, 90, 30, 30, 40, 2, 91, 1, T_BUG, T_GROUND, 50 },  // 290 bicho 0.5m
  { "NINJASK", 0, 0, R_EVO, 0x7CC4, 61, 90, 45, 50, 50, 160, 2, 105, 1, T_BUG, T_FLYING, 50 },  // 291 bicho 0.8m
  { "SHEDINJA", 0, 0, R_RARO, 0x7CC4, 1, 90, 45, 30, 30, 40, 2, 102, 1, T_BUG, T_GHOST, 255 },  // 292 bicho 0.8m
  { "WHISMUR", 294, 20, R_COMUN, 0x8C4D, 64, 60, 36, 51, 36, 28, 0, 96, 2, T_NORMAL, T_NONE, 50 },  // 293 normal 0.6m
  { "LOUDRED", 295, 40, R_EVO, 0x8C4D, 84, 80, 43, 80, 43, 48, 0, 123, 4, T_NORMAL, T_NONE, 50 },  // 294 normal 1.0m
  { "EXPLOUD", 0, 0, R_EVO, 0x8C4D, 104, 100, 63, 100, 63, 68, 0, 133, 8, T_NORMAL, T_NONE, 50 },  // 295 normal 1.5m
  { "MAKUHITA", 297, 24, R_COMUN, 0xA2A5, 72, 60, 30, 20, 30, 25, 0, 110, 9, T_FIGHT, T_NONE, 25 },  // 296 lucha 1.0m
  { "HARIYAMA", 0, 0, R_EVO, 0xA2A5, 144, 120, 60, 40, 60, 50, 0, 139, 25, T_FIGHT, T_NONE, 25 },  // 297 lucha 2.3m
  { "AZURILL", 183, 15, R_COMUN, 0x8C4D, 50, 20, 40, 20, 40, 20, 0, 101, 1, T_NORMAL, T_FAIRY, 75 },  // 298 normal 0.2m
  { "NOSEPASS", 476, 40, R_RARO, 0x9407, 30, 45, 135, 45, 90, 30, 4, 106, 10, T_ROCK, T_NONE, 50 },  // 299 roca 1.0m
  { "SKITTY", 301, 0, R_COMUN, 0x8C4D, 50, 65, 45, 35, 35, 60, 0, 98, 1, T_NORMAL, T_FAIRY, 75 },  // 300 normal 0.6m
  { "DELCATTY", 0, 0, R_EVO, 0x8C4D, 90, 95, 75, 90, 65, 100, 0, 109, 3, T_NORMAL, T_FAIRY, 75 },  // 301 normal 1.1m
  { "SABLEYE", 0, 0, R_RARO, 0x49E6, 75, 90, 90, 90, 80, 50, 0, 98, 1, T_DARK, T_GHOST, 50 },  // 302 siniestro 0.5m
  { "MAWILE", 0, 0, R_RARO, 0x7C53, 65, 85, 85, 55, 55, 50, 4, 112, 1, T_STEEL, T_FAIRY, 50 },  // 303 acero 0.6m
  { "ARON", 305, 32, R_COMUN, 0x7C53, 50, 70, 100, 40, 40, 30, 4, 87, 6, T_STEEL, T_ROCK, 50 },  // 304 acero 0.4m
  { "LAIRON", 306, 42, R_EVO, 0x7C53, 60, 90, 140, 50, 50, 40, 4, 118, 12, T_STEEL, T_ROCK, 50 },  // 305 acero 0.9m
  { "AGGRON", 0, 0, R_EVO, 0x7C53, 70, 110, 180, 60, 60, 50, 4, 137, 36, T_STEEL, T_ROCK, 50 },  // 306 acero 2.1m
  { "MEDITITE", 308, 37, R_COMUN, 0xA2A5, 30, 40, 55, 40, 55, 60, 0, 96, 1, T_FIGHT, T_PSYCHIC, 50 },  // 307 lucha 0.6m
  { "MEDICHAM", 0, 0, R_EVO, 0xA2A5, 60, 60, 75, 60, 75, 80, 0, 106, 3, T_FIGHT, T_PSYCHIC, 50 },  // 308 lucha 1.3m
  { "ELECTRIKE", 310, 26, R_COMUN, 0xBCA1, 40, 45, 40, 65, 40, 65, 0, 101, 2, T_ELEC, T_NONE, 50 },  // 309 electrico 0.6m
  { "MANECTRIC", 0, 0, R_EVO, 0xBCA1, 70, 75, 60, 105, 60, 105, 0, 119, 4, T_ELEC, T_NONE, 50 },  // 310 electrico 1.5m
  { "PLUSLE", 0, 0, R_COMUN, 0xBCA1, 60, 50, 40, 105, 75, 95, 0, 91, 1, T_ELEC, T_NONE, 50 },  // 311 electrico 0.4m
  { "MINUN", 0, 0, R_COMUN, 0xBCA1, 60, 40, 50, 75, 105, 95, 0, 89, 1, T_ELEC, T_NONE, 50 },  // 312 electrico 0.4m
  { "VOLBEAT", 0, 0, R_COMUN, 0x7CC4, 65, 53, 55, 100, 75, 105, 2, 105, 2, T_BUG, T_ELEC, 0 },  // 313 bicho 0.7m
  { "ILLUMISE", 0, 0, R_COMUN, 0x7CC4, 65, 53, 55, 95, 100, 85, 2, 100, 2, T_BUG, T_FAIRY, 100 },  // 314 bicho 0.6m
  { "ROSELIA", 407, 0, R_COMUN, 0x3C49, 50, 70, 45, 100, 80, 65, 2, 103, 1, T_GRASS, T_POISON, 50 },  // 315 planta 0.3m
  { "GULPIN", 317, 28, R_COMUN, 0x8A73, 70, 43, 53, 53, 53, 40, 0, 89, 1, T_POISON, T_NONE, 50 },  // 316 veneno 0.4m
  { "SWALOT", 0, 0, R_EVO, 0x8A73, 100, 73, 83, 80, 83, 55, 0, 125, 8, T_POISON, T_NONE, 50 },  // 317 veneno 1.7m
  { "CARVANHA", 319, 30, R_COMUN, 0x4C98, 45, 90, 20, 65, 20, 65, 1, 102, 2, T_WATER, T_DARK, 50 },  // 318 agua 0.8m
  { "SHARPEDO", 0, 0, R_EVO, 0x4C98, 70, 120, 50, 90, 50, 100, 1, 114, 9, T_WATER, T_DARK, 50 },  // 319 agua 1.8m
  { "WAILMER", 321, 40, R_COMUN, 0x4C98, 130, 70, 35, 70, 35, 60, 1, 122, 13, T_WATER, T_NONE, 50 },  // 320 agua 2.0m
  { "WAILORD", 0, 0, R_EVO, 0x4C98, 170, 80, 65, 90, 65, 60, 1, 147, 40, T_WATER, T_NONE, 50 },  // 321 agua 14.5m
  { "NUMEL", 323, 33, R_COMUN, 0xEA87, 60, 60, 40, 65, 45, 35, 3, 104, 2, T_FIRE, T_GROUND, 50 },  // 322 fuego 0.7m
  { "CAMERUPT", 0, 0, R_EVO, 0xEA87, 70, 100, 80, 105, 85, 40, 3, 133, 22, T_FIRE, T_GROUND, 50 },  // 323 fuego 1.9m
  { "TORKOAL", 0, 0, R_RARO, 0xEA87, 70, 85, 140, 95, 70, 20, 3, 131, 8, T_FIRE, T_NONE, 50 },  // 324 fuego 0.5m
  { "SPOINK", 326, 32, R_COMUN, 0xD28F, 60, 25, 35, 70, 80, 60, 0, 91, 3, T_PSYCHIC, T_NONE, 50 },  // 325 psiquico 0.7m
  { "GRUMPIG", 0, 0, R_EVO, 0xD28F, 80, 45, 75, 100, 110, 80, 0, 121, 7, T_PSYCHIC, T_NONE, 50 },  // 326 psiquico 0.9m
  { "SPINDA", 0, 0, R_RARO, 0x8C4D, 70, 100, 60, 100, 60, 60, 0, 101, 1, T_NORMAL, T_NONE, 50 },  // 327 normal 1.1m
  { "TRAPINCH", 329, 35, R_RARO, 0xB447, 45, 100, 45, 45, 45, 10, 4, 97, 2, T_GROUND, T_NONE, 50 },  // 328 tierra 0.7m
  { "VIBRAVA", 330, 45, R_EVO, 0xB447, 50, 70, 50, 50, 50, 70, 4, 110, 2, T_GROUND, T_DRAGON, 50 },  // 329 tierra 1.1m
  { "FLYGON", 0, 0, R_EVO, 0xB447, 80, 100, 80, 100, 80, 110, 4, 126, 8, T_GROUND, T_DRAGON, 50 },  // 330 tierra 2.0m
  { "CACNEA", 332, 30, R_COMUN, 0x3C49, 50, 85, 40, 85, 40, 35, 2, 102, 5, T_GRASS, T_NONE, 50 },  // 331 planta 0.4m
  { "CACTURNE", 0, 0, R_EVO, 0x3C49, 85, 115, 60, 115, 60, 55, 2, 121, 8, T_GRASS, T_DARK, 50 },  // 332 planta 1.3m
  { "SWABLU", 334, 35, R_COMUN, 0x8C4D, 45, 60, 60, 60, 75, 50, 0, 107, 1, T_NORMAL, T_FLYING, 50 },  // 333 normal 0.4m
  { "ALTARIA", 0, 0, R_EVO, 0x5A98, 75, 90, 90, 90, 95, 80, 1, 118, 2, T_FLYING, T_DRAGON, 50 },  // 334 dragon 1.1m
  { "ZANGOOSE", 0, 0, R_RARO, 0x8C4D, 83, 115, 70, 60, 70, 100, 0, 116, 4, T_NORMAL, T_NONE, 50 },  // 335 normal 1.3m
  { "SEVIPER", 0, 0, R_RARO, 0x8A73, 83, 110, 70, 90, 60, 65, 0, 128, 5, T_POISON, T_NONE, 50 },  // 336 veneno 2.7m
  { "LUNATONE", 0, 0, R_RARO, 0x9407, 90, 55, 70, 115, 90, 70, 4, 107, 17, T_ROCK, T_PSYCHIC, 255 },  // 337 roca 1.0m
  { "SOLROCK", 0, 0, R_RARO, 0x9407, 90, 115, 90, 55, 70, 70, 4, 115, 15, T_ROCK, T_PSYCHIC, 255 },  // 338 roca 1.2m
  { "BARBOACH", 340, 30, R_COMUN, 0x4C98, 60, 65, 43, 65, 41, 60, 1, 89, 1, T_WATER, T_GROUND, 50 },  // 339 agua 0.4m
  { "WHISCASH", 0, 0, R_EVO, 0x4C98, 110, 95, 73, 76, 81, 60, 1, 122, 2, T_WATER, T_GROUND, 50 },  // 340 agua 0.9m
  { "CORPHISH", 342, 30, R_COMUN, 0x4C98, 43, 80, 65, 50, 35, 35, 1, 107, 1, T_WATER, T_NONE, 50 },  // 341 agua 0.6m
  { "CRAWDAUNT", 0, 0, R_EVO, 0x4C98, 63, 120, 85, 90, 55, 55, 1, 123, 3, T_WATER, T_DARK, 50 },  // 342 agua 1.1m
  { "BALTOY", 344, 33, R_COMUN, 0xB447, 40, 40, 55, 60, 70, 55, 4, 88, 2, T_GROUND, T_PSYCHIC, 255 },  // 343 tierra 0.5m
  { "CLAYDOL", 0, 0, R_EVO, 0xB447, 70, 60, 105, 90, 120, 75, 4, 125, 11, T_GROUND, T_PSYCHIC, 255 },  // 344 tierra 1.5m
  { "LILEEP", 346, 40, R_RARO, 0x9407, 66, 41, 77, 61, 87, 23, 4, 108, 2, T_ROCK, T_GRASS, 12 },  // 345 roca 1.0m
  { "CRADILY", 0, 0, R_EVO, 0x9407, 86, 81, 97, 81, 107, 43, 4, 125, 6, T_ROCK, T_GRASS, 12 },  // 346 roca 1.5m
  { "ANORITH", 348, 40, R_RARO, 0x9407, 45, 95, 50, 40, 50, 75, 4, 101, 1, T_ROCK, T_BUG, 12 },  // 347 roca 0.7m
  { "ARMALDO", 0, 0, R_EVO, 0x9407, 75, 125, 100, 70, 80, 45, 4, 132, 7, T_ROCK, T_BUG, 12 },  // 348 roca 1.5m
  { "FEEBAS", 350, 37, R_RARO, 0x4C98, 50, 45, 35, 60, 55, 70, 1, 97, 1, T_WATER, T_NONE, 50 },  // 349 agua 0.6m
  { "MILOTIC", 0, 0, R_EVO, 0x4C98, 95, 60, 79, 100, 125, 81, 1, 139, 16, T_WATER, T_NONE, 50 },  // 350 agua 6.2m
  { "CASTFORM", 0, 0, R_RARO, 0x8C4D, 90, 70, 90, 100, 90, 70, 0, 82, 1, T_NORMAL, T_NONE, 50 },  // 351 normal 0.3m
  { "KECLEON", 0, 0, R_RARO, 0x8C4D, 60, 100, 70, 60, 120, 40, 0, 106, 2, T_NORMAL, T_NONE, 50 },  // 352 normal 1.0m
  { "SHUPPET", 354, 32, R_COMUN, 0x6AD3, 44, 75, 35, 63, 33, 45, 0, 92, 1, T_GHOST, T_NONE, 50 },  // 353 fantasma 0.6m
  { "BANETTE", 0, 0, R_EVO, 0x6AD3, 64, 115, 65, 83, 63, 65, 0, 110, 1, T_GHOST, T_NONE, 50 },  // 354 fantasma 1.1m
  { "DUSKULL", 356, 30, R_COMUN, 0x6AD3, 20, 55, 90, 30, 90, 25, 0, 96, 2, T_GHOST, T_NONE, 50 },  // 355 fantasma 0.8m
  { "DUSCLOPS", 477, 45, R_EVO, 0x6AD3, 40, 70, 130, 60, 130, 25, 0, 124, 3, T_GHOST, T_NONE, 50 },  // 356 fantasma 1.6m
  { "TROPIUS", 0, 0, R_RARO, 0x3C49, 99, 90, 93, 90, 87, 51, 2, 145, 10, T_GRASS, T_FLYING, 50 },  // 357 planta 2.0m
  { "CHIMECHO", 0, 0, R_RARO, 0xD28F, 75, 50, 80, 95, 90, 65, 0, 91, 1, T_PSYCHIC, T_NONE, 50 },  // 358 psiquico 0.6m
  { "ABSOL", 0, 0, R_RARO, 0x49E6, 65, 130, 60, 75, 60, 75, 0, 115, 5, T_DARK, T_NONE, 50 },  // 359 siniestro 1.2m
  { "WYNAUT", 202, 15, R_RARO, 0xD28F, 95, 23, 48, 23, 48, 23, 0, 94, 1, T_PSYCHIC, T_NONE, 50 },  // 360 psiquico 0.6m
  { "SNORUNT", 362, 42, R_RARO, 0x4DB8, 50, 50, 50, 50, 50, 50, 5, 90, 2, T_ICE, T_NONE, 50 },  // 361 hielo 0.7m
  { "GLALIE", 0, 0, R_EVO, 0x4DB8, 90, 90, 80, 90, 80, 80, 5, 122, 26, T_ICE, T_DARK, 50 },  // 362 hielo 1.5m
  { "SPHEAL", 364, 32, R_COMUN, 0x4DB8, 70, 40, 50, 55, 50, 25, 5, 94, 4, T_ICE, T_WATER, 50 },  // 363 hielo 0.8m
  { "SEALEO", 365, 44, R_EVO, 0x4DB8, 90, 60, 70, 75, 70, 45, 5, 114, 9, T_ICE, T_WATER, 50 },  // 364 hielo 1.1m
  { "WALREIN", 0, 0, R_EVO, 0x4DB8, 110, 90, 90, 95, 90, 65, 5, 130, 15, T_ICE, T_WATER, 50 },  // 365 hielo 1.4m
  { "CLAMPERL", 367, 40, R_RARO, 0x4C98, 35, 64, 85, 74, 55, 32, 1, 103, 5, T_WATER, T_NONE, 50 },  // 366 agua 0.4m
  { "HUNTAIL", 0, 0, R_EVO, 0x4C98, 55, 104, 105, 94, 75, 52, 1, 124, 3, T_WATER, T_NONE, 50 },  // 367 agua 1.7m
  { "GOREBYSS", 0, 0, R_EVO, 0x4C98, 55, 84, 105, 114, 75, 52, 1, 110, 2, T_WATER, T_NONE, 50 },  // 368 agua 1.8m
  { "RELICANTH", 0, 0, R_RARO, 0x4C98, 100, 100, 130, 45, 65, 55, 1, 109, 2, T_WATER, T_ROCK, 12 },  // 369 agua 1.0m
  { "LUVDISC", 0, 0, R_RARO, 0x4C98, 63, 30, 55, 110, 65, 100, 1, 82, 1, T_WATER, T_FAIRY, 75 },  // 370 agua 0.6m
  { "BAGON", 372, 30, R_RARO, 0x5A98, 45, 75, 60, 40, 30, 50, 1, 97, 4, T_DRAGON, T_NONE, 50 },  // 371 dragon 0.6m
  { "SHELGON", 373, 50, R_EVO, 0x5A98, 65, 95, 100, 60, 50, 50, 1, 109, 11, T_DRAGON, T_NONE, 50 },  // 372 dragon 1.1m
  { "SALAMENCE", 0, 0, R_EVO, 0x5A98, 95, 135, 80, 110, 80, 100, 1, 142, 10, T_DRAGON, T_FLYING, 50 },  // 373 dragon 1.5m
  { "BELDUM", 375, 20, R_RARO, 0x7C53, 40, 55, 80, 35, 60, 30, 4, 93, 10, T_STEEL, T_PSYCHIC, 255 },  // 374 acero 0.6m
  { "METANG", 376, 45, R_EVO, 0x7C53, 60, 75, 100, 55, 80, 50, 4, 121, 20, T_STEEL, T_PSYCHIC, 255 },  // 375 acero 1.2m
  { "METAGROSS", 0, 0, R_EVO, 0x7C53, 80, 135, 130, 95, 90, 70, 4, 126, 55, T_STEEL, T_PSYCHIC, 255 },  // 376 acero 1.6m
  { "REGIROCK", 0, 0, R_LEGENDARIO, 0x9407, 80, 100, 200, 50, 100, 50, 4, 137, 23, T_ROCK, T_NONE, 255 },  // 377 roca 1.7m
  { "REGICE", 0, 0, R_LEGENDARIO, 0x4DB8, 80, 50, 100, 100, 200, 50, 5, 132, 18, T_ICE, T_NONE, 255 },  // 378 hielo 1.8m
  { "REGISTEEL", 0, 0, R_LEGENDARIO, 0x7C53, 80, 75, 150, 75, 150, 50, 4, 129, 20, T_STEEL, T_NONE, 255 },  // 379 acero 1.9m
  { "LATIAS", 0, 0, R_LEGENDARIO, 0x5A98, 80, 80, 90, 110, 130, 110, 1, 115, 4, T_DRAGON, T_PSYCHIC, 100 },  // 380 dragon 1.4m
  { "LATIOS", 0, 0, R_LEGENDARIO, 0x5A98, 80, 90, 80, 130, 110, 110, 1, 124, 6, T_DRAGON, T_PSYCHIC, 0 },  // 381 dragon 2.0m
  { "KYOGRE", 0, 0, R_LEGENDARIO, 0x4C98, 100, 100, 90, 150, 140, 90, 1, 138, 35, T_WATER, T_NONE, 255 },  // 382 agua 4.5m
  { "GROUDON", 0, 0, R_LEGENDARIO, 0xB447, 100, 150, 140, 100, 90, 90, 4, 140, 95, T_GROUND, T_NONE, 255 },  // 383 tierra 3.5m
  { "RAYQUAZA", 0, 0, R_LEGENDARIO, 0x5A98, 105, 150, 90, 150, 90, 95, 1, 137, 21, T_DRAGON, T_FLYING, 255 },  // 384 dragon 7.0m
  { "JIRACHI", 0, 0, R_LEGENDARIO, 0x7C53, 100, 100, 100, 100, 100, 100, 4, 101, 1, T_STEEL, T_PSYCHIC, 255 },  // 385 acero 0.3m
  { "DEOXYS", 0, 0, R_LEGENDARIO, 0xD28F, 50, 150, 50, 150, 50, 150, 0, 126, 6, T_PSYCHIC, T_NONE, 255 },  // 386 psiquico 1.7m
  { "TURTWIG", 388, 18, R_COMUN, 0x3C49, 55, 68, 64, 45, 55, 31, 2, 92, 1, T_GRASS, T_NONE, 50 },  // 387 planta 0.4m
  { "GROTLE", 389, 32, R_EVO, 0x3C49, 75, 89, 85, 55, 65, 36, 2, 126, 10, T_GRASS, T_NONE, 50 },  // 388 planta 1.1m
  { "TORTERRA", 0, 0, R_EVO, 0x3C49, 95, 109, 105, 75, 85, 56, 2, 142, 31, T_GRASS, T_GROUND, 50 },  // 389 planta 2.2m
  { "CHIMCHAR", 391, 14, R_COMUN, 0xEA87, 44, 58, 44, 58, 44, 61, 3, 93, 1, T_FIRE, T_NONE, 50 },  // 390 fuego 0.5m
  { "MONFERNO", 392, 36, R_EVO, 0xEA87, 64, 78, 52, 78, 52, 81, 3, 112, 2, T_FIRE, T_FIGHT, 50 },  // 391 fuego 0.9m
  { "INFERNAPE", 0, 0, R_EVO, 0xEA87, 76, 104, 71, 104, 71, 108, 3, 122, 6, T_FIRE, T_FIGHT, 50 },  // 392 fuego 1.2m
  { "PIPLUP", 394, 16, R_COMUN, 0x4C98, 53, 51, 53, 61, 56, 40, 1, 85, 1, T_WATER, T_NONE, 50 },  // 393 agua 0.4m
  { "PRINPLUP", 395, 36, R_EVO, 0x4C98, 64, 66, 68, 81, 76, 50, 1, 109, 2, T_WATER, T_NONE, 50 },  // 394 agua 0.8m
  { "EMPOLEON", 0, 0, R_EVO, 0x4C98, 84, 86, 88, 111, 101, 60, 1, 130, 8, T_WATER, T_STEEL, 50 },  // 395 agua 1.7m
  { "STARLY", 397, 14, R_COMUN, 0x8C4D, 40, 55, 30, 30, 30, 60, 0, 91, 1, T_NORMAL, T_FLYING, 50 },  // 396 normal 0.3m
  { "STARAVIA", 398, 34, R_EVO, 0x8C4D, 55, 75, 50, 40, 40, 80, 0, 102, 2, T_NORMAL, T_FLYING, 50 },  // 397 normal 0.6m
  { "STARAPTOR", 0, 0, R_EVO, 0x8C4D, 85, 120, 70, 50, 60, 100, 0, 117, 2, T_NORMAL, T_FLYING, 50 },  // 398 normal 1.2m
  { "BIDOOF", 400, 15, R_COMUN, 0x8C4D, 59, 45, 40, 35, 40, 31, 0, 101, 2, T_NORMAL, T_NONE, 50 },  // 399 normal 0.5m
  { "BIBAREL", 0, 0, R_EVO, 0x8C4D, 90, 85, 70, 70, 80, 71, 0, 125, 3, T_NORMAL, T_WATER, 50 },  // 400 normal 1.0m
  { "KRICKETOT", 402, 15, R_COMUN, 0x7CC4, 37, 45, 41, 45, 41, 25, 2, 98, 1, T_BUG, T_NONE, 50 },  // 401 bicho 0.3m
  { "KRICKETUNE", 0, 0, R_EVO, 0x7CC4, 77, 105, 61, 55, 86, 65, 2, 113, 3, T_BUG, T_NONE, 50 },  // 402 bicho 1.0m
  { "SHINX", 404, 16, R_COMUN, 0xBCA1, 45, 65, 34, 40, 34, 65, 0, 97, 1, T_ELEC, T_NONE, 50 },  // 403 electrico 0.5m
  { "LUXIO", 405, 30, R_EVO, 0xBCA1, 60, 85, 49, 65, 49, 80, 0, 110, 3, T_ELEC, T_NONE, 50 },  // 404 electrico 0.9m
  { "LUXRAY", 0, 0, R_EVO, 0xBCA1, 80, 120, 75, 75, 75, 100, 0, 122, 4, T_ELEC, T_NONE, 50 },  // 405 electrico 1.4m
  { "BUDEW", 315, 20, R_RARO, 0x3C49, 50, 50, 35, 55, 60, 55, 2, 86, 1, T_GRASS, T_POISON, 50 },  // 406 planta 0.2m
  { "ROSERADE", 0, 0, R_EVO, 0x3C49, 60, 70, 65, 125, 105, 90, 2, 116, 1, T_GRASS, T_POISON, 50 },  // 407 planta 0.9m
  { "CRANIDOS", 409, 30, R_RARO, 0x9407, 67, 125, 40, 30, 30, 58, 4, 104, 3, T_ROCK, T_NONE, 12 },  // 408 roca 0.9m
  { "RAMPARDOS", 0, 0, R_EVO, 0x9407, 97, 165, 60, 65, 50, 68, 4, 126, 10, T_ROCK, T_NONE, 12 },  // 409 roca 1.6m
  { "SHIELDON", 411, 30, R_RARO, 0x9407, 30, 42, 118, 42, 88, 30, 4, 96, 6, T_ROCK, T_STEEL, 12 },  // 410 roca 0.5m
  { "BASTIODON", 0, 0, R_EVO, 0x9407, 70, 52, 168, 47, 138, 30, 4, 129, 15, T_ROCK, T_STEEL, 12 },  // 411 roca 1.3m
  { "BURMY", 413, 20, R_COMUN, 0x7CC4, 40, 29, 45, 29, 45, 36, 2, 85, 1, T_BUG, T_NONE, 50 },  // 412 bicho 0.2m
  { "WORMADAM", 0, 0, R_EVO, 0x7CC4, 80, 59, 95, 95, 105, 36, 2, 95, 1, T_BUG, T_GRASS, 100 },  // 413 bicho 0.5m
  { "MOTHIM", 0, 0, R_EVO, 0x7CC4, 70, 84, 60, 105, 70, 86, 2, 116, 2, T_BUG, T_FLYING, 0 },  // 414 bicho 0.9m
  { "COMBEE", 416, 21, R_COMUN, 0x7CC4, 30, 30, 42, 30, 42, 70, 2, 98, 1, T_BUG, T_FLYING, 12 },  // 415 bicho 0.3m
  { "VESPIQUEN", 0, 0, R_EVO, 0x7CC4, 70, 90, 102, 70, 102, 40, 2, 117, 4, T_BUG, T_FLYING, 100 },  // 416 bicho 1.2m
  { "PACHIRISU", 0, 0, R_COMUN, 0xBCA1, 60, 45, 70, 90, 90, 115, 0, 103, 1, T_ELEC, T_NONE, 50 },  // 417 electrico 0.4m
  { "BUIZEL", 419, 26, R_COMUN, 0x4C98, 55, 65, 35, 60, 30, 85, 1, 97, 3, T_WATER, T_NONE, 50 },  // 418 agua 0.7m
  { "FLOATZEL", 0, 0, R_EVO, 0x4C98, 85, 110, 55, 85, 50, 115, 1, 119, 3, T_WATER, T_NONE, 50 },  // 419 agua 1.1m
  { "CHERUBI", 421, 25, R_COMUN, 0x3C49, 45, 35, 45, 62, 53, 35, 2, 86, 1, T_GRASS, T_NONE, 50 },  // 420 planta 0.4m
  { "CHERRIM", 0, 0, R_EVO, 0x3C49, 70, 60, 70, 100, 88, 100, 2, 99, 1, T_GRASS, T_NONE, 50 },  // 421 planta 0.5m
  { "SHELLOS", 423, 30, R_COMUN, 0x4C98, 76, 48, 48, 57, 62, 34, 1, 89, 1, T_WATER, T_NONE, 50 },  // 422 agua 0.3m
  { "GASTRODON", 0, 0, R_EVO, 0x4C98, 111, 83, 68, 92, 82, 39, 1, 109, 3, T_WATER, T_GROUND, 50 },  // 423 agua 0.9m
  { "AMBIPOM", 0, 0, R_EVO, 0x8C4D, 75, 100, 66, 60, 66, 115, 0, 126, 2, T_NORMAL, T_NONE, 50 },  // 424 normal 1.2m
  { "DRIFLOON", 426, 28, R_RARO, 0x6AD3, 90, 50, 34, 60, 44, 70, 0, 94, 1, T_GHOST, T_FLYING, 50 },  // 425 fantasma 0.4m
  { "DRIFBLIM", 0, 0, R_EVO, 0x6AD3, 150, 80, 44, 90, 54, 80, 0, 117, 2, T_GHOST, T_FLYING, 50 },  // 426 fantasma 1.2m
  { "BUNEARY", 428, 35, R_COMUN, 0x8C4D, 55, 66, 44, 44, 56, 85, 0, 97, 1, T_NORMAL, T_NONE, 50 },  // 427 normal 0.4m
  { "LOPUNNY", 0, 0, R_EVO, 0x8C4D, 65, 106, 84, 54, 96, 105, 0, 117, 3, T_NORMAL, T_FIGHT, 50 },  // 428 normal 1.2m
  { "MISMAGIUS", 0, 0, R_EVO, 0x6AD3, 60, 60, 60, 105, 105, 105, 0, 115, 1, T_GHOST, T_FAIRY, 50 },  // 429 fantasma 0.9m
  { "HONCHKROW", 0, 0, R_EVO, 0x49E6, 100, 125, 52, 105, 52, 71, 0, 114, 3, T_DARK, T_FLYING, 50 },  // 430 siniestro 0.9m
  { "GLAMEOW", 432, 28, R_RARO, 0x8C4D, 49, 55, 42, 42, 37, 85, 0, 113, 1, T_NORMAL, T_NONE, 75 },  // 431 normal 0.5m
  { "PURUGLY", 0, 0, R_EVO, 0x8C4D, 85, 102, 64, 64, 59, 112, 0, 121, 4, T_NORMAL, T_NONE, 75 },  // 432 normal 1.0m
  { "CHINGLING", 358, 30, R_RARO, 0xD28F, 55, 30, 50, 65, 60, 45, 0, 86, 1, T_PSYCHIC, T_NONE, 50 },  // 433 psiquico 0.2m
  { "STUNKY", 435, 34, R_RARO, 0x8A73, 63, 63, 47, 41, 41, 74, 0, 103, 2, T_POISON, T_DARK, 50 },  // 434 veneno 0.4m
  { "SKUNTANK", 0, 0, R_EVO, 0x8A73, 103, 103, 67, 71, 61, 84, 0, 127, 4, T_POISON, T_DARK, 50 },  // 435 veneno 1.0m
  { "BRONZOR", 437, 33, R_COMUN, 0x7C53, 57, 24, 86, 24, 86, 23, 4, 92, 6, T_STEEL, T_PSYCHIC, 255 },  // 436 acero 0.5m
  { "BRONZONG", 0, 0, R_EVO, 0x7C53, 67, 89, 116, 79, 116, 33, 4, 126, 19, T_STEEL, T_PSYCHIC, 255 },  // 437 acero 1.3m
  { "BONSLY", 185, 25, R_RARO, 0x9407, 50, 80, 95, 10, 45, 50, 4, 95, 2, T_ROCK, T_NONE, 50 },  // 438 roca 0.5m
  { "MIME JR", 122, 20, R_RARO, 0xD28F, 60, 25, 45, 90, 90, 90, 0, 99, 1, T_PSYCHIC, T_FAIRY, 50 },  // 439 psiquico 0.6m
  { "HAPPINY", 113, 0, R_RARO, 0x8C4D, 100, 5, 5, 15, 65, 30, 0, 89, 2, T_NORMAL, T_NONE, 100 },  // 440 normal 0.6m
  { "CHATOT", 0, 0, R_RARO, 0x8C4D, 76, 90, 55, 100, 52, 100, 0, 93, 1, T_NORMAL, T_FLYING, 50 },  // 441 normal 0.5m
  { "SPIRITOMB", 0, 0, R_RARO, 0x6AD3, 60, 92, 108, 92, 108, 35, 0, 123, 11, T_GHOST, T_DARK, 50 },  // 442 fantasma 1.0m
  { "GIBLE", 444, 24, R_RARO, 0x5A98, 58, 70, 45, 40, 45, 42, 1, 107, 2, T_DRAGON, T_GROUND, 50 },  // 443 dragon 0.7m
  { "GABITE", 445, 48, R_EVO, 0x5A98, 68, 90, 65, 50, 55, 82, 1, 119, 6, T_DRAGON, T_GROUND, 50 },  // 444 dragon 1.4m
  { "GARCHOMP", 0, 0, R_EVO, 0x5A98, 108, 130, 95, 80, 85, 102, 1, 137, 10, T_DRAGON, T_GROUND, 50 },  // 445 dragon 1.9m
  { "MUNCHLAX", 143, 35, R_RARO, 0x8C4D, 135, 85, 40, 40, 85, 5, 0, 100, 10, T_NORMAL, T_NONE, 12 },  // 446 normal 0.6m
  { "RIOLU", 448, 35, R_RARO, 0xA2A5, 40, 70, 40, 35, 40, 60, 0, 97, 2, T_FIGHT, T_NONE, 12 },  // 447 lucha 0.7m
  { "LUCARIO", 0, 0, R_EVO, 0xA2A5, 70, 110, 70, 115, 70, 90, 0, 109, 5, T_FIGHT, T_STEEL, 12 },  // 448 lucha 1.2m
  { "HIPPOPOTAS", 450, 34, R_RARO, 0xB447, 68, 72, 78, 38, 42, 32, 4, 111, 5, T_GROUND, T_NONE, 50 },  // 449 tierra 0.8m
  { "HIPPOWDON", 0, 0, R_EVO, 0xB447, 108, 112, 118, 68, 72, 47, 4, 135, 30, T_GROUND, T_NONE, 50 },  // 450 tierra 2.0m
  { "SKORUPI", 452, 32, R_RARO, 0x8A73, 40, 50, 90, 30, 55, 65, 0, 107, 1, T_POISON, T_BUG, 50 },  // 451 veneno 0.8m
  { "DRAPION", 0, 0, R_EVO, 0x8A73, 70, 90, 110, 60, 75, 95, 0, 137, 6, T_POISON, T_DARK, 50 },  // 452 veneno 1.3m
  { "CROAGUNK", 454, 37, R_RARO, 0x8A73, 48, 61, 40, 61, 40, 50, 0, 99, 2, T_POISON, T_FIGHT, 50 },  // 453 veneno 0.7m
  { "TOXICROAK", 0, 0, R_EVO, 0x8A73, 83, 106, 65, 86, 65, 90, 0, 116, 4, T_POISON, T_FIGHT, 50 },  // 454 veneno 1.3m
  { "CARNIVINE", 0, 0, R_RARO, 0x3C49, 80, 110, 72, 90, 72, 46, 2, 123, 3, T_GRASS, T_NONE, 50 },  // 455 planta 1.4m
  { "FINNEON", 457, 31, R_COMUN, 0x4C98, 49, 49, 56, 69, 61, 66, 1, 90, 1, T_WATER, T_NONE, 50 },  // 456 agua 0.4m
  { "LUMINEON", 0, 0, R_EVO, 0x4C98, 79, 69, 76, 100, 86, 91, 1, 115, 2, T_WATER, T_NONE, 50 },  // 457 agua 1.2m
  { "MANTYKE", 226, 24, R_RARO, 0x4C98, 45, 20, 50, 60, 120, 50, 1, 108, 6, T_WATER, T_FLYING, 50 },  // 458 agua 1.0m
  { "SNOVER", 460, 40, R_RARO, 0x3C49, 60, 62, 50, 62, 60, 40, 2, 115, 5, T_GRASS, T_ICE, 50 },  // 459 planta 1.0m
  { "ABOMASNOW", 0, 0, R_EVO, 0x3C49, 90, 92, 85, 92, 95, 60, 2, 135, 14, T_GRASS, T_ICE, 50 },  // 460 planta 2.2m
  { "WEAVILE", 0, 0, R_EVO, 0x49E6, 70, 120, 65, 45, 85, 125, 0, 119, 3, T_DARK, T_ICE, 50 },  // 461 siniestro 1.1m
  { "MAGNEZONE", 0, 0, R_EVO, 0xBCA1, 70, 70, 115, 130, 90, 60, 0, 122, 18, T_ELEC, T_STEEL, 255 },  // 462 electrico 1.2m
  { "LICKILICKY", 0, 0, R_EVO, 0x8C4D, 110, 85, 95, 80, 95, 50, 0, 139, 14, T_NORMAL, T_NONE, 50 },  // 463 normal 1.7m
  { "RHYPERIOR", 0, 0, R_EVO, 0xB447, 115, 140, 130, 55, 55, 40, 4, 142, 28, T_GROUND, T_ROCK, 50 },  // 464 tierra 2.4m
  { "TANGROWTH", 0, 0, R_EVO, 0x3C49, 100, 100, 125, 110, 50, 50, 2, 138, 13, T_GRASS, T_NONE, 50 },  // 465 planta 2.0m
  { "ELECTIVIRE", 0, 0, R_EVO, 0xBCA1, 75, 123, 67, 95, 85, 95, 0, 137, 14, T_ELEC, T_NONE, 25 },  // 466 electrico 1.8m
  { "MAGMORTAR", 0, 0, R_EVO, 0xEA87, 75, 95, 67, 125, 95, 83, 3, 136, 7, T_FIRE, T_NONE, 25 },  // 467 fuego 1.6m
  { "TOGEKISS", 0, 0, R_EVO, 0xDC76, 85, 50, 95, 120, 115, 80, 0, 114, 4, T_FAIRY, T_FLYING, 12 },  // 468 hada 1.5m
  { "YANMEGA", 0, 0, R_EVO, 0x7CC4, 86, 76, 86, 116, 56, 95, 2, 114, 5, T_BUG, T_FLYING, 50 },  // 469 bicho 1.9m
  { "LEAFEON", 0, 0, R_EVO, 0x3C49, 65, 110, 130, 60, 65, 95, 2, 109, 3, T_GRASS, T_NONE, 50 },  // 470 planta 1.0m
  { "GLACEON", 0, 0, R_EVO, 0x4DB8, 65, 60, 110, 130, 95, 65, 5, 110, 3, T_ICE, T_NONE, 50 },  // 471 hielo 0.8m
  { "GLISCOR", 0, 0, R_EVO, 0xB447, 75, 95, 125, 45, 75, 95, 4, 127, 4, T_GROUND, T_FLYING, 50 },  // 472 tierra 2.0m
  { "MAMOSWINE", 0, 0, R_EVO, 0x4DB8, 110, 130, 80, 70, 60, 80, 5, 142, 29, T_ICE, T_GROUND, 50 },  // 473 hielo 2.5m
  { "PORYGON-Z", 0, 0, R_EVO, 0x8C4D, 85, 80, 70, 135, 75, 90, 0, 102, 3, T_NORMAL, T_NONE, 255 },  // 474 normal 0.9m
  { "GALLADE", 0, 0, R_EVO, 0xD28F, 68, 125, 65, 65, 115, 80, 0, 111, 5, T_PSYCHIC, T_FIGHT, 0 },  // 475 psiquico 1.6m
  { "PROBOPASS", 0, 0, R_EVO, 0x9407, 60, 55, 145, 75, 150, 40, 4, 128, 34, T_ROCK, T_STEEL, 50 },  // 476 roca 1.4m
  { "DUSKNOIR", 0, 0, R_EVO, 0x6AD3, 45, 100, 135, 65, 135, 45, 0, 128, 11, T_GHOST, T_NONE, 50 },  // 477 fantasma 2.2m
  { "FROSLASS", 0, 0, R_EVO, 0x4DB8, 70, 80, 70, 90, 70, 110, 5, 106, 3, T_ICE, T_GHOST, 100 },  // 478 hielo 1.3m
  { "ROTOM", 0, 0, R_RARO, 0xBCA1, 50, 50, 77, 95, 77, 91, 0, 103, 1, T_ELEC, T_GHOST, 255 },  // 479 electrico 0.3m
  { "UXIE", 0, 0, R_LEGENDARIO, 0xD28F, 75, 75, 130, 75, 130, 95, 0, 97, 1, T_PSYCHIC, T_NONE, 255 },  // 480 psiquico 0.3m
  { "MESPRIT", 0, 0, R_LEGENDARIO, 0xD28F, 80, 105, 105, 105, 105, 80, 0, 105, 1, T_PSYCHIC, T_NONE, 255 },  // 481 psiquico 0.3m
  { "AZELF", 0, 0, R_LEGENDARIO, 0xD28F, 75, 125, 70, 125, 70, 115, 0, 97, 1, T_PSYCHIC, T_NONE, 255 },  // 482 psiquico 0.3m
  { "DIALGA", 0, 0, R_LEGENDARIO, 0x7C53, 100, 120, 120, 150, 100, 90, 4, 143, 68, T_STEEL, T_DRAGON, 255 },  // 483 acero 5.4m
  { "PALKIA", 0, 0, R_LEGENDARIO, 0x4C98, 90, 120, 100, 150, 120, 100, 1, 140, 34, T_WATER, T_DRAGON, 255 },  // 484 agua 4.2m
  { "HEATRAN", 0, 0, R_LEGENDARIO, 0xEA87, 91, 90, 106, 130, 106, 77, 3, 125, 43, T_FIRE, T_STEEL, 50 },  // 485 fuego 1.7m
  { "REGIGIGAS", 0, 0, R_LEGENDARIO, 0x8C4D, 110, 160, 110, 80, 110, 100, 0, 141, 42, T_NORMAL, T_NONE, 255 },  // 486 normal 3.7m
  { "GIRATINA", 0, 0, R_LEGENDARIO, 0x6AD3, 150, 100, 120, 100, 120, 90, 0, 148, 75, T_GHOST, T_DRAGON, 255 },  // 487 fantasma 4.5m
  { "CRESSELIA", 0, 0, R_LEGENDARIO, 0xD28F, 120, 70, 110, 75, 120, 85, 0, 123, 9, T_PSYCHIC, T_NONE, 100 },  // 488 psiquico 1.5m
  { "PHIONE", 0, 0, R_LEGENDARIO, 0x4C98, 80, 80, 80, 80, 80, 80, 1, 97, 1, T_WATER, T_NONE, 255 },  // 489 agua 0.4m
  { "MANAPHY", 0, 0, R_LEGENDARIO, 0x4C98, 100, 100, 100, 100, 100, 100, 1, 95, 1, T_WATER, T_NONE, 255 },  // 490 agua 0.3m
  { "DARKRAI", 0, 0, R_LEGENDARIO, 0x49E6, 70, 90, 90, 135, 90, 125, 0, 124, 5, T_DARK, T_NONE, 255 },  // 491 siniestro 1.5m
  { "SHAYMIN", 0, 0, R_LEGENDARIO, 0x3C49, 100, 100, 75, 120, 75, 130, 2, 90, 1, T_GRASS, T_FLYING, 255 },  // 492 planta 0.2m
  { "ARCEUS", 0, 0, R_LEGENDARIO, 0x8C4D, 120, 120, 120, 120, 120, 120, 0, 129, 32, T_NORMAL, T_NONE, 255 },  // 493 normal 3.2m
  { "VICTINI", 0, 0, R_LEGENDARIO, 0xD28F, 100, 100, 100, 100, 100, 100, 0, 101, 1, T_PSYCHIC, T_FIRE, 255 },  // 494 psiquico 0.4m
  { "SNIVY", 496, 17, R_COMUN, 0x3C49, 45, 50, 55, 50, 55, 63, 2, 92, 1, T_GRASS, T_NONE, 50 },  // 495 planta 0.6m
  { "SERVINE", 497, 36, R_EVO, 0x3C49, 60, 70, 75, 70, 75, 83, 2, 101, 2, T_GRASS, T_NONE, 50 },  // 496 planta 0.8m
  { "SERPERIOR", 0, 0, R_EVO, 0x3C49, 75, 85, 95, 85, 95, 113, 2, 131, 6, T_GRASS, T_PSYCHIC, 50 },  // 497 planta 3.3m
  { "TEPIG", 499, 17, R_COMUN, 0xEA87, 65, 63, 45, 45, 45, 45, 3, 90, 1, T_FIRE, T_NONE, 50 },  // 498 fuego 0.5m
  { "PIGNITE", 500, 36, R_EVO, 0xEA87, 90, 93, 55, 70, 55, 55, 3, 116, 6, T_FIRE, T_DARK, 50 },  // 499 fuego 1.0m
  { "EMBOAR", 0, 0, R_EVO, 0xEA87, 110, 123, 65, 100, 65, 65, 3, 143, 15, T_FIRE, T_DARK, 50 },  // 500 fuego 1.6m
  { "OSHAWOTT", 502, 17, R_COMUN, 0x4C98, 55, 55, 45, 63, 45, 45, 1, 89, 1, T_WATER, T_NONE, 50 },  // 501 agua 0.5m
  { "DEWOTT", 503, 36, R_EVO, 0x4C98, 75, 75, 60, 83, 60, 60, 1, 102, 2, T_WATER, T_NONE, 50 },  // 502 agua 0.8m
  { "SAMUROTT", 0, 0, R_EVO, 0x4C98, 95, 108, 85, 100, 70, 70, 1, 136, 9, T_WATER, T_FIGHT, 50 },  // 503 agua 1.5m
  { "PATRAT", 505, 20, R_COMUN, 0x8C4D, 45, 55, 39, 35, 39, 42, 0, 99, 1, T_NORMAL, T_NONE, 50 },  // 504 normal 0.5m
  { "WATCHOG", 0, 0, R_EVO, 0x8C4D, 70, 95, 69, 60, 69, 110, 0, 110, 3, T_NORMAL, T_NONE, 50 },  // 505 normal 1.1m
  { "LILLIPUP", 507, 16, R_COMUN, 0x8C4D, 45, 60, 45, 25, 45, 55, 0, 97, 1, T_NORMAL, T_NONE, 50 },  // 506 normal 0.4m
  { "HERDIER", 508, 32, R_EVO, 0x8C4D, 65, 80, 65, 35, 65, 60, 0, 111, 1, T_NORMAL, T_NONE, 50 },  // 507 normal 0.9m
  { "STOUTLAND", 0, 0, R_EVO, 0x8C4D, 85, 110, 90, 45, 90, 80, 0, 134, 6, T_NORMAL, T_NONE, 50 },  // 508 normal 1.2m
  { "PURRLOIN", 510, 20, R_COMUN, 0x49E6, 41, 50, 37, 50, 37, 66, 0, 100, 1, T_DARK, T_NONE, 50 },  // 509 siniestro 0.4m
  { "LIEPARD", 0, 0, R_EVO, 0x49E6, 70, 110, 60, 88, 60, 120, 0, 117, 4, T_DARK, T_NONE, 50 },  // 510 siniestro 1.1m
  { "PANSAGE", 512, 0, R_COMUN, 0x3C49, 50, 60, 48, 53, 48, 64, 2, 100, 1, T_GRASS, T_NONE, 12 },  // 511 planta 0.6m
  { "SIMISAGE", 0, 0, R_EVO, 0x3C49, 75, 98, 63, 98, 63, 101, 2, 123, 3, T_GRASS, T_NONE, 12 },  // 512 planta 1.1m
  { "PANSEAR", 514, 0, R_COMUN, 0xEA87, 50, 53, 48, 60, 48, 64, 3, 100, 1, T_FIRE, T_NONE, 12 },  // 513 fuego 0.6m
  { "SIMISEAR", 0, 0, R_EVO, 0xEA87, 75, 98, 63, 98, 63, 101, 3, 121, 3, T_FIRE, T_NONE, 12 },  // 514 fuego 1.0m
  { "PANPOUR", 516, 0, R_COMUN, 0x4C98, 50, 53, 48, 60, 48, 64, 1, 102, 1, T_WATER, T_NONE, 12 },  // 515 agua 0.6m
  { "SIMIPOUR", 0, 0, R_EVO, 0x4C98, 75, 98, 63, 98, 63, 101, 1, 118, 3, T_WATER, T_NONE, 12 },  // 516 agua 1.0m
  { "MUNNA", 518, 0, R_COMUN, 0xD28F, 76, 25, 45, 67, 55, 24, 0, 90, 2, T_PSYCHIC, T_NONE, 50 },  // 517 psiquico 0.6m
  { "MUSHARNA", 0, 0, R_EVO, 0xD28F, 116, 55, 85, 107, 95, 29, 0, 125, 6, T_PSYCHIC, T_NONE, 50 },  // 518 psiquico 1.1m
  { "PIDOVE", 520, 21, R_COMUN, 0x8C4D, 50, 36, 50, 55, 30, 43, 0, 89, 1, T_NORMAL, T_FLYING, 50 },  // 519 normal 0.3m
  { "TRANQUILL", 521, 32, R_EVO, 0x8C4D, 62, 50, 62, 77, 42, 65, 0, 103, 2, T_NORMAL, T_FLYING, 50 },  // 520 normal 0.6m
  { "UNFEZANT", 0, 0, R_EVO, 0x8C4D, 80, 65, 80, 115, 55, 93, 0, 120, 3, T_NORMAL, T_FLYING, 50 },  // 521 normal 1.2m
  { "BLITZLE", 523, 27, R_COMUN, 0xBCA1, 45, 60, 32, 50, 32, 76, 0, 107, 3, T_ELEC, T_NONE, 50 },  // 522 electrico 0.8m
  { "ZEBSTRIKA", 0, 0, R_EVO, 0xBCA1, 85, 110, 63, 80, 63, 116, 0, 122, 8, T_ELEC, T_NONE, 50 },  // 523 electrico 1.6m
  { "ROGGENROLA", 525, 25, R_COMUN, 0x9407, 55, 75, 85, 25, 25, 15, 4, 81, 2, T_ROCK, T_NONE, 50 },  // 524 roca 0.4m
  { "BOLDORE", 526, 40, R_EVO, 0x9407, 80, 105, 105, 50, 60, 20, 4, 113, 10, T_ROCK, T_NONE, 50 },  // 525 roca 0.9m
  { "GIGALITH", 0, 0, R_EVO, 0x9407, 95, 135, 130, 60, 80, 25, 4, 135, 26, T_ROCK, T_NONE, 50 },  // 526 roca 1.7m
  { "WOOBAT", 528, 25, R_COMUN, 0xD28F, 55, 45, 43, 55, 43, 72, 0, 101, 1, T_PSYCHIC, T_FLYING, 50 },  // 527 psiquico 0.4m
  { "SWOOBAT", 0, 0, R_EVO, 0xD28F, 67, 67, 55, 100, 65, 114, 0, 124, 1, T_PSYCHIC, T_FLYING, 50 },  // 528 psiquico 0.9m
  { "DRILBUR", 530, 31, R_COMUN, 0xB447, 60, 85, 40, 30, 45, 68, 4, 111, 1, T_GROUND, T_NONE, 50 },  // 529 tierra 0.3m
  { "EXCADRILL", 0, 0, R_EVO, 0xB447, 110, 135, 60, 50, 65, 88, 4, 132, 4, T_GROUND, T_STEEL, 50 },  // 530 tierra 0.7m
  { "AUDINO", 0, 0, R_RARO, 0x8C4D, 80, 60, 86, 90, 86, 50, 0, 112, 3, T_NORMAL, T_NONE, 50 },  // 531 normal 1.1m
  { "TIMBURR", 533, 23, R_COMUN, 0xA2A5, 75, 80, 55, 35, 35, 35, 0, 101, 1, T_FIGHT, T_NONE, 25 },  // 532 lucha 0.6m
  { "GURDURR", 534, 45, R_EVO, 0xA2A5, 85, 105, 85, 50, 50, 40, 0, 121, 4, T_FIGHT, T_NONE, 25 },  // 533 lucha 1.2m
  { "CONKELDURR", 0, 0, R_EVO, 0xA2A5, 105, 140, 95, 55, 65, 45, 0, 139, 9, T_FIGHT, T_NONE, 25 },  // 534 lucha 1.4m
  { "TYMPOLE", 536, 20, R_COMUN, 0x4C98, 50, 50, 40, 50, 40, 64, 1, 82, 1, T_WATER, T_NONE, 50 },  // 535 agua 0.5m
  { "PALPITOAD", 537, 36, R_EVO, 0x4C98, 75, 65, 55, 65, 55, 69, 1, 106, 2, T_WATER, T_GROUND, 50 },  // 536 agua 0.8m
  { "SEISMITOAD", 0, 0, R_EVO, 0x4C98, 105, 95, 75, 90, 75, 74, 1, 133, 6, T_WATER, T_GROUND, 50 },  // 537 agua 1.5m
  { "THROH", 0, 0, R_RARO, 0xA2A5, 120, 100, 85, 30, 85, 45, 0, 118, 6, T_FIGHT, T_NONE, 0 },  // 538 lucha 1.3m
  { "SAWK", 0, 0, R_RARO, 0xA2A5, 75, 125, 75, 30, 75, 85, 0, 117, 5, T_FIGHT, T_NONE, 0 },  // 539 lucha 1.4m
  { "SEWADDLE", 541, 20, R_COMUN, 0x7CC4, 45, 53, 70, 40, 60, 42, 2, 88, 1, T_BUG, T_GRASS, 50 },  // 540 bicho 0.3m
  { "SWADLOON", 542, 25, R_EVO, 0x7CC4, 55, 63, 90, 50, 80, 42, 2, 99, 1, T_BUG, T_GRASS, 50 },  // 541 bicho 0.5m
  { "LEAVANNY", 0, 0, R_EVO, 0x7CC4, 75, 110, 80, 70, 70, 92, 2, 112, 2, T_BUG, T_GRASS, 50 },  // 542 bicho 1.2m
  { "VENIPEDE", 544, 22, R_COMUN, 0x7CC4, 30, 45, 59, 30, 39, 57, 2, 97, 1, T_BUG, T_POISON, 50 },  // 543 bicho 0.4m
  { "WHIRLIPEDE", 545, 30, R_EVO, 0x7CC4, 40, 55, 99, 40, 79, 47, 2, 112, 6, T_BUG, T_POISON, 50 },  // 544 bicho 1.2m
  { "SCOLIPEDE", 0, 0, R_EVO, 0x7CC4, 60, 100, 89, 55, 69, 112, 2, 132, 20, T_BUG, T_POISON, 50 },  // 545 bicho 2.5m
  { "COTTONEE", 547, 0, R_COMUN, 0x3C49, 40, 27, 60, 37, 50, 66, 2, 89, 1, T_GRASS, T_FAIRY, 50 },  // 546 planta 0.3m
  { "WHIMSICOTT", 0, 0, R_EVO, 0x3C49, 60, 67, 85, 97, 75, 116, 2, 122, 1, T_GRASS, T_FAIRY, 50 },  // 547 planta 0.7m
  { "PETILIL", 549, 0, R_COMUN, 0x3C49, 45, 35, 50, 70, 50, 30, 2, 91, 1, T_GRASS, T_NONE, 100 },  // 548 planta 0.5m
  { "LILLIGANT", 0, 0, R_EVO, 0x3C49, 70, 60, 75, 110, 75, 90, 2, 122, 2, T_GRASS, T_NONE, 100 },  // 549 planta 1.1m
  { "BASCULIN", 902, 40, R_RARO, 0x4C98, 70, 92, 65, 80, 55, 98, 1, 92, 2, T_WATER, T_NONE, 50 },  // 550 agua 1.0m
  { "SANDILE", 552, 25, R_COMUN, 0xB447, 50, 72, 35, 35, 35, 65, 4, 92, 2, T_GROUND, T_DARK, 50 },  // 551 tierra 0.7m
  { "KROKOROK", 553, 40, R_EVO, 0xB447, 60, 82, 45, 45, 45, 74, 4, 113, 3, T_GROUND, T_DARK, 50 },  // 552 tierra 1.0m
  { "KROOKODILE", 0, 0, R_EVO, 0xB447, 95, 117, 80, 65, 70, 92, 4, 137, 10, T_GROUND, T_DARK, 50 },  // 553 tierra 1.5m
  { "DARUMAKA", 555, 35, R_COMUN, 0xEA87, 70, 90, 45, 15, 45, 50, 3, 93, 4, T_FIRE, T_NONE, 50 },  // 554 fuego 0.6m
  { "DARMANITAN", 0, 0, R_EVO, 0xEA87, 105, 140, 55, 30, 55, 95, 3, 117, 9, T_FIRE, T_NONE, 50 },  // 555 fuego 1.3m
  { "MARACTUS", 0, 0, R_RARO, 0x3C49, 75, 86, 77, 106, 77, 60, 2, 119, 3, T_GRASS, T_NONE, 50 },  // 556 planta 1.0m
  { "DWEBBLE", 558, 32, R_COMUN, 0x7CC4, 50, 65, 85, 35, 35, 55, 2, 95, 1, T_BUG, T_ROCK, 50 },  // 557 bicho 0.3m
  { "CRUSTLE", 0, 0, R_EVO, 0x7CC4, 70, 105, 135, 65, 75, 45, 2, 137, 20, T_BUG, T_ROCK, 50 },  // 558 bicho 1.4m
  { "SCRAGGY", 560, 32, R_COMUN, 0x49E6, 50, 75, 70, 35, 70, 48, 0, 90, 1, T_DARK, T_FIGHT, 50 },  // 559 siniestro 0.6m
  { "SCRAFTY", 0, 0, R_EVO, 0x49E6, 65, 90, 115, 45, 115, 58, 0, 110, 3, T_DARK, T_FIGHT, 50 },  // 560 siniestro 1.1m
  { "SIGILYPH", 0, 0, R_RARO, 0xD28F, 72, 58, 80, 103, 80, 97, 0, 132, 1, T_PSYCHIC, T_FLYING, 50 },  // 561 psiquico 1.4m
  { "YAMASK", 563, 34, R_COMUN, 0x6AD3, 38, 30, 85, 55, 65, 30, 0, 93, 1, T_GHOST, T_NONE, 50 },  // 562 fantasma 0.5m
  { "COFAGRIGUS", 0, 0, R_EVO, 0x6AD3, 58, 50, 145, 95, 105, 30, 0, 141, 8, T_GHOST, T_NONE, 50 },  // 563 fantasma 1.7m
  { "TIRTOUGA", 565, 37, R_RARO, 0x4C98, 54, 78, 103, 53, 45, 22, 1, 103, 2, T_WATER, T_ROCK, 12 },  // 564 agua 0.7m
  { "CARRACOSTA", 0, 0, R_EVO, 0x4C98, 74, 108, 133, 83, 65, 32, 1, 128, 8, T_WATER, T_ROCK, 12 },  // 565 agua 1.2m
  { "ARCHEN", 567, 37, R_RARO, 0x9407, 55, 112, 45, 74, 45, 70, 4, 105, 1, T_ROCK, T_FLYING, 12 },  // 566 roca 0.5m
  { "ARCHEOPS", 0, 0, R_EVO, 0x9407, 75, 140, 65, 112, 65, 110, 4, 132, 3, T_ROCK, T_FLYING, 12 },  // 567 roca 1.4m
  { "TRUBBISH", 569, 26, R_COMUN, 0x8A73, 65, 50, 62, 50, 62, 65, 0, 91, 3, T_POISON, T_NONE, 50 },  // 568 veneno 0.6m
  { "GARBODOR", 0, 0, R_EVO, 0x8A73, 100, 95, 82, 85, 82, 55, 0, 140, 11, T_POISON, T_STEEL, 50 },  // 569 veneno 1.9m
  { "ZORUA", 571, 30, R_RARO, 0x49E6, 40, 65, 40, 80, 40, 65, 0, 102, 1, T_DARK, T_NONE, 12 },  // 570 siniestro 0.7m
  { "ZOROARK", 0, 0, R_EVO, 0x49E6, 60, 105, 60, 120, 60, 105, 0, 124, 8, T_DARK, T_NONE, 12 },  // 571 siniestro 1.6m
  { "MINCCINO", 573, 0, R_COMUN, 0x8C4D, 55, 50, 40, 40, 40, 75, 0, 113, 1, T_NORMAL, T_NONE, 75 },  // 572 normal 0.4m
  { "CINCCINO", 0, 0, R_EVO, 0x8C4D, 75, 95, 60, 65, 60, 115, 0, 124, 1, T_NORMAL, T_NONE, 75 },  // 573 normal 0.5m
  { "GOTHITA", 575, 25, R_COMUN, 0xD28F, 45, 30, 50, 55, 65, 45, 0, 88, 1, T_PSYCHIC, T_NONE, 75 },  // 574 psiquico 0.4m
  { "GOTHORITA", 576, 38, R_EVO, 0xD28F, 60, 45, 70, 75, 85, 55, 0, 105, 2, T_PSYCHIC, T_NONE, 75 },  // 575 psiquico 0.7m
  { "GOTHITELLE", 0, 0, R_EVO, 0xD28F, 70, 55, 95, 95, 110, 65, 0, 124, 4, T_PSYCHIC, T_NONE, 75 },  // 576 psiquico 1.5m
  { "SOLOSIS", 578, 25, R_COMUN, 0xD28F, 45, 30, 40, 105, 50, 20, 0, 86, 1, T_PSYCHIC, T_NONE, 50 },  // 577 psiquico 0.3m
  { "DUOSION", 579, 38, R_EVO, 0xD28F, 65, 40, 50, 125, 60, 30, 0, 107, 1, T_PSYCHIC, T_NONE, 50 },  // 578 psiquico 0.6m
  { "REUNICLUS", 0, 0, R_EVO, 0xD28F, 110, 65, 75, 125, 85, 30, 0, 127, 2, T_PSYCHIC, T_NONE, 50 },  // 579 psiquico 1.0m
  { "DUCKLETT", 581, 30, R_COMUN, 0x4C98, 62, 44, 50, 64, 50, 55, 1, 87, 1, T_WATER, T_FLYING, 50 },  // 580 agua 0.5m
  { "SWANNA", 0, 0, R_EVO, 0x4C98, 75, 87, 63, 107, 63, 98, 1, 112, 2, T_WATER, T_FLYING, 50 },  // 581 agua 1.3m
  { "VANILLITE", 583, 25, R_COMUN, 0x4DB8, 36, 50, 50, 65, 60, 44, 5, 84, 1, T_ICE, T_FAIRY, 50 },  // 582 hielo 0.4m
  { "VANILLISH", 584, 40, R_EVO, 0x4DB8, 51, 65, 65, 80, 75, 59, 5, 100, 4, T_ICE, T_FAIRY, 50 },  // 583 hielo 1.1m
  { "VANILLUXE", 0, 0, R_EVO, 0x4DB8, 71, 75, 85, 130, 95, 79, 5, 119, 6, T_ICE, T_FAIRY, 50 },  // 584 hielo 1.3m
  { "DEERLING", 586, 32, R_COMUN, 0x8C4D, 60, 60, 50, 40, 50, 75, 0, 95, 2, T_NORMAL, T_GRASS, 50 },  // 585 normal 0.6m
  { "SAWSBUCK", 0, 0, R_EVO, 0x8C4D, 80, 115, 70, 60, 70, 105, 0, 114, 9, T_NORMAL, T_GRASS, 50 },  // 586 normal 1.9m
  { "EMOLGA", 0, 0, R_RARO, 0xBCA1, 55, 95, 60, 95, 60, 110, 0, 105, 1, T_ELEC, T_FLYING, 50 },  // 587 electrico 0.4m
  { "KARRABLAST", 589, 32, R_COMUN, 0x7CC4, 50, 75, 45, 40, 45, 60, 2, 89, 1, T_BUG, T_NONE, 50 },  // 588 bicho 0.5m
  { "ESCAVALIER", 0, 0, R_EVO, 0x7CC4, 70, 135, 105, 60, 105, 20, 2, 125, 3, T_BUG, T_STEEL, 50 },  // 589 bicho 1.0m
  { "FOONGUS", 591, 36, R_COMUN, 0x3C49, 69, 55, 45, 55, 55, 15, 2, 86, 1, T_GRASS, T_POISON, 50 },  // 590 planta 0.2m
  { "AMOONGUSS", 0, 0, R_EVO, 0x3C49, 114, 85, 70, 85, 80, 30, 2, 123, 1, T_GRASS, T_POISON, 50 },  // 591 planta 0.6m
  { "FRILLISH", 593, 35, R_COMUN, 0x4C98, 55, 40, 50, 65, 85, 40, 1, 114, 3, T_WATER, T_GHOST, 50 },  // 592 agua 1.2m
  { "JELLICENT", 0, 0, R_EVO, 0x4C98, 100, 60, 70, 85, 105, 60, 1, 133, 14, T_WATER, T_GHOST, 50 },  // 593 agua 2.2m
  { "ALOMOMOLA", 0, 0, R_RARO, 0x4C98, 165, 75, 80, 75, 45, 65, 1, 114, 3, T_WATER, T_NONE, 50 },  // 594 agua 1.2m
  { "JOLTIK", 596, 32, R_COMUN, 0x7CC4, 50, 47, 50, 57, 50, 65, 2, 92, 1, T_BUG, T_ELEC, 50 },  // 595 bicho 0.1m
  { "GALVANTULA", 0, 0, R_EVO, 0x7CC4, 70, 77, 60, 97, 60, 108, 2, 116, 1, T_BUG, T_ELEC, 50 },  // 596 bicho 0.8m
  { "FERROSEED", 598, 40, R_RARO, 0x3C49, 44, 50, 91, 24, 86, 10, 2, 85, 2, T_GRASS, T_STEEL, 50 },  // 597 planta 0.6m
  { "FERROTHORN", 0, 0, R_EVO, 0x3C49, 74, 94, 131, 54, 116, 20, 2, 120, 11, T_GRASS, T_STEEL, 50 },  // 598 planta 1.0m
  { "KLINK", 600, 25, R_COMUN, 0x7C53, 40, 55, 70, 45, 60, 30, 4, 95, 2, T_STEEL, T_NONE, 255 },  // 599 acero 0.3m
  { "KLANG", 601, 40, R_EVO, 0x7C53, 60, 80, 95, 70, 85, 50, 4, 119, 5, T_STEEL, T_NONE, 255 },  // 600 acero 0.6m
  { "KLINKLANG", 0, 0, R_EVO, 0x7C53, 60, 100, 115, 70, 85, 90, 4, 127, 8, T_STEEL, T_NONE, 255 },  // 601 acero 0.6m
  { "TYNAMO", 603, 30, R_RARO, 0xBCA1, 35, 55, 40, 45, 40, 60, 0, 78, 1, T_ELEC, T_NONE, 50 },  // 602 electrico 0.2m
  { "EELEKTRIK", 604, 0, R_EVO, 0xBCA1, 65, 85, 70, 75, 70, 40, 0, 112, 2, T_ELEC, T_NONE, 50 },  // 603 electrico 1.2m
  { "EELEKTROSS", 0, 0, R_EVO, 0xBCA1, 85, 115, 80, 105, 80, 50, 0, 135, 8, T_ELEC, T_NONE, 50 },  // 604 electrico 2.1m
  { "ELGYEM", 606, 33, R_RARO, 0xD28F, 55, 55, 55, 85, 55, 30, 0, 94, 1, T_PSYCHIC, T_NONE, 50 },  // 605 psiquico 0.5m
  { "BEHEEYEM", 0, 0, R_EVO, 0xD28F, 75, 75, 85, 125, 95, 40, 0, 109, 3, T_PSYCHIC, T_NONE, 50 },  // 606 psiquico 1.0m
  { "LITWICK", 608, 34, R_RARO, 0x6AD3, 50, 50, 55, 65, 65, 20, 0, 87, 1, T_GHOST, T_FIRE, 50 },  // 607 fantasma 0.3m
  { "LAMPENT", 609, 0, R_EVO, 0x6AD3, 60, 55, 65, 95, 65, 55, 0, 101, 1, T_GHOST, T_FIRE, 50 },  // 608 fantasma 0.6m
  { "CHANDELURE", 0, 0, R_EVO, 0x6AD3, 60, 55, 90, 145, 90, 80, 0, 115, 3, T_GHOST, T_FIRE, 50 },  // 609 fantasma 1.0m
  { "AXEW", 611, 30, R_RARO, 0x5A98, 46, 87, 60, 30, 40, 57, 1, 94, 2, T_DRAGON, T_NONE, 50 },  // 610 dragon 0.6m
  { "FRAXURE", 612, 48, R_EVO, 0x5A98, 66, 117, 70, 40, 50, 67, 1, 117, 4, T_DRAGON, T_NONE, 50 },  // 611 dragon 1.0m
  { "HAXORUS", 0, 0, R_EVO, 0x5A98, 76, 147, 90, 60, 70, 97, 1, 130, 11, T_DRAGON, T_NONE, 50 },  // 612 dragon 1.8m
  { "CUBCHOO", 614, 34, R_COMUN, 0x4DB8, 55, 70, 40, 60, 40, 40, 5, 90, 1, T_ICE, T_NONE, 50 },  // 613 hielo 0.5m
  { "BEARTIC", 0, 0, R_EVO, 0x4DB8, 95, 130, 80, 70, 80, 50, 5, 131, 26, T_ICE, T_NONE, 50 },  // 614 hielo 2.6m
  { "CRYOGONAL", 0, 0, R_RARO, 0x4DB8, 80, 50, 50, 95, 135, 105, 5, 132, 15, T_ICE, T_NONE, 255 },  // 615 hielo 1.1m
  { "SHELMET", 617, 32, R_COMUN, 0x7CC4, 50, 40, 85, 40, 65, 25, 2, 96, 1, T_BUG, T_NONE, 50 },  // 616 bicho 0.4m
  { "ACCELGOR", 0, 0, R_EVO, 0x7CC4, 80, 70, 40, 100, 60, 145, 2, 115, 3, T_BUG, T_NONE, 50 },  // 617 bicho 0.8m
  { "STUNFISK", 0, 0, R_RARO, 0xB447, 109, 66, 84, 81, 99, 32, 4, 104, 1, T_GROUND, T_ELEC, 50 },  // 618 tierra 0.7m
  { "MIENFOO", 620, 34, R_RARO, 0xA2A5, 45, 85, 50, 55, 50, 65, 0, 97, 2, T_FIGHT, T_NONE, 50 },  // 619 lucha 0.9m
  { "MIENSHAO", 0, 0, R_EVO, 0xA2A5, 65, 125, 60, 95, 60, 105, 0, 116, 4, T_FIGHT, T_NONE, 50 },  // 620 lucha 1.4m
  { "DRUDDIGON", 0, 0, R_RARO, 0x5A98, 77, 120, 90, 60, 90, 48, 1, 134, 14, T_DRAGON, T_NONE, 50 },  // 621 dragon 1.6m
  { "GOLETT", 623, 35, R_RARO, 0xB447, 59, 74, 50, 35, 50, 35, 4, 118, 9, T_GROUND, T_GHOST, 255 },  // 622 tierra 1.0m
  { "GOLURK", 0, 0, R_EVO, 0xB447, 89, 124, 80, 55, 80, 55, 4, 144, 33, T_GROUND, T_GHOST, 255 },  // 623 tierra 2.8m
  { "PAWNIARD", 625, 32, R_RARO, 0x49E6, 45, 85, 70, 40, 40, 60, 0, 99, 1, T_DARK, T_STEEL, 50 },  // 624 siniestro 0.5m
  { "BISHARP", 983, 42, R_EVO, 0x49E6, 65, 125, 100, 60, 70, 70, 0, 116, 7, T_DARK, T_STEEL, 50 },  // 625 siniestro 1.6m
  { "BOUFFALANT", 0, 0, R_RARO, 0x8C4D, 95, 110, 95, 40, 95, 55, 0, 123, 9, T_NORMAL, T_NONE, 50 },  // 626 normal 1.6m
  { "RUFFLET", 628, 35, R_RARO, 0x8C4D, 70, 83, 50, 37, 50, 60, 0, 99, 1, T_NORMAL, T_FLYING, 0 },  // 627 normal 0.5m
  { "BRAVIARY", 0, 0, R_EVO, 0x8C4D, 100, 123, 75, 57, 75, 80, 0, 143, 4, T_NORMAL, T_FLYING, 0 },  // 628 normal 1.5m
  { "VULLABY", 630, 35, R_RARO, 0x49E6, 70, 55, 75, 45, 65, 60, 0, 109, 1, T_DARK, T_FLYING, 100 },  // 629 siniestro 0.5m
  { "MANDIBUZZ", 0, 0, R_EVO, 0x49E6, 110, 65, 105, 55, 95, 80, 0, 123, 4, T_DARK, T_FLYING, 100 },  // 630 siniestro 1.2m
  { "HEATMOR", 0, 0, R_RARO, 0xEA87, 85, 110, 86, 95, 86, 65, 3, 125, 6, T_FIRE, T_STEEL, 50 },  // 631 fuego 1.4m
  { "DURANT", 0, 0, R_RARO, 0x7CC4, 58, 109, 112, 48, 48, 109, 2, 98, 3, T_BUG, T_STEEL, 50 },  // 632 bicho 0.3m
  { "DEINO", 634, 36, R_RARO, 0x49E6, 52, 65, 50, 45, 50, 38, 0, 97, 2, T_DARK, T_DRAGON, 50 },  // 633 siniestro 0.8m
  { "ZWEILOUS", 635, 55, R_EVO, 0x49E6, 72, 85, 70, 65, 70, 58, 0, 117, 5, T_DARK, T_DRAGON, 50 },  // 634 siniestro 1.4m
  { "HYDREIGON", 0, 0, R_EVO, 0x49E6, 92, 105, 90, 125, 90, 98, 0, 138, 16, T_DARK, T_DRAGON, 50 },  // 635 siniestro 1.8m
  { "LARVESTA", 637, 55, R_RARO, 0x7CC4, 75, 85, 55, 85, 55, 70, 2, 101, 3, T_BUG, T_FIRE, 50 },  // 636 bicho 1.1m
  { "VOLCARONA", 0, 0, R_EVO, 0x7CC4, 85, 60, 65, 135, 105, 100, 2, 132, 5, T_BUG, T_FIRE, 50 },  // 637 bicho 1.6m
  { "COBALION", 0, 0, R_LEGENDARIO, 0x7C53, 91, 90, 129, 90, 72, 108, 4, 123, 25, T_STEEL, T_FIGHT, 255 },  // 638 acero 2.1m
  { "TERRAKION", 0, 0, R_LEGENDARIO, 0x9407, 91, 129, 90, 72, 90, 108, 4, 133, 26, T_ROCK, T_FIGHT, 255 },  // 639 roca 1.9m
  { "VIRIZION", 0, 0, R_LEGENDARIO, 0x3C49, 91, 90, 72, 90, 129, 108, 2, 116, 20, T_GRASS, T_FIGHT, 255 },  // 640 planta 2.0m
  { "TORNADUS", 0, 0, R_LEGENDARIO, 0x8C4D, 79, 115, 70, 125, 80, 111, 0, 133, 6, T_FLYING, T_NONE, 0 },  // 641 normal 1.5m
  { "THUNDURUS", 0, 0, R_LEGENDARIO, 0xBCA1, 79, 115, 70, 125, 80, 111, 0, 136, 6, T_ELEC, T_FLYING, 0 },  // 642 electrico 1.5m
  { "RESHIRAM", 0, 0, R_LEGENDARIO, 0x5A98, 100, 120, 100, 150, 120, 90, 1, 155, 33, T_DRAGON, T_FIRE, 255 },  // 643 dragon 3.2m
  { "ZEKROM", 0, 0, R_LEGENDARIO, 0x5A98, 100, 150, 120, 120, 100, 90, 1, 149, 34, T_DRAGON, T_ELEC, 255 },  // 644 dragon 2.9m
  { "LANDORUS", 0, 0, R_LEGENDARIO, 0xB447, 89, 125, 90, 115, 80, 101, 4, 138, 7, T_GROUND, T_FLYING, 0 },  // 645 tierra 1.5m
  { "KYUREM", 0, 0, R_LEGENDARIO, 0x5A98, 125, 130, 90, 130, 90, 95, 1, 137, 32, T_DRAGON, T_ICE, 255 },  // 646 dragon 3.0m
  { "KELDEO", 0, 0, R_LEGENDARIO, 0x4C98, 91, 72, 90, 129, 90, 108, 1, 113, 5, T_WATER, T_FIGHT, 255 },  // 647 agua 1.4m
  { "MELOETTA", 0, 0, R_LEGENDARIO, 0x8C4D, 100, 77, 77, 128, 128, 90, 0, 103, 1, T_NORMAL, T_PSYCHIC, 255 },  // 648 normal 0.6m
  { "GENESECT", 0, 0, R_LEGENDARIO, 0x7CC4, 71, 120, 95, 120, 95, 99, 2, 123, 8, T_BUG, T_STEEL, 255 },  // 649 bicho 1.5m
  { "CHESPIN", 651, 16, R_COMUN, 0x3C49, 56, 61, 65, 48, 45, 38, 2, 101, 1, T_GRASS, T_NONE, 50 },  // 650 planta 0.4m
  { "QUILLADIN", 652, 36, R_EVO, 0x3C49, 71, 78, 95, 56, 58, 57, 2, 113, 3, T_GRASS, T_NONE, 50 },  // 651 planta 0.7m
  { "CHESNAUGHT", 0, 0, R_EVO, 0x3C49, 100, 117, 122, 64, 75, 64, 2, 144, 9, T_GRASS, T_FIGHT, 50 },  // 652 planta 1.6m
  { "FENNEKIN", 654, 16, R_COMUN, 0xEA87, 40, 45, 40, 62, 60, 60, 3, 104, 1, T_FIRE, T_NONE, 50 },  // 653 fuego 0.4m
  { "BRAIXEN", 655, 36, R_EVO, 0xEA87, 59, 59, 58, 90, 70, 73, 3, 116, 1, T_FIRE, T_NONE, 50 },  // 654 fuego 1.0m
  { "DELPHOX", 0, 0, R_EVO, 0xEA87, 75, 69, 72, 114, 100, 104, 3, 135, 4, T_FIRE, T_PSYCHIC, 50 },  // 655 fuego 1.5m
  { "FROAKIE", 657, 16, R_COMUN, 0x4C98, 41, 56, 40, 62, 44, 71, 1, 92, 1, T_WATER, T_NONE, 50 },  // 656 agua 0.3m
  { "FROGADIER", 658, 36, R_EVO, 0x4C98, 54, 63, 52, 83, 56, 97, 1, 105, 1, T_WATER, T_NONE, 50 },  // 657 agua 0.6m
  { "GRENINJA", 0, 0, R_EVO, 0x4C98, 72, 95, 67, 103, 71, 122, 1, 127, 4, T_WATER, T_DARK, 50 },  // 658 agua 1.5m
  { "BUNNELBY", 660, 20, R_COMUN, 0x8C4D, 38, 35, 38, 32, 36, 57, 0, 105, 1, T_NORMAL, T_NONE, 50 },  // 659 normal 0.4m
  { "DIGGERSBY", 0, 0, R_EVO, 0x8C4D, 85, 56, 77, 50, 77, 78, 0, 135, 4, T_NORMAL, T_GROUND, 50 },  // 660 normal 1.0m
  { "FLETCHLING", 662, 17, R_COMUN, 0x8C4D, 45, 50, 43, 40, 38, 62, 0, 86, 1, T_NORMAL, T_FLYING, 50 },  // 661 normal 0.3m
  { "FLETCHINDER", 663, 35, R_EVO, 0xEA87, 62, 73, 55, 56, 52, 84, 3, 109, 2, T_FIRE, T_FLYING, 50 },  // 662 fuego 0.7m
  { "TALONFLAME", 0, 0, R_EVO, 0xEA87, 78, 90, 71, 74, 69, 126, 3, 130, 2, T_FIRE, T_FLYING, 50 },  // 663 fuego 1.2m
  { "SCATTERBUG", 665, 9, R_COMUN, 0x7CC4, 38, 35, 40, 27, 25, 35, 2, 89, 1, T_BUG, T_NONE, 50 },  // 664 bicho 0.3m
  { "SPEWPA", 666, 12, R_EVO, 0x7CC4, 45, 22, 60, 27, 30, 29, 2, 92, 1, T_BUG, T_NONE, 50 },  // 665 bicho 0.3m
  { "VIVILLON", 0, 0, R_EVO, 0x7CC4, 80, 52, 50, 105, 90, 89, 2, 122, 2, T_BUG, T_FLYING, 50 },  // 666 bicho 1.2m
  { "LITLEO", 668, 30, R_RARO, 0xEA87, 62, 50, 58, 73, 54, 72, 3, 101, 1, T_FIRE, T_NORMAL, 87 },  // 667 fuego 0.6m
  { "PYROAR", 0, 0, R_EVO, 0xEA87, 86, 68, 72, 115, 66, 106, 3, 130, 8, T_FIRE, T_NORMAL, 87 },  // 668 fuego 1.5m
  { "FLABEBE", 670, 19, R_RARO, 0xDC76, 44, 38, 39, 61, 79, 42, 0, 105, 1, T_FAIRY, T_NONE, 100 },  // 669 hada 0.1m
  { "FLOETTE", 671, 0, R_EVO, 0xDC76, 54, 45, 47, 75, 98, 52, 0, 116, 1, T_FAIRY, T_NONE, 100 },  // 670 hada 0.2m
  { "FLORGES", 0, 0, R_EVO, 0xDC76, 78, 65, 68, 112, 154, 75, 0, 129, 1, T_FAIRY, T_NONE, 100 },  // 671 hada 1.1m
  { "SKIDDO", 673, 32, R_COMUN, 0x3C49, 66, 65, 48, 62, 57, 52, 2, 105, 3, T_GRASS, T_NONE, 50 },  // 672 planta 0.9m
  { "GOGOAT", 0, 0, R_EVO, 0x3C49, 123, 100, 62, 97, 81, 68, 2, 122, 9, T_GRASS, T_NONE, 50 },  // 673 planta 1.7m
  { "PANCHAM", 675, 32, R_RARO, 0xA2A5, 67, 82, 62, 46, 48, 43, 0, 91, 1, T_FIGHT, T_NONE, 50 },  // 674 lucha 0.6m
  { "PANGORO", 0, 0, R_EVO, 0xA2A5, 95, 124, 78, 69, 71, 58, 0, 135, 14, T_FIGHT, T_DARK, 50 },  // 675 lucha 2.1m
  { "FURFROU", 0, 0, R_RARO, 0x8C4D, 75, 80, 60, 65, 90, 102, 0, 117, 3, T_NORMAL, T_NONE, 50 },  // 676 normal 1.2m
  { "ESPURR", 678, 25, R_RARO, 0xD28F, 62, 48, 54, 63, 60, 68, 0, 93, 1, T_PSYCHIC, T_NONE, 50 },  // 677 psiquico 0.3m
  { "MEOWSTIC", 0, 0, R_EVO, 0xD28F, 74, 48, 76, 83, 81, 104, 0, 110, 1, T_PSYCHIC, T_NONE, 50 },  // 678 psiquico 0.6m
  { "HONEDGE", 680, 35, R_RARO, 0x7C53, 45, 80, 100, 35, 37, 28, 4, 109, 1, T_STEEL, T_GHOST, 50 },  // 679 acero 0.8m
  { "DOUBLADE", 681, 0, R_EVO, 0x7C53, 59, 110, 150, 45, 49, 35, 4, 122, 1, T_STEEL, T_GHOST, 50 },  // 680 acero 0.8m
  { "AEGISLASH", 0, 0, R_EVO, 0x7C53, 60, 50, 140, 50, 140, 60, 4, 140, 5, T_STEEL, T_GHOST, 50 },  // 681 acero 1.7m
  { "SPRITZEE", 683, 27, R_RARO, 0xDC76, 78, 52, 60, 63, 65, 23, 0, 93, 1, T_FAIRY, T_NONE, 50 },  // 682 hada 0.2m
  { "AROMATISSE", 0, 0, R_EVO, 0xDC76, 101, 72, 72, 99, 89, 29, 0, 114, 2, T_FAIRY, T_NONE, 50 },  // 683 hada 0.8m
  { "SWIRLIX", 685, 27, R_RARO, 0xDC76, 62, 48, 66, 59, 57, 49, 0, 94, 1, T_FAIRY, T_NONE, 50 },  // 684 hada 0.4m
  { "SLURPUFF", 0, 0, R_EVO, 0xDC76, 82, 80, 86, 85, 75, 72, 0, 112, 1, T_FAIRY, T_NONE, 50 },  // 685 hada 0.8m
  { "INKAY", 687, 30, R_RARO, 0x49E6, 53, 54, 53, 37, 46, 45, 0, 100, 1, T_DARK, T_PSYCHIC, 50 },  // 686 siniestro 0.4m
  { "MALAMAR", 0, 0, R_EVO, 0x49E6, 86, 100, 88, 68, 85, 73, 0, 134, 5, T_DARK, T_PSYCHIC, 50 },  // 687 siniestro 1.5m
  { "BINACLE", 689, 39, R_RARO, 0x9407, 42, 52, 67, 39, 56, 50, 4, 107, 3, T_ROCK, T_WATER, 50 },  // 688 roca 0.5m
  { "BARBARACLE", 0, 0, R_EVO, 0x9407, 72, 115, 115, 86, 68, 54, 4, 137, 10, T_ROCK, T_WATER, 50 },  // 689 roca 1.3m
  { "SKRELP", 691, 35, R_RARO, 0x8A73, 50, 60, 60, 60, 60, 30, 0, 101, 1, T_POISON, T_WATER, 50 },  // 690 veneno 0.5m
  { "DRAGALGE", 0, 0, R_EVO, 0x8A73, 65, 75, 90, 97, 123, 44, 0, 133, 8, T_POISON, T_DRAGON, 50 },  // 691 veneno 1.8m
  { "CLAUNCHER", 693, 35, R_RARO, 0x4C98, 50, 53, 62, 58, 63, 44, 1, 99, 1, T_WATER, T_NONE, 50 },  // 692 agua 0.5m
  { "CLAWITZER", 0, 0, R_EVO, 0x4C98, 71, 73, 88, 120, 89, 59, 1, 139, 4, T_WATER, T_NONE, 50 },  // 693 agua 1.3m
  { "HELIOPTILE", 695, 0, R_RARO, 0xBCA1, 44, 38, 33, 61, 43, 70, 0, 93, 1, T_ELEC, T_NORMAL, 50 },  // 694 electrico 0.5m
  { "HELIOLISK", 0, 0, R_EVO, 0xBCA1, 62, 55, 52, 115, 94, 109, 0, 115, 2, T_ELEC, T_NORMAL, 50 },  // 695 electrico 1.0m
  { "TYRUNT", 697, 39, R_RARO, 0x9407, 58, 89, 77, 45, 45, 48, 4, 113, 3, T_ROCK, T_DRAGON, 12 },  // 696 roca 0.8m
  { "TYRANTRUM", 0, 0, R_EVO, 0x9407, 82, 121, 119, 69, 59, 71, 4, 141, 27, T_ROCK, T_DRAGON, 12 },  // 697 roca 2.5m
  { "AMAURA", 699, 39, R_RARO, 0x9407, 77, 59, 50, 67, 63, 46, 4, 100, 3, T_ROCK, T_ICE, 12 },  // 698 roca 1.3m
  { "AURORUS", 0, 0, R_EVO, 0x9407, 123, 77, 72, 99, 92, 58, 4, 135, 22, T_ROCK, T_ICE, 12 },  // 699 roca 2.7m
  { "SYLVEON", 0, 0, R_EVO, 0xDC76, 95, 65, 65, 110, 130, 60, 0, 111, 2, T_FAIRY, T_NONE, 50 },  // 700 hada 1.0m
  { "HAWLUCHA", 0, 0, R_RARO, 0xA2A5, 78, 92, 75, 74, 63, 118, 0, 121, 2, T_FIGHT, T_FLYING, 50 },  // 701 lucha 0.8m
  { "DEDENNE", 0, 0, R_RARO, 0xBCA1, 67, 58, 57, 100, 85, 101, 0, 98, 1, T_ELEC, T_FAIRY, 50 },  // 702 electrico 0.2m
  { "CARBINK", 0, 0, R_RARO, 0x9407, 50, 50, 150, 50, 150, 50, 4, 95, 1, T_ROCK, T_FAIRY, 255 },  // 703 roca 0.3m
  { "GOOMY", 705, 40, R_RARO, 0x5A98, 45, 50, 35, 55, 75, 40, 1, 97, 1, T_DRAGON, T_NONE, 50 },  // 704 dragon 0.3m
  { "SLIGGOO", 706, 50, R_EVO, 0x5A98, 68, 75, 53, 83, 113, 60, 1, 104, 2, T_DRAGON, T_NONE, 50 },  // 705 dragon 0.8m
  { "GOODRA", 0, 0, R_EVO, 0x5A98, 90, 100, 70, 110, 150, 80, 1, 124, 15, T_DRAGON, T_NONE, 50 },  // 706 dragon 2.0m
  { "KLEFKI", 0, 0, R_RARO, 0x7C53, 57, 80, 91, 80, 87, 75, 4, 114, 1, T_STEEL, T_FAIRY, 50 },  // 707 acero 0.2m
  { "PHANTUMP", 709, 38, R_RARO, 0x6AD3, 43, 70, 48, 50, 60, 38, 0, 96, 1, T_GHOST, T_GRASS, 50 },  // 708 fantasma 0.4m
  { "TREVENANT", 0, 0, R_EVO, 0x6AD3, 85, 110, 86, 65, 82, 56, 0, 145, 7, T_GHOST, T_GRASS, 50 },  // 709 fantasma 1.5m
  { "PUMPKABOO", 711, 37, R_RARO, 0x6AD3, 59, 66, 70, 44, 55, 41, 0, 94, 1, T_GHOST, T_GRASS, 50 },  // 710 fantasma 0.3m
  { "GOURGEIST", 0, 0, R_EVO, 0x6AD3, 85, 100, 122, 58, 75, 54, 0, 117, 1, T_GHOST, T_GRASS, 50 },  // 711 fantasma 0.7m
  { "BERGMITE", 713, 37, R_RARO, 0x4DB8, 55, 69, 85, 32, 35, 28, 5, 96, 10, T_ICE, T_NONE, 50 },  // 712 hielo 1.0m
  { "AVALUGG", 0, 0, R_EVO, 0x4DB8, 95, 117, 184, 44, 46, 28, 5, 138, 50, T_ICE, T_NONE, 50 },  // 713 hielo 2.0m
  { "NOIBAT", 715, 42, R_RARO, 0x5A98, 60, 65, 55, 65, 50, 80, 1, 111, 1, T_FLYING, T_DRAGON, 50 },  // 714 dragon 0.5m
  { "NOIVERN", 0, 0, R_EVO, 0x5A98, 85, 70, 80, 97, 80, 123, 1, 164, 8, T_FLYING, T_DRAGON, 50 },  // 715 dragon 1.5m
  { "XERNEAS", 0, 0, R_LEGENDARIO, 0xDC76, 126, 131, 95, 131, 98, 99, 0, 136, 22, T_FAIRY, T_NONE, 255 },  // 716 hada 3.0m
  { "YVELTAL", 0, 0, R_LEGENDARIO, 0x49E6, 126, 131, 95, 131, 98, 99, 0, 141, 20, T_DARK, T_FLYING, 255 },  // 717 siniestro 5.8m
  { "ZYGARDE", 0, 0, R_LEGENDARIO, 0x5A98, 108, 100, 121, 81, 95, 95, 1, 141, 30, T_DRAGON, T_GROUND, 255 },  // 718 dragon 5.0m
  { "DIANCIE", 0, 0, R_LEGENDARIO, 0x9407, 50, 100, 150, 100, 150, 50, 4, 114, 1, T_ROCK, T_FAIRY, 255 },  // 719 roca 0.7m
  { "HOOPA", 0, 0, R_LEGENDARIO, 0xD28F, 80, 110, 60, 150, 130, 70, 0, 106, 1, T_PSYCHIC, T_GHOST, 255 },  // 720 psiquico 0.5m
  { "VOLCANION", 0, 0, R_LEGENDARIO, 0xEA87, 100, 70, 120, 130, 130, 80, 3, 143, 20, T_FIRE, T_WATER, 255 },  // 721 fuego 1.7m
  { "ROWLET", 723, 16, R_COMUN, 0x3C49, 68, 55, 55, 50, 50, 42, 2, 91, 1, T_GRASS, T_FLYING, 50 },  // 722 planta 0.3m
  { "DARTRIX", 724, 36, R_EVO, 0x3C49, 78, 75, 75, 70, 70, 52, 2, 109, 2, T_GRASS, T_FLYING, 50 },  // 723 planta 0.7m
  { "DECIDUEYE", 0, 0, R_EVO, 0x3C49, 78, 117, 75, 80, 75, 105, 2, 130, 4, T_GRASS, T_GHOST, 50 },  // 724 planta 1.6m
  { "LITTEN", 726, 16, R_COMUN, 0xEA87, 45, 65, 40, 60, 40, 70, 3, 101, 1, T_FIRE, T_NONE, 50 },  // 725 fuego 0.4m
  { "TORRACAT", 727, 36, R_EVO, 0xEA87, 65, 85, 50, 80, 50, 90, 3, 110, 2, T_FIRE, T_NONE, 50 },  // 726 fuego 0.7m
  { "INCINEROAR", 0, 0, R_EVO, 0xEA87, 95, 115, 90, 80, 90, 60, 3, 139, 8, T_FIRE, T_DARK, 50 },  // 727 fuego 1.8m
  { "POPPLIO", 729, 16, R_COMUN, 0x4C98, 50, 54, 54, 66, 56, 40, 1, 96, 1, T_WATER, T_NONE, 50 },  // 728 agua 0.4m
  { "BRIONNE", 730, 36, R_EVO, 0x4C98, 60, 69, 69, 91, 81, 50, 1, 107, 2, T_WATER, T_NONE, 50 },  // 729 agua 0.6m
  { "PRIMARINA", 0, 0, R_EVO, 0x4C98, 80, 74, 74, 126, 116, 60, 1, 132, 4, T_WATER, T_FAIRY, 50 },  // 730 agua 1.8m
  { "PIKIPEK", 732, 14, R_COMUN, 0x8C4D, 35, 75, 30, 30, 30, 65, 0, 86, 1, T_NORMAL, T_FLYING, 50 },  // 731 normal 0.3m
  { "TRUMBEAK", 733, 28, R_EVO, 0x8C4D, 55, 85, 50, 40, 50, 75, 0, 98, 1, T_NORMAL, T_FLYING, 50 },  // 732 normal 0.6m
  { "TOUCANNON", 0, 0, R_EVO, 0x8C4D, 80, 120, 75, 75, 75, 60, 0, 112, 3, T_NORMAL, T_FLYING, 50 },  // 733 normal 1.1m
  { "YUNGOOS", 735, 20, R_COMUN, 0x8C4D, 48, 70, 30, 30, 30, 45, 0, 102, 1, T_NORMAL, T_NONE, 50 },  // 734 normal 0.4m
  { "GUMSHOOS", 0, 0, R_EVO, 0x8C4D, 88, 110, 60, 55, 60, 85, 0, 116, 1, T_NORMAL, T_NONE, 50 },  // 735 normal 0.7m
  { "GRUBBIN", 737, 20, R_COMUN, 0x7CC4, 47, 62, 45, 55, 45, 46, 2, 87, 1, T_BUG, T_NONE, 50 },  // 736 bicho 0.4m
  { "CHARJABUG", 738, 36, R_EVO, 0x7CC4, 57, 82, 95, 55, 75, 36, 2, 100, 1, T_BUG, T_ELEC, 50 },  // 737 bicho 0.5m
  { "VIKAVOLT", 0, 0, R_EVO, 0x7CC4, 77, 70, 90, 145, 75, 43, 2, 120, 4, T_BUG, T_ELEC, 50 },  // 738 bicho 1.5m
  { "CRABRAWLER", 740, 36, R_COMUN, 0xA2A5, 47, 82, 57, 42, 47, 63, 0, 115, 1, T_FIGHT, T_NONE, 50 },  // 739 lucha 0.6m
  { "CRABOMINABLE", 0, 0, R_EVO, 0xA2A5, 97, 132, 77, 62, 67, 43, 0, 152, 18, T_FIGHT, T_ICE, 50 },  // 740 lucha 1.7m
  { "ORICORIO", 0, 0, R_RARO, 0xEA87, 75, 70, 70, 98, 70, 93, 3, 126, 1, T_FIRE, T_FLYING, 75 },  // 741 fuego 0.6m
  { "CUTIEFLY", 743, 25, R_COMUN, 0x7CC4, 40, 45, 40, 55, 40, 84, 2, 89, 1, T_BUG, T_FAIRY, 50 },  // 742 bicho 0.1m
  { "RIBOMBEE", 0, 0, R_EVO, 0x7CC4, 60, 55, 60, 95, 75, 124, 2, 102, 1, T_BUG, T_FAIRY, 50 },  // 743 bicho 0.2m
  { "ROCKRUFF", 745, 25, R_RARO, 0x9407, 45, 65, 40, 30, 40, 60, 4, 102, 1, T_ROCK, T_NONE, 50 },  // 744 roca 0.5m
  { "LYCANROC", 0, 0, R_EVO, 0x9407, 75, 115, 65, 55, 65, 112, 4, 122, 2, T_ROCK, T_NONE, 50 },  // 745 roca 0.8m
  { "WISHIWASHI", 0, 0, R_RARO, 0x4C98, 45, 20, 20, 25, 25, 40, 1, 91, 1, T_WATER, T_NONE, 50 },  // 746 agua 0.2m
  { "MAREANIE", 748, 38, R_COMUN, 0x8A73, 50, 53, 62, 43, 52, 45, 0, 111, 1, T_POISON, T_WATER, 50 },  // 747 veneno 0.4m
  { "TOXAPEX", 0, 0, R_EVO, 0x8A73, 50, 63, 152, 53, 142, 35, 0, 134, 1, T_POISON, T_WATER, 50 },  // 748 veneno 0.7m
  { "MUDBRAY", 750, 30, R_COMUN, 0xB447, 70, 100, 70, 45, 55, 45, 4, 107, 11, T_GROUND, T_NONE, 50 },  // 749 tierra 1.0m
  { "MUDSDALE", 0, 0, R_EVO, 0xB447, 100, 125, 100, 55, 85, 35, 4, 135, 92, T_GROUND, T_NONE, 50 },  // 750 tierra 2.5m
  { "DEWPIDER", 752, 26, R_RARO, 0x4C98, 38, 40, 52, 40, 72, 27, 1, 95, 1, T_WATER, T_BUG, 50 },  // 751 agua 0.3m
  { "ARAQUANID", 0, 0, R_EVO, 0x4C98, 68, 70, 92, 50, 132, 42, 1, 127, 8, T_WATER, T_BUG, 50 },  // 752 agua 1.8m
  { "FOMANTIS", 754, 34, R_RARO, 0x3C49, 40, 55, 35, 50, 35, 35, 2, 93, 1, T_GRASS, T_NONE, 50 },  // 753 planta 0.3m
  { "LURANTIS", 0, 0, R_EVO, 0x3C49, 70, 105, 90, 80, 90, 45, 2, 117, 2, T_GRASS, T_NONE, 50 },  // 754 planta 0.9m
  { "MORELULL", 756, 24, R_RARO, 0x3C49, 55, 35, 55, 65, 75, 15, 2, 88, 1, T_GRASS, T_FAIRY, 50 },  // 755 planta 0.2m
  { "SHIINOTIC", 0, 0, R_EVO, 0x3C49, 75, 55, 80, 90, 100, 30, 2, 133, 1, T_GRASS, T_FAIRY, 50 },  // 756 planta 1.0m
  { "SALANDIT", 758, 30, R_RARO, 0x8A73, 48, 44, 40, 71, 40, 77, 0, 108, 1, T_POISON, T_FIRE, 12 },  // 757 veneno 0.6m
  { "SALAZZLE", 0, 0, R_EVO, 0x8A73, 68, 64, 60, 111, 60, 117, 0, 129, 2, T_POISON, T_FIRE, 100 },  // 758 veneno 1.2m
  { "STUFFUL", 760, 30, R_COMUN, 0x8C4D, 70, 75, 50, 45, 50, 50, 0, 97, 1, T_NORMAL, T_FIGHT, 50 },  // 759 normal 0.5m
  { "BEWEAR", 0, 0, R_EVO, 0x8C4D, 120, 125, 80, 55, 60, 60, 0, 126, 14, T_NORMAL, T_FIGHT, 50 },  // 760 normal 2.1m
  { "BOUNSWEET", 762, 18, R_COMUN, 0x3C49, 42, 30, 38, 30, 38, 32, 2, 91, 1, T_GRASS, T_NONE, 100 },  // 761 planta 0.3m
  { "STEENEE", 763, 30, R_EVO, 0x3C49, 52, 60, 48, 60, 48, 62, 2, 106, 1, T_GRASS, T_NONE, 100 },  // 762 planta 0.7m
  { "TSAREENA", 0, 0, R_EVO, 0x3C49, 72, 120, 98, 50, 98, 72, 2, 120, 2, T_GRASS, T_NONE, 100 },  // 763 planta 1.2m
  { "COMFEY", 0, 0, R_RARO, 0xDC76, 51, 52, 90, 82, 110, 100, 0, 111, 1, T_FAIRY, T_NONE, 75 },  // 764 hada 0.1m
  { "ORANGURU", 0, 0, R_RARO, 0x8C4D, 90, 60, 80, 90, 110, 60, 0, 133, 8, T_NORMAL, T_PSYCHIC, 50 },  // 765 normal 1.5m
  { "PASSIMIAN", 0, 0, R_RARO, 0xA2A5, 100, 120, 90, 40, 60, 80, 0, 123, 8, T_FIGHT, T_NONE, 50 },  // 766 lucha 2.0m
  { "WIMPOD", 768, 30, R_RARO, 0x7CC4, 25, 35, 40, 20, 30, 80, 2, 98, 1, T_BUG, T_WATER, 50 },  // 767 bicho 0.5m
  { "GOLISOPOD", 0, 0, R_EVO, 0x7CC4, 75, 125, 110, 60, 90, 40, 2, 128, 11, T_BUG, T_WATER, 50 },  // 768 bicho 2.0m
  { "SANDYGAST", 770, 40, R_RARO, 0x6AD3, 55, 55, 80, 70, 45, 15, 0, 99, 7, T_GHOST, T_GROUND, 50 },  // 769 fantasma 0.5m
  { "PALOSSAND", 0, 0, R_EVO, 0x6AD3, 85, 75, 110, 100, 75, 35, 0, 123, 25, T_GHOST, T_GROUND, 50 },  // 770 fantasma 1.3m
  { "PYUKUMUKU", 0, 0, R_RARO, 0x4C98, 55, 60, 130, 30, 130, 5, 1, 91, 1, T_WATER, T_NONE, 50 },  // 771 agua 0.3m
  { "TYPE NULL", 773, 35, R_RARO, 0x8C4D, 95, 95, 95, 95, 95, 59, 0, 132, 12, T_NORMAL, T_NONE, 255 },  // 772 normal 1.9m
  { "SILVALLY", 0, 0, R_EVO, 0x8C4D, 95, 95, 95, 95, 95, 95, 0, 138, 10, T_NORMAL, T_NONE, 255 },  // 773 normal 2.3m
  { "MINIOR", 0, 0, R_RARO, 0x9407, 60, 60, 100, 60, 100, 60, 4, 96, 4, T_ROCK, T_FLYING, 255 },  // 774 roca 0.3m
  { "KOMALA", 0, 0, R_RARO, 0x8C4D, 75, 115, 65, 75, 95, 65, 0, 99, 2, T_NORMAL, T_NONE, 50 },  // 775 normal 0.4m
  { "TURTONATOR", 0, 0, R_RARO, 0xEA87, 60, 78, 135, 91, 85, 36, 3, 128, 21, T_FIRE, T_DRAGON, 50 },  // 776 fuego 2.0m
  { "TOGEDEMARU", 0, 0, R_RARO, 0xBCA1, 65, 98, 63, 40, 73, 96, 0, 97, 1, T_ELEC, T_STEEL, 50 },  // 777 electrico 0.3m
  { "MIMIKYU", 0, 0, R_RARO, 0x6AD3, 55, 105, 80, 50, 105, 96, 0, 96, 1, T_GHOST, T_FAIRY, 50 },  // 778 fantasma 0.2m
  { "BRUXISH", 0, 0, R_RARO, 0x4C98, 68, 110, 70, 70, 70, 100, 1, 97, 2, T_WATER, T_PSYCHIC, 50 },  // 779 agua 0.9m
  { "DRAMPA", 0, 0, R_RARO, 0x8C4D, 85, 60, 85, 135, 91, 36, 0, 139, 18, T_NORMAL, T_DRAGON, 50 },  // 780 normal 3.0m
  { "DHELMISE", 0, 0, R_RARO, 0x6AD3, 70, 131, 100, 86, 90, 40, 0, 133, 21, T_GHOST, T_GRASS, 255 },  // 781 fantasma 3.9m
  { "JANGMO-O", 783, 35, R_RARO, 0x5A98, 45, 55, 65, 45, 45, 45, 1, 101, 3, T_DRAGON, T_NONE, 50 },  // 782 dragon 0.6m
  { "HAKAMO-O", 784, 45, R_EVO, 0x5A98, 55, 75, 90, 65, 70, 65, 1, 116, 5, T_DRAGON, T_FIGHT, 50 },  // 783 dragon 1.2m
  { "KOMMO-O", 0, 0, R_EVO, 0x5A98, 75, 110, 125, 100, 105, 85, 1, 145, 8, T_DRAGON, T_FIGHT, 50 },  // 784 dragon 1.6m
  { "TAPU KOKO", 0, 0, R_LEGENDARIO, 0xBCA1, 70, 115, 85, 95, 75, 130, 0, 126, 2, T_ELEC, T_FAIRY, 255 },  // 785 electrico 1.8m
  { "TAPU LELE", 0, 0, R_LEGENDARIO, 0xD28F, 70, 85, 75, 130, 115, 95, 0, 125, 2, T_PSYCHIC, T_FAIRY, 255 },  // 786 psiquico 1.2m
  { "TAPU BULU", 0, 0, R_LEGENDARIO, 0x3C49, 70, 130, 115, 85, 95, 75, 2, 125, 5, T_GRASS, T_FAIRY, 255 },  // 787 planta 1.9m
  { "TAPU FINI", 0, 0, R_LEGENDARIO, 0x4C98, 70, 75, 115, 95, 130, 85, 1, 142, 2, T_WATER, T_FAIRY, 255 },  // 788 agua 1.3m
  { "COSMOG", 790, 43, R_LEGENDARIO, 0xD28F, 43, 29, 31, 29, 31, 37, 0, 100, 1, T_PSYCHIC, T_NONE, 255 },  // 789 psiquico 0.2m
  { "COSMOEM", 791, 0, R_EVO, 0xD28F, 43, 29, 131, 29, 131, 37, 0, 100, 100, T_PSYCHIC, T_NONE, 255 },  // 790 psiquico 0.1m
  { "SOLGALEO", 0, 0, R_EVO, 0xD28F, 137, 137, 107, 113, 89, 97, 0, 147, 23, T_PSYCHIC, T_STEEL, 255 },  // 791 psiquico 3.4m
  { "LUNALA", 0, 0, R_EVO, 0xD28F, 137, 113, 89, 137, 107, 97, 0, 154, 12, T_PSYCHIC, T_GHOST, 255 },  // 792 psiquico 4.0m
  { "NIHILEGO", 0, 0, R_LEGENDARIO, 0x9407, 109, 53, 47, 127, 131, 103, 4, 119, 6, T_ROCK, T_POISON, 255 },  // 793 roca 1.2m
  { "BUZZWOLE", 0, 0, R_LEGENDARIO, 0x7CC4, 107, 139, 139, 53, 53, 79, 2, 154, 33, T_BUG, T_FIGHT, 255 },  // 794 bicho 2.4m
  { "PHEROMOSA", 0, 0, R_LEGENDARIO, 0x7CC4, 71, 137, 37, 137, 37, 151, 2, 143, 2, T_BUG, T_FIGHT, 255 },  // 795 bicho 1.8m
  { "XURKITREE", 0, 0, R_LEGENDARIO, 0xBCA1, 83, 89, 71, 173, 71, 83, 0, 139, 10, T_ELEC, T_NONE, 255 },  // 796 electrico 3.8m
  { "CELESTEELA", 0, 0, R_LEGENDARIO, 0x7C53, 97, 101, 103, 107, 101, 61, 4, 155, 100, T_STEEL, T_FLYING, 255 },  // 797 acero 9.2m
  { "KARTANA", 0, 0, R_LEGENDARIO, 0x3C49, 59, 181, 131, 59, 31, 109, 2, 108, 1, T_GRASS, T_STEEL, 255 },  // 798 planta 0.3m
  { "GUZZLORD", 0, 0, R_LEGENDARIO, 0x49E6, 223, 101, 53, 97, 53, 43, 0, 160, 89, T_DARK, T_DRAGON, 255 },  // 799 siniestro 5.5m
  { "NECROZMA", 0, 0, R_LEGENDARIO, 0xD28F, 97, 107, 101, 127, 89, 79, 0, 144, 23, T_PSYCHIC, T_NONE, 255 },  // 800 psiquico 2.4m
  { "MAGEARNA", 0, 0, R_LEGENDARIO, 0x7C53, 80, 95, 115, 130, 115, 65, 4, 113, 8, T_STEEL, T_FAIRY, 255 },  // 801 acero 1.0m
  { "MARSHADOW", 0, 0, R_LEGENDARIO, 0xA2A5, 90, 125, 80, 90, 90, 125, 0, 96, 2, T_FIGHT, T_GHOST, 255 },  // 802 lucha 0.7m
  { "POIPOLE", 804, 50, R_LEGENDARIO, 0x8A73, 67, 73, 67, 73, 67, 73, 0, 99, 1, T_POISON, T_NONE, 255 },  // 803 veneno 0.6m
  { "NAGANADEL", 0, 0, R_EVO, 0x8A73, 73, 73, 73, 117, 73, 121, 0, 125, 15, T_POISON, T_DRAGON, 255 },  // 804 veneno 3.6m
  { "STAKATAKA", 0, 0, R_LEGENDARIO, 0x9407, 61, 121, 181, 53, 91, 13, 4, 152, 82, T_ROCK, T_STEEL, 255 },  // 805 roca 5.5m
  { "BLACEPHALON", 0, 0, R_LEGENDARIO, 0xEA87, 53, 97, 53, 141, 69, 107, 3, 116, 1, T_FIRE, T_GHOST, 255 },  // 806 fuego 1.8m
  { "ZERAORA", 0, 0, R_LEGENDARIO, 0xBCA1, 88, 112, 75, 102, 80, 143, 0, 125, 4, T_ELEC, T_NONE, 255 },  // 807 electrico 1.5m
  { "MELTAN", 809, 50, R_LEGENDARIO, 0x7C53, 46, 65, 65, 35, 34, 55, 4, 92, 1, T_STEEL, T_NONE, 255 },  // 808 acero 0.2m
  { "MELMETAL", 0, 0, R_EVO, 0x7C53, 135, 143, 143, 65, 34, 80, 4, 155, 80, T_STEEL, T_NONE, 255 },  // 809 acero 2.5m
  { "GROOKEY", 811, 16, R_COMUN, 0x3C49, 50, 65, 50, 40, 40, 65, 2, 96, 1, T_GRASS, T_NONE, 50 },  // 810 planta 0.3m
  { "THWACKEY", 812, 36, R_EVO, 0x3C49, 70, 95, 70, 55, 60, 80, 2, 110, 1, T_GRASS, T_NONE, 50 },  // 811 planta 0.7m
  { "RILLABOOM", 0, 0, R_EVO, 0x3C49, 100, 125, 90, 60, 70, 86, 2, 147, 9, T_GRASS, T_NONE, 50 },  // 812 planta 2.1m
  { "SCORBUNNY", 814, 16, R_COMUN, 0xEA87, 50, 71, 40, 40, 40, 69, 3, 97, 1, T_FIRE, T_NONE, 50 },  // 813 fuego 0.3m
  { "RABOOT", 815, 36, R_EVO, 0xEA87, 65, 86, 60, 55, 60, 94, 3, 106, 1, T_FIRE, T_NONE, 50 },  // 814 fuego 0.6m
  { "CINDERACE", 0, 0, R_EVO, 0xEA87, 80, 116, 75, 65, 75, 119, 3, 116, 3, T_FIRE, T_NONE, 50 },  // 815 fuego 1.4m
  { "SOBBLE", 817, 16, R_COMUN, 0x4C98, 50, 40, 40, 70, 40, 70, 1, 95, 1, T_WATER, T_NONE, 50 },  // 816 agua 0.3m
  { "DRIZZILE", 818, 36, R_EVO, 0x4C98, 65, 50, 55, 95, 50, 90, 1, 103, 1, T_WATER, T_DARK, 50 },  // 817 agua 0.7m
  { "INTELEON", 0, 0, R_EVO, 0x4C98, 70, 85, 65, 125, 65, 120, 1, 117, 5, T_WATER, T_DARK, 50 },  // 818 agua 1.9m
  { "SKWOVET", 820, 20, R_COMUN, 0x8C4D, 70, 55, 55, 35, 35, 25, 0, 103, 1, T_NORMAL, T_NONE, 50 },  // 819 normal 0.3m
  { "GREEDENT", 0, 0, R_EVO, 0x8C4D, 120, 95, 95, 55, 75, 20, 0, 137, 1, T_NORMAL, T_NONE, 50 },  // 820 normal 0.6m
  { "ROOKIDEE", 822, 18, R_COMUN, 0x8C4D, 38, 47, 35, 33, 35, 57, 0, 96, 1, T_FLYING, T_NONE, 50 },  // 821 normal 0.2m
  { "CORVISQUIRE", 823, 38, R_EVO, 0x8C4D, 68, 67, 55, 43, 55, 77, 0, 126, 2, T_FLYING, T_NONE, 50 },  // 822 normal 0.8m
  { "CORVIKNIGHT", 0, 0, R_EVO, 0x7C53, 98, 87, 105, 53, 85, 67, 4, 121, 8, T_FLYING, T_STEEL, 50 },  // 823 acero 2.2m
  { "BLIPBUG", 825, 10, R_COMUN, 0x7CC4, 25, 20, 20, 25, 45, 45, 2, 88, 1, T_BUG, T_NONE, 50 },  // 824 bicho 0.4m
  { "DOTTLER", 826, 30, R_EVO, 0x7CC4, 50, 35, 80, 50, 90, 30, 2, 109, 2, T_BUG, T_PSYCHIC, 50 },  // 825 bicho 0.4m
  { "ORBEETLE", 0, 0, R_EVO, 0x7CC4, 60, 45, 110, 80, 120, 90, 2, 126, 4, T_BUG, T_PSYCHIC, 50 },  // 826 bicho 0.4m
  { "NICKIT", 828, 18, R_COMUN, 0x49E6, 40, 28, 28, 48, 52, 50, 0, 114, 1, T_DARK, T_NONE, 50 },  // 827 siniestro 0.6m
  { "THIEVUL", 0, 0, R_EVO, 0x49E6, 70, 58, 58, 87, 92, 90, 0, 130, 2, T_DARK, T_NONE, 50 },  // 828 siniestro 1.2m
  { "GOSSIFLEUR", 830, 20, R_COMUN, 0x3C49, 40, 40, 60, 40, 60, 10, 2, 105, 1, T_GRASS, T_NONE, 50 },  // 829 planta 0.4m
  { "ELDEGOSS", 0, 0, R_EVO, 0x3C49, 60, 50, 90, 90, 120, 60, 2, 118, 1, T_GRASS, T_NONE, 50 },  // 830 planta 0.5m
  { "WOOLOO", 832, 24, R_COMUN, 0x8C4D, 42, 40, 55, 40, 45, 48, 0, 103, 1, T_NORMAL, T_NONE, 50 },  // 831 normal 0.6m
  { "DUBWOOL", 0, 0, R_EVO, 0x8C4D, 72, 80, 100, 60, 90, 88, 0, 135, 4, T_NORMAL, T_NONE, 50 },  // 832 normal 1.3m
  { "CHEWTLE", 834, 24, R_COMUN, 0x4C98, 50, 64, 50, 38, 38, 44, 1, 94, 1, T_WATER, T_NONE, 50 },  // 833 agua 0.3m
  { "DREDNAW", 0, 0, R_EVO, 0x4C98, 90, 115, 90, 48, 68, 74, 1, 124, 12, T_WATER, T_ROCK, 50 },  // 834 agua 1.0m
  { "YAMPER", 836, 25, R_COMUN, 0xBCA1, 59, 45, 50, 40, 50, 26, 0, 101, 1, T_ELEC, T_NONE, 50 },  // 835 electrico 0.3m
  { "BOLTUND", 0, 0, R_EVO, 0xBCA1, 69, 90, 60, 90, 60, 121, 0, 122, 3, T_ELEC, T_NONE, 50 },  // 836 electrico 1.0m
  { "ROLYCOLY", 838, 18, R_COMUN, 0x9407, 30, 40, 50, 40, 50, 30, 4, 103, 1, T_ROCK, T_NONE, 50 },  // 837 roca 0.3m
  { "CARKOL", 839, 40, R_EVO, 0x9407, 80, 60, 90, 60, 70, 50, 4, 118, 8, T_ROCK, T_FIRE, 50 },  // 838 roca 1.1m
  { "COALOSSAL", 0, 0, R_EVO, 0x9407, 110, 80, 120, 80, 90, 30, 4, 141, 31, T_ROCK, T_FIRE, 50 },  // 839 roca 2.8m
  { "APPLIN", 1011, 28, R_COMUN, 0x3C49, 40, 40, 80, 40, 40, 20, 2, 92, 1, T_GRASS, T_DRAGON, 50 },  // 840 planta 0.2m
  { "FLAPPLE", 0, 0, R_EVO, 0x3C49, 70, 110, 80, 95, 60, 70, 2, 117, 1, T_GRASS, T_DRAGON, 50 },  // 841 planta 0.3m
  { "APPLETUN", 0, 0, R_EVO, 0x3C49, 110, 85, 80, 100, 80, 30, 2, 135, 1, T_GRASS, T_DRAGON, 50 },  // 842 planta 0.4m
  { "SILICOBRA", 844, 31, R_COMUN, 0xB447, 52, 57, 75, 35, 50, 46, 4, 100, 1, T_GROUND, T_NONE, 50 },  // 843 tierra 2.2m
  { "SANDACONDA", 0, 0, R_EVO, 0xB447, 72, 107, 125, 65, 70, 71, 4, 131, 7, T_GROUND, T_NONE, 50 },  // 844 tierra 3.8m
  { "CRAMORANT", 0, 0, R_RARO, 0x4C98, 70, 85, 55, 95, 85, 85, 1, 119, 2, T_FLYING, T_WATER, 50 },  // 845 agua 0.8m
  { "ARROKUDA", 847, 26, R_COMUN, 0x4C98, 41, 63, 40, 40, 30, 66, 1, 95, 1, T_WATER, T_NONE, 50 },  // 846 agua 0.5m
  { "BARRASKEWDA", 0, 0, R_EVO, 0x4C98, 61, 123, 60, 60, 50, 136, 1, 112, 3, T_WATER, T_NONE, 50 },  // 847 agua 1.3m
  { "TOXEL", 849, 30, R_RARO, 0xBCA1, 40, 38, 35, 35, 40, 40, 0, 104, 1, T_ELEC, T_POISON, 50 },  // 848 electrico 0.4m
  { "TOXTRICITY", 0, 0, R_EVO, 0xBCA1, 75, 98, 70, 114, 70, 75, 0, 125, 4, T_ELEC, T_POISON, 50 },  // 849 electrico 1.6m
  { "SIZZLIPEDE", 851, 28, R_COMUN, 0xEA87, 50, 65, 45, 50, 50, 45, 3, 103, 1, T_FIRE, T_BUG, 50 },  // 850 fuego 0.7m
  { "CENTISKORCH", 0, 0, R_EVO, 0xEA87, 100, 115, 65, 90, 90, 65, 3, 132, 12, T_FIRE, T_BUG, 50 },  // 851 fuego 3.0m
  { "CLOBBOPUS", 853, 30, R_RARO, 0xA2A5, 50, 68, 60, 50, 50, 32, 0, 97, 1, T_FIGHT, T_NONE, 50 },  // 852 lucha 0.6m
  { "GRAPPLOCT", 0, 0, R_EVO, 0xA2A5, 80, 118, 90, 70, 80, 42, 0, 124, 4, T_FIGHT, T_NONE, 50 },  // 853 lucha 1.6m
  { "SINISTEA", 855, 0, R_RARO, 0x6AD3, 40, 45, 45, 74, 50, 50, 0, 92, 1, T_GHOST, T_NONE, 255 },  // 854 fantasma 0.1m
  { "POLTEAGEIST", 0, 0, R_EVO, 0x6AD3, 60, 65, 65, 134, 114, 70, 0, 120, 1, T_GHOST, T_NONE, 255 },  // 855 fantasma 0.2m
  { "HATENNA", 857, 32, R_RARO, 0xD28F, 42, 30, 45, 56, 53, 39, 0, 89, 1, T_PSYCHIC, T_NONE, 100 },  // 856 psiquico 0.4m
  { "HATTREM", 858, 42, R_EVO, 0xD28F, 57, 40, 65, 86, 73, 49, 0, 113, 1, T_PSYCHIC, T_NONE, 100 },  // 857 psiquico 0.6m
  { "HATTERENE", 0, 0, R_EVO, 0xD28F, 57, 90, 95, 136, 103, 29, 0, 129, 1, T_PSYCHIC, T_FAIRY, 100 },  // 858 psiquico 2.1m
  { "IMPIDIMP", 860, 32, R_RARO, 0x49E6, 45, 45, 30, 40, 50, 50, 0, 108, 1, T_DARK, T_FAIRY, 0 },  // 859 siniestro 0.4m
  { "MORGREM", 861, 42, R_EVO, 0x49E6, 65, 60, 45, 55, 70, 70, 0, 119, 1, T_DARK, T_FAIRY, 0 },  // 860 siniestro 0.8m
  { "GRIMMSNARL", 0, 0, R_EVO, 0x49E6, 95, 120, 65, 95, 75, 60, 0, 140, 6, T_DARK, T_FAIRY, 0 },  // 861 siniestro 1.5m
  { "OBSTAGOON", 0, 0, R_RARO, 0x49E6, 93, 95, 101, 60, 81, 95, 0, 131, 5, T_DARK, T_NORMAL, 50 },  // 862 siniestro 1.6m
  { "PERRSERKER", 0, 0, R_RARO, 0x7C53, 70, 110, 100, 50, 60, 50, 4, 119, 3, T_STEEL, T_NONE, 50 },  // 863 acero 0.8m
  { "CURSOLA", 0, 0, R_RARO, 0x6AD3, 60, 95, 50, 145, 130, 30, 0, 135, 1, T_GHOST, T_NONE, 75 },  // 864 fantasma 1.0m
  { "SIRFETCHD", 0, 0, R_RARO, 0xA2A5, 62, 135, 95, 68, 82, 65, 0, 125, 12, T_FIGHT, T_NONE, 50 },  // 865 lucha 0.8m
  { "MR. RIME", 0, 0, R_EVO, 0x4DB8, 80, 85, 75, 110, 100, 70, 5, 118, 6, T_ICE, T_PSYCHIC, 50 },  // 866 hielo 1.5m
  { "RUNERIGUS", 0, 0, R_RARO, 0x6AD3, 58, 95, 145, 50, 105, 30, 0, 131, 7, T_GHOST, T_GROUND, 50 },  // 867 fantasma 1.6m
  { "MILCERY", 869, 0, R_RARO, 0xDC76, 45, 40, 40, 50, 61, 34, 0, 89, 1, T_FAIRY, T_NONE, 100 },  // 868 hada 0.2m
  { "ALCREMIE", 0, 0, R_EVO, 0xDC76, 65, 60, 75, 110, 121, 64, 0, 109, 1, T_FAIRY, T_NONE, 100 },  // 869 hada 0.3m
  { "FALINKS", 0, 0, R_RARO, 0xA2A5, 65, 100, 100, 70, 60, 75, 0, 125, 6, T_FIGHT, T_NONE, 255 },  // 870 lucha 3.0m
  { "PINCURCHIN", 0, 0, R_RARO, 0xBCA1, 48, 101, 95, 91, 85, 15, 0, 85, 1, T_ELEC, T_NONE, 50 },  // 871 electrico 0.3m
  { "SNOM", 873, 25, R_RARO, 0x4DB8, 30, 25, 35, 45, 30, 20, 5, 85, 1, T_ICE, T_BUG, 50 },  // 872 hielo 0.3m
  { "FROSMOTH", 0, 0, R_EVO, 0x4DB8, 70, 65, 60, 125, 90, 65, 5, 131, 4, T_ICE, T_BUG, 50 },  // 873 hielo 1.3m
  { "STONJOURNER", 0, 0, R_RARO, 0x9407, 100, 125, 135, 20, 20, 70, 4, 136, 52, T_ROCK, T_NONE, 50 },  // 874 roca 2.5m
  { "EISCUE", 0, 0, R_RARO, 0x4DB8, 75, 80, 110, 65, 90, 50, 5, 128, 9, T_ICE, T_NONE, 50 },  // 875 hielo 1.4m
  { "INDEEDEE", 0, 0, R_RARO, 0xD28F, 60, 65, 55, 105, 95, 95, 0, 102, 3, T_PSYCHIC, T_NORMAL, 50 },  // 876 psiquico 0.9m
  { "MORPEKO", 0, 0, R_RARO, 0xBCA1, 58, 95, 58, 70, 58, 97, 0, 94, 1, T_ELEC, T_DARK, 50 },  // 877 electrico 0.3m
  { "CUFANT", 879, 34, R_RARO, 0x7C53, 72, 80, 49, 40, 49, 40, 4, 119, 10, T_STEEL, T_NONE, 50 },  // 878 acero 1.2m
  { "COPPERAJAH", 0, 0, R_EVO, 0x7C53, 122, 130, 69, 80, 69, 30, 4, 145, 65, T_STEEL, T_NONE, 50 },  // 879 acero 3.0m
  { "DRACOZOLT", 0, 0, R_RARO, 0xBCA1, 90, 115, 90, 80, 70, 75, 0, 138, 19, T_ELEC, T_DRAGON, 255 },  // 880 electrico 1.8m
  { "ARCTOZOLT", 0, 0, R_RARO, 0xBCA1, 90, 115, 90, 90, 80, 55, 0, 135, 15, T_ELEC, T_ICE, 255 },  // 881 electrico 2.3m
  { "DRACOVISH", 0, 0, R_RARO, 0x4C98, 90, 110, 100, 70, 80, 75, 1, 133, 22, T_WATER, T_DRAGON, 255 },  // 882 agua 2.3m
  { "ARCTOVISH", 0, 0, R_RARO, 0x4C98, 90, 115, 90, 90, 80, 55, 1, 138, 18, T_WATER, T_ICE, 255 },  // 883 agua 2.0m
  { "DURALUDON", 1018, 55, R_RARO, 0x7C53, 70, 95, 115, 120, 50, 85, 4, 126, 4, T_STEEL, T_DRAGON, 50 },  // 884 acero 1.8m
  { "DREEPY", 886, 50, R_RARO, 0x5A98, 28, 60, 30, 40, 30, 82, 1, 99, 1, T_DRAGON, T_GHOST, 50 },  // 885 dragon 0.5m
  { "DRAKLOAK", 887, 60, R_EVO, 0x5A98, 68, 80, 50, 60, 50, 102, 1, 120, 1, T_DRAGON, T_GHOST, 50 },  // 886 dragon 1.4m
  { "DRAGAPULT", 0, 0, R_EVO, 0x5A98, 88, 120, 75, 100, 75, 142, 1, 134, 5, T_DRAGON, T_GHOST, 50 },  // 887 dragon 3.0m
  { "ZACIAN", 0, 0, R_LEGENDARIO, 0xDC76, 92, 120, 115, 80, 115, 138, 0, 139, 11, T_FAIRY, T_NONE, 255 },  // 888 hada 2.8m
  { "ZAMAZENTA", 0, 0, R_LEGENDARIO, 0xA2A5, 92, 120, 115, 80, 115, 138, 0, 142, 21, T_FIGHT, T_NONE, 255 },  // 889 lucha 2.9m
  { "ETERNATUS", 0, 0, R_LEGENDARIO, 0x8A73, 140, 85, 95, 145, 95, 130, 0, 144, 95, T_POISON, T_DRAGON, 255 },  // 890 veneno 20.0m
  { "KUBFU", 892, 0, R_LEGENDARIO, 0xA2A5, 60, 90, 60, 53, 50, 72, 0, 104, 1, T_FIGHT, T_NONE, 12 },  // 891 lucha 0.6m
  { "URSHIFU", 0, 0, R_EVO, 0xA2A5, 100, 130, 100, 63, 60, 97, 0, 134, 10, T_FIGHT, T_DARK, 12 },  // 892 lucha 1.9m
  { "ZARUDE", 0, 0, R_LEGENDARIO, 0x49E6, 105, 120, 105, 70, 95, 105, 0, 136, 7, T_DARK, T_GRASS, 255 },  // 893 siniestro 1.8m
  { "REGIELEKI", 0, 0, R_LEGENDARIO, 0xBCA1, 80, 100, 50, 100, 50, 200, 0, 135, 14, T_ELEC, T_NONE, 255 },  // 894 electrico 1.2m
  { "REGIDRAGO", 0, 0, R_LEGENDARIO, 0x5A98, 200, 100, 50, 100, 50, 80, 1, 136, 20, T_DRAGON, T_NONE, 255 },  // 895 dragon 2.1m
  { "GLASTRIER", 0, 0, R_LEGENDARIO, 0x4DB8, 100, 145, 130, 65, 110, 30, 5, 127, 80, T_ICE, T_NONE, 255 },  // 896 hielo 2.2m
  { "SPECTRIER", 0, 0, R_LEGENDARIO, 0x6AD3, 100, 65, 60, 145, 80, 130, 0, 128, 4, T_GHOST, T_NONE, 255 },  // 897 fantasma 2.0m
  { "CALYREX", 0, 0, R_LEGENDARIO, 0xD28F, 100, 80, 80, 80, 80, 80, 0, 127, 1, T_PSYCHIC, T_GRASS, 255 },  // 898 psiquico 1.1m
  { "WYRDEER", 0, 0, R_EVO, 0x8C4D, 103, 105, 72, 105, 75, 65, 0, 133, 10, T_NORMAL, T_PSYCHIC, 50 },  // 899 normal 1.8m
  { "KLEAVOR", 0, 0, R_EVO, 0x7CC4, 70, 135, 95, 45, 70, 85, 2, 140, 9, T_BUG, T_ROCK, 50 },  // 900 bicho 1.8m
  { "URSALUNA", 0, 0, R_EVO, 0x8C4D, 130, 140, 105, 45, 80, 50, 0, 136, 29, T_NORMAL, T_GROUND, 50 },  // 901 normal 2.4m
  { "BASCULEGION", 0, 0, R_EVO, 0x4C98, 120, 112, 65, 80, 75, 78, 1, 124, 11, T_WATER, T_GHOST, 50 },  // 902 agua 3.0m
  { "SNEASLER", 0, 0, R_COMUN, 0xA2A5, 80, 130, 60, 40, 80, 120, 0, 116, 4, T_FIGHT, T_POISON, 50 },  // 903 lucha 1.3m
  { "OVERQWIL", 0, 0, R_COMUN, 0x49E6, 85, 115, 95, 65, 65, 85, 0, 142, 6, T_DARK, T_POISON, 50 },  // 904 siniestro 2.5m
  { "ENAMORUS", 0, 0, R_LEGENDARIO, 0xDC76, 74, 115, 70, 135, 80, 106, 0, 128, 5, T_FAIRY, T_FLYING, 100 },  // 905 hada 1.8m
  { "SPRIGATITO", 907, 16, R_COMUN, 0x3C49, 40, 61, 54, 45, 45, 65, 2, 98, 1, T_GRASS, T_NONE, 50 },  // 906 planta 0.4m
  { "FLORAGATO", 908, 36, R_EVO, 0x3C49, 61, 80, 63, 60, 63, 83, 2, 110, 1, T_GRASS, T_NONE, 50 },  // 907 planta 0.9m
  { "MEOWSCARADA", 0, 0, R_EVO, 0x3C49, 76, 110, 70, 81, 70, 123, 2, 124, 3, T_GRASS, T_DARK, 50 },  // 908 planta 1.5m
  { "FUECOCO", 910, 16, R_COMUN, 0xEA87, 67, 45, 59, 63, 40, 36, 3, 104, 1, T_FIRE, T_NONE, 50 },  // 909 fuego 0.4m
  { "CROCALOR", 911, 36, R_EVO, 0xEA87, 81, 55, 78, 90, 58, 49, 3, 116, 3, T_FIRE, T_NONE, 50 },  // 910 fuego 1.0m
  { "SKELEDIRGE", 0, 0, R_EVO, 0xEA87, 104, 75, 100, 110, 75, 66, 3, 128, 33, T_FIRE, T_GHOST, 50 },  // 911 fuego 1.6m
  { "QUAXLY", 913, 16, R_COMUN, 0x4C98, 55, 65, 45, 50, 45, 50, 1, 97, 1, T_WATER, T_NONE, 50 },  // 912 agua 0.5m
  { "QUAXWELL", 914, 36, R_EVO, 0x4C98, 70, 85, 65, 65, 60, 65, 1, 104, 2, T_WATER, T_NONE, 50 },  // 913 agua 1.2m
  { "QUAQUAVAL", 0, 0, R_EVO, 0x4C98, 85, 120, 80, 85, 75, 85, 1, 122, 6, T_WATER, T_FIGHT, 50 },  // 914 agua 1.8m
  { "LECHONK", 916, 18, R_COMUN, 0x8C4D, 54, 45, 40, 35, 45, 35, 0, 94, 1, T_NORMAL, T_NONE, 50 },  // 915 normal 0.5m
  { "OINKOLOGNE", 0, 0, R_EVO, 0x8C4D, 110, 100, 75, 59, 80, 65, 0, 115, 12, T_NORMAL, T_NONE, 50 },  // 916 normal 1.0m
  { "TAROUNTULA", 918, 20, R_COMUN, 0x7CC4, 35, 41, 45, 29, 40, 20, 2, 105, 1, T_BUG, T_NONE, 50 },  // 917 bicho 0.3m
  { "SPIDOPS", 0, 0, R_EVO, 0x7CC4, 70, 100, 92, 52, 86, 35, 2, 133, 2, T_BUG, T_NONE, 50 },  // 918 bicho 1.0m
  { "NYMBLE", 920, 24, R_COMUN, 0x7CC4, 33, 46, 40, 21, 25, 45, 2, 100, 1, T_BUG, T_NONE, 50 },  // 919 bicho 0.2m
  { "LOKIX", 0, 0, R_EVO, 0x7CC4, 71, 112, 78, 52, 55, 92, 2, 125, 2, T_BUG, T_DARK, 50 },  // 920 bicho 1.0m
  { "PAWMI", 922, 18, R_COMUN, 0xBCA1, 45, 50, 20, 40, 25, 60, 0, 102, 1, T_ELEC, T_NONE, 50 },  // 921 electrico 0.3m
  { "PAWMO", 923, 25, R_EVO, 0xBCA1, 60, 75, 40, 50, 40, 85, 0, 101, 1, T_ELEC, T_FIGHT, 50 },  // 922 electrico 0.4m
  { "PAWMOT", 0, 0, R_EVO, 0xBCA1, 70, 115, 70, 70, 60, 105, 0, 119, 4, T_ELEC, T_FIGHT, 50 },  // 923 electrico 0.9m
  { "TANDEMAUS", 925, 25, R_COMUN, 0x8C4D, 50, 50, 45, 40, 45, 75, 0, 117, 1, T_NORMAL, T_NONE, 255 },  // 924 normal 0.3m
  { "MAUSHOLD", 0, 0, R_EVO, 0x8C4D, 74, 85, 70, 65, 75, 111, 0, 121, 1, T_NORMAL, T_NONE, 255 },  // 925 normal 0.3m
  { "FIDOUGH", 927, 26, R_COMUN, 0xDC76, 37, 55, 70, 30, 55, 65, 0, 93, 1, T_FAIRY, T_NONE, 50 },  // 926 hada 0.3m
  { "DACHSBUN", 0, 0, R_EVO, 0xDC76, 57, 80, 115, 50, 80, 95, 0, 120, 1, T_FAIRY, T_NONE, 50 },  // 927 hada 0.5m
  { "SMOLIV", 929, 25, R_COMUN, 0x3C49, 41, 35, 45, 58, 51, 30, 2, 84, 1, T_GRASS, T_NORMAL, 50 },  // 928 planta 0.3m
  { "DOLLIV", 930, 35, R_EVO, 0x3C49, 52, 53, 60, 78, 78, 33, 2, 108, 1, T_GRASS, T_NORMAL, 50 },  // 929 planta 0.6m
  { "ARBOLIVA", 0, 0, R_EVO, 0x3C49, 78, 69, 90, 125, 109, 39, 2, 136, 5, T_GRASS, T_NORMAL, 50 },  // 930 planta 1.4m
  { "SQUAWKABILLY", 0, 0, R_COMUN, 0x8C4D, 82, 106, 51, 45, 61, 102, 0, 108, 1, T_NORMAL, T_FLYING, 50 },  // 931 normal 0.6m
  { "NACLI", 933, 24, R_COMUN, 0x9407, 55, 55, 75, 35, 35, 25, 4, 87, 2, T_ROCK, T_NONE, 50 },  // 932 roca 0.4m
  { "NACLSTACK", 934, 38, R_EVO, 0x9407, 60, 60, 100, 35, 65, 35, 4, 123, 10, T_ROCK, T_NONE, 50 },  // 933 roca 0.6m
  { "GARGANACL", 0, 0, R_EVO, 0x9407, 100, 100, 130, 45, 90, 35, 4, 156, 24, T_ROCK, T_NONE, 50 },  // 934 roca 2.3m
  { "CHARCADET", 936, 0, R_COMUN, 0xEA87, 40, 50, 40, 50, 40, 35, 3, 98, 1, T_FIRE, T_NONE, 50 },  // 935 fuego 0.6m
  { "ARMAROUGE", 0, 0, R_EVO, 0xEA87, 85, 60, 100, 125, 80, 75, 3, 138, 8, T_FIRE, T_PSYCHIC, 50 },  // 936 fuego 1.5m
  { "CERULEDGE", 0, 0, R_EVO, 0xEA87, 75, 125, 80, 60, 100, 85, 3, 142, 6, T_FIRE, T_GHOST, 50 },  // 937 fuego 1.6m
  { "TADBULB", 939, 0, R_COMUN, 0xBCA1, 61, 31, 41, 59, 35, 45, 0, 80, 1, T_ELEC, T_NONE, 50 },  // 938 electrico 0.3m
  { "BELLIBOLT", 0, 0, R_EVO, 0xBCA1, 109, 64, 91, 103, 83, 45, 0, 130, 11, T_ELEC, T_NONE, 50 },  // 939 electrico 1.2m
  { "WATTREL", 941, 25, R_COMUN, 0xBCA1, 40, 40, 35, 55, 40, 70, 0, 93, 1, T_ELEC, T_FLYING, 50 },  // 940 electrico 0.4m
  { "KILOWATTREL", 0, 0, R_EVO, 0xBCA1, 70, 70, 60, 105, 60, 125, 0, 125, 4, T_ELEC, T_FLYING, 50 },  // 941 electrico 1.4m
  { "MASCHIFF", 943, 30, R_COMUN, 0x49E6, 60, 78, 60, 40, 51, 51, 0, 107, 2, T_DARK, T_NONE, 50 },  // 942 siniestro 0.5m
  { "MABOSSTIFF", 0, 0, R_EVO, 0x49E6, 80, 120, 90, 60, 70, 85, 0, 130, 6, T_DARK, T_NONE, 50 },  // 943 siniestro 1.1m
  { "SHROODLE", 945, 28, R_COMUN, 0x8A73, 40, 65, 35, 40, 35, 75, 0, 82, 1, T_POISON, T_NORMAL, 50 },  // 944 veneno 0.2m
  { "GRAFAIAI", 0, 0, R_EVO, 0x8A73, 63, 105, 65, 80, 72, 110, 0, 120, 3, T_POISON, T_NORMAL, 50 },  // 945 veneno 0.7m
  { "BRAMBLIN", 947, 30, R_COMUN, 0x3C49, 40, 65, 30, 45, 35, 60, 2, 97, 1, T_GRASS, T_GHOST, 50 },  // 946 planta 0.6m
  { "BRAMBLEGHAST", 0, 0, R_EVO, 0x3C49, 55, 115, 80, 80, 70, 100, 2, 111, 1, T_GRASS, T_GHOST, 50 },  // 947 planta 1.2m
  { "TOEDSCOOL", 949, 30, R_COMUN, 0xB447, 40, 40, 35, 50, 100, 70, 4, 101, 3, T_GROUND, T_GRASS, 50 },  // 948 tierra 0.9m
  { "TOEDSCRUEL", 0, 0, R_EVO, 0xB447, 80, 70, 65, 80, 120, 100, 4, 131, 6, T_GROUND, T_GRASS, 50 },  // 949 tierra 1.9m
  { "KLAWF", 0, 0, R_RARO, 0x9407, 80, 100, 115, 35, 55, 75, 4, 111, 8, T_ROCK, T_NONE, 50 },  // 950 roca 1.3m
  { "CAPSAKID", 952, 0, R_COMUN, 0x3C49, 50, 62, 40, 62, 40, 50, 2, 92, 1, T_GRASS, T_NONE, 50 },  // 951 planta 0.3m
  { "SCOVILLAIN", 0, 0, R_EVO, 0x3C49, 65, 108, 65, 108, 65, 95, 2, 120, 2, T_GRASS, T_FIRE, 50 },  // 952 planta 0.9m
  { "RELLOR", 954, 30, R_COMUN, 0x7CC4, 51, 50, 60, 31, 58, 40, 2, 105, 1, T_BUG, T_NONE, 50 },  // 953 bicho 0.2m
  { "RABSCA", 0, 0, R_EVO, 0x7CC4, 75, 50, 85, 115, 100, 45, 2, 140, 1, T_BUG, T_PSYCHIC, 50 },  // 954 bicho 0.3m
  { "FLITTLE", 956, 35, R_COMUN, 0xD28F, 30, 35, 30, 55, 30, 75, 0, 101, 1, T_PSYCHIC, T_NONE, 50 },  // 955 psiquico 0.2m
  { "ESPATHRA", 0, 0, R_EVO, 0xD28F, 95, 60, 60, 101, 70, 105, 0, 121, 9, T_PSYCHIC, T_NONE, 50 },  // 956 psiquico 1.9m
  { "TINKATINK", 958, 24, R_COMUN, 0xDC76, 50, 45, 45, 35, 64, 58, 0, 90, 1, T_FAIRY, T_STEEL, 100 },  // 957 hada 0.4m
  { "TINKATUFF", 959, 38, R_EVO, 0xDC76, 65, 55, 55, 45, 82, 78, 0, 113, 6, T_FAIRY, T_STEEL, 100 },  // 958 hada 0.7m
  { "TINKATON", 0, 0, R_EVO, 0xDC76, 85, 90, 77, 70, 105, 94, 0, 152, 11, T_FAIRY, T_STEEL, 100 },  // 959 hada 0.7m
  { "WIGLETT", 961, 26, R_COMUN, 0x4C98, 10, 55, 25, 35, 25, 95, 1, 100, 1, T_WATER, T_NONE, 50 },  // 960 agua 1.2m
  { "WUGTRIO", 0, 0, R_EVO, 0x4C98, 35, 100, 50, 50, 70, 120, 1, 122, 1, T_WATER, T_NONE, 50 },  // 961 agua 1.2m
  { "BOMBIRDIER", 0, 0, R_RARO, 0x49E6, 70, 113, 95, 60, 85, 82, 0, 129, 4, T_FLYING, T_DARK, 50 },  // 962 siniestro 1.5m
  { "FINIZEN", 964, 38, R_COMUN, 0x4C98, 70, 45, 40, 45, 40, 75, 1, 102, 6, T_WATER, T_NONE, 50 },  // 963 agua 1.3m
  { "PALAFIN", 0, 0, R_EVO, 0x4C98, 100, 70, 72, 53, 62, 100, 1, 106, 6, T_WATER, T_NONE, 50 },  // 964 agua 1.3m
  { "VAROOM", 966, 40, R_COMUN, 0x7C53, 45, 70, 63, 30, 45, 47, 4, 113, 4, T_STEEL, T_POISON, 50 },  // 965 acero 1.0m
  { "REVAVROOM", 0, 0, R_EVO, 0x7C53, 80, 119, 90, 54, 67, 100, 4, 137, 12, T_STEEL, T_POISON, 50 },  // 966 acero 1.8m
  { "CYCLIZAR", 0, 0, R_RARO, 0x5A98, 70, 95, 65, 85, 65, 121, 1, 120, 6, T_DRAGON, T_NORMAL, 50 },  // 967 dragon 1.6m
  { "ORTHWORM", 0, 0, R_RARO, 0x7C53, 70, 85, 145, 60, 55, 65, 4, 110, 31, T_STEEL, T_NONE, 50 },  // 968 acero 2.5m
  { "GLIMMET", 970, 35, R_COMUN, 0x9407, 48, 35, 42, 105, 60, 60, 4, 86, 1, T_ROCK, T_POISON, 50 },  // 969 roca 0.7m
  { "GLIMMORA", 0, 0, R_EVO, 0x9407, 83, 55, 90, 130, 81, 86, 4, 126, 4, T_ROCK, T_POISON, 50 },  // 970 roca 1.5m
  { "GREAVARD", 972, 30, R_COMUN, 0x6AD3, 50, 61, 60, 30, 55, 34, 0, 127, 4, T_GHOST, T_NONE, 50 },  // 971 fantasma 0.6m
  { "HOUNDSTONE", 0, 0, R_EVO, 0x6AD3, 72, 111, 100, 50, 97, 68, 0, 125, 2, T_GHOST, T_GROUND, 50 },  // 972 fantasma 2.0m
  { "FLAMIGO", 0, 0, R_RARO, 0xA2A5, 82, 115, 74, 75, 74, 90, 0, 99, 4, T_FLYING, T_FIGHT, 50 },  // 973 lucha 1.6m
  { "CETODDLE", 975, 34, R_COMUN, 0x4DB8, 108, 68, 45, 30, 40, 43, 5, 106, 4, T_ICE, T_NONE, 50 },  // 974 hielo 1.2m
  { "CETITAN", 0, 0, R_EVO, 0x4DB8, 170, 113, 65, 45, 55, 73, 5, 127, 70, T_ICE, T_NONE, 50 },  // 975 hielo 4.5m
  { "VELUZA", 0, 0, R_RARO, 0x4C98, 90, 112, 73, 78, 65, 100, 1, 98, 9, T_WATER, T_PSYCHIC, 50 },  // 976 agua 2.5m
  { "DONDOZO", 0, 0, R_RARO, 0x4C98, 100, 100, 115, 65, 65, 35, 1, 134, 22, T_WATER, T_NONE, 50 },  // 977 agua 12.0m
  { "TATSUGIRI", 0, 0, R_RARO, 0x5A98, 68, 50, 60, 120, 95, 92, 1, 84, 1, T_DRAGON, T_WATER, 50 },  // 978 dragon 0.3m
  { "ANNIHILAPE", 0, 0, R_EVO, 0xA2A5, 110, 115, 80, 50, 90, 90, 0, 129, 6, T_FIGHT, T_GHOST, 50 },  // 979 lucha 1.2m
  { "CLODSIRE", 0, 0, R_COMUN, 0x8A73, 130, 75, 60, 45, 100, 20, 0, 118, 22, T_POISON, T_GROUND, 50 },  // 980 veneno 1.8m
  { "FARIGIRAF", 0, 0, R_EVO, 0x8C4D, 120, 90, 70, 110, 70, 60, 0, 123, 16, T_NORMAL, T_PSYCHIC, 50 },  // 981 normal 3.2m
  { "DUDUNSPARCE", 0, 0, R_EVO, 0x8C4D, 125, 100, 80, 85, 75, 55, 0, 103, 4, T_NORMAL, T_NONE, 50 },  // 982 normal 3.6m
  { "KINGAMBIT", 0, 0, R_EVO, 0x49E6, 100, 135, 120, 60, 85, 50, 0, 132, 12, T_DARK, T_STEEL, 50 },  // 983 siniestro 2.0m
  { "GREAT TUSK", 0, 0, R_RARO, 0xB447, 115, 131, 131, 53, 53, 87, 4, 135, 32, T_GROUND, T_FIGHT, 255 },  // 984 tierra 2.2m
  { "SCREAM TAIL", 0, 0, R_RARO, 0xDC76, 115, 65, 99, 65, 115, 111, 0, 103, 1, T_FAIRY, T_PSYCHIC, 255 },  // 985 hada 1.2m
  { "BRUTE BONNET", 0, 0, R_RARO, 0x3C49, 111, 127, 99, 79, 99, 55, 2, 128, 2, T_GRASS, T_DARK, 255 },  // 986 planta 1.2m
  { "FLUTTER MANE", 0, 0, R_RARO, 0x6AD3, 55, 55, 55, 135, 135, 135, 0, 114, 1, T_GHOST, T_FAIRY, 255 },  // 987 fantasma 1.4m
  { "SLITHER WING", 0, 0, R_RARO, 0x7CC4, 85, 135, 79, 85, 105, 81, 2, 129, 9, T_BUG, T_DRAGON, 255 },  // 988 bicho 3.2m
  { "SANDY SHOCKS", 0, 0, R_RARO, 0xBCA1, 85, 81, 97, 121, 85, 101, 0, 132, 6, T_ELEC, T_GROUND, 255 },  // 989 electrico 2.3m
  { "IRON TREADS", 0, 0, R_RARO, 0xB447, 90, 112, 120, 72, 70, 106, 4, 125, 24, T_GROUND, T_STEEL, 255 },  // 990 tierra 0.9m
  { "IRON BUNDLE", 0, 0, R_RARO, 0x4DB8, 56, 80, 114, 124, 60, 136, 5, 108, 1, T_ICE, T_WATER, 255 },  // 991 hielo 0.6m
  { "IRON HANDS", 0, 0, R_RARO, 0xA2A5, 154, 140, 108, 50, 68, 50, 0, 145, 38, T_FIGHT, T_ELEC, 255 },  // 992 lucha 1.8m
  { "IRON JUGULIS", 0, 0, R_RARO, 0x49E6, 94, 80, 86, 122, 80, 108, 0, 148, 11, T_DARK, T_FLYING, 255 },  // 993 siniestro 1.3m
  { "IRON MOTH", 0, 0, R_RARO, 0xEA87, 80, 70, 60, 140, 110, 110, 3, 143, 4, T_FIRE, T_POISON, 255 },  // 994 fuego 1.2m
  { "IRON THORNS", 0, 0, R_RARO, 0x9407, 100, 134, 110, 70, 84, 72, 4, 130, 30, T_ROCK, T_ELEC, 255 },  // 995 roca 1.6m
  { "FRIGIBAX", 997, 35, R_RARO, 0x5A98, 65, 75, 45, 35, 45, 55, 1, 87, 2, T_DRAGON, T_ICE, 50 },  // 996 dragon 0.5m
  { "ARCTIBAX", 998, 54, R_EVO, 0x5A98, 90, 95, 66, 45, 65, 62, 1, 108, 3, T_DRAGON, T_ICE, 50 },  // 997 dragon 0.8m
  { "BAXCALIBUR", 0, 0, R_EVO, 0x5A98, 115, 145, 92, 75, 86, 87, 1, 147, 21, T_DRAGON, T_ICE, 50 },  // 998 dragon 2.1m
  { "GIMMIGHOUL", 1000, 50, R_RARO, 0x6AD3, 45, 30, 70, 75, 70, 10, 0, 112, 1, T_GHOST, T_NONE, 255 },  // 999 fantasma 0.3m
  { "GHOLDENGO", 0, 0, R_EVO, 0x7C53, 87, 60, 95, 133, 91, 84, 4, 148, 3, T_STEEL, T_GHOST, 255 },  // 1000 acero 1.2m
  { "WO-CHIEN", 0, 0, R_LEGENDARIO, 0x49E6, 85, 85, 100, 95, 135, 70, 0, 152, 7, T_DARK, T_GRASS, 255 },  // 1001 siniestro 1.5m
  { "CHIEN-PAO", 0, 0, R_LEGENDARIO, 0x49E6, 80, 120, 80, 90, 65, 135, 0, 130, 15, T_DARK, T_ICE, 255 },  // 1002 siniestro 1.9m
  { "TING-LU", 0, 0, R_LEGENDARIO, 0x49E6, 155, 110, 125, 55, 80, 45, 0, 136, 70, T_DARK, T_GROUND, 255 },  // 1003 siniestro 2.7m
  { "CHI-YU", 0, 0, R_LEGENDARIO, 0x49E6, 55, 80, 80, 135, 120, 100, 0, 102, 1, T_DARK, T_FIRE, 255 },  // 1004 siniestro 0.4m
  { "ROARING MOON", 0, 0, R_RARO, 0x5A98, 105, 139, 71, 55, 101, 119, 1, 170, 38, T_DRAGON, T_DARK, 255 },  // 1005 dragon 2.0m
  { "IRON VALIANT", 0, 0, R_RARO, 0xDC76, 74, 130, 90, 120, 60, 116, 0, 131, 4, T_FAIRY, T_FIGHT, 255 },  // 1006 hada 1.4m
  { "KORAIDON", 0, 0, R_LEGENDARIO, 0xA2A5, 100, 135, 115, 85, 100, 135, 0, 154, 30, T_FIGHT, T_DRAGON, 255 },  // 1007 lucha 2.5m
  { "MIRAIDON", 0, 0, R_LEGENDARIO, 0xBCA1, 100, 85, 100, 135, 115, 135, 0, 146, 24, T_ELEC, T_DRAGON, 255 },  // 1008 electrico 3.5m
  { "WALKING WAKE", 0, 0, R_LEGENDARIO, 0x4C98, 99, 83, 91, 125, 83, 109, 1, 145, 28, T_WATER, T_DRAGON, 255 },  // 1009 agua 3.5m
  { "IRON LEAVES", 0, 0, R_LEGENDARIO, 0x3C49, 90, 130, 88, 70, 108, 104, 2, 114, 12, T_GRASS, T_PSYCHIC, 255 },  // 1010 planta 1.5m
  { "DIPPLIN", 1019, 50, R_EVO, 0x3C49, 80, 80, 110, 95, 80, 40, 2, 106, 1, T_GRASS, T_DRAGON, 50 },  // 1011 planta 0.4m
  { "POLTCHAGEIST", 1013, 0, R_COMUN, 0x3C49, 40, 45, 45, 74, 54, 50, 2, 113, 1, T_GRASS, T_GHOST, 255 },  // 1012 planta 0.1m
  { "SINISTCHA", 0, 0, R_EVO, 0x6AD3, 71, 60, 106, 121, 80, 70, 0, 111, 1, T_GHOST, T_GRASS, 255 },  // 1013 fantasma 0.2m
  { "OKIDOGI", 0, 0, R_LEGENDARIO, 0x8A73, 88, 128, 115, 58, 80, 80, 0, 139, 9, T_POISON, T_FIGHT, 0 },  // 1014 veneno 1.8m
  { "MUNKIDORI", 0, 0, R_LEGENDARIO, 0x8A73, 88, 75, 66, 130, 90, 106, 0, 107, 1, T_POISON, T_PSYCHIC, 0 },  // 1015 veneno 1.0m
  { "FEZANDIPITI", 0, 0, R_LEGENDARIO, 0x8A73, 88, 91, 82, 70, 125, 99, 0, 114, 3, T_POISON, T_FAIRY, 0 },  // 1016 veneno 1.4m
  { "OGERPON", 0, 0, R_LEGENDARIO, 0x3C49, 80, 120, 84, 60, 96, 110, 2, 125, 4, T_GRASS, T_NONE, 100 },  // 1017 planta 1.2m
  { "ARCHALUDON", 0, 0, R_EVO, 0x7C53, 90, 105, 130, 125, 65, 85, 4, 139, 6, T_STEEL, T_DRAGON, 50 },  // 1018 acero 2.0m
  { "HYDRAPPLE", 0, 0, R_EVO, 0x3C49, 106, 80, 110, 130, 80, 44, 2, 109, 9, T_GRASS, T_DRAGON, 50 },  // 1019 planta 1.8m
  { "GOUGING FIRE", 0, 0, R_LEGENDARIO, 0xEA87, 105, 115, 121, 65, 93, 91, 3, 147, 59, T_FIRE, T_DRAGON, 255 },  // 1020 fuego 3.5m
  { "RAGING BOLT", 0, 0, R_LEGENDARIO, 0xBCA1, 125, 73, 91, 137, 89, 75, 0, 136, 48, T_ELEC, T_DRAGON, 255 },  // 1021 electrico 5.2m
  { "IRON BOULDER", 0, 0, R_LEGENDARIO, 0x9407, 90, 120, 80, 68, 108, 124, 4, 140, 16, T_ROCK, T_PSYCHIC, 255 },  // 1022 roca 1.5m
  { "IRON CROWN", 0, 0, R_LEGENDARIO, 0x7C53, 90, 72, 100, 122, 108, 98, 4, 118, 16, T_STEEL, T_PSYCHIC, 255 },  // 1023 acero 1.6m
  { "TERAPAGOS", 0, 0, R_LEGENDARIO, 0x8C4D, 90, 65, 85, 65, 85, 60, 0, 99, 1, T_NORMAL, T_NONE, 50 },  // 1024 normal 0.2m
  { "PECHARUNT", 0, 0, R_LEGENDARIO, 0x8A73, 88, 88, 160, 88, 88, 88, 0, 102, 1, T_POISON, T_GHOST, 255 },  // 1025 veneno 0.3m
};

static inline bool dexGenderless(int16_t dex) {
  return dex >= 1 && dex <= DEX_COUNT &&
         DEX_TBL[dex].femPct == GENDERLESS;
}

// PS a un nivel. UNA sola cuenta para los CUATRO sitios que la hacian
// por su cuenta (Combatant::init, Pet::maxHp, la fila del equipo y la
// barra del combate), que ya se habian despegado: la del combate se
// olvidaba del entrenamiento.
//
// Era `bHp * 2 + nivel * 2 + 10`, que escala con la BASE y casi nada
// con el NIVEL. El dano si escala con el nivel, asi que las dos cosas
// iban en escalas distintas: a nivel 11 un Snorlax tenia 352 PS cuando
// en los juegos tiene 59, y tumbar a cualquiera costaba de 3 a 6 veces
// mas turnos. Por eso hasta un golpe muy eficaz parecia hacer
// cosquillas: el dano estaba BIEN, el deposito estaba inflado.
//
// Ahora es la de los juegos, con nuestro entrenamiento (trHp, 0..100)
// en el sitio de los EVs. Al bajar el tope, `Pet::hpSync()` recorta lo
// que sobre, asi que una partida vieja entra sin tocar nada.
// La cuenta, a partir del PS BASE. Existe aparte porque una
// megaevolucion no tiene numero de dex y su bHp sale de MEGA_TBL:
// sin esto habria que copiar la formula en battle.cpp, que es
// justo lo que este fichero vino a evitar.
// bHp en uint16_t: una mega puede pasar de 255 (el Mewtwo
// Armadura del Anil tiene 400 de PS base).
static inline uint16_t hpAtLevelBase(uint16_t bHp, uint16_t level,
                                     uint8_t trHp) {
  uint32_t v = ((uint32_t)bHp * 2 + trHp / 4) * level / 100
             + level + 10;
  return (uint16_t)(v > 65535 ? 65535 : (v ? v : 1));
}

// Para una FORMA (un Meowth de Galar no tiene los mismos PS base)
// esta hpAtLevelForm() en forms.h. No puede estar aqui: forms.h
// necesita los T_* y el DEX_COUNT de este fichero, asi que si este
// mirara FORM_TBL los dos se incluirian en circulo.
static inline uint16_t hpAtLevel(int16_t dex, uint16_t level, uint8_t trHp) {
  if (dex < 1 || dex > DEX_COUNT) return 1;
  return hpAtLevelBase(DEX_TBL[dex].bHp, level, trHp);
}

// Los otros CINCO stats, tambien como en los juegos. Estaba duplicada
// en battle.cpp y pet.cpp (las dos `calcStat`, identicas) y era
// `base * gene / 100 + nivel + tr`: casi no crecia con el nivel. Un
// Charizard tenia Ataque 85 a nivel 1 y 134 a nivel 50, cuando en los
// juegos son 7 y 104 — subir de nivel apenas se notaba en la ficha.
//
// No se veia en el dano porque A/D se cancela: atacante y defensor
// usaban la misma cuenta mala. Lo que arregla esto es el TACTO de
// subir de nivel y que los numeros de la ficha signifiquen algo.
//
// `gene` (90..110) hace de IV y `tr` (0..100) de EV, en los mismos
// sitios que ocupan en la formula de verdad.
static inline uint16_t statAtLevel(uint8_t base, uint8_t gene,
                                   uint16_t level, uint8_t tr) {
  uint32_t v = ((uint32_t)base * 2 * gene / 100 + tr / 4) * level / 100 + 5;
  return (uint16_t)(v > 65535 ? 65535 : (v ? v : 1));
}

// atajo: efectividad de un golpe contra una especie de la Pokedex
static inline uint16_t typeEffectVs(uint8_t atk, int16_t dex) {
  if (dex < 1 || dex > DEX_COUNT) return 100;
  return typeEffect(atk, DEX_TBL[dex].type1, DEX_TBL[dex].type2);
}

// La forma BASE de la linea evolutiva: sube por la cadena hasta el
// que no tiene padre. Un Charizard deja un huevo de CHARMANDER, no
// de Charizard: de la otra forma la despedida regalaba una forma
// final de nivel 1 y el linaje se saltaba entero el criar.
//
// La tabla solo apunta hacia DELANTE (evolvesTo), asi que hay que
// buscar quien apunta a este. Es un barrido de la tabla entera y se
// llama una vez por huevo, asi que no importa.
static inline int16_t baseFormOf(int16_t dex) {
  if (dex < 1 || dex > DEX_COUNT) return dex;
  for (int salto = 0; salto < 3; salto++) {  // ninguna cadena pasa de 3
    int16_t padre = 0;
    for (int16_t d = 1; d <= DEX_COUNT && !padre; d++)
      if (DEX_TBL[d].evolvesTo == dex) padre = d;
    if (!padre) break;
    dex = padre;
  }
  return dex;
}

// Nombres oficiales por idioma. FR y DE son los unicos latinos que difieren
// del ingles en gen 1 (ES/IT/PT usan el de DEX_TBL); JA y KO van en UTF-8,
// que se pinta con la fuente U8g2. nullptr = sin nombre propio.
static const char *const DEX_NAME_FR[DEX_COUNT + 1] = {
  nullptr, "BULBIZARRE", "HERBIZARRE", "FLORIZARRE",
  "SALAMECHE", "REPTINCEL", "DRACAUFEU", "CARAPUCE",
  "CARABAFFE", "TORTANK", "CHENIPAN", "CHRYSACIER",
  "PAPILUSION", "ASPICOT", "COCONFORT", "DARDARGNAN",
  "ROUCOOL", "ROUCOUPS", "ROUCARNAGE", nullptr,
  "RATTATAC", "PIAFABEC", "RAPASDEPIC", "ABO",
  nullptr, nullptr, nullptr, "SABELETTE",
  "SABLAIREAU", nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, "MELOFEE",
  "MELODELFE", "GOUPIX", "FEUNARD", "RONDOUDOU",
  "GRODOUDOU", "NOSFERAPTI", "NOSFERALTO", "MYSTHERBE",
  "ORTIDE", "RAFFLESIA", nullptr, nullptr,
  "MIMITOSS", "AEROMITE", "TAUPIQUEUR", "TRIOPIKEUR",
  "MIAOUSS", nullptr, "PSYKOKWAK", "AKWAKWAK",
  "FEROSINGE", "COLOSSINGE", "CANINOS", "ARCANIN",
  "PTITARD", "TETARTE", "TARTARD", nullptr,
  nullptr, nullptr, "MACHOC", "MACHOPEUR",
  "MACKOGNEUR", "CHETIFLOR", "BOUSTIFLOR", "EMPIFLOR",
  nullptr, nullptr, "RACAILLOU", "GRAVALANCH",
  "GROLEM", nullptr, "GALOPA", "RAMOLOSS",
  "FLAGADOSS", "MAGNETI", nullptr, "CANARTICHO",
  nullptr, nullptr, "OTARIA", "LAMANTINE",
  "TADMORV", "GROTADMORV", "KOKIYAS", "CRUSTABRI",
  "FANTOMINUS", "SPECTRUM", "ECTOPLASMA", nullptr,
  "SOPORIFIK", "HYPNOMADE", nullptr, "KRABBOSS",
  "VOLTORBE", nullptr, "NOEUNOEUF", "NOADKOKO",
  "OSSELAIT", "OSSATUEUR", "KICKLEE", "TYGNON",
  "EXCELANGUE", "SMOGO", "SMOGOGO", "RHINOCORNE",
  "RHINOFEROS", "LEVEINARD", "SAQUEDENEU", "KANGOUREX",
  "HYPOTREMPE", "HYPOCEAN", "POISSIRENE", "POISSOROY",
  "STARI", "STAROSS", "M. MIME", "INSECATEUR",
  "LIPPOUTOU", "ELEKTEK", nullptr, "SCARABRUTE",
  nullptr, "MAGICARPE", "LEVIATOR", "LOKHLASS",
  "METAMORPH", "EVOLI", "AQUALI", "VOLTALI",
  "PYROLI", nullptr, "AMONITA", "AMONISTAR",
  nullptr, nullptr, "PTERA", "RONFLEX",
  "ARTIKODIN", "ELECTHOR", "SULFURA", "MINIDRACO",
  "DRACO", "DRACOLOSSE", nullptr, nullptr,
  "GERMIGNON", "MACRONIUM", nullptr, "HERICENDRE",
  "FEURISSON", nullptr, "KAIMINUS", "CROCRODIL",
  "ALIGATUEUR", "FOUINETTE", "FOUINAR", nullptr,
  "NOARFANG", "COXY", "COXYCLAQUE", "MIMIGAL",
  "MIGALOS", "NOSTENFER", "LOUPIO", nullptr,
  nullptr, "MELO", "TOUDOUDOU", nullptr,
  nullptr, nullptr, nullptr, "WATTOUAT",
  "LAINERGIE", "PHARAMP", "JOLIFLOR", nullptr,
  nullptr, "SIMULARBRE", "TARPAUD", "GRANIVOL",
  "FLORAVOL", "COTOVOL", "CAPUMAIN", "TOURNEGRIN",
  "HELIATRONC", nullptr, "AXOLOTO", "MARAISTE",
  "MENTALI", "NOCTALI", "CORNEBRE", "ROIGADA",
  "FEUFOREVE", "ZARBI", "QULBUTOKE", nullptr,
  "POMDEPIK", "FORETRESS", "INSOLOURDO", "SCORPLANE",
  nullptr, nullptr, nullptr, nullptr,
  "CIZAYOX", "CARATROC", "SCARHINO", "FARFURET",
  nullptr, nullptr, "LIMAGMA", "VOLCAROPOD",
  "MARCACRIN", "COCHIGNON", "CORAYON", nullptr,
  nullptr, "CADOIZO", "DEMANTA", "AIRMURE",
  "MALOSSE", "DEMOLOSSE", "HYPOROI", nullptr,
  nullptr, nullptr, "CERFROUSSE", "QUEULORIOR",
  "DEBUGANT", "KAPOERA", "LIPPOUTI", nullptr,
  nullptr, "ECREMEUH", "LEUPHORIE", nullptr,
  nullptr, nullptr, "EMBRYLEX", "YMPHECT",
  "TYRANOCIF", nullptr, nullptr, nullptr,
  "ARCKO", "MASSKO", "JUNGKO", "POUSSIFEU",
  "GALIFEU", "BRASEGALI", "GOBOU", "FLOBIO",
  "LAGGRON", "MEDHYENA", "GRAHYENA", "ZIGZATON",
  "LINEON", "CHENIPOTTE", "ARMULYS", "CHARMILLON",
  "BLINDALYS", "PAPINOX", "NENUPIOT", nullptr,
  nullptr, "GRAINIPIOT", "PIFEUIL", "TENGALICE",
  "NIRONDELLE", "HELEDELLE", "GOELISE", "BEKIPAN",
  "TARSAL", nullptr, nullptr, "ARAKDO",
  "MASKADRA", "BALIGNON", "CHAPIGNON", "PARECOOL",
  nullptr, "MONAFLEMIT", "NINGALE", nullptr,
  "MUNJA", "CHUCHMUR", "RAMBOUM", "BROUHABAM",
  nullptr, nullptr, nullptr, "TARINOR",
  nullptr, nullptr, "TENEFIX", "MYSDIBULE",
  "GALEKID", "GALEGON", "GALEKING", "MEDITIKKA",
  "CHARMINA", "DYNAVOLT", "ELECSPRINT", "POSIPI",
  "NEGAPI", "MUCIOLE", "LUMIVOLE", nullptr,
  "GLOUPTI", "AVALTOUT", nullptr, nullptr,
  nullptr, nullptr, "CHAMALLOT", nullptr,
  "CHARTOR", nullptr, "GRORET", nullptr,
  "KRAKNOIX", "VIBRANINF", "LIBEGON", nullptr,
  nullptr, "TYLTON", nullptr, "MANGRIFF",
  nullptr, "SELEROC", "SOLAROC", "BARLOCHE",
  "BARBICHA", "ECRAPINCE", "COLHOMARD", "BALBUTO",
  "KAORINE", "LILIA", "VACILYS", nullptr,
  nullptr, "BARPAU", "MILOBELLUS", "MORPHEO",
  nullptr, "POLICHOMBR", "BRANETTE", "SKELENOX",
  "TERACLOPE", nullptr, "EOKO", nullptr,
  "OKEOKE", "STALGAMIN", "ONIGLALI", "OBALIE",
  "PHOGLEUR", "KAIMORSE", "COQUIPERL", "SERPANG",
  "ROSABYSS", nullptr, "LOVDISC", "DRABY",
  "DRACKHAUS", "DRATTAK", "TERHAL", nullptr,
  "METALOSSE", nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, "TORTIPOUSS",
  "BOSKARA", nullptr, "OUISTICRAM", "CHIMPENFEU",
  "SIMIABRAZ", "TIPLOUF", "PRINPLOUF", "PINGOLEON",
  "ETOURMI", "ETOURVOL", "ETOURAPTOR", "KEUNOTOR",
  "CASTORNO", "CRIKZIK", "MELOKRIK", "LIXY",
  nullptr, nullptr, "ROZBOUTON", nullptr,
  "KRANIDOS", "CHARKOS", "DINOCLIER", nullptr,
  "CHENITI", "CHENISELLE", "PAPILORD", "APITRINI",
  "APIREINE", nullptr, "MUSTEBOUEE", "MUSTEFLOTT",
  "CERIBOU", "CERIFLOR", "SANCOKI", "TRITOSOR",
  "CAPIDEXTRE", "BAUDRIVE", "GRODRIVE", "LAPOREILLE",
  "LOCKPIN", "MAGIREVE", "CORBOSS", "CHAGLAM",
  "CHAFFREUX", "KORILLON", "MOUFOUETTE", "MOUFFLAIR",
  "ARCHEOMIRE", "ARCHEODONG", "MANZAI", nullptr,
  "PTIRAVI", "PIJAKO", nullptr, "GRIKNOT",
  "CARMACHE", "CARCHACROK", "GOINFREX", nullptr,
  nullptr, nullptr, "HIPPODOCUS", "RAPION",
  "DRASCORE", "CRADOPAUD", "COATOX", "VORTENTE",
  "ECAYON", nullptr, "BABIMANTA", "BLIZZI",
  "BLIZZAROI", "DIMORET", nullptr, "COUDLANGUE",
  "RHINASTOC", "BOULDENEU", "ELEKABLE", "MAGANON",
  nullptr, nullptr, "PHYLLALI", "GIVRALI",
  "SCORVOL", "MAMMOCHON", nullptr, "GALLAME",
  "TARINORME", "NOCTUNOIR", "MOMARTIK", "MOTISMA",
  "CREHELF", "CREFOLLET", "CREFADET", nullptr,
  nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, "VIPELIERRE",
  "LIANAJA", "MAJASPIC", "GRUIKUI", "GROTICHON",
  "ROITIFLAM", "MOUSTILLON", "MATELOUTRE", "CLAMIRAL",
  "RATENTIF", "MIRADAR", "PONCHIOT", "PONCHIEN",
  "MASTOUFFE", "CHACRIPAN", "LEOPARDUS", "FEUILLAJOU",
  "FEUILOUTAN", "FLAMAJOU", "FLAMOUTAN", "FLOTAJOU",
  "FLOTOUTAN", nullptr, "MUSHANA", "POICHIGEON",
  "COLOMBEAU", "DEFLAISAN", "ZEBIBRON", "ZEBLITZ",
  "NODULITHE", "GEOLITHE", "GIGALITHE", "CHOVSOURIR",
  "RHINOLOVE", "ROTOTAUPE", "MINOTAUPE", "NANMEOUIE",
  "CHARPENTI", "OUVRIFIER", "BETOCHEF", "TRITONDE",
  "BATRACNE", "CRAPUSTULE", "JUDOKRAK", "KARACLEE",
  "LARVEYETTE", "COUVERDURE", "MANTERNEL", "VENIPATTE",
  "SCOBOLIDE", "BRUTAPODE", "DOUDOUVET", "FARFADUVET",
  "CHLOROBULE", "FRAGILADY", "BARGANTUA", "MASCAIMAN",
  "ESCROCO", "CROCORIBLE", "DARUMAROND", "DARUMACHO",
  "MARACACHI", "CRABICOQUE", "CRABARAQUE", "BAGGIGUANE",
  "BAGGAID", "CRYPTERO", "TUTAFEH", "TUTANKAFER",
  "CARAPAGOS", "MEGAPAGOS", "ARKEAPTI", "AEROPTERYX",
  "MIAMIASME", "MIASMAX", nullptr, nullptr,
  "CHINCHIDOU", "PASHMILLA", "SCRUTELLA", "MESMERELLA",
  "SIDERELLA", "NUCLEOS", "MEIOS", "SYMBIOS",
  "COUANETON", "LAKMECYGNE", "SORBEBE", "SORBOUL",
  "SORBOUBOUL", "VIVALDAIM", "HAYDAIM", nullptr,
  "CARABING", "LANCARGOT", "TROMPIGNON", "GAULET",
  "VISKUSE", "MOYADE", "MAMANBO", "STATITIK",
  "MYGAVOLT", "GRINDUR", "NOACIER", "TIC",
  "CLIC", "CLITICLIC", "ANCHWATT", "LAMPEROIE",
  "OHMASSACRE", "LEWSOR", "NEITRAM", "FUNECIRE",
  "MELANCOLUX", "LUGULABRE", "COUPENOTTE", "INCISACHE",
  "TRANCHODON", "POLARHUME", "POLAGRIFFE", "HEXAGEL",
  "ESCARGAUME", "LIMASPEED", "LIMONDE", "KUNGFOUINE",
  "SHAOFOUINE", "DRAKKARMIN", "GRINGOLEM", "GOLEMASTOC",
  "SCALPION", "SCALPROIE", "FRISON", "FURAIGLON",
  "GUERIAIGLE", "VOSTOURNO", "VAUTUTRICE", "AFLAMANOIR",
  "FERMITE", "SOLOCHI", "DIAMAT", "TRIOXHYDRE",
  "PYRONILLE", "PYRAX", "COBALTIUM", "TERRAKIUM",
  "VIRIDIUM", "BOREAS", "FULGURIS", nullptr,
  nullptr, "DEMETEROS", nullptr, nullptr,
  nullptr, nullptr, "MARISSON", "BOGUERISSE",
  "BLINDEPIQUE", "FEUNNEC", "ROUSSIL", "GOUPELIN",
  "GRENOUSSE", "CROAPORAL", "AMPHINOBI", "SAPEREAU",
  "EXCAVARENNE", "PASSEROUGE", "BRAISILLON", "FLAMBUSARD",
  "LEPIDONILLE", "PEREGRAIN", "PRISMILLON", "HELIONCEAU",
  "NEMELIOS", nullptr, nullptr, nullptr,
  "CABRIOLAINE", "CHEVROUM", "PANDESPIEGLE", "PANDARBARE",
  "COUAFAREL", "PSYSTIGRI", "MISTIGRIX", "MONORPALE",
  "DIMOCLES", "EXAGIDE", "FLUVETIN", "COCOTINE",
  "SUCROQUIN", "CUPCANAILLE", "SEPIATOP", "SEPIATROCE",
  "OPERMINE", "GOLGOPATHE", "VENALGUE", "KRAVARECH",
  "FLINGOUSTE", "GAMBLAST", "GALVARAN", "IGUOLTA",
  "PTYRANIDUR", "REXILLIUS", "AMAGARA", "DRAGMARA",
  "NYMPHALI", "BRUTALIBRE", nullptr, "STRASSIE",
  "MUCUSCULE", "COLIMUCUS", "MUPLODOCUS", "TROUSSELIN",
  "BROCELOME", "DESSELIANDE", "PITROUILLE", "BANSHITROUYE",
  "GRELACON", "SERACRAWL", "SONISTRELLE", "BRUYVERNE",
  nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, "BRINDIBOU", "EFFLECHE",
  "ARCHEDUC", "FLAMIAOU", "MATOUFEU", "FELINFERNO",
  "OTAQUIN", "OTARLETTE", "ORATORIA", "PICASSAUT",
  "PICLAIRON", "BAZOUCAN", "MANGLOUTON", "ARGOUSTE",
  "LARVIBULE", "CHRYSAPILE", "LUCANON", "CRABAGARRE",
  nullptr, "PLUMELINE", "BOMBYDOU", "RUBOMBELLE",
  "ROCABOT", "LOUGAROC", "FROUSSARDINE", "VORASTERIE",
  "PREDASTERIE", "TIBOUDET", "BOURRINOS", "ARAQUA",
  "TARENBULLE", "MIMANTIS", "FLORAMANTIS", "SPODODO",
  "LAMPIGNON", "TRITOX", "MALAMANDRE", "NOUNOURSON",
  "CHELOURS", "CROQUINE", "CANDINE", "SUCREINE",
  "GUERILANDE", "GOUROUTAN", "QUARTERMAC", "SOVKIPOU",
  "SARMURAI", "BACABOUH", "TREPASSABLE", "CONCOMBAFFE",
  "TYPE:0", "SILVALLIE", "METENO", "DODOALA",
  "BOUMATA", nullptr, "MIMIQUI", "DENTICRISSE",
  "DRAIEUL", "SINISTRAIL", "BEBECAILLE", "ECAID",
  "EKAISER", "TOKORICO", "TOKOPIYON", "TOKOTORO",
  "TOKOPISCO", nullptr, "COSMOVUM", nullptr,
  nullptr, "ZEROID", "MOUSCOTO", "CANCRELOVE",
  "CABLIFERE", "BAMBOISELLE", "KATAGAMI", "ENGLOUTYRAN",
  nullptr, nullptr, nullptr, "VEMINI",
  "MANDRILLON", "AMA-AMA", "PIERROTEKNIK", nullptr,
  nullptr, nullptr, "OUISTEMPO", "BADABOUIN",
  "GORYTHMIC", "FLAMBINO", "LAPYRO", "PYROBUT",
  "LARMELEON", "ARROZARD", "LEZARGUS", "RONGOURMAND",
  "RONGRIGOU", "MINISANGE", "BLEUSEILLE", "CORVAILLUS",
  "LARVADAR", "COLEODOME", "ASTRONELLE", "GOUPILOU",
  "ROUBLENARD", "TOURNICOTON", "BLANCOTON", "MOUMOUTON",
  "MOUMOUFLON", "KHELOCROK", "TORGAMORD", "VOLTOUTOU",
  "FULGUDOG", "CHARBI", "WAGOMINE", "MONTHRACITE",
  "VERPOM", "POMDRAPI", "DRATATIN", "DUNAJA",
  "DUNACONDA", "NIGOSIER", "EMBROCHET", "HASTACUDA",
  "TOXIZAP", "SALARSEN", "GRILLEPATTES", "SCOLOCENDRE",
  "POULPAF", "KRAKOS", "THEFFROI", "POLTHEGEIST",
  "BIBICHUT", "CHAPOTUS", "SORCILENCE", "GRIMALIN",
  "FOURBELIN", "ANGOLIATH", "IXON", "BERSERKATT",
  "CORAYOME", "PALARTICHO", "M. GLAQUETTE", "TUTETEKRI",
  "CREMY", "CHARMILLY", "HEXADRON", "WATTAPIK",
  "FRISSONILLE", "BELDENEIGE", "DOLMAN", "BEKAGLACON",
  "WIMESSIR", nullptr, "CHARIBARI", "PACHYRADJAH",
  "GALVAGON", "GALVAGLA", "HYDRAGON", "HYDRAGLA",
  "DURALUGON", "FANTYRM", "DISPAREPTIL", "LANSSORIEN",
  nullptr, nullptr, "ETHERNATOS", "WUSHOURS",
  "SHIFOURS", nullptr, nullptr, nullptr,
  "BLIZZEVAL", "SPECTREVAL", "SYLVEROY", "CERBYLLIN",
  "HACHECATEUR", "URSAKING", "PARAGRUEL", "FARFUREX",
  "QWILPIK", "AMOVENUS", "POUSSACHA", "MATOURGEON",
  "MIASCARADE", "CHOCHODILE", "CROCOGRIL", "FLAMIGATOR",
  "COIFFETON", "CANARBELLO", "PALMAVAL", "GOURMELET",
  "FRAGROIN", "TISSENBOULE", "FILENTRAPPE", "LILLITERELLE",
  "GAMBEX", "POHM", "POHMOTTE", "POHMARMOTTE",
  "COMPAGNOL", "FAMIGNOL", "PATACHIOT", "BRIOCHIEN",
  "OLIVINI", "OLIVADO", nullptr, "TAPATOES",
  "SELUTIN", "AMASSEL", "GIGANSEL", "CHARBAMBIN",
  "CARMADURA", "MALVALAME", "TETAMPOULE", "AMPIBIDOU",
  "ZAPETREL", "FULGULAIRO", "GRONDOGUE", "DOGRINO",
  "GRIBOURAIGNE", "TAG-TAG", "VIROVENT", "VIREVORREUR",
  "TERRACOOL", "TERRACRUEL", "CRAPAROI", "PIMITO",
  "SCOVILAIN", "LEBOULEROU", "BERASCA", "FLOTILLON",
  "CLEOPSYTRA", "FORGERETTE", "FORGELLA", "FORGELINA",
  "TAUPIKEAU", "TRIOPIKEAU", "LESTOMBAILE", "DOFIN",
  "SUPERDOFIN", "VROMBI", "VROMBOTOR", "MOTORIZARD",
  "FERDETER", "GERMECLAT", "FLORECLAT", "TOUTOMBE",
  "TOMBERRO", "FLAMENROULE", "PIETACE", "BALBALEZE",
  "DELESTIN", "OYACATA", "NIGIRIGON", "COURROUSINGE",
  "TERRAISTE", nullptr, "DEUSOLOURDO", "SCALPEREUR",
  "FORT-IVOIRE", "HURLE-QUEUE", "FONGUS-FURIE", "FLOTTE-MECHE",
  "RAMPE-AILES", "PELAGE-SABLE", "ROUE-DE-FER", "HOTTE-DE-FER",
  "PAUME-DE-FER", "TETES-DE-FER", "MITE-DE-FER", "EPINE-DE-FER",
  "FRIGODO", "CRYODO", "GLAIVODO", "MORDUDOR",
  "GROMAGO", "CHONGJIAN", "BAOJIAN", "DINGLU",
  "YUYU", "RUGIT-LUNE", "GARDE-DE-FER", nullptr,
  nullptr, "SERPENTE-EAU", "VERT-DE-FER", "POMDRAMOUR",
  nullptr, "THEFFROYABLE", "FELICANIS", "FORTUSIMIA",
  "FAVIANOS", nullptr, "PONDRALUGON", "POMDOROCHI",
  "FEU-PERCANT", "IRE-FOUDRE", "ROC-DE-FER", "CHEF-DE-FER",
  nullptr, "PECHAMINUS",
};

static const char *const DEX_NAME_DE[DEX_COUNT + 1] = {
  nullptr, "BISASAM", "BISAKNOSP", "BISAFLOR",
  "GLUMANDA", "GLUTEXO", "GLURAK", "SCHIGGY",
  "SCHILLOK", "TURTOK", "RAUPY", "SAFCON",
  "SMETTBO", "HORNLIU", "KOKUNA", "BIBOR",
  "TAUBSI", "TAUBOGA", "TAUBOSS", "RATTFRATZ",
  "RATTIKARL", "HABITAK", "IBITAK", "RETTAN",
  nullptr, nullptr, nullptr, "SANDAN",
  "SANDAMER", nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, "PIEPI",
  "PIXI", nullptr, "VULNONA", "PUMMELUFF",
  "KNUDDELUFF", nullptr, nullptr, "MYRAPLA",
  "DUFLOR", "GIFLOR", nullptr, "PARASEK",
  "BLUZUK", "OMOT", "DIGDA", "DIGDRI",
  "MAUZI", "SNOBILIKAT", "ENTON", "ENTORON",
  "MENKI", "RASAFF", "FUKANO", "ARKANI",
  "QUAPSEL", "QUAPUTZI", "QUAPPO", nullptr,
  nullptr, "SIMSALA", "MACHOLLO", "MASCHOCK",
  "MACHOMEI", "KNOFENSA", "ULTRIGARIA", "SARZENIA",
  "TENTACHA", "TENTOXA", "KLEINSTEIN", "GEOROK",
  "GEOWAZ", "PONITA", "GALLOPA", "FLEGMON",
  "LAHMUS", "MAGNETILO", nullptr, "PORENTA",
  "DODU", "DODRI", "JUROB", "JUGONG",
  "SLEIMA", "SLEIMOK", "MUSCHAS", "AUSTOS",
  "NEBULAK", "ALPOLLO", nullptr, nullptr,
  "TRAUMATO", nullptr, nullptr, nullptr,
  "VOLTOBAL", "LEKTROBAL", "OWEI", "KOKOWEI",
  "TRAGOSSO", "KNOGGA", "KICKLEE", "NOCKCHAN",
  "SCHLURP", "SMOGON", "SMOGMOG", "RIHORN",
  "RIZEROS", "CHANEIRA", nullptr, "KANGAMA",
  "SEEPER", "SEEMON", "GOLDINI", "GOLKING",
  "STERNDU", nullptr, "PANTIMOS", "SICHLOR",
  "ROSSANA", "ELEKTEK", nullptr, nullptr,
  nullptr, "KARPADOR", "GARADOS", nullptr,
  nullptr, "EVOLI", "AQUANA", "BLITZA",
  "FLAMARA", nullptr, "AMONITAS", "AMOROSO",
  nullptr, nullptr, nullptr, "RELAXO",
  "ARKTOS", nullptr, "LAVADOS", nullptr,
  "DRAGONIR", "DRAGORAN", "MEWTU", nullptr,
  "ENDIVIE", "LORBLATT", "MEGANIE", "FEURIGEL",
  "IGELAVAR", "TORNUPTO", "KARNIMANI", "TYRACROC",
  "IMPERGATOR", "WIESOR", "WIESENIOR", nullptr,
  "NOCTUH", nullptr, nullptr, "WEBARAK",
  nullptr, "IKSBAT", "LAMPI", nullptr,
  nullptr, "PII", "FLUFFELUFF", nullptr,
  nullptr, nullptr, nullptr, "VOLTILAMM",
  "WAATY", nullptr, "BLUBELLA", nullptr,
  nullptr, "MOGELBAUM", "QUAXO", "HOPPSPROSS",
  "HUBELUPF", "PAPUNGHA", "GRIFFEL", "SONNKERN",
  "SONNFLORA", nullptr, "FELINO", "MORLORD",
  "PSIANA", "NACHTARA", "KRAMURX", "LASCHOKING",
  "TRAUNFUGIL", "ICOGNITO", "WOINGENAU", nullptr,
  "TANNZA", "FORSTELLKA", "DUMMISEL", "SKORGLA",
  "STAHLOS", nullptr, nullptr, "BALDORFISH",
  "SCHEROX", "POTTROTT", "SKARABORN", "SNIEBEL",
  nullptr, nullptr, "SCHNECKMAG", nullptr,
  "QUIEKEL", "KEIFEL", "CORASONN", nullptr,
  nullptr, "BOTOGEL", "MANTAX", "PANZAERON",
  "HUNDUSTER", "HUNDEMON", "SEEDRAKING", nullptr,
  nullptr, nullptr, "DAMHIRPLEX", "FARBEAGLE",
  "RABAUZ", "KAPOERA", "KUSSILLA", nullptr,
  nullptr, nullptr, "HEITEIRA", nullptr,
  nullptr, nullptr, nullptr, nullptr,
  "DESPOTAR", nullptr, nullptr, nullptr,
  "GECKARBOR", "REPTAIN", "GEWALDRO", "FLEMMLI",
  "JUNGGLUT", "LOHGOCK", "HYDROPI", "MOORABBEL",
  "SUMPEX", "FIFFYEN", "MAGNAYEN", "ZIGZACHS",
  "GERADAKS", "WAUMPEL", "SCHALOKO", "PAPINELLA",
  "PANEKON", "PUDOX", "LOTURZEL", "LOMBRERO",
  "KAPPALORES", "SAMURZEL", "BLANAS", "TENGULIST",
  "SCHWALBINI", "SCHWALBOSS", nullptr, nullptr,
  "TRASLA", nullptr, "GUARDEVOIR", "GEHWEIHER",
  "MASKEREGEN", "KNILZ", "KAPILZ", "BUMMELZ",
  "MUNTIER", "LETARKING", nullptr, nullptr,
  "NINJATOM", "FLURMEL", "KRAKEELO", "KRAWUMMS",
  nullptr, nullptr, nullptr, "NASGNET",
  "ENECO", "ENEKORO", "ZOBIRIS", "FLUNKIFER",
  "STOLLUNIOR", "STOLLRAK", "STOLLOSS", "MEDITIE",
  "MEDITALIS", "FRIZELBLIZ", "VOLTENSO", nullptr,
  nullptr, nullptr, nullptr, nullptr,
  "SCHLUPPUCK", "SCHLUKWECH", "KANIVANHA", "TOHAIDO",
  nullptr, nullptr, "CAMAUB", nullptr,
  "QURTEL", nullptr, "GROINK", "PANDIR",
  "KNACKLION", nullptr, "LIBELLDRA", "TUSKA",
  "NOKTUSKA", "WABLU", nullptr, "SENGO",
  "VIPITIS", "LUNASTEIN", "SONNFEL", "SCHMERBE",
  "WELSAR", "KREBSCORPS", "KREBUTACK", "PUPPANCE",
  "LEPUMENTAS", "LILIEP", "WIELIE", nullptr,
  nullptr, "BARSCHWA", nullptr, "FORMEO",
  nullptr, nullptr, nullptr, "ZWIRRLICHT",
  "ZWIRRKLOP", nullptr, "PALIMPALIM", nullptr,
  "ISSO", "SCHNEPPKE", "FIRNONTOR", "SEEMOPS",
  "SEEJONG", "WALRAISA", "PERLU", "AALABYSS",
  "SAGANABYSS", nullptr, "LIEBISKUS", "KINDWURM",
  "DRASCHEL", "BRUTALANDA", "TANHEL", nullptr,
  nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, "CHELAST",
  "CHELCARAIN", "CHELTERRAR", "PANFLAM", "PANPYRO",
  "PANFERNO", "PLINFA", "PLIPRIN", "IMPOLEON",
  "STARALILI", nullptr, nullptr, "BIDIZA",
  "BIDIFAS", "ZIRPURZE", "ZIRPEISE", "SHEINUX",
  nullptr, "LUXTRA", "KNOSPI", nullptr,
  "KOKNODON", "RAMEIDON", "SCHILTERUS", "BOLLTERUS",
  nullptr, "BURMADAME", "MOTERPEL", "WADRIBIE",
  "HONWEISEL", nullptr, "BAMELIN", "BOJELIN",
  "KIKUGI", "KINOSO", "SCHALELLOS", nullptr,
  "AMBIDIFFEL", "DRIFTLON", "DRIFZEPELI", "HASPIROR",
  "SCHLAPOR", "TRAUNMAGIL", "KRAMSHEF", "CHARMIAN",
  "SHNURGARST", "KLINGPLIM", "SKUNKAPUH", nullptr,
  "BRONZEL", nullptr, "MOBAI", "PANTIMIMI",
  "WONNEIRA", "PLAUDAGEI", "KRYPPUK", "KAUMALAT",
  "KNARKSEL", "KNAKRACK", "MAMPFAXO", nullptr,
  nullptr, nullptr, "HIPPOTERUS", "PIONSKORA",
  "PIONDRAGI", "GLIBUNKEL", "TOXIQUAK", "VENUFLIBIS",
  nullptr, nullptr, "MANTIRPS", "SHNEBEDECK",
  "REXBLISAR", "SNIBUNNA", nullptr, "SCHLURPLEK",
  "RIHORNIOR", "TANGOLOSS", "ELEVOLTEK", "MAGBRANT",
  nullptr, nullptr, "FOLIPURBA", "GLAZIOLA",
  "SKORGRO", "MAMUTEL", nullptr, "GALAGLADI",
  "VOLUMINAS", "ZWIRRFINST", "FROSDEDJE", nullptr,
  "SELFE", "VESPRIT", "TOBUTZ", nullptr,
  nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, "SERPIFEU",
  "EFOSERP", "SERPIROYAL", "FLOINK", "FERKOKEL",
  "FLAMBIREX", "OTTARO", "ZWOTTRONIN", "ADMURAI",
  "NAGELOTZ", "KUKMARDA", "YORKLEFF", "TERRIBARK",
  "BISSBARK", "FELILOU", "KLEOPARDA", "VEGIMAK",
  "VEGICHITA", "GRILLMAK", "GRILLCHITA", "SODAMAK",
  "SODACHITA", "SOMNIAM", "SOMNIVORA", "DUSSELGURR",
  "NAVITAUB", "FASASNOB", "ELEZEBA", "ZEBRITZ",
  "KIESLING", "SEDIMANTUR", "BROCKOLOSS", "FLEKNOIL",
  "FLETIAMO", "ROTOMURF", "STALOBOR", "OHRDOCH",
  "PRAKTIBALK", "STREPOLI", "MEISTAGRIF", "SCHALLQUAP",
  "MEBRANA", "BRANAWARZ", "JIUTESTO", "KARADONIS",
  "STRAWICKL", "FOLIKON", "MATRIFOL", "TOXIPED",
  "ROLLUM", "CERAPENDRA", "WAUMBOLL", "ELFUN",
  "LILMINIP", "DRESSELLA", "BARSCHUFT", "GANOVIL",
  "ROKKAIMAN", "RABIGATOR", "FLAMPION", "FLAMPIVIAN",
  "MARACAMBA", "LITHOMITH", "CASTELLITH", "ZURROKEX",
  "IROKEX", "SYMVOLARA", "MAKABAJA", "ECHNATOLL",
  "GALAPAFLOS", "KARIPPAS", "FLAPTERYX", "AEROPTERYX",
  "UNRATUTOX", "DEPONITOX", nullptr, nullptr,
  "PICOCHILLA", "CHILLABELL", "MOLLIMORBA", "HYPNOMORBA",
  "MORBITESSE", "MONOZYTO", "MITODOS", "ZYTOMEGA",
  "PICCOLENTE", "SWARONESS", "GELATINI", "GELATROPPO",
  "GELATWINO", "SESOKITZ", "KRONJUWILD", nullptr,
  "LAUKAPS", "CAVALANZAS", "TARNPIGNON", "HUTSASSA",
  "QUABBEL", "APOQUALLYP", "MAMOLIDA", "WATTZAPF",
  "VOLTULA", "KASTADUR", "TENTANTEL", "KLIKK",
  "KLIKLAK", "KLIKDIKLAK", "ZAPPLARDIN", "ZAPPLALEK",
  "ZAPPLARANG", "PYGRAULON", "MEGALON", "LICHTEL",
  "LATERNECTO", "SKELABRA", "MILZA", "SHARFAX",
  "MAXAX", "PETZNIEF", "SIBERIO", "FRIGOMETRI",
  "SCHNUTHELM", "HYDRAGIL", "FLUNSCHLIK", "LIN-FU",
  "WIE-SHU", "SHARDRAGO", "GOLBIT", "GOLGANTES",
  "GLADIANTRI", "CAESURIO", "BISOFANK", "GERONIMATZ",
  "WASHAKWIL", "SKALLYK", "GRYPHELDIS", "FURNIFRASS",
  "FERMICULA", "KAPUNO", "DUODINO", "TRIKEPHALO",
  "IGNIVOR", "RAMOTH", "KOBALIUM", "TERRAKIUM",
  "VIRIDIUM", "BOREOS", "VOLTOLOS", nullptr,
  nullptr, "DEMETEROS", nullptr, nullptr,
  nullptr, nullptr, "IGAMARO", "IGASTARNISH",
  "BRIGARON", "FYNX", "RUTENA", "FENNEXIS",
  "FROXY", "AMPHIZEL", "QUAJUTSU", "SCOPPEL",
  "GREBBIT", "DARTIRI", "DARTIGNIS", "FIARO",
  "PURMEL", "PUPONCHO", nullptr, "LEUFEO",
  "PYROLEO", nullptr, nullptr, nullptr,
  "MAHIKEL", "CHEVRUMM", "PAM-PAM", "PANDAGRO",
  "COIFFWAFF", "PSIAU", "PSIAUGON", "GRAMOKLES",
  "DUOKLES", "DURENGARD", "PARFI", "PARFINESSE",
  "FLAUSCHLING", "SABBAIONE", "ISCALAR", "CALAMANERO",
  "BITHORA", "THANATHORA", "ALGITT", "TANDRAK",
  "SCAMPISTO", "WUMMER", "EGUANA", "ELEZARD",
  "BALGORAS", "MONARGORAS", "AMARINO", "AMAGARGA",
  "FEELINARA", "RESLADERO", nullptr, "ROCARA",
  "VISCORA", "VISCARGOT", "VISCOGON", "CLAVION",
  "PARAGONI", "TROMBORK", "IRRBIS", "PUMPDJINN",
  "ARKTIP", "ARKTILAS", "EF-EM", "UHAFNIR",
  nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, "BAUZ", "ARBORETOSS",
  "SILVARRO", "FLAMIAU", "MIEZUNDER", "FUEGRO",
  "ROBBALL", "MARIKECK", "PRIMARENE", "PEPPECK",
  "TROMPECK", "TUKANON", "MANGUNIOR", "MANGUSPEKTOR",
  "MABULA", "AKKUP", "DONARION", "KRABBOX",
  "KRAWELL", "CHOREOGEL", "WOMMEL", "BANDELBY",
  "WUFFELS", "WOLWEROCK", "LUSARDIN", "GARSTELLA",
  "AGGROSTELLA", "PAMPULI", "PAMPROSS", "ARAQUA",
  "ARANESTRO", "IMANTIS", "MANTIDEA", "BUBUNGUS",
  "LAMELLUX", "MOLUNK", "AMFIRA", "VELURSI",
  "KOSTURSO", "FRUBBERL", "FRUBAILA", "FRUYAL",
  "CURELEI", "KOMMANDUTAN", "QUARTERMAK", "REISSLAUS",
  "TECTASS", "SANKABUH", "COLOSSAND", "GUFA",
  "TYP:NULL", "AMIGENTO", "METENO", "KOALELU",
  "TORTUNATOR", nullptr, "MIMIGMA", "KNIRFISH",
  "SEN-LONG", "MORUDA", "MINIRAS", "MEDIRAS",
  "GRANDIRAS", "KAPU-RIKI", "KAPU-FALA", "KAPU-TORO",
  "KAPU-KIME", nullptr, "COSMOVUM", nullptr,
  nullptr, "ANEGO", "MASSKITO", "SCHABELLE",
  "VOLTRIANT", "KAGURON", "KATAGAMI", "SCHLINGKING",
  nullptr, nullptr, nullptr, "VENICRO",
  "AGOYON", "MURAMURA", "KOPPLOSIO", nullptr,
  nullptr, nullptr, "CHIMPEP", "CHIMSTIX",
  "GORTROM", "HOPPLO", "KICKERLO", "LIBERLO",
  "MEMMEON", "PHLEGLEON", "INTELLEON", "RAFFEL",
  "SCHLARAFFEL", "MEIKRO", "KRANOVIZ", "KRARMOR",
  "SENSECT", "KERADAR", "MARITELLIT", "KLEPTIFUX",
  "GAUNUX", "COTTINI", "COTTOMI", "WOLLY",
  "ZWOLLOCK", "KAMEHAPS", "KAMALM", "VOLDI",
  "BELLEKTRO", "KLONKETT", "WAGONG", "MONTECARBO",
  "KNAPFEL", "DRAPFEL", "SCHLAPFEL", "SALANGA",
  "SANACONDA", "URGL", "PIKUDA", "BARRAKIEFA",
  nullptr, "RIFFEX", "THERMOPOD", "INFERNOPOD",
  "KLOPPTOPUS", "KAOCTO", "FATALITEE", "MORTIPOT",
  "BRIMOVA", "BRIMANO", "SILEMBRIM", "BAHMON",
  "PELZEBUB", "OLANGAAR", "BARRIKADAX", "MAUZINGER",
  "GORGASONN", "LAUCHZELOT", "PANTIFROST", "OGHNATOLL",
  "HOKUMIL", "POKUSAN", "LEGIOS", "BRITZIGEL",
  "SNOMNOM", "MOTTINEVA", "HUMANOLITH", "KUBUIN",
  "SERVOL", nullptr, "KUPFANTI", "PATINARAJA",
  "LECTRAGON", "LECRYODON", "PESCRAGON", "PESCRYODON",
  nullptr, "GROLLDRA", "PHANDRA", "KATAPULDRA",
  nullptr, nullptr, "ENDYNALOS", "DAKUMA",
  "WULAOSU", nullptr, nullptr, nullptr,
  "POLAROSS", "PHANTOROSS", "CORONOSPA", "DAMYTHIR",
  "AXANTOR", nullptr, "SALMAGNIS", "SNIEBOSS",
  "MYRIADOR", "CUPIDOS", "FELORI", "FELIOSPA",
  "MASKAGATO", "KROKEL", "LOKROKO", "SKELOKROK",
  "KWAKS", "FUENTENTE", "BAILONDA", "FERKULI",
  "FRAGRUNZ", "TARUNDEL", "SPINSIDIAS", "MICRICK",
  "LEXTREMO", "PAMO", "PAMAMO", "PAMOMAMO",
  "ZWIEPS", "FAMIEPS", "HEFEL", "BACKEL",
  "OLINI", "OLIVINIO", "OLITHENA", "KRAWALLORO",
  "GEOSALI", "SEDISAL", "SALTIGANT", "KNARBON",
  "CRIMANZO", "AZUGLADIS", "BLIPP", "WAMPITZ",
  "VOLTREL", "VOLTREAN", "MOBTIFF", "MASTIFIOSO",
  "SPROXI", "AFFITI", "WEHERBA", "HORRERBA",
  "TENTAGRA", "TENTERRA", "KLIBBE", "CHILINGEL",
  "HALUPENJO", "RELLUK", "SKARABAKS", "FLATTUTU",
  "PSIOPATRA", "FORGITA", "TAFFORGITA", "GRANFORGITA",
  "SCHLIGDA", "SCHLIGDRI", "ADEBOM", "NORMIFIN",
  "DELFINATOR", "KNATTOX", "KNATTATOX", "MOPEX",
  "SCHLURM", "LUMISPROSS", "LUMIFLORA", "GRUFF",
  "FRIEDWUFF", "FLAMINKNO", "FLANIWAL", "KOLOWAL",
  "AGILUZA", "HEERASHAI", "NIGIRAGI", "EPITAFF",
  "SUELORD", nullptr, "DUMMIMISEL", "GLADIMPERIO",
  "RIESENZAHN", "BRULLSCHWEIF", "WUTPILZ", "FLATTERHAAR",
  "KRIECHFLUGEL", "SANDFELL", "EISENRAD", "EISENBUNDEL",
  "EISENHAND", "EISENHALS", "EISENFALTER", "EISENDORN",
  "FROSPINO", "CRYOSPINO", "ESPINODON", "GIERSPENST",
  "MONETIGO", "CHONGJIAN", "BAOJIAN", "DINGLU",
  "YUYU", "DONNERSICHEL", "EISENKRIEGER", nullptr,
  nullptr, "WINDEWOGE", "EISENBLATT", "SIRAPFEL",
  "MORTCHA", "FATALITCHA", "BONINU", "BENESARU",
  "BEATORI", nullptr, "BRIDURADON", "HYDRAPFEL",
  "KEILFLAMME", "FURIENBLITZ", "EISENFELS", "EISENHAUPT",
  nullptr, "INFAMOMO",
};

static const char *const DEX_NAME_JA[DEX_COUNT + 1] = {
  nullptr, "フシギダネ", "フシギソウ", "フシギバナ",
  "ヒトカゲ", "リザード", "リザードン", "ゼニガメ",
  "カメール", "カメックス", "キャタピー", "トランセル",
  "バタフリー", "ビードル", "コクーン", "スピアー",
  "ポッポ", "ピジョン", "ピジョット", "コラッタ",
  "ラッタ", "オニスズメ", "オニドリル", "アーボ",
  "アーボック", "ピカチュウ", "ライチュウ", "サンド",
  "サンドパン", "ニドランF", "ニドリーナ", "ニドクイン",
  "ニドランM", "ニドリーノ", "ニドキング", "ピッピ",
  "ピクシー", "ロコン", "キュウコン", "プリン",
  "プクリン", "ズバット", "ゴルバット", "ナゾノクサ",
  "クサイハナ", "ラフレシア", "パラス", "パラセクト",
  "コンパン", "モルフォン", "ディグダ", "ダグトリオ",
  "ニャース", "ペルシアン", "コダック", "ゴルダック",
  "マンキー", "オコリザル", "ガーディ", "ウインディ",
  "ニョロモ", "ニョロゾ", "ニョロボン", "ケーシィ",
  "ユンゲラー", "フーディン", "ワンリキー", "ゴーリキー",
  "カイリキー", "マダツボミ", "ウツドン", "ウツボット",
  "メノクラゲ", "ドククラゲ", "イシツブテ", "ゴローン",
  "ゴローニャ", "ポニータ", "ギャロップ", "ヤドン",
  "ヤドラン", "コイル", "レアコイル", "カモネギ",
  "ドードー", "ドードリオ", "パウワウ", "ジュゴン",
  "ベトベター", "ベトベトン", "シェルダー", "パルシェン",
  "ゴース", "ゴースト", "ゲンガー", "イワーク",
  "スリープ", "スリーパー", "クラブ", "キングラー",
  "ビリリダマ", "マルマイン", "タマタマ", "ナッシー",
  "カラカラ", "ガラガラ", "サワムラー", "エビワラー",
  "ベロリンガ", "ドガース", "マタドガス", "サイホーン",
  "サイドン", "ラッキー", "モンジャラ", "ガルーラ",
  "タッツー", "シードラ", "トサキント", "アズマオウ",
  "ヒトデマン", "スターミー", "バリヤード", "ストライク",
  "ルージュラ", "エレブー", "ブーバー", "カイロス",
  "ケンタロス", "コイキング", "ギャラドス", "ラプラス",
  "メタモン", "イーブイ", "シャワーズ", "サンダース",
  "ブースター", "ポリゴン", "オムナイト", "オムスター",
  "カブト", "カブトプス", "プテラ", "カビゴン",
  "フリーザー", "サンダー", "ファイヤー", "ミニリュウ",
  "ハクリュー", "カイリュー", "ミュウツー", "ミュウ",
  "チコリータ", "ベイリーフ", "メガニウム", "ヒノアラシ",
  "マグマラシ", "バクフーン", "ワニノコ", "アリゲイツ",
  "オーダイル", "オタチ", "オオタチ", "ホーホー",
  "ヨルノズク", "レディバ", "レディアン", "イトマル",
  "アリアドス", "クロバット", "チョンチー", "ランターン",
  "ピチュー", "ピィ", "ププリン", "トゲピー",
  "トゲチック", "ネイティ", "ネイティオ", "メリープ",
  "モココ", "デンリュウ", "キレイハナ", "マリル",
  "マリルリ", "ウソッキー", "ニョロトノ", "ハネッコ",
  "ポポッコ", "ワタッコ", "エイパム", "ヒマナッツ",
  "キマワリ", "ヤンヤンマ", "ウパー", "ヌオー",
  "エーフィ", "ブラッキー", "ヤミカラス", "ヤドキング",
  "ムウマ", "アンノーン", "ソーナンス", "キリンリキ",
  "クヌギダマ", "フォレトス", "ノコッチ", "グライガー",
  "ハガネール", "ブルー", "グランブル", "ハリーセン",
  "ハッサム", "ツボツボ", "ヘラクロス", "ニューラ",
  "ヒメグマ", "リングマ", "マグマッグ", "マグカルゴ",
  "ウリムー", "イノムー", "サニーゴ", "テッポウオ",
  "オクタン", "デリバード", "マンタイン", "エアームド",
  "デルビル", "ヘルガー", "キングドラ", "ゴマゾウ",
  "ドンファン", "ポリゴン２", "オドシシ", "ドーブル",
  "バルキー", "カポエラー", "ムチュール", "エレキッド",
  "ブビィ", "ミルタンク", "ハピナス", "ライコウ",
  "エンテイ", "スイクン", "ヨーギラス", "サナギラス",
  "バンギラス", "ルギア", "ホウオウ", "セレビィ",
  "キモリ", "ジュプトル", "ジュカイン", "アチャモ",
  "ワカシャモ", "バシャーモ", "ミズゴロウ", "ヌマクロー",
  "ラグラージ", "ポチエナ", "グラエナ", "ジグザグマ",
  "マッスグマ", "ケムッソ", "カラサリス", "アゲハント",
  "マユルド", "ドクケイル", "ハスボー", "ハスブレロ",
  "ルンパッパ", "タネボー", "コノハナ", "ダーテング",
  "スバメ", "オオスバメ", "キャモメ", "ペリッパー",
  "ラルトス", "キルリア", "サーナイト", "アメタマ",
  "アメモース", "キノココ", "キノガッサ", "ナマケロ",
  "ヤルキモノ", "ケッキング", "ツチニン", "テッカニン",
  "ヌケニン", "ゴニョニョ", "ドゴーム", "バクオング",
  "マクノシタ", "ハリテヤマ", "ルリリ", "ノズパス",
  "エネコ", "エネコロロ", "ヤミラミ", "クチート",
  "ココドラ", "コドラ", "ボスゴドラ", "アサナン",
  "チャーレム", "ラクライ", "ライボルト", "プラスル",
  "マイナン", "バルビート", "イルミーゼ", "ロゼリア",
  "ゴクリン", "マルノーム", "キバニア", "サメハダー",
  "ホエルコ", "ホエルオー", "ドンメル", "バクーダ",
  "コータス", "バネブー", "ブーピッグ", "パッチール",
  "ナックラー", "ビブラーバ", "フライゴン", "サボネア",
  "ノクタス", "チルット", "チルタリス", "ザングース",
  "ハブネーク", "ルナトーン", "ソルロック", "ドジョッチ",
  "ナマズン", "ヘイガニ", "シザリガー", "ヤジロン",
  "ネンドール", "リリーラ", "ユレイドル", "アノプス",
  "アーマルド", "ヒンバス", "ミロカロス", "ポワルン",
  "カクレオン", "カゲボウズ", "ジュペッタ", "ヨマワル",
  "サマヨール", "トロピウス", "チリーン", "アブソル",
  "ソーナノ", "ユキワラシ", "オニゴーリ", "タマザラシ",
  "トドグラー", "トドゼルガ", "パールル", "ハンテール",
  "サクラビス", "ジーランス", "ラブカス", "タツベイ",
  "コモルー", "ボーマンダ", "ダンバル", "メタング",
  "メタグロス", "レジロック", "レジアイス", "レジスチル",
  "ラティアス", "ラティオス", "カイオーガ", "グラードン",
  "レックウザ", "ジラーチ", "デオキシス", "ナエトル",
  "ハヤシガメ", "ドダイトス", "ヒコザル", "モウカザル",
  "ゴウカザル", "ポッチャマ", "ポッタイシ", "エンペルト",
  "ムックル", "ムクバード", "ムクホーク", "ビッパ",
  "ビーダル", "コロボーシ", "コロトック", "コリンク",
  "ルクシオ", "レントラー", "スボミー", "ロズレイド",
  "ズガイドス", "ラムパルド", "タテトプス", "トリデプス",
  "ミノムッチ", "ミノマダム", "ガーメイル", "ミツハニー",
  "ビークイン", "パチリス", "ブイゼル", "フローゼル",
  "チェリンボ", "チェリム", "カラナクシ", "トリトドン",
  "エテボース", "フワンテ", "フワライド", "ミミロル",
  "ミミロップ", "ムウマージ", "ドンカラス", "ニャルマー",
  "ブニャット", "リーシャン", "スカンプー", "スカタンク",
  "ドーミラー", "ドータクン", "ウソハチ", "マネネ",
  "ピンプク", "ペラップ", "ミカルゲ", "フカマル",
  "ガバイト", "ガブリアス", "ゴンベ", "リオル",
  "ルカリオ", "ヒポポタス", "カバルドン", "スコルピ",
  "ドラピオン", "グレッグル", "ドクロッグ", "マスキッパ",
  "ケイコウオ", "ネオラント", "タマンタ", "ユキカブリ",
  "ユキノオー", "マニューラ", "ジバコイル", "ベロベルト",
  "ドサイドン", "モジャンボ", "エレキブル", "ブーバーン",
  "トゲキッス", "メガヤンマ", "リーフィア", "グレイシア",
  "グライオン", "マンムー", "ポリゴンＺ", "エルレイド",
  "ダイノーズ", "ヨノワール", "ユキメノコ", "ロトム",
  "ユクシー", "エムリット", "アグノム", "ディアルガ",
  "パルキア", "ヒードラン", "レジギガス", "ギラティナ",
  "クレセリア", "フィオネ", "マナフィ", "ダークライ",
  "シェイミ", "アルセウス", "ビクティニ", "ツタージャ",
  "ジャノビー", "ジャローダ", "ポカブ", "チャオブー",
  "エンブオー", "ミジュマル", "フタチマル", "ダイケンキ",
  "ミネズミ", "ミルホッグ", "ヨーテリー", "ハーデリア",
  "ムーランド", "チョロネコ", "レパルダス", "ヤナップ",
  "ヤナッキー", "バオップ", "バオッキー", "ヒヤップ",
  "ヒヤッキー", "ムンナ", "ムシャーナ", "マメパト",
  "ハトーボー", "ケンホロウ", "シママ", "ゼブライカ",
  "ダンゴロ", "ガントル", "ギガイアス", "コロモリ",
  "ココロモリ", "モグリュー", "ドリュウズ", "タブンネ",
  "ドッコラー", "ドテッコツ", "ローブシン", "オタマロ",
  "ガマガル", "ガマゲロゲ", "ナゲキ", "ダゲキ",
  "クルミル", "クルマユ", "ハハコモリ", "フシデ",
  "ホイーガ", "ペンドラー", "モンメン", "エルフーン",
  "チュリネ", "ドレディア", "バスラオ", "メグロコ",
  "ワルビル", "ワルビアル", "ダルマッカ", "ヒヒダルマ",
  "マラカッチ", "イシズマイ", "イワパレス", "ズルッグ",
  "ズルズキン", "シンボラー", "デスマス", "デスカーン",
  "プロトーガ", "アバゴーラ", "アーケン", "アーケオス",
  "ヤブクロン", "ダストダス", "ゾロア", "ゾロアーク",
  "チラーミィ", "チラチーノ", "ゴチム", "ゴチミル",
  "ゴチルゼル", "ユニラン", "ダブラン", "ランクルス",
  "コアルヒー", "スワンナ", "バニプッチ", "バニリッチ",
  "バイバニラ", "シキジカ", "メブキジカ", "エモンガ",
  "カブルモ", "シュバルゴ", "タマゲタケ", "モロバレル",
  "プルリル", "ブルンゲル", "ママンボウ", "バチュル",
  "デンチュラ", "テッシード", "ナットレイ", "ギアル",
  "ギギアル", "ギギギアル", "シビシラス", "シビビール",
  "シビルドン", "リグレー", "オーベム", "ヒトモシ",
  "ランプラー", "シャンデラ", "キバゴ", "オノンド",
  "オノノクス", "クマシュン", "ツンベアー", "フリージオ",
  "チョボマキ", "アギルダー", "マッギョ", "コジョフー",
  "コジョンド", "クリムガン", "ゴビット", "ゴルーグ",
  "コマタナ", "キリキザン", "バッフロン", "ワシボン",
  "ウォーグル", "バルチャイ", "バルジーナ", "クイタラン",
  "アイアント", "モノズ", "ジヘッド", "サザンドラ",
  "メラルバ", "ウルガモス", "コバルオン", "テラキオン",
  "ビリジオン", "トルネロス", "ボルトロス", "レシラム",
  "ゼクロム", "ランドロス", "キュレム", "ケルディオ",
  "メロエッタ", "ゲノセクト", "ハリマロン", "ハリボーグ",
  "ブリガロン", "フォッコ", "テールナー", "マフォクシー",
  "ケロマツ", "ゲコガシラ", "ゲッコウガ", "ホルビー",
  "ホルード", "ヤヤコマ", "ヒノヤコマ", "ファイアロー",
  "コフキムシ", "コフーライ", "ビビヨン", "シシコ",
  "カエンジシ", "フラベベ", "フラエッテ", "フラージェス",
  "メェークル", "ゴーゴート", "ヤンチャム", "ゴロンダ",
  "トリミアン", "ニャスパー", "ニャオニクス", "ヒトツキ",
  "ニダンギル", "ギルガルド", "シュシュプ", "フレフワン",
  "ペロッパフ", "ペロリーム", "マーイーカ", "カラマネロ",
  "カメテテ", "ガメノデス", "クズモー", "ドラミドロ",
  "ウデッポウ", "ブロスター", "エリキテル", "エレザード",
  "チゴラス", "ガチゴラス", "アマルス", "アマルルガ",
  "ニンフィア", "ルチャブル", "デデンネ", "メレシー",
  "ヌメラ", "ヌメイル", "ヌメルゴン", "クレッフィ",
  "ボクレー", "オーロット", "バケッチャ", "パンプジン",
  "カチコール", "クレベース", "オンバット", "オンバーン",
  "ゼルネアス", "イベルタル", "ジガルデ", "ディアンシー",
  "フーパ", "ボルケニオン", "モクロー", "フクスロー",
  "ジュナイパー", "ニャビー", "ニャヒート", "ガオガエン",
  "アシマリ", "オシャマリ", "アシレーヌ", "ツツケラ",
  "ケララッパ", "ドデカバシ", "ヤングース", "デカグース",
  "アゴジムシ", "デンヂムシ", "クワガノン", "マケンカニ",
  "ケケンカニ", "オドリドリ", "アブリー", "アブリボン",
  "イワンコ", "ルガルガン", "ヨワシ", "ヒドイデ",
  "ドヒドイデ", "ドロバンコ", "バンバドロ", "シズクモ",
  "オニシズクモ", "カリキリ", "ラランテス", "ネマシュ",
  "マシェード", "ヤトウモリ", "エンニュート", "ヌイコグマ",
  "キテルグマ", "アマカジ", "アママイコ", "アマージョ",
  "キュワワー", "ヤレユータン", "ナゲツケサル", "コソクムシ",
  "グソクムシャ", "スナバァ", "シロデスナ", "ナマコブシ",
  "タイプ：ヌル", "シルヴァディ", "メテノ", "ネッコアラ",
  "バクガメス", "トゲデマル", "ミミッキュ", "ハギギシリ",
  "ジジーロン", "ダダリン", "ジャラコ", "ジャランゴ",
  "ジャラランガ", "カプ・コケコ", "カプ・テテフ", "カプ・ブルル",
  "カプ・レヒレ", "コスモッグ", "コスモウム", "ソルガレオ",
  "ルナアーラ", "ウツロイド", "マッシブーン", "フェローチェ",
  "デンジュモク", "テッカグヤ", "カミツルギ", "アクジキング",
  "ネクロズマ", "マギアナ", "マーシャドー", "ベベノム",
  "アーゴヨン", "ツンデツンデ", "ズガドーン", "ゼラオラ",
  "メルタン", "メルメタル", "サルノリ", "バチンキー",
  "ゴリランダー", "ヒバニー", "ラビフット", "エースバーン",
  "メッソン", "ジメレオン", "インテレオン", "ホシガリス",
  "ヨクバリス", "ココガラ", "アオガラス", "アーマーガア",
  "サッチムシ", "レドームシ", "イオルブ", "クスネ",
  "フォクスライ", "ヒメンカ", "ワタシラガ", "ウールー",
  "バイウールー", "カムカメ", "カジリガメ", "ワンパチ",
  "パルスワン", "タンドン", "トロッゴン", "セキタンザン",
  "カジッチュ", "アップリュー", "タルップル", "スナヘビ",
  "サダイジャ", "ウッウ", "サシカマス", "カマスジョー",
  "エレズン", "ストリンダー", "ヤクデ", "マルヤクデ",
  "タタッコ", "オトスパス", "ヤバチャ", "ポットデス",
  "ミブリム", "テブリム", "ブリムオン", "ベロバー",
  "ギモー", "オーロンゲ", "タチフサグマ", "ニャイキング",
  "サニゴーン", "ネギガナイト", "バリコオル", "デスバーン",
  "マホミル", "マホイップ", "タイレーツ", "バチンウニ",
  "ユキハミ", "モスノウ", "イシヘンジン", "コオリッポ",
  "イエッサン", "モルペコ", "ゾウドウ", "ダイオウドウ",
  "パッチラゴン", "パッチルドン", "ウオノラゴン", "ウオチルドン",
  "ジュラルドン", "ドラメシヤ", "ドロンチ", "ドラパルト",
  "ザシアン", "ザマゼンタ", "ムゲンダイナ", "ダクマ",
  "ウーラオス", "ザルード", "レジエレキ", "レジドラゴ",
  "ブリザポス", "レイスポス", "バドレックス", "アヤシシ",
  "バサギリ", "ガチグマ", "イダイトウ", "オオニューラ",
  "ハリーマン", "ラブトロス", "ニャオハ", "ニャローテ",
  "マスカーニャ", "ホゲータ", "アチゲータ", "ラウドボーン",
  "クワッス", "ウェルカモ", "ウェーニバル", "グルトン",
  "パフュートン", "タマンチュラ", "ワナイダー", "マメバッタ",
  "エクスレッグ", "パモ", "パモット", "パーモット",
  "ワッカネズミ", "イッカネズミ", "パピモッチ", "バウッツェル",
  "ミニーブ", "オリーニョ", "オリーヴァ", "イキリンコ",
  "コジオ", "ジオヅム", "キョジオーン", "カルボウ",
  "グレンアルマ", "ソウブレイズ", "ズピカ", "ハラバリー",
  "カイデン", "タイカイデン", "オラチフ", "マフィティフ",
  "シルシュルー", "タギングル", "アノクサ", "アノホラグサ",
  "ノノクラゲ", "リククラゲ", "ガケガニ", "カプサイジ",
  "スコヴィラン", "シガロコ", "ベラカス", "ヒラヒナ",
  "クエスパトラ", "カヌチャン", "ナカヌチャン", "デカヌチャン",
  "ウミディグダ", "ウミトリオ", "オトシドリ", "ナミイルカ",
  "イルカマン", "ブロロン", "ブロロローム", "モトトカゲ",
  "ミミズズ", "キラーメ", "キラフロル", "ボチ",
  "ハカドッグ", "カラミンゴ", "アルクジラ", "ハルクジラ",
  "ミガルーサ", "ヘイラッシャ", "シャリタツ", "コノヨザル",
  "ドオー", "リキキリン", "ノココッチ", "ドドゲザン",
  "イダイナキバ", "サケブシッポ", "アラブルタケ", "ハバタクカミ",
  "チヲハウハネ", "スナノケガワ", "テツノワダチ", "テツノツツミ",
  "テツノカイナ", "テツノコウベ", "テツノドクガ", "テツノイバラ",
  "セビエ", "セゴール", "セグレイブ", "コレクレー",
  "サーフゴー", "チオンジェン", "パオジアン", "ディンルー",
  "イーユイ", "トドロクツキ", "テツノブジン", "コライドン",
  "ミライドン", "ウネルミナモ", "テツノイサハ", "カミッチュ",
  "チャデス", "ヤバソチャ", "イイネイヌ", "マシマシラ",
  "キチキギス", "オーガポン", "ブリジュラス", "カミツオロチ",
  "ウガツホムラ", "タケルライコ", "テツノイワオ", "テツノカシラ",
  "テラパゴス", "モモワロウ",
};

static const char *const DEX_NAME_KO[DEX_COUNT + 1] = {
  nullptr, "이상해씨", "이상해풀", "이상해꽃",
  "파이리", "리자드", "리자몽", "꼬부기",
  "어니부기", "거북왕", "캐터피", "단데기",
  "버터플", "뿔충이", "딱충이", "독침붕",
  "구구", "피죤", "피죤투", "꼬렛",
  "레트라", "깨비참", "깨비드릴조", "아보",
  "아보크", "피카츄", "라이츄", "모래두지",
  "고지", "니드런 암", "니드리나", "니드퀸",
  "니드런 수", "니드리노", "니드킹", "삐삐",
  "픽시", "식스테일", "나인테일", "푸린",
  "푸크린", "주뱃", "골뱃", "뚜벅쵸",
  "냄새꼬", "라플레시아", "파라스", "파라섹트",
  "콘팡", "도나리", "디그다", "닥트리오",
  "나옹", "페르시온", "고라파덕", "골덕",
  "망키", "성원숭", "가디", "윈디",
  "발챙이", "슈륙챙이", "강챙이", "캐이시",
  "윤겔라", "후딘", "알통몬", "근육몬",
  "괴력몬", "모다피", "우츠동", "우츠보트",
  "왕눈해", "독파리", "꼬마돌", "데구리",
  "딱구리", "포니타", "날쌩마", "야돈",
  "야도란", "코일", "레어코일", "파오리",
  "두두", "두트리오", "쥬쥬", "쥬레곤",
  "질퍽이", "질뻐기", "셀러", "파르셀",
  "고오스", "고우스트", "팬텀", "롱스톤",
  "슬리프", "슬리퍼", "크랩", "킹크랩",
  "찌리리공", "붐볼", "아라리", "나시",
  "탕구리", "텅구리", "시라소몬", "홍수몬",
  "내루미", "또가스", "또도가스", "뿔카노",
  "코뿌리", "럭키", "덩쿠리", "캥카",
  "쏘드라", "시드라", "콘치", "왕콘치",
  "별가사리", "아쿠스타", "마임맨", "스라크",
  "루주라", "에레브", "마그마", "쁘사이저",
  "켄타로스", "잉어킹", "갸라도스", "라프라스",
  "메타몽", "이브이", "샤미드", "쥬피썬더",
  "부스터", "폴리곤", "암나이트", "암스타",
  "투구", "투구푸스", "프테라", "잠만보",
  "프리져", "썬더", "파이어", "미뇽",
  "신뇽", "망나뇽", "뮤츠", "뮤",
  "치코리타", "베이리프", "메가니움", "브케인",
  "마그케인", "블레이범", "리아코", "엘리게이",
  "장크로다일", "꼬리선", "다꼬리", "부우부",
  "야부엉", "레디바", "레디안", "페이검",
  "아리아도스", "크로뱃", "초라기", "랜턴",
  "피츄", "삐", "푸푸린", "토게피",
  "토게틱", "네이티", "네이티오", "메리프",
  "보송송", "전룡", "아르코", "마릴",
  "마릴리", "꼬지모", "왕구리", "통통코",
  "두코", "솜솜코", "에이팜", "해너츠",
  "해루미", "왕자리", "우파", "누오",
  "에브이", "블래키", "니로우", "야도킹",
  "무우마", "안농", "마자용", "키링키",
  "피콘", "쏘콘", "노고치", "글라이거",
  "강철톤", "블루", "그랑블루", "침바루",
  "핫삼", "단단지", "헤라크로스", "포푸니",
  "깜지곰", "링곰", "마그마그", "마그카르고",
  "꾸꾸리", "메꾸리", "코산호", "총어",
  "대포무노", "딜리버드", "만타인", "무장조",
  "델빌", "헬가", "킹드라", "코코리",
  "코리갑", "폴리곤2", "노라키", "루브도",
  "배루키", "카포에라", "뽀뽀라", "에레키드",
  "마그비", "밀탱크", "해피너스", "라이코",
  "앤테이", "스이쿤", "애버라스", "데기라스",
  "마기라스", "루기아", "칠색조", "세레비",
  "나무지기", "나무돌이", "나무킹", "아차모",
  "영치코", "번치코", "물짱이", "늪짱이",
  "대짱이", "포챠나", "그라에나", "지그제구리",
  "직구리", "개무소", "실쿤", "뷰티플라이",
  "카스쿤", "독케일", "연꽃몬", "로토스",
  "로파파", "도토링", "잎새코", "다탱구",
  "테일로", "스왈로", "갈모매", "패리퍼",
  "랄토스", "킬리아", "가디안", "비구술",
  "비나방", "버섯꼬", "버섯모", "게을로",
  "발바로", "게을킹", "토중몬", "아이스크",
  "껍질몬", "소곤룡", "노공룡", "폭음룡",
  "마크탕", "하리뭉", "루리리", "코코파스",
  "에나비", "델케티", "깜까미", "입치트",
  "가보리", "갱도라", "보스로라", "요가랑",
  "요가램", "썬더라이", "썬더볼트", "플러시",
  "마이농", "볼비트", "네오비트", "로젤리아",
  "꼴깍몬", "꿀꺽몬", "샤프니아", "샤크니아",
  "고래왕자", "고래왕", "둔타", "폭타",
  "코터스", "피그점프", "피그킹", "얼루기",
  "톱치", "비브라바", "플라이곤", "선인왕",
  "밤선인", "파비코", "파비코리", "쟝고",
  "세비퍼", "루나톤", "솔록", "미꾸리",
  "메깅", "가재군", "가재장군", "오뚝군",
  "점토도리", "릴링", "릴리요", "아노딥스",
  "아말도", "빈티나", "밀로틱", "캐스퐁",
  "켈리몬", "어둠대신", "다크펫", "해골몽",
  "미라몽", "트로피우스", "치렁", "앱솔",
  "마자", "눈꼬마", "얼음귀신", "대굴레오",
  "씨레오", "씨카이저", "진주몽", "헌테일",
  "분홍장이", "시라칸", "사랑동이", "아공이",
  "쉘곤", "보만다", "메탕", "메탕구",
  "메타그로스", "레지락", "레지아이스", "레지스틸",
  "라티아스", "라티오스", "가이오가", "그란돈",
  "레쿠쟈", "지라치", "테오키스", "모부기",
  "수풀부기", "토대부기", "불꽃숭이", "파이숭이",
  "초염몽", "팽도리", "팽태자", "엠페르트",
  "찌르꼬", "찌르버드", "찌르호크", "비버니",
  "비버통", "귀뚤뚜기", "귀뚤톡크", "꼬링크",
  "럭시오", "렌트라", "꼬몽울", "로즈레이드",
  "두개도스", "램펄드", "방패톱스", "바리톱스",
  "도롱충이", "도롱마담", "나메일", "세꿀버리",
  "비퀸", "파치리스", "브이젤", "플로젤",
  "체리버", "체리꼬", "깝질무", "트리토돈",
  "겟핸보숭", "흔들풍손", "둥실라이드", "이어롤",
  "이어롭", "무우마직", "돈크로우", "나옹마",
  "몬냥이", "랑딸랑", "스컹뿡", "스컹탱크",
  "동미러", "동탁군", "꼬지지", "흉내내",
  "핑복", "페라페", "화강돌", "딥상어동",
  "한바이트", "한카리아스", "먹고자", "리오르",
  "루카리오", "히포포타스", "하마돈", "스콜피",
  "드래피온", "삐딱구리", "독개굴", "무스틈니",
  "형광어", "네오라이트", "타만타", "눈쓰개",
  "눈설왕", "포푸니라", "자포코일", "내룸벨트",
  "거대코뿌리", "덩쿠림보", "에레키블", "마그마번",
  "토게키스", "메가자리", "리피아", "글레이시아",
  "글라이온", "맘모꾸리", "폴리곤Z", "엘레이드",
  "대코파스", "야느와르몽", "눈여아", "로토무",
  "유크시", "엠라이트", "아그놈", "디아루가",
  "펄기아", "히드런", "레지기가스", "기라티나",
  "크레세리아", "피오네", "마나피", "다크라이",
  "쉐이미", "아르세우스", "비크티니", "주리비얀",
  "샤비", "샤로다", "뚜꾸리", "차오꿀",
  "염무왕", "수댕이", "쌍검자비", "대검귀",
  "보르쥐", "보르그", "요테리", "하데리어",
  "바랜드", "쌔비냥", "레파르다스", "야나프",
  "야나키", "바오프", "바오키", "앗차프",
  "앗차키", "몽나", "몽얌나", "콩둘기",
  "유토브", "켄호로우", "줄뮤마", "제브라이카",
  "단굴", "암트르", "기가이어스", "또르박쥐",
  "맘박쥐", "두더류", "몰드류", "다부니",
  "으랏차", "토쇠골", "노보청", "동챙이",
  "두까비", "두빅굴", "던지미", "타격귀",
  "두르보", "두르쿤", "모아머", "마디네",
  "휠구", "펜드라", "소미안", "엘풍",
  "치릴리", "드레디어", "배쓰나이", "깜눈크",
  "악비르", "악비아르", "달막화", "불비달마",
  "마라카치", "돌살이", "암팰리스", "곤율랭",
  "곤율거니", "심보러", "데스마스", "데스니칸",
  "프로토가", "늑골라", "아켄", "아케오스",
  "깨봉이", "더스트나", "조로아", "조로아크",
  "치라미", "치라치노", "고디탱", "고디보미",
  "고디모아젤", "유니란", "듀란", "란쿨루스",
  "꼬지보리", "스완나", "바닐프티", "바닐리치",
  "배바닐라", "사철록", "바라철록", "에몽가",
  "딱정곤", "슈바르고", "깜놀버슬", "뽀록나",
  "탱그릴", "탱탱겔", "맘복치", "파쪼옥",
  "전툴라", "철시드", "너트령", "기어르",
  "기기어르", "기기기어르", "저리어", "저리릴",
  "저리더프", "리그레", "벰크", "불켜미",
  "램프라", "샹델라", "터검니", "액슨도",
  "액스라이즈", "코고미", "툰베어", "프리지오",
  "쪼마리", "어지리더", "메더", "비조푸",
  "비조도", "크리만", "골비람", "골루그",
  "자망칼", "절각참", "버프론", "수리둥보",
  "워글", "벌차이", "버랜지나", "앤티골",
  "아이앤트", "모노두", "디헤드", "삼삼드래",
  "활화르바", "불카모스", "코바르온", "테라키온",
  "비리디온", "토네로스", "볼트로스", "레시라무",
  "제크로무", "랜드로스", "큐레무", "케르디오",
  "메로엣타", "게노세크트", "도치마론", "도치보구",
  "브리가론", "푸호꼬", "테르나", "마폭시",
  "개구마르", "개굴반장", "개굴닌자", "파르빗",
  "파르토", "화살꼬빈", "불화살빈", "파이어로",
  "분이벌레", "분떠도리", "비비용", "레오꼬",
  "화염레오", "플라베베", "플라엣테", "플라제스",
  "메이클", "고고트", "판짱", "부란다",
  "트리미앙", "냐스퍼", "냐오닉스", "단칼빙",
  "쌍검킬", "킬가르도", "슈쁘", "프레프티르",
  "나룸퍼프", "나루림", "오케이징", "칼라마네로",
  "거북손손", "거북손데스", "수레기", "드래캄",
  "완철포", "블로스터", "목도리키텔", "일레도리자드",
  "티고라스", "견고라스", "아마루스", "아마루르가",
  "님피아", "루차불", "데덴네", "멜리시",
  "미끄메라", "미끄네일", "미끄래곤", "클레피",
  "나목령", "대로트", "호바귀", "펌킨인",
  "꽁어름", "크레베이스", "음뱃", "음번",
  "제르네아스", "이벨타르", "지가르데", "디안시",
  "후파", "볼케니온", "나몰빼미", "빼미스로우",
  "모크나이퍼", "냐오불", "냐오히트", "어흥염",
  "누리공", "키요공", "누리레느", "콕코구리",
  "크라파", "왕큰부리", "영구스", "형사구스",
  "턱지충이", "전지충이", "투구뿌논", "오기지게",
  "모단단게", "춤추새", "에블리", "에리본",
  "암멍이", "루가루암", "약어리", "시마사리",
  "더시마사리", "머드나기", "만마드", "물거미",
  "깨비물거미", "짜랑랑", "라란티스", "자마슈",
  "마셰이드", "야도뇽", "염뉴트", "포곰곰",
  "이븐곰", "달콤아", "달무리나", "달코퀸",
  "큐아링", "하랑우탄", "내던숭이", "꼬시레",
  "갑주무사", "모래꿍", "모래성이당", "해무기",
  "타입:널", "실버디", "메테노", "자말라",
  "폭거북스", "토게데마루", "따라큐", "치갈기",
  "할비롱", "타타륜", "짜랑꼬", "짜랑고우",
  "짜랑고우거", "카푸꼬꼬꼭", "카푸나비나", "카푸브루루",
  "카푸느지느", "코스모그", "코스모움", "솔가레오",
  "루나아라", "텅비드", "매시붕", "페로코체",
  "전수목", "철화구야", "종이신도", "악식킹",
  "네크로즈마", "마기아나", "마샤도", "베베놈",
  "아고용", "차곡차곡", "두파팡", "제라오라",
  "멜탄", "멜메탈", "흥나숭", "채키몽",
  "고릴타", "염버니", "래비풋", "에이스번",
  "울머기", "누겔레온", "인텔리레온", "탐리스",
  "요씽리스", "파라꼬", "파크로우", "아머까오",
  "두루지벌레", "레돔벌레", "이올브", "훔처우",
  "폭슬라이", "꼬모카", "백솜모카", "우르",
  "배우르", "깨물부기", "갈가부기", "멍파치",
  "펄스멍", "탄동", "탄차곤", "석탄산",
  "과사삭벌레", "애프룡", "단지래플", "모래뱀",
  "사다이사", "윽우지", "찌로꼬치", "꼬치조",
  "일레즌", "스트린더", "태우지네", "다태우지네",
  "때때무노", "케오퍼스", "데인차", "포트데스",
  "몸지브림", "손지브림", "브리무음", "메롱꿍",
  "쏘겨모", "오롱털", "가로막구리", "나이킹",
  "산호르곤", "창파나이트", "마임꽁꽁", "데스판",
  "마빌크", "마휘핑", "대여르", "찌르성게",
  "누니머기", "모스노우", "돌헨진", "빙큐보",
  "에써르", "모르페코", "끼리동", "대왕끼리동",
  "파치래곤", "파치르돈", "어래곤", "어치르돈",
  "두랄루돈", "드라꼰", "드래런치", "드래펄트",
  "자시안", "자마젠타", "무한다이노", "치고마",
  "우라오스", "자루도", "레지에레키", "레지드래고",
  "블리자포스", "레이스포스", "버드렉스", "신비록",
  "사마자르", "다투곰", "대쓰여너", "포푸니크",
  "장침바루", "러브로스", "나오하", "나로테",
  "마스카나", "뜨아거", "악뜨거", "라우드본",
  "꾸왁스", "아꾸왁", "웨이니발", "맛보돈",
  "퍼퓨돈", "타랜툴라", "트래피더", "콩알뚜기",
  "엑스레그", "빠모", "빠모트", "빠르모트",
  "두리쥐", "파밀리쥐", "쫀도기", "바우첼",
  "미니브", "올리뇨", "올리르바", "시비꼬",
  "베베솔트", "스태솔트", "콜로솔트", "카르본",
  "카디나르마", "파라블레이즈", "빈나두", "찌리배리",
  "찌리비", "찌리비크", "오라티프", "마피티프",
  "땃쭈르", "태깅구르", "그푸리", "공푸리",
  "들눈해", "육파리", "절벼게", "캡싸이",
  "스코빌런", "구르데", "베라카스", "하느라기",
  "클레스퍼트라", "어리짱", "벼리짱", "두드리짱",
  "바다그다", "바닥트리오", "떨구새", "맨돌핀",
  "돌핀맨", "부르롱", "부르르룸", "모토마",
  "꿈트렁", "초롱순", "킬라플로르", "망망이",
  "묘두기", "꼬이밍고", "터벅고래", "우락고래",
  "가비루사", "어써러셔", "싸리용", "저승갓숭",
  "토오", "키키링", "노고고치", "대도각참",
  "위대한엄니", "우렁찬꼬리", "사나운버섯", "날개치는머리",
  "땅을기는날개", "모래털가죽", "무쇠바퀴", "무쇠보따리",
  "무쇠손", "무쇠머리", "무쇠독나방", "무쇠가시",
  "드니차", "드니꽁", "드닐레이브", "모으령",
  "타부자고", "총지엔", "파오젠", "딩루",
  "위유이", "고동치는달", "무쇠무인", "코라이돈",
  "미라이돈", "굽이치는물결", "무쇠잎새", "과미르",
  "차데스", "그우린차", "조타구", "이야후",
  "기로치", "오거폰", "브리두라스", "과미드라",
  "꿰뚫는화염", "날뛰는우레", "무쇠암석", "무쇠감투",
  "테라파고스", "복숭악동",
};

// Nombre de la especie en el idioma activo (cae al de DEX_TBL si ese
// idioma no tiene nombre propio para ella).
static inline const char *dexName(int16_t dex) {
  if (dex < 1 || dex > DEX_COUNT) return DEX_TBL[0].name;
  // Nidoran: a fonte classica (CP437) TEM femea e macho nos codigos 12
  // e 11 (octal 014 e 013), e o Arduino_GFX so trata a quebra de linha e
  // o retorno como controle: os dois se desenham. Nas latinas sai o
  // simbolo em vez do H/M do espanhol; JA/KO (fontes U8g2 sem eles)
  // ficam com as tabelas deles (F/M e am/su). DEX_TBL segue com o nome
  // de sempre: as ferramentas leem dex.h e casam por ele.
  if (!LANG_IS_CJK(gLang) && (dex == 29 || dex == 32))
    return dex == 29 ? "NIDORAN\014" : "NIDORAN\013";
  const char *n = (gLang == LANG_FR)   ? DEX_NAME_FR[dex]
                  : (gLang == LANG_DE) ? DEX_NAME_DE[dex]
                  : (gLang == LANG_JA) ? DEX_NAME_JA[dex]
                  : (gLang == LANG_KO) ? DEX_NAME_KO[dex]
                                       : nullptr;
  return n ? n : DEX_TBL[dex].name;
}

// el primer huevo de la partida: iniciales clasicos
static const int16_t CLASSIC_DEX[] = { 1, 4, 7, 25, 133 };
#define NUM_CLASSIC_DEX 5
