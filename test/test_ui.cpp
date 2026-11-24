// LA INTERFAZ: el sketch de verdad, con toques de verdad.
//
// Los .ino son dos tercios del codigo escrito a mano y no tenian ni un
// test. La excusa de siempre es que afirmar sobre un pixel cuesta caro
// y se rompe con cualquier retoque de diseno — y es cierta, pero solo
// para una parte. Hay mucho que SI se puede afirmar barato:
//
//   - que la pantalla no se queda en negro ni se congela
//   - que no se pinta NADA fuera del circulo (el panel es redondo: lo
//     que caiga en las esquinas no existe para el que lo mira)
//   - que las paginas de la ficha no se salen por los lados
//   - que el gimnasio reparte siempre una jugada buena, una regular y
//     una mala, que es lo que hace que perder sea culpa tuya
//   - que las cuentas de texto (ancho, centrado, corte por palabras)
//     dan lo que dicen
//
// Este binario es APARTE del de siempre (tamapoke_uitests) porque el
// sketch necesita los shims del simulador: pantalla, tactil y Wire. Los
// dos comparten main.cpp y framework.h.
#include "framework.h"
#include <Arduino.h>
#include <Arduino_GFX_Library.h>

#include "../pet.h"
#include "../dex.h"
#include "../battle.h"
#include "../battle_ui.h"   // bt, battleStart, las fases
#include "../sdmon.h"
#include "../audio.h"   // o volume, no teste da musica da aventura
#include "../i18n.h"
#include "../duelo.h"
#include "../pvp.h"
#include "../abil_names.h"   // abilName: a caixa diz o nome do idioma (leva 78)
#include "../desktop/guion.h"

#include <algorithm>
#include <set>
#include <tuple>
#include <filesystem>
#include <string>
#include <vector>

// Los guiones estan donde el simulador los deja. El Makefile corre
// desde test/.
static const std::string DESKTOP = "../desktop";

static std::vector<std::string> guionesDelSimulador() {
  std::vector<std::string> out;
  std::error_code ec;
  for (auto &e : std::filesystem::directory_iterator(DESKTOP, ec))
    if (e.path().extension() == ".txt")
      out.push_back(e.path().filename().string());
  std::sort(out.begin(), out.end());
  return out;
}

// Lo que el sketch expone (no es `static`).
void setup();
void loop();
void handleTouch();
void onTap(int16_t x, int16_t y);
void onSwipe(int dir);
void onSwipeV(int dir);
bool inPetZone(int16_t x, int16_t y);
uint16_t textW(const char *s, uint8_t size);
int centerX(const char *s, uint8_t size);
void setSize(uint8_t n);
void setCur(int x, int y);
void printT(const char *s);
extern bool gCjkFont;
void applyLangFont();
uint16_t lerp565(uint16_t a, uint16_t b, int i, int n);
int sceneHour();
extern Arduino_Canvas *gfx;
extern Pet pet;
extern uint8_t choiceKind;
void applyLangFont();
extern Arduino_CO5300 *panel;
extern int16_t monFor;
bool btPartyTap(int16_t x, int16_t y);   // battle_ui.ino, no esta en el .h
void btOpenBag();                        // capture.ino, idem
// El alto de CUERPO que acaba de pintarse de cada lado (battle_ui.ino).
extern int16_t btAltoDibujado[2];
extern int16_t btPedido[2];
// Y el alto REAL en pixeles de lo ultimo pintado de cada lado.
extern int16_t btAltoReal[2];
// Y en que X se pinto la bola de la ENTRADA de cada lado (-1 = ninguna).
extern int16_t btBolaX[2];
// El estado de la Pokedex, que vive en TamaPoke.ino y tampoco esta en un .h
extern bool galleryOpen;
extern int galleryPage;
extern int16_t galleryDetail;
// BT_CARD_Y0/Y1 de battle_ui.ino: la banda de la tarjeta de golpes, que es
// donde hay que tocar para usar el que esta elegido.
// La intro: cuatro paginas del profesor y el boton de genero. Copiado de
// intro.ino, como el resto de la geometria de este fichero.
#define OAK_PAGES_T 5
#define GEN_BTN_Y_T 330
#define GEN_BTN_H_T 54
#define BT_CARD_Y0_T 334
#define BT_CARD_Y1_T 420
extern uint32_t lastInteract;
extern uint8_t dimStage;
extern bool screenOff;
void updateBrightness(uint32_t now);

// Del shim: el tactil y la "SD".
void mockTouchSet(int16_t x, int16_t y, bool down);
void mockTouchTick();
void mockSdRoot(const char *path);
void mockSetEpoch(uint32_t e);

// El shim de pantalla llama a esto al final de cada frame. En el
// simulador vuelca un PPM; aqui solo se cuenta, que es lo que hace
// falta para saber si la pantalla sigue viva.
static uint32_t g_frames = 0;
void simOnFlush(Arduino_Canvas *) { g_frames++; }

// ---------------------------------------------------------------- utiles

static const int CX = 233, CY = 233, R = 233;

// setup() UNA vez: monta la SD, crea el Pet y deja el firmware listo.
// Los tests no pueden reiniciarlo (el sketch tiene estado global), asi
// que lo comparten — por eso cada uno deja la pantalla donde la
// encontro.
static void arranca() {
  static bool hecho = false;
  if (hecho) return;
  hecho = true;
  mockSerialSilence(true);
  mockSdRoot("../tools/sdcard");
  mockSetEpoch(1758700000);
  mockNvsReset();
  mockSetMillis(0);
  setup();
  for (int i = 0; i < 20; i++) { mockAdvanceMillis(16); loop(); }

  // SALIR DE LA INTRO. No se va con el tiempo: solo con toques (portada,
  // cuatro paginas del profesor, boton de genero —— intro.ino:208). Y
  // mientras siga abierta se come TODOS los toques, porque onTap empieza
  // con `if (introOpen) { introTap(x, y); return; }`. Sin esto ningun
  // toque de ningun test de UI llegaba a la pantalla que el test cree que
  // esta tocando: los que comprobaban "no pasa nada" pasaban vacios.
  mockAdvanceMillis(1200);          // la portada se come el primer segundo
  for (int i = 0; i < 2 + OAK_PAGES_T; i++) {
    mockTouchSet(142, GEN_BTN_Y_T + GEN_BTN_H_T / 2, true);
    mockTouchTick();
    for (int j = 0; j < 4; j++) { mockAdvanceMillis(16); loop(); }
    mockTouchSet(142, GEN_BTN_Y_T + GEN_BTN_H_T / 2, false);
    mockTouchTick();
    for (int j = 0; j < 8; j++) { mockAdvanceMillis(16); loop(); }
  }
}

static void avanza(uint32_t ms, uint32_t paso = 16) {
  for (uint32_t t = 0; t < ms; t += paso) {
    mockAdvanceMillis(paso);
    loop();
  }
}

static void toca(int16_t x, int16_t y) {
  mockTouchSet(x, y, true);
  mockTouchTick();
  avanza(60);
  mockTouchSet(x, y, false);
  mockTouchTick();
  avanza(120);
}

// ¿se pinto algo que no sea el color de fondo?
static int pixelesDistintos(uint16_t fondo) {
  const uint16_t *fb = gfx->getFramebuffer();
  int n = 0;
  for (int i = 0; i < gfx->width() * gfx->height(); i++)
    if (fb[i] != fondo) n++;
  return n;
}

// ---------------------------------------------------------------- tests

// El parpadeo negro al tocar la pantalla, que me costo dos intentos
// fallidos por buscarlo donde no estaba (la cadencia de repintado).
//
// loop() toma `now` en su primera linea. handleTouch() corre MAS ABAJO en
// la misma vuelta y marca lastInteract con millis(), que para entonces ya
// avanzo. O sea que lastInteract puede ir POR DELANTE de `now`, y la resta
// sin signo se daba la vuelta a ~4.29e9: dimStage saltaba a 2, el brillo
// caia a 8 de 255 (negro en un AMOLED) y volvia a la vuelta siguiente.
//
// Se afirma sobre dimStage y sobre el brillo que recibe el panel, no sobre
// pixeles: es la cuenta la que estaba mal.
// Lo que el dueno vio: con un Encore encima, la caja decia el golpe que el
// habia elegido y el dano era de otro. El motor nunca estuvo mal; mentia
// la caja, porque btLaunch miraba lockTurns y la carga pero NO encoreSlot.
// LOS RESTOS, por el camino de verdad. El test del motor pasa desde
// siempre; lo que no habia era nadie comprobando que el objeto llega al
// combate y que el turno de la PANTALLA lo cobra.
TEST(combate_ui, los_restos_curan_jugando) {
  arranca();
  if (pet.awaitingStarter()) pet.chooseStarter(1);
  while (pet.isEgg()) pet.eggTap();
  pet.sleeping = false;
  pet.energy = 100;
  pet.exp = Pet::expForLevel(30);

  // el objeto, puesto como lo pone el jugador
  uint8_t restos = 0;
  for (uint8_t i = 1; i < ITEM_COUNT; i++)
    if (ITEM_TBL[i].fx == IT_LEFTOVERS) { restos = i; break; }
  CHECK_MSG(restos, "no hay ningun objeto con IT_LEFTOVERS");
  CHECK_MSG(pet.takeItem(restos), "no se pudo meter en la bolsa");
  CHECK_MSG(pet.holdItem(restos), "no se lo pudo poner");

  pet.hpLeft = (uint16_t)(pet.maxHp() / 2);   // a media vida, para notarlo
  // El rival tiene que AGUANTAR mi golpe (si cae, el turno se acaba antes
  // de cerrarse) y a la vez no matarme a mi (si caigo, igual). Un Pidgey
  // de nivel 10 al que se le inflan los PS a mano cumple las dos cosas:
  // la primera version puso un Snorlax de 60 y me mataba de un golpe.
  battleStart(16, 10);
  CHECK_MSG(bt.open, "el combate no se abrio");
  for (int i = 0; i < 400 && bt.phase != BP_CHOOSE; i++) avanza(16);
  CHECK_MSG(bt.phase == BP_CHOOSE, "no se llego a elegir golpe");

  CHECK_MSG(bt.b.side[BT_PLAYER].item == restos,
            "el objeto no llego al combate: lo que lleva puesto el bicho "
            "no se esta copiando al combatiente");

  // un turno entero, pasando por la pantalla
  // Los dos tienen que AGUANTAR un turno entero: si cae cualquiera, el
  // cierre de turno no llega a correr y no hay Restos que valgan. Con los
  // PS de la partida recien empezada (30) el rival me mataba de un golpe.
  bt.b.side[BT_FOE].hpMax = 9999;
  bt.b.side[BT_FOE].hp = 9999;
  bt.b.side[BT_PLAYER].hpMax = 400;
  bt.b.side[BT_PLAYER].hp = 200;
  uint16_t antes = bt.b.side[BT_PLAYER].hp;
  toca(233, (BT_CARD_Y0_T + BT_CARD_Y1_T) / 2);
  bool subio = false, loDijo = false;
  uint16_t previo = antes;
  for (int i = 0; i < 600 && bt.open; i++) {
    avanza(16);
    uint16_t h = bt.b.side[BT_PLAYER].hp;
    // Cualquier SUBIDA entre dos muestras, no "por encima del principio":
    // con el rival pegando, los PS nunca vuelven al valor inicial, y la
    // primera version de este test medía eso y no veía nada.
    if (h > previo) subio = true;
    previo = h;
    if (strstr(bt.msg, "restored HP")) loDijo = true;
  }
  // Y que SE VEA. Curar un dieciseisavo sin decir nada es, a nivel bajo,
  // uno o dos PS en una barra de 466 px: el dueno dio el objeto por roto
  // y el motor llevaba un test verde desde siempre.
  CHECK_MSG(loDijo,
            "los Restos curaron y la caja no dijo nada: desde fuera el "
            "objeto no hace nada");
  CHECK_MSG(subio,
            "paso un turno entero con los Restos puestos y los PS no "
            "subieron ni una vez");
}

// Una ronda entera por la pantalla, y lo que dijo la caja. Sirve para medir
// lo que el dueno LEE: en la logica, el aviso y el veneno estaban los dos.
static std::string rondaFrases(uint8_t yawnQueda, uint8_t encoreQueda, uint8_t estado) {
  arranca();
  if (pet.awaitingStarter()) pet.chooseStarter(1);
  while (pet.isEgg()) pet.eggTap();
  pet.sleeping = false;
  pet.energy = 100;
  pet.exp = Pet::expForLevel(20);
  pet.hpLeft = pet.maxHp();
  battleStart(16, 10);
  for (int i = 0; i < 400 && bt.phase != BP_CHOOSE; i++) avanza(16);
  uint16_t splash = 0;
  for (uint16_t m = 0; m < MOVE_COUNT; m++)
    if (!strcmp(MOVE_TBL[m].name, "SPLASH")) splash = m;
  Combatant &yo = bt.b.side[BT_PLAYER];
  Combatant &el = bt.b.side[BT_FOE];
  yo.moves[0] = splash; yo.usesLeft[0] = 30; yo.nMoves = 1;
  el.moves[0] = splash; el.usesLeft[0] = 30; el.nMoves = 1;
  yo.hpMax = 900; yo.hp = 900;
  yo.status = estado;
  yo.encoreSlot = encoreQueda ? 0 : 0xFF;
  yo.encoreTurns = encoreQueda;
  yo.yawn = yawnQueda;
  bt.sel = 0;
  toca(233, (BT_CARD_Y0_T + BT_CARD_Y1_T) / 2);
  std::string guion;
  std::string ultima;
  bool salio = false;
  for (int i = 0; i < 700; i++) {
    avanza(16);
    if (bt.phase != BP_CHOOSE) salio = true;
    if (bt.msg[0] && ultima != bt.msg) {
      ultima = bt.msg;
      guion += "\n    | " + ultima;
    }
    if (salio && bt.phase == BP_CHOOSE) break;
  }
  return guion;
}

TEST(combate_ui, encore_que_acaba_no_se_come_la_frase_del_veneno) {
  std::string g = rondaFrases(0, 1, ST_POISON);
  CHECK_MSG(g.find("encore ended") != std::string::npos,
            "nadie dijo que el Encore se acabo:" + g);
  CHECK_MSG(g.find("hurt by poison") != std::string::npos,
            "el aviso del Encore se comio la frase del veneno:" + g);
  CHECK_MSG(bt.b.side[BT_PLAYER].encoreTurns == 0, "y el Encore tenia que acabar");
}

TEST(combate_ui, bostezo_dice_que_se_durmio) {
  std::string g = rondaFrases(1, 0, ST_NONE);
  CHECK_MSG(g.find("fell asleep") != std::string::npos,
            "se durmio sin que nadie lo dijera:" + g);
  CHECK_MSG(bt.b.side[BT_PLAYER].status == ST_SLEEP, "y tenia que estar dormido");
}

// Encore sobre un golpe ANULADO: en el Anil sale Forcejeo, y la caja tiene
// que decir eso —— no el golpe del Encore, que es el que no puede usar.
TEST(combate_ui, encore_sobre_un_golpe_anulado_anuncia_forcejeo) {
  arranca();
  if (pet.awaitingStarter()) pet.chooseStarter(1);
  while (pet.isEgg()) pet.eggTap();
  pet.sleeping = false;
  pet.energy = 100;
  pet.exp = Pet::expForLevel(20);
  pet.hpLeft = pet.maxHp();
  battleStart(16, 10);
  for (int i = 0; i < 400 && bt.phase != BP_CHOOSE; i++) avanza(16);
  CHECK_MSG(bt.phase == BP_CHOOSE, "no se llego a elegir golpe");
  uint16_t tackle = 0, growl = 0, splash = 0;
  for (uint16_t m = 0; m < MOVE_COUNT; m++) {
    if (!strcmp(MOVE_TBL[m].name, "TACKLE")) tackle = m;
    if (!strcmp(MOVE_TBL[m].name, "GROWL")) growl = m;
    if (!strcmp(MOVE_TBL[m].name, "SPLASH")) splash = m;
  }
  Combatant &yo = bt.b.side[BT_PLAYER];
  Combatant &el = bt.b.side[BT_FOE];
  yo.moves[0] = tackle;  yo.usesLeft[0] = 30;
  yo.moves[1] = growl;   yo.usesLeft[1] = 30;
  yo.nMoves = 2;
  yo.hpMax = 900; yo.hp = 900;
  el.moves[0] = splash; el.usesLeft[0] = 30; el.nMoves = 1;
  el.hpMax = 900; el.hp = 900;
  yo.lastSlot = 0;
  yo.encoreSlot = 0;   yo.encoreTurns = 3;
  yo.disabledSlot = 0; yo.disableTurns = 3;
  bt.sel = 1;
  std::string mio = dexName(yo.asDex);
  toca(233, (BT_CARD_Y0_T + BT_CARD_Y1_T) / 2);
  std::string guion, ultima;
  bool salio = false;
  for (int i = 0; i < 700; i++) {
    avanza(16);
    if (bt.phase != BP_CHOOSE) salio = true;
    if (bt.msg[0] && ultima != bt.msg) {
      ultima = bt.msg;
      guion += "\n    | " + ultima;
    }
    if (salio && bt.phase == BP_CHOOSE) break;
  }
  CHECK_MSG(guion.find(mio + " used STRUGGLE") != std::string::npos,
            "tenia que anunciar Forcejeo:" + guion);
  CHECK_MSG(guion.find(mio + " used TACKLE") == std::string::npos &&
            guion.find(mio + " used GROWL") == std::string::npos,
            "anuncio un golpe que no puede usar:" + guion);
  CHECK_MSG(yo.hp < 900, "y Forcejeo cobra su retroceso");
  CHECK_EQ((int)yo.usesLeft[0] + (int)yo.usesLeft[1], 60);
}

TEST(combate_ui, con_encore_la_tarjeta_se_queda_en_el_golpe_del_encore) {
  arranca();
  if (pet.awaitingStarter()) pet.chooseStarter(1);
  while (pet.isEgg()) pet.eggTap();
  pet.sleeping = false;
  pet.energy = 100;
  pet.exp = Pet::expForLevel(20);
  pet.hpLeft = pet.maxHp();
  battleStart(16, 10);
  for (int i = 0; i < 400 && bt.phase != BP_CHOOSE; i++) avanza(16);
  CHECK_MSG(bt.phase == BP_CHOOSE, "no se llego a elegir golpe");
  uint16_t tackle = 0, growl = 0;
  for (uint16_t m = 0; m < MOVE_COUNT; m++) {
    if (!strcmp(MOVE_TBL[m].name, "TACKLE")) tackle = m;
    if (!strcmp(MOVE_TBL[m].name, "GROWL")) growl = m;
  }
  Combatant &yo = bt.b.side[BT_PLAYER];
  yo.moves[0] = tackle; yo.usesLeft[0] = 30;
  yo.moves[1] = growl;  yo.usesLeft[1] = 30;
  yo.moves[2] = growl;  yo.usesLeft[2] = 30;
  yo.nMoves = 3;
  bt.sel = 0;
  // sin Encore las flechas mueven la tarjeta (premisa del test)
  toca(233 + 110, (BT_CARD_Y0_T + BT_CARD_Y1_T) / 2);
  avanza(48);
  CHECK_EQ((int)bt.sel, 1);
  // con Encore en el hueco 2, la tarjeta se va a el...
  yo.encoreSlot = 2;
  yo.encoreTurns = 3;
  avanza(48);
  CHECK_EQ((int)bt.sel, 2);
  // ...y las flechas no la sacan de ahi
  toca(233 - 110, (BT_CARD_Y0_T + BT_CARD_Y1_T) / 2);
  avanza(48);
  CHECK_EQ((int)bt.sel, 2);
  CHECK_MSG(bt.phase == BP_CHOOSE, "y sigue esperando: no sale solo");
}

TEST(combate_ui, la_caja_anuncia_el_golpe_del_encore_y_no_el_elegido) {
  arranca();
  if (pet.awaitingStarter()) pet.chooseStarter(1);
  while (pet.isEgg()) pet.eggTap();
  pet.sleeping = false;
  pet.energy = 100;
  pet.exp = Pet::expForLevel(20);
  pet.hpLeft = pet.maxHp();
  battleStart(16, 10);
  CHECK_MSG(bt.open, "el combate no se abrio");
  for (int i = 0; i < 400 && bt.phase != BP_CHOOSE; i++) avanza(16);
  CHECK_MSG(bt.phase == BP_CHOOSE, "no se llego a elegir golpe");

  uint16_t tackle = 0, growl = 0;
  for (uint16_t m = 0; m < MOVE_COUNT; m++) {
    if (!strcmp(MOVE_TBL[m].name, "TACKLE")) tackle = m;
    if (!strcmp(MOVE_TBL[m].name, "GROWL")) growl = m;
  }
  CHECK_MSG(tackle && growl, "faltan golpes en la tabla");

  Combatant &yo = bt.b.side[BT_PLAYER];
  yo.moves[0] = tackle;  yo.usesLeft[0] = 30;
  yo.moves[1] = growl;   yo.usesLeft[1] = 30;
  yo.nMoves = 2;
  // el rival me puso Encore en el hueco 0 (Placaje)...
  yo.encoreSlot = 0;
  yo.encoreTurns = 3;
  // ...y yo elijo el hueco 1 (Grunido), que es el que la caja decia antes
  bt.sel = 1;
  std::string mio = dexName(yo.asDex);

  toca(233, (BT_CARD_Y0_T + BT_CARD_Y1_T) / 2);

  std::vector<std::string> frases;
  for (int i = 0; i < 500; i++) {
    avanza(16);
    if (bt.msg[0] && (frases.empty() || frases.back() != bt.msg))
      frases.push_back(bt.msg);
    if (!bt.open) break;
  }
  std::string guion;
  for (const std::string &f : frases) guion += "\n    | " + f;

  bool dijoPlacaje = false, dijoGrunido = false;
  for (const std::string &f : frases) {
    if (f.find(mio + " used TACKLE") != std::string::npos) dijoPlacaje = true;
    if (f.find(mio + " used GROWL") != std::string::npos) dijoGrunido = true;
  }
  CHECK_MSG(!dijoGrunido,
            "la caja anuncio el golpe que elegi (GROWL) y el que sale es el "
            "del Encore (TACKLE): desde la pantalla Encore no hace nada:" +
                guion);
  CHECK_MSG(dijoPlacaje,
            "nadie anuncio el golpe del Encore (TACKLE):" + guion);
}

TEST(interfaz, un_toque_recien_dado_no_apaga_la_pantalla) {
  arranca();
  screenOff = false;
  pet.sleeping = false;
  lastInteract = 100000;
  updateBrightness(99995);   // el bucle va 5 ms por detras del dedo
  CHECK_MSG(dimStage == 0, "la resta de inactividad se dio la vuelta: "
                           "dimStage=" + std::to_string((int)dimStage));
  CHECK_MSG(panel->brightness() > 100,
            "la pantalla se apago justo al tocarla: brillo=" +
                std::to_string((int)panel->brightness()));
}

// Y la otra mitad: cuando el reposo es de verdad, SI tiene que atenuar.
// Sin esto, "arreglar" el parpadeo poniendo idle a 0 a secas pasaria.
TEST(interfaz, cinco_minutos_sin_tocar_si_atenuan) {
  arranca();
  screenOff = false;
  pet.sleeping = false;
  lastInteract = 0;
  updateBrightness(400000);  // 6 min y medio quieto
  CHECK_EQ((int)dimStage, 2);
}

TEST(interfaz, el_firmware_arranca_y_pinta) {
  arranca();
  CHECK_MSG(g_frames > 0, "setup()+loop() no llegaron a pintar un frame");
  CHECK(gfx != nullptr);
  CHECK_EQ((int)gfx->width(), 466);
  CHECK_EQ((int)gfx->height(), 466);
  int vivos = pixelesDistintos(0);
  CHECK_MSG(vivos > 1000, "la pantalla esta practicamente vacia (" +
                          std::to_string(vivos) + " pixeles pintados)");
}

// NO HAY comprobacion de "que no se pinte fuera del circulo", y conste
// que lo intente dos veces.
//
// La primera exigia que las esquinas estuvieran negras: falla, porque
// el firmware hace `fillScreen` del color de fondo y eso toca el buffer
// entero (lo correcto: recortar un circulo pixel a pixel costaria mas
// que pintar de mas un trozo que nadie ve).
//
// La segunda exigia que hubiera un solo color ahi: falla tambien, y con
// 201. Los escenarios de la SD se vuelcan como un bitmap de 466x466, asi
// que las esquinas llevan paisaje. Tampoco es un fallo.
//
// Conclusion, escrita para no volver a intentarlo: un panel redondo NO
// da un invariante sobre las esquinas del buffer. Si alguna vez hace
// falta saber si una caja se salio, hay que mirar la caja, no la
// pantalla.
//
// (La primera version ademas llevaba `uint16_t vistos[8]` y un `n++`
// sin tope: al noveno color leia fuera del array y se llevaba el
// proceso por delante. El primer fallo que encontro esta bateria de
// interfaz fue el de la propia bateria.)

// Cuantos pixeles hay que no sean el color mas repetido. Es la forma
// barata de preguntar "¿hay ALGO en la pantalla o es un rectangulo de
// un color?".
#include <map>
static int pixelesConContenido() {
  const uint16_t *fb = gfx->getFramebuffer();
  std::map<uint16_t, int> cuenta;
  const int N = 466 * 466;
  for (int i = 0; i < N; i++) cuenta[fb[i]]++;
  int masComun = 0;
  for (const auto &kv : cuenta) if (kv.second > masComun) masComun = kv.second;
  return N - masComun;
}

TEST(interfaz, la_pantalla_sigue_viva_despues_de_un_buen_rato) {
  // Lo que NO puede pasar: que deje de repintar. Un contador de frames
  // parado es la pantalla congelada, que en la placa es el unico fallo
  // del que el jugador no puede salir.
  //
  // El numero se mide, no se supone: el firmware solo vuelca cuando
  // hay algo que cambiar, asi que pedir 30 por segundo seria pedir que
  // malgaste bateria. Lo que se exige es que no se PARE.
  arranca();
  uint32_t antes = g_frames;
  avanza(5000);
  uint32_t pintados = g_frames - antes;
  CHECK_MSG(pintados > 0, "en 5 segundos no se pinto ni un frame: "
                          "la pantalla esta congelada");
  printf("        (%u frames en 5 s)\n", (unsigned)pintados);
}

// Cuantos pixeles encendidos hay en un rectangulo.
// La SUMA de lo que hay en un rectangulo. Contar pixeles distintos del
// negro no sirve en el combate: el fondo ya llena la zona entera, asi que
// dibujar una bola encima cambia QUE pixeles hay pero no CUANTOS.
static uint32_t huellaDe(int x, int y, int w, int h) {
  const uint16_t *fb = gfx->getFramebuffer();
  uint32_t acc = 0;
  for (int yy = y; yy < y + h; yy++)
    for (int xx = x; xx < x + w; xx++)
      if (xx >= 0 && yy >= 0 && xx < gfx->width() && yy < gfx->height())
        acc = acc * 31 + fb[yy * gfx->width() + xx];
  return acc;
}

static int pixelesEn(int x, int y, int w, int h, uint16_t fondo) {
  const uint16_t *fb = gfx->getFramebuffer();
  int n = 0;
  for (int yy = y; yy < y + h; yy++)
    for (int xx = x; xx < x + w; xx++)
      if (xx >= 0 && yy >= 0 && xx < gfx->width() && yy < gfx->height() &&
          fb[yy * gfx->width() + xx] != fondo)
        n++;
  return n;
}

// a~ y o~, que CP437 no tiene y el portugues necesita en casi cada frase.
// printT() los dibuja poniendo la '~' de la fuente encima de la letra.
//
// Lo que se afirma aqui no es "se ve bonito" sino lo que se puede romper
// sin darse cuenta: que el til aparezca, que caiga ENCIMA (en las filas
// que la 'a' deja libres) y, sobre todo, que no gaste ni un pixel de
// ancho. El centrado de toda la pantalla es strlen()*6*talla; un til que
// avanzara descuadraria las 110 cadenas de los ocho idiomas de golpe.
TEST(interfaz, el_til_del_portugues_se_pinta_y_no_mueve_nada) {
  arranca();
  const uint8_t T = 2;          // la talla de las descripciones
  const int X = 100, Y = 200;

  // el ancho no depende del til: son los mismos tres bytes
  CHECK_EQ((int)textW("nao", T), (int)textW("n\260o", T));
  CHECK_EQ((int)textW("n\260o", T), 3 * 6 * T);

  gfx->fillScreen(0);
  gfx->setTextColor(0xFFFF);
  setSize(T);
  setCur(X, Y);
  printT("nao");
  int finPelado = gfx->getCursorX();
  int celdaPelada = pixelesEn(X + 6 * T, Y, 6 * T, 8 * T, 0);
  int arribaPelado = pixelesEn(X + 6 * T, Y, 6 * T, 2 * T, 0);

  gfx->fillScreen(0);
  setSize(T);
  setCur(X, Y);
  printT("n\260o");
  int finTil = gfx->getCursorX();
  int celdaTil = pixelesEn(X + 6 * T, Y, 6 * T, 8 * T, 0);
  int arribaTil = pixelesEn(X + 6 * T, Y, 6 * T, 2 * T, 0);

  // el cursor acaba en el MISMO sitio: el til no avanza
  CHECK_EQ(finTil, finPelado);
  // la 'a' pelada no toca las dos filas de arriba de su celda
  CHECK_MSG(arribaPelado == 0,
            "la 'a' ya usa las filas de arriba: el til no cabe dentro de la "
            "celda y se metera en la linea anterior (" +
            std::to_string(arribaPelado) + " pixeles)");
  // y con til, si
  CHECK_MSG(arribaTil > 0, "el til no se ha pintado");
  CHECK_MSG(celdaTil > celdaPelada,
            "con til tiene que haber MAS pixeles que sin el");

  // lo mismo para la o~
  gfx->fillScreen(0);
  setSize(T);
  setCur(X, Y);
  printT("p\261e");
  CHECK_MSG(pixelesEn(X + 6 * T, Y, 6 * T, 2 * T, 0) > 0,
            "la o~ no se ha pintado");

  // y que no se cuelen en japones, donde 0260/0261 son bytes de
  // continuacion de cualquier kanji y no son ningun til
  // setLang() solo GUARDA el idioma; quien cambia la fuente es
  // applyLangFont(), que corre al pintar. Hay que llamarla a mano.
  Lang antes = gLang;
  setLang(LANG_JA);
  applyLangFont();
  CHECK_MSG(gCjkFont, "el japones tendria que usar la fuente CJK");
  setLang(antes);
  applyLangFont();
  CHECK_MSG(!gCjkFont, "al volver tendria que usar otra vez la 5x7");
}

TEST(interfaz, las_cuentas_de_texto_dan_lo_que_dicen) {
  arranca();
  // La fuente es de ancho fijo: el doble de letras, el doble de ancho.
  uint16_t a = textW("A", 1), aa = textW("AA", 1);
  CHECK_MSG(a > 0, "una letra mide 0");
  CHECK_EQ((int)aa, (int)(a * 2));
  CHECK_MSG(textW("A", 2) == a * 2, "la escala 2 tiene que doblar el ancho");
  CHECK_EQ((int)textW("", 1), 0);
  // Centrar es poner la mitad del hueco a cada lado.
  CHECK_EQ(centerX("HOLA", 1), CX - (int)textW("HOLA", 1) / 2);
  // Un texto mas ancho que la pantalla no puede devolver un x positivo
  // que lo deje colgando: tiene que empezar a la izquierda.
  CHECK(centerX("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789ABCDEFGHIJ", 3) < CX);
}

TEST(interfaz, la_zona_del_bicho_esta_dentro_de_la_pantalla) {
  arranca();
  CHECK_MSG(inPetZone(CX, CY), "el centro tiene que ser zona del bicho");
  // y las esquinas, que ni siquiera se ven, no
  CHECK(!inPetZone(2, 2));
  CHECK(!inPetZone(463, 463));
  // nada de lo que inPetZone acepta puede caer fuera del circulo
  for (int16_t y = 0; y < 466; y += 3) {
    for (int16_t x = 0; x < 466; x += 3) {
      if (!inPetZone(x, y)) continue;
      int dx = x - CX, dy = y - CY;
      CHECK_MSG(dx * dx + dy * dy <= R * R,
                "inPetZone acepta " + std::to_string(x) + "," +
                std::to_string(y) + ", que esta fuera del panel");
    }
  }
}

TEST(interfaz, el_degradado_de_color_empieza_y_acaba_donde_debe) {
  CHECK_EQ((int)lerp565(0x0000, 0xFFFF, 0, 10), 0x0000);
  CHECK_EQ((int)lerp565(0x0000, 0xFFFF, 10, 10), 0xFFFF);
  // y por el medio no se sale de los dos extremos en ningun canal
  for (int i = 0; i <= 10; i++) {
    uint16_t c = lerp565(0x0000, 0xFFFF, i, 10);
    CHECK((c >> 11) <= 31);
    CHECK(((c >> 5) & 63) <= 63);
    CHECK((c & 31) <= 31);
  }
  // n = 0 no puede dividir por cero
  (void)lerp565(0x1234, 0x4321, 0, 0);
}

TEST(interfaz, la_hora_del_escenario_es_una_hora) {
  arranca();
  int h = sceneHour();
  CHECK_RANGE(h, 0, 23);
}

TEST(interfaz, tocar_por_todas_partes_no_rompe_nada) {
  // Barrido bruto: 400 toques repartidos por la pantalla, incluidas las
  // esquinas que no existen. Ninguno puede dejar el firmware en un
  // estado imposible ni parar el repintado. Es el equivalente de un
  // nino con el aparato en la mano.
  arranca();
  // os toques acertam a pilula de idioma dos ajustes: o idioma volta no
  // fim, senao o teste seguinte le as frases de combate em outra lingua
  const Lang langAntes = gLang;
  randomSeed(4242);
  std::string roto;
  for (int i = 0; i < 400 && roto.empty(); i++) {
    toca((int16_t)random(466), (int16_t)random(466));
    if (pet.speciesId != -1 &&
        (pet.speciesId < 1 || pet.speciesId > DEX_COUNT))
      roto = "la especie se fue a " + std::to_string(pet.speciesId);
    else if (pet.fullness > 100 || pet.joy > 100 || pet.energy > 100 ||
             pet.hygiene > 100)
      roto = "una barra paso de 100 en el toque " + std::to_string(i);
    else if (pet.hpLeft > pet.maxHp())
      roto = "PS " + std::to_string(pet.hpLeft) + " de " +
             std::to_string(pet.maxHp());
  }
  CHECK_MSG(roto.empty(), roto);
  uint32_t antes = g_frames;
  avanza(1000);
  CHECK_MSG(g_frames > antes, "tras el barrido la pantalla dejo de pintar");
  setLang(langAntes);
  applyLangFont();
  avanza(100);
}

TEST(interfaz, los_gestos_no_dejan_la_ficha_fuera_de_rango) {
  // La ficha tiene 7 paginas y se navega arrastrando. Pasarse por
  // cualquiera de los dos lados saldria por un indice que no existe.
  arranca();
  onSwipeV(1);                 // abrir la ficha
  avanza(400);
  for (int i = 0; i < 40; i++) { onSwipe(1); avanza(80); }
  for (int i = 0; i < 40; i++) { onSwipe(-1); avanza(80); }
  onSwipeV(-1);                // y cerrarla
  avanza(400);
  CHECK_MSG(g_frames > 0, "el firmware dejo de pintar navegando la ficha");
  int vivos = pixelesDistintos(0);
  CHECK_MSG(vivos > 1000, "tras navegar la ficha la pantalla quedo vacia");
}

TEST(interfaz, ninguna_pantalla_se_queda_en_blanco) {
  // El fallo que un jugador NO puede sortear: una pantalla que no pinta
  // nada. No se ve en un test de logica (el estado es correcto) ni en
  // el simulador (que guarda el PNG en negro sin quejarse).
  //
  // Se recorre a toques y se exige que SIEMPRE haya contenido: algo
  // mas que un rectangulo de un color.
  arranca();
  randomSeed(909);
  int peor = 466 * 466, cuando = -1;
  for (int i = 0; i < 150; i++) {
    toca((int16_t)random(466), (int16_t)random(466));
    int c = pixelesConContenido();
    if (c < peor) { peor = c; cuando = i; }
  }
  CHECK_MSG(peor > 2000,
            "tras el toque " + std::to_string(cuando) + " la pantalla se "
            "quedo con solo " + std::to_string(peor) + " pixeles de "
            "contenido: esta practicamente en blanco");
}

// --- los 86 guiones del simulador, pero CON comprobaciones -----------
//
// Esos ficheros llevan meses escritos (elegir inicial, pelear, la
// ficha, la bolsa, el gimnasio, capturar, la Pokedex...) y lo unico
// que hacian era dibujar PNG que miraba un ojo cuando se acordaba.
// Pasarlos aqui, con los invariantes puestos despues de CADA comando,
// es lo que convierte 4000 lineas de .ino sin red en 4000 lineas con
// una red fina. Y de paso dispara la cobertura, porque recorren
// pantallas a las que un test escrito a mano no llega.
// CADA GUION EMPIEZA EN CASA. Los guiones estan escritos para un arranque
// en frio y aqui corren uno detras de otro sobre el mismo firmware: lo que
// uno deja abierto lo hereda el siguiente. Hasta la leva 15 no se notaba
// porque casi cualquier toque cerraba casi cualquier pantalla; desde que
// fallar un blanco ya no cierra, la lista de la revancha se quedaba
// abierta para los veinte guiones de detras (y para los testes de despues,
// que pintaban la ficha DEBAJO de ella).
static void recogeTodoT();     // definido al final, con el mapa de toques

TEST(guiones, los_86_del_simulador_pasan_con_los_invariantes_puestos) {
  arranca();
  // alguns guioes trocam o idioma: ele volta no fim (desde a leva 75 as
  // frases de combate seguem o idioma e o teste seguinte as leria em outro)
  const Lang langAntes = gLang;
  std::vector<std::string> fallos;
  int pasados = 0, comandos = 0;

  for (const std::string &g : guionesDelSimulador()) {
    std::string roto;
    bool ok = corriendoGuion(
        DESKTOP + "/" + g,
        [](uint32_t ms) { avanza(ms); },
        nullptr,                       // no se guardan fotogramas
        [&](const std::string &linea) {
          comandos++;
          if (!roto.empty()) return;
          if (pet.speciesId != -1 &&
              (pet.speciesId < 1 || pet.speciesId > DEX_COUNT))
            roto = "la especie se fue a " + std::to_string(pet.speciesId);
          else if (pet.fullness > 100 || pet.joy > 100 ||
                   pet.energy > 100 || pet.hygiene > 100)
            roto = "una barra paso de 100";
          else if (pet.hpLeft > pet.maxHp())
            roto = "PS " + std::to_string(pet.hpLeft) + " de " +
                   std::to_string(pet.maxHp());
          else if (pet.level() < 1 || pet.level() > 100)
            roto = "nivel " + std::to_string(pet.level());
          else if (pixelesConContenido() < 500)
            roto = "la pantalla se quedo casi vacia";
          if (!roto.empty()) roto += "  (tras: " + linea + ")";
        });
    recogeTodoT();
    if (!ok) fallos.push_back(g + ": no se puede abrir");
    else if (!roto.empty()) fallos.push_back(g + ": " + roto);
    else pasados++;
  }

  printf("        %d guiones, %d comandos\n", pasados, comandos);
  CHECK_MSG(pasados > 50, "solo pasaron " + std::to_string(pasados) +
                          " guiones: ¿se estan encontrando los .txt?");
  for (const std::string &f : fallos) CHECK_MSG(false, f);
  setLang(langAntes);
  applyLangFont();
  avanza(100);
}

// NINGUNA FRASE SE SALE DEL PANEL, EN NINGUN IDIOMA.
//
// Lo pregunto el dueno al hablar de traducir: "vc consegue tomar cuidado
// com as CAIXAS DE TEXTO em si? pq linguas diferentes, tamanhos de
// palavras diferentes". Tener cuidado no sirve: son 110 frases por 8
// idiomas repartidas por decenas de pantallas, y el aleman hace palabras
// del doble que el ingles. Que lo mire la maquina.
//
// Se recorren los 86 guiones del simulador una vez POR IDIOMA, con el
// metro del texto puesto (desktop/shim/gfx.cpp): apunta cada pixel
// ENCENDIDO de cada letra y lo compara con el circulo de 466. Mira los
// pixeles encendidos y no la caja de la letra porque la celda de la
// fuente lleva columnas en blanco y daria falsos avisos.
//
// Lo que este test NO ve: una frase que se salga de su CUADRO pero siga
// dentro del circulo. Eso no se puede comprobar sin una tabla de
// cuadro-por-frase; lo que si se hace es que cada sitio donde el texto
// puede crecer mida con textW() y encoja o parta (drawWrapped).
static const char *const IDIOMA[LANG_COUNT] = {
  "ES", "EN", "FR", "DE", "IT", "PT", "JA", "KO",
};

TEST(idiomas, ninguna_frase_se_sale_del_panel_redondo) {
  arranca();
  Lang antes = gLang;
  // texto -> (cuanto se sale como maximo, donde, en que idiomas)
  std::map<std::string, std::tuple<int, int, int, std::string>> peor;
  int mirados = 0;

  for (int li = 0; li < LANG_COUNT; li++) {
    setLang((Lang)li);
    for (const std::string &g : guionesDelSimulador()) {
      mockTextoReset();
      corriendoGuion(DESKTOP + "/" + g,
                     [](uint32_t ms) { avanza(ms); }, nullptr, nullptr);
      gfx->cierraRun();            // el ultimo run tambien cuenta
      recogeTodoT();
      mirados++;
      for (const TextoSalido &t : mockTextoFuera()) {
        auto it = peor.find(t.texto);
        if (it == peor.end()) {
          peor[t.texto] = std::make_tuple(t.lejos, t.x, t.y,
                                          std::string(IDIOMA[li]));
        } else {
          std::string &idiomas = std::get<3>(it->second);
          if (idiomas.find(IDIOMA[li]) == std::string::npos)
            idiomas += std::string(" ") + IDIOMA[li];
          if (t.lejos > std::get<0>(it->second)) {
            std::get<0>(it->second) = t.lejos;
            std::get<1>(it->second) = t.x;
            std::get<2>(it->second) = t.y;
          }
        }
      }
    }
  }
  setLang(antes);

  // de la que mas se sale a la que menos
  std::vector<std::pair<int, std::string>> orden;
  for (auto &kv : peor) orden.push_back({std::get<0>(kv.second), kv.first});
  std::sort(orden.rbegin(), orden.rend());

  printf("        %d pasadas (%d guiones x %d idiomas), %d frases distintas "
         "fuera\n", mirados, mirados / LANG_COUNT, (int)LANG_COUNT,
         (int)orden.size());
  for (size_t i = 0; i < orden.size() && i < 20; i++) {
    auto &d = peor[orden[i].second];
    printf("          %3d px  \"%s\"  en %d,%d  [%s]\n", std::get<0>(d),
           orden[i].second.c_str(), std::get<1>(d), std::get<2>(d),
           std::get<3>(d).c_str());
  }
  CHECK_MSG(orden.empty(),
            std::to_string(orden.size()) +
            " frases distintas se salen del panel (la peor: \"" +
            (orden.empty() ? "" : orden[0].second) + "\", " +
            std::to_string(orden.empty() ? 0 : orden[0].first) + " px)");
}

TEST(interfaz, despues_del_trote_la_pantalla_sigue_entera) {
  // Lo mismo DESPUES de todo lo anterior (400 toques al azar y la ficha
  // navegada entera), que es cuando de verdad se destapan las cosas.
  arranca();
  avanza(600);
  CHECK_MSG(pixelesConContenido() > 2000,
            "tras el trote la pantalla se quedo casi vacia");
  uint32_t antes = g_frames;
  avanza(1000);
  CHECK_MSG(g_frames > antes, "y dejo de repintar");
}

// ---------------------------------------------------------------------
// El relevo de un ENTRENADOR no regala un ataque.
//
// Lo conto el dueno jugando: "derroto el pokemon de un treinador, entra
// el proximo y mi pokemon ataca ele de novo". Contra un salvaje no
// pasaba, y por eso ni los guiones ni nadie lo vio: al tumbar a un
// salvaje, knockOut() pone `over` y la maquina de fases entra por la
// rama del final. Un entrenador con banquillo deja `over` en FALSE a
// proposito -- para poder sacar al siguiente -- y esa rama miraba solo
// `over` y "ha caido el MIO". El de enfrente no se miraba, asi que el
// turno seguia y lanzaba el segundo golpe pendiente contra el recien
// llegado.
//
// Esto no prueba la pantalla: prueba la REGLA. Despues de que caiga
// cualquiera de los dos, no puede quedar un golpe en la recamara.
// ---------------------------------------------------------------------
// Cierra el combate que haya abierto y espera a que se note. Los tests
// comparten `bt` y `pet`, asi que el que abre uno tiene que recogerlo.
static void cierraElCombate() {
  if (!bt.open) return;
  bt.b.over = true;
  bt.phase = BP_END;
  bt.until = millis();
  for (int i = 0; i < 300 && bt.open; i++) avanza(16);
}

// LOS OBJETOS AL ACABAR EL COMBATE. Lo que le quitan a un bicho (Desarme,
// Ladron, Trapicheo) vuelve; lo que GASTA (una baya) se pierde. Antes se
// miraba "volvio con las manos vacias", y un Desarme del rival borraba el
// objeto para siempre —— la piedra mega incluida.
static uint8_t objetoCon(uint8_t fx) {
  for (uint8_t i = 1; i < ITEM_COUNT; i++)
    if (ITEM_TBL[i].fx == fx) return i;
  return 0;
}

// Arranca un combate con el principal llevando `mio` y un suplente llevando
// `suyo`. Devuelve el hueco del suplente.
static uint8_t combateConObjetos(uint8_t mio, uint8_t suyo) {
  arranca();
  if (pet.awaitingStarter()) pet.chooseStarter(1);
  while (pet.isEgg()) pet.eggTap();
  pet.sleeping = false;
  pet.energy = 100;
  pet.exp = Pet::expForLevel(30);
  pet.hpLeft = pet.maxHp();
  pet.heldItem = mio;
  for (uint8_t i = 0; i < TEAM_SLOTS; i++)
    if (i != pet.mainSlot) pet.team[i] = StoredMon{};
  StoredMon sup{};
  sup.species = (pet.speciesId == 7) ? 25 : 7;
  sup.exp = Pet::expForLevel(10);
  sup.geneAtk = sup.geneDef = sup.geneSpe = 100;
  sup.moves[0] = MOVE_STRUGGLE;
  sup.ppLeft[0] = 5;
  sup.hpLeft = 50;
  sup.item = suyo;
  CHECK_MSG(pet.addToTeam(sup), "no entro el suplente en el equipo");
  battleStart(16, 10, -1, false, -1);
  CHECK_MSG(bt.open, "el combate no se abrio");
  for (int i = 0; i < 400 && bt.phase != BP_CHOOSE; i++) avanza(16);
  bt.b.side[BT_FOE].hpMax = 9999;
  bt.b.side[BT_FOE].hp = 9999;
  uint8_t hueco = 0xFF;
  for (uint8_t i = 0; i < TEAM_SLOTS; i++)
    if (i != pet.mainSlot && pet.team[i].species == sup.species) hueco = i;
  CHECK_MSG(hueco != 0xFF, "no se encontro el hueco del suplente");
  return hueco;
}

static void relevaA(uint8_t hueco) {
  bt.phase = BP_PARTY;
  CHECK_MSG(btPartyTap(96, 128 + hueco * 44 + 15), "la lista no cogio el toque");
  CHECK_MSG(bt.mySlot == hueco, "no llego a relevar");
  // que el rival acabe su turno y se vuelva a poder elegir
  for (int i = 0; i < 900 && bt.open && bt.phase != BP_CHOOSE; i++) {
    bt.b.side[BT_PLAYER].hp = bt.b.side[BT_PLAYER].hpMax;   // que no caiga
    avanza(16);
  }
}

TEST(combate_ui, el_objeto_que_le_quitan_al_principal_vuelve_al_acabar) {
  uint8_t restos = objetoCon(IT_LEFTOVERS);
  combateConObjetos(restos, 0);
  CHECK_EQ((int)bt.b.side[BT_PLAYER].item, (int)restos);
  bt.b.side[BT_PLAYER].perderObjeto();         // un Desarme del rival
  cierraElCombate();
  CHECK_EQ((int)pet.heldItem, (int)restos);
}

TEST(combate_ui, el_objeto_que_GASTA_el_principal_se_pierde) {
  uint8_t baya = objetoCon(IT_BERRY_HEAL);
  CHECK_MSG(baya, "premisa: hay una baya de cura");
  combateConObjetos(baya, 0);
  bt.b.side[BT_PLAYER].perderObjeto(true);     // se la comio
  cierraElCombate();
  CHECK_EQ((int)pet.heldItem, 0);
}

TEST(combate_ui, al_suplente_tambien_le_vuelve_lo_que_le_quitan) {
  uint8_t restos = objetoCon(IT_LEFTOVERS), baya = objetoCon(IT_BERRY_HEAL);
  uint8_t hueco = combateConObjetos(baya, restos);
  relevaA(hueco);
  CHECK_EQ((int)bt.b.side[BT_PLAYER].item, (int)restos);
  bt.b.side[BT_PLAYER].perderObjeto();         // Desarme
  // sale y vuelve a entrar EN EL MISMO COMBATE: sigue sin el
  relevaA(pet.mainSlot);
  CHECK_EQ((int)bt.b.side[BT_PLAYER].item, (int)baya);
  relevaA(hueco);
  CHECK_EQ((int)bt.b.side[BT_PLAYER].item, 0);
  cierraElCombate();
  CHECK_EQ((int)pet.team[hueco].item, (int)restos);   // y al acabar, vuelve
  CHECK_EQ((int)pet.heldItem, (int)baya);             // el otro, intacto
}

TEST(combate_ui, lo_que_gasta_un_suplente_se_pierde_aunque_acabe_otro_en_pista) {
  uint8_t restos = objetoCon(IT_LEFTOVERS), baya = objetoCon(IT_BERRY_HEAL);
  uint8_t hueco = combateConObjetos(restos, baya);
  relevaA(hueco);
  bt.b.side[BT_PLAYER].perderObjeto(true);     // el suplente se come su baya
  relevaA(pet.mainSlot);                       // y acaba el principal en pista
  bt.b.side[BT_PLAYER].perderObjeto(true);     // que gasta lo suyo tambien...
  bt.b.side[BT_PLAYER].item = restos;          // ...pero se lo devuelven (Reciclaje)
  cierraElCombate();
  CHECK_EQ((int)pet.team[hueco].item, 0);
  CHECK_EQ((int)pet.heldItem, (int)restos);
}

TEST(combate_ui, comerse_una_baya_ROBADA_no_le_quita_su_objeto) {
  uint8_t restos = objetoCon(IT_LEFTOVERS), baya = objetoCon(IT_BERRY_HEAL);
  combateConObjetos(restos, 0);
  Combatant &yo = bt.b.side[BT_PLAYER];
  yo.perderObjeto();                           // le tiran los Restos
  yo.item = baya;                              // roba una baya...
  yo.perderObjeto(true);                       // ...y se la come
  cierraElCombate();
  CHECK_EQ((int)pet.heldItem, (int)restos);
}

static uint8_t ganaElCombate();   // definido mas abajo, con su explicacion

TEST(combate_ui, robarle_a_un_salvaje_lo_mete_en_la_mochila) {
  uint8_t restos = objetoCon(IT_LEFTOVERS);
  combateConObjetos(0, 0);
  uint16_t thief = 0;
  for (uint16_t m = 0; m < MOVE_COUNT; m++)
    if (!strcmp(MOVE_TBL[m].name, "THIEF")) thief = m;
  CHECK_MSG(thief, "premisa: existe THIEF");
  Combatant &yo = bt.b.side[BT_PLAYER];
  Combatant &el = bt.b.side[BT_FOE];
  yo.moves[0] = thief; yo.usesLeft[0] = 20; yo.nMoves = 1;
  yo.hpMax = 900; yo.hp = 900;
  el.item = restos;
  bt.foeItemIni = restos;
  el.nMoves = 0;
  uint8_t tenia = pet.bag[restos];
  bt.sel = 0;
  toca(233, (BT_CARD_Y0_T + BT_CARD_Y1_T) / 2);
  std::string guion, ultima;
  bool salio = false;
  for (int i = 0; i < 900; i++) {
    avanza(16);
    if (bt.phase != BP_CHOOSE) salio = true;
    if (bt.msg[0] && ultima != bt.msg) { ultima = bt.msg; guion += "\n    | " + ultima; }
    if (salio && bt.phase == BP_CHOOSE) break;
    if (!bt.open) break;
  }
  CHECK_EQ((int)el.item, 0);
  CHECK_EQ((int)pet.bag[restos], (int)tenia + 1);
  CHECK_MSG(guion.find("stole") != std::string::npos,
            "nadie dijo que se lo habia robado:" + guion);
  // y al ganar NO se lo vuelven a dar como botin
  randomSeed(2025);
  CHECK_EQ((int)ganaElCombate(), 0);
}

// GANAR de verdad: el rival cae y la pantalla pasa por BP_FAINT, que es
// donde se cobra el botin. cierraElCombate() va derecho a BP_END y NO pasa
// por ahi: con el, un test del botin daba verde con el fallo puesto.
// Devuelve lo que el combate dio como "lo que llevaba el rival".
static uint8_t ganaElCombate() {
  bt.b.side[BT_FOE].hp = 0;
  bt.b.over = true;
  bt.b.winner = BT_PLAYER;
  bt.fainter = BT_FOE;
  bt.phase = BP_FAINT;
  bt.until = millis();
  uint8_t llevaba = 0;
  bool cobrado = false;
  for (int i = 0; i < 1200 && bt.open; i++) {
    avanza(16);
    if (!cobrado && bt.phase != BP_FAINT) { cobrado = true; llevaba = bt.loot[0]; }
    if (i % 40 == 39) toca(233, 233);          // las pantallas de premio
  }
  cierraElCombate();
  return llevaba;
}

TEST(combate_ui, al_ganarle_a_un_salvaje_te_llevas_lo_que_llevaba) {
  // La premisa de los dos testes de abajo: por este camino el botin SE cobra.
  uint8_t restos = objetoCon(IT_LEFTOVERS);
  combateConObjetos(0, 0);
  bt.b.side[BT_FOE].item = restos;
  bt.foeItemIni = restos;
  uint8_t tenia = pet.bag[restos];
  randomSeed(2025);
  CHECK_EQ((int)ganaElCombate(), (int)restos);
  CHECK_MSG(pet.bag[restos] > tenia, "no llego a la mochila");
}

TEST(combate_ui, tras_un_trapicheo_el_botin_no_duplica_tu_objeto) {
  uint8_t restos = objetoCon(IT_LEFTOVERS);
  combateConObjetos(restos, 0);
  // Trapicheo contra un salvaje sin nada: ahora MI objeto lo lleva el.
  bt.b.side[BT_PLAYER].item = IT_NONE;
  bt.b.side[BT_FOE].item = restos;
  bt.foeItemIni = IT_NONE;
  randomSeed(2025);
  CHECK_EQ((int)ganaElCombate(), 0);                 // no es botin: es mio
  CHECK_EQ((int)pet.heldItem, (int)restos);          // y lo mio vuelve
}

// Un turno del jugador por la pantalla, con el rival quieto. Devuelve lo
// que fue diciendo la caja.
static std::string turnoCon(const char *golpe, bool rivalProtegido) {
  combateConObjetos(0, 0);
  uint16_t mv = 0;
  for (uint16_t m = 0; m < MOVE_COUNT; m++)
    if (!strcmp(MOVE_TBL[m].name, golpe)) mv = m;
  CHECK_MSG(mv, "premisa: el golpe existe");
  Combatant &yo = bt.b.side[BT_PLAYER];
  Combatant &el = bt.b.side[BT_FOE];
  yo.moves[0] = mv; yo.usesLeft[0] = 20; yo.nMoves = 1;
  yo.hpMax = 900; yo.hp = 900;
  yo.stage[SG_SPE] = 6;                        // yo primero
  el.nMoves = 0;
  el.protecting = rivalProtegido;
  bt.sel = 0;
  toca(233, (BT_CARD_Y0_T + BT_CARD_Y1_T) / 2);
  std::string guion, ultima;
  bool salio = false;
  for (int i = 0; i < 900; i++) {
    avanza(16);
    if (bt.phase != BP_CHOOSE) salio = true;
    if (bt.msg[0] && ultima != bt.msg) { ultima = bt.msg; guion += "\n    | " + ultima; }
    if ((salio && bt.phase == BP_CHOOSE) || !bt.open) break;
  }
  return guion;
}

TEST(combate_ui, estrellarse_contra_un_protect_se_dice) {
  std::string g = turnoCon("HIGHJUMPKICK", true);
  CHECK_MSG(g.find("protected itself") != std::string::npos,
            "premisa: se lo pararon:" + g);
  CHECK_MSG(g.find("kept going and crashed") != std::string::npos,
            "se estrello y la caja no lo dijo:" + g);
  // la mitad de 900 se fue en el porrazo (y unos pocos mas en el Forcejeo
  // del rival, que no tiene golpes)
  CHECK_MSG(bt.b.side[BT_PLAYER].hp <= 450 && bt.b.side[BT_PLAYER].hp > 400,
            "el porrazo son 450 de 900 y le quedan " +
                std::to_string(bt.b.side[BT_PLAYER].hp));
}

TEST(combate_ui, salpicadura_dice_que_no_paso_nada) {
  std::string g = turnoCon("SPLASH", false);
  CHECK_MSG(g.find("But nothing happened!") != std::string::npos,
            "Salpicadura tenia que decir que no paso nada:" + g);
  std::string mio = dexName(bt.b.side[BT_PLAYER].asDex);
  size_t usa = g.find(mio + " used SPLASH");
  CHECK_MSG(usa != std::string::npos, "premisa: la uso:" + g);
  size_t fallo = g.find("But it failed!", usa);
  size_t rival = g.find(" used ", usa + 5);
  CHECK_MSG(fallo == std::string::npos || (rival != std::string::npos && fallo > rival),
            "dijo 'But it failed!' de la Salpicadura:" + g);
}

// EL ORDEN DE LO QUE SE LEE EN LA CAJA.
//
// Reportado tal cual: "el pokemon hace la animacion del ataque, dice que
// ataco, y despues dice que esta paralizado/dormido". Y era verdad: la
// pregunta "se mueve este turno?" vivia dentro de useMove(), que solo corre
// cuando se acaba el anuncio. O sea que la caja tenia que escribir "X uso
// Arañazo!" sin saber todavia si X iba a moverse, y desdecirse a los dos
// segundos.
//
// El test de battle.cpp clava el mecanismo (una sola tirada); este clava lo
// que de verdad se reporto, que es el ORDEN EN PANTALLA: la frase del estado
// no puede llegar detras de un "used".
TEST(combate_ui, el_estado_se_dice_antes_de_anunciar_el_golpe) {
  arranca();
  // PRIMERO sacar la partida de la pantalla de elegir inicial. En onTap()
  // ese ramo va DELANTE del combate y acaba en `return`:
  //
  //     if (pet.awaitingStarter()) { ...; return; }
  //     if (bt.open) { battleTap(x, y); return; }
  //
  // asi que mientras `awaitingStarter()` sea cierto ningun toque llega a
  // battleTap(). El combate se abre igual (battleStart no pasa por el
  // toque) y se queda clavado en BP_CHOOSE: bt.open a 1, la caja muda y
  // `frases` vacia. Fallaba a ratos porque ese estado depende de lo que
  // hayan dejado los tests anteriores en la NVS simulada.
  if (pet.awaitingStarter()) pet.chooseStarter(1);
  while (pet.isEgg()) pet.eggTap();
  pet.sleeping = false;
  pet.energy = 100;
  // El nivel se fija en vez de heredar el del huevo: no era la causa del
  // fallo, pero asi el test no depende de con que nivel se nazca.
  pet.exp = Pet::expForLevel(20);
  pet.hpLeft = pet.maxHp();
  battleStart(16, 10);                 // salvaje, sin gimnasio
  CHECK_MSG(bt.open, "el combate no se abrio");
  for (int i = 0; i < 400 && bt.phase != BP_CHOOSE; i++) avanza(16);
  CHECK_MSG(bt.phase == BP_CHOOSE, "no se llego a elegir golpe");

  // bien dormido: seis turnos, no despierta en este
  bt.b.side[BT_PLAYER].applyStatus(ST_SLEEP);
  bt.b.side[BT_PLAYER].statusTurns = 6;
  std::string mio = dexName(bt.b.side[BT_PLAYER].asDex);

  // Y EL MIO SE MUEVE PRIMERO, dicho a las claras. Lo que se mide abajo
  // es el ORDEN de las frases, y la del sueno solo puede ir la primera
  // si a mi me toca antes. Eso NO estaba garantizado: el bicho entra al
  // combate bajado al techo de la region (nivel 11 con cero insignias,
  // ver Pet::battleLevel), asi que el Pidgey de nivel 10 le gana la
  // velocidad —— 13 contra 16, medido. El test pasaba por suerte y
  // medía lo que no creía. Con +6 escalones la premisa es del test y no
  // del azar de los genes del salvaje.
  bt.b.side[BT_PLAYER].applyStage(SG_SPE, +6);
  CHECK_MSG(bt.b.movesFirst(BT_PLAYER, bt.sel, bt.b.pickFoeMove()),
            "premisa del test: el mio tiene que mover primero");

  // y se usa el golpe elegido, tocando la tarjeta como se toca jugando
  printf("        [diag] awaitingStarter %d especie %d\n",
         (int)pet.awaitingStarter(), (int)pet.speciesId);
  printf("        [diag] antes: niv %d fase %d msg[%s]\n",
         (int)pet.level(), (int)bt.phase, bt.msg);
  toca(233, (BT_CARD_Y0_T + BT_CARD_Y1_T) / 2);
  printf("        [diag] tras:  fase %d msg[%s] open %d\n",
         (int)bt.phase, bt.msg, (int)bt.open);

  // todo lo que pasa por la caja, en orden y sin repetir
  std::vector<std::string> frases;
  for (int i = 0; i < 500; i++) {
    avanza(16);
    if (bt.msg[0] && (frases.empty() || frases.back() != bt.msg))
      frases.push_back(bt.msg);
    if (!bt.open) break;
  }

  std::string guion;
  for (const std::string &f : frases) guion += "\n    | " + f;

  int dormido = -1, usado = -1;
  for (int i = 0; i < (int)frases.size(); i++) {
    if (dormido < 0 && frases[i].find("asleep") != std::string::npos)
      dormido = i;
    // "X used Y!" DEL NUESTRO. El rival si ataca este turno, y su anuncio
    // es correcto: lo que no puede salir es el nuestro, que no se movio.
    if (usado < 0 && frases[i].find(mio + " used ") != std::string::npos)
      usado = i;
  }

  CHECK_MSG(dormido >= 0, "nadie dijo que estaba dormido:" + guion);
  CHECK_MSG(usado < 0,
            "se anuncio un golpe de " + mio +
            " que nunca llego a salir, porque estaba dormido:" + guion);
  CHECK_MSG(dormido == 0,
            "la primera frase del turno tendria que ser la del sueno:" + guion);

  // Y SE CIERRA. `bt` es uno solo para toda la bateria: dejarlo abierto le
  // deja el combate puesto al test siguiente, que llama a battleStart() y
  // se encuentra con que no hace nada. Costo un rato averiguarlo.
  cierraElCombate();
}

// ATRAPAS ALGO -> SE VE. Hasta ahora capturar terminaba en una linea de
// texto que se iba sola y el bicho desaparecia dentro de la caja: para
// mirarlo habia que acordarse de abrir la Pokedex y buscarlo a mano entre
// 1025. Lo que uno quiere despues de atrapar algo es verlo.
//
// El test entra por el camino de verdad (BP_WOBBLE -> btResolveCatch ->
// BP_CAUGHT -> cierre) en vez de poner `caught` a mano, porque lo que puede
// romperse es justo el orden: si la ficha se abriera en BP_CAUGHT, se
// comeria el resumen de la victoria y el botin.
// EL FALLO DE LAS CINCO TENTATIVAS.
//
// Reportado asi: "quando vc simplesmente aumenta o pokemon do lado do
// player, vc deixa o adversario gigante....ja eh o que, a quinta
// tentativa?". Y era literal: btDrawSide llamaba a drawPmdActM sin pasar
// `dex`, cuyo valor por defecto es -1 = "el bicho que se cria", asi que
// el sprite del RIVAL se escalaba ADEMAS con el screenH del bicho del
// JUGADOR. Medido en el simulador con tres capturas del MISMO Caterpie:
// salia pequeno con un Caterpie delante, mediano con un Dratini y enorme
// con un Pidgeotto. Por eso cuatro intentos de ajustar la tabla de
// tamanos no arreglaron nada: el fallo no estaba en los numeros.
//
// Lo que se afirma es un invariante LOCAL: lo que el combate pide y lo
// que acaba pintandose tienen que ser lo mismo. Mi primer intento
// comparaba dos combates con bichos propios distintos y se quedo VERDE
// con el fallo puesto a proposito —— el estado del bicho entre combates
// no era el que yo creia, y el test no probaba nada. Un invariante que
// se cierra dentro de un fotograma no se puede enganar asi.
TEST(combate_ui, el_rival_sale_del_tamano_que_el_combate_pidio) {
  arranca();
  cierraElCombate();
  while (pet.isEgg()) pet.eggTap();
  pet.sleeping = false;
  pet.energy = 100;
  pet.hpLeft = pet.maxHp();
  battleStart(10, 10);                 // Caterpie
  CHECK_MSG(bt.open, "el combate no se abrio");
  btAltoDibujado[BT_FOE] = 0;
  for (int n = 0; n < 400 && !btAltoDibujado[BT_FOE]; n++) avanza(16);
  CHECK_MSG(btAltoDibujado[BT_FOE] > 0, "el rival no llego a pintarse");

  for (int lado = 0; lado < 2; lado++) {
    int pide = btPedido[lado], sale = btAltoDibujado[lado];
    if (!pide || !sale) continue;      // ese lado no se pinto este frame
    int dif = pide > sale ? pide - sale : sale - pide;
    // 10%: el bicho RESPIRA (+-2% en drawPmdActM) y la escala entera
    // obliga a redondear. El fallo valia casi el doble.
    CHECK_MSG(dif * 10 <= pide,
              std::string(lado == BT_FOE ? "el rival" : "el tuyo") +
              " se pidio a " + std::to_string(pide) + " px y salio a " +
              std::to_string(sale) + ": el dibujante le esta anadiendo un "
              "tamano por su cuenta (el del bicho del jugador)");
  }
  cierraElCombate();
}

// Y la otra mitad, para que "arreglarlo" dejando a todos los rivales del
// mismo tamano no pase: el rival SI tiene que cambiar segun QUE rival es.
TEST(combate_ui, rivales_distintos_salen_de_tamanos_distintos) {
  int16_t medido[2] = {0, 0};
  const int16_t CHICO = 10;    // Caterpie, 2,9 kg
  const int16_t GRANDE = 321;  // Wailord, 398 kg

  for (int i = 0; i < 2; i++) {
    arranca();
    cierraElCombate();
    while (pet.isEgg()) pet.eggTap();
    pet.sleeping = false;
    pet.energy = 100;
    pet.hpLeft = pet.maxHp();
    battleStart(i ? GRANDE : CHICO, 10);
    CHECK_MSG(bt.open, "el combate no se abrio");
    btAltoDibujado[BT_FOE] = 0;
    for (int n = 0; n < 400 && !btAltoDibujado[BT_FOE]; n++) avanza(16);
    medido[i] = btAltoDibujado[BT_FOE];
    cierraElCombate();
  }

  CHECK_MSG(medido[1] > medido[0] + 20,
            "un Wailord sale a " + std::to_string((int)medido[1]) +
            " px y un Caterpie a " + std::to_string((int)medido[0]) +
            ": el tamano del rival no dice nada");
}

// "as pokebolas, quando arremessadas em pokemons que usaram um golpe que
// sai da tela, tipo bounce, fly, dig... estao capturando mesmo assim no
// turno que estao fora". Agora a bola chega, nao ha ninguem, e erra.
static void volcaPantalla(const char *nombre);
extern bool cardOpen;
extern bool gestosOn;
TEST(combate_ui, a_bola_ERRA_quem_esta_escondido) {
  arranca();
  cierraElCombate();
  if (pet.awaitingStarter()) pet.chooseStarter(1);
  gestosOn = false;
  galleryOpen = false;
  cardOpen = false;
  while (pet.isEgg()) pet.eggTap();
  pet.sleeping = false;
  pet.energy = 100;
  pet.hpLeft = pet.maxHp();
  battleStart(16, 8);                  // Pidgey
  CHECK_MSG(bt.open, "el combate no se abrio");
  for (int i = 0; i < 400 && bt.phase != BP_CHOOSE; i++) avanza(16);
  CHECK_MSG(bt.phase == BP_CHOOSE, "premissa: o combate tem que abrir");
  bt.b.side[BT_FOE].hiding = HID_SKY;  // usou Fly no turno anterior
  bt.ballId = BALL_ID[BALL_TIERS - 1]; // nem a Master Ball alcanca
  bt.foeInBall = false;
  bt.phase = BP_THROW;
  bt.until = millis() + 700;
  avanza(350);
  volcaPantalla("bola_voando");
  for (int i = 0; i < 100 && bt.phase == BP_THROW; i++) avanza(16);
  CHECK_MSG(bt.phase != BP_SUCK && bt.phase != BP_WOBBLE,
            "a bola engoliu um bicho que estava no ceu");
  CHECK_MSG(!strcmp(bt.msg, "The ball missed!"),
            std::string("a mensagem foi \"") + bt.msg + "\"");
  CHECK_MSG(!bt.caught, "e nao pode ter capturado");
  cierraElCombate();
}

// So para OLHAR: o forcejeo com a sombra da bola e o TAP!.
TEST(combate_ui, preview_forcejeo_da_bola) {
  arranca();
  cierraElCombate();
  if (pet.awaitingStarter()) pet.chooseStarter(1);
  gestosOn = false;
  galleryOpen = false;
  cardOpen = false;
  while (pet.isEgg()) pet.eggTap();
  pet.sleeping = false;
  pet.energy = 100;
  pet.hpLeft = pet.maxHp();
  battleStart(16, 8);
  for (int i = 0; i < 400 && bt.phase != BP_CHOOSE; i++) avanza(16);
  CHECK_MSG(bt.phase == BP_CHOOSE, "premissa: o combate tem que abrir");
  bt.ballId = BALL_ID[0];
  bt.foeInBall = true;
  bt.phase = BP_WOBBLE;
  bt.wobble = 3;
  bt.taps = 3;
  bt.until = millis() + 2600;
  avanza(100);
  volcaPantalla("bola_forcejeo");
  cierraElCombate();
}

uint8_t ballMult(uint8_t id);
uint8_t ballArte(uint8_t id);
static uint8_t bolaUi(const char *n) {
  for (uint8_t i = 1; i < ITEM_COUNT; i++)
    if (!strcmp(ITEM_TBL[i].name, n)) return i;
  return 0;
}

// Abre um combate contra `dex` de nivel `lv` e espera a hora de escolher.
static bool combateDeBola(int16_t dex, uint16_t lv) {
  arranca();
  cierraElCombate();
  if (pet.awaitingStarter()) pet.chooseStarter(1);
  gestosOn = false;
  galleryOpen = false;
  cardOpen = false;
  while (pet.isEgg()) pet.eggTap();
  pet.sleeping = false;
  pet.energy = 100;
  pet.hpLeft = pet.maxHp();
  battleStart(dex, lv);
  for (int i = 0; i < 400 && bt.phase != BP_CHOOSE; i++) avanza(16);
  return bt.open && bt.phase == BP_CHOOSE;
}

// A regra de cada bola, contra o rival de verdade do combate.
TEST(combate_ui, cada_bola_de_condicao_segue_a_sua_regra) {
  CHECK_MSG(combateDeBola(10, 5), "premissa: o combate tem que abrir");  // Caterpie
  uint8_t quick = bolaUi("Quick Ball"), timer = bolaUi("Timer Ball"),
          net = bolaUi("Net Ball"), level = bolaUi("Level Ball"),
          dream = bolaUi("Dream Ball"), nest = bolaUi("Nest Ball"),
          repeat = bolaUi("Repeat Ball"), premier = bolaUi("Premier Ball");
  CHECK_MSG(quick && timer && net && level && dream && nest && repeat && premier,
            "premissa: as bolas existem");
  bt.b.turn = 0;
  CHECK_EQ((int)ballMult(quick), 50);
  CHECK_EQ((int)ballMult(timer), 10);
  bt.b.turn = 20;                       // dez rodadas
  CHECK_EQ((int)ballMult(quick), 10);
  CHECK_EQ((int)ballMult(timer), 40);
  CHECK_EQ((int)ballMult(net), 35);     // Caterpie e Inseto
  CHECK_EQ((int)ballMult(nest), 36);    // nivel 5: (41-5)/10
  CHECK_EQ((int)ballMult(premier), 10);
  bt.b.side[BT_PLAYER].level = 20;      // 4x o nivel dele
  CHECK_EQ((int)ballMult(level), 80);
  bt.b.side[BT_PLAYER].level = 5;
  CHECK_EQ((int)ballMult(level), 10);
  bt.b.side[BT_FOE].status = ST_NONE;
  CHECK_EQ((int)ballMult(dream), 10);
  bt.b.side[BT_FOE].status = ST_SLEEP;
  CHECK_EQ((int)ballMult(dream), 40);
  CHECK_EQ((int)ballMult(repeat), pet.isRegistered(10) ? 35 : 10);
  // e as de escalao continuam com o numero delas
  CHECK_EQ((int)ballMult(BALL_ID[1]), 15);
  CHECK_EQ((int)ballMult(BALL_ID[BALL_TIERS - 1]), 255);
  cierraElCombate();

  CHECK_MSG(combateDeBola(16, 5), "premissa: o segundo combate abre");  // Pidgey
  CHECK_EQ((int)ballMult(net), 10);     // Normal/Voador: a Net nao ajuda
  cierraElCombate();
}

// Capturar com uma bola: ela fica com o bicho. E o arremesso usa a arte
// DELA, nao a da Poke Ball.
TEST(combate_ui, o_capturado_guarda_a_bola_e_ela_e_a_que_voa) {
  CHECK_MSG(combateDeBola(16, 8), "premissa: o combate tem que abrir");
  // Lugar garantido no time: com o time e a caixa cheios (outros testes
  // deixam assim) o capturado ia para OUTRA caixa e nao era achado aqui.
  for (uint8_t i = 0; i < TEAM_SLOTS; i++)
    if (i != pet.mainSlot) pet.team[i].species = 0;
  for (uint8_t i = 0; i < BANK_SLOTS; i++) pet.bank[i].species = 0;
  uint8_t quick = bolaUi("Quick Ball");
  CHECK_EQ((int)ballArte(quick), (int)BALL_TIERS);   // primeira de condicao
  CHECK_EQ((int)ballArte(BALL_ID[0]), 0);
  pet.bag[quick] = 1;
  bt.ballId = quick;
  bt.foeInBall = false;
  bt.phase = BP_THROW;
  bt.until = millis() + 700;
  avanza(350);
  volcaPantalla("bola_quick_voando");
  // Entra seguro: o forcejeo direto com chance resolvida a mao nao vale
  // (btResolveCatch recalcula), entao o rival vai a 1 PS e dorme.
  bt.b.side[BT_FOE].hp = 1;
  bt.b.side[BT_FOE].status = ST_SLEEP;
  bool pego = false;
  for (int tent = 0; tent < 30 && !pego; tent++) {
    bt.ballId = quick;
    bt.phase = BP_WOBBLE;
    bt.taps = 50;
    bt.until = millis();
    for (int i = 0; i < 3 && bt.phase == BP_WOBBLE; i++) avanza(16);
    pego = bt.caught;
    if (!pego) {
      for (int i = 0; i < 200 && bt.phase != BP_CHOOSE && bt.open; i++) avanza(16);
      if (!bt.open) break;
    }
  }
  CHECK_MSG(pego, "premissa: em 30 tentativas a 1 PS tinha que entrar");
  bool achou = false;
  for (uint8_t i = 0; i < TEAM_SLOTS; i++)
    if (pet.team[i].species == 16 && ballDe(pet.team[i]) == quick) achou = true;
  if (!achou)
    for (uint8_t i = 0; i < BANK_SLOTS; i++)
      if (pet.bank[i].species == 16 && ballDe(pet.bank[i]) == quick) achou = true;
  CHECK_MSG(achou, "o Pidgey capturado nao ficou com a Quick Ball");
  for (int i = 0; i < 400 && bt.open; i++) avanza(16);
  galleryOpen = false;
  galleryDetail = 0;
  cierraElCombate();
}

TEST(combate_ui, al_capturar_se_abre_la_ficha_de_ese_bicho) {
  arranca();
  cierraElCombate();
  galleryOpen = false;
  galleryDetail = 0;
  while (pet.isEgg()) pet.eggTap();
  pet.sleeping = false;
  pet.energy = 100;
  pet.hpLeft = pet.maxHp();
  const int16_t PRESA = 16;            // Pidgey
  battleStart(PRESA, 8);
  CHECK_MSG(bt.open, "el combate no se abrio");
  for (int i = 0; i < 400 && bt.phase != BP_CHOOSE; i++) avanza(16);

  // la bola ya volando, y que entre seguro
  CHECK_MSG(bt.b.side[BT_FOE].dex == PRESA,
            "el montaje no vale: en pista hay el numero " +
            std::to_string((int)bt.b.side[BT_FOE].dex));
  // Master Ball, que es la unica forma de que entre SEGURO: btResolveCatch
  // recalcula `chance` con captureChance(), asi que ponerlo a mano aqui no
  // vale de nada —— se pisa en el acto.
  bt.ballId = BALL_ID[BALL_TIERS - 1];
  bt.phase = BP_WOBBLE;
  bt.until = millis();
  // justo lo que tarda en cerrarse, sin reloj de sobra: con mas, puede
  // saltar un encuentro salvaje nuevo y el que se mira ya no es este
  for (int i = 0; i < 400 && bt.open; i++) avanza(16);

  CHECK_MSG(!bt.open, "el combate no llego a cerrarse");
  CHECK_MSG(bt.caught, "con chance 100 tenia que entrar");
  CHECK_MSG(galleryOpen, "capturar no abrio la Pokedex");
  CHECK_MSG(galleryDetail == PRESA,
            "la Pokedex se abrio en el numero " +
            std::to_string((int)galleryDetail) + " y el capturado fue el " +
            std::to_string((int)PRESA));
  CHECK_MSG(galleryPage == (PRESA - 1) / 16,
            "la pagina de debajo no es la suya: salir del detalle caeria "
            "en otro sitio");

  // y la pantalla sigue pintando algo
  avanza(200);
  galleryOpen = false;
  galleryDetail = 0;
}

TEST(combate_ui, al_caer_el_rival_no_queda_golpe_en_la_recamara) {
  arranca();
  // arranca() deja una partida NUEVA, o sea un huevo, y battleStart se
  // niega con un huevo. Hay que sacarlo y dejarlo en condiciones.
  while (pet.isEgg()) pet.eggTap();
  pet.sleeping = false;
  pet.energy = 100;
  pet.hpLeft = pet.maxHp();
  // con entrenador (el 0): es lo que hace que `over` se quede en false
  // al tumbar al primero, porque le queda banquillo
  battleStart(16, 10, -1, false, 0);
  CHECK_MSG(bt.open, "el combate no se abrio");

  // se simula el final del turno con el rival ya tumbado y un golpe
  // pendiente, que es el estado exacto en el que fallaba
  bt.b.side[BT_FOE].hp = 0;
  bt.secondPending = true;
  bt.phase = BP_MSG;
  bt.until = millis();
  avanza(40);

  CHECK_MSG(!bt.secondPending,
            "queda un golpe pendiente despues de tumbar al rival: "
            "se lo come el que entra de relevo");
  CHECK_MSG(bt.phase == BP_FAINT || bt.phase == BP_END,
            "tumbado el rival, la fase deberia ser el desmayo");
}

// El de arriba se para en el desmayo, y POR ESO daba verde mientras en la
// placa el relevo seguia comiendose un golpe: la segunda fuga estaba mas
// adelante, en btNextFoe(), que anunciaba "el entrenador saca al
// siguiente" con BP_SAY. BP_SAY no es una frase cualquiera: es "X uso Y!",
// y su paso del reloj llama a btResolve(). A los 1200 ms el ultimo golpe
// del jugador caia encima del recien entrado.
//
// Este test deja correr el reloj HASTA que el de relevo esta en el campo,
// que es lo que el otro no hacia.
// Cambiar de bicho en mitad del combate y volver a casa con el SUPLENTE
// pintado en la pantalla principal.
//
// `pmd` es UN solo sprite y el combate lo reutiliza: btSwitchTo carga ahi
// al que entra. Pero `monFor` — el cache que dice "lo que hay en pmd es el
// principal" — seguia intacto, porque el principal NO cambia al relevar (y
// no tiene que cambiar). Resultado: ensureMon() se creia el cache y no
// recargaba jamas.
//
// Se afirma sobre monFor y no sobre pixeles porque es el cache el que
// mentia: PmdMon no guarda de quien es el dibujo que lleva dentro.
TEST(combate_ui, tras_relevar_la_pantalla_de_casa_recarga_su_sprite) {
  arranca();
  while (pet.isEgg()) pet.eggTap();
  pet.sleeping = false;
  pet.energy = 100;
  pet.hpLeft = pet.maxHp();

  // Hueco limpio: los tests comparten un solo `pet` y los de antes dejan
  // el equipo lleno, asi que addToTeam fallaba y el test no probaba nada.
  for (uint8_t i = 0; i < TEAM_SLOTS; i++)
    if (i != pet.mainSlot) pet.team[i] = StoredMon{};

  // un suplente que NO sea la misma especie que el principal, o el test
  // no distinguiria un sprite del otro
  StoredMon sup{};
  sup.species = (pet.speciesId == 7) ? 25 : 7;
  sup.exp = Pet::expForLevel(10);
  sup.geneAtk = sup.geneDef = sup.geneSpe = 100;
  sup.moves[0] = MOVE_STRUGGLE;
  sup.ppLeft[0] = 5;
  sup.hpLeft = 50;
  CHECK_MSG(pet.addToTeam(sup), "no entro el suplente en el equipo");

  int16_t principal = pet.speciesId;
  battleStart(16, 10, -1, false, -1);
  CHECK_MSG(bt.open, "el combate no se abrio");

  uint8_t hueco = 0xFF;
  for (uint8_t i = 0; i < TEAM_SLOTS; i++)
    if (i != pet.mainSlot && pet.team[i].species == sup.species) hueco = i;
  CHECK_MSG(hueco != 0xFF, "no se encontro el hueco del suplente");

  // el relevo por la puerta de verdad: la lista de equipo del combate.
  // Las medidas son BPTY_X/BPTY_Y/BPTY_H de battle_ui.ino (66, 128, 44),
  // que son #define y no se ven desde aqui.
  bt.phase = BP_PARTY;
  CHECK_MSG(btPartyTap(96, 128 + hueco * 44 + 15), "la lista no cogio el toque");
  CHECK_MSG(bt.mySlot == hueco, "no llego a relevar");

  // Se cierra por donde se cierra de verdad (la fase final), pero llamando
  // a battleTick() A PELO y no por el bucle: ensureMon() corre en cada
  // vuelta y restaura monFor al instante, asi que pasando por loop() no
  // hay forma de ver el cache invalidado — el test daba verde con el fallo
  // puesto. Primero se mira el cache, y DESPUES se deja correr el bucle.
  bt.phase = BP_END;
  bt.until = millis();
  battleTick();
  CHECK_MSG(!bt.open, "el combate no se cerro");

  CHECK_MSG(pet.speciesId == principal,
            "relevar NO cambia de quien cuidas (lo dice pet.h)");
  CHECK_MSG(monFor != pet.speciesId,
            "el cache del sprite de casa sigue diciendo que dentro esta el "
            "principal, y lo que hay es el suplente");

  // y en las vueltas siguientes del bucle ensureMon() lo deja bien
  avanza(300);
  CHECK_MSG(monFor == pet.speciesId,
            "ensureMon() no restauro el sprite del principal");
}

// Contra un SALVAJE, caer uno no es perder el combate si te queda equipo.
// Pasaba lo contrario: los capturados se guardaban con hpLeft=0 ("sin
// estrenar"), btCanSend() lee ese 0 como desmayado, btReserveCount() daba
// 0 y playerReserves quedaba en false; knockOut() entonces ponia `over` y
// la pelea se acababa ahi, con fanfarria y todo, en vez de abrir la lista
// para sacar a otro.
TEST(combate_ui, un_suplente_que_megaevoluciona_vuelve_con_los_PS_que_le_caben) {
  // Siete de las 127 megas de la tabla SUBEN los PS base: el Anil mete
  // versiones Dynamax entre sus megas (Pikachu 35->55, Eevee 55->65).
  //
  // Al principal ya le recortaba Pet::afterBattle. Al suplente no: se
  // guardaba `m.hpLeft = c.hp` a pelo, asi que volvia del combate con
  // los PS de la mega medidos contra el maximo de su forma normal —— la
  // barra por encima del cien por cien hasta el siguiente centro.
  arranca();
  while (pet.isEgg()) pet.eggTap();
  pet.sleeping = false;
  pet.energy = 100;
  pet.hpLeft = pet.maxHp();
  for (uint8_t i = 0; i < TEAM_SLOTS; i++)
    if (i != pet.mainSlot) pet.team[i] = StoredMon{};
  // El techo de nivel, FIJADO. Los tests de UI comparten un solo `pet`:
  // con 0 insignias el techo es 11, el suplente de 30 pelearia a 11 y
  // a ese nivel la mega sube menos PS que el maximo de 30 —— no habria
  // nada que recortar y el test daba verde o rojo segun quien corrio
  // antes.
  pet.badges = GYM_BADGES;

  // Una mega que (a) tenga PIEDRA en la tabla de objetos y (b) SUBA los
  // PS base. Se busca, no se elige a dedo: la primera version ponia al
  // Pikachu, que es de las que NO tienen piedra (ver MEGAS_SIN_PIEDRA en
  // gen_items.py) y el test caia en la primera linea.
  uint8_t piedra = IT_NONE;
  int16_t quien = 0;
  for (uint16_t f = 0; f < MEGA_COUNT && !piedra; f++) {
    int16_t base = MEGA_TBL[f].base;
    if (base < 1 || base > DEX_COUNT) continue;
    if (MEGA_TBL[f].bHp <= DEX_TBL[base].bHp) continue;   // no sube los PS
    // Y que salga de la forma NORMAL: la del Pikachu sube los PS y tiene
    // piedra, pero sale de la forma 16, asi que un Pikachu de los de
    // siempre no puede usarla y canMega devolvia -1.
    if (MEGA_TBL[f].fromForm != 0 &&
        MEGA_TBL[f].fromForm != MEGA_ANY_FORM) continue;
    for (uint16_t i = 1; i < ITEM_COUNT; i++)
      if (ITEM_TBL[i].fx == IT_MEGA && ITEM_TBL[i].arg == MEGA_TBL[f].item) {
        piedra = (uint8_t)i;
        quien = base;
        break;
      }
  }
  CHECK_MSG(piedra != IT_NONE,
            "hace falta una mega con piedra que suba los PS: si no queda "
            "ninguna, este arreglo ya no tiene caso que probar");
  if (piedra == IT_NONE) return;

  StoredMon sup{};
  sup.species = quien;
  sup.exp = Pet::expForLevel(30);
  sup.geneAtk = sup.geneDef = sup.geneSpe = 100;
  sup.moves[0] = MOVE_STRUGGLE;
  sup.ppLeft[0] = 5;
  sup.item = piedra;
  uint16_t topeNormal = hpAtLevelForm(sup.species, Pet::levelOf(sup),
                                      sup.trHp, sup.form);
  sup.hpLeft = topeNormal;
  CHECK_MSG(pet.addToTeam(sup), "no entro el suplente en el equipo");

  battleStart(16, 30, -1, false, -1);
  CHECK_MSG(bt.open, "el combate no se abrio");
  uint8_t hueco = 0xFF;
  for (uint8_t i = 0; i < TEAM_SLOTS; i++)
    if (i != pet.mainSlot && pet.team[i].species == quien) hueco = i;
  CHECK_MSG(hueco != 0xFF, "no se encontro el hueco del suplente");

  bt.phase = BP_PARTY;
  CHECK_MSG(btPartyTap(96, 128 + hueco * 44 + 15), "la lista no cogio el toque");
  CHECK_MSG(bt.mySlot == hueco, "no llego a relevar");
  // Y pelea A SU nivel, que es la condicion en la que el fallo existe:
  // si el techo lo bajara, su maximo en pista seria menor que el
  // guardado y no habria desborde posible.
  CHECK_MSG(bt.b.side[BT_PLAYER].level == Pet::levelOf(pet.team[hueco]),
            "el techo le bajo el nivel: asi el test no prueba nada");

  // Megaevoluciona de verdad, por el mismo sitio que el boton.
  CHECK_MSG(bt.b.canMega(BT_PLAYER) >= 0, "con su piedra tiene que poder");
  CHECK_MSG(bt.b.doMega(BT_PLAYER), "la mega no salio");
  uint16_t topeMega = bt.b.side[BT_PLAYER].hpMax;
  CHECK_MSG(topeMega > topeNormal,
            "esta mega tiene que SUBIR los PS, o el test no prueba nada");
  bt.b.side[BT_PLAYER].hp = topeMega;      // acaba entero

  bt.phase = BP_END;
  bt.until = millis();
  battleTick();
  CHECK_MSG(!bt.open, "el combate no se cerro");

  // El maximo DE AHORA, con los mismos campos que mira btStoreActive:
  // durante el combate el suplente puede haber ganado EVs (los reparte
  // teamShareExp) y entonces su maximo ya no es el de antes de entrar.
  // Comparar contra el viejo era medir otra cosa.
  const StoredMon &vuelto = pet.team[hueco];
  uint16_t topeAhora = hpAtLevelForm(vuelto.species, Pet::levelOf(vuelto),
                                     vuelto.trHp, vuelto.form);
  CHECK_MSG(vuelto.hpLeft <= topeAhora,
            "el suplente volvio con " + std::to_string((int)vuelto.hpLeft) +
                " PS y su maximo es " + std::to_string((int)topeAhora));
  // Y que de verdad hubo algo que recortar: sin esto, un dia en que la
  // mega dejara de subir los PS el test seguiria verde sin probar nada.
  CHECK_MSG(topeMega > topeAhora,
            "la mega tiene que dejarlo por encima de su maximo: " +
                std::to_string((int)topeMega) + " vs " +
                std::to_string((int)topeAhora));
}

TEST(combate_ui, contra_un_salvaje_caer_uno_abre_la_lista_si_queda_equipo) {
  arranca();
  while (pet.isEgg()) pet.eggTap();
  pet.sleeping = false;
  pet.energy = 100;
  pet.hpLeft = pet.maxHp();
  for (uint8_t i = 0; i < TEAM_SLOTS; i++)
    if (i != pet.mainSlot) pet.team[i] = StoredMon{};
  // un suplente CAPTURADO, por la puerta de verdad
  CHECK_EQ((int)pet.catchMon(19, 10, false, 0), (int)CATCH_TEAM);

  battleStart(16, 10, -1, false, -1);   // salvaje, sin entrenador
  CHECK_MSG(bt.open, "el combate no se abrio");
  CHECK_MSG(bt.b.playerReserves,
            "el combate no ve el banquillo: el capturado no cuenta");

  // cae el mio
  bt.b.side[BT_PLAYER].hp = 0;
  bt.b.knockOut(BT_PLAYER);
  CHECK_MSG(!bt.b.over,
            "con equipo detras, caer uno no puede acabar el combate");

  bt.fainter = BT_PLAYER;
  bt.phase = BP_FAINT;
  bt.until = millis();
  avanza(2400);
  CHECK_MSG(bt.phase == BP_PARTY,
            "tendria que pedirte a quien sacas, y la fase es otra");
  CHECK_MSG(bt.mustSwitch, "el relevo tiene que ser obligatorio");
}

TEST(combate_ui, el_relevo_del_entrenador_no_entra_recibiendo_un_golpe) {
  arranca();
  while (pet.isEgg()) pet.eggTap();
  pet.sleeping = false;
  pet.energy = 100;
  pet.hpLeft = pet.maxHp();
  // El 1 ("GAMER") y no el 0: el 0 es el BUG CATCHER y saca UNO solo, asi
  // que con el no hay relevo que probar. La guarda de abajo esta para que
  // esto se vea si algun dia cambia la tabla, en vez de dar verde en vacio.
  battleStart(16, 10, -1, false, 1);
  CHECK_MSG(bt.open, "el combate no se abrio");
  CHECK_MSG(bt.foesLeft > 1,
            "este entrenador no tiene banquillo: sin relevo el test no "
            "prueba nada");

  // final de turno con el rival tumbado y un golpe todavia engatillado,
  // que es exactamente como se llega al relevo jugando
  bt.b.side[BT_FOE].hp = 0;
  bt.acting = BT_PLAYER;
  bt.pendingSlot = 0;
  bt.secondPending = false;
  bt.phase = BP_MSG;
  bt.extraStep = 9;
  bt.until = millis();

  // se avanza a pasos cortos vigilando las fases: que el golpe NO se tire
  // es mas fuerte que mirar solo los PS al final, porque un golpe puede
  // fallar y dejar el marcador limpio por casualidad.
  bool relevoDentro = false;
  bool pegoDespues = false;
  for (int i = 0; i < 250 && !pegoDespues; i++) {
    mockAdvanceMillis(16);
    loop();
    if (!relevoDentro && bt.b.side[BT_FOE].hp > 0) relevoDentro = true;
    if (relevoDentro && (bt.phase == BP_ATTACK || bt.phase == BP_HIT))
      pegoDespues = true;
  }

  CHECK_MSG(relevoDentro, "el entrenador no llego a sacar al siguiente");
  CHECK_MSG(!pegoDespues,
            "el que entra de relevo recibe un golpe sin que nadie lo pida");
  const Combatant &n = bt.b.side[BT_FOE];
  CHECK_MSG(n.hp == n.hpMax,
            "el de relevo entra tocado: " + std::to_string((int)n.hp) + "/" +
                std::to_string((int)n.hpMax) + " PS");
  CHECK_MSG(bt.phase == BP_CHOOSE || bt.phase == BP_MSG,
            "tras el relevo le toca elegir al jugador, y la fase es otra");
}

// ------------------------------------------------- la bolsa EN COMBATE
//
// Geometria de capture.ino, copiada como el resto de este fichero.
// Las medidas de la lista son BPTY_X/BPTY_Y/BPTY_H de battle_ui.ino
// (66, 128, 44), copiadas como el resto de la geometria de aqui.
#define BPTY_Y_T 128
#define BPTY_H_T 44
#define BPTY_X_T 66
#define BBAG_BTN_X_T 400
#define BBAG_BTN_Y_T 296
#define BBAG_Y_T 164

// Deja la bolsa de combate con UN solo objeto, para que caiga en la
// primera fila y el toque no dependa de lo que lleve encima el jugador.
static void soloEsteEnLaBolsa(uint8_t id) {
  for (uint16_t i = 1; i < ITEM_COUNT; i++)
    if (ITEM_TBL[i].pocket == BP_BATTLE) pet.bag[i] = 0;
  pet.bag[id] = 3;
}

// Busca un objeto por lo que HACE, no por su numero: la tabla sale del
// PBS del Anil y se le anaden filas.
static uint8_t objetoQueHace(uint8_t fx, uint8_t arg) {
  for (uint16_t i = 1; i < ITEM_COUNT; i++)
    if (ITEM_TBL[i].pocket == BP_BATTLE && ITEM_TBL[i].fx == fx &&
        ITEM_TBL[i].arg == arg)
      return (uint8_t)i;
  return 0;
}

static void enCombateConLaBolsa(uint8_t id) {
  if (pet.awaitingStarter()) pet.chooseStarter(1);
  while (pet.isEgg()) pet.eggTap();
  pet.sleeping = false;
  pet.energy = 100;
  pet.badges = GYM_BADGES;     // el techo de nivel, fijado: los tests
                               // de UI comparten un solo `pet`
  // Nivel 30 y no el de recien nacido: con 11 PS de maximo, una pocion
  // de 20 se corta contra el techo y no hay nada que medir.
  pet.exp = Pet::expForLevel(30);
  pet.hpLeft = pet.maxHp();
  for (uint8_t i = 0; i < TEAM_SLOTS; i++)
    if (i != pet.mainSlot) pet.team[i] = StoredMon{};
  soloEsteEnLaBolsa(id);
  battleStart(16, 10, -1, false, -1);
  // El combate NO abre eligiendo: entra por la presentacion. La bolsa
  // solo existe en BP_CHOOSE, asi que hay que llegar hasta ahi.
  for (int i = 0; i < 400 && bt.phase != BP_CHOOSE; i++) avanza(16);
}

TEST(combate_ui, una_pocion_cura_al_que_esta_EN_PISTA) {
  arranca();
  uint8_t poc = objetoQueHace(IT_POTION, 20);
  CHECK_MSG(poc != 0, "no hay Potion de 20 en la tabla");
  enCombateConLaBolsa(poc);
  CHECK_MSG(bt.open, "el combate no se abrio");
  CHECK_MSG(bt.phase == BP_CHOOSE, "no se llego a elegir: la bolsa no existe");

  Combatant &c = bt.b.side[BT_PLAYER];
  CHECK_MSG(c.hpMax > 60, "con tan poca vida una pocion de 20 no se mide");
  c.hp = (uint16_t)(c.hpMax / 2);
  const uint16_t antes = c.hp;
  // La PREMISA del fallo: durante el combate `pet.hpLeft` esta congelado
  // -- solo se escribe al salir de pista (btStoreActive). Mirando ahi,
  // una pocion con medio corazon veia a uno entero y decia que no.
  CHECK_MSG(pet.hpLeft > antes,
            "premisa: lo guardado tiene que ir por delante de la pista");

  onTap(BBAG_BTN_X_T, BBAG_BTN_Y_T);
  CHECK_MSG(bt.phase == BP_BAG, "el boton no abrio la bolsa");
  onTap(200, BBAG_Y_T + 17);
  // Y EL SEGUNDO TOQUE: desde que el Revivir existe de verdad, los
  // objetos que el PBS del Anil marca `BattleUse = OnPokemon` abren la
  // lista del equipo en vez de aplicarse solos. Aqui se apunta al que
  // esta EN PISTA, que es de quien habla este teste. No se afloja nada:
  // se anade el toque que el jugador tambien tiene que dar.
  CHECK_MSG(bt.phase == BP_PARTY, "la bolsa tenia que preguntar en quien");
  onTap(BPTY_X_T + 40, BPTY_Y_T + bt.mySlot * BPTY_H_T + 16);

  CHECK_MSG(c.hp == (uint16_t)(antes + 20),
            "la pocion no curo al de la pista: " + std::to_string((int)c.hp) +
                " cuando tenia " + std::to_string((int)antes));
  CHECK_MSG(pet.bag[poc] == 2, "no se gasto la pocion");
}

TEST(combate_ui, una_X_Attack_sube_DOS_escalones) {
  arranca();
  uint8_t xat = objetoQueHace(IT_XSTAT, SG_ATK);
  CHECK_MSG(xat != 0, "no hay X Attack en la tabla");
  enCombateConLaBolsa(xat);
  Combatant &c = bt.b.side[BT_PLAYER];
  c.stage[SG_ATK] = 0;

  onTap(BBAG_BTN_X_T, BBAG_BTN_Y_T);
  CHECK_MSG(bt.phase == BP_BAG, "el boton no abrio la bolsa");
  onTap(200, BBAG_Y_T + 17);

  // DOS y no uno: el Anil va por MECHANICS_GENERATION 9 y
  // X_STAT_ITEMS_RAISE_BY_TWO_STAGES es (>= 7).
  CHECK_MSG(c.stage[SG_ATK] == 2,
            "la X Attack dejo el ataque en " +
                std::to_string((int)c.stage[SG_ATK]) + " escalones");
  CHECK_MSG(pet.bag[xat] == 2, "no se gasto la X Attack");
}

TEST(combate_ui, una_Full_Heal_quita_el_estado_Y_la_confusion) {
  arranca();
  uint8_t fh = objetoQueHace(IT_CURE, 0);
  CHECK_MSG(fh != 0, "no hay Full Heal en la tabla");
  enCombateConLaBolsa(fh);
  Combatant &c = bt.b.side[BT_PLAYER];
  c.status = ST_POISON;
  c.statusTurns = 3;
  c.confuseTurns = 3;

  onTap(BBAG_BTN_X_T, BBAG_BTN_Y_T);
  CHECK_MSG(bt.phase == BP_BAG, "el boton no abrio la bolsa");
  onTap(200, BBAG_Y_T + 17);
  // Y EL SEGUNDO TOQUE: desde que el Revivir existe de verdad, los
  // objetos que el PBS del Anil marca `BattleUse = OnPokemon` abren la
  // lista del equipo en vez de aplicarse solos. Aqui se apunta al que
  // esta EN PISTA, que es de quien habla este teste. No se afloja nada:
  // se anade el toque que el jugador tambien tiene que dar.
  CHECK_MSG(bt.phase == BP_PARTY, "la bolsa tenia que preguntar en quien");
  onTap(BPTY_X_T + 40, BPTY_Y_T + bt.mySlot * BPTY_H_T + 16);

  CHECK_MSG(c.status == ST_NONE, "la Full Heal no quito el veneno");
  // Tambien la confusion: en el Anil su BattleUseOnPokemon llama a
  // pbCureStatus Y a pbCureConfusion (041_Items/004_Item_BattleEffects.rb).
  CHECK_MSG(c.confuseTurns == 0, "la Full Heal no quito la confusion");
}

TEST(combate_ui, con_un_suplente_fuera_la_pocion_no_toca_al_principal) {
  arranca();
  uint8_t poc = objetoQueHace(IT_POTION, 20);
  CHECK_MSG(poc != 0, "no hay Potion de 20 en la tabla");
  enCombateConLaBolsa(poc);
  // Un suplente por la puerta de verdad, y a pista.
  CHECK_EQ((int)pet.catchMon(19, 10, false, 0), (int)CATCH_TEAM);
  uint8_t hueco = 0xFF;
  for (uint8_t i = 0; i < TEAM_SLOTS; i++)
    if (i != pet.mainSlot && pet.team[i].species == 19) hueco = i;
  CHECK_MSG(hueco != 0xFF, "no se encontro el hueco del suplente");
  bt.phase = BP_PARTY;
  CHECK_MSG(btPartyTap(96, 128 + hueco * 44 + 15), "la lista no cogio el toque");
  CHECK_MSG(bt.mySlot == hueco, "no llego a relevar");
  bt.phase = BP_CHOOSE;

  // El PRINCIPAL, tocado y guardado. Es la trampa: la version vieja
  // curaba ESTO (Pet::useItem va sobre el principal) y despues copiaba
  // su vida encima del que estaba en pista.
  pet.hpLeft = 30;
  Combatant &c = bt.b.side[BT_PLAYER];
  // A UNO de vida y no a la mitad: el suplente de nivel 10 tiene 26 de
  // maximo, asi que a la mitad la pocion de 20 se corta contra el techo
  // y curar bien y curar de mas dan el mismo numero. La primera version
  // de este teste caia por eso, y el fallo era del teste.
  CHECK_MSG(c.hpMax > 21, "el suplente tiene que aguantar los 20 de la "
                          "pocion sin llegar al tope, o no se mide nada");
  c.hp = 1;
  const uint16_t antes = c.hp;
  // Y su vida no puede coincidir con la que tendria el principal ya
  // curado (30 + 20), que es lo que la version vieja le copiaba encima.
  CHECK_MSG((uint16_t)(antes + 20) != 50, "premisa: los dos numeros chocan");

  onTap(BBAG_BTN_X_T, BBAG_BTN_Y_T);
  CHECK_MSG(bt.phase == BP_BAG, "el boton no abrio la bolsa");
  onTap(200, BBAG_Y_T + 17);
  // Y EL SEGUNDO TOQUE: desde que el Revivir existe de verdad, los
  // objetos que el PBS del Anil marca `BattleUse = OnPokemon` abren la
  // lista del equipo en vez de aplicarse solos. Aqui se apunta al que
  // esta EN PISTA, que es de quien habla este teste. No se afloja nada:
  // se anade el toque que el jugador tambien tiene que dar.
  CHECK_MSG(bt.phase == BP_PARTY, "la bolsa tenia que preguntar en quien");
  onTap(BPTY_X_T + 40, BPTY_Y_T + bt.mySlot * BPTY_H_T + 16);

  CHECK_MSG(c.hp == (uint16_t)(antes + 20),
            "la pocion no curo al suplente, que es el que esta peleando: " +
                std::to_string((int)c.hp) + "/" + std::to_string((int)c.hpMax) +
                " (antes " + std::to_string((int)antes) + ")");
  CHECK_MSG(pet.hpLeft == 30,
            "la pocion curo al principal, que ni esta en pista: " +
                std::to_string((int)pet.hpLeft));
}

// ------------------------------------------------- la MEGA en la Pokedex
//
// Lo que expone el sketch: la ficha y las habilidades de la version que
// se mira. `galleryVers` codifica -1 normal, -2 shiny, >=0 fila de
// FORM_TBL y GVER_MEGA0 - fila para las megas (TamaPoke.ino).
void openDexEntry(int16_t dex);
uint8_t galleryAbils(int16_t dex, uint16_t *out);
extern int8_t galleryVer;
extern int16_t galleryVers[];
extern uint8_t galleryNVers;
extern bool galleryDirty;
#define GVER_MEGA0_T (-1000)

// La mega de `base` que esta en la lista de versiones, o -1.
static int8_t versionDeLaMega(int16_t fila) {
  for (uint8_t i = 0; i < galleryNVers; i++)
    if (galleryVers[i] == (int16_t)(GVER_MEGA0_T - fila)) return (int8_t)i;
  return -1;
}

// Los tests comparten UNA pantalla y este fichero tiene por regla que
// cada uno la deje como la encontro. Dejarse la ficha abierta hacia que
// todos los `avanza()` de los tests de despues pintaran la Pokedex con
// su sprite animado: la bateria entera se quedaba colgada, y a solas
// cada test pasaba. Por eso se cierra a mano.
static void cierraLaFicha() {
  galleryVer = 0;
  galleryDetail = 0;
  galleryOpen = false;
  galleryDirty = true;
}

TEST(pokedex, una_mega_vista_se_apunta_y_sale_como_version) {
  arranca();
  CHECK_MSG(MEGA_COUNT > 0, "no hay megas en la tabla");
  const int16_t fila = 0;
  const int16_t base = MEGA_TBL[fila].base;
  CHECK_MSG(base >= 1 && base <= DEX_COUNT, "la mega 0 no apunta a una especie");

  // Sin verla, la ficha NO la ofrece: es la mitad que hace falta para
  // que "aparece al megaevolucionar" signifique algo.
  pet.dexMegaReg[fila >> 3] &= (uint8_t) ~(1 << (fila & 7));
  CHECK_MSG(!pet.isMegaRegistered(fila), "premisa: tiene que empezar sin ver");
  openDexEntry(base);
  CHECK_MSG(versionDeLaMega(fila) < 0,
            "la mega sale en la ficha sin haberla visto nunca");

  pet.registerMega(fila);
  CHECK_MSG(pet.isMegaRegistered(fila), "verla no la apunto");
  // Y la BASE queda registrada: si no, la ficha seguiria diciendo "???"
  // con una version colgando de ella.
  CHECK_MSG(pet.isRegistered(base), "la mega no registro a su especie base");

  openDexEntry(base);
  CHECK_MSG(versionDeLaMega(fila) >= 0,
            "tras verla, la ficha sigue sin ofrecerla: hay " +
                std::to_string((int)galleryNVers) + " version(es)");
  cierraLaFicha();
}

// El boton de la mega, de capture.ino (BMEG_BTN_X = BPTY_BTN_X = 50).
#define BMEG_BTN_X_T 50
#define BMEG_BTN_Y_T 188

// El que de verdad contesta a lo que se pidio: MEGAEVOLUCIONAR EN
// COMBATE deja la mega en la Pokedex. Los otros dos tests llaman a
// registerMega a mano, asi que prueban la ficha pero NO el cable que va
// del combate a la ficha —— que es justo donde no habia nada.
TEST(pokedex, megaevolucionar_en_combate_deja_la_mega_en_la_ficha) {
  arranca();
  if (pet.awaitingStarter()) pet.chooseStarter(1);
  while (pet.isEgg()) pet.eggTap();
  pet.sleeping = false;
  pet.energy = 100;
  pet.badges = GYM_BADGES;
  pet.exp = Pet::expForLevel(30);
  pet.hpLeft = pet.maxHp();
  for (uint8_t i = 0; i < TEAM_SLOTS; i++)
    if (i != pet.mainSlot) pet.team[i] = StoredMon{};

  // Una mega con piedra en la tabla de objetos, buscada por lo que hace.
  uint8_t piedra = IT_NONE;
  int16_t quien = 0, fila = -1;
  for (uint16_t f = 0; f < MEGA_COUNT && piedra == IT_NONE; f++) {
    int16_t base = MEGA_TBL[f].base;
    if (base < 1 || base > DEX_COUNT) continue;
    if (MEGA_TBL[f].fromForm != 0) continue;     // tiene que salir de la normal
    for (uint16_t i = 1; i < ITEM_COUNT; i++)
      if (ITEM_TBL[i].fx == IT_MEGA && ITEM_TBL[i].arg == MEGA_TBL[f].item) {
        piedra = (uint8_t)i;
        quien = base;
        fila = (int16_t)f;
        break;
      }
  }
  CHECK_MSG(piedra != IT_NONE, "hace falta una mega con piedra que salga "
                               "de la forma normal");
  if (piedra == IT_NONE) return;

  // Sin verla: la Pokedex no la tiene.
  pet.dexMegaReg[fila >> 3] &= (uint8_t) ~(1 << (fila & 7));
  CHECK_MSG(!pet.isMegaRegistered(fila), "premisa: tiene que empezar sin ver");

  StoredMon m{};
  m.species = quien;
  m.exp = Pet::expForLevel(30);
  m.bond = 60;
  m.moves[0] = MOVE_STRUGGLE;
  m.ppLeft[0] = 5;
  m.item = piedra;
  m.hpLeft = hpAtLevelForm(m.species, Pet::levelOf(m), m.trHp, m.form);
  CHECK_MSG(pet.addToTeam(m), "no entro en el equipo");
  uint8_t hueco = 0xFF;
  for (uint8_t i = 0; i < TEAM_SLOTS; i++)
    if (i != pet.mainSlot && pet.team[i].species == quien) hueco = i;
  CHECK_MSG(hueco != 0xFF, "no se encontro el hueco");

  battleStart(16, 30, -1, false, -1);
  CHECK_MSG(bt.open, "el combate no se abrio");
  for (int i = 0; i < 400 && bt.phase != BP_CHOOSE; i++) avanza(16);
  bt.phase = BP_PARTY;
  CHECK_MSG(btPartyTap(96, 128 + hueco * 44 + 15), "la lista no cogio el toque");
  CHECK_MSG(bt.mySlot == hueco, "no llego a relevar");
  bt.phase = BP_CHOOSE;
  CHECK_MSG(bt.b.canMega(BT_PLAYER) >= 0, "con su piedra tiene que poder");

  // Por el BOTON, no llamando a doMega: lo que se esta probando es el
  // camino que recorre el jugador.
  onTap(BMEG_BTN_X_T, BMEG_BTN_Y_T);
  CHECK_MSG(bt.phase == BP_MEGA, "el boton no arranco la escena de la mega");
  // La escena dura BT_MEGA_MS (1400) y la transformacion pasa AL FINAL.
  for (int i = 0; i < 200 && bt.phase == BP_MEGA; i++) avanza(16);
  CHECK_MSG(bt.b.side[BT_PLAYER].isMega(), "no llego a megaevolucionar");

  CHECK_MSG(pet.isMegaRegistered(fila),
            "megaevoluciono y la Pokedex no se entero");
  openDexEntry(quien);
  CHECK_MSG(versionDeLaMega(fila) >= 0,
            "la mega no sale como version en la ficha despues del combate");
  cierraLaFicha();
}

// Barrido de LAS 127. El usuario dice que la del Arcanine sigue sin
// salir aunque el aparato la da por APUNTADA, asi que el fallo esta
// entre apuntarla y ensenarla —— y probar una sola no lo encuentra.
TEST(pokedex, TODAS_las_megas_vistas_salen_en_su_ficha) {
  arranca();
  while (pet.isEgg()) pet.eggTap();
  int fallan = 0;
  for (uint16_t i = 0; i < MEGA_COUNT; i++) {
    int16_t base = MEGA_TBL[i].base;
    if (base < 1 || base > DEX_COUNT) continue;
    pet.registerMega((int16_t)i);
    openDexEntry(base);
    if (versionDeLaMega((int16_t)i) < 0) {
      if (fallan < 8)
        printf("        NO SALE: %-14s (fila %u, base %d) con %u versiones\n",
               MEGA_TBL[i].name, (unsigned)i, (int)base,
               (unsigned)galleryNVers);
      fallan++;
    }
  }
  cierraLaFicha();
  CHECK_MSG(fallan == 0, std::to_string(fallan) +
                             " megas apuntadas no salen en su ficha");
}

TEST(pokedex, la_ficha_de_una_mega_ensena_SU_habilidad) {
  arranca();
  // Una mega cuya habilidad NO sea ninguna de las de su especie base: es
  // el unico caso en que el test distingue leer la tabla de la mega de
  // leer la de la especie —— a ojo las dos pintan un parrafo creible.
  int16_t fila = -1;
  for (uint16_t i = 0; i < MEGA_COUNT && fila < 0; i++) {
    int16_t b = MEGA_TBL[i].base;
    if (b < 1 || b > DEX_COUNT) continue;
    uint16_t a = MEGA_TBL[i].ability;
    if (!a || a >= ABIL_COUNT) continue;
    if (a != ABIL_OF[b][0] && a != ABIL_OF[b][1] && a != ABIL_OF[b][2])
      fila = (int16_t)i;
  }
  CHECK_MSG(fila >= 0,
            "hace falta una mega con habilidad propia: si no queda "
            "ninguna, este test no distingue nada");
  if (fila < 0) return;
  const int16_t base = MEGA_TBL[fila].base;

  pet.registerMega(fila);
  openDexEntry(base);
  int8_t v = versionDeLaMega(fila);
  CHECK_MSG(v >= 0, "la mega no esta entre las versiones");
  if (v < 0) return;

  galleryVer = v;
  uint16_t ab[4];
  uint8_t n = galleryAbils(base, ab);
  CHECK_MSG(n == 1, "una mega tiene UNA habilidad, y salieron " +
                        std::to_string((int)n));
  CHECK_MSG(n >= 1 && ab[0] == MEGA_TBL[fila].ability,
            "la ficha ensena la habilidad de la especie base y no la de "
            "la mega");
  // Y esa habilidad tiene texto que ensenar, que es lo que se pidio.
  CHECK_MSG(n >= 1 && ABIL_DESC[ab[0]] && *ABIL_DESC[ab[0]],
            "la habilidad de la mega no tiene descripcion que leer");

  // En la version NORMAL vuelven a salir las de la especie: el cambio de
  // version tiene que cambiar tambien lo que se lee.
  galleryVer = 0;
  n = galleryAbils(base, ab);
  CHECK_MSG(n >= 1 && ab[0] != MEGA_TBL[fila].ability,
            "en la version normal se quedo la habilidad de la mega");
  cierraLaFicha();
}

// ------------------------------------------------- los TRES de la casa

void btNextFoe();   // battle_ui.ino, no esta en el .h

static uint16_t bstDe(int16_t d) {
  const DexEntry &e = DEX_TBL[d];
  return (uint16_t)e.bHp + e.bAtk + e.bDef + e.bSpA + e.bSpD + e.bSpe;
}

TEST(gimnasio, los_de_la_casa_no_sacan_legendarios_NUNCA) {
  arranca();
  // De donde venia el ARTICUNO: el reparto de la hierba suelta a los
  // legendarios en cuanto tienes las ocho insignias. El picker de la
  // casa ya no mira las insignias para nada —— se ponen igual para que,
  // si alguien lo devuelve a `wildWeight`, este test caiga.
  pet.badges = GYM_BADGES;
  int salieron = 0, leyendas = 0;
  // Todos los tipos y TODA la escala de niveles, de 2 (el primero de
  // Brock) a 80 (los del campeon, cuyo as es de 82). Se empieza en el
  // tipo 1: T_NONE es el segundo tipo de los puros, y pedirlo casaria
  // con medio catalogo.
  //
  // La escala entera importa: el techo de fuerza NO basta para echar a
  // los legendarios. COSMOG tiene 200 de BST y entra ya en el techo de
  // un entrenador de nivel 2; a nivel 80 el techo es 700 y pasan 101 de
  // los 103 legendarios de la tabla. Probando solo niveles bajos, este
  // test daria verde con el filtro quitado.
  for (uint8_t t = 1; t < T_COUNT; t++) {
    for (uint8_t nv = 2; nv <= 80; nv += 13) {
      for (int tirada = 0; tirada < 25; tirada++) {
        int16_t d = gymTrainerSpecies(t, nv);
        if (d < 1) continue;
        salieron++;
        if (DEX_TBL[d].rarity == R_LEGENDARIO) {
          leyendas++;
          if (leyendas == 1)
            printf("        primer legendario: %s (tipo %u, nivel %u)\n",
                   DEX_TBL[d].name, (unsigned)t, (unsigned)nv);
        }
      }
    }
  }
  CHECK_MSG(salieron > 500, "apenas salio nada: el test no mira casi nada (" +
                                std::to_string(salieron) + " tiradas)");
  CHECK_MSG(leyendas == 0,
            std::to_string(leyendas) + " de " + std::to_string(salieron) +
                " bichos de la casa eran LEGENDARIOS");
}

TEST(gimnasio, los_de_la_casa_pegan_segun_SU_nivel) {
  arranca();
  pet.badges = GYM_BADGES;
  // El tipo normal, que es el que mas candidatos tiene de arriba abajo.
  uint32_t flojos = 0, duros = 0;
  int nf = 0, nd = 0;
  for (int i = 0; i < 120; i++) {
    int16_t a = gymTrainerSpecies(T_NORMAL, 2);
    int16_t b = gymTrainerSpecies(T_NORMAL, 42);
    if (a >= 1) { flojos += bstDe(a); nf++; }
    if (b >= 1) { duros  += bstDe(b); nd++; }
  }
  CHECK_MSG(nf > 50 && nd > 50, "no salieron bastantes para comparar");
  if (!nf || !nd) return;
  uint32_t ma = flojos / (uint32_t)nf, mb = duros / (uint32_t)nd;
  printf("        normal: nivel 2 -> BST medio %u, nivel 42 -> %u\n",
         (unsigned)ma, (unsigned)mb);
  // El de nivel 42 tiene que sacar bichos mas fuertes que el de 2: es lo
  // que separa al primero de Brock del tercero de Giovanni.
  CHECK_MSG(mb > ma + 30,
            "el nivel no cambia lo que sacan: BST medio " +
                std::to_string((int)ma) + " a nivel 2 y " +
                std::to_string((int)mb) + " a nivel 42");
}

TEST(gimnasio, el_lider_saca_su_piedra_mega_con_el_que_la_lleva) {
  arranca();
  // Un desafio DE LA ELITE 4 O EL CAMPEON (de MEGA_DESDE_GIMNASIO en
  // adelante) cuyo equipo tenga a alguien con mega: si no, no hay piedra
  // que repartir.
  //
  // El tope es `pet.gymCount()` y NO GYM_COUNT: ese vale 8, que son solo
  // los lideres, y la Elite 4 son los indices 8..11 con el campeon en el
  // 12. Con GYM_COUNT el bucle no daba ni una vuelta.
  int8_t cual = -1, hueco = -1;
  for (uint8_t gi = MEGA_DESDE_GIMNASIO; gi < pet.gymCount() && cual < 0; gi++) {
    const GymEntry &g = gymAt(pet.region, gi);
    for (int8_t s = (int8_t)gymTeamSize(g) - 1; s >= 0 && cual < 0; s--) {
      int16_t d = gymTeamAt(g, (uint8_t)s);
      if (d < 1) continue;
      for (uint16_t k = 1; k < ITEM_COUNT; k++)
        if (ITEM_TBL[k].fx == IT_MEGA && megaFormOf(d, ITEM_TBL[k].arg) >= 0) {
          cual = (int8_t)gi;
          hueco = s;
          break;
        }
    }
  }
  CHECK_MSG(cual >= 0, "ningun lider de esta region tiene equipo con mega: "
                       "el test no prueba nada");
  if (cual < 0) return;

  const GymEntry &g = gymAt(pet.region, (uint8_t)cual);
  const uint8_t n = gymTeamSize(g);
  printf("        %s: la piedra le toca al hueco %d de %u\n", g.leader,
         (int)hueco, (unsigned)n);

  while (pet.isEgg()) pet.eggTap();
  pet.sleeping = false;
  pet.energy = 100;
  pet.hpLeft = pet.maxHp();
  battleStart(gymTeamAt(g, 0), gymLevelAt(g, 0), cual);
  CHECK_MSG(bt.open, "el combate no se abrio");

  // Se recorre el equipo entero y se mira QUIEN sale con la piedra.
  bool laLlevo = false;
  for (uint8_t i = 0; i < n; i++) {
    if (i > 0) {
      bt.foeIdx = (uint8_t)(i - 1);      // btNextFoe lo incrementa
      btNextFoe();
    }
    const Combatant &r = bt.b.side[BT_FOE];
    bool esperada = ((int8_t)i == hueco);
    bool tiene = (r.item != IT_NONE && itemAt(r.item).fx == IT_MEGA);
    if (tiene) laLlevo = true;
    CHECK_MSG(tiene == esperada,
              "el hueco " + std::to_string((int)i) +
                  (esperada ? " tendria que llevar la piedra y no la lleva"
                            : " lleva una piedra que no le toca"));
  }
  CHECK_MSG(laLlevo, "nadie del equipo llego a salir con la piedra");
  bt.open = false;
}

TEST(gimnasio, un_lider_normal_NO_megaevoluciona) {
  arranca();
  while (pet.isEgg()) pet.eggTap();
  pet.sleeping = false;
  pet.energy = 100;
  pet.hpLeft = pet.maxHp();
  // El otro lado de la regla que pidio el usuario: la mega es cosa de la
  // Elite 4 y el campeon. Un lider de los ocho no saca piedra ni aunque
  // su equipo pueda usarla.
  //
  // Se recorren LOS OCHO y no el primero que pueda. La primera version
  // paraba en el primero —— la Misty, que es el desafio 1 ——, y el
  // sabotaje la dejo VERDE: con el techo devuelto a 4 la Misty sigue sin
  // piedra, asi que ese test no probaba el techo, probaba a la Misty.
  // Los que distinguen son los del 4 al 7.
  int mirados = 0;
  for (uint8_t gi = 0; gi < GYM_BADGES; gi++) {
    const GymEntry &g = gymAt(pet.region, gi);
    bool puede = false;
    for (uint8_t s = 0; s < gymTeamSize(g) && !puede; s++) {
      int16_t d = gymTeamAt(g, s);
      if (d < 1) continue;
      for (uint16_t k = 1; k < ITEM_COUNT && !puede; k++)
        if (ITEM_TBL[k].fx == IT_MEGA && megaFormOf(d, ITEM_TBL[k].arg) >= 0)
          puede = true;
    }
    if (!puede) continue;        // sin mega que dar no distingue nada
    mirados++;
    battleStart(gymTeamAt(g, 0), gymLevelAt(g, 0), (int8_t)gi);
    CHECK_MSG(bt.open, "el combate no se abrio");
    for (uint8_t i = 0; i < gymTeamSize(g); i++) {
      if (i > 0) { bt.foeIdx = (uint8_t)(i - 1); btNextFoe(); }
      const Combatant &r = bt.b.side[BT_FOE];
      CHECK_MSG(r.item == IT_NONE || itemAt(r.item).fx != IT_MEGA,
                std::string(g.leader) + " (desafio " + std::to_string((int)gi) +
                    ") salio con piedra mega en el hueco " +
                    std::to_string((int)i));
    }
    bt.open = false;
  }
  printf("        %d lideres de los ocho podrian megaevolucionar y ninguno lo hace\n",
         mirados);
  CHECK_MSG(mirados > 0, "ningun lider de los ocho tiene equipo con mega: "
                         "asi este test no distingue nada");
}

// ------------------------------------------------- la REVANCHA

void rematchStart(uint8_t gym);

TEST(revancha, todos_pelean_al_nivel_del_campeon_y_con_mega) {
  arranca();
  while (pet.isEgg()) pet.eggTap();
  pet.sleeping = false;
  pet.energy = 100;
  pet.hpLeft = pet.maxHp();
  // CORONADO: es la condicion de la revancha.
  pet.badges = pet.gymCount();
  CHECK_MSG(pet.allGymsDone(), "premisa: hace falta estar coronado");

  const uint8_t champ = gymChampLevel(pet.region);
  // El PRIMER desafio de la region, que es el que mas lejos esta del
  // campeon: si la revancha lo sube hasta ahi, lo sube todo.
  const GymEntry &g = gymAt(pet.region, 0);
  CHECK_MSG(gymLevelAt(g, 0) + 20 < champ,
            "el primer desafio ya pelea cerca del campeon: asi no se nota "
            "si la revancha sube el nivel o no");

  rematchStart(0);
  CHECK_MSG(bt.open, "la revancha no se abrio");
  CHECK_MSG(bt.rematch, "el combate no se marco como revancha");
  CHECK_MSG(bt.b.side[BT_FOE].level == champ,
            "el primero salio a nivel " +
                std::to_string((int)bt.b.side[BT_FOE].level) +
                " y el campeon es de " + std::to_string((int)champ));

  // Y TODO el equipo, no solo el primero. De paso se mira la piedra: en
  // revancha entra aunque sea un lider del principio.
  bool alguienConPiedra = false;
  for (uint8_t i = 1; i < gymTeamSize(g); i++) {
    bt.foeIdx = (uint8_t)(i - 1);
    btNextFoe();
    const Combatant &r = bt.b.side[BT_FOE];
    CHECK_MSG(r.level == champ,
              "el hueco " + std::to_string((int)i) + " salio a nivel " +
                  std::to_string((int)r.level));
    if (r.item != IT_NONE && itemAt(r.item).fx == IT_MEGA)
      alguienConPiedra = true;
  }
  // Solo se exige piedra si ESE equipo puede usarla.
  bool puede = false;
  for (uint8_t s = 0; s < gymTeamSize(g); s++) {
    int16_t d = gymTeamAt(g, s);
    if (d < 1) continue;
    for (uint16_t k = 1; k < ITEM_COUNT; k++)
      if (ITEM_TBL[k].fx == IT_MEGA && megaFormOf(d, ITEM_TBL[k].arg) >= 0)
        puede = true;
  }
  if (puede)
    CHECK_MSG(alguienConPiedra,
              "en revancha el equipo puede megaevolucionar y nadie salio "
              "con la piedra");
  bt.open = false;
  bt.rematch = false;
}

TEST(revancha, ganarla_no_da_otra_insignia) {
  arranca();
  while (pet.isEgg()) pet.eggTap();
  pet.sleeping = false;
  pet.energy = 100;
  pet.hpLeft = pet.maxHp();
  pet.badges = pet.gymCount();
  const uint8_t antes = pet.badges;
  // Pet::winGym() se va de vacio con la region ya hecha. Esto lo FIJA:
  // si alguien le quita esa guarda, la revancha empezaria a repartir
  // insignias de las que ya tienes.
  pet.winGym();
  CHECK_MSG(pet.badges == antes,
            "ganar con la region ya hecha dio otra insignia: " +
                std::to_string((int)antes) + " -> " +
                std::to_string((int)pet.badges));
}

// ------------------------------------------------- viajar NO cuesta nada

TEST(regiones, viajar_no_quita_las_insignias) {
  arranca();
  while (pet.isEgg()) pet.eggTap();
  pet.sleeping = false;
  pet.energy = 100;
  CHECK_MSG(REGION_COUNT >= 2, "con una sola region no hay viaje que probar");
  if (REGION_COUNT < 2) return;

  // Coronado en la primera y con la segunda abierta. Se limpian TODAS
  // las regiones: los tests comparten un solo `pet`, y uno de antes
  // podia dejar insignias en la segunda —— entonces llegar alli no la
  // encontraba a cero y este test fallaba solo en la bateria entera.
  pet.region = 0;
  for (uint8_t r = 0; r < REGION_COUNT; r++) pet.badgesIn[r] = 0;
  pet.badges = pet.gymCount();
  pet.badgesIn[0] = pet.badges;
  pet.maxRegion = 1;
  const uint8_t deKanto = pet.badges;
  CHECK_MSG(pet.allGymsDone(), "premisa: hace falta estar coronado");

  CHECK_MSG(pet.setZone(1), "no se pudo viajar a la segunda region");
  CHECK_MSG(pet.region == 1, "no llego");
  // En la region nueva se empieza de cero EN ESA region...
  CHECK_MSG(pet.badges == 0,
            "la region nueva tendria que empezar sin insignias suyas");
  // ...pero las de la primera NO se han perdido.
  CHECK_MSG(pet.badgesOf(0) == deKanto,
            "viajar borro las insignias de la primera region: quedan " +
                std::to_string((int)pet.badgesOf(0)) + " de " +
                std::to_string((int)deKanto));

  // Y volver las devuelve.
  CHECK_MSG(pet.setZone(0), "no se pudo volver");
  CHECK_MSG(pet.badges == deKanto,
            "volver no devolvio las insignias: " +
                std::to_string((int)pet.badges));
  CHECK_MSG(pet.allGymsDone(), "al volver sigue coronado");
}

TEST(regiones, el_techo_de_nivel_no_baja_al_viajar) {
  arranca();
  while (pet.isEgg()) pet.eggTap();
  CHECK_MSG(REGION_COUNT >= 2, "hace falta otra region");
  if (REGION_COUNT < 2) return;
  pet.region = 0;
  for (uint8_t r = 0; r < REGION_COUNT; r++) pet.badgesIn[r] = 0;
  pet.badges = pet.gymCount();
  pet.badgesIn[0] = pet.badges;
  pet.maxRegion = 1;
  const uint16_t antes = pet.levelCap();

  CHECK_MSG(pet.setZone(1), "no se pudo viajar");
  const uint16_t ahora = pet.levelCap();
  // Si el techo cayera, el equipo hecho pelearia como novato en la
  // region nueva: es la misma pena que quitar las insignias, por la
  // puerta de atras.
  CHECK_MSG(ahora >= antes,
            "el techo de nivel bajo al viajar: " + std::to_string((int)antes) +
                " -> " + std::to_string((int)ahora));
  pet.setZone(0);
}

// ------------------------------------------------- los LOGROS

TEST(logros, hay_uno_por_region_y_se_gana_al_terminarla) {
  arranca();
  while (pet.isEgg()) pet.eggTap();
  pet.sleeping = false;
  pet.energy = 100;
  // Sin terminar nada: el logro de la region 0 no esta.
  pet.region = 0;
  pet.badges = 0;
  for (uint8_t r = 0; r < REGION_COUNT; r++) pet.badgesIn[r] = 0;
  pet.medals &= ~MED_REGION(0);
  pet.jugadorMedals();
  CHECK_MSG(!pet.hasMedal(MED_REGION(0)),
            "el logro de la region sale sin haberla terminado");

  pet.badges = pet.gymCount();
  pet.badgesIn[0] = pet.badges;
  pet.jugadorMedals();
  CHECK_MSG(pet.hasMedal(MED_REGION(0)),
            "terminar la region no dio su logro");
  // Y hay uno por CADA region, con premio asignado.
  for (uint8_t r = 0; r < REGION_COUNT; r++) {
    int bit = MED_REGION0 + r;
    CHECK_MSG(bit < MED_COUNT, "la region " + std::to_string((int)r) +
                                   " no tiene bit de logro");
    CHECK_MSG(MEDAL_PRIZE[bit] != 0,
              "el logro de la region " + std::to_string((int)r) +
                  " no paga nada");
  }
}

TEST(logros, todos_tienen_nombre_y_premio) {
  arranca();
  // El fallo que esto caza: las tablas por idioma tenian OCHO entradas y
  // MED_COUNT paso a 30. Dimensionadas con MED_COUNT habrian quedado con
  // 22 punteros nulos por idioma, y el primer logro nuevo reventaria
  // dentro de strlen —— que es como ya reventó una vez la fanfarria
  // numero ocho del simulador.
  Lang antes = gLang;
  for (int li = 0; li < LANG_COUNT; li++) {
    setLang((Lang)li);
    for (int i = 0; i < MED_COUNT; i++) {
      const char *n = medalName(i), *l = medalLabel(i), *d = medalDesc(i);
      CHECK_MSG(n && *n, "logro " + std::to_string(i) + " sin nombre (idioma " +
                             std::to_string(li) + ")");
      CHECK_MSG(l && *l, "logro " + std::to_string(i) + " sin etiqueta");
      CHECK_MSG(d && *d, "logro " + std::to_string(i) + " sin descripcion");
      CHECK_MSG(MEDAL_PRIZE[i] != 0 && MEDAL_PRIZE[i] < ITEM_COUNT,
                "logro " + std::to_string(i) + " con premio invalido: " +
                    std::to_string((int)MEDAL_PRIZE[i]));
    }
  }
  setLang(antes);
}

TEST(logros, los_del_jugador_cuentan_lo_que_llevas_hecho) {
  arranca();
  while (pet.isEgg()) pet.eggTap();
  // 100 combates ganados.
  pet.medals &= ~(MED_WINS100 | MED_WINS500);
  pet.wins = 99;
  pet.jugadorMedals();
  CHECK_MSG(!pet.hasMedal(MED_WINS100), "99 victorias no son 100");
  pet.wins = 100;
  pet.jugadorMedals();
  CHECK_MSG(pet.hasMedal(MED_WINS100), "100 victorias no dieron su logro");
  CHECK_MSG(!pet.hasMedal(MED_WINS500), "100 no pueden dar el de 500");

  // Y la primera mega, que viene de la Pokedex.
  pet.medals &= ~MED_MEGA1;
  for (uint16_t i = 0; i < sizeof(pet.dexMegaReg); i++) pet.dexMegaReg[i] = 0;
  pet.jugadorMedals();
  CHECK_MSG(!pet.hasMedal(MED_MEGA1), "sin megas vistas no hay logro de mega");
  pet.registerMega(0);
  pet.jugadorMedals();
  CHECK_MSG(pet.hasMedal(MED_MEGA1), "ver una mega no dio su logro");
}

// ====================================================================
// LO QUE SE VE EN EL COMBATE. Tres cosas que el dueno vio jugando y que
// ningun test miraba, porque todos los de combate afirmaban sobre PS y
// sobre fases y ninguno sobre DONDE y SI se pinto cada cosa.
// ====================================================================

// 1) EXCAVAR / VUELO / BUCEO. El motor saca al bicho del campo durante
//    un turno (Combatant::hiding) y el dibujante no lo miraba: el sprite
//    se quedaba plantado mientras los golpes le pasaban por encima.
//
//    Se mide sobre btAltoDibujado[], que es el alto de lo que btDrawSide
//    acaba de pintar de ese lado —— no una bandera nueva: el mismo
//    numero que ya usa la bola de captura para colocarse.
TEST(combate_ui, el_que_vuela_DESAPARECE_de_la_pantalla) {
  arranca();
  // Sin esto render() se queda en la pantalla de elegir inicial: su
  // ramo va DELANTE del combate en render() y acaba en return, asi que
  // renderBattle() no llega a correr y no se pinta ninguna bola.
  if (pet.awaitingStarter()) pet.chooseStarter(1);
  while (pet.isEgg()) pet.eggTap();
  pet.sleeping = false;
  pet.energy = 100;

  battleStart(16, 10);
  CHECK_MSG(bt.open, "el combate no se abrio");
  for (int i = 0; i < 400 && bt.phase != BP_CHOOSE; i++) avanza(16);
  CHECK_MSG(bt.phase == BP_CHOOSE, "no se llego a elegir golpe");

  avanza(64);
  CHECK_MSG(btAltoDibujado[BT_FOE] > 0,
            "premisa: a la vista, el rival se pinta");
  int16_t aLaVista = btAltoDibujado[BT_FOE];

  // En mitad de la subida ya tiene que estar encogiendose: si solo
  // desapareciera de golpe seria un corte, no una transicion.
  bt.b.side[BT_FOE].hiding = HID_SKY;
  avanza(16);                       // un fotograma: arranca el reloj
  avanza(BT_HIDE_MS / 2);
  int16_t aMedias = btAltoDibujado[BT_FOE];
  CHECK_MSG(aMedias > 0 && aMedias < aLaVista,
            "a mitad de camino tiene que verse MAS PEQUENO: " +
            std::to_string(aMedias) + " contra " + std::to_string(aLaVista));

  // El combate NO repinta en cada vuelta de loop() (va a unos 11 fps),
  // asi que el reloj del escondite arranca en el primer repintado que
  // caiga DESPUES del cambio —— hasta 90 ms mas tarde. Esperar
  // BT_HIDE_MS justos se queda corto por ese margen, y asi es como la
  // primera version de este test se puso roja midiendo u=0,9.
  avanza(BT_HIDE_MS * 3);
  CHECK_MSG(btAltoDibujado[BT_FOE] == 0,
            "volando NO puede seguir dibujado: se pintaron " +
            std::to_string(btAltoDibujado[BT_FOE]) + " px de alto");

  // Y al bajar vuelve, que es la otra mitad de la transicion.
  bt.b.side[BT_FOE].hiding = HID_NONE;
  avanza(BT_HIDE_MS * 3);
  CHECK_MSG(btAltoDibujado[BT_FOE] > 0, "al volver tiene que verse otra vez");

  // Bajo tierra, lo mismo por el otro lado.
  bt.b.side[BT_FOE].hiding = HID_UNDER;
  avanza(BT_HIDE_MS * 3);
  CHECK_MSG(btAltoDibujado[BT_FOE] == 0,
            "y cavando tampoco se le ve");
  bt.b.side[BT_FOE].hiding = HID_NONE;
  avanza(BT_HIDE_MS * 3);
  while (bt.open) { bt.phase = BP_END; bt.until = millis(); avanza(32); }
}

// 2) DE QUE LADO VIENE CADA BOLA. El jugador esta fuera de pantalla por
//    la IZQUIERDA y el rival por la DERECHA, asi que cada bola entra
//    desde su entrenador. Estaban cambiadas: las dos salian del centro
//    hacia fuera, o sea cada una desde el lado del otro.
TEST(combate_ui, cada_bola_entra_desde_el_lado_de_su_entrenador) {
  arranca();
  // Sin esto render() se queda en la pantalla de elegir inicial: su
  // ramo va DELANTE del combate en render() y acaba en return, asi que
  // renderBattle() no llega a correr y no se pinta ninguna bola.
  if (pet.awaitingStarter()) pet.chooseStarter(1);
  while (pet.isEgg()) pet.eggTap();
  pet.sleeping = false;
  pet.energy = 100;

  battleStart(16, 10);
  CHECK_MSG(bt.open, "el combate no se abrio");
  // Hasta BP_CHOOSE: durante la entrada el combate todavia esta
  // encadenando sus propias bolas y sus propias frases, y la primera
  // version de este test ponia bt.sendAt encima de una de ellas.
  for (int i = 0; i < 400 && bt.phase != BP_CHOOSE; i++) avanza(16);
  CHECK_MSG(bt.phase == BP_CHOOSE, "no se llego a elegir golpe");

  // La X de mi plataforma y la del rival, tal como las pinta renderBattle.
  const int MI_CX = 145, SU_CX = BT_FOE_CX;

  // Al principio del arco es cuando la bola esta mas lejos de su sitio;
  // al final las dos caen en el centro de la plataforma, asi que mirar
  // ahi no distinguiria nada.
  // Hasta que SE PINTE: el combate va a unos 11 fps, asi que una sola
  // vuelta de loop() puede no repintar nada. Se para en cuanto hay bola,
  // que es lo antes posible dentro del arco.
  // Se REARMA en cada vuelta hasta que haya repintado: el combate no
  // pinta en cada loop(), y asi el fotograma que salga cae siempre al
  // principio del arco —— que es donde la bola esta mas lejos de su
  // plataforma y donde la diferencia entre un lado y otro se ve.
  bt.sendSide = BT_PLAYER;
  for (int i = 0; i < 40 && btBolaX[BT_PLAYER] < 0; i++) {
    bt.sendAt = millis();
    avanza(16);
  }
  CHECK_MSG(btBolaX[BT_PLAYER] >= 0, "no se llego a pintar mi bola");
  CHECK_MSG(btBolaX[BT_PLAYER] < MI_CX,
            "la MIA la tiro el jugador, que esta a la IZQUIERDA: tiene "
            "que entrar por x<" + std::to_string(MI_CX) + " y entro en " +
            std::to_string(btBolaX[BT_PLAYER]));

  bt.sendSide = BT_FOE;
  for (int i = 0; i < 40 && btBolaX[BT_FOE] < 0; i++) {
    bt.sendAt = millis();
    avanza(16);
  }
  CHECK_MSG(btBolaX[BT_FOE] >= 0, "no se llego a pintar la suya");
  CHECK_MSG(btBolaX[BT_FOE] > SU_CX,
            "la SUYA la tira el rival, que esta a la DERECHA: tiene que "
            "entrar por x>" + std::to_string(SU_CX) + " y entro en " +
            std::to_string(btBolaX[BT_FOE]));

  bt.sendAt = 0;
  while (bt.open) { bt.phase = BP_END; bt.until = millis(); avanza(32); }
}

// 3) LA MEGA DEL RIVAL TIENE ESCENA. Se hacia con un doMega() en la
//    misma linea del toque: el bicho cambiaba de sprite entre dos
//    fotogramas, sin anillos, sin sello y sin sonido.
TEST(combate_ui, la_mega_del_rival_no_es_instantanea) {
  arranca();
  // Sin esto render() se queda en la pantalla de elegir inicial: su
  // ramo va DELANTE del combate en render() y acaba en return, asi que
  // renderBattle() no llega a correr y no se pinta ninguna bola.
  if (pet.awaitingStarter()) pet.chooseStarter(1);
  while (pet.isEgg()) pet.eggTap();
  pet.sleeping = false;
  pet.energy = 100;
  pet.exp = Pet::expForLevel(40);
  pet.hpLeft = pet.maxHp();

  // Un rival que PUEDA megaevolucionar: una especie con mega y su piedra.
  int16_t fila = -1;
  for (uint16_t i = 0; i < MEGA_COUNT && fila < 0; i++)
    if (MEGA_TBL[i].base >= 1 && MEGA_TBL[i].base <= DEX_COUNT) fila = (int16_t)i;
  CHECK_MSG(fila >= 0, "no hay ninguna mega en la tabla");
  int16_t suDex = MEGA_TBL[fila].base;

  battleStart(suDex, 40);
  CHECK_MSG(bt.open, "el combate no se abrio");
  for (int i = 0; i < 400 && bt.phase != BP_CHOOSE; i++) avanza(16);
  CHECK_MSG(bt.phase == BP_CHOOSE, "no se llego a elegir golpe");

  // La piedra, puesta a mano: en la hierba no la lleva nadie.
  uint8_t piedra = IT_NONE;
  for (uint8_t i = 1; i < ITEM_COUNT; i++)
    if (ITEM_TBL[i].fx == IT_MEGA &&
        megaFormOf(suDex, ITEM_TBL[i].arg, bt.b.side[BT_FOE].form) == fila) {
      piedra = i;
      break;
    }
  CHECK_MSG(piedra != IT_NONE, "no se encontro la piedra de esa mega");
  bt.b.side[BT_FOE].item = piedra;
  // Que el turno no se acabe antes de verse nada.
  bt.b.side[BT_FOE].hpMax = 9999; bt.b.side[BT_FOE].hp = 9999;
  bt.b.side[BT_PLAYER].hpMax = 9999; bt.b.side[BT_PLAYER].hp = 9999;
  CHECK_MSG(bt.b.canMega(BT_FOE) >= 0, "premisa: con su piedra tiene que poder");

  toca(233, (BT_CARD_Y0_T + BT_CARD_Y1_T) / 2);
  CHECK_MSG(bt.phase == BP_MEGA,
            "al elegir golpe, la mega del rival tiene que ABRIR ESCENA");
  CHECK_MSG(bt.megaSide == BT_FOE, "y la escena es la de SU lado");
  CHECK_MSG(!bt.b.side[BT_FOE].isMega(),
            "y no puede haberse transformado todavia: la escena es "
            "justo lo que falta por ver");

  // El sello y los anillos viven mientras dure la fase; la
  // transformacion cae AL FINAL, con el ultimo anillo.
  int fotogramas = 0;
  for (int i = 0; i < 300 && bt.phase == BP_MEGA; i++) { avanza(16); fotogramas++; }
  CHECK_MSG(fotogramas > 10,
            "la escena duro " + std::to_string(fotogramas) +
            " fotogramas: eso no se ve");
  CHECK_MSG(bt.b.side[BT_FOE].isMega(), "al acabar la escena, megaevoluciono");
  CHECK_MSG(strstr(bt.msg, "MEGA EVOLVED") != nullptr,
            "y se dice: la caja decia '" + std::string(bt.msg) + "'");

  // Y el turno SIGUE: la mega del rival cae en mitad del turno, asi que
  // de su escena no se vuelve al menu —— se pega.
  bool siguio = false;
  for (int i = 0; i < 400 && bt.open; i++) {
    avanza(16);
    if (bt.phase == BP_ATTACK || bt.phase == BP_HIT) { siguio = true; break; }
  }
  CHECK_MSG(siguio, "tras la mega del rival el turno se quedo colgado");

  while (bt.open) { bt.phase = BP_END; bt.until = millis(); avanza(32); }
}

// ====================================================================
// LOS TIPOS. El dueno: "nenhum lugar mostra o tipo do pokemon que
// temos, nem na pokedex e nem no menu". La chapa existia desde hacia
// tiempo (drawTypeChip, con icono y color) pero solo la usaba la
// pantalla de gimnasio para decir de que tipo era LA CASA.
//
// Se mide sobre PIXELES y no sobre una bandera: lo que fallaba era que
// no se dibujaba nada, asi que la unica afirmacion que prueba algo es
// contar el color de la chapa dentro del rectangulo donde tiene que
// estar. El color se saca igual que drawTypeChip lo elige.
// ====================================================================

// El color lo decide el sketch (colorDeChapaTipo, en TamaPoke.ino): la
// chapa lleva icono en unos tipos y no en otros, y copiar esa regla
// aqui seria tener dos verdades —— la del test se quedaria vieja al
// primer tipo que estrene icono.
uint16_t colorDeChapaTipo(uint8_t tipo);
extern bool cardOpen;
extern uint8_t cardPage;
// La flecha que pasa de version en la ficha (GVER_X/GVER_Y de
// TamaPoke.ino), igual que el resto de la geometria de este fichero.
#define GVER_X_T 70
#define GVER_Y_T 258

static int pixelesDeColor(uint16_t col, int x0, int y0, int x1, int y1) {
  const uint16_t *fb = gfx->getFramebuffer();
  int n = 0;
  for (int y = y0; y <= y1; y++)
    for (int x = x0; x <= x1; x++)
      if (fb[y * gfx->width() + x] == col) n++;
  return n;
}

// La franja de la ficha de COMBATE y la columna de la Pokedex, copiadas
// de TamaPoke.ino. Si se mueven alli, estos numeros tienen que moverse.
#define TIPOS_CARD_X0 124
#define TIPOS_CARD_X1 342
#define TIPOS_CARD_Y0 264
#define TIPOS_CARD_Y1 292
#define TIPOS_DEX_X0 26
#define TIPOS_DEX_X1 134
#define TIPOS_DEX_Y0 164
#define TIPOS_DEX_Y1 226

// GINASIO e MEDALHAS sairam da ficha para o mapa (2026-11-13). Os numeros
// sao os de TamaPoke.ino (MAP_BT_*).
#define MAP_BT_Y_T 372
#define MAP_BT_H_T 40
#define MAP_BT_W_T 108
#define MAP_BT_LX_T 118
#define MAP_BT_RX_T 240
extern bool cardSolo, mapOpen;
extern uint8_t mapView;
void onSwipeV(int dir);

TEST(ficha, arrastando_nao_passa_mais_por_ginasio_nem_medalhas) {
  arranca();
  if (pet.awaitingStarter()) pet.chooseStarter(1);
  while (pet.isEgg()) pet.eggTap();
  onSwipeV(-1);                          // abre a ficha pelo gesto
  avanza(200);
  CHECK_MSG(cardOpen && !cardSolo && cardPage == 0, "premissa: a ficha abriu");
  std::string vistas = "0";
  for (int i = 0; i < 12; i++) {
    onSwipe(-1);                         // esquerda avanca
    avanza(80);
    if (std::to_string(cardPage) != vistas.substr(vistas.size() - 1))
      vistas += std::to_string(cardPage);
  }
  CHECK_EQ(vistas, std::string("0123468"));
  onSwipeV(-1);
  avanza(200);
  CHECK(!cardOpen);
}

TEST(mapa, os_botoes_abrem_ginasio_e_medalhas_e_voltam_ao_mapa) {
  arranca();
  if (pet.awaitingStarter()) pet.chooseStarter(1);
  while (pet.isEgg()) pet.eggTap();
  mapOpen = true;
  mapView = pet.region;
  avanza(300);
  toca(MAP_BT_LX_T + MAP_BT_W_T / 2, MAP_BT_Y_T + MAP_BT_H_T / 2);
  CHECK_MSG(cardOpen && cardSolo && cardPage == 5 && !mapOpen,
            "o botao da esquerda nao abriu o ginasio");
  onSwipe(-1);                           // solo: arrastar nao pagina
  avanza(80);
  CHECK_EQ((int)cardPage, 5);
  onSwipeV(-1);                          // fecha
  avanza(300);
  CHECK_MSG(!cardOpen && mapOpen && mapView == pet.region,
            "fechar o ginasio nao voltou ao mapa");
  toca(MAP_BT_RX_T + MAP_BT_W_T / 2, MAP_BT_Y_T + MAP_BT_H_T / 2);
  CHECK_MSG(cardOpen && cardSolo && cardPage == 7 && !mapOpen,
            "o botao da direita nao abriu as medalhas");
  onSwipeV(-1);
  avanza(300);
  CHECK_MSG(!cardOpen && mapOpen, "fechar as medalhas nao voltou ao mapa");
  mapOpen = false;
  avanza(100);
}

TEST(tipos, la_pagina_de_combate_dice_LOS_DOS_tipos) {
  arranca();
  if (pet.awaitingStarter()) pet.chooseStarter(1);
  while (pet.isEgg()) pet.eggTap();

  // Charizard: fuego Y volador. Una especie de dos tipos es la unica
  // que distingue "sale el tipo" de "sale el primero y ya".
  pet.speciesId = 6;
  pet.form = 0;
  const uint8_t t1 = DEX_TBL[6].type1, t2 = DEX_TBL[6].type2;
  CHECK_MSG(t1 != T_NONE && t2 != T_NONE && t1 != t2,
            "premisa: la especie de prueba tiene que tener dos tipos");

  cardOpen = true;
  cardPage = 1;
  avanza(400);
  int a = pixelesDeColor(colorDeChapaTipo(t1), TIPOS_CARD_X0, TIPOS_CARD_Y0,
                         TIPOS_CARD_X1, TIPOS_CARD_Y1);
  int b = pixelesDeColor(colorDeChapaTipo(t2), TIPOS_CARD_X0, TIPOS_CARD_Y0,
                         TIPOS_CARD_X1, TIPOS_CARD_Y1);
  cardOpen = false;
  cardPage = 0;
  CHECK_MSG(a > 400, "no esta la chapa del primer tipo: " + std::to_string(a) +
                     " pixeles de su color");
  CHECK_MSG(b > 400, "no esta la chapa del segundo tipo: " + std::to_string(b) +
                     " pixeles de su color");
}

TEST(tipos, la_ficha_de_la_pokedex_dice_el_tipo) {
  arranca();
  // Sin esto, corriendo SOLO (que es como los corre la sabotagem) la
  // pantalla se queda en la de elegir inicial: su ramo va antes que la
  // Pokedex y que la ficha en render(), y acaba en return. El test
  // medira entonces una pantalla que no es la suya —— y se quedo verde
  // con el fallo puesto, que es como se descubrio.
  if (pet.awaitingStarter()) pet.chooseStarter(1);
  while (pet.isEgg()) pet.eggTap();
  pet.registerSpecies(6, false);
  openDexEntry(6);
  avanza(400);
  int a = pixelesDeColor(colorDeChapaTipo(DEX_TBL[6].type1), TIPOS_DEX_X0,
                         TIPOS_DEX_Y0, TIPOS_DEX_X1, TIPOS_DEX_Y1);
  int b = pixelesDeColor(colorDeChapaTipo(DEX_TBL[6].type2), TIPOS_DEX_X0,
                         TIPOS_DEX_Y0, TIPOS_DEX_X1, TIPOS_DEX_Y1);
  cierraLaFicha();
  CHECK_MSG(a > 400, "falta el primer tipo en la ficha: " + std::to_string(a));
  CHECK_MSG(b > 400, "falta el segundo tipo en la ficha: " + std::to_string(b));
}

// De un bicho que no has criado NO se sabe el tipo, igual que el nombre
// sale "???" y no hay entrada de Pokedex. Sin esta, la chapa seria un
// chivatazo de lo que todavia no has encontrado.
TEST(tipos, de_uno_sin_registrar_NO_se_dice_el_tipo) {
  arranca();
  // Sin esto, corriendo SOLO (que es como los corre la sabotagem) la
  // pantalla se queda en la de elegir inicial: su ramo va antes que la
  // Pokedex y que la ficha en render(), y acaba en return. El test
  // medira entonces una pantalla que no es la suya —— y se quedo verde
  // con el fallo puesto, que es como se descubrio.
  if (pet.awaitingStarter()) pet.chooseStarter(1);
  while (pet.isEgg()) pet.eggTap();
  // Uno que seguro no esta: se desmarca a mano.
  const int16_t quien = 149;   // Dragonite
  pet.dexReg[(quien - 1) >> 3] &= (uint8_t) ~(1 << ((quien - 1) & 7));
  CHECK_MSG(!pet.isRegistered(quien), "premisa: tiene que estar sin registrar");
  openDexEntry(quien);
  avanza(400);
  int a = pixelesDeColor(colorDeChapaTipo(DEX_TBL[quien].type1), TIPOS_DEX_X0,
                         TIPOS_DEX_Y0, TIPOS_DEX_X1, TIPOS_DEX_Y1);
  // CUALQUIER chapa, no solo la suya: contando solo el color de su tipo,
  // pintar la chapa EQUIVOCADA ahi habria pasado por buena.
  int cualquiera = 0;
  for (uint8_t t = 1; t < T_COUNT; t++)
    cualquiera += pixelesDeColor(colorDeChapaTipo(t), TIPOS_DEX_X0,
                                 TIPOS_DEX_Y0, TIPOS_DEX_X1, TIPOS_DEX_Y1);
  cierraLaFicha();
  CHECK_MSG(a < 50, "de un bicho sin registrar no se puede decir el tipo, y "
                    "se pintaron " + std::to_string(a) + " pixeles de su chapa");
  CHECK_MSG(cualquiera < 50,
            "no puede haber NINGUNA chapa en la ficha de uno sin criar: "
            + std::to_string(cualquiera) + " pixeles de color de tipo");
}

// Y EL DE LA VERSION QUE SE MIRA, no el de la especie base: una mega
// cambia de tipo, y ensenar el del normal mientras se mira al Mega
// Charizard X seria mentir a la cara.
TEST(tipos, una_mega_ensena_SU_tipo_y_no_el_de_la_base) {
  arranca();
  // Sin esto, corriendo SOLO (que es como los corre la sabotagem) la
  // pantalla se queda en la de elegir inicial: su ramo va antes que la
  // Pokedex y que la ficha en render(), y acaba en return. El test
  // medira entonces una pantalla que no es la suya —— y se quedo verde
  // con el fallo puesto, que es como se descubrio.
  if (pet.awaitingStarter()) pet.chooseStarter(1);
  while (pet.isEgg()) pet.eggTap();

  // Una mega cuyo SEGUNDO tipo no sea el de su especie, y que los dos
  // existan: es el unico caso en que la chapa distingue leer MEGA_TBL
  // de leer DEX_TBL.
  int16_t fila = -1;
  for (uint16_t i = 0; i < MEGA_COUNT && fila < 0; i++) {
    int16_t b = MEGA_TBL[i].base;
    if (b < 1 || b > DEX_COUNT) continue;
    if (MEGA_TBL[i].type2 != T_NONE && DEX_TBL[b].type2 != T_NONE &&
        MEGA_TBL[i].type2 != DEX_TBL[b].type2 &&
        MEGA_TBL[i].type2 != DEX_TBL[b].type1)
      fila = (int16_t)i;
  }
  CHECK_MSG(fila >= 0, "no hay ninguna mega que cambie de segundo tipo");
  const int16_t base = MEGA_TBL[fila].base;

  pet.registerSpecies(base, false);
  pet.registerMega(fila);
  openDexEntry(base);
  int8_t v = versionDeLaMega(fila);
  CHECK_MSG(v >= 0, "la mega no esta en la lista de versiones");
  // Por la FLECHA, que es el camino del jugador: tocarla pasa de
  // version y recarga el sprite. Poner galleryVer a mano dejaria el
  // sprite sin recargar, o sea un estado que jugando no existe.
  for (int i = 0; i < v; i++) toca(GVER_X_T, GVER_Y_T);
  CHECK_MSG(galleryVer == v, "la flecha no llego a la version de la mega");
  avanza(400);
  int suyo = pixelesDeColor(colorDeChapaTipo(MEGA_TBL[fila].type2), TIPOS_DEX_X0,
                            TIPOS_DEX_Y0, TIPOS_DEX_X1, TIPOS_DEX_Y1);
  int deLaBase = pixelesDeColor(colorDeChapaTipo(DEX_TBL[base].type2), TIPOS_DEX_X0,
                                TIPOS_DEX_Y0, TIPOS_DEX_X1, TIPOS_DEX_Y1);
  cierraLaFicha();
  CHECK_MSG(suyo > 400,
            std::string(MEGA_TBL[fila].name) + " no ensena su propio tipo: " +
            std::to_string(suyo) + " pixeles");
  CHECK_MSG(deLaBase < 50,
            "sigue ensenando el tipo de la especie base: " +
            std::to_string(deLaBase) + " pixeles");
}

// ================================================ ELEGIR EN QUIEN
// "revive esta quebrado, nao tem como usar na batalha pois nao deixa eu
// escolher o pokemon". La bolsa aplicaba TODO al que estaba en pista,
// asi que un Revivir no tenia a quien levantar: el de pista, por
// definicion, esta en pie. Ahora los objetos que el PBS del Anil marca
// `BattleUse = OnPokemon` abren la lista del equipo.
//
// Se mide por el CAMINO DEL JUGADOR —— tocar la fila de la bolsa y
// luego la del equipo ——, no llamando a la funcion: lo que estaba roto
// era justamente que no habia forma de llegar.
//

// Un suplente CAIDO en un hueco limpio. Devuelve el hueco, o 0xFF.
static uint8_t suplenteCaido(int16_t especie) {
  for (uint8_t i = 0; i < TEAM_SLOTS; i++)
    if (i != pet.mainSlot) pet.team[i] = StoredMon{};
  StoredMon sup{};
  sup.species = especie;
  sup.exp = Pet::expForLevel(10);
  sup.moves[0] = MOVE_STRUGGLE;
  sup.ppLeft[0] = 5;
  sup.hpLeft = 0;            // caido: es a quien hay que poder apuntar
  if (!pet.addToTeam(sup)) return 0xFF;
  for (uint8_t i = 0; i < TEAM_SLOTS; i++)
    if (i != pet.mainSlot && pet.team[i].species == especie) return i;
  return 0xFF;
}

TEST(objetos_ui, un_revive_abre_la_lista_en_vez_de_aplicarse_solo) {
  arranca();
  uint8_t rev = objetoQueHace(IT_REVIVE, 50);
  CHECK_MSG(rev, "no hay Revivir en la tabla");
  enCombateConLaBolsa(rev);
  CHECK_MSG(bt.phase == BP_CHOOSE, "no se llego a elegir golpe");
  uint8_t hueco = suplenteCaido(25);
  CHECK_MSG(hueco != 0xFF, "no se pudo meter un suplente caido");

  btOpenBag();
  avanza(120);
  CHECK_MSG(bt.phase == BP_BAG, "la bolsa no se abrio");
  uint8_t antes = pet.bag[rev];

  // La primera fila de la bolsa, por el toque de verdad.
  toca(200, BBAG_Y_T + 17);
  CHECK_MSG(bt.phase == BP_PARTY,
            "tocar el Revivir tiene que ABRIR LA LISTA, y la fase quedo en " +
            std::to_string((int)bt.phase));
  CHECK_MSG(bt.itemPick == rev, "y llevarlo en la mano");
  CHECK_MSG(pet.bag[rev] == antes,
            "todavia no se gasta: no se ha elegido a nadie");
  CHECK_MSG(pet.team[hueco].hpLeft == 0, "ni se ha levantado a nadie");

  // Y la fila del caido.
  toca(BPTY_X_T + 40, BPTY_Y_T + hueco * BPTY_H_T + 16);
  CHECK_MSG(pet.team[hueco].hpLeft > 0,
            "el caido tiene que levantarse: quedo con " +
            std::to_string(pet.team[hueco].hpLeft));
  CHECK_MSG(pet.bag[rev] == antes - 1, "y AHORA si se gasta el objeto");
  CHECK_MSG(bt.itemPick == 0, "y se sueltan las manos");
  CHECK_MSG(strstr(bt.msg, "Revive") != nullptr,
            "y se dice que se uso: la caja decia '" + std::string(bt.msg) + "'");

  cierraElCombate();
}

TEST(objetos_ui, tocar_fuera_devuelve_a_la_bolsa_sin_gastar_nada) {
  arranca();
  uint8_t rev = objetoQueHace(IT_REVIVE, 50);
  enCombateConLaBolsa(rev);
  uint8_t hueco = suplenteCaido(25);
  CHECK_MSG(hueco != 0xFF, "no se pudo meter un suplente caido");
  btOpenBag();
  avanza(120);
  uint8_t antes = pet.bag[rev];
  toca(200, BBAG_Y_T + 17);
  CHECK_MSG(bt.phase == BP_PARTY, "premisa: la lista tiene que estar abierta");
  toca(233, 70);                     // arriba, fuera de las seis filas
  CHECK_MSG(bt.phase == BP_BAG,
            "tocar fuera vuelve a la BOLSA, no al menu de golpes");
  CHECK_MSG(bt.itemPick == 0, "y se suelta el objeto");
  CHECK_MSG(pet.bag[rev] == antes, "sin gastar nada");
  cierraElCombate();
}

TEST(objetos_ui, una_X_se_aplica_SIN_preguntar) {
  // La otra mitad: si TODO abriese la lista, las X —— que suben
  // escalones, y los escalones son del que esta en pista —— pedirian un
  // toque de mas para algo que no se puede elegir. El PBS las marca
  // OnBattler, y por eso no preguntan.
  arranca();
  uint8_t x = objetoQueHace(IT_XSTAT, SG_ATK);
  CHECK_MSG(x, "no hay X Attack en la tabla");
  enCombateConLaBolsa(x);
  btOpenBag();
  avanza(120);
  uint8_t antes = pet.bag[x];
  toca(200, BBAG_Y_T + 17);
  CHECK_MSG(bt.phase != BP_PARTY, "una X no tiene a quien preguntar");
  CHECK_MSG(bt.b.side[BT_PLAYER].stage[SG_ATK] == 2,
            "tiene que haber subido DOS escalones ya mismo, y quedo en " +
            std::to_string((int)bt.b.side[BT_PLAYER].stage[SG_ATK]));
  CHECK_MSG(pet.bag[x] == antes - 1, "y gastarse");
  cierraElCombate();
}

TEST(objetos_ui, la_lista_apaga_a_quien_el_objeto_no_le_haria_nada) {
  // Un Revivir sobre uno EN PIE no hace nada, y eso tiene que notarse
  // ANTES de tocarlo: si no, es un objeto tirado a la basura.
  arranca();
  uint8_t rev = objetoQueHace(IT_REVIVE, 50);
  enCombateConLaBolsa(rev);
  uint8_t hueco = suplenteCaido(25);
  CHECK_MSG(hueco != 0xFF, "no se pudo meter un suplente");

  CHECK_MSG(btItemSirveEn(rev, hueco), "a un caido, el Revivir SI le sirve");
  pet.team[hueco].hpLeft = 1;        // ahora en pie
  CHECK_MSG(!btItemSirveEn(rev, hueco),
            "a uno en pie, un Revivir no le hace nada");

  // Y el toque sobre el que no sirve no gasta nada ni cierra la lista.
  btOpenBag();
  avanza(120);
  uint8_t antes = pet.bag[rev];
  toca(200, BBAG_Y_T + 17);
  toca(BPTY_X_T + 40, BPTY_Y_T + hueco * BPTY_H_T + 16);
  CHECK_MSG(pet.bag[rev] == antes,
            "tocar a quien no le sirve no puede gastar el objeto");
  CHECK_MSG(bt.itemPick == rev, "y deja la lista abierta para elegir otro");
  cierraElCombate();
}


// ====================================================== LA SOMBRA
// "as sombras no chao... mtos pokemon a sombra nao casa (pé voando,
// sensacao que estao flutuando)".
//
// La sombra se centraba en el medio de la TINTA del sprite, y un ala
// abierta o una cola larga se llevan ese medio lejos de donde el bicho
// APOYA. Medido sobre las 1025 (tools/mide_sombras.py y el de centro):
// 819 especies a 3 px o mas, mediana 12, Lugia 97.
//
// Se mide donde cae la mancha EN LA PANTALLA, no `centroPie()`: leer la
// variable nueva seria un teste que sobrevive a desconectarla del
// dibujo. Se busca la fila del suelo y se compara el centro de lo que
// se oscurecio con el centro de los pies del sprite.

// La linea de suelo del rival (BT_GROUND_FOE de battle_ui.ino, que es
// un #define y no se ve desde aqui) y su sprite.
#define BT_GROUND_FOE_T 178
extern PmdMon foePmd;

// Cuanto se oscurecio cada columna de la fila `y` respecto de la de
// arriba del todo (que es cielo o fondo sin sombra).
static int sombraCentroEn(int y, int x0, int x1, int *ancho) {
  const uint16_t *fb = gfx->getFramebuffer();
  int w = gfx->width();
  long suma = 0, peso = 0;
  int n = 0;
  for (int x = x0; x <= x1; x++) {
    uint16_t aqui = fb[y * w + x];
    uint16_t arriba = fb[(y + 30) * w + x];   // mismo suelo, sin sombra
    int dq = (int)((aqui >> 11) & 31) - (int)((arriba >> 11) & 31);
    int dg = (int)((aqui >> 5) & 63) - (int)((arriba >> 5) & 63);
    if (dq < -1 && dg < -2) {                 // mas oscuro: es sombra
      suma += (long)x * 1;
      peso += 1;
      n++;
    }
  }
  if (ancho) *ancho = n;
  return peso ? (int)(suma / peso) : -1;
}

// A cor do cadeado. E C565(0x8a, 0x93, 0xb0) do TamaPoke.ino, escrita
// aqui como numero porque o C565 e macro do .ino e este arquivo nao o
// enxerga: (138>>3)<<11 | (147>>2)<<5 | (176>>3) = 0x8C96.
#define COR_CANDADO 0x8C96
// A pagina 4 tem DUAS abas: o time e a caixa. Sem isto o teste
// renderizava o time, onde cadeado nenhum aparece —— e media 0.
extern bool bankView;

extern int8_t bankSoltar;

// "No banco, adicionar alguma opcao para soltar o pokemon selecionado
// (com msg de confirmacao)". O X da fila PERGUNTA; so o SIM solta; e o
// que ele levava volta para a bolsa.
TEST(banco, o_X_pergunta_e_so_o_SIM_solta) {
  arranca();
  if (pet.awaitingStarter()) pet.chooseStarter(1);
  while (pet.isEgg()) pet.eggTap();
  for (uint8_t i = 0; i < BANK_SLOTS; i++) pet.bank[i].species = 0;
  StoredMon m{};
  m.species = 275;                    // Shiftry
  m.exp = Pet::expForLevel(5);
  m.hpLeft = 10;
  m.moves[0] = MOVE_STRUGGLE;
  m.item = 1;
  pet.bank[3] = m;
  uint8_t itensAntes = pet.bag[1];

  cardOpen = true;
  cardPage = 4;
  bankView = true;
  avanza(200);
  const int yFila = 126 + 17;   // BANK_ROW_Y0: a primeira (e unica) fila

  toca(44, yFila);                      // o X
  CHECK_MSG(bankSoltar == 3, "o X nao abriu a pergunta");
  CHECK_MSG(pet.bank[3].used(), "o X sozinho ja soltou: nao pode");
  avanza(200);
  volcaPantalla("banco_soltar");
  toca(298, 278);                       // NAO
  CHECK_MSG(pet.bank[3].used(), "o NAO soltou o bicho");
  CHECK_MSG(cardOpen, "o NAO fechou a ficha");

  toca(200, yFila);                     // o corpo da fila continua trazendo
  // (o time pode estar cheio; o que importa e que NAO soltou)
  bool aindaExiste = pet.bank[3].used();
  for (uint8_t i = 0; i < TEAM_SLOTS; i++)
    if (pet.team[i].species == 275) aindaExiste = true;
  CHECK_MSG(aindaExiste, "tocar a fila soltou o bicho");
  if (!pet.bank[3].used()) {
    // voltou para o time: devolve-o a caixa para o resto do teste
    for (uint8_t i = 0; i < TEAM_SLOTS; i++)
      if (pet.team[i].species == 275) { pet.team[i].species = 0; }
    pet.bank[3] = m;
  }

  toca(44, yFila);
  toca(168, 278);                       // SIM
  CHECK_MSG(!pet.bank[3].used(), "o SIM nao soltou");
  CHECK_EQ((int)pet.bag[1], (int)itensAntes + 1);
  CHECK_MSG(bankSoltar < 0, "a pergunta ficou aberta");

  cardOpen = false;
  cardPage = 0;
  bankView = false;
}

TEST(cadeado, el_banco_marca_a_quien_pasa_del_techo) {
  // "garantia que no banco, aparece sei la, um cadeadinho em cima do
  // pokemon que ultrapassa o cap para nao pegarmos".
  //
  // A recusa ja funcionava sozinha (fromBank devolve false), mas sem o
  // desenho o jogador toca, ouve um beep e acha que travou.
  arranca();
  if (pet.awaitingStarter()) pet.chooseStarter(1);
  while (pet.isEgg()) pet.eggTap();

  // A caixa, limpa e com UM so: assim o que se mede e dele.
  for (uint8_t i = 0; i < BANK_SLOTS; i++) pet.bank[i].species = 0;
  StoredMon m{};
  m.species = 7;
  m.exp = Pet::expForLevel(60);
  m.hpLeft = 10;
  m.moves[0] = MOVE_STRUGGLE;
  m.geneHp = m.geneAtk = m.geneDef = 100;
  m.geneSpa = m.geneSpd = m.geneSpe = 100;
  pet.bank[0] = m;
  CHECK_MSG(pet.bankLocked(pet.bank[0]),
            "premissa: um nivel 60 tem que passar do teto daqui");

  cardOpen = true;
  cardPage = 4;
  bankView = true;              // a aba da CAIXA, nao a do time
  avanza(400);
  int conCandado = pixelesDeColor(COR_CANDADO, 350, 100, 378, 310);

  // E o MESMO bicho, agora dentro do teto: o cadeado tem que sumir. Sem
  // esta metade o teste passaria com um cadeado pintado SEMPRE.
  pet.bank[0].exp = Pet::expForLevel(3);
  CHECK_MSG(!pet.bankLocked(pet.bank[0]), "premissa: um nivel 3 cabe");
  avanza(400);
  int sinCandado = pixelesDeColor(COR_CANDADO, 350, 100, 378, 310);

  cardOpen = false;
  cardPage = 0;
  bankView = false;
  for (uint8_t i = 0; i < BANK_SLOTS; i++) pet.bank[i].species = 0;

  CHECK_MSG(conCandado > 20,
            "nao pintou o cadeado: " + std::to_string(conCandado) + " pixeis");
  CHECK_MSG(sinCandado == 0,
            "o cadeado continua la com o bicho dentro do teto: " +
                std::to_string(sinCandado) + " pixeis");
}

// A cruz de raiva tem UMA cor so na paleta (0xF800, vermelho puro), o
// que a torna um marcador sem ambiguidade: se ela esta na tela, foi
// desenhada.
#define COR_RAIVA 0xF800
// O roxo escuro da nuvem de sujeira (ART_SUJEIRA, celula 0 do Emote3).
#define COR_SUJEIRA 0xA254
extern uint32_t determUntil;   // o balao de decidido do matinho
extern bool gestosOn;          // gestos do acelerometro

// Despeja a tela num PPM, pra poder OLHAR o que o teste viu. Um teste
// de pixeis diz "ha 180 pixeis vermelhos"; nao diz se o balao ficou
// bonito nem se o simbolo esta montado em cima da cara. Isto nao
// afirma nada —— e so para eu e o dono vermos.
static void volcaPantalla(const char *nombre) {
  const uint16_t *fb = gfx->getFramebuffer();
  int w = gfx->width(), h = gfx->height();
  char ruta[160];
  snprintf(ruta, sizeof(ruta), "../_claude_scratch/prev_%s.ppm", nombre);
  FILE *f = fopen(ruta, "wb");
  if (!f) return;
  // O %c com 10 e a quebra de linha. Escrita assim de proposito: esta
  // linha ja se partiu uma vez por causa de um heredoc que comeu as
  // barras, e um cabecalho PPM partido nao da erro, da arquivo torto.
  fprintf(f, "P6%c%d %d%c255%c", 10, w, h, 10, 10);
  for (int i = 0; i < w * h; i++) {
    uint16_t c = fb[i];
    unsigned char rgb[3] = { (unsigned char)(((c >> 11) & 31) << 3),
                             (unsigned char)(((c >> 5) & 63) << 2),
                             (unsigned char)((c & 31) << 3) };
    fwrite(rgb, 1, 3, f);
  }
  fclose(f);
}

TEST(matinho, un_toque_lo_manda_al_COMBATE_sin_aviso_arriba) {
  // Antes, tocar no matinho largava o "!" no ALTO da tela —— o mesmo
  // canto onde as vezes aparece o treinador —— e so um SEGUNDO toque
  // ali comecava o combate. Dois toques, e o olhar jogado pra longe do
  // bicho bem na hora em que ele e o assunto.
  arranca();
  if (pet.awaitingStarter()) pet.chooseStarter(1);
  while (pet.isEgg()) pet.eggTap();
  pet.sleeping = false;
  pet.energy = 100;
  pet.fullness = 80;
  pet.wildClear();
  pet.huntUntil = 0;          // pronto pra cacar
  determUntil = 0;
  bt.open = false;   // como fazem os outros testes de UI
  avanza(200);
  CHECK_MSG(!bt.open, "premissa: o combate tem que comecar fechado");

  toca(196, 419);             // o botao do matinho
  CHECK_MSG(determUntil != 0, "o balao de decidido tem que comecar");
  CHECK_MSG(!pet.wildWaiting(),
            "e NAO pode sobrar o aviso de '!' no alto da tela");
  CHECK_MSG(!bt.open, "o combate ainda nao: primeiro o balao");

  avanza(1500);               // passa o DETERM_MS
  CHECK_MSG(bt.open, "passado o balao, o combate tem que entrar sozinho");
  bt.open = false;   // como fazem os outros testes de UI
  avanza(100);
}

TEST(vitoria, la_cara_no_tapa_nada) {
  arranca();
  if (pet.awaitingStarter()) pet.chooseStarter(1);
  while (pet.isEgg()) pet.eggTap();
  pet.sleeping = false;
  pet.energy = 100;
  gestosOn = false;
  galleryOpen = false;
  galleryDetail = 0;
  cardOpen = false;
  determUntil = 0;
  mockTouchSet(0, 0, false);
  mockTouchTick();

  battleStart(16, 10, -1, false, -1);
  for (int i = 0; i < 400 && bt.phase != BP_CHOOSE; i++) avanza(16);
  CHECK_MSG(bt.phase == BP_CHOOSE, "premissa: o combate tem que abrir");

  // Forca-se a tela de vitoria: o que se quer olhar e o DESENHO dela,
  // nao o caminho ate ela.
  // O CASO CHEIO, nao o vazio. Foi aqui que eu errei: olhei uma vitoria
  // sem subida e sem premio, vi espaco livre no meio da janela e pus a
  // cara ali —— mas esse espaco e justamente onde entram o
  // "Lv.A -> Lv.B" e os seis stats quando se sobe de nivel, mais os
  // itens recolhidos. Se o layout aguenta este caso, aguenta o outro.
  bt.winDex = pet.speciesId;
  bt.expWon = 137;
  bt.loot[0] = 1;
  bt.loot[1] = 2;
  bt.lvlAntes = 4;
  bt.lvlWon = 1;
  bt.phase = BP_WIN;
  avanza(500);
  volcaPantalla("vitoria");

  // A QUEIXA, virada em teste: "a imagem ficou na frente de onde esta
  // escrito o lvl".
  //
  // O retrato tem fundo opaco e de cor propria, que nao existe no resto
  // da janela. Entao pega-se essa cor dentro da cara e exige-se que ela
  // NAO apareca na faixa do "Lv.N" (alinhado a direita terminando em
  // x=366, na linha y=142). Se alguem subir a cara de novo, cai aqui.
  const uint16_t *fb = gfx->getFramebuffer();
  const int w = gfx->width();
  // O retrato tem fundo opaco de cor propria, que nao existe no resto
  // da janela: serve de marcador. Pega-se dentro da cara (que agora
  // vive no canto de cima a direita) e exige-se que ela nao apareca
  // NEM na barra de EXP NEM na faixa dos itens e dos stats.
  const uint16_t corDaCara = fb[150 * w + 348];
  int naBarra = 0, nosStats = 0;
  for (int y = 180; y <= 206; y++)
    for (int x = 86; x <= 386; x++)
      if (fb[y * w + x] == corDaCara) naBarra++;
  for (int y = 230; y <= 335; y++)
    for (int x = 86; x <= 386; x++)
      if (fb[y * w + x] == corDaCara) nosStats++;
  const int invasores = naBarra + nosStats;

  bt.open = false;
  avanza(100);
  CHECK_MSG(invasores == 0,
            "a cara tapa o que nao devia: " + std::to_string(naBarra) +
                " pixeis na barra de EXP e " + std::to_string(nosStats) +
                " na faixa dos itens e stats");
}

TEST(reacao, el_globo_ensena_la_onomatopeya_de_lo_que_pide) {
  // A onomatopeia vai em TODOS os estados de proposito: so o retrato
  // `Normal` existe pras 1025 especies, as outras ficam em 70-78%.
  // Quando a cara que sobra e a neutra, quem diz o que esta
  // acontecendo e o simbolo —— entao se ele nao aparecer, a reacao nao
  // se le, mesmo com a cara certa carregada.
  arranca();
  if (pet.awaitingStarter()) pet.chooseStarter(1);
  while (pet.isEgg()) pet.eggTap();
  pet.sleeping = false;

  // A tela de CASA, e nao outra. Este teste ja mediu 0 e 0 porque
  // um teste anterior deixou a Pokedex aberta: media uma tela que
  // nao era a dele e a conta dava zero dos dois lados.
  galleryOpen = false;
  galleryDetail = 0;
  cardOpen = false;
  bt.open = false;
  determUntil = 0;
  // E o que TAPA o balao. Olhando o despejo da bateria apareceu um
  // cartaz de medalha ("BEAT PALDEA") cobrindo o meio da tela e o
  // coracao de carinho aceso —— e o coracao manda sobre o pedido DE
  // PROPOSITO (quem acabou de fazer carinho nao ouve "estou com fome").
  // Nenhum dos dois era defeito: era lixo de outro teste.
  // heartUntil, medalUntil e milestoneUntil sao privados —— e e bom que
  // sejam. Todos os tres sao por TEMPO, entao deixa-se o tempo passar,
  // que e o que aconteceria no jogo. O newMedal e publico e tem que ir
  // junto: o cartaz exige os dois.
  pet.newMedal = 0;
  // SOLTAR O DEDO. Se um teste anterior acabou com o toque pressionado,
  // cada loop() volta a ler um toque em cima do bicho —— e tocar no
  // bicho e CARINHO, entao o coracao reacende para sempre e nunca ha
  // balao de pedido. Foi isso: o coracao nao apagava nem depois de
  // vinte segundos de espera.
  mockTouchSet(0, 0, false);
  mockTouchTick();
  avanza(100);
  // O CORACAO. Ele manda sobre o pedido de proposito —— quem acabou de
  // fazer carinho nao ouve "estou com fome" ——, entao enquanto estiver
  // aceso nao ha balao de pedido nenhum. Na bateria ele chegava aceso
  // aqui, e o teste media uma tela sem balao e dizia "a cruz nao
  // apareceu", que aponta pro sintoma e nao pra causa.
  //
  // Espera-se ele apagar em vez de zerar o campo (que e privado, e e
  // bom que seja), e exige-se que tenha apagado.
  // OS GESTOS DO ACELEROMETRO, desligados enquanto se mede.
  //
  // Esta foi a causa, e levou cinco palpites errados ate eu parar de
  // supor e medir: com os gestos ligados, loop() le imuGestos() a cada
  // quadro, e no arnes de UI nao ha mock de IMU —— o que ele devolve
  // passa por CARICIA todo quadro. Carinho acende o coracao, e o
  // coracao manda sobre o pedido de proposito. Resultado: o coracao
  // nao apagava nem depois de 900 s simulados, porque estava sendo
  // reaceso o tempo todo.
  const bool gestosAntes = gestosOn;
  gestosOn = false;
  int saltos = 0;
  for (; saltos < 400 && pet.showHeart(); saltos++) avanza(60000, 2000);
  CHECK_MSG(!pet.showHeart(),
            "premissa: o coracao tem que apagar antes de medir o pedido");
  // Trinta minutos simulados, medido: o prazo do coracao vem de
  // `heartUntil = ceremonyUntil` (coracoes durante toda a despedida,
  // que e proposital) e outro teste deixou a cerimonia pelo caminho.
  // Nao e defeito do jogo: numa despedida de verdade o prazo expira
  // junto com ela. Mas o teste tem que esperar, e o salto e de um
  // minuto por volta porque de cem em cem milissegundos nao chegaria la.

  // Limpo e satisfeito: nao pede nada, entao nao ha simbolo nenhum.
  pet.fullness = 90; pet.joy = 90; pet.energy = 90; pet.hygiene = 90;
  avanza(400);
  int limpo = pixelesDeColor(COR_SUJEIRA, 70, 40, 396, 300);
  volcaPantalla("limpo");

  // Sujo: pede banho, e a nuvem roxa de sujeira tem que aparecer.
  pet.hygiene = 5;
  CHECK_MSG(pet.need() == NEED_CLEAN,
            "premissa: com a higiene no chao ele tem que pedir banho");
  avanza(400);
  int sujo = pixelesDeColor(COR_SUJEIRA, 70, 40, 396, 300);
  volcaPantalla("sujo");
  // PREMISSA: o balao tem que estar MESMO na tela. Sem isto o teste
  // media 0 e 0 e dizia "a cruz nao apareceu", quando na verdade nao
  // havia balao nenhum —— outro teste tinha deixado um cartaz por cima.
  // Uma falha tem que apontar para a causa, nao para o sintoma.
  // As condicoes EXATAS pra haver balao de pedido. Checa-las aqui faz a
  // falha apontar pra causa; contar pixeis brancos nao fazia, porque ha
  // branco de sobra na tela que nao e balao nenhum.
  CHECK_MSG(!pet.showHeart(), "o coracao voltou a acender e tapa o pedido");
  CHECK_MSG(determUntil == 0, "o balao de decidido esta por cima");
  CHECK_MSG(pet.need() == NEED_CLEAN, "ele deixou de pedir banho");

  pet.hygiene = 90;
  gestosOn = gestosAntes;     // devolver o estado global que mexi
  // DIFERENCIAL, nao absoluto. Medindo "satisfeito tem zero vermelho"
  // o teste caiu com 57: ha outra coisa 0xF800 nessa janela (o proprio
  // bicho, os botoes), e isso nao e defeito nenhum. O que importa e
  // quanto vermelho APARECE quando ele passa a pedir banho —— e e isso
  // que some se o simbolo deixar de ser desenhado.
  CHECK_MSG(sujo > limpo + 60,
            "a nuvem de sujeira nao apareceu ao sujar: limpo " + std::to_string(limpo) +
                ", sujo " + std::to_string(sujo));
}

TEST(reacao, sin_retrato_el_globo_SIGUE_avisando) {
  // O retrato vive no cartao. Um aparelho sem SD —— ou um bicho cuja
  // cara nao carregou —— nao pode ficar sem aviso nenhum: o icone de
  // sempre e o caminho de reserva, e tem que continuar pintando.
  arranca();
  if (pet.awaitingStarter()) pet.chooseStarter(1);
  while (pet.isEgg()) pet.eggTap();
  pet.sleeping = false;
  pet.fullness = 90; pet.joy = 90; pet.energy = 90;
  pet.hygiene = 5;

  porMon.unload();                 // como se nao houvesse cartao
  avanza(400);
  // O balao e branco com borda preta; sem ele a zona de cima fica com o
  // fundo. Conta-se o branco do balao, que o fundo de casa nao tem.
  // 0xFFFF e o UI_WHITE do species.h, escrito aqui como numero
  // porque este arquivo nao inclui esse cabecalho.
  int conBalao = pixelesDeColor(0xFFFF, 120, 60, 350, 240);
  CHECK_MSG(conBalao > 200,
            "sem retrato o balao tem que continuar aparecendo: " +
                std::to_string(conBalao));
  pet.hygiene = 90;
}

TEST(pokedex, la_ficha_dice_QUIEN_dibujo_el_retrato) {
  // A licenca do SpriteCollab e CC BY-NC: nao comercial E com credito.
  // O credito e por especie, entao vive aqui. Se esta linha sumir, a
  // gente esta usando a arte fora da licenca.
  arranca();
  if (pet.awaitingStarter()) pet.chooseStarter(1);
  while (pet.isEgg()) pet.eggTap();
  // A TELA LIMPA antes de abrir a ficha. O render() desenha o combate
  // antes da galeria, entao com uma batalha aberta de outro teste a
  // ficha nunca chega a ser pintada —— o despejo saiu com uma tela de
  // combate e eu quase fui procurar o defeito no layout.
  bt.open = false;
  cardOpen = false;
  determUntil = 0;
  gestosOn = false;
  mockTouchSet(0, 0, false);
  mockTouchTick();
  avanza(200);

  pet.registerSpecies(25, false);
  openDexEntry(25);
  avanza(400);
  // A faixa do credito, logo acima do "voltar". Conta-se tudo que nao
  // seja a cor dominante da faixa: se ha texto, ha pixeis diferentes.
  const uint16_t *fb = gfx->getFramebuffer();
  int w = gfx->width();
  int fundo = fb[384 * w + 70];        // a pele da Pokedex, na borda
  volcaPantalla("ficha");
  int texto = 0;
  // 376..392: o credito desceu para a faixa entre o fim da caixa de
  // texto (374) e o "voltar" (408). Antes ele caia DENTRO da caixa.
  for (int y = 376; y <= 392; y++)
    for (int x = 90; x <= 376; x++)
      if (fb[y * w + x] != fundo) texto++;
  // DEVOLVER o estado global. So zerar o galleryDetail nao basta:
  // openDexEntry tambem liga o galleryOpen, e com ele ligado a
  // tela de casa nunca mais era desenhada —— o teste da reacao,
  // que vem depois, media uma tela que nao era a dele e dava 0.
  galleryDetail = 0;
  galleryOpen = false;
  CHECK_MSG(texto > 40,
            "nao ha credito na ficha: " + std::to_string(texto) + " pixeis");
}

extern SdAnim animMove;

// A ANIMACAO DO ANIL tocando de verdade num combate: voce usa o golpe e
// depois o rival usa o mesmo. Cada quadro de 50 ms vai para
// _claude_scratch/prev_anim_<GOLPE>_<p|f>_<NN>.ppm, que e o que vira o
// GIF para o dono olhar. E mede-se o EFEITO: a tela do quadro do meio
// tem de ser outra que a de antes do golpe (as particulas estao la).
static void tocaAnimacao(const char *golpe) {
  arranca();
  if (pet.awaitingStarter()) pet.chooseStarter(1);
  while (pet.isEgg()) pet.eggTap();
  pet.sleeping = false;
  pet.energy = 100;
  gestosOn = false;
  galleryOpen = false;
  galleryDetail = 0;
  cardOpen = false;
  determUntil = 0;

  uint16_t mv = 0;
  for (uint16_t m = 0; m < MOVE_COUNT; m++)
    if (!strcmp(MOVE_TBL[m].name, golpe)) mv = m;
  CHECK_MSG(mv, std::string("premissa: o golpe existe: ") + golpe);
  if (!mv) return;

  battleStart(143, 40);              // um Snorlax: aguenta e e Normal
  for (int i = 0; i < 400 && bt.phase != BP_CHOOSE; i++) avanza(16);
  CHECK_MSG(bt.phase == BP_CHOOSE, "premissa: o combate tem que abrir");
  bt.bg = -1;
  for (uint8_t s = 0; s < 2; s++) {
    Combatant &c = bt.b.side[s];
    c.moves[0] = mv;
    c.usesLeft[0] = 30;
    c.nMoves = 1;
    c.hp = c.hpMax = 999;
    // Vinculo cheio: um bicho de nivel alto deixado por outro teste
    // DESOBEDECIA ("won't listen!") e o golpe nem era usado.
    c.bond = 255;
    // O jogador age PRIMEIRO: com o Snorlax rival na frente, um golpe que
    // faz recuar (o Triple Hit, 100%) deixava o seu sem turno e a
    // animacao dele nunca tocava.
    c.stage[SG_SPE] = (s == BT_PLAYER) ? 6 : -6;
    // Golpe que ERRA nao anima (como no Anil): sem isto o Play Rough (90%)
    // do rival dependia da ordem dos sorteios dos testes de antes.
    c.stage[SG_ACC] = 6;
    // E ninguem recua: com 100% de recuo o SEGUNDO a agir ficava sem
    // turno, e a animacao dele nao tocava nunca.
    for (uint16_t h = 1; h < ABIL_COUNT; h++)
      if (abilAt(h).fx == AB_NO_FLINCH) { c.ability = h; break; }
  }
  avanza(40);
  static uint16_t antes[466 * 466];
  const int W = gfx->width(), H = gfx->height();
  memcpy(antes, gfx->getFramebuffer(), (size_t)W * H * 2);

  bt.sel = 0;
  toca(233, (BT_CARD_Y0_T + BT_CARD_Y1_T) / 2);
  int ultimo[2] = { -1, -1 }, quadros[2] = { 0, 0 }, mudou[2] = { 0, 0 };
  std::string frases;
  for (int i = 0; i < 3000 && bt.open; i++) {
    avanza(10, 10);
    if (bt.msg[0] && frases.find(bt.msg) == std::string::npos)
      frases += std::string(" | ") + bt.msg;
    if (!bt.anim || bt.phase != BP_ATTACK) {
      if (bt.phase == BP_CHOOSE && quadros[0] && quadros[1]) break;
      continue;
    }
    uint8_t lado = (bt.acting == BT_FOE) ? 1 : 0;
    int q = (int)((millis() - bt.fxAt) / 50);
    if (q == ultimo[lado] || q >= animMove.nq) continue;
    ultimo[lado] = q;
    quadros[lado]++;
    char nome[64];
    snprintf(nome, sizeof(nome), "anim_%s_%c_%02d", golpe, lado ? 'f' : 'p', q);
    volcaPantalla(nome);
    if (q == animMove.nq / 2) {
      const uint16_t *fb = gfx->getFramebuffer();
      for (int k = 0; k < W * H; k++) if (fb[k] != antes[k]) mudou[lado]++;
    }
  }
  CHECK_MSG(quadros[0] > 0, std::string(golpe) + ": a sua animacao nao tocou;" + frases);
  CHECK_MSG(quadros[1] > 0, std::string(golpe) + ": a do rival nao tocou");
  CHECK_MSG(mudou[0] > 500 && mudou[1] > 500,
            std::string(golpe) + ": o quadro do meio quase nao muda a tela (" +
                std::to_string(mudou[0]) + " / " + std::to_string(mudou[1]) + ")");
  while (bt.open) { bt.phase = BP_END; bt.until = millis(); avanza(32); }
}

void drawBadge(uint8_t reg, uint8_t i, int cx, int cy, uint8_t esc);

TEST(insignia, cada_regiao_tem_as_SUAS) {
  // "as insignias de johto estao iguais as de kanto". Desenham-se as 9
  // regioes x 8 numa folha (o despejo e para olhar) e exige-se que cada
  // regiao saia DIFERENTE de Kanto na mesma casa.
  arranca();
  gfx->fillScreen(0xFFFF);
  const uint16_t *fb = gfx->getFramebuffer();
  const int W = gfx->width();
  auto marca = [&](int cx, int cy) {
    uint32_t h = 2166136261u;
    for (int y = cy - 24; y < cy + 24; y++)
      for (int x = cx - 24; x < cx + 24; x++) h = (h ^ fb[y * W + x]) * 16777619u;
    return h;
  };
  uint32_t m[REGION_COUNT][8];
  for (uint8_t r = 0; r < REGION_COUNT; r++)
    for (uint8_t i = 0; i < 8; i++) {
      int cx = 33 + i * 50, cy = 30 + r * 50;
      drawBadge(r, i, cx, cy, 1);
      m[r][i] = marca(cx, cy);
    }
  volcaPantalla("insignias");
  for (uint8_t r = 1; r < REGION_COUNT; r++) {
    int iguais = 0;
    for (uint8_t i = 0; i < 8; i++) if (m[r][i] == m[0][i]) iguais++;
    CHECK_MSG(iguais == 0, std::string("a regiao ") + std::to_string(r) + " tem " +
                               std::to_string(iguais) + " insignias iguais as de Kanto");
  }
}

TEST(insignia, vencer_o_lider_mostra_a_tela_e_conta_UMA_vez) {
  arranca();
  if (pet.awaitingStarter()) pet.chooseStarter(1);
  while (pet.isEgg()) pet.eggTap();
  pet.sleeping = false;
  pet.energy = 100;
  gestosOn = false;
  galleryOpen = false;
  galleryDetail = 0;
  cardOpen = false;
  determUntil = 0;
  const uint8_t antes = 2;
  pet.badges = antes;
  pet.badgesIn[pet.region] = antes;
  pet.gymTrainers = GYM_TRAINERS;

  battleStart(16, 10, -1, false, -1);
  for (int i = 0; i < 400 && bt.phase != BP_CHOOSE; i++) avanza(16);
  CHECK_MSG(bt.phase == BP_CHOOSE, "premissa: o combate tem que abrir");
  // Forca-se o fim de um combate de LIDER ganho: o que se testa e a saida.
  bt.gym = antes;
  bt.rematch = false;
  bt.b.winner = BT_PLAYER;
  bt.msg[0] = 0;
  bt.phase = BP_WIN;
  avanza(100);
  battleTap(233, 233);
  CHECK_MSG(bt.phase == BP_BADGE, "tocar a vitoria de um lider tem que ir para a insignia");
  CHECK_EQ((int)pet.badges, antes + 1);
  avanza(200);
  volcaPantalla("tela_insignia");
  battleTap(233, 233);
  for (int i = 0; i < 200 && bt.open; i++) avanza(16);
  CHECK_MSG(!bt.open, "o combate tem que fechar depois da insignia");
  CHECK_MSG(pet.badges == antes + 1,
            "a insignia contou " + std::to_string(pet.badges - antes) + " vezes");
}

extern SdClima animClima;

TEST(anim, o_DROUGHT_do_rival_traz_o_sol_do_Anil) {
  // "status provindos de habilidades (drought, steam veil...) que dao
  // sunny day estao sem animacoes". No Anil a habilidade chama
  // pbStartWeather, que mostra a animacao DO CLIMA (Common:Sun), e o plugin
  // Continuous Weather Animations a deixa em laco. Antes tocava-se aqui o
  // golpe Sunny Day, que nao e o que o Anil faz.
  arranca();
  if (pet.awaitingStarter()) pet.chooseStarter(1);
  while (pet.isEgg()) pet.eggTap();
  pet.sleeping = false;
  pet.energy = 100;
  gestosOn = false;
  galleryOpen = false;
  galleryDetail = 0;
  cardOpen = false;
  determUntil = 0;
  uint16_t drought = 0, sunny = 0;
  for (uint16_t i = 1; i < ABIL_COUNT; i++)
    if (!strcmp(abilAt(i).name, "Drought")) drought = i;
  for (uint16_t m = 1; m < MOVE_COUNT; m++)
    if (!strcmp(MOVE_TBL[m].name, "SUNNYDAY")) sunny = m;
  CHECK_MSG(drought && sunny, "premissa: Drought e SUNNYDAY existem");

  battleStart(324, 20);               // Torkoal
  // Forca-se a entrada com Drought (a habilidade do rival e sorteada).
  bt.b.side[BT_FOE].ability = drought;
  bt.b.startAbil[BT_PLAYER] = 0;
  bt.b.startAbil[BT_FOE] = drought;
  bt.b.weather = WE_NONE;
  bt.introAbil = 0;
  bt.phase = BP_INTRO;
  bt.until = millis();
  bt.bg = -1;
  avanza(20);
  static uint16_t antes[466 * 466];
  const int W = gfx->width(), H = gfx->height();
  memcpy(antes, gfx->getFramebuffer(), (size_t)W * H * 2);
  // o que o motor faz ao entrar com Drought
  bt.b.weather = WE_SUN;
  bt.b.weatherTurns = 5;
  bool golpe = false, clima = false;
  int mudou = 0;
  for (int i = 0; i < 60; i++) {      // 1,2 s: o sol chega ao auge aos 0,5
    avanza(20, 20);
    if (bt.anim && animMove.move == sunny) golpe = true;
    if (animClima.clima == WE_SUN) clima = true;
    if (i == 35) {
      const uint16_t *fb = gfx->getFramebuffer();
      for (int k = 0; k < W * H; k++) if (fb[k] != antes[k]) mudou++;
      volcaPantalla("anim_drought");
    }
  }
  CHECK_MSG(clima, "o Drought nao trouxe a animacao do sol (/anim/w_1.bin)");
  CHECK_MSG(!golpe, "o Drought tocou o GOLPE Sunny Day: no Anil e a animacao do clima");
  CHECK_MSG(mudou > 20000, "o sol quase nao muda a tela: " + std::to_string(mudou));
  while (bt.open) { bt.phase = BP_END; bt.until = millis(); avanza(32); }
}

TEST(anim, FLAMETHROWER) { tocaAnimacao("FLAMETHROWER"); }
TEST(anim, EXTREMESPEED) { tocaAnimacao("EXTREMESPEED"); }
TEST(anim, THUNDERBOLT) { tocaAnimacao("THUNDERBOLT"); }
TEST(anim, SEISMICTOSS) { tocaAnimacao("SEISMICTOSS"); }
TEST(anim, TACKLE) { tocaAnimacao("TACKLE"); }
TEST(anim, ATRAPAMOSCAS) { tocaAnimacao("ATRAPAMOSCAS"); }
TEST(anim, DESLIZAMIENTO) { tocaAnimacao("DESLIZAMIENTO"); }
TEST(anim, ESCALOFRIO) { tocaAnimacao("ESCALOFRIO"); }
TEST(anim, IVYCUDGEL) { tocaAnimacao("IVYCUDGEL"); }
TEST(anim, TRIPLEHIT) { tocaAnimacao("TRIPLEHIT"); }
TEST(anim, PLAYROUGH) { tocaAnimacao("PLAYROUGH"); }
TEST(anim, SURF) { tocaAnimacao("SURF"); }

// Um combate Snorlax x Snorlax, pronto para escolher, com estes golpes
// nos dois lados. O jogador age primeiro e ninguem recua.
static uint16_t golpeAnim(const char *n) {
  for (uint16_t m = 1; m < MOVE_COUNT; m++)
    if (!strcmp(MOVE_TBL[m].name, n)) return m;
  return 0;
}

static Pet petAntesAnim;

static bool combateDe(const uint16_t *mv, uint8_t n) {
  arranca();
  if (pet.awaitingStarter()) pet.chooseStarter(1);
  while (pet.isEgg()) pet.eggTap();
  petAntesAnim = pet;          // fechaCombate devolve (o Explosion desmaia)
  pet.sleeping = false;
  pet.energy = 100;
  gestosOn = false;
  galleryOpen = false;
  galleryDetail = 0;
  cardOpen = false;
  determUntil = 0;
  battleStart(143, 40);
  for (int i = 0; i < 400 && bt.phase != BP_CHOOSE; i++) avanza(16);
  if (bt.phase != BP_CHOOSE) return false;
  bt.bg = -1;
  for (uint8_t s = 0; s < 2; s++) {
    Combatant &c = bt.b.side[s];
    for (uint8_t i = 0; i < n; i++) { c.moves[i] = mv[i]; c.usesLeft[i] = 30; }
    c.nMoves = n;
    c.hp = c.hpMax = 999;
    c.bond = 255;
    c.stage[SG_SPE] = (s == BT_PLAYER) ? 6 : -6;
    for (uint16_t h = 1; h < ABIL_COUNT; h++)
      if (abilAt(h).fx == AB_NO_FLINCH) { c.ability = h; break; }
  }
  avanza(40);
  return true;
}

static void fechaCombate() {
  while (bt.open) { bt.phase = BP_END; bt.until = millis(); avanza(32); }
  pet = petAntesAnim;
}

// Pixeis da caixa do RIVAL (325, chao 178) diferentes de `ref`.
static int difRival(const uint16_t *ref) {
  const uint16_t *fb = gfx->getFramebuffer();
  const int W = gfx->width();
  int n = 0;
  for (int y = 40; y <= 182; y++)
    for (int x = 250; x <= 400; x++)
      if (fb[y * W + x] != ref[y * W + x]) n++;
  return n;
}

TEST(anim, golpe_em_si_mesmo_nao_mexe_no_rival) {
  // "outros golpes que o pokemon usa em si mesmo". No Anil o alvo de um
  // golpe User e o PROPRIO usuario: a cel 1 da animacao (o bicho de quem
  // recebe) e o mesmo sprite que a cel 0, e o rival nem entra no tocador.
  // Aqui a cel 1 ia sempre para o rival: Iron Defense o deixava cinza de
  // metal, Shell Smash o fazia piscar. Mede-se o rival na tela durante a
  // animacao, contra o tanto que ele ja muda sozinho parado (o respirar).
  const char *golpes[] = { "IRONDEFENSE", "SHELLSMASH" };
  for (const char *g : golpes) {
    uint16_t mv[1] = { golpeAnim(g) };
    CHECK_MSG(mv[0], std::string("premissa: existe ") + g);
    if (!mv[0] || !combateDe(mv, 1)) { CHECK_MSG(false, "premissa: combate"); return; }
    static uint16_t ref[466 * 466];
    const int W = gfx->width(), H = gfx->height();
    memcpy(ref, gfx->getFramebuffer(), (size_t)W * H * 2);
    int parado = 0;                      // o quanto ele muda sozinho
    // ~4 s: o respirar do rival tem ciclo longo; com 1 s o pico dele
    // podia cair fora da janela e a animacao (varios segundos) o pegava.
    for (int i = 0; i < 250; i++) {
      avanza(16);
      int d = difRival(ref);
      if (d > parado) parado = d;
    }
    bt.sel = 0;
    toca(233, (BT_CARD_Y0_T + BT_CARD_Y1_T) / 2);
    int pior = 0, quadros = 0, ultimo = -1;
    for (int i = 0; i < 3000 && bt.open; i++) {
      avanza(10, 10);
      if (!bt.anim || bt.acting != BT_PLAYER || bt.phase != BP_ATTACK) {
        if (quadros && bt.acting != BT_PLAYER) break;
        continue;
      }
      int q = (int)((millis() - bt.fxAt) / 50);
      if (q == ultimo || q >= animMove.nq) continue;
      ultimo = q;
      quadros++;
      int d = difRival(ref);
      if (d > pior) { pior = d; }
      if (q == 6) {
        char nome[64];
        snprintf(nome, sizeof(nome), "anim_proprio_%s", g);
        volcaPantalla(nome);
      }
    }
    CHECK_MSG(quadros > 10, std::string(g) + ": a animacao nao tocou");
    CHECK_MSG(pior <= parado + 300,
              std::string(g) + ": o rival mudou " + std::to_string(pior) +
                  " pixeis durante a animacao (parado muda " + std::to_string(parado) + ")");
    fechaCombate();
  }
}

TEST(anim, a_proxima_animacao_nao_toca_durante_a_frase) {
  // "explosion, o adversario esta ficando branco antes de explodir". A
  // animacao do golpe seguinte e lida DURANTE a frase "X usou Y!", e
  // entrava no lugar da anterior com o relogio dela: o FIM da nova tocava
  // antes da hora. No Explosion o fim e a tela branca do Anil.
  //
  // Turno 1 os dois usam TACKLE; turno 2 o jogador usa EXPLOSION. Mede-se
  // o branco puro na tela enquanto a frase do Explosion esta la e a
  // animacao dele ainda nao comecou —— e, de premissa, que o branco
  // aparece SIM depois, quando ela toca, senao a medida nao veria nada.
  uint16_t mv[2] = { golpeAnim("TACKLE"), golpeAnim("EXPLOSION") };
  CHECK_MSG(mv[0] && mv[1], "premissa: TACKLE e EXPLOSION existem");
  if (!mv[0] || !mv[1] || !combateDe(mv, 2)) { CHECK_MSG(false, "premissa: combate"); return; }
  // o rival so usa TACKLE
  bt.b.side[BT_FOE].nMoves = 1;
  const int W = gfx->width(), H = gfx->height();
  auto branco = [&]() {
    const uint16_t *fb = gfx->getFramebuffer();
    int n = 0;
    for (int k = 0; k < W * H; k++) if (fb[k] == 0xFFFF) n++;
    return n;
  };
  bt.sel = 0;
  toca(233, (BT_CARD_Y0_T + BT_CARD_Y1_T) / 2);
  for (int i = 0; i < 3000 && bt.phase != BP_CHOOSE; i++) avanza(10, 10);
  CHECK_MSG(bt.phase == BP_CHOOSE, "premissa: o turno 1 acaba");
  // O relogio velho (o do TACKLE do rival) tem de cair no trecho branco
  // da Explosion (quadros 17 a 44) quando a nova for lida: espera-se ate
  // ~1 s depois do comeco dela antes de escolher.
  while ((int32_t)(millis() - (bt.fxAt + 900)) < 0) avanza(10, 10);
  int antesBranco = branco();
  bt.sel = 1;
  toca(233, (BT_CARD_Y0_T + BT_CARD_Y1_T) / 2);
  int naFrase = 0, depois = 0;
  bool viuFrase = false;
  for (int i = 0; i < 3000 && bt.open; i++) {
    avanza(10, 10);
    bool frase = strstr(bt.msg, "EXPLOSION") != nullptr;
    bool tocando = bt.anim && bt.phase == BP_ATTACK && animMove.move == mv[1];
    if (frase && !tocando) {
      viuFrase = true;
      int b = branco();
      if (b > naFrase) { naFrase = b; if (b > W * H / 4) volcaPantalla("anim_frase_branca"); }
    }
    if (tocando) {
      int b = branco();
      if (b > depois) depois = b;
      if ((int)((millis() - bt.fxAt) / 50) > 30) break;
    }
  }
  CHECK_MSG(viuFrase, "premissa: a frase do EXPLOSION apareceu");
  CHECK_MSG(depois > W * H / 2,
            "premissa: a animacao do Explosion pinta a tela de branco (" +
                std::to_string(depois) + ")");
  CHECK_MSG(naFrase < antesBranco + W * H / 20,
            "a tela ficou branca DURANTE a frase: " + std::to_string(naFrase) +
                " pixeis (antes " + std::to_string(antesBranco) + ")");
  fechaCombate();
}

// Pixeis diferentes de `ref` numa caixa.
static int difCaixa(const uint16_t *ref, int x0, int y0, int x1, int y1) {
  const uint16_t *fb = gfx->getFramebuffer();
  const int W = gfx->width();
  int n = 0;
  for (int y = y0; y <= y1; y++)
    for (int x = x0; x <= x1; x++)
      if (fb[y * W + x] != ref[y * W + x]) n++;
  return n;
}

TEST(anim, cada_impacto_toca_a_SUA_animacao) {
  // Leva 63c. No Anil o impacto N de um golpe de varios golpes toca a
  // animacao "X hit N" (pbProcessMoveHit passa hitNum = i e a animacao e
  // a do golpe + hitNum). Aqui todos repetiam a do primeiro. Quase todo
  // "hit 2" do Anil e copia do 1 (o do Double Kick e); o do Double Slap
  // NAO: a mao vem do outro lado. Compara-se a tela no MESMO quadro da
  // animacao nos dois impactos.
  uint16_t mv[1] = { golpeAnim("DOUBLESLAP") };
  CHECK_MSG(mv[0], "premissa: DOUBLESLAP");
  if (!mv[0] || !combateDe(mv, 1)) { CHECK_MSG(false, "premissa: combate"); return; }
  bt.b.side[BT_PLAYER].stage[SG_ACC] = 6;   // 85% de precisao: sem sorte
  const int W = gfx->width(), H = gfx->height();
  // O que MUDA de um quadro para o seguinte, na area do rival, e a mao
  // (o respirar e um seno de 3 s: em 50 ms quase nao mexe). Soma-se o x
  // de cada pixel que mudou nos quadros da animacao de cada tapa: o x
  // medio diz de que lado a mao veio.
  static uint16_t ant[466 * 466];
  long sx[2] = { 0, 0 }, n[2] = { 0, 0 };
  int ultimo = -1, ultimoN = -1;
  bt.sel = 0;
  toca(233, (BT_CARD_Y0_T + BT_CARD_Y1_T) / 2);
  for (int i = 0; i < 3000 && bt.open; i++) {
    avanza(10, 10);
    if (!bt.anim || bt.acting != BT_PLAYER || bt.phase != BP_ATTACK) continue;
    if (bt.golpeN > 1) break;
    int q = (int)((millis() - bt.fxAt) / 50);
    if (q >= animMove.nq || (q == ultimo && bt.golpeN == ultimoN)) continue;
    const uint16_t *fb = gfx->getFramebuffer();
    if (bt.golpeN == ultimoN && q == ultimo + 1) {
      for (int y = 20; y <= 200; y++)
        for (int x = 250; x <= 465; x++)
          if (fb[y * W + x] != ant[y * W + x]) { sx[bt.golpeN] += x; n[bt.golpeN]++; }
    }
    memcpy(ant, fb, (size_t)W * H * 2);
    ultimo = q;
    ultimoN = bt.golpeN;
    if (bt.golpeN == 1 && q == 3) volcaPantalla("anim_impacto2_DOUBLESLAP");
  }
  CHECK_MSG(n[0] > 200 && n[1] > 200, "premissa: os dois tapas animaram (" +
                                         std::to_string(n[0]) + ", " + std::to_string(n[1]) + ")");
  if (n[0] && n[1]) {
    long x1 = sx[0] / n[0], x2 = sx[1] / n[1];
    // o primeiro vem pela direita do rival, o segundo pela esquerda
    CHECK_MSG(x1 - x2 > 15, "o segundo tapa veio do mesmo lado do primeiro (x medio " +
                                std::to_string(x1) + " e " + std::to_string(x2) + ")");
  }
  fechaCombate();
}

TEST(anim, a_carga_toca_e_o_bicho_ja_sai_sumido) {
  // Leva 63c. No Anil o turno de carga do Fly toca "Fly charging" (a bola
  // escura sobe; TwoTurnMove passa hitNum = 1), e ai o bicho ja esta fora
  // de cena. Aqui nao tocava nada: ele ficava parado na frase e so depois
  // subia com a nossa transicao. Mede-se a caixa do jogador: (1) durante
  // a frase ela muda (a animacao); (2) nos 400 ms depois de a animacao
  // PARAR, cada quadro ja e igual ao de quando ele sumiu de vez —— nada de
  // ele voltar ao chao e subir outra vez.
  uint16_t mv[1] = { golpeAnim("FLY") };
  CHECK_MSG(mv[0], "premissa: FLY");
  if (!mv[0] || !combateDe(mv, 1)) { CHECK_MSG(false, "premissa: combate"); return; }
  bt.b.side[BT_FOE].moves[0] = golpeAnim("TACKLE");
  const int W = gfx->width(), H = gfx->height();
  const int X0 = 20, Y0 = 120, X1 = 240, Y1 = 320;   // o bicho do jogador
  const size_t TAM = (size_t)W * H;
  static uint16_t ref[466 * 466], sumido[466 * 466];
  static std::vector<uint16_t> depois;
  depois.clear();
  memcpy(ref, gfx->getFramebuffer(), TAM * 2);
  int parado = 0;
  for (int i = 0; i < 60; i++) {
    avanza(16);
    int d = difCaixa(ref, X0, Y0, X1, Y1);
    if (d > parado) parado = d;
  }
  bt.sel = 0;
  toca(233, (BT_CARD_Y0_T + BT_CARD_Y1_T) / 2);
  int naCarga = 0;
  bool viuCarga = false, tocou = false;
  uint32_t parou = 0;
  for (int i = 0; i < 4000 && bt.open; i++) {
    avanza(10, 10);
    bool carga = bt.phase == BP_MSG && bt.extraStep == 9 && bt.acting == BT_PLAYER;
    bool animando = carga && bt.anim && (int32_t)(millis() - bt.animFim) < 0;
    if (carga) {
      viuCarga = true;
      int d = difCaixa(ref, X0, Y0, X1, Y1);
      if (d > naCarga) naCarga = d;
    }
    if (animando) tocou = true;
    if (tocou && !animando && !parou) parou = millis();
    // A partir de 60 ms: o primeiro repintado depois do fim ainda e o
    // ultimo quadro da animacao (o vento do Fly). O bicho voltando ao
    // chao e subindo de novo dura 260 ms e e o bicho inteiro (~9000 px).
    if (parou && millis() - parou >= 60 && (int32_t)(millis() - (parou + 400)) < 0) {
      const uint16_t *fb = gfx->getFramebuffer();
      depois.insert(depois.end(), fb, fb + TAM);
    }
    if (parou && (int32_t)(millis() - (parou + 1500)) >= 0) {
      memcpy(sumido, gfx->getFramebuffer(), TAM * 2);
      break;
    }
  }
  CHECK_MSG(viuCarga, "premissa: o turno de carga do FLY");
  CHECK_MSG(tocou, "a animacao de carga do FLY nao tocou");
  CHECK_MSG(naCarga > parado + 2000,
            "a caixa do jogador nao mudou na carga: " + std::to_string(naCarga) +
                " (parado " + std::to_string(parado) + ")");
  CHECK_MSG(parou && depois.size() >= TAM * 5, "premissa: quadros depois da animacao");
  int pior = 0;
  for (size_t o = 0; o + TAM <= depois.size(); o += TAM) {
    int d = 0;
    for (int y = Y0; y <= Y1; y++)
      for (int x = X0; x <= X1; x++)
        if (depois[o + y * W + x] != sumido[y * W + x]) d++;
    if (d > pior) pior = d;
  }
  // o bicho tem uns 4000 pixeis; o fundo da caixa e parado
  CHECK_MSG(pior <= 400,
            "depois da animacao de carga o bicho reapareceu: " + std::to_string(pior) +
                " pixeis diferentes de quando ele ja sumiu");
  depois.clear();
  depois.shrink_to_fit();
  fechaCombate();
}

TEST(anim, o_solar_beam_ao_sol_diz_a_carga_e_so_depois_pega) {
  // Leva 64. No Anil (pbQuickChargingMove) o Rayo Solar ao sol diz "took
  // in sunlight!", toca a animacao de CARGA (hitNum 1) e so entao a do
  // golpe —— tudo no mesmo turno. Aqui saia direto o golpe, sem frase.
  // E a barra do rival tem de esperar o golpe: durante a frase ela ainda
  // nao caiu.
  uint16_t mv[1] = { golpeAnim("SOLARBEAM") };
  CHECK_MSG(mv[0], "premissa: SOLARBEAM");
  if (!mv[0] || !combateDe(mv, 1)) { CHECK_MSG(false, "premissa: combate"); return; }
  bt.b.side[BT_FOE].moves[0] = golpeAnim("TACKLE");
  bt.b.side[BT_PLAYER].stage[SG_ACC] = 6;
  // O nosso e nivel 1: com 999 de PS o golpe nao mexia um pixel da barra.
  bt.b.side[BT_PLAYER].stage[SG_SPA] = 6;
  bt.b.side[BT_FOE].hp = bt.b.side[BT_FOE].hpMax = 40;
  bt.b.side[BT_FOE].item = IT_NONE;
  bt.b.weather = WE_SUN;
  bt.b.weatherTurns = 5;
  avanza(40);
  const int W = gfx->width();
  (void)W;
  // So o miolo da barra de PS do rival: em volta o sol anima o fundo.
  const int BY0 = 68 + 18, BY1 = 68 + 25, BX0 = 95, BX1 = 255;
  static uint16_t ref[466 * 466];
  memcpy(ref, gfx->getFramebuffer(), (size_t)W * gfx->height() * 2);
  const uint16_t vidaAntes = bt.b.side[BT_FOE].hp;
  bt.sel = 0;
  toca(233, (BT_CARD_Y0_T + BT_CARD_Y1_T) / 2);
  bool viuFrase = false, tocouCarga = false, depoisGolpe = false;
  bool golpeDepoisDaCarga = false;
  int barraNaFrase = 0, barraDepois = 0;
  for (int i = 0; i < 3000 && bt.open; i++) {
    avanza(10, 10);
    if (bt.acting != BT_PLAYER) continue;
    if (bt.phase == BP_CARGA) {
      if (strstr(bt.msg, "took in sunlight")) viuFrase = true;
      if (bt.anim && animMove.move == mv[0] && animMove.hit == 1) tocouCarga = true;
      int d = difCaixa(ref, BX0, BY0, BX1, BY1);
      if (d > barraNaFrase) barraNaFrase = d;
      CHECK(bt.b.side[BT_FOE].hp + bt.hpPend >= vidaAntes);
    }
    if ((bt.phase == BP_ATTACK || bt.phase == BP_HIT) && tocouCarga) {
      golpeDepoisDaCarga = true;
      if (bt.anim && animMove.hit != 0) CHECK_MSG(false, "o golpe saiu com a animacao da carga");
    }
    if (golpeDepoisDaCarga && bt.phase == BP_MSG) {
      depoisGolpe = true;
      int d = difCaixa(ref, BX0, BY0, BX1, BY1);
      if (d > barraDepois) barraDepois = d;
    }
    if (depoisGolpe && bt.phase == BP_CHOOSE) break;
  }
  CHECK_MSG(viuFrase, "ao sol o Rayo Solar nao disse \"took in sunlight!\"");
  CHECK_MSG(tocouCarga, "a animacao de carga (SOLARBEAM.1) nao tocou");
  CHECK_MSG(golpeDepoisDaCarga, "o golpe nao veio depois da carga");
  CHECK_MSG(vidaAntes > bt.b.side[BT_FOE].hp, "premissa: o golpe tirou vida");
  CHECK_MSG(barraDepois > 8, "premissa: a barra muda quando o golpe cai: " +
                                 std::to_string(barraDepois));
  CHECK_MSG(barraNaFrase <= 2, "a barra caiu durante a frase da carga: " +
                                   std::to_string(barraNaFrase) + " pixeis");
  fechaCombate();
}

TEST(anim, a_magnitude_diz_o_nivel_antes_do_golpe) {
  // Leva 65. No Anil (pbOnStartUse) a Magnitude diz "Magnitude N!" logo
  // depois do anuncio e antes do golpe. Aqui o numero nunca aparecia.
  uint16_t mv[1] = { golpeAnim("MAGNITUDE") };
  CHECK_MSG(mv[0], "premissa: MAGNITUDE");
  if (!mv[0] || !combateDe(mv, 1)) { CHECK_MSG(false, "premissa: combate"); return; }
  bt.b.side[BT_FOE].moves[0] = golpeAnim("TACKLE");
  bt.b.side[BT_PLAYER].stage[SG_ACC] = 6;
  bt.b.side[BT_PLAYER].stage[SG_ATK] = 6;
  bt.b.side[BT_FOE].hp = bt.b.side[BT_FOE].hpMax = 8;   // uns PS que o Bulbasaur nv1 derruba
  bt.b.side[BT_FOE].item = IT_NONE;
  avanza(40);
  const int W = gfx->width();
  const int BY0 = 68 + 18, BY1 = 68 + 25;            // a barra de PS do rival
  static uint16_t ref[466 * 466];
  memcpy(ref, gfx->getFramebuffer(), (size_t)W * gfx->height() * 2);
  const uint16_t vidaAntes = bt.b.side[BT_FOE].hp;
  bt.sel = 0;
  toca(233, (BT_CARD_Y0_T + BT_CARD_Y1_T) / 2);
  bool viuAnuncio = false, viuNivel = false, nivelAntesDoGolpe = false;
  bool golpe = false, nivelCerto = true;
  int barraNaFrase = 0, barraDepois = 0;
  unsigned nivel = 0;
  uint16_t vidaMin = vidaAntes;
  char frase[48] = "";
  for (int i = 0; i < 3000 && bt.open; i++) {
    avanza(10, 10);
    if (bt.acting != BT_PLAYER) continue;
    if (strstr(bt.msg, "used MAGNITUDE")) viuAnuncio = true;
    if (!strncmp(bt.msg, "Magnitude ", 10)) {
      viuNivel = true;
      if (!golpe) nivelAntesDoGolpe = viuAnuncio;
      nivel = bt.last.magnitud;
      snprintf(frase, sizeof(frase), "Magnitude %u!", nivel);
      if (strcmp(bt.msg, frase)) nivelCerto = false;
      int d = difCaixa(ref, 95, BY0, 255, BY1);
      if (d > barraNaFrase) barraNaFrase = d;
    }
    if (bt.phase == BP_ATTACK || bt.phase == BP_HIT) golpe = true;
    if (bt.b.side[BT_FOE].hp < vidaMin) vidaMin = bt.b.side[BT_FOE].hp;
    if (golpe && (bt.phase == BP_MSG || bt.phase == BP_FAINT)) {
      int d = difCaixa(ref, 95, BY0, 255, BY1);
      if (d > barraDepois) barraDepois = d;
    }
    if (golpe && bt.phase == BP_CHOOSE) break;
  }
  CHECK_MSG(viuAnuncio, "premissa: \"used MAGNITUDE\"");
  CHECK_MSG(viuNivel, "a Magnitude nao disse o nivel");
  CHECK_MSG(nivelAntesDoGolpe, "o nivel tem de vir depois do anuncio e antes do golpe");
  CHECK_MSG(nivelCerto, std::string("a frase nao e o nivel sorteado: ") + bt.msg);
  CHECK_MSG(nivel >= 4 && nivel <= 10, "premissa: nivel 4..10");
  CHECK_MSG(vidaAntes > vidaMin, "premissa: o golpe tirou vida");
  CHECK_MSG(barraDepois > 8, "premissa: a barra muda quando o golpe cai: " +
                                 std::to_string(barraDepois));
  CHECK_MSG(barraNaFrase <= 2, "a barra caiu durante a frase do nivel: " +
                                   std::to_string(barraNaFrase) + " pixeis");
  fechaCombate();
}

TEST(sombra, cae_bajo_los_PIES_y_no_bajo_el_ala) {
  arranca();
  if (pet.awaitingStarter()) pet.chooseStarter(1);
  while (pet.isEgg()) pet.eggTap();
  pet.sleeping = false;
  pet.energy = 100;

  // PIDGEOTTO: apoya en las columnas 72..78 de 84 y su tinta va de la 0
  // a la 80 —— el ala abierta se llevaba la mancha a la 40. Es el caso
  // que mejor separa las dos cuentas.
  battleStart(17, 30);
  CHECK_MSG(bt.open, "el combate no se abrio");
  for (int i = 0; i < 400 && bt.phase != BP_CHOOSE; i++) avanza(16);
  CHECK_MSG(bt.phase == BP_CHOOSE, "no se llego a elegir golpe");
  // EL FONDO, FIJO. Cada combate sortea una foto segun el bioma, y la
  // sombra se busca mirando que columnas se oscurecieron respecto del
  // suelo de al lado: sobre una foto con piedras y hierba, esa cuenta
  // recoge el decorado y se va. -1 es el cielo DIBUJADO, que es liso.
  //
  // Corriendo solo este teste calaba una foto que colaba; en la bateria
  // entera salia otra y el teste se caia. Sin fijarlo, lo que se mide
  // depende de que test corrio antes.
  bt.bg = -1;
  avanza(200);
  volcaPantalla("sombra_combate");

  CHECK_MSG(foePmd.loaded, "premisa: el sprite del rival tiene que cargar");
  const PmdAct &ac = foePmd.acts[PMD_IDLE];
  CHECK_MSG(ac.pieR >= ac.pieL, "premisa: hay fila de apoyo medida");
  // Que las dos cuentas DIFIERAN de verdad, o el teste no prueba nada:
  // en un bicho simetrico pasarian las dos.
  float centroTinta = (ac.footL + ac.footR) * 0.5f;
  CHECK_MSG(fabsf(ac.centroPie() - centroTinta) > 8.0f,
            "premisa: en este bicho el apoyo y el medio de la tinta "
            "tienen que estar lejos, y distan " +
                std::to_string(fabsf(ac.centroPie() - centroTinta)));

  // A SOMBRA E A SILHUETA DO BICHO, e mede-se POR DIFERENCA: a mesma
  // tela com e sem ela. O que mudou e a sombra e so ela —— nada de
  // adivinhar o que e chao e o que e sombra olhando uma foto.
  //
  // A elipse antiga ficava 3 px abaixo do pe e o teste a procurava ali;
  // a silhueta deita a partir do pe para cima e para a direita.
  static uint16_t semSombra[466 * 466];
  const int W = gfx->width(), H = gfx->height();
  CHECK(W * H <= 466 * 466);
  gSombra.desligada = true;
  renderBattle();   // SEM avancar o relogio: mesmo frame da animacao
  memcpy(semSombra, gfx->getFramebuffer(), (size_t)W * H * 2);
  gSombra.desligada = false;
  renderBattle();   // SEM avancar o relogio: mesmo frame da animacao
  const uint16_t *fb = gfx->getFramebuffer();
  // so a metade do rival, longe do bicho do jogador e dos botoes
  int baixo = -1, cima = 9999, clareou = 0, nSomb = 0;
  for (int y = 20; y < 200; y++)
    for (int x = 240; x < 450; x++) {
      uint16_t a = semSombra[y * W + x], b = fb[y * W + x];
      if (a == b) continue;
      int ga = (a >> 5) & 63, gb = (b >> 5) & 63;
      if (gb > ga) { clareou++; continue; }
      nSomb++;
      if (y > baixo) baixo = y;
      if (y < cima) cima = y;
    }
  CHECK_MSG(clareou == 0, std::to_string(clareou) + " pixeis CLAREARAM: "
            "sombra so escurece");
  CHECK_MSG(nSomb > 60, "nao ha sombra: " + std::to_string(nSomb) + " pixeis");
  if (baixo < 0) { while (bt.open) { bt.phase = BP_END; bt.until = millis(); avanza(32); } return; }
  // O centro da linha de BAIXO (onde a inclinacao e zero) cai no pe.
  auto centroDe = [&](int y0, int y1) {
    long sx = 0, n = 0;
    for (int y = y0; y <= y1; y++)
      for (int x = 240; x < 450; x++)
        if (semSombra[y * W + x] != fb[y * W + x]) { sx += x; n++; }
    return n ? (int)(sx / n) : -1;
  };
  int cen = centroDe(baixo - 2, baixo);
  int px100 = (int)(100.0f * (btAltoReal[BT_FOE] > 0 && ac.inkH()
                                  ? (float)btAltoReal[BT_FOE] / ac.inkH()
                                  : 1.0f));
  int pieEnPantalla =
      BT_FOE_CX + (int)((ac.centroPie() - ac.w * 0.5f) * px100 / 100);
  // O PROPRIO bicho tapa a sombra bem embaixo do pe, entao o que a
  // diferenca ve na linha de baixo e so a ponta descoberta: o centro
  // dela anda para o lado. O que se exige e que a faixa de baixo
  // ENCOSTE no pe —— uma sombra solta para a direita nao encosta.
  int xMin = 9999, xMax = -1;
  for (int y = baixo - 2; y <= baixo; y++)
    for (int x = 240; x < 450; x++)
      if (semSombra[y * W + x] != fb[y * W + x]) {
        if (x < xMin) xMin = x;
        if (x > xMax) xMax = x;
      }
  CHECK_MSG(pieEnPantalla >= xMin - 12 && pieEnPantalla <= xMax + 12,
            "a sombra tem que nascer nos pes: a base dela vai de x=" +
                std::to_string(xMin) + " a " + std::to_string(xMax) +
                " e o apoio esta em x=" + std::to_string(pieEnPantalla));
  // Deitada: alto ~38% do bicho. Uma copia em pe (achatar quebrado)
  // ou um risco (alto zero) caem aqui.
  int alto = baixo - cima + 1, bicho = btAltoReal[BT_FOE];
  CHECK_MSG(alto > bicho / 5 && alto < bicho * 3 / 5,
            "a sombra tem " + std::to_string(alto) + " px de alto para um bicho de " +
                std::to_string(bicho));
  // Inclinada: o topo dela fica a DIREITA do pe.
  int topo = centroDe(cima, cima + 2);
  CHECK_MSG(topo > cen + 5, "a sombra nao inclina: topo em x=" +
                               std::to_string(topo) + ", pe em x=" + std::to_string(cen));

  while (bt.open) { bt.phase = BP_END; bt.until = millis(); avanza(32); }
}

TEST(sombra, el_apoyo_se_mide_en_la_fila_que_toca) {
  // La otra mitad, barata y directa: el apoyo tiene que estar DENTRO
  // del cuerpo y ser mas estrecho o igual que el. Si alguien vuelve a
  // medirlo sobre todo el lienzo, esto cae.
  arranca();
  while (pet.isEgg()) pet.eggTap();
  battleStart(17, 30);
  for (int i = 0; i < 400 && bt.phase != BP_CHOOSE; i++) avanza(16);
  avanza(120);
  CHECK_MSG(foePmd.loaded, "premisa: sprite cargado");
  const PmdAct &ac = foePmd.acts[PMD_IDLE];
  CHECK_MSG(ac.pieL >= ac.footL && ac.pieR <= ac.footR,
            "el apoyo tiene que caer dentro del cuerpo");
  // ESTRICTAMENTE mas estrecho, y por un buen trozo. Un Pidgeotto apoya
  // en 7 columnas de las 81 que ocupa su tinta.
  //
  // Mi primera version pedia "<=" y se quedo VERDE con el fallo puesto:
  // al medir en la fila equivocada el codigo cae en su respaldo ("sin
  // fila de apoyo: el cuerpo") y entonces apoyo y cuerpo son iguales,
  // que cumple un "<=". El teste no distinguia medir bien de no medir.
  int anchoPie = ac.pieR - ac.pieL + 1;
  int anchoCuerpo = ac.footR - ac.footL + 1;
  CHECK_MSG(anchoPie * 3 < anchoCuerpo,
            "este bicho apoya en una parte PEQUENA de su ancho, y el "
            "apoyo medido son " + std::to_string(anchoPie) +
            " columnas de " + std::to_string(anchoCuerpo));
  while (bt.open) { bt.phase = BP_END; bt.until = millis(); avanza(32); }
}

// So para OLHAR: um bicho de costas na batalha (PREVIEW_COSTAS=dex).
TEST(preview, costas_na_batalha) {
  const char *e = getenv("PREVIEW_COSTAS");
  if (!e) return;
  arranca();
  if (pet.awaitingStarter()) pet.chooseStarter(1);
  while (pet.isEgg()) pet.eggTap();
  pet.sleeping = false;
  pet.energy = 100;
  gestosOn = false;
  galleryOpen = false;
  cardOpen = false;
  pet.speciesId = (int16_t)atoi(e);
  pet.form = 0;
  pet.hpLeft = pet.maxHp();
  battleStart(16, 10);
  for (int i = 0; i < 400 && bt.phase != BP_CHOOSE; i++) avanza(16);
  avanza(500);
  char nome[32];
  snprintf(nome, sizeof(nome), "costas_%s", e);
  volcaPantalla(nome);
  cierraElCombate();
}

extern int16_t btLadosCostas[2];
// "Sprite do shiftry ... muito grande a largura" e "se possivel ja valida
// os demais": os 18 que passavam da borda ou entravam na placa de PS
// (medidos com a conta do firmware) tem de caber entre x=4 e x=256, com
// folga antes da placa (x=268).
TEST(combate_ui, o_bicho_de_costas_cabe_entre_a_borda_e_a_placa) {
  const int16_t LARGOS[] = { 275, 581, 382, 715, 169, 894, 902, 321 };
  for (int16_t d : LARGOS) {
    arranca();
    cierraElCombate();
    if (pet.awaitingStarter()) pet.chooseStarter(1);
    while (pet.isEgg()) pet.eggTap();
    pet.sleeping = false;
    pet.energy = 100;
    gestosOn = false;
    galleryOpen = false;
    cardOpen = false;
    pet.speciesId = d;
    pet.form = 0;
    pet.hpLeft = pet.maxHp();
    btLadosCostas[0] = btLadosCostas[1] = 0;
    battleStart(16, 10);
    for (int i = 0; i < 400 && bt.phase != BP_CHOOSE; i++) avanza(16);
    avanza(100);
    if (getenv("MOSTRA_LADOS"))
      printf("  %d %s %d..%d alto %d pedido %d%c", d, dexName(d), btLadosCostas[0],
             btLadosCostas[1], btAltoDibujado[BT_PLAYER], btPedido[BT_PLAYER], 10);
    CHECK_MSG(btLadosCostas[1] > btLadosCostas[0],
              "premissa: o " + std::to_string(d) + " foi desenhado de costas");
    CHECK_MSG(btLadosCostas[0] >= 4 && btLadosCostas[1] <= 256,
              std::string(dexName(d)) + " de costas vai de x=" +
              std::to_string(btLadosCostas[0]) + " a x=" +
              std::to_string(btLadosCostas[1]));
    cierraElCombate();
  }
}

// So para OLHAR: a ficha do titular com a bola em que foi capturado.
TEST(preview, ficha_com_a_bola) {
  if (!getenv("PREVIEW_FICHA")) return;
  arranca();
  if (pet.awaitingStarter()) pet.chooseStarter(1);
  while (pet.isEgg()) pet.eggTap();
  gestosOn = false;
  galleryOpen = false;
  for (uint8_t i = 1; i < ITEM_COUNT; i++)
    if (!strcmp(ITEM_TBL[i].name, "Quick Ball")) pet.ballId = i;
  cardOpen = true;
  cardPage = 0;
  avanza(300);
  volcaPantalla("ficha_bola");
  cardOpen = false;
}

// "quando um pokemon meu e derrotado, e o seguinte vence a batalha, na
// tela de vitoria aparece o pokemon derrotado como vencedor". O vencedor
// era gravado so na PRIMEIRA queda de rival.
void btFoeDefeated();   // battle_ui.ino
TEST(combate_ui, a_vitoria_e_de_quem_terminou_o_combate) {
  arranca();
  cierraElCombate();
  if (pet.awaitingStarter()) pet.chooseStarter(1);
  while (pet.isEgg()) pet.eggTap();
  pet.sleeping = false;
  pet.energy = 100;
  gestosOn = false;
  galleryOpen = false;
  cardOpen = false;
  pet.hpLeft = pet.maxHp();
  uint8_t outro = pet.mainSlot == 0 ? 1 : 0;
  StoredMon m{};
  m.species = 25;                      // Pikachu
  m.exp = Pet::expForLevel(12);
  m.hpLeft = 30;
  m.moves[0] = MOVE_STRUGGLE;
  pet.team[outro] = m;
  battleStart(16, 5);
  for (int i = 0; i < 400 && bt.phase != BP_CHOOSE; i++) avanza(16);
  CHECK_MSG(bt.phase == BP_CHOOSE, "premissa: o combate abre");
  const int16_t primeiro = bt.b.side[BT_PLAYER].dex;
  btFoeDefeated();                     // o titular derruba o primeiro rival
  CHECK_EQ((int)bt.winDex, (int)primeiro);
  // ...cai, e o Pikachu entra e derruba o segundo
  bt.mySlot = outro;
  bt.b.side[BT_PLAYER].dex = 25;
  bt.b.side[BT_PLAYER].asDex = 25;
  btFoeDefeated();
  CHECK_MSG(bt.winDex == 25, "a vitoria ficou com o " +
                             std::string(dexName(bt.winDex)) + ", que tinha caido");
  CHECK_EQ((int)bt.lvlAntes, 12);
  pet.team[outro].species = 0;
  cierraElCombate();
}

// Excavar (e Voo, Mergulho, Raio Solar): no segundo turno o golpe sai
// SOZINHO. A tela parava no menu e obrigava a tocar o golpe de novo,
// quando no Anil esse turno nao se escolhe. Aqui so se toca UMA vez.
TEST(combate_ui, o_segundo_turno_do_excavar_sai_sozinho) {
  arranca();
  cierraElCombate();
  if (pet.awaitingStarter()) pet.chooseStarter(1);
  while (pet.isEgg()) pet.eggTap();
  pet.sleeping = false;
  pet.energy = 100;
  gestosOn = false;
  galleryOpen = false;
  cardOpen = false;
  pet.hpLeft = pet.maxHp();
  battleStart(19, 5);                  // Rattata: a terra pega nele
  for (int i = 0; i < 400 && bt.phase != BP_CHOOSE; i++) avanza(16);
  CHECK_MSG(bt.phase == BP_CHOOSE, "premissa: o combate abre");
  uint16_t cava = 0, nada = 0;
  for (uint16_t i = 0; i < MOVE_COUNT; i++) {
    if (!strcmp(MOVE_TBL[i].name, "DIG")) cava = i;
    if (!strcmp(MOVE_TBL[i].name, "SPLASH")) nada = i;
  }
  CHECK_MSG(cava && nada, "premissa: DIG e SPLASH existem");
  Combatant &yo = bt.b.side[BT_PLAYER];
  Combatant &ele = bt.b.side[BT_FOE];
  yo.moves[0] = cava; yo.nMoves = 1; yo.usesLeft[0] = 9;
  ele.moves[0] = nada; ele.nMoves = 1; ele.usesLeft[0] = 9;
  ele.hpMax = ele.hp = 999;
  bt.b.strictOrders = true;
  bt.sel = 0;
  btCommitTurn();                      // o UNICO toque
  bool carregou = false;
  for (int i = 0; i < 4000 && ele.hp == 999; i++) {
    avanza(16);
    if (yo.chargingMove == cava) carregou = true;
  }
  CHECK_MSG(carregou, "premissa: o Excavar foi para baixo da terra");
  CHECK_MSG(ele.hp < 999, "o Excavar ficou esperando outro toque no segundo turno");
  cierraElCombate();
}

// So para OLHAR: insignias de Hoenn (do cartao) e as fitas das conquistas.
TEST(preview, insignias_e_fitas) {
  if (!getenv("PREVIEW_FITAS")) return;
  arranca();
  if (pet.awaitingStarter()) pet.chooseStarter(1);
  while (pet.isEgg()) pet.eggTap();
  gestosOn = false;
  galleryOpen = false;
  uint8_t regAntes = pet.region, insAntes = pet.badges;
  uint32_t medAntes = pet.medals;
  pet.region = 2;
  pet.badges = 8;
  cardOpen = true;
  cardPage = 6;
  avanza(300);
  volcaPantalla("cartao_hoenn");
  pet.medals = 0x2A5u;
  cardPage = 7;
  avanza(300);
  volcaPantalla("fitas");
  pet.region = regAntes;
  pet.badges = insAntes;
  pet.medals = medAntes;
  cardOpen = false;
  cardPage = 0;
}

// A MEGA FICA BRANCA. No Anil o bicho vai a branco enquanto muda e volta
// do branco ja transformado (pbFlashInOut). Mede-se no ecra: o mesmo
// fotograma com e sem o fundido, contando pixeis brancos onde esta o bicho.
TEST(combate_ui, a_mega_fica_branca_e_volta) {
  arranca();
  cierraElCombate();
  if (pet.awaitingStarter()) pet.chooseStarter(1);
  while (pet.isEgg()) pet.eggTap();
  pet.sleeping = false;
  pet.energy = 100;
  gestosOn = false;
  galleryOpen = false;
  cardOpen = false;
  // A especie e fixa: um teste anterior deixa o pet como Charizard (e
  // outros como outra coisa), e o fotograma depende de quem esta.
  int16_t espAntes = pet.speciesId;
  pet.speciesId = 4;
  pet.form = 0;
  avanza(400);                             // carrega o sprite do Charmander
  pet.hpLeft = pet.maxHp();
  battleStart(19, 5);
  for (int i = 0; i < 400 && bt.phase != BP_CHOOSE; i++) avanza(16);
  CHECK_MSG(bt.phase == BP_CHOOSE, "premissa: o combate abre");
  // "quase branco": o tingido passa por RGB888 e volta a 565
  // A LUZ somada da zona do bicho, nao "pixeis brancos": conforme a hora
  // do relogio simulado a cena escurece por cima, e o branco puro do
  // sprite chega ao ecra cinzento. O que o fundido faz em qualquer hora e
  // CLAREAR o bicho.
  auto quaseBranco = []() {
    const uint16_t *fb = gfx->getFramebuffer();
    long n = 0;
    for (int y = 150; y <= 312; y++)
      for (int x = 0; x <= 300; x++) {
        uint16_t c = fb[y * gfx->width() + x];
        n += ((c >> 11) & 31) * 2 + ((c >> 5) & 63) + (c & 31) * 2;
      }
    return (int)(n / 100);
  };
  // O ecra repinta a cada 100 ms ou mais (a cadencia de loop): 220 ms
  // garantem pelo menos um fotograma. Na medida com branco, o reloj da
  // volta reinicia-se a cada passo para o fotograma pintado ser branco puro.
  bt.megaRevelaLado = 0xFF;
  avanza(220);
  int antes = quaseBranco();
  CHECK_MSG(btMegaBlanco(BT_PLAYER, millis()) == 0.0f, "premissa: sem mega, sem branco");
  bt.megaRevelaLado = BT_PLAYER;
  for (int i = 0; i < 14; i++) {
    bt.megaRevelaAt = millis() + 16;      // o passo seguinte e o instante 0
    avanza(16);
  }
  int branco = quaseBranco();
  // Medido, sozinho e na bateria inteira: sem o fundido a luz muda entre
  // -100 e +180 de um fotograma ao outro; com ele sobe +9400..+10700.
  CHECK_MSG(branco > antes + 5000,
            "o bicho nao ficou branco: luz " + std::to_string(antes) + " -> " +
            std::to_string(branco));
  // e passa: 400 ms depois ja nao ha branco
  CHECK_MSG(btMegaBlanco(BT_PLAYER, millis() + 450) == 0.0f, "o branco nao some");
  // durante a cena sobe: no comeco nada, no fim tudo
  bt.phase = BP_MEGA;
  bt.megaSide = BT_PLAYER;
  bt.megaAt = millis();
  CHECK(btMegaBlanco(BT_PLAYER, millis() + 50) == 0.0f);
  CHECK(btMegaBlanco(BT_PLAYER, millis() + BT_MEGA_MS - 50) == 1.0f);
  CHECK(btMegaBlanco(BT_FOE, millis() + BT_MEGA_MS - 50) == 0.0f);
  bt.phase = BP_CHOOSE;
  bt.megaRevelaLado = 0xFF;
  cierraElCombate();
  pet.speciesId = espAntes;                // devolve o estado global
}

// A EVOLUCAO SOA COMO NO ANIL (pbEvolution): o grito do de antes, a musica
// de evolucao enquanto muda, e no fim o grito do NOVO, para a musica e
// so entao a fanfarria. Antes soavam fanfarria e grito novo juntos, ao
// tocar, e o grito cortava a fanfarria.
TEST(evolucao, toca_a_musica_e_depois_a_fanfarria) {
  arranca();
  cierraElCombate();
  if (pet.awaitingStarter()) pet.chooseStarter(1);
  while (pet.isEgg()) pet.eggTap();
  pet.sleeping = false;
  pet.energy = 100;
  gestosOn = false;
  galleryOpen = false;
  cardOpen = false;
  // Charmander fixo: um teste anterior deixa o pet como Charizard, que
  // nao evolui.
  int16_t espAntes = pet.speciesId;
  pet.speciesId = 4;
  pet.form = 0;
  pet.exp = Pet::expForLevel(40);
  CHECK_MSG(pet.canEvolveNow(), "premissa: o Charmander pode evoluir a nivel 40");
  if (!pet.canEvolveNow()) { pet.speciesId = espAntes; return; }
  int16_t antes = pet.speciesId;
  mockAudioLogClear();
  choiceKind = 1;
  onTap(233, 230);                         // "evoluir"
  CHECK_MSG(pet.evolving(), "o toque nao comecou a evolucao");
  std::string a = mockAudioLog();
  CHECK_MSG(a.find("bgm:EVOLVE") != std::string::npos,
            "durante a evolucao nao toca a musica: " + a);
  CHECK_MSG(a.find("jin:EVOLVE") == std::string::npos,
            "a fanfarria saiu antes de acabar: " + a);
  avanza(EVOLVE_ANIM_MS + 600);
  a = mockAudioLog();
  size_t c0 = a.find("cry:#" + std::to_string(antes) + " ");
  size_t m = a.find("bgm:EVOLVE");
  size_t c1 = a.find("cry:#" + std::to_string(pet.speciesId) + " ");
  size_t st = a.find("bgm:stop", m == std::string::npos ? 0 : m);
  size_t j = a.find("jin:EVOLVE");
  CHECK_MSG(c0 < m && m < c1 && c1 < st && st < j && j != std::string::npos,
            "ordem errada: " + a);
  pet.speciesId = espAntes;                // devolve o estado global
}

// LA BOLSA EN COMBATE. Reportado: "quase 100% das vezes, a tela volta pra
// batalha" al intentar bajar por la lista. La flecha se tocaba en 66x26 px
// y cualquier toque que no cayera en algo cerraba la bolsa, tambien dentro
// del panel. Ahora hay una faja entera para pasar pagina, arrastrar mueve
// la lista y solo se cierra tocando FUERA del panel.
extern bool clockOpen;
extern bool manualQrOpen;   // leva 74
TEST(combate_ui, en_la_bolsa_se_pasa_pagina_sin_que_se_cierre) {
  combateConObjetos(0, 0);
  uint8_t guardada[ITEM_COUNT];
  for (uint16_t i = 0; i < ITEM_COUNT; i++) guardada[i] = pet.bag[i];
  for (uint16_t i = 1; i < ITEM_COUNT; i++) pet.bag[i] = 1;   // de todo

  onTap(BBAG_BTN_X_T, BBAG_BTN_Y_T);
  CHECK_MSG(bt.phase == BP_BAG, "el boton no abrio la bolsa");
  CHECK_EQ((int)bt.bagScroll, 0);

  // bajar: cualquier punto de la mitad DERECHA de la faja...
  onTap(380, 344);
  CHECK_EQ((int)bt.bagScroll, 3);
  // ...tambien pegado a la ultima fila, que es donde cae un dedo de verdad
  onTap(250, 321);
  CHECK_EQ((int)bt.bagScroll, 6);
  // y la mitad izquierda sube
  onTap(100, 360);
  CHECK_EQ((int)bt.bagScroll, 3);
  CHECK_MSG(bt.phase == BP_BAG, "pasar pagina no cierra");

  // un toque DENTRO del panel que no cae en nada: no hace nada
  onTap(233, 384);                             // debajo de la faja
  CHECK_MSG(bt.phase == BP_BAG, "tocar el pie del panel cerro la bolsa");
  onTap(64, 240);                              // el borde, al lado de una fila
  CHECK_MSG(bt.phase == BP_BAG, "tocar el borde del panel cerro la bolsa");
  CHECK_EQ((int)bt.bagScroll, 3);

  // arrastrar: el dedo hacia arriba baja por la lista, y vuelve
  onSwipeV(-1);
  CHECK_EQ((int)bt.bagScroll, 6);
  onSwipeV(+1);
  CHECK_EQ((int)bt.bagScroll, 3);
  CHECK_MSG(!cardOpen && !clockOpen && bt.phase == BP_BAG,
            "el gesto abrio otra cosa en vez de mover la lista");

  // hasta el fondo, y seguir tocando: ni se pasa ni se cierra
  uint8_t fondo = 0;
  for (int i = 0; i < 120; i++) { onTap(380, 344); fondo = bt.bagScroll; }
  onTap(380, 344);
  CHECK_EQ((int)bt.bagScroll, (int)fondo);
  CHECK_MSG(fondo > 6, "premisa: la lista era larga");
  CHECK_MSG(bt.phase == BP_BAG, "tocar 'bajar' en el fondo cerro la bolsa");

  // FUERA del panel si cierra
  onTap(233, 60);
  CHECK_MSG(bt.phase == BP_CHOOSE, "tocar fuera tiene que volver al combate");

  for (uint16_t i = 0; i < ITEM_COUNT; i++) pet.bag[i] = guardada[i];
  cierraElCombate();
}

// =====================================================================
// EL MAPA DE TOQUES
//
// La bolsa del combate se cerraba "quase 100% das vezes" al intentar bajar
// por la lista: el blanco media 66x26 px y CUALQUIER toque que no acertase
// a nada devolvia al combate. Los tests no lo veian porque tocaban en el
// pixel exacto. Esto es lo contrario: se toca la pantalla ENTERA en rejilla
// y de cada punto se apunta que paso:
//
//   NADA    no cambio nada ni sono nada
//   ACCION  algo cambio o sono (un blanco)
//   CIERRA  la pantalla se fue atras sin hacer nada mas
//
// Con eso se puede medir lo que importa: cuanto mide cada blanco, y si
// hay puntos que CIERRAN pegados a un blanco (fallar por un pelo te echa).
//
// Antes de cada toque se restauran `pet` y `bt` enteros y se vuelve a
// montar la pantalla, asi que un toque no ve lo que hizo el anterior.
// Van AL FINAL del fichero a proposito: los toques arrancan combates y
// gastan objetos, y aunque todo se restaura, lo que no vive en `pet` ni
// en `bt` (sprites cargados, musica) queda como quede.
uint32_t mockSonidos();
const char *mockSdRootGet();

extern bool kbOpen, mapOpen, rematchOpen, quizOpen, gymOpen, abilShown;
extern uint8_t cardScroll, bagPocket, bagShown, movesSlot, mapView;
extern uint8_t rematchScroll, achPage;
extern uint16_t moveShown;
extern uint32_t feedMenuUntil, confirmUntil, apagarUntil, bankSoltarAte;
extern uint32_t bathUntil;
extern const char *moveDescPtr;
extern const char *bagDescPtr;
extern int8_t dexPag;
extern bool trocaOpen, trocaEscolhe, trocaBanco;
extern uint8_t trocaRola, trocaSlot;
extern uint32_t trocaTravaAte;
extern uint8_t darItemId;
extern uint32_t trocaFeitoEm;
extern Troca troca;
void trocaFecha();
// a batalha entre placas (leva 57)
extern Duelo duelo;
extern uint8_t duPasso, duFormato, duItens, duNEsc;
extern uint8_t duOrdem[DUELO_MAX];
extern uint8_t duViz[6];
// o combate pela rede (leva 59)
extern uint8_t pvpPonto, pvpPrecisa, pvpTem, pvpEu;
extern uint16_t pvpPontos;
extern bool pvpAcabou, pvpRede, pvpCortado;
extern uint32_t pvpDesistirAte, pvpAbertoEm;
extern uint8_t duResultado;
extern void (*pvpAoAbrir)();
uint32_t pvpRestaMs();
void radioFantasmaCala(bool s);
void radioFantasmaHashErrado(bool s);

enum { TQ_FUERA = 0, TQ_NADA = 1, TQ_CIERRA = 2, TQ_ACCION = 3 };

static uint32_t fnvT(uint32_t h, const void *p, size_t n) {
  const uint8_t *b = (const uint8_t *)p;
  for (size_t i = 0; i < n; i++) { h ^= b[i]; h *= 16777619u; }
  return h;
}

// Todo lo que un toque puede mover, menos los sonidos (van aparte: cerrar
// la revancha suena y cerrar la ficha no, y las dos son "cerrar").
static uint32_t huellaDelEstado() {
  uint32_t h = 2166136261u;
#define HV(v) h = fnvT(h, &(v), sizeof(v))
  HV(bt); HV(pet);
  HV(cardOpen); HV(cardPage); HV(cardScroll); HV(bagPocket); HV(bagShown);
  HV(moveShown); HV(movesSlot); HV(bankView); HV(bankSoltar); HV(abilShown);
  HV(kbOpen); HV(galleryOpen); HV(galleryPage); HV(galleryDetail);
  HV(galleryVer); HV(mapOpen); HV(mapView); HV(rematchOpen);
  HV(rematchScroll); HV(achPage); HV(clockOpen); HV(quizOpen); HV(gymOpen);
  HV(choiceKind); HV(confirmUntil); HV(feedMenuUntil); HV(apagarUntil);
  HV(bathUntil); HV(moveDescPtr); HV(bagDescPtr); HV(dexPag);
  HV(determUntil);
  // a troca: sem isto, abrir a troca (fecha a ficha) era igual a "fechar"
  // e o mapa nao via o botao TROCAR —— errar por um fio fechava a ficha.
  HV(trocaOpen); HV(troca.estado); HV(troca.tenhoMeu); HV(troca.slotMeu);
  HV(trocaEscolhe); HV(trocaBanco); HV(trocaRola); HV(darItemId);
  // a batalha: trocar ou batalhar?, as regras, quem foi escolhido
  HV(duPasso); HV(duFormato); HV(duItens); HV(duNEsc); HV(duOrdem);
  HV(duelo.estado); HV(duelo.tenhoMeu); HV(duelo.motivo);
  // o combate entre placas: a jogada posta e a bandeira armada
  HV(pvpTem); HV(pvpDesistirAte);
#undef HV
  return h;
}

extern bool avOpen;
static void todoCerradoT() {
  if (trocaOpen) trocaFecha();
  // As sessoes saem do random(), que os testes semeiam igual: sem isto a
  // troca de um teste repete o numero da anterior, e o `feitas` dela
  // (que na placa responde a pacotes atrasados) recusa a sessao "ja feita".
  troca = Troca();
  duelo = Duelo();
  duPasso = 0; duNEsc = 0;
  memset(duOrdem, 0, sizeof(duOrdem));
  duFormato = DF_3V3; duItens = DI_PROIBIDO;
  duResultado = 0xFF;
  pvpAoAbrir = nullptr;
  trocaEscolhe = false; trocaBanco = false; trocaRola = 0;
  trocaTravaAte = 0;
  darItemId = 0;
  // Os guioes tocam por coordenada, e agora um deles acerta a mochila.
  avOpen = false;
  pet.avDesde = 0;
  cardOpen = false; kbOpen = false; galleryOpen = false; galleryDetail = 0;
  galleryVer = 0; mapOpen = false; rematchOpen = false; clockOpen = false;
  manualQrOpen = false;
  quizOpen = false; gymOpen = false; choiceKind = 0; confirmUntil = 0;
  feedMenuUntil = 0; apagarUntil = 0; abilShown = false; bankView = false;
  bankSoltar = -1; bagShown = 0; moveShown = 0; movesSlot = 0xFF;
  cardScroll = 0; cardPage = 0; rematchScroll = 0; achPage = 0;
  bathUntil = 0; determUntil = 0; bagPocket = 0;
}

static void recogeTodoT() {
  cierraElCombate();
  todoCerradoT();
}

struct PantallaT {
  std::string nombre;
  bool combate;                    // se monta sobre el combate abierto
  std::function<void()> monta;     // antes de CADA toque, tras restaurar
  std::function<void()> cierra;    // lo que es "cerrar sin mas" (o nada)
};

struct MapaT {
  int paso = 4, n = 0;
  // Toques que arrancaron de un estado DISTINTO al del primero: algo que el
  // barrido no restaura. Con fugas el mapa miente (un "cerrar" deja de
  // parecerse al cerrar de referencia y se apunta como accion), asi que la
  // guarda exige cero.
  int fugas = 0;
  std::vector<uint8_t> clase;
  std::vector<uint32_t> firma;     // solo en ACCION: que accion fue
  uint8_t en(int i, int j) const {
    if (i < 0 || j < 0 || i >= n || j >= n) return TQ_FUERA;
    return clase[(size_t)j * n + i];
  }
};

static MapaT barreT(const PantallaT &p, int paso, const Pet &pet0,
                    const BattleUi &bt0) {
  MapaT m;
  m.paso = paso;
  m.n = (466 + paso - 1) / paso;
  m.clase.assign((size_t)m.n * m.n, TQ_FUERA);
  m.firma.assign((size_t)m.n * m.n, 0);
  // Lo que vive fuera de `pet` y `bt` y un toque puede mover sin cerrar
  // nada: por donde va el texto de cada ficha.
  const char *md0 = moveDescPtr, *bd0 = bagDescPtr;
  auto pon = [&]() {
    pet = pet0;
    bt = bt0;
    todoCerradoT();
    moveDescPtr = md0;
    bagDescPtr = bd0;
    dexPag = 0;
    p.monta();
    randomSeed(7);
  };
  uint32_t base = 0;
  bool primero = true;
  // como queda el estado tras "cerrar sin mas"
  uint32_t cerrada = 0;
  bool hayCierre = (bool)p.cierra;
  if (hayCierre) { pon(); p.cierra(); cerrada = huellaDelEstado(); }
  for (int j = 0; j < m.n; j++) {
    for (int i = 0; i < m.n; i++) {
      int x = i * paso + paso / 2, y = j * paso + paso / 2;
      int dx = x - 233, dy = y - 233;
      if (dx * dx + dy * dy > 229 * 229) continue;   // fuera del cristal
      pon();
      uint32_t antes = huellaDelEstado();
      if (primero) { base = antes; primero = false; }
      else if (antes != base) m.fugas++;
      uint32_t sonAntes = mockSonidos();
      onTap((int16_t)x, (int16_t)y);
      uint32_t despues = huellaDelEstado();
      bool sono = mockSonidos() != sonAntes;
      size_t k = (size_t)j * m.n + i;
      if (hayCierre && despues == cerrada) m.clase[k] = TQ_CIERRA;
      else if (despues != antes || sono) {
        m.clase[k] = TQ_ACCION;
        m.firma[k] = despues ^ (sono ? 0x9e3779b9u : 0u);
      } else m.clase[k] = TQ_NADA;
    }
  }
  pon();
  return m;
}

// Cuantos puntos que CIERRAN estan a `foso` px o menos (en cuadrado) de un
// punto que hace algo. Devuelve tambien el peor, para poder decir donde.
static int trampasT(const MapaT &m, int foso, int *px = nullptr, int *py = nullptr) {
  int r = foso / m.paso, n = 0;
  for (int j = 0; j < m.n; j++)
    for (int i = 0; i < m.n; i++) {
      if (m.en(i, j) != TQ_CIERRA) continue;
      bool cerca = false;
      for (int b = -r; b <= r && !cerca; b++)
        for (int a = -r; a <= r && !cerca; a++)
          if (m.en(i + a, j + b) == TQ_ACCION) cerca = true;
      if (!cerca) continue;
      if (!n) {
        if (px) *px = i * m.paso + m.paso / 2;
        if (py) *py = j * m.paso + m.paso / 2;
      }
      n++;
    }
  return n;
}

static void vuelcaMapaT(const PantallaT &p, const MapaT &m) {
  char ruta[200];
  snprintf(ruta, sizeof(ruta), "../_claude_scratch/prev_toq_%s.txt", p.nombre.c_str());
  FILE *f = fopen(ruta, "wb");
  if (!f) return;
  // numeros: 0 fuera, 1 nada, 2 cierra, 3.. cada accion distinta
  std::vector<uint32_t> vistas;
  fprintf(f, "%d %d%c", m.paso, m.n, 10);
  for (int j = 0; j < m.n; j++) {
    for (int i = 0; i < m.n; i++) {
      size_t k = (size_t)j * m.n + i;
      int v = m.clase[k];
      if (v == TQ_ACCION) {
        size_t q = 0;
        while (q < vistas.size() && vistas[q] != m.firma[k]) q++;
        if (q == vistas.size()) vistas.push_back(m.firma[k]);
        v = 3 + (int)q;
      }
      fprintf(f, "%d ", v);
    }
    fprintf(f, "%c", 10);
  }
  fclose(f);
}

// El bicho, el equipo, el banco y la mochila con los que se miran TODAS
// las pantallas: listas largas (para que rueden) y de todo en la bolsa.
static void preparaElMundoT() {
  arranca();
  if (pet.awaitingStarter()) pet.chooseStarter(1);
  while (pet.isEgg()) pet.eggTap();
  cierraElCombate();
  todoCerradoT();
  pet.sleeping = false;
  pet.energy = 100;
  pet.fullness = 60;
  pet.exp = Pet::expForLevel(30);
  pet.hpLeft = pet.maxHp();
  pet.heldItem = 0;
  pet.badges = 0;
  for (uint16_t i = 1; i < ITEM_COUNT; i++) pet.bag[i] = 2;
  for (uint8_t i = 0; i < TEAM_SLOTS; i++)
    if (i != pet.mainSlot) pet.team[i] = StoredMon{};
  for (uint8_t i = 0; i < BANK_SLOTS; i++) pet.bank[i] = StoredMon{};
  // a caixa aberta pode ter ficado em outra (um teste anterior abriu a 6):
  // o banco montado aqui e o da caixa 0, e os testes contam com isso
  pet.boxOpen = 0;
  const int16_t esp[3] = { 7, 25, 16 };
  for (uint8_t k = 0; k < 3; k++) {
    StoredMon s{};
    s.species = (pet.speciesId == esp[k]) ? 4 : esp[k];
    s.exp = Pet::expForLevel(12);
    s.geneAtk = s.geneDef = s.geneSpe = 100;
    s.moves[0] = MOVE_STRUGGLE;
    s.ppLeft[0] = 5;
    s.hpLeft = 20;
    pet.addToTeam(s);
  }
  for (uint8_t k = 0; k < 8 && k < BANK_SLOTS; k++) {
    StoredMon s{};
    s.species = (int16_t)(10 + k);
    s.exp = Pet::expForLevel(5);
    s.moves[0] = MOVE_STRUGGLE;
    s.hpLeft = 10;
    pet.bank[k] = s;
  }
}

static uint8_t primerObjetoT(uint8_t bolsillo, uint8_t uso) {
  for (uint16_t i = 1; i < ITEM_COUNT; i++)
    if (ITEM_TBL[i].pocket == bolsillo && (uso == 0xFF || ITEM_TBL[i].use == uso))
      return (uint8_t)i;
  return 0;
}

void trocaAbre();
// A troca ja pareada, sem radio de verdade: a lista de escolha (do time ou
// do banco) ou o resumo com o ESCOLHER.
static void trocaParaEscolhaT(bool lista, bool banco) {
  trocaAbre();
  troca.estado = TE_ESCOLHA;
  troca.tenhoMeu = false;
  troca.tenhoDele = false;
  troca.slotMeu = 0;
  trocaEscolhe = lista;
  trocaBanco = banco;
  trocaRola = 0;
}

// A batalha sem radio: a pergunta, as regras, ou ja pareada escolhendo o
// time (com `escolhido`, o time ja foi mandado e espera o do outro).
static void dueloParaT(uint8_t passo, bool pareado, bool escolhido) {
  trocaAbre();
  troca.estado = TE_PROCURA;
  static const uint8_t ELA[6] = { 0x02, 0, 0, 0, 0, 0x02 };
  memcpy(duViz, ELA, 6);
  duPasso = passo;
  if (!pareado) return;
  duPasso = 0;
  troca.estado = TE_DUELO;
  duelo.comeca(troca.eu, ELA, 77, true, duRegras(DF_1V1, DI_PROIBIDO));
  duNEsc = 0;
  if (escolhido) {
    uint8_t r[1][TROCA_MON];
    memcpy(r[0], &pet.team[pet.mainSlot], TROCA_MON);
    duelo.meuTime(r, 1);
  }
}

static uint8_t itemPorNomeT(const char *n) {
  for (uint16_t i = 1; i < ITEM_COUNT; i++)
    if (!strcmp(ITEM_TBL[i].name, n)) return (uint8_t)i;
  return 0;
}

static std::vector<PantallaT> pantallasT() {
  std::vector<PantallaT> v;
  auto ficha = [&v](const char *nombre, uint8_t pag, std::function<void()> mas) {
    v.push_back({ nombre, false,
                  [pag, mas]() { cardOpen = true; cardPage = pag; if (mas) mas(); },
                  []() { cardOpen = false; } });
  };
  // ---- el combate
  v.push_back({ "bat_elegir", true, []() {}, nullptr });
  v.push_back({ "bat_bolsa", true,
                []() { bt.phase = BP_BAG; bt.bagPocket = 0; bt.bagScroll = 3; bt.itemPick = 0; },
                []() { bt.phase = BP_CHOOSE; } });
  v.push_back({ "bat_bolas", true,
                []() { bt.phase = BP_BAG; bt.bagPocket = 1; bt.bagScroll = 0; bt.itemPick = 0; },
                []() { bt.phase = BP_CHOOSE; } });
  v.push_back({ "bat_equipo", true,
                []() { bt.phase = BP_PARTY; bt.itemPick = 0; bt.mustSwitch = false; },
                []() { bt.phase = BP_CHOOSE; } });
  v.push_back({ "bat_equipo_objeto", true,
                []() { bt.phase = BP_PARTY; bt.itemPick = primerObjetoT(BP_BATTLE, IU_POKEMON); },
                []() { bt.itemPick = 0; bt.phase = BP_BAG; } });
  // ---- la ficha
  ficha("ficha_0_perfil", 0, nullptr);
  ficha("ficha_1_stats", 1, nullptr);
  ficha("ficha_2_golpes", 2, nullptr);
  ficha("ficha_2_golpes_desc", 2, []() { moveShown = pet.ppMove[0]; });
  ficha("ficha_3_bolsa", 3, []() { bagPocket = BP_BATTLE; });
  ficha("ficha_3_bolsa_objeto", 3,
        []() { bagPocket = BP_HELD; bagShown = primerObjetoT(BP_HELD, 0xFF); });
  ficha("ficha_4_equipo", 4, nullptr);
  // leva 51: o "dar X a quem?" e a ficha de um objeto de troca (USAR/DAR)
  // (fechar daqui tambem esquece o objeto: e o "fechar" de referencia)
  v.push_back({ "ficha_4_dar", false,
                []() { cardOpen = true; cardPage = 4; darItemId = itemPorNomeT("Metal Coat"); },
                []() { cardOpen = false; darItemId = 0; } });
  ficha("ficha_3_bolsa_troca", 3, []() {
    bagPocket = BP_EVO;
    bagShown = itemPorNomeT("Metal Coat");
  });
  ficha("ficha_4_banco", 4, []() { bankView = true; });
  // La pregunta de soltar no se "cierra": cualquier toque que no sea el SI
  // desiste, y que un casi-SI desista es lo seguro. Sin `cierra`.
  v.push_back({ "ficha_4_banco_soltar", false,
                []() { cardOpen = true; cardPage = 4; bankView = true; bankSoltar = 0;
                       bankSoltarAte = millis() + 10000; },
                nullptr });
  ficha("ficha_5_gimnasio", 5, nullptr);
  // con los tres de la casa vencidos el boton del lider esta VIVO...
  ficha("ficha_5_gimnasio_listo", 5, []() { pet.gymTrainers = GYM_TRAINERS; });
  // ...y coronado, el mismo boton viaja
  ficha("ficha_5_gimnasio_coronado", 5, []() { pet.badges = pet.gymCount(); });
  ficha("ficha_6_entrenador", 6, nullptr);
  ficha("ficha_7_medallas", 7, nullptr);
  ficha("ficha_8_progreso", 8, nullptr);
  // ---- lo demas
  v.push_back({ "dex_rejilla", false,
                []() { galleryOpen = true; galleryPage = 0; galleryDetail = 0; galleryDirty = true; },
                []() { galleryOpen = false; } });
  v.push_back({ "dex_ficha", false,
                []() { galleryOpen = true; galleryPage = 0; galleryDetail = pet.speciesId; galleryDirty = true; },
                []() { galleryDetail = 0; } });
  v.push_back({ "comida", false,
                []() { feedMenuUntil = millis() + 6000; },
                []() { feedMenuUntil = 0; } });
  v.push_back({ "revancha", false,
                []() { pet.badges = pet.gymCount(); rematchOpen = true; rematchScroll = 1; },
                []() { rematchOpen = false; } });
  v.push_back({ "mapa", false,
                []() { mapOpen = true; mapView = pet.region; },
                []() { mapOpen = false; } });
  v.push_back({ "mapa_coronado", false,
                []() { pet.badges = pet.gymCount(); mapOpen = true; mapView = pet.region; },
                []() { mapOpen = false; } });
  // ---- a troca (leva 49). Nenhuma das duas fecha por toque (so o VOLTAR
  // e o CANCELAR saem), por isso sem `cierra`: o mapa ve os blancos.
  v.push_back({ "troca_lista", false, []() { trocaParaEscolhaT(true, false); }, nullptr });
  v.push_back({ "troca_lista_banco", false, []() { trocaParaEscolhaT(true, true); }, nullptr });
  v.push_back({ "troca_resumo", false, []() { trocaParaEscolhaT(false, false); }, nullptr });
  // ---- a batalha entre placas (leva 57): tambem so saem pelos botoes
  v.push_back({ "duelo_pergunta", false, []() { dueloParaT(1, false, false); }, nullptr });
  v.push_back({ "duelo_regras", false, []() { dueloParaT(2, false, false); }, nullptr });
  v.push_back({ "duelo_escolha", false, []() { dueloParaT(0, true, false); }, nullptr });
  v.push_back({ "duelo_espera", false, []() { dueloParaT(0, true, true); }, nullptr });
  v.push_back({ "casa", false, []() {}, nullptr });
  return v;
}

// Corre `usa` con cada pantalla y su mapa. Monta el mundo, abre un combate
// para las pantallas de combate, y al acabar lo deja todo cerrado.
static void conCadaPantallaT(const std::string &solo, int paso, bool foto,
    const std::function<void(const PantallaT &, const MapaT &)> &usa) {
  preparaElMundoT();
  std::string sdAntes = mockSdRootGet();
  Pet petCasa = pet;
  BattleUi btCasa = bt;
  // el combate: un salvaje que no cae
  battleStart(16, 10, -1, false, -1);
  for (int i = 0; i < 400 && bt.phase != BP_CHOOSE; i++) avanza(16);
  bt.b.side[BT_FOE].hpMax = 9999;
  bt.b.side[BT_FOE].hp = 9999;
  Pet petBat = pet;
  BattleUi btBat = bt;
  // El banco y las cajas ESCRIBEN en la tarjeta: se van a una carpeta de
  // usar y tirar, que la de verdad no es para que la toque un barrido.
  mockSdRoot("../_claude_scratch/sd_barrido");
  for (const PantallaT &p : pantallasT()) {
    if (!solo.empty() && p.nombre.find(solo) == std::string::npos) continue;
    const Pet &p0 = p.combate ? petBat : petCasa;
    const BattleUi &b0 = p.combate ? btBat : btCasa;
    if (foto) {
      pet = p0; bt = b0; todoCerradoT(); p.monta();
      mockSdRoot(sdAntes.c_str());
      avanza(160);
      volcaPantalla(("toq_" + p.nombre).c_str());
      mockSdRoot("../_claude_scratch/sd_barrido");
    }
    MapaT m = barreT(p, paso, p0, b0);
    usa(p, m);
  }
  mockSdRoot(sdAntes.c_str());
  pet = petBat;
  bt = btBat;
  todoCerradoT();
  cierraElCombate();
  avanza(200);
}

// No afirma nada: deja en _claude_scratch/ la foto y el mapa de cada
// pantalla, para tools... _claude_scratch/mapa_toques.py, que los pinta.
#include <cstdlib>
//
//   TAMA_MAPA=1 ./build/tamapoke_uitests mapa
//   python _claude_scratch/mapa_toques.py <carpeta de salida>
TEST(mapa, vuelca_los_toques_de_cada_pantalla) {
  if (!getenv("TAMA_MAPA")) return;      // solo cuando se pide
  // TAMA_MAPA=2: sin foto (y sin el repintado que la foto trae consigo)
  bool foto = std::string(getenv("TAMA_MAPA")) != "2";
  conCadaPantallaT("", foto ? 3 : 4, foto, [](const PantallaT &p, const MapaT &m) {
    vuelcaMapaT(p, m);
  });
}

// LA GUARDA. En ninguna pantalla puede haber un punto que CIERRE a 16 px
// o menos de un punto que hace algo: fallar un blanco por un pelo no te
// puede echar de la pantalla. Es lo que pasaba en la bolsa del combate
// ("quase 100% das vezes, a tela volta pra batalha") y, visto el mapa,
// en casi todas las listas.
TEST(toques, fallar_por_un_pelo_no_cierra_ninguna_pantalla) {
  std::string mal;
  int vistas = 0;
  conCadaPantallaT("", 4, false, [&](const PantallaT &p, const MapaT &m) {
    vistas++;
    int nCierra = 0, nAccion = 0;
    for (uint8_t c : m.clase) {
      if (c == TQ_CIERRA) nCierra++;
      if (c == TQ_ACCION) nAccion++;
    }
    // Las premisas, o el teste pasaria vacio: se puede volver tocando, y
    // hay algo que tocar (menos en las dos paginas que solo se leen).
    if (p.cierra && !nCierra) mal += p.nombre + ": no hay donde tocar para volver; ";
    bool soloLeer = p.nombre == "ficha_6_entrenador" || p.nombre == "ficha_8_progreso";
    if (!nAccion && !soloLeer) mal += p.nombre + ": el barrido no encontro ni un blanco; ";
    if (m.fugas)
      mal += p.nombre + ": el barrido no restaura todo (" + std::to_string(m.fugas) +
             " toques salieron de otro estado); ";
    int x = 0, y = 0;
    int t = trampasT(m, 16, &x, &y);
    if (t)
      mal += p.nombre + ": " + std::to_string(t) + " puntos cierran pegados a un blanco, "
             "el primero en (" + std::to_string(x) + "," + std::to_string(y) + "); ";
  });
  CHECK_MSG(vistas >= 27, "premisa: no se barrieron todas las pantallas");
  CHECK_MSG(mal.empty(), mal);
}

// La pagina de golpes dibuja CINCO filas y aceptaba toques en seis: la
// sexta caia sobre el letrero "tap a move to see what it does", y tocarlo
// ponia o quitaba un golpe que no estaba a la vista.
TEST(toques, tocar_el_letrero_de_los_golpes_no_toca_un_golpe_que_no_se_ve) {
  preparaElMundoT();
  pet.exp = Pet::expForLevel(60);
  cardOpen = true;
  cardPage = 2;
  moveShown = 0;
  cardScroll = 0;
  avanza(100);
  // premisa: hay mas golpes que filas (si no, no hay sexto que tocar)
  onTap(414, 290);
  CHECK_MSG(cardScroll == 1, "premisa: la lista no rueda, no hay golpe escondido");
  cardScroll = 0;
  uint16_t antes[4];
  for (uint8_t i = 0; i < 4; i++) antes[i] = pet.ppMove[i];
  onTap(200, 332);                              // el letrero
  // y ENCIMA de la lista: la cuenta de filas truncaba hacia cero y los
  // 33 px de arriba de la primera fila contaban como la primera fila
  onTap(200, 138);
  CHECK_MSG(cardOpen, "tocar el letrero cerro la ficha");
  CHECK_EQ((int)moveShown, 0);
  for (uint8_t i = 0; i < 4; i++) CHECK_EQ((int)pet.ppMove[i], (int)antes[i]);
  // y la quinta fila, que SI se ve, sigue respondiendo
  onTap(200, 146 + 4 * 34 + 16);
  CHECK_MSG(moveShown != 0, "la quinta fila dejo de responder");
  todoCerradoT();
}

// Las listas de la ficha: en medio, fallar no cierra; el titulo y el pie si.
TEST(toques, las_listas_de_la_ficha_se_cierran_arriba_o_abajo_y_no_en_medio) {
  preparaElMundoT();
  struct { uint8_t pag; int x, y; const char *donde; } huecos[] = {
    { 2, 229, 80,  "entre dos huecos de golpe" },
    { 2, 382, 200, "entre las filas y las flechas (golpes)" },
    { 3, 382, 200, "entre las filas y las flechas (bolsa)" },
    { 3, 60, 120,  "al lado del boton de ordenar" },
    { 4, 200, 101, "entre las pestanas y la primera fila" },
    { 4, 200, 106 + 36, "entre dos filas del equipo" },
  };
  for (auto &h : huecos) {
    todoCerradoT();
    cardOpen = true;
    cardPage = h.pag;
    onTap((int16_t)h.x, (int16_t)h.y);
    CHECK_MSG(cardOpen && cardPage == h.pag, std::string("cerro la ficha: ") + h.donde);
  }
  for (uint8_t pag = 2; pag <= 4; pag++) {
    todoCerradoT();
    cardOpen = true;
    cardPage = pag;
    onTap(233, 30);
    CHECK_MSG(!cardOpen, "tocar el titulo tiene que cerrar, pagina " + std::to_string(pag));
    cardOpen = true;
    cardPage = pag;
    // no time o "tap: back" desce para 398 (o botao TROCAR fica em cima)
    onTap(233, pag == 4 ? 413 : 396);
    CHECK_MSG(!cardOpen, "tocar 'tap: back' tiene que cerrar, pagina " + std::to_string(pag));
  }
  todoCerradoT();
}

// La flecha de las medallas estaba ENCIMA del "tap: back": tocar el
// letrero de volver pasaba de pagina.
TEST(toques, en_las_medallas_tap_back_vuelve_y_la_flecha_pasa_pagina) {
  preparaElMundoT();
  cardOpen = true;
  cardPage = 7;
  achPage = 0;
  onTap(233, 342);                              // la flecha
  CHECK_EQ((int)achPage, 1);
  CHECK_MSG(cardOpen, "pasar pagina cerro la ficha");
  onTap(233, 396);                              // el letrero
  CHECK_MSG(!cardOpen, "tocar 'tap: back' no volvio");
  CHECK_EQ((int)achPage, 1);
  todoCerradoT();
}

// LA REVANCHA tenia las mismas flechas que la bolsa vieja: 66x33 px, y
// fallarlas cerraba la lista.
TEST(toques, en_la_revancha_se_pasa_pagina_sin_que_se_cierre) {
  preparaElMundoT();
  uint8_t tenia = pet.badges;
  pet.badges = pet.gymCount();
  CHECK_MSG(pet.gymCount() > 7, "premisa: la lista era larga");
  rematchOpen = true;
  rematchScroll = 0;
  onTap(380, 380);                              // mitad derecha: baja
  CHECK_EQ((int)rematchScroll, 3);
  onTap(250, 362);                              // pegado a la ultima fila
  CHECK_EQ((int)rematchScroll, 6);
  onTap(100, 394);                              // mitad izquierda: sube
  CHECK_EQ((int)rematchScroll, 3);
  onTap(233, 356);                              // entre la ultima fila y la faja
  onTap(50, 240);                               // un pelo a la izquierda de una fila
  CHECK_MSG(rematchOpen && !bt.open, "fallar por un pelo cerro la revancha");
  CHECK_EQ((int)rematchScroll, 3);
  onSwipeV(-1);                                 // arrastrar baja
  CHECK_EQ((int)rematchScroll, 6);
  onSwipeV(+1);
  CHECK_EQ((int)rematchScroll, 3);
  CHECK_MSG(rematchOpen && !cardOpen && !clockOpen,
            "el gesto abrio otra cosa en vez de mover la lista");
  for (int i = 0; i < 20; i++) onTap(380, 380); // hasta el fondo y seguir
  CHECK_EQ((int)rematchScroll, (int)pet.gymCount() - 4);
  CHECK_MSG(rematchOpen, "tocar 'bajar' en el fondo cerro la revancha");
  onTap(233, 40);                               // el titulo: fuera
  CHECK_MSG(!rematchOpen, "tocar fuera tiene que volver");
  pet.badges = tenia;
  todoCerradoT();
}

// La lista de relevo del combate: el hueco entre dos filas devolvia al
// combate (o a la bolsa, con un objeto en la mano).
TEST(toques, en_la_lista_de_relevo_fallar_una_fila_no_cierra) {
  uint8_t hueco = combateConObjetos(0, 0);
  uint8_t antes = bt.mySlot;
  bt.phase = BP_PARTY;
  bt.itemPick = 0;
  onTap(200, 128 + 41);                         // entre la primera fila y la segunda
  onTap(48, 128 + hueco * 44 + 15);             // un pelo a la izquierda de una fila
  onTap(233, 400);                              // justo debajo del panel
  CHECK_MSG(bt.phase == BP_PARTY, "fallar una fila cerro la lista");
  CHECK_EQ((int)bt.mySlot, (int)antes);
  onTap(233, 60);                               // fuera de verdad
  CHECK_MSG(bt.phase == BP_CHOOSE, "tocar fuera tiene que volver al combate");

  // con un objeto en la mano: lo mismo, y fuera vuelve a la BOLSA
  uint8_t pocion = primerObjetoT(BP_BATTLE, IU_POKEMON);
  CHECK_MSG(pocion != 0, "premisa: hay un objeto de los que piden a quien");
  bt.phase = BP_PARTY;
  bt.itemPick = pocion;
  onTap(200, 128 + 41);
  CHECK_MSG(bt.phase == BP_PARTY && bt.itemPick == pocion,
            "fallar una fila solto el objeto");
  onTap(233, 60);
  CHECK_MSG(bt.phase == BP_BAG && bt.itemPick == 0, "fuera tiene que volver a la bolsa");
  bt.phase = BP_CHOOSE;
  cierraElCombate();
}

// Las pestanas de la bolsa del combate miden 28 px de alto dibujadas: se
// tocan en toda la cabecera del panel.
TEST(toques, en_la_bolsa_del_combate_la_cabecera_cambia_de_pestana) {
  combateConObjetos(0, 0);
  onTap(BBAG_BTN_X_T, BBAG_BTN_Y_T);
  CHECK_MSG(bt.phase == BP_BAG, "el boton no abrio la bolsa");
  CHECK_EQ((int)bt.bagPocket, 0);
  onTap(320, 112);                              // a la altura del titulo
  CHECK_EQ((int)bt.bagPocket, 1);
  onTap(120, 158);                              // justo debajo de la pestana
  CHECK_EQ((int)bt.bagPocket, 0);
  CHECK_MSG(bt.phase == BP_BAG, "cambiar de pestana cerro la bolsa");
  bt.phase = BP_CHOOSE;
  cierraElCombate();
}

// =====================================================================
// O CLIMA em tela cheia. "a animacao do sandstorm esta errada, deveria ser
// na tela toda nao?" —— no Anil a animacao do clima (Common:Sandstorm) toca
// em laco por cima do combate enquanto o clima dura; aqui so tocava a do
// golpe, um redemoinho em volta de quem usava.

// Quantos pixeis de uma caixa mudaram em relacao a `ref`, em por mil.
static int mudouPorMil(const uint16_t *ref, int x0, int y0, int x1, int y1) {
  const uint16_t *fb = gfx->getFramebuffer();
  int n = 0, tot = 0;
  for (int y = y0; y < y1; y++)
    for (int x = x0; x < x1; x++, tot++)
      if (fb[y * 466 + x] != ref[y * 466 + x]) n++;
  return tot ? n * 1000 / tot : 0;
}

// Quatro caixas espalhadas pelo vidro, longe das chapas de PS e dos botoes
// (que vao POR CIMA do clima e por isso nao mudam).
static const int CAIXAS_CLIMA[4][4] = {
  { 130, 22, 200, 50 },      // o ceu, em cima
  { 70, 130, 120, 165 },     // a esquerda, ao lado do rival
  { 205, 200, 260, 230 },    // o meio, entre os dois
  { 375, 345, 405, 375 },    // embaixo a direita
};

TEST(clima, a_areia_cobre_a_tela_inteira_e_fica_em_laco) {
  combateConObjetos(0, 0);
  bt.bg = -1;
  bt.b.weather = WE_NONE;
  avanza(120, 20);
  static uint16_t limpo[466 * 466];
  memcpy(limpo, gfx->getFramebuffer(), sizeof(limpo));
  CHECK_MSG(animClima.blob == nullptr, "premissa: sem clima nao ha nada carregado");

  bt.b.weather = WE_SAND;
  bt.b.weatherTurns = 5;
  avanza(640, 20);                 // quadro 12 de 20: a areia no auge
  CHECK_MSG(animClima.clima == WE_SAND, "nao carregou /anim/w_3.bin");
  volcaPantalla("clima_areia");
  for (auto &c : CAIXAS_CLIMA) {
    int m = mudouPorMil(limpo, c[0], c[1], c[2], c[3]);
    CHECK_MSG(m > 600, "a areia nao chega a (" + std::to_string(c[0]) + "," +
                       std::to_string(c[1]) + "): mudou " + std::to_string(m) + " por mil");
  }

  // ANDA: a figura rola, dois quadros seguidos nao sao o mesmo desenho
  static uint16_t antes[466 * 466];
  memcpy(antes, gfx->getFramebuffer(), sizeof(antes));
  avanza(100, 20);
  CHECK_MSG(mudouPorMil(antes, 130, 22, 200, 50) > 300, "a areia esta parada");

  // EM LACO: a animacao tem 20 quadros (1 s). Tres segundos depois, na
  // mesma altura do laco, continua la.
  avanza(2900, 20);
  CHECK_MSG(mudouPorMil(limpo, 130, 22, 200, 50) > 600,
            "passado o primeiro segundo a areia sumiu: nao esta em laco");

  // acabou o clima: some, e o arquivo se solta
  bt.b.weather = WE_NONE;
  avanza(100, 20);
  CHECK_MSG(mudouPorMil(limpo, 130, 22, 200, 50) < 50, "acabou o clima e a areia ficou");
  CHECK_MSG(animClima.blob == nullptr, "acabou o clima e o arquivo ficou na memoria");
  cierraElCombate();
}

TEST(clima, os_quatro_climas_tem_a_sua_animacao) {
  combateConObjetos(0, 0);
  bt.bg = -1;
  bt.b.weather = WE_NONE;
  avanza(120, 20);
  static uint16_t limpo[466 * 466];
  memcpy(limpo, gfx->getFramebuffer(), sizeof(limpo));
  const char *nome[5] = { "", "sol", "chuva", "areia", "granizo" };
  // Mede-se numa faixa do CEU onde, sem clima, nao muda nada: nem os
  // bichos respirando, nem as chapas, nem o letreiro do clima. A primeira
  // versao olhava a tela quase inteira e seguia verde sem as gotas —— o
  // que mexia era o resto.
  const int FX0 = 100, FY0 = 0, FX1 = 366, FY1 = 60;
  avanza(700, 25);
  CHECK_EQ(mudouPorMil(limpo, FX0, FY0, FX1, FY1), 0);   // premissa: faixa parada
  for (uint8_t w = WE_SUN; w <= WE_HAIL; w++) {
    bt.b.weather = w;
    bt.b.weatherTurns = 5;
    // um laco inteiro, olhando quadro a quadro: a chuva e o granizo sao
    // poucas particulas, basta que ALGUM quadro mexa na tela
    // E mede-se o que MEXE de um quadro para o seguinte, nao a diferenca
    // com a tela limpa: o cenario escurece com a chuva por conta propria,
    // e isso tambem deixava o teste verde sem uma gota na tela.
    int maior = 0;
    static uint16_t ant[466 * 466];
    for (int i = 0; i < 45; i++) {
      avanza(50, 25);
      // Os tres primeiros nao contam: a tela repinta a cada dois passos e
      // ainda mostra o clima ANTERIOR sumindo, que e "movimento" de sobra.
      int m = i >= 3 ? mudouPorMil(ant, FX0, FY0, FX1, FY1) : 0;
      memcpy(ant, gfx->getFramebuffer(), sizeof(ant));
      if (m > maior) maior = m;
      if (i == 12) volcaPantalla((std::string("clima_") + nome[w]).c_str());
    }
    CHECK_MSG(animClima.clima == w, std::string("nao carregou a animacao: ") + nome[w]);
    CHECK_MSG(maior > 5, std::string("o clima nao se mexe no ceu: ") + nome[w] + " (" +
                         std::to_string(maior) + " por mil)");
  }
  bt.b.weather = WE_NONE;
  avanza(60, 20);
  cierraElCombate();
}

// =====================================================================
// OS SONS DA POKEBOLA. "consegue colocar os sons da pokebola arremessada e
// capturando?" —— a bola voava, abria e fechava com o pitido do menu. A
// ordem e a do Anil (PokeballThrowCapture): arremesso, chegada, queda, um
// toc por balanco, e no fim o clique (pegou) OU a abertura (escapou).
// Tira `bola` pela bolsa de verdade e confere os sons ate a bola se
// resolver. Devolve 1 se pegou, 0 se escapou, -1 se nem saiu.
static int tiraEEscuta(uint8_t bola) {
  const size_t nao = std::string::npos;
  for (uint16_t i = 1; i < ITEM_COUNT; i++)
    if (ITEM_TBL[i].pocket == BP_BALL) pet.bag[i] = 0;
  pet.bag[bola] = 5;
  mockAudioLogClear();
  onTap(BBAG_BTN_X_T, BBAG_BTN_Y_T);
  onTap(320, 112);                 // a aba das bolas
  onTap(233, 164 + 17);            // a primeira (e unica) fila
  CHECK_MSG(bt.phase == BP_THROW, "premissa: a bola saiu da bolsa");
  if (bt.phase != BP_THROW) return -1;
  for (int i = 0; i < 500 && (bt.phase == BP_THROW || bt.phase == BP_SUCK ||
                              bt.phase == BP_WOBBLE); i++)
    avanza(16);
  std::string a = mockAudioLog();
  size_t t = a.find("sfx:BALL_THROW"), h = a.find("sfx:BALL_HIT");
  size_t d = a.find("sfx:BALL_DROP"), s1 = a.find("sfx:BALL_SHAKE");
  int vaivens = 0;
  for (size_t p = s1; p != nao; p = a.find("sfx:BALL_SHAKE", p + 1)) vaivens++;
  size_t fim = a.find(bt.caught ? "sfx:BALL_CLICK" : "sfx:BALL_OPEN");
  size_t outro = a.find(bt.caught ? "sfx:BALL_OPEN" : "sfx:BALL_CLICK");
  CHECK_MSG(t != nao && h != nao && d != nao && s1 != nao && fim != nao &&
            t < h && h < d && d < s1 && s1 < fim,
            std::string(bt.caught ? "pegou" : "escapou") +
            ", e os sons da bola nao sairam nesta ordem: " + a);
  CHECK_MSG(vaivens >= 3, "poucos balancos com som: " + std::to_string(vaivens));
  CHECK_MSG(outro == nao, "soaram o clique E a abertura: " + a);
  CHECK_MSG(t == nao || a.find("sfx:TAP", t) == nao,
            "depois do arremesso voltou a soar o pitido do menu: " + a);
  return bt.caught ? 1 : 0;
}

TEST(captura, a_bola_soa_como_no_anil) {
  // ESCAPA: um Mewtwo de nivel 70 inteiro, com a bola comum.
  CHECK_MSG(combateDeBola(150, 70), "premissa: o combate tem que abrir");
  for (uint8_t i = 0; i < TEAM_SLOTS; i++)
    if (i != pet.mainSlot) pet.team[i].species = 0;
  for (uint8_t i = 0; i < BANK_SLOTS; i++) pet.bank[i].species = 0;
  bool escapou = false;
  for (int tent = 0; tent < 6 && bt.open && !escapou; tent++) {
    for (int i = 0; i < 900 && bt.open && bt.phase != BP_CHOOSE; i++) {
      bt.b.side[BT_PLAYER].hpMax = 9999;      // que o meu nao caia
      bt.b.side[BT_PLAYER].hp = 9999;
      avanza(16);
    }
    if (!bt.open || bt.phase != BP_CHOOSE) break;
    if (tiraEEscuta(BALL_ID[0]) == 0) escapou = true;
  }
  CHECK_MSG(escapou, "premissa: um Mewtwo inteiro tinha de escapar da bola comum");
  for (int i = 0; i < 400 && bt.open; i++) avanza(16);
  galleryOpen = false;
  galleryDetail = 0;
  cierraElCombate();

  // PEGA: um Pidgey a 1 PS, dormindo, com a Master Ball.
  CHECK_MSG(combateDeBola(16, 3), "premissa: o segundo combate tem que abrir");
  uint8_t master = bolaUi("Master Ball");
  CHECK_MSG(master != 0, "premissa: a Master Ball existe");
  bt.b.side[BT_FOE].hp = 1;
  bt.b.side[BT_FOE].status = ST_SLEEP;
  CHECK_MSG(tiraEEscuta(master) == 1, "premissa: a Master Ball tinha de pegar");
  for (int i = 0; i < 400 && bt.open; i++) avanza(16);
  galleryOpen = false;
  galleryDetail = 0;
  cierraElCombate();
}

// =====================================================================
// EL MULTIGOLPE EN PANTALLA. "se for acertar 3x vezes, bater 3 vezes":
// una animacion por impacto, y la barra del rival baja un trozo con cada
// uno, no de golpe al anunciar el golpe.
static int anchoBarraRival() {
  // la barra de PS de la chapa del rival (btHpBar en y=68: relleno en
  // y+18..y+25), contada en pixeles del verde de "bien de vida"
  return pixelesDeColor(0x5DCD /* UI_BAR_OK, species.h */, 0, 86, 466, 94);
}

TEST(multigolpe, cada_impacto_toca_la_animacion_y_baja_la_barra) {
  arranca();
  if (pet.awaitingStarter()) pet.chooseStarter(1);
  while (pet.isEgg()) pet.eggTap();
  cierraElCombate();
  pet.sleeping = false;
  pet.energy = 100;
  gestosOn = false;
  galleryOpen = false;
  cardOpen = false;
  determUntil = 0;
  uint16_t dk = 0, splash = 0;
  for (uint16_t m = 1; m < MOVE_COUNT; m++) {
    if (!strcmp(MOVE_TBL[m].name, "DOUBLEKICK")) dk = m;
    if (!strcmp(MOVE_TBL[m].name, "SPLASH")) splash = m;
  }
  CHECK_MSG(dk && splash, "premisa: DOUBLEKICK y SPLASH existen");
  battleStart(143, 40);                  // un Snorlax: aguanta
  for (int i = 0; i < 400 && bt.phase != BP_CHOOSE; i++) avanza(16);
  CHECK_MSG(bt.phase == BP_CHOOSE, "premisa: el combate tiene que abrir");
  bt.bg = -1;
  for (uint8_t s = 0; s < 2; s++) {
    Combatant &c = bt.b.side[s];
    c.moves[0] = s == BT_PLAYER ? dk : splash;
    c.usesLeft[0] = 30;
    c.nMoves = 1;
    // 200 y no 999: un chute quita ~4 PS, y con 999 eso es menos de un
    // pixel de barra —— segun la tirada el segundo no movia nada (paso
    // por suerte hasta que otro test cambio el orden de los azares).
    c.hp = c.hpMax = 200;
    c.bond = 255;
    c.stage[SG_SPE] = (s == BT_PLAYER) ? 6 : -6;   // yo primero
  }
  avanza(60);
  const int w0 = anchoBarraRival();
  CHECK_MSG(w0 > 100, "premisa: la barra del rival se ve llena");

  bt.sel = 0;
  toca(233, (BT_CARD_Y0_T + BT_CARD_Y1_T) / 2);
  int ataques = 0, w1 = -1, w2 = -1;
  uint8_t fase = bt.phase;
  for (int i = 0; i < 1500 && bt.open; i++) {
    avanza(10, 10);
    if (bt.phase == BP_ATTACK && fase != BP_ATTACK) ataques++;
    if (bt.phase == BP_HIT && bt.acting == BT_PLAYER) {
      if (bt.golpeN == 0) w1 = anchoBarraRival();
      if (bt.golpeN == 1) w2 = anchoBarraRival();
    }
    fase = bt.phase;
    if (bt.phase == BP_MSG && bt.acting == BT_PLAYER && w2 >= 0) break;
  }
  CHECK_MSG(bt.last.hits == 2, "premisa: DOUBLEKICK pego dos veces");
  CHECK_MSG(ataques == 2, "DOUBLEKICK toco la animacion " + std::to_string(ataques) +
                          " veces y son dos impactos");
  CHECK_MSG(w1 >= 0 && w2 >= 0, "no se vio la barra en los dos impactos");
  CHECK_MSG(bt.last.hitDmg[1] >= 2, "premisa: el segundo impacto quita PS");
  CHECK_MSG(w0 > w1, "al primer impacto la barra no bajo (" + std::to_string(w0) +
                     " -> " + std::to_string(w1) + ")");
  CHECK_MSG(w1 > w2, "al segundo impacto la barra no bajo otro trozo (" +
                     std::to_string(w1) + " -> " + std::to_string(w2) + ")");
  // y despues del golpe la barra ya esta donde dice el motor: no da otro salto
  avanza(60);
  CHECK_MSG(bt.phase != BP_ATTACK && bt.phase != BP_HIT, "premisa: ya se resolvio");
  CHECK_EQ(anchoBarraRival(), w2);
  while (bt.open) { bt.phase = BP_END; bt.until = millis(); avanza(32); }
}

// A leva 21 deixou a barra SUBINDO antes de cair e caindo so no fim da
// animacao. Mede-se a barra inteira, nas tres cores, quadro a quadro.
static int barraRivalToda() {
  return pixelesDeColor(0x5DCD, 0, 86, 466, 94) +
         pixelesDeColor(0xED07, 0, 86, 466, 94) +
         pixelesDeColor(0xEA87, 0, 86, 466, 94);
}

// Toca o golpe com o rival em vida/vidaMax. Devolve a maior barra vista
// depois do toque e, em *cedo, a barra 150 ms depois de a animacao comecar.
static int gDegraus[3] = { -1, -1, -1 };   // a barra no fim de cada impacto
static int barraDuranteOGolpe(const char *golpe, uint16_t vida, uint16_t vidaMax,
                              int *antes, int *cedo) {
  arranca();
  if (pet.awaitingStarter()) pet.chooseStarter(1);
  while (pet.isEgg()) pet.eggTap();
  cierraElCombate();
  pet.sleeping = false;
  pet.energy = 100;
  gestosOn = false;
  galleryOpen = false;
  cardOpen = false;
  determUntil = 0;
  uint16_t mv = 0, splash = 0;
  for (uint16_t m = 1; m < MOVE_COUNT; m++) {
    if (!strcmp(MOVE_TBL[m].name, golpe)) mv = m;
    if (!strcmp(MOVE_TBL[m].name, "SPLASH")) splash = m;
  }
  pet.hpLeft = pet.maxHp();
  battleStart(143, 40);
  for (int i = 0; i < 400 && bt.phase != BP_CHOOSE; i++) avanza(16);
  bt.bg = -1;
  for (uint8_t s = 0; s < 2; s++) {
    Combatant &c = bt.b.side[s];
    c.moves[0] = s == BT_PLAYER ? mv : splash;
    c.usesLeft[0] = 30;
    c.nMoves = 1;
    c.bond = 255;
    c.stage[SG_SPE] = (s == BT_PLAYER) ? 6 : -6;
    c.hp = c.hpMax = 999;
  }
  // Dano de sobra: o Snorlax aguenta, e sem isto um bicho fraco de outro
  // teste nem passa da vida que o rival tem.
  bt.b.side[BT_PLAYER].stage[SG_ATK] = 6;
  bt.b.side[BT_FOE].stage[SG_DEF] = -6;
  bt.b.side[BT_FOE].hpMax = vidaMax;
  bt.b.side[BT_FOE].hp = vida;
  avanza(60);
  *antes = barraRivalToda();
  *cedo = -1;
  int maior = 0;
  bt.sel = 0;
  toca(233, (BT_CARD_Y0_T + BT_CARD_Y1_T) / 2);
  for (int i = 0; i < 1500 && bt.open; i++) {
    avanza(10, 10);
    if (bt.phase != BP_SAY && bt.phase != BP_ATTACK && bt.phase != BP_HIT) break;
    int w = barraRivalToda();
    if (w > maior) maior = w;
    if (*cedo < 0 && bt.phase == BP_ATTACK && millis() - bt.fxAt > 150) *cedo = w;
    if (bt.phase == BP_HIT && bt.golpeN < 3) gDegraus[bt.golpeN] = w;
  }
  return maior;
}

// A barra de quem APANHA, medida a cada 10 ms desde o toque ate acabar o
// golpe. Devolve quantas vezes ela subiu; *caiu = se chegou a cair.
static int subidasDaBarra(const char *golpe, uint8_t quemBate, uint16_t vida, bool *caiu,
                          std::string *serie) {
  arranca();
  if (pet.awaitingStarter()) pet.chooseStarter(1);
  while (pet.isEgg()) pet.eggTap();
  cierraElCombate();
  pet.sleeping = false;
  pet.energy = 100;
  pet.hpLeft = pet.maxHp();     // o caso anterior pode ter derrubado o meu
  gestosOn = false;
  galleryOpen = false;
  cardOpen = false;
  determUntil = 0;
  uint16_t mv = 0, splash = 0;
  for (uint16_t m = 1; m < MOVE_COUNT; m++) {
    if (!strcmp(MOVE_TBL[m].name, golpe)) mv = m;
    if (!strcmp(MOVE_TBL[m].name, "SPLASH")) splash = m;
  }
  battleStart(143, 40);
  for (int i = 0; i < 400 && bt.phase != BP_CHOOSE; i++) avanza(16);
  bt.bg = -1;
  for (uint8_t s = 0; s < 2; s++) {
    Combatant &c = bt.b.side[s];
    c.moves[0] = s == quemBate ? mv : splash;
    c.usesLeft[0] = 30;
    c.nMoves = 1;
    c.bond = 255;
    c.item = 0;
    c.stage[SG_SPE] = (s == quemBate) ? 6 : -6;
    c.hpMax = 300;
    c.hp = s == quemBate ? 150 : vida;
  }
  const int y0 = quemBate == BT_PLAYER ? 86 : 246;
  auto barra = [&]() {
    return pixelesDeColor(0x5DCD, 0, y0, 466, y0 + 8) +
           pixelesDeColor(0xED07, 0, y0, 466, y0 + 8) +
           pixelesDeColor(0xEA87, 0, y0, 466, y0 + 8);
  };
  avanza(60);
  int w = barra(), w0 = w, sobe = 0;
  bool viu = false;
  *serie = std::to_string(w);
  bt.sel = 0;
  toca(233, (BT_CARD_Y0_T + BT_CARD_Y1_T) / 2);
  for (int i = 0; i < 2500 && bt.open; i++) {
    avanza(10, 10);
    if (bt.phase == BP_ATTACK && bt.acting == quemBate) viu = true;
    if (viu && bt.phase != BP_ATTACK && bt.phase != BP_HIT) break;
    int n = barra();
    if (n != w) *serie += ">" + std::to_string(n);
    // A barra anda de 8 em 8 pixels (1 px de largura, 8 de alto): menos do
    // que isso e um pixel solto de particula da mesma cor, nao a barra.
    if (n >= w + 8) sobe++;
    w = n;
  }
  *caiu = w < w0;
  *serie += " (dano " + std::to_string(bt.last.damage) + ", vida " + std::to_string(vida) + ")";
  while (bt.open) { bt.phase = BP_END; bt.until = millis(); avanza(32); }
  return sobe;
}

TEST(multigolpe, a_barra_de_quem_apanha_so_desce) {
  static const char *const GOLPES[] = { "TACKLE", "DOUBLEKICK", "FURYSWIPES", "GIGADRAIN",
                                        "DOUBLEEDGE", "SONICBOOM", "FLAREBLITZ" };
  std::string mal;
  int cairam = 0;
  for (const char *g : GOLPES)
    for (uint8_t quem = 0; quem < 2; quem++)
      for (uint16_t vida : { (uint16_t)280, (uint16_t)12 }) {
        bool caiu = false;
        std::string serie;
        randomSeed(11);
        int sobe = subidasDaBarra(g, quem, vida, &caiu, &serie);
        if (caiu) cairam++;
        if (sobe) mal += std::string(g) + (quem ? " rival" : " meu") + " vida " +
                         std::to_string(vida) + ": " + serie + "; ";
      }
  CHECK_MSG(cairam >= 14, "premissa: a barra caiu em poucos casos: " + std::to_string(cairam));
  CHECK_MSG(mal.empty(), "a barra SUBIU em: " + mal);
}

TEST(multigolpe, tres_impactos_sao_tres_degraus_na_barra) {
  // Com dois impactos o ultimo ja zera o resto e a conta do meio nao aparece.
  int antes, cedo;
  gDegraus[0] = gDegraus[1] = gDegraus[2] = -1;
  // O bicho e FIXO: herdado de outro teste, um fraco nao anda nem 1 px na
  // barra e um forte derruba antes do terceiro impacto.
  arranca();
  if (pet.awaitingStarter()) pet.chooseStarter(1);
  while (pet.isEgg()) pet.eggTap();
  cierraElCombate();
  const int16_t espAntes = pet.speciesId;
  const uint32_t expAntes = pet.exp;
  pet.speciesId = 25;
  pet.form = 0;
  pet.exp = Pet::expForLevel(30);
  barraDuranteOGolpe("TRIPLEHIT", 600, 600, &antes, &cedo);
  CHECK_EQ((int)bt.last.hits, 3);
  CHECK_MSG(antes > gDegraus[0] && gDegraus[0] > gDegraus[1] && gDegraus[1] > gDegraus[2],
            "a barra nao desceu em tres degraus: " + std::to_string(antes) + " " +
            std::to_string(gDegraus[0]) + " " + std::to_string(gDegraus[1]) + " " +
            std::to_string(gDegraus[2]));
  while (bt.open) { bt.phase = BP_END; bt.until = millis(); avanza(32); }
  pet.speciesId = espAntes;
  pet.exp = expAntes;
  pet.hpLeft = pet.maxHp();
}

TEST(multigolpe, um_golpe_so_baixa_a_barra_quando_a_animacao_comeca) {
  int antes, cedo;
  barraDuranteOGolpe("TACKLE", 120, 120, &antes, &cedo);
  CHECK_MSG(bt.last.damage > 0 && bt.last.hits == 1, "premissa: um impacto, com dano");
  CHECK_MSG(cedo >= 0, "premissa: a animacao tocou");
  CHECK_MSG(cedo < antes, "150 ms depois de comecar a barra ainda nao caiu (" +
                          std::to_string(antes) + " -> " + std::to_string(cedo) + ")");
  while (bt.open) { bt.phase = BP_END; bt.until = millis(); avanza(32); }
}

// =====================================================================
// A AVENTURA NA TELA.
extern bool avOpen;
extern int8_t avFixo;
extern AvResumo avRes;
extern int8_t avFixoClima;
void aventuraAbre();
void aventuraMede(int16_t dex, int *alto, int *largo);
uint32_t rtcEpoch();
extern uint8_t achPage;

// Um titular com time e caixa, para o que a cena precisa.
static void mundoDaAventura(int16_t titular) {
  preparaElMundoT();
  pet.speciesId = titular;
  pet.form = 0;
  pet.exp = Pet::expForLevel(50);
  pet.heldItem = 0;
  pet.avDesde = 0;
}

TEST(aventura_ui, o_botao_da_casa_abre_e_finish_mostra_o_resumo) {
  mundoDaAventura(59);
  pet.exp = Pet::expForLevel(5);       // abaixo do teto de nivel deste cenario
  pet.sleeping = false;
  avanza(200);
  mockSetEpoch(1760000000);
  onTap(270, 419);                     // a mochila, o quarto botao
  CHECK_MSG(avOpen && pet.aventuraAtiva(), "o botao nao abriu a aventura");
  avanza(300);
  // tocar no meio nao acaba a aventura
  onTap(233, 200);
  onTap(60, 233);
  CHECK_MSG(avOpen && pet.aventuraAtiva(), "encostar na tela acabou a aventura");
  // os gestos tambem nao abrem nada por baixo
  onSwipe(-1);
  onSwipeV(1);
  CHECK_MSG(!galleryOpen && !clockOpen && !cardOpen, "um gesto abriu outra tela por baixo");
  uint32_t e0 = pet.exp;
  mockSetEpoch(1760000000 + 3 * 3600);
  onTap(233, 410);
  CHECK_MSG(!pet.aventuraAtiva(), "FINISH nao acabou");
  CHECK_MSG(pet.exp > e0, "tres horas nao deram nada");
  CHECK_MSG(avRes.nMons >= 2, "o resumo nao trouxe o elenco");
  avanza(200);
  volcaPantalla("av_resumo");
  for (int i = 0; i < 6 && avOpen; i++) onTap(233, 233);
  CHECK_MSG(!avOpen, "o resumo nao fecha tocando");
  mockSetEpoch(1758700000);
}

#define AV_RAZAO_T 0.243f   // AV_RAZAO_PADRAO de aventura.ino
TEST(aventura_ui, quem_nao_tem_sprite_pmd_cai_no_padrao_do_mesmo_tamanho) {
  // 59 tem /av/; 626 (Bouffalant) nao. Os dois tem de sair com a altura que a
  // conta pede, e nao um gigante e um anao.
  extern int16_t ultimoAltoPintado;
  CHECK_MSG(std::filesystem::exists("../tools/sdcard/av/p0059.bin"), "premissa: 59 tem sprite da aventura");
  CHECK_MSG(!std::filesystem::exists("../tools/sdcard/av/p0626.bin"), "premissa: 626 nao tem");
  int altura[2] = { 0, 0 };
  const int16_t QUEM[2] = { 59, 626 };
  for (int k = 0; k < 2; k++) {
    mundoDaAventura(QUEM[k]);
    mockSetEpoch(1760000000);
    pet.aventuraComeca(rtcEpoch(), 7);
    // A luta: no comeco ele esta PARADO (a acao de que sai a escala) e e o
    // ultimo a ser pintado. Andando, o Arcanine e mais baixo: 26 linhas contra 29.
    avFixo = 3;
    aventuraAbre();
    avanza(400);
    altura[k] = ultimoAltoPintado;
    CHECK_MSG(altura[k] > 40, "o bicho " + std::to_string(QUEM[k]) + " nao se desenhou");
    AvResumo r;
    pet.aventuraFim(rtcEpoch(), r);
    avOpen = false;
    avFixo = -1;
  }
  // O do PMD: o zoom unico sobre a altura dele (Arcanine, 29 px parado). O padrao,
  // sem como medir assim: a tabela do jogo na mesma escala.
  CHECK_MSG(altura[0] > 29 * 4 * 0.85f && altura[0] < 29 * 4 * 1.15f,
            "o 59 saiu com " + std::to_string(altura[0]) + " e o zoom pede 116");
  const float padrao = DEX_TBL[626].screenSz * AV_RAZAO_T * 4.0f;
  CHECK_MSG(altura[1] > padrao * 0.85f && altura[1] < padrao * 1.15f,
            "o 626 saiu com " + std::to_string(altura[1]) + " e a conta pede " + std::to_string((int)padrao));
  mockSetEpoch(1758700000);
}

TEST(aventura_ui, a_cena_nunca_sobe_o_tamanho_nem_some_em_nenhum_evento) {
  mundoDaAventura(6);
  for (uint8_t i = 1; i < ITEM_COUNT; i++)
    if (!strcmp(ITEM_TBL[i].name, "Charizardite X")) pet.heldItem = i;
  mockSetEpoch(1760000000);
  pet.aventuraComeca(rtcEpoch(), 11);
  mockSetEpoch(1760000000 + 8 * 3600);    // ja ha itens para achar
  uint32_t quadros = g_frames;
  for (int e = 0; e < 7; e++) {
    avFixo = (int8_t)e;
    aventuraAbre();
    for (int q = 0; q < 8; q++) avanza(900, 30);
  }
  CHECK_MSG(g_frames > quadros + 100, "a cena parou de pintar");
  AvResumo r;
  pet.aventuraFim(rtcEpoch(), r);
  avOpen = false;
  avFixo = -1;
  mockSetEpoch(1758700000);
}

TEST(aventura_ui, um_gallade_e_bem_maior_que_um_minun) {
  // "nao faz sentido um gallade minusculo e um minun gigante do tamanho dele"
  arranca();
  int ag, lg, am, lm, ac, lc, ap, lp;
  aventuraMede(475, &ag, &lg);
  aventuraMede(312, &am, &lm);
  aventuraMede(6, &ac, &lc);
  aventuraMede(10, &ap, &lp);
  CHECK_MSG(ag > 0 && am > 0 && ac > 0 && ap > 0, "premissa: todos carregam");
  // E os grandes continuam grandes, nao espremidos ate o tamanho de um Eevee.
  int ae, le, agz, lgz;
  aventuraMede(133, &ae, &le);
  aventuraMede(799, &agz, &lgz);
  CHECK_MSG(ag * 2 >= am * 3, "Gallade " + std::to_string(ag) + " e Minun " + std::to_string(am));
  CHECK_MSG(ac > ap, "Charizard " + std::to_string(ac) + " e Caterpie " + std::to_string(ap));
  CHECK_MSG(am >= 60, "o Minun sumiu: " + std::to_string(am));
  // "GUZZLORD FICOU MINUSCULO": a caixa simetrica espremia ele a metade.
  CHECK_MSG(agz >= 130, "Guzzlord ficou com " + std::to_string(agz));
  CHECK_MSG(agz >= ae * 3 / 2, "Guzzlord " + std::to_string(agz) + " perto do Eevee " + std::to_string(ae));
}

TEST(aventura_ui, os_gigantes_cabem_e_dois_cabem_lado_a_lado) {
  // Reclamado de antemao: "tem pokemons que o sprite ja sao grandes por
  // natureza, tipo o Guzzlord". A cena pede x 136 e x 330: cada um tem ate
  // ~95 px para cada lado sem invadir o outro, e o circulo fecha em cima.
  arranca();
  int vistos = 0;
  std::string mal;
  std::vector<int16_t> quem = { 799, 321, 890, 208, 95, 384, 382, 383, 248, 143, 6, 59, 10, 132 };
  for (int16_t d = 1; d <= DEX_COUNT; d += 37) quem.push_back(d);
  for (int16_t d : quem) {
    int alto = 0, largo = 0;
    aventuraMede(d, &alto, &largo);
    if (!alto) continue;
    vistos++;
    if (alto > 192) mal += std::to_string(d) + " alto " + std::to_string(alto) + "; ";
    if (largo > 302) mal += std::to_string(d) + " largo " + std::to_string(largo) + "; ";
    if (alto < 36) mal += std::to_string(d) + " pequeno " + std::to_string(alto) + "; ";
  }
  CHECK_MSG(vistos >= 30, "premissa: poucos bichos medidos: " + std::to_string(vistos));
  CHECK_MSG(mal.empty(), mal);
}

TEST(aventura_ui, o_clima_de_enfeite_nao_vaza_para_a_batalha) {
  mundoDaAventura(59);
  mockSetEpoch(1760000000);
  pet.aventuraComeca(rtcEpoch(), 7);
  avFixoClima = WE_SAND;
  aventuraAbre();
  avanza(400);
  CHECK_EQ((int)bt.b.weather, (int)WE_SAND);
  avFixoClima = -1;
  onTap(233, 410);                     // FINISH
  CHECK_EQ((int)bt.b.weather, (int)WE_NONE);
  for (int i = 0; i < 6 && avOpen; i++) onTap(233, 233);
  CHECK_MSG(!avOpen, "premissa: o resumo fechou");
  // e numa batalha de verdade o clima comeca limpo
  pet.sleeping = false;
  pet.energy = 100;
  pet.hpLeft = pet.maxHp();
  battleStart(16, 5);
  CHECK_EQ((int)bt.b.weather, (int)WE_NONE);
  cierraElCombate();
  mockSetEpoch(1758700000);
}

// OS SEIS BOTOES: cada um faz o seu, e nenhum cai no do lado.
TEST(casa, os_seis_botoes_fazem_cada_um_o_seu) {
  mundoDaAventura(25);
  pet.sleeping = false;
  pet.energy = 100;
  pet.hpLeft = pet.maxHp();
  avanza(60);
  extern uint32_t feedMenuUntil, bathUntil;
  extern bool gymOpen;
  onTap(74, 337);
  CHECK_MSG(feedMenuUntil != 0, "o primeiro nao abriu a comida");
  feedMenuUntil = 0;
  onTap(270, 419);
  CHECK_MSG(avOpen, "o quarto nao abriu a aventura");
  AvResumo r;
  pet.aventuraFim(rtcEpoch(), r);
  avOpen = false;
  bool luz = pet.sleeping;
  onTap(339, 390);
  CHECK_MSG(pet.sleeping != luz, "o quinto nao mexeu na luz");
  onTap(339, 390);
  CHECK_MSG(pet.sleeping == luz, "o quinto nao voltou a luz");
  pet.sleeping = false;
  onTap(392, 337);
  CHECK_MSG(bathUntil != 0, "o sexto nao deu banho");
  avanza(3200, 40);
  onTap(127, 390);
  CHECK_MSG(gymOpen, "o segundo nao abriu o treino");
  gymOpen = false;
}

// Em casa, o balao fica fora do corpo tambem num bicho grande: a distancia sai da
// largura dele. "percebi que os baloes em pokemons maiores cobrem boa parte deles"
int balaoAoLado();
TEST(casa, o_balao_se_afasta_de_quem_e_grande) {
  mundoDaAventura(321);                // Wailord
  pet.sleeping = false;
  avanza(300);
  const int grande = balaoAoLado();
  mundoDaAventura(25);                 // Pikachu
  pet.sleeping = false;
  avanza(300);
  const int pequeno = balaoAoLado();
  CHECK_MSG(grande > pequeno, "Wailord " + std::to_string(grande) + " e Pikachu " + std::to_string(pequeno));
  CHECK_MSG(pequeno >= 86, "premissa: o minimo de sempre");
  todoCerradoT();
}

// "quando ficar um tempo sem interagir, a musica tb cessa"
const char *mockAudioLog();
void mockAudioLogClear();
TEST(aventura_ui, a_musica_para_no_escuro_e_volta_ao_tocar) {
  mundoDaAventura(25);
  // Com som: outro teste pode ter deixado o volume a zero, e ai nada toca.
  const uint8_t volAntes = audioVolume();
  audioSetVolume(AUDIO_VOL_MAX);
  mockSetEpoch(1760000000);
  pet.aventuraComeca(rtcEpoch(), 7);
  aventuraAbre();
  avanza(500, 50);
  lastInteract = millis();                   // o ultimo toque foi agora
  mockAudioLogClear();
  avanza(95000, 250);                        // 95 s sem tocar: a tela escurece
  CHECK_MSG(dimStage >= 1, "premissa: a tela escureceu (um coracao do bicho de casa a segura?)");
  std::string a = mockAudioLog();
  CHECK_MSG(a.find("bgm:stop") != std::string::npos, "a musica nao parou no escuro: " + a);
  const size_t parou = a.rfind("bgm:stop");
  CHECK_MSG(a.find("bgm:", parou + 1) == std::string::npos, "voltou a tocar no escuro: " + a);
  mockAudioLogClear();
  toca(233, 200);                            // acorda (o toque nao acaba a aventura)
  avanza(600, 50);
  a = mockAudioLog();
  CHECK_MSG(a.find("bgm:") != std::string::npos, "nao voltou ao acordar: " + a);
  CHECK_MSG(avOpen, "o toque acabou a aventura");
  AvResumo r;
  pet.aventuraFim(rtcEpoch(), r);
  avOpen = false;
  audioSetVolume(volAntes);
  mockSetEpoch(1758700000);
}

// "jogar sem gravar, sem saber" foi o que levou Hoenn: com o save falhado, uma
// faixa vermelha no alto, e a cada minuto se tenta de novo.
static int vermelhosNoAlto() {
  const uint16_t *fb = gfx->getFramebuffer();
  int n = 0;
  for (int y = 118; y < 148; y++)
    for (int x = 233 - 80; x < 233 + 80; x++) {
      const uint16_t c = fb[y * gfx->width() + x];
      if (((c >> 11) & 31) > 22 && ((c >> 5) & 63) < 16 && (c & 31) < 8) n++;
    }
  return n;
}
void mockNvsRecusa(bool);
TEST(casa, o_save_que_falhou_aparece_e_se_tenta_de_novo) {
  mundoDaAventura(25);
  pet.sleeping = false;
  // Lixo de outros testes cobria a casa: "se sente abandonado" (o bicho
  // carente) e o cartaz de premio. Bicho bem cuidado, sem cartaz, e o tempo
  // dos cartazes passa (sao privados e por tempo).
  pet.fullness = pet.joy = pet.hygiene = pet.energy = 100;
  pet.newMedal = 0;
  mockSetEpoch(1758700000);
  pet.lastSeenEpoch = rtcEpoch();
  avanza(20000, 100);
  lastInteract = millis();
  avanza(300);
  CHECK_MSG(vermelhosNoAlto() < 50, "premissa: sem falha, sem faixa");
  mockNvsRecusa(true);                  // a NVS cheia: nada grava
  pet.saveNow();
  CHECK_MSG(pet.guardadoFallo, "premissa: com a NVS recusando, o save falha");
  lastInteract = millis();
  avanza(300);
  if (getenv("TAMA_FAIXA")) { gLang = LANG_PT; avanza(300); volcaPantalla("faixa_save"); gLang = LANG_DEFAULT; }
  CHECK_MSG(vermelhosNoAlto() > 300, "o save falhou e a tela nao disse nada");
  avanza(70000, 250);
  lastInteract = millis();
  avanza(300);
  CHECK_MSG(pet.guardadoFallo && vermelhosNoAlto() > 300, "com a NVS ainda cheia, o aviso sumiu");
  mockNvsRecusa(false);                 // abriu sitio
  avanza(61000, 250);                   // o minuto seguinte: tenta e passa
  CHECK_MSG(!pet.guardadoFallo, "nao tentou gravar de novo");
  lastInteract = millis();
  avanza(300);
  CHECK_MSG(vermelhosNoAlto() < 50, "a faixa ficou depois de gravar");
  todoCerradoT();
}

// Olhar a tela de casa: TAMA_CASA=1.
TEST(preview, casa) {
  if (!getenv("TAMA_CASA")) return;
  mundoDaAventura(59);
  pet.sleeping = false;
  pet.energy = 80;
  pet.fullness = 62;
  pet.joy = 40;
  pet.hygiene = 18;
  pet.hpLeft = pet.maxHp();
  pet.newMedal = 0;                  // o cartaz do cenario de teste taparia o bicho
  avanza(4500, 40);
  pet.newMedal = 0;
  avanza(400);
  volcaPantalla("casa_a");
  pet.hpLeft = pet.maxHp() / 3;
  avanza(200);
  volcaPantalla("casa_b");
  pet.hpLeft = pet.maxHp();
  onTap(392, 337);
  for (int k = 0; k < 4; k++) {
    avanza(620, 20);
    char n[16];
    snprintf(n, sizeof(n), "casa_c%d", k);
    volcaPantalla(n);
  }
  avanza(800);
}

// A CENA, para o dono olhar. Nao afirma nada e nao corre sem TAMA_AV=1.
TEST(preview, aventura) {
  if (!getenv("TAMA_AV")) return;
  mundoDaAventura((int16_t)atoi(getenv("TAMA_AV")));
  for (uint8_t i = 1; i < ITEM_COUNT; i++)
    if (!strcmp(ITEM_TBL[i].name, "Charizardite X")) pet.heldItem = i;
  StoredMon s{};
  s.species = getenv("TAMA_AV2") ? (int16_t)atoi(getenv("TAMA_AV2")) : (int16_t)94;
  s.exp = Pet::expForLevel(40);
  s.moves[0] = MOVE_STRUGGLE;
  s.hpLeft = 50;
  // so ele no time, para o colega da cena ser este
  for (uint8_t i = 0; i < TEAM_SLOTS; i++) if (i != pet.mainSlot) pet.team[i] = StoredMon{};
  for (uint8_t i = 0; i < BANK_SLOTS; i++) pet.bank[i] = StoredMon{};
  pet.team[pet.mainSlot == 1 ? 2 : 1] = s;
  gestosOn = false;
  mockSetEpoch(1760000000);
  pet.aventuraComeca(rtcEpoch(), 1234);
  mockSetEpoch(1760000000 + 8 * 3600);
  // a guarda do relogio do loop() desfaria o salto, com razao: ela compara com isto
  pet.lastSeenEpoch = rtcEpoch();
  for (int e = 0; e < 7; e++) {
    avFixo = (int8_t)e;
    avFixoClima = e == 3 ? (int8_t)WE_RAIN : e == 1 ? (int8_t)WE_SUN : (int8_t)WE_NONE;
    aventuraAbre();
    for (int q = 0; q < 50; q++) {
      avanza(140, 20);
      char nome[32];
      snprintf(nome, sizeof(nome), "av_%d_%02d", e, q);
      volcaPantalla(nome);
    }
  }
  AvResumo r;
  pet.aventuraFim(rtcEpoch(), r);
  avFixo = -1;
  avFixoClima = -1;
  bt.b.weather = WE_NONE;
  avOpen = false;
  mockSetEpoch(1758700000);
}

// ---------------------------------------------------------------- a troca
#include <filesystem>
// O diario e o save da troca vao para uma pasta limpa; volta a de sempre.
struct RaizSdT {
  RaizSdT() {
    std::error_code ec;
    std::filesystem::remove_all("../_claude_scratch/sd_troca_ui", ec);
    std::filesystem::create_directories("../_claude_scratch/sd_troca_ui", ec);
    mockSdRoot("../_claude_scratch/sd_troca_ui");
  }
  ~RaizSdT() { mockSdRoot("../tools/sdcard"); }
};
// Leva 49, as duas queixas do dono na primeira troca entre placas:
//  1. o TROCAR da pagina do time "tive que tentar umas 6 vezes": era
//     140x30, e abaixo de y=352 a ficha FECHAVA —— errar por um fio
//     fechava tudo;
//  2. ao parear a tela ja escolheu sozinha um de cada, e nao havia como
//     escolher outro (nem do banco).
// Leva 69: a escolha na troca e no duelo virou grade 3x2 (celula 112x106,
// passo 120x110, a partir de x=57). O centro da celula k:
static int celXT(int k) { return 57 + 56 + (k % 3) * 120; }
static int celYT(int k, int y0) { return y0 + 53 + (k / 3) * 110; }
// A altura do que foi desenhado num retangulo: as linhas onde ha algum
// pixel diferente do fundo. Mede o ICONE, nao o numero da caixa.
static int altoDaTintaT(int x, int y, int w, int h, uint16_t fundo) {
  const uint16_t *fb = gfx->getFramebuffer();
  int y0 = -1, y1 = -1;
  for (int yy = y; yy < y + h; yy++)
    for (int xx = x; xx < x + w; xx++)
      if (fb[yy * gfx->width() + xx] != fundo) {
        if (y0 < 0) y0 = yy;
        y1 = yy;
        break;
      }
  return y0 < 0 ? 0 : y1 - y0 + 1;
}
static uint16_t pixelT(int x, int y) { return gfx->getFramebuffer()[y * gfx->width() + x]; }
extern void renderTroca();

TEST(troca_ui, o_botao_trocar_aceita_o_dedo_torto_e_o_pe_ainda_fecha) {
  preparaElMundoT();
  // os cantos do desenho (133..333 x 344..384) e um pouco alem
  const int toques[][2] = { { 125, 340 }, { 340, 340 }, { 125, 383 }, { 340, 383 },
                            { 233, 337 }, { 233, 364 } };
  for (auto &t : toques) {
    todoCerradoT();
    cardOpen = true;
    cardPage = 4;
    onTap((int16_t)t[0], (int16_t)t[1]);
    CHECK_MSG(trocaOpen, "o TROCAR nao abriu tocando em (" + std::to_string(t[0]) + "," +
                         std::to_string(t[1]) + ")");
    CHECK(!cardOpen);
  }
  // entre o botao e o "tap: back": nada (nem abre, nem fecha)
  todoCerradoT();
  cardOpen = true;
  cardPage = 4;
  onTap(233, 396);
  CHECK_MSG(cardOpen && !trocaOpen, "entre o TROCAR e o pe tem de ser um vao");
  todoCerradoT();
}

TEST(troca_ui, ao_parear_abre_a_lista_e_escolher_do_banco_troca_de_verdade) {
  preparaElMundoT();
  RaizSdT raiz;   // a troca grava o diario: numa pasta de usar e tirar
  cardOpen = true;
  cardPage = 4;
  onTap(233, 364);                                   // TROCAR
  CHECK(trocaOpen);
  for (int i = 0; i < 100 && troca.nViz == 0; i++) avanza(50);
  avanza(400);   // a tela acabou de abrir: a trava de toque
  CHECK_MSG(troca.nViz > 0, "premissa: a placa fantasma nao apareceu");
  onTap(200, 112 + 20);                              // a fantasma na lista
  avanza(400);   // abriu "trocar ou batalhar?": a trava de toque
  onTap(162, 282);                                   // TROCAR
  for (int i = 0; i < 100 && troca.estado != TE_ESCOLHA; i++) avanza(50);
  CHECK_EQ((int)troca.estado, (int)TE_ESCOLHA);
  // NAO escolheu sozinha: a lista abriu e nada foi oferecido
  CHECK_MSG(trocaEscolhe, "ao parear, a lista de escolha tem de abrir");
  CHECK_MSG(!troca.tenhoMeu, "ninguem pode ser oferecido sem o jogador escolher");
  avanza(400);
  {
    // leva 69: "os icones ficam bem pequenos". A grade desenha a 2x.
    int16_t sp = 0;
    for (uint8_t i = 0; i < TEAM_SLOTS && !sp; i++) if (pet.team[i].used()) sp = pet.team[i].species;
    const uint8_t *th = thumbs.get(sp);
    CHECK_MSG(th != nullptr, "premissa: sem miniatura");
    renderTroca();
    int x = 57, y = 102;   // a 1a celula do time
    int alto = th ? altoDaTintaT(x + 24, y + 3, 64, 80, pixelT(x + 3, y + 50)) : 0;
    CHECK_MSG(th && alto >= 2 * th[1] - 4, "o icone da lista nao esta a 2x: " + std::to_string(alto) +
              " px para uma miniatura de " + std::to_string(th ? th[1] : 0));
  }
  CHECK(!troca.tenhoMeu);
  // a aba BANCO, e o terceiro da caixa
  onTap(290, 77);
  CHECK(trocaBanco);
  uint8_t terceiro = 0xFF, vistos = 0;
  for (uint8_t i = 0; i < BANK_SLOTS; i++)
    if (pet.bank[i].used() && vistos++ == 2) terceiro = i;
  CHECK_MSG(terceiro != 0xFF, "premissa: a caixa tem tres");
  int16_t saiu = pet.bank[terceiro].species;
  onTap(celXT(2), celYT(2, 130));   // a grade do banco: abaixo da barra da caixa
  CHECK_EQ((int)trocaSlot, (int)(TROCA_BANCO | terceiro));
  CHECK(troca.tenhoMeu);
  CHECK(!trocaEscolhe);
  // tocar no escolhido reabre a lista (para trocar de ideia)
  avanza(400);
  onTap(200, 110);
  CHECK(trocaEscolhe);
  CHECK(trocaBanco);                                 // volta onde estava
  avanza(400);
  onTap(233, 362);                                   // VOLTAR
  CHECK(!trocaEscolhe);
  CHECK_EQ((int)trocaSlot, (int)(TROCA_BANCO | terceiro));
  // confirma; a fantasma aceita e manda um Kadabra (chega Alakazam)
  for (int i = 0; i < 60 && !troca.podeAceitar(); i++) avanza(50);
  CHECK(troca.podeAceitar());
  avanza(400);                                       // passa a trava de toque
  onTap(98 + 64, 316 + 22);
  for (int i = 0; i < 200 && troca.estado != TE_FEITO; i++) avanza(20);
  CHECK_EQ((int)troca.estado, (int)TE_FEITO);
  // chegou evoluindo: a cena roda e o OK nao fecha no meio dela
  avanza(500);
  onTap(233, 316 + 22);
  CHECK_MSG(trocaOpen, "o OK fechou no meio da cena da evolucao");
  avanza(3200);
  {
    const uint8_t *th = thumbs.get(65);   // o Alakazam que chegou
    renderTroca();
    int alto = th ? altoDaTintaT(233 - 70, 70, 140, 140, pixelT(40, 140)) : 0;
    CHECK_MSG(th && alto >= 3 * th[1] - 6, "a evolucao da troca nao esta a 3x: " + std::to_string(alto) +
              " px para uma miniatura de " + std::to_string(th ? th[1] : 0));
  }
  onTap(233, 316 + 22);
  CHECK_MSG(!trocaOpen, "depois da cena, o OK tem de fechar");
  CHECK_EQ(pet.bank[terceiro].species, 65);          // no mesmo hueco
  int sobrou = 0;
  for (uint8_t i = 0; i < BANK_SLOTS; i++) sobrou += pet.bank[i].species == saiu;
  for (uint8_t i = 0; i < TEAM_SLOTS; i++) sobrou += pet.team[i].species == saiu;
  CHECK_EQ(sobrou, 0);
  trocaFecha();
  todoCerradoT();
}

// So quando se pede (TAMA_FOTO_TROCA=1): as telas da leva 49 para o dono
// aprovar. Nao confirma a troca, entao nada se grava no SD de verdade.
TEST(troca_ui, fotos_para_aprovar) {
  if (!getenv("TAMA_FOTO_TROCA")) return;
  preparaElMundoT();
  cardOpen = true;
  cardPage = 4;
  avanza(160);
  volcaPantalla("tr49_1_time");
  onTap(233, 364);
  for (int i = 0; i < 100 && troca.nViz == 0; i++) avanza(50);
  avanza(400);   // a tela acabou de abrir: a trava de toque
  onTap(200, 112 + 20);
  avanza(400);   // abriu "trocar ou batalhar?": a trava de toque
  onTap(162, 282);                                   // TROCAR
  for (int i = 0; i < 100 && troca.estado != TE_ESCOLHA; i++) avanza(50);
  avanza(400);
  volcaPantalla("tr49_2_lista_time");
  onTap(290, 77);
  avanza(400);
  volcaPantalla("tr49_3_lista_banco");
  onTap(celXT(2), celYT(2, 130));   // a grade do banco: abaixo da barra da caixa
  avanza(400);
  volcaPantalla("tr49_4_resumo");
  // leva 50: confirmar, a espera e a cena da evolucao
  for (int i = 0; i < 40 && !troca.podeAceitar(); i++) avanza(50);
  avanza(400);
  onTap(98 + 64, 316 + 22);
  for (int i = 0; i < 200 && troca.estado != TE_FEITO; i++) avanza(20);
  const int quadros[] = { 200, 900, 1500, 2300, 2800, 3100, 3600 };
  uint32_t t0 = millis() - trocaFeitoEm;
  for (int q = 0; q < 7; q++) {
    if ((uint32_t)quadros[q] > t0) avanza((uint32_t)quadros[q] - t0, 10);
    t0 = millis() - trocaFeitoEm;
    char nome[32];
    snprintf(nome, sizeof nome, "tr50_evo_%d", q);
    volcaPantalla(nome);
  }
  {
    std::error_code ec;
    std::filesystem::remove_all("../tools/sdcard/troca", ec);   // o diario
  }
  trocaFecha();
  todoCerradoT();
}

// O dono: "quando uma das placas deu o confirmar, a outra ja aceitou". O
// painel as vezes solta e aperta de novo no meio de um toque; o segundo
// toque caia na tela nova, e o VOLTAR/as ultimas linhas da lista ficam em
// cima do TROCAR do resumo. Escolher NAO pode virar confirmar.
TEST(troca_ui, o_toque_repetido_ao_escolher_nao_confirma_a_troca) {
  preparaElMundoT();
  RaizSdT raiz;
  cardOpen = true;
  cardPage = 4;
  onTap(233, 364);
  for (int i = 0; i < 100 && troca.nViz == 0; i++) avanza(50);
  avanza(400);   // a tela acabou de abrir: a trava de toque
  onTap(200, 112 + 20);
  avanza(400);   // abriu "trocar ou batalhar?": a trava de toque
  onTap(162, 282);                                   // TROCAR
  for (int i = 0; i < 100 && troca.estado != TE_ESCOLHA; i++) avanza(50);
  avanza(400);
  CHECK(trocaEscolhe);
  // a fantasma ja ofereceu: confirmar so depende de mim
  for (int i = 0; i < 40 && !troca.tenhoDele; i++) avanza(50);
  CHECK_MSG(troca.tenhoDele, "premissa: a fantasma nao ofereceu");
  // no banco (8 na caixa), escolhe na fila de baixo da grade (a 4a celula,
  // y 240..346), em y=318, e o dedo "quica" 40 ms depois NO MESMO
  // PONTO —— que no resumo e o botao TROCAR (98..226 x 316..360)
  onTap(290, 77);
  CHECK(trocaBanco);
  avanza(400);
  onTap(140, 318);
  CHECK(troca.tenhoMeu);
  CHECK(!trocaEscolhe);
  avanza(40);
  CHECK_MSG(troca.podeAceitar(), "premissa: o TROCAR estaria vivo");
  onTap(140, 318);
  CHECK_MSG(troca.estado == TE_ESCOLHA, "o toque repetido confirmou a troca sozinho");
  // um toque de verdade, depois, confirma
  avanza(400);
  onTap(98 + 64, 316 + 22);
  // (a fantasma ja tinha aceitado: segue direto a preparar)
  CHECK_MSG(troca.estado >= TE_ACEITEI && troca.estado <= TE_FEITO,
            "o toque de verdade nao confirmou");
  trocaFecha();
  todoCerradoT();
}

// ---------------------------------------------------------------- segurar
// O dono: "como eu faco ele segurar o item? se clico nele, ele ja evolui
// direto". O Metal Coat era so pedra (usar = evoluir o titular), segurar
// era so do titular, e o titular nao sai numa troca.
TEST(segurar, o_metal_coat_se_da_a_um_do_time_pela_bolsa) {
  preparaElMundoT();
  const uint8_t coat = itemPorNomeT("Metal Coat");
  CHECK_MSG(coat, "premissa: Metal Coat existe");
  // um Scyther no time, fora o titular
  uint8_t slot = 0xFF;
  for (uint8_t i = 0; i < TEAM_SLOTS; i++)
    if (i != pet.mainSlot && pet.team[i].used()) { slot = i; break; }
  CHECK(slot != 0xFF);
  pet.team[slot].species = 123;
  pet.team[slot].item = 0;
  int16_t titularAntes = pet.speciesId;
  uint8_t naBolsa = pet.bag[coat];
  // a bolsa, bolso das evolucoes, e o Metal Coat na lista
  cardOpen = true;
  cardPage = 3;
  bagPocket = BP_EVO;
  cardScroll = 0;
  int fila = -1;
  for (uint8_t i = 0; i < pet.pocketCount(BP_EVO); i++)
    if (pet.pocketItem(BP_EVO, i) == coat) fila = i;
  CHECK_MSG(fila >= 0, "premissa: o Metal Coat esta na bolsa");
  cardScroll = (uint8_t)(fila > 2 ? fila - 2 : 0);
  avanza(100);
  int r = fila - cardScroll;
  onTap(200, 146 + r * 34 + 17);   // CARD_ROW_Y, CARD_ROW_H do sketch
  // tocar NAO usa mais: abre a ficha (o titular nao evoluiu)
  CHECK_EQ((int)bagShown, (int)coat);
  CHECK_EQ(pet.speciesId, titularAntes);
  CHECK_EQ((int)pet.bag[coat], (int)naBolsa);
  // DAR (metade direita do botao) -> pagina do time perguntando
  onTap(300, 362 + 15);
  CHECK_EQ((int)cardPage, 4);
  CHECK_EQ((int)darItemId, (int)coat);
  avanza(100);
  onTap(200, 106 + slot * 38 + 17);
  CHECK_EQ((int)pet.team[slot].item, (int)coat);
  CHECK_EQ((int)pet.bag[coat], (int)naBolsa - 1);
  CHECK_EQ((int)darItemId, 0);
  CHECK(cardOpen);
  todoCerradoT();
}

TEST(segurar, dar_tirar_e_passar_de_um_para_outro) {
  preparaElMundoT();
  const uint8_t coat = itemPorNomeT("Metal Coat");
  const uint8_t restos = itemPorNomeT("Leftovers");
  uint8_t a = 0xFF, b = 0xFF;
  for (uint8_t i = 0; i < TEAM_SLOTS; i++)
    if (i != pet.mainSlot && pet.team[i].used()) { if (a == 0xFF) a = i; else if (b == 0xFF) b = i; }
  CHECK(a != 0xFF && b != 0xFF);
  pet.team[a].item = pet.team[b].item = 0;
  pet.bag[coat] = 1;
  pet.bag[restos] = 1;
  CHECK(pet.darItem(a, coat));
  CHECK_EQ((int)pet.bag[coat], 0);
  CHECK_EQ((int)pet.quemLeva(coat), (int)a);
  // a bolsa ainda o mostra (e por la que se tira)
  bool lista = false;
  for (uint8_t i = 0; i < pet.pocketCount(BP_EVO); i++) lista |= pet.pocketItem(BP_EVO, i) == coat;
  CHECK_MSG(lista, "o objeto que alguem segura sumiu da bolsa");
  // trocar o que ele leva: o velho volta
  CHECK(pet.darItem(a, restos));
  CHECK_EQ((int)pet.team[a].item, (int)restos);
  CHECK_EQ((int)pet.bag[coat], 1);
  CHECK(pet.darItem(a, coat));
  // passar de A para B sem ter outro na bolsa
  CHECK(pet.darItem(b, coat));
  CHECK_EQ((int)pet.team[b].item, (int)coat);
  CHECK_EQ((int)pet.team[a].item, 0);
  // o mesmo de novo: tira
  CHECK(pet.darItem(b, coat));
  CHECK_EQ((int)pet.team[b].item, 0);
  CHECK_EQ((int)pet.bag[coat], 1);
  // o titular vai pelo heldItem
  CHECK(pet.darItem(pet.mainSlot, coat));
  CHECK_EQ((int)pet.heldItem, (int)coat);
  CHECK(pet.darItem(pet.mainSlot, coat));
  CHECK_EQ((int)pet.heldItem, 0);
  // sem nenhum: nao da
  pet.bag[coat] = 0;
  CHECK(!pet.darItem(a, coat));
  todoCerradoT();
}

TEST(segurar, fotos_para_aprovar) {
  if (!getenv("TAMA_FOTO_TROCA")) return;
  preparaElMundoT();
  const uint8_t coat = itemPorNomeT("Metal Coat");
  uint8_t slot = 0xFF;
  for (uint8_t i = 0; i < TEAM_SLOTS; i++)
    if (i != pet.mainSlot && pet.team[i].used()) { slot = i; break; }
  pet.team[slot].species = 123;
  pet.team[slot].item = 0;
  cardOpen = true;
  cardPage = 3;
  bagPocket = BP_EVO;
  bagShown = coat;
  avanza(160);
  volcaPantalla("tr51_1_bolsa");
  onTap(300, 362 + 15);
  avanza(160);
  volcaPantalla("tr51_2_dar");
  onTap(200, 106 + slot * 38 + 17);
  avanza(160);
  volcaPantalla("tr51_3_segura");
  todoCerradoT();
}

// Leva 63. "os botoes de pedras use/hold tao daquele jeito horrivel de
// apertar". Eram 30 px e so o desenho pegava: um dedo um pouco alto virava
// a pagina do texto, um pouco baixo nao fazia nada. Toques tortos em volta
// do DAR tem de ser o DAR; e o "voltar" (que caia em cima dos botoes) agora
// fica abaixo da ficha e fecha.
TEST(troca_ui, os_botoes_da_ficha_do_objeto_pegam_o_dedo_torto) {
  preparaElMundoT();
  uint8_t coat = itemPorNomeT("Metal Coat");
  CHECK_MSG(coat, "premissa: Metal Coat");
  if (pet.bag[coat] == 0) pet.bag[coat] = 1;
  struct { int x, y; const char *onde; } tortos[] = {
    { 300, 348, "um pouco ACIMA do botao" },
    { 300, 416, "um pouco ABAIXO da caixa" },
    { 390, 378, "a DIREITA da caixa" },
    { 300, 378, "no meio do botao" },
  };
  for (auto &t : tortos) {
    cardOpen = true;
    cardPage = 3;
    bagPocket = BP_EVO;
    bagShown = coat;
    darItemId = 0;
    avanza(160);
    onTap(t.x, t.y);
    CHECK_MSG(cardPage == 4 && darItemId == coat,
              std::string("tocar ") + t.onde + " nao apertou o DAR");
    todoCerradoT();
  }
  // o "voltar" com a ficha aberta: pintado abaixo dela, e tocar ali fecha
  cardOpen = true;
  cardPage = 3;
  bagPocket = BP_EVO;
  bagShown = coat;
  avanza(160);
  volcaPantalla("bolsa_botoes_grandes");
  onTap(233, 440);
  CHECK_MSG(!cardOpen, "tocar o voltar abaixo da ficha nao fechou");
  todoCerradoT();
}

// Leva 52. O dono: "o banco aparece um so na hora de escolher, precisa
// mostrar todos". A lista da troca so via a caixa ABERTA (das 32).
TEST(troca_ui, a_lista_mostra_as_outras_caixas_e_a_oferta_lembra_a_sua) {
  preparaElMundoT();
  RaizSdT raiz;
  // um Pidgey sozinho na caixa 3 (a 1 e a 2 vazias); volta a 0
  CHECK(pet.boxGo(3));
  for (uint8_t i = 0; i < BANK_SLOTS; i++) pet.bank[i] = StoredMon{};
  StoredMon p{};
  p.species = 16;
  p.exp = Pet::expForLevel(7);
  p.hpLeft = 15;
  p.moves[0] = MOVE_STRUGGLE;
  pet.bank[4] = p;
  CHECK(pet.boxGo(0));
  CHECK_EQ((int)pet.boxCount(3), 1);
  cardOpen = true;
  cardPage = 4;
  onTap(233, 364);
  for (int i = 0; i < 100 && troca.nViz == 0; i++) avanza(50);
  avanza(400);
  onTap(200, 112 + 20);
  avanza(400);   // abriu "trocar ou batalhar?": a trava de toque
  onTap(162, 282);                                   // TROCAR
  for (int i = 0; i < 100 && troca.estado != TE_ESCOLHA; i++) avanza(50);
  avanza(400);
  onTap(290, 77);                                    // BANCO
  CHECK(trocaBanco);
  avanza(400);
  onTap(370, 111);                                   // a seta: proxima caixa
  CHECK_MSG(pet.boxOpen == 3, "a seta tinha de pular as vazias e ir a caixa 3");
  avanza(100);
  onTap(celXT(0), celYT(0, 130));                    // o unico da caixa 3
  CHECK_EQ((int)trocaSlot, (int)(TROCA_BANCO | 4));
  CHECK(troca.tenhoMeu);
  // espia a caixa 0 e volta SEM escolher
  avanza(400);
  onTap(200, 110);
  CHECK(trocaEscolhe);
  avanza(400);
  onTap(70, 111);                                    // seta para tras
  CHECK_EQ((int)pet.boxOpen, 0);
  avanza(400);
  onTap(233, 362);                                   // VOLTAR
  avanza(100);
  CHECK_MSG(pet.boxOpen == 3, "fora da lista a caixa aberta tinha de ser a da oferta");
  // troca de verdade: o Pidgey sai da caixa 3 e o Kadabra->Alakazam fica la
  for (int i = 0; i < 60 && !troca.podeAceitar(); i++) avanza(50);
  avanza(400);
  onTap(98 + 64, 316 + 22);
  for (int i = 0; i < 200 && troca.estado != TE_FEITO; i++) avanza(20);
  CHECK_EQ((int)troca.estado, (int)TE_FEITO);
  CHECK_EQ((int)pet.boxOpen, 3);
  CHECK_EQ(pet.bank[4].species, 65);
  avanza(4000);
  trocaFecha();
  todoCerradoT();
}

// Leva 53. O dono, dos botoes GINASIO/MEDALHAS do mapa: "dificil de
// apertar e ficar voltando". A faixa das setas descia ate y=392 e pegava
// x<112 / x>354: errar o GINASIO pela esquerda trocava para a regiao de
// antes, onde os botoes nem aparecem. O mapa de toques nao via: trocar de
// regiao tambem e um "alvo", nao fecha nada.
TEST(toques, no_mapa_errar_ginasio_ou_medalhas_nao_troca_de_regiao) {
  preparaElMundoT();
  pet.badges = 1;   // a regiao atual liberada, com mais de uma para os lados
  const int ginasio[][2] = { { 100, 380 }, { 110, 364 }, { 226, 418 }, { 170, 390 } };
  const int medalhas[][2] = { { 366, 380 }, { 356, 364 }, { 240, 418 }, { 300, 390 } };
  for (int g = 0; g < 2; g++) {
    for (auto &t : (g ? medalhas : ginasio)) {
      todoCerradoT();
      mapOpen = true;
      mapView = pet.region;
      uint8_t antes = mapView;
      onTap((int16_t)t[0], (int16_t)t[1]);
      std::string onde = "(" + std::to_string(t[0]) + "," + std::to_string(t[1]) + ")";
      CHECK_MSG(mapView == antes, "trocou de regiao tocando em " + onde);
      CHECK_MSG(cardOpen && cardPage == (g ? 7 : 5),
                std::string(g ? "MEDALHAS" : "GINASIO") + " nao abriu em " + onde);
    }
  }
  // as setas desenhadas continuam valendo (a da direita: proxima regiao)
  todoCerradoT();
  mapOpen = true;
  mapView = pet.region;
  uint8_t tope = (uint8_t)(pet.region + 1 < REGION_COUNT ? pet.region + 1 : pet.region);
  if (tope > pet.region) {
    onTap(394, 345);
    CHECK_MSG(mapView == pet.region + 1, "a seta da direita parou de funcionar");
  }
  todoCerradoT();
}

TEST(toques, foto_do_mapa_para_aprovar) {
  if (!getenv("TAMA_FOTO_TROCA")) return;
  preparaElMundoT();
  pet.badges = 1;
  mapOpen = true;
  mapView = pet.region;
  avanza(300);
  volcaPantalla("tr53_mapa");
  todoCerradoT();
}


// ================================================================ BATALHA
// Leva 57: o convite com as regras, a escolha do time e a espera, contra a
// placa fantasma do PC (que aceita e manda Kadabra, Machoke, Graveler...).
static uint32_t saveHashT() {
  uint32_t h = 2166136261u;
  h = fnvT(h, pet.team, sizeof(pet.team));
  h = fnvT(h, pet.bank, sizeof(pet.bank));
  h = fnvT(h, &pet.exp, sizeof(pet.exp));
  h = fnvT(h, &pet.hpLeft, sizeof(pet.hpLeft));
  h = fnvT(h, pet.ppLeft, sizeof(pet.ppLeft));
  h = fnvT(h, &pet.heldItem, sizeof(pet.heldItem));
  return h;
}

static void abreConectarT() {
  cardOpen = true;
  cardPage = 4;
  onTap(233, 364);                                   // CONECTAR
  CHECK(trocaOpen);
  for (int i = 0; i < 100 && troca.nViz == 0; i++) avanza(50);
  avanza(400);
  CHECK_MSG(troca.nViz > 0, "premissa: a placa fantasma nao apareceu");
  onTap(200, 112 + 20);                              // a fantasma
  avanza(400);
  CHECK_EQ((int)duPasso, 1);
}

TEST(duelo_ui, convidar_com_as_regras_escolher_e_os_times_chegam_sem_mexer_no_save) {
  preparaElMundoT();
  RaizSdT raiz;
  // dois no time: o titular e um Lapras (os outros saem)
  for (uint8_t i = 0; i < TEAM_SLOTS; i++)
    if (i != pet.mainSlot) pet.team[i] = StoredMon{};
  StoredMon l{};
  l.species = 131;
  l.exp = Pet::expForLevel(20);
  l.hpLeft = 40;
  l.moves[0] = MOVE_STRUGGLE;
  uint8_t hueco = 0xFF;
  for (uint8_t i = 0; i < TEAM_SLOTS && hueco == 0xFF; i++)
    if (!pet.team[i].used()) hueco = i;
  CHECK(hueco != 0xFF);
  pet.team[hueco] = l;
  uint32_t antes = saveHashT();
  abreConectarT();
  onTap(304, 282);                                   // BATALHAR
  avanza(400);
  CHECK_EQ((int)duPasso, 2);
  // so 2 no time: o 3v3 lembrado cai para 1v1 e o 6v6 nao se escolhe
  CHECK_EQ((int)duFormato, (int)DF_1V1);
  onTap(337, 136);                                   // 6v6 (apagado)
  CHECK_EQ((int)duFormato, (int)DF_1V1);
  onTap(233, 212);                                   // ATE 3
  CHECK_EQ((int)duItens, (int)DI_ATE3);
  onTap(162, 338);                                   // CONVIDAR
  for (int i = 0; i < 100 && troca.estado != TE_DUELO; i++) avanza(50);
  CHECK_EQ((int)troca.estado, (int)TE_DUELO);
  CHECK_EQ((int)duelo.regras, (int)duRegras(DF_1V1, DI_ATE3));
  CHECK_MSG(duelo.anfitriao, "quem convida e o lado 0");
  CHECK_EQ((int)duelo.n, 1);
  avanza(400);
  // PRONTO sem ninguem escolhido: nada
  onTap(162, 368);
  CHECK(!duelo.tenhoMeu);
  // o Lapras (a linha dele), depois o titular: o segundo nao cabe no 1v1
  uint8_t fila = 0;
  for (uint8_t i = 0; i < hueco; i++) if (pet.team[i].used()) fila++;
  onTap(celXT(fila), celYT(fila, 96));
  CHECK_EQ((int)duNEsc, 1);
  CHECK_EQ((int)duOrdem[0], (int)hueco);
  onTap(celXT(fila == 0 ? 1 : 0), celYT(fila == 0 ? 1 : 0, 96));
  CHECK_EQ((int)duNEsc, 1);
  onTap(162, 368);                                   // PRONTO
  CHECK(duelo.tenhoMeu);
  StoredMon foi;
  memcpy(&foi, duelo.meu[0], sizeof(foi));
  CHECK_EQ((int)foi.species, 131);
  for (int i = 0; i < 200 && duelo.estado != DU_BATALHA; i++) avanza(50);
  CHECK_EQ((int)duelo.estado, (int)DU_BATALHA);
  StoredMon veio;
  memcpy(&veio, duelo.dele[0], sizeof(veio));
  CHECK_EQ((int)veio.species, 64);                   // o Kadabra da fantasma
  CHECK_MSG(saveHashT() == antes, "a batalha mexeu no save");
  // O combate abre sozinho, POR CIMA da troca, ligado ao radio.
  for (int i = 0; i < 100 && !bt.open; i++) avanza(50);
  CHECK(bt.open && bt.pvp);
  CHECK_MSG(pvpRede, "o combate abriu sem o radio");
  CHECK_EQ((int)pvpEu, 0);
  for (int i = 0; i < 400 && !(pvpPonto == PT_TURNO && bt.phase == BP_CHOOSE); i++) avanza(16);
  CHECK(pvpPonto == PT_TURNO && bt.phase == BP_CHOOSE);
  // A bandeira: o 1o toque so arma (pergunta na caixa)...
  onTap(50, 252);
  CHECK_MSG(pvpDesistirAte != 0, "o 1o toque na bandeira nao armou");
  CHECK(bt.open && !pvpAcabou);
  // ...e com ela armada, tocar onde estava a carta so desarma: a caixa
  // tapa a carta, e um golpe que nao se ve nao pode sair.
  onTap(233, 377);
  CHECK_EQ((int)pvpDesistirAte, 0);
  CHECK_MSG(!(pvpTem & 1), "tocar fora da bandeira armada escolheu um golpe escondido");
  // armada de novo, e o 2o toque confirma
  uint16_t d0 = pet.duD, v0 = pet.duV, e0 = pet.duE;
  onTap(50, 252);
  onTap(50, 252);
  for (int i = 0; i < 200 && bt.open; i++) avanza(50);
  CHECK(!bt.open);
  CHECK_EQ((int)duelo.estado, (int)DU_FIM);
  CHECK_EQ((int)duelo.motivo, (int)DM_SAI);
  CHECK_MSG(pet.duD == d0 + 1 && pet.duV == v0 && pet.duE == e0,
            "desistir nao foi para o placar como derrota");
  avanza(400);
  onTap(233, 368);                                   // OK
  CHECK_EQ((int)troca.estado, (int)TE_PROCURA);
  CHECK_MSG(saveHashT() == antes, "desistir da batalha mexeu no save");
  todoCerradoT();
}

TEST(duelo_ui, o_titular_vai_como_esta_agora_e_nada_e_gravado) {
  preparaElMundoT();
  RaizSdT raiz;
  // o hueco do titular esta velho de proposito: vai o Pet vivo
  pet.team[pet.mainSlot].exp = 1;
  uint32_t antes = saveHashT();
  abreConectarT();
  onTap(304, 282);
  avanza(400);
  onTap(129, 136);                                   // 1v1
  onTap(162, 338);
  for (int i = 0; i < 100 && troca.estado != TE_DUELO; i++) avanza(50);
  avanza(400);
  uint8_t fila = 0;
  for (uint8_t i = 0; i < pet.mainSlot; i++) if (pet.team[i].used()) fila++;
  onTap(celXT(fila), celYT(fila, 96));
  onTap(162, 368);
  CHECK(duelo.tenhoMeu);
  StoredMon foi;
  memcpy(&foi, duelo.meu[0], sizeof(foi));
  CHECK_EQ((int)foi.species, (int)pet.speciesId);
  CHECK_EQ((long)foi.exp, (long)pet.exp);
  CHECK_MSG(pet.team[pet.mainSlot].exp == 1, "o hueco do titular foi gravado");
  CHECK_MSG(saveHashT() == antes, "mandar o titular mexeu no save");
  todoCerradoT();
}

TEST(duelo_ui, cancelar_na_escolha_avisa_e_volta_a_lista_e_trocar_segue_igual) {
  preparaElMundoT();
  RaizSdT raiz;
  abreConectarT();
  onTap(304, 282);
  avanza(400);
  onTap(162, 338);
  for (int i = 0; i < 100 && troca.estado != TE_DUELO; i++) avanza(50);
  avanza(400);
  onTap(304, 368);                                   // CANCELAR
  CHECK_EQ((int)troca.estado, (int)TE_PROCURA);
  CHECK_EQ((int)duelo.motivo, (int)DM_SAI);
  // e a troca continua: placa, TROCAR, pareia sem regras
  todoCerradoT();
  abreConectarT();
  onTap(162, 282);                                   // TROCAR
  for (int i = 0; i < 100 && troca.estado != TE_ESCOLHA; i++) avanza(50);
  CHECK_EQ((int)troca.estado, (int)TE_ESCOLHA);
  CHECK_EQ((int)troca.modo, 0);
  // VOLTAR na pergunta tambem volta a lista
  todoCerradoT();
  abreConectarT();
  onTap(233, 362);                                   // VOLTAR
  CHECK_EQ((int)duPasso, 0);
  CHECK_EQ((int)troca.estado, (int)TE_PROCURA);
  todoCerradoT();
}


// ================================================================ COMBATE PvP
// Leva 58: o combate entre placas pela TELA (battle_ui), sem radio. O teste
// faz as duas placas: o lado 0 toca na tela, o lado 1 decide por
// pvpDecide (e o que a rede vai fazer). Nada pode tocar no save.
extern PvpLado pvpL[2];
extern uint8_t pvpPonto, pvpPrecisa, pvpTem, pvpEu, pvpVenceu;
extern bool pvpAcabou;
extern uint16_t pvpPontos;
bool battleStartPvp(const uint8_t r0[][TROCA_MON], const uint8_t r1[][TROCA_MON],
                    uint8_t n, uint8_t eu, uint32_t semente, uint8_t regras);
void pvpDecide(uint8_t lado, const DueloDecisao &d);

static uint16_t golpeT(const char *n) {
  for (uint16_t m = 1; m < MOVE_COUNT; m++)
    if (!strcmp(MOVE_TBL[m].name, n)) return m;
  CHECK_MSG(false, std::string("golpe com nome errado: ") + n);
  return MOVE_STRUGGLE;
}

static void regPvpT(uint8_t out[TROCA_MON], int16_t sp, const char *g0,
                    const char *g1, const char *g2, const char *g3) {
  StoredMon m{};
  m.species = sp;
  m.exp = Pet::expForLevel(30);
  m.flags |= SM_MOVES_PICKED;
  const char *gs[4] = { g0, g1, g2, g3 };
  for (uint8_t i = 0; i < 4; i++) { m.moves[i] = golpeT(gs[i]); m.ppLeft[i] = 1; }
  m.hpLeft = 1;
  const uint16_t *a = ABIL_OF[sp];
  m.ability = a[0] ? a[0] : a[1];
  memcpy(out, &m, TROCA_MON);
}

struct ResumoPvpUiT {
  std::vector<uint32_t> hashes;   // pvpHash ao abrir cada ponto
  int turnos = 0, repoeQueda = 0, repoeIda = 0, trocasGusto = 0;
  int toquesGolpe = 0, toquesLista = 0;
  int toquesPerdidos = 0;         // tocou e a jogada nao entrou
  int bolsaForaDoTick = 0;        // a bolsa mudou sem o minuto do bicho
  int desobedeceu = 0;            // "won't listen", "ignores you", "instead!"
  uint8_t venceu = 0xFF;
  bool vioSono = false, vioToxico = false, vioConfusao = false, acabou = false;
};

// Um 3v3 inteiro. `passo` e o tamanho do quadro (as placas nao desenham
// no mesmo ritmo) e `ruido` gasta random() pelo meio, como a tela faz.
static void combatePvpUiT(uint32_t semente, uint32_t passo, bool ruido,
                          ResumoPvpUiT &r) {
  uint8_t a[3][TROCA_MON], b[3][TROCA_MON];
  regPvpT(a[0], 131, "THUNDER", "HYPNOSIS", "SCALD", "UTURN");
  regPvpT(a[1], 135, "THUNDER", "TOXIC", "SHADOWBALL", "BATONPASS");
  regPvpT(a[2], 59, "FLAMETHROWER", "ROAR", "CONFUSERAY", "TACKLE");
  regPvpT(b[0], 134, "SCALD", "CONFUSERAY", "ROAR", "THUNDER");
  regPvpT(b[1], 94, "SHADOWBALL", "HYPNOSIS", "TOXIC", "THUNDER");
  regPvpT(b[2], 149, "DRAGONTAIL", "FLAMETHROWER", "THUNDER", "UTURN");
  r = ResumoPvpUiT();
  CHECK(battleStartPvp(a, b, 3, 0, semente, duRegras(DF_3V3, DI_PROIBIDO)));
  uint16_t visto = 0xFFFF;
  int giro = 0;
  for (int it = 0; it < 60000 && bt.open; it++) {
    uint32_t bolsa0 = fnvT(2166136261u, pet.bag, sizeof(pet.bag));
    uint32_t idade0 = pet.ageMinutes;
    avanza(passo, passo);
    if (ruido) random(13);
    // O bicho continua vivo durante o combate: o minuto dele (Pickup,
    // medalhas) pode por coisas na bolsa, como num combate normal. O que
    // NAO pode e a bolsa mudar fora desse minuto: ai foi o combate.
    if (fnvT(2166136261u, pet.bag, sizeof(pet.bag)) != bolsa0 && pet.ageMinutes == idade0)
      r.bolsaForaDoTick++;
    if (strstr(bt.msg, "won't listen") || strstr(bt.msg, "ignores you") ||
        strstr(bt.msg, "instead!"))
      r.desobedeceu++;
    for (uint8_t s = 0; s < 2; s++) {
      r.vioSono = r.vioSono || bt.b.side[s].status == ST_SLEEP;
      r.vioToxico = r.vioToxico || bt.b.side[s].status == ST_BADPOISON;
      r.vioConfusao = r.vioConfusao || bt.b.side[s].confuseTurns > 0;
    }
    if (pvpPonto == PT_NADA) continue;
    uint8_t falta = (uint8_t)(pvpPrecisa & ~pvpTem);
    if (!falta) continue;
    if (visto != pvpPontos) {
      visto = pvpPontos;
      r.hashes.push_back(pvpHash(bt.b, pvpL));
      if (pvpPonto == PT_TURNO) r.turnos++;
    }
    // o lado 1 primeiro (a "rede"), depois o 0 pela tela
    for (int s = 1; s >= 0; s--) {
      if (!(falta & (1u << s))) continue;
      if (s == 0 && bt.phase != BP_CHOOSE && bt.phase != BP_PARTY) continue;
      DueloDecisao d;
      int8_t primeiro = -1;
      for (uint8_t i = 0; i < pvpL[s].n && primeiro < 0; i++)
        if (pvpL[s].podeEntrar(i)) primeiro = (int8_t)i;
      if (pvpPonto == PT_REPOE) {
        d.tipo = DD_TROCA;
        d.arg = (uint8_t)primeiro;
        if (bt.b.side[s].fainted()) r.repoeQueda++;
        else r.repoeIda++;
      } else if (giro % 7 == 5 && primeiro >= 0 && bt.b.canSwitchOut((uint8_t)s)) {
        d.tipo = DD_TROCA;
        d.arg = (uint8_t)primeiro;
        r.trocasGusto++;
      } else {
        const Combatant &c = bt.b.side[s];
        uint8_t nm = c.nMoves ? c.nMoves : 1;
        d.tipo = DD_GOLPE;
        d.arg = (uint8_t)((giro + s * 2) % nm);
      }
      uint8_t temAntes = pvpTem;
      uint16_t pontoAntes = pvpPontos;
      if (s == 0 && d.tipo == DD_GOLPE && bt.phase == BP_CHOOSE) {
        bt.sel = d.arg;
        onTap(233, 377);                     // o centro da carta de golpe
        r.toquesGolpe++;
      } else if (s == 0 && d.tipo == DD_TROCA) {
        if (bt.phase == BP_CHOOSE) onTap(50, 316);   // o botao do time
        onTap(233, 128 + d.arg * 44 + 18);           // a linha dele
        r.toquesLista++;
      } else {
        pvpDecide((uint8_t)s, d);
      }
      if (s == 0 && pvpPontos == pontoAntes && pvpTem == temAntes) {
        r.toquesPerdidos++;
        printf("    toque perdido: ponto %d fase %d tipo %d arg %d\n", (int)pvpPonto,
               (int)bt.phase, (int)d.tipo, (int)d.arg);
      }
    }
    giro++;
  }
  r.venceu = pvpVenceu;
  r.acabou = pvpAcabou && !bt.open;
}

TEST(pvp_ui, combate_3v3_pela_tela_acaba_e_nao_toca_no_save) {
  preparaElMundoT();
  RaizSdT raiz;
  uint32_t antes = saveHashT();
  ResumoPvpUiT r;
  randomSeed(5);
  combatePvpUiT(0xC0FFEE, 16, false, r);
  CHECK_MSG(r.acabou, "o combate nao chegou ao fim pela tela");
  CHECK_MSG(r.venceu <= 1, "ninguem ganhou");
  CHECK_MSG(!btRngLigado(), "o dado proprio ficou ligado depois do combate");
  CHECK_MSG(!bt.pvp, "bt.pvp ficou ligado");
  CHECK_MSG(saveHashT() == antes, "o combate entre placas mexeu no save (time, banco, PS, PP, objeto)");
  CHECK_MSG(r.bolsaForaDoTick == 0, "o combate mexeu na bolsa");
  // Os registros de teste tem vinculo 0: fora daqui, a desobediencia
  // mais alta que existe. Entre placas ela nao pode aparecer nunca.
  CHECK_MSG(Battle::disobeyChance(0) > 0, "premissa: vinculo 0 tem de desobedecer fora do PvP");
  CHECK_EQ(r.desobedeceu, 0);
  printf("    [pvp tela] turnos %d, quedas %d, idas %d, trocas %d, toques %d/%d, venceu %d, sono %d tox %d conf %d\n",
         r.turnos, r.repoeQueda, r.repoeIda, r.trocasGusto, r.toquesGolpe, r.toquesLista,
         (int)r.venceu, (int)r.vioSono, (int)r.vioToxico, (int)r.vioConfusao);
  CHECK_MSG(r.repoeQueda >= 1, "ninguem repos depois de cair");
  // 3v3: o perdedor repoe 2 vezes, o vencedor no maximo 2. Mais que 4 e
  // o mesmo relevo pedido duas vezes.
  CHECK_MSG(r.repoeQueda <= 4, "pediu o mesmo relevo duas vezes");
  CHECK_MSG(r.repoeIda >= 1, "nenhum U-turn/Baton Pass pediu relevo");
  CHECK_MSG(r.trocasGusto >= 1, "ninguem trocou por gosto");
  CHECK_MSG(r.toquesGolpe >= 3 && r.toquesLista >= 1, "a tela quase nao foi tocada");
  CHECK_EQ(r.toquesPerdidos, 0);
  todoCerradoT();
}

TEST(pvp_ui, o_mesmo_combate_sai_igual_com_outro_ritmo_de_tela_e_outro_ruido) {
  preparaElMundoT();
  RaizSdT raiz;
  ResumoPvpUiT r1, r2, r3;
  randomSeed(5);
  combatePvpUiT(0xBEEF, 16, false, r1);
  randomSeed(999);
  combatePvpUiT(0xBEEF, 23, true, r2);
  CHECK(r1.acabou && r2.acabou);
  CHECK_EQ((int)r1.hashes.size(), (int)r2.hashes.size());
  bool iguais = r1.hashes.size() == r2.hashes.size();
  for (size_t i = 0; i < r1.hashes.size() && i < r2.hashes.size(); i++)
    iguais = iguais && r1.hashes[i] == r2.hashes[i];
  CHECK_MSG(iguais, "a mesma semente e as mesmas jogadas deram combates diferentes");
  CHECK_EQ((int)r1.venceu, (int)r2.venceu);
  // e o dado manda: outra semente, outro combate
  combatePvpUiT(0x5EED, 16, false, r3);
  bool outro = r3.hashes.size() != r1.hashes.size();
  for (size_t i = 0; i < r1.hashes.size() && i < r3.hashes.size(); i++)
    outro = outro || r1.hashes[i] != r3.hashes[i];
  CHECK_MSG(outro, "outra semente deu o mesmo combate");
  todoCerradoT();
}

TEST(pvp_ui, desistir_acaba_e_o_outro_ganha) {
  preparaElMundoT();
  RaizSdT raiz;
  uint8_t a[1][TROCA_MON], b[1][TROCA_MON];
  regPvpT(a[0], 25, "THUNDERBOLT", "QUICKATTACK", "TACKLE", "GROWL");
  regPvpT(b[0], 1, "TACKLE", "GROWL", "VINEWHIP", "LEECHSEED");
  CHECK(battleStartPvp(a, b, 1, 0, 42, duRegras(DF_1V1, DI_PROIBIDO)));
  for (int i = 0; i < 400 && pvpPonto != PT_TURNO; i++) avanza(16);
  CHECK_EQ((int)pvpPonto, (int)PT_TURNO);
  DueloDecisao d;
  d.tipo = DD_DESISTE;
  pvpDecide(1, d);
  CHECK(bt.open);                       // ainda falta a minha
  d.tipo = DD_GOLPE;
  d.arg = 0;
  pvpDecide(0, d);
  for (int i = 0; i < 600 && bt.open; i++) avanza(16);
  CHECK(!bt.open);
  CHECK_EQ((int)pvpVenceu, 0);
  todoCerradoT();
}

// Quem age primeiro no turno: o mais rapido, e a prioridade passa na
// frente da velocidade. Os dois lados vem de pvpDecide, entao e a FILA do
// PvP (pvpSegue) que ordena, nao o btCommitTurn do jogo normal.
static uint8_t primeiroAAgirT(uint8_t slotLento) {
  uint8_t a[1][TROCA_MON], b[1][TROCA_MON];
  // lado 0: Golem (velocidade base 45); lado 1: Jolteon (130)
  regPvpT(a[0], 76, "ROCKTHROW", "QUICKATTACK", "TACKLE", "DEFENSECURL");
  regPvpT(b[0], 135, "TACKLE", "GROWL", "SANDATTACK", "TAILWHIP");
  CHECK(battleStartPvp(a, b, 1, 0, 7, duRegras(DF_1V1, DI_PROIBIDO)));
  for (int i = 0; i < 400 && pvpPonto != PT_TURNO; i++) avanza(16);
  DueloDecisao d;
  d.tipo = DD_GOLPE;
  d.arg = 0;                 // TACKLE do Jolteon
  pvpDecide(1, d);
  d.arg = slotLento;
  pvpDecide(0, d);
  uint8_t quem = bt.phase == BP_SAY ? bt.acting : 0xFF;
  // fecha sem esperar o combate
  DueloDecisao fora;
  fora.tipo = DD_DESISTE;
  for (int i = 0; i < 4000 && bt.open; i++) {
    avanza(16);
    if (pvpPonto == PT_TURNO) { pvpDecide(0, fora); pvpDecide(1, fora); }
  }
  return quem;
}

// Os dois ultimos caem juntos (Explosion): EMPATE, como no Anil
// (pbJudge: os dois lados sem ninguem -> pbDecisionOnDraw = 5). O motor
// normal dava a vitoria a quem recebeu.
TEST(pvp_ui, os_dois_ultimos_caem_juntos_e_empate_como_no_anil) {
  preparaElMundoT();
  RaizSdT raiz;
  uint8_t a[1][TROCA_MON], b[1][TROCA_MON];
  regPvpT(a[0], 76, "EXPLOSION", "EXPLOSION", "EXPLOSION", "EXPLOSION");   // Golem
  regPvpT(b[0], 129, "SPLASH", "SPLASH", "SPLASH", "SPLASH");              // Magikarp
  CHECK(battleStartPvp(a, b, 1, 0, 3, duRegras(DF_1V1, DI_PROIBIDO)));
  for (int i = 0; i < 400 && pvpPonto != PT_TURNO; i++) avanza(16);
  DueloDecisao d;
  d.tipo = DD_GOLPE;
  pvpDecide(1, d);
  pvpDecide(0, d);
  bool disseEmpate = false, caiuOs2 = false;
  for (int i = 0; i < 4000 && bt.open; i++) {
    avanza(16);
    if (bt.b.side[0].fainted() && bt.b.side[1].fainted()) caiuOs2 = true;
    if (strstr(bt.msg, "draw")) disseEmpate = true;
  }
  CHECK_MSG(caiuOs2, "premissa: a Explosion nao derrubou os dois");
  CHECK(!bt.open);
  CHECK_EQ((int)pvpVenceu, 0xFF);
  CHECK_MSG(disseEmpate, "a tela nao disse empate");
  todoCerradoT();
}

// ---- A BOLSA entre placas (leva 59b). Uma copia da minha: o save fica.
extern uint8_t pvpBolsa[ITEM_COUNT];
extern uint8_t pvpRegras;
extern DueloDecisao pvpQuer[2];
bool pvpBolsaNaRegra();
bool pvpBolsaPode();

static uint8_t itemT(const char *n) {
  for (uint8_t i = 1; i < ITEM_COUNT; i++)
    if (!strcmp(ITEM_TBL[i].name, n)) return i;
  CHECK_MSG(false, std::string("objeto com nome errado: ") + n);
  return 0;
}

// espera a minha vez no turno, com a carta na tela
static bool esperaMinhaEscolhaT() {
  for (int i = 0; i < 4000 && bt.open; i++) {
    if (pvpPonto == PT_TURNO && bt.phase == BP_CHOOSE && !(pvpTem & 1u)) return true;
    avanza(16);
  }
  return false;
}

static bool esperaMinhaEscolhaT2() {
  uint8_t bit = (uint8_t)(1u << pvpEu);
  for (int i = 0; i < 4000 && bt.open; i++) {
    if (pvpPonto == PT_TURNO && bt.phase == BP_CHOOSE && !(pvpTem & bit)) return true;
    avanza(16);
  }
  return false;
}

// o lado 1 (sem tela) faz o golpe 0
static void outroGolpeT() {
  DueloDecisao d;
  d.tipo = DD_GOLPE;
  d.arg = 0;
  pvpDecide(1, d);
}

TEST(pvp_ui, com_a_bolsa_proibida_o_botao_nao_abre) {
  preparaElMundoT();
  RaizSdT raiz;
  memset(pet.bag, 0, sizeof(pet.bag));
  pet.bag[itemT("Potion")] = 5;
  uint8_t a[1][TROCA_MON], b[1][TROCA_MON];
  regPvpT(a[0], 76, "TACKLE", "TACKLE", "TACKLE", "TACKLE");
  regPvpT(b[0], 129, "SPLASH", "SPLASH", "SPLASH", "SPLASH");
  CHECK(battleStartPvp(a, b, 1, 0, 5, duRegras(DF_1V1, DI_PROIBIDO)));
  CHECK(esperaMinhaEscolhaT());
  CHECK(!pvpBolsaNaRegra());
  onTap(400, 296);                         // onde fica a bolsa
  CHECK_EQ((int)bt.phase, (int)BP_CHOOSE);
  CHECK(!(pvpTem & 1u));
  DueloDecisao fora;
  fora.tipo = DD_DESISTE;
  pvpDecide(0, fora);
  pvpDecide(1, fora);
  for (int i = 0; i < 600 && bt.open; i++) avanza(16);
  todoCerradoT();
}

// "Ate 3": a Pocao vira a minha jogada, cura NO TURNO (antes do golpe do
// outro), gasta da copia e nao do save; a 4a o botao nega.
TEST(pvp_ui, ate_3_a_pocao_cura_no_turno_gasta_a_copia_e_o_save_fica) {
  preparaElMundoT();
  RaizSdT raiz;
  memset(pet.bag, 0, sizeof(pet.bag));
  uint8_t pot = itemT("Potion");
  pet.bag[pot] = 5;
  uint32_t antes = saveHashT();
  uint8_t a[1][TROCA_MON], b[1][TROCA_MON];
  regPvpT(a[0], 76, "DEFENSECURL", "DEFENSECURL", "DEFENSECURL", "DEFENSECURL");   // Golem
  regPvpT(b[0], 129, "SPLASH", "SPLASH", "SPLASH", "SPLASH");                      // Magikarp
  CHECK(battleStartPvp(a, b, 1, 0, 9, duRegras(DF_1V1, DI_ATE3)));
  for (uint8_t vez = 0; vez < 3; vez++) {
    CHECK(esperaMinhaEscolhaT());
    Combatant &g = bt.b.side[0];
    g.hp = (uint16_t)(g.hpMax - 30);       // machucado, para a Pocao servir
    uint16_t hp0 = g.hp;
    onTap(400, 296);                       // a bolsa
    CHECK_EQ((int)bt.phase, (int)BP_BAG);
    onTap(233, 164 + 17);                  // a 1a fila: a Pocao
    CHECK_EQ((int)bt.phase, (int)BP_PARTY);
    CHECK_EQ((int)bt.itemPick, (int)pot);
    onTap(233, 128 + 18);                  // o Golem, na lista
    CHECK_MSG(pvpTem & 1u, "tocar o Golem nao virou a jogada");
    CHECK_EQ((int)bt.b.side[0].hp, (int)hp0);   // ainda nao: so no turno
    outroGolpeT();
    bool disse = false;
    for (int i = 0; i < 4000 && bt.open && !(pvpPonto == PT_TURNO && !(pvpTem & 1u)); i++) {
      avanza(16);
      if (strstr(bt.msg, "Used Potion on GOLEM!")) disse = true;
    }
    CHECK_MSG(disse, "a tela nao disse a Pocao");
    CHECK_EQ((int)bt.b.side[0].hp, (int)hp0 + 20);
    CHECK_EQ((int)pvpL[0].itensUsados, (int)vez + 1);
    CHECK_EQ((int)pvpBolsa[pot], 5 - (int)vez - 1);
    CHECK_EQ((int)pet.bag[pot], 5);
  }
  // as 3 foram: a bolsa ja nao abre
  CHECK(esperaMinhaEscolhaT());
  CHECK(!pvpBolsaPode());
  onTap(400, 296);
  CHECK_EQ((int)bt.phase, (int)BP_CHOOSE);
  DueloDecisao fora;
  fora.tipo = DD_DESISTE;
  pvpDecide(0, fora);
  pvpDecide(1, fora);
  for (int i = 0; i < 600 && bt.open; i++) avanza(16);
  CHECK(!bt.open);
  CHECK_MSG(saveHashT() == antes, "usar a bolsa entre placas mexeu no save");
  todoCerradoT();
}

// Os 60 s acabam com a bolsa aberta: ela fecha e a placa escolhe o golpe.
TEST(pvp_ui, o_tempo_acaba_com_a_bolsa_aberta_e_ela_fecha) {
  preparaElMundoT();
  RaizSdT raiz;
  memset(pet.bag, 0, sizeof(pet.bag));
  pet.bag[itemT("Potion")] = 1;
  uint8_t a[1][TROCA_MON], b[1][TROCA_MON];
  regPvpT(a[0], 76, "TACKLE", "TACKLE", "TACKLE", "TACKLE");
  regPvpT(b[0], 129, "SPLASH", "SPLASH", "SPLASH", "SPLASH");
  CHECK(battleStartPvp(a, b, 1, 0, 3, duRegras(DF_1V1, DI_LIVRE)));
  CHECK(esperaMinhaEscolhaT());
  onTap(400, 296);
  CHECK_EQ((int)bt.phase, (int)BP_BAG);
  for (int i = 0; i < 5000 && !(pvpTem & 1u); i++) avanza(16);
  CHECK_MSG(pvpTem & 1u, "o tempo acabou e a placa nao escolheu");
  CHECK_EQ((int)pvpQuer[0].tipo, (int)DD_GOLPE);
  CHECK(bt.phase != BP_BAG);
  CHECK_EQ((int)bt.itemPick, 0);
  DueloDecisao fora;
  fora.tipo = DD_DESISTE;
  pvpDecide(1, fora);
  for (int i = 0; i < 4000 && bt.open; i++) {
    avanza(16);
    if (pvpPonto == PT_TURNO) { pvpDecide(0, fora); pvpDecide(1, fora); }
  }
  todoCerradoT();
}

// O Revive vai num CAIDO do banco (nos de pe a lista nega) e ele volta a
// poder entrar. Com a regra livre.
TEST(pvp_ui, o_revive_levanta_um_caido_do_banco_e_ele_volta_a_poder_entrar) {
  preparaElMundoT();
  RaizSdT raiz;
  memset(pet.bag, 0, sizeof(pet.bag));
  uint8_t rev = itemT("Revive");
  pet.bag[rev] = 1;
  uint8_t a[3][TROCA_MON], b[3][TROCA_MON];
  regPvpT(a[0], 76, "DEFENSECURL", "DEFENSECURL", "DEFENSECURL", "DEFENSECURL");
  regPvpT(a[1], 131, "SURF", "SURF", "SURF", "SURF");
  regPvpT(a[2], 135, "TACKLE", "TACKLE", "TACKLE", "TACKLE");
  for (uint8_t i = 0; i < 3; i++) regPvpT(b[i], 129, "SPLASH", "SPLASH", "SPLASH", "SPLASH");
  CHECK(battleStartPvp(a, b, 3, 0, 13, duRegras(DF_3V3, DI_LIVRE)));
  CHECK(esperaMinhaEscolhaT());
  pvpL[0].mon[1].hp = 0;                   // o Lapras caiu antes
  CHECK(!pvpL[0].podeEntrar(1));
  onTap(400, 296);
  onTap(233, 164 + 17);                    // o Revive
  CHECK_EQ((int)bt.phase, (int)BP_PARTY);
  onTap(233, 128 + 2 * 44 + 18);           // o Jolteon, de pe: nao
  CHECK_MSG(!(pvpTem & 1u), "o Revive entrou num que esta de pe");
  CHECK_EQ((int)bt.phase, (int)BP_PARTY);
  onTap(233, 128 + 1 * 44 + 18);           // o Lapras caido
  CHECK(pvpTem & 1u);
  outroGolpeT();
  bool disse = false;
  for (int i = 0; i < 4000 && bt.open && !(pvpPonto == PT_TURNO && !(pvpTem & 1u)); i++) {
    avanza(16);
    if (strstr(bt.msg, "Used Revive on LAPRAS!")) disse = true;
  }
  CHECK_MSG(disse, "a tela nao disse o Revive");
  CHECK_EQ((int)pvpL[0].mon[1].hp, (int)(pvpL[0].mon[1].hpMax / 2));
  CHECK(pvpL[0].podeEntrar(1));
  CHECK_EQ((int)pvpBolsa[rev], 0);
  CHECK_EQ((int)pet.bag[rev], 1);
  DueloDecisao fora;
  fora.tipo = DD_DESISTE;
  CHECK(esperaMinhaEscolhaT());
  pvpDecide(0, fora);
  pvpDecide(1, fora);
  for (int i = 0; i < 600 && bt.open; i++) avanza(16);
  todoCerradoT();
}

// A ordem do Anil (pbAttackPhase): o objeto vem ANTES dos golpes, mesmo
// do mais lento. Aqui o outro lado (Magikarp, mais lento que o Jolteon)
// usa X Attack, e a tela diz "Foe used".
TEST(pvp_ui, o_objeto_do_outro_vem_antes_do_golpe_do_mais_rapido) {
  preparaElMundoT();
  RaizSdT raiz;
  uint8_t xat = itemT("X Attack");
  uint8_t a[1][TROCA_MON], b[1][TROCA_MON];
  regPvpT(a[0], 135, "GROWL", "GROWL", "GROWL", "GROWL");     // Jolteon
  regPvpT(b[0], 129, "SPLASH", "SPLASH", "SPLASH", "SPLASH"); // Magikarp
  CHECK(battleStartPvp(a, b, 1, 0, 21, duRegras(DF_1V1, DI_ATE3)));
  CHECK(esperaMinhaEscolhaT());
  CHECK_MSG(bt.b.movesFirst(0, 0, 0), "premissa: o Jolteon nao e o mais rapido");
  DueloDecisao it;
  it.tipo = DD_ITEM;
  it.arg = xat;
  it.extra = 0;
  pvpDecide(1, it);
  DueloDecisao g;
  g.tipo = DD_GOLPE;
  g.arg = 0;
  pvpDecide(0, g);
  int ordemItem = -1, ordemGolpe = -1, n = 0;
  char ultima[44] = { 0 };
  for (int i = 0; i < 4000 && bt.open && !(pvpPonto == PT_TURNO && !(pvpTem & 1u)); i++) {
    avanza(16);
    if (bt.msg[0] && strcmp(bt.msg, ultima)) {
      snprintf(ultima, sizeof(ultima), "%s", bt.msg);
      n++;
      if (ordemItem < 0 && strstr(bt.msg, "Foe used X Attack on MAGIKARP!")) ordemItem = n;
      if (ordemGolpe < 0 && strstr(bt.msg, "GROWL")) ordemGolpe = n;
    }
  }
  CHECK_MSG(ordemItem > 0, "a tela nao disse o X Attack do outro");
  CHECK_MSG(ordemGolpe > 0, "premissa: o Growl nao apareceu");
  CHECK_MSG(ordemItem < ordemGolpe, "o objeto veio depois do golpe");
  // +2 do X Attack e -1 do Growl
  CHECK_EQ((int)bt.b.side[1].stage[SG_ATK], 1);
  CHECK_EQ((int)pvpL[1].itensUsados, 1);
  DueloDecisao fora;
  fora.tipo = DD_DESISTE;
  pvpDecide(0, fora);
  pvpDecide(1, fora);
  for (int i = 0; i < 600 && bt.open; i++) avanza(16);
  todoCerradoT();
}

// ---- A CONVIDADA desenha espelhado (leva 60): o combate e o mesmo (o
// anfitriao e o lado 0), mas o lado dela fica EMBAIXO.
extern int16_t foePmdTried;   // o que tentou carregar (no PC o SD nao tem o sprite)
void radioFantasmaConvida(uint8_t regras);

// O que se espera em foePmdFor para o bicho de um lado (sem formas).
static int16_t spriteEsperadoT(const Combatant &c) {
  return c.shiny ? (int16_t)(c.asDex + DEX_COUNT + 1) : c.asDex;
}

// pixels da cor exata numa faixa de linhas (o preenchimento da barra de PS
// e y+18..y+26 de btHpBar: so ele, nao os chips de estado ao lado)
static int corNaFaixaT(uint16_t cor, int y0, int y1) {
  const uint16_t *fb = gfx->getFramebuffer();
  int n = 0;
  for (int y = y0; y < y1; y++)
    for (int x = 0; x < gfx->width(); x++)
      if (fb[y * gfx->width() + x] == cor) n++;
  return n;
}

TEST(pvp_ui, a_convidada_ve_o_lado_dela_embaixo_e_a_carta_e_a_dela) {
  preparaElMundoT();
  RaizSdT raiz;
  uint8_t a[1][TROCA_MON], b[1][TROCA_MON];
  regPvpT(a[0], 76, "TACKLE", "TACKLE", "TACKLE", "TACKLE");     // Golem: o anfitriao
  regPvpT(b[0], 129, "SPLASH", "SPLASH", "SPLASH", "SPLASH");    // Magikarp: eu
  CHECK(battleStartPvp(a, b, 1, 1, 4, duRegras(DF_1V1, DI_PROIBIDO)));
  CHECK(esperaMinhaEscolhaT2());
  avanza(200);
  // em cima (de frente) o Golem do anfitriao
  CHECK_EQ((int)foePmdTried, (int)spriteEsperadoT(bt.b.side[0]));
  // o MEU a 10%: a barra de baixo fica vermelha e a de cima nao
  bt.b.side[1].hp = (uint16_t)(bt.b.side[1].hpMax / 10);
  bt.b.side[0].hp = bt.b.side[0].hpMax;
  avanza(200);
  int baixo = corNaFaixaT(0xEA87 /* UI_BAR_BAD */, 228 + 18, 228 + 26);
  int cima = corNaFaixaT(0xEA87 /* UI_BAR_BAD */, 68 + 18, 68 + 26);
  printf("    [espelho] vermelho embaixo %d, em cima %d\n", baixo, cima);
  CHECK_MSG(baixo > 20, "a barra de baixo nao e a do meu lado");
  CHECK_MSG(cima == 0, "a barra de cima mostra o meu lado");
  // a carta: o toque decide pelo LADO 1
  onTap(233, 377);
  CHECK_MSG(pvpTem & 2u, "o toque na carta nao decidiu pelo meu lado");
  CHECK(!(pvpTem & 1u));
  CHECK_EQ((int)pvpQuer[1].tipo, (int)DD_GOLPE);
  DueloDecisao fora;
  fora.tipo = DD_DESISTE;
  pvpDecide(0, fora);
  for (int i = 0; i < 4000 && bt.open; i++) {
    avanza(16);
    if (pvpPonto == PT_TURNO) { pvpDecide(0, fora); pvpDecide(1, fora); }
  }
  CHECK(!bt.open);
  todoCerradoT();
}

TEST(pvp_ui, o_mais_rapido_age_primeiro_e_a_prioridade_passa_na_frente) {
  preparaElMundoT();
  RaizSdT raiz;
  CHECK_EQ((int)primeiroAAgirT(0), 1);   // Rock Throw do Golem: o Jolteon antes
  CHECK_EQ((int)primeiroAAgirT(1), 0);   // Quick Attack (+1): o Golem antes
  CHECK(!bt.open);
  todoCerradoT();
}


// ================================================================ PvP PELA REDE
// Leva 59: o combate inteiro pela tela da troca, contra a placa fantasma
// (troca_radio.cpp), que joga o lado dela lendo o ponto aberto aqui. A
// jogada de cada lado viaja pelo Duelo com o hash do combate.

// Tres no time: o titular, um Lapras e um Jolteon (nivel 30, golpes do
// nivel). O resto sai.
static void timeDeTresT() {
  for (uint8_t i = 0; i < TEAM_SLOTS; i++)
    if (i != pet.mainSlot) pet.team[i] = StoredMon{};
  const int16_t esp[2] = { 131, 135 };
  uint8_t pos = 0;
  for (uint8_t i = 0; i < TEAM_SLOTS && pos < 2; i++) {
    if (i == pet.mainSlot) continue;
    StoredMon m{};
    m.species = esp[pos++];
    m.exp = Pet::expForLevel(30);
    m.hpLeft = 50;
    // golpes escolhidos e fortes, o 1o (o que a carta marca) de proposito:
    // Surf no Graveler, Shadow Ball no Kadabra -- a fantasma tem de cair
    m.flags |= SM_MOVES_PICKED;
    const char *g[2][4] = { { "SURF", "ICEBEAM", "BODYSLAM", "PSYCHIC" },
                            { "SHADOWBALL", "THUNDERBOLT", "PINMISSILE", "QUICKATTACK" } };
    for (uint8_t k = 0; k < 4; k++) { m.moves[k] = golpeT(g[pos - 1][k]); m.ppLeft[k] = 5; }
    pet.team[i] = m;
  }
}

// CONECTAR -> a fantasma -> BATALHAR -> formato -> CONVIDAR -> os n
// primeiros da lista -> PRONTO, e espera o combate abrir.
static void entraNoCombateRedeT(int xFormato, uint8_t n) {
  abreConectarT();
  onTap(304, 282);                                   // BATALHAR
  avanza(400);
  onTap(xFormato, 136);                              // 1v1 (129) / 3v3 (233)
  onTap(162, 338);                                   // CONVIDAR
  for (int i = 0; i < 100 && troca.estado != TE_DUELO; i++) avanza(50);
  avanza(400);
  for (uint8_t i = 0; i < n; i++) onTap(celXT(i), celYT(i, 96));
  CHECK_EQ((int)duNEsc, (int)n);
  onTap(162, 368);                                   // PRONTO
  for (int i = 0; i < 200 && !bt.open; i++) avanza(50);
  CHECK_MSG(bt.open && bt.pvp && pvpRede, "o combate pela rede nao abriu");
}

// e a minha vez de escolher, e a tela esta pedindo
static bool minhaVezT() {
  uint8_t bit = (uint8_t)(1u << pvpEu);
  return bt.open && bt.pvp && pvpPonto != PT_NADA && (pvpPrecisa & bit) &&
         !(pvpTem & bit) && (bt.phase == BP_CHOOSE || bt.phase == BP_PARTY);
}

static void jogaPelaTelaT() {
  if (bt.phase == BP_CHOOSE) {
    onTap(233, 377);                                 // a carta de golpe
    return;
  }
  for (uint8_t i = 0; i < pvpL[pvpEu].n; i++)
    if (pvpL[pvpEu].podeEntrar(i)) { onTap(233, 128 + i * 44 + 18); return; }
}

static void avanzaAteT(uint32_t quando) {
  while ((int32_t)(millis() - quando) < 0) avanza(16);
}

static void desisteT() {
  for (int i = 0; i < 4000 && bt.open && !(pvpPonto == PT_TURNO && bt.phase == BP_CHOOSE &&
                                           !(pvpTem & (1u << pvpEu))); i++)
    avanza(16);
  if (!bt.open) return;
  onTap(50, 252);
  onTap(50, 252);
  for (int i = 0; i < 400 && bt.open; i++) avanza(16);
}

// Cada ponto que abre, pelo gancho: um ponto em que so a fantasma repoe
// abre e fecha dentro do mesmo quadro (mando NADA, ela responde), e olhar
// o estado entre quadros nunca o via.
static int gPontosT = 0, gSoElaT = 0, gSoEuT = 0;
static void contaPontoT() {
  gPontosT++;
  if (!(pvpPrecisa & (1u << pvpEu))) gSoElaT++;
  if (pvpPrecisa == (uint8_t)(1u << pvpEu)) gSoEuT++;
}

// A Pocao pela rede: a jogada DD_ITEM vai pelo Duelo, o ponto fecha com
// as duas e a cura entra. Regra "ate 3" escolhida no convite.
TEST(pvp_rede, a_pocao_vai_pela_rede_e_cura_sem_mexer_no_save) {
  preparaElMundoT();
  RaizSdT raiz;
  timeDeTresT();
  memset(pet.bag, 0, sizeof(pet.bag));
  uint8_t pot = itemT("Super Potion");
  pet.bag[pot] = 2;
  uint32_t antes = saveHashT();
  randomSeed(5);
  duItens = DI_ATE3;
  entraNoCombateRedeT(233, 3);
  duItens = DI_PROIBIDO;
  CHECK_EQ((int)DR_ITENS(pvpRegras), (int)DI_ATE3);
  for (int i = 0; i < 4000 && !minhaVezT(); i++) avanza(16);
  CHECK(minhaVezT() && pvpPonto == PT_TURNO);
  Combatant &c = bt.b.side[pvpEu];
  c.hp = (uint16_t)(c.hpMax / 3);
  uint16_t hp0 = c.hp;
  onTap(400, 296);
  onTap(233, 164 + 17);
  onTap(233, 128 + pvpL[pvpEu].ativo * 44 + 18);
  CHECK(pvpTem & (1u << pvpEu));
  bool disse = false;
  uint16_t hpNaHora = 0;
  uint16_t k = pvpPontos;
  for (int i = 0; i < 4000 && bt.open && pvpPontos == k; i++) avanza(16);
  for (int i = 0; i < 300 && bt.open && !disse; i++) {
    avanza(16);
    if (strstr(bt.msg, "Used Super Potion on")) { disse = true; hpNaHora = bt.b.side[pvpEu].hp; }
  }
  CHECK_MSG(disse, "a Pocao pela rede nao entrou");
  CHECK(!pvpCortado);
  CHECK_EQ((int)hpNaHora, (int)(hp0 + 60 > c.hpMax ? c.hpMax : hp0 + 60));
  CHECK_EQ((int)pvpL[pvpEu].itensUsados, 1);
  desisteT();
  CHECK(!bt.open);
  CHECK_MSG(saveHashT() == antes, "a bolsa pela rede mexeu no save");
  todoCerradoT();
}

// A fantasma CONVIDA: eu sou a convidada (lado 1). O 3v3 vai ate o fim
// pela tela, com o meu lado embaixo o combate inteiro, e o save fica.
TEST(pvp_rede, como_convidada_o_3v3_vai_ate_o_fim_com_o_meu_lado_embaixo) {
  preparaElMundoT();
  RaizSdT raiz;
  timeDeTresT();
  uint32_t antes = saveHashT();
  randomSeed(17);
  abreConectarT();
  avanza(400);
  radioFantasmaConvida(duRegras(DF_3V3, DI_PROIBIDO));
  for (int i = 0; i < 100 && troca.estado != TE_CONVIDADO; i++) avanza(50);
  CHECK_EQ((int)troca.estado, (int)TE_CONVIDADO);
  avanza(800);                                       // a tela acabou de mudar: trava curta
  onTap(162, 338);                                   // SIM
  for (int i = 0; i < 100 && troca.estado != TE_DUELO; i++) avanza(50);
  avanza(400);
  for (uint8_t i = 0; i < 3; i++) onTap(celXT(i), celYT(i, 96));
  CHECK_EQ((int)duNEsc, 3);
  if (getenv("TAMA_FOTO_TROCA")) { avanza(100); volcaPantalla("tr69_duelo"); }
  onTap(162, 368);                                   // PRONTO
  for (int i = 0; i < 200 && !bt.open; i++) avanza(50);
  CHECK_MSG(bt.open && bt.pvp && pvpRede, "o combate como convidada nao abriu");
  CHECK_EQ((int)pvpEu, 1);
  int minhas = 0, vezesOlhadas = 0, spriteErrado = 0, barraErrada = 0;
  for (int it = 0; it < 60000 && bt.open; it++) {
    avanza(16);
    if (minhaVezT()) {
      if (bt.phase == BP_CHOOSE) {
        // a cada vez minha: em cima o anfitriao; a barra de baixo e a minha
        avanza(120);
        if (!minhaVezT() || bt.phase != BP_CHOOSE) continue;
        vezesOlhadas++;
        if (foePmdTried != spriteEsperadoT(bt.b.side[0])) spriteErrado++;
        uint16_t guarda = bt.b.side[1].hp, guardaEle = bt.b.side[0].hp;
        bt.b.side[1].hp = (uint16_t)(bt.b.side[1].hpMax / 10);
        bt.b.side[0].hp = bt.b.side[0].hpMax;
        avanza(100);
        if (corNaFaixaT(0xEA87 /* UI_BAR_BAD */, 228 + 18, 228 + 26) <= 20 || corNaFaixaT(0xEA87 /* UI_BAR_BAD */, 68 + 18, 68 + 26) != 0)
          barraErrada++;
        bt.b.side[1].hp = guarda;
        bt.b.side[0].hp = guardaEle;
        if (!minhaVezT()) continue;
      }
      jogaPelaTelaT();
      minhas++;
    }
  }
  printf("    [convidada] jogadas %d, olhadas %d, sprite errado %d, barra errada %d, resultado %d\n",
         minhas, vezesOlhadas, spriteErrado, barraErrada, (int)duResultado);
  CHECK_MSG(!bt.open, "o combate como convidada nao acabou");
  CHECK_EQ((int)duelo.motivo, (int)DM_FIM);
  CHECK_MSG(!pvpCortado, "acabou cortado (dessincronia?) e nao pelo combate");
  CHECK_MSG(vezesOlhadas >= 3, "premissa: poucas vezes minhas para olhar a tela");
  CHECK_EQ(spriteErrado, 0);
  CHECK_EQ(barraErrada, 0);
  CHECK_MSG(saveHashT() == antes, "o combate como convidada mexeu no save");
  avanza(400);

  avanza(400);
  todoCerradoT();
}

TEST(pvp_rede, um_3v3_contra_a_fantasma_vai_ate_o_fim_e_o_save_fica) {
  preparaElMundoT();
  RaizSdT raiz;
  timeDeTresT();
  uint32_t antes = saveHashT();
  randomSeed(11);
  entraNoCombateRedeT(233, 3);
  CHECK_EQ((int)pvpEu, 0);
  uint16_t pv[3] = { pet.duV, pet.duD, pet.duE };
  gPontosT = gSoElaT = gSoEuT = 0;
  pvpAoAbrir = contaPontoT;
  int minhas = 0;
  for (int it = 0; it < 60000 && bt.open; it++) {
    avanza(16);
    if (minhaVezT()) { jogaPelaTelaT(); minhas++; }
  }
  pvpAoAbrir = nullptr;
  int pontos = gPontosT, soEla = gSoElaT, soEu = gSoEuT;
  printf("    [pvp rede] pontos %d, so ela repos %d, so eu %d, jogadas minhas %d, resultado %d\n",
         pontos, soEla, soEu, minhas, (int)duResultado);
  CHECK_MSG(!bt.open, "o combate pela rede nao acabou");
  CHECK_EQ((int)duelo.estado, (int)DU_FIM);
  CHECK_EQ((int)duelo.motivo, (int)DM_FIM);
  CHECK_MSG(!pvpCortado, "acabou cortado (sumiu/dessincronia) e nao pelo combate");
  CHECK(duResultado <= 2);
  CHECK(pontos >= 4);
  {
    uint16_t agora[3] = { pet.duV, pet.duD, pet.duE };
    for (uint8_t k = 0; k < 3; k++)
      CHECK_EQ((int)agora[k], (int)pv[k] + (k == duResultado ? 1 : 0));
  }
  // o NADA: num ponto em que so a fantasma repoe, eu tenho de dizer NADA
  // pela rede, senao o Duelo nao passa ao ponto seguinte
  CHECK_MSG(soEla >= 1, "premissa: nenhum ponto em que so a fantasma repos");
  CHECK_MSG(saveHashT() == antes, "o combate pela rede mexeu no save");
  // a tela do resultado, por baixo, e o OK volta a lista
  avanza(400);
  {
    // e so UMA vez: com a tela do resultado aberta o placar nao segue somando
    uint16_t agora[3] = { pet.duV, pet.duD, pet.duE };
    for (uint8_t k = 0; k < 3; k++)
      CHECK_EQ((int)agora[k], (int)pv[k] + (k == duResultado ? 1 : 0));
  }
  const uint16_t *fb = gfx->getFramebuffer();
  uint16_t fondo = fb[140 * gfx->width() + 233];
  CHECK_MSG(pixelesEn(90, 180, 286, 44, fondo) > 60, "a tela do resultado esta vazia");
  onTap(233, 368);
  avanza(200);
  CHECK_EQ((int)troca.estado, (int)TE_PROCURA);
  todoCerradoT();
}

TEST(pvp_rede, sem_tocar_a_placa_escolhe_aos_60s_e_conta_os_ultimos_10) {
  preparaElMundoT();
  RaizSdT raiz;
  timeDeTresT();
  randomSeed(12);
  entraNoCombateRedeT(129, 1);
  for (int i = 0; i < 4000 && !minhaVezT(); i++) avanza(16);
  CHECK(minhaVezT());
  CHECK(bt.phase == BP_CHOOSE);
  uint32_t abriu = pvpAbertoEm;
  uint16_t ponto = pvpPontos;
  // premissa: sem o relogio, o alto da tela fica parado
  avanzaAteT(abriu + 44000);
  uint32_t a = huellaDe(212, 19, 43, 43);
  avanzaAteT(abriu + 45000);
  uint32_t b = huellaDe(212, 19, 43, 43);
  CHECK_MSG(a == b, "premissa: o alto da tela muda sozinho (nao da para ver o relogio)");
  avanzaAteT(abriu + 52000);
  CHECK(minhaVezT());
  CHECK_MSG(huellaDe(212, 19, 43, 43) != b, "os ultimos 10 s nao aparecem na tela");
  avanzaAteT(abriu + 59500);
  CHECK_MSG(minhaVezT(), "a placa escolheu antes dos 60 s");
  avanzaAteT(abriu + 60600);
  CHECK_MSG(pvpPontos != ponto || (pvpTem & (1u << pvpEu)), "aos 60 s ninguem escolheu por mim");
  CHECK_MSG(!pvpCortado && duelo.estado == DU_BATALHA, "o tempo acabou o combate");
  desisteT();
  CHECK(!bt.open);
  todoCerradoT();
}

TEST(pvp_rede, a_outra_some_no_meio_e_o_combate_fecha_sem_mexer_no_save) {
  preparaElMundoT();
  RaizSdT raiz;
  timeDeTresT();
  uint32_t antes = saveHashT();
  randomSeed(13);
  entraNoCombateRedeT(129, 1);
  for (int i = 0; i < 4000 && !minhaVezT(); i++) avanza(16);
  CHECK(minhaVezT());
  uint32_t placar0 = (uint32_t)pet.duV + pet.duD + pet.duE;
  radioFantasmaCala(true);
  jogaPelaTelaT();
  uint32_t t0 = millis();
  for (int i = 0; i < 4000 && bt.open; i++) avanza(16);
  CHECK_MSG(!bt.open, "a outra sumiu e o combate ficou aberto");
  CHECK_MSG(millis() - t0 >= 19000, "deu a outra por sumida cedo demais");
  CHECK_EQ((int)duelo.motivo, (int)DM_SUMIU);
  CHECK(pvpCortado);
  CHECK_EQ((int)duResultado, 0xFF);
  CHECK_MSG((uint32_t)pet.duV + pet.duD + pet.duE == placar0, "a queda do radio foi para o placar");
  CHECK_MSG(saveHashT() == antes, "perder a outra placa mexeu no save");
  radioFantasmaCala(false);
  todoCerradoT();
}

TEST(pvp_rede, se_as_placas_veem_combates_diferentes_acaba_em_dessincronia) {
  preparaElMundoT();
  RaizSdT raiz;
  timeDeTresT();
  uint32_t antes = saveHashT();
  randomSeed(14);
  entraNoCombateRedeT(129, 1);
  radioFantasmaHashErrado(true);
  for (int i = 0; i < 4000 && !minhaVezT(); i++) avanza(16);
  CHECK(minhaVezT());
  jogaPelaTelaT();
  for (int i = 0; i < 1000 && bt.open; i++) avanza(16);
  CHECK(!bt.open);
  CHECK_EQ((int)duelo.motivo, (int)DM_DESSINCRONIA);
  CHECK(pvpCortado);
  CHECK_MSG(saveHashT() == antes, "a dessincronia mexeu no save");
  radioFantasmaHashErrado(false);
  todoCerradoT();
}

TEST(interfaz, a_fuga_pede_confirmacao_e_o_toque_duplo_nao_confirma) {
  // Leva 66. O botao de escapada soltava o bicho com UM toque, e o botao
  // (y 176..234) pisa o primeiro do dialogo (206..258): um toque duplo
  // sem querer confirmava qualquer coisa. Agora: dialogo, 600 ms surdo,
  // FICAR em cima e DEIXAR IR em baixo.
  arranca();
  todoCerradoT();
  if (pet.awaitingStarter()) pet.chooseStarter(1);
  while (pet.isEgg()) pet.eggTap();
  pet.sleeping = false;
  Pet guarda = pet;
  pet.dbgRunawayReady();
  avanza(100);
  CHECK_MSG(pet.canRunawayNow(), "premissa: o bicho pode escapar");
  if (!pet.canRunawayNow()) { pet = guarda; return; }
  const int BX = 233, BY = 216;          // dentro do botao E do 1o do dialogo
  toca(BX, BY);
  CHECK_MSG(choiceKind == 3, "o botao de escapada nao abriu a pergunta");
  CHECK_MSG(pet.ceremony == CER_NONE, "um toque so ja soltou o bicho");
  toca(BX, BY);                          // o segundo toque do "duplo"
  CHECK_MSG(choiceKind == 3 && pet.ceremony == CER_NONE,
            "o toque duplo decidiu sozinho");
  avanza(700);
  toca(BX, BY);                          // em cima = ficar
  CHECK_MSG(choiceKind == 0, "ficar nao fechou a pergunta");
  CHECK_MSG(pet.ceremony == CER_NONE, "o botao de cima soltou o bicho");
  avanza(100);
  toca(BX, BY);
  avanza(700);
  toca(BX, 294);                         // em baixo = deixar ir
  CHECK_MSG(pet.ceremony == CER_RUNAWAY, "deixar ir nao soltou o bicho");
  pet = guarda;                          // devolve o estado global
  choiceKind = 0;
  todoCerradoT();
}

TEST(interfaz, o_bicho_abandonado_chama_com_o_grito) {
  arranca();
  todoCerradoT();
  if (pet.awaitingStarter()) pet.chooseStarter(1);
  while (pet.isEgg()) pet.eggTap();
  pet.sleeping = false;
  Pet guarda = pet;
  pet.dbgRunawayReady();
  mockAudioLogClear();
  avanza(61000, 250);                    // um minuto do bicho
  std::string a = mockAudioLog();
  CHECK_MSG(a.find("cry:#" + std::to_string(pet.speciesId) + " ") != std::string::npos,
            "abandonado e nao chamou: " + a);
  pet = guarda;
  todoCerradoT();
}

void aventuraMedeMega(int16_t fila, int *alto, int *largo, bool *real);
TEST(aventura_ui, a_mega_sem_sprite_proprio_nao_encolhe) {
  // Leva 67. "o mega blaziken aparece minusculo": sem /av/ a mega saia
  // do tamanho da TABELA da forma normal, num quadro esticado pelas
  // chamas —— 122 px contra 128 do proprio Blaziken. Medido: 13 das 48
  // megas do desenho do jogo abaixo de 90% do normal, nenhuma maior.
  // Agora o normal na cena vezes o quanto a mega e maior no desenho.
  mundoDaAventura(257);
  int nJogo = 0, encolhe = 0;
  std::string quem;
  for (int f = 0; f < MEGA_COUNT; f++) {
    int am = 0, lm = 0, ab = 0, lb = 0;
    bool real = false;
    aventuraMedeMega((int16_t)f, &am, &lm, &real);
    aventuraMede(MEGA_TBL[f].base, &ab, &lb);
    if (real || !am || !ab) continue;
    nJogo++;
    if (am * 100 < ab * 90) { encolhe++; quem += std::string(MEGA_TBL[f].name) + " "; }
  }
  CHECK_MSG(nJogo > 40, "premissa: megas do desenho do jogo medidas: " + std::to_string(nJogo));
  CHECK_MSG(encolhe == 0, std::to_string(encolhe) + " megas menores que a forma normal: " + quem);
  int am = 0, lm = 0, ab = 0, lb = 0;
  bool real = true;
  int16_t fila = -1;
  for (int f = 0; f < MEGA_COUNT; f++)
    if (MEGA_TBL[f].base == 257) fila = (int16_t)f;
  CHECK_MSG(fila >= 0, "premissa: Mega Blaziken na tabela");
  if (fila < 0) return;
  aventuraMedeMega(fila, &am, &lm, &real);
  aventuraMede(257, &ab, &lb);
  CHECK_MSG(!real, "premissa: o Mega Blaziken nao tem sprite da aventura");
  CHECK_MSG(am * 100 > ab * 120, "Mega Blaziken " + std::to_string(am) +
                                     " px contra " + std::to_string(ab) + " do Blaziken");
}

#include "../quiz.h"
extern QuizQ quizQ;
extern uint8_t quizIdx, quizHits, quizPrize, quizPrizeQtd;
extern int8_t quizPick;
extern bool quizEnd;
void quizStart();
void quizTap(int16_t x, int16_t y);
// O centro do botao i, com a MESMA conta da tela (qzBtnY e static la).
static int qzCentroY(int i) {
  return quizQ.kind == QZ_WHO ? 224 + i * 58 + 25 : 186 + i * 62 + 27;
}
static void qzResponde(bool certo) {
  int i = certo ? quizQ.right : (quizQ.right + 1) % QUIZ_OPTS;
  quizTap(233, (int16_t)qzCentroY(i));
  avanza(800);
  quizTap(233, 233);                     // segue para a proxima
  avanza(50);
}

TEST(quiz_ui, quatro_certas_e_a_quinta_e_quem_e_em_silhueta) {
  // Leva 68. Com 4 de 4, a quinta e SEMPRE "quem e esse Pokemon" com o
  // dibujo todo preto, e os 5 de 5 pagam dois Rare Candy (ou a Shiny Potion).
  arranca();
  todoCerradoT();
  if (pet.awaitingStarter()) pet.chooseStarter(1);
  while (pet.isEgg()) pet.eggTap();
  Pet guarda = pet;
  randomSeed(6802);
  quizStart();
  avanza(100);
  for (int k = 0; k < 4; k++) {
    CHECK_EQ((int)quizIdx, k);
    qzResponde(true);
  }
  CHECK_EQ((int)quizHits, 4);
  CHECK_MSG(quizQ.kind == QZ_WHO, "a quinta nao e o quem e: classe " + std::to_string(quizQ.kind));
  CHECK_MSG(quizQ.silueta, "a quinta nao esta em silhueta");
  avanza(100);
  volcaPantalla("quiz_quem_silhueta");
  // o dibujo em silhueta: so preto na caixa do dibujo
  quizTap(233, (int16_t)qzCentroY(quizQ.right));
  avanza(100);
  volcaPantalla("quiz_quem_revelado");
  avanza(800);
  quizTap(233, 233);
  avanza(100);
  CHECK(quizEnd);
  CHECK_EQ((int)quizHits, 5);
  CHECK_MSG(quizPrize == quizItemDe(IT_RARECANDY) || quizPrize == quizItemDe(IT_SHINY),
            "5 de 5 nao deu o premio cheio: " + std::to_string(quizPrize));
  if (quizPrize == quizItemDe(IT_RARECANDY)) CHECK_EQ((int)quizPrizeQtd, 2);
  volcaPantalla("quiz_premio");
  quizTap(233, 233);
  pet = guarda;
  todoCerradoT();
}

TEST(quiz_ui, tres_de_quatro_e_a_quinta_e_normal) {
  arranca();
  todoCerradoT();
  if (pet.awaitingStarter()) pet.chooseStarter(1);
  while (pet.isEgg()) pet.eggTap();
  Pet guarda = pet;
  bool viuQuemColorido = false;
  for (int r = 0; r < 12; r++) {
    randomSeed(6900 + r);
    quizStart();
    avanza(100);
    for (int k = 0; k < 4; k++) {
      if (quizQ.kind == QZ_WHO && !quizQ.silueta && !viuQuemColorido) {
        viuQuemColorido = true;
        volcaPantalla("quiz_quem_colorido");
      }
      qzResponde(k != 2);                // erra a terceira
    }
    CHECK_EQ((int)quizHits, 3);
    CHECK_MSG(!quizQ.silueta, "sem 4 acertos a quinta saiu em silhueta");
    qzResponde(true);
    CHECK(quizEnd);
    CHECK_MSG(quizPrize != quizItemDe(IT_SHINY) && quizPrize != quizItemDe(IT_RARECANDY),
              "4 de 5 deu o premio cheio");
    quizTap(233, 233);
  }
  CHECK_MSG(viuQuemColorido, "em 12 rondas nao saiu nenhum quem e colorido");
  pet = guarda;
  todoCerradoT();
}


// ---- leva 70: as FORMAS DE COMBATE e a ILUSAO, na tela. Mede-se o
// desenho que ficou CARREGADO (o tamanho do quadro parado), nao a chave:
// a chave certa com o ficheiro errado passava.
extern int16_t foePmdFor;
extern int16_t myPmdFor;
extern PmdMon pmd;
void btNextFoe();
static uint16_t habUiT(uint8_t fx) {
  for (uint16_t i = 1; i < ABIL_COUNT; i++)
    if (ABIL_TBL[i].fx == fx) return i;
  return 0;
}
static uint32_t marcaPmdT(const PmdMon &m) {
  if (!m.loaded) return 0;
  return ((uint32_t)m.acts[PMD_IDLE].w << 16) | ((uint32_t)m.acts[PMD_IDLE].h << 8) |
         m.acts[PMD_IDLE].frames;
}

TEST(formas_ui, o_castform_ao_sol_carrega_o_desenho_do_sol) {
  CHECK_MSG(combateDeBola(351, 30), "premissa: o combate abre");
  Combatant &c = bt.b.side[BT_FOE];
  c.ability = habUiT(AB_FORECAST);
  CHECK(c.ability != 0);
  avanza(100);
  static PmdMon normal, sol;
  normal.load(351, c.shiny, false);
  sol.loadBatalha(351, 1, c.shiny);
  CHECK_MSG(sol.loaded, "premissa: c0351_1.bin esta no cartao");
  CHECK_MSG(marcaPmdT(normal) != marcaPmdT(sol), "premissa: os dois desenhos diferem");
  CHECK_MSG(marcaPmdT(foePmd) == marcaPmdT(normal), "sem sol, o de sempre");
  bt.b.weather = WE_SUN;
  bt.b.weatherTurns = 5;
  bt.b.syncForma(BT_FOE, 2);
  CHECK(c.bform == bformRow(351, 1));
  avanza(100);
  CHECK_MSG(marcaPmdT(foePmd) == marcaPmdT(sol), "ao sol, o desenho do sol");
  bt.b.weather = WE_NONE;
  bt.b.weatherTurns = 0;
  bt.b.syncForma(BT_FOE, 2);
  avanza(100);
  CHECK_MSG(marcaPmdT(foePmd) == marcaPmdT(normal), "sem sol, volta");
  cierraElCombate();
  todoCerradoT();
}

TEST(formas_ui, o_meu_aegislash_vira_filo_e_volta) {
  Pet guarda = pet;
  arranca();
  cierraElCombate();
  if (pet.awaitingStarter()) pet.chooseStarter(1);
  while (pet.isEgg()) pet.eggTap();
  pet.speciesId = 681;                  // Aegislash
  pet.form = 0;
  pet.sleeping = false;
  pet.energy = 100;
  pet.hpLeft = pet.maxHp();
  battleStart(143, 30);
  for (int i = 0; i < 400 && bt.phase != BP_CHOOSE; i++) avanza(16);
  CHECK_MSG(bt.open && bt.phase == BP_CHOOSE, "premissa: o combate abre");
  Combatant &c = bt.b.side[BT_PLAYER];
  CHECK(c.dex == 681);
  c.ability = habUiT(AB_STANCE);
  static PmdMon escudo, filo;
  escudo.load(681, c.shiny, false);
  filo.loadBatalha(681, 1, c.shiny);
  CHECK_MSG(filo.loaded && marcaPmdT(escudo) != marcaPmdT(filo), "premissa: dois desenhos");
  avanza(100);
  CHECK_MSG(marcaPmdT(pmd) == marcaPmdT(escudo), "entra de escudo");
  c.blade = true;
  bt.b.syncForma(BT_PLAYER, 0);
  avanza(100);
  CHECK_MSG(marcaPmdT(pmd) == marcaPmdT(filo), "afilado, o desenho do filo");
  c.blade = false;
  bt.b.syncForma(BT_PLAYER, 0);
  avanza(100);
  CHECK_MSG(marcaPmdT(pmd) == marcaPmdT(escudo), "e volta ao escudo");
  cierraElCombate();
  pet = guarda;
  todoCerradoT();
}

TEST(formas_ui, o_zoroark_do_treinador_entra_com_a_cara_do_ultimo) {
  int8_t tr = -1;
  for (uint8_t i = 0; i < TRAINER_COUNT && tr < 0; i++)
    if (trainerAt(i).team >= 2) tr = (int8_t)i;
  CHECK_MSG(tr >= 0, "premissa: um treinador com dois ou mais");
  arranca();
  cierraElCombate();
  if (pet.awaitingStarter()) pet.chooseStarter(1);
  while (pet.isEgg()) pet.eggTap();
  pet.sleeping = false;
  pet.energy = 100;
  pet.hpLeft = pet.maxHp();
  battleStart(571, 30, -1, false, tr);
  for (int i = 0; i < 400 && bt.phase != BP_CHOOSE; i++) avanza(16);
  CHECK_MSG(bt.open && bt.phase == BP_CHOOSE, "premissa: o combate abre");
  Combatant &c = bt.b.side[BT_FOE];
  CHECK_MSG(c.dex == 571 && c.abil() == AB_ILLUSION, "premissa: Zoroark com Ilusao");
  int16_t ultimo = bt.foeUltimo;
  CHECK_MSG(ultimo > 0 && ultimo != 571, "o ultimo do treinador foi sorteado");
  CHECK_MSG(c.illusion && c.pareceDex() == ultimo, "entra com a cara do ultimo");
  CHECK_MSG(c.form == 0, "e nao e um Zoroark de Hisui");
  static PmdMon cara, proprio;
  cara.load(ultimo, false, false);
  proprio.load(571, c.shiny, false);
  avanza(100);
  CHECK_MSG(marcaPmdT(foePmd) == marcaPmdT(cara), "o desenho e o do ultimo");
  c.illusion = false;                   // o que faz o primeiro golpe
  avanza(100);
  CHECK_MSG(marcaPmdT(foePmd) == marcaPmdT(proprio), "caida a ilusao, e um Zoroark");
  // e o ultimo do treinador sai de VERDADE quando chegar a vez dele
  bt.foesLeft = 1;
  btNextFoe();
  CHECK_MSG(bt.b.side[BT_FOE].dex == ultimo, "o ultimo e o mesmo que a ilusao mostrou");
  cierraElCombate();
  todoCerradoT();
}


// O vizinho do conserto acima: o meu Ditto transformado. O btSyncMySprite
// marcava monFor = -2, e o ensureMon() (que corre em CADA vuelta, tambem
// em combate) recarregava o Ditto no quadro seguinte.
TEST(formas_ui, o_meu_ditto_transformado_fica_com_o_desenho_do_outro) {
  Pet guarda = pet;
  arranca();
  cierraElCombate();
  if (pet.awaitingStarter()) pet.chooseStarter(1);
  while (pet.isEgg()) pet.eggTap();
  pet.speciesId = 132;                  // Ditto
  pet.form = 0;
  pet.sleeping = false;
  pet.energy = 100;
  pet.hpLeft = pet.maxHp();
  battleStart(143, 30);                 // Snorlax
  for (int i = 0; i < 400 && bt.phase != BP_CHOOSE; i++) avanza(16);
  CHECK_MSG(bt.open && bt.phase == BP_CHOOSE, "premissa: o combate abre");
  Combatant &c = bt.b.side[BT_PLAYER];
  c.transformInto(bt.b.side[BT_FOE]);
  static PmdMon outro;
  outro.load(143, c.shiny, false);
  CHECK_MSG(outro.loaded, "premissa: o Snorlax esta no cartao");
  avanza(400);                          // varias voltas do bucle
  CHECK_MSG(marcaPmdT(pmd) == marcaPmdT(outro), "o Ditto voltou a ser Ditto no desenho");
  cierraElCombate();
  pet = guarda;
  todoCerradoT();
}


// ---- leva 71: FOTOS das tres telas que vao mudar (evolucao, medalha,
// insignia). So com TAMA_FOTO71; sem ela o teste nao faz nada.
extern uint8_t choiceKind;
extern uint32_t choiceUntil;
TEST(fotos71, as_tres_telas) {
  if (!getenv("TAMA_FOTO71")) return;
  Pet guarda = pet;
  arranca();
  Lang langAntes = gLang;
  setLang(LANG_PT);
  applyLangFont();
  cierraElCombate();
  if (pet.awaitingStarter()) pet.chooseStarter(1);
  while (pet.isEgg()) pet.eggTap();
  pet.sleeping = false;
  avanza(300);
  choiceKind = 1;
  choiceUntil = millis() + 12000;
  avanza(100);
  volcaPantalla("f71_evolucao");
  choiceKind = 2;
  choiceUntil = millis() + 12000;
  avanza(100);
  volcaPantalla("f71_despedida");
  choiceKind = 3;
  choiceUntil = millis() + 12000;
  avanza(100);
  volcaPantalla("f71_fuga");
  choiceKind = 0;
  avanza(100);
  pet.celebraMedalla(1u << 0);
  avanza(100);
  volcaPantalla("f71_medalha");
  pet.celebraMedalla(0, 0);
  avanza(100);
  pet.energy = 100;
  pet.hpLeft = pet.maxHp();
  battleStart(16, 8);
  for (int i = 0; i < 400 && bt.phase != BP_CHOOSE; i++) avanza(16);
  if (pet.badges < 1) pet.badges = 1;
  bt.insigniaIdx = 0;
  int16_t tmAntes = pet.lastTM;
  pet.lastTM = 0;
  bt.phase = BP_BADGE;
  bt.until = millis();
  avanza(100);
  volcaPantalla("f71_insignia");
  pet.lastTM = tmAntes;
  cierraElCombate();
  setLang(langAntes);
  applyLangFont();
  pet = guarda;
  todoCerradoT();
}


// ---- leva 71: os CARTELES (evolucao, medalha, insignia) com a letra de
// DS e a janela dourada. Mede-se o que fica NA TELA (pixels), nao a funcao.
enum Cartel71 { C71_EVO, C71_MEDALHA, C71_INSIGNIA };
static void abreCartel71(Cartel71 qual, Lang lang) {
  arranca();
  // DEPOIS do arranca: ele volta o idioma ao guardado (o teste comparava
  // ingles com ingles e passava com a traducao sabotada)
  setLang(lang);
  applyLangFont();
  cierraElCombate();
  // Tudo fechado e um bicho conhecido: no lote inteiro um teste anterior
  // deixava outra tela aberta ou um botao (CTA) por cima e a seta dava 0.
  todoCerradoT();
  gestosOn = false;
  if (pet.awaitingStarter()) pet.chooseStarter(1);
  for (int i = 0; i < 50 && pet.isEgg(); i++) pet.eggTap();
  pet.speciesId = 1;                    // Bulbasaur -> Ivysaur
  pet.form = 0;
  pet.sleeping = false;
  pet.celebraMedalla(0, 0);
  avanza(200);
  // O relogio do teste volta a 0 em cada teste, mas o evolveUntil que um
  // teste anterior deixou fica no futuro: o bicho seguia "evoluindo" e a
  // animacao tapava a janela (seta = 0 so no lote inteiro). Espera-se.
  for (int i = 0; i < 400 && pet.evolving(); i++) avanza(50);
  CHECK_MSG(!pet.evolving(), "premissa: ficou uma evolucao a meio");
  if (qual == C71_EVO) {
    choiceKind = 1;
    choiceUntil = millis() + 12000;
  } else if (qual == C71_MEDALHA) {
    pet.celebraMedalla(1u << 0);
  } else {
    pet.energy = 100;
    pet.hpLeft = pet.maxHp();
    battleStart(16, 8);
    for (int i = 0; i < 400 && bt.phase != BP_CHOOSE; i++) avanza(16);
    if (pet.badges < 1) pet.badges = 1;
    bt.insigniaIdx = 0;
    pet.lastTM = 0;
    bt.phase = BP_BADGE;
    bt.until = millis();
  }
  avanza(100);
}
static void fechaCartel71() {
  choiceKind = 0;
  pet.celebraMedalla(0, 0);
  cierraElCombate();
  avanza(50);
}
// quantos pixels da faixa [y0, y1) x [x0, x1) satisfazem `f`
template <class F>
static int contaPixT(int x0, int x1, int y0, int y1, F f) {
  const uint16_t *fb = gfx->getFramebuffer();
  int n = 0;
  for (int y = y0; y < y1; y++)
    for (int x = x0; x < x1; x++) {
      uint16_t c = fb[y * gfx->width() + x];
      if (f((c >> 11) << 3, ((c >> 5) & 63) << 2, (c & 31) << 3)) n++;
    }
  return n;
}

TEST(carteles71, as_tres_telas_tem_a_cara_nova) {
  Pet guarda = pet;
  Lang langAntes = gLang;
  setLang(LANG_PT);
  applyLangFont();
  // EVOLUCAO: a seta laranja entre as duas miniaturas
  abreCartel71(C71_EVO, LANG_PT);
  int seta = contaPixT(CX - 30, CX + 20, 156, 190, [](int r, int g, int b) {
    return r > 200 && g > 120 && b < 150;
  });
  if (seta <= 150) {
    volcaPantalla("f71_falha_seta");
    printf("  [diag] choiceKind=%u restante=%ld evolving=%d\n", (unsigned)choiceKind,
           (long)(choiceUntil - millis()), (int)pet.evolving());
  }
  CHECK_MSG(seta > 150, "a seta da evolucao nao aparece: " + std::to_string(seta));
  // e a miniatura de quem evolui, a esquerda, nao e so papel
  int papel = contaPixT(CX - 110, CX - 54, 144, 200, [](int r, int g, int b) {
    return r > 235 && g > 235 && b > 225;
  });
  CHECK_MSG(papel < 56 * 56 - 150, "sem miniatura de quem evolui");
  fechaCartel71();
  // MEDALHA: o disco dourado
  abreCartel71(C71_MEDALHA, LANG_PT);
  int ouro = contaPixT(CX - 20, CX + 20, 160, 200, [](int r, int g, int b) {
    return r > 230 && g > 190 && g < 225 && b < 100;
  });
  CHECK_MSG(ouro > 200, "a medalha desenhada nao aparece: " + std::to_string(ouro));
  // e o titulo na cor de titulo (laranja escuro), nao no preto antigo
  int tit = contaPixT(100, 366, 212, 245, [](int r, int g, int b) {
    return r > 170 && g > 60 && g < 100 && b < 40;
  });
  CHECK_MSG(tit > 200, "o titulo MEDALHA nao esta na letra nova: " + std::to_string(tit));
  fechaCartel71();
  // INSIGNIA: titulo laranja e a faixa do titulo MUDA com o idioma
  abreCartel71(C71_INSIGNIA, LANG_PT);
  int titI = contaPixT(60, 406, 100, 132, [](int r, int g, int b) {
    return r > 170 && g > 60 && g < 100 && b < 40;
  });
  CHECK_MSG(titI > 200, "o titulo da insignia nao esta na letra nova: " + std::to_string(titI));
  // Conta-se a TINTA de cada linha, e nao a faixa inteira: o brilho atras
  // da insignia respira e mudava a faixa sozinho (a sabotagem passava).
  auto tituloTinta = []() {
    return contaPixT(60, 406, 100, 128, [](int r, int g, int b) {
      return r > 170 && g > 60 && g < 100 && b < 40;
    });
  };
  // "MT:" e "TM:" tem as MESMAS letras: conta igual. Mede-se ONDE esta a
  // tinta (a soma dos x), que muda com a ordem das letras.
  auto mtTinta = []() {
    const uint16_t *fb = gfx->getFramebuffer();
    long soma = 0;
    int n = 0;
    for (int y = 280; y < 310; y++)
      for (int x = 60; x < 406; x++) {
        uint16_t c = fb[y * gfx->width() + x];
        int r = (c >> 11) << 3, g = ((c >> 5) & 63) << 2, bb = (c & 31) << 3;
        if (r < 80 && g > 70 && g < 110 && bb > 150) { soma += x; n++; }
      }
    return n > 100 ? (int)(soma % 1000003) + 1 : n;
  };
  int ptTit = tituloTinta(), ptMt = mtTinta();
  CHECK_MSG(ptMt > 100, "premissa: a linha da MT aparece: " + std::to_string(ptMt));
  fechaCartel71();
  setLang(LANG_EN);
  applyLangFont();
  abreCartel71(C71_INSIGNIA, LANG_EN);
  int enTit = tituloTinta(), enMt = mtTinta();
  CHECK_MSG(ptTit != enTit, "NOVA INSIGNIA e NEW BADGE! pintaram o mesmo: o titulo nao se traduz");
  CHECK_MSG(ptMt != enMt, "MT: e TM: pintaram o mesmo: a linha da MT nao se traduz");
  fechaCartel71();
  setLang(langAntes);
  applyLangFont();
  pet = guarda;
  todoCerradoT();
}

uint16_t tituloW(const char *s, uint8_t n);
uint8_t tituloTalla(const char *s, uint8_t n, int ancho);
TEST(carteles71, o_titulo_encolhe_para_caber_na_janela) {
  // A janela tem 350 px; os carteles pedem <= 300. O vidro deixa passar
  // mais do que isso a esta altura, por isso o metro do texto nao pega:
  // quem pega e a medida da letra.
  const char *longo = "PROVA DE UM TITULO MUITO COMPRIDO";
  CHECK(tituloW(longo, 3) > 300);
  uint8_t n = tituloTalla(longo, 3, 300);
  CHECK_MSG(n < 3 && tituloW(longo, n) <= 300, "o titulo comprido nao encolheu");
  Lang langAntes = gLang;
  for (int l = 0; l < LANG_COUNT; l++) {
    setLang((Lang)l);
    applyLangFont();
    for (int id : { S_BADGE_NEW, S_BADGE_ZCRYSTAL, S_EVO_Q, S_MEDAL_BANNER }) {
      const char *t = T((StrId)id);
      CHECK_MSG(tituloW(t, tituloTalla(t, 3, 300)) <= 300,
                std::string("nao cabe: ") + t);
    }
  }
  setLang(langAntes);
  applyLangFont();
}

TEST(carteles71, nenhum_texto_sai_do_vidro_em_idioma_nenhum) {
  Pet guarda = pet;
  Lang langAntes = gLang;
  std::string ruins;
  for (int l = 0; l < LANG_COUNT; l++) {
    setLang((Lang)l);
    applyLangFont();
    for (Cartel71 q : { C71_EVO, C71_MEDALHA, C71_INSIGNIA }) {
      abreCartel71(q, (Lang)l);
      mockTextoReset();
      avanza(100);
      for (const TextoSalido &t : mockTextoFuera())
        ruins += "[lang " + std::to_string(l) + " cartel " + std::to_string((int)q) +
                 "] '" + t.texto + "' sai " + std::to_string(t.lejos) + " px; ";
      fechaCartel71();
    }
  }
  setLang(langAntes);
  applyLangFont();
  pet = guarda;
  CHECK_MSG(ruins.empty(), ruins);
  todoCerradoT();
}


// ---- leva 72: FOTOS da Pokedex com VISTOS e a CACADA. So com
// TAMA_FOTO72; sem ela o teste nao faz nada.
static void poeVisto72(int16_t d) {
  pet.dexReg[(d - 1) >> 3] &= (uint8_t) ~(1 << ((d - 1) & 7));
  pet.markSeen(d);
}
static void tiraTudo72(int16_t d) {
  pet.dexReg[(d - 1) >> 3] &= (uint8_t) ~(1 << ((d - 1) & 7));
  pet.dexSeen[(d - 1) >> 3] &= (uint8_t) ~(1 << ((d - 1) & 7));
}
TEST(fotos72, a_pokedex_com_vistos) {
  if (!getenv("TAMA_FOTO72")) return;
  Pet guarda = pet;
  arranca();
  Lang langAntes = gLang;
  setLang(LANG_PT);
  applyLangFont();
  cierraElCombate();
  todoCerradoT();
  if (pet.awaitingStarter()) pet.chooseStarter(1);
  for (int i = 0; i < 50 && pet.isEgg(); i++) pet.eggTap();
  pet.sleeping = false;
  // pagina 1: seus 1, 4, 7 e 8; vistos 10, 13, 16, 19 e 12; o resto nunca
  for (int16_t d = 1; d <= 16; d++) tiraTudo72(d);
  const int16_t seus[] = { 1, 4, 7, 8 };
  for (int16_t d : seus) pet.registerSpecies(d, false);
  const int16_t vistos[] = { 10, 12, 13, 16 };
  for (int16_t d : vistos) poeVisto72(d);
  pet.setCaca(16);
  galleryOpen = true;
  galleryPage = 0;
  galleryDetail = 0;
  galleryDirty = true;
  avanza(300);
  volcaPantalla("f72_grade");
  openDexEntry(13);
  avanza(400);
  volcaPantalla("f72_ficha_cacar");
  galleryDetail = 0;
  galleryDirty = true;
  openDexEntry(16);
  avanza(400);
  volcaPantalla("f72_ficha_cacando");
  galleryDetail = 0;
  tiraTudo72(150);
  poeVisto72(150);
  openDexEntry(150);
  avanza(400);
  volcaPantalla("f72_ficha_nao_sai");
  cierraLaFicha();
  avanza(50);
  setLang(langAntes);
  applyLangFont();
  pet = guarda;
  todoCerradoT();
}


// ---- leva 72: VISTOS e CACADA, medidos na TELA ----
// Copiados de TamaPoke.ino (GAL_X/GAL_Y/GAL_CELL, DEX_VISTO, DEX_SIL,
// DEX_CACA e o botao CACA_*). Se mudarem la, mudam aqui.
#define V72_GAL_X 73
#define V72_GAL_Y 84
#define V72_CELL 80
#define V72_C565(r, g, b) ((uint16_t)((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3)))
static const uint16_t V72_VISTO = V72_C565(0x9c, 0xac, 0xbc);
static const uint16_t V72_SIL = V72_C565(0x24, 0x33, 0x40);
static const uint16_t V72_CACA = V72_C565(0xf0, 0x80, 0x30);
static const uint16_t V72_VERMELHO = V72_C565(0xe0, 0x50, 0x38);
static const uint16_t V72_AZUL = V72_C565(0x58, 0x70, 0x98);

static void abre72(Lang lang) {
  arranca();
  setLang(lang);
  applyLangFont();
  cierraElCombate();
  todoCerradoT();
  gestosOn = false;
  if (pet.awaitingStarter()) pet.chooseStarter(1);
  for (int i = 0; i < 50 && pet.isEgg(); i++) pet.eggTap();
  pet.sleeping = false;
  pet.setCaca(0);
}
static int celula72(int16_t dex, uint16_t cor) {
  int i = (dex - 1) % 16, x = V72_GAL_X + (i % 4) * V72_CELL, y = V72_GAL_Y + (i / 4) * V72_CELL;
  return pixelesDeColor(cor, x, y, x + V72_CELL - 1, y + V72_CELL - 1);
}

TEST(vistos72, o_rival_da_batalha_fica_visto_na_pokedex) {
  Pet guarda = pet;
  Lang langAntes = gLang;
  abre72(LANG_PT);
  tiraTudo72(19);
  CHECK_MSG(!pet.isSeen(19), "premissa: o Rattata nao pode estar visto");
  pet.energy = 100;
  pet.hpLeft = pet.maxHp();
  battleStart(19, 5);
  for (int i = 0; i < 400 && bt.phase != BP_CHOOSE; i++) avanza(16);
  CHECK_MSG(pet.isSeen(19), "o Rattata apareceu na batalha e nao ficou visto");
  CHECK_MSG(!pet.isRegistered(19), "ver na batalha nao pode registrar");
  cierraElCombate();
  avanza(50);
  setLang(langAntes);
  applyLangFont();
  pet = guarda;
  todoCerradoT();
}

TEST(vistos72, a_grade_tem_tres_estados_e_o_anel_do_cacado) {
  Pet guarda = pet;
  Lang langAntes = gLang;
  abre72(LANG_PT);
  for (int16_t d = 1; d <= 16; d++) tiraTudo72(d);
  pet.registerSpecies(4, false);
  poeVisto72(13);
  poeVisto72(16);
  pet.setCaca(16);
  galleryOpen = true;
  galleryPage = 0;
  galleryDetail = 0;
  galleryDirty = true;
  avanza(300);
  int visto = celula72(13, V72_VISTO), nunca = celula72(14, V72_VISTO);
  int escuro = celula72(14, V72_SIL), seuClaro = celula72(4, V72_VISTO);
  int anel = celula72(16, V72_CACA), semAnel = celula72(13, V72_CACA);
  if (visto <= 200) volcaPantalla("f72_falha_grade");
  CHECK_MSG(visto > 200, "o visto nao saiu em silhueta clara: " + std::to_string(visto));
  CHECK_MSG(nunca == 0, "o que nunca apareceu saiu claro: " + std::to_string(nunca));
  CHECK_MSG(escuro > 200, "o que nunca apareceu perdeu a silhueta escura");
  CHECK_MSG(seuClaro == 0, "o seu saiu em silhueta em vez da cor");
  CHECK_MSG(anel > 60, "o cacado nao tem o anel: " + std::to_string(anel));
  CHECK_MSG(semAnel == 0, "um nao cacado ganhou anel");
  cierraLaFicha();
  avanza(50);
  setLang(langAntes);
  applyLangFont();
  pet = guarda;
  todoCerradoT();
}

TEST(vistos72, na_ficha_do_visto_o_botao_liga_e_desliga_a_cacada) {
  Pet guarda = pet;
  Lang langAntes = gLang;
  abre72(LANG_PT);
  tiraTudo72(13);
  poeVisto72(13);
  openDexEntry(13);
  avanza(400);
  // o bicho a CORES: a silhueta e uma cor so; o Weedle tem varias
  std::set<uint16_t> cores;
  {
    const uint16_t *fb = gfx->getFramebuffer();
    for (int y = 170; y < 286; y++)
      for (int x = 160; x < 306; x++) cores.insert(fb[y * gfx->width() + x]);
  }
  CHECK_MSG(cores.size() > 12, "a ficha do visto mostra a silhueta, nao o bicho: " +
            std::to_string(cores.size()) + " cores");
  int verm = pixelesDeColor(V72_VERMELHO, 133, 300, 333, 370);
  if (verm <= 1500) volcaPantalla("f72_falha_cacar");
  CHECK_MSG(verm > 1500, "a ficha do visto nao tem o botao CACAR: " + std::to_string(verm));
  toca(233, 335);
  CHECK_MSG(pet.cacaDex == 13, "tocar CACAR nao ligou a cacada");
  CHECK_MSG(galleryDetail == 13, "tocar o botao fechou a ficha");
  avanza(200);
  int azul = pixelesDeColor(V72_AZUL, 133, 300, 333, 370);
  CHECK_MSG(azul > 1500, "cacando, nao aparece o PARAR: " + std::to_string(azul));
  toca(233, 340);
  CHECK_MSG(pet.cacaDex == 0, "tocar PARAR nao desligou a cacada");
  // errar o botao por pouco (dentro da caixa) nao fecha a ficha
  toca(90, 300);
  CHECK_MSG(galleryDetail == 13, "um toque na caixa, fora do botao, fechou a ficha");
  cierraLaFicha();
  avanza(50);
  setLang(langAntes);
  applyLangFont();
  pet = guarda;
  todoCerradoT();
}

TEST(vistos72, o_lendario_visto_nao_tem_botao_e_o_toque_nao_caca) {
  Pet guarda = pet;
  Lang langAntes = gLang;
  abre72(LANG_PT);
  pet.badges = 0;          // sem a regiao feita o lendario ainda nao sai
  tiraTudo72(150);
  poeVisto72(150);
  openDexEntry(150);
  avanza(400);
  int verm = pixelesDeColor(V72_VERMELHO, 133, 300, 333, 370);
  CHECK_MSG(verm < 50, "o Mewtwo ganhou botao de cacar: " + std::to_string(verm));
  toca(233, 335);
  CHECK_MSG(pet.cacaDex == 0, "tocar onde seria o botao cacou o Mewtwo");
  cierraLaFicha();
  avanza(50);
  setLang(langAntes);
  applyLangFont();
  pet = guarda;
  todoCerradoT();
}


// ---- MANUAL (docs/manual/): as fotos do manual do jogador. So com
// TAMA_MANUAL=pt ou =en; sem ela o teste nao faz nada. Grava
// _claude_scratch/prev_man_<lingua>_<tela>.ppm, que tools/manual_fotos.py
// recorta em PNG redondos.
//
//   TAMA_MANUAL=pt ./build/tamapoke_uitests manual
//   TAMA_MANUAL=en ./build/tamapoke_uitests manual
//   python tools/manual_fotos.py
extern bool introOpen;
extern uint8_t introStep, introPage;
extern uint32_t introSince;
void startIntro();
void openClock();
void startGym();
void gymStartRound(uint8_t which);
extern uint32_t healUntil;
static std::string manLingua;
static void fotoMan(const char *nome) {
  volcaPantalla(("man_" + manLingua + "_" + nome).c_str());
}

TEST(manual, as_fotos_do_manual) {
  const char *qual = getenv("TAMA_MANUAL");
  if (!qual) return;
  manLingua = qual;
  Lang lang = manLingua == "en" ? LANG_EN : LANG_PT;
  Pet guarda = pet;
  Lang langAntes = gLang;

  preparaElMundoT();
  setLang(lang);
  applyLangFont();
  std::string sdAntes = mockSdRootGet();
  // um jogo ja andado: a dex com uns registrados e uns vistos, a ficha
  // de treinador com numeros
  for (int16_t d = 1; d <= 40; d++)
    if (d % 7 != 3 && d != 12 && d != 13 && d != 19)
      pet.dexReg[(d - 1) >> 3] |= (uint8_t)(1 << ((d - 1) & 7));
  for (int16_t d : { 3, 10, 17, 24, 31, 38, 41, 42 }) pet.markSeen(d);
  pet.wins = 42;
  pet.caught = 37;

  // ---- a casa
  avanza(400);
  fotoMan("casa");
  feedMenuUntil = millis() + 6000;
  avanza(160);
  fotoMan("comida");
  feedMenuUntil = 0;
  avanza(100);

  // ---- as decisoes
  for (uint8_t k = 1; k <= 3; k++) {
    choiceKind = k;
    choiceUntil = millis() + 12000;
    avanza(120);
    fotoMan(k == 1 ? "decide_evolucao" : k == 2 ? "decide_despedida" : "decide_fuga");
  }
  choiceKind = 0;
  avanza(100);
  pet.celebraMedalla(1u << 0);
  avanza(120);
  fotoMan("medalha");
  pet.celebraMedalla(0, 0);
  avanza(100);
  confirmUntil = millis() + 6000;
  avanza(120);
  fotoMan("soltar");
  confirmUntil = 0;
  apagarUntil = millis() + 6000;
  avanza(120);
  fotoMan("desligar");
  apagarUntil = 0;
  avanza(100);

  // ---- treino (o saco de pancadas)
  startGym();
  avanza(160);
  fotoMan("treino_escolha");
  gymStartRound(0);
  avanza(600);
  fotoMan("treino_rodada");
  todoCerradoT();
  avanza(100);

  // ---- relogio / ajustes
  openClock();
  avanza(160);
  fotoMan("ajustes");
  manualQrOpen = true;   // o QR do manual (leva 74)
  avanza(160);
  fotoMan("ajustes_qr");
  todoCerradoT();
  avanza(100);

  // ---- quiz do professor
  quizStart();
  avanza(300);
  fotoMan("quiz");
  todoCerradoT();
  avanza(100);

  // ---- as telas da lista do mapa de toques (ficha, dex, troca, duelo...)
  Pet petCasa = pet;
  BattleUi btCasa = bt;
  battleStart(16, 10, -1, false, -1);
  for (int i = 0; i < 400 && bt.phase != BP_CHOOSE; i++) avanza(16);
  bt.b.side[BT_FOE].hpMax = 9999;
  bt.b.side[BT_FOE].hp = 9999;
  Pet petBat = pet;
  BattleUi btBat = bt;
  for (const PantallaT &p : pantallasT()) {
    if (p.nombre.rfind("troca_", 0) == 0) continue;   // pelo caminho real, abaixo
    pet = p.combate ? petBat : petCasa;
    bt = p.combate ? btBat : btCasa;
    todoCerradoT();
    p.monta();
    avanza(160);
    fotoMan(p.nombre.c_str());
  }
  pet = petBat;
  bt = btBat;
  todoCerradoT();

  // ---- o combate: forcejeo da bola, vitoria, insignia
  bt.ballId = BALL_ID[0];
  bt.foeInBall = true;
  bt.phase = BP_WOBBLE;
  bt.wobble = 3;
  bt.taps = 3;
  bt.until = millis() + 2600;
  avanza(100);
  fotoMan("bat_bola");
  bt.foeInBall = false;
  bt.winDex = pet.speciesId;
  bt.expWon = 137;
  bt.loot[0] = 1;
  bt.loot[1] = 2;
  bt.lvlAntes = 29;
  bt.lvlWon = 1;
  bt.phase = BP_WIN;
  avanza(500);
  fotoMan("bat_vitoria");
  if (pet.badges < 1) pet.badges = 1;
  bt.insigniaIdx = 0;
  pet.lastTM = 0;
  bt.phase = BP_BADGE;
  bt.until = millis();
  avanza(120);
  fotoMan("bat_insignia");
  cierraElCombate();
  pet = petCasa;
  bt = btCasa;
  todoCerradoT();
  avanza(200);

  // ---- a troca pelo caminho de verdade (a placa fantasma responde);
  // nao se confirma, entao nada se grava
  cardOpen = true;
  cardPage = 4;
  avanza(160);
  onTap(233, 364);
  for (int i = 0; i < 100 && troca.nViz == 0; i++) avanza(50);
  avanza(400);
  fotoMan("conectar_placas");
  onTap(200, 112 + 20);
  avanza(400);
  onTap(162, 282);                                   // TROCAR
  for (int i = 0; i < 100 && troca.estado != TE_ESCOLHA; i++) avanza(50);
  avanza(400);
  fotoMan("troca_lista");
  onTap(290, 77);
  avanza(400);
  fotoMan("troca_lista_banco");
  onTap(celXT(2), celYT(2, 130));
  avanza(400);
  fotoMan("troca_resumo");
  trocaFecha();
  todoCerradoT();
  avanza(200);

  // ---- pokedex com os tres estados e a cacada
  pet.markSeen(13);
  pet.markSeen(19);
  pet.setCaca(10);
  galleryOpen = true; galleryPage = 0; galleryDetail = 0; galleryDirty = true;
  avanza(200);
  fotoMan("dex_estados");
  galleryDetail = 13; galleryDirty = true;
  avanza(300);
  fotoMan("dex_visto");
  galleryDetail = 10; galleryDirty = true;
  avanza(300);
  fotoMan("dex_cacando");
  todoCerradoT();
  avanza(100);

  // ---- a aventura
  pet.avDesde = 0;
  mockSetEpoch(1760000000);
  onTap(270, 419);
  avanza(1500);
  fotoMan("aventura");
  mockSetEpoch(1760000000 + 3 * 3600);
  onTap(233, 410);
  avanza(300);
  fotoMan("aventura_resumo");
  for (int i = 0; i < 8 && avOpen; i++) onTap(233, 233);
  mockSetEpoch(1758700000);
  todoCerradoT();
  avanza(100);

  // ---- a primeira vez: intro e a escolha do inicial
  startIntro();
  avanza(400);
  fotoMan("intro_titulo");
  introStep = 1;   // IN_OAK
  introPage = 0;
  avanza(300);
  fotoMan("intro_professor");
  introPage = 4;   // a 5a fala: onde fica o manual
  avanza(300);
  fotoMan("intro_manual");
  introStep = 2;   // IN_GENDER
  avanza(300);
  fotoMan("intro_genero");
  introOpen = false;
  todoCerradoT();

  mockSdRoot(sdAntes.c_str());
  setLang(langAntes);
  applyLangFont();
  pet = guarda;
  todoCerradoT();
  avanza(100);
}


// ---- leva 73: os textos de TELA que estavam fixos em ingles. Cada tela
// e aberta em PORTUGUES e o que se escreveu nela (o simulador guarda os
// bytes: mockTextoTodo) tem de trazer a frase traduzida e NAO a inglesa.
// Mede-se o que sai na tela, nao a tabela: a tabela ja estava certa e o
// sketch continuava a escrever o literal.
extern uint8_t healCount;
extern uint32_t healUntil;
extern bool avOpen;
void openClock();

// o til sai como '~' por cima da letra (imprimeConTil)
static std::string comoSai73(const char *s) {
  std::string r;
  for (const unsigned char *p = (const unsigned char *)s; *p; p++) {
    if (*p == 0260) r += "~a";
    else if (*p == 0261) r += "~o";
    else if (*p == 0262) r += "~A";
    else if (*p == 0263) r += "~O";
    else r += (char)*p;
  }
  return r;
}
// o maior pedaco fixo da frase (entre os %s/%u), que e o que se procura
static std::string pedaco73(const char *s) {
  std::string melhor, cur;
  for (const char *p = s; *p; p++) {
    if (*p == '%') {
      if (cur.size() > melhor.size()) melhor = cur;
      cur.clear();
      p++;
      while (*p && strchr("-0123456789l", *p)) p++;
      if (!*p) break;
      continue;
    }
    cur += *p;
  }
  if (cur.size() > melhor.size()) melhor = cur;
  // as pontas com espaco nao contam (os "  -  " do meio sim)
  while (!melhor.empty() && melhor.back() == ' ') melhor.pop_back();
  while (!melhor.empty() && melhor.front() == ' ') melhor.erase(0, 1);
  return melhor;
}
static std::string ingles73(StrId id) {
  Lang a = gLang;
  gLang = LANG_EN;
  std::string s = pedaco73(T(id));
  gLang = a;
  return s;
}
static Lang lang73 = LANG_PT;
static const char *const NOME73[LANG_COUNT] = { "ES", "EN", "FR", "DE", "IT", "PT", "JA", "KO" };
static void abre73() {
  preparaElMundoT();
  setLang(lang73);
  applyLangFont();
  avanza(200);
}
static void fecha73() {
  todoCerradoT();
  cierraElCombate();
  setLang(LANG_EN);
  applyLangFont();
  avanza(100);
}
// confere a tela desenhada desde o ultimo mockTextoTodoReset()
static void confere73(const char *tela0, std::initializer_list<StrId> ids) {
  const std::string &log = mockTextoTodo();
  std::string tela = std::string("[") + NOME73[lang73] + "] " + tela0;
  for (StrId id : ids) {
    // o til so existe nas latinas: em JA/KO 0260/0261 sao bytes de UTF-8
    std::string pt = LANG_IS_CJK(gLang) ? pedaco73(T(id)) : comoSai73(pedaco73(T(id)).c_str());
    std::string en = ingles73(id);
    CHECK_MSG(log.find(pt) != std::string::npos,
              tela + ": nao saiu \"" + pt + "\" (id " + std::to_string((int)id) + "); saiu: ..." +
              log.substr(log.size() > 240 ? log.size() - 240 : 0));
    if (en != pt && en.size() >= 4)
      CHECK_MSG(log.find(en) == std::string::npos,
                tela + ": ainda sai em ingles \"" + en + "\"");
  }
}

static void corpo73_desligar_e_curar() {
  abre73();
  mockTextoTodoReset();
  apagarUntil = millis() + 6000;
  avanza(150);
  confere73("desligar", { S_APA_Q, S_APA_PAUSA, S_APA_PAUSA_SUB, S_APA_DESL,
                          S_APA_DESL_SUB, S_APA_PIE });
  apagarUntil = 0;
  avanza(100);
  mockTextoTodoReset();
  healCount = 3;
  healUntil = millis() + 2200;
  avanza(150);
  confere73("curados", { S_CURADOS, S_CURADOS_FMT });
  healUntil = 0;
  fecha73();
}

static void corpo73_mapa_e_revanche() {
  abre73();
  // um teste anterior deixa maxRegion = 1 (preparaElMundoT nao o zera):
  // a "proxima, ainda fechada" tem de estar fechada de verdade
  const uint8_t maxAntes = pet.maxRegion;
  pet.maxRegion = pet.region;
  CHECK_MSG(!pet.zoneUnlocked((uint8_t)(pet.region + 1)), "premissa: a proxima regiao fechada");
  mockTextoTodoReset();
  mapOpen = true;
  mapView = (uint8_t)(pet.region + 1);    // a proxima, ainda fechada
  avanza(200);
  confere73("mapa fechado", { S_MAPA_VENCE_FMT });
  todoCerradoT();
  pet.badges = pet.gymCount();
  mockTextoTodoReset();
  mapOpen = true;
  mapView = pet.region;
  avanza(200);
  confere73("mapa coronado", { S_REVANCHA });
  todoCerradoT();
  mockTextoTodoReset();
  rematchOpen = true;
  rematchScroll = 0;
  avanza(200);
  confere73("revanche", { S_REVANCHA, S_REV_SUB_FMT, S_FORA_VOLTAR });
  todoCerradoT();
  mockTextoTodoReset();
  cardOpen = true;
  cardPage = 5;
  avanza(200);
  confere73("ginasio coronado", { S_IR_PARA_FMT, S_INSIG_FICAM });
  todoCerradoT();
  // uma regiao ja aberta que nao e esta: tocar para viajar
  pet.advanceRegion();
  if (pet.awaitingStarter()) pet.chooseStarter(1);
  for (int i = 0; i < 50 && pet.isEgg(); i++) pet.eggTap();
  avanza(200);
  todoCerradoT();
  mockTextoTodoReset();
  mapOpen = true;
  mapView = 0;
  avanza(200);
  confere73("mapa outra regiao", { S_MAPA_VIAJAR });
  if (pet.maxRegion < maxAntes) pet.maxRegion = maxAntes;
  fecha73();
}

static void corpo73_a_ficha() {
  abre73();
  mockTextoTodoReset();
  cardOpen = true;
  cardPage = 1;
  avanza(200);
  confere73("combate", { S_HABILIDADE });
  abilShown = true;
  avanza(200);
  confere73("habilidade", { S_TOQUE_FECHAR });
  todoCerradoT();
  mockTextoTodoReset();
  cardOpen = true;
  cardPage = 2;
  moveShown = 0;
  avanza(200);
  confere73("golpes", { S_TOQUE_GOLPE });
  todoCerradoT();
  mockTextoTodoReset();
  cardOpen = true;
  cardPage = 4;
  avanza(200);
  confere73("equipe", { S_EQUIPE_DICA });
  todoCerradoT();
  // o Eevee nao sobe por nivel: precisa de "uma pedra"
  const int16_t especieAntes = pet.speciesId;
  pet.speciesId = 133;
  pet.form = 0;
  mockTextoTodoReset();
  cardOpen = true;
  cardPage = 8;
  avanza(200);
  confere73("progresso", { S_PRECISA_FMT, S_UMA_PEDRA });
  pet.speciesId = especieAntes;      // o titular volta: os testes seguintes contam com ele
  fecha73();
}

static void corpo73_a_mochila() {
  abre73();
  for (uint8_t s = 0; s < 3; s++) {
    pet.bagSort = s;
    mockTextoTodoReset();
    cardOpen = true;
    cardPage = 3;
    bagPocket = BP_HELD;
    avanza(200);
    confere73("ordem", { s == 0 ? S_ORD_BOLSA : s == 1 ? S_ORD_AZ : S_ORD_TIPO,
                         S_BOLSO_0, S_BOLSO_1, S_BOLSO_2, S_BOLSO_3, S_BOLSO_4 });
    todoCerradoT();
  }
  pet.bagSort = 0;
  // o objeto que ele segura: "SEGURA" na linha, "TIRAR" na ficha
  uint8_t obj = primerObjetoT(BP_HELD, 0xFF);
  pet.heldItem = obj;
  mockTextoTodoReset();
  cardOpen = true;
  cardPage = 3;
  bagPocket = BP_HELD;
  avanza(200);
  confere73("linha segurada", { S_EQUIPADO });
  bagShown = obj;
  avanza(200);
  confere73("tirar", { S_TIRAR });
  todoCerradoT();
  pet.heldItem = 0;
  mockTextoTodoReset();
  cardOpen = true;
  cardPage = 3;
  bagPocket = BP_HELD;
  bagShown = obj;
  avanza(200);
  confere73("segurar", { S_SEGURAR });
  todoCerradoT();
  // bolso vazio (o das pedras: o das chaves nunca esvazia)
  for (uint16_t i = 1; i < ITEM_COUNT; i++)
    if (ITEM_TBL[i].pocket == BP_EVO) pet.bag[i] = 0;
  mockTextoTodoReset();
  cardOpen = true;
  cardPage = 3;
  bagPocket = BP_EVO;
  avanza(200);
  confere73("bolso vazio", { S_VAZIO });
  fecha73();
}

static void corpo73_a_despedida_mostra_o_ovo() {
  abre73();
  mockTextoTodoReset();
  choiceKind = 2;
  choiceUntil = millis() + 12000;
  avanza(150);
  confere73("despedida", { S_OVO_FMT });
  choiceKind = 0;
  fecha73();
}

static void corpo73_a_batalha() {
  abre73();
  pet.energy = 100;
  pet.hpLeft = pet.maxHp();
  battleStart(16, 10, -1, false, -1);
  for (int i = 0; i < 400 && bt.phase != BP_CHOOSE; i++) avanza(16);
  CHECK_MSG(bt.phase == BP_CHOOSE, "premissa: o combate tem que abrir");
  bt.b.side[BT_FOE].hp = bt.b.side[BT_FOE].hpMax = 9999;
  // um do time desmaiado, para o "desmaiou"
  for (uint8_t i = 0; i < TEAM_SLOTS; i++)
    if (i != pet.mainSlot && pet.team[i].species) { pet.team[i].hpLeft = 0; break; }
  mockTextoTodoReset();
  bt.phase = BP_PARTY; bt.itemPick = 0; bt.mustSwitch = false;
  avanza(200);
  confere73("equipe", { S_CARD_TEAM, S_EM_CAMPO, S_DESMAIOU, S_FORA_VOLTAR });
  mockTextoTodoReset();
  bt.mustSwitch = true;
  avanza(200);
  confere73("quem agora", { S_QUEM_PROXIMO, S_MANDE_ALGUEM });
  bt.mustSwitch = false;
  mockTextoTodoReset();
  bt.itemPick = primerObjetoT(BP_BATTLE, IU_POKEMON);
  avanza(200);
  confere73("em quem", { S_EM_QUEM_FMT, S_FORA_BOLSA });
  bt.itemPick = 0;
  mockTextoTodoReset();
  bt.phase = BP_BAG; bt.bagPocket = 0; bt.bagScroll = 0;
  avanza(200);
  confere73("bolsa", { S_CARD_BAG, S_BOLAS, S_CURA, S_FORA_VOLTAR });
  for (uint16_t i = 1; i < ITEM_COUNT; i++)
    if (ITEM_TBL[i].pocket == BP_BATTLE) pet.bag[i] = 0;
  mockTextoTodoReset();
  avanza(200);
  confere73("bolsa vazia", { S_NADA_AQUI });
  mockTextoTodoReset();
  bt.winDex = pet.speciesId;
  bt.expWon = 137;
  bt.loot[0] = 1;
  bt.loot[1] = 2;
  bt.lvlAntes = 29;
  bt.lvlWon = 1;
  bt.phase = BP_WIN;
  avanza(500);
  confere73("vitoria", { S_VITORIA, S_OUTROS_EXP_FMT, S_GANHOU, S_INTRO_TAP });
  bt.loot[0] = bt.loot[1] = 0;
  fecha73();
}

static void corpo73_a_aventura() {
  abre73();
  mockSetEpoch(1760000000);
  pet.avDesde = 0;
  mockTextoTodoReset();
  onTap(270, 419);
  avanza(600);
  CHECK_MSG(avOpen, "premissa: a aventura abriu");
  confere73("aventura", { S_AV_TERMINAR });
  // acabar na hora: curto demais
  mockTextoTodoReset();
  onTap(233, 410);
  avanza(300);
  confere73("resumo curto", { S_AVENTURA, S_AV_FORA_FMT, S_AV_CURTO, S_BACK });
  for (int i = 0; i < 8 && avOpen; i++) onTap(233, 233);
  // oito horas (o teto): sobe de nivel e tem paginas
  pet.exp = Pet::expForLevel(5);
  pet.avDesde = 0;
  avanza(200);
  onTap(270, 419);
  avanza(600);
  mockSetEpoch(1760000000 + 8 * 3600);
  mockTextoTodoReset();
  onTap(233, 410);
  avanza(300);
  confere73("resumo", { S_AV_SUBIU_FMT, S_AV_PROX_FMT });
  for (int i = 0; i < 8 && avOpen; i++) onTap(233, 233);
  mockSetEpoch(1758700000);
  fecha73();
}

// Cada tela nas OITO linguas: o pedaco tem de sair inteiro (o que nao cabe
// no cristal sai cortado com "..", e ai o pedaco nao aparece).
TEST(telas73, desligar_e_curar) {
  for (int li = 0; li < LANG_COUNT; li++) {
    lang73 = (Lang)li;
    corpo73_desligar_e_curar();
  }
  lang73 = LANG_PT;
}
TEST(telas73, mapa_e_revanche) {
  for (int li = 0; li < LANG_COUNT; li++) {
    lang73 = (Lang)li;
    corpo73_mapa_e_revanche();
  }
  lang73 = LANG_PT;
}
TEST(telas73, a_ficha) {
  for (int li = 0; li < LANG_COUNT; li++) {
    lang73 = (Lang)li;
    corpo73_a_ficha();
  }
  lang73 = LANG_PT;
}
TEST(telas73, a_mochila) {
  for (int li = 0; li < LANG_COUNT; li++) {
    lang73 = (Lang)li;
    corpo73_a_mochila();
  }
  lang73 = LANG_PT;
}
TEST(telas73, a_despedida_mostra_o_ovo) {
  for (int li = 0; li < LANG_COUNT; li++) {
    lang73 = (Lang)li;
    corpo73_a_despedida_mostra_o_ovo();
  }
  lang73 = LANG_PT;
}
TEST(telas73, a_batalha) {
  for (int li = 0; li < LANG_COUNT; li++) {
    lang73 = (Lang)li;
    corpo73_a_batalha();
  }
  lang73 = LANG_PT;
}
TEST(telas73, a_aventura) {
  for (int li = 0; li < LANG_COUNT; li++) {
    lang73 = (Lang)li;
    corpo73_a_aventura();
  }
  lang73 = LANG_PT;
}

// Os BOTOES e janelas com largura fixa: o corte do cristal (acima) nao ve um
// texto que transborda da caixa mas ainda cabe na tela. Cada caixa com o
// texto que leva, nas oito linguas, ao tamanho em que e desenhado.
// O simulador NAO tem a unifont (U8g2lib.h do shim a deixa em nullptr) e
// mede o japones e o coreano com a letra latina, byte a byte: 3x o real.
// Na placa a unifont tem 16 px por letra larga e 8 por ASCII, a escala e
// tam/CJK_SIZE_DIV (minimo 1) e o printT() engorda 1 px. E isso que se conta.
static int largCJK73(const char *s, uint8_t tam) {
  int w = 0;
  for (const unsigned char *p = (const unsigned char *)s; *p; p++) {
    if ((*p & 0xC0) == 0x80) continue;          // continuacao do UTF-8
    w += (*p >= 0xE0) ? 16 : 8;
  }
  int esc = tam >= 2 ? tam / 2 : 1;
  return w * esc + 1;
}
TEST(telas73, os_textos_cabem_nos_botoes) {
  arranca();
  Lang antes = gLang;
  struct Caixa { StrId id; uint8_t tam; int larg; const char *onde; };
  const Caixa CX73[] = {
    { S_APA_Q, 2, 270, "janela de desligar" },
    { S_APA_PAUSA, 2, 230, "botao pausar" }, { S_APA_DESL, 2, 230, "botao desligar" },
    { S_APA_PAUSA_SUB, 1, 230, "botao pausar" }, { S_APA_DESL_SUB, 1, 230, "botao desligar" },
    { S_APA_PAUSADO, 3, 400, "aviso" }, { S_APA_TCHAU, 3, 400, "aviso" },
    { S_CURADOS, 3, 304, "janela da enfermeira" }, { S_CURADOS_FMT, 2, 304, "janela da enfermeira" },
    { S_REVANCHA, 2, 180, "botao do mapa" }, { S_REVANCHA, 3, 380, "titulo" },
    { S_TIRAR, 2, 132, "botao da mochila" }, { S_SEGURAR, 2, 132, "botao da mochila" },
    { S_BAG_USAR, 2, 132, "botao da mochila" },
    { S_AV_TERMINAR, 2, 138, "botao da aventura" },
    { S_VITORIA, 3, 342, "janela da vitoria" }, { S_AVENTURA, 3, 360, "titulo" },
    { S_BOLSO_0, 1, 52, "aba" }, { S_BOLSO_1, 1, 52, "aba" }, { S_BOLSO_2, 1, 52, "aba" },
    { S_BOLSO_3, 1, 52, "aba" }, { S_BOLSO_4, 1, 52, "aba" },
    { S_BOLAS, 2, 112, "aba da batalha (com o numero)" }, { S_CURA, 2, 112, "aba da batalha (com o numero)" },
    { S_ORD_BOLSA, 2, 280, "ordenar" }, { S_ORD_AZ, 2, 280, "ordenar" }, { S_ORD_TIPO, 2, 280, "ordenar" },
  };
  for (int li = 0; li < LANG_COUNT; li++) {
    setLang((Lang)li);
    applyLangFont();
    for (const Caixa &c : CX73) {
      char b[96];
      snprintf(b, sizeof b, T(c.id), 6u);   // os %u com um numero de verdade
      int w = LANG_IS_CJK((Lang)li) ? largCJK73(b, c.tam) : (int)textW(b, c.tam);
      CHECK_MSG(w <= c.larg, std::string("[") + NOME73[li] + "] \"" + b + "\" mede " +
                std::to_string(w) + " px e o " + c.onde + " tem " + std::to_string(c.larg));
    }
  }
  setLang(antes);
  applyLangFont();
}


// ---- leva 74: o manual dentro do jogo. Um botao MANUAL no topo dos
// ajustes abre um QR que leva ao docs/manual do repo (LEIAME em portugues,
// README nos outros), e o Carvalho ganha uma 5a fala dizendo onde ele esta.
// Mede-se a TELA: cada modulo do QR desenhado tem de bater com a tabela
// gerada (e o tools/confere_qr.py le o PPM com uma camera de verdade, cv2).
#include "../qr_manual.h"
extern int clockH;
uint8_t tallaQueCabe(const char *s, int y, uint8_t max, int extra);

// modulos desenhados que nao batem com a tabela (+ margem que nao e branca)
static int qrErros74(const uint8_t *qr) {
  const uint16_t *fb = gfx->getFramebuffer();
  const int W = gfx->width();
  const int mod = 6, lado = (QR_MANUAL_N + 8) * mod;
  const int x0 = 233 - lado / 2, y0 = 233 - lado / 2;
  auto claro = [&](int x, int y) { return ((fb[y * W + x] >> 11) & 31) > 15; };
  int erros = 0;
  for (int r = 0; r < QR_MANUAL_N; r++)
    for (int c = 0; c < QR_MANUAL_N; c++) {
      bool escuro = qr[r * QR_MANUAL_BPL + c / 8] & (0x80 >> (c % 8));
      int x = x0 + (c + 4) * mod + mod / 2, y = y0 + (r + 4) * mod + mod / 2;
      if (escuro == claro(x, y)) erros++;
    }
  // a margem de 4 modulos (o leitor precisa dela): toda branca
  for (int k = 0; k < QR_MANUAL_N + 8; k++) {
    int p = x0 + k * mod + mod / 2;
    if (!claro(p, y0 + mod * 2)) erros++;
    if (!claro(p, y0 + lado - mod * 2)) erros++;
    if (!claro(x0 + mod * 2, p)) erros++;
    if (!claro(x0 + lado - mod * 2, p)) erros++;
  }
  return erros;
}

TEST(manual74, o_botao_do_topo_abre_o_qr_do_idioma) {
  preparaElMundoT();
  setLang(LANG_PT);
  applyLangFont();
  avanza(100);
  openClock();
  avanza(100);
  CHECK_MSG(!manualQrOpen, "premissa: os ajustes abrem sem o QR");
  onTap(233, 22);
  avanza(100);
  CHECK_MSG(manualQrOpen, "tocar MANUAL no topo dos ajustes abre o QR");
  CHECK_MSG(qrErros74(QR_MANUAL_PT) == 0,
            "o QR desenhado em portugues nao bate com QR_MANUAL_PT: " +
            std::to_string(qrErros74(QR_MANUAL_PT)) + " modulos");
  CHECK_MSG(qrErros74(QR_MANUAL_EN) > 40, "premissa: os dois QR sao diferentes");
  volcaPantalla("qr74_pt");
  // qualquer toque fecha o QR e continua nos ajustes
  onTap(233, 233);
  avanza(100);
  CHECK_MSG(!manualQrOpen, "um toque no QR fecha o QR");
  CHECK_MSG(clockOpen, "fechar o QR volta aos ajustes, nao a casa");
  // em ingles (e nos outros) e o README
  setLang(LANG_EN);
  applyLangFont();
  onTap(233, 22);
  avanza(100);
  CHECK_MSG(qrErros74(QR_MANUAL_EN) == 0,
            "o QR em ingles nao bate com QR_MANUAL_EN: " +
            std::to_string(qrErros74(QR_MANUAL_EN)) + " modulos");
  volcaPantalla("qr74_en");
  // deslizar com o QR aberto fecha so o QR
  onSwipeV(1);
  avanza(100);
  CHECK_MSG(!manualQrOpen && clockOpen, "deslizar no QR fecha so o QR");
  // fora do botao (e do foso) no topo nao abre nada
  onTap(233 - 66 - UI_FOSO - 12, 64);
  avanza(100);
  CHECK_MSG(!manualQrOpen, "tocar ao lado do botao nao abre o QR");
  // e os botoes da hora continuam la
  int h0 = clockH;
  onTap(199, 219);
  CHECK_MSG(clockH == (h0 + 1) % 24, "o + da hora ainda funciona");
  todoCerradoT();
  avanza(100);
}

TEST(manual74, os_textos_do_qr_e_do_carvalho_cabem) {
  for (int li = 0; li < LANG_COUNT; li++) {
    if (LANG_IS_CJK((Lang)li)) continue;   // o simulador nao tem a fonte CJK
    preparaElMundoT();
    setLang((Lang)li);
    applyLangFont();
    avanza(100);
    const std::string nome = std::to_string(li);
    // o botao: o rotulo inteiro, dentro da pilula
    mockTextoTodoReset();
    openClock();
    avanza(100);
    CHECK_MSG(mockTextoTodo().find(T(S_MANUAL)) != std::string::npos,
              "[" + nome + "] o botao MANUAL nao saiu inteiro");
    CHECK_MSG((int)textW(T(S_MANUAL), 2) <= 132 - 12 || (int)textW(T(S_MANUAL), 1) <= 132 - 12,
              "[" + nome + "] MANUAL nao cabe na pilula");
    // a dica no tamanho 2: no 1 fica minuscula e ninguem le (leva 74b)
    CHECK_MSG(tallaQueCabe(T(S_QR_DICA), 233 - (QR_MANUAL_N + 8) * 3 - 22, 2, 0) == 2,
              "[" + nome + "] a dica do QR encolheu para o tamanho 1: " + std::string(T(S_QR_DICA)));
    // a tela do QR: titulo, dica e rodape inteiros (sem o '..' do recorte)
    mockTextoTodoReset();
    onTap(233, 22);
    avanza(100);
    const std::string &log = mockTextoTodo();
    for (StrId id : { S_MANUAL, S_QR_DICA, S_TOQUE_FECHAR })
      CHECK_MSG(log.find(comoSai73(T(id))) != std::string::npos,
                "[" + nome + "] na tela do QR nao saiu inteiro: " + comoSai73(T(id)));
    todoCerradoT();
    // a 5a fala do Carvalho: todas as palavras (4 linhas; o que sobra some)
    startIntro();
    introStep = 1;
    introPage = 4;
    mockTextoTodoReset();
    avanza(300);
    const std::string fala = comoSai73(T(S_OAK_5));
    const std::string &lf = mockTextoTodo();
    size_t a = 0;
    while (a < fala.size()) {
      size_t b = fala.find(' ', a);
      if (b == std::string::npos) b = fala.size();
      std::string w = fala.substr(a, b - a);
      if (!w.empty())
        CHECK_MSG(lf.find(w) != std::string::npos,
                  "[" + nome + "] a 5a fala do Carvalho perdeu \"" + w + "\"");
      a = b + 1;
    }
    introOpen = false;
    todoCerradoT();
  }
  setLang(LANG_EN);
  applyLangFont();
  avanza(100);
}

TEST(manual74, o_carvalho_fala_cinco_vezes) {
  preparaElMundoT();
  startIntro();
  introStep = 1;
  introPage = 0;
  avanza(200);
  for (int i = 0; i < 4; i++) {
    onTap(233, 233);
    avanza(200);
    CHECK_MSG(introStep == 1, "o Carvalho parou antes da 5a fala (pagina " + std::to_string(i + 1) + ")");
  }
  CHECK_MSG(introPage == 4, "premissa: chegou a 5a fala");
  onTap(233, 233);
  avanza(200);
  CHECK_MSG(introStep == 2, "depois da 5a fala vem menino/menina");
  introOpen = false;
  todoCerradoT();
  avanza(100);
}


// ---- leva 75: as mensagens de combate saem no idioma. Mede-se o que a
// CAIXA disse num turno de verdade (bt.msg a cada quadro), golpe a golpe,
// e que toda frase cabe na caixa com nomes compridos.
int btParteLinhas(const char *s, int maxW, uint8_t talla, char lin[3][sizeof(bt.msg)], int maxLin);
const char *moveName(uint16_t mv);
const char *btFraseCarga(uint16_t g);

static uint16_t golpe75(const char *n) {
  for (uint16_t m = 0; m < MOVE_COUNT; m++)
    if (!strcmp(MOVE_TBL[m].name, n)) return m;
  return 0;
}

// um turno: eu uso `meu`, o rival `dele`; devolve tudo o que a caixa disse
static std::string turno75(Lang lg, const char *meu, const char *dele) {
  preparaElMundoT();
  setLang(lg);
  applyLangFont();
  pet.energy = 100;
  // premissa imposta: com vinculo >= OBEY_BOND_OK ele SEMPRE obedece
  // (sem isso o 'nao obedece' era sorteado e o teste passava por sorte)
  pet.bond = 100;
  pet.hpLeft = pet.maxHp();
  battleStart(16, 10);
  for (int i = 0; i < 400 && bt.phase != BP_CHOOSE; i++) avanza(16);
  Combatant &yo = bt.b.side[BT_PLAYER];
  Combatant &el = bt.b.side[BT_FOE];
  yo.moves[0] = golpe75(meu); yo.usesLeft[0] = 30; yo.nMoves = 1;
  el.moves[0] = golpe75(dele); el.usesLeft[0] = 30; el.nMoves = 1;
  yo.hpMax = yo.hp = 900;
  el.hpMax = el.hp = 900;
  // eu primeiro (a ordem das frases fica fixa): o rival no nivel 2. O MEU
  // nao sobe: acima do que as insignias deixam ele desobedece (sorteado)
  el.level = 2;
  bt.sel = 0;
  toca(233, (BT_CARD_Y0_T + BT_CARD_Y1_T) / 2);
  std::string guion, ultima;
  bool salio = false;
  for (int i = 0; i < 900; i++) {
    avanza(16);
    if (bt.phase != BP_CHOOSE) salio = true;
    if (bt.msg[0] && ultima != bt.msg) {
      ultima = bt.msg;
      guion += ultima + "\n";
    }
    if (salio && bt.phase == BP_CHOOSE) break;
  }
  cierraElCombate();
  todoCerradoT();
  return guion;
}

static std::string comT75(Lang lg, StrId id) {
  Lang a = gLang;
  gLang = lg;
  std::string s = T(id);
  gLang = a;
  return s;
}

TEST(bat75, um_turno_sai_no_idioma) {
  for (int li = 0; li < LANG_COUNT; li++) {
    const Lang lg = (Lang)li;
    const std::string nome = NOME73[li];
    std::string g = turno75(lg, "GROWL", "SPLASH");
    // "X usou GROWL!": o pedaco fixo da frase do idioma
    std::string usou = pedaco73(comT75(lg, S_BM_USED).c_str());
    CHECK_MSG(g.find(usou) != std::string::npos,
              "[" + nome + "] o turno nao disse \"" + usou + "\":\n" + g);
    // o atributo que caiu: nome do atributo e a frase inteira no idioma
    std::string atq = comT75(lg, S_BM_ATTACK);
    CHECK_MSG(g.find(atq) != std::string::npos,
              "[" + nome + "] o Growl nao disse o atributo \"" + atq + "\":\n" + g);
    if (lg != LANG_EN) {
      CHECK_MSG(g.find(" used ") == std::string::npos && g.find(" fell!") == std::string::npos,
                "[" + nome + "] ainda sai em ingles:\n" + g);
    }
  }
  setLang(LANG_EN);
  applyLangFont();
}

// o clima e a carga vinham de TABELAS: o perigo e a tabela guardar o texto
// do primeiro idioma e nao mudar mais. Troca-se o idioma e confere-se.
TEST(bat75, tabelas_seguem_o_idioma) {
  setLang(LANG_EN);
  std::string en = btFraseCarga(golpe75("FLY"));
  setLang(LANG_PT);
  std::string pt = btFraseCarga(golpe75("FLY"));
  CHECK_MSG(en != pt, "a frase do Fly nao mudou com o idioma: " + pt);
  CHECK_EQ(pt, comT75(LANG_PT, S_BM_FLEW_UP_HIGH));
  for (int li = 0; li < LANG_COUNT; li++) {
    std::string g = turno75((Lang)li, "SUNNYDAY", "SPLASH");
    std::string sol = comT75((Lang)li, S_BM_THE_SUNLIGHT_TURNED_HARSH);
    CHECK_MSG(g.find(sol) != std::string::npos,
              "[" + std::string(NOME73[li]) + "] o Sunny Day nao disse \"" + sol + "\":\n" + g);
  }
  setLang(LANG_EN);
  applyLangFont();
}

// toda frase cabe na caixa (2 linhas, letra pequena no pior caso) com
// nomes de 12 letras e numeros de 3 algarismos. No CJK o simulador nao
// tem a fonte: conta-se 16 px por letra larga e 8 por ASCII.
TEST(bat75, toda_frase_cabe_na_caixa) {
  static const StrId FRAG[] = { S_BM_ATTACK, S_BM_DEFENSE, S_BM_SP_ATK, S_BM_SP_DEF,
                                S_BM_SPEED, S_BM_ACCURACY, S_BM_EVASION, S_BM_THE_HOLD,
                                S_BM_THE_CURSE, S_BM_ITS_BURN, S_BM_POISON, S_BM_THE_SEED,
                                S_BM_THE_SANDSTORM, S_BM_THE_HAIL, S_BM_THE_WEATHER };
  for (int li = 0; li < LANG_COUNT; li++) {
    setLang((Lang)li);
    applyLangFont();
    // o NOME mais comprido de verdade neste idioma (Pokemon, golpe, objeto,
    // habilidade): em japones um nome de 6 kana sao 18 bytes, e a conta com
    // 12 letras fixas escondia isso
    std::string nomeMax;
    auto ve = [&](const char *n) { if (n && strlen(n) > nomeMax.size()) nomeMax = n; };
    for (int d = 1; d <= DEX_COUNT; d++) ve(dexName((int16_t)d));
    for (uint16_t m = 0; m < MOVE_COUNT; m++) ve(moveName(m));   // o da tela (leva 76)
    for (uint16_t i = 1; i < ITEM_COUNT; i++) ve(ITEM_TBL[i].name);
    for (uint16_t i = 1; i < ABIL_COUNT; i++) ve(abilAt(i).name);
    const char *NOME = nomeMax.c_str();
    // o fragmento mais comprido de cada familia entra no lugar do %s dele
    std::string statMax, causaMax;
    for (int k = 0; k < 7; k++) if (strlen(T(FRAG[k])) > statMax.size()) statMax = T(FRAG[k]);
    for (int k = 7; k < 15; k++) if (strlen(T(FRAG[k])) > causaMax.size()) causaMax = T(FRAG[k]);
    for (int id = S_BM_SENDS; id <= S_BM_ABILITY; id++) {
      bool frag = false;
      for (StrId f : FRAG) frag |= (f == id);
      if (frag) continue;
      const char *t = T((StrId)id);
      // monta a frase: %s -> nome (ou o fragmento, no segundo %s das duas que usam)
      std::string m;
      int ns = 0;
      for (const char *p = t; *p; p++) {
        if (p[0] == '%' && p[1] == 's') {
          bool seg = ns++ == 1;
          if (seg && (id == S_BM_STAT_UP || id == S_BM_STAT_DOWN)) m += statMax;
          else if (seg && id == S_BM_IS_HURT_BY) m += causaMax;
          else m += NOME;
          p++;
        } else if (p[0] == '%' && (p[1] == 'u' || (p[1] == 'l' && p[2] == 'u'))) {
          m += "999";
          p += p[1] == 'l' ? 2 : 1;
        } else {
          m += *p;
        }
      }
      // e cabe no buffer: o snprintf cortaria calado (JA/KO = 3 bytes por letra)
      CHECK_MSG(m.size() < sizeof(bt.msg), "[" + std::string(NOME73[li]) + "] id " +
                std::to_string(id) + " passa de " + std::to_string(sizeof(bt.msg)) + " bytes: " + m);
      if (LANG_IS_CJK((Lang)li)) {
        int w = 0;
        for (const unsigned char *p = (const unsigned char *)m.c_str(); *p;) {
          if (*p < 0x80) { w += 8; p++; }
          else { w += 16; p += (*p & 0xF0) == 0xE0 ? 3 : 2; }
        }
        CHECK_MSG(w <= 3 * 276 - 40, "[" + std::string(NOME73[li]) + "] id " +
                  std::to_string(id) + " nao cabe nem em 3 linhas: " + m);
      } else {
        char lin[3][sizeof(bt.msg)];
        CHECK_MSG(btParteLinhas(m.c_str(), 276, 1, lin, 2) >= 0,
                  "[" + std::string(NOME73[li]) + "] id " + std::to_string(id) +
                  " nao cabe na caixa: " + m);
      }
    }
  }
  setLang(LANG_EN);
  applyLangFont();
}

// a caixa parte pela largura: nenhuma linha passa do maximo, nada se perde
TEST(bat75, a_caixa_parte_pela_largura_sem_perder_texto) {
  setLang(LANG_PT);
  applyLangFont();
  const char *s = "Pikachu esta preparado para usar Thunderbolt no proximo turno";
  char lin[3][sizeof(bt.msg)];
  int n = btParteLinhas(s, 276, 1, lin, 2);
  CHECK_MSG(n == 2, "premissa: a frase precisa de 2 linhas na letra pequena");
  std::string junto;
  for (int i = 0; i < n; i++) {
    CHECK_MSG((int)textW(lin[i], 1) <= 276, std::string("linha larga demais: ") + lin[i]);
    junto += (i ? " " : "") + std::string(lin[i]);
  }
  CHECK_EQ(junto, std::string(s));
  CHECK_MSG(btParteLinhas(s, 276, 2, lin, 2) < 0, "na letra grande nao cabe em 2: tem de avisar (-1)");
  setLang(LANG_EN);
  applyLangFont();
}


// ---- Nidoran: femea e macho com o SIMBOLO nas linguas latinas. A fonte
// classica (CP437) tem os dois nos codigos 12 e 11; mede-se que eles SE
// DESENHAM: a 8a casa do nome tem exatamente os pixels do glifo (17 pontos
// do femea e 13 do macho, x4 na talla 2). No JA/KO a fonte U8g2 nao tem os
// simbolos: ali o nome nao pode levar byte de controle.
TEST(nidoran, femea_e_macho_com_o_simbolo) {
  for (int li = 0; li < LANG_COUNT; li++) {
    setLang((Lang)li);
    applyLangFont();
    const std::string nome = NOME73[li];
    for (int16_t d : { (int16_t)29, (int16_t)32 }) {
      const char *n = dexName(d);
      bool controle = false;
      for (const unsigned char *p = (const unsigned char *)n; *p; p++) controle |= *p < 0x20;
      if (LANG_IS_CJK((Lang)li)) {
        CHECK_MSG(!controle, "[" + nome + "] nome do Nidoran com byte de controle: a fonte U8g2 nao tem");
        continue;
      }
      CHECK_EQ(std::string(n), std::string(d == 29 ? "NIDORAN\014" : "NIDORAN\013"));
      gfx->fillScreen(0);
      gfx->setTextColor(0xFFFF);
      setSize(2);
      setCur(100, 200);
      printT(n);
      int pts = pixelesEn(100 + 7 * 12, 200, 12, 16, 0);
      CHECK_MSG(pts == (d == 29 ? 68 : 52),
                "[" + nome + "] o simbolo do Nidoran " + std::to_string(d) +
                " nao se desenhou: " + std::to_string(pts) + " pixels na 8a casa");
    }
  }
  setLang(LANG_EN);
  applyLangFont();
  avanza(100);
}


// ---- previa da leva 75 (so com TAMA_PREV75=1): um turno em PT contra um
// Nidoran femea, uma foto da tela a cada mensagem nova da caixa.
TEST(prev75, fotos_do_combate_traduzido) {
  if (!getenv("TAMA_PREV75")) return;
  const Lang lg = (Lang)atoi(getenv("TAMA_PREV75"));
  preparaElMundoT();
  setLang(lg);
  applyLangFont();
  pet.energy = 100;
  pet.bond = 100;
  pet.hpLeft = pet.maxHp();
  battleStart(29, 12);
  for (int i = 0; i < 400 && bt.phase != BP_CHOOSE; i++) avanza(16);
  volcaPantalla((std::string("p75_") + NOME73[lg] + "_0").c_str());
  Combatant &yo = bt.b.side[BT_PLAYER];
  Combatant &el = bt.b.side[BT_FOE];
  yo.moves[0] = golpe75("GROWL"); yo.usesLeft[0] = 30; yo.nMoves = 1;
  el.moves[0] = golpe75("SUNNYDAY"); el.usesLeft[0] = 30; el.nMoves = 1;
  el.level = 2;
  bt.sel = 0;
  toca(233, (BT_CARD_Y0_T + BT_CARD_Y1_T) / 2);
  std::string ultima;
  int n = 1;
  bool salio = false;
  for (int i = 0; i < 900 && n < 9; i++) {
    avanza(16);
    if (bt.phase != BP_CHOOSE) salio = true;
    if (bt.msg[0] && ultima != bt.msg) {
      ultima = bt.msg;
      avanza(120);
      volcaPantalla((std::string("p75_") + NOME73[lg] + "_" + std::to_string(n++)).c_str());
    }
    if (salio && bt.phase == BP_CHOOSE) break;
  }
  cierraElCombate();
  todoCerradoT();
  setLang(LANG_EN);
  applyLangFont();
  avanza(100);
}


// ---- o estado que entra no que PEGA: a frase diz o nome DELE. Era
// "KINGDRA foi envenenado!" quando o Poison Point do Kingdra envenenava
// o rival que bateu nele (o dono viu jogando). Mede-se a caixa.
static uint16_t habil76(const char *n) {
  for (uint16_t i = 1; i < ABIL_COUNT; i++)
    if (!strcmp(abilAt(i).name, n)) return i;
  return 0;
}

// um turno em que o rival usa `dele` em mim; eu tenho a habilidade `hab`
static std::string turnoLado(const char *meu, const char *dele, const char *hab,
                             std::string &euNome, std::string &eleNome) {
  preparaElMundoT();
  setLang(LANG_PT);
  applyLangFont();
  pet.energy = 100;
  pet.bond = 100;
  pet.hpLeft = pet.maxHp();
  battleStart(16, 10);
  for (int i = 0; i < 400 && bt.phase != BP_CHOOSE; i++) avanza(16);
  Combatant &yo = bt.b.side[BT_PLAYER];
  Combatant &el = bt.b.side[BT_FOE];
  yo.moves[0] = golpe75(meu); yo.usesLeft[0] = 30; yo.nMoves = 1;
  el.moves[0] = golpe75(dele); el.usesLeft[0] = 30; el.nMoves = 1;
  yo.hpMax = yo.hp = 900;
  el.hpMax = el.hp = 900;
  if (hab) yo.ability = habil76(hab);
  yo.status = ST_NONE;
  el.status = ST_NONE;
  euNome = dexName(yo.pareceDex());
  eleNome = dexName(el.pareceDex());
  bt.sel = 0;
  mockForceRandom(0);          // os 30% da habilidade de contato saem sempre
  toca(233, (BT_CARD_Y0_T + BT_CARD_Y1_T) / 2);
  std::string guion, ultima;
  bool salio = false;
  for (int i = 0; i < 900; i++) {
    avanza(16);
    if (bt.phase != BP_CHOOSE) salio = true;
    if (bt.msg[0] && ultima != bt.msg) { ultima = bt.msg; guion += ultima + "\n"; }
    if (salio && bt.phase == BP_CHOOSE) break;
  }
  mockClearForcedRandom();
  cierraElCombate();
  todoCerradoT();
  setLang(LANG_EN);
  applyLangFont();
  avanza(100);
  return guion;
}

TEST(lado76, poison_point_envenena_quem_bateu_e_a_frase_diz_ele) {
  std::string eu, ele;
  std::string g = turnoLado("SPLASH", "TACKLE", "Poison Point", eu, ele);
  CHECK_MSG(habil76("Poison Point") != 0, "premissa: a habilidade existe");
  // a caixa esta em PT: desde a leva 78 diz o nome portugues da habilidade
  setLang(LANG_PT);
  const std::string pp = abilName(habil76("Poison Point"));
  setLang(LANG_EN);
  CHECK_MSG(g.find(pp) != std::string::npos, "premissa: a habilidade disparou (" + pp + "):\n" + g);
  // T(S_BM_WAS_POISONED) em PT e "%s foi envenenado!"
  CHECK_MSG(g.find(ele + " foi envenenado!") != std::string::npos,
            "o rival que bateu e quem se envenena, e a caixa nao disse:\n" + g);
  CHECK_MSG(g.find(eu + " foi envenenado!") == std::string::npos,
            "a caixa disse que o MEU se envenenou (o dono do Poison Point):\n" + g);
}

TEST(lado76, rest_faz_dormir_quem_usou_e_a_frase_diz_ele) {
  std::string eu, ele;
  std::string g = turnoLado("REST", "SPLASH", nullptr, eu, ele);
  // T(S_BM_FELL_ASLEEP) em PT e "%s adormeceu!"
  CHECK_MSG(g.find(eu + " adormeceu!") != std::string::npos,
            "quem usou Rest e quem dorme, e a caixa nao disse:\n" + g);
  CHECK_MSG(g.find(ele + " adormeceu!") == std::string::npos,
            "a caixa disse que o RIVAL dormiu com o meu Rest:\n" + g);
}


// ---- quiz: depois de responder a proxima vem SOZINHA (o dono pediu: so
// passava com outro toque). Acerto 1,5 s; erro 2,5 s (para ler a certa).
// A tela final (o premio) continua esperando o toque.
TEST(quiz_ui, sem_tocar_a_proxima_vem_sozinha) {
  arranca();
  todoCerradoT();
  if (pet.awaitingStarter()) pet.chooseStarter(1);
  while (pet.isEgg()) pet.eggTap();
  Pet guarda = pet;
  randomSeed(7601);
  quizStart();
  avanza(100);
  // acerto: ainda mostrando aos 1,3 s, ja na proxima aos 1,7 s
  quizTap(233, (int16_t)qzCentroY(quizQ.right));
  avanza(1300);
  CHECK_MSG(quizIdx == 0 && quizPick >= 0, "passou cedo demais depois do acerto");
  avanza(400);
  CHECK_MSG(quizIdx == 1 && quizPick == -1, "depois do acerto nao passou sozinha");
  // erro: espera mais (2,5 s)
  quizTap(233, (int16_t)qzCentroY((quizQ.right + 1) % QUIZ_OPTS));
  avanza(2300);
  CHECK_MSG(quizIdx == 1 && quizPick >= 0, "depois do erro passou antes dos 2,5 s");
  avanza(400);
  CHECK_MSG(quizIdx == 2 && quizPick == -1, "depois do erro nao passou sozinha");
  // ate o fim sem tocar: a tela do premio fica (nao fecha sozinha)
  for (int k = 0; k < 8 && !quizEnd; k++) {
    quizTap(233, (int16_t)qzCentroY(quizQ.right));
    avanza(2700);
  }
  CHECK_MSG(quizEnd, "o quiz nao chegou ao fim passando sozinho");
  avanza(5000);
  CHECK_MSG(quizOpen && quizEnd, "a tela do premio fechou sem o toque");
  quizTap(233, 233);
  avanza(100);
  CHECK_MSG(!quizOpen, "o toque na tela do premio nao fechou");
  pet = guarda;
  todoCerradoT();
}


// ---- leva 76: o nome OFICIAL do golpe no idioma. Todo golpe, em cada
// lugar onde aparece, fica dentro da sua caixa: nenhum pixel passa da borda
// (printNaLargura encolhe e, se preciso, corta com ".."). Latinas so: no
// simulador nao ha fonte CJK.
uint8_t printNaLargura(const char *s, int x, int y, int maxW, uint8_t talla);
const char *moveName(uint16_t mv);

static int pixelsDireita76(int x0, int y0, int h) {
  const uint16_t *fb = gfx->getFramebuffer();
  int n = 0;
  for (int y = y0; y < y0 + h; y++)
    for (int x = x0; x < 466; x++)
      if (fb[y * 466 + x]) n++;
  return n;
}

TEST(nomes76, todo_golpe_fica_dentro_da_caixa) {
  struct Lugar { const char *nome; int maxW; uint8_t talla; bool podeCortar; };
  // o espaco do golpe na ficha (2x2), a lista da ficha, o cartao do combate
  const Lugar L[] = { { "espaco da ficha", 118, 1, true },
                      { "lista da ficha", 200, 2, false },
                      { "cartao do combate", 192, 2, false } };
  for (int li = 0; li < LANG_COUNT; li++) {
    if (LANG_IS_CJK((Lang)li)) continue;
    setLang((Lang)li);
    applyLangFont();
    int cortados = 0;
    for (uint16_t m = 1; m < MOVE_COUNT; m++) {
      const char *nm = moveName(m);
      for (const Lugar &l : L) {
        gfx->fillScreen(0);
        gfx->setTextColor(0xFFFF);
        uint8_t t = printNaLargura(nm, 100, 200, l.maxW, l.talla);
        int fora = pixelsDireita76(100 + l.maxW, 196, 24);
        CHECK_MSG(fora == 0, std::string("[") + NOME73[li] + "] " + nm + " passa da borda do " +
                  l.nome + ": " + std::to_string(fora) + " pixels");
        bool cabe = (int)textW(nm, t) <= l.maxW;
        if (!cabe) {
          cortados++;
          CHECK_MSG(l.podeCortar, std::string("[") + NOME73[li] + "] " + nm +
                    " teve de ser cortado no " + l.nome);
        }
      }
    }
    // hoje so o TORMENTA DE DIAMANTES (ES) se corta no espaco da ficha
    CHECK_MSG(cortados <= 2, std::string("[") + NOME73[li] + "] " + std::to_string(cortados) +
              " nomes cortados: a caixa ficou pequena demais para os nomes do idioma");
  }
  setLang(LANG_EN);
  applyLangFont();
  avanza(100);
}

// o nome na tela segue o idioma (e o id nao muda: e ele que o motor usa)
TEST(nomes76, o_cartao_do_combate_mostra_o_nome_do_idioma) {
  for (int li = 0; li < LANG_COUNT; li++) {
    preparaElMundoT();
    setLang((Lang)li);
    applyLangFont();
    pet.energy = 100;
    pet.hpLeft = pet.maxHp();
    battleStart(16, 10);
    for (int i = 0; i < 400 && bt.phase != BP_CHOOSE; i++) avanza(16);
    Combatant &yo = bt.b.side[BT_PLAYER];
    uint16_t g = golpe75("THUNDERBOLT");
    yo.moves[0] = g; yo.usesLeft[0] = 30; yo.nMoves = 1;
    bt.sel = 0;
    mockTextoTodoReset();
    avanza(200);
    std::string nm = moveName(g);
    CHECK_MSG(mockTextoTodo().find(nm) != std::string::npos,
              std::string("[") + NOME73[li] + "] o cartao nao mostrou " + nm);
    if (li != LANG_EN && li != LANG_PT)
      CHECK_MSG(nm != "THUNDERBOLT", std::string("[") + NOME73[li] + "] sem nome proprio");
    CHECK_MSG(!strcmp(MOVE_TBL[g].name, "THUNDERBOLT"), "o id do golpe mudou");
    cierraElCombate();
    todoCerradoT();
  }
  setLang(LANG_EN);
  applyLangFont();
  avanza(100);
}


// ---- previa da leva 76 (so com TAMA_PREV76=<idioma>): o cartao do golpe no
// combate, a ficha de golpes e uma pergunta do quiz sobre golpe.
extern uint8_t quizIdx;
TEST(prev76, fotos_dos_nomes_de_golpe) {
  if (!getenv("TAMA_PREV76")) return;
  const Lang lg = (Lang)atoi(getenv("TAMA_PREV76"));
  const std::string pre = std::string("p76_") + NOME73[lg] + "_";
  preparaElMundoT();
  setLang(lg);
  applyLangFont();
  pet.energy = 100;
  pet.hpLeft = pet.maxHp();
  battleStart(16, 10);
  for (int i = 0; i < 400 && bt.phase != BP_CHOOSE; i++) avanza(16);
  Combatant &yo = bt.b.side[BT_PLAYER];
  const char *G[4] = { "THUNDERBOLT", "DIAMONDSTORM", "SUNNYDAY", "DESLIZAMIENTO" };
  for (int k = 0; k < 4; k++) { yo.moves[k] = golpe75(G[k]); yo.usesLeft[k] = 10; }
  yo.nMoves = 4;
  for (int k = 0; k < 2; k++) {
    bt.sel = (uint8_t)k;
    avanza(200);
    volcaPantalla((pre + "cartao" + std::to_string(k)).c_str());
  }
  cierraElCombate();
  todoCerradoT();
  // a ficha de golpes: os quatro do principal
  for (int k = 0; k < 4; k++) pet.ppMove[k] = golpe75(G[k]);
  cardOpen = true;
  cardPage = 2;
  moveShown = 0;
  avanza(300);
  volcaPantalla((pre + "ficha").c_str());
  todoCerradoT();
  // o quiz: uma pergunta de tipo de golpe
  quizStart();
  for (int t = 0; t < 40 && quizQ.kind != QZ_MOVE_TYPE; t++) quizUna(quizQ, QZ_MOVE_TYPE);
  avanza(300);
  volcaPantalla((pre + "quiz").c_str());
  quizOpen = false;
  todoCerradoT();
  setLang(LANG_EN);
  applyLangFont();
  avanza(100);
}


// ---- leva 77: o nome do TIPO no idioma (o quiz perguntava em espanhol e
// respondia FIRE). Em cada idioma, cada tipo cabe nos chips de largura
// fixa: o da Pokedex (104), o da ficha (108), as opcoes do ginasio (96) e
// o saco do ginasio (72, que cresce). No CJK o simulador nao tem a fonte:
// conta-se 16 px por letra larga.
TEST(tipos77, o_nome_do_tipo_e_do_idioma_e_cabe_nos_chips) {
  const int FIXOS[] = { 104, 108, 96, 72 };
  for (int li = 0; li < LANG_COUNT; li++) {
    setLang((Lang)li);
    applyLangFont();
    for (uint8_t t = 1; t < T_COUNT; t++) {
      const char *n = typeName(t);
      int w;
      if (LANG_IS_CJK((Lang)li)) {
        w = 0;
        for (const unsigned char *p = (const unsigned char *)n; *p;) {
          if (*p < 0x80) { w += 8; p++; } else { w += 16; p += (*p & 0xF0) == 0xE0 ? 3 : 2; }
        }
      } else {
        w = (int)textW(n, 1);
      }
      for (int f : FIXOS) {
        // o saco cresce (min 72): conta a largura que ele vai ter
        int chip = f == 72 ? (w + 12 > 72 ? w + 12 : 72) : f;
        CHECK_MSG(w + 8 <= chip, std::string("[") + NOME73[li] + "] tipo " + n +
                  " (" + std::to_string(w) + " px) nao cabe no chip de " + std::to_string(f));
      }
    }
    // nao-ingles: o nome e o do idioma, nao o rotulo ingles
    if (li != LANG_EN) {
      bool algumDiferente = false;
      for (uint8_t t = 1; t < T_COUNT; t++)
        algumDiferente |= strcmp(typeName(t), TYPE_NAME[t]) != 0;
      CHECK_MSG(algumDiferente, std::string("[") + NOME73[li] + "] os tipos sairam em ingles");
    }
  }
  // o ingles nao mudou (o chip do golpe foi medido com FIGHT/ELEC/PSY)
  setLang(LANG_EN);
  applyLangFont();
  for (uint8_t t = 0; t < T_COUNT; t++) CHECK_EQ(std::string(typeName(t)), std::string(TYPE_NAME[t]));
  avanza(100);
}

// e o cartao do golpe no combate mostra o tipo do idioma
TEST(tipos77, o_chip_do_golpe_no_combate_esta_no_idioma) {
  for (int li = 0; li < LANG_COUNT; li++) {
    preparaElMundoT();
    setLang((Lang)li);
    applyLangFont();
    pet.energy = 100;
    pet.hpLeft = pet.maxHp();
    battleStart(16, 10);
    for (int i = 0; i < 400 && bt.phase != BP_CHOOSE; i++) avanza(16);
    Combatant &yo = bt.b.side[BT_PLAYER];
    uint16_t g = golpe75("FLAMETHROWER");
    yo.moves[0] = g; yo.usesLeft[0] = 30; yo.nMoves = 1;
    bt.sel = 0;
    mockTextoTodoReset();
    avanza(200);
    // da TABELA do idioma, nao de typeName(): comparar com a propria funcao
    // era tautologia (a sabotagem que a fazia ignorar o idioma passava)
    std::string tipo = TYPE_NAME_L[li][MOVE_TBL[g].type];
    CHECK_MSG(mockTextoTodo().find(tipo) != std::string::npos,
              std::string("[") + NOME73[li] + "] o chip nao disse " + tipo);
    cierraElCombate();
    todoCerradoT();
  }
  setLang(LANG_EN);
  applyLangFont();
  avanza(100);
}


#include "../item_names.h"
#include "../abil_names.h"
const char *drawWrapped(const char *s, int y0, int cols, int lh, int maxLines, uint8_t talla);

// ---- leva 78: nome e descricao de ITEM e HABILIDADE no idioma ----
// item_names.h / abil_names.h (tools/gen_textos_itens.py). As tabelas sao
// indexadas pela linha de ITEM_TBL/ABIL_TBL: a assinatura (o nome ingles de
// cada linha) tem de bater, senao a tela le o VIZINHO.
TEST(textos78, as_assinaturas_batem_linha_a_linha) {
  for (int i = 0; i < ITEM_COUNT; i++)
    CHECK_MSG(strcmp(ITEM_NAME_SIG[i], ITEM_TBL[i].name) == 0,
              std::string("item ") + std::to_string(i) + ": " + ITEM_NAME_SIG[i] + " != " + ITEM_TBL[i].name);
  for (int i = 0; i < ABIL_COUNT; i++)
    CHECK_MSG(strcmp(ABIL_NAME_SIG[i], ABIL_TBL[i].name) == 0,
              std::string("habil ") + std::to_string(i) + ": " + ABIL_NAME_SIG[i] + " != " + ABIL_TBL[i].name);
}

// Em cada idioma o nome e a descricao saem do idioma -- nao do ingles.
TEST(textos78, nomes_e_descricoes_sao_do_idioma) {
  for (int li = 0; li < LANG_COUNT; li++) {
    setLang((Lang)li);
    int nDifI = 0, dDifI = 0, comDescI = 0, nDifA = 0, dDifA = 0, comDescA = 0;
    for (int i = 1; i < ITEM_COUNT; i++) {
      CHECK_MSG(itemName((uint8_t)i) && *itemName((uint8_t)i), std::string("[") + NOME73[li] + "] item sem nome " + std::to_string(i));
      nDifI += strcmp(itemName((uint8_t)i), ITEM_TBL[i].name) != 0;
      if (*ITEM_DESC[i]) { comDescI++; dDifI += strcmp(itemDesc((uint8_t)i), ITEM_DESC[i]) != 0; }
    }
    for (int i = 1; i < ABIL_COUNT; i++) {
      CHECK_MSG(abilName((uint16_t)i) && *abilName((uint16_t)i), std::string("[") + NOME73[li] + "] habil sem nome " + std::to_string(i));
      nDifA += strcmp(abilName((uint16_t)i), ABIL_TBL[i].name) != 0;
      if (*ABIL_DESC[i]) { comDescA++; dDifA += strcmp(abilDesc((uint16_t)i), ABIL_DESC[i]) != 0; }
    }
    if (li == LANG_EN) {
      CHECK_EQ(nDifI + nDifA + dDifI + dDifA, 0);
      continue;
    }
    std::string q = std::string("[") + NOME73[li] + "] ";
    // nomes: muitos coincidem de verdade (megas, "Pokedex"); descricoes quase nenhuma
    CHECK_MSG(nDifI * 2 > ITEM_COUNT, q + "nomes de item em ingles: " + std::to_string(nDifI));
    CHECK_MSG(nDifA * 2 > ABIL_COUNT, q + "nomes de habilidade em ingles: " + std::to_string(nDifA));
    CHECK_MSG(dDifI * 100 >= comDescI * 95, q + "descricoes de item em ingles: " + std::to_string(comDescI - dDifI));
    CHECK_MSG(dDifA * 100 >= comDescA * 95, q + "descricoes de habilidade em ingles: " + std::to_string(comDescA - dDifA));
  }
  setLang(LANG_EN);
  applyLangFont();
}

// O corte do paragrafo passou a ser pela LARGURA. No latino tem de dar
// EXATAMENTE o mesmo que o corte por bytes de antes (copiado aqui), pagina
// a pagina, em todas as descricoes dos seis idiomas latinos.
static const char *wrapAntigo78(const char *s, int cols, int maxLines) {
  if (!s || !*s) return s;
  int n = 0;
  while (*s && n < maxLines) {
    while (*s == ' ') s++;
    if (!*s) break;
    int corte = 0, ultimo = 0;
    while (s[corte] && corte < cols) {
      if (s[corte] == ' ') ultimo = corte;
      corte++;
    }
    if (s[corte] && ultimo) corte = ultimo;
    s += corte;
    n++;
  }
  while (*s == ' ') s++;
  return s;
}

TEST(textos78, a_quebra_latina_nao_mudou) {
  int conf = 0;
  for (int li = 0; li < LANG_COUNT; li++) {
    if (LANG_IS_CJK((Lang)li)) continue;
    setLang((Lang)li);
    applyLangFont();
    std::vector<const char *> todos;
    for (int i = 1; i < ITEM_COUNT; i++) todos.push_back(itemDesc((uint8_t)i));
    for (int i = 1; i < ABIL_COUNT; i++) todos.push_back(abilDesc((uint16_t)i));
    for (const char *s : todos) {
      for (int cols : { 25, 26 }) {
        const char *a = s, *b = s;
        for (int pag = 0; pag < 20 && a && *a; pag++) {
          const char *na = drawWrapped(a, 200, cols, 18, 2, 2);
          const char *nb = wrapAntigo78(b, cols, 2);
          CHECK_MSG(na == nb, std::string("[") + NOME73[li] + "] corte mudou em: " + s);
          if (na != nb) break;
          a = na; b = nb;
          conf++;
        }
      }
    }
  }
  CHECK_MSG(conf > 5000, "premissa: conferiu poucas paginas");
  setLang(LANG_EN);
  applyLangFont();
}

// Japones nao tem espacos e cada letra sao 3 bytes: o corte por bytes partia
// a letra ao meio. Pagina a pagina, o que sai tem de ser UTF-8 valido e
// trazer o texto inteiro, em ordem (o simulador pinta cada linha duas vezes:
// a pseudo-negrita; por isso "em ordem" e subsequencia).
static bool utf8Valido78(const std::string &s) {
  for (size_t i = 0; i < s.size();) {
    unsigned char c = (unsigned char)s[i];
    int n = c < 0x80 ? 1 : (c & 0xE0) == 0xC0 ? 2 : (c & 0xF0) == 0xE0 ? 3 : (c & 0xF8) == 0xF0 ? 4 : 0;
    if (!n || i + n > s.size()) return false;
    for (int k = 1; k < n; k++)
      if (((unsigned char)s[i + k] & 0xC0) != 0x80) return false;
    i += n;
  }
  return true;
}

TEST(textos78, japones_e_coreano_nao_partem_letra) {
  for (Lang lg : { LANG_JA, LANG_KO }) {
    setLang(lg);
    applyLangFont();
    std::vector<const char *> todos;
    for (int i = 1; i < ITEM_COUNT; i++) todos.push_back(itemDesc((uint8_t)i));
    for (int i = 1; i < ABIL_COUNT; i++) todos.push_back(abilDesc((uint16_t)i));
    // e as mesmas SEM espacos: o japones dos jogos nem sempre os tem, e com
    // espaco o corte cai nele e nunca no meio de uma letra (a sabotagem que
    // partia a letra passava)
    std::vector<std::string> semEsp;
    for (const char *s : todos) {
      std::string t;
      for (const char *q = s; *q; q++) if (*q != ' ') t += *q;
      semEsp.push_back(t);
    }
    for (const std::string &t : semEsp) todos.push_back(t.c_str());
    int partidas = 0, perdidas = 0;
    std::string exemplo;
    for (const char *s : todos) {
      mockTextoTodoReset();
      const char *p = s;
      for (int pag = 0; pag < 60 && p && *p; pag++) p = drawWrapped(p, 200, 25, 18, 2, 2);
      CHECK_MSG(!p || !*p, std::string("nao chegou ao fim: ") + s);
      const std::string &out = mockTextoTodo();
      if (!utf8Valido78(out)) { partidas++; if (exemplo.empty()) exemplo = s; }
      // subsequencia, sem os espacos
      size_t j = 0;
      bool ok = true;
      for (const char *q = s; *q; q++) {
        if (*q == ' ') continue;
        j = out.find(*q, j);
        if (j == std::string::npos) { ok = false; break; }
        j++;
      }
      if (!ok) perdidas++;
    }
    CHECK_MSG(partidas == 0, std::string("[") + NOME73[lg] + "] letras partidas em " + std::to_string(partidas) + " textos, ex.: " + exemplo);
    CHECK_MSG(perdidas == 0, std::string("[") + NOME73[lg] + "] texto perdido em " + std::to_string(perdidas));
  }
  setLang(LANG_EN);
  applyLangFont();
}

// A ficha do objeto na mochila mostra o nome e a descricao do idioma. Troca o
// idioma com a ficha ABERTA: o ponteiro da pagina era do idioma velho.
// O esperado sai das TABELAS (nao de itemName/itemDesc: comparar a tela com a
// propria funcao era tautologia -- a sabotagem que a fazia ignorar o idioma passava).
static const char *const *const NOME_ITEM78[LANG_COUNT] = {
  ITEM_NAME_ES, nullptr, ITEM_NAME_FR, ITEM_NAME_DE, ITEM_NAME_IT, ITEM_NAME_PT, ITEM_NAME_JA, ITEM_NAME_KO };
static const char *const *const DESC_ITEM78[LANG_COUNT] = {
  ITEM_DESC_ES, nullptr, ITEM_DESC_FR, ITEM_DESC_DE, ITEM_DESC_IT, ITEM_DESC_PT, ITEM_DESC_JA, ITEM_DESC_KO };
TEST(textos78, a_ficha_do_objeto_esta_no_idioma) {
  preparaElMundoT();
  uint8_t restos = itemPorNomeT("Leftovers");
  CHECK_MSG(restos, "premissa: Leftovers");
  if (pet.bag[restos] == 0) pet.bag[restos] = 1;
  if (pet.heldItem == restos) pet.holdItem(0);
  cardOpen = true;
  cardPage = 3;
  bagPocket = BP_HELD;
  bagShown = restos;
  avanza(100);
  for (int li = 0; li < LANG_COUNT; li++) {
    setLang((Lang)li);
    applyLangFont();
    mockTextoTodoReset();
    avanza(100);
    const std::string &out = mockTextoTodo();
    std::string q = std::string("[") + NOME73[li] + "] ";
    const char *nomeEsp = NOME_ITEM78[li] ? NOME_ITEM78[li][restos] : ITEM_TBL[restos].name;
    CHECK_MSG(out.find(nomeEsp) != std::string::npos, q + "a lista nao disse " + nomeEsp);
    // o comeco da descricao: ate o 1o espaco (ou 3 letras largas)
    const char *d = DESC_ITEM78[li] && DESC_ITEM78[li][restos] ? DESC_ITEM78[li][restos] : ITEM_DESC[restos];
    int n = 0, letras = 0;
    while (d[n] && d[n] != ' ' && letras < 3) {
      unsigned char c = (unsigned char)d[n];
      int cl = !LANG_IS_CJK((Lang)li) || c < 0x80 ? 1 : (c & 0xE0) == 0xC0 ? 2 : (c & 0xF0) == 0xE0 ? 3 : 4;
      n += cl;
      if (cl > 1) letras++;
    }
    std::string ini(d, n);
    CHECK_MSG(out.find(ini) != std::string::npos, q + "a ficha nao comeca por " + ini);
  }
  setLang(LANG_EN);
  applyLangFont();
  todoCerradoT();
}

// O cuadro da habilidade tinha 4 linhas e nada mais: em alemao a descricao
// mais longa nao cabe. Agora pagina: cada toque mostra a seguinte, o ultimo
// fecha. Lendo todas as paginas, tem de aparecer o texto inteiro.
TEST(textos78, o_cuadro_da_habilidade_pagina_ate_o_fim) {
  for (Lang lg : { LANG_DE, LANG_FR, LANG_JA }) {
    preparaElMundoT();
    setLang(lg);
    applyLangFont();
    uint16_t maior = 1;
    for (uint16_t i = 1; i < ABIL_COUNT; i++)
      if (strlen(abilDesc(i)) > strlen(abilDesc(maior))) maior = i;
    std::string d = abilDesc(maior);
    // no japones a maior cabe em 4 linhas (19 letras por linha): so tem de abrir e fechar
    bool longo = !LANG_IS_CJK(lg);
    CHECK_MSG(!longo || d.size() > 4 * 26, std::string("premissa: a maior nao passa de 4 linhas: ") + d);
    pet.ability = maior;
    cardOpen = true;
    cardPage = 1;
    abilShown = true;
    std::string lido;
    int toques = 0;
    while (abilShown && toques < 10) {
      mockTextoTodoReset();
      avanza(100);
      lido += mockTextoTodo();
      onTap(220, 220);
      toques++;
      avanza(50);
    }
    CHECK_MSG(!abilShown, "o ultimo toque nao fechou");
    CHECK_MSG(!longo || toques >= 2, std::string("premissa: uma pagina so (") + d + ")");
    // cada palavra (latino) / cada letra (japones) aparece
    size_t j = 0;
    bool ok = true;
    for (char c : d) {
      if (c == ' ') continue;
      j = lido.find(c, j);
      if (j == std::string::npos) { ok = false; break; }
      j++;
    }
    CHECK_MSG(ok, std::string("[") + NOME73[lg] + "] parte do texto nunca apareceu: " + d);
    todoCerradoT();
  }
  setLang(LANG_EN);
  applyLangFont();
  avanza(50);
}

// A mochila em ordem alfabetica: a do NOME QUE SE LE.
TEST(textos78, a_mochila_ordena_pelo_nome_do_idioma) {
  preparaElMundoT();
  for (uint8_t i = 1; i < ITEM_COUNT; i++)
    if (ITEM_TBL[i].pocket == BP_HELD && pet.bag[i] == 0) pet.bag[i] = 1;
  pet.bagSort = 1;
  for (Lang lg : { LANG_FR, LANG_DE, LANG_JA }) {
    setLang(lg);
    uint8_t n = pet.pocketCount(BP_HELD);
    CHECK_MSG(n > 10, "premissa: poucos objetos");
    int foraIdioma = 0, foraIngles = 0;
    for (uint8_t k = 1; k < n; k++) {
      uint8_t a = pet.pocketItem(BP_HELD, k - 1), b = pet.pocketItem(BP_HELD, k);
      foraIdioma += strcmp(itemName(a), itemName(b)) > 0;
      foraIngles += strcmp(ITEM_TBL[a].name, ITEM_TBL[b].name) > 0;
    }
    CHECK_MSG(foraIdioma == 0, std::string("[") + NOME73[lg] + "] fora de ordem: " + std::to_string(foraIdioma));
    CHECK_MSG(foraIngles > 0, std::string("premissa [") + NOME73[lg] + "]: a ordem do idioma e a do ingles");
  }
  pet.bagSort = 0;
  setLang(LANG_EN);
  todoCerradoT();
}


// fotos para aprovar (leva 78): TAMA_PREV78=1 ./build/tamapoke_uitests prev78
TEST(prev78, fotos_dos_itens_e_habilidades) {
  if (!getenv("TAMA_PREV78")) return;
  for (Lang lg : { LANG_PT, LANG_DE, LANG_FR, LANG_ES }) {
    const std::string pre = std::string("p78_") + NOME73[lg] + "_";
    preparaElMundoT();
    setLang(lg);
    applyLangFont();
    // a mochila (objetos que se levam) com a ficha dos Restos
    for (uint8_t i = 1; i < ITEM_COUNT; i++)
      if (ITEM_TBL[i].pocket == BP_HELD && pet.bag[i] == 0) pet.bag[i] = 1;
    uint8_t restos = itemPorNomeT("Leftovers");
    if (pet.heldItem == restos) pet.holdItem(0);
    pet.bagSort = 1;
    cardOpen = true;
    cardPage = 3;
    bagPocket = BP_HELD;
    bagShown = restos;
    avanza(200);
    volcaPantalla((pre + "mochila").c_str());
    todoCerradoT();
    // o cuadro da habilidade: a de descricao mais longa, pagina 1 e 2
    uint16_t maior = 1;
    for (uint16_t i = 1; i < ABIL_COUNT; i++)
      if (strlen(abilDesc(i)) > strlen(abilDesc(maior))) maior = i;
    pet.ability = maior;
    cardOpen = true;
    cardPage = 1;
    abilShown = true;
    avanza(200);
    volcaPantalla((pre + "habil1").c_str());
    onTap(220, 220);
    avanza(200);
    if (abilShown) volcaPantalla((pre + "habil2").c_str());
    pet.bagSort = 0;
    todoCerradoT();
  }
  setLang(LANG_EN);
  applyLangFont();
}
