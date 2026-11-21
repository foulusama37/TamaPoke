#pragma once
// Pantalla de combate. Se separa del .ino porque ya es bastante grande y esto
// es una pieza entera; la logica pura vive aparte todavia, en battle.cpp.
//
// Geometria: la pantalla es REDONDA (466 de diametro). El ancho util depende
// de la altura, y eso manda en el reparto:
//   y  62..102  ->  297 px  barras de PS
//   y 334..420  ->  257 px  tarjeta de golpe
// Una rejilla 2x2 de golpes pediria ~340 px y no cabe. Por eso la tarjeta es
// UNA sola con flechas a los lados: ademas asi el numero de golpes deja de
// condicionar el hueco, y se puede pasar de 2 a 4 sin rehacer nada.
#include <stdint.h>

#include "battle.h"

// O ponto de decisao aberto na batalha entre placas (battle_ui.ino).
enum : uint8_t { PT_NADA = 0, PT_TURNO, PT_REPOE };

enum : uint8_t {
  BP_INTRO,     // "un X salvaje!"
  BP_CHOOSE,    // el jugador elige golpe
  BP_SAY,       // "X uso Y!": sin esto no hay forma de saber que hizo el rival
  BP_ATTACK,    // el que pega se lanza hacia delante
  BP_HIT,       // el que recibe parpadea y se sacude
  BP_MSG,       // "es muy eficaz", "se curo", "el retroceso le dolio"...
  BP_FAINT,     // el que cae se hunde tras la plataforma
  BP_END,       // se acabo
  BP_BAG,       // la bolsa, abierta en mitad del combate
  BP_THROW,     // la bola volando hacia el rival, girando
  BP_SUCK,      // se abre y se lo traga: el bicho se encoge y se mete dentro
  BP_WOBBLE,    // forcejea, y el jugador toca para empujar
  BP_CAUGHT,    // atrapado
  BP_PARTY,     // el equipo, para relevar
  // La MEGAEVOLUCION, que es una escena y no un numero que cambia: los
  // anillos, el sello creciendo y el bicho temblando. La transformacion
  // se hace al ACABAR, para que el cambio de sprite caiga con el final.
  BP_MEGA,
  // La frase de la mega DEL RIVAL. Hace falta una fase propia porque la
  // suya cae EN MITAD del turno: el rival megaevoluciona cuando tu ya
  // has elegido golpe, asi que al acabar la escena no se vuelve a
  // BP_CHOOSE —— hay que leer "Foe X MEGA EVOLVED!" y SEGUIR el turno.
  // Colgado de BP_MSG, el anuncio lo pisaba btLaunch en el mismo
  // fotograma y la mega del rival volvia a no verse.
  BP_MEGA_SAY,
  // Lo que te llevas. Va ANTES de BP_END porque BP_END cierra en el
  // acto: el premio se cobraba y solo salia por el puerto serie, o
  // sea que el jugador no se enteraba de nada.
  BP_LOOT,
  // El RESUMEN de lo que has sacado. Antes de esto, ganar era: cae el
  // rival, quiza una pantalla de objeto, y de vuelta a casa en segundo y
  // medio. La experiencia, el nivel y los stats nuevos solo salian por el
  // puerto serie, asi que lo unico que de verdad avanza en el juego era lo
  // unico que no se veia.
  BP_WIN,
  // A INSIGNIA, depois do resumo, quando se venceu um LIDER (nao a Elite
  // 4, nao a revanche): qual insignia, o nome, a MT e o premio. Antes so
  // tocava a fanfarra e o resto saia pelo puerto serie.
  BP_BADGE,
  // BATALHA ENTRE PLACAS: ja escolhi, esperando a jogada da outra placa
  // (ou ela escolhendo quem repoe). Nada aqui avanca o combate: quem
  // fecha o ponto e pvpDecide, quando chegam as duas.
  BP_ESPERA,
  // CARGA RAPIDA (Rayo Solar al sol, Electrorrayo con lluvia): no Anil o
  // pbQuickChargingMove diz a frase da carga e toca a animacao dela, e SO
  // ENTAO sai o golpe, no mesmo turno. Passos em `cargaPasso`. Vai no fim
  // do enum para nao mexer no numero das outras fases.
  BP_CARGA,
};

// Donde se planta el rival. Estaba en 347, pegado al borde: la pantalla es
// REDONDA y ahi arriba la cuerda ya no da de si, asi que un bicho alto se
// salia por la esquina. En 325 caben 137 px de alto.
//
// Vive AQUI y no en battle_ui.ino porque la escena de combate la pinta
// TamaPoke.ino, que el generador concatena ANTES: alli la macro todavia no
// existiria y la plataforma del rival se quedaria descuadrada otra vez.
#define BT_FOE_CX 325

// O modo sombra do desenhador (ver gSombra no TamaPoke.ino). Aqui no
// header porque o teste precisa de `desligada` para medir a sombra por
// diferenca: tela com ela menos tela sem ela.
struct SombraModo {
  bool on = false;
  bool desligada = false;   // so os testes ligam
  uint8_t num = 6, den = 10;
  float achata = 0.38f, inclina = 0.95f;
};
extern SombraModo gSombra;

// TINGIMENTO do bicho que a animacao do Anil pede (o "fica vermelho" do
// lanca-chamas, o sumir do Seismic Toss): opacidade, puxar para uma cor
// (cor[3] = quanto) e tom (soma rgb, cinza = dessatura). drawPmdActM o
// aplica a cada pixel enquanto `on`.
// La MEGA en el Anil se pone BLANCA mientras cambia y vuelve del blanco
// ya transformada. Cuanto blanco lleva ahora el lado `lado`, 0..1.
float btMegaBlanco(uint8_t lado, uint32_t now);

struct TintaModo {
  bool on = false;
  uint8_t op = 255;
  uint8_t cor[4] = { 0, 0, 0, 0 };
  int16_t tom[4] = { 0, 0, 0, 0 };
};
extern TintaModo gTinta;
uint16_t tingeCor(uint16_t c, const uint8_t cor[4], const int16_t tom[4]);
void misturaPixel(uint16_t *d, uint16_t s, uint8_t a, uint8_t modo);

// Lo que dura la entrada. La frase que la acompana ("Go, X!" / "X sends
// Y!") dura 1200 ms, asi que esto tiene que caber holgado dentro: la bola
// se abre y el bicho acaba de subir antes de que toque elegir.
#define BT_SENDOUT_MS 700

// Lo que tarda un bicho en QUITARSE DE EN MEDIO con Excavar, Vuelo o
// Buceo, y lo mismo al volver. Corto a proposito: cae dentro del aviso
// de "X is charging up!" (900 ms) y de la embestida del turno siguiente
// (320 ms), asi que no alarga el combate ni un fotograma.
#define BT_HIDE_MS 260

// Cuanto dura la escena de la mega. Lo bastante para que se lea como una
// transformacion y lo bastante poco para no estorbar cada combate.
//
// 1400 y no 1100: suena la fanfarria del Anil (JIN_MEGA, 1,37 s) y a
// 1100 la escena se cerraba con el sonido todavia sonando encima del
// menu. Se alarga la imagen hasta el sonido y no al reves —— recortar el
// wav le quitaria justo la cola, que es lo que suena a transformacion.
//
// Vivia en capture.ino. Se muda aqui porque battle_ui.ino se concatena
// ANTES y ahora tambien la necesita: la mega del RIVAL la arranca
// battleTap().
#define BT_MEGA_MS 1400

// EL FOSO. Un toque que no acierta a nada solo CIERRA una pantalla si cae
// a mas de UI_FOSO px de cualquier cosa que se pueda tocar. El visor da
// unos 10,5 px por mm: son 1,7 mm, lo que se va un dedo.
//
// Antes "cerrar" era "todo lo que no sea un blanco", y los blancos miden
// 3 mm de alto: en la bolsa del combate, bajar por la lista devolvia al
// combate "quase 100% das vezes". El mapa de toques (teste `toques` de
// test_ui.cpp) vigila que esto se cumpla en TODAS las pantallas.
#define UI_FOSO 18
static inline bool lejosDe(int x, int y, int x0, int y0, int x1, int y1) {
  return x < x0 - UI_FOSO || x > x1 + UI_FOSO ||
         y < y0 - UI_FOSO || y > y1 + UI_FOSO;
}

struct BattleUi {
  uint32_t megaAt = 0;   // cuando empezo la escena de la mega
  // De QUIEN es la escena de la mega. El rival tambien megaevoluciona y
  // hasta ahora lo hacia de golpe, sin anillos y sin sonido: la escena
  // estaba clavada en el lado del jugador (cx=145) porque era el unico
  // que llegaba a ella.
  uint8_t megaSide = 0;
  // De que lado y desde cuando la mega vuelve del blanco (btMegaBlanco).
  uint8_t megaRevelaLado = 0xFF;
  uint32_t megaRevelaAt = 0;
  // Excavar / Vuelo / Buceo: el motor saca al bicho del campo durante un
  // turno (Combatant::hiding), pero el dibujante no se enteraba y el
  // sprite se quedaba plantado donde estaba. Esto es el escondite que se
  // vio en el ultimo fotograma, de que clase era y cuando cambio, que es
  // todo lo que hace falta para animar la ida y la vuelta.
  uint8_t hidWas[2] = { 0, 0 };
  uint8_t hidKind[2] = { 0, 0 };
  uint32_t hidAt[2] = { 0, 0 };
  // O bicho que CARREGA (Fly, Dig...) so some depois da animacao de carga,
  // como no Anil (frase, animacao, e so entao some). Ate esta hora o
  // `hiding` dele nao se desenha.
  uint32_t cargaAte[2] = { 0, 0 };
  // A animacao de carga TOCOU: ela ja o tirou de cena (a bola do Fly sobe,
  // o Dig afunda), entao ele entra ja sumido, sem a nossa transicao.
  bool cargaAnim[2] = { false, false };
  // BP_CARGA: 0 = frase + animacao de carga, 1 = a subida (Electrorrayo),
  // 2 = vai para o golpe.
  uint8_t cargaPasso = 0;
  uint16_t cargaMv = 0;        // o golpe que carrega
  uint16_t cargaHpPend = 0;    // o hpPend do golpe, guardado durante a frase
  bool cargaAnimOk = false;    // a animacao do golpe estava pronta
  bool open = false;
  Battle b;
  uint8_t phase = BP_INTRO;
  uint32_t until = 0;      // fin de la fase actual
  uint8_t sel = 0;         // golpe seleccionado en la tarjeta
  uint8_t acting = 0;      // quien esta pegando en esta fase
  bool secondPending = false;  // falta el golpe del otro este turno
  uint8_t foeSlot = 0;
  uint8_t pendingSlot = 0;  // el golpe anunciado, pendiente de resolverse
  uint8_t extraStep = 9;    // cola de avisos tras el golpe: 0 drenaje, 1 retroceso, 2 estado
  uint8_t endStep = 0;      // 0 y 1 = el desgaste de cada lado al cerrar el turno
  uint8_t introAbil = 0;    // por que bando va el anuncio de habilidad de salida
  uint8_t fainter = 0;      // quien se esta hundiendo en BP_FAINT
  // La entrada del que acaba de salir de la bola. No es una fase: es
  // cuando empezo (0 = nadie entrando) y de que lado. Ver btSendoutFx.
  uint32_t sendAt = 0;
  uint8_t sendSide = 0;
  int8_t gym = -1;          // -1 = salvaje; si no, el indice en GYM_TBL
  // -1 = hierba; si no, el entrenador que te reta. No se captura y
  // paga mejor: por eso hace falta saberlo al cerrar el combate.
  int8_t trainer = -1;
  // Y si ESE entrenador es uno de los tres de la casa del gimnasio,
  // cual (0..2). -1 = es uno de a pie, de los del camino.
  //
  // Va aparte de `trainer` y no en su lugar porque la cara, el nombre y
  // todo lo que ya estaba escrito para un entrenador (que no se huye,
  // que no se captura, la propina) vale igual para estos: lo unico
  // propio es de donde salen sus bichos y que al ganar suben el porton.
  int8_t gymTr = -1;
  // REVANCHA: el mismo desafio, pero despues de coronarse. Todo el
  // equipo pelea al nivel del campeon, la piedra mega entra aunque sea
  // un lider del principio, y el botin paga mejor. No da insignia —— de
  // eso ya se encarga Pet::winGym(), que se va de vacio con las ocho
  // puestas.
  bool rematch = false;
  // A insignia ja foi dada na tela BP_BADGE (winGym rodou ali): o
  // battleClose nao pode dar de novo.
  bool insigniaDada = false;
  uint8_t insigniaIdx = 0;
  // Cuantos bichos le quedan por sacar (1 = el que esta en pista).
  // Lo que se gano en este combate, para poder ENSENARLO. Hasta dos:
  // lo que llevaba encima el salvaje y lo que suelta el combate.
  uint8_t loot[2] = { 0, 0 };
  uint8_t lootShown = 0;

  uint8_t foesLeft = 1;
  uint8_t foeIdx = 0;       // cual del equipo esta en pista
  // El ULTIMO de un entrenador del camino, si una Ilusion ya tuvo que
  // saber quien era (leva 70). 0 = aun sin sortear.
  int16_t foeUltimo = 0;
  uint16_t foeUltimoNv = 0;
  // Captura. `bagScroll` es la lista de la bolsa; `taps` lo que el jugador
  // consigue empujar mientras la bola se mueve.
  uint8_t bagScroll = 0;
  uint8_t ballId = 0;       // la bola que se acaba de tirar
  uint8_t taps = 0;
  uint8_t chance = 0;       // 0..100, se fija al soltar la bola
  uint8_t wobble = 0;       // sacudidas que quedan por ensenar
  bool caught = false;
  // Desde que el bicho esta DENTRO de la bola. Mientras valga, no se le
  // dibuja: antes se quedaba plantado en su sitio mientras la bola se
  // sacudia en el suelo, que es lo que lo delataba como un adorno.
  bool foeInBall = false;
  // Fondo de /bg: se elige al empezar (gimnasio, entrenador o el tipo del
  // salvaje) y se deja fijo. -1 = el cielo pintado de siempre, que es lo que
  // se ve si no hay SD o falta el fichero.
  int8_t bg = -1;
  // Instante en que empezo el combate, para el destello de shiny. Se mide
  // desde aqui y NO desde la fase: los anuncios de habilidad de salida
  // vuelven a BP_INTRO reiniciando `until`, y el destello se repetia.
  uint32_t startedAt = 0;
  // Efecto del golpe en curso: el TIPO (no el golpe) decide el dibujo, y son
  // 437 golpes contra 18 tipos. T_NONE = nada que pintar, que es lo que pasa
  // cuando el golpe falla o lo paran.
  uint8_t fxType = 0;
  // Indice en FX_SUP cuando el golpe no pega, o 0xFF para usar el del tipo.
  uint8_t fxSup = 0xFF;
  // El golpe en curso: si tiene animacion propia (FX_BY_MOVE) manda esa.
  uint16_t fxMove = 0;
  uint32_t fxAt = 0;
  uint8_t fxSide = 0;       // sobre quien se pinta
  // LA HOJA QUE FALTA POR TRAER de la tarjeta, y desde cuando.
  //
  // Se pedia al RESOLVER el golpe, o sea DESPUES de los 850 ms del
  // anuncio: la lectura caia en el hueco de ~200 ms que queda antes de
  // la embestida, y por eso el generador tenia que dejar fuera 51
  // golpes por caros (Crunch pide 122 KB, y a 0,2 MB/s son 600 ms).
  //
  // Ahora se apunta en btLaunch y la trae battleTick MIENTRAS se lee la
  // frase. El retraso no es capricho: leer antes de que el anuncio este
  // pintado seria mover el tiron delante, que es peor —— se notaria
  // como que el toque no responde.
  uint16_t fxPend = 0;
  // A animacao do Anil: a pedida (lida durante o anuncio, como a
  // fxPend) e se a que esta tocando e ela.
  uint16_t animPend = 0;
  bool animPendRival = false;
  bool anim = false;
  // Fim da animacao de CAMPO (Drought & cia). Ate la o combate espera:
  // senao o anuncio do proximo golpe carregava a animacao dele no mesmo
  // lugar e cortava o sol no meio.
  uint32_t animFim = 0;
  uint32_t fxPendAt = 0;
  // Que hueco del equipo esta en pista. Arranca en `pet.mainSlot` (el que
  // crias) y cambia con los relevos; NO toca `mainSlot`, que es otra cosa:
  // pelear con un suplente no cambia de quien cuidas.
  uint8_t mySlot = 0;
  // LOS OBJETOS DE ESTE COMBATE, por hueco del equipo (6 = TEAM_SLOTS, hay
  // un static_assert en battle_ui.ino). `itemIni` es con lo que entro cada
  // uno y `itemAhora` lo que lleva en este momento: lo que le quitan
  // (Desarme, Ladron, Trapicheo) sigue quitado mientras dure el combate, y
  // al acabar vuelve. `itemGastado` es la mascara de los que se lo
  // GASTARON de verdad (baya, Banda Focus): esos si lo pierden.
  uint8_t itemIni[6] = { 0 };
  uint8_t itemAhora[6] = { 0 };
  uint8_t itemGastado = 0;
  uint8_t foeItemIni = 0;   // con lo que salio el salvaje (para el botin)
  uint8_t partyScroll = 0;  // la lista de relevos
  // EL OBJETO QUE ESTA EN LA MANO esperando a que elijas en quien. 0 =
  // la lista del equipo es la de siempre (relevar).
  //
  // Lo trajo el dueno: "revive esta quebrado, nao tem como usar na
  // batalha pois nao deixa eu escolher o pokemon". Y era verdad —— la
  // bolsa lo aplicaba todo al que estaba EN PISTA, asi que un Revivir
  // no tenia a quien levantar: el de pista, por definicion, esta en
  // pie. Quien pregunta y quien no lo dice el PBS del Anil en
  // `BattleUse`, y viaja en ItemEntry::use (IU_*).
  uint8_t itemPick = 0;
  bool mustSwitch = false;  // cayo el tuyo y queda banquillo: no se cancela
  HitResult last;
  // MULTIGOLPE en pantalla: que impacto va, y cuanto dano del golpe queda
  // todavia por ENSENAR en la barra (el motor ya lo resto todo de golpe).
  uint8_t golpeN = 0;
  uint16_t hpPend = 0;
  // El peor caso ya no es "CHARMANDER used STOMPINGTANTRUM!" (32) sino la
  // desobediencia: "CHARMANDER did STOMPINGTANTRUM instead!" son 38.
  char msg[44] = { 0 };
  // Que bolsillo de la bolsa se esta mirando en el combate: 0 curativos,
  // 1 bolas. Mezclados, con cinco filas y sin forma de bajar, las bolas
  // quedaban enterradas detras de las pociones.
  uint8_t bagPocket = 0;
  // Lo que se ha ganado en ESTE combate, para poder contarlo al final.
  uint32_t expWon = 0;     // experiencia sumada entre todos los que cayeron
  uint8_t lvlWon = 0;      // niveles que ha subido el que peleaba
  uint16_t lvlAntes = 0;   // y de que nivel venia
  int16_t winDex = 0;      // quien peleaba (no siempre el principal)
  // E de que hueco do time: se o primeiro cai e OUTRO termina o combate, a
  // tela de vitoria e do segundo (ver btFoeDefeated).
  uint8_t winSlot = 0xFF;
  // BATALHA ENTRE PLACAS (pvp.h): os dois times sao de registros, nada
  // toca no save, e a jogada do lado 1 vem da outra placa.
  bool pvp = false;
  bool pvpMega = false;    // marquei megaevoluir neste turno
};
extern BattleUi bt;

// gym: -1 para un salvaje, o el indice en GYM_TBL de a quien desafias.
void battleStart(int16_t foeDex, uint16_t foeLevel, int8_t gym = -1,
                 bool foeShiny = false, int8_t trainer = -1,
                 int8_t gymTr = -1, bool revancha = false);
// Arranca la REVANCHA del desafio `gym` de la region actual: el equipo
// entero al nivel del campeon. Es la puerta que usa la pantalla.
void rematchStart(uint8_t gym);
// Un bicho del TIPO de la casa, para los tres del gimnasio. De cualquier
// region, NUNCA legendario y con un techo de fuerza atado a `nivel`.
// Devuelve 0 si no hay ninguno (y entonces se cae al reparto de la hierba).
int16_t gymTrainerSpecies(uint8_t tipo, uint8_t nivel);
// Lanza el turno con el golpe ya elegido (decide el orden y saca al
// primero). Se declara aqui porque battleTick() la llama desde mucho
// mas arriba del fichero que donde esta escrita: la mega del rival se
// mete entre elegir golpe y pegar, y al acabar su escena hay que seguir.
void btCommitTurn();
void battleTick();          // avanza las fases; llamar desde loop()
void renderBattle();
void battleTap(int16_t x, int16_t y);

// Cuanto lleva tragado el rival, 0..1 (1 = ya esta dentro y no se dibuja).
// Vive en capture.ino, que se concatena DESPUES de battle_ui.ino, asi que
// se declara aqui y no se depende del generador de prototipos.
float btSuckProgress(uint32_t now);

// El boton de HUIR, por el mismo motivo: vive en capture.ino y lo
// llaman el pintado y el toque de battle_ui.ino.
void btDrawRunButton();
bool btRunTap(int x, int y);

// Los OBJETOS sobre un hueco del equipo. Viven en capture.ino con el
// resto de la bolsa, y los llama la lista del equipo (battle_ui.ino),
// que se concatena ANTES: por eso se declaran aqui.
//
//   `aplicar` a false pregunta "¿le haria algo?" sin tocar nada. Es la
//   misma funcion y no una copia de las condiciones: una copia se
//   habria quedado vieja al primer objeto nuevo, y entonces la lista
//   ofreceria huecos que al tocarlos pitan.
bool btItemEnSlot(uint8_t id, uint8_t slot, bool aplicar);
bool btItemPickTap(int16_t x, int16_t y);
static inline bool btItemSirveEn(uint8_t id, uint8_t slot) {
  return btItemEnSlot(id, slot, false);
}
