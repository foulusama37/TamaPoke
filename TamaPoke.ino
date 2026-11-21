// TamaPoke - tamagotchi pixel art inspirado en la gen 1
// para Waveshare ESP32-S3-Touch-AMOLED-1.75
//
// Librerias (Library Manager o repo de Waveshare):
//   - "GFX Library for Arduino" (moononournation), con soporte CO5300 QSPI
//   - "SensorLib" (Lewis He), driver tactil CST9217
//
// Placa: ESP32S3 Dev Module | Flash 16MB | PSRAM: OPI PSRAM | USB CDC On Boot: Enabled
//
// Los sprites y la tabla de especies se generan con tools/sprites.py (emit).

#include <Arduino.h>
#include <Wire.h>
#include "Arduino_GFX_Library.h"
#include "TouchDrvCSTXXX.hpp"
#include <U8g2lib.h>  // fuentes CJK (japones); ver applyLangFont()
#include "pin_config.h"
#include "species.h"
#include "dex.h"
#include "dex_text.h"
#include "moves.h"
#include "quiz.h"      // el cuestionario del Profesor
#include "battle.h"
#include "battle_ui.h"
#include "pet.h"
#include "troca.h"
#include "troca_radio.h"
#include "duelo.h"
#include "pvp.h"
#include "imu.h"
#include "sdmon.h"
#include "retratos.h"   // POR_ESPECIES / POR_RANURAS / os estados
#include "creditos.h"   // quem desenhou o retrato de cada especie
#include "usbdisco.h"
#include "rtcbat.h"
#include "i18n.h"
#include "audio.h"
#include "intro.h"        // intro de primera partida (creditos, profesor)
#include "bgs.h"          // que escenario de /bg va en cada sitio

// Version del firmware. Subir este numero en cada release (y manifest.json para
// el instalador web). Se muestra en la pantalla de ajustes y por serie al arrancar.
#define FW_VERSION "1.41"

// RGB de 8 bits al 565 del panel. Arriba del todo porque una macro es
// texto: usada antes de esta linea, el compilador no la conoce.
#define C565(r, g, b) ((uint16_t)((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3)))

Arduino_DataBus *bus = new Arduino_ESP32QSPI(
  LCD_CS, LCD_SCLK, LCD_SDIO0, LCD_SDIO1, LCD_SDIO2, LCD_SDIO3);
Arduino_CO5300 *panel = new Arduino_CO5300(
  bus, LCD_RESET, 0 /*rotation*/, LCD_WIDTH, LCD_HEIGHT, 6, 0, 0, 0);
// Framebuffer completo en PSRAM: dibujamos todo y hacemos flush() (sin parpadeo)
Arduino_Canvas *gfx = new Arduino_Canvas(LCD_WIDTH, LCD_HEIGHT, panel);

TouchDrvCST92xx touch;
// fuente CJK activa: de momento siempre false, la carga llegara con el
// soporte de japones/coreano/chino (ver PR #13)
bool gCjkFont = false;
#define TOUCH_ADDR 0x5A  // CST9217
Pet pet;

// sprite animado de la SD para la especie actual (si existe el archivo)
SdMon mon;          // sprite B/N (respaldo y minijuego si no hay PMD)
PmdMon pmd;         // sprite PMD multi-accion (pantalla principal)
// Lo ultimo que se pinto, en las DOS unidades que hacen falta:
//
//   ultimaMagPintada   la MAGNITUD dibujada. Es la que se compara con lo
//                      que se pidio, porque pedir y pintar tienen que
//                      estar en la misma unidad. Tenerlo en alto mientras
//                      se pedia magnitud hacia saltar el test con un
//                      fallo inventado (63 contra 75).
//   ultimoAltoPintado  el alto de cuerpo, en pixeles. Este es para
//                      mirarlo con ojos humanos (el comando TALLA).
int16_t ultimaMagPintada = 0;
int16_t ultimoAltoPintado = 0;

// MODO SOMBRA do desenhador. Ligado, drawPmdActM pinta o MESMO frame
// achatado (`achata` do alto) e inclinado (`inclina` do alto, para a
// direita no topo), e em vez de cor ESCURECE o que ja esta na tela por
// num/den. Como sai do frame que se esta vendo, a sombra mexe junto
// com a animacao —— e o que a elipse nao fazia.
SombraModo gSombra;   // a struct mora no battle_ui.h
TintaModo gTinta;     // idem
PmdMon evoPmd;      // forma anterior, solo durante el parpadeo de evolucion
// Y QUE especie era: hace falta para pintarla a SU tamano. Sin esto las dos
// formas salian normalizadas y un Kakuna (160 px en crudo) parecia encoger
// al convertirse en Beedrill (140).
int16_t evoFromDex = 0;
extern uint8_t evoFase;   // la cola sonora de la evolucion (ver evoSonido)
int16_t monFor = -2;

// Los GESTOS del acelerometro, apagados por defecto.
//
// Los umbrales de gestos.h (1,7 g para un meneo, 3 tapas en 1,2 s para
// una caricia...) se escribieron SIN la placa delante: son una
// estimacion mia. Con el aparato en la mano se disparan solos -- el
// dueno lo dijo: "o acelerometro ta disparado" -- y un bicho que se
// cree acariciado o zarandeado estando quieto es peor que uno que no
// siente nada.
//
// Asi que nacen apagados y hay un interruptor. Cuando haya tiempo de
// medir de verdad (leer imuModulo() quieto, andando y meneando, y poner
// los numeros que salgan), se encienden y se queda. Apagarlos NO apaga
// el chip: imuTick() sigue leyendo, que es lo que permite medirlos.
bool gestosOn = false;

// Lo que tarda el dibujo, en ms: el peor de los ultimos frames. De el
// sale la cadencia de repintado (ver loop), para que el redibujado no
// pise el volcado DMA del anterior y aparezcan parpadeos negros.
uint32_t renderPeor = 0;

// El objeto que se esta MIRANDO en la bolsa, y por donde va su texto.
// Antes tocar un objeto lo equipaba en el acto, sin decir que era: el
// dueno se encontro con algo puesto que no sabia identificar y que
// ademas no salia. Ahora tocar ENSENA, y para ponerselo o quitarselo
// hay un boton con su nombre escrito.
uint8_t bagShown = 0;
// "Dar X a quem?": o objeto que a pagina do time esta para entregar (0 =
// nada). Tocar num do time entrega; qualquer outro toque desiste.
uint8_t darItemId = 0;
const char *bagDescPtr = nullptr;
const char *bagDescFin = nullptr;
uint8_t bagDescDe = 0;

bool monShinyFor = false;

// Deformacion que drawPmdActM aplica AL DIBUJAR, para dar peso a un sprite que
// no trae ciclo de andar. Ver petFxFor() para las curvas de cada estado.
struct PetFx {
  float sx = 1.0f, sy = 1.0f;  // squash & stretch (1 = sin tocar)
  float dx = 0.0f, dy = 0.0f;  // desplazamiento en pixeles de pantalla
  float lean = 0.0f;           // cizalla: px que se desplaza la fila mas alta
  float lag = 0.0f;            // retraso de fase de las filas altas (rad)
  float phase = 0.0f;          // fase del ciclo (rad)
  // Linea por debajo de la cual no se dibuja nada. Sirve para que el que cae
  // se meta DENTRO del suelo en vez de seguir pintandose por debajo de el.
  int16_t clipY = 0x7FFF;      // 0x7FFF = sin recorte
};

// Declaradas a mano y AQUI ARRIBA, antes de la primera llamada: el prototipado
// automatico del .ino no lleva los argumentos por defecto, asi que si se queda
// el suyo las llamadas que no pasan `fx` no compilan.
// `dex` = que especie se esta pintando, para darle SU tamano. La Pokedex y
// la evolucion pintan otra especie, asi que no vale mirar a pet.speciesId
// desde dentro. Tres valores con significado:
//
//    -1  el bicho que se cria (pet.speciesId)
//   > 0  esa especie
//     0  EL QUE LLAMA YA DIO EL TAMANO: no tocarlo.
//
// El 0 no es un detalle. El combate pinta DOS bichos con su propia
// perspectiva y durante mucho tiempo llamaba aqui sin pasar `dex`, o sea
// con el -1 por defecto: el sprite del RIVAL acababa escalado por el
// screenSz del bicho del JUGADOR. Subir el tamano del tuyo agrandaba a
// todos los rivales del juego, y asi se reporto — "quando aumenta o
// pokemon do lado do player, deixa o adversario gigante". Cinco intentos
// de ajustar tamanos murieron contra esto, porque el fallo no estaba en
// los numeros sino en quien los aplicaba.
void drawPmdActM(PmdMon &m, uint8_t actId, int cx, int groundY, uint32_t t, bool loop, bool sil,
                 uint8_t maxS, const PetFx *fx = nullptr, int16_t dex = -1);
void drawPmdAct(uint8_t actId, int cx, int groundY, uint32_t t, bool loop, bool sil,
                uint8_t maxS, const PetFx *fx = nullptr, int16_t dex = -1);
// screenSz ya viene calibrado PARA CASA, asi que aqui no hay que corregir
// nada. Antes valia 0,73 porque la tabla estaba calibrada para el combate
// y la casa tenia que deshacerlo; hoy el combate es quien aplica SU factor
// (perspectiva) sobre este. Se deja con nombre y no como literal porque es
// el sitio donde tocar si la pantalla de cuidado se queda corta de hueco.
#define HOME_K 1.0f
// Lo que hay entre la cabecera (acaba en y~105) y la linea de suelo
// (PET_GROUND = 304). Por encima de esto el bicho se mete en el titulo.
#define HOME_ALTO_MAX 190

float altoPedido(int16_t dex, float factor);
float factorTinta(const PmdAct &a, uint8_t s, float quiere, int altoMax);
uint8_t escalaDibujo(const PmdMon &m, const PmdAct &a, uint8_t maxS, float quiere);
int balaoAoLado();
void drawNeedBubble(PetNeed n, int cx, int baseY, int haciaX = -1,
                    uint8_t estadoForzado = 0xFF,
                    const Art *simboloForzado = nullptr);
void tintEllipse(int cx, int cy, int rx, int ry, uint8_t num, uint8_t den, bool aclarar);
// Viven en capture.ino: la regla de cada bola y donde esta su arte.
uint8_t ballMult(uint8_t id);
uint8_t ballArte(uint8_t id);
static void tintRect(int x, int y, int w, int h, uint8_t num, uint8_t den);
static void misturaRect(int x, int y, int w, int h, uint16_t c, uint8_t a);
PetFx petFxFor(uint8_t act, uint32_t now, bool moving);
void drawBattleScene(uint32_t now);
extern bool avOpen;            // aventura.ino
void renderAventura();
void aventuraAbre();
void aventuraComeca();
void btClimaAtras(uint32_t now);
void btClimaFrente(uint32_t now);
bool aventuraTap(int16_t x, int16_t y);
bool gEspelho = false;         // drawPmdActM: desenha de costas para o lado
// Viven en battle_ui.ino, que se concatena DESPUES de este fichero: lo
// que el combate pide de alto de cuerpo y lo que acaba saliendo, por
// lado. Los lee el comando serie TALLA.
extern int16_t btPedido[2];
extern int16_t btAltoDibujado[2];
extern int16_t btAltoReal[2];

// comportamiento del bicho en pantalla
struct {
  uint8_t mode = 0;     // 0 idle, 1 paseo, 2 gesto one-shot
  uint8_t act = PMD_IDLE;
  uint32_t t0 = 0;      // inicio de la animacion en curso
  uint32_t until = 0;   // fin del estado actual
  float x = 233, targetX = 233;
} beh;
#define PET_GROUND 304  // linea de suelo de la mascota
PmdMon galleryPmd;  // sprite grande de la vista detalle de la galeria (PMD/TPK2, legal)

// galeria pokedex
bool galleryOpen = false;
bool galleryDirty = false;
// Cuantas paginas de 16 hacen falta. Sale de DEX_COUNT y no a mano: con
// Johto pasaron de 10 a 16, y un numero fijo habria dejado 100 especies
// sin forma de llegar a ellas.
#define GAL_PAGES ((DEX_COUNT + 15) / 16)
int galleryPage = 0;
int16_t galleryDetail = 0;  // dex en vista detalle, 0 = rejilla
// El ultimo que grito en la Pokedex. La vista detalle se repinta 30 veces
// por segundo; sin esto, el grito se relanzaria en cada fotograma. Se
// compara con `galleryDetail`, que vuelve a 0 al cerrar, asi que reabrir la
// misma ficha vuelve a sonar.
int16_t galleryCried = 0;

// ---- las VERSIONES de un bicho en la ficha de la Pokedex ----
// Un mismo numero de dex puede tener mas de un dibujo que TU hayas tenido:
// el normal, el shiny y las formas regionales (A.RAICHU y companía). Antes
// la ficha enseñaba el shiny siempre que lo tuvieras registrado y no habia
// forma de ver el normal; las formas no se podian ver nunca.
#define GVER_MAX 6
int8_t galleryVer = 0;            // cual se esta mirando
// Lo que es cada version: -1 normal, -2 shiny, >=0 fila de FORM_TBL
int16_t galleryVers[GVER_MAX] = { -1 };
uint8_t galleryNVers = 1;
// El boton de pasar a la siguiente, a la derecha del bicho.
// A seta de versao (shiny, formas, megas). Vive na ESQUERDA, logo
// abaixo das chapas de tipo —— elas ocupam x 26..134, y 164..226 ——
// e acima da caixa de texto, que comeca em y=290. Estava na direita,
// que agora e do retrato.
#define GVER_X 70
#define GVER_Y 258
#define GVER_R 22
// Leva 72: o botao CACAR/PARAR, dentro da caixa do texto (70..396 x 290..374).
// CACAR sobe 8 (CACA_BY - 8) porque sai sozinho, sem o aviso por cima.
#define CACA_BX 133
#define CACA_BY 318
#define CACA_BW 200
#define CACA_BH 50


bool screenOff = false;       // pulsacion corta del boton PWR
bool cardOpen = false;        // ficha del bicho (deslizar vertical)
bool kbOpen = false;          // teclado para renombrar al bicho
char nameBuf[12] = "";
uint8_t nameLen = 0;
// 0 perfil, 1 stats, 2 golpes, 3 bolsa, 4 equipo/banco, 5 gimnasios,
// 6 ficha de entrenador, 7 medallas, 8 progreso
#define CARD_PAGES 9
uint8_t cardPage = 0;
// Arrastando so se passa pelas do BICHO. A 5 (ginasios) e a 7 (medalhas)
// sao da jornada: abrem-se pelos botoes do mapa, "solo" (sem paginar) e, ao
// fechar, volta-se ao mapa. Antes eram nove paginas e o dono achou demais
// (2026-11-13).
static const uint8_t CARD_SEQ[] = { 0, 1, 2, 3, 4, 6, 8 };
#define CARD_SEQ_N ((int)sizeof(CARD_SEQ))
bool cardSolo = false;
// Soltar do banco: o hueco da caixa aberta a confirmar (-1 = nenhum) e
// ate quando a pergunta fica na tela.
int8_t bankSoltar = -1;
uint32_t bankSoltarAte = 0;
bool bankView = false;        // dentro de la pagina de equipo: false = equipo
// Las dos paginas nuevas son LISTAS y no caben enteras: a nivel 50 un bicho
// conoce 17 golpes de media (hasta 31) y la bolsa tiene 61 huecos posibles.
uint8_t cardScroll = 0;       // primera fila visible de la lista
uint8_t bagPocket = 0;        // BP_*: pestana abierta en la bolsa
#define CARD_ROWS 6           // filas que caben entre el titulo y los puntos
// En el BANCO caben cinco: la sexta fila se la queda la barra de la caja.
#define BANK_ROWS 5
#define BANK_BAR_Y0 98
#define BANK_BAR_Y1 122
#define BANK_ROW_Y0 126
// El boton de ordenar la bolsa, entre los bolsillos y la primera fila.
#define BAG_SORT_Y0 108
#define BAG_SORT_Y1 134
// LAS TRES CASILLAS de los entrenadores de la casa, donde estaban las
// ocho insignias.
//
// El ancho lo manda la ALTURA DE ARRIBA, que es donde el circulo es mas
// estrecho: a y=72, la cuerda del circulo de 231 va de x=68 a x=398, o
// sea 330 px. Tres de 100 con 8 de hueco son 316, centradas de 75 a 391.
// Con 126 (lo primero que puse) se salian, y el teste de UI lo caza por
// la marca de "ya esta", que acababa 3 px fuera.
#define GYMTR_X0  75
#define GYMTR_W   100
#define GYMTR_GAP 8
#define GYMTR_Y0  72
#define GYMTR_H   104
// La COLUMNA DE LA DERECHA: nombre, sitio, chapa de tipo, el as y el
// porton. Movida de 210 a 220 junto con el boton, que estaba centrado en
// la pantalla y le comia el sitio al retrato.
#define GYM_COL_X 220

// El retrato del lider va DEBAJO de las casillas, no encima (acaban en
// GYMTR_Y0 + GYMTR_H = 176) y a la izquierda de esa columna.
//
// Baja hasta 350 y no hasta 306: el boton se ha ido a la derecha, asi
// que por debajo de 316 ya no hay nada suyo a la izquierda de 212 y el
// retrato puede seguir bajando. Con eso pasa de 102-122 px de alto a
// 113-166 (media 120 -> 163). Los numeros salen de medir los 117
// retratos contra el circulo, el boton y la columna: con 180 de ancho
// cinco se salian del cristal por abajo a la izquierda, con 176 ninguno.
//
// Ya NO se topa a escala 2: el tope era lo que lo mantenia pequeno. Once
// de los 117 pasan ahora de 2,5x (el mayor, 3,06x) porque su original es
// mas chico; se ven algo mas bastos, pero TODOS salen del mismo tamano
// en pantalla, que es lo que se nota al mirarlos.
#define GYM_LEAD_TOP  184
#define GYM_LEAD_BASE 350
#define GYM_LEAD_CX   121
#define GYM_LEAD_MAXW 176
#define CARD_ROW_Y 146        // primera fila
// LAS PAGINAS CON LISTA (golpes, bolsa, equipo) se cierran tocando ARRIBA,
// en el titulo, o ABAJO, donde pone "tap: back". En medio, un toque que no
// acierta a nada no hace nada: antes cerraba la ficha entera, y las filas
// miden 3 mm.
#define CARD_CIERRA_ARRIBA 50
// (Macro y no funcion: una funcion aqui arriba arrastraria el bloque de
// prototipos del simulador por encima de los tipos.)
#define cardCierraEn(y, fondo) ((y) < CARD_CIERRA_ARRIBA || (y) > (fondo) + UI_FOSO)
// La franja de la habilidad en la pagina de stats, lo que se TOCA.
#define ABIL_Y0 84
#define ABIL_Y1 122
#define CARD_ROW_H 34
// Los cuatro huecos de golpe, en 2x2. El ancho sale de lo que da el circulo a
// la altura de la PRIMERA fila (y=66), que es donde mas estrecho es: media
// cuerda = sqrt(233^2 - 167^2) = 162, o sea de x=71 a x=395.
#define SLOT_X 74
#define SLOT_Y 66
#define SLOT_W 152
#define SLOT_H 28
bool clockOpen = false;       // pantalla de ajuste de hora (deslizar abajo)
int clockH = 12, clockM = 0;  // hora en edicion
// Borrado de fabrica: 0 = nada, 1 = primera pregunta, 2 = segunda. Son DOS
// porque no se deshace, y una sola confirmacion la despacha cualquiera sin
// llegar a leerla.
uint8_t resetStep = 0;
// Lo ultimo que se toco en la pagina de golpes, para poder contar QUE HACE.
// Sin descripcion esa pantalla era una lista de nombres y numeros: no habia
// forma de saber que hace Sweet Scent sin probarlo en un combate.
uint16_t moveShown = 0;
// El texto del golpe no cabe de una a talla 2, asi que se lee por
// paginas: `moveDescPtr` es por donde va y `moveDescFin` lo que devolvio
// el ultimo dibujado (o sea, donde empieza la pagina siguiente).
const char *moveDescPtr = nullptr;
const char *moveDescFin = nullptr;
uint16_t moveDescDe = 0;     // de que golpe es el puntero de arriba
// De QUIEN son los golpes que se estan mirando. 0xFF = el titular (que vive
// en el propio Pet y no en su hueco del equipo); 0..5 = un hueco del equipo.
// Sin esto, para ver los golpes de un suplente habia que hacerlo titular
// primero, y eso cuesta el vinculo del que lo era: mirar no puede costar.
#define MOVES_MAIN 0xFF
uint8_t movesSlot = MOVES_MAIN;
// Lo que la pagina de golpes necesita saber de ese alguien. Las dos mitades
// (pintar y tocar) piden exactamente lo mismo, y repetir la condicion en cada
// uso era la forma segura de que se despegaran. Vive aqui arriba porque el
// generador de prototipos del sketch los emite antes del cuerpo del fichero.
struct MovesView {
  int16_t species = 0;
  uint16_t level = 0;
  uint16_t slotMv[4] = { 0, 0, 0, 0 };   // los cuatro puestos
  uint8_t slotPp[4] = { 0, 0, 0, 0 };
  const char *owner = nullptr;           // nullptr = el titular
};
// Y si esta abierto el cuadro de la habilidad (pagina de combate).
bool abilShown = false;
// El comando serie SETTINGS la abre desde muy arriba del fichero, antes de
// que el preprocesador de Arduino haya generado su prototipo.
void openClock();

// escena de bano: espuma sobre el bicho y limpieza al reventar
uint32_t bathUntil = 0;
bool bathPending = false;
struct { int16_t x, y; uint8_t r, ph; } bubbles[14];
#define BATH_MS 3000
uint32_t feedMenuUntil = 0;   // selector de comida abierto hasta este millis

// ---------- gimnasio: el saco de tipos ----------
// Sustituye a los dos minijuegos de antes (la pelota y el saco de aporrear),
// que no tenian nada de Pokemon: eran "toca rapido". Este usa la MISMA tabla
// de tipos que el combate, asi que jugarlo es practicar lo unico que de
// verdad hay que saber para pelear. Y el premio son EVs del stat que elijas,
// que es lo que el jugador pidio: entrenar deberia ser una decision.
#define GYM_PICK 0    // eligiendo que EV entrenar
#define GYM_ROUND 1   // aporreando el saco
#define GYM_DONE 2    // pantalla de resultado
#define GYM_MS 15000  // lo que dura una sesion
bool gymOpen = false;
// El mapa de regiones. `mapView` es la que se esta MIRANDO, que no tiene por
// que ser en la que estas: con las flechas se pasean todas las abiertas y
// una mas —— la siguiente, con el candado ——, para que se vea que hay mas
// juego detras del campeon y no parezca que Kanto es todo lo que hay.
bool mapOpen = false;
bool trocaOpen = false;   // a tela de troca entre placas (troca_ui.ino)
Troca troca;
uint8_t mapView = 0;
// La pantalla de REVANCHA, que se abre desde el mapa. Declaradas aqui
// arriba porque renderMap y mapTap las usan y viven antes que ellas.
extern bool rematchOpen;
extern uint8_t rematchScroll;
static bool rematchReady();
uint32_t healUntil = 0;   // el cartel de "curados" en pantalla hasta aqui

// ---- el cuestionario del Profesor (quiz.h monta las preguntas) ----
bool quizOpen = false;
QuizQ quizQ;
uint8_t quizIdx = 0;       // que pregunta va, 0..QUIZ_LEN-1
uint8_t quizHits = 0;      // aciertos
int8_t quizPick = -1;      // boton tocado (-1 = todavia nada)
uint32_t quizUntil = 0;    // hasta cuando se ensena el acierto/fallo
bool quizEnd = false;      // pantalla final (marcador y premio)
uint8_t quizPrize = 0;     // objeto entregado, 0 si ninguno
uint8_t quizPrizeQtd = 1;  // cuantos (los 5 de 5 dan dos Rare Candy)
uint8_t quizLastKind = 0xFF;
uint8_t healCount = 0;
uint8_t gymPhase = GYM_PICK;
uint8_t gymStat = EV_ATK;        // que EV se esta entrenando
uint32_t gymUntil = 0, gymDoneUntil = 0;
uint16_t gymScore = 0;
uint8_t gymGain = 0;             // EVs que entraron de verdad
bool gymNewHi = false;
uint8_t gymBagType = 0;          // tipo del saco de ahora
uint8_t gymOpts[3] = { 0, 0, 0 };  // los tres tipos que se pueden lanzar
uint8_t gymChain = 0;            // aciertos seguidos (dan bonus)
int8_t gymVerdict = -1;          // 2 muy eficaz, 1 neutro, 0 resistido
uint32_t gymFlash = 0;           // cuando se anuncio el ultimo golpe
float gymShake = 0;

// las 9 especies con sprite propio en flash (respaldo sin SD): dex -> indice
int flashIdxForDex(int16_t dex) {
  static const int8_t IDX[10] = { -1, 3, 4, 5, 0, 1, 2, 6, 7, 8 };
  return (dex >= 1 && dex <= 9) ? IDX[dex] : -1;
}

#define CX 233  // centro de la pantalla redonda
#define CY 233
#define PET_CY 202  // centro vertical del sprite

static const uint16_t INK_K = 0x18C4;  // spriteColor('k')

// botones de icono siguiendo el arco inferior de la pantalla redonda
// (los exteriores van mas altos para no salirse del circulo)
struct Btn {
  int16_t cx, cy;
  const char *const *icon;
};
// CINCO desde que existe el de cazar. Las posiciones siguen el arco de
// abajo: a x=109 el circulo baja hasta y=430, asi que el centro no puede
// pasar de 404; en el medio hay sitio de sobra. El de cazar va EN MEDIO
// porque es el unico que no es de cuidado, y asi no se confunde con ellos.
// SEIS pelo arco de baixo: raio 190, de 22,7 em 22,7 graus (+-11,35, +-34, +-56,75).
// O passo tem de ser igual no ANGULO: igual na horizontal, no arco as pontas ficam
// mais longe umas das outras. E o de 22,7 e o maior com que os das pontas ainda
// ficam inteiros abaixo da linha do painel (y=306).
#define BTN_N 6
Btn buttons[BTN_N] = {
  {  74, 337, SPR_ICON_FOOD },   // comer
  { 127, 390, SPR_ICON_PLAY },   // jugar
  { 196, 419, nullptr },         // CAZAR (dibuja ART_HIERBA)
  { 270, 419, nullptr },         // a AVENTURA (o mapa)
  { 339, 390, SPR_ICON_LIGHT },  // luz
  { 392, 337, SPR_ICON_CLEAN },  // bano
};
#define BTN_HALF 27  // boton redondo, como los de la batalha
#define BTN_HIT 36   // radio tactil (un poco mas generoso)

// estrellas del modo noche
static const uint16_t STARS[][2] = { {120,140},{330,120},{370,210},{95,230},{280,90},{160,95} };

bool wasPressed = false;
// O MATINHO: o que o sorteio escolheu, esperando o balao acabar.
// Guardado porque sortear outra vez depois do balao daria outro
// bicho —— o jogador veria um e pelejaria com outro.
#define DETERM_MS 1100
uint32_t determUntil = 0;
int16_t determDex = 0;
uint16_t determLv = 0;
bool determShiny = false;
int8_t determTrainer = -1;
// Escolha do inicial. NAO e mais so a primeira partida: chegar numa
// regiao nova abre a mesma tela, e os tres tem que ser os DELA ——
// era uma tabela fixa com os de Kanto, entao Johto ofereceria
// Bulbasaur. O trio de cada regiao vive em gyms.h.
static inline int16_t starterDex(uint8_t i) {
  return (int16_t)regionAt(pet.region).starters[i < 3 ? i : 0];
}
#define STARTER_DEX_COUNT 3
#define STARTER_ROW_Y 110
#define STARTER_ROW_H 70
#define STARTER_ROW_GAP 8
// boton-CTA de evolucion (centrado, mitad de pantalla)
// La chapa de "esto puede evolucionar". PEQUENA y A UN LADO a proposito:
// evolucionar es una opcion, no una orden. Ocupaba 256x64 en mitad de la
// pantalla, encima del bicho, y no dejaba ni mirarlo mientras decidias.
//
// A y=132 la cuerda del circulo da de x=15 a x=451, asi que a la izquierda
// cabe de sobra y no se come ni el titulo (y~55) ni al bicho (y>180).
//
// La usan los DOS avisos —— el del principal y el de un suplente ——, que
// nunca salen a la vez (ver la cadena de CTA en el dibujo de casa).
#define EVO_BTN_W 116
#define EVO_BTN_H 46
#define EVO_BTN_X 20
#define EVO_BTN_Y 132
// LA ENFERMERA, a la izquierda del bicho y debajo de la banda de la chapa
// de evolucion (y=132..178) para no pisarla. El bicho no llega: su sprite no
// pasa de ~80 px a cada lado del centro, o sea de x=153 a x=313.
//
// El mapa no tiene boton: se abre arrastrando a la izquierda (ver onSwipe).
// La pantalla de casa ya tiene cinco botones abajo y dos avisos flotando;
// el sexto habria sido ruido, y el gesto estaba medio libre.
// EL PROFESOR. Enfrente de la enfermera (mismo alto, lado opuesto) para
// que las dos visitas quepan a la vez sin taparse, y lejos del "!" de la
// hierba, que esta arriba a la derecha.
#define OAK_MARK_X 380
#define OAK_MARK_Y 250
#define OAK_MARK_R 32

#define PC_MARK_X 86
#define PC_MARK_Y 250
#define PC_MARK_R 32
// Y EL MAPA POR DENTRO. El poster no va centrado en el circulo —— debajo
// tienen que caber las flechas y el nombre ——, asi que sus esquinas de
// ARRIBA son las que mandan: con el centro en (233, 196) y 320x240 de
// lamina, la mas lejana cae a 224 del centro y el radio es 233. Entra con
// el marco. Estos numeros son los mismos que tools/pack_maps.py usa para
// escalar: si se cambian aqui hay que volver a empaquetar.
#define MAP_CX 233
#define MAP_CY 196
#define MAP_MAX_W 320
#define MAP_MAX_H 240
#define MAP_NAME_Y 336
#define MAP_ARROW_Y 352
#define MAP_ARROW_LX 72
#define MAP_ARROW_RX 394
#define MAP_ARROW_HIT 40
#define MAP_FOOT_Y 394
// El boton de REVANCHA, dentro del poster y abajo: fuera no cabe —— el
// nombre de la region ya ocupa la banda de 336 y las flechas sus lados.
#define MAP_RE_W 196
#define MAP_RE_H 40
#define MAP_RE_X (MAP_CX - MAP_RE_W / 2)
#define MAP_RE_Y 262
// GINASIO e MEDALHAS, no lugar do rodape "you are here" da regiao atual.
// Por dentro das zonas das flechas (x < 112 e x > 354) e do circulo.
// Leva 53: o dono "dificil de apertar e ficar voltando". A faixa das SETAS
// (MAP_ARROW_HIT = 40 em volta de y=352) descia ate 392 e pegava x<112 e
// x>354: errar o GINASIO pela esquerda trocava para a regiao anterior
// (onde os botoes nem aparecem). Agora os botoes vem ANTES das setas, com
// toque de x 96..233 e 233..370 (MAP_BT_TOQ_*), e as setas param acima.
#define MAP_BT_Y 368
#define MAP_BT_H 44
#define MAP_BT_W 112
#define MAP_BT_LX 116
#define MAP_BT_RX 238
#define MAP_BT_TOQ_X0 96
#define MAP_BT_TOQ_X1 370
#define MAP_BT_TOQ_Y0 (MAP_BT_Y - 6)
#define MAP_BT_TOQ_Y1 (MAP_BT_Y + MAP_BT_H + 12)
// el "!" del encuentro salvaje: a la derecha del bicho y despejado de los
// botones de abajo (se dibuja en drawWildMark, pero onTap esta mas arriba)
#define WILD_MARK_X 380
#define WILD_MARK_Y 168
// 30 y no 27: dentro va la hierba del Anil, que a su tamano mide 47
// de ancho y pedia un pelin mas de bola.
#define WILD_MARK_R 30
// boton-CTA de despedida (mas ancho: lleva el nombre + frase)
#define FAR_BTN_W 408
#define FAR_BTN_H 58
#define FAR_BTN_X (CX - FAR_BTN_W / 2)
#define FAR_BTN_Y 176
// el CST9217 avisa por el pin INT cuando hay datos tactiles; lo usamos para no
// leer el bus I2C mientras el chip esta dormido (esa lectura se colgaba ~1s)
volatile bool gTouchIrq = false;
void IRAM_ATTR touchIsr() { gTouchIrq = true; }
uint32_t lastRender = 0;
// proteccion del AMOLED: atenuado por inactividad
uint32_t lastInteract = 0;
uint8_t dimStage = 0;        // 0 despierto, 1 atenuado (90s), 2 casi apagado (5min)
bool swallowGesture = false; // el toque que despierta no acciona nada
uint32_t holdStart = 0;     // pulsacion larga sobre el bicho
uint32_t confirmUntil = 0;  // dialogo "soltar?" activo hasta este millis

// ---- la pregunta de apagar (aguantar el boton PWR) ----
// Apagar NO pausa: el RTC sigue contando con la placa muerta y al volver
// se aplica todo de golpe. Eso es a proposito — es un tamagotchi —, pero
// hay que poder guardarlo en un cajon sin que vuelva hecho un cristo, y
// hasta ahora no habia manera de pedirlo.
uint32_t apagarUntil = 0;   // la pregunta esta en pantalla hasta este millis
#define APA_BX 110          // los dos botones, mismo ancho
#define APA_BW 246
#define APA_BH 56
#define APA_B1Y 206         // GUARDAR
#define APA_B2Y 270         // APAGAR normal
uint8_t choiceKind = 0;     // dialogo de decision: 0 ninguno, 1 evolucion, 2 despedida, 3 escapada
// Todos se abren con 12 s. Los primeros 600 ms no aceptan toques: el
// boton que lo abre (y=176..234) pisa el primero del dialogo (206..258),
// y un toque doble sin querer —— en el bolsillo —— confirmaba solo.
#define CHOICE_MS 12000
#define CHOICE_GRACA_MS 600
uint32_t choiceUntil = 0;   // se cierra solo a este millis
int16_t tX0, tY0, tXl, tYl; // gesto en curso (inicio y ultima posicion)
uint32_t tStart = 0;
bool holdFired = false;

// Por que arranco. Un bucle de reinicios se diagnostica solo con esto:
// BROWNOUT dice "falta corriente", PANIC dice "fallo el codigo", TASK_WDT
// dice "algo se quedo colgado". Sin saberlo se arregla a ciegas.
static const char *porQueArranco() {
  switch (esp_reset_reason()) {
    case ESP_RST_POWERON:  return "encendido";
    case ESP_RST_SW:       return "ESP.restart()";
    case ESP_RST_PANIC:    return "PANICO (fallo del codigo)";
    case ESP_RST_INT_WDT:  return "WATCHDOG de interrupciones";
    case ESP_RST_TASK_WDT: return "WATCHDOG de tarea";
    case ESP_RST_WDT:      return "WATCHDOG";
    case ESP_RST_BROWNOUT: return "BROWNOUT (se hundio la tension)";
    case ESP_RST_DEEPSLEEP: return "sueno profundo";
    case ESP_RST_EXT:      return "reset externo";
    default:               return "desconocido";
  }
}

void setup() {
  Serial.setRxBufferSize(8192);  // la transferencia a SD llega en bloques de 2 KB
  Serial.begin(115200);
  // CRITICO: sin esto, Serial.print BLOQUEA el juego cuando no hay un
  // monitor serie abierto en el host (el bufer TX del USB CDC se llena
  // y nadie lo vacia) -> con timeout 0 los mensajes se descartan
  Serial.setTxTimeoutMs(0);
  Serial.printf("TamaPoke fw v%s\n", FW_VERSION);
  loadLang();  // idioma guardado (ES por defecto)
  Wire.begin(IIC_SDA, IIC_SCL);
  // CST9217 (tactil), AXP2101 (PMU) y PCF85063 (RTC) comparten este bus I2C.
  // Red de seguridad para PMU/RTC (SensorLib NO respeta este timeout en el
  // tactil; el cuelgue del tactil dormido se resuelve gateando por INT, ver
  // handleTouch).
  Wire.setTimeOut(50);

  // CRITICO: encender la alimentacion del panel (BLDO1=OLED VDD 3.3V) ANTES de
  // inicializar el display. Si el PMU se reseteo (drenaje total), este rail
  // queda OFF y la pantalla se ve negra aunque el resto de la placa funcione.
  pmuEnablePanel();

  // QSPI a 80MHz (por defecto 40): el flush del framebuffer es el cuello de
  // botella del fps (~56ms a 40MHz). Si el panel mostrara basura, bajar a 40M.
  if (!gfx->begin(80000000)) Serial.println("gfx->begin() fallo");
  panel->setBrightness(180);
  applyLangFont();  // fuente del idioma guardado (clasica salvo CJK)

  touch.setPins(TP_RESET, TP_INT);
  bool touchOk = false;
  for (int i = 0; i < 3 && !touchOk; i++) {  // a veces falla al primer intento
    touchOk = touch.begin(Wire, 0x5A, IIC_SDA, IIC_SCL);
    if (!touchOk) delay(150);
  }
  if (!touchOk) Serial.println("CST9217 no detectado");
  // begin() deja el chip en modo comando (lee la identidad y no sale);
  // hace falta un reset por hardware para que vuelva a reportar toques
  touch.reset();
  touch.setMaxCoordinates(LCD_WIDTH, LCD_HEIGHT);
  touch.setMirrorXY(true, true);  // el panel esta montado girado 180 grados
  // INT activo-bajo: salta cuando hay datos. Gatea las lecturas I2C (ver loop)
  pinMode(TP_INT, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(TP_INT), touchIsr, FALLING);

  // LA TARJETA VA ANTES QUE EL PET, y no es un detalle de orden: el
  // load() del Pet migra el banco viejo (60 huecos en la NVS) a las
  // cajas de la SD, y si la tarjeta no esta montada esa escritura falla
  // callada. El save siguiente deja "bank" en 30 registros y lo que
  // estaba en las casas 30..59 se pierde. Paso de verdad.
  fxGolpesEsperados = MOVE_COUNT;
  // A mesma firma pros retratos: /por/por.bin tambem vai indexado
  // por numero, e um cartao velho mostraria a cara do vizinho sem
  // que nada dissesse.
  porEspeciesEsperadas = POR_ESPECIES;
  porRanurasEsperadas = POR_RANURAS;
  porVersionEsperada = POR_VERSION;
  sdBegin();

  pet.begin();
  // La intro va ANTES de elegir inicial, y solo la primera vez.
  if (!pet.introDone) startIntro();
  // Antes de leer nada de la tarjeta: SdFx lo necesita para saber si
  // el /fx/fx.bin que hay es de esta version de la tabla de golpes.
  {
    Preferences g;
    g.begin("tamapoke", true);
    gestosOn = g.getBool("gest", false);
    g.end();
    Serial.printf("gestos del acelerometro: %s\n", gestosOn ? "ON" : "OFF");
  }
  // (sdBegin ya corrio, arriba: tiene que ir antes que pet.begin)
  // El lector USB se prepara aqui, con la tarjeta recien montada y
  // antes de que nadie lea de ella. Nace VACIO (el PC ve un lector sin
  // tarjeta dentro): hasta que no llegue "USB ON" no cambia nada.
  usbMscBegin();
  Serial.printf("[arranque] motivo: %s\n", porQueArranco());
  Serial.flush();
  Serial.printf("[arranque] %s\n", "thumbs"); Serial.flush();
  thumbs.load();

  // reloj real: aplica el tiempo que estuvo apagado
  Serial.printf("[arranque] %s\n", "rtc"); Serial.flush();
  rtcBegin();
  // El acelerometro va DESPUES del RTC: comparten el bus I2C y el
  // primero que llega es quien hace Wire.begin(). Si no responde,
  // el juego sigue sin gestos y lo dice por el puerto serie.
  Serial.printf("[arranque] %s\n", "imu"); Serial.flush();
  imuBegin();
  Serial.printf("[arranque] %s\n", "bat"); Serial.flush();
  batBegin();
  Serial.printf("[arranque] %s\n", "pmu"); Serial.flush();
  pwrSetup();
  uint32_t e = rtcEpoch();
  if (e == 0) {
    // Sin pila de respaldo, el PCF85063 pierde la hora al cortar la
    // alimentacion. Sembrar siempre la fecha fija haria RETROCEDER el tiempo de
    // juego respecto a lo ya guardado, y eso rompe la racha, la edad y la hora
    // de la escena. Se siembra con la ultima hora vista, si es posterior, para
    // que el tiempo no vaya nunca hacia atras. Los minutos apagado se pierden
    // igual (sin RTC no hay forma de saberlos), pero nada se descuadra.
    uint32_t seed = 1767225600UL;             // RTC virgen de verdad
    uint32_t seen = pet.savedEpoch();         // usa la NVS que pet ya tiene abierta
    if (seen > seed) seed = seen;
    rtcSetEpoch(seed);
    e = rtcEpoch();
    Serial.printf("RTC sin hora: sembrado en %u%s\n", seed,
                  seen > 1767225600UL ? " (desde la ultima hora guardada)" : "");
  }
  pet.syncClock(e);

  audioBegin();  // ES8311 + I2S + amplificador (suena un jingle de arranque)
  // Contra que tabela conferir o pacote de som (/snd/golpes.txt).
  audioMoveTable(MOVE_COUNT, [](uint16_t i) { return MOVE_TBL[i].name; });

  lastInteract = millis();
}

// carga/descarga el sprite de SD cuando cambia la especie
void ensureMon() {
  if (pet.speciesId == monFor && monShinyFor == pet.shiny && !sdDirty) return;
  // los ficheros recien recibidos pueden incluir un thumbs.bin nuevo, que se
  // cargaba solo en setup(): sin esto no se veia en la galeria hasta reiniciar
  if (sdDirty) { thumbs.unload(); thumbs.load(); }
  sdDirty = false;
  monFor = pet.speciesId;
  monShinyFor = pet.shiny;
  mon.unload();
  pmd.unload();
  beh.x = beh.targetX = 233;
  beh.mode = 0;
  beh.until = 0;
  if (pet.speciesId >= 1 && pet.speciesId <= DEX_COUNT) {
    // `pet.female` ya existia para los nombres y las evoluciones; su
    // comentario decia desde el principio "y para elegir el sprite, que
    // 98 especies tienen uno propio de hembra". Esta es esa parte.
    pmd.load(pet.speciesId, pet.shiny, pet.female);   // principal: PMD
    if (!pmd.loaded) mon.load(pet.speciesId, pet.shiny);  // respaldo: B/N
  }
}

void loop() {
  imuTick();     // el acelerometro, antes de que nadie pregunte
  uint32_t now = millis();
  pet.update(now);
  // Abandonado: el bicho llama con su grito (cada 15 min, ver pet.h).
  if (pet.tomaAvisoAbandono() && !bt.open) cryPlay((uint16_t)pet.speciesId);
  // La enfermera: decide si toca visita. Va aqui y no dentro de update()
  // porque update() cuenta MINUTOS del bicho (sobrevive a apagar la placa)
  // y la visita cuenta millis de ESTA sesion.
  pet.pcTick(millis());
  pet.quizTick(millis());
  battleTick();  // avanza las fases del combate, si hay uno abierto
  trocaLoop();   // a troca entre placas (so faz algo com a tela aberta)

  // O retrato do bicho que esta na tela, trazido AQUI e nao no
  // desenho: load() le do cartao, e uma leitura de SD dentro do
  // quadro trava a animacao. Sai sozinho se ja for esse bicho.
  // Durante o combate nao: a tela de vitoria pode estar mostrando a
  // cara de um suplente, e o tick ficaria trocando uma pela outra a
  // cada quadro —— uma leitura de SD por quadro.
  if (!bt.open && !pet.isEgg() && pet.speciesId > 0)
    porMon.load((uint16_t)pet.speciesId, pet.shiny);

  // O matinho: acabou o balao de decidido, entra o combate.
  // Com a tela de troca aberta o matinho espera: um combate por cima
  // esconderia a troca no meio, com o radio ligado.
  if (determUntil && !trocaOpen && (int32_t)(millis() - determUntil) >= 0) {
    determUntil = 0;
    if (determDex > 0) {
      int16_t d = determDex;
      determDex = 0;
      battleStart(d, determLv, -1, determShiny, determTrainer);
    }
  }

  // el amplificador sigue al estado de sueno (la llamada sale sola si no cambia).
  // Aqui cubre todas las vias: el boton de luz, el sueno nocturno y el que llega
  // aplicado desde la progresion offline.
  audioSetSleeping(pet.sleeping);

  // ---- LOS GESTOS DEL ACELEROMETRO ------------------------------------
  // Todos entran por acciones que YA existian (caress, el peso, el
  // forcejeo de la bola) en vez de traer sistemas nuevos. Asi una
  // caricia a tapas cuenta para el vinculo con su tope diario, saca el
  // mismo corazon y registra el cuidado igual que tocar la pantalla:
  // una accion, dos maneras de pedirla.
  if (gestosOn) {
    Gestos g = imuGestos();

    // CARICIAS a tapas. pet.caress() ya hacia todo: alegria, vinculo con
    // tope, el corazon en pantalla y el registro de cuidado.
    if (g.caricia) {
      pet.caress();
      // Su PROPIO grito, no un efecto de menu. Es lo que convierte el
      // mimo en algo del bicho y no de la interfaz, y cambia con la
      // especie, asi que acariciar a uno no suena como acariciar a otro.
      if (!pet.sleeping) cryPlay((uint16_t)pet.speciesId);
    }

    // DOS TAPAS: arriba. Solo despierta si fuiste TU quien lo acosto
    // (ver Pet::wakeUp): desmayado, dormir es como se cura.
    if (g.dosTapas && pet.wakeUp()) sfxPlay(SFX_TAP);

    // BOCA ABAJO un rato: a la cama. Esto no existia — hasta ahora solo
    // se dormia al desmayarse.
    if (g.bocabajo && pet.restDown()) sfxPlay(SFX_TAP);

    // ZARANDEO brusco: al bicho no le gusta. Es el precio de menear.
    if (g.susto) {
      pet.scare();
      sfxPlay(SFX_DENY);
    }

    // PASOS: andar con la consola encima quema peso y anima, sin gastar
    // fuelle. El que anda eres tu.
    if (g.pasos) pet.walk(g.pasos);

    // MENEAR durante el forcejeo de la bola: el gesto de verdad de una
    // Pokebola. Entra en la misma cuenta que los toques (capture.ino).
    if (g.meneo) btCaptureMeneo(g.fuerza);
  }

  // avisa con un sonido cuando el bicho pasa a estar listo para evolucionar
  // (incluye el caso de cumplir al despertar). canEvolveNow es false durmiendo.
  static bool wasEvoReady = false;
  bool evoReady = pet.wantEvolveButton();
  if (evoReady && !wasEvoReady) sfxPlay(SFX_MEDAL);
  wasEvoReady = evoReady;
  // aviso sombrio cuando el bicho esta a punto de escaparse por abandono
  static bool wasRunReady = false;
  bool runReady = pet.canRunawayNow();
  if (runReady && !wasRunReady) sfxPlay(SFX_DENY);
  wasRunReady = runReady;

  // El aviso de la hierba, de OIDO. El "!" aparece y se va solo a los
  // ocho minutos: si no estas mirando la pantalla en ese momento, te lo
  // pierdes entero. Suena una vez, al aparecer, y distinto segun sea
  // bicho o entrenador.
  //
  // Se mira el FLANCO aqui y no se avisa desde pet.cpp a proposito: ahi
  // no hay audio (lo compilan la bateria de tests y el simulador), y el
  // patron de "esto acaba de cambiar" ya es el de arriba.
  static bool habiaAviso = false;
  bool hayAviso = pet.wildWaiting();
  if (hayAviso && !habiaAviso)
    sfxPlay(pet.wildTrainer >= 0 ? SFX_TRAINER : SFX_WILD);
  habiaAviso = hayAviso;

  handleTouch();
  handleSerial();
  // Expulsar la unidad en el PC es la senal de "ya termine", y la unica
  // que garantiza que vacio su cache de escritura. Se recupera la
  // tarjeta sola. Si el PC escribio algo, se reinicia: los indices que
  // el firmware tiene en memoria (thumbs.bin, el sprite cargado) hablan
  // de una tarjeta que ya no existe, y seguir con ellos es ensenar
  // sprites de otro bicho.
  if (usbMscPrestada() && usbMscExpulsada()) {
    uint32_t esc = usbMscEscritos();
    usbMscPrestar(false);
    Serial.printf("tarjeta devuelta (%u sectores escritos)\n", (unsigned)esc);
    if (esc) {
      pet.saveNow();
      Serial.println("reiniciando para releer la tarjeta...");
      Serial.flush();
      delay(200);
      ESP.restart();
    }
  }
  ensureMon();

  // El boton PWR. Corta: pantalla on/off. AGUANTADO: la pregunta de apagar.
  //
  // 120 ms y no 250: el aguante se mide aqui, y con un cuarto de segundo de
  // paso la pregunta salia tarde y a destiempo. Son dos lecturas I2C de un
  // registro por vuelta, que al lado de repintar 466x466 no es nada.
  static uint32_t lastPwr = 0;
  static uint32_t teclaDesde = 0;
  if (now - lastPwr > 120) {
    lastPwr = now;
    // Con la pregunta puesta el toque corto NO apaga la pantalla: dejaria
    // el dialogo a oscuras esperando una respuesta que no se ve.
    if (pwrShortPressed() && !apagarUntil) {
      screenOff = !screenOff;
      if (!screenOff) lastInteract = now;
    }
    if (pwrKeyHeld()) {
      if (!teclaDesde) teclaDesde = now;
      // 700 ms: lo bastante para que un toque corto no la dispare, y muy
      // por debajo de los 6 s a los que el PMU corta por su cuenta.
      if (!apagarUntil && now - teclaDesde > 700) {
        apagarUntil = now + 15000;
        screenOff = false;
        lastInteract = now;
        sfxPlay(SFX_TAP);
      }
    } else {
      teclaDesde = 0;
    }
  }

  updateBrightness(now);

  // vuelca el autoguardado periodico SOLO con la pantalla atenuada/apagada o
  // durmiendo: la escritura a NVS congela ~1s ambos cores (cachÃ© de flash off),
  // y aqui no hay animacion que se corte ni dedo esperando respuesta. Con 90s
  // de inactividad la pantalla ya atenua, asi que se vuelca enseguida; el uso
  // activo persiste igual por los guardados de cada accion (comer/jugar/...).
  if (pet.savePending() && (screenOff || dimStage >= 1 || pet.sleeping)) {
    pet.flushSave();
  }
  // SAVE QUE FALHOU: tenta de novo a cada minuto ate passar. O aviso na tela
  // (avisoDeSave) fica enquanto nao passar: jogar sem gravar, sem saber, foi
  // o que levou Hoenn.
  // O minuto conta a partir de quando a falha APARECEU: medido desde o boot,
  // a primeira tentativa vinha na hora e o aviso nem chegava a ser visto.
  {
    static uint32_t reintento = 0;
    static bool falhaVista = false;
    if (!pet.guardadoFallo) {
      falhaVista = false;
    } else if (!falhaVista) {
      falhaVista = true;
      reintento = now;
    } else if (now - reintento > 60000) {
      reintento = now;
      pet.saveNow();
    }
  }

  // Anota la hora real cada 30 s (se persiste en cada save del juego).
  //
  // Y la VIGILA. Estando encendidos sabemos cuanto tiempo ha pasado de
  // verdad: lo dice millis(). Si el PCF85063 contesta con bastante mas que
  // eso, es una lectura mala — rtcEpoch() solo valida el AÑO, asi que un
  // dia mal leido pasa el filtro tan campante — y aceptarla envenena
  // `seen`. En el arranque siguiente syncClock se cree que han pasado 24 h
  // y devuelve al bicho rojo, hambriento y lleno de caca sin que nadie
  // haya hecho nada. Se encontro con el reloj de la placa un DIA entero
  // por delante del real, con la hora, el minuto y el segundo correctos.
  //
  // Al detectarlo no se descarta y ya: se CORRIGE el chip a lo que dice
  // millis(). Descartando a secas, un reloj que se quedo adelantado se
  // rechazaria para siempre y `seen` no volveria a avanzar nunca.
  static uint32_t lastClock = 0;
  if (now - lastClock > 30000) {
    uint32_t paso = (now - lastClock) / 1000;   // lo que marco millis()
    lastClock = now;
    uint32_t e = rtcEpoch();
    if (e) {
      uint32_t prev = pet.lastSeenEpoch;
      // 120 s de margen: cubre el jitter del bucle y los redondeos
      if (prev && e > prev + paso + 120) {
        uint32_t bueno = prev + paso;
        Serial.printf("RTC adelantado %lu s estando encendidos (millis dice "
                      "%lu): corregido a %lu\n",
                      (unsigned long)(e - prev), (unsigned long)paso,
                      (unsigned long)bueno);
        rtcSetEpoch(bueno);
        pet.lastSeenEpoch = bueno;
      } else {
        pet.lastSeenEpoch = e;
      }
    }
  }

  // latido de salud cada 5 min (para el soak test; se descarta si no hay monitor)
  static uint32_t lastHealth = 0;
  if (now - lastHealth > 300000) {
    lastHealth = now;
    Serial.printf("HEALTH up=%lus heap=%u min=%u bat=%d%% mv=%d chg=%d usb=%d dim=%u off=%d\n",
                  (unsigned long)(now / 1000), ESP.getFreeHeap(), ESP.getMinFreeHeap(),
                  batPercent(), batMillivolts(), batCharging() ? 1 : 0,
                  usbPresent() ? 1 : 0, dimStage, screenOff ? 1 : 0);
    Serial.printf("  dibujo: peor %lu ms -> cadencia %lu ms\n",
                  (unsigned long)renderPeor,
                  (unsigned long)(renderPeor + 40 < 100 ? 100 : renderPeor + 40));
  }

  // 85 ms en juego/saco: margen seguro para que el redibujado no pise el envio
  // DMA del frame anterior (a 40-65 ms solapaba y causaba flashes negros; con
  // sprites grandes el dibujo tarda mas, asi que se deja colchon)
  // Con la pantalla apagada no se dibuja: a brillo 0 no se ve nada, pero el
  // redibujado y el flush DMA del framebuffer de 466x466 seguian corriendo a
  // 10 fps, y medido son 70 ms de trabajo por cada 100 (46 de ellos solo el
  // volcado). Es la mayor carga evitable de la placa. Cada render repinta la
  // escena entera, asi que no queda nada a medias; y al quedarse lastRender
  // congelado, el primer frame tras despertar sale en el acto.
  // La cadencia la manda lo que TARDA el dibujo, no un numero fijo.
  //
  // El numero fijo ya fallo una vez (a 40-65 ms) y volvio a fallar
  // ahora: al agrandar el bicho del jugador de 0,74 a 1,05 de su altura
  // -- para que se notara la perspectiva -- el dibujo paso a tardar mas
  // y los 100 ms dejaron de dar margen. Reaparecieron los parpadeos
  // negros, que es el redibujado pisando el volcado DMA del frame
  // anterior. Un numero fijo no puede saber cuanto ocupa un sprite.
  //
  // Asi que se mide: el peor tiempo reciente mas 25 ms de colchon. Sube
  // de golpe (un frame lento manda ya) y baja despacio, para no entrar y
  // salir del limite en cada fotograma. Suelo el de siempre y techo en
  // MEDIDO en la placa: un repintado de la pantalla de casa tarda 75
  // ms (lo dice "dib" en HEALTH). Con 25 de colchon la cadencia daba
  // justo los 100 de antes, o sea que no arreglaba nada donde mas se
  // nota; con 40 quedan 115 y el volcado DMA tiene sitio. Techo en
  // 200 ms, que son 5 por segundo: por debajo de eso se notaria a tiron
  // y seria mejor saberlo que esconderlo.
  uint32_t suelo = (uint32_t)(gymOpen ? 85 : 100);
  uint32_t cadencia = renderPeor + 40;
  if (cadencia < suelo) cadencia = suelo;
  if (cadencia > 200) cadencia = 200;
  if (avOpen && dimStage >= 2) cadencia = 1000;   // aventura no escuro: 1 por segundo
  // A AVENTURA E PARA DEIXAR PARADO: com a tela escurecida ou apagada, a
  // musica cala (tocava em laco a noite toda). Num toque a tela acende e
  // homeMusic() a traz de volta.
  if (avOpen && (screenOff || dimStage >= 1)) musicStop();
  if (!screenOff && now - lastRender >= cadencia) {
    lastRender = now;
    uint32_t t0 = millis();
    render();
    uint32_t tardo = millis() - t0;
    if (tardo > renderPeor) renderPeor = tardo;
    else renderPeor = (renderPeor * 15 + tardo) / 16;
  }
}

// brillo segun sueno + inactividad (proteccion del AMOLED)
void updateBrightness(uint32_t now) {
  // los eventos visibles despiertan la pantalla solos
  // Na aventura o bicho de casa nem aparece: um coracao dele pendente nao
  // pode segurar a tela acesa (nem a musica, que cala no escuro).
  if (!avOpen && (pet.evolving() || pet.ceremony || pet.eating() || pet.showHeart())) {
    lastInteract = now;
  }
  // OJO con esta resta. `now` lo toma loop() en su primera linea;
  // handleTouch() corre MAS ABAJO en la misma vuelta y marca lastInteract
  // con millis(), que para entonces ya ha avanzado. O sea que lastInteract
  // puede ser MAYOR que now, y sin signo la resta se daba la vuelta a
  // ~4.29e9: dimStage saltaba a 2, el brillo caia a 8 de 255 (negro en un
  // AMOLED) y a la vuelta siguiente volvia a 145.
  //
  // ESO era el "parpadeo negro al tocar la pantalla". No tenia nada que
  // ver con el volcado a la pantalla (el bus QSPI escribe por sondeo, no
  // hay DMA solapado que capturar a medias), por eso tocar la cadencia de
  // repintado no lo arreglo. Y explica que fuera "a veces": solo pasa si
  // el cuerpo del bucle cruza un milisegundo entre tomar `now` y leer el
  // dedo, que con sprites grandes es casi siempre.
  //
  // De paso arregla los toques que se comian: dimStage quedaba en 2, y
  // handleTouch mira dimStage ANTES de que updateBrightness lo corrija,
  // asi que ponia swallowGesture y se tragaba el toque siguiente. Eso es
  // lo que hacia que la barra de la pokebola no subiera dando toques
  // rapidos.
  uint32_t idle = (now > lastInteract) ? (now - lastInteract) : 0;
  dimStage = (idle > 300000) ? 2 : (idle > 90000) ? 1 : 0;
  uint8_t target = pet.sleeping ? 25 : (usbPresent() ? 180 : 145);
  if (dimStage == 1) target = pet.sleeping ? 10 : 60;
  else if (dimStage == 2) target = 8;
  if (screenOff) target = 0;
  static uint8_t current = 255;
  if (target != current) {
    current = target;
    panel->setBrightness(target);
  }
}

// ---------- consola serie (provision de SD + depuracion) ----------

void handleSerial() {
  if (!Serial.available()) return;
  String line = Serial.readStringUntil('\n');
  line.trim();
  if (line.length() == 0) return;
  if (sdSerialCommand(line)) return;
  if (usbMscSerialCommand(line)) return;

  if (line.startsWith("GESTOS")) {
    String q = line.substring(6);
    q.trim();
    if (q == "ON" || q == "OFF") {
      gestosOn = (q == "ON");
      Preferences g;
      g.begin("tamapoke", false);
      g.putBool("gest", gestosOn);
      g.end();
    }
    Serial.printf("gestos: %s\n", gestosOn ? "ON" : "OFF");
    Serial.println("DONE");
  } else if (line == "HATCH") {
    pet.eggTap(); pet.eggTap(); pet.eggTap();
    Serial.println("DONE");
  } else if (line.startsWith("SPEC ")) {
    // CAMBIA LA ESPECIE DEL BICHO VIVO, y nada mas: ni los golpes, ni la
    // experiencia, ni el vinculo. Es justo lo que se queria para mirar un
    // sprite... y es destructivo sobre una partida de verdad.
    //
    // Paso: se uso en la placa del usuario para ver el sprite de un
    // Pidgeotto y su Dratini se quedo convertido en "un pidgeotto com
    // golpes de dragao", con todo lo demas del Dratini. Un comando de
    // depuracion que pisa la partida y no se puede deshacer no deberia
    // existir asi, y menos usarse sobre el cacharro de alguien.
    //
    // Ahora guarda lo que habia y `SPEC UNDO` lo devuelve. Guardar cuesta
    // dos bytes; no tenerlo costo un bicho.
    static int16_t specAntes = -1;
    String arg = line.substring(5);
    arg.trim();
    if (arg == "UNDO") {
      if (specAntes >= 1 && specAntes <= DEX_COUNT) {
        pet.speciesId = specAntes;
        pet.hpSync();
        Serial.printf("devuelto a #%d %s\n", specAntes, DEX_TBL[specAntes].name);
        specAntes = -1;
      } else {
        Serial.println("SPEC UNDO: no hay nada que deshacer");
      }
      Serial.println("DONE");
      return;
    }
    int n = arg.toInt();
    if (n >= 1 && n <= DEX_COUNT) {
      if (pet.speciesId != n) specAntes = pet.speciesId;
      Serial.printf("OJO: esto PISA la especie del bicho vivo (era #%d). "
                    "Para deshacer: SPEC UNDO\n", pet.speciesId);
      pet.prevSpeciesId = pet.speciesId;
      pet.speciesId = n;
      // Los PS, detras. Cambiar de especie cambia el maximo, y sin
      // esto el bicho se quedaba con MAS vida que su tope: la barra se
      // salia de su caja. Es un comando de depuracion, pero uno que
      // deja el estado imposible miente sobre lo que estas probando.
      pet.hpSync();
      Serial.printf("especie #%d %s\n", n, DEX_TBL[n].name);
    }
    Serial.println("DONE");
  } else if (line.startsWith("FIGHT ")) {
    // FIGHT <dex> [nivel]  - lanza un combate contra ese salvaje. Mientras no
    // haya encuentros de verdad, esta es la unica puerta de entrada, y sirve
    // igual para probar en el simulador que con la placa enchufada.
    String resto = line.substring(6);
    resto.trim();
    int esp = 0, niv = 0;
    int sp = resto.indexOf(' ');
    if (sp > 0) {
      esp = resto.substring(0, sp).toInt();
      niv = resto.substring(sp + 1).toInt();
    } else {
      esp = resto.toInt();
    }
    if (niv <= 0) niv = (int)pet.level();
    // una "S" al final fuerza que el rival sea shiny, como en WILD: 1 de cada
    // 128 no hay forma de verlo a proposito
    bool bri = resto.indexOf('S') >= 0;
    if (esp >= 1 && esp <= DEX_COUNT && !pet.isEgg() && pet.canBattle()) {
      battleStart((int16_t)esp, (uint16_t)niv, -1, bri);
      Serial.printf("combate contra #%d %s nv%d (PS %u/%u)\n", esp,
                    DEX_TBL[esp].name, niv, pet.hpLeft, pet.maxHp());
      Serial.printf("  habilidades: %s vs %s\n",
                    abilAt(bt.b.side[BT_PLAYER].ability).name,
                    abilAt(bt.b.side[BT_FOE].ability).name);
    } else if (!pet.isEgg() && !pet.canBattle()) {
      Serial.printf("ERR: %s (PS %u/%u, energia %u, hace falta %u)\n",
                    pet.whyNotBattle(), pet.hpLeft, pet.maxHp(),
                    pet.energy, BATTLE_ENERGY_MIN);
    } else {
      Serial.println("ERR: FIGHT <1..DEX_COUNT> [nivel], y no siendo huevo");
    }
    Serial.println("DONE");
  } else if (line.startsWith("CLIMA")) {
    // CLIMA [0..4]  - poe esse clima no combate aberto (1 sol, 2 chuva,
    // 3 areia, 4 granizo) e diz quanto custou o ultimo quadro. Nao grava
    // nada: e so para medir na placa o que o simulador nao mede.
    extern uint32_t gQuadroMs, gClimaMs;
    String resto = line.substring(5);
    resto.trim();
    if (resto.length()) {
      int w = resto.toInt();
      if (bt.open && w >= 0 && w <= WE_HAIL) {
        bt.b.weather = (uint8_t)w;
        bt.b.weatherTurns = w ? 8 : 0;
      } else {
        Serial.println("ERR: CLIMA 0..4, com um combate aberto");
      }
    }
    Serial.printf("clima=%u quadro=%u ms (clima %u ms)\n", (unsigned)bt.b.weather,
                  (unsigned)gQuadroMs, (unsigned)gClimaMs);
    Serial.println("DONE");
  } else if (line.startsWith("STATUS ")) {
    // STATUS <0=yo|1=rival> <1..6>  - fuerza un estado en pleno combate.
    // Los estados entran por tirada de probabilidad, asi que sin esto probar
    // el chip o el desgaste es cuestion de suerte.
    String resto = line.substring(7);
    resto.trim();
    int sp = resto.indexOf(' ');
    int lado = resto.substring(0, sp > 0 ? sp : resto.length()).toInt();
    int st = sp > 0 ? resto.substring(sp + 1).toInt() : 0;
    if (bt.open && lado >= 0 && lado <= 1 && st >= 1 && st <= ST_FREEZE) {
      Combatant &c = bt.b.side[lado];
      c.status = ST_NONE;                  // se pisa el que hubiera
      c.applyStatus((uint8_t)st);
      Serial.printf("estado %d en %s -> %d\n", st, dexName(c.asDex), c.status);
    } else {
      Serial.println("ERR: STATUS <0|1> <1..6>, y con un combate abierto");
    }
    Serial.println("DONE");
  } else if (line.startsWith("STAGE ")) {
    // STAGE <0|1> <0..5> <delta>  - mueve un stage, para ver las flechas
    String resto = line.substring(6);
    resto.trim();
    int v[3] = { 0, 0, 0 }, n = 0;
    while (n < 3 && resto.length()) {
      int sp2 = resto.indexOf(' ');
      v[n++] = (sp2 > 0 ? resto.substring(0, sp2) : resto).toInt();
      resto = sp2 > 0 ? resto.substring(sp2 + 1) : String();
      resto.trim();
    }
    if (bt.open && v[0] >= 0 && v[0] <= 1 && v[1] >= 0 && v[1] < SG_COUNT) {
      bt.b.side[v[0]].applyStage((uint8_t)v[1], (int8_t)v[2]);
      Serial.printf("stage %d = %d\n", v[1], bt.b.side[v[0]].stage[v[1]]);
    } else {
      Serial.println("ERR: STAGE <0|1> <0..5> <delta>, con combate abierto");
    }
    Serial.println("DONE");
  } else if (line.startsWith("PARTY ")) {
    // PARTY <dex> <nivel>  - mete uno en el equipo (o en el banco si esta
    // lleno). Sin esto no hay forma de ver la pantalla de equipo con algo
    // dentro: los bichos solo entran capturando, y la captura no existe aun.
    String resto = line.substring(6);
    resto.trim();
    int sp = resto.indexOf(' ');
    int d = sp > 0 ? resto.substring(0, sp).toInt() : resto.toInt();
    int nv = sp > 0 ? resto.substring(sp + 1).toInt() : 5;
    if (d >= 1 && d <= DEX_COUNT) {
      StoredMon m;
      m.species = (int16_t)d;
      m.exp = Pet::expForLevel((uint16_t)(nv < 1 ? 1 : nv));
      m.geneAtk = m.geneDef = m.geneSpe = (uint8_t)(90 + random(21));
      uint16_t mv[4];
      uint8_t n = movesAtLevel((int16_t)d, Pet::levelOf(m), mv);
      for (uint8_t i = 0; i < n; i++) { m.moves[i] = mv[i]; m.ppLeft[i] = movePP(mv[i]); }
      m.hpLeft = hpAtLevel((int16_t)d, Pet::levelOf(m), m.trHp);
      const uint16_t *a = ABIL_OF[d];
      m.ability = a[0] ? a[0] : a[1];
      m.bond = (uint8_t)random(40);
      bool ok = pet.addToTeam(m);
      // Tem o bicho = tem a ficha. Sem isto o Hoenn refeito a mao ficou
      // com seis especies sem entrada na Pokedex (2026-11-09).
      pet.registerSpecies(m.species, false, m.form);
      galleryDirty = true;
      if (!ok) {
        // equipo lleno: al banco, que para eso esta
        for (uint8_t i = 0; i < BANK_SLOTS && !ok; i++)
          if (!pet.bank[i].used()) { pet.bank[i] = m; ok = true; }
      }
      Serial.printf("%s nv%u -> %s (equipo %u, banco %u)\n", DEX_TBL[d].name,
                    Pet::levelOf(m), ok ? "dentro" : "NO CABE",
                    pet.teamCount(), pet.bankCount());
    } else {
      Serial.println("ERR: PARTY <1..DEX_COUNT> <nivel>");
    }
    Serial.println("DONE");
  } else if (line.startsWith("BANCO ")) {
    // BANCO <dex> <nivel> [objeto]  - ESCREVE no save: poe um na caixa
    // aberta segurando o objeto (id do ITEM_TBL), e grava. Existe para
    // testar as trocas (Scyther + Metal Coat): segurar so se poe no
    // titular, e o titular nao sai. Ler: CAJA <n> ("lleva algo").
    // Desfazer: o X do banco solta e devolve o objeto a bolsa.
    int v[3] = { 0, 5, 0 };
    String resto = line.substring(6);
    resto.trim();
    for (int k = 0; k < 3 && resto.length(); k++) {
      int sp = resto.indexOf(' ');
      v[k] = (sp > 0 ? resto.substring(0, sp) : resto).toInt();
      resto = sp > 0 ? resto.substring(sp + 1) : String();
      resto.trim();
    }
    int d = v[0], nv = v[1] < 1 ? 1 : v[1], it = v[2];
    if (d >= 1 && d <= DEX_COUNT && it >= 0 && it < ITEM_COUNT) {
      StoredMon m;
      m.species = (int16_t)d;
      m.exp = Pet::expForLevel((uint16_t)nv);
      m.geneAtk = m.geneDef = m.geneSpe = (uint8_t)(90 + random(21));
      uint16_t mv[4];
      uint8_t n = movesAtLevel((int16_t)d, Pet::levelOf(m), mv);
      for (uint8_t i = 0; i < n; i++) { m.moves[i] = mv[i]; m.ppLeft[i] = movePP(mv[i]); }
      m.hpLeft = hpAtLevel((int16_t)d, Pet::levelOf(m), m.trHp);
      const uint16_t *a = ABIL_OF[d];
      m.ability = a[0] ? a[0] : a[1];
      m.bond = (uint8_t)random(40);
      m.item = (uint8_t)it;
      bool ok = pet.addToBank(m);   // grava (save) se entrou
      if (ok) pet.registerSpecies(m.species, false, m.form);
      galleryDirty = true;
      Serial.printf("%s nv%u segurando %s -> %s (caixa %u, %u nela)\n", DEX_TBL[d].name,
                    Pet::levelOf(m), it ? ITEM_TBL[it].name : "nada",
                    ok ? "dentro" : "NO CABE", pet.boxOpen, pet.bankCount());
    } else {
      Serial.println("ERR: BANCO <1..DEX_COUNT> <nivel> [objeto]");
    }
    Serial.println("DONE");
  } else if (line.startsWith("ABIL ")) {
    // ABIL <0|1> <NOMBRE>  - fuerza la habilidad de un lado en pleno combate.
    // Sale sorteada en el huevo, asi que sin esto ver una concreta actuar es
    // cuestion de insistir hasta que toque.
    String resto = line.substring(5);
    resto.trim();
    int sp = resto.indexOf(' ');
    int lado = sp > 0 ? resto.substring(0, sp).toInt() : -1;
    String nom = sp > 0 ? resto.substring(sp + 1) : String();
    nom.trim();
    // uint16_t: ABIL_COUNT paso de 255 con Galar, y con un contador de un
    // byte la condicion `i < ABIL_COUNT` nunca es falsa — el bucle se
    // colgaria buscando una habilidad que no existe.
    uint16_t id = 0;
    for (uint16_t i = 1; i < ABIL_COUNT && !id; i++)
      if (nom.equalsIgnoreCase(ABIL_TBL[i].name)) id = i;
    if (id && lado == 0 && !bt.open) {
      // Sin combate abierto se le cambia al BICHO. Hace falta asi para probar
      // Intimidate y compania, que saltan al empezar y ya no hay vuelta atras.
      pet.ability = id;
      Serial.printf("el bicho ahora tiene %s\n", ABIL_TBL[id].name);
    } else if (bt.open && lado >= 0 && lado <= 1 && id) {
      bt.b.side[lado].ability = id;
      Serial.printf("%s ahora tiene %s\n", dexName(bt.b.side[lado].asDex),
                    ABIL_TBL[id].name);
    } else {
      Serial.println("ERR: ABIL <0|1> <nombre>");
    }
    Serial.println("DONE");
  } else if (line.startsWith("MOVE ")) {
    // MOVE <0|1> <slot> <NOMBRE>  - mete un golpe concreto en un hueco.
    // Los golpes salen del learnset, asi que sin esto ver un Solar Beam o un
    // Hyper Beam en el simulador es cuestion de dar con la especie y el nivel.
    String resto = line.substring(5);
    resto.trim();
    int sp1 = resto.indexOf(' ');
    int lado = sp1 > 0 ? resto.substring(0, sp1).toInt() : -1;
    String cola = sp1 > 0 ? resto.substring(sp1 + 1) : String();
    cola.trim();
    int sp2 = cola.indexOf(' ');
    int hueco = sp2 > 0 ? cola.substring(0, sp2).toInt() : -1;
    String nom = sp2 > 0 ? cola.substring(sp2 + 1) : String();
    nom.trim();
    nom.toUpperCase();
    uint16_t mv = MOVE_COUNT;
    for (uint16_t i = 0; i < MOVE_COUNT && mv == MOVE_COUNT; i++)
      if (nom == MOVE_TBL[i].name) mv = i;
    if (bt.open && lado >= 0 && lado <= 1 && hueco >= 0 && hueco < BT_MOVES &&
        mv < MOVE_COUNT) {
      Combatant &c = bt.b.side[lado];
      c.moves[hueco] = mv;
      c.usesLeft[hueco] = 9;
      if (hueco >= c.nMoves) c.nMoves = (uint8_t)(hueco + 1);
      Serial.printf("%s aprende %s en el hueco %d\n", dexName(c.asDex),
                    MOVE_TBL[mv].name, hueco);
    } else {
      Serial.println("ERR: MOVE <0|1> <0..3> <NOMBRE>, con un combate abierto");
    }
    Serial.println("DONE");
  } else if (line.startsWith("SOM ")) {
    // SOM <NOMBRE>  - toca el sonido de ese golpe como en combate y dice
    // si salio el suyo o habria caido al pitido, y por que. Solo suena:
    // no toca la partida. Para "tal golpe no suena" sin montar un combate.
    String nom = line.substring(4);
    nom.trim();
    nom.toUpperCase();
    uint16_t mv = MOVE_COUNT;
    for (uint16_t i = 0; i < MOVE_COUNT && mv == MOVE_COUNT; i++)
      if (nom == MOVE_TBL[i].name) mv = i;
    if (mv < MOVE_COUNT) {
      bool ok = movePlay(mv);
      Serial.printf("%s g%03u: %s (firma=%d sonido=%d)\n", MOVE_TBL[mv].name,
                    (unsigned)mv, ok ? "el suyo" : "PITIDO", audioMoveFirma(),
                    audioEnabled() ? audioVolume() : 0);
    } else {
      Serial.println("ERR: SOM <NOMBRE>");
    }
    Serial.println("DONE");
  } else if (line.startsWith("LVL ")) {
    // Desde que el nivel salio de la edad, esto tiene que tocar la EXP: poner
    // ageMinutes ya no subia de nivel y el comando mentia en silencio.
    int n = line.substring(4).toInt();
    if (n < 1) n = 1;
    if (n > LEVEL_CAP) n = LEVEL_CAP;
    pet.exp = Pet::expForLevel((uint16_t)n);
    pet.hpSync();
    pet.ppSync();
    Serial.printf("nivel=%u exp=%lu\n", pet.level(), (unsigned long)pet.exp);
    Serial.println("DONE");
  } else if (line.startsWith("TIME ")) {
    uint32_t e = (uint32_t)line.substring(5).toInt();
    rtcSetEpoch(e);
    pet.setClock(e);
    Serial.printf("rtc=%u\n", rtcEpoch());
    Serial.println("DONE");
  } else if (line.startsWith("RTCSET ")) {  // solo RTC (simular apagados en pruebas)
    rtcSetEpoch((uint32_t)line.substring(7).toInt());
    Serial.printf("rtc=%u\n", rtcEpoch());
    Serial.println("DONE");
  } else if (line == "TIME") {
    Serial.printf("rtc=%u\n", rtcEpoch());
    Serial.println("DONE");
  } else if (line.startsWith("CARTEL ")) {
    // CARTEL adios|medalla - solo PINTA el cartel, para poder mirarlo.
    // No toca el estado del bicho (ni especie, ni edad, ni medallas
    // ganadas): las dos cosas caducan solas en unos segundos.
    String q = line.substring(7);
    q.trim();
    if (q == "adios") {
      choiceKind = 2;
      choiceUntil = millis() + 12000;
    } else if (q.startsWith("medalla")) {
      int i = q.length() > 7 ? q.substring(7).toInt() : 0;
      if (i < 0 || i >= MED_COUNT) i = 0;
      pet.celebraMedalla((uint16_t)(1 << i));
    } else {
      Serial.println("uso: CARTEL adios | CARTEL medalla [0..7]");
    }
    Serial.println("DONE");
  } else if (line == "QUIZ") {
    // QUIZ - planta al Profesor YA. Mismo motivo que PC: con once minutos
    // entre visitas, probarlo de verdad seria esperar once minutos por
    // intento. `QUIZ YA` entra directo al cuestionario, sin tocar la marca.
    pet.quizNext = millis();
    pet.quizGone = pet.quizNext + QUIZ_STAY_MS;
    Serial.printf("profesor en pantalla %u ms\n", (unsigned)QUIZ_STAY_MS);
    Serial.println("DONE");
  } else if (line == "QUIZ YA") {
    quizStart();
    Serial.println("DONE");
  } else if (line == "PC") {
    // PC - baja la vida a todo el mundo y planta a la enfermera YA. Con
    // siete minutos entre visitas, probarla de verdad es esperar siete
    // minutos por intento.
    pet.hpLeft = (uint16_t)(pet.maxHp() / 3 + 1);
    for (uint8_t i = 0; i < 4; i++) if (pet.ppLeft[i] > 1) pet.ppLeft[i] = 1;
    for (uint8_t i = 0; i < TEAM_SLOTS; i++)
      if (pet.team[i].used()) pet.team[i].hpLeft = 1;
    pet.pcNext = millis();
    pet.pcGone = pet.pcNext + PC_STAY_MS;
    Serial.printf("enfermera en pantalla %u ms\n", (unsigned)PC_STAY_MS);
    Serial.println("DONE");
  } else if (line.startsWith("REPONER")) {
    // Devolve o time que a migracao comeu. ESCREVE: por isso o aviso,
    // a copia de seguranca e o UNDO.
    String arg = line.substring(7);
    arg.trim();
    // (dex, nivel, shiny, nome da megapedra ou "")
    struct Repo { int16_t dex; uint16_t lv; bool sh; const char *piedra; };
    static const Repo REPO[6] = {
      {  94, 50, true,  "Gengarite"      },   // Gengar shiny
      { 150, 50, false, ""               },   // Mewtwo
      {  59, 50, false, "Arcaninite"     },   // Arcanine
      {   6, 50, false, "Charizardite Y" },   // Charizard
      {   3, 50, false, "Venusaurite"    },   // Venusaur
      { 149, 50, false, ""               },   // Dragonite
    };

    if (arg == "UNDO") {
      if (!pet.equipoDeCopia("teambk2")) {
        Serial.println("no hay copia: nada que deshacer");
        Serial.println("DONE");
        return;
      }
      Serial.println("devuelto el equipo anterior");
      Serial.println("DONE");
      return;
    }

    if (arg != "JA") {
      Serial.println("REPONER pondria en el equipo de ESTA region:");
      for (uint8_t k = 0; k < 6; k++)
        Serial.printf("  #%d %s nv%u%s%s%s\n", REPO[k].dex,
                      DEX_TBL[REPO[k].dex].name, REPO[k].lv,
                      REPO[k].sh ? " SHINY" : "",
                      REPO[k].piedra[0] ? " con " : "", REPO[k].piedra);
      Serial.println("el equipo de ahora se copia a \"teambk2\" antes.");
      Serial.println("para hacerlo: REPONER JA   /   deshacer: REPONER UNDO");
      Serial.println("DONE");
      return;
    }

    // A COPIA, primeiro. Se isto falhar nao se escreve nada.
    if (!pet.equipoACopia("teambk2")) {
      Serial.println("no se pudo guardar la copia: NO se toca el equipo");
      Serial.println("DONE");
      return;
    }

    for (uint8_t i = 0; i < TEAM_SLOTS; i++) pet.team[i].species = 0;
    // O BICHO VIVO TEM DE FICAR QUIETO ENQUANTO SE ENCHE O EQUIPO.
    //
    // catchMon grava o save a cada bicho, e save() chama storeActive(),
    // que escreve o bicho VIVO no slot do titular. Resultado: o Gengar
    // entrava no slot 0 e era apagado pelo Totodile na mesma chamada,
    // e os outros cinco escorregavam um slot. Tentei corrigir DEPOIS do
    // catchMon e nao adiantou —— o atropelo acontece dentro dele.
    //
    // storeActive() comeca com `if (mainSlot >= TEAM_SLOTS) return;`,
    // entao pondo o mainSlot fora de alcance ele vira um nao-faz-nada.
    const uint8_t slotAntes = pet.mainSlot;
    pet.mainSlot = TEAM_SLOTS;
    for (uint8_t k = 0; k < 6; k++) {
      uint8_t donde = pet.catchMon(REPO[k].dex, REPO[k].lv, REPO[k].sh,
                                   ABIL_OF[REPO[k].dex][0]);
      (void)slotAntes;
      if (donde != CATCH_TEAM) {
        Serial.printf("  #%d no entro en el equipo (%u)\n", REPO[k].dex,
                      (unsigned)donde);
        continue;
      }
      if (!REPO[k].piedra[0]) continue;
      // A pedra POR NOME: indice fixo e o que se parte quando a tabela
      // cresce —— foi assim que os sons dos golpes passaram a tocar o
      // do vizinho.
      for (uint8_t it = 1; it < ITEM_COUNT; it++) {
        if (strcasecmp(itemAt(it).name, REPO[k].piedra) != 0) continue;
        for (uint8_t s = 0; s < TEAM_SLOTS; s++)
          if (pet.team[s].species == REPO[k].dex) { pet.team[s].item = it; break; }
        break;
      }
    }
    pet.mainSlot = 0;
    pet.cargaDelHueco(0);
    pet.saveNow();
    Serial.println("hecho. mira con EQUIPO; para deshacer, REPONER UNDO");
    Serial.println("DONE");
  } else if (line.startsWith("CAJA ")) {
    // SO LE. Qualquer caixa, sem abrir nenhuma: boxRead e const.
    uint8_t cn = (uint8_t)line.substring(5).toInt();
    if (cn >= BOX_COUNT) {
      Serial.println("caja fuera de rango");
      Serial.println("DONE");
      return;
    }
    static StoredMon tmp[BANK_SLOTS];
    if (!pet.boxRead(cn, tmp)) {
      Serial.printf("caja %u: no se pudo leer\n", cn);
      Serial.println("DONE");
      return;
    }
    uint8_t n = 0;
    for (uint8_t i = 0; i < BANK_SLOTS; i++) {
      const StoredMon &m = tmp[i];
      if (!m.used()) continue;
      n++;
      Serial.printf("  %2u: #%d %s nv%u%s%s\n", i, m.species,
                    (m.species >= 1 && m.species <= DEX_COUNT)
                        ? DEX_TBL[m.species].name : "?",
                    Pet::levelOf(m),
                    (m.flags & SM_SHINY) ? " SHINY" : "",
                    m.item ? " (lleva algo)" : "");
    }
    Serial.printf("caja %u: %u bichos\n", cn, n);
    Serial.println("DONE");
  } else if (line == "DIAG") {
    pet.diagSave();
    Serial.println("DONE");
  } else if (line == "BKTEAMS") {
    // O que ha na copia de seguranca dos times (barreira C).
    pet.diagBackup();
    Serial.println("DONE");
  } else if (line == "REGIONES") {
    // SO LE. Os nove times, a mascara de regioes pisadas e as
    // insignias de cada uma. Sem isto nao ha como saber onde foi parar
    // o que —— e um armazenamento novo sem leitor deixa cego justo na
    // hora de consertar.
    Serial.printf("aqui=%u (%s)  maxRegion=%u  visitadas=0x%03X\n",
                  pet.region, regionAt(pet.region).name, pet.maxRegion,
                  (unsigned)pet.visitedMask);
    for (uint8_t r = 0; r < REGION_COUNT; r++) {
      uint8_t n = 0;
      uint16_t alto = 0;
      // A regiao ONDE SE ESTA vive em `team`, nao em `teamsIn`: esse
      // so se sincroniza ao viajar. Ler o errado aqui mostraria a
      // regiao atual vazia e eu sairia a procurar um bug que nao ha.
      for (uint8_t i = 0; i < TEAM_SLOTS; i++) {
        const StoredMon &m = (r == pet.region) ? pet.team[i] : pet.teamsIn[r][i];
        if (!m.used()) continue;
        n++;
        uint16_t lv = Pet::levelOf(m);
        if (lv > alto) alto = lv;
      }
      Serial.printf("  %-7s %s pisada=%u insignias=%u  time=%u  maior=nv%u\n",
                    regionAt(r).name, r == pet.region ? "<-" : "  ",
                    pet.regionVisited(r) ? 1u : 0u,
                    (unsigned)pet.badgesOf(r), n, alto);
      for (uint8_t i = 0; i < TEAM_SLOTS; i++) {
        const StoredMon &m = (r == pet.region) ? pet.team[i] : pet.teamsIn[r][i];
        if (!m.used()) continue;
        Serial.printf("      %u: #%d %s nv%u\n", i, m.species,
                      (m.species >= 1 && m.species <= DEX_COUNT)
                          ? DEX_TBL[m.species].name : "?",
                      Pet::levelOf(m));
      }
    }
    Serial.println("DONE");
  } else if (line == "MAPA") {
    Serial.printf("region=%u zone=%u (%s)\n", pet.region, pet.region,
                  regionAt(pet.region).name);
    Serial.println("DONE");
  } else if (line.startsWith("WILD")) {
    // WILD [dex] [nivel]  - fuerza el aviso. Con 8% por minuto y 20 de
    // hueco, esperar a que salga uno de verdad son media hora por intento.
    String resto = line.substring(4);
    resto.trim();
    int sp = resto.indexOf(' ');
    int16_t d = (int16_t)(resto.length() ? (sp > 0 ? resto.substring(0, sp)
                                                   : resto).toInt() : 0);
    uint16_t nv = (uint16_t)(sp > 0 ? resto.substring(sp + 1).toInt() : 0);
    uint8_t hora = (uint8_t)(pet.lastSeenEpoch ? (pet.lastSeenEpoch / 3600) % 24 : 13);
    if (d < 1 || d > DEX_COUNT) d = pet.pickWildSpecies(hora);
    if (!nv) nv = pet.wildLevelFor(d);
    pet.wildDex = d;
    pet.wildLevel = nv;
    // una "S" al final fuerza shiny: 1 de cada 128 no hay forma de
    // provocarlo en un guion del simulador
    pet.wildShiny = (resto.indexOf('S') >= 0) || (random(WILD_SHINY_ODDS) == 0);
    // una "T" al final fuerza que sea un entrenador
    if (resto.indexOf('T') >= 0) {
      pet.wildTrainer = (int8_t)random(TRAINER_COUNT);
      pet.wildShiny = false;
    } else {
      pet.wildTrainer = -1;
    }
    pet.wildTtl = WILD_TTL_MIN;
    Serial.printf("salvaje: %s%s Nv.%u (techo BST %u, %u insignias)\n",
                  pet.wildShiny ? "*" : "", dexName(d), nv,
                  pet.wildBstCap(), pet.badges);
    Serial.println("DONE");
  } else if (line.startsWith("AGE ")) {
    // AGE <dias>  - envejece al bicho. La despedida pide tres dias de reloj
    // y no hay forma de probarla esperandolos.
    uint32_t d = (uint32_t)line.substring(4).toInt();
    pet.ageMinutes = d * 24UL * 60UL;
    Serial.printf("edad=%lu min (%lu dias) despedida=%d\n",
                  (unsigned long)pet.ageMinutes, (unsigned long)d,
                  pet.canFarewellNow());
    Serial.println("DONE");
  } else if (line == "LEGACY") {
    // Que dejaria si se despidiera ahora. Llegar a una despedida de verdad
    // son siete dias de reloj.
    Serial.printf("score=%u/%u suelo=%u puede=%d (nv.%u vin.%u desc.%u ins.%u)\n",
                  pet.legacyScore(), LEGACY_SCORE_MAX,
                  LEGACY_FLOOR_BASE + pet.legacyScore() * 2,
                  pet.canFarewellNow(), pet.level(), pet.bond,
                  pet.careMistakes, pet.badges);
    if (pet.hasLegacyEgg())
      Serial.printf("huevo pendiente: %s suelo=%u golpe=%s objeto=%s\n",
                    dexName(pet.legacyDex), pet.legacyFloor,
                    pet.legacyMove ? MOVE_TBL[pet.legacyMove].name : "-",
                    pet.legacyItem ? itemAt(pet.legacyItem).name : "-");
    Serial.println("DONE");
  } else if (line.startsWith("WINS ")) {
    // WINS <n>  - el porton del gimnasio: cuantos de los TRES de la casa
    // llevas. Pelearlos de verdad son tres combates enteros, y esto es
    // para poder mirar la pantalla del lider sin hacerlos.
    int v = line.substring(5).toInt();
    pet.gymTrainers = (uint8_t)(v < 0 ? 0 : (v > GYM_TRAINERS ? GYM_TRAINERS : v));
    Serial.printf("porton=%u/%u listo=%d\n", pet.gymTrainers, GYM_TRAINERS,
                  pet.gymReady());
    Serial.println("DONE");
  } else if (line.startsWith("BADGES ")) {
    pet.badges = (uint8_t)line.substring(7).toInt();
    // Al TOPE DE LA ESCALERA (13), no a GYM_COUNT (8, que es el tamano de
    // la tabla de techos de BST). Con el tope viejo no habia forma de
    // llegar a coronarse desde el simulador, y por tanto tampoco de probar
    // el viaje a Johto.
    if (pet.badges > pet.gymCount()) pet.badges = pet.gymCount();
    int n = 0;
    uint8_t hora = (uint8_t)(pet.lastSeenEpoch ? (pet.lastSeenEpoch / 3600) % 24 : 13);
    for (int16_t d = 1; d <= DEX_COUNT; d++) if (pet.wildWeight(d, hora)) n++;
    Serial.printf("insignias=%u techo=%u especies=%d\n", pet.badges,
                  pet.wildBstCap(), n);
    Serial.println("DONE");
  } else if (line == "SETTINGS") {
    // Abrir los ajustes es un deslizamiento hacia ABAJO, y los gestos no
    // salen fiables en los guiones del simulador (misma razon que CARD).
    openClock();
    Serial.println("DONE");
  } else if (line.startsWith("BOLA")) {
    // BOLA [escalon]  - tira una bola en el combate que haya abierto.
    // 0 Poke, 1 Super, 2 Ultra... Llegar aqui jugando es abrir la bolsa,
    // elegir pestana y tocar la bola: cuatro toques por coordenada, que es
    // justo lo que no sale fiable en un guion (misma razon que CARD).
    String resto = line.substring(4);
    resto.trim();
    int esc = resto.length() ? resto.toInt() : 0;
    if (!bt.open) {
      Serial.println("ERR: no hay combate abierto");
    } else if (bt.phase != BP_CHOOSE) {
      Serial.println("ERR: solo mientras toca elegir");
    } else {
      if (esc < 0) esc = 0;
      if (esc >= BALL_TIERS) esc = BALL_TIERS - 1;
      uint8_t id = BALL_ID[esc];
      pet.giveBall((uint8_t)esc);        // que haya una que tirar
      btThrowBall(id);
      Serial.printf("bola %s lanzada\n", itemAt(id).name);
    }
    Serial.println("DONE");
  } else if (line.startsWith("IDIOMA")) {
    // IDIOMA [n]  - 0 ES, 1 EN, 2 FR, 3 DE, 4 IT, 5 PT, 6 JA, 7 KO.
    // Sin numero, dice en cual esta. Existe para poder sacar la captura de
    // una pantalla en un idioma concreto: por los ajustes hay que dar la
    // vuelta a los ocho y es justo lo que no se puede escribir en un guion.
    String resto = line.substring(6);
    resto.trim();
    if (resto.length()) {
      int n = resto.toInt();
      if (n < 0 || n >= LANG_COUNT) {
        Serial.printf("ERR: idioma %d, hay %d\n", n, (int)LANG_COUNT);
      } else {
        setLang((Lang)n);
      }
    }
    // solo el numero: LANG_CODES se declara mas abajo en el fichero y el
    // .ino no genera prototipos para las variables, solo para las funciones
    Serial.printf("idioma %u\n", (unsigned)gLang);
    Serial.println("DONE");
  } else if (line.startsWith("MEGAS ")) {
    // MEGAS <dex>  - ABRE la ficha de esa especie y dice que versiones
    // le salieron y si el sprite de cada una se pudo cargar. Es lo que
    // separa "no esta apuntada" de "esta apuntada y no se ensena".
    int d = line.substring(6).toInt();
    if (d < 1 || d > DEX_COUNT) {
      Serial.printf("ERR: MEGAS <1..%d>\n", (int)DEX_COUNT);
    } else {
      openDexEntry((int16_t)d);
      Serial.printf("%s: registrada=%d  versiones=%u\n", dexName((int16_t)d),
                    (int)pet.isRegistered((int16_t)d), (unsigned)galleryNVers);
      for (uint8_t v = 0; v < galleryNVers; v++) {
        galleryVer = (int8_t)v;
        galleryCargaVer((int16_t)d);
        Serial.printf("  [%u] cod=%-6d %-14s sprite=%s\n", (unsigned)v,
                      (int)galleryVers[v], galleryVerNombre((int16_t)d),
                      galleryPmd.loaded ? "SI" : "NO");
      }
      galleryVer = 0;
      galleryDetail = 0;
      galleryOpen = false;
      galleryPmd.unload();
    }
    Serial.println("DONE");
  } else if (line == "MEGAS") {
    // Que megas tiene APUNTADAS la Pokedex y de cuales llevas la piedra.
    //
    // Existe para contestar "la mega de X no me sale" sin adivinar: si
    // sale APUNTADA, el fallo esta en ensenarla; si no sale, es que no
    // se ha megaevolucionado todavia (el apunte se hace al verlo pasar,
    // no al tener la piedra).
    uint16_t n = 0;
    for (uint16_t i = 0; i < MEGA_COUNT; i++) {
      bool reg = pet.isMegaRegistered((int16_t)i);
      uint8_t piedra = 0;
      for (uint16_t k = 1; k < ITEM_COUNT; k++)
        if (ITEM_TBL[k].fx == IT_MEGA && ITEM_TBL[k].arg == MEGA_TBL[i].item) {
          piedra = (uint8_t)k;
          break;
        }
      bool llevas = piedra && pet.bag[piedra];
      if (!reg && !llevas) continue;   // ni vista ni con piedra: no interesa
      n++;
      Serial.printf("  %3u %-14s base %-4d %-9s piedra:%s\n", (unsigned)i,
                    MEGA_TBL[i].name, (int)MEGA_TBL[i].base,
                    reg ? "APUNTADA" : "sin ver", llevas ? "SI" : "no");
    }
    Serial.printf("%u megas apuntadas o con piedra (de %d)\n",
                  (unsigned)n, (int)MEGA_COUNT);
    Serial.println("DONE");
  } else if (line.startsWith("CARD")) {
    // CARD [pagina]  - abre la ficha por esa pagina. El gesto de abrirla es
    // un deslizamiento y en los guiones del simulador no hay forma fiable de
    // reproducirlo, asi que sin esto las paginas nuevas no se pueden mirar.
    String resto = line.substring(4);
    resto.trim();
    int p = resto.length() ? resto.toInt() : 0;
    if (pet.isEgg()) {
      Serial.println("ERR: siendo huevo no hay ficha");
    } else {
      cardOpen = true;
      cardPage = (uint8_t)(p < 0 ? 0 : (p >= CARD_PAGES ? CARD_PAGES - 1 : p));
      cardSolo = (cardPage == 5 || cardPage == 7);   // as do mapa
      cardScroll = 0;
      movesSlot = MOVES_MAIN;
      Serial.printf("ficha pagina %u\n", cardPage);
    }
    Serial.println("DONE");
  } else if (line == "ITEMS") {
    // que hay en la bolsa y que lleva puesto
    Serial.printf("lleva: %s\n", itemAt(pet.heldItem).name);
    for (uint8_t b = 0; b < BP_COUNT; b++) {
      if (!pet.pocketCount(b)) continue;
      Serial.printf("[%s]\n", POCKET_NAME[b]);
      for (uint8_t n = 0; n < pet.pocketCount(b); n++) {
        uint8_t id = pet.pocketItem(b, n);
        Serial.printf("  %2u x%u  %s\n", id, pet.bag[id], ITEM_TBL[id].name);
      }
    }
    Serial.println("DONE");
  } else if (line.startsWith("GIVE ")) {
    // GIVE <id>       - mete uno en la bolsa
    int id = line.substring(5).toInt();
    if (pet.takeItem((uint8_t)id)) Serial.printf("+1 %s\n", ITEM_TBL[id].name);
    else Serial.printf("ERR: GIVE <1..%d>\n", ITEM_COUNT - 1);
    Serial.println("DONE");
  } else if (line.startsWith("HOLD ")) {
    // HOLD <id>       - se lo pone (0 = quitarselo)
    int id = line.substring(5).toInt();
    if (pet.holdItem((uint8_t)id)) Serial.printf("lleva: %s\n", itemAt(pet.heldItem).name);
    else Serial.println("ERR: no lo tiene en la bolsa");
    Serial.println("DONE");
  } else if (line.startsWith("USE ")) {
    int id = line.substring(4).toInt();
    if (pet.useItem((uint8_t)id)) Serial.printf("uso %s\n", ITEM_TBL[id].name);
    else Serial.println("ERR: no lo tiene, o no le haria nada");
    Serial.println("DONE");
  } else if (line.startsWith("REG ")) {
    // REG <dex> [dex...]  - marca especies como criadas. Sin esto, para ver
    // la galeria con algo dentro habria que criar 151 bichos.
    String resto = line.substring(4);
    resto.trim();
    int n = 0;
    while (resto.length()) {
      int sp = resto.indexOf(' ');
      int d = (sp > 0 ? resto.substring(0, sp) : resto).toInt();
      resto = sp > 0 ? resto.substring(sp + 1) : String();
      resto.trim();
      // de depuracion: registra la especie a secas, NUNCA como shiny
      if (d >= 1 && d <= DEX_COUNT) { pet.registerSpecies((int16_t)d, false); n++; }
    }
    galleryDirty = true;
    Serial.printf("registradas %d\n", n);
    Serial.println("DONE");
  } else if (line == "GAL" || line.startsWith("GAL ")) {
    // GAL [pagina]  - abre la galeria, opcionalmente por esa pagina. Pasar
    // de pagina es un deslizamiento, y con 16 paginas desde Johto llegar a
    // las de arriba a base de gestos en un guion no sale fiable (misma
    // razon que CARD). Sin pagina sigue siendo un interruptor.
    String resto = line.substring(3);
    resto.trim();
    if (resto.length()) {
      int p = resto.toInt();
      galleryOpen = true;
      galleryPage = p < 0 ? 0 : (p >= GAL_PAGES ? GAL_PAGES - 1 : p);
    } else {
      galleryOpen = !galleryOpen;
      galleryPage = 0;
    }
    galleryDetail = 0;
    galleryDirty = true;
    if (!galleryOpen) galleryPmd.unload();
    Serial.printf("galeria=%d pagina=%d/%d\n", galleryOpen, galleryPage, GAL_PAGES);
    Serial.println("DONE");
  } else if (line == "EGGS") {
    // simula 20 tiradas de huevo (no cambia el estado del juego)
    for (int i = 0; i < 20; i++) {
      int16_t d = pet.pickEggSpecies();
      Serial.printf("%d:%s(r%u) ", d, DEX_TBL[d].name, DEX_TBL[d].rarity);
    }
    Serial.println();
    Serial.println("DONE");
  } else if (line == "SHINY") {  // alterna shiny del actual (pruebas)
    pet.shiny = !pet.shiny;
    Serial.printf("shiny=%d\n", pet.shiny);
    Serial.println("DONE");
  } else if (line.startsWith("NICK ")) {
    pet.rename(line.substring(5).c_str());
    Serial.printf("nick=%s\n", pet.nick);
    Serial.println("DONE");
  } else if (line == "CAREDAY") {  // simula un dia nuevo cuidado (pruebas)
    pet.setClock(pet.lastSeenEpoch + 86400);
    pet.caress();
    Serial.printf("streak=%u bond=%u medals=0x%X\n", pet.streak, pet.bond, pet.medals);
    Serial.println("DONE");
  } else if (line == "BYE") {
    pet.startFarewell();
    Serial.println("DONE");
  } else if (line == "RUN") {
    pet.startRunaway();
    Serial.println("DONE");
  } else if (line == "BEEP") {
    sfxPlay(SFX_HATCH);  // prueba de audio
    Serial.println("DONE");
  } else if (line == "ABANDON") {
    pet.dbgRunawayReady();  // fuerza el estado "lista para escaparse" (test del boton)
    Serial.println("DONE");
  } else if (line == "WIPE") {
    pet.factoryReset();     // borra NVS y reinicia -> partida nueva (eleccion de inicial)
    Serial.println("DONE");
    delay(100);
    ESP.restart();
  } else if (line == "REG") {
    Serial.printf("pokedex %u/%u:", pet.registeredCount(), (unsigned)DEX_COUNT);
    for (int i = 1; i <= DEX_COUNT; i++)
      if (pet.isRegistered(i)) Serial.printf(" %d", i);
    Serial.println();
    Serial.println("DONE");
  } else if (line == "HEALTH") {
    // `dib` es lo que tarda un repintado: de ahi sale la cadencia, y
    // es el numero que delata un parpadeo negro antes de verlo.
    Serial.printf("up=%lus heap=%u min=%u sd=%d mon=%d dib=%lums\n",
                  (unsigned long)(millis() / 1000), ESP.getFreeHeap(),
                  ESP.getMinFreeHeap(), sdReady, pmd.loaded || mon.loaded,
                  (unsigned long)renderPeor);
    Serial.println("DONE");
  } else if (line.startsWith("BARS ")) {
    // BARS <com> <fel> <ene> <lim>  - pone las cuatro barras. Hace falta para
    // probar todo lo que depende de ellas (la energia como portero, los
    // descuidos, la escapada) sin esperar horas de juego.
    String resto = line.substring(5);
    resto.trim();
    int v[4] = { -1, -1, -1, -1 }, n = 0;
    while (n < 4 && resto.length()) {
      int sp = resto.indexOf(' ');
      v[n++] = (sp > 0 ? resto.substring(0, sp) : resto).toInt();
      resto = sp > 0 ? resto.substring(sp + 1) : String();
      resto.trim();
    }
    auto tope = [](int x) { return (uint8_t)(x > 100 ? 100 : x); };
    if (v[0] >= 0) pet.fullness = tope(v[0]);
    if (v[1] >= 0) pet.joy = tope(v[1]);
    if (v[2] >= 0) pet.energy = tope(v[2]);
    if (v[3] >= 0) pet.hygiene = tope(v[3]);
    Serial.printf("com=%u fel=%u ene=%u lim=%u  pelea=%d entrena=%d (%s)\n",
                  pet.fullness, pet.joy, pet.energy, pet.hygiene,
                  pet.canBattle(), pet.canTrain(), pet.whyNotBattle());
    Serial.println("DONE");
  } else if (line.startsWith("BOND ")) {
    // BOND <0..100>  - el vinculo sube 4 al dia como mucho, asi que ver la
    // desobediencia esperando a que baje solo son semanas.
    int v = line.substring(5).toInt();
    pet.bond = (uint8_t)(v < 0 ? 0 : (v > 100 ? 100 : v));
    Serial.printf("vinculo=%u  desobedece=%u%%\n", pet.bond,
                  Battle::disobeyChance(pet.bond));
    Serial.println("DONE");
  } else if (line.startsWith("EV ")) {
    // EV <hp> <atk> <def> <spa> <spd> <spe>  - pone los seis EVs a pelo.
    // Sirve para ver la pantalla del gimnasio con el presupuesto gastado sin
    // jugar veinte sesiones.
    String resto = line.substring(3);
    resto.trim();
    uint8_t *dst[EV_N] = { &pet.trHp, &pet.trAtk, &pet.trDef,
                           &pet.trSpa, &pet.trSpd, &pet.trSpe };
    for (uint8_t i = 0; i < EV_N && resto.length(); i++) {
      int sp = resto.indexOf(' ');
      int v = (sp > 0 ? resto.substring(0, sp) : resto).toInt();
      *dst[i] = (uint8_t)(v < 0 ? 0 : (v > EV_STAT_CAP ? EV_STAT_CAP : v));
      resto = sp > 0 ? resto.substring(sp + 1) : String();
      resto.trim();
    }
    Serial.printf("EV %u/%u/%u/%u/%u/%u  total=%u/%u\n", pet.trHp, pet.trAtk,
                  pet.trDef, pet.trSpa, pet.trSpd, pet.trSpe,
                  pet.evTotal(), EV_TOTAL_CAP);
    Serial.println("DONE");
  } else if (line.startsWith("MAIN ")) {
    // Pone de titular al del hueco N. Hace EXACTAMENTE lo que el boton de
    // la pantalla de equipo (pet.setMain), asi que no es un atajo que deje
    // la partida en un estado que el juego no sepa alcanzar.
    //
    // Existe para poder REPARAR: con EQUIPO se ve que hueco esta mal y con
    // esto se puede traer al titular para arreglarlo. Sin el, el unico
    // comando que escribia la especie (SPEC) solo alcanzaba al titular y
    // no habia forma de llegar a los otros cinco.
    int sl = line.substring(5).toInt();
    if (sl >= 0 && sl < TEAM_SLOTS && pet.team[sl].used()) {
      if (pet.setMain((uint8_t)sl))
        Serial.printf("titular ahora: hueco %d, #%d %s\n", sl,
                      pet.speciesId,
                      (pet.speciesId >= 1 && pet.speciesId <= DEX_COUNT)
                          ? DEX_TBL[pet.speciesId].name : "?");
      else
        Serial.println("MAIN: setMain dijo que no (ya es el titular, o hay "
                       "una ceremonia en marcha)");
    } else {
      Serial.println("MAIN: hueco vacio o fuera de rango");
    }
    Serial.println("DONE");
  } else if (line == "TALLA") {
    // El alto de CUERPO (la tinta) que se acaba de pintar, y el que se
    // habia pedido. Los dos, porque la pregunta util no es "cuanto mide"
    // sino "sale lo que se pidio": esa resta es la que destapo que el
    // rival se estaba escalando con el bicho del jugador.
    Serial.printf("magnitud pintada=%d pedida=%d | alto real=%d px (casa, #%d %s)\n",
                  (int)ultimaMagPintada,
                  (int)(altoPedido(-1, HOME_K) + 0.5f),
                  (int)ultimoAltoPintado,
                  pet.speciesId,
                  (pet.speciesId >= 1 && pet.speciesId <= DEX_COUNT)
                      ? DEX_TBL[pet.speciesId].name : "?");
    if (bt.open)
      Serial.printf("combate (magnitud): tuyo pide %d sale %d, alto %d | rival pide %d sale %d, alto %d\n",
                    (int)btPedido[BT_PLAYER],
                    (int)btAltoDibujado[BT_PLAYER],
                    (int)btAltoReal[BT_PLAYER],
                    (int)btPedido[BT_FOE], (int)btAltoDibujado[BT_FOE],
                    (int)btAltoReal[BT_FOE]);
    Serial.println("DONE");
  } else if (line == "CAJAS") {
    // Cuantos hay en cada caja, SIN tocar nada. Mismo motivo que EQUIPO:
    // la migracion del banco viejo (60 en la NVS -> cajas en la SD) corre
    // sobre la partida de verdad, y sin un comando que lea no hay forma
    // de comprobar que no se perdio nadie por el camino.
    uint16_t total = 0;
    for (uint8_t c = 0; c < BOX_COUNT; c++) {
      uint8_t n = pet.boxCount(c);
      total = (uint16_t)(total + n);
      if (n) Serial.printf("  caja %2u: %u/%u%s\n",
                           c, n, BANK_SLOTS,
                           c == pet.boxOpen ? "  <- abierta" : "");
    }
    Serial.printf("total en cajas: %u (de %u huecos)\n", total,
                  (unsigned)(BOX_COUNT * BANK_SLOTS));
    // Y lo que no se ve desde fuera: si la tarjeta deja escribir una
    // caja. Si no deja, una captura con la caja llena acaba en
    // "no hay sitio" aunque queden 930 huecos.
    Serial.printf("escribir en /boxes: %s\n",
                  pet.pruebaDeEscritura() ? "SI" : "NO");
    // Y cuanto le queda a la NVS. Un save que no cabe devuelve 0 y
    // hasta hoy no decia nada: el bloque del banco era el mas gordo
    // (60 x 52 = 3.120 bytes) y con las cajas bajo a la mitad.
    {
      Preferences g;
      g.begin("tamapoke", true);
      Serial.printf("NVS libre: %u entradas\n",
                    (unsigned)g.freeEntries());
      g.end();
    }
    Serial.println("DONE");
  } else if (line == "TROCA") {
    // Abre a tela de troca (liga o radio). Nao grava nada: a troca so
    // acontece com os toques das DUAS pessoas na tela.
    if (!trocaOpen) {
      cardOpen = false;
      mapOpen = false;
      trocaAbre();
    }
    Serial.printf("troca aberta=%d radio=%d%s", trocaOpen, radioLigado(), "\n");
    Serial.println("DONE");
  } else if (line == "TROCAST") {
    // SO LE: estado do protocolo, vizinhos ouvidos e o diario do SD.
    Serial.printf("aberta=%d radio=%d estado=%d motivo=%d sessao=%08lX%s", trocaOpen,
                  radioLigado(), (int)troca.estado, troca.motivo,
                  (unsigned long)troca.sessao, "\n");
    for (uint8_t i = 0; i < troca.nViz; i++)
      Serial.printf("  viz %02X%02X #%d nv%u%s", troca.viz[i].mac[4], troca.viz[i].mac[5],
                    troca.viz[i].titular, troca.viz[i].nivel, "\n");
    TrocaDiario d;
    if (Pet::trocaLeDiario(d))
      Serial.printf("diario sessao=%08lX estado=%u slot=%u%s", (unsigned long)d.sessao,
                    d.estado, d.slot, "\n");
    else
      Serial.printf("sem diario%s", "\n");
    Serial.printf("trocaUltima=%08lX%s", (unsigned long)pet.trocaUltima, "\n");
    Serial.println("DONE");
  } else if (line == "EQUIPO") {
    // Lee el equipo y el banco SIN tocar nada. Hacia falta y no estaba:
    // cuando `SPEC` dejo un Pidgeotto con los golpes de un Dratini en la
    // partida del usuario, no habia forma de ver en que hueco habia
    // quedado el estropicio. Un comando que escribe sin que exista el que
    // lee deja a ciegas justo cuando hay que reparar.
    Serial.printf("titular: #%d %s nv%u\n", pet.speciesId,
                  (pet.speciesId >= 1 && pet.speciesId <= DEX_COUNT)
                      ? DEX_TBL[pet.speciesId].name : "?",
                  pet.level());
    for (uint8_t i = 0; i < TEAM_SLOTS; i++) {
      const StoredMon &m = pet.team[i];
      if (!m.used()) { Serial.printf("  equipo %u: vacio\n", i); continue; }
      Serial.printf("  equipo %u:%s #%d %-12s nv%u", i,
                    i == pet.mainSlot ? "*" : " ", m.species,
                    (m.species >= 1 && m.species <= DEX_COUNT)
                        ? DEX_TBL[m.species].name : "?",
                    Pet::levelOf(m));
      for (uint8_t j = 0; j < 4; j++)
        if (m.moves[j]) Serial.printf(" | %s", MOVE_TBL[m.moves[j]].name);
      Serial.println();
    }
    uint8_t nb = 0;
    for (uint8_t i = 0; i < BANK_SLOTS; i++) {
      const StoredMon &m = pet.bank[i];
      if (!m.used()) continue;
      nb++;
      Serial.printf("  banco  %u:  #%d %-12s nv%u\n", i, m.species,
                    (m.species >= 1 && m.species <= DEX_COUNT)
                        ? DEX_TBL[m.species].name : "?",
                    Pet::levelOf(m));
    }
    Serial.printf("  (%u en equipo, %u en banco)\n", pet.teamCount(), nb);
    Serial.println("DONE");
  } else if (line == "STATS") {
    Serial.printf("spec=%d nv=%u com=%u fel=%u ene=%u lim=%u desc=%u sd=%d mon=%d bat=%d usb=%d rtc=%u\n",
                  pet.speciesId, pet.level(), pet.fullness, pet.joy, pet.energy,
                  pet.hygiene, pet.careMistakes, sdReady, mon.loaded,
                  batPercent(), usbPresent(), rtcEpoch());
    Serial.printf("peso=%u fue=%u def=%u vel=%u genes=%u/%u/%u tr=%u/%u/%u baya=%d\n",
                  pet.weight, pet.atkStat(), pet.defStat(), pet.speStat(),
                  pet.geneAtk, pet.geneDef, pet.geneSpe,
                  pet.trAtk, pet.trDef, pet.trSpe, pet.berryKnown);
    Serial.printf("shiny=%d streak=%u/%u bond=%u medals=0x%X(%u) nick=%s\n",
                  pet.shiny, pet.streak, pet.bestStreak, pet.bond, pet.medals,
                  pet.totalMedals, pet.nick);
    Serial.println("DONE");
  }
}

// ---------- entrada tactil ----------

bool inPetZone(int16_t x, int16_t y) {
  return x > 110 && x < 356 && y > 95 && y < 310;
}

// el toque se resuelve al LEVANTAR el dedo para distinguir tap de deslizar
void handleTouch() {
  static uint32_t lastPoll = 0;
  if (millis() - lastPoll < 20) return;  // 50 Hz le sobra a un dedo
  lastPoll = millis();
  // solo tocamos el bus si el chip aviso por INT o si el dedo sigue abajo (hay
  // que detectar el levantamiento). Leer el CST9217 dormido se colgaba ~1s y
  // congelaba el loop entero; SensorLib no respeta el timeout de Wire.
  if (!gTouchIrq && !wasPressed) return;
  gTouchIrq = false;
  // Un ciclo de solo direccion antes de leer. Sin esto, getPoint() se colgaba
  // exactamente 1000 ms (el timeout por defecto del driver I2C, que SensorLib no
  // acota) y congelaba el loop entero: es lo que se percibia como "el minijuego
  // se congela 2-3 segundos al tocar la bola" (issue #16), porque ahi los toques
  // son rapidos y seguidos.
  //
  // Medido en placa, jugando lo mismo (~470 lecturas, marcador ~50):
  //   sin esta linea: 5 parones de 1000 ms en 60 s
  //   con ella:       0
  // El contador de rechazos salio 0 en ambos casos, asi que NO funciona
  // saltandose lecturas cuando el chip no contesta: lo que hace es despertarlo,
  // para que la lectura siguiente no se encuentre el CST9217 dormido.
  Wire.beginTransmission(TOUCH_ADDR);
  if (Wire.endTransmission() != 0) return;  // no responde: se reintenta en 20 ms
  int16_t x, y;
  bool pressed = touch.getPoint(&x, &y, 1) > 0;

  // gimnasio: cada toque cuenta al instante, sin esperar a soltar. El reloj
  // corre y hacer pasar el golpe por el detector de gestos le metia un
  // retardo que en una ronda de 15 s se nota.
  if (gymOpen) {
    if (pressed && !wasPressed) {
      lastInteract = millis();
      gymTap(x, y);
    }
    wasPressed = pressed;
    return;
  }

  if (pressed && !wasPressed) {  // empieza el gesto
    tX0 = tXl = x;
    tY0 = tYl = y;
    tStart = millis();
    holdFired = false;
    swallowGesture = (dimStage > 0) || screenOff;  // si estaba a oscuras, solo despierta
    screenOff = false;
    lastInteract = millis();
  } else if (pressed) {  // sigue apoyado
    tXl = x;
    tYl = y;
    // pulsacion larga sin moverse sobre el bicho -> dialogo de soltar
    if (!holdFired && !swallowGesture && !galleryOpen && !cardOpen && !kbOpen && !clockOpen && millis() - tStart > 3000 &&
        abs(tXl - tX0) < 30 && abs(tYl - tY0) < 30 && inPetZone(tX0, tY0) &&
        !pet.isEgg() && !confirmUntil && !pet.ceremony) {
      confirmUntil = millis() + 10000;
      holdFired = true;
    }
  } else if (wasPressed) {  // levanta el dedo: resolver gesto
    lastInteract = millis();
    int dx = tXl - tX0, dy = tYl - tY0;
    uint32_t dt = millis() - tStart;
    if (!holdFired && !swallowGesture) {
      if (abs(dx) > 80 && abs(dy) < 70 && dt < 800) onSwipe(dx > 0 ? 1 : -1);
      else if (abs(dy) > 80 && abs(dx) < 70 && dt < 800) onSwipeV(dy > 0 ? 1 : -1);
      else if (dt < 1500 && abs(dx) < 40 && abs(dy) < 40) onTap(tX0, tY0);
    }
  }
  wasPressed = pressed;
}

// A posicao de uma pagina na sequencia de arrastar (CARD_SEQ).
static int cardSeqIdx(uint8_t p) {
  for (int i = 0; i < CARD_SEQ_N; i++)
    if (CARD_SEQ[i] == p) return i;
  return 0;
}

// deslizar vertical: abre/cierra la ficha del bicho
void openClock();  // prototipo

bool btBagSwipeV(int dir);  // capture.ino
bool rematchSwipeV(int dir);

void onSwipeV(int dir) {
  if (introOpen || pet.awaitingStarter() || avOpen) return;  // intro/inicial/aventura
  if (trocaOpen) return;   // a troca sai pelos botoes dela
  if (btBagSwipeV(dir)) return;     // la bolsa del combate: mueve la lista
  if (rematchSwipeV(dir)) return;   // y la de la revancha
  if (gymOpen || galleryOpen || kbOpen || pet.ceremony) return;
  if (clockOpen) { clockOpen = false; resetStep = 0; return; }
  if (cardOpen) {
    if (dir < 0) cardOpen = false;  // arriba cierra la ficha
    return;
  }
  if (dir > 0) {                    // deslizar abajo: ajustar hora
    if (!confirmUntil && !feedMenuUntil) openClock();
  } else if (!pet.isEgg() && !confirmUntil && !feedMenuUntil) {
    cardOpen = true;                // deslizar arriba: ficha
    cardSolo = false;
    cardPage = 0;
    movesSlot = MOVES_MAIN;
  }
}

// deslizar: dir +1 = hacia la derecha
void onSwipe(int dir) {
  if (introOpen || pet.awaitingStarter() || avOpen) return;  // intro/inicial/aventura
  if (trocaOpen) return;
  if (gymOpen || kbOpen || clockOpen) return;
  if (cardOpen) {  // dentro de la ficha: cambiar de pagina
    if (cardSolo) return;   // ginasio/medalhas do mapa: uma pagina so
    int p = cardSeqIdx(cardPage) + (dir > 0 ? -1 : 1);  // izquierda avanza
    p = p < 0 ? 0 : (p > CARD_SEQ_N - 1 ? CARD_SEQ_N - 1 : p);
    cardPage = CARD_SEQ[p];
    cardScroll = 0;   // cada pagina empieza por arriba
    bankView = false;
    bankSoltar = -1;
    movesSlot = MOVES_MAIN;   // deslizando se miran los golpes del titular
    return;
  }
  if (mapOpen) {
    // Se sale POR DONDE SE ENTRO: el mapa se abre arrastrando a la
    // DERECHA, asi que se cierra arrastrando a la izquierda. Y a la
    // derecha, la region siguiente.
    if (dir < 0) {
      mapImg.unload();
      mapOpen = false;
    } else {
      uint8_t tope = (uint8_t)(pet.region + 1 < REGION_COUNT ? pet.region + 1
                                                             : pet.region);
      if (mapView < tope) { mapView++; sfxPlay(SFX_TAP); }
    }
    return;
  }
  if (!galleryOpen) {
    if (pet.ceremony || confirmUntil) return;
    // DERECHA el mapa, IZQUIERDA la Pokedex. Antes los dos lados abrian la
    // Pokedex, o sea que medio gesto no hacia nada distinto; el mapa se
    // queda con ese medio gesto que sobraba en vez de con un boton mas en
    // una pantalla que ya tiene cinco.
    //
    // Los lados van asi y no al reves porque la Pokedex lleva de siempre
    // arrastrando a la izquierda: cambiarla de sitio por meter el mapa
    // habria sido mover lo conocido para hacer hueco a lo nuevo.
    if (dir > 0) {
      if (pet.isEgg()) return;   // con un huevo no hay donde viajar
      mapView = pet.region;        // se abre por donde estas, no por Kanto
      mapOpen = true;
      sfxPlay(SFX_TAP);
      return;
    }
    galleryOpen = true;
    galleryPage = 0;
    galleryDetail = 0;
    galleryDirty = true;
    return;
  }
  if (galleryDetail) {  // en detalle: volver a la rejilla
    galleryDetail = 0;
    galleryPmd.unload();
    galleryDirty = true;
    return;
  }
  int np = galleryPage - dir;  // deslizar a la izquierda avanza pagina
  if (np < 0) {                // retroceder desde la primera = salir
    galleryOpen = false;
    galleryPmd.unload();
    return;
  }
  if (np > GAL_PAGES - 1) np = GAL_PAGES - 1;
  if (np != galleryPage) {
    galleryPage = np;
    galleryDirty = true;
  }
}

void onTap(int16_t x, int16_t y) {
  // Serial.printf("TOUCH %d %d\n", x, y);  // diagnostico (silenciado: satura el log)
  if (introOpen) {              // la intro se come todo hasta que termina
    introTap(x, y);
    return;
  }
  if (pet.awaitingStarter()) {  // primera partida: elegir inicial
    for (int i = 0; i < 3; i++) {
      int ry = STARTER_ROW_Y + i * (STARTER_ROW_H + STARTER_ROW_GAP);
      if (x >= 70 && x <= 396 && y >= ry && y <= ry + STARTER_ROW_H) {
        pet.chooseStarter(starterDex((uint8_t)i));
        sfxPlay(SFX_TAP);
        break;
      }
    }
    return;
  }
  // o combate entre placas abre POR CIMA da troca (que segue viva por baixo)
  if (trocaOpen && !(bt.open && bt.pvp)) {
    trocaTap(x, y);
    return;
  }
  if (bt.open) {
    battleTap(x, y);
    return;
  }
  if (avOpen && !apagarUntil) { aventuraTap(x, y); return; }
  if (mapOpen) {
    mapTap(x, y);
    return;
  }
  if (rematchOpen) {
    rematchTap(x, y);
    return;
  }
  if (galleryOpen) {
    galleryTap(x, y);
    return;
  }
  if (kbOpen) {
    keyboardTap(x, y);
    return;
  }
  if (clockOpen) {
    clockTap(x, y);
    return;
  }
  if (quizOpen) {
    quizTap(x, y);
    return;
  }
  if (pet.ceremony) return;  // durante la despedida no hay botones
  if (cardOpen) {
    if (cardPage == 2) { cardMovesTap(x, y); return; }
    if (cardPage == 3) { cardBagTap(x, y); return; }
    if (cardPage == 4) { cardTeamTap(x, y); return; }
    if (cardPage == 5) { cardGymTap(x, y); return; }
    if (cardPage == 7) { cardMedalsTap(x, y); return; }
    if (cardPage == 1 && abilShown) { abilShown = false; return; }  // cerrar el cuadro
    if (cardPage == 0 && y < 84) openKeyboard();  // tocar el nombre = renombrar
    else if (cardPage == 0 && y < 84 + UI_FOSO) { }   // un pelo debajo: nada
    // El boton de IR AL GIMNASIO ya no esta (ver renderCardStats): ese
    // trozo de pantalla lo ocupa ahora la tabla de debilidades, que no
    // se toca. Dejar el ramo aqui habria hecho que tocar una chapa
    // abriese el entrenamiento sin venir a cuento.
    // La franja de la habilidad mide 22 px de alto dibujada (2 mm): se
    // toca en 38, y un pelo fuera no cierra la ficha.
    else if (cardPage == 1 && pet.ability && y >= ABIL_Y0 && y <= ABIL_Y1 &&
               x >= 96 && x <= 356) {
      abilShown = true;            // tocar la habilidad: contar que hace
      sfxPlay(SFX_TAP);
    } else if (cardPage == 1 && pet.ability &&
               !lejosDe(x, y, 96, ABIL_Y0, 356, ABIL_Y1)) {
    } else {
      cardOpen = false;
    }
    return;
  }
  if (choiceKind) {          // dialogo de decision: boton accion (arriba) / mantener (abajo)
    // So se ABRIU agora mesmo (resta entre 11,4 e 12 s). Um choiceUntil
    // velho, de outra tela, nunca cai nessa janela e nao trava o toque.
    {
      const int32_t resta = (int32_t)(choiceUntil - millis());
      if (resta > CHOICE_MS - CHOICE_GRACA_MS && resta <= CHOICE_MS) return;
    }
    bool b1 = (x >= 93 && x <= 373 && y >= 206 && y <= 258);  // accion
    bool b2 = (x >= 93 && x <= 373 && y >= 268 && y <= 320);  // mantener / quedaros
    if (choiceKind == 1) {                 // evolucion
      if (b1) {
      int16_t old = pet.speciesId;
      pet.evolve();
      evoPmd.load(old, pet.shiny);
      evoFromDex = old;
      // EL ORDEN DEL ANIL (pbEvolution): el grito del de ANTES, la musica
      // de evolucion en bucle mientras cambia, y al acabar el grito del
      // NUEVO y despues la fanfarria. Antes sonaban la fanfarria y el
      // grito nuevo a la vez, al tocar, y el grito cortaba la fanfarria
      // (los dos van por el mismo canal).
      cryPlay((uint16_t)old);
      musicPlay(MUS_EVOLVE);
      evoFase = 1;
    }
      else if (b2) pet.declineEvolve();
    } else if (choiceKind == 2) {          // despedida
      if (b1) pet.startFarewell();
      else if (b2) pet.declineFarewell();
    } else if (choiceKind == 3) {          // escapada: AL REVES que los otros
      // Arriba (donde cae el segundo toque) va QUEDARSE; dejarlo ir es
      // el de abajo. Perder el bicho tiene que ser una decision.
      if (b2) pet.startRunaway();
    }
    choiceKind = 0;
    return;
  }
  if (apagarUntil) {         // la pregunta de apagar: GUARDAR / APAGAR
    if (timeLeft(apagarUntil) && x >= APA_BX && x <= APA_BX + APA_BW) {
      bool guardar = (y >= APA_B1Y && y <= APA_B1Y + APA_BH);
      bool apagar  = (y >= APA_B2Y && y <= APA_B2Y + APA_BH);
      if (guardar || apagar) {
        // Volcar SIEMPRE antes de cortar: lo que diferencia a los dos es
        // la marca, no el guardado. pet.guardar() ya vuelca por dentro.
        if (guardar) pet.guardar(rtcEpoch());
        else         pet.saveNow();
        sfxPlay(SFX_TAP);
        // Un acuse de recibo antes de cortar. Sin esto la pantalla se va a
        // negro igual pulses lo que pulses, y no hay forma de saber cual
        // de los dos cogio — que es exactamente la duda que trajo el
        // "lo puse para apagar y no pauso".
        gfx->fillScreen(RGB565_BLACK);
        gfx->fillCircle(CX, CY, 231, C565(0x10, 0x14, 0x1c));
        const char *ack = guardar ? T(S_APA_PAUSADO) : T(S_APA_TCHAU);
        gfx->setTextColor(guardar ? UI_BAR_OK : UI_BAR_BAD);
        setSize(3);
        setCur(centerX(ack, 3), CY - 12);
        printT(ack);
        const char *sub = guardar ? T(S_APA_ESPERA) : T(S_APA_DESL_SUB);
        gfx->setTextColor(UI_WHITE);
        setSize(1);
        setCur(centerX(sub, 1), CY + 24);
        printT(sub);
        gfx->flush();
        delay(1100);  // y de paso el volcado a NVS sale entero
        pwrOff();
        return;
      }
    }
    apagarUntil = 0;   // fuera de los botones: no era eso
    return;
  }
  if (confirmUntil) {        // dialogo "soltar?": SI / NO
    if (timeLeft(confirmUntil) && x >= 118 && x <= 218 && y >= 252 && y <= 304) {
      pet.release();
    }
    confirmUntil = 0;
    return;
  }
  if (feedMenuUntil) {       // selector de comida
    if (timeLeft(feedMenuUntil) && y >= 282 && y <= 352 && x >= 101 && x <= 365) {
      int item = (x - 101) / 66;
      if (item > 3) item = 3;
      if (item == 3) pet.feedCandy();
      else pet.feedBerry(item);
      sfxPlay(SFX_EAT);
    } else if (timeLeft(feedMenuUntil) && !lejosDe(x, y, 101, 282, 365, 352)) {
      return;                  // un pelo al lado de la comida: sigue abierto
    }
    feedMenuUntil = 0;
    return;
  }
  if (pet.isEgg()) {
    pet.eggTap();
    if (pet.isEgg()) {
      sfxPlay(SFX_TAP);
    } else {
      // acaba de nacer: fanfarria y su primer grito
      jinglePlay(JIN_HATCH);
      cryPlay((uint16_t)pet.speciesId);
    }
    return;
  }
  // La enfermera y el mapa, por delante de todo lo de la escena y en el
  // mismo orden en que se pintan.
  if (!pet.isEgg() && !pet.evolving()) {
    if (pet.pcWaiting(millis())) {
      int dx = x - PC_MARK_X, dy = y - PC_MARK_Y;
      if (dx * dx + dy * dy <= (PC_MARK_R + 10) * (PC_MARK_R + 10)) {
        healCount = pet.healAll();
        healUntil = millis() + 2200;
        jinglePlay(JIN_HEAL);
        return;
      }
    }
  }
  // El Profesor, en el mismo orden en que se pinta.
  if (!pet.isEgg() && !pet.evolving() && pet.quizWaiting(millis())) {
    int dx = x - OAK_MARK_X, dy = y - OAK_MARK_Y;
    if (dx * dx + dy * dy <= (OAK_MARK_R + 10) * (OAK_MARK_R + 10)) {
      quizStart();
      return;
    }
  }
  // el "!": tocarlo es meterse en la hierba. Va ANTES que los botones y que
  // la caricia porque esta encima de la escena y es lo que el jugador esta
  // mirando en ese momento.
  if (pet.wildWaiting()) {
    int dx = x - WILD_MARK_X, dy = y - WILD_MARK_Y;
    if (dx * dx + dy * dy <= (WILD_MARK_R + 10) * (WILD_MARK_R + 10)) {
      int16_t d = pet.wildDex;
      uint16_t nv = pet.wildLevel;
      bool sh = pet.wildShiny;   // hay que cogerlo ANTES de limpiar el aviso
      int8_t tr = pet.wildTrainer;
      pet.wildClear();
      battleStart(d, nv, -1, sh, tr);
      return;
    }
  }
  // La chapa de evolucion. Es UNA, y la comparten el principal y el
  // suplente en el mismo orden en que se pintan: si el principal tiene la
  // suya, es la suya; si no, la del banquillo.
  if (x >= EVO_BTN_X && x <= EVO_BTN_X + EVO_BTN_W &&
      y >= EVO_BTN_Y && y <= EVO_BTN_Y + EVO_BTN_H) {
    if (pet.wantEvolveButton()) {
      choiceKind = 1; choiceUntil = millis() + 12000;
      return;
    }
    // ...y solo si no hay ningun otro CTA ocupando la pantalla, que es la
    // misma condicion con la que se dibuja.
    if (!pet.canRunawayNow() && !pet.wantFarewellButton()) {
      uint8_t slot = pet.reserveReadyToEvolve();
      if (slot != 0xFF && pet.evolveReserve(slot)) {
        pet.saveNow();
        sfxPlay(SFX_EVOLVE);
        return;
      }
    }
  }
  // botones de final (mismo recuadro): escapada directa; despedida abre dialogo
  if (x >= FAR_BTN_X && x <= FAR_BTN_X + FAR_BTN_W &&
      y >= FAR_BTN_Y && y <= FAR_BTN_Y + FAR_BTN_H) {
    if (pet.canRunawayNow()) { choiceKind = 3; choiceUntil = millis() + CHOICE_MS; return; }
    if (pet.wantFarewellButton()) { choiceKind = 2; choiceUntil = millis() + 12000; return; }
  }
  for (int i = 0; i < BTN_N; i++) {
    int dx = x - buttons[i].cx, dy = y - buttons[i].cy;
    if (dx * dx + dy * dy <= BTN_HIT * BTN_HIT) {
      Serial.printf("BTN %d\n", i);
      if (i == 0) {
        sfxPlay(SFX_TAP);
        if (!pet.sleeping) feedMenuUntil = millis() + 6000;
      } else if (i == 1) {
        sfxPlay(SFX_TAP);
        startGym();
      } else if (i == 2) {
        // CAZAR. El aviso que sale es el mismo que el de la hierba de
        // siempre, asi que a partir de aqui todo sigue igual.
        uint32_t ahora = millis();
        uint8_t hora = (uint8_t)(pet.lastSeenEpoch ? (pet.lastSeenEpoch / 3600) % 24 : 13);
        if (pet.huntReady(ahora) && pet.hunt(hora, ahora)) {
          sfxPlay(SFX_WILD);
          // O sorteio ja escolheu. Em vez de largar o "!" no alto da
          // tela e esperar um segundo toque, guarda-se o que saiu, faz-se
          // o balao de decidido ao lado do bicho e o combate entra
          // sozinho quando ele acabar.
          determDex = pet.wildDex;
          determLv = pet.wildLevel;
          determShiny = pet.wildShiny;
          determTrainer = pet.wildTrainer;
          pet.wildClear();
          determUntil = ahora + DETERM_MS;
        } else {
          sfxPlay(SFX_DENY);
        }
      } else if (i == 3) {
        sfxPlay(SFX_TAP);
        if (!pet.sleeping) aventuraComeca();
      } else if (i == 4) {
        sfxPlay(SFX_TAP);
        pet.toggleLight();
      } else {
        sfxPlay(SFX_TAP);
        startBath();
      }
      return;
    }
  }
  // tocar al bicho = caricia
  if (inPetZone(x, y)) {
    Serial.println("PET");
    pet.caress();
    if (!pet.sleeping) cryPlay((uint16_t)pet.speciesId);
  }
}

// ---------- render ----------

bool gNight = false;  // noche real (por hora) o durmiendo: lo fija render()
uint16_t inkColor() { return gNight ? UI_INK_NIGHT : UI_INK; }

// La banda de abajo dejo de ser beige y paso a ser una chapa oscura (ver
// drawHomePanel), asi que lo que va DENTRO de ella se escribe en claro pase
// lo que pase con el dia y la noche.
uint16_t panelInk() { return UI_WHITE; }

// ---------- escena de fondo: bioma del tipo + hora real del RTC ----------

#define HORIZON 232  // linea donde el cielo se encuentra con el suelo

uint16_t lerp565(uint16_t a, uint16_t b, int i, int n) {
  if (n <= 0) return a;
  int ar = (a >> 11) & 31, ag = (a >> 5) & 63, ab = a & 31;
  int br = (b >> 11) & 31, bg = (b >> 5) & 63, bb = b & 31;
  return (uint16_t)((((ar + (br - ar) * i / n) << 11)) |
                    (((ag + (bg - ag) * i / n) << 5)) | (ab + (bb - ab) * i / n));
}

// hora del dia 0-23 (de la hora real cacheada cada 30s; 13 si no hay reloj)
int sceneHour() {
  uint32_t e = pet.lastSeenEpoch;
  return e ? (int)((e / 3600) % 24) : 13;
}

// suelo de cada bioma de dia (de noche se mezcla hacia el azul nocturno)
static const uint16_t BIOME_SOIL[6] = {
  C565(0x7e, 0xc0, 0x7f),  // 0 pradera
  C565(0xdc, 0xca, 0x94),  // 1 playa (arena)
  C565(0x4f, 0x8a, 0x55),  // 2 bosque
  C565(0x8a, 0x55, 0x44),  // 3 volcan
  C565(0xa8, 0x90, 0x6a),  // 4 montana
  C565(0xe6, 0xee, 0xf5),  // 5 nieve
};

// ---------- la chapa de la POKEDEX ----------
//
// Los tonos son los de UI/Pokedex/bg_list.png del Anil, medidos pixel a
// pixel: casi negro azulado, rejilla apenas mas clara, una bola fantasma de
// marca de agua y paneles blancos con filo teal. Se DIBUJA en vez de
// empaquetarse porque son cuatro colores planos, unas lineas y un circulo:
// como bitmap serian 424 KB y asi no cuesta nada y vale para la pantalla
// redonda, que es lo que ninguna captura del Anil encaja.
#define DEX_BG    C565(0x0a, 0x0f, 0x16)
#define DEX_GRID  C565(0x10, 0x18, 0x21)
#define DEX_MARK  C565(0x17, 0x20, 0x29)
#define DEX_TEAL  C565(0x29, 0x7b, 0x73)
#define DEX_PAPER C565(0xef, 0xef, 0xef)
// La silueta de lo que aun no has criado. Era INK_K, que sobre el beige de
// antes se leia y sobre esta pantalla casi negra desaparecia.
#define DEX_SIL   C565(0x24, 0x33, 0x40)
// Leva 72: o que voce ja VIU mas nao teve. Nem a cor (nao e seu) nem o
// escuro de quem nunca apareceu: uma silhueta clara.
#define DEX_VISTO C565(0x9c, 0xac, 0xbc)
// e o anel de quem esta sendo CACADO, na grade
#define DEX_CACA  C565(0xf0, 0x80, 0x30)
#define DEX_STEP 32

// La ficha (las ocho paginas) usa la MISMA chapa que la Pokedex, asi que su
// tinta esta invertida: lo que era oscuro sobre beige ahora es claro sobre
// casi negro. Se nombran aparte en vez de reusar UI_INK para que se vea de
// un vistazo que ese trozo de codigo pinta sobre fondo oscuro.
#define CARD_INK   DEX_PAPER
// RELLENO de pastilla (fila activa, medalla, pestana apagada). NO sirve de
// color de texto sobre el fondo oscuro: ahi da 3,36 de contraste y la
// fuente es de 5x7 px. Para eso esta CARD_SOFT.
#define CARD_DIM   C565(0x4e, 0x6b, 0x72)
// Texto SECUNDARIO sobre el fondo oscuro de la ficha. Medido: 5,45 sobre
// DEX_BG (la WCAG pide 4,5), y sigue leyendose como apagado al lado de
// CARD_INK, que da 16,7. Un solo color hacia los dos trabajos y en el
// oscuro no se leia ninguno de los dos bien.
#define CARD_SOFT  C565(0x76, 0x8c, 0x94)
// Radio de la pantalla, para calcular cuanto ancho hay de verdad a cada
// altura: a y=72 la cuerda da 336 px, no 466.
#define BAG_TAB_R 233
#define CARD_PAPER C565(0x0f, 0x18, 0x20)

void drawDexSkin() {
  gfx->fillScreen(RGB565_BLACK);
  gfx->fillCircle(CX, CY, 231, DEX_BG);
  // La rejilla se recorta al circulo calculando la cuerda a cada altura; con
  // lineas enteras se saldria por el borde negro.
  for (int x = CX - 224; x <= CX + 224; x += DEX_STEP) {
    int dx = x - CX, h = (int)sqrtf((float)(231 * 231 - dx * dx));
    gfx->fillRect(x, CY - h, 1, h * 2, DEX_GRID);
  }
  for (int y = CY - 224; y <= CY + 224; y += DEX_STEP) {
    int dy = y - CY, w = (int)sqrtf((float)(231 * 231 - dy * dy));
    gfx->fillRect(CX - w, y, w * 2, 1, DEX_GRID);
  }
  // la bola de marca de agua, como en el original
  for (int r = 117; r <= 118; r++) gfx->drawCircle(CX, CY, r, DEX_MARK);
  for (int r = 33; r <= 34; r++) gfx->drawCircle(CX, CY, r, DEX_MARK);
  gfx->fillRect(CX - 118, CY - 2, 236, 5, DEX_MARK);
}

// Panel blanco con el filo teal y la barra gorda a la izquierda: es el
// recuadro que la Pokedex del Anil usa para todo.
void drawDexBox(int x, int y, int w, int h) {
  gfx->fillRect(x, y, w, h, DEX_TEAL);
  gfx->fillRect(x + 9, y + 2, w - 11, h - 4, DEX_PAPER);
}

// ---------- la chapa de HGSS ----------
//
// Los cuatro tonos del marco de Windowskins/speech hgss 1.png del Anil,
// medidos pixel a pixel. Viven aqui arriba porque los usan tanto la banda de
// casa (justo debajo) como las cajas de dialogo (drawWindow, mas abajo), y
// una macro solo vale a partir de donde se escribe.
#define WIN_DARK  C565(0x48, 0x40, 0x40)
#define WIN_GOLD  C565(0xf8, 0xd8, 0x68)
#define WIN_GOLD2 C565(0xe8, 0xa0, 0x38)
#define WIN_SHADE C565(0x60, 0x60, 0x68)
#define WIN_PAPER C565(0xf8, 0xf8, 0xf8)

// La banda de abajo. Era un rectangulo beige plano y es lo primero que se ve
// al encender; ademas, con una foto de escenario detras, el beige se comia
// el paisaje. Ahora es la misma chapa oscura con filo dorado que las cajas
// de texto, que es lo que hace que el conjunto parezca de un juego.
#define HOME_PANEL_Y 306
extern SdBg homeBg;

void drawHomePanel() {
  // O MESMO fundo da Pokedex e da ficha: quase preto azulado, rejilla apagada e
  // filo teal. E o que da cara de jogo em vez de uma faixa cinza.
  gfx->fillRect(0, HOME_PANEL_Y, 466, 466 - HOME_PANEL_Y, DEX_BG);
  for (int x = CX - 224; x <= CX + 224; x += DEX_STEP)
    gfx->fillRect(x, HOME_PANEL_Y, 1, 466 - HOME_PANEL_Y, DEX_GRID);
  for (int y = CY + 224; y > HOME_PANEL_Y; y -= DEX_STEP) gfx->fillRect(0, y, 466, 1, DEX_GRID);
  gfx->fillRect(0, HOME_PANEL_Y, 466, 3, DEX_TEAL);
  gfx->fillRect(0, HOME_PANEL_Y + 3, 466, 1, DEX_PAPER);
}


void drawClouds(uint32_t now, uint16_t col) {
  for (int k = 0; k < 2; k++) {
    int cx = (int)((now / 50 + k * 250) % 560) - 40;
    int cy = 70 + k * 34;
    gfx->fillCircle(cx, cy, 16, col);
    gfx->fillCircle(cx + 18, cy + 3, 13, col);
    gfx->fillCircle(cx - 15, cy + 4, 12, col);
  }
}

// El escenario de casa, de la SD. Es un SdBg APARTE del del combate: si
// compartieran, cada pelea desalojaria el de casa y al volver habria que
// releer 424 KB de tarjeta con el parpadeo correspondiente.
SdBg homeBg;

// De noche manda la noche, y de dia el bioma del bicho. Cada uno tiene
// VARIOS paisajes (bgs.h) y se van turnando: con uno solo, una pantalla que
// esta siempre encendida cansa en un dia.
//
// El turno no es un dado por fotograma â€” seria un estroboscopio â€” sino un
// tramo de HOME_BG_MIN minutos de encendido. Asi cambia solo, de vez en
// cuando, y nunca a media mirada.
#define HOME_BG_MIN 20

static void loadHomeBg(uint8_t biome, bool night) {
  uint32_t tramo = millis() / (HOME_BG_MIN * 60UL * 1000UL);
  const BgSet &s = night ? NIGHT_BGS : bgSetOfBiome(biome);
  homeBg.load(bgPick(s, tramo + biome));
}

// Un escenario de /bg a pantalla completa, para las pantallas que no son ni
// la casa ni el combate (la intro del profesor, la eleccion de inicial).
// Reaprovecha `homeBg` porque ninguna de ellas coincide en el tiempo con la
// pantalla de casa; lo unico que cuesta es releer 424 KB al salir, y de esas
// pantallas se sale una vez en la vida del cacharro.
bool drawSceneBg(uint8_t id) {
  if (!homeBg.load(id)) return false;
  if (homeBg.w != LCD_WIDTH || homeBg.h != LCD_HEIGHT) return false;
  gfx->draw16bitRGBBitmap(0, 0, homeBg.pix, homeBg.w, homeBg.h);
  return true;
}

// La musica de casa se turna igual que el paisaje y por el mismo motivo:
// una sola pista en la pantalla que esta siempre encendida cansa en una
// tarde. Va al MISMO tramo de HOME_BG_MIN minutos, asi que el cambio de
// cancion coincide con el de escenario y se lee como "paso el rato" en vez
// de como dos cosas sueltas. De noche hay una fija.
//
// musicPlay() ignora la peticion si ya suena esa, asi que llamar desde el
// bucle de dibujo no relanza nada.
// La cola sonora de la evolucion: 1 = cambiando (suena MUS_EVOLVE),
// 2 = ya cambio y suena el grito del nuevo, 0 = nada. La lleva el bucle de
// dibujo, como la musica de casa, porque la animacion la lleva el reloj.
uint8_t evoFase = 0;
static void evoSonido() {
  if (evoFase == 1 && !pet.evolving()) {
    cryPlay((uint16_t)pet.speciesId);   // el grito NUEVO
    evoFase = 2;
  } else if (evoFase == 2 && !audioOneShotBusy()) {
    musicStop();                         // pbBGMStop
    jinglePlay(JIN_EVOLVE);              // pbMEPlay("Evolution success")
    evoFase = 0;
  }
}

static void homeMusic(bool night) {
  evoSonido();
  if (evoFase) return;                   // la de evolucion no se pisa
  if (bt.open || galleryOpen) return;    // el combate y la Pokedex mandan
  // Ni encima de la fanfarria de victoria, que suena justo al volver.
  if (audioOneShotBusy()) return;
  uint32_t tramo = millis() / (HOME_BG_MIN * 60UL * 1000UL);
  musicPlay(night ? (uint8_t)MUS_HOME_NIGHT
                  : (uint8_t)(MUS_HOME + tramo % MUS_HOME_COUNT));
}

void drawScene(uint8_t biome, uint32_t now, bool night) {
  loadHomeBg(biome, night);
  homeMusic(night);
  if (homeBg.loaded && homeBg.w == LCD_WIDTH && homeBg.h == LCD_HEIGHT) {
    gfx->draw16bitRGBBitmap(0, 0, homeBg.pix, homeBg.w, homeBg.h);
    // Las estrellas solo si el paisaje de noche NO las trae ya pintadas
    // (Nocturno tiene luna y cielo estrellado propios).
    if (night && homeBg.id == BG_NIGHT)
      for (auto &st : STARS) gfx->fillRect(st[0], st[1], 4, 4, UI_WHITE);
    return;
  }

  // Sin tarjeta (o sin /bg) se pinta el cielo de siempre: bandas de color,
  // sol o luna y cuatro formas por bioma. No es bonito, pero el cacharro
  // tiene que arrancar sin tarjeta.
  int h = sceneHour();
  uint16_t top, bot;
  if (night)            { top = C565(0x0c, 0x12, 0x24); bot = C565(0x1e, 0x26, 0x46); }
  else if (h < 8)       { top = C565(0xd1, 0x6a, 0x86); bot = C565(0xf3, 0xb8, 0x7c); }  // amanecer
  else if (h < 18)      { top = C565(0x8f, 0xc8, 0xea); bot = C565(0xdc, 0xee, 0xe6); }  // dia
  else                  { top = C565(0xc7, 0x5a, 0x4a); bot = C565(0xf0, 0xae, 0x64); }  // atardecer

  // cielo en bandas
  for (int y = 0; y < HORIZON; y += 8)
    gfx->fillRect(0, y, 466, 8, lerp565(top, bot, y, HORIZON));

  // sol o luna
  if (night) {
    gfx->fillCircle(360, 78, 24, C565(0xe8, 0xee, 0xf5));
    gfx->fillCircle(370, 72, 22, lerp565(top, bot, 78, HORIZON));  // creciente
    for (auto &st : STARS) gfx->fillRect(st[0], st[1], 4, 4, UI_WHITE);
  } else if (h < 18) {
    gfx->fillCircle(360, 84, 26, h < 8 ? C565(0xff, 0xd9, 0x8a) : C565(0xff, 0xe7, 0x9f));
    drawClouds(now, C565(0xff, 0xff, 0xff));
  } else {
    gfx->fillCircle(233, HORIZON - 6, 34, C565(0xff, 0xf1, 0xc8));  // sol poniente
  }

  // mar de la playa: una franja de agua sobre la arena
  uint16_t soil = BIOME_SOIL[biome < 6 ? biome : 0];
  if (night) soil = lerp565(soil, C565(0x16, 0x1c, 0x30), 9, 16);
  if (biome == 1) {
    uint16_t sea = night ? C565(0x1c, 0x34, 0x52) : C565(0x4f, 0x96, 0xc4);
    gfx->fillRect(0, HORIZON - 26, 466, 26, sea);
    for (int i = 0; i < 3; i++) {
      int wy = HORIZON - 22 + i * 7;
      uint16_t fc = night ? C565(0x3a, 0x58, 0x78) : C565(0xbf, 0xe6, 0xf5);
      gfx->fillRect(60 + ((now / 60 + i * 30) % 60), wy, 26, 2, fc);
      gfx->fillRect(300 - ((now / 60 + i * 20) % 60), wy, 26, 2, fc);
    }
  }

  // suelo
  gfx->fillRect(0, HORIZON, 466, 466 - HORIZON, soil);
  uint16_t hill = lerp565(soil, night ? C565(0x0c, 0x12, 0x24) : C565(0xff, 0xff, 0xff), 3, 16);
  gfx->fillRoundRect(-60, HORIZON - 14, 586, 60, 30, hill);

  // detalles del bioma
  uint16_t dk = lerp565(soil, C565(0x10, 0x18, 0x20), night ? 11 : 7, 16);
  if (biome == 2) {  // bosque: coniferas en silueta
    for (int tx : { 60, 150, 360, 416 }) {
      gfx->fillTriangle(tx, HORIZON - 46, tx - 16, HORIZON, tx + 16, HORIZON, dk);
      gfx->fillTriangle(tx, HORIZON - 60, tx - 12, HORIZON - 28, tx + 12, HORIZON - 28, dk);
    }
  } else if (biome == 3) {  // volcan: rocas y brasas
    gfx->fillTriangle(70, HORIZON, 40, HORIZON + 30, 100, HORIZON + 30, dk);
    gfx->fillTriangle(400, HORIZON + 4, 372, HORIZON + 30, 430, HORIZON + 30, dk);
    if (!night)
      for (int e = 0; e < 4; e++)
        gfx->fillRect(120 + e * 70, HORIZON + 8 + (e % 2) * 6, 4, 4, C565(0xff, 0x9b, 0x3a));
  } else if (biome == 4) {  // montana: cumbres al fondo
    gfx->fillTriangle(140, HORIZON - 50, 60, HORIZON, 220, HORIZON, dk);
    gfx->fillTriangle(330, HORIZON - 38, 250, HORIZON, 410, HORIZON, dk);
  } else if (biome == 5 && !night) {  // nieve: copos cayendo
    for (int f = 0; f < 10; f++) {
      int fx = (f * 53 + now / 40) % 466;
      int fy = (f * 90 + now / 18) % HORIZON;
      gfx->fillRect(fx, fy, 3, 3, UI_WHITE);
    }
  } else if (biome == 0) {  // pradera: matas de hierba
    for (int gx : { 80, 175, 300, 395 })
      for (int b = -1; b <= 1; b++)
        gfx->fillRect(gx + b * 5, HORIZON + 6, 2, 8 + (b == 0 ? 4 : 0), dk);
  }
}

// primera partida: elige inicial entre Bulbasaur / Charmander / Squirtle
void renderStarterSelect() {
  gfx->fillScreen(RGB565_BLACK);
  // el laboratorio de verdad; sin tarjeta, el circulo claro de siempre
  if (!drawSceneBg(BG_LAB)) gfx->fillCircle(CX, CY, 231, UI_BG_DAY);
  const char *t = T(S_CHOOSE_STARTER);
  printOutlined(centerX(t, 2), 68, t, 2, UI_WHITE);
  for (int i = 0; i < 3; i++) {
    int16_t d = starterDex((uint8_t)i);
    const DexEntry &de = DEX_TBL[d];
    int ry = STARTER_ROW_Y + i * (STARTER_ROW_H + STARTER_ROW_GAP);
    gfx->fillRoundRect(70, ry, 326, STARTER_ROW_H, 14, lerp565(de.accent, UI_WHITE, 6, 8));
    gfx->drawRoundRect(70, ry, 326, STARTER_ROW_H, 14, de.accent);
    // La miniatura se ajusta a la fila en vez de ir a escala fija: con los
    // iconos de Anil (hasta 40 px) una escala 3 se salia de la caja.
    const uint8_t *th = thumbs.get(d);
    if (th) drawThumbFit(th, 112, ry + STARTER_ROW_H / 2, STARTER_ROW_H - 10, false);
    gfx->setTextColor(UI_INK);
    setSize(3);
    setCur(178, ry + 24);
    printT(dexName(d));
  }
  gfx->flush();
}

// ---------- texto: tamano y cursor (preparado para fuentes CJK) ----------
// La fuente clasica ancla el cursor en la ESQUINA SUPERIOR del texto, y las 74
// coordenadas Y del codigo estan escritas asi. Las fuentes U8g2 lo anclan en la
// LINEA BASE (drawChar hace curY = y - baseline*size), asi que al cambiar de
// fuente todo el texto subiria. setCur() compensa esa diferencia en un solo
// sitio, para que la Y siga significando "arriba" con cualquier fuente.
//
// setSize() existe solo porque la libreria tiene textsize_x protegido y setCur()
// necesita saber la escala activa para multiplicar el ascenso.
//
// Con la fuente clasica gFontAscent es 0, asi que setCur(x,y) es exactamente
// setCur(x,y): esto no mueve un pixel en los seis idiomas actuales.
uint8_t gTextSize = 1;
int gFontAscent = 0;  // px del borde superior a la linea base, 0 = fuente clasica

// --- ajuste de la fuente CJK (los dos valores van juntos) ---
// La fuente clasica mide 8 px de alto a escala 1. Si la CJK mide mas, hay que
// dividir la escala para que los tamanos cuadren... pero dividir ADELGAZA el
// trazo: a escala 1 la unifont pinta lineas de 1 px mientras el latino a escala
// 2 las pinta de 2, y el japones se ve tenue aunque mida igual.
//
// Por eso conviene una fuente base pequena Y EN NEGRITA (_b_), que permita usar
// la MISMA escala que el latino: asi el grosor coincide y no hay que dividir.
//
// Se probo una fuente realmente negrita (b10_b) y NO sirve a este tamano: los
// trazos se juntan y los kana pierden detalle, ilegibles incluso para un lector
// nativo. Se vuelve a la unifont, que es la mas legible, y el trazo fino se
// compensa en printT() repintando 1 px desplazado (pseudo-negrita).
// japanese3 y no japanese1: los subconjuntos 1 y 2 traen kana y kanji pero
// NINGUNA puntuacion CJK, y Arduino_GFX no dibuja nada cuando le falta el
// glifo (ni avanza el cursor), asi que los 25 signos ï¼ y ï¼Ÿ de las cadenas
// desaparecian. Es la misma familia unifont: cabecera de metricas identica y
// superconjunto estricto, comprobado glifo a glifo sobre los 226 codepoints
// que usa el firmware. Cuesta 102 KB mas de flash y no mueve un pixel.
// El coreano necesita una fuente con hangul, y CJK_FONT era un solo #define.
// En vez de dar por hecho que un subconjunto sirve para las dos escrituras, la
// fuente se elige por idioma. korean2 y no korean1: el coreano de aqui usa 296
// silabas distintas y todas estan en KS X 1001, el conjunto de 2350 que es el
// tamano medido para korean2; korean1 trae 478. La comprobacion mira la
// pertenencia a KS X 1001, no recorre la tabla de la fuente, asi que se apoya
// en que el conjunto de korean2 sea ese; el ASCII se da por presente.
// Sin comprobar en placa: si CJK_SIZE_DIV 2 le sienta a korean2 igual que a la
// japonesa. El ascenso si se mide en tiempo de ejecucion en applyLangFont().
#define CJK_FONT_JA u8g2_font_unifont_t_japanese3
#define CJK_FONT_KO u8g2_font_unifont_t_korean2
#define CJK_SIZE_DIV 2

// ---- la fuente REDONDA ----
// La 5x7 clasica de Adafruit es cuadrada y seca; los numeros (niveles,
// PS, PP, el numero de la Pokedex) se leian como una calculadora. Esta es
// "power clear bold", la de la era DS, que el Anil trae en Fonts/ y que
// tools/gen_font.py convierte a GFXfont.
//
// Cuanto avanza una letra. Era un 6 suelto repetido por media docena de
// sitios; ahora tiene nombre, que es lo que permitio PROBAR otra fuente
// cambiando un numero.
//
// Y se probo: "power clear" y "power clear bold" del Anil (las de los
// juegos), convertidas con tools/gen_font.py, a 6, 7 y 8 px de avance.
// Resultado: PEOR que la 5x7. A seis los digitos se tocan; a siete entran
// pero cada caja de la pantalla esta medida para seis, asi que "SUPPORT"
// se come al "PP 40/40" y los nombres largos se aprietan contra el borde.
// La 5x7 clasica, que parece sosa, es la que cabe. Para cambiarla de
// verdad habria que rehacer las cajas, no solo la fuente.
#define LATIN_ADV 6

void setSize(uint8_t n) {
  // gTextSize guarda la escala REALMENTE aplicada, no la pedida: setCur()
  // multiplica el ascenso por ella y tiene que cuadrar con lo que se pinta.
  gTextSize = gCjkFont ? (n >= CJK_SIZE_DIV ? n / CJK_SIZE_DIV : 1) : n;
  gfx->setTextSize(gTextSize);
}

void setCur(int x, int y) {
  gfx->setCursor(x, y + gFontAscent * gTextSize);
}

// Fija la fuente del idioma activo. Es el UNICO sitio que la toca: el resto del
// codigo dibuja igual para todos los idiomas gracias a textW()/setCur().
//
// El ascenso se mide en vez de codificarlo: getTextBounds() devuelve y1 como
// desplazamiento del borde superior respecto al cursor, negativo con las fuentes
// U8g2 (que anclan en la linea base). Asi setCur() puede seguir tratando la Y
// como "arriba" sea cual sea la fuente, sin constantes magicas por fuente.
void applyLangFont() {
  gCjkFont = LANG_IS_CJK(gLang);
  if (!gCjkFont) {
    gfx->setFont();            // 5x7 clasica, CP437: la Y ya es el borde superior
    gfx->setUTF8Print(false);
    gFontAscent = 0;
    return;
  }
  gfx->setFont((gLang == LANG_KO) ? CJK_FONT_KO : CJK_FONT_JA);
  gfx->setUTF8Print(true);     // las cadenas CJK son UTF-8 multibyte
  int16_t x1, y1;
  uint16_t w, h;
  uint8_t antes = gTextSize;
  gfx->setTextSize(1);
  gfx->getTextBounds("A", 0, 0, &x1, &y1, &w, &h);
  gFontAscent = -y1;           // y1 negativo: subir desde la linea base
  gfx->setTextSize(antes);
}

// ---------- medida de texto (preparado para fuentes CJK) ----------
// La fuente clasica de Arduino_GFX avanza 6 px por caracter a tamano 1, y la UI
// centraba con strlen(s)*6*n a pelo. Eso solo vale para un byte por caracter y
// ancho fijo: con una fuente U8g2 para japones/coreano/chino, ni strlen() cuenta
// caracteres (UTF-8 es multibyte) ni todos miden lo mismo. Centralizarlo aqui es
// lo que permitira anadir esos idiomas sin tocar cada punto de dibujado.
//
// Para la fuente clasica devuelve EXACTAMENTE la misma cuenta que habia antes,
// asi que este cambio no mueve un pixel en los seis idiomas actuales.
uint16_t textW(const char *s, uint8_t size) {
  if (!gCjkFont) return (uint16_t)strlen(s) * LATIN_ADV * size;
  int16_t x1, y1;
  uint16_t w, h;
  gfx->getTextBounds(s, 0, 0, &x1, &y1, &w, &h);
  return w;
}

// x del cursor para dejar el texto centrado en CX
int centerX(const char *s, uint8_t size) { return CX - textW(s, size) / 2; }

// EL ANCHO QUE DE VERDAD HAY a esa altura. El panel es un circulo de 466,
// asi que a y=52 caben 294 px y no 466; una frase centrada mas ancha se
// sale por los DOS lados. `alto` es el alto de la linea, porque la banda
// se estrecha hacia los bordes y manda la fila peor.
int anchoUtil(int y, int alto) {
  int peor = -1;
  for (int k = 0; k <= 1; k++) {
    int yy = y + (k ? alto : 0);
    int dy = yy - CY;
    int d2 = CX * CX - dy * dy;
    int w = d2 > 0 ? 2 * (int)sqrtf((float)d2) : 0;
    if (peor < 0 || w < peor) peor = w;
  }
  return peor > 16 ? peor - 16 : 0;   // 8 px de aire a cada lado
}

// El tamano mas grande (hasta `max`) con el que la frase cabe ahi. Existe
// porque las cadenas cambian de largo con el idioma: el aleman hace
// palabras del doble que el ingles y nadie puede repasar 110 frases por 8
// idiomas en cada pantalla. Lo comprueba test_ui.cpp pasando los guiones
// del simulador en los ocho.
uint8_t tallaQueCabe(const char *s, int y, uint8_t max, int extra) {
  for (uint8_t t = max; t > 1; t--)
    if (textW(s, t) + extra <= anchoUtil(y, 8 * t)) return t;
  return 1;
}

// Imprime respetando la fuente activa. Con la CJK repinta el texto desplazado
// 1 px: la unifont a escala reducida deja trazos de 1 px y se ve tenue, y una
// fuente de verdad negrita a este tamano junta los trazos y estropea los kana.
// Repintar engorda el trazo sin deformar el glifo. Probado en placa por
// usakomint, que comparo las dos opciones en japones.
// El teclado de apodos es solo alfabeto (se deja asi a proposito), de modo que
// aqui nunca hay CJK y no hace falta engordar nada.
void printT(char c) { gfx->print(c); }

// Lo que de verdad cabe desde donde esta el cursor hasta el borde del
// CRISTAL, que es redondo. Devuelve la misma cadena si cabe entera.
//
// Existe por el japones: encoger la letra (tallaQueCabe) salva los seis
// idiomas latinos, pero la fuente CJK ya va al minimo y no hay de donde
// encoger —— "リザードンがなにかいいたそう" mide 540 px y empezaba a
// pintarse en x=-37. Lo que cae fuera del cristal no se ve de todas
// formas; con ".." al menos se sabe que hay mas.
static const char *recortaAlPanel(const char *s) {
  static char buf[160];
  if (!s || !*s) return s;
  int x = gfx->getCursorX();
  int y = gfx->getCursorY();
  int alto = 8 * gTextSize;
  // el borde derecho del cristal en la fila PEOR de esta linea
  int der = CX * 2;
  for (int k = 0; k <= 1; k++) {
    int dy = (y + (k ? alto : 0)) - CY;
    int d2 = CX * CX - dy * dy;
    int b = d2 > 0 ? CX + (int)sqrtf((float)d2) : CX;
    if (b < der) der = b;
  }
  der -= 8;                        // el mismo aire que anchoUtil
  int izq = CX * 2 - der;          // el circulo es simetrico
  // Empezar FUERA por la izquierda: pasa cuando alguien centra una frase
  // mas ancha que el panel (centerX da x negativo). Se arrima al borde y
  // lo que sobre por la derecha lo corta el resto de la funcion.
  if (x < izq) {
    gfx->setCursor(izq, y);
    x = izq;
  }
  if (x >= der) return s;          // el cursor ya esta fuera: no es cosa de esto
  int hueco = der - x;
  if (textW(s, gTextSize) <= hueco) return s;
  size_t n = 0, ult = 0;
  while (s[n] && n < sizeof(buf) - 8) {
    size_t paso = 1;
    unsigned char c = (unsigned char)s[n];
    if (c >= 0xC0) paso = (c >= 0xF0) ? 4 : (c >= 0xE0) ? 3 : 2;
    for (size_t k = 0; k < paso && s[n + k]; k++) buf[n + k] = s[n + k];
    buf[n + paso] = 0;
    if (textW(buf, gTextSize) > hueco) break;
    n += paso;
    ult = n;
  }
  // sitio para los dos puntos: se quita un caracter mas si hace falta
  buf[ult] = 0;
  while (ult > 0 && textW(buf, gTextSize) + textW("..", gTextSize) > hueco) {
    do { ult--; } while (ult > 0 && ((unsigned char)buf[ult] & 0xC0) == 0x80);
    buf[ult] = 0;
  }
  buf[ult] = '.';
  buf[ult + 1] = '.';
  buf[ult + 2] = 0;
  return buf;
}

// Pinta la cadena partiendola en los codigos con til (ver i18n.h). Entre
// uno y otro va un solo print() por trozo, no por letra: el texto es lo
// que mas se dibuja en esta pantalla.
//
// La letra se pinta DESPUES de la '~' y en la misma posicion, asi que el
// cursor acaba justo donde lo habria dejado un print normal: 6 px por
// byte, como cuenta textW().
static void imprimeConTil(const char *s) {
  const char *ini = s;
  for (const char *p = s;; p++) {
    unsigned char c = (unsigned char)*p;
    if (c && c != (unsigned char)CH_ATIL && c != (unsigned char)CH_OTIL &&
        c != (unsigned char)CH_ATILM && c != (unsigned char)CH_OTILM)
      continue;
    if (p > ini) {
      char trozo[192];
      size_t n = (size_t)(p - ini);
      if (n >= sizeof(trozo)) n = sizeof(trozo) - 1;
      memcpy(trozo, ini, n);
      trozo[n] = 0;
      gfx->print(trozo);
    }
    if (!c) return;
    // La minuscula lleva el til DENTRO de su celda (la 'a' y la 'o' dejan
    // libres las filas 0 y 1). La mayuscula no cabe: va tres filas mas
    // arriba, que es seguro en un boton o un titulo y no en un parrafo.
    bool may = (c == (unsigned char)CH_ATILM || c == (unsigned char)CH_OTILM);
    bool esA = (c == (unsigned char)CH_ATIL || c == (unsigned char)CH_ATILM);
    int16_t x = gfx->getCursorX(), y = gfx->getCursorY();
    gfx->setCursor(x, (int16_t)(y - (may ? 3 * gTextSize : 0)));
    gfx->print('~');
    gfx->setCursor(x, y);
    gfx->print(may ? (esA ? 'A' : 'O') : (esA ? 'a' : 'o'));
    ini = p + 1;
  }
}

void printT(const char *s) {
  s = recortaAlPanel(s);
  if (gCjkFont) {
    // el japones y el coreano van en UTF-8, donde 0260/0261 son bytes de
    // continuacion de cualquier kanji: el camino del til NO es para ellos
    int16_t x = gfx->getCursorX(), y = gfx->getCursorY();
    gfx->print(s);
    gfx->setCursor(x + 1, y);
    gfx->print(s);
    return;
  }
  imprimeConTil(s);
}

// ---- los CARTELES (leva 71) ----
// "a msg de evolucao ... esta com a fonte estranha e quadro feio": la 5x7 a
// talla 3 es una calculadora. Los carteles grandes (evolucion, medalla,
// insignia) usan la letra de los juegos de DS, "power clear bold" del Anil
// (font_titulo.h, tools/gen_font_titulo.py), PROPORCIONAL y con sombra
// abajo-derecha como en los juegos. Se mide con tituloW(), nunca con
// textW(): textW() cuenta 6 px por letra y aqui cada una mide lo suyo.
// En japones y coreano no hay power clear: va la fuente del idioma.
#include "font_titulo.h"
#define TITULO_CAP 9   // alto de una mayuscula a talla 1 (para centrar)

uint16_t tituloW(const char *s, uint8_t n) {
  if (gCjkFont) {
    // textW() no CJK mede com a talla que estiver ACTIVA e ignora `n`:
    // poe-se a talla pedida para medir e volta-se a que estava.
    uint8_t antes = gTextSize;
    setSize(n);
    uint16_t w = textW(s, n);
    gfx->setTextSize(antes);
    gTextSize = antes;
    return w;
  }
  uint16_t w = 0;
  for (const unsigned char *p = (const unsigned char *)s; *p; p++)
    if (*p >= FONT_TITULO.first && *p <= FONT_TITULO.last)
      w += FONT_TITULO_GL[*p - FONT_TITULO.first].xAdvance;
  return (uint16_t)(w * n);
}

// La talla mas grande (de `n` hacia abajo) en la que `s` cabe en `ancho`.
uint8_t tituloTalla(const char *s, uint8_t n, int ancho) {
  while (n > 1 && tituloW(s, n) > ancho) n--;
  return n;
}

// `y` es el BORDE DE ARRIBA de las mayusculas, como en setCur().
void printTitulo(const char *s, int x, int y, uint8_t n, uint16_t cor,
                 uint16_t sombra) {
  if (gCjkFont) {
    setSize(n);
    gfx->setTextColor(sombra);
    setCur(x + 1, y + 1);
    printT(s);
    gfx->setTextColor(cor);
    setCur(x, y);
    printT(s);
    return;
  }
  uint8_t antes = gTextSize;
  gfx->setFont(&FONT_TITULO);
  gfx->setTextSize(n);
  int base = y + TITULO_CAP * n;
  gfx->setTextColor(sombra);
  static const int8_t SOM[3][2] = { { 1, 0 }, { 0, 1 }, { 1, 1 } };
  for (auto &d : SOM) {
    gfx->setCursor(x + d[0] * n, base + d[1] * n);
    gfx->print(s);
  }
  gfx->setTextColor(cor);
  gfx->setCursor(x, base);
  gfx->print(s);
  applyLangFont();
  setSize(antes);
}

void printTituloC(const char *s, int y, uint8_t n, uint16_t cor, uint16_t sombra) {
  printTitulo(s, CX - tituloW(s, n) / 2, y, n, cor, sombra);
}

static uint16_t mezcla565(uint16_t a, uint16_t b, uint8_t t) {  // t/255 de b
  int ra = a >> 11, ga = (a >> 5) & 63, ba = a & 31;
  int rb = b >> 11, gb = (b >> 5) & 63, bb = b & 31;
  return (uint16_t)(((ra + (rb - ra) * t / 255) << 11) |
                    ((ga + (gb - ga) * t / 255) << 5) | (ba + (bb - ba) * t / 255));
}

// Boton de cartel, como los del combate de los juegos: borde oscuro, la
// mitad de arriba mas clara (brillo) y el texto blanco con sombra.
void drawBotaoCartel(int x, int y, int w, int h, uint16_t cor, const char *txt) {
  gfx->fillRoundRect(x, y, w, h, 14, INK_K);
  gfx->fillRoundRect(x + 3, y + 3, w - 6, h - 6, 11, mezcla565(cor, 0, 70));
  gfx->fillRoundRect(x + 3, y + 3, w - 6, h - 9, 11, cor);
  gfx->fillRoundRect(x + 9, y + 6, w - 18, (h - 12) / 2 - 2, 8, mezcla565(cor, 0xFFFF, 70));
  uint8_t n = tituloTalla(txt, 2, w - 24);
  printTituloC(txt, y + (h - TITULO_CAP * n) / 2 - 1, n, UI_WHITE,
               mezcla565(cor, 0, 150));
}

// La pregunta de apagar. Es MODAL de verdad — lo primero de render() y
// con su propio flush — y no un adorno encima de la pantalla de casa como
// el "soltar?". Tiene que salir tambien en mitad de un combate o del
// gimnasio: el boton se puede aguantar en cualquier momento, y que no
// pasara nada visible seria peor que no tener la pregunta.
static void renderApagar() {
  gfx->fillScreen(RGB565_BLACK);
  gfx->fillCircle(CX, CY, 231, C565(0x10, 0x14, 0x1c));
  drawWindow(86, 150, 294, 190, 16);
  gfx->setTextColor(UI_INK);
  const char *t = T(S_APA_Q);
  setSize(2);
  setCur(centerX(t, 2), 172);
  printT(t);

  // VERDE el que no hace dano. Guardar congela el reloj del bicho: al
  // encender vuelve exactamente como se dejo.
  gfx->fillRoundRect(APA_BX, APA_B1Y, APA_BW, APA_BH, 12, UI_BAR_OK);
  gfx->setTextColor(UI_WHITE);
  const char *b1 = T(S_APA_PAUSA);
  setSize(2);
  setCur(APA_BX + (APA_BW - textW(b1, 2)) / 2, APA_B1Y + 9);
  printT(b1);
  const char *s1 = T(S_APA_PAUSA_SUB);
  setSize(1);
  setCur(APA_BX + (APA_BW - textW(s1, 1)) / 2, APA_B1Y + 36);
  printT(s1);

  // ROJO el de siempre: el RTC sigue, y al volver se cobra el rato entero.
  gfx->fillRoundRect(APA_BX, APA_B2Y, APA_BW, APA_BH, 12, UI_BAR_BAD);
  // "TURN OFF" a secas se leia como "esto es lo que quiero, apagar", y el
  // que lo pulsaba creia estar pausando. El boton dice lo que NO hace.
  const char *b2 = T(S_APA_DESL);
  setSize(2);
  setCur(APA_BX + (APA_BW - textW(b2, 2)) / 2, APA_B2Y + 9);
  printT(b2);
  const char *s2 = T(S_APA_DESL_SUB);
  setSize(1);
  setCur(APA_BX + (APA_BW - textW(s2, 1)) / 2, APA_B2Y + 36);
  printT(s2);

  gfx->setTextColor(UI_WHITE);
  const char *pie = T(S_APA_PIE);
  setSize(1);
  setCur(centerX(pie, 1), 356);
  printT(pie);
  gfx->flush();
}

void render() {
  if (apagarUntil) {            // aguantaste el PWR: guardar o apagar
    if (!timeLeft(apagarUntil)) {
      apagarUntil = 0;
    } else {
      renderApagar();
      return;
    }
  }
  if (introOpen) {              // primerisima partida: creditos y profesor
    renderIntro();
    return;
  }
  if (pet.awaitingStarter()) {  // primera partida: elegir inicial (prioridad total)
    renderStarterSelect();
    return;
  }
  if (trocaOpen && !(bt.open && bt.pvp)) {
    renderTroca();
    return;
  }
  if (bt.open) {
    cardSolo = false;   // saiu do ginasio para lutar: ao fim, casa
    renderBattle();
    gfx->flush();
    return;
  }
  // Fechou o ginasio/medalhas aberto pelo mapa: de volta ao mapa.
  if (cardSolo && !cardOpen) {
    cardSolo = false;
    if (!pet.ceremony && !avOpen) {
      mapView = pet.region;
      mapOpen = true;
    }
  }
  // Voltou com uma aventura em curso (apagou no meio): segue nela.
  static bool avVisto = false;
  if (!avVisto) {
    avVisto = true;
    if (pet.aventuraAtiva() && !avOpen) aventuraAbre();
  }
  if (avOpen) {                 // a aventura (aventura.ino)
    renderAventura();
    gfx->flush();
    return;
  }
  if (galleryOpen) {
    renderGallery();
    return;
  }
  if (gymOpen) {
    renderGym();
    return;
  }
  if (mapOpen) {
    renderMap();
    return;
  }
  if (rematchOpen) {
    renderRematch();
    return;
  }
  if (kbOpen) {
    renderKeyboard();
    return;
  }
  if (clockOpen) {
    renderClock();
    return;
  }
  // o "dar a quem" so vive na pagina do time
  if (!cardOpen || cardPage != 4 || bankView) darItemId = 0;
  if (cardOpen) {
    renderCard();
    return;
  }
  if (quizOpen) {
    renderQuiz();
    return;
  }
  int h = sceneHour();
  gNight = pet.sleeping || h < 6 || h >= 20;
  // drawScene cubre los 466x466 completos: sin fillScreen(NEGRO) previo para
  // que un flush DMA solapado nunca capture negro a medias (anti-parpadeo)
  drawScene(pet.isEgg() ? 0 : DEX_TBL[pet.speciesId].biome, millis(), gNight);

  if (pet.ceremony) {
    const DexEntry &d = DEX_TBL[pet.speciesId];
    const char *msg = (pet.ceremony == CER_FAREWELL) ? T(S_FAREWELL)
                      : (pet.ceremony == CER_RUNAWAY) ? T(S_RUNAWAY)
                                                      : T(S_GOODBYE);
    drawHeader(dexName(pet.speciesId), d.accent, msg, pet.shiny);
    drawCeremony();
    gfx->flush();
    return;
  }

  if (pet.isEgg()) {
    drawHeader(T(S_EGG_HDR), inkColor(), eggMsg(), false);
    // El huevo y sus grietas son los del Anil (art.h). El de antes era un
    // borron dibujado a mano en species.h que no parecia de Pokemon, y las
    // grietas eran dos punados de pixeles sueltos.
    //
    // Hay 5 fotogramas de grieta y solo 3 estados de huevo (eggTaps 0..2,
    // eclosiona al tercero), asi que se cogen separados â€” el 1 y el 3 â€” para
    // que el salto entre toque y toque se note. Comparten recorte con el
    // huevo, o sea que van en la MISMA posicion, sin desplazamientos.
    const int s = 3, baseY = PET_CY + (ART_EGG.h * s) / 2;
    drawArt(ART_EGG, CX, baseY, s);
    if (pet.eggCracks() >= 1)
      drawArt(*ART_CRACKS[pet.eggCracks() >= 2 ? 3 : 1], CX, baseY, s);
    if (pet.eggRarity() >= R_RARO) {
      // A 316 quedaba DEBAJO de la banda de abajo, que se pinta despues: el
      // aviso de huevo raro no se ha visto nunca. Ahora va justo encima.
      const char *rar = (pet.eggRarity() == R_LEGENDARIO) ? T(S_EGG_LEGEND) : T(S_EGG_RARE);
      printOutlined(centerX(rar, 2), 282, rar, 2,
                    pet.eggRarity() == R_LEGENDARIO ? UI_BAR_WARN : 0x4C98);
    }
    char reg[24];
    snprintf(reg, sizeof(reg), T(S_POKEDEX_FMT), pet.registeredCount(),
             (unsigned)DEX_COUNT);
    drawHomePanel();
    gfx->setTextColor(panelInk());
    setSize(2);
    setCur(centerX(reg, 2), 348);
    printT(reg);
  } else {
    const DexEntry &d = DEX_TBL[pet.speciesId];
    char name[DEX_HEAD_BUF];
    const char *base = pet.nick[0] ? pet.nick : dexName(pet.speciesId);
    snprintf(name, sizeof(name), T(S_NAME_FMT), "", base, pet.level());
    drawHeader(name, gNight ? UI_INK_NIGHT : d.accent, statusMsg(), pet.shiny);
    // La chapa de racha ya NO va aqui. Primero porque se pintaba fuera
    // del cristal redondo (26,16 cae 49 px fuera: no se ha visto nunca), y
    // despues porque el dueno zanjo que sobra —— el contador de dias
    // seguidos ya esta en la ficha, y repetirlo en casa solo quita sitio.
    drawPet();
    drawBath();
    drawPoops();
    // Durante la evolucion NO hay panel: el telon ocupa la pantalla entera,
    // como en los juegos. Con las barras y los botones encima, la ceremonia
    // parecia un efecto pegado sobre la pantalla de siempre.
    if (!pet.evolving()) {
      drawHomePanel();
      drawBars();
      drawButtons();
    }
    drawCelebration();
    if (pet.wantEvolveButton()) drawEvolveButton();        // CTA rojo: evolucionar
    else if (pet.canRunawayNow()) drawRunawayButton();     // CTA sombrio: escapada (abandono)
    else if (pet.wantFarewellButton()) drawFarewellButton();  // CTA dorado: despedida
    else drawTeamEvolveButton();                           // CTA verde: uno del equipo
  }

  if (pet.sleeping) {
    gfx->setTextColor(UI_INK_NIGHT);
    setSize(3);
    setCur(320, 130);
    printT("Zz");
  }

  if (pet.wildWaiting()) drawWildMark();
  // Con el huevo no hay equipo que curar ni sitio al que viajar, y durante
  // la evolucion no se pinta nada encima del telon (misma regla que las
  // barras y los botones).
  if (!pet.isEgg() && !pet.evolving() && pet.pcWaiting(millis())) drawPcMark();
  if (!pet.isEgg() && !pet.evolving() && pet.quizWaiting(millis())) drawOakMark();

  // El acuse de la curacion. Hace falta decirlo: los PS del banquillo no se
  // ven en ninguna barra de la pantalla de casa, asi que sin cartel tocar a
  // la enfermera no tendria NINGUN efecto visible y pareceria roto.
  if (healUntil) {
    if (!timeLeft(healUntil)) {
      healUntil = 0;
    } else {
      drawWindow(73, 168, 320, 90, 16);
      gfx->setTextColor(UI_BAR_OK);
      setSize(3);
      const char *l1 = T(S_CURADOS);
      setCur(centerX(l1, 3), 190);
      printT(l1);
      char l2[32];
      snprintf(l2, sizeof(l2), T(S_CURADOS_FMT), (unsigned)healCount);
      gfx->setTextColor(UI_INK);
      setSize(2);
      setCur(centerX(l2, 2), 224);
      printT(l2);
    }
  }

  // selector de comida
  if (feedMenuUntil) {
    if (!timeLeft(feedMenuUntil)) {
      feedMenuUntil = 0;
    } else {
      // Las cuatro comidas, en la caja de HGSS y con los sprites del juego:
      // eran cuatro borrones de 16x16 dibujados a mano y parecian gominolas.
      drawWindow(101, 282, 264, 70, 14);
      static const Art *const COMIDA[4] = {
        &ART_FEED_RED, &ART_UI_FOOD, &ART_FEED_GREEN, &ART_FEED_CANDY
      };
      for (int i = 0; i < 4; i++) {
        const Art &a = *COMIDA[i];
        drawArt(a, 134 + i * 66, 317 + a.h / 2, 1);
      }
    }
  }

  // dialogo "soltar?" (pulsacion larga sobre el bicho)
  if (confirmUntil) {
    if (!timeLeft(confirmUntil)) {
      confirmUntil = 0;
    } else {
      gfx->fillRoundRect(94, 168, 278, 152, 16, UI_WHITE);
      gfx->drawRoundRect(94, 168, 278, 152, 16, UI_INK);
      // 48 y no 28: el buffer se dimensiono cuando toda cadena era de un byte
      // por caracter. Con una fila UTF-8, "%s ë†“ì•„ì¤„ê¹Œìš”?" mas el nombre pasa
      // de 28 y snprintf corta a mitad de una secuencia de 3 bytes, que la
      // fuente ya no sabe dibujar.
      char q[48];
      snprintf(q, sizeof(q), T(S_RELEASE_FMT), dexName(pet.speciesId));
      gfx->setTextColor(UI_INK);
      setSize(2);
      setCur(centerX(q, 2), 196);
      printT(q);
      gfx->fillRoundRect(118, 252, 100, 52, 12, UI_BAR_OK);
      gfx->setTextColor(UI_WHITE);
      setCur(118 + (100 - textW(T(S_YES), 2)) / 2, 270);
      printT(T(S_YES));
      gfx->fillRoundRect(248, 252, 100, 52, 12, UI_BAR_BAD);
      setCur(248 + (100 - textW(T(S_NO), 2)) / 2, 270);
      printT(T(S_NO));
    }
  }

  // dialogo de decision (evolucionar/mantener, despedirse/quedaros)
  if (choiceKind) {
    if (!timeLeft(choiceUntil)) choiceKind = 0;
    else drawChoiceDialog();
  }

  gfx->flush();
}

// ---------- gimnasio: el saco de tipos ----------
//
// Como se juega: cuelga un saco con UN tipo pintado encima y abajo hay tres
// tipos de golpe. Uno de los tres es muy eficaz contra el saco, otro es
// neutro y el tercero le resbala. Aciertas el bueno y el saco encaja de
// verdad; fallas y pierdes tiempo. Al acabar, la puntuacion se convierte en
// EVs del stat que elegiste al entrar.
//
// Por que asi: los dos minijuegos de antes (mantener la pelota en el aire y
// aporrear un saco) eran de reflejos y no tenian nada de Pokemon. Este usa
// la MISMA TYPE_CHART que el combate, o sea que la habilidad que entrena el
// jugador es exactamente la que le hace falta para pelear. Y como el premio
// es el EV que el elija, entrenar pasa a ser una decision con un presupuesto
// (EV_TOTAL_CAP) en vez de una barra que solo sube.

// reparte los tres botones sobre la cuerda inferior del circulo
#define GYM_OPT_Y 352
#define GYM_OPT_H 52
#define GYM_OPT_W 96
#define GYM_OPT_GAP 8
#define GYM_OPT_X0 (CX - (3 * GYM_OPT_W + 2 * GYM_OPT_GAP) / 2)
// rejilla del selector de stat: 2 columnas x 3 filas
#define GYM_PICK_W 146
#define GYM_PICK_H 62
#define GYM_PICK_GAP 10
#define GYM_PICK_X0 (CX - (2 * GYM_PICK_W + GYM_PICK_GAP) / 2)
// 116 y no 126: con 126 la ultima fila (SPD y SPE) acababa en y=332 y el
// texto del presupuesto empieza en 330. Se pisaban por 2 px, y el texto
// va despues, asi que se veia escrito sobre los cuadros. Con 116 la fila
// acaba en 322 y quedan 8 px de aire. El titulo de arriba acaba en 108
// aun creciendo a talla 3, asi que por arriba tampoco aprieta.
#define GYM_PICK_Y0 116

// Elige saco y opciones. Garantiza que SIEMPRE haya exactamente una respuesta
// muy eficaz: si el reparto fuera al azar puro habria rondas sin buena
// jugada, y el jugador no sabria si fallo el o el juego.
void gymNewBag() {
  uint8_t fuertes[T_COUNT], flojos[T_COUNT], neutros[T_COUNT];
  uint8_t nf = 0, nl = 0, nn = 0;
  gymBagType = (uint8_t)(1 + random(T_COUNT - 1));
  for (uint8_t t = 1; t < T_COUNT; t++) {
    uint8_t e = TYPE_CHART[t][gymBagType];
    if (e > 100) fuertes[nf++] = t;
    else if (e < 100) flojos[nl++] = t;
    else neutros[nn++] = t;
  }
  // En esta tabla los 18 tipos tienen al menos una debilidad y al menos algo
  // que les resbala, pero si algun dia se retoca (tools/type_chart.py) el
  // reparto no debe dejar un hueco sin rellenar.
  gymOpts[0] = nf ? fuertes[random(nf)] : (uint8_t)(1 + random(T_COUNT - 1));
  gymOpts[1] = nn ? neutros[random(nn)] : (uint8_t)(1 + random(T_COUNT - 1));
  gymOpts[2] = nl ? flojos[random(nl)] : (uint8_t)(1 + random(T_COUNT - 1));
  for (int i = 2; i > 0; i--) {  // barajar: si no, el bueno siempre seria el primero
    int j = random(i + 1);
    uint8_t tmp = gymOpts[i]; gymOpts[i] = gymOpts[j]; gymOpts[j] = tmp;
  }
}

void startGym() {
  // La energia es el portero: entrenar cansa, y sin dormir no se entrena.
  // Es lo que ata el entrenamiento al ciclo de cuidado.
  if (!pet.canTrain()) { sfxPlay(SFX_DENY); return; }
  gymOpen = true;
  gymPhase = GYM_PICK;
  gymScore = 0;
  gymGain = 0;
  gymChain = 0;
  gymVerdict = -1;
  gymFlash = 0;
  gymShake = 0;
  gymNewHi = false;
  gymDoneUntil = 0;
}

void gymStartRound(uint8_t which) {
  gymStat = which;
  gymPhase = GYM_ROUND;
  gymUntil = millis() + GYM_MS;
  gymScore = 0;
  gymChain = 0;
  gymVerdict = -1;
  gymFlash = 0;
  gymNewHi = false;
  gymNewBag();
}

void gymFinishRound() {
  gymNewHi = (gymScore > pet.trainHi);
  gymGain = pet.trainEV(gymStat, gymScore);
  sfxPlay(gymNewHi && gymScore > 0 ? SFX_MEDAL : SFX_LEVEL);
  gymPhase = GYM_DONE;
  gymDoneUntil = millis() + 4000;
}

void gymTap(int16_t x, int16_t y) {
  if (y < 72) { gymOpen = false; return; }  // tocar arriba = salir

  if (gymPhase == GYM_DONE) {               // tocar = saltarse el resultado
    gymOpen = false;
    return;
  }

  if (gymPhase == GYM_PICK) {
    for (uint8_t i = 0; i < EV_N; i++) {
      int bx = GYM_PICK_X0 + (i % 2) * (GYM_PICK_W + GYM_PICK_GAP);
      int by = GYM_PICK_Y0 + (i / 2) * (GYM_PICK_H + GYM_PICK_GAP);
      if (x < bx || x > bx + GYM_PICK_W || y < by || y > by + GYM_PICK_H) continue;
      // Un stat al tope no se puede elegir: gastar una sesion entera (y la
      // energia) para recibir un +0 es una tomadura de pelo.
      if (pet.evOf(i) >= EV_STAT_CAP || pet.evTotal() >= EV_TOTAL_CAP) {
        sfxPlay(SFX_DENY);
        return;
      }
      sfxPlay(SFX_TAP);
      gymStartRound(i);
      return;
    }
    return;
  }

  // ronda activa: solo cuentan los tres botones de tipo
  if (!timeLeft(gymUntil)) return;
  for (uint8_t i = 0; i < 3; i++) {
    int bx = GYM_OPT_X0 + i * (GYM_OPT_W + GYM_OPT_GAP);
    if (x < bx || x > bx + GYM_OPT_W ||
        y < GYM_OPT_Y || y > GYM_OPT_Y + GYM_OPT_H) continue;
    uint16_t e = TYPE_CHART[gymOpts[i]][gymBagType];
    if (e > 100) {
      gymChain++;
      // la racha premia encadenar aciertos, que es lo que separa una sesion
      // buena de una a lo loco
      gymScore = (uint16_t)(gymScore + 3 + (gymChain >= 3 ? 1 : 0));
      gymVerdict = 2;
      gymShake = 20;
      sfxPlay(SFX_MEDAL);
    } else if (e == 100) {
      gymChain = 0;
      gymScore++;
      gymVerdict = 1;
      gymShake = 9;
      sfxPlay(SFX_PLAY);
    } else {
      gymChain = 0;
      gymVerdict = 0;
      gymShake = 3;
      sfxPlay(SFX_DENY);
      // castigo en TIEMPO y no en puntos: asi machacar los tres botones a lo
      // bruto sale peor que pensarlo, sin llegar a dejar la puntuacion en
      // negativo ni bloquear al que se equivoca
      uint32_t queda = timeLeft(gymUntil) ? gymUntil - millis() : 0;
      gymUntil = millis() + (queda > 900 ? queda - 900 : 0);
    }
    gymFlash = millis();
    gymNewBag();
    return;
  }
}

// ---------- caja de dialogo ----------
//
// El marco de HGSS, capa a capa: borde oscuro, filo dorado (mas oscuro por
// abajo, que es lo que le da volumen), sombra gris y el papel blanco dentro.
// Los colores estan arriba, con la banda de casa, porque los comparten.
//
// Va con rectangulos y no como bitmap porque son cuatro colores planos: asi
// no cuesta un byte de flash, sirve para cualquier tamano y no hay que
// tramar las esquinas de un 9-slice.
#define WIN_EDGE 6   // lo que se come el marco por cada lado

static int winR(int r, int quita) { return r - quita > 1 ? r - quita : 1; }

static void drawWindow(int x, int y, int w, int h, int r) {
  gfx->fillRoundRect(x, y, w, h, r, WIN_DARK);
  gfx->fillRoundRect(x + 2, y + 2, w - 4, h - 4, winR(r, 2), WIN_GOLD);
  gfx->fillRect(x + 4, y + h - 4, w - 8, 2, WIN_GOLD2);
  gfx->fillRoundRect(x + 4, y + 4, w - 8, h - 8, winR(r, 4), WIN_SHADE);
  gfx->fillRoundRect(x + WIN_EDGE, y + WIN_EDGE, w - WIN_EDGE * 2,
                     h - WIN_EDGE * 2, winR(r, WIN_EDGE), WIN_PAPER);
}

// La marca de shiny: la estrella de cuatro puntas del Anil, centrada en
// (cx, cy). Sustituye al asterisco que iba delante del nombre, que se leia
// como una llamada a pie de pagina y no como "este bicho es especial".
// `a` es ART_SHINY (20x28, junto a un nombre grande) o ART_SHINY_S (9x11,
// para la chapa de PS del combate, donde la grande cruzaba la barra).
static void drawShinyStar(const Art &a, int cx, int cy) {
  int b = cy + a.h / 2;
  static const int8_t D[4][2] = { { -1, 0 }, { 1, 0 }, { 0, -1 }, { 0, 1 } };
  for (auto &d : D) drawArtSolid(a, cx + d[0], b + d[1], 1, INK_K);
  drawArt(a, cx, b, 1);
}

// Insignia cuadrada del tipo: el color de su chapa en el Anil con el simbolo
// blanco encima. Se usa donde no cabe el rotulo (las filas de golpes), y es
// autonoma: el simbolo es blanco, asi que SIEMPRE necesita su color detras.
#define TYPE_BADGE 24

static void drawTypeBadge(int cx, int cy, uint8_t tipo) {
  const Art *ic = typeArt(tipo);
  uint16_t bg = typeArtBg(tipo);
  int x = cx - TYPE_BADGE / 2, y = cy - TYPE_BADGE / 2;
  gfx->fillRoundRect(x, y, TYPE_BADGE, TYPE_BADGE, 6,
                     (ic && bg) ? bg : typeColor(tipo));
  if (ic && bg) drawArt(*ic, cx, cy + ic->h / 2, 1);
}

// Chip de tipo: la chapa del Anil rehecha con nuestro rotulo. El color y el
// simbolo salen de su hoja; el texto lo ponemos nosotros porque el de la hoja
// esta en espanol ("SEA" por agua). Va en blanco con sombra, como el original:
// el simbolo tambien es blanco y los dos tienen que leerse sobre el mismo
// color.
//
// Segun lo que quepa: apilado (simbolo arriba, nombre debajo), en linea
// (simbolo a la izquierda) o solo el nombre.
// Con que color sale el fondo de la chapa de un tipo. Vive aparte y NO
// es static porque es lo unico que deja afirmar "la chapa se dibujo" en
// un test: se cuentan los pixeles de ese color dentro del rectangulo
// donde tendria que estar. Sacarlo de aqui y copiarlo en el test seria
// tener dos verdades, y la del test siempre acaba quedandose vieja.
uint16_t colorDeChapaTipo(uint8_t tipo) {
  const Art *ic = typeArt(tipo);
  uint16_t bg = typeArtBg(tipo);
  return (ic && bg) ? bg : typeColor(tipo);
}

static void drawTypeChip(int x, int y, int w, int h, uint8_t tipo, uint8_t talla) {
  const Art *ic = typeArt(tipo);
  uint16_t bg = colorDeChapaTipo(tipo);
  if (!typeArtBg(tipo)) ic = nullptr;
  gfx->fillRoundRect(x, y, w, h, 10, bg);
  gfx->drawRoundRect(x, y, w, h, 10, UI_INK);
  const char *n = typeName(tipo);
  int tw = textW(n, talla), th = 8 * talla;
  int tx = x + (w - tw) / 2, ty = y + (h - th) / 2;

  if (ic && h >= ic->h + th + 6 && w >= ic->w + 4) {
    int hueco = (h - ic->h - th - 2) / 2;
    drawArt(*ic, x + w / 2, y + hueco + ic->h, 1);
    ty = y + hueco + ic->h + 2;
  } else if (ic && h >= ic->h + 2 && w >= ic->w + tw + 10) {
    drawArt(*ic, x + 5 + ic->w / 2, y + (h + ic->h) / 2, 1);
    tx = x + 5 + ic->w + (w - 5 - ic->w - tw) / 2;
  }

  setSize(talla);
  gfx->setTextColor(ic ? INK_K : UI_INK);
  setCur(tx + (ic ? 1 : 0), ty + (ic ? 1 : 0));
  printT(n);
  if (!ic) return;
  gfx->setTextColor(UI_WHITE);
  setCur(tx, ty);
  printT(n);
}

// LOS TIPOS DEL BICHO. Lo dijo el dueno: "nenhum lugar mostra o tipo do
// pokemon que temos, nem na pokedex e nem no menu". Y era verdad: la
// chapa estaba hecha desde hacia tiempo —— con su icono y su color ——
// pero la unica pantalla que la usaba era la del gimnasio, para decir
// de que tipo era LA CASA. Del bicho propio no lo decia nadie, y es el
// dato del que depende todo lo demas.

// Los dos tipos de una especie, respetando la FORMA y la MEGA: un
// Meowth de Alola es siniestro y no normal, y el Mega Charizard X es
// dragon. Misma cascada que Combatant::type1/type2, que es la que vale
// en combate. `mega` = fila de MEGA_TBL, o -1 si no es una.
static void tiposDe(int16_t dex, uint8_t form, int16_t mega,
                    uint8_t *t1, uint8_t *t2) {
  *t1 = *t2 = T_NONE;
  if (mega >= 0 && mega < (int16_t)MEGA_COUNT) {
    *t1 = MEGA_TBL[mega].type1;
    *t2 = MEGA_TBL[mega].type2;
    return;
  }
  if (dex < 1 || dex > DEX_COUNT) return;
  int16_t f = formRow(dex, form);
  *t1 = f >= 0 ? FORM_TBL[f].type1 : DEX_TBL[dex].type1;
  *t2 = f >= 0 ? FORM_TBL[f].type2 : DEX_TBL[dex].type2;
}

// Una o dos chapas, en fila o en columna. El hueco reservado es SIEMPRE
// el de dos: con una sola se centra dentro de el, que si no un Pikachu
// deja media franja vacia y parece que algo no acabo de cargar.
static void drawTypesOf(int x, int y, int w, int h, int sep, bool apilado,
                        uint8_t t1, uint8_t t2, uint8_t talla) {
  uint8_t tipos[2], n = 0;
  if (t1 != T_NONE) tipos[n++] = t1;
  if (t2 != T_NONE && t2 != t1) tipos[n++] = t2;
  if (!n) return;
  int paso = (apilado ? h : w) + sep;
  int sobra = (2 - n) * paso / 2;
  for (uint8_t i = 0; i < n; i++)
    drawTypeChip(apilado ? x : x + sobra + i * paso,
                 apilado ? y + sobra + i * paso : y,
                 w, h, tipos[i], talla);
}

// LO QUE LE HACE DANO, y lo que no le hace NADA. Es el dato que no se
// puede deducir mirando dos chapas de tipo: la tabla son 18x18, y con
// dos tipos encima hay que multiplicar.
//
// Cada chapa se explica SOLA ("FIRE x2"), y eso ademas la deja fuera
// del problema de traducir un rotulo a ocho idiomas: el multiplicador
// se lee igual en todos. Por eso no hay ni un "WEAK TO" ni un "IMMUNE".
//
// Orden: primero lo que mas duele (x4), luego el x2, y las inmunidades
// al final —— son menos y van aparte porque no son "duele menos" sino
// "no pasa nada", que es otra decision.
static void drawDebilidades(int y0, uint8_t t1, uint8_t t2) {
  uint8_t tipo[T_COUNT];
  const char *mul[T_COUNT];
  uint8_t n = 0;
  for (uint8_t paso = 0; paso < 3 && n < T_COUNT; paso++) {
    for (uint8_t a = 1; a < T_COUNT && n < T_COUNT; a++) {
      uint16_t e = typeEffect(a, t1, t2);
      bool entra = (paso == 0 && e >= 400) || (paso == 1 && e == 200) ||
                   (paso == 2 && e == 0);
      if (!entra) continue;
      tipo[n] = a;
      mul[n] = paso == 0 ? "x4" : paso == 1 ? "x2" : "x0";
      n++;
    }
  }
  if (!n) return;

  char txt[T_COUNT][20];
  int w[T_COUNT];
  for (uint8_t i = 0; i < n; i++) {
    snprintf(txt[i], sizeof(txt[i]), "%s %s", typeName(tipo[i]), mul[i]);
    w[i] = textW(txt[i], 1) + 12;
  }

  // Se empaqueta codicioso y cada fila se centra: con tres filas de 24
  // la ultima acaba en 368 y el "tap: back" vive en 396, asi que no se
  // tocan ni en el peor caso (Roca/Hielo son siete debilidades).
  const int MAXW = 330, H = 20, SEP = 5, FILAS = 3;
  uint8_t i = 0;
  int y = y0;
  for (int fila = 0; fila < FILAS && i < n; fila++) {
    int ancho = 0;
    uint8_t j = i;
    while (j < n && ancho + w[j] + (j > i ? SEP : 0) <= MAXW) {
      ancho += w[j] + (j > i ? SEP : 0);
      j++;
    }
    if (j == i) { j = (uint8_t)(i + 1); ancho = w[i]; }   // una no cabe: va sola
    int x = 233 - ancho / 2;
    for (uint8_t k = i; k < j; k++) {
      gfx->fillRoundRect(x, y, w[k], H, 6, typeColor(tipo[k]));
      gfx->drawRoundRect(x, y, w[k], H, 6, UI_INK);
      int tx = x + (w[k] - textW(txt[k], 1)) / 2, ty = y + 6;
      // Sombra oscura debajo y el texto en blanco encima: los colores de
      // tipo van del amarillo al morado y ningun color de letra solo se
      // lee sobre los dieciocho.
      setSize(1);
      gfx->setTextColor(INK_K);
      setCur(tx + 1, ty + 1);
      printT(txt[k]);
      gfx->setTextColor(UI_WHITE);
      setCur(tx, ty);
      printT(txt[k]);
      x += w[k] + SEP;
    }
    i = j;
    y += H + 4;
  }
}

void drawGameScene();  // prototipo (definida mas abajo)

// el bicho del jugador, plantado a la izquierda del saco
static void drawGymMon(int px, int baseY) {
  if (pmd.loaded) {
    drawPmdAct(PMD_IDLE, px, baseY, millis(), true, false, 3);
  } else if (mon.loaded) {
    int s = (mon.h * 2 > 130) ? 1 : 2;
    int w = mon.w * s, h = mon.h * s;
    uint16_t fm = mon.frameMs ? mon.frameMs : 100;
    uint16_t fi = (millis() / fm) % mon.frames;
    const uint8_t *fr = mon.data + (uint32_t)fi * mon.w * mon.h;
    int x0 = px - w / 2, y0 = baseY - h;
    for (int r = 0; r < mon.h; r++)
      for (int c = 0; c < mon.w; c++) {
        uint8_t idx = fr[r * mon.w + c];
        if (idx == 0xFF) continue;
        gfx->fillRect(x0 + c * s, y0 + r * s, s, s, mon.pal[idx]);
      }
  }
}

static void renderGymPick(uint16_t ink) {
  // Con borde y un punto mas grande. Iba a talla 2 y con printT pelado:
  // sobre el fondo del gimnasio la pregunta se perdia y habia que
  // adivinarla. Las otras pantallas de esta misma familia ya usan
  // printOutlined (ver renderGymRound); esta se habia quedado atras.
  printOutlined(centerX(T(S_GYM_PICK), 3), 80, T(S_GYM_PICK), 3, UI_WHITE);

  bool lleno = pet.evTotal() >= EV_TOTAL_CAP;
  for (uint8_t i = 0; i < EV_N; i++) {
    int bx = GYM_PICK_X0 + (i % 2) * (GYM_PICK_W + GYM_PICK_GAP);
    int by = GYM_PICK_Y0 + (i / 2) * (GYM_PICK_H + GYM_PICK_GAP);
    uint8_t v = pet.evOf(i);
    bool tope = lleno || v >= EV_STAT_CAP;
    gfx->fillRoundRect(bx, by, GYM_PICK_W, GYM_PICK_H, 12,
                       tope ? UI_TRACK : UI_WHITE);
    gfx->drawRoundRect(bx, by, GYM_PICK_W, GYM_PICK_H, 12, UI_INK);
    gfx->setTextColor(UI_INK);
    setSize(2);
    setCur(bx + 12, by + 10);
    printT(Pet::evName(i));
    char n[10];
    snprintf(n, sizeof(n), "%u", v);
    setCur(bx + GYM_PICK_W - 12 - textW(n, 2), by + 10);
    printT(n);
    // barrita del EV: se ve de un vistazo en que esta invertido el presupuesto
    int bw = GYM_PICK_W - 24;
    gfx->fillRoundRect(bx + 12, by + 36, bw, 12, 4, UI_TRACK);
    int fw = bw * v / EV_STAT_CAP;
    if (fw > 2)
      gfx->fillRoundRect(bx + 12, by + 36, fw, 12, 4,
                         tope ? UI_BAR_WARN : UI_BAR_OK);
  }

  // el presupuesto comun, que es lo que convierte esto en una eleccion
  char pres[24];
  snprintf(pres, sizeof(pres), T(S_GYM_BUDGET_FMT),
           (unsigned)pet.evTotal(), (unsigned)EV_TOTAL_CAP);
  printOutlined(centerX(pres, 2), 330, pres, 2,
                lleno ? UI_BAR_BAD : UI_WHITE);
  if (lleno) {
    // 354 y no mas abajo: el horizonte del fondo esta en 376 y el aviso se
    // quedaba escrito sobre la tierra, medio ilegible.
    printOutlined(centerX(T(S_GYM_FULL), 2), 354, T(S_GYM_FULL), 2,
                  UI_BAR_BAD);
  }
}

static void renderGymRound(uint32_t now, uint16_t ink) {
  if (!timeLeft(gymUntil)) { gymFinishRound(); return; }

  // Todo el texto vive ARRIBA del todo, en la franja que no pisa ni el saco
  // ni al bicho: la primera version ponia el veredicto a media altura y caia
  // justo encima del sprite, ilegible.
  printOutlined(centerX(Pet::evName(gymStat), 2), 22, Pet::evName(gymStat),
                2, UI_BAR_OK);
  char hdr[24];
  snprintf(hdr, sizeof(hdr), T(S_SCORE_FMT), (unsigned)gymScore);
  printOutlined(centerX(hdr, 3), 44, hdr, 3, UI_WHITE);

  // veredicto del ultimo golpe (o la pista mientras no haya ninguno)
  if (gymFlash && now - gymFlash < 700 && gymVerdict >= 0) {
    const char *msg = gymVerdict == 2 ? T(S_GYM_SUPER)
                    : gymVerdict == 1 ? T(S_GYM_OK) : T(S_GYM_BAD);
    // con contorno, como el nombre de casa: el fondo ya no es un color plano
    printOutlined(centerX(msg, 2), 80, msg, 2,
                  gymVerdict == 2 ? UI_BAR_OK
                  : gymVerdict == 1 ? UI_WHITE : UI_BAR_BAD);
  } else {
    printOutlined(centerX(T(S_GYM_HINT), 2), 80, T(S_GYM_HINT), 2, UI_WHITE);
  }

  // el saco, con su tipo pintado encima
  gymShake *= 0.82f;
  int off = (int)(gymShake * sinf(now * 0.05f));
  int sx = CX + 62 + off, top = 134;
  gfx->fillRect(sx - 3, 110, 6, top - 110, ink);           // cuerda
  gfx->fillRoundRect(sx - 40, top, 80, 126, 24, C565(0xb5, 0x3a, 0x3a));
  gfx->fillRoundRect(sx - 40, top, 80, 20, 16, C565(0x7e, 0x28, 0x28));
  gfx->drawRoundRect(sx - 40, top, 80, 126, 24, ink);
  // mas alto que antes para que quepa el simbolo encima del rotulo; el saco
  // mide 126, asi que de top+34 a top+90 sigue holgado
  drawTypeChip(sx - 36, top + 34, 72, 56, gymBagType, 1);

  drawGymMon(CX - 104, 306);

  // barra de tiempo, justo encima de los botones
  uint32_t left = gymUntil - now;
  int bw = 280, fw = (int)((uint32_t)bw * left / GYM_MS);
  gfx->fillRoundRect(CX - bw / 2, 322, bw, 14, 5, UI_TRACK);
  if (fw > 2)
    gfx->fillRoundRect(CX - bw / 2, 322, fw, 14, 5,
                       left < 4000 ? UI_BAR_BAD : UI_BAR_OK);

  for (uint8_t i = 0; i < 3; i++)
    drawTypeChip(GYM_OPT_X0 + i * (GYM_OPT_W + GYM_OPT_GAP), GYM_OPT_Y,
                 GYM_OPT_W, GYM_OPT_H, gymOpts[i], 1);
}

static void renderGymDone(uint16_t ink) {
  if (!timeLeft(gymDoneUntil)) { gymOpen = false; return; }
  char b[24];
  snprintf(b, sizeof(b), T(S_SCORE_FMT), (unsigned)gymScore);
  gfx->setTextColor(ink);
  setSize(4);
  setCur(centerX(b, 4), 140);
  printT(b);

  // Lo que ENTRO de verdad, no lo que daba la puntuacion: con el stat o el
  // presupuesto al tope cabe menos y cantar un +12 que no ha pasado seria
  // mentir al jugador.
  char g[28];
  snprintf(g, sizeof(g), T(S_GYM_GAIN_FMT), Pet::evName(gymStat), gymGain);
  gfx->setTextColor(gymGain ? UI_BAR_OK : UI_BAR_BAD);
  setSize(3);
  setCur(centerX(g, 3), 196);
  printT(g);

  // donde ha quedado ese EV y cuanto presupuesto va gastado: sin esto el
  // jugador sabe que ha ganado 4 puntos pero no de que le sirven.
  uint8_t v = pet.evOf(gymStat);
  int bw = 260, bx = CX - bw / 2;
  gfx->fillRoundRect(bx, 238, bw, 16, 5, UI_TRACK);
  int fw = bw * v / EV_STAT_CAP;
  if (fw > 2)
    gfx->fillRoundRect(bx, 238, fw, 16, 5,
                       v >= EV_STAT_CAP ? UI_BAR_WARN : UI_BAR_OK);
  char pres[24];
  snprintf(pres, sizeof(pres), T(S_GYM_BUDGET_FMT),
           (unsigned)pet.evTotal(), (unsigned)EV_TOTAL_CAP);
  gfx->setTextColor(ink);
  setSize(2);
  setCur(centerX(pres, 2), 264);
  printT(pres);
  if (!gymGain) {
    gfx->setTextColor(UI_BAR_BAD);
    setCur(centerX(T(S_GYM_FULL), 2), 292);
    printT(T(S_GYM_FULL));
    return;
  }

  // Las bolas que ha pagado la sesion. Van aqui y no en un aviso aparte
  // porque es media razon para volver: sin verlas, el jugador no ata el
  // entrenar con el poder capturar.
  if (pet.lastBallCount) {
    char b[36];
    snprintf(b, sizeof(b), "+%u %s", pet.lastBallCount,
             itemAt(pet.lastBallId).name);
    gfx->setTextColor(UI_BAR_BAD);
    setSize(2);
    setCur(centerX(b, 2), 292);
    printT(b);
  }

  setSize(2);
  if (gymNewHi && gymScore > 0) {
    gfx->setTextColor(UI_BAR_WARN);
    setCur(centerX(T(S_NEW_RECORD), 2), 316);
    printT(T(S_NEW_RECORD));
  } else {
    // 24 y no 12: en japones "ãã‚ã %u" son 10 bytes antes de la cifra y el
    // record se quedaba en su primer digito. Las tallas de los buffers de
    // texto las vigila TestBuffersDeTexto en test/test_tools.py.
    char r[24];
    snprintf(r, sizeof(r), T(S_RECORD_FMT), pet.trainHi);
    gfx->setTextColor(ink);
    setCur(centerX(r, 2), 316);
    printT(r);
  }
}

void renderGym() {
  // sin fillScreen(NEGRO): drawGameScene cubre los 466x466 completos. Si el
  // DMA del flush anterior aun lee el buffer, vera contenido valido (no negro
  // a medio pintar), que era el parpadeo a 25 fps.
  drawGameScene();
  bool night = sceneHour() < 6 || sceneHour() >= 20;
  uint16_t ink = night ? UI_INK_NIGHT : UI_INK;

  if (gymPhase == GYM_PICK)       renderGymPick(ink);
  else if (gymPhase == GYM_ROUND) renderGymRound(millis(), ink);
  else                            renderGymDone(ink);

  gfx->flush();
}

// fondo del minijuego: hatibat del bicho (cielo por hora + suelo del bioma)
void drawGameScene() {
  // Se entrena DONDE vives: se reaprovecha el paisaje que ya esta cargado
  // para la pantalla de casa. Cargar uno propio costaria desalojarlo y
  // releer 424 KB al entrar y otros tantos al salir, cada sesion.
  if (homeBg.loaded && homeBg.w == LCD_WIDTH && homeBg.h == LCD_HEIGHT) {
    gfx->draw16bitRGBBitmap(0, 0, homeBg.pix, homeBg.w, homeBg.h);
    return;
  }

  int hh = sceneHour();
  bool night = hh < 6 || hh >= 20;
  uint16_t top, bot;
  if (night)       { top = C565(0x0c, 0x12, 0x24); bot = C565(0x1e, 0x26, 0x46); }
  else if (hh < 8) { top = C565(0xd1, 0x6a, 0x86); bot = C565(0xf3, 0xb8, 0x7c); }
  else if (hh < 18){ top = C565(0x8f, 0xc8, 0xea); bot = C565(0xdc, 0xee, 0xe6); }
  else             { top = C565(0xc7, 0x5a, 0x4a); bot = C565(0xf0, 0xae, 0x64); }
  int hor = 376;
  for (int y = 0; y < hor; y += 8)
    gfx->fillRect(0, y, 466, 8, lerp565(top, bot, y, hor));
  if (night)
    for (auto &st : STARS) gfx->fillRect(st[0], st[1], 4, 4, UI_WHITE);
  uint8_t bio = pet.isEgg() ? 0 : DEX_TBL[pet.speciesId].biome;
  uint16_t soil = BIOME_SOIL[bio < 6 ? bio : 0];
  if (night) soil = lerp565(soil, C565(0x16, 0x1c, 0x30), 9, 16);
  gfx->fillRect(0, hor, 466, 466 - hor, soil);
}

// ---------- ficha del bicho (deslizar vertical) ----------

// x donde arrancan las barras de la ficha. Estaba fijo en 150, que daba de sobra
// para etiquetas latinas de 3 caracteres pero no para las japonesas, mas anchas:
// ã“ã†ã’ã se metia dentro de la barra. Se calcula a partir de la etiqueta mas
// larga para que las cuatro barras sigan alineadas en cualquier idioma, con el
// valor original como suelo (asi en los idiomas latinos no cambia nada).
// Requiere tener ya puesto el tamano de texto 2, porque textW() lo necesita.
static int statBarX() {
  // Tienen que estar TODAS las que se pintan con barra: si falta una, esa
  // etiqueta se mete dentro de su propia barra en los idiomas anchos.
  const StrId ids[] = { S_STAT_ATK, S_STAT_DEF, S_STAT_SPA, S_STAT_SPD,
                        S_STAT_SPE, S_STAT_WGT, S_VIN };
  int ancho = 0;
  for (StrId id : ids) {
    int w = textW(T(id), 2);
    if (w > ancho) ancho = w;
  }
  int x = 96 + ancho + 12;   // 12 px de aire entre etiqueta y barra
  return x < 150 ? 150 : x;
}

void drawCardStat(int y, const char *label, uint16_t val, uint16_t maxBar, uint16_t color) {
  // CARD_INK y no UI_INK: UI_INK es la tinta de la pantalla de CASA, hecha
  // para su panel claro. Aqui se pintaba sobre el fondo casi negro de la
  // ficha y daba 1,36 de contraste â€” las etiquetas y los numeros de los
  // stats estaban, literalmente, invisibles.
  gfx->setTextColor(CARD_INK);
  setSize(2);
  setCur(96, y);
  printT(label);
  char num[8];
  snprintf(num, sizeof(num), "%u", val);
  setCur(330, y);
  printT(num);
  int bx = statBarX();
  int bw = 310 - bx;   // la barra siempre acaba en 310, dejando aire hasta el numero
  int fw = (int)val * bw / maxBar;
  if (fw > bw) fw = bw;
  gfx->fillRoundRect(bx, y + 2, bw, 11, 3, UI_TRACK);
  if (fw > 2) gfx->fillRoundRect(bx, y + 2, fw, 11, 3, color);
}

// ---------- ajuste de hora en pantalla (deslizar abajo) ----------
// El usuario pone su hora LOCAL a ojo; el firmware la usa tal cual, asi que
// no hay que gestionar zona horaria. Preserva el dia (no rompe racha/edad).

void openClock() {
  uint32_t e = pet.lastSeenEpoch ? pet.lastSeenEpoch : rtcEpoch();
  clockH = (e / 3600) % 24;
  clockM = (e / 60) % 60;
  clockOpen = true;
}

void applyClock() {
  uint32_t base = pet.lastSeenEpoch ? pet.lastSeenEpoch : rtcEpoch();
  uint32_t e = (base / 86400) * 86400 + (uint32_t)clockH * 3600 + (uint32_t)clockM * 60;
  rtcSetEpoch(e);
  pet.setClock(e);
  clockOpen = false;
  resetStep = 0;   // salir de ajustes cancela un borrado a medias
}

void drawClockBtn(int x, int y, const char *l) {
  gfx->fillRoundRect(x, y, 58, 58, 12, UI_WHITE);
  gfx->drawRoundRect(x, y, 58, 58, 12, UI_INK);
  gfx->setTextColor(UI_INK);
  setSize(4);
  setCur(x + 17, y + 15);
  printT(l);
}

// pildoras de idioma centradas en y; rellena la activa
#define LANG_PILL_Y 296
#define LANG_PILL_H 30
#define LANG_PILL_X 336          // pildora de idioma (cicla LANG_COUNT al tocar)
#define LANG_PILL_W 96
// borrar la partida: en el hueco que queda entre sonido (34..130) e idioma
#define RESET_PILL_X 152
#define RESET_PILL_W 162
static const char *const LANG_CODES[LANG_COUNT] = { "ES", "EN", "FR", "DE", "IT", "PT", "JA", "KO" };

void renderClock() {
  drawDexSkin();
  gfx->setTextColor(CARD_INK);
  setSize(3);
  setCur(centerX(T(S_SET_TIME), 3), 44);
  printT(T(S_SET_TIME));

  char t[8];
  snprintf(t, sizeof(t), "%02d:%02d", clockH, clockM);
  setSize(7);
  setCur(CX - 105, 108);
  printT(t);

  drawClockBtn(104, 190, "-");  // hora -
  drawClockBtn(170, 190, "+");  // hora +
  drawClockBtn(252, 190, "-");  // min -
  drawClockBtn(318, 190, "+");  // min +
  setSize(2);
  gfx->setTextColor(CARD_SOFT);
  setCur(120, 256);
  printT(T(S_HOUR));
  setCur(276, 256);
  printT(T(S_MIN));

  // Volumen: la misma pildora de antes, pero con CUATRO escalones en vez de
  // encendido/apagado. Toca y cicla 0 -> 1 -> 2 -> 3 -> 0; el 0 es el apagado
  // de siempre, asi que no se pierde nada y se gana el control que faltaba.
  uint8_t vol = audioVolume();
  gfx->fillRoundRect(34, LANG_PILL_Y, 96, LANG_PILL_H, 8,
                     vol ? UI_BAR_OK : CARD_PAPER);
  gfx->drawRoundRect(34, LANG_PILL_Y, 96, LANG_PILL_H, 8, CARD_INK);
  if (!vol) {
    const char *sl = T(S_SND_OFF);
    gfx->setTextColor(CARD_INK);
    setSize(2);
    setCur(34 + (96 - textW(sl, 2)) / 2, LANG_PILL_Y + 8);
    printT(sl);
  } else {
    // tres barritas que crecen: se lee de un vistazo, y no hay que traducir
    for (uint8_t i = 0; i < AUDIO_VOL_MAX; i++) {
      int h = 8 + i * 5, bx = 52 + i * 16, by = LANG_PILL_Y + LANG_PILL_H - 8 - h;
      gfx->fillRect(bx, by, 10, h, i < vol ? CARD_PAPER : C565(0x1d, 0x5c, 0x2a));
    }
  }

  // borrar la partida, en el hueco entre los dos interruptores. Roja y con
  // el texto en blanco: es lo unico de esta pantalla que no se deshace y
  // tiene que leerse distinta de un interruptor cualquiera.
  const char *rl = T(S_RESET);
  gfx->fillRoundRect(RESET_PILL_X, LANG_PILL_Y, RESET_PILL_W, LANG_PILL_H, 8, UI_BAR_BAD);
  gfx->drawRoundRect(RESET_PILL_X, LANG_PILL_Y, RESET_PILL_W, LANG_PILL_H, 8, CARD_INK);
  gfx->setTextColor(CARD_PAPER);
  setSize(1);
  setCur(RESET_PILL_X + (RESET_PILL_W - textW(rl, 1)) / 2, LANG_PILL_Y + 12);
  printT(rl);

  // selector de idioma: una pildora que cicla los 6 idiomas al tocar
  gfx->fillRoundRect(LANG_PILL_X, LANG_PILL_Y, LANG_PILL_W, LANG_PILL_H, 8, CARD_PAPER);
  gfx->drawRoundRect(LANG_PILL_X, LANG_PILL_Y, LANG_PILL_W, LANG_PILL_H, 8, CARD_INK);
  char lp[10];
  snprintf(lp, sizeof(lp), "%s >", LANG_CODES[gLang]);
  gfx->setTextColor(CARD_INK);
  setSize(2);
  setCur(LANG_PILL_X + (LANG_PILL_W - textW(lp, 2)) / 2, LANG_PILL_Y + 8);
  printT(lp);

  gfx->fillRoundRect(133, 340, 200, 48, 14, UI_BAR_OK);
  gfx->setTextColor(CARD_PAPER);
  setSize(3);
  setCur(CX - 18, 352);
  printT("OK");

  gfx->setTextColor(CARD_SOFT);
  setSize(2);
  // abajo del todo el circulo se estrecha: en espanol esta frase se salia
  uint8_t cs = tallaQueCabe(T(S_CLOCK_CANCEL), 410, 2, 0);
  setSize(cs);
  setCur(centerX(T(S_CLOCK_CANCEL), cs), 410);
  printT(T(S_CLOCK_CANCEL));

  // version del firmware (discreta, abajo del todo)
  char ver[20];
  snprintf(ver, sizeof(ver), "TamaPoke v%s", FW_VERSION);
  setSize(1);
  setCur(centerX(ver, 1), 436);
  printT(ver);

  if (resetStep) drawResetDialog();
  gfx->flush();
}

// Las dos preguntas del borrado. Tapan la pantalla entera a proposito: si se
// vieran los ajustes por detras, seguir tocandolos parece posible y no lo es.
#define RESET_YES_X 66
#define RESET_NO_X 246
#define RESET_BTN_Y 270
#define RESET_BTN_W 154
#define RESET_BTN_H 60

void drawResetDialog() {
  gfx->fillCircle(CX, CY, 231, CARD_INK);
  gfx->fillRoundRect(46, 120, 374, 220, 18, CARD_PAPER);
  gfx->drawRoundRect(46, 120, 374, 220, 18, UI_BAR_BAD);

  gfx->setTextColor(UI_BAR_BAD);
  setSize(2);
  setCur(centerX(T(S_RESET), 2), 148);
  printT(T(S_RESET));

  gfx->setTextColor(CARD_INK);
  setSize(1);
  drawWrapped(T(resetStep == 1 ? S_RESET_ASK1 : S_RESET_ASK2), 196, 52, 14, 3, 1);

  // El SI cambia: la primera vez es un boton normal y la segunda es rojo y
  // esta a la derecha, donde la primera estaba el NO. Mover el dedo es la
  // parte del "estas seguro" que de verdad funciona.
  bool dos = (resetStep == 2);
  int sx = dos ? RESET_NO_X : RESET_YES_X;
  int nx = dos ? RESET_YES_X : RESET_NO_X;
  gfx->fillRoundRect(sx, RESET_BTN_Y, RESET_BTN_W, RESET_BTN_H, 14, UI_BAR_BAD);
  gfx->setTextColor(CARD_PAPER);
  setSize(3);
  setCur(sx + (RESET_BTN_W - textW(T(S_YES), 3)) / 2, RESET_BTN_Y + 20);
  printT(T(S_YES));
  gfx->fillRoundRect(nx, RESET_BTN_Y, RESET_BTN_W, RESET_BTN_H, 14, CARD_DIM);
  gfx->setTextColor(CARD_INK);
  setCur(nx + (RESET_BTN_W - textW(T(S_NO), 3)) / 2, RESET_BTN_Y + 20);
  printT(T(S_NO));
}

void clockTap(int16_t x, int16_t y) {
  if (resetStep) {
    if (y < RESET_BTN_Y || y > RESET_BTN_Y + RESET_BTN_H) return;
    bool dos = (resetStep == 2);
    int sx = dos ? RESET_NO_X : RESET_YES_X;
    if (x >= sx && x <= sx + RESET_BTN_W) {       // SI
      sfxPlay(SFX_TAP);
      if (!dos) { resetStep = 2; return; }
      pet.factoryReset();
      delay(80);
      ESP.restart();
      return;
    }
    int nx = dos ? RESET_YES_X : RESET_NO_X;
    if (x >= nx && x <= nx + RESET_BTN_W) resetStep = 0;   // NO
    return;
  }
  if (y >= 190 && y <= 248) {  // fila de botones +/-
    if (x >= 104 && x < 162) clockH = (clockH + 23) % 24;
    else if (x >= 170 && x < 228) clockH = (clockH + 1) % 24;
    else if (x >= 252 && x < 310) clockM = (clockM + 59) % 60;
    else if (x >= 318 && x < 376) clockM = (clockM + 1) % 60;
    return;
  }
  if (y >= LANG_PILL_Y && y <= LANG_PILL_Y + LANG_PILL_H) {
    if (x >= 34 && x < 130) {                  // volumen: cicla los escalones
      audioSetVolume((uint8_t)((audioVolume() + 1) % (AUDIO_VOL_MAX + 1)));
      if (audioVolume()) sfxPlay(SFX_TAP);     // suena a ese volumen: se oye
      return;
    }
    if (x >= RESET_PILL_X && x < RESET_PILL_X + RESET_PILL_W) {
      resetStep = 1;                           // abre la primera pregunta
      sfxPlay(SFX_DENY);
      return;
    }
    if (x >= LANG_PILL_X && x < LANG_PILL_X + LANG_PILL_W) {  // cicla idioma
      setLang((Lang)((gLang + 1) % LANG_COUNT));
      applyLangFont();  // la fuente cambia con el idioma
      sfxPlay(SFX_TAP);
      return;
    }
  }
  if (y >= 340 && y <= 388 && x >= 133 && x <= 333) { applyClock(); return; }
}

// El aviso de que hay alguien en la hierba. Es un "!" que bota, a la
// derecha del bicho y despejado de los botones de abajo. No dice QUIEN es a
// proposito: en los juegos tampoco lo sabes hasta que te metes.
//
// Se puede ignorar sin coste: a los WILD_TTL_MIN minutos se va solo. Eso es
// lo que deja que siga siendo un tamagotchi y no una lista de tareas.
void drawWildMark() {
  int bob = (int)(sinf(millis() * 0.006f) * 5.0f);
  int cy = WILD_MARK_Y + bob;
  // Si quien te espera es un ENTRENADOR, se le ve a el. Tiene que notarse
  // antes de tocar: pega mas fuerte, su bicho no se captura y a cambio paga
  // mejor, y decidir si vas o no es media gracia del aviso.
  if (pet.wildTrainer >= 0) {
    const Art &a = *ART_TRAINER[(uint8_t)pet.wildTrainer % 8];
    drawArt(a, WILD_MARK_X, cy + 40, 1);
    // El bocadillo del "!" del Anil (Animations/029-Emotion01), que es
    // el que sale cuando un entrenador te ve. Antes era el caracter "!"
    // de la fuente del sistema: se entendia, pero no parecia del juego.
    //
    // Los tres fotogramas CRECEN. Se recorren una vez y se queda en el
    // ultimo: en bucle cerrado latiria como un corazon, que es otra cosa.
    // El ciclo es mas largo que los frames justo para que haya pausa.
    uint32_t paso = (millis() / 150) % 14;
    const Art &e = *ART_EXCLAMA[paso < ART_EXCLAMA_N ? paso
                                                     : ART_EXCLAMA_N - 1];
    drawArt(e, WILD_MARK_X, cy - 26, 1);
    if (pet.wildTtl <= 1 && (millis() / 300) % 2 == 0) return;
    gfx->setTextColor(UI_INK);
    setSize(1);
    const char *n = trainerAt((uint8_t)pet.wildTrainer).name;
    setCur(WILD_MARK_X - textW(n, 1) / 2, cy + 46);
    printT(n);
    return;
  }
  gfx->fillCircle(WILD_MARK_X, cy, WILD_MARK_R, UI_WHITE);
  gfx->drawCircle(WILD_MARK_X, cy, WILD_MARK_R, UI_BAR_BAD);
  gfx->drawCircle(WILD_MARK_X, cy, WILD_MARK_R - 1, UI_BAR_BAD);
  // el rabito de bocadillo, apuntando al bicho
  gfx->fillTriangle(WILD_MARK_X - 24, cy + 10, WILD_MARK_X - 8, cy + 20,
                    WILD_MARK_X - 6, cy + 2, UI_WHITE);
  // Y dentro de la bolita, la HIERBA ALTA: el tile del tileset del Anil,
  // el mismo por el que se anda y donde salen los bichos. Se MECE: un
  // pixel a cada lado, despacio. En los juegos la hierba esta quieta,
  // pero aqui es un aviso y tiene que llamar al ojo desde lejos.
  int vaiven = (int)(sinf(millis() * 0.004f) * 1.6f);
  drawArt(ART_HIERBA, WILD_MARK_X + vaiven, cy + ART_HIERBA.h / 2 + 2, 1);
  // se apaga el ultimo minuto: avisa de que se esta yendo
  if (pet.wildTtl <= 1 && (millis() / 300) % 2 == 0)
    gfx->drawCircle(WILD_MARK_X, cy, WILD_MARK_R + 3, UI_TRACK);
}

// llama + numero de racha arriba a la izquierda
// LA ENFERMERA. Se asoma cada tantos minutos y SOLO si hay algo que curar
// (pet.pcTick decide); tocarla deja a todo el equipo —— titular, banquillo y
// caja —— con los PS y los PP llenos, con la fanfarria del centro Pokemon.
//
// Hacia falta porque el unico sitio donde se recuperaba algo era dormir, y
// dormir solo cura al TITULAR. Con cinco suplentes a cero y nada que los
// levante, el equipo deja de existir a la segunda derrota.
void drawPcMark() {
  int bob = (int)(sinf(millis() * 0.005f) * 4.0f);
  int cy = PC_MARK_Y + bob;
  gfx->fillCircle(PC_MARK_X, cy, PC_MARK_R, UI_WHITE);
  gfx->drawCircle(PC_MARK_X, cy, PC_MARK_R, UI_BAR_BAD);
  gfx->drawCircle(PC_MARK_X, cy, PC_MARK_R - 1, UI_BAR_BAD);
  // La cruz roja detras, que es lo que se lee de un vistazo aunque la
  // enfermera sea de 40 px.
  gfx->fillRect(PC_MARK_X - 16, cy - 5, 32, 10, 0xFCF3);
  gfx->fillRect(PC_MARK_X - 5, cy - 16, 10, 32, 0xFCF3);
  drawArt(ART_NURSE, PC_MARK_X, cy + 26, 1);
  // Y parpadea el ultimo cuarto de la visita: se va a ir.
  uint32_t ahora = millis();
  if (pet.pcGone && (int32_t)(pet.pcGone - ahora) < (int32_t)(PC_STAY_MS / 4) &&
      (ahora / 300) % 2 == 0)
    gfx->drawCircle(PC_MARK_X, cy, PC_MARK_R + 3, UI_BAR_BAD);
}

// LA MARCA DEL PROFESOR. Igual que la de la enfermera: se asoma, flota,
// parpadea cuando se va a ir y se toca. El dibujo sale del Anil
// (ART_OAKMINI, recorte de Characters/oak.png); no esta hecho con
// circulos, que ya se probo una vez y quedo feo.
void drawOakMark() {
  int bob = (int)(sinf(millis() * 0.005f + 1.5f) * 4.0f);
  int cy = OAK_MARK_Y + bob;
  gfx->fillCircle(OAK_MARK_X, cy, OAK_MARK_R, UI_WHITE);
  gfx->drawCircle(OAK_MARK_X, cy, OAK_MARK_R, UI_BAR_OK);
  gfx->drawCircle(OAK_MARK_X, cy, OAK_MARK_R - 1, UI_BAR_OK);
  drawArt(ART_OAKMINI, OAK_MARK_X, cy + 26, 1);
  gfx->setTextColor(UI_BAR_OK);
  setSize(2);
  setCur(OAK_MARK_X + OAK_MARK_R - 10, cy - OAK_MARK_R - 2);
  printT("?");
  uint32_t ahora = millis();
  if (pet.quizGone &&
      (int32_t)(pet.quizGone - ahora) < (int32_t)(QUIZ_STAY_MS / 4) &&
      (ahora / 300) % 2 == 0)
    gfx->drawCircle(OAK_MARK_X, cy, OAK_MARK_R + 3, UI_BAR_OK);
}

// El enunciado, en DOS lineas si hace falta.
//
// A una sola no caben: "What type is the move DOUBLE-EDGE?" son 36
// caracteres, y a tamano 2 eso pasa de los 466 px de la pantalla.
// Encogerlo a tamano 1 lo deja ilegible —— que es justo la queja que hubo
// con el cartel de la despedida ——, asi que se parte por el espacio mas
// cercano al medio y se mantiene el tamano.
static void drawWrapped(const char *s, int y, int maxW, uint8_t size) {
  if (textW(s, size) <= maxW) {
    setSize(size);
    setCur(centerX(s, size), y);
    printT(s);
    return;
  }
  int n = (int)strlen(s), corte = -1, mejor = n;
  for (int i = 0; i < n; i++) {
    if (s[i] != ' ') continue;
    int d = i > n / 2 ? i - n / 2 : n / 2 - i;
    if (d < mejor) { mejor = d; corte = i; }
  }
  if (corte < 0) {            // una sola palabra enorme: no hay donde partir
    setSize(size);
    setCur(centerX(s, size), y);
    printT(s);
    return;
  }
  char a[72], b[72];
  int na = corte < (int)sizeof(a) - 1 ? corte : (int)sizeof(a) - 1;
  memcpy(a, s, na);
  a[na] = 0;
  snprintf(b, sizeof(b), "%s", s + corte + 1);
  setSize(size);
  setCur(centerX(a, size), y);
  printT(a);
  setCur(centerX(b, size), y + 10 + 8 * size);
  printT(b);
}

static void quizPrompt(char *out, size_t n) {
  switch (quizQ.kind) {
    case QZ_EVOLVES:   snprintf(out, n, T(S_QUIZ_Q_EVO), quizQ.subject); break;
    case QZ_MOVE_TYPE: snprintf(out, n, T(S_QUIZ_Q_MOVE), quizQ.subject); break;
    case QZ_HEAVIER:   snprintf(out, n, "%s", T(S_QUIZ_Q_HEAVY)); break;
    case QZ_LEARNS:    snprintf(out, n, T(S_QUIZ_Q_LEARN), quizQ.subject); break;
    case QZ_WHO:       snprintf(out, n, "%s", T(S_QUIZ_Q_WHO)); break;
    default:           snprintf(out, n, T(S_QUIZ_Q_TYPE), quizQ.subject); break;
  }
}

#define QZ_BTN_X 83
#define QZ_BTN_W 300
#define QZ_BTN_H 54
#define QZ_BTN_Y0 186
#define QZ_BTN_GAP 62
// "Quem e esse Pokemon?": o dibujo vai entre o enunciado e os botoes, e
// os botoes descem. A resposta (CORRECT/WRONG) desce junto.
#define QZ_WHO_Y0 224
#define QZ_WHO_GAP 58
#define QZ_WHO_H 50
#define QZ_WHO_IMG_Y 166
static int qzBtnY(int i) {
  return quizQ.kind == QZ_WHO ? QZ_WHO_Y0 + i * QZ_WHO_GAP : QZ_BTN_Y0 + i * QZ_BTN_GAP;
}
static int qzBtnH() { return quizQ.kind == QZ_WHO ? QZ_WHO_H : QZ_BTN_H; }

void renderQuiz() {
  gfx->fillScreen(UI_BG_DAY);
  gfx->setTextColor(UI_INK);

  if (quizEnd) {
    // El retrato mide 64x151. Apoyado en 150 su cabeza quedaba en y=-1,
    // o sea cortada: "quando ganha tb o sprite dele fica meio fora". A 64
    // de ancho necesita empezar en y>=3 para caber en el circulo, asi que
    // se apoya en 158 y el texto de abajo se aprieta para dejarle sitio.
    drawArt(ART_OAK, CX, 158, 1);
    char m[40];
    snprintf(m, sizeof(m), T(S_QUIZ_SCORE_FMT), (unsigned)quizHits,
             (unsigned)QUIZ_LEN);
    setSize(3);
    setCur(centerX(m, 3), 186);
    printT(m);
    if (quizPrize) {
      char p[56];
      char nome[24];
      if (quizPrizeQtd > 1)
        snprintf(nome, sizeof(nome), "%s x%u", itemAt(quizPrize).name, (unsigned)quizPrizeQtd);
      else
        snprintf(nome, sizeof(nome), "%s", itemAt(quizPrize).name);
      snprintf(p, sizeof(p), T(S_PRIZE_FMT), nome);
      gfx->setTextColor(UI_BAR_OK);
      drawWrapped(p, 240, 360, 2);
    } else {
      gfx->setTextColor(UI_BAR_WARN);
      drawWrapped(T(S_QUIZ_NOPRIZE), 240, 360, 2);
    }
    gfx->setTextColor(UI_INK);
    setSize(2);
    setCur(centerX(T(S_BACK), 2), 330);
    printT(T(S_BACK));
    gfx->flush();
    return;
  }

  // En 92 se salia del cristal: el sprite mide 40x61, asi que su esquina
  // de arriba caia en y=31, donde el circulo empieza en x=117. Lo dijo el
  // dueno: "o oak pequeno em cima ficou fora do circulo".
  drawArt(ART_OAKMINI, 140, 96, 1);
  setSize(2);
  setCur(centerX(T(S_QUIZ_TITLE), 2) + 30, 58);
  printT(T(S_QUIZ_TITLE));

  // en que pregunta vamos: un punto por cada una
  int x0 = CX - (QUIZ_LEN * 14) / 2 + 7;
  for (int i = 0; i < QUIZ_LEN; i++) {
    uint16_t c = i < quizIdx ? UI_BAR_OK
                             : (i == quizIdx ? UI_BAR_WARN : UI_TRACK);
    gfx->fillCircle(x0 + i * 14, 84, 4, c);
  }

  char q[72];
  quizPrompt(q, sizeof(q));
  const bool quem = quizQ.kind == QZ_WHO;
  drawWrapped(q, quem ? 104 : 112, 380, 2);
  if (quem) {
    // O dibujo a 2x (80 px). Em silhueta, todo preto ate responder: depois
    // de tocar se revela a cor, que e metade da graca.
    const uint8_t *th = thumbs.get(quizQ.subjDex);
    if (th) drawThumbFit(th, CX, QZ_WHO_IMG_Y, 80, quizQ.silueta && quizPick < 0);
  }

  for (int i = 0; i < QUIZ_OPTS; i++) {
    int y = qzBtnY(i);
    const int bh = qzBtnH();
    uint16_t fondo = UI_TRACK, tinta = UI_INK;
    // Ya contestada: la BUENA en verde siempre, y la tocada en rojo si
    // estaba mal. Ensenar cual era es la mitad de la gracia; con un "fallo"
    // a secas no se aprende nada.
    if (quizPick >= 0) {
      if (i == quizQ.right) { fondo = UI_BAR_OK; tinta = UI_WHITE; }
      else if (i == quizPick) { fondo = UI_BAR_BAD; tinta = UI_WHITE; }
    }
    gfx->fillRoundRect(QZ_BTN_X, y, QZ_BTN_W, bh, 12, fondo);
    gfx->drawRoundRect(QZ_BTN_X, y, QZ_BTN_W, bh, 12, UI_INK);
    gfx->setTextColor(tinta);
    const char *o = quizQ.opt[i] ? quizQ.opt[i] : "";
    uint8_t ts = textW(o, 2) <= QZ_BTN_W - 16 ? 2 : 1;
    setSize(ts);
    setCur(QZ_BTN_X + (QZ_BTN_W - textW(o, ts)) / 2,
           y + (bh - 8 * ts) / 2);
    printT(o);
  }

  if (quizPick >= 0) {
    bool bien = quizPick == (int8_t)quizQ.right;
    const char *f = T(bien ? S_QUIZ_RIGHT : S_QUIZ_WRONG);
    gfx->setTextColor(bien ? UI_BAR_OK : UI_BAR_BAD);
    setSize(3);
    setCur(centerX(f, 3), quem ? 400 : 376);
    printT(f);
  }
  gfx->flush();
}

void quizNextQ() {
  quizPick = -1;
  if (quizIdx >= QUIZ_LEN) {
    // Se acabo: el premio sale del marcador. QUIZ_PRIZE vive en pet.h al
    // lado del de las medallas, para que las dos tablas se vean juntas.
    quizEnd = true;
    uint8_t qtd = 1;
    uint8_t it = quizHits >= QUIZ_LEN ? quizPremioCheio(&qtd)
                                      : QUIZ_PRIZE[quizHits];
    uint8_t dados = 0;
    for (uint8_t k = 0; it && k < qtd; k++)
      if (pet.takeItem(it)) dados++;
    quizPrize = dados ? it : 0;
    quizPrizeQtd = dados ? dados : 1;
    if (quizPrize) jinglePlay(JIN_BADGE);
    return;
  }
  if (quizIdx == QUIZ_LEN - 1 && quizHits == QUIZ_LEN - 1) {
    // Quatro de quatro: a ultima e a dificil, em silhueta, e paga melhor.
    for (int k = 0; k < 16; k++) {
      qzQuienEs(quizQ, true);
      qzBaraja(quizQ);
      if (thumbs.get(quizQ.subjDex)) break;
    }
  } else {
    for (int k = 0; k < 16; k++) {
      if (!quizSiguiente(quizQ, quizLastKind)) quizSiguiente(quizQ);
      // sem o dibujo (cartao sem thumbs.bin) o "quem e" nao tem pergunta
      if (quizQ.kind != QZ_WHO || thumbs.get(quizQ.subjDex)) break;
    }
    if (quizQ.kind == QZ_WHO && !thumbs.get(quizQ.subjDex)) quizUna(quizQ, QZ_MOVE_TYPE);
  }
  quizLastKind = quizQ.kind;
}

void quizStart() {
  quizOpen = true;
  quizEnd = false;
  quizIdx = 0;
  quizHits = 0;
  quizPrize = 0;
  quizPrizeQtd = 1;
  quizLastKind = 0xFF;
  quizNextQ();
  pet.quizLeave();     // la visita se consume al entrar
  sfxPlay(SFX_TAP);
}

void quizTap(int16_t x, int16_t y) {
  if (quizEnd) { quizOpen = false; return; }
  if (quizPick >= 0) {                 // ensenando el resultado
    if (millis() < quizUntil) return;  // un momento para leerlo
    quizIdx++;
    quizNextQ();
    return;
  }
  for (int i = 0; i < QUIZ_OPTS; i++) {
    int by = qzBtnY(i);
    if (x < QZ_BTN_X || x > QZ_BTN_X + QZ_BTN_W) continue;
    if (y < by || y > by + qzBtnH()) continue;
    quizPick = (int8_t)i;
    quizUntil = millis() + 700;
    if (i == (int)quizQ.right) { quizHits++; sfxPlay(SFX_MEDAL); }
    else sfxPlay(SFX_DENY);
    return;
  }
}

void drawStreakBadge() {
  if (pet.streak < 1) return;
  // Estaba en 26,16, que en un panel REDONDO de 466 cae 49 px fuera del
  // cristal: el numero de dias de racha no se ha visto nunca en la placa.
  // Aqui abajo ya hay banda de sobra y sigue arriba a la izquierda, que
  // es donde se buscaba.
  int x = 62, y = 104;
  gfx->fillTriangle(x + 8, y, x + 1, y + 17, x + 15, y + 17, UI_BAR_BAD);
  gfx->fillTriangle(x + 8, y + 7, x + 4, y + 17, x + 12, y + 17, UI_BAR_WARN);
  char s[6];
  snprintf(s, sizeof(s), "%u", pet.streak);
  gfx->setTextColor(inkColor());
  setSize(2);
  setCur(x + 22, y + 2);
  printT(s);
}

// banner temporal: medalla nueva o hito de racha
// EL MAPA DE REGIONES.
//
// Que hace: ensena el poster de una region y, si ya la tienes abierta, te
// deja plantarte en ella. Donde estes decide que saca la hierba —— lo de esa
// region sale cuatro veces mas (wildWeight) —— pero NO cierra la puerta a
// las demas: en Johto te siguen saliendo Pidgeys. Esa es la diferencia
// entre un mapa y una puerta.
//
// Se puede mirar una region MAS de las abiertas, con el candado puesto:
// que se vea que hay juego detras del campeon. Sin eso, el que acaba de
// empezar no tiene forma de saber que Kanto no es todo lo que hay.
void renderMap() {
  gfx->fillScreen(C565(0x14, 0x18, 0x24));
  bool abierta = pet.zoneUnlocked(mapView);
  const char *nom = regionAt(mapView).name;

  if (abierta && mapImg.load(mapView, "map") && mapImg.pix) {
    int w = mapImg.w, h = mapImg.h;
    int x0 = MAP_CX - w / 2, y0 = MAP_CY - h / 2;
    // El marco PRIMERO y por debajo: un poster sin marco sobre fondo
    // oscuro parece un trozo de pantalla sin pintar.
    gfx->fillRoundRect(x0 - 7, y0 - 7, w + 14, h + 14, 14, C565(0x6b, 0x53, 0x38));
    gfx->drawRoundRect(x0 - 7, y0 - 7, w + 14, h + 14, 14, C565(0x3a, 0x2c, 0x1d));
    gfx->draw16bitRGBBitmap(x0, y0, mapImg.pix, w, h);
  } else {
    // Sin fichero (tarjeta sin /map) o con candado. En los dos casos hay
    // que pintar ALGO: una pantalla negra con un nombre suelto se lee como
    // un cuelgue, y hay un test que no deja que ninguna se quede en blanco.
    int w = MAP_MAX_W, h = MAP_MAX_H;
    int x0 = MAP_CX - w / 2, y0 = MAP_CY - h / 2;
    gfx->fillRoundRect(x0, y0, w, h, 14, C565(0x23, 0x29, 0x3a));
    gfx->drawRoundRect(x0, y0, w, h, 14, C565(0x4a, 0x54, 0x70));
    if (!abierta) {
      // El candado, dibujado: cuerpo y arco.
      int cx = MAP_CX, cy = MAP_CY - 14;
      gfx->fillRoundRect(cx - 26, cy, 52, 42, 7, C565(0x8a, 0x93, 0xb0));
      for (int g = 0; g < 7; g++)
        gfx->drawCircle(cx, cy - 2, 18 - g / 3, C565(0x8a, 0x93, 0xb0));
      gfx->fillRect(cx - 20, cy - 2, 40, 22, C565(0x23, 0x29, 0x3a));
      gfx->fillRoundRect(cx - 26, cy, 52, 42, 7, C565(0x8a, 0x93, 0xb0));
      gfx->fillCircle(cx, cy + 18, 6, C565(0x23, 0x29, 0x3a));
      gfx->setTextColor(C565(0x8a, 0x93, 0xb0));
      setSize(2);
      setCur(centerX("? ? ?", 2), cy + 62);
      printT("? ? ?");
    } else {
      gfx->setTextColor(C565(0x8a, 0x93, 0xb0));
      setSize(2);
      const char *f = "map file missing";
      setCur(centerX(f, 2), MAP_CY - 8);
      printT(f);
    }
  }

  // El boton de REVANCHA: solo sobre la region en la que estas y solo
  // coronado en ella. Encima del poster porque es el unico hueco que
  // queda —— y solo sale cuando hay algo que revanchar, asi que no le
  // tapa el mapa a nadie que no lo haya pedido.
  if (mapView == pet.region && rematchReady()) {
    gfx->fillRoundRect(MAP_RE_X, MAP_RE_Y, MAP_RE_W, MAP_RE_H, 12, UI_BAR_BAD);
    gfx->drawRoundRect(MAP_RE_X, MAP_RE_Y, MAP_RE_W, MAP_RE_H, 12, UI_INK);
    gfx->setTextColor(UI_WHITE);
    setSize(2);
    setCur(MAP_RE_X + (MAP_RE_W - textW(T(S_REVANCHA), 2)) / 2, MAP_RE_Y + 12);
    printT(T(S_REVANCHA));
  }

  // el nombre, grande y fuera del poster
  gfx->setTextColor(abierta ? UI_WHITE : C565(0x7a, 0x82, 0x9c));
  setSize(3);
  setCur(centerX(nom, 3), MAP_NAME_Y);
  printT(nom);

  // las flechas: solo las que llevan a algun sitio
  uint8_t tope = (uint8_t)(pet.region + 1 < REGION_COUNT ? pet.region + 1
                                                         : pet.region);
  if (mapView > 0)
    gfx->fillTriangle(MAP_ARROW_LX - 16, MAP_ARROW_Y, MAP_ARROW_LX + 12,
                      MAP_ARROW_Y - 20, MAP_ARROW_LX + 12, MAP_ARROW_Y + 20,
                      UI_WHITE);
  if (mapView < tope)
    gfx->fillTriangle(MAP_ARROW_RX + 16, MAP_ARROW_Y, MAP_ARROW_RX - 12,
                      MAP_ARROW_Y - 20, MAP_ARROW_RX - 12, MAP_ARROW_Y + 20,
                      UI_WHITE);

  // y el pie, que es lo unico que explica que hacer
  const char *pie;
  uint16_t col;
  if (!abierta) {
    // Se dice QUE falta, con nombre y todo. "Bloqueada" no es informacion.
    static char buf[40];
    snprintf(buf, sizeof(buf), T(S_MAPA_VENCE_FMT), regionAt(pet.region).name);
    pie = buf;
    col = C565(0x7a, 0x82, 0x9c);
  } else if (mapView == pet.region) {
    // No lugar do "you are here": GINASIO e MEDALHAS (que sairam da ficha).
    const char *rot[2] = { T(S_GYMS), T(S_MAP_MEDALS) };
    const int bx[2] = { MAP_BT_LX, MAP_BT_RX };
    for (int k = 0; k < 2; k++) {
      gfx->fillRoundRect(bx[k], MAP_BT_Y, MAP_BT_W, MAP_BT_H, 12,
                         C565(0x2c, 0x4a, 0x6e));
      gfx->drawRoundRect(bx[k], MAP_BT_Y, MAP_BT_W, MAP_BT_H, 12, UI_INK);
      gfx->setTextColor(UI_WHITE);
      int sz = textW(rot[k], 2) <= MAP_BT_W - 12 ? 2 : 1;
      setSize(sz);
      setCur(bx[k] + (MAP_BT_W - textW(rot[k], sz)) / 2,
             MAP_BT_Y + (MAP_BT_H - 8 * sz) / 2);
      printT(rot[k]);
    }
    gfx->flush();
    return;
  } else {
    pie = T(S_MAPA_VIAJAR);
    col = UI_BAR_WARN;
  }
  gfx->setTextColor(col);
  // traduzido nao cabe a 2 em todo idioma ("toque no mapa para viajar")
  uint8_t pz = tallaQueCabe(pie, MAP_FOOT_Y, 2, 0);
  setSize(pz);
  setCur(centerX(pie, pz), MAP_FOOT_Y + (2 - pz) * 4);
  printT(pie);
  gfx->flush();
}

// Devuelve true si el toque era para el mapa. Cerrar va al final: tocar
// fuera de todo lo demas es la forma de salir, y asi no hace falta una X
// que en una pantalla redonda no tiene esquina donde ponerse.
bool mapTap(int16_t x, int16_t y) {
  uint8_t tope = (uint8_t)(pet.region + 1 < REGION_COUNT ? pet.region + 1
                                                         : pet.region);
  // GINASIO / MEDALHAS: so no mapa da regiao onde se esta. ANTES das
  // setas, com a faixa inteira dividida ao meio: nao ha vao entre eles.
  const bool botoes = mapView == pet.region && pet.zoneUnlocked(mapView);
  if (botoes && y >= MAP_BT_TOQ_Y0 && y <= MAP_BT_TOQ_Y1 &&
      x >= MAP_BT_TOQ_X0 && x <= MAP_BT_TOQ_X1) {
    mapImg.unload();
    mapOpen = false;
    cardOpen = true;
    cardSolo = true;
    cardPage = (uint8_t)(x < MAP_CX ? 5 : 7);
    cardScroll = 0;
    movesSlot = MOVES_MAIN;
    sfxPlay(SFX_TAP);
    return true;
  }
  int dy = y - MAP_ARROW_Y;
  // com os botoes la, as setas param acima deles
  int dyMax = botoes ? MAP_BT_TOQ_Y0 - MAP_ARROW_Y : MAP_ARROW_HIT;
  if (dy > -MAP_ARROW_HIT && dy < dyMax) {
    if (x < MAP_ARROW_LX + MAP_ARROW_HIT) {
      if (mapView > 0) { mapView--; sfxPlay(SFX_TAP); }
      return true;
    }
    if (x > MAP_ARROW_RX - MAP_ARROW_HIT) {
      if (mapView < tope) { mapView++; sfxPlay(SFX_TAP); }
      return true;
    }
  }
  // La REVANCHA va ANTES que el poster: cae dentro de el, y el poster
  // viaja. Sin esto, tocar el boton te mandaba de viaje.
  if (mapView == pet.region && rematchReady() &&
      x >= MAP_RE_X && x <= MAP_RE_X + MAP_RE_W &&
      y >= MAP_RE_Y && y <= MAP_RE_Y + MAP_RE_H) {
    mapImg.unload();
    mapOpen = false;
    rematchOpen = true;
    rematchScroll = 0;
    sfxPlay(SFX_TAP);
    return true;
  }
  // un pelo al lado de los botones no cierra
  if (botoes && !lejosDe(x, y, MAP_BT_TOQ_X0, MAP_BT_TOQ_Y0, MAP_BT_TOQ_X1, MAP_BT_TOQ_Y1))
    return true;
  // el poster: viajar
  if (x > MAP_CX - MAP_MAX_W / 2 && x < MAP_CX + MAP_MAX_W / 2 &&
      y > MAP_CY - MAP_MAX_H / 2 && y < MAP_CY + MAP_MAX_H / 2) {
    if (pet.setZone(mapView)) {
      sfxPlay(SFX_EVOLVE);
      mapImg.unload();
      mapOpen = false;
    } else {
      sfxPlay(SFX_DENY);   // con candado, o ya estabas ahi
    }
    return true;
  }
  // Un pelo al lado del poster o de una flecha no cierra el mapa.
  if (!lejosDe(x, y, MAP_CX - MAP_MAX_W / 2, MAP_CY - MAP_MAX_H / 2,
               MAP_CX + MAP_MAX_W / 2, MAP_CY + MAP_MAX_H / 2))
    return true;
  if (dy > -MAP_ARROW_HIT - UI_FOSO && dy < MAP_ARROW_HIT + UI_FOSO &&
      (x < MAP_ARROW_LX + MAP_ARROW_HIT + UI_FOSO ||
       x > MAP_ARROW_RX - MAP_ARROW_HIT - UI_FOSO))
    return true;
  mapImg.unload();         // 150 KB de PSRAM que no hacen falta con esto cerrado
  mapOpen = false;
  return true;
}

// ----------------------------------------------------------- REVANCHA
//
// Coronado en una region, sus desafios se pueden volver a pelear: los
// ocho lideres, la Elite 4 y el campeon. Lo que cambia es que pelean
// TODOS al nivel del campeon, con piedra mega puesta y pagando mejor
// —— que es lo que lo convierte en el sitio donde entrenar de verdad y
// no en pegarle otra vez a un Brock de nivel 8.
//
// No da insignia: Pet::winGym() se va de vacio con las ocho puestas, o
// sea que esto no toca la progresion por ningun lado.
//
// Es de la region en la que ESTAS. Viajar baja las insignias a cero y
// con ellas la corona, asi que las revanchas de la region anterior se
// quedan atras —— igual que el techo de nivel. Viajar ya era una
// decision y sigue siendolo.
bool rematchOpen = false;
uint8_t rematchScroll = 0;
#define RE_ROWS 4
#define RE_X    60
#define RE_W    346
#define RE_Y0   112
#define RE_H    62
// LA FAJA DE PASAR PAGINA, debajo de la ultima fila: subir a la izquierda
// y bajar a la derecha, como en la bolsa del combate. Antes eran dos
// triangulitos (66x33 px, 3 mm de alto) y fallarlos cerraba la pantalla.
#define RE_PAG_Y 364
#define RE_PAG_H 34
#define RE_PASO  3        // filas por toque: se ve una de las de antes

// Se puede revanchar lo de ESTA region? Solo coronado en ella.
static bool rematchReady() {
  return !pet.isEgg() && pet.allGymsDone();
}

void renderRematch() {
  drawDexSkin();
  const uint8_t n = pet.gymCount();
  const uint8_t niv = gymChampLevel(pet.region);
  gfx->setTextColor(CARD_INK);
  setSize(3);
  setCur(centerX(T(S_REVANCHA), 3), 44);
  printT(T(S_REVANCHA));
  char sub[40];
  snprintf(sub, sizeof(sub), T(S_REV_SUB_FMT), regionAt(pet.region).name,
           (unsigned)niv);
  gfx->setTextColor(CARD_SOFT);
  setSize(1);
  setCur(centerX(sub, 1), 72);
  printT(sub);

  if (rematchScroll + RE_ROWS > n)
    rematchScroll = (uint8_t)(n > RE_ROWS ? n - RE_ROWS : 0);

  for (uint8_t r = 0; r < RE_ROWS; r++) {
    uint8_t i = (uint8_t)(rematchScroll + r);
    if (i >= n) break;
    const GymEntry &g = gymAt(pet.region, i);
    int y = RE_Y0 + r * RE_H;
    gfx->fillRoundRect(RE_X, y, RE_W, RE_H - 8, 12, CARD_PAPER);
    gfx->drawRoundRect(RE_X, y, RE_W, RE_H - 8, 12, CARD_DIM);
    gfx->setTextColor(CARD_INK);
    setSize(2);
    setCur(RE_X + 14, y + 10);
    printT(g.leader);
    gfx->setTextColor(CARD_SOFT);
    setSize(1);
    setCur(RE_X + 14, y + 32);
    printT(g.place);
    // La chapa del tipo, a la derecha: dice de un vistazo contra que vas.
    int cw = 26 + textW(typeName(g.type), 1) + 16;
    drawTypeChip(RE_X + RE_W - cw - 12, y + 14, cw, 26, g.type, 1);
  }

  // PASAR PAGINA: dos botones con cuantos quedan en cada sentido. El que
  // no tiene a donde ir se pinta apagado (y tocarlo no hace nada).
  if (n > RE_ROWS) {
    uint8_t arriba = rematchScroll;
    uint8_t abajo = (uint8_t)(n - rematchScroll - RE_ROWS);
    for (uint8_t k = 0; k < 2; k++) {
      uint8_t quedan = k ? abajo : arriba;
      int bx = k ? 238 : 78;
      gfx->fillRoundRect(bx, RE_PAG_Y, 150, RE_PAG_H, 10, quedan ? CARD_DIM : CARD_PAPER);
      gfx->drawRoundRect(bx, RE_PAG_Y, 150, RE_PAG_H, 10, CARD_DIM);
      uint16_t tinta = quedan ? CARD_INK : CARD_DIM;
      int cx = bx + 56, cy = RE_PAG_Y + RE_PAG_H / 2;
      if (k) gfx->fillTriangle(cx, cy + 9, cx - 16, cy - 8, cx + 16, cy - 8, tinta);
      else   gfx->fillTriangle(cx, cy - 9, cx - 16, cy + 8, cx + 16, cy + 8, tinta);
      if (quedan) {
        char q[8];
        snprintf(q, sizeof(q), "%u", (unsigned)quedan);
        gfx->setTextColor(CARD_INK);
        setSize(2);
        setCur(bx + 84, RE_PAG_Y + 10);
        printT(q);
      }
    }
  }

  gfx->setTextColor(CARD_SOFT);
  setSize(1);
  setCur(centerX(T(S_FORA_VOLTAR), 1), 404);
  printT(T(S_FORA_VOLTAR));
  gfx->flush();
}

// Mueve la lista una pagina. dir +1 = bajar. true si se movio.
static bool rematchPagina(int dir) {
  uint8_t n = pet.gymCount();
  uint8_t tope = (uint8_t)(n > RE_ROWS ? n - RE_ROWS : 0);
  uint8_t antes = rematchScroll;
  if (dir > 0)
    rematchScroll = (uint8_t)(rematchScroll + RE_PASO > tope ? tope
                                                             : rematchScroll + RE_PASO);
  else
    rematchScroll = (uint8_t)(rematchScroll > RE_PASO ? rematchScroll - RE_PASO : 0);
  return rematchScroll != antes;
}

// ARRASTRAR en vertical mueve la lista (el dedo hacia arriba baja por
// ella). true si el gesto era para la revancha: antes abria el reloj o la
// ficha POR DEBAJO de la lista.
bool rematchSwipeV(int dir) {
  if (!rematchOpen) return false;
  if (rematchPagina(dir < 0 ? +1 : -1)) sfxPlay(SFX_TAP);
  return true;
}

void rematchTap(int16_t x, int16_t y) {
  const uint8_t n = pet.gymCount();
  // LA FAJA DE PASAR PAGINA, de lado a lado: mitad izquierda sube, mitad
  // derecha baja. Un toque aqui NUNCA cierra, aunque no haya a donde ir.
  if (y >= RE_PAG_Y - 4 && y <= RE_PAG_Y + RE_PAG_H &&
      x >= RE_X && x <= RE_X + RE_W) {
    if (rematchPagina(x < 233 ? -1 : +1)) sfxPlay(SFX_TAP);
    return;
  }
  for (uint8_t r = 0; r < RE_ROWS; r++) {
    uint8_t i = (uint8_t)(rematchScroll + r);
    if (i >= n) break;
    int ry = RE_Y0 + r * RE_H;
    if (x < RE_X || x > RE_X + RE_W || y < ry || y > ry + RE_H - 8) continue;
    if (!pet.canBattle()) { sfxPlay(SFX_DENY); return; }
    rematchOpen = false;
    rematchStart(i);
    return;
  }
  // Cerrar es tocar FUERA de la lista, y lejos: entre dos filas, o un pelo
  // al lado, no pasa nada.
  if (!lejosDe(x, y, RE_X, RE_Y0, RE_X + RE_W, RE_PAG_Y + RE_PAG_H)) return;
  rematchOpen = false;    // tocar fuera: volver
  sfxPlay(SFX_TAP);
}

void drawCelebration() {
  const char *l1 = nullptr, *l2 = nullptr;
  char prem[48];
  prem[0] = 0;
  // 32 y no 20: "%u ã«ã¡ã‚Œã‚“ãžãï¼" ya pasa de 20 con una sola cifra, y snprintf
  // cortaba a mitad de un caracter de 3 bytes.
  char buf[32];
  if (pet.showMedal()) {
    for (int i = 0; i < MED_COUNT; i++)
      if (pet.newMedal & (1 << i)) {
        l2 = medalName(i);
        // Y QUE te dan por ella. Sin esta linea el objeto aparecia en la
        // bolsa sin que nada lo hubiera dicho, que es lo mismo que no
        // darlo: el jugador no se entera.
        uint8_t it = MEDAL_PRIZE[i];
        if (it) snprintf(prem, sizeof(prem), T(S_PRIZE_FMT), itemAt(it).name);
        break;
      }
    l1 = T(S_MEDAL_BANNER);
  } else if (pet.showMilestone()) {
    snprintf(buf, sizeof(buf), T(S_STREAK_DAYS_FMT), pet.streak);
    l1 = T(S_GREAT);
    l2 = buf;
  }
  // Las DOS, no solo la primera: si newMedal no lleva ningun bit que el
  // bucle sepa encontrar, l2 se queda a nullptr y centerX() acaba
  // llamando a strlen(NULL). Pasaba al ganar una medalla y nacer el
  // huevo dentro de los 4 s del cartel.
  if (!l1 || !l2) return;
  // leva 71: a janela dourada do jogo, uma medalha desenhada (fita +
  // disco com estrela) e a letra de DS. Antes era um retangulo laranja.
  bool medalha = pet.showMedal();
  int alto = prem[0] ? 196 : 164, y0 = 136;
  drawWindow(58, y0, 350, alto, 18);
  int my = y0 + 44;
  if (medalha) {
    // a fita, atras do disco
    gfx->fillTriangle(CX - 22, my - 34, CX - 4, my - 34, CX - 14, my, C565(0x30, 0x60, 0xd0));
    gfx->fillTriangle(CX + 4, my - 34, CX + 22, my - 34, CX + 14, my, C565(0xd0, 0x30, 0x40));
    gfx->fillCircle(CX, my, 21, INK_K);
    gfx->fillCircle(CX, my, 18, C565(0xc8, 0x90, 0x18));
    gfx->fillCircle(CX, my, 14, C565(0xf8, 0xd0, 0x40));
    // estrela de 5 pontas
    float a0 = -1.5708f;
    for (int k = 0; k < 5; k++) {
      float a = a0 + k * 1.2566f, b = a + 0.6283f, c = a - 0.6283f;
      gfx->fillTriangle(CX + (int)(10 * cosf(a)), my + (int)(10 * sinf(a)),
                        CX + (int)(4 * cosf(b)), my + (int)(4 * sinf(b)),
                        CX + (int)(4 * cosf(c)), my + (int)(4 * sinf(c)),
                        C565(0xff, 0xff, 0xe0));
    }
    gfx->fillCircle(CX, my, 4, C565(0xff, 0xff, 0xe0));
  } else {
    drawMap(SPR_HEART, 32, CX - 16, my - 16, 1, false);
  }
  uint8_t n1 = tituloTalla(l1, 3, 300);
  printTituloC(l1, y0 + 76, n1, C565(0xc0, 0x50, 0x10), C565(0xf0, 0xd8, 0x90));
  uint8_t n2 = tituloTalla(l2, 2, 300);
  printTituloC(l2, y0 + 114, n2, UI_INK, C565(0xc8, 0xc0, 0xa8));
  if (prem[0]) {
    uint8_t ts = tituloTalla(prem, 2, 290);
    printTituloC(prem, y0 + 148, ts, C565(0x28, 0x80, 0x40), C565(0xc8, 0xe0, 0xc0));
  }
}

// medallas en la ficha: badge con etiqueta, color si conseguida
void drawMedalBadge(int x, int y, int i) {
  bool got = pet.hasMedal(1 << i);
  gfx->fillRoundRect(x, y, 100, 24, 6, got ? UI_BAR_OK : UI_TRACK);
  if (!got) gfx->drawRoundRect(x, y, 100, 24, 6, UI_TRACK);
  gfx->setTextColor(got ? UI_BG_DAY : 0x9492);
  setSize(2);
  setCur(x + (100 - textW(medalLabel(i), 2)) / 2, y + 5);
  printT(medalLabel(i));
}

// pagina 0: perfil (retrato grande, identidad, racha, vinculo, baya)
void renderCardProfile() {
  const DexEntry &d = DEX_TBL[pet.speciesId];
  const char *nm = pet.nick[0] ? pet.nick : dexName(pet.speciesId);
  char head[DEX_HEAD_BUF];
  snprintf(head, sizeof(head), T(S_NAME_FMT), "", nm, pet.level());
  gfx->setTextColor(d.accent);
  // auto-encoge: a tamano 3 los nombres largos no caben en la franja estrecha de
  // arriba de la pantalla redonda, asi que se cortaban por el borde. El ancho se
  // MIDE con textW(): strlen() cuenta bytes, y en una fila UTF-8 no hay un byte
  // por caracter ni todos miden lo mismo. Con la fuente clasica textW() devuelve
  // strlen()*6*size, o sea exactamente la cuenta anterior: no mueve un pixel en
  // los seis idiomas latinos (11 caracteres a tamano 3 son esos 198 px).
  setSize(3);
  int hts = (textW(head, 3) <= 198) ? 3 : 2;
  setSize(hts);
  int hy = hts == 3 ? 34 : 40;
  // la estrella entra en la medida, para centrar el conjunto y no el texto
  // E a BOLA em que foi capturado, antes de tudo, como nos jogos. Quem
  // nasceu aqui (ou veio de antes das bolas) aparece na Poke Ball.
  const uint8_t bolaId = pet.ballId ? pet.ballId : BALL_ID[0];
  const Art *bola = bolaId < ITEM_COUNT ? ART_ITEM[bolaId] : nullptr;
  int bhueco = bola ? bola->w + 6 : 0;
  int hhueco = (pet.shiny ? ART_SHINY.w + 6 : 0) + bhueco;
  int hx = (466 - (textW(head, hts) + hhueco)) / 2;
  if (bola) drawArt(*bola, hx + bola->w / 2, hy + 4 * hts + bola->h / 2, 1);
  if (pet.shiny)
    drawShinyStar(ART_SHINY, hx + bhueco + ART_SHINY.w / 2, hy + 4 * hts);
  setCur(hx + hhueco, hy);
  printT(head);
  if (pet.nick[0]) {  // especie real bajo el apodo
    const char *sp = dexName(pet.speciesId);
    char par[32];
    snprintf(par, sizeof(par), "(%s)", sp);
    gfx->setTextColor(CARD_SOFT);
    setSize(2);
    setCur(centerX(par, 2), 64);   // medido, no contado en bytes
    printT(par);
  }

  // retrato grande animado
  if (pmd.loaded) drawPmdAct(PMD_IDLE, CX, 206, millis(), true, false, 4);

  // Lo que lleva encima, en una chapa junto al bicho. Hasta ahora el objeto
  // equipado solo se veia como una fila resaltada dentro de la bolsa: habia
  // que abrirla y buscar cual estaba marcada para saber que llevaba puesto.
  // OJO: 46 de los 212 objetos no tienen icono —— entre ellos TODAS las
  // megapiedras. Mientras esto exigia el dibujo, llevar una Arcanite se
  // veia exactamente igual que no llevar nada, y el dueno dio por hecho
  // que la piedra se habia perdido al megaevolucionar. No se habia
  // perdido: no se dibujaba. Sin icono, se escribe el nombre.
  if (pet.heldItem && pet.heldItem < ITEM_COUNT) {
    const Art *ic = ART_ITEM[pet.heldItem];
    if (ic) {
      gfx->fillCircle(366, 178, 22, CARD_PAPER);
      gfx->drawCircle(366, 178, 22, CARD_INK);
      drawArt(*ic, 366, 178 + ic->h / 2, 1);
    } else {
      const char *nom = itemAt(pet.heldItem).name;
      int w = (int)strlen(nom) * 6 + 10;
      if (w > 150) w = 150;
      gfx->fillRoundRect(366 - w / 2, 168, w, 20, 6, CARD_PAPER);
      gfx->drawRoundRect(366 - w / 2, 168, w, 20, 6, CARD_INK);
      gfx->setTextColor(CARD_INK);
      setSize(1);
      setCur(366 - (int)strlen(nom) * 3, 175);
      printT(nom);
    }
  }

  // racha con llama
  int sx = 138, sy = 224;
  gfx->fillTriangle(sx + 8, sy, sx + 1, sy + 18, sx + 15, sy + 18, UI_BAR_BAD);
  gfx->fillTriangle(sx + 8, sy + 7, sx + 4, sy + 18, sx + 12, sy + 18, UI_BAR_WARN);
  char rl[40];  // en japones pierde cifras con 30 a partir de 100 dias
  snprintf(rl, sizeof(rl), T(S_STREAK_FMT), pet.streak, pet.bestStreak);
  gfx->setTextColor(CARD_INK);
  setSize(2);
  setCur(sx + 24, sy + 2);
  printT(rl);

  // La barra se pone ROJA justo cuando el bicho empieza a no hacerte caso.
  // Sin esto el jugador ve "se planta" en combate y no tiene forma de atar
  // ese cabo con el descuido, que es de lo que va todo el asunto.
  uint8_t pasa = Battle::disobeyChance(pet.bond);
  drawCardStat(258, T(S_VIN), pet.bond, 100,
               pasa ? UI_BAR_BAD : C565(0xd4, 0x52, 0x7e));
  if (pasa) {
    char av[28];
    snprintf(av, sizeof(av), "%s %u%%", T(S_DISOBEY_LBL), pasa);
    gfx->setTextColor(UI_BAR_BAD);
    setSize(1);
    setCur(centerX(av, 1), 280);
    printT(av);
  }

  const char *berry = !pet.berryKnown ? T(S_BERRY_UNK)
                      : pet.lovesBerry(0) ? T(S_BERRY_RED)
                      : pet.lovesBerry(1) ? T(S_BERRY_BLUE)
                                          : T(S_BERRY_GREEN);
  char info[48];
  snprintf(info, sizeof(info), T(S_INFO_FMT), berry,
           (unsigned long)(pet.ageMinutes / 1440));
  gfx->setTextColor(CARD_INK);
  setSize(2);
  setCur(centerX(info, 2), 296);
  printT(info);

  // El PESO vive aqui y no entre los stats de combate: no se pelea con el,
  // se engorda comiendo y adelgaza con el tiempo (ver `whyChubby`). Estaba
  // en la pagina de BATTLE con una barra al lado del ataque, como si fuera
  // un stat, y encima casi siempre en 0 porque solo sube al comer.
  // La linea de BERRY/AGE acaba en y=312 (tamano 2 son 16 px de alto), asi
  // que el peso va en 320 y la pista baja a 338: a 316 y 332 quedaban
  // pegados y parecian la misma linea partida.
  char pes[20];
  snprintf(pes, sizeof(pes), "%s %u", T(S_STAT_WGT), pet.weight);
  gfx->setTextColor(CARD_SOFT);
  setSize(1);
  setCur(centerX(pes, 1), 320);
  printT(pes);

  setSize(2);
  setCur(centerX(T(S_RENAME_HINT), 2), 338);
  printT(T(S_RENAME_HINT));
}

// pagina 1: combate (4 barras + boton de entrenar)
void renderCardStats() {
  gfx->setTextColor(CARD_INK);
  setSize(3);
  setCur(centerX(T(S_BATTLE), 3), 48);
  printT(T(S_BATTLE));

  // Los PS arriba del todo y con su barra: ahora que atraviesan los combates,
  // es el numero que hay que mirar antes de salir a pelear. "HP" va sin
  // traducir a proposito: es lo que ponen los propios juegos en todos los
  // idiomas, incluido el japones.
  {
    uint16_t tope = pet.maxHp();
    uint32_t pct = tope ? (uint32_t)pet.hpLeft * 100 / tope : 0;
    uint16_t col = pct >= 50 ? UI_BAR_OK
                 : (pct >= BATTLE_HP_FLOOR_PCT ? UI_BAR_WARN : UI_BAR_BAD);
    drawCardStat(78, "HP", pet.hpLeft, tope ? tope : 1, col);
  }
  // La habilidad: no es una barra, es una etiqueta. Va arriba con los PS
  // porque es lo otro que define como pelea este bicho en concreto.
  if (pet.ability) {
    gfx->setTextColor(CARD_INK);
    setSize(1);
    setCur(96, 100);
    printT(T(S_HABILIDADE));
    setSize(2);
    const char *an = abilAt(pet.ability).name;
    setCur(330 - (int)strlen(an) * 12, 96);
    printT(an);
    // Una punta de flecha diminuta: es lo unico de esta pagina que se puede
    // tocar aparte del boton, y sin la marca nadie lo descubriria.
    gfx->fillTriangle(340, 98, 348, 102, 340, 106, CARD_DIM);
  }
  // Los SEIS stats de combate, no tres. El fisico y el especial llevan
  // tiempo siendo cosas distintas (genes y EVs propios desde que se
  // partieron), pero esta pagina solo ensenaba ATK/DEF/VEL: quien criaba un
  // bicho especial no tenia donde ver el resultado. Las filas bajan de 42 a
  // 30 px de paso para que quepan las seis sin tocar el boton de abajo.
  drawCardStat(116, T(S_STAT_ATK), pet.atkStat(), 260, UI_BAR_BAD);
  drawCardStat(146, T(S_STAT_DEF), pet.defStat(), 260, 0x4C98);
  // Morado y verdeazul APAGADOS: el magenta y el cian puros cantaban
  // muchisimo al lado del rojo/azul/naranja de las otras barras.
  drawCardStat(176, T(S_STAT_SPA), pet.spAtkStat(), 260, C565(0x9b, 0x59, 0xc4));
  drawCardStat(206, T(S_STAT_SPD), pet.spDefStat(), 260, C565(0x3d, 0xa0, 0xa0));
  drawCardStat(236, T(S_STAT_SPE), pet.speStat(), 260, UI_BAR_WARN);
  // El PESO se fue a la pagina de perfil: no es un stat de combate, y aqui
  // parecia uno (con su barra, al lado del ataque) estando casi siempre a 0.

  // Y EL TIPO, que no salia en ninguna pantalla del juego. Va aqui y no
  // en el perfil por dos razones: esta es la pagina de COMBATE, que es
  // para lo unico que el tipo sirve, y es el unico hueco limpio que hay
  // —— la ultima barra acaba en 260 y el boton empieza en 300, asi que
  // una franja de 28 en 264 no le quita el sitio a nada.
  {
    uint8_t t1, t2;
    tiposDe(pet.speciesId, pet.form, -1, &t1, &t2);
    drawTypesOf(233 - (2 * 104 + 10) / 2, 264, 104, 28, 10, false, t1, t2, 1);
  }

  // Aqui habia un boton de IR AL GIMNASIO. Era un atajo a algo que ya
  // esta a UN TOQUE en la pantalla de casa (el segundo boton del
  // circulo), y el dueno lo zanjo: "pode trocar por alguma coisa mais
  // util, ja temos o treino la fora". El sitio pasa a lo que de verdad
  // se mira antes de pelear y no se puede deducir de las dos chapas de
  // arriba —— contra que es debil y que no le hace nada.
  {
    uint8_t wt1, wt2;
    tiposDe(pet.speciesId, pet.form, -1, &wt1, &wt2);
    drawDebilidades(300, wt1, wt2);
  }

  // El cuadro de la habilidad, encima de todo. Va en popup y no fijo porque
  // la habilidad es UNA y no cambia nunca: ocuparia tres lineas permanentes
  // para algo que se consulta una vez en la vida del bicho.
  if (abilShown && pet.ability) {
    gfx->fillRoundRect(58, 150, 324, 130, 14, CARD_INK);
    gfx->fillRoundRect(62, 154, 316, 122, 12, CARD_PAPER);
    gfx->setTextColor(UI_BAR_BAD);
    setSize(2);
    setCur(centerX(abilAt(pet.ability).name, 2), 168);
    printT(abilAt(pet.ability).name);
    gfx->setTextColor(CARD_INK);
    // Talla 2. La caja da 316 px por dentro: a 12 px el caracter son 26
    // columnas, y cuatro lineas de 18 ocupan de 196 a 268, que cabe en
    // los 122 de alto. Antes eran cinco lineas de 50 columnas a talla 1
    // y no habia quien lo leyera en una pantalla de 466.
    drawWrapped(ABIL_DESC[pet.ability], 196, 26, 18, 4, 2);
    gfx->setTextColor(CARD_SOFT);
    setSize(1);
    setCur(centerX(T(S_TOQUE_FECHAR), 1), 266);
    printT(T(S_TOQUE_FECHAR));
  }
}

// ---------- pagina de gimnasios ----------
// El desafio NO salta solo: se viene aqui cuando uno se siente preparado,
// que es lo que el jugador pidio. Lo unico que hace el porton es decir
// cuando tienes derecho, y lo dice contando.
// El boton, alineado con la columna de texto (GYM_COL_X) en vez de
// centrado en la pantalla. Centrado ocupaba de 122 a 344 y era lo que le
// quitaba el sitio al retrato por abajo. Mas estrecho (176 en vez de
// 222) y mas a la derecha; la letra la elige elTamanoQueCabe.
#define GYM_BTN_X GYM_COL_X
#define GYM_BTN_Y 316
#define GYM_BTN_W 176
#define GYM_BTN_H 46

// Las TRES casillas de la casa. Se pelea en orden, asi que solo una es
// tocable: la siguiente. Las ya ganadas quedan en silueta con su marca y
// las que faltan en silueta a secas —— se lee de un vistazo por donde vas
// sin tener que contar nada.
static void gymDrawTrainers(const GymEntry &g) {
  uint8_t hecho = pet.gymTrainers;
  for (uint8_t i = 0; i < GYM_TRAINERS; i++) {
    int x = GYMTR_X0 + i * (GYMTR_W + GYMTR_GAP);
    bool ganado = (i < hecho);
    bool toca = (i == hecho);
    const Art &spr = *ART_TRAINER[gymTrainerIdx(pet.gymNext(), i)];
    gfx->fillRoundRect(x, GYMTR_Y0, GYMTR_W, GYMTR_H, 12,
                       toca ? CARD_DIM : CARD_PAPER);
    // El que toca, con filo que late: es lo unico que se puede tocar en
    // media pantalla y tiene que decirlo.
    if (toca && ((millis() / 420) & 1))
      gfx->drawRoundRect(x, GYMTR_Y0, GYMTR_W, GYMTR_H, 12, UI_BAR_WARN);

    // A escala 1 el mas alto son 80 px: con la base en +84 le sobran 4
    // por arriba y quedan 20 abajo para la tira de estado.
    int cx = x + GYMTR_W / 2, base = GYMTR_Y0 + 84;
    const int tira = GYMTR_Y0 + GYMTR_H - 19;   // el renglon de estado
    // Tres estados, tres lecturas: el hecho en gris legible, el que toca
    // a color y el que falta en la misma silueta apagada con la que la
    // Pokedex dice "este todavia no".
    if (toca)        drawArt(spr, cx, base, 1);
    else if (ganado) drawArtSolid(spr, cx, base, 1, CARD_SOFT);
    else             drawArtSolid(spr, cx, base, 1, DEX_SIL);

    if (ganado) {
      // Tira verde con un visto. El visto va a dos rayas y no con la 'v'
      // de la fuente: esa mide 5x7, dentro de una tira de 16 nada, y
      // centrarla a ojo fue lo que la saco del circulo.
      gfx->fillRoundRect(x + 16, tira, GYMTR_W - 32, 16, 6, UI_BAR_OK);
      gfx->drawLine(cx - 6, tira + 8, cx - 2, tira + 12, CARD_PAPER);
      gfx->drawLine(cx - 6, tira + 7, cx - 2, tira + 11, CARD_PAPER);
      gfx->drawLine(cx - 2, tira + 12, cx + 6, tira + 4, CARD_PAPER);
      gfx->drawLine(cx - 2, tira + 11, cx + 6, tira + 3, CARD_PAPER);
    } else if (toca) {
      // Y a que nivel pelea, que es lo que hay que saber antes de entrar.
      char lv[12];
      snprintf(lv, sizeof(lv), "Lv.%u", (unsigned)gymTrainerLevel(g, i));
      gfx->setTextColor(CARD_INK);
      setSize(1);
      setCur(cx - textW(lv, 1) / 2, tira + 5);
      printT(lv);
    }
  }
}

void renderCardGym() {
  gfx->setTextColor(CARD_INK);
  setSize(3);
  setCur(centerX(T(S_GYMS), 3), 30);
  printT(T(S_GYMS));
  // En que region estas. Antes sobraba (solo habia una) y ahora es lo
  // primero que hay que saber al abrir la pagina: las mismas ocho
  // insignias vacias significan cosas distintas en Kanto y en Johto.
  gfx->setTextColor(CARD_SOFT);
  setSize(1);
  setCur(centerX(pet.regionName(), 1), 58);
  printT(pet.regionName());

  // Las insignias YA NO van aqui: se fueron a la ficha de entrenador,
  // que es donde se ensena lo que has hecho. Esta pagina es lo que
  // tienes POR DELANTE, y lo de delante son los tres de la casa.

  if (pet.allGymsDone()) {
    char ch[40];
    snprintf(ch, sizeof(ch), T(S_GYM_CHAMP), pet.regionName());
    gfx->setTextColor(UI_BAR_WARN);
    setSize(2);
    setCur(centerX(ch, 2), 210);
    printT(ch);
    // Coronado y con otra region esperando: el boton deja de desafiar y
    // pasa a viajar. Es la MISMA casilla porque es la misma pregunta â€”
    // "que hago ahora". Ya no es una decision con precio —— viajar no
    // quita nada ——, solo el camino corto: el mapa tambien lleva.
    if (pet.canAdvanceRegion()) {
      char ir[48];
      snprintf(ir, sizeof(ir), T(S_IR_PARA_FMT), regionAt(pet.region + 1).name);
      gfx->fillRoundRect(GYM_BTN_X, GYM_BTN_Y, GYM_BTN_W, GYM_BTN_H, 13, UI_BAR_OK);
      gfx->setTextColor(CARD_PAPER);
      // Mismo criterio que el otro boton: lo mas grande que quepa. El
      // boton es ahora mas estrecho (176) y "GO TO JOHTO" a tamano 2
      // mide 132, pero una region de nombre largo se saldria.
      int ti = 2;
      while (ti > 1 && textW(ir, (uint8_t)ti) > GYM_BTN_W - 16) ti--;
      setSize((uint8_t)ti);
      setCur(GYM_BTN_X + (GYM_BTN_W - textW(ir, (uint8_t)ti)) / 2,
             GYM_BTN_Y + (GYM_BTN_H - ti * 8) / 2);
      printT(ir);
      // Ya NO dice "badges back to zero": viajar dejo de costar nada.
      // Cada region recuerda sus insignias y se intercambian al moverse,
      // asi que ir y volver no te quita lo que hiciste.
      gfx->setTextColor(CARD_SOFT);
      setSize(1);
      setCur(centerX(T(S_INSIG_FICAM), 1), GYM_BTN_Y - 14);
      printT(T(S_INSIG_FICAM));
    }
    return;
  }

  const GymEntry &g = pet.gymHere();
  gymDrawTrainers(g);

  // El retrato del que toca. A escala 2 (los de Kanto vienen reducidos a la
  // mitad y los de Johto ya nacen a ese tamano, ver gen_art.py) llena la
  // franja hasta el boton y se le ve la cara; a escala 1 era un sello.
  //
  // Los de Johto NO son del Anil: no los tiene, ni Unbreakable Ties ni
  // Reloaded. Salen de Bulbagarden Archives (tools/fetch_leaders.py), que
  // es la fuente que paso el usuario.
  static_assert(sizeof(ART_LEADER) / sizeof(ART_LEADER[0]) == REGION_COUNT,
                "falta una fila de retratos: una region quedaria con la cara de otra");
  // METIDO en la banda que dejan las casillas, llenandola. Antes iba a
  // escala 2 fija: el mas alto medía 160 con la base en 306, arrancaba
  // en 146 y le pisaba el pie a las dos primeras casillas, que acaban en
  // 176. Se arreglo encogiendolo —— y quedo CHICO.
  //
  // Ahora se hace al reves: se le da toda la banda (184 a 350, que es
  // hasta donde llega sin tocar el boton ni el cristal) y se estira lo
  // que quepa. Sin tope de escala: el tope era lo que lo dejaba pequeno.
  {
    const Art &ret = *ART_LEADER[pet.region][pet.gymNext()];
    const int banda = GYM_LEAD_BASE - GYM_LEAD_TOP;
    int ancho = ret.h ? (int)ret.w * banda / (int)ret.h : GYM_LEAD_MAXW;
    // Y no puede llegar a la columna de texto ni salirse del cristal por
    // abajo a la izquierda: con 180 de ancho cinco de los 117 se salian.
    if (ancho > GYM_LEAD_MAXW) ancho = GYM_LEAD_MAXW;
    const int alto = ret.w ? (int)ret.h * ancho / (int)ret.w : 0;
    drawArtZoom(ret, GYM_LEAD_CX, GYM_LEAD_BASE - alto / 2, ancho);
  }

  gfx->setTextColor(CARD_INK);
  setSize(2);
  setCur(GYM_COL_X, 190);
  printT(g.leader);
  gfx->setTextColor(CARD_SOFT);
  setSize(1);
  setCur(GYM_COL_X, 212);
  printT(g.place);
  // chip del tipo de la casa
  drawTypeChip(GYM_COL_X, 222, 26 + textW(typeName(g.type), 1) + 16, 26,
               g.type, 1);
  char nv[32];
  // El AS y cuantos saca: antes de entrar hay que saber que no es uno solo.
  uint8_t nEq = gymTeamSize(g);
  if (nEq > 1)
    snprintf(nv, sizeof(nv), "%s Lv.%u  x%u", dexName(gymTeamAt(g, nEq - 1)),
             g.level, nEq);
  else
    snprintf(nv, sizeof(nv), "%s Lv.%u", dexName(gymTeamAt(g, 0)), g.level);
  gfx->setTextColor(CARD_INK);
  setCur(GYM_COL_X, 256);
  printT(nv);
  // El techo, que es lo que de verdad importa antes de entrar: se pelea a
  // este nivel aunque el bicho sea de 70.
  char cp[40];
  snprintf(cp, sizeof(cp), T(S_CAP_LV_FMT), (unsigned)pet.levelCap());
  gfx->setTextColor(pet.overCap() ? UI_BAR_BAD : CARD_DIM);
  setCur(GYM_COL_X, 268);
  printT(cp);

  // El porton, contado. 32 y no 24: en japones "ã—ã‚‡ã†ã‚Š" son 12 bytes y con
  // las dos cifras se pasaba (lo caza TestBuffersDeTexto).
  char w[32];
  // Ya no cuenta victorias contra salvajes: cuenta los TRES de la casa.
  // Se reaprovecha el mismo formato ("%u/%u victorias") porque es lo que
  // sigue diciendo —— cuantas llevas de las que hacen falta.
  snprintf(w, sizeof(w), T(S_GYM_WINS_FMT), pet.gymTrainers, GYM_TRAINERS);
  bool listo = pet.gymReady();
  gfx->setTextColor(listo ? UI_BAR_OK : CARD_INK);
  setSize(2);
  setCur(GYM_COL_X, 280);
  printT(w);

  gfx->fillRoundRect(GYM_BTN_X, GYM_BTN_Y, GYM_BTN_W, GYM_BTN_H, 13,
                     listo ? UI_BAR_BAD : CARD_DIM);
  const char *et = listo ? T(S_GYM_GO) : T(S_GYM_NOT_YET);
  gfx->setTextColor(listo ? CARD_PAPER : CARD_INK);
  // La letra, lo mas grande QUE QUEPA, y no un tamano fijo por estado.
  // Fijo habia que elegirlo por el idioma peor: "HERAUSFORDERN" mide 234
  // px a tamano 3 y "Aun no estas listo" 216 a tamano 2, asi que para que
  // el aleman no se saliera iban todos pequenos. Asi el ingles entra a 3
  // y a 2, y el que no quepa baja solo —— igual que el titulo de la ficha
  // de entrenador.
  const int util = GYM_BTN_W - 16;
  int ts = listo ? 3 : 2;
  while (ts > 1 && textW(et, (uint8_t)ts) > util) ts--;
  setSize((uint8_t)ts);
  setCur(GYM_BTN_X + (GYM_BTN_W - textW(et, (uint8_t)ts)) / 2,
         GYM_BTN_Y + (GYM_BTN_H - ts * 8) / 2);
  printT(et);
}

void cardGymTap(int16_t x, int16_t y) {
  bool enBoton = (x >= GYM_BTN_X && x <= GYM_BTN_X + GYM_BTN_W &&
                  y >= GYM_BTN_Y && y <= GYM_BTN_Y + GYM_BTN_H);
  // Un pelo al lado del boton (o de la casilla que toca) no cierra.
  bool casiBoton = !lejosDe(x, y, GYM_BTN_X, GYM_BTN_Y, GYM_BTN_X + GYM_BTN_W,
                            GYM_BTN_Y + GYM_BTN_H);
  if (pet.canAdvanceRegion()) {   // coronado: el boton viaja
    if (enBoton) sfxPlay(pet.advanceRegion() ? SFX_PLAY : SFX_DENY);
    else if (!casiBoton) cardOpen = false;
    return;
  }
  // Las tres casillas: la que TOCA se puede pelear. Va antes que el
  // boton del lider porque mientras falte uno de los tres, el boton del
  // lider no hace nada y estas casillas son lo unico vivo de la pagina.
  if (!pet.allGymsDone() && pet.gymTrainerNext() < GYM_TRAINERS) {
    uint8_t i = pet.gymTrainerNext();
    int bx = GYMTR_X0 + i * (GYMTR_W + GYMTR_GAP);
    if (!lejosDe(x, y, bx, GYMTR_Y0, bx + GYMTR_W, GYMTR_Y0 + GYMTR_H)) {
      if (x < bx || x > bx + GYMTR_W || y < GYMTR_Y0 || y > GYMTR_Y0 + GYMTR_H)
        return;                    // casi: nada
      {
        const GymEntry &g = pet.gymHere();
        uint8_t hora = (uint8_t)(pet.lastSeenEpoch
                                 ? (pet.lastSeenEpoch / 3600) % 24 : 13);
        int16_t d = gymTrainerSpecies(g.type, gymTrainerLevel(g, i));
        if (d < 1) d = pet.pickWildSpecies(hora, true);
        if (d < 1) { sfxPlay(SFX_DENY); return; }
        cardOpen = false;
        battleStart(d, gymTrainerLevel(g, i), -1, false,
                    (int8_t)gymTrainerIdx(pet.gymNext(), i), (int8_t)i);
        return;
      }
    }
  }
  if (!pet.gymReady()) { cardOpen = false; return; }
  if (!enBoton) { if (!casiBoton) cardOpen = false; return; }
  const GymEntry &g = pet.gymHere();
  uint8_t cual = pet.gymNext();
  cardOpen = false;
  battleStart(gymTeamAt(g, 0), gymLevelAt(g, 0), (int8_t)cual);
}

// LA FICHA DE ENTRENADOR. Es la pagina de lo que YA hiciste —— por eso va
// pegada a la del gimnasio, que es la de lo que te falta —— y es donde
// viven ahora las ocho insignias.
//
// El sprite es el del jugador DE VERDAD (el que eligio en la intro), no
// un monigote: es su ficha.
// UMA insignia, centrada em (cx, cy): a DAQUELA regiao. Kanto e Johto sao
// mascaras em cinza, tingidas aqui com o tipo do ginasio (brilho x cor,
// a mesma conta que o gen_art fazia); de Hoenn em diante vem com cor. Sem
// desenho (o Brycen), cai na mascara de Kanto tingida com o tipo dele.
void drawBadge(uint8_t reg, uint8_t i, int cx, int cy, uint8_t esc) {
  if (i >= 8) return;
  const uint8_t r = reg < ART_BADGE_REGIOES ? reg : 0;
  const Art *a = ART_BADGE_REG[r][i];
  bool mascara = ART_BADGE_MASCARA[r];
  uint8_t np = ART_BADGE_NPAL[r][i];
  // De Hoenn em diante vem do cartao, com cor.
  if (r >= ART_BADGE_SD_PRIMEIRA) {
    a = insigniasSd.get(r, i);
    mascara = false;
  }
  if (!a) {
    a = ART_BADGE_REG[0][i];
    mascara = true;
    np = ART_BADGE_NPAL[0][i];
  }
  Art c = *a;
  static uint16_t pal[256];
  if (mascara) {
    const uint16_t t = typeColor(gymAt(reg, i).type);
    const int tr = ((t >> 11) & 31) * 255 / 31, tg = ((t >> 5) & 63) * 255 / 63,
              tb = (t & 31) * 255 / 31;
    for (uint8_t k = 0; k < np; k++) {
      uint16_t p = a->pal[k];
      int r8 = ((p >> 11) & 31) * 255 / 31, g8 = ((p >> 5) & 63) * 255 / 63,
          b8 = (p & 31) * 255 / 31;
      int luma = (r8 * 30 + g8 * 59 + b8 * 11) / 100;
      pal[k] = C565(tr * luma / 255, tg * luma / 255, tb * luma / 255);
    }
    c.pal = pal;
  }
  drawArt(c, cx, cy + c.h * esc / 2, esc);
}

#define TCARD_BX0 74      // las insignias, dos filas de cuatro
#define TCARD_BY0 248
void renderCardTrainer() {
  // El titulo, a y=44: ahi la cuerda del circulo da 264 px. A 28 solo
  // daba 212 y "TRAINER CARD" mide 216 —— y ese es el corto, el frances
  // son 252. Si aun asi no cupiera (los ocho idiomas no miden lo mismo
  // y el japones ni usa esta fuente), baja solo a tamano 2.
  const char *tit = T(S_CARD);
  const int ts = textW(tit, 3) <= 260 ? 3 : 2;
  gfx->setTextColor(CARD_INK);
  setSize(ts);
  setCur(centerX(tit, ts), ts == 3 ? 42 : 48);
  printT(tit);
  gfx->setTextColor(CARD_SOFT);
  setSize(1);
  setCur(centerX(pet.regionName(), 1), 68);
  printT(pet.regionName());

  // El retrato, a la izquierda. Mide 139 de alto: con la base en 214 le
  // queda la cabeza en 75, justo debajo del subtitulo.
  const Art &yo = pet.playerGirl ? ART_GIRL : ART_BOY;
  drawArt(yo, 110, 214, 1);

  // Y las cuentas, a la derecha: etiqueta pequena encima y el numero
  // gordo debajo, que es como se lee un marcador. Son las dos que pidio
  // el usuario —— victorias y capturados —— y las dos son del JUGADOR,
  // no del bicho: sobreviven a cada crianza.
  char val[24];
  const StrId etiqueta[2] = { S_CARD_WINS, S_CARD_CAUGHT };
  for (int i = 0; i < 2; i++) {
    int y = 84 + i * 50;
    gfx->setTextColor(CARD_SOFT);
    setSize(1);
    setCur(206, y);
    printT(T(etiqueta[i]));
    if (i == 0) snprintf(val, sizeof(val), "%lu", (unsigned long)pet.wins);
    else        snprintf(val, sizeof(val), "%u", (unsigned)pet.caught);
    gfx->setTextColor(CARD_INK);
    setSize(3);
    setCur(206, y + 12);
    printT(val);
  }
  // Y la Pokedex, con SU cadena: "POKEDEX %u/%u" ya estaba traducida en
  // los ocho idiomas, asi que no hace falta inventar una etiqueta nueva.
  snprintf(val, sizeof(val), T(S_POKEDEX_FMT),
           (unsigned)pet.registeredCount(), (unsigned)DEX_COUNT);
  gfx->setTextColor(CARD_SOFT);
  setSize(2);
  setCur(206, 190);
  printT(val);

  // Las ocho insignias, que es a lo que venia la pagina. Las que faltan,
  // en gris: se ve de un vistazo cuanto queda de region.
  snprintf(val, sizeof(val), "%s %u/%u", T(S_CARD_BADGES),
           (unsigned)(pet.badges < GYM_BADGES ? pet.badges : GYM_BADGES),
           (unsigned)GYM_BADGES);
  gfx->setTextColor(CARD_INK);
  setSize(2);
  setCur(centerX(val, 2), 226);
  printT(val);
  for (uint8_t i = 0; i < GYM_BADGES; i++) {
    int bx = TCARD_BX0 + (i % 4) * 82, by = TCARD_BY0 + (i / 4) * 54;
    if (pet.hasBadge(i)) {
      drawBadge(pet.region, i, bx + 24, by + 24, 1);
    } else {
      gfx->drawCircle(bx + 24, by + 24, 17, CARD_DIM);
      gfx->drawCircle(bx + 24, by + 24, 16, CARD_DIM);
    }
  }
}

// pagina 2: medallas con etiqueta descriptiva
// LOGROS. Ya no caben los 30 de golpe: van de ocho en ocho, con los
// puntitos de pagina arriba y una flecha abajo para pasar.
#define ACH_PER_PAGE 8
// La flecha de pasar pagina. Estaba en 376, justo encima del "tap: back"
// del pie (y=390), y su banda de toque lo tapaba: tocar "tap: back" pasaba
// de pagina en vez de volver. Las filas acaban en 310.
#define ACH_ARROW_Y 334
#define ACH_BAND_X0 133
#define ACH_BAND_X1 333
#define ACH_BAND_Y0 (ACH_ARROW_Y - 14)
#define ACH_BAND_Y1 (ACH_ARROW_Y + 30)
uint8_t achPage = 0;

void renderCardMedals() {
  int got = 0;
  for (int i = 0; i < MED_COUNT; i++)
    if (pet.hasMedal(1u << i)) got++;
  const uint8_t paginas = (MED_COUNT + ACH_PER_PAGE - 1) / ACH_PER_PAGE;
  if (achPage >= paginas) achPage = 0;
  char head[24];
  snprintf(head, sizeof(head), T(S_MEDALS_FMT), got, MED_COUNT);
  gfx->setTextColor(CARD_INK);
  setSize(3);
  setCur(centerX(head, 3), 48);
  printT(head);
  // En que pagina estas, en puntitos: con cuatro paginas, ver donde
  // estas dice mas que un "3/4" suelto.
  for (uint8_t p = 0; p < paginas; p++) {
    int px = CX - (paginas - 1) * 9 + p * 18;
    if (p == achPage) gfx->fillCircle(px, 88, 4, CARD_INK);
    else              gfx->drawCircle(px, 88, 4, CARD_DIM);
  }

  const int base = achPage * ACH_PER_PAGE;
  for (int k = 0; k < ACH_PER_PAGE; k++) {
    const int i = base + k;
    if (i >= MED_COUNT) break;
    int x = 28 + (k % 2) * 206, y = 104 + (k / 2) * 54;
    bool g = pet.hasMedal(1u << i);
    // La que FALTA va vacia con su filo, no rellena: el texto sobre el
    // relleno de pastilla daba 1,50 de contraste y no se leia. Vacia se lee
    // y ademas dice mejor "esta todavia no", que es lo que significa.
    gfx->fillRoundRect(x, y, 196, 44, 10, g ? UI_BAR_OK : CARD_PAPER);
    if (!g) gfx->drawRoundRect(x, y, 196, 44, 10, CARD_DIM);
    // A FITA da conquista (PokeSprite, /por/fitas.bin): com cor quando
    // ja e sua, uma sombra apagada quando falta. Sem cartao, a marca de
    // sempre.
    const Art *fita = fitasSd.get(0, (uint8_t)i);
    if (fita) {
      Art c = *fita;
      static uint16_t apagada[256];
      if (!g) {
        for (uint16_t k = 0; k < 256; k++) apagada[k] = CARD_DIM;
        c.pal = apagada;
      }
      drawArt(c, x + 22, y + 22 + c.h / 2, 1);
    } else if (g) {  // marca de conseguida
      gfx->fillCircle(x + 22, y + 22, 11, CARD_PAPER);
      gfx->setTextColor(UI_BAR_OK);
      setSize(2);
      setCur(x + 16, y + 13);
      printT("v");
    }
    // El texto se encoge si no cabe: los nuevos son mas largos que
    // "Lv.10" ("30 DAY STREAK", "BEAT SINNOH") y a tamano 2 se salian
    // de la pastilla por la derecha.
    const char *d = medalDesc(i);
    int ts = textW(d, 2) <= 196 - 52 ? 2 : 1;
    gfx->setTextColor(g ? CARD_PAPER : CARD_SOFT);
    setSize((uint8_t)ts);
    setCur(x + 44, y + (ts == 2 ? 14 : 18));
    printT(d);
  }

  // La flecha de pasar pagina.
  if (paginas > 1) {
    gfx->fillTriangle(CX, ACH_ARROW_Y + 14, CX - 12, ACH_ARROW_Y,
                      CX + 12, ACH_ARROW_Y, CARD_INK);
  }
}

// Tocar la flecha (o cualquier sitio de su banda) pasa de pagina.
void cardMedalsTap(int16_t x, int16_t y) {
  const uint8_t paginas = (MED_COUNT + ACH_PER_PAGE - 1) / ACH_PER_PAGE;
  if (paginas < 2) { cardOpen = false; return; }
  if (y >= ACH_BAND_Y0 && y <= ACH_BAND_Y1 && x >= ACH_BAND_X0 && x <= ACH_BAND_X1) {
    achPage = (uint8_t)((achPage + 1) % paginas);
    sfxPlay(SFX_TAP);
    return;
  }
  // un pelo al lado de la flecha no cierra
  if (!lejosDe(x, y, ACH_BAND_X0, ACH_BAND_Y0, ACH_BAND_X1, ACH_BAND_Y1)) return;
  cardOpen = false;
}

// pagina 3: progreso (nivel, evolucion, descuidos) â€” saca a la luz mecanicas
// que antes eran invisibles (cuanto falta para subir/evolucionar y por que)
void renderCardProgress() {
  const DexEntry &d = DEX_TBL[pet.speciesId];
  gfx->setTextColor(CARD_INK);
  setSize(3);
  setCur(centerX(T(S_PROGRESS), 3), 44);
  printT(T(S_PROGRESS));

  // nivel grande
  char lv[10];
  snprintf(lv, sizeof(lv), T(S_LVL_FMT), pet.level());
  setSize(5);
  setCur(centerX(lv, 5), 86);
  printT(lv);

  // Si va por encima del techo de la region, hay que DECIRLO: si no, entra
  // a un gimnasio con su Lv.70 y de pronto pelea como un Lv.11 sin que nada
  // en pantalla lo hubiera avisado.
  if (pet.overCap()) {
    char cp[40];
    snprintf(cp, sizeof(cp), T(S_CAP_FMT), (unsigned)pet.levelCap());
    gfx->setTextColor(UI_BAR_BAD);
    setSize(2);
    setCur(centerX(cp, 2), 134);
    printT(cp);
  }

  // Barra al siguiente nivel. Antes era tiempo ("faltan N minutos"); ahora es
  // EXPERIENCIA, que es de donde sale el nivel desde que se separo de la edad.
  int bx = 93, bw = 280, by = 158, bh = 22;
  gfx->fillRoundRect(bx, by, bw, bh, 6, CARD_DIM);
  int fw = (int)((bw - 4) * pet.levelProgress());
  if (fw > 0) gfx->fillRoundRect(bx + 2, by + 2, fw, bh - 4, 5, UI_BAR_OK);
  // 48 y no 32: una fila UTF-8 mete tres bytes por caracter, y cortar
  // a mitad de una secuencia deja un glifo que la fuente no sabe pintar
  // (la misma razon que el buffer del dialogo de soltar).
  char nx[48];
  if (pet.level() >= LEVEL_CAP) {
    snprintf(nx, sizeof(nx), "MAX");
  } else {
    uint32_t falta = Pet::expForLevel(pet.level() + 1) - pet.exp;
    snprintf(nx, sizeof(nx), T(S_EXP_NEXT_FMT),
             (unsigned long)falta, pet.level() + 1);
  }
  gfx->setTextColor(CARD_INK);
  setSize(2);
  setCur(centerX(nx, 2), by + 32);
  printT(nx);

  // estado de evolucion
  gfx->setTextColor(CARD_SOFT);
  setCur(centerX(T(S_EVO_LABEL), 2), 230);
  printT(T(S_EVO_LABEL));
  char evoBuf[48];
  const char *evo;
  uint16_t evoCol = CARD_INK;
  if (d.evolvesTo == 0) {
    evo = T(S_FINAL_FORM);
  } else if (needsStone(pet.speciesId)) {
    // Este no sube por nivel NUNCA: hay que decir QUE objeto le hace falta,
    // o el jugador se queda esperando un nivel que no llega. Se nombra el
    // objeto de verdad buscandolo por su tipo en la tabla.
    uint8_t tipo = stoneTypeFor(pet.speciesId);
    const char *obj = T(S_UMA_PEDRA);   // Eevee y cia: hay varias, elige el jugador
    if (tipo != STONE_MANY)
      for (uint8_t i = 1; i < ITEM_COUNT; i++)
        if (ITEM_TBL[i].fx == IT_STONE && ITEM_TBL[i].arg == tipo) {
          obj = ITEM_TBL[i].name;
          break;
        }
    snprintf(evoBuf, sizeof(evoBuf), T(S_PRECISA_FMT), obj);
    evo = evoBuf;
  } else {
    int needed = d.evolveLevel;
    if (pet.level() >= needed) {
      evo = T(S_EVO_READY);
      evoCol = UI_BAR_OK;
    } else {
      snprintf(evoBuf, sizeof(evoBuf), T(S_EVO_IN_FMT), needed - pet.level());
      evo = evoBuf;
    }
  }
  gfx->setTextColor(evoCol);
  setCur(centerX(evo, 2), 256);
  printT(evo);

  // descuidos (retrasan la evolucion)
  char ms[24];
  snprintf(ms, sizeof(ms), T(S_MISTAKES_FMT), pet.careMistakes);
  gfx->setTextColor(pet.careMistakes > 0 ? UI_BAR_BAD : CARD_INK);
  setCur(centerX(ms, 2), 312);
  printT(ms);
}

// Flechas de la lista, a la derecha, donde el circulo es mas ancho. Se pintan
// apagadas en los extremos para que se vea que ya no hay mas.
static void cardArrows(uint8_t total) {
  // En el banco se ven CINCO filas, no seis: si aqui se cuentan seis, la
  // flecha de bajar se apaga con una fila todavia por ver.
  uint8_t filas = bankView ? BANK_ROWS : CARD_ROWS;
  bool arriba = cardScroll > 0;
  bool abajo = (uint16_t)(cardScroll + filas) < total;
  gfx->fillTriangle(414, 176, 402, 196, 426, 196, arriba ? CARD_INK : CARD_DIM);
  gfx->fillTriangle(414, 300, 402, 280, 426, 280, abajo ? CARD_INK : CARD_DIM);
}

static bool cardArrowTap(int x, int y, uint8_t total) {
  if (x < 386) return false;
  if (y >= 150 && y < 233) {                       // arriba
    if (cardScroll) cardScroll--;
    return true;
  }
  if (y >= 233 && y <= 326) {                      // abajo
    uint8_t filas = bankView ? BANK_ROWS : CARD_ROWS;
    if ((uint16_t)(cardScroll + filas) < total) cardScroll++;
    return true;
  }
  return false;
}

// Que fila de la lista se ha tocado, o 0xFF si el toque no cayo en ninguna.
static uint8_t cardRowAt(int x, int y) {
  if (x < 62 || x > 378) return 0xFF;
  // ANTES de dividir: con y por encima de la primera fila la resta da
  // negativo y la division entera trunca hacia CERO, no hacia abajo, asi
  // que los 33 px de encima de la lista contaban como la fila 0.
  if (y < CARD_ROW_Y) return 0xFF;
  int r = (y - CARD_ROW_Y) / CARD_ROW_H;
  if (r < 0 || r >= CARD_ROWS) return 0xFF;
  if (y - (CARD_ROW_Y + r * CARD_ROW_H) > 32) return 0xFF;  // el hueco entre filas
  return (uint8_t)r;
}

// Tambien SANEA `movesSlot`: si el suplente que se estaba mirando se fue al
// banco (o paso a titular) mientras la pagina estaba abierta, se vuelve al
// titular en vez de enseÃ±ar un hueco vacio.
static MovesView movesView() {
  MovesView v;
  if (movesSlot >= TEAM_SLOTS || movesSlot == pet.mainSlot ||
      !pet.team[movesSlot].used()) {
    movesSlot = MOVES_MAIN;
    v.species = pet.speciesId;
    v.level = pet.level();
    for (uint8_t i = 0; i < 4; i++) {
      v.slotMv[i] = pet.ppMove[i];
      v.slotPp[i] = pet.ppLeft[i];
    }
    return v;
  }
  const StoredMon &m = pet.team[movesSlot];
  v.species = m.species;
  v.level = Pet::levelOf(m);
  pet.teamMoves(movesSlot, v.slotMv, v.slotPp);
  v.owner = m.nick[0] ? m.nick : dexName(m.species);
  return v;
}

// Lo que PUEDE llevar: lo que sabe por nivel, y detras las MT que tienes
// y que esta especie puede aprender.
//
// Van en la MISMA lista a proposito, y no en una pantalla aparte: el
// gesto de tocar para equipar ya existe y funciona igual para las dos,
// asi que una pantalla nueva solo anadiria sitios donde perderse. Lo
// que las distingue es una marca "TM" a la derecha.
//
// `desdeTM` dice a partir de que indice empiezan, para poder marcarlas.
static uint8_t movesConTMs(int16_t species, uint16_t level, uint16_t *out,
                           uint8_t max, uint8_t *desdeTM) {
  uint8_t n = movesKnown(species, level, out, max);
  *desdeTM = n;
  for (uint16_t t = 0; t < TM_COUNT && n < max; t++) {
    if (!pet.hasTM(t) || !tmFits(species, t)) continue;
    uint16_t mv = tmMove(t);
    bool ya = false;
    for (uint8_t k = 0; k < n; k++) if (out[k] == mv) ya = true;
    if (!ya) out[n++] = mv;
  }
  return n;
}

void cardMovesTap(int x, int y) {
  MovesView v = movesView();
  bool mio = (movesSlot == MOVES_MAIN);
  uint16_t todos[80];
  uint8_t desdeTM = 0;
  uint8_t n = movesConTMs(v.species, v.level, todos, 80, &desdeTM);
  if (cardArrowTap(x, y, n)) return;

  // los cuatro huecos de arriba: tocar uno lo suelta
  for (uint8_t i = 0; i < 4; i++) {
    int sx = SLOT_X + (i & 1) * (SLOT_W + 8);
    int sy = SLOT_Y + (i >> 1) * (SLOT_H + 5);
    if (x < sx || x > sx + SLOT_W || y < sy || y > sy + SLOT_H) continue;
    if (v.slotMv[i]) {
      moveShown = v.slotMv[i];   // se cuenta ANTES de soltarlo
      if (mio) pet.unequipMove(i);
      else     pet.teamUnequipMove(movesSlot, i);
      sfxPlay(SFX_TAP);
    }
    return;
  }

  // La ficha del golpe, abajo, pasa de pagina al tocarla. Va ANTES del
  // 0xFF porque esa zona cae fuera de las filas y, sin esto, tocar el
  // texto para seguir leyendo CERRABA la pantalla.
  if (moveShown && x >= 72 && x <= 376 && y >= 320 && y <= 398) {
    if (moveDescFin && *moveDescFin) moveDescPtr = moveDescFin;
    else moveDescPtr = moveDesc(moveShown);   // al final, vuelve al principio
    sfxPlay(SFX_TAP);
    return;
  }

  uint8_t r = cardRowAt(x, y);
  // Aqui se dibujan CINCO filas (ver renderCardMoves), y cardRowAt da seis.
  // La sexta caia sobre el "tap a move to see what it does": tocar ese
  // letrero ponia o quitaba un golpe que no se veia.
  if (r != 0xFF && r >= CARD_ROWS - 1) r = 0xFF;
  if (r == 0xFF) {
    // Fallar un blanco NO cierra: solo arriba (el titulo) o abajo.
    if (!cardCierraEn(y, moveShown ? 398 : 326)) return;
    // Mirando a un suplente, salir devuelve a la lista del equipo, que es de
    // donde se vino; en el titular sigue cerrando la ficha.
    if (mio) cardOpen = false;
    else { movesSlot = MOVES_MAIN; cardPage = 4; cardScroll = 0; }
    return;
  }
  uint8_t i = (uint8_t)(cardScroll + r);
  if (i >= n) return;
  moveShown = todos[i];   // tocar equipa Y explica: un solo gesto
  // Un solo gesto para las dos cosas: si ya lo lleva, lo suelta; si no, lo
  // pone en el primer hueco libre. Con los cuatro llenos hay que soltar uno
  // primero, y eso se ve en la pantalla sin que nadie lo explique.
  uint8_t puesto = 4;
  for (uint8_t k = 0; k < 4; k++) if (v.slotMv[k] == todos[i]) { puesto = k; break; }
  if (puesto < 4) {
    if (mio) pet.unequipMove(puesto);
    else     pet.teamUnequipMove(movesSlot, puesto);
    sfxPlay(SFX_TAP);
  } else if (mio ? pet.equipMove(todos[i])
                 : pet.teamEquipMove(movesSlot, todos[i])) {
    sfxPlay(SFX_PLAY);
  } else {
    sfxPlay(SFX_DENY);      // los cuatro ocupados
  }
}

// La ficha de Pokedex en el idioma activo. Mismo reparto que moveDesc():
// el espanol sale del PBS (gratis) y el portugues esta escrito a mano en
// tools/pt_dex.py, ahi indexado por NUMERO de Pokedex, que a diferencia de
// la fila de un golpe es la identidad del bicho y no se mueve.
static const char *dexDesc(int16_t dex) {
  if (dex < 1 || dex > DEX_COUNT) return "";
  if (gLang == LANG_PT && DEX_TEXT_PT[dex]) return DEX_TEXT_PT[dex];
  if (gLang == LANG_ES && DEX_TEXT_ES[dex]) return DEX_TEXT_ES[dex];
  return DEX_TEXT[dex];
}

// Que hace el golpe, en el idioma activo.
//
// El espanol sale del PBS del Anil (gratis: es de donde se traduce el
// ingles) y el portugues esta escrito a mano en tools/pt_golpes.py, que
// lo indexa por SLUG y no por fila. Lo que aun no esta traducido vale
// nullptr y cae al ingles, que es el idioma por defecto: un golpe sin
// traducir se lee, no se queda en blanco.
//
// Los otros cinco idiomas no tienen estas frases y tambien caen al
// ingles. Son 200 KB de texto por idioma (2225 frases entre golpes,
// Pokedex, objetos y habilidades), asi que cada idioma nuevo es una
// decision de sitio, no un anadido gratis.
static const char *moveDesc(uint16_t mv) {
  if (mv >= MOVE_COUNT) return "";
  if (gLang == LANG_PT && MOVE_DESC_PT[mv]) return MOVE_DESC_PT[mv];
  if (gLang == LANG_ES && MOVE_DESC_ES[mv]) return MOVE_DESC_ES[mv];
  return MOVE_DESC[mv];
}

// Con la ficha abierta solo caben CUATRO filas: la lista va de y=146 a
// 146+6*34=350 y la ficha empieza en 300. Sin esto se pisaban.
//
// Va AQUI y no arriba del fichero: el generador de prototipos del
// simulador los mete antes de la primera funcion, y poniendola la
// primera arrastraba el bloque por encima de los tipos.
static inline uint8_t bagFilas() { return bagShown ? 4 : CARD_ROWS; }

// La ficha del objeto mirado: su caja y el boton de abajo.
#define BAGF_X 72
#define BAGF_Y 300
#define BAGF_W 304
#define BAGF_H 110
// Os botoes, dentro da caixa. Eram 30 px (~3 mm) e so o desenho pegava:
// "os botoes de pedras use/hold tao daquele jeito horrivel de apertar".
// Agora 44 px (~4 mm), e o toque pega com folga em volta (BAGB_TOQUE_*):
// um pouco acima ainda e o botao (nao vira a pagina do texto), um pouco
// abaixo da caixa tambem (nao fecha), e os lados passam da caixa.
#define BAGB_Y 356
#define BAGB_H 44
#define BAGB_TOQUE_Y0 (BAGB_Y - 10)
#define BAGB_TOQUE_Y1 (BAGF_Y + BAGF_H + 8)
#define BAGB_TOQUE_X0 (BAGF_X - 18)
#define BAGB_TOQUE_X1 (BAGF_X + BAGF_W + 18)
// o "tap: back" com a ficha aberta: abaixo do foso, onde tocar FECHA
#define BAGF_VOLTA_Y (BAGB_TOQUE_Y1 + UI_FOSO + 4)

void cardBagTap(int x, int y) {
  // El boton de ordenar va ANTES que nada: esta por encima de la lista y
  // si se mira despues, el toque se lo come la fila de arriba.
  if (y >= BAG_SORT_Y0 - 2 && y <= BAG_SORT_Y1 + 6 && x >= 120 && x <= 346) {
    pet.cycleBagSort();
    cardScroll = 0;           // la lista es otra: volver arriba
    sfxPlay(SFX_TAP);
    return;
  }
  uint8_t n = pet.pocketCount(bagPocket);

  // Los BOTONES, antes que nada: caen fuera de las filas y sin esto el
  // toque se iria por el 0xFF y cerraria la ficha. Dos: a la izquierda
  // lo de siempre (ponerselo el titular / USAR el de troca, que evoluciona
  // ya), a la derecha DAR a cualquiera del equipo.
  if (bagShown && x >= BAGB_TOQUE_X0 && x <= BAGB_TOQUE_X1 &&
      y >= BAGB_TOQUE_Y0 && y <= BAGB_TOQUE_Y1) {
    if (x >= BAGF_X + BAGF_W / 2) {
      darItemId = bagShown;
      bagShown = 0;
      cardPage = 4;
      bankView = false;
      cardScroll = 0;
      sfxPlay(SFX_TAP);
    } else if (ITEM_TBL[bagShown].pocket == BP_EVO) {
      if (pet.useStone(bagShown)) { bagShown = 0; cardOpen = false; }
      else sfxPlay(SFX_DENY);
    } else if (pet.heldItem == bagShown) { pet.holdItem(0); sfxPlay(SFX_TAP); }
    else                                 { pet.holdItem(bagShown); sfxPlay(SFX_PLAY); }
    return;
  }
  // y el resto de la ficha pasa de pagina, como la de los golpes
  if (bagShown && x >= BAGF_X && x <= BAGF_X + BAGF_W &&
      y >= BAGF_Y && y < BAGB_TOQUE_Y0) {
    if (bagDescFin && *bagDescFin) bagDescPtr = bagDescFin;
    else bagDescPtr = ITEM_DESC[bagShown];
    sfxPlay(SFX_TAP);
    return;
  }

  if (cardArrowTap(x, y, n)) return;

  if (y >= 66 && y <= 104) {                     // pestanas (27 px dibujadas)
    // La MISMA cuenta que renderCardBag: si se queda con el 42 y el 78 de
    // antes, tocas una pestana y se abre la de al lado.
    int c = (int)sqrtf((float)(BAG_TAB_R * BAG_TAB_R - 161 * 161));
    int x0 = CX - c + 6, ancho = (2 * c - 12) / BP_COUNT;
    int b = ancho > 0 ? (x - x0) / ancho : -1;
    if (b >= 0 && b < BP_COUNT) {
      bagPocket = (uint8_t)b;
      cardScroll = 0;
      sfxPlay(SFX_TAP);
    }
    return;
  }

  uint8_t r = cardRowAt(x, y);
  if (r == 0xFF) {
    // Fallar un blanco NO cierra: solo arriba (el titulo) o abajo.
    if (cardCierraEn(y, bagShown ? BAGB_TOQUE_Y1 : CARD_ROW_Y + CARD_ROWS * CARD_ROW_H))
      cardOpen = false;
    return;
  }
  if (r >= bagFilas()) return;   // esa fila no se esta dibujando
  uint8_t i = (uint8_t)(cardScroll + r);
  if (i >= n) return;
  uint8_t id = pet.pocketItem(bagPocket, i);
  if (!id) return;

  if (ITEM_TBL[id].pocket == BP_HELD) {
    // Mirar, no equipar. El boton de abajo es el que pone y quita.
    bagShown = id;
    sfxPlay(SFX_TAP);
  } else if (ITEM_TBL[id].pocket == BP_BATTLE) {
    sfxPlay(pet.useItem(id) ? SFX_PLAY : SFX_DENY);
  } else if (ITEM_TBL[id].pocket == BP_EVO && evoItemDeTroca(ITEM_TBL[id].arg)) {
    // Os de TROCA (Metal Coat...): usar evolui o titular ja, mas tambem
    // se DAO a alguem do time, para ele chegar segurando numa troca. Antes
    // tocar ja usava, e nao havia como segurar.
    bagShown = id;
    sfxPlay(SFX_TAP);
  } else if (ITEM_TBL[id].pocket == BP_EVO) {
    // Para los que evolucionan asi, ESTE es el unico camino. Se cierra la
    // ficha al usarla para que se vea la transformacion en la pantalla
    // principal, que es donde vive la animacion.
    if (pet.useStone(id)) cardOpen = false;
    else sfxPlay(SFX_DENY);   // a este bicho esa piedra no le hace nada
  } else {
    sfxPlay(SFX_DENY);   // las bolas solo valen dentro de un combate
  }
}

// pagina 2: los golpes. Arriba los cuatro que lleva puestos, abajo TODO lo
// que sabe. Tocar uno de la lista lo pone; tocar uno puesto lo quita. Sin
// modos ni confirmaciones: con cuatro huecos y una pantalla redonda, cuanto
// menos estado tenga la interaccion, mejor.
void renderCardMoves() {
  MovesView v = movesView();
  gfx->setTextColor(CARD_INK);
  setSize(3);
  setCur(centerX(T(S_CARD_MOVES), 3), 34);
  printT(T(S_CARD_MOVES));
  // De quien son, si no son los del titular. Sin esto, al llegar desde la
  // lista del equipo no hay forma de saber a cual estas mirando.
  if (v.owner) {
    char de[20];
    snprintf(de, sizeof(de), "%s  Lv%u", v.owner, v.level);
    gfx->setTextColor(CARD_SOFT);
    setSize(1);
    setCur(centerX(de, 1), 58);
    printT(de);
  }

  uint16_t todos[80];
  uint8_t desdeTM = 0;
  uint8_t n = movesConTMs(v.species, v.level, todos, 80, &desdeTM);

  // Los cuatro huecos, en 2x2 y no en fila. Con cuatro en fila cada caja se
  // queda en 80 px = 11 caracteres, y hay nombres de 15 (STOMPINGTANTRUM):
  // salian cortados. En 2x2 la caja tiene 158 y no se corta ninguno.
  for (uint8_t i = 0; i < 4; i++) {
    int x = SLOT_X + (i & 1) * (SLOT_W + 8);
    int y = SLOT_Y + (i >> 1) * (SLOT_H + 5);
    bool lleno = v.slotMv[i] != 0;
    gfx->fillRoundRect(x, y, SLOT_W, SLOT_H, 8, lleno ? CARD_DIM : CARD_PAPER);
    gfx->drawRoundRect(x, y, SLOT_W, SLOT_H, 8, CARD_INK);
    if (!lleno) continue;
    const MoveEntry &m = MOVE_TBL[v.slotMv[i]];
    // la insignia del tipo en lugar de la franja de color: mismo sitio, pero
    // el color por si solo no dice nada a quien no se sepa la tabla
    drawTypeBadge(x + 4 + TYPE_BADGE / 2, y + SLOT_H / 2, m.type);
    int tx = x + 6 + TYPE_BADGE;
    gfx->setTextColor(CARD_INK);
    setSize(1);
    setCur(tx, y + 5);
    printT(m.name);
    char pp[12];
    snprintf(pp, sizeof(pp), "PP %u/%u", v.slotPp[i], movePP(v.slotMv[i]));
    setCur(tx, y + 16);
    printT(pp);
  }

  // La lista de todo lo que sabe. Aqui son CINCO filas y no seis: la
  // sexta se cambio por la franja de descripcion, que vale mas que una
  // linea mas de nombres.
  const uint8_t FILAS = CARD_ROWS - 1;
  if (cardScroll + FILAS > n) cardScroll = (uint8_t)(n > FILAS ? n - FILAS : 0);
  for (uint8_t r = 0; r < FILAS; r++) {
    uint8_t i = (uint8_t)(cardScroll + r);
    if (i >= n) break;
    const MoveEntry &m = MOVE_TBL[todos[i]];
    int y = CARD_ROW_Y + r * CARD_ROW_H;
    bool puesto = false;
    for (uint8_t k = 0; k < 4; k++) if (v.slotMv[k] == todos[i]) puesto = true;
    if (puesto) gfx->fillRoundRect(62, y, 316, 32, 8, CARD_DIM);
    drawTypeBadge(68 + TYPE_BADGE / 2, y + 16, m.type);
    gfx->setTextColor(CARD_INK);
    setSize(2);
    setCur(70 + TYPE_BADGE + 6, y + 8);
    printT(m.name);
    setSize(1);
    char der[16];
    if (m.power) snprintf(der, sizeof(der), "%u", m.power);
    else         snprintf(der, sizeof(der), "--");
    setCur(352 - (int)strlen(der) * LATIN_ADV, y + 12);
    printT(der);
    // Las que vienen de una MT llevan su marca: si no, el jugador no
    // tendria forma de saber por que de pronto hay golpes nuevos en la
    // lista de un bicho que no ha subido de nivel.
    if (i >= desdeTM) {
      gfx->setTextColor(CARD_SOFT);
      setCur(352 - (int)strlen(der) * LATIN_ADV - 22, y + 2);
      printT("TM");
      gfx->setTextColor(CARD_INK);
    }
  }
  drawMoveInfo();
  cardArrows(n);
}

// Que hace el ultimo golpe que se toco, mas sus numeros. Va SIEMPRE
// visible y no en un popup: tocar ya sirve para equipar, y meterle un
// segundo gesto encima seria pedirle al jugador que lo adivine.
void drawMoveInfo() {
  if (!moveShown || moveShown >= MOVE_COUNT) {
    gfx->setTextColor(CARD_SOFT);
    setSize(1);
    setCur(centerX(T(S_TOQUE_GOLPE), 1), 336);
    printT(T(S_TOQUE_GOLPE));
    return;
  }
  const MoveEntry &m = MOVE_TBL[moveShown];
  // La caja crece a 78 de alto (320..398) y se estrecha a 304 de ancho.
  // Lo segundo no es estetica: baja hasta y=398, y a esa altura la
  // cuerda del circulo ya no da los 316 de antes -- se habria salido por
  // las esquinas de abajo.
  //
  // El texto va a TALLA 2 y PAGINADO, que es la unica forma de que
  // quepa: las descripciones tienen 77 caracteres de mediana y llegan a
  // 118, y a talla 2 entran 25 por linea. Se midieron las 717: a 25x3
  // se cortaban 489. Con dos lineas por pagina y la flechita, no se
  // corta ninguna.
  gfx->fillRoundRect(72, 320, 304, 78, 8, CARD_PAPER);
  gfx->drawRoundRect(72, 320, 304, 78, 8, typeColor(m.type));
  gfx->setTextColor(CARD_INK);
  setSize(1);
  // la ficha tecnica en una linea: tipo, fisico/especial, precision
  char tec[40];
  // ACC 0 es "nunca falla", no "no acierta nunca". Escrito tal cual seria
  // lo contrario de lo que pasa, asi que esos llevan la raya.
  if (m.acc == 0)
    snprintf(tec, sizeof(tec), "%s  %s  ACC --", typeName(m.type),
             m.power ? (m.phys ? "PHYS" : "SPEC") : "SUPPORT");
  else
    snprintf(tec, sizeof(tec), "%s  %s  ACC %u", typeName(m.type),
             m.power ? (m.phys ? "PHYS" : "SPEC") : "SUPPORT", m.acc);
  setCur(centerX(tec, 1), 326);
  printT(tec);
  // Al cambiar de golpe se vuelve al principio del texto.
  if (moveDescDe != moveShown) {
    moveDescDe = moveShown;
    moveDescPtr = moveDesc(moveShown);
  }
  moveDescFin = drawWrapped(moveDescPtr, 344, 25, 18, 2, 2);
  if (moveDescFin && *moveDescFin) flechaMas(358, 386, CARD_SOFT);
}

// pagina 3: la bolsa, con sus bolsillos. Tocar la pestana cambia de bolsillo;
// tocar un objeto lo pone (si es de llevar) o lo usa (si es de gastar).
void renderCardBag() {
  gfx->setTextColor(CARD_INK);
  setSize(3);
  setCur(centerX(T(S_CARD_BAG), 3), 34);
  printT(T(S_CARD_BAG));

  // Pestanas. NO caben "justas": iban de x=42 a 432 y a su altura (y=72) la
  // cuerda del circulo solo da de 65 a 401, asi que la primera y la ultima
  // salian cortadas por el cristal. Se calcula el ancho a partir de la
  // cuerda de verdad en vez de darlo por bueno.
  {
    int c = (int)sqrtf((float)(BAG_TAB_R * BAG_TAB_R - 161 * 161));  // y=72
    int x0 = CX - c + 6, ancho = (2 * c - 12) / BP_COUNT;
    for (uint8_t b = 0; b < BP_COUNT; b++) {
      int x = x0 + b * ancho, y = 72, w = ancho - 4;
      bool act = (b == bagPocket);
      gfx->fillRoundRect(x, y, w, 26, 7, act ? CARD_INK : CARD_PAPER);
      gfx->drawRoundRect(x, y, w, 26, 7, CARD_INK);
      gfx->setTextColor(act ? CARD_PAPER : CARD_INK);
      setSize(1);
      // pela largura de verdade: em japones e coreano strlen conta bytes
      const char *nb = T((StrId)(S_BOLSO_0 + b));
      setCur(x + (w - textW(nb, 1)) / 2, y + 9);
      printT(nb);
    }
  }

  // EL BOTON DE ORDENAR. Entre los bolsillos (y=72..98) y la primera fila
  // (y=146) hay sitio de sobra, asi que no hay que quitarle filas a la
  // lista para meterlo.
  {
    static const StrId ORDEN[3] = { S_ORD_BOLSA, S_ORD_AZ, S_ORD_TIPO };
    const char *et = T(ORDEN[pet.bagSort < 3 ? pet.bagSort : 0]);
    // pela largura de verdade: em japones e coreano strlen conta bytes
    int w = (int)textW(et, 2) + 20;
    int x = CX - w / 2;
    gfx->fillRoundRect(x, BAG_SORT_Y0, w, 26, 7, CARD_PAPER);
    gfx->drawRoundRect(x, BAG_SORT_Y0, w, 26, 7, CARD_INK);
    gfx->setTextColor(CARD_INK);
    setSize(2);
    setCur(CX - (int)textW(et, 2) / 2, BAG_SORT_Y0 + 6);
    printT(et);
  }

  uint8_t n = pet.pocketCount(bagPocket);
  const uint8_t FIL = bagFilas();
  if (cardScroll + FIL > n) cardScroll = (uint8_t)(n > FIL ? n - FIL : 0);
  if (!n) {
    gfx->setTextColor(CARD_SOFT);
    setSize(2);
    setCur(centerX(T(S_VAZIO), 2), 200);
    printT(T(S_VAZIO));
    return;
  }
  for (uint8_t r = 0; r < FIL; r++) {
    uint8_t i = (uint8_t)(cardScroll + r);
    if (i >= n) break;
    uint8_t id = pet.pocketItem(bagPocket, i);
    int y = CARD_ROW_Y + r * CARD_ROW_H;
    bool puesto = (id == pet.heldItem);
    if (puesto) gfx->fillRoundRect(62, y, 316, 32, 8, CARD_DIM);
    // El icono del Anil. Los 67 estan, pero se comprueba el nullptr: si
    // manana entra un objeto nuevo sin dibujo, la lista no debe caerse.
    if (id < ITEM_COUNT && ART_ITEM[id])
      drawArt(*ART_ITEM[id], 84, y + 16 + ART_ITEM[id]->h / 2, 1);
    gfx->setTextColor(CARD_INK);
    setSize(2);
    setCur(106, y + 8);
    printT(ITEM_TBL[id].name);
    // "HELD" y no "x0": el que se lleva puesto esta descontado de la
    // bolsa, asi que su cuenta es cero y el jugador no entendia que esa
    // linea era precisamente la del objeto que tiene equipado -- ni que
    // tocarla era la forma de quitarselo.
    char der[24];
    if (puesto) snprintf(der, sizeof(der), "%s", T(S_EQUIPADO));
    else if (ITEM_TBL[id].fx == IT_KEY) der[0] = 0;   // chave: sem contagem
    else        snprintf(der, sizeof(der), "x%u", pet.bag[id]);
    setSize(1);
    setCur(360 - (int)strlen(der) * LATIN_ADV, y + 12);
    printT(der);
  }
  cardArrows(n);

  // ---- la ficha del objeto que se esta mirando ----
  // Igual que la del golpe: nombre, texto a talla 2 paginado con su
  // flechita, y abajo el boton. Lo que NO hace es equipar al tocar la
  // lista, que era lo que dejaba al jugador con algo puesto sin saber
  // que era ni como quitarlo.
  if (bagShown && (ITEM_TBL[bagShown].pocket == BP_HELD ||
                   ITEM_TBL[bagShown].pocket == BP_EVO)) {
    gfx->fillRoundRect(BAGF_X, BAGF_Y, BAGF_W, BAGF_H, 8, CARD_PAPER);
    gfx->drawRoundRect(BAGF_X, BAGF_Y, BAGF_W, BAGF_H, 8, CARD_SOFT);
    gfx->setTextColor(CARD_INK);
    if (bagDescDe != bagShown) {
      bagDescDe = bagShown;
      bagDescPtr = ITEM_DESC[bagShown];
    }
    bagDescFin = drawWrapped(bagDescPtr, BAGF_Y + 8, 25, 18, 2, 2);
    if (bagDescFin && *bagDescFin) flechaMas(BAGF_X + BAGF_W - 16,
                                             BAGF_Y + 46, CARD_SOFT);
    // los botones, con lo que VA A PASAR escrito
    bool evo = ITEM_TBL[bagShown].pocket == BP_EVO;
    bool puesto = !evo && (pet.heldItem == bagShown);
    const int bw = (BAGF_W - 24) / 2;
    const int bx0 = BAGF_X + 8, bx1 = BAGF_X + 16 + bw;
    gfx->fillRoundRect(bx0, BAGB_Y, bw, BAGB_H, 8, puesto ? UI_BAR_BAD : UI_BAR_OK);
    gfx->fillRoundRect(bx1, BAGB_Y, bw, BAGB_H, 8, C565(0x2c, 0x4a, 0x6e));
    gfx->setTextColor(CARD_PAPER);
    const char *et = evo ? T(S_BAG_USAR) : puesto ? T(S_TIRAR) : T(S_SEGURAR);
    setSize(2);
    setCur(bx0 + (bw - textW(et, 2)) / 2, BAGB_Y + 14);
    printT(et);
    gfx->setTextColor(UI_WHITE);
    const char *dar = T(S_BAG_DAR);
    setCur(bx1 + (bw - textW(dar, 2)) / 2, BAGB_Y + 14);
    printT(dar);
  }
}

// Una fila de bicho: icono, nombre, nivel y barra de PS. Vale para el equipo
// y para el banco, que solo se diferencian en que se puede hacer con ellos.
static void drawMonRow(const StoredMon &m, int y, bool activo,
                       bool trancado = false, uint8_t objeto = 0xFF) {
  if (activo) gfx->fillRoundRect(58, y, 324, 34, 8, CARD_DIM);
  gfx->drawRoundRect(58, y, 324, 34, 8, activo ? CARD_INK : CARD_DIM);
  if (!m.used()) {
    gfx->setTextColor(CARD_SOFT);
    setSize(1);
    setCur(76, y + 14);
    printT(T(S_VAZIO));
    return;
  }
  const uint8_t *th = thumbs.get(m.species);
  if (th) drawThumbFit(th, 78, y + 17, 28, false);
  gfx->setTextColor(CARD_INK);
  setSize(2);
  // Hasta ahora un shiny en el banco no se distinguia de uno normal: la
  // miniatura es la del icono, que no tiene version shiny.
  // Va DESPUES de la miniatura (que acaba en x=106) y no pegada al 100, para
  // no montarse encima de la cola del bicho.
  int mx = 100;
  if (m.flags & SM_SHINY) {
    drawShinyStar(ART_SHINY_S, 108 + ART_SHINY_S.w / 2, y + 12);
    mx = 108 + ART_SHINY_S.w + 4;
  }
  setCur(mx, y + 4);
  printT(dexName(m.species));
  char lv[10];
  snprintf(lv, sizeof(lv), "Lv%u", Pet::levelOf(m));
  setSize(1);
  setCur(346 - (int)strlen(lv) * LATIN_ADV, y + 6);
  printT(lv);
  // barra de PS: el maximo se recalcula con la misma formula del Pet
  uint16_t tope = hpAtLevel(m.species, Pet::levelOf(m), m.trHp);
  uint32_t pct = tope ? (uint32_t)m.hpLeft * 100 / tope : 0;
  if (pct > 100) pct = 100;
  gfx->fillRect(100, y + 24, 200, 5, CARD_DIM);
  int fw = (int)(200 * pct / 100);
  if (fw > 0)
    gfx->fillRect(100, y + 24, fw, 5,
                  pct >= 50 ? UI_BAR_OK : pct >= 25 ? UI_BAR_WARN : UI_BAR_BAD);
  // O que leva, pequeno, depois da barra: sem isto nao havia como saber
  // que o Scyther esta com o Metal Coat antes de oferece-lo numa troca.
  // (0xFF = o do registro; o titular passa o dele, que vive no Pet.)
  uint8_t leva = objeto == 0xFF ? m.item : objeto;
  if (leva && leva < ITEM_COUNT) {
    char nm[16];
    snprintf(nm, sizeof nm, "%.*s", trancado ? 7 : 12, ITEM_TBL[leva].name);
    gfx->setTextColor(UI_BAR_OK);
    setSize(1);
    setCur(306, y + 22);
    printT(nm);
  }
  if (trancado) {
    // Passa do teto DESTA regiao: nao sai da caixa aqui. O cadeado e o
    // mesmo do mapa, pequeno: corpo e arco.
    int cx = 362, cy = y + 14;
    gfx->fillRoundRect(cx - 8, cy, 16, 13, 3, C565(0x8a, 0x93, 0xb0));
    // O arco: dois tracos verticais e um por cima. A 16 px de largura
    // um arco redondo de verdade nao se distingue deste, e o canvas
    // nao tem drawCircleHelper.
    uint16_t cor = C565(0x8a, 0x93, 0xb0);
    gfx->drawFastVLine(cx - 5, cy - 6, 6, cor);
    gfx->drawFastVLine(cx - 4, cy - 6, 6, cor);
    gfx->drawFastVLine(cx + 4, cy - 6, 6, cor);
    gfx->drawFastVLine(cx + 5, cy - 6, 6, cor);
    gfx->drawFastHLine(cx - 5, cy - 7, 11, cor);
    gfx->drawFastHLine(cx - 5, cy - 6, 11, cor);
    // e a fila inteira apaga um pouco, pra ler de longe que esta fora
    for (int yy = y + 1; yy < y + 34; yy += 3)
      gfx->drawFastHLine(59, yy, 322, CARD_DIM);
  }
}

// pagina 4: el equipo y el banco. El circulo de la izquierda pone de titular,
// tocar la fila abre SUS golpes y la flecha de la derecha lo manda al banco.
// En el banco, tocar lo recupera.
// o botao TROCAR da pagina do time
// Era 140x30 em y=350, e abaixo de y=352 a ficha FECHAVA: errar o botao
// por um fio fechava tudo e o dono "tentou umas 6 vezes". Agora e grande,
// o toque vale 12 px alem do desenho, e o fechar so comeca 18 px abaixo
// (o "tap: back" desta pagina desce para la: CARD_PE_TIME).
#define CARD_TR_X 133
#define CARD_TR_Y 344
#define CARD_TR_W 200
#define CARD_TR_H 40
#define CARD_TR_MARGEM 12
#define CARD_TR_TOPO 336      // a fila 5 acaba em 330
#define CARD_PE_TIME 398
void renderCardTeam() {
  gfx->setTextColor(CARD_INK);
  setSize(3);
  const char *tit = T(bankView ? S_CARD_BANK : S_CARD_TEAM);
  setCur(centerX(tit, 3), 34);
  printT(tit);

  for (uint8_t b = 0; b < 2; b++) {
    int x = 116 + b * 118, y = 70;
    bool act = (b == (bankView ? 1 : 0));
    const char *t = T(b ? S_CARD_BANK : S_CARD_TEAM);
    gfx->fillRoundRect(x, y, 112, 24, 7, act ? CARD_INK : CARD_PAPER);
    gfx->drawRoundRect(x, y, 112, 24, 7, CARD_INK);
    gfx->setTextColor(act ? CARD_PAPER : CARD_INK);
    setSize(1);
    setCur(x + 56 - (int)strlen(t) * 3, y + 8);
    printT(t);
  }

  if (!bankView) {
    for (uint8_t i = 0; i < TEAM_SLOTS; i++) {
      int y = 106 + i * 38;
      bool principal = (i == pet.mainSlot);
      drawMonRow(pet.team[i], y, principal, false, principal ? pet.heldItem : 0xFF);
      // El circulo de la izquierda dice de QUIEN cuidas, y es tambien el
      // boton para cambiarlo. Tocar la fila NO lo cambia: cambiar de
      // principal es una decision, no algo que pase por rozar la pantalla.
      if (pet.team[i].used()) {
        if (principal) gfx->fillCircle(44, y + 17, 7, UI_BAR_WARN);
        else           gfx->drawCircle(44, y + 17, 7, CARD_DIM);
      }
      // flecha de "al banco", apagada en el principal y en los huecos vacios
      bool puede = pet.team[i].used() && !principal;
      gfx->fillTriangle(396, y + 10, 396, y + 24, 410, y + 17,
                        puede ? CARD_INK : CARD_DIM);
    }
    if (darItemId) {
      // no lugar do TROCAR: a pergunta, grande
      char q[48];
      snprintf(q, sizeof q, T(S_BAG_DARA), ITEM_TBL[darItemId].name);
      gfx->setTextColor(UI_BAR_OK);
      setSize(2);
      setCur(centerX(q, 2), CARD_TR_Y + 12);
      printT(q);
      return;
    }
    gfx->setTextColor(CARD_SOFT);
    setSize(1);
    setCur(centerX(T(S_EQUIPE_DICA), 1), 333);
    printT(T(S_EQUIPE_DICA));
    // TROCAR: abre a troca com outra placa (ESP-NOW)
    gfx->fillRoundRect(CARD_TR_X, CARD_TR_Y, CARD_TR_W, CARD_TR_H, 10,
                       C565(0x2c, 0x4a, 0x6e));
    gfx->drawRoundRect(CARD_TR_X, CARD_TR_Y, CARD_TR_W, CARD_TR_H, 10, UI_INK);
    gfx->setTextColor(UI_WHITE);
    setSize(2);
    setCur(CARD_TR_X + (CARD_TR_W - textW(T(S_CONECTAR), 2)) / 2,
           CARD_TR_Y + (CARD_TR_H - 14) / 2);
    printT(T(S_CONECTAR));
    return;
  }

  // LA BARRA DE LA CAJA. El banco ya no es una lista de 60 y punto: son
  // 32 cajas de 30 en la SD, y sin algo en pantalla que lo diga el
  // jugador no tendria forma de llegar a las otras 31.
  uint8_t n = pet.bankCount();
  {
    char bar[32];
    snprintf(bar, sizeof(bar), "BOX %u/%u   %u/%u",
             (unsigned)(pet.boxOpen + 1), (unsigned)BOX_COUNT,
             (unsigned)n, (unsigned)BANK_SLOTS);
    gfx->setTextColor(CARD_INK);
    setSize(2);
    setCur(centerX(bar, 2), BANK_BAR_Y0 + 5);
    printT(bar);
    int cy = BANK_BAR_Y0 + 12;
    gfx->fillTriangle(70, cy, 84, cy - 9, 84, cy + 9, CARD_INK);
    gfx->fillTriangle(370, cy, 356, cy - 9, 356, cy + 9, CARD_INK);
  }

  if (!n) {
    gfx->setTextColor(CARD_SOFT);
    setSize(2);
    setCur(centerX(T(S_VAZIO), 2), 210);
    printT(T(S_VAZIO));
    return;
  }
  if (cardScroll + BANK_ROWS > n)
    cardScroll = (uint8_t)(n > BANK_ROWS ? n - BANK_ROWS : 0);
  uint8_t fila = 0, vistos = 0;
  for (uint8_t i = 0; i < BANK_SLOTS && fila < BANK_ROWS; i++) {
    if (!pet.bank[i].used()) continue;
    if (vistos++ < cardScroll) continue;
    int y = BANK_ROW_Y0 + fila * 38;
    drawMonRow(pet.bank[i], y, false, pet.bankLocked(pet.bank[i]));
    // O "X" de soltar, onde no time fica o circulo de titular.
    gfx->fillCircle(44, y + 17, 10, UI_BAR_BAD);
    for (int d = 0; d < 2; d++) {   // duas linhas de 1 px: um X de 2 px
      gfx->drawLine(40 + d, y + 13, 48 + d, y + 21, UI_WHITE);
      gfx->drawLine(48 + d, y + 13, 40 + d, y + 21, UI_WHITE);
    }
    fila++;
  }
  cardArrows(n);

  // A pergunta. Soltar nao tem volta, entao nada acontece sem o SIM.
  if (bankSoltar >= 0 && bankSoltar < BANK_SLOTS && timeLeft(bankSoltarAte) &&
      pet.bank[bankSoltar].used()) {
    const StoredMon &m = pet.bank[bankSoltar];
    gfx->fillRoundRect(94, 168, 278, 152, 16, UI_WHITE);
    gfx->drawRoundRect(94, 168, 278, 152, 16, UI_INK);
    char q[48];
    snprintf(q, sizeof(q), T(S_RELEASE_FMT), m.nick[0] ? m.nick : dexName(m.species));
    gfx->setTextColor(UI_INK);
    setSize(2);
    setCur(centerX(q, 2), 196);
    printT(q);
    char lv[16];
    snprintf(lv, sizeof(lv), "Lv%u", (unsigned)Pet::levelOf(m));
    gfx->setTextColor(UI_TRACK);
    setSize(1);
    setCur(centerX(lv, 1), 222);
    printT(lv);
    gfx->fillRoundRect(118, 252, 100, 52, 12, UI_BAR_OK);
    gfx->setTextColor(UI_WHITE);
    setSize(2);
    setCur(118 + (100 - textW(T(S_YES), 2)) / 2, 270);
    printT(T(S_YES));
    gfx->fillRoundRect(248, 252, 100, 52, 12, UI_BAR_BAD);
    setCur(248 + (100 - textW(T(S_NO), 2)) / 2, 270);
    printT(T(S_NO));
  }
}

void cardTeamTap(int x, int y) {
  // A pergunta de soltar come o toque: SIM solta, qualquer outro lugar
  // desiste.
  if (bankSoltar >= 0) {
    int8_t s = bankSoltar;
    bankSoltar = -1;
    if (bankView && timeLeft(bankSoltarAte) &&
        x >= 118 && x <= 218 && y >= 252 && y <= 304)
      sfxPlay(pet.releaseFromBank((uint8_t)s) ? SFX_BYE : SFX_DENY);
    else
      sfxPlay(SFX_TAP);
    return;
  }
  // "Dar X a quem?": tocar num do time entrega (o circulo e a seta
  // tambem: a fila inteira e o alvo). Fora das filas: desiste, sem fechar;
  // so o pe (o fechar de sempre) fecha.
  if (darItemId && !bankView) {
    uint8_t it = darItemId;
    darItemId = 0;
    int fila = y < 106 ? -1 : (y - 106) / 38;
    if (fila >= 0 && fila < TEAM_SLOTS && (y - (106 + fila * 38)) <= 36 &&
        pet.team[fila].used()) {
      sfxPlay(pet.darItem((uint8_t)fila, it) ? SFX_PLAY : SFX_DENY);
      return;
    }
    // Desiste no meio; perto das bordas (titulo, pe) nao faz nada, que
    // ali comeca o fechar e um "desistir" colado nele seria o mesmo fosso
    // que o TROCAR tinha.
    if (cardCierraEn(y, CARD_TR_Y + CARD_TR_H)) cardOpen = false;
    else if (y >= 70 && y <= CARD_TR_Y + CARD_TR_H) sfxPlay(SFX_TAP);
    else darItemId = it;   // vao: segue perguntando
    return;
  }
  if (!bankView && bankSoltar < 0 && x >= CARD_TR_X - CARD_TR_MARGEM &&
      x <= CARD_TR_X + CARD_TR_W + CARD_TR_MARGEM &&
      y >= CARD_TR_TOPO && y <= CARD_TR_Y + CARD_TR_H) {
    cardOpen = false;
    trocaAbre();
    return;
  }
  if (y >= 66 && y <= 96 && x >= 116 && x <= 346) {   // las dos pestanas
    bankView = (x >= 234);
    cardScroll = 0;
    sfxPlay(SFX_TAP);
    return;
  }
  // Las flechas de la CAJA. Van antes que las filas y solo en el banco.
  // De x=62 a x=378 porque las de rodar la lista viven en x>=386.
  if (bankView && y >= BANK_BAR_Y0 && y <= BANK_BAR_Y1 && x >= 62 && x <= 378) {
    uint8_t destino = pet.boxOpen;
    if (x <= 150) destino = (uint8_t)((pet.boxOpen + BOX_COUNT - 1) % BOX_COUNT);
    else if (x >= 290) destino = (uint8_t)((pet.boxOpen + 1) % BOX_COUNT);
    if (destino != pet.boxOpen) {
      // boxGo guarda la abierta ANTES de soltarla y devuelve false si no
      // pudo: cambiar de pagina no puede costar una caja.
      sfxPlay(pet.boxGo(destino) ? SFX_TAP : SFX_DENY);
      cardScroll = 0;
    }
    return;
  }

  int filaY0 = bankView ? BANK_ROW_Y0 : 106;
  int filasMax = bankView ? BANK_ROWS : 6;
  // El `y < filaY0` va aparte: la division entera trunca hacia cero, y sin
  // el los 37 px de encima de la lista contaban como la fila 0 (tocar al
  // lado de una pestana abria los golpes del primero, o le ponia la X).
  int fila = y < filaY0 ? -1 : (y - filaY0) / 38;
  if (fila < 0 || fila >= filasMax || (y - (filaY0 + fila * 38)) > 34) {
    if (bankView && cardArrowTap(x, y, pet.bankCount())) return;
    // Fallar un blanco NO cierra: solo arriba (el titulo) o abajo. Entre
    // dos filas, o al lado de una pestana, no pasa nada.
    int fondo = filaY0 + filasMax * 38;
    if (bankView && fondo < 326) fondo = 326;       // las flechas de rodar
    if (!bankView) fondo = CARD_TR_Y + CARD_TR_H;   // o botao TROCAR
    if (cardCierraEn(y, fondo)) cardOpen = false;
    return;
  }

  if (!bankView) {
    if (x >= 386) {                       // la flecha: al banco
      sfxPlay(pet.toBank((uint8_t)fila) ? SFX_PLAY : SFX_DENY);
    } else if (x <= 62) {
      // El circulo: cambiar de PRINCIPAL, o sea de a quien crias. Llega con
      // SU vinculo, que es lo que hace que la decision cueste algo. Es un
      // boton aparte a proposito: no puede pasar por tocar la fila.
      sfxPlay(pet.setMain((uint8_t)fila) ? SFX_PLAY : SFX_DENY);
    } else if (pet.team[fila].used()) {
      // El cuerpo de la fila: SUS golpes. Antes no hacia nada y para ver los
      // de un suplente habia que hacerlo titular, que cuesta el vinculo del
      // que lo era. Mirar (y cambiarselos) no puede costar eso.
      movesSlot = (fila == pet.mainSlot) ? MOVES_MAIN : (uint8_t)fila;
      cardPage = 2;
      cardScroll = 0;
      moveShown = 0;
      sfxPlay(SFX_TAP);
    }
    return;
  }
  if (cardArrowTap(x, y, pet.bankCount())) return;
  // en el banco: encontrar el hueco real que corresponde a esta fila
  uint8_t objetivo = (uint8_t)(cardScroll + fila), vistos = 0;
  for (uint8_t i = 0; i < BANK_SLOTS; i++) {
    if (!pet.bank[i].used()) continue;
    if (vistos++ != objetivo) continue;
    if (x <= 62) {                        // o X: pergunta antes de soltar
      bankSoltar = (int8_t)i;
      bankSoltarAte = millis() + 10000;
      sfxPlay(SFX_TAP);
      return;
    }
    sfxPlay(pet.fromBank(i) ? SFX_PLAY : SFX_DENY);
    return;
  }
}

void renderCard() {
  drawDexSkin();
  if (cardPage == 0) renderCardProfile();
  else if (cardPage == 1) renderCardStats();
  else if (cardPage == 2) renderCardMoves();
  else if (cardPage == 3) renderCardBag();
  else if (cardPage == 4) renderCardTeam();
  else if (cardPage == 5) renderCardGym();
  else if (cardPage == 6) renderCardTrainer();
  else if (cardPage == 7) renderCardMedals();
  else renderCardProgress();

  // Ayuda primero y los puntos DEBAJO. Antes los ocho puntos iban en
  // y=374 con radio 5, que es justo donde acaban las listas de las
  // paginas (golpes, equipo, bolsa) y se metian en el texto. Son un
  // "por donde vas", no algo que haya que leer: abajo del todo y
  // pequenos. A esa altura el circulo todavia da 258 px de ancho y los
  // ocho ocupan 98.
  gfx->setTextColor(UI_TRACK);
  setSize(2);
  // Com a ficha do objeto aberta o pe desce para baixo dela: em 390 ele
  // caia em cima dos botoes (e tocar ali e apertar o botao, nao voltar).
  const bool fichaObj = cardPage == 3 && bagShown &&
                        (ITEM_TBL[bagShown].pocket == BP_HELD ||
                         ITEM_TBL[bagShown].pocket == BP_EVO);
  setCur(centerX(T(S_BACK), 2), (cardPage == 4 && !bankView) ? CARD_PE_TIME
                                : fichaObj ? BAGF_VOLTA_Y : 390);
  printT(T(S_BACK));
  if (!cardSolo) {
    int x0 = 233 - (CARD_SEQ_N - 1) * 7;
    int aqui = cardSeqIdx(cardPage);
    for (int i = 0; i < CARD_SEQ_N; i++) {
      if (i == aqui) gfx->fillCircle(x0 + i * 14, 424, 4, UI_INK);
      else gfx->drawCircle(x0 + i * 14, 424, 3, UI_TRACK);
    }
  }
  gfx->flush();
}

// ---------- teclado para renombrar ----------

static const char KB_KEYS[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ.-";  // 28 + DEL + OK = 30
#define KB_COLS 6
#define KB_X 40
#define KB_Y 150
#define KB_W 64
#define KB_H 52

void openKeyboard() {
  kbOpen = true;
  strncpy(nameBuf, pet.nick, sizeof(nameBuf) - 1);
  nameBuf[sizeof(nameBuf) - 1] = 0;
  nameLen = strlen(nameBuf);
}

void renderKeyboard() {
  drawDexSkin();
  gfx->setTextColor(CARD_INK);
  setSize(2);
  setCur(centerX(T(S_NAME), 2), 56);
  printT(T(S_NAME));
  // buffer actual
  gfx->fillRoundRect(83, 84, 300, 40, 8, CARD_PAPER);
  gfx->drawRoundRect(83, 84, 300, 40, 8, CARD_INK);
  setSize(3);
  setCur(95, 94);
  printT(nameLen ? nameBuf : "_");

  for (int i = 0; i < 30; i++) {
    int x = KB_X + (i % KB_COLS) * KB_W, y = KB_Y + (i / KB_COLS) * KB_H;
    bool special = (i >= 28);
    gfx->fillRoundRect(x, y, KB_W - 6, KB_H - 6, 6, special ? UI_BAR_WARN : CARD_PAPER);
    gfx->drawRoundRect(x, y, KB_W - 6, KB_H - 6, 6, CARD_INK);
    gfx->setTextColor(CARD_INK);
    setSize(2);
    if (i < 28) {
      setCur(x + KB_W / 2 - 9, y + KB_H / 2 - 10);
      printT(KB_KEYS[i]);
    } else {
      const char *lab = (i == 28) ? "<-" : "OK";
      setCur(x + KB_W / 2 - 15, y + KB_H / 2 - 10);
      printT(lab);
    }
  }
  gfx->flush();
}

void keyboardTap(int16_t x, int16_t y) {
  // filtra ANTES de dividir: con x/y menores que KB_X/KB_Y la resta da negativo
  // y la division entera trunca hacia cero (no hacia -inf), asi que col/row
  // saldria 0 en vez de negativo y el filtro de abajo no lo detectaria
  if (x < KB_X || y < KB_Y) return;
  int col = (x - KB_X) / KB_W, row = (y - KB_Y) / KB_H;
  if (col >= KB_COLS || row >= 5) return;
  int i = row * KB_COLS + col;
  if (i >= 30) return;
  if (i == 28) {  // borrar
    if (nameLen) nameBuf[--nameLen] = 0;
  } else if (i == 29) {  // OK
    pet.rename(nameBuf);
    kbOpen = false;
  } else if (nameLen < sizeof(nameBuf) - 1) {
    nameBuf[nameLen++] = KB_KEYS[i];
    nameBuf[nameLen] = 0;
  }
}

// ---------- galeria pokedex ----------

#define GAL_X 73
#define GAL_Y 84
#define GAL_CELL 80

// dibuja una miniatura centrada en su celda; sil=true la pinta en tinta
// Como drawThumb, pero cabiendo en una caja: elige la escala mas grande que
// entre y lo centra en un punto. Hace falta porque las miniaturas cambiaron
// de tamano al pasar de PMD a los iconos de Anil -las de antes eran de 24 px
// y las de ahora llegan a 40- y una escala fija que iba bien con unas se sale
// de la pantalla con las otras.
void drawThumbFit(const uint8_t *b, int cx, int cy, int caja, bool sil) {
  uint8_t w = b[0], h = b[1], n = b[2];
  const uint8_t *pal = b + 3;
  const uint8_t *d = pal + n * 2;
  int mayor = w > h ? w : h;
  int s = mayor ? caja / mayor : 1;
  if (s < 1) s = 1;
  int ox = cx - w * s / 2, oy = cy - h * s / 2;
  for (int r = 0; r < h; r++) {
    for (int c = 0; c < w; c++) {
      uint8_t idx = d[r * w + c];
      if (idx == 0xFF) continue;
      uint16_t col = sil ? INK_K
                         : (uint16_t)(pal[idx * 2] | (pal[idx * 2 + 1] << 8));
      gfx->fillRect(ox + c * s, oy + r * s, s, s, col);
    }
  }
}

// `silCor` < 0 = com as cores dele; senao, silhueta lisa dessa cor (leva 72:
// a escura de quem nunca apareceu, ou a clara de quem ja foi visto).
// (a firma va en UNA linea a proposito: ver el aviso de desktop/tools/gen_sketch.py)
void drawThumbCor(const uint8_t *b, int x, int y, int s, int32_t silCor) {
  uint8_t w = b[0], h = b[1], n = b[2];
  const uint8_t *pal = b + 3;
  const uint8_t *d = pal + n * 2;
  int ox = x + (GAL_CELL - w * s) / 2;
  int oy = y + (GAL_CELL - h * s) / 2;
  for (int r = 0; r < h; r++) {
    for (int c = 0; c < w; c++) {
      uint8_t idx = d[r * w + c];
      if (idx == 0xFF) continue;
      uint16_t col = silCor >= 0 ? (uint16_t)silCor
                                 : (uint16_t)(pal[idx * 2] | (pal[idx * 2 + 1] << 8));
      gfx->fillRect(ox + c * s, oy + r * s, s, s, col);
    }
  }
}

void drawThumb(const uint8_t *b, int x, int y, int s, bool sil) {
  drawThumbCor(b, x, y, s, sil ? (int32_t)DEX_SIL : -1);
}

// La entrada de Pokedex, partida en lineas por PALABRAS. Sin barra de scroll
// a proposito: la mas larga de las 151 son 163 caracteres y en una caja de
// 300 px a escala 1 (50 caracteres por linea) eso son cinco lineas. Cabe
// entera, y una caja que no se desliza siempre se lee mejor que una que si.
#define DEX_TXT_W 300
#define DEX_TXT_COLS (DEX_TXT_W / LATIN_ADV)
#define DEX_TXT_LH 11
#define DEX_TXT_MAX 6

// Parte un texto en lineas por PALABRAS y lo centra. Devuelve cuantas lineas
// ha pintado. Lo usan la Pokedex y la intro: una sola implementacion del
// corte, que es justo lo que se rompe cuando hay dos.
// Texto en parrafo, centrado, cortando por palabras.
//
// `talla` NO estaba, y esta funcion solo sabia pintar a tamano 1: el
// centrado era `233 - corte * 3`, donde ese 3 es la mitad de los 6 px
// que mide un caracter a escala 1. Subir el tamano desde fuera dejaba
// el parrafo descolocado a la izquierda. Con la placa en la mano el
// dueno del juego vio lo de siempre desde el otro lado: la charla del
// profesor, la habilidad y la ficha de golpe eran ilegibles de puro
// pequenas.
//
// Devuelve DONDE SE QUEDO, no cuantas lineas pinto. Ese cambio es lo
// que deja paginar un texto largo sin partirlo a mano en el i18n: el
// llamante guarda el puntero y en la pagina siguiente sigue por ahi.
// Si cabe entero, devuelve el final de la cadena.
// (la firma va en UNA linea a proposito: ver el aviso de desktop/tools/gen_sketch.py)
const char *drawWrapped(const char *s, int y0, int cols, int lh, int maxLines, uint8_t talla) {
  if (!s || !*s || cols < 1) return s;
  char linea[DEX_TXT_COLS + 1];
  if (cols > DEX_TXT_COLS) cols = DEX_TXT_COLS;
  setSize(talla);
  int n = 0;
  while (*s && n < maxLines) {
    while (*s == ' ') s++;
    if (!*s) break;
    // cuanto cabe sin partir la ultima palabra
    int corte = 0, ultimo = 0;
    while (s[corte] && corte < cols) {
      if (s[corte] == ' ') ultimo = corte;
      corte++;
    }
    if (s[corte] && ultimo) corte = ultimo;   // hay mas texto: cortar en el hueco
    memcpy(linea, s, corte);
    linea[corte] = 0;
    // centrado con la medida DE VERDAD, que ya sabe de la talla y de las
    // fuentes CJK (donde los caracteres no miden todos lo mismo).
    setCur(centerX(linea, talla), y0 + n * lh);
    printT(linea);
    s += corte;
    n++;
  }
  while (*s == ' ') s++;
  return s;
}

// Por donde va la ficha de la Pokedex y donde acabo la pagina pintada.
const char *dexTxtPtr = nullptr;
const char *dexTxtFin = nullptr;
int16_t dexTxtDe = 0;
// Que se esta leyendo: 0 la ficha de la dex, 1..n la habilidad n de la
// version que se esta mirando. Y de QUE version, para que cambiar de
// version con la flecha no deje el texto de la anterior puesto.
int8_t dexPag = 0;
int8_t dexTxtVer = -1;
// No es static (igual que openDexEntry) para que la bateria de UI pueda
// preguntarle que habilidades ensena cada version: es lo unico que
// distingue leer la tabla de la mega de leer la de su especie base, y a
// ojo las dos pintan un parrafo igual de creible.
uint8_t galleryAbils(int16_t dex, uint16_t *out);

static void drawDexEntry(int16_t dex, int y0) {
  if (dex < 1 || dex > DEX_COUNT) return;
  uint16_t ab[4];
  uint8_t nab = galleryAbils(dex, ab);
  // Cambiar de bicho O de version manda al principio: si no, se seguia
  // leyendo la pagina 3 de la habilidad de otro.
  if (dexTxtDe != dex || dexTxtVer != galleryVer) {
    dexTxtDe = dex;
    dexTxtVer = galleryVer;
    dexPag = 0;
    dexTxtPtr = dexDesc(dex);
  }
  if (dexPag > (int8_t)nab) { dexPag = 0; dexTxtPtr = dexDesc(dex); }

  // La CABECERA dice de QUIEN es el texto. Sin ella, la descripcion de
  // una habilidad leida a secas parece la continuacion de la ficha.
  int ty = y0;
  if (dexPag > 0) {
    const char *an = abilAt(ab[dexPag - 1]).name;
    gfx->setTextColor(DEX_MARK);
    setSize(1);
    setCur(centerX(an, 1), y0);
    printT(an);
    ty = y0 + 14;
  }
  gfx->setTextColor(UI_INK);
  // Talla 2 y paginado. La caja (70,306,326,84) deja 310 px utiles, o
  // sea 25 columnas. Cuatro lineas de 18 en la ficha, y tres cuando hay
  // cabecera de habilidad encima.
  dexTxtFin = drawWrapped(dexTxtPtr, ty, 25, 18, dexPag > 0 ? 3 : 4, 2);
  // La flecha sale tambien cuando falta habilidad por leer, que es lo
  // que dice que hay mas detras de la ficha.
  if ((dexTxtFin && *dexTxtFin) || dexPag < (int8_t)nab)
    flechaMas(378, 378, DEX_MARK);
}

// Pasa de pagina: mas de este texto, y si se acabo, a la habilidad
// siguiente; despues de la ultima se vuelve a la ficha.
static void dexPasaPagina(int16_t dex) {
  if (dexTxtFin && *dexTxtFin) { dexTxtPtr = dexTxtFin; return; }
  uint16_t ab[4];
  uint8_t nab = galleryAbils(dex, ab);
  dexPag = (int8_t)(dexPag + 1);
  if (dexPag > (int8_t)nab) dexPag = 0;
  dexTxtPtr = dexPag > 0 ? ABIL_DESC[ab[dexPag - 1]] : dexDesc(dex);
}

// Como se codifica cada version en galleryVers: -1 el normal, -2 el
// shiny, >= 0 una fila de FORM_TBL. Las MEGAS no caben en ninguno de los
// dos sitios (viven en su propia tabla, ver megas.h), asi que se meten
// por debajo: GVER_MEGA0 - fila.
#define GVER_MEGA0 (-1000)

// Rellena galleryVers con lo que el jugador TIENE de ese numero.
static void galleryCalcVers(int16_t dex) {
  galleryNVers = 0;
  galleryVer = 0;
  galleryVers[galleryNVers++] = -1;                 // el normal, siempre
  if (pet.isShinyRegistered(dex) && galleryNVers < GVER_MAX)
    galleryVers[galleryNVers++] = -2;
  // Las formas se recorren por la tabla y no por `dex`: una especie puede
  // tener varias (Alola, Galar...), y solo entran las que estan marcadas.
  for (uint8_t f = 1; f < 8 && galleryNVers < GVER_MAX; f++) {
    int16_t fila = formRow(dex, f);
    if (fila >= 0 && pet.isFormRegistered(fila))
      galleryVers[galleryNVers++] = fila;
  }
  // Y las megas que hayas visto. Se recorre MEGA_TBL entera porque una
  // especie puede tener DOS (las X y las Y), y porque la tabla no esta
  // indexada por dex.
  for (uint16_t i = 0; i < MEGA_COUNT && galleryNVers < GVER_MAX; i++)
    if (MEGA_TBL[i].base == dex && pet.isMegaRegistered((int16_t)i))
      galleryVers[galleryNVers++] = (int16_t)(GVER_MEGA0 - (int16_t)i);
}

// La fila de MEGA_TBL que mira `q`, o -1 si no es una mega.
static inline int16_t galleryMegaDe(int16_t q) {
  return q <= GVER_MEGA0 ? (int16_t)(GVER_MEGA0 - q) : (int16_t)-1;
}

// Carga en galleryPmd la version que toca.
static void galleryCargaVer(int16_t dex) {
  int16_t q = galleryVers[galleryVer];
  galleryPmd.unload();
  int16_t mg = galleryMegaDe(q);
  if (mg >= 0)     galleryPmd.loadMega(mg);
  else if (q >= 0) galleryPmd.loadForm(q);
  else             galleryPmd.load(dex, q == -2);
}

// Como se llama la que se esta viendo.
static const char *galleryVerNombre(int16_t dex) {
  int16_t q = galleryVers[galleryVer];
  int16_t mg = galleryMegaDe(q);
  if (mg >= 0) return MEGA_TBL[mg].name;
  if (q == -1) return "NORMAL";
  if (q == -2) return "SHINY";
  return FORM_TBL[q].name;
}

// Las habilidades de la version que se esta mirando, sin repetidas.
// Una mega o una forma propia tienen UNA, que es justo la gracia de
// mirarlas; la normal puede llevar hasta tres (las dos y la oculta).
uint8_t galleryAbils(int16_t dex, uint16_t *out) {
  int16_t q = galleryVers[galleryVer];
  int16_t mg = galleryMegaDe(q);
  if (mg >= 0 && MEGA_TBL[mg].ability) { out[0] = MEGA_TBL[mg].ability; return 1; }
  if (q >= 0 && FORM_TBL[q].ability)   { out[0] = FORM_TBL[q].ability; return 1; }
  if (dex < 1 || dex > DEX_COUNT) return 0;
  uint8_t n = 0;
  for (uint8_t i = 0; i < 3; i++) {
    uint16_t a = ABIL_OF[dex][i];
    if (!a || a >= ABIL_COUNT) continue;
    bool rep = false;
    for (uint8_t j = 0; j < n; j++) if (out[j] == a) rep = true;
    if (!rep) out[n++] = a;
  }
  return n;
}

void renderGallery() {
  if (galleryDetail) {  // vista detalle: se redibuja siempre (animada)
    drawDexSkin();
    const DexEntry &d = DEX_TBL[galleryDetail];
    bool reg = pet.isRegistered(galleryDetail);
    // Leva 72: VISTO e nao registrado mostra o bicho de verdade (nome, cor,
    // tipo, grito) e no lugar do texto da Pokedex, que so se sabe de quem
    // voce teve, vem o botao de CACAR.
    bool vis = pet.isSeen(galleryDetail);
    char head[32];
    snprintf(head, sizeof(head), "N.%03d %s%s", galleryDetail,
             "", vis ? dexName(galleryDetail) : "???");
    // auto-encoge nombres largos (no caben a t3), midiendo en vez de contar
    // bytes; 13 caracteres a tamano 3 son los 234 px de antes.
    setSize(3);
    int gts = (textW(head, 3) <= 234) ? 3 : 2;
    int gy = gts == 3 ? 56 : 60;
    bool gsh = pet.isShinyRegistered(galleryDetail);
    int ghueco = gsh ? ART_SHINY.w + 6 : 0;
    int gx = (466 - (textW(head, gts) + ghueco)) / 2;
    // el rotulo va en su panel, como en la Pokedex del Anil
    drawDexBox(gx + ghueco - 14, gy - 8, textW(head, gts) + 24, 8 * gts + 16);
    if (gsh) drawShinyStar(ART_SHINY, gx + ART_SHINY.w / 2, gy + 4 * gts);
    gfx->setTextColor(reg ? d.accent : UI_INK);
    setSize(gts);
    setCur(gx + ghueco, gy);
    printT(head);
    if (galleryPmd.loaded) {
      // animado y a color si esta registrado; silueta estatica si no (estilo "?")
      // a SU tamano, no al normalizado: en la Pokedex es justo donde se
    // comparan unos con otros
    // 284 e nao 300: a caixa de texto subiu junto para abrir a faixa
    // do credito, que antes caia DENTRO dela.
    drawPmdActM(galleryPmd, PMD_IDLE, CX, 284, vis ? millis() : 0, true, !vis, 6,
                nullptr, galleryDetail);
    } else {
      const uint8_t *t = thumbs.get(galleryDetail);
      if (t) drawThumb(t, CX - GAL_CELL, 135, 4, !vis);
    }
    // EL TIPO, en la columna de la izquierda. Es lo primero que se
    // busca en una ficha de Pokedex y no estaba en ninguna parte.
    //
    // A la izquierda y en columna porque es el unico hueco de verdad:
    // arriba manda el rotulo del nombre (y el de la version, cuando hay
    // mas de una), el centro es del bicho —— medido en el simulador, un
    // Onix ocupa de x=140 a x=300 —— y abajo esta la caja del texto. La
    // flecha de version vive en la derecha (416,196), asi que las
    // chapas en la izquierda ademas equilibran la pantalla.
    //
    // Y del que se esta VIENDO, no de la especie base: una forma de
    // Alola o una mega cambian de tipo, y ensenar el del normal mientras
    // se mira al Mega Charizard X seria decir una mentira a la cara.
    // Aqui se lee FORM_TBL/MEGA_TBL a mano y no por tiposDe() porque la
    // galeria guarda FILAS de tabla y tiposDe recibe un indice de forma.
    if (vis) {
      int16_t q = galleryVers[galleryVer];
      int16_t mg = galleryMegaDe(q);
      uint8_t t1, t2;
      if (mg >= 0)     { t1 = MEGA_TBL[mg].type1; t2 = MEGA_TBL[mg].type2; }
      else if (q >= 0) { t1 = FORM_TBL[q].type1;  t2 = FORM_TBL[q].type2; }
      else             { t1 = d.type1;            t2 = d.type2; }
      drawTypesOf(26, 164, 108, 28, 6, true, t1, t2, 1);
    }

    // La entrada de Pokedex, debajo del bicho. Solo si esta registrado: de un
    // bicho que no has criado no sabes nada, igual que el nombre sale "???".
    if (reg) {
      // 84 de alto y no 76: entra una cuarta linea, que es lo que hace
      // sitio a la cabecera con el nombre de la habilidad. No puede
      // crecer mas —— a y=400 la cuerda del circulo mide de 70,5 a
      // 395,5 y la caja (70 a 396) ya asomaria por las dos esquinas.
      // Sobe de 306 para 290: assim termina em y=374 e sobra a faixa
      // 376..392 para o credito, que fica entre ela e o "voltar".
      drawDexBox(70, 290, 326, 84);
      // 296 e nao 312: a caixa subiu 16 px e o texto tem que subir
      // com ela, senao a ultima linha vaza por baixo e bate no
      // credito —— que foi exatamente o que aconteceu.
      drawDexEntry(galleryDetail, 296);
    } else if (vis) {
      // Leva 72: a CACADA, na mesma caixa. Cacando este: o aviso e PARAR.
      // Pode sair no mato: CACAR. Nao pode (outra regiao, lendario, forte
      // demais para as insignias): so o aviso, sem botao que diga nao.
      drawDexBox(70, 290, 326, 84);
      const bool este = (pet.cacaDex == galleryDetail);
      if (este) {
        const char *s = T(S_DEX_CACANDO);
        printTituloC(s, 296, tituloTalla(s, 2, 300), DEX_CACA, C565(0x40, 0x20, 0x08));
        drawBotaoCartel(CACA_BX, CACA_BY, CACA_BW, CACA_BH, C565(0x58, 0x70, 0x98),
                        T(S_DEX_PARAR));
      } else if (pet.podeCacar(galleryDetail, 12)) {
        drawBotaoCartel(CACA_BX, CACA_BY - 8, CACA_BW, CACA_BH, C565(0xe0, 0x50, 0x38),
                        T(S_DEX_CACAR));
      } else {
        // Na letra de sempre: na de titulo, 25 letras so cabiam a talla 1
        // e ficava miuda. A 2 sao 12 px por letra, 300 px no maximo.
        const char *s = T(S_DEX_NAO_SAI);
        uint8_t ts = 2;
        while (ts > 1 && textW(s, ts) > 300) ts--;
        gfx->setTextColor(UI_INK);
        setSize(ts);
        setCur(centerX(s, ts), 326);
        printT(s);
      }
    }
    // El grito, una vez al abrir la ficha: es lo que hace que la Pokedex
    // sea una Pokedex y no una lista de dibujos. Solo de lo registrado
    // (leva 72: e do visto, que tambem ja se ouviu).
    if (vis && galleryCried != galleryDetail) {
      galleryCried = galleryDetail;
      cryPlay((uint16_t)galleryDetail);
    }

    // El selector de version: solo sale si hay MAS DE UNA. Con una sola, un
    // boton que no lleva a ningun sitio es ruido.
    if (reg && galleryNVers > 1) {
      const char *vn = galleryVerNombre(galleryDetail);
      uint8_t vt = 1;
      drawDexBox(CX - (textW(vn, vt) + 22) / 2, 96, textW(vn, vt) + 22, 22);
      gfx->setTextColor(UI_INK);
      setSize(vt);
      setCur(centerX(vn, vt), 103);
      printT(vn);
      // la flecha, a la derecha del bicho. A y=196 la cuerda deja sitio de
      // sobra para un circulo de radio 22 centrado en x=416.
      gfx->fillCircle(GVER_X, GVER_Y, GVER_R, DEX_TEAL);
      gfx->drawCircle(GVER_X, GVER_Y, GVER_R, UI_INK);
      gfx->setTextColor(UI_WHITE);
      setSize(2);
      setCur(GVER_X - 5, GVER_Y - 7);
      printT(">");
    }

    // A CARA e o CREDITO de quem a desenhou.
    //
    // O credito nao e enfeite: a licenca do SpriteCollab (CC BY-NC) pede
    // credito, e ele e POR ESPECIE —— esta ficha e a unica tela que ja e
    // por especie, entao e aqui que ele pertence.
    // (leva 72: tambem do VISTO —— o desenho dele aparece, e o credito e
    // de quem fez o desenho)
    if (vis && galleryDetail > 0 && galleryDetail <= DEX_COUNT) {
      // A CARA, a direita, logo acima da setinha de versao.
      //
      // A primeira tentativa foi em (92,150) e cobriu as chapas de
      // tipo —— a coluna da esquerda e delas. A direita estava livre:
      // ali so vive a setinha, em (416,196).
      // A cara acompanha a VERSAO que o sprite grande mostra: na
      // shiny (-2) a cara shiny, nas outras a normal.
      const bool caraSh = galleryNVers && galleryVers[galleryVer] == -2;
      // Na versao MEGA, a cara da mega, quando foi feita (a Mega
      // Arcanine); as outras caem na da especie.
      const int16_t mgv = galleryNVers ? galleryMegaDe(galleryVers[galleryVer]) : -1;
      const uint8_t caraMega =
          (mgv >= 0 && MEGA_TBL[mgv].base == galleryDetail) ? MEGA_TBL[mgv].form : 0;
      // (a versao mega da Pokedex mostra o sprite NORMAL da mega, entao
      // a cara tambem e a normal: uma shiny deixada pela vitoria nao serve)
      const bool megaPronta = caraMega && porMon.dex == (uint16_t)galleryDetail &&
                              porMon.mega == caraMega && !porMon.shiny && porMon.blob;
      if (caraMega && !megaPronta)
        porMon.loadMega((uint16_t)galleryDetail, caraMega, false);
      const bool temMega = caraMega && porMon.dex == (uint16_t)galleryDetail &&
                           porMon.mega == caraMega && !porMon.shiny && porMon.blob;
      if (!temMega &&
          (porMon.dex != (uint16_t)galleryDetail || porMon.shiny != caraSh || porMon.mega))
        porMon.load((uint16_t)galleryDetail, caraSh);
      const Art *cara = (porMon.dex == (uint16_t)galleryDetail &&
                         (temMega || (!porMon.mega && porMon.shiny == caraSh)))
                            ? porMon.cara(POR_NORMAL)
                            : nullptr;
      // Escala 2 e DENTRO do circulo. A 40x40 virava um selo
      // minusculo; a 80x80 em (392,172) o canto de cima a direita
      // caia fora da tela —— a 466 de diametro, na altura y=92 a
      // corda so chega a x=418 e o retrato ia ate 432.
      //
      // Aqui ocupa x 342..422, y 178..258. O canto pior, (422,178),
      // fica a 197 px do centro, com 36 de folga ate a borda.
      if (cara) drawArt(*cara, 382, 258, 2);

      // Ate tres autores. Em texto pequeno e apagado: e um credito, nao
      // um titulo —— mas tem que estar visivel, nao escondido num menu.
      char cred[64];
      cred[0] = 0;
      for (uint8_t k = 0; k < CREDITO_MAX; k++) {
        uint8_t a = CREDITO_DE[galleryDetail][k];
        if (a >= CREDITO_AUTORES) continue;
        if (cred[0]) strncat(cred, ", ", sizeof(cred) - strlen(cred) - 1);
        strncat(cred, CREDITO_NOMBRE[a], sizeof(cred) - strlen(cred) - 1);
      }
      if (cred[0]) {
        gfx->setTextColor(DEX_TEAL);
        setSize(1);
        // 380: entre o fim da caixa de texto (374) e o "voltar" (408).
        setCur(centerX(cred, 1), 380);
        printT(cred);
      }
    }

    printOutlined(centerX(T(S_DETAIL_BACK), 2), 408, T(S_DETAIL_BACK), 2, DEX_TEAL);
    gfx->flush();
    return;
  }

  if (!galleryDirty) return;  // la rejilla es estatica
  galleryDirty = false;

  drawDexSkin();
  char head[24];
  snprintf(head, sizeof(head), T(S_POKEDEX_FMT), pet.registeredCount(),
           (unsigned)DEX_COUNT);
  // La TALLA se elige, no se fija. La pantalla es redonda y la caja del
  // titulo empieza en y=28, donde la cuerda visible mide 221 px:
  // "POKEDEX 1/1025" a talla 3 pide 252 mas 28 de caja, asi que salia
  // cortado por las dos puntas. Y empeoraba al pasar de 99 registrados,
  // que es un digito mas. Tampoco se puede bajar la caja: la rejilla
  // empieza en GAL_Y (84) y ya roza el circulo por abajo.
  //
  // Asi que se prueba 3, y si no cabe se baja. Vale igual para los
  // idiomas cuya palabra sea mas larga, que es lo que no se puede
  // comprobar a ojo uno por uno.
  uint8_t tt = 3;
  while (tt > 1 && textW(head, tt) + 28 > anchoVisible(28)) tt--;
  drawDexBox(centerX(head, tt) - 16, 28, textW(head, tt) + 28, 40);
  gfx->setTextColor(UI_INK);
  setSize(tt);
  setCur(centerX(head, tt), 36 + (3 - tt) * 4);
  printT(head);

  for (int r = 0; r < 4; r++) {
    for (int c = 0; c < 4; c++) {
      int16_t dex = galleryPage * 16 + r * 4 + c + 1;
      if (dex > DEX_COUNT) break;
      int x = GAL_X + c * GAL_CELL, y = GAL_Y + r * GAL_CELL;
      const uint8_t *t = thumbs.get(dex);
      if (t) {
        // Leva 72: tres estados. Seu = a cor; visto = silhueta clara;
        // nunca apareceu = a escura de sempre.
        if (dex == pet.cacaDex) {     // o cacado: anel laranja por tras
          int r0 = GAL_CELL / 2 - 3;
          for (int k = 0; k < 3; k++)
            gfx->drawCircle(x + GAL_CELL / 2, y + GAL_CELL / 2, r0 - k, DEX_CACA);
        }
        drawThumbCor(t, x, y, 2, pet.isRegistered(dex) ? -1
                                 : pet.isSeen(dex)   ? (int32_t)DEX_VISTO
                                                     : (int32_t)DEX_SIL);
        if (pet.isShinyRegistered(dex)) {
          gfx->setTextColor(UI_BAR_WARN);
          setSize(2);
          setCur(x + 62, y + 4);
          printT("*");
        }
      } else {
        char num[6];
        snprintf(num, sizeof(num), "%d", dex);
        gfx->setTextColor(DEX_MARK);
        setSize(2);
        setCur(x + 24, y + 32);
        printT(num);
      }
    }
  }
  // Puntos de pagina. Eran 10 fijos a partir de x=170; con Johto son 16 y
  // ya no caben con ese paso, asi que la fila se centra y se aprieta: a
  // y=436 la cuerda del circulo mide 228 px y 16 x 12 son 192.
  for (int i = 0; i < GAL_PAGES; i++) {
    int x = CX + (i - (GAL_PAGES - 1) / 2.0f) * 12;
    if (i == galleryPage) gfx->fillCircle(x, 436, 4, DEX_TEAL);
    else gfx->drawCircle(x, 436, 3, DEX_MARK);
  }
  gfx->flush();
}

// Abre la Pokedex DIRECTAMENTE en la ficha de una especie. Lo usa el final
// del combate cuando acabas de capturar algo: el bicho entra en la caja y
// hasta ahora lo unico que lo decia era una linea de texto que se iba sola.
//
// No es static porque la llama battleClose(), que vive en battle_ui.ino. Y
// va AQUI ABAJO y no arriba del todo porque galleryCalcVers/galleryCargaVer
// si son static: el generador de prototipos del .ino no las declara, asi
// que quien las use tiene que ir detras de ellas.
void openDexEntry(int16_t dex) {
  if (dex < 1 || dex > DEX_COUNT) return;
  galleryOpen = true;
  galleryPage = (dex - 1) / 16;   // para que salir del detalle caiga bien
  galleryDetail = dex;
  galleryCalcVers(dex);
  galleryCargaVer(dex);
  galleryDirty = true;
}

void galleryTap(int16_t x, int16_t y) {
  if (galleryDetail) {
    // Tocar la FICHA pasa de pagina; tocar fuera de ella vuelve a la
    // rejilla, que es lo que hacia antes toda la pantalla. Sin separar
    // las dos zonas, seguir leyendo te sacaba de la ficha.
    // la flecha de version ANTES que nada: cae fuera de la caja del texto,
    // pero dentro de la zona que cierra la ficha
    if (galleryNVers > 1 && pet.isRegistered(galleryDetail) &&
        abs(x - GVER_X) <= GVER_R + 6 && abs(y - GVER_Y) <= GVER_R + 6) {
      galleryVer = (int8_t)((galleryVer + 1) % galleryNVers);
      galleryCargaVer(galleryDetail);
      sfxPlay(SFX_TAP);
      return;
    }
    if (x >= 70 && x <= 396 && y >= 306 && y <= 390 &&
        pet.isRegistered(galleryDetail)) {
      dexPasaPagina(galleryDetail);
      sfxPlay(SFX_TAP);
      return;
    }
    // Leva 72: o botao da CACADA (visto e nao registrado). A caixa inteira
    // e dele —— errar o botao por pouco nao fecha a ficha (o "foso").
    if (pet.isSeen(galleryDetail) && !pet.isRegistered(galleryDetail)) {
      const bool noBotao = x >= CACA_BX && x <= CACA_BX + CACA_BW &&
                           y >= CACA_BY - 8 && y <= CACA_BY + CACA_BH;
      if (noBotao && pet.cacaDex == galleryDetail) {
        pet.setCaca(0);                       // PARAR
        sfxPlay(SFX_TAP);
        return;
      }
      if (noBotao && pet.podeCacar(galleryDetail, 12)) {
        pet.setCaca(galleryDetail);           // CACAR (troca o anterior)
        sfxPlay(SFX_TAP);
        return;
      }
      if (!lejosDe(x, y, 70, 290, 396, 374)) return;
    }
    // un pelo al lado de la caja del texto, o de la flecha de version: nada
    if (pet.isRegistered(galleryDetail)) {
      if (!lejosDe(x, y, 70, 306, 396, 390)) return;
      if (galleryNVers > 1 &&
          !lejosDe(x, y, GVER_X - GVER_R - 6, GVER_Y - GVER_R - 6,
                   GVER_X + GVER_R + 6, GVER_Y + GVER_R + 6))
        return;
    }
    galleryDetail = 0;
    galleryPmd.unload();
    galleryDirty = true;
    return;
  }
  if (y < GAL_Y - UI_FOSO) {  // tocar la cabecera = salir (lejos de la rejilla)
    galleryOpen = false;
    galleryPmd.unload();
    return;
  }
  // mismo motivo que en el teclado: filtrar antes de dividir
  if (x < GAL_X || y < GAL_Y) return;
  int c = (x - GAL_X) / GAL_CELL, r = (y - GAL_Y) / GAL_CELL;
  if (c > 3 || r > 3) return;
  int16_t dex = galleryPage * 16 + r * 4 + c + 1;
  if (dex > DEX_COUNT) return;
  galleryDetail = dex;
  // Se abre por el NORMAL, no por el shiny. Antes ponia
  // `load(dex, pet.isShinyRegistered(dex))`: teniendo uno shiny registrado
  // la ficha enseñaba el shiny y punto, sin manera de ver el de siempre.
  galleryCalcVers(dex);
  galleryCargaVer(dex);
}

void drawBattery() {
  int pc = batPercent();
  if (pc < 0) return;  // sin bateria conectada
  int x = CX - 14, y = 12, w = 24, h = 11;
  bool charging = batCharging();
  uint16_t col = charging ? UI_BAR_OK
                 : (pc >= 40) ? inkColor()
                 : (pc >= 15) ? UI_BAR_WARN
                              : UI_BAR_BAD;
  gfx->drawRoundRect(x, y, w, h, 2, col);
  gfx->fillRect(x + w, y + 3, 3, 5, col);  // borne
  if (charging) {
    // rayo de carga (zigzag) en vez de la barra de nivel
    uint16_t bolt = C565(0xff, 0xd9, 0x4a);
    int bx = x + w / 2;
    gfx->fillTriangle(bx + 3, y + 1, bx - 4, y + 6, bx + 1, y + 6, bolt);
    gfx->fillTriangle(bx - 1, y + 5, bx + 4, y + 5, bx - 3, y + 10, bolt);
  } else {
    int fw = (w - 4) * pc / 100;
    if (fw > 0) gfx->fillRect(x + 2, y + 2, fw, h - 4, col);
  }
}

// Texto con contorno oscuro: cuatro pasadas desplazadas y la buena encima.
// Hace falta desde que el fondo es una FOTO â€” el nombre en color de tipo
// sobre el volcan del Anil no se leia â€” y es lo que hacen los juegos con el
// texto que va suelto sobre el escenario.
// Cuantos pixeles de ANCHO se ven a esa altura, con margen para el
// bisel. La pantalla es un circulo de radio 233: arriba y abajo cabe
// mucho menos que en el centro, y eso es lo que corta los titulos.
// La punta de flecha de "hay mas texto". Pequena y en la esquina de
// abajo a la derecha, que es donde la busca el ojo al acabar de leer.
// Parpadea despacio para que se note sin molestar.
void flechaMas(int x, int y, uint16_t col) {
  if ((millis() / 500) % 2) return;
  gfx->fillTriangle(x, y - 5, x + 9, y, x, y + 5, col);
}

int anchoVisible(int y) {
  int dy = y - 233;
  int v = 233 * 233 - dy * dy;
  return v > 0 ? 2 * (int)sqrtf((float)v) - 16 : 0;
}

void printOutlined(int x, int y, const char *s, uint8_t talla, uint16_t col) {
  static const int8_t D[4][2] = { { -1, 0 }, { 1, 0 }, { 0, -1 }, { 0, 1 } };
  setSize(talla);
  gfx->setTextColor(INK_K);
  for (auto &d : D) { setCur(x + d[0], y + d[1]); printT(s); }
  gfx->setTextColor(col);
  setCur(x, y);
  printT(s);
}

void drawHeader(const char *name, uint16_t nameColor, const char *msg, bool shiny) {
  drawBattery();
  // La estrella y el nombre se centran JUNTOS: si solo se centrase el nombre,
  // un bicho shiny aparentaria estar descolocado a la derecha.
  int hueco = shiny ? ART_SHINY.w + 6 : 0;
  // A tamano 3 fijo se salia: "FLORIZARRE Niv.25" mide 306 px y a esa
  // altura el circulo solo da 294. No es un caso raro —— pasa en cuanto
  // el nombre es largo o el idioma escribe "Niv." en vez de "Lv.".
  uint8_t ts = tallaQueCabe(name, 52, 3, hueco);
  int x = (466 - (textW(name, ts) + hueco)) / 2;
  if (shiny) drawShinyStar(ART_SHINY, x + ART_SHINY.w / 2, 61);
  printOutlined(x + hueco, ts == 3 ? 52 : 56, name, ts, nameColor);
  uint8_t ms = tallaQueCabe(msg, 90, 2, 0);
  printOutlined(centerX(msg, ms), 90, msg, ms, UI_WHITE);
}

// animacion de la ceremonia (10s): despedida = reverencia con corazones y se
// aleja caminando; escapada = se asusta y sale corriendo. Sustituye al idle.
void drawCeremony() {
  if (!pmd.loaded) { drawPet(); return; }  // respaldo si no hay sprite PMD
  uint32_t now = millis();
  float t = pet.ceremonyT();               // 0..1 a lo largo de los 10s
  bool panic = (pet.ceremony == CER_RUNAWAY);
  int x = CX, y = PET_GROUND;
  uint8_t act = PMD_IDLE;

  if (panic) {
    // final triste: penumbra azulada + lluvia
    for (int i = 0; i < 46; i++) {
      int rx = (i * 47 + now / 3) % 466;
      int ry = (i * 91 + now / 2) % 470;
      gfx->drawLine(rx, ry, rx - 3, ry + 12, C565(0x6a, 0x84, 0xb0));
    }
    bool fade = false;
    if (t < 0.30f) {                       // cabizbajo, temblando
      act = pmd.has(PMD_HURT) ? PMD_HURT : PMD_IDLE;
      x = CX + (int)(4 * sinf(now * 0.04f));
    } else {                               // se aleja despacio y se desvanece
      act = pmd.has(PMD_WALKL) ? PMD_WALKL : PMD_IDLE;
      x = CX - (int)(((t - 0.30f) / 0.70f) * (CX + 120));
      fade = (t > 0.6f) && ((now / 160) % 2 == 0);  // parpadea hacia la silueta
    }
    drawPmdAct(act, x, y, now, true, fade, 5);  // fade=silueta: se difumina al irse
    // lagrima cayendo del bicho
    if (t < 0.55f) {
      int ty = y - 150 + (int)((now / 6) % 40);
      gfx->fillRect(x + 6, ty, 3, 6, C565(0x9a, 0xc4, 0xe8));
    }
    return;
  }

  // despedida epica: halo dorado pulsante + chispas y corazones que ascienden
  int gcy = PET_GROUND - 96;
  for (int k = 0; k < 4; k++) {
    int r = 60 + k * 34 + (int)(10 * sinf(now * 0.02f));
    gfx->drawCircle(CX, gcy, r, C565(0xff, 0xdf, 0x8a));
  }
  for (int i = 0; i < 16; i++) {
    int px = (i * 71 + 28) % 466;
    int py = 410 - (int)((now / 8 + i * 70) % 360);   // suben y reaparecen abajo
    if (py < 30) continue;
    if (i % 4 == 0) drawMap(SPR_HEART, 32, px - 8, py - 8, 1, false);  // corazoncito
    else gfx->fillRect(px, py, 4, 4, (i % 2) ? C565(0xff, 0xe7, 0x9f) : C565(0xff, 0x9a, 0xc0));
  }

  if (t < 0.45f) {                         // reverencia / pose de despedida
    act = pmd.has(PMD_POSE) ? PMD_POSE : (pmd.has(PMD_NOD) ? PMD_NOD : PMD_IDLE);
  } else {                                 // se aleja por la derecha
    act = pmd.has(PMD_WALKR) ? PMD_WALKR : PMD_IDLE;
    x = CX + (int)(((t - 0.45f) / 0.55f) * (CX + 140));
  }
  drawPmdAct(act, x, y, now, true, false, 5);
  if (pet.showHeart())                     // corazon grande siguiendo al bicho
    drawMap(SPR_HEART, 32, x + 50, y - 190, 2, false);
}

// dialogo de decision (2 botones apilados): evolucionar/mantener o despedirse/quedaros
void drawChoiceDialog() {
  const char *q, *o1, *o2;
  uint16_t c1, c2, t1, t2;
  if (choiceKind == 1) {  // evolucion
    q = T(S_EVO_Q); o1 = T(S_EVO_TAP); o2 = T(S_EVO_KEEP);
    c1 = C565(0xe0, 0x50, 0x38); t1 = UI_WHITE; c2 = C565(0x58, 0x70, 0x98); t2 = UI_WHITE;
  } else if (choiceKind == 3) {  // escapada: quedarse arriba, irse abajo
    q = T(S_RUN_Q); o1 = T(S_RUN_STAY); o2 = T(S_RUN_GO);
    c1 = UI_BAR_OK; t1 = UI_WHITE;
    c2 = C565(0x3a, 0x44, 0x5a); t2 = C565(0xc8, 0xd2, 0xe0);
  } else {                // despedida
    q = T(S_FAR_Q); o1 = T(S_FAR_GO); o2 = T(S_FAR_STAY);
    c1 = C565(0xd8, 0x98, 0x20); t1 = UI_INK; c2 = UI_BAR_OK; t2 = UI_WHITE;
  }
  // La despedida es DEFINITIVA, asi que el cuadro es mas alto y ensena lo
  // que vas a recibir a cambio ANTES de confirmar. Sin esto es una apuesta
  // a ciegas, y una decision a ciegas no es una decision.
  bool adios = (choiceKind == 2);
  // La despedida sube y crece: lleva tres lineas (pregunta, que huevo
  // deja y la calidad) y los botones estan fijos en 206 y 268. Con la
  // caja en 132 los puntos de calidad caian justo detras del primer
  // boton y no se veian.
  // leva 71: la ventana dorada del juego (la misma de la insignia) y la
  // letra de DS. La evolucion sube para ensenar DE QUIEN a QUIEN.
  bool evo = (choiceKind == 1);
  int cy = adios ? 110 : evo ? 92 : 148, ch = adios ? 230 : evo ? 246 : 194;
  drawWindow(58, cy, 350, ch, 18);
  // A talla 3 si cabe ("o texto estava mto pequeno").
  uint8_t qs = tituloTalla(q, 3, 300);
  printTituloC(q, cy + 18, qs, UI_INK, C565(0xc8, 0xc0, 0xa8));
  if (evo) {
    int16_t de = pet.speciesId, a = DEX_TBL[de].evolvesTo;
    const uint8_t *t1 = thumbs.get(de), *t2 = a > 0 ? thumbs.get(a) : nullptr;
    int ty = cy + 80;
    if (t1) drawThumbFit(t1, CX - 82, ty, 72, false);
    if (t2) drawThumbFit(t2, CX + 82, ty, 72, !pet.isRegistered(a));
    // a seta, com o brilho da evolucao a pulsar
    uint16_t sc = mezcla565(C565(0xff, 0xb0, 0x30), 0xFFFF,
                            (uint8_t)(110 + 100 * sinf(millis() * 0.008f)));
    gfx->fillTriangle(CX + 18, ty, CX - 8, ty - 16, CX - 8, ty + 16, INK_K);
    gfx->fillRect(CX - 30, ty - 6, 24, 12, INK_K);
    gfx->fillTriangle(CX + 14, ty, CX - 6, ty - 12, CX - 6, ty + 12, sc);
    gfx->fillRect(CX - 28, ty - 4, 24, 8, sc);
  }
  if (adios) {
    // que huevo deja, y de que calidad
    char h[40];
    snprintf(h, sizeof(h), T(S_OVO_FMT), dexName(pet.speciesId));
    gfx->setTextColor(UI_BAR_OK);
    uint8_t hs = textW(h, 2) <= 296 ? 2 : 1;
    setSize(hs);
    setCur(centerX(h, hs), cy + 56);
    printT(h);
    uint8_t s = pet.legacyScore();
    int x0 = CX - (LEGACY_SCORE_MAX * 16) / 2 + 8;
    for (uint8_t i = 0; i < LEGACY_SCORE_MAX; i++) {
      if (i < s) gfx->fillCircle(x0 + i * 16, cy + 86, 5, UI_BAR_WARN);
      else gfx->drawCircle(x0 + i * 16, cy + 86, 5, UI_TRACK);
    }
  }
  // Los MISMOS sitios de siempre (choiceTap mira 206 y 268).
  (void)t1;
  (void)t2;
  drawBotaoCartel(93, 206, 280, 52, c1, o1);        // boton accion
  drawBotaoCartel(93, 268, 280, 52, c2, o2);        // boton mantener/quedaros
}

// boton-CTA rojo y grande para evolucionar (pulsa para llamar la atencion)
// El recuadro de "esto va a evolucionar", con las DOS miniaturas y una
// flecha en medio.
//
// Antes era un rectangulo con la palabra EVOLVE dentro y no decia nada:
// ni en que se convierte ni de quien se habla. Ahora se ve de quien sale y
// en que acaba, que es justo lo que uno quiere saber antes de pulsar.
//
// Lo que TODAVIA no has criado sale en SILUETA, como en la Pokedex: la
// gracia de evolucionar es parte sorpresa, y ensenarlo a color la quita.
// En cuanto lo tengas registrado, la proxima vez ya sale entero.
static void drawEvoPanel(int x, int y, int w, int h, uint16_t fondo,
                         int16_t deDex, int16_t aDex, const char *titulo,
                         const char *pie) {
  gfx->fillRoundRect(x, y, w, h, 16, fondo);
  gfx->drawRoundRect(x, y, w, h, 16, UI_WHITE);
  gfx->drawRoundRect(x + 2, y + 2, w - 4, h - 4, 14, UI_WHITE);

  int cy = y + h / 2;
  int caja = h - 20;
  if (caja > 34) caja = 34;
  const uint8_t *t1 = thumbs.get(deDex);
  const uint8_t *t2 = thumbs.get(aDex);
  int x1 = x + 12 + caja / 2;
  int x2 = x + w - 12 - caja / 2;
  if (t1) drawThumbFit(t1, x1, cy, caja, false);
  if (t2) drawThumbFit(t2, x2, cy, caja, !pet.isRegistered(aDex));

  // la flecha, justo a la izquierda de la segunda miniatura
  int fx = x2 - caja / 2 - 14;
  gfx->fillTriangle(fx + 10, cy, fx - 2, cy - 7, fx - 2, cy + 7, UI_WHITE);
  gfx->fillRect(fx - 10, cy - 3, 9, 7, UI_WHITE);

  // El texto solo si de verdad cabe. En la chapa pequena no hay sitio: las
  // dos miniaturas y la flecha YA dicen "esto se convierte en esto", que es
  // el idioma de toda la saga y no necesita traduccion.
  if (!titulo) return;
  int tcx = (x1 + caja / 2 + fx - 12) / 2;
  gfx->setTextColor(UI_WHITE);
  setSize(2);
  setCur(tcx - textW(titulo, 2) / 2, cy - (pie ? 13 : 8));
  printT(titulo);
  if (pie) {
    setSize(1);
    setCur(tcx - textW(pie, 1) / 2, cy + 7);
    printT(pie);
  }
}

void drawEvolveButton() {
  uint32_t now = millis();
  int p = (int)(5 * sinf(now * 0.006f));  // late: -5..5
  // el latido, mas suave: una chapa pequena dando saltos de cinco pixeles
  // en una esquina se lee como un fallo, no como un aviso
  p = (int)(2 * sinf(now * 0.006f));
  int x = EVO_BTN_X - p, y = EVO_BTN_Y - p, w = EVO_BTN_W + 2 * p, h = EVO_BTN_H + 2 * p;
  drawEvoPanel(x, y, w, h, UI_BAR_BAD, pet.speciesId,
               DEX_TBL[pet.speciesId].evolvesTo, nullptr, nullptr);
}

// CTA de "a uno del equipo le toca evolucionar".
//
// Estuvo un rato en la pagina de equipo, como un circulito en su fila, y
// fue mala idea por dos motivos: cae DENTRO de la fila, que ya significa
// "abrir sus golpes" y tiene la flecha del banco al lado, asi que o no se
// acertaba o se acertaba otra cosa; y ademas sus medidas se llamaban
// EVO_BTN_X, que YA era el CTA de evolucion del principal — redefinir esa
// macro movio tambien el boton del principal. Dos fallos de un tiro.
//
// Aqui no hay nada de eso: es el mismo sitio y el mismo recuadro que los
// otros CTA de la pantalla de cuidado, y va el ULTIMO de la cadena porque
// lo del bicho que crias manda sobre lo del banquillo.
void drawTeamEvolveButton() {
  uint8_t slot = pet.reserveReadyToEvolve();
  if (slot == 0xFF) return;
  uint32_t now = millis();
  int p = (int)(2 * sinf(now * 0.005f));
  int x = EVO_BTN_X - p, y = EVO_BTN_Y - p;
  int w = EVO_BTN_W + 2 * p, h = EVO_BTN_H + 2 * p;
  int16_t de = pet.team[slot].species;
  drawEvoPanel(x, y, w, h, UI_BAR_OK, de, DEX_TBL[de].evolvesTo, nullptr, nullptr);
}

// boton-CTA dorado de despedida: "<nombre> quiere decirte algo..."
void drawFarewellButton() {
  uint32_t now = millis();
  int p = (int)(4 * sinf(now * 0.005f));
  int x = FAR_BTN_X - p, y = FAR_BTN_Y - p, w = FAR_BTN_W + 2 * p, h = FAR_BTN_H + 2 * p;
  gfx->fillRoundRect(x, y, w, h, 16, UI_BAR_WARN);
  gfx->drawRoundRect(x, y, w, h, 16, UI_INK);
  char buf[52];
  const char *nm = pet.nick[0] ? pet.nick : dexName(pet.speciesId);
  snprintf(buf, sizeof(buf), T(S_FAREWELL_BTN), nm);
  gfx->setTextColor(UI_INK);
  setSize(2);
  setCur(centerX(buf, 2), y + h / 2 - 8);
  printT(buf);
}

// boton-CTA sombrio de escapada por abandono: "<nombre> se siente abandonado..."
// (final triste: azul-gris oscuro, latido lento y apagado)
void drawRunawayButton() {
  uint32_t now = millis();
  int p = (int)(3 * sinf(now * 0.003f));
  int x = FAR_BTN_X - p, y = FAR_BTN_Y - p, w = FAR_BTN_W + 2 * p, h = FAR_BTN_H + 2 * p;
  gfx->fillRoundRect(x, y, w, h, 16, C565(0x3a, 0x44, 0x5a));
  gfx->drawRoundRect(x, y, w, h, 16, C565(0x70, 0x80, 0x98));
  char buf[52];
  const char *nm = pet.nick[0] ? pet.nick : dexName(pet.speciesId);
  snprintf(buf, sizeof(buf), T(S_RUNAWAY_BTN), nm);
  gfx->setTextColor(C565(0xc8, 0xd2, 0xe0));
  setSize(2);
  setCur(centerX(buf, 2), y + h / 2 - 8);
  printT(buf);
}

// animacion epica de evolucion: halo radial + rayos giratorios + parpadeo del
// sprite acelerando + chispas que salen disparadas + fogonazo final
// El telon de la evolucion: el resplandor de arcos concentricos de
// UI/evolution_bg.gif del Anil, rehecho a mano. La hoja son DIEZ fotogramas
// de 512x384: empaquetarla serian 4 MB de tarjeta para un efecto de ocho
// segundos, y es un degradado con anillos â€” se dibuja mas barato de lo que
// se guarda.
static void drawEvolveBackdrop(uint32_t now, float t) {
  gfx->fillScreen(RGB565_BLACK);
  int cx = CX, cy = 430;            // el foco va por debajo, como el original
  // Blanco calido en el nucleo, amarillo, naranja y negro fuera. En tres
  // tramos y no en una sola interpolacion: de blanco a negro en linea recta
  // se pasa por un gris azulado que no tiene nada que ver con el original
  // (lo tenia, y se veia).
  for (int r = 470; r > 0; r -= 6) {
    int k = r * 255 / 470;                 // 0 dentro, 255 fuera
    uint16_t col;
    if (k < 90)       col = lerp565(C565(0xff, 0xf6, 0xd8), C565(0xff, 0xcc, 0x38), k, 90);
    else if (k < 170) col = lerp565(C565(0xff, 0xcc, 0x38), C565(0xe0, 0x5c, 0x00), k - 90, 80);
    else              col = lerp565(C565(0xe0, 0x5c, 0x00), RGB565_BLACK, k - 170, 85);
    gfx->fillCircle(cx, cy, r, col);
  }
  // los aros claros que viajan hacia fuera
  int fase = (int)((now / 40 + (uint32_t)(t * 200)) % 46);
  for (int r = fase; r < 470; r += 46)
    gfx->drawCircle(cx, cy, r, C565(0xff, 0xf2, 0xc0));
}

void drawEvolveFX(uint32_t now) {
  float t = pet.evolveT();          // 0..1
  drawEvolveBackdrop(now, t);
  int cx = CX, cy = PET_GROUND - 96;

  // halo radial que crece y pulsa
  int halo = 36 + (int)(t * 150) + (int)(8 * sinf(now * 0.02f));
  for (int k = 0; k < 4; k++) {
    int r = halo - k * 7;
    if (r > 0) gfx->drawCircle(cx, cy, r, UI_WHITE);
  }
  // rayos giratorios desde el centro del bicho
  float base = now * 0.004f;
  for (int i = 0; i < 12; i++) {
    float a = base + i * (float)(PI / 6);
    int len = 90 + (int)(70 * (0.5f + 0.5f * sinf(now * 0.012f + i)));
    gfx->drawLine(cx, cy, cx + (int)(cosf(a) * len), cy + (int)(sinf(a) * len), UI_WHITE);
  }
  // parpadeo entre la forma ANTERIOR y la NUEVA (siluetas), acelerando; al
  // final (t>0.9) se queda fija en la nueva para el fogonazo de revelado
  int period = 60 + (int)(220 * (1.0f - t));
  bool showOld = t < 0.9f && evoPmd.loaded && ((now / period) % 2) == 0;
  // cada forma a SU tamano: asi evolucionar se ve como crecer
  if (showOld) drawPmdActM(evoPmd, PMD_IDLE, cx, PET_GROUND, 0, true, true, 5,
                           nullptr, evoFromDex);
  else drawPmdAct(PMD_IDLE, cx, PET_GROUND, 0, true, true, 5);
  // chispas que salen disparadas
  for (int i = 0; i < 10; i++) {
    float a = i * (float)(PI / 5) + t * 4.0f;
    int d = (int)((now / 14 + i * 33) % 200);
    int sx = cx + (int)(cosf(a) * d), sy = cy + (int)(sinf(a) * d);
    gfx->fillRect(sx - 2, sy - 2, 5, 5, (i & 1) ? C565(0xff, 0xe0, 0x70) : UI_WHITE);
  }
  // fogonazo final antes de revelar la forma nueva
  if (t > 0.9f) gfx->fillCircle(cx, cy, (int)(300 * (t - 0.9f) / 0.1f), UI_WHITE);
}

void drawPet() {
  if (pmd.loaded) {
    drawPetPMD();
    return;
  }
  if (mon.loaded) {
    drawPetSD();
    return;
  }
  int fi = flashIdxForDex(pet.speciesId);
  if (fi < 0) {
    // sin SD y sin sprite de flash: aviso claro de que faltan sprites
    gfx->setTextColor(inkColor());
    setSize(6);
    setCur(CX - 18, PET_CY - 80);
    printT("?");
    setSize(2);
    const char *l1 = T(S_NO_SPRITES);
    setCur(centerX(l1, 2), PET_CY - 4);
    printT(l1);
    const char *l2 = T(S_LOAD_SPRITES);
    setCur(centerX(l2, 2), PET_CY + 20);
    printT(l2);
    return;
  }
  const Species &sp = SPECIES[fi];
  int s = sp.scale;
  int x = CX - 16 * s;
  int y = PET_CY - 16 * s;

  // animacion de evolucion: alterna la silueta de la forma anterior y la nueva
  if (pet.evolving()) {
    bool flash = (millis() / 300) % 2;
    int16_t showDex = (flash && pet.prevSpeciesId >= 0) ? pet.prevSpeciesId : pet.speciesId;
    int sfi = flashIdxForDex(showDex);
    if (sfi >= 0) {
      const Species &show = SPECIES[sfi];
      drawMap(show.sprite, SPRITE_H, CX - 16 * show.scale, PET_CY - 16 * show.scale, show.scale, flash);
    }
    return;
  }

  PetMood m = pet.mood();
  if (m == MOOD_HAPPY && (millis() / 500) % 2) y -= 6;  // saltito

  drawMap(sp.sprite, SPRITE_H, x, y, s, false);

  // expresiones superpuestas usando las anclas de la especie
  bool blink = (millis() % 3500 < 300);
  if (m == MOOD_SLEEPING || blink) {
    overlayEye(sp, x, y, s, sp.eyeColL);
    overlayEye(sp, x, y, s, sp.eyeColR);
  }
  if (m == MOOD_EATING) overlayMouth(sp, x, y, s, true);
  else if (m == MOOD_SAD) overlayMouth(sp, x, y, s, false);

  if (pet.showHeart()) drawMap(SPR_HEART, 32, x + 20 * s, y - 2 * s, 2, false);
}

// ---------- escena de bano ----------

void startBath() {
  if (pet.isEgg() || pet.sleeping || pet.ceremony || bathUntil) return;
  bathUntil = millis() + BATH_MS;
  bathPending = true;
  int cx = (int)beh.x;
  for (auto &b : bubbles) {
    b.x = cx - 70 + random(140);
    b.y = PET_GROUND - random(40);
    b.r = 7 + random(13);
    b.ph = random(64);
  }
}

void drawBath() {
  uint32_t now = millis();
  if (!timeLeft(bathUntil)) {
    bathUntil = 0;
    if (bathPending) {
      bathPending = false;
      pet.clean();
      // pose de alegria al quedar limpio
      if (pmd.has(PMD_POSE)) {
        beh.mode = 2;
        beh.act = PMD_POSE;
        beh.t0 = now;
        beh.until = now + pmdActTotalMs(pmd.acts[PMD_POSE]) * 2;
      }
    }
    return;
  }
  // As bolhas: cada uma nasce no seu tempo, sobe balancando, cresce e estoura.
  // Vidro de verdade: transparente, com a borda clara e o brilho do lado da
  // luz. No fim, o brilho do Anil em quem ficou limpo.
  const uint32_t e = BATH_MS - (bathUntil - now);
  for (auto &b : bubbles) {
    const uint32_t t0 = (uint32_t)b.ph * 24;   // 0..1,5 s
    const uint32_t vida = 900 + (uint32_t)b.r * 20;
    if (e < t0) continue;
    const float f = (float)(e - t0) / vida;
    const int bx = b.x + (int)(sinf((e + b.ph * 40) / 240.0f) * (4 + b.r / 3));
    const int by = b.y - (int)(f * 130.0f);
    if (f < 1.0f) {
      const int r = (int)(b.r * (0.55f + 0.45f * f));
      tintEllipse(bx, by, r, r, 3, 10, true);
      gfx->drawCircle(bx, by, r, C565(0xc8, 0xee, 0xff));
      gfx->drawCircle(bx, by, r - 1, C565(0x7c, 0xc8, 0xf0));
      gfx->fillCircle(bx - r * 2 / 5, by - r * 2 / 5, r / 4 + 1, UI_WHITE);
      gfx->fillCircle(bx + r * 2 / 5, by + r * 2 / 5, r / 8 + 1, C565(0xe0, 0xf6, 0xff));
    } else if (f < 1.22f) {
      // estoura: um anel que abre e quatro gotinhas
      const float p = (f - 1.0f) / 0.22f;
      const int r = b.r + (int)(p * 10);
      gfx->drawCircle(bx, by, r, C565(0xc8, 0xee, 0xff));
      for (int k = 0; k < 4; k++) {
        const float a = k * 1.57f + 0.6f;
        gfx->fillCircle(bx + (int)(cosf(a) * r), by + (int)(sinf(a) * r), 1, UI_WHITE);
      }
    }
  }
  if (e > BATH_MS - 700) {
    static const int8_t S[3][2] = { { -46, -60 }, { 38, -92 }, { 6, -30 } };
    const uint32_t q = (e - (BATH_MS - 700)) / 70;
    if (q < ART_SPARK_FRAMES && ART_SPARK[q])
      for (auto &p : S) drawArt(*ART_SPARK[q], (int)beh.x + p[0], PET_GROUND + p[1], 2);
  }
}

// ---------- mascota PMD: comportamiento ----------

uint32_t pmdActTotalMs(const PmdAct &a) {
  uint32_t t = 0;
  for (uint8_t i = 0; i < a.frames; i++) t += a.ms(i);
  return t ? t : 100;
}

uint8_t pmdFrameAt(const PmdAct &a, uint32_t t, bool loop) {
  uint32_t total = pmdActTotalMs(a);
  if (!loop && t >= total) return a.frames - 1;
  t %= total;
  uint8_t i = 0;
  // guarda: nunca mas vueltas que frames, por si un .bin corrupto trae una
  // duracion 0 pese al saneo de PmdAct::ms() (defensa en profundidad)
  for (uint8_t guard = 0; guard < a.frames && t >= a.ms(i); guard++) {
    t -= a.ms(i);
    i = (i + 1) % a.frames;
  }
  return i;
}

// dibuja una accion anclada por la base (centro-x, suelo) y devuelve su escala
// dibuja una accion de un PmdMon concreto (m); drawPmdAct usa el global pmd
//
// Si se pasa `fx`, el sprite se deforma al dibujarlo. Los battlers de Anil solo
// traen UN angulo animado, asi que el peso del movimiento (andar, respirar,
// encogerse al dormir, encajar un golpe) no puede salir de los sprites: sale de
// aqui. Es una sola transformacion alimentada con curvas distintas por estado,
// asi que "anda" y "salta" dejan de ser dos features y pasan a ser dos curvas.
void drawPmdActM(PmdMon &m, uint8_t actId, int cx, int groundY, uint32_t t, bool loop, bool sil, uint8_t maxS,
                 const PetFx *fx, int16_t dex) {
  const PmdAct &a = m.acts[actId];
  if (!a.frames) return;
  // El alto que pide la especie y la escala con la que se llega a el: las
  // dos salen de los mismos tres helpers en todas partes (casa, Pokedex,
  // evolucion, combate). Cuando esta cuenta estaba copiada en tres sitios,
  // los tres se desincronizaron.
  float quiere = altoPedido(dex, HOME_K);
  // Con dex == 0 el que llama ya hizo la cuenta y manda la escala en
  // `maxS`: ahi no es un techo, es LA escala. Pasarla por escalaDibujo
  // la rebajaria con la heuristica vieja (170/lienzo) y un Wailord de
  // espaldas saldria a la mitad.
  uint8_t s = (dex == 0) ? (maxS ? maxS : 1) : escalaDibujo(m, a, maxS, quiere);
  uint8_t fi = pmdFrameAt(a, t, loop);
  const uint8_t *fr = a.data + (uint32_t)fi * a.w * a.h;
  int pivot = a.base ? a.base : a.h;  // fila de los pies

  // A animacao do Anil pode TINGIR o bicho: monta-se a paleta tingida uma
  // vez, e a opacidade vai por retangulo. A sombra nunca se tinge.
  const bool tinge = gTinta.on && !gSombra.on && !sil;
  static uint16_t palT[256];
  const uint16_t *pal = m.pal;
  if (tinge && (gTinta.cor[3] || gTinta.tom[0] || gTinta.tom[1] ||
                gTinta.tom[2] || gTinta.tom[3])) {
    for (uint16_t i = 0; i < m.palCount && i < 256; i++)
      palT[i] = tingeCor(m.pal[i], gTinta.cor, gTinta.tom);
    pal = palT;
  }
  const uint8_t opac = tinge ? gTinta.op : 255;

  // El TAMANO DE LA ESPECIE se aplica aqui, no en quien llama. Antes la
  // correccion la metia UN SOLO sitio — la animacion de la pantalla de
  // casa — y las otras seis llamadas pintaban normalizado: la ficha, la
  // Pokedex, las ceremonias y la EVOLUCION.
  //
  // De ahi lo de "Kakuna gigante y Beedrill pequeno", y que al evolucionar
  // pareciera encogerse: en crudo Kakuna sale a 160 px (40x4) y Beedrill a
  // 140 (70x2), o sea que la evolucion lo hacia MAS PEQUENO.
  PetFx propio;
  float talla = factorTinta(a, s, quiere, HOME_ALTO_MAX);
  if (talla != 1.0f) {
    if (fx) propio = *fx;
    propio.sx *= talla;
    propio.sy *= talla;
    fx = &propio;
  }
  // Sombra: o alto CHEIO (antes de achatar) da a inclinacao em px; o
  // pulo (dy) e o balanco (lean) do bicho nao vao para a sombra —— ela
  // fica no chao enquanto ele sobe.
  float sombraLean = 0.0f;
  if (gSombra.on) {
    if (fx != &propio) propio = fx ? *fx : PetFx();
    sombraLean = a.inkH() * s * propio.sy * gSombra.inclina;
    propio.sy *= gSombra.achata;
    propio.dy = 0.0f;
    propio.lean = 0.0f;
    propio.clipY = 0x7FFF;
    fx = &propio;
  }

  // ALTO DE CUERPO QUE SE ACABA DE PINTAR. Lo escribe el dibujante y no
  // quien llama, y eso no es un detalle de estilo: la primera version de
  // esta sonda vivia en btDrawSide y media lo que btDrawSide habia
  // calculado, no lo que drawPmdActM anadia por su cuenta —— que era
  // justo donde estaba el fallo. El test se quedo VERDE con el fallo
  // puesto a proposito. Una sonda en el sitio equivocado es peor que
  // ninguna: da por cubierto lo que no mira.
  if (!gSombra.on) {   // a sombra nao e o bicho: nao mexe na sonda
    float sy = fx ? fx->sy : 1.0f;
    ultimaMagPintada = (int16_t)(a.mag * s * sy + 0.5f);
    ultimoAltoPintado = (int16_t)(a.inkH() * s * sy + 0.5f);
  }

  // Sin deformacion: el camino de siempre, un fillRect por pixel.
  if (!fx) {
    // anclar por los pies (a.base), no por el alto del lienzo: asi las acciones
    // con padding distinto (Hurt, Eat...) quedan todas a la misma altura de suelo
    int x0 = cx - a.w * s / 2, y0 = groundY - pivot * s;
    for (int r = 0; r < a.h; r++) {
      const uint8_t *row = fr + r * a.w;
      for (int c = 0; c < a.w; c++) {
        uint8_t idx = row[c];
        if (idx == 0xFF) continue;
        if (opac < 255) misturaRect(x0 + c * s, y0 + r * s, s, s, pal[idx], opac);
        else gfx->fillRect(x0 + c * s, y0 + r * s, s, s, sil ? INK_K : pal[idx]);
      }
    }
    return;
  }

  // Escalado por bordes: el borde derecho de una columna es el izquierdo de la
  // siguiente, asi que al achatar no aparecen ni huecos ni solapes de 1 px.
  int fullW = (int)(a.w * s * fx->sx + 0.5f);
  int fullH = (int)(a.h * s * fx->sy + 0.5f);
  if (fullW < 1 || fullH < 1) return;
  int x0 = cx - fullW / 2 + (int)(fx->dx + (fx->dx < 0 ? -0.5f : 0.5f));
  int y0 = groundY - (int)(pivot * s * fx->sy + 0.5f) + (int)(fx->dy + (fx->dy < 0 ? -0.5f : 0.5f));

  int16_t colX[257];
  for (int c = 0; c <= a.w; c++) colX[c] = (int16_t)(x0 + (c * fullW) / a.w);

  for (int r = 0; r < a.h; r++) {
    int yA = y0 + (r * fullH) / a.h;
    int hgt = (y0 + ((r + 1) * fullH) / a.h) - yA;
    if (hgt <= 0) continue;
    // Recorte: las filas van de arriba abajo, asi que en cuanto una empieza
    // por debajo de la linea ya no hay nada mas que pintar.
    if (yA >= fx->clipY) break;
    if (yA + hgt > fx->clipY) hgt = fx->clipY - yA;
    // Cizalla en vez de rotacion: desplazar filas enteras un numero ENTERO de
    // pixeles no reinterpola nada, asi que la pixel art sigue limpia (rotarla
    // la dentaria). El pie no se mueve y el desfase crece con la altura; y como
    // cada fila consulta la fase RETRASADA segun su altura, la parte de arriba
    // llega despues que el cuerpo: eso es lo que da el movimiento secundario
    // (la flor de Venusaur cimbrea detras de el) y quita el aire de fantasma.
    int dxRow = 0;
    if (fx->lean != 0.0f && pivot > 1) {
      float q = (float)(pivot - 1 - r) / (float)(pivot - 1);
      if (q > 0.0f) dxRow = (int)(fx->lean * sinf(fx->phase - fx->lag * q) * q + 0.5f);
    }
    if (sombraLean != 0.0f && pivot > 1) {
      float q = (float)(pivot - 1 - r) / (float)(pivot - 1);
      if (q > 0.0f) dxRow += (int)(sombraLean * q + 0.5f);
    }
    const uint8_t *row = fr + r * a.w;
    for (int c = 0; c < a.w; c++) {
      uint8_t idx = row[gEspelho ? a.w - 1 - c : c];
      if (idx == 0xFF) continue;
      int xA = colX[c] + dxRow;
      int wid = colX[c + 1] - colX[c];
      if (wid <= 0) continue;
      if (gSombra.on) tintRect(xA, yA, wid, hgt, gSombra.num, gSombra.den);
      else if (opac < 255) misturaRect(xA, yA, wid, hgt, pal[idx], opac);
      else gfx->fillRect(xA, yA, wid, hgt, sil ? INK_K : pal[idx]);
    }
  }
}
void drawPmdAct(uint8_t actId, int cx, int groundY, uint32_t t, bool loop, bool sil, uint8_t maxS,
                const PetFx *fx, int16_t dex) {
  drawPmdActM(pmd, actId, cx, groundY, t, loop, sil, maxS, fx, dex);
}

// elige el siguiente capricho del bicho cuando esta contento
void behNext() {
  uint32_t now = millis();
  beh.t0 = now;
  int r = random(100);
  if (r < 35 && (pmd.has(PMD_WALKL) || pmd.has(PMD_WALKR))) {
    beh.mode = 1;  // paseo
    beh.targetX = 150 + random(176);
    beh.until = now + 15000;
  } else if (r < 60) {
    // gesto aleatorio entre los disponibles
    // (Hop fuera: salta demasiado alto; Sit fuera: mira hacia atras)
    static const uint8_t flair[] = { PMD_POSE, PMD_NOD, PMD_BREATH };
    uint8_t pick[3], n = 0;
    for (uint8_t f : flair)
      if (pmd.has(f)) pick[n++] = f;
    if (n) {
      beh.mode = 2;
      beh.act = pick[random(n)];
      beh.until = now + pmdActTotalMs(pmd.acts[beh.act]);
      return;
    }
    beh.mode = 0;
    beh.until = now + 2000 + random(3000);
  } else {
    beh.mode = 0;  // mirar al frente
    beh.until = now + 2000 + random(3000);
  }
}

// declaradas a mano: el prototipado automatico del .ino no es de fiar con tipos
// propios (PetNeed, PetFx) ni con argumentos por defecto
// fase 0..2PI de un ciclo de `periodMs`. Con el modulo antes de convertir a
// float no hay salto al dar la vuelta millis() ni perdida de precision.
static inline float cyclePhase(uint32_t now, uint32_t periodMs) {
  return (float)(now % periodMs) * (2.0f * (float)PI / (float)periodMs);
}

// ------------------------------------------------- EL TAMANO DE UN BICHO
//
// Tres funciones y una sola regla: lo que se fija es el alto del CUERPO
// (la tinta), no el del lienzo. Costo cinco intentos llegar aqui, y los
// cinco fallaron por el mismo motivo —— ajustar un numero que no era el
// que el ojo lee:
//
//   1. `170 / alto_del_lienzo`: normalizaba. Un Eevee salia tan alto como
//      un Charizard, y un pixel de lienzo (56 contra 57) daba un 32% de
//      diferencia en pantalla por culpa de la division entera.
//   2. el alto del sprite del Anil: depende de la POSE. Pidgeotto esta
//      dibujado volando (116 px) y Pidgeot posado (68), asi que evolucionar
//      encogia. 55 evoluciones salian mas pequenas que su forma anterior.
//   3. correccion por densidad de tinta: un parche encima del lienzo, que
//      acertaba de media y no por bicho.
//   4. tope de altura por rol: aplastaba a 475 de 1025 rivales contra el
//      mismo numero, asi que el tamano dejaba de decir nada.
//   5. y el de verdad: el combate llamaba a dibujar SIN pasar `dex`, con
//      lo que el rival se escalaba con el screenSz del bicho del jugador.
//      Agrandar el tuyo agrandaba al rival. Ningun ajuste de la tabla
//      podia arreglar eso, porque el fallo no estaba en la tabla.
//
// Ahora DEX_TBL[].screenSz ES el alto de cuerpo que se quiere en la
// pantalla de casa, en pixeles (gen_sizes.py), y aqui solo se traduce a la
// escala entera + el factor fraccionario con que lo pinta drawPmdActM.

// Alto de CUERPO que pide esta especie, en pixeles de pantalla.
// Devuelve 0 cuando NO hay que tocar el tamano: huevo, especie invalida, o
// `dex == 0`, que significa "el que llama ya lo dio" y lo usa el combate.
float altoPedido(int16_t dex, float factor) {
  if (dex == 0) return 0.0f;          // el combate: ya trae su perspectiva
  if (dex < 0) {
    if (pet.isEgg()) return 0.0f;
    dex = pet.speciesId;
  }
  if (dex < 1 || dex > DEX_COUNT) return 0.0f;
  return DEX_TBL[dex].screenSz * factor;
}

// La escala ENTERA con la que se va a dibujar. Sale de lo que SE QUIERE y
// no de un numero fijo: asi el factor fraccionario de abajo se queda cerca
// de 1 y sus topes no llegan a morder. Con el 170 fijo de antes, un sprite
// ya grande de por si se pasaba de rosca y el recorte se comia la
// correccion entera sin decirlo.
uint8_t escalaDibujo(const PmdMon &m, const PmdAct &a, uint8_t maxS, float quiere) {
  uint8_t s;
  if (quiere >= 1.0f) {
    int v = (int)(quiere / a.mag + 0.5f);
    s = (uint8_t)(v < 1 ? 1 : (v > 255 ? 255 : v));
  } else {
    // sin tamano pedido (huevo, mega, forma): el reparto de siempre
    s = m.acts[PMD_IDLE].h ? (uint8_t)(170 / m.acts[PMD_IDLE].h) : 5;
  }
  // El minimo es 1 Y NO 2. Lo era porque la version vieja normalizaba con
  // `170/lienzo` y nunca pedia menos de 2; ahora la escala sale del tamano
  // pedido, y a un sprite grande dibujado pequeno (un rival al fondo) le
  // toca 1 de verdad. Con el minimo en 2 se le pedia el doble de lo que
  // cabe y lo arreglaba `k` bajando a su suelo de 0,45 —— o sea, saliendo a
  // otro tamano en silencio. Medido: le pasaba a 101 de las 1025.
  if (s < 1) s = 1;
  if (s > maxS) s = maxS;
  // acciones con el frame mas grande que el idle (ataque): que no se salgan
  while (s > 1 && a.h * s > 250) s--;
  return s;
}

// El factor fraccionario que lleva el CUERPO de esta accion al alto pedido,
// dada la escala entera con la que se va a pintar.
//
// Se mide `a`, la accion QUE SE DIBUJA, y no el idle: la vista de espaldas
// del combate pinta PMD_SLEEP, y su lienzo difiere del idle en 904 de las
// 1025 especies. Escalando con el idle, Dondozo pedia 145 px y salia a 214.
float factorTinta(const PmdAct &a, uint8_t s, float quiere, int altoMax) {
  if (quiere < 1.0f || !s) return 1.0f;
  float k = quiere / (a.mag * s);
  // Y QUE QUEPA. La magnitud dice cuanto SITIO tiene que ocupar, no cuanto
  // mide de alto: un sprite muy estirado que ocupe el sitio que le toca
  // puede salirse por arriba. Medido sobre las 1025, a 31 les pasa.
  //
  // El recorte aplasta —— todos los recortados salen al mismo alto —— asi
  // que la banda de gen_sizes.py esta elegida para que sean pocos, y
  // tools/audita_tamanos.py cuenta cuantos son.
  if (altoMax > 0) {
    float alto = a.inkH() * s * k;
    if (alto > altoMax) k = altoMax / (float)(a.inkH() * s);
  }
  // Red de seguridad, no una herramienta: con la escala sacada de `quiere`
  // esta franja no deberia recortar a nadie, y tools/audita_tamanos.py lo
  // comprueba especie a especie. Esta por si un fichero raro pide estirar
  // un sprite a lo bestia.
  if (k < 0.45f) k = 0.45f;
  if (k > 1.9f) k = 1.9f;
  return k;
}

// Curvas de movimiento por estado. Todas alimentan la MISMA deformacion de
// drawPmdActM; cambiar de "andar" a "saltar" es cambiar de curva, no de codigo.
PetFx petFxFor(uint8_t act, uint32_t now, bool moving) {
  PetFx f;
  if (moving) {
    // Dos pisadas por vuelta. sq vale +1 al apoyar y -1 en el aire: achatar y
    // ensanchar A LA VEZ es lo que se lee como masa aplastandose contra el
    // suelo (solo achatar hace que el bicho parezca desinflarse).
    float ph = cyclePhase(now, 920);
    float sq = cosf(ph);
    f.phase = ph;
    f.sy = 1.0f - 0.10f * sq;
    f.sx = 1.0f + 0.075f * sq;
    // bob corto a proposito: con el pie despegando mucho del suelo el bicho
    // parece flotar, sobre todo si la sombra se le queda detras.
    f.dy = -fabsf(sinf(ph * 0.5f)) * 4.0f;
    f.lean = 7.0f;
    f.lag = 0.55f;
    return f;
  }
  if (act == PMD_SLEEP) {
    float r = sinf(cyclePhase(now, 5200));  // respiracion lenta del sueno
    f.sy = 0.90f + 0.020f * r;              // ademas se acomoda, encogido
    f.sx = 1.06f - 0.015f * r;
    return f;
  }
  if (act == PMD_HURT) {
    f.dx = ((now / 70) % 2) ? 4.0f : -4.0f;  // temblor
    f.sy = 0.96f;
    f.sx = 1.03f;
    return f;
  }
  float r = sinf(cyclePhase(now, 3200));  // quieto tambien respira, muy poco:
  f.sy = 1.0f - 0.022f * r;               // sin esto el bicho parece una estatua
  f.sx = 1.0f + 0.016f * r;
  return f;
}

void drawPetPMD() {
  uint32_t now = millis();

  if (pet.evolving()) {
    drawEvolveFX(now);
    return;
  }
  if (evoPmd.loaded) evoPmd.unload();  // termino la evolucion: libera la forma anterior

  PetMood m = pet.mood();
  uint8_t act;
  bool loop = true;
  bool moving = false;
  if (m == MOOD_SLEEPING && pmd.has(PMD_SLEEP)) {
    act = PMD_SLEEP;
    beh.mode = 0;
  } else if (m == MOOD_EATING && pmd.has(PMD_EAT)) {
    act = PMD_EAT;
    beh.t0 = 0;
  } else if (m == MOOD_SAD && pmd.has(PMD_HURT)) {
    act = PMD_HURT;
  } else {
    // contento: el planificador decide (idle / paseo / gesto)
    if (now > beh.until) behNext();
    if (beh.mode == 1) {
      float d = beh.targetX - beh.x;
      if (fabsf(d) < 4) {
        behNext();
        act = PMD_IDLE;
      } else {
        beh.x += (d > 0 ? 3.0f : -3.0f);
        act = (d > 0) ? PMD_WALKR : PMD_WALKL;
        moving = true;
      }
    } else {
      act = (beh.mode == 2) ? beh.act : PMD_IDLE;
      loop = false;
    }
    if (!pmd.has(act)) act = PMD_IDLE;
  }

  PetFx fx = petFxFor(act, now, moving);
  // El tamano de la especie YA NO se aplica aqui: lo hace drawPmdActM, que
  // es por donde pasan los SIETE sitios que dibujan un bicho. Aqui se
  // aplicaba solo para esta animacion y las otras seis salian normalizadas.
  float quiereCasa = altoPedido(-1, HOME_K);

  // Sombra de CONTACTO, no una mancha suelta: pegada a los pies y mas oscura
  // justo al apoyar. La primera version la encogia al subir el bicho, y entre
  // eso y un bob largo el resultado era que parecia flotar; asi ancla.
  //
  // El ancho sale de la HUELLA (las columnas que apoyan) y no del lienzo: hay
  // battlers que ocupan 93 px de ancho y apoyan en 7, y con el lienzo les
  // salia una mancha que no tenia nada que ver con ellos. Y como algunos
  // frames se despegan del suelo, la sombra se ensancha y se aclara con la
  // altura en vez de quedarse igual: asi lo que flota se lee como que flota.
  {
    // Los MISMOS helpers que usa drawPmdActM, y no una copia de la cuenta:
    // esta era la cuarta copia y ya se habia desincronizado de las otras
    // tres. Si la sombra no se escala igual que el bicho, el bicho flota.
    const PmdAct &ac = pmd.has(act) ? pmd.acts[act] : pmd.acts[PMD_IDLE];
    uint8_t sa = escalaDibujo(pmd, ac, 5, quiereCasa);
    float px = sa * factorTinta(ac, sa, quiereCasa, HOME_ALTO_MAX);

    float vuelo = ac.liftAt(pmdFrameAt(ac, now - beh.t0, loop || act == PMD_IDLE)) * px / 40.0f;
    if (vuelo > 1.0f) vuelo = 1.0f;

    int cuerpo = (int)((ac.footR - ac.footL + 1) * px);
    int sombraW = (int)(cuerpo * (0.82f + 0.40f * vuelo));
    if (sombraW < 16) sombraW = 16;
    // Centrada en el APOYO y no en el medio de la tinta: ver
    // PmdAct::centroPie(). El ancho sigue siendo el del cuerpo.
    int sombraX = (int)beh.x + (int)((ac.centroPie() - ac.w * 0.5f) * px);
    uint8_t dens = 3;
    if (moving) dens = (cosf(fx.phase) > 0.0f) ? 2 : 4;  // pisada = mas densa
    dens = (uint8_t)(dens + 2.0f * vuelo);               // en el aire, mas clara
    // A SILHUETA do frame que se ve, e nao uma elipse: "nos jogos a
    // sombra vem do proprio pokemon". Mesmos argumentos do desenho do
    // bicho logo abaixo, para cair no mesmo pe e no mesmo frame.
    (void)sombraW; (void)sombraX;
    gSombra.on = !gSombra.desligada;
    gSombra.num = (uint8_t)(4 + dens);
    gSombra.den = (uint8_t)(8 + dens);
    if (gSombra.on) drawPmdAct(act, (int)beh.x, PET_GROUND, now - beh.t0, loop || act == PMD_IDLE, false, 5, &fx);
    gSombra.on = false;
  }

  // polvillo en la pisada, aclarando el suelo que haya debajo
  if (moving) {
    float u = fmodf(fx.phase / (2.0f * (float)PI), 1.0f);
    if (u < 0.5f) {
      int edad = (int)(u * 2.0f * 14.0f);
      for (int lado = -1; lado <= 1; lado += 2)
        tintEllipse((int)beh.x + lado * (6 + edad), PET_GROUND - edad / 3,
                    3 + edad / 3, 2 + edad / 5, 14 - edad, 24, true);
    }
  }

  drawPmdAct(act, (int)beh.x, PET_GROUND, now - beh.t0, loop || act == PMD_IDLE, false, 5, &fx);

  // el corazon (carino, baya favorita) manda sobre la peticion: si acabas de
  // tocarlo no tiene sentido que te conteste "tengo hambre".
  if (determUntil) {
    // Decidido: manda sobre o coracao e sobre o pedido. Ele acabou de
    // entrar no matinho; nao e hora de dizer que esta com fome.
    const int lado = ((int)beh.x < CX) ? +1 : -1;
    drawNeedBubble(NEED_NONE, (int)beh.x + lado * balaoAoLado(), PET_GROUND - 92,
                   (int)beh.x, POR_PRONTO, &ART_PRONTO);
  } else if (pet.sleeping && !pet.isEgg()) {
    // DORMINDO —— e este caminho NAO EXISTIA.
    //
    // need() devolve NEED_NONE enquanto ele dorme, e com razao: "o que
    // ele precisa e dormir, e ja esta fazendo". So que o balao inteiro
    // pendurava do need(), entao dormindo nao aparecia nada —— e o ZZz
    // foi a PRIMEIRA onomatopeia que o dono pediu e nunca tinha sido
    // vista por ninguem.
    const int lado = ((int)beh.x < CX) ? +1 : -1;
    drawNeedBubble(NEED_NONE, (int)beh.x + lado * balaoAoLado(), PET_GROUND - 92,
                   (int)beh.x, POR_SONO, &ART_ZZZ);
  } else if (pet.showHeart()) {
    // ACABOU DE COMER, de brincar ou de ganhar carinho. Antes era um
    // coracao solto no ar; agora e o balao com a CARA contente dele e
    // o coracao no canto —— a mesma linguagem das outras reacoes, e e
    // o que o dono pediu ("tem que aparecer alguma tb dele feliz com
    // coracao").
    const int lado = ((int)beh.x < CX) ? +1 : -1;
    drawNeedBubble(NEED_NONE, (int)beh.x + lado * balaoAoLado(), PET_GROUND - 92,
                   (int)beh.x, POR_FESTA, &ART_CORACAO);
  } else {
    // O lado OPOSTO ao que ele esta: andando para a esquerda do
    // centro, o balao sai pela direita, e ao contrario. Assim ele
    // nunca e espremido contra a borda curva da tela redonda.
    const int lado = ((int)beh.x < CX) ? +1 : -1;
    drawNeedBubble(pet.need(), (int)beh.x + lado * balaoAoLado(), PET_GROUND - 92,
                   (int)beh.x);
  }
}

// sprite animado desde la SD: zoom entero por pixel, frames a su ritmo
void drawPetSD() {
  int s = mon.scale;
  int w = mon.w * s, h = mon.h * s;
  int x = CX - w / 2;
  int y = PET_CY - h / 2;

  bool sil = false;
  if (pet.evolving()) {
    sil = (millis() / 300) % 2;
  } else if (pet.mood() == MOOD_HAPPY && (millis() / 500) % 2) {
    y -= 6;  // saltito
  }

  uint16_t fm = mon.frameMs ? mon.frameMs : 100;
  uint16_t fi = pet.sleeping ? 0 : (millis() / fm) % mon.frames;
  const uint8_t *fr = mon.data + (uint32_t)fi * mon.w * mon.h;
  for (int r = 0; r < mon.h; r++) {
    const uint8_t *row = fr + r * mon.w;
    for (int c = 0; c < mon.w; c++) {
      uint8_t idx = row[c];
      if (idx == 0xFF) continue;
      gfx->fillRect(x + c * s, y + r * s, s, s, sil ? INK_K : mon.pal[idx]);
    }
  }

  // emotes en vez de expresiones (los sprites importados no tienen anclas)
  if (pet.showHeart()) drawMap(SPR_HEART, 32, x + w - 30, y - 50, 2, false);
  else drawNeedBubble(pet.need(), x + w - 14, y - 20);
}

// ojo cerrado: borra el ojo 3x4 y dibuja el parpado
void overlayEye(const Species &sp, int x, int y, int s, int col) {
  gfx->fillRect(x + col * s, y + sp.eyeRow * s, 3 * s, 4 * s, sp.bodyColor);
  gfx->fillRect(x + col * s, y + (sp.eyeRow + 2) * s, 3 * s, s, INK_K);
}

// borra la sonrisa base y pinta boca abierta (comer) o ceno (triste)
void overlayMouth(const Species &sp, int x, int y, int s, bool open) {
  int mc = sp.mouthCol, mr = sp.mouthRow;
  gfx->fillRect(x + (mc - 3) * s, y + mr * s, 7 * s, 2 * s, sp.bodyColor);
  if (open) {
    gfx->fillRect(x + (mc - 2) * s, y + mr * s, 5 * s, 2 * s, INK_K);
  } else {
    gfx->fillRect(x + (mc - 2) * s, y + mr * s, 5 * s, s, INK_K);
    gfx->fillRect(x + (mc - 3) * s, y + (mr + 1) * s, s, s, INK_K);
    gfx->fillRect(x + (mc + 3) * s, y + (mr + 1) * s, s, s, INK_K);
  }
}

// Oscurece (o aclara) una elipse mezclando SOBRE el framebuffer, en vez de
// pintar un color plano encima. Hace falta porque el suelo cambia con el bioma
// y con la hora: una mancha opaca quedaria bien en la pradera y fatal en la
// nieve. Como Arduino_Canvas tiene el framebuffer entero en PSRAM, basta leer
// cada pixel y reescalarlo.
// Um RGB565 puxado para `cor` (alfa cor[3]) e depois com o tom do
// Essentials: cinza dessatura, e r/g/b somam (-255..255).
uint16_t tingeCor(uint16_t c, const uint8_t cor[4], const int16_t tom[4]) {
  int r = ((c >> 11) & 31) * 255 / 31, g = ((c >> 5) & 63) * 255 / 63,
      b = (c & 31) * 255 / 31;
  if (cor[3]) {
    r += (cor[0] - r) * cor[3] / 255;
    g += (cor[1] - g) * cor[3] / 255;
    b += (cor[2] - b) * cor[3] / 255;
  }
  if (tom[3]) {
    int l = (r * 77 + g * 150 + b * 29) >> 8;
    r += (l - r) * tom[3] / 255;
    g += (l - g) * tom[3] / 255;
    b += (l - b) * tom[3] / 255;
  }
  r += tom[0]; g += tom[1]; b += tom[2];
  r = r < 0 ? 0 : r > 255 ? 255 : r;
  g = g < 0 ? 0 : g > 255 ? 255 : g;
  b = b < 0 ? 0 : b > 255 ? 255 : b;
  return (uint16_t)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
}

// Um pixel do framebuffer misturado com `s` por `a` (0..255): modo 0
// normal, 1 soma (brilho), 2 subtrai —— os tres do Essentials.
void misturaPixel(uint16_t *d, uint16_t s, uint8_t a, uint8_t modo) {
  int dr = (*d >> 11) & 31, dg = (*d >> 5) & 63, db = *d & 31;
  int sr = (s >> 11) & 31, sg = (s >> 5) & 63, sb = s & 31;
  if (modo == 1) {
    dr += sr * a / 255; dg += sg * a / 255; db += sb * a / 255;
    if (dr > 31) dr = 31;
    if (dg > 63) dg = 63;
    if (db > 31) db = 31;
  } else if (modo == 2) {
    dr -= sr * a / 255; dg -= sg * a / 255; db -= sb * a / 255;
    if (dr < 0) dr = 0;
    if (dg < 0) dg = 0;
    if (db < 0) db = 0;
  } else {
    dr += (sr - dr) * a / 255; dg += (sg - dg) * a / 255; db += (sb - db) * a / 255;
  }
  *d = (uint16_t)((dr << 11) | (dg << 5) | db);
}

// Um retangulo de `c` com opacidade `a` (o bicho meio apagado).
static void misturaRect(int x, int y, int w, int h, uint16_t c, uint8_t a) {
  uint16_t *fb = gfx->getFramebuffer();
  if (!fb) return;
  if (x < 0) { w += x; x = 0; }
  if (y < 0) { h += y; y = 0; }
  if (x + w > LCD_WIDTH) w = LCD_WIDTH - x;
  if (y + h > LCD_HEIGHT) h = LCD_HEIGHT - y;
  for (int yy = y; yy < y + h; yy++)
    for (int xx = x; xx < x + w; xx++)
      misturaPixel(fb + (uint32_t)yy * LCD_WIDTH + xx, c, a, 0);
}

// Escurece um retangulo do framebuffer por num/den (a sombra de silhueta).
static void tintRect(int x, int y, int w, int h, uint8_t num, uint8_t den) {
  uint16_t *fb = gfx->getFramebuffer();
  if (!fb || den == 0) return;
  if (x < 0) { w += x; x = 0; }
  if (y < 0) { h += y; y = 0; }
  if (x + w > LCD_WIDTH) w = LCD_WIDTH - x;
  if (y + h > LCD_HEIGHT) h = LCD_HEIGHT - y;
  for (int yy = y; yy < y + h; yy++) {
    uint16_t *line = fb + (uint32_t)yy * LCD_WIDTH;
    for (int xx = x; xx < x + w; xx++) {
      uint16_t p = line[xx];
      uint16_t r = ((p >> 11) & 0x1F) * num / den;
      uint16_t g = ((p >> 5) & 0x3F) * num / den;
      uint16_t b = (p & 0x1F) * num / den;
      line[xx] = (uint16_t)((r << 11) | (g << 5) | b);
    }
  }
}

void tintEllipse(int cx, int cy, int rx, int ry, uint8_t num, uint8_t den, bool aclarar) {
  if (rx <= 0 || ry <= 0 || den == 0) return;
  uint16_t *fb = gfx->getFramebuffer();
  if (!fb) return;
  int y0 = cy - ry, y1 = cy + ry, x0 = cx - rx, x1 = cx + rx;
  if (y0 < 0) y0 = 0;
  if (x0 < 0) x0 = 0;
  if (y1 >= LCD_HEIGHT) y1 = LCD_HEIGHT - 1;
  if (x1 >= LCD_WIDTH) x1 = LCD_WIDTH - 1;
  int32_t rx2 = (int32_t)rx * rx, ry2 = (int32_t)ry * ry;
  for (int y = y0; y <= y1; y++) {
    int32_t dy = y - cy, term = dy * dy * rx2;
    uint16_t *line = fb + (uint32_t)y * LCD_WIDTH;
    for (int x = x0; x <= x1; x++) {
      int32_t dx = x - cx;
      if (dx * dx * ry2 + term > rx2 * ry2) continue;
      uint16_t p = line[x];
      uint16_t r = (p >> 11) & 0x1F, g = (p >> 5) & 0x3F, b = p & 0x1F;
      if (aclarar) {
        r += (31 - r) * num / den;
        g += (63 - g) * num / den;
        b += (31 - b) * num / den;
      } else {
        r = r * num / den;
        g = g * num / den;
        b = b * num / den;
      }
      line[x] = (uint16_t)((r << 11) | (g << 5) | b);
    }
  }
}

// Fondo del combate: cielo con degradado y las dos plataformas. Es propio y no
// drawScene() porque alli el bicho vive en el centro y aqui hay dos, uno lejos
// y otro cerca, y las plataformas tienen que decir cual es cual.
// Oscurece un RGB565 un 25%: con eso basta para que la plataforma se despegue
// del suelo de la foto sin pintarle encima un color que no pinta nada ahi.
static uint16_t darken565(uint16_t c) {
  uint16_t r = (c >> 11) & 0x1F, g = (c >> 5) & 0x3F, b = c & 0x1F;
  return (uint16_t)(((r * 3 / 4) << 11) | ((g * 3 / 4) << 5) | (b * 3 / 4));
}

// El pixel del fondo en ese punto, para tenir la plataforma con el color que
// ya hay debajo.
static uint16_t bgPixel(int x, int y, uint16_t sino) {
  if (!bgImg.loaded || x < 0 || y < 0 || x >= bgImg.w || y >= bgImg.h) return sino;
  return bgImg.pix[(uint32_t)y * bgImg.w + x];
}

void drawBattleScene(uint32_t now) {
  if (bgImg.loaded && bgImg.w == LCD_WIDTH && bgImg.h == LCD_HEIGHT) {
    gfx->draw16bitRGBBitmap(0, 0, bgImg.pix, bgImg.w, bgImg.h);
    // SIN plataformas. Eran dos elipses de color plano —— una gris y otra
    // verde —— pegadas encima de la foto, y es lo primero que canta.
    //
    // Estaban para decir quien esta delante, pero eso ya lo dicen dos cosas
    // mejores: el TAMANO (el de enfrente se dibuja mas pequeno desde que la
    // escala sale del sprite del Anil) y la SOMBRA que cada bicho proyecta
    // bajo sus pies en btDrawSide, que ademas se estrecha cuando salta.
    //
    // Y es lo que hace el Anil: sus Battlebacks son el paisaje limpio, sin
    // discos de ningun tipo.
    (void)now;
    return;
  }

  // degradado de cielo, en componentes RGB565 ya (5/6/5), de #96cdeb a #d6eefa
  for (int y = 0; y < LCD_HEIGHT; y++) {
    int k = y * 255 / LCD_HEIGHT;
    uint16_t r = 18 + (26 - 18) * k / 255;
    uint16_t g = 51 + (59 - 51) * k / 255;
    uint16_t b = 29 + (31 - 29) * k / 255;
    gfx->fillRect(0, y, LCD_WIDTH, 1, (uint16_t)((r << 11) | (g << 5) | b));
  }
  // Sin tarjeta no hay paisaje, solo el cielo: aqui las plataformas SI
  // hacen falta, porque sobre un degradado liso no hay suelo que pisar.
  // Centradas donde se planta cada uno (el rival esta en BT_FOE_CX, no en
  // el 347 de antes: al moverlo, su plataforma se quedo donde estaba y se
  // veia descuadrado).
  gfx->fillEllipse(BT_FOE_CX, 171, 80, 20, 0xAE71);
  gfx->fillEllipse(145, 295, 112, 29, 0x960F);
  (void)now;
}

// globo de peticion: caja redondeada con contorno, rabo apuntando al bicho y
// dentro el icono de lo que pide. Son los MISMOS iconos 16x16 de los botones
// de abajo, asi "pide esto" y "pulsa este boton" se leen como lo mismo.
// Que cara e que simbolo pede cada necessidade.
//
// "bravo pq esta sujo/sem atencao": a sujeira e a falta de brincadeira
// levam as duas a mesma cara brava, que foi o que ele descreveu.
static uint8_t estadoDeNecesidad(PetNeed n) {
  switch (n) {
    case NEED_FOOD:   return POR_FOME;
    case NEED_ENERGY: return POR_SONO;
    case NEED_CLEAN:
    case NEED_PLAY:   return POR_SUJO;
    default:          return POR_NORMAL;
  }
}
// A que distancia do centro do bicho vai o balao: a metade da largura dele, de
// verdade, mais a metade do balao. Com os 86 fixos de antes, num bicho grande o
// balao ficava em cima da cara dele.
int balaoAoLado() {
  const PmdAct &a = pmd.acts[PMD_IDLE];
  int metade = 40;
  if (a.inkH() && a.inkR > a.inkL && ultimoAltoPintado > 0) {
    const float k = (float)ultimoAltoPintado / a.inkH();
    metade = (int)((a.inkR - a.inkL + 1) * k / 2);
  }
  const int d = metade + 52 + 6;
  return d < 86 ? 86 : d;
}

static const Art *simboloDeNecesidad(PetNeed n) {
  switch (n) {
    case NEED_FOOD:   return &ART_FOME;
    case NEED_ENERGY: return &ART_ZZZ;
    case NEED_CLEAN:  return &ART_SUJEIRA;
    case NEED_PLAY:   return &ART_RAIVA;
    default:          return nullptr;
  }
}

void drawNeedBubble(PetNeed n, int cx, int baseY, int haciaX,
                    uint8_t estadoForzado, const Art *simboloForzado) {
  const char *const *icon = SPR_ICON_LIGHT;
  switch (n) {
    case NEED_FOOD:   icon = SPR_ICON_FOOD;  break;
    case NEED_PLAY:   icon = SPR_ICON_PLAY;  break;
    case NEED_CLEAN:  icon = SPR_ICON_CLEAN; break;
    case NEED_ENERGY: icon = SPR_ICON_LIGHT; break;
    default: break;   // NEED_NONE: so vale com estado forcado
  }
  // Sem pedido E sem estado forcado nao ha balao nenhum.
  if (n == NEED_NONE && estadoForzado == 0xFF) return;
  // A CARA, se o cartao a trouxe. porMon ja vem carregado do tick: aqui
  // nao se le da SD, que isto corre dentro do quadro.
  const uint8_t estado = (estadoForzado != 0xFF) ? estadoForzado
                                                : estadoDeNecesidad(n);
  const Art *cara = (porMon.dex == (uint16_t)pet.speciesId &&
                     porMon.shiny == pet.shiny && !porMon.mega)
                        ? porMon.cara(estado)
                        : nullptr;
  const Art *simbolo = simboloForzado ? simboloForzado
                                      : simboloDeNecesidad(n);

  const int s = 3, ic = cara ? cara->w * 2 : 16 * s, pad = 9;
  const int ih = cara ? cara->h * 2 : 16 * s;
  const int w = ic + pad * 2, h = ih + pad * 2;
  // La pantalla es REDONDA: arriba del todo solo hay circulo entre x=75 y
  // x=391, y el bicho pasea hasta x=325. Sin esto el globo se sale por el
  // borde curvo cuando el bicho esta en una punta.
  if (cx < 115) cx = 115;
  if (cx > 351) cx = 351;
  const int x = cx - w / 2, y = baseY - h;
  const int bob = ((millis() / 420) % 2) ? 1 : -2;  // latido lento

  gfx->fillRoundRect(x - 3, y - 3 + bob, w + 6, h + 6, 14, INK_K);
  gfx->fillRoundRect(x, y + bob, w, h, 11, UI_WHITE);
  // O rabo aponta PARA O BICHO. Com o balao ao lado, um rabo sempre no
  // mesmo canto apontaria para fora quando ele troca de lado.
  const int px = (haciaX < 0) ? cx - 3
                              : (haciaX < cx ? x + 12 : x + w - 12);
  gfx->fillTriangle(cx - 10, y + h - 4 + bob, cx + 10, y + h - 4 + bob,
                    px, y + h + 15 + bob, INK_K);
  gfx->fillTriangle(cx - 6, y + h - 6 + bob, cx + 6, y + h - 6 + bob,
                    px, y + h + 8 + bob, UI_WHITE);
  if (cara) {
    // drawArt ancora pelo pe, entao o baseY e o fundo da caixa.
    drawArt(*cara, x + w / 2, y + pad + ih + bob, 2);
  } else {
    // Sem cartao ou sem retrato: o icone de sempre. Um aparelho sem SD
    // nao pode ficar sem avisar.
    drawMap(icon, 16, x + pad, y + pad + bob, s, false);
  }
  // A ONOMATOPEIA VAI SEMPRE, com cara ou com icone.
  //
  // Estava dentro do ramo da cara, e isso era o avesso do que ela existe
  // pra fazer: ela e quem carrega o recado quando a cara que sobrou e a
  // neutra —— e com mais razao ainda quando nao ha cara nenhuma. Um
  // aparelho sem cartao perdia o simbolo junto com o retrato. Ela mora
  // na flash, entao esta sempre disponivel.
  if (simbolo)
    drawArt(*simbolo, x + w - 4, y + pad + simbolo->h / 2 + bob, 1);
}

void drawPoops() {
  // ART_POOP es BLACKSLUDGE del Anil tenido de marron (ver ICONOS en
  // tools/gen_art.py): el unico monton con remolino que hay en el juego,
  // que es justo la silueta que toca. La de antes era SPR_POOP, un mapa
  // de caracteres de 32x32 escrito a mano en species.h, y se notaba.
  //
  // drawArt ancla por los PIES, asi que el suelo va donde acababa el
  // dibujo viejo (244 + 64 = 308) y no donde empezaba.
  for (int i = 0; i < pet.poops; i++)
    drawArt(ART_POOP, 70 + i * 66, 306, 2);
}

// Franja de PS. Solo aparece cuando el bicho esta tocado: el resto del tiempo
// seria una quinta barra permanentemente llena, y no hay sitio. Va en los 6 px
// libres entre el borde del panel (312) y las etiquetas (318). Sin texto a
// proposito: es roja, encoge, y solo sale cuando pasa algo.
// As barras NA CHAPA DA BATALHA, inclinada: papel branco, contorno escuro, trilho
// escuro e a cor pelo valor (verde, amarelo, vermelho). A de PS do bicho so
// aparece quando falta vida.
#define CHAPA_X 108
#define CHAPA_W 250
#define CHAPA_Y 311
#define CHAPA_H 46
#define CHAPA_INC 12
static void chapaBarra(int x, int y, int w, const char *rotulo, int rotW, uint8_t pct, bool hp) {
  gfx->setTextColor(INK_K);
  setSize(1);
  setCur(x, y + 1);
  printT(rotulo);
  const int bx = x + rotW + 4, bw = w - rotW - 4;
  if (bw < 10) return;
  gfx->fillRoundRect(bx, y, bw, 10, 4, C565(0x46, 0x4e, 0x58));
  uint16_t col = pct >= 50 ? UI_BAR_OK
               : (pct >= (hp ? BATTLE_HP_FLOOR_PCT : 25) ? UI_BAR_WARN : UI_BAR_BAD);
  const int fw = (bw - 4) * pct / 100;
  if (fw > 0) gfx->fillRoundRect(bx + 2, y + 2, fw, 6, 3, col);
}

// Faixa vermelha no alto enquanto o ultimo save tiver falhado.
void avisoDeSave() {
  if (!pet.guardadoFallo) return;
  // Grande e abaixo do nome e do estado: e um alarme, nao um detalhe.
  setSize(2);
  const char *t = T(S_SAVE_FAIL);
  const int w = textW(t, 2) + 28, y = 118;
  gfx->fillRoundRect(CX - w / 2 - 2, y - 2, w + 4, 34, 9, INK_K);
  gfx->fillRoundRect(CX - w / 2, y, w, 30, 8, C565(0xd0, 0x20, 0x20));
  gfx->setTextColor(UI_WHITE);
  setCur(CX - w / 2 + 14, y + 8);
  printT(t);
}

void drawBars() {
  avisoDeSave();
  const uint16_t tope = pet.maxHp();
  const bool ferido = tope && pet.hpLeft < tope;
  const int x0 = CHAPA_X, x1 = CHAPA_X + CHAPA_W, y0 = CHAPA_Y, y1 = CHAPA_Y + CHAPA_H, k = CHAPA_INC;
  // o paralelogramo: contorno escuro e papel por cima
  gfx->fillTriangle(x0 + k, y0 - 2, x1 + 2, y0 - 2, x0 - 2, y1 + 2, INK_K);
  gfx->fillTriangle(x1 + 2, y0 - 2, x1 - k + 2, y1 + 2, x0 - 2, y1 + 2, INK_K);
  gfx->fillTriangle(x0 + k, y0, x1, y0, x0, y1, UI_WHITE);
  gfx->fillTriangle(x1, y0, x1 - k, y1, x0, y1, UI_WHITE);
  // A mesma largura de rotulo para os quatro: as barras alinham em coluna, e
  // uma etiqueta longa de outro idioma encolhe a barra em vez de invadi-la.
  const char *const R[4] = { T(S_BAR_FOOD), T(S_BAR_JOY), T(S_BAR_ENE), T(S_BAR_HYG) };
  const uint8_t V[4] = { pet.fullness, pet.joy, pet.energy, pet.hygiene };
  setSize(1);
  int rotW = 0;
  for (const char *r : R) { int w = textW(r, 1); if (w > rotW) rotW = w; }
  const int colW = (CHAPA_W - 46) / 2;
  int y = y0 + (ferido ? 4 : 9);
  const int passo = ferido ? 13 : 18;
  if (ferido) {
    chapaBarra(x0 + 22, y, CHAPA_W - 44, "HP", rotW,
               (uint8_t)((uint32_t)pet.hpLeft * 100 / tope), true);
    y += passo;
  }
  for (int k2 = 0; k2 < 4; k2++) {
    const int linha = k2 >> 1;
    // cada linha anda para a esquerda com a inclinacao da chapa
    chapaBarra(x0 + 20 - linha * 6 + (k2 & 1) * (colW + 8), y + linha * passo, colW, R[k2], rotW, V[k2], false);
  }
}

// Los cuatro iconos de abajo. Tres son SPRITES DEL JUEGO (art.h) y no
// mapas de caracteres dibujados a mano: la pokeball salia cuadrada y la
// gota no parecia agua.
//
// El de dormir sigue dibujado, pero ahora es una luna de verdad â€” dos
// circulos, el de delante del color del fondo â€” en vez de un borron. En
// todo el Anil no hay un solo icono de luna o de sueno que sirva.
// Cabe num quadrado de `lado`, centrado: um desenho maior nao vaza do botao.
static void iconeNoMiolo(const Art &a, int cx, int cy, int lado) {
  const int w = a.w >= a.h ? lado : lado * a.w / a.h;
  drawArtZoom(a, cx, cy, w);
}

void drawBtnIcon(int i, int cx, int cy) {
  const int L = 28;
  switch (i) {
    case 0:   // comer: a maca (Leftovers, o primeiro objeto da tabela)
      if (ART_ITEM[1]) iconeNoMiolo(*ART_ITEM[1], cx, cy, L);
      return;
    case 1: {  // treino: um haltere. Era uma pokebola, igual ao de cacar.
      const uint16_t f = C565(0x4a, 0x52, 0x60);
      gfx->fillRect(cx - 10, cy - 2, 20, 4, f);
      gfx->fillRoundRect(cx - 15, cy - 9, 7, 18, 2, f);
      gfx->fillRoundRect(cx + 8, cy - 9, 7, 18, 2, f);
      gfx->fillRoundRect(cx - 19, cy - 6, 4, 12, 1, f);
      gfx->fillRoundRect(cx + 15, cy - 6, 4, 12, 1, f);
      return;
    }
    case 2:   // cacar: o mesmo matinho do aviso, agora dentro do botao
      iconeNoMiolo(ART_HIERBA, cx, cy, L);
      return;
    case 3: {  // a aventura: um mapa com o X. A mochila, nesse tamanho, parecia um alvo.
      const uint16_t papel = C565(0xf0, 0xd8, 0x98), linha = C565(0x9a, 0x70, 0x38);
      gfx->fillRect(cx - 13, cy - 10, 26, 20, papel);
      gfx->fillRect(cx - 5, cy - 10, 1, 20, linha);
      gfx->fillRect(cx + 4, cy - 10, 1, 20, linha);
      gfx->drawRect(cx - 13, cy - 10, 26, 20, INK_K);
      for (int k = 0; k < 4; k++) gfx->fillRect(cx - 10 + k * 4, cy + 4 - k * 2, 2, 2, linha);
      const uint16_t x = C565(0xd8, 0x30, 0x30);
      for (int k = -3; k <= 3; k++) {
        gfx->fillRect(cx + 6 + k, cy - 4 + k, 2, 2, x);
        gfx->fillRect(cx + 6 + k, cy - 4 - k, 2, 2, x);
      }
      return;
    }
    case 5: {  // o banho: as bolhas
      const uint16_t b = C565(0x4c, 0xa8, 0xe0);
      gfx->fillCircle(cx - 5, cy + 4, 8, C565(0xc8, 0xee, 0xff));
      gfx->drawCircle(cx - 5, cy + 4, 8, b);
      gfx->fillRect(cx - 9, cy, 3, 3, UI_WHITE);
      gfx->fillCircle(cx + 8, cy - 3, 6, C565(0xc8, 0xee, 0xff));
      gfx->drawCircle(cx + 8, cy - 3, 6, b);
      gfx->fillRect(cx + 5, cy - 6, 2, 2, UI_WHITE);
      gfx->fillCircle(cx, cy - 10, 3, C565(0xc8, 0xee, 0xff));
      gfx->drawCircle(cx, cy - 10, 3, b);
      return;
    }
    default: {  // a luz: a lua
      uint16_t luna = C565(0xf2, 0xc9, 0x4c), hueco = pet.sleeping ? UI_BG_NIGHT : UI_WHITE;
      gfx->fillCircle(cx, cy, 13, luna);
      gfx->fillCircle(cx + 7, cy - 4, 11, hueco);
      return;
    }
  }
}

void drawButtons() {
  uint32_t ahora = millis();
  // O aro diz de que e o botao antes de olhar o desenho.
  static const uint16_t ARO[BTN_N] = {
    C565(0xe8, 0x60, 0x48), C565(0xe8, 0x96, 0x3c), C565(0x54, 0xb0, 0x5c),
    C565(0x46, 0x96, 0xe2), C565(0x78, 0x68, 0xd6), C565(0x46, 0xc4, 0xd6),
  };
  for (int i = 0; i < BTN_N; i++) {
    bool off = pet.sleeping && i != 4;  // durmiendo solo funciona LUZ
    // El de JUGAR pide energia, como todo lo que cansa. Se pinta apagado en
    // vez de dejarlo muerto sin decir nada: un boton que no responde y no se
    // ve distinto parece una averia.
    bool sinFuelle = (i == 1) && !pet.sleeping && !pet.canTrain();
    // Y el de CAZAR, mientras se enfria. Mismo criterio: se ve que no toca.
    if (i == 2 && !pet.sleeping && !pet.huntReady(ahora)) sinFuelle = true;
    const int cx = buttons[i].cx, cy = buttons[i].cy;
    // MEIA-POKEBOLA: em cima a cor do botao, embaixo branco, faixa escura no meio
    // e o miolo branco com o desenho.
    const uint16_t cor = sinFuelle || off ? UI_TRACK : ARO[i];
    gfx->fillCircle(cx, cy, BTN_HALF, INK_K);
    gfx->fillCircle(cx, cy, BTN_HALF - 2, pet.sleeping ? UI_BG_NIGHT : UI_WHITE);
    {
      const int r = BTN_HALF - 2;
      for (int dy = -r; dy < 0; dy++) {
        const int m = (int)sqrtf((float)(r * r - dy * dy));
        gfx->fillRect(cx - m, cy + dy, m * 2 + 1, 1, cor);
      }
    }
    gfx->fillRect(cx - BTN_HALF, cy - 2, BTN_HALF * 2 + 1, 4, INK_K);
    gfx->fillCircle(cx, cy, 19, INK_K);
    gfx->fillCircle(cx, cy, 17, pet.sleeping ? UI_BG_NIGHT : UI_WHITE);
    if (!off) drawBtnIcon(i, cx, cy);
    if (sinFuelle) {
      // una tachadura fina: se lee "ahora no" sin tapar el icono
      tintEllipse(cx, cy, 17, 17, 7, 10, true);
    }
    // Al de cazar, ademas, LOS SEGUNDOS que faltan: "esta gris" no dice si
    // son dos segundos o un minuto, y eso es justo lo que uno quiere saber.
    if (i == 2 && !pet.sleeping && !pet.huntReady(ahora)) {
      char q[8];
      snprintf(q, sizeof(q), "%u", (unsigned)pet.huntLeftS(ahora));
      printOutlined(cx - textW(q, 1) / 2, cy + BTN_HALF - 12, q, 1, UI_WHITE);
    }
  }
}

const char *eggMsg() {
  switch (pet.eggCracks()) {
    case 0: return T(S_EGG_TOUCH);
    case 1: return T(S_EGG_MOVES);
    default: return T(S_EGG_ALMOST);
  }
}

const char *statusMsg() {
  if (pet.evolving()) return T(S_EVOLVING);
  if (bathUntil) return "Splish splash!";  // onomatopeya universal
  if (pet.sleeping) return "Zzz...";
  if (pet.eating()) return T(S_EATING);
  if (pet.showHeart()) return T(S_LIKES);
  if (pet.fullness < 25) return T(S_HUNGRY);
  if (pet.hygiene < 25) return T(S_NEEDS_BATH);
  if (pet.energy < 25) return T(S_EXHAUSTED);
  if (pet.joy < 25) return T(S_SAD);
  if (pet.weight > 60) return T(S_CHUBBY);
  if (pet.shiny && pet.ageMinutes < 15) return T(S_IS_SHINY);
  return T(S_HAPPY);
}

// dibuja un mapa de n x n pixeles escalado; silhouette=true lo pinta en tinta
void drawMap(const char *const *map, int n, int x, int y, int s, bool silhouette) {
  for (int r = 0; r < n; r++) {
    for (int c = 0; c < n; c++) {
      char ch = map[r][c];
      if (ch == '.') continue;
      gfx->fillRect(x + c * s, y + r * s, s, s, silhouette ? INK_K : spriteColor(ch));
    }
  }
}
