// La bolsa dentro del combate y la captura.
//
// Va aparte de battle_ui.ino porque ese fichero ya carga con las ocho fases
// del combate; esto son cuatro mas y una pantalla propia.
//
// Las dos decisiones que mandan aqui:
//
//  - Las bolas son ESCASAS (no hay tienda: salen del cuidado diario y de lo
//    que sueltan los combates), asi que las probabilidades son ALTAS. La
//    peor sensacion de Pokemon es cruzarte con algo raro y no tener con que
//    cogerlo; si ademas fallaras, seria doble castigo por lo mismo.
//  - Mientras la bola forcejea, TOCAR SUMA. No lo decide todo (CAPT_TAP_MAX
//    lo limita), pero convierte los tres segundos de espera en algo que se
//    hace en vez de algo que se mira.

#include "battle_ui.h"

// lista de la bolsa en combate: mismas filas que la ficha
#define BBAG_ROWS 4
#define BBAG_Y 164
// Las dos pestanas, encima de la lista
#define BTAB_Y 128
#define BTAB_H 28
// LA FAJA DE PASAR PAGINA, debajo de la ultima fila: dos botones grandes,
// subir a la izquierda y bajar a la derecha. Antes eran dos triangulitos
// (el de bajar se tocaba en 66x26 px y el de subir en 66x8) y fallar por
// un pelo cerraba la bolsa.
#define BPAG_Y 326
#define BPAG_H 36
#define BPAG_PASO 3       // filas por toque: se ve una de las de antes
// El panel entero (drawWindow en btRenderBag): fuera de el es "fuera".
#define BPAN_X0 60
#define BPAN_X1 406
#define BPAN_Y0 96
#define BPAN_Y1 396
#define BBAG_H 40
#define BBAG_X 70
#define BBAG_W 326
// boton de la bolsa, a la derecha de la tarjeta de golpe
#define BBAG_BTN_X 400
#define BBAG_BTN_Y 296
#define BBAG_BTN_R 27

// Que se puede usar aqui: lo de combate y las bolas. Las piedras y las
// llaves no pintan nada en mitad de una pelea.
// Lo que se puede usar EN COMBATE y esta en el bolsillo que se mira.
// Antes los dos bolsillos salian en la misma lista: con cinco filas y sin
// forma de bajar, quien llevara media docena de pociones no llegaba nunca
// a sus bolas.
static uint8_t btBagQtd(uint8_t id) {
  return bt.pvp ? pvpBolsa[id] : pet.bag[id];   // entre placas: a copia
}

static bool btUsableAqui(uint8_t id) {
  if (id == 0 || id >= ITEM_COUNT) return false;
  if (!btBagQtd(id)) return false;
  // Entre placas, so o bolso de cura e das X: nada de bolas.
  if (bt.pvp) return ITEM_TBL[id].pocket == BP_BATTLE &&
                     (ITEM_TBL[id].use == IU_BATTLER || ITEM_TBL[id].use == IU_POKEMON);
  // Lo que NO se puede usar en combate se esconde, en vez de dejarlo
  // tocar para que pite: una lista que solo ofrece lo que funciona se
  // lee mas rapido que una llena de filas muertas.
  //
  // Quien entra y quien no lo dice el PBS del Anil en `BattleUse`, no
  // una lista escrita aqui. Antes era un `fx == IT_VITAMIN` a mano, que
  // acertaba por casualidad: acertaba con las vitaminas porque son las
  // unicas que hoy tenemos sin uso en combate, y se habria quedado
  // viejo con el primer objeto nuevo que tampoco valga.
  if (ITEM_TBL[id].use == IU_NONE) return false;
  uint8_t p = ITEM_TBL[id].pocket;
  return p == (bt.bagPocket ? (uint8_t)BP_BALL : (uint8_t)BP_BATTLE);
}

static uint8_t btBagCount() {
  uint8_t n = 0;
  for (uint8_t i = 1; i < ITEM_COUNT; i++) if (btUsableAqui(i)) n++;
  return n;
}

// Cuantos hay en el OTRO bolsillo, para poder decirlo en su pestana.
static uint8_t btBagCountPocket(uint8_t pocket) {
  uint8_t n = 0;
  for (uint8_t i = 1; i < ITEM_COUNT; i++)
    if (pet.bag[i] && ITEM_TBL[i].pocket == pocket) n++;
  return n;
}

static uint8_t btBagAt(uint8_t idx) {
  uint8_t n = 0;
  for (uint8_t i = 1; i < ITEM_COUNT; i++) {
    if (!btUsableAqui(i)) continue;
    if (n == idx) return i;
    n++;
  }
  return 0;
}

void btOpenBag() {
  bt.phase = BP_BAG;
  bt.bagScroll = 0;
  bt.bagPocket = 0;
  bt.itemPick = 0;     // la bolsa se abre con las manos vacias
  bt.msg[0] = 0;
}

// Un objeto usado EN COMBATE va sobre el que esta EN PISTA.
//
// Antes esto lo hacia `Pet::useItem`, y por eso la bolsa no servia de
// nada. Esa funcion mira `pet.hpLeft`, que durante el combate esta
// CONGELADO —— solo se escribe al salir de pista (btStoreActive). Asi que
// con medio corazon en pantalla veia a uno entero, decia "no haria nada"
// y pitaba. Y si el que peleaba era un suplente, curaba al principal y
// despues le copiaba su vida encima al suplente.
//
// Las reglas salen del Anil (Data/Scripts/041_Items/004_Item_BattleEffects.rb),
// que es la autoridad de este port y no los juegos oficiales.
//
// O EFEITO vive em btItemNoCombatente, que nao sabe de bolsa nem de save:
// e o mesmo que as duas placas aplicam entre placas (no que esta na
// pista ou num do banco, que la tambem e um Combatant).
bool btItemNoCombatente(Combatant &c, uint8_t id, bool aplicar) {
  if (id == 0 || id >= ITEM_COUNT) return false;
  const ItemEntry &it = ITEM_TBL[id];
  bool sirvio = false;
  // Num caido so o Revivir entra (pbCanUseOnPokemon do Anil).
  if (c.hp == 0 && it.fx != IT_REVIVE) return false;

  switch (it.fx) {
    case IT_POTION: {
      if (!c.hpMax || c.hp == 0 || c.hp >= c.hpMax) break;
      if (!aplicar) { sirvio = true; break; }
      uint16_t cura = (it.arg == 255) ? c.hpMax : it.arg;
      uint32_t n = (uint32_t)c.hp + cura;
      c.hp = (n > c.hpMax) ? c.hpMax : (uint16_t)n;
      sirvio = true;
      break;
    }
    case IT_FULLREST:
      // Vale si falta vida O hay estado O hay confusion, los tres.
      if (!c.hpMax) break;
      if (c.hp >= c.hpMax && c.status == ST_NONE && c.confuseTurns == 0) break;
      if (!aplicar) { sirvio = true; break; }
      c.hp = c.hpMax;
      c.status = ST_NONE;
      c.statusTurns = 0;
      c.confuseTurns = 0;
      sirvio = true;
      break;
    case IT_CURE:
      if (it.arg == 0) {
        // Full Heal: tambien la confusion. En el Anil su
        // BattleUseOnPokemon llama a pbCureStatus Y a pbCureConfusion.
        if (c.status == ST_NONE && c.confuseTurns == 0) break;
        if (!aplicar) { sirvio = true; break; }
        c.confuseTurns = 0;
      } else {
        if (c.status != it.arg) break;
        if (!aplicar) { sirvio = true; break; }
      }
      c.status = ST_NONE;
      c.statusTurns = 0;
      sirvio = true;
      break;
    case IT_REVIVE:
      // El de pista esta en pie por definicion (la bolsa solo se abre
      // desde BP_CHOOSE). Entre placas los del banco tambien son
      // Combatant, y un caido de ahi SI se levanta: PS en `arg` %.
      if (!c.hpMax || c.hp != 0) break;
      if (!aplicar) { sirvio = true; break; }
      c.hp = (uint16_t)((uint32_t)c.hpMax * it.arg / 100);
      if (!c.hp) c.hp = 1;
      c.status = ST_NONE;
      c.statusTurns = 0;
      sirvio = true;
      break;
    case IT_ETHER: {
      // Al golpe MAS gastado, que es donde hace falta.
      uint8_t peor = 0xFF;
      uint16_t falta = 0;
      for (uint8_t i = 0; i < c.nMoves && i < BT_MOVES; i++) {
        uint8_t tope = movePP(c.moves[i]);
        if (c.usesLeft[i] >= tope) continue;
        uint16_t d = (uint16_t)(tope - c.usesLeft[i]);
        if (d > falta) { falta = d; peor = i; }
      }
      if (peor == 0xFF) break;
      if (!aplicar) { sirvio = true; break; }
      uint8_t tope = movePP(c.moves[peor]);
      uint16_t n = (uint16_t)c.usesLeft[peor] + (it.arg == 255 ? tope : it.arg);
      c.usesLeft[peor] = (n > tope) ? tope : (uint8_t)n;
      sirvio = true;
      break;
    }
    case IT_ELIXIR:
      for (uint8_t i = 0; i < c.nMoves && i < BT_MOVES; i++) {
        uint8_t tope = movePP(c.moves[i]);
        if (c.usesLeft[i] >= tope) continue;
        sirvio = true;
        if (!aplicar) break;
        uint16_t n = (uint16_t)c.usesLeft[i] + (it.arg == 255 ? tope : it.arg);
        c.usesLeft[i] = (n > tope) ? tope : (uint8_t)n;
      }
      break;
    case IT_XSTAT:
      // DOS escalones y no uno: X_STAT_ITEMS_RAISE_BY_TWO_STAGES es
      // (MECHANICS_GENERATION >= 7) y el Anil va por la 9.
      if (it.arg >= SG_COUNT || c.stage[it.arg] >= 6) break;
      if (!aplicar) { sirvio = true; break; }
      sirvio = c.applyStage(it.arg, 2);
      break;
    case IT_XCRIT:
      // Dire Hit FIJA el escalon en 2 (effects[FocusEnergy] = 2 en el
      // Anil), no suma: darlo dos veces no apila.
      if (c.critStage >= 2) break;
      if (!aplicar) { sirvio = true; break; }
      c.critStage = 2;
      sirvio = true;
      break;
    default:
      break;
  }
  return sirvio;
}

static bool btItemEnPista(uint8_t id, bool aplicar) {
  if (id == 0 || id >= ITEM_COUNT || !pet.bag[id]) return false;
  Combatant &c = bt.b.side[BT_PLAYER];
  if (!btItemNoCombatente(c, id, aplicar)) return false;
  if (!aplicar) return true;       // solo se estaba preguntando
  // Con el principal en pista, que lo guardado no se quede atras: es lo
  // que mira la ficha y lo que se graba si el cacharro se apaga aqui.
  if (btIsMain(bt.mySlot)) pet.hpLeft = c.hp;
  pet.spendItem(id);
  return true;
}

// UN OBJETO SOBRE UNO QUE NO ESTA EN PISTA. El de pista vive en el
// Combatant (btItemEnPista); estos viven en `pet` (si el principal esta
// en el banquillo) o en `pet.team[slot]`.
//
// Lo que se puede y lo que no sale de lo que el port GUARDA de un bicho
// parado: PS y PP si, estado NO —— un StoredMon no tiene campo de
// estado, porque fuera del combate aqui nadie esta envenenado. Asi que
// una cura de estado sobre uno del banquillo no hace nada y la lista lo
// ensena apagado, en vez de dejar gastarla para nada.
static bool btItemEnGuardado(uint8_t id, uint8_t slot, bool aplicar) {
  if (slot >= TEAM_SLOTS) return false;
  const ItemEntry &it = ITEM_TBL[id];
  bool main = btIsMain(slot);
  if (main ? pet.isEgg() : !pet.team[slot].used()) return false;

  // Los PS: del Pet si es el principal, del hueco si no. Se trabaja
  // con punteros para no escribir cada caso dos veces.
  StoredMon *m = main ? nullptr : &pet.team[slot];
  uint16_t tope = main ? pet.maxHp()
                       : hpAtLevelForm(m->species, Pet::levelOf(*m), m->trHp, m->form);
  uint16_t vida = main ? pet.hpLeft : m->hpLeft;
  uint8_t *pp = main ? pet.ppLeft : m->ppLeft;
  uint16_t *gol = main ? pet.ppMove : nullptr;
  bool sirvio = false;

  switch (it.fx) {
    case IT_POTION:
    case IT_FULLREST: {
      // A un CAIDO no le entra una pocion: para eso esta el Revivir, y
      // es la misma regla del Anil (pbCanUseOnPokemon mira fainted?).
      if (!tope || vida == 0 || vida >= tope) break;
      if (!aplicar) { sirvio = true; break; }
      uint32_t n = (it.fx == IT_FULLREST || it.arg == 255)
                       ? tope
                       : (uint32_t)vida + it.arg;
      vida = (n > tope) ? tope : (uint16_t)n;
      sirvio = true;
      break;
    }
    case IT_REVIVE:
      if (vida != 0) break;            // solo a los caidos
      if (!aplicar) { sirvio = true; break; }
      vida = (uint16_t)((uint32_t)tope * it.arg / 100);
      if (!vida) vida = 1;
      sirvio = true;
      break;
    case IT_CURE:
      // Un bicho parado no esta envenenado: no se guarda el estado.
      break;
    case IT_ETHER: {
      // Al golpe MAS gastado, igual que en pista.
      uint8_t peor = 0xFF;
      uint16_t falta = 0;
      for (uint8_t i = 0; i < 4; i++) {
        uint16_t mv = main ? gol[i] : m->moves[i];
        if (!mv || mv >= MOVE_COUNT) continue;
        uint8_t max = movePP(mv);
        if (pp[i] >= max) continue;
        uint16_t d = (uint16_t)(max - pp[i]);
        if (d > falta) { falta = d; peor = i; }
      }
      if (peor == 0xFF) break;
      if (!aplicar) { sirvio = true; break; }
      uint16_t mv = main ? gol[peor] : m->moves[peor];
      uint8_t max = movePP(mv);
      uint16_t n = (uint16_t)pp[peor] + (it.arg == 255 ? max : it.arg);
      pp[peor] = (n > max) ? max : (uint8_t)n;
      sirvio = true;
      break;
    }
    case IT_ELIXIR:
      for (uint8_t i = 0; i < 4; i++) {
        uint16_t mv = main ? gol[i] : m->moves[i];
        if (!mv || mv >= MOVE_COUNT) continue;
        uint8_t max = movePP(mv);
        if (pp[i] >= max) continue;
        sirvio = true;
        if (!aplicar) break;
        uint16_t n = (uint16_t)pp[i] + (it.arg == 255 ? max : it.arg);
        pp[i] = (n > max) ? max : (uint8_t)n;
      }
      break;
    default:
      // Las X y el Mas Psique suben ESCALONES, y los escalones son del
      // combate: no existen fuera de la pista. Su BattleUse en el PBS
      // es OnBattler justamente por eso, y por ahi no llegan aqui.
      break;
  }

  if (!sirvio) return false;
  if (!aplicar) return true;
  if (main) pet.hpLeft = vida; else m->hpLeft = vida;
  pet.spendItem(id);
  return true;
}

// EL DESPACHO: el hueco que esta en pista va por el Combatant, y el
// resto por lo guardado. Un solo sitio, para que la lista del equipo y
// el toque no puedan discrepar.
bool btItemEnSlot(uint8_t id, uint8_t slot, bool aplicar) {
  // Entre placas so se pergunta (a lista do time); aplicar e no turno.
  if (bt.pvp) return !aplicar && pvpItemServe(pvpEu, id, slot);
  if (id == 0 || id >= ITEM_COUNT || !pet.bag[id]) return false;
  if (slot == bt.mySlot) return btItemEnPista(id, aplicar);
  return btItemEnGuardado(id, slot, aplicar);
}

// Y EL TOQUE sobre la lista cuando se lleva un objeto en la mano. Es
// hermano de btPartyTap, pero no relevar: aplicar.
bool btItemPickTap(int16_t x, int16_t y) {
  uint8_t id = bt.itemPick;
  for (uint8_t i = 0; i < TEAM_SLOTS; i++) {
    int ry = BPTY_Y + i * BPTY_H;
    if (x < BPTY_X || x > BPTY_X + BPTY_W || y < ry || y > ry + BPTY_H - 6)
      continue;
    if (!btItemEnSlot(id, i, false)) { sfxPlay(SFX_DENY); return true; }
    char quien[16];
    snprintf(quien, sizeof(quien), "%s",
             btIsMain(i) ? dexName(pet.speciesId) : dexName(pet.team[i].species));
    btItemEnSlot(id, i, true);
    // Si se curo al que esta EN PISTA, su Combatant ya lo sabe; si fue
    // a uno del banquillo, lo guardado tambien. No hay nada que copiar
    // de un lado al otro —— eso era justo el fallo viejo de la bolsa.
    snprintf(bt.msg, sizeof(bt.msg), T(S_BM_USED_ON), itemName(id), quien);
    bt.itemPick = 0;
    sfxPlay(SFX_EAT);
    // Usar un objeto cuesta el turno, como en los juegos.
    bt.phase = BP_MSG;
    bt.extraStep = 9;
    bt.endStep = 0;
    bt.until = millis() + 900;
    bt.acting = BT_PLAYER;
    bt.secondPending = true;
    bt.foeSlot = bt.b.pickFoeMove();
    bt.b.inicioDeRondaRival(bt.foeSlot);   // el jugador gasto la ronda en otra cosa
    return true;
  }
  // Fuera del PANEL (y lejos): se suelta el objeto y se vuelve a la bolsa,
  // que es de donde se vino. Volver al menu de golpes seria perder la
  // bolsa abierta por tocar al lado. Entre dos filas no pasa nada.
  if (lejosDe(x, y, BPTY_PAN_X0, BPTY_PAN_Y0, BPTY_PAN_X1, BPTY_PAN_Y1)) {
    bt.itemPick = 0;
    bt.phase = BP_BAG;
  }
  return true;
}

// el boton redondo de la bolsa (solo mientras se elige)
void btDrawBagButton() {
  // entre placas, apagada quando a regra ja nao deixa (as 3 gastas)
  bool apagada = bt.pvp && !pvpBolsaPode();
  gfx->fillCircle(BBAG_BTN_X, BBAG_BTN_Y, BBAG_BTN_R, apagada ? UI_TRACK : UI_WHITE);
  gfx->drawCircle(BBAG_BTN_X, BBAG_BTN_Y, BBAG_BTN_R, INK_K);
  // La mochila del jugador (UI/Bag/bag_1.png del Anil). La de antes eran
  // dos rectangulos marrones y se leia como una maleta de carton.
  drawArt(ART_BAG, BBAG_BTN_X, BBAG_BTN_Y + ART_BAG.h / 2, 1);
}

// El boton de MEGAEVOLUCIONAR. Va a la IZQUIERDA, espejado del de equipo:
// la carta de golpe ocupa el centro (x 97..369) y los otros dos botones el
// lado derecho, asi que el hueco libre es este.
//
// Solo sale cuando de verdad se puede — el bicho lleva SU piedra y todavia
// no la ha usado en este combate. Un boton gris que no hace nada seria
// peor que no tenerlo: con 122 piedras, la mayoria de los combates no
// tienen nada que megaevolucionar.
// EN LA COLUMNA DE LA IZQUIERDA, encima del boton de huir. Estaba a la
// derecha, en (466-50, 316), y la mochila esta en (400,296) con el mismo
// radio 27: los dos circulos se cruzan. Y no era solo feo —— battleTap
// mira la mochila ANTES que la mega, asi que en la zona comun el toque
// se lo quedaba la bolsa y la mega no habia forma de pulsarla.
//
// La columna queda 188 / 252 / 316 (mega, huir, equipo): 64 px entre
// centros con radio 27, no se tocan. Y la mochila se queda sola.
#define BMEG_BTN_X BPTY_BTN_X
#define BMEG_BTN_Y 188
#define BMEG_BTN_R BPTY_BTN_R
// BT_MEGA_MS se mudo a battle_ui.h: battle_ui.ino se concatena ANTES
// que este fichero y ahora tambien lo necesita —— la mega del RIVAL se
// arranca desde battleTap(), que vive alli.

void btDrawMegaButton() {
  if (bt.b.canMega(btAbajo()) < 0) return;
  // late: es la decision gorda del combate y tiene que pedirse a gritos
  bool on = bt.pvp ? bt.pvpMega : ((millis() / 420) & 1);
  uint16_t fondo = on ? C565(0xf8, 0xd8, 0x68) : UI_WHITE;
  gfx->fillCircle(BMEG_BTN_X, BMEG_BTN_Y, BMEG_BTN_R, fondo);
  gfx->drawCircle(BMEG_BTN_X, BMEG_BTN_Y, BMEG_BTN_R, INK_K);
  // El sello de la megaevolucion DE VERDAD: el orbe con la helice, que
  // sale de Pictures/megaevolucion.png del Anil. Antes era un rombo
  // partido en dos tonos dibujado a mano, que no era el simbolo de nada.
  drawArt(ART_MEGA, BMEG_BTN_X, BMEG_BTN_Y + ART_MEGA.h / 2, 1);
}

// LA ESCENA DE LA MEGA. Tres anillos que salen del bicho en los dos
// colores del sello, desfasados, y el rombo creciendo en el centro. Se
// dibuja DESPUES de los bichos y antes de las chapas de PS, como el
// destello de los golpes: envuelve al bicho sin taparle la barra.
void btMegaFx(uint32_t now) {
  if (bt.phase != BP_MEGA) return;
  float u = (float)(now - bt.megaAt) / (float)BT_MEGA_MS;
  if (u < 0.0f) u = 0.0f;
  if (u > 1.0f) u = 1.0f;
  // Sobre el que megaevoluciona, que no siempre soy yo. Los anillos
  // estaban clavados en mi plataforma porque el rival no llegaba a esta
  // fase: se transformaba de golpe, sin escena. El suyo pisa mas arriba
  // y se dibuja mas pequeno, asi que el centro sale de SU suelo.
  const bool mio = (bt.megaSide == btAbajo());
  const int cx = mio ? 145 : BT_FOE_CX;
  const int cy = (mio ? BT_GROUND_MINE - 70 : BT_GROUND_FOE - 50);
  for (int k = 0; k < 3; k++) {
    float p = u * 2.2f - (float)k * 0.45f;
    if (p <= 0.0f || p >= 1.0f) continue;
    int r = (int)(18.0f + p * 105.0f);
    uint16_t col = (k & 1) ? C565(0x4a, 0x8d, 0xd4) : C565(0xd4, 0x52, 0x7e);
    gfx->drawCircle(cx, cy, r, col);
    gfx->drawCircle(cx, cy, r - 1, col);
  }
  // Y el sello creciendo en el centro. El mismo de Anil que lleva el
  // boton, en su version grande (88 px de origen) y ampliado por regla de
  // tres: de 16 a 126 px de ancho a lo largo de la escena. Con escalas
  // ENTERAS esto serian tres saltos, que es justo lo que no queremos.
  drawArtZoom(ART_MEGA_BIG, cx, cy, 16 + (int)(u * 110.0f));
}

// El boton de HUIR. Va abajo a la izquierda, espejado del de la bolsa:
// los cuatro botones quedan en las esquinas de la tarjeta de golpe.
//
// Solo sale cuando se puede huir de verdad (salvaje, en pie, sin estar
// atrapado). Con un entrenador delante no aparece, que es mas claro
// que un boton gris: la respuesta a "¿por que no puedo?" la da el no
// verlo donde los otros combates si lo tienen.
// El de HUIR ya no es el espejo del de la bolsa, y el motivo es que los
// dos lados NO tienen lo mismo debajo. El bicho del jugador se dibuja
// centrado en x=145 con los pies en y=312, asi que un boton espejado en
// (66, 296) le caia encima: con la placa en la mano se veia el circulo
// pisando al Bulbasaur. El rival esta mucho mas arriba (y=178), asi que
// el de la bolsa no tiene ese problema y se queda donde estaba.
//
// (50, 252) esta a 184 px del centro: con el radio de 27 quedan 22 px
// hasta el borde de la pantalla redonda. Y la chapa de PS del jugador
// vive en la DERECHA (x 272..454), asi que esta franja esta libre.
#define BRUN_BTN_X 50
#define BRUN_BTN_Y 252
#define BRUN_BTN_R BBAG_BTN_R

void btDrawRunButton() {
  // Entre placas a bandeira e DESISTIR: o 1o toque arma (fica vermelha e
  // a caixa pede o 2o), o 2o confirma.
  if (bt.pvp) {
    if (pvpAcabou) return;
    gfx->fillCircle(BRUN_BTN_X, BRUN_BTN_Y, BRUN_BTN_R, pvpDesistirAte ? UI_BAR_BAD : UI_WHITE);
    gfx->drawCircle(BRUN_BTN_X, BRUN_BTN_Y, BRUN_BTN_R, INK_K);
    drawArt(ART_FLEE, BRUN_BTN_X, BRUN_BTN_Y + ART_FLEE.h / 2, 1);
    return;
  }
  if (!bt.b.canFlee()) return;
  gfx->fillCircle(BRUN_BTN_X, BRUN_BTN_Y, BRUN_BTN_R, UI_WHITE);
  gfx->drawCircle(BRUN_BTN_X, BRUN_BTN_Y, BRUN_BTN_R, INK_K);
  // Bandera blanca (Icons/menuQuit.png del Anil): "me rindo, me voy" se
  // entiende sin leer nada. Antes era un triangulo azul con tres rayas,
  // que lo mismo podia ser "atras" que "rebobinar".
  drawArt(ART_FLEE, BRUN_BTN_X, BRUN_BTN_Y + ART_FLEE.h / 2, 1);
}

// A bandeira entre placas. Com ela armada, um toque fora dela so desarma
// (a caixa da pergunta tapa a carta de golpe: tocar ali nao pode escolher
// um golpe que nao se ve).
bool pvpDesistirTap(int x, int y) {
  if (!bt.pvp || pvpAcabou || bt.phase != BP_CHOOSE) return false;
  int dx = x - BRUN_BTN_X, dy = y - BRUN_BTN_Y;
  bool naBandeira = dx * dx + dy * dy <= (BRUN_BTN_R + 8) * (BRUN_BTN_R + 8);
  uint32_t now = millis();
  bool armada = pvpDesistirAte && (int32_t)(now - pvpDesistirAte) < 0;
  if (!naBandeira) {
    if (!armada) return false;
    pvpDesistirAte = 0;
    bt.msg[0] = 0;
    sfxPlay(SFX_TAP);
    return true;
  }
  if (!armada) {
    pvpDesistirAte = now + 3000;
    snprintf(bt.msg, sizeof(bt.msg), T(S_BM_GIVE_UP_TAP_THE));
    sfxPlay(SFX_DENY);
    return true;
  }
  pvpDesistirAte = 0;
  sfxPlay(SFX_TAP);
  if (pvpAoDesistir) pvpAoDesistir();
  pvpInterrompe(T(S_BM_YOU_GAVE_UP), (uint8_t)(pvpEu ^ 1));
  return true;
}

bool btRunTap(int x, int y) {
  if (bt.phase != BP_CHOOSE || !bt.b.canFlee()) return false;
  int dx = x - BRUN_BTN_X, dy = y - BRUN_BTN_Y;
  if (dx * dx + dy * dy > (BRUN_BTN_R + 8) * (BRUN_BTN_R + 8)) return false;
  if (bt.b.tryFlee()) {
    sfxPlay(SFX_TAP);
    snprintf(bt.msg, sizeof(bt.msg), T(S_BM_GOT_AWAY_SAFELY));
    bt.phase = BP_END;
    bt.until = millis() + 1200;
  } else {
    // Fallar CUESTA EL TURNO: si no, huir seria gratis y no habria
    // decision. El rival pega mientras te das la vuelta.
    sfxPlay(SFX_DENY);
    snprintf(bt.msg, sizeof(bt.msg), T(S_BM_COULDN_T_GET_AWAY));
    bt.foeSlot = bt.b.pickFoeMove();
    bt.b.inicioDeRondaRival(bt.foeSlot);   // el jugador gasto la ronda en otra cosa
    bt.secondPending = false;
    btLaunch(BT_FOE, bt.foeSlot);
  }
  return true;
}

// true si el toque era suyo.
bool btMegaTap(int x, int y) {
  if (bt.phase != BP_CHOOSE || bt.b.canMega(bt.pvp ? pvpEu : BT_PLAYER) < 0) return false;
  int dx = x - BMEG_BTN_X, dy = y - BMEG_BTN_Y;
  if (dx * dx + dy * dy > (BMEG_BTN_R + 8) * (BMEG_BTN_R + 8)) return false;
  // Entre placas so se MARCA: a mega acontece no comeco do turno, na fila,
  // quando as duas jogadas estiverem na mesa.
  if (bt.pvp) {
    bt.pvpMega = !bt.pvpMega;
    sfxPlay(SFX_TAP);
    return true;
  }
  // Aqui solo se ARRANCA la escena. La transformacion se hace cuando
  // acaba (ver BP_MEGA en el tick): antes se hacia todo en este mismo
  // instante y la decision mas gorda del combate duraba cero.
  bt.phase = BP_MEGA;
  bt.megaSide = BT_PLAYER;     // la escena es MIA; la del rival va aparte
  bt.megaAt = millis();
  bt.until = bt.megaAt + BT_MEGA_MS;
  bt.msg[0] = 0;
  // La fanfarria de mega del Anil. Si no hay tarjeta no hay fanfarrias,
  // y entonces cae al pitido de siempre: la decision mas gorda del
  // combate no se puede quedar MUDA porque falte una microSD.
  if (!jinglePlay(JIN_MEGA)) sfxPlay(SFX_MEDAL);
  return true;
}

void btRenderBag() {
  // la misma caja de HGSS que el resto de los dialogos, no un panel beige
  drawWindow(60, 96, 346, 300, 16);
  // El titulo va DENTRO del panel: a la altura de antes caia encima de la
  // barra de PS del rival y se leian los dos a la vez.
  gfx->setTextColor(UI_INK);
  setSize(2);
  setCur(centerX(T(S_CARD_BAG), 2), 108);
  printT(T(S_CARD_BAG));

  // Las dos pestanas, con cuantos hay en cada una: asi se ve que hay algo
  // en la otra sin tener que entrar a mirar.
  if (bt.pvp) {
    // Entre placas: so a cura, e quanto a regra ainda deixa.
    char et[48];
    if (DR_ITENS(pvpRegras) == DI_ATE3) {
      uint8_t u = pvpL[pvpEu].itensUsados;
      snprintf(et, sizeof(et), T(S_RESTAM_FMT), (unsigned)(u < 3 ? 3 - u : 0));
    } else {
      snprintf(et, sizeof(et), "%s", T(S_LIVRE_SAVE));
    }
    gfx->fillRoundRect(72, BTAB_Y, 316, BTAB_H, 8, UI_INK);
    gfx->setTextColor(UI_WHITE);
    setSize(2);
    setCur(72 + (316 - textW(et, 2)) / 2, BTAB_Y + 7);
    printT(et);
  }
  for (uint8_t t = 0; t < 2 && !bt.pvp; t++) {
    int tx = 72 + t * 162;
    bool act = (bt.bagPocket == t);
    gfx->fillRoundRect(tx, BTAB_Y, 154, BTAB_H, 8, act ? UI_INK : UI_TRACK);
    gfx->setTextColor(UI_WHITE);
    char et[32];
    snprintf(et, sizeof(et), "%s %u", t ? T(S_BOLAS) : T(S_CURA),
             (unsigned)btBagCountPocket(t ? BP_BALL : BP_BATTLE));
    setSize(2);
    setCur(tx + (154 - textW(et, 2)) / 2, BTAB_Y + 7);
    printT(et);
  }

  uint8_t n = btBagCount();
  if (!n) {
    gfx->setTextColor(UI_TRACK);
    setSize(2);
    setCur(centerX(T(S_NADA_AQUI), 2), 240);
    printT(T(S_NADA_AQUI));
    gfx->setTextColor(UI_TRACK);
    setSize(1);
    setCur(centerX(T(S_FORA_VOLTAR), 1), 372);
    printT(T(S_FORA_VOLTAR));
    return;
  }
  if (bt.bagScroll + BBAG_ROWS > n)
    bt.bagScroll = (uint8_t)(n > BBAG_ROWS ? n - BBAG_ROWS : 0);

  for (uint8_t r = 0; r < BBAG_ROWS; r++) {
    uint8_t i = (uint8_t)(bt.bagScroll + r);
    if (i >= n) break;
    uint8_t id = btBagAt(i);
    int y = BBAG_Y + r * BBAG_H;
    bool bola = (ITEM_TBL[id].pocket == BP_BALL);
    gfx->fillRoundRect(BBAG_X + 8, y, BBAG_W - 16, 34, 8,
                       bola ? C565(0xf2, 0xd9, 0xd9) : UI_WHITE);
    gfx->drawRoundRect(BBAG_X + 8, y, BBAG_W - 16, 34, 8, UI_TRACK);
    if (ART_ITEM[id]) drawArt(*ART_ITEM[id], BBAG_X + 32, y + 17 + ART_ITEM[id]->h / 2, 1);
    gfx->setTextColor(UI_INK);
    setSize(2);
    setCur(BBAG_X + 54, y + 9);
    printT(itemName(id));
    char c[8];
    snprintf(c, sizeof(c), "x%u", btBagQtd(id));
    setSize(1);
    setCur(BBAG_X + BBAG_W - 26 - (int)strlen(c) * LATIN_ADV, y + 13);
    printT(c);
  }
  // PASAR PAGINA: dos botones que ocupan la faja entera, con cuantos quedan
  // en cada sentido. Solo salen si la lista no cabe; el que no tiene a
  // donde ir se pinta apagado (y tocarlo no hace nada: no cierra).
  if (n > BBAG_ROWS) {
    uint8_t arriba = bt.bagScroll;
    uint8_t abajo = (uint8_t)(n - bt.bagScroll - BBAG_ROWS);
    for (uint8_t k = 0; k < 2; k++) {
      uint8_t quedan = k ? abajo : arriba;
      int bx = k ? 238 : 78;
      gfx->fillRoundRect(bx, BPAG_Y, 150, BPAG_H, 10, quedan ? UI_INK : UI_TRACK);
      int cx = bx + 56, cy = BPAG_Y + BPAG_H / 2;
      if (k) gfx->fillTriangle(cx, cy + 9, cx - 16, cy - 8, cx + 16, cy - 8, UI_WHITE);
      else   gfx->fillTriangle(cx, cy - 9, cx - 16, cy + 8, cx + 16, cy + 8, UI_WHITE);
      if (quedan) {
        char q[8];
        snprintf(q, sizeof(q), "%u", (unsigned)quedan);
        gfx->setTextColor(UI_WHITE);
        setSize(2);
        setCur(bx + 84, BPAG_Y + 11);
        printT(q);
      }
    }
  }

  gfx->setTextColor(UI_TRACK);
  setSize(1);
  setCur(centerX(T(S_FORA_VOLTAR), 1), 372);
  printT(T(S_FORA_VOLTAR));
}

// ---------- la bola ----------

// O multiplicador (x10) da bola `id` contra o rival de AGORA. As de
// escalao trazem o numero no `arg`; as de condicao (IT_BALLX) dependem do
// combate, e as regras sao as dos jogos, com o que este jogo tem:
//   Quick   x5 se ninguem se mexeu ainda (o primeiro turno)
//   Timer   +0,3 por rodada, ate x4
//   Dusk    x3 de noite (20h-6h, o mesmo relogio dos salvajes noturnos)
//   Net     x3,5 em Inseto e Agua
//   Nest    (41 - nivel)/10 abaixo do 31; depois x1
//   Repeat  x3,5 se a especie ja esta na sua Pokedex
//   Level   x2 / x4 / x8 com o seu nivel acima / 2x / 4x o dele
//   Fast    x4 com velocidade base 100 ou mais
//   Heavy   x1,5 de 100 kg, x2 de 200 kg, x3 de 300 kg
//   Moon    x4 em quem evolui com a Moon Stone
//   Dream   x4 no adormecido
//   Friend, Luxury, Premier: x1 (o que elas fazem e depois de capturar)
uint8_t ballMult(uint8_t id) {
  if (id >= ITEM_COUNT) return 10;
  const ItemEntry &it = ITEM_TBL[id];
  if (it.fx != IT_BALLX) return it.arg ? it.arg : 10;
  const Combatant &foe = bt.b.side[BT_FOE];
  const DexEntry &d = DEX_TBL[foe.dex];
  switch (it.arg) {
    case BALLK_QUICK:
      return bt.b.turn == 0 ? 50 : 10;
    case BALLK_TIMER: {
      // `turn` conta GOLPES (dois por rodada), nao rodadas.
      uint16_t r = (uint16_t)(10 + 3 * (bt.b.turn / 2));
      return (uint8_t)(r > 40 ? 40 : r);
    }
    case BALLK_DUSK: {
      uint8_t h = (uint8_t)(pet.lastSeenEpoch ? (pet.lastSeenEpoch / 3600) % 24 : 13);
      return (h < 6 || h >= 20) ? 30 : 10;
    }
    case BALLK_NET:
      return (foe.type1() == T_BUG || foe.type2() == T_BUG ||
              foe.type1() == T_WATER || foe.type2() == T_WATER) ? 35 : 10;
    case BALLK_NEST:
      return foe.level < 31 ? (uint8_t)(41 - foe.level) : 10;
    case BALLK_REPEAT:
      return pet.isRegistered(foe.dex) ? 35 : 10;
    case BALLK_LEVEL: {
      uint16_t meu = bt.b.side[BT_PLAYER].level, dele = foe.level;
      return meu >= 4 * dele ? 80 : meu >= 2 * dele ? 40 : meu > dele ? 20 : 10;
    }
    case BALLK_FAST:
      return d.bSpe >= 100 ? 40 : 10;
    case BALLK_HEAVY:     // weight10 e o peso em decenas de quilo
      return d.weight10 >= 30 ? 30 : d.weight10 >= 20 ? 20 : d.weight10 >= 10 ? 15 : 10;
    case BALLK_MOON:
      return (stoneTarget(foe.dex, T_FAIRY, true) > 0 ||
              stoneTarget(foe.dex, T_FAIRY, false) > 0) ? 40 : 10;
    case BALLK_DREAM:
      return foe.status == ST_SLEEP ? 40 : 10;
    default:
      return 10;
  }
}

// Em que posicao de ART_BALL esta a arte da bola `id` (0 = Poke Ball).
uint8_t ballArte(uint8_t id) {
  for (uint8_t i = 0; i < BALL_ALL_COUNT; i++)
    if (BALL_ALL[i] == id) return i;
  return 0;
}

static void btThrowBall(uint8_t id) {
  const Combatant &foe = bt.b.side[BT_FOE];
  bt.ballId = id;
  bt.taps = 0;
  // O multiplicador: o `arg` nas de escalao, a regra da bola nas outras.
  bt.chance = captureChance(foe, bt.b.side[BT_PLAYER].level, ballMult(id), 0);
  pet.bag[id]--;                       // la bola se gasta al tirarla
  bt.foeInBall = false;
  bt.phase = BP_THROW;
  bt.until = millis() + 700;
  snprintf(bt.msg, sizeof(bt.msg), T(S_BM_THREW_A), itemName(id));
  sfxPlay(SFX_BALL_THROW);
}

// La bola llega, se ABRE y se lo traga. Es la pieza que faltaba: antes el
// vuelo enlazaba directo con el forcejeo, asi que la bola nunca se abria y
// el bicho se quedaba plantado fuera mientras ella se sacudia en el suelo.
#define CAPT_SUCK_MS 760
// Que fraccion de esa fase dura el encogimiento (el resto, la bola cerrada
// botando). El combate repinta a unos 11 fotogramas por segundo: con 280 ms
// el encogimiento cabia en DOS y parecia que el bicho se esfumaba de golpe,
// y con 540 sobraban tres fotogramas de bola abierta y vacia. 0,45 de 760 ms
// son ~340: tres o cuatro fotogramas entrando y la tapa cerrandose encima.
#define CAPT_SUCK_IN 0.45f
// Los sonidos de la bola que dependen del RELOJ y no de un cambio de fase:
// la caida (cuando la tapa se cierra, a mitad de BP_SUCK) y cada vaiven
// del forcejeo.
static bool gBolaCayo = false;
static uint32_t gBolaVaiven = 0;
// El dibujo se mece con sinf(now * 0,012): un vaiven cada 2*PI/0,012 ms.
#define CAPT_VAIVEN_MS 524

static void btStartSuck() {
  bt.phase = BP_SUCK;
  bt.until = millis() + CAPT_SUCK_MS;
  bt.msg[0] = 0;
  gBolaCayo = false;
  sfxPlay(SFX_BALL_HIT);
}

// La bola llego y NO HABIA NADIE: el rival esta bajo tierra, en el
// cielo, bajo el agua o esfumado (Combatant::hiding). Antes la bola se lo
// tragaba igual, desde su escondite. Como en los juegos, se gasta y se
// pierde el turno: el rival sale de donde estaba y pega.
static void btBallMissed() {
  snprintf(bt.msg, sizeof(bt.msg), T(S_BM_THE_BALL_MISSED));
  bt.foeInBall = false;
  sfxPlay(SFX_DENY);
  bt.phase = BP_MSG;
  bt.until = millis() + 900;
  // Igual que "It broke free!": el turno lo gasto el jugador.
  bt.acting = BT_PLAYER;
  bt.secondPending = true;
  bt.foeSlot = bt.b.pickFoeMove();
  bt.b.inicioDeRondaRival(bt.foeSlot);   // el jugador gasto la ronda en otra cosa
  pet.battleSave();
}

// El forcejeo. Aqui es donde los toques cuentan: la tirada NO se hace hasta
// que se acaba, para que apretar sirva de verdad y no sea teatro.
static void btStartWobble() {
  bt.phase = BP_WOBBLE;
  bt.wobble = 3;
  bt.until = millis() + 2600;
  bt.msg[0] = 0;
  bt.foeInBall = true;        // ya esta dentro: deja de dibujarse
  gBolaVaiven = millis() / CAPT_VAIVEN_MS;   // el primer toc, al primer vaiven
}

static void btResolveCatch() {
  const Combatant &foe = bt.b.side[BT_FOE];
  bt.chance = captureChance(foe, bt.b.side[BT_PLAYER].level, ballMult(bt.ballId),
                            bt.taps);
  bt.caught = ((uint8_t)random(100) < bt.chance);
  Serial.printf("captura: %s Nv.%u  toques=%u  prob=%u%%  -> %s\n",
                dexName(foe.dex), foe.level, bt.taps, bt.chance,
                bt.caught ? "ATRAPADO" : "se solto");
  if (bt.caught) {
    uint8_t donde = pet.catchMon(foe.dex, foe.level, foe.shiny, foe.ability,
                                 bt.ballId);
    snprintf(bt.msg, sizeof(bt.msg), T(S_BM_WAS_CAUGHT),
             foe.shiny ? "*" : "", dexName(foe.dex));
    if (donde == CATCH_BANK)
      snprintf(bt.msg, sizeof(bt.msg), T(S_BM_CAUGHT_SENT_TO_THE));
    else if (donde == CATCH_FULL)
      // Dos motivos distintos, dos recados distintos: "no hay sitio" con
      // 930 huecos libres manda al jugador a buscar algo que ya tiene.
      snprintf(bt.msg, sizeof(bt.msg),
               pet.cajasSinTarjeta ? T(S_BM_CARD_ERROR_IT_GOT)
                                   : T(S_BM_NO_ROOM_IT_GOT));
    // Capturar CUENTA como ganar: da experiencia, suma al porton del
    // gimnasio y puede soltar objeto, como en los juegos modernos. Hay que
    // decirlo a las claras (battleClose mira `winner`); sin esto quedaba en
    // 0xFF y lo que pasara dependia de la suerte del codigo.
    if (donde != CATCH_FULL) bt.b.winner = BT_PLAYER;
    bt.phase = BP_CAUGHT;
    bt.until = millis() + 2200;
    sfxPlay(SFX_BALL_CLICK);
    // la fanfarria de captura, que es la que todo el mundo reconoce
    musicStop();
    if (donde != CATCH_FULL) jinglePlay(JIN_CAUGHT);
  } else {
    snprintf(bt.msg, sizeof(bt.msg), T(S_BM_IT_BROKE_FREE));
    bt.foeInBall = false;      // se abre y sale: vuelve a verse
    sfxPlay(SFX_BALL_OPEN);
    // Fallar cuesta el turno: el rival ataca. Si no costara nada, tirar
    // bolas seria gratis y la escasez dejaria de importar.
    bt.phase = BP_MSG;
    bt.until = millis() + 900;
    // `acting` TIENE que quedar en BT_PLAYER: quien reparte el golpe
    // pendiente es `btLaunch(bt.acting ^ 1, ...)`, y sin tocarlo se queda
    // con el del turno anterior. Si el ultimo en moverse habia sido el
    // rival, el ^1 apuntaba a TI y tu propio bicho atacaba despues de que
    // se te escapara la bola. Lo mismo hace el relevo de equipo.
    bt.acting = BT_PLAYER;     // el turno se gasto tirando la bola
    bt.secondPending = true;
    bt.foeSlot = bt.b.pickFoeMove();
    bt.b.inicioDeRondaRival(bt.foeSlot);   // el jugador gasto la ronda en otra cosa
  }
  pet.battleSave();
}

// Donde esta el bicho rival en pantalla: es a donde va la bola y de donde
// se encoge el bicho al entrar. Un solo sitio, que si no se despega de
// renderBattle().
#define CAPT_BALL_X 347
#define CAPT_BALL_Y 196

// Cuanto lleva tragado el rival, 0..1. Lo usa renderBattle para encogerlo.
float btSuckProgress(uint32_t now) {
  if (bt.foeInBall) return 1.0f;
  if (bt.phase != BP_SUCK) return 0.0f;
  float u = 1.0f - (float)((int32_t)bt.until - (int32_t)now) / (float)CAPT_SUCK_MS;
  if (u < 0) u = 0;
  if (u > 1) u = 1;
  // Se lo traga en la PRIMERA mitad; la segunda es la bola cerrandose y
  // cayendo. Si durase lo mismo, la tapa se cerraria con el bicho a medias.
  // Se lo traga en los primeros CAPT_SUCK_IN y el resto es la bola
  // cerrandose y botando. El reparto NO es libre: el combate repinta a unos
  // 11 fotogramas por segundo, asi que un encogimiento de 280 ms cabia en
  // DOS fotogramas y se leia como un corte seco, no como que se lo tragaba.
  u /= CAPT_SUCK_IN;
  return u > 1.0f ? 1.0f : u;
}

// Pinta la bola: volando y girando, abriendose para tragar, o forcejeando
// en el suelo con sus sacudidas.
// Lo que mide la bola en pantalla: 1 = su tamano nativo (32 px). Esta
// aqui y no suelta en la llamada porque el centrado de abajo la necesita.
#define CAPT_BALL_ESCALA 1

void btDrawBall(uint32_t now) {
  uint8_t tier = ballArte(bt.ballId);

  // La bola apunta al CUERPO del bicho, no a una altura fija. CAPT_BALL_Y
  // son 196 y el rival pisa en 178: la bola se paraba por debajo de sus
  // pies. Y subir la constante solo acertaria con un tamano —— un
  // Caterpie y un Onix no tienen el cuerpo a la misma altura.
  //
  // btAltoDibujado[BT_FOE] es lo que battle_ui acaba de pintar de alto
  // (el mismo numero que usa la chapa de PS para no taparlo), asi que la
  // mitad por encima del suelo es su medio. Si todavia no hay medida, se
  // usa la constante de siempre.
  int alto = btAltoDibujado[BT_FOE];
  int y = alto > 0 ? (BT_GROUND_FOE - alto / 2) : CAPT_BALL_Y;
  int x = CAPT_BALL_X;
  const Art *bola = ART_BALL[tier][0];

  if (bt.phase == BP_THROW) {
    float u = 1.0f - (float)((int32_t)bt.until - (int32_t)now) / 700.0f;
    if (u < 0) u = 0;
    if (u > 1) u = 1;
    x = (int)(150 + u * (CAPT_BALL_X - 150));
    y = (int)(300 - u * (300 - y) - sinf(u * (float)PI) * 70.0f);
    // GIRANDO: las ocho celdas de la hoja son la misma bola rotada. Antes se
    // usaba solo la primera y la bola llegaba volando tiesa.
    uint8_t f = (uint8_t)((uint32_t)(u * 16.0f) % BALL_SPIN_FRAMES);
    bola = ART_BALL[tier][f];
  } else if (bt.phase == BP_SUCK) {
    // ABIERTA mientras se lo traga, y cerrada en cuanto cae dentro.
    float s = btSuckProgress(now);
    bola = (s < 1.0f) ? ART_BALL_OPEN[tier] : ART_BALL[tier][0];
    // al cerrarse da un bote corto, como en los juegos
    if (s >= 1.0f) {
      float u = 1.0f - (float)((int32_t)bt.until - (int32_t)now) / (float)CAPT_SUCK_MS;
      y -= (int)(sinf((u - 0.5f) * 2.0f * (float)PI) * 10.0f);
    }
  } else if (bt.phase == BP_WOBBLE) {
    // se sacude a un lado y a otro mientras quedan meneos
    x += (int)(sinf((float)now * 0.012f) * 7.0f);
  }
  // BP_CAUGHT: quieta. Es el clic, y lo que lo vende es que PARE de moverse.

  // La bola de VERDAD, la que se tiro: Battle animations/ball_*.png del
  // Anil, que es la misma que lanza el juego. Antes eran cuatro circulos
  // pintados a mano.
  //
  // A escala 1, o sea 32 px: el dibujo mide 32 de origen, asi que este es
  // su tamano NATIVO y cada pixel cae en su sitio. Estaba a 2 (64 px) y el
  // dueno dijo que se veia demasiado grande. No hay un paso intermedio
  // limpio: 48 saldria de estirar 32 por 1,5 y emborronaria el pixel art.
  //
  // Y se ancla por el CENTRO, no por el fondo. drawArt recibe el fondo
  // (baseY), y la llamada de antes pasaba `y + bola->h` —— que solo
  // centraba la bola por casualidad a escala 2 (alto dibujado 64, fondo
  // en y+32, centro en y). Al bajar a escala 1 el centro se fue 16 px
  // para abajo y la bola caia por debajo del bicho. Asi da igual la
  // escala que se ponga manana.
  // A SOMBRA, no chao debaixo dela. No voo o chao vai do seu lado (312)
  // ao do rival (178), e quanto mais alto a bola, menor a sombra: e o que
  // faz a bola parecer estar NO AR e nao pintada por cima do cenario.
  {
    int chao = BT_GROUND_FOE;
    if (bt.phase == BP_THROW) {
      float u = 1.0f - (float)((int32_t)bt.until - (int32_t)now) / 700.0f;
      if (u < 0) u = 0;
      if (u > 1) u = 1;
      chao = (int)(BT_GROUND_MINE + u * (BT_GROUND_FOE - BT_GROUND_MINE));
    }
    int alto = chao - y;
    if (alto < 0) alto = 0;
    int rx = 13 - alto / 14;
    if (rx < 6) rx = 6;
    tintEllipse(x, chao, rx, rx / 3 + 1, 3, 5, false);
  }
  drawArt(*bola, x, y + (bola->h * CAPT_BALL_ESCALA) / 2, CAPT_BALL_ESCALA);

  if (bt.phase != BP_WOBBLE) return;
  // barra de lo que llevas empujado: sin esto el jugador no sabe si tocar
  // sirve de algo, y entonces no toca
  int bw = 200, fw = bw * (bt.taps * CAPT_TAP > CAPT_TAP_MAX
                               ? CAPT_TAP_MAX : bt.taps * CAPT_TAP) / CAPT_TAP_MAX;
  gfx->fillRoundRect(CX - bw / 2, 348, bw, 16, 5, UI_TRACK);
  if (fw > 2) gfx->fillRoundRect(CX - bw / 2, 348, fw, 16, 5, UI_BAR_OK);
  gfx->setTextColor(INK_K);
  setSize(2);
  setCur(centerX("TAP!", 2), 326);
  printT("TAP!");
}

// Sacudir durante el forcejeo. Entra en la MISMA cuenta que los toques
// (bt.taps) en vez de inventarse un sistema: el tope, la barra y la
// formula de captura ya estaban hechos y probados, y asi sacudir y tocar
// se suman en lugar de competir.
//
// La diferencia esta en el precio: un toque vale CAPT_TAP y un meneo
// vale de uno a tres toques segun la fuerza. Es lo que hace que agitar
// la Pokebola -el gesto de verdad- sea mejor que picotear la pantalla.
void btCaptureMeneo(uint8_t fuerza) {
  if (bt.phase != BP_WOBBLE) return;
  uint8_t vale = (uint8_t)(1 + fuerza / 96);     // 1, 2 o 3
  bt.taps = (uint8_t)(bt.taps + vale > 250 ? 250 : bt.taps + vale);
  sfxPlay(SFX_TAP);
}

// ---------- enganche con el combate ----------

// Mueve la lista de la bolsa una pagina. dir +1 = bajar. true si se movio.
static bool btBagPagina(int dir) {
  uint8_t n = btBagCount();
  uint8_t tope = (uint8_t)(n > BBAG_ROWS ? n - BBAG_ROWS : 0);
  uint8_t antes = bt.bagScroll;
  if (dir > 0)
    bt.bagScroll = (uint8_t)(bt.bagScroll + BPAG_PASO > tope ? tope
                                                             : bt.bagScroll + BPAG_PASO);
  else
    bt.bagScroll = (uint8_t)(bt.bagScroll > BPAG_PASO ? bt.bagScroll - BPAG_PASO : 0);
  return bt.bagScroll != antes;
}

// ARRASTRAR en vertical con la bolsa abierta mueve la lista: el dedo hacia
// arriba baja por ella, como en cualquier lista. true si el gesto era para
// la bolsa (y entonces no abre ni el reloj ni la ficha).
bool btBagSwipeV(int dir) {
  if (!bt.open || bt.phase != BP_BAG) return false;
  if (btBagPagina(dir < 0 ? +1 : -1)) sfxPlay(SFX_TAP);
  return true;
}

// Devuelve true si se ha comido el toque.
bool btCaptureTap(int16_t x, int16_t y) {
  if (bt.phase == BP_WOBBLE) {
    bt.taps++;                 // empujar: es lo unico que se puede hacer aqui
    // Y la probabilidad SE RECALCULA aqui. Estaba puesta al tirar la
    // bola con taps=0 y no se volvia a tocar hasta resolver, asi que la
    // barra se quedaba clavada mientras el jugador aporreaba la
    // pantalla: los toques contaban de verdad al final, pero no se veia
    // ni uno. Sin respuesta, parece roto aunque funcione.
    uint8_t mult = ballMult(bt.ballId);
    bt.chance = captureChance(bt.b.side[BT_FOE],
                              bt.b.side[BT_PLAYER].level, mult, bt.taps);
    return true;
  }
  if (bt.phase == BP_BAG) {
    // Las PESTANAS: toda la cabecera del panel, mitad y mitad. Dibujadas
    // miden 28 px de alto (2,7 mm); tocarlas es de aqui a la primera fila.
    if (y >= BPAN_Y0 && y < BBAG_Y - 4 && x >= BPAN_X0 && x <= BPAN_X1) {
      if (bt.pvp) return true;      // entre placas ha uma pestana so
      bt.bagPocket = (uint8_t)(x < 233 ? 0 : 1);
      bt.bagScroll = 0;
      sfxPlay(SFX_TAP);
      return true;
    }
    uint8_t n = btBagCount();
    // LA FAJA DE PASAR PAGINA: toda la franja de debajo de las filas, de
    // lado a lado del panel. Mitad izquierda sube, mitad derecha baja. Un
    // toque aqui NUNCA cierra, aunque ya no haya a donde ir.
    if (y >= BPAG_Y - 6 && y <= BPAG_Y + BPAG_H + 6 &&
        x >= BPAN_X0 && x <= BPAN_X1) {
      if (btBagPagina(x < 233 ? -1 : +1)) sfxPlay(SFX_TAP);
      return true;
    }
    for (uint8_t r = 0; r < BBAG_ROWS; r++) {
      uint8_t i = (uint8_t)(bt.bagScroll + r);
      if (i >= n) break;
      int ry = BBAG_Y + r * BBAG_H;
      if (x < BBAG_X + 8 || x > BBAG_X + BBAG_W - 8 || y < ry || y > ry + 34)
        continue;
      uint8_t id = btBagAt(i);
      if (ITEM_TBL[id].pocket == BP_BALL) {
        // El bicho de un lider o de un entrenador es SUYO: no se le tira
        // una bola encima, igual que en los juegos.
        if (bt.gym >= 0 || bt.trainer >= 0) { sfxPlay(SFX_DENY); return true; }
        btThrowBall(id);
      } else if (ITEM_TBL[id].use == IU_POKEMON) {
        // HAY QUE ELEGIR A CUAL. Lo dice el PBS del Anil en `BattleUse`
        // (OnPokemon), y es lo que faltaba para que un Revivir sirviera
        // de algo: antes la bolsa lo aplicaba todo al de pista, que por
        // definicion esta en pie.
        //
        // No se gasta el turno todavia: hasta que no se toca un hueco no
        // ha pasado nada, y tocar fuera devuelve a la bolsa.
        bt.itemPick = id;
        bt.phase = BP_PARTY;
        bt.partyScroll = 0;
        sfxPlay(SFX_TAP);
      } else if (bt.pvp) {
        // as X: no que esta na pista. Vira a minha jogada do turno.
        pvpUsaItem(id, pvpL[pvpEu].ativo);
      } else if (btItemEnPista(id, true)) {
        // Usar un objeto cuesta el turno, como en los juegos.
        snprintf(bt.msg, sizeof(bt.msg), T(S_BM_USED_2), itemName(id));
        // Aqui habia un `side[BT_PLAYER].hp = pet.hpLeft`. Era lo que
        // remataba el fallo: copiaba la vida de lo GUARDADO encima de la
        // del que estaba peleando. btItemOnField ya trabaja sobre el
        // Combatant, asi que no hay nada que copiar de vuelta.
        bt.phase = BP_MSG;
        bt.until = millis() + 900;
        bt.acting = BT_PLAYER;   // mismo motivo que al tirar la bola
        bt.secondPending = true;
        bt.foeSlot = bt.b.pickFoeMove();
        bt.b.inicioDeRondaRival(bt.foeSlot);   // el jugador gasto la ronda en otra cosa
        sfxPlay(SFX_EAT);
      } else {
        sfxPlay(SFX_DENY);     // no haria nada: no se desperdicia
      }
      return true;
    }
    // Cerrar es tocar FUERA DEL PANEL. Antes era "fuera de la lista", y un
    // toque dentro del panel que no acertase a nada —— al lado de una
    // flecha, entre dos filas —— devolvia al combate.
    if (lejosDe(x, y, BPAN_X0, BPAN_Y0, BPAN_X1, BPAN_Y1))
      bt.phase = BP_CHOOSE;
    return true;
  }
  if (bt.phase == BP_CHOOSE) {
    int dx = x - BBAG_BTN_X, dy = y - BBAG_BTN_Y;
    if (dx * dx + dy * dy <= (BBAG_BTN_R + 8) * (BBAG_BTN_R + 8)) {
      if (bt.pvp && !pvpBolsaPode()) { sfxPlay(SFX_DENY); return true; }
      btOpenBag();
      sfxPlay(SFX_TAP);
      return true;
    }
  }
  return false;
}

// Avanza las fases propias. true si ha consumido el tick.
bool btCaptureTick(uint32_t now) {
  if (bt.phase == BP_THROW) {
    if (!timeLeft(bt.until)) {
      if (bt.b.side[BT_FOE].hiding != HID_NONE) btBallMissed();
      else btStartSuck();
    }
    return true;
  }
  if (bt.phase == BP_SUCK) {
    // La tapa se cierra y la bola cae: el golpe contra el suelo, una vez.
    if (!gBolaCayo &&
        (int32_t)(bt.until - now) < (int32_t)(CAPT_SUCK_MS * (1.0f - CAPT_SUCK_IN))) {
      gBolaCayo = true;
      sfxPlay(SFX_BALL_DROP);
    }
    if (!timeLeft(bt.until)) btStartWobble();
    return true;
  }
  if (bt.phase == BP_WOBBLE) {
    // Un toc por vaiven, al compas del dibujo.
    if (now / CAPT_VAIVEN_MS != gBolaVaiven) {
      gBolaVaiven = now / CAPT_VAIVEN_MS;
      if (timeLeft(bt.until)) sfxPlay(SFX_BALL_SHAKE);
    }
    if (!timeLeft(bt.until)) btResolveCatch();
    return true;
  }
  if (bt.phase == BP_CAUGHT) {
    if (!timeLeft(bt.until)) { bt.b.over = true; bt.phase = BP_END; }
    return true;
  }
  return bt.phase == BP_BAG;   // la bolsa congela el combate
}
