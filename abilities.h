// GENERADO por tools/gen_abilities.py -- no editar a mano
// Se emiten TODAS las que pueden salir en la dex, aunque solo una parte
// tenga mecanica: que un Rattata tenga Fuga y aqui no sirva de nada es
// mejor que fingir que no la tiene. Los nombres vienen de la traduccion
// inglesa del propio Anil; las mecanicas estan escritas a mano.
#pragma once
#include <stdint.h>
#include "dex.h"      // T_*
#include "btstate.h"  // ST_*

// El cielo se declara en battle.h, que llega despues. Se repite el enum
// aqui no: se usan los numeros por nombre desde alli.
enum : uint8_t { AB_WE_NONE = 0, AB_WE_SUN, AB_WE_RAIN, AB_WE_SAND,
                 AB_WE_HAIL };

enum : uint8_t {
  AB_NONE = 0,
  AB_SPEED_WEATHER,   // arg = clima: velocidad x2 mientras dure
  AB_EVA_WEATHER,     // arg = clima: +1 de evasion
  AB_HEAL_WEATHER,    // arg = clima: se cura 1/16 por turno
  AB_SET_WEATHER,     // arg = clima: lo pone al entrar
  AB_IMMUNE_TYPE,     // arg = T_*: ese tipo no le hace nada
  AB_ABSORB_HEAL,     // arg = T_*: en vez de dano, se cura
  AB_ABSORB_BOOST,    // arg = T_*: en vez de dano, +1 at. especial
  AB_ABSORB_SPEED,    // arg = T_*: en vez de dano, +1 velocidad
  AB_ABSORB_ATK,      // arg = T_*: en vez de dano, +1 ataque
  AB_STEAM_VEIL,      // arg = T_*: lo evapora (+1 evasion) y saca el sol
  AB_SOUNDPROOF,       // los golpes de SONIDO no le entran
  AB_BULLETPROOF,      // ni los de bomba, bola o canon
  AB_BITE_BOOST,       // +50% a los mordiscos (MV_BITE)
  AB_KICK_BOOST,       // +20% a las patadas (del Anil)
  AB_FORESTRY,         // +30% a TODO sobre hierba (del Anil)
  AB_CRIT_UP,          // +1 al escalon de critico
  AB_SKILL_LINK,       // los multigolpe siempre al maximo
  AB_BEAST_BOOST,      // al tumbar, +1 a su stat MAS ALTO
  AB_JUSTIFIED,        // un golpe siniestro le sube el ataque
  AB_QUICK_FEET,       // +50% de velocidad si esta mal, sin el corte
  AB_EARLY_BIRD,       // duerme la mitad de turnos
  AB_ILLUMINATE,       // al entrar, -1 a la punteria del rival
  AB_CONTRARY,         // los cambios de stat se le invierten
  AB_TOUCH_STATUS,     // arg = ST_*: 30% de ponerlo a quien TOCA
  AB_CURSED_BODY,      // 30% de anular el golpe que le acaba de dar
  AB_SET_TERRAIN,      // arg = TR_*: pone ese suelo al entrar
  AB_PARADOX,          // arg = clima: con el, +30% a su stat mas alto
  AB_PARADOX_TR,       // arg = TR_*: lo mismo, pero con suelo
  AB_HARVEST,          // al cerrar el turno, recupera la baya comida
  AB_NEURO_FORCE,      // +25% a los golpes que le van bien
  AB_MOODY,            // cada turno, +2 a un stat al azar y -1 a otro
  AB_HEAVY_METAL,      // pesa el doble para los golpes de peso
  AB_CORROSION,        // envenena hasta a los de acero y veneno
  AB_GLACIAL,          // con nieve, +50% al hielo y a la velocidad
  AB_STENCH,           // 10% de amedrentar con cualquier golpe
  AB_CLOUD_NINE,       // el cielo deja de contar para los dos
  AB_ANGER_POINT,      // un critico le pone el ataque al maximo
  AB_FOREWARN,         // al entrar, ve el golpe mas fuerte del rival
  AB_SLICE_BOOST,      // +50% a los cortantes (MV_SLICE)
  AB_PROTEAN,          // se vuelve del tipo del golpe que va a usar
  AB_SWEET_VEIL,       // no le duermen, ni el a si mismo
  AB_FLUFFY,           // la mitad del contacto, el doble del fuego
  AB_SOLAR_POWER,      // al sol pega mas y se quema un octavo
  AB_BERSERK,          // bajar de media vida le sube el especial
  AB_TYPE_SWAP,        // arg = T_*: sus normales pasan a ese tipo, +20%
  AB_NORMALIZE,        // TODOS sus golpes pasan a normal, +20%
  AB_HALLOWED,         // del Anil: campanada al entrar y sonido psiquico
  AB_AURA,             // arg = T_*: ese tipo pega 1/3 mas, para los DOS
  AB_TYPE_X15,         // arg = T_*: sus golpes de ese tipo, +50%
  AB_TYPE_X13,         // arg = T_*: ...+30%
  AB_PULSE_BOOST,      // +50% a los de pulso y aura (MV_PULSE)
  AB_RUIN,             // arg = SG_*: baja ese stat del OTRO un 25%
  AB_HALVE_CLASS,      // arg 1 = la mitad del fisico, 0 = del especial
  AB_HALVE_TYPE,       // arg = T_*: la mitad de ese tipo
  AB_ANTI_SE,          // un cuarto menos de lo que le va mal
  AB_WONDER_GUARD,     // solo le entra lo supereficaz
  AB_ENTRY_STAGE,      // arg = SG_*: +1 la PRIMERA vez que sale
  AB_DOWNLOAD,         // al entrar, +1 al ataque que le vaya mejor
  AB_IMPOSTER,         // al entrar se transforma en el de enfrente
  AB_TRUANT,           // un turno si y otro no
  AB_SLOW_START,       // cinco turnos a mitad de ataque y velocidad
  AB_DEFEATIST,        // por debajo de media vida, mitad de ataques
  AB_WONDER_SKIN,      // los golpes de estado le llegan al 50%
  AB_MYCELIUM,         // sus golpes de estado van ultimos y no los paran
  AB_MUMMY,            // al que le toca le pega SU habilidad
  AB_SPIRIT_SWAP,      // al que le toca le cambia la habilidad
  AB_BAD_DREAMS,       // al rival dormido le roe un octavo por turno
  AB_ACC_UP,           // arg = % de punteria extra para el
  AB_GOOD_AS_GOLD,     // los golpes de ESTADO no le entran
  AB_GUARD_DOG,        // Intimidacion le sube el ataque, y no le echan
  AB_TRAP_TYPE,        // arg = T_*: al rival de ese tipo no le deja irse
  AB_COMATOSE,         // duerme siempre: ningun estado le entra
  AB_TOXIC_CHAIN,      // 30% de entoxicar con cualquier golpe
  AB_POISON_PUPPET,    // al que envenena, tambien le confunde
  AB_WIMP_OUT,         // por debajo de media vida, se larga
  AB_UNSEEN_FIST,      // sus golpes de contacto pasan el Protect
  AB_ORICHALCUM,       // saca el sol al entrar Y crece con el
  AB_HADRON,           // idem con el campo electrico
  AB_FORECAST,         // su tipo sigue al cielo (agua, fuego, hielo)
  AB_TYPE_FROM_ITEM,   // su tipo es el de la tabla que lleve
  AB_SUN_BOOST,        // al sol, +50% de ataque y defensa especial
  AB_STANCE,           // pegando se afila; de apoyo, se escuda
  AB_HP_FORM,          // arg 1 = crece bajo media vida; 0 = sobre un cuarto
  AB_HERO,             // al retirarse se vuelve heroe, y vuelve crecido
  AB_TERA_SHIFT,       // al entrar, +1 a su stat mas alto
  AB_DISGUISE,         // el primer golpe le quita un octavo y nada mas
  AB_GULP,             // tras un golpe de agua, al que le toca le duele
  AB_HUNGER,           // alterna cada turno y le cambia el tipo al golpe
  AB_ILLUSION,         // entra con la cara de otro hasta que le peguen
  AB_DANCER,           // copia el baile que use el rival
  AB_RELAY_BOOST,      // arg 1 = deja al siguiente +30% especial, 2 = todo
  AB_RELAY_CURE,       // al retirarse, le cura el estado al siguiente
  AB_COMMANDER,        // al entrar, +2 de especial y de velocidad
  AB_CONTACT_BOOST,    // +1/3 a los golpes de contacto
  AB_MAGIC_BOUNCE,     // le devuelve al rival sus golpes de estado
  AB_TRACE,            // al entrar le copia la habilidad al rival
  AB_PARENTAL,         // pega DOS veces, la segunda a un cuarto
  AB_PERISH_BODY,      // quien le toca cae en tres turnos, y el tambien
  AB_WIND_RIDER,       // el viento no le toca y encima le crece
  AB_DELTA_STREAM,     // al volador se le quitan sus debilidades
  AB_QUEENLY,          // al rival no le salen los golpes de prioridad
  AB_PUNK_ROCK,        // el sonido pega +30% y le entra a la mitad
  AB_THERMAL,          // el fuego le sube el ataque y no se quema
  AB_DEEP_ROOT,        // lo que drena, drena mas (del Anil)
  AB_HALVE_FIRE_ICE,  // fuego y hielo le hacen la mitad
  AB_CONTACT_STATUS,  // arg = ST_*: 30% al que le pegue de cerca
  AB_CONTACT_CONFUSE, // 30% de confundir al que le pegue de cerca
  AB_CONTACT_HURT,    // arg = divisor: le devuelve parte al que toca
  AB_PINCH_TYPE,      // arg = T_*: +50% a ese tipo por debajo de 1/3
  AB_TECHNICIAN,      // arg = potencia: +50% a los golpes flojos
  AB_GUTS,            // +50% de ataque si esta mal de estado
  AB_HUGE_POWER,      // ataque x2
  AB_ADAPTABILITY,    // el STAB pasa de x1,5 a x2
  AB_NO_CRIT,         // no le entran criticos
  AB_STURDY,          // estando entero, aguanta un golpe mortal
  AB_MULTISCALE,      // estando entero, recibe la mitad
  AB_NO_SECONDARY,    // los efectos secundarios no le entran
  AB_MARVELSCALE,     // +50% de defensa si esta mal de estado
  AB_IMMUNE_STATUS,   // arg = ST_* que no le entra
  AB_NO_CONFUSE,
  AB_NO_FLINCH,
  AB_SHED_SKIN,       // arg = % de curarse el estado al cerrar el turno
  AB_SYNCHRONIZE,     // le pasa el estado al que se lo puso
  AB_POISON_HEAL,     // el veneno le cura en vez de hacerle dano
  AB_INTIMIDATE,      // al entrar, -1 al ataque del rival
  AB_PRESSURE,        // al rival se le va el doble de PP
  AB_KEEN_EYE,        // no le bajan la punteria
  AB_NO_LOWER_ATK,
  AB_NO_LOWER_DEF,
  AB_CLEAR_BODY,      // no le bajan NINGUN stat
  AB_SPEED_BOOST,     // +1 de velocidad al cerrar cada turno
  AB_COMPOUND_EYES,   // arg = % de punteria extra
  AB_NO_GUARD,        // ni el falla ni le fallan
  AB_SERENE_GRACE,    // los efectos secundarios salen el doble
  // --- segunda tanda (2026-10-04) ---------------------------
  // Hasta aqui habia 67 habilidades con mecanica de 304, y las
  // otras 237 salian en la ficha sin hacer nada. Estas son las
  // que mas especies llevaban encima.
  AB_SHEER_FORCE,     // +30% a los golpes con efecto, y sin efecto
  AB_PUNCH_BOOST,     // +20% a los golpes de puno (MV_PUNCH)
  AB_HUSTLE,          // +50% de ataque fisico, -20% de punteria
  AB_TINTED_LENS,     // x2 cuando el tipo le va mal
  AB_SNIPER,          // sus criticos pegan la mitad mas
  AB_RIVALRY,         // +25% al del mismo sexo, -25% al otro
  AB_NO_RECOIL,       // el retroceso no le hace nada
  AB_BLITZ,           // 1er turno fuera: +50% vel, +20% atq (del Anil)
  AB_WEAK_ARMOR,      // al recibir un fisico: -1 defensa, +2 velocidad
  AB_OVERCOAT,        // ni el clima ni los polvos le tocan
  AB_INFILTRATOR,     // se salta Reflejo, Pantalla Luz y Velo Sagrado
  AB_MOLD_BREAKER,    // la habilidad del rival no cuenta
  AB_CURE_WEATHER,    // arg = clima: con ese cielo se cura el estado
  AB_REGENERATOR,     // al salir del combate recupera un tercio
  AB_PRANKSTER,       // sus golpes de apoyo salen primero
  AB_UNNERVE,         // el rival no puede comerse su baya
  AB_RUN_AWAY,        // de un salvaje se escapa siempre
  AB_FRISK,           // al entrar, ve que lleva el rival
  AB_TELEPATHY,       // al relevar, entra sin comerse el golpe
  // --- tercera tanda ------------------------------------------
  AB_RECKLESS,        // +20% a los golpes con retroceso
  AB_RATTLED,         // +1 velocidad si le pegan bicho/siniestro/fantasma
  AB_MOXIE,           // arg = SG_*: +1 de ese stat al tumbar a alguien
  AB_SPITE_STAT,      // arg = SG_*: +2 si el RIVAL le baja un stat
  AB_DAMP,            // nadie se autodestruye delante de el
  AB_UNAWARE,         // se salta los stages del otro
  AB_SAND_FORCE,      // +30% a roca/tierra/acero con tormenta de arena
  AB_SCRAPPY,         // normal y lucha SI le entran al fantasma
  AB_MAGIC_GUARD,     // solo le hace dano lo que le pegan de frente
  AB_GOOEY,           // al que le toca, -1 de velocidad
  AB_STEADFAST,       // +1 de velocidad cada vez que le amilanan
  AB_ANALYTIC,        // +30% si pega el ULTIMO
  AB_UNBURDEN,        // sin objeto encima, el doble de velocidad
  AB_GLUTTONY,        // la baya le salta a la mitad, no al cuarto
  // --- cuarta tanda -------------------------------------------
  AB_PICKUP,          // FUERA del combate: de vez en cuando trae algo
  AB_PICKPOCKET,      // al que le toca sin manos libres, le roba
  AB_ANTICIPATION,    // al entrar, avisa si el otro trae algo peligroso
  AB_GUARD_WEATHER,   // arg = clima: con ese cielo no le entran estados
  AB_SUCTION_CUPS,    // no la sacan de la pista a empujones
  // --- quinta tanda (leva 70: las formas de combate) ----------
  AB_ZEN_MODE,        // Darmanitan: a media vida, Modo Daruma
  AB_ICE_FACE,        // Eiscue: el primer fisico lo para el hielo
};

struct AbilityEntry {
  const char *name;
  uint8_t fx;   // AB_*
  uint8_t arg;  // significa una cosa distinta en cada efecto
};

#define ABIL_COUNT 305
// El indice de habilidad es uint16_t EN TODAS PARTES (ABIL_OF,
// Pet::ability, StoredMon::ability, Combatant::ability). Era
// uint8_t y aguantaba de sobra con 198, pero el PBS del Anil tiene
// 293 habilidades entre las 1025 especies y el numero 256 llega en
// ZACIAN (#888, Galar). Eso NO habria roto el build: la habilidad
// 260 se guarda como la 4 y el bicho sale con otra, en silencio,
// que es el fallo que mas veces se ha repetido en este proyecto
// (evolvesTo, el indice de golpe...). Se arreglo de una vez en vez
// de esperar a tropezarse.
static_assert(ABIL_COUNT <= 65535, "el indice de habilidad es uint16_t");

static const AbilityEntry ABIL_TBL[] = {
  { "-", AB_NONE, 0 },
  { "Blitz",            AB_BLITZ, 0 },
  { "Adaptability",     AB_ADAPTABILITY, 0 },
  { "Aerilate",         AB_TYPE_SWAP, T_FLYING },
  { "Aftermath",        AB_NONE, 0 },
  { "Air Lock",         AB_CLOUD_NINE, 0 },
  { "Analytic",         AB_ANALYTIC, 0 },
  { "Anger Point",      AB_ANGER_POINT, 0 },
  { "Anger Shell",      AB_NONE, 0 },
  { "Anticipation",     AB_ANTICIPATION, 0 },
  { "Arena Trap",       AB_NONE, 0 },
  { "Armor Tail",       AB_NONE, 0 },
  { "Aroma Veil",       AB_NONE, 0 },
  { "Bad Dreams",       AB_BAD_DREAMS, 0 },
  { "Ball Fetch",       AB_NONE, 0 },
  { "Battery",          AB_RELAY_BOOST, 1 },
  { "Battle Armor",     AB_NO_CRIT, 0 },
  { "Beads of Ruin",    AB_RUIN, SG_SPD },
  { "Beast Boost",      AB_BEAST_BOOST, 0 },
  { "Berserk",          AB_BERSERK, 0 },
  { "Big Pecks",        AB_NO_LOWER_DEF, 0 },
  { "Blaze",            AB_PINCH_TYPE, T_FIRE },
  { "Bulletproof",      AB_BULLETPROOF, 0 },
  { "Burning Touch",    AB_TOUCH_STATUS, ST_BURN },
  { "Striker",          AB_KICK_BOOST, 0 },
  { "Cheek Pouch",      AB_NONE, 0 },
  { "Chilling Neigh",   AB_MOXIE, SG_ATK },
  { "Chlorophyll",      AB_SPEED_WEATHER, AB_WE_SUN },
  { "Clear Body",       AB_CLEAR_BODY, 0 },
  { "Cloud Nine",       AB_CLOUD_NINE, 0 },
  { "Color Change",     AB_NONE, 0 },
  { "Comatose",         AB_COMATOSE, 0 },
  { "Commander",        AB_COMMANDER, 0 },
  { "Competitive",      AB_SPITE_STAT, SG_SPA },
  { "Compound Eyes",    AB_COMPOUND_EYES, 30 },
  { "Contrary",         AB_CONTRARY, 0 },
  { "Corrosion",        AB_CORROSION, 0 },
  { "Costar",           AB_NONE, 0 },
  { "Cotton Down",      AB_NONE, 0 },
  { "Cud Chew",         AB_NONE, 0 },
  { "Curious Medicine", AB_NONE, 0 },
  { "Cursed Body",      AB_CURSED_BODY, 0 },
  { "Cute Charm",       AB_CONTACT_CONFUSE, 0 },
  { "Damp",             AB_DAMP, 0 },
  { "Dancer",           AB_DANCER, 0 },
  { "Dark Aura",        AB_AURA, T_DARK },
  { "Dauntless Shield", AB_ENTRY_STAGE, SG_DEF },
  { "Dazzling",         AB_NONE, 0 },
  { "Defeatist",        AB_DEFEATIST, 0 },
  { "Defiant",          AB_SPITE_STAT, SG_ATK },
  { "Delta Stream",     AB_DELTA_STREAM, 0 },
  { "Disguise",         AB_DISGUISE, 0 },
  { "Download",         AB_DOWNLOAD, 0 },
  { "Dragon's Maw",     AB_TYPE_X15, T_DRAGON },
  { "Drizzle",          AB_SET_WEATHER, AB_WE_RAIN },
  { "Drought",          AB_SET_WEATHER, AB_WE_SUN },
  { "Dry Skin",         AB_ABSORB_HEAL, T_WATER },
  { "Early Bird",       AB_EARLY_BIRD, 0 },
  { "Earth Eater",      AB_NONE, 0 },
  { "Effect Spore",     AB_CONTACT_STATUS, ST_SLEEP },
  { "Electric Surge",   AB_SET_TERRAIN, TR_ELEC },
  { "Electromorphosis", AB_NONE, 0 },
  { "Emergency Exit",   AB_WIMP_OUT, 0 },
  { "Daunt",            AB_NONE, 0 },
  { "Fairy Aura",       AB_AURA, T_FAIRY },
  { "Filter",           AB_ANTI_SE, 0 },
  { "Flame Body",       AB_CONTACT_STATUS, ST_BURN },
  { "Flare Boost",      AB_NONE, 0 },
  { "Flash Fire",       AB_ABSORB_BOOST, T_FIRE },
  { "Bloom",            AB_NONE, 0 },
  { "Flower Gift",      AB_SUN_BOOST, 0 },
  { "Flower Veil",      AB_NONE, 0 },
  { "Fluffy",           AB_FLUFFY, 0 },
  { "Forecast",         AB_FORECAST, 0 },
  { "Forewarn",         AB_FOREWARN, 0 },
  { "Friend Guard",     AB_NONE, 0 },
  { "Frisk",            AB_FRISK, 0 },
  { "Full Metal Body",  AB_CLEAR_BODY, 0 },
  { "Fur Coat",         AB_HALVE_CLASS, 1 },
  { "Gale Wings",       AB_NONE, 0 },
  { "Gluttony",         AB_GLUTTONY, 0 },
  { "Good as Gold",     AB_GOOD_AS_GOLD, 0 },
  { "Gooey",            AB_GOOEY, 0 },
  { "Gorilla Tactics",  AB_NONE, 0 },
  { "Grassy Surge",     AB_SET_TERRAIN, TR_GRASS },
  { "Grim Neigh",       AB_MOXIE, SG_SPA },
  { "Guard Dog",        AB_GUARD_DOG, 0 },
  { "Gulp Missile",     AB_GULP, 0 },
  { "Guts",             AB_GUTS, 0 },
  { "Hadron Engine",    AB_HADRON, 0 },
  { "Harvest",          AB_HARVEST, 0 },
  { "Healer",           AB_RELAY_CURE, 0 },
  { "Heatproof",        AB_HALVE_TYPE, T_FIRE },
  { "Heavy Metal",      AB_HEAVY_METAL, 0 },
  { "Honey Gather",     AB_NONE, 0 },
  { "Hospitality",      AB_NONE, 0 },
  { "Huge Power",       AB_HUGE_POWER, 0 },
  { "Hunger Switch",    AB_HUNGER, 0 },
  { "Hustle",           AB_HUSTLE, 0 },
  { "Hydration",        AB_CURE_WEATHER, AB_WE_RAIN },
  { "Hyper Cutter",     AB_NO_LOWER_ATK, 0 },
  { "Ice Body",         AB_HEAL_WEATHER, AB_WE_HAIL },
  { "Ice Face",         AB_ICE_FACE, 0 },
  { "Ice Scales",       AB_HALVE_CLASS, 0 },
  { "Illuminate",       AB_ILLUMINATE, 0 },
  { "Illusion",         AB_ILLUSION, 0 },
  { "Immunity",         AB_IMMUNE_STATUS, ST_POISON },
  { "Imposter",         AB_IMPOSTER, 0 },
  { "Infiltrator",      AB_INFILTRATOR, 0 },
  { "Innards Out",      AB_NONE, 0 },
  { "Inner Focus",      AB_NO_FLINCH, 0 },
  { "Insomnia",         AB_IMMUNE_STATUS, ST_SLEEP },
  { "Intimidate",       AB_INTIMIDATE, 0 },
  { "Intrepid Sword",   AB_ENTRY_STAGE, SG_ATK },
  { "Iron Barbs",       AB_CONTACT_HURT, 8 },
  { "Iron Fist",        AB_PUNCH_BOOST, 0 },
  { "Justified",        AB_JUSTIFIED, 0 },
  { "Keen Eye",         AB_KEEN_EYE, 0 },
  { "Klutz",            AB_NONE, 0 },
  { "Leaf Guard",       AB_GUARD_WEATHER, AB_WE_SUN },
  { "Levitate",         AB_IMMUNE_TYPE, T_GROUND },
  { "Libero",           AB_PROTEAN, 0 },
  { "Light Metal",      AB_NONE, 0 },
  { "Lightning Rod",    AB_ABSORB_BOOST, T_ELEC },
  { "Limber",           AB_IMMUNE_STATUS, ST_PARALYSIS },
  { "Lingering Aroma",  AB_NONE, 0 },
  { "Liquid Ooze",      AB_NONE, 0 },
  { "Liquid Voice",     AB_NONE, 0 },
  { "Long Reach",       AB_NONE, 0 },
  { "Magic Bounce",     AB_MAGIC_BOUNCE, 0 },
  { "Magic Guard",      AB_MAGIC_GUARD, 0 },
  { "Magician",         AB_PICKPOCKET, 0 },
  { "Magnet Pull",      AB_TRAP_TYPE, T_STEEL },
  { "Marvel Scale",     AB_MARVELSCALE, 0 },
  { "Mega Launcher",    AB_PULSE_BOOST, 0 },
  { "Merciless",        AB_NONE, 0 },
  { "Mimicry",          AB_NONE, 0 },
  { "Minus",            AB_NONE, 0 },
  { "Mirror Armor",     AB_NONE, 0 },
  { "Misty Surge",      AB_SET_TERRAIN, TR_MISTY },
  { "Mold Breaker",     AB_MOLD_BREAKER, 0 },
  { "Moody",            AB_MOODY, 0 },
  { "Motor Drive",      AB_ABSORB_SPEED, T_ELEC },
  { "Moxie",            AB_MOXIE, SG_ATK },
  { "Multiscale",       AB_MULTISCALE, 0 },
  { "Multitype",        AB_TYPE_FROM_ITEM, 0 },
  { "Mummy",            AB_MUMMY, 0 },
  { "Mycelium Might",   AB_MYCELIUM, 0 },
  { "Natural Cure",     AB_SHED_SKIN, 100 },
  { "Neuro Force",      AB_NEURO_FORCE, 0 },
  { "Neutralizing Gas", AB_NONE, 0 },
  { "No Guard",         AB_NO_GUARD, 0 },
  { "Oblivious",        AB_NO_CONFUSE, 0 },
  { "Opportunist",      AB_NONE, 0 },
  { "Orichalcum Pulse", AB_ORICHALCUM, 0 },
  { "Overcoat",         AB_OVERCOAT, 0 },
  { "Overgrow",         AB_PINCH_TYPE, T_GRASS },
  { "Own Tempo",        AB_NO_CONFUSE, 0 },
  { "Parental Bond",    AB_PARENTAL, 0 },
  { "Pastel Veil",      AB_NONE, 0 },
  { "Perish Body",      AB_PERISH_BODY, 0 },
  { "Pickpocket",       AB_PICKPOCKET, 0 },
  { "Pickup",           AB_PICKUP, 0 },
  { "Foliate",          AB_NONE, 0 },
  { "Pixilate",         AB_TYPE_SWAP, T_FAIRY },
  { "Plus",             AB_NONE, 0 },
  { "Glacial Might",    AB_GLACIAL, 0 },
  { "Sage Power",       AB_NONE, 0 },
  { "Poison Heal",      AB_POISON_HEAL, 0 },
  { "Poison Point",     AB_CONTACT_STATUS, ST_POISON },
  { "Poison Puppeteer", AB_POISON_PUPPET, 0 },
  { "Poison Touch",     AB_TOUCH_STATUS, ST_POISON },
  { "Power Construct",  AB_HP_FORM, 1 },
  { "Power Spot",       AB_RELAY_BOOST, 2 },
  { "Prankster",        AB_PRANKSTER, 0 },
  { "Pressure",         AB_PRESSURE, 0 },
  { "Prism Armor",      AB_ANTI_SE, 0 },
  { "Propeller Tail",   AB_NONE, 0 },
  { "Protean",          AB_PROTEAN, 0 },
  { "Protosynthesis",   AB_PARADOX, AB_WE_SUN },
  { "Psychic Surge",    AB_SET_TERRAIN, TR_PSY },
  { "Punk Rock",        AB_PUNK_ROCK, 0 },
  { "Pure Power",       AB_HUGE_POWER, 0 },
  { "Purifying Salt",   AB_NONE, 0 },
  { "Quark Drive",      AB_PARADOX_TR, TR_ELEC },
  { "Queenly Majesty",  AB_QUEENLY, 0 },
  { "Quick Draw",       AB_NONE, 0 },
  { "Quick Feet",       AB_QUICK_FEET, 0 },
  { "Rain Dish",        AB_HEAL_WEATHER, AB_WE_RAIN },
  { "Rattled",          AB_RATTLED, 0 },
  { "Receiver",         AB_NONE, 0 },
  { "Reckless",         AB_RECKLESS, 0 },
  { "Refrigerate",      AB_TYPE_SWAP, T_ICE },
  { "Regenerator",      AB_REGENERATOR, 0 },
  { "Ripen",            AB_NONE, 0 },
  { "Rivalry",          AB_RIVALRY, 0 },
  { "RKS System",       AB_TYPE_FROM_ITEM, 0 },
  { "Rock Head",        AB_NO_RECOIL, 0 },
  { "Rocky Payload",    AB_NONE, 0 },
  { "Rough Skin",       AB_CONTACT_HURT, 8 },
  { "Run Away",         AB_RUN_AWAY, 0 },
  { "Sand Force",       AB_SAND_FORCE, 0 },
  { "Sand Rush",        AB_SPEED_WEATHER, AB_WE_SAND },
  { "Sand Spit",        AB_NONE, 0 },
  { "Sand Stream",      AB_SET_WEATHER, AB_WE_SAND },
  { "Sand Veil",        AB_EVA_WEATHER, AB_WE_SAND },
  { "Sap Sipper",       AB_ABSORB_ATK, T_GRASS },
  { "Schooling",        AB_HP_FORM, 0 },
  { "Scrappy",          AB_SCRAPPY, 0 },
  { "Screen Cleaner",   AB_NONE, 0 },
  { "Seed Sower",       AB_NONE, 0 },
  { "Serene Grace",     AB_SERENE_GRACE, 0 },
  { "Shadow Shield",    AB_MULTISCALE, 0 },
  { "Shadow Tag",       AB_TRAP_TYPE, 0 },
  { "Sharpness",        AB_SLICE_BOOST, 0 },
  { "Shed Skin",        AB_SHED_SKIN, 30 },
  { "Sheer Force",      AB_SHEER_FORCE, 0 },
  { "Shell Armor",      AB_NO_CRIT, 0 },
  { "Shield Dust",      AB_NO_SECONDARY, 0 },
  { "Shields Down",     AB_HP_FORM, 1 },
  { "Forestry",         AB_FORESTRY, 0 },
  { "Simple",           AB_NONE, 0 },
  { "Skill Link",       AB_SKILL_LINK, 0 },
  { "Slow Start",       AB_SLOW_START, 0 },
  { "Slush Rush",       AB_SPEED_WEATHER, AB_WE_HAIL },
  { "Sniper",           AB_SNIPER, 0 },
  { "Snow Cloak",       AB_EVA_WEATHER, AB_WE_HAIL },
  { "Snow Warning",     AB_SET_WEATHER, AB_WE_HAIL },
  { "Overcharge",       AB_PINCH_TYPE, T_ELEC },
  { "Solar Power",      AB_SOLAR_POWER, 0 },
  { "Solid Rock",       AB_ANTI_SE, 0 },
  { "Soul-Heart",       AB_MOXIE, SG_SPA },
  { "Soundproof",       AB_SOUNDPROOF, 0 },
  { "Speed Boost",      AB_SPEED_BOOST, 0 },
  { "Stakeout",         AB_NONE, 0 },
  { "Stall",            AB_NONE, 0 },
  { "Stalwart",         AB_NONE, 0 },
  { "Stamina",          AB_NONE, 0 },
  { "Stance Change",    AB_STANCE, 0 },
  { "Static",           AB_CONTACT_STATUS, ST_PARALYSIS },
  { "Steadfast",        AB_STEADFAST, 0 },
  { "Steam Engine",     AB_NONE, 0 },
  { "Steam Veil",       AB_STEAM_VEIL, T_WATER },
  { "Steelworker",      AB_TYPE_X15, T_STEEL },
  { "Steely Spirit",    AB_NONE, 0 },
  { "Stench",           AB_STENCH, 0 },
  { "Sticky Hold",      AB_NONE, 0 },
  { "Storm Drain",      AB_ABSORB_BOOST, T_WATER },
  { "Strong Jaw",       AB_BITE_BOOST, 0 },
  { "Sturdy",           AB_STURDY, 0 },
  { "Deep Root",        AB_DEEP_ROOT, 0 },
  { "Suction Cups",     AB_SUCTION_CUPS, 0 },
  { "Super Luck",       AB_CRIT_UP, 0 },
  { "Supersweet Syrup", AB_NONE, 0 },
  { "Supreme Overlord", AB_NONE, 0 },
  { "Swarm",            AB_PINCH_TYPE, T_BUG },
  { "Sweet Veil",       AB_SWEET_VEIL, 0 },
  { "Swift Swim",       AB_SPEED_WEATHER, AB_WE_RAIN },
  { "Sword of Ruin",    AB_RUIN, SG_DEF },
  { "Symbiosis",        AB_NONE, 0 },
  { "Synchronize",      AB_SYNCHRONIZE, 0 },
  { "Tablets of Ruin",  AB_RUIN, SG_ATK },
  { "Tangled Feet",     AB_NONE, 0 },
  { "Tangling Hair",    AB_GOOEY, 0 },
  { "Technician",       AB_TECHNICIAN, 60 },
  { "Telepathy",        AB_TELEPATHY, 0 },
  { "Tera Shift",       AB_TERA_SHIFT, 0 },
  { "Teravolt",         AB_MOLD_BREAKER, 0 },
  { "Thermal Exchange", AB_THERMAL, 0 },
  { "Thick Fat",        AB_HALVE_FIRE_ICE, 0 },
  { "Tinted Lens",      AB_TINTED_LENS, 0 },
  { "Hallowed Chime",   AB_HALLOWED, 0 },
  { "Torrent",          AB_PINCH_TYPE, T_WATER },
  { "Tough Claws",      AB_CONTACT_BOOST, 0 },
  { "Toxic Boost",      AB_NONE, 0 },
  { "Toxic Chain",      AB_TOXIC_CHAIN, 0 },
  { "Toxic Debris",     AB_NONE, 0 },
  { "Trace",            AB_TRACE, 0 },
  { "Transistor",       AB_TYPE_X13, T_ELEC },
  { "Triage",           AB_NONE, 0 },
  { "Truant",           AB_TRUANT, 0 },
  { "Turboblaze",       AB_MOLD_BREAKER, 0 },
  { "Unaware",          AB_UNAWARE, 0 },
  { "Unburden",         AB_UNBURDEN, 0 },
  { "Unnerve",          AB_UNNERVE, 0 },
  { "Unseen Fist",      AB_UNSEEN_FIST, 0 },
  { "Vessel of Ruin",   AB_RUIN, SG_SPA },
  { "Victory Star",     AB_ACC_UP, 10 },
  { "Vital Spirit",     AB_IMMUNE_STATUS, ST_SLEEP },
  { "Volt Absorb",      AB_ABSORB_HEAL, T_ELEC },
  { "Wandering Spirit", AB_SPIRIT_SWAP, 0 },
  { "Water Absorb",     AB_ABSORB_HEAL, T_WATER },
  { "Water Bubble",     AB_NONE, 0 },
  { "Water Compaction", AB_NONE, 0 },
  { "Water Veil",       AB_IMMUNE_STATUS, ST_BURN },
  { "Weak Armor",       AB_WEAK_ARMOR, 0 },
  { "Well-Baked Body",  AB_NONE, 0 },
  { "White Smoke",      AB_CLEAR_BODY, 0 },
  { "Wimp Out",         AB_WIMP_OUT, 0 },
  { "Wind Power",       AB_NONE, 0 },
  { "Wind Rider",       AB_WIND_RIDER, 0 },
  { "Wonder Guard",     AB_WONDER_GUARD, 0 },
  { "Wonder Skin",      AB_WONDER_SKIN, 0 },
  { "Zen Mode",         AB_ZEN_MODE, 0 },
  { "Zero to Hero",     AB_HERO, 0 },
};

// Que hace cada habilidad, indexado igual que ABIL_TBL.
static const char *const ABIL_DESC[ABIL_COUNT] = {
  "",
  "Boosts Speed by 50% and Attack by 20% on the first turn.",
  "Increases Same Type Attack Bonus (STAB) from 1.5x to 2x.",
  "Converts Normal-type moves to Flying-type and boosts power by 20%.",
  "Damages the attacker by 25% of their max HP if knocked out by a contact move.",
  "Negates all weather effects.",
  "Boosts power by 30% if moving last.",
  "Maximizes Attack after taking a critical hit.",
  "Drops Def/Sp. Def but raises Atk/Sp. Atk/Speed when HP drops below half.",
  "Shudders if the foe has a super-effective move.",
  "Prevents foes from fleeing unless they are Ghost-type, Flying-type, or levitating.",
  "Blocks priority moves against the team.",
  "Protects the team from moves that restrict choices, like Taunt or Cursed Body.",
  "Damages sleeping foes by 1/8 max HP at the end of each turn.",
  "Retrieves the first failed Poke Ball if holding no item.",
  "Boosts allies' special moves by 30%.",
  "The Pokemon is protected against critical hits.",
  "Lowers Sp. Def of all other Pokemon.",
  "Boosts highest stat after defeating a foe.",
  "Raises Sp. Atk when HP drops below half. Can activate multiple times.",
  "Prevents Defense from being lowered by others.",
  "Boosts Fire-type moves by 50% when HP is 1/3 or lower.",
  "Immune to all bullet, ball, and bomb-based moves.",
  "Has a 30% chance of burning the target on contact.",
  "Boosts kicking moves by 20%.",
  "Restores 1/3 max HP when eating a Berry.",
  "Raises Attack after defeating a foe.",
  "Doubles Speed in harsh sunlight.",
  "Prevents other Pokemon from lowering its stats.",
  "Negates all weather effects while on the field.",
  "Changes type to the type of the last move it was hit by.",
  "Never wakes up, but can attack while asleep.",
  "Enters an ally Dondozo's mouth to command it.",
  "Sharply raises Sp. Atk when a stat is lowered by the foe.",
  "Boosts Accuracy by 30% and increases the chance of finding wild Pokemon with items.",
  "Reverses stat changes. Stats rise when they should fall, and vice versa.",
  "Can poison Steel- and Poison-types.",
  "Copies ally's stat changes upon entry.",
  "Lowers Speed of the other Pokemon when hit by an attack.",
  "Regurgitates and re-eats a Berry at the end of the next turn.",
  "Resets allies' stat changes upon entering battle.",
  "30% chance to disable the move used against it for 4 turns.",
  "Contact has a 30% chance to cause infatuation if the foe is the opposite gender.",
  "Prevents the use of self-destructing moves and the Aftermath ability.",
  "Copies any dance move used by other Pokemon.",
  "Boosts the power of all Dark-type moves by 33%.",
  "Raises Defense by one stage upon entering battle.",
  "Prevents foes from using priority moves against the team.",
  "Halves Attack and Sp. Atk when HP drops below 50%.",
  "Attack raises sharply when a stat is lowered by the foe.",
  "Summons strong winds that remove Flying-type weaknesses. Cannot be overridden by standard weather.",
  "Reduces damage from the first hit taken to 1/8 max HP.",
  "Adjusts offensive stats based on the foe's defenses.",
  "Boosts Dragon-type moves by 50%.",
  "Summons rain for 5 turns upon entering battle. Lasts 8 turns if holding a Damp Rock.",
  "Summons harsh sunlight for 5 turns upon entering battle. Lasts 8 turns if holding a Heat Rock.",
  "Loses HP in sun, heals in rain. Takes more Fire damage, heals from Water moves.",
  "Sleeps for half the normal duration.",
  "Restores HP if hit by a Ground-type move.",
  "Contact may poison, paralyze, or put the attacker to sleep (30% chance).",
  "Summons Electric Terrain upon entering battle.",
  "Taking damage charges the next Electric-type move.",
  "Switches out after using a Bug-type move.",
  "Frightens the opponent and lowers its Sp. Atk.",
  "Boosts the power of all Fairy-type moves by 33%.",
  "Reduces damage from super-effective attacks by 1/4.",
  "Contact has a 30% chance to burn the attacker.",
  "Boosts Special attacks by 50% when burned.",
  "Boosts Fire-type moves by 50% if hit by a Fire-type move. Grants immunity to Fire.",
  "Boosts the power of Grass-type moves.",
  "Boosts Attack and Sp. Def of the team in sunny weather.",
  "Protects allied Grass-types from status conditions and stat drops.",
  "Halves damage from contact moves, but doubles Fire damage taken.",
  "Changes type based on the weather.",
  "Reveals the foe's strongest move upon entering battle.",
  "Reduces damage taken by allies by 25%.",
  "Reveals the foe's held item upon entering battle.",
  "Prevents stats from being lowered by others.",
  "Halves damage from physical moves.",
  "Gives priority to Flying-type moves when at full HP.",
  "Eats held Berries at 50% HP instead of 25%.",
  "Immune to status moves.",
  "Lowers the attacker's Speed on contact.",
  "Boosts Attack by 50%, but locks the first move used.",
  "Summons Grassy Terrain upon entering battle.",
  "Raises Sp. Atk after defeating a foe.",
  "Blocks Intimidate and boosts Attack instead. Prevents forced switching.",
  "Changes form after using Surf or Dive based on HP.",
  "Boosts Attack by 50% if suffering from a status condition, ignoring the Burn penalty.",
  "Summons Electric Terrain. Boosts Sp. Atk on Electric Terrain.",
  "50% chance to restore a used Berry at end of turn. 100% in sunlight.",
  "30% chance to heal an ally's status condition at the end of each turn.",
  "Halves damage from Fire-type moves. Reduces burn damage.",
  "Doubles the Pokemon's weight.",
  "May gather Honey after battle.",
  "Heals an ally's HP upon entering battle.",
  "Doubles the Pokemon's Attack stat.",
  "Changes form and Aura Wheel's type at the end of each turn.",
  "Boosts Attack or Sp. Atk by 50%, but lowers accuracy of offensive moves by 20%.",
  "Heals status conditions at the end of turn if it is raining.",
  "Prevents Attack from being lowered by others.",
  "Gradually restores HP in a hailstorm.",
  "Changes to Noice Face if hit by a physical move. Restored by hail.",
  "Halves damage from Special attacks.",
  "Lowers the foe's Accuracy upon entering battle. Increases wild encounter rate.",
  "Disguises itself as the last Pokemon in the party.",
  "Cannot be poisoned.",
  "Transforms into the foe, copying stats (except HP).",
  "Attacks ignore screens and Substitutes.",
  "Inflicts damage equal to remaining HP before fainting.",
  "Prevents flinching and Intimidate.",
  "Prevents the Pokemon from falling asleep or using Rest. Wakes up if gained while sleeping.",
  "Lowers the opposing Pokemon's Attack upon entering battle.",
  "Raises Attack by one stage upon entering battle.",
  "Inflicts 1/8 max HP damage to the attacker on contact.",
  "Boosts the power of punching moves by 20%.",
  "Raises Attack when hit by a Dark-type move.",
  "Prevents Accuracy from being lowered. Ignores foe's Evasion boosts.",
  "Prevents the use of held items and negates their effects.",
  "Prevents status conditions in sunny weather.",
  "Immune to all Ground-type moves.",
  "Changes type to match the move about to be used.",
  "Halves the Pokemon's weight.",
  "Draws in Electric-type moves to boost Sp. Atk. Works even on Ground-types.",
  "Cannot be paralyzed.",
  "Contact changes the attacker's Ability to Lingering Aroma.",
  "Damages foes using HP-draining moves.",
  "Sound-based moves become Water-type and get a 20% boost.",
  "Moves make no contact with the target.",
  "Reflects status moves back at the user.",
  "Takes only direct damage. Immune to indirect damage and trapping.",
  "Steals the target's item when hitting with a contact move.",
  "Traps Steel-type foes, preventing escape unless they hold a Shed Shell.",
  "Boosts Defense by 50% if suffering from a status condition.",
  "Boosts pulse and aura moves by 50%.",
  "Attacks always result in critical hits if the target is poisoned.",
  "Changes type to match the current Terrain.",
  "Boosts Sp. Atk by 50% if an ally has the Plus or Minus Ability.",
  "Reflects stat-lowering effects back at the user.",
  "Summons Misty Terrain upon entering battle.",
  "Moves ignore the target's Ability.",
  "Sharply raises a random stat and lowers another at the end of each turn.",
  "Raises Speed if hit by an Electric-type move. Immune to Electric damage.",
  "Raises Attack after knocking out a foe.",
  "Halves damage taken when at full HP.",
  "Changes type to match the held Plate or Z-Crystal.",
  "Changes the attacker's Ability to Mummy on contact.",
  "Status moves move last but ignore Abilities.",
  "Cures all status conditions upon switching out or ending battle.",
  "Boosts super-effective moves by 25%.",
  "Neutralizes all Abilities while on the field.",
  "All moves used by or against this Pokemon will never miss.",
  "Prevents infatuation, Taunt, and Intimidate.",
  "Copies the foe's stat increases.",
  "Summons harsh sunlight. Boosts Attack in sunlight.",
  "Protects from sandstorm, hail, and powder/spore moves.",
  "Boosts Grass-type moves by 50% when HP is 1/3 or lower.",
  "Prevents confusion and Intimidate.",
  "Attacks twice per turn. The second hit deals 25% damage.",
  "Protects the team from poison. Cures allies upon entry.",
  "Both Pokemon faint in 3 turns if hit by a contact move.",
  "Steals the attacker's item on contact if not already holding one.",
  "10% chance to pick up an item after battle. May pick up items lost in turn.",
  "Converts Normal-type moves to Grass-type and boosts power.",
  "Converts Normal-type moves to Fairy-type and boosts power by 20%.",
  "Boosts Sp. Atk by 50% if an ally has the Plus or Minus Ability.",
  "Boosts Ice-type power and Speed by 50% in snowy weather.",
  "Boosts Sp. Atk by 50%, but locks the first move used.",
  "Restores 1/8 max HP per turn if poisoned.",
  "Contact has a 30% chance to poison the attacker.",
  "Poisoned foes also become confused.",
  "Contact has a 30% chance to poison the target.",
  "Transforms into Complete Forme when HP drops below 50%.",
  "Boosts allies' moves by 30%.",
  "Gives priority to status moves. Dark-types are immune.",
  "Forces the foe to use 2 PP per move.",
  "Reduces super-effective damage by 1/4.",
  "The user's moves ignore being redirected by the opponent's moves or abilities.",
  "Changes type to match the move about to be used.",
  "Boosts highest stat in harsh sunlight or if holding Booster Energy.",
  "Summons Psychic Terrain upon entering battle.",
  "Boosts sound moves by 30% and halves damage taken from them.",
  "Doubles the Attack stat.",
  "Immune to status conditions. Halves Ghost-type damage.",
  "Boosts highest stat on Electric Terrain or if holding Booster Energy.",
  "Blocks priority moves from hitting the Pokemon.",
  "30% chance to move first within the same priority bracket.",
  "Boosts Speed by 50% if suffering a status condition. Ignores Paralysis speed drop.",
  "Gradually restores HP in rain.",
  "Boosts Speed when hit by Ghost-, Dark-, or Bug-type moves, or Intimidate.",
  "Inherits an ally's Ability when they faint.",
  "Boosts recoil moves by 20%.",
  "Converts Normal-type moves to Ice-type and boosts power by 20%.",
  "Restores 1/3 max HP upon switching out.",
  "Doubles the effect of consumed Berries.",
  "Deals 25% more damage to foes of the same gender.",
  "Changes type based on the held Memory Disc.",
  "Prevents recoil damage, except from Struggle.",
  "Boosts Rock-type moves by 50%.",
  "Inflicts damage equal to 1/16 max HP to the attacker on contact.",
  "Guarantees escape from wild Pokemon.",
  "Boosts Ground-, Steel-, and Rock-type moves by 30% in a sandstorm.",
  "Doubles Speed in a sandstorm and grants immunity to sand damage.",
  "Summons a Sandstorm when hit. Lasts 8 turns if holding a Smooth Rock.",
  "Summons a sandstorm for 5 turns upon entering battle. Lasts 8 turns if holding a Smooth Rock.",
  "Boosts Evasion by 20% in a sandstorm and grants immunity to sandstorm damage.",
  "Absorbs Grass-type moves to raise Attack.",
  "Transforms into School Form if HP is above 25% (Level 20+). Reverts if below.",
  "Normal- and Fighting-type moves can hit Ghost-types. Immune to Intimidate.",
  "Removes Light Screen, Reflect, and Aurora Veil from the field.",
  "Summons Grassy Terrain when hit.",
  "Doubles the chance of moves' secondary effects occurring. Applies to King's Rock and Razor Fang.",
  "Halves damage taken when at full HP.",
  "Prevents the foe from escaping or switching out, unless they are Ghost-type or holding a Shed Shell.",
  "Boosts slicing moves by 50%.",
  "30% chance to heal status conditions at the end of each turn.",
  "Boosts moves with secondary effects by 30%, but removes the effects.",
  "The Pokemon is protected against critical hits.",
  "Blocks additional effects of attacks taken.",
  "Breaks its shell when HP drops below half, changing form.",
  "Increases damage of all moves by 30% in Grassy Terrain.",
  "Doubles all stat changes.",
  "Multi-hit moves always hit the maximum number of times.",
  "Halves Attack and Speed for the first 5 turns.",
  "Doubles Speed in hail and grants immunity to hail damage.",
  "Increases critical hit damage from 1.5x to 2.25x.",
  "Raises Evasion by 20% during a hailstorm.",
  "Summons a hailstorm for 5 turns. Lasts 8 turns if holding Icy Rock.",
  "Boosts Electric-type moves by 50% when HP is 1/3 or lower.",
  "Boosts Sp. Atk by 50% in sunlight, but loses 1/8 HP per turn.",
  "Reduces damage from super-effective attacks by 1/4.",
  "Raises Sp. Atk whenever a Pokemon faints.",
  "Immune to all sound-based moves.",
  "Raises the Pokemon's Speed by one stage at the end of each turn.",
  "Deals double damage to Pokemon that switched in this turn.",
  "The Pokemon moves last within its priority bracket.",
  "The user's moves ignore being redirected by the opponent's moves or abilities.",
  "Raises Defense when hit by a damaging move.",
  "Changes to Blade Form when attacking, and Shield Form when using King's Shield.",
  "Contact with the Pokemon has a 30% chance to cause paralysis. Also affects Ground-types.",
  "Speed is raised one stage each time it flinches.",
  "Maxes out Speed if hit by a Fire- or Water-type move.",
  "Summons harsh sunlight for 6 turns on entry. Water moves turn to steam, dealing no damage and raising evasiveness.",
  "Boosts Steel-type moves by 50%.",
  "Boosts Steel-type moves for the user and allies by 50%.",
  "Has a 10% chance to make the foe flinch when attacking. Does not stack with Razor Fang or King's Rock.",
  "Held item cannot be taken in battle.",
  "Draws in Water-type moves to boost Sp. Atk. No damage taken.",
  "Boosts biting moves by 50%.",
  "Prevents the Pokemon from being knocked out in one hit when at full HP.",
  "Draining moves recover more HP.",
  "Negates all moves and items that force switching out.",
  "Raises critical-hit ratio by one stage.",
  "Lowers the foe's Evasion upon entry.",
  "Boosts Atk and Sp. Atk for each fainted party member.",
  "Boosts Bug-type moves by 50% when HP is 1/3 or lower.",
  "Prevents the Pokemon and its allies from falling asleep or using Rest.",
  "Doubles Speed in rain.",
  "Lowers Defense of all other Pokemon.",
  "Passes held item to an ally who has used theirs.",
  "Passes burn, poison, or paralysis to the foe.",
  "Lowers Attack of all other Pokemon.",
  "Halves the accuracy of moves used against this Pokemon if it is confused.",
  "Lowers the attacker's Speed on contact.",
  "Boosts moves with 60 base power or less by 1.5x.",
  "Avoids damage from allies' attacks.",
  "Enters battle in Terastal Form.",
  "Moves ignore the target's Ability.",
  "Immune to Burn. Fire-type moves boost Attack.",
  "Halves damage from Fire- and Ice-type moves.",
  "Doubles the power of \"not very effective\" moves.",
  "Heals team status on entry. Sound moves become Psychic-type and get a 20% boost.",
  "Boosts Water-type moves by 50% when HP is 1/3 or lower.",
  "Boosts contact moves by 33%.",
  "Boosts physical attacks by 50% when poisoned.",
  "Attacks may badly poison the target.",
  "Sets Toxic Spikes when hit by a physical move.",
  "Copies the Ability of an opponent.",
  "Boosts Electric-type moves by 30%.",
  "Gives +3 priority to healing moves.",
  "Pokemon can only attack every other turn.",
  "Moves ignore the target's Ability.",
  "Attacks ignore the foe's stat changes.",
  "Doubles Speed if a held item is used or lost.",
  "Unnerves the foe, preventing them from eating Berries.",
  "Contact moves hit through protection.",
  "Lowers Sp. Atk of all other Pokemon.",
  "Raises Accuracy of the user and allies by 10%.",
  "Cannot fall asleep. Rest will fail if used.",
  "Restores 1/4 HP if hit by an Electric-type move. Also blocks status moves.",
  "Swaps Abilities with the attacker on contact.",
  "Restores 1/4 HP if hit by a Water-type move. Also blocks status moves.",
  "Does a few things.",
  "Sharply raises Defense if hit by a Water-type move (no immunity).",
  "Cannot be burned. Heals burn if gained while burned.",
  "Lowers Defense but sharply boosts Speed when hit by a physical move.",
  "Sharply raises Defense if hit by a Fire-type move.",
  "Stats cannot be lowered by other Pokemon.",
  "Flees the battle when HP drops below 50%.",
  "Wind moves charge the next Electric-type move.",
  "Boosts Attack in Tailwind or if hit by a wind move (nor will damage be taken).",
  "Only takes damage from super effective moves.",
  "Lowers the accuracy of incoming status moves to 50%.",
  "Changes form when HP drops below half.",
  "Changes to Hero Form when switching out.",
};

// Las que puede tener cada especie: [0] y [1] normales, [2] la
// oculta. 0 = no tiene esa ranura.
static const uint16_t ABIL_OF[1026][3] = {
  { 0, 0, 0 },
  { 156,   0,  27 },  // 1
  { 156,   0,  27 },  // 2
  { 156,   0,  27 },  // 3
  {  21,   0, 229 },  // 4
  {  21,   0, 229 },  // 5
  {  21,   0, 229 },  // 6
  { 272,   0, 188 },  // 7
  { 272,   0, 188 },  // 8
  { 272,   0, 188 },  // 9
  { 218,   0, 200 },  // 10
  { 215,   0,   0 },  // 11
  {  34,   0, 270 },  // 12
  { 218,   0, 200 },  // 13
  { 215,   0,   0 },  // 14
  { 255,   0, 225 },  // 15
  { 117, 262,  20 },  // 16
  { 117, 262,  20 },  // 17
  { 117, 262, 151 },  // 18
  { 200,  88,  98 },  // 19
  { 200,  88,  98 },  // 20
  { 117,   0, 225 },  // 21
  { 117,   0, 225 },  // 22
  { 112, 215, 284 },  // 23
  { 112, 215, 135 },  // 24
  { 239,   0, 123 },  // 25
  { 239,   0, 123 },  // 26
  { 205,   0, 202 },  // 27
  { 205,   0, 202 },  // 28
  { 169, 195,  98 },  // 29
  { 169, 195,  98 },  // 30
  { 169, 216, 195 },  // 31
  { 169, 195,  98 },  // 32
  { 169, 195,  98 },  // 33
  { 169, 216, 195 },  // 34
  {  42, 130,  75 },  // 35
  {  42, 130, 282 },  // 36
  {  68,   0,  55 },  // 37
  {  68,   0,  55 },  // 38
  {  42,  33, 139 },  // 39
  {  42,  33, 139 },  // 40
  { 110,   0, 108 },  // 41
  { 110,   0, 108 },  // 42
  {  27,   0, 200 },  // 43
  {  27,   0, 245 },  // 44
  {  27,   0,  59 },  // 45
  {  59,  56,  43 },  // 46
  {  59,  56,  43 },  // 47
  {  34, 270, 200 },  // 48
  { 218, 270, 302 },  // 49
  { 205,  10, 201 },  // 50
  { 205,  10, 201 },  // 51
  { 162, 264, 284 },  // 52
  { 124, 264, 273 },  // 53
  { 257,  29,  19 },  // 54
  { 257,  29,  19 },  // 55
  {   1,   7,  49 },  // 56
  {   1,   7,  49 },  // 57
  { 112,  68, 116 },  // 58
  { 112,  68, 116 },  // 59
  { 291,   0, 257 },  // 60
  { 291,   0, 257 },  // 61
  { 291,   0, 257 },  // 62
  { 260, 110, 130 },  // 63
  { 260, 110, 130 },  // 64
  { 260, 110, 130 },  // 65
  {  88, 151, 240 },  // 66
  {  88, 151, 240 },  // 67
  {  88, 151, 240 },  // 68
  {  27,   0,  80 },  // 69
  {  27,   0,  80 },  // 70
  {  27,   0,  80 },  // 71
  {  28, 126, 188 },  // 72
  {  28, 126, 188 },  // 73
  { 197, 249, 205 },  // 74
  { 197, 249, 205 },  // 75
  { 197, 249, 205 },  // 76
  { 191,  68,   1 },  // 77
  { 191,  68,   1 },  // 78
  { 152, 157, 193 },  // 79
  { 152, 193, 193 },  // 80
  { 132, 249,   6 },  // 81
  { 132, 249,   6 },  // 82
  { 252,   0,  49 },  // 83
  { 200, 264,  24 },  // 84
  { 143, 264,  24 },  // 85
  { 269,  99, 101 },  // 86
  { 269,  78, 101 },  // 87
  { 245, 246, 171 },  // 88
  { 245, 171, 246 },  // 89
  { 217, 222, 155 },  // 90
  { 217, 222, 155 },  // 91
  { 120,   0,   0 },  // 92
  { 120,   0,   0 },  // 93
  {  41,   0,   0 },  // 94
  { 197, 249, 295 },  // 95
  { 111,  74, 110 },  // 96
  { 111,  74, 110 },  // 97
  { 100, 217, 216 },  // 98
  { 100, 217, 216 },  // 99
  {  60,   0, 239 },  // 100
  {  60,   0,   4 },  // 101
  {  27,  90, 222 },  // 102
  {  27,  90,  55 },  // 103
  { 197, 123,  16 },  // 104
  { 197, 123,  16 },  // 105
  {  24, 191, 283 },  // 106
  { 117, 115, 110 },  // 107
  {  82, 152,  29 },  // 108
  { 120, 150, 139 },  // 109
  { 120, 150, 139 },  // 110
  { 123, 197, 191 },  // 111
  { 123, 197, 191 },  // 112
  { 148, 211,  91 },  // 113
  { 193,   0,  84 },  // 114
  {  57, 208,   1 },  // 115
  { 257, 225,  43 },  // 116
  { 169, 225,  43 },  // 117
  { 257, 294, 123 },  // 118
  { 257, 294, 123 },  // 119
  { 104, 148,   6 },  // 120
  { 104, 148,   6 },  // 121
  { 264,  65, 180 },  // 122
  { 255, 264, 240 },  // 123
  { 152,  74,  56 },  // 124
  { 239,   0, 288 },  // 125
  {  66,   0, 288 },  // 126
  { 100, 140, 143 },  // 127
  { 112,   1, 216 },  // 128
  { 257,   0, 189 },  // 129
  { 112,   0, 143 },  // 130
  { 291, 217,  99 },  // 131
  { 107,   0, 107 },  // 132
  { 200,   2,   9 },  // 133
  { 291,   0,  99 },  // 134
  { 289,   0, 187 },  // 135
  {  68,   0,  88 },  // 136
  { 277,  52,   6 },  // 137
  { 257, 217, 295 },  // 138
  { 257, 217, 295 },  // 139
  { 257,  16, 295 },  // 140
  { 257,  16, 295 },  // 141
  { 197, 175, 284 },  // 142
  { 106, 269,  80 },  // 143
  { 175,   0, 226 },  // 144
  { 175,   0, 239 },  // 145
  { 175,   0,  66 },  // 146
  { 215,   0, 133 },  // 147
  { 215,   0, 133 },  // 148
  { 110,   0, 144 },  // 149
  { 175,   0, 284 },  // 150
  { 260,   0,   0 },  // 151
  { 156,   0, 279 },  // 152
  { 156,   0, 279 },  // 153
  { 156,   0, 279 },  // 154
  {  21,   0,  19 },  // 155
  {  21,   0,  19 },  // 156
  {  21,   0,  19 },  // 157
  { 272,   0, 216 },  // 158
  { 272,   0, 216 },  // 159
  { 272,   0, 216 },  // 160
  { 200, 117,  76 },  // 161
  {  78,   0,  76 },  // 162
  { 111, 117, 270 },  // 163
  { 270, 117, 111 },  // 164
  { 255,  57, 189 },  // 165
  {  96,   0, 115 },  // 166
  { 255, 111, 225 },  // 167
  { 255, 171, 225 },  // 168
  { 110,   0, 108 },  // 169
  { 289, 104, 291 },  // 170
  { 289, 104, 291 },  // 171
  { 239,   0, 123 },  // 172
  {  42, 130,  75 },  // 173
  {  42,  33,  75 },  // 174
  {  98, 211, 252 },  // 175
  {  98, 211, 252 },  // 176
  { 129,  57, 149 },  // 177
  { 129,  57, 149 },  // 178
  { 239,   0, 165 },  // 179
  { 239,   0, 165 },  // 180
  { 239,   0, 165 },  // 181
  {  27,   0,  91 },  // 182
  { 269,  96, 206 },  // 183
  { 269,  96, 206 },  // 184
  { 249, 197, 189 },  // 185
  { 291,  43,  54 },  // 186
  {  27, 119, 108 },  // 187
  {  27, 119, 108 },  // 188
  {  27, 119, 108 },  // 189
  { 200, 162, 222 },  // 190
  {  27,   0, 229 },  // 191
  {  84,   0,  55 },  // 192
  { 233,  34,  76 },  // 193
  {  43, 291, 282 },  // 194
  {  43, 291, 282 },  // 195
  { 149,   0, 129 },  // 196
  { 260,   0, 110 },  // 197
  { 111, 252, 174 },  // 198
  { 152, 157, 193 },  // 199
  { 120,   0,   0 },  // 200
  { 120,   0,   0 },  // 201
  { 213,   0, 265 },  // 202
  { 180,   0, 206 },  // 203
  { 249,   0, 155 },  // 204
  { 249,   0, 155 },  // 205
  { 211, 200, 189 },  // 206
  { 100, 205, 106 },  // 207
  { 197, 249, 216 },  // 208
  { 112,   1, 189 },  // 209
  { 112,   1, 189 },  // 210
  { 169, 276, 112 },  // 211
  { 255, 264, 122 },  // 212
  { 249,  80,  35 },  // 213
  { 255,  88, 143 },  // 214
  { 110, 117, 161 },  // 215
  { 162, 187,  94 },  // 216
  {  88, 187, 284 },  // 217
  {  82,  66, 295 },  // 218
  {  82,  66, 295 },  // 219
  { 152, 226, 269 },  // 220
  { 152, 226, 269 },  // 221
  { 148,  98, 193 },  // 222
  {  98, 225, 141 },  // 223
  { 251, 225, 141 },  // 224
  { 288,  98, 111 },  // 225
  { 257, 291, 294 },  // 226
  { 117, 249, 295 },  // 227
  {  57,  68, 284 },  // 228
  {  57,  68, 284 },  // 229
  { 257, 225,  43 },  // 230
  { 162,   1, 205 },  // 231
  { 249,   1, 205 },  // 232
  { 277,  52,   6 },  // 233
  { 112,  76, 206 },  // 234
  { 157, 264, 141 },  // 235
  {  88, 240, 288 },  // 236
  { 112, 264, 240 },  // 237
  { 152,  74,  99 },  // 238
  { 239,   0, 288 },  // 239
  {  66,   0, 288 },  // 240
  { 269, 208, 206 },  // 241
  { 148, 211,  91 },  // 242
  { 175,   0, 289 },  // 243
  { 175,   0,  68 },  // 244
  { 175,   0, 291 },  // 245
  {  88,   0, 205 },  // 246
  { 215,   0,   0 },  // 247
  { 204,   0, 284 },  // 248
  { 175,   0, 144 },  // 249
  { 175,   0, 193 },  // 250
  { 148,   0,   0 },  // 251
  { 156,   0, 220 },  // 252
  { 156,   0, 220 },  // 253
  { 156,   0, 220 },  // 254
  {  21,   0, 233 },  // 255
  {  21,  24, 233 },  // 256
  {  21,  24, 233 },  // 257
  { 272,   0,  43 },  // 258
  { 272,   0,  43 },  // 259
  { 272,   0,  43 },  // 260
  { 200, 187, 189 },  // 261
  { 112, 187, 143 },  // 262
  { 161,  88, 187 },  // 263
  { 161,  88, 187 },  // 264
  { 218,   0, 200 },  // 265
  { 215,   0,   0 },  // 266
  { 255,   0, 195 },  // 267
  { 215,   0,   0 },  // 268
  { 218,   0,  34 },  // 269
  { 257, 188, 157 },  // 270
  { 257, 188, 157 },  // 271
  { 257, 188, 157 },  // 272
  {  27, 220, 161 },  // 273
  {  27, 220, 161 },  // 274
  { 300, 220, 214 },  // 275
  {  88,   0, 208 },  // 276
  {  88,   0, 208 },  // 277
  { 117,  99, 188 },  // 278
  { 117,  54, 188 },  // 279
  { 260, 277, 265 },  // 280
  { 260, 277, 265 },  // 281
  { 260, 277, 265 },  // 282
  { 257,   0, 188 },  // 283
  { 112,   0, 284 },  // 284
  {  59, 168, 187 },  // 285
  {  59, 264, 168 },  // 286
  { 280,   0,   0 },  // 287
  { 288,   0,   0 },  // 288
  { 280,   0,   0 },  // 289
  {  34,   0, 200 },  // 290
  { 233,   0, 108 },  // 291
  { 301,   0,   0 },  // 292
  { 232, 181, 189 },  // 293
  { 232, 181, 208 },  // 294
  { 232, 181, 208 },  // 295
  { 269,  88, 216 },  // 296
  { 269,  88, 216 },  // 297
  { 269,  96, 206 },  // 298
  { 249, 132, 201 },  // 299
  { 164,   0, 302 },  // 300
  { 164,   0, 302 },  // 301
  { 117, 235, 174 },  // 302
  { 100, 112, 216 },  // 303
  { 249, 197,  93 },  // 304
  { 249, 197,  93 },  // 305
  { 249, 197,  93 },  // 306
  { 182,   0, 265 },  // 307
  { 182,   0, 265 },  // 308
  { 239, 123, 137 },  // 309
  { 239, 123, 137 },  // 310
  { 165,   0, 123 },  // 311
  { 137,   0, 289 },  // 312
  { 104, 255, 174 },  // 313
  { 270, 104, 174 },  // 314
  { 148, 169, 119 },  // 315
  {  36,  82,  80 },  // 316
  {  36,  82,  80 },  // 317
  { 199,   1, 233 },  // 318
  { 199,   1, 233 },  // 319
  { 294, 152, 175 },  // 320
  { 294, 152, 175 },  // 321
  { 152, 221, 241 },  // 322
  { 216, 230, 241 },  // 323
  { 297,  55, 241 },  // 324
  { 269, 157,  80 },  // 325
  { 269, 157,  80 },  // 326
  { 157, 262,  35 },  // 327
  { 100,  10, 216 },  // 328
  { 120,   0,   0 },  // 329
  { 120,   0,   0 },  // 330
  { 205,   0, 291 },  // 331
  { 291,   0, 291 },  // 332
  { 148,   0,  29 },  // 333
  { 164,   0,  29 },  // 334
  { 106,   0, 274 },  // 335
  { 135,   0, 215 },  // 336
  { 120,   0,   0 },  // 337
  { 120,   0,   0 },  // 338
  { 152,   9,  99 },  // 339
  { 152,   9,  99 },  // 340
  { 100,   2, 217 },  // 341
  { 100,   2, 217 },  // 342
  { 120,   0,   0 },  // 343
  { 120,   0,   0 },  // 344
  { 251,   0, 247 },  // 345
  { 251,   0, 247 },  // 346
  {  16,   8, 257 },  // 347
  {  16,   8, 257 },  // 348
  { 257, 152,   2 },  // 349
  { 133,  33,  42 },  // 350
  {  73,   0,   0 },  // 351
  {  30,   0, 178 },  // 352
  { 111,  76,  41 },  // 353
  {  41,  76, 111 },  // 354
  { 120,   0,  76 },  // 355
  { 175,   0,  76 },  // 356
  {  90,   3,  55 },  // 357
  { 271,   0,   0 },  // 358
  { 175, 252, 116 },  // 359
  { 213,   0, 265 },  // 360
  { 166, 101, 141 },  // 361
  { 166, 101, 141 },  // 362
  { 269, 101, 152 },  // 363
  { 269, 101, 152 },  // 364
  { 269, 101, 152 },  // 365
  { 217,   0, 189 },  // 366
  { 257, 248, 294 },  // 367
  { 257,   0,  99 },  // 368
  { 257, 197, 249 },  // 369
  { 257,   0,  99 },  // 370
  { 197,   0, 216 },  // 371
  { 197,   0, 155 },  // 372
  { 112,   0, 143 },  // 373
  {  28,   0, 122 },  // 374
  {  28,   0, 122 },  // 375
  {  28,   0, 115 },  // 376
  {  28,   0, 249 },  // 377
  {  28,   0, 101 },  // 378
  {  28,   0, 122 },  // 379
  { 120,   0,   0 },  // 380
  { 120,   0,   0 },  // 381
  {  54,   0,   0 },  // 382
  {  55,   0,   0 },  // 383
  {   5,   0,   0 },  // 384
  { 211,   0,   0 },  // 385
  { 175,   0,   0 },  // 386
  { 156,   0, 217 },  // 387
  { 156,   0, 217 },  // 388
  { 156,   0, 197 },  // 389
  {  21,   0, 115 },  // 390
  {  21,   0, 115 },  // 391
  {  21,   0, 115 },  // 392
  { 272,   0,  33 },  // 393
  { 272,   0,  33 },  // 394
  { 272,   0,  33 },  // 395
  { 117,   0, 191 },  // 396
  { 112,   0, 191 },  // 397
  { 112,   0, 191 },  // 398
  { 221, 282, 141 },  // 399
  { 221, 282, 141 },  // 400
  { 215,   0, 200 },  // 401
  { 255, 264, 214 },  // 402
  { 195, 112,  88 },  // 403
  { 195, 112,  88 },  // 404
  { 248, 112,  88 },  // 405
  { 148, 169, 119 },  // 406
  { 148, 169, 264 },  // 407
  { 140,   0, 216 },  // 408
  { 140,   0, 216 },  // 409
  { 249,   0, 232 },  // 410
  { 249,   0, 232 },  // 411
  { 215,   0, 155 },  // 412
  {   9,   0, 155 },  // 413
  { 255,   0, 270 },  // 414
  {  94,   0,  98 },  // 415
  { 175, 112, 284 },  // 416
  {  78, 162, 289 },  // 417
  { 257,   0, 294 },  // 418
  { 257,   0, 294 },  // 419
  {  27,   0,   0 },  // 420
  {  70,   0,   0 },  // 421
  {  82, 247, 201 },  // 422
  {  82, 247, 201 },  // 423
  { 264, 162, 222 },  // 424
  {   4, 283,  67 },  // 425
  {   4, 283,  67 },  // 426
  { 200, 118, 124 },  // 427
  {  42,  24, 208 },  // 428
  { 120,   0,   0 },  // 429
  { 111, 252, 143 },  // 430
  { 124, 157, 117 },  // 431
  { 269, 157,  49 },  // 432
  { 271,   0,   0 },  // 433
  { 245,   4, 117 },  // 434
  { 245,   4, 117 },  // 435
  { 120,  92,  93 },  // 436
  { 120,  92,  93 },  // 437
  { 249, 197, 189 },  // 438
  { 232,  65, 264 },  // 439
  { 148, 211,  75 },  // 440
  { 117, 262, 181 },  // 441
  { 175,   0, 108 },  // 442
  { 205,   0, 199 },  // 443
  { 205,   0, 199 },  // 444
  { 205,   0, 199 },  // 445
  { 162, 269,  80 },  // 446
  { 240, 110, 174 },  // 447
  { 240, 110, 116 },  // 448
  { 204,   0, 201 },  // 449
  { 204,   0, 201 },  // 450
  {  16, 225, 117 },  // 451
  {  16, 225, 117 },  // 452
  { 171,  56,   9 },  // 453
  { 171,  56,   9 },  // 454
  { 120,   0,   0 },  // 455
  { 257, 247, 294 },  // 456
  { 257, 247, 294 },  // 457
  { 257, 291, 294 },  // 458
  { 227, 103, 232 },  // 459
  { 227, 103, 232 },  // 460
  { 110, 175, 161 },  // 461
  { 132, 249,   6 },  // 462
  {  82, 152,  29 },  // 463
  { 123, 230, 191 },  // 464
  { 193,   0,  84 },  // 465
  { 142,   0, 288 },  // 466
  {  66,   0, 134 },  // 467
  {  98, 211, 252 },  // 468
  { 233, 270,  76 },  // 469
  { 119,   0,  27 },  // 470
  { 226, 166, 101 },  // 471
  { 100, 205, 168 },  // 472
  { 152, 226, 269 },  // 473
  {   2,  52,   6 },  // 474
  { 240, 214, 116 },  // 475
  { 249, 132, 201 },  // 476
  { 175, 115,  76 },  // 477
  { 226, 166,  41 },  // 478
  { 120,   0,   0 },  // 479
  { 120,   0,   0 },  // 480
  { 120,   0,   0 },  // 481
  { 120,   0,   0 },  // 482
  { 175,   0, 265 },  // 483
  { 175,   0, 265 },  // 484
  {  68,   0,  66 },  // 485
  { 223,   0,   0 },  // 486
  { 175,   0, 265 },  // 487
  { 120,   0,   0 },  // 488
  {  99,   0,   0 },  // 489
  {  99,   0,   0 },  // 490
  {  13,   0,   0 },  // 491
  { 211,   0,   0 },  // 492
  { 145,   0,   0 },  // 493
  { 287,   0,   0 },  // 494
  { 156,   0,  35 },  // 495
  { 156,   0,  35 },  // 496
  { 156,   0,  35 },  // 497
  {  21,   0, 216 },  // 498
  {  21,   0, 216 },  // 499
  {  21,   0, 216 },  // 500
  { 272,   0, 217 },  // 501
  { 272,   0, 217 },  // 502
  { 272,   0, 214 },  // 503
  { 200, 117,   6 },  // 504
  { 104, 117,   6 },  // 505
  { 288, 162, 200 },  // 506
  { 112, 202, 208 },  // 507
  { 112, 202, 208 },  // 508
  { 124, 283, 174 },  // 509
  { 124, 283, 174 },  // 510
  {  80,   0, 156 },  // 511
  {  80,   0, 167 },  // 512
  {  80,   0,  21 },  // 513
  {  80,   0, 167 },  // 514
  {  80,   0, 272 },  // 515
  {  80,   0, 167 },  // 516
  {  74, 260, 139 },  // 517
  {  74, 260, 139 },  // 518
  { 252,   0, 195 },  // 519
  { 252,   0, 195 },  // 520
  { 252,   0, 195 },  // 521
  { 191, 142, 206 },  // 522
  { 191, 142, 206 },  // 523
  { 249, 295, 201 },  // 524
  { 249, 295, 201 },  // 525
  { 249, 204, 201 },  // 526
  { 282, 118, 221 },  // 527
  { 282, 118, 221 },  // 528
  { 202, 201, 140 },  // 529
  { 202, 201, 140 },  // 530
  { 279, 193, 118 },  // 531
  {  88, 216, 115 },  // 532
  {  88, 216, 115 },  // 533
  {  88, 216, 115 },  // 534
  { 257,  99, 291 },  // 535
  { 257,  99, 291 },  // 536
  { 257, 171, 291 },  // 537
  {  88, 110, 140 },  // 538
  { 249, 110, 140 },  // 539
  { 255,  27, 155 },  // 540
  { 119,  27, 155 },  // 541
  { 214,  27, 220 },  // 542
  { 169, 255, 233 },  // 543
  { 169, 255, 233 },  // 544
  { 169, 255, 233 },  // 545
  { 174, 108,  27 },  // 546
  { 174, 108,  27 },  // 547
  {  27, 157, 119 },  // 548
  {  27, 157, 119 },  // 549
  { 191,   2, 140 },  // 550
  { 112, 143,   7 },  // 551
  { 112, 143,   7 },  // 552
  { 112, 143,   7 },  // 553
  {  98,   0, 110 },  // 554
  { 216,   0, 303 },  // 555
  { 291,  27, 247 },  // 556
  { 249, 217, 295 },  // 557
  { 249, 217,   8 },  // 558
  { 215, 143, 112 },  // 559
  { 215, 143, 112 },  // 560
  { 302, 130, 270 },  // 561
  { 146,   0,   0 },  // 562
  { 146,   0,   0 },  // 563
  { 230, 249, 257 },  // 564
  { 230, 249, 257 },  // 565
  {  48,   0,   0 },  // 566
  {  48,   0,   0 },  // 567
  { 245, 246,  36 },  // 568
  { 245, 295,  36 },  // 569
  { 105,   0,   0 },  // 570
  { 105,   0,   0 },  // 571
  {  42, 264, 222 },  // 572
  {  42, 264, 222 },  // 573
  {  76,  33, 213 },  // 574
  {  76,  33, 213 },  // 575
  {  76,  33,  63 },  // 576
  { 155, 130, 193 },  // 577
  { 155, 130, 193 },  // 578
  { 155, 130, 193 },  // 579
  { 117,  20,  99 },  // 580
  { 127,   0,  99 },  // 581
  { 101, 226, 295 },  // 582
  { 101, 226, 295 },  // 583
  { 101, 227, 166 },  // 584
  {  27, 206, 220 },  // 585
  {  27, 206, 220 },  // 586
  { 239,   0, 142 },  // 587
  { 255, 215, 151 },  // 588
  { 255, 217, 155 },  // 589
  {  59,   0, 193 },  // 590
  {  59,   0, 193 },  // 591
  { 291,  41,  43 },  // 592
  { 291,  41,  43 },  // 593
  {  99,  91, 193 },  // 594
  {  34, 284, 255 },  // 595
  {  34, 284, 255 },  // 596
  { 114,   0,   9 },  // 597
  { 114,   0,   9 },  // 598
  { 165, 137,  28 },  // 599
  { 165, 137,  28 },  // 600
  { 120,   0,  28 },  // 601
  { 120,   0,   0 },  // 602
  { 120,   0,   0 },  // 603
  { 120,   0,   0 },  // 604
  { 265, 260,   6 },  // 605
  { 265, 260,   6 },  // 606
  {  68,  66, 108 },  // 607
  {  68,  66, 108 },  // 608
  {  68,  66, 108 },  // 609
  { 195, 140, 284 },  // 610
  { 195, 140, 284 },  // 611
  { 195, 140, 284 },  // 612
  { 226, 224, 189 },  // 613
  { 226, 224, 257 },  // 614
  { 120,   0, 166 },  // 615
  {  99, 217, 155 },  // 616
  {  99,  56, 283 },  // 617
  { 239, 124, 247 },  // 618
  { 191, 193, 110 },  // 619
  { 191, 193, 110 },  // 620
  { 199, 216, 140 },  // 621
  { 115, 118, 151 },  // 622
  { 115,   0, 151 },  // 623
  {  49, 110, 175 },  // 624
  {  49, 110, 175 },  // 625
  { 191, 206,   1 },  // 626
  { 117, 216,  98 },  // 627
  { 117, 216,  49 },  // 628
  {  20, 155, 295 },  // 629
  {  20, 155, 295 },  // 630
  {  68,  58, 297 },  // 631
  { 255,  98, 280 },  // 632
  { 112,   0,   0 },  // 633
  { 112,   0,   0 },  // 634
  { 120,   0,   0 },  // 635
  {  66,   0, 255 },  // 636
  {  66,   0, 255 },  // 637
  { 116,   0,   0 },  // 638
  { 116,   0,   0 },  // 639
  { 116,   0,   0 },  // 640
  { 174, 193,  49 },  // 641
  { 174, 289,  49 },  // 642
  { 281,   0,   0 },  // 643
  { 267,   0,   0 },  // 644
  { 201, 112, 216 },  // 645
  { 175,   0,   0 },  // 646
  { 116,   0,   0 },  // 647
  { 211,   0,   0 },  // 648
  {  52,   0,   0 },  // 649
  { 156,   0,  22 },  // 650
  { 156,   0,  22 },  // 651
  { 156,   0,  22 },  // 652
  {  21,   0, 130 },  // 653
  {  21,   0, 130 },  // 654
  {  21,   0, 130 },  // 655
  { 272,   0, 178 },  // 656
  { 272,   0, 178 },  // 657
  { 272,   0, 178 },  // 658
  { 162,  25,  96 },  // 659
  { 162,  25,  96 },  // 660
  {  20,   0,  79 },  // 661
  {  66,   0,  79 },  // 662
  {  66,   0,  79 },  // 663
  { 218,  34, 270 },  // 664
  { 215,  34, 270 },  // 665
  { 218,  34, 270 },  // 666
  { 195, 284, 112 },  // 667
  { 195, 284, 112 },  // 668
  {  69,   0,  84 },  // 669
  {  69,   0,  84 },  // 670
  {  69,   0,  84 },  // 671
  { 206,   1, 163 },  // 672
  { 206,   1, 163 },  // 673
  { 115, 140, 208 },  // 674
  { 115, 140, 208 },  // 675
  {  78,   0,   0 },  // 676
  { 149, 108, 157 },  // 677
  { 174, 149,  33 },  // 678
  { 120,   0,   0 },  // 679
  { 120,   0,   0 },  // 680
  { 238,   0,   0 },  // 681
  {  91,  42,  12 },  // 682
  {  91,  42,  12 },  // 683
  {  91, 283, 256 },  // 684
  {  91, 283, 256 },  // 685
  {  35, 251, 108 },  // 686
  {  35, 251, 108 },  // 687
  { 273, 225,   8 },  // 688
  { 273, 225,   8 },  // 689
  { 169, 171,   2 },  // 690
  { 169, 171,   2 },  // 691
  { 134,   0,   0 },  // 692
  { 134,   0,   0 },  // 693
  {  56, 205, 229 },  // 694
  {  56, 205, 229 },  // 695
  { 248,   0, 249 },  // 696
  { 248,   0, 197 },  // 697
  { 192,   0, 103 },  // 698
  { 192, 227, 103 },  // 699
  { 164,   0,  42 },  // 700
  { 283, 124, 140 },  // 701
  {  60,  25, 164 },  // 702
  {  28,   0, 249 },  // 703
  { 206,  99,  82 },  // 704
  { 206,  99,  82 },  // 705
  { 206,  99,  82 },  // 706
  { 174,   0, 131 },  // 707
  { 148,  76,  90 },  // 708
  { 148,  76,  90 },  // 709
  {  68,  68,  68 },  // 710
  {  68,  68,  68 },  // 711
  { 166, 101, 249 },  // 712
  { 166, 101, 249 },  // 713
  {  76, 108, 265 },  // 714
  {  76, 108, 265 },  // 715
  {  64,   0,   0 },  // 716
  {  45,   0,   0 },  // 717
  { 172,   0,   0 },  // 718
  {  28,   0,   0 },  // 719
  { 131,   0,   0 },  // 720
  { 291,  68,   0 },  // 721
  { 156,   0, 128 },  // 722
  { 156,   0, 128 },  // 723
  { 156, 128,   1 },  // 724
  {  21,   0, 112 },  // 725
  {  21,   0, 112 },  // 726
  {  21,   0, 112 },  // 727
  { 272,   0, 127 },  // 728
  { 272,   0, 127 },  // 729
  { 272,   0, 127 },  // 730
  { 117, 222, 162 },  // 731
  { 117, 222, 162 },  // 732
  { 117, 222, 216 },  // 733
  { 234, 248,   2 },  // 734
  { 234, 248,   2 },  // 735
  { 255,   0,   0 },  // 736
  {  15,   0,   0 },  // 737
  { 120,   0,   0 },  // 738
  { 100, 115,   7 },  // 739
  { 100, 115,   7 },  // 740
  {  44,   0,   0 },  // 741
  { 270, 218, 256 },  // 742
  { 270, 218, 256 },  // 743
  { 117, 288, 240 },  // 744
  { 117, 202, 240 },  // 745
  { 207,   0,   0 },  // 746
  { 135, 124, 193 },  // 747
  { 135, 124, 193 },  // 748
  { 157, 237, 110 },  // 749
  { 157, 237, 110 },  // 750
  { 292,   0, 291 },  // 751
  { 292,   0, 291 },  // 752
  { 119,   0,  35 },  // 753
  {   2,   0,  35 },  // 754
  { 104,  59, 188 },  // 755
  { 104,  59, 188 },  // 756
  {  36,   0, 152 },  // 757
  {  36,   0, 152 },  // 758
  {  72,   0,  42 },  // 759
  {  72,   0, 208 },  // 760
  { 119, 152, 256 },  // 761
  { 119, 152, 256 },  // 762
  {  24, 185, 256 },  // 763
  { 279,  71, 148 },  // 764
  { 110, 265, 259 },  // 765
  { 190,  74,  49 },  // 766
  { 298,   0,   0 },  // 767
  {  62,   0,   0 },  // 768
  { 293,   0, 205 },  // 769
  { 293,   0, 205 },  // 770
  { 109,   0, 282 },  // 771
  {  16,   0,   0 },  // 772
  { 196,   0,   0 },  // 773
  { 219,   0,   0 },  // 774
  {  31,   0,   0 },  // 775
  { 217,   0,   0 },  // 776
  { 114, 123, 249 },  // 777
  {  51,   0,   0 },  // 778
  {  47, 248, 302 },  // 779
  {  19, 206,  29 },  // 780
  { 243,   0,   0 },  // 781
  {  22, 232, 155 },  // 782
  {  22, 232, 155 },  // 783
  {  22, 232, 155 },  // 784
  {  60,   0, 265 },  // 785
  { 180,   0, 265 },  // 786
  {  84,   0, 265 },  // 787
  { 139,   0, 265 },  // 788
  { 282,   0,   0 },  // 789
  { 249,   0,   0 },  // 790
  {  77,   0,   0 },  // 791
  { 212,   0,   0 },  // 792
  {  18,   0,   0 },  // 793
  {  18,   0,   0 },  // 794
  {  18,   0,   0 },  // 795
  {  18,   0,   0 },  // 796
  {  18,   0,   0 },  // 797
  {  18,   0,   0 },  // 798
  {  18,   0,   0 },  // 799
  { 176,   0,   0 },  // 800
  { 231,   0,   0 },  // 801
  { 264,   0,   0 },  // 802
  {  18,   0,   0 },  // 803
  {  18,   0,   0 },  // 804
  {  18,   0,   0 },  // 805
  {  18,   0,   0 },  // 806
  { 289,   0,   0 },  // 807
  { 132,   0,   0 },  // 808
  { 115,   0,   0 },  // 809
  { 156,   0,  84 },  // 810
  { 156,   0,  84 },  // 811
  { 156,   0,  84 },  // 812
  {  21,   0, 121 },  // 813
  {  21,  24, 121 },  // 814
  {  21,  24, 121 },  // 815
  { 272,   0, 225 },  // 816
  { 272,   0, 225 },  // 817
  { 272,   0, 225 },  // 818
  {  25,   0,  80 },  // 819
  {  25,   0,  80 },  // 820
  { 117, 284,  20 },  // 821
  { 117, 284,  20 },  // 822
  { 175, 284, 138 },  // 823
  { 255,  34, 149 },  // 824
  { 255,  34, 149 },  // 825
  { 149,   0, 180 },  // 826
  { 200, 283, 264 },  // 827
  { 200, 283, 264 },  // 828
  {  38, 193,  59 },  // 829
  {  38, 193,  59 },  // 830
  {  72,   0,  22 },  // 831
  {  72,   0,  22 },  // 832
  { 248, 217, 257 },  // 833
  { 248, 217, 257 },  // 834
  {  14, 239, 189 },  // 835
  { 248, 239,  33 },  // 836
  { 241,  92,  68 },  // 837
  { 241,  66,  68 },  // 838
  { 241,  66,  68 },  // 839
  { 194,  80,  22 },  // 840
  { 194,  80,  98 },  // 841
  { 194,  80, 269 },  // 842
  { 203, 215, 205 },  // 843
  { 203, 215, 205 },  // 844
  {  87,   0,   0 },  // 845
  { 257,   0, 112 },  // 846
  { 257,   0, 177 },  // 847
  { 189, 239, 118 },  // 848
  { 232, 181, 264 },  // 849
  {  68, 297,  66 },  // 850
  {  68, 297,  66 },  // 851
  { 124,   0, 264 },  // 852
  { 124,   0, 264 },  // 853
  { 295,   0,  41 },  // 854
  { 295,   0,  41 },  // 855
  {  91,   9, 129 },  // 856
  {  91,   9, 129 },  // 857
  {  91,   9, 129 },  // 858
  { 174,  76, 161 },  // 859
  { 174,  76, 161 },  // 860
  { 174,  76, 161 },  // 861
  { 191,  88,  49 },  // 862
  {  16, 273, 244 },  // 863
  { 295,   0, 160 },  // 864
  { 240,   0, 208 },  // 865
  { 264, 209, 180 },  // 866
  { 290,   0,   0 },  // 867
  { 193,   0,  42 },  // 868
  { 296,   0,  42 },  // 869
  { 222,  16,  49 },  // 870
  {  60,   0, 123 },  // 871
  { 103,   0, 227 },  // 872
  { 103, 270, 227 },  // 873
  { 173,   0,   0 },  // 874
  { 102,   0,   0 },  // 875
  { 180,   0, 260 },  // 876
  {  97,   0,   0 },  // 877
  { 216,   0,  93 },  // 878
  { 216,   0,  93 },  // 879
  { 289,  98, 202 },  // 880
  { 289, 239, 224 },  // 881
  { 291, 248, 202 },  // 882
  { 291, 101, 224 },  // 883
  { 122,  93, 236 },  // 884
  {  28, 108,  41 },  // 885
  {  28, 108,  41 },  // 886
  {  28, 108,  41 },  // 887
  { 113,   0,   0 },  // 888
  {  46,   0,   0 },  // 889
  { 175,   0,   0 },  // 890
  { 110,   0,   0 },  // 891
  { 285,   0,   0 },  // 892
  { 119,   0,   0 },  // 893
  { 278,   0,   0 },  // 894
  {  53,   0,   0 },  // 895
  {  26,   0,   0 },  // 896
  {  85,   0,   0 },  // 897
  { 284,   0,   0 },  // 898
  { 112,  76, 206 },  // 899
  { 255, 216, 214 },  // 900
  {  88,  22, 284 },  // 901
  { 191,   2, 140 },  // 902
  { 175, 283, 171 },  // 903
  { 169, 276, 112 },  // 904
  {  42, 155,  35 },  // 905
  { 156,   0, 178 },  // 906
  { 156,   0, 178 },  // 907
  { 156,   0, 178 },  // 908
  {  21,   0, 282 },  // 909
  {  21,   0, 282 },  // 910
  {  21,   0, 282 },  // 911
  { 272,   0, 143 },  // 912
  { 272,   0, 143 },  // 913
  { 272,   0, 143 },  // 914
  {  12,  80, 269 },  // 915
  { 125,  80, 269 },  // 916
  { 111,   0, 255 },  // 917
  { 264,   0, 234 },  // 918
  { 255,   0, 270 },  // 919
  { 255,   0, 270 },  // 920
  { 239, 148, 115 },  // 921
  { 289, 148, 115 },  // 922
  { 289, 148, 115 },  // 923
  { 162, 200, 157 },  // 924
  {  75,  25, 264 },  // 925
  { 157,   0, 118 },  // 926
  { 296,   0,  72 },  // 927
  {  57,   0,  90 },  // 928
  {  57,   0,  90 },  // 929
  { 210,   0,  90 },  // 930
  { 112,  98,  88 },  // 931
  { 183, 249,  28 },  // 932
  { 183, 249,  28 },  // 933
  { 183, 249,  28 },  // 934
  {  68,   0,  66 },  // 935
  {  68,   0, 295 },  // 936
  {  68,   0, 295 },  // 937
  { 157, 239,  43 },  // 938
  { 239,  61,  43 },  // 939
  { 299, 289,  33 },  // 940
  { 299, 289,  33 },  // 941
  { 112, 200, 234 },  // 942
  { 112,  86, 234 },  // 943
  { 283, 161, 174 },  // 944
  { 283, 171, 174 },  // 945
  { 300,   0, 108 },  // 946
  { 300,   0, 108 },  // 947
  { 147,   0,   0 },  // 948
  { 147,   0,   0 },  // 949
  {   8, 217, 193 },  // 950
  {  27, 111, 118 },  // 951
  {  27, 111, 141 },  // 952
  {  34,   0, 215 },  // 953
  { 260,   0, 265 },  // 954
  {   9,  76, 233 },  // 955
  { 153,  76, 233 },  // 956
  { 140, 157, 161 },  // 957
  { 140, 157, 161 },  // 958
  { 140, 157, 161 },  // 959
  {  82, 189, 205 },  // 960
  {  82, 189, 205 },  // 961
  { 198,   0,  98 },  // 962
  { 294,   0,   0 },  // 963
  { 304,   0,   0 },  // 964
  { 155,   0, 223 },  // 965
  { 155,   0,  65 },  // 966
  { 215,   0, 193 },  // 967
  {  58,   0, 205 },  // 968
  { 276,   0,  36 },  // 969
  { 276,   0,  36 },  // 970
  { 162,   0,  72 },  // 971
  { 202,   0,  72 },  // 972
  { 208,  24,  37 },  // 973
  { 269, 226, 216 },  // 974
  { 269, 224, 216 },  // 975
  { 140,   0, 214 },  // 976
  { 282, 152, 294 },  // 977
  {  32,   0, 247 },  // 978
  {   1, 110,  49 },  // 979
  { 169, 291, 282 },  // 980
  { 180,  11,  39 },  // 981
  { 211, 200, 189 },  // 982
  {  49, 254, 175 },  // 983
  { 179,   0,   0 },  // 984
  { 179,   0,   0 },  // 985
  { 179,   0,   0 },  // 986
  { 179,   0,   0 },  // 987
  { 179,   0,   0 },  // 988
  { 179,   0,   0 },  // 989
  { 184,   0,   0 },  // 990
  { 184,   0,   0 },  // 991
  { 184,   0,   0 },  // 992
  { 184,   0,   0 },  // 993
  { 184,   0,   0 },  // 994
  { 184,   0,   0 },  // 995
  { 268,   0, 101 },  // 996
  { 268, 101, 269 },  // 997
  { 268, 101, 269 },  // 998
  { 189,   0,   0 },  // 999
  {  81,   0,   0 },  // 1000
  { 261,   0,   0 },  // 1001
  { 258,   0,   0 },  // 1002
  { 286,   0,   0 },  // 1003
  {  17,   0,   0 },  // 1004
  { 179,   0,   0 },  // 1005
  { 184,   0,   0 },  // 1006
  { 154,   0,   0 },  // 1007
  {  89,   0,   0 },  // 1008
  { 179,   0,   0 },  // 1009
  { 184,   0,   0 },  // 1010
  { 253,  80, 246 },  // 1011
  {  95,   0,  92 },  // 1012
  {  95,  41,  92 },  // 1013
  { 275,   0,  86 },  // 1014
  { 275,   0,  76 },  // 1015
  { 275,   0, 264 },  // 1016
  {  49,   0,   0 },  // 1017
  { 237, 249, 236 },  // 1018
  { 253, 193, 246 },  // 1019
  { 179,   0,   0 },  // 1020
  { 179,   0,   0 },  // 1021
  { 184,   0,   0 },  // 1022
  { 184,   0,   0 },  // 1023
  { 266,   0,   0 },  // 1024
  { 170,   0,   0 },  // 1025
};

static inline const AbilityEntry &abilAt(uint16_t i) {
  return ABIL_TBL[i < ABIL_COUNT ? i : 0];
}
