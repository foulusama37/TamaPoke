#pragma once
#include <Arduino.h>
#include <Preferences.h>

#include "abilities.h"
#include "forms.h"
#include "items.h"
#include "megas.h"   // MEGA_COUNT: la Pokedex guarda las megas vistas
#include "party.h"
#include "troca.h"
#define TROCA_BANCO 0x80
#include "gyms.h"
#include "stones.h"
#include "tms.h"
#include "trainers.h"

// 1 tick = 1 minuto de juego. Baja este valor para probar mas rapido
// (p. ej. 5000UL = las estadisticas caen 12x mas rapido).
#define PET_TICK_MS 60000UL
// Minutos de juego por nivel. Con 60, CHARMANDER evoluciona a las ~16 h
// de juego con cuidado perfecto. Baja a 1 para ver evoluciones al momento.
#define MINUTES_PER_LEVEL 60
#define EAT_ANIM_MS 2500UL
#define HEART_MS 1500UL
// Cuantos pasos cuesta un punto de peso quemado. Treinta son unos
// veinte segundos andando: suficiente para que se note en un paseo
// y no tanto que un meneo en el sofa adelgace al bicho.
#define PASOS_TRAMO 30
#define EVOLVE_ANIM_MS 5200UL              // animacion de evolucion (mas larga = mas epica)
#define CEREMONY_MS 10000UL                // duracion de la despedida en pantalla
#define FAREWELL_AGE_MIN (3UL * 24 * 60)   // se despide a los 3 dias de juego (en forma final)
// Y ADEMAS un dia entero DESDE que llego a su forma final. Sin esto, un
// bicho que ya tenia los tres dias y el vinculo recibia la peticion de
// despedida en el mismo segundo en que evolucionaba: el usuario evoluciono
// un Kadabra y "em seguida veio uma msg para ele querer ser liberto".
// Evolucionar es la recompensa; pedirle que se vaya en ese momento la
// convierte en un castigo. Este dia es para disfrutar la forma final.
#define FAREWELL_SETTLE  (24UL * 60)
#define RUNAWAY_TICKS 60                   // se escapa tras 1 h con TODO a cero
// Abandonado y despierto, el bicho LLAMA (su grito) al primer minuto y
// luego cada 15: cuatro avisos antes de que salga el boton de escapada.
#define ABANDONO_AVISO_TICKS 15
#define LEVEL_CAP 100                      // como en los juegos
// Un pelin de experiencia por cuidarlo bien. Existe para que el lado
// tamagotchi siga empujando la progresion: sin esto, un jugador que solo
// quiere criar al bicho no ve una evolucion en su vida. Es poco a proposito —
// lo que sube de nivel de verdad es pelear.
#define EXP_PER_CARE_DAY 120
// Lo que tarda en volver a estar verde el boton de cazar, desde que se usa
// y desde que se sale del combate.
#define HUNT_COOLDOWN_MS 60000UL
// Leva 72: a cacada da Pokedex. Uma de cada CACA_CHANCE buscas no mato sai
// a especie cacada (o dono escolheu 1 em 4).
#define CACA_CHANCE 4

// EL CENTRO POKEMON.
//
// Hasta ahora los PS y los PP solo se recuperaban durmiendo o con bayas, y
// eso deja al equipo de reserva tirado: el titular duerme y se cura, los
// suplentes no duermen. Quedarse con cinco bichos a cero y sin manera de
// levantarlos no es dificultad, es un callejon.
//
// La enfermera aparece cada tanto Y SOLO SI hace falta: si esta todo lleno
// no se asoma, porque un aviso que no sirve para nada ensena a ignorar los
// avisos. Se queda un rato y se va; se pierde una visita y viene la
// siguiente.
#define PC_EVERY_MS 420000UL   // siete minutos entre visitas
// El Profesor pasa menos a menudo que la enfermera y se queda mas: ella
// resuelve una urgencia (hay algo que curar), el trae un juego, y un juego
// que se te escapa por no mirar la pantalla a tiempo molesta.
#define QUIZ_EVERY_MS (11UL * 60 * 1000)
#define QUIZ_STAY_MS  (3UL * 60 * 1000)
#define PC_STAY_MS  120000UL   // y se queda dos
// Lo que se lleva uno por tumbar a otro: bst * nivel / esto. Cuanto MAS
// pequeno, mas experiencia. Era 35; 16 es algo mas del doble, para
// compensar que aqui se pelea mucho menos que en los juegos.
#define EXP_DIVISOR 16


// LOS LEGENDARIOS, de postre.
//
// Hasta ahora entraban en el sorteo normal al sacar las ocho insignias,
// con peso 2 como cualquier otro: entre mil especies eso es no salir
// nunca. Terminar una region tiene que NOTARSE, y lo que se nota es que
// de pronto puede aparecer un Articuno.
//
// Se tira APARTE del sorteo de siempre, con su porcentaje propio: los de
// BST alto (Mewtwo, Mew) un poco menos que los pajaros, que es lo que uno
// espera. El tope esta para que una region con muchos no se convierta en
// una feria de legendarios.
//
// EM MILESIMOS (2026-11-02). Eram 3%/2% com teto de 15%, e o dono viu
// "uns 5 mews, 6 articunos": vencer a regiao e para LIBERAR, nao para
// encher a hierba. Agora Kanto inteira soma 0,8% —— perto da chance de
// shiny (1/128) —— e um lendario volta a ser um achado.
//
// LEVA 72 (2026-11-21): o teto de 10 por mil era somado POR ORDEM DE DEX e
// parava ao bater: so os 6 primeiros (os 5 de Kanto e o Raikou) tinham
// chance, e os outros 97 lendarios e miticos NUNCA saiam —— e no mato e o
// unico jeito de capturar. Agora e UMA tirada: LEG_PM_TOTAL por mil de
// sair ALGUM, e quem sai se sorteia entre TODOS os liberados, os da
// regiao onde voce esta pesando LEG_PESO_AQUI. O dono escolheu 3%.
#define LEG_PM_TOTAL 30    // por mil, de sair algum lendario numa busca
#define LEG_PESO_AQUI 4    // os da regiao atual pesam isto (os outros, 1)
// e cacando um lendario, uma de cada CACA_CHANCE_LEG buscas e ele
#define CACA_CHANCE_LEG 10

// Lo que cuesta pelear y lo que lo devuelve, todo junto para poder aflojarlo
// sin ir buscando numeros por el codigo. La idea de fondo: el combate gasta,
// y lo unico que repone es cuidar al bicho.
#define BATTLE_ENERGY_COST 8    // energia por combate, se gane como se gane
#define BATTLE_HP_FLOOR_PCT 25  // por debajo de esto se niega a pelear
// La energia es el PORTERO de todo lo que cansa. Es lo que ata el combate al
// ciclo de cuidado: pelear gasta, dormir repone, y sin dormir no se pelea.
// Sin esto se puede moler sin fin y el lado tamagotchi deja de importar.
#define BATTLE_ENERGY_MIN 20    // por debajo, no sale a pelear
#define TRAIN_ENERGY_MIN 30     // entrenar cansa mas que pelear una vez
#define TRAIN_ENERGY_COST 12    // lo que cuesta una sesion de entrenamiento
// Con cuanto vinculo nace. NO es cero: desde que existe la desobediencia,
// nacer a cero significaria plantarse uno de cada cinco turnos desde el
// primer combate, castigando al jugador por algo que todavia no ha hecho.
// Coincide con OBEY_BOND_OK a proposito: recien nacido te hace caso siempre,
// y a partir de ahi lo que hagas lo mueve en un sentido o en el otro.
// ENCUENTROS SALVAJES. De vez en cuando aparece un bicho y sale un "!" en
// pantalla; si no le haces caso, se va. Es lo que convierte el cuidado en un
// ciclo con algo que hacer, sin obligar a nada: ignorarlo no cuesta nada.
//
// El hueco minimo importa mas que la probabilidad: sin el, una racha de
// suerte encadena tres avisos seguidos y deja de ser un acontecimiento.
// Cada cuanto llama la hierba.
//
// Antes era un hueco de 20 minutos y luego un 8% FIJO por minuto: media
// de ~32 minutos y, por ser fijo, una cola larguisima -- podias pasarte
// una hora sin ver nada por mala suerte y sin que el juego se enterara.
// En un cacharro que se mira de reojo eso es no pasar nada nunca.
//
// Ahora la probabilidad SUBE con la espera: cuanto mas llevas sin que
// salga uno, mas facil es que salga. Eso recorta la cola sin volverlo
// una metralleta, que es lo que pasaria subiendo el numero fijo.
//
//   minuto 1 del sorteo: 10%   (0,90 de seguir sin nada)
//   minuto 2:            14%   (0,77)
//   minuto 3:            18%   (0,63)
//   minuto 4:            22%   (0,50)  <- la mitad ya vio uno
//   minuto 8:            38%   (0,10)
//
// Con el hueco de 8 minutos delante, lo tipico son ~12 minutos entre
// avisos, y casi nunca mas de 16. Perder uno NO cuesta nada: el hueco
// solo se arma al ENTRAR al combate (wildClear), asi que si el "!" se
// va solo, el siguiente sorteo empieza al minuto siguiente.
// Segunda bajada, y por un motivo de juego, no de gusto: con ~12
// minutos entre avisos el bicho tardaba una eternidad en subir de nivel
// y en llenar la Pokedex. Un tamagotchi se mira de reojo muchas veces
// al dia; si cada mirada esta vacia, no hay juego.
//
//   minuto 1 del sorteo: 30%   (0,70 de seguir sin nada)
//   minuto 2:            42%   (0,41)
//   minuto 3:            54%   (0,19)  <- la mitad ya vio uno antes
//   minuto 4:            66%   (0,06)
//
// Con el hueco de 2 minutos delante salen ~4 minutos entre avisos, y el
// "!" espera 8, asi que casi siempre hay algo que hacer al mirar.
// Perder uno sigue sin costar nada: el hueco solo se arma al ENTRAR al
// combate (wildClear).
#define WILD_GAP_MIN 2       // minutos de descanso tras PELEAR con uno
#define WILD_CHANCE_PCT 30   // con la que empieza el sorteo
#define WILD_CHANCE_STEP 12  // cuanto sube por minuto de espera
#define WILD_CHANCE_MAX 85   // techo: que siga habiendo algo de sorpresa
#define WILD_TTL_MIN 8       // lo que espera el bicho antes de irse
// RECOGIDA: 1 entre tantos por minuto DESPIERTO. Con 240 sale algo
// cada cuatro horas de vigilia; en una vida de tres dias son un
// punado de objetos, no una tienda.
#define PICKUP_PCT 240
// Un salvaje tambien puede salir SHINY. Uno de cada 128 y no de cada 4096
// como en los juegos: alli te cruzas con cientos al dia y aqui sale uno
// cada media hora larga. Con 128 toca uno cada varios dias de juego, que es
// lo bastante raro para que se cuente y lo bastante posible para pasar.
#define WILD_SHINY_ODDS 128
// Y una parte de los avisos son ENTRENADORES en vez de hierba. No desde el
// principio: con cero insignias todavia estas aprendiendo a pelear, y que
// te salga uno que pega mas fuerte es castigar la primera hora.
// SIN USO desde que el aviso automatico es siempre un entrenador (ver
// Pet::rollWild). Se queda escrito en vez de borrarse: una constante que
// desaparece sin decir por que es la que hace dudar al siguiente que lea
// si el reparto 75/25 se perdio por descuido. Fue a proposito —— los
// salvajes salen por CAZAR.
#define TRAINER_PCT 25       // (sin uso) de cada 100 avisos, cuantos eran
// 0 = desde el primer minuto. Estaba en 1 y por eso en la placa no
// salia ni un entrenador: sin insignia el porton no abria nunca.
#define TRAINER_FROM_BADGE 0
// El techo de BST por insignia ya no vive aqui: es POR REGION y esta en
// `Region::bstCap` (gyms.h), que lo calcula tools/curva_bst.py. La tabla
// unica que habia eran los percentiles de las 151 de Kanto con los stats
// OFICIALES, y dejo de valer por partida doble: el catalogo son 386 y los
// stats son los del Anil, que sube a 140 de ellos.
//
// GYM_COUNT es el mismo 8 que GYM_BADGES de gyms.h; se conserva el nombre
// porque lo usan 20 sitios, pero sale de alli para que no puedan separarse.
#define GYM_COUNT GYM_BADGES
// LA DESPEDIDA deja un HUEVO de la misma especie (idea del usuario, mejor
// que el final seco que habia). Lo que hiciste con el que se va mejora al
// que viene: genes, shiny, un golpe que no le tocaria y lo que llevaba
// encima.
//
// La herencia sube el SUELO de los genes, no el techo. Si subiera el techo,
// la vigesima generacion seria un dios y el juego se acabaria solo; subiendo
// el suelo, un linaje bien cuidado nace bueno pero nunca se sale de la
// escala. 90..110 de serie; 102..110 en el mejor de los casos.
#define LEGACY_FLOOR_BASE 90
#define LEGACY_FLOOR_MAX 102
#define LEGACY_SCORE_MAX 6
// Y hay ATRITO para pedirla: forma final (ya estaba), siete dias (ya estaba)
// y vinculo alto. Sin esto, lo optimo seria despedirse en cuanto se pueda y
// el bicho pasaria a ser material desechable, que es justo lo contrario.
#define FAREWELL_BOND_MIN 70
// DE DONDE SALEN LAS BOLAS. No hay tienda a proposito: salen de CUIDAR y
// de ENTRENAR, que es la tesis del juego. Dos grifos, los dos atados al
// ciclo diario:
//
//   - el primer cuidado del dia da una Poke Ball, y la racha da mejores
//     (una Great cada semana, una Ultra cada mes);
//   - una sesion del gimnasio paga segun lo bien que lo hagas.
//
// La Master Ball (el ultimo escalon) NO se regala nunca por aqui.
// Y el profesor te suelta un punado al empezar. Sin esto la primera partida
// arranca seca: te cruzas con los primeros salvajes sin nada con que
// cogerlos y hay que esperar al dia siguiente.
#define OAK_BALLS 5
#define BALL_STREAK_GREAT 7      // cada tantos dias de racha, una Great
#define BALL_STREAK_ULTRA 30     // y cada tantos, una Ultra
#define TRAIN_BALL_SCORE 12      // puntos del gimnasio por cada bola
#define TRAIN_BALL_MAX 2         // tope por sesion
#define TRAIN_BALL_GREAT 16      // a partir de aqui pueden salir mejores
#define TRAIN_BALL_ULTRA 26
// As bolas de condicao: % de o cuidado do dia trazer uma a mais, % de um
// treino bom (>= TRAIN_BALL_GREAT) trazer uma, e o pacote de boas-vindas
// (Quick, Level e Friend) que toda partida recebe UMA vez (as velhas
// tambem, ao carregar).
#define BALLX_DAILY_PCT 25
#define BALLX_TRAIN_PCT 20
#define BALLX_WELCOME 3
// Friend Ball: chega com este vinculo em vez de BOND_AT_BIRTH. Luxury
// Ball: cada subida de vinculo vale +1 e o teto do dia sobe para este.
#define BOND_FRIEND_BALL 85
#define BOND_DAY_CAP 20
#define BOND_DAY_CAP_LUXURY 30

// A donde fue a parar lo que acabas de capturar
enum : uint8_t { CATCH_FULL = 0, CATCH_TEAM, CATCH_BANK };
#define BOND_AT_BIRTH 60
#define EV_STAT_CAP 100         // techo por stat
#define EV_TOTAL_CAP 200        // techo de la suma: da para dos stats a tope
#define TRAIN_SCORE_PER_EV 2    // puntos del saco que hacen falta por EV
#define TRAIN_EV_PER_SESSION 12 // tope de una sesion: los EVs se forjan lento
#define FAINT_HP_PCT 10         // con cuantos PS se levanta tras desmayarse
#define FAINT_JOY_COST 25       // el susto se le nota
#define HP_HEAL_SLEEP_PCT 1     // % del maximo por minuto durmiendo (~1h40 lleno)
#define HP_HEAL_AWAKE_MIN 5     // despierto: 1 PS cada tantos minutos
#define HP_HEAL_BERRY_PCT 15    // % del maximo por baya (la favorita, el doble)

// milisegundos que faltan hasta `deadline` (0 si ya paso, o si deadline==0:
// "temporizador inactivo"). Todos los temporizadores del juego (dialogos,
// animaciones) se guardan como un instante absoluto "deadline = millis() +
// duracion" y se comparaban con millis() < deadline / millis() > deadline;
// eso se rompe cuando millis() da la vuelta a los ~49,7 dias de uptime. La
// resta sin signo, reinterpretada como con signo, es segura frente a esa
// vuelta mientras el intervalo real sea muy inferior a ~24,8 dias (aqui el
// temporizador mas largo son ~12 s), sin cambiar como se guarda el deadline.
static inline uint32_t timeLeft(uint32_t deadline) {
  if (!deadline) return 0;
  int32_t left = (int32_t)(deadline - millis());
  return left > 0 ? (uint32_t)left : 0;
}

// ceremonias de fin de ciclo
enum : uint8_t { CER_NONE = 0, CER_FAREWELL, CER_RUNAWAY, CER_RELEASE };

enum PetMood : uint8_t { MOOD_HAPPY, MOOD_SAD, MOOD_EATING, MOOD_SLEEPING };

// Lo que el bicho PIDE. mood() solo dice "esta mal"; esto dice de que, para
// poder sacarle un globo con el icono correspondiente y que el jugador no
// tenga que abrir la ficha para enterarse. Avisa en 35, antes de que
// lowestStat() < 25 lo ponga MOOD_SAD: primero pide, luego se entristece.
enum PetNeed : uint8_t { NEED_NONE, NEED_FOOD, NEED_PLAY, NEED_CLEAN, NEED_ENERGY };
#define NEED_WARN 35

// LOGROS (bitmask). Antes eran ocho "medallas" del bicho que criabas;
// ahora son del JUGADOR y cuentan tambien lo que haces fuera de casa:
// la Pokedex, los combates, las megas y haberse coronado en cada region.
//
// Los ocho primeros CONSERVAN su bit a proposito: una partida guardada
// trae un uint16 con esos ocho, y al leerlo en el uint32 nuevo cae en su
// sitio sin convertir nada.
enum : uint32_t {
  MED_LV10 = 1u << 0, MED_LV25 = 1u << 1, MED_LV50 = 1u << 2,
  MED_BERRY = 1u << 3, MED_STREAK7 = 1u << 4, MED_BOND = 1u << 5,
  MED_FINAL = 1u << 6, MED_FIT = 1u << 7,
  // --- los nuevos ---
  MED_LV100 = 1u << 8,      // al techo del juego
  MED_STREAK30 = 1u << 9,   // un mes cuidandolo
  MED_DEX50 = 1u << 10, MED_DEX150 = 1u << 11, MED_DEX500 = 1u << 12,
  MED_CAUGHT100 = 1u << 13,
  MED_WINS100 = 1u << 14, MED_WINS500 = 1u << 15,
  MED_SHINY = 1u << 16,     // uno brillante en la Pokedex
  MED_MEGA1 = 1u << 17, MED_MEGA10 = 1u << 18,
  MED_REMATCH = 1u << 19,   // una revancha ganada
  MED_TEAM6 = 1u << 20,     // el equipo lleno
};
// Y uno por REGION terminada, detras de los de arriba. Son los que pidio
// el usuario ("por derrotar kanto, johto, etc") y van calculados en vez
// de escritos uno a uno: anadir una region no deja su logro fuera.
// Cuantos estan TRADUCIDOS a los ocho idiomas: los ocho de siempre.
// Los de despues llevan una tabla sola en i18n.cpp —— son cifras y
// nombres propios. Las tablas por idioma se dimensionan con esto, y
// no con MED_COUNT: con MED_COUNT quedarian medio vacias de punteros
// nulos y el primer logro nuevo reventaria dentro de strlen.
#define MED_TRANSLATED 8
#define MED_REGION0 21
#define MED_REGION(r) (1u << (MED_REGION0 + (r)))
#define MED_COUNT (MED_REGION0 + REGION_COUNT)
static_assert(MED_COUNT <= 32, "los logros viven en un uint32");

// Lo que PAGA cada medalla, por numero de bit (ver el enum de arriba).
// Hasta ahora una medalla salia en pantalla, sonaba y se acababa ahi.
// Son ids de ITEM_TBL y los comprueba test_pet.cpp por NOMBRE: una tabla
// de numeros sueltos se desalinea sin que nadie se entere en cuanto
// gen_items.py mueva una fila.
static const uint8_t MEDAL_PRIZE[MED_COUNT] = {
  29,   // Nv.10        Super Potion
  30,   // Nv.25        Hyper Potion
  34,   // Nv.50        Max Revive
  2,    // BAYA         Sitrus Berry
  1,    // RACHA 7      Leftovers
  5,    // VINCULO      Focus Band
  10,   // FINAL        Scope Lens
  51,   // EN FORMA     Protein
  59,   // Nv.100       Master Ball —— el techo del juego paga el mejor objeto
  32,   // RACHA 30     Full Restore
  58,   // DEX 50       Ultra Ball
  44,   // DEX 150      Max Elixir
  6,    // DEX 500      Life Orb
  58,   // 100 CAZADOS  Ultra Ball
  7,    // 100 GANADOS  Choice Band
  8,    // 500 GANADOS  Choice Specs
  31,   // SHINY        Max Potion
  52,   // 1a MEGA      Iron
  55,   // 10 MEGAS     Carbos
  34,   // REVANCHA     Max Revive
  50,   // EQUIPO LLENO HP Up
  // Y las nueve regiones. La primera paga una Master Ball; las de
  // despues reparten lo bueno sin repetir dos seguidas.
  59, 32, 44, 6, 8, 53, 54, 7, 31,
};

// Lo que paga el cuestionario del Profesor, por aciertos (0..QUIZ_LEN).
// 0 = no paga nada: con dos de cinco todavia hay premio, con uno no, y
// asi acertar significa algo.
// (leva 68: subiu um degrau, e os 5 de 5 vao a quizPremioCheio.)
static const uint8_t QUIZ_PRIZE[6] = {
  0,    // 0 de 5
  0,    // 1
  29,   // 2        Super Potion
  30,   // 3        Hyper Potion
  31,   // 4        Max Potion
  31,   // 5        (nao se usa: ver quizPremioCheio)
};

// Os 5 de 5. Com 4 acertos a quinta e SEMPRE "quem e esse Pokemon" em
// silhueta preta (a mais dificil), e acertar paga melhor: dois Rare
// Candy, ou, 1 vez em 20, a Shiny Potion. `qtd` diz quantos.
#define QUIZ_SHINY_EM 20
static inline uint8_t quizItemDe(uint8_t fx) {
  for (uint16_t i = 1; i < ITEM_COUNT; i++)
    if (ITEM_TBL[i].fx == fx) return (uint8_t)i;
  return 0;
}
static inline uint8_t quizPremioCheio(uint8_t *qtd) {
  if (random(QUIZ_SHINY_EM) == 0) { *qtd = 1; return quizItemDe(IT_SHINY); }
  *qtd = 2;
  return quizItemDe(IT_RARECANDY);
}

// A partir de aqui se considera que el bicho esta gordo. Es el mismo
// numero con el que ya le baja la energia, no uno nuevo inventado.
#define WEIGHT_FAT 50

// LA AVENTURA (aventura.ino es la cena; aqui, lo que se gana). Se cuenta por
// el reloj, asi que vale con el aparato apagado.
#define AV_MIN_MIN 1           // ya desde el primer minuto da algo, poquito
#define AV_TETO_MIN 480        // 8 h: dejarlo una semana da lo mismo que 8 h
#define AV_ENTERO_MIN 120      // hasta aqui cuenta entero; de aqui al techo, la mitad
#define AV_ROLO_MIN 40         // una tirada de objeto cada tanto
#define AV_MAX_ITENS 8
#define AV_DE_LA_CAJA 6        // cuantos del banco salen en cada aventura
#define AV_MAX_ELENCO (TEAM_SLOTS + AV_DE_LA_CAJA)
struct AvParticipa {
  int16_t dex = 0;
  bool shiny = false;
  uint8_t origen = 0;          // 0 titular, 1 equipo, 2 caja
  uint8_t hueco = 0;
  uint32_t exp = 0;
  uint8_t subiu = 0;
};
struct AvResumo {
  uint32_t minutos = 0;
  uint8_t nMons = 0;
  AvParticipa mons[AV_MAX_ELENCO];
  uint8_t nItens = 0;
  uint8_t itens[AV_MAX_ITENS] = { 0 };
};

class Pet {
public:
  // La aventura en curso: desde cuando (epoch, 0 = ninguna) y su semilla.
  uint32_t avDesde = 0, avSeed = 0;
  bool aventuraAtiva() const { return avDesde != 0; }
  bool aventuraComeca(uint32_t nowEpoch, uint32_t seed);
  uint32_t aventuraMinutos(uint32_t nowEpoch) const;  // ya con el techo
  uint16_t aventuraRolos(uint32_t nowEpoch) const;
  uint8_t aventuraItem(uint16_t i) const;   // el objeto de la tirada i, 0 = nada
  uint8_t aventuraElenco(AvParticipa *out) const;     // [0] es el titular
  // Reparte, guarda y deja de haber aventura. Se cobra UNA vez.
  void aventuraFim(uint32_t nowEpoch, AvResumo &r);
  // Estadisticas 0..100
  uint8_t fullness = 80;  // comida
  uint8_t joy = 80;       // felicidad
  uint8_t energy = 80;    // energia
  uint8_t hygiene = 100;  // limpieza
  uint8_t poops = 0;      // cacas en pantalla (max 3)
  uint8_t weight = 0;     // 0-100: las chuches engordan, el minijuego quema
  // Genes (IVs, 90-110%, se tiran al eclosionar) y entrenamiento (EVs, 0-100).
  //
  // Eran TRES y ahora son SEIS. Con tres, el ataque especial compartia campo
  // con el fisico y la defensa especial con la suya, asi que Proteina y
  // Calcio habrian hecho exactamente lo mismo y no habia forma de criar a un
  // bicho especial. Los seis stats existian desde hace tiempo; lo que
  // faltaba era poder tocarlos por separado.
  uint8_t geneHp = 100, geneAtk = 100, geneDef = 100;
  uint8_t geneSpa = 100, geneSpd = 100, geneSpe = 100;
  uint8_t trHp = 0, trAtk = 0, trDef = 0;
  uint8_t trSpa = 0, trSpd = 0, trSpe = 0;
  // La habilidad de ESTE individuo. Sale en el huevo y no se cambia: a
  // diferencia del objeto o de los golpes, es parte de quien es el bicho.
  // 0 = sin sortear todavia (partida vieja): se resuelve al cargar.
  // uint16_t: son 293 habilidades con la dex entera (ver party.h).
  uint16_t ability = 0;
  bool berryKnown = false;  // ya descubrio su baya favorita
  bool shiny = false;       // variante de color rara (se sortea en el huevo)
  // El SEXO. Se sortea en el huevo con el reparto de su especie
  // (DexEntry::femPct) y no cambia nunca, como la habilidad. Hace falta
  // para dos evoluciones del Anil — Snorunt solo da Froslass si es
  // hembra, y Burmy se parte en Wormadam (hembra) y Mothim (macho) — y
  // para elegir el sprite, que 98 especies tienen uno propio de hembra.
  //
  // `genderSet` distingue "macho" de "aun sin sortear": sin el, una
  // partida vieja volveria a tirar el dado en cada carga.
  bool female = false;
  bool genderSet = false;
  // La FORMA regional. 0 = la normal. Sale de DONDE se consiguio el
  // bicho: en Alola, un Vulpix nace de Alola. Al evolucionar se conserva
  // si el destino la tiene (A.Vulpix da A.Ninetales).
  uint8_t form = 0;
  uint32_t ageMinutes = 0;   // solo EDAD: eclosion, cuidado, despedida
  uint32_t exp = 1;          // y de aqui sale el NIVEL, que son cosas distintas
  int16_t speciesId = -1;      // numero de Pokedex (1-151), -1 = huevo
  int16_t prevSpeciesId = -1;  // para la animacion de evolucion
  // Si la ultima evolucion dejo ademas OTRO bicho (Nincada da Ninjask
  // y de propina un Shedinja), cual fue. Lo lee la pantalla para poder
  // contarlo; si no se avisa, al jugador le aparece un bicho en el
  // equipo sin explicacion. 0 = no hubo. No se guarda en la NVS: es de
  // usar y tirar, como el aviso del salvaje.
  int16_t lastShedDex = 0;
  // Descuidos. NO tocan la evolucion —— llegar al nivel basta ——: cuentan
  // para la calidad del huevo que deja este bicho y para la medalla de
  // criarlo sin un solo fallo. Son el futuro, no un peaje.
  uint8_t careMistakes = 0;
  bool sleeping = false;
  // Lo acosto EL JUGADOR poniendo la consola boca abajo, no es que se
  // haya desmayado. Hace falta distinguirlo: dormir es como se cura
  // (ver faint()), asi que despertarle a tapas solo puede valer si
  // fuiste tu quien lo mando a la cama. Si no, dos golpecitos sin
  // querer le cortarian la cura.
  bool restByGesture = false;
  // Pasos: los de hoy (para una medalla y la pantalla) y los que aun
  // no se han cobrado. Se cobra por TRAMOS, no por paso.
  uint16_t stepsToday = 0;
  uint16_t stepsPend = 0;
  uint32_t lastSeenEpoch = 0;   // ultima hora RTC vista (para progresion offline)
  // Se apago GUARDANDO, no a lo bruto: al volver NO se aplica el tiempo de
  // fuera. Es la unica pausa que existe — apagar normal no pausa nada,
  // porque el RTC sigue contando con la placa muerta y eso es a proposito.
  bool guardado = false;
  uint8_t ceremony = CER_NONE;  // despedida/escapada/liberacion en curso
  uint8_t lastEnd = CER_NONE;   // como acabo la anterior (afecta al huevo)
  // Lo que dejo en herencia el que se despidio. Vive aqui y no en el bicho
  // porque tiene que sobrevivir a newEgg() y al apagon: el huevo puede
  // pasarse horas sin eclosionar.
  int16_t legacyDex = 0;     // 0 = huevo normal, sin herencia
  uint8_t legacyFloor = LEGACY_FLOOR_BASE;
  uint16_t legacyMove = 0;   // golpe heredado (0 = ninguno)
  uint8_t legacyItem = 0;    // lo que llevaba puesto
  // Pokedex de criados, un bit por especie. El tamano sale de DEX_COUNT: al
  // entrar Johto, 19 bytes se quedaban en 152 bits y los 99 ultimos se
  // registraban encima de otros, sin aviso. Un guardado viejo se lee tal
  // cual (`getBytes` copia lo que hay y el resto queda a cero) y el bit de
  // cada especie sigue siendo el mismo.
  uint8_t dexReg[DEX_REG_BYTES] = { 0 };
  uint8_t dexShinyReg[DEX_REG_BYTES] = { 0 };  // criados en version shiny
  // Leva 72: os VISTOS. Um bit por especie que apareceu do lado de la
  // (selvagem, treinador, lider, a outra placa) ou no passeio da aventura.
  // Registrado ja conta como visto (isSeen), entao um save velho nao perde
  // nada: so comeca vazio o que nunca existiu.
  uint8_t dexSeen[DEX_REG_BYTES] = { 0 };
  // A CACADA: a especie que se quer achar no mato (0 = nenhuma). Uma de
  // cada CACA_CHANCE buscas sai ela, se puder sair ali. Acaba ao capturar.
  int16_t cacaDex = 0;
  // Y las FORMAS regionales que has tenido (una fila de FORM_TBL cada bit).
  // Sin esto la Pokedex no podia ensenar un A.RAICHU aunque lo hubieras
  // criado: solo se guardaba "el 26 esta registrado", que es Raichu normal.
  uint8_t dexFormReg[(FORM_COUNT + 7) / 8] = { 0 };
  // Y las MEGAS que has visto megaevolucionar, una fila de MEGA_TBL por
  // bit. No van en dexFormReg porque una mega NO es una forma: vive en
  // su propia tabla y no tiene numero de dex (ver megas.h). Son 16 bytes.
  uint8_t dexMegaReg[(MEGA_COUNT + 7) / 8] = { 0 };
  // racha de cuidado diario (del jugador: persiste entre crianzas)
  uint16_t streak = 0, bestStreak = 0;
  uint32_t lastCareDay = 0;
  // Del JUGADOR, no del bicho: sobreviven a cada crianza y solo los borra un
  // factoryReset(). La intro se ve una vez en la vida del cacharro.
  bool introDone = false;
  bool playerGirl = false;
  // Progresion por Kanto. `badges` cuenta desafios superados (no solo
  // insignias: el Alto Mando y el campeon tambien suman, y por eso son lo
  // que mueve el techo de especies salvajes).
  // Las de la region en la que ESTAS. Las de las demas no se pierden:
  // viven en `badgesIn` y se intercambian al viajar.
  uint8_t badges = 0;
  // Las de CADA region. Viajar ya no cuesta nada —— antes cambiar de
  // region ponia las insignias a cero, que es como decirle al jugador
  // que lo que hizo no cuenta. Ahora cada region recuerda lo suyo y lo
  // que se gana al terminar una es la REVANCHA, no una cuenta nueva.
  uint8_t badgesIn[REGION_COUNT] = { 0 };
  // De los TRES de la casa, cuantos llevas ganados. Es el porton: con
  // tres se abre el lider. Sustituye al de antes (cinco victorias contra
  // salvajes), que medias el reloj y no al jugador.
  uint8_t gymTrainers = 0;
  // Las dos cuentas de la ficha de entrenador. Son del JUGADOR, como la
  // racha: sobreviven a cada crianza y solo las borra un factoryReset().
  uint32_t wins = 0;     // combates ganados, de siempre
  // A ultima TROCA aplicada (sessao). Vai na NVS no MESMO save que muda o
  // time: e isto que torna reaplicar idempotente (reiniciar entre o save e
  // marcar o diario APLICADO nao da o bicho duas vezes).
  uint32_t trocaUltima = 0;
  // O PLACAR das batalhas entre placas: a UNICA coisa que uma batalha
  // dessas grava (regra do user). Desistir conta; a queda de radio nao.
  uint16_t duV = 0, duD = 0, duE = 0;
  void contaDuelo(uint8_t r);   // 0 venci, 1 perdi, 2 empate
  uint16_t caught = 0;   // bichos capturados, de siempre
  // EN CUAL ESTAS. Manda sobre los gimnasios, las insignias, el nombre y
  // la hierba: estar en Johto hace que salgan muchos de Johto, no que
  // dejen de salir los de Kanto (ver wildWeight).
  //
  // Antes esto era "hasta donde has llegado" y habia un `zone` aparte
  // para "donde andas". Se junto en uno: con las insignias guardadas por
  // region, viajar es ir y volver sin precio, asi que no hacen falta dos
  // numeros —— y todo lo que ya preguntaba por `pet.region` (los
  // gimnasios, el techo, el retrato del lider) pasa a seguirte solo.
  uint8_t region = 0;
  // Hasta DONDE has destrabado. Esto no baja nunca: sube al terminar una
  // region, y es lo unico que decide que mapas se pueden abrir.
  uint8_t maxRegion = 0;

  // El salvaje que espera ahora mismo. NO se guarda en la NVS a proposito:
  // es de usar y tirar, y cada save() congela los dos cores casi un segundo.
  // Apagar el cacharro pierde el aviso, que es exactamente lo que pasaria si
  // te dieras la vuelta.
  int16_t wildDex = 0;
  uint16_t wildLevel = 0;
  bool wildShiny = false;
  // -1 = es un salvaje de la hierba; 0..TRAINER_COUNT-1 = te reta ese
  // entrenador. Cambia el aviso, el nivel, el premio y que no se captura.
  int8_t wildTrainer = -1;
  uint8_t wildTtl = 0;       // minutos que aguanta el aviso (0 = no hay nadie)
  uint8_t wildGap = 0;       // minutos hasta que pueda salir el siguiente
  uint8_t wildWait = 0;      // minutos sorteando sin suerte: sube la probabilidad
  // Hasta cuando esta gastado el boton de CAZAR (millis). No se guarda en
  // la NVS a proposito: es un enfriamiento de un minuto, no un estado del
  // bicho, y apagar y encender para saltarselo no compensa a nadie.
  uint32_t huntUntil = 0;
  // La enfermera. NO se guarda en NVS a proposito: es del rato que lleva
  // encendido el cacharro, no del bicho. Guardarla haria que al volver de
  // tres dias apagado la visita estuviera "vencida" desde hace dos.
  uint32_t pcNext = 0;   // millis en que se asoma
  uint32_t pcGone = 0;   // y en que se va
  // Lo mismo para la visita del Profesor. No se guarda en la NVS, igual
  // que la de la enfermera: es un evento de la sesion, y al encender
  // conviene que no te asalte un cuestionario en el primer segundo.
  uint32_t quizNext = 0;
  uint32_t quizGone = 0;
  // vinculo (del bicho: sube lento con cuidado, se resetea al nacer otro)
  uint8_t bond = 0;
  char nick[12] = "";    // apodo (vacio = nombre de especie)
  // A bola em que o titular foi capturado (indice de ITEM_TBL; 0 = nao
  // se sabe / nasceu aqui). Viaja com ele: StoredMon::ball.
  uint8_t ballId = 0;
  // medallas: del individuo + contador acumulado entre todas las crianzas
  // uint32 y no uint16: ahora son 30 logros. Al leer una partida
  // vieja el uint16 cae en los ocho bits de abajo, que son los mismos.
  uint32_t medals = 0;
  uint16_t totalMedals = 0;
  // Medallas ya PAGADAS. Separado de `medals` porque el premio se invento
  // despues: las que ya estaban ganadas tienen que cobrar la primera vez
  // que se mire, y sin este bit se pagarian otra vez en cada arranque.
  uint32_t medalPaid = 0;
  // Veces que ESTE individuo ha evolucionado, y a que edad llego a su
  // forma final. Lo primero es lo que convierte MED_FINAL en un logro
  // (un Tauros nace final); lo segundo, lo que evita que la despedida
  // caiga encima de la evolucion.
  uint8_t evoCount = 0;
  uint32_t finalAge = 0;
  // Si alguna vez estuvo gordo. MED_FIT pedia weight == 0, que es el peso
  // con el que NACE: la medalla de "en forma" se daba por no haber hecho
  // nada. Ahora hay que engordar y volver a bajarlo.
  bool wasFat = false;
  uint32_t newMedal = 0;   // recien conseguida(s), para celebrar
  uint16_t lastMilestone = 0;  // hito de racha ya celebrado
  uint16_t trainHi = 0;    // mejor puntuacion en el gimnasio (del jugador)

  // PP, con los numeros de siempre y ATRAVESANDO los combates: si se
  // restaurasen solos al acabar cada uno, un Tackle de 35 no se agotaria
  // jamas y el numero seria decoracion. Se recupera durmiendo y con bayas, o
  // sea que el sitio donde se recarga es el ciclo de cuidado del bicho.
  // Se guarda tambien QUE golpe habia en cada hueco: al subir de nivel el
  // moveset cambia, y un golpe nuevo tiene que entrar con sus PP llenos en
  // vez de heredar los del que ocupaba el hueco.
  uint16_t ppMove[4] = { 0, 0, 0, 0 };
  uint8_t ppLeft[4] = { 0, 0, 0, 0 };
  // Mientras esto sea false, los cuatro huecos se rellenan solos con los
  // ultimos golpes aprendidos, como hacia el juego antes. En cuanto el
  // jugador toca la pantalla de golpes pasa a true y ya manda el: un golpe
  // nuevo no le va a echar de los cuatro el que el eligio.
  bool movesPicked = false;

  // Los EVs. Son los mismos tres campos de siempre (trAtk/trDef/trSpe): ya
  // funcionaban como EVs, solo que repartidos de otra forma y sin nadie que
  // los llamara asi.
  //
  // De donde salen: de A QUIEN DERROTAS, como en los juegos. Cada especie
  // entrena aquello en lo que destaca, y eso se saca de su stat base mas
  // alto — sin datos nuevos. Se comprobo contra los EVs reales de 14
  // especies conocidas (Geodude, Abra, Chansey, Onix...) y acierta las 14.
  // Asi "entrenar" es elegir a quien cazar, que es justo lo que hace que un
  // encuentro salvaje sea una decision y no un combate mas.
  // SG_* no tiene casilla para los PS, asi que aqui se usa un indice propio
  // de seis que si los incluye.
  // Hay DOS techos, como en los juegos: uno por stat y otro para la suma.
  // Sin el segundo, entrenar bastante acababa poniendo los seis a 100 y los
  // EVs dejaban de ser una decision. Con 100 por stat y 200 en total salen
  // "dos stats a tope", que es exactamente la proporcion de 252/510.
  static uint8_t evYieldOf(int16_t dex);          // devuelve EV_*
  uint8_t gainEV(uint8_t which, uint8_t cuanto);  // devuelve lo que entro
  uint16_t evTotal() const;
  uint8_t evOf(uint8_t which) const;
  static const char *evName(uint8_t which);

  // ------------------------------------------------------- equipo y banco
  // OJO con la diferencia, que no es la misma cosa:
  //
  //   PRINCIPAL (mainSlot) — el que estas criando. Es el que sale en la
  //     pantalla, el que come, duerme, se ensucia y sube el vinculo. Se
  //     cambia SOLO a proposito, desde la pagina de equipo, y sigue siendo
  //     el principal aunque en un combate saques a otro o aunque caiga.
  //
  //   QUIEN PELEA — transitorio, de un combate. Cuando exista el cambio en
  //     mitad del combate vivira en la pantalla de combate, no aqui, y no
  //     tocara `mainSlot`. Perder no cambia de quien cuidas.
  //
  // El principal es este mismo Pet: es el que tiene barras, animacion y
  // suciedad. `team[mainSlot]` es su hueco; los demas del equipo estan
  // congelados salvo por un hilo de experiencia, y los del banco, del todo.
  //
  // Las cuatro barras son GLOBALES a proposito: describen al bicho que estas
  // criando ahora. Si viajaran con cada uno, los del banco se moririan de
  // hambre en la caja. El VINCULO si es de cada uno — es lo que hace que
  // cambiar de titular cueste algo.
  StoredMon team[TEAM_SLOTS];
  // ------------------------------------------ UM TIME POR REGIAO
  // `team` e sempre o da regiao onde voce esta. `teamsIn[r]` guarda
  // o das outras, e setZone() troca os dois —— exatamente como
  // `badgesIn` ja faz com as insignias. Sao 9*6*52 = 2,8 KB, que
  // cabem na NVS: de proposito NAO vao pro cartao, porque a SD nem
  // esta montada quando pet.begin() roda a migracao.
  StoredMon teamsIn[REGION_COUNT][TEAM_SLOTS];
  uint8_t mainSlotIn[REGION_COUNT] = { 0 };
  // Que regioes voce ja PISOU. Nao e `maxRegion` (o que esta
  // destravado): o ovo do inicial sai na primeira vez que voce poe
  // o pe la, e so nessa vez.
  uint16_t visitedMask = 1;      // Kanto, onde todo mundo comeca
  // A QUE REGIAO PERTENCE O TIME VIVO.
  //
  // Parece redundante com `region` e nao e: `region` e um numero
  // solto que, se voltar errado de um arranque, faz arquivar o
  // time no lugar errado sem que nada avise. Foi o que comeu o
  // time de Kanto do dono —— ele estava em Johto e a regiao
  // reapareceu como Kanto.
  //
  // Com isto o time traz a sua origem consigo. Se os dois
  // discordarem, manda este.
  uint8_t teamRegion = 0;
  // La caja ABIERTA. Las otras 31 estan en la SD; esto es la que se ve y
  // la unica que ocupa RAM.
  StoredMon bank[BANK_SLOTS];
  uint8_t boxOpen = 0;
  // El banco viejo (60 en una entrada de NVS) todavia no se ha podido
  // pasar a las cajas, porque la SD no estaba. Mientras esto este en
  // alto, save() NO toca la clave "bank": el blob de 60 se queda
  // entero y la proxima carga lo reintenta. Perder un cambio de esta
  // sesion es barato; perder treinta bichos no.
  bool bancoViejoPendiente = false;
  // La ultima vez que no se pudo guardar una captura, ¿fue porque no
  // habia sitio de verdad, o porque la tarjeta no dejo escribir? Para el
  // jugador son cosas muy distintas y el recado era el mismo.
  bool cajasSinTarjeta = false;
  // La ultima gravacion fallo (la NVS no acepto algun bloque). Los once
  // putBytes de save() no miraban el retorno: una particion llena
  // devolvia 0, no se guardaba nada y el juego seguia tan contento ——
  // el jugador solo lo nota cuando el progreso vuelve atras solo.
  bool guardadoFallo = false;
  uint8_t bloquesFallados = 0;
  // Regioes no SD (pet.cpp, "as regioes NO SD"). seqSave: a sequencia do
  // ultimo save; sdManda: o boot viu o SD mais novo que a NVS.
  uint32_t seqSave = 0;
  bool sdManda = false;
  bool sdGravouEstado = false;
  // ¿Se puede escribir en las cajas? Escribe un fichero de prueba en
  // /boxes y lo borra. No toca ninguna caja.
  bool pruebaDeEscritura() const;
  uint8_t mainSlot = 0;

  uint8_t teamCount() const;
  uint8_t bankCount() const;
  void storeActive();                  // vuelca el Pet a su hueco del equipo
  StoredMon fotoTitular() const;       // o mesmo registro, sem tocar em nada
  // Cambia de quien cuidas. Es una decision, no un gesto de paso: solo se
  // llama desde el boton de la pagina de equipo.
  bool setMain(uint8_t slot);
  // O miolo do setMain: despeja um StoredMon no Pet vivo. Esta
  // separado porque viajar de regiao faz a mesmissima coisa, e
  // eram 25 linhas pra duplicar.
  bool cargaDelHueco(uint8_t slot);
  // Copia o equipo inteiro numa chave propria, e devolve-o.
  // Existem para o comando REPONER ter UNDO: escrever no save de
  // alguem sem caminho de volta ja custou cinco bichos de nivel 50.
  // Diagnostico, so leitura: tamanhos reais das chaves da NVS e
  // estado das gravacoes. Existe porque adivinhar a causa de uma
  // perda de dados no aparelho de alguem custou caro quatro vezes.
  // Uma chave por regiao, carimbada. Ver pet.cpp.
  void guardaTimeDe(uint8_t r);
  bool leTimeDe(uint8_t r);
  void diagSave();
  void diagBackup();
  bool equipoACopia(const char *clave);
  bool equipoDeCopia(const char *clave);
  // Pisar numa regiao pela primeira vez: ovo do inicial dela.
  void llegadaARegion();
  bool toBank(uint8_t teamSlot);       // manda uno del equipo al banco
  bool fromBank(uint8_t bankSlot);     // lo trae de vuelta al equipo
  // Solta de vez um bicho do banco (a caixa aberta). O que ele levava
  // volta para a bolsa. Nao tem volta: a tela pede confirmacao antes.
  bool releaseFromBank(uint8_t bankSlot);
  // Las cajas. boxGo guarda la abierta y abre otra; boxWrite y boxRead
  // son el fichero de la SD. boxCount cuenta los de una caja SIN abrirla,
  // que es lo que necesita la pantalla para decir "3/30" de las demas.
  bool boxGo(uint8_t n);
  bool boxWrite() const;
  bool boxRead(uint8_t n, StoredMon *dst) const;
  uint8_t boxCount(uint8_t n) const;
  bool addToTeam(const StoredMon &m);  // al primer hueco libre del equipo
  bool addToBank(const StoredMon &m);  // idem en el banco
  // Un bicho capturado. Devuelve CATCH_TEAM, CATCH_BANK o CATCH_FULL. Va al
  // equipo si hay sitio y al banco si no, como en los juegos: quien captura
  // no quiere que le pregunten, quiere que se guarde.
  uint8_t catchMon(int16_t dex, uint16_t lvl, bool isShiny, uint16_t abil,
                   uint8_t ball = 0);
  // Reparto de bolas. `giveBall(tier)` mete una del escalon pedido (0 =
  // Poke) y devuelve su id. Las otras dos son los dos grifos del juego.
  // RECOGIDA: con Pickup, de vez en cuando aparece con algo. Lo llama
  // el tick, no el combate. `lastPickup` es lo que acaba de traer (0
  // si nada), para que la pantalla pueda contarlo.
  void rollPickup();
  int16_t lastPickup = 0;
  // La ULTIMA evolucion de un suplente, para que la pantalla pueda
  // contarla. Los del banquillo evolucionan solos — no estan en pantalla,
  // asi que no hay animacion que ensenar ni boton que pulsar —, pero
  // enterarse de que tu Weedle ya es un Kakuna no puede depender de que
  // se te ocurra abrir la lista de equipo.
  int16_t teamEvoFrom = 0, teamEvoTo = 0;

  // ---------------------------------------------------------- las MT
  //
  // Un mapa de bits: 286 MT caben en 36 bytes. No son objetos de la
  // bolsa a proposito (ver tms.h): los ids de objeto son uint8_t y no
  // habria sitio.
  //
  // Las MT son del JUGADOR, no del bicho: se ganan una vez y sirven
  // para siempre, como en los juegos modernos. Asi la decision es QUE
  // CUATRO GOLPES le pones, que es la interesante, y no "me la gasto o
  // la guardo".
  uint8_t tms[TM_BYTES] = { 0 };
  bool hasTM(uint16_t tm) const {
    return tm < TM_COUNT && ((tms[tm >> 3] >> (tm & 7)) & 1);
  }
  bool giveTM(uint16_t tm);          // true si es NUEVA
  uint8_t tmCount() const;           // cuantas tienes
  // ¿Se la puedes ensenar AHORA a tu bicho? Tiene que ser suya, tienes
  // que tenerla y no puede sabersela ya.
  bool canTeachTM(uint16_t tm) const;
  // Se la ensena en ese hueco (0..3). Devuelve false si no se puede.
  bool teachTM(uint16_t tm, uint8_t slot);
  // Una MT al azar de las que tu bicho PODRIA aprender y no tienes.
  // Es la que reparte el gimnasio: regalar una que no le sirve a nadie
  // seria un premio que no se nota.
  // `tipo` = el tipo del lider que la regala (T_NONE = sin preferencia).
  // Se PREFIERE ese tipo, no se exige: un bicho de Agua que gane a Brock
  // puede no aprender ninguna MT de Roca, y dejarlo sin premio seria peor
  // que darle una rara.
  int16_t rollTM(uint8_t tipo = T_NONE) const;
  int16_t lastTM = -1;               // la ultima que te dieron, para contarlo

  uint8_t giveBall(uint8_t tier);
  // Uma das bolas de condicao (Quick, Level...), sorteada. Nao tem loja:
  // caem do cuidado diario e do treino, como as outras.
  uint8_t giveBallX();
  uint8_t dailyBalls();              // primer cuidado del dia; devuelve cuantas
  uint8_t trainBalls(uint16_t score);  // premio del gimnasio; idem
  // Lo ultimo que se repartio, para que la pantalla pueda contarlo.
  uint8_t lastBallId = 0, lastBallCount = 0;
  // El hilo de experiencia de los que no salen: poco, pero que no se queden
  // clavados mientras el titular sube. Lo llama el combate al ganar.
  // `peleo` = el hueco del que estuvo en pista (se lleva la parte
  // entera y no cobra aqui). 0xFF = el principal, que es quien pelea
  // mientras no se releve a nadie.
  void teamShareExp(uint32_t total, uint8_t peleo = 0xFF);
  static uint16_t levelOf(const StoredMon &m);

  void rollAbility();                // sortea la del individuo (al eclosionar)
  void rollGender();                 // idem con el sexo, segun su especie
  // Lo que se pinta al lado del nombre: "", " M" o " F". Los 155 sin sexo
  // no llevan nada, que es lo que hacen los juegos.
  const char *genderMark() const;
  bool equipMove(uint16_t mv);       // al primer hueco libre
  bool unequipMove(uint8_t slot);
  bool moveEquipped(uint16_t mv) const;

  // Lo mismo para un SUPLENTE, por hueco del equipo. La pagina de golpes
  // sirve para los seis: antes habia que hacer titular al que querias mirar,
  // y eso cuesta el vinculo del que lo era. Mirar no puede costar.
  //
  // El titular no pasa por aqui: su copia de `team[mainSlot]` solo se pone al
  // dia en storeActive(), asi que editarla se perderia. Las tres devuelven
  // false si les dan su hueco.
  bool teamMoves(uint8_t slot, uint16_t *out4, uint8_t *pp4) const;
  bool teamEquipMove(uint8_t slot, uint16_t mv);
  bool teamUnequipMove(uint8_t slot, uint8_t i);

  // PS, con la misma regla que los PP: atraviesan los combates y los devuelve
  // el cuidado. Caer NO es morir ni perder nada — el bicho se desmaya, se
  // duerme solo y se levanta tocado. El precio es de GENERACION: cuenta como
  // error de cuidado, y los errores ya empeoran el sorteo del proximo huevo y
  // la tirada de shiny. Se pierde un poco del futuro, no del presente.
  uint16_t hpLeft = 0;
  uint16_t hpSeen = 0;   // maxHp la ultima vez: al subir de nivel, la subida
                         // se suma a los PS actuales, como en los juegos

  // Lo que lleva encima, y lo que tiene guardado. La bolsa es un contador
  // por objeto y no una lista: con 28 objetos son 28 bytes y no hay que
  // ordenar nada. Los objetos se encuentran ganando combates.
  uint8_t heldItem = 0;
  uint8_t bag[ITEM_COUNT] = { 0 };

  bool takeItem(uint8_t id);     // guardar uno en la bolsa
  bool holdItem(uint8_t id);     // ponerselo (0 = quitarselo)
  // Da o objeto `id` da bolsa a quem esta no hueco `slot` do time (o
  // titular passa por holdItem: o dele vive no Pet). O que ele levava
  // volta a bolsa. Dar o MESMO que ja leva = tirar. Existe para as
  // trocas: Scyther segurando Metal Coat, e o titular nao sai.
  bool darItem(uint8_t slot, uint8_t id);
  // Quem do time (fora o titular) leva `id`; 0xFF = ninguem.
  uint8_t quemLeva(uint8_t id) const;
  bool useItem(uint8_t id);      // gastarlo: pocion, eter, cura... FUERA del combate
  // Restar una unidad y grabar, sin aplicar ningun efecto. Lo pide el
  // combate: alli el objeto va sobre el que esta EN PISTA (que puede ser
  // un suplente, y cuya vida vive en el Combatant) y no sobre lo
  // guardado, asi que el efecto lo aplica btItemOnField y aqui solo
  // queda el apunte contable.
  bool spendItem(uint8_t id);
  // Que cae al ganar (0 = nada). `rico` = era un entrenador: suelta mas a
  // menudo y con mas peso de piedras, que es la via razonable de
  // conseguirlas (los lideres solo dan UNA de cada).
  uint8_t rollBattleDrop(bool rico = false);
  // Una piedra que le sirva a alguien de los tuyos, 0 si ninguna. Se mira
  // antes de sortear a ciegas: con siete objetos de evolucion, el que te
  // hace falta caia 1 de cada 7 y lo normal era acumular las que no usas.
  uint8_t usefulStone() const;
  // Cuantos tipos distintos hay en un bolsillo, y el n-esimo. La bolsa por
  // dentro es un contador por objeto (61 bytes, sin ordenar nada); los
  // bolsillos son una VISTA encima, que es lo que hace falta para pintarlos.
  // Como se ordena la lista de un bolsillo: 0 la de la tabla (como
  // estaba), 1 por nombre, 2 por efecto y luego por nombre. Es una
  // PREFERENCIA, asi que vive aqui y se guarda, no en la pantalla.
  uint8_t bagSort = 0;
  static const uint8_t BAG_SORTS = 3;
  // Rueda al siguiente orden y lo guarda. Va aqui y no en la pantalla
  // porque guardar es cosa del Pet (save() es suyo y es privado).
  void cycleBagSort() {
    bagSort = (uint8_t)((bagSort + 1) % BAG_SORTS);
    save();
  }
  uint8_t pocketCount(uint8_t pocket) const;
  uint8_t pocketItem(uint8_t pocket, uint8_t n) const;

  void hpSync();                      // ajusta al nivel y al tope
  void hpHeal(uint16_t cuanto);
  void hpFaint();                     // cayo en combate
  bool canBattle() const;             // entero y despierto de sobra para pelear
  bool canTrain() const;              // con energia para una sesion
  // Por que NO puede: para poder decirselo al jugador en vez de dejar el
  // boton muerto sin explicacion.
  const char *whyNotBattle() const;
  void afterBattle(uint16_t hpQueda); // cierra el combate: PS, energia, desmayo

  void ppSync();                    // pone al dia los cuatro huecos
  void ppRestoreAll();              // dormir: todo al maximo
  uint8_t ppRestore(uint8_t cuanto);  // baya: reparte y devuelve lo repuesto
  bool ppEmpty() const;             // sin nada que usar: solo queda forcejear
  // Vuelca SOLO lo del combate (PS y PP). Al acabar un combate no ha cambiado
  // nada mas, y un save() entero congela los dos cores casi un segundo.
  void battleSave();

  void begin();                 // carga estado de NVS (o crea el primer huevo)
  void update(uint32_t nowMs);  // llamar en cada loop()

  // Acciones (botones tactiles)
  void feed();              // baya roja (compatibilidad)
  void feedBerry(uint8_t color);  // 0 roja, 1 azul, 2 verde
  void feedCandy();
  bool lovesBerry(uint8_t color) const {
    return !isEgg() && (speciesId % 3) == color;  // gusto oculto por especie
  }
  // Gimnasio: una sesion del saco de tipos entrena el EV que elijas.
  // Devuelve los puntos que de verdad entraron (puede ser menos de lo que
  // daba la puntuacion si el stat o el total ya estaban al tope).
  uint8_t trainEV(uint8_t which, uint16_t score);

  // stats de combate: base real de gen 1 x genes + nivel + entrenamiento
  uint16_t atkStat() const;
  uint16_t defStat() const;
  uint16_t speStat() const;
  uint16_t spAtkStat() const;  // los especiales comparten genes con su pareja
  uint16_t spDefStat() const;  // fisica; ver el comentario en pet.cpp
  uint16_t maxHp() const;      // PS de combate (formula propia, no calcStat)
  void play();
  void toggleLight();  // dormir / despertar
  void clean();
  void caress();
  // --- lo que llega por el ACELEROMETRO ---
  // Un zarandeo brusco: al bicho no le gusta. Es el precio de menear.
  void scare();
  // Boca abajo: a la cama. Devuelve false si no era momento (huevo,
  // ceremonia, o ya dormia).
  bool restDown();
  // Dos tapas: arriba. Solo si lo acostaste TU.
  bool wakeUp();
  // Andar con la consola encima: ejercicio de verdad. Quema peso y
  // anima, y no gasta fuelle porque el que anda eres tu.
  void walk(uint16_t pasos);  // tocar al bicho
  void eggTap();  // tocar el huevo: 3 toques y eclosiona
  void newEgg();   // empezar de cero con un inicial aleatorio
  void release();  // soltar (pulsacion larga + confirmar)
  void syncClock(uint32_t nowEpoch);  // aplica el tiempo transcurrido apagado
  // Marcar y volcar JUSTO antes de cortar la corriente: la vuelta siguiente
  // se salta la progresion offline. La marca se gasta en una sola vuelta.
  void guardar(uint32_t nowEpoch);
  void setClock(uint32_t nowEpoch);   // fija la hora sin aplicar progresion
  void startFarewell();  // tambien usable desde la consola serie (BYE)
  void startRunaway();   // tambien usable desde la consola serie (RUN)

  // ------------------------------------------------- encuentros salvajes
  // La hora entra por parametro y no se lee de dentro: asi el modelo no
  // depende de que haya reloj puesto y los tests pueden pedir "a las 3 de
  // la madrugada" sin tocar la NVS.
  bool wildWaiting() const { return wildDex > 0 && wildTtl > 0; }
  // El boton de cazar: listo, cuanto le falta, y usarlo.
  bool huntReady(uint32_t now) const {
    return !huntUntil || (int32_t)(now - huntUntil) >= 0;
  }
  uint16_t huntLeftS(uint32_t now) const {
    if (huntReady(now)) return 0;
    return (uint16_t)((huntUntil - now + 999) / 1000);
  }
  // Saca un salvaje AHORA. Devuelve false si no es momento (durmiendo,
  // huevo, ceremonia o ya hay uno esperando).
  bool hunt(uint8_t hour, uint32_t now);

  // ---- el centro Pokemon ----
  bool needsHealing() const;              // hay algo que curar ahora mismo
  bool pcWaiting(uint32_t now) const {    // la enfermera esta en pantalla
    return pcGone && (int32_t)(now - pcNext) >= 0 && (int32_t)(now - pcGone) < 0;
  }
  void pcTick(uint32_t now);
  void quizTick(uint32_t now);          // la visita del Profesor
  bool quizWaiting(uint32_t now) const {
    return quizGone && (int32_t)(now - quizNext) >= 0 &&
           (int32_t)(now - quizGone) < 0;
  }
  void quizLeave() { quizNext = 0; quizGone = 0; }              // programa la siguiente visita
  uint8_t healAll();   // PS y PP de TODOS; devuelve a cuantos les hizo falta

  // ---- el mapa ----
  bool zoneUnlocked(uint8_t r) const { return r <= maxRegion; }
  // VIAJAR. Ir y volver entre lo que ya tienes abierto no cuesta nada:
  // las insignias de cada region se guardan y se recuperan, asi que
  // volver a Kanto te devuelve tus ocho y no te las quita.
  bool setZone(uint8_t r) {
    if (!zoneUnlocked(r) || r >= REGION_COUNT) return false;
    if (region == r) return false;
    // Uma regiao ja pisada cujo time nao se le (SD fora, arquivo ruim) e sem
    // nada na memoria: NAO se viaja. Chegar la daria o ovo do inicial, e o
    // save logo a seguir gravaria o ovo por cima do time de verdade.
    if (regionVisited(r) && !leTimeDe(r)) {
      bool algum = false;
      for (uint8_t i = 0; i < TEAM_SLOTS; i++)
        if (teamsIn[r][i].used()) algum = true;
      if (!algum) return false;
    }
    // O DAQUI, guardado. O titular vai pro buraco dele antes, senao se
    // perde o que ele andou (vida, PP, vinculo) desde a ultima vez.
    storeActive();
    badgesIn[region] = badges;
    for (uint8_t i = 0; i < TEAM_SLOTS; i++) teamsIn[region][i] = team[i];
    mainSlotIn[region] = mainSlot;
    // A chave DESTA regiao, fechada antes de sair. Assim o time dela
    // fica gravado no seu proprio sitio, com o seu carimbo, antes de
    // `region` mudar de valor —— que e o momento em que tudo se
    // estragou nas duas vezes anteriores.
    guardaTimeDe(region);

    region = r;
    teamRegion = r;   // o time que vem a seguir e DESTA regiao

    // E o DE LA, recuperado.
    badges = badgesIn[r];
    for (uint8_t i = 0; i < TEAM_SLOTS; i++) team[i] = teamsIn[r][i];
    mainSlot = mainSlotIn[r] < TEAM_SLOTS ? mainSlotIn[r] : 0;
    gymTrainers = 0;               // o portao e de CADA casa

    if (!regionVisited(r)) {
      // Primeira vez aqui: ovo do inicial da regiao. O titular que voce
      // tinha NAO some —— ficou no time da regiao de onde voce veio, e
      // volta a ser titular se voce voltar pra la.
      visitedMask |= (uint16_t)(1u << r);
      llegadaARegion();
    } else {
      // O titular tem de ser ALGUEM DAQUI. Isto era um
      // `else if (team[mainSlot].used())`, e quando o slot guardado
      // estava vazio o cargaDelHueco era pulado: o bicho da regiao
      // anterior continuava carregado e o save() logo abaixo o gravava
      // NESTA regiao. Viajar movia o bicho de lugar.
      //
      // Nao aparecia nos testes porque neles o slot de destino sempre
      // tinha alguem —— so quebra quando esta vazio.
      uint8_t hueco = TEAM_SLOTS;
      if (team[mainSlot].used()) {
        hueco = mainSlot;
      } else {
        for (uint8_t i = 0; i < TEAM_SLOTS; i++)
          if (team[i].used()) { hueco = i; break; }
      }
      if (hueco < TEAM_SLOTS) {
        mainSlot = hueco;
        cargaDelHueco(hueco);
      } else {
        // Pisada mas sem ninguem (um save antigo, ou este mesmo bug).
        // Melhor um ovo do que importar o bicho da outra regiao calado.
        llegadaARegion();
      }
    }
    save();
    return true;
  }
  void wildClear() {
    wildDex = 0; wildTtl = 0; wildShiny = false; wildTrainer = -1;
    wildGap = WILD_GAP_MIN;
    wildWait = 0;
  }
  bool wildCanAppear() const;          // condiciones para que salga alguien
  uint16_t wildBstCap() const {
    // La ultima insignia LEVANTA el techo en vez de subirlo un escalon mas:
    // has terminado la region, ya no hay zonas que no te toquen. Ajustar el
    // numero hasta que Mewtwo (680) entrara por los pelos seria un apano.
    // Y el techo es el de TU region: cada una reparte su BST de otra forma
    // (ver Region::bstCap).
    // Terminada CUALQUIER region, el techo se levanta del todo y no
    // vuelve a bajar: viajar a una region nueva con el equipo hecho no
    // puede devolverte a la hierba de principiante.
    if (anyRegionDone()) return 0xFFFF;
    return badges >= GYM_COUNT ? 0xFFFF : regionAt(region).bstCap[badges];
  }
  // `fuga` = esta es la tirada que se asoma a la region SIGUIENTE.
  uint8_t wildWeight(int16_t dex, uint8_t hour, bool fuga = false) const;  // 0 = no sale
  // `deTreinador`: sem lendario nenhum —— nem a rolagem dos de regiao
  // terminada, nem os que entram no peso comum com as oito insignias.
  // Um lendario e um encontro, nao o bicho de um treinador qualquer.
  int16_t pickWildSpecies(uint8_t hour, bool deTreinador = false) const;
  // El sorteo APARTE de los legendarios de una region ya terminada.
  // Devuelve 0 si no toca ninguno. Publica para poder medirla.
  int16_t rollLegendary(uint8_t hour) const;
  uint16_t wildLevelFor(int16_t dex) const;
  // Cada cuantos encuentros se asoma uno de la region SIGUIENTE. Uno de
  // diez: suficiente para que la hierba no sea siempre la misma lista y
  // poco para que la region siga teniendo cara propia.
  static const uint8_t WILD_FUGA = 10;
  void rollWild(uint8_t hour);         // la llama tick(); saca uno o no

  // ------------------------------------------------------ gimnasios
  // El porton es CONTABLE y se ve: "3/5". Un desafio que se abre solo, sin
  // que sepas por que, se siente arbitrario; uno que puedes contar da algo
  // que perseguir mientras cuidas al bicho.
  uint8_t gymCount() const { return regionAt(region).count; }
  bool allGymsDone() const { return badges >= gymCount(); }
  uint8_t gymNext() const {
    return badges < gymCount() ? badges : (uint8_t)(gymCount() - 1);
  }
  const GymEntry &gymHere() const { return gymAt(region, gymNext()); }
  bool gymReady() const {
    return !allGymsDone() && gymTrainers >= GYM_TRAINERS;
  }
  // Cual de los tres toca ahora (GYM_TRAINERS = ya estan los tres).
  uint8_t gymTrainerNext() const {
    return gymTrainers < GYM_TRAINERS ? gymTrainers : GYM_TRAINERS;
  }
  void winWild();        // una victoria mas (la llama el cierre del combate)
  void winGymTrainer();  // uno de los tres de la casa, abajo
  void winGym();         // desafio superado: insignia y vuelta a empezar
  bool hasBadge(uint8_t i) const { return i < badges; }
  const char *regionName() const { return regionAt(region).name; }
  // Terminada una region y habiendo otra, la siguiente queda abierta.
  // Ya no es "una decision con precio": viajar no cuesta nada, asi que
  // esto solo dice si hay sitio nuevo al que ir.
  bool canAdvanceRegion() const {
    return allGymsDone() && (uint8_t)(region + 1) < REGION_COUNT;
  }
  bool advanceRegion();
  // Las insignias de una region cualquiera (la de ahora sale de `badges`,
  // que es la que se esta moviendo).
  uint8_t badgesOf(uint8_t r) const {
    if (r >= REGION_COUNT) return 0;
    return r == region ? badges : badgesIn[r];
  }
  bool regionDone(uint8_t r) const {
    return r < REGION_COUNT && badgesOf(r) >= regionAt(r).count;
  }
  bool anyRegionDone() const {
    for (uint8_t r = 0; r < REGION_COUNT; r++)
      if (regionDone(r)) return true;
    return false;
  }

  // TECHO DE NIVEL: el del proximo desafio mas un margen.
  //
  // Se mira en TODAS las regiones y se queda con el mas alto, no solo
  // con el de donde estas. Si bajara al viajar, volver a Kanto con el
  // equipo hecho lo dejaria sin poder pelear a su altura —— que es la
  // misma pena que quitarle las insignias, por la puerta de atras.
  uint16_t levelCap() const {
    if (anyRegionDone()) return LEVEL_CAP;
    uint16_t mejor = 0;
    for (uint8_t r = 0; r < REGION_COUNT; r++) {
      uint8_t b = badgesOf(r), n = regionAt(r).count;
      // Solo cuentan las empezadas, y la de ahora aunque este a cero:
      // es contra lo que vas a pelear ya mismo.
      if (!b && r != region) continue;
      uint16_t c = (uint16_t)gymAt(r, b < n ? b : (uint8_t)(n - 1)).level +
                   GYM_CAP_SLACK;
      if (c > mejor) mejor = c;
    }
    return mejor > LEVEL_CAP ? LEVEL_CAP : mejor;
  }
  // Con el que PELEA. El bicho no pierde ni su nivel ni su EXP: solo se le
  // pide que baje a la altura de la region en la que esta. Es lo que deja
  // traerse el equipo de Kanto a Johto sin arrasarla.
  uint16_t battleLevel() const {
    uint16_t n = level(), t = levelCap();
    return n > t ? t : n;
  }
  bool overCap() const { return level() > levelCap(); }

  bool regionVisited(uint8_t r) const {
    return r < REGION_COUNT && (visitedMask & (uint16_t)(1u << r));
  }

  // TETO PRA SACAR DO BANCO —— e SO pra isso.
  //
  // Nao confundir com levelCap(), que e o de combate e fica com o
  // MAIOR de todas as regioes de proposito: se ele caisse ao
  // viajar, voltar pra Kanto com o time pronto seria a mesma pena
  // que tomar suas insignias.
  //
  // Este aqui e o da regiao onde voce ESTA, e e o que faz uma
  // regiao nova comecar de baixo: o banco e compartilhado, mas
  // aqui so sai quem couber no teto daqui. Cada lider que cai
  // levanta um degrau, e terminar a regiao libera tudo.
  uint16_t bankCapHere() const {
    if (regionDone(region)) return LEVEL_CAP;
    const Region &g = regionAt(region);
    uint8_t b = badges < g.count ? badges : (uint8_t)(g.count - 1);
    uint16_t teto = (uint16_t)gymAt(region, b).level + GYM_CAP_SLACK;
    // E NUNCA abaixo do seu titular. Sem este piso a regra prendia um
    // bicho criado em casa: em Kanto com 0 insignias o teto fica perto
    // de 11, entao guardar um nivel 12 na caixa era perde-lo.
    //
    // O piso nao abre a porta pro veterano que veio de fora: em Johto
    // o seu titular e um inicial recem-chocado, entao o teto continua
    // baixo. So garante que voce sempre possa sacar algo que nao seja
    // mais forte do que o que ja esta jogando aqui.
    if (!isEgg()) {
      uint16_t meu = level();
      if (meu > teto) teto = meu;
    }
    return teto;
  }
  // O cadeadinho: quem passa do teto daqui nao sai da caixa.
  bool bankLocked(const StoredMon &m) const {
    return m.used() && levelOf(m) > bankCapHere();
  }

  bool isEgg() const { return speciesId < 0; }
  uint8_t eggCracks() const { return eggTaps; }
  bool eating() const { return timeLeft(eatUntil) > 0; }
  bool showHeart() const { return timeLeft(heartUntil) > 0; }
  bool evolving() const { return timeLeft(evolveUntil) > 0; }
  float evolveT() const {     // progreso de la animacion de evolucion 0..1
    return 1.0f - (float)timeLeft(evolveUntil) / (float)EVOLVE_ANIM_MS;
  }
  bool canEvolveNow() const;  // condiciones de evolucion cumplidas (lista)
  // Si a ese suplente le toca evolucionar. Separado de hacerlo porque la
  // pantalla necesita preguntarlo para pintar el boton.
  bool canEvolveReserve(uint8_t slot) const;
  // El PRIMER hueco del equipo al que le toca evolucionar, o 0xFF. Lo usa
  // la pantalla de cuidado para sacar su CTA.
  uint8_t reserveReadyToEvolve() const;
  // Y hacerlo. Devuelve true si evoluciono y deja el antes/despues en
  // teamEvoFrom/teamEvoTo.
  bool evolveReserve(uint8_t slot);
  // ---- TROCAS (ESP-NOW; o protocolo esta em troca.h)
  // Quem pode sair: do time, nao o titular (ele E o Pet, nao um registro),
  // ou da caixa ABERTA do banco. O slot: 0..5 = time; TROCA_BANCO|i = o
  // hueco i da caixa aberta (o diario guarda qual caixa em `rsv`).
  bool trocaPodeOferecer(uint8_t slot) const;
  const StoredMon *trocaMeu(uint8_t slot) const;
  // Como o bicho que chegou fica aqui: evolui pela troca (Kadabra; Onix
  // segurando Metal Coat, que se gasta), menos com Everstone.
  static StoredMon trocaChegada(const StoredMon &veio);
  // Ganchos do protocolo. preparar: confere que o meu ainda esta la,
  // calcula `chega` e grava o diario. aplicar: idempotente.
  bool trocaPreparar(TrocaDiario &d);
  bool trocaAplicar(const TrocaDiario &d);
  void trocaDescartar(const TrocaDiario &d);
  // O diario no SD (/troca/diario.bin): o ultimo, em qualquer estado.
  static bool trocaLeDiario(TrocaDiario &d);
  static bool trocaGravaDiario(const TrocaDiario &d);
  // Poe na Pokedex todo bicho do time, do banco e dos times das regioes.
  // Roda no load; publica para os testes.
  void registraOsQueTem();
  void evolve();              // dispara la transformacion (la llama un toque del usuario)
  // Piedra de evolucion. Se salta el NIVEL (ese es el atajo que compras)
  // pero no el cuidado, y deja ELEGIR la rama: es lo unico que hace que un
  // Eevee sea tuyo y no el que al juego le toque sortear.
  bool useStone(uint8_t itemId);
  void becomeSpecies(int16_t next);  // el paso comun de las dos vias
  bool canFarewellNow() const;  // final + 3 dias + 1 desde evolucionar + vinculo
  // Calidad de la herencia, 0..LEGACY_SCORE_MAX. Es PUBLICA y se puede
  // consultar ANTES de confirmar: despedirse es definitivo, y saber lo que
  // vas a recibir a cambio es la diferencia entre una decision y una
  // apuesta a ciegas.
  uint8_t legacyScore() const;
  bool hasLegacyEgg() const { return legacyDex > 0; }
  bool canRunawayNow() const;   // abandono total 1h: lista para escaparse (boton triste)
  // El aviso de abandono, para la pantalla: lo devuelve UNA vez y lo borra.
  bool tomaAvisoAbandono() { bool a = avisoAbandono; avisoAbandono = false; return a; }
  // el usuario decide en un dialogo; "mantener/quedaros" pospone y re-ofrece luego
  bool wantEvolveButton() const { return canEvolveNow() && level() > evoDeclinedLv; }
  bool wantFarewellButton() const { return canFarewellNow() && ageMinutes >= farDeclinedAge; }
  void declineEvolve() { evoDeclinedLv = level(); }              // re-ofrece al subir de nivel
  void declineFarewell() { farDeclinedAge = ageMinutes + 1440; } // re-ofrece dentro de 1 dia
  // primera partida: el jugador elige inicial (Bulbasaur/Charmander/Squirtle)
  bool awaitingStarter() const { return starterPick; }
  void chooseStarter(int16_t dex) { eggTarget = dex; starterPick = false; save(); }
  // Cierra la intro de primera partida. Se guarda al momento: si se apagara
  // justo despues, volver a soltarle los creditos y al profesor seria molesto.
  void finishIntro(bool girl);
  void factoryReset() { prefs.clear(); }  // borra la NVS (test: comando serie WIPE)
  void dbgRunawayReady() { fullness = joy = energy = hygiene = 0; neglectTicks = RUNAWAY_TICKS; avisoCuenta = 0; }  // test
  // El nivel sale de la EXPERIENCIA, no de la edad. Hasta ahora eran la misma
  // variable (`1 + ageMinutes/60`) y por eso un bicho subia de nivel sin hacer
  // nada — lo que hace imposible que uno guardado en la caja tenga nivel
  // propio. La edad sigue existiendo, pero ya solo manda en la eclosion, el
  // cuidado y la despedida.
  //
  // Curva medium-fast, la de siempre: la experiencia del nivel n es n^3. El
  // 100 pide un millon, que cabe de sobra en 32 bits.
  static uint32_t expForLevel(uint16_t n) {
    if (n < 1) n = 1;
    if (n > LEVEL_CAP) n = LEVEL_CAP;
    return (uint32_t)n * n * n;
  }
  uint16_t level() const {
    uint16_t lv = 1;
    while (lv < LEVEL_CAP && expForLevel(lv + 1) <= exp) lv++;
    return lv;
  }
  // cuanto falta para el siguiente, 0..1 (para la barra de la ficha)
  float levelProgress() const {
    uint16_t lv = level();
    if (lv >= LEVEL_CAP) return 1.0f;
    uint32_t a = expForLevel(lv), b = expForLevel(lv + 1);
    return b > a ? (float)(exp - a) / (float)(b - a) : 0.0f;
  }
  // Suma experiencia y devuelve cuantos niveles ha subido (0 si ninguno).
  uint8_t gainExp(uint32_t cuanta);
  // Las dos de arriba, pero para UN HUECO del equipo. Si el hueco es
  // el del principal, caen donde siempre. Hacen falta porque quien
  // pelea puede ser un suplente, y lo que se gana peleando es de
  // quien peleo.
  uint8_t gainExpOf(uint8_t slot, uint32_t cuanta);
  uint8_t gainEVOf(uint8_t slot, uint8_t which, uint8_t cuanto);
  // Lo que da tumbar a un rival. Sin `base_exp` por especie se usa la suma de
  // los stats base, que es con lo que ese numero correlaciona en los juegos.
  static uint32_t expFromFoe(int16_t foeDex, uint16_t foeLevel);
  bool isRegistered(int16_t dex) const {
    return dex >= 1 && dex <= DEX_COUNT &&
           (dexReg[(dex - 1) >> 3] & (1 << ((dex - 1) & 7)));
  }
  // Visto = apareceu na tela OU registrado (o que voce teve, voce viu).
  bool isSeen(int16_t dex) const {
    return isRegistered(dex) ||
           (dex >= 1 && dex <= DEX_COUNT &&
            (dexSeen[(dex - 1) >> 3] & (1 << ((dex - 1) & 7))));
  }
  // Apunta e grava, so na PRIMEIRA vez (chamam-na a cada quadro do combate).
  void markSeen(int16_t dex);
  // Pode cacar este? Visto e ainda nao registrado, e que saia no mato
  // agora: comum com regiao destravada e dentro do teto de BST; lendario
  // com as 8 insignias da regiao (lendarioPodeSair).
  bool podeCacar(int16_t dex, uint8_t hour) const {
    return isSeen(dex) && !isRegistered(dex) &&
           (wildWeight(dex, hour) > 0 || lendarioPodeSair(dex));
  }
  bool lendarioPodeSair(int16_t d) const;
  // Liga (ou com 0 desliga) a cacada. Grava no proximo momento parado.
  void setCaca(int16_t dex) {
    cacaDex = (dex >= 1 && dex <= DEX_COUNT) ? dex : 0;
    pendingSave = true;
  }
  bool isShinyRegistered(int16_t dex) const {
    return dex >= 1 && dex <= DEX_COUNT &&
           (dexShinyReg[(dex - 1) >> 3] & (1 << ((dex - 1) & 7)));
  }
  // `fila` es un indice de FORM_TBL, el que devuelve formRow(dex, forma).
  bool isFormRegistered(int16_t fila) const {
    return fila >= 0 && fila < FORM_COUNT &&
           (dexFormReg[fila >> 3] & (1 << (fila & 7)));
  }
  // `fila` es un indice de MEGA_TBL, el que devuelve megaFormOf().
  bool isMegaRegistered(int16_t fila) const {
    return fila >= 0 && fila < MEGA_COUNT &&
           (dexMegaReg[fila >> 3] & (1 << (fila & 7)));
  }
  // Se apunta al VERLA, no al tenerla: una mega dura lo que el combate y
  // no se queda en el equipo, asi que si solo contara lo propio no habria
  // forma de que la Pokedex la ensenara nunca. Vale tambien la del rival,
  // como en los juegos: haberla visto es lo que registra.
  void registerMega(int16_t fila);
  uint16_t registeredCount() const;
  bool lineHasUnregistered(int16_t base) const;
  uint8_t eggRarity() const;       // rareza del huevo actual (sin revelar especie)
  int16_t pickEggSpecies();        // publica para poder simular tiradas (EGGS)
  uint8_t lowestStat() const { return min(min(fullness, joy), min(energy, hygiene)); }
  PetMood mood() const;
  PetNeed need() const;   // que pide ahora mismo (NEED_NONE si esta bien)
  // progreso de la ceremonia de despedida/escapada, 0..1 (para animarla)
  float ceremonyT() const {
    if (ceremony == CER_NONE) return 0.0f;
    return 1.0f - (float)timeLeft(ceremonyUntil) / (float)CEREMONY_MS;
  }

  // racha / vinculo / medallas / nombre
  void rename(const char *name);
  // Cobra las medallas ganadas y aun sin pagar. Publico porque la
  // pantalla lo llama para ensenar QUE te han dado.
  uint8_t payMedals();
  // Los logros del JUGADOR (Pokedex, combates, megas, regiones).
  // Publico porque la bateria los comprueba uno a uno: que un logro
  // se de cuando toca no se ve mirando la pantalla.
  void jugadorMedals();
  // La REVANCHA ganada, que la apunta el cierre del combate.
  void winRematch() { medals |= MED_REMATCH; checkMedals(); }
  // Solo para MIRAR el cartel (comando de serie CARTEL): ensena la
  // celebracion de esa medalla sin ganarla ni pagarla.
  void celebraMedalla(uint32_t bit, uint32_t ms = 6000) {
    newMedal = bit;
    medalUntil = millis() + ms;
  }
  bool hasMedal(uint32_t m) const { return (medals & m) != 0; }
  // Y con una medalla que ensenar: el cartel la busca en newMedal y si
  // no la encuentra se queda sin texto. hatch() borra newMedal sin
  // tocar el reloj, asi que esto pasaba de verdad —— y acababa en un
  // strlen(NULL) dentro de drawCelebration.
  bool showMedal() const { return timeLeft(medalUntil) > 0 && newMedal; }
  bool showMilestone() const { return timeLeft(milestoneUntil) > 0; }
  int careBonus() const;  // mejora del huevo por racha + vinculo

  // guardado periodico diferido: tick() marca pendiente y el loop lo vuelca
  // cuando la pantalla esta atenuada/apagada (la escritura a flash congela
  // ~1s ambos cores: asi no se ve ni corta el tactil)
  bool savePending() const { return pendingSave; }
  // ultima hora real persistida; sirve para resembrar un RTC que perdio la hora
  uint32_t savedEpoch() { return prefs.getUInt("seen", 0); }
  void flushSave();
  // Guarda YA, haya o no cambios marcados.
  //
  // La usa el SIMULADOR al cerrar. Ahi no sirve flushSave(): el juego
  // solo marca `pendingSave` en unos pocos sitios (escribir en flash
  // congela los dos nucleos casi un segundo, asi que no se hace a cada
  // cambio) y los comandos de depuracion no lo marcan. En la placa da
  // igual, porque el loop sigue y el autoguardado acaba pasando; el
  // simulador, en cambio, se cierra y ahi se acaba todo.
  void saveNow() { save(); }
  // marcar una especie como criada. Normalmente lo hace hatch() solo, pero el
  // comando de consola REG lo necesita: sin el, para ver la galeria con algo
  // dentro habria que criar 151 bichos de verdad.
  // OJO con los parametros: son los del bicho QUE SE REGISTRA, no los del
  // Pet. Antes esto leia `shiny` del propio Pet, asi que teniendo uno
  // shiny TODO lo que registrabas quedaba marcado como shiny.
  void registerSpecies(int16_t dex, bool esShiny, uint8_t forma = 0);

private:
  Preferences prefs;
  uint32_t lastTick = 0;
  uint32_t eatUntil = 0;
  uint32_t heartUntil = 0;
  uint32_t evolveUntil = 0;
  int16_t eggTarget = 1;       // dex oculto que saldra del huevo
  bool eggShiny = false;       // sorpresa sorteada al crear el huevo
  uint8_t eggTaps = 0;
  uint8_t mistakeCooldown = 0;
  uint8_t ticksSinceSave = 0;
  bool pendingSave = false;     // guardado periodico pendiente de volcar
  uint16_t evoDeclinedLv = 0;   // "mantener forma": no ofrecer evolucion hasta subir de nivel
  uint32_t farDeclinedAge = 0;  // "quedaros juntos": no ofrecer despedida hasta esta edad
  bool starterPick = false;     // primera partida: esperando que el jugador elija inicial
  uint8_t neglectTicks = 0;
  uint8_t avisoCuenta = 0;      // minutos de abandono dentro del tramo de aviso
  bool avisoAbandono = false;   // toca llamar (ver ABANDONO_AVISO_TICKS)
  uint16_t goodTicks = 0;  // racha bien cuidado: forja la DEF
  uint32_t ceremonyUntil = 0;
  uint8_t bondToday = 0;       // tope diario de subida de vinculo
  uint32_t medalUntil = 0;     // celebracion de medalla en pantalla
  uint32_t milestoneUntil = 0; // celebracion de hito de racha

  uint32_t today() const { return lastSeenEpoch ? lastSeenEpoch / 86400 : 0; }
  void registerCare();   // primer cuidado del dia: racha + vinculo
  void addBond(uint8_t amt);
  void checkMedals();
  void premiaMedals(uint32_t before);  // contar, celebrar y pagar
  void tick();
  void hatch();
  void save();
  // Un putBytes que comprueba lo que escribio. Los de save() pasan por
  // aqui; si alguno no escribe lo que se le pidio, queda apuntado.
  size_t guarda(const char *clave, const void *datos, size_t n);
  void load();
  static uint8_t clamp100(int v) { return v < 0 ? 0 : (v > 100 ? 100 : v); }
};
