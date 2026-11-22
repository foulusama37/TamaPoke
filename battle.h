#pragma once
#include <stdint.h>

#include "abilities.h"
#include "dex.h"
#include "forms.h"
#include "formas_combate.h"
#include "items.h"
#include "megas.h"
#include "moves.h"

// El cielo se declara abajo, pero abilities.h necesitaba nombrarlo antes para
// su tabla y trae su propio enum AB_WE_*. Que los dos coincidan no se puede
// dejar a la suerte: si alguien reordena uno, el Chlorophyll empezaria a
// activarse con lluvia y nadie lo notaria hasta jugar.
#define AB_WEATHER_MATCHES (AB_WE_SUN == 1 && AB_WE_RAIN == 2 && \
                            AB_WE_SAND == 3 && AB_WE_HAIL == 4)

// Combate 1v1. La logica vive aqui y no en el .ino a proposito: asi se compila
// y se prueba en el PC con los tests, que es donde se cazan los errores de
// formula. El .ino solo dibuja lo que este estado dice.

#define BT_MOVES 4

// Quien pega y quien recibe, para no confundir lados en las funciones.
enum : uint8_t { BT_PLAYER = 0, BT_FOE = 1 };

// O DADO DO COMBATE. Toda tirada do motor passa por aqui.
//
// Fora da batalha entre placas e o random() de sempre (os testes com
// semente fixa continuam a ver a MESMA sequencia). Na batalha entre placas
// as duas simulam o mesmo combate a partir das mesmas escolhas, e isso so
// da o mesmo resultado se o dado for o mesmo: na placa o random() e o
// gerador de HARDWARE do ESP32, que nao aceita semente, e alem disso a
// tela gasta random() em coisas que so uma das placas desenha. Com
// btRngSemente() o motor passa a um gerador proprio, que so ele usa.
long btRand(long n);
void btRngSemente(uint32_t s);   // liga o gerador proprio (s != 0)
void btRngDesliga();             // volta ao random()
bool btRngLigado();
uint32_t btRngEstado();

// ST_* (estados) y SG_* (stages) viven en btstate.h, que llega por items.h:
// la tabla de objetos tambien los nombra (un Antidoto es IT_CURE ST_POISON) y
// no podian quedarse aqui sin montar un ciclo de includes.
#define SCREEN_TURNS 5

// Por que no puede moverse un combatiente este turno.
enum : uint8_t { BLK_NONE = 0, BLK_SLEEP, BLK_FREEZE, BLK_PARALYSIS,
                 BLK_FLINCH, BLK_CONFUSED, BLK_RECHARGE, BLK_DISOBEY,
                 BLK_TAUNT,   // le han mofado: nada de golpes de apoyo
                 BLK_TRUANT,  // Pereza: este turno le toca vago
                 BLK_ATTRACT, // enamorado: se queda mirando
                 BLK_HEALBLOCK,  // Anticura: el golpe cura y no puede
                 BLK_COUNT };

// OBEDIENCIA. Es lo que ata el lado tamagotchi al combate: un bicho mal
// cuidado pelea peor, y no por tener menos stats sino por no hacerte caso.
// Solo le pasa al bicho del JUGADOR — un salvaje no tiene a quien desobedecer.
//
// El tope existe a proposito. Perder uno de cada cinco turnos ya escuece y se
// nota enseguida; perder la mitad convertiria el combate en una moneda al
// aire y el jugador dejaria de pelear en vez de cuidar mejor, que es justo lo
// contrario de lo que se busca. El carino es el unico freno y es total: a
// partir de OBEY_BOND_OK obedece siempre.
#define OBEY_BOND_OK 60   // vinculo desde el que obedece siempre
#define OBEY_MAX_PCT 20   // tope de turnos perdidos con el vinculo por los suelos

// CAPTURA. Las probabilidades son ALTAS a proposito, y eso no es un
// descuido de equilibrio: aqui las bolas son escasas de verdad (no hay
// tienda, salen del cuidado diario y de lo que sueltan los combates). La
// peor sensacion de Pokemon es cruzarte con algo raro y no tener con que
// cogerlo; si las bolas son pocas, fallar tambien tiene que ser raro.
//
// Y el jugador puede empujar: mientras la bola se mueve, cada toque suma.
// No lo decide todo (CAPT_TAP_MAX), pero convierte los tres segundos de
// espera en algo que se hace en vez de algo que se mira.
#define CAPT_BASE 40        // suelo, con el bicho entero y una Poke Ball
#define CAPT_BY_DAMAGE 40   // lo que suma tenerlo a cero de PS
#define CAPT_SLEEP 15       // dormido o congelado
#define CAPT_STATUS 8       // cualquier otro estado
// 3 por toque y hasta 30, no 2 y 20. Con los numeros viejos habia que
// dar DIEZ toques para el tope y cada uno movia la barra dos pixeles de
// cien: aporreabas la pantalla sin notar nada. Ahora el tope llega en
// diez igual, pero cada toque se ve, y el premio por insistir pasa de
// un quinto a casi un tercio de la tirada.
#define CAPT_TAP 3          // por toque durante el forcejeo
#define CAPT_TAP_MAX 30     // tope de lo que aportan los toques
#define CAPT_FLOOR 5        // nunca imposible
#define CAPT_CEIL 95        // nunca seguro

// (la funcion se declara al final, cuando Combatant ya existe)

enum : uint8_t { OB_OBEY = 0, OB_IGNORE, OB_OTHER };
struct Order {
  uint8_t slot = 0;        // el golpe que va a salir DE VERDAD
  uint8_t kind = OB_OBEY;  // OB_IGNORE: se planta. OB_OTHER: hace lo que quiere.
};

// El cielo. Vive en el combate y no en el combatiente porque le pasa a los
// DOS a la vez, y porque es lo que convierte Sunny Day de "un golpe que no
// hace nada" en "un golpe que cambia las reglas cinco turnos".
enum : uint8_t { WE_NONE = 0, WE_SUN, WE_RAIN, WE_SAND, WE_HAIL };
#define WEATHER_TURNS 5
// El sol del Velo Vapor dura mas que un Dia Soleado normal: lo dice la
// descripcion de la habilidad en el PBS del Anil, que es la unica
// especificacion que hay (el Mega Arcanine se lo invento el Anil).
#define STEAM_VEIL_TURNS 6
static_assert(AB_WE_SUN == WE_SUN && AB_WE_RAIN == WE_RAIN &&
              AB_WE_SAND == WE_SAND && AB_WE_HAIL == WE_HAIL,
              "los dos enums de clima tienen que coincidir");

// El TERRENO. Es el hermano del cielo: dura lo mismo y tambien vive en el
// combate, pero solo alcanza a QUIEN PISA EL SUELO — un Pidgey volando se
// libra tanto del campo electrico como de la curacion del campo de hierba.
// Esa es toda la diferencia, y es la que le da sentido a que sean dos cosas.
// TR_* (el SUELO) se mudo a btstate.h: abilities.h lo necesita para los
// cuatro "Surge" y no puede incluir battle.h, que es quien le incluye a el.
// Se mudo en vez de duplicarlo como se hizo con el clima (ver el
// static_assert de AB_WE_*): un enum en dos sitios es un enum que algun
// dia deja de coincidir.
#define TERRAIN_TURNS 5
#define TERRAIN_BOOST 130   // x100 al golpe del tipo del terreno
#define TERRAIN_DRAGON 50   // el campo de niebla parte el dano dragon
#define GRASSY_HEAL_DIV 16  // lo que cura el campo de hierba por turno

// multiplicador x100 de un stage -6..+6 (el de siempre: 2/8 .. 8/2)
uint16_t stageMul(int8_t s);

struct Combatant {
  int16_t dex = 0;        // la especie de verdad (la que se registra, la que evoluciona)
  int16_t asDex = 0;      // la que APARENTA: Transform la cambia, el resto del tiempo == dex
  uint16_t level = 1;
  bool shiny = false;
  bool transformed = false;
  // Los STATS copiados por Transform, ya calculados y SIN escalones
  // (atk, def, spa, spd, spe). 0 = no se copio ninguno.
  //
  // En los juegos Transform copia los STATS DEL OTRO, no su especie: un
  // Ditto de nivel 5 que copia un Dragonite de 50 pega como ese
  // Dragonite. Copiando solo la especie aparente se recalculaban con MIS
  // genes y MI nivel —— y entonces copiar a uno grande no servia de nada.
  uint16_t copiado[5] = { 0, 0, 0, 0, 0 };

  uint16_t hp = 0, hpMax = 0;
  uint16_t moves[BT_MOVES] = { 0, 0, 0, 0 };
  uint8_t usesLeft[BT_MOVES] = { 0, 0, 0, 0 };
  uint8_t nMoves = 0;

  // genes y entrenamiento: los trae el bicho del jugador, un salvaje va a 100/0
  uint8_t geneHp = 100, geneAtk = 100, geneDef = 100;
  uint8_t geneSpa = 100, geneSpd = 100, geneSpe = 100;
  uint8_t trHp = 0, trAtk = 0, trDef = 0;
  uint8_t trSpa = 0, trSpd = 0, trSpe = 0;

  uint8_t status = ST_NONE;
  uint8_t statusTurns = 0;    // cuanto queda de sueno; vueltas de toxico
  uint8_t confuseTurns = 0;   // 0 = no confundido
  bool flinched = false;      // dura solo este turno
  // { 0 } y no una lista de ceros: la lista tenia SEIS cuando SG_COUNT ya
  // era siete, y aunque aqui el relleno del compilador daba lo correcto, en
  // las dos tablas de nombres el mismo despiste imprimia "<null>".
  int8_t stage[SG_COUNT] = { 0 };

  // Los cuatro estados de varios turnos. Son 5 bytes y con ellos entran
  // Bind y sus 6 hermanos, Outrage y los suyos, Hyper Beam y Solar Beam.
  uint8_t bindTurns = 0;      // atrapado: pierde PS al cerrar el turno
  uint8_t lockTurns = 0;      // Outrage: obligado a repetir, luego se confunde
  uint8_t lockSlot = 0;       // que golpe esta repitiendo
  bool recharging = false;    // Hyper Beam: este turno no hace nada
  uint16_t chargingMove = 0xFFFF;  // Solar Beam: cargo, y ahora suelta

  // Pantallas: van en el combatiente y no en el combate porque protegen a UN
  // lado. Con un bicho por banda es lo mismo, pero el dia que haya 3x3 esto ya
  // esta donde tiene que estar.
  uint8_t reflect = 0, lightScreen = 0, safeguard = 0;

  // Solo este turno
  bool protecting = false;    // Protect: no le entra nada
  bool enduring = false;      // Endure: aguanta con 1 PS
  uint8_t protectRun = 0;     // veces seguidas: encadenarlo falla cada vez mas
  bool sureHit = false;       // Mind Reader: el siguiente no falla
  bool sureCrit = false;      // Laser Focus: el siguiente es critico

  int8_t critStage = 0;       // Focus Energy sube esto, no un stat normal
  bool leechSeed = false;     // le chupan un octavo por turno
  bool aquaRing = false;      // se cura un dieciseisavo por turno
  // MALDICION, pero la de un FANTASMA: un cuarto de PS cada turno y no
  // se quita con nada. El que la echa paga la mitad de los suyos. Si el
  // que la usa NO es fantasma el golpe es otra cosa (stats) y esto ni se
  // toca —— por eso vive aqui y no en `status`.
  bool cursed = false;
  // AUTOTOMIA: cuantas veces se ha quitado peso de encima. Cada una son
  // 100 kg menos (el Anil resta 1000 decimas a PBEffects::WeightChange),
  // y el peso no baja de 0,1 kg. Importa porque hay dos golpes que lo
  // miran —— Patada Baja contra el rival y Cuerpo Pesado comparando los
  // dos —— y porque Peso Pesado lo dobla.
  uint8_t ligero = 0;

  // RIZO DEFENSA deja al bicho enroscado, y eso no es un adorno: en
  // el Anil le suma un escalon entero al 2**shift de Rodar y Bola
  // Hielo, o sea que DOBLA la cadena entera. Vive aqui y no en un
  // stage porque la subida de defensa ya la hace el sx; esto es la
  // otra mitad del golpe, que es la que faltaba.
  bool curled = false;

  // Para Disable, Encore y Spite hace falta recordar por donde iba el rival.
  uint16_t lastMove = 0xFFFF;
  uint8_t lastSlot = 0xFF;
  uint8_t disabledSlot = 0xFF, disableTurns = 0;
  uint8_t encoreSlot = 0xFF, encoreTurns = 0;
  // El Encore le entro a mitad de ronda sobre un golpe que no puede elegir
  // (anulado, mofado...): esta ronda sigue con el que habia pedido
  // (169_Battler_UseMove.rb:168). Se apaga al cerrar la ronda.
  bool encoreSalta = false;

  // Lo que le acaban de hacer este turno, para Counter y Mirror Coat.
  uint16_t tookPhys = 0, tookSpec = 0;

  // Veces seguidas que ha repetido el MISMO golpe. Lo piden los que
  // crecen al repetirse: Corte Furia dobla cada vez y Rodar va x2, x4,
  // x8... Se pone a cero en cuanto cambia de golpe o falla.
  uint8_t rachaMove = 0;
  // Esta ronda no tenia golpe que elegir y sale Forcejeo. Lo decide
  // inicioDeRonda() (pbAutoChooseMove del Anil) y dura la ronda. Vive AQUI,
  // en el byte de relleno que quedaba antes de `rachaDe`, para no engordar
  // la ficha (el tope de sizeof(Combatant) lo vigila un test).
  bool forcejea = false;
  uint16_t rachaDe = 0xFFFF;   // cual es el golpe de la racha

  // Mofa: tantos turnos sin poder usar golpes de apoyo. Es lo que
  // obliga a pegar a quien se habia montado una estrategia de subirse
  // stats, y no se podia hacer antes porque no habia donde apuntarlo.
  uint8_t tauntTurns = 0;
  // BASTION FINAL (No Retreat): ya lo uso, asi que no puede repetirlo ni
  // relevarse (PBEffects::NoRetreat). Va en el byte de relleno de antes de
  // `subHp`: no engorda la ficha.
  bool noRetreat = false;
  // SUSTITUTO: los PS del muneco. Mientras tenga, los golpes le pegan a
  // el y no al bicho, y los de apoyo del rival fallan (menos los de
  // sonido, que lo atraviesan). Se va al relevar, como en los juegos.
  uint16_t subHp = 0;
  // EMBARGO: turnos en que el objeto que lleva no hace nada.
  uint8_t embargoTurns = 0;
  // ANTICURA: turnos en que no puede curarse: ni golpes que curan, ni
  // drenajes, ni Restos, ni bayas, ni el Deseo.
  uint8_t healBlockTurns = 0;
  // BOSTEZO: 2 al recibirlo, baja al cerrar cada turno y a 0 se duerme.
  uint8_t yawn = 0;
  // PICO CANON y COPARAZON: los marca el principio de la ronda (el Anil
  // lo hace al elegir, antes de que nadie se mueva) y los borra el cierre.
  bool beakBlast = false;
  bool shellTrap = false;
  // VENGANZA (Bide): turnos que le quedan aguantando, y lo que lleva
  // recibido. 0 = no esta en ello.
  uint8_t bideTurns = 0;
  uint16_t bideDmg = 0;
  // Rastreo: le han leido los movimientos. Pierde la evasion que
  // tuviera y deja de ser intocable para normal y lucha.
  bool foreseen = false;
  // OJO DE MENTE: el hermano de Rastreo con los tipos cambiados —— le
  // anula la evasion igual, y al SINIESTRO deja de hacerle cero lo
  // psiquico (en vez del fantasma contra normal y lucha). Va aparte y
  // no en `foreseen` porque la mitad del tipo es distinta, y mezclarlas
  // haria que Rastreo tumbase la inmunidad del siniestro.
  bool mirado = false;
  // Atrapado: ni huye ni releva. Lo ponen Mal de Ojo, Bloqueo y
  // Telarana, que hasta que existieron la huida y los relevos no
  // significaban nada.
  bool trapped2 = false;
  // Antiaereo: le han bajado al suelo. Deja de librarse de los golpes
  // de tierra por ser volador.
  bool grounded2 = false;
  // Niebla: turnos en los que no le bajan ningun stat.
  uint8_t mistTurns = 0;
  // Golpes que ha usado al menos una vez. Lo pide Ultima Baza, que
  // falla mientras le quede alguno sin estrenar.
  uint8_t usedMask = 0;
  // Veces seguidas que SU LADO ha usado Eco Voz. Va aqui y no en la
  // racha normal porque no se pierde al relevar: la voz sigue sonando
  // aunque cambies de bicho.
  uint8_t echoRun = 0;
  // Le acaba de caer un companero: Represalia pega el doble.
  // Espejo de vengefulSide[] (ver Battle). Vive aqui porque computeHit
  // es una funcion LIBRE y no ve los arrays del combate, igual que
  // `chanted` es el espejo de luckyChant[].
  // LAZO DESTINO y RENCOR: los dos MARCAN y no tumban a nadie. Lo que
  // marcan solo pasa si el que los uso cae antes de volver a moverse ——
  // y antes los dos le mataban en el acto, que es regalarle el turno al
  // rival.
  bool destinyBond = false;
  bool grudge = false;
  bool vengeful = false;
  // Cuantos golpes le han entrado en este combate, y cuantos companeros
  // han caido. Los pide Punorrabia (+50 por golpe recibido) y Ultimo Adios
  // (+50 por caido): son dos contadores y no un si/no como `vengeful`,
  // porque el golpe crece con la cuenta.
  uint8_t hitsTaken = 0;
  uint8_t alliesFallen = 0;
  uint8_t saltCure = 0;     // Salazon puesta: le roe cada turno hasta que salga
  bool expuesto = false;    // Asalto Espada: hasta su turno, le pegan el doble
  uint16_t noRepetir = 0;   // Martillo Giga: el golpe que NO puede repetir
  uint8_t tipoPerdido = 0;  // T_* que se quemo (Ultimo Fuego, Electrochoque)
  // RESPIRO, que dura UN TURNO y lo borra el cierre de turno. No se
  // mete en `tipoPerdido` porque aquel es permanente mientras el
  // bicho siga en pista, y este se va solo.
  bool roosted = false;
  // Se ha comido una baya en este combate. Lo pide Eructo, que falla
  // mientras no haya nada que eructar.
  bool ateBerry = false;
  // CUAL se comio, para que Cosecha pueda devolverla: sin esto la
  // habilidad no tendria nada que recuperar.
  uint8_t lastBerry = IT_NONE;
  // Su ultimo golpe fallo (o le fallo). Lo pide Pataleta, que pega el
  // doble justo entonces.
  bool lastFailed = false;
  // Turnos que le queda de Golpe Mordaza: sin golpes de SONIDO.
  uint8_t gagTurns = 0;
  // SELLAR: tiene un sello puesto, asi que el RIVAL no puede usar los
  // golpes que este tambien sabe. La bandera va en el que lo puso.
  bool imprisoning = false;
  // PEREZA: un turno si y otro no. El bit dice si a ESTE le toca vago.
  bool loafing = false;
  // Las de entrada que valen UNA vez (Espada Indomable, Escudo Recio):
  // su ficha dice "la primera vez que entra", no en cada relevo.
  bool entryDone = false;
  // Los bits de las habilidades "de forma". Leva 70: el Anil SI trae
  // las formas (Castform al sol, Aegislash Filo...), y ahora se ganan y
  // se pierden de verdad: `bform` abajo, con sus stats, sus tipos y su
  // dibujo. Estos bits siguen siendo el ESTADO de cada habilidad.
  bool blade = false;      // Cambio Tactico: afilado en vez de escudado
  bool hero = false;       // De Cero a Heroe: ya se retiro una vez
  bool disguised = false;  // Disfraz: la cascara esta entera
  bool gulped = false;     // Tragamisil: lleva algo en la boca
  bool hungry = false;     // Mas que Mono: le toca forma voraz
  bool illusion = false;   // Ilusion: aun lleva la cara de otro
  // A QUIEN aparenta: el ultimo del equipo que aun puede luchar
  // (pbLastInTeam del Anil). Lo rellena quien conoce el equipo -- la
  // pantalla, o pvp.cpp -- antes de sacarlo; 0 = nadie (un salvaje, o
  // el ultimo es el mismo), y entonces la Ilusion no se enciende.
  int16_t ilusDex = 0;
  uint8_t ilusForm = 0;
  bool ilusShiny = false;
  // La FORMA DE COMBATE puesta: fila de BFORM_TBL, o -1. Se pone y se
  // quita dentro del combate (Battle::syncForma) y al salir de la pista
  // se olvida. Manda sobre la regional y cede ante la mega.
  int8_t bform = -1;
  // Lo que le dejo el que se fue: 0 nada, 1 sus especiales un 30%
  // mas (Bateria), 2 todos (Punto Fuerte). Dura mientras este el en
  // la pista; al relevar otra vez se va con el.
  uint8_t charged = 0;
  // CUERPO MORTAL y CANTO MORTAL: turnos que le quedan antes de caer
  // (3, 2, 1, 0 y cae al cerrar el siguiente). 0xFF = sin cuenta.
  // Relevarse la borra: es la salida, y la gracia de los dos.
  uint8_t perish = 0xFF;
  // ATRACCION: enamorado, la mitad de los turnos se queda mirando.
  bool attracted = false;
  // CONJURO: su bando esta protegido de criticos. Viaja en el bicho
  // porque computeHit no sabe de bandos; lo pone el cierre de turno.
  bool chanted = false;
  // LEVITON: turnos flotando. Mientras dure, tierra no le entra.
  uint8_t levitating = 0;
  // TORMENTO: no puede repetir el golpe anterior. Es un bloqueo distinto
  // de Mofa y de Mordaza, y se mira contra `lastSlot`.
  bool tormented = false;
  // ARRAIGO: se cura cada turno y ya no se puede ir. Va aparte de
  // `aquaRing` porque ese no ata a nadie.
  bool ingrained = false;
  // El deposito de Reserva: 0..3. Lo llena Reserva y lo gastan Escupir
  // (potencia) y Tragar (cura). Los tres solos no valen nada; es el
  // deposito el que los convierte en una jugada.
  uint8_t stockpile = 0;
  // Donde se ha metido mientras carga: 0 en la pista, HID_* escondido.
  // Mientras no esta en la pista casi nada le alcanza, y los pocos que si
  // le pillan le pegan el doble. Es la mitad de Excavar y de Vuelo que
  // faltaba: sin esto eran Rayo Solar con otro nombre.
  uint8_t hiding = 0;
  // FURIA: desde que la usa, cada golpe que RECIBE le sube el ataque. Se
  // va al relevar, como todo lo volatil.
  bool raging = false;
  // Acaba de entrar relevando POR GUSTO (no por una caida). Es la unica
  // cosa que Persecucion castiga. Lo pone sendOut() y lo borra el cierre
  // de turno, asi que vale para el golpe gratis del rival y nada mas.
  bool relieved = false;

  // Lo que lleva encima. Las bayas se GASTAN al usarse, asi que `item` se
  // pone a 0 y hay que acordarse de no devolverselo al bicho al acabar.
  uint8_t item = IT_NONE;
  // LIVIANO: "acaba de largar lo que llevaba", no "va ligero". Se
  // enciende al PERDER un objeto y se apaga al salir a pista. Sin esto,
  // el que entraba con las manos vacias corria el doble para siempre.
  bool soltoAlgo = false;
  // El unico sitio por el que se pierde un objeto. Existe para que la
  // bandera de arriba no haya que acordarse de ponerla en los ocho
  // sitios que lo limpiaban a mano.
  // `comido` = lo GASTO (baya, Banda Focus, Lanzamiento...). Lo que le
  // quitan (Desarme, Ladron, Trapicheo) NO lo es: en el Anil eso es
  // pbRemoveItem(false) y el objeto vuelve al acabar el combate.
  void perderObjeto(bool comido = false) {
    if (item == IT_NONE) return;
    if (comido) itemComido = item;
    item = IT_NONE;
    soltoAlgo = true;
  }
  uint8_t choiceSlot = 0xFF;  // atado por Choice Band/Specs al primero que uso
  // El ultimo objeto que GASTO de verdad en este combate. Lo lee la pantalla
  // al guardar: si es el que traia puesto, ese si se pierde. Va en el byte
  // de relleno de antes de `ability`.
  uint8_t itemComido = IT_NONE;

  // La habilidad. A diferencia del objeto, no se elige ni se cambia: sale en
  // el huevo y es parte de quien es el bicho.
  // uint16_t: son 304 habilidades con la dex entera (ver party.h).
  uint16_t ability = 0;
  // La FORMA regional (0 = la normal). Se lleva igual que la mega: una
  // variable y los accesores miran. La MEGA manda sobre ella, porque
  // megaevolucionar sustituye la forma entera.
  uint8_t form = 0;
  bool abilityShown = false;  // ya se anuncio al empezar (Intimidate y cia)

  // Vinculo, solo util en el lado del jugador. 100 por omision para que un
  // rival (y cualquier combate de prueba) obedezca siempre sin tener que
  // acordarse de ponerlo.
  uint8_t bond = 100;
  bool refusing = false;      // este turno se planta; lo pone giveOrder()

  // El SEXO, para Rivalry (pega mas al del mismo). 0 macho, 1 hembra,
  // 2 sin sexo. init() lo saca del reparto de la especie; el lado del
  // jugador lo pisa con el de su bicho, que ya estaba decidido.
  uint8_t sexo = 2;
  // Le echa el sexo que le toque por su especie. Lo llama quien saca a
  // un SALVAJE; el del jugador se copia del bicho, que ya lo tenia.
  void rollSexo() {
    uint8_t fem = valid() ? DEX_TBL[asDex].femPct : 255;
    sexo = fem == 255 ? 2 : (uint8_t)(random(100) < fem ? 1 : 0);
  }
  // Turnos que lleva fuera. Lo pide Acometida (del Anil): el PRIMER
  // turno que sale, +50% de velocidad y +20% de ataque. Sin contador
  // no hay forma de saber cual es el primero.
  uint8_t turnsOut = 0;

  // Aplica un estado. Devuelve false si no prendio: ya tenia otro, o es
  // inmune por tipo (el fuego no se quema, el electrico no se paraliza...).
  // `terreno` porque el campo electrico no deja dormir y el de niebla no
  // deja NINGUN estado, pero solo a quien pisa el suelo. Por omision TR_NONE
  // para que los tests y el comando de depuracion no tengan que saberlo.
  // `cielo` lo pide Leaf Guard, que al sol no deja entrar ningun
  // estado. Como el terreno: lo sabe el combate, no el combatiente.
  // `ruido` = hay un Alboroto en marcha, y con ese escandalo no se duerme
  // nadie. Por omision false: quien no lo sepa, no estorba.
  // `corrosivo` = el que lo pone tiene Corrosion, asi que el veneno
  // entra hasta en los de acero y veneno. Como el suelo y el cielo:
  // es de FUERA, no del que lo recibe.
  bool applyStatus(uint8_t st, uint8_t terreno = TR_NONE,
                   uint8_t cielo = WE_NONE, bool ruido = false,
                   bool corrosivo = false);
  // Sube o baja un stage y lo deja entre -6 y +6. false si ya estaba al tope.
  // `deFuera` = se lo esta bajando el RIVAL. Lo pide Niebla, que
  // protege de eso y no de lo que uno se baja a si mismo.
  bool applyStage(uint8_t which, int8_t delta, bool deFuera = false);
  // Que le impide moverse este turno, y consume lo que haya que consumir.
  uint8_t blockedThisTurn();
  // La MISMA pregunta, pero que se puede hacer dos veces.
  //
  // blockedThisTurn() tiene efectos: despierta al que ya cumplio su sueno,
  // gasta el aturdimiento, descuenta la confusion, alterna la pereza. O sea
  // que no se puede preguntar "se mueve?" para pintar y volver a preguntarlo
  // para resolver —— la segunda respuesta seria otra.
  //
  // Y hacia falta preguntarlo ANTES: la pantalla anunciaba "X uso Arañazo!",
  // pintaba el golpe, y SOLO DESPUES decia "X esta paralizado". Al reves de
  // como pasa. Esto tira el dado una vez y guarda la respuesta; useMove() se
  // encuentra la que ya habia y la consume.
  uint8_t peekBlock();
  uint8_t blockCache = 0xFF;   // 0xFF = todavia sin preguntar este turno
  // Dano de final de turno por quemadura o veneno. Devuelve lo que se llevo.
  uint16_t endOfTurnDamage();

  void init(int16_t species, uint16_t lvl, bool isShiny = false,
            uint8_t formNum = 0);
  // BILIS le anula la habilidad lo que queda de combate. Va aqui, en el
  // accesor, y no en cada uno de los sitios que preguntan por la
  // habilidad: son decenas y olvidarse de uno seria una habilidad que se
  // anula a medias.
  bool abilOff = false;
  // atajo: el efecto de su habilidad, o AB_NONE
  uint8_t abil() const { return abilOff ? (uint8_t)AB_NONE : abilAt(ability).fx; }
  uint8_t abilArg() const { return abilOff ? (uint8_t)0 : abilAt(ability).arg; }

  // Transform de Ditto y Mew: copia especie aparente, stats y golpes del rival,
  // pero NO los PS, igual que en los juegos. Por eso existe asDex separado de
  // dex: al acabar el combate el bicho sigue siendo el que era.
  void transformInto(const Combatant &other);

  uint16_t atkStat() const;
  // El ataque SIN el recorte de la quemadura. Lo pide la formula de dano,
  // que tiene que decidir de QUIEN es la quemadura: Juego Sucio pega con
  // el ataque del rival y la quemadura sigue siendo la del que pega.
  uint16_t atkStatRaw() const;
  uint16_t defStat() const;
  uint16_t spAtkStat() const;
  uint16_t spDefStat() const;
  uint16_t speStat() const;
  // MEGAEVOLUCION. Mientras dura, los tipos, los stats base, la
  // habilidad y el sprite son los de la forma mega y no los de la
  // especie. Se lleva como `asDex`: una variable y los accesores miran,
  // en vez de tocar los treinta sitios que leen DEX_TBL.
  //
  // Es de COMBATE y no se guarda: al acabar, el bicho vuelve a ser el de
  // siempre. En el guardado solo queda la piedra que llevaba puesta.
  uint8_t megaIdx = 0xFF;      // fila de MEGA_TBL, 0xFF = sin megaevolucionar
  bool isMega() const { return megaIdx < MEGA_COUNT; }
  // La ficha de donde salen tipos y stats: la mega si la hay.
  // uint16_t porque los PS base de una mega pueden pasar de 255 (el
  // Mewtwo Armadura del Anil tiene 400).
  uint16_t base(uint8_t cual) const;  // cual = 0..5, el orden de BASE_STATS
  // El stat `cual` (1..5) ya calculado y SIN escalones. Devuelve el
  // copiado si esta transformado; si no, lo suyo.
  uint16_t statBruto(uint8_t cual) const;
  bool megaEvolve(uint8_t fila);      // a la forma `fila` de MEGA_TBL
  // La fila de su forma regional, o -1. Mira `asDex` y no `dex` para que
  // un Ditto transformado en un Meowth de Galar salga de Galar.
  int16_t formRow_() const {
    return valid() ? formRow(asDex, form) : -1;
  }
  // La especie que se VE y se NOMBRA: la de la Ilusion mientras dure.
  // Solo para la pantalla; tipos, stats y golpes siguen siendo los suyos.
  int16_t pareceDex() const { return (illusion && ilusDex > 0) ? ilusDex : asDex; }
  uint8_t pareceForm() const { return (illusion && ilusDex > 0) ? ilusForm : form; }
  // TIPO PISADO: 0 = el que le toque por especie/forma/mega. Lo pone
  // Mutatipo antes de soltar su golpe. Va en los dos accesores y no en
  // los sitios que preguntan por el tipo, que son muchisimos: la
  // efectividad, el STAB, el clima, el terreno, las inmunidades...
  uint8_t typeSet = 0;
  uint8_t typeSet2 = 0;   // el segundo se BORRA al pisar: queda mono
  // EL TIPO QUEMADO. Ultimo Fuego y Electrochoque se gastan su propio
  // tipo para pegar una vez muy fuerte: despues, el bicho deja de ser de
  // ese tipo —— pierde el STAB y tambien sus debilidades. Es la mitad que
  // hace del golpe una DECISION y no un Lanzallamas de 130.
  //
  // Se filtra en los dos accesores y no en un sitio suelto porque el tipo
  // se mira en diecisiete lugares del combate; arreglarlo en uno solo
  // seria arreglarlo en uno solo.
  uint8_t sinQuemado_(uint8_t t) const {
    return (tipoPerdido && t == tipoPerdido) ? (uint8_t)T_NONE : t;
  }
  // El tipo TAL CUAL le toca por especie, forma, mega o Mutatipo, ya
  // sin lo que haya quemado Ultimo Fuego. Lo de Respiro va aparte, en
  // type1/type2, porque para resolverlo hay que mirar los DOS.
  uint8_t tipoCrudo_(bool primero) const {
    if (typeSet) return sinQuemado_(primero ? typeSet : typeSet2);
    if (isMega())
      return sinQuemado_(primero ? MEGA_TBL[megaIdx].type1
                                 : MEGA_TBL[megaIdx].type2);
    if (bform >= 0)
      return sinQuemado_(primero ? BFORM_TBL[bform].type1
                                 : BFORM_TBL[bform].type2);
    int16_t f = formRow_();
    if (f >= 0) return sinQuemado_(primero ? FORM_TBL[f].type1
                                           : FORM_TBL[f].type2);
    const DexEntry &d = DEX_TBL[valid() ? asDex : 0];
    return sinQuemado_(primero ? d.type1 : d.type2);
  }
  // RESPIRO: mientras dure el turno, el que lo uso DEJA DE SER VOLADOR.
  // Si no le queda ningun tipo (un volador puro, como Tornadus) pasa a
  // NORMAL; asi lo hace `pbTypes` del Anil: `ret.delete(:FLYING)` y
  // `ret.push(:NORMAL) if ret.length == 0`.
  //
  // Lo que eso significa va en los dos sentidos, y conviene no
  // confundirlos —— yo lo hice, y me lo dijo un teste en rojo:
  //
  //   A FAVOR  roca, hielo y electrico dejan de ser supereficaces, que
  //            es por lo que un Zapdos se posa.
  //   EN CONTRA  pierde la INMUNIDAD A TIERRA. Posado, un Terremoto le
  //            entra; volando no le llega. Ese es el precio.
  //
  // Aqui no estaba ninguna de las dos: Respiro era media cura a secas.
  uint8_t type1() const {
    uint8_t a = tipoCrudo_(true);
    if (!roosted || a != T_FLYING) return a;
    uint8_t b = tipoCrudo_(false);
    return (b == T_NONE || b == T_FLYING) ? (uint8_t)T_NORMAL : b;
  }
  uint8_t type2() const {
    if (roosted && (tipoCrudo_(true) == T_FLYING ||
                    tipoCrudo_(false) == T_FLYING))
      return T_NONE;      // el que quedaba ya sale como primero
    return tipoCrudo_(false);
  }
  bool valid() const { return asDex >= 1 && asDex <= DEX_COUNT; }
  bool fainted() const { return hp == 0; }
  // Pisa el suelo? Es lo unico que decide a quien alcanza el terreno. Vuela
  // quien es de tipo volador y quien levita (Levitate es AB_IMMUNE_TYPE
  // contra tierra, asi que se reconoce por ahi y no por un numero suelto).
  bool grounded() const {
    // LEVITON: mientras flote, no pisa el suelo.
    if (levitating) return false;
    // Antiaereo manda por encima de todo: te han bajado al suelo y
    // ahi te quedas, seas volador o levites.
    if (grounded2) return true;
    if (type1() == T_FLYING || type2() == T_FLYING) return false;
    const AbilityEntry &a = ABIL_TBL[ability < ABIL_COUNT ? ability : 0];
    return !(a.fx == AB_IMMUNE_TYPE && a.arg == T_GROUND);
  }
  // PATALETA mira si fallo el golpe de la RONDA ANTERIOR, no el ultimo:
  // el Anil copia lastMoveFailed -> lastRoundMoveFailed al cerrar cada ronda
  // (160_Battle_EndOfRoundPhase.rb:817). Va el ultimo, en el byte de cola.
  bool lastRoundFailed = false;
  // se queda sin nada que usar: entonces forcejea, como en el juego
  bool anyUsable() const;
};

// Resultado de un golpe, para que la pantalla sepa que contar y que animar.
#define HIT_MAX 10        // impactos como mucho (Bomba Demografica)
struct HitResult {
  uint16_t damage = 0;    // total, ya sumados los impactos de un multigolpe
  uint16_t effect = 100;  // x100: 0, 25, 50, 100, 200, 400
  bool crit = false;
  bool missed = false;
  bool struggled = false;
  uint8_t hits = 1;       // >1 en los multigolpe, para poder contarlo
  // Lo que hizo CADA impacto, para que la pantalla pueda bajar la barra
  // uno a uno. Bomba Demografica son hasta diez.
  uint16_t hitDmg[HIT_MAX] = { 0 };
  uint16_t healed = 0;    // lo que se cura el que pega (Giga Drain y compania)
  uint16_t recoil = 0;    // lo que se lleva el que pega (Double-Edge, forcejeo)
  bool crashed = false;   // ese `recoil` es de ESTRELLARSE (Patada Salto Alta)
  // Objetos que se movieron con el golpe. Los pinta la pantalla: un
  // objeto que desaparece sin que nadie lo diga parece un fallo.
  uint8_t itemLost = IT_NONE;    // tirado al suelo (Desarme, Lanzamiento)
  uint8_t itemStolen = IT_NONE;  // robado (Ladron, Antojo)
  bool itemToBag = false;        // ...a un SALVAJE: va a la mochila, no a la mano
  bool itemSwap = false;         // Trapicheo: se los cambiaron
  bool itemLostSelf = false;     // `itemLost` era del que PEGA (Lanzamiento)
  uint8_t blocked = 0;    // BLK_*: no se movio, y por que
  uint8_t gotStatus = 0;  // ST_* que le acaba de entrar al rival, si prendio
  bool statusSelf = false; // ...entrou no que PEGA (Poison Point do outro, Rest,
                           // Gulp Missile): a frase tem de dizer o nome DELE
  uint8_t stageWhich = 0xFF;  // stage tocado, SG_*
  int8_t stageDelta = 0;      // en que sentido
  bool stageSelf = false;     // el stage es del que PEGA, no del que recibe
  uint16_t endDamage = 0;     // lo que se llevo el veneno o la quemadura
  uint8_t endWho = 0;         // a quien
  bool charging = false;      // este turno solo cargo (Solar Beam), no pego
  bool cargaRapida = false;   // cargo Y pego en el mismo turno (Rayo Solar al sol)
  uint8_t magnitud = 0;       // Magnitud: el nivel que salio (4..10), 0 = otro golpe
  bool selfConfused = false;  // se le acabo la furia y se mareo solo (Outrage)
  bool trapped = false;       // el rival acaba de quedarse atrapado (Bind)
  uint8_t weatherSet = WE_NONE;  // acaba de cambiar el cielo
  uint8_t terrainSet = TR_NONE;  // ...o el suelo
  bool noDamage = false;      // golpe de apoyo: no pega, solo hace su efecto
  bool blockedByProtect = false;  // el rival se protegio y no entro nada
  bool failed = false;        // el golpe no se pudo hacer (Protect encadenado...)
  uint8_t sideFx = 0;         // FX_* de pantalla/estado que se acaba de poner
  uint16_t drained = 0;       // lo que chupo la semilla al cerrar el turno
  uint8_t itemUsed = IT_NONE; // se gasto un objeto (baya, banda)
  bool fickleBig = false;     // Rayo Umbrio salio en modo serio
  bool presentHeal = false;   // el Regalo le curo al rival en vez de pegarle
  uint16_t healedTarget = 0;  // ...y cuanto
  bool curedTarget = false;   // Estimulo le quito la paralisis al rival
  uint8_t lostType = 0;       // el que pega se quemo ese tipo suyo
  bool stolenStages = false;  // Robo Espectral le quito las subidas
  bool heldOn = false;        // el objeto le salvo de caer (Focus Sash)
  bool subHit = false;        // el golpe se lo llevo el Sustituto
  bool subBroke = false;      // ...y lo rompio
  bool beakBurn = false;      // el que pego se quemo con el Pico Canon
  bool coins = false;         // Dia de Pago: monedas por el suelo
  bool bideStore = false;     // Venganza: este turno solo aguanta
  bool bideRelease = false;   // ...y este suelta
  // Que habilidad acaba de saltar, y de quien. Sin esto el jugador ve que el
  // ataque le baja solo y nunca se entera de por que: la del rival no sale en
  // ninguna pantalla, se aprende viendola actuar.
  uint16_t abilFired = 0;
  bool abilOfDef = false;     // true = la del que RECIBE el golpe
};

struct Battle {
  Combatant side[2];
  uint8_t turn = 0;        // numero de turno, para el log y para los tests
  // Lo que Frisk le vio al de enfrente (IT_NONE = no vio nada). Lo
  // pinta la pantalla; aqui solo se apunta.
  uint8_t frisked[2] = { IT_NONE, IT_NONE };
  // PREVISION: el golpe mas fuerte que le vio al rival al entrar, o
  // 0xFFFF. Es informacion, como Frisk, y aqui la informacion vale:
  // saber que trae un Terremoto decide si te quedas o te vas.
  uint16_t forewarned[2] = { 0xFFFF, 0xFFFF };
  // Y si Anticipation le hizo temblar al entrar.
  bool shuddered[2] = { false, false };
  bool temblor(uint8_t quien) const;   // ¿hay motivo para temblar?

  // Quien ya se ha movido en ESTE turno. Lo pide Analytic, que pega
  // mas si va el ultimo, y es informacion que el motor no tenia: cada
  // golpe se resuelve por su cuenta y nadie llevaba la cuenta.
  bool movedThisTurn[2] = { false, false };
  // TRAMPAS DE ENTRADA, por bando: pican al que ENTRA en ese lado. No
  // existian porque sin banquillo nadie entra nunca; con relevos son lo
  // que convierte un cambio en una decision y no en algo gratis.
  // Un golpe las pone en el lado DEL RIVAL (ver FX_ROCKS y compania), y
  // Giro Rapido limpia las del suyo.
  uint8_t rocks[2] = { 0, 0 };     // piden un octavo al entrar
  uint8_t spikes[2] = { 0, 0 };    // 0..3 capas, cada una pica mas
  uint8_t tspikes[2] = { 0, 0 };   // 0..2: una envenena, dos entoxican
  uint8_t web[2] = { 0, 0 };       // le baja la velocidad al entrar
  // VIENTO AFIN: su bando al doble de velocidad, por turnos.
  uint8_t tailwind[2] = { 0, 0 };
  // Alguna trampa le cobro al ultimo que entro, para que la pantalla lo
  // pueda contar. Lo pone sendOut y lo lee quien maneja el banquillo.
  uint8_t hazardHit = 0;
  // Los dos "deportes": mientras duran, el fuego (o lo electrico) pega
  // la mitad. Son del CAMPO, no de un bando: alcanzan a los dos.
  uint8_t waterSport = 0, mudSport = 0;
  // RELEVO: lo que el que se va le deja al que entra. `baton` dice que
  // hay un traspaso pendiente en ese lado y sendOut lo aplica.
  // BATERIA y PUNTO FUERTE: lo que el que SE RETIRA deja preparado
  // para el siguiente. Mismo patron que `baton`: se guarda en el
  // bando y sendOut lo pone en el que entra.
  // CONJURO: turnos que a ese bando no le entran criticos.
  uint8_t luckyChant[2] = { 0, 0 };
  // ANTICIPO: ESTE turno, a ese bando no le entran los golpes de
  // prioridad. Dura un turno, asi que se apaga al cerrarlo.
  bool quickGuard[2] = { false, false };
  uint8_t relayBoost[2] = { 0, 0 };
  // ...y si el que se retira deja al siguiente CURADO (Cura).
  bool relayCure[2] = { false, false };
  bool baton[2] = { false, false };
  int8_t batonStage[2][SG_COUNT] = { { 0 }, { 0 } };
  // Turnos que queda de ALBOROTO. Es del combate y no de un bando: el
  // ruido lo oyen los dos, y mientras corre nadie puede dormir.
  uint8_t uproar = 0;
  // PREMONICION: el golpe que ya salio y aun no ha llegado. Se guarda por
  // LADO (quien lo lanzo) con lo que va a pegar y cuanto le queda; cuando
  // el reloj llega a cero, pega al otro lado aunque el que lo lanzo ya no
  // este en la pista. Eso ultimo es justo la gracia del golpe.
  uint8_t futureTurns[2] = { 0, 0 };
  // Con QUE golpe (Premonicion o Deseo Oculto). El dano NO se guarda: se
  // calcula al caer, que es lo que hace el Anil —— contra quien este
  // entonces (un siniestro que entro despues es inmune) y con los stats
  // que el que lo lanzo tenga en ese momento.
  uint16_t futureMv[2] = { 0xFFFF, 0xFFFF };
  // Si el que lo lanzo ya no esta en pista: su foto al irse (sin subidas,
  // como el `pbInitDummyPokemon` del Anil), y si se fue DEBILITADO, en cuyo
  // caso el golpe no llega ("return if !moveUser # User is fainted").
  bool futureFuera[2] = { false, false };
  bool futureCaido[2] = { false, false };
  Combatant futureDe[2];
  // El golpe del futuro que ACABA de caer, para que la pantalla lo pueda
  // contar: un bicho que pierde PS sin que nadie le haya pegado parece un
  // fallo. 0xFF = este turno no cayo ninguno.
  uint8_t futureHitOn = 0xFF;
  uint16_t futureHitDmg = 0;
  // ESPACIO RARO: turnos en que, dentro de cada escalon de prioridad, el
  // mas LENTO va primero. Es del campo entero; usarlo con el puesto lo
  // quita (en los juegos tambien).
  uint8_t trickRoom = 0;
  // DESEO: se cumple al cerrar el turno SIGUIENTE y cura al que este en
  // ese lado entonces, aunque ya no sea quien lo pidio. La cantidad es la
  // mitad de los PS maximos del que lo PIDIO, como en los juegos.
  uint8_t wishTurns[2] = { 0, 0 };
  uint16_t wishAmt[2] = { 0, 0 };
  // Lo que la pantalla tiene que contar al cerrar el turno, igual que
  // futureHitOn: el Deseo que se acaba de cumplir (0xFF = ninguno) y la
  // cuenta del Canto Mortal que acaba de bajar (0xFF = ninguna).
  uint8_t wishHealOn = 0xFF;
  uint16_t wishHealAmt = 0;
  uint8_t perishTick = 0xFF;
  uint8_t encoreFin = 0xFF;   // a quien se le acaba de pasar el Encore
  uint8_t yawnDurmio = 0xFF;  // a quien acaba de dormir el Bostezo
  // PALIZA (Beat Up): el ataque base de los del equipo que pueden pegar
  // (en pie y sin estado). El combate no conoce el banquillo, asi que se
  // lo cuenta la pantalla con ponEquipo; sin eso, solo el que esta.
  uint8_t equipoAtk[2][6] = { { 0 }, { 0 } };
  uint8_t equipoN[2] = { 0, 0 };
  void ponEquipo(uint8_t lado, const uint8_t *atk, uint8_t n);
  // El principio de la ronda, con los golpes ya elegidos: Pico Canon y
  // Coparazon se preparan AQUI, antes de que nadie se mueva.
  void inicioDeRonda(uint8_t slotJugador, uint8_t slotRival);
  // La ronda en la que el jugador NO usa golpe (objeto, bola, relevo, huida
  // que no sale): el rival elige igual, y para Pataleta lo del jugador
  // "cuenta como un golpe que salio bien" (159_Battle_AttackPhase.rb).
  void inicioDeRondaRival(uint8_t slotRival);
  // A batalha entre placas: QUALQUER lado pode nao usar golpe (troca,
  // objeto). `usa` = esse lado usa golpe nesta ronda; quem nao usa fica
  // como o jogador em inicioDeRondaRival. Com os dois usando e igual a
  // inicioDeRonda; so o rival usando, igual a inicioDeRondaRival.
  void inicioDeRondaDos(bool usa0, uint8_t slot0, bool usa1, uint8_t slot1);
  // El ENTORNO del combate, el `environment` del Anil: lo pide Poder
  // Secreto. Lo pone la pantalla a partir del escenario (bgEntorno).
  uint8_t entorno = EN_NONE;

  // HUIR. De un entrenador no se huye; de un salvaje si, y hace falta:
  // un combate que no quieres pelear te estropea los EVs del bicho
  // (los EVs salen de a QUIEN tumbas) y te gasta PP y PS por nada.
  //
  // `isTrainer` lo pone quien arranca el combate. `fleeTries` sube con
  // cada intento y lo hace mas facil, como en los juegos: insistir
  // acaba funcionando, pero cada intento cuesta un turno.
  // Ida y Vuelta acaba de pegar y el jugador se va. El motor NO saca a
  // nadie: quien maneja el banquillo es la pantalla, que es la que
  // sabe que hay en el equipo. Aqui solo se avisa.
  bool wantsSwitch = false;

  bool isTrainer = false;
  uint8_t fleeTries = 0;
  bool fled = false;       // se acabo porque te escapaste, no por un KO
  bool canFlee() const;    // ¿se puede intentar siquiera?
  bool tryFlee();          // lo intenta; true si se escapa

  bool over = false;
  uint8_t winner = 0xFF;   // BT_PLAYER, BT_FOE o 0xFF si sigue
  uint8_t weather = WE_NONE;
  uint8_t weatherTurns = 0;
  uint8_t terrain = TR_NONE;
  uint8_t terrainTurns = 0;
  // Las que saltan al SALIR (Intimidate, Drought). No hay HitResult donde
  // meterlas, asi que se dejan aqui para que la pantalla las cuente en la
  // presentacion. 0 = ninguna.
  // Una megaevolucion por lado y por combate, como en los juegos. No es
  // un adorno: sin el limite, el jugador megaevoluciona siempre y deja
  // de ser una decision.
  bool megaUsed[2] = { false, false };
  // El par FUSION. Llama Fusion pega el doble si Rayo Fusion salio antes en
  // este turno, y al reves. Vive en el combate y no en el combatiente
  // porque cuenta igual lo haya usado quien lo haya usado —— es el unico
  // par de golpes del juego que se ayudan entre bandos.
  // 0 nada, 1 salio el rayo, 2 salio la llama. Se borra al cerrar el turno.
  uint8_t lastFusion = 0;
  // PUNOS PLASMA: durante el resto del turno, los golpes NORMALES pasan a
  // electricos. Es del campo y no del bicho —— le pasa al rival tambien ——
  // y dura hasta que se cierra el turno.
  bool normalEsElec = false;
  // Si ESE lado puede megaevolucionar ahora mismo: tiene forma para la
  // piedra que lleva puesta y no la ha usado todavia.
  int16_t canMega(uint8_t quien) const;
  // Lo hace. Deja en `megaFired` la fila, para que la pantalla lo cuente.
  bool doMega(uint8_t quien);
  int16_t megaFired = -1;   // -1 = este turno no megaevoluciono nadie

  uint16_t startAbil[2] = { 0, 0 };
  // Lo que la habilidad hizo al cerrar el turno (Shed Skin, Speed Boost...).
  uint16_t endAbil = 0;
  // En un gimnasio del primer escalon no se desobedece: perder la insignia
  // por una tirada de dados seria rabia, no dificultad.
  // REPRESALIA: cuantos cierres de turno le quedan a la venganza de ese
  // LADO. Del lado y no del bicho, porque el que se cobra la venganza es
  // justo el que acaba de entrar —— y el relevo copia la ficha guardada,
  // que borraria una marca puesta en el Combatant.
  uint8_t vengefulSide[2] = { 0, 0 };
  // DESEO CURACION y DANZA LUNAR: el que ENTRE por ese lado llega entero.
  // 0 nada, 1 Deseo Curacion, 2 Danza Lunar (que ademas repone PP).
  uint8_t deseoCura[2] = { 0, 0 };
  // Lo que el cierre de turno CURO, y por que. El desgaste ya se contaba
  // (endTurn devuelve el dano) y las habilidades tambien (endAbil), pero
  // la cura no la contaba nadie: los Restos curaban en silencio y desde
  // fuera parecian no hacer nada.
  uint16_t endHeal = 0;
  uint8_t endHealItem = 0;   // el objeto que curo, 0 si fue otra cosa
  // LAS PANTALLAS, que son del BANDO y no del bicho: duran cinco turnos
  // este quien este en pista. Vivian en el Combatant, y como sendOut
  // copia la ficha guardada encima, se perdian en cada relevo.
  // Los campos del Combatant siguen existiendo como ESPEJO (computeHit
  // es una funcion libre y no ve esto), igual que `chanted`.
  enum : uint8_t { PA_REFLECT = 0, PA_LIGHT, PA_SAFE, PA_MIST, PA_COUNT };
  uint8_t pantalla[2][PA_COUNT] = { { 0, 0, 0, 0 }, { 0, 0, 0, 0 } };
  // Copia los contadores del lado al bicho que esta en pista.
  void espejaPantallas(uint8_t quien) {
    Combatant &c = side[quien];
    c.reflect = pantalla[quien][PA_REFLECT];
    c.lightScreen = pantalla[quien][PA_LIGHT];
    c.safeguard = pantalla[quien][PA_SAFE];
    c.mistTurns = pantalla[quien][PA_MIST];
  }
  bool strictOrders = false;
  // Le queda banquillo al jugador? Lo mantiene la pantalla, que es la unica
  // que sabe del equipo. Si es true, caer NO acaba el combate: obliga a
  // sacar a otro. El bando rival no lo usa — sus relevos los lleva
  // btNextFoe() mirando `over`, y eso se queda como estaba.
  bool playerReserves = false;
  // O MESMO para o rival, e so a batalha entre placas o liga: la o outro
  // lado e um time de verdade, e cair nao acaba nada enquanto houver
  // banco. Desligado (o jogo de sempre), tudo fica como era.
  bool foeReserves = false;
  bool temReservas(uint8_t lado) const {
    return lado == BT_PLAYER ? playerReserves
         : lado == BT_FOE ? foeReserves : false;
  }
  // Os pedidos de relevo POR LADO, para quem tem banco. `wantsSwitch`
  // continua a ser so o do jogador, como sempre foi.
  //   querTrocar: o dono escolhe quem entra (Ida e Volta, Relevo, Huida).
  //   arrastado:  entra um SORTEADO (Rugido, Cola Dragao), como no Anil
  //               (pbGetReplacementPokemonIndex(idx, true)).
  // O motor nao saca ninguem: quem tem o banco le, saca e apaga.
  bool querTrocar[2] = { false, false };
  bool arrastado[2] = { false, false };

  // Alguien se ha caido. Pasa por aqui TODO el juego (son ocho sitios), para
  // que la regla de "solo se acaba si no queda nadie" viva en un unico lugar.
  void knockOut(uint8_t quien);
  // Relevo: pone a `c` en la pista. Borra lo volatil (stages, confusion,
  // agarre, furia, carga) y dispara las habilidades de salida.
  // `porGusto` = el relevo es voluntario (no se releva porque cayo el
  // anterior). Solo cambia una cosa: Persecucion.
  void sendOut(uint8_t quien, const Combatant &c, bool porGusto = false);
  // Lo que pasa al pisar la pista, igual al empezar que al relevar.
  // Lo llaman start() y sendOut(): una habilidad de entrada que solo
  // mirase start() no se disparara nunca en un mega, que entra de
  // relevo. Lo aprendi con el Velo Vapor.
  void alEntrar(uint8_t quien);
  // Lo que hace la habilidad de uno NADA MAS estar en pista: el suelo,
  // el cielo, Intimidacion, Iluminacion. Lo llaman el arranque del
  // combate, el relevo Y la megaevolucion —— esta ultima porque la mega
  // trae habilidad nueva y en los juegos entra funcionando.
  void habilidadDeEntrada(uint8_t quien);
  // Vuelve a calcular el tipo de quien no lo saca de su especie
  // (Veleta del tiempo, Multitipo). Lo llaman la entrada y el cierre
  // de turno, porque el cielo cambia en mitad del combate.
  void syncTipo(uint8_t quien);
  // Leva 70: las formas de combate (Castform, Aegislash, Daruma...).
  // momento 0 = al entrar, 1 = al cerrar la ronda, 2 = cambio el cielo.
  void syncForma(uint8_t quien, uint8_t momento);
  bool ponForma(uint8_t quien, uint8_t forma);
  // Regenerator: cura un tercio al que se RETIRA. Se llama ANTES de
  // guardarlo en el banquillo, o la cura no le llega.
  uint16_t regenOnSwitch(uint8_t quien);

  void start(const Combatant &mine, const Combatant &foe);
  // true si `a` se mueve antes que `b` en este turno
  // ¿Se mueve `a` antes que el otro? Los dos huecos son los golpes que
  // van a salir, y hacen falta SOLO para Prankster, que adelanta los de
  // apoyo. Quien no los sepa (el arranque del combate, donde aun no hay
  // golpe elegido) pasa 0xFF y la velocidad decide como siempre.
  bool movesFirst(uint8_t a, uint8_t slotA = 0xFF,
                  uint8_t slotB = 0xFF) const;
  // EL HUECO QUE VA A SALIR DE VERDAD. Se pide un golpe y pueden salir
  // otro cuatro cosas distintas: Outrage ata (lockTurns), Encore obliga
  // a repetir, la Choice encadena y un golpe a medio cargar se suelta.
  // Esto vivia DOS veces —— en useMove y en btLaunch —— con listas
  // distintas, y por eso la caja anunciaba un golpe y salia otro. Quien
  // necesite saber que golpe sale (la caja, la animacion, el orden del
  // turno) pregunta aqui y no lo deduce.
  uint8_t realSlot(uint8_t quien, uint8_t slot) const;
  // ¿ESE HUECO PUEDE SALIR? Las seis cosas que lo impiden sin gastar el
  // golpe: sin PP, Anular, Mofa, Golpe Mordaza, Tormento, Martillo Giga
  // y el Sellar del rival. Lo preguntan la IA del rival (que antes solo
  // miraba los PP y quemaba turnos en golpes que no podian salir) y la
  // tarjeta del jugador (que solo pintaba el de Anular).
  //
  // useMove tiene su propia lista, porque alli cada puerta hace cosas
  // distintas y el orden entre ellas importa. Que las dos no se separen
  // lo vigila el test `los_bloqueos_que_ve_la_IA_son_los_de_verdad`.
  bool slotBloqueado(uint8_t quien, uint8_t slot) const;
  // No hay golpe que elegir: con Encore, el suyo esta cerrado; sin Encore,
  // lo estan todos. En el Anil eso es Forcejeo, no un turno perdido.
  bool debeForcejear(uint8_t quien) const;
  // ¿Ese hueco lleva un golpe de apoyo y el bicho tiene Prankster?
  // EL ESCALON DE PRIORIDAD del golpe que ese lado va a sacar: el del PBS
  // mas lo que le sumen la habilidad y el terreno. Es lo PRIMERO que decide
  // el turno, por delante de la velocidad y de la Garra Rapida.
  //
  // Con slot 0xFF (el valor por defecto de movesFirst, "no se que golpe
  // es") devuelve 0: quien no dice el golpe no puede pedir prioridad.
  int8_t speedTier(uint8_t quien, uint8_t slot) const;
  bool prankster(uint8_t quien, uint8_t slot) const;
  // ...y el espejo: Micofuerza atrasa los de apoyo en vez de adelantarlos.
  bool micelio(uint8_t quien, uint8_t slot) const;
  // ¿Puede ese lado relevar POR GUSTO? El que cayo sale siempre; al que
  // esta agarrado o atrapado no se le deja cambiar, que es justo de lo
  // que viven Bind, Mal de Ojo, Presa Maxilar y Arraigo.
  bool canSwitchOut(uint8_t quien) const;
  // Los deportes en marcha, como mascara para computeHit.
  uint8_t sportMask() const {
    return (uint8_t)((waterSport ? SP_WATER : 0) | (mudSport ? SP_MUD : 0));
  }
  // ¿Ese hueco lleva un golpe al que SU terreno le adelanta el turno?
  // (Fitoimpulso con hierba, Bruma Explosiva con niebla)
  bool grassGlide(uint8_t quien, uint8_t slot) const;
  // aplica un golpe de `from` contra el otro; slot es el indice en moves[]
  // `forzado` = usa ESE golpe y no el del hueco, sin gastarle PP. Lo
  // pide Bailarin, que copia un baile que puede no saber. Por omision
  // 0xFFFF: quien no lo use, no se entera.
  HitResult useMove(uint8_t from, uint8_t slot, uint16_t forzado = 0xFFFF);
  HitResult useMoveDentro(uint8_t from, uint8_t slot, uint16_t forzado);

  // Lo que el Anil hace en pbCancelMoves: un golpe que NO entro no
  // deja a nadie atado ni le guarda la racha. Es una funcion y no
  // un bloque porque hay dos salidas distintas que la necesitan ——
  // fallar la punteria sale por un `return` mucho antes del final
  // de useMove, y por ahi se colaban Rodar, Enfado y Corte Furia.
  void rompeCadena(uint8_t from, uint16_t mv, HitResult &r);
  // Para que no se baile un baile copiado: sin esto, dos Bailarines
  // frente a frente se copiarian el uno al otro para siempre.
  bool dancing = false;
  // El baile que se acaba de copiar, para que la pantalla lo cuente.
  uint16_t danced = 0xFFFF;
  // Le das la orden y te dice que va a pasar de verdad. Se llama ANTES de
  // anunciar el golpe y SOLO una vez por turno, porque tira el dado: si la
  // pantalla anunciara con una tirada y useMove() resolviera con otra, la
  // caja de texto mentiria. El slot que devuelve es el que hay que pasarle
  // luego a useMove().
  Order giveOrder(uint8_t from, uint8_t slot);
  static uint8_t disobeyChance(uint8_t bond);
  // la IA del salvaje: el golpe que mas dano esperado hace, con lo que le queda
  uint8_t pickFoeMove() const;
  // Cierra el turno: quemadura y veneno cobran aqui. Se llama UNA vez, cuando
  // los dos ya se han movido. Devuelve lo que se llevo `who`.
  uint16_t endTurn(uint8_t who);
};

// Dano de un golpe concreto, sin aplicarlo. Publica para que la pantalla pueda
// enseñar "muy eficaz" antes de pegar si algun dia se quiere. El cielo entra
// como parametro porque cambia la potencia (al sol el fuego pega mas).
// `ultimo` = el que pega va DESPUES del otro en este turno. Solo lo
// mira Analytic; quien no lo sepa lo deja en false y no cambia nada.
// `sport` = los dos "deportes" en marcha, como mascara: SP_WATER baja el
// fuego a la mitad y SP_MUD lo electrico. Va por parametro, como el cielo
// y el suelo, porque es del combate y no del combatiente. Por omision 0:
// quien no lo sepa no estorba.
HitResult computeHit(const Combatant &att, const Combatant &def, uint16_t mv,
                     uint8_t weather = WE_NONE, uint8_t terrain = TR_NONE,
                     bool ultimo = false, uint8_t sport = 0,
                     uint8_t fusionPrev = 0, bool plasma = false);

// Probabilidad 0..100 de que la bola aguante. `ballMult` es x10 (una Poke
// Ball es 10, una Super 15, una Ultra 20) y `taps` los toques del jugador
// durante el forcejeo. Ver el bloque CAPT_* de arriba para el por que de
// unos numeros tan generosos.
uint8_t captureChance(const Combatant &foe, uint16_t myLevel,
                      uint8_t ballMult, uint8_t taps);

// Impressao digital do combate, para as duas placas compararem a cada
// turno: se divergirem, e melhor saber no turno em que aconteceu do que
// ver cada placa anunciar um vencedor diferente. Entra o estado do dado,
// que e o alarme mais sensivel (uma tirada a mais numa so placa ja muda).
uint32_t btHash(const Battle &b);
