// Los gimnasios, el Alto Mando y el campeon. Escrito a mano y no generado:
// son catorce filas que hay que curar una a una.
//
// LOS EQUIPOS SON LOS DEL ANIL, sacados de su PBS/trainers.txt (tools/,
// lee_lideres), no los de la gen 1 de memoria. El campeon es AZUL, tu rival,
// con un Charizard.
//
// La escalera es la CLASICA de Kanto y acaba en Giovanni. El Anil pone en su
// octavo gimnasio a un lider propio (Urano), pero es de su historia y no de
// Kanto, asi que fuera; Giovanni si esta en el juego, como jefe de Rocket, y
// de su primer combate sale justo el Rhydon del gimnasio de Viridian.
//
// De cada equipo se coge el bicho de mas nivel QUE EXISTA en nuestras 151:
// media plantilla del Anil es de generaciones posteriores (Crobat, Houndoom,
// Tyranitar...). El motor pelea 1v1, asi que un lider es su mejor bicho.
#pragma once
#include <stdint.h>
#include "dex.h"
#include "stones.h"   // T_SUN: el premio de Giovanni no es un tipo
#include "trainers.h" // las caras de los tres de la casa salen de ahi

// LOS TRES DE LA CASA. Antes del lider hay que ganarles a los tres, que es
// lo que hace que un gimnasio sea un gimnasio.
//
// Esto SUSTITUYE al porton de antes (cinco victorias contra salvajes).
// Moler hierba no abria un gimnasio: abria el reloj. Los tres de la casa
// pelean al nivel del lider, asi que el porton mide lo que tiene que
// medir —— si llegas o no llegas.
#define GYM_TRAINERS 3

// Quien es el i-esimo de la casa del gimnasio G. Cuenta fija sobre los
// ocho de a pie: el primero de Pewter es SIEMPRE el mismo y se reconoce
// al volver, sin una tabla de 13x3 que curar a mano.
static inline uint8_t gymTrainerIdx(uint8_t gym, uint8_t i) {
  return (uint8_t)(((uint16_t)gym * GYM_TRAINERS + i) % TRAINER_COUNT);
}

struct GymEntry {
  const char *leader;   // nombre propio: NO se traduce
  const char *place;    // sala; tampoco
  uint8_t type;         // T_*, el tipo de la casa
  // Su equipo, con el AS EL ULTIMO: se pelea en orden, primero los de
  // apoyo. 0 = hueco vacio. Hasta hace poco era un solo bicho porque el
  // motor solo sabia 1v1; ahora que un entrenador cualquiera saca tres,
  // que un lider de gimnasio sacara uno era lo raro.
  int16_t team[3];
  // El nivel del AS. Los de delante van de GYM_TEAM_STEP en GYM_TEAM_STEP
  // por debajo, que es como se reparte en los juegos.
  uint8_t level;
  // El premio. Es el TIPO de piedra que suelta (T_NONE = el Cable), o
  // GP_BALLS si no le toca piedra. Se nombra por tipo y no por indice de
  // objeto por la misma razon que en todos lados: items.h es generado.
  //
  // Esto es lo que pone las piedras en circulacion. Sin una via de
  // conseguirlas, quien evoluciona con piedra se quedaba sin evolucionar
  // NUNCA, que es peor que como estaba antes.
  int8_t prize;
};
#define GP_BALLS (-1)

// TECHO DE NIVEL. Sin el, llegar a Johto con un Charizard de 70 convertiria
// su primer gimnasio en un paseo, y lo mismo pasa dentro de una region si
// alguien se pone a moler salvajes antes de Brock.
//
// El techo es el nivel del bicho del PROXIMO desafio mas un margen: puedes
// ir un poco por encima del lider, no diez niveles por encima.
//
// Quien lo pasa NO pierde nada: conserva su nivel y su EXP, pero PELEA al
// techo (Pet::battleLevel). Eso es lo que deja traerse los de Kanto a Johto
// y que sigan sirviendo sin arrasar la region nueva.
#define GYM_CAP_SLACK 3

// Desde que desafio puede el rival MEGAEVOLUCIONAR.
//
// El 8: los ocho lideres son los indices 0..7, la Elite 4 va del 8 al 11
// y el campeon es el 12. O sea que la mega del rival queda SOLO para la
// Elite 4 y el campeon.
//
// Antes estaba en 4 (la mitad del camino de la region). Es decision del
// usuario, y la razon es buena: si un lider cualquiera megaevoluciona,
// al ultimo tramo ya no le queda nada que ensenar —— y ese es justo el
// que tiene que doler. Nunca se llego a notar en el aparato, porque la
// piedra no se daba por un fallo aparte (ver btFoeItem en battle_ui.ino).
#define MEGA_DESDE_GIMNASIO 8

#define GYM_TOTAL 13

// Los NIVELES estan comprimidos. Los del Anil van de 14 a 75 y ahi no llega
// nadie: con esta curva de EXP, el nivel 50 son cientos de combates y el 75
// no se alcanza jugando. Se mantiene la forma (cada uno por encima del
// anterior) sobre un recorrido que si se puede andar.
static const GymEntry KANTO_GYMS[GYM_TOTAL] = {
  // Los premios reparten las CINCO piedras y el Cable a lo largo de la
  // region, en el orden en que hacen falta: la de agua pronto (Poliwhirl,
  // Shellder, Staryu son bichos de principio) y el Cable tarde, que va con
  // Kadabra y Machoke.
  // Los equipos salen del PBS del Anil, filtrados a nuestras 151: media
  // plantilla suya es de generaciones posteriores (Crobat, Steelix,
  // Rhyperior...). A los tres que se quedaban en UN solo bicho de gen 1
  // (Erika, Sabrina, Agatha) se les anade uno de su tipo que SI tienen en
  // los juegos originales, para que un lider no salga mas flojo que un
  // excursionista cualquiera.
  { "BROCK",    "PEWTER",    T_ROCK,    { 138,  74,  95 },  8, GP_BALLS },
  { "MISTY",    "CERULEAN",  T_WATER,   { 118,  55, 121 }, 13, T_WATER  },
  { "SURGE",    "VERMILION", T_ELEC,    { 101,  82,  26 }, 18, T_ELEC   },
  { "ERIKA",    "CELADON",   T_GRASS,   {   0,  71,  45 }, 24, T_GRASS  },
  { "KOGA",     "FUCHSIA",   T_POISON,  {  73,  89,  15 }, 30, T_FAIRY  },
  { "SABRINA",  "SAFFRON",   T_PSYCHIC, {   0, 122,  65 }, 35, T_NONE   },
  { "BLAINE",   "CINNABAR",  T_FIRE,    {   0,  38,  59 }, 39, T_FIRE   },
  // La ultima de Kanto paga la Sun Stone. Antes no la soltaba nadie porque
  // no servia para nada (compartia clave con la de fuego); ahora abre
  // Bellossom y Espeon, y ya en Johto, Sunflora: justo despues.
  { "GIOVANNI", "VIRIDIAN",  T_GROUND,  {  34, 115, 112 }, 44, T_SUN    },
  { "LORELEI",  "ELITE 4",   T_ICE,     {  38, 124, 131 }, 47, GP_BALLS },
  { "BRUNO",    "ELITE 4",   T_FIGHT,   { 106,  62,  68 }, 49, GP_BALLS },
  { "AGATHA",   "ELITE 4",   T_GHOST,   {  42,  93,  94 }, 51, GP_BALLS },
  { "LANCE",    "ELITE 4",   T_DRAGON,  { 142, 103, 149 }, 54, GP_BALLS },
  { "AZUL",     "CHAMPION",  T_FIRE,    {  18,  65,   6 }, 58, GP_BALLS },
};

// Cuantos bichos saca, y cual es el n-esimo (el as es el ultimo).
#define GYM_TEAM_STEP 3
static inline uint8_t gymTeamSize(const GymEntry &g) {
  uint8_t n = 0;
  for (uint8_t i = 0; i < 3; i++) if (g.team[i]) n++;
  return n ? n : 1;
}
// Los huecos vacios van DELANTE, asi que se salta lo que sea 0.
static inline int16_t gymTeamAt(const GymEntry &g, uint8_t i) {
  uint8_t k = 0;
  for (uint8_t j = 0; j < 3; j++) {
    if (!g.team[j]) continue;
    if (k++ == i) return g.team[j];
  }
  return g.team[2];
}
// El as es el ultimo y lleva el nivel de la fila; los de delante bajan.
static inline uint8_t gymLevelAt(const GymEntry &g, uint8_t i) {
  uint8_t n = gymTeamSize(g);
  uint8_t baja = (uint8_t)((n - 1 - (i < n ? i : n - 1)) * GYM_TEAM_STEP);
  return g.level > baja + 1 ? (uint8_t)(g.level - baja) : 1;
}

// A que nivel pelea el i-esimo de la casa: por debajo del AS del lider y
// subiendo de dos en dos. El tercero se queda a dos del as, que es el
// aviso de que ya toca el de verdad.
//
// La referencia es el AS (`g.level`) y no el primero de su equipo: ese
// ya viene rebajado por gymLevelAt, y en las casas de nivel bajo (Brock
// lleva 8, su primero es 2) restarle otros seis dejaba a los tres en el
// suelo —— los tres a Lv.1, visto en captura.
static inline uint8_t gymTrainerLevel(const GymEntry &g, uint8_t i) {
  uint8_t baja = (uint8_t)((GYM_TRAINERS - (i < GYM_TRAINERS ? i : 0)) * 2);
  return g.level > baja ? (uint8_t)(g.level - baja) : 1;
}

// Cuantos dan insignia: los ocho primeros. El Alto Mando y el campeon son
// titulos, no insignias, pero siguen abriendo especies porque `badges` es lo
// que mueve el techo (ver WILD_BST_CAP).
#define GYM_BADGES 8

// REGIONES. Hoy solo hay Kanto, pero la costura va puesta desde ya porque
// anadir Johto tiene que ser una FILA de datos y no una reforma: el techo de
// nivel, el porton y la pagina de gimnasios salen todos de aqui.
//
// Al cambiar de region el contador de insignias vuelve a cero, y con el el
// techo de nivel: por eso el primer gimnasio de Johto se pelea al nivel del
// primero de Kanto aunque llegues con el equipo hecho.
struct Region {
  const char *name;
  const GymEntry *gyms;
  uint8_t count;
  uint8_t badges;   // cuantos de los `count` dan insignia
  // Que especies salen en la hierba segun las insignias que lleves: un
  // techo de suma de stats base por insignia. La ultima LEVANTA el techo
  // del todo en vez de subir un escalon mas.
  //
  // Es POR REGION y no una sola tabla, porque cada una reparte su BST de
  // otra forma: con la tabla unica que habia (los percentiles de Kanto),
  // la segunda insignia de Johto abria CINCO especies. Los numeros los
  // calcula tools/curva_bst.py, que hay que volver a correr cuando cambie
  // el catalogo o la fuente de stats.
  const uint16_t *bstCap;
  // Os TRES iniciais da regiao. Ao pisar nela pela primeira vez o
  // jogador ganha um ovo de um deles, igual ao comeco do jogo —— e
  // e isso que faz uma regiao nova comecar de baixo em vez de ser
  // atropelada pelo time que veio de tras.
  uint16_t starters[3];
};

// Percentiles reales de cada region (tools/curva_bst.py, con los stats del
// Anil y sin contar legendarios, que tienen su propia puerta). Abren de +9
// a +28 especies por insignia.
// (GYM_BADGES y no GYM_COUNT: son el mismo 8, pero GYM_COUNT lo define
// pet.h, que incluye a este y no al reves.)
static const uint16_t BST_KANTO[GYM_BADGES] = { 300, 318, 340, 405, 455, 485, 500, 9999 };
static const uint16_t BST_JOHTO[GYM_BADGES] = { 280, 305, 360, 405, 455, 465, 490, 9999 };
static const uint16_t BST_HOENN[GYM_BADGES] = { 275, 305, 335, 405, 450, 470, 490, 9999 };
static const uint16_t BST_SINNOH[GYM_BADGES] = { 300, 330, 350, 466, 486, 501, 515, 9999 };
static const uint16_t BST_UNOVA[GYM_BADGES]  = { 295, 315, 348, 401, 468, 488, 498, 9999 };
static const uint16_t BST_KALOS[GYM_BADGES]  = { 303, 320, 355, 405, 466, 494, 500, 9999 };
static const uint16_t BST_ALOLA[GYM_BADGES]  = { 280, 320, 385, 430, 478, 488, 500, 9999 };
static const uint16_t BST_GALAR[GYM_BADGES]  = { 265, 305, 370, 455, 483, 500, 510, 9999 };
static const uint16_t BST_PALDEA[GYM_BADGES] = { 280, 310, 380, 460, 491, 510, 530, 9999 };

// JOHTO. Los equipos NO salen del Anil: su PBS no tiene estos lideres (su
// octavo es Urano, de su propia historia, y de ahi para arriba va el Alto
// Mando de Kanto). Van escritos a mano con las plantillas de GSC/HGSS,
// filtradas a nuestras 251 y con el AS EL ULTIMO, igual que en Kanto.
//
// Koga y Bruno se repiten: en los juegos Koga asciende de gimnasio de
// Fucsia al Alto Mando de Johto y Bruno se queda. Es lo que pasa de verdad,
// asi que se deja; cada fila dice su sala, que es lo que los distingue.
//
// Los niveles siguen la misma compresion que Kanto, empezando por encima de
// donde acaba: al cambiar de region las insignias vuelven a cero, asi que
// el techo de nivel baja y el equipo de Kanto pelea a la altura de aqui.
static const GymEntry JOHTO_GYMS[GYM_TOTAL] = {
  // Aqui caen los CUATRO objetos de Johto. Kanto ya reparte las cinco
  // piedras, la de la luna y el Cable, asi que repetirlas aqui no anadia
  // nada: a Johto solo se llega coronando Kanto. Lo que no tenia via
  // ninguna eran los objetos nuevos, y sin ellos Steelix, Scizor, Kingdra,
  // Porygon2, Politoed y Slowking no salian de ningun lado.
  //
  // Cada uno con quien le toca, que es ademas donde estan en los juegos:
  // la Roca del Rey en Azalea (el pozo de los Slowpoke), la Mejora en
  // Ciudad Trigal, la Garra Metal en el gimnasio de acero (que saca un
  // Steelix) y la Escama Dragon con CLAIR (que saca un Kingdra).
  //                                      de apoyo ->  AS     nv  premio
  { "FALKNER",  "VIOLET",     T_FLYING,  {   0,  16,  17 }, 10, GP_BALLS },
  { "BUGSY",    "AZALEA",     T_BUG,     {  11,  14, 123 }, 15, EVOI_KINGSROCK   },
  { "WHITNEY",  "GOLDENROD",  T_NORMAL,  {   0,  35, 241 }, 20, EVOI_UPGRADE     },
  { "MORTY",    "ECRUTEAK",   T_GHOST,   {  92,  93,  94 }, 26, T_NONE   },
  { "CHUCK",    "CIANWOOD",   T_FIGHT,   {   0,  57,  62 }, 31, T_WATER  },
  { "JASMINE",  "OLIVINE",    T_STEEL,   {   0,  81, 208 }, 36, EVOI_METALCOAT   },
  { "PRYCE",    "MAHOGANY",   T_ICE,     {  86,  87, 221 }, 40, T_ELEC   },
  { "CLAIR",    "BLACKTHORN", T_DRAGON,  {   0, 148, 230 }, 45, EVOI_DRAGONSCALE },
  { "WILL",     "ELITE 4",    T_PSYCHIC, { 124, 103, 178 }, 48, GP_BALLS },
  { "KOGA",     "ELITE 4",    T_POISON,  { 168,  89, 169 }, 50, GP_BALLS },
  { "BRUNO",    "ELITE 4",    T_FIGHT,   { 237, 107,  68 }, 52, GP_BALLS },
  { "KAREN",    "ELITE 4",    T_DARK,    { 197,  94, 229 }, 55, GP_BALLS },
  { "LANCE",    "CHAMPION",   T_DRAGON,  { 130, 142, 149 }, 60, GP_BALLS },
};

// La tercera. Los OCHO primeros retratos salen del propio Anil
// (LIDER1HOENN..LIDER8HOENN); del Alto Mando de Hoenn no tiene ninguno, asi
// que esos cinco se bajan de Archives como se hizo con Johto.
//
// LA ORDEM E OS TIMES SAO OS DO ANIL (PBS trainers.txt, LIDERnHOENN), como em
// Kanto: o retrato n e o do LIDERnHOENN, entao a linha n TEM de ser esse lider.
// Estava na ordem da Esmeralda e o primeiro, "ROXANNE", saia com a cara do
// Norman (2026-11-09). tools/auditoria.py (lideres) confere contra o PBS.
// Filtrados as 386 como sempre em Hoenn; o AS e o de nivel mais alto que
// sobra (empate: o ultimo), os de apoio os primeiros na ordem do Anil.
// Tate e Liza pelean como uno: el motor saca tres bichos.
static const GymEntry HOENN_GYMS[GYM_TOTAL] = {
  // Los premios colocan los TRES objetos nuevos, cada uno donde le toca en
  // los juegos: el Diente Abisal se saca buceando (Marcial vive en una
  // isla), Feebas vive en la ruta de Ciudad Arborada (Alana) y la Escama
  // Abisal, en Arrecipolis, que esta literalmente dentro de un crater con
  // agua. Los otros cinco huecos repiten piedras de las regiones de antes
  // para quien llegue aqui sin ellas.
  // Cada premio vai com o SEU lider (o motivo de cada um, acima).
  //                                      de apoyo ->  AS     nv  premio
  { "NORMAN",   "PETALBURG",  T_NORMAL,  { 287, 300, 294 }, 12, T_NONE   },
  { "FLANNERY", "LAVARIDGE",  T_FIRE,    { 126, 219, 324 }, 17, T_FIRE   },
  { "WINONA",   "FORTREE",    T_FLYING,  { 357, 279, 334 }, 22, EVOI_PRISMSCALE },
  { "WATTSON",  "MAUVILLE",   T_ELEC,    {   0, 125, 310 }, 28, T_ELEC   },
  { "ROXANNE",  "RUSTBORO",   T_ROCK,    { 348, 346, 142 }, 33, GP_BALLS },
  { "BRAWLY",   "DEWFORD",    T_FIGHT,   {  68, 308, 297 }, 38, EVOI_SEATOOTH   },
  { "JUAN",     "SOOTOPOLIS", T_WATER,   { 272, 130, 350 }, 42, EVOI_SEASCALE   },
  { "TATE&LIZA","MOSSDEEP",   T_PSYCHIC, { 338, 337, 282 }, 47, T_SUN    },
  { "SIDNEY",   "ELITE 4",    T_DARK,    { 262, 332, 359 }, 50, GP_BALLS },
  { "PHOEBE",   "ELITE 4",    T_GHOST,   { 354, 302, 356 }, 52, GP_BALLS },
  { "GLACIA",   "ELITE 4",    T_ICE,     { 362, 364, 365 }, 54, GP_BALLS },
  { "DRAKE",    "ELITE 4",    T_DRAGON,  { 334, 330, 373 }, 57, GP_BALLS },
  { "STEVEN",   "CHAMPION",   T_STEEL,   { 306, 348, 376 }, 62, GP_BALLS },
};

// La cuarta. De Sinnoh el Anil no trae NI UN retrato (su Cintia es de su
// propia historia, no la campeona de Sinnoh), asi que los trece se bajan
// de Archives como los de Johto, y vienen ya a 80 px.
//
// Equipos de Diamante/Perla recortados a tres, con el AS EL ULTIMO.
static const GymEntry SINNOH_GYMS[GYM_TOTAL] = {
  // Sinnoh apenas evoluciona por nivel: tira de objetos, y son DIEZ. Por
  // eso aqui no sobra ni un hueco de premio — hasta el primer lider y dos
  // del Alto Mando sueltan uno, que no pasaba en las otras regiones. Sin
  // ellos media region no se completa.
  //
  // Cada uno con quien pega: la Piedra Dia con Gardenia (Roserade es su
  // as), la Piedra Noche con Fantina (Mismagius es el suyo), la Piedra
  // Alba con Byron, que vive al lado del Monte Corona, y el Electrizador
  // con Volkner, que saca un Electivire.
  //                                      de apoyo ->  AS     nv  premio
  { "ROARK",    "OREBURGH",   T_ROCK,    {  74,  95, 408 }, 14, EVOI_PROTECTOR },
  { "GARDENIA", "ETERNA",     T_GRASS,   { 420, 387, 407 }, 19, EVOI_SHINY     },
  { "MAYLENE",  "VEILSTONE",  T_FIGHT,   { 307,  67, 448 }, 24, EVOI_DUBIOUS   },
  { "WAKE",     "PASTORIA",   T_WATER,   { 195, 130, 419 }, 30, EVOI_OVAL      },
  { "FANTINA",  "HEARTHOME",  T_GHOST,   { 355,  93, 429 }, 35, EVOI_DUSK      },
  { "BYRON",    "CANALAVE",   T_STEEL,   {  82, 208, 411 }, 40, EVOI_DAWN      },
  { "CANDICE",  "SNOWPOINT",  T_ICE,     { 215, 478, 460 }, 45, EVOI_ICESTONE  },
  { "VOLKNER",  "SUNYSHORE",  T_ELEC,    {  26, 405, 466 }, 50, EVOI_ELECTIRIZ },
  { "AARON",    "ELITE 4",    T_BUG,     { 269, 416, 452 }, 53, GP_BALLS       },
  { "BERTHA",   "ELITE 4",    T_GROUND,  { 340,  76, 450 }, 56, EVOI_REAPER    },
  { "FLINT",    "ELITE 4",    T_FIRE,    {  78, 208, 392 }, 59, EVOI_MAGMARIZ  },
  { "LUCIAN",   "ELITE 4",    T_PSYCHIC, { 122, 437, 475 }, 62, GP_BALLS       },
  { "CYNTHIA",  "CHAMPION",   T_DRAGON,  { 442, 468, 445 }, 68, GP_BALLS       },
};

// La quinta. Equipos de Blanco/Negro; los trece retratos, de Archives.
//
// Es la primera region que NO necesita ningun objeto de evolucion nuevo:
// la gen 5 reaprovecha las piedras que ya hay (Minccino con la Brillante,
// Munna con la de la Luna, Petilil y Cottonee con la del Sol) y los dos
// que se intercambian, Karrablast y Shelmet, van con el Cable. Por eso
// aqui los premios vuelven a ser repuestos.
static const GymEntry UNOVA_GYMS[GYM_TOTAL] = {
  //                                      de apoyo ->  AS     nv  premio
  { "CILAN",    "STRIATON",   T_GRASS,   {   0, 506, 511 }, 15, GP_BALLS },
  { "LENORA",   "NACRENE",    T_NORMAL,  {   0, 507, 505 }, 20, T_GRASS  },
  { "BURGH",    "CASTELIA",   T_BUG,     { 544, 557, 542 }, 25, T_FAIRY  },
  { "ELESA",    "NIMBASA",    T_ELEC,    { 587, 587, 523 }, 30, T_ELEC   },
  { "CLAY",     "DRIFTVEIL",  T_GROUND,  { 552, 536, 530 }, 35, T_NONE   },
  { "SKYLA",    "MISTRALTON", T_FLYING,  { 528, 521, 581 }, 40, T_WATER  },
  { "BRYCEN",   "ICIRRUS",    T_ICE,     { 583, 615, 614 }, 45, EVOI_ICESTONE },
  { "DRAYDEN",  "OPELUCID",   T_DRAGON,  { 611, 621, 612 }, 50, T_FIRE   },
  { "SHAUNTAL", "ELITE 4",    T_GHOST,   { 563, 593, 609 }, 53, GP_BALLS },
  { "GRIMSLEY", "ELITE 4",    T_DARK,    { 560, 553, 625 }, 56, GP_BALLS },
  { "CAITLIN",  "ELITE 4",    T_PSYCHIC, { 518, 561, 579 }, 59, GP_BALLS },
  { "MARSHAL",  "ELITE 4",    T_FIGHT,   { 538, 539, 534 }, 62, GP_BALLS },
  { "ALDER",    "CHAMPION",   T_BUG,     { 617, 589, 637 }, 70, GP_BALLS },
};

// La sexta. Equipos de X/Y. Es la region mas corta (72 especies), asi que
// los equipos tiran mas de bichos de regiones anteriores que ninguna otra:
// es lo que hacen los lideres de Kalos en los propios juegos.
//
// Los retratos son los de OVERWORLD y no los de combate: de la gen 6 en
// adelante no hay sprite de combate (XY pasa a 3D) y el retrato VS es de
// medio cuerpo, que al lado de un entrenador entero de HGSS no pega.
static const GymEntry KALOS_GYMS[GYM_TOTAL] = {
  //                                      de apoyo ->  AS     nv  premio
  { "VIOLA",    "SANTALUNE",  T_BUG,     {   0, 283, 666 }, 16, GP_BALLS },
  { "GRANT",    "CYLLAGE",    T_ROCK,    {   0, 698, 697 }, 21, T_WATER  },
  { "KORRINA",  "SHALOUR",    T_FIGHT,   { 619, 674, 448 }, 26, T_GRASS  },
  { "RAMOS",    "COUMARINE",  T_GRASS,   { 189, 672, 673 }, 31, EVOI_SACHET },
  { "CLEMONT",  "LUMIOSE",    T_ELEC,    { 587, 404, 695 }, 36, T_ELEC   },
  { "VALERIE",  "LAVERRE",    T_FAIRY,   { 303, 682, 700 }, 41, EVOI_NATA },
  { "OLYMPIA",  "ANISTAR",    T_PSYCHIC, { 561, 678, 282 }, 46, T_FAIRY  },
  { "WULFRIC",  "SNOWBELLE",  T_ICE,     { 615, 713, 460 }, 51, EVOI_ICESTONE },
  { "MALVA",    "ELITE 4",    T_FIRE,    { 668, 324, 609 }, 54, GP_BALLS },
  { "SIEBOLD",  "ELITE 4",    T_WATER,   { 693, 689, 130 }, 57, GP_BALLS },
  { "WIKSTROM", "ELITE 4",    T_STEEL,   { 707, 681, 376 }, 60, GP_BALLS },
  { "DRASNA",   "ELITE 4",    T_DRAGON,  { 621, 715, 334 }, 63, GP_BALLS },
  { "DIANTHA",  "CHAMPION",   T_FAIRY,   { 699, 711, 282 }, 72, GP_BALLS },
};

// La septima. En Alola NO hay gimnasios: hay pruebas con un capitan y, al
// final de cada isla, un kahuna. Se tratan igual que un lider porque lo
// que cambia es el nombre, no lo que haces (pelear y llevarte algo).
//
// Los trece retratos NO salen de Archives: estan en Unbreakable Ties, el
// otro juego del usuario, en pixel art de 160 px como los del Anil (ver
// gen_art.ALOLA_CARAS). Es lo que contesto a "los sprites los miraste en
// el Anil?" — para Sinnoh, Teselia y Kalos no habia nada, pero para Alola
// estaba todo y casi se baja de fuera por no mirar.
static const GymEntry ALOLA_GYMS[GYM_TOTAL] = {
  //                                      de apoyo ->  AS     nv  premio
  { "ILIMA",    "VERDANT",    T_NORMAL,  {   0,  19, 735 }, 17, GP_BALLS },
  { "HALA",     "MELEMELE",   T_FIGHT,   { 296, 731, 740 }, 22, T_WATER  },
  // El as de Lana es Araquanid y no Wishiwashi: el segundo tiene 175 de
  // BST en su forma suelta (la de Banco son 620, pero eso es un cambio de
  // forma en combate que aqui no existe) y de as seria un regalo.
  { "LANA",     "BROOKLET",   T_WATER,   { 751, 746, 752 }, 27, EVOI_SEATOOTH },
  { "KIAWE",    "WELA",       T_FIRE,    {   0,  58, 757 }, 32, T_FIRE   },
  { "MALLOW",   "LUSH",       T_GRASS,   { 753, 755, 756 }, 37, T_GRASS  },
  { "OLIVIA",   "AKALA",      T_ROCK,    { 112, 558, 745 }, 42, EVOI_PROTECTOR },
  { "SOPHOCLES","HOKULANI",   T_ELEC,    {   0, 737, 738 }, 47, EVOI_ELECTIRIZ },
  { "ACEROLA",  "THRIFTY",    T_GHOST,   { 353, 356, 778 }, 52, EVOI_REAPER },
  { "NANU",     "ELITE 4",    T_DARK,    { 198, 552, 553 }, 55, GP_BALLS },
  { "MINA",     "ELITE 4",    T_FAIRY,   { 303, 743, 700 }, 58, GP_BALLS },
  { "HAPU",     "ELITE 4",    T_GROUND,  { 623, 450, 750 }, 61, GP_BALLS },
  { "MOLAYNE",  "ELITE 4",    T_STEEL,   { 462, 437, 376 }, 64, GP_BALLS },
  // Kukui va de FUEGO y no de lucha: en los juegos es aficionado a los
  // golpes de lucha, pero su equipo no tiene ni uno y el chip de tipo
  // estaria mintiendo. Su as es Incineroar.
  { "KUKUI",    "CHAMPION",   T_FIRE,    { 745, 628, 727 }, 75, GP_BALLS },
};

// La octava. Galar tiene DIEZ lideres contando los dos pares que cambian
// de version (Bea/Allister y Gordie/Melony): entran los diez, y los tres
// huecos de Alto Mando son la Copa de Campeones (Marnie, Hop y Lionel).
//
// Los retratos son los MUNECOS DE MASTERS. En Espada/Escudo no hay sprite
// de combate ni de overworld (todo 3D), y el retrato VS es de medio
// cuerpo: puesto a 80 px no se lee al lado de un entrenador entero. El de
// Masters es cuerpo entero y si (_claude_scratch/galar/).
//
// Ofelia no esta en Masters, asi que su hueco lo ocupa BEDE, que es quien
// le quita el gimnasio de hada en la historia postliga.
static const GymEntry GALAR_GYMS[GYM_TOTAL] = {
  //                                      de apoyo ->  AS     nv  premio
  // El Mineral Negro (Kleavor, que sale de Scyther) no tiene un lider
  // con quien pegue: Galar no tiene gimnasio de bicho. Va con el primero,
  // como la Protectora con Roca en Sinnoh.
  { "MILO",     "TURFFIELD",  T_GRASS,   {   0, 829, 830 }, 18, EVOI_AUGURITA },
  { "NESSA",    "HULBURY",    T_WATER,   { 833, 846, 834 }, 23, T_WATER  },
  { "KABU",     "MOTOSTOKE",  T_FIRE,    {  58, 851, 324 }, 28, T_FIRE   },
  // Los dos pergaminos parten a Kubfu en sus dos estilos. Uno con BEA,
  // que es de lucha y cuyo as ES un Urshifu, y el otro con PIERS, que es
  // de siniestro como el pergamino que suelta.
  { "BEA",      "STOW",       T_FIGHT,   { 870, 760, 892 }, 33, EVOI_ROLLOAGU },
  { "ALLISTER", "STOW",       T_GHOST,   { 855, 867, 864 }, 38, EVOI_REAPER },
  { "BEDE",     "BALLONLEA",  T_FAIRY,   { 856, 869, 858 }, 43, EVOI_SACHET },
  { "GORDIE",   "CIRCHESTER", T_ROCK,    { 526, 839, 874 }, 48, EVOI_PROTECTOR },
  { "MELONY",   "CIRCHESTER", T_ICE,     { 875, 873, 131 }, 53, EVOI_ICESTONE },
  { "PIERS",    "SPIKEMUTH",  T_DARK,    { 862, 861, 849 }, 56, EVOI_ROLLOSCU },
  { "RAIHAN",   "HAMMERLOCKE", T_DRAGON, { 526, 884, 887 }, 59, EVOI_DRAGONSCALE },
  { "MARNIE",   "CHAMPION CUP", T_DARK,  { 861, 859, 862 }, 62, GP_BALLS },
  { "HOP",      "CHAMPION CUP", T_NORMAL, { 823, 862, 887 }, 65, GP_BALLS },
  { "LEON",     "CHAMPION",   T_DRAGON,  { 884, 887,   6 }, 78, GP_BALLS },
};

// La novena y ultima: con esta la Pokedex esta entera (1025).
//
// Cuatro lideres de Paldea (Katy, Kofu, Ryme y Tulip) no tienen ningun
// dibujo de cuerpo entero: ni sprite, ni overworld, ni muneco de Masters,
// solo el retrato VS de medio cuerpo que ya se descarto en Kalos y en
// Galar. En sus huecos van personajes de la MISMA region a los que
// tambien se pelea: Arven, el director Clavell, Penny y los dos de la
// Academia Arandano.
static const GymEntry PALDEA_GYMS[GYM_TOTAL] = {
  //                                      de apoyo ->  AS     nv  premio
  { "ARVEN",    "CABO POCO",  T_DARK,    {   0, 942, 943 }, 19, GP_BALLS },
  { "BRASSIUS", "ARTAZON",    T_GRASS,   { 953, 192, 908 }, 24, GP_BALLS },
  { "IONO",     "LEVINCIA",   T_ELEC,    { 940, 944, 939 }, 29, T_ELEC   },
  { "CLAVELL",  "NARANJA",    T_NORMAL,  { 915, 926, 128 }, 34, T_NONE   },
  { "LARRY",    "MEDALI",     T_NORMAL,  { 775, 398, 982 }, 39, T_FAIRY  },
  // Penny es la de los Eevee: su as es Umbreon y la acompanan otras dos
  // ramas, que es lo que hace en los juegos.
  { "PENNY",    "ZERO LAB",   T_DARK,    { 134, 700, 197 }, 44, EVOI_DUSK },
  { "CARMINE",  "BLUEBERRY",  T_GHOST,   { 971, 1012, 1013 }, 49, EVOI_REAPER },
  { "GRUSHA",   "GLASEADO",   T_ICE,     { 974, 873, 975 }, 54, EVOI_ICESTONE },
  { "RIKA",     "ELITE 4",    T_GROUND,  { 980, 932, 934 }, 57, GP_BALLS },
  { "POPPY",    "ELITE 4",    T_STEEL,   { 879, 959, 1000 }, 60, GP_BALLS },
  { "DRAYTON",  "ELITE 4",    T_DRAGON,  { 1019, 334, 887 }, 63, GP_BALLS },
  { "HASSEL",   "ELITE 4",    T_DRAGON,  { 230, 621, 998 }, 66, GP_BALLS },
  { "GEETA",    "CHAMPION",   T_ROCK,    { 983, 976, 970 }, 82, GP_BALLS },
};

#define REGION_COUNT 9
static const Region REGIONS[REGION_COUNT] = {
  { "KANTO",  KANTO_GYMS,  GYM_TOTAL, GYM_BADGES, BST_KANTO, { 1, 4, 7 } },   // Bulbasaur / Charmander / Squirtle
  { "JOHTO",  JOHTO_GYMS,  GYM_TOTAL, GYM_BADGES, BST_JOHTO, { 152, 155, 158 } },   // Chikorita / Cyndaquil / Totodile
  { "HOENN",  HOENN_GYMS,  GYM_TOTAL, GYM_BADGES, BST_HOENN, { 252, 255, 258 } },   // Treecko / Torchic / Mudkip
  { "SINNOH", SINNOH_GYMS, GYM_TOTAL, GYM_BADGES, BST_SINNOH, { 387, 390, 393 } },   // Turtwig / Chimchar / Piplup
  { "UNOVA",  UNOVA_GYMS,  GYM_TOTAL, GYM_BADGES, BST_UNOVA, { 495, 498, 501 } },   // Snivy / Tepig / Oshawott
  { "KALOS",  KALOS_GYMS,  GYM_TOTAL, GYM_BADGES, BST_KALOS, { 650, 653, 656 } },   // Chespin / Fennekin / Froakie
  { "ALOLA",  ALOLA_GYMS,  GYM_TOTAL, GYM_BADGES, BST_ALOLA, { 722, 725, 728 } },   // Rowlet / Litten / Popplio
  { "GALAR",  GALAR_GYMS,  GYM_TOTAL, GYM_BADGES, BST_GALAR, { 810, 813, 816 } },   // Grookey / Scorbunny / Sobble
  { "PALDEA", PALDEA_GYMS, GYM_TOTAL, GYM_BADGES, BST_PALDEA, { 906, 909, 912 } },   // Sprigatito / Fuecoco / Quaxly
};

static inline const Region &regionAt(uint8_t r) {
  return REGIONS[r < REGION_COUNT ? r : 0];
}
// O NOME de cada insignia, na ordem dos ginasios de cada regiao (a mesma
// de gymAt). Alola nao tem insignias —— sao provas, e o premio e o Cristal
// Z do capitao ——; em Paldea, Arven e Penny ganham as da Equipe Star.
static const char *const BADGE_NAME[REGION_COUNT][8] = {
  { "BOULDER BADGE", "CASCADE BADGE", "THUNDER BADGE", "RAINBOW BADGE",
    "SOUL BADGE", "MARSH BADGE", "VOLCANO BADGE", "EARTH BADGE" },
  { "ZEPHYR BADGE", "HIVE BADGE", "PLAIN BADGE", "FOG BADGE",
    "STORM BADGE", "MINERAL BADGE", "GLACIER BADGE", "RISING BADGE" },
  { "STONE BADGE", "KNUCKLE BADGE", "DYNAMO BADGE", "HEAT BADGE",
    "BALANCE BADGE", "FEATHER BADGE", "MIND BADGE", "RAIN BADGE" },
  { "COAL BADGE", "FOREST BADGE", "COBBLE BADGE", "FEN BADGE",
    "RELIC BADGE", "MINE BADGE", "ICICLE BADGE", "BEACON BADGE" },
  { "TRIO BADGE", "BASIC BADGE", "INSECT BADGE", "BOLT BADGE",
    "QUAKE BADGE", "JET BADGE", "FREEZE BADGE", "LEGEND BADGE" },
  { "BUG BADGE", "CLIFF BADGE", "RUMBLE BADGE", "PLANT BADGE",
    "VOLTAGE BADGE", "FAIRY BADGE", "PSYCHIC BADGE", "ICEBERG BADGE" },
  { "NORMALIUM Z", "FIGHTINIUM Z", "WATERIUM Z", "FIRIUM Z",
    "GRASSIUM Z", "ROCKIUM Z", "ELECTRIUM Z", "GHOSTIUM Z" },
  { "GRASS BADGE", "WATER BADGE", "FIRE BADGE", "FIGHTING BADGE",
    "GHOST BADGE", "FAIRY BADGE", "ROCK BADGE", "ICE BADGE" },
  { "STAR DARK BADGE", "GRASS BADGE", "ELECTRIC BADGE", "PSYCHIC BADGE",
    "NORMAL BADGE", "STAR FAIRY BADGE", "GHOST BADGE", "ICE BADGE" },
};
static inline const char *badgeName(uint8_t r, uint8_t i) {
  return (r < REGION_COUNT && i < 8) ? BADGE_NAME[r][i] : "BADGE";
}

static inline const GymEntry &gymAt(uint8_t r, uint8_t i) {
  const Region &g = regionAt(r);
  return g.gyms[i < g.count ? i : g.count - 1];
}
// El nivel del CAMPEON de una region: el as del ultimo desafio. Es el
// nivel al que pelea todo el mundo en una revancha —— que es lo que la
// convierte en el sitio donde entrenar de verdad, y no en pegarle otra
// vez a un Brock de nivel 8.
static inline uint8_t gymChampLevel(uint8_t r) {
  const Region &g = regionAt(r);
  return g.gyms[g.count - 1].level;
}
