// GENERADO por tools/gen_megas.py desde pokemon_forms.txt del Anil.
// No editar.
//
// Una mega no es una especie: no tiene numero de dex. Es una forma
// que se toma DENTRO del combate llevando su piedra, y de la que se
// vuelve al acabar, asi que no entra en DEX_TBL.
//
// OJO: aqui los PS SI cambian. En los juegos oficiales una mega
// nunca los toca, pero el Anil lo hace en varias (entre sus megas
// hay versiones Dynamax). Por eso bHp esta en la tabla y por eso
// megaEvolve() escala la vida actual con el maximo nuevo.
#pragma once
#include <stdint.h>
#include "dex.h"
#include "items.h"

// Una mega que no esta atada a una forma concreta: Meowstic,
// Zygarde y Alcremie, que tienen muchas y vuelven a la que fueran.
#define MEGA_ANY_FORM 0xFF

struct MegaForm {
  int16_t base;      // numero de dex del que se transforma
  uint8_t item;      // MEGA_* : el `arg` de su piedra en ITEM_TBL
  uint8_t form;      // numero de forma en el Anil: el sprite es
                     // <slug>_<form>.bin en la tarjeta
  // De QUE forma sale (0 = de la normal, MEGA_ANY_FORM = de
  // cualquiera). Lo dice UnmegaForm en el PBS, y es lo unico que
  // distingue las dos megas de Toxtricity, que piden la misma
  // piedra: una sale del normal y otra del Grave.
  uint8_t fromForm;
  // bHp es uint16_t y los otros cinco uint8_t. Hoy el mayor es el
  // Mega Snorlax con 160, asi que cabria en un byte; se queda
  // ancho porque el Anil mete versiones Dynamax entre sus megas y
  // ya hubo una (el Mewtwo Armadura) con 400 de PS base. Pasarlo a
  // byte ahorra 127 bytes de flash y vuelve a romperse solo.
  uint16_t bHp;
  uint8_t bAtk, bDef, bSpA, bSpD, bSpe;
  uint8_t type1, type2;
  uint16_t ability;
  uint8_t screenSz;   // alto en pantalla, como DexEntry
  const char *name;  // 12 caracteres como mucho, igual que la dex
};

#define MEGA_COUNT 127
static const MegaForm MEGA_TBL[MEGA_COUNT] = {
  {    3, MEGA_VENUSAURITEX,  1,  0,  80, 100, 123, 122, 120,  80, T_GRASS   , T_POISON  , 269, 154, "M.VENUSAUR X" },  // VENUSAUR
  {    3, MEGA_VENUSAURITEY,  2,  0,  80,  92, 103, 130, 150,  70, T_GRASS   , T_POISON  , 250, 162, "M.VENUSAUR Y" },  // VENUSAUR
  {    6, MEGA_CHARIZARDITEX,  1,  0,  78, 130, 111, 130,  85, 100, T_FIRE    , T_DRAGON  , 273, 152, "M.CHARIZAR X" },  // CHARIZARD
  {    6, MEGA_CHARIZARDITEY,  2,  0,  78, 104,  78, 159, 115, 100, T_FIRE    , T_FLYING  ,  55, 151, "M.CHARIZAR Y" },  // CHARIZARD
  {    9, MEGA_BLASTOISINITEX,  1,  0,  79, 103, 120, 135, 115,  78, T_WATER   , T_NONE    , 134, 142, "M.BLASTOIS X" },  // BLASTOISE
  {    9, MEGA_BLASTOISINITEY,  2,  0,  79,  93, 140, 115, 145,  58, T_WATER   , T_STEEL   , 217, 161, "M.BLASTOIS Y" },  // BLASTOISE
  {   12, MEGA_BUTTERFREEITE,  1,  0,  60,  55,  70, 120, 130, 110, T_BUG     , T_FLYING  ,  34, 154, "M.BUTTERFREE" },  // BUTTERFREE
  {   15, MEGA_BEEDRILLITE,  1,  0,  65, 150,  40,  15,  80, 145, T_BUG     , T_POISON  ,   2, 123, "M.BEEDRILL" },  // BEEDRILL
  {   18, MEGA_PIDGEOTITE ,  1,  0,  83,  80,  80, 135,  80, 121, T_NORMAL  , T_FLYING  , 151, 152, "MEGA PIDGEOT" },  // PIDGEOT
  {   25, MEGA_PIKACHUITE , 17, 16,  55, 135, 100, 130, 100, 110, T_ELEC    , T_NONE    , 228, 145, "MEGA PIKACHU" },  // PIKACHU
  {   26, MEGA_RAICHUITEX ,  2,  0,  60, 135,  95,  90,  95, 110, T_ELEC    , T_NONE    , 191, 139, "M.RAICHU X" },  // RAICHU
  {   26, MEGA_RAICHUITEY ,  3,  0,  60, 100,  55, 160,  80, 130, T_ELEC    , T_NONE    ,   2, 142, "M.RAICHU Y" },  // RAICHU
  {   36, MEGA_CLEFABLITE ,  1,  0,  95,  80,  93, 135, 110,  70, T_FAIRY   , T_FLYING  ,   3, 147, "M.CLEFABLE" },  // CLEFABLE
  {   59, MEGA_ARCANINITE ,  2,  0,  90, 150,  95, 150,  95, 125, T_FIRE    , T_GROUND  , 242, 151, "M.ARCANINE" },  // ARCANINE
  {   65, MEGA_ALAKAZITE  ,  1,  0,  55,  50,  65, 175,  95, 150, T_PSYCHIC , T_NONE    , 277, 136, "M.ALAKAZAM" },  // ALAKAZAM
  {   68, MEGA_MACHAMPITE ,  1,  0,  90, 160, 100,  75,  95,  75, T_FIGHT   , T_GROUND  , 151, 146, "MEGA MACHAMP" },  // MACHAMP
  {   71, MEGA_VICTREEBELITE,  1,  0,  80, 125,  85, 135,  95,  70, T_GRASS   , T_POISON  ,  36, 136, "M.VICTREEBEL" },  // VICTREEBEL
  {   80, MEGA_SLOWBRONITE,  2,  0,  95,  75, 180, 130,  80,  30, T_WATER   , T_PSYCHIC , 217, 122, "MEGA SLOWBRO" },  // SLOWBRO
  {   94, MEGA_GENGARITEX ,  1,  0,  60,  75,  80, 160,  95, 130, T_GHOST   , T_POISON  , 213, 133, "M.GENGAR X" },  // GENGAR
  {   94, MEGA_GENGARITEY ,  2,  0,  60,  95, 110, 130, 115,  90, T_GHOST   , T_DARK    , 160, 161, "M.GENGAR Y" },  // GENGAR
  {   99, MEGA_KINGLERITE ,  1,  0,  75, 170, 135,  70,  75,  75, T_WATER   , T_NONE    , 273, 161, "MEGA KINGLER" },  // KINGLER
  {  115, MEGA_KANGASKHANITE,  1,  0, 105, 115, 100,  60, 100, 100, T_NORMAL  , T_NONE    , 158, 137, "M.KANGASKHAN" },  // KANGASKHAN
  {  121, MEGA_STARMINITE ,  1,  0,  60, 100, 105, 130, 105, 120, T_WATER   , T_PSYCHIC ,  96, 127, "MEGA STARMIE" },  // STARMIE
  {  127, MEGA_PINSIRITE  ,  1,  0,  65, 155, 120,  65,  90, 105, T_BUG     , T_FLYING  ,   3, 136, "MEGA PINSIR" },  // PINSIR
  {  130, MEGA_GYARADOSITE,  1,  0,  95, 155, 109,  70, 130,  81, T_WATER   , T_DARK    , 140, 149, "M.GYARADOS" },  // GYARADOS
  {  131, MEGA_LAPRASITE  ,  1,  0, 130,  95, 100, 125, 105,  80, T_WATER   , T_ICE     , 192, 151, "MEGA LAPRAS" },  // LAPRAS
  {  133, MEGA_EEVEEITE   ,  2,  1,  65,  90, 110,  80, 115,  75, T_NORMAL  , T_NONE    ,   2, 143, "MEGA EEVEE" },  // EEVEE
  {  142, MEGA_AERODACTYLITE,  1,  0,  80, 135,  85,  70,  95, 150, T_ROCK    , T_FLYING  , 273, 129, "M.AERODACTYL" },  // AERODACTYL
  {  143, MEGA_SNORLAXITE ,  1,  0, 160, 130,  85,  95, 140,  30, T_NORMAL  , T_NONE    , 269, 152, "MEGA SNORLAX" },  // SNORLAX
  {  149, MEGA_DRAGONITITE,  1,  0,  91, 124, 115, 145, 125, 100, T_DRAGON  , T_FLYING  , 144, 148, "M.DRAGONITE" },  // DRAGONITE
  {  150, MEGA_MEWTWONITEX,  1,  0, 106, 190, 100, 154, 100, 130, T_PSYCHIC , T_FIGHT   , 240, 117, "M.MEWTWO X" },  // MEWTWO
  {  150, MEGA_MEWTWONITEY,  2,  0, 106, 150,  70, 194, 120, 140, T_PSYCHIC , T_NONE    , 111, 111, "M.MEWTWO Y" },  // MEWTWO
  {  154, MEGA_MEGANIUMITE,  1,  0,  80,  92, 115, 143, 115,  80, T_GRASS   , T_FAIRY   ,  65, 136, "M.MEGANIUM" },  // MEGANIUM
  {  160, MEGA_FERALIGITE ,  1,  0,  85, 160, 125,  89,  93,  78, T_WATER   , T_DRAGON  , 216, 140, "M.FERALIGATR" },  // FERALIGATR
  {  181, MEGA_AMPHAROSITE,  1,  0,  90,  95, 105, 165, 110,  45, T_ELEC    , T_DRAGON  , 140, 129, "M.AMPHAROS" },  // AMPHAROS
  {  189, MEGA_JUMPLUFITE ,  1,  0,  75, 100, 100,  55, 125, 130, T_GRASS   , T_FLYING  , 300, 115, "M.JUMPLUFF" },  // JUMPLUFF
  {  208, MEGA_STEELIXITE ,  1,  0,  75, 125, 230,  55,  95,  30, T_STEEL   , T_GROUND  , 216, 152, "MEGA STEELIX" },  // STEELIX
  {  212, MEGA_SCIZORITE  ,  1,  0,  70, 150, 140,  65, 100,  75, T_BUG     , T_STEEL   , 264, 131, "MEGA SCIZOR" },  // SCIZOR
  {  214, MEGA_HERACRONITE,  1,  0,  80, 185, 115,  40, 105,  75, T_BUG     , T_FIGHT   , 222, 129, "M.HERACROSS" },  // HERACROSS
  {  227, MEGA_SKARMORITE ,  1,  0,  65, 140, 110,  40, 100, 110, T_STEEL   , T_FLYING  , 273, 122, "M.SKARMORY" },  // SKARMORY
  {  229, MEGA_HOUNDOOMINITE,  1,  0,  75,  90,  90, 140,  90, 115, T_DARK    , T_FIRE    , 229, 124, "M.HOUNDOOM" },  // HOUNDOOM
  {  248, MEGA_TYRANITARITE,  1,  0, 100, 164, 150,  95, 120,  71, T_ROCK    , T_DARK    , 204, 145, "M.TYRANITAR" },  // TYRANITAR
  {  254, MEGA_SCEPTILITE ,  1,  0,  70, 145,  75, 110,  85, 145, T_GRASS   , T_DRAGON  , 220, 140, "M.SCEPTILE" },  // SCEPTILE
  {  257, MEGA_BLAZIKENITE,  1,  0,  80, 160,  80, 130,  80, 100, T_FIRE    , T_FIGHT   , 233, 138, "M.BLAZIKEN" },  // BLAZIKEN
  {  260, MEGA_SWAMPERTITE,  1,  0, 100, 150, 110,  95, 110,  70, T_WATER   , T_GROUND  , 257, 165, "M.SWAMPERT" },  // SWAMPERT
  {  282, MEGA_GARDEVOIRITE,  1,  0,  68,  85,  65, 165, 135, 100, T_PSYCHIC , T_FAIRY   , 164, 131, "M.GARDEVOIR" },  // GARDEVOIR
  {  302, MEGA_SABLENITE  ,  1,  0,  75, 125,  95,  85,  85, 110, T_DARK    , T_GHOST   , 129, 124, "MEGA SABLEYE" },  // SABLEYE
  {  303, MEGA_MAWILITE   ,  1,  0,  65, 105, 125,  55,  95,  50, T_STEEL   , T_FAIRY   ,  96, 135, "MEGA MAWILE" },  // MAWILE
  {  306, MEGA_AGGRONITE  ,  1,  0,  70, 140, 230,  60,  80,  50, T_STEEL   , T_NONE    ,  65, 151, "MEGA AGGRON" },  // AGGRON
  {  308, MEGA_MEDICHAMITE,  1,  0,  60, 100,  85,  80,  85, 100, T_FIGHT   , T_PSYCHIC , 182, 125, "M.MEDICHAM" },  // MEDICHAM
  {  310, MEGA_MANECTITE  ,  1,  0,  70,  75,  80, 135,  80, 135, T_ELEC    , T_NONE    , 112, 141, "M.MANECTRIC" },  // MANECTRIC
  {  319, MEGA_SHARPEDONITE,  1,  0,  70, 140,  70, 110,  65, 105, T_WATER   , T_DARK    , 248, 120, "M.SHARPEDO" },  // SHARPEDO
  {  323, MEGA_CAMERUPTITE,  1,  0,  70, 120, 100, 135, 105,  30, T_FIRE    , T_GROUND  , 216, 139, "M.CAMERUPT" },  // CAMERUPT
  {  334, MEGA_ALTARIANITE,  1,  0,  75, 110, 110, 120, 105,  90, T_DRAGON  , T_FAIRY   , 164, 156, "MEGA ALTARIA" },  // ALTARIA
  {  354, MEGA_BANETTITE  ,  1,  0,  64, 165,  75,  93,  83,  75, T_GHOST   , T_NONE    , 174, 132, "MEGA BANETTE" },  // BANETTE
  {  358, MEGA_CHIMECHITE ,  1,  0,  75,  50, 110, 135, 120,  65, T_PSYCHIC , T_STEEL   , 271, 139, "M.CHIMECHO" },  // CHIMECHO
  {  359, MEGA_ABSOLITE   ,  1,  0,  65, 150,  60, 115,  60, 115, T_DARK    , T_NONE    , 129, 124, "MEGA ABSOL" },  // ABSOL
  {  359, MEGA_ABSOLITEZ  ,  2,  0,  65, 154,  60,  75,  60, 151, T_DARK    , T_GHOST   , 214, 123, "MEGA ABSOL Z" },  // ABSOL
  {  362, MEGA_GLALITITE  ,  1,  0,  90, 130,  80, 130,  80, 100, T_ICE     , T_DARK    , 192, 132, "MEGA GLALIE" },  // GLALIE
  {  373, MEGA_SALAMENCITE,  1,  0,  95, 145, 130, 120,  90, 120, T_DRAGON  , T_FLYING  ,   3, 145, "M.SALAMENCE" },  // SALAMENCE
  {  376, MEGA_METAGROSSITE,  1,  0,  95, 145, 130, 120,  90, 120, T_STEEL   , T_PSYCHIC , 273, 149, "M.METAGROSS" },  // METAGROSS
  {  380, MEGA_LATIASITE  ,  1,  0,  80, 100, 120, 140, 150, 110, T_DRAGON  , T_PSYCHIC , 120, 123, "MEGA LATIAS" },  // LATIAS
  {  381, MEGA_LATIOSITE  ,  1,  0,  80, 130, 100, 160, 120, 110, T_DRAGON  , T_PSYCHIC , 120, 122, "MEGA LATIOS" },  // LATIOS
  {  384, MEGA_RAYQUAZITE ,  1,  0, 105, 180, 100, 180, 100, 115, T_DRAGON  , T_FLYING  ,  50, 156, "M.RAYQUAZA" },  // RAYQUAZA
  {  398, MEGA_STARAPTITE ,  1,  0,  85, 140, 100,  60,  90, 110, T_FIGHT   , T_FLYING  , 191, 146, "M.STARAPTOR" },  // STARAPTOR
  {  428, MEGA_LOPUNNITE  ,  1,  0,  65, 136,  94,  54,  96, 135, T_NORMAL  , T_FIGHT   , 208, 120, "MEGA LOPUNNY" },  // LOPUNNY
  {  445, MEGA_GARCHOMPITE,  1,  0, 108, 170, 115, 120,  95,  92, T_DRAGON  , T_GROUND  , 201, 138, "M.GARCHOMP" },  // GARCHOMP
  {  445, MEGA_GARCHOMPITEZ,  2,  0, 108, 130,  85, 141,  85, 151, T_DRAGON  , T_NONE    , 120, 154, "M.GARCHOMP Z" },  // GARCHOMP
  {  448, MEGA_LUCARIONITE,  1,  0,  70, 145,  88, 140,  70, 112, T_FIGHT   , T_STEEL   ,   2, 114, "MEGA LUCARIO" },  // LUCARIO
  {  448, MEGA_LUCARIONITEZ,  2,  0,  70, 100,  70, 164,  70, 151, T_FIGHT   , T_PSYCHIC , 134, 125, "M.LUCARIO Z" },  // LUCARIO
  {  460, MEGA_ABOMASITE  ,  1,  0,  90, 132, 105, 132, 105,  30, T_GRASS   , T_ICE     , 227, 157, "M.ABOMASNOW" },  // ABOMASNOW
  {  475, MEGA_GALLADITE  ,  1,  0,  68, 165,  95,  65, 115, 110, T_PSYCHIC , T_FIGHT   , 110, 136, "MEGA GALLADE" },  // GALLADE
  {  478, MEGA_FROSLASSITE,  1,  0,  70,  80,  70, 140, 100, 120, T_ICE     , T_GHOST   , 227, 145, "M.FROSLASS" },  // FROSLASS
  {  485, MEGA_HEATRANITE ,  1,  0,  91, 120, 106, 175, 141,  67, T_FIRE    , T_STEEL   ,  55, 165, "MEGA HEATRAN" },  // HEATRAN
  {  491, MEGA_DARKRANITE ,  1,  0,  70, 120, 130, 165, 130,  85, T_DARK    , T_NONE    ,  13, 127, "MEGA DARKRAI" },  // DARKRAI
  {  500, MEGA_EMBOARITE  ,  1,  0, 110, 148,  75, 110, 110,  75, T_FIRE    , T_DARK    , 191, 148, "MEGA EMBOAR" },  // EMBOAR
  {  530, MEGA_EXCADRITE  ,  1,  0, 110, 165, 100,  65,  65, 103, T_GROUND  , T_STEEL   , 140, 143, "M.EXCADRILL" },  // EXCADRILL
  {  531, MEGA_AUDINITE   ,  1,  0, 103,  60, 126,  80, 126,  50, T_NORMAL  , T_FAIRY   ,  91, 114, "MEGA AUDINO" },  // AUDINO
  {  545, MEGA_SCOLIPITE  ,  1,  0,  69, 140, 149,  75,  99,  62, T_BUG     , T_POISON  ,  16, 138, "M.SCOLIPEDE" },  // SCOLIPEDE
  {  560, MEGA_SCRAFTINITE,  1,  0,  65, 130, 135,  55, 135,  68, T_DARK    , T_FIGHT   , 143, 114, "MEGA SCRAFTY" },  // SCRAFTY
  {  569, MEGA_GARBODORITE,  1,  0, 100, 125, 112,  95, 112,  55, T_POISON  , T_STEEL   , 132, 162, "M.GARBODOR" },  // GARBODOR
  {  604, MEGA_EELEKTROSSITE,  1,  0,  85, 145,  80, 135,  90,  80, T_ELEC    , T_NONE    , 233, 130, "M.EELEKTROSS" },  // EELEKTROSS
  {  609, MEGA_CHANDELURITE,  1,  0,  60,  75, 110, 175, 110,  90, T_GHOST   , T_FIRE    , 104, 135, "M.CHANDELURE" },  // CHANDELURE
  {  623, MEGA_GOLURKITE  ,  1,  0,  89, 159, 105,  70, 105,  55, T_GROUND  , T_GHOST   , 115, 168, "MEGA GOLURK" },  // GOLURK
  {  652, MEGA_CHESNAUGHTITE,  1,  0, 100, 137, 172,  74, 115,  44, T_GRASS   , T_FIGHT   , 185, 151, "M.CHESNAUGHT" },  // CHESNAUGHT
  {  655, MEGA_DELPHOXITE ,  1,  0,  75,  69,  72, 159, 125, 134, T_FIRE    , T_PSYCHIC , 120, 138, "MEGA DELPHOX" },  // DELPHOX
  {  658, MEGA_GRENINJITE ,  3,  0,  72, 125,  77, 133,  81, 142, T_WATER   , T_DARK    , 222, 129, "M.GRENINJA" },  // GRENINJA
  {  668, MEGA_PYROARITE  ,  1,  0,  86,  88,  92, 129,  86, 126, T_FIRE    , T_NORMAL  ,   2, 141, "MEGA PYROAR" },  // PYROAR
  {  670, MEGA_FLOETTITE  ,  6,  5,  74,  65,  67, 125, 128,  92, T_FAIRY   , T_NONE    , 134, 121, "MEGA FLOETTE" },  // FLOETTE
  {  678, MEGA_MEOWSTICITE,  2, 255,  74,  48,  76, 143, 101, 124, T_PSYCHIC , T_NONE    , 180, 125, "M.MEOWSTIC" },  // MEOWSTIC
  {  687, MEGA_MALAMARITE ,  1,  0,  86, 102,  88,  98, 120,  88, T_DARK    , T_PSYCHIC , 149, 138, "MEGA MALAMAR" },  // MALAMAR
  {  689, MEGA_BARBARACITE,  1,  0,  72, 140, 130,  64, 106,  88, T_ROCK    , T_FIGHT   , 273, 138, "M.BARBARACLE" },  // BARBARACLE
  {  691, MEGA_DRAGALGITE ,  1,  0,  65,  85, 105, 132, 163,  44, T_POISON  , T_DRAGON  ,   2, 146, "M.DRAGALGE" },  // DRAGALGE
  {  701, MEGA_HAWLUCHITE ,  1,  0,  78, 137, 100,  74,  93, 118, T_FIGHT   , T_FLYING  , 191, 140, "M.HAWLUCHA" },  // HAWLUCHA
  {  718, MEGA_ZYGARDITE  ,  4, 255,  74,  65,  67, 125, 128,  92, T_DRAGON  , T_GROUND  , 172, 150, "MEGA ZYGARDE" },  // ZYGARDE
  {  719, MEGA_DIANCITE   ,  1,  0,  50, 160, 110, 160, 110, 110, T_ROCK    , T_FAIRY   , 129, 142, "MEGA DIANCIE" },  // DIANCIE
  {  740, MEGA_CRABOMINITE,  1,  0,  97, 157, 122,  62, 107,  33, T_FIGHT   , T_ICE     , 115, 149, "M.CRABOMINAB" },  // CRABOMINABLE
  {  768, MEGA_GOLISOPITE ,  1,  0,  75, 150, 175,  70, 120,  40, T_BUG     , T_STEEL   , 217, 156, "M.GOLISOPOD" },  // GOLISOPOD
  {  780, MEGA_DRAMPANITE ,  1,  0,  78,  85, 110, 160, 116,  36, T_NORMAL  , T_DRAGON  ,  19, 132, "MEGA DRAMPA" },  // DRAMPA
  {  801, MEGA_MAGEARNITE ,  2,  0,  80, 125, 115, 170, 115,  95, T_STEEL   , T_FAIRY   , 231, 134, "M.MAGEARNA" },  // MAGEARNA
  {  801, MEGA_MAGEARNITE ,  3,  1,  80, 125, 115, 170, 115,  95, T_STEEL   , T_FAIRY   , 231, 134, "M.MAGEARNA O" },  // MAGEARNA
  {  807, MEGA_ZERAORITE  ,  1,  0,  88, 157,  75, 147,  80, 153, T_ELEC    , T_NONE    , 289, 134, "MEGA ZERAORA" },  // ZERAORA
  {  812, MEGA_RILLABOOMITE,  1,  0, 100, 155, 120, 100,  90,  66, T_GRASS   , T_NONE    , 181, 170, "M.RILLABOOM" },  // RILLABOOM
  {  815, MEGA_CINDERECITE,  1,  0,  80, 146,  85,  95,  85, 139, T_FIRE    , T_NONE    , 121, 151, "M.CINDERACE" },  // CINDERACE
  {  818, MEGA_INTELEONITE,  1,  0,  70,  95,  75, 175,  75, 140, T_WATER   , T_DARK    , 225, 126, "M.INTELEON" },  // INTELEON
  {  823, MEGA_CORVIKNIGHTITE,  1,  0,  98, 117, 125,  73,  95,  87, T_FLYING  , T_STEEL   , 191, 170, "M.CORVIKNIGH" },  // CORVIKNIGHT
  {  826, MEGA_ORBEETLEITE,  1,  0,  60,  65, 130, 110, 140, 100, T_BUG     , T_PSYCHIC , 149, 170, "M.ORBEETLE" },  // ORBEETLE
  {  834, MEGA_DREDNAWITE ,  1,  0,  90, 145, 130,  58,  88,  74, T_WATER   , T_ROCK    , 230, 166, "MEGA DREDNAW" },  // DREDNAW
  {  839, MEGA_COALOSSALITE,  1,  0, 110, 110, 140, 110, 110,  30, T_ROCK    , T_FIRE    ,  66, 157, "M.COALOSSAL" },  // COALOSSAL
  {  841, MEGA_FLAPPLETUNITE,  1,  0, 110, 125, 100, 100, 100,  50, T_GRASS   , T_DRAGON  ,  98, 140, "MEGA FLAPPLE" },  // FLAPPLE
  {  842, MEGA_FLAPPLETUNITE,  1,  0, 110, 105, 100, 120, 100,  50, T_GRASS   , T_DRAGON  , 269, 140, "M.APPLETUN" },  // APPLETUN
  {  844, MEGA_SANDACONDITE,  1,  0,  72, 137, 125,  75,  80, 131, T_GROUND  , T_NONE    , 204, 143, "M.SANDCONDA" },  // SANDACONDA
  {  849, MEGA_TOXTRICITITE,  2,  0,  75, 118,  90, 144,  80,  95, T_ELEC    , T_POISON  ,  52, 148, "M.TOXTRICITY" },  // TOXTRICITY
  {  849, MEGA_TOXTRICITITE,  3,  1,  75, 118,  90, 144,  80,  95, T_ELEC    , T_POISON  ,  52, 148, "M.TOXTRICI L" },  // TOXTRICITY
  {  851, MEGA_CENTISKORCHITE,  1,  0, 100, 145,  95, 100,  90,  95, T_FIRE    , T_BUG     ,  21, 154, "M.CENTISKORC" },  // CENTISKORCH
  {  858, MEGA_HATTERENITE,  1,  0,  57, 110, 115, 156, 123,  49, T_PSYCHIC , T_FAIRY   , 129, 141, "M.HATTERENE" },  // HATTERENE
  {  861, MEGA_GRIMMSNARLITE,  1,  0,  95, 150,  85, 115,  75,  90, T_DARK    , T_FAIRY   , 216, 137, "M.GRIMMSNARL" },  // GRIMMSNARL
  {  869, MEGA_ALCREMITE  , 63, 255,  65,  80, 105, 130, 151,  64, T_FAIRY   , T_NONE    , 193, 141, "M.ALCREMIE" },  // ALCREMIE
  {  870, MEGA_FALINKSITE ,  1,  0,  65, 135, 135,  70,  65, 100, T_FIGHT   , T_STEEL   ,   2, 136, "MEGA FALINKS" },  // FALINKS
  {  879, MEGA_COPPERAJITE,  1,  0, 122, 150,  99,  80,  69,  80, T_STEEL   , T_NONE    , 216, 161, "M.COPPERAJAH" },  // COPPERAJAH
  {  884, MEGA_DURALUDONITE,  1,  0,  70, 115, 135, 140,  70, 105, T_STEEL   , T_DRAGON  ,  28, 142, "M.DURALUDON" },  // DURALUDON
  {  952, MEGA_SCOVILLAINITE,  1,  0,  65, 138,  85, 138,  85,  75, T_GRASS   , T_FIRE    ,  23, 126, "M.SCOVILLAIN" },  // SCOVILLAIN
  {  970, MEGA_GLIMMORANITE,  1,  0,  83,  90, 105, 150,  96, 101, T_ROCK    , T_POISON  , 120, 156, "M.GLIMMORA" },  // GLIMMORA
  {  978, MEGA_TATSUGIRINITE,  3,  0,  68,  65,  90, 135, 125,  92, T_DRAGON  , T_WATER   ,  32, 150, "M.TATSUGIRI" },  // TATSUGIRI
  {  978, MEGA_TATSUGIRINITE,  4,  1,  68,  65,  90, 135, 125,  92, T_DRAGON  , T_WATER   ,  32, 151, "M.TATSUGIR D" },  // TATSUGIRI
  {  978, MEGA_TATSUGIRINITE,  5,  2,  68,  65,  90, 135, 125,  92, T_DRAGON  , T_WATER   ,  32, 151, "M.TATSUGIR S" },  // TATSUGIRI
  {  998, MEGA_BAXCALIBRITE,  1,  0, 115, 175, 117, 105, 101,  87, T_DRAGON  , T_ICE     , 268, 140, "M.BAXCALIBUR" },  // BAXCALIBUR
};

// La mega de una especie con ESA piedra, o -1.
//
// Hacen falta las tres cosas: Charizard tiene dos megas y las
// separa la PIEDRA; Toxtricity tiene dos con la misma piedra y
// las separa la FORMA de la que sale (la normal o la Grave).
static inline int16_t megaFormOf(int16_t dex, uint8_t item,
                                 uint8_t form = 0) {
  for (uint16_t i = 0; i < MEGA_COUNT; i++)
    if (MEGA_TBL[i].base == dex && MEGA_TBL[i].item == item &&
        (MEGA_TBL[i].fromForm == form ||
         MEGA_TBL[i].fromForm == MEGA_ANY_FORM))
      return (int16_t)i;
  return -1;
}

// Si a esta especie le sirve alguna piedra, para poder decirlo en
// la ficha sin tener que saber cual lleva encima.
static inline bool hasMega(int16_t dex) {
  for (uint16_t i = 0; i < MEGA_COUNT; i++)
    if (MEGA_TBL[i].base == dex) return true;
  return false;
}

