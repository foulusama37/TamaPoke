#include "pet.h"

#include <string.h>   // memcpy: el equipo y el banco se copian en bloque

#include <FS.h>
#include <SD_MMC.h>
#include "dex.h"
#include "moves.h"
#include "item_names.h"   // a ordem alfabetica e a do nome que se le
#include "audio.h"
#include "tms_v304.h"

// Partida nova: o /save da velha sai inteiro, senao os times dela voltariam
// nas regioes desta.
static void svBaseRegiao(uint8_t r, char *out, size_t n);   // mais abaixo
static void svApagaTudo() {
  char p[40];
  for (uint8_t r = 0; r <= REGION_COUNT; r++) {
    char base[24];
    if (r < REGION_COUNT) svBaseRegiao(r, base, sizeof(base));
    else snprintf(base, sizeof(base), "/save/estado");
    snprintf(p, sizeof(p), "%s.bin", base);
    if (SD_MMC.exists(p)) SD_MMC.remove(p);
    snprintf(p, sizeof(p), "%s.new", base);
    if (SD_MMC.exists(p)) SD_MMC.remove(p);
  }
}

void Pet::begin() {
  prefs.begin("tamapoke", false);
  if (!prefs.getBool("init", false)) {
    svApagaTudo();
    prefs.putBool("init", true);
    newEgg();
  } else {
    load();
  }
  // O pacote das bolas novas, UMA vez por partida: quem ja jogava antes
  // delas recebe ao carregar, e a partida nova logo no comeco.
  // Sem sorteio: o pacote e sempre o mesmo (uma rapida, uma pelo nivel
  // e uma amiga), e assim abrir uma partida nao gasta o random() que os
  // genes e os encontros usam.
  if (!prefs.getBool("bx1", false)) {
    static const uint8_t BOAS_VINDAS[] = { BALLK_QUICK, BALLK_LEVEL, BALLK_FRIEND };
    for (uint8_t k : BOAS_VINDAS)
      for (uint8_t i = BALL_TIERS; i < BALL_ALL_COUNT; i++)
        if (ITEM_TBL[BALL_ALL[i]].arg == k && bag[BALL_ALL[i]] < 99) bag[BALL_ALL[i]]++;
    prefs.putBool("bx1", true);
    save();
  }
  lastTick = millis();
}

void Pet::newEgg() {
  ceremony = CER_NONE;
  neglectTicks = 0;
  weight = 0;
  speciesId = -1;
  prevSpeciesId = -1;
  // Un huevo de despedida es de la MISMA especie que el que se fue: eso es
  // lo que hace que sea "su" huevo y no otro bicho cualquiera. Lo nuevo
  // llega capturando; el huevo trae CALIDAD.
  eggTarget = legacyDex > 0 ? legacyDex : pickEggSpecies();
  starterPick = (registeredCount() == 0);  // primera partida: el jugador elige inicial
  // Sorteo shiny. Un huevo cualquiera es 1/48; uno de DESPEDIDA es la via
  // buena de verdad, porque cuesta tres dias de cuidado y renunciar al
  // bicho que tienes. Con el linaje al maximo baja hasta 1/4: es el premio
  // gordo de todo el ciclo y tiene que sentirse asi.
  int shinyBase;
  if (legacyDex > 0) {
    shinyBase = 20 - careBonus() - (legacyFloor - LEGACY_FLOOR_BASE);
    if (shinyBase < 4) shinyBase = 4;
  } else {
    shinyBase = 48 - careBonus();
    if (shinyBase < 16) shinyBase = 16;
  }
  eggShiny = (random(shinyBase) == 0);
  eggTaps = 0;
  fullness = 80;
  joy = 80;
  energy = 80;
  hygiene = 100;
  poops = 0;
  ageMinutes = 0;
  exp = 1;
  careMistakes = 0;
  mistakeCooldown = 0;
  sleeping = false;
  hpLeft = 0;
  hpSeen = 0;   // 0 = "sin estrenar": hpSync() lo dejara entero al eclosionar
  for (uint8_t i = 0; i < 4; i++) { ppMove[i] = 0; ppLeft[i] = 0; }
  movesPicked = false;   // otro bicho, otra eleccion
  ability = 0;           // se sorteara al eclosionar
  save();
}

// progresion offline: el tiempo paso aunque estuviera apagado, pero con
// piedad — las barras bajan con suelo en 15 (vuelve hambriento, no muerto),
// sin descuidos ni escapadas en ausencia
static uint8_t dropTo(uint8_t v, uint8_t d, uint8_t fl) {
  if (v <= fl) return v;
  return (v - fl > d) ? v - d : fl;
}

void Pet::guardar(uint32_t nowEpoch) {
  guardado = true;
  if (nowEpoch) lastSeenEpoch = nowEpoch;
  save();   // sincrono: detras de esto viene el corte de corriente
}

void Pet::setClock(uint32_t nowEpoch) {
  lastSeenEpoch = nowEpoch;
  if (nowEpoch) save();  // persiste ya: un corte de luz no pierde la referencia
}

void Pet::syncClock(uint32_t nowEpoch) {
  uint32_t seen = prefs.getUInt("seen", 0);
  lastSeenEpoch = nowEpoch;
  if (nowEpoch == 0) return;
  // LA PAUSA. Se apago guardando, asi que el tiempo de fuera no cuenta:
  // el bicho vuelve exactamente como se dejo, estuviera un rato o tres
  // meses en un cajon. El `if (guardado)` esta mas abajo, DESPUES de
  // calcular los minutos, y no es casualidad: ver el comentario de ahi.
  uint32_t mins = (seen && nowEpoch > seen) ? (nowEpoch - seen) / 60 : 0;
  if (mins < 2 || ceremony != CER_NONE || starterPick) {
    // La marca de pausa NO se gasta aqui. Un arranque que no aplica tiempo
    // no es "la vuelta": el AXP2101 vuelve a encender solo si enchufas el
    // USB, y un rearranque asi se comia la pausa sin usarla. La siguiente
    // ausencia de verdad ya no estaba protegida y el bicho volvia rojo.
    save();
    return;
  }
  // De aventura: el tiempo de fuera es de la aventura, no del hambre. La marca
  // de pausa se GASTA igual, que esta es la vuelta buena: si se quedara, el
  // proximo apagado normal, ya sin aventura, contaria como pausa.
  if (avDesde) {
    guardado = false;
    save();
    return;
  }
  // Y AQUI si: hay tiempo que aplicar, o sea que esta es la vuelta buena.
  // La marca vale para UNA: si la proxima vez se apaga a lo bruto (o se
  // acaba la bateria), el tiempo corre como siempre.
  if (guardado) {
    guardado = false;
    save();
    return;
  }
  if (mins > 14UL * 24 * 60) mins = 14UL * 24 * 60;  // tope: 2 semanas

  for (uint32_t i = 0; i < mins; i++) {
    ageMinutes++;
    if (isEgg()) {
      if (ageMinutes >= 3) hatch();  // eclosiona en tu ausencia
      continue;
    }
    if (sleeping) {  // descanso: baja lento y con suelo, igual que en vivo
      energy = clamp100(energy + 6);
      if (ageMinutes % 2 == 0) {
        fullness = dropTo(fullness, 1, 30);
        joy = dropTo(joy, 1, 35);
      }
      if (ageMinutes % 3 == 0) hygiene = dropTo(hygiene, 1, 45);
      continue;
    }
    fullness = dropTo(fullness, 2, 15);
    energy = dropTo(energy, 1, 15);
    hygiene = dropTo(hygiene, 1, 15);
    joy = dropTo(joy, 1, 15);
  }
  if (!isEgg()) {
    if (!sleeping) {  // durmiendo no ensucia
      uint8_t p = poops + mins / 240;
      poops = p > 3 ? 3 : p;
    }
    // la evolucion NO se aplica offline: queda lista y la dispara el usuario
    // tocando al bicho cuando vuelve (para que vea la transformacion)
  }
  Serial.printf("offline: %u min aplicados (nv.%u)\n", mins, level());
  save();
}

void Pet::update(uint32_t nowMs) {
  // fin de ceremonia: la criatura se va y queda un huevo nuevo
  if (ceremony != CER_NONE && !timeLeft(ceremonyUntil)) {
    newEgg();
    return;
  }
  while (nowMs - lastTick >= PET_TICK_MS) {
    lastTick += PET_TICK_MS;
    tick();
  }
}

// ------------------------------------------------------------ la aventura
static uint32_t avMezcla(uint32_t seed, uint32_t a, uint32_t b) {
  uint32_t x = seed * 2654435761u + a * 40503u + b * 2166136261u;
  x ^= x >> 15;
  x *= 2246822519u;
  x ^= x >> 13;
  return x;
}

bool Pet::aventuraComeca(uint32_t nowEpoch, uint32_t seed) {
  if (isEgg() || ceremony != CER_NONE || starterPick || !nowEpoch || avDesde) return false;
  avDesde = nowEpoch;
  avSeed = seed ? seed : 1;
  save();
  return true;
}

uint32_t Pet::aventuraMinutos(uint32_t nowEpoch) const {
  if (!avDesde || nowEpoch <= avDesde) return 0;
  uint32_t m = (nowEpoch - avDesde) / 60;
  return m > AV_TETO_MIN ? AV_TETO_MIN : m;
}

uint16_t Pet::aventuraRolos(uint32_t nowEpoch) const {
  return (uint16_t)(aventuraMinutos(nowEpoch) / AV_ROLO_MIN);
}

// Puro: lo que sale en la tirada i depende solo de la semilla. Por eso la cena
// puede ensenar el objeto antes de que se cobre y sea el mismo.
uint8_t Pet::aventuraItem(uint16_t i) const {
  const uint32_t r = avMezcla(avSeed, i, 1) % 100;
  // 40% nada, 32 curas, 12 objetos para llevar, 10 bolas, 4 piedras de
  // evolucion, 2 piedras mega
  uint8_t bolsillo;
  bool mega = false;
  if (r < 40) return 0;
  else if (r < 72) bolsillo = BP_BATTLE;
  else if (r < 84) bolsillo = BP_HELD;
  else if (r < 94) bolsillo = BP_BALL;
  else if (r < 98) bolsillo = BP_EVO;
  else { bolsillo = BP_HELD; mega = true; }
  uint16_t n = 0;
  for (uint16_t k = 1; k < ITEM_COUNT; k++)
    if (ITEM_TBL[k].pocket == bolsillo && (ITEM_TBL[k].fx == IT_MEGA) == mega) n++;
  if (!n) return 0;
  uint16_t alvo = (uint16_t)(avMezcla(avSeed, i, 2) % n);
  for (uint16_t k = 1; k < ITEM_COUNT; k++)
    if (ITEM_TBL[k].pocket == bolsillo && (ITEM_TBL[k].fx == IT_MEGA) == mega && alvo-- == 0)
      return (uint8_t)k;
  return 0;
}

// El titular, todo el equipo, y hasta seis de la caja abierta sacados por la
// semilla. Es LA lista: la cena solo saca a estos y el resumo les paga.
uint8_t Pet::aventuraElenco(AvParticipa *out) const {
  uint8_t n = 0;
  out[n] = AvParticipa();
  out[n].dex = speciesId;
  out[n].shiny = shiny;
  n++;
  for (uint8_t i = 0; i < TEAM_SLOTS; i++) {
    if (i == mainSlot || !team[i].used()) continue;
    out[n] = AvParticipa();
    out[n].dex = team[i].species;
    out[n].shiny = (team[i].flags & SM_SHINY) != 0;
    out[n].origen = 1;
    out[n].hueco = i;
    n++;
  }
  uint16_t total = 0;
  for (uint8_t i = 0; i < BANK_SLOTS; i++) if (bank[i].used()) total++;
  uint16_t ya[AV_DE_LA_CAJA];
  uint8_t q = 0;
  for (uint8_t t = 0; t < 12 && q < AV_DE_LA_CAJA && q < total; t++) {
    uint16_t alvo = (uint16_t)(avMezcla(avSeed, t, 3) % total);
    bool rep = false;
    for (uint8_t k = 0; k < q; k++) if (ya[k] == alvo) rep = true;
    if (rep) continue;
    ya[q++] = alvo;
    uint16_t c = 0;
    for (uint8_t i = 0; i < BANK_SLOTS; i++) {
      if (!bank[i].used()) continue;
      if (c++ != alvo) continue;
      out[n] = AvParticipa();
      out[n].dex = bank[i].species;
      out[n].shiny = (bank[i].flags & SM_SHINY) != 0;
      out[n].origen = 2;
      out[n].hueco = i;
      n++;
      break;
    }
  }
  return n;
}

// La EXP de un trozo: una fraccion de lo que le falta a ESE nivel para subir,
// asi un nivel 5 y un nivel 60 tardan lo mismo. `pct` el reparto: titular
// entero, equipo la mitad, caja un cuarto. Nunca pasa del techo de la region.
static uint32_t avExpDe(uint32_t expAhora, uint16_t nivel, uint16_t tope, uint32_t minEf, uint8_t pct) {
  uint32_t paso = Pet::expForLevel((uint16_t)(nivel + 1)) - Pet::expForLevel(nivel);
  // un cuarto de nivel por hora efectiva, el titular
  uint64_t e = (uint64_t)paso * minEf * pct / (240UL * 100UL);
  uint32_t techo = Pet::expForLevel(tope);
  if (expAhora >= techo) return 0;
  if (!e && minEf) e = 1;      // un minuto a nivel bajo redondea a cero: poco, pero algo
  if (e > techo - expAhora) e = techo - expAhora;
  return (uint32_t)e;
}

void Pet::aventuraFim(uint32_t nowEpoch, AvResumo &r) {
  r = AvResumo();
  if (!avDesde) return;
  r.minutos = aventuraMinutos(nowEpoch);
  if (r.minutos >= AV_MIN_MIN) {
    uint32_t ef = r.minutos <= AV_ENTERO_MIN ? r.minutos
                                             : AV_ENTERO_MIN + (r.minutos - AV_ENTERO_MIN) / 2;
    const uint16_t tope = levelCap();
    r.nMons = aventuraElenco(r.mons);
    for (uint8_t i = 0; i < r.nMons; i++) {
      AvParticipa &p = r.mons[i];
      if (p.origen == 0) {
        uint32_t e = avExpDe(exp, level(), tope, ef, 100);
        p.exp = e;
        p.subiu = gainExp(e);
      } else {
        StoredMon &m = p.origen == 1 ? team[p.hueco] : bank[p.hueco];
        uint16_t antes = levelOf(m);
        uint32_t e = avExpDe(m.exp, antes, tope, ef, p.origen == 1 ? 50 : 25);
        m.exp += e;
        p.exp = e;
        p.subiu = (uint8_t)(levelOf(m) - antes);
      }
    }
    uint16_t rolos = aventuraRolos(nowEpoch);
    for (uint16_t i = 0; i < rolos && r.nItens < AV_MAX_ITENS; i++) {
      uint8_t id = aventuraItem(i);
      if (!id || bag[id] >= 99) continue;
      bag[id]++;
      r.itens[r.nItens++] = id;
    }
  }
  // Antes del save y en el mismo: con la luz cortada a mitad no se cobra dos veces.
  avDesde = 0;
  save();
}

void Pet::tick() {
  if (ceremony != CER_NONE) return;  // el tiempo se detiene en la despedida
  // De aventura el tiempo tampoco corre para el: nada de hambre, descuidos ni
  // escapadas por dejarlo jugando, que es justo para lo que esta esto.
  if (avDesde) return;
  if (starterPick) return;  // la partida no empieza hasta elegir inicial: si el
                            // tiempo corriera aqui, el huevo eclosionaria solo a
                            // los 3 min con la especie sorteada y se perderia la
                            // eleccion del jugador
  ageMinutes++;

  if (isEgg()) {
    if (ageMinutes >= 3) hatch();  // si no lo tocas, eclosiona solo a los 3 min
    return;
  }

  // el sueño es descanso: la energia se recupera y las necesidades bajan MUCHO
  // mas lento que despierto y con suelo (amanece pidiendo algo de mimo, no a
  // cero, sin descuidos ni escapadas). despierto: comida -2/min, hig/joy -1/min.
  // El peso aun se quema; la racha de buen cuidado (goodTicks) queda en pausa.
  if (sleeping) {
    energy = clamp100(energy + 6);
    if (weight > 0 && ageMinutes % 3 == 0) weight--;
    if (ageMinutes % 2 == 0) {                 // ~4x mas lento que despierto
      fullness = dropTo(fullness, 1, 30);
      joy = dropTo(joy, 1, 35);
    }
    if (ageMinutes % 3 == 0) hygiene = dropTo(hygiene, 1, 45);
    // Dormir recupera PP, y POCO A POCO: una siesta da un repaso, una noche
    // entera lo deja todo lleno. Si fuera de golpe, apagar y encender la luz
    // seria un boton de recarga infinita y el PP no limitaria nada.
    ppRestore(1);
    hpHeal((uint16_t)(maxHp() * HP_HEAL_SLEEP_PCT / 100 + 1));   // ~1h40 lleno
    checkMedals();  // aun puede cruzar un nivel por edad mientras duerme
    if (++ticksSinceSave >= 5) pendingSave = true;
    return;
  }

  // (aqui iba un sonido de subir de nivel cada hora: el nivel ya no sale de
  // la edad, asi que lo toca gainExp cuando de verdad se sube)

  fullness = clamp100(fullness - 2);
  energy = clamp100(energy - 1);
  if (fullness > 40 && poops < 3 && random(100) < 15) poops++;

  hygiene = clamp100(hygiene - 1 - 4 * poops);
  // el sobrepeso da pereza: la energia cae el doble
  if (weight > 50) energy = clamp100(energy - 1);
  if (weight > 0 && ageMinutes % 3 == 0) weight--;

  // la disciplina forja la defensa: 12 h seguidas bien cuidado = +1 DEF
  if (lowestStat() >= 40) {
    if (++goodTicks >= 720) {
      goodTicks = 0;
      if (trDef < 100) trDef++;
    }
  } else {
    goodTicks = 0;
  }

  int dJoy = -1;
  if (fullness < 30) dJoy -= 2;
  if (hygiene < 30) dJoy -= 2;
  joy = clamp100(joy + dJoy);

  // Hilo lento de PS despierto. Existe para que NUNCA se pueda quedar atascado
  // sin poder pelear: aunque no se le haga nada, acaba volviendo. Pero es tan
  // lento que sigue compensando dormir o darle una baya.
  if (ageMinutes % HP_HEAL_AWAKE_MIN == 0) hpHeal(1);

  // Descuido: dejar una estadistica por los suelos cuenta como error de
  // cuidado (con enfriamiento para no contar el mismo descuido cada minuto)
  if (mistakeCooldown > 0) mistakeCooldown--;
  if (lowestStat() <= 10 && mistakeCooldown == 0) {
    careMistakes++;
    mistakeCooldown = 60;
    if (bond > 1) bond--;  // el descuido enfria el vinculo, pero sin arrasarlo:
                           // a -3 cada 30 min se perdia mucho mas de lo que se
                           // podia ganar en todo un dia y el vinculo se atascaba
  }

  // Un salvaje de vez en cuando. Va en tick() y no en la puesta al dia de
  // lo que estuvo apagado: un aviso que "paso" hace seis horas no sirve de
  // nada, y encontrarse el "!" puesto al encender seria mentira.
  rollWild((uint8_t)(lastSeenEpoch ? (lastSeenEpoch / 3600) % 24 : 13));
  rollPickup();

  checkMedals();  // la evolucion la dispara el usuario (canEvolveNow + tap), no el tick

  // abandono total: con TODO a cero durante una hora queda lista para escaparse;
  // NO se va sola, la dispara el usuario con el boton (final triste, lo presencia)
  if (fullness == 0 && joy == 0 && energy == 0 && hygiene == 0) {
    if (neglectTicks < RUNAWAY_TICKS) neglectTicks++;
    // Llama al entrar en el abandono y cada 15 minutos, tambien con el
    // boton ya fuera: que nadie se entere de la escapada por sorpresa.
    if (++avisoCuenta > ABANDONO_AVISO_TICKS) avisoCuenta = 1;
    if (avisoCuenta == 1 && !sleeping && !isEgg()) avisoAbandono = true;
  } else {
    neglectTicks = 0;  // un solo cuidado la salva
    avisoCuenta = 0;
    avisoAbandono = false;
  }

  // ciclo completo (forma final + 7 dias): la despedida NO salta sola; queda
  // lista (canFarewellNow) y la dispara el usuario con el boton, para que la vea

  // autoguardado periodico: NO escribir a flash aqui (corre dentro del loop,
  // mientras se anima); solo marcar y dejar que el loop lo vuelque al atenuar
  if (++ticksSinceSave >= 5) pendingSave = true;
}

// vuelca el guardado periodico pendiente (lo llama el loop en un momento sin
// animacion para que el paron de la escritura a flash no se vea)
void Pet::flushSave() {
  if (pendingSave) save();
}

// quedan miembros sin registrar en la linea evolutiva de esta base?
bool Pet::lineHasUnregistered(int16_t base) const {
  int16_t cur = base;
  for (int guard = 0; cur >= 1 && cur <= DEX_COUNT && guard < 6; guard++) {
    if (!isRegistered(cur)) return true;
    if (cur == DEX_EEVEE) {
      for (uint8_t i = 0; i < EEVEE_BRANCHES; i++)
        if (!isRegistered(EEVEE_BRANCH[i])) return true;
      return false;
    }
    cur = DEX_TBL[cur].evolvesTo;
  }
  return false;
}

uint8_t Pet::eggRarity() const {
  return (eggTarget >= 1 && eggTarget <= DEX_COUNT) ? DEX_TBL[eggTarget].rarity : R_COMUN;
}

// elige la especie del huevo: tirada de rareza (mejorada por una despedida
// completa, castigada por una escapada) y sesgo hacia lineas incompletas
int16_t Pet::pickEggSpecies() {
  // primera partida: inicial clasico
  if (registeredCount() == 0) {
    return CLASSIC_DEX[random(NUM_CLASSIC_DEX)];
  }

  uint8_t tier = R_COMUN;
  if (lastEnd != CER_RUNAWAY) {
    bool blessed = (lastEnd == CER_FAREWELL);
    int rare = (blessed ? 45 : 27) + careBonus();
    int leg = (registeredCount() >= 25) ? (blessed ? 10 : 3) + careBonus() / 3 : 0;
    int r = random(100);
    if (r < leg) tier = R_LEGENDARIO;
    else if (r < leg + rare) tier = R_RARO;
  }

  // candidatos del tier con linea incompleta; si no hay, baja de tier;
  // si la pokedex del tier esta completa, vale cualquiera del tier
  for (int pass = 0; pass < 2; pass++) {
    for (int t = tier; t >= R_COMUN; t--) {
      // Se sortea SOBRE LA MARCHA (muestreo de reservorio) en vez de juntar
      // una lista: era un `int16_t cand[80]` que con 77 comunes en Kanto ya
      // iba justo, y al entrar Johto habria recortado en silencio todo lo
      // que viniera despues del candidato 80. El reservorio no tiene tope y
      // no gasta pila.
      int16_t elegido = 0;
      int n = 0;
      for (int16_t d = 1; d <= DEX_COUNT; d++) {
        if (DEX_TBL[d].rarity != t) continue;
        // Hasta donde HAS LLEGADO, igual que la hierba (wildWeight). Si el
        // huevo no mirara la region, daria igual que la hierba la mirase:
        // seguirias criando Mudkips en Pueblo Paleta y viajar no
        // significaria nada. Hacia atras si valen todas.
        if (dexRegionOf(d) > region) continue;
        if (pass == 0 && !lineHasUnregistered(d)) continue;
        if (random(++n) == 0) elegido = d;   // 1/n: sale uniforme
      }
      if (elegido) return elegido;
    }
  }
  return CLASSIC_DEX[random(NUM_CLASSIC_DEX)];  // inalcanzable, por si acaso
}

// ---------------------------------------------------------------------- PP
// Los PP viven aqui y no en el combate porque atraviesan los combates: eso es
// lo unico que hace que el numero signifique algo. Se recuperan durmiendo y
// con bayas, asi que el "centro Pokemon" de este juego es cuidar al bicho.

// La habilidad del individuo. Entre las dos normales a medias, y una de cada
// 32 saca la OCULTA: es la version "rara" de un bicho que no es shiny, y le
// da a cada crianza algo mas que mirar.
// ------------------------------------------------------- equipo y banco
uint16_t Pet::levelOf(const StoredMon &m) {
  if (!m.used()) return 1;
  uint16_t lv = 1;
  while (lv < LEVEL_CAP && expForLevel(lv + 1) <= m.exp) lv++;
  return lv;
}

uint8_t Pet::teamCount() const {
  uint8_t n = 0;
  for (uint8_t i = 0; i < TEAM_SLOTS; i++) if (team[i].used()) n++;
  return n;
}

uint8_t Pet::bankCount() const {
  uint8_t n = 0;
  for (uint8_t i = 0; i < BANK_SLOTS; i++) if (bank[i].used()) n++;
  return n;
}

void Pet::storeActive() {
  if (mainSlot >= TEAM_SLOTS) return;
  if (isEgg()) { team[mainSlot].species = 0; return; }
  hpSync();
  ppSync();
  team[mainSlot] = fotoTitular();
}

// O registro do titular SEM mexer em nada: a batalha entre placas manda
// ele pelo ar e o save nao pode mudar (nem o hpSync/ppSync rodam).
// Parte do hueco, para o que o Pet vivo nao guarda (o resto do registro).
StoredMon Pet::fotoTitular() const {
  StoredMon m = mainSlot < TEAM_SLOTS ? team[mainSlot] : StoredMon();
  if (isEgg()) { m.species = 0; return m; }
  m.species = speciesId;
  m.exp = exp;
  m.hpLeft = hpLeft;
  for (uint8_t i = 0; i < 4; i++) { m.moves[i] = ppMove[i]; m.ppLeft[i] = ppLeft[i]; }
  m.geneHp = geneHp; m.geneAtk = geneAtk; m.geneDef = geneDef;
  m.geneSpa = geneSpa; m.geneSpd = geneSpd; m.geneSpe = geneSpe;
  m.trHp = trHp; m.trAtk = trAtk; m.trDef = trDef;
  m.trSpa = trSpa; m.trSpd = trSpd; m.trSpe = trSpe;
  m.ability = ability;
  m.item = heldItem;
  m.bond = bond;
  m.form = form;
  m.flags = (uint8_t)((shiny ? SM_SHINY : 0) |
                      (movesPicked ? SM_MOVES_PICKED : 0) |
                      (female ? SM_FEMALE : 0) |
                      (genderSet ? SM_GENDER_SET : 0) |
                      (ballId ? SM_BALL_SET : 0));
  memcpy(m.nick, nick, sizeof(m.nick));
  m.ball = ballId;
  return m;
}

bool Pet::setMain(uint8_t slot) {
  if (slot >= TEAM_SLOTS || !team[slot].used() || slot == mainSlot) return false;
  if (ceremony != CER_NONE) return false;
  storeActive();                      // el que sale se guarda tal cual estaba
  if (!cargaDelHueco(slot)) return false;
  save();
  return true;
}

// O miolo: despeja um StoredMon no Pet vivo. Viajar de regiao faz a
// MESMA coisa que trocar de titular —— o bicho que voce cuida passa a
// ser outro ——, e eram 25 linhas pra duplicar.
//
// Nao tem a guarda `slot == mainSlot` que o setMain tem: ao chegar numa
// regiao o mainSlot ja foi trocado pelo de la, e mesmo assim e preciso
// carregar.
// A chave de uma regiao: "tm" + o numero. O primeiro byte do bloco
// e o CARIMBO —— a regiao a que o time pertence. Guardar o dono
// dentro do dado e o que permite recusar um bloco que foi parar na
// chave errada, em vez de o carregar como se fosse dali.
static void claveTime(uint8_t r, char *out, size_t n) {
  snprintf(out, n, "tm%u", (unsigned)r);
}

// ------------------------------------------------ as regioes NO SD
//
// 2026-11-08: a NVS encheu em Hoenn e parou de gravar sem dizer nada; o
// reinicio levou a regiao inteira. Agora o time de cada regiao vive num
// arquivo do SD, e um arquivo de ESTADO guarda onde se esta, ate onde se
// chegou, as insignias e a SEQUENCIA do save. A NVS guarda a mesma
// sequencia ("svn"): se no boot o SD for mais novo, a NVS falhou e o SD
// manda.
//
// Cada arquivo leva cabecalho (assinatura, tamanho, CRC) e grava-se em tres
// passos: escreve X.new, rele e confere, so entao troca X.bin. Um corte de
// luz no meio deixa um dos dois inteiro, e o leitor tenta os dois.
#define SV_MAGIC 0x56535054u   // "TPSV"
#define SV_VERSION 1
struct SvHeader {
  uint32_t magic;
  uint8_t version;
  uint8_t monSize;    // um StoredMon que cresca invalida o arquivo em vez de
  uint16_t tam;       // ser lido desalinhado
  uint32_t crc;
};

static uint32_t svCrc(const uint8_t *p, size_t n) {
  uint32_t c = 0xFFFFFFFFu;
  for (size_t i = 0; i < n; i++) {
    c ^= p[i];
    for (uint8_t k = 0; k < 8; k++) c = (c >> 1) ^ (0xEDB88320u & (0u - (c & 1u)));
  }
  return ~c;
}

static bool svLeUm(const char *path, uint8_t *dados, size_t n) {
  File f = SD_MMC.open(path, FILE_READ);
  if (!f) return false;
  SvHeader h;
  bool ok = f.read((uint8_t *)&h, sizeof(h)) == (int)sizeof(h) && h.magic == SV_MAGIC &&
            h.version == SV_VERSION && h.monSize == (uint8_t)sizeof(StoredMon) && h.tam == n &&
            f.read(dados, n) == (int)n && svCrc(dados, n) == h.crc;
  f.close();
  return ok;
}

// Le `base`.bin; se estiver ruim ou faltar, o `base`.new (corte no meio da troca).
static bool svLe(const char *base, void *dados, size_t n) {
  char p[40];
  snprintf(p, sizeof(p), "%s.bin", base);
  if (svLeUm(p, (uint8_t *)dados, n)) return true;
  snprintf(p, sizeof(p), "%s.new", base);
  return svLeUm(p, (uint8_t *)dados, n);
}

static bool svGrava(const char *base, const void *dados, size_t n) {
  SD_MMC.mkdir("/save");
  char nov[40], fin[40];
  snprintf(nov, sizeof(nov), "%s.new", base);
  snprintf(fin, sizeof(fin), "%s.bin", base);
  SvHeader h;
  h.magic = SV_MAGIC;
  h.version = SV_VERSION;
  h.monSize = (uint8_t)sizeof(StoredMon);
  h.tam = (uint16_t)n;
  h.crc = svCrc((const uint8_t *)dados, n);
  // FILE_WRITE ANADE se o arquivo existe (a mesma trampa das caixas).
  if (SD_MMC.exists(nov)) SD_MMC.remove(nov);
  File f = SD_MMC.open(nov, FILE_WRITE);
  if (!f) return false;
  bool ok = f.write((const uint8_t *)&h, sizeof(h)) == sizeof(h) &&
            f.write((const uint8_t *)dados, n) == n;
  f.close();
  if (!ok) return false;
  // Rele ANTES de tocar no bom: uma tarjeta pode aceitar e nao gravar.
  static uint8_t volta[2 + sizeof(StoredMon) * TEAM_SLOTS + 64];
  if (n > sizeof(volta) || !svLeUm(nov, volta, n) || memcmp(volta, dados, n) != 0) return false;
  if (SD_MMC.exists(fin)) SD_MMC.remove(fin);
  return SD_MMC.rename(nov, fin);
}

static void svBaseRegiao(uint8_t r, char *out, size_t n) {
  snprintf(out, n, "/save/reg%02u", (unsigned)r);
}

// O que o arquivo de estado guarda. So tipos de tamanho fixo.
struct SvEstado {
  uint32_t seq;
  uint8_t region, maxRegion, teamRegion, regioes;
  uint16_t visitedMask;
  uint8_t badgesIn[REGION_COUNT];
  uint8_t mainSlotIn[REGION_COUNT];
};

void Pet::guardaTimeDe(uint8_t r) {
  if (r >= REGION_COUNT) return;
  // static, NAO na pilha: o load() ja carrega um array de 3 KB do
  // banco antigo, e cada buffer a mais na pilha do arranque e o que
  // faz a placa travar e reiniciar em loop.
  static uint8_t buf[2 + sizeof(StoredMon) * TEAM_SLOTS];
  buf[0] = r;                       // o carimbo
  buf[1] = mainSlotIn[r];
  memcpy(buf + 2, teamsIn[r], sizeof(StoredMon) * TEAM_SLOTS);
  char k[8];
  claveTime(r, k, sizeof(k));
  char base[24];
  svBaseRegiao(r, base, sizeof(base));
  if (svGrava(base, buf, sizeof(buf))) {
    // No SD e conferido: a copia da NVS sai, que ela ficaria velha e a NVS
    // e justamente o que nao pode encher.
    if (prefs.isKey(k)) prefs.remove(k);
    return;
  }
  // Sem SD (ou com ele recusando): a NVS, como antes.
  guarda(k, buf, sizeof(buf));
}

bool Pet::leTimeDe(uint8_t r) {
  if (r >= REGION_COUNT) return false;
  // static, NAO na pilha: o load() ja carrega um array de 3 KB do
  // banco antigo, e cada buffer a mais na pilha do arranque e o que
  // faz a placa travar e reiniciar em loop.
  static uint8_t buf[2 + sizeof(StoredMon) * TEAM_SLOTS];
  char k[8];
  claveTime(r, k, sizeof(k));
  char base[24];
  svBaseRegiao(r, base, sizeof(base));
  // Quem manda: a chave da NVS, se existir (so existe quando o SD falhou ao
  // gravar, ou antes da mudanca); o SD, se o boot viu que a NVS ficou para
  // tras (sdManda).
  bool lido = false;
  if (sdManda) lido = svLe(base, buf, sizeof(buf));
  if (!lido) lido = prefs.getBytes(k, buf, sizeof(buf)) == sizeof(buf);
  if (!lido && !sdManda) lido = svLe(base, buf, sizeof(buf));
  if (!lido) return false;
  // O CARIMBO. Se o bloco diz que e de outra regiao, nao se carrega:
  // melhor uma regiao vazia, que se ve, do que o time de outra
  // ocupando o lugar, que nao se ve.
  if (buf[0] != r) {
    Serial.printf("%s traz o carimbo da regiao %u: RECUSADO\n",
                  k, (unsigned)buf[0]);
    return false;
  }
  mainSlotIn[r] = buf[1] < TEAM_SLOTS ? buf[1] : 0;
  memcpy(teamsIn[r], buf + 2, sizeof(StoredMon) * TEAM_SLOTS);
  return true;
}

void Pet::diagSave() {
  static const char *CLAVES[] = { "team", "teams", "mslin",
                                  "bank", "teamsbk", "teambk2" };
  Serial.printf("mainSlot=%u region=%u maxRegion=%u vmask=0x%03X\n",
                mainSlot, region, maxRegion, (unsigned)visitedMask);
  Serial.printf("seq=%lu svn(NVS)=%lu sdManda=%u sdGravouEstado=%u\n",
                (unsigned long)seqSave, (unsigned long)prefs.getUInt("svn", 0),
                sdManda ? 1u : 0u, sdGravouEstado ? 1u : 0u);
  Serial.printf("guardadoFallo=%u bloquesFallados=%u teamsmig=%u\n",
                guardadoFallo ? 1u : 0u, bloquesFallados,
                prefs.getBool("teamsmig", false) ? 1u : 0u);
  Serial.printf("esperado: team=%u teams=%u bank=%u\n",
                (unsigned)sizeof(team), (unsigned)sizeof(teamsIn),
                (unsigned)sizeof(bank));
  for (uint8_t i = 0; i < 6; i++)
    Serial.printf("  %-8s %u bytes\n", CLAVES[i],
                  (unsigned)prefs.getBytesLength(CLAVES[i]));
}

void Pet::diagBackup() {
  static StoredMon bk[REGION_COUNT][TEAM_SLOTS];
  if (prefs.getBytes("teamsbk", bk, sizeof(bk)) != sizeof(bk)) {
    Serial.println("teamsbk: nao existe");
    return;
  }
  for (uint8_t r = 0; r < REGION_COUNT; r++) {
    uint8_t n = 0;
    for (uint8_t i = 0; i < TEAM_SLOTS; i++) if (bk[r][i].used()) n++;
    if (!n) continue;
    Serial.printf("  %s: %u\n", regionAt(r).name, n);
    for (uint8_t i = 0; i < TEAM_SLOTS; i++) {
      const StoredMon &m = bk[r][i];
      if (!m.used()) continue;
      Serial.printf("     %u: #%d %s nv%u\n", i, m.species,
                    (m.species >= 1 && m.species <= DEX_COUNT)
                        ? DEX_TBL[m.species].name : "?",
                    Pet::levelOf(m));
    }
  }
}

bool Pet::equipoACopia(const char *clave) {
  return guarda(clave, team, sizeof(team)) == sizeof(team);
}

bool Pet::equipoDeCopia(const char *clave) {
  StoredMon prev[TEAM_SLOTS];
  if (prefs.getBytes(clave, prev, sizeof(prev)) != sizeof(prev))
    return false;
  for (uint8_t i = 0; i < TEAM_SLOTS; i++) team[i] = prev[i];
  mainSlot = 0;
  for (uint8_t i = 0; i < TEAM_SLOTS; i++)
    if (team[i].used()) { mainSlot = i; break; }
  cargaDelHueco(mainSlot);
  save();
  return true;
}

bool Pet::cargaDelHueco(uint8_t slot) {
  if (slot >= TEAM_SLOTS || !team[slot].used()) return false;
  const StoredMon &m = team[slot];
  prevSpeciesId = speciesId;
  speciesId = m.species;
  exp = m.exp;
  hpLeft = m.hpLeft;
  hpSeen = 0;                         // se recalcula para la especie nueva
  for (uint8_t i = 0; i < 4; i++) { ppMove[i] = m.moves[i]; ppLeft[i] = m.ppLeft[i]; }
  geneHp = m.geneHp; geneAtk = m.geneAtk; geneDef = m.geneDef;
  geneSpa = m.geneSpa; geneSpd = m.geneSpd; geneSpe = m.geneSpe;
  trHp = m.trHp; trAtk = m.trAtk; trDef = m.trDef;
  trSpa = m.trSpa; trSpd = m.trSpd; trSpe = m.trSpe;
  ability = m.ability;
  heldItem = m.item;
  bond = m.bond;                      // el vinculo es SUYO: por eso cambiar cuesta
  shiny = (m.flags & SM_SHINY) != 0;
  movesPicked = (m.flags & SM_MOVES_PICKED) != 0;
  female = (m.flags & SM_FEMALE) != 0;
  genderSet = (m.flags & SM_GENDER_SET) != 0;
  form = m.form;
  if (!genderSet) rollGender();   // guardado de antes del sexo
  memcpy(nick, m.nick, sizeof(nick));
  nick[sizeof(nick) - 1] = 0;
  ballId = ballDe(m);
  mainSlot = slot;
  hpSync();
  ppSync();
  // los tres salen del StoredMon que se acaba de copiar encima
  registerSpecies(speciesId, shiny, form);
  return true;
}

bool Pet::addToTeam(const StoredMon &m) {
  if (!m.used()) return false;
  for (uint8_t i = 0; i < TEAM_SLOTS; i++) {
    if (team[i].used()) continue;
    team[i] = m;
    save();
    return true;
  }
  return false;
}

// ---------------------------------------------------------------- cajas
//
// El nombre del fichero de una caja. Va aparte para que el path se
// escriba UNA vez: un typo aqui no rompe el build, pierde la caja.
static void boxPath(uint8_t n, char *dst, size_t cap) {
  snprintf(dst, cap, "/boxes/box%02u.bin", (unsigned)n);
}

bool Pet::boxWrite() const {
  if (boxOpen >= BOX_COUNT) return false;
  SD_MMC.mkdir("/boxes");
  char path[32];
  boxPath(boxOpen, path, sizeof(path));
  // FILE_WRITE ANADE al final si el fichero existe (la misma trampa que
  // ya mordio al instalador de sprites), asi que se borra antes.
  if (SD_MMC.exists(path)) SD_MMC.remove(path);
  File f = SD_MMC.open(path, FILE_WRITE);
  if (!f) return false;
  BoxHeader h;
  h.magic = BOX_MAGIC;
  h.version = BOX_VERSION;
  h.monSize = (uint8_t)sizeof(StoredMon);
  h.slots = BANK_SLOTS;
  h.box = boxOpen;
  h.reservado[0] = h.reservado[1] = 0;
  bool ok = f.write((const uint8_t *)&h, sizeof(h)) == sizeof(h) &&
            f.write((const uint8_t *)bank, sizeof(bank)) == sizeof(bank);
  f.close();
  return ok;
}

bool Pet::boxRead(uint8_t n, StoredMon *dst) const {
  if (n >= BOX_COUNT || !dst) return false;
  char path[32];
  boxPath(n, path, sizeof(path));
  File f = SD_MMC.open(path, FILE_READ);
  if (!f) return false;
  BoxHeader h;
  bool ok = f.read((uint8_t *)&h, sizeof(h)) == (int)sizeof(h);
  // LA FIRMA. Sin esto, un StoredMon que crezca (ya paso: 48 -> 52) haria
  // que todas las cajas viejas se leyeran desplazadas —— cada bicho con
  // los bytes del siguiente —— y nada se quejaria.
  if (ok)
    ok = h.magic == BOX_MAGIC && h.version == BOX_VERSION &&
         h.monSize == (uint8_t)sizeof(StoredMon) && h.slots == BANK_SLOTS &&
         h.box == n;
  if (ok)
    ok = f.read((uint8_t *)dst, sizeof(StoredMon) * BANK_SLOTS) ==
         (int)(sizeof(StoredMon) * BANK_SLOTS);
  f.close();
  return ok;
}

uint8_t Pet::boxCount(uint8_t n) const {
  if (n == boxOpen) {
    uint8_t c = 0;
    for (uint8_t i = 0; i < BANK_SLOTS; i++) if (bank[i].used()) c++;
    return c;
  }
  StoredMon tmp[BANK_SLOTS];
  if (!boxRead(n, tmp)) return 0;
  uint8_t c = 0;
  for (uint8_t i = 0; i < BANK_SLOTS; i++) if (tmp[i].used()) c++;
  return c;
}

bool Pet::boxGo(uint8_t n) {
  if (n >= BOX_COUNT) return false;
  if (n == boxOpen) return true;
  // La abierta se guarda ANTES de soltarla. Si la escritura falla no se
  // cambia de caja: perder una caja por cambiar de pagina seria el peor
  // fallo posible de esta pantalla.
  if (!boxWrite()) return false;
  StoredMon tmp[BANK_SLOTS];
  if (boxRead(n, tmp)) {
    for (uint8_t i = 0; i < BANK_SLOTS; i++) bank[i] = tmp[i];
  } else {
    // Caja que aun no existe (o ilegible): se abre vacia. No se borra el
    // fichero: si estaba corrupto, que se quede para poder mirarlo.
    for (uint8_t i = 0; i < BANK_SLOTS; i++) bank[i].species = 0;
  }
  boxOpen = n;
  save();
  return true;
}

bool Pet::addToBank(const StoredMon &m) {
  if (!m.used()) return false;
  for (uint8_t i = 0; i < BANK_SLOTS; i++) {
    if (bank[i].used()) continue;
    bank[i] = m;
    save();
    return true;
  }
  // La caja abierta esta llena: se busca por las demas y se ABRE la que
  // tenga sitio, como en los juegos ("Box 2 was opened"). Antes de esto,
  // con el banco lleno la captura simplemente se perdia.
  //
  // Y se apunta POR QUE no se pudo: si boxGo falla, es que la tarjeta no
  // dejo escribir, no que falte sitio. Decirle al jugador "no room" con
  // 930 huecos libres le manda a buscar algo que ya tiene.
  cajasSinTarjeta = false;
  bool algunaSeNego = false;
  for (uint8_t c = 0; c < BOX_COUNT; c++) {
    if (c == boxOpen) continue;
    if (boxCount(c) >= BANK_SLOTS) continue;
    if (!boxGo(c)) { algunaSeNego = true; continue; }
    for (uint8_t i = 0; i < BANK_SLOTS; i++) {
      if (bank[i].used()) continue;
      bank[i] = m;
      save();
      return true;
    }
  }
  cajasSinTarjeta = algunaSeNego;
  return false;
}

bool Pet::pruebaDeEscritura() const {
  SD_MMC.mkdir("/boxes");
  const char *p = "/boxes/prueba.tmp";
  if (SD_MMC.exists(p)) SD_MMC.remove(p);
  File f = SD_MMC.open(p, FILE_WRITE);
  if (!f) return false;
  const uint8_t dato[4] = { 'T', 'P', 'B', 'X' };
  bool ok = f.write(dato, sizeof(dato)) == sizeof(dato);
  f.close();
  // Y releerlo: una tarjeta puede aceptar el open y tragarse la escritura.
  if (ok) {
    File g = SD_MMC.open(p, FILE_READ);
    uint8_t leido[4] = { 0, 0, 0, 0 };
    ok = g && g.read(leido, sizeof(leido)) == (int)sizeof(leido) &&
         memcmp(leido, dato, sizeof(dato)) == 0;
    if (g) g.close();
  }
  SD_MMC.remove(p);
  return ok;
}

void Pet::finishIntro(bool girl) {
  introDone = true;
  playerGirl = girl;
  // El regalo del profesor. Sin el, la primera partida arranca seca: los
  // primeros salvajes pasan por delante y no hay con que cogerlos.
  for (uint8_t i = 0; i < OAK_BALLS; i++) giveBall(0);
  lastBallCount = OAK_BALLS;
  save();
}

uint8_t Pet::giveBall(uint8_t tier) {
  // El ultimo escalon es la Master Ball y no se regala: si cayera de un
  // premio diario, capturar dejaria de ser una decision.
  uint8_t tope = BALL_TIERS > 1 ? (uint8_t)(BALL_TIERS - 2) : 0;
  if (tier > tope) tier = tope;
  uint8_t id = BALL_ID[tier];
  if (bag[id] < 99) bag[id]++;
  lastBallId = id;
  return id;
}

uint8_t Pet::giveBallX() {
  if (BALL_ALL_COUNT <= BALL_TIERS) return 0;
  uint8_t id = BALL_ALL[BALL_TIERS + random(BALL_ALL_COUNT - BALL_TIERS)];
  if (bag[id] < 99) bag[id]++;
  lastBallId = id;
  return id;
}

// RECOGIDA (Pickup). La del combate (quedarse lo que el rival gasto)
// aqui no se nota; la que todo el mundo recuerda es la OTRA: el bicho
// aparece con algo en la boca mientras pasea. Por eso vive en el tick
// y no en battle.cpp.
//
// Decision del usuario: "de tempo em tempo coloque um item no
// inventario". El ritmo sale de PICKUP_PCT por minuto DESPIERTO —
// dormido no recoge nada, que es lo que hace que sea del paseo y no
// un grifo que corre solo mientras el aparato esta apagado. Con 1 de
// cada 240 sale algo cada cuatro horas de vigilia, que en una vida de
// tres dias son un punado y no una tienda.
//
// Lo que trae es lo barato de la bolsa: comida y curas. Ni piedras ni
// bolas — esas tienen sus propios grifos y vaciarian el sentido del
// gimnasio y del cuidado diario.
// Los premios del cuestionario (Caramelo Raro, Pocion Brillante) no caen
// por ahi: estan en el bolsillo de usables, que es de donde sortean la
// Recogida y el botin de ganar, y sin esto la Pocion saldria 1 de cada 60
// victorias. Se ganan acertando al Profesor.
static bool soDoQuiz(uint8_t id) {
  return ITEM_TBL[id].fx == IT_RARECANDY || ITEM_TBL[id].fx == IT_SHINY;
}

void Pet::rollPickup() {
  lastPickup = 0;
  if (isEgg() || sleeping || ceremony != CER_NONE) return;
  if (abilAt(ability).fx != AB_PICKUP) return;
  if (random(PICKUP_PCT) != 0) return;
  // Se sortea entre lo que puede caer, no entre la tabla entera: una
  // Master Ball de regalo rompe el juego, y una piedra evolutiva se
  // la gana el que va al gimnasio.
  uint8_t cand[24];
  uint8_t n = 0;
  for (uint8_t i = 1; i < ITEM_COUNT && n < 24; i++) {
    const ItemEntry &it = ITEM_TBL[i];
    if (it.pocket != BP_BATTLE) continue;        // pociones, curas, bayas
    if (it.fx == IT_VITAMIN) continue;           // los EVs se ganan peleando
    if (soDoQuiz(i)) continue;
    cand[n++] = i;
  }
  if (!n) return;
  uint8_t id = cand[random(n)];
  if (bag[id] < 99) {
    bag[id]++;
    lastPickup = id;
    pendingSave = true;
  }
}

// ---------------------------------------------------------------- MT

bool Pet::giveTM(uint16_t tm) {
  if (tm >= TM_COUNT || hasTM(tm)) return false;
  tms[tm >> 3] |= (uint8_t)(1 << (tm & 7));
  lastTM = (int16_t)tm;
  pendingSave = true;
  return true;
}

uint8_t Pet::tmCount() const {
  uint16_t n = 0;
  for (uint16_t i = 0; i < TM_COUNT; i++) if (hasTM(i)) n++;
  return (uint8_t)(n > 255 ? 255 : n);
}

bool Pet::canTeachTM(uint16_t tm) const {
  if (isEgg() || !hasTM(tm) || !tmFits(speciesId, tm)) return false;
  // Y que no se la sepa ya: ensenarle lo que ya tiene seria gastar un
  // hueco para nada y el jugador no entenderia por que no cambia nada.
  uint16_t mv = tmMove(tm);
  for (uint8_t i = 0; i < 4; i++) if (ppMove[i] == mv) return false;
  return true;
}

bool Pet::teachTM(uint16_t tm, uint8_t slot) {
  if (slot >= 4 || !canTeachTM(tm)) return false;
  uint16_t mv = tmMove(tm);
  ppMove[slot] = mv;
  ppLeft[slot] = movePP(mv);
  // A partir de aqui los golpes los eliges TU: si no, el siguiente
  // nivel pisaria lo que acabas de ensenar. Es la misma marca que pone
  // elegir un golpe a mano en la ficha.
  movesPicked = true;
  save();
  return true;
}

int16_t Pet::rollTM(uint8_t tipo) const {
  // Solo entre las que tu bicho PODRIA aprender y no tienes. Regalar
  // una que no le sirve a nadie es un premio que no se nota.
  //
  // Y, si quien la regala es un lider, DE SU TIPO. Antes no se miraba el
  // tipo en ningun sitio y Brock podia soltar Seduccion: ganar la
  // insignia de Roca tiene que abrir algo de Roca, o la insignia no
  // significa nada.
  //
  // Dos vueltas, no una: la primera exige el tipo y la segunda se
  // conforma con cualquiera. Un bicho de Agua que gane a Brock puede no
  // aprender ninguna MT de Roca, y quedarse sin premio seria peor.
  if (isEgg()) return -1;
  for (uint8_t vuelta = 0; vuelta < 2; vuelta++) {
    bool exigeTipo = (vuelta == 0 && tipo != T_NONE);
    uint16_t cand = 0;
    for (uint16_t i = 0; i < TM_COUNT; i++) {
      if (!tmFits(speciesId, i) || hasTM(i)) continue;
      if (exigeTipo && MOVE_TBL[tmMove(i)].type != tipo) continue;
      cand++;
    }
    if (!cand) continue;               // ninguna de su tipo: a la segunda
    uint16_t n = (uint16_t)random(cand);
    for (uint16_t i = 0; i < TM_COUNT; i++) {
      if (!tmFits(speciesId, i) || hasTM(i)) continue;
      if (exigeTipo && MOVE_TBL[tmMove(i)].type != tipo) continue;
      if (!n) return (int16_t)i;
      n--;
    }
  }
  return -1;
}

uint8_t Pet::dailyBalls() {
  // Una por dia, y la racha paga mejor. Es el grifo lento y seguro: aunque
  // no pelees ni entrenes, cuidando llegas a tener con que capturar.
  lastBallCount = 0;
  giveBall(0);
  lastBallCount++;
  if (streak && streak % BALL_STREAK_ULTRA == 0) { giveBall(2); lastBallCount++; }
  else if (streak && streak % BALL_STREAK_GREAT == 0) { giveBall(1); lastBallCount++; }
  // De vez em quando, uma das de condicao. E a que se anuncia (lastBallId).
  if ((uint8_t)random(100) < BALLX_DAILY_PCT) { giveBallX(); lastBallCount++; }
  return lastBallCount;
}

uint8_t Pet::trainBalls(uint16_t score) {
  // El grifo rapido: el gimnasio paga segun lo bien que lo hagas. Una
  // sesion floja no da nada, una redonda da dos y puede caer una Ultra.
  uint8_t n = (uint8_t)(score / TRAIN_BALL_SCORE);
  if (n > TRAIN_BALL_MAX) n = TRAIN_BALL_MAX;
  lastBallCount = 0;
  lastBallId = 0;
  for (uint8_t i = 0; i < n; i++) {
    uint8_t tier = 0;
    if (score >= TRAIN_BALL_ULTRA && random(100) < 20) tier = 2;
    else if (score >= TRAIN_BALL_GREAT && random(100) < 30) tier = 1;
    uint8_t id = giveBall(tier);
    // se anuncia la MEJOR de la tanda, que es la que el jugador quiere ver
    if (id > lastBallId) lastBallId = id;
    lastBallCount++;
  }
  // Um treino bom pode trazer uma das de condicao (os ids delas vem
  // depois de todas as outras, entao e ela a anunciada).
  if (n && score >= TRAIN_BALL_GREAT && (uint8_t)random(100) < BALLX_TRAIN_PCT) {
    giveBallX();
    lastBallCount++;
  }
  return lastBallCount;
}

uint8_t Pet::catchMon(int16_t dex, uint16_t lvl, bool isShiny, uint16_t abil,
                      uint8_t ball) {
  if (dex < 1 || dex > DEX_COUNT) return CATCH_FULL;
  StoredMon m;
  if (ball && ball < ITEM_COUNT) {
    m.ball = ball;
    m.flags |= SM_BALL_SET;
  }
  m.species = dex;
  m.exp = expForLevel(lvl ? lvl : 1);
  // Genes propios: un salvaje capturado no es un clon del anterior. Mismo
  // rango que un recien nacido normal, sin el suelo alto de la herencia:
  // eso se lo gana quien cria, no quien tira una bola.
  m.geneHp = (uint8_t)(LEGACY_FLOOR_BASE + random(21));
  m.geneAtk = (uint8_t)(LEGACY_FLOOR_BASE + random(21));
  m.geneDef = (uint8_t)(LEGACY_FLOOR_BASE + random(21));
  m.geneSpa = (uint8_t)(LEGACY_FLOOR_BASE + random(21));
  m.geneSpd = (uint8_t)(LEGACY_FLOOR_BASE + random(21));
  m.geneSpe = (uint8_t)(LEGACY_FLOOR_BASE + random(21));
  m.ability = abil;
  m.bond = BOND_AT_BIRTH;
  // A Friend Ball e isso: chega gostando de voce.
  if (ball < ITEM_COUNT && ITEM_TBL[ball].fx == IT_BALLX &&
      ITEM_TBL[ball].arg == BALLK_FRIEND)
    m.bond = BOND_FRIEND_BALL;
  // Lo que cazas es de DONDE lo cazas: en Alola, un Vulpix de Alola.
  m.form = formForRegion(dex, region, (uint8_t)random(100));
  // Y el sexo, con el reparto de su especie, igual que al eclosionar.
  m.flags |= SM_GENDER_SET;
  {
    uint8_t p = DEX_TBL[dex].femPct;
    if (p != GENDERLESS && p > 0 && (int)random(100) < p)
      m.flags |= SM_FEMALE;
  }
  if (isShiny) m.flags |= SM_SHINY;
  // Los cuatro ultimos golpes que sabe a su nivel, con los PP llenos: llega
  // listo para pelear, no en blanco.
  {
    uint16_t todos[40];
    uint8_t n = movesKnown(dex, lvl ? lvl : 1, todos, 40);
    uint8_t desde = n > 4 ? (uint8_t)(n - 4) : 0;
    for (uint8_t i = 0; i + desde < n && i < 4; i++) {
      m.moves[i] = todos[desde + i];
      m.ppLeft[i] = movePP(m.moves[i]);
    }
  }
  // ENTERO, con sus PS de verdad. Antes se dejaba en 0 con el sentido de
  // "sin estrenar, ya se le calculara al sacarlo", pero 0 es TAMBIEN como
  // se guarda un desmayado (btStoreActive vuelca c.hp, que es 0 al caer).
  // Los dos sentidos en el mismo numero, y el combate se cree el segundo:
  // btCanSend() exige hpLeft > 0, asi que un bicho recien capturado no
  // contaba como relevo. Con el equipo entero de capturados,
  // btReserveCount() daba 0, playerReserves quedaba en false y al caer el
  // tuyo knockOut() daba el combate por perdido en vez de dejarte sacar a
  // otro. Contra un salvaje, perder uno era perder la pelea.
  m.hpLeft = hpAtLevelForm(dex, Pet::levelOf(m), m.trHp, m.form);
  // del CAPTURADO (isShiny y m.form), no del que llevas tu
  registerSpecies(dex, isShiny, m.form);   // capturar llena la Pokedex
  if (caught < 0xFFFF) caught++;           // y la cuenta de la ficha
  uint8_t donde = addToTeam(m) ? CATCH_TEAM
                               : (addToBank(m) ? CATCH_BANK : CATCH_FULL);
  // Leva 72: a cacada acaba quando o cacado FICA com voce (time ou banco).
  // Com tudo cheio ele se perde, e entao a cacada continua.
  if (donde != CATCH_FULL && dex == cacaDex) cacaDex = 0;
  if (donde != CATCH_FULL) save();
  return donde;
}

bool Pet::toBank(uint8_t teamSlot) {
  // El titular no se guarda en el banco: primero hay que pasarle el relevo a
  // otro. Si no, el juego se quedaria sin bicho en pantalla.
  if (teamSlot >= TEAM_SLOTS || teamSlot == mainSlot) return false;
  if (!team[teamSlot].used()) return false;
  StoredMon copia = team[teamSlot];
  // addToBank ya sabe buscar por todas las cajas y abrir la que tenga
  // sitio; repetir el bucle aqui era lo que dejaba "banco lleno" con 930
  // huecos libres en las otras cajas.
  if (!addToBank(copia)) return false;   // de verdad no cabe en ninguna
  team[teamSlot].species = 0;
  save();
  return true;
}

// Pisar numa regiao pela PRIMEIRA vez: ovo do inicial dela.
//
// E o que faz a regiao nova comecar de baixo em vez de ser atropelada.
// O titular que voce tinha nao some: ficou no time da regiao de onde
// voce veio e volta a ser titular se voce voltar pra la —— trocar de
// regiao e, pra ele, a mesma coisa que ser trocado de titular.
void Pet::llegadaARegion() {
  // Sem heranca: o ovo e do inicial DAQUI, nao da especie que ficou
  // pra tras. Se legacyDex ficasse de pe, newEgg() chocaria o bicho
  // da regiao anterior outra vez.
  legacyDex = 0;
  newEgg();
  // newEgg() so marca starterPick na primeira partida (dex vazia), e
  // aqui ela nunca esta vazia. A escolha entre os tres e justamente o
  // que o dono pediu ("igual do inicio com charmander bulbasaur e
  // squirtle"), entao se forca.
  starterPick = true;
  mainSlot = 0;
}

bool Pet::fromBank(uint8_t bankSlot) {
  if (bankSlot >= BANK_SLOTS || !bank[bankSlot].used()) return false;
  // O CADEADO. O banco e compartilhado entre as regioes, mas aqui so
  // sai quem couber no teto daqui: sem isto, o time de Kanto inteiro
  // entrava em Johto pela porta da caixa e o teto nao servia de nada.
  if (bankLocked(bank[bankSlot])) return false;
  for (uint8_t i = 0; i < TEAM_SLOTS; i++) {
    if (team[i].used()) continue;
    team[i] = bank[bankSlot];
    bank[bankSlot].species = 0;
    save();
    return true;
  }
  return false;   // equipo lleno
}

bool Pet::releaseFromBank(uint8_t bankSlot) {
  if (bankSlot >= BANK_SLOTS || !bank[bankSlot].used()) return false;
  uint8_t it = bank[bankSlot].item;
  if (it && it < ITEM_COUNT && bag[it] < 99) bag[it]++;
  bank[bankSlot] = StoredMon();
  save();
  return true;
}

void Pet::teamShareExp(uint32_t total, uint8_t peleo) {
  // Los que no salen ganan poco, pero ganan: si no, el equipo entero se
  // queda atras del titular y cambiar de activo deja de ser una opcion.
  //
  // `peleo` es el hueco del que SI estuvo en pista, que se lleva la
  // parte entera y por tanto no cobra aqui. Por omision es el
  // principal, que es quien pelea mientras no se releve a nadie.
  if (peleo >= TEAM_SLOTS) peleo = mainSlot;
  uint32_t parte = total / 4;
  if (!parte) parte = 1;
  uint32_t tope = expForLevel(LEVEL_CAP);
  for (uint8_t i = 0; i < TEAM_SLOTS; i++) {
    if (i == peleo || !team[i].used()) continue;
    team[i].exp = (team[i].exp > tope - parte) ? tope : team[i].exp + parte;
  }
  // Y si el que peleo era un suplente, el PRINCIPAL tambien cobra su
  // parte: su experiencia vive en el Pet, no en team[mainSlot].
  if (peleo != mainSlot && !isEgg()) gainExp(parte);
}

// -------------------------------------------------------------------- EVs
uint8_t Pet::evYieldOf(int16_t dex) {
  if (dex < 1 || dex > DEX_COUNT) return EV_ATK;
  const DexEntry &d = DEX_TBL[dex];
  const uint8_t st[EV_N] = { d.bHp, d.bAtk, d.bDef, d.bSpA, d.bSpD, d.bSpe };
  uint8_t mejor = 0;
  for (uint8_t i = 1; i < EV_N; i++) if (st[i] > st[mejor]) mejor = i;
  return mejor;   // el indice YA es EV_*
}

const char *Pet::evName(uint8_t which) {
  static const char *N[EV_N] = { "HP", "ATK", "DEF", "SPA", "SPD", "SPE" };
  return which < EV_N ? N[which] : "?";
}

// ------------------------------------------------------ encuentros salvajes

bool Pet::wildCanAppear() const {
  if (isEgg() || sleeping || ceremony != CER_NONE || starterPick) return false;
  if (wildWaiting() || wildGap > 0) return false;
  // Las mismas condiciones que para pelear. Si saliera un salvaje sin fuelle
  // o medio muerto, el "!" seria un boton que solo sabe decir que no.
  return canBattle();
}

// CAZAR a mano. La hierba sale sola cada tanto, pero esperarla es lo unico
// del juego que no depende de ti: con el hueco minimo y la probabilidad
// subiendo, siguen siendo varios minutos mirando la pantalla. Esto lo pone
// en tus manos, y el enfriamiento de un minuto es lo que impide que se
// convierta en un boton de farmear sin fin.
bool Pet::hunt(uint8_t hour, uint32_t now) {
  if (isEgg() || sleeping || ceremony != CER_NONE || starterPick) return false;
  if (wildWaiting()) return false;        // ya hay uno ahi fuera
  // Leva 72: a CACADA. Uma de cada CACA_CHANCE buscas sai o cacado, desde
  // que ele possa sair ali agora (wildWeight > 0: regiao destravada, teto
  // de BST). Um LENDARIO cacado: uma de cada CACA_CHANCE_LEG, e so quando
  // ele ja pode sair (lendarioPodeSair: as 8 insignias). A tirada extra so
  // existe com cacada ligada, para nao mexer na sequencia de quem nao caca.
  int16_t d = 0;
  if (cacaDex >= 1 && cacaDex <= DEX_COUNT) {
    const bool leg = DEX_TBL[cacaDex].rarity == R_LEGENDARIO;
    if (leg ? (lendarioPodeSair(cacaDex) && random(CACA_CHANCE_LEG) == 0)
            : (wildWeight(cacaDex, hour) > 0 && random(CACA_CHANCE) == 0))
      d = cacaDex;
  }
  if (d < 1) d = pickWildSpecies(hour);
  if (d < 1) return false;
  wildDex = d;
  wildLevel = wildLevelFor(d);
  wildShiny = (random(WILD_SHINY_ODDS) == 0);
  wildTrainer = -1;
  wildTtl = WILD_TTL_MIN;
  wildWait = 0;
  huntUntil = now + HUNT_COOLDOWN_MS;
  return true;
}

uint8_t Pet::wildWeight(int16_t dex, uint8_t hour, bool fuga) const {
  if (dex < 1 || dex > DEX_COUNT) return 0;
  const DexEntry &e = DEX_TBL[dex];
  uint16_t bst = (uint16_t)e.bHp + e.bAtk + e.bDef + e.bSpA + e.bSpD + e.bSpe;
  if (bst > wildBstCap()) return 0;               // por aqui aun no se ve eso
  // Y no se ve lo de la region de MAS ALLA. El techo de BST no basta para
  // esto: Poochyena y Zigzagoon pegan menos que un Pidgey, asi que en
  // Kanto con cero insignias la hierba sacaba bichos de Hoenn. Hacia atras
  // SI valen, que es lo bonito de cambiar de region: en Hoenn te sigues
  // encontrando a los de casa.
  //
  // ...salvo en una tirada de cada WILD_FUGA, que es la que se asoma a la
  // region SIGUIENTE (`fuga`). Nunca dos regiones adelante, y el techo de
  // BST sigue mandando: lo que se cuela es flojo.
  //
  // La puerta la abre `maxRegion` (lo DESTRABADO) y no `region` (donde
  // estas): si mirara donde estas, volver a Kanto con Johto ganado
  // borraria a Johto de la hierba —— que es exactamente lo contrario de
  // lo que hace el mapa. Donde estas pesa, pero mas abajo (el x4).
  uint8_t suya = dexRegionOf(dex);
  if (fuga) {
    if (suya != (uint8_t)(maxRegion + 1)) return 0;
  } else if (suya > maxRegion) {
    return 0;
  }
  // Los legendarios NO entran en el peso comun, nunca: salen solo por
  // rollLegendary(), que es donde esta su porcentaje. Con las ocho
  // insignias entraban TAMBIEN aqui, y eso los doblaba.
  if (e.rarity == R_LEGENDARIO) return 0;

  uint8_t w = 2;
  // DONDE ESTAS. El mapa ya no hay que inventarlo: lo de la region que
  // tienes elegida sale cuatro veces mas, y lo de las otras SIGUE SALIENDO.
  // Esa es la diferencia entre un mapa y una puerta: en Johto te encuentras
  // sobre todo bichos de Johto, pero un Pidgey tambien se asoma.
  if (dexRegionOf(dex) == region) w *= 4;
  // Vives en algun sitio: lo que comparte bioma con tu bicho sale mas. Es lo
  // que hace que la zona se note sin tener que inventar un mapa.
  if (!isEgg() && speciesId > 0 && e.biome == DEX_TBL[speciesId].biome) w *= 3;
  // Y de noche anda suelta otra gente: fantasmas y venenosos de madrugada,
  // el resto de dia.
  bool noche = (hour < 6 || hour >= 20);
  bool nocturno = (e.type1 == T_GHOST || e.type2 == T_GHOST ||
                   e.type1 == T_POISON || e.type2 == T_POISON);
  if (noche == nocturno) w *= 2;
  return w;
}

// Los legendarios de una region TERMINADA, cada uno con su porcentaje.
//
// Va aparte del sorteo por pesos a proposito: con peso 2 entre mil
// especies, "ya puede salir" y "no sale nunca" son lo mismo. Asi el premio
// por acabar una region se puede decir en voz alta — un 3% por bicho — y
// encima se puede medir.
// Leva 72: QUEM pode sair. Region terminada (as 8 insignias da atual), o
// lendario e de la ou de uma de tras, e nao e o que voce ja esta criando.
// A mesma regra serve ao sorteio e a cacada.
bool Pet::lendarioPodeSair(int16_t d) const {
  if (d < 1 || d > DEX_COUNT || DEX_TBL[d].rarity != R_LEGENDARIO) return false;
  if (badges < GYM_COUNT) return false;      // la region no esta hecha
  if (dexRegionOf(d) > region) return false; // de tu region o de las de atras
  return d != speciesId;                     // no el que YA estas criando
}

int16_t Pet::rollLegendary(uint8_t hour) const {
  (void)hour;
  if (badges < GYM_COUNT) return 0;
  // Leva 72: UMA tirada para "sai algum?" e outra para "qual". O teto de
  // antes somava por ordem de dex e cortava: so 6 dos 103 tinham chance.
  if ((uint32_t)random(1000) >= LEG_PM_TOTAL) return 0;
  uint32_t total = 0;
  for (int16_t d = 1; d <= DEX_COUNT; d++)
    if (lendarioPodeSair(d)) total += dexRegionOf(d) == region ? LEG_PESO_AQUI : 1;
  if (!total) return 0;
  uint32_t r = (uint32_t)random((long)total);
  for (int16_t d = 1; d <= DEX_COUNT; d++) {
    if (!lendarioPodeSair(d)) continue;
    uint32_t w = dexRegionOf(d) == region ? LEG_PESO_AQUI : 1;
    if (r < w) return d;
    r -= w;
  }
  return 0;
}

int16_t Pet::pickWildSpecies(uint8_t hour, bool deTreinador) const {
  // Primero los legendarios: si sale uno, manda sobre el sorteo normal.
  // Para un entrenador, nunca (ver pet.h).
  // (No peso comum lendario nenhum entra: wildWeight ja os zera.)
  int16_t leg = deTreinador ? 0 : rollLegendary(hour);
  if (leg) return leg;
  auto peso = [&](int16_t d, bool f) -> uint32_t { return wildWeight(d, hour, f); };
  // Una de cada WILD_FUGA se asoma a la region siguiente. La tirada va
  // AQUI y no en los pesos: los pesos son enteros pequenos y su total
  // cambia con cada insignia (el techo de BST deja entrar mas bichos),
  // asi que "mas o menos un 10%" por esa via seria un numero que se
  // mueve solo. Asi la frecuencia es la que dice y se puede medir.
  bool fuga = (random(WILD_FUGA) == 0);
  for (uint8_t intento = 0; intento < 2; intento++) {
    uint32_t total = 0;
    for (int16_t d = 1; d <= DEX_COUNT; d++) total += peso(d, fuga);
    if (total) {
      uint32_t r = (uint32_t)random((long)total);
      for (int16_t d = 1; d <= DEX_COUNT; d++) {
        uint32_t w = peso(d, fuga);
        if (r < w) return (int16_t)d;
        r -= w;
      }
    }
    // En la ultima region no hay "siguiente", y en las primeras el techo
    // de BST puede dejarla vacia: entonces se sortea de lo de casa, que
    // es lo que habria pasado sin la fuga.
    if (!fuga) break;
    fuga = false;
  }
  return 0;
}

uint16_t Pet::wildLevelFor(int16_t dex) const {
  (void)dex;
  // Alrededor del nivel CON EL QUE PELEAS y nunca por encima de +2: el
  // salvaje esta para dar algo que hacer, no para castigar por mirar la
  // pantalla. Sale del nivel capado y no del real, o al llegar a una region
  // nueva con el equipo hecho la hierba estaria llena de bichos de 70.
  uint16_t mio = battleLevel();
  uint16_t lo = mio > 3 ? (uint16_t)(mio - 3) : 1;
  uint16_t n = (uint16_t)(lo + random(6));
  if (n > mio + 2) n = (uint16_t)(mio + 2);
  if (n > LEVEL_CAP) n = LEVEL_CAP;
  return n ? n : 1;
}

void Pet::rollWild(uint8_t hour) {
  if (wildTtl > 0 && --wildTtl == 0) wildDex = 0;   // se canso de esperar
  if (wildGap > 0) wildGap--;
  if (!wildCanAppear()) return;
  // La probabilidad crece con la espera (ver WILD_CHANCE_* en pet.h). El
  // contador solo corre cuando el sorteo esta ABIERTO: los minutos de
  // hueco, o los que pasa el bicho dormido, no cuentan como "llevo mucho
  // esperando", porque en ellos no podia salir nada.
  uint16_t pct = (uint16_t)WILD_CHANCE_PCT + (uint16_t)wildWait * WILD_CHANCE_STEP;
  if (pct > WILD_CHANCE_MAX) pct = WILD_CHANCE_MAX;
  if (wildWait < 255) wildWait++;
  if (random(100) >= (int)pct) return;
  // O aviso automatico e sempre um treinador (ver abaixo): sem lendario.
  int16_t d = pickWildSpecies(hour, true);
  // El contador se borra cuando SALE uno de verdad, no al ganar el
  // sorteo: si la hora no tiene ningun bicho que ofrecer, la espera
  // acumulada no se puede tirar a la basura.
  if (d <= 0) return;
  wildWait = 0;
  wildDex = d;
  wildLevel = wildLevelFor(d);
  wildShiny = (random(WILD_SHINY_ODDS) == 0);
  // EL AVISO AUTOMATICO ES SIEMPRE UN ENTRENADOR. Antes era 75% salvaje y
  // 25% entrenador, y no habia forma de saber cual era hasta entrar.
  //
  // Ahora cada puerta hace una cosa: los salvajes salen por CAZAR, que es
  // cuando el jugador DECIDE ir a buscarlos, y el "!" que aparece solo es
  // el camino hasta el gimnasio. Su bicho no se captura pero paga mejor,
  // y el nivel es el MISMO que el tuyo: lo que sube el liston es que
  // salen varios seguidos y tu vas gastando PS y PP por el camino.
  wildTrainer = (int8_t)random(TRAINER_COUNT);
  wildShiny = false;     // el bicho de un entrenador es suyo, no un hallazgo
  wildTtl = WILD_TTL_MIN;
}

// ------------------------------------------------------------- gimnasios

void Pet::winWild() {
  // Ya no abre ningun porton —— de eso se encargan los tres de la casa ——,
  // pero la victoria se apunta: es la cuenta que ensena la ficha.
  if (wins < 0xFFFFFFFFul) wins++;
}

// Uno de los TRES de la casa. Con los tres, `gymReady()` abre el lider.
void Pet::winGymTrainer() {
  if (wins < 0xFFFFFFFFul) wins++;
  if (allGymsDone()) return;
  if (gymTrainers < GYM_TRAINERS) gymTrainers++;
  save();   // es progresion, no un aviso pasajero
}

void Pet::winGym() {
  if (allGymsDone()) return;
  // El premio ANTES de subir el contador: `gymHere()` mira `badges`, y si
  // se incrementa primero se estaria cobrando el premio del siguiente.
  int8_t p = gymHere().prize;
  lastBallId = 0;
  lastBallCount = 0;
  if (p == GP_BALLS) {
    // Tres del mejor escalon que se regala. Los ultimos desafios son los
    // unicos sitios donde caen Ultra de golpe.
    uint8_t tier = badges >= GYM_BADGES ? 2 : (badges >= 4 ? 1 : 0);
    for (uint8_t i = 0; i < 3; i++) giveBall(tier);
    lastBallCount = 3;
  } else {
    // Una piedra (o el Cable). Es la UNICA via de conseguirlas: sin esto,
    // quien evoluciona con piedra no evolucionaba nunca.
    for (uint8_t i = 1; i < ITEM_COUNT; i++)
      if (ITEM_TBL[i].fx == IT_STONE && ITEM_TBL[i].arg == (uint8_t)p) {
        if (bag[i] < 99) bag[i]++;
        lastBallId = i;      // la pantalla lo cuenta igual que una bola
        lastBallCount = 1;
        break;
      }
  }
  // Y la MT del lider, SIEMPRE, encima de lo otro. Es lo que hace que
  // ganar una insignia abra algo nuevo y no solo suba un contador: de
  // golpe tu bicho puede aprender algo que no aprendia.
  //
  // Se sortea entre las que EL podria aprender: una MT que no le sirve
  // a nadie es un premio que no se nota.
  // `gymHere()` todavia apunta al lider que se acaba de ganar: `badges`
  // sube despues, a proposito (ver arriba).
  lastTM = rollTM(gymHere().type);
  if (lastTM >= 0) giveTM((uint16_t)lastTM);
  badges++;
  badgesIn[region] = badges;    // y en la cuenta de ESTA region
  if (wins < 0xFFFFFFFFul) wins++;
  gymTrainers = 0;  // los tres del siguiente empiezan de cero
  // Terminada la region, la SIGUIENTE queda abierta en el mapa. No te
  // manda alli ni te quita nada: solo deja de estar con candado.
  if (allGymsDone() && (uint8_t)(region + 1) < REGION_COUNT &&
      maxRegion < (uint8_t)(region + 1))
    maxRegion = (uint8_t)(region + 1);
  save();          // esto SI se guarda: es progresion, no un aviso pasajero
}

// Viajar a la region siguiente. Las insignias vuelven a CERO, y con ellas el
// techo de nivel (`levelCap`) y el techo de BST de los salvajes: es lo que
// hace que el primer gimnasio de Johto se pelee a la altura del primero de
// Kanto aunque llegues con el equipo hecho. No se pierde nada mas — ni
// nivel, ni EXP, ni equipo, ni la Pokedex.
// Ya NO pone nada a cero. Las insignias de cada region viven en
// `badgesIn` y se intercambian al moverse, asi que ir a Johto y volver
// te devuelve tus ocho de Kanto intactas. Lo que se gana al terminar una
// region es la REVANCHA —— quitarle las insignias al que acaba de
// coronarse es decirle que lo que hizo no cuenta.
bool Pet::advanceRegion() {
  if (!canAdvanceRegion()) return false;
  uint8_t destino = (uint8_t)(region + 1);
  if (destino > maxRegion) maxRegion = destino;
  return setZone(destino);
}

// ------------------------------------------------- el centro Pokemon
bool Pet::needsHealing() const {
  if (isEgg()) return false;
  if (hpLeft < maxHp()) return true;
  for (uint8_t i = 0; i < 4; i++)
    if (ppMove[i] && ppLeft[i] < movePP(ppMove[i])) return true;
  // Y el banquillo, que es para quien esto existe de verdad: el titular ya
  // se curaba durmiendo, los suplentes no dormian.
  for (uint8_t i = 0; i < TEAM_SLOTS; i++) {
    const StoredMon &m = team[i];
    if (!m.used() || i == mainSlot) continue;
    if (m.hpLeft < hpAtLevelForm(m.species, Pet::levelOf(m), m.trHp, m.form))
      return true;
    // Sin PP tambien: un suplente que ya no puede atacar necesita a la
    // enfermera aunque tenga la vida llena.
    for (uint8_t k = 0; k < 4; k++)
      if (m.moves[k] && m.ppLeft[k] < movePP(m.moves[k])) return true;
  }
  return false;
}

void Pet::pcTick(uint32_t now) {
  if (isEgg() || sleeping || ceremony != CER_NONE) return;
  if (pcWaiting(now)) {
    // Si mientras estaba en pantalla se curo todo por otro lado (una baya,
    // una siesta), se va sola: una enfermera esperando a que la toques para
    // no hacer nada es un boton roto.
    if (!needsHealing()) { pcNext = 0; pcGone = 0; }
    return;
  }
  if (!needsHealing()) { pcNext = 0; pcGone = 0; return; }
  if (!pcNext || (int32_t)(now - pcGone) >= 0) {
    pcNext = now + PC_EVERY_MS;
    pcGone = pcNext + PC_STAY_MS;
  }
}

// La visita del Profesor. Mismo esqueleto que pcTick, con dos
// diferencias: no depende de que haga falta nada (no viene a resolver un
// problema, viene a jugar) y por eso tampoco se va antes de tiempo.
void Pet::quizTick(uint32_t now) {
  if (isEgg() || sleeping || ceremony != CER_NONE) return;
  if (quizWaiting(now)) return;
  if (!quizNext || (int32_t)(now - quizGone) >= 0) {
    quizNext = now + QUIZ_EVERY_MS;
    quizGone = quizNext + QUIZ_STAY_MS;
  }
}

uint8_t Pet::healAll() {
  uint8_t curados = 0;
  if (!isEgg()) {
    uint16_t tope = maxHp();
    bool algo = hpLeft < tope;
    hpLeft = tope;
    hpSeen = tope;
    for (uint8_t i = 0; i < 4; i++)
      if (ppMove[i] && ppLeft[i] < movePP(ppMove[i])) algo = true;
    ppRestoreAll();
    if (algo) curados++;
  }
  // El equipo Y el banco. "Todos" quiere decir todos: un bicho desmayado
  // guardado en la caja no se cura con nada mas, y dejarlo asi para siempre
  // no es una regla, es un olvido.
  StoredMon *listas[2] = { team, bank };
  const uint8_t cuantos[2] = { TEAM_SLOTS, BANK_SLOTS };
  for (uint8_t L = 0; L < 2; L++) {
    for (uint8_t i = 0; i < cuantos[L]; i++) {
      StoredMon &m = listas[L][i];
      if (!m.used()) continue;
      // El TITULAR vive en dos sitios: aqui arriba y en team[mainSlot]. Si
      // no se salta, se cuenta dos veces y el cartel dice "2 restored" con
      // un solo bicho en el equipo. Curarlo igualmente no sobra —— el de
      // arriba ya le puso el tope ——, lo que no vale es contarlo.
      bool esElTitular = (L == 0 && i == mainSlot && !isEgg());
      uint16_t tope = hpAtLevelForm(m.species, Pet::levelOf(m), m.trHp, m.form);
      bool algo = m.hpLeft < tope;
      m.hpLeft = tope;
      if (esElTitular) algo = false;
      // Y los PP: cada StoredMon guarda los suyos, y el relevo entra al
      // combate con ellos tal como quedaron. Aqui decia que no guardaba
      // ninguno, y la enfermera dejaba al equipo sin PP.
      for (uint8_t k = 0; k < 4; k++) {
        if (!m.moves[k]) continue;
        uint8_t lleno = movePP(m.moves[k]);
        if (m.ppLeft[k] < lleno) algo = true;
        m.ppLeft[k] = lleno;
      }
      if (esElTitular) algo = false;
      if (algo) curados++;
    }
  }
  // El titular vive en DOS sitios (aqui y en team[mainSlot]) y el de arriba
  // ya le ha puesto el tope a los dos: storeActive() los deja de acuerdo
  // antes de guardar, no sea que una copia quede a medias.
  storeActive();
  pcNext = 0;
  pcGone = 0;
  save();
  return curados;
}

uint16_t Pet::evTotal() const {
  return (uint16_t)trHp + trAtk + trDef + trSpa + trSpd + trSpe;
}

uint8_t Pet::evOf(uint8_t which) const {
  switch (which) {
    case EV_HP:  return trHp;
    case EV_ATK: return trAtk;
    case EV_DEF: return trDef;
    case EV_SPA: return trSpa;
    case EV_SPD: return trSpd;
    case EV_SPE: return trSpe;
    default:     return 0;
  }
}

// La cuenta de los EVs, sobre SEIS campos cualesquiera. Esta aparte
// porque la necesitan dos: el bicho principal (que los lleva sueltos
// en el Pet) y cualquier suplente (que los lleva en su StoredMon).
//
// Antes solo existia la version del principal, y de ahi salia un fallo
// de verdad: peleando con un suplente, los EVs se los quedaba EL
// PRINCIPAL, que no habia peleado. Ver Pet::gainEVOf.
static uint8_t sumaEV(uint8_t *tr, uint8_t which, uint8_t cuanto) {
  if (!cuanto || which >= EV_N) return 0;
  uint8_t *dst = &tr[which];
  if (*dst >= EV_STAT_CAP) return 0;
  uint16_t hueco = (uint16_t)(EV_STAT_CAP - *dst);
  // ...y el hueco que quede en el presupuesto comun. Una partida vieja
  // puede venir ya por encima del total (antes no habia techo): no se le
  // quita nada, simplemente deja de crecer.
  uint16_t total = 0;
  for (uint8_t i = 0; i < EV_N; i++) total += tr[i];
  uint16_t bolsa = total >= EV_TOTAL_CAP ? 0 : (uint16_t)(EV_TOTAL_CAP - total);
  if (bolsa < hueco) hueco = bolsa;
  if (!hueco) return 0;
  uint8_t entra = cuanto > hueco ? (uint8_t)hueco : cuanto;
  *dst = (uint8_t)(*dst + entra);
  return entra;
}

uint8_t Pet::gainEV(uint8_t which, uint8_t cuanto) {
  if (isEgg()) return 0;
  // El orden de EV_* es el de estos seis, y eso lo fija un test
  // (los_seis_EV_estan_en_el_orden_de_la_estructura).
  uint8_t tr[EV_N] = { trHp, trAtk, trDef, trSpa, trSpd, trSpe };
  uint8_t entra = sumaEV(tr, which, cuanto);
  trHp = tr[0]; trAtk = tr[1]; trDef = tr[2];
  trSpa = tr[3]; trSpd = tr[4]; trSpe = tr[5];
  return entra;
}

uint8_t Pet::gainEVOf(uint8_t slot, uint8_t which, uint8_t cuanto) {
  // Los EVs son de QUIEN PELEA. Hasta ahora siempre se los quedaba el
  // principal, aunque el que estuviera en pista fuera un suplente: el
  // que se lo curraba no se llevaba nada y el que miraba desde el
  // banquillo se llevaba todo. Justo al reves de lo que el jugador
  // espera cuando cambia de bicho para no estropearle el reparto.
  if (slot >= TEAM_SLOTS || slot == mainSlot) return gainEV(which, cuanto);
  StoredMon &m = team[slot];
  if (!m.used()) return 0;
  uint8_t tr[EV_N] = { m.trHp, m.trAtk, m.trDef, m.trSpa, m.trSpd, m.trSpe };
  uint8_t entra = sumaEV(tr, which, cuanto);
  m.trHp = tr[0]; m.trAtk = tr[1]; m.trDef = tr[2];
  m.trSpa = tr[3]; m.trSpd = tr[4]; m.trSpe = tr[5];
  return entra;
}

uint8_t Pet::gainExpOf(uint8_t slot, uint32_t cuanta) {
  if (slot >= TEAM_SLOTS || slot == mainSlot) return gainExp(cuanta);
  StoredMon &m = team[slot];
  if (!m.used() || !cuanta) return 0;
  uint16_t antes = levelOf(m);
  uint32_t tope = expForLevel(LEVEL_CAP);
  m.exp = (m.exp > tope - cuanta) ? tope : m.exp + cuanta;
  return (uint8_t)(levelOf(m) - antes);
}

// ------------------------------------------------------------ experiencia
uint8_t Pet::gainExp(uint32_t cuanta) {
  if (isEgg() || !cuanta) return 0;
  uint16_t antes = level();
  if (antes >= LEVEL_CAP) return 0;
  uint32_t tope = expForLevel(LEVEL_CAP);
  exp = (exp > tope - cuanta) ? tope : exp + cuanta;
  uint16_t ahora = level();
  if (ahora > antes) {
    hpSync();   // el maximo sube y la subida se suma a los PS de ahora
    ppSync();
    sfxPlay(SFX_LEVEL);
  }
  return (uint8_t)(ahora - antes);
}

uint32_t Pet::expFromFoe(int16_t foeDex, uint16_t foeLevel) {
  if (foeDex < 1 || foeDex > DEX_COUNT) return 0;
  const DexEntry &d = DEX_TBL[foeDex];
  uint32_t bst = (uint32_t)d.bHp + d.bAtk + d.bDef + d.bSpA + d.bSpD + d.bSpe;
  // La de los juegos es base_exp * nivel / 7. Aqui no hay base_exp por
  // especie, y el total de stats base es ~5 veces mayor (Magikarp: 200 de
  // total contra 40 de base_exp), asi que el divisor lo absorbe. Sin eso, un
  // solo Magikarp llevaba del nivel 1 al 7 de golpe.
  //
  // Y el 35 de antes paso a EXP_DIVISOR, mas generoso. El motivo no es que
  // la formula estuviera mal: es que AQUI se pelea mucho menos. En los
  // juegos cruzas una ruta y te salen treinta combates; en el reloj la
  // hierba saca uno cada varios minutos y el boton de cazar tiene un
  // minuto de espera. Con el divisor viejo, una hora de juego apenas movia
  // la barra a partir del nivel 30.
  uint32_t e = bst * (foeLevel ? foeLevel : 1) / EXP_DIVISOR;
  return e ? e : 1;
}

void Pet::rollAbility() {
  if (speciesId < 1 || speciesId > DEX_COUNT) { ability = 0; return; }
  const uint16_t *a = ABIL_OF[speciesId];
  if (a[2] && random(32) == 0) { ability = a[2]; return; }
  if (a[1] && random(2) == 0) { ability = a[1]; return; }
  ability = a[0] ? a[0] : a[1];
}

// El sexo del individuo, con el reparto de su especie. Se tira UNA vez.
void Pet::rollGender() {
  genderSet = true;
  if (speciesId < 1 || speciesId > DEX_COUNT) { female = false; return; }
  uint8_t p = DEX_TBL[speciesId].femPct;
  female = (p != GENDERLESS) && (p > 0) && ((int)random(100) < p);
}

const char *Pet::genderMark() const {
  if (speciesId < 1 || speciesId > DEX_COUNT) return "";
  if (DEX_TBL[speciesId].femPct == GENDERLESS) return "";
  return female ? " F" : " M";
}

bool Pet::moveEquipped(uint16_t mv) const {
  for (uint8_t i = 0; i < 4; i++) if (ppMove[i] == mv) return true;
  return false;
}

bool Pet::equipMove(uint16_t mv) {
  if (isEgg() || mv >= MOVE_COUNT || moveEquipped(mv)) return false;
  for (uint8_t i = 0; i < 4; i++) {
    if (ppMove[i]) continue;
    ppMove[i] = mv;
    ppLeft[i] = movePP(mv);   // el que entra, entra con los PP llenos
    movesPicked = true;
    save();
    return true;
  }
  return false;                // los cuatro ocupados: primero hay que soltar
}

bool Pet::unequipMove(uint8_t slot) {
  if (slot >= 4 || !ppMove[slot]) return false;
  ppMove[slot] = 0;
  ppLeft[slot] = 0;
  movesPicked = true;
  save();
  return true;
}

// Los cuatro de un SUPLENTE. Si nunca le han tocado los golpes, `moves[]`
// esta vacio (o con lo de antes de evolucionar): lo que de verdad sacaria al
// campo es el aprendizaje de su nivel, que es lo que hace btFromSlot. Aqui se
// devuelve ESO, para que la pantalla no mienta.
bool Pet::teamMoves(uint8_t slot, uint16_t *out4, uint8_t *pp4) const {
  if (slot >= TEAM_SLOTS || slot == mainSlot) return false;
  const StoredMon &m = team[slot];
  if (!m.used()) return false;
  if (m.flags & SM_MOVES_PICKED) {
    for (uint8_t i = 0; i < 4; i++) { out4[i] = m.moves[i]; pp4[i] = m.ppLeft[i]; }
    return true;
  }
  uint16_t mv[4];
  uint8_t n = movesAtLevel(m.species, levelOf(m), mv);
  for (uint8_t i = 0; i < 4; i++) {
    out4[i] = (i < n) ? mv[i] : 0;
    pp4[i] = out4[i] ? movePP(out4[i]) : 0;
  }
  return true;
}

// Al elegir por primera vez hay que CONGELAR lo que tenia: si no, el primer
// golpe que pongas se sumaria a un `moves[]` vacio y el bicho saldria al
// campo con uno solo.
static void fijaGolpes(StoredMon &m) {
  if (m.flags & SM_MOVES_PICKED) return;
  uint16_t mv[4];
  uint8_t n = movesAtLevel(m.species, Pet::levelOf(m), mv);
  for (uint8_t i = 0; i < 4; i++) {
    m.moves[i] = (i < n) ? mv[i] : 0;
    m.ppLeft[i] = m.moves[i] ? movePP(m.moves[i]) : 0;
  }
  m.flags |= SM_MOVES_PICKED;
}

bool Pet::teamEquipMove(uint8_t slot, uint16_t mv) {
  if (slot >= TEAM_SLOTS || slot == mainSlot) return false;
  StoredMon &m = team[slot];
  if (!m.used() || !mv || mv >= MOVE_COUNT) return false;
  fijaGolpes(m);
  for (uint8_t i = 0; i < 4; i++) if (m.moves[i] == mv) return false;
  for (uint8_t i = 0; i < 4; i++) {
    if (m.moves[i]) continue;
    m.moves[i] = mv;
    m.ppLeft[i] = movePP(mv);   // el que entra, entra con los PP llenos
    save();
    return true;
  }
  return false;                 // los cuatro ocupados: primero hay que soltar
}

bool Pet::teamUnequipMove(uint8_t slot, uint8_t i) {
  if (slot >= TEAM_SLOTS || slot == mainSlot || i >= 4) return false;
  StoredMon &m = team[slot];
  if (!m.used()) return false;
  fijaGolpes(m);
  if (!m.moves[i]) return false;
  m.moves[i] = 0;
  m.ppLeft[i] = 0;
  save();
  return true;
}

void Pet::ppSync() {
  if (isEgg()) {
    for (uint8_t i = 0; i < 4; i++) { ppMove[i] = 0; ppLeft[i] = 0; }
    return;
  }
  if (movesPicked) {
    // Manda el jugador: solo se comprueba que los PP no pasen del tope (por
    // si la tabla cambia entre versiones). Los huecos vacios se quedan
    // vacios: dejarlos con menos de cuatro golpes es una eleccion valida.
    for (uint8_t i = 0; i < 4; i++) {
      if (!ppMove[i]) { ppLeft[i] = 0; continue; }
      if (ppLeft[i] > movePP(ppMove[i])) ppLeft[i] = movePP(ppMove[i]);
    }
    return;
  }
  uint16_t mv[4];
  uint8_t n = movesAtLevel(speciesId, level(), mv);
  for (uint8_t i = 0; i < 4; i++) {
    uint16_t nuevo = (i < n) ? mv[i] : 0;
    if (nuevo != ppMove[i]) {
      // hueco vacio, golpe recien aprendido, o especie cambiada al evolucionar:
      // el que entra lo hace con los PP llenos, no con los del anterior
      ppMove[i] = nuevo;
      ppLeft[i] = nuevo ? movePP(nuevo) : 0;
    } else if (nuevo && ppLeft[i] > movePP(nuevo)) {
      ppLeft[i] = movePP(nuevo);   // por si la tabla cambia entre versiones
    }
  }
}

void Pet::ppRestoreAll() {
  ppSync();
  for (uint8_t i = 0; i < 4; i++)
    if (ppMove[i]) ppLeft[i] = movePP(ppMove[i]);
}

uint8_t Pet::ppRestore(uint8_t cuanto) {
  ppSync();
  uint8_t dado = 0;
  // Se reparte empezando por el golpe MAS gastado: si no, una baya se gasta
  // entera en el que ya estaba casi lleno y no se nota nada.
  while (cuanto) {
    int peor = -1, falta = 0;
    for (uint8_t i = 0; i < 4; i++) {
      if (!ppMove[i]) continue;
      int f = (int)movePP(ppMove[i]) - (int)ppLeft[i];
      if (f > falta) { falta = f; peor = i; }
    }
    if (peor < 0) break;   // todos llenos: lo que sobra no se pierde, no se usa
    ppLeft[peor]++;
    dado++;
    cuanto--;
  }
  return dado;
}

// -------------------------------------------------------------- objetos
bool Pet::takeItem(uint8_t id) {
  if (id == 0 || id >= ITEM_COUNT || bag[id] >= 99) return false;
  bag[id]++;
  save();   // encontrar algo y perderlo al apagar seria de las cosas que mas
            // molestan, y pasa poco: como mucho una vez por combate
  return true;
}

bool Pet::holdItem(uint8_t id) {
  if (id >= ITEM_COUNT) return false;
  if (id && !bag[id]) return false;      // no se puede llevar lo que no se tiene
  if (heldItem) bag[heldItem]++;         // lo que llevaba vuelve a la bolsa
  if (id) bag[id]--;
  heldItem = id;
  save();
  return true;
}

// El que LLEVA PUESTO cuenta como si siguiera en la bolsa.
//
// holdItem() descuenta el objeto del inventario (bag[id]--), asi que
// con un solo ejemplar -- que es lo normal -- al ponerselo desaparecia
// de la lista. Y como quitarselo es tocarlo OTRA VEZ en esa lista, ya
// no habia forma: se quedaba puesto para siempre. El dueno lo encontro
// a la primera ("ja equipou direto... e nao sai").
//
// Se resuelve aqui, en el listado, y no en holdItem(): el inventario
// esta bien como esta -- un objeto puesto no esta en la mochila -- lo
// que faltaba era VERLO para poder devolverlo.
static bool enEstaLista(const Pet &p, uint8_t i, uint8_t pocket) {
  if (ITEM_TBL[i].pocket != pocket) return false;
  // Os itens-chave sao de enfeite e estao sempre la (Town Map, Pokedex,
  // Mega Ring...): o bolso vazio parecia um defeito.
  if (ITEM_TBL[i].fx == IT_KEY) return true;
  // O que alguem do time segura tambem aparece (pela mesma razao do
  // titular): e por aqui que se tira de volta.
  return p.bag[i] || p.heldItem == i || p.quemLeva(i) != 0xFF;
}

uint8_t Pet::quemLeva(uint8_t id) const {
  if (!id) return 0xFF;
  for (uint8_t s = 0; s < TEAM_SLOTS; s++)
    if (s != mainSlot && team[s].used() && team[s].item == id) return s;
  return 0xFF;
}

bool Pet::darItem(uint8_t slot, uint8_t id) {
  if (slot >= TEAM_SLOTS || !team[slot].used() || !id || id >= ITEM_COUNT) return false;
  if (slot == mainSlot) return holdItem(heldItem == id ? 0 : id);
  StoredMon &m = team[slot];
  if (m.item == id) {            // o mesmo: tirar
    bag[id]++;
    m.item = 0;
    save();
    return true;
  }
  if (!bag[id]) {
    // Na bolsa nao ha, mas outro do time leva um: passa dele para este
    // (e o caminho para mudar de ideia sem tirar primeiro).
    uint8_t outro = quemLeva(id);
    if (outro == 0xFF) {
      if (heldItem != id) return false;
      holdItem(0);               // o titular devolve, e segue abaixo
    } else {
      team[outro].item = 0;
      bag[id]++;
    }
  }
  if (m.item) bag[m.item]++;     // o que levava volta a bolsa
  bag[id]--;
  m.item = id;
  save();
  return true;
}

uint8_t Pet::pocketCount(uint8_t pocket) const {
  uint8_t n = 0;
  for (uint8_t i = 1; i < ITEM_COUNT; i++)
    if (enEstaLista(*this, i, pocket)) n++;
  return n;
}

// ¿Va `a` antes que `b` con el orden pedido? Separado de la busqueda
// porque los dos modos ordenados solo se distinguen en esta linea.
static bool vaAntes(uint8_t a, uint8_t b, uint8_t modo) {
  if (modo == 2 && ITEM_TBL[a].fx != ITEM_TBL[b].fx)
    return ITEM_TBL[a].fx < ITEM_TBL[b].fx;
  // pelo nome do idioma (leva 78): em frances a ordem do ingles parecia
  // aleatoria. Bytes: no japones a ordem dos kana e a do Unicode.
  int c = strcmp(itemName(a), itemName(b));
  if (c != 0) return c < 0;
  return a < b;            // desempate estable: dos nombres iguales
}

uint8_t Pet::pocketItem(uint8_t pocket, uint8_t n) const {
  if (bagSort == 0) {
    // La ordenacion de la tabla, que es como estaba: sin copiar nada.
    for (uint8_t i = 1; i < ITEM_COUNT; i++) {
      if (!enEstaLista(*this, i, pocket)) continue;
      if (n-- == 0) return i;
    }
    return 0;
  }
  // Los dos modos ordenados. Se monta la lista del bolsillo y se ordena
  // por insercion: son los objetos que el jugador TIENE en ese bolsillo,
  // no los 211 de la tabla, asi que esto se queda en unas pocas decenas.
  uint8_t lista[ITEM_COUNT];
  uint8_t cuantos = 0;
  for (uint8_t i = 1; i < ITEM_COUNT; i++) {
    if (!enEstaLista(*this, i, pocket)) continue;
    uint8_t j = cuantos++;
    while (j > 0 && vaAntes(i, lista[j - 1], bagSort)) {
      lista[j] = lista[j - 1];
      j--;
    }
    lista[j] = i;
  }
  return n < cuantos ? lista[n] : 0;
}

bool Pet::spendItem(uint8_t id) {
  if (id == 0 || id >= ITEM_COUNT || !bag[id]) return false;
  bag[id]--;
  save();
  return true;
}

// Gastar uno FUERA del combate. Devuelve false si no lo tiene o si no
// serviria de nada — dar una pocion a un bicho entero no deberia tirarla
// a la basura.
//
// Dentro del combate esto NO vale y no se llama: mira `hpLeft`, que
// durante el combate esta congelado (solo se escribe al salir de pista,
// ver btStoreActive), y cura al PRINCIPAL aunque el que este peleando
// sea un suplente. De eso se encarga btItemOnField en capture.ino.
bool Pet::useItem(uint8_t id) {
  if (id == 0 || id >= ITEM_COUNT || !bag[id]) return false;
  const ItemEntry &it = ITEM_TBL[id];
  hpSync();
  uint16_t tope = maxHp();
  bool sirvio = false;

  switch (it.fx) {
    case IT_POTION:
      if (!tope || hpLeft >= tope || hpLeft == 0) break;   // entero o desmayado
      hpHeal(it.arg == 255 ? tope : it.arg);
      sirvio = true;
      break;
    case IT_FULLREST:
      if (!tope || (hpLeft >= tope && !ppEmpty())) break;
      hpLeft = tope;
      sirvio = true;
      break;
    case IT_REVIVE:
      // solo sirve si esta en los huesos; aqui no hay "desmayado" permanente,
      // asi que el umbral es el mismo que le impide pelear
      if (!tope || (uint32_t)hpLeft * 100 >= (uint32_t)tope * BATTLE_HP_FLOOR_PCT) break;
      hpLeft = (uint16_t)((uint32_t)tope * it.arg / 100);
      sirvio = true;
      break;
    case IT_ETHER:
    case IT_ELIXIR:
      ppSync();
      if (it.arg == 255 && it.fx == IT_ELIXIR) {
        // el maximo de verdad: los cuatro huecos a tope
        bool faltaba = false;
        for (uint8_t k = 0; k < 4; k++)
          if (ppMove[k] && ppLeft[k] < movePP(ppMove[k])) faltaba = true;
        if (faltaba) { ppRestoreAll(); sirvio = true; }
      } else {
        // ppRestore reparte empezando por el mas gastado, que es justo donde
        // hace falta; un eter de 10 acaba yendo casi entero a un solo golpe
        sirvio = ppRestore(it.arg == 255 ? 60 : it.arg) > 0;
      }
      break;
    case IT_VITAMIN:
      // El camino rapido a los EVs. El lento y gratis es cazar a la especie
      // que entrena lo que quieres; esto lo acelera a cambio de gastar algo
      // que solo se encuentra ganando.
      sirvio = gainEV(it.arg, 10) > 0;
      break;
    case IT_RARECANDY: {
      // Un nivel, por el MISMO camino que la experiencia de un combate:
      // asi trae los PS de la subida, los PP y el sonido.
      if (isEgg()) break;
      const uint16_t lv = level();
      if (lv >= LEVEL_CAP) break;
      sirvio = gainExp(expForLevel(lv + 1) - exp) > 0;
      break;
    }
    case IT_SHINY:
      // El premio raro del cuestionario: el titular se vuelve shiny. Ya
      // shiny, no se gasta.
      if (isEgg() || shiny) break;
      shiny = true;
      sirvio = true;
      break;
    case IT_XSTAT:
    case IT_XCRIT:
      // fuera de combate no hay stages que subir: se guarda para el combate
      break;
    default:
      break;
  }

  if (!sirvio) return false;
  bag[id]--;
  save();
  return true;
}

// Que se encuentra al ganar. Los buenos de verdad (Life Orb, Choice, Focus)
// salen poco: si cayeran igual que una baya, el combate dejaria de tener
// tension a la semana.
// Una piedra que le sirva A ALGUIEN de los tuyos, o 0 si ninguna.
//
// Esto importa mas que la probabilidad: con siete objetos de evolucion y
// sorteo plano, recibir justo el que te hace falta es 1 entre 7, y lo
// normal era acumular Piedras Hoja para un equipo que no tiene ni una
// Oddish. Mirando a quien tienes, la piedra que cae SIRVE.
uint8_t Pet::usefulStone() const {
  uint8_t cand[8], n = 0;
  for (uint8_t i = 1; i < ITEM_COUNT && n < 8; i++) {
    if (ITEM_TBL[i].fx != IT_STONE) continue;
    bool sirve = (!isEgg() && stoneTarget(speciesId, ITEM_TBL[i].arg) != 0);
    for (uint8_t s = 0; s < TEAM_SLOTS && !sirve; s++)
      if (team[s].used() && stoneTarget(team[s].species, ITEM_TBL[i].arg))
        sirve = true;
    if (sirve) cand[n++] = i;
  }
  return n ? cand[random(n)] : 0;
}

uint8_t Pet::rollBattleDrop(bool rico) {
  // Un entrenador suelta mas a menudo: cuesta mas ganarle (saca varios) y
  // ese es el pago.
  if (random(100) >= (rico ? 60 : 35)) return 0;
  // Se sortea por BOLSILLO y no por indices sueltos: asi anadir objetos a la
  // tabla no descuadra las probabilidades ni hace falta tocar esto.
  long t = random(100);
  uint8_t pocket;
  if (rico) {
    // Los entrenadores son la via razonable de conseguir piedras: los
    // lideres solo dan UNA de cada, y con una Piedra Agua no evolucionan
    // Poliwhirl, Shellder, Staryu y Eevee a la vez.
    pocket = (t < 30) ? BP_BATTLE : (t < 55) ? BP_HELD
           : (t < 80) ? BP_BALL : BP_EVO;
  } else {
    pocket = (t < 52) ? BP_BATTLE      // pociones y curas: lo normal
           : (t < 76) ? BP_HELD        // algo que llevar encima
           : (t < 90) ? BP_BALL        // una bola
                      : BP_EVO;        // una piedra, de vez en cuando
  }
  // Si toca piedra, se intenta primero una que le sirva a alguien tuyo.
  if (pocket == BP_EVO) {
    uint8_t util = usefulStone();
    if (util && takeItem(util)) return util;
  }
  // se cuenta cuantos hay de ese bolsillo en la TABLA (no en la bolsa) y se
  // elige uno al azar entre ellos
  uint8_t n = 0;
  for (uint8_t i = 1; i < ITEM_COUNT; i++)
    if (ITEM_TBL[i].pocket == pocket && !soDoQuiz(i)) n++;
  if (!n) return 0;
  uint8_t k = (uint8_t)random(n), id = 0;
  for (uint8_t i = 1; i < ITEM_COUNT; i++) {
    if (ITEM_TBL[i].pocket != pocket || soDoQuiz(i)) continue;
    if (k-- == 0) { id = i; break; }
  }
  return (id && takeItem(id)) ? id : 0;
}

void Pet::battleSave() {
  guarda("ppm", ppMove, sizeof(ppMove));
  guarda("ppl", ppLeft, sizeof(ppLeft));
  prefs.putUShort("hp", hpLeft);
  prefs.putUShort("hpsn", hpSeen);
}

// ---------------------------------------------------------------------- PS
// Misma regla que los PP: atraviesan los combates y los devuelve el cuidado.
// Ganar raspando cuesta algo, ganar limpio no cuesta nada: eso es lo que hace
// que jugar bien valga la pena.

void Pet::hpSync() {
  if (isEgg()) { hpLeft = 0; hpSeen = 0; return; }
  uint16_t tope = maxHp();
  if (hpSeen == 0) {
    hpLeft = tope;                       // recien nacido, o partida vieja
  } else if (tope > hpSeen) {
    // Subir de nivel (o evolucionar) SUMA a los PS de ahora, como en los
    // juegos. Si no, el bicho iria pareciendo cada vez mas tocado sin que
    // nadie le hiciera nada.
    hpLeft = (uint16_t)(hpLeft + (tope - hpSeen));
  }
  hpSeen = tope;
  if (hpLeft > tope) hpLeft = tope;
}

void Pet::hpHeal(uint16_t cuanto) {
  hpSync();
  uint16_t tope = maxHp();
  if (!tope) return;
  hpLeft = (uint32_t)hpLeft + cuanto > tope ? tope : (uint16_t)(hpLeft + cuanto);
}

void Pet::hpFaint() {
  hpSync();
  uint16_t tope = maxHp();
  // No se queda a cero: se levanta tocado. Asi la pantalla nunca tiene que
  // enseñar un bicho "muerto" y siempre hay camino de vuelta.
  uint16_t suelo = (uint16_t)((uint32_t)tope * FAINT_HP_PCT / 100);
  hpLeft = suelo ? suelo : 1;
  joy = clamp100((int)joy - FAINT_JOY_COST);
  sleeping = true;   // se desmaya: se duerme solo, y durmiendo es como se cura

  // Y aqui esta TODO el castigo, que ademas no es de ahora: cuenta como error
  // de cuidado, y los errores empeoran el sorteo del proximo huevo y la tirada
  // de shiny. No se pierde el bicho, ni nivel, ni nada del presente: se pierde
  // un poco del futuro. Eso es tamagotchi, no RPG.
  if (mistakeCooldown == 0) {
    careMistakes++;
    mistakeCooldown = 60;
    if (bond > 1) bond--;
  }
  save();
}

bool Pet::canBattle() const {
  if (isEgg() || ceremony != CER_NONE || sleeping) return false;
  if (energy < BATTLE_ENERGY_MIN) return false;
  uint16_t tope = maxHp();
  if (!tope) return false;
  return (uint32_t)hpLeft * 100 >= (uint32_t)tope * BATTLE_HP_FLOOR_PCT;
}

bool Pet::canTrain() const {
  if (isEgg() || ceremony != CER_NONE || sleeping) return false;
  return energy >= TRAIN_ENERGY_MIN;
}

// Dejar un boton muerto sin decir por que es de las cosas que mas confunden.
const char *Pet::whyNotBattle() const {
  if (isEgg()) return "still an egg";
  if (ceremony != CER_NONE) return "not right now";
  if (sleeping) return "fast asleep";
  if (energy < BATTLE_ENERGY_MIN) return "too tired";
  uint16_t tope = maxHp();
  if (tope && (uint32_t)hpLeft * 100 < (uint32_t)tope * BATTLE_HP_FLOOR_PCT)
    return "too hurt";
  return "";
}

void Pet::afterBattle(uint16_t hpQueda) {
  hpSync();                                  // deja hpSeen al dia
  uint16_t tope = maxHp();
  hpLeft = hpQueda > tope ? tope : hpQueda;
  // Pelear cansa aunque se gane sin un rasguno: si no, se puede moler sin fin
  // y el combate deja de tener sitio en el dia del bicho.
  energy = dropTo(energy, BATTLE_ENERGY_COST, 0);
  if (hpLeft == 0) hpFaint();                // hpFaint ya guarda
  else { battleSave(); save(); }
}

bool Pet::ppEmpty() const {
  for (uint8_t i = 0; i < 4; i++)
    if (ppMove[i] && ppLeft[i]) return false;
  return true;
}

void Pet::registerSpecies(int16_t dex, bool esShiny, uint8_t forma) {
  if (dex < 1 || dex > DEX_COUNT) return;
  dexReg[(dex - 1) >> 3] |= (1 << ((dex - 1) & 7));
  // `esShiny` es de QUIEN SE REGISTRA. Aqui ponia `if (shiny)`, que es el
  // del propio Pet: con una mascota shiny, cada bicho que capturabas se
  // marcaba como shiny en la Pokedex y salia con la estrella y con los
  // colores raros sin haber visto uno en tu vida.
  if (esShiny) dexShinyReg[(dex - 1) >> 3] |= (1 << ((dex - 1) & 7));
  int16_t fila = formRow(dex, forma);
  if (fila >= 0 && fila < FORM_COUNT) dexFormReg[fila >> 3] |= (1 << (fila & 7));
}

// Leva 72: o VISTO. Chamam-na a cada quadro do combate, entao so faz algo
// na primeira vez. E nao grava ja: gravar na flash congela a placa quase um
// segundo, e isto cai no meio da luta. Marca o pendente e o loop grava no
// proximo momento parado, como o autoguardado.
void Pet::markSeen(int16_t dex) {
  if (dex < 1 || dex > DEX_COUNT || isSeen(dex)) return;
  dexSeen[(dex - 1) >> 3] |= (1 << ((dex - 1) & 7));
  pendingSave = true;
}

// Uma batalha entre placas acabou (ou alguem desistiu): soma e grava.
void Pet::contaDuelo(uint8_t r) {
  uint16_t *c = r == 0 ? &duV : r == 1 ? &duD : r == 2 ? &duE : nullptr;
  if (!c) return;
  if (*c < 0xFFFF) (*c)++;
  save();
}

void Pet::registerMega(int16_t fila) {
  if (fila < 0 || fila >= MEGA_COUNT) return;
  if (isMegaRegistered(fila)) return;        // ya estaba: no se graba de nuevo
  dexMegaReg[fila >> 3] |= (1 << (fila & 7));
  // Y el de BASE, que es bajo cuyo numero se mira en la Pokedex. Sin
  // esto, ver la mega de un bicho que no conoces dejaba una version
  // colgando de una ficha que seguia diciendo "???".
  registerSpecies(MEGA_TBL[fila].base, false);
  save();
}

// la racha y el vinculo mejoran el sorteo del huevo (0..~14)
int Pet::careBonus() const {
  int s = streak > 30 ? 30 : streak;
  // Del vinculo cuenta lo que TU has construido, no lo que traia de nacimiento
  // (BOND_AT_BIRTH). Con el suelo contando, cada bicho regalaba 2 puntos de
  // bonus por existir. La escala sigue siendo 0..4: se reparte sobre la franja
  // que de verdad se puede ganar.
  int ganado = bond > BOND_AT_BIRTH ? bond - BOND_AT_BIRTH : 0;
  return s / 3 + ganado * 4 / (100 - BOND_AT_BIRTH);
}

// primer cuidado del dia: avanza la racha y afianza el vinculo
void Pet::registerCare() {
  if (isEgg() || ceremony != CER_NONE) return;
  uint32_t d = today();
  if (d == 0 || d == lastCareDay) return;  // sin reloj, o ya conto hoy
  if (lastCareDay == 0 || d == lastCareDay + 1) {
    streak++;
  } else {
    streak = 1;        // hubo un hueco de dias
    lastMilestone = 0;
  }
  lastCareDay = d;
  bondToday = 0;
  if (streak > bestStreak) bestStreak = streak;
  bond = clamp100(bond + 4);
  // Un pelin de experiencia por el primer cuidado del dia. Es poco a
  // proposito: lo que sube de nivel es pelear. Pero sin esto, quien solo
  // quiere criar al bicho no ve una evolucion en su vida.
  gainExp(EXP_PER_CARE_DAY);
  // Y la bola del dia. Es el grifo lento pero seguro: aunque no pelees ni
  // entrenes, cuidando acabas teniendo con que capturar. No hay tienda a
  // proposito; las bolas salen de cuidar, que es de lo que va esto.
  dailyBalls();
  uint16_t ms = (streak >= 100) ? 100 : (streak >= 30) ? 30
              : (streak >= 7)   ? 7   : (streak >= 3)  ? 3 : 0;
  if (ms > lastMilestone) {
    lastMilestone = ms;
    milestoneUntil = millis() + 4500;
  }
  checkMedals();
  save();
}

void Pet::addBond(uint8_t amt) {
  // Luxury Ball: cada subida vale um a mais, e o teto do dia e maior.
  bool luxo = ballId && ballId < ITEM_COUNT && ITEM_TBL[ballId].fx == IT_BALLX &&
              ITEM_TBL[ballId].arg == BALLK_LUXURY;
  uint8_t teto = luxo ? BOND_DAY_CAP_LUXURY : BOND_DAY_CAP;
  if (bondToday >= teto) return;  // tope diario: el vinculo no se farmea
  if (luxo && amt) amt++;
  bond = clamp100(bond + amt);
  bondToday += amt;
}

void Pet::checkMedals() {
  // Los del BICHO piden que haya bicho; los del JUGADOR (la Pokedex, los
  // combates, las regiones) cuentan igual con un huevo en el nido, que
  // es justo cuando se miran las paginas.
  uint32_t before = medals;
  jugadorMedals();
  if (isEgg()) { premiaMedals(before); return; }
  if (level() >= 10) medals |= MED_LV10;
  if (level() >= 25) medals |= MED_LV25;
  if (level() >= 50) medals |= MED_LV50;
  if (berryKnown) medals |= MED_BERRY;
  if (streak >= 7) medals |= MED_STREAK7;
  if (bond >= 100) medals |= MED_BOND;
  // HABER LLEGADO a la forma final, no nacer en ella. Un Tauros, un
  // Lapras o un Skarmory no evolucionan nunca: antes recibian esta
  // medalla al nivel 1 por existir. Ahora pide al menos una evolucion, y
  // los que no tienen linea evolutiva simplemente no la tienen —— que es
  // honesto: no han llegado a ningun sitio.
  if (DEX_TBL[speciesId].evolvesTo == 0 && evoCount > 0) medals |= MED_FINAL;
  // Y "en forma" pide haber estado gordo y haberlo bajado. weight == 0 es
  // el peso con el que NACE, asi que la condicion de antes se cumplia
  // sola: bastaba llegar al nivel 5 sin fallos.
  if (wasFat && weight == 0 && level() >= 5 && careMistakes == 0)
    medals |= MED_FIT;
  premiaMedals(before);
}

// Los logros del JUGADOR: no dependen del bicho que tengas ahora, sino
// de lo que llevas hecho. Por eso se miran tambien con un huevo.
void Pet::jugadorMedals() {
  if (streak >= 30) medals |= MED_STREAK30;
  uint16_t reg = registeredCount();
  if (reg >= 50) medals |= MED_DEX50;
  if (reg >= 150) medals |= MED_DEX150;
  if (reg >= 500) medals |= MED_DEX500;
  if (caught >= 100) medals |= MED_CAUGHT100;
  if (wins >= 100) medals |= MED_WINS100;
  if (wins >= 500) medals |= MED_WINS500;
  // Un shiny EN LA POKEDEX: tenerlo o haberlo visto, que es lo que la
  // ficha ya ensena con su estrella.
  for (uint16_t i = 0; i < sizeof(dexShinyReg); i++)
    if (dexShinyReg[i]) { medals |= MED_SHINY; break; }
  uint8_t megas = 0;
  for (uint16_t i = 0; i < MEGA_COUNT; i++)
    if (isMegaRegistered((int16_t)i)) megas++;
  if (megas >= 1) medals |= MED_MEGA1;
  if (megas >= 10) medals |= MED_MEGA10;
  uint8_t enEquipo = 0;
  for (uint8_t i = 0; i < TEAM_SLOTS; i++)
    if (i == mainSlot || team[i].used()) enEquipo++;
  if (enEquipo >= TEAM_SLOTS) medals |= MED_TEAM6;
  // Y una por REGION terminada. `regionDone` mira badgesIn, asi que
  // vale para las que dejaste atras y no solo para la de ahora.
  for (uint8_t r = 0; r < REGION_COUNT; r++)
    if (regionDone(r)) medals |= MED_REGION(r);
}

// Lo que hay que hacer cuando alguno acaba de caer: contarlo, celebrarlo
// y pagarlo. Esta aparte porque ahora hay dos caminos (con bicho y con
// huevo) y duplicarlo era la forma de que uno de los dos se olvidara.
void Pet::premiaMedals(uint32_t before) {
  uint32_t gained = medals & ~before;
  if (gained) {
    for (uint32_t m = gained; m; m &= (m - 1)) totalMedals++;
    newMedal = gained;
    medalUntil = millis() + 4000;
    if (!sleeping) sfxPlay(SFX_MEDAL);
    save();
  }
  payMedals();
}

// El PREMIO de cada medalla. Se paga aparte de ganarla porque el premio
// se anadio despues: las medallas que el jugador ya tenia cobran la
// primera vez que pasen por aqui, y `medalPaid` impide que vuelvan a
// cobrar en el siguiente arranque.
//
// Devuelve el ultimo objeto entregado (0 si ninguno), que es lo que el
// cartel de la pantalla ensena.
uint8_t Pet::payMedals() {
  uint32_t debe = medals & ~medalPaid;
  if (!debe) return 0;
  uint8_t ultimo = 0;
  for (int i = 0; i < MED_COUNT; i++) {
    if (!(debe & (1 << i))) continue;
    uint8_t it = MEDAL_PRIZE[i];
    // Se marca como pagada AUNQUE la bolsa este llena (99 de ese objeto):
    // si no, volveria a intentarlo en cada tick y el cartel se quedaria
    // clavado para siempre.
    medalPaid |= (1u << i);
    if (it && takeItem(it)) ultimo = it;
  }
  save();
  return ultimo;
}

void Pet::rename(const char *name) {
  strncpy(nick, name, sizeof(nick) - 1);
  nick[sizeof(nick) - 1] = 0;
  save();
}

// statAtLevel vive en dex.h: esta copia local y la de battle.cpp eran la
// misma cuenta escrita dos veces, que es como acaban diciendo cosas
// distintas. Lo que la ficha enseña tiene que ser lo que pelea.

uint16_t Pet::atkStat() const {
  return isEgg() ? 0 : statAtLevel(DEX_TBL[speciesId].bAtk, geneAtk, level(), trAtk);
}
uint16_t Pet::defStat() const {
  return isEgg() ? 0 : statAtLevel(DEX_TBL[speciesId].bDef, geneDef, level(), trDef);
}
uint16_t Pet::speStat() const {
  return isEgg() ? 0 : statAtLevel(DEX_TBL[speciesId].bSpe, geneSpe, level(), trSpe);
}
// Los especiales comparten genes y entrenamiento con su pareja fisica: el
// Los especiales YA tienen genes y EVs propios. Compartian con su pareja
// fisica para no gastar cuatro bytes en la NVS, pero eso hacia imposible
// criar a un bicho especial y dejaba a Proteina y Calcio haciendo lo mismo.
uint16_t Pet::spAtkStat() const {
  return isEgg() ? 0 : statAtLevel(DEX_TBL[speciesId].bSpA, geneSpa, level(), trSpa);
}
uint16_t Pet::spDefStat() const {
  return isEgg() ? 0 : statAtLevel(DEX_TBL[speciesId].bSpD, geneSpd, level(), trSpd);
}
// Los PS llevan su propia cuenta, distinta de la de los otros stats: la base
// pesa el doble y hay un suelo, como en los juegos. Vive en dex.h (hpAtLevel)
// porque la hacian CUATRO sitios y ya se habian despegado entre ellos.
uint16_t Pet::maxHp() const {
  if (isEgg()) return 0;
  return hpAtLevelForm(speciesId, level(), trHp, form);
}

uint16_t Pet::registeredCount() const {
  uint16_t n = 0;
  for (int i = 1; i <= DEX_COUNT; i++)
    if (isRegistered(i)) n++;
  return n;
}

// forma final que ya cumplio su ciclo (7 dias): lista para despedirse. La
// despedida la dispara el usuario con el boton (no salta sola, para que la vea)
bool Pet::canFarewellNow() const {
  return !isEgg() && !sleeping && ceremony == CER_NONE &&
         DEX_TBL[speciesId].evolvesTo == 0 && ageMinutes >= FAREWELL_AGE_MIN &&
         // y un dia DESDE que es forma final: si no, evolucionar y que te
         // pidan despedirte pasaban en el mismo instante (ver
         // FAREWELL_SETTLE).
         ageMinutes >= finalAge + FAREWELL_SETTLE &&
         bond >= FAREWELL_BOND_MIN;   // atrito: un bicho descuidado no lega
}

// Cuanto vale lo que deja. Cuatro cosas, y las cuatro son cosas que el
// jugador HIZO: hasta donde lo subio, si lo cuido bien, si se encarino y
// cuanto avanzo con el. Techo bajo a proposito (LEGACY_SCORE_MAX): la
// herencia tiene que notarse, no decidir la partida.
uint8_t Pet::legacyScore() const {
  if (isEgg()) return 0;
  int s = level() / 20;                  // 0..5
  if (bond >= 80) s++;
  if (careMistakes == 0) s++;            // criado sin un solo descuido
  s += badges / 4;                       // 0..3
  return (uint8_t)(s > LEGACY_SCORE_MAX ? LEGACY_SCORE_MAX : s);
}

// abandono total durante 1h: lista para escaparse. La dispara el usuario con el
// boton (final triste); cuidarla un solo tick la salva (neglectTicks se resetea)
bool Pet::canRunawayNow() const {
  return !isEgg() && !sleeping && ceremony == CER_NONE && neglectTicks >= RUNAWAY_TICKS;
}

void Pet::startFarewell() {
  if (isEgg() || ceremony != CER_NONE) return;
  // Deja un huevo de SU especie con lo que ha ganado. Se apunta aqui, antes
  // de que newEgg() lo borre todo.
  uint8_t s = legacyScore();
  // El huevo es de la forma BASE de su linea: un Charizard deja un
  // Charmander. Poner la forma final dentro del huevo regalaba un bicho
  // acabado de nivel 1 y se saltaba entero el juego de criarlo.
  legacyDex = baseFormOf(speciesId);
  legacyFloor = (uint8_t)(LEGACY_FLOOR_BASE + s * 2);
  if (legacyFloor > LEGACY_FLOOR_MAX) legacyFloor = LEGACY_FLOOR_MAX;
  legacyItem = heldItem;               // lo que llevaba puesto se lo pasa
  legacyMove = 0;
  // Un golpe que al recien nacido no le tocaria todavia: se coge de lo que
  // el difunto sabia POR ENCIMA del nivel 1. Es el "golpe de huevo" y hace
  // falta haberlo criado de verdad (score 2) para que lo pase.
  if (s >= 2) {
    uint16_t todos[40], alNacer[8];
    uint8_t n = movesKnown(speciesId, level(), todos, 40);
    // OJO: dos buffers. movesKnown ESCRIBE en el que le pasas, asi que
    // reutilizar el mismo pisaria la lista larga con la corta.
    // Y se compara con lo que sabra EL BEBE al nacer (legacyDex), no el que
    // se va: de otro modo se podria "heredar" un golpe que la cria ya trae.
    uint8_t cuna = movesKnown(legacyDex, 1, alNacer, 8);
    for (uint8_t i = 0; i < n; i++) {
      bool yaLoSabe = false;
      for (uint8_t k = 0; k < cuna; k++) if (alNacer[k] == todos[i]) yaLoSabe = true;
      if (yaLoSabe) { todos[i] = todos[--n]; i--; }
    }
    if (n) legacyMove = todos[random(n)];
  }
  lastEnd = CER_FAREWELL;
  ceremony = CER_FAREWELL;
  ceremonyUntil = millis() + CEREMONY_MS;
  heartUntil = ceremonyUntil;  // corazones durante toda la despedida
  sfxPlay(SFX_BYE);
  save();
}

void Pet::startRunaway() {
  if (isEgg() || ceremony != CER_NONE) return;
  lastEnd = CER_RUNAWAY;
  ceremony = CER_RUNAWAY;
  ceremonyUntil = millis() + CEREMONY_MS;
  sfxPlay(SFX_BYE);
  save();
}

void Pet::release() {
  if (isEgg() || ceremony != CER_NONE) return;
  lastEnd = CER_RELEASE;
  ceremony = CER_RELEASE;
  ceremonyUntil = millis() + CEREMONY_MS;
  heartUntil = ceremonyUntil;
  sfxPlay(SFX_BYE);
  save();
}

void Pet::hatch() {
  speciesId = eggTarget;
  shiny = eggShiny;
  // Genes del individuo: del SUELO al 110 por stat, y cada crianza unica. De
  // serie el suelo es 90; un huevo de despedida lo trae mas alto (nunca por
  // encima de LEGACY_FLOOR_MAX). Sube el suelo y no el techo a proposito:
  // asi un linaje cuidado nace bueno pero no se escapa de la escala.
  uint8_t piso = legacyDex > 0 ? legacyFloor : LEGACY_FLOOR_BASE;
  if (piso > LEGACY_FLOOR_MAX) piso = LEGACY_FLOOR_MAX;
  uint8_t rango = (uint8_t)(111 - piso);
  geneHp = (uint8_t)(piso + random(rango));
  geneAtk = (uint8_t)(piso + random(rango));
  geneDef = (uint8_t)(piso + random(rango));
  geneSpa = (uint8_t)(piso + random(rango));
  geneSpd = (uint8_t)(piso + random(rango));
  geneSpe = (uint8_t)(piso + random(rango));
  trHp = trAtk = trDef = trSpa = trSpd = trSpe = 0;
  berryKnown = false;
  // Vinculo, medallas y nombre son del individuo. El vinculo no arranca a
  // cero: nace confiando en ti (ver BOND_AT_BIRTH) y es lo que hagas despues
  // lo que lo sube o lo baja.
  bond = BOND_AT_BIRTH;
  bondToday = 0;
  medals = 0;
  newMedal = 0;
  // Y el reloj del cartel con ella: si se queda corriendo apunta a una
  // medalla que ya no existe, y el cartel se dibuja sin nada que decir.
  medalUntil = 0;
  medalPaid = 0;
  evoCount = 0;
  finalAge = 0;
  wasFat = false;
  nick[0] = 0;
  ballId = 0;            // nasceu aqui: nao veio de bola nenhuma
  // Nace entero y con los golpes cargados. Sin esto se quedaba con hpLeft=0
  // desde el huevo y no podia pelear nada mas nacer.
  hpSeen = 0;
  hpSync();
  ppSync();
  rollAbility();
  rollGender();
  // La forma sale de DONDE estas: un Vulpix que nace en Alola es de
  // Alola. La tirada es por Hisui, que no es una region del juego y sale
  // de vez en cuando en Sinnoh en vez de sustituir al bicho normal.
  form = formForRegion(speciesId, region, (uint8_t)random(100));
  // Y el resto de la herencia: el golpe que no le tocaria todavia y lo que
  // llevaba puesto su antecesor. El golpe entra DESPUES de ppSync(), en el
  // ultimo hueco, para no pisar los que le tocan por nivel.
  if (legacyDex > 0) {
    if (legacyMove) {
      ppMove[3] = legacyMove;
      ppLeft[3] = movePP(legacyMove);
      movesPicked = true;   // se lo dieron: que ppSync no se lo quite
    }
    if (legacyItem) { heldItem = legacyItem; bag[legacyItem]++; }
    legacyDex = 0;          // la herencia se gasta al nacer
    legacyMove = 0;
    legacyItem = 0;
    legacyFloor = LEGACY_FLOOR_BASE;
  }
  registerSpecies(speciesId, shiny, form);  // criado = registrado
  checkMedals();     // por si nace ya en forma final (legendario)
  sfxPlay(SFX_HATCH);
  save();
}

// ¿se dan ya las condiciones para evolucionar? Cada descuido retrasa la
// evolucion 1 nivel, y ademas tiene que estar bien cuidado en ese momento
// (ninguna estadistica por debajo de 40). NO evoluciona sola: la dispara el
// usuario tocando al bicho (evolve()), para que vea la transformacion.
// Los del banquillo tambien crecen. Antes NO evolucionaban nunca: ganaban
// experiencia (teamShareExp) y subian de nivel, pero al llegar al nivel de
// su evolucion no pasaba nada, para siempre. Un Weedle metido en el equipo
// se quedaba Weedle aunque llegara al 50.
//
// La dispara el JUGADOR con el boton de la pagina de equipo, igual que la
// del principal. Automatica tenia un agujero feo: solo se miraba AL SUBIR
// de nivel, asi que uno que ya estuviera pasado de nivel —— por ejemplo
// porque la version anterior no evolucionaba —— se quedaba atascado hasta
// el siguiente nivel. Con boton, si le toca, se ve que le toca.
//
// Las condiciones son las del principal MENOS las que son suyas y solo
// suyas: los descuidos y el stat mas bajo describen a quien ESTAS criando,
// no a uno que espera en el banquillo. Quedan el nivel, que no sea de los
// que solo evolucionan con objeto, y la Piedra Eterna — que si la lleva
// puesta es porque alguien se la puso para esto.
bool Pet::canEvolveReserve(uint8_t slot) const {
  if (slot >= TEAM_SLOTS || slot == mainSlot) return false;
  const StoredMon &m = team[slot];
  if (!m.used()) return false;
  if (itemAt(m.item).fx == IT_EVERSTONE) return false;
  const DexEntry &d = DEX_TBL[m.species];
  if (d.evolvesTo == 0 || onlyByItem(m.species)) return false;
  return levelOf(m) >= (uint16_t)d.evolveLevel;
}

uint8_t Pet::reserveReadyToEvolve() const {
  for (uint8_t i = 0; i < TEAM_SLOTS; i++)
    if (canEvolveReserve(i)) return i;
  return 0xFF;
}

bool Pet::evolveReserve(uint8_t slot) {
  if (!canEvolveReserve(slot)) return false;
  StoredMon &m = team[slot];
  const DexEntry &d = DEX_TBL[m.species];
  teamEvoFrom = m.species;
  m.form = formAfterEvolve(d.evolvesTo, m.form);
  m.species = d.evolvesTo;
  teamEvoTo = m.species;
  // Na Pokedex, como a evolucao do titular. Faltava: o Torchic do usuario
  // evoluiu no banco de reservas e o Combusken ficou sem ficha (2026-11-09).
  registerSpecies(m.species, (m.flags & SM_SHINY) != 0, m.form);
  // Los PS suben con la especie; se le dejan llenos, que es lo que hace
  // el juego al evolucionar y ademas evita dejarlo con mas PS que su tope.
  m.hpLeft = hpAtLevelForm(m.species, levelOf(m), m.trHp, m.form);
  Serial.printf("el suplente %s evoluciono a %s\n",
                dexName(teamEvoFrom), dexName(teamEvoTo));
  return true;
}

// ---------------------------------------------------------------- trocas

#define TROCA_DIARIO "/troca/diario.bin"

bool Pet::trocaGravaDiario(const TrocaDiario &d) {
  SD_MMC.mkdir("/troca");
  // FILE_WRITE anexa se o arquivo existe: apaga antes, como nas caixas.
  if (SD_MMC.exists(TROCA_DIARIO)) SD_MMC.remove(TROCA_DIARIO);
  File f = SD_MMC.open(TROCA_DIARIO, FILE_WRITE);
  if (!f) return false;
  bool ok = f.write((const uint8_t *)&d, sizeof(d)) == sizeof(d);
  f.close();
  // Relido: "gravou" so vale se o que esta no cartao e o que se quis por.
  TrocaDiario r;
  return ok && trocaLeDiario(r) && memcmp(&r, &d, sizeof(d)) == 0;
}

bool Pet::trocaLeDiario(TrocaDiario &d) {
  File f = SD_MMC.open(TROCA_DIARIO, FILE_READ);
  if (!f) return false;
  bool ok = f.read((uint8_t *)&d, sizeof(d)) == (int)sizeof(d);
  f.close();
  return ok && trocaDiarioValido(d);
}

const StoredMon *Pet::trocaMeu(uint8_t slot) const {
  if (slot & TROCA_BANCO) {
    uint8_t i = (uint8_t)(slot & 0x7F);
    return i < BANK_SLOTS ? &bank[i] : nullptr;
  }
  return slot < TEAM_SLOTS ? &team[slot] : nullptr;
}

bool Pet::trocaPodeOferecer(uint8_t slot) const {
  const StoredMon *m = trocaMeu(slot);
  if (!m || !m->used()) return false;
  return (slot & TROCA_BANCO) || slot != mainSlot;
}

StoredMon Pet::trocaChegada(const StoredMon &veio) {
  StoredMon m = veio;
  if (!m.used() || m.species > DEX_COUNT) return m;
  const ItemEntry &it = itemAt(m.item);
  if (it.fx == IT_EVERSTONE) return m;   // a Piedra Eterna manda aqui tambem
  const bool hembra = (m.flags & SM_FEMALE) != 0;
  const uint8_t arg = (m.item && it.fx == IT_STONE) ? it.arg : 0xFF;
  int16_t to = evoTrocaTarget(m.species, arg, hembra);
  if (!to) return m;
  // Se foi pelo objeto (Onix + Metal Coat), ele se gasta, como no jogo.
  for (uint8_t i = 0; i < EVO_ITEM_COUNT; i++)
    if (EVO_ITEM[i].from == m.species && EVO_ITEM[i].troca == 2 &&
        EVO_ITEM[i].item == arg && EVO_ITEM[i].to == to) {
      m.item = 0;
      break;
    }
  m.form = formAfterEvolve(to, m.form);
  m.species = to;
  m.hpLeft = hpAtLevelForm(m.species, levelOf(m), m.trHp, m.form);
  return m;
}

bool Pet::trocaPreparar(TrocaDiario &d) {
  // O meu ainda e o que eu ofereci? (a tela de troca e modal, mas e barato
  // conferir, e um diario com o bicho errado seria pior que nao trocar)
  if (!trocaPodeOferecer(d.slot)) return false;
  if (memcmp(trocaMeu(d.slot), d.sai, TROCA_MON) != 0) return false;
  // do banco: QUAL caixa (se reiniciar com outra aberta, o aplicar volta a ela)
  d.rsv = (d.slot & TROCA_BANCO) ? boxOpen : 0;
  StoredMon veio;
  memcpy(&veio, d.veio, TROCA_MON);
  if (!veio.used() || veio.species > DEX_COUNT) return false;
  StoredMon chega = trocaChegada(veio);
  memcpy(d.chega, &chega, TROCA_MON);
  d.estado = TD_PREPARADO;
  trocaSelaDiario(d);
  return trocaGravaDiario(d);
}

bool Pet::trocaAplicar(const TrocaDiario &d) {
  if (!trocaDiarioValido(d)) return false;
  if (trocaUltima != d.sessao) {
    StoredMon chega;
    memcpy(&chega, d.chega, TROCA_MON);
    // Tira o meu. Primeiro onde estava; se o jogador mexeu entre o diario
    // e agora (reiniciou com a troca pendente), procura no mesmo lugar
    // (time ou a caixa de onde saiu).
    const bool doBanco = (d.slot & TROCA_BANCO) != 0;
    StoredMon *lugar = doBanco ? bank : team;
    const uint8_t n = doBanco ? BANK_SLOTS : TEAM_SLOTS;
    // reiniciou com outra caixa aberta: volta a de onde saiu (se a SD
    // nao deixar, nao aplica agora; o diario segue e tenta de novo)
    if (doBanco && d.rsv != boxOpen && !boxGo(d.rsv)) return false;
    int8_t onde = -1;
    const uint8_t s0 = (uint8_t)(d.slot & 0x7F);
    if (s0 < n && memcmp(&lugar[s0], d.sai, TROCA_MON) == 0) onde = (int8_t)s0;
    for (uint8_t i = 0; onde < 0 && i < n; i++)
      if ((doBanco || i != mainSlot) && memcmp(&lugar[i], d.sai, TROCA_MON) == 0)
        onde = (int8_t)i;
    if (onde >= 0) lugar[onde] = StoredMon();
    // Do banco, quem chega fica NO MESMO hueco da caixa (la o cadeado vale
    // sozinho). Do time, a trava de nivel: acima do teto daqui vai para o
    // banco (com o cadeado), como qualquer outro; senao, no lugar do meu.
    const bool acima = levelOf(chega) > bankCapHere();
    bool posto = false;
    if (doBanco) {
      if (onde >= 0) {
        lugar[onde] = chega;
        posto = true;
      }
    } else {
      if (acima) posto = addToBank(chega);
      if (!posto && onde >= 0 && !acima) {
        team[onde] = chega;
        posto = true;
      }
    }
    // (o acima do teto NUNCA entra no time, nem como ultimo recurso)
    if (!posto) posto = (!acima && addToTeam(chega)) || addToBank(chega);
    if (!posto) {
      // sem lugar nenhum: devolve o meu e nao aplica (o diario segue)
      if (onde >= 0) memcpy(&lugar[onde], d.sai, TROCA_MON);
      return false;
    }
    registerSpecies(chega.species, (chega.flags & SM_SHINY) != 0, chega.form);
    // Se evoluiu, a Pokedex tambem viu o de antes.
    StoredMon veio;
    memcpy(&veio, d.veio, TROCA_MON);
    if (veio.species != chega.species)
      registerSpecies(veio.species, (veio.flags & SM_SHINY) != 0, veio.form);
    trocaUltima = d.sessao;
    save();
  }
  TrocaDiario a = d;
  a.estado = TD_APLICADO;
  trocaSelaDiario(a);
  trocaGravaDiario(a);   // se falhar, trocaUltima ja impede reaplicar
  return true;
}

void Pet::trocaDescartar(const TrocaDiario &d) {
  TrocaDiario a = d;
  a.estado = TD_DESCARTADO;
  trocaSelaDiario(a);
  trocaGravaDiario(a);
}

bool Pet::canEvolveNow() const {
  if (isEgg() || sleeping || ceremony != CER_NONE) return false;
  // Piedra Eterna: mientras la lleve encima, no evoluciona. Aqui vale doble,
  // porque el bicho vive tres dias y quedarse con la forma que te gusta es
  // una decision de verdad.
  if (itemAt(heldItem).fx == IT_EVERSTONE) return false;
  const DexEntry &d = DEX_TBL[speciesId];
  if (d.evolvesTo == 0) return false;
  // Quien SOLO evoluciona con objeto no sube por nivel: si no, el objeto no
  // serviria para nada porque el bicho ya se habria transformado solo.
  //
  // Pero "le sirve un objeto" no es lo mismo que "solo con objeto", y antes
  // se trataban igual: el Anil le da a Onix DOS caminos (nivel 42 y Metal
  // Coat) y bloquearlo le quitaba el de nivel. Lo mismo con Scyther (45),
  // Seadra (50) y Porygon (30).
  if (onlyByItem(speciesId)) return false;
  // LLEGO AL NIVEL, EVOLUCIONA. Y ya.
  //
  // Aqui habia dos portones mas, los dos invisibles: se sumaba `careMistakes`
  // al nivel —— con ocho descuidos un Bulbasaur pedia el 24 —— y ademas se
  // exigia `lowestStat() >= 40`. Resultado: llegabas al 16, no pasaba nada,
  // y la pantalla no decia por que.
  //
  // Los descuidos siguen contando donde les toca: en la calidad del huevo
  // que deja (legacyScore) y en la medalla de criarlo sin un fallo. Eso es
  // lo que son —— el futuro ——, no un peaje sobre lo que ya te ganaste.
  return level() >= (uint16_t)d.evolveLevel;
}

// El paso comun: transformarse en `next` y lanzar la animacion. Lo usan la
// evolucion por nivel y la de piedra, que solo se diferencian en COMO se
// decide el destino y en que requisitos piden.
void Pet::becomeSpecies(int16_t next) {
  if (next < 1 || next > DEX_COUNT) return;
  prevSpeciesId = speciesId;
  // La FORMA se conserva si el destino la tiene: un Vulpix de Alola da
  // un Ninetales de Alola. Si no la tiene, vuelve a la normal — que es
  // lo que pasa, por ejemplo, con un Pikachu de forma suelta.
  form = formAfterEvolve(next, form);
  speciesId = next;
  // Cuenta la evolucion y fecha la forma final. Va AQUI porque este es
  // "el paso comun de las dos vias": subir de nivel (evolve) y la PIEDRA
  // (useStone). Lo tuve primero en evolve() y la piedra se quedaba sin
  // contar —— ni medalla de forma final, ni el dia de margen antes de que
  // te pidan despedirte. El comando SPEC de la consola no entra por aqui
  // (hace `speciesId = n` a pelo), que es lo que hace que esto sea el
  // sitio correcto y no uno demasiado ancho.
  if (evoCount < 255) evoCount++;
  if (DEX_TBL[speciesId].evolvesTo == 0) finalAge = ageMinutes;
  // Los PS, DESPUES de cambiar de especie. Faltaba, y no era inofensivo:
  // hpSync() es quien suma la diferencia cuando el maximo sube (asi
  // evolucionar no te deja pareciendo tocado) y quien lo RECORTA cuando
  // baja. Y baja de verdad, sin tocar ningun comando de depuracion:
  // Tyrogue (55) da Hitmontop (50), Mime Jr. (60) da Mr. Mime (50) y
  // Nincada (31) deja un Shedinja (1). Sin esto el bicho se quedaba con
  // mas vida que su maximo, la barra se salia de su caja y el umbral de
  // "demasiado tocado para pelear" contaba sobre un numero imposible.
  //
  // Lo encontro la bateria de interfaz pasando los guiones del
  // simulador: ninguno de los 541 tests de logica miraba este caso.
  hpSync();
  registerSpecies(speciesId, shiny, form);
  sfxPlay(SFX_EVOLVE);
  evolveUntil = millis() + EVOLVE_ANIM_MS;
  save();
}

bool Pet::useStone(uint8_t itemId) {
  if (isEgg() || sleeping || ceremony != CER_NONE) return false;
  const ItemEntry &it = itemAt(itemId);
  if (it.fx != IT_STONE || bag[itemId] == 0) return false;
  int16_t next = stoneTarget(speciesId, it.arg, female);
  if (!next) return false;             // esa piedra no le hace nada (o no a el:
                                       // la Alba solo da Froslass a una hembra)
  // La Piedra Eterna manda por encima de todo, igual que con el nivel.
  if (itemAt(heldItem).fx == IT_EVERSTONE) return false;
  // La piedra se salta el NIVEL, que es justo el atajo que compras. Lo que
  // NO se salta es el cuidado: un bicho hecho polvo no evoluciona ni con
  // piedra, porque esa regla es de lo que va el juego.
  if (lowestStat() < 40) return false;
  bag[itemId]--;
  becomeSpecies(next);
  return true;
}

void Pet::evolve() {
  if (!canEvolveNow()) return;
  const DexEntry &d = DEX_TBL[speciesId];
  int16_t next = d.evolvesTo;
  if (speciesId == DEX_EEVEE) {
    // rama de Eevee: prefiere la evolucion que falte en la pokedex
    int16_t opts[EEVEE_BRANCHES];
    int n = 0;
    for (uint8_t i = 0; i < EEVEE_BRANCHES; i++)
      if (!isRegistered(EEVEE_BRANCH[i])) opts[n++] = EEVEE_BRANCH[i];
    next = n > 0 ? opts[random(n)]
                 : EEVEE_BRANCH[random((long)EEVEE_BRANCHES)];
  }
  // Y las que se parten por SEXO: el campo guarda el destino de la
  // hembra, asi que al macho hay que mandarlo al suyo. Hoy es solo
  // Burmy (hembra Wormadam, macho Mothim), que hasta ahora no tenia
  // forma de dar Mothim.
  if (!female) {
    int16_t m = dexMaleBranch(speciesId);
    if (m) next = m;
  }
  // Y las otras ramas de nivel (EVO_RAMA, sacada del PBS). Sin esto,
  // `evolvesTo` guardaba una y las demas no pasaban nunca: Hitmonlee,
  // Hitmonchan, Cascoon (y con el, Dustox) y Shedinja solo salian en la
  // hierba, que es taparlo, no resolverlo.
  int16_t extra = 0;
  for (uint8_t i = 0; i < EVO_RAMAS; i++) {
    const EvoRama &r = EVO_RAMA[i];
    if (r.from != speciesId) continue;
    switch (r.cond) {
      case RAMA_ATK_MAYOR:
        if (atkStat() > defStat()) next = r.to;
        break;
      case RAMA_DEF_MAYOR:
        if (defStat() > atkStat()) next = r.to;
        break;
      case RAMA_AZAR:
        // A suertes, pero UNA sola vez en la vida del bicho: la tirada
        // sale de sus genes, que no cambian nunca. Con random() aqui,
        // mirar la ficha y evolucionar podrian discrepar, y dos
        // partidas guardadas del mismo bicho darian cosas distintas.
        if ((geneAtk + geneSpe) & 1) next = r.to;
        break;
      case RAMA_EXTRA:
        // No sustituye: se SUMA. Nincada da Ninjask y ademas deja un
        // Shedinja, si queda hueco en el equipo — que es la version
        // aqui de la Poke Ball libre que pedian los juegos.
        extra = r.to;
        break;
      default:
        break;
    }
  }
  uint16_t nivel = level();
  becomeSpecies(next);
  if (extra) {
    // Por la misma puerta que una captura: va al equipo si hay hueco y
    // al banco si no, y si las dos estan llenas no aparece. Eso es
    // exactamente la regla de los juegos (hacia falta una Poke Ball
    // libre) y sale gratis.
    lastShedDex = (catchMon(extra, nivel, false, ABIL_OF[extra][0])
                   == CATCH_FULL) ? 0 : extra;
  }
}

void Pet::feed() {
  feedBerry(0);
}

void Pet::feedBerry(uint8_t color) {
  if (ceremony != CER_NONE) return;
  if (isEgg() || sleeping) return;
  if (lovesBerry(color)) {
    fullness = clamp100(fullness + 35);
    joy = clamp100(joy + 10);
    heartUntil = millis() + HEART_MS;  // "le encanta!"
    berryKnown = true;                 // descubierto: se muestra en la ficha
    addBond(2);
  } else {
    fullness = clamp100(fullness + 25);
  }
  // Las bayas tambien reponen PP: es el centro Pokemon de bolsillo, para no
  // tener que echar a dormir al bicho entre un combate y el siguiente. La
  // favorita repone mas, que para eso es la favorita.
  ppRestore(lovesBerry(color) ? 5 : 3);
  hpHeal((uint16_t)((uint32_t)maxHp() * HP_HEAL_BERRY_PCT *
                    (lovesBerry(color) ? 2 : 1) / 100));
  eatUntil = millis() + EAT_ANIM_MS;
  registerCare();
  save();
}

void Pet::feedCandy() {
  if (ceremony != CER_NONE) return;
  if (isEgg() || sleeping) return;
  fullness = clamp100(fullness + 10);
  joy = clamp100(joy + 12);
  weight = clamp100(weight + 12);  // las chuches pasan factura
  if (weight >= WEIGHT_FAT) wasFat = true;   // para MED_FIT: ver pet.h
  eatUntil = millis() + EAT_ANIM_MS;
  registerCare();
  save();
}

// Gimnasio: una sesion del saco de tipos. La puntuacion se convierte en EVs
// del stat que el jugador eligio, no en un stat suelto: es la traduccion del
// "entrenamiento" del tamagotchi al idioma de Pokemon.
uint8_t Pet::trainEV(uint8_t which, uint16_t score) {
  if (ceremony != CER_NONE || isEgg()) return 0;
  uint16_t gain = score / TRAIN_SCORE_PER_EV;
  if (gain > TRAIN_EV_PER_SESSION) gain = TRAIN_EV_PER_SESSION;
  // Lo que de verdad entro: con el stat o el total al tope cabe menos, y la
  // pantalla no debe cantar un +12 que no ha pasado.
  uint8_t entro = gainEV(which, (uint8_t)gain);
  energy = dropTo(energy, TRAIN_ENERGY_COST, 5);   // cansa
  fullness = dropTo(fullness, 5, 5);
  int burn = (int)weight - score / 2;  // tambien quema peso
  weight = burn > 0 ? burn : 0;
  joy = clamp100(joy + 6);
  if (score >= 10) heartUntil = millis() + HEART_MS;
  if (score > trainHi) trainHi = score;   // record del jugador
  addBond(2);
  // Y las bolas del gimnasio: el grifo RAPIDO. El diario es lento y seguro;
  // este paga segun lo bien que lo hagas, asi que jugar bien el saco de
  // tipos es lo que te deja salir a capturar hoy y no manana.
  //
  // OJO al orden: registerCare() puede repartir la bola del dia y pisar
  // lastBallId/lastBallCount, asi que va ANTES y el premio del gimnasio
  // despues, que es el que la pantalla del gimnasio va a contar.
  registerCare();
  trainBalls(score);
  save();
  return entro;
}

void Pet::play() {
  if (ceremony != CER_NONE) return;
  if (isEgg() || sleeping) return;
  joy = clamp100(joy + 25);
  energy = clamp100(energy - 10);
  fullness = clamp100(fullness - 5);
  heartUntil = millis() + HEART_MS;
  addBond(2);
  registerCare();
  save();
}

void Pet::toggleLight() {
  if (ceremony != CER_NONE) return;
  if (isEgg()) return;
  sleeping = !sleeping;
  save();
}

void Pet::clean() {
  if (ceremony != CER_NONE) return;
  poops = 0;
  hygiene = 100;
  addBond(1);
  registerCare();
  save();
}

// Un zarandeo fuerte. Sin esto menear seria gratis y el jugador
// sacudiria la consola todo el dia sin consecuencia; con esto, agitar es
// una decision. Durmiendo no se le asusta: ya esta quieto.
void Pet::scare() {
  if (isEgg() || sleeping || ceremony != CER_NONE) return;
  joy = (uint8_t)(joy > 4 ? joy - 4 : 0);
}

// Boca abajo = a la cama. Esto NO existia: hasta ahora solo se dormia al
// desmayarse, asi que el jugador no podia mandarle a descansar. Y dormir
// cura, asi que es una jugada de verdad y no un adorno.
bool Pet::restDown() {
  if (isEgg() || sleeping || ceremony != CER_NONE) return false;
  sleeping = true;
  restByGesture = true;
  save();
  return true;
}

// Dos tapas = arriba. Solo si lo acostaste TU: si se durmio por
// desmayo, dormir es como se cura y despertarle a golpecitos seria
// cortarle la cura sin querer.
bool Pet::wakeUp() {
  if (!sleeping || !restByGesture) return false;
  sleeping = false;
  restByGesture = false;
  save();
  return true;
}

// Andar con la consola encima. Quema peso como el minijuego pero NO
// gasta fuelle: el que se cansa eres tu. Los pasos llegan a puñados (el
// bucle principal los lee cada vuelta), asi que se cobra por tramos y no
// por paso, o cien pasos darian cien puntos de alegria.
void Pet::walk(uint16_t pasos) {
  if (isEgg() || sleeping || ceremony != CER_NONE || !pasos) return;
  stepsToday = (uint16_t)(stepsToday + pasos > 60000 ? 60000
                                                     : stepsToday + pasos);
  stepsPend = (uint16_t)(stepsPend + pasos);
  // Cada tramo de PASOS_TRAMO quema un punto de peso y da uno de alegria.
  while (stepsPend >= PASOS_TRAMO) {
    stepsPend = (uint16_t)(stepsPend - PASOS_TRAMO);
    if (weight > 0) weight--;
    joy = clamp100(joy + 1);
    registerCare();
  }
}

void Pet::caress() {
  if (ceremony != CER_NONE) return;
  if (isEgg() || sleeping) return;
  joy = clamp100(joy + 5);
  heartUntil = millis() + HEART_MS;
  addBond(1);
  registerCare();
}

void Pet::eggTap() {
  if (!isEgg()) return;
  if (++eggTaps >= 3) hatch();
  else save();
}

PetMood Pet::mood() const {
  if (sleeping) return MOOD_SLEEPING;
  if (eating()) return MOOD_EATING;
  if (lowestStat() < 25) return MOOD_SAD;
  return MOOD_HAPPY;
}

// Pide lo peor que tenga. El huevo no pide nada; dormido tampoco (lo que
// necesita es dormir, y ya lo esta haciendo); durante una ceremonia se calla
// para no competir con ella. En empate manda el orden de urgencia real:
// el hambre cae sola y mas rapido, y la higiene tambien se hunde con cada
// caca, mientras que animo y energia dependen de que el jugador actue.
PetNeed Pet::need() const {
  if (isEgg() || sleeping || eating() || ceremony != CER_NONE) return NEED_NONE;
  uint8_t lo = lowestStat();
  if (lo >= NEED_WARN) return NEED_NONE;
  if (fullness == lo) return NEED_FOOD;
  if (hygiene == lo) return NEED_CLEAN;
  if (joy == lo) return NEED_PLAY;
  return NEED_ENERGY;
}

size_t Pet::guarda(const char *clave, const void *datos, size_t n) {
  size_t w = prefs.putBytes(clave, datos, n);
  if (w != n) {
    guardadoFallo = true;
    if (bloquesFallados < 255) bloquesFallados++;
    Serial.printf("[save] NO se guardo '%s': pedidos %u bytes, escritos %u\n",
                  clave, (unsigned)n, (unsigned)w);
  }
  return w;
}

void Pet::save() {
  ticksSinceSave = 0;
  pendingSave = false;
  // A sequencia DESTA gravacao: vai para a NVS ("svn", no fim) e para o
  // estado do SD. No boot, a maior diz quem gravou por ultimo.
  if (!seqSave) seqSave = prefs.getUInt("svn", 0);
  seqSave++;
  // Cada save empieza limpio: lo que importa es si ESTA gravacion fallo.
  guardadoFallo = false;
  bloquesFallados = 0;
  prefs.putUChar("full", fullness);
  prefs.putUChar("joy", joy);
  prefs.putUChar("ene", energy);
  prefs.putUChar("hyg", hygiene);
  prefs.putUChar("poop", poops);
  prefs.putUChar("wgt", weight);
  prefs.putUChar("gatk", geneAtk);
  prefs.putUChar("gdef", geneDef);
  prefs.putUChar("gspe", geneSpe);
  prefs.putUChar("tatk", trAtk);
  prefs.putUChar("tdef", trDef);
  prefs.putUChar("tspe", trSpe);
  prefs.putUChar("ghp", geneHp);
  prefs.putUChar("gspa", geneSpa);
  prefs.putUChar("gspd", geneSpd);
  prefs.putUChar("thp", trHp);
  prefs.putUChar("tspa", trSpa);
  prefs.putUChar("tspd", trSpd);
  prefs.putBool("bk", berryKnown);
  // uint32: con 30 logros el uint16 de antes perdia los de la mitad
  // de arriba, y esos habrian vuelto a pagar en cada arranque.
  prefs.putUInt("mpd32", medalPaid);
  prefs.putUChar("evoc", evoCount);
  prefs.putUInt("fage", finalAge);
  prefs.putBool("fat", wasFat);
  prefs.putBool("shy", shiny);
  prefs.putBool("eshy", eggShiny);
  prefs.putBool("stpk", starterPick);
  prefs.putBool("gdo", guardado);
  guarda("dexsh", dexShinyReg, sizeof(dexShinyReg));
  guarda("dexfm", dexFormReg, sizeof(dexFormReg));
  guarda("dexmg", dexMegaReg, sizeof(dexMegaReg));
  prefs.putUInt("age", ageMinutes);
  prefs.putUInt("exp", exp);
  prefs.putShort("dexn", speciesId);
  prefs.putShort("eggT2", eggTarget);
  prefs.putUChar("crack", eggTaps);
  prefs.putUChar("mist", careMistakes);
  prefs.putBool("sleep", sleeping);
  prefs.putUChar("lend", lastEnd);
  // La herencia del huevo: el huevo puede pasarse horas sin eclosionar
  // y un apagon no puede tragarse lo que dejo el que se despidio.
  prefs.putShort("lgdx", legacyDex);
  prefs.putUChar("lgfl", legacyFloor);
  prefs.putUShort("lgmv", legacyMove);
  prefs.putUChar("lgit", legacyItem);
  if (lastSeenEpoch) prefs.putUInt("seen", lastSeenEpoch);
  guarda("dexreg", dexReg, sizeof(dexReg));
  // Leva 72. "dexvis" e nao "seen": "seen" ja e o lastSeenEpoch, ali em cima.
  guarda("dexvis", dexSeen, sizeof(dexSeen));
  prefs.putShort("cacad", cacaDex);
  prefs.putUChar("held", heldItem);
  prefs.putBool("mvpick", movesPicked);
  // putUShort y no putUChar: la habilidad paso de 255 con Galar y
  // guardarla en un byte la truncaba al cargar.
  prefs.putUShort("abil", ability);
  prefs.putBool("gset", genderSet);
  prefs.putBool("fem", female);
  prefs.putUChar("form", form);
  prefs.putUChar("aslot", mainSlot);
  storeActive();   // el titular vive en el Pet: se vuelca a su hueco al guardar
  guarda("team", team, sizeof(team));
  // (Aqui havia a "geracao de historia": relia o bloco "teams" inteiro e,
  // se encolhesse, gravava outra copia de 2,8 KB em "teamsbk". Saiu junto
  // com o bloco, ver abaixo.)
  // O time da regiao ONDE SE ESTA sai do `team` vivo, nao do cache:
  // `teamsIn[region]` so se sincroniza ao viajar, entao sem isto um
  // reinicio no meio do jogo perdia tudo o que se andou desde a
  // ultima viagem.
  if (region < REGION_COUNT) {
    for (uint8_t i = 0; i < TEAM_SLOTS; i++) teamsIn[region][i] = team[i];
    mainSlotIn[region] = mainSlot;
    guardaTimeDe(region);
  }
  // O BLOCO UNICO "teams" NAO SE ESCREVE MAIS. Eram 2,8 KB repetindo o que
  // tm0..tm8 ja guardam, e com ele a NVS ficou com 409 de 504 entradas: para
  // regravar um bloco desses ela precisa de sitio para a copia nova ANTES
  // de apagar a velha (409 + 92 = 501). Passou do limite em Hoenn, nada mais
  // se gravou, e o reinicio levou a regiao inteira do dono (2026-11-08).
  guarda("mslin", mainSlotIn, sizeof(mainSlotIn));
  prefs.putUShort("vmask", visitedMask);
  // Mientras la migracion del banco viejo no se haya podido completar,
  // esta clave NO se toca: todavia guarda los 60 registros de antes y
  // escribir 30 encima seria borrar la mitad.
  if (!bancoViejoPendiente)
    guarda("bank", bank, sizeof(bank));   // la caja ABIERTA, de red
  prefs.putUChar("boxn", boxOpen);
  prefs.putUChar("bsort", bagSort);
  guarda("bag", bag, sizeof(bag));
  // Las MT, y CUANTAS habia cuando se guardaron. El numero de cada MT
  // sale del orden alfabetico de los golpes, asi que si el Anil anade
  // uno se renumeran todas: sin esta cuenta, una partida vieja
  // aparecería con MT distintas de las que gano. Ver load().
  guarda("tms", tms, sizeof(tms));
  prefs.putUShort("tmn", TM_COUNT);
  guarda("ppm", ppMove, sizeof(ppMove));
  guarda("ppl", ppLeft, sizeof(ppLeft));
  prefs.putUInt("avd", avDesde);
  prefs.putUInt("avs", avSeed);
  // Con cuantos golpes se guardo. Los indices son posiciones en una tabla
  // ORDENADA POR NOMBRE, asi que solo valen mientras la tabla sea la misma
  // (ver load()).
  prefs.putUShort("mvn", (uint16_t)MOVE_COUNT);
  prefs.putUShort("hp", hpLeft);
  prefs.putUShort("hpsn", hpSeen);
  prefs.putUShort("strk", streak);
  prefs.putUShort("bstrk", bestStreak);
  prefs.putUInt("cday", lastCareDay);
  prefs.putUChar("bond", bond);
  prefs.putUInt("ach", medals);
  prefs.putUShort("tmedal", totalMedals);
  prefs.putUShort("mstone", lastMilestone);
  prefs.putUShort("shi", trainHi);  // clave vieja: no se toca para no perder
                                    // el record de las partidas ya guardadas
  // Marca de que este guardado ya es de la epoca de la obediencia. Se escribe
  // SIEMPRE, no solo al migrar: si solo lo pusiera load(), un guardado nuevo
  // que nunca hubiera pasado por load() se tomaria por antiguo y le subirian
  // el vinculo al suelo sin venir a cuento.
  prefs.putUChar("bnd0", 1);
  prefs.putBool("intro", introDone);
  prefs.putBool("girl", playerGirl);
  prefs.putUChar("bdg", badges);
  prefs.putUChar("gtr", gymTrainers);
  prefs.putUInt("wins", wins);
  prefs.putUInt("trcu", trocaUltima);
  prefs.putUShort("duv", duV);
  prefs.putUShort("dud", duD);
  prefs.putUShort("due", duE);
  prefs.putUShort("caught", caught);
  prefs.putUChar("regn", region);
  prefs.putUChar("tmreg", teamRegion);
  prefs.putUChar("maxrg", maxRegion);
  // Las de cada region. La de AHORA se vuelca primero: `badges` es la
  // que se mueve, y `badgesIn[region]` solo se pone al dia al viajar y
  // al ganar un desafio. Sin esto, apagar el cacharro entre medias
  // guardaba la cuenta de antes.
  {
    uint8_t copia[REGION_COUNT];
    memcpy(copia, badgesIn, sizeof(copia));
    if (region < REGION_COUNT) copia[region] = badges;
    guarda("bdgin", copia, sizeof(copia));
  }
  prefs.putString("nick", nick);
  prefs.putUChar("ball", ballId);
  // O ESTADO no SD: onde se esta, ate onde se chegou, insignias, e a
  // sequencia. Se a NVS parar, e por aqui que o boot se acha.
  {
    SvEstado e;
    memset(&e, 0, sizeof(e));
    e.seq = seqSave;
    e.region = region;
    e.maxRegion = maxRegion;
    e.teamRegion = teamRegion;
    e.regioes = REGION_COUNT;
    e.visitedMask = visitedMask;
    memcpy(e.badgesIn, badgesIn, sizeof(e.badgesIn));
    if (region < REGION_COUNT) e.badgesIn[region] = badges;
    memcpy(e.mainSlotIn, mainSlotIn, sizeof(e.mainSlotIn));
    if (region < REGION_COUNT) e.mainSlotIn[region] = mainSlot;
    sdGravouEstado = svGrava("/save/estado", &e, sizeof(e));
  }
  // A SENTINELA, por ultimo. As escritas pequenas (putUChar & cia.) nao
  // olham o retorno; com a NVS sem sitio TODAS falham, e sem isto o save so
  // se queixaria se um bloco grande falhasse.
  if (prefs.putUInt("svn", seqSave) != sizeof(uint32_t)) {
    guardadoFallo = true;
    if (bloquesFallados < 255) bloquesFallados++;
    Serial.println("[save] a NVS recusou a sentinela: NADA desta gravacao e garantido");
  }
}

void Pet::load() {
  guardado = prefs.getBool("gdo", false);
  fullness = prefs.getUChar("full", 80);
  joy = prefs.getUChar("joy", 80);
  energy = prefs.getUChar("ene", 80);
  hygiene = prefs.getUChar("hyg", 100);
  poops = prefs.getUChar("poop", 0);
  weight = prefs.getUChar("wgt", 0);
  geneAtk = prefs.getUChar("gatk", 0);
  geneDef = prefs.getUChar("gdef", 0);
  geneSpe = prefs.getUChar("gspe", 0);
  if (geneAtk == 0) {  // mascota anterior a los genes: tirada unica ahora
    geneAtk = 90 + random(21);
    geneDef = 90 + random(21);
    geneSpe = 90 + random(21);
  }
  geneHp = prefs.getUChar("ghp", 0);
  geneSpa = prefs.getUChar("gspa", 0);
  geneSpd = prefs.getUChar("gspd", 0);
  trHp = prefs.getUChar("thp", 0);
  trSpa = prefs.getUChar("tspa", 0);
  trSpd = prefs.getUChar("tspd", 0);
  trAtk = prefs.getUChar("tatk", 0);
  trDef = prefs.getUChar("tdef", 0);
  trSpe = prefs.getUChar("tspe", 0);
  // Partida de cuando solo habia TRES genes y tres EVs. El especial usaba el
  // campo de su pareja fisica, asi que HEREDARLOS deja los stats exactamente
  // como estaban; poner 0 o volver a tirar le cambiaria el bicho al jugador
  // por sorpresa. Se detecta por geneSpa, que en una partida nueva nunca
  // vale 0 (los genes van de 90 a 110).
  if (geneSpa == 0) {
    geneHp = geneDef;
    geneSpa = geneAtk;
    geneSpd = geneDef;
    trSpa = trAtk;
    trSpd = trDef;
    trHp = 0;
  }
  berryKnown = prefs.getBool("bk", false);
  // Una partida vieja solo tiene la llave corta: sus ocho bits caen
  // donde siempre y el resto queda a cero (= aun sin pagar).
  medalPaid = prefs.getUInt("mpd32", prefs.getUShort("mpd", 0));
  evoCount = prefs.getUChar("evoc", 0);
  finalAge = prefs.getUInt("fage", 0);
  wasFat = prefs.getBool("fat", false);
  shiny = prefs.getBool("shy", false);
  eggShiny = prefs.getBool("eshy", false);
  starterPick = prefs.getBool("stpk", false);
  // Reparacion de una sola vez de las partidas hechas antes de lo de
  // arriba: los capturados que se guardaron con hpLeft=0 estan vivos pero
  // el combate los ve desmayados, y como nada los cura se quedan
  // inservibles para siempre. Se les pone su maximo.
  //
  // Va con llave propia para que ocurra UNA vez: si corriera en cada
  // arranque, resucitaria tambien a los que cayeron de verdad.
  if (!prefs.getBool("hpfix", false)) {
    prefs.putBool("hpfix", true);
    for (uint8_t i = 0; i < TEAM_SLOTS; i++) {
      StoredMon &m = team[i];
      if (m.used() && m.hpLeft == 0)
        m.hpLeft = hpAtLevelForm(m.species, Pet::levelOf(m), m.trHp, m.form);
    }
    for (uint8_t i = 0; i < BANK_SLOTS; i++) {
      StoredMon &m = bank[i];
      if (m.used() && m.hpLeft == 0)
        m.hpLeft = hpAtLevelForm(m.species, Pet::levelOf(m), m.trHp, m.form);
    }
  }
  prefs.getBytes("dexsh", dexShinyReg, sizeof(dexShinyReg));
  prefs.getBytes("dexfm", dexFormReg, sizeof(dexFormReg));
  prefs.getBytes("dexmg", dexMegaReg, sizeof(dexMegaReg));
  ageMinutes = prefs.getUInt("age", 0);
  exp = prefs.getUInt("exp", 0);
  if (!exp) {
    // Partida anterior a que el nivel dejara de ser la edad: se convierte el
    // nivel que tenia a la experiencia equivalente, para que nadie pierda
    // progreso al actualizar el firmware.
    uint32_t viejo = 1 + ageMinutes / MINUTES_PER_LEVEL;
    if (viejo > LEVEL_CAP) viejo = LEVEL_CAP;
    exp = expForLevel((uint16_t)viejo);
  }
  if (prefs.isKey("dexn")) {
    speciesId = prefs.getShort("dexn", -1);
    eggTarget = prefs.getShort("eggT2", 4);
  } else {
    // migracion desde la version con indices de flash (0-8)
    static const uint8_t OLD2DEX[9] = { 4, 5, 6, 1, 2, 3, 7, 8, 9 };
    int8_t old = prefs.getChar("spec", -1);
    speciesId = (old >= 0 && old < 9) ? OLD2DEX[old] : -1;
    int8_t oldT = prefs.getChar("eggT", 0);
    eggTarget = (oldT >= 0 && oldT < 9) ? OLD2DEX[oldT] : 4;
  }
  eggTaps = prefs.getUChar("crack", 0);
  careMistakes = prefs.getUChar("mist", 0);
  sleeping = prefs.getBool("sleep", false);
  lastEnd = prefs.getUChar("lend", CER_NONE);
  legacyDex = prefs.getShort("lgdx", 0);
  legacyFloor = prefs.getUChar("lgfl", LEGACY_FLOOR_BASE);
  legacyMove = prefs.getUShort("lgmv", 0);
  legacyItem = prefs.getUChar("lgit", 0);
  prefs.getBytes("dexreg", dexReg, sizeof(dexReg));
  // Leva 72: um save velho nao tem a chave e fica com os vistos a zero;
  // isSeen() conta os registrados, entao nada que voce teve some.
  memset(dexSeen, 0, sizeof(dexSeen));
  prefs.getBytes("dexvis", dexSeen, sizeof(dexSeen));
  cacaDex = prefs.getShort("cacad", 0);
  if (cacaDex < 0 || cacaDex > DEX_COUNT) cacaDex = 0;
  heldItem = prefs.getUChar("held", 0);
  movesPicked = prefs.getBool("mvpick", false);
  ability = prefs.getUShort("abil", 0);
  // El sexo: `gset` dice si ya se sorteo. Una partida anterior al sexo no
  // lo tiene, y entonces se tira ahora en vez de darla por macho.
  genderSet = prefs.getBool("gset", false);
  female = prefs.getBool("fem", false);
  form = prefs.getUChar("form", 0);
  mainSlot = prefs.getUChar("aslot", 0);
  if (mainSlot >= TEAM_SLOTS) mainSlot = 0;
  prefs.getBytes("team", team, sizeof(team));
  // EL BANCO VIEJO. Hasta ahora eran 60 huecos en una sola entrada de la
  // NVS; ahora son 30 (una caja) y el resto vive en la SD. Una partida de
  // antes trae 60, y getBytes dice cuantos bytes habia de verdad: los 30
  // primeros se quedan en la caja 0 y los otros 30 pasan a la caja 1.
  {
    // static: 60 x 52 = 3 KB. Na pilha do arranque, somado ao resto,
    // e o tipo de coisa que estoura e poe a placa em loop.
    static StoredMon viejo[60];
    for (uint8_t i = 0; i < 60; i++) viejo[i].species = 0;
    size_t leidos = prefs.getBytes("bank", viejo, sizeof(viejo));
    uint8_t habia = (uint8_t)(leidos / sizeof(StoredMon));
    for (uint8_t i = 0; i < BANK_SLOTS; i++) bank[i] = viejo[i];
    boxOpen = (uint8_t)prefs.getUChar("boxn", 0);
    bagSort = (uint8_t)prefs.getUChar("bsort", 0);
    if (bagSort >= BAG_SORTS) bagSort = 0;
    if (boxOpen >= BOX_COUNT) boxOpen = 0;
    if (habia > BANK_SLOTS) {
      // Hay cola del banco antiguo: se escribe tal cual en la caja 1 y la
      // NVS se queda solo con la caja que corresponda.
      uint8_t guardaba = boxOpen;
      StoredMon aparte[BANK_SLOTS];
      for (uint8_t i = 0; i < BANK_SLOTS; i++) {
        uint8_t j = (uint8_t)(BANK_SLOTS + i);
        aparte[i] = (j < habia) ? viejo[j] : StoredMon();
      }
      StoredMon abierta[BANK_SLOTS];
      for (uint8_t i = 0; i < BANK_SLOTS; i++) abierta[i] = bank[i];
      for (uint8_t i = 0; i < BANK_SLOTS; i++) bank[i] = aparte[i];
      boxOpen = 1;
      // Si la caja 1 no se puede escribir, NO se da la migracion por
      // hecha: se levanta la bandera y save() deja el blob viejo de 60
      // intacto, para reintentarlo en la proxima carga. Sin esto, un
      // fallo de escritura aqui borra lo que habia en las casas 30..59.
      bancoViejoPendiente = !boxWrite();
      for (uint8_t i = 0; i < BANK_SLOTS; i++) bank[i] = abierta[i];
      boxOpen = guardaba;
    }
  }
  if (!ability && !isEgg()) rollAbility();  // partida vieja: se sortea ahora
  prefs.getBytes("bag", bag, sizeof(bag));
  // Las MT solo valen si el catalogo no ha cambiado de tamano. Si
  // cambio, los numeros ya no significan lo mismo y lo honrado es
  // empezar con cero MT: quedarse con un mapa de bits que apunta a
  // otros golpes seria regalar cosas al azar sin que nadie lo note.
  //
  // Desde que la orden de las MT esta congelada (tools/orden_mts.txt), las
  // nuevas van al FINAL: un guardado de entre TM_COUNT_FIJO y TM_COUNT
  // tiene los mismos bits que hoy y se lee tal cual; lo que no tenia
  // simplemente no lo tiene.
  uint16_t tmn = prefs.getUShort("tmn", 0);
  if (tmn >= TM_COUNT_FIJO && tmn <= TM_COUNT) {
    memset(tms, 0, sizeof(tms));
    prefs.getBytes("tms", tms, sizeof(tms));
  } else if (tmn == TM_COUNT_V304 && TM_V304_MOVE_COUNT >= MOVE_COUNT_FIJO &&
             TM_V304_MOVE_COUNT <= MOVE_COUNT) {
    // (los indices de golpe de TM_MOVE_V304 siguen valiendo: la tabla de
    // golpes tambien tiene la orden congelada desde MOVE_COUNT_FIJO)
    // O save da tabela VELHA de MT, que apontava cada MT para o golpe
    // errado (ver tms_v304.h). Cada uma que voce tinha vira a MT nova do
    // MESMO golpe que voce via; se esse golpe nem e MT de verdade (havia
    // V-Create, Roar of Time...), vira uma do mesmo TIPO que voce ainda
    // nao tem. Fica a mesma quantidade, dos mesmos tipos.
    uint8_t velho[TM_BYTES_V304] = { 0 };
    prefs.getBytes("tms", velho, sizeof(velho));
    memset(tms, 0, sizeof(tms));
    for (uint16_t i = 0; i < TM_COUNT_V304; i++) {
      if (!(velho[i >> 3] & (1 << (i & 7)))) continue;
      uint16_t mv = TM_MOVE_V304[i];
      int16_t novo = -1;
      for (uint16_t j = 0; j < TM_COUNT && novo < 0; j++)
        if (TM_MOVE[j] == mv && !hasTM(j)) novo = (int16_t)j;
      for (uint16_t j = 0; j < TM_COUNT && novo < 0; j++)
        if (!hasTM(j) && MOVE_TBL[TM_MOVE[j]].type == MOVE_TBL[mv].type)
          novo = (int16_t)j;
      if (novo >= 0) tms[novo >> 3] |= (uint8_t)(1 << (novo & 7));
    }
  } else {
    memset(tms, 0, sizeof(tms));
  }
  prefs.getBytes("ppm", ppMove, sizeof(ppMove));
  prefs.getBytes("ppl", ppLeft, sizeof(ppLeft));
  // La tabla de golpes esta ORDENADA POR NOMBRE, asi que un golpe nuevo se
  // mete en medio y corre los indices de todos los que van detras. Lo que
  // habia guardado como "SURF" pasa a ser otro golpe, sin aviso: al entrar
  // Johto fueron 46 golpes nuevos y la mitad de la tabla se movio.
  //
  // Se guarda cuantos golpes habia. Si ya no son los mismos, se suelta la
  // eleccion del jugador (la suya y la de los suplentes) y cada uno vuelve a
  // su aprendizaje, que SI se recalcula bien. Se pierde la eleccion una vez;
  // la alternativa es un equipo con golpes que nadie puso.
  //
  // Desde la orden congelada (tools/orden_golpes.txt) lo nuevo va AL
  // FINAL: una partida de entre MOVE_COUNT_FIJO y MOVE_COUNT golpes tiene
  // los mismos indices que hoy y no se toca. Solo una tabla de ANTES de
  // congelar (o una mas grande, de un firmware mas nuevo) se rehace.
  uint16_t golpesAntes = prefs.getUShort("mvn", 0);
  if (golpesAntes && (golpesAntes < MOVE_COUNT_FIJO || golpesAntes > MOVE_COUNT)) {
    Serial.printf("la tabla de golpes cambio (%u -> %u): se rehacen los movesets\n",
                  golpesAntes, (unsigned)MOVE_COUNT);
    movesPicked = false;
    for (uint8_t i = 0; i < 4; i++) { ppMove[i] = 0; ppLeft[i] = 0; }
    for (uint8_t s = 0; s < TEAM_SLOTS; s++) team[s].flags &= (uint8_t)~SM_MOVES_PICKED;
    for (uint8_t s = 0; s < BANK_SLOTS; s++) bank[s].flags &= (uint8_t)~SM_MOVES_PICKED;
  }
  hpLeft = prefs.getUShort("hp", 0);
  avDesde = prefs.getUInt("avd", 0);
  avSeed = prefs.getUInt("avs", 0);
  hpSeen = prefs.getUShort("hpsn", 0);
  ppSync();   // partida vieja sin la clave, o moveset cambiado estando apagado
  hpSync();   // idem: sin "hpsn" arranca entero, y si subio de nivel apagado, suma
  streak = prefs.getUShort("strk", 0);
  bestStreak = prefs.getUShort("bstrk", 0);
  lastCareDay = prefs.getUInt("cday", 0);
  bond = prefs.getUChar("bond", BOND_AT_BIRTH);
  // Una partida ANTERIOR a la obediencia se crio con el vinculo arrancando en
  // cero, asi que hasta el jugador mas cuidadoso puede andar por 20 — y con
  // la regla nueva eso seria desobedecer sin haber hecho nada mal. Se le
  // sube al suelo UNA vez; la clave es el marcador de que ya esta hecho.
  if (!prefs.isKey("bnd0")) {
    if (bond < BOND_AT_BIRTH) bond = BOND_AT_BIRTH;
    prefs.putUChar("bnd0", 1);
  }
  introDone = prefs.getBool("intro", false);
  playerGirl = prefs.getBool("girl", false);
  badges = prefs.getUChar("bdg", 0);
  gymTrainers = prefs.getUChar("gtr", 0);
  wins = prefs.getUInt("wins", 0);
  trocaUltima = prefs.getUInt("trcu", 0);
  duV = prefs.getUShort("duv", 0);
  duD = prefs.getUShort("dud", 0);
  duE = prefs.getUShort("due", 0);
  caught = prefs.getUShort("caught", 0);
  // MIGRACION. Antes `regn` era "hasta donde llegaste" y `zone` "donde
  // andas"; ahora `region` es DONDE ANDAS y `maxRegion` lo destrabado.
  // Una partida vieja se lee sola: su `regn` es el tope de entonces y su
  // `zone` el sitio. Sin esto, quien tuviera Johto abierto y estuviera
  // de visita en Kanto despertaba en Johto.
  uint8_t regGuardada = prefs.getUChar("regn", 0);
  maxRegion = prefs.getUChar("maxrg", regGuardada);
  region = prefs.getUChar("zone", regGuardada);
  // Acotadas SIEMPRE: un valor imposible (fichero tocado, region
  // retirada del juego) no puede dejar la hierba vacia en silencio.
  if (maxRegion >= REGION_COUNT) maxRegion = (uint8_t)(REGION_COUNT - 1);
  // NAO SE TELEPORTA O JOGADOR. Esta linha era
  //    if (region > maxRegion || region >= REGION_COUNT)
  //        region = maxRegion;
  // e foi ela que mandou o dono de Johto para Kanto: bastou o
  // maxRegion voltar menor num boot. Dali a migracao gravou o time
  // de Johto como se fosse o de Kanto e zerou o resto —— cinco
  // bichos de nivel 50 com megapedras.
  //
  // Estar numa regiao e a PROVA de que ela estava destravada.
  // Entao quem se corrige e o maxRegion, nunca o region: mover o
  // jogador de lugar sem ele pedir e sempre o erro pior.
  if (region >= REGION_COUNT) region = 0;

  // O CARIMBO DO TIME MANDA.
  //
  // Se o time guardado diz pertencer a outra regiao, e porque
  // `region` se perdeu —— nao porque o time andou sozinho. Entao
  // corrige-se a regiao a partir do time, e nunca o contrario:
  // seguir o `region` errado faria a proxima gravacao arquivar
  // este time por cima do de outra regiao, que foi exatamente o
  // que aconteceu.
  teamRegion = prefs.getUChar("tmreg", region);
  if (teamRegion >= REGION_COUNT) teamRegion = region;
  if (teamRegion != region) {
    Serial.printf("a regiao veio %u mas o time e da %u: manda o time\n",
                  (unsigned)region, (unsigned)teamRegion);
    region = teamRegion;
  }
  if (maxRegion < region) maxRegion = region;
  if (maxRegion >= REGION_COUNT) maxRegion = (uint8_t)(REGION_COUNT - 1);
  prefs.getBytes("bdgin", badgesIn, sizeof(badgesIn));
  // Una partida anterior a las insignias por region solo tiene la cuenta
  // suelta: es la de la region en la que estaba.
  if (region < REGION_COUNT && !badgesIn[region]) badgesIn[region] = badges;
  badges = badgesIn[region < REGION_COUNT ? region : 0];

  // MIGRACAO: save de um time so -> um time por regiao.
  //
  // E segura por construcao e por isso NAO precisa de commit em duas
  // fases: a chave "team" continua sendo a fonte do time atual e nao se
  // apaga nunca. Se a escrita de "teams" falhar, o boot seguinte refaz
  // a migracao a partir dela. Nada velho e destruido —— que foi
  // exatamente o que deu errado na migracao do banco.
  //
  // O time que existe vai pra regiao onde o jogador ESTA, e so essa
  // conta como pisada: as outras ganham o ovo do inicial quando ele
  // chegar la.
  uint8_t regAhora = region < REGION_COUNT ? region : 0;
  // AS NOVE CHAVES, uma por regiao, cada uma carimbada. Lidas uma a
  // uma: uma que falhe ou venha com o carimbo errado deixa a SUA
  // regiao vazia e nao toca nas outras.
  // O SD esta a frente da NVS? Entao a NVS parou de gravar em algum
  // momento, e os times saem do SD.
  SvEstado estSd;
  const bool temEstadoSd = svLe("/save/estado", &estSd, sizeof(estSd)) &&
                           estSd.regioes == REGION_COUNT;
  const uint32_t seqNvs = prefs.getUInt("svn", 0);
  seqSave = seqNvs;
  sdManda = temEstadoSd && estSd.seq > seqNvs;
  bool algumaChave = false;
  uint8_t chavesRuins = 0;   // existem mas nao leram (tamanho, carimbo)
  for (uint8_t r = 0; r < REGION_COUNT; r++) {
    if (leTimeDe(r)) {
      algumaChave = true;
    } else {
      char k[8];
      claveTime(r, k, sizeof(k));
      if (prefs.isKey(k)) chavesRuins++;
    }
  }

  size_t lidos = algumaChave ? sizeof(teamsIn)
                             : prefs.getBytes("teams", teamsIn, sizeof(teamsIn));
  if (!algumaChave && lidos == sizeof(teamsIn)) {
    // Save do formato antigo (um bloco so): converte-se para as nove
    // chaves. O bloco velho NAO se apaga.
    prefs.getBytes("mslin", mainSlotIn, sizeof(mainSlotIn));
    for (uint8_t r = 0; r < REGION_COUNT; r++) guardaTimeDe(r);
    Serial.println("times: convertidos para uma chave por regiao");
  }
  // UMA VEZ E SO UMA. Antes a condicao era "a leitura nao bateu", e
  // com ela a migracao voltava a rodar em QUALQUER boot em que a
  // chave nao voltasse inteira —— e o que ela fazia era ZERAR os nove
  // times para refazer so o da regiao atual. Um boot ruim em Johto
  // apagava o time de Kanto. Comeu cinco bichos de nivel 50 do dono.
  //
  // Agora quem manda e uma marca propria, e ela so se poe depois de a
  // escrita ter dado certo.
  bool jaMigrou = prefs.getBool("teamsmig", false);
  if (!jaMigrou && lidos != sizeof(teamsIn)) {
    // NAO SE ZERA NADA. Num boot de verdade o Pet nasce com os times
    // vazios, entao o zeramento nunca protegeu de coisa nenhuma: so
    // servia para apagar o que tivesse sobrevivido.
    //
    // E so se preenche uma regiao VAZIA: se ja ha gente la, o que quer
    // que tenha acontecido, nao se passa por cima.
    bool vazia = true;
    for (uint8_t i = 0; i < TEAM_SLOTS; i++)
      if (teamsIn[regAhora][i].used()) vazia = false;
    if (vazia) {
      for (uint8_t i = 0; i < TEAM_SLOTS; i++) teamsIn[regAhora][i] = team[i];
      mainSlotIn[regAhora] = mainSlot;
    }
    visitedMask |= (uint16_t)(1u << regAhora);
    // A marca so vai depois de guardar, e so se guardar deu certo.
    guarda("teams", teamsIn, sizeof(teamsIn));
    if (!guardadoFallo) prefs.putBool("teamsmig", true);
  } else {
    prefs.getBytes("mslin", mainSlotIn, sizeof(mainSlotIn));
    visitedMask = prefs.getUShort("vmask", (uint16_t)(1u << regAhora));
    // Onde voce esta conta como pisada, sempre: um save que chegou aqui
    // com a mascara zerada ficaria pedindo inicial pra sempre.
    visitedMask |= (uint16_t)(1u << regAhora);
  }
  if (sdManda) {
    // A NVS ficou para tras: onde se esta, ate onde se chegou e as
    // insignias saem do SD, e o titular sai do time desta regiao.
    Serial.printf("save: o SD (seq %lu) e mais novo que a NVS (seq %lu): manda o SD\n",
                  (unsigned long)estSd.seq, (unsigned long)seqNvs);
    seqSave = estSd.seq;
    if (estSd.region < REGION_COUNT) region = estSd.region;
    maxRegion = estSd.maxRegion < REGION_COUNT ? estSd.maxRegion : region;
    if (maxRegion < region) maxRegion = region;
    teamRegion = region;
    visitedMask = (uint16_t)(estSd.visitedMask | (1u << region));
    memcpy(badgesIn, estSd.badgesIn, sizeof(badgesIn));
    badges = badgesIn[region];
    for (uint8_t r = 0; r < REGION_COUNT; r++)
      if (estSd.mainSlotIn[r] < TEAM_SLOTS) mainSlotIn[r] = estSd.mainSlotIn[r];
    bool alguem = false;
    for (uint8_t i = 0; i < TEAM_SLOTS; i++) {
      team[i] = teamsIn[region][i];
      if (team[i].used()) alguem = true;
    }
    mainSlot = mainSlotIn[region] < TEAM_SLOTS ? mainSlotIn[region] : 0;
    if (alguem) {
      if (!team[mainSlot].used())
        for (uint8_t i = 0; i < TEAM_SLOTS; i++)
          if (team[i].used()) { mainSlot = i; break; }
      cargaDelHueco(mainSlot);
    }
  }
  // AS CHAVES QUE SOBRARAM NA NVS vao para o SD (cada uma so sai da NVS com
  // o arquivo gravado e conferido). A da regiao atual leva o time VIVO, que
  // e o que vale para ela.
  for (uint8_t r = 0; r < REGION_COUNT; r++) {
    char k[8];
    claveTime(r, k, sizeof(k));
    if (!prefs.isKey(k)) continue;
    if (r == region)
      for (uint8_t i = 0; i < TEAM_SLOTS; i++) teamsIn[r][i] = team[i];
    guardaTimeDe(r);
  }
  // Havendo chaves por regiao, o bloco unico ja nao e fonte de nada (so se
  // le com chave NENHUMA, ver acima) e so ocupa a NVS. Sai, e a copia
  // "teamsbk" com ele. Chave que falta nao conta (regiao nunca pisada); uma
  // que EXISTE e nao leu, sim: ai o bloco velho fica, pode ser o que salva.
  if (algumaChave && !chavesRuins) {
    if (prefs.isKey("teams")) {
      prefs.remove("teams");
      Serial.println("times: bloco unico 'teams' apagado (as 9 chaves estao boas)");
    }
    if (prefs.isKey("teamsbk")) prefs.remove("teamsbk");
  }
  medals = prefs.getUInt("ach", prefs.getUShort("medal", 0));
  totalMedals = prefs.getUShort("tmedal", 0);
  lastMilestone = prefs.getUShort("mstone", 0);
  trainHi = prefs.getUShort("shi", 0);
  prefs.getString("nick", nick, sizeof(nick));
  ballId = prefs.getUChar("ball", 0);
  if (ballId >= ITEM_COUNT || ITEM_TBL[ballId].pocket != BP_BALL) ballId = 0;
  // siembra: la mascota actual cuenta como criada (guardados antiguos)
  if (speciesId >= 1) registerSpecies(speciesId, shiny, form);
  // AUTOCURA: quem voce TEM esta na Pokedex. O dexreg mora na NVS e ja se
  // perdeu (NVS cheia, 2026-11-08: capturas sem ficha), e havia caminhos que
  // punham bicho no time sem registrar. Time, banco (caixa aberta) e os times
  // das 9 regioes; as caixas fechadas no SD ficam de fora (ler 32 arquivos no
  // boot), essas foram consertadas a mao com REG.
  registraOsQueTem();
}

void Pet::registraOsQueTem() {
  auto reg = [this](const StoredMon &m) {
    if (m.used()) registerSpecies(m.species, (m.flags & SM_SHINY) != 0, m.form);
  };
  for (uint8_t i = 0; i < TEAM_SLOTS; i++) reg(team[i]);
  for (uint8_t i = 0; i < BANK_SLOTS; i++) reg(bank[i]);
  for (uint8_t r = 0; r < REGION_COUNT; r++)
    for (uint8_t i = 0; i < TEAM_SLOTS; i++) reg(teamsIn[r][i]);
}
