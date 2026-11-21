// GENERADO por tools/gen_items.py -- no editar a mano
// La bolsa. La lista va a mano (el PBS tiene 889 objetos y aqui casi
// ninguno significa nada: no hay tienda, ni mapa, ni equipo); de la
// traduccion del propio Anil solo sale el nombre en ingles.
//
// Los bolsillos son los NUESTROS: el PBS mete Restos, piedras y
// repelentes todos en el mismo, y aqui lo que importa es si un objeto se
// LLEVA encima, se USA, o es una bola.
#pragma once
#include <stdint.h>
#include "dex.h"      // T_*
#include "btstate.h"  // ST_* y SG_*: un Antidoto necesita nombrarlos

enum : uint8_t {
  BP_HELD, BP_BATTLE, BP_BALL, BP_EVO, BP_KEY, BP_COUNT,
};

enum : uint8_t {
  IT_NONE = 0,
  IT_LEFTOVERS,   // cura un dieciseisavo al cerrar el turno
  IT_BERRY_HEAL,  // arg = % del maximo; se come sola por debajo de la mitad
  IT_BERRY_FIX,   // igual, pero arg = PS FIJOS (la Aranja cura 10, no un %)
  IT_BERRY_CURE,  // se come sola y quita el estado que haya
  IT_FOCUS_SASH,  // estando ENTERO, aguanta un golpe mortal con 1 PS
  IT_LIFE_ORB,    // arg = % de dano extra; cuesta un 10% de tu vida
  IT_CHOICE,      // +50% (arg: 1 fisico, 0 especial) atado al primer golpe
  IT_QUICK_CLAW,  // arg = % de salir primero aunque seas mas lento
  IT_SCOPE_LENS,  // +1 al escalon de critico
  IT_EVERSTONE,   // no evoluciona mientras lo lleve
  IT_TYPE_BOOST,  // arg = T_*: +20% a los golpes de ese tipo
  // --- los que se USAN, no se llevan ---
  IT_POTION,      // arg = PS que cura (255 = del todo)
  IT_FULLREST,    // PS al maximo y sin estado
  IT_REVIVE,      // arg = % con el que levanta a un desmayado
  IT_CURE,        // arg = ST_* que quita (0 = el que sea)
  IT_ETHER,       // arg = PP a UN golpe (255 = al maximo)
  IT_ELIXIR,      // idem a los cuatro
  IT_XSTAT,       // arg = SG_*: +1 a ese stage, solo en combate
  IT_XCRIT,       // +2 al escalon de critico
  IT_BALL,        // arg = multiplicador x10. Aun no hace nada: no hay captura
  IT_STONE,       // piedra de evolucion. Aqui se evoluciona por nivel, aun no hace nada
  IT_KEY,         // no se gasta y no se tira
  IT_VITAMIN,     // arg = EV_*: +10 a ese EV, hasta el tope
  IT_MEGA,        // piedra mega. arg = MEGA_*: cual de las 121 es
  IT_BALLX,       // bola de condicao. arg = BALLK_*: ballMult() decide
  IT_RARECANDY,   // +1 nivel ao titular
  IT_SHINY,       // o titular fica shiny (premio raro do quiz)
};

// QUE bola de condicao e (o `arg` de IT_BALLX).
enum : uint8_t {
  BALLK_NONE = 0,
  BALLK_QUICK,
  BALLK_TIMER,
  BALLK_DUSK,
  BALLK_NET,
  BALLK_NEST,
  BALLK_REPEAT,
  BALLK_LEVEL,
  BALLK_FAST,
  BALLK_HEAVY,
  BALLK_MOON,
  BALLK_DREAM,
  BALLK_FRIEND,
  BALLK_LUXURY,
  BALLK_PREMIER,
};

// EN QUIEN se usa dentro del combate. Sale del `BattleUse` del PBS
// del Anil, no de una lista escrita a mano: es el dato que decide
// si la bolsa tiene que preguntar a cual del equipo.
enum : uint8_t {
  IU_NONE = 0,   // en combate no se puede usar (las vitaminas)
  IU_BATTLER,    // al que esta en pista, sin preguntar (las X)
  IU_POKEMON,    // hay que ELEGIR a cual del equipo (pociones, revivir)
  IU_FOE,        // contra el rival: las bolas, que van por su camino
};

// La CLAVE de cada objeto de evolucion (el `arg` de IT_STONE). Las
// piedras usan su tipo porque se leia solo; los que no son de ningun
// tipo van detras de T_COUNT, que por definicion no choca con uno.
// Los nombres salen del PBS del Anil, que dice objeto por objeto cual
// pide cada evolucion ("TradeItem,METALCOAT"): antes habia un solo
// "cable" generico para todos, que era inventarselo.
#define EVOI_CABLE       (T_NONE)
#define EVOI_SUN         (T_COUNT)
#define EVOI_KINGSROCK   (T_COUNT + 1)
#define EVOI_DRAGONSCALE (T_COUNT + 2)
#define EVOI_UPGRADE     (T_COUNT + 3)
#define EVOI_METALCOAT   (T_COUNT + 4)
#define EVOI_PRISMSCALE  (T_COUNT + 5)
#define EVOI_SEATOOTH    (T_COUNT + 6)
#define EVOI_SEASCALE    (T_COUNT + 7)
#define EVOI_SHINY       (T_COUNT + 8)
#define EVOI_DUSK        (T_COUNT + 9)
#define EVOI_DAWN        (T_COUNT + 10)
#define EVOI_ICESTONE    (T_COUNT + 11)
#define EVOI_PROTECTOR   (T_COUNT + 12)
#define EVOI_ELECTIRIZ   (T_COUNT + 13)
#define EVOI_MAGMARIZ    (T_COUNT + 14)
#define EVOI_DUBIOUS     (T_COUNT + 15)
#define EVOI_REAPER      (T_COUNT + 16)
#define EVOI_OVAL        (T_COUNT + 17)
#define EVOI_SACHET      (T_COUNT + 18)
#define EVOI_NATA        (T_COUNT + 19)
#define EVOI_AUGURITA    (T_COUNT + 20)
#define EVOI_ROLLOSCU    (T_COUNT + 21)
#define EVOI_ROLLOAGU    (T_COUNT + 22)

// Las piedras MEGA. Son 121 y van numeradas de corrido, no detras
// de T_COUNT como las de evolucion: estas no comparten espacio con
// nada porque su efecto (IT_MEGA) es solo suyo. El numero es lo que
// megas.h guarda en MegaForm::item para saber cual es cual.
#define MEGA_VENUSAURITEX       0
#define MEGA_CHARIZARDITEX      1
#define MEGA_CHARIZARDITEY      2
#define MEGA_BLASTOISINITEX     3
#define MEGA_BEEDRILLITE        4
#define MEGA_PIDGEOTITE         5
#define MEGA_PIKACHUITE         6
#define MEGA_ALAKAZITE          7
#define MEGA_SLOWBRONITE        8
#define MEGA_GENGARITEX         9
#define MEGA_KANGASKHANITE      10
#define MEGA_PINSIRITE          11
#define MEGA_GYARADOSITE        12
#define MEGA_AERODACTYLITE      13
#define MEGA_MEWTWONITEX        14
#define MEGA_MEWTWONITEY        15
#define MEGA_AMPHAROSITE        16
#define MEGA_STEELIXITE         17
#define MEGA_SCIZORITE          18
#define MEGA_HERACRONITE        19
#define MEGA_HOUNDOOMINITE      20
#define MEGA_TYRANITARITE       21
#define MEGA_SCEPTILITE         22
#define MEGA_BLAZIKENITE        23
#define MEGA_SWAMPERTITE        24
#define MEGA_GARDEVOIRITE       25
#define MEGA_SABLENITE          26
#define MEGA_MAWILITE           27
#define MEGA_AGGRONITE          28
#define MEGA_MEDICHAMITE        29
#define MEGA_MANECTITE          30
#define MEGA_SHARPEDONITE       31
#define MEGA_CAMERUPTITE        32
#define MEGA_ALTARIANITE        33
#define MEGA_BANETTITE          34
#define MEGA_ABSOLITE           35
#define MEGA_GLALITITE          36
#define MEGA_SALAMENCITE        37
#define MEGA_METAGROSSITE       38
#define MEGA_LATIASITE          39
#define MEGA_LATIOSITE          40
#define MEGA_RAYQUAZITE         41
#define MEGA_LOPUNNITE          42
#define MEGA_GARCHOMPITE        43
#define MEGA_LUCARIONITE        44
#define MEGA_ABOMASITE          45
#define MEGA_GALLADITE          46
#define MEGA_AUDINITE           47
#define MEGA_DIANCITE           48
#define MEGA_ARCANINITE         49
#define MEGA_VENUSAURITEY       50
#define MEGA_BLASTOISINITEY     51
#define MEGA_GENGARITEY         52
#define MEGA_KINGLERITE         53
#define MEGA_LAPRASITE          54
#define MEGA_MACHAMPITE         55
#define MEGA_GARBODORITE        56
#define MEGA_CORVIKNIGHTITE     57
#define MEGA_ORBEETLEITE        58
#define MEGA_BUTTERFREEITE      59
#define MEGA_DREDNAWITE         60
#define MEGA_COALOSSALITE       61
#define MEGA_FLAPPLETUNITE      62
#define MEGA_SANDACONDITE       63
#define MEGA_TOXTRICITITE       64
#define MEGA_CENTISKORCHITE     65
#define MEGA_HATTERENITE        66
#define MEGA_GRIMMSNARLITE      67
#define MEGA_ALCREMITE          68
#define MEGA_COPPERAJITE        69
#define MEGA_DURALUDONITE       70
#define MEGA_EEVEEITE           71
#define MEGA_JUMPLUFITE         72
#define MEGA_RILLABOOMITE       73
#define MEGA_CINDERECITE        74
#define MEGA_INTELEONITE        75
#define MEGA_SNORLAXITE         76
#define MEGA_CLEFABLITE         77
#define MEGA_STARMINITE         78
#define MEGA_VICTREEBELITE      79
#define MEGA_DRAGONITITE        80
#define MEGA_MEGANIUMITE        81
#define MEGA_FERALIGITE         82
#define MEGA_SKARMORITE         83
#define MEGA_FROSLASSITE        84
#define MEGA_EMBOARITE          85
#define MEGA_EXCADRITE          86
#define MEGA_SCOLIPITE          87
#define MEGA_SCRAFTINITE        88
#define MEGA_EELEKTROSSITE      89
#define MEGA_CHANDELURITE       90
#define MEGA_CHESNAUGHTITE      91
#define MEGA_DELPHOXITE         92
#define MEGA_GRENINJITE         93
#define MEGA_PYROARITE          94
#define MEGA_MALAMARITE         95
#define MEGA_BARBARACITE        96
#define MEGA_DRAGALGITE         97
#define MEGA_HAWLUCHITE         98
#define MEGA_DRAMPANITE         99
#define MEGA_FALINKSITE         100
#define MEGA_FLOETTITE          101
#define MEGA_ZYGARDITE          102
#define MEGA_STARAPTITE         103
#define MEGA_LUCARIONITEZ       104
#define MEGA_CRABOMINITE        105
#define MEGA_MAGEARNITE         106
#define MEGA_ABSOLITEZ          107
#define MEGA_CHIMECHITE         108
#define MEGA_GARCHOMPITEZ       109
#define MEGA_GOLURKITE          110
#define MEGA_TATSUGIRINITE      111
#define MEGA_MEOWSTICITE        112
#define MEGA_BAXCALIBRITE       113
#define MEGA_SCOVILLAINITE      114
#define MEGA_GLIMMORANITE       115
#define MEGA_GOLISOPITE         116
#define MEGA_ZERAORITE          117
#define MEGA_HEATRANITE         118
#define MEGA_DARKRANITE         119
#define MEGA_RAICHUITEX         120
#define MEGA_RAICHUITEY         121

struct ItemEntry {
  const char *name;  // ASCII, de la traduccion inglesa del propio Anil
  uint8_t pocket;    // BP_*
  uint8_t fx;        // IT_*
  uint8_t arg;       // significa una cosa distinta en cada efecto
  uint8_t use;       // IU_*: EN QUIEN se usa dentro del combate
};

#define ITEM_COUNT 230

static const ItemEntry ITEM_TBL[] = {
  { "-", BP_HELD, IT_NONE, 0, IU_NONE },  // 0 = nada / hueco vacio
  { "Leftovers",          BP_HELD,   IT_LEFTOVERS, 0, IU_NONE },  // pbs:LEFTOVERS un dieciseisavo por turno
  { "Sitrus Berry",       BP_HELD,   IT_BERRY_HEAL, 25, IU_POKEMON },  // pbs:SITRUSBERRY se la come a media vida: un cuarto
  { "Oran Berry",         BP_HELD,   IT_BERRY_FIX, 10, IU_POKEMON },  // pbs:ORANBERRY la version barata: diez PS fijos
  { "Lum Berry",          BP_HELD,   IT_BERRY_CURE, 0, IU_POKEMON },  // pbs:LUMBERRY se come el estado que sea
  { "Focus Sash",         BP_HELD,   IT_FOCUS_SASH, 0, IU_NONE },  // pbs:FOCUSSASH aguanta un golpe mortal estando entero
  { "Life Orb",           BP_HELD,   IT_LIFE_ORB, 30, IU_NONE },  // pbs:LIFEORB +30% de dano, y un 10% de tu vida
  { "Choice Band",        BP_HELD,   IT_CHOICE, 1, IU_NONE },  // pbs:CHOICEBAND +50% fisico, pero atado a un golpe
  { "Choice Specs",       BP_HELD,   IT_CHOICE, 0, IU_NONE },  // pbs:CHOICESPECS idem con el especial
  { "Quick Claw",         BP_HELD,   IT_QUICK_CLAW, 20, IU_NONE },  // pbs:QUICKCLAW 20% de salir primero pase lo que pase
  { "Scope Lens",         BP_HELD,   IT_SCOPE_LENS, 0, IU_NONE },  // pbs:SCOPELENS +1 al escalon de critico
  { "Everstone",          BP_HELD,   IT_EVERSTONE, 0, IU_NONE },  // pbs:EVERSTONE no evoluciona mientras lo lleve
  { "Charcoal",           BP_HELD,   IT_TYPE_BOOST, T_FIRE, IU_NONE },  // pbs:CHARCOAL +20% al fuego
  { "Mystic Water",       BP_HELD,   IT_TYPE_BOOST, T_WATER, IU_NONE },  // pbs:MYSTICWATER +20% al agua
  { "Miracle Seed",       BP_HELD,   IT_TYPE_BOOST, T_GRASS, IU_NONE },  // pbs:MIRACLESEED +20% a la planta
  { "Magnet",             BP_HELD,   IT_TYPE_BOOST, T_ELEC, IU_NONE },  // pbs:MAGNET +20% al electrico
  { "Twisted Spoon",      BP_HELD,   IT_TYPE_BOOST, T_PSYCHIC, IU_NONE },  // pbs:TWISTEDSPOON +20% al psiquico
  { "Never-Melt Ice",     BP_HELD,   IT_TYPE_BOOST, T_ICE, IU_NONE },  // pbs:NEVERMELTICE +20% al hielo
  { "Black Belt",         BP_HELD,   IT_TYPE_BOOST, T_FIGHT, IU_NONE },  // pbs:BLACKBELT +20% a la lucha
  { "Sharp Beak",         BP_HELD,   IT_TYPE_BOOST, T_FLYING, IU_NONE },  // pbs:SHARPBEAK +20% al volador
  { "Poison Barb",        BP_HELD,   IT_TYPE_BOOST, T_POISON, IU_NONE },  // pbs:POISONBARB +20% al veneno
  { "Soft Sand",          BP_HELD,   IT_TYPE_BOOST, T_GROUND, IU_NONE },  // pbs:SOFTSAND +20% a la tierra
  { "Hard Stone",         BP_HELD,   IT_TYPE_BOOST, T_ROCK, IU_NONE },  // pbs:HARDSTONE +20% a la roca
  { "Silver Powder",      BP_HELD,   IT_TYPE_BOOST, T_BUG, IU_NONE },  // pbs:SILVERPOWDER +20% al bicho
  { "Spell Tag",          BP_HELD,   IT_TYPE_BOOST, T_GHOST, IU_NONE },  // pbs:SPELLTAG +20% al fantasma
  { "Dragon Fang",        BP_HELD,   IT_TYPE_BOOST, T_DRAGON, IU_NONE },  // pbs:DRAGONFANG +20% al dragon
  { "Black Glasses",      BP_HELD,   IT_TYPE_BOOST, T_DARK, IU_NONE },  // pbs:BLACKGLASSES +20% al siniestro
  { "Silk Scarf",         BP_HELD,   IT_TYPE_BOOST, T_NORMAL, IU_NONE },  // pbs:SILKSCARF +20% al normal
  { "Potion",             BP_BATTLE, IT_POTION, 20, IU_POKEMON },  // pbs:POTION cura 20 PS
  { "Super Potion",       BP_BATTLE, IT_POTION, 60, IU_POKEMON },  // pbs:SUPERPOTION cura 60
  { "Hyper Potion",       BP_BATTLE, IT_POTION, 120, IU_POKEMON },  // pbs:HYPERPOTION cura 120
  { "Max Potion",         BP_BATTLE, IT_POTION, 255, IU_POKEMON },  // pbs:MAXPOTION 255 = del todo
  { "Full Restore",       BP_BATTLE, IT_FULLREST, 0, IU_POKEMON },  // pbs:FULLRESTORE PS al maximo y sin estado
  { "Revive",             BP_BATTLE, IT_REVIVE, 50, IU_POKEMON },  // pbs:REVIVE levanta al desmayado a media vida
  { "Max Revive",         BP_BATTLE, IT_REVIVE, 100, IU_POKEMON },  // pbs:MAXREVIVE ...y entero
  { "Antidote",           BP_BATTLE, IT_CURE, ST_POISON, IU_POKEMON },  // pbs:ANTIDOTE quita el veneno
  { "Paralyze Heal",      BP_BATTLE, IT_CURE, ST_PARALYSIS, IU_POKEMON },  // pbs:PARALYZEHEAL quita la paralisis
  { "Awakening",          BP_BATTLE, IT_CURE, ST_SLEEP, IU_POKEMON },  // pbs:AWAKENING despierta
  { "Burn Heal",          BP_BATTLE, IT_CURE, ST_BURN, IU_POKEMON },  // pbs:BURNHEAL quita la quemadura
  { "Ice Heal",           BP_BATTLE, IT_CURE, ST_FREEZE, IU_POKEMON },  // pbs:ICEHEAL descongela
  { "Full Heal",          BP_BATTLE, IT_CURE, 0, IU_POKEMON },  // pbs:FULLHEAL 0 = el que sea
  { "Ether",              BP_BATTLE, IT_ETHER, 10, IU_POKEMON },  // pbs:ETHER 10 PP a un golpe
  { "Max Ether",          BP_BATTLE, IT_ETHER, 255, IU_POKEMON },  // pbs:MAXETHER un golpe al maximo
  { "Elixir",             BP_BATTLE, IT_ELIXIR, 10, IU_POKEMON },  // pbs:ELIXIR 10 PP a los cuatro
  { "Max Elixir",         BP_BATTLE, IT_ELIXIR, 255, IU_POKEMON },  // pbs:MAXELIXIR los cuatro al maximo
  { "X Attack",           BP_BATTLE, IT_XSTAT, SG_ATK, IU_BATTLER },  // pbs:XATTACK +1 ataque en combate
  { "X Defense",          BP_BATTLE, IT_XSTAT, SG_DEF, IU_BATTLER },  // pbs:XDEFENSE +1 defensa
  { "X Speed",            BP_BATTLE, IT_XSTAT, SG_SPE, IU_BATTLER },  // pbs:XSPEED +1 velocidad
  { "X Sp. Atk",          BP_BATTLE, IT_XSTAT, SG_SPA, IU_BATTLER },  // pbs:XSPATK +1 at. especial
  { "Dire Hit",           BP_BATTLE, IT_XCRIT, 0, IU_BATTLER },  // pbs:DIREHIT +2 al escalon de critico
  { "HP Up",              BP_BATTLE, IT_VITAMIN, EV_HP, IU_NONE },  // pbs:HPUP +10 EV de PS
  { "Protein",            BP_BATTLE, IT_VITAMIN, EV_ATK, IU_NONE },  // pbs:PROTEIN +10 EV de ataque
  { "Iron",               BP_BATTLE, IT_VITAMIN, EV_DEF, IU_NONE },  // pbs:IRON +10 EV de defensa
  { "Calcium",            BP_BATTLE, IT_VITAMIN, EV_SPA, IU_NONE },  // pbs:CALCIUM +10 EV de at. especial
  { "Zinc",               BP_BATTLE, IT_VITAMIN, EV_SPD, IU_NONE },  // pbs:ZINC +10 EV de def. especial
  { "Carbos",             BP_BATTLE, IT_VITAMIN, EV_SPE, IU_NONE },  // pbs:CARBOS +10 EV de velocidad
  { "Poke Ball",          BP_BALL,   IT_BALL, 10, IU_FOE },  // pbs:POKEBALL la de siempre
  { "Great Ball",         BP_BALL,   IT_BALL, 15, IU_FOE },  // pbs:GREATBALL x1,5
  { "Ultra Ball",         BP_BALL,   IT_BALL, 20, IU_FOE },  // pbs:ULTRABALL x2
  { "Master Ball",        BP_BALL,   IT_BALL, 255, IU_FOE },  // pbs:MASTERBALL no falla
  { "Fire Stone",         BP_EVO,    IT_STONE, T_FIRE, IU_NONE },  // pbs:FIRESTONE
  { "Water Stone",        BP_EVO,    IT_STONE, T_WATER, IU_NONE },  // pbs:WATERSTONE
  { "Thunder Stone",      BP_EVO,    IT_STONE, T_ELEC, IU_NONE },  // pbs:THUNDERSTONE
  { "Leaf Stone",         BP_EVO,    IT_STONE, T_GRASS, IU_NONE },  // pbs:LEAFSTONE
  { "Moon Stone",         BP_EVO,    IT_STONE, T_FAIRY, IU_NONE },  // pbs:MOONSTONE
  { "Sun Stone",          BP_EVO,    IT_STONE, EVOI_SUN, IU_NONE },  // pbs:SUNSTONE la del sol
  { "Linking Cord",       BP_EVO,    IT_STONE, EVOI_CABLE, IU_NONE },  // pbs:LINKINGCORD hace de intercambio
  { "King's Rock",        BP_EVO,    IT_STONE, EVOI_KINGSROCK, IU_NONE },  // pbs:KINGSROCK Politoed, Slowking
  { "Dragon Scale",       BP_EVO,    IT_STONE, EVOI_DRAGONSCALE, IU_NONE },  // pbs:DRAGONSCALE Kingdra
  { "Up-Grade",           BP_EVO,    IT_STONE, EVOI_UPGRADE, IU_NONE },  // pbs:UPGRADE Porygon2
  { "Metal Coat",         BP_EVO,    IT_STONE, EVOI_METALCOAT, IU_NONE },  // pbs:METALCOAT Steelix, Scizor
  { "Prism Scale",        BP_EVO,    IT_STONE, EVOI_PRISMSCALE, IU_NONE },  // pbs:PRISMSCALE Milotic
  { "Deep Sea Tooth",     BP_EVO,    IT_STONE, EVOI_SEATOOTH, IU_NONE },  // pbs:DEEPSEATOOTH Huntail
  { "Deep Sea Scale",     BP_EVO,    IT_STONE, EVOI_SEASCALE, IU_NONE },  // pbs:DEEPSEASCALE Gorebyss
  { "Shiny Stone",        BP_EVO,    IT_STONE, EVOI_SHINY, IU_NONE },  // pbs:SHINYSTONE Togekiss, Roserade
  { "Dusk Stone",         BP_EVO,    IT_STONE, EVOI_DUSK, IU_NONE },  // pbs:DUSKSTONE Honchkrow, Mismagius
  { "Dawn Stone",         BP_EVO,    IT_STONE, EVOI_DAWN, IU_NONE },  // pbs:DAWNSTONE Gallade, Froslass
  { "Ice Stone",          BP_EVO,    IT_STONE, EVOI_ICESTONE, IU_NONE },  // pbs:ICESTONE Glaceon
  { "Protector",          BP_EVO,    IT_STONE, EVOI_PROTECTOR, IU_NONE },  // pbs:PROTECTOR Rhyperior
  { "Electirizer",        BP_EVO,    IT_STONE, EVOI_ELECTIRIZ, IU_NONE },  // pbs:ELECTIRIZER Electivire
  { "Magmarizer",         BP_EVO,    IT_STONE, EVOI_MAGMARIZ, IU_NONE },  // pbs:MAGMARIZER Magmortar
  { "Dubious Disc",       BP_EVO,    IT_STONE, EVOI_DUBIOUS, IU_NONE },  // pbs:DUBIOUSDISC Porygon-Z
  { "Reaper Cloth",       BP_EVO,    IT_STONE, EVOI_REAPER, IU_NONE },  // pbs:REAPERCLOTH Dusknoir
  { "Oval Stone",         BP_EVO,    IT_STONE, EVOI_OVAL, IU_NONE },  // pbs:OVALSTONE Chansey
  { "Sachet",             BP_EVO,    IT_STONE, EVOI_SACHET, IU_NONE },  // pbs:SACHET Aromatisse
  { "Sweet Cream",        BP_EVO,    IT_STONE, EVOI_NATA, IU_NONE },  // pbs:WHIPPEDDREAM Slurpuff
  { "Black Mineral",      BP_EVO,    IT_STONE, EVOI_AUGURITA, IU_NONE },  // pbs:BLACKAUGURITE Kleavor
  { "Scroll of Darkness", BP_EVO,    IT_STONE, EVOI_ROLLOSCU, IU_NONE },  // pbs:SCROLLOFDARKNESS Urshifu oscuro
  { "Scroll of Waters",   BP_EVO,    IT_STONE, EVOI_ROLLOAGU, IU_NONE },  // pbs:SCROLLOFWATERS Urshifu de agua
  { "Town Map",           BP_KEY,    IT_KEY, 0, IU_NONE },  // pbs:TOWNMAP de momento, un recuerdo
  { "Venusaurite",        BP_HELD,   IT_MEGA, MEGA_VENUSAURITEX, IU_NONE },  // pbs:VENUSAURITEX Mega Venusaur X
  { "Charizardite X",     BP_HELD,   IT_MEGA, MEGA_CHARIZARDITEX, IU_NONE },  // pbs:CHARIZARDITEX Mega Charizard X
  { "Charizardite Y",     BP_HELD,   IT_MEGA, MEGA_CHARIZARDITEY, IU_NONE },  // pbs:CHARIZARDITEY Mega Charizard Y
  { "Blastoisinite",      BP_HELD,   IT_MEGA, MEGA_BLASTOISINITEX, IU_NONE },  // pbs:BLASTOISINITEX Mega Blastoise X
  { "Beedrillite",        BP_HELD,   IT_MEGA, MEGA_BEEDRILLITE, IU_NONE },  // pbs:BEEDRILLITE Mega Beedrill
  { "Pidgeotite",         BP_HELD,   IT_MEGA, MEGA_PIDGEOTITE, IU_NONE },  // pbs:PIDGEOTITE Mega Pidgeot
  { "Pikachunite Max",    BP_HELD,   IT_MEGA, MEGA_PIKACHUITE, IU_NONE },  // pbs:PIKACHUITE Mega Pikachu
  { "Alakazite",          BP_HELD,   IT_MEGA, MEGA_ALAKAZITE, IU_NONE },  // pbs:ALAKAZITE Mega Alakazam
  { "Slowbronite",        BP_HELD,   IT_MEGA, MEGA_SLOWBRONITE, IU_NONE },  // pbs:SLOWBRONITE Mega Slowbro
  { "Gengarite",          BP_HELD,   IT_MEGA, MEGA_GENGARITEX, IU_NONE },  // pbs:GENGARITEX Mega Gengar X
  { "Kangaskhanite",      BP_HELD,   IT_MEGA, MEGA_KANGASKHANITE, IU_NONE },  // pbs:KANGASKHANITE Mega Kangaskhan
  { "Pinsirite",          BP_HELD,   IT_MEGA, MEGA_PINSIRITE, IU_NONE },  // pbs:PINSIRITE Mega Pinsir
  { "Gyaradosite",        BP_HELD,   IT_MEGA, MEGA_GYARADOSITE, IU_NONE },  // pbs:GYARADOSITE Mega Gyarados
  { "Aerodactylite",      BP_HELD,   IT_MEGA, MEGA_AERODACTYLITE, IU_NONE },  // pbs:AERODACTYLITE Mega Aerodactyl
  { "Mewtwonite X",       BP_HELD,   IT_MEGA, MEGA_MEWTWONITEX, IU_NONE },  // pbs:MEWTWONITEX Mega Mewtwo X
  { "Mewtwonite Y",       BP_HELD,   IT_MEGA, MEGA_MEWTWONITEY, IU_NONE },  // pbs:MEWTWONITEY Mega Mewtwo Y
  { "Ampharosite",        BP_HELD,   IT_MEGA, MEGA_AMPHAROSITE, IU_NONE },  // pbs:AMPHAROSITE Mega Ampharos
  { "Steelixite",         BP_HELD,   IT_MEGA, MEGA_STEELIXITE, IU_NONE },  // pbs:STEELIXITE Mega Steelix
  { "Scizorite",          BP_HELD,   IT_MEGA, MEGA_SCIZORITE, IU_NONE },  // pbs:SCIZORITE Mega Scizor
  { "Heracronite",        BP_HELD,   IT_MEGA, MEGA_HERACRONITE, IU_NONE },  // pbs:HERACRONITE Mega Heracross
  { "Houndoominite",      BP_HELD,   IT_MEGA, MEGA_HOUNDOOMINITE, IU_NONE },  // pbs:HOUNDOOMINITE Mega Houndoom
  { "Tyranitarite",       BP_HELD,   IT_MEGA, MEGA_TYRANITARITE, IU_NONE },  // pbs:TYRANITARITE Mega Tyranitar
  { "Sceptilite",         BP_HELD,   IT_MEGA, MEGA_SCEPTILITE, IU_NONE },  // pbs:SCEPTILITE Mega Sceptile
  { "Blazikenite",        BP_HELD,   IT_MEGA, MEGA_BLAZIKENITE, IU_NONE },  // pbs:BLAZIKENITE Mega Blaziken
  { "Swampertite",        BP_HELD,   IT_MEGA, MEGA_SWAMPERTITE, IU_NONE },  // pbs:SWAMPERTITE Mega Swampert
  { "Gardevoirite",       BP_HELD,   IT_MEGA, MEGA_GARDEVOIRITE, IU_NONE },  // pbs:GARDEVOIRITE Mega Gardevoir
  { "Sablenite",          BP_HELD,   IT_MEGA, MEGA_SABLENITE, IU_NONE },  // pbs:SABLENITE Mega Sableye
  { "Mawilite",           BP_HELD,   IT_MEGA, MEGA_MAWILITE, IU_NONE },  // pbs:MAWILITE Mega Mawile
  { "Aggronite",          BP_HELD,   IT_MEGA, MEGA_AGGRONITE, IU_NONE },  // pbs:AGGRONITE Mega Aggron
  { "Medichamite",        BP_HELD,   IT_MEGA, MEGA_MEDICHAMITE, IU_NONE },  // pbs:MEDICHAMITE Mega Medicham
  { "Manectite",          BP_HELD,   IT_MEGA, MEGA_MANECTITE, IU_NONE },  // pbs:MANECTITE Mega Manectric
  { "Sharpedonite",       BP_HELD,   IT_MEGA, MEGA_SHARPEDONITE, IU_NONE },  // pbs:SHARPEDONITE Mega Sharpedo
  { "Cameruptite",        BP_HELD,   IT_MEGA, MEGA_CAMERUPTITE, IU_NONE },  // pbs:CAMERUPTITE Mega Camerupt
  { "Altarianite",        BP_HELD,   IT_MEGA, MEGA_ALTARIANITE, IU_NONE },  // pbs:ALTARIANITE Mega Altaria
  { "Banettite",          BP_HELD,   IT_MEGA, MEGA_BANETTITE, IU_NONE },  // pbs:BANETTITE Mega Banette
  { "Absolite",           BP_HELD,   IT_MEGA, MEGA_ABSOLITE, IU_NONE },  // pbs:ABSOLITE Mega Absol
  { "Glalitite",          BP_HELD,   IT_MEGA, MEGA_GLALITITE, IU_NONE },  // pbs:GLALITITE Mega Glalie
  { "Salamencite",        BP_HELD,   IT_MEGA, MEGA_SALAMENCITE, IU_NONE },  // pbs:SALAMENCITE Mega Salamence
  { "Metagrossite",       BP_HELD,   IT_MEGA, MEGA_METAGROSSITE, IU_NONE },  // pbs:METAGROSSITE Mega Metagross
  { "Latiasite",          BP_HELD,   IT_MEGA, MEGA_LATIASITE, IU_NONE },  // pbs:LATIASITE Mega Latias
  { "Latiosite",          BP_HELD,   IT_MEGA, MEGA_LATIOSITE, IU_NONE },  // pbs:LATIOSITE Mega Latios
  { "Rayquazite",         BP_HELD,   IT_MEGA, MEGA_RAYQUAZITE, IU_NONE },  // pbs:RAYQUAZITE Mega Rayquaza
  { "Lopunnite",          BP_HELD,   IT_MEGA, MEGA_LOPUNNITE, IU_NONE },  // pbs:LOPUNNITE Mega Lopunny
  { "Garchompite",        BP_HELD,   IT_MEGA, MEGA_GARCHOMPITE, IU_NONE },  // pbs:GARCHOMPITE Mega Garchomp
  { "Lucarionite",        BP_HELD,   IT_MEGA, MEGA_LUCARIONITE, IU_NONE },  // pbs:LUCARIONITE Mega Lucario
  { "Abomasite",          BP_HELD,   IT_MEGA, MEGA_ABOMASITE, IU_NONE },  // pbs:ABOMASITE Mega Abomasnow
  { "Galladite",          BP_HELD,   IT_MEGA, MEGA_GALLADITE, IU_NONE },  // pbs:GALLADITE Mega Gallade
  { "Audinite",           BP_HELD,   IT_MEGA, MEGA_AUDINITE, IU_NONE },  // pbs:AUDINITE Mega Audino
  { "Diancite",           BP_HELD,   IT_MEGA, MEGA_DIANCITE, IU_NONE },  // pbs:DIANCITE Mega Diancie
  { "Arcanite",           BP_HELD,   IT_MEGA, MEGA_ARCANINITE, IU_NONE },  // pbs:ARCANINITE Mega Arcanine
  { "Venusaurite Max",    BP_HELD,   IT_MEGA, MEGA_VENUSAURITEY, IU_NONE },  // pbs:VENUSAURITEY Mega Venusaur Y
  { "Blastoisinite Max",  BP_HELD,   IT_MEGA, MEGA_BLASTOISINITEY, IU_NONE },  // pbs:BLASTOISINITEY Mega Blastoise Y
  { "Gengarite Max",      BP_HELD,   IT_MEGA, MEGA_GENGARITEY, IU_NONE },  // pbs:GENGARITEY Mega Gengar Y
  { "Kinglerite Max",     BP_HELD,   IT_MEGA, MEGA_KINGLERITE, IU_NONE },  // pbs:KINGLERITE Mega Kingler
  { "Laprasite Max",      BP_HELD,   IT_MEGA, MEGA_LAPRASITE, IU_NONE },  // pbs:LAPRASITE Mega Lapras
  { "Machampite Max",     BP_HELD,   IT_MEGA, MEGA_MACHAMPITE, IU_NONE },  // pbs:MACHAMPITE Mega Machamp
  { "Garbodorite Max",    BP_HELD,   IT_MEGA, MEGA_GARBODORITE, IU_NONE },  // pbs:GARBODORITE Mega Garbodor
  { "Corviknightite Max", BP_HELD,   IT_MEGA, MEGA_CORVIKNIGHTITE, IU_NONE },  // pbs:CORVIKNIGHTITE Mega Corviknight
  { "Orbeetlite Max",     BP_HELD,   IT_MEGA, MEGA_ORBEETLEITE, IU_NONE },  // pbs:ORBEETLEITE Mega Orbeetle
  { "Butterfrite Max",    BP_HELD,   IT_MEGA, MEGA_BUTTERFREEITE, IU_NONE },  // pbs:BUTTERFREEITE Mega Butterfree
  { "Drednawite Max",     BP_HELD,   IT_MEGA, MEGA_DREDNAWITE, IU_NONE },  // pbs:DREDNAWITE Mega Drednaw
  { "Coalossalite Max",   BP_HELD,   IT_MEGA, MEGA_COALOSSALITE, IU_NONE },  // pbs:COALOSSALITE Mega Coalossal
  { "Flappletunite Max",  BP_HELD,   IT_MEGA, MEGA_FLAPPLETUNITE, IU_NONE },  // pbs:FLAPPLETUNITE Mega Flapple
  { "Sandacondite Max",   BP_HELD,   IT_MEGA, MEGA_SANDACONDITE, IU_NONE },  // pbs:SANDACONDITE Mega Sandconda
  { "Toxtricitite Max",   BP_HELD,   IT_MEGA, MEGA_TOXTRICITITE, IU_NONE },  // pbs:TOXTRICITITE Mega Toxtricity
  { "Centiskorchite Max", BP_HELD,   IT_MEGA, MEGA_CENTISKORCHITE, IU_NONE },  // pbs:CENTISKORCHITE Mega Centiskorch
  { "Hatterenite Max",    BP_HELD,   IT_MEGA, MEGA_HATTERENITE, IU_NONE },  // pbs:HATTERENITE Mega Hatterene
  { "Grimmsnarlite Max",  BP_HELD,   IT_MEGA, MEGA_GRIMMSNARLITE, IU_NONE },  // pbs:GRIMMSNARLITE Mega Grimmsnarl
  { "Alcremite Max",      BP_HELD,   IT_MEGA, MEGA_ALCREMITE, IU_NONE },  // pbs:ALCREMITE Mega Alcremie
  { "Copperajahite Max",  BP_HELD,   IT_MEGA, MEGA_COPPERAJITE, IU_NONE },  // pbs:COPPERAJITE Mega Copperajah
  { "Duraludonite Max",   BP_HELD,   IT_MEGA, MEGA_DURALUDONITE, IU_NONE },  // pbs:DURALUDONITE Mega Duraludon
  { "Eevite Max",         BP_HELD,   IT_MEGA, MEGA_EEVEEITE, IU_NONE },  // pbs:EEVEEITE Mega Eevee
  { "Jumpluffite",        BP_HELD,   IT_MEGA, MEGA_JUMPLUFITE, IU_NONE },  // pbs:JUMPLUFITE Mega Jumpluff
  { "Rillaboomite Max",   BP_HELD,   IT_MEGA, MEGA_RILLABOOMITE, IU_NONE },  // pbs:RILLABOOMITE Mega Rillaboom
  { "Cinderacite Max",    BP_HELD,   IT_MEGA, MEGA_CINDERECITE, IU_NONE },  // pbs:CINDERECITE Mega Cinderace
  { "Inteleonite Max",    BP_HELD,   IT_MEGA, MEGA_INTELEONITE, IU_NONE },  // pbs:INTELEONITE Mega Inteleon
  { "Snorlaxite Max",     BP_HELD,   IT_MEGA, MEGA_SNORLAXITE, IU_NONE },  // pbs:SNORLAXITE Mega Snorlax
  { "Clefablite",         BP_HELD,   IT_MEGA, MEGA_CLEFABLITE, IU_NONE },  // pbs:CLEFABLITE Mega Clefable
  { "Starminite",         BP_HELD,   IT_MEGA, MEGA_STARMINITE, IU_NONE },  // pbs:STARMINITE Mega Starmie
  { "Victreebelite",      BP_HELD,   IT_MEGA, MEGA_VICTREEBELITE, IU_NONE },  // pbs:VICTREEBELITE Mega Victreebel
  { "Dragoninite",        BP_HELD,   IT_MEGA, MEGA_DRAGONITITE, IU_NONE },  // pbs:DRAGONITITE Mega Dragonite
  { "Meganiumite",        BP_HELD,   IT_MEGA, MEGA_MEGANIUMITE, IU_NONE },  // pbs:MEGANIUMITE Mega Meganium
  { "Feraligite",         BP_HELD,   IT_MEGA, MEGA_FERALIGITE, IU_NONE },  // pbs:FERALIGITE Mega Feraligatr
  { "Skarmorite",         BP_HELD,   IT_MEGA, MEGA_SKARMORITE, IU_NONE },  // pbs:SKARMORITE Mega Skarmory
  { "Froslassite",        BP_HELD,   IT_MEGA, MEGA_FROSLASSITE, IU_NONE },  // pbs:FROSLASSITE Mega Froslass
  { "Emboarite",          BP_HELD,   IT_MEGA, MEGA_EMBOARITE, IU_NONE },  // pbs:EMBOARITE Mega Emboar
  { "Excadrite",          BP_HELD,   IT_MEGA, MEGA_EXCADRITE, IU_NONE },  // pbs:EXCADRITE Mega Excadrill
  { "Scolipite",          BP_HELD,   IT_MEGA, MEGA_SCOLIPITE, IU_NONE },  // pbs:SCOLIPITE Mega Scolipede
  { "Scraftinite",        BP_HELD,   IT_MEGA, MEGA_SCRAFTINITE, IU_NONE },  // pbs:SCRAFTINITE Mega Scrafty
  { "Eelektrossite",      BP_HELD,   IT_MEGA, MEGA_EELEKTROSSITE, IU_NONE },  // pbs:EELEKTROSSITE Mega Eelektross
  { "Chandelurite",       BP_HELD,   IT_MEGA, MEGA_CHANDELURITE, IU_NONE },  // pbs:CHANDELURITE Mega Chandelure
  { "Chesnaughtite",      BP_HELD,   IT_MEGA, MEGA_CHESNAUGHTITE, IU_NONE },  // pbs:CHESNAUGHTITE Mega Chesnaught
  { "Delphoxite",         BP_HELD,   IT_MEGA, MEGA_DELPHOXITE, IU_NONE },  // pbs:DELPHOXITE Mega Delphox
  { "Greninjite",         BP_HELD,   IT_MEGA, MEGA_GRENINJITE, IU_NONE },  // pbs:GRENINJITE Mega Greninja
  { "Pyroarite",          BP_HELD,   IT_MEGA, MEGA_PYROARITE, IU_NONE },  // pbs:PYROARITE Mega Pyroar
  { "Malamarite",         BP_HELD,   IT_MEGA, MEGA_MALAMARITE, IU_NONE },  // pbs:MALAMARITE Mega Malamar
  { "Barbaracite",        BP_HELD,   IT_MEGA, MEGA_BARBARACITE, IU_NONE },  // pbs:BARBARACITE Mega Barbaracle
  { "Dragalgite",         BP_HELD,   IT_MEGA, MEGA_DRAGALGITE, IU_NONE },  // pbs:DRAGALGITE Mega Dragalge
  { "Hawluchanite",       BP_HELD,   IT_MEGA, MEGA_HAWLUCHITE, IU_NONE },  // pbs:HAWLUCHITE Mega Hawlucha
  { "Drampanite",         BP_HELD,   IT_MEGA, MEGA_DRAMPANITE, IU_NONE },  // pbs:DRAMPANITE Mega Drampa
  { "Falinksite",         BP_HELD,   IT_MEGA, MEGA_FALINKSITE, IU_NONE },  // pbs:FALINKSITE Mega Falinks
  { "Floettite",          BP_HELD,   IT_MEGA, MEGA_FLOETTITE, IU_NONE },  // pbs:FLOETTITE Mega Floette
  { "Zygardite",          BP_HELD,   IT_MEGA, MEGA_ZYGARDITE, IU_NONE },  // pbs:ZYGARDITE Mega Zygarde
  { "Staraptorite",       BP_HELD,   IT_MEGA, MEGA_STARAPTITE, IU_NONE },  // pbs:STARAPTITE Mega Staraptor
  { "Lucarite Z",         BP_HELD,   IT_MEGA, MEGA_LUCARIONITEZ, IU_NONE },  // pbs:LUCARIONITEZ Mega Lucario Z
  { "Crabominablite",     BP_HELD,   IT_MEGA, MEGA_CRABOMINITE, IU_NONE },  // pbs:CRABOMINITE Mega Crabominable
  { "Magearnite",         BP_HELD,   IT_MEGA, MEGA_MAGEARNITE, IU_NONE },  // pbs:MAGEARNITE Mega Magearna
  { "Absolite Z",         BP_HELD,   IT_MEGA, MEGA_ABSOLITEZ, IU_NONE },  // pbs:ABSOLITEZ Mega Absol Z
  { "Chimechite",         BP_HELD,   IT_MEGA, MEGA_CHIMECHITE, IU_NONE },  // pbs:CHIMECHITE Mega Chimecho
  { "Garchompite Z",      BP_HELD,   IT_MEGA, MEGA_GARCHOMPITEZ, IU_NONE },  // pbs:GARCHOMPITEZ Mega Garchomp Z
  { "Golurkite",          BP_HELD,   IT_MEGA, MEGA_GOLURKITE, IU_NONE },  // pbs:GOLURKITE Mega Golurk
  { "Tatsugirite",        BP_HELD,   IT_MEGA, MEGA_TATSUGIRINITE, IU_NONE },  // pbs:TATSUGIRINITE Mega Tatsugiri Forma Curvada
  { "Meowsticite",        BP_HELD,   IT_MEGA, MEGA_MEOWSTICITE, IU_NONE },  // pbs:MEOWSTICITE Mega Meowstic
  { "Baxcaliburite",      BP_HELD,   IT_MEGA, MEGA_BAXCALIBRITE, IU_NONE },  // pbs:BAXCALIBRITE Mega Baxcalibur
  { "Scovillainite",      BP_HELD,   IT_MEGA, MEGA_SCOVILLAINITE, IU_NONE },  // pbs:SCOVILLAINITE Mega Scovillain
  { "Glimmoranite",       BP_HELD,   IT_MEGA, MEGA_GLIMMORANITE, IU_NONE },  // pbs:GLIMMORANITE Mega Glimmora
  { "Golisopodite",       BP_HELD,   IT_MEGA, MEGA_GOLISOPITE, IU_NONE },  // pbs:GOLISOPITE Mega Golisopod
  { "Zeraoranite",        BP_HELD,   IT_MEGA, MEGA_ZERAORITE, IU_NONE },  // pbs:ZERAORITE Mega Zeraora
  { "Heatranite",         BP_HELD,   IT_MEGA, MEGA_HEATRANITE, IU_NONE },  // pbs:HEATRANITE Mega Heatran
  { "Darkrainite",        BP_HELD,   IT_MEGA, MEGA_DARKRANITE, IU_NONE },  // pbs:DARKRANITE Mega Darkrai
  { "Raichunite X",       BP_HELD,   IT_MEGA, MEGA_RAICHUITEX, IU_NONE },  // pbs:RAICHUITEX Mega Raichu x
  { "Raichunite Y",       BP_HELD,   IT_MEGA, MEGA_RAICHUITEY, IU_NONE },  // pbs:RAICHUITEY Mega Raichu Y
  { "Pokedex",            BP_KEY,    IT_KEY, 0, IU_NONE },  // pbs:POKEDEX enfeite
  { "Mega Ring",          BP_KEY,    IT_KEY, 0, IU_NONE },  // pbs:MEGARING enfeite
  { "Quick Ball",         BP_BALL,   IT_BALLX, BALLK_QUICK, IU_FOE },  // pbs:QUICKBALL x5 no 1o turno
  { "Timer Ball",         BP_BALL,   IT_BALLX, BALLK_TIMER, IU_FOE },  // pbs:TIMERBALL melhora a cada turno
  { "Dusk Ball",          BP_BALL,   IT_BALLX, BALLK_DUSK, IU_FOE },  // pbs:DUSKBALL x3 de noite
  { "Net Ball",           BP_BALL,   IT_BALLX, BALLK_NET, IU_FOE },  // pbs:NETBALL x3,5 inseto/agua
  { "Nest Ball",          BP_BALL,   IT_BALLX, BALLK_NEST, IU_FOE },  // pbs:NESTBALL melhor em nivel baixo
  { "Repeat Ball",        BP_BALL,   IT_BALLX, BALLK_REPEAT, IU_FOE },  // pbs:REPEATBALL x3,5 se ja capturou
  { "Level Ball",         BP_BALL,   IT_BALLX, BALLK_LEVEL, IU_FOE },  // pbs:LEVELBALL pelo seu nivel
  { "Fast Ball",          BP_BALL,   IT_BALLX, BALLK_FAST, IU_FOE },  // pbs:FASTBALL x4 nos rapidos
  { "Heavy Ball",         BP_BALL,   IT_BALLX, BALLK_HEAVY, IU_FOE },  // pbs:HEAVYBALL melhor nos pesados
  { "Moon Ball",          BP_BALL,   IT_BALLX, BALLK_MOON, IU_FOE },  // pbs:MOONBALL x4 nos da Moon Stone
  { "Dream Ball",         BP_BALL,   IT_BALLX, BALLK_DREAM, IU_FOE },  // pbs:DREAMBALL x4 no adormecido
  { "Friend Ball",        BP_BALL,   IT_BALLX, BALLK_FRIEND, IU_FOE },  // pbs:FRIENDBALL chega com vinculo alto
  { "Luxury Ball",        BP_BALL,   IT_BALLX, BALLK_LUXURY, IU_FOE },  // pbs:LUXURYBALL o vinculo cresce mais
  { "Premier Ball",       BP_BALL,   IT_BALLX, BALLK_PREMIER, IU_FOE },  // pbs:PREMIERBALL comemorativa
  { "Rare Candy",         BP_BATTLE, IT_RARECANDY, 0, IU_NONE },  // pbs:RARECANDY +1 nivel
  { "Shiny Potion",       BP_BATTLE, IT_SHINY, 0, IU_NONE },  // pbs:SHINYZADOR o titular fica shiny
};

// Que hace cada objeto, en ingles del propio Anil.
static const char *const ITEM_DESC[ITEM_COUNT] = {
  "",
  "An item to be held by a Pokemon. The holder's HP is gradually restored during battle.",
  "A Poffin ingredient. It may be used or held by a Pokemon to restore some HP.",
  "A Poffin ingredient. It may be used or held by a Pokemon to restore 10 HP.",
  "A Poffin ingredient. It may be used or held by a Pokemon to heal any status condition.",
  "An item to be held by a Pokemon. If it has full HP, the holder will endure one potential KO attack with 1 HP.",
  "Boosts moves, but consumes 30% of HP with each attack. Must be held by a Pokemon.",
  "Boosts Attack by 50% but only allows the use of one type of move 50% of the time. Must be held by a Pokemon.",
  "Boosts Sp. Atk by 50% but only allows the use of one type of move. Must be held by a Pokemon.",
  "An item to be held by a Pokemon. A light, sharp claw that lets the bearer move first occasionally.",
  "A lens that increases the chance of scoring a critical hit.",
  "An item to be held by a Pokemon. The Pokemon holding this peculiar stone is prevented from evolving.",
  "An item to be held by a Pokemon. It is a combustible fuel that boosts the power of Fire-type moves.",
  "A teardrop-shaped gem that boosts the power of Water-type moves by 20%. Must be held by a Pokemon.",
  "An item to be held by a Pokemon. It is a seed imbued with life that ups the power of Grass-type moves.",
  "An item to be held by a Pokemon. It is a powerful magnet that boosts the power of Electric-type moves.",
  "An item to be held by a Pokemon. It is a spoon imbued with telekinetic power that boosts Psychic-type moves.",
  "An item to be held by a Pokemon. It is a piece of ice that repels heat and boosts Ice-type moves.",
  "An item to be held by a Pokemon. It is a belt that boosts determination and Fighting-type moves.",
  "An item to be held by a Pokemon. It is a long, sharp beak that boosts the power of Flying-type moves.",
  "A poisonous barb that boosts the power of Poison-type moves by 20%. Must be held by a Pokemon.",
  "An item to be held by a Pokemon. It is a loose, silky sand that boosts the power of Ground-type moves.",
  "An item to be held by a Pokemon. It is an unbreakable stone that ups the power of Rock-type moves.",
  "An item to be held by a Pokemon. It is a shiny, silver powder that ups the power of Bug-type moves.",
  "An item to be held by a Pokemon. It is a sinister, eerie tag that boosts the power of Ghost-type moves.",
  "An item to be held by a Pokemon. It is a hard and sharp fang that ups the power of Dragon-type moves.",
  "An item to be held by a Pokemon. It is a shady-looking pair of glasses that boosts Dark-type moves.",
  "An item to be held by a Pokemon. It is a sumptuous scarf that boosts the power of Normal-type moves.",
  "A spray-type medicine for wounds. It restores the HP of one Pokemon by 20 points.",
  "A spray-type medicine for wounds. It restores the HP of one Pokemon by 60 points.",
  "A spray-type medicine for wounds. It restores the HP of one Pokemon by 120 points.",
  "A spray-type medicine for wounds. It completely restores the HP of a single Pokemon.",
  "A medicine that fully restores the HP and heals any status problems of a single Pokemon.",
  "A medicine that revives a fainted Pokemon. It restores half the Pokemon's maximum HP.",
  "A medicine that revives a fainted Pokemon. It fully restores the Pokemon's HP.",
  "A spray-type medicine. It lifts the effect of poison from one Pokemon.",
  "A spray-type medicine. It eliminates paralysis from a single Pokemon.",
  "A spray-type medicine. It awakens a Pokemon from the clutches of sleep.",
  "A spray-type medicine. It heals a single Pokemon that is suffering from a burn.",
  "A spray-type medicine. It defrosts a Pokemon that has been frozen solid.",
  "A spray-type medicine. It heals all the status problems of a single Pokemon.",
  "It restores the PP of a Pokemon's selected move by a maximum of 10 points.",
  "It fully restores the PP of a single selected move that has been learned by the target Pokemon.",
  "It restores the PP of all the moves learned by the targeted Pokemon by 10 points each.",
  "It fully restores the PP of all the moves learned by the targeted Pokemon.",
  "Increases Attack by 1 stage in battle. The effect wears off if the Pokemon switches out.",
  "Increases Defense by 1 stage in battle. The effect wears off if the Pokemon switches out.",
  "Increases Speed by 1 stage in battle. The effect wears off if the Pokemon switches out.",
  "Increases Sp. Atk by 1 stage in battle. The effect wears off if the Pokemon switches out.",
  "Increases critical-hit ratio by 1 stage in battle. The effect wears off if the Pokemon switches out.",
  "A nutritious drink that increases a Pokemon's HP.",
  "A nutritious drink that increases a Pokemon's Attack.",
  "A nutritious drink that increases a Pokemon's Defense.",
  "A nutritious drink that raises the Sp. Atk of a Pokemon.",
  "A nutritious drink that increases a Pokemon's Special Defense.",
  "A nutritious drink that increases a Pokemon's Speed.",
  "A device for catching wild Pokemon. It is thrown like a ball at a Pokemon. It is designed as a capsule system.",
  "A good, high-performance Ball that provides a higher Pokemon catch rate than a standard Poke Ball.",
  "An ultra-performance Ball that provides a higher Pokemon catch rate than a Great Ball.",
  "The best Ball with the ultimate level of performance. It will catch any wild Pokemon without fail.",
  "A peculiar stone that can make certain species of Pokemon evolve. It has a fiery orange heart.",
  "A peculiar stone that can make certain species of Pokemon evolve. It is the blue of clear water.",
  "A peculiar stone that can make certain species of Pokemon evolve. It has a thunderbolt pattern.",
  "A peculiar stone that can make certain species of Pokemon evolve. It has a leaf pattern.",
  "A peculiar stone that can make certain species of Pokemon evolve. It is as black as jet.",
  "A peculiar stone that can make certain species of Pokemon evolve. It is as red as the sun.",
  "A string that exudes a mysterious energy. It makes certain Pokemon evolve.",
  "An item to be held by a Pokemon. When the holder inflicts damage, the target may flinch.",
  "A thick and tough scale. Dragon-type Pokemon may be holding this item when caught.",
  "A peculiar device manufactured by Silph Co. that contains all kinds of data. Allows Porygon to evolve.",
  "An item to be held by a Pokemon. It is a special metallic film that ups the power of Steel-type moves.",
  "A beautiful scale that allows Feebas to evolve. It glows with the colors of the rainbow.",
  "An item to be held by Clamperl. A fang that gleams a sharp silver, it raises the Sp. Atk stat.",
  "An item to be held by Clamperl. A scale that shines a faint pink, it raises the Sp. Def stat.",
  "A peculiar stone that can make certain species of Pokemon evolve. It shines with a dazzling light.",
  "A peculiar stone that can make certain species of Pokemon evolve. It is as dark as the night sky.",
  "A peculiar stone that can make certain species of Pokemon evolve. It sparkles like a glittering eye.",
  "A peculiar stone that can make certain species of Pokemon evolve. It has a snowy pattern.",
  "A very heavy protective device. It is used to evolve Rhydon.",
  "This device contains a large amount of electrical energy. It is used to evolve Electabuzz.",
  "This box contains an enormous amount of magmatic energy. It is used to evolve Magmar.",
  "A transparent device containing mysterious data. Allows Porygon2 to evolve.",
  "Cloth imbued with a very powerful spiritual energy. It is used to evolve Dusclops.",
  "A peculiar stone that allows Happiny to evolve. It is egg-shaped.",
  "A sachet that gives off a peculiar aroma. It is used to evolve Spritzee.",
  "A sweet based on soft and fluffy whipped cream. It is used to evolve Swirlix.",
  "A glassy black stone that makes Scyther evolve.",
  "A peculiar scroll that allows Kubfu to evolve into Single Strike Style Urshifu.",
  "A peculiar scroll that allows Kubfu to evolve into Rapid Strike Style Urshifu.",
  "A very convenient map that can be viewed at any time. It even shows your present location.",
  "One of the mysterious Mega Stones. It allows Venusaur to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Charizard to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Charizard to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Blastoise to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Beedrill to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Pidgeot to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Pikachu to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Alakazam to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Slowbro to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Gengar to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Kangaskhan to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Pinsir to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Gyarados to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Aerodactyl to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Mewtwo to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Mewtwo to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Ampharos to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Steelix to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Scizor to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Heracross to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Houndoom to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Tyranitar to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Sceptile to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Blaziken to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Swampert to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Gardevoir to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Sableye to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Mawile to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Aggron to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Medicham to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Manectric to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Sharpedo to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Camerupt to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Altaria to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Banette to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Absol to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Glalie to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Salamence to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Metagross to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Latias to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Latios to Mega Evolve in battle.",
  "",
  "One of the mysterious Mega Stones. It allows Lopunny to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Garchomp to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Lucario to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Abomasnow to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Gallade to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Audino to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Diancie to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. Allows Arcanine to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Venusaur to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Blastoise to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Gengar to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Kingler to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Lapras to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Machamp to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Garbodor to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Corviknight to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Orbeetle to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Butterfree to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Drednaw to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Coalossal to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Flapple or Appletun to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Sandaconda to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Toxtricity to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Centiskorch to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Hatterene to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Grimmsnarl to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Alcremie to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Copperajah to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Duraludon to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Eevee to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Jumpluff to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Rillaboom to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Cinderace to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Inteleon to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Snorlax to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Clefable to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Starmie to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Victreebel to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Dragonite to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Meganium to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Feraligatr to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Skarmory to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Froslass to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Emboar to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Excadrill to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Scolipede to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Scrafty to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Eelektross to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Chandelure to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Chesnaught to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Delphox to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Greninja to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Pyroar to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Malamar to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Barbaracle to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Dragalge to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Hawlucha to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Drampa to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Falinks to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Eternal Flower Floette to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Zygarde to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. Allows Mega Evolution of Staraptor in battle.",
  "One of the mysterious Mega Stones. It allows Lucario to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. Allows Mega Evolution of Crabominable in battle.",
  "One of the mysterious Mega Stones. Allows Mega Evolution of Magearna in battle.",
  "One of the mysterious Mega Stones. It allows Absol to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Chimecho to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Garchomp to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. Allows Mega Evolution of Golurk in battle.",
  "One of the mysterious Mega Stones. Allows Mega Evolution of Tatsugiri in battle.",
  "One of the mysterious Mega Stones. Allows Mega Evolution of Meowstic in battle.",
  "One of the mysterious Mega Stones. It allows Baxcalibur to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. Allows Mega Evolution of Scovillain in battle.",
  "One of the mysterious Mega Stones. Allows Mega Evolution of Glimmora in battle.",
  "One of the mysterious Mega Stones. Allows Mega Evolution of Golisopod in battle.",
  "One of the mysterious Mega Stones. Allows Mega Evolution of Zeraora in battle.",
  "One of the mysterious Mega Stones. Allows Mega Evolution of Heatran in battle.",
  "One of the mysterious Mega Stones. Allows Mega Evolution of Darkrai in battle.",
  "One of the mysterious Mega Stones. It allows Raichu to Mega Evolve in battle.",
  "One of the mysterious Mega Stones. It allows Raichu to Mega Evolve in battle.",
  "A high-tech encyclopedia that records every Pokemon you have seen and caught.",
  "A ring that enables a Trainer to Mega Evolve Pokemon.",
  "A Poke Ball that has a better catch rate if used at the start of a wild encounter.",
  "A somewhat different Poke Ball that becomes progressively better the more turns there are in a battle.",
  "A somewhat different Poke Ball that makes it easier to catch wild Pokemon at night or in dark places like caves.",
  "A somewhat different Poke Ball that works especially well on Water- and Bug-type Pokemon.",
  "A somewhat different Poke Ball that works especially well on weaker Pokemon in the wild.",
  "A somewhat different Poke Ball that works especially well on Pokemon species that were previously caught.",
  "A Poke Ball that makes it easier to catch Pokemon that are at a lower level than your own Pokemon.",
  "A somewhat different Poke Ball that makes it easier to catch fast Pokemon.",
  "A Poke Ball that is better than usual at catching very heavy Pokemon.",
  "A Poke Ball that works best when catching Pokemon that evolve using a Moon Stone.",
  "A somewhat different Poke Ball that works better when the Pokemon is asleep.",
  "A Poke Ball that makes caught Pokemon more friendly.",
  "A comfortable Poke Ball that makes a caught wild Pokemon quickly grow friendly.",
  "A quite rare Poke Ball that has been specially crafted to commemorate an occasion of some sort.",
  "A candy that is packed with energy. It raises the level of a single Pokemon by one.",
  "A sparkling potion. The Pokemon that drinks it becomes a shiny Pokemon for good.",
};

// Lo mismo en ES, de el PBS del Anil. Misma fila.
static const char *const ITEM_DESC_ES[ITEM_COUNT] = {
  "",
  "Restauran gradualmente los PS del Pok\202mon que los lleva durante el combate.",
  "Los Pok\202mon pueden usarla o llevarla para restaurar algunos PS.",
  "Los Pok\202mon pueden usarla o llevarla para restaurar 10 PS.",
  "Los Pok\202mon pueden llevarla o usarla para recuperarse de problemas de estado.",
  "Si el Pok\202mon que la lleva tiene los PS al m\240ximo, resistir\240 un golpe que potencialmente lo deja KO con 1 PS.",
  "Potencia los movimientos, pero consume PS con cada ataque un 30%. Debe llevarla un Pok\202mon.",
  "Aumenta el Ataque un 50% pero solo permite usar un tipo de movimiento un 50%. Debe llevarla un Pok\202mon.",
  "Potencian el At. Esp. un 50% pero solo permiten usar un tipo de movimiento. Debe llevarlas un Pok\202mon.",
  "Garra ligera y afilada que permite que el Pok\202mon que la lleve ataque primero ocasionalmente.",
  "Su lente aumenta la posibilidad que tiene el Pok\202mon que lo lleva de conseguir un golpe cr\241tico.",
  "El Pok\202mon que lleve esta piedra m\240gica no evolucionar\240.",
  "Act\243a como combustible y potencia la energ\241a de los ataques de tipo Fuego. Debe llevarlo un Pok\202mon.",
  "Gema con forma de l\240grima que fortalece los ataques de tipo Agua un 20%. Debe llevarla un Pok\202mon.",
  "Debe llevarla un Pok\202mon. La vitalidad de la semilla fortalece los ataques de tipo Planta.",
  "Debe llevarlo un Pok\202mon. Su potente magnetismo fortalece los ataques de tipo El\202ctrico.",
  "Cuchara llena de energ\241a telequin\202tica que potencia los movimientos de tipo Ps\241quico.",
  "Fragmento de hielo que repele el calor y fortalece los ataques de tipo Hielo. Debe llevarlo un Pok\202mon.",
  "Debe llevarlo un Pok\202mon. Aumenta la decisi\242n y fortalece los ataques de tipo Lucha.",
  "Pico largo y afilado que aumenta la potencia de los movimientos de tipo Volador. Debe llevarlo un Pok\202mon.",
  "Flecha venenosa que refuerza los movimientos de tipo Veneno un 20%. Debe llevarla un Pok\202mon.",
  "Debe llevarla un Pok\202mon. La suavidad de la arena fortalece los ataques de tipo Tierra.",
  "Debe llevarla un Pok\202mon. La robustez de la piedra fortalece los ataques de tipo Roca.",
  "Polvo de brillo arg\202nteo que fortalece los ataques de tipo Bicho. Debe llevarlo un Pok\202mon.",
  "Misterioso y oscuro objeto que refuerza los ataques de tipo Fantasma. Debe llevarlo un Pok\202mon.",
  "Colmillo de gran dureza que refuerza los ataques de tipo Drag\242n. Debe llevarlo un Pok\202mon.",
  "Debe llevarlas un Pok\202mon. Fortalecen los ataques de tipo Siniestro.",
  "La delicadeza del pa\244uelo fortalece los ataques de tipo Normal. Debe llevarlo un Pok\202mon.",
  "Medicina en spray que cura heridas y restaura 20 PS de un Pok\202mon.",
  "Medicina en spray que cura heridas y restaura 60 PS de un Pok\202mon.",
  "Medicina en spray que cura heridas y restaura 120 PS de un Pok\202mon.",
  "Medicina en spray que cura heridas y restaura todos los PS de un Pok\202mon.",
  "Medicina que restaura todos los PS y cura todos los problemas de estado de un Pok\202mon.",
  "Medicina que reanima a un Pok\202mon debilitado y restaura la mitad de sus PS.",
  "Medicina que reanima a un Pok\202mon debilitado y restaura todos sus PS.",
  "Medicina en spray que contrarresta los efectos del veneno en un Pok\202mon.",
  "Medicina en spray que cura a un Pok\202mon paralizado.",
  "Medicina en spray que libera a un Pok\202mon de las garras del sue\244o.",
  "Medicina en spray que cura las quemaduras a un Pok\202mon.",
  "Medicina en spray que descongela a un Pok\202mon.",
  "Medicina en spray que cura todos los problemas de estado de un Pok\202mon.",
  "Restaura un m\240ximo de 10 PP del ataque que elijas de un Pok\202mon.",
  "Restaura completamente los PP del movimiento que elijas de un Pok\202mon.",
  "Restaura 10 PP de todos los movimientos aprendidos por un Pok\202mon.",
  "Restaura completamente los PP de todos los movimientos aprendidos por el Pok\202mon elegido.",
  "Aumenta 1 nivel el Ataque en combate. Al cambiar de Pok\202mon, el efecto desaparece.",
  "Aumenta 1 nivel la Defensa en combate. Al cambiar de Pok\202mon, el efecto desaparece.",
  "Aumenta 1 nivel la Velocidad en combate. Al cambiar de Pok\202mon, el efecto desaparece.",
  "Aumenta 1 nivel el At. Especial en combate. Al cambiar de Pok\202mon, el efecto desaparece.",
  "Aumenta 1 nivel el \241ndice de golpe cr\241tico en combate. Al cambiar de Pok\202mon, el efecto desaparece.",
  "Nutritiva bebida que aumenta los PS de un Pok\202mon.",
  "Nutritiva bebida que aumenta el Ataque de un Pok\202mon.",
  "Nutritiva bebida que aumenta la Defensa de un Pok\202mon.",
  "Nutritiva bebida que sube el At. Esp. de un Pok\202mon.",
  "Nutritiva bebida que aumenta la Defensa Especial de un Pok\202mon.",
  "Nutritiva bebida que sube la Velocidad de un Pok\202mon.",
  "Dispositivo con dise\244o capsular que atrapa Pok\202mon salvajes. Se lanza como una bola contra el blanco.",
  "Pok\202 Ball de alto rendimiento. Tiene un \241ndice de \202xito superior al de la Pok\202 Ball.",
  "Pok\202 Ball de rendimiento superior. Tiene m\240s \241ndice de \202xito que la Superball.",
  "La Pok\202 Ball definitiva. Atrapa a cualquier Pok\202mon salvaje y no falla nunca.",
  "Curiosa piedra que permite evolucionar a determinadas especies de Pok\202mon. Amarilla como el fuego.",
  "Curiosa piedra que permite evolucionar a determinadas especies de Pok\202mon. Es azul, como el agua.",
  "Curiosa piedra que permite evolucionar a determinadas especies de Pok\202mon. Tiene dibujado un rayo.",
  "Curiosa piedra que permite evolucionar a determinadas especies de Pok\202mon. Tiene dibujada una hoja.",
  "Curiosa piedra que permite evolucionar a determinadas especies de Pok\202mon. Negra como el azabache.",
  "Curiosa piedra que permite evolucionar a determinadas especies de Pok\202mon. Es roja como el sol.",
  "Extra\244o cord\242n que recuerda a un cable. Permite evolucionar a ciertos Pok\202mon.",
  "Puede amedrentar al enemigo cuando el Pok\202mon que la lleva le hiera.",
  "Una gruesa y resistente escama que pueden llevar los Pok\202mon de tipo Drag\242n cuando son capturados.",
  "Peculiar dispositivo fabricado en Silph S.A. que contiene todo tipo de datos. Permite evolucionar a Porygon.",
  "Pel\241cula met\240lica que fortalece los ataques de tipo Acero. Debe llevarlo un Pok\202mon.",
  "Preciosa escama que permite evolucionar a Feebas. Brilla con los colores del arco\241ris.",
  "Tiene el brillo afilado de la plata y debe llevarlo Clamperl. Sube el At. Esp.",
  "Tiene un d\202bil brillo rosado y debe llevarla Clamperl. Sube la Def. Esp.",
  "Una piedra peculiar que permite evolucionar a algunos Pok\202mon. Tiene un brillo espectacular.",
  "Una piedra peculiar que permite evolucionar a algunos Pok\202mon. Es oscura como la noche.",
  "Una piedra peculiar que permite evolucionar a algunos Pok\202mon. Brilla como un lucero.",
  "Curiosa piedra que permite evolucionar a determinadas especies de Pok\202mon. Tiene un patr\242n nevado.",
  "Dispositivo de protecci\242n muy pesado. Sirve para evolucionar a Rhydon.",
  "Este dispositivo contiene una gran cantidad de energ\241a el\202ctrica. Sirve para evolucionar a Electabuzz.",
  "Esta caja contiene una enorme cantidad de energ\241a magm\240tica. Sirve para evolucionar a Magmar.",
  "Dispositivo transparente que contiene datos misteriosos. Permite evolucionar a Porygon2.",
  "Tela imbuida de una energ\241a espiritual muy potente. Sirve para evolucionar a Dusclops.",
  "Una piedra peculiar que permite evolucionar a Happiny. Tiene forma de huevo.",
  "Saquito que desprende un aroma peculiar. Sirve para evolucionar a Spritzee.",
  "Un dulce a base de suave y esponjosa nata montada. Sirve para evolucionar a Swirlix.",
  "Mineral de color muy oscuro que permite evolucionar a Scyther.",
  "Curioso manuscrito que permite cambiar a Urshifu a su forma Estilo brusco.",
  "Curioso manuscrito que permite cambiar a Urshifu a su forma Estilo fluido.",
  "Un mapa que puede consultarse en cualquier momento y que te indica d\242nde te encuentras.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Venusaur en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Charizard en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Charizard en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Blastoise en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Beedrill en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Pidgeot en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Pikachu en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Alakazam en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Slowbro en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Gengar en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Kangaskhan en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Pinsir en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Gyarados en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Aerodactyl en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Mewtwo en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Mewtwo en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Ampharos en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Steelix en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Scizor en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Heracross en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Houndoom en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Tyranitar en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Sceptile en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Blaziken en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Swampert en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Gardevoir en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Sableye en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Mawile en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Aggron en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Medicham en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Manectric en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Sharpedo en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Camerupt en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Altaria en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Banette en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Absol en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Glalie en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Salamence en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Metagross en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Latias en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Latios en combate.",
  nullptr,
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Lopunny en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Garchomp en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Lucario en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Abomasnow en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Gallade en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Audino en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Diancie en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Arcanine en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Venusaur en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Blastoise en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Gengar en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Kingler en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Lapras en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Machamp en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Garbodor en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Corvinight en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Orbeetle en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Butterfree en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Drednaw en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Coalossal en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Flapple y Appletun en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Sandaconda en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Toxtricity en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Centiskorch en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Hatterene en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Grimmsnarl en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Alcremie en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Copperajah en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Duraludon en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Eevee en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Jumpluff en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Rillaboom en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Cinderace en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Inteleon en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Snorlax en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Clefable en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Starmie en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Victreebel en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Dragonite en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Meganium en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Feraligatr en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Skarmory en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Froslass en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Emboar en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Excadril en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Scolipede en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Scrafty en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Eelektross en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Chandelure en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Chesnaught en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Delphox en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Greninja en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Pyroar en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Malamar en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Barbaracle en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Dragalge en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Hawlucha en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Drampa en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Falinks en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Floette Flor Eterna en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Zygarde en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Staraptor en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Lucario en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Crabominable en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Magearna en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Absol en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Chimecho en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Garchomp en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Golurk en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Tatsugiri en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Meowstic en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Baxcalibur en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Scovillain en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Glimmora en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Golisopod en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Zeraora en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Heatran en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Darkrai en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Raichu en combate.",
  "Una de las misteriosas Megapiedras. Permite megaevolucionar a Raichu en combate.",
  nullptr,
  "Aro que sirve para activar el poder de la Mega Evoluci\242n.",
  "Una Pok\202 Ball que aumenta su eficacia si se usa en el turno 1 del combate.",
  "Una Pok\202 Ball algo distinta que va mejorando su rendimiento con cada turno de combate.",
  "Es algo distinta. Hace que sea m\240s f\240cil capturar a Pok\202mon salvajes en la oscuridad.",
  "Es algo distinta a las dem\240s. Funciona especialmente bien con los Pok\202mon de tipo Agua y Bicho.",
  "Es algo distinta a las dem\240s. Funciona especialmente bien con los Pok\202mon salvajes de menor nivel.",
  "Es algo distinta a las dem\240s. Funciona especialmente bien con los Pok\202mon iguales a los que ya has capturado.",
  "Una Pok\202 Ball dise\244ada para capturar Pok\202mon de nivel inferior al de tu Pok\202mon.",
  "Pok\202 Ball algo distinta, especialmente indicada para capturar Pok\202mon con Velocidad muy alta.",
  "Una Pok\202 Ball dise\244ada para capturar Pok\202mon muy pesados.",
  "Una Pok\202 Ball dise\244ada para capturar Pok\202mon que evolucionan con Piedra Lunar.",
  "Una Pok\202 Ball algo diferente que funciona mejor cuando el Pok\202mon est\240 dormido.",
  "Una Pok\202 Ball que hace m\240s amistosos a los Pok\202mon capturados.",
  "Acogedora Pok\202 Ball que hace m\240s amistosos a los Pok\202mon salvajes capturados.",
  "Es una Pok\202 Ball muy particular que se cre\242 para conmemorar alg\243n acontecimiento.",
  "Caramelo energ\202tico que sube a un Pok\202mon de nivel.",
  "Una pocion brillante. El Pokemon que la bebe se vuelve variocolor para siempre.",
};

// Lo mismo en PT, de tools/pt_items.py. Misma fila.
// nullptr = sin traducir: la pantalla cae al ingles.
static const char *const ITEM_DESC_PT[ITEM_COUNT] = {
  "",
  "Restauram aos poucos o HP do Pok\202mon que os leva durante o combate.",
  "Os Pok\202mon podem us\240-la ou lev\240-la para recuperar algum HP.",
  "Os Pok\202mon podem us\240-la ou lev\240-la para recuperar 10 HP.",
  "Os Pok\202mon podem lev\240-la ou us\240-la para se curar de problemas de estado.",
  "Se o Pok\202mon que a leva estiver com o HP cheio, aguenta com 1 HP um golpe que o derrubaria.",
  "Potencia os golpes, mas consome 30% do HP a cada ataque. Tem de ser levado por um Pok\202mon.",
  "Aumenta o Ataque em 50%, mas s\242 deixa usar um tipo de golpe. Tem de ser levada por um Pok\202mon.",
  "Aumentam o At. Esp. em 50%, mas s\242 deixam usar um tipo de golpe. T\210m de ser levados por um Pok\202mon.",
  "Garra leve e afiada que de vez em quando deixa o Pok\202mon que a leva atacar primeiro.",
  "A lente aumenta a chance de o Pok\202mon que a leva dar um golpe cr\241tico.",
  "O Pok\202mon que levar esta pedra m\240gica n\260o evolui.",
  "Serve de combust\241vel e potencia os ataques do tipo FIRE. Tem de ser levado por um Pok\202mon.",
  "Gema em forma de l\240grima que fortalece os ataques do tipo WATER em 20%. Tem de ser levada por um Pok\202mon.",
  "Tem de ser levada por um Pok\202mon. A vitalidade da semente fortalece os ataques do tipo GRASS.",
  "Tem de ser levado por um Pok\202mon. O magnetismo potente fortalece os ataques do tipo ELEC.",
  "Colher cheia de energia telecin\202tica que potencia os golpes do tipo PSY.",
  "Fragmento de gelo que repele o calor e fortalece os ataques do tipo ICE. Tem de ser levado por um Pok\202mon.",
  "Tem de ser levado por um Pok\202mon. Aumenta a determina\207\260o e fortalece os ataques do tipo FIGHT.",
  "Bico longo e afiado que aumenta a for\207a dos golpes do tipo FLY. Tem de ser levado por um Pok\202mon.",
  "Dardo venenoso que refor\207a os golpes do tipo POISON em 20%. Tem de ser levado por um Pok\202mon.",
  "Tem de ser levada por um Pok\202mon. A maciez da areia fortalece os ataques do tipo GROUND.",
  "Tem de ser levada por um Pok\202mon. A robustez da pedra fortalece os ataques do tipo ROCK.",
  "P\242 de brilho prateado que fortalece os ataques do tipo BUG. Tem de ser levado por um Pok\202mon.",
  "Objeto misterioso e sombrio que refor\207a os ataques do tipo GHOST. Tem de ser levado por um Pok\202mon.",
  "Presa muito dura que refor\207a os ataques do tipo DRAGON. Tem de ser levada por um Pok\202mon.",
  "T\210m de ser levados por um Pok\202mon. Fortalecem os ataques do tipo DARK.",
  "A delicadeza do len\207o fortalece os ataques do tipo NORMAL. Tem de ser levado por um Pok\202mon.",
  "Rem\202dio em spray que cura ferimentos e recupera 20 HP de um Pok\202mon.",
  "Rem\202dio em spray que cura ferimentos e recupera 60 HP de um Pok\202mon.",
  "Rem\202dio em spray que cura ferimentos e recupera 120 HP de um Pok\202mon.",
  "Rem\202dio em spray que cura ferimentos e recupera todo o HP de um Pok\202mon.",
  "Rem\202dio que recupera todo o HP e cura todos os problemas de estado de um Pok\202mon.",
  "Rem\202dio que reanima um Pok\202mon desmaiado e recupera metade do HP dele.",
  "Rem\202dio que reanima um Pok\202mon desmaiado e recupera todo o HP dele.",
  "Rem\202dio em spray que anula os efeitos do veneno num Pok\202mon.",
  "Rem\202dio em spray que cura um Pok\202mon paralisado.",
  "Rem\202dio em spray que livra um Pok\202mon das garras do sono.",
  "Rem\202dio em spray que cura as queimaduras de um Pok\202mon.",
  "Rem\202dio em spray que descongela um Pok\202mon.",
  "Rem\202dio em spray que cura todos os problemas de estado de um Pok\202mon.",
  "Recupera no m\240ximo 10 PP do golpe que voc\210 escolher de um Pok\202mon.",
  "Recupera por completo os PP do golpe que voc\210 escolher de um Pok\202mon.",
  "Recupera 10 PP de todos os golpes aprendidos por um Pok\202mon.",
  "Recupera por completo os PP de todos os golpes aprendidos pelo Pok\202mon escolhido.",
  "Aumenta 1 n\241vel o Ataque em combate. Ao trocar de Pok\202mon, o efeito some.",
  "Aumenta 1 n\241vel a Defesa em combate. Ao trocar de Pok\202mon, o efeito some.",
  "Aumenta 1 n\241vel a Velocidade em combate. Ao trocar de Pok\202mon, o efeito some.",
  "Aumenta 1 n\241vel o At. Especial em combate. Ao trocar de Pok\202mon, o efeito some.",
  "Aumenta 1 n\241vel a chance de golpe cr\241tico em combate. Ao trocar de Pok\202mon, o efeito some.",
  "Bebida nutritiva que aumenta o HP de um Pok\202mon.",
  "Bebida nutritiva que aumenta o Ataque de um Pok\202mon.",
  "Bebida nutritiva que aumenta a Defesa de um Pok\202mon.",
  "Bebida nutritiva que aumenta o At. Esp. de um Pok\202mon.",
  "Bebida nutritiva que aumenta a Defesa Especial de um Pok\202mon.",
  "Bebida nutritiva que aumenta a Velocidade de um Pok\202mon.",
  "Dispositivo em forma de c\240psula que captura Pok\202mon selvagens. Joga-se como uma bola no alvo.",
  "Pok\202 Ball de alto desempenho. Tem uma taxa de sucesso maior que a da Pok\202 Ball.",
  "Pok\202 Ball de desempenho superior. Tem mais taxa de sucesso que a Great Ball.",
  "A Pok\202 Ball definitiva. Captura qualquer Pok\202mon selvagem e nunca erra.",
  "Pedra curiosa que faz certas esp\202cies de Pok\202mon evolu\241rem. Amarela como o fogo.",
  "Pedra curiosa que faz certas esp\202cies de Pok\202mon evolu\241rem. \220 azul, como a \240gua.",
  "Pedra curiosa que faz certas esp\202cies de Pok\202mon evolu\241rem. Tem um raio desenhado.",
  "Pedra curiosa que faz certas esp\202cies de Pok\202mon evolu\241rem. Tem uma folha desenhada.",
  "Pedra curiosa que faz certas esp\202cies de Pok\202mon evolu\241rem. Preta como azeviche.",
  "Pedra curiosa que faz certas esp\202cies de Pok\202mon evolu\241rem. \220 vermelha como o sol.",
  "Cord\260o estranho que lembra um cabo. Faz certos Pok\202mon evolu\241rem.",
  "Pode deixar o inimigo hesitante quando o Pok\202mon que a leva o ferir.",
  "Uma escama grossa e resistente que os Pok\202mon do tipo DRAGON podem levar quando s\260o capturados.",
  "Dispositivo peculiar fabricado pela Silph S.A. que cont\202m todo tipo de dado. Faz Porygon evoluir.",
  "Pel\241cula met\240lica que fortalece os ataques do tipo STEEL. Tem de ser levada por um Pok\202mon.",
  "Escama linda que faz Feebas evoluir. Brilha com as cores do arco-\241ris.",
  "Tem o brilho afiado da prata e tem de ser levado por Clamperl. Sobe o At. Esp.",
  "Tem um brilho rosado fraco e tem de ser levada por Clamperl. Sobe a Def. Esp.",
  "Uma pedra peculiar que faz alguns Pok\202mon evolu\241rem. Tem um brilho espetacular.",
  "Uma pedra peculiar que faz alguns Pok\202mon evolu\241rem. \220 escura como a noite.",
  "Uma pedra peculiar que faz alguns Pok\202mon evolu\241rem. Brilha como uma estrela.",
  "Pedra curiosa que faz certas esp\202cies de Pok\202mon evolu\241rem. Tem um padr\260o de neve.",
  "Dispositivo de prote\207\260o bem pesado. Serve para fazer Rhydon evoluir.",
  "Este dispositivo cont\202m uma grande quantidade de energia el\202trica. Serve para fazer Electabuzz evoluir.",
  "Esta caixa cont\202m uma enorme quantidade de energia de magma. Serve para fazer Magmar evoluir.",
  "Dispositivo transparente que cont\202m dados misteriosos. Faz Porygon2 evoluir.",
  "Tecido imbu\241do de uma energia espiritual muito potente. Serve para fazer Dusclops evoluir.",
  "Uma pedra peculiar que faz Happiny evoluir. Tem forma de ovo.",
  "Saquinho que solta um aroma peculiar. Serve para fazer Spritzee evoluir.",
  "Um doce feito de chantili macio e fofo. Serve para fazer Swirlix evoluir.",
  "Mineral de cor bem escura que faz Scyther evoluir.",
  "Manuscrito curioso que muda Urshifu para a forma Estilo Bruto.",
  "Manuscrito curioso que muda Urshifu para a forma Estilo Fluido.",
  "Um mapa que d\240 para consultar a qualquer momento e que mostra onde voc\210 est\240.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Venusaur em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Charizard em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Charizard em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Blastoise em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Beedrill em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Pidgeot em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Pikachu em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Alakazam em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Slowbro em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Gengar em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Kangaskhan em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Pinsir em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Gyarados em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Aerodactyl em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Mewtwo em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Mewtwo em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Ampharos em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Steelix em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Scizor em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Heracross em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Houndoom em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Tyranitar em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Sceptile em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Blaziken em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Swampert em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Gardevoir em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Sableye em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Mawile em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Aggron em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Medicham em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Manectric em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Sharpedo em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Camerupt em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Altaria em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Banette em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Absol em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Glalie em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Salamence em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Metagross em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Latias em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Latios em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Rayquaza em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Lopunny em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Garchomp em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Lucario em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Abomasnow em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Gallade em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Audino em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Diancie em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Arcanine em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Venusaur em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Blastoise em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Gengar em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Kingler em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Lapras em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Machamp em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Garbodor em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Corvinight em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Orbeetle em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Butterfree em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Drednaw em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Coalossal em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Flapple e Appletun em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Sandaconda em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Toxtricity em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Centiskorch em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Hatterene em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Grimmsnarl em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Alcremie em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Copperajah em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Duraludon em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Eevee em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Jumpluff em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Rillaboom em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Cinderace em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Inteleon em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Snorlax em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Clefable em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Starmie em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Victreebel em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Dragonite em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Meganium em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Feraligatr em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Skarmory em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Froslass em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Emboar em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Excadril em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Scolipede em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Scrafty em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Eelektross em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Chandelure em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Chesnaught em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Delphox em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Greninja em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Pyroar em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Malamar em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Barbaracle em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Dragalge em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Hawlucha em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Drampa em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Falinks em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Floette Flor Eterna em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Zygarde em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Staraptor em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Lucario em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Crabominable em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Magearna em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Absol em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Chimecho em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Garchomp em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Golurk em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Tatsugiri em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Meowstic em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Baxcalibur em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Scovillain em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Glimmora em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Golisopod em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Zeraora em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Heatran em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Darkrai em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Raichu em combate.",
  "Uma das misteriosas Megapedras. Permite megaevoluir Raichu em combate.",
  "Enciclop\202dia de alta tecnologia que registra os Pok\202mon vistos e capturados.",
  "Anel que reage \205s Mega Stones e permite a Mega Evolu\207\260o em combate.",
  "Pok\202 Ball que funciona muito melhor se for usada logo no primeiro turno do combate.",
  "Pok\202 Ball que fica melhor a cada turno que o combate dura.",
  "Pok\202 Ball que facilita a captura de Pok\202mon selvagens \205 noite.",
  "Pok\202 Ball que funciona especialmente bem com Pok\202mon dos tipos WATER e BUG.",
  "Pok\202 Ball que funciona especialmente bem com Pok\202mon selvagens de n\241vel baixo.",
  "Pok\202 Ball que funciona especialmente bem com esp\202cies que voc\210 j\240 capturou.",
  "Pok\202 Ball que facilita capturar Pok\202mon de n\241vel menor que o do seu.",
  "Pok\202 Ball que facilita a captura de Pok\202mon muito r\240pidos.",
  "Pok\202 Ball que funciona melhor com Pok\202mon muito pesados.",
  "Pok\202 Ball feita para capturar Pok\202mon que evoluem com a Moon Stone.",
  "Pok\202 Ball que funciona melhor quando o Pok\202mon est\240 dormindo.",
  "Pok\202 Ball que faz o Pok\202mon capturado chegar mais amig\240vel.",
  "Pok\202 Ball confort\240vel que faz o Pok\202mon capturado ficar amigo mais r\240pido.",
  "Pok\202 Ball rara, feita para comemorar alguma ocasi\260o especial.",
  "Um doce cheio de energia. Sobe um nivel do Pokemon que o come.",
  "Uma pocao brilhante. O Pokemon que a bebe fica shiny para sempre.",
};

static inline const ItemEntry &itemAt(uint8_t i) {
  return ITEM_TBL[i < ITEM_COUNT ? i : 0];
}

static const char *const POCKET_NAME[BP_COUNT] = {
  "Held", "Battle", "Balls", "Stones", "Key",
};

// Las bolas de la mas floja a la mas fuerte (por su multiplicador).
// Se nombran por ESCALON, nunca por indice: asi meter una bola nueva
// no obliga a tocar el codigo que las reparte.
#define BALL_TIERS 4
static const uint8_t BALL_ID[BALL_TIERS] = { 56, 57, 58, 59 };  // Poke Ball, Great Ball, Ultra Ball, Master Ball
// Todas as bolas, na ordem das artes de arremesso (ART_BALL).
#define BALL_ALL_COUNT 18
static const uint8_t BALL_ALL[BALL_ALL_COUNT] = { 56, 57, 58, 59, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224, 225, 226, 227 };
