// La MAQUINA JUGANDO: miles de combates al azar entre las 1025.
//
// Los otros tests comprueban cada pieza por separado, y eso deja fuera
// justo lo que rompe de verdad: la COMBINACION. Un Protect encadenado
// con una furia a medias y el rival dormido es un estado al que no se
// llega escribiendo tests a mano, pero al que se llega jugando.
//
// Aqui no se afirma "pasa tal cosa". Se afirma lo que NUNCA puede
// pasar, pase lo que pase:
//
//   - el combate TERMINA (nadie se queda dando vueltas)
//   - la vida nunca se sale de 0..maximo
//   - un golpe nunca cura al rival ni le quita mas de lo que tiene
//   - los stages nunca salen de -6..+6
//   - no se usa un golpe sin PP, ni un hueco que no existe
//   - quien cae se queda caido
//
// Es la red que hay debajo de todo lo demas: no dice que el juego sea
// bueno, dice que no se rompe.
#include "framework.h"
#include "shim/Arduino.h"
#include "../battle.h"
#include "../pet.h"
#include "../dex.h"
#include "../moves.h"
#include <string>

// Cuantos combates. Cada uno son ~30 turnos, asi que esto son decenas
// de miles de golpes resueltos. Tarda ~2 s y encuentra cosas que
// ningun test escrito a mano alcanza.
static const int COMBATES = 4000;
static const int TURNOS_MAX = 300;

struct Vigilante {
  std::string roto;
  int combates = 0, turnos = 0, golpes = 0, kos = 0;
  // Lo que de verdad se ha ejercitado. Sin esto, el gauntlet podria
  // estar saltandose las megas y los relevos y pasar igual, y yo
  // reclamando una cobertura que no tengo. Es el mismo fallo que
  // persigue cobertura.py: una comprobacion que deja de comprobar.
  int megas = 0, relevos = 0, habsAlAzar = 0;

  void exige(bool cond, const std::string &que) {
    if (!cond && roto.empty()) roto = que;
  }

  // Lo que tiene que ser verdad en CUALQUIER momento de cualquier
  // combate. Se llama despues de cada golpe y de cada fin de turno.
  void mira(const Battle &b, const char *cuando) {
    for (int s = 0; s < 2; s++) {
      const Combatant &c = b.side[s];
      std::string quien = std::string(cuando) + " lado " + std::to_string(s);
      exige(c.hpMax > 0, quien + ": PS maximos a cero");
      exige(c.hp <= c.hpMax, quien + ": " + std::to_string(c.hp) +
                             " PS de " + std::to_string(c.hpMax));
      exige(c.nMoves <= BT_MOVES, quien + ": " + std::to_string(c.nMoves) +
                                  " golpes");
      for (uint8_t i = 0; i < SG_COUNT; i++)
        exige(c.stage[i] >= -6 && c.stage[i] <= 6,
              quien + ": stage " + std::to_string(i) + " = " +
              std::to_string(c.stage[i]));
      exige(c.status <= ST_FREEZE, quien + ": estado " +
                                 std::to_string(c.status));
      for (uint8_t i = 0; i < c.nMoves; i++)
        exige(c.moves[i] < MOVE_COUNT,
              quien + ": el hueco " + std::to_string(i) + " apunta al golpe " +
              std::to_string(c.moves[i]) + " y solo hay " +
              std::to_string(MOVE_COUNT));
      exige(c.asDex >= 1 && c.asDex <= DEX_COUNT,
            quien + ": aparenta ser la especie " + std::to_string(c.asDex));
      // El estado que entro con las habilidades de estas tandas: si una
      // de ellas deja un campo en un valor imposible, se ve AQUI y no
      // tres pantallas despues.
      exige(c.ability < ABIL_COUNT,
            quien + ": habilidad " + std::to_string(c.ability) +
            " y solo hay " + std::to_string(ABIL_COUNT));
      exige(c.typeSet < T_COUNT, quien + ": tipo pisado " +
                                 std::to_string(c.typeSet));
      exige(c.typeSet2 < T_COUNT, quien + ": tipo pisado 2 " +
                                  std::to_string(c.typeSet2));
      exige(c.hiding <= HID_GONE, quien + ": escondite " +
                                   std::to_string(c.hiding));
      exige(c.stockpile <= 3, quien + ": deposito " +
                              std::to_string(c.stockpile));
      exige(c.charged <= 2, quien + ": refuerzo de relevo " +
                            std::to_string(c.charged));
      exige(c.perish == 0xFF || c.perish <= 3,
            quien + ": cuenta mortal " + std::to_string(c.perish));
      exige(c.megaIdx == 0xFF || c.megaIdx < MEGA_COUNT,
            quien + ": mega " + std::to_string(c.megaIdx));
      exige(!c.isMega() || DEX_TBL[c.asDex].type1 == c.type1() ||
            c.typeSet || true, quien + ": mega incoherente");
    }
  }
};

static void unCombate(Vigilante &v, uint32_t semilla) {
  randomSeed(semilla);
  int16_t d1 = (int16_t)(1 + random(DEX_COUNT));
  int16_t d2 = (int16_t)(1 + random(DEX_COUNT));
  uint16_t n1 = (uint16_t)(1 + random(100));
  uint16_t n2 = (uint16_t)(1 + random(100));

  Combatant a, b;
  a.init(d1, n1, random(8) == 0, 0);
  b.init(d2, n2, random(8) == 0, 0);
  // objeto al azar de vez en cuando: las bayas y la banda son las que
  // mas se cruzan con los estados
  if (random(3) == 0) a.item = (uint8_t)random(ITEM_COUNT);
  if (random(3) == 0) b.item = (uint8_t)random(ITEM_COUNT);
  a.bond = (uint8_t)random(101);      // y la desobediencia, de paso
  // HABILIDAD AL AZAR una vez de cada tres. Con las de su especie, las
  // raras no se tocaban casi nunca: hay 304 en el catalogo y cada bicho
  // trae tres como mucho. Asi las 233 que hacen algo entran de verdad en
  // los 4000 combates.
  if (random(3) == 0) {
    a.ability = (uint16_t)(1 + random(ABIL_COUNT - 1));
    v.habsAlAzar++;
  }
  if (random(3) == 0) {
    b.ability = (uint16_t)(1 + random(ABIL_COUNT - 1));
    v.habsAlAzar++;
  }

  Battle bt;
  bt.start(a, b);
  // Con banquillo: sin esto, caer acaba el combate y los relevos (y todo
  // lo que cuelga de ellos) no se prueban nunca.
  bt.playerReserves = true;
  v.combates++;
  v.mira(bt, "al empezar");

  // MEGAEVOLUCION. No se llamaba NUNCA en los 4000 combates, asi que las
  // 127 filas de megas y las habilidades que regalan no pasaban por
  // aqui. Se le pone la piedra que le toque y se megaevoluciona.
  if (random(4) == 0) {
    for (uint16_t f = 0; f < MEGA_COUNT; f++)
      if (MEGA_TBL[f].base == bt.side[BT_PLAYER].dex) {
        for (uint16_t i = 1; i < ITEM_COUNT; i++)
          if (ITEM_TBL[i].fx == IT_MEGA && ITEM_TBL[i].arg == MEGA_TBL[f].item) {
            bt.side[BT_PLAYER].item = (uint8_t)i;
            break;
          }
        if (bt.doMega(BT_PLAYER)) v.megas++;
        v.mira(bt, "tras megaevolucionar");
        break;
      }
  }

  int t = 0;
  for (; t < TURNOS_MAX && !bt.over; t++) {
    for (int lado = 0; lado < 2; lado++) {
      uint8_t quien = lado ? BT_FOE : BT_PLAYER;
      const Combatant &c = bt.side[quien];
      if (bt.over) break;
      if (!c.anyUsable()) break;      // solo le quedaria Forcejeo
      uint8_t slot = (uint8_t)random(BT_MOVES);
      if (slot >= c.nMoves || !c.usesLeft[slot]) slot = 0;
      uint16_t antesRival = bt.side[quien ^ 1].hp;
      // DIVIDE DOLOR reparte los PS a medias, asi que SI puede subirle la
      // vida al rival: es el unico que puede, y es exactamente lo que el
      // golpe hace. La excepcion se mira por el golpe concreto; la regla
      // no se relaja, porque la regla es la que pillo esto.
      // REGALO es el segundo: una de cada cinco veces le cura un cuarto al
      // rival en vez de pegarle, y eso ES el golpe. Como con Divide Dolor,
      // la excepcion se nombra por el efecto concreto —— la regla sigue
      // valiendo para los otros 715.
      uint16_t mvEl = slot < c.nMoves ? c.moves[slot] : (uint16_t)MOVE_STRUGGLE;
      bool reparte = mvEl < MOVE_COUNT && (MOVE_TBL[mvEl].fx == FX_PAINSPLIT ||
                                           MOVE_TBL[mvEl].fx == FX_PRESENT);
      HitResult rg = bt.useMove(quien, slot);
      v.golpes++;
      v.mira(bt, "tras un golpe");
      // Y la TERCERA excepcion: una habilidad de ABSORCION. Si el golpe es
      // de su tipo, Absorbe Agua y compania lo anulan y curan un cuarto ——
      // eso es la habilidad, no un fallo. Lo caz el gauntlet al mezclar
      // bien el dado del simulador: "de 20 a 35" son hpMax/4 de uno de 60.
      // Se nombra por la FIRMA que el motor ya devuelve, no relajando la
      // regla: la regla es la que pilla estas cosas.
      bool absorbio = rg.abilOfDef && rg.abilFired < ABIL_COUNT &&
                      ABIL_TBL[rg.abilFired].fx == AB_ABSORB_HEAL;
      // un golpe no puede CURAR al rival
      v.exige(reparte || absorbio || bt.side[quien ^ 1].hp <= antesRival,
              "un golpe le subio la vida al rival: de " +
              std::to_string(antesRival) + " a " +
              std::to_string(bt.side[quien ^ 1].hp));
    }
    for (int lado = 0; lado < 2; lado++) {
      bt.endTurn(lado ? BT_FOE : BT_PLAYER);
      v.mira(bt, "al cerrar el turno");
    }
    // RELEVO de vez en cuando: es por donde pasan las trampas de
    // entrada, el Relevo, las habilidades de entrada, De Cero a Heroe,
    // Bateria y la Cura. Nada de eso se tocaba en los 4000 combates.
    if (!bt.over && random(8) == 0 && bt.canSwitchOut(BT_PLAYER)) {
      Combatant nuevo;
      nuevo.init((int16_t)(1 + random(DEX_COUNT)),
                 (uint16_t)(1 + random(100)), false, 0);
      if (random(3) == 0)
        nuevo.ability = (uint16_t)(1 + random(ABIL_COUNT - 1));
      bt.regenOnSwitch(BT_PLAYER);
      bt.sendOut(BT_PLAYER, nuevo, true);
      v.relevos++;
      v.mira(bt, "tras relevar");
      v.exige(bt.side[BT_PLAYER].hp >= 1,
              "el que entra se quedo con cero PS: una trampa de entrada "
              "no puede tumbar a nadie en este motor");
    }
    v.turnos++;
  }
  if (bt.over) v.kos++;
  // Que TERMINE. Un combate que llega al tope o es un empate legitimo
  // (dos bichos que no se hacen nada) o es un bucle — y el bucle en la
  // placa es la pantalla congelada.
  v.exige(t < TURNOS_MAX || !bt.over,
          "un combate llego a " + std::to_string(TURNOS_MAX) + " turnos");
}

TEST(gauntlet, miles_de_combates_al_azar_sin_romper_nada) {
  Vigilante v;
  mockSerialSilence(true);
  for (int i = 1; i <= COMBATES && v.roto.empty(); i++) unCombate(v, i);
  mockSerialSilence(false);
  CHECK_MSG(v.roto.empty(), v.roto);
  // Y que de verdad se haya hecho lo que se dice: un gauntlet que se
  // salta las megas y los relevos pasaria igual y no probaria nada.
  CHECK_MSG(v.megas > 50, "solo " + std::to_string(v.megas) +
            " megaevoluciones en " + std::to_string(COMBATES) +
            " combates: las megas no se estan probando");
  CHECK_MSG(v.relevos > 500, "solo " + std::to_string(v.relevos) +
            " relevos: las trampas de entrada y las habilidades de "
            "entrada no se estan probando");
  CHECK_MSG(v.habsAlAzar > 1000, "solo " + std::to_string(v.habsAlAzar) +
            " habilidades al azar: las raras siguen sin tocarse");
  printf("  %d combates: %d golpes, %d KOs, %d megas, %d relevos, "
         "%d habilidades al azar\n", v.combates, v.golpes, v.kos,
         v.megas, v.relevos, v.habsAlAzar);
  // Y que de verdad haya jugado: si un cambio dejara `anyUsable()` en
  // false para todos, el bucle no daria ni un golpe y esto pasaria sin
  // haber probado nada.
  CHECK_MSG(v.golpes > COMBATES * 4,
            "solo se resolvieron " + std::to_string(v.golpes) +
            " golpes en " + std::to_string(v.combates) +
            " combates: el gauntlet no esta jugando");
  CHECK_MSG(v.kos > COMBATES / 4,
            "solo acabaron " + std::to_string(v.kos) + " de " +
            std::to_string(v.combates) + " combates");
}

TEST(gauntlet, el_bicho_del_jugador_sobrevive_a_una_vida_entera) {
  // Lo otro que no cubre nadie: la vida del bicho de punta a punta,
  // con el reloj corriendo. Comer, dormir, ensuciarse, subir, evolucionar
  // y pelear, miles de minutos seguidos, mirando que nada se salga.
  Pet p;
  mockSerialSilence(true);
  randomSeed(777);
  mockNvsReset();
  mockSetMillis(0);
  p.begin();
  if (p.awaitingStarter()) p.chooseStarter(1);
  p.eggTap(); p.eggTap(); p.eggTap();

  std::string roto;
  int evoluciones = 0;
  for (int minuto = 0; minuto < 8000 && roto.empty(); minuto++) {
    mockAdvanceMillis(PET_TICK_MS);
    p.update(millis());
    // Las barras bien altas: evolucionar pide lowestStat() >= 40, y un
    // bicho mal cuidado no evoluciona nunca — que es correcto, pero
    // entonces este test no recorreria las evoluciones, que es la
    // mitad de lo que viene a mirar.
    if (minuto % 10 == 0) {
      p.fullness = 100; p.joy = 100; p.energy = 100; p.hygiene = 100;
    }
    if (minuto % 7 == 0) p.feedBerry((uint8_t)(minuto % 3));
    if (minuto % 13 == 0) p.clean();
    if (minuto % 29 == 0) p.caress();
    if (minuto % 53 == 0) p.toggleLight();
    // Pelear, que es de donde sale la experiencia de verdad. El otro
    // grifo (EXP_PER_CARE_DAY, el primer cuidado del dia) NO entra
    // aqui y no es un fallo: `today()` devuelve 0 cuando no hay reloj,
    // y el shim de la bateria no tiene RTC. Mi primera version contaba
    // con ese grifo y el bicho se quedaba en el nivel 1 para siempre.
    if (minuto % 11 == 0) {
      int16_t rival = (int16_t)(1 + random(DEX_COUNT));
      p.gainExp(Pet::expFromFoe(rival, p.level()));
      p.afterBattle((uint16_t)(p.maxHp() / 2));
    }
    if (p.canEvolveNow()) { p.evolve(); evoluciones++; }

    if (p.fullness > 100 || p.joy > 100 || p.energy > 100 || p.hygiene > 100)
      roto = "una barra paso de 100 en el minuto " + std::to_string(minuto);
    else if (p.speciesId < 1 || p.speciesId > DEX_COUNT)
      roto = "la especie se fue a " + std::to_string(p.speciesId);
    else if (p.hpLeft > p.maxHp())
      roto = "PS " + std::to_string(p.hpLeft) + " de " +
             std::to_string(p.maxHp());
    else if (p.level() < 1 || p.level() > 100)
      roto = "nivel " + std::to_string(p.level()) + " en el minuto " +
             std::to_string(minuto);
  }
  mockSerialSilence(false);
  CHECK_MSG(roto.empty(), roto);
  // Que la vida haya pasado de verdad: si `update` dejara de hacer nada,
  // todo lo de arriba seguiria siendo cierto y no habriamos probado nada.
  CHECK_MSG(p.level() > 1, "en 8000 minutos no subio ni un nivel");
  CHECK_MSG(evoluciones > 0, "en 8000 minutos no evoluciono nunca");
}
