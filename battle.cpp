#include <string.h>
#include "battle.h"

#include <new>      // placement new, para Battle::start()

#include <Arduino.h>

// ------------------------------------------------------- o dado do combate
static bool rngProprio = false;
static uint32_t rngS = 0;

void btRngSemente(uint32_t s) { rngS = s ? s : 0x9E3779B9u; rngProprio = true; }
void btRngDesliga() { rngProprio = false; }
bool btRngLigado() { return rngProprio; }
uint32_t btRngEstado() { return rngProprio ? rngS : 0; }

long btRand(long n) {
  if (!rngProprio) return random(n);
  if (n <= 0) return 0;
  // xorshift32: pequeno, sem tabelas, e igual em qualquer placa e no PC
  rngS ^= rngS << 13;
  rngS ^= rngS >> 17;
  rngS ^= rngS << 5;
  return (long)(rngS % (uint32_t)n);
}

static void hMix(uint32_t &h, uint32_t v) {
  for (int i = 0; i < 4; i++) { h ^= (v >> (i * 8)) & 0xFF; h *= 16777619u; }
}

uint32_t btHash(const Battle &b) {
  uint32_t h = 2166136261u;
  hMix(h, b.turn);
  for (uint8_t l = 0; l < 2; l++) {
    const Combatant &c = b.side[l];
    hMix(h, (uint32_t)(uint16_t)c.dex | ((uint32_t)(uint16_t)c.asDex << 16));
    hMix(h, c.level);
    hMix(h, (uint32_t)c.hp | ((uint32_t)c.hpMax << 16));
    for (uint8_t i = 0; i < BT_MOVES; i++)
      hMix(h, (uint32_t)c.moves[i] | ((uint32_t)c.usesLeft[i] << 16));
    hMix(h, (uint32_t)c.status | ((uint32_t)c.statusTurns << 8) | ((uint32_t)c.item << 16));
    hMix(h, c.ability);
    for (uint8_t i = 0; i < SG_COUNT; i++) hMix(h, (uint8_t)c.stage[i]);
    hMix(h, (uint32_t)c.confuseTurns | ((uint32_t)c.flinched << 8) | ((uint32_t)c.protecting << 16));
    hMix(h, (uint32_t)b.rocks[l] | ((uint32_t)b.spikes[l] << 8) |
                ((uint32_t)b.tspikes[l] << 16) | ((uint32_t)b.web[l] << 24));
    hMix(h, (uint32_t)b.tailwind[l] | ((uint32_t)b.futureTurns[l] << 8) |
                ((uint32_t)b.wishTurns[l] << 16));
  }
  hMix(h, (uint32_t)b.weather | ((uint32_t)b.weatherTurns << 8) |
              ((uint32_t)b.terrain << 16) | ((uint32_t)b.terrainTurns << 24));
  hMix(h, (uint32_t)b.trickRoom | ((uint32_t)b.uproar << 8) |
              ((uint32_t)b.winner << 16) | ((uint32_t)b.over << 24));
  hMix(h, btRngEstado());
  return h;
}

// La cuenta de los stats vive en dex.h (statAtLevel). Estaba copiada aqui y
// en pet.cpp, que es justo como se despegan: el bicho criado tiene que
// pelear con los MISMOS numeros que enseña su ficha.

void Combatant::init(int16_t species, uint16_t lvl, bool isShiny,
                     uint8_t formNum) {
  dex = asDex = species;
  level = lvl ? lvl : 1;
  shiny = isShiny;
  // La forma ANTES de la cuenta de PS: un Meowth de Galar no tiene los
  // mismos PS base que uno normal, y ponerla despues dejaria la barra
  // con el numero del otro.
  form = formNum;
  megaIdx = 0xFF;
  transformed = false;
  // El entrenamiento cuenta: antes esta cuenta se lo saltaba y un bicho con
  // los PS subidos peleaba con los PS de serie. Ahora es la MISMA que usa la
  // ficha (hpAtLevel, en dex.h), que es lo unico que garantiza que lo que se
  // ve fuera del combate sea lo que entra en el.
  hpMax = valid() ? hpAtLevelBase(base(0), level, trHp) : 1;
  hp = hpMax;
  nMoves = movesAtLevel(asDex, level, moves);
  // PP llenos: es el punto de partida. Al bicho del jugador se los pisa
  // battleStart() con los que traia de antes, que son los que cuentan.
  for (uint8_t i = 0; i < nMoves; i++) usesLeft[i] = movePP(moves[i]);
  for (uint8_t i = nMoves; i < BT_MOVES; i++) { moves[i] = 0; usesLeft[i] = 0; }
  // El sexo arranca en "no se sabe" y lo pone QUIEN CREA al combatiente:
  // el del jugador sale de su bicho (que lo tiene desde que nacio) y el
  // del salvaje, de rollSexo() al aparecer.
  //
  // Aqui NO se tira el dado, y el motivo merece quedar escrito: init()
  // la llaman cientos de tests con semilla fija, y gastar un random mas
  // les corre la secuencia a TODOS. Lo descubri rompiendo uno que no
  // tenia nada que ver (Technician): el dado del sexo le cambio los
  // criticos. Una funcion que usan los tests como punto de partida
  // tiene que ser determinista.
  sexo = 2;
  turnsOut = 0;
}

// Copia al otro. Sigue a `pbTransform` del Anil
// (029_Battler/004_Battler_ChangeSelf.rb): especie, tipos, HABILIDAD,
// los cinco stats, los ESCALONES y SUS golpes a 5 PP. Los PS no, igual
// que en los juegos: si Transform copiara los PS, un Ditto contra un
// Snorlax se curaria al transformarse.
void Combatant::transformInto(const Combatant &other) {
  if (!other.valid()) return;
  asDex = other.asDex;
  transformed = true;
  // LA HABILIDAD. Faltaba, y es media gracia de copiar: un Ditto que
  // copiaba un Gyarados se quedaba con la suya.
  ability = other.ability;
  // LOS STATS YA CALCULADOS (menos los PS). Antes solo se copiaba la
  // especie aparente y se recalculaba con MIS genes y MI nivel, asi que
  // copiar a uno de nivel 50 desde el nivel 5 no servia de nada.
  copiado[0] = statAtLevel(other.base(1), other.geneAtk, other.level, other.trAtk);
  copiado[1] = statAtLevel(other.base(2), other.geneDef, other.level, other.trDef);
  copiado[2] = statAtLevel(other.base(3), other.geneSpa, other.level, other.trSpa);
  copiado[3] = statAtLevel(other.base(4), other.geneSpd, other.level, other.trSpd);
  copiado[4] = statAtLevel(other.base(5), other.geneSpe, other.level, other.trSpe);
  // LOS ESCALONES, uno a uno, y el escalon de critico con ellos
  // (el Anil copia FocusEnergy y LaserFocus con MECHANICS_GENERATION 9).
  for (uint8_t i = 0; i < SG_COUNT; i++) stage[i] = other.stage[i];
  critStage = other.critStage;
  sureCrit = other.sureCrit;
  // SUS GOLPES, no los que me tocarian por nivel. Esto era lo peor de
  // todo: se llamaba a movesAtLevel(especie copiada, MI nivel), asi que
  // copiar un Dragonite de 55 daba los golpes de un Dragonite de 5.
  nMoves = 0;
  for (uint8_t i = 0; i < BT_MOVES && i < other.nMoves; i++) {
    if (!other.moves[i]) continue;
    moves[nMoves] = other.moves[i];
    usesLeft[nMoves] = 5;      // 5 PP, como en el Anil
    nMoves++;
  }
  for (uint8_t i = nMoves; i < BT_MOVES; i++) { moves[i] = 0; usesLeft[i] = 0; }
  // Y se olvida el Disable: apuntaba a un hueco de la lista ANTERIOR.
  disabledSlot = 0xFF;
  disableTurns = 0;
}

// Tabla de siempre: +1 es x1,5 y -1 es x0,67, hasta x4 y x0,25.
uint16_t stageMul(int8_t s) {
  static const uint16_t T[13] = { 25, 29, 33, 40, 50, 67, 100,
                                  150, 200, 250, 300, 350, 400 };
  if (s < -6) s = -6;
  if (s > 6) s = 6;
  return T[s + 6];
}

static uint16_t conStage(uint16_t base, int8_t st) {
  uint32_t v = (uint32_t)base * stageMul(st) / 100;
  return v ? (uint16_t)(v > 65535 ? 65535 : v) : 1;
}

// El stat BASE que toca usar: el de la mega si esta megaevolucionado, y
// si no el de la especie. Un solo sitio para los seis, para que no pueda
// haber un stat que se olvide de mirar la mega.
//   0 PS  1 Atq  2 Def  3 AtqEsp  4 DefEsp  5 Vel
uint16_t Combatant::base(uint8_t cual) const {
  if (!valid()) return 1;
  if (isMega()) {
    const MegaForm &m = MEGA_TBL[megaIdx];
    switch (cual) {
      case 0: return m.bHp;
      case 1: return m.bAtk;
      case 2: return m.bDef;
      case 3: return m.bSpA;
      case 4: return m.bSpD;
      default: return m.bSpe;
    }
  }
  if (bform >= 0) {
    const BattleForm &f = BFORM_TBL[bform];
    switch (cual) {
      case 0: return f.bHp;
      case 1: return f.bAtk;
      case 2: return f.bDef;
      case 3: return f.bSpA;
      case 4: return f.bSpD;
      default: return f.bSpe;
    }
  }
  int16_t fr = formRow_();
  if (fr >= 0) {
    const FormVariant &f = FORM_TBL[fr];
    switch (cual) {
      case 0: return f.bHp;
      case 1: return f.bAtk;
      case 2: return f.bDef;
      case 3: return f.bSpA;
      case 4: return f.bSpD;
      default: return f.bSpe;
    }
  }
  const DexEntry &d = DEX_TBL[asDex];
  switch (cual) {
    case 0: return d.bHp;
    case 1: return d.bAtk;
    case 2: return d.bDef;
    case 3: return d.bSpA;
    case 4: return d.bSpD;
    default: return d.bSpe;
  }
}

// LO QUE PESA AHORA MISMO, en decimas de kilo. No es el de la tabla:
// Peso Pesado lo dobla y cada Autotomia le quita 100 kg. En el Anil es
// `user.pbWeight + user.effects[PBEffects::WeightChange]`, con minimo
// 0,1 kg —— por eso el suelo es 1 y no 0.
//
// Vive aqui, en un sitio solo, porque lo miran DOS golpes (Patada Baja
// contra el rival y Cuerpo Pesado comparando los dos) y antes cada uno
// leia la tabla por su cuenta: anadir Autotomia habria sido acordarse
// de los dos, y de los que vengan.
static uint16_t pesoAhora(const Combatant &c) {
  uint32_t w = c.valid() ? DEX_TBL[c.asDex].weight10 : 1;
  if (c.abil() == AB_HEAVY_METAL) w *= 2;
  uint32_t quita = (uint32_t)c.ligero * 1000;   // 100,0 kg por cada una
  w = (w > quita) ? (w - quita) : 1;
  return (uint16_t)(w ? w : 1);
}

// LAS BAYAS QUE CURAN PS son DOS reglas distintas, y en el Anil estan
// una al lado de la otra (010_Battle_ItemEffects.rb):
//
//   SITRUSBERRY   amt = battler.totalhp / 4     una fraccion
//   ORANBERRY     amt = 10                      diez PS a secas
//
// Aqui las dos eran un porcentaje, y a la Aranja eso le cambia el
// sentido: a uno de nivel 50 con 150 PS le curaba 18 en vez de 10, y a
// uno recien nacido le curaba dos. Una baya de 20 monedas no escala con
// el nivel —— para eso esta la Zidra, que cuesta lo mismo y SI es una
// fraccion, y esa es justo la decision que el jugador tiene que poder
// tomar.
//
// Vive en un sitio solo porque la miran tres: el cierre de turno,
// Picadura (que se come la del rival) y Lanzamiento (que pesa lo que
// tira).
// EMBARGO: mientras dure, el objeto que LLEVA no hace nada —— ni
// Restos, ni bayas, ni banda, ni Vidasfera. Se sigue llevando (se puede
// robar o tirar), solo que no actua. Todo lo que mira el EFECTO del
// objeto en combate pasa por aqui; lo que mira si HAY objeto (Desarme,
// Ladron, Trick) sigue mirando `item`.
static inline const ItemEntry &objeto(const Combatant &c) {
  return itemAt(c.embargoTurns ? (uint8_t)IT_NONE : c.item);
}

// Los golpes que ANTICURA no deja usar: los que curan al que los usa y,
// desde la sexta generacion, los que drenan (el Anil: healingMove?).
static bool curaAlUsarlo(uint16_t mv) {
  if (mv >= MOVE_COUNT) return false;
  switch (MOVE_TBL[mv].fx) {
    case FX_HEAL_HALF: case FX_HEAL_WEATHER: case FX_HEAL_QUARTER:
    case FX_REST: case FX_ROOST: case FX_SHORE_UP: case FX_SWALLOW:
    case FX_PURIFY: case FX_SAP_ATK: case FX_WISH: case FX_HEALING_WISH:
    case FX_DRAIN: case FX_DRAIN_3_4: case FX_DRAIN_SLEEP:
      return true;
    default:
      return false;
  }
}

// DON NATURAL: tipo y potencia de la baya que lleva (los NaturalGift_*
// del items.txt del Anil). Hoy hay tres bayas y cada una tiene un efecto
// propio, asi que el efecto la identifica. false = no lleva ninguna.
static bool donNatural(const Combatant &c, uint8_t &tipo, uint8_t &pot) {
  switch (objeto(c).fx) {
    case IT_BERRY_HEAL: tipo = T_PSYCHIC; pot = 80; return true;   // Zidra
    case IT_BERRY_FIX:  tipo = T_POISON;  pot = 80; return true;   // Aranja
    case IT_BERRY_CURE: tipo = T_FLYING;  pot = 80; return true;   // Ziuela
    default: return false;
  }
}
// Lo que computeHit tiene que usar en ESTE golpe: la baya del Don Natural
// se gasta antes de pegar (el Anil la gasta aunque falle), y la potencia
// de cada golpe de Paliza depende de quien lo da. 0 = no hay nada puesto.
static uint8_t gDonTipo = 0, gDonPot = 0, gPalizaPot = 0;
// MAGNITUD: el nivel se sortea en useMove (pbOnStartUse del Anil: antes de
// la precision y de Proteccion, y se anuncia aunque falle) y computeHit lo
// lee de aqui. 0 = sortearlo en computeHit (los tests que lo llaman suelto).
static uint8_t gMagnitud = 0;
// Los escalones del Anil: 4 (5%), 5 (10%), 6 (20%), 7 (30%), 8 (20%),
// 9 (10%), 10 (5%).
static uint8_t magnitudAlAzar() {
  static const uint8_t PESO[] = { 5, 10, 20, 30, 20, 10, 5 };
  uint8_t t = (uint8_t)btRand(100), i = 0, acum = 0;
  for (; i < 6; i++) { acum = (uint8_t)(acum + PESO[i]); if (t < acum) break; }
  return (uint8_t)(4 + i);
}
// TRIPLE KICK / TRIPLE HIT: o numero do impacto (1, 2, 3). O Anil soma a
// potencia base a cada um (@calcBaseDmg += baseDmg): 10, 20, 30.
static uint8_t gImpacto = 0;

// Los de apoyo que el SUSTITUTO no para. Base del Anil: los de sonido y
// Infiltracion (Battle::Move#ignoresSubstitute?); y encima las clases que lo
// declaran a mano y que aqui existen. Play Nice no tiene efecto propio.
static bool atraviesaMuneco(uint16_t mv, const Combatant &att) {
  if (mv >= MOVE_COUNT) return false;
  if (moveHas(mv, MV_SOUND) || att.abil() == AB_INFILTRATOR) return true;
  switch (MOVE_TBL[mv].fx) {
    case FX_ENCORE: case FX_TAUNT: case FX_DISABLE: case FX_TORMENT:
    case FX_ATTRACT: case FX_FORESIGHT: case FX_MIRACLE_EYE: case FX_ROAR:
    case FX_SPITE: case FX_SKETCH:
      return true;
    default:
      return strcmp(MOVE_TBL[mv].name, "PLAYNICE") == 0;
  }
}

static inline bool esBayaDeCura(const ItemEntry &it) {
  return it.fx == IT_BERRY_HEAL || it.fx == IT_BERRY_FIX;
}
static inline uint16_t curaDeBaya(const ItemEntry &it, uint16_t hpMax) {
  if (it.fx == IT_BERRY_FIX) return it.arg;           // PS fijos
  if (it.fx != IT_BERRY_HEAL) return 0;
  return (uint16_t)((uint32_t)hpMax * it.arg / 100);  // una fraccion
}

// El stat ya calculado y SIN escalones. Un transformado usa el COPIADO:
// Transform se lleva los stats del otro, no solo su especie.
uint16_t Combatant::statBruto(uint8_t cual) const {
  if (!valid() || cual < 1 || cual > 5) return 1;
  if (transformed && copiado[cual - 1]) return copiado[cual - 1];
  switch (cual) {
    case 1:  return statAtLevel(base(1), geneAtk, level, trAtk);
    case 2:  return statAtLevel(base(2), geneDef, level, trDef);
    case 3:  return statAtLevel(base(3), geneSpa, level, trSpa);
    case 4:  return statAtLevel(base(4), geneSpd, level, trSpd);
    default: return statAtLevel(base(5), geneSpe, level, trSpe);
  }
}

uint16_t Combatant::atkStatRaw() const {
  if (!valid()) return 1;
  return conStage(statBruto(1), stage[SG_ATK]);
}

uint16_t Combatant::atkStat() const {
  uint16_t v = atkStatRaw();
  // la quemadura parte por la mitad el ataque FISICO, como en los juegos.
  // La FORMULA de dano no usa esta version, sino atkStatRaw() mas el
  // recorte aplicado aparte: ahi hay que saber de QUIEN es la quemadura,
  // y Juego Sucio pega con el ataque del otro.
  if (status == ST_BURN) v = v / 2 ? v / 2 : 1;
  return v;
}
uint16_t Combatant::defStat() const {
  return valid() ? conStage(statBruto(2), stage[SG_DEF]) : 1;
}
uint16_t Combatant::spAtkStat() const {
  return valid() ? conStage(statBruto(3), stage[SG_SPA]) : 1;
}
uint16_t Combatant::spDefStat() const {
  return valid() ? conStage(statBruto(4), stage[SG_SPD]) : 1;
}
uint16_t Combatant::speStat() const {
  if (!valid()) return 1;
  uint16_t v = conStage(statBruto(5), stage[SG_SPE]);
  // PIES RAPIDOS: estar mal le CORRE en vez de frenarle, y la
  // paralisis deja de recortarle. Va aqui y no en movesFirst porque
  // el corte de la paralisis vive aqui: fuera, se aplicarian los dos.
  if (abil() == AB_QUICK_FEET) {
    if (status != ST_NONE) v = v * 3 / 2;
  } else if (status == ST_PARALYSIS) {
    v = v / 2 ? v / 2 : 1;
  }
  return v;
}

// La mega que ESE lado puede hacer ahora, o -1. Pide tres cosas: que no
// haya megaevolucionado ya en este combate, que lleve una piedra puesta,
// y que esa piedra sea la SUYA (la de Charizard X no sirve para la Y, y
// la de otro bicho no sirve de nada).
int16_t Battle::canMega(uint8_t quien) const {
  if (quien > 1 || megaUsed[quien]) return -1;
  const Combatant &c = side[quien];
  if (!c.valid() || c.fainted() || c.isMega()) return -1;
  const ItemEntry &it = itemAt(c.item);
  if (it.fx != IT_MEGA) return -1;
  // Con la FORMA: un Toxtricity Grave y uno normal piden la misma piedra
  // y dan megas distintas.
  return megaFormOf(c.dex, it.arg, c.form);
}

bool Battle::doMega(uint8_t quien) {
  int16_t f = canMega(quien);
  if (f < 0 || !side[quien].megaEvolve((uint8_t)f)) return false;
  megaUsed[quien] = true;
  megaFired = f;
  // La mega trae HABILIDAD NUEVA, y en los juegos entra funcionando: el
  // Mega Charizard Y saca el sol en el acto. Aqui megaEvolve solo cambiaba
  // el campo `ability` y nadie disparaba nada —— por eso el Velo Vapor del
  // Mega Arcanine no se veia hacer nada.
  habilidadDeEntrada(quien);
  return true;
}

// Megaevoluciona. Devuelve false si no habia nada que hacer.
//
// Los PS: el Anil cambia el bHp en varias megas (entre las suyas hay
// versiones Dynamax), asi que el maximo se mueve. La vida actual se
// escala con el para conservar la FRACCION — si no, megaevolucionar
// seria curarse gratis cuando el maximo sube, y caer en el acto cuando
// baja.
bool Combatant::megaEvolve(uint8_t fila) {
  if (isMega() || fila >= MEGA_COUNT || !valid()) return false;
  if (MEGA_TBL[fila].base != dex) return false;
  if (MEGA_TBL[fila].fromForm != form &&
      MEGA_TBL[fila].fromForm != MEGA_ANY_FORM) return false;
  uint16_t antes = hpMax ? hpMax : 1;
  megaIdx = fila;
  ability = MEGA_TBL[fila].ability;
  hpMax = hpAtLevelBase(base(0), level, trHp);
  hp = (uint16_t)(((uint32_t)hp * hpMax + antes - 1) / antes);   // redondea arriba
  if (hp > hpMax) hp = hpMax;
  if (!hp) hp = 1;            // la mega no mata a quien la usa
  return true;
}

// Inmunidades por tipo: el fuego no se quema, el veneno y el acero no se
// envenenan, el electrico no se paraliza, el hielo no se congela. Sin esto un
// Charizard quemandose con su propio Flare Blitz seria raro de ver.
static bool inmuneA(uint8_t st, uint8_t t1, uint8_t t2) {
  switch (st) {
    case ST_BURN:      return t1 == T_FIRE || t2 == T_FIRE;
    case ST_POISON:
    case ST_BADPOISON: return t1 == T_POISON || t2 == T_POISON ||
                              t1 == T_STEEL || t2 == T_STEEL;
    case ST_PARALYSIS: return t1 == T_ELEC || t2 == T_ELEC;
    case ST_FREEZE:    return t1 == T_ICE || t2 == T_ICE;
    default:           return false;
  }
}

bool Combatant::applyStatus(uint8_t st, uint8_t terreno, uint8_t cielo,
                            bool ruido, bool corrosivo) {
  if (st == ST_NONE || status != ST_NONE) return false;  // ya tiene uno
  // ALBOROTO: con ese ruido no hay quien duerma. Vale para los dos lados,
  // el del que grita incluido (Descanso tampoco le funciona).
  // VELO DULCE: no le duermen, y la ficha dice que tampoco puede
  // dormirse el (Descanso no le funciona), asi que no mira de donde
  // viene el sueno.
  // LETARGO PERENNE: duerme siempre, asi que ningun estado nuevo le
  // entra. Aqui eso es toda la habilidad: lo de "ataca dormido" le
  // sale gratis, porque el sueno que le bloquea es el de `status`.
  // TERMOCAMBIO: no se quema. Es la mitad defensiva de la habilidad.
  if (abil() == AB_THERMAL && st == ST_BURN) return false;
  if (abil() == AB_COMATOSE) return false;
  if (abil() == AB_SWEET_VEIL && st == ST_SLEEP) return false;
  if (ruido && st == ST_SLEEP) return false;
  // Safeguard: cinco turnos sin que le entre nada. Es la razon de gastar el
  // turno en ponerlo frente a un rival que va de Toxic y Thunder Wave.
  if (safeguard) return false;
  // LEAF GUARD: con SU cielo (el sol) no le entra ningun estado. El
  // cielo llega como parametro por el mismo motivo que el suelo: lo
  // sabe el combate, no el combatiente. Quien no lo sepa pasa
  // WE_NONE y la habilidad no estorba.
  if (abil() == AB_GUARD_WEATHER && cielo == abilArg()) return false;
  // El suelo, si lo esta pisando: la niebla no deja que entre NADA y el
  // campo electrico no deja dormir. Lo de "si lo esta pisando" es la gracia:
  // contra un volador el terreno no vale de escudo.
  if (grounded()) {
    if (terreno == TR_MISTY) return false;
    if (terreno == TR_ELEC && st == ST_SLEEP) return false;
  }
  // Inmunidad por habilidad (Limber, Insomnia, Immunity...). El toxico cuenta
  // como veneno: quien no se envenena, no se envenena de ninguna manera.
  if (abil() == AB_IMMUNE_STATUS) {
    uint8_t no = abilArg();
    if (st == no || (no == ST_POISON && st == ST_BADPOISON)) return false;
  }
  // CORROSION del que ATACA se salta la inmunidad por tipo del
  // veneno. Llega por parametro, como el suelo y el cielo: la
  // habilidad es del otro, no de este.
  bool venenoso = (st == ST_POISON || st == ST_BADPOISON);
  if (!(corrosivo && venenoso) && inmuneA(st, type1(), type2()))
    return false;
  status = st;
  statusTurns = (st == ST_SLEEP) ? (uint8_t)(1 + btRand(3)) : 0;  // 1..3 turnos
  return true;
}

bool Combatant::applyStage(uint8_t which, int8_t delta, bool deFuera) {
  // CONTRARIO: lo que le suben le baja y lo que le bajan le sube.
  // Va lo PRIMERO de esta funcion porque todo cambio de stat pasa
  // por aqui — el suyo, el del rival, el de un objeto y el de una
  // trampa. Ponerlo en los sitios que llaman seria olvidarse de uno.
  if (abil() == AB_CONTRARY) delta = (int8_t)-delta;
  // NIEBLA: mientras dure, no le bajan nada. Solo vale contra lo que
  // viene de FUERA — lo que uno se baja a si mismo (Overheat, Close
  // Combat) se paga igual, que es lo que lo separa de ser gratis.
  if (delta < 0 && deFuera && mistTurns) return false;
  if (which >= SG_COUNT) return false;
  // Habilidades que no dejan que le BAJEN los stats. Subirlos siempre se
  // puede: Clear Body no impide que su dueno use Swords Dance.
  //
  // Y solo contra lo que viene de FUERA, igual que la Niebla de arriba:
  // el precio de Sofoco, Meteoro Draco o Brazo Martillo se paga aunque
  // tengas Cuerpo Puro. Aqui faltaba el `deFuera` y por eso esos golpes
  // salian gratis —— 130 de potencia sin contrapartida, cada turno.
  if (delta < 0 && deFuera) {
    switch (abil()) {
      case AB_CLEAR_BODY:    return false;
      case AB_NO_LOWER_ATK:  if (which == SG_ATK) return false; break;
      case AB_NO_LOWER_DEF:  if (which == SG_DEF) return false; break;
      case AB_KEEN_EYE:      if (which == SG_ACC) return false; break;
      default: break;
    }
  }
  int v = stage[which] + delta;
  if (v > 6) v = 6;
  if (v < -6) v = -6;
  if (v == stage[which]) return false;   // ya estaba al tope
  stage[which] = (int8_t)v;
  return true;
}

uint8_t Combatant::peekBlock() {
  if (blockCache == 0xFF) blockCache = blockedThisTurn();
  return blockCache;
}

uint8_t Combatant::blockedThisTurn() {
  // La recarga va la PRIMERA: Hyper Beam te deja tirado pase lo que pase.
  if (recharging) { recharging = false; return BLK_RECHARGE; }
  // Plantarse lo decidio giveOrder() antes de anunciar nada; aqui solo se
  // consume, como el flinch.
  if (refusing) { refusing = false; return BLK_DISOBEY; }
  if (flinched) { flinched = false; return BLK_FLINCH; }
  if (status == ST_SLEEP) {
    // MADRUGADOR: descuenta DOS por turno, asi que duerme la mitad.
    // Con un numero impar de turnos se le queda corto, que es lo que
    // dice su ficha del PBS.
    uint8_t paso = (abil() == AB_EARLY_BIRD) ? 2 : 1;
    if (statusTurns > 0) {
      statusTurns = (statusTurns > paso) ? (uint8_t)(statusTurns - paso) : 0;
      return BLK_SLEEP;
    }
    status = ST_NONE;                    // se despierta y ya puede moverse
  }
  if (status == ST_FREEZE) {
    if (btRand(5) != 0) return BLK_FREEZE;  // 20% de descongelarse
    status = ST_NONE;
  }
  // PEREZA: un turno si y otro no. El bit se enciende el turno que SI
  // se mueve, para que el siguiente le toque vago; asi el primer turno
  // que sale siempre actua, que es como se juega.
  // ATRACCION: enamorado, la mitad de los turnos se queda mirando. Va
  // con el resto de lo que corta el turno, y DESPUES de la paralisis
  // para que no se cuenten dos veces lo mismo.
  if (attracted && btRand(2) == 0) return BLK_ATTRACT;
  if (abil() == AB_TRUANT) {
    if (loafing) { loafing = false; return BLK_TRUANT; }
    loafing = true;
  }
  if (status == ST_PARALYSIS && btRand(4) == 0) return BLK_PARALYSIS;
  if (confuseTurns > 0) {
    // El Anil: pbConfusionDuration da 2..5, se descuenta ANTES de mirar y
    // al llegar a 0 se le pasa y ya actua, sin tirar el dado.
    confuseTurns--;
    if (confuseTurns && btRand(3) == 0) return BLK_CONFUSED;  // se golpea
  }
  return BLK_NONE;
}

uint16_t Combatant::endOfTurnDamage() {
  if (fainted()) return 0;
  uint16_t d = 0;
  // SALAZON: un octavo por turno, y un CUARTO si eres de agua o de acero
  // —— que es de lo que va el golpe: a esos dos les sienta fatal la sal.
  // Va antes que el estado principal porque se pueden tener los dos.
  if (saltCure) {
    uint8_t t1 = type1(), t2 = type2();
    bool sensible = t1 == T_WATER || t2 == T_WATER ||
                    t1 == T_STEEL || t2 == T_STEEL;
    uint16_t sal = (uint16_t)(hpMax / (sensible ? 4 : 8));
    d += sal ? sal : 1;
  }
  // Atrapado: roe un octavo por turno hasta que se suelta. Se cobra aparte del
  // estado principal porque se pueden tener los dos a la vez.
  if (bindTurns) {
    bindTurns--;
    uint16_t mordisco = hpMax / 8 ? hpMax / 8 : 1;
    d += mordisco;
  }
  if (status == ST_BURN || status == ST_POISON) {
    d += hpMax / 16;                     // un dieciseisavo por turno
  } else if (status == ST_BADPOISON) {
    // el toxico va a mas: 1/16, 2/16, 3/16...
    statusTurns = statusTurns < 15 ? (uint8_t)(statusTurns + 1) : 15;
    d += (uint16_t)((uint32_t)hpMax * statusTurns / 16);
  }
  if (!d && status != ST_NONE && status != ST_PARALYSIS &&
      status != ST_SLEEP && status != ST_FREEZE) d = 1;
  // MAGIC GUARD: solo le hace dano lo que le pegan de frente. El
  // veneno, la quemadura y el agarre dejan de contarle — el estado
  // sigue puesto (la paralisis le frena igual), pero no le roe.
  if (abil() == AB_MAGIC_GUARD) d = 0;
  if (d >= hp) d = hp;
  hp -= d;
  return d;
}

bool Combatant::anyUsable() const {
  for (uint8_t i = 0; i < nMoves; i++)
    if (usesLeft[i] > 0) return true;
  return false;
}

// Cual de los cinco stats de combate tiene mas alto AHORA (contando
// los stages). Lo piden Protosintesis, Carga Cuantica y Alma Fuerte:
// las tres dicen "su stat mas alto" y cual sea depende del bicho y de
// lo que se haya subido, asi que no cabe en el arg de la habilidad.
// El golpe de mas potencia que ese bicho sabe, o 0xFFFF si no sabe
// ninguno que pegue. Lo pide Prevision.
static uint16_t golpeMasFuerte(const Combatant &c) {
  uint16_t mejor = 0xFFFF, pot = 0;
  for (uint8_t i = 0; i < c.nMoves && i < BT_MOVES; i++) {
    uint16_t mv = c.moves[i];
    if (mv >= MOVE_COUNT || MOVE_TBL[mv].power == 0) continue;
    if (MOVE_TBL[mv].power > pot) { pot = MOVE_TBL[mv].power; mejor = mv; }
  }
  return mejor;
}

static uint8_t statMasAlto(const Combatant &c) {
  static const uint8_t CUAL[5] = { SG_ATK, SG_DEF, SG_SPA, SG_SPD, SG_SPE };
  uint16_t v[5] = { c.atkStat(), c.defStat(), c.spAtkStat(),
                    c.spDefStat(), c.speStat() };
  uint8_t mejor = 0;
  for (uint8_t k = 1; k < 5; k++) if (v[k] > v[mejor]) mejor = k;
  return CUAL[mejor];
}

// Gastar el deposito de Reserva devuelve las subidas que dio. Desde la
// 4a generacion, usar Escupir o Tragar quita la Defensa y la Def.Esp que
// Reserva habia puesto: esa es la mitad del precio del golpe.
//
// Se bajan tantos escalones como cargas habia, que es lo que Reserva
// subio. Van sin `deFuera`: se lo cobra el bicho a si mismo.
static void gastaReserva(Combatant &c) {
  uint8_t cargas = c.stockpile;
  c.stockpile = 0;
  while (cargas--) {
    c.applyStage(SG_DEF, -1);
    c.applyStage(SG_SPD, -1);
  }
}

// ¿ESTE SECUNDARIO SE LO APLICA EL QUE PEGA, A SI MISMO?
//
// Esto se escribia con los extremos del enum, y el comentario del sitio
// donde se usaba contaba lo que costo: al anadir cuatro valores al final
// se quedaron fuera del rango y Escamas Ruidosas le bajaba la defensa al
// RIVAL —— el precio del golpe convertido en premio, sin un solo aviso
// del compilador. Con ocho valores nuevos entrando, una lista que hay
// que escribir a mano es mas barata que un rango que falla callado.
static bool sxEsPropio(uint8_t sx) {
  switch (sx) {
    // el precio de los golpes gordos
    case SX_SELF_SPA2: case SX_SELF_DEFSPD: case SX_SELF_SPE:
    case SX_SELF_ATK:  case SX_SELF_DEF:    case SX_SELF_SPA:
    case SX_SELF_SPE2: case SX_SELF_DEFSPDSPE: case SX_SELF_SCALE:
    // las subidas, de uno y de dos escalones
    case SX_RAISE_ATK: case SX_RAISE_DEF: case SX_RAISE_SPA:
    case SX_RAISE_SPD: case SX_RAISE_SPE: case SX_RAISE_ACC:
    case SX_RAISE_EVA: case SX_RAISE_ALL:
    case SX_RAISE2_ATK: case SX_RAISE2_DEF: case SX_RAISE2_SPA:
    case SX_RAISE2_SPD: case SX_RAISE2_SPE: case SX_RAISE2_ACC:
    case SX_RAISE2_EVA:
    // las de dos y tres stats de golpe
    case SX_RAISE_ATKDEF: case SX_RAISE_ATKSPE: case SX_RAISE_ATKSPA:
    case SX_RAISE_SPASPD: case SX_RAISE_DEFSPD: case SX_RAISE_ATKACC:
    case SX_RAISE_ATKDEFACC:
    // y las que entraron con los codigos del PBS bien leidos
    case SX_RAISE_SPASPDSPE: case SX_RAISE3_SPA:
    case SX_RAISE2_SPASPDSPE: case SX_RAISE_SPASPD_CURA:
    case SX_RAISE3_ATK_KO:
      return true;
    default:
      return false;
  }
}

// ¿ESTE GOLPE TOCA AL RIVAL? Lo pregunta el Protect, que tiene que parar
// lo que va contra el bicho y dejar pasar lo que uno se hace a si mismo o
// al campo. Antes se usaba `power > 0`, y asi la mitad de lo que el escudo
// existe para parar lo atravesaba.
//
// No hay campo de objetivo en la tabla de golpes, asi que la lista se
// saco mirando los 183 golpes de potencia 0 agrupados por efecto (no los
// 237 efectos): los de estado y los de bajar stats son tramos seguidos
// del enum, y los de apoyo que van contra el rival son estos 18.
//
// NO estan, y es a proposito: las trampas de entrada (Puas, Rocas, Red
// Viscosa) tampoco las para el Protect en los juegos, porque no atacan al
// bicho sino al lado; y FX_PSYCHUP, FX_SKETCH, FX_COPYCAT y FX_SELF_KO
// estan mal por su cuenta —— FX_SELF_KO mete ocho golpes distintos en un
// solo case —— y se arreglan por separado.
static bool tocaAlRival(uint16_t mv) {
  if (mv >= MOVE_COUNT) return true;
  const MoveEntry &m = MOVE_TBL[mv];
  if (m.power > 0) return true;
  switch (m.fx) {
    case FX_ATTRACT: case FX_DISABLE: case FX_ENCORE: case FX_ENTRAIN:
    case FX_FORESIGHT: case FX_GASTRO: case FX_LEECHSEED:
    case FX_MINDREADER: case FX_PAINSPLIT: case FX_ROAR: case FX_SOAK:
    case FX_SPITE: case FX_TAUNT: case FX_TORMENT: case FX_TRANSFORM:
    case FX_TRAP: case FX_TRICK: case FX_WORRY_SEED:
    case FX_EMBARGO: case FX_HEALBLOCK:
      return true;
    default:
      break;
  }
  // Estados (quemar, dormir, paralizar...), amedrentar, y las bajadas de
  // stats de uno y de dos escalones: todo eso va contra el rival.
  uint8_t sx = m.sx;
  if (sx >= SX_BURN && sx <= SX_FLINCH) return true;
  if (sx >= SX_LOWER_ATK && sx <= SX_LOWER_EVA) return true;
  if (sx >= SX_LOWER2_ATK && sx <= SX_LOWER2_EVA) return true;
  if (sx == SX_LOWER_ATKSPA) return true;
  return false;
}

// unlosableItem? del Anil (163_Battle_Battler.rb:497): la piedra mega de
// ESE bicho no se pierde, ni megaevolucionado ni antes. Transformado si:
// la piedra era del original.
static bool objetoIntocable(const Combatant &c, uint8_t item) {
  if (item == IT_NONE || !c.valid() || c.transformed) return false;
  const ItemEntry &it = itemAt(item);
  if (it.fx != IT_MEGA) return false;
  if (c.isMega()) return MEGA_TBL[c.megaIdx].item == it.arg;
  return megaFormOf(c.dex, it.arg, c.form) >= 0;
}

// POTENCIA BRUTA (Sheer Force), como o Anil decide: `SHEERFORCE &&
// move.addlEffect > 0`, o EffectChance do PBS (MV_ADDL). Com ela o golpe
// pega 30% mais e perde TODO efeito adicional — inclusive a subida do
// proprio atacante (Ancient Power, Flame Charge) — e os efeitos de depois
// do golpe (pbEffectsAfterMove2/3: Life Orb, Berserk, Pickpocket, Wimp
// Out). A baixa do Overheat/Close Combat NAO e efeito adicional (vem em
// pbEffectAfterAllHits, sem EffectChance): nem ganha o +30% nem se perde.
// Antes era `sx != SX_NONE`: Overheat pegava 30% mais e o Ancient Power
// pegava 30% mais E subia tudo.
static bool fuerzaBruta(const Combatant &att, uint16_t mv) {
  return att.abil() == AB_SHEER_FORCE && mv < MOVE_COUNT && moveHas(mv, MV_ADDL);
}

HitResult computeHit(const Combatant &att, const Combatant &def, uint16_t mv,
                     uint8_t weather, uint8_t terrain, bool ultimo,
                     uint8_t sport, uint8_t fusionPrev, bool plasma) {
  HitResult r;
  if (mv >= MOVE_COUNT || !att.valid() || !def.valid()) return r;
  // COPIA, no referencia: hay dos golpes que no tienen un tipo fijo y se
  // lo cambian aqui. El tipo se mira en diecisiete sitios de esta funcion
  // (efectividad, STAB, clima, terreno, bayas, objetos...), asi que
  // arreglarlo en el sitio es la unica forma de que salga bien en todos.
  MoveEntry m = MOVE_TBL[mv];
  if (m.fx == FX_NATURALGIFT) {
    uint8_t t = 0, p = 0;
    if (gDonPot) { t = gDonTipo; p = gDonPot; }
    else if (!donNatural(att, t, p)) { t = T_NORMAL; p = 1; }
    m.type = t;
    m.power = p;
  } else if (m.fx == FX_BEATUP) {
    m.power = gPalizaPot ? gPalizaPot
                         : (uint8_t)(5 + DEX_TBL[att.asDex].bAtk / 10);
  } else if (m.fx == FX_MULTI_3_UP && gImpacto > 1) {
    m.power = (uint8_t)(m.power * gImpacto);
  }
  if (m.fx == FX_HIDDEN_POWER) {
    // PODER OCULTO: su tipo sale de los GENES del bicho, asi que dos
    // Poder Oculto nunca son el mismo golpe. Los 16 tipos y la cuenta
    // por bits son los del juego; normal se queda fuera porque es el
    // tipo que trae la tabla cuando no se mira esto.
    static const uint8_t TIPOS[16] = {
      T_FIGHT, T_FLYING, T_POISON, T_GROUND, T_ROCK, T_BUG, T_GHOST,
      T_STEEL, T_FIRE, T_WATER, T_GRASS, T_ELEC, T_PSYCHIC, T_ICE,
      T_DRAGON, T_DARK,
    };
    uint8_t n = (uint8_t)((att.geneHp & 1) | ((att.geneAtk & 1) << 1) |
                          ((att.geneDef & 1) << 2) | ((att.geneSpe & 1) << 3) |
                          ((att.geneSpa & 1) << 4) | ((att.geneSpd & 1) << 5));
    // n*15/63, no n*16/64. Lo segundo es n/4: reparte mal los 64 valores
    // entre los 16 tipos y le cambia el tipo a la mayoria de los genes.
    m.type = TIPOS[(n * 15 / 63) & 15];
  } else if (m.fx == FX_WEATHER_BALL) {
    // METEOROBOLA: con el cielo cerrado cambia de tipo Y pega el doble.
    // Es el golpe que mas premia haber gastado un turno en el clima.
    if (weather != WE_NONE) {
      m.type = weather == WE_SUN  ? T_FIRE
             : weather == WE_RAIN ? T_WATER
             : weather == WE_SAND ? T_ROCK
                                  : T_ICE;   // granizo
      m.power = (uint8_t)(m.power * 2 > 255 ? 255 : m.power * 2);
    }
  } else if (m.fx == FX_TYPE_MINE1) {
    // DANZA DESPERTAR: del tipo del que la baila. Es lo que la hace el
    // golpe de Oricorio y no un Hiperrayo con otro nombre.
    if (att.type1() != T_NONE) m.type = att.type1();
  } else if (m.fx == FX_TYPE_MINE2) {
    // GARROTE HIEDRA: del SEGUNDO tipo. Sin segundo tipo se queda como
    // viene, que es lo que hace en los juegos.
    if (att.type2() != T_NONE) m.type = att.type2();
  } else if (m.fx == FX_TYPE_TERRAIN) {
    // CAMPO DE PULSOS: del suelo que haya, y el doble de fuerte. Sin
    // terreno es un golpe normal de 50, que es justo su precio.
    if (terrain != TR_NONE && att.grounded()) {
      m.type = terrain == TR_ELEC  ? T_ELEC
             : terrain == TR_GRASS ? T_GRASS
             : terrain == TR_MISTY ? T_FAIRY
                                   : T_PSYCHIC;
      m.power = (uint8_t)(m.power * 2 > 255 ? 255 : m.power * 2);
    }
  }
  // ...y lo que dejo puesto Punos Plasma, que no es una habilidad pero
  // manda igual sobre el tipo.
  if (plasma && m.type == T_NORMAL) m.type = T_ELEC;
  // Las habilidades que le cambian el TIPO al golpe. Van aqui, con
  // Poder Oculto y Meteorobola, porque es el unico sitio donde el
  // arreglo vale para los diecisiete lugares que miran el tipo.
  if (att.abil() == AB_TYPE_SWAP && m.type == T_NORMAL) {
    m.type = att.abilArg();          // Piel Feerica y sus primas
    m.power = (uint8_t)(m.power * 12 / 10 > 255 ? 255 : m.power * 12 / 10);
  } else if (att.abil() == AB_NORMALIZE && m.fx != FX_HIDDEN_POWER &&
             m.fx != FX_WEATHER_BALL) {
    // NORMALIDAD: todo a normal. Los que se calculan su propio tipo
    // quedan fuera, que es lo que dice su ficha.
    m.type = T_NORMAL;
    m.power = (uint8_t)(m.power * 12 / 10 > 255 ? 255 : m.power * 12 / 10);
  } else if (att.abil() == AB_HALLOWED && moveHas(mv, MV_SOUND)) {
    // TINTINEO, del Anil: sus golpes de sonido suenan a psiquico.
    m.type = T_PSYCHIC;
    m.power = (uint8_t)(m.power * 12 / 10 > 255 ? 255 : m.power * 12 / 10);
  } else if (att.abil() == AB_HUNGER && m.type == T_NORMAL) {
    // MAS QUE MONO: alterna cada turno, y con el cambia el tipo de su
    // golpe. Saciado pega electrico, voraz siniestro.
    m.type = att.hungry ? T_DARK : T_ELEC;
  }
  // SE QUEMO SU PROPIO TIPO. Ultimo Fuego y Electrochoque se gastan el
  // tipo para pegar una vez muy fuerte: a partir de ahi, sus golpes de ese
  // tipo dejan de llevar STAB. El tipo del BICHO lo apaga type1()/type2()
  // mirando `tipoPerdido`; aqui no hay nada que hacer —— el comentario
  // esta para que nadie lo busque dos veces.

  // GEISER FOTON y TERA ESTALLIDO: pegan por donde MAS duela. Se decide
  // aqui, antes de elegir los stats, para que el resto de la funcion no
  // tenga que enterarse —— `m` es una copia justo para esto.
  if (m.fx == FX_BEST_CATEGORY || m.fx == FX_PHOTON)
    m.phys = att.atkStat() > att.spAtkStat() ? 1 : 0;

  r.struggled = (mv == MOVE_STRUGGLE);

  // Golpe de apoyo (Swords Dance, Toxic, Sunny Day...): no pega a nadie, asi
  // que ni tipos ni formula. Su razon de ser esta entera en fx/sx, y de eso
  // se encarga useMove(). Aqui solo hay que no inventarse un dano.
  if (m.power == 0) {
    r.noDamage = true;
    r.effect = 100;   // no es inmunidad: es que no hay golpe que resistir
    // El Anil solo mira la tabla de tipos en los golpes de DANO
    // (171_Battler_UseMoveSuccessChecks.rb:460): Rayo Confuso marea a un
    // Normal y Malicioso le baja la defensa a un Fantasma. La excepcion es
    // ParalyzeTargetIfNotTypeImmune, que en todo el PBS es solo Onda Trueno.
    // Toxico contra Acero no pasa por aqui: lo para applyStatus.
    if (tocaAlRival(mv)) {
      if (strcmp(m.name, "THUNDERWAVE") == 0 &&
          typeEffect(m.type, def.type1(), def.type2()) == 0) {
        r.effect = 0;
        return r;
      }
      // POLVOS Y ESPORAS: los de tipo PLANTA no los respiran (6a gen).
      // La bandera MV_POWDER llevaba en la tabla sin que nadie la leyera.
      if (moveHas(mv, MV_POWDER) &&
          (def.type1() == T_GRASS || def.type2() == T_GRASS)) {
        r.effect = 0;
        return r;
      }
    }
    return r;
  }

  // REGALO: el dado se tira ANTES de nada (pbOnStartUse) y es el del Anil:
  // 40% potencia 40, 30% 80, 10% 120 y 20% cura. Cuando cura NO es un golpe
  // de dano (pbDamagingMove? = false): ni tipos ni inmunidad, asi que
  // tambien le cura a un fantasma. La cura la hace useMove.
  if (m.fx == FX_PRESENT) {
    long t = btRand(100);
    if (t >= 80) {
      r.presentHeal = true;
      r.noDamage = true;
      r.effect = 100;
      return r;
    }
    m.power = (uint8_t)(t < 40 ? 40 : t < 70 ? 80 : 120);
  }

  // La habilidad del que RECIBE, contando Mold Breaker. Quien lo tenga
  // pega como si el otro no tuviera ninguna: ni Levitate, ni Sturdy,
  // ni Multiscale, ni Shell Armor.
  //
  // Solo vale para lo que decide si el golpe entra y cuanto hace. Las
  // de CONTACTO (Static, Rough Skin) no se saltan, porque esas no
  // paran el golpe: saltan despues, cuando ya te acercaste.
  // MICOFUERZA cuenta como Mold Breaker, pero SOLO en sus golpes de
  // apoyo: es la otra mitad de la habilidad (la primera, ir ultimo, va
  // en movesFirst).
  // Rayo Umbrio Lunar y Meteoimpacto hacen lo mismo que Rompemoldes, pero
  // por GOLPE y no por habilidad: se enchufan al mismo interruptor en vez
  // de inventar un segundo camino que habria que acordarse de mantener.
  const bool rompe = att.abil() == AB_MOLD_BREAKER ||
                     m.fx == FX_IGNORE_ABIL ||
                     // GEISER FOTON: su FunctionCode del PBS acaba en
                     // `IgnoreTargetAbility` y esa mitad se perdia en el
                     // mapa. Se engancha al mismo interruptor.
                     m.fx == FX_PHOTON ||
                     (att.abil() == AB_MYCELIUM && m.power == 0);
  const uint8_t defAb = rompe ? (uint8_t)AB_NONE : def.abil();

  // AIRE CLARO: mientras este en la pista, el cielo no cuenta para
  // nadie — pero NO se va del campo, asi que al salir vuelve a
  // contar. Se apaga aqui, lo primero, porque de aqui para abajo el
  // clima se mira en ocho sitios distintos.
  if (att.abil() == AB_CLOUD_NINE || defAb == AB_CLOUD_NINE)
    weather = WE_NONE;

  r.effect = typeEffect(m.type, def.type1(), def.type2());
  // LIOFILIZACION: el agua le es superefectiva POR TIPO (pbCalcTypeModSingle),
  // asi que un Agua/Tierra recibe x4 y no x2 a medias.
  if (m.fx == FX_FREEZEDRY) {
    uint16_t e1 = def.type1() == T_WATER ? 200 : typeEffect(m.type, def.type1(), T_NONE);
    uint16_t e2 = def.type2() == T_NONE ? 100
                : def.type2() == T_WATER ? 200 : typeEffect(m.type, def.type2(), T_NONE);
    r.effect = (uint16_t)((uint32_t)e1 * e2 / 100);
  }
  // ANTIAEREO (Smack Down): al que han bajado al suelo le entra la tierra,
  // sea volador o levite. `grounded2` ya lo decia y `grounded()` lo leia,
  // pero la inmunidad del volador a tierra sale de la TABLA DE TIPOS, que
  // no miraba nada de esto: el golpe no servia para nada.
  const bool antiaereo = (m.type == T_GROUND && def.grounded2);
  if (antiaereo && r.effect == 0)
    r.effect = typeEffect(T_GROUND,
                          def.type1() == T_FLYING ? T_NONE : def.type1(),
                          def.type2() == T_FLYING ? T_NONE : def.type2());
  // PLANCHA VOLADORA: golpea como si fuera lucha Y volador a la vez, asi
  // que su eficacia es el producto de las dos. Es lo unico que la hace
  // distinta de una patada de lucha normal.
  if (m.fx == FX_ALSO_FLYING)
    r.effect = (uint16_t)((uint32_t)r.effect *
                          typeEffect(T_FLYING, def.type1(), def.type2()) / 100);
  // ATRAPAMOSCAS (del Anil): muy eficaz contra bicho pase lo que pase,
  // por encima de la tabla de tipos. Se multiplica sobre lo que ya habia
  // —— no se sustituye ——, para que un bicho/acero siga notando su acero.
  if (m.fx == FX_SUPER_VS_BUG &&
      (def.type1() == T_BUG || def.type2() == T_BUG))
    r.effect = (uint16_t)(r.effect * 2);
  // SCRAPPY: normal y lucha SI le entran a un fantasma. Es la unica
  // forma de que un bicho sin golpes de otro tipo no se quede mirando.
  if (r.effect == 0 && att.abil() == AB_SCRAPPY &&
      (m.type == T_NORMAL || m.type == T_FIGHT) &&
      (def.type1() == T_GHOST || def.type2() == T_GHOST))
    r.effect = typeEffect(m.type,
                          def.type1() == T_GHOST ? T_NONE : def.type1(),
                          def.type2() == T_GHOST ? T_NONE : def.type2());
  // OJO DE MENTE: al marcado, lo psiquico le ENTRA aunque sea
  // siniestro. Es la misma forma que Rastreo de abajo, con el tipo
  // cambiado, y por eso son dos banderas y no una.
  if (r.effect == 0 && def.mirado && m.type == T_PSYCHIC &&
      (def.type1() == T_DARK || def.type2() == T_DARK))
    r.effect = typeEffect(m.type,
                          def.type1() == T_DARK ? T_NONE : def.type1(),
                          def.type2() == T_DARK ? T_NONE : def.type2());
  // RASTREO: a quien le han rastreado deja de ser intocable para normal y
  // lucha, igual que con Scrappy. `foreseen` se escribia y no lo leia
  // nadie: el golpe no hacia ni esta mitad ni la otra.
  if (r.effect == 0 && def.foreseen &&
      (m.type == T_NORMAL || m.type == T_FIGHT) &&
      (def.type1() == T_GHOST || def.type2() == T_GHOST))
    r.effect = typeEffect(m.type,
                          def.type1() == T_GHOST ? T_NONE : def.type1(),
                          def.type2() == T_GHOST ? T_NONE : def.type2());
  // Inmunidades y absorciones por habilidad: Levitate contra tierra, Flash
  // Fire contra fuego, Water Absorb contra agua. Se marca aqui como inmune y
  // useMove() se encarga de la parte buena (curarse, subir un stat).
  switch (defAb) {
    case AB_IMMUNE_TYPE:
    case AB_ABSORB_HEAL:
    case AB_ABSORB_BOOST:
    case AB_ABSORB_SPEED:
    case AB_ABSORB_ATK:
    case AB_STEAM_VEIL:
      // El Antiaereo tambien anula el Leviton: si lo han bajado al suelo,
      // flotar ya no le salva de la tierra.
      if (m.type == def.abilArg() && !antiaereo) r.effect = 0;
      break;
    default:
      break;
  }
  // AISLANTE y ANTIBALAS: la inmunidad no es por TIPO sino por la
  // bandera del golpe, asi que no cabe en el switch de arriba. Lo
  // de "salvo los que use el propio Pokemon" que dice la ficha del
  // Aislante sale gratis: aqui solo se mira al que RECIBE.
  // ESCUDO PRODIGIOSO: solo le hace dano lo que le va MUY bien. Es la
  // habilidad mas extrema del juego y cabe en una linea porque la
  // efectividad ya esta calculada aqui arriba.
  if (defAb == AB_WONDER_GUARD && m.power > 0 && r.effect <= 100) {
    r.effect = 0;
    return r;
  }
  // SURCAVIENTOS: los golpes de VIENTO no le hacen nada. Lo que le
  // crece con ellos se resuelve en useMove, como las absorciones.
  if (defAb == AB_WIND_RIDER && moveHas(mv, MV_WIND)) {
    r.effect = 0;
    return r;
  }
  if ((defAb == AB_SOUNDPROOF && moveHas(mv, MV_SOUND)) ||
      (defAb == AB_BULLETPROOF && moveHas(mv, MV_BOMB))) {
    r.effect = 0;
    return r;
  }
  if (r.effect == 0) return r;  // inmune: ni se tira precision

  // Golpes que se saltan la formula entera: el dano no depende de stats ni de
  // tipos, solo de que entre. La efectividad ya esta calculada arriba, asi que
  // una inmunidad los para igual.
  switch (m.fx) {
    case FX_OHKO:
      r.damage = def.hp;          // fulmina; la precision bajisima lo compensa
      return r;
    case FX_FIXED_LVL:
      r.damage = att.level ? att.level : 1;
      return r;
    case FX_FIXED_LVL_RND: {      // nivel x 0,5..1,5
      uint32_t d = (uint32_t)att.level * (50 + btRand(101)) / 100;
      r.damage = (uint16_t)(d ? d : 1);
      return r;
    }
    case FX_FIXED_40:
      r.damage = 40;
      return r;
    case FX_BIDE: {               // el doble de lo que aguanto
      uint32_t d = (uint32_t)att.bideDmg * 2;
      r.damage = (uint16_t)(d > 65535 ? 65535 : (d ? d : 1));
      return r;
    }
    case FX_FIXED_20:
      r.damage = 20;              // Bomba Sonica, y siempre 20
      return r;
    case FX_HALF_HP:
      // Superdiente: le deja a la mitad, los que sean. Va aqui y no con
      // los de potencia variable porque no pasa por la formula; y aqui
      // DENTRO, y no antes, para que un fantasma siga siendo inmune a el.
      r.damage = (uint16_t)(def.hp / 2 ? def.hp / 2 : 1);
      return r;
    default:
      break;
  }

  // Golpes que copian lo que les acaban de hacer. Van aqui porque no usan la
  // formula para nada: el dano ya esta decidido por el golpe del rival.
  if (m.fx == FX_COUNTER || m.fx == FX_MIRRORCOAT) {
    uint16_t recibido = (m.fx == FX_COUNTER) ? att.tookPhys : att.tookSpec;
    if (!recibido) { r.failed = true; return r; }
    uint32_t d = (uint32_t)recibido * 2;
    r.damage = (uint16_t)(d > 65535 ? 65535 : d);
    return r;
  }
  // REPRESION METAL: como Contraataque pero sin mirar de que tipo fue el
  // golpe, y devolviendo vez y media en vez del doble.
  if (m.fx == FX_METAL_BURST) {
    uint32_t recibido = (uint32_t)att.tookPhys + att.tookSpec;
    if (!recibido) { r.failed = true; return r; }
    uint32_t d = recibido * 3 / 2;
    r.damage = (uint16_t)(d > 65535 ? 65535 : (d ? d : 1));
    return r;
  }
  // POLTERGEIST: le pega con lo que lleva encima, asi que sin objeto no
  // hay golpe. Ahora que los salvajes llevan cosas, esto significa algo.
  if (m.fx == FX_POLTERGEIST && def.item == IT_NONE) {
    r.failed = true;
    return r;
  }
  // SINCRORRUIDO: solo le entra a quien comparte tipo con el que pega.
  if (m.fx == FX_SAME_TYPE &&
      !(def.type1() == att.type1() ||
        (att.type2() && def.type1() == att.type2()) ||
        (def.type2() && (def.type2() == att.type1() ||
                         (att.type2() && def.type2() == att.type2()))))) {
    r.failed = true;
    return r;
  }
  // ESCARAMUZA: solo el primer turno que sale a la pista. Es un golpe de
  // entrada, igual que Primer Impacto.
  if (m.fx == FX_FIRST_TURN && att.turnsOut > 0) {
    r.failed = true;
    return r;
  }
  if (m.fx == FX_ENDEAVOR) {
    // le deja los mismos PS que tenga el que pega: inutil si vas por delante
    if (def.hp <= att.hp) { r.failed = true; return r; }
    r.damage = (uint16_t)(def.hp - att.hp);
    return r;
  }

  // 1 de cada 16, como en los juegos; algunos golpes lo garantizan. Focus
  // Energy sube el escalon en vez de un stat, que es lo que le da sentido:
  // no pega mas fuerte, pega critico mas a menudo.
  int8_t ce = att.critStage;
  if (m.fx == FX_ALWAYS_CRIT) ce += 3;
  if (att.abil() == AB_CRIT_UP) ce++;        // Super Suerte
  if (objeto(att).fx == IT_SCOPE_LENS) ce++;
  uint8_t unoEntre = ce >= 3 ? 1 : ce == 2 ? 2 : ce == 1 ? 8 : 16;
  r.crit = att.sureCrit || unoEntre == 1 || (btRand(unoEntre) == 0);
  if (defAb == AB_NO_CRIT) r.crit = false;   // Shell Armor
  // CONJURO: al bando protegido no le entran criticos. Llega en el
  // combatiente (`chanted`) porque computeHit no sabe de bandos.
  if (def.chanted) r.crit = false;

  // Psyshock y Psystrike son especiales pero muerden la defensa FISICA.
  // Body Press ademas pega con la DEFENSA del que golpea, no con su ataque.
  bool contraDefFisica = m.phys || (m.fx == FX_VS_DEF);
  // JUEGO SUCIO pega con el ataque DEL RIVAL: cuanto mas fuerte sea el
  // otro, mas duele. Por eso lo lleva gente que no pega nada.
  uint16_t A = (m.fx == FX_FOUL_PLAY) ? def.atkStatRaw()
             : (m.fx == FX_BODYPRESS) ? att.defStat()
             : m.phys ? att.atkStatRaw() : att.spAtkStat();
  uint16_t D = contraDefFisica ? def.defStat() : def.spDefStat();

  // ESPADA SANTA, GUARDIA BAJA y cia: pegan como si el rival no se
  // hubiera subido nada. Se rehace la defensa sin su stage, igual que
  // Unaware; la evasion que tambien ignoran se mira en la precision.
  if (m.fx == FX_IGNORE_STAGES) {
    int8_t st = def.stage[contraDefFisica ? SG_DEF : SG_SPD];
    // Sin signo: Espada Santa ignora los cambios de defensa del alvo para
    // ARRIBA y para ABAJO. Con el `st > 0` de antes, un alvo con Chirrido
    // encima (-2 Def) hacia que el golpe pegara MAS que en los juegos.
    if (st) D = (uint16_t)((uint32_t)D * 100 / (stageMul(st) ? stageMul(st) : 100));
  }

  // UNAWARE: pega como si el otro no se hubiera subido nada. Se rehace
  // la defensa sin su stage, que es justo lo que la habilidad dice.
  // No vale al reves (no se ignora lo que uno mismo se ha subido): eso
  // seria castigarse por tenerla.
  if (att.abil() == AB_UNAWARE) {
    int8_t st = def.stage[contraDefFisica ? SG_DEF : SG_SPD];
    if (st) D = (uint16_t)((uint32_t)D * 100 / (stageMul(st) ? stageMul(st) : 100));
  }

  // Habilidades sobre los stats brutos, antes de meterlos en la formula.
  if (att.abil() == AB_HUGE_POWER && m.phys) A = (uint16_t)(A * 2);
  // LA QUEMADURA, aqui y no dentro de atkStat(): es la del que PEGA, vale
  // para todo golpe FISICO —— tambien Plancha Corporal, que pega con su
  // defensa y antes se libraba —— y NO es la del rival, que es lo que se
  // colaba en Juego Sucio.
  //
  // Agallas se la salta entera. Antes esto cogia el valor ya partido por
  // la mitad y lo multiplicaba por 1,5: 0,75x, o sea lo contrario de lo
  // que dice su propia linea.
  if (m.phys && att.status == ST_BURN && att.abil() != AB_GUTS)
    A = A / 2 ? A / 2 : 1;
  // Guts: estar mal de estado deja de ser solo malo.
  if (att.abil() == AB_GUTS && att.status != ST_NONE && m.phys) A = A * 3 / 2;
  // Hustle: pega mas fuerte y apunta peor. Lo segundo esta en la
  // precision, mas abajo.
  if (att.abil() == AB_HUSTLE && m.phys) A = A * 3 / 2;
  // Acometida (del Anil): el PRIMER turno que sale, +20% de ataque.
  // El +50% de velocidad va en movesFirst, que es donde se mira.
  if (att.abil() == AB_BLITZ && att.turnsOut == 0 && m.phys) A = A * 12 / 10;
  if (defAb == AB_MARVELSCALE && def.status != ST_NONE && contraDefFisica)
    D = D * 3 / 2;
  // PARADOJA (Protosintesis con sol, Carga Cuantica con campo
  // electrico): le crece su stat MAS ALTO. Cual es depende del bicho,
  // asi que no cabe en el arg: se calcula.
  {
    uint8_t fa = att.abil(), fd = defAb;
    // Pulso Oricalco y Motor Hadron son Protosintesis y Carga Cuantica
    // que ADEMAS se ponen su propio clima o suelo al entrar, asi que
    // aqui cuentan igual (el clima lo pone alEntrar).
    bool aOn = (fa == AB_PARADOX && weather == att.abilArg()) ||
               (fa == AB_ORICHALCUM && weather == WE_SUN) ||
               ((fa == AB_PARADOX_TR && terrain == att.abilArg() &&
                 att.grounded())) ||
               (fa == AB_HADRON && terrain == TR_ELEC && att.grounded());
    bool dOn = (fd == AB_PARADOX && weather == def.abilArg()) ||
               (fd == AB_ORICHALCUM && weather == WE_SUN) ||
               ((fd == AB_PARADOX_TR && terrain == def.abilArg() &&
                 def.grounded())) ||
               (fd == AB_HADRON && terrain == TR_ELEC && def.grounded());
    if (aOn && statMasAlto(att) == (m.phys ? SG_ATK : SG_SPA))
      A = (uint16_t)((uint32_t)A * 13 / 10);
    if (dOn && statMasAlto(def) == (contraDefFisica ? SG_DEF : SG_SPD))
      D = (uint16_t)((uint32_t)D * 13 / 10);
  }
  // TESOROS FUNESTOS: bajan un 25% ese stat de TODOS LOS DEMAS, asi
  // que el del rival mira MI stat y el mio mira el SUYO. Cruzado a
  // proposito: la habilidad no se aplica a quien la lleva.
  {
    uint8_t ruinA = (defAb == AB_RUIN) ? def.abilArg() : 0xFF;
    uint8_t ruinD = (att.abil() == AB_RUIN) ? att.abilArg() : 0xFF;
    if (ruinA == (m.phys ? SG_ATK : SG_SPA)) A = A * 3 / 4;
    if (ruinD == (contraDefFisica ? SG_DEF : SG_SPD)) D = D * 3 / 4;
  }
  // DON FLORAL: al sol, un 50% mas de ataque y de defensa especial.
  // En los juegos es para todo el equipo; aqui, sin compañero en la
  // pista, es para el. Se mira en los dos lados por lo mismo.
  if (att.abil() == AB_SUN_BOOST && weather == WE_SUN && m.phys)
    A = A * 3 / 2;
  if (defAb == AB_SUN_BOOST && weather == WE_SUN && !contraDefFisica)
    D = D * 3 / 2;
  // (Cambio Tactico, Banco, Escudo Limitado y Agrupamiento eran aqui un
  // x1,5 / x1,3 de mentira. Desde la leva 70 son la FORMA de verdad, con
  // sus stats del Anil: Aegislash Filo 150 de ataque, no 50 x 1,5.)
  // Lo que le dejo el companero que se retiro: un 30% a los
  // especiales (Bateria) o a todo (Punto Fuerte).
  if (att.charged == 2 || (att.charged == 1 && !m.phys))
    A = A * 13 / 10;
  // DE CERO A HEROE: una vez que se ha retirado, vuelve crecido.
  if (att.abil() == AB_HERO && att.hero) A = A * 3 / 2;
  // DERROTISTA: por debajo de media vida pega la mitad, del tipo que
  // sea. Es la unica habilidad que es puramente un defecto.
  if (att.abil() == AB_DEFEATIST && att.hpMax && att.hp * 2 <= att.hpMax)
    A /= 2;
  // INICIO LENTO: los cinco primeros turnos, a medio gas. La otra
  // mitad (la velocidad) va en movesFirst.
  if (att.abil() == AB_SLOW_START && att.turnsOut < 5 && m.phys) A /= 2;
  if (D == 0) D = 1;

  // Potencia variable segun la situacion. En estos golpes el PBS deja Power
  // en 1 porque el numero real se calcula aqui.
  uint16_t pow = m.power;
  switch (m.fx) {
    case FX_POW_LOW_HP: {         // Flail: casi muerto, casi letal
      uint32_t frac = att.hpMax ? (uint32_t)att.hp * 48 / att.hpMax : 48;
      // El primer escalon es frac == 0, no frac <= 1: la tabla oficial es
      // n<1, n<5, n<10, n<17, n<33. Los otros cuatro ya estaban bien, que
      // es lo que delata que este fue un desliz al copiarla.
      pow = frac == 0 ? 200 : frac <= 4 ? 150 : frac <= 9 ? 100
            : frac <= 16 ? 80 : frac <= 32 ? 40 : 20;
      break;
    }
    case FX_POW_SPEED: {          // Electro Ball
      uint16_t sd = def.speStat() ? def.speStat() : 1;
      uint32_t razon = (uint32_t)att.speStat() * 100 / sd;
      pow = razon >= 400 ? 150 : razon >= 300 ? 120 : razon >= 200 ? 80
            : razon >= 100 ? 60 : 40;
      break;
    }
    case FX_POW_SLOW: {           // Gyro Ball: al reves
      uint16_t sa = att.speStat() ? att.speStat() : 1;
      uint32_t p = 25 * def.speStat() / sa;
      pow = (uint16_t)(p > 150 ? 150 : (p < 1 ? 1 : p));
      break;
    }
    case FX_POW_STAGES: {         // Stored Power: 20 + 20 por cada subida
      int subidas = 0;
      for (uint8_t i = 0; i < SG_COUNT; i++)
        if (att.stage[i] > 0) subidas += att.stage[i];
      // 20 * (1 + subidas) y SIN tope, como en el Anil: con los siete
      // stats a +6 son 860. Aqui habia un tope de 250 puesto a mano.
      uint32_t p = 20 + 20 * (uint32_t)subidas;
      pow = (uint16_t)p;
      break;
    }
    case FX_POW_LESS_PP: {        // Trump Card: la ultima carta es la buena
      // Estaba declarado, puesto en TRUMPCARD y leido en NINGUN sitio.
      // El PBS deja Power en 1 para los golpes que se calculan aqui,
      // asi que Trump Card pegaba 1 SIEMPRE. No lo encontro ningun
      // test — no podia: no habia codigo que pudiera dar mal. Lo
      // encontro tools/cobertura.py al ver que nadie miraba su FX.
      //
      // computeHit recibe el GOLPE, no el hueco, asi que hay que
      // buscar cual de los cuatro es para saberle los usos.
      static const uint16_t POR_USOS[] = { 200, 80, 60, 50, 40 };
      uint8_t usos = 4;
      for (uint8_t i = 0; i < BT_MOVES; i++)
        if (att.moves[i] == mv) { usos = att.usesLeft[i]; break; }
      pow = POR_USOS[usos < 4 ? usos : 4];
      break;
    }
    // --- los que miran que paso ANTES en este turno -----------------
    // Ninguno de estos se podia hacer hasta que el combate supo quien
    // se habia movido ya (`ultimo`, que entro con Analytic).
    case FX_X2_IF_ACTED:        // Vendetta: si ya se movio, el doble
      if (ultimo) pow = (uint16_t)(pow * 2);
      break;
    case FX_AFTER_BOLT:         // Llama Fusion, tras un Rayo Fusion
      if (fusionPrev == 1) pow = (uint16_t)(pow * 2);
      break;
    case FX_AFTER_FLARE:        // Rayo Fusion, tras una Llama Fusion
      if (fusionPrev == 2) pow = (uint16_t)(pow * 2);
      break;
    case FX_X2_IF_NOT_ACTED:
      // Electropico y Fauces Fuertes: el doble si le pillas ANTES de que
      // se mueva. Son el espejo de Vendetta, y por eso premian ser rapido
      // en vez de aguantar.
      if (!ultimo) pow = (uint16_t)(pow * 2);
      break;
    case FX_X2_IF_PARA:
      // Estimulo: el doble al paralizado. Que ADEMAS le cure la paralisis
      // va en useMove, porque aqui no se puede tocar al rival.
      if (def.status == ST_PARALYSIS) pow = (uint16_t)(pow * 2);
      break;
    case FX_X133_IF_SUPER:
      // Justo Castigo y Electrotransmision: un tercio mas si ya era muy
      // eficaz. `r.effect` viene x100 y se calculo mas arriba, asi que
      // aqui ya se sabe.
      if (r.effect > 100) pow = (uint16_t)(pow * 4 / 3);
      break;
    case FX_X15_SUN:
      // Vapor Hidro: el unico golpe de agua al que el sol le VIENE BIEN.
      // El recorte del clima se aplica aparte y mas abajo; esto es el
      // aumento propio del golpe.
      if (weather == WE_SUN) pow = (uint16_t)(pow * 3 / 2);
      break;
    case FX_CHARGE:
      // RAYO SOLAR y CUCHILLA SOLAR: la mitad con cualquier cielo que no
      // sea el sol (TwoTurnAttackOneTurnInSun del Anil, en su
      // pbBaseDamageMultiplier: "Power halved in all weather except
      // sunshine"). Sin esto un Rayo Solar bajo la lluvia pegaba entero.
      if (m.type == T_GRASS && weather != WE_NONE && weather != WE_SUN)
        pow = (uint16_t)(pow / 2);
      break;
    case FX_X15_ELEC_TR:
      if (terrain == TR_ELEC && att.grounded()) pow = (uint16_t)(pow * 3 / 2);
      break;
    case FX_X15_PSY_TR:
      if (terrain == TR_PSY && att.grounded()) pow = (uint16_t)(pow * 3 / 2);
      break;
    case FX_RAGE_FIST: {
      // Punorrabia: +50 por cada golpe que te han metido en el combate.
      // Topado en 350 como en los juegos; sin tope, un combate largo lo
      // convertiria en un OHKO.
      uint32_t pr = 50u + 50u * att.hitsTaken;
      pow = (uint16_t)(pr > 350 ? 350 : pr);
      break;
    }
    case FX_LAST_RESPECTS: {
      // Ultimo Adios: +50 por cada companero caido. Es el golpe que
      // convierte ir perdiendo en una amenaza.
      uint32_t pr = 50u + 50u * att.alliesFallen;
      pow = (uint16_t)(pr > 500 ? 500 : pr);
      break;
    }
    case FX_FICKLE:
      // Rayo Umbrio: un 30% de las veces, el doble. En los juegos ademas
      // lo anuncia ("se esta tomando en serio"), y eso lo cuenta la
      // pantalla mirando r.crit... no: lleva su propio aviso, ver
      // r.fickleBig.
      if (btRand(100) < 30) {
        pow = (uint16_t)(pow * 2);
        r.fickleBig = true;
      }
      break;
    case FX_BY_BOND_INV:
      // Frustracion: el espejo de Retribucion. Cuanto MENOS te quiera,
      // mas pega. Con el vinculo al tope se queda en 1, que es justo la
      // gracia de tener los dos golpes en el juego.
      pow = (uint16_t)((100 - (att.bond > 100 ? 100 : att.bond)) * 102 / 100);
      if (!pow) pow = 1;
      break;
    case FX_PRESENT:
      // Regalo: la potencia ya salio del dado, arriba del todo (m.power).
      break;
    case FX_X2_IF_HURT:         // Buena Baza: si ya le pegaron, el doble
      if (def.tookPhys || def.tookSpec) pow = (uint16_t)(pow * 2);
      break;
    case FX_X2_IF_HALF:         // Salmuera: al rival tocado, el doble
      if (def.hpMax && def.hp * 2 <= def.hpMax) pow = (uint16_t)(pow * 2);
      break;
    case FX_RETALIATE:
      // Represalia: el doble si acaba de caer uno de los tuyos. Aqui
      // "los tuyos" es el equipo, que ya existe.
      if (att.vengeful) pow = (uint16_t)(pow * 2);
      break;
    case FX_BY_TARGET_STAGES: {
      // Castigo: cuanto mas se haya subido el rival, mas duele. Es la
      // respuesta a quien se pasa el combate bailando.
      int subidas = 0;
      for (uint8_t i = 0; i < SG_COUNT; i++)
        if (def.stage[i] > 0) subidas += def.stage[i];
      uint32_t p = 60 + 20 * (uint32_t)subidas;
      pow = (uint16_t)(p > 200 ? 200 : p);
      break;
    }
    case FX_SPIT_UP:
      // Escupir: 100 por cada carga. Sin deposito no hay nada que
      // escupir, y el golpe falla (lo mira useMove).
      pow = (uint16_t)(att.stockpile * 100);
      if (!pow) pow = 1;
      break;
    case FX_FINAL_GAMBIT:
      // SACRIFICIO: hace tanto dano como PS le queden al que lo usa.
      // Llevaba potencia 1 en la tabla y eso era literalmente lo que
      // hacia: 1 de dano y caer. El caer se cobra en useMove.
      pow = att.hp ? att.hp : 1;
      break;
    case FX_POW_HIGH_HP: {
      // Estallido y Salpicar: al reves que Flail. Pegan como un camion
      // enteros y como nada tocados, lo que los vuelve un golpe de
      // apertura y no de ultimo recurso.
      uint32_t p = att.hpMax ? (uint32_t)150 * att.hp / att.hpMax : 150;
      pow = (uint16_t)(p ? p : 1);
      break;
    }
    case FX_TANTRUM:            // Pataleta: el doble si el anterior fallo
      if (att.lastRoundFailed) pow = (uint16_t)(pow * 2);
      break;
    case FX_X2_IF_UNDER:        // Terremoto al que esta cavando
      if (def.hiding == HID_UNDER) pow = (uint16_t)(pow * 2);
      break;
    case FX_X2_IF_SKY:          // Tornado al que esta volando
      if (def.hiding == HID_SKY) pow = (uint16_t)(pow * 2);
      break;
    case FX_X2_IF_WATER:        // Surf al que esta buceando
      if (def.hiding == HID_WATER) pow = (uint16_t)(pow * 2);
      break;
    case FX_BIND:               // Torbellino tambien: agarra Y pega el doble
      if (def.hiding == HID_WATER && m.type == T_WATER) pow = (uint16_t)(pow * 2);
      break;
    case FX_MAGNITUDE: {
      // Magnitud: el PBS deja la potencia en 1 porque el numero sale de
      // un dado. Los escalones y sus probabilidades son los del juego
      // (del 4 al 10), y al que esta cavando le llega el doble.
      static const uint8_t POT[]  = { 10, 30, 50, 70, 90, 110, 150 };
      const uint8_t n = (gMagnitud >= 4 && gMagnitud <= 10) ? gMagnitud : magnitudAlAzar();
      pow = POT[n - 4];
      if (def.hiding == HID_UNDER) pow = (uint16_t)(pow * 2);
      break;
    }
    case FX_X2_IF_POISON:       // Carga Toxica: al envenenado, el doble
      if (def.status == ST_POISON || def.status == ST_BADPOISON)
        pow = (uint16_t)(pow * 2);
      break;
    case FX_X2_IF_I_HURT:       // Desquite: duele mas si acabas de cobrar
      if (att.tookPhys || att.tookSpec) pow = (uint16_t)(pow * 2);
      break;
    case FX_FACADE:
      // Fachada: el doble si el que pega esta mal. Y OJO: con quemadura
      // vale doble de verdad, porque el recorte de ataque de la quemadura
      // se aplica luego sobre un numero ya duplicado.
      if (att.status == ST_POISON || att.status == ST_BADPOISON ||
          att.status == ST_BURN || att.status == ST_PARALYSIS)
        pow = (uint16_t)(pow * 2);
      break;
    case FX_WAKE_SLAP:          // Espabila: al dormido, el doble
      if (def.status == ST_SLEEP) pow = (uint16_t)(pow * 2);
      break;
    case FX_X2_ELEC_TERR:
      // Alto Voltaje: el doble con campo electrico, y segun quien PISA el
      // suelo —— pero el que importa es el ALVO, no el que pega: la
      // corriente le sube a EL por los pies. Aqui se miraba `att`, asi
      // que contra un volador doblaba igual y contra un alvo en el suelo
      // no doblaba si el atacante volaba.
      if (terrain == TR_ELEC && def.grounded()) pow = (uint16_t)(pow * 2);
      break;
    case FX_BY_TARGET_HP: {
      // Estrujon: cuanto mas entero este el rival, mas aprieta. 120 a
      // tope, y nunca menos de 1.
      uint32_t p = def.hpMax ? (uint32_t)120 * def.hp / def.hpMax : 120;
      pow = (uint16_t)(p ? p : 1);
      break;
    }
    case FX_PURSUIT:
      // Persecucion: el doble al que se va. Aqui el turno no se parte en
      // dos (el relevo YA se resolvio y el que entra es el que recibe el
      // golpe gratis), asi que no pego al que huye sino al que asoma. El
      // precio del cambio voluntario es el mismo, que es de lo que vive
      // el golpe; lo que no hace es dejar al que se iba a medio salir.
      if (def.relieved) pow = (uint16_t)(pow * 2);
      break;
    case FX_ECHO:
      // Eco Voz: crece mientras se repita, y la racha NO se pierde al
      // relevar (por eso va en el lado y no en el bicho).
      pow = (uint16_t)(pow * (1 + (att.echoRun > 4 ? 4 : att.echoRun)));
      break;
    // --- los que crecen --------------------------------------------
    case FX_RAMP: {
      // CORTE FURIA: dobla cada vez que lo repites, y el tope NO es
      // redondo —— el Anil dobla hasta el PRIMER valor que llega a 160 y
      // ahi se queda (`while (@power << (maxMult - 1)) < 160`). Con sus
      // 40 de base eso son 40-80-160 y se acabo: x4, no x16.
      //
      // Aqui estaba topado en x16 como Rodar y ademas recortado a 250,
      // asi que Corte Furia pegaba 250 donde el Anil pega 160.
      uint8_t tope = 0;
      while ((uint32_t)(m.power << tope) < 160 && tope < 8) tope++;
      uint8_t n = (att.rachaDe == mv) ? att.rachaMove : 0;
      if (n > tope) n = tope;
      pow = (uint16_t)(pow << n);
      break;
    }
    case FX_ROLLOUT: {
      // RODAR / BOLA HIELO: x1, x2, x4, x8, x16 y VUELTA A EMPEZAR. El
      // contador del Anil es el mismo que nos ata (5->0): mientras ate,
      // `shift = 5 - contador`; suelto, shift = 0.
      //
      // Antes se leia `rachaMove`, que no baja nunca: el quinto turno
      // dejaba el x16 clavado para el resto del combate.
      uint8_t n = (att.lockTurns > 0 && att.lockTurns <= 4)
                      ? (uint8_t)(5 - att.lockTurns) : 0;
      // Y enroscado dobla: es un escalon mas, no un x1,5.
      if (att.curled) n++;
      pow = (uint16_t)(pow << n);
      // Sin recorte a 250: en el Anil el quinto turno son 480 de
      // potencia (960 con Rizo Defensa), y ese numero es justo el
      // premio por haberse quedado cinco turnos sin elegir.
      break;
    }
    case FX_BY_BOND:
      // Retribucion: pega tanto como te quiera. 0..100 de vinculo se
      // reparten sobre 102 de potencia, como en los juegos.
      pow = (uint16_t)(att.bond * 102 / 100);
      if (!pow) pow = 1;
      break;
    case FX_BY_WEIGHT: {
      // Patada Baja: cuanto mas pesa el rival, mas duele. Los escalones
      // son los de los juegos, en decenas de kilo.
      uint16_t w = pesoAhora(def);
      pow = w >= 20 ? 120 : w >= 10 ? 100 : w >= 5 ? 80 : w >= 2 ? 60 : 40;
      break;
    }
    case FX_BY_WEIGHT_REL: {
      // Cuerpo Pesado: lo que importa es cuanto MAS pesas tu.
      uint16_t mio = pesoAhora(att);
      uint16_t suyo = pesoAhora(def);
      if (!suyo) suyo = 1;
      uint16_t veces = (uint16_t)(mio * 10 / suyo);   // x10 para no perder
      pow = veces >= 50 ? 120 : veces >= 40 ? 100 : veces >= 30 ? 80
          : veces >= 20 ? 60 : 40;
      break;
    }
    case FX_KNOCK_OFF:
      // Desarme: pega mas FUERTE si hay algo que tirar. Quitarselo es
      // lo de despues, en useMove. Lo que no se puede tirar no cuenta.
      if (def.item != IT_NONE && !objetoIntocable(def, def.item))
        pow = (uint16_t)(pow * 3 / 2);
      break;
    case FX_NO_ITEM_X2:
      // Acrobacia: el doble yendo ligero. Es la pareja de Unburden y
      // la razon de que ir sin nada sea una decision.
      if (att.item == IT_NONE) pow = (uint16_t)(pow * 2);
      break;
    case FX_FLING: {
      // Lanzamiento: tira lo que lleve. Lo que pega depende de QUE
      // sea, y sin peso por objeto en la tabla se usa el bolsillo,
      // que es la informacion que si hay: una piedra pesa, una baya
      // no. Sin nada en las manos el golpe falla (en useMove).
      const ItemEntry &it = objeto(att);
      pow = att.item == IT_NONE ? 1
          : it.pocket == BP_EVO   ? 80     // una piedra en la cabeza
          : it.pocket == BP_BALL  ? 30
          : esBayaDeCura(it) || it.fx == IT_BERRY_CURE ? 10
          : 40;
      break;
    }
    case FX_X2_IF_STATUS:         // Hex
      if (def.status != ST_NONE) pow = (uint16_t)(pow * 2);
      break;
    default:
      break;
  }
  if (pow == 0) pow = 1;

  // Formula clasica simplificada. El +2 final evita el dano 0, que en una
  // pantalla sin numeros pequeños se leeria como "no paso nada".
  uint32_t base = ((uint32_t)(2 * att.level / 5 + 2) * pow * A / D) / 50 + 2;

  if (r.crit) {
    base = base * 3 / 2;
    // Sniper: sus criticos pegan la mitad otra vez (x2,25 en total).
    if (att.abil() == AB_SNIPER) base = base * 3 / 2;
  }
  // STAB: si el golpe es del tipo del que pega, x1,5 (x2 con Adaptability)
  if (m.type == att.type1() || (att.type2() && m.type == att.type2()))
    base = (att.abil() == AB_ADAPTABILITY) ? base * 2 : base * 3 / 2;

  // Habilidades que tocan la potencia del golpe.
  switch (att.abil()) {
    case AB_PINCH_TYPE:
      // Blaze, Overgrow y compania: solo con el deposito por debajo de un
      // tercio. Es lo que convierte estar a punto de caer en una amenaza.
      if (m.type == att.abilArg() && att.hpMax && att.hp * 3 <= att.hpMax)
        base = base * 3 / 2;
      break;
    case AB_TECHNICIAN:
      if (m.power && m.power <= att.abilArg()) base = base * 3 / 2;
      break;
    case AB_SHEER_FORCE:
      // +30% a los golpes que llevan efecto ADICIONAL (EffectChance del
      // PBS: ver fuerzaBruta). El efecto se pierde en useMove().
      if (moveHas(mv, MV_ADDL)) base = base * 13 / 10;
      break;
    case AB_BITE_BOOST:
      if (moveHas(mv, MV_BITE)) base = base * 3 / 2;
      break;
    case AB_KICK_BOOST:
      // CAMORRISTA, del Anil: un 20% a las patadas. La bandera
      // Kicking la traen 13 golpes del PBS.
      if (moveHas(mv, MV_KICK)) base = base * 12 / 10;
      break;
    case AB_FORESTRY:
      // SILVANO, del Anil: +30% a CUALQUIER tipo sobre hierba. No
      // mira el tipo del golpe, solo el suelo — y por eso es buena.
      if (terrain == TR_GRASS) base = base * 13 / 10;
      break;
    case AB_TYPE_X15:
      if (m.type == att.abilArg()) base = base * 3 / 2;
      break;
    case AB_TYPE_X13:
      if (m.type == att.abilArg()) base = base * 13 / 10;
      break;
    case AB_PULSE_BOOST:
      if (moveHas(mv, MV_PULSE)) base = base * 3 / 2;
      break;
    case AB_CONTACT_BOOST:
      // GARRA DURA: un tercio a lo que toca. La llevan SEIS megas,
      // mas que cualquier otra de las que les faltaban.
      if (moveHas(mv, MV_CONTACT)) base = base * 4 / 3;
      break;
    case AB_PUNK_ROCK:
      // PUNK ROCK: el sonido pega un 30% mas. La otra mitad (que le
      // entre a la mitad) va con las mitades del que recibe.
      if (moveHas(mv, MV_SOUND)) base = base * 13 / 10;
      break;
    case AB_SLICE_BOOST:
      if (moveHas(mv, MV_SLICE)) base = base * 3 / 2;
      break;
    case AB_SOLAR_POWER:
      // Al sol pega un 50% mas de especial. El precio (un octavo por
      // turno) se cobra al cerrar el turno, no aqui.
      if (weather == WE_SUN && !m.phys) base = base * 3 / 2;
      break;
    case AB_GLACIAL:
      // PODER GELIDO, del Anil: con nieve, el hielo pega un 50% mas.
      // El +50% de velocidad va en movesFirst, que es donde se mira.
      if (weather == WE_HAIL && m.type == T_ICE) base = base * 3 / 2;
      break;
    case AB_PUNCH_BOOST:
      if (moveHas(mv, MV_PUNCH)) base = base * 12 / 10;
      break;
    case AB_RIVALRY:
      // Mas fuerte contra el de su mismo sexo, mas flojo contra el
      // otro. Con alguno sin sexo no pasa nada, que es lo correcto.
      if (att.sexo != 2 && def.sexo != 2)
        base = (att.sexo == def.sexo) ? base * 5 / 4 : base * 3 / 4;
      break;
    case AB_RECKLESS:
      // Temerario: los golpes que le duelen a uno mismo, mas fuertes.
      // Es la pareja de Rock Head — aquel quita el precio, este sube
      // el premio.
      if (m.fx == FX_RECOIL_2 || m.fx == FX_RECOIL_3 ||
          m.fx == FX_RECOIL_4 || m.fx == FX_RECOIL_HP || m.fx == FX_CRASH)
        base = base * 12 / 10;
      break;
    case AB_SAND_FORCE:
      if (weather == WE_SAND &&
          (m.type == T_ROCK || m.type == T_GROUND || m.type == T_STEEL))
        base = base * 13 / 10;
      break;
    case AB_ANALYTIC:
      // Pega mas si va el ULTIMO: le ha dado tiempo a mirar. Premia
      // ser lento, que es raro y por eso vale la pena tenerlo.
      if (ultimo) base = base * 13 / 10;
      break;
    default:
      break;
  }
  // AURA: ese tipo pega un tercio mas, lo lance QUIEN SEA. Por eso se
  // mira en los dos lados: un Aura Oscura del rival tambien refuerza
  // tus golpes siniestros, y eso es lo que la hace rara.
  if ((att.abil() == AB_AURA && m.type == att.abilArg()) ||
      (defAb == AB_AURA && m.type == def.abilArg()))
    base = base * 4 / 3;
  // Las mitades del que recibe.
  if (defAb == AB_HALVE_CLASS &&
      ((def.abilArg() != 0) == (m.phys != 0)))
    base /= 2;
  if (defAb == AB_HALVE_TYPE && m.type == def.abilArg()) base /= 2;
  // ARMADURA PRISMA: un cuarto menos de lo que le va mal.
  if (defAb == AB_ANTI_SE && r.effect > 100) base = base * 3 / 4;
  // PUNK ROCK tambien AGUANTA el sonido: le entra a la mitad.
  if (defAb == AB_PUNK_ROCK && moveHas(mv, MV_SOUND)) base /= 2;
  // CORRIENTE DELTA: al volador se le quitan sus debilidades. Se
  // baja la efectividad a neutra en vez de tocar la potencia, que es
  // lo que dice la ficha ("anula las vulnerabilidades").
  if (defAb == AB_DELTA_STREAM && r.effect > 100 &&
      (def.type1() == T_FLYING || def.type2() == T_FLYING))
    r.effect = 100;
  // FUERZA MENTAL: un 25% mas a lo que ya le va bien. Va aqui porque
  // necesita la efectividad, que arriba todavia no esta.
  if (att.abil() == AB_NEURO_FORCE && r.effect > 100)
    base = base * 125 / 100;
  // LANUDO: la mitad del contacto y el doble del fuego. Las dos
  // mitades juntas, que es lo que lo hace un trueque y no un regalo.
  if (defAb == AB_FLUFFY) {
    if (m.type == T_FIRE) base = base * 2;
    else if (moveHas(mv, MV_CONTACT)) base /= 2;
  }
  if (defAb == AB_HALVE_FIRE_ICE && (m.type == T_FIRE || m.type == T_ICE))
    base /= 2;
  // Multiscale: entero aguanta el doble. Se rompe al primer golpe, asi que
  // premia llegar intacto y no se acumula con nada.
  if (defAb == AB_MULTISCALE && def.hp == def.hpMax) base /= 2;
  // ASALTO ESPADA: al que acaba de usarlo le entra TODO al doble hasta
  // que vuelva a moverse. Es el precio de sus 120 de potencia, y sin esto
  // el golpe era gratis.
  if (def.expuesto) base *= 2;
  // El cielo: al sol el fuego arrecia y el agua se evapora, con lluvia al
  // reves. Es la mitad de para que sirve Sunny Day.
  if (weather == WE_SUN) {
    if (m.type == T_FIRE) base = base * 3 / 2;
    // VAPOR HIDRO es la excepcion: es de agua y el sol le VIENE BIEN (su
    // aumento esta arriba, en la potencia). Sin esta linea el recorte
    // general se comia el aumento y de hecho lo dejaba PEOR que sin sol
    // —— medido: 26 de dano con sol contra 35 sin el ——, o sea lo
    // contrario de lo que el golpe hace.
    else if (m.type == T_WATER && m.fx != FX_X15_SUN) base = base / 2;
  } else if (weather == WE_RAIN) {
    if (m.type == T_WATER) base = base * 3 / 2;
    else if (m.type == T_FIRE) base = base / 2;
  }

  // Los dos "deportes": mientras duran, ese tipo pega la mitad. Van con
  // el cielo porque son la misma clase de cosa — una regla del campo que
  // alguien gasto un turno en poner — y alcanzan a los dos bandos.
  if ((sport & SP_WATER) && m.type == T_FIRE) base /= 2;
  if ((sport & SP_MUD) && m.type == T_ELEC) base /= 2;

  // El SUELO. A diferencia del cielo, solo cuenta si quien va a notarlo esta
  // pisandolo: el campo electrico sube el golpe de quien LO LANZA desde el
  // suelo, y el campo de niebla parte el dano dragon de quien lo RECIBE ahi.
  if (terrain == TR_ELEC && m.type == T_ELEC && att.grounded())
    base = base * TERRAIN_BOOST / 100;
  else if (terrain == TR_GRASS && m.type == T_GRASS && att.grounded())
    base = base * TERRAIN_BOOST / 100;
  else if (terrain == TR_PSY && m.type == T_PSYCHIC && att.grounded())
    base = base * TERRAIN_BOOST / 100;
  else if (terrain == TR_MISTY && m.type == T_DRAGON && def.grounded())
    base = base * TERRAIN_DRAGON / 100;
  // El campo de hierba amortigua los temblores: Terremoto, Magnitud y
  // Terratemblor pegan la mitad (pbModifyDamage / pbBaseDamage del Anil;
  // sin mirar si alguien pisa el suelo, como alli).
  if (terrain == TR_GRASS &&
      (m.fx == FX_MAGNITUDE || !strcmp(m.name, "EARTHQUAKE") || !strcmp(m.name, "BULLDOZE")))
    base /= 2;
  // Lo que lleva encima. Va antes de la efectividad porque son todo
  // multiplicadores y el orden no cambia el resultado, pero asi se lee junto.
  {
    const ItemEntry &it = objeto(att);
    if (it.fx == IT_TYPE_BOOST && m.type == it.arg) base = base * 12 / 10;
    else if (it.fx == IT_LIFE_ORB) base = base * (100 + it.arg) / 100;
    else if (it.fx == IT_CHOICE && (it.arg != 0) == (m.phys != 0)) base = base * 3 / 2;
  }

  base = base * r.effect / 100;
  // Tinted Lens: lo que le va mal al tipo, el doble. Deja de ser un
  // bicho con un solo golpe util.
  if (att.abil() == AB_TINTED_LENS && r.effect < 100) base *= 2;
  // Pantallas: parten el dano por la mitad. Un critico las atraviesa, que es
  // justo lo que impide que Reflect convierta el combate en un muro.
  // Infiltrator tambien: se cuela por detras de la pared.
  if (!r.crit && att.abil() != AB_INFILTRATOR) {
    if (m.phys && def.reflect) base /= 2;
    else if (!m.phys && def.lightScreen) base /= 2;
  }
  base = base * (85 + btRand(16)) / 100;  // la varianza de siempre

  if (base == 0) base = 1;
  if (base > 65535) base = 65535;
  // False Swipe nunca tumba: deja siempre al rival con uno. Va DESPUES del
  // suelo de 1, o el suelo volveria a matarlo cuando al rival le queda un PS.
  if (m.fx == FX_FALSESWIPE && def.hp && base >= def.hp) base = def.hp - 1;
  r.damage = (uint16_t)base;
  return r;
}

// Lo que pasa nada mas pisar la pista y vale IGUAL al empezar el
// combate y al relevar. Esta aparte porque la leccion del Velo Vapor
// fue justo esa: una habilidad de entrada que solo mira start() no se
// dispara nunca en un mega, que siempre entra de relevo.
// VEleta del tiempo (Forecast) y la tabla de Multitipo: su tipo no es
// el de su especie, asi que se pisa con typeSet. Se recalcula cada vez
// que puede haber cambiado: al entrar y al cerrar el turno, porque el
// cielo cambia en mitad del combate.
void Battle::syncTipo(uint8_t quien) {
  Combatant &c = side[quien];
  if (!c.valid()) return;
  // (Prediccion ya no pasa por aqui: desde la leva 70 Castform CAMBIA DE
  // FORMA, y el tipo sale de la forma. Ver syncForma.)
  if (c.abil() == AB_TYPE_FROM_ITEM) {
    // La tabla que lleve manda. Sin tabla se queda con su tipo.
    const ItemEntry &it = itemAt(c.item);
    if (it.fx == IT_TYPE_BOOST && it.arg) {
      c.typeSet = it.arg;
      c.typeSet2 = 0;
    }
  }
}

void Battle::habilidadDeEntrada(uint8_t quien) {
  Combatant &yo = side[quien];
  Combatant &otro = side[quien ^ 1];
  if (yo.abil() == AB_SET_TERRAIN) {
    // Los cuatro "Surge" son el Drought del suelo.
    terrain = yo.abilArg();
    terrainTurns = TERRAIN_TURNS;
    startAbil[quien] = yo.ability;
  }
  if (yo.abil() == AB_ILLUMINATE && otro.applyStage(SG_ACC, -1, true))
    startAbil[quien] = yo.ability;   // el Illuminate del Anil
  if (yo.abil() == AB_STEAM_VEIL) {
    // VELO VAPOR saca el sol al entrar, como un Drought, pero por seis
    // turnos y con el tipo que absorbe en el arg (agua), asi que no
    // puede compartir la rama de AB_SET_WEATHER.
    weather = WE_SUN;
    weatherTurns = STEAM_VEIL_TURNS;
    startAbil[quien] = yo.ability;
  } else if (yo.abil() == AB_SET_WEATHER) {
    weather = yo.abilArg();
    weatherTurns = WEATHER_TURNS;
    startAbil[quien] = yo.ability;
  } else if (yo.abil() == AB_INTIMIDATE) {
    // INTIMIDACION, y lo que la contesta: a un PERRO GUARDIAN no le baja
    // el ataque, se lo SUBE. Esta mitad solo estaba en el arranque del
    // combate; relevando se perdia.
    if (otro.abil() == AB_GUARD_DOG) {
      if (otro.applyStage(SG_ATK, +1)) startAbil[quien ^ 1] = otro.ability;
    } else if (otro.applyStage(SG_ATK, -1, true)) {
      // `true` = viene de FUERA: la baja el rival, asi que la paran la
      // Niebla y Cuerpo Puro.
      startAbil[quien] = yo.ability;
    }
  }
}

// Leva 70. Pone la forma de combate `forma` (0 = la de siempre). Es
// pbChangeForm del Anil: si los PS base cambian (Zygarde Completo), se
// conserva el DANO recibido, no los PS. true si cambio algo.
bool Battle::ponForma(uint8_t quien, uint8_t forma) {
  Combatant &c = side[quien];
  int8_t nueva = forma ? bformRow(c.dex, forma) : (int8_t)-1;
  if (forma && nueva < 0) return false;     // esa especie no tiene esa forma
  if (c.bform == nueva) return false;
  uint16_t antes = hpAtLevelBase(c.base(0), c.level, c.trHp);
  c.bform = nueva;
  uint16_t ahora = hpAtLevelBase(c.base(0), c.level, c.trHp);
  if (ahora != antes) {
    uint16_t dano = c.hpMax > c.hp ? (uint16_t)(c.hpMax - c.hp) : 0;
    int32_t tope = (int32_t)c.hpMax + (int32_t)ahora - (int32_t)antes;
    c.hpMax = (uint16_t)(tope > 1 ? tope : 1);
    c.hp = dano < c.hpMax ? (uint16_t)(c.hpMax - dano) : 1;
  }
  return true;
}

// Leva 70. LAS FORMAS DE COMBATE, con las reglas de Battler_ChangeSelf
// del Anil (pbCheckForm y pbCheckFormOnWeatherChange). Como alli, solo
// cambia LA ESPECIE (isSpecies?): un Snorlax con Prediccion copiada no
// se vuelve de fuego. Y nunca transformado ni mega.
//   momento 0 = al entrar, 1 = al cerrar la ronda, 2 = el cielo cambio
void Battle::syncForma(uint8_t quien, uint8_t momento) {
  Combatant &c = side[quien];
  if (!c.valid() || c.fainted() || c.transformed || c.isMega()) return;
  const uint8_t ab = c.abil();
  switch (c.dex) {
    case 351:   // Castform, Prediccion: sol 1, lluvia 2, granizo 3
      ponForma(quien, ab != AB_FORECAST ? 0
                      : weather == WE_SUN  ? 1
                      : weather == WE_RAIN ? 2
                      : weather == WE_HAIL ? 3 : 0);
      break;
    case 421:   // Cherrim, Don Floral: al sol se abre
      ponForma(quien, (ab == AB_SUN_BOOST && weather == WE_SUN) ? 1 : 0);
      break;
    case 875:   // Eiscue: el granizo le rehace la cara (y al entrar con el)
      if (ab == AB_ICE_FACE && momento != 2 && c.bform >= 0 &&
          weather == WE_HAIL)
        ponForma(quien, 0);
      break;
    default:
      break;
  }
  if (momento == 2) return;   // lo demas no mira el cielo
  switch (c.dex) {
    case 555:   // Darmanitan, Modo Daruma: a media vida. El de Galar es la
                // forma 2, y su Daruma la 3: la forma regional + 1.
      if (ab == AB_ZEN_MODE)
        ponForma(quien, (uint32_t)c.hp * 2 <= c.hpMax ? (uint8_t)(c.form + 1) : 0);
      break;
    case 774:   // Minior, Escudo Limitado: bajo media vida, el nucleo
      if (ab == AB_HP_FORM)
        ponForma(quien, (uint32_t)c.hp * 2 > c.hpMax ? 0 : 7);
      break;
    case 746:   // Wishiwashi, Banco: desde el nivel 20 y por encima de 1/4
      if (ab == AB_HP_FORM)
        ponForma(quien, (c.level >= 20 && (uint32_t)c.hp * 4 > c.hpMax) ? 1 : 0);
      break;
    case 718:   // Zygarde, Agrupamiento: al cerrar la ronda a media vida, y
                // ya no vuelve
      if (ab == AB_HP_FORM && momento == 1 && (uint32_t)c.hp * 2 <= c.hpMax)
        ponForma(quien, 2);
      break;
    case 681:   // Aegislash: la postura la decide useMove (`blade`)
      if (ab == AB_STANCE) ponForma(quien, c.blade ? 1 : 0);
      break;
    case 778:   // Mimikyu: sin disfraz, la forma descubierta
      if (ab == AB_DISGUISE) ponForma(quien, c.disguised ? 0 : 1);
      break;
    case 877:   // Morpeko: voraz un turno si y otro no
      if (ab == AB_HUNGER) ponForma(quien, c.hungry ? 1 : 0);
      break;
    case 845:   // Cramorant: el bocado lo pone useMove; sin el, la normal
      if (!c.gulped) ponForma(quien, 0);
      break;
    default:
      break;
  }
}

void Battle::alEntrar(uint8_t quien) {
  Combatant &yo = side[quien];
  Combatant &otro = side[quien ^ 1];
  syncTipo(quien);
  if (!yo.valid() || yo.fainted()) return;
  switch (yo.abil()) {
    case AB_ENTRY_STAGE:
      // Espada Indomable y Escudo Recio: UNA vez por combate, que es
      // lo que dice su ficha, no una por relevo.
      if (!yo.entryDone && yo.applyStage(yo.abilArg(), +1)) {
        yo.entryDone = true;
        startAbil[quien] = yo.ability;
      }
      break;
    case AB_DOWNLOAD:
      // DESCARGA: mira cual de las dos defensas del rival es mas
      // flaca y se sube el ataque que la aprovecha.
      if (yo.applyStage(otro.defStat() <= otro.spDefStat() ? SG_ATK
                                                           : SG_SPA, +1))
        startAbil[quien] = yo.ability;
      break;
    case AB_IMPOSTER:
      // TRANSISTOR (Imposter): se vuelve el otro. transformInto ya
      // existia para Ditto, asi que la habilidad es una linea.
      if (otro.valid() && !yo.isMega()) {
        yo.transformInto(otro);
        startAbil[quien] = yo.ability;
      }
      break;
    case AB_COMMANDER: {
      // COMANDANTE: toma el control. En los juegos el +2 lo recibe el
      // Dondozo en el que se mete; sin aliado, el que manda es el. Solo
      // en lo que USA (especial y velocidad): las defensas de regalo
      // romperian el trueque de ser fragil (68 de PS).
      bool algo = yo.applyStage(SG_SPA, +2);
      if (yo.applyStage(SG_SPE, +2)) algo = true;
      if (algo) startAbil[quien] = yo.ability;
      break;
    }
    case AB_TRACE:
      // RASTRO: al entrar se queda con la habilidad del rival. No se
      // copia una que sea Rastro, que seria copiarse a si mismo.
      if (otro.valid() && otro.ability != 0 &&
          otro.abil() != AB_TRACE) {
        yo.ability = otro.ability;
        yo.abilOff = false;
        startAbil[quien] = yo.ability;
      }
      break;
    case AB_TERA_SHIFT:
      // CAMBIO TERA: al entrar absorbe energia. Sin forma a la que
      // cambiar, lo que se nota es el stat: +1 al mas alto.
      if (yo.applyStage(statMasAlto(yo), +1))
        startAbil[quien] = yo.ability;
      break;
    case AB_DISGUISE:
      // DISFRAZ: entra con la cascara puesta.
      yo.disguised = true;
      startAbil[quien] = yo.ability;
      break;
    case AB_ILLUSION:
      // ILUSION: entra con la cara del ULTIMO del equipo que aun puede
      // luchar (pbLastInTeam del Anil), si no es el mismo. Esto usaba
      // la forma 1 creyendo que era la ilusion -- y la forma 1 de Zorua
      // es la de HISUI: salia un Zorua de Hisui. La cara la trae
      // `ilusDex`, que pone quien conoce el equipo; sin ella no hay.
      // Y NO se anuncia: una ilusion que se presenta no engana.
      yo.illusion = yo.ilusDex > 0 && yo.ilusDex != yo.asDex;
      break;
    case AB_ORICHALCUM:
      // PULSO ORICALCO: saca el sol al entrar. Lo que le crece con el sol
      // se mira en la formula, con Protosintesis.
      weather = WE_SUN;
      weatherTurns = WEATHER_TURNS;
      startAbil[quien] = yo.ability;
      break;
    case AB_HADRON:
      terrain = TR_ELEC;
      terrainTurns = TERRAIN_TURNS;
      startAbil[quien] = yo.ability;
      break;
    case AB_HALLOWED:
      // TINTINEO, del Anil: la campanada le quita el estado al
      // entrar. La otra mitad (sonido psiquico) va en la potencia.
      if (yo.status != ST_NONE) {
        yo.status = ST_NONE;
        yo.statusTurns = 0;
        startAbil[quien] = yo.ability;
      }
      break;
    default:
      break;
  }
  // La forma, AL FINAL: el Disfraz y compania ya estan puestos.
  syncForma(quien, 0);
}

void Battle::start(const Combatant &mine, const Combatant &foe) {
  // TODO el estado vuelve a su valor inicial. `bt.b` es un Battle GLOBAL
  // que se reaprovecha —— battle_ui llama a start() y nunca construye uno
  // nuevo ——, y aqui se limpiaban NUEVE campos de cuarenta y siete. Lo
  // demas cruzaba de un combate al siguiente: las trampas de entrada, el
  // viento de cola, el Canto Suerte, una Premonicion a medio volar... y
  // `megaUsed`, que es por lo que el boton de megaevolucionar no volvia
  // a salir nunca mas despues de usarlo una vez.
  //
  // Se asigna un Battle recien construido y no un memset: hay campos que
  // NO empiezan en cero (frisked es IT_NONE, forewarned es 0xFFFF,
  // futureHitOn es 0xFF, danced es 0xFFFF) y un memset los estropearia.
  //
  // Lo que se salva no es estado, es la DESCRIPCION del combate, y la
  // pone quien lo abre —— isTrainer en battle_ui.ino:120, que corre
  // ANTES de llegar aqui.
  const bool cfgTrainer = isTrainer;
  const bool cfgStrict = strictOrders;
  const bool cfgReservas = playerReserves;
  const bool cfgReservasRival = foeReserves;
  // EN EL SITIO, sin temporal. `*this = Battle()` construye un Battle
  // entero en la PILA y lo copia: son 800 bytes desde que la Premonicion
  // guarda la foto del que la lanzo (antes 468), y este proyecto ya tuvo
  // una placa en bucle por buffers grandes en la pila. Asi no sube nada.
  this->~Battle();
  new (this) Battle();
  isTrainer = cfgTrainer;
  strictOrders = cfgStrict;
  playerReserves = cfgReservas;
  foeReserves = cfgReservasRival;

  side[BT_PLAYER] = mine;
  side[BT_FOE] = foe;

  // Lo que pasa nada mas salir. El orden importa: primero el cielo (que lo
  // pone el mas rapido, como en los juegos) y luego los amilanamientos.
  startAbil[0] = startAbil[1] = 0;
  endAbil = 0;
  // El orden se decide UNA vez. Estaba dentro del bucle, y movesFirst()
  // tira dados (Garra Rapida, y la moneda al aire de los empates), asi
  // que las dos vueltas podian contestar distinto: con velocidades
  // iguales habia una de cada cuatro en que un lado salia DOS veces y
  // el otro NINGUNA, y su habilidad de entrada no pasaba. Nadie lo
  // habia visto porque hasta ahora las de entrada eran Drought y
  // compania, que son raras; lo canto el test de Illuminate.
  const bool yoPrimero = movesFirst(BT_PLAYER);
  for (uint8_t k = 0; k < 2; k++) {
    uint8_t s = yoPrimero ? (k == 0 ? BT_PLAYER : BT_FOE)
                          : (k == 0 ? BT_FOE : BT_PLAYER);
    alEntrar(s);
    // Todo lo que hace una habilidad por estar en pista vive en UNA
    // funcion, que llaman tambien el relevo y la megaevolucion. Estaba
    // escrito aqui y otra vez en sendOut(), con listas parecidas pero
    // no iguales —— el Perro Guardian solo estaba en esta.
    habilidadDeEntrada(s);
  }
  // FRISK: le ve lo que lleva encima. Es informacion, no dano — y la
  // PREVISION: le mira el golpe mas fuerte. Va con Frisk y
  // Anticipation porque es lo mismo: informacion al entrar.
  for (uint8_t s = 0; s < 2; s++) {
    forewarned[s] = 0xFFFF;
    if (side[s].abil() == AB_FOREWARN) {
      forewarned[s] = golpeMasFuerte(side[s ^ 1]);
      if (forewarned[s] != 0xFFFF) startAbil[s] = side[s].ability;
    }
  }
  // informacion aqui vale, porque saber que el otro trae Banda Audaz
  // o unas Sobras cambia si te quedas o te vas.
  for (uint8_t s = 0; s < 2; s++)
    if (side[s].abil() == AB_FRISK && side[s ^ 1].item != IT_NONE) {
      frisked[s] = side[s ^ 1].item;
      startAbil[s] = side[s].ability;
    }
  // ANTICIPATION: se estremece si el otro trae algo que le hace mucho
  // dano. Como Frisk, es informacion — y la informacion aqui pesa,
  // porque decide si te quedas o te vas antes de comerte el golpe.
  for (uint8_t s = 0; s < 2; s++)
    if (side[s].abil() == AB_ANTICIPATION && temblor(s)) {
      shuddered[s] = true;
      startAbil[s] = side[s].ability;
    }
}

// ¿El de enfrente trae algun golpe que le haga MUCHO dano? Es lo que
// mira Anticipation. "Mucho" = super eficaz o fulminante.
bool Battle::temblor(uint8_t quien) const {
  const Combatant &yo = side[quien];
  const Combatant &otro = side[quien ^ 1];
  for (uint8_t i = 0; i < otro.nMoves; i++) {
    uint16_t mv = otro.moves[i];
    if (mv >= MOVE_COUNT) continue;
    const MoveEntry &m = MOVE_TBL[mv];
    if (m.fx == FX_OHKO) return true;
    if (m.power == 0) continue;
    if (typeEffect(m.type, yo.type1(), yo.type2()) > 100) return true;
  }
  return false;
}

// DEFIANT y COMPETITIVE: que el RIVAL le baje un stat le sienta mal y
// se crece dos escalones. Se llama solo cuando la bajada vino del
// otro — bajarse uno mismo (Overheat, Close Combat) no cuenta, que es
// lo que lo separa de una subida gratis.
static void picado(Combatant &c, HitResult &r) {
  if (c.abil() != AB_SPITE_STAT) return;
  if (c.applyStage(c.abilArg(), +2)) {
    r.abilFired = c.ability;
    r.abilOfDef = true;
  }
}

// A quien ata una habilidad de atrapar: con arg 0 ata a todos menos a
// los fantasma (Sombra Tenaz); con un tipo, solo a los de ese tipo
// (Iman). Esta aparte porque lo miran los dos sitios por los que se sale
// del combate, y uno de los dos se me habia olvidado una vez.
static bool atrapaA(const Combatant &c, uint8_t arg) {
  if (arg == 0)
    return c.type1() != T_GHOST && c.type2() != T_GHOST;
  return c.type1() == arg || c.type2() == arg;
}

// FANTASMA: ese se va SIEMPRE. No es una rareza suelta sino una linea
// del Anil que vale para huir Y para relevar, y va ANTES de mirar nada
// mas —— ni el agarre, ni Mal de Ojo, ni Sombra Tenaz lo retienen:
//
//   028_Battle/008_Battle_ActionRunning.rb:9
//     return true if battler.pbHasType?(:GHOST) && MORE_TYPE_EFFECTS
//   028_Battle/006_Battle_ActionSwitching.rb:49
//     return true if MORE_TYPE_EFFECTS && battler.pbHasType?(:GHOST)
//
// MORE_TYPE_EFFECTS es `MECHANICS_GENERATION >= 6` y el Anil va por la
// 9, asi que esta puesto. Es la otra cara de que a un fantasma no le
// entren los golpes normales: atraviesa las paredes en los dos
// sentidos. Aqui no estaba, y un Gengar agarrado por un Bind se
// quedaba preso como cualquier otro.
static inline bool esFantasma(const Combatant &c) {
  return c.type1() == T_GHOST || c.type2() == T_GHOST;
}

bool Battle::canFlee() const {
  if (over || isTrainer) return false;
  const Combatant &yo = side[BT_PLAYER];
  if (!yo.valid() || yo.fainted()) return false;
  if (esFantasma(yo)) return true;          // ver esFantasma()
  // Atrapado no se va: de eso va Bind, y si se pudiera huir el golpe
  // dejaria de significar nada.
  if (yo.bindTurns || yo.trapped2 || yo.noRetreat) return false;
  // IMAN: al rival del tipo que diga su arg no le deja escapar. La
  // habilidad es DEL OTRO, asi que se mira ahi y no en el que huye.
  const Combatant &otro = side[BT_FOE];
  // arg 0 = TODOS menos los fantasma (Sombra Tenaz); arg = T_* = solo
  // los de ese tipo (Iman). Lo del fantasma lo dice su propia ficha.
  if (otro.abil() == AB_TRAP_TYPE && atrapaA(yo, otro.abilArg()))
    return false;
  return true;
}

bool Battle::canSwitchOut(uint8_t quien) const {
  if (quien > 1 || over) return false;
  const Combatant &yo = side[quien];
  if (!yo.valid()) return false;
  // El que ya cayo SIEMPRE sale: lo que no se puede es irse por gusto.
  if (yo.fainted()) return true;
  // Y al que esta a medias de un golpe de varios turnos no se le ensena
  // el menu: ni elige golpe ni pide relevo. Es lo que hace que Rodar
  // tenga precio (la cadena se cortaba gratis sacando al bicho) y lo que
  // hace que el mareo de Enfado no se pueda esquivar.
  if (yo.lockTurns || yo.recharging || yo.chargingMove != 0xFFFF ||
      yo.bideTurns)
    return false;
  if (esFantasma(yo)) return true;          // ver esFantasma()
  // `trapped2` llevaba cuatro sitios ESCRIBIENDOLO y ninguno
  // leyendolo, asi que Mal de Ojo, Presa Maxilar y Arraigo ponian una
  // bandera que no ataba a nadie. Es el mismo fallo que Persecucion
  // (declarado y nunca leido) y el mismo que Trump Card. Aqui se lee.
  if (yo.bindTurns || yo.trapped2 || yo.noRetreat) return false;
  const Combatant &otro = side[quien ^ 1];
  if (otro.abil() == AB_TRAP_TYPE && !otro.fainted() &&
      atrapaA(yo, otro.abilArg()))
    return false;
  return true;
}

bool Battle::tryFlee() {
  if (!canFlee()) return false;
  const Combatant &yo = side[BT_PLAYER];
  const Combatant &otro = side[BT_FOE];
  fleeTries++;

  // Fuga: se escapa SIEMPRE. Es toda la habilidad, y aqui por fin
  // sirve para algo (antes no habia de donde huir).
  bool escapa = yo.abil() == AB_RUN_AWAY;
  if (!escapa) {
    // La cuenta de los juegos: tu velocidad contra la suya, y cada
    // intento suma. Si el otro es lentisimo, te vas seguro.
    uint32_t suya = otro.speStat() / 4;
    if (suya == 0) {
      escapa = true;
    } else {
      uint32_t f = (uint32_t)yo.speStat() * 32 / (suya % 256 ? suya % 256 : 1)
                   + 30u * fleeTries;
      escapa = f >= 256 || (uint32_t)btRand(256) < f;
    }
  }
  if (escapa) {
    over = true;
    fled = true;
    winner = 0xFF;      // ni ganaste ni perdiste: te fuiste
  }
  return escapa;
}

bool Battle::micelio(uint8_t quien, uint8_t slot) const {
  const Combatant &c = side[quien];
  if (c.abil() != AB_MYCELIUM || slot >= BT_MOVES) return false;
  uint16_t mv = c.moves[slot];
  return mv < MOVE_COUNT && MOVE_TBL[mv].power == 0;
}

bool Battle::prankster(uint8_t quien, uint8_t slot) const {
  const Combatant &c = side[quien];
  if (c.abil() != AB_PRANKSTER || slot >= BT_MOVES) return false;
  uint16_t mv = c.moves[slot];
  return mv < MOVE_COUNT && MOVE_TBL[mv].power == 0;
}

bool Battle::grassGlide(uint8_t quien, uint8_t slot) const {
  const Combatant &c = side[quien];
  if (slot >= BT_MOVES || !c.grounded()) return false;
  uint16_t mv = c.moves[slot];
  if (mv >= MOVE_COUNT) return false;
  // Dos golpes con la misma idea y distinto suelo: Fitoimpulso sobre
  // hierba y Bruma Explosiva sobre niebla. Van juntos porque la regla es
  // la misma (su terreno le adelanta) y separarlos seria repetir el
  // codigo dos veces para cambiar una constante.
  uint8_t fx = MOVE_TBL[mv].fx;
  return (terrain == TR_GRASS && fx == FX_PRIO_GRASS) ||
         (terrain == TR_MISTY && fx == FX_PRIO_MISTY);
}

bool Battle::slotBloqueado(uint8_t quien, uint8_t slot) const {
  const Combatant &att = side[quien];
  const Combatant &def = side[quien ^ 1];
  if (slot >= BT_MOVES || slot >= att.nMoves) return true;
  if (att.usesLeft[slot] == 0) return true;
  uint16_t mv = att.moves[slot];
  if (mv >= MOVE_COUNT) return true;
  // Mismo orden y mismas condiciones que las puertas de useMove.
  if (att.disableTurns > 0 && slot == att.disabledSlot) return true;
  // ENCORE y la CHOICE: el Anil deja elegir solo ESE golpe y apaga los
  // demas en el menu. Aqui el menu los dejaba tocar y realSlot sacaba el
  // otro por detras: tocabas Placaje y salia lo que fuera.
  if (att.encoreTurns > 0 && att.encoreSlot < BT_MOVES && slot != att.encoreSlot)
    return true;
  if (att.choiceSlot < BT_MOVES && objeto(att).fx == IT_CHOICE &&
      slot != att.choiceSlot && att.usesLeft[att.choiceSlot] > 0)
    return true;
  if (att.tauntTurns && MOVE_TBL[mv].power == 0) return true;
  if (att.healBlockTurns && curaAlUsarlo(mv)) return true;
  if (att.gagTurns && moveHas(mv, MV_SOUND)) return true;
  if (att.tormented && slot == att.lastSlot && att.lastSlot < BT_MOVES)
    return true;
  if (att.noRepetir && mv == att.noRepetir) return true;
  if (def.imprisoning)
    for (uint8_t i = 0; i < def.nMoves; i++)
      if (def.moves[i] == mv) return true;
  return false;
}

bool Battle::debeForcejear(uint8_t quien) const {
  if (quien > 1) return false;
  const Combatant &c = side[quien];
  if (!c.valid() || c.fainted()) return false;
  // El golpe que no se elige va antes (pbCanShowCommands?): a medio Enfado,
  // cargando, recargando o con la Venganza no hay menu ni Forcejeo.
  if (c.lockTurns || c.recharging || c.chargingMove != 0xFFFF || c.bideTurns)
    return false;
  // Con Encore solo cuenta ESE golpe: o sale el, o Forcejeo.
  if (c.encoreTurns > 0 && c.encoreSlot < BT_MOVES)
    return slotBloqueado(quien, c.encoreSlot);
  for (uint8_t i = 0; i < c.nMoves && i < BT_MOVES; i++)
    if (!slotBloqueado(quien, i)) return false;
  return true;
}

void Battle::ponEquipo(uint8_t lado, const uint8_t *atk, uint8_t n) {
  if (lado > 1) return;
  if (n > 6) n = 6;
  for (uint8_t i = 0; i < n; i++) equipoAtk[lado][i] = atk[i];
  equipoN[lado] = n;
}

// Lo que se decide de UN lado al elegir, antes de que nadie se mueva.
static void preparaLado(Battle &b, uint8_t k, uint8_t slot) {
  Combatant &c = b.side[k];
  // Se decide AQUI, al elegir, y no al moverse: lo que le cierren a mitad
  // de ronda le hace fallar (las puertas de useMove), no forcejear.
  c.forcejea = b.debeForcejear(k);
  uint8_t s = b.realSlot(k, slot);
  uint16_t mv = (s < c.nMoves && !c.forcejea) ? c.moves[s] : MOVE_STRUGGLE;
  uint8_t fx = mv < MOVE_COUNT ? MOVE_TBL[mv].fx : 0;
  c.beakBlast = (fx == FX_BEAKBLAST);
  c.shellTrap = (fx == FX_SHELLTRAP);
}

void Battle::inicioDeRonda(uint8_t slotJugador, uint8_t slotRival) {
  preparaLado(*this, BT_PLAYER, slotJugador);
  preparaLado(*this, BT_FOE, slotRival);
}

void Battle::inicioDeRondaRival(uint8_t slotRival) {
  Combatant &yo = side[BT_PLAYER];
  yo.forcejea = false;
  yo.beakBlast = false;
  yo.shellTrap = false;
  yo.lastFailed = false;      // usar un objeto no es fallar un golpe
  preparaLado(*this, BT_FOE, slotRival);
}

void Battle::inicioDeRondaDos(bool usa0, uint8_t slot0, bool usa1, uint8_t slot1) {
  const bool usa[2] = { usa0, usa1 };
  const uint8_t slot[2] = { slot0, slot1 };
  for (uint8_t k = 0; k < 2; k++) {
    if (usa[k]) { preparaLado(*this, k, slot[k]); continue; }
    Combatant &c = side[k];
    c.forcejea = false;
    c.beakBlast = false;
    c.shellTrap = false;
    c.lastFailed = false;      // trocar ou usar objeto nao e falhar um golpe
  }
}

uint8_t Battle::realSlot(uint8_t quien, uint8_t slot) const {
  const Combatant &att = side[quien];
  // Mismo orden que useMove(): lo de fuera manda sobre lo de dentro.
  // Outrage y compania: mientras dure, no eliges tu.
  if (att.lockTurns > 0) slot = att.lockSlot;
  // Encore es lo mismo pero se lo hace el RIVAL: te obliga a repetir.
  if (att.encoreTurns > 0 && att.encoreSlot < BT_MOVES && !att.encoreSalta)
    slot = att.encoreSlot;
  // Y la Choice se lo hace uno mismo a cambio del +50%.
  if (att.choiceSlot < BT_MOVES && objeto(att).fx == IT_CHOICE)
    slot = att.choiceSlot;
  // La Venganza tampoco se elige: dura sus tres turnos.
  if (att.bideTurns)
    for (uint8_t i = 0; i < att.nMoves; i++)
      if (att.moves[i] < MOVE_COUNT && MOVE_TBL[att.moves[i]].fx == FX_BIDE) {
        slot = i;
        break;
      }
  // Un golpe a medio cargar se suelta solo: el hueco es el suyo.
  if (att.chargingMove != 0xFFFF)
    for (uint8_t i = 0; i < att.nMoves; i++)
      if (att.moves[i] == att.chargingMove) { slot = i; break; }
  return slot;
}

int8_t Battle::speedTier(uint8_t quien, uint8_t slot) const {
  const Combatant &c = side[quien];
  if (slot >= BT_MOVES) return 0;
  // La prioridad es del golpe que va a SALIR, no del que se pidio: con
  // Encore o con la Choice son distintos y el turno se ordenaba mal.
  slot = realSlot(quien, slot);
  if (slot >= BT_MOVES) return 0;
  uint16_t mv = c.moves[slot];
  if (mv >= MOVE_COUNT) return 0;
  int t = MOVE_TBL[mv].prio;
  // Prankster: +1 a los de apoyo. Fitoimpulso y Bruma Explosiva: +1 sobre
  // su terreno. En los juegos los dos son el MISMO escalon (+1), asi que
  // entre ellos manda la velocidad; antes aqui se comparaban en cascada y
  // Prankster ganaba siempre, que no es lo que pasa jugando.
  if (prankster(quien, slot)) t++;
  if (grassGlide(quien, slot)) t++;
  if (t > 7) t = 7;
  if (t < -7) t = -7;
  return (int8_t)t;
}

bool Battle::movesFirst(uint8_t a, uint8_t slotA, uint8_t slotB) const {
  uint8_t b = a ^ 1;

  // EL ESCALON DE PRIORIDAD, lo primero de todo y por delante de la
  // velocidad. Esto faltaba entero: `MoveEntry` no tenia el campo, asi que
  // Ataque Rapido era un golpe normal de 40 y el turno lo decidia solo la
  // velocidad. Los 55 golpes que el PBS marca con prioridad —— Ataque
  // Rapido, Veloc. Extrema, Acua Jet, Puno Bala, Golpe Bajo, Proteccion,
  // Sorpresa... —— no hacian lo unico que los define.
  //
  // El orden de abajo es el de los juegos: escalon, luego Garra Rapida
  // DENTRO del escalon, luego Micofuerza (el ultimo de SU escalon), y por
  // fin la velocidad.
  int8_t ea = speedTier(a, slotA), eb = speedTier(b, slotB);
  if (ea != eb) return ea > eb;

  // Garra Rapida: a veces sales primero aunque seas mas lento. Pero no
  // salta un escalon: adelanta DENTRO del suyo, que es lo que hace en los
  // juegos. Antes iba la primera de todas y una Garra con suerte se ponia
  // por delante de una Proteccion (+4), que no puede pasar.
  const ItemEntry &ia = objeto(side[a]);
  const ItemEntry &ib = objeto(side[b]);
  bool garraA = ia.fx == IT_QUICK_CLAW && (uint8_t)btRand(100) < ia.arg;
  bool garraB = ib.fx == IT_QUICK_CLAW && (uint8_t)btRand(100) < ib.arg;
  if (garraA != garraB) return garraA;

  // MICOFUERZA es el espejo de Prankster: sus golpes de APOYO van los
  // ultimos de su escalon, por rapido que sea. A cambio no se los para
  // ninguna habilidad del rival (eso se mira en computeHit, como Mold
  // Breaker).
  bool lentoA = micelio(a, slotA), lentoB = micelio(b, slotB);
  if (lentoA != lentoB) return lentoB;

  uint32_t sa = side[a].speStat(), sb = side[b].speStat();
  // Swift Swim, Chlorophyll y compania: en su clima, el doble. Va aqui y no
  // en speStat() porque el cielo lo sabe el combate, no el combatiente.
  // PODER GELIDO, del Anil: con nieve corre un 50% mas. La otra
  // mitad (el hielo que pega mas) va en la potencia.
  if (side[a].abil() == AB_GLACIAL && weather == WE_HAIL) sa = sa * 3 / 2;
  if (side[b].abil() == AB_GLACIAL && weather == WE_HAIL) sb = sb * 3 / 2;
  if (side[a].abil() == AB_SPEED_WEATHER && weather == side[a].abilArg()) sa *= 2;
  if (side[b].abil() == AB_SPEED_WEATHER && weather == side[b].abilArg()) sb *= 2;
  // Acometida (del Anil): el primer turno que sale, un 50% mas rapido.
  if (side[a].abil() == AB_BLITZ && side[a].turnsOut == 0) sa = sa * 3 / 2;
  if (side[b].abil() == AB_BLITZ && side[b].turnsOut == 0) sb = sb * 3 / 2;
  // LIVIANO: el doble, pero solo si LARGO algo. Aqui ponia "con las
  // manos vacias" y un comentario diciendo que no habia forma de
  // distinguirlo: si la hay, y el Anil la usa —— el efecto Unburden se
  // enciende en pbRemoveItem y la velocidad pide las dos cosas
  // (`effects[Unburden] && !item`). Sin eso, el que entraba sin nada
  // corria el doble desde el primer turno y para siempre.
  if (side[a].abil() == AB_UNBURDEN && side[a].soltoAlgo &&
      side[a].item == IT_NONE) sa *= 2;
  if (side[b].abil() == AB_UNBURDEN && side[b].soltoAlgo &&
      side[b].item == IT_NONE) sb *= 2;
  // VIENTO AFIN: es del BANDO, asi que sobrevive al relevo. Por eso vale
  // la pena gastar un turno en el: no se lo lleva el que lo puso.
  if (tailwind[a]) sa *= 2;
  if (tailwind[b]) sb *= 2;
  // ESPACIO RARO da la vuelta SOLO a la velocidad: la prioridad, la Garra
  // Rapida y Micofuerza ya se decidieron arriba, como en los juegos.
  if (sa != sb) return trickRoom ? sa < sb : sa > sb;
  return (btRand(2) == 0);  // empate: moneda al aire, como en el juego
}

uint8_t captureChance(const Combatant &foe, uint16_t myLevel,
                      uint8_t ballMult, uint8_t taps) {
  if (!foe.valid() || !foe.hpMax) return 0;
  // La Master Ball no falla. Es lo unico que la hace ser la Master Ball, y
  // con la formula normal se habria quedado en el 95% del tope.
  if (ballMult >= 255) return 100;
  int c = CAPT_BASE;
  // Cuanto mas tocado, mas facil. Es la regla de siempre y es la que hace
  // que debilitarlo antes de tirar la bola sea la jugada.
  c += (int)((uint32_t)(foe.hpMax - foe.hp) * CAPT_BY_DAMAGE / foe.hpMax);
  if (foe.status == ST_SLEEP || foe.status == ST_FREEZE) c += CAPT_SLEEP;
  else if (foe.status != ST_NONE) c += CAPT_STATUS;
  c = c * (ballMult ? ballMult : 10) / 10;
  // Uno muy por encima de tu nivel se resiste. Sin esto, una Poke Ball a un
  // legendario de 60 seria lo mismo que a un Rattata.
  if (foe.level > myLevel) c -= (int)(foe.level - myLevel) * 2;
  int extra = (int)taps * CAPT_TAP;
  c += extra > CAPT_TAP_MAX ? CAPT_TAP_MAX : extra;
  if (c < CAPT_FLOOR) c = CAPT_FLOOR;
  if (c > CAPT_CEIL) c = CAPT_CEIL;
  return (uint8_t)c;
}

uint8_t Battle::disobeyChance(uint8_t bond) {
  if (bond >= OBEY_BOND_OK) return 0;
  // Recta: 0 de vinculo -> OBEY_MAX_PCT, OBEY_BOND_OK -> 0.
  // Se redondea HACIA ARRIBA a proposito. Con division entera, 59 de vinculo
  // daba 20/60 = 0 y la cuesta no empezaba hasta 57, a saltos de tres: el
  // jugador cruzaba el umbral y no pasaba nada. Por debajo del umbral,
  // cualquier descuido tiene que costar algo.
  uint16_t n = (uint16_t)(OBEY_BOND_OK - bond) * OBEY_MAX_PCT;
  return (uint8_t)((n + OBEY_BOND_OK - 1) / OBEY_BOND_OK);
}

Order Battle::giveOrder(uint8_t from, uint8_t slot) {
  Order o;
  o.slot = slot;
  if (over || from > 1 || slot >= BT_MOVES || strictOrders) return o;
  Combatant &att = side[from];
  // Una orden solo se puede desobedecer si de verdad la diste: enfurecido,
  // con un Encore encima, soltando una carga o recargando, el golpe no lo
  // eliges tu, asi que no hay nada que desobedecer.
  if (att.lockTurns > 0 || att.encoreTurns > 0 || att.recharging ||
      att.chargingMove != 0xFFFF || att.bideTurns || att.forcejea)
    return o;

  uint8_t pct = disobeyChance(att.bond);
  if (!pct || (uint8_t)btRand(100) >= pct) return o;

  // Dos caras, mitad y mitad. Las dos son del juego, y tener dos hace que se
  // lea como un bicho con caracter y no como un fallo del mando.
  uint8_t otro = 0xFF;
  if (btRand(2)) {
    uint8_t cand[BT_MOVES], n = 0;
    for (uint8_t i = 0; i < att.nMoves && i < BT_MOVES; i++)
      if (i != slot && att.usesLeft[i] > 0) cand[n++] = i;
    if (n) otro = cand[btRand(n)];
  }
  if (otro != 0xFF) {
    o.slot = otro;
    o.kind = OB_OTHER;
  } else {
    att.refusing = true;   // lo consume blockedThisTurn()
    o.kind = OB_IGNORE;
  }
  return o;
}

void Battle::rompeCadena(uint8_t from, uint16_t mv, HitResult &r) {
  if (mv >= MOVE_COUNT || over) return;
  Combatant &att = side[from];
  uint8_t fxl = MOVE_TBL[mv].fx;
  // La racha de los que crecen al repetirse se pierde al fallar.
  if (fxl == FX_RAMP || fxl == FX_ROLLOUT) {
    att.rachaMove = 0;
    att.rachaDe = 0xFFFF;
  }
  if (att.lockTurns == 0) return;
  if (fxl == FX_ROLLOUT) {
    // Rodar se suelta y ya esta: el precio de la cadena es haberla
    // empezado, no quedarse colgado de un golpe que no entro.
    att.lockTurns = 0;
    att.lockSlot = 0xFF;
  } else if (fxl == FX_LOCK || fxl == FX_UPROAR) {
    att.lockTurns = 0;
    att.lockSlot = 0xFF;
    if (fxl == FX_UPROAR) {
      uproar = 0;
    } else if (att.confuseTurns == 0) {
      att.confuseTurns = (uint8_t)(2 + btRand(4));
      r.selfConfused = true;
    }
  }
}

// O alvo NAO sai empurrado (Rugido, Remolino, Cola Dragao, Llave Giro).
// E a lista do Anil, 187_MoveEffects_SwitchingActing.rb: quem lancou
// Arraigo, e Ventosas e Perro Guardian a nao ser contra Rompemoldes.
static bool naoSaiEmpurrado(const Combatant &att, const Combatant &def) {
  if (def.ingrained) return true;
  uint8_t ab = def.abil();
  return (ab == AB_SUCTION_CUPS || ab == AB_GUARD_DOG) &&
         att.abil() != AB_MOLD_BREAKER;
}

// Leva 70. Tras cada golpe el cielo puede haber cambiado (Dia Soleado,
// Danza Lluvia...), y en el Anil Castform y Cherrim cambian EN EL ACTO
// (pbCheckFormOnWeatherChange). useMoveDentro tiene 37 salidas; aqui
// hay una.
HitResult Battle::useMove(uint8_t from, uint8_t slot, uint16_t forzado) {
  HitResult r = useMoveDentro(from, slot, forzado);
  syncForma(BT_PLAYER, 2);
  syncForma(BT_FOE, 2);
  return r;
}

HitResult Battle::useMoveDentro(uint8_t from, uint8_t slot, uint16_t forzado) {
  HitResult r;
  if (over || from > 1 || slot >= BT_MOVES) return r;
  Combatant &att = side[from];
  Combatant &def = side[from ^ 1];
  // Quien ya se movio este turno. Lo pide Analytic (pega mas si va el
  // ultimo) y se apunta ANTES de nada, porque hasta perder el turno
  // por estar dormido cuenta como haberse movido: el otro ya no le
  // pilla por sorpresa.
  //
  // Se reinicia al cerrar el turno, en endTurn del segundo lado.
  movedThisTurn[from] = true;
  // Como el Anil al entrar en pbUseMove: de aqui para abajo, cada camino
  // que FALLA lo vuelve a poner. Antes los que se iban sin tocarlo dejaban
  // el valor del golpe anterior.
  att.lastFailed = false;

  // Antes de nada: puede moverse? Dormido, congelado, paralizado, aturdido o
  // confundido se pierde el turno (y si es confusion, se pega a si mismo).
  // Si la pantalla ya lo pregunto para poder decirlo antes de anunciar el
  // golpe, esta es la misma respuesta; si nadie lo pregunto, se tira aqui.
  r.blocked = att.peekBlock();
  att.blockCache = 0xFF;          // consumida: el turno que viene, dado nuevo
  if (r.blocked != BLK_NONE) {
    r.hits = 0;
    if (r.blocked == BLK_CONFUSED) {
      // se golpea con 40 de potencia fisica contra su propia defensa
      uint32_t d = ((uint32_t)(2 * att.level / 5 + 2) * 40 * att.atkStat() /
                    (att.defStat() ? att.defStat() : 1)) / 50 + 2;
      r.damage = (uint16_t)(d > 65535 ? 65535 : d);
      att.hp = (r.damage >= att.hp) ? 0 : (uint16_t)(att.hp - r.damage);
      if (att.hp == 0) knockOut(from);
    }
    // Que te corten el turno rompe la furia y tira la carga por la borda: sin
    // esto un Outrage interrumpido dejaria al bicho atado para siempre.
    att.lockTurns = 0;
    att.chargingMove = 0xFFFF;
    // Y le baja del cielo: si no, dormirse a medio Vuelo lo volveria
    // intocable hasta que despertara.
    att.hiding = HID_NONE;
    att.lastFailed = true;     // perder el turno cuenta como fallar
    turn++;
    return r;
  }

  // Que golpe sale de verdad: Outrage ata, Encore obliga, la Choice
  // encadena, un golpe a medio cargar se suelta. Esta lista estaba
  // escrita AQUI y otra vez (mas corta) en btLaunch, y de ahi venia que
  // la caja anunciara un golpe y saliera otro. Ahora hay un solo sitio.
  slot = realSlot(from, slot);

  // SIN GOLPE QUE ELEGIR: lo decidio inicioDeRonda(). Sale Forcejeo y las
  // puertas de abajo (Anular, Mofa, Tormento...) no se miran: son las que
  // le dejaron sin golpe. Bailarin (`forzado`) no pasa por aqui.
  const bool forcejea = att.forcejea && forzado == 0xFFFF;
  if (forcejea) att.noRepetir = 0;   // uso otro golpe: el Martillo vuelve

  // MARTILLO GIGA: 160 de potencia a cambio de no poder encadenarlo. El
  // apunte se borra en cuanto usa cualquier otro golpe, que es justo lo
  // que convierte el turno siguiente en una decision.
  if (att.noRepetir && slot < BT_MOVES) {   // `mv` todavia no existe aqui
    if (att.moves[slot] == att.noRepetir) {
      r.failed = true;
      r.hits = 0;
      att.lastFailed = true;
      turn++;
      return r;
    }
    att.noRepetir = 0;
  }

  // ASALTO ESPADA: la exposicion dura hasta TU siguiente turno, asi que se
  // apaga aqui, al empezar a moverte, y no al cerrar el turno del otro.
  att.expuesto = false;
  // Y lo mismo con Lazo Destino y Rencor: valen hasta que el que los puso
  // vuelve a moverse. Si duraran mas, un Lazo Destino de hace diez turnos
  // seguiria cobrandose, que no es lo que hacen los juegos.
  att.destinyBond = false;
  att.grudge = false;

  // (Sin sangrar a proposito: lo de dentro son las puertas de siempre, y
  // moverlas de columna romperia las anclas de tools/sabotaje_held.py.)
  if (!forcejea) {
  // Disable: ese hueco esta bloqueado y el golpe se pierde. No se redirige a
  // otro a proposito — perder el turno es justo el precio.
  if (att.disableTurns > 0 && slot == att.disabledSlot) {
    r.failed = true;
    r.hits = 0;
    att.lastFailed = true;     // lo mira Pataleta
    turn++;
    return r;
  }

  // COMESUENOS: solo le entra a quien DUERME. Su FunctionCode del PBS
  // acaba en `IfTargetAsleep` y el mapa lo leia como un drenaje normal,
  // asi que se comia los suenos de uno despierto.
  // (aqui todavia no existe `mv`: las puertas vecinas miran moves[slot])
  if (att.moves[slot] < MOVE_COUNT &&
      MOVE_TBL[att.moves[slot]].fx == FX_DRAIN_SLEEP &&
      def.status != ST_SLEEP) {
    r.failed = true;
    r.hits = 0;
    att.lastFailed = true;
    turn++;
    return r;
  }

  // MOFA: mientras dure, los golpes de apoyo no salen. Es el precio
  // de haberse montado una estrategia sin pegar.
  if (att.tauntTurns && att.moves[slot] < MOVE_COUNT &&
      MOVE_TBL[att.moves[slot]].power == 0) {
    r.failed = true;
    r.hits = 0;
    r.blocked = BLK_TAUNT;
    att.lastFailed = true;
    turn++;
    return r;
  }
  if (att.healBlockTurns && curaAlUsarlo(att.moves[slot])) {
    r.failed = true;
    r.hits = 0;
    r.blocked = BLK_HEALBLOCK;
    att.lastFailed = true;
    turn++;
    return r;
  }

  // ULTIMA BAZA: falla mientras le quede algun golpe sin estrenar. Es
  // una condicion sobre TODO el combate, asi que necesita que alguien
  // lleve la cuenta de lo que ya uso (usedMask).
  if (att.moves[slot] < MOVE_COUNT &&
      MOVE_TBL[att.moves[slot]].fx == FX_LAST_RESORT) {
    bool quedaAlguno = false;
    for (uint8_t i = 0; i < att.nMoves; i++)
      if (i != slot && !((att.usedMask >> i) & 1)) quedaAlguno = true;
    if (quedaAlguno || att.nMoves < 2) {
      r.failed = true;
      r.hits = 0;
      att.lastFailed = true;
      turn++;
      return r;
    }
  }

  // SELLAR del rival: no puede usar ningun golpe que el otro tambien
  // sepa. Se mira aqui, al elegir, por lo mismo que Mofa y Mordaza.
  if (def.imprisoning && att.moves[slot] < MOVE_COUNT) {
    for (uint8_t i = 0; i < def.nMoves; i++)
      if (def.moves[i] == att.moves[slot]) {
        r.failed = true;
        r.hits = 0;
        att.lastFailed = true;
        turn++;
        return r;
      }
  }

  // MAJESTAD: al rival no le salen los golpes de PRIORIDAD. Aqui la
  // prioridad son Golpe Bajo, los de apoyo con Prankster y los que su
  // terreno adelanta; el PP se gasta igual, que es lo que dice la
  // ficha ("haciendoles consumir PP como si el mov. hubiera salido").
  if ((def.abil() == AB_QUEENLY || quickGuard[from ^ 1]) &&
      att.moves[slot] < MOVE_COUNT) {
    // Majestad (habilidad) y Anticipo (golpe) hacen LO MISMO: cortar los
    // golpes de prioridad del otro. Comparten el sitio a proposito; si
    // se separaran, el dia que cambie que cuenta como prioridad habria
    // que acordarse de los dos.
    // Ahora se pregunta por el ESCALON, que es lo que la ficha quiere
    // decir. Antes habia que enumerar a mano los tres efectos que
    // adelantaban, y con eso Majestad dejaba pasar Ataque Rapido, Veloc.
    // Extrema y los otros 52 golpes de prioridad del PBS —— o sea casi
    // todos los que tenia que parar.
    bool adelanta = speedTier(from, slot) > 0;
    if (adelanta) {
      r.failed = true;
      r.hits = 0;
      r.abilFired = def.ability;
      r.abilOfDef = true;
      att.lastFailed = true;
      turn++;
      return r;
    }
  }

  // TORMENTO: no puede repetir el golpe anterior. Se mira contra el
  // hueco, no contra el golpe, para que dos huecos con el mismo golpe no
  // sirvan de escapatoria.
  if (att.tormented && slot == att.lastSlot && att.lastSlot < BT_MOVES) {
    r.failed = true;
    r.hits = 0;
    att.lastFailed = true;
    turn++;
    return r;
  }

  // GOLPE MORDAZA del rival: mientras dure, los golpes de SONIDO no
  // salen. Es como Mofa pero mirando la bandera del golpe.
  if (att.gagTurns && att.moves[slot] < MOVE_COUNT &&
      moveHas(att.moves[slot], MV_SOUND)) {
    r.failed = true;
    r.hits = 0;
    att.lastFailed = true;     // lo mira Pataleta
    turn++;
    return r;
  }

  // ERUCTO: hace falta haberse comido una baya. Es literalmente lo que
  // el golpe es, y ahora que las bayas se gastan de verdad se puede.
  if (att.moves[slot] < MOVE_COUNT &&
      MOVE_TBL[att.moves[slot]].fx == FX_BELCH && !att.ateBerry) {
    r.failed = true;
    r.hits = 0;
    att.lastFailed = true;     // lo mira Pataleta
    turn++;
    return r;
  }

  // ESCUPIR con el deposito vacio: no hay nada que escupir.
  if (att.moves[slot] < MOVE_COUNT &&
      MOVE_TBL[att.moves[slot]].fx == FX_SPIT_UP && att.stockpile == 0) {
    r.failed = true;
    r.hits = 0;
    att.lastFailed = true;     // lo mira Pataleta
    turn++;
    return r;
  }

  // PUNO CERTERO: se esta concentrando, y si le tocan antes de soltarlo
  // se le va. Va aqui por lo mismo que Golpe Bajo: el fallo tiene que
  // decidirse ANTES del dano o "falla" y pega igual.
  if (att.moves[slot] < MOVE_COUNT &&
      MOVE_TBL[att.moves[slot]].fx == FX_FOCUS_PUNCH &&
      (att.tookPhys || att.tookSpec)) {
    r.failed = true;
    r.hits = 0;
    att.lastFailed = true;     // lo mira Pataleta
    turn++;
    return r;
  }

  // GOLPE BAJO: solo entra si le pillas ANTES de que se mueva. Es el
  // golpe entero: sin esto era un Placaje de 70 que nunca fallaba, y
  // lo saben 85 especies.
  //
  // Va AQUI, antes de nada. Mi primera version lo puso al final, con
  // los efectos secundarios, y para entonces el dano ya estaba hecho:
  // el golpe "fallaba" y pegaba igual. Lo canto su propio test.
  if (att.moves[slot] < MOVE_COUNT &&
      MOVE_TBL[att.moves[slot]].fx == FX_ONLY_IF_FIRST &&
      movedThisTurn[from ^ 1]) {
    r.failed = true;
    r.hits = 0;
    att.lastFailed = true;
    att.rachaMove = 0;
    att.rachaDe = 0xFFFF;
    turn++;
    return r;
  }
  }  // if (!forcejea)
  // Lo mismo con el que viene cargando: si empezaste un Solar Beam, lo sueltas.
  if (att.chargingMove != 0xFFFF) {
    for (uint8_t i = 0; i < att.nMoves; i++)
      if (att.moves[i] == att.chargingMove) { slot = i; break; }
  }

  uint16_t mv = att.moves[slot];
  // BAILARIN: el golpe viene de fuera, no del hueco. Se pisa aqui, antes
  // de todo, por lo mismo que Copion: de aqui para abajo todo mira `mv`.
  if (forzado != 0xFFFF) mv = forzado;
  if (forcejea) mv = MOVE_STRUGGLE;
  // COPION: se convierte en el ultimo golpe que se vio. Se resuelve AQUI,
  // antes de cualquier cosa, porque a partir de este punto todo el resto
  // de la funcion mira `mv`: el bloqueo, la precision, la potencia y el
  // efecto. Cambiarlo mas abajo seria copiar el nombre y no el golpe.
  // Sin nada que copiar se queda como esta y falla en su `case`.
  if (mv < MOVE_COUNT && MOVE_TBL[mv].fx == FX_COPYCAT &&
      def.lastMove < MOVE_COUNT && def.lastMove != MOVE_STRUGGLE &&
      MOVE_TBL[def.lastMove].fx != FX_COPYCAT)
    mv = def.lastMove;
  // CAMBIO TACTICO: la postura la decide el golpe que va a lanzar, no
  // un turno ni un objeto. Va aqui, con mv ya elegido y antes de
  // resolverlo, porque la postura tiene que valer para ESTE golpe.
  // Como en el Anil (Battler_UseMove): un golpe de dano lo afila y SOLO
  // el Escudo Real lo vuelve a escudar. Antes cualquier golpe de apoyo
  // lo escudaba.
  if (att.abil() == AB_STANCE && mv < MOVE_COUNT) {
    bool afila = att.blade;
    if (MOVE_TBL[mv].power > 0) afila = true;
    else if (!strcmp(MOVE_TBL[mv].name, "KINGSSHIELD")) afila = false;
    if (att.blade != afila) {
      att.blade = afila;
      r.abilFired = att.ability;
      syncForma(from, 0);
    }
  }

  // MUTATIPO: se vuelve del tipo del golpe que va a usar. Va AQUI,
  // con mv ya decidido y antes de la precision, porque su ficha dice
  // que el tipo cambia aunque el ataque falle. Solo si cambia algo:
  // si ya es de ese tipo, no se gasta (y el STAB no se duplica).
  if (att.abil() == AB_PROTEAN && mv < MOVE_COUNT && !forcejea &&
      MOVE_TBL[mv].type != att.type1()) {
    att.typeSet = MOVE_TBL[mv].type;
    att.typeSet2 = 0;        // se queda de UN tipo, como en los juegos
    r.abilFired = att.ability;
  }

  // Este turno es el remate de una carga que ya se pago el turno pasado.
  const bool soltando = (mv < MOVE_COUNT && att.chargingMove == mv);

  // ESTRELLARSE (Patada Salto, Patada Salto Alta, Patada Hacha, Placaje
  // Supercelula). El Anil llama a pbCrashDamage siempre que el golpe "falla
  // contra todos los objetivos" (169_Battler_UseMove.rb:614): punteria, un
  // rival escondido, el Protect y la inmunidad. Aqui solo se estrellaba al
  // fallar la punteria. Muro Magico no lo sufre (takesIndirectDamage?).
  auto seEstrella = [&]() {
    if (mv >= MOVE_COUNT || MOVE_TBL[mv].fx != FX_CRASH) return;
    if (att.abil() == AB_MAGIC_GUARD || att.fainted()) return;
    r.recoil = (uint16_t)(att.hpMax / 2);
    r.crashed = true;
    att.hp = (r.recoil >= att.hp) ? 0 : (uint16_t)(att.hp - r.recoil);
    if (att.hp == 0) knockOut(from);
  };

  // Sin usos (o sin golpes) se forcejea: el combate no puede quedarse clavado.
  // MENOS soltando una carga: o PP saiu ao subir (Vuelo, Bote, Excavar,
  // Buceo, Rayo Solar), a descida nao custa nem confere PP (Essentials:
  // TwoTurnAttack pula o menu e o pbReducePP). Sem o `!soltando`, quem
  // subia com o ultimo PP descia como Forcejeo, o codigo que o traz de volta
  // nao rodava e ele ficava fora da tela o resto do combate (2026-11-13).
  if (forzado == 0xFFFF && !soltando &&
      (slot >= att.nMoves || att.usesLeft[slot] == 0)) {
    mv = MOVE_STRUGGLE;
  } else if (mv != MOVE_STRUGGLE && att.lockTurns == 0 && !soltando &&
             att.bideTurns == 0 && forzado == 0xFFFF) {
    // Pressure del rival: cada golpe cuesta el doble. No hace dano, pero
    // gana combates largos vaciandote el deposito.
    uint8_t coste = (def.abil() == AB_PRESSURE) ? 2 : 1;
    att.usesLeft[slot] = (uint8_t)(att.usesLeft[slot] > coste
                                       ? att.usesLeft[slot] - coste : 0);
  }

  // PREMONICION: el golpe sale AHORA y llega al tercer cierre de ronda.
  // Aqui solo se apunta QUE golpe es; el dano se calcula al caer (endTurn),
  // y cae aunque el que lo lanzo ya no este en la pista.
  if (mv < MOVE_COUNT && MOVE_TBL[mv].fx == FX_FUTURE) {
    if (futureTurns[from]) {      // ya hay uno de camino
      r.failed = true;
      r.hits = 0;
      att.lastFailed = true;
      turn++;
      return r;
    }
    futureMv[from] = mv;
    futureFuera[from] = false;
    futureCaido[from] = false;
    futureTurns[from] = 3;    // FutureSightCounter = 3: cae al 3er cierre
    r.charging = true;            // la pantalla ya sabe pintar esto
    r.hits = 0;
    att.lastMove = mv;
    att.lastSlot = slot;
    turn++;
    return r;
  }

  // VENGANZA: dos turnos aguantando (y apuntando lo que recibe) y al
  // tercero devuelve el doble. Es el Anil: Bide = 3 al empezar, baja uno
  // por uso, y con Bide == 1 es el turno de pegar.
  if (mv < MOVE_COUNT && MOVE_TBL[mv].fx == FX_BIDE) {
    if (att.bideTurns != 1) {
      if (att.bideTurns == 0) { att.bideTurns = 3; att.bideDmg = 0; }
      att.bideTurns--;
      r.charging = true;
      r.bideStore = true;
      r.hits = 0;
      att.lastMove = mv;
      att.lastSlot = slot;
      turn++;
      return r;
    }
    att.bideTurns = 0;
    if (!att.bideDmg) {
      r.failed = true;
      r.hits = 0;
      att.lastFailed = true;
      att.lastMove = mv;
      att.lastSlot = slot;
      turn++;
      return r;
    }
  }
  // COPARAZON: solo sale si esta ronda le entro un golpe FISICO de verdad
  // (al bicho, no al muneco). Si no, "la trampa no funciono".
  if (mv < MOVE_COUNT && MOVE_TBL[mv].fx == FX_SHELLTRAP &&
      !(att.shellTrap && att.tookPhys > 0)) {
    r.failed = true;
    r.hits = 0;
    att.lastFailed = true;
    att.lastMove = mv;
    att.lastSlot = slot;
    turn++;
    return r;
  }
  // DON NATURAL: sin baya falla; con ella, la baya se GASTA ya, pegue o
  // no (el Anil: "the item is consumed even if this move was Protected
  // against or it missed"). El tipo y la potencia se guardan antes.
  gDonTipo = gDonPot = 0;
  if (mv < MOVE_COUNT && MOVE_TBL[mv].fx == FX_NATURALGIFT) {
    if (!donNatural(att, gDonTipo, gDonPot)) {
      r.failed = true;
      r.hits = 0;
      att.lastFailed = true;
      att.lastMove = mv;
      att.lastSlot = slot;
      turn++;
      return r;
    }
    att.lastBerry = att.item;     // Cosecha la puede devolver
    att.perderObjeto(true);
  }

  // MAGNITUD: el nivel sale ya, antes de acertar o de chocar con una
  // Proteccion, y la pantalla lo dice ("Magnitude 7!") pase lo que pase.
  gMagnitud = 0;
  if (mv < MOVE_COUNT && MOVE_TBL[mv].fx == FX_MAGNITUDE)
    r.magnitud = gMagnitud = magnitudAlAzar();
  const uint8_t magnitud = gMagnitud;

  // Lo que hizo la carga: el `r = h` del primer impacto lo pisa, asi que
  // se guarda aparte y se devuelve despues del bucle de impactos.
  bool cargaRapida = false;
  uint8_t cargaSt = 0xFF;
  if (mv < MOVE_COUNT && (MOVE_TBL[mv].fx == FX_CHARGE ||
                          MOVE_TBL[mv].fx == FX_CHARGE_DIG ||
                          MOVE_TBL[mv].fx == FX_CHARGE_FLY ||
                          MOVE_TBL[mv].fx == FX_CHARGE_DIVE ||
                          MOVE_TBL[mv].fx == FX_CHARGE_GONE)) {
    // Con el sol pegando, Solar Beam no necesita recoger nada: sale directo.
    // Esta es la otra mitad de para que sirve Sunny Day, y es la razon de que
    // un golpe que no hace dano pueda ganar un combate.
    bool alSol = (weather == WE_SUN && MOVE_TBL[mv].type == T_GRASS);
    // ELECTROSHOT es el mismo trato con la lluvia
    // (TwoTurnAttackOneTurnInRainRaiseUserSpAtk1 del Anil).
    bool aLluvia = (weather == WE_RAIN && !strcmp(MOVE_TBL[mv].name, "ELECTROSHOT"));
    if (!soltando) {
      // LO QUE HACE LA CARGA (pbChargingTurnEffect del Anil): Cabezazo
      // sube la defensa y Rayo Meteorico y Electrorrayo el especial EN EL
      // TURNO DE CARGA, asi que el golpe ya sale con la subida —— y la
      // subida queda aunque luego falle. Antes iba como secundario, DESPUES
      // del dano y solo si entraba: el golpe nunca la aprovechaba.
      // Tambien vale en la carga rapida (sol, lluvia): pbQuickChargingMove
      // llama al mismo pbChargingTurnEffect.
      const MoveEntry &cm = MOVE_TBL[mv];
      if (cm.power > 0 && (cm.sx == SX_RAISE_DEF || cm.sx == SX_RAISE_SPA)) {
        uint8_t w = (cm.sx == SX_RAISE_DEF) ? SG_DEF : SG_SPA;
        if (att.applyStage(w, +1)) {
          r.stageWhich = w; r.stageDelta = +1; r.stageSelf = true;
          cargaSt = w;
        }
      }
    }
    // Sol o lluvia: carga Y suelta en el mismo turno. En el Anil sale la
    // frase de la carga y su animacion, y despues el golpe; la pantalla
    // lo sabe por esto.
    if (!soltando && (alSol || aLluvia)) r.cargaRapida = cargaRapida = true;
    if (!soltando && !alSol && !aLluvia) {
      // Primer turno: se recoge la energia y no pega nadie. Excavar, Vuelo
      // y Buceo ademas se QUITAN DE EN MEDIO, que es lo que los distingue
      // de Rayo Solar y lo que hacia falta para que Terremoto y Tornado
      // tuvieran a quien pegar el doble.
      att.chargingMove = mv;
      att.hiding = MOVE_TBL[mv].fx == FX_CHARGE_DIG  ? HID_UNDER
                 : MOVE_TBL[mv].fx == FX_CHARGE_FLY  ? HID_SKY
                 : MOVE_TBL[mv].fx == FX_CHARGE_DIVE ? HID_WATER
                 : MOVE_TBL[mv].fx == FX_CHARGE_GONE ? HID_GONE
                                                     : HID_NONE;
      r.charging = true;
      r.hits = 0;
      turn++;
      return r;
    }
    att.chargingMove = 0xFFFF;   // suelta y queda libre otra vez
    att.hiding = HID_NONE;       // y vuelve a estar donde se le ve
  }

  // Precision: se tira contra acc del golpe, ajustada por el stage de punteria
  // del que pega. Hasta ahora esto no existia y no fallaba ni un golpe.
  // Con el cielo cerrado hay golpes que no fallan: Thunder bajo la lluvia y
  // Blizzard con granizo. Es otra razon para gastar un turno en el clima.
  bool nuncaFalla = mv < MOVE_COUNT &&
                    ((weather == WE_RAIN && MOVE_TBL[mv].type == T_ELEC &&
                      MOVE_TBL[mv].acc <= 70) ||
                     (weather == WE_HAIL && MOVE_TBL[mv].type == T_ICE &&
                      MOVE_TBL[mv].acc <= 70));
  // TOXICO echado por un VENENO no falla nunca. Otro golpe que cambia
  // segun quien lo usa: en el Anil es `pbOverrideSuccessCheckPerHit`
  // de BadPoisonTarget (030_Move/008_MoveEffects_BattlerOther.rb:138),
  // y pide las tres cosas —— MORE_TYPE_EFFECTS (gen >= 6: el Anil es
  // 9), que sea el golpe de ESTADO y que el que lo tira sea veneno.
  //
  // Por eso se mira `power == 0` y no solo el efecto: Colmillo Veneno
  // y Cadena Virulenta comparten clase y SON de dano, asi que a ellos
  // esta linea no les toca. Con 90 de precision, el Toxico de un
  // veneno fallaba uno de cada diez y no tenia que fallar ninguno.
  if (mv < MOVE_COUNT && MOVE_TBL[mv].power == 0 &&
      MOVE_TBL[mv].sx == SX_BADPOISON &&
      (att.type1() == T_POISON || att.type2() == T_POISON))
    nuncaFalla = true;
  if (att.sureHit) { nuncaFalla = true; att.sureHit = false; }  // Mind Reader
  // No Guard vale para los DOS lados: ni el falla ni le fallan.
  if (att.abil() == AB_NO_GUARD || def.abil() == AB_NO_GUARD) nuncaFalla = true;
  // A precisao final, para os golpes que a tiram A CADA impacto (Triple
  // Kick, Bomba Demografica). 100 = nao se tira (nunca falha ou acc 0).
  uint32_t pAcierto = 100;
  if (!nuncaFalla && mv < MOVE_COUNT && MOVE_TBL[mv].acc > 0) {
    // La evasion del rival juega en contra de la punteria del que pega: es lo
    // que hace que Minimize y Double Team sirvan para algo. Se multiplica al
    // reves (mas evasion = menos probabilidad) usando la misma tabla.
    uint32_t p = (uint32_t)MOVE_TBL[mv].acc * stageMul(att.stage[SG_ACC]) / 100;
    // Los que ignoran los stages del rival ignoran TAMBIEN su evasion: es
    // la mitad de la gracia de Espada Santa.
    // La evasion no cuenta si el golpe la ignora (Espada Santa) o si le
    // han rastreado. Y se ignora ENTERA, no solo lo que se haya subido:
    // antes exigia `> 0`, asi que contra un alvo con la evasion BAJADA el
    // golpe se aprovechaba de la bajada. Seis lineas mas abajo, el mismo
    // concepto para Unaware ya estaba sin signo.
    int8_t ev = (MOVE_TBL[mv].fx == FX_IGNORE_STAGES || def.foreseen ||
                 def.mirado)
                ? 0 : def.stage[SG_EVA];
    p = p * 100 / (stageMul(ev) ? stageMul(ev) : 100);
    // ESTRELLA VICTORIA: un 10% mas de punteria para el.
    if (att.abil() == AB_ACC_UP) p = p * (100 + att.abilArg()) / 100;
    // PIEL MILAGRO: los golpes de ESTADO le llegan a la mitad. Lo que
    // pega no se toca, que es lo que la hace una habilidad de aguante
    // y no un escudo general.
    if (def.abil() == AB_WONDER_SKIN && MOVE_TBL[mv].power == 0) p /= 2;
    if (att.abil() == AB_COMPOUND_EYES) p = p * (100 + att.abilArg()) / 100;
    // Hustle: el precio de pegar un 50% mas fuerte es apuntar un 20%
    // peor, y solo en los fisicos, que son los que se benefician.
    if (att.abil() == AB_HUSTLE && MOVE_TBL[mv].phys) p = p * 80 / 100;
    // Sand Veil y Snow Cloak: en su clima, cuesta acertarles.
    if (def.abil() == AB_EVA_WEATHER && weather == def.abilArg()) p = p * 4 / 5;
    if (p > 100) p = 100;
    pAcierto = p;
    if ((uint32_t)btRand(100) >= p) {
      r.missed = true;
      r.hits = 0;      // fallar no es 'pegar una vez y hacer cero'
      att.lastFailed = true;
      // Y aqui se rompe la cadena. ESTA LINEA FALTABA: el bloque del
      // final de useMove no se alcanza nunca por este camino, asi que
      // un Rodar fallado seguia atando y un Corte Furia fallado
      // conservaba el multiplicador.
      rompeCadena(from, mv, r);
      // Hi Jump Kick: si fallas te estrellas. Es lo que hace que valga la pena
      // arriesgarse con un golpe de 130 y 90 de precision.
      seEstrella();
      turn++;
      return r;
    }
  }

  // ESCONDIDO: el que esta cavando, volando o buceando no esta ahi. Solo
  // le alcanzan los pocos golpes que llegan hasta su escondite, y a esos
  // les pega el doble (eso va en la potencia). Va DESPUES de la precision
  // y ANTES del Protect porque no es puntería ni es escudo: es que no
  // tiene a quien pegar.
  //
  // Los golpes que se lanza a si mismo no cuentan: subirse el ataque
  // mientras el otro vuela tiene que seguir valiendo. Los de ESTADO que van
  // AL RIVAL si: un Toxico o una Onda Trueno al que esta bajo tierra
  // fallan igual que un placaje. En el PBS los de uno mismo (y los de
  // campo) tienen precision 0 y los que apuntan al rival no; Rugido y
  // Remolino tambien tienen 0, pero sin nadie a quien soplar fallan.
  if (def.hiding != HID_NONE && mv < MOVE_COUNT &&
      (MOVE_TBL[mv].power > 0 || MOVE_TBL[mv].acc > 0 ||
       MOVE_TBL[mv].fx == FX_ROAR)) {
    uint8_t fx = MOVE_TBL[mv].fx, ty = MOVE_TBL[mv].type;
    // Fisura es la otra que llega al agujero (como Terremoto, pero sin el
    // doble) y Torbellino la otra que llega al fondo del agua.
    bool alcanza =
        (def.hiding == HID_UNDER && (fx == FX_X2_IF_UNDER || fx == FX_MAGNITUDE ||
                                     (fx == FX_OHKO && ty == T_GROUND))) ||
        (def.hiding == HID_SKY   && (fx == FX_X2_IF_SKY || fx == FX_HITS_SKY ||
                                     fx == FX_SMACK_DOWN)) ||
        (def.hiding == HID_WATER && (fx == FX_X2_IF_WATER ||
                                     (fx == FX_BIND && ty == T_WATER)));
    // No Guard tampoco falla contra un escondite: es toda la habilidad.
    if (att.abil() == AB_NO_GUARD || def.abil() == AB_NO_GUARD) alcanza = true;
    if (!alcanza) {
      r.missed = true;
      r.hits = 0;
      att.lastFailed = true;
      seEstrella();             // saltar contra quien no esta tambien duele
      att.lastMove = mv;
      att.lastSlot = slot;
      turn++;
      return r;
    }
  }

  // ESPEJO MAGICO: los golpes de ESTADO se le vuelven al que los
  // lanzo. Se resuelve lanzandolo del otro lado con el golpe forzado,
  // la misma pieza que usa Bailarin, y con el mismo guardia para que
  // dos Espejos no se lo devuelvan el uno al otro sin parar.
  if (!dancing && def.abil() == AB_MAGIC_BOUNCE && mv < MOVE_COUNT &&
      MOVE_TBL[mv].power == 0 && !def.fainted()) {
    r.failed = true;
    r.hits = 0;
    r.abilFired = def.ability;
    r.abilOfDef = true;
    att.lastFailed = true;
    dancing = true;
    useMove((uint8_t)(from ^ 1), 0, mv);
    dancing = false;
    turn++;
    return r;
  }

  // CUERPO AUREO: los golpes de ESTADO del rival no le hacen nada.
  // Se mira junto al Protect porque es lo mismo: el golpe no llega.
  if (def.abil() == AB_GOOD_AS_GOLD && mv < MOVE_COUNT &&
      MOVE_TBL[mv].power == 0 && from != (uint8_t)(from ^ 1)) {
    r.failed = true;
    r.hits = 0;
    r.abilFired = def.ability;
    r.abilOfDef = true;
    att.lastFailed = true;
    turn++;
    return r;
  }

  // Protect del rival: se come el golpe entero. Feint lo atraviesa, y los
  // golpes que no tocan al rival (Swords Dance, Sunny Day) le dan igual.
  // PUÑO INVISIBLE: sus golpes de CONTACTO atraviesan el Protect, igual
  // que Finta pero sin gastar un golpe en ello.
  //
  // Aqui habia `power > 0`, que es un atajo equivocado para "toca al
  // rival": dejaba pasar Onda Trueno, Toxico, Rayo Confuso, Espora,
  // Drenadoras, Mofa, Encore, Trapicheo y todas las bajadas de stats.
  // Lo decide tocaAlRival(), abajo.
  if (def.protecting && mv < MOVE_COUNT && tocaAlRival(mv) &&
      MOVE_TBL[mv].fx != FX_FEINT &&
      !(att.abil() == AB_UNSEEN_FIST && moveHas(mv, MV_CONTACT))) {
    r.blockedByProtect = true;
    r.hits = 0;
    // Que se lo paren NO es fallar: el Anil lo salta al decidirlo
    // (169_Battler_UseMove.rb, `next if t.damageState.protected`). Pero
    // quien salto con una Patada Salto Alta se estrella igual.
    seEstrella();
    att.lastMove = mv;
    att.lastSlot = slot;
    turn++;
    return r;
  }
  if (mv < MOVE_COUNT && MOVE_TBL[mv].fx == FX_FEINT) def.protecting = false;
  // SUSTITUTO: un golpe de APOYO contra el rival le da al muneco y no
  // hace nada. Los de sonido lo atraviesan, e Infiltracion tambien.
  if (def.subHp && mv < MOVE_COUNT && MOVE_TBL[mv].power == 0 &&
      tocaAlRival(mv) && !atraviesaMuneco(mv, att)) {
    r.failed = true;
    r.hits = 0;
    att.lastFailed = true;
    att.lastMove = mv;
    att.lastSlot = slot;
    turn++;
    return r;
  }
  if (mv < MOVE_COUNT && MOVE_TBL[mv].fx == FX_BREAK_SCREENS) {
    // Brick Break rompe antes de pegar. Al LADO del rival, que es donde
    // viven de verdad: borrarlas solo en el bicho las devolvia al relevar.
    pantalla[from ^ 1][PA_REFLECT] = 0;
    pantalla[from ^ 1][PA_LIGHT] = 0;
    espejaPantallas(from ^ 1);
  }

  // Multigolpe: se tira cada impacto aparte para que el azar y el critico
  // cuenten por separado, como en los juegos.
  uint8_t golpes = 1;
  if (mv < MOVE_COUNT) {
    uint8_t fx = MOVE_TBL[mv].fx;
    if (fx == FX_MULTI_2) golpes = 2;
    else if (fx == FX_MULTI_3 || fx == FX_MULTI_3_UP) golpes = 3;
    else if (fx == FX_MULTI_10) golpes = 10;   // Bomba Demografica
    else if (fx == FX_BEATUP) golpes = equipoN[from] ? equipoN[from] : 1;
    else if (fx == FX_MULTI_2_5) {
      // 2 y 3 son mucho mas probables que 4 y 5: la tabla del Anil
      // (HitTwoToFiveTimes) es 7, 7, 3 y 3 de 20 = 35/35/15/15%. Antes
      // era de 8 (37,5/37,5/12,5/12,5).
      long t = btRand(20);
      golpes = (t < 7) ? 2 : (t < 14) ? 3 : (t < 17) ? 4 : 5;
      // ENLACE: siempre los cinco. Se tira el dado igual para no
      // mover la secuencia de azar de quien NO la tiene.
      if (att.abil() == AB_SKILL_LINK) golpes = 5;
    }
  }
  // VINCULO FAMILIAR: los golpes de un solo impacto pegan DOS veces, y
  // la segunda a un cuarto (el recorte va en el bucle de abajo, que es
  // quien suma los impactos). Va FUERA del if de arriba para no tocar la
  // cadena de los multigolpe, que ya estaba bien.
  if (golpes == 1 && mv < MOVE_COUNT && MOVE_TBL[mv].power > 0 &&
      att.abil() == AB_PARENTAL) {
    golpes = 2;
    r.abilFired = att.ability;
  }

  // Absorciones: el golpe no le hace nada Y encima le viene bien. Se resuelve
  // antes del bucle de dano porque no hay dano que repartir.
  //
  // Y SOLO si sigue en pie. Sin el `!def.fainted()`, un bicho ya tumbado
  // que recibiera un golpe de su tipo se curaba un cuarto del maximo:
  // resucitaba. Lo encontro el gauntlet ("un golpe le subio la vida al
  // rival: de 0 a 6", que es hpMax/4 de uno de 24 PS) despues de que un
  // cambio en el retroceso moviera las tiradas y lo llevara por ahi.
  // Llevaba ahi desde siempre; lo nuevo fue el camino, no el fallo.
  if (mv < MOVE_COUNT && MOVE_TBL[mv].power > 0 && !def.fainted()) {
    uint8_t ab = def.abil();
    bool esSuTipo = MOVE_TBL[mv].type == def.abilArg();
    if (esSuTipo && (ab == AB_ABSORB_HEAL || ab == AB_ABSORB_BOOST ||
                     ab == AB_ABSORB_SPEED || ab == AB_ABSORB_ATK ||
                     ab == AB_STEAM_VEIL)) {
      r.effect = 0;
      r.hits = 0;
      r.abilFired = def.ability;
      r.abilOfDef = true;
      if (ab == AB_ABSORB_HEAL) {
        uint16_t cura = def.hpMax / 4 ? def.hpMax / 4 : 1;
        uint16_t hueco = (uint16_t)(def.hpMax - def.hp);
        def.hp += (cura > hueco ? hueco : cura);
      } else {
        // VELO VAPOR: el agua se le evapora encima y la nube le tapa.
        def.applyStage(ab == AB_ABSORB_BOOST ? SG_SPA
                     : ab == AB_ABSORB_SPEED ? SG_SPE
                     : ab == AB_STEAM_VEIL   ? SG_EVA : SG_ATK, +1);
      }
      att.lastFailed = true;      // no le afecto: para Pataleta es un fallo
      seEstrella();
      att.lastMove = mv;
      att.lastSlot = slot;
      turn++;
      return r;
    }
  }

  uint32_t total = 0;
  uint8_t hechos = 0;
  // Triple Kick, Triple Hit e Bomba Demografica tiram a precisao a cada
  // impacto (successCheckPerHit?); falhar um corta os que faltam. Com
  // Skill Link nao se tira (@accCheckPerHit = !SKILLLINK).
  bool porImpacto = mv < MOVE_COUNT &&
                    (MOVE_TBL[mv].fx == FX_MULTI_3_UP || MOVE_TBL[mv].fx == FX_MULTI_10) &&
                    att.abil() != AB_SKILL_LINK;
  for (uint8_t i = 0; i < golpes; i++) {
    if (i > 0 && porImpacto && pAcierto < 100 && (uint32_t)btRand(100) >= pAcierto) break;
    gImpacto = (uint8_t)(i + 1);
    // PALIZA: cada impacto es de uno del equipo, con SU ataque base.
    gPalizaPot = (mv < MOVE_COUNT && MOVE_TBL[mv].fx == FX_BEATUP && equipoN[from])
                     ? (uint8_t)(5 + equipoAtk[from][i] / 10) : 0;
    HitResult h = computeHit(att, def, mv, weather, terrain,
                             movedThisTurn[from ^ 1], sportMask(), lastFusion,
                             normalEsElec);
    if (i == 0) r = h;                 // efectividad y critico del primero
    // El segundo zarpazo del Vinculo Familiar vale un cuarto.
    if (i > 0 && att.abil() == AB_PARENTAL) h.damage = (uint16_t)(h.damage / 4);
    total += h.damage;
    if (h.effect == 0) { total = 0; break; }
    hechos = (uint8_t)(i + 1);
    if (i < HIT_MAX) r.hitDmg[i] = h.damage;   // (el `r = h` de arriba lo deja a cero)
    if (total >= def.hp) break;        // ya cayo: no se siguen tirando impactos
  }
  // Los impactos que de verdad ENTRARON (realNumHits del Anil), no los que
  // estaban previstos: si cae al segundo de cinco, "Hit 2 times!".
  r.hits = hechos;
  if (!r.noDamage && r.effect == 0 && !r.failed) seEstrella();
  r.damage = (uint16_t)(total > 65535 ? 65535 : total);
  gPalizaPot = 0;
  gImpacto = 0;
  gDonTipo = gDonPot = 0;
  gMagnitud = 0;
  // (despues del bucle: `r = h` del primer impacto pisaba la marca)
  if (cargaRapida) r.cargaRapida = true;
  if (magnitud) r.magnitud = magnitud;
  if (cargaSt != 0xFF) {
    r.stageWhich = cargaSt; r.stageDelta = +1; r.stageSelf = true;
  }
  if (mv < MOVE_COUNT && MOVE_TBL[mv].fx == FX_BIDE) {
    att.bideDmg = 0;
    r.bideRelease = true;
  }

  if (r.failed) {
    // Counter sin nada que devolver, Endeavor contra alguien mas tocado que tu
    r.hits = 0;
    r.damage = 0;
    att.lastFailed = true;
    att.lastMove = mv;
    att.lastSlot = slot;
    att.sureCrit = false;
    turn++;
    return r;
  }

  // DISFRAZ: el primer golpe que recibe se lo come la cascara, y le cuesta
  // un octavo pase lo que pase. Va AQUI, ANTES de la cadena que decide si
  // el golpe mata, porque el disfraz cambia el dano y por tanto cambia si
  // mata o no. Mi primera version lo metio DENTRO de la rama del golpe
  // mortal, donde solo llegaba cuando ya era tarde; lo canto su test.
  // CARA DE HIELO (Eiscue): el primer golpe FISICO se lo come el hielo
  // entero, sin octavo, y la cara se rompe (Move_Usage del Anil).
  if (!r.noDamage && r.damage && def.dex == 875 && !def.transformed &&
      def.abil() == AB_ICE_FACE && def.bform < 0 && mv < MOVE_COUNT &&
      MOVE_TBL[mv].phys) {
    r.damage = 0;
    ponForma((uint8_t)(from ^ 1), 1);
    r.abilFired = def.ability;
    r.abilOfDef = true;
  }
  if (!r.noDamage && r.damage && def.dex == 778 && !def.transformed &&
      def.abil() == AB_DISGUISE && def.disguised) {
    def.disguised = false;
    ponForma((uint8_t)(from ^ 1), 1);
    r.damage = (uint16_t)(def.hpMax / 8 ? def.hpMax / 8 : 1);
    if (r.damage >= def.hp) r.damage = (uint16_t)(def.hp - 1);
    r.abilFired = def.ability;
    r.abilOfDef = true;
  }

  // Lo que le quedaba al rival ANTES de cobrarle. El retroceso y el
  // drenaje son fracciones del dano HECHO, y el dano hecho no puede pasar
  // de esto —— pero `r.damage` se queda con la tirada entera porque hay
  // media docena de efectos mas que la leen y que no son cosa de este
  // arreglo (probado: recortarla ahi pone rojo el gauntlet).
  uint16_t vidaRivalAntes = def.hp;

  // SUSTITUTO: el dano va al muneco, que aguanta hasta sus PS y se rompe.
  // Lo que se apunta es lo que se llevo el muneco: el drenaje y el
  // retroceso salen de ahi, como en los juegos. El sonido y la
  // Infiltracion lo atraviesan.
  bool alMuneco = !r.noDamage && r.damage && def.subHp && mv < MOVE_COUNT &&
                  !moveHas(mv, MV_SOUND) && att.abil() != AB_INFILTRATOR;
  if (alMuneco) {
    if (r.damage > def.subHp) r.damage = def.subHp;
    def.subHp = (uint16_t)(def.subHp - r.damage);
    r.subHit = true;
    r.subBroke = (def.subHp == 0);
  }

  if (r.noDamage) {
    // Golpe de apoyo: no toca al rival. Se salta el bloque de dano entero y
    // baja directo a los efectos, que es donde esta todo lo que hace.
    r.hits = 0;
  } else if (alMuneco) {
    // ya cobrado arriba: el bicho de detras no pierde nada
  } else if (r.damage >= def.hp) {
    // Focus Sash: estando ENTERO aguanta un golpe mortal. Solo desde el maximo
    // -si no, seria una vida extra en cada intercambio- y se gasta.
    if (def.hp == def.hpMax &&
        (objeto(def).fx == IT_FOCUS_SASH || def.abil() == AB_STURDY)) {
      r.damage = (uint16_t)(def.hp - 1);
      def.hp = 1;
      r.heldOn = true;
      // Sturdy es habilidad y no se gasta; la banda si
      if (objeto(def).fx == IT_FOCUS_SASH) {
        r.itemUsed = def.item;
        def.perderObjeto(true);
      } else {
        r.abilFired = def.ability;
        r.abilOfDef = true;
      }
    } else if (def.enduring && def.hp > 0) {
      // Endure: aguanta con uno pase lo que pase. Es lo que convierte un
      // turno perdido en una segunda oportunidad.
      r.damage = (uint16_t)(def.hp - 1);
      def.hp = 1;
    } else {
      // El dano que se APUNTA es el que de verdad se hizo, no la tirada.
      // Los dos caminos de arriba (Focus Sash, Endure) ya lo recortaban y
      // este no, y de ahi salia el bicho que se mataba solo: un Flare
      // Blitz de Arcanine que tira 300 contra un Bellsprout de 25 PS
      // cobraba 300/3 = 100 de retroceso en vez de 25/3 = 9.
      //
      // Es lo que hacen los juegos y Essentials: el retroceso, el drenaje,
      // `tookPhys`/`tookSpec` (Contraataque, Manto Espejo, Alud,
      // Represalia) y el numero que se ensena son fracciones del dano
      // INFLIGIDO, y ese no puede pasar de los PS que le quedaban.
      def.hp = 0;
      knockOut(from ^ 1);
      // LAZO DESTINO: si el que cae lo tenia puesto, se lleva por delante
      // al que le tumbo. Es TODO el golpe, y antes no existia: el caso
      // unico del SELF_KO le mataba al usarlo y ya esta.
      if (def.destinyBond && !att.fainted()) {
        att.hp = 0;
        knockOut(from);
        r.sideFx = FX_DESTINY_BOND;
      }
      // RENCOR: el golpe que le tumbo se queda sin PP. Tambien era todo
      // el golpe, y tambien faltaba entero.
      if (def.grudge && slot < BT_MOVES && mv != MOVE_STRUGGLE) {
        att.usesLeft[slot] = 0;
        r.sideFx = FX_GRUDGE;
      }
    }
  } else {
    def.hp -= r.damage;
  }

  // PICO CANON: el que le toca con un golpe de CONTACTO mientras calienta
  // el pico se quema (pbCanBurn: los que no pueden, no).
  if (r.damage && !r.subHit && mv < MOVE_COUNT && def.beakBlast &&
      moveHas(mv, MV_CONTACT) && !att.fainted() &&
      att.applyStatus(ST_BURN, terrain, weather))
    r.beakBurn = true;
  // DIA DE PAGO: en el Anil suma 5 x nivel al premio. Aqui no hay
  // dinero; queda la frase, que es lo que se ve.
  if (r.damage && mv < MOVE_COUNT && MOVE_TBL[mv].fx == FX_PAYDAY) r.coins = true;
  // LOS COLMILLOS: BurnFlinchTarget y compania tiran DOS dados con la misma
  // probabilidad, uno para el estado (el sx) y otro para amedrentar.
  if (r.damage && !r.subHit && mv < MOVE_COUNT && MOVE_TBL[mv].fx == FX_FANG &&
      !def.fainted() && att.abil() != AB_SHEER_FORCE &&
      def.abil() != AB_NO_SECONDARY) {
    uint16_t prob = MOVE_TBL[mv].chance;
    if (att.abil() == AB_SERENE_GRACE) prob = (uint16_t)(prob * 2);
    if ((uint32_t)btRand(100) < prob && !movedThisTurn[from ^ 1] &&
        def.abil() != AB_NO_FLINCH) {
      def.flinched = true;
      if (def.abil() == AB_STEADFAST) def.applyStage(SG_SPE, +1);
    }
  }

  // PODER SECRETO: 30% de un efecto que depende del SUELO y, sin
  // terreno, del ENTORNO —— el escenario del combate (Battle::entorno, lo
  // pone la pantalla). La tabla es la de EffectDependsOnEnvironment del
  // Anil. Una vez por golpe, no por impacto, y no a traves del muneco ni
  // con Potencia Bruta.
  if (r.damage && !r.subHit && mv < MOVE_COUNT &&
      MOVE_TBL[mv].fx == FX_SECRETPOWER && !def.fainted() &&
      att.abil() != AB_SHEER_FORCE && def.abil() != AB_NO_SECONDARY) {
    uint16_t prob = MOVE_TBL[mv].chance;
    if (att.abil() == AB_SERENE_GRACE) prob = (uint16_t)(prob * 2);
    if ((uint32_t)btRand(100) < prob) {
      uint8_t e = terrain == TR_ELEC  ? EN_NONE
                : terrain == TR_GRASS ? EN_GRASS
                : terrain == TR_MISTY ? EN_MISTY
                : terrain == TR_PSY   ? EN_PSY
                                      : entorno;
      switch (e) {
        case EN_GRASS:      // Latigo Cepa: dormir
          if (def.applyStatus(ST_SLEEP, terrain, weather)) r.gotStatus = ST_SLEEP;
          break;
        case EN_MISTY:      // Viento Feerico: -1 At. Esp.
          if (def.applyStage(SG_SPA, -1)) r.stageWhich = SG_SPA;
          break;
        case EN_PSY:        // Confusion: -1 Velocidad
          if (def.applyStage(SG_SPE, -1)) r.stageWhich = SG_SPE;
          break;
        case EN_WATER:      // Hidropulso: -1 Ataque
          if (def.applyStage(SG_ATK, -1)) r.stageWhich = SG_ATK;
          break;
        case EN_SAND:       // Bofeton Lodo: -1 precision (arena y roca)
          if (def.applyStage(SG_ACC, -1)) r.stageWhich = SG_ACC;
          break;
        case EN_SNOW:       // Canto Helado: congelar
          if (def.applyStatus(ST_FREEZE, terrain, weather)) r.gotStatus = ST_FREEZE;
          break;
        case EN_VOLCANO:    // Calcinacion: quemar
          if (def.applyStatus(ST_BURN, terrain, weather)) r.gotStatus = ST_BURN;
          break;
        case EN_CAVE:       // Lanzarrocas: amedrentar, si aun no se movio
          if (!movedThisTurn[from ^ 1] && def.abil() != AB_NO_FLINCH)
            def.flinched = true;
          break;
        default:            // Golpe Cuerpo: paralizar
          if (def.applyStatus(ST_PARALYSIS, terrain, weather)) r.gotStatus = ST_PARALYSIS;
          break;
      }
    }
  }

  // EL PAR FUSION, apuntado al salir. Se borra al cerrar el turno
  // (endTurn), asi que solo vale para el golpe siguiente —— que es lo que
  // dice la ficha: "si se usa justo despues de".
  if (mv < MOVE_COUNT) {
    uint8_t fxf = MOVE_TBL[mv].fx;
    if (fxf == FX_AFTER_FLARE) lastFusion = 1;        // este ERA el rayo
    else if (fxf == FX_AFTER_BOLT) lastFusion = 2;    // y este la llama
  }

  // Lo que acaba de recibir, para que pueda devolverlo con Counter. Se guarda
  // separado fisico/especial porque Counter y Mirror Coat miran uno cada uno.
  if (r.damage && mv < MOVE_COUNT && !r.subHit) {
    if (MOVE_TBL[mv].phys) def.tookPhys = r.damage;
    else                   def.tookSpec = r.damage;
    if (def.bideTurns) {
      uint32_t s = (uint32_t)def.bideDmg + r.damage;
      def.bideDmg = (uint16_t)(s > 65535 ? 65535 : s);
    }
    // Y la CUENTA, que es de todo el combate y no solo del turno: es lo
    // que hace crecer a Punorrabia. Topada en 6 porque el golpe ya topa
    // en 350 de potencia (50 + 50*6) y asi el byte no da la vuelta en un
    // combate largo.
    if (def.hitsTaken < 6) def.hitsTaken++;
  }

  // FURIA: al que esta furioso, cobrar le sube el ataque. Va justo aqui,
  // donde se acaba de contar el dano recibido, y no en el bando del que
  // pega: lo que lo dispara es RECIBIR, venga de donde venga.
  if (r.damage && def.raging && !def.fainted()) def.applyStage(SG_ATK, +1);

  // Queda apuntado que ESE hueco ya se uso: lo mira Ultima Baza.
  if (slot < 8 && mv != MOVE_STRUGGLE) att.usedMask |= (uint8_t)(1 << slot);

  // La racha de Eco Voz, que es del LADO y no del bicho: no se pierde
  // al relevar, porque la voz sigue sonando.
  if (mv < MOVE_COUNT && MOVE_TBL[mv].fx == FX_ECHO)
    att.echoRun = (uint8_t)(att.echoRun < 250 ? att.echoRun + 1 : 250);
  else
    att.echoRun = 0;

  // Antiaereo: le baja al suelo, y ahi se queda.
  // RAIZ PROFUNDA, del Anil: lo que drena, drena mas. Se suma un
  // cuarto de lo curado, encima de lo que el golpe ya daba.
  // (RAIZ PROFUNDA estaba AQUI, y aqui `r.healed` todavia vale 0:
  //  quien lo rellena es el drenaje, 450 lineas mas abajo. La
  //  habilidad no hacia nada desde que existe. Se movio con el
  //  drenaje, que es donde hay algo que aumentar.)

  if (r.damage && mv < MOVE_COUNT && MOVE_TBL[mv].fx == FX_SMACK_DOWN)
    def.grounded2 = true;

  // ESPABILA: pega el doble al dormido (eso va en la potencia) y de paso
  // le despierta. Lo segundo es lo que lo hace util de verdad: sirve para
  // sacar a tu propio equipo del apuro de tener delante a un dormilon.
  if (r.damage && mv < MOVE_COUNT && MOVE_TBL[mv].fx == FX_WAKE_SLAP &&
      def.status == ST_SLEEP) {
    def.status = ST_NONE;
    def.statusTurns = 0;
  }

  // COLA DRAGON: pega Y le saca del combate. Contra un salvaje eso
  // acaba el encuentro, igual que Rugido; contra un entrenador no hay a
  // quien sacar, asi que se queda en el golpe (que ya es un golpe).
  //
  // Como no Anil (SwitchOutTargetDamagingMove): o golpe que bateu no
  // MUNECO nao empurra ninguem, e um salvaje de nivel MAIOR que quem
  // pega nao foge. Contra um time COM BANCO (a batalha entre placas) o
  // alvo e arrastado: entra um sorteado, se ele nao se agarrar.
  if (r.damage && !over && mv < MOVE_COUNT &&
      MOVE_TBL[mv].fx == FX_DRAGONTAIL && !def.fainted() && !r.subHit) {
    const uint8_t alvo = (uint8_t)(from ^ 1);
    if (!isTrainer) {
      if (from == BT_PLAYER && def.level <= att.level) {
        over = true;
        fled = true;
        winner = 0xFF;
        r.sideFx = FX_DRAGONTAIL;
      }
    } else if (from <= 1 && temReservas(alvo) && !att.fainted() &&
               !naoSaiEmpurrado(att, def)) {
      arrastado[alvo] = true;
      r.sideFx = FX_DRAGONTAIL;
    }
  }

  // GOLPE MORDAZA: ThroatChop = 2 en el Anil. Vale la pena contra quien vive
  // de los golpes de sonido (Alboroto, Eco Voz, Chirrido).
  if (r.damage && mv < MOVE_COUNT && MOVE_TBL[mv].fx == FX_GAG &&
      def.gagTurns == 0 && !fuerzaBruta(att, mv))
    def.gagTurns = 2;

  // PRESA MAXILAR: muerde y no suelta. Se quedan atrapados LOS DOS, que
  // es lo que lo hace una apuesta y no un golpe gratis.
  if (r.damage && mv < MOVE_COUNT && MOVE_TBL[mv].fx == FX_TRAP_BOTH) {
    def.trapped2 = true;
    att.trapped2 = true;
  }

  // METALASER: le cuesta la mitad de su vida. Se cobra DESPUES del dano
  // para que el golpe llegue a hacerse aunque le deje en los huesos.
  if (r.damage && mv < MOVE_COUNT && MOVE_TBL[mv].fx == FX_COST_HALF) {
    uint16_t peaje = (uint16_t)(att.hpMax / 2 ? att.hpMax / 2 : 1);
    r.recoil = (uint16_t)(r.recoil + peaje);
    att.hp = (peaje >= att.hp) ? 0 : (uint16_t)(att.hp - peaje);
    if (att.hp == 0) knockOut(from);
  }

  // FURIA: a partir de aqui, cada golpe que reciba le sube el ataque.
  // El +1 no se lo da el golpe: se lo dan los golpes del RIVAL, y eso
  // se mira donde se cobra el dano (ver mas abajo, en `raging`).
  if (r.damage && mv < MOVE_COUNT && MOVE_TBL[mv].fx == FX_RAGE)
    att.raging = true;

  // Ida y Vuelta: pega y se cambia. El motor no saca a nadie — eso es
  // cosa de quien maneja el banquillo —, asi que solo lo avisa.
  if (r.damage && !over && mv < MOVE_COUNT &&
      MOVE_TBL[mv].fx == FX_UTURN) {
    if (from == BT_PLAYER) wantsSwitch = true;
    // Com banco, de qualquer lado. Sem ninguem para entrar o Anil fica
    // so com o dano (pbEndOfMoveUsageEffect nao acha substituto).
    if (from <= 1 && temReservas(from)) querTrocar[from] = true;
  }

  // La RACHA de repetir el mismo golpe: la miran Corte Furia y Rodar,
  // que crecen mientras no cambies ni falles. Se lleva la cuenta aqui
  // y no en computeHit porque computeHit no cambia nada.
  if (mv < MOVE_COUNT) {
    uint8_t fxr = MOVE_TBL[mv].fx;
    if ((fxr == FX_RAMP || fxr == FX_ROLLOUT) && r.damage) {
      att.rachaMove = (att.rachaDe == mv && att.rachaMove < 250)
                      ? (uint8_t)(att.rachaMove + 1) : 1;
      att.rachaDe = mv;
    } else {
      att.rachaMove = 0;
      att.rachaDe = 0xFFFF;
    }
  }

  // Los que se bajan algo a si mismos al pegar (Fuerza Bruta).
  if (r.damage && mv < MOVE_COUNT && MOVE_TBL[mv].fx == FX_SELF_ATKDEF) {
    att.applyStage(SG_ATK, -1);
    att.applyStage(SG_DEF, -1);
    r.stageWhich = SG_ATK;
    r.stageDelta = -1;
    r.stageSelf = true;
  }

  // Giro Rapido: se sacude lo que le tenia agarrado y la semilla.
  if (r.damage && mv < MOVE_COUNT && MOVE_TBL[mv].fx == FX_SPIN) {
    att.bindTurns = 0;
    att.leechSeed = false;
    // Y BARRE SU LADO. Esta mitad no existia porque no habia trampas que
    // barrer; con ellas, es la respuesta a quien se dedica a ponerlas.
    rocks[from] = spikes[from] = tspikes[from] = web[from] = 0;
    // E +1 de Velocidade: da 8a geracao em diante (o Anil roda na 9, e o
    // RemoveUserBindingAndEntryHazards dele e um StatUpMove de SPEED 1).
    // Faltava. Isto SIM e efeito adicional: a Potencia Bruta o apaga; a
    // varredura de cima, nao.
    if (!fuerzaBruta(att, mv) && att.applyStage(SG_SPE, +1)) {
      r.stageWhich = SG_SPE;
      r.stageDelta = +1;
      r.stageSelf = true;
    }
  }

  // HACHAZO PETREO: pega Y deja las piedras. Las trampas no son solo de
  // los golpes de apoyo, asi que se mira aqui tambien.
  if (r.damage && mv < MOVE_COUNT && MOVE_TBL[mv].fx == FX_ROCKS)
    rocks[from ^ 1] = 1;

  // CONTROL NUCLEAR: pega Y le anula la habilidad. Igual que arriba: hay
  // golpes que hacen dano Y ponen un efecto de los de apoyo, y mirarlo
  // solo en el `switch` de los de potencia 0 los dejaba a medias.
  if (r.damage && mv < MOVE_COUNT && MOVE_TBL[mv].fx == FX_GASTRO &&
      def.ability != 0)
    def.abilOff = true;

  // Los que van a por la BAYA del rival. Picadura se la come (y le
  // hace efecto, que es la gracia: una Sitrus te cura a ti);
  // Calcinacion la quema sin comersela.
  if (r.damage && !over && mv < MOVE_COUNT && def.item != IT_NONE) {
    uint8_t fxb = MOVE_TBL[mv].fx;
    const ItemEntry &suya = itemAt(def.item);
    bool esBaya = esBayaDeCura(suya) || suya.fx == IT_BERRY_CURE;
    if (esBaya && (fxb == FX_EAT_BERRY || fxb == FX_BURN_BERRY)) {
      if (fxb == FX_EAT_BERRY) {
        if (esBayaDeCura(suya)) {
          uint16_t cura = curaDeBaya(suya, att.hpMax);
          uint16_t hueco = (uint16_t)(att.hpMax - att.hp);
          att.hp += (cura > hueco ? hueco : cura);
          r.healed = (uint16_t)(r.healed + (cura > hueco ? hueco : cura));
        } else if (att.status != ST_NONE) {
          att.status = ST_NONE;
          att.statusTurns = 0;
        }
      }
      r.itemLost = def.item;
      def.perderObjeto(true);     // se la comieron: esa no vuelve
    }
  }

  // Los golpes que mueven OBJETOS, despues de que el dano este hecho.
  // Van aqui y no en computeHit porque computeHit no cambia nada: solo
  // cuenta. Mezclarlo seria que "ver cuanto haria" robara un objeto.
  if (r.damage && !over && mv < MOVE_COUNT) {
    uint8_t fxi = MOVE_TBL[mv].fx;
    // Las reglas son las del Anil (185_MoveEffects_Items.rb): un SALVAJE
    // no quita ni roba; al muneco no se le quita nada; y la piedra mega del
    // que la lleva (o la que seria del que roba) no se toca.
    const bool pegaSalvaje = (from == BT_FOE && !isTrainer);
    const bool recibeSalvaje = (from == BT_PLAYER && !isTrainer);
    if (fxi == FX_KNOCK_OFF && def.item != IT_NONE && !r.subHit &&
        !pegaSalvaje && !objetoIntocable(def, def.item)) {
      // Desarme: se lo tira al suelo. No se lo queda nadie.
      r.itemLost = def.item;
      def.perderObjeto();
    } else if (fxi == FX_STEAL && def.item != IT_NONE && !r.subHit &&
               !pegaSalvaje && !objetoIntocable(def, def.item) &&
               !objetoIntocable(att, def.item) &&
               (att.item == IT_NONE || recibeSalvaje)) {
      // Ladron y Antojo: se lo quedan, pero solo con las manos libres.
      // A un salvaje se le roba SIEMPRE y va derecho a la mochila.
      r.itemStolen = def.item;
      if (recibeSalvaje) r.itemToBag = true;
      else att.item = def.item;
      def.perderObjeto();
    }
  }
  // Lanzamiento gasta lo que tiro, haya hecho dano o no.
  if (mv < MOVE_COUNT && MOVE_TBL[mv].fx == FX_FLING &&
      att.item != IT_NONE) {
    r.itemLost = att.item;
    r.itemLostSelf = true;        // lo tiro EL, no se lo tiraron
    att.perderObjeto(true);
  }

  // RATTLED: los tipos que le dan miedo le hacen salir corriendo, y
  // correr aqui es +1 de velocidad. No hace falta que sea de contacto.
  if (r.damage && !over && mv < MOVE_COUNT &&
      def.abil() == AB_RATTLED &&
      (MOVE_TBL[mv].type == T_BUG || MOVE_TBL[mv].type == T_DARK ||
       MOVE_TBL[mv].type == T_GHOST) &&
      def.applyStage(SG_SPE, +1)) {
    r.abilFired = def.ability;
    r.abilOfDef = true;
  }

  // Weak Armor: al recibir un FISICO, se le descascarilla la coraza.
  // Pierde defensa y gana velocidad; va antes del bloque de contacto
  // porque no pide tocar, solo que el golpe sea fisico.
  if (r.damage && !over && mv < MOVE_COUNT && MOVE_TBL[mv].phys &&
      def.abil() == AB_WEAK_ARMOR) {
    bool algo = def.applyStage(SG_DEF, -1);
    algo = def.applyStage(SG_SPE, +2) || algo;
    if (algo) { r.abilFired = def.ability; r.abilOfDef = true; }
  }

  // Habilidades de CONTACTO. Hasta ahora se aproximaban con "es
  // fisico", que no es lo mismo: Rayo Burbuja es especial y toca,
  // Bomba Lodo es fisico y no. Ahora se mira la bandera de verdad del
  // PBS (MV_CONTACT), que entro con este mismo cambio.
  // Static, Poison Point, Rough Skin: el precio de pegarle de cerca a
  // segun quien.
  // ILUSION: se cae en cuanto le pegan. Volver a `form` 0 es lo que
  // la pantalla mira para dibujarle su cara de verdad.
  if (r.damage && def.illusion) {
    def.illusion = false;
    r.abilFired = def.ability;
    r.abilOfDef = true;
  }

  // TRAGAMISIL: si lleva algo en la boca, al que le toca le escupe y
  // le cuesta un cuarto. Y se lo gasta: es una vez por bocado.
  // Como en el Anil (Battler_UseMoveTriggerEffects): CUALQUIER golpe que
  // le de, no solo el de contacto. Le quita un cuarto al que pego y,
  // segun lo que llevaba: con Arrokuda (forma 1) -1 de defensa, con
  // Pikachu (forma 2) lo paraliza.
  if (r.damage && !over && def.dex == 845 && !def.transformed &&
      def.abil() == AB_GULP && def.gulped) {
    uint8_t bocado = def.bform >= 0 ? BFORM_TBL[def.bform].form : 1;
    def.gulped = false;
    ponForma((uint8_t)(from ^ 1), 0);
    uint16_t escupe = (uint16_t)(att.hpMax / 4 ? att.hpMax / 4 : 1);
    r.recoil = (uint16_t)(r.recoil + escupe);
    att.hp = (escupe >= att.hp) ? 1 : (uint16_t)(att.hp - escupe);
    if (bocado == 2) {
      if (att.applyStatus(ST_PARALYSIS, terrain, weather, uproar > 0))
        r.gotStatus = ST_PARALYSIS;
    } else {
      att.applyStage(SG_DEF, -1);
    }
    r.abilFired = def.ability;
    r.abilOfDef = true;
  }

  // ...y se LLENA la boca con Surf o Buceo, no con cualquier golpe de
  // agua. Va despues para que el bocado de este turno no se escupa en
  // el mismo turno. Con mas de media vida coge un Arrokuda; si no, un
  // Pikachu.
  if (att.dex == 845 && !att.transformed && att.abil() == AB_GULP &&
      !att.gulped && mv < MOVE_COUNT &&
      (!strcmp(MOVE_TBL[mv].name, "SURF") || !strcmp(MOVE_TBL[mv].name, "DIVE"))) {
    att.gulped = true;
    ponForma(from, (uint32_t)att.hp * 2 > att.hpMax ? 1 : 2);
  }

  // MOMIA: al que le toca le pega SU habilidad. ESPIRITU ERRANTE las
  // intercambia. Las dos van aqui, con el contacto, y las dos se
  // miran ANTES de Hedor porque pueden quitarle la habilidad al que
  // pega y entonces Hedor ya no seria suya.
  if (r.damage && !over && moveHas(mv, MV_CONTACT) && !def.fainted()) {
    if (def.abil() == AB_MUMMY && att.ability != def.ability) {
      att.ability = def.ability;
      att.abilOff = false;
      r.abilFired = def.ability;
      r.abilOfDef = true;
    } else if (def.abil() == AB_SPIRIT_SWAP &&
               att.ability != def.ability) {
      uint16_t tmp = att.ability;
      att.ability = def.ability;
      def.ability = tmp;
      r.abilFired = att.ability;
    }
  }

  // CADENA TOXICA: un 30% de ENTOXICAR con cualquier golpe, toque o no.
  if (r.damage && !over && att.abil() == AB_TOXIC_CHAIN &&
      btRand(100) < 30 &&
      def.applyStatus(ST_BADPOISON, terrain, weather, uproar > 0,
                      true)) {
    r.gotStatus = ST_BADPOISON;
    r.abilFired = att.ability;
  }

  // HEDOR: un 10% de amedrentar con CUALQUIER golpe que haga dano.
  // Se mira antes del resto para que no choque con el flinch del
  // propio golpe: si ya se amedrento, no se vuelve a tirar.
  // Y, como el flinch del propio golpe, solo si al rival le queda turno:
  // pegando el segundo no se amedrenta a nadie.
  if (r.damage && !over && att.abil() == AB_STENCH && !movedThisTurn[from ^ 1] &&
      !def.fainted() && !def.flinched && btRand(100) < 10) {
    def.flinched = true;
    r.abilFired = att.ability;
  }

  // PUNTO FURIA: un critico le pone el ataque al MAXIMO. Es todo o
  // nada, y por eso vale la pena en un bicho que aguanta uno.
  if (r.damage && r.crit && !over && def.abil() == AB_ANGER_POINT &&
      !def.fainted() && def.stage[SG_ATK] < 6) {
    def.stage[SG_ATK] = 6;
    r.abilFired = def.ability;
    r.abilOfDef = true;
  }

  // BERSERK: que le bajen de media vida le sube el especial. Se mira
  // AQUI, con los PS ya cobrados, y solo si antes estaba por encima:
  // si no, cada golpe a partir de la mitad se lo subiria otra vez.
  if (r.damage && !over && def.abil() == AB_BERSERK && !def.fainted() &&
      !fuerzaBruta(att, mv) &&
      def.hp * 2 <= def.hpMax && (def.hp + r.damage) * 2 > def.hpMax &&
      def.applyStage(SG_SPA, +1)) {
    r.abilFired = def.ability;
    r.abilOfDef = true;
  }

  // JUSTICIADO: un golpe siniestro le da coraje. No es por contacto,
  // asi que va antes del bloque de abajo.
  if (r.damage && !over && mv < MOVE_COUNT &&
      MOVE_TBL[mv].type == T_DARK && def.abil() == AB_JUSTIFIED &&
      !def.fainted() && def.applyStage(SG_ATK, +1)) {
    r.abilFired = def.ability;
    r.abilOfDef = true;
  }

  // CUERPO MALDITO: un 30% de anularle el golpe que le acaba de dar,
  // cuatro turnos. Reusa el bloqueo de Anulacion, que ya existia.
  if (r.damage && !over && def.abil() == AB_CURSED_BODY &&
      !def.fainted() && att.disableTurns == 0 && slot < BT_MOVES &&
      mv != MOVE_STRUGGLE && btRand(100) < 30) {
    att.disabledSlot = slot;
    att.disableTurns = 3;   // Cuerpo Maldito: Disable = 3 en el Anil
    r.abilFired = def.ability;
    r.abilOfDef = true;
  }

  // CUERPO MORTAL: al que le toca le pone la cuenta atras, y se la
  // pone TAMBIEN a si mismo. Es un trueque, no un castigo gratis.
  if (r.damage && !over && moveHas(mv, MV_CONTACT) &&
      def.abil() == AB_PERISH_BODY && !def.fainted() &&
      att.perish == 0xFF) {
    att.perish = 3;
    def.perish = 3;
    r.abilFired = def.ability;
    r.abilOfDef = true;
  }

  // TERMOCAMBIO: un golpe de FUEGO le sube el ataque (y no se quema,
  // que se mira en applyStatus). No le hace inmune: cobra el golpe.
  if (r.damage && !over && mv < MOVE_COUNT &&
      MOVE_TBL[mv].type == T_FIRE && def.abil() == AB_THERMAL &&
      !def.fainted() && def.applyStage(SG_ATK, +1)) {
    r.abilFired = def.ability;
    r.abilOfDef = true;
  }

  // SURCAVIENTOS: el viento no le entra (eso va en computeHit) y
  // encima le sube el ataque. Como las absorciones, el premio va
  // aqui; el golpe ya se marco inmune arriba.
  if (!over && mv < MOVE_COUNT && moveHas(mv, MV_WIND) &&
      def.abil() == AB_WIND_RIDER && !def.fainted() &&
      def.applyStage(SG_ATK, +1)) {
    r.abilFired = def.ability;
    r.abilOfDef = true;
  }

  // TOQUE: el que TOCA le pone un estado, al reves que Punto Toxico.
  // El estado sale del arg, asi que el Toque Toxico y el Toque Igneo
  // (del Anil) son la misma habilidad con distinto arg.
  if (r.damage && !over && moveHas(mv, MV_CONTACT) &&
      att.abil() == AB_TOUCH_STATUS && btRand(100) < 30 &&
      def.applyStatus(att.abilArg(), terrain, weather, uproar > 0)) {
    r.gotStatus = att.abilArg();
    r.abilFired = att.ability;
  }

  if (r.damage && !over && moveHas(mv, MV_CONTACT)) {
    switch (def.abil()) {
      case AB_CONTACT_STATUS:
        if (btRand(100) < 30 &&
            att.applyStatus(def.abilArg(), terrain, weather, uproar > 0)) {
          r.gotStatus = def.abilArg();   // se lo lleva el que PEGO
          r.abilFired = def.ability;
          r.abilOfDef = true;
        }
        break;
      case AB_CONTACT_CONFUSE:
        if (btRand(100) < 30 && att.confuseTurns == 0) {
          att.confuseTurns = (uint8_t)(2 + btRand(4));
          r.abilFired = def.ability;
          r.abilOfDef = true;
        }
        break;
      case AB_PICKPOCKET:
        // Le mete la mano en el bolsillo al que se le acerco. Solo si
        // el tiene las manos libres y el otro lleva algo: no cambia
        // objetos, los roba.
        // El Anil: un salvaje no hurta, tras un muneco tampoco, y lo
        // intocable (la piedra mega de cualquiera de los dos) no se mueve.
        if (def.item == IT_NONE && att.item != IT_NONE && !r.subHit &&
            !(from == BT_PLAYER && !isTrainer) && !fuerzaBruta(att, mv) &&
            !objetoIntocable(att, att.item) && !objetoIntocable(def, att.item)) {
          def.item = att.item;
          att.perderObjeto();
          r.abilFired = def.ability;
          r.abilOfDef = true;
        }
        break;
      case AB_GOOEY:
        // Se queda pegajoso: al que le toca se le quedan los pies
        // enganchados y pierde velocidad.
        if (att.applyStage(SG_SPE, -1)) {
          r.abilFired = def.ability;
          r.abilOfDef = true;
        }
        break;
      case AB_CONTACT_HURT: {
        uint8_t div = def.abilArg() ? def.abilArg() : 8;
        uint16_t pupa = att.hpMax / div ? att.hpMax / div : 1;
        r.recoil = (uint16_t)(r.recoil + pupa);
        att.hp = (pupa >= att.hp) ? 0 : (uint16_t)(att.hp - pupa);
        if (att.hp == 0) knockOut(from);
        r.abilFired = def.ability;
        r.abilOfDef = true;
        break;
      }
      default:
        break;
    }
  }
  att.sureCrit = false;      // Laser Focus se gasta al primer golpe

  // LAZO DESTINO y RENCOR no tumban a nadie: MARCAN. Lo que marcan pasa
  // solo si el que los uso cae antes de volver a moverse, y se resuelve
  // en el golpe que le tumba. Estaban en el mismo caso que Explosion, o
  // sea que usarlos era suicidarse en el acto y gratis para el rival.
  if (mv < MOVE_COUNT && MOVE_TBL[mv].fx == FX_DESTINY_BOND) {
    att.destinyBond = true;
    r.sideFx = MOVE_TBL[mv].fx;
    turn++;
    return r;
  }
  if (mv < MOVE_COUNT && MOVE_TBL[mv].fx == FX_GRUDGE) {
    att.grudge = true;
    r.sideFx = MOVE_TBL[mv].fx;
    turn++;
    return r;
  }

  // LEGADO: cae, y le deja al rival el ataque y el at.esp por los suelos.
  // Esa segunda mitad es el golpe entero —— caer solo lo hace cualquiera.
  if (mv < MOVE_COUNT && MOVE_TBL[mv].fx == FX_MEMENTO) {
    if (!def.fainted()) {
      def.applyStage(SG_SPA, -2, true);
      if (def.applyStage(SG_ATK, -2, true)) {
        r.stageWhich = SG_ATK;
        r.stageDelta = -2;
      }
    }
    att.hp = 0;
    if (!over) knockOut(from);
    turn++;
    return r;
  }

  // DESEO CURACION y DANZA LUNAR: cae, y el que entre por su lado llega
  // entero. Sin eso son un Explosion sin dano: solo el precio.
  if (mv < MOVE_COUNT && (MOVE_TBL[mv].fx == FX_HEALING_WISH ||
                          MOVE_TBL[mv].fx == FX_LUNAR_DANCE)) {
    deseoCura[from] = (MOVE_TBL[mv].fx == FX_LUNAR_DANCE) ? 2 : 1;
    att.hp = 0;
    if (!over) knockOut(from);
    turn++;
    return r;
  }

  // Explosion y Autodestruccion: el que pega cae, pase lo que pase. Se hace
  // antes que el resto para que no se cure ni nada raro.
  if (mv < MOVE_COUNT && MOVE_TBL[mv].fx == FX_SELF_KO) {
    // DAMP: delante de el nadie se autodestruye. Es una habilidad que
    // no hace dano y aun asi gana combates, porque Explosion es la
    // jugada que no se puede responder.
    if (def.abil() == AB_DAMP) {
      r.failed = true;
      r.abilFired = def.ability;
      r.abilOfDef = true;
      att.lastFailed = true;
      turn++;
      return r;
    }
    att.hp = 0;
    if (!over) knockOut(from);
    else if (def.fainted()) { /* los dos caen: gana el que recibio */ winner = from ^ 1; }
    turn++;
    return r;
  }

  // SACRIFICIO: el dano ya esta hecho (la potencia son sus PS), y ahora
  // cae. Va DESPUES de pegar, al reves que Explosion, porque el dano se
  // calcula con los PS que todavia tiene.
  if (mv < MOVE_COUNT && MOVE_TBL[mv].fx == FX_FINAL_GAMBIT && !r.missed &&
      r.effect > 0) {
    att.hp = 0;
    if (!over) knockOut(from);
  }

  // Drenaje y retroceso: cuentas sobre el dano ya hecho, sin estado nuevo.
  if (r.damage) {
    // EL DANO HECHO, que es sobre lo que se cuentan estas dos cosas en los
    // juegos. Rematar a un Bellsprout de 25 PS con un Flare Blitz que tiro
    // 300 cuesta 25/3, no 300/3 —— que es como el Arcanine del dueno se
    // mataba a si mismo estando entero.
    uint16_t hecho = r.damage > vidaRivalAntes ? vidaRivalAntes : r.damage;
    uint8_t fx = (mv < MOVE_COUNT) ? (uint8_t)MOVE_TBL[mv].fx : (uint8_t)FX_NONE;
    if (fx == FX_DRAIN || fx == FX_DRAIN_3_4 || fx == FX_DRAIN_SLEEP) {
      // Se cura la mitad (o tres cuartos) de lo que se HIZO, no de la
      // tirada: contra un bicho de 19 PS, un Gigadrenado que saca 608 cura
      // 10 y no 269. Es la misma regla que el retroceso.
      //
      // Aqui habia un aviso que decia que recortarlo ponia rojo el
      // gauntlet y que no se sabia por que. Ya se sabe: el gauntlet es
      // determinista por semilla, y cambiar cuanto cura el drenaje cambia
      // el numero de tiradas de random(), lo que desvia TODO el flujo y
      // lleva la simulacion por un camino que ya existia —— y ese camino
      // destapaba OTRO fallo, las absorciones curando a un tumbado. Ese
      // ya esta arreglado, y con el puesto esto pasa verde. El gauntlet
      // tenia razon; el fallo era otro.
      uint16_t cura = (fx == FX_DRAIN_3_4) ? (uint16_t)((hecho * 3 + 3) / 4)
                                           : (uint16_t)((hecho + 1) / 2);
      uint16_t hueco = (uint16_t)(att.hpMax - att.hp);
      r.healed = att.healBlockTurns ? 0 : (cura > hueco ? hueco : cura);
      att.hp += r.healed;
      // RAIZ PROFUNDA: un cuarto mas de lo drenado. Estaba 450 lineas
      // mas ARRIBA, leyendo un `r.healed` que todavia valia 0 —— la
      // habilidad no hizo nada desde que existe, y el test que la
      // vigilaba comparaba una tirada contra otra ("36 contra 35"), o
      // sea ruido. Sumando 40 semillas salio 1547 contra 1547.
      if (att.abil() == AB_DEEP_ROOT && r.healed) {
        uint16_t extra = (uint16_t)(r.healed / 4 ? r.healed / 4 : 1);
        uint16_t sitio = (uint16_t)(att.hpMax - att.hp);
        if (extra > sitio) extra = sitio;
        att.hp = (uint16_t)(att.hp + extra);
        r.healed = (uint16_t)(r.healed + extra);
      }
    } else if (att.abil() == AB_NO_RECOIL && mv != MOVE_STRUGGLE &&
               (fx == FX_RECOIL_2 || fx == FX_RECOIL_3 ||
                fx == FX_RECOIL_4 || fx == FX_RECOIL_HP)) {
      // Rock Head: el retroceso no le hace nada, y por eso un
      // Doble Filo suyo es gratis. Forcejeo SI le duele: eso no es
      // retroceso, es darse de cabezazos por no tener otra cosa.
    } else if (fx == FX_RECOIL_2 || fx == FX_RECOIL_3 || fx == FX_RECOIL_4 ||
               fx == FX_RECOIL_HP || mv == MOVE_STRUGGLE) {
      // Forcejear tambien se paga, para que no sea una salida gratis cuando se
      // acaban los usos. Se redondea hacia arriba: un golpe con retroceso
      // siempre duele algo.
      //
      // Pero el Forcejeo NO cobra sobre el dano: desde la 4a generacion
      // cuesta un cuarto del MAXIMO del que lo usa, pase lo que pase.
      // Estaba metido en el mismo saco que Derribo (un cuarto del dano) y
      // por eso salia carisimo contra un rival grande y casi gratis contra
      // uno diminuto. Cuesta lo mismo siempre: es un ultimo recurso, no
      // una apuesta.
      if (mv == MOVE_STRUGGLE) {
        r.recoil = (uint16_t)(att.hpMax / 4 ? att.hpMax / 4 : 1);
      } else if (fx == FX_RECOIL_HP) {
        r.recoil = (uint16_t)((att.hpMax + 1) / 2);   // (totalhp / 2.0).ceil
      } else {
        uint8_t div = (fx == FX_RECOIL_2) ? 2 : (fx == FX_RECOIL_3) ? 3 : 4;
        r.recoil = (uint16_t)((hecho + div - 1) / div);
      }
      att.hp = (r.recoil >= att.hp) ? 0 : (uint16_t)(att.hp - r.recoil);
      // Si los dos caen en el mismo golpe gana el que recibio: el que se mato
      // a si mismo no puede ganar.
      if (att.hp == 0) knockOut(from);
    }
  }

  // Clima y cura. Van antes que los estados porque no dependen de haberle
  // dado a nadie: son lo que HACE el golpe, no un efecto secundario.
  if (!over && mv < MOVE_COUNT) {
    const MoveEntry &m = MOVE_TBL[mv];
    uint16_t hueco = (uint16_t)(att.hpMax - att.hp);
    switch (m.fx) {
      case FX_W_SUN:
      case FX_W_RAIN:
      case FX_W_SAND:
      case FX_W_HAIL: {
        uint8_t nuevo = (m.fx == FX_W_SUN)  ? WE_SUN
                      : (m.fx == FX_W_RAIN) ? WE_RAIN
                      : (m.fx == FX_W_SAND) ? WE_SAND : WE_HAIL;
        // Si ya hacia ese tiempo, FALLA: no se renuevan los cinco
        // turnos gratis cada vez que se repite el golpe.
        if (weather == nuevo) { r.failed = true; break; }
        weather = nuevo;
        weatherTurns = WEATHER_TURNS;
        r.weatherSet = nuevo;
        break;
      }
      case FX_T_ELEC:
      case FX_T_GRASS:
      case FX_T_MISTY:
      case FX_T_PSY: {
        uint8_t nuevo = (m.fx == FX_T_ELEC)  ? TR_ELEC
                      : (m.fx == FX_T_GRASS) ? TR_GRASS
                      : (m.fx == FX_T_MISTY) ? TR_MISTY : TR_PSY;
        terrain = nuevo;
        terrainTurns = TERRAIN_TURNS;
        r.terrainSet = nuevo;
        break;
      }
      case FX_HEAL_HALF:
        // Con la vida llena FALLA, como en los juegos —— y como ya hace
        // FX_HEAL_QUARTER aqui mismo. Antes se gastaba el turno en
        // silencio: ni curaba ni decia nada.
        if (!hueco) { r.failed = true; break; }
        r.healed = (uint16_t)((att.hpMax + 1) / 2);
        if (r.healed > hueco) r.healed = hueco;
        att.hp += r.healed;
        break;

      // PURIFICACION. Media cura, SI, pero lo primero es que le quita el
      // estado AL OTRO —— y si el otro no tiene nada, el golpe falla
      // entero. Asi lo escribe el Anil en
      // CureTargetStatusHealUserHalfOfTotalHP
      // (030_Move/011_MoveEffects_Healing.rb:89): pbFailsAgainstTarget?
      // con `target.status == :NONE`, y luego pbCureStatus ANTES de la
      // cura. Aqui estaba cayendo en FX_HEAL_HALF por subcadena, o sea
      // que era un Recuperacion de tipo veneno: curaba y ya.
      //
      // Suena raro curar al rival, y lo es: en un combate de uno contra
      // uno este golpe le limpia el estado al que tienes delante. Es lo
      // que hace, y el Anil no lo trata de otra forma.
      case FX_PURIFY:
        if (def.status == ST_NONE) { r.failed = true; break; }
        def.status = ST_NONE;
        def.statusTurns = 0;
        r.sideFx = m.fx;
        if (hueco) {
          r.healed = (uint16_t)((att.hpMax + 1) / 2);
          if (r.healed > hueco) r.healed = hueco;
          att.hp += r.healed;
        }
        break;

      // DRENAJE DE FUERZA. Se cura LO QUE VALGA EL ATAQUE del otro
      // —— con sus escalones puestos ——, y de paso se lo baja. No es
      // una cura fija: contra un Machamp a +2 puede ser media vida, y
      // contra un Chansey no es casi nada. Esa es toda la gracia del
      // golpe, y aqui solo estaba el -1 de ataque (SX_LOWER_ATK): la
      // mitad que le da nombre faltaba.
      //
      // El Anil falla si el ataque del otro ya esta en el suelo
      // (pbFailsAgainstTarget? con statStageAtMin?), aunque el -1 no
      // pueda aplicarse por una habilidad: lo que importa es el
      // escalon, no si se puede mover.
      //
      // La bajada de ataque NO se hace aqui: la aplica SX_LOWER_ATK,
      // que corre DESPUES de este bloque. El orden importa y coincide
      // con el del Anil —— primero se mide el ataque, luego se baja.
      case FX_SAP_ATK: {
        if (def.stage[SG_ATK] <= -6) { r.failed = true; break; }
        uint16_t cura = def.atkStatRaw();
        if (!hueco || !cura) break;      // ya esta lleno: el -1 sigue valiendo
        r.healed = cura > hueco ? hueco : cura;
        att.hp += r.healed;
        break;
      }

      // ESTRIDENCIA. Los cinco +1 se PAGAN con un tercio de la vida, y
      // el golpe falla si no te llega para pagarlo. Estaba cayendo en
      // SX_RAISE_ALL por subcadena, asi que subia los cinco GRATIS: un
      // Danza Dragon doble sin precio y sin riesgo.
      //
      // Los stats se aplican aqui y no por el sx porque el cobro y el
      // fallo son la misma decision: con SX_RAISE_ALL puesto, un fallo
      // de este bloque habria subido los stats igual (el sx no mira
      // r.failed).
      // BASTION FINAL (No Retreat). Los cinco +1, UNA vez: falla si ya lo
      // uso, y le deja sin poder relevarse. Estaba como un SX_RAISE_ALL
      // pelado, o sea +1 a todo cada turno, gratis y sin atarle. Si ya
      // estaba atrapado por otra cosa sube igual y NO queda marcado (es
      // lo que hace el Anil: `if !user.trappedInBattle?`).
      case FX_NORETREAT: {
        if (att.noRetreat) { r.failed = true; break; }
        bool subio = false;
        for (uint8_t i = 0; i < SG_ACC; i++)
          if (att.applyStage(i, +1)) subio = true;
        if (!subio) { r.failed = true; break; }     // todo ya al tope
        if (!(att.bindTurns || att.trapped2)) {
          att.noRetreat = true;
          r.sideFx = m.fx;
        }
        r.stageWhich = SG_ATK; r.stageDelta = +1; r.stageSelf = true;
        break;
      }
      case FX_CLANG_SOUL: {
        uint16_t precio = (uint16_t)(att.hpMax / 3 ? att.hpMax / 3 : 1);
        if (att.hp <= precio) { r.failed = true; break; }
        for (uint8_t i = 0; i < SG_ACC; i++) att.applyStage(i, +1);
        att.hp = (uint16_t)(att.hp - precio);
        r.stageWhich = SG_ATK; r.stageDelta = +1; r.stageSelf = true;
        break;
      }

      // AUTOTOMIA. Ademas del +2 de velocidad (que hace el sx), se
      // quita 100 kg de encima. No es decorado: el peso decide la
      // potencia de Patada Baja contra el y la de Cuerpo Pesado cuando
      // lo usa el. En el Anil son 1000 decimas de kilo a
      // PBEffects::WeightChange, y no baja de 0,1 kg.
      // RIZO DEFENSA. El +1 de defensa lo hace el sx; lo que hace el
      // fx es dejarlo ENROSCADO, y eso dobla Rodar y Bola Hielo.
      // Sin esta linea el golpe era medio golpe.
      case FX_CURL:
        att.curled = true;
        break;

      case FX_AUTOTOMIZE:
        if (pesoAhora(att) <= 1) { r.failed = true; break; }
        if (att.ligero < 255) att.ligero++;
        r.sideFx = m.fx;
        break;

      // LIMPIEZA. Barre las trampas de LOS DOS lados, no solo del tuyo
      // —— eso es lo que lo separa de Despejar, que solo limpia el
      // tuyo y encima baja la evasion del otro. Aqui no barria nada:
      // era un Danza Dragon de normal con otro nombre.
      case FX_TIDYUP:
        for (uint8_t s = 0; s < 2; s++) {
          rocks[s] = spikes[s] = tspikes[s] = web[s] = 0;
        }
        r.sideFx = m.fx;
        break;

      // ------ los que estaban FUERA de la tabla por no tener efecto
      //
      // Un golpe de apoyo sin efecto modelado lo descarta el generador,
      // asi que estos seis no existian NI EN EL LEARNSET: el bicho que
      // los aprendia se quedaba con un hueco. Lo trajo el dueno al ver
      // un Mewtwo de 45 con tres golpes de apoyo y uno que pega.

      // REFRESCO: se quita quemadura, veneno o paralisis —— las tres y
      // nada mas: dormir y congelar NO (CureUserBurnPoisonParalysis).
      case FX_REFRESH:
        if (att.status != ST_BURN && att.status != ST_POISON &&
            att.status != ST_BADPOISON && att.status != ST_PARALYSIS) {
          r.failed = true;
          break;
        }
        att.status = ST_NONE;
        att.statusTurns = 0;
        r.sideFx = m.fx;
        break;

      // RECICLAJE: le vuelve la baya que se comio. `lastBerry` ya estaba
      // ahi para Cosecha, asi que esto es atar un cabo suelto.
      case FX_RECYCLE:
        if (att.item != IT_NONE || !att.lastBerry) { r.failed = true; break; }
        att.item = att.lastBerry;
        if (att.itemComido == att.item) att.itemComido = IT_NONE;
        att.lastBerry = 0;
        r.sideFx = m.fx;
        break;

      // ACUPRESION: +2 a UNO de sus stats al azar, de los que todavia
      // puedan subir. Su objetivo en el PBS es UserOrNearAlly, y aqui no
      // hay companero: va siempre sobre el que lo usa.
      case FX_ACUPRESSURE: {
        uint8_t puede[SG_COUNT], n = 0;
        for (uint8_t i = 0; i < SG_COUNT; i++)
          if (att.stage[i] < 6) puede[n++] = i;
        if (!n) { r.failed = true; break; }
        uint8_t cual = puede[btRand(n)];
        att.applyStage(cual, +2);
        r.stageWhich = cual;
        r.stageDelta = +2;
        r.stageSelf = true;
        break;
      }

      // LODO VENENOSO: solo le entra al ENVENENADO, y entonces le baja
      // ataque, especial y velocidad. Contra uno sano, falla.
      case FX_VENOM_DRENCH:
        if (def.status != ST_POISON && def.status != ST_BADPOISON) {
          r.failed = true;
          break;
        }
        def.applyStage(SG_SPA, -1, true);
        def.applyStage(SG_SPE, -1, true);
        if (def.applyStage(SG_ATK, -1, true)) {
          r.stageWhich = SG_ATK;
          r.stageDelta = -1;
          r.stageSelf = false;
        }
        break;

      // REFUGIO: media cura, y DOS TERCIOS con tormenta de arena. Es el
      // Sintesis de los de tierra, y el clima que le sirve es el que a
      // todos los demas les hace dano.
      case FX_SHORE_UP: {
        if (!hueco) { r.failed = true; break; }
        uint32_t cura = (weather == WE_SAND) ? (uint32_t)att.hpMax * 2 / 3
                                             : (uint32_t)(att.hpMax + 1) / 2;
        r.healed = (cura > hueco) ? hueco : (uint16_t)cura;
        att.hp += r.healed;
        break;
      }

      // OJO DE MENTE: le anula la evasion y le quita al siniestro la
      // inmunidad a lo psiquico. Las dos mitades viven en computeHit.
      case FX_MIRACLE_EYE:
        if (def.mirado) { r.failed = true; break; }
        def.mirado = true;
        r.sideFx = m.fx;
        break;

      // RESPIRO. La media cura ya la hacia (caia en FX_HEAL_HALF por
      // subcadena); lo que faltaba es que mientras dure el turno DEJA
      // DE SER VOLADOR —— con lo bueno (roca, hielo y electrico ya no
      // le son supereficaces) y con lo malo (pierde la inmunidad a
      // tierra: posado, un Terremoto le entra). El borrado lo hace el
      // cierre de turno.
      //
      // Falla con la vida llena, como todas las curas y como el Anil
      // (es una HealingMove: pbMoveFailed? mira canHeal?). Y entonces
      // TAMPOCO pierde el tipo: no se paga lo que no se cobra.
      case FX_ROOST:
        if (!hueco) { r.failed = true; break; }
        r.healed = (uint16_t)((att.hpMax + 1) / 2);
        if (r.healed > hueco) r.healed = hueco;
        att.hp += r.healed;
        att.roosted = true;
        r.sideFx = m.fx;
        break;

      // DESPEJAR. Barre, y barre en tres sitios distintos:
      //
      //   las TRAMPAS de los DOS lados   (gen >= 6; el Anil es 9)
      //   las PANTALLAS del lado del otro (Reflejo, Pantalla de Luz,
      //                                    Neblina y Velo Sagrado)
      //   el TERRENO                      (gen >= 8)
      //
      // El -1 de evasion no se hace aqui: lo aplica SX_LOWER_EVA, que
      // corre despues. Sin este caso, Despejar era eso y nada mas —— un
      // golpe de limpieza que no limpiaba.
      case FX_DEFOG: {
        uint8_t suyo = (uint8_t)(from ^ 1);
        for (uint8_t s = 0; s < 2; s++) {
          rocks[s] = spikes[s] = tspikes[s] = web[s] = 0;
        }
        for (uint8_t i = 0; i < PA_COUNT; i++) pantalla[suyo][i] = 0;
        espejaPantallas(suyo);
        terrain = TR_NONE;
        terrainTurns = 0;
        r.sideFx = m.fx;
        break;
      }
      case FX_HEAL_WEATHER: {
        // Con el sol de cara se cura casi entero; con el cielo cerrado,
        // apenas. Synthesis es un golpe distinto segun el tiempo que haga.
        if (!hueco) { r.failed = true; break; }   // lleno: falla, no calla
        uint16_t cura = (weather == WE_SUN)    ? (uint16_t)(att.hpMax * 2 / 3)
                      : (weather == WE_NONE)   ? (uint16_t)((att.hpMax + 1) / 2)
                                               : (uint16_t)(att.hpMax / 4);
        r.healed = cura > hueco ? hueco : cura;
        att.hp += r.healed;
        break;
      }
      case FX_REST:
        // Con un Alboroto en marcha no hay quien pegue ojo, ni el que
        // quiere dormirse a proposito.
        if (uproar) { r.failed = true; break; }
        // Se cura del todo, pero a cambio se duerme: dos turnos regalados.
        r.healed = hueco;
        att.hp = att.hpMax;
        att.status = ST_SLEEP;       // pisa lo que hubiera, que es medio truco
        att.statusTurns = 2;
        r.gotStatus = ST_SLEEP;
        break;

      // --- pantallas: protegen al que las pone, cinco turnos ---
      // Las cuatro van al LADO, y se espejan en el bicho que esta en
      // pista para que computeHit y applyStage las vean ya este turno.
      case FX_REFLECT:
        pantalla[from][PA_REFLECT] = SCREEN_TURNS;
        espejaPantallas(from);
        r.sideFx = m.fx;
        break;
      case FX_LIGHTSCREEN:
        pantalla[from][PA_LIGHT] = SCREEN_TURNS;
        espejaPantallas(from);
        r.sideFx = m.fx;
        break;
      case FX_SAFEGUARD:
        pantalla[from][PA_SAFE] = SCREEN_TURNS;
        espejaPantallas(from);
        r.sideFx = m.fx;
        break;
      case FX_MIST:
        pantalla[from][PA_MIST] = SCREEN_TURNS;
        espejaPantallas(from);
        r.sideFx = m.fx;
        break;

      // --- los que le hacen algo al rival sin pegarle ---------------
      // Casi todos estaban FUERA de la tabla: sin dano ni efecto
      // modelado, gen_moves los descartaba. Varios ademas no se podian
      // hacer hasta que hubo huida y relevos.
      // --- la segunda leva ---------------------------------------------
      case FX_YAWN: {
        // Falla si ya bosteza o si no podria dormirse (pbCanSleep); se
        // prueba en una copia porque applyStatus lo aplica.
        Combatant prueba = def;
        if (def.yawn || !prueba.applyStatus(ST_SLEEP, terrain, weather, uproar > 0)) {
          r.failed = true;
          break;
        }
        def.yawn = 2;
        r.sideFx = m.fx;
        break;
      }
      case FX_SHELLSMASH:
        // La bajada de defensas la hace el sx; esto es la otra mitad.
        att.applyStage(SG_ATK, +2);
        att.applyStage(SG_SPA, +2);
        att.applyStage(SG_SPE, +2);
        r.stageSelf = true;
        break;
      case FX_TOXICTHREAD:
        if (def.applyStage(SG_SPE, -1)) r.stageWhich = SG_SPE;
        break;
      case FX_GROWTH:
        // El sx ya da +1; con sol es +2.
        if (weather == WE_SUN) {
          att.applyStage(SG_ATK, +1);
          att.applyStage(SG_SPA, +1);
        }
        break;
      case FX_TRICKROOM:
        // Con el puesto, usarlo lo QUITA (el Anil: pbMoveFailed no, sino
        // field.effects[TrickRoom] = 0). sideFx lo cuenta igual; la
        // pantalla mira trickRoom para saber cual de las dos fue.
        trickRoom = trickRoom ? 0 : 5;
        r.sideFx = m.fx;
        break;
      case FX_SUBSTITUTE: {
        // Un cuarto de los PS al muneco. Falla si ya hay uno o si no le
        // llega la vida (con justo un cuarto tambien: se quedaria a 0).
        uint16_t coste = att.hpMax / 4 ? att.hpMax / 4 : 1;
        if (att.subHp || att.hp <= coste) { r.failed = true; break; }
        att.hp = (uint16_t)(att.hp - coste);
        att.subHp = coste;
        att.bindTurns = 0;              // el Anil: se suelta de lo que le ata
        r.sideFx = m.fx;
        break;
      }
      case FX_WISH:
        if (wishTurns[from]) { r.failed = true; break; }
        wishTurns[from] = 2;
        wishAmt[from] = (uint16_t)((att.hpMax + 1) / 2);
        r.sideFx = m.fx;
        break;
      case FX_PERISH: {
        // A TODOS los que lo oyen, el que canta incluido. Insonorizar
        // libra al rival; si nadie queda por marcar, falla.
        bool alguno = false;
        for (uint8_t k = 0; k < 2; k++) {
          Combatant &q = side[k];
          if (!q.valid() || q.fainted() || q.perish != 0xFF) continue;
          if (k != from && q.abil() == AB_SOUNDPROOF) continue;
          q.perish = 3;             // la misma cuenta que Cuerpo Mortal
          alguno = true;
        }
        if (!alguno) { r.failed = true; break; }
        r.sideFx = m.fx;
        break;
      }
      case FX_EMBARGO:
        if (def.embargoTurns) { r.failed = true; break; }
        def.embargoTurns = 5;
        r.sideFx = m.fx;
        break;
      case FX_HEALBLOCK:
        if (def.healBlockTurns) { r.failed = true; break; }
        def.healBlockTurns = 5;
        r.sideFx = m.fx;
        break;
      case FX_TAUNT:
        // Mofa: Taunt = 4 en el Anil, y baja al cerrar cada ronda (tambien
        // la de hoy): 4 turnos si aun no se movio y 3 si ya. Es lo que obliga a
        // pegar al que se habia montado una torre de subidas.
        // Miraba `att.tauntTurns`, y un mofado no puede usar Mofa: la
        // condicion no se cumplia nunca y la segunda Mofa renovaba la
        // primera. El Anil falla si el OBJETIVO ya la tiene.
        if (def.tauntTurns) { r.failed = true; break; }
        def.tauntTurns = 4;
        r.sideFx = m.fx;
        break;
      case FX_FORESIGHT:
        // Rastreo: su evasion deja de contar y deja de ser intocable para
        // normal y lucha. Las dos mitades las hace ahora `foreseen`, que
        // antes se escribia aqui y no lo leia nadie.
        //
        // Y se IGNORA la evasion, no se BORRA: borrar el stage cambia
        // Castigo, Poder Reserva, Relevo y Psicoataque, que leen los
        // stages de verdad. En los juegos el stage sigue ahi.
        def.foreseen = true;
        r.sideFx = m.fx;
        break;
      case FX_TRAP:
        // Ancla e Sombra Presa: e efeito adicional (a Potencia Bruta o
        // apaga). Mal de Ojo nao tem EffectChance: nao entra aqui.
        if (fuerzaBruta(att, mv)) break;
        // Mal de Ojo y companía: no se va. Antes esto no significaba
        // nada; ahora hay de donde irse.
        def.trapped2 = true;
        r.sideFx = m.fx;
        break;
      case FX_HEAL_BELL:
        // Cura de estado a TODO el equipo. Aqui solo esta en pista el
        // tuyo, asi que se le quita a el y quien maneja el banquillo
        // se encarga del resto (sideFx se lo dice).
        att.status = ST_NONE;
        att.statusTurns = 0;
        r.sideFx = m.fx;
        break;
      case FX_SOAK:
        // EMPAPAR: le vuelve de tipo AGUA. Esto estaba FUERA de la
        // tabla por no poder pisar el tipo, y la pieza la trajo
        // Mutatipo (typeSet). Un golpe desbloqueado por otro.
        if (def.typeSet == T_WATER) { r.failed = true; break; }
        def.typeSet = T_WATER;
        def.typeSet2 = 0;
        r.sideFx = m.fx;
        break;
      case FX_WORRY_SEED: {
        // ABATIDORAS: le cambia la habilidad por Insomnio. Se busca
        // en la tabla en vez de clavar un numero, que es lo que se
        // rompe al reordenar el catalogo.
        uint16_t ins = 0;
        for (uint16_t k = 1; k < ABIL_COUNT; k++)
          if (ABIL_TBL[k].fx == AB_IMMUNE_STATUS &&
              ABIL_TBL[k].arg == ST_SLEEP) { ins = k; break; }
        if (!ins || def.ability == ins) { r.failed = true; break; }
        def.ability = ins;
        def.abilOff = false;
        r.sideFx = m.fx;
        break;
      }
      case FX_LUCKY_CHANT:
        if (luckyChant[from]) { r.failed = true; break; }
        luckyChant[from] = 5;
        // Y vale YA. `chanted` es lo unico que computeHit mira, y solo se
        // sincronizaba al cerrar el turno: el rival podia dar un critico
        // en el mismo turno en que se canto.
        att.chanted = true;
        r.sideFx = m.fx;
        break;
      case FX_ATTRACT:
        // ATRACCION: solo entre sexos distintos, y con los dos
        // sexados. Es la unica cosa del combate que mira el sexo
        // aparte de Rivalidad.
        if (def.attracted || att.sexo == 2 || def.sexo == 2 ||
            att.sexo == def.sexo) {
          r.failed = true;
          break;
        }
        def.attracted = true;
        r.sideFx = m.fx;
        break;
      case FX_MAGNET_RISE:
        if (att.levitating) { r.failed = true; break; }
        att.levitating = 5;
        r.sideFx = m.fx;
        break;
      case FX_QUICK_GUARD:
        // ANTICIPO: este turno, a su bando no le entran los golpes
        // de prioridad. Es el Protect de la prioridad.
        quickGuard[from] = true;
        r.sideFx = m.fx;
        break;
      case FX_GASTRO:
        // BILIS: le anula la habilidad lo que queda de combate. Contra un
        // Intimidate ya gastado no hace nada; contra un Levitate o un
        // Multiscale, le quita media defensa.
        if (def.abilOff || def.ability == 0) { r.failed = true; break; }
        def.abilOff = true;
        r.sideFx = m.fx;
        break;
      case FX_ENTRAIN:
        // DANZA AMIGA: le pone LA SUYA. En un uno contra uno eso suele
        // ser regalarle algo flojo, que es justo la jugada.
        if (att.ability == 0 || def.ability == att.ability) {
          r.failed = true;
          break;
        }
        def.ability = att.ability;
        def.abilOff = false;
        def.abilityShown = false;   // la nueva se anuncia cuando toque
        r.sideFx = m.fx;
        break;
      case FX_IMPRISON:
        // SELLAR: le bloquea los golpes que el que lo usa TAMBIEN sabe.
        // Se marca y el bloqueo se mira al elegir golpe (ver arriba).
        if (att.imprisoning) { r.failed = true; break; }
        att.imprisoning = true;
        r.sideFx = m.fx;
        break;
      // --- lo que el banquillo hizo posible -------------------------
      case FX_ROCKS:
        // Las trampas van en el lado DEL RIVAL: pican a lo que el saque
        // despues. Contra un salvaje, que no releva, no haran nada — y
        // esta bien que no hagan nada en vez de fingir que si.
        if (rocks[from ^ 1]) { r.failed = true; break; }
        rocks[from ^ 1] = 1;
        r.sideFx = m.fx;
        break;
      case FX_SPIKES:
        if (spikes[from ^ 1] >= 3) { r.failed = true; break; }
        spikes[from ^ 1]++;
        r.sideFx = m.fx;
        break;
      case FX_TSPIKES:
        if (tspikes[from ^ 1] >= 2) { r.failed = true; break; }
        tspikes[from ^ 1]++;
        r.sideFx = m.fx;
        break;
      case FX_WEB:
        if (web[from ^ 1]) { r.failed = true; break; }
        web[from ^ 1] = 1;
        r.sideFx = m.fx;
        break;
      case FX_TAILWIND:
        if (tailwind[from]) { r.failed = true; break; }
        tailwind[from] = 4;
        r.sideFx = m.fx;
        break;
      case FX_WATERSPORT:
        if (waterSport) { r.failed = true; break; }
        waterSport = 5;
        r.sideFx = m.fx;
        break;
      case FX_MUDSPORT:
        if (mudSport) { r.failed = true; break; }
        mudSport = 5;
        r.sideFx = m.fx;
        break;
      case FX_BATON:
        // RELEVO: se cambia y le DEJA los stats al que entra. El motor no
        // saca a nadie (eso es del banquillo), asi que guarda el traspaso
        // y avisa, igual que Ida y Vuelta.
        if (from > 1 || !temReservas(from)) { r.failed = true; break; }
        baton[from] = true;
        for (uint8_t i = 0; i < SG_COUNT; i++) batonStage[from][i] = att.stage[i];
        if (from == BT_PLAYER) wantsSwitch = true;
        querTrocar[from] = true;
        r.sideFx = m.fx;
        break;
      case FX_INGRAIN:
        // ARRAIGO: echa raices. Se cura cada turno y a cambio ya no se
        // puede ir: las dos mitades juntas o no es el golpe.
        if (att.ingrained) { r.failed = true; break; }
        att.ingrained = true;
        att.trapped2 = true;
        break;
      case FX_PAINSPLIT: {
        // DIVIDE DOLOR: los dos a la media. Es el golpe que le da sentido
        // a un bicho con muchos PS y poco mas.
        uint16_t media = (uint16_t)(((uint32_t)att.hp + def.hp) / 2);
        // Cada uno se recorta por SU maximo, los dos desde la media. Antes
        // la media se recortaba por el maximo del ATACANTE y de ahi se
        // sacaba la del alvo: con un Shedinja (1 PS de maximo) la media de
        // 1 y 200 daba 100, se quedaba en 1, y el rival caia de 200 a 1.
        // Divide Dolor era un fulminante.
        uint16_t mia = media > att.hpMax ? att.hpMax : media;
        uint16_t suya = media > def.hpMax ? def.hpMax : media;
        if (att.hp == mia && def.hp == suya) { r.failed = true; break; }
        if (mia > att.hp) r.healed = (uint16_t)(mia - att.hp);
        att.hp = mia;
        def.hp = suya;
        if (def.hp == 0) knockOut((uint8_t)(from ^ 1));
        break;
      }
      case FX_TRICK: {
        // TRAPICHEO: se cambian el objeto. Sin objetos por ninguna parte
        // no hay trapicheo; con uno solo, se lo pasa.
        if (att.item == IT_NONE && def.item == IT_NONE) { r.failed = true; break; }
        // Falla tambien si lo usa un salvaje, o si cualquiera de los dos
        // objetos es intocable para cualquiera de los dos.
        if ((from == BT_FOE && !isTrainer) ||
            objetoIntocable(def, def.item) || objetoIntocable(def, att.item) ||
            objetoIntocable(att, att.item) || objetoIntocable(att, def.item)) {
          r.failed = true;
          break;
        }
        r.itemSwap = true;
        uint8_t tmp = att.item;
        att.item = def.item;
        def.item = tmp;
        // Liviano: cuenta para el que se queda con las manos vacias,
        // igual que en el Anil (012_MoveEffects_Items.rb:120).
        if (att.item == IT_NONE && tmp != IT_NONE) att.soltoAlgo = true;
        if (def.item == IT_NONE && att.item != IT_NONE) def.soltoAlgo = true;
        att.choiceSlot = 0xFF;      // el objeto cambio: ya no esta atado
        def.choiceSlot = 0xFF;
        r.itemStolen = att.item;    // la pantalla lo cuenta
        r.itemLost = def.item;
        break;
      }
      case FX_HEAL_QUARTER:
        // GOTA VITAL y companía: en los juegos cura al equipo; aqui, sin
        // compañero en la pista, cura al que lo usa. Un cuarto.
        if (!hueco) { r.failed = true; break; }
        {
          uint16_t cura = (uint16_t)(att.hpMax / 4 ? att.hpMax / 4 : 1);
          if (cura > hueco) cura = hueco;
          att.hp = (uint16_t)(att.hp + cura);
          r.healed = cura;
        }
        break;
      case FX_TORMENT:
        // TORMENTO: no le deja repetir golpe. Contra un bicho de cuatro
        // golpes molesta; contra uno que vive de uno solo, lo rompe.
        if (def.tormented) { r.failed = true; break; }
        def.tormented = true;
        r.sideFx = m.fx;
        break;
      case FX_COPYCAT:
        // COPION: usa el ultimo golpe que se vio. Se resuelve antes, al
        // elegir el golpe (ver arriba): si llega aqui es que no habia
        // nada que copiar.
        r.failed = true;
        break;
      case FX_STOCKPILE:
        // Reserva: llena una carga (tres como mucho) y sube defensa y
        // defensa especial. Lo que lo hace valer es que luego se gasta.
        if (att.stockpile >= 3) { r.failed = true; break; }
        att.stockpile++;
        att.applyStage(SG_DEF, +1);
        if (att.applyStage(SG_SPD, +1)) {
          r.stageWhich = SG_SPD; r.stageDelta = +1; r.stageSelf = true;
        }
        break;
      case FX_SWALLOW: {
        // Tragar: se come lo acumulado y cura segun cuanto habia. Un
        // cuarto, la mitad, todo. Vacia el deposito siempre.
        if (att.stockpile == 0) { r.failed = true; break; }
        uint16_t cura = att.stockpile == 1 ? (uint16_t)(att.hpMax / 4)
                      : att.stockpile == 2 ? (uint16_t)(att.hpMax / 2)
                                           : att.hpMax;
        if (cura > hueco) cura = hueco;
        // Gastar el deposito devuelve tambien lo que Reserva habia
        // subido. Sin esto, Reserva x3 + Tragar era +3/+3 gratis ademas
        // de la cura: el golpe no tenia precio ninguno.
        gastaReserva(att);
        if (!cura) { r.failed = true; break; }
        att.hp = (uint16_t)(att.hp + cura);
        r.healed = cura;
        break;
      }
      case FX_ROAR:
        // Rugido: le echa del combate. Contra un SALVAJE eso es que se
        // acaba el combate — y es justo para lo que sirve, para
        // librarse de un encuentro sin estropear los EVs. Contra un
        // entrenador, aqui no hay a quien sacar, asi que falla.
        //
        // As falhas do Anil (SwitchOutTargetStatusMove#pbFailsAgainstTarget?)
        // valem para os dois casos: quem se agarra nao sai, e um salvaje
        // de nivel MAIOR que quem ruge tambem nao. Contra um time COM
        // BANCO (a batalha entre placas) arrasta: entra um sorteado.
        if (naoSaiEmpurrado(att, def)) { r.failed = true; break; }
        if (isTrainer) {
          const uint8_t alvo = (uint8_t)(from ^ 1);
          if (from > 1 || !temReservas(alvo)) { r.failed = true; break; }
          arrastado[alvo] = true;
          r.sideFx = m.fx;
          break;
        }
        if (def.level > att.level) { r.failed = true; break; }
        over = true;
        fled = true;            // ni ganaste ni perdiste: se fue
        winner = 0xFF;
        r.sideFx = m.fx;
        break;

      // --- solo este turno ---
      case FX_PROTECT:
      case FX_ENDURE: {
        // Encadenarlos falla cada vez mas: si no, Protect en bucle seria un
        // empate garantizado contra cualquier rival que solo sepa pegar.
        bool sale = (att.protectRun == 0) || (btRand(1 << att.protectRun) == 0);
        if (!sale) { r.failed = true; att.protectRun = 0; break; }
        if (m.fx == FX_PROTECT) att.protecting = true;
        else                    att.enduring = true;
        if (att.protectRun < 4) att.protectRun++;
        r.sideFx = m.fx;
        break;
      }

      // --- mover lo que ya hay puesto ---
      case FX_HAZE:
        for (uint8_t i = 0; i < SG_COUNT; i++) { att.stage[i] = 0; def.stage[i] = 0; }
        r.sideFx = m.fx;
        break;
      case FX_CLEAR_TARGET:
        for (uint8_t i = 0; i < SG_COUNT; i++) def.stage[i] = 0;
        r.sideFx = m.fx;
        break;
      case FX_PSYCHUP:
        for (uint8_t i = 0; i < SG_COUNT; i++) att.stage[i] = def.stage[i];
        r.sideFx = m.fx;
        break;
      case FX_BELLYDRUM:
        // Ataque al maximo a cambio de medio deposito: la apuesta mas cara
        // del juego, y por eso falla si no te llega para pagarla.
        if (att.hp * 2 <= att.hpMax) { r.failed = true; break; }
        att.hp = (uint16_t)(att.hp - att.hpMax / 2);
        att.stage[SG_ATK] = 6;
        r.stageWhich = SG_ATK; r.stageDelta = 6; r.stageSelf = true;
        break;

      // --- criticos y punteria ---
      case FX_FOCUSENERGY:
        if (att.critStage >= 2) { r.failed = true; break; }
        att.critStage = (int8_t)(att.critStage + 2);
        r.sideFx = m.fx;
        break;
      case FX_LASERFOCUS:  att.sureCrit = true; r.sideFx = m.fx; break;
      case FX_MINDREADER:  att.sureHit = true;  r.sideFx = m.fx; break;

      // --- que va royendo turno a turno ---
      case FX_LEECHSEED:
        // Las semillas no prenden en los de planta, que es lo que hace que
        // Venusaur sea un mal objetivo para su propio golpe.
        if (def.leechSeed || def.type1() == T_GRASS || def.type2() == T_GRASS) {
          r.failed = true;
          break;
        }
        def.leechSeed = true;
        r.sideFx = m.fx;
        break;
      case FX_AQUARING:
        if (att.aquaRing) { r.failed = true; break; }
        att.aquaRing = true;
        r.sideFx = m.fx;
        break;

      // MALDICION. Son DOS golpes distintos dentro del mismo, y cual
      // sale depende del TIPO del que lo usa. Asi lo escribe el Anil en
      // CurseTargetOrLowerUserSpd1RaiseUserAtkDef1
      // (030_Move/013_MoveEffects_ChangeMoveEffect.rb):
      //
      //   FANTASMA  paga la MITAD de sus PS maximos y maldice al otro,
      //             que pierde un cuarto de los suyos al cerrar cada
      //             turno. Falla si el otro ya esta maldito.
      //   LOS DEMAS -1 velocidad, +1 ataque y +1 defensa, a si mismo.
      //
      // Aqui era solo SX_RAISE_ATKDEF: ni bajaba la velocidad —— asi que
      // para un no-fantasma era una Danza Dragon regalada —— ni existia
      // la mitad de fantasma, que es la que le da nombre al golpe. Un
      // Gengar se subia ataque y defensa, que es justo lo que Maldicion
      // NO hace en sus manos.
      case FX_CURSE:
        if (att.type1() == T_GHOST || att.type2() == T_GHOST) {
          if (def.cursed || def.fainted()) { r.failed = true; break; }
          def.cursed = true;
          // La mitad de los PS MAXIMOS, y puede costarle el combate: el
          // Anil llama a pbReduceHP sin red, y caer pagandola es parte
          // del precio del golpe.
          uint16_t paga = (uint16_t)(att.hpMax / 2 ? att.hpMax / 2 : 1);
          att.hp = (paga >= att.hp) ? 0 : (uint16_t)(att.hp - paga);
          r.sideFx = m.fx;
          if (att.hp == 0) knockOut(from);
          break;
        }
        // La velocidad PRIMERO, como en el Anil (pbEffectGeneral hace
        // antes el statDown y luego el statUp), y solo falla si no se
        // pudo mover NINGUNO de los tres.
        {
          bool algo = att.applyStage(SG_SPE, -1);
          if (att.applyStage(SG_DEF, +1)) algo = true;
          if (att.applyStage(SG_ATK, +1)) {
            algo = true;
            r.stageWhich = SG_ATK; r.stageDelta = +1; r.stageSelf = true;
          }
          if (!algo) r.failed = true;
        }
        break;

      // ESTIMULO: ademas del doble dano, le QUITA la paralisis. Es un
      // golpe con precio —— le curas para pegarle fuerte —— y sin esta
      // mitad no es mas que un Placaje de 140 contra un paralizado.
      case FX_X2_IF_PARA:
        if (def.status == ST_PARALYSIS) {
          def.status = ST_NONE;
          def.statusTurns = 0;
          r.curedTarget = true;
        }
        break;

      // REGALO: una de cada cinco veces le cura un cuarto al rival en vez
      // de pegarle. computeHit ya lo decidio (no puede tocar a nadie desde
      // alli); aqui se hace.
      case FX_PRESENT:
        if (r.presentHeal) {
          // canHeal? del Anil: entero o con Anticura, "pero ha fallado".
          if (def.fainted() || def.hp >= def.hpMax || def.healBlockTurns) {
            r.failed = true;
            break;
          }
          uint16_t cura = def.hpMax / 4 ? def.hpMax / 4 : 1;
          if (def.hp + cura > def.hpMax) cura = (uint16_t)(def.hpMax - def.hp);
          def.hp = (uint16_t)(def.hp + cura);
          r.healedTarget = cura;
        }
        break;

      // PUNOS PLASMA: el resto del turno, lo normal pasa a electrico. En
      // uno contra uno eso quiere decir que el golpe normal del rival,
      // si va despues, le entra a un fantasma —— que es justo la jugada.
      case FX_NORMAL_IS_ELEC:
        normalEsElec = true;
        break;

      // BORRAR EL TERRENO. Bola Acero y Trompo Hielo pegan Y dejan el
      // campo pelado: es la respuesta a quien se monta su terreno y vive
      // de el. Bola Acero ademas FALLA si no habia terreno, que es lo que
      // la hace una apuesta y no un golpe de 130 gratis.
      case FX_WIPE_TERRAIN:
        if (terrain == TR_NONE && MOVE_TBL[mv].power >= 130) {
          r.failed = true;
          break;
        }
        terrain = TR_NONE;
        terrainTurns = 0;
        break;

      // QUEMARSE EL PROPIO TIPO. Se paga DESPUES de pegar, asi que el
      // golpe de esta vez si lleva su STAB; el siguiente ya no.
      case FX_LOSE_FIRE:
        att.tipoPerdido = T_FIRE;
        r.lostType = T_FIRE;
        break;
      case FX_LOSE_ELEC:
        att.tipoPerdido = T_ELEC;
        r.lostType = T_ELEC;
        break;

      // SALAZON: le roe cada turno, como el veneno, y ademas el doble si
      // es de agua o de acero. El desgaste lo cobra endTurn.
      case FX_SALT_CURE:
        if (fuerzaBruta(att, mv)) break;     // efeito adicional
        if (!def.saltCure) {
          def.saltCure = 1;
          r.sideFx = m.fx;
        }
        break;

      // ARIA BURBUJA: le APAGA la quemadura al rival. Es de las pocas
      // cosas buenas que un golpe le hace al otro, y por eso hay que
      // decirlo en pantalla.
      case FX_CURE_BURN:
        if (fuerzaBruta(att, mv)) break;     // efeito adicional
        if (def.status == ST_BURN) {
          def.status = ST_NONE;
          def.statusTurns = 0;
          r.curedTarget = true;
        }
        break;

      // ASALTO ESPADA: pegas 120 y hasta tu siguiente turno te entran los
      // golpes al doble. Se apaga al principio de tu turno (ver useMove).
      case FX_EXPOSED:
        att.expuesto = true;
        break;

      // MARTILLO GIGA: 160 de potencia a cambio de no poder repetirlo.
      case FX_NO_REPEAT:
        att.noRepetir = mv;
        break;

      // BOMBSEMILLA: pega Y planta la semilla, las dos cosas.
      case FX_HIT_AND_SEED:
        if (!def.leechSeed && def.type1() != T_GRASS && def.type2() != T_GRASS) {
          def.leechSeed = true;
          r.sideFx = FX_HIT_AND_SEED;
        }
        break;

      // ROBO ESPECTRAL: le quita al rival TODO lo que se haya subido y se
      // lo queda. Es el castigo a quien se pasa el combate bailando, y el
      // unico golpe que convierte la ventaja del otro en la tuya.
      case FX_STEAL_STAGES: {
        bool algo = false;
        for (uint8_t i = 0; i < SG_COUNT; i++) {
          if (def.stage[i] <= 0) continue;
          att.applyStage(i, def.stage[i]);
          def.stage[i] = 0;
          algo = true;
        }
        if (algo) r.stolenStages = true;
        break;
      }

      // --- tocarle los golpes al rival ---
      case FX_DISABLE:
        if (def.lastSlot >= BT_MOVES || def.disableTurns) { r.failed = true; break; }
        def.disabledSlot = def.lastSlot;
        def.disableTurns = 5;           // Disable = 5 en el Anil
        r.sideFx = m.fx;
        break;
      case FX_ENCORE:
        // Encore = 4 y baja al cerrar cada ronda, tambien la de hoy: repite
        // 4 turnos si aun no se habia movido y 3 si ya (no es uno solo).
        // Falla como en el Anil: ya lo tiene, no ha usado nada, el ultimo
        // golpe no le quedan PP, va a usar Coparazon, o el golpe esta en la
        // lista negra (Encore, Forcejeo, los que copian).
        if (def.lastSlot >= BT_MOVES || def.encoreTurns || def.shellTrap ||
            !def.usesLeft[def.lastSlot] ||
            def.moves[def.lastSlot] >= MOVE_COUNT ||
            def.moves[def.lastSlot] == MOVE_STRUGGLE) {
          r.failed = true;
          break;
        }
        {
          uint8_t fxU = MOVE_TBL[def.moves[def.lastSlot]].fx;
          if (fxU == FX_ENCORE || fxU == FX_SKETCH || fxU == FX_TRANSFORM ||
              fxU == FX_COPYCAT) {
            r.failed = true;
            break;
          }
        }
        def.encoreSlot = def.lastSlot;
        def.encoreTurns = 4;
        // Si aun no se movio y ESE golpe no lo puede elegir, esta ronda
        // sigue con lo que pidio (169_Battler_UseMove.rb:168).
        def.encoreSalta = !movedThisTurn[from ^ 1] &&
                          slotBloqueado(from ^ 1, def.encoreSlot);
        r.sideFx = m.fx;
        break;
      case FX_SPITE:
        if (def.lastSlot >= BT_MOVES || !def.usesLeft[def.lastSlot]) {
          r.failed = true;
          break;
        }
        def.usesLeft[def.lastSlot] =
            (uint8_t)(def.usesLeft[def.lastSlot] > 4 ? def.usesLeft[def.lastSlot] - 4 : 0);
        r.sideFx = m.fx;
        break;

      case FX_SPLASH:
        r.failed = true;   // literalmente no pasa nada, y esa es la gracia
        break;

      // TRANSFORMACION. La maquinaria ya estaba (transformInto, escrita
      // para la Imposter del Ditto); lo que faltaba era el GOLPE. Y
      // faltaba de una forma que no se ve en una tabla: Transformacion
      // es lo UNICO que aprende el Ditto, asi que sin este caso el
      // golpe se caia del aprendizaje y el bicho embarcaba sabiendo
      // solo Forcejeo. Lo mismo el Smeargle con Esquema.
      case FX_TRANSFORM:
        // No se copia a quien ya es una copia: dos Dittos enfrentados se
        // quedarian mirandose, y copiar una copia no significa nada.
        if (!def.valid() || att.transformed || def.transformed) {
          r.failed = true;
          break;
        }
        att.transformInto(def);
        r.sideFx = m.fx;
        break;

      // ESQUEMA: se queda con el ULTIMO golpe que uso el otro, en el
      // hueco de Esquema. Permanente en los juegos; aqui dura el
      // combate, que es lo unico que hay (los golpes del bicho del
      // jugador se recalculan por nivel al guardar).
      case FX_SKETCH: {
        if (def.lastMove == 0xFFFF || def.lastMove == 0) { r.failed = true; break; }
        // No se copia un Esquema con otro Esquema: quedaria un hueco que
        // solo sabe copiarse a si mismo.
        if (MOVE_TBL[def.lastMove].fx == FX_SKETCH) { r.failed = true; break; }
        // Y no se copia uno que ya tiene: gastaria el hueco en nada.
        for (uint8_t i = 0; i < att.nMoves; i++)
          if (att.moves[i] == def.lastMove) { r.failed = true; break; }
        if (r.failed) break;
        att.moves[slot] = def.lastMove;
        att.usesLeft[slot] = movePP(def.lastMove);
        r.sideFx = m.fx;
        break;
      }
      default:
        break;
    }
  }

  // Efecto de estado. Se tira DESPUES del dano y solo si el golpe entro: un
  // golpe que no hizo nada por inmunidad de tipo tampoco quema ni paraliza.
  if (mv < MOVE_COUNT && MOVE_TBL[mv].sx != SX_NONE && r.effect > 0) {
    const MoveEntry &m = MOVE_TBL[mv];
    // Shield Dust apaga los efectos secundarios de los golpes que le pegan;
    // Serene Grace los dobla. Ninguna de las dos toca a los que se aplica el
    // propio atacante (Overheat bajandose el especial no es un secundario).
    // OJO con este rango: decide si el secundario va al RIVAL o a uno
    // mismo, y se escribe con los extremos del enum. Al entrar los cuatro
    // nuevos (SX_SELF_DEF y companía, que van DESPUES de SX_RAISE_ALL)
    // quedaban fuera, y Escamas Ruidosas le habria bajado la defensa al
    // rival en vez de al que la usa —— un precio convertido en premio, y
    // sin ningun error de compilacion de por medio.
    bool suyo = sxEsPropio(m.sx);
    // Si el golpe TUMBO al rival, lo que iba dirigido a el ya no tiene a
    // quien tocar — pero lo que el atacante se aplica a SI MISMO sigue
    // valiendo: Overheat te deja el especial hecho trizas aunque el otro
    // caiga, igual que en los juegos. Antes `over` cortaba las dos cosas, y
    // no se notaba porque con los PS inflados nada tumbaba de un golpe; al
    // arreglar los PS, Overheat y Close Combat salian gratis.
    if (over && !suyo) {
      // el rival ya no esta para recibir nada
    } else if (!suyo && r.subHit) {
      // se los comio el muneco: ni quema ni baja nada
    } else if (m.fx == FX_YAWN) {
      // el sx solo lo marca como golpe contra el rival; duerme el reloj
    } else if (m.fx == FX_CHARGE && m.power > 0 &&
               (m.sx == SX_RAISE_DEF || m.sx == SX_RAISE_SPA)) {
      // Cabezazo, Rayo Meteorico, Electrorrayo: la subida ya se dio en
      // el turno de carga (ver FX_CHARGE en useMove). Otra aqui seria doble.
    } else if (!suyo && def.abil() == AB_NO_SECONDARY && m.power > 0) {
      // ni se tira el dado
    } else if (m.power > 0 && fuerzaBruta(att, mv)) {
      // Sheer Force: la otra cara del +30% que ya se cobro en el dano.
      // El golpe pega mas y NO hace su efecto. Es un trato, no un
      // regalo: con el, Rayo Hielo deja de congelar.
    } else {
    uint16_t prob = m.chance;
    if (!suyo && att.abil() == AB_SERENE_GRACE && prob < 100) prob = (uint16_t)(prob * 2);
    // UM DADO POR IMPACTO no Anil (pbAdditionalEffect vive em
    // pbProcessMoveHit). Um so dado com a chance de "pelo menos um" da o
    // mesmo: Twineedle 20% x2 = 36%, Double Iron Bash 30% x2 = 51%.
    if (!suyo && prob < 100 && r.hits > 1) {
      uint32_t falla = 100 - prob;
      for (uint8_t k = 1; k < r.hits; k++) falla = falla * (100 - prob) / 100;
      prob = (uint16_t)(100 - falla);
    }
    if (prob >= 100 || (uint32_t)btRand(100) < prob) {
      uint8_t sxTiro = m.sx;
      if (m.fx == FX_TRIATTACK) {
        static const uint8_t TRI[3] = { SX_BURN, SX_FREEZE, SX_PARALYZE };
        sxTiro = TRI[btRand(3)];
      } else if (m.fx == FX_DIRECLAW) {
        static const uint8_t GARRA[3] = { SX_POISON, SX_PARALYZE, SX_SLEEP };
        sxTiro = GARRA[btRand(3)];
      }
      switch (sxTiro) {
        // El CIELO va con el suelo en todas: lo pide Leaf Guard, que
        // al sol no deja entrar ningun estado.
        case SX_BURN:      if (def.applyStatus(ST_BURN, terrain, weather))      r.gotStatus = ST_BURN; break;
        case SX_POISON:    if (def.applyStatus(ST_POISON, terrain, weather, uproar > 0,
                                               att.abil() == AB_CORROSION)) r.gotStatus = ST_POISON; break;
        case SX_BADPOISON: if (def.applyStatus(ST_BADPOISON, terrain, weather, uproar > 0,
                                               att.abil() == AB_CORROSION)) r.gotStatus = ST_BADPOISON; break;
        case SX_PARALYZE:  if (def.applyStatus(ST_PARALYSIS, terrain, weather)) r.gotStatus = ST_PARALYSIS; break;
        case SX_SLEEP:     if (def.applyStatus(ST_SLEEP, terrain, weather, uproar > 0)) r.gotStatus = ST_SLEEP; break;
        case SX_FREEZE:    if (def.applyStatus(ST_FREEZE, terrain, weather))    r.gotStatus = ST_FREEZE; break;
        case SX_CONFUSE:
          // Oblivious y Own Tempo: a estos no se les marea. Y Safeguard y
          // el campo de Niebla tambien lo paran: applyStatus() los mira
          // los dos, pero la confusion no pasa por ahi —— va en su propio
          // contador —— asi que se quedaron fuera.
          if (def.abil() != AB_NO_CONFUSE && def.confuseTurns == 0 &&
              !def.safeguard &&
              !(terrain == TR_MISTY && def.grounded()))
            def.confuseTurns = (uint8_t)(2 + btRand(4));
          break;
        case SX_FLINCH:
          // Solo sirve si el rival NO se ha movido todavia este turno. El
          // comentario de antes decia esto mismo y daba por hecho que
          // marcarlo bastaba —— y no bastaba: `flinched` solo se borra
          // cuando el marcado INTENTA moverse, asi que pegando el segundo
          // la marca sobrevivia al turno y le saltaba en el siguiente.
          // `movedThisTurn` ya existia; solo faltaba mirarlo.
          if (movedThisTurn[from ^ 1]) {
            // ya tuvo su turno: asustarle ahora no le quita nada
          } else if (def.abil() == AB_STEADFAST) {
            // Impasible: cada susto le hace reaccionar. Se amilana
            // IGUAL (pierde el turno) y ademas gana velocidad, que es
            // lo que convierte que te asusten en algo que te sirve.
            def.flinched = true;
            if (def.applyStage(SG_SPE, +1)) {
              r.abilFired = def.ability;
              r.abilOfDef = true;
            }
          } else if (def.abil() != AB_NO_FLINCH) {
            def.flinched = true;   // Inner Focus se libra
          }
          break;
        // Los que se los aplica el que PEGA: son el precio de un golpe muy
        // potente (Overheat pega 130 pero se deja el especial hecho trizas).
        case SX_SELF_SPA2:
          if (att.applyStage(SG_SPA, -2)) { r.stageWhich = SG_SPA; r.stageDelta = -2; r.stageSelf = true; }
          break;
        case SX_SELF_DEFSPD:
          att.applyStage(SG_SPD, -1);
          if (att.applyStage(SG_DEF, -1)) { r.stageWhich = SG_DEF; r.stageDelta = -1; r.stageSelf = true; }
          break;
        case SX_SELF_SPE:
          if (att.applyStage(SG_SPE, -1)) { r.stageWhich = SG_SPE; r.stageDelta = -1; r.stageSelf = true; }
          break;
        case SX_SELF_ATK:
          if (att.applyStage(SG_ATK, -1)) { r.stageWhich = SG_ATK; r.stageDelta = -1; r.stageSelf = true; }
          break;
        case SX_SELF_DEF:           // Escamas Ruidosas
          if (att.applyStage(SG_DEF, -1)) { r.stageWhich = SG_DEF; r.stageDelta = -1; r.stageSelf = true; }
          break;
        case SX_SELF_SPA:           // Lluvia Monedas
          if (att.applyStage(SG_SPA, -1)) { r.stageWhich = SG_SPA; r.stageDelta = -1; r.stageSelf = true; }
          break;
        case SX_SELF_SPE2:          // Giro Acero
          if (att.applyStage(SG_SPE, -2)) { r.stageWhich = SG_SPE; r.stageDelta = -2; r.stageSelf = true; }
          break;
        case SX_SELF_SCALE:         // Escama Disparo: -1 Def y +1 Vel, en ese orden
        {
          bool baixou = att.applyStage(SG_DEF, -1);
          if (att.applyStage(SG_SPE, +1)) { r.stageWhich = SG_SPE; r.stageDelta = +1; r.stageSelf = true; }
          else if (baixou) { r.stageWhich = SG_DEF; r.stageDelta = -1; r.stageSelf = true; }
          break;
        }
        case SX_SELF_DEFSPDSPE:     // Fuego Explosivo: 180 de potencia y te deja temblando
          att.applyStage(SG_SPD, -1);
          att.applyStage(SG_SPE, -1);
          if (att.applyStage(SG_DEF, -1)) { r.stageWhich = SG_DEF; r.stageDelta = -1; r.stageSelf = true; }
          break;
        case SX_RAISE_ALL:
          for (uint8_t i = 0; i < SG_ACC; i++) att.applyStage(i, +1);
          r.stageWhich = SG_ATK; r.stageDelta = +1; r.stageSelf = true;
          break;
        // Dos stats de golpe: el pack que hace que valga la pena gastar el
        // turno en Dragon Dance en vez de pegar.
        case SX_RAISE_ATKDEF:
        case SX_RAISE_ATKSPE:
        case SX_RAISE_ATKSPA:
        case SX_RAISE_SPASPD:
        case SX_RAISE_ATKACC:
        case SX_RAISE_DEFSPD: {
          uint8_t a = SG_ATK, b = SG_DEF;
          if (m.sx == SX_RAISE_ATKSPE) { a = SG_ATK; b = SG_SPE; }
          else if (m.sx == SX_RAISE_ATKACC) { a = SG_ATK; b = SG_ACC; }
          else if (m.sx == SX_RAISE_ATKSPA) { a = SG_ATK; b = SG_SPA; }
          else if (m.sx == SX_RAISE_SPASPD) { a = SG_SPA; b = SG_SPD; }
          else if (m.sx == SX_RAISE_DEFSPD) { a = SG_DEF; b = SG_SPD; }
          att.applyStage(b, +1);
          if (att.applyStage(a, +1)) { r.stageWhich = a; r.stageDelta = +1; r.stageSelf = true; }
          break;
        }
        // DANZA ALETEO y GEOCONTROL: at.esp, def.esp y velocidad. El mapa
        // los resumia a "sube el at.esp" y se perdian dos tercios.
        case SX_RAISE_SPASPDSPE:
        case SX_RAISE2_SPASPDSPE: {
          int8_t cuanto = (m.sx == SX_RAISE2_SPASPDSPE) ? 2 : 1;
          att.applyStage(SG_SPD, cuanto);
          att.applyStage(SG_SPE, cuanto);
          if (att.applyStage(SG_SPA, cuanto)) {
            r.stageWhich = SG_SPA; r.stageDelta = cuanto; r.stageSelf = true;
          }
          break;
        }
        // LUZ COLA: at.esp +3. Es el unico golpe del juego que sube tres
        // escalones de una, y salia como un +1 cualquiera.
        case SX_RAISE3_SPA:
          if (att.applyStage(SG_SPA, +3)) {
            r.stageWhich = SG_SPA; r.stageDelta = +3; r.stageSelf = true;
          }
          break;
        // ANIMO: sube los dos especiales Y se quita el estado. Lo segundo
        // es lo que lo separa de Paz Mental, y no estaba.
        case SX_RAISE_SPASPD_CURA:
          att.applyStage(SG_SPD, +1);
          if (att.applyStage(SG_SPA, +1)) {
            r.stageWhich = SG_SPA; r.stageDelta = +1; r.stageSelf = true;
          }
          if (att.status != ST_NONE) {
            att.status = ST_NONE;
            att.statusTurns = 0;
          }
          break;
        // AGUIJON LETAL: +3 al ataque, y SOLO si el golpe lo tumba. Salia
        // como un +1 cada vez que acertaba, tumbara o no.
        case SX_RAISE3_ATK_KO:
          if (def.fainted() && att.applyStage(SG_ATK, +3)) {
            r.stageWhich = SG_ATK; r.stageDelta = +3; r.stageSelf = true;
          }
          break;
        // SEDUCCION: -2 al at.esp, pero solo a quien PODRIA atraer. Las
        // mismas condiciones que FX_ATTRACT, que ya las tiene escritas.
        case SX_LOWER2_SPA_ATR:
          if (att.sexo != 2 && def.sexo != 2 && att.sexo != def.sexo &&
              def.applyStage(SG_SPA, -2, true)) {
            r.stageWhich = SG_SPA; r.stageDelta = -2;
          }
          break;
        // CONTONEO y CAMELO: le suben un stat AL RIVAL y le marean. La
        // subida es el precio —— sin ella el golpe es un Rayo Confuso con
        // otro nombre, que es justo en lo que se habia quedado.
        case SX_CONFUSE_ATK2:
        case SX_CONFUSE_SPA1: {
          uint8_t cual = (m.sx == SX_CONFUSE_ATK2) ? SG_ATK : SG_SPA;
          int8_t cuanto = (m.sx == SX_CONFUSE_ATK2) ? 2 : 1;
          if (def.applyStage(cual, cuanto, true)) {
            r.stageWhich = cual; r.stageDelta = cuanto;
          }
          if (def.abil() != AB_NO_CONFUSE && def.confuseTurns == 0 &&
              !def.safeguard &&
              !(terrain == TR_MISTY && def.grounded()))
            def.confuseTurns = (uint8_t)(2 + btRand(4));
          break;
        }
        case SX_RAISE_ATKDEFACC:
          // ENROSQUE: ataque, defensa Y punteria del que lo usa. Tres de
          // golpe, que es lo que justifica gastar el turno.
          att.applyStage(SG_DEF, +1);
          att.applyStage(SG_ACC, +1);
          if (att.applyStage(SG_ATK, +1)) {
            r.stageWhich = SG_ATK; r.stageDelta = +1; r.stageSelf = true;
          }
          break;
        case SX_LOWER_ATKDEF:
          // HALAGO: ataque y defensa del rival. Mismo patron que el de
          // abajo —— va por `deFuera`, que es lo que respeta Niebla.
          def.applyStage(SG_DEF, -1, true);
          if (def.applyStage(SG_ATK, -1, true)) {
            r.stageWhich = SG_ATK;
            r.stageDelta = -1;
            r.stageSelf = false;
          }
          break;
        case SX_LOWER_ATKSPA:
          // Rugido de Guerra y Ojos Llorosos: los DOS ataques del rival.
          // Va aparte del bloque de arriba porque eso se lo aplica el que
          // pega a SI MISMO y esto es al otro — y porque lo del otro pasa
          // por `deFuera`, que es lo que respeta Niebla.
          def.applyStage(SG_SPA, -1, true);
          if (def.applyStage(SG_ATK, -1, true)) {
            r.stageWhich = SG_ATK; r.stageDelta = -1;
          }
          break;
        default: {
          // las subidas y bajadas de stage comparten forma
          // OJO con los limites: la evasion se anadio DESPUES y al principio
          // quedo fuera de los rangos, asi que Minimize no hacia nada. Tienen
          // que llegar hasta *_EVA, que es el ultimo de cada bloque.
          static const uint8_t CUAL[] = { SG_ATK, SG_DEF, SG_SPA, SG_SPD,
                                          SG_SPE, SG_ACC, SG_EVA };
          if (m.sx >= SX_LOWER_ATK && m.sx <= SX_LOWER_EVA) {
            uint8_t w = CUAL[m.sx - SX_LOWER_ATK];
            if (def.applyStage(w, -1, true)) {
              r.stageWhich = w; r.stageDelta = -1;
              picado(def, r);
            }
          } else if (m.sx >= SX_RAISE_ATK && m.sx <= SX_RAISE_EVA) {
            uint8_t w = CUAL[m.sx - SX_RAISE_ATK];
            if (att.applyStage(w, +1)) { r.stageWhich = w; r.stageDelta = +1; r.stageSelf = true; }
          } else if (m.sx >= SX_LOWER2_ATK && m.sx <= SX_LOWER2_EVA) {
            uint8_t w = CUAL[m.sx - SX_LOWER2_ATK];
            if (def.applyStage(w, -2, true)) {
              r.stageWhich = w; r.stageDelta = -2;
              picado(def, r);
            }
          } else if (m.sx >= SX_RAISE2_ATK && m.sx <= SX_RAISE2_EVA) {
            uint8_t w = CUAL[m.sx - SX_RAISE2_ATK];
            if (att.applyStage(w, +2)) { r.stageWhich = w; r.stageDelta = +2; r.stageSelf = true; }
          }
          break;
        }
      }
    }
    }
  }

  // ...pero el Enfado y el Alboroto no se quedan ATADOS a un golpe que no
  // entro. El bloque de abajo descuenta sus turnos y vive dentro del
  // `if` que exige que el golpe entrara —— correcto para Fire Spin, que no
  // atrapa a quien no toco, y fatal para el descuento: Enfado contra un
  // HADA (inmune) no bajaba nunca el contador y el bicho repetia un golpe
  // de 0 de dano el resto del combate. En los juegos la furia se corta ahi
  // mismo y se marea solo.
  if (!(r.effect > 0 && !r.missed)) rompeCadena(from, mv, r);

  // Estados de varios turnos. Van los ultimos porque solo prenden si el golpe
  // entro de verdad: fallar o pegarle a un inmune no ata a nadie.
  if (mv < MOVE_COUNT && !over && r.effect > 0 && !r.missed) {
    switch (MOVE_TBL[mv].fx) {
      case FX_BIND:
        // 4 o 5 turnos royendo. Si ya estaba atrapado no se reinicia el reloj,
        // que si no Fire Spin en bucle seria una carcel perpetua.
        if (def.bindTurns == 0) {
          def.bindTurns = (uint8_t)(4 + btRand(2));
          r.trapped = true;
        }
        break;
      case FX_LOCK:
        if (att.lockTurns == 0) {
          // este turno ya lo gasto; quedan 1 o 2 mas y despues el mareo
          att.lockTurns = (uint8_t)(1 + btRand(2));
          att.lockSlot = slot;
        } else if (--att.lockTurns == 0) {
          // se le acaba la furia y se marea solo: es el precio de los 120
          if (att.confuseTurns == 0) att.confuseTurns = (uint8_t)(2 + btRand(4));
          r.selfConfused = true;
        }
        break;
      case FX_UPROAR:
        // ALBOROTO: se encadena como Enfado pero sin marearse al final.
        // Lo suyo es el ruido: mientras dure NADIE duerme, y al empezar
        // despierta a los dos si hacia falta.
        if (att.lockTurns == 0) {
          att.lockTurns = 2;            // Uproar = 3 usos, fijo, en el Anil
          att.lockSlot = slot;
          uproar = (uint8_t)(att.lockTurns + 1);
        } else {
          --att.lockTurns;
          uproar = (uint8_t)(att.lockTurns + 1);
        }
        for (uint8_t q = 0; q < 2; q++)
          if (side[q].status == ST_SLEEP) {
            side[q].status = ST_NONE;
            side[q].statusTurns = 0;
          }
        break;
      case FX_ROLLOUT:
        // El precio de la bola de nieve: cuatro turnos mas sin elegir.
        // `lockTurns` ya hace todo lo que pide el Anil —— fuerza el
        // hueco (realSlot), no cobra PP y no deja relevar —— y ademas
        // es de donde sale el multiplicador, asi que la cadena baja
        // sola y el sexto uso vuelve a x1.
        if (att.lockTurns == 0) {
          att.lockTurns = 4;
          att.lockSlot = slot;
        } else {
          --att.lockTurns;
        }
        break;
      case FX_RECHARGE:
        att.recharging = true;   // el turno que viene se queda mirando
        break;
      default: break;
    }
  }
  // Si ya no hay rival en pie no tiene sentido seguir enfurecido ni cargando.
  if (over) { att.lockTurns = 0; att.chargingMove = 0xFFFF; att.hiding = HID_NONE; }

  // Life Orb: pega mas y te cobra un 10% de tu vida cada vez. Solo si el
  // golpe llego a hacer dano, que si no seria un castigo por usar Protect.
  //
  // Y NO se lo cobra a Guardia Magica. El Anil lo dice en una linea
  // —— `next if !user.takesIndirectDamage?` ——, y takesIndirectDamage?
  // devuelve false con MAGICGUARD. Aqui el peaje escribia `att.hp` a
  // mano, saltandose Combatant::tickDamage, que es la UNICA funcion que
  // mira la habilidad: la misma fuga de siempre, el estado escrito en
  // dos sitios.
  if (!over && r.damage && objeto(att).fx == IT_LIFE_ORB &&
      att.abil() != AB_MAGIC_GUARD && !fuerzaBruta(att, mv)) {
    uint16_t peaje = att.hpMax / 10 ? att.hpMax / 10 : 1;
    r.recoil = (uint16_t)(r.recoil + peaje);
    att.hp = (peaje >= att.hp) ? 0 : (uint16_t)(att.hp - peaje);
    if (att.hp == 0) knockOut(from);
  }

  // Choice Band/Specs: el +50% se paga quedandose atado al primer golpe.
  if (objeto(att).fx == IT_CHOICE && att.choiceSlot == 0xFF &&
      mv != MOVE_STRUGGLE)
    att.choiceSlot = slot;

  // Synchronize: el estado que te pongan se lo llevan ellos tambien. Va al
  // final, cuando ya se sabe si prendio.
  if (r.gotStatus && def.abil() == AB_SYNCHRONIZE &&
      (r.gotStatus == ST_BURN || r.gotStatus == ST_POISON ||
       r.gotStatus == ST_BADPOISON || r.gotStatus == ST_PARALYSIS)) {
    att.applyStatus(r.gotStatus, terrain, weather);
  }

  // TITIRITERO TOXICO: a quien envenena, tambien le marea. Va aqui,
  // cuando ya se sabe si el veneno prendio de verdad.
  if (att.abil() == AB_POISON_PUPPET && !over &&
      (r.gotStatus == ST_POISON || r.gotStatus == ST_BADPOISON) &&
      def.confuseTurns == 0 && !def.fainted()) {
    def.confuseTurns = (uint8_t)(2 + btRand(4));
    r.abilFired = att.ability;
  }

  // HUIDA: por debajo de media vida se larga. Como Ida y Vuelta, el
  // motor solo avisa; sacar a otro es cosa del banquillo.
  if (r.damage && !over && def.abil() == AB_WIMP_OUT && !def.fainted() &&
      !fuerzaBruta(att, mv) &&
      def.hpMax && def.hp * 2 <= def.hpMax &&
      (def.hp + r.damage) * 2 > def.hpMax &&
      from <= 1 && temReservas((uint8_t)(from ^ 1))) {
    if ((uint8_t)(from ^ 1) == BT_PLAYER) wantsSwitch = true;
    querTrocar[from ^ 1] = true;
    r.abilFired = def.ability;
    r.abilOfDef = true;
  }

  // Por donde iba: lo miran Disable, Encore y Spite del rival.
  att.lastMove = mv;
  // Forcejeo no esta en ningun hueco: Anular, Encore y Rencor fallan contra
  // el, y Tormento no lo cuenta como "el mismo golpe" (el Anil lo exime).
  att.lastSlot = (mv == MOVE_STRUGGLE) ? (uint8_t)0xFF : slot;

  // Si el golpe salio o no, para Pataleta. Los caminos que se van antes
  // (mofa, mordaza, ultima baza...) lo dejan puesto a mano al salir; los
  // que llegan hasta aqui se resuelven con lo que diga el resultado.
  // Fallar la punteria y no afectarle (inmune) son fallo; el Protect no.
  // Salpicadura "no tuvo ningun efecto", que en el Anil NO es fallar (lo
  // dice en pbEffectGeneral, no en pbMoveFailed?): aqui va con r.failed
  // porque es lo que hace que la pantalla lo cuente.
  const bool nada = mv < MOVE_COUNT && MOVE_TBL[mv].fx == FX_SPLASH;
  att.lastFailed = (r.failed && !nada) || r.missed ||
                   (r.effect == 0 && !r.blockedByProtect);

  // ESCUPIR vacia el deposito: la potencia ya se cobro en computeHit.
  // Y con el se van las subidas que Reserva habia dado, igual que con
  // Tragar: es el precio del golpe.
  if (mv < MOVE_COUNT && MOVE_TBL[mv].fx == FX_SPIT_UP && !r.failed)
    gastaReserva(att);

  // BAILARIN: si el otro acaba de bailar, el se copia el baile. Va aqui,
  // al final, con el golpe original ya resuelto del todo, y con un
  // guardia: sin el, dos Bailarines frente a frente se copiarian el uno
  // al otro para siempre. El golpe copiado no gasta PP (va forzado).
  if (!over && !dancing && mv < MOVE_COUNT && moveHas(mv, MV_DANCE) &&
      def.abil() == AB_DANCER && !def.fainted()) {
    dancing = true;
    danced = mv;
    useMove((uint8_t)(from ^ 1), 0, mv);
    dancing = false;
  }

  turn++;
  return r;
}

// A quien le hace pupa el cielo. La tormenta de arena respeta a los de roca,
// tierra y acero; el granizo, a los de hielo. Por eso Sandstorm en manos de un
// Onix no es un golpe neutro: le hace dano al otro y a el no.
static bool sufreElClima(uint8_t weather, const Combatant &c) {
  // Overcoat lleva abrigo y Magic Guard solo nota lo que le pegan de
  // frente: a ninguno de los dos le raspa el cielo.
  if (c.abil() == AB_OVERCOAT || c.abil() == AB_MAGIC_GUARD) return false;
  if (weather == WE_SAND) {
    uint8_t t1 = c.type1(), t2 = c.type2();
    return !(t1 == T_ROCK || t2 == T_ROCK || t1 == T_GROUND || t2 == T_GROUND ||
             t1 == T_STEEL || t2 == T_STEEL);
  }
  if (weather == WE_HAIL) return !(c.type1() == T_ICE || c.type2() == T_ICE);
  return false;
}

uint16_t Battle::endTurn(uint8_t who) {
  endHeal = 0;
  endHealItem = 0;
  // El par Fusion solo vale DENTRO del turno: al cerrarlo, se olvida.
  if (who == 1) { lastFusion = 0; normalEsElec = false; }
  // El aviso del golpe del futuro dura hasta que se cierra el turno
  // siguiente: es la ventana en la que la pantalla puede leerlo.
  if (who == 0) { futureHitOn = 0xFF; futureHitDmg = 0; }
  if (who == 0) { wishHealOn = 0xFF; wishHealAmt = 0; perishTick = 0xFF;
                  encoreFin = 0xFF; yawnDurmio = 0xFF; }
  // RESPIRO dura UN TURNO: al cerrarlo, el que lo uso vuelve a ser
  // volador. En el Anil es una linea del cierre de ronda
  // (028_Battle/012_Battle_EndOfRoundPhase.rb:800). Va ANTES del `over`
  // porque borrar una bandera de un turno no depende de que el combate
  // siga, y dejarsela puesta seria quitarle el tipo para siempre.
  if (who <= 1) side[who].roosted = false;
  // Y las dos de la ronda del Forcejeo, por lo mismo: son de UNA ronda.
  if (who <= 1) side[who].encoreSalta = false;
  if (who <= 1) side[who].forcejea = false;
  // Y lo que mira Pataleta: como le fue en ESTA ronda.
  if (who <= 1) side[who].lastRoundFailed = side[who].lastFailed;
  if (over || who > 1) return 0;
  uint16_t d = side[who].endOfTurnDamage();

  // El cielo cobra aparte del veneno, igual que el agarre. Magic
  // Guard tampoco lo nota: va en sufreElClima, con Overcoat.
  if (weather && !side[who].fainted() && sufreElClima(weather, side[who])) {
    Combatant &c = side[who];
    uint16_t mordisco = c.hpMax / 16 ? c.hpMax / 16 : 1;
    if (mordisco > c.hp) mordisco = c.hp;
    c.hp -= mordisco;
    d = (uint16_t)(d + mordisco);
  }

  Combatant &c = side[who];
  Combatant &otro = side[who ^ 1];
  endAbil = 0;

  // Habilidades de fin de turno. Van antes que los Restos porque curar dos
  // veces al que ya estaba lleno no cambia nada, y asi se leen juntas.
  if (!c.fainted()) {
    switch (c.abil()) {
      case AB_HEAL_WEATHER:            // Rain Dish, Ice Body
        if (weather == c.abilArg() && c.hp < c.hpMax) {
          uint16_t cura = c.hpMax / 16 ? c.hpMax / 16 : 1;
          uint16_t hueco = (uint16_t)(c.hpMax - c.hp);
          c.hp += (cura > hueco ? hueco : cura);
        }
        break;
      case AB_SHED_SKIN:               // Shed Skin, Natural Cure
        if (c.status != ST_NONE && (uint8_t)btRand(100) < c.abilArg()) {
          c.status = ST_NONE;
          c.statusTurns = 0;
          endAbil = c.ability;
        }
        break;
      case AB_HUNGER:
        // MAS QUE MONO: alterna cada turno. Lo que cambia con la
        // forma es el tipo de su golpe, y eso se mira en la potencia.
        // Como en el Anil, sin cartel: solo el dibujo cambia.
        c.hungry = !c.hungry;
        break;
      case AB_BAD_DREAMS:
        // PESADILLA: al RIVAL dormido le roe un octavo. Es la unica
        // habilidad de cierre de turno que le hace algo al otro.
        if (otro.status == ST_SLEEP && !otro.fainted()) {
          uint16_t roe = (uint16_t)(otro.hpMax / 8 ? otro.hpMax / 8 : 1);
          otro.hp = (roe >= otro.hp) ? 0 : (uint16_t)(otro.hp - roe);
          endAbil = c.ability;
          if (otro.hp == 0) knockOut((uint8_t)(who ^ 1));
        }
        break;
      // COSECHA no esta aqui: va al FINAL del turno, despues de las
      // bayas. Ver el bloque al cerrar endTurn.
      case AB_MOODY: {
        // VELETA: +2 a un stat al azar y -1 a otro, cada turno. Los
        // dos tienen que ser DISTINTOS, que si no se anularia solo.
        uint8_t sube = (uint8_t)btRand(SG_COUNT);
        uint8_t baja = (uint8_t)btRand(SG_COUNT);
        if (baja == sube) baja = (uint8_t)((baja + 1) % SG_COUNT);
        c.applyStage(sube, +2);
        c.applyStage(baja, -1);
        endAbil = c.ability;
        break;
      }
      case AB_SOLAR_POWER:
        // El precio del +50% al sol: un octavo por turno. Sin esto
        // seria una habilidad gratis, y no lo es.
        if (weather == WE_SUN && !c.fainted()) {
          uint16_t peaje = (uint16_t)(c.hpMax / 8 ? c.hpMax / 8 : 1);
          c.hp = (peaje >= c.hp) ? 1 : (uint16_t)(c.hp - peaje);
          endAbil = c.ability;
        }
        break;
      case AB_SPEED_BOOST:
        if (c.applyStage(SG_SPE, +1)) endAbil = c.ability;
        break;
      case AB_POISON_HEAL:
        // el veneno le CURA. endOfTurnDamage ya se lo cobro, asi que aqui se
        // devuelve el doble: lo que le quito mas lo que deberia haber ganado
        if (c.status == ST_POISON || c.status == ST_BADPOISON) {
          uint16_t cura = c.hpMax / 8 ? c.hpMax / 8 : 1;
          uint16_t hueco = (uint16_t)(c.hpMax - c.hp);
          c.hp += (cura > hueco ? hueco : cura);
        }
        break;
      case AB_CURE_WEATHER:
        // Hydration: con SU cielo se quita el estado de encima. Es la
        // pareja de Rain Dish, y la razon de que la lluvia no sea solo
        // para el agua.
        if (weather == c.abilArg() && c.status != ST_NONE) {
          c.status = ST_NONE;
          c.statusTurns = 0;
          endAbil = c.ability;
        }
        break;
      default:
        break;
    }
  }

  // Lleva un turno mas fuera. Lo mira Acometida, que solo vale el
  // primero. Va aqui, al cerrar, para que durante TODO el primer turno
  // el contador siga a cero.
  if (c.turnsOut < 255) c.turnsOut++;
  // Y se olvida quien se movio: empieza turno nuevo para este lado.
  movedThisTurn[who] = false;
  // El susto tambien se olvida. Dura UN turno (lo dice battle.h) y hasta
  // ahora solo se borraba al intentar moverse: si el marcado no llegaba a
  // mover —— porque ya se habia movido, porque cayo, porque hubo relevo ——
  // la marca cruzaba al turno siguiente y le robaba la vez.
  c.flinched = false;
  // El relevo ya se ha cobrado: Persecucion no sigue valiendo doble el
  // resto del combate.
  c.relieved = false;
  // El cielo pudo cambiar en mitad del turno, asi que a quien saca su
  // tipo del cielo (Veleta del tiempo) se le vuelve a calcular aqui.
  syncTipo(who);
  syncTipo((uint8_t)(who ^ 1));
  // Las formas de combate de cierre de ronda (Daruma, Banco, nucleo...).
  syncForma(who, 1);
  syncForma((uint8_t)(who ^ 1), 2);

  // CUERPO MORTAL: la cuenta corre para quien la lleve, sea el que la
  // puso o el que la recibio. Va FUERA del switch de habilidades: mi
  // primera version la metio dentro, detras de un `break`, y no se
  // ejecutaba nunca. Lo canto su propio test.
  if (c.perish != 0xFF && !c.fainted()) {
    if (c.perish == 0) {
      c.hp = 0;
      knockOut(who);
    } else {
      c.perish--;
      perishTick = who;   // la pantalla dice la cuenta que queda
    }
  }

  // El reloj del Alboroto, como el del clima, corre UNA vez por turno:
  // se descuenta al cerrar el segundo lado.
  if (who == 1 && uproar) uproar--;
  // Los deportes, lo mismo: son del campo, no de un bando.
  if (who == 1 && waterSport) waterSport--;
  if (who == 1 && mudSport) mudSport--;
  // El Viento Afin SI es por bando, asi que se descuenta con su lado.
  if (tailwind[who]) tailwind[who]--;
  // El Espacio Raro es del campo: una vez por turno, como el Alboroto.
  if (who == 1 && trickRoom) trickRoom--;

  // DESEO: se cumple al cerrar el turno SIGUIENTE al que se pidio, en el
  // lado de quien lo pidio, y cura al que este alli. Anticura lo tira.
  if (wishTurns[who] && --wishTurns[who] == 0) {
    Combatant &q = side[who];
    uint16_t cura = wishAmt[who];
    wishAmt[who] = 0;
    if (q.valid() && !q.fainted() && q.hp < q.hpMax && !q.healBlockTurns) {
      uint16_t hueco = (uint16_t)(q.hpMax - q.hp);
      if (cura > hueco) cura = hueco;
      q.hp = (uint16_t)(q.hp + cura);
      wishHealOn = who;
      wishHealAmt = cura;
    }
  }

  // PREMONICION: los dos relojes se miran al cerrar el segundo lado, por
  // lo mismo que el clima. Cuando uno llega a cero, el golpe cae sobre
  // el OTRO bando — el que lo lanzo ya puede no estar ahi.
  if (who == 1) {
    for (uint8_t q = 0; q < 2; q++) {
      if (!futureTurns[q] || --futureTurns[q]) continue;
      Combatant &victima = side[q ^ 1];
      const uint16_t mvF = futureMv[q];
      futureMv[q] = 0xFFFF;
      // El que lo lanzo: el de la pista si sigue ahi, y si no su foto. Si
      // esta debilitado (en pista o al irse), el golpe no llega.
      const bool fuera = futureFuera[q];
      const bool caido = fuera ? futureCaido[q] : side[q].fainted();
      futureFuera[q] = futureCaido[q] = false;
      if (caido || mvF >= MOVE_COUNT) continue;
      if (!victima.valid() || victima.fainted() || over) continue;
      // Se calcula AHORA: tipos del que esta, stats de este momento.
      HitResult f = computeHit(fuera ? futureDe[q] : side[q], victima, mvF,
                               weather, terrain, false, sportMask());
      if (f.effect == 0 || !f.damage) continue;     // no le afecta
      uint16_t golpe = f.damage;
      victima.hp = (golpe >= victima.hp) ? 0 : (uint16_t)(victima.hp - golpe);
      futureHitOn = (uint8_t)(q ^ 1);   // para que la pantalla lo cuente
      futureHitDmg = golpe;
      if (victima.hp == 0) knockOut((uint8_t)(q ^ 1));
    }
  }

  // Restos: un dieciseisavo por turno. Es poco, y justo por eso convierte un
  // combate largo en algo que se puede ganar aguantando.
  if (objeto(c).fx == IT_LEFTOVERS && !c.fainted() && c.hp < c.hpMax &&
      !c.healBlockTurns) {
    uint16_t cura = c.hpMax / 16 ? c.hpMax / 16 : 1;
    uint16_t hueco = (uint16_t)(c.hpMax - c.hp);
    uint16_t dada = (cura > hueco ? hueco : cura);
    c.hp += dada;
    // Y se APUNTA, para que la pantalla pueda decirlo. Sin esto cura en
    // silencio y a nivel bajo son uno o dos PS: el objeto parece roto.
    endHeal = dada;
    endHealItem = c.item;
  }

  // Aqua Ring cura antes de que la semilla chupe: asi los dos se notan en vez
  // de anularse segun el orden.
  // Arraigo cura igual que Velo Agua: la diferencia esta en el precio
  // (no poder irse), no en lo que cura.
  if ((c.aquaRing || c.ingrained) && !c.fainted() && c.hp < c.hpMax &&
      !c.healBlockTurns) {
    uint16_t cura = c.hpMax / 16 ? c.hpMax / 16 : 1;
    uint16_t hueco = (uint16_t)(c.hpMax - c.hp);
    uint16_t dada = (cura > hueco ? hueco : cura);
    c.hp += dada;
    endHeal = (uint16_t)(endHeal + dada);   // tan callados como los Restos
  }

  // MALDICION: un cuarto de los PS maximos, y no se quita con nada.
  // Va ANTES de la semilla por el mismo orden del Anil
  // (028_Battle/012_Battle_EndOfRoundPhase.rb, pbEOREffectDamage va
  // delante del dano por atrapar), y encima asi el que esta maldito Y
  // con semilla pierde el cuarto aunque la semilla lo tumbe despues.
  if (c.cursed && !c.fainted()) {
    uint16_t muerde = c.hpMax / 4 ? c.hpMax / 4 : 1;
    if (muerde > c.hp) muerde = c.hp;
    c.hp -= muerde;
    d = (uint16_t)(d + muerde);
  }

  // Leech Seed: lo que le quita se lo queda el otro. Por eso no es solo dano.
  if (c.leechSeed && !c.fainted()) {
    uint16_t chupa = c.hpMax / 8 ? c.hpMax / 8 : 1;
    if (chupa > c.hp) chupa = c.hp;
    c.hp -= chupa;
    d = (uint16_t)(d + chupa);
    if (!otro.fainted() && !otro.healBlockTurns) {
      uint16_t hueco = (uint16_t)(otro.hpMax - otro.hp);
      otro.hp += (chupa > hueco ? hueco : chupa);
    }
  }

  // Las bayas se comen SOLAS y se gastan. Van al final del turno, cuando ya
  // se sabe como ha quedado: asi la Sitrus salta justo cuando hace falta.
  //
  // Unnerve: el de enfrente le pone tan nervioso que no se la come. Es
  // una habilidad que no hace dano y aun asi cambia el combate.
  //
  // EL UMBRAL ES LA MITAD, y no un cuarto. Esto estuvo mal: habia aqui
  // un comentario diciendo que el cuarto "ademas es el de los juegos"
  // y que asi Glotoneria significaba algo. Las dos cosas eran falsas.
  //
  // El Anil (010_Battle_ItemEffects.rb) hace que Oran y Sitrus llamen a
  // `canConsumePinchBerry?(false)`, y esa funcion con el `false`:
  //     return true if @hp <= @totalhp / 4
  //     return true if @hp <= @totalhp / 2 && (!check_gluttony || GLUTTONY)
  // El segundo `return` dispara SIEMPRE cuando check_gluttony es false.
  // O sea: la mitad, para todo el mundo, y Glotoneria no pinta nada.
  //
  // Glotoneria es de las bayas de STAT (Liechi, Salac), que llaman con
  // `true`. De esas no tenemos ninguna, asi que aqui no tiene nada que
  // adelantar —— y por eso NO se le inventa un trabajo.
  //
  // A un cuarto la baya casi nunca llegaba a tiempo, que es justo la
  // sensacion de "este objeto no funciona" que reporto el usuario.
  {
    const ItemEntry &it = objeto(c);
    bool nervioso = otro.abil() == AB_UNNERVE && !otro.fainted();
    // Fracción de vida a la que salta: 1/BERRY_AT.
    const uint16_t BERRY_AT = 2;
    if (nervioso) {
      // ni la toca
    } else if (!c.fainted() && esBayaDeCura(it) && !c.healBlockTurns &&
               c.hp * BERRY_AT <= c.hpMax) {
      uint16_t cura = curaDeBaya(it, c.hpMax);
      uint16_t hueco = (uint16_t)(c.hpMax - c.hp);
      c.hp += (cura > hueco ? hueco : cura);
      c.lastBerry = c.item;    // cual era, para que Cosecha la devuelva
      c.perderObjeto(true);
      c.ateBerry = true;       // ya hay algo que eructar
    } else if (!c.fainted() && it.fx == IT_BERRY_CURE &&
               (c.status != ST_NONE || c.confuseTurns > 0)) {
      // LA CONFUSION TAMBIEN. En el Anil la baya salta si hay estado O
      // confusion, y limpia las DOS: pbCureStatus seguido de
      // pbCureConfusion (010_Battle_ItemEffects.rb:532). Aqui solo se
      // miraba `status`, asi que un bicho SOLO confuso se quedaba
      // confuso con la baya en la mano sin llegar a comersela nunca,
      // y el que la usaba por el estado seguia mareado despues.
      //
      // Es el mismo agujero que tenia la Full Heal, ya arreglado: la
      // confusion va APARTE del estado principal (se acumulan), y
      // quien la olvida la deja fuera de todo lo que limpia.
      c.status = ST_NONE;
      c.statusTurns = 0;
      c.confuseTurns = 0;
      c.lastBerry = c.item;
      c.perderObjeto(true);
      c.ateBerry = true;
    }
  }

  // COSECHA, y AQUI: despues de las bayas, no antes.
  //
  // La mitad de las veces le vuelve la baya que se comio; al sol,
  // siempre, y solo con las manos libres. Estaba en el bloque de las
  // habilidades, que va ANTES del de las bayas, asi que en el mismo
  // turno se la devolvia y se la volvia a comer: con poca vida, una
  // baya por turno para siempre. No saltaba por aritmetica —— con el
  // umbral en un cuarto, la Sitrus dejaba la vida justo por encima.
  //
  // El Anil la tiene en EndOfRoundGainItem, que es el ULTIMO paso del
  // turno (012_Battle_EndOfRoundPhase.rb:752), detras de todos los
  // pbItemHPHealCheck. Por eso alli la baya devuelta espera al turno
  // siguiente, que es lo que tiene que pasar.
  if (c.abil() == AB_HARVEST && !c.fainted() &&
      c.lastBerry != IT_NONE && c.item == IT_NONE &&
      (weather == WE_SUN || btRand(100) < 50)) {
    c.item = c.lastBerry;
    if (c.itemComido == c.item) c.itemComido = IT_NONE;
    c.lastBerry = IT_NONE;
    endAbil = c.ability;
  }

  // Lo que dura un solo turno se apaga aqui. Y si NO se protegio este turno,
  // la racha se rompe: encadenar Protect solo penaliza si es seguido.
  if (!c.protecting && !c.enduring) c.protectRun = 0;
  c.protecting = false;
  c.enduring = false;
  c.tookPhys = c.tookSpec = 0;
  // OJO: aqui ANTES se borraban tambien tipoPerdido, saltCure, expuesto,
  // noRepetir y hitsTaken, con un comentario que decia "al retirarse se
  // olvida". Retirarse es el RELEVO, y esto es el cierre de TURNO: se
  // estaban borrando una vez por turno y cinco mecanicas no funcionaban.
  // Punorrabia no pasaba de la potencia minima, Salazon roia un turno,
  // Martillo Giga se podia encadenar, el tipo quemado volvia al turno
  // siguiente y Asalto Espada no llegaba a cobrarse nunca. Ahora van
  // donde les toca: en sendOut().

  // El reloj de las pantallas corre en el LADO, no en el bicho: si corre
  // en el bicho, relevar lo para (o lo duplica).
  for (uint8_t k = 0; k < PA_COUNT; k++)
    if (pantalla[who][k]) pantalla[who][k]--;
  espejaPantallas(who);
  if (c.tauntTurns) c.tauntTurns--;
  c.beakBlast = false;
  c.shellTrap = false;
  // BOSTEZO: se duerme al cerrar el turno SIGUIENTE, si aun puede.
  if (c.yawn && --c.yawn == 0 && !c.fainted() &&
      c.applyStatus(ST_SLEEP, terrain, weather, uproar > 0))
    yawnDurmio = who;
  if (c.embargoTurns) c.embargoTurns--;
  if (c.healBlockTurns) c.healBlockTurns--;
  if (c.gagTurns) c.gagTurns--;
  if (c.levitating) c.levitating--;
  // El Conjuro y el Anticipo son de BANDO. El Anticipo dura ESTE
  // turno, asi que se apaga; el Conjuro cuenta cinco.
  if (luckyChant[who]) luckyChant[who]--;
  c.chanted = luckyChant[who] > 0;
  quickGuard[who] = false;
  // La venganza dura UN turno: si durase mas, Represalia pegaria el
  // doble el resto del combate por una caida de hace diez turnos.
  //
  // Pero "un turno" es el turno SIGUIENTE a la caida, que es cuando el
  // que entro llega a moverse. Esto la apagaba en el cierre del MISMO
  // turno en que cayo el companero, asi que no llegaba viva a ninguna
  // parte. Y el relevo la borraba antes todavia, al copiar la ficha
  // guardada: por eso ahora el contador es del LADO.
  if (vengefulSide[who]) vengefulSide[who]--;
  c.vengeful = vengefulSide[who] > 0;
  if (c.disableTurns && --c.disableTurns == 0) c.disabledSlot = 0xFF;
  // Y se acaba tambien si al golpe se le gastaron los PP.
  if (c.encoreTurns && !c.fainted() &&
      (--c.encoreTurns == 0 ||
       (c.encoreSlot < BT_MOVES && !c.usesLeft[c.encoreSlot]))) {
    c.encoreTurns = 0;
    c.encoreSlot = 0xFF;
    encoreFin = who;
  }

  // El campo de hierba cura a quien lo pisa. Va aqui, con el desgaste del
  // clima, porque es el mismo momento del turno; el orden importa poco
  // porque curar no puede tumbar a nadie.
  if (terrain == TR_GRASS && !c.fainted() && c.grounded() && c.hp < c.hpMax) {
    uint16_t cura = (uint16_t)(c.hpMax / GRASSY_HEAL_DIV);
    if (!cura) cura = 1;
    uint16_t hueco = (uint16_t)(c.hpMax - c.hp);
    c.hp += (cura > hueco ? hueco : cura);
  }

  // El reloj del clima corre UNA vez por turno, no una por bando: se descuenta
  // al cerrar el segundo lado. El del suelo igual.
  if (who == 1 && weatherTurns) {
    if (--weatherTurns == 0) weather = WE_NONE;
  }
  if (who == 1 && terrainTurns) {
    if (--terrainTurns == 0) terrain = TR_NONE;
  }

  if (side[who].fainted()) knockOut(who);
  return d;
}

void Battle::knockOut(uint8_t quien) {
  // MOXIE (y sus primos): tumbar a alguien le crece. El stat sale del
  // arg, porque Chilling Neigh sube el ataque y Grim Neigh el
  // especial. Va antes de decidir si el combate se acaba: la subida
  // vale igual, y con banquillo el combate sigue.
  Combatant &ganador = side[quien ^ 1];
  if (!ganador.fainted() && ganador.abil() == AB_MOXIE)
    ganador.applyStage(ganador.abilArg(), +1);
  // ALMA FUERTE: igual que Moxie pero el stat NO es fijo: es el mas
  // alto que tenga en ese momento, asi que depende del bicho y de lo
  // que se haya subido ya. Por eso no puede ir en el arg.
  if (!ganador.fainted() && ganador.abil() == AB_BEAST_BOOST) {
    static const uint8_t CUAL[5] = { SG_ATK, SG_DEF, SG_SPA, SG_SPD, SG_SPE };
    uint16_t v[5] = { ganador.atkStat(), ganador.defStat(),
                      ganador.spAtkStat(), ganador.spDefStat(),
                      ganador.speStat() };
    uint8_t mejor = 0;
    for (uint8_t k = 1; k < 5; k++) if (v[k] > v[mejor]) mejor = k;
    ganador.applyStage(CUAL[mejor], +1);
  }
  // Al LADO que acaba de perder a uno le toca Represalia: el siguiente
  // que salga pega el doble con ella. Dura un turno (ver endTurn).
  // Dos cierres de turno: el de ESTE turno (que lo baja a 1) y el del
  // siguiente, que es cuando el que entro ya se ha podido mover con ella.
  vengefulSide[quien] = 2;
  side[quien].vengeful = true;

  // Con equipo, caer no es perder: es tener que sacar a otro. El combate
  // solo se acaba cuando a ese bando no le queda nadie. El rival nunca trae
  // `playerReserves` — sus relevos de gimnasio los saca la pantalla mirando
  // `over` —, asi que para el la regla sigue siendo la de siempre.
  if (temReservas(quien)) return;
  over = true;
  winner = quien ^ 1;
}

// Saca a otro a la pista. Lo que se va se lleva sus PS y su estado; lo que
// NO viaja es todo lo volatil (stages, confusion, agarre, furia, carga):
// eso es del turno y del bicho que estaba, no del que entra. Es justo por
// esto por lo que cambiar tiene precio y sentido.
// REGENERATOR: al retirarse recupera un tercio. Va APARTE de sendOut
// porque quien maneja el banquillo guarda al que sale ANTES de meter
// al nuevo; curando dentro de sendOut la cura llegaria tarde y se
// perderia. Cura en el sitio y devuelve cuanto, para poder contarlo.
uint16_t Battle::regenOnSwitch(uint8_t quien) {
  // DE CERO A HEROE: la forma heroica la gana AL RETIRARSE, asi que
  // se marca aqui, que es a quien se va a quien se llama. Va antes
  // del return de Regenerator para que valga aunque no tenga esa.
  if (quien <= 1 && side[quien].abil() == AB_HERO)
    side[quien].hero = true;
  // BATERIA y PUNTO FUERTE dejan preparado al siguiente. Va aqui, en el
  // mismo sitio que De Cero a Heroe, porque es el unico punto donde se
  // sabe QUIEN se esta retirando.
  if (quien <= 1 && side[quien].abil() == AB_RELAY_BOOST)
    relayBoost[quien] = side[quien].abilArg();
  if (quien <= 1 && side[quien].abil() == AB_RELAY_CURE)
    relayCure[quien] = true;
  if (quien > 1) return 0;
  Combatant &c = side[quien];
  if (!c.valid() || c.fainted() || c.abil() != AB_REGENERATOR ||
      c.hp >= c.hpMax)
    return 0;
  uint16_t cura = c.hpMax / 3 ? c.hpMax / 3 : 1;
  uint16_t hueco = (uint16_t)(c.hpMax - c.hp);
  if (cura > hueco) cura = hueco;
  c.hp = (uint16_t)(c.hp + cura);
  return cura;
}

void Battle::sendOut(uint8_t quien, const Combatant &c, bool porGusto) {
  // PREMONICION en camino y el que la lanzo se va: se guarda su foto, sin
  // subidas (el Anil lo reconstruye del equipo), y si se va debilitado.
  if (quien <= 1 && futureTurns[quien] && !futureFuera[quien]) {
    futureFuera[quien] = true;
    futureCaido[quien] = side[quien].fainted();
    futureDe[quien] = side[quien];
    for (uint8_t i = 0; i < SG_COUNT; i++) futureDe[quien].stage[i] = 0;
  }
  side[quien] = c;
  Combatant &yo = side[quien];
  yo.bform = -1;          // la forma de combate era del que estaba
  yo.illusion = false;    // la vuelve a encender alEntrar, si toca
  yo.turnsOut = 0;        // acaba de salir: Acometida vuelve a contar
  // Se pone SIEMPRE (no solo cuando es true) porque `c` sale de una ficha
  // guardada y podria traerlo pegado de un relevo anterior.
  yo.relieved = porGusto;
  Combatant &otro = side[quien ^ 1];
  for (uint8_t i = 0; i < SG_COUNT; i++) yo.stage[i] = 0;
  yo.confuseTurns = 0;
  yo.bindTurns = 0;
  yo.lockTurns = 0;
  yo.lockSlot = 0xFF;
  yo.recharging = false;
  // 0xFFFF es "no esta cargando"; 0 es el golpe STRUGGLE. Aqui habia un
  // 0, asi que todo el que entraba a pista quedaba marcado como a medio
  // cargar —— y giveOrder no tira el dado de la desobediencia cuando el
  // golpe no lo eliges tu, de modo que el vinculo bajo dejaba de costar
  // nada en cuanto se relevaba. Pasaba callado porque el bucle que busca
  // el hueco del golpe cargado no casa nunca: STRUGGLE no esta en la
  // lista de golpes de nadie.
  yo.chargingMove = 0xFFFF;
  yo.soltoAlgo = false;   // lo que largo el anterior no le hace ligero
  yo.hiding = HID_NONE;   // el que entra entra a la pista, no a un agujero
  yo.cursed = false;      // la maldicion era del que estaba, no del que entra
  yo.ligero = 0;          // el peso que se quito Autotomia vuelve al salir
  yo.raging = false;      // la furia es del que estaba, no del que entra
  yo.gagTurns = 0;        // al que entra no le han callado
  yo.stockpile = 0;       // el deposito no viaja
  yo.tormented = false;   // ni el tormento
  yo.ingrained = false;   // las raices eran del que se fue
  yo.imprisoning = false; // el sello se fue con quien lo puso
  // Lo que es del BICHO EN PISTA y no del bicho: al retirarse se olvida.
  // El tipo quemado vuelve (en los juegos tambien), la sal se cae con el,
  // la exposicion se va y el martillo se puede volver a usar. Esto vivia
  // por error en el cierre de turno, donde se borraba cada turno.
  yo.tipoPerdido = 0;
  yo.roosted = false;     // el Respiro era del que estaba en pista
  yo.saltCure = 0;
  yo.subHp = 0;           // el muneco era del que se fue
  yo.perish = 0xFF;       // relevarse es justo como se escapa del Canto
  yo.embargoTurns = 0;
  yo.healBlockTurns = 0;
  yo.yawn = 0;            // relevarse tambien libra del Bostezo
  yo.forcejea = false;    // eran de la ronda del que se fue
  yo.encoreSalta = false;
  // El aviso era del que se fue: no se lo cuelgues al que entra.
  if (yawnDurmio == quien) yawnDurmio = 0xFF;
  if (encoreFin == quien) encoreFin = 0xFF;
  yo.bideTurns = 0;       // la Venganza se corta al salir
  yo.bideDmg = 0;
  yo.beakBlast = yo.shellTrap = false;
  yo.expuesto = false;
  yo.mirado = false;      // el ojo estaba puesto en el que se fue
  yo.noRepetir = 0;
  yo.hitsTaken = 0;
  // `abilOff` NO se borra: Bilis anula la habilidad DE ESE BICHO, y
  // sigue anulada cuando vuelva a salir. Es lo que lo hace valer.
  yo.lastFailed = false;
  yo.lastRoundFailed = false;
  yo.noRetreat = false;   // el Bastion era del que estaba en pista
  // `ateBerry` NO se borra: la baya se la comio ESE bicho y sigue siendo
  // verdad cuando vuelva a salir. Es lo unico de aqui que es suyo y no
  // del turno.
  yo.protecting = false;
  yo.enduring = false;
  yo.protectRun = 0;
  // DESEO CURACION / DANZA LUNAR: el que entra por este lado llega
  // entero. Va aqui, con el bicho ya copiado, porque es a EL a quien le
  // toca. Sin esto los dos golpes eran solo el precio: caer y nada mas.
  if (deseoCura[quien] && yo.valid() && !yo.fainted()) {
    yo.hp = yo.hpMax;
    yo.status = ST_NONE;
    yo.statusTurns = 0;
    yo.confuseTurns = 0;
    if (deseoCura[quien] == 2)       // Danza Lunar repone tambien los PP
      for (uint8_t i = 0; i < yo.nMoves && i < BT_MOVES; i++)
        if (yo.moves[i] < MOVE_COUNT) yo.usesLeft[i] = MOVE_TBL[yo.moves[i]].pp;
    deseoCura[quien] = 0;
  }

  // Lo que es del LADO y no del bicho se vuelve a poner: la ficha
  // guardada no lo sabe. Sin esto, Represalia moria justo al entrar el
  // que tenia que cobrarla, y el que entraba se quedaba sin el Canto
  // Suerte que su lado tenia puesto.
  yo.vengeful = vengefulSide[quien] > 0;
  yo.chanted = luckyChant[quien] > 0;
  // Y las pantallas del bando, que el que entra hereda: son suyas por
  // estar en ese lado, no por ser el bicho que las puso.
  espejaPantallas(quien);
  // El que se queda pierde lo que tenia agarrado AL QUE SE FUE
  otro.bindTurns = 0;
  // Y la PRESA MAXILAR se suelta: muerde a los dos y se acaba en cuanto
  // CUALQUIERA de los dos sale del campo. Esto no lo borraba nadie, asi
  // que al caer el mordido el que mordia se quedaba preso el resto del
  // combate —— sin poder relevar ni huir.
  otro.trapped2 = false;

  // Las habilidades de salida saltan tambien al relevar: Intimidate del que
  // entra baja el ataque del que estaba, como en los juegos.
  startAbil[quien] = 0;
  // VELO VAPOR tambien saca el sol AL RELEVAR, no solo al empezar: es
  // una habilidad de entrada como Drought, y dejarla solo en el
  // arranque seria que el Mega Arcanine no hiciera nada si entra de
  // relevo, que es justo cuando entra un mega.
  alEntrar(quien);
  habilidadDeEntrada(quien);

  // TELEPATHY al relevar. En los juegos esquiva el golpe de tu
  // companero y aqui no hay dobles, asi que vale para la otra cosa
  // que se le parece: entrar SIN comerse el golpe gratis que
  // normalmente te llevas al cambiar. Le lee la intencion y se aparta.
  //
  // Dura ese turno y nada mas: cubre el momento en que cambiar duele,
  // no el combate entero.
  //
  // Va AQUI ABAJO y no junto al resto de la entrada porque arriba se
  // limpia `protecting` (es un flag de un solo turno) y ponerlo antes
  // era ponerlo para que lo borraran dos lineas despues. Me costo un
  // test en rojo darme cuenta.
  if (yo.abil() == AB_TELEPATHY) yo.protecting = true;

  // Frisk tambien se asoma al relevar, no solo al empezar el combate.
  forewarned[quien] = 0xFFFF;
  if (yo.abil() == AB_FOREWARN) {
    forewarned[quien] = golpeMasFuerte(otro);
    if (forewarned[quien] != 0xFFFF) startAbil[quien] = yo.ability;
  }
  frisked[quien] = IT_NONE;
  if (yo.abil() == AB_FRISK && otro.item != IT_NONE)
    frisked[quien] = otro.item;

  // RELEVO: el que se fue le dejo sus stats. Va DESPUES de haberlos
  // puesto a cero arriba, que es justo lo que el golpe se salta.
  if (baton[quien]) {
    for (uint8_t i = 0; i < SG_COUNT; i++) yo.stage[i] = batonStage[quien][i];
    baton[quien] = false;
  }

  // ...y lo que le dejo preparado el que se fue (Bateria, Punto Fuerte).
  // Se pone SIEMPRE, tambien a cero: si no, el refuerzo se quedaria
  // pegado al siguiente y al siguiente.
  yo.charged = relayBoost[quien];
  relayBoost[quien] = 0;
  if (relayCure[quien]) {
    yo.status = ST_NONE;
    yo.statusTurns = 0;
    relayCure[quien] = false;
  }

  // LAS TRAMPAS cobran al entrar, y en este orden: primero las que
  // pican y luego las que ensucian. Es el momento en el que un relevo
  // deja de ser gratis.
  //
  // Piedras: un octavo, y mas si el tipo las sufre. Puas: solo a quien
  // pisa el suelo, porque son puas y estan en el suelo.
  // Las trampas NO tumban: dejan en 1 PS como poco. En los juegos si
  // tumban, pero aqui sendOut lo llama la pantalla EN MEDIO de un
  // relevo, y caerse ahi dejaria el combate pidiendo otro relevo dentro
  // del que se esta haciendo. Es una decision, no un descuido: cuando
  // el relevo pase a ser cosa del motor, se quita el suelo de 1 PS.
  hazardHit = 0;
  if (!yo.fainted()) {
    if (rocks[quien]) {
      uint16_t ef = typeEffect(T_ROCK, yo.type1(), yo.type2());
      uint16_t pico = (uint16_t)((uint32_t)yo.hpMax * ef / 100 / 8);
      if (!pico && ef) pico = 1;
      if (pico >= yo.hp) pico = (uint16_t)(yo.hp - 1);
      yo.hp = (uint16_t)(yo.hp - pico);
      if (pico) hazardHit = 1;
    }
    if (spikes[quien] && yo.grounded()) {
      // un octavo, un sexto, un cuarto
      uint8_t div = spikes[quien] == 1 ? 8 : spikes[quien] == 2 ? 6 : 4;
      uint16_t pico = (uint16_t)(yo.hpMax / div ? yo.hpMax / div : 1);
      if (pico >= yo.hp) pico = (uint16_t)(yo.hp - 1);
      yo.hp = (uint16_t)(yo.hp - pico);
      if (pico) hazardHit = 1;
    }
    if (tspikes[quien] && yo.grounded()) {
      if (yo.applyStatus(tspikes[quien] >= 2 ? ST_BADPOISON : ST_POISON,
                         terrain, weather, uproar > 0))
        hazardHit = 1;
    }
    if (web[quien] && yo.grounded() && yo.applyStage(SG_SPE, -1, true))
      hazardHit = 1;
  }
}

uint8_t Battle::pickFoeMove() const {
  const Combatant &att = side[BT_FOE];
  const Combatant &def = side[BT_PLAYER];
  uint8_t mejor = 0xFF;
  uint32_t mejorDano = 0;
  for (uint8_t i = 0; i < att.nMoves; i++) {
    // Antes aqui solo se miraban los PP, y el rival elegia tan a gusto un
    // golpe de sonido con Mordaza encima o el hueco anulado por Anular:
    // turno perdido con un "But it failed!" que desde fuera parece un
    // fallo del juego.
    if (slotBloqueado(BT_FOE, i)) continue;
    const uint16_t mv = att.moves[i];
    if (mv >= MOVE_COUNT) continue;
    const MoveEntry &m = MOVE_TBL[mv];
    uint32_t est;
    if (m.power == 0) {
      // Golpe de apoyo. Vale la pena de vez en cuando, no siempre: si se
      // valorase como los de dano, un salvaje con Swords Dance se pasaria el
      // combate bailando. Y curarse solo tiene sentido estando tocado.
      uint8_t fx = MOVE_TBL[mv].fx;
      bool util = (fx == FX_HEAL_HALF || fx == FX_HEAL_WEATHER ||
                   fx == FX_REST || fx == FX_WISH)
                      ? (att.hp * 2 < att.hpMax)
                      : (btRand(4) == 0);
      // Los de la segunda leva que, puestos, no hacen nada (o deshacen
      // lo suyo: un segundo Espacio Raro lo QUITA).
      if ((fx == FX_SUBSTITUTE && (att.subHp || att.hp * 4 <= att.hpMax)) ||
          (fx == FX_WISH && wishTurns[BT_FOE]) ||
          (fx == FX_PERISH && (def.perish != 0xFF || att.perish != 0xFF)) ||
          (fx == FX_EMBARGO && (def.embargoTurns || def.item == IT_NONE)) ||
          (fx == FX_HEALBLOCK && def.healBlockTurns) ||
          (fx == FX_TRICKROOM && trickRoom) ||
          (fx == FX_NORETREAT && att.noRetreat))
        util = false;
      est = util ? 70 * 100 : 0;
    } else {
      // estimacion sin azar: potencia x efectividad, y el STAB si toca
      est = (uint32_t)m.power * typeEffect(m.type, def.type1(), def.type2());
      if (m.type == att.type1() || (att.type2() && m.type == att.type2()))
        est = est * 3 / 2;
    }
    if (est > mejorDano || mejor == 0xFF) { mejorDano = est; mejor = i; }
  }
  // Si TODO esta bloqueado no hay nada bueno que elegir: se devuelve el
  // primero con PP y que useMove diga lo que toque (o forcejee). Sin esto
  // se devolveria 0xFF, que no es un hueco.
  if (mejor == 0xFF) {
    for (uint8_t i = 0; i < att.nMoves; i++)
      if (att.usesLeft[i] > 0) return i;
    return 0;
  }
  return mejor;
}
