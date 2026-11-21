// GENERADO por tools/gen_evos.py desde el PBS del Anil - no editar.
//
// Que OBJETO pide cada evolucion. Antes esto estaba a mano en
// stones.h y se invento la mitad: un "cable" generico donde el Anil
// dice Metal Coat, Dragon Scale o King's Rock, y piedras para Espeon
// y Umbreon donde dice amistad.
//
// Item, TradeItem y DayHoldItem acaban todos aqui: en un cacharro sin
// radio no hay con quien intercambiar, asi que el objeto se USA desde
// la bolsa. El Cable sigue existiendo para los que se cambian SIN
// objeto (Alakazam, Machamp, Golem, Gengar).
#pragma once
#include <stdint.h>
#include "items.h"

struct EvoItem {
  int16_t from;
  uint8_t item;    // EVOI_*: el `arg` del objeto en ITEM_TBL
  int16_t to;
  uint8_t sexo;    // 0 cualquiera, 1 solo hembra, 2 solo macho
  // Se ela vem de TROCA no Anil: 1 = Trade (sem objeto, o Cabo),
  // 2 = TradeItem (chegar segurando o objeto). Com ESP-NOW a troca
  // de verdade evolui por isto; o objeto da bolsa continua valendo.
  uint8_t troca;
};

#define EVO_ITEM_COUNT 87
static const EvoItem EVO_ITEM[EVO_ITEM_COUNT] = {
  {  25, T_ELEC           ,  26, 0, 0 },  // PIKACHU -> RAICHU (Item, THUNDERSTONE)
  {  30, T_FAIRY          ,  31, 0, 0 },  // NIDORINA -> NIDOQUEEN (Item, MOONSTONE)
  {  33, T_FAIRY          ,  34, 0, 0 },  // NIDORINO -> NIDOKING (Item, MOONSTONE)
  {  35, T_FAIRY          ,  36, 0, 0 },  // CLEFAIRY -> CLEFABLE (Item, MOONSTONE)
  {  37, T_FIRE           ,  38, 0, 0 },  // VULPIX -> NINETALES (Item, FIRESTONE)
  {  39, T_FAIRY          ,  40, 0, 0 },  // JIGGLYPUFF -> WIGGLYTUFF (Item, MOONSTONE)
  {  44, T_GRASS          ,  45, 0, 0 },  // GLOOM -> VILEPLUME (Item, LEAFSTONE)
  {  44, EVOI_SUN         , 182, 0, 0 },  // GLOOM -> BELLOSSOM (Item, SUNSTONE)
  {  58, T_FIRE           ,  59, 0, 0 },  // GROWLITHE -> ARCANINE (Item, FIRESTONE)
  {  61, T_WATER          ,  62, 0, 0 },  // POLIWHIRL -> POLIWRATH (Item, WATERSTONE)
  {  61, EVOI_KINGSROCK   , 186, 0, 0 },  // POLIWHIRL -> POLITOED (DayHoldItem, KINGSROCK)
  {  64, EVOI_CABLE       ,  65, 0, 1 },  // KADABRA -> ALAKAZAM (Trade)
  {  67, EVOI_CABLE       ,  68, 0, 1 },  // MACHOKE -> MACHAMP (Trade)
  {  70, T_GRASS          ,  71, 0, 0 },  // WEEPINBELL -> VICTREEBEL (Item, LEAFSTONE)
  {  75, EVOI_CABLE       ,  76, 0, 1 },  // GRAVELER -> GOLEM (Trade)
  {  79, EVOI_KINGSROCK   , 199, 0, 0 },  // SLOWPOKE -> SLOWKING (DayHoldItem, KINGSROCK)
  {  79, T_WATER          , 199, 0, 0 },  // SLOWPOKE -> SLOWKING (Item, WATERSTONE)
  {  90, T_WATER          ,  91, 0, 0 },  // SHELLDER -> CLOYSTER (Item, WATERSTONE)
  {  93, EVOI_CABLE       ,  94, 0, 1 },  // HAUNTER -> GENGAR (Trade)
  {  95, EVOI_METALCOAT   , 208, 0, 2 },  // ONIX -> STEELIX (TradeItem, METALCOAT)
  { 102, T_GRASS          , 103, 0, 0 },  // EXEGGCUTE -> EXEGGUTOR (Item, LEAFSTONE)
  { 112, EVOI_PROTECTOR   , 464, 0, 2 },  // RHYDON -> RHYPERIOR (TradeItem, PROTECTOR)
  { 117, EVOI_DRAGONSCALE , 230, 0, 2 },  // SEADRA -> KINGDRA (TradeItem, DRAGONSCALE)
  { 120, T_WATER          , 121, 0, 0 },  // STARYU -> STARMIE (Item, WATERSTONE)
  { 122, EVOI_DAWN        , 866, 0, 0 },  // MR. MIME -> MR. RIME (Item, DAWNSTONE)
  { 123, EVOI_METALCOAT   , 212, 0, 2 },  // SCYTHER -> SCIZOR (TradeItem, METALCOAT)
  { 123, EVOI_DUSK        , 900, 0, 0 },  // SCYTHER -> KLEAVOR (Item, DUSKSTONE)
  { 123, EVOI_AUGURITA    , 900, 0, 0 },  // SCYTHER -> KLEAVOR (Item, BLACKAUGURITE)
  { 125, EVOI_ELECTIRIZ   , 466, 0, 2 },  // ELECTABUZZ -> ELECTIVIRE (TradeItem, ELECTIRIZER)
  { 126, EVOI_MAGMARIZ    , 467, 0, 2 },  // MAGMAR -> MAGMORTAR (TradeItem, MAGMARIZER)
  { 133, T_WATER          , 134, 0, 0 },  // EEVEE -> VAPOREON (Item, WATERSTONE)
  { 133, T_ELEC           , 135, 0, 0 },  // EEVEE -> JOLTEON (Item, THUNDERSTONE)
  { 133, T_FIRE           , 136, 0, 0 },  // EEVEE -> FLAREON (Item, FIRESTONE)
  { 133, EVOI_SUN         , 196, 0, 0 },  // EEVEE -> ESPEON (HappinessDay (apano))
  { 133, T_FAIRY          , 197, 0, 0 },  // EEVEE -> UMBREON (HappinessNight (apano))
  { 133, T_GRASS          , 470, 0, 0 },  // EEVEE -> LEAFEON (Item, LEAFSTONE)
  { 133, EVOI_ICESTONE    , 471, 0, 0 },  // EEVEE -> GLACEON (Item, ICESTONE)
  { 133, EVOI_SHINY       , 700, 0, 0 },  // EEVEE -> SYLVEON (Item, SHINYSTONE)
  { 137, EVOI_UPGRADE     , 233, 0, 2 },  // PORYGON -> PORYGON2 (TradeItem, UPGRADE)
  { 176, EVOI_SHINY       , 468, 0, 0 },  // TOGETIC -> TOGEKISS (Item, SHINYSTONE)
  { 191, EVOI_SUN         , 192, 0, 0 },  // SUNKERN -> SUNFLORA (Item, SUNSTONE)
  { 198, EVOI_DUSK        , 430, 0, 0 },  // MURKROW -> HONCHKROW (Item, DUSKSTONE)
  { 200, EVOI_DUSK        , 429, 0, 0 },  // MISDREAVUS -> MISMAGIUS (Item, DUSKSTONE)
  { 233, EVOI_DUBIOUS     , 474, 0, 2 },  // PORYGON2 -> PORYGON-Z (TradeItem, DUBIOUSDISC)
  { 271, T_WATER          , 272, 0, 0 },  // LOMBRE -> LUDICOLO (Item, WATERSTONE)
  { 274, T_GRASS          , 275, 0, 0 },  // NUZLEAF -> SHIFTRY (Item, LEAFSTONE)
  { 281, EVOI_DAWN        , 475, 0, 0 },  // KIRLIA -> GALLADE (Item, DAWNSTONE)
  { 300, T_FAIRY          , 301, 0, 0 },  // SKITTY -> DELCATTY (Item, MOONSTONE)
  { 315, EVOI_SHINY       , 407, 0, 0 },  // ROSELIA -> ROSERADE (Item, SHINYSTONE)
  { 349, EVOI_PRISMSCALE  , 350, 0, 2 },  // FEEBAS -> MILOTIC (TradeItem, PRISMSCALE)
  { 356, EVOI_REAPER      , 477, 0, 2 },  // DUSCLOPS -> DUSKNOIR (TradeItem, REAPERCLOTH)
  { 361, EVOI_DAWN        , 478, 1, 0 },  // SNORUNT -> FROSLASS (ItemFemale, DAWNSTONE)
  { 366, EVOI_SEATOOTH    , 367, 0, 2 },  // CLAMPERL -> HUNTAIL (TradeItem, DEEPSEATOOTH)
  { 366, T_WATER          , 368, 0, 0 },  // CLAMPERL -> GOREBYSS (Item, WATERSTONE)
  { 366, EVOI_SEASCALE    , 368, 0, 2 },  // CLAMPERL -> GOREBYSS (TradeItem, DEEPSEASCALE)
  { 440, EVOI_OVAL        , 113, 0, 0 },  // HAPPINY -> CHANSEY (DayHoldItem, OVALSTONE)
  { 511, T_GRASS          , 512, 0, 0 },  // PANSAGE -> SIMISAGE (Item, LEAFSTONE)
  { 513, T_FIRE           , 514, 0, 0 },  // PANSEAR -> SIMISEAR (Item, FIRESTONE)
  { 515, T_WATER          , 516, 0, 0 },  // PANPOUR -> SIMIPOUR (Item, WATERSTONE)
  { 517, T_FAIRY          , 518, 0, 0 },  // MUNNA -> MUSHARNA (Item, MOONSTONE)
  { 525, EVOI_CABLE       , 526, 0, 1 },  // BOLDORE -> GIGALITH (Trade)
  { 533, EVOI_CABLE       , 534, 0, 1 },  // GURDURR -> CONKELDURR (Trade)
  { 546, EVOI_SUN         , 547, 0, 0 },  // COTTONEE -> WHIMSICOTT (Item, SUNSTONE)
  { 548, EVOI_SUN         , 549, 0, 0 },  // PETILIL -> LILLIGANT (Item, SUNSTONE)
  { 572, EVOI_SHINY       , 573, 0, 0 },  // MINCCINO -> CINCCINO (Item, SHINYSTONE)
  { 603, T_ELEC           , 604, 0, 0 },  // EELEKTRIK -> EELEKTROSS (Item, THUNDERSTONE)
  { 608, EVOI_DUSK        , 609, 0, 0 },  // LAMPENT -> CHANDELURE (Item, DUSKSTONE)
  { 670, EVOI_SHINY       , 671, 0, 0 },  // FLOETTE -> FLORGES (Item, SHINYSTONE)
  { 680, EVOI_DUSK        , 681, 0, 0 },  // DOUBLADE -> AEGISLASH (Item, DUSKSTONE)
  { 682, EVOI_SACHET      , 683, 0, 2 },  // SPRITZEE -> AROMATISSE (TradeItem, SACHET)
  { 684, EVOI_NATA        , 685, 0, 2 },  // SWIRLIX -> SLURPUFF (TradeItem, WHIPPEDDREAM)
  { 694, EVOI_SUN         , 695, 0, 0 },  // HELIOPTILE -> HELIOLISK (Item, SUNSTONE)
  { 708, EVOI_CABLE       , 709, 0, 1 },  // PHANTUMP -> TREVENANT (Trade)
  { 710, EVOI_CABLE       , 711, 0, 1 },  // PUMPKABOO -> GOURGEIST (Trade)
  { 790, EVOI_SHINY       , 791, 0, 0 },  // COSMOEM -> SOLGALEO (Item, SHINYSTONE)
  { 790, EVOI_DUSK        , 792, 0, 0 },  // COSMOEM -> LUNALA (Item, DUSKSTONE)
  { 840, T_GRASS          , 841, 0, 0 },  // APPLIN -> FLAPPLE (Item, LEAFSTONE)
  { 840, EVOI_SHINY       , 842, 0, 0 },  // APPLIN -> APPLETUN (Item, SHINYSTONE)
  { 854, EVOI_DUSK        , 855, 0, 0 },  // SINISTEA -> POLTEAGEIST (Item, DUSKSTONE)
  { 868, EVOI_SHINY       , 869, 0, 0 },  // MILCERY -> ALCREMIE (Item, SHINYSTONE)
  { 891, EVOI_ROLLOSCU    , 892, 0, 0 },  // KUBFU -> URSHIFU (Item, SCROLLOFDARKNESS)
  { 891, EVOI_ROLLOAGU    , 892, 0, 0 },  // KUBFU -> URSHIFU (Item, SCROLLOFWATERS)
  { 935, T_FIRE           , 936, 0, 0 },  // CHARCADET -> ARMAROUGE (Item, FIRESTONE)
  { 935, EVOI_DUSK        , 937, 0, 0 },  // CHARCADET -> CERULEDGE (Item, DUSKSTONE)
  { 938, T_ELEC           , 939, 0, 0 },  // TADBULB -> BELLIBOLT (Item, THUNDERSTONE)
  { 951, T_FIRE           , 952, 0, 0 },  // CAPSAKID -> SCOVILLAIN (Item, FIRESTONE)
  { 1012, T_GRASS          , 1013, 0, 0 },  // POLTCHAGEIST -> SINISTCHA (Item, LEAFSTONE)
};

// Si a este le sirve algun objeto para evolucionar.
static inline bool evoHasItem(int16_t dex) {
  for (uint8_t i = 0; i < EVO_ITEM_COUNT; i++)
    if (EVO_ITEM[i].from == dex) return true;
  return false;
}

// En que se convierte con ESE objeto. 0 = en nada.
//
// `hembra` hace falta porque hay evoluciones que ademas piden
// sexo: Snorunt con la Piedra Alba da Froslass, pero solo si es
// hembra. A un macho la piedra no le hace nada, como en los
// juegos. Por omision `true` para que quien no sepa de sexos (los
// tests viejos, la ficha) siga viendo el destino.
static inline int16_t evoItemTarget(int16_t dex, uint8_t item,
                                    bool hembra = true) {
  for (uint8_t i = 0; i < EVO_ITEM_COUNT; i++) {
    if (EVO_ITEM[i].from != dex || EVO_ITEM[i].item != item) continue;
    if (EVO_ITEM[i].sexo == 1 && !hembra) continue;
    if (EVO_ITEM[i].sexo == 2 && hembra) continue;
    return EVO_ITEM[i].to;
  }
  return 0;
}

// Em que vira ao CHEGAR numa troca, segurando `item` (o `arg` do
// objeto, 0xFF = nada). Primeiro a de objeto (Onix com Metal Coat),
// depois a sem objeto (Kadabra). 0 = nao evolui.
static inline int16_t evoTrocaTarget(int16_t dex, uint8_t item,
                                     bool hembra = true) {
  for (uint8_t i = 0; i < EVO_ITEM_COUNT; i++) {
    const EvoItem &e = EVO_ITEM[i];
    if (e.from != dex || e.troca != 2 || e.item != item) continue;
    if (e.sexo == 1 && !hembra) continue;
    if (e.sexo == 2 && hembra) continue;
    return e.to;
  }
  for (uint8_t i = 0; i < EVO_ITEM_COUNT; i++)
    if (EVO_ITEM[i].from == dex && EVO_ITEM[i].troca == 1)
      return EVO_ITEM[i].to;
  return 0;
}

// Se o objeto (`arg`) e de troca para alguem: e o que faz a bolsa
// perguntar Usar / Segurar.
static inline bool evoItemDeTroca(uint8_t item) {
  for (uint8_t i = 0; i < EVO_ITEM_COUNT; i++)
    if (EVO_ITEM[i].troca == 2 && EVO_ITEM[i].item == item) return true;
  return false;
}

// Que objeto le hace falta, para poder decirselo al jugador.
// 0xFF = ninguno; 0xFE = varios (y entonces elige el jugador).
#define EVO_ITEM_MANY 0xFE
static inline uint8_t evoItemFor(int16_t dex) {
  uint8_t visto = 0xFF;
  for (uint8_t i = 0; i < EVO_ITEM_COUNT; i++) {
    if (EVO_ITEM[i].from != dex) continue;
    if (visto != 0xFF && visto != EVO_ITEM[i].item) return EVO_ITEM_MANY;
    visto = EVO_ITEM[i].item;
  }
  return visto;
}

// Los que SOLO salen por objeto (el Anil no les da ningun camino por
// nivel). gen_dex.py les pone evolveLevel = 0 a partir de esta misma
// lectura del PBS, para que no suban solos y el objeto sirva de algo:
//   PIKACHU, NIDORINA, NIDORINO, CLEFAIRY, VULPIX, JIGGLYPUFF, GLOOM, GROWLITHE, POLIWHIRL, WEEPINBELL, SHELLDER, EXEGGCUTE, STARYU, MR. MIME, EEVEE, TOGETIC, SUNKERN, MURKROW, MISDREAVUS, LOMBRE, NUZLEAF, SKITTY, ROSELIA, HAPPINY, PANSAGE, PANSEAR, PANPOUR, MUNNA, COTTONEE, PETILIL, MINCCINO, EELEKTRIK, LAMPENT, FLOETTE, DOUBLADE, HELIOPTILE, COSMOEM, SINISTEA, MILCERY, KUBFU, CHARCADET, TADBULB, CAPSAKID, POLTCHAGEIST
