// Tests de la logica de la mascota (pet.cpp) sobre los shims del host.
#include "framework.h"
#include "shim/Arduino.h"
#include "../pet.h"
#include "../tms_v304.h"
#include "../dex.h"
#include "../moves.h"
#include "../battle.h"   // disobeyChance: el vinculo del Pet se lee desde aqui
#include <string.h>

// ---------------------------------------------------------------- helpers
static void advance(Pet &p, uint32_t minutes) {
  for (uint32_t i = 0; i < minutes; i++) {
    mockAdvanceMillis(PET_TICK_MS);
    p.update(millis());
  }
}

// mascota recien eclosionada de la especie pedida, con la NVS limpia
static void makePet(Pet &p, int16_t dex) {
  mockNvsReset();
  mockSetMillis(0);
  p.begin();
  if (p.awaitingStarter()) p.chooseStarter(dex);
  p.eggTap();
  p.eggTap();
  p.eggTap();  // 3 toques = eclosion inmediata
}

// El nivel ya no sale de la edad: para dejar a un bicho en un nivel hay que
// darle la EXPERIENCIA, no envejecerlo.
static void setLevel(Pet &p, uint16_t n) { p.exp = Pet::expForLevel(n); }

static void setStats(Pet &p, uint8_t f, uint8_t j, uint8_t e, uint8_t h) {
  p.fullness = f; p.joy = j; p.energy = e; p.hygiene = h;
}

// ---------------------------------------------------------------- stats
// La MT que regala un lider sale de SU tipo.
//
// Antes rollTM() solo miraba "lo puede aprender y no la tiene" y sorteaba
// uniforme, asi que Brock podia soltar Seduccion. Lo canto el dueno
// jugando, no un test: de ahi este.
//
// Se tira muchas veces a proposito. Una sola tirada puede acertar el tipo
// por casualidad y dejaria pasar un sorteo que no filtra nada.
// El cartel de medalla no puede quedarse sin medalla que ensenar: el que
// lo pinta busca el bit en newMedal y, si no lo encuentra, se queda con el
// puntero del texto a nullptr y acaba en strlen(NULL). Lo encontro el gdb
// en el replay de los guiones, y la via es esta: ganar la medalla y que el
// huevo nazca dentro de los 4 segundos que el cartel dura.
// ------------------------------------------------------------- cajas
//
// La SD de MENTIRA de estos tests es una carpeta propia, no
// ../tools/sdcard (que es el default del shim): ahi vive la imagen que se
// instala en la placa de verdad, y unos tests que le escriben /boxes
// dentro se la llevarian puesta al cacharro.
#include <FS.h>
#include <SD_MMC.h>
#include <filesystem>

const char *mockSdRootGet();
// O /save do SD falso (as regioes no SD, leva 44).
static std::string svCaminho(const char *rel) {
  return std::string(mockSdRootGet()) + "/save/" + rel;
}
static bool svExiste(const char *rel) {
  std::error_code ec;
  return std::filesystem::exists(svCaminho(rel), ec);
}
static void svApagaDir() {
  std::error_code ec;
  std::filesystem::remove_all(std::string(mockSdRootGet()) + "/save", ec);
}

static void sdDePruebas() {
  // `build` ya existe (lo crea el Makefile) y es de usar y tirar.
  mockSdRoot("build");
  // Y las CAJAS se borran. Viven en ficheros, asi que sobrevivian de una
  // corrida a la siguiente: `una_caja_guardada_vuelve_igual` abre la caja
  // 3 esperandola vacia y la corrida ANTERIOR se la habia dejado llena.
  // O sea que la bateria solo daba verde la primera vez despues de un
  // `clean` —— un verde que dependia de quien hubiera corrido antes no
  // dice nada. Se vio al correrla dos veces seguidas.
  std::error_code ec;
  std::filesystem::remove_all("build/boxes", ec);
}

static StoredMon unBicho(int16_t dex, uint8_t bond) {
  StoredMon m;
  m.species = dex;
  m.exp = 1000;
  m.hpLeft = 20;
  m.bond = bond;
  return m;
}

// Una gravacion que falla tiene que NOTARSE. Los once putBytes de save()
// no miraban el retorno: con la particion de NVS sin sitio, putBytes
// devuelve 0, no se guarda nada y el juego sigue igual —— el jugador solo
// lo descubre cuando el progreso vuelve atras solo.
//
// (El dueno conto que "cuando enxeu os 30, parou de capturar", con un
// banco que tenia 60 huecos. El blob del banco era el mas gordo de todos,
// 3.120 bytes, y con las cajas bajo a 1.560. No puedo demostrar que fuera
// eso; lo que si no puede seguir es que falle sin decirlo.)
TEST(guardado, un_guardado_que_falla_no_puede_pasar_callado) {
  Pet p;
  makePet(p, 1);
  p.bank[0] = unBicho(25, 77);

  // una NVS que no acepta nada grande
  mockNvsTopeBlob(8);
  p.addToBank(unBicho(143, 5));   // addToBank llama a save()
  CHECK_MSG(p.guardadoFallo,
            "la NVS rechazo los bloques y save() no se entero");
  CHECK_MSG(p.bloquesFallados > 0, "no conto ni uno");

  // y con sitio, no se queja
  mockNvsTopeBlob(0);
  p.addToBank(unBicho(6, 9));
  CHECK_MSG(!p.guardadoFallo,
            "con sitio de sobra sigue diciendo que fallo");
}


// Una caja que se guarda y se vuelve a abrir tiene que volver IGUAL. Es
// lo minimo, y es lo que no se podia tener mientras el banco entero vivia
// en una sola entrada de la NVS.
// La bolsa ordenada. La tabla de objetos NO esta en ningun orden (empieza
// Leftovers, Sitrus Berry, Oran Berry...), que es la ordenacion en que
// fueron escritos: con treinta objetos en un bolsillo, buscar uno es
// leerlos todos.
// LA MIGRACION. Una partida de antes trae el banco entero (60 huecos) en
// una sola entrada de la NVS; ahora son 30 por caja y el resto vive en la
// SD. Esto corre sobre la partida REAL de quien actualice, asi que un
// bicho perdido aqui es un bicho perdido de verdad.
// Si la caja 1 NO se puede escribir, el banco viejo no se toca.
//
// Esto no es hipotetico: en la placa paso. setup() montaba la tarjeta
// DESPUES de cargar el Pet, la escritura fallo callada y el primer save
// dejo "bank" en 30 registros encima de los 60. La orden ya esta
// arreglada, pero no puede ser la unica defensa —— una tarjeta ausente o
// llena haria lo mismo.
TEST(cajas, si_la_caja_no_se_puede_escribir_no_se_borra_nada) {
  mockNvsReset();
  mockSetMillis(0);
  {
    StoredMon viejo[60];
    for (uint8_t i = 0; i < 60; i++) {
      viejo[i] = StoredMon();
      viejo[i].species = (int16_t)(i + 1);
      viejo[i].exp = 1000;
    }
    Preferences pr;
    pr.begin("tamapoke", false);
    pr.putBool("init", true);
    pr.putBytes("bank", viejo, sizeof(viejo));
    pr.end();
  }

  // La "SD" apunta a una carpeta que no existe: mkdir y open fallan,
  // que es justo lo que pasa con la tarjeta sin montar.
  mockSdRoot("build/sd-que-no-esta/ni-de-lejos");

  Pet p;
  p.begin();
  CHECK_MSG(p.bancoViejoPendiente,
            "la migracion se dio por hecha sin haber podido escribir");

  // ...y ahora el juego guarda, como hace todo el rato
  p.hpLeft = p.maxHp();
  p.toBank(0);                 // cualquier cosa que llame a save()

  // la clave tiene que seguir con los SESENTA
  StoredMon leido[60];
  Preferences pr;
  pr.begin("tamapoke", true);
  size_t bytes = pr.getBytes("bank", leido, sizeof(leido));
  pr.end();
  CHECK_MSG(bytes == sizeof(leido),
            std::string("el banco viejo paso de 60 registros a ") +
                std::to_string(bytes / sizeof(StoredMon)) +
                ": se escribieron 30 encima y la otra mitad se perdio");
  int vivos = 0;
  for (uint8_t i = 0; i < 60; i++)
    if (leido[i].species == (int16_t)(i + 1)) vivos++;
  CHECK_MSG(vivos == 60,
            std::string("solo quedan ") + std::to_string(vivos) +
                " de los 60 en la NVS");

  sdDePruebas();               // devolver la SD buena a los demas tests
}

TEST(cajas, el_banco_viejo_de_sesenta_no_pierde_a_nadie) {
  sdDePruebas();
  mockNvsReset();
  mockSetMillis(0);

  // Se escribe a mano un banco VIEJO: 60 registros bajo la clave "bank",
  // tal cual lo dejaba el firmware anterior.
  {
    StoredMon viejo[60];
    for (uint8_t i = 0; i < 60; i++) {
      viejo[i] = StoredMon();
      viejo[i].species = (int16_t)(i + 1);   // 1..60, todos distintos
      viejo[i].exp = 1000;
      viejo[i].bond = (uint8_t)i;
    }
    Preferences pr;
    pr.begin("tamapoke", false);
    // `init` es lo que le dice a begin() que hay partida que CARGAR; sin
    // el llama a newEgg() y no mira el banco siquiera. La primera version
    // de este test no lo ponia y daba "perdio los 60" —— que era verdad,
    // pero del montaje, no del codigo.
    pr.putBool("init", true);
    pr.putBytes("bank", viejo, sizeof(viejo));
    pr.end();
  }

  // ...y se borran las cajas de la SD, para que la migracion no se
  // encuentre una caja 1 de una corrida anterior y parezca que funciona.
  SD_MMC.remove("/boxes/box00.bin");
  SD_MMC.remove("/boxes/box01.bin");

  Pet p;
  p.begin();

  bool visto[61] = { false };
  for (uint8_t caja = 0; caja < 2; caja++) {
    CHECK_MSG(p.boxGo(caja), "no se pudo abrir la caja tras migrar");
    for (uint8_t i = 0; i < BANK_SLOTS; i++) {
      if (!p.bank[i].used()) continue;
      int16_t e = p.bank[i].species;
      CHECK_MSG(e >= 1 && e <= 60,
                std::string("aparecio una especie que no estaba: ") +
                    std::to_string(e));
      CHECK_MSG(!visto[e],
                std::string("el bicho ") + std::to_string(e) +
                    " salio DOS veces: la migracion lo duplico");
      visto[e] = true;
    }
  }
  int faltan = 0;
  for (int e = 1; e <= 60; e++) if (!visto[e]) faltan++;
  CHECK_MSG(faltan == 0,
            std::string("la migracion del banco viejo perdio ") +
                std::to_string(faltan) +
                " de los 60: eso es perdida de datos en una partida real");
}

TEST(bolsa, la_bolsa_se_ordena_por_nombre_y_por_tipo) {
  Pet p;
  makePet(p, 1);

  // se le dan todos los objetos de un bolsillo con mas de tres
  uint8_t bolsillo = 0xFF;
  for (uint8_t b = 0; b < BP_COUNT && bolsillo == 0xFF; b++) {
    uint8_t n = 0;
    for (uint8_t i = 1; i < ITEM_COUNT; i++)
      if (ITEM_TBL[i].pocket == b) n++;
    if (n >= 4) bolsillo = b;
  }
  CHECK_MSG(bolsillo != 0xFF, "ningun bolsillo tiene cuatro objetos");
  for (uint8_t i = 1; i < ITEM_COUNT; i++)
    if (ITEM_TBL[i].pocket == bolsillo) p.bag[i] = 1;

  uint8_t n = p.pocketCount(bolsillo);
  CHECK(n >= 4);

  // 1: por NOMBRE
  p.bagSort = 1;
  for (uint8_t i = 1; i < n; i++) {
    const char *a = ITEM_TBL[p.pocketItem(bolsillo, (uint8_t)(i - 1))].name;
    const char *b = ITEM_TBL[p.pocketItem(bolsillo, i)].name;
    CHECK_MSG(strcmp(a, b) <= 0,
              std::string("fuera de orden alfabetico: '") + a + "' antes de '" +
                  b + "'");
  }

  // 2: por TIPO, y dentro del tipo por nombre
  p.bagSort = 2;
  for (uint8_t i = 1; i < n; i++) {
    const ItemEntry &a = ITEM_TBL[p.pocketItem(bolsillo, (uint8_t)(i - 1))];
    const ItemEntry &b = ITEM_TBL[p.pocketItem(bolsillo, i)];
    CHECK_MSG(a.fx < b.fx || (a.fx == b.fx && strcmp(a.name, b.name) <= 0),
              std::string("fuera de orden por tipo: ") + a.name + " (fx " +
                  std::to_string(a.fx) + ") antes de " + b.name + " (fx " +
                  std::to_string(b.fx) + ")");
  }

  // y los tres modos tienen que ensenar los MISMOS objetos: ordenar no
  // puede esconder nada, que seria la peor forma de "organizar".
  for (uint8_t modo = 0; modo < Pet::BAG_SORTS; modo++) {
    p.bagSort = modo;
    CHECK_EQ((int)p.pocketCount(bolsillo), (int)n);
    bool visto[ITEM_COUNT] = { false };
    for (uint8_t i = 0; i < n; i++) {
      uint8_t id = p.pocketItem(bolsillo, i);
      CHECK_MSG(id != 0 && !visto[id],
                std::string("el modo ") + std::to_string(modo) +
                    " repitio o se salto un objeto");
      visto[id] = true;
    }
  }
}

TEST(cajas, una_caja_guardada_vuelve_igual) {
  sdDePruebas();
  Pet p;
  makePet(p, 1);
  p.bank[0] = unBicho(25, 77);
  p.bank[5] = unBicho(143, 12);
  CHECK_MSG(p.boxGo(3), "no se pudo cambiar de caja");
  CHECK_MSG(!p.bank[0].used(), "la caja nueva tendria que estar vacia");
  p.bank[2] = unBicho(6, 55);
  CHECK_MSG(p.boxGo(0), "no se pudo volver a la caja 0");
  CHECK_MSG(p.bank[0].species == 25 && p.bank[0].bond == 77,
            "la caja 0 no volvio como se dejo");
  CHECK_MSG(p.bank[5].species == 143, "falta el segundo de la caja 0");
  CHECK_MSG(p.boxGo(3), "no se pudo volver a la 3");
  CHECK_MSG(p.bank[2].species == 6, "la caja 3 no volvio como se dejo");
}

// LA FIRMA. El cabecero lleva el sizeof de StoredMon porque esa
// estructura ya crecio una vez (48 -> 52) y un fichero de registros de
// tamano fijo que cambia de tamano se lee entero desplazado, en silencio.
// Con la firma, una caja que no cuadre se ignora en vez de leerse torcida.
TEST(cajas, una_caja_con_otra_firma_no_se_lee) {
  sdDePruebas();
  Pet p;
  makePet(p, 1);
  p.bank[0] = unBicho(25, 77);
  CHECK(p.boxGo(1));          // guarda la 0 en la SD
  CHECK(p.boxGo(0));
  CHECK_MSG(p.bank[0].species == 25, "montaje: la caja 0 no se guardo");

  // se le estropea el cabecero a mano, como si lo hubiera escrito una
  // version con otro StoredMon
  CHECK(p.boxGo(1));
  {
    File f = SD_MMC.open("/boxes/box00.bin", FILE_READ);
    CHECK_MSG((bool)f, "no esta el fichero de la caja 0");
    static uint8_t todo[4096];
    int n = f.read(todo, sizeof(todo));
    f.close();
    CHECK(n > (int)sizeof(BoxHeader));
    BoxHeader *h = (BoxHeader *)todo;
    h->monSize = (uint8_t)(sizeof(StoredMon) + 1);   // "creció un byte"
    SD_MMC.remove("/boxes/box00.bin");
    File w = SD_MMC.open("/boxes/box00.bin", FILE_WRITE);
    CHECK((bool)w);
    w.write(todo, n);
    w.close();
  }
  CHECK(p.boxGo(0));
  CHECK_MSG(!p.bank[0].used(),
            "se leyo una caja escrita con otro tamano de StoredMon: cada "
            "bicho saldria con los bytes del siguiente");
}

// El banco ya no se llena con 60: cuando la caja abierta se llena, la
// captura se va a la siguiente y esa se abre, como en los juegos.
TEST(cajas, cuando_una_caja_se_llena_se_abre_la_siguiente) {
  sdDePruebas();
  Pet p;
  makePet(p, 1);
  CHECK(p.boxGo(0));
  for (uint8_t i = 0; i < BANK_SLOTS; i++) p.bank[i] = unBicho(16, 10);
  CHECK_MSG(p.addToBank(unBicho(25, 99)),
            "con la caja llena la captura se perdio: hay 31 cajas mas");
  CHECK_MSG(p.boxOpen != 0, "se quedo en la caja llena");
  bool esta = false;
  for (uint8_t i = 0; i < BANK_SLOTS; i++)
    if (p.bank[i].species == 25 && p.bank[i].bond == 99) esta = true;
  CHECK_MSG(esta, "no aparecio en la caja que se abrio");
  // y la caja 0 sigue entera
  CHECK(p.boxGo(0));
  uint8_t n = 0;
  for (uint8_t i = 0; i < BANK_SLOTS; i++) if (p.bank[i].used()) n++;
  CHECK_EQ((int)n, (int)BANK_SLOTS);
}

TEST(cartel, el_cartel_de_medalla_nunca_se_queda_sin_medalla) {
  Pet p;
  makePet(p, 1);
  p.celebraMedalla(1);                       // cartel en marcha
  CHECK_MSG(p.showMedal(), "el cartel no arranco");
  CHECK_MSG(p.newMedal != 0, "arranco sin medalla");

  // Esto es lo que le hace hatch() al nacer: borra la medalla. (hatch() es
  // privado, y no se abre por un test; lo que importa es el estado que
  // deja, y es exactamente este.)
  p.newMedal = 0;
  CHECK_MSG(!p.showMedal(),
            "el cartel sigue en pie con newMedal a 0: quien lo pinte no "
            "encuentra el nombre de la medalla, se queda con el puntero a "
            "nullptr y llama a strlen(NULL)");
}

TEST(mt, la_mt_del_lider_es_de_su_tipo) {
  Pet p;
  p.speciesId = 6;                     // Charizard: aprende MT de sobra
  p.exp = Pet::expForLevel(50);

  // cuantas MT de tipo ROCA puede aprender
  uint16_t deRoca = 0;
  for (uint16_t i = 0; i < TM_COUNT; i++)
    if (tmFits(p.speciesId, i) && MOVE_TBL[tmMove(i)].type == T_ROCK) deRoca++;
  CHECK_MSG(deRoca > 0, "Charizard tendria que aprender alguna MT de Roca; "
                        "sin eso este test no prueba nada");

  for (int t = 0; t < 200; t++) {
    int16_t tm = p.rollTM(T_ROCK);
    CHECK_MSG(tm >= 0, "no salio ninguna MT");
    CHECK_MSG(MOVE_TBL[tmMove((uint16_t)tm)].type == T_ROCK,
              std::string("salio una MT que no es de Roca: ") +
              MOVE_TBL[tmMove((uint16_t)tm)].name);
  }
}

// Y si no puede aprender NINGUNA de ese tipo, igual se lleva premio.
TEST(mt, sin_ninguna_de_su_tipo_el_lider_regala_otra) {
  Pet p;
  p.speciesId = 6;
  p.exp = Pet::expForLevel(50);

  // un tipo del que Charizard no aprenda ninguna MT; si aprende de todos,
  // el test se salta solo en vez de mentir
  int8_t vacio = -1;
  for (uint8_t ti = 1; ti < T_COUNT; ti++) {
    uint16_t n = 0;
    for (uint16_t i = 0; i < TM_COUNT; i++)
      if (tmFits(p.speciesId, i) && MOVE_TBL[tmMove(i)].type == ti) n++;
    if (!n) { vacio = (int8_t)ti; break; }
  }
  if (vacio < 0) return;               // no hay hueco: nada que probar
  int16_t tm = p.rollTM((uint8_t)vacio);
  CHECK_MSG(tm >= 0, "se quedo sin premio por no haber MT de ese tipo");
}

TEST(stats, los_seis_stats_existen_y_crecen_con_el_nivel) {
  Pet p; makePet(p, 6);          // Charizard: 78/84/78/109/85/100
  uint16_t a1 = p.atkStat(), s1 = p.spAtkStat(), h1 = p.maxHp();
  CHECK(a1 > 0); CHECK(p.defStat() > 0); CHECK(p.speStat() > 0);
  CHECK(s1 > 0); CHECK(p.spDefStat() > 0); CHECK(h1 > 0);
  // Charizard pega mas fuerte por el lado especial que por el fisico
  CHECK(s1 > a1);
  setLevel(p, 11);
  CHECK(p.atkStat() > a1);
  CHECK(p.spAtkStat() > s1);
  CHECK(p.maxHp() > h1);
}

TEST(stats, un_huevo_no_tiene_stats) {
  Pet p; mockNvsReset(); mockSetMillis(0);
  p.begin();
  if (p.awaitingStarter()) p.chooseStarter(1);
  CHECK(p.isEgg());
  CHECK_EQ((int)p.atkStat(), 0);
  CHECK_EQ((int)p.spAtkStat(), 0);
  CHECK_EQ((int)p.spDefStat(), 0);
  CHECK_EQ((int)p.maxHp(), 0);
}

TEST(stats, los_ps_tienen_cuenta_propia_y_CRECEN_CON_EL_NIVEL) {
  // Los PS no salen del mismo calcStat que el resto: llevan la cuenta de los
  // juegos (la base pesa el doble, mas un suelo de nivel+10), para que un
  // bicho de nivel 1 no lo borre un golpe de 90 de potencia.
  //
  // Lo que fija este test es que CREZCAN CON EL NIVEL, que es justo lo que
  // estaba roto: la cuenta vieja era `bHp*2 + nivel*2 + 10`, que escala con la
  // BASE y casi nada con el nivel. A nivel 11 un Snorlax tenia 352 PS (en los
  // juegos tiene 59) y hasta un golpe muy eficaz tardaba ocho turnos. El dano
  // estaba bien; el deposito estaba inflado.
  //
  // El test viejo comprobaba `maxHp() > 300` a nivel 1: afirmaba el bug.
  Pet p; makePet(p, 143);        // Snorlax, 160 de PS base
  Pet q; makePet(q, 129);        // Magikarp, 20 de PS base
  uint16_t p1 = p.maxHp(), q1 = q.maxHp();
  CHECK_MSG(p1 >= 10 && q1 >= 10, "a nivel 1 hay suelo: nadie cae de un toque");
  CHECK_MSG(p1 > q1, "mas PS base, mas PS");

  setLevel(p, 50); p.hpSync();
  setLevel(q, 50); q.hpSync();
  CHECK_MSG(p.maxHp() > (uint16_t)(p1 * 5), "los PS crecen con el nivel");
  CHECK_MSG(p.maxHp() > (uint16_t)(q.maxHp() * 2), "y la base sigue pesando");
}

// ---------------------------------------------------------------- peticiones
TEST(need, contento_no_pide_nada) {
  Pet p; makePet(p, 1);
  setStats(p, 80, 80, 80, 100);
  CHECK_EQ((uint8_t)p.need(), (uint8_t)NEED_NONE);
}

TEST(need, avisa_justo_por_debajo_del_umbral) {
  Pet p; makePet(p, 1);
  setStats(p, NEED_WARN, 80, 80, 100);
  CHECK_EQ((uint8_t)p.need(), (uint8_t)NEED_NONE);   // en el umbral, todavia no
  setStats(p, NEED_WARN - 1, 80, 80, 100);
  CHECK_EQ((uint8_t)p.need(), (uint8_t)NEED_FOOD);   // uno por debajo, ya pide
}

TEST(need, pide_lo_que_peor_tiene) {
  Pet p; makePet(p, 1);
  setStats(p, 90, 10, 90, 90);
  CHECK_EQ((uint8_t)p.need(), (uint8_t)NEED_PLAY);
  setStats(p, 90, 90, 90, 8);
  CHECK_EQ((uint8_t)p.need(), (uint8_t)NEED_CLEAN);
  setStats(p, 90, 90, 5, 90);
  CHECK_EQ((uint8_t)p.need(), (uint8_t)NEED_ENERGY);
  setStats(p, 4, 90, 90, 90);
  CHECK_EQ((uint8_t)p.need(), (uint8_t)NEED_FOOD);
}

TEST(need, en_empate_manda_el_orden_de_urgencia) {
  Pet p; makePet(p, 1);
  setStats(p, 10, 10, 10, 10);
  CHECK_EQ((uint8_t)p.need(), (uint8_t)NEED_FOOD);   // hambre primero
  setStats(p, 90, 10, 10, 10);
  CHECK_EQ((uint8_t)p.need(), (uint8_t)NEED_CLEAN);  // luego higiene
  setStats(p, 90, 10, 10, 90);
  CHECK_EQ((uint8_t)p.need(), (uint8_t)NEED_PLAY);   // luego animo
}

TEST(need, callado_durmiendo_o_de_huevo) {
  Pet p; makePet(p, 1);
  setStats(p, 5, 5, 5, 5);
  p.toggleLight();                                   // a dormir
  CHECK_EQ((uint8_t)p.need(), (uint8_t)NEED_NONE);
  p.toggleLight();                                   // despierta: vuelve a pedir
  CHECK_EQ((uint8_t)p.need(), (uint8_t)NEED_FOOD);

  Pet h; mockNvsReset(); mockSetMillis(0);
  h.begin();
  if (h.awaitingStarter()) h.chooseStarter(1);
  CHECK(h.isEgg());
  CHECK_EQ((uint8_t)h.need(), (uint8_t)NEED_NONE);
}

TEST(need, pedir_llega_antes_que_entristecerse) {
  // entre NEED_WARN y el 25 de MOOD_SAD hay una franja donde ya pide pero
  // todavia esta contento: ese es justo el aviso util.
  Pet p; makePet(p, 1);
  setStats(p, 30, 90, 90, 90);
  CHECK_EQ((uint8_t)p.need(), (uint8_t)NEED_FOOD);
  CHECK_EQ((uint8_t)p.mood(), (uint8_t)MOOD_HAPPY);
}

// ---------------------------------------------------------------- huevo
TEST(egg, primera_partida_pide_inicial) {
  mockNvsReset();
  Pet p;
  p.begin();
  CHECK(p.isEgg());
  CHECK(p.awaitingStarter());
  CHECK_EQ(p.eggCracks(), (uint8_t)0);
  CHECK_EQ(p.registeredCount(), (uint16_t)0);
  CHECK_EQ(p.ageMinutes, (uint32_t)0);
}

// regresion v1.3: el huevo eclosionaba solo a los 3 min mientras el jugador
// aun estaba eligiendo inicial, y se perdia la eleccion
TEST(egg, no_eclosiona_mientras_se_elige_inicial) {
  mockNvsReset();
  Pet p;
  p.begin();
  CHECK(p.awaitingStarter());
  advance(p, 600);  // 10 horas de juego
  CHECK_MSG(p.isEgg(), "el huevo NO debe eclosionar durante la eleccion");
  CHECK_EQ(p.ageMinutes, (uint32_t)0);
  p.chooseStarter(7);
  CHECK(!p.awaitingStarter());
  advance(p, 3);
  CHECK(!p.isEgg());
  CHECK_EQ(p.speciesId, (int16_t)7);
}

TEST(egg, eclosiona_sola_a_los_3_minutos) {
  mockNvsReset();
  Pet p;
  p.begin();
  p.chooseStarter(1);
  advance(p, 2);
  CHECK(p.isEgg());
  advance(p, 1);
  CHECK(!p.isEgg());
  CHECK_EQ(p.speciesId, (int16_t)1);
}

TEST(egg, tres_toques_eclosionan) {
  mockNvsReset();
  Pet p;
  p.begin();
  p.chooseStarter(4);
  p.eggTap();
  CHECK_EQ(p.eggCracks(), (uint8_t)1);
  CHECK(p.isEgg());
  p.eggTap();
  CHECK_EQ(p.eggCracks(), (uint8_t)2);
  CHECK(p.isEgg());
  p.eggTap();
  CHECK(!p.isEgg());
  CHECK_EQ(p.speciesId, (int16_t)4);
}

TEST(egg, eclosionar_sortea_genes_y_resetea_al_individuo) {
  Pet p;
  makePet(p, 4);
  CHECK_RANGE(p.geneAtk, 90, 110);
  CHECK_RANGE(p.geneDef, 90, 110);
  CHECK_RANGE(p.geneSpe, 90, 110);
  CHECK_EQ(p.trAtk, (uint8_t)0);
  CHECK_EQ(p.trDef, (uint8_t)0);
  CHECK_EQ(p.trSpe, (uint8_t)0);
  CHECK_EQ(p.bond, (uint8_t)BOND_AT_BIRTH);   // nace haciendote caso, no a cero
  CHECK_EQ(p.berryKnown, false);
  CHECK_STREQ(p.nick, "");
  CHECK_MSG(p.isRegistered(4), "criar = registrar en la pokedex");
  CHECK_EQ(p.registeredCount(), (uint16_t)1);
}

TEST(egg, tocar_huevo_no_hace_nada_si_ya_eclosiono) {
  Pet p;
  makePet(p, 4);
  uint8_t cracks = p.eggCracks();
  p.eggTap();
  CHECK_EQ(p.eggCracks(), cracks);
  CHECK_EQ(p.speciesId, (int16_t)4);
}

TEST(egg, rareza_del_huevo_es_valida) {
  mockNvsReset();
  Pet p;
  p.begin();
  CHECK_RANGE(p.eggRarity(), (uint8_t)R_COMUN, (uint8_t)R_LEGENDARIO);
}

// ---------------------------------------------------------------- comida
TEST(feed, baya_normal_sube_25_y_topa_en_100) {
  Pet p;
  makePet(p, 4);
  p.fullness = 50;
  uint8_t notLoved = (uint8_t)((p.speciesId + 1) % 3);
  p.feedBerry(notLoved);
  CHECK_EQ(p.fullness, (uint8_t)75);
  p.feedBerry(notLoved);
  CHECK_EQ(p.fullness, (uint8_t)100);  // 100 en vez de 105
}

TEST(feed, baya_favorita_sube_mas_y_da_corazon) {
  Pet p;
  makePet(p, 4);  // 4 % 3 == 1 -> le gusta la azul
  CHECK(p.lovesBerry(1));
  CHECK(!p.lovesBerry(0));
  CHECK(!p.lovesBerry(2));
  p.fullness = 10;
  p.joy = 10;
  p.feedBerry(1);
  CHECK_EQ(p.fullness, (uint8_t)45);
  CHECK_EQ(p.joy, (uint8_t)20);
  CHECK(p.berryKnown);
  CHECK(p.showHeart());
  CHECK(p.eating());
}

TEST(feed, chuche_engorda) {
  Pet p;
  makePet(p, 4);
  p.fullness = 50;
  p.joy = 50;
  p.feedCandy();
  CHECK_EQ(p.fullness, (uint8_t)60);
  CHECK_EQ(p.joy, (uint8_t)62);
  CHECK_EQ(p.weight, (uint8_t)12);
}

TEST(feed, dormido_o_huevo_no_come) {
  Pet p;
  makePet(p, 4);
  p.fullness = 40;
  p.toggleLight();  // a dormir
  CHECK(p.sleeping);
  p.feedBerry(1);
  p.feedCandy();
  CHECK_EQ(p.fullness, (uint8_t)40);

  Pet e;
  mockNvsReset();
  e.begin();
  e.chooseStarter(4);
  e.fullness = 40;
  e.feedBerry(0);
  CHECK_EQ(e.fullness, (uint8_t)40);
}

// ---------------------------------------------------------------- tick
TEST(tick, despierto_baja_comida_2_y_energia_1_por_minuto) {
  Pet p;
  makePet(p, 4);
  setStats(p, 100, 100, 100, 100);
  p.poops = 0;
  mockForceRandom(99);  // random(100) < 15 falso: sin cacas
  advance(p, 10);
  mockClearForcedRandom();
  CHECK_EQ(p.fullness, (uint8_t)80);
  CHECK_EQ(p.energy, (uint8_t)90);
  CHECK_EQ(p.hygiene, (uint8_t)90);
  CHECK_EQ(p.joy, (uint8_t)90);
  CHECK_EQ(p.ageMinutes, (uint32_t)10);
  CHECK_EQ(p.poops, (uint8_t)0);
}

TEST(tick, las_barras_nunca_bajan_de_cero) {
  Pet p;
  makePet(p, 4);
  setStats(p, 5, 3, 2, 1);
  advance(p, 30);
  CHECK_EQ(p.fullness, (uint8_t)0);
  CHECK_EQ(p.joy, (uint8_t)0);
  CHECK_EQ(p.energy, (uint8_t)0);
  CHECK_EQ(p.hygiene, (uint8_t)0);
}

TEST(tick, dormir_recupera_energia_con_suelos) {
  Pet p;
  makePet(p, 4);
  setStats(p, 100, 100, 10, 100);
  p.toggleLight();
  advance(p, 20);
  CHECK_MSG(p.energy == 100, "durmiendo la energia sube 6/min hasta 100");
  CHECK_MSG(p.fullness > 80, "durmiendo el hambre baja ~4x mas lento");
  advance(p, 2000);  // dormir muchisimo: los suelos protegen
  CHECK_MSG(p.fullness >= 30, "suelo de comida durmiendo = 30");
  CHECK_MSG(p.joy >= 35, "suelo de felicidad durmiendo = 35");
  CHECK_MSG(p.hygiene >= 45, "suelo de higiene durmiendo = 45");
  CHECK_EQ(p.careMistakes, (uint8_t)0);
}

TEST(tick, sobrepeso_se_quema_solo) {
  Pet p;
  makePet(p, 4);
  p.weight = 30;
  advance(p, 30);
  CHECK_EQ(p.weight, (uint8_t)20);  // -1 cada 3 minutos
}

TEST(tick, descuido_cuenta_una_vez_por_hora) {
  Pet p;
  makePet(p, 4);
  setStats(p, 0, 0, 0, 0);
  advance(p, 1);
  CHECK_EQ(p.careMistakes, (uint8_t)1);
  advance(p, 50);
  CHECK_MSG(p.careMistakes == 1, "el enfriamiento evita contar el mismo descuido cada minuto");
  advance(p, 11);
  CHECK_EQ(p.careMistakes, (uint8_t)2);
}

// regresion v1.4: el descuido restaba 3 de vinculo cada 30 min y era imposible
// recuperarlo; ahora resta 1 y nunca lo deja en 0
TEST(tick, descuido_enfria_el_vinculo_sin_arrasarlo) {
  Pet p;
  makePet(p, 4);
  p.bond = 50;
  setStats(p, 0, 0, 0, 0);
  advance(p, 1);
  CHECK_EQ(p.bond, (uint8_t)49);
  p.bond = 1;
  advance(p, 61);
  CHECK_MSG(p.bond == 1, "el vinculo no cae por debajo de 1");
}

TEST(tick, buen_cuidado_12h_forja_defensa) {
  Pet p;
  makePet(p, 4);
  CHECK_EQ(p.trDef, (uint8_t)0);
  for (int h = 0; h < 720; h++) {  // mantener todo >= 40 durante 12 h
    setStats(p, 100, 100, 100, 100);
    advance(p, 1);
  }
  CHECK_EQ(p.trDef, (uint8_t)1);
}

// ---------------------------------------------------------------- niveles
// El nivel SALIA de la edad (un nivel por hora). Se separo para que un bicho
// guardado en la caja pueda tener nivel propio: ahora sale de la experiencia,
// y la edad solo manda en la eclosion, el cuidado y la despedida.
TEST(level, el_nivel_sale_de_la_experiencia_y_no_de_la_edad) {
  Pet p;
  makePet(p, 4);
  CHECK_EQ((int)p.level(), 1);

  uint32_t antes = p.exp;
  p.ageMinutes += 10 * 60;         // diez horas de vida
  CHECK_MSG((int)p.level() == 1, "envejecer ya no sube de nivel");
  CHECK_EQ((int)p.exp, (int)antes);

  p.gainExp(Pet::expForLevel(10) - p.exp);
  CHECK_EQ((int)p.level(), 10);
}

TEST(level, la_curva_es_la_clasica_n_al_cubo) {
  CHECK_EQ((int)Pet::expForLevel(1), 1);
  CHECK_EQ((int)Pet::expForLevel(10), 1000);
  CHECK_EQ((int)Pet::expForLevel(50), 125000);
  CHECK_EQ((int)Pet::expForLevel(100), 1000000);
  // por debajo y por encima no se sale
  CHECK_EQ((int)Pet::expForLevel(0), 1);
  CHECK_EQ((int)Pet::expForLevel(200), (int)Pet::expForLevel(LEVEL_CAP));
}

TEST(level, topa_en_100_y_no_desborda) {
  Pet p;
  makePet(p, 4);
  p.gainExp(4000000000u);          // una burrada
  CHECK_EQ((int)p.level(), LEVEL_CAP);
  CHECK_EQ((int)p.gainExp(1000000), 0);   // ya no sube mas
  CHECK_EQ((int)p.level(), LEVEL_CAP);
}

TEST(level, ganar_experiencia_devuelve_cuantos_niveles_subio) {
  Pet p;
  makePet(p, 4);
  CHECK_EQ((int)p.gainExp(0), 0);
  uint8_t n = p.gainExp(Pet::expForLevel(5));
  CHECK_EQ((int)n, 4);                    // del 1 al 5
  CHECK_EQ((int)p.level(), 5);
}

TEST(level, la_barra_de_progreso_va_de_0_a_1) {
  Pet p;
  makePet(p, 4);
  setLevel(p, 10);
  CHECK(p.levelProgress() < 0.01f);       // recien subido
  p.exp = (Pet::expForLevel(10) + Pet::expForLevel(11)) / 2;
  CHECK_RANGE((int)(p.levelProgress() * 100), 40, 60);
  setLevel(p, LEVEL_CAP);
  CHECK(p.levelProgress() >= 1.0f);
}

TEST(level, tumbar_a_uno_mas_gordo_da_mas_experiencia) {
  uint32_t flojo = Pet::expFromFoe(129, 20);    // Magikarp
  uint32_t duro = Pet::expFromFoe(150, 20);     // Mewtwo
  CHECK(duro > flojo * 2);
  // y el nivel del rival tambien cuenta
  CHECK(Pet::expFromFoe(150, 40) > Pet::expFromFoe(150, 20));
  CHECK_EQ((int)Pet::expFromFoe(0, 20), 0);     // dex invalido, sin reventar
}

TEST(level, cuidarlo_da_un_poco_de_experiencia) {
  // Existe para que quien solo quiera criar al bicho vea alguna evolucion.
  // Es poco a proposito: lo que sube de nivel de verdad es pelear.
  Pet p;
  makePet(p, 4);
  uint32_t antes = p.exp;
  p.setClock(1700000000u);      // hace falta reloj para que cuente el dia
  p.feedBerry(0);
  CHECK_MSG(p.exp > antes, "el primer cuidado del dia suma algo");
  CHECK_MSG(p.exp - antes < Pet::expForLevel(5),
            "pero no tanto como para no tener que pelear nunca");
}

// ---------------------------------------------------------------- evolucion
// LA REGLA: llego al nivel, evoluciona. Punto.
//
// Antes habia dos peajes invisibles: se sumaba `careMistakes` al nivel y
// ademas se exigia que ninguna barra bajara de 40. Un Bulbasaur llegaba al
// 16, no pasaba nada, y la pantalla no decia por que. Los descuidos siguen
// contando donde les toca —— la calidad del huevo que deja y la medalla de
// criarlo sin fallos ——, no sobre algo que ya te ganaste peleando.
TEST(evolve, al_llegar_al_nivel_evoluciona_y_punto) {
  Pet p;
  makePet(p, 4);        // CHARMANDER evoluciona a nivel 16
  setLevel(p, 15);
  CHECK_MSG(!p.canEvolveNow(), "al 15 todavia no");
  setLevel(p, 16);
  CHECK_MSG(p.canEvolveNow(), "al 16 si");
}

TEST(evolve, los_descuidos_no_retrasan_la_evolucion) {
  Pet p;
  makePet(p, 4);
  setLevel(p, 16);
  p.careMistakes = 30;
  CHECK_MSG(p.canEvolveNow(),
            "los descuidos volvieron a pesar sobre la evolucion");
}

TEST(evolve, estar_hecho_polvo_tampoco_lo_bloquea) {
  Pet p;
  makePet(p, 4);
  setLevel(p, 16);
  setStats(p, 15, 15, 15, 15);   // las cuatro barras por los suelos
  CHECK_MSG(p.canEvolveNow(),
            "volvio el porton de lowestStat(), que nadie veia");
}

// Y los descuidos SIGUEN contando donde deben: en lo que hereda el
// siguiente huevo.
TEST(evolve, los_descuidos_si_pesan_en_la_herencia) {
  Pet a, b;
  makePet(a, 4);
  setLevel(a, 40);
  a.bond = 90;
  a.careMistakes = 0;
  makePet(b, 4);
  setLevel(b, 40);
  b.bond = 90;
  b.careMistakes = 3;
  CHECK_MSG(a.legacyScore() > b.legacyScore(),
            "criarlo sin un fallo tiene que valer mas para el huevo");
}

// Arreglado en v1.8: el cast a uint8_t de (evolveLevel + careMistakes) desbordaba
TEST(evolve, muchos_descuidos_no_adelantan_la_evolucion) {
  Pet p;
  makePet(p, 4);       // CHARMANDER: evoluciona a nivel 16
  p.careMistakes = 250;  // 250 descuidos: deberia estar lejisimos de evolucionar
  setLevel(p, 10);  // nivel 10
  CHECK_MSG(!p.canEvolveNow(), "con 250 descuidos no puede evolucionar a nivel 10");
}

TEST(evolve, no_evoluciona_dormido_ni_de_huevo_ni_en_ceremonia) {
  Pet p;
  makePet(p, 4);
  setLevel(p, 21);
  CHECK(p.canEvolveNow());
  p.toggleLight();
  CHECK(!p.canEvolveNow());
  p.toggleLight();
  p.startFarewell();
  CHECK(!p.canEvolveNow());

  Pet e;
  mockNvsReset();
  e.begin();
  CHECK(!e.canEvolveNow());
}

TEST(evolve, transforma_registra_y_anima) {
  Pet p;
  makePet(p, 4);
  setLevel(p, 21);
  p.evolve();
  CHECK_EQ(p.speciesId, (int16_t)5);
  CHECK_EQ(p.prevSpeciesId, (int16_t)4);
  CHECK(p.isRegistered(5));
  CHECK(p.evolving());
  CHECK_RANGE(p.evolveT(), 0.0f, 1.0f);
  mockAdvanceMillis(EVOLVE_ANIM_MS + 1);
  CHECK(!p.evolving());
}

TEST(evolve, forma_final_no_evoluciona) {
  Pet p;
  makePet(p, 6);  // CHARIZARD
  setLevel(p, 201);
  CHECK(!p.canEvolveNow());
  p.evolve();
  CHECK_EQ(p.speciesId, (int16_t)6);
}

// ------------------------------------------------------------- piedras
// Quien evoluciona con piedra o con el cable NO sube por nivel. Antes el
// generador les habia puesto un nivel de reserva y subian solos, con lo que
// los seis objetos de la bolsa no habrian servido para nada.

static uint8_t itemPorTipo(uint8_t tipo) {
  for (uint8_t i = 1; i < ITEM_COUNT; i++)
    if (ITEM_TBL[i].fx == IT_STONE && ITEM_TBL[i].arg == tipo) return i;
  return 0;
}

TEST(piedra, con_piedra_no_se_sube_por_nivel) {
  Pet p;
  makePet(p, DEX_EEVEE);
  setLevel(p, 100);
  CHECK_MSG(!p.canEvolveNow(), "Eevee no evoluciona por nivel, ni al 100");
  p.evolve();
  CHECK_EQ(p.speciesId, (int16_t)DEX_EEVEE);
}

TEST(piedra, cada_piedra_da_SU_rama_de_eevee) {
  // Esto es lo que las piedras anaden de verdad: ELEGIR, en vez de que el
  // juego sortee la que falte en la Pokedex.
  const struct { uint8_t tipo; int16_t sale; } CASOS[] = {
    { T_WATER, 134 }, { T_ELEC, 135 }, { T_FIRE, 136 },
  };
  for (auto &c : CASOS) {
    Pet p;
    makePet(p, DEX_EEVEE);
    setStats(p, 80, 80, 80, 80);
    uint8_t id = itemPorTipo(c.tipo);
    CHECK(id != 0);
    p.bag[id] = 1;
    CHECK(p.useStone(id));
    CHECK_EQ(p.speciesId, c.sale);
    CHECK_MSG(p.bag[id] == 0, "la piedra se gasta");
    CHECK(p.isRegistered(c.sale));
  }
}

TEST(piedra, la_que_no_le_toca_no_se_gasta) {
  Pet p;
  makePet(p, 25);           // Pikachu: solo la Thunder
  setStats(p, 80, 80, 80, 80);
  uint8_t agua = itemPorTipo(T_WATER);
  p.bag[agua] = 1;
  CHECK(!p.useStone(agua));
  CHECK_MSG(p.bag[agua] == 1, "si no hace nada, no se tira");
  CHECK_EQ(p.speciesId, (int16_t)25);

  uint8_t rayo = itemPorTipo(T_ELEC);
  p.bag[rayo] = 1;
  CHECK(p.useStone(rayo));
  CHECK_EQ(p.speciesId, (int16_t)26);   // Raichu
}

TEST(piedra, el_cable_hace_de_intercambio) {
  // Los cuatro que en los juegos solo evolucionan cambiandolos de consola.
  //
  // El Anil NO los deja colgados: les da las tres vias (nivel 45, Trade y
  // un Linking Cord). Antes esto comprobaba que al 100 seguian sin subir,
  // que era lo que se habia inventado aqui; lo que de verdad hace el cable
  // es ADELANTARLO, asi que se mira por debajo de su nivel.
  const struct { int16_t de, a; } CASOS[] = {
    { 64, 65 }, { 67, 68 }, { 75, 76 }, { 93, 94 },
  };
  uint8_t cable = itemPorTipo(T_NONE);
  CHECK(cable != 0);
  for (auto &c : CASOS) {
    Pet p;
    makePet(p, c.de);
    setStats(p, 80, 80, 80, 80);
    CHECK_MSG(DEX_TBL[c.de].evolveLevel == 45, "el PBS del Anil los pone al 45");
    setLevel(p, 30);
    CHECK_MSG(!p.canEvolveNow(), "por debajo de su nivel no suben solos");
    p.bag[cable] = 1;
    CHECK(p.useStone(cable));
    CHECK_EQ(p.speciesId, c.a);
  }
}

TEST(piedra, sin_cuidado_no_hay_piedra_que_valga) {
  // La piedra se salta el NIVEL, que es el atajo que compras. El cuidado no:
  // esa regla es de lo que va el juego.
  Pet p;
  makePet(p, 25);
  setStats(p, 10, 80, 80, 80);
  uint8_t rayo = itemPorTipo(T_ELEC);
  p.bag[rayo] = 1;
  CHECK(!p.useStone(rayo));
  CHECK_EQ((int)p.bag[rayo], 1);
  setStats(p, 80, 80, 80, 80);
  CHECK(p.useStone(rayo));
}

TEST(piedra, la_eterna_manda_por_encima) {
  Pet p;
  makePet(p, 25);
  setStats(p, 80, 80, 80, 80);
  for (uint8_t i = 1; i < ITEM_COUNT; i++)
    if (ITEM_TBL[i].fx == IT_EVERSTONE) { p.bag[i] = 1; p.holdItem(i); }
  uint8_t rayo = itemPorTipo(T_ELEC);
  p.bag[rayo] = 1;
  CHECK_MSG(!p.useStone(rayo), "con la Everstone puesta no evoluciona ni asi");
}

TEST(piedra, la_tabla_apunta_a_especies_reales) {
  // Una fila mal copiada no se ve hasta que alguien usa esa piedra, que
  // pueden ser horas de juego.
  for (uint8_t i = 0; i < STONE_EVO_COUNT; i++) {
    const EvoItem &s = STONE_EVO[i];
    CHECK_RANGE((int)s.from, 1, DEX_COUNT);
    CHECK_RANGE((int)s.to, 1, DEX_COUNT);
    CHECK_MSG(s.from != s.to, "nadie evoluciona en si mismo");
    CHECK_MSG(needsStone(s.from), "y needsStone tiene que verlo");
    // el objeto de esa piedra tiene que existir en la bolsa
    CHECK_MSG(itemPorTipo(s.item) != 0, "piedra sin objeto que la represente");
  }
}

TEST(evolve, boton_de_evolucion_se_pospone_al_declinar) {
  Pet p;
  makePet(p, 4);
  setLevel(p, 16);  // nivel 16
  CHECK(p.wantEvolveButton());
  p.declineEvolve();
  CHECK_MSG(!p.wantEvolveButton(), "'mantener forma' quita el boton");
  CHECK_MSG(p.canEvolveNow(), "pero la evolucion sigue disponible");
  setLevel(p, 17);  // sube de nivel: se re-ofrece
  CHECK(p.wantEvolveButton());
}

// ---------------------------------------------------------------- stats
// (2*base*gene/100 + tr/4) * nivel / 100 + 5, la de los juegos: `gene` hace
// de IV y `tr` de EV. La vieja era `base*gene/100 + nivel + tr`, que apenas
// crecia con el nivel ??? un Charizard tenia Ataque 85 a nivel 1.
static uint16_t statEsperado(uint16_t base, uint16_t gene, uint16_t lvl, uint16_t tr) {
  return (uint16_t)((base * 2 * gene / 100 + tr / 4) * lvl / 100 + 5);
}

TEST(stats, formula_de_stats_como_en_los_juegos) {
  Pet p;
  makePet(p, 4);  // CHARMANDER: atk 52, def 43, spe 65
  p.geneAtk = p.geneDef = p.geneSpe = 100;
  p.trAtk = p.trDef = p.trSpe = 0;
  p.ageMinutes = 0;  // nivel 1
  CHECK_EQ(p.atkStat(), statEsperado(52, 100, 1, 0));
  CHECK_EQ(p.defStat(), statEsperado(43, 100, 1, 0));
  CHECK_EQ(p.speStat(), statEsperado(65, 100, 1, 0));
  setLevel(p, 10);
  p.trAtk = 20;
  CHECK_EQ(p.atkStat(), statEsperado(52, 100, 10, 20));
  p.geneAtk = 110;
  CHECK_EQ(p.atkStat(), statEsperado(52, 110, 10, 20));
}

TEST(stats, los_stats_CRECEN_CON_EL_NIVEL) {
  // Lo que de verdad estaba roto. Sin esto, la cuenta puede volver a ser
  // "base + nivel" y los tests de arriba seguirian pasando: comprueban la
  // formula contra si misma.
  Pet p;
  makePet(p, 4);
  p.geneAtk = 100;
  p.trAtk = 0;
  setLevel(p, 1);
  uint16_t a1 = p.atkStat();
  setLevel(p, 50);
  uint16_t a50 = p.atkStat();
  setLevel(p, 100);
  uint16_t a100 = p.atkStat();
  CHECK_MSG(a50 > a1 * 5, "de nivel 1 a 50 el ataque tiene que dispararse");
  CHECK_MSG(a100 > a50 + a50 / 2, "y seguir subiendo hasta el 100");
  // y el entrenamiento y los genes siguen contando
  p.trAtk = 100;
  CHECK_MSG(p.atkStat() > a100, "el entreno suma");
  p.trAtk = 0;
  p.geneAtk = 110;
  CHECK_MSG(p.atkStat() > a100, "y los genes tambien");
}

TEST(stats, el_huevo_no_tiene_stats) {
  mockNvsReset();
  Pet p;
  p.begin();
  CHECK_EQ(p.atkStat(), (uint16_t)0);
  CHECK_EQ(p.defStat(), (uint16_t)0);
  CHECK_EQ(p.speStat(), (uint16_t)0);
}

TEST(train, el_gimnasio_da_un_ev_cada_dos_puntos_con_tope) {
  Pet p;
  makePet(p, 4);
  CHECK_EQ(p.trainEV(EV_ATK, 0), (uint8_t)0);
  CHECK_EQ(p.trainEV(EV_ATK, 20), (uint8_t)10);
  CHECK_EQ(p.trAtk, (uint8_t)10);
  CHECK_MSG(p.trainEV(EV_ATK, 1000) == TRAIN_EV_PER_SESSION,
            "tope por sesion");
  CHECK_EQ(p.trainHi, (uint16_t)1000);
}

TEST(train, el_gimnasio_entrena_el_ev_que_le_pidas) {
  // Lo importante del cambio: el saco ya no entrena solo la fuerza. Si esto
  // se rompiera, elegir stat en la pantalla seria decoracion.
  Pet p;
  makePet(p, 4);
  p.trainEV(EV_SPA, 20);
  CHECK_EQ(p.trSpa, (uint8_t)10);
  CHECK_MSG(p.trAtk == 0, "no toca el fisico");
  p.trainEV(EV_HP, 8);
  CHECK_EQ(p.trHp, (uint8_t)4);
}

// Arreglado al portar la bateria: devolvia la subida teorica, no la real
TEST(train, el_gimnasio_anuncia_lo_que_de_verdad_sube) {
  Pet p;
  makePet(p, 4);
  p.trAtk = 96;
  uint8_t before = p.trAtk;
  uint8_t gain = p.trainEV(EV_ATK, 24);  // 24/2 = 12, pero solo caben 4
  CHECK_EQ(gain, (uint8_t)4);
  CHECK_EQ(gain, (uint8_t)(p.trAtk - before));
}

TEST(train, entrenamiento_topa_en_100) {
  Pet p;
  makePet(p, 4);
  for (int i = 0; i < 20; i++) p.trainEV(EV_ATK, 40);
  CHECK_EQ(p.trAtk, (uint8_t)EV_STAT_CAP);
}

TEST(train, el_gimnasio_cansa_y_quema_peso) {
  Pet p;
  makePet(p, 4);
  p.weight = 40;
  p.energy = 80;
  p.fullness = 80;
  p.trainEV(EV_ATK, 20);
  CHECK_EQ(p.energy, (uint8_t)(80 - TRAIN_ENERGY_COST));
  CHECK_EQ(p.fullness, (uint8_t)75);
  CHECK_EQ(p.weight, (uint8_t)30);
  p.energy = 3;  // el suelo protege de dejarlo a cero
  p.trainEV(EV_ATK, 20);
  CHECK_EQ(p.energy, (uint8_t)3);
}

TEST(play, jugar_sube_felicidad_y_cansa) {
  Pet p;
  makePet(p, 4);
  setStats(p, 50, 50, 50, 50);
  p.play();
  CHECK_EQ(p.joy, (uint8_t)75);
  CHECK_EQ(p.energy, (uint8_t)40);
  CHECK_EQ(p.fullness, (uint8_t)45);
  CHECK(p.showHeart());
}

TEST(care, limpiar_quita_cacas_y_deja_higiene_a_tope) {
  Pet p;
  makePet(p, 4);
  p.poops = 3;
  p.hygiene = 10;
  p.clean();
  CHECK_EQ(p.poops, (uint8_t)0);
  CHECK_EQ(p.hygiene, (uint8_t)100);
}

TEST(care, mimar_sube_felicidad_y_vinculo) {
  Pet p;
  makePet(p, 4);
  p.joy = 50;
  p.caress();
  CHECK_EQ(p.joy, (uint8_t)55);
  CHECK_EQ(p.bond, (uint8_t)(BOND_AT_BIRTH + 1));
  CHECK(p.showHeart());
}

TEST(care, el_vinculo_tiene_tope_diario) {
  Pet p;
  makePet(p, 4);
  for (int i = 0; i < 50; i++) p.play();  // +2 de vinculo cada vez
  CHECK_MSG(p.bond <= BOND_AT_BIRTH + 20, "tope de 20 puntos de vinculo al dia");
  CHECK_EQ(p.bond, (uint8_t)(BOND_AT_BIRTH + 20));
}

TEST(mood, refleja_el_estado) {
  Pet p;
  makePet(p, 4);
  setStats(p, 80, 80, 80, 80);
  CHECK_EQ((int)p.mood(), (int)MOOD_HAPPY);
  p.joy = 20;
  CHECK_EQ((int)p.mood(), (int)MOOD_SAD);
  p.joy = 80;
  p.feedBerry(0);
  CHECK_EQ((int)p.mood(), (int)MOOD_EATING);
  mockAdvanceMillis(EAT_ANIM_MS + 1);
  p.toggleLight();
  CHECK_EQ((int)p.mood(), (int)MOOD_SLEEPING);
}

// ---------------------------------------------------------------- pokedex
// Teniendo una mascota SHINY, todo lo que registrabas salia marcado como
// shiny en la Pokedex: registerSpecies() miraba `shiny`, que es el del
// propio Pet, en vez del bicho que se esta registrando. Con un inicial
// shiny la Pokedex entera se llenaba de estrellas sin haber visto uno.
TEST(dexreg, una_mascota_shiny_no_marca_shiny_a_los_demas) {
  Pet p;
  makePet(p, 4);
  memset(p.dexShinyReg, 0, sizeof(p.dexShinyReg));
  p.shiny = true;                       // el TUYO es shiny
  p.registerSpecies(16, false);         // y registras un Pidgey del monton
  CHECK_MSG(p.isRegistered(16), "el Pidgey tenia que quedar registrado");
  CHECK_MSG(!p.isShinyRegistered(16),
            "se marco shiny un bicho que no lo era, por serlo el tuyo");
  p.registerSpecies(19, true);          // y uno que SI lo es
  CHECK_MSG(p.isShinyRegistered(19), "el shiny de verdad no se marco");
}

// Y las formas regionales: sin su propio registro, la ficha no podia
// enseñar un A.RAICHU aunque lo hubieras criado.
TEST(dexreg, la_forma_regional_se_registra_aparte) {
  Pet p;
  makePet(p, 4);
  memset(p.dexFormReg, 0, sizeof(p.dexFormReg));
  int16_t fila = formRow(26, 1);        // Raichu de Alola
  CHECK_MSG(fila >= 0, "FORM_TBL ya no trae al Raichu de Alola: "
                       "el test habla de otra cosa");
  CHECK_MSG(!p.isFormRegistered(fila), "no deberia estar marcada aun");
  p.registerSpecies(26, false, 1);
  CHECK_MSG(p.isFormRegistered(fila), "la forma no se registro");
  // y la forma normal del mismo numero no inventa marca
  CHECK_MSG(formRow(26, 0) < 0 || p.isFormRegistered(formRow(26, 0)),
            "coherencia de la fila 0");
}

TEST(dexreg, bitmap_cubre_toda_la_dex) {
  Pet p;
  makePet(p, 4);
  memset(p.dexReg, 0, sizeof(p.dexReg));
  CHECK_EQ(p.registeredCount(), (uint16_t)0);
  for (int16_t d = 1; d <= DEX_COUNT; d++) {
    p.dexReg[(d - 1) >> 3] |= (uint8_t)(1 << ((d - 1) & 7));
    CHECK(p.isRegistered(d));
  }
  CHECK_EQ(p.registeredCount(), (uint16_t)DEX_COUNT);
}

TEST(dexreg, fuera_de_rango_no_esta_registrado) {
  Pet p;
  makePet(p, 4);
  memset(p.dexReg, 0xFF, sizeof(p.dexReg));
  CHECK(!p.isRegistered(0));
  CHECK(!p.isRegistered(-1));
  CHECK(!p.isRegistered(DEX_COUNT + 1));
  CHECK(!p.isRegistered(32767));
  CHECK_EQ(p.registeredCount(), (uint16_t)DEX_COUNT);
}

TEST(dexreg, linea_incompleta_se_detecta) {
  Pet p;
  makePet(p, 1);  // BULBASAUR -> IVYSAUR -> VENUSAUR
  CHECK(p.lineHasUnregistered(1));
  p.dexReg[(2 - 1) >> 3] |= (uint8_t)(1 << ((2 - 1) & 7));
  p.dexReg[(3 - 1) >> 3] |= (uint8_t)(1 << ((3 - 1) & 7));
  CHECK_MSG(!p.lineHasUnregistered(1), "linea 1-2-3 completa");
}

TEST(dexreg, la_rama_de_eevee_cuenta_todas) {
  Pet p;
  makePet(p, DEX_EEVEE);
  CHECK(p.lineHasUnregistered(DEX_EEVEE));
  // Son CINCO desde Johto (Espeon y Umbreon), y no van seguidas: recorrer
  // 134..136 a mano daria la linea por completa sin estarlo.
  for (uint8_t i = 0; i < EEVEE_BRANCHES; i++) {
    int16_t d = EEVEE_BRANCH[i];
    p.dexReg[(d - 1) >> 3] |= (uint8_t)(1 << ((d - 1) & 7));
  }
  CHECK(!p.lineHasUnregistered(DEX_EEVEE));
}

TEST(egg, el_sorteo_siempre_da_una_especie_valida) {
  Pet p;
  makePet(p, 4);
  for (int i = 0; i < 400; i++) {
    randomSeed(i * 7919 + 1);
    int16_t d = p.pickEggSpecies();
    CHECK_RANGE(d, (int16_t)1, (int16_t)DEX_COUNT);
    CHECK_MSG(DEX_TBL[d].rarity != R_EVO, "de un huevo nunca sale una forma evolucionada");
  }
}

TEST(region, la_hierba_y_los_huevos_no_adelantan_regiones) {
  // El techo de BST no sirve para esto y por eso hacia falta mirarlo
  // aparte: Poochyena (220) y Zigzagoon (240) pegan MENOS que un Pidgey
  // (251), asi que en Kanto con cero insignias la hierba ya sacaba bichos
  // de Hoenn y el huevo tambien. Viajar no significaba nada.
  Pet p;
  makePet(p, 4);
  CHECK_EQ((int)p.region, 0);
  for (int16_t d = DEX_REGION_END[0] + 1; d <= DEX_COUNT; d++)
    CHECK_MSG(p.wildWeight(d, 13) == 0,
              std::string("en Kanto no puede salir ") + DEX_TBL[d].name);

  for (int i = 0; i < 400; i++) {
    randomSeed(i * 104729 + 3);
    int16_t d = p.pickEggSpecies();
    CHECK_MSG(d <= DEX_REGION_END[0],
              std::string("en Kanto no puede nacer ") + DEX_TBL[d].name);
  }

  // Y al rev??s: los de casa siguen saliendo cuando te vas lejos.
  // `maxRegion` tambien: la puerta de la hierba la abre lo DESTRABADO,
  // y `region` solo pesa donde estas.
  p.maxRegion = DEX_REGIONS - 1;
  p.region = DEX_REGIONS - 1;
  p.badges = GYM_COUNT;
  bool deKanto = false, deLaUltima = false;
  for (int16_t d = 1; d <= DEX_COUNT && !(deKanto && deLaUltima); d++) {
    if (!p.wildWeight(d, 13)) continue;
    if (d <= DEX_REGION_END[0]) deKanto = true;
    if (d > DEX_REGION_END[DEX_REGIONS - 2]) deLaUltima = true;
  }
  CHECK_MSG(deKanto, "en la ultima region ya no sale nadie de la primera");
  CHECK_MSG(deLaUltima, "en la ultima region no sale nadie de la ultima");
}

// --- formas regionales -----------------------------------------------

TEST(forma, la_tabla_cuadra_con_la_dex) {
  CHECK(FORM_COUNT > 50);
  for (uint16_t i = 0; i < FORM_COUNT; i++) {
    const FormVariant &f = FORM_TBL[i];
    CHECK_RANGE((int)f.base, 1, (int)DEX_COUNT);
    CHECK_MSG(f.form > 0, "la forma 0 es la normal y no va en la tabla");
    CHECK_MSG(f.ability < ABIL_COUNT,
              std::string(f.name) + ": habilidad fuera de la tabla");
    CHECK_RANGE((int)f.type1, 1, (int)T_COUNT - 1);
    CHECK_MSG(f.type2 == T_NONE || f.type1 != f.type2,
              std::string(f.name) + ": tipo repetido");
    CHECK_MSG(strlen(f.name) <= 12,
              std::string(f.name) + ": no cabe en la ficha");
    CHECK_MSG(f.region == FORM_NO_REGION || f.region < DEX_REGIONS,
              std::string(f.name) + ": region que no existe");
  }
}

TEST(forma, una_especie_no_repite_forma) {
  for (uint16_t i = 0; i < FORM_COUNT; i++)
    for (uint16_t j = i + 1; j < FORM_COUNT; j++)
      if (FORM_TBL[i].base == FORM_TBL[j].base)
        CHECK_MSG(FORM_TBL[i].form != FORM_TBL[j].form,
                  std::string(FORM_TBL[i].name) + " repite numero de forma");
}

TEST(forma, dos_formas_no_se_llaman_igual) {
  // Los tres Tauros de Paldea se llamaban los tres "P.TAUROS": en la
  // ficha eran el mismo bicho con tres dibujos. Cada fila por separado
  // estaba bien, y por eso no lo vio nada ??? solo la hoja de contacto.
  for (uint16_t i = 0; i < FORM_COUNT; i++)
    for (uint16_t j = i + 1; j < FORM_COUNT; j++)
      CHECK_MSG(strcmp(FORM_TBL[i].name, FORM_TBL[j].name) != 0,
                std::string(FORM_TBL[i].name) + ": dos formas con el "
                "mismo rotulo no se distinguen en pantalla");
  for (uint16_t i = 0; i < MEGA_COUNT; i++)
    for (uint16_t j = i + 1; j < MEGA_COUNT; j++)
      CHECK_MSG(strcmp(MEGA_TBL[i].name, MEGA_TBL[j].name) != 0,
                std::string(MEGA_TBL[i].name) + ": dos megas con el "
                "mismo rotulo no se distinguen en pantalla");
}

TEST(forma, toda_forma_con_region_se_puede_sacar) {
  // formForRegion acumula probabilidades y devuelve la primera que pasa
  // la tirada. Devolviendo la primera que CASABA, las variedades 2 y 3
  // de un mismo bicho no salian jamas: el sprite estaba en la tarjeta y
  // no habia tirada que lo alcanzara. Aqui se recorren las 100 tiradas
  // y se exige que cada forma con region aparezca en alguna.
  for (uint16_t i = 0; i < FORM_COUNT; i++) {
    const FormVariant &f = FORM_TBL[i];
    if (f.region == FORM_NO_REGION) continue;
    bool sale = false;
    for (uint8_t roll = 0; roll < 100 && !sale; roll++)
      if (formForRegion(f.base, f.region, roll) == f.form) sale = true;
    CHECK_MSG(sale, std::string(f.name) + ": no sale con ninguna tirada");
  }
}

TEST(forma, las_tres_variedades_de_tauros_salen_las_tres) {
  // Paldea tiene Tauros de Combate, de Llamas y de Agua, repartidos. Es
  // el unico caso hoy de varias variedades del mismo bicho en la misma
  // region, asi que es el que guarda la regla.
  int visto[4] = {0, 0, 0, 0};
  for (uint8_t roll = 0; roll < 100; roll++) {
    uint8_t f = formForRegion(128, 8, roll);   // 8 = PALDEA
    CHECK_RANGE((int)f, 1, 3);
    visto[f]++;
  }
  for (int f = 1; f <= 3; f++)
    CHECK_MSG(visto[f] > 20, "la variedad de Tauros " + std::to_string(f) +
                             " sale " + std::to_string(visto[f]) + " de 100");
  CHECK_EQ(visto[1] + visto[2] + visto[3], 100);
}

TEST(forma, el_grupo_de_una_region_no_pasa_de_100) {
  // Si las formas de un bicho en una region sumaran mas de 100, la
  // tirada nunca llegaria a las ultimas y volverian a ser filas muertas.
  for (uint16_t i = 0; i < FORM_COUNT; i++) {
    if (FORM_TBL[i].region == FORM_NO_REGION) continue;
    uint16_t suma = 0;
    for (uint16_t j = 0; j < FORM_COUNT; j++)
      if (FORM_TBL[j].base == FORM_TBL[i].base &&
          FORM_TBL[j].region == FORM_TBL[i].region)
        suma += FORM_TBL[j].chance;
    CHECK_MSG(suma > 0 && suma <= 100,
              std::string(FORM_TBL[i].name) + ": su grupo suma " +
              std::to_string(suma) + "%");
  }
}

TEST(forma, la_de_alola_es_de_hielo) {
  // Vulpix normal es fuego; el de Alola, hielo. Es el ejemplo mas claro
  // de que la forma cambia el combate y no solo el dibujo.
  CHECK_EQ((int)DEX_TBL[37].type1, (int)T_FIRE);
  int16_t f = formRow(37, formForRegion(37, 6));   // 6 = ALOLA
  CHECK(f >= 0);
  CHECK_EQ((int)FORM_TBL[f].type1, (int)T_ICE);
}

TEST(forma, sale_la_de_tu_region_y_no_otra) {
  // En Kanto un Vulpix es el de siempre; en Alola, el de Alola.
  CHECK_EQ((int)formForRegion(37, 0), 0);
  CHECK(formForRegion(37, 6) != 0);
  // y una especie sin forma regional no cambia en ninguna
  for (uint8_t r = 0; r < DEX_REGIONS; r++)
    CHECK_EQ((int)formForRegion(1, r), 0);        // Bulbasaur
}

TEST(forma, se_conserva_al_evolucionar) {
  // Un Vulpix de Alola da un Ninetales de Alola. Sin esto, evolucionar
  // "curaria" la forma y la region dejaria de significar nada.
  uint8_t fa = formForRegion(37, 6);
  CHECK(fa != 0);
  CHECK_EQ((int)formAfterEvolve(38, fa), (int)fa);   // Ninetales la tiene
  // y si el destino NO la tiene, se vuelve a la normal
  CHECK_EQ((int)formAfterEvolve(1, fa), 0);
}

TEST(forma, el_bicho_la_hereda_al_evolucionar) {
  Pet p;
  makePet(p, 37);                      // Vulpix
  p.maxRegion = 6;                     // destrabado hasta Alola
  p.region = 6;                        // y estas alli
  p.form = formForRegion(37, p.region);
  CHECK(p.form != 0);
  uint8_t antes = p.form;
  setStats(p, 80, 80, 80, 80);
  uint8_t piedra = itemPorTipo(T_FIRE);
  p.bag[piedra] = 1;
  CHECK(p.useStone(piedra));
  CHECK_EQ(p.speciesId, (int16_t)38);
  CHECK_MSG(p.form == antes, "el Ninetales tiene que seguir siendo de Alola");
}

TEST(forma, cambia_los_PS_en_combate) {
  // Meowth de Galar: otros stats base. La barra de vida tiene que salir
  // con los SUYOS, no con los del Meowth de siempre.
  uint8_t fg = formForRegion(52, 7);   // 7 = GALAR
  CHECK(fg != 0);
  Combatant a, b;
  a.init(52, 50, false, 0);
  b.init(52, 50, false, fg);
  CHECK_MSG(a.type1() != b.type1() || a.base(1) != b.base(1),
            "el Meowth de Galar tiene que pelear distinto");
}

// --- sexo ------------------------------------------------------------

// --- las RAMAS de la evolucion por nivel ------------------------------
//
// `DexEntry::evolvesTo` guarda UN destino. Cuando el Anil parte una
// evolucion en dos o tres, las otras no pasaban nunca y el bicho del
// otro lado solo aparecia en la hierba. Esto fija las cuatro ramas.
//
// Nota de honestidad: yo informe de que Burmy macho acababa de Wormadam,
// y era MENTIRA: ya estaba resuelto con DEX_SEXADA. Lo que fallaba era
// mi tools/contraste.py, que solo miraba dex.h y evos.h y no conocia esa
// tercera tabla. El test de abajo esta para que no haya que creerme.

// Deja al bicho listo para evolucionar ya: nivel de sobra y barras bien.
static void listoParaEvolucionar(Pet &p, uint16_t nivel) {
  setLevel(p, nivel);
  setStats(p, 100, 100, 100, 100);
  p.careMistakes = 0;
}

TEST(rama, burmy_hembra_da_wormadam_y_macho_mothim) {
  Pet h;
  makePet(h, 412);                 // Burmy
  h.female = true;
  h.genderSet = true;
  listoParaEvolucionar(h, 25);
  CHECK(h.canEvolveNow());
  h.evolve();
  CHECK_MSG(h.speciesId == 413, "una Burmy hembra tiene que dar Wormadam");

  Pet m;
  makePet(m, 412);
  m.female = false;
  m.genderSet = true;
  listoParaEvolucionar(m, 25);
  CHECK(m.canEvolveNow());
  m.evolve();
  CHECK_MSG(m.speciesId == 414, "un Burmy macho tiene que dar Mothim, "
                                "no Wormadam");
}

TEST(rama, tyrogue_mira_su_ataque_contra_su_defensa) {
  // Las tres ramas: Hitmonlee si pega mas, Hitmonchan si aguanta mas,
  // Hitmontop si estan igualados. Antes solo existia la tercera.
  struct Caso { uint8_t gAtk, gDef; int16_t espera; const char *por; };
  const Caso casos[] = {
    { 110, 90, 106, "con mas Ataque tiene que dar Hitmonlee" },
    { 90, 110, 107, "con mas Defensa tiene que dar Hitmonchan" },
    { 100, 100, 237, "igualado tiene que dar Hitmontop" },
  };
  for (const Caso &c : casos) {
    Pet p;
    makePet(p, 236);
    p.geneAtk = c.gAtk;
    p.geneDef = c.gDef;
    p.trAtk = 0;
    p.trDef = 0;
    listoParaEvolucionar(p, 25);
    CHECK(p.canEvolveNow());
    p.evolve();
    CHECK_MSG(p.speciesId == c.espera,
              std::string(c.por) + ", y dio " + std::to_string(p.speciesId));
  }
}

TEST(rama, wurmple_da_los_dos_y_siempre_el_mismo_a_cada_bicho) {
  // La rama de Wurmple es a suertes, pero la tirada sale de sus GENES:
  // el mismo bicho da siempre lo mismo. Con random() aqui, la ficha y
  // la evolucion podrian discrepar.
  int silcoon = 0, cascoon = 0;
  for (uint8_t g = 90; g <= 110; g++) {
    Pet a, b;
    makePet(a, 265);
    a.geneAtk = g; a.geneSpe = 100;
    listoParaEvolucionar(a, 12);
    a.evolve();
    makePet(b, 265);
    b.geneAtk = g; b.geneSpe = 100;
    listoParaEvolucionar(b, 12);
    b.evolve();
    CHECK_MSG(a.speciesId == b.speciesId,
              "dos Wurmple con los mismos genes dieron cosas distintas");
    if (a.speciesId == 266) silcoon++;
    else if (a.speciesId == 268) cascoon++;
    else CHECK_MSG(false, "Wurmple dio " + std::to_string(a.speciesId));
  }
  CHECK_MSG(silcoon > 0, "ningun Wurmple dio Silcoon");
  CHECK_MSG(cascoon > 0, "ningun Wurmple dio Cascoon: la rama no existe");
}

TEST(rama, nincada_deja_un_shedinja_de_propina) {
  Pet p;
  makePet(p, 290);                 // Nincada
  listoParaEvolucionar(p, 25);
  CHECK(p.canEvolveNow());
  uint8_t antes = p.teamCount();
  p.evolve();
  CHECK_MSG(p.speciesId == 291, "Nincada tiene que dar Ninjask");
  CHECK_MSG(p.lastShedDex == 292, "y dejar ademas un Shedinja");
  CHECK_MSG(p.teamCount() == antes + 1,
            "el Shedinja tiene que ocupar un hueco del equipo");
  // y que sea EL de verdad, no un hueco con el numero puesto
  bool esta = false;
  for (uint8_t i = 0; i < TEAM_SLOTS; i++)
    if (p.team[i].species == 292) esta = true;
  CHECK_MSG(esta, "no encuentro al Shedinja en el equipo");
}

TEST(rama, con_el_equipo_lleno_el_shedinja_no_se_pierde_ni_revienta) {
  Pet p;
  makePet(p, 290);
  listoParaEvolucionar(p, 25);
  // llenar equipo Y banco seria mucho; con el equipo lleno tiene que
  // irse al banco, que es lo que hace una captura
  for (int i = 0; i < 6; i++) p.catchMon(19, 5, false, 0);
  p.evolve();
  CHECK_EQ((int)p.speciesId, 291);
  bool enAlgunSitio = false;
  for (uint8_t i = 0; i < TEAM_SLOTS; i++)
    if (p.team[i].species == 292) enAlgunSitio = true;
  for (uint8_t i = 0; i < BANK_SLOTS; i++)
    if (p.bank[i].species == 292) enAlgunSitio = true;
  CHECK_MSG(enAlgunSitio, "el Shedinja se perdio con el equipo lleno");
}

TEST(rama, evolucionar_a_algo_con_MENOS_PS_no_deja_la_vida_por_encima) {
  // Casi todas las evoluciones suben los PS base, y por eso esto
  // aguanto tanto sin verse. Pero hay tres que los BAJAN, y ninguna
  // necesita comandos de depuracion:
  //   Tyrogue  55 -> Hitmontop 50
  //   Mime Jr. 60 -> Mr. Mime  50
  //   Nincada  31 -> Shedinja   1   (el de propina)
  // Sin recortar, el bicho se quedaba con mas vida que su maximo: la
  // barra se salia de su caja y el umbral de "demasiado tocado para
  // pelear" contaba sobre un numero imposible.
  //
  // Lo encontro la bateria de INTERFAZ pasando los guiones del
  // simulador; los 541 tests de logica no lo miraban.
  const int16_t BAJAN[] = { 236, 439 };   // Tyrogue, Mime Jr.
  for (int16_t d : BAJAN) {
    Pet p;
    makePet(p, d);
    listoParaEvolucionar(p, 40);
    CHECK_MSG(p.canEvolveNow(), std::string(DEX_TBL[d].name) +
                                " tendria que poder evolucionar");
    p.evolve();
    CHECK_MSG(p.hpLeft <= p.maxHp(),
              std::string(DEX_TBL[d].name) + " acabo con " +
              std::to_string(p.hpLeft) + " PS y su maximo es " +
              std::to_string(p.maxHp()));
    CHECK_MSG(p.hpLeft > 0, "y tampoco puede acabar a cero");
  }
}

// --- las MT -----------------------------------------------------------
//
// Idea del usuario: los golpes que un bicho PUEDE aprender pero nunca
// aprende solo, repartidos como premio. El Anil ya lo trae en
// TutorMoves (1021 de 1025 especies, mediana de 57 golpes utiles por
// bicho), asi que no hubo que inventarse ninguna lista.

TEST(mt, la_tabla_cuadra_y_apunta_a_golpes_que_existen) {
  CHECK_MSG(TM_COUNT > 200, "esperaba un par de cientos de MT");
  for (uint16_t i = 0; i < TM_COUNT; i++)
    CHECK_MSG(tmMove(i) < MOVE_COUNT,
              "la MT " + std::to_string(i) + " apunta al golpe " +
              std::to_string(tmMove(i)) + " y solo hay " +
              std::to_string((int)MOVE_COUNT));
  // un numero que no existe no revienta
  CHECK_EQ((int)tmMove(TM_COUNT + 10), (int)MOVE_STRUGGLE);
  CHECK(!tmFits(1, TM_COUNT + 10));
  CHECK(!tmFits(0, 0));
  CHECK(!tmFits(DEX_COUNT + 1, 0));
}

TEST(mt, casi_todas_las_especies_pueden_aprender_algo) {
  int sinNada = 0;
  for (int16_t d = 1; d <= DEX_COUNT; d++)
    if (tmFitCount(d) == 0) sinNada++;
  CHECK_MSG(sinNada < 20, std::to_string(sinNada) +
                          " especies no pueden aprender ninguna MT");
}

TEST(mt, no_se_le_puede_ensenar_una_que_no_es_suya) {
  Pet p;
  makePet(p, 1);
  int16_t suya = -1, ajena = -1;
  for (uint16_t i = 0; i < TM_COUNT; i++) {
    if (suya < 0 && tmFits(1, i)) suya = (int16_t)i;
    if (ajena < 0 && !tmFits(1, i)) ajena = (int16_t)i;
  }
  CHECK(suya >= 0 && ajena >= 0);
  CHECK_MSG(!p.canTeachTM((uint16_t)suya), "sin tenerla no se ensena");
  CHECK(p.giveTM((uint16_t)suya));
  CHECK(p.giveTM((uint16_t)ajena));
  CHECK_MSG(p.canTeachTM((uint16_t)suya), "ahora si");
  CHECK_MSG(!p.canTeachTM((uint16_t)ajena),
            "la que no es de su especie no, aunque la tengas");
}

TEST(mt, ensenarla_pone_el_golpe_con_sus_PP_y_fija_la_eleccion) {
  Pet p;
  makePet(p, 1);
  int16_t tm = -1;
  for (uint16_t i = 0; i < TM_COUNT && tm < 0; i++)
    if (tmFits(1, i)) tm = (int16_t)i;
  CHECK(tm >= 0);
  p.giveTM((uint16_t)tm);
  CHECK(p.teachTM((uint16_t)tm, 2));
  uint16_t mv = tmMove((uint16_t)tm);
  CHECK_EQ((int)p.ppMove[2], (int)mv);
  CHECK_MSG(p.ppLeft[2] == movePP(mv), "tiene que entrar con los PP llenos");
  CHECK_MSG(p.movesPicked,
            "a partir de aqui los golpes los elige el jugador, o el "
            "siguiente nivel pisaria lo que acaba de ensenar");
  CHECK_MSG(!p.canTeachTM((uint16_t)tm), "y ya se la sabe");
}

TEST(mt, un_hueco_que_no_existe_no_cuela) {
  Pet p;
  makePet(p, 1);
  int16_t tm = -1;
  for (uint16_t i = 0; i < TM_COUNT && tm < 0; i++)
    if (tmFits(1, i)) tm = (int16_t)i;
  p.giveTM((uint16_t)tm);
  CHECK(!p.teachTM((uint16_t)tm, 4));
  CHECK(!p.teachTM((uint16_t)tm, 200));
}

TEST(mt, darla_dos_veces_solo_cuenta_una) {
  Pet p;
  makePet(p, 1);
  CHECK(p.giveTM(7));
  CHECK_MSG(!p.giveTM(7), "la segunda vez no es nueva");
  CHECK_EQ((int)p.tmCount(), 1);
  CHECK(p.hasTM(7));
  CHECK(!p.hasTM(8));
}

TEST(mt, el_gimnasio_regala_una_que_tu_bicho_pueda_aprender) {
  Pet p;
  makePet(p, 1);
  setStats(p, 100, 100, 100, 100);
  uint8_t antes = p.tmCount();
  p.winGym();
  CHECK_MSG(p.tmCount() == antes + 1, "ganar una insignia da una MT");
  CHECK_MSG(p.lastTM >= 0, "y se tiene que poder contar cual");
  CHECK_MSG(tmFits(p.speciesId, (uint16_t)p.lastTM),
            "y tiene que ser una que EL pueda aprender");
}

TEST(mt, sobreviven_a_un_reinicio) {
  Pet p;
  makePet(p, 1);
  p.giveTM(3);
  p.giveTM(90);
  p.giveTM(200);
  p.saveNow();
  Pet q;                               // misma NVS: es como reencender
  q.begin();
  CHECK(q.hasTM(3));
  CHECK(q.hasTM(90));
  CHECK(q.hasTM(200));
  CHECK(!q.hasTM(4));
  CHECK_EQ((int)q.tmCount(), 3);
}

// --- de QUIEN son los EVs y la experiencia ----------------------------
//
// Pregunta del usuario: al cambiar de principal, los EVs y los IVs se
// quedan con el bicho que sale? La respuesta es SI y lo fija el primer
// test. Pero al ir a comprobarlo salio que lo ganado PELEANDO iba
// siempre al principal, aunque el que estuviera en pista fuera un
// suplente: o sea, justo lo contrario de lo que uno espera cuando
// cambia de bicho para no estropearle el reparto.

// El mismo apano que `ponSuplente`, que vive 2600 lineas mas abajo y
// aqui todavia no se ve. (En este fichero los helpers se declaran justo
// antes del grupo que los usa; moverlo entero seria peor.)
static uint8_t meteSuplente(Pet &p, int16_t dex, uint16_t nivel) {
  StoredMon m{};
  m.species = dex;
  m.exp = Pet::expForLevel(nivel);
  m.hpLeft = 50;
  p.addToTeam(m);
  for (uint8_t i = 0; i < TEAM_SLOTS; i++)
    if (p.team[i].species == dex && i != p.mainSlot) return i;
  return 0xFF;
}

TEST(entrenamiento, los_EV_y_los_IV_viajan_con_su_bicho) {
  Pet p;
  makePet(p, 1);
  // al principal se le marcan unos EVs y unos genes reconocibles
  p.trAtk = 40; p.trSpe = 12;
  p.geneAtk = 107;
  uint8_t otro = meteSuplente(p, 4, 20);
  CHECK(otro != 0xFF);
  p.team[otro].trAtk = 3; p.team[otro].trSpe = 90;
  p.team[otro].geneAtk = 93;

  CHECK_MSG(p.setMain(otro), "tendria que poder cambiar de principal");
  // ahora el principal es el otro, con LO SUYO
  CHECK_EQ((int)p.trAtk, 3);
  CHECK_EQ((int)p.trSpe, 90);
  CHECK_EQ((int)p.geneAtk, 93);
  // y el que salio conserva lo suyo en su hueco
  bool encontrado = false;
  for (uint8_t i = 0; i < TEAM_SLOTS; i++) {
    if (p.team[i].species != 1) continue;
    encontrado = true;
    CHECK_EQ((int)p.team[i].trAtk, 40);
    CHECK_EQ((int)p.team[i].trSpe, 12);
    CHECK_EQ((int)p.team[i].geneAtk, 107);
  }
  CHECK_MSG(encontrado, "el que salio tendria que seguir en el equipo");
}

TEST(entrenamiento, lo_ganado_peleando_es_de_QUIEN_pelea) {
  Pet p;
  makePet(p, 1);
  uint8_t sup = meteSuplente(p, 4, 20);
  CHECK(sup != 0xFF && sup != p.mainSlot);
  uint8_t antesMain = p.trAtk;
  uint8_t antesSup = p.team[sup].trAtk;
  uint32_t expMain = p.exp, expSup = p.team[sup].exp;

  // pelea el SUPLENTE
  p.gainExpOf(sup, 500);
  p.gainEVOf(sup, EV_ATK, 1);
  p.teamShareExp(500, sup);

  CHECK_MSG(p.team[sup].trAtk == antesSup + 1,
            "el EV es del que peleo");
  CHECK_MSG(p.trAtk == antesMain,
            "y el principal, que miraba, no se lleva ninguno");
  CHECK_MSG(p.team[sup].exp >= expSup + 500,
            "la experiencia entera tambien es suya");
  CHECK_MSG(p.exp > expMain && p.exp < expMain + 500,
            "el principal cobra solo su parte de banquillo");
}

TEST(entrenamiento, peleando_el_principal_todo_sigue_como_antes) {
  Pet p;
  makePet(p, 1);
  uint8_t sup = meteSuplente(p, 4, 20);
  uint8_t antesSup = p.team[sup].trAtk;
  uint8_t antesMain = p.trAtk;
  p.gainExpOf(p.mainSlot, 500);
  p.gainEVOf(p.mainSlot, EV_ATK, 1);
  p.teamShareExp(500, p.mainSlot);
  CHECK_MSG(p.trAtk == antesMain + 1, "el EV es del principal");
  CHECK_MSG(p.team[sup].trAtk == antesSup, "el suplente no gana EVs");
  CHECK_MSG(p.team[sup].exp > 1, "pero si su parte de experiencia");
}

TEST(entrenamiento, el_techo_de_EV_vale_igual_para_un_suplente) {
  // Los dos topes (100 por stat, 200 en total) salieron de la version
  // del principal; al darle EVs a un suplente hay que respetarlos
  // igual, o el banquillo seria la via de saltarselos.
  Pet p;
  makePet(p, 1);
  uint8_t sup = meteSuplente(p, 4, 20);
  for (int i = 0; i < 300; i++) p.gainEVOf(sup, EV_ATK, 1);
  CHECK_MSG(p.team[sup].trAtk <= EV_STAT_CAP,
            "se paso del tope por stat: " +
            std::to_string(p.team[sup].trAtk));
  uint16_t total = (uint16_t)(p.team[sup].trHp + p.team[sup].trAtk +
                              p.team[sup].trDef + p.team[sup].trSpa +
                              p.team[sup].trSpd + p.team[sup].trSpe);
  CHECK_MSG(total <= EV_TOTAL_CAP, "se paso del presupuesto comun");
}

// --- RECOGIDA (Pickup) ------------------------------------------------
//
// Idea del usuario: "de tempo em tempo coloque um item no inventario".
// No es la Pickup del combate (quedarse lo que el rival gasto, que
// aqui no se notaria), sino la del paseo, que es la que se recuerda.

static uint16_t bolsaEntera(const Pet &p) {
  uint16_t n = 0;
  for (uint8_t i = 1; i < ITEM_COUNT; i++) n += p.bag[i];
  return n;
}

static uint16_t abilConFx(uint8_t fx) {
  for (uint16_t i = 1; i < ABIL_COUNT; i++)
    if (ABIL_TBL[i].fx == fx) return i;
  return 0;
}

TEST(recogida, con_pickup_acaba_trayendo_cosas) {
  Pet p;
  makePet(p, 19);
  p.ability = abilConFx(AB_PICKUP);
  CHECK(p.ability != 0);
  uint16_t antes = bolsaEntera(p);
  advance(p, 3000);                  // dos dias de vigilia
  CHECK_MSG(bolsaEntera(p) > antes,
            "en 3000 minutos despierto tendria que haber traido algo");
}

TEST(recogida, sin_la_habilidad_no_aparece_nada) {
  Pet p;
  makePet(p, 19);
  p.ability = 0;
  uint16_t antes = bolsaEntera(p);
  advance(p, 3000);
  CHECK_MSG(bolsaEntera(p) == antes,
            "sin Pickup la bolsa no se llena sola");
}

TEST(recogida, durmiendo_no_recoge) {
  // Es lo que la ata al paseo: si contara dormido seria un grifo que
  // corre solo con el aparato en un cajon.
  Pet p;
  makePet(p, 19);
  p.ability = abilConFx(AB_PICKUP);
  p.sleeping = true;
  uint16_t antes = bolsaEntera(p);
  for (int i = 0; i < 3000; i++) {
    p.sleeping = true;               // el tick podria despertarlo
    mockAdvanceMillis(PET_TICK_MS);
    p.update(millis());
  }
  CHECK_MSG(bolsaEntera(p) == antes, "dormido no recoge nada");
}

TEST(recogida, nunca_trae_una_bola_ni_una_piedra) {
  // Esas tienen sus propios grifos (el cuidado diario y el gimnasio).
  // Si cayeran de aqui, esos dos dejarian de significar algo.
  Pet p;
  makePet(p, 19);
  p.ability = abilConFx(AB_PICKUP);
  // Se siembra UNA vez y se tira muchas. Resembrando en cada vuelta,
  // lo que se mide es la primera salida del generador para semillas
  // 0,1,2..., que no es una muestra de nada: con 1 entre 240 salian
  // cero aciertos en 400 vueltas.
  randomSeed(31337);
  int traidos = 0;
  for (int v = 0; v < 6000; v++) {
    p.lastPickup = 0;
    p.rollPickup();
    if (!p.lastPickup) continue;
    traidos++;
    const ItemEntry &it = itemAt((uint8_t)p.lastPickup);
    CHECK_MSG(it.pocket == BP_BATTLE,
              std::string("trajo ") + it.name + ", que no es de usar");
    CHECK_MSG(it.fx != IT_VITAMIN, "las vitaminas se ganan peleando");
  }
  CHECK_MSG(traidos > 5, "en 6000 tiradas solo trajo " +
                         std::to_string(traidos) + ": ??esta activo?");
}

TEST(rama, la_tabla_de_ramas_cuadra_con_la_dex) {
  CHECK(EVO_RAMAS > 0);
  for (uint8_t i = 0; i < EVO_RAMAS; i++) {
    const EvoRama &r = EVO_RAMA[i];
    CHECK_RANGE((int)r.from, 1, (int)DEX_COUNT);
    CHECK_RANGE((int)r.to, 1, (int)DEX_COUNT);
    CHECK_MSG(r.to != DEX_TBL[r.from].evolvesTo,
              "una rama que repite el destino de siempre no es una rama");
    CHECK_MSG(DEX_TBL[r.from].evolveLevel > 0,
              std::string(DEX_TBL[r.from].name) +
              " tiene rama y no sube por nivel: no se alcanzaria");
  }
}

TEST(sexo, el_reparto_sale_del_anil_y_se_respeta) {
  // 155 especies no tienen sexo y a esas nunca se les puede poner uno.
  int sinSexo = 0, siempreHembra = 0;
  for (int16_t d = 1; d <= DEX_COUNT; d++) {
    uint8_t p = DEX_TBL[d].femPct;
    CHECK_MSG(p <= 100 || p == GENDERLESS,
              std::string(DEX_TBL[d].name) + ": femPct fuera de 0..100/255");
    if (p == GENDERLESS) sinSexo++;
    if (p == 100) siempreHembra++;
  }
  CHECK_MSG(sinSexo > 100, "el PBS da 155 sin sexo");
  CHECK_MSG(siempreHembra > 20, "y varias decenas siempre hembra");
  CHECK(dexGenderless(81));          // Magnemite
  CHECK(!dexGenderless(1));          // Bulbasaur
}

TEST(sexo, se_sortea_una_vez_y_no_cambia) {
  Pet p;
  makePet(p, 4);
  p.rollGender();
  bool antes = p.female;
  CHECK(p.genderSet);
  for (int i = 0; i < 20; i++) p.storeActive();   // guardar no lo mueve
  CHECK_EQ((int)p.female, (int)antes);
}

TEST(sexo, los_sin_sexo_nunca_salen_hembra) {
  Pet p;
  makePet(p, 81);                     // Magnemite
  for (int i = 0; i < 50; i++) {
    randomSeed(i * 31 + 7);
    p.rollGender();
    CHECK_MSG(!p.female, "un Magnemite no puede ser hembra");
  }
  CHECK_STREQ(p.genderMark(), "");    // y no se le pinta marca
}

TEST(sexo, el_reparto_se_nota_en_la_tirada) {
  // Salandit es FemaleOneEighth: con 200 tiradas tienen que salir
  // bastantes mas machos que hembras, o el sorteo no mira el reparto.
  Pet p;
  makePet(p, 757);
  int hembras = 0;
  for (int i = 0; i < 200; i++) {
    randomSeed(i * 7919 + 3);
    p.rollGender();
    if (p.female) hembras++;
  }
  CHECK_MSG(hembras > 0 && hembras < 70,
            "Salandit es 1/8 hembra: ni cero ni la mitad");
}

TEST(sexo, froslass_solo_de_una_hembra) {
  // El Anil: SNORUNT da GLALIE por nivel y FROSLASS con la Piedra Alba,
  // pero solo si es hembra. A un macho la piedra no le hace nada.
  uint8_t alba = 0;
  for (uint8_t i = 1; i < ITEM_COUNT; i++)
    if (ITEM_TBL[i].fx == IT_STONE && ITEM_TBL[i].arg == EVOI_DAWN) alba = i;
  CHECK(alba != 0);
  CHECK_EQ((int)stoneTarget(361, EVOI_DAWN, true), 478);    // hembra
  CHECK_EQ((int)stoneTarget(361, EVOI_DAWN, false), 0);     // macho: nada

  Pet p;
  makePet(p, 361);
  setStats(p, 80, 80, 80, 80);
  p.female = false;
  p.genderSet = true;
  p.bag[alba] = 1;
  CHECK_MSG(!p.useStone(alba), "un Snorunt macho no da Froslass");
  p.female = true;
  CHECK(p.useStone(alba));
  CHECK_EQ(p.speciesId, (int16_t)478);
}

TEST(sexo, burmy_se_parte_en_dos) {
  // La hembra da Wormadam (que es lo que guarda evolvesTo) y el macho
  // Mothim, que antes del sexo no salia de ningun sitio.
  CHECK_EQ((int)dexMaleBranch(412), 414);
  CHECK_EQ((int)DEX_TBL[412].evolvesTo, 413);

  for (int macho = 0; macho < 2; macho++) {
    Pet p;
    makePet(p, 412);
    setStats(p, 80, 80, 80, 80);
    setLevel(p, 40);
    p.female = !macho;
    p.genderSet = true;
    CHECK(p.canEvolveNow());
    p.evolve();
    CHECK_EQ(p.speciesId, (int16_t)(macho ? 414 : 413));
  }
}

TEST(egg, el_sorteo_llega_hasta_la_ultima_region) {
  // pickEggSpecies juntaba los candidatos en un `int16_t cand[80]`, asi que
  // con mas de 80 en un tier lo que viniera despues se quedaba fuera EN
  // SILENCIO: una region entera de huevos que nunca salen. Ahora sortea por
  // reservorio y no tiene tope, pero el fallo era invisible y conviene que
  // siga habiendo quien lo mire.
  const int16_t ULTIMA = DEX_REGION_END[DEX_REGIONS - 2];
  Pet p;
  makePet(p, 4);
  p.maxRegion = DEX_REGIONS - 1;  // ya ha llegado: si no, el sorteo la capa
  p.region = DEX_REGIONS - 1;
  int16_t masAlto = 0;
  for (int i = 0; i < 600; i++) {
    randomSeed(i * 7919 + 1);
    int16_t d = p.pickEggSpecies();
    if (d > masAlto) masAlto = d;
  }
  CHECK_MSG(masAlto > ULTIMA,
            "en 600 tiradas no salio nada de la ultima region: el sorteo "
            "recorta por arriba");
}

TEST(egg, el_primer_huevo_es_un_inicial_clasico) {
  for (int seed = 1; seed <= 20; seed++) {
    randomSeed(seed);
    mockNvsReset();
    Pet p;
    p.begin();
    int16_t d = p.pickEggSpecies();
    bool classic = false;
    for (int i = 0; i < NUM_CLASSIC_DEX; i++)
      if (CLASSIC_DEX[i] == d) classic = true;
    CHECK_MSG(classic, "sin pokedex, solo salen iniciales clasicos");
  }
}

TEST(egg, una_escapada_castiga_la_rareza) {
  Pet p;
  makePet(p, 4);
  p.lastEnd = CER_RUNAWAY;
  for (int seed = 1; seed <= 60; seed++) {
    randomSeed(seed);
    int16_t d = p.pickEggSpecies();
    CHECK_MSG(DEX_TBL[d].rarity == R_COMUN, "tras una escapada solo salen comunes");
  }
}

// ---------------------------------------------------------------- racha
TEST(streak, el_primer_cuidado_del_dia_suma) {
  Pet p;
  makePet(p, 4);
  p.setClock(100u * 86400);
  p.caress();
  CHECK_EQ(p.streak, (uint16_t)1);
  CHECK_EQ(p.bestStreak, (uint16_t)1);
  p.caress();
  CHECK_MSG(p.streak == 1, "el segundo cuidado del mismo dia no suma");
  p.setClock(101u * 86400);
  p.caress();
  CHECK_EQ(p.streak, (uint16_t)2);
}

TEST(streak, un_dia_perdido_reinicia) {
  Pet p;
  makePet(p, 4);
  p.setClock(100u * 86400);
  p.caress();
  p.setClock(101u * 86400);
  p.caress();
  p.setClock(105u * 86400);  // hueco de dias
  p.caress();
  CHECK_EQ(p.streak, (uint16_t)1);
  CHECK_MSG(p.bestStreak == 2, "el record de racha se conserva");
}

TEST(streak, sin_reloj_no_hay_racha) {
  Pet p;
  makePet(p, 4);
  p.caress();
  CHECK_EQ(p.streak, (uint16_t)0);
}

TEST(streak, celebra_los_hitos) {
  Pet p;
  makePet(p, 4);
  for (uint32_t d = 100; d < 103; d++) {
    p.setClock(d * 86400);
    p.caress();
  }
  CHECK_EQ(p.streak, (uint16_t)3);
  CHECK_MSG(p.showMilestone(), "hito de 3 dias celebrado");
  CHECK_EQ(p.lastMilestone, (uint16_t)3);
}

TEST(bonus, racha_y_vinculo_mejoran_el_huevo) {
  Pet p;
  makePet(p, 4);
  // Recien nacido no ha ganado nada todavia, aunque el vinculo no sea cero:
  // del vinculo solo cuenta lo que se construye por encima del suelo.
  CHECK_EQ(p.careBonus(), 0);
  p.streak = 30;
  p.bond = 100;
  CHECK_EQ(p.careBonus(), 14);
  p.streak = 300;  // la racha se capa a 30
  CHECK_EQ(p.careBonus(), 14);
  p.bond = BOND_AT_BIRTH;   // solo la racha
  CHECK_EQ(p.careBonus(), 10);
}

// ---------------------------------------------------------------- medallas
TEST(medals, nivel_10_da_medalla) {
  Pet p;
  makePet(p, 4);
  CHECK(!p.hasMedal(MED_LV10));
  setLevel(p, 10);
  advance(p, 1);
  CHECK(p.hasMedal(MED_LV10));
  CHECK(p.showMedal());
}

// EN FORMA pide haber estado gordo y haberlo bajado.
//
// Antes pedia `weight == 0`, que es el peso con el que el bicho NACE: la
// medalla se regalaba por llegar al nivel 5 sin descuidos, sin tocar el
// peso para nada. "En forma" tiene que significar que lo pusiste en
// forma.
TEST(medals, en_forma_pide_engordar_y_volver_a_bajarlo) {
  Pet p;
  makePet(p, 4);
  setLevel(p, 5);
  advance(p, 1);
  CHECK_MSG(!p.hasMedal(MED_FIT),
            "la medalla de estar en forma se da sin haber hecho nada");

  for (int i = 0; i < 5; i++) p.feedCandy();   // +12 cada una: pasa de 50
  CHECK(p.wasFat);
  CHECK_MSG(!p.hasMedal(MED_FIT), "gordo y ya premiado");

  p.weight = 0;                                 // quemado en el minijuego
  advance(p, 1);
  CHECK_MSG(p.hasMedal(MED_FIT), "engordo, lo bajo, y sigue sin medalla");

  Pet q;                                        // con un descuido, no
  makePet(q, 4);
  for (int i = 0; i < 5; i++) q.feedCandy();
  q.weight = 0;
  q.careMistakes = 1;
  setLevel(q, 5);
  advance(q, 1);
  CHECK(!q.hasMedal(MED_FIT));
}

TEST(medals, cada_medalla_nueva_suma_al_total) {
  Pet p;
  makePet(p, 4);
  for (int i = 0; i < 5; i++) p.feedCandy();
  p.weight = 0;
  setLevel(p, 10);
  advance(p, 1);  // cruza nivel 10: MED_LV10 + MED_FIT de golpe
  CHECK(p.hasMedal(MED_LV10));
  CHECK(p.hasMedal(MED_FIT));
  CHECK_MSG(p.totalMedals == 2, "el total cuenta cada bit ganado, no cada evento");
}

// FORMA FINAL es por LLEGAR, no por nacer ahi.
//
// Un Snorlax no evoluciona nunca, asi que antes recibia esta medalla al
// nivel 1 por existir. El usuario pidio premio "quando conseguir algo", y
// nacer no es conseguir nada.
TEST(medals, forma_final_no_se_regala_al_nacer) {
  Pet p;
  makePet(p, 143);  // SNORLAX ya nace en forma final
  advance(p, 1);
  CHECK_MSG(!p.hasMedal(MED_FINAL),
            "la medalla de forma final se da por nacer, no por llegar");
  CHECK_EQ(p.totalMedals, (uint16_t)0);
}

TEST(medals, forma_final_se_gana_evolucionando_hasta_el_tope) {
  Pet p;
  makePet(p, 4);          // CHARMANDER -> CHARMELEON (16) -> CHARIZARD (36)
  setLevel(p, 16);
  p.evolve();
  advance(p, 1);
  CHECK_MSG(!p.hasMedal(MED_FINAL), "CHARMELEON todavia evoluciona");
  setLevel(p, 36);
  p.evolve();
  advance(p, 1);
  CHECK_EQ(p.speciesId, (int16_t)6);
  CHECK_MSG(p.hasMedal(MED_FINAL), "llego a CHARIZARD y no hay medalla");
  CHECK_EQ(p.evoCount, (uint8_t)2);
}

// Y por PIEDRA tambien cuenta. Esta via estuvo sin contar: la cuenta
// vivia en evolve() y useStone() no pasa por ahi, asi que evolucionar con
// piedra no daba medalla de forma final ni ponia el dia de margen de la
// despedida. Evolucionar es evolucionar, se llegue por donde se llegue.
TEST(medals, evolucionar_con_piedra_tambien_cuenta) {
  Pet p;
  makePet(p, 133);              // EEVEE: sus evoluciones son todas por piedra
  CHECK_EQ(p.evoCount, (uint8_t)0);
  uint8_t piedra = 0;
  for (uint8_t i = 1; i < ITEM_COUNT; i++)
    if (itemAt(i).fx == IT_STONE && stoneTarget(133, itemAt(i).arg, p.female)) {
      piedra = i;
      break;
    }
  if (!piedra) return;          // sin piedra util en la tabla no hay nada que probar
  p.bag[piedra] = 1;
  setStats(p, 90, 90, 90, 90);  // hecho polvo no evoluciona ni con piedra
  CHECK(p.useStone(piedra));
  CHECK_MSG(p.evoCount == 1, "la piedra no cuenta como evolucion");
  CHECK_MSG(p.finalAge == p.ageMinutes,
            "la piedra no fecha la forma final, asi que la despedida "
            "puede caer en el mismo instante");
  advance(p, 1);
  CHECK_MSG(p.hasMedal(MED_FINAL), "evoluciono con piedra y no hay medalla");
}

// --------------------------------------------------------- los premios

// Que la tabla apunte a los objetos que DICE apuntar. Son indices de
// ITEM_TBL, y hoy mismo se vio lo que pasa con una tabla de indices que
// alguien regenera: los sonidos de golpe acabaron corridos una fila y
// cada golpe sonaba como el siguiente. Aqui se comprueba por NOMBRE.
TEST(medals, la_tabla_de_premios_apunta_a_lo_que_dice) {
  // LOS TREINTA. Con solo los ocho primeros el resto quedaba a nullptr
  // y el strcmp de abajo reventaba —— el mismo fallo que las tablas de
  // idioma: una lista dimensionada con una constante que crecio.
  const char *espera[MED_COUNT] = {
    "Super Potion", "Hyper Potion", "Max Revive", "Sitrus Berry",
    "Leftovers", "Focus Sash", "Scope Lens", "Protein",
    // los nuevos
    "Master Ball", "Full Restore", "Ultra Ball", "Max Elixir", "Life Orb",
    "Ultra Ball", "Choice Band", "Choice Specs", "Max Potion", "Iron",
    "Carbos", "Max Revive", "HP Up",
    // y uno por region
    "Master Ball", "Full Restore", "Max Elixir", "Life Orb", "Choice Specs",
    "Calcium", "Zinc", "Choice Band", "Max Potion",
  };
  static_assert(sizeof(espera) / sizeof(espera[0]) == MED_COUNT,
                "falta el nombre del premio de algun logro");
  for (int i = 0; i < MED_COUNT; i++) {
    CHECK_MSG(MEDAL_PRIZE[i] > 0 && MEDAL_PRIZE[i] < ITEM_COUNT,
              std::string("medalla ") + std::to_string(i) + ": id fuera de la tabla");
    CHECK_MSG(strcmp(itemAt(MEDAL_PRIZE[i]).name, espera[i]) == 0,
              std::string("medalla ") + std::to_string(i) + " deberia pagar '" +
              espera[i] + "' y paga '" + itemAt(MEDAL_PRIZE[i]).name + "'");
  }
}

// Lo mismo para el cuestionario: por aciertos, y comprobado por nombre.
TEST(medals, los_premios_del_quiz_suben_con_los_aciertos) {
  const char *espera[6] = {
    // leva 68: um degrau acima; os 5 de 5 vao a quizPremioCheio (test aparte)
    nullptr, nullptr, "Super Potion", "Hyper Potion", "Max Potion", nullptr,
  };
  for (int i = 0; i < 5; i++) {
    if (!espera[i]) {
      CHECK_MSG(QUIZ_PRIZE[i] == 0,
                std::string("con ") + std::to_string(i) + " aciertos no deberia pagar");
      continue;
    }
    CHECK(QUIZ_PRIZE[i] > 0 && QUIZ_PRIZE[i] < ITEM_COUNT);
    CHECK_MSG(strcmp(itemAt(QUIZ_PRIZE[i]).name, espera[i]) == 0,
              std::string("con ") + std::to_string(i) + " aciertos deberia pagar '" +
              espera[i] + "' y paga '" + itemAt(QUIZ_PRIZE[i]).name + "'");
  }
}

TEST(medals, ganar_una_medalla_paga_su_premio) {
  Pet p;
  makePet(p, 4);
  uint8_t it = MEDAL_PRIZE[0];          // el de Nv.10
  uint8_t antes = p.bag[it];
  setLevel(p, 10);
  advance(p, 1);
  CHECK(p.hasMedal(MED_LV10));
  CHECK_MSG(p.bag[it] == antes + 1,
            std::string("la medalla de nivel 10 no pago su ") +
            itemAt(it).name);
}

// Y una sola vez: `medalPaid` es lo que impide que el premio vuelva a
// caer en cada tick (checkMedals corre en cada minuto de juego).
TEST(medals, el_premio_se_paga_una_sola_vez) {
  Pet p;
  makePet(p, 4);
  uint8_t it = MEDAL_PRIZE[0];
  setLevel(p, 10);
  advance(p, 1);
  uint8_t tras = p.bag[it];
  advance(p, 30);
  CHECK_MSG(p.bag[it] == tras,
            "el premio se vuelve a pagar en cada vuelta");
}

// Las que ya estaban ganadas ANTES de que existiera el premio cobran la
// primera vez que se mire. Si no, el bicho que el usuario tiene vivo se
// queda sin lo que ya se habia ganado.
TEST(medals, las_medallas_de_antes_cobran_el_atrasado) {
  Pet p;
  makePet(p, 4);
  setLevel(p, 10);
  advance(p, 1);
  // se simula una partida vieja: la medalla esta, el pago no
  p.medalPaid = 0;
  uint8_t it = MEDAL_PRIZE[0];
  uint8_t antes = p.bag[it];
  CHECK_EQ(p.payMedals(), it);
  CHECK_MSG(p.bag[it] == antes + 1, "no cobro el atrasado");
  CHECK_EQ(p.payMedals(), (uint8_t)0);   // y ya no vuelve
}

TEST(medals, la_baya_favorita_da_medalla) {
  Pet p;
  makePet(p, 4);
  p.feedBerry(1);  // su favorita
  CHECK(p.berryKnown);
  advance(p, 1);
  CHECK(p.hasMedal(MED_BERRY));
}

// El total es DEL JUGADOR y sobrevive a la crianza; las medallas son del
// individuo y se reinician. Antes esto se probaba con MED_FINAL, que se
// daba al nacer; ahora hace falta ganarse algo de verdad.
TEST(medals, el_total_acumula_entre_crianzas) {
  Pet p;
  makePet(p, 4);
  setLevel(p, 10);
  advance(p, 1);
  CHECK_EQ(p.totalMedals, (uint16_t)1);   // Nv.10
  p.newEgg();
  p.chooseStarter(1);
  p.eggTap(); p.eggTap(); p.eggTap();
  CHECK_MSG(p.medals == 0, "las medallas del individuo se reinician");
  setLevel(p, 10);
  advance(p, 1);
  CHECK_MSG(p.totalMedals == 2, "el total del jugador se conserva");
}

// ---------------------------------------------------------------- ceremonias
TEST(ceremony, despedida_solo_en_forma_final_a_los_3_dias_y_con_carino) {
  Pet p;
  makePet(p, 6);  // CHARIZARD, forma final
  p.bond = FAREWELL_BOND_MIN;
  p.ageMinutes = FAREWELL_AGE_MIN - 1;
  CHECK(!p.canFarewellNow());
  p.ageMinutes = FAREWELL_AGE_MIN;
  CHECK(p.canFarewellNow());

  // El atrito nuevo: un bicho descuidado no deja herencia. Sin esto, lo
  // optimo seria despedirse en cuanto se pueda y el bicho seria desechable.
  p.bond = FAREWELL_BOND_MIN - 1;
  CHECK_MSG(!p.canFarewellNow(), "sin vinculo no hay despedida");

  Pet q;
  makePet(q, 4);  // CHARMANDER no es forma final
  q.bond = 100;
  q.ageMinutes = FAREWELL_AGE_MIN * 3;
  CHECK(!q.canFarewellNow());
}

// Lo que conto el usuario: "evolui um kadabra para alakazam, e em
// seguida veio uma msg para ele querer ser liberto". El bicho ya tenia
// los tres dias y el vinculo, asi que la EVOLUCION encendio el ultimo
// requisito en el acto. Ahora hace falta un dia mas desde que llego a su
// forma final (FAREWELL_SETTLE) para poder disfrutarla.
TEST(ceremony, evolucionar_no_dispara_la_despedida_en_el_acto) {
  Pet p;
  makePet(p, 64);              // KADABRA -> ALAKAZAM al nivel 45
  p.bond = 100;
  p.ageMinutes = FAREWELL_AGE_MIN * 2;   // de sobra por edad
  setLevel(p, 45);
  p.evolve();
  CHECK_EQ(p.speciesId, (int16_t)65);
  CHECK_MSG(!p.canFarewellNow(),
            "pide despedirse en el mismo instante de evolucionar");
  p.ageMinutes += FAREWELL_SETTLE;
  CHECK_MSG(p.canFarewellNow(),
            "pasado el dia de margen sigue sin poder despedirse");
}

TEST(ceremony, la_despedida_deja_un_huevo_nuevo) {
  Pet p;
  makePet(p, 6);
  p.ageMinutes = FAREWELL_AGE_MIN;
  p.startFarewell();
  CHECK_EQ(p.ceremony, (uint8_t)CER_FAREWELL);
  CHECK_RANGE(p.ceremonyT(), 0.0f, 1.0f);
  mockAdvanceMillis(CEREMONY_MS + 1);
  p.update(millis());
  CHECK(p.isEgg());
  CHECK_EQ(p.ceremony, (uint8_t)CER_NONE);
  CHECK_EQ(p.lastEnd, (uint8_t)CER_FAREWELL);
  CHECK_EQ(p.ageMinutes, (uint32_t)0);
}

TEST(ceremony, escaparse_pide_una_hora_de_abandono_total) {
  Pet p;
  makePet(p, 4);
  setStats(p, 0, 0, 0, 0);
  CHECK(!p.canRunawayNow());
  advance(p, RUNAWAY_TICKS - 1);
  CHECK(!p.canRunawayNow());
  advance(p, 1);
  CHECK(p.canRunawayNow());
}

TEST(ceremony, un_solo_cuidado_salva_del_abandono) {
  Pet p;
  makePet(p, 4);
  p.dbgRunawayReady();
  CHECK(p.canRunawayNow());
  p.feedBerry(0);
  advance(p, 1);
  CHECK_MSG(!p.canRunawayNow(), "comer resetea el contador de abandono");
}

// Leva 66. Al dueno se le escapo el Swampert dejando la placa encendida
// horas: nada avisaba. Ahora el bicho llama al quedarse abandonado y cada
// 15 minutos —— cuatro veces antes de que salga el boton de escapada.
TEST(ceremony, el_abandonado_llama_antes_de_poder_escaparse) {
  Pet p;
  makePet(p, 4);
  setStats(p, 0, 0, 0, 0);
  int avisos = 0, primero = -1;
  for (int t = 1; t <= RUNAWAY_TICKS; t++) {
    advance(p, 1);
    if (p.tomaAvisoAbandono()) {
      avisos++;
      if (primero < 0) primero = t;
      CHECK_MSG(!p.tomaAvisoAbandono(), "el aviso se da UNA vez");
    }
  }
  CHECK_MSG(p.canRunawayNow(), "premisa: a la hora ya puede escaparse");
  CHECK_EQ(primero, 1);
  CHECK_MSG(avisos >= 4, "solo " + std::to_string(avisos) + " avisos en la hora");
  // ...y sigue llamando con el boton ya fuera
  bool despues = false;
  for (int t = 0; t < ABANDONO_AVISO_TICKS; t++) {
    advance(p, 1);
    despues |= p.tomaAvisoAbandono();
  }
  CHECK_MSG(despues, "con el boton fuera dejo de llamar");
  // Un cuidado lo calla (una baya sola no aguanta 15 minutos: luego
  // puede volver a quedarse a cero, y entonces si que vuelve a llamar).
  p.feedBerry(0);
  advance(p, 1);
  CHECK_MSG(!p.tomaAvisoAbandono(), "recien cuidado, y sigue llamando");
}

// Leva 68. Los 5 de 5: dos Rare Candy, o 1 de cada 20 la Shiny Potion.
TEST(quiz_premio, cinco_de_cinco_da_caramelos_y_a_veces_la_pocion) {
  const uint8_t caramelo = quizItemDe(IT_RARECANDY);
  const uint8_t pocion = quizItemDe(IT_SHINY);
  CHECK_MSG(caramelo && pocion, "premisa: los dos objetos estan en la tabla");
  randomSeed(6800);
  int shiny = 0, dulces = 0;
  const int N = 4000;
  for (int i = 0; i < N; i++) {
    uint8_t q = 0;
    uint8_t it = quizPremioCheio(&q);
    if (it == pocion) { shiny++; CHECK_EQ(q, 1); }
    else if (it == caramelo) { dulces++; CHECK_EQ(q, 2); }
    else CHECK_MSG(false, "premio que no es ninguno de los dos");
  }
  CHECK_MSG(shiny > N * 3 / 100 && shiny < N * 7 / 100,
            "la pocion sale " + std::to_string(shiny) + " de " + std::to_string(N));
  CHECK_EQ(shiny + dulces, N);
}

// Y no caen por ahi: ni la Recogida ni el botin de ganar los sortean.
TEST(quiz_premio, los_premios_del_quiz_no_salen_ganando) {
  const uint8_t caramelo = quizItemDe(IT_RARECANDY);
  const uint8_t pocion = quizItemDe(IT_SHINY);
  Pet p;
  makePet(p, 4);
  randomSeed(6801);
  for (int i = 0; i < 6000; i++) {
    uint8_t id = p.rollBattleDrop(i & 1);
    CHECK_MSG(id != caramelo && id != pocion, "el botin dio un premio del quiz");
  }
}

// El Rare Candy sube UN nivel; la Shiny Potion deja shiny y no se gasta dos veces.
TEST(quiz_premio, caramelo_sube_uno_y_la_pocion_hace_shiny) {
  const uint8_t caramelo = quizItemDe(IT_RARECANDY);
  const uint8_t pocion = quizItemDe(IT_SHINY);
  Pet p;
  makePet(p, 4);
  p.exp = Pet::expForLevel(20);
  p.shiny = false;
  p.takeItem(caramelo);
  p.takeItem(pocion);
  p.takeItem(pocion);
  CHECK(p.useItem(caramelo));
  CHECK_EQ((int)p.level(), 21);
  CHECK_EQ((int)p.bag[caramelo], 0);
  CHECK(p.useItem(pocion));
  CHECK(p.shiny);
  CHECK_MSG(!p.useItem(pocion), "ya shiny, la segunda no se gasta");
  CHECK_EQ((int)p.bag[pocion], 1);
}

TEST(ceremony, bien_cuidado_no_llama) {
  Pet p;
  makePet(p, 4);
  setStats(p, 80, 80, 80, 80);
  bool aviso = false;
  for (int t = 0; t < 2 * ABANDONO_AVISO_TICKS; t++) {
    advance(p, 1);
    aviso |= p.tomaAvisoAbandono();
  }
  CHECK(!aviso);
}

TEST(ceremony, durante_la_ceremonia_no_se_puede_interactuar) {
  Pet p;
  makePet(p, 4);
  setStats(p, 50, 50, 50, 50);
  p.poops = 2;
  p.startRunaway();
  p.feedBerry(0);
  p.feedCandy();
  p.play();
  p.clean();
  p.caress();
  p.toggleLight();
  CHECK_EQ(p.fullness, (uint8_t)50);
  CHECK_EQ(p.joy, (uint8_t)50);
  CHECK_EQ(p.poops, (uint8_t)2);
  CHECK(!p.sleeping);
}

TEST(ceremony, soltar_marca_el_final_como_liberacion) {
  Pet p;
  makePet(p, 4);
  p.release();
  CHECK_EQ(p.ceremony, (uint8_t)CER_RELEASE);
  CHECK_EQ(p.lastEnd, (uint8_t)CER_RELEASE);
  mockAdvanceMillis(CEREMONY_MS + 1);
  p.update(millis());
  CHECK(p.isEgg());
}

TEST(ceremony, un_huevo_no_se_despide_ni_se_suelta) {
  mockNvsReset();
  Pet p;
  p.begin();
  p.startFarewell();
  p.release();
  p.startRunaway();
  CHECK_EQ(p.ceremony, (uint8_t)CER_NONE);
}

// ---------------------------------------------------------------- offline
TEST(offline, aplica_el_tiempo_apagado_con_suelo_de_15) {
  Pet p;
  makePet(p, 4);
  setStats(p, 80, 80, 80, 100);
  p.setClock(1000u * 86400);
  p.syncClock(1000u * 86400 + 600 * 60);  // 600 minutos apagado
  CHECK_EQ(p.ageMinutes, (uint32_t)600);
  CHECK_MSG(p.fullness == 15, "suelo de comida offline = 15");
  CHECK_MSG(p.energy == 15, "suelo de energia offline = 15");
  CHECK_MSG(p.joy == 15, "suelo de felicidad offline = 15");
  CHECK_MSG(p.hygiene == 15, "suelo de higiene offline = 15");
  CHECK_MSG(p.careMistakes == 0, "en ausencia no se acumulan descuidos");
  CHECK_EQ(p.poops, (uint8_t)2);  // una caca cada 4 h
}

// LA PAUSA, que es lo unico que para el reloj. Apagar normal no pausa: el
// RTC sigue contando con la placa muerta y al encender se cobra el rato
// entero de golpe. Guardando, no.
TEST(offline, guardado_el_bicho_no_envejece_en_el_cajon) {
  Pet p;
  makePet(p, 4);
  setStats(p, 80, 80, 80, 100);
  p.setClock(1000u * 86400);
  p.guardar(1000u * 86400);                  // se apaga GUARDANDO
  p.syncClock(1000u * 86400 + 30u * 86400);  // un mes en un cajon
  CHECK_EQ(p.ageMinutes, (uint32_t)0);
  CHECK_MSG(p.fullness == 80, "guardado, la comida no baja");
  CHECK_MSG(p.energy == 80, "guardado, la energia no baja");
  CHECK_MSG(p.hygiene == 100, "guardado, la higiene no baja");
  CHECK_EQ(p.poops, (uint8_t)0);
  // la marca se gasta: vale para UNA vuelta, no es un modo que se queda
  CHECK_MSG(!p.guardado, "la marca de guardado tenia que gastarse al volver");
}

// La otra mitad: sin guardar, el mismo rato SI pasa factura. Sin esto, un
// "arreglo" que se saltara siempre la progresion daria verde arriba.
TEST(offline, sin_guardar_el_mismo_rato_si_pasa_factura) {
  Pet p;
  makePet(p, 4);
  setStats(p, 80, 80, 80, 100);
  p.setClock(1000u * 86400);
  p.syncClock(1000u * 86400 + 30u * 86400);
  CHECK_MSG(p.fullness == 15, "sin guardar, la comida cae al suelo");
  CHECK_MSG(p.ageMinutes > 0, "sin guardar, el bicho envejece");
}

// El de arriba prueba la LOGICA, los dos objetos son el mismo y la marca
// nunca pasa por la NVS. Esto prueba lo que de verdad pasa al apagar: se
// guarda, se corta la corriente, y al volver el firmware hace begin() (que
// lee de la NVS) y luego syncClock(). Si la marca no sobrevive al viaje, el
// bicho vuelve hecho un cristo igual.
TEST(offline, la_pausa_sobrevive_al_apagon) {
  mockNvsReset();
  {
    Pet a;
    a.begin();
    // Hay que SALIR de la eleccion de inicial. Sin esto starterPick se
    // queda puesto, syncClock se va por su early-return y el test daba
    // verde sin probar la pausa: los PS no bajaban por otro motivo.
    if (a.awaitingStarter()) a.chooseStarter(4);
    while (a.isEgg()) a.eggTap();
    CHECK_MSG(!a.awaitingStarter(), "sigue en la eleccion de inicial");
    a.ceremony = CER_NONE;
    setStats(a, 80, 80, 80, 100);
    a.setClock(1000u * 86400);
    a.guardar(1000u * 86400);      // "PAUSE" y corte de corriente
  }
  // ...y vuelve a encenderse: objeto NUEVO, leido de la NVS
  Pet b;
  b.begin();
  CHECK_MSG(b.guardado, "la marca de pausa no sobrevivio a la NVS");
  b.syncClock(1000u * 86400 + 30u * 86400);   // un mes en el cajon
  CHECK_MSG(b.fullness == 80, "guardado, la comida no podia bajar");
  CHECK_MSG(b.hygiene == 100, "guardado, la higiene no podia bajar");
  CHECK_EQ(b.poops, (uint8_t)0);
  CHECK_MSG(!b.guardado, "la marca tenia que gastarse al volver");
}

// Un arranque que NO aplica tiempo no puede gastarse la pausa. El AXP2101
// vuelve a encender solo con que enchufes el USB, y ese rearranque se
// comia la marca sin usarla: la ausencia siguiente ya no estaba protegida
// y el bicho volvia rojo igual. Es lo que hacia que "lo puse para pausar y
// no pauso" fuera cierto aunque la logica pareciera bien.
TEST(offline, un_rearranque_en_seco_no_se_gasta_la_pausa) {
  mockNvsReset();
  {
    Pet a;
    a.begin();
    if (a.awaitingStarter()) a.chooseStarter(4);
    while (a.isEgg()) a.eggTap();
    setStats(a, 80, 80, 80, 100);
    a.setClock(1000u * 86400);
    a.guardar(1000u * 86400);
  }
  {
    // se enchufa el USB y la placa arranca sola: cero tiempo transcurrido
    Pet b;
    b.begin();
    b.syncClock(1000u * 86400 + 30);     // medio minuto
    CHECK_MSG(b.guardado, "el rearranque en seco se comio la pausa");
  }
  // y AHORA si, la ausencia de verdad
  Pet c;
  c.begin();
  CHECK_MSG(c.guardado, "la marca no llego hasta aqui");
  c.syncClock(1000u * 86400 + 3u * 86400);
  CHECK_MSG(c.fullness == 80, "la pausa no protegio: la comida bajo");
  CHECK_MSG(!c.guardado, "ahora SI tenia que gastarse");
}

TEST(offline, tope_de_dos_semanas) {
  Pet p;
  makePet(p, 4);
  p.setClock(1000u * 86400);
  p.syncClock(1000u * 86400 + 90u * 86400);  // 90 dias
  CHECK_EQ(p.ageMinutes, (uint32_t)(14u * 24 * 60));
}

TEST(offline, menos_de_dos_minutos_no_hace_nada) {
  Pet p;
  makePet(p, 4);
  setStats(p, 80, 80, 80, 100);
  p.setClock(1000u * 86400);
  p.syncClock(1000u * 86400 + 90);
  CHECK_EQ(p.ageMinutes, (uint32_t)0);
  CHECK_EQ(p.fullness, (uint8_t)80);
}

TEST(offline, el_huevo_eclosiona_en_tu_ausencia) {
  Pet p;
  makePet(p, 4);
  p.newEgg();  // ya hay pokedex: no pide inicial
  CHECK(!p.awaitingStarter());
  p.setClock(1000u * 86400);
  p.syncClock(1000u * 86400 + 600);  // 10 minutos
  CHECK(!p.isEgg());
  CHECK_RANGE(p.speciesId, (int16_t)1, (int16_t)151);
}

TEST(offline, durmiendo_se_descansa_tambien_apagado) {
  Pet p;
  makePet(p, 4);
  setStats(p, 90, 90, 20, 90);
  p.toggleLight();
  p.setClock(1000u * 86400);
  p.syncClock(1000u * 86400 + 120 * 60);
  CHECK_EQ(p.energy, (uint8_t)100);
  CHECK_MSG(p.fullness >= 30, "suelos de sueno tambien offline");
  CHECK_MSG(p.poops == 0, "durmiendo no ensucia");
}

TEST(offline, sin_reloj_no_se_aplica_nada) {
  Pet p;
  makePet(p, 4);
  setStats(p, 80, 80, 80, 100);
  p.syncClock(0);
  CHECK_EQ(p.ageMinutes, (uint32_t)0);
  CHECK_EQ(p.fullness, (uint8_t)80);
}

// ---------------------------------------------------------------- guardado
TEST(save, el_estado_sobrevive_a_un_reinicio) {
  Pet a;
  makePet(a, 25);  // PIKACHU
  a.ageMinutes = 1234;
  a.streak = 9;
  a.bestStreak = 11;
  a.bond = 44;
  a.trAtk = 17;
  a.weight = 33;
  a.trainHi = 21;
  a.rename("SPARKY");  // rename persiste todo el estado

  Pet b;
  b.begin();  // mismo "NVS": simula un reinicio
  CHECK_EQ(b.speciesId, (int16_t)25);
  CHECK_EQ(b.ageMinutes, (uint32_t)1234);
  CHECK_EQ(b.streak, (uint16_t)9);
  CHECK_EQ(b.bestStreak, (uint16_t)11);
  CHECK_EQ(b.bond, (uint8_t)44);
  CHECK_EQ(b.trAtk, (uint8_t)17);
  CHECK_EQ(b.weight, (uint8_t)33);
  CHECK_EQ(b.trainHi, (uint16_t)21);
  CHECK_STREQ(b.nick, "SPARKY");
  CHECK(b.isRegistered(25));
  CHECK(!b.awaitingStarter());
}

TEST(save, el_apodo_se_recorta_a_11_caracteres) {
  Pet p;
  makePet(p, 4);
  p.rename("NOMBRE-LARGUISIMO-QUE-NO-CABE");
  CHECK_EQ((int)strlen(p.nick), 11);
  CHECK_STREQ(p.nick, "NOMBRE-LARG");
}

TEST(save, borrado_de_fabrica_deja_todo_como_nuevo) {
  Pet a;
  makePet(a, 4);
  a.factoryReset();
  Pet b;
  b.begin();
  CHECK(b.isEgg());
  CHECK(b.awaitingStarter());
  CHECK_EQ(b.registeredCount(), (uint16_t)0);
}

TEST(save, el_autoguardado_se_marca_y_se_vuelca) {
  Pet p;
  makePet(p, 4);
  CHECK(!p.savePending());
  advance(p, 5);
  CHECK_MSG(p.savePending(), "cada 5 ticks queda un guardado pendiente");
  p.flushSave();
  CHECK(!p.savePending());
}

// ------------------------------------------------------------------- PP
// Los PP son los de siempre y ATRAVIESAN los combates: si se rellenasen solos
// al acabar cada uno, un Tackle de 35 no se agotaria jamas y el numero seria
// decoracion. Lo que los devuelve es cuidar al bicho, y eso es lo que se
// prueba aqui.

// mascota de la especie y el nivel pedidos, con los PP ya puestos al dia
static void makeLuchador(Pet &p, int16_t dex, uint16_t nivel) {
  makePet(p, dex);
  setLevel(p, nivel);
  p.ppSync();
  p.hpSync();   // saltar 20 niveles de golpe sube el maximo: hay que repartirlo
}

static uint16_t ppTotal(const Pet &p) {
  uint16_t t = 0;
  for (uint8_t i = 0; i < 4; i++) t = (uint16_t)(t + p.ppLeft[i]);
  return t;
}

static void vaciarPP(Pet &p) {
  for (uint8_t i = 0; i < 4; i++)
    if (p.ppMove[i]) p.ppLeft[i] = 0;
}

TEST(pp, arranca_lleno_y_con_los_numeros_del_pbs) {
  Pet p;
  makeLuchador(p, 4, 40);              // Charmander subido
  bool alguno = false;
  for (uint8_t i = 0; i < 4; i++) {
    if (!p.ppMove[i]) continue;
    alguno = true;
    CHECK_EQ((int)p.ppLeft[i], (int)movePP(p.ppMove[i]));
  }
  CHECK(alguno);
  CHECK(!p.ppEmpty());
}

TEST(pp, un_huevo_no_tiene_golpes) {
  Pet p;
  mockNvsReset();
  mockSetMillis(0);
  p.begin();
  if (p.awaitingStarter()) p.chooseStarter(4);
  p.ppSync();                          // sigue siendo huevo: nada que sincronizar
  for (uint8_t i = 0; i < 4; i++) CHECK_EQ((int)p.ppMove[i], 0);
  CHECK(p.ppEmpty());
}

TEST(pp, sincronizar_no_deshace_el_gasto) {
  Pet p;
  makeLuchador(p, 4, 40);
  uint8_t antes = p.ppLeft[0];
  CHECK(antes >= 4);
  p.ppLeft[0] = (uint8_t)(antes - 3);
  p.ppSync();
  CHECK_EQ((int)p.ppLeft[0], (int)(antes - 3));
}

TEST(pp, un_golpe_recien_aprendido_entra_lleno) {
  Pet p;
  makeLuchador(p, 4, 8);
  for (uint8_t i = 0; i < 4; i++) if (p.ppMove[i]) p.ppLeft[i] = 1;
  uint16_t viejos[4];
  for (uint8_t i = 0; i < 4; i++) viejos[i] = p.ppMove[i];

  setLevel(p, 61);   // de golpe al nivel 60
  p.ppSync();
  bool cambio = false;
  for (uint8_t i = 0; i < 4; i++) {
    if (!p.ppMove[i]) continue;
    if (p.ppMove[i] != viejos[i]) {
      cambio = true;
      CHECK_EQ((int)p.ppLeft[i], (int)movePP(p.ppMove[i]));  // el nuevo, lleno
    } else {
      CHECK_EQ((int)p.ppLeft[i], 1);                         // el viejo, gastado
    }
  }
  CHECK_MSG(cambio, "de nivel 8 a 60 tiene que aprender algo");
}

TEST(pp, dormir_devuelve_de_poco_en_poco) {
  Pet p;
  makeLuchador(p, 4, 40);
  vaciarPP(p);
  CHECK(p.ppEmpty());

  CHECK_EQ((int)p.ppRestore(1), 1);
  CHECK(!p.ppEmpty());
  CHECK_EQ((int)ppTotal(p), 1);        // una siesta corta no lo llena entero
}

TEST(pp, una_noche_entera_durmiendo_lo_llena) {
  Pet p;
  makeLuchador(p, 4, 40);
  vaciarPP(p);
  p.toggleLight();                     // luz apagada = a dormir
  advance(p, 8 * 60);                  // ocho horas de juego
  for (uint8_t i = 0; i < 4; i++)
    if (p.ppMove[i]) CHECK_EQ((int)p.ppLeft[i], (int)movePP(p.ppMove[i]));
}

TEST(pp, la_baya_reparte_por_el_mas_gastado) {
  Pet p;
  makeLuchador(p, 4, 40);
  CHECK(p.ppMove[0] != 0);
  p.ppLeft[0] = 0;                     // solo este vacio, el resto llenos
  CHECK_EQ((int)p.ppRestore(4), 4);
  CHECK_EQ((int)p.ppLeft[0], 4);       // los cuatro puntos van donde hacen falta
}

TEST(pp, reponer_con_todo_lleno_no_hace_nada) {
  Pet p;
  makeLuchador(p, 4, 40);
  CHECK_EQ((int)p.ppRestore(20), 0);
  for (uint8_t i = 0; i < 4; i++)
    if (p.ppMove[i]) CHECK_EQ((int)p.ppLeft[i], (int)movePP(p.ppMove[i]));
}

TEST(pp, nunca_pasa_del_tope) {
  Pet p;
  makeLuchador(p, 4, 40);
  p.ppRestore(255);
  p.ppRestoreAll();
  for (uint8_t i = 0; i < 4; i++)
    if (p.ppMove[i]) CHECK(p.ppLeft[i] <= movePP(p.ppMove[i]));
}

TEST(pp, la_baya_favorita_repone_mas) {
  Pet p, q;
  makeLuchador(p, 4, 40);
  uint8_t favorita = (uint8_t)(p.speciesId % 3);
  CHECK(p.lovesBerry(favorita));

  vaciarPP(p);
  p.feedBerry(favorita);
  uint16_t conFavorita = ppTotal(p);

  makeLuchador(q, 4, 40);
  vaciarPP(q);
  q.feedBerry((uint8_t)((favorita + 1) % 3));
  uint16_t conOtra = ppTotal(q);

  CHECK(conOtra > 0);
  CHECK(conFavorita > conOtra);
}

TEST(pp, sobrevive_a_un_reinicio) {
  Pet p;
  makeLuchador(p, 4, 40);
  CHECK(p.ppMove[0] != 0);
  uint16_t golpe = p.ppMove[0];
  p.ppLeft[0] = 2;
  p.clean();         // cualquier accion de cuidado vuelca la NVS

  Pet q;                               // misma NVS de mentira, sin resetear
  q.begin();
  CHECK_EQ((int)q.ppMove[0], (int)golpe);
  CHECK_EQ((int)q.ppLeft[0], 2);
}

// ------------------------------------------------------------------- PS
// Misma regla que los PP: atraviesan los combates y los devuelve el cuidado.
// Caer no es morir: el bicho se desmaya, se duerme solo y se levanta tocado.
// El precio es de GENERACION -un error de cuidado, que empeora el proximo
// huevo-, no del presente.

TEST(ps, arranca_entero) {
  Pet p;
  makeLuchador(p, 4, 20);
  CHECK_EQ((int)p.hpLeft, (int)p.maxHp());
  CHECK(p.canBattle());
}

TEST(ps, el_dano_no_se_deshace_al_acabar_el_combate) {
  Pet p;
  makeLuchador(p, 4, 40);
  uint16_t tope = p.maxHp();
  p.afterBattle((uint16_t)(tope / 2));
  CHECK_EQ((int)p.hpLeft, (int)(tope / 2));
  CHECK(p.canBattle());                      // medio lleno todavia pelea
}

TEST(ps, pelear_cansa_aunque_se_gane_limpio) {
  Pet p;
  makeLuchador(p, 4, 40);
  p.energy = 100;
  p.afterBattle(p.maxHp());                  // sin un rasguno
  CHECK_EQ((int)p.energy, 100 - BATTLE_ENERGY_COST);
  CHECK_EQ((int)p.hpLeft, (int)p.maxHp());
}

TEST(ps, muy_tocado_se_niega_a_pelear) {
  Pet p;
  makeLuchador(p, 4, 40);
  uint16_t tope = p.maxHp();
  p.afterBattle((uint16_t)((uint32_t)tope * (BATTLE_HP_FLOOR_PCT - 5) / 100));
  CHECK(!p.canBattle());
}

TEST(ps, caer_no_lo_deja_a_cero_ni_lo_mata) {
  Pet p;
  makeLuchador(p, 4, 40);
  int16_t especie = p.speciesId;
  uint16_t nivel = p.level();

  p.afterBattle(0);                          // cayo
  CHECK(p.hpLeft > 0);                       // se levanta tocado, no muerto
  CHECK(p.hpLeft < p.maxHp());
  CHECK(!p.canBattle());                     // y con el deposito por debajo del suelo
  CHECK_EQ((int)p.speciesId, (int)especie);  // no pierde especie
  CHECK_EQ((int)p.level(), (int)nivel);      // ni nivel
  CHECK_EQ((int)p.ceremony, (int)CER_NONE);  // ni se va a ningun lado
}

TEST(ps, caer_lo_duerme_y_le_baja_el_animo) {
  Pet p;
  makeLuchador(p, 4, 40);
  p.joy = 90;
  CHECK(!p.sleeping);
  p.afterBattle(0);
  CHECK_MSG(p.sleeping, "se desmaya: se duerme solo, y durmiendo se cura");
  CHECK_EQ((int)p.joy, 90 - FAINT_JOY_COST);
}

TEST(ps, caer_cuesta_en_la_generacion_siguiente) {
  // El unico castigo: cuenta como error de cuidado, y los errores empeoran el
  // sorteo del proximo huevo y la tirada de shiny.
  Pet p;
  makeLuchador(p, 4, 40);
  uint8_t antes = p.careMistakes;
  p.afterBattle(0);
  CHECK_EQ((int)p.careMistakes, (int)(antes + 1));
}

TEST(ps, dormir_lo_cura_entero) {
  Pet p;
  makeLuchador(p, 4, 40);
  p.afterBattle(1);
  CHECK(!p.canBattle());
  p.toggleLight();                 // a dormir (o ya lo esta, si se desmayo)
  if (!p.sleeping) p.toggleLight();
  advance(p, 3 * 60);              // tres horas de juego
  CHECK_EQ((int)p.hpLeft, (int)p.maxHp());
  p.toggleLight();                 // despierta: dormido no pelea nadie
  CHECK(p.canBattle());
}

TEST(ps, la_baya_cura_un_pedazo_y_la_favorita_el_doble) {
  Pet p, q;
  makeLuchador(p, 4, 40);
  uint8_t favorita = (uint8_t)(p.speciesId % 3);
  p.afterBattle(1);
  if (p.sleeping) p.toggleLight();       // despierto para poder comer
  uint16_t base = p.hpLeft;
  p.feedBerry(favorita);
  uint16_t conFavorita = (uint16_t)(p.hpLeft - base);

  makeLuchador(q, 4, 40);
  q.afterBattle(1);
  if (q.sleeping) q.toggleLight();
  uint16_t base2 = q.hpLeft;
  q.feedBerry((uint8_t)((favorita + 1) % 3));
  uint16_t conOtra = (uint16_t)(q.hpLeft - base2);

  CHECK(conOtra > 0);
  // "el doble" con margen: los dos son porcentajes enteros del maximo y el
  // redondeo hacia abajo se come hasta un par de puntos
  CHECK(conFavorita >= conOtra * 2);
  CHECK(conFavorita <= conOtra * 2 + 2);
}

TEST(ps, despierto_se_recupera_solo_para_no_atascarse) {
  // Sin esto, un bicho al que no se le hace nada podria quedarse para siempre
  // sin poder pelear. Es lento a proposito: compensa dormir o darle de comer.
  Pet p;
  makeLuchador(p, 4, 40);
  p.afterBattle(1);
  if (p.sleeping) p.toggleLight();
  uint16_t antes = p.hpLeft;
  advance(p, HP_HEAL_AWAKE_MIN * 4);
  CHECK(p.hpLeft > antes);
}

TEST(ps, subir_de_nivel_suma_a_los_de_ahora) {
  // Como en los juegos: la subida se suma a los PS actuales. Si no, el bicho
  // iria pareciendo cada vez mas tocado sin que nadie le hiciera nada.
  Pet p;
  makeLuchador(p, 4, 20);
  uint16_t tope1 = p.maxHp();
  p.afterBattle((uint16_t)(tope1 / 2));
  uint16_t antes = p.hpLeft;

  setLevel(p, 41);
  p.hpSync();
  uint16_t tope2 = p.maxHp();
  CHECK(tope2 > tope1);
  CHECK_EQ((int)p.hpLeft, (int)(antes + (tope2 - tope1)));
  CHECK(p.hpLeft < tope2);                   // sigue tocado, no se cura gratis
}

TEST(ps, nunca_pasa_del_tope) {
  Pet p;
  makeLuchador(p, 4, 40);
  p.hpHeal(60000);
  CHECK_EQ((int)p.hpLeft, (int)p.maxHp());
  p.afterBattle(60000);                      // valor imposible desde el combate
  CHECK_EQ((int)p.hpLeft, (int)p.maxHp());
}

TEST(ps, un_huevo_no_tiene_ps_ni_pelea) {
  Pet p;
  mockNvsReset();
  mockSetMillis(0);
  p.begin();
  if (p.awaitingStarter()) p.chooseStarter(4);
  p.hpSync();
  CHECK_EQ((int)p.hpLeft, 0);
  CHECK(!p.canBattle());
}

TEST(ps, sobreviven_a_un_reinicio) {
  Pet p;
  makeLuchador(p, 4, 40);
  p.afterBattle((uint16_t)(p.maxHp() / 3));
  uint16_t quedaron = p.hpLeft;

  Pet q;
  q.begin();
  CHECK_EQ((int)q.hpLeft, (int)quedaron);
}

// ---------------------------------------------------------- objetos
TEST(objetos, la_piedra_eterna_impide_evolucionar) {
  Pet p;
  makeLuchador(p, 4, 40);               // Charmander muy pasado de nivel
  setStats(p, 90, 90, 90, 90);
  CHECK_MSG(p.canEvolveNow(), "sin nada encima tendria que poder");

  uint8_t piedra = 0;
  for (uint8_t i = 1; i < ITEM_COUNT; i++)
    if (ITEM_TBL[i].fx == IT_EVERSTONE) piedra = i;
  CHECK(piedra != 0);
  p.takeItem(piedra);
  p.holdItem(piedra);
  CHECK_MSG(!p.canEvolveNow(), "con la piedra encima, no");
}

TEST(objetos, la_bolsa_y_lo_que_lleva_puesto_cuadran) {
  Pet p;
  makeLuchador(p, 4, 20);
  CHECK(!p.holdItem(3));                // no se puede llevar lo que no se tiene

  CHECK(p.takeItem(3));
  CHECK_EQ((int)p.bag[3], 1);
  CHECK(p.holdItem(3));
  CHECK_EQ((int)p.heldItem, 3);
  CHECK_EQ((int)p.bag[3], 0);           // sale de la bolsa

  CHECK(p.takeItem(5));
  CHECK(p.holdItem(5));                 // cambiar devuelve el anterior
  CHECK_EQ((int)p.heldItem, 5);
  CHECK_EQ((int)p.bag[3], 1);

  CHECK(p.holdItem(0));                 // quitarselo
  CHECK_EQ((int)p.heldItem, 0);
  CHECK_EQ((int)p.bag[5], 1);
}

TEST(objetos, lo_que_lleva_sobrevive_a_un_reinicio) {
  Pet p;
  makeLuchador(p, 4, 20);
  p.takeItem(7);
  p.holdItem(7);
  p.takeItem(9);
  p.takeItem(9);

  Pet q;
  q.begin();
  CHECK_EQ((int)q.heldItem, 7);
  CHECK_EQ((int)q.bag[9], 2);
}

TEST(objetos, ganar_combates_es_lo_que_los_da) {
  randomSeed(129);
  Pet p;
  makeLuchador(p, 4, 20);
  int encontrados = 0;
  for (int i = 0; i < 200; i++) if (p.rollBattleDrop()) encontrados++;
  CHECK_MSG(encontrados > 20, "tiene que caer algo de vez en cuando");
  CHECK_MSG(encontrados < 160, "pero no en casi todos los combates");
}

// ----------------------------------------------------------- la bolsa
// Por dentro es un contador por objeto (un byte cada uno, sin ordenar nada);
// los bolsillos son una VISTA encima, que es lo que hace falta para pintarlos.

static uint8_t primeroDe(uint8_t pocket) {
  for (uint8_t i = 1; i < ITEM_COUNT; i++)
    if (ITEM_TBL[i].pocket == pocket) return i;
  return 0;
}

static uint8_t porEfecto(uint8_t fx) {
  for (uint8_t i = 1; i < ITEM_COUNT; i++)
    if (ITEM_TBL[i].fx == fx) return i;
  return 0;
}

TEST(bolsa, hay_de_los_cinco_bolsillos) {
  for (uint8_t b = 0; b < BP_COUNT; b++) {
    uint8_t n = 0;
    for (uint8_t i = 1; i < ITEM_COUNT; i++) if (ITEM_TBL[i].pocket == b) n++;
    CHECK_MSG(n > 0, "un bolsillo vacio en la tabla no tiene sentido");
    CHECK(POCKET_NAME[b] != nullptr && POCKET_NAME[b][0] != 0);
  }
}

TEST(bolsa, solo_se_ve_lo_que_se_tiene_y_en_su_bolsillo) {
  Pet p;
  makeLuchador(p, 4, 20);
  // Os itens-chave sao de enfeite e estao sempre la; o resto, vazio.
  int chaves = 0;
  for (uint8_t i = 1; i < ITEM_COUNT; i++) if (ITEM_TBL[i].fx == IT_KEY) chaves++;
  CHECK_MSG(chaves >= 3, "premissa: Town Map, Pokedex e Mega Ring");
  // O pacote de bolas novas (BALLX_WELCOME) cai em toda partida: fora,
  // para o bolso comecar vazio como o resto.
  for (uint8_t i = 1; i < ITEM_COUNT; i++)
    if (ITEM_TBL[i].pocket == BP_BALL) p.bag[i] = 0;
  for (uint8_t b = 0; b < BP_COUNT; b++)
    CHECK_EQ((int)p.pocketCount(b), b == BP_KEY ? chaves : 0);

  uint8_t pocion = porEfecto(IT_POTION);
  uint8_t bola = primeroDe(BP_BALL);
  p.takeItem(pocion);
  p.takeItem(pocion);
  p.takeItem(bola);

  CHECK_EQ((int)p.pocketCount(BP_BATTLE), 1);   // dos unidades, UN tipo
  CHECK_EQ((int)p.pocketCount(BP_BALL), 1);
  CHECK_EQ((int)p.pocketCount(BP_HELD), 0);
  CHECK_EQ((int)p.pocketItem(BP_BATTLE, 0), (int)pocion);
  CHECK_EQ((int)p.bag[pocion], 2);
  CHECK_EQ((int)p.pocketItem(BP_BATTLE, 1), 0); // no hay un segundo tipo
}

TEST(bolsa, la_pocion_cura_y_no_se_malgasta) {
  Pet p;
  makeLuchador(p, 4, 40);
  uint8_t pocion = porEfecto(IT_POTION);
  p.takeItem(pocion);

  // entero: no deja usarla, para no tirarla a la basura
  CHECK(!p.useItem(pocion));
  CHECK_EQ((int)p.bag[pocion], 1);

  p.afterBattle((uint16_t)(p.maxHp() / 2));
  uint16_t antes = p.hpLeft;
  CHECK(p.useItem(pocion));
  CHECK(p.hpLeft > antes);
  CHECK_EQ((int)p.bag[pocion], 0);
}

TEST(bolsa, el_eter_devuelve_pp) {
  Pet p;
  makeLuchador(p, 4, 40);
  uint8_t eter = porEfecto(IT_ETHER);
  p.takeItem(eter);

  CHECK(!p.useItem(eter));            // con los PP llenos no hace nada
  for (uint8_t i = 0; i < 4; i++) if (p.ppMove[i]) p.ppLeft[i] = 0;
  CHECK(p.useItem(eter));
  uint16_t total = 0;
  for (uint8_t i = 0; i < 4; i++) total = (uint16_t)(total + p.ppLeft[i]);
  CHECK(total > 0);
}

TEST(bolsa, el_elixir_maximo_llena_los_cuatro) {
  Pet p;
  makeLuchador(p, 4, 40);
  uint8_t maxElixir = 0;
  for (uint8_t i = 1; i < ITEM_COUNT; i++)
    if (ITEM_TBL[i].fx == IT_ELIXIR && ITEM_TBL[i].arg == 255) maxElixir = i;
  CHECK(maxElixir != 0);
  p.takeItem(maxElixir);

  for (uint8_t i = 0; i < 4; i++) if (p.ppMove[i]) p.ppLeft[i] = 0;
  CHECK(p.useItem(maxElixir));
  for (uint8_t i = 0; i < 4; i++)
    if (p.ppMove[i]) CHECK_EQ((int)p.ppLeft[i], (int)movePP(p.ppMove[i]));
}

TEST(bolsa, el_revivir_solo_sirve_estando_en_los_huesos) {
  Pet p;
  makeLuchador(p, 4, 40);
  uint8_t revivir = porEfecto(IT_REVIVE);
  p.takeItem(revivir);
  p.takeItem(revivir);

  p.afterBattle((uint16_t)(p.maxHp() / 2));
  CHECK_MSG(!p.useItem(revivir), "a media vida no hace falta revivir a nadie");

  p.afterBattle(0);                   // se desmayo: cae y se duerme solo
  CHECK(!p.canBattle());
  CHECK(p.useItem(revivir));
  // revivir devuelve los PS, no el fuelle ni lo despierta: eso es cosa del
  // cuidado, que es justo lo que la energia como portero quiere forzar
  p.toggleLight();
  p.energy = 100;
  CHECK(p.canBattle());
}

TEST(bolsa, no_se_puede_usar_lo_que_no_se_tiene) {
  Pet p;
  makeLuchador(p, 4, 40);
  p.afterBattle(1);
  CHECK(!p.useItem(porEfecto(IT_POTION)));
  CHECK(!p.useItem(0));
  CHECK(!p.useItem(250));
}

TEST(bolsa, los_drops_caen_repartidos_entre_bolsillos) {
  randomSeed(141);
  Pet p;
  makeLuchador(p, 4, 20);
  for (int i = 0; i < 400; i++) p.rollBattleDrop();

  uint8_t conAlgo = 0;
  for (uint8_t b = 0; b < BP_COUNT; b++) if (p.pocketCount(b)) conAlgo++;
  CHECK_MSG(conAlgo >= 3, "en 400 combates tienen que tocar varios bolsillos");
  CHECK_MSG(p.pocketCount(BP_BATTLE) > 0, "lo mas comun son pociones y curas");
}

// ------------------------------------------------- elegir los cuatro golpes
// Antes los cuatro huecos eran los cuatro ULTIMOS aprendidos y el jugador no
// pintaba nada. A nivel 50 un bicho conoce 17 de media, asi que quedarse con
// los ultimos cuatro tiraba casi todo a la basura.

TEST(elegir, la_lista_completa_es_mucho_mayor_que_cuatro) {
  uint16_t todos[40], cuatro[4];
  uint8_t n = movesKnown(6, 50, todos, 40);       // Charizard
  uint8_t c = movesAtLevel(6, 50, cuatro);
  CHECK(c <= 4);
  CHECK_MSG(n > 8, "si no, no hacia falta pantalla para elegir");
  // los cuatro automaticos tienen que estar todos en la lista completa
  for (uint8_t i = 0; i < c; i++) {
    bool esta = false;
    for (uint8_t k = 0; k < n; k++) if (todos[k] == cuatro[i]) esta = true;
    CHECK(esta);
  }
}

TEST(elegir, sin_tocar_nada_sigue_siendo_automatico) {
  Pet p;
  makeLuchador(p, 4, 30);
  CHECK(!p.movesPicked);
  uint16_t antes[4];
  for (uint8_t i = 0; i < 4; i++) antes[i] = p.ppMove[i];

  setLevel(p, 61);        // sube mucho de nivel
  p.ppSync();
  bool cambio = false;
  for (uint8_t i = 0; i < 4; i++) if (p.ppMove[i] != antes[i]) cambio = true;
  CHECK_MSG(cambio, "sin elegir, los nuevos entran solos como antes");
}

TEST(elegir, en_cuanto_eliges_mandas_tu) {
  Pet p;
  makeLuchador(p, 4, 30);
  CHECK(p.unequipMove(0));
  CHECK(p.movesPicked);
  CHECK_EQ((int)p.ppMove[0], 0);

  uint16_t resto[4];
  for (uint8_t i = 0; i < 4; i++) resto[i] = p.ppMove[i];
  setLevel(p, 61);
  p.ppSync();
  for (uint8_t i = 0; i < 4; i++)
    CHECK_MSG(p.ppMove[i] == resto[i], "un golpe nuevo no puede echar al elegido");
}

TEST(elegir, poner_y_quitar) {
  Pet p;
  makeLuchador(p, 4, 40);
  uint16_t todos[40];
  uint8_t n = movesKnown(p.speciesId, p.level(), todos, 40);
  CHECK(n >= 5);

  // vaciar los cuatro
  for (uint8_t i = 0; i < 4; i++) if (p.ppMove[i]) p.unequipMove(i);
  for (uint8_t i = 0; i < 4; i++) CHECK_EQ((int)p.ppMove[i], 0);

  for (uint8_t i = 0; i < 4; i++) CHECK(p.equipMove(todos[i]));
  CHECK_MSG(!p.equipMove(todos[4]), "con los cuatro llenos hay que soltar antes");
  CHECK(p.moveEquipped(todos[0]));
  CHECK(!p.moveEquipped(todos[4]));

  CHECK_MSG(!p.equipMove(todos[0]), "no se puede poner dos veces el mismo");
}

TEST(elegir, el_que_entra_lo_hace_con_los_pp_llenos) {
  Pet p;
  makeLuchador(p, 4, 40);
  uint16_t todos[40];
  movesKnown(p.speciesId, p.level(), todos, 40);
  p.unequipMove(0);
  CHECK(p.equipMove(todos[0]));
  for (uint8_t i = 0; i < 4; i++)
    if (p.ppMove[i] == todos[0]) CHECK_EQ((int)p.ppLeft[i], (int)movePP(todos[0]));
}

TEST(elegir, se_puede_ir_con_menos_de_cuatro) {
  Pet p;
  makeLuchador(p, 4, 40);
  for (uint8_t i = 1; i < 4; i++) if (p.ppMove[i]) p.unequipMove(i);
  p.ppSync();
  CHECK_MSG(p.ppMove[1] == 0, "dejar huecos vacios es una eleccion valida");
  CHECK(p.ppMove[0] != 0);
  CHECK(!p.ppEmpty());
}

TEST(elegir, la_eleccion_sobrevive_a_un_reinicio) {
  Pet p;
  makeLuchador(p, 4, 40);
  p.unequipMove(1);
  uint16_t esperado[4];
  for (uint8_t i = 0; i < 4; i++) esperado[i] = p.ppMove[i];

  Pet q;
  q.begin();
  CHECK(q.movesPicked);
  for (uint8_t i = 0; i < 4; i++) CHECK_EQ((int)q.ppMove[i], (int)esperado[i]);
}

TEST(elegir, otro_bicho_vuelve_a_empezar) {
  Pet p;
  makeLuchador(p, 4, 40);
  p.unequipMove(0);
  CHECK(p.movesPicked);
  p.release();                     // esto solo ARRANCA la ceremonia
  advance(p, 1);                   // ...y al acabar queda un huevo nuevo
  CHECK(p.isEgg());
  CHECK_MSG(!p.movesPicked, "la eleccion era de aquel bicho, no del jugador");
}

TEST(level, una_partida_vieja_no_pierde_su_nivel) {
  // Antes el nivel era `1 + ageMinutes/60` y no habia clave "exp" en la NVS.
  // Al actualizar el firmware hay que convertir, o el jugador abre el juego y
  // se encuentra su bicho de nivel 40 en el nivel 1.
  mockNvsReset();
  mockSetMillis(0);
  {
    Pet viejo;
    viejo.begin();
    if (viejo.awaitingStarter()) viejo.chooseStarter(4);
    viejo.eggTap(); viejo.eggTap(); viejo.eggTap();
    viejo.ageMinutes = 39 * 60;     // lo que antes era nivel 40
    viejo.clean();                  // vuelca a la NVS
  }
  {
    // se le quita la clave a mano: asi queda un save igual a los de antes
    Preferences px;
    px.begin("tamapoke", false);
    px.remove("exp");
    px.end();
  }

  Pet nuevo;
  nuevo.begin();
  CHECK_EQ((int)nuevo.level(), 40);
  CHECK_EQ((int)nuevo.exp, (int)Pet::expForLevel(40));
  CHECK_MSG(nuevo.ageMinutes == 39u * 60, "la edad se conserva tal cual");
}

// ------------------------------------------------------------------ EVs
// El entrenamiento es elegir a QUIEN cazas: cada especie entrena aquello en
// lo que destaca. Se saca de su stat base mas alto, sin datos nuevos.

TEST(ev, cada_especie_entrena_lo_suyo) {
  CHECK_EQ((int)Pet::evYieldOf(74), (int)EV_DEF);    // Geodude
  CHECK_EQ((int)Pet::evYieldOf(66), (int)EV_ATK);    // Machop
  CHECK_EQ((int)Pet::evYieldOf(63), (int)EV_SPA);    // Abra
  CHECK_EQ((int)Pet::evYieldOf(50), (int)EV_SPE);    // Diglett
  CHECK_EQ((int)Pet::evYieldOf(72), (int)EV_SPD);    // Tentacool
  CHECK_EQ((int)Pet::evYieldOf(95), (int)EV_DEF);    // Onix
  CHECK_EQ((int)Pet::evYieldOf(143), (int)EV_HP);    // Snorlax: ya tiene su casilla
  CHECK_EQ((int)Pet::evYieldOf(113), (int)EV_HP);    // Chansey
  CHECK_EQ((int)Pet::evYieldOf(0), (int)EV_ATK);     // dex invalido, sin reventar
  // Un numero FUERA de la dex, sacado de DEX_COUNT. Estaba escrito 999,
  // que era imposible con 151 especies y con Paldea es Gimmighoul.
  CHECK_EQ((int)Pet::evYieldOf(DEX_COUNT + 1), (int)EV_ATK);
}

TEST(ev, entrenar_sube_el_stat_de_verdad) {
  Pet p;
  makeLuchador(p, 4, 30);
  p.trAtk = 0;
  uint16_t antes = p.atkStat();
  for (int i = 0; i < 40; i++) p.gainEV(EV_ATK, 1);
  CHECK_EQ((int)p.trAtk, 40);
  CHECK_MSG(p.atkStat() > antes, "los EVs tienen que notarse en el stat");
}

TEST(ev, el_especial_ya_tiene_los_suyos) {
  // Antes spAtkStat() usaba trAtk y spDefStat() usaba trDef: eran 6 stats
  // pero 3 EVs, y con eso Proteina y Calcio habrian hecho lo mismo. Ahora
  // cada uno va por su lado y se puede criar a un bicho especial.
  Pet p;
  makeLuchador(p, 4, 30);
  p.trAtk = p.trSpa = p.trDef = p.trSpd = 0;
  uint16_t atkAntes = p.atkStat(), spaAntes = p.spAtkStat();

  p.gainEV(EV_SPA, 40);
  CHECK_EQ((int)p.trSpa, 40);
  CHECK_MSG((int)p.trAtk == 0, "entrenar el especial no toca el fisico");
  CHECK(p.spAtkStat() > spaAntes);
  CHECK_EQ((int)p.atkStat(), (int)atkAntes);

  p.gainEV(EV_SPD, 7);
  CHECK_EQ((int)p.trSpd, 7);
  CHECK_EQ((int)p.trDef, 0);
}

TEST(ev, los_ps_tambien_se_entrenan) {
  Pet p;
  makeLuchador(p, 4, 30);
  p.trHp = 0;
  uint16_t antes = p.maxHp();
  p.gainEV(EV_HP, 100);
  CHECK_EQ((int)p.trHp, 100);
  CHECK_MSG(p.maxHp() > antes, "el EV de PS tiene que subir el maximo");
  CHECK_MSG(p.maxHp() - antes <= 25, "pero sin convertir a cualquiera en Snorlax");
}

TEST(ev, hay_techo_y_no_se_desborda) {
  Pet p;
  makeLuchador(p, 4, 30);
  p.trSpe = 95;
  CHECK_EQ((int)p.gainEV(EV_SPE, 20), 5);   // solo entran los que caben
  CHECK_EQ((int)p.trSpe, 100);
  CHECK_EQ((int)p.gainEV(EV_SPE, 20), 0);   // lleno: no entra nada
}

// ------------------------------------------------------ salvajes
// El "!" que aparece de vez en cuando. Lo que se fija aqui es el REPARTO de
// especies (que es la progresion por Kanto) y que el aviso caduque solo.

static uint16_t bstDe(int16_t d) {
  const DexEntry &e = DEX_TBL[d];
  return (uint16_t)e.bHp + e.bAtk + e.bDef + e.bSpA + e.bSpD + e.bSpe;
}

TEST(salvaje, al_principio_solo_sale_el_principio_de_kanto) {
  Pet p;
  makePet(p, 4);
  p.badges = 0;
  // Lo que define la primera zona: bichos flojos si, evolucionados no.
  CHECK_MSG(p.wildWeight(16, 12) > 0, "Pidgey tiene que salir ya");   // BST 266
  CHECK_MSG(p.wildWeight(19, 12) > 0, "Rattata tambien");             // BST 253
  CHECK_MSG(p.wildWeight(18, 12) == 0, "Pidgeot no, todavia");        // BST 529
  CHECK_MSG(p.wildWeight(149, 12) == 0, "ni Dragonite");              // BST 600
  // El techo es el de TU region: ya no hay una tabla unica.
  CHECK_EQ((int)p.wildBstCap(), (int)REGIONS[0].bstCap[0]);
}

TEST(salvaje, la_progresion_reparte_de_verdad) {
  // Con una rampa lineal de BST, cinco insignias ya abrian 138 de 151 y las
  // tres ultimas no daban nada: los techos son percentiles del catalogo.
  Pet p;
  makePet(p, 4);
  // O que esta ABERTO: o peso comum, mais os lendarios que a regiao
  // terminada libera —— esses saem pela rolagem propria (rollLegendary)
  // e nao pelo peso, mas a oitava insignia e justamente o que os solta.
  auto aberto = [&](int16_t d) {
    if (p.wildWeight(d, 12)) return true;
    return DEX_TBL[d].rarity == R_LEGENDARIO && p.badges >= GYM_COUNT &&
           dexRegionOf(d) <= p.maxRegion;
  };
  int prev = 0;
  for (uint8_t b = 0; b <= GYM_COUNT; b++) {
    p.badges = b;
    int n = 0;
    for (int16_t d = 1; d <= DEX_COUNT; d++) if (aberto(d)) n++;
    CHECK_MSG(n > prev, "cada insignia tiene que abrir algo");
    if (b > 0 && b < GYM_COUNT)
      CHECK_MSG(n - prev >= 10, "y no un goteo simbolico");
    prev = n;
  }
  // Con las ocho se abre TU region entera, no el catalogo entero: la
  // hierba ya no adelanta regiones. El catalogo completo pide ademas
  // haber llegado a la ultima.
  CHECK_EQ(prev, (int)DEX_REGION_END[0]);
  // El catalogo entero pide haber DESTRABADO hasta la ultima, que es lo
  // que mira la puerta de la hierba (antes esto era `region`).
  p.maxRegion = DEX_REGIONS - 1;
  p.region = DEX_REGIONS - 1;
  int n = 0;
  for (int16_t d = 1; d <= DEX_COUNT; d++) if (aberto(d)) n++;
  CHECK_EQ(n, (int)DEX_COUNT);
}

// EL MAPA, por el lado de la hierba. Lo pedido era "una vez alli, la
// incidencia de los pokemon de la region sube, PERO AUN APARECERIAN los de
// otra region". Las dos mitades de esa frase son las dos afirmaciones de
// abajo, y la segunda es la que se rompe sola: lo facil es poner un
// `return 0` y convertir el mapa en una puerta.
TEST(salvaje, la_zona_sube_los_de_casa_sin_echar_a_los_demas) {
  Pet p;
  makePet(p, 4);
  // `region` es DONDE ESTAS y `maxRegion` hasta donde has destrabado:
  // antes esto eran `region` y `zone` al reves.
  p.maxRegion = 1;           // Johto abierto
  p.region = 1;              // y estas alli
  p.badges = GYM_COUNT;      // sin techo de BST, que aqui estorba

  int deJohto = 0, deKanto = 0;
  for (int16_t d = 1; d <= DEX_COUNT; d++) {
    uint8_t w = p.wildWeight(d, 12);
    if (!w) continue;
    if (dexRegionOf(d) == 1) deJohto += w;
    else if (dexRegionOf(d) == 0) deKanto += w;
  }
  CHECK_MSG(deKanto > 0,
            "estando en Johto no puede desaparecer Kanto de la hierba: "
            "eso es una puerta, no un mapa");

  // y ahora de vuelta a Kanto, con lo mismo abierto
  p.region = 0;
  int deJohto2 = 0, deKanto2 = 0;
  for (int16_t d = 1; d <= DEX_COUNT; d++) {
    uint8_t w = p.wildWeight(d, 12);
    if (!w) continue;
    if (dexRegionOf(d) == 1) deJohto2 += w;
    else if (dexRegionOf(d) == 0) deKanto2 += w;
  }
  CHECK_MSG(deJohto2 > 0, "y al reves tampoco: Johto sigue estando");
  CHECK_MSG(deKanto2 > deKanto && deJohto > deJohto2,
            "viajar tiene que NOTARSE: en Kanto pesa Kanto (" +
            std::to_string(deKanto2) + " vs " + std::to_string(deKanto) +
            ") y en Johto pesa Johto (" + std::to_string(deJohto) + " vs " +
            std::to_string(deJohto2) + ")");
}

TEST(salvaje, no_se_viaja_a_donde_no_has_llegado) {
  Pet p;
  makePet(p, 4);
  p.maxRegion = 1;           // Johto destrabado
  p.region = 0;              // pero estas en Kanto
  CHECK_MSG(!p.setZone(5), "Kalos no esta abierto y lo dejo entrar");
  CHECK_EQ((int)p.region, 0);
  CHECK_MSG(p.setZone(1), "Johto SI esta abierto");
  CHECK_EQ((int)p.region, 1);
  CHECK_MSG(!p.setZone(1), "volver a elegir donde ya estas no es un viaje");
  // Y viajar NO cuesta insignias. Antes advanceRegion las ponia a cero;
  // ahora cada region guarda las suyas y volver te las devuelve, que es
  // lo que pidio el usuario.
  p.setZone(0);
  p.badges = p.gymCount();
  p.gymTrainers = GYM_TRAINERS;
  const uint8_t deKanto = p.badges;
  if (p.canAdvanceRegion()) {
    CHECK_MSG(p.advanceRegion(), "no se pudo viajar a la region siguiente");
    CHECK_MSG(p.badgesOf(0) == deKanto,
              "viajar borro las insignias de Kanto: quedan " +
                  std::to_string((int)p.badgesOf(0)));
    CHECK_MSG(p.setZone(0), "no se pudo volver");
    CHECK_MSG(p.badges == deKanto,
              "volver no devolvio las insignias de Kanto");
  }
}

// EL CENTRO POKEMON. Lo que arregla es concreto: el banquillo no dormia, o
// sea que no habia NINGUNA forma de levantar a un suplente caido.
TEST(centro, la_enfermera_levanta_al_banquillo) {
  Pet p;
  makePet(p, 4);
  setLevel(p, 20);
  p.hpLeft = 1;
  // un suplente, tumbado
  p.team[1].species = 16;
  p.team[1].exp = Pet::expForLevel(12);
  p.team[1].hpLeft = 0;
  uint16_t topeSupl = hpAtLevelForm(16, 12, p.team[1].trHp, p.team[1].form);

  CHECK_MSG(p.needsHealing(), "con el titular a 1 PS y un suplente a cero, "
                              "la enfermera tiene que tener trabajo");
  uint8_t n = p.healAll();
  CHECK_EQ((int)p.hpLeft, (int)p.maxHp());
  CHECK_EQ((int)p.team[1].hpLeft, (int)topeSupl);
  CHECK_MSG(n == 2, "curo " + std::to_string((int)n) +
                    " y eran dos: el titular y el suplente");
  CHECK_MSG(!p.needsHealing(), "despues de curar no puede quedar nada");
}

TEST(centro, no_se_asoma_si_no_hace_falta) {
  Pet p;
  makePet(p, 4);
  setLevel(p, 20);
  p.hpLeft = p.maxHp();
  p.ppRestoreAll();
  CHECK_MSG(!p.needsHealing(), "todo lleno y dice que hay que curar");
  p.pcTick(1000);
  CHECK_MSG(!p.pcWaiting(1000 + PC_EVERY_MS + 1),
            "se planto una enfermera que no tenia nada que hacer");

  // en cuanto hay algo roto, se programa la visita
  p.hpLeft = 1;
  p.pcTick(1000);
  CHECK_MSG(p.pcWaiting(1000 + PC_EVERY_MS + 100),
            "con el bicho a 1 PS la visita tiene que llegar");
  // y se va sola pasado su rato
  CHECK_MSG(!p.pcWaiting(1000 + PC_EVERY_MS + PC_STAY_MS + 100),
            "la enfermera se queda para siempre");
}

TEST(centro, curar_tambien_llega_a_la_caja) {
  // La caja es donde acaba todo lo que capturas. Un bicho desmayado ahi
  // dentro no se cura con NADA mas: ni duerme, ni pelea, ni come.
  Pet p;
  makePet(p, 4);
  setLevel(p, 20);
  p.bank[3].species = 25;
  p.bank[3].exp = Pet::expForLevel(9);
  p.bank[3].hpLeft = 0;
  uint16_t tope = hpAtLevelForm(25, 9, p.bank[3].trHp, p.bank[3].form);
  p.healAll();
  CHECK_MSG(p.bank[3].hpLeft == tope,
            "el de la caja se quedo a " + std::to_string((int)p.bank[3].hpLeft) +
            " de " + std::to_string((int)tope));
}

TEST(salvaje, cada_insignia_abre_especies_nuevas) {
  Pet p;
  makePet(p, 4);
  int antes = 0, despues = 0;
  p.badges = 0;
  for (int16_t d = 1; d <= DEX_COUNT; d++) if (p.wildWeight(d, 12)) antes++;
  p.badges = 4;
  for (int16_t d = 1; d <= DEX_COUNT; d++) if (p.wildWeight(d, 12)) despues++;
  CHECK_MSG(despues > antes, "cuatro insignias tienen que notarse");
  // y nunca al reves: una insignia no puede QUITAR bichos del mapa
  for (int16_t d = 1; d <= DEX_COUNT; d++)
    if (bstDe(d) <= REGIONS[0].bstCap[0] && DEX_TBL[d].rarity != R_LEGENDARIO) {
      p.badges = 0; bool a = p.wildWeight(d, 12) > 0;
      p.badges = 8; bool b = p.wildWeight(d, 12) > 0;
      CHECK_MSG(!a || b, "lo que salia con cero insignias sigue saliendo");
    }
}

TEST(salvaje, los_legendarios_son_el_remate) {
  Pet p;
  makePet(p, 4);
  // Nunca por el peso comun, ni con las ocho: los lendarios salen solo
  // por rollLegendary(), con su porcentaje (que se mide aparte).
  for (uint8_t b = 0; b <= GYM_COUNT; b++) {
    p.badges = b;
    CHECK_MSG(p.wildWeight(150, 12) == 0, "Mewtwo no entra en el peso comun");
  }
}

TEST(salvaje, de_noche_cambia_quien_anda_suelto) {
  Pet p;
  makePet(p, 4);   // Charmander, bioma volcan: no desempata por bioma aqui
  p.badges = 0;
  // Zubat es Veneno/Volador: de madrugada tiene que pesar mas que a mediodia.
  CHECK(p.wildWeight(41, 3) > p.wildWeight(41, 13));
  // Pidgey es Normal/Volador: al reves.
  CHECK(p.wildWeight(16, 13) > p.wildWeight(16, 3));
}

TEST(salvaje, el_aviso_caduca_si_no_le_haces_caso) {
  Pet p;
  makePet(p, 4);
  // DURMIENDO, y no por capricho: rollWild primero gasta el aviso viejo
  // y luego puede sortear uno NUEVO en la misma llamada, asi que el test
  // dependia de que el dado no saliera. Aguanto hasta que otra tirada en
  // otro sitio corrio la secuencia y entonces fallo ??? por suerte, no por
  // un fallo del codigo. Dormido, rollWild solo hace caducar.
  p.sleeping = true;
  p.wildDex = 16;
  p.wildLevel = 5;
  p.wildTtl = 2;
  CHECK(p.wildWaiting());
  p.rollWild(12);
  CHECK_MSG(p.wildWaiting(), "aun le queda un minuto");
  p.rollWild(12);
  CHECK_MSG(!p.wildWaiting(), "pasado el rato se va solo");
}

TEST(salvaje, tras_uno_hay_un_descanso) {
  // Sin el hueco minimo, una racha de suerte encadena tres avisos seguidos
  // y deja de ser un acontecimiento.
  Pet p;
  makePet(p, 4);
  p.wildDex = 16;
  p.wildTtl = 1;
  p.wildClear();
  CHECK_EQ((int)p.wildGap, WILD_GAP_MIN);
  CHECK(!p.wildCanAppear());
  for (int i = 0; i < WILD_GAP_MIN; i++) p.rollWild(12);
  CHECK_EQ((int)p.wildGap, 0);
}

TEST(salvaje, no_sale_nadie_sin_fuelle_ni_dormido) {
  Pet p;
  makePet(p, 4);
  setStats(p, 80, 80, 80, 80);
  CHECK(p.wildCanAppear());
  p.energy = 5;
  CHECK_MSG(!p.wildCanAppear(), "sin energia el ! seria un boton que dice que no");
  p.energy = 80;
  p.sleeping = true;
  CHECK_MSG(!p.wildCanAppear(), "durmiendo no se entera de nada");
}

TEST(salvaje, el_nivel_va_a_la_par_del_tuyo) {
  Pet p;
  makePet(p, 4);
  setLevel(p, 30);
  for (int i = 0; i < 200; i++) {
    uint16_t n = p.wildLevelFor(16);
    CHECK_RANGE((int)n, 1, 32);   // nunca mas de +2 por encima
  }
  setLevel(p, 1);
  for (int i = 0; i < 50; i++) CHECK(p.wildLevelFor(16) >= 1);
}

TEST(salvaje, el_reparto_nunca_devuelve_un_hueco_vacio) {
  Pet p;
  makePet(p, 4);
  for (uint8_t b = 0; b <= GYM_COUNT; b++) {
    p.badges = b;
    for (uint8_t h = 0; h < 24; h += 5) {
      int16_t d = p.pickWildSpecies(h);
      CHECK_RANGE((int)d, 1, DEX_COUNT);
      // Hay DOS repartos: el de casa y el de la fuga (uno de cada diez se
      // asoma a la region siguiente). El bicho que salga tiene que valer
      // en UNO de los dos —— antes solo se miraba el de casa, y con la
      // fuga puesta este test cazo la diferencia, que es su trabajo.
      CHECK_MSG(p.wildWeight(d, h) > 0 || p.wildWeight(d, h, true) > 0,
                "no puede sacar uno que no toca en ninguno de los dos "
                "repartos");
    }
  }
}

// LA FUGA: una de cada diez tiradas se asoma a la region SIGUIENTE.
//
// El dueno lo pidio porque en Kanto solo salia Kanto —— que era la regla
// escrita, no mala suerte. Se mide la frecuencia de verdad en vez de
// confiar en que los pesos den "mas o menos un 10%": los pesos son
// enteros pequenos y su total cambia con cada insignia.
TEST(salvaje, una_de_cada_diez_se_asoma_a_la_region_siguiente) {
  Pet p;
  makePet(p, 4);
  p.region = 0;                 // Kanto, y es donde estas
  p.maxRegion = 0;
  p.badges = GYM_COUNT;         // techo de BST alto, para que haya de sobra

  int deFuera = 0, total = 0;
  for (int s = 1; s <= 600; s++) {
    randomSeed(s);
    int16_t d = p.pickWildSpecies(12);
    if (d < 1) continue;
    total++;
    if (dexRegionOf(d) > p.region) deFuera++;
  }
  CHECK_MSG(total > 500, "el montaje no sorteo casi nada");
  CHECK_MSG(deFuera > 0,
            "en 600 tiradas no se asomo ni uno de la region siguiente: la "
            "fuga no esta funcionando");
  // Alrededor de uno de cada diez. El rango es ancho a proposito: dentro
  // de la fuga el reparto sigue siendo por pesos y puede quedar vacio en
  // algun momento (entonces cae de vuelta a lo de casa).
  CHECK_MSG(deFuera * 100 / total <= 18,
            std::string("se asomaron ") +
                std::to_string(deFuera * 100 / total) +
                "% de la region siguiente: eso ya no es asomarse");

  // Y NUNCA dos regiones adelante.
  for (int s = 1; s <= 600; s++) {
    randomSeed(s);
    int16_t d = p.pickWildSpecies(12);
    if (d < 1) continue;
    CHECK_MSG(dexRegionOf(d) <= p.region + 1,
              std::string("salio uno de la region ") +
                  std::to_string(dexRegionOf(d)) + " estando en la " +
                  std::to_string(p.region));
  }
}

// -------------------------------------------------------------- gimnasios

TEST(gimnasio, el_porton_son_los_tres_de_la_casa) {
  Pet p;
  makePet(p, 4);
  CHECK(!p.gymReady());
  for (int i = 0; i < GYM_TRAINERS; i++) {
    CHECK_MSG(!p.gymReady(), "con menos de tres el lider sigue cerrado");
    p.winGymTrainer();
  }
  CHECK_MSG(p.gymReady(), "los tres de la casa abren al lider");
  p.winGymTrainer();
  p.winGymTrainer();
  CHECK_MSG(p.gymTrainers == GYM_TRAINERS,
            "el porton es 'tres y ya', no un marcador que engorda");
}

TEST(gimnasio, moler_hierba_ya_no_abre_el_gimnasio) {
  // Es el cambio: antes cinco salvajes bastaban, y eso medía el reloj y
  // no al jugador. Ahora da igual cuantos mates.
  Pet p;
  makePet(p, 4);
  for (int i = 0; i < 20; i++) p.winWild();
  CHECK_MSG(!p.gymReady(), "veinte salvajes no son un gimnasio");
  CHECK_EQ((int)p.gymTrainers, 0);
}

TEST(gimnasio, la_ficha_cuenta_victorias_y_capturas) {
  // Las dos cuentas que ensena la ficha de entrenador. Son del JUGADOR:
  // suman pase lo que pase en el combate, de donde venga.
  Pet p;
  makePet(p, 4);
  CHECK_EQ((int)p.wins, 0);
  p.winWild();
  p.winWild();
  CHECK_EQ((int)p.wins, 2);
  p.winGymTrainer();
  CHECK_MSG(p.wins == 3, "uno de la casa tambien es una victoria");
  for (int i = 0; i < GYM_TRAINERS; i++) p.winGymTrainer();
  uint32_t antes = p.wins;
  p.winGym();
  CHECK_MSG(p.wins == antes + 1, "y el lider, la que mas");
}

TEST(gimnasio, los_tres_de_la_casa_pelean_por_debajo_del_lider) {
  // Son el porton: tienen que estar a la altura del lider, pero NO por
  // encima. Si el tercero pegara mas que el as, el porton seria el
  // combate de verdad y el lider el tramite.
  for (uint8_t r = 0; r < REGION_COUNT; r++) {
    for (uint8_t gi = 0; gi < GYM_TOTAL; gi++) {
      const GymEntry &g = gymAt(r, gi);
      uint8_t antes = 0;
      // Y que de verdad SUBAN: con la cota de arriba sola, los tres a
      // Lv.1 pasaban el teste. Fue el fallo que hubo que ver en una
      // captura porque ningun teste lo miraba.
      if (g.level > GYM_TRAINERS * 2)
        CHECK_MSG(gymTrainerLevel(g, GYM_TRAINERS - 1) >
                      gymTrainerLevel(g, 0),
                  "los tres de " + std::string(g.leader) +
                      " estan todos al mismo nivel");
      for (uint8_t i = 0; i < GYM_TRAINERS; i++) {
        uint8_t nv = gymTrainerLevel(g, i);
        CHECK_MSG(nv >= antes, "los tres van de menos a mas");
        CHECK_MSG(nv < g.level,
                  "ninguno de la casa llega al as del lider: " +
                      std::string(g.leader) + " nv." + std::to_string(nv));
        CHECK_MSG(nv >= 1, "y ninguno se queda en cero");
        antes = nv;
      }
    }
  }
}

TEST(gimnasio, cada_casa_tiene_sus_tres_caras_y_no_se_salen_de_la_tabla) {
  // El indice sale de una cuenta, no de una tabla curada: si algun dia
  // cambia TRAINER_COUNT, esto es lo que avisa de que se sale.
  for (uint8_t gi = 0; gi < GYM_TOTAL; gi++)
    for (uint8_t i = 0; i < GYM_TRAINERS; i++)
      CHECK(gymTrainerIdx(gi, i) < TRAINER_COUNT);
  // Y los tres de una misma casa son TRES caras distintas, no la misma
  // repetida: con ocho entrenadores y tres por casa, la cuenta lo da.
  for (uint8_t gi = 0; gi < GYM_TOTAL; gi++) {
    uint8_t a = gymTrainerIdx(gi, 0), b = gymTrainerIdx(gi, 1),
            c = gymTrainerIdx(gi, 2);
    CHECK_MSG(a != b && b != c && a != c,
              "la casa " + std::to_string(gi) + " repite cara");
  }
}

TEST(gimnasio, ganar_da_insignia_y_vuelve_a_cerrar_el_porton) {
  Pet p;
  makePet(p, 4);
  for (int i = 0; i < GYM_TRAINERS; i++) p.winGymTrainer();
  CHECK_EQ((int)p.badges, 0);
  p.winGym();
  CHECK_EQ((int)p.badges, 1);
  CHECK_EQ((int)p.gymTrainers, 0);
  CHECK_MSG(!p.gymReady(), "el siguiente hay que ganarselo otra vez");
}

TEST(gimnasio, la_insignia_abre_especies_salvajes) {
  // Es lo que ata las dos cosas: la insignia no es un cromo, es lo que
  // cambia lo que te encuentras por ahi.
  Pet p;
  makePet(p, 4);
  int antes = 0, despues = 0;
  for (int16_t d = 1; d <= DEX_COUNT; d++) if (p.wildWeight(d, 12)) antes++;
  for (int i = 0; i < GYM_TRAINERS; i++) p.winGymTrainer();
  p.winGym();
  for (int16_t d = 1; d <= DEX_COUNT; d++) if (p.wildWeight(d, 12)) despues++;
  CHECK(despues > antes);
}

TEST(gimnasio, cada_lider_paga_lo_suyo) {
  // Los premios son la UNICA via de conseguir piedras: sin esto, quien
  // evoluciona con piedra no evolucionaba nunca.
  Pet p;
  makePet(p, 4);
  for (uint8_t i = 0; i < GYM_TOTAL; i++) {
    int8_t esperado = gymAt(0, i).prize;
    p.badges = i;
    p.winGym();
    CHECK_MSG(p.lastBallCount > 0, "todo desafio paga algo");
    if (esperado == GP_BALLS) {
      CHECK_MSG(ITEM_TBL[p.lastBallId].pocket == BP_BALL, "bolas");
    } else {
      CHECK_MSG(ITEM_TBL[p.lastBallId].fx == IT_STONE, "piedra o cable");
      CHECK_EQ((int)ITEM_TBL[p.lastBallId].arg, (int)esperado);
    }
  }
}

TEST(gimnasio, todo_objeto_de_evolucion_cae_de_algun_lider) {
  // Si un objeto no lo soltara nadie, su familia entera se quedaria sin
  // evolucionar y no se notaria hasta mucho despues. Es justo lo que pasaba
  // con los cuatro de Johto (Roca del Rey, Escama Dragon, Mejora y Garra
  // Metal) al traer las evoluciones de verdad del Anil: la tabla los pedia
  // y ningun lider los daba, asi que Steelix, Scizor, Kingdra, Porygon2,
  // Politoed y Slowking no salian de ningun sitio.
  //
  // Se recorren las DOS regiones, que es lo que hace un jugador: los de
  // Johto solo los suelta Johto.
  Pet p;
  makePet(p, 4);
  for (uint8_t r = 0; r < REGION_COUNT; r++) {
    for (uint8_t i = 0; i < GYM_TOTAL; i++) { p.badges = i; p.winGym(); }
    p.badges = GYM_TOTAL;
    if (r + 1 < REGION_COUNT) CHECK(p.advanceRegion());
  }
  for (uint8_t s = 0; s < STONE_EVO_COUNT; s++) {
    uint8_t tipo = STONE_EVO[s].item;
    bool hay = false;
    const char *nombre = "?";
    for (uint8_t it = 1; it < ITEM_COUNT; it++)
      if (ITEM_TBL[it].fx == IT_STONE && ITEM_TBL[it].arg == tipo) {
        nombre = ITEM_TBL[it].name;
        if (p.bag[it]) hay = true;
      }
    CHECK_MSG(hay, std::string("ningun lider suelta ") + nombre + ", que pide " +
                       DEX_TBL[STONE_EVO[s].from].name);
  }
}

// --- JOHTO: la segunda region ---------------------------------------------

TEST(region, coronar_kanto_abre_el_viaje_y_no_antes) {
  Pet p;
  makeLuchador(p, 4, 30);
  CHECK_EQ((int)p.region, 0);
  CHECK_MSG(!p.canAdvanceRegion(), "sin terminar Kanto no se viaja");
  CHECK(!p.advanceRegion());

  for (uint8_t i = 0; i < p.gymCount(); i++) p.winGym();
  CHECK(p.allGymsDone());
  CHECK(p.canAdvanceRegion());
  CHECK(p.advanceRegion());
  CHECK_EQ((int)p.region, 1);
  CHECK_STREQ(p.regionName(), "JOHTO");
}

// La regla CAMBIO, y es decision del usuario: "nada de zerar insigneas,
// ganhou pode viajar pela regiao livremente, o que libera eh que pode ter
// a revanche". Antes este test pedia lo contrario —— que viajar dejara
// las insignias y el techo a cero ——, y el nombre lo decia.
TEST(region, viajar_NO_quita_insignias_ni_baja_el_techo) {
  Pet p;
  makeLuchador(p, 4, 30);
  for (uint8_t i = 0; i < p.gymCount(); i++) p.winGym();
  const uint8_t deKanto = p.badges;
  const uint16_t techoCoronado = p.levelCap();
  p.winWild();
  CHECK_MSG(p.advanceRegion(), "no se pudo viajar");
  CHECK_EQ((int)p.region, 1);

  // En Johto se empieza de cero EN JOHTO...
  CHECK_EQ((int)p.badges, 0);
  CHECK_EQ((int)p.gymTrainers, 0);
  // ...pero las de Kanto siguen ahi, y el techo NO baja: si bajara, el
  // equipo hecho no podria pelear a su altura en ningun sitio, que es la
  // misma pena por la puerta de atras.
  CHECK_MSG(p.badgesOf(0) == deKanto,
            "viajar borro las insignias de Kanto");
  CHECK_MSG(p.levelCap() >= techoCoronado,
            "el techo bajo al viajar: " + std::to_string((int)techoCoronado) +
                " -> " + std::to_string((int)p.levelCap()));
  // Y volver las devuelve enteras.
  CHECK_MSG(p.setZone(0), "no se pudo volver a Kanto");
  CHECK_EQ((int)p.badges, (int)deKanto);
  CHECK_MSG(p.allGymsDone(), "al volver sigue coronado en Kanto");
  // el bicho no pierde nada suyo
  CHECK_EQ((int)p.level(), 30);
  CHECK_EQ((int)p.speciesId, 4);
}

TEST(region, no_hay_a_donde_ir_despues_de_la_ultima) {
  Pet p;
  makeLuchador(p, 4, 30);
  for (uint8_t r = 0; r < REGION_COUNT; r++) {
    for (uint8_t i = 0; i < p.gymCount(); i++) p.winGym();
    if (r + 1 < REGION_COUNT) CHECK(p.advanceRegion());
  }
  CHECK_EQ((int)p.region, REGION_COUNT - 1);
  CHECK_MSG(!p.canAdvanceRegion(), "la ultima region no lleva a ninguna parte");
  CHECK(!p.advanceRegion());
  CHECK_MSG(p.allGymsDone(), "y se queda coronado, no se desbarata");
}

TEST(region, el_viaje_sobrevive_al_apagado) {
  Pet p;
  makeLuchador(p, 4, 30);
  for (uint8_t i = 0; i < p.gymCount(); i++) p.winGym();
  p.advanceRegion();
  Pet q;
  q.begin();
  CHECK_EQ((int)q.region, 1);
  CHECK_EQ((int)q.badges, 0);
}

TEST(region, los_equipos_de_johto_son_de_especies_reales) {
  // Escritos a mano (el PBS del Anil no tiene estos lideres), asi que un
  // numero mal tecleado no lo caza nadie hasta que el combate abre vacio.
  for (uint8_t r = 0; r < REGION_COUNT; r++) {
    const Region &reg = regionAt(r);
    for (uint8_t i = 0; i < reg.count; i++) {
      const GymEntry &g = reg.gyms[i];
      CHECK(g.leader && g.place && g.leader[0] && g.place[0]);
      CHECK_RANGE((int)g.type, 1, (int)T_COUNT - 1);
      CHECK_MSG(gymTeamSize(g) >= 1, "un lider sin equipo no se puede pelear");
      for (uint8_t k = 0; k < 3; k++) {
        if (!g.team[k]) continue;
        CHECK_RANGE((int)g.team[k], 1, (int)DEX_COUNT);
      }
      // los huecos vacios van DELANTE: gymTeamAt cuenta con ello
      bool visto = false;
      for (uint8_t k = 0; k < 3; k++) {
        if (g.team[k]) visto = true;
        else CHECK_MSG(!visto, "un hueco vacio detras de un bicho descoloca al AS");
      }
      if (i) CHECK_MSG(g.level > reg.gyms[i - 1].level, "la escalera tiene que subir");
    }
  }
}

TEST(gimnasio, la_escalera_va_en_orden_y_tiene_final) {
  Pet p;
  makePet(p, 4);
  for (uint8_t i = 0; i < p.gymCount(); i++) {
    CHECK_EQ((int)p.gymNext(), (int)i);
    CHECK(!p.allGymsDone());
    for (int w = 0; w < GYM_TRAINERS; w++) p.winGymTrainer();
    p.winGym();
  }
  CHECK(p.allGymsDone());
  CHECK_MSG(!p.gymReady(), "coronado, ya no hay a quien desafiar");
  p.winGym();
  CHECK_EQ((int)p.badges, GYM_TOTAL);   // y no se pasa de la cuenta
}

TEST(gimnasio, los_niveles_suben_y_los_bichos_existen) {
  // Una fila mal copiada del PBS (un bicho de gen 2, un nivel al reves) no
  // se ve hasta que alguien llega a ese gimnasio, que son horas de juego.
  uint8_t prev = 0;
  for (uint8_t i = 0; i < GYM_TOTAL; i++) {
    const GymEntry &g = gymAt(0, i);
    // el equipo entero tiene que apuntar a especies reales, no solo el as
    uint8_t n = gymTeamSize(g);
    CHECK_RANGE((int)n, 1, 3);
    uint8_t prevNv = 0;
    for (uint8_t k = 0; k < n; k++) {
      CHECK_RANGE((int)gymTeamAt(g, k), 1, DEX_COUNT);
      uint8_t nv = gymLevelAt(g, k);
      CHECK_MSG(nv >= prevNv, "el as va el ULTIMO y es el de mas nivel");
      prevNv = nv;
    }
    CHECK_MSG(gymLevelAt(g, n - 1) == g.level, "el as lleva el nivel de la fila");
    CHECK_MSG(g.level > prev, "cada desafio tiene que ser mas duro");
    CHECK_RANGE((int)g.level, 1, LEVEL_CAP);
    CHECK(g.leader && *g.leader);
    CHECK(g.place && *g.place);
    CHECK_RANGE((int)g.type, 1, T_COUNT - 1);
    prev = g.level;
  }
  CHECK_MSG(GYM_BADGES <= GYM_TOTAL, "no puede haber mas insignias que desafios");
}

TEST(salvaje, alguna_vez_sale_shiny_cazando) {
  // Raro pero posible: si nunca pasara, no habria nada que perseguir.
  //
  // Este test miraba rollWild(), que era de donde salian los salvajes.
  // Ya no: el "!" automatico es SIEMPRE un entrenador y los salvajes
  // salen por CAZAR. Se reescribe contra la puerta nueva —— si se
  // borrara, nadie vigilaria que el shiny sigue siendo posible, que es
  // justo lo que el dueno pidio garantizar.
  randomSeed(33);
  Pet p;
  makePet(p, 4);
  setStats(p, 90, 90, 90, 90);
  int shinies = 0, salidas = 0;
  const int VUELTAS = 4000;
  for (int i = 0; i < VUELTAS; i++) {
    p.wildDex = 0;
    p.wildTtl = 0;
    p.huntUntil = 0;                 // el enfriamiento no es lo que se mide
    if (!p.hunt(12, 1000)) continue;
    salidas++;
    if (p.wildShiny) shinies++;
  }
  CHECK_MSG(salidas > 100, "cazar no saco casi nada: el sorteo no sortea");
  CHECK_MSG(shinies > 0, "cazando tiene que poder salir uno shiny");
  // margen amplio: esto vigila el orden de magnitud, no la tirada
  CHECK_MSG(shinies * WILD_SHINY_ODDS < VUELTAS * 4, "pero no es lo normal");
}

TEST(salvaje, el_shiny_se_limpia_con_el_aviso) {
  Pet p;
  makePet(p, 4);
  p.wildDex = 16;
  p.wildTtl = 3;
  p.wildShiny = true;
  p.wildClear();
  CHECK(!p.wildShiny);
}

// ----------------------------------------------------------- las bolas
// No hay tienda a proposito: las bolas salen de CUIDAR y de ENTRENAR.
// Si estos dos grifos se secan, la captura deja de existir.

static uint16_t bolasTotales(const Pet &p) {
  uint16_t n = 0;
  for (uint8_t t = 0; t < BALL_TIERS; t++) n = (uint16_t)(n + p.bag[BALL_ID[t]]);
  return n;
}

TEST(bolas, el_profesor_da_un_punado_al_empezar) {
  // Sin esto la primera partida arranca seca: los primeros salvajes pasan
  // por delante y no hay con que cogerlos hasta el dia siguiente.
  mockNvsReset();
  Pet p;
  makePet(p, 4);
  uint16_t antes = bolasTotales(p);
  p.finishIntro(false);
  CHECK_EQ((int)(bolasTotales(p) - antes), OAK_BALLS);
}

TEST(bolas, el_cuidado_diario_da_una) {
  Pet p;
  makePet(p, 4);
  p.streak = 1;
  uint16_t antes = bolasTotales(p);
  p.dailyBalls();
  CHECK_EQ((int)(bolasTotales(p) - antes), 1);
  // una Poke Ball normal, o la de condicao que as vezes vem junto (e
  // essa a anunciada)
  CHECK_MSG(p.lastBallId == BALL_ID[0] || ITEM_TBL[p.lastBallId].fx == IT_BALLX,
            std::string("anunciou ") + ITEM_TBL[p.lastBallId].name);
}

TEST(bolas, as_de_condicao_caem_do_cuidado_e_do_treino) {
  randomSeed(31);
  Pet p;
  makePet(p, 4);
  auto conta = [](const Pet &x) {
    int n = 0;
    for (uint8_t i = BALL_TIERS; i < BALL_ALL_COUNT; i++) n += x.bag[BALL_ALL[i]];
    return n;
  };
  int antes = conta(p);
  for (int i = 0; i < 40; i++) p.dailyBalls();
  int doDia = conta(p) - antes;
  CHECK_MSG(doDia > 3 && doDia < 25,
            "40 dias de cuidado deram " + std::to_string(doDia) +
            " bolas de condicao (esperado ~" + std::to_string(40 * BALLX_DAILY_PCT / 100) + ")");
  antes = conta(p);
  for (int i = 0; i < 40; i++) p.trainBalls(TRAIN_BALL_GREAT);
  CHECK_MSG(conta(p) > antes, "o treino bom nunca trouxe uma");
  antes = conta(p);
  for (int i = 0; i < 40; i++) p.trainBalls(TRAIN_BALL_SCORE);
  CHECK_MSG(conta(p) == antes, "um treino fraco nao traz das de condicao");
}

TEST(bolas, la_racha_paga_mejor) {
  Pet p;
  makePet(p, 4);
  p.streak = BALL_STREAK_GREAT;
  uint16_t antes = bolasTotales(p);
  uint8_t n = p.dailyBalls();
  CHECK_MSG(n == 2 || n == 3, "en el hito de la semana caen dos (mas la de condicao, a veces)");
  CHECK_MSG(p.bag[BALL_ID[1]] > 0, "y una es Great");
  CHECK_EQ((int)(bolasTotales(p) - antes), 2);

  Pet q;
  makePet(q, 4);
  q.streak = BALL_STREAK_ULTRA;
  q.dailyBalls();
  CHECK_MSG(q.bag[BALL_ID[2]] > 0, "al mes, una Ultra");
}

TEST(bolas, el_gimnasio_paga_segun_lo_bien_que_lo_hagas) {
  Pet p;
  makePet(p, 4);
  CHECK_MSG(p.trainBalls(3) == 0, "una sesion floja no da nada");
  CHECK_EQ((int)p.trainBalls(TRAIN_BALL_SCORE), 1);
  CHECK_EQ((int)p.trainBalls(TRAIN_BALL_SCORE * 2), TRAIN_BALL_MAX);
  uint8_t n = p.trainBalls(999);
  CHECK_MSG(n == TRAIN_BALL_MAX || n == TRAIN_BALL_MAX + 1,
            "con tope por sesion (mas, a veces, uma de condicao)");
}

TEST(bolas, una_sesion_redonda_puede_soltar_de_las_buenas) {
  randomSeed(77);
  Pet p;
  makePet(p, 4);
  for (int i = 0; i < 400; i++) p.trainBalls(30);
  CHECK_MSG(p.bag[BALL_ID[1]] > 0 || p.bag[BALL_ID[2]] > 0,
            "con puntuacion alta tienen que caer mejores");
}

TEST(bolas, la_master_ball_no_se_regala_nunca) {
  // Es la unica que no falla: si cayera de un premio diario, capturar
  // dejaria de ser una decision.
  randomSeed(5);
  Pet p;
  makePet(p, 4);
  for (int i = 0; i < 300; i++) { p.dailyBalls(); p.trainBalls(40); }
  CHECK_EQ((int)p.bag[BALL_ID[BALL_TIERS - 1]], 0);
  // y pedirla a mano tampoco cuela
  p.giveBall(BALL_TIERS - 1);
  CHECK_EQ((int)p.bag[BALL_ID[BALL_TIERS - 1]], 0);
}

TEST(bolas, entrenar_paga_bolas_de_verdad) {
  // El camino completo: una sesion del gimnasio deja EVs y bolas.
  Pet p;
  makePet(p, 4);
  uint16_t antes = bolasTotales(p);
  p.trainEV(EV_ATK, 24);
  CHECK(p.trAtk > 0);
  CHECK_MSG(bolasTotales(p) > antes, "entrenar bien tiene que dar con que capturar");
}

// --------------------------------------------------------------- MT velhas

// O save de antes (tmn=304) tinha MT que apontavam para o golpe errado.
// Ao carregar, cada uma vira a MT nova do mesmo golpe; se o golpe nem e
// MT, uma do mesmo tipo. Nada de zerar a colecao.
TEST(mt, o_save_da_tabela_velha_migra_sem_perder_mt) {
  int16_t mesmo = -1, sumiu = -1;
  for (uint16_t i = 0; i < TM_COUNT_V304 && (mesmo < 0 || sumiu < 0); i++) {
    bool existe = false;
    for (uint16_t j = 0; j < TM_COUNT; j++)
      if (TM_MOVE[j] == TM_MOVE_V304[i]) existe = true;
    if (existe && mesmo < 0) mesmo = (int16_t)i;
    if (!existe && sumiu < 0) sumiu = (int16_t)i;
  }
  CHECK_MSG(mesmo >= 0 && sumiu >= 0, "premissa: ha dos dois casos");
  if (mesmo < 0 || sumiu < 0) return;

  Pet p;
  makePet(p, 4);
  p.saveNow();
  {
    uint8_t velho[TM_BYTES_V304] = { 0 };
    velho[mesmo >> 3] |= (uint8_t)(1 << (mesmo & 7));
    velho[sumiu >> 3] |= (uint8_t)(1 << (sumiu & 7));
    Preferences pr;
    pr.begin("tamapoke", false);
    pr.putUShort("tmn", TM_COUNT_V304);
    pr.putBytes("tms", velho, sizeof(velho));
    pr.end();
  }
  Pet q;
  q.begin();
  int tem = 0;
  bool achouMesmo = false, achouTipo = false;
  for (uint16_t j = 0; j < TM_COUNT; j++) {
    if (!q.hasTM(j)) continue;
    tem++;
    if (TM_MOVE[j] == TM_MOVE_V304[mesmo]) achouMesmo = true;
    else if (MOVE_TBL[TM_MOVE[j]].type == MOVE_TBL[TM_MOVE_V304[sumiu]].type)
      achouTipo = true;
  }
  CHECK_EQ(tem, 2);
  CHECK_MSG(achouMesmo, std::string("perdeu a MT de ") +
                        MOVE_TBL[TM_MOVE_V304[mesmo]].name);
  CHECK_MSG(achouTipo, std::string("a de ") + MOVE_TBL[TM_MOVE_V304[sumiu]].name +
                       " nao virou uma do mesmo tipo");
  // e na proxima gravacao ja fica no formato novo
  q.saveNow();
  Preferences pr;
  pr.begin("tamapoke", true);
  CHECK_EQ((int)pr.getUShort("tmn", 0), TM_COUNT);
  pr.end();
}

// A orden das MT esta congelada desde TM_COUNT_FIJO: as novas (Embargo,
// Substitute, Trick Room) entram no FIM. O save de 306 MT tem os mesmos
// bits de hoje e nao pode perder nenhuma —— antes, qualquer TM_COUNT
// diferente zerava a colecao inteira.
TEST(mt, o_save_de_antes_das_mt_novas_fica_igual) {
  CHECK_MSG(TM_COUNT_FIJO < TM_COUNT, "premissa: ha MT novas depois da orden fija");
  Pet p;
  makePet(p, 4);
  p.saveNow();
  const uint16_t marca[3] = { 0, 77, (uint16_t)(TM_COUNT_FIJO - 1) };
  {
    uint8_t velho[(TM_COUNT_FIJO + 7) / 8] = { 0 };
    for (uint16_t k : marca) velho[k >> 3] |= (uint8_t)(1 << (k & 7));
    Preferences pr;
    pr.begin("tamapoke", false);
    pr.putUShort("tmn", TM_COUNT_FIJO);
    pr.putBytes("tms", velho, sizeof(velho));
    pr.end();
  }
  Pet q;
  q.begin();
  int tem = 0;
  for (uint16_t j = 0; j < TM_COUNT; j++) if (q.hasTM(j)) tem++;
  CHECK_EQ(tem, 3);
  for (uint16_t k : marca)
    CHECK_MSG(q.hasTM(k), std::string("perdeu a MT de ") + MOVE_TBL[TM_MOVE[k]].name);
}

// --------------------------------------------------------------- as bolas

static uint8_t bolaPorNome(const char *n) {
  for (uint8_t i = 1; i < ITEM_COUNT; i++)
    if (!strcmp(ITEM_TBL[i].name, n)) return i;
  return 0;
}

TEST(bolas, a_bola_da_captura_fica_com_o_bicho_e_a_friend_chega_amiga) {
  mockNvsReset();
  Pet p;
  makePet(p, 4);
  uint8_t friendB = bolaPorNome("Friend Ball");
  CHECK_MSG(friendB, "premissa: a Friend Ball existe");
  CHECK_EQ((int)p.catchMon(19, 12, false, 0, friendB), (int)CATCH_TEAM);
  uint8_t slot = 0xFF;
  for (uint8_t i = 0; i < TEAM_SLOTS; i++)
    if (i != p.mainSlot && p.team[i].species == 19) slot = i;
  CHECK_MSG(slot != 0xFF, "o Rattata nao entrou no time");
  if (slot == 0xFF) return;
  CHECK_EQ((int)ballDe(p.team[slot]), (int)friendB);
  CHECK_EQ((int)p.team[slot].bond, BOND_FRIEND_BALL);
  // A captura sem bola dita (o caminho velho) nao inventa uma.
  CHECK_EQ((int)p.catchMon(19, 12, false, 0), (int)CATCH_TEAM);
  uint8_t outro = 0xFF;
  for (uint8_t i = 0; i < TEAM_SLOTS; i++)
    if (i != p.mainSlot && i != slot && p.team[i].species == 19) outro = i;
  CHECK(outro != 0xFF);
  if (outro != 0xFF) {
    CHECK_EQ((int)ballDe(p.team[outro]), 0);
    CHECK_EQ((int)p.team[outro].bond, BOND_AT_BIRTH);
  }

  // Vira o titular: a bola vai junto, e sobrevive a gravar e carregar.
  CHECK(p.setMain(slot));
  CHECK_EQ((int)p.ballId, (int)friendB);
  p.saveNow();
  Pet q;
  q.begin();
  CHECK_MSG(q.ballId == friendB,
            "depois de carregar a bola do titular virou " +
            std::to_string((int)q.ballId));
}

TEST(bolas, um_registro_velho_sem_a_bandeira_nao_le_o_lixo_como_bola) {
  StoredMon m;
  m.species = 19;
  m.ball = 77;          // o que havia no alinhamento antes do campo existir
  CHECK_EQ((int)ballDe(m), 0);
  m.flags |= SM_BALL_SET;
  CHECK_EQ((int)ballDe(m), 77);
}

TEST(bolas, toda_partida_recebe_o_pacote_das_bolas_novas_uma_vez) {
  mockNvsReset();
  Pet p;
  makePet(p, 4);
  auto conta = [](const Pet &x) {
    int n = 0;
    for (uint8_t i = BALL_TIERS; i < BALL_ALL_COUNT; i++) n += x.bag[BALL_ALL[i]];
    return n;
  };
  CHECK_EQ(conta(p), BALLX_WELCOME);
  p.saveNow();
  Pet q;
  q.begin();
  CHECK_MSG(conta(q) == BALLX_WELCOME,
            "carregar de novo deu o pacote outra vez: " +
            std::to_string(conta(q)));
}

TEST(bolas, a_luxury_faz_o_vinculo_subir_mais) {
  int sobe[2];
  uint8_t lux = bolaPorNome("Luxury Ball");
  CHECK_MSG(lux, "premissa: a Luxury Ball existe");
  for (int k = 0; k < 2; k++) {
    mockNvsReset();
    Pet p;
    makePet(p, 4);
    p.bond = 40;
    p.ballId = k ? lux : 0;
    p.energy = 100;
    uint8_t antes = p.bond;
    p.trainEV(EV_ATK, 20);
    sobe[k] = p.bond - antes;
  }
  CHECK_MSG(sobe[0] > 0, "premissa: treinar sobe o vinculo");
  CHECK_MSG(sobe[1] > sobe[0],
            "com a Luxury subiu " + std::to_string(sobe[1]) + " e sem ela " +
            std::to_string(sobe[0]));
}

// --------------------------------------------------------------- captura

TEST(captura, lo_capturado_va_al_equipo_y_queda_registrado) {
  mockNvsReset();
  Pet p;
  makePet(p, 4);
  CHECK(!p.isRegistered(19));
  uint8_t donde = p.catchMon(19, 12, false, 0);   // Rattata
  CHECK_EQ((int)donde, (int)CATCH_TEAM);
  // Y ENTERO. Con hpLeft=0 el combate lo veia desmayado (btCanSend exige
  // hpLeft > 0, y 0 es como se guarda un caido), asi que un equipo de
  // capturados daba cero relevos: al caer el tuyo, knockOut() daba el
  // combate por perdido contra un salvaje en vez de dejarte sacar a otro.
  {
    uint8_t slot = 0xFF;
    for (uint8_t i = 0; i < TEAM_SLOTS; i++)
      if (i != p.mainSlot && p.team[i].species == 19) slot = i;
    CHECK_MSG(slot != 0xFF, "el Rattata no aparecio en el equipo");
    CHECK_MSG(p.team[slot].hpLeft > 0,
              "el capturado entra con 0 PS: el combate lo cuenta desmayado");
  }
  CHECK_MSG(p.isRegistered(19), "capturar tambien llena la Pokedex");
  // y llega listo para pelear, no en blanco
  bool tiene = false;
  for (uint8_t i = 0; i < TEAM_SLOTS; i++)
    if (p.team[i].species == 19) {
      CHECK_EQ((int)Pet::levelOf(p.team[i]), 12);
      CHECK_MSG(p.team[i].moves[0] != 0, "con golpes");
      CHECK_MSG(p.team[i].ppLeft[0] > 0, "y con PP");
      CHECK_RANGE((int)p.team[i].geneAtk, LEGACY_FLOOR_BASE, 110);
      tiene = true;
    }
  CHECK(tiene);
}

TEST(captura, con_el_equipo_lleno_se_va_al_banco) {
  mockNvsReset();
  Pet p;
  makePet(p, 4);
  for (int i = 0; i < TEAM_SLOTS + 2; i++) p.catchMon(19, 5, false, 0);
  CHECK_EQ((int)p.teamCount(), TEAM_SLOTS);
  CHECK_MSG(p.bankCount() >= 2, "los que no caben van al banco");
  CHECK_EQ((int)p.catchMon(19, 5, false, 0), (int)CATCH_BANK);
}

TEST(captura, el_shiny_capturado_se_queda_shiny) {
  mockNvsReset();
  Pet p;
  makePet(p, 4);
  p.catchMon(130, 20, true, 0);
  bool visto = false;
  for (uint8_t i = 0; i < TEAM_SLOTS; i++)
    if (p.team[i].species == 130) {
      CHECK_MSG((p.team[i].flags & SM_SHINY) != 0, "sigue siendo shiny");
      visto = true;
    }
  CHECK(visto);
}

TEST(captura, una_especie_imposible_no_entra) {
  Pet p;
  makePet(p, 4);
  CHECK_EQ((int)p.catchMon(0, 5, false, 0), (int)CATCH_FULL);
  // DEX_COUNT + 1 y no un 999 a pelo: con 151 especies el 999 era
  // imposible, con las 1025 de Paldea es Gimmighoul y el test dejaba de
  // probar lo que dice su nombre.
  CHECK_EQ((int)p.catchMon(DEX_COUNT + 1, 5, false, 0), (int)CATCH_FULL);
}

// --------------------------------------------------------- entrenadores

TEST(entrenador, el_aviso_automatico_es_SIEMPRE_un_entrenador) {
  // Este test ha cambiado DOS veces, y las dos por una decision del dueno
  // al jugar en la placa:
  //   1) decia que sin insignias no salia ninguno (TRAINER_FROM_BADGE 1);
  //      con el porton puesto no veia un entrenador en toda la partida.
  //   2) decia que eran la MINORIA (TRAINER_PCT 25), porque el "!" era
  //      sobre todo hierba. Ahora los salvajes salen por CAZAR y el "!"
  //      es el camino al gimnasio: siempre alguien.
  // Se reescribe en vez de borrarse: lo que protege es la decision nueva.
  randomSeed(19);
  Pet p;
  makePet(p, 4);
  setStats(p, 90, 90, 90, 90);
  p.badges = 0;
  int avisos = 0, entrenadores = 0, salvajes = 0;
  for (int i = 0; i < 2000; i++) {
    p.wildGap = 0; p.wildTtl = 0; p.wildDex = 0; p.wildTrainer = -1;
    p.rollWild(12);
    if (p.wildDex > 0) {
      avisos++;
      if (p.wildTrainer >= 0) entrenadores++; else salvajes++;
    }
  }
  CHECK_MSG(avisos > 100, "casi no sale nadie: el sorteo no esta sorteando");
  CHECK_MSG(salvajes == 0,
            std::string("el aviso automatico saco ") +
                std::to_string(salvajes) +
                " salvajes: esa puerta es la de CAZAR, no esta");
  CHECK_EQ(entrenadores, avisos);
}

TEST(entrenador, el_suyo_no_es_shiny) {
  // El bicho de un entrenador es SUYO, no un hallazgo. Si saliera shiny,
  // el jugador veria el brillo que no puede capturar.
  randomSeed(23);
  Pet p;
  makePet(p, 4);
  setStats(p, 90, 90, 90, 90);
  p.badges = 4;
  int total = 0;
  for (int i = 0; i < 2000; i++) {
    p.wildGap = 0; p.wildTtl = 0; p.wildDex = 0;
    p.rollWild(12);
    if (!p.wildDex) continue;
    total++;
    CHECK_MSG(!p.wildShiny, "un entrenador saco un bicho shiny");
  }
  CHECK(total > 100);
}

TEST(entrenador, su_bicho_es_de_TU_nivel) {
  // La dificultad esta en que saca VARIOS, no en que sean mas fuertes: un
  // rival de nivel superior se siente injusto.
  randomSeed(29);
  Pet p;
  makePet(p, 4);
  setStats(p, 90, 90, 90, 90);
  setLevel(p, 30);
  // `setLevel` solo toca la EXP; en el juego subir de nivel pasa SIEMPRE por
  // hpSync(), que suma la subida a los PS de ahora. Sin esto el bicho se
  // queda con los PS de nivel 1 sobre el tope de nivel 30 ??? por debajo del
  // 25% ??? y canBattle() apaga los encuentros, asi que no saldria ninguno.
  p.hpSync();
  p.badges = 8;
  bool visto = false;
  for (int i = 0; i < 2000 && !visto; i++) {
    p.wildGap = 0; p.wildTtl = 0; p.wildDex = 0;
    p.rollWild(12);
    if (p.wildTrainer < 0) continue;
    visto = true;
    CHECK_RANGE((int)p.wildLevel, 1, (int)p.battleLevel() + 2);
    CHECK_MSG(!p.wildShiny, "el bicho de un entrenador no es un hallazgo");
  }
  CHECK(visto);
}

TEST(entrenador, la_tabla_esta_sana) {
  for (uint8_t i = 0; i < TRAINER_COUNT; i++) {
    const TrainerEntry &t = trainerAt(i);
    CHECK(t.name && *t.name);
    CHECK_RANGE((int)t.team, 1, TRAINER_TEAM_MAX);
  }
}

TEST(piedra, la_que_cae_es_una_que_te_SIRVE) {
  // Con siete objetos de evolucion y sorteo plano, el que necesitas caia 1
  // de cada 7: lo normal era acumular Piedras Hoja sin una sola Oddish.
  Pet p;
  makePet(p, 25);          // Pikachu: solo le vale la Thunder
  uint8_t id = p.usefulStone();
  CHECK(id != 0);
  CHECK_EQ((int)ITEM_TBL[id].arg, (int)T_ELEC);

  // y si nadie de los tuyos usa piedra, no se inventa ninguna
  Pet q;
  makePet(q, 6);           // Charizard: forma final
  CHECK_EQ((int)q.usefulStone(), 0);
}

TEST(piedra, tambien_mira_al_equipo_y_no_solo_al_titular) {
  Pet p;
  makePet(p, 6);           // el titular no usa piedra
  CHECK_EQ((int)p.usefulStone(), 0);
  p.catchMon(133, 10, false, 0);   // pero pilla un Eevee
  uint8_t id = p.usefulStone();
  CHECK_MSG(id != 0, "una piedra del banquillo tambien cuenta");
  CHECK(stoneTarget(133, ITEM_TBL[id].arg) != 0);
}

// ------------------------------------------------------------- herencia
// La despedida deja un huevo de la MISMA especie con lo que ganaste. Es lo
// que convierte un final seco en el principio de un linaje.

static void criaParaDespedir(Pet &p, int16_t dex, uint16_t nivel) {
  makePet(p, dex);
  setLevel(p, nivel);
  p.bond = 100;
  p.careMistakes = 0;
  p.ageMinutes = FAREWELL_AGE_MIN;
}

TEST(herencia, la_calidad_sale_de_lo_que_hiciste) {
  Pet p;
  makePet(p, 6);
  setLevel(p, 1);
  p.bond = BOND_AT_BIRTH;
  p.careMistakes = 9;
  p.badges = 0;
  uint8_t flojo = p.legacyScore();

  p.bond = 100;
  CHECK_MSG(p.legacyScore() > flojo, "el carino cuenta");
  p.careMistakes = 0;
  setLevel(p, 60);
  p.badges = 8;
  CHECK_EQ((int)p.legacyScore(), LEGACY_SCORE_MAX);
  CHECK_MSG(p.legacyScore() <= LEGACY_SCORE_MAX,
            "con techo: la herencia se nota, no decide la partida");
}

TEST(herencia, el_huevo_es_de_su_LINEA_pero_de_la_forma_base) {
  // Un Charizard deja un CHARMANDER. Poner la forma final dentro del huevo
  // regalaba un bicho acabado de nivel 1 y se saltaba entero el criarlo,
  // que es de lo que va el juego.
  mockNvsReset();
  mockSetMillis(0);
  Pet p;
  criaParaDespedir(p, 6, 50);   // CHARIZARD
  p.startFarewell();
  CHECK(p.hasLegacyEgg());
  mockAdvanceMillis(CEREMONY_MS + 100);
  p.update(millis());
  CHECK_MSG(p.isEgg(), "la ceremonia acaba en huevo");
  p.eggTap(); p.eggTap(); p.eggTap();
  CHECK_EQ((int)p.speciesId, 4);   // CHARMANDER
}

TEST(herencia, baseFormOf_sube_la_cadena_entera) {
  CHECK_EQ((int)baseFormOf(3), 1);     // Venusaur  -> Bulbasaur
  CHECK_EQ((int)baseFormOf(6), 4);     // Charizard -> Charmander
  CHECK_EQ((int)baseFormOf(9), 7);     // Blastoise -> Squirtle
  CHECK_EQ((int)baseFormOf(5), 4);     // desde el medio tambien
  CHECK_EQ((int)baseFormOf(4), 4);     // una base es su propia base
  CHECK_EQ((int)baseFormOf(150), 150); // sin evolucion: el mismo
  // y nunca devuelve algo fuera de la dex, pase lo que pase
  for (int16_t d = 1; d <= DEX_COUNT; d++)
    CHECK_RANGE((int)baseFormOf(d), 1, DEX_COUNT);
}

TEST(herencia, sube_el_suelo_de_los_genes_no_el_techo) {
  // Si subiera el techo, la vigesima generacion seria un dios. Subiendo el
  // suelo, un linaje cuidado nace bueno pero no se sale de la escala.
  mockNvsReset();
  mockSetMillis(0);
  Pet p;
  criaParaDespedir(p, 6, 60);
  p.badges = 8;
  CHECK_EQ((int)p.legacyScore(), LEGACY_SCORE_MAX);
  p.startFarewell();
  mockAdvanceMillis(CEREMONY_MS + 100);
  p.update(millis());
  p.eggTap(); p.eggTap(); p.eggTap();
  const uint8_t g[6] = { p.geneHp, p.geneAtk, p.geneDef,
                         p.geneSpa, p.geneSpd, p.geneSpe };
  for (uint8_t v : g) {
    CHECK_MSG(v >= LEGACY_FLOOR_MAX, "nace con el suelo alto");
    CHECK_MSG(v <= 110, "pero el techo no se mueve");
  }
}

TEST(herencia, un_huevo_normal_sigue_arrancando_en_90) {
  Pet p;
  makePet(p, 4);
  CHECK(!p.hasLegacyEgg());
  CHECK_RANGE((int)p.geneAtk, LEGACY_FLOOR_BASE, 110);
}

TEST(herencia, pasa_un_golpe_que_no_le_tocaria_y_lo_que_llevaba) {
  mockNvsReset();
  mockSetMillis(0);
  Pet p;
  criaParaDespedir(p, 6, 60);
  p.badges = 8;
  p.heldItem = 1;                       // Leftovers
  // lo que sabra EL BEBE (Charmander), no el que se va
  uint16_t alNacer[8];
  uint8_t base = movesKnown(4, 1, alNacer, 8);
  p.startFarewell();
  CHECK_MSG(p.legacyMove != 0, "con score alto tiene que legar un golpe");
  // y tiene que ser uno que el recien nacido NO sabria: eso es lo que hace
  // que sea un golpe de huevo y no un regalo vacio
  for (uint8_t i = 0; i < base; i++)
    CHECK_MSG(alNacer[i] != p.legacyMove, "un golpe de nacimiento no es herencia");

  mockAdvanceMillis(CEREMONY_MS + 100);
  p.update(millis());
  p.eggTap(); p.eggTap(); p.eggTap();
  bool lo_tiene = false;
  for (uint8_t i = 0; i < 4; i++) if (p.ppMove[i]) lo_tiene = true;
  CHECK(lo_tiene);
  CHECK_EQ((int)p.heldItem, 1);
  CHECK_MSG(p.bag[1] > 0, "el objeto heredado tambien esta en la bolsa");
}

TEST(herencia, se_gasta_al_nacer) {
  // Si no se gastara, TODOS los huevos siguientes saldrian de esa especie
  // con el suelo alto, y capturar dejaria de tener sentido.
  mockNvsReset();
  mockSetMillis(0);
  Pet p;
  criaParaDespedir(p, 6, 60);
  p.startFarewell();
  mockAdvanceMillis(CEREMONY_MS + 100);
  p.update(millis());
  p.eggTap(); p.eggTap(); p.eggTap();
  CHECK_MSG(!p.hasLegacyEgg(), "la herencia es de un solo uso");
  CHECK_EQ((int)p.legacyFloor, LEGACY_FLOOR_BASE);
}

TEST(herencia, sobrevive_a_un_apagon_con_el_huevo_puesto) {
  // El huevo puede pasarse horas sin eclosionar; si la herencia no se
  // guardara, apagar el cacharro se la tragaria.
  mockNvsReset();
  mockSetMillis(0);
  {
    Pet p;
    p.begin();
    if (p.awaitingStarter()) p.chooseStarter(6);
    p.eggTap(); p.eggTap(); p.eggTap();
    setLevel(p, 60);
    p.bond = 100;
    p.careMistakes = 0;
    p.badges = 8;
    p.ageMinutes = FAREWELL_AGE_MIN;
    p.startFarewell();
    mockAdvanceMillis(CEREMONY_MS + 100);
    p.update(millis());   // queda el huevo
  }
  Pet q;
  q.begin();
  CHECK(q.isEgg());
  CHECK_MSG(q.hasLegacyEgg(), "el huevo sigue siendo de despedida");
  CHECK_EQ((int)q.legacyFloor, LEGACY_FLOOR_MAX);
}

TEST(herencia, escaparse_no_deja_nada) {
  // El final triste no premia. Es lo unico que separa despedirse de
  // abandonar, y sin eso las dos cosas darian igual.
  Pet p;
  makePet(p, 6);
  p.bond = 100;
  p.startRunaway();
  CHECK(!p.hasLegacyEgg());
}

// ---------------------------------------------------- techo de nivel
// Sin esto, llegar a una region nueva con el equipo hecho convierte su
// primer gimnasio en un paseo. El bicho no pierde nada: PELEA al techo.

TEST(techo, sale_del_proximo_desafio_con_su_margen) {
  Pet p;
  makePet(p, 4);
  for (uint8_t i = 0; i < p.gymCount(); i++) {
    p.badges = i;
    CHECK_EQ((int)p.levelCap(), (int)gymAt(p.region, i).level + GYM_CAP_SLACK);
  }
}

TEST(techo, se_levanta_al_terminar_la_region) {
  Pet p;
  makePet(p, 4);
  p.badges = p.gymCount();
  CHECK(p.allGymsDone());
  CHECK_EQ((int)p.levelCap(), LEVEL_CAP);
}

TEST(techo, por_debajo_no_estorba) {
  Pet p;
  makePet(p, 4);
  p.badges = 0;
  setLevel(p, 5);
  CHECK(!p.overCap());
  CHECK_EQ((int)p.battleLevel(), 5);
}

TEST(techo, por_encima_pelea_capado_pero_no_pierde_nivel) {
  Pet p;
  makePet(p, 4);
  p.badges = 0;                       // el primero: Onix Nv.8 -> techo 11
  setLevel(p, 70);
  CHECK(p.overCap());
  CHECK_EQ((int)p.levelCap(), 11);
  CHECK_MSG(p.battleLevel() == 11, "pelea al techo");
  CHECK_MSG(p.level() == 70, "pero sigue siendo de 70 fuera del combate");
  CHECK_MSG(p.exp > Pet::expForLevel(70) - 1, "y conserva su EXP");
}

TEST(techo, ganar_insignias_lo_va_soltando) {
  Pet p;
  makePet(p, 4);
  setLevel(p, 70);
  uint16_t prev = 0;
  for (uint8_t i = 0; i < p.gymCount(); i++) {
    p.badges = i;
    CHECK_MSG(p.levelCap() > prev, "cada desafio superado sube el techo");
    CHECK_MSG(p.battleLevel() == p.levelCap(),
              "estando de 70 siempre pelea justo al techo");
    prev = p.levelCap();
  }
  p.badges = p.gymCount();
  CHECK_EQ((int)p.battleLevel(), 70);   // region terminada: a pelear de verdad
}

TEST(techo, los_salvajes_tambien_lo_respetan) {
  // Si escalaran con el nivel REAL, al llegar a una region nueva con el
  // equipo hecho la hierba estaria llena de bichos de 70.
  Pet p;
  makePet(p, 4);
  p.badges = 0;
  setLevel(p, 70);
  for (int i = 0; i < 200; i++)
    CHECK_RANGE((int)p.wildLevelFor(16), 1, (int)p.levelCap() + 2);
}

TEST(techo, una_region_nueva_vuelve_a_bajarlo) {
  // Simula el salto: las insignias se ponen a cero (es lo que hara el
  // cambio de region) y el techo vuelve al del primer gimnasio.
  Pet p;
  makePet(p, 4);
  setLevel(p, 70);
  p.badges = p.gymCount();
  CHECK_EQ((int)p.battleLevel(), 70);
  p.badges = 0;                          // "has llegado a Johto"
  CHECK_EQ((int)p.battleLevel(), (int)p.levelCap());
  CHECK_MSG(p.battleLevel() < 20, "el primer gimnasio se pelea de tu a tu");
}

// ------------------------------------------------------------------ intro
// La intro de primera partida y el genero del jugador son del JUGADOR, no
// del bicho: sobreviven a cada crianza y solo los borra un reseteo.

TEST(intro, la_primera_vez_hay_que_verla) {
  mockNvsReset();
  mockSetMillis(0);
  Pet p;
  p.begin();
  CHECK_MSG(!p.introDone, "partida nueva: la intro esta por ver");
}

TEST(intro, verla_una_vez_basta_para_siempre) {
  mockNvsReset();
  mockSetMillis(0);
  {
    Pet primera;
    primera.begin();
    primera.finishIntro(true);     // eligio chica
    CHECK(primera.introDone);
  }
  Pet segunda;
  segunda.begin();
  CHECK_MSG(segunda.introDone, "encender otra vez no la repite");
  CHECK_MSG(segunda.playerGirl, "el genero elegido tambien persiste");
}

TEST(intro, criar_otro_bicho_no_la_devuelve) {
  // newEgg() resetea lo del individuo (vinculo, medallas, apodo). La intro
  // no es del individuo: verla en cada eclosion seria insufrible.
  mockNvsReset();
  mockSetMillis(0);
  Pet p;
  p.begin();
  p.finishIntro(false);
  p.newEgg();
  CHECK(p.introDone);
}

TEST(intro, el_reseteo_de_fabrica_la_devuelve) {
  // "Borrar todo" tiene que significar todo: quien resetea espera volver a
  // la primera pantalla, creditos incluidos.
  mockNvsReset();
  mockSetMillis(0);
  {
    Pet p;
    p.begin();
    p.finishIntro(true);
    p.factoryReset();
  }
  Pet nueva;
  nueva.begin();
  CHECK(!nueva.introDone);
  CHECK(!nueva.playerGirl);
  CHECK_MSG(nueva.awaitingStarter(), "y vuelve a preguntar por el inicial");
}

// ------------------------------------------------------------- obediencia
// La parte del vinculo que vive en el Pet. La mecanica en si esta en
// test_battle.cpp (grupo `obediencia`).

TEST(vinculo, nace_haciendote_caso) {
  // Naciendo a cero se plantaria uno de cada cinco turnos desde el primer
  // combate, castigando al jugador por algo que todavia no ha hecho.
  Pet p;
  makePet(p, 4);
  CHECK_EQ((int)p.bond, BOND_AT_BIRTH);
  CHECK_MSG(Battle::disobeyChance(p.bond) == 0,
            "recien nacido tiene que obedecer siempre");
}

TEST(vinculo, el_descuido_es_lo_unico_que_lo_baja_hasta_desobedecer) {
  Pet p;
  makePet(p, 4);
  p.bond = BOND_AT_BIRTH;
  CHECK_EQ((int)Battle::disobeyChance(p.bond), 0);
  p.bond = BOND_AT_BIRTH - 1;
  CHECK_MSG(Battle::disobeyChance(p.bond) > 0, "un descuido ya se nota");
  p.bond = 0;
  CHECK_EQ((int)Battle::disobeyChance(p.bond), OBEY_MAX_PCT);
}

TEST(vinculo, una_partida_anterior_a_la_obediencia_no_sale_castigada) {
  // Antes el vinculo arrancaba en cero y subia 4 al dia: hasta el jugador
  // mas cuidadoso podia andar por 20. Con la regla nueva eso seria
  // desobedecer sin haber hecho nada mal, asi que se sube al suelo una vez.
  mockNvsReset();
  mockSetMillis(0);
  {
    Pet viejo;
    viejo.begin();
    if (viejo.awaitingStarter()) viejo.chooseStarter(4);
    viejo.eggTap(); viejo.eggTap(); viejo.eggTap();
    viejo.bond = 20;
    viejo.clean();                  // vuelca a la NVS
  }
  {
    Preferences px;                 // asi queda un save igual a los de antes
    px.begin("tamapoke", false);
    px.remove("bnd0");
    px.end();
  }

  Pet nuevo;
  nuevo.begin();
  CHECK_EQ((int)nuevo.bond, BOND_AT_BIRTH);
  CHECK_EQ((int)Battle::disobeyChance(nuevo.bond), 0);
}

TEST(vinculo, la_subida_solo_se_aplica_una_vez) {
  // Si se aplicara en cada arranque, descuidar al bicho no bajaria nunca del
  // suelo: bastaria apagar y encender para perdonarlo todo.
  mockNvsReset();
  mockSetMillis(0);
  {
    Pet viejo;
    viejo.begin();
    if (viejo.awaitingStarter()) viejo.chooseStarter(4);
    viejo.eggTap(); viejo.eggTap(); viejo.eggTap();
    viejo.clean();
  }
  {
    Pet maltratado;
    maltratado.begin();             // aqui se aplica la subida
    maltratado.bond = 12;           // y despues el jugador lo descuida
    maltratado.rename("X");         // rename persiste sin tocar el vinculo
  }                                 // (clean() daria addBond(1) y falsearia)
  Pet otra;
  otra.begin();
  CHECK_MSG(otra.bond == 12, "el suelo no se vuelve a aplicar al reiniciar");
}

TEST(ev, el_presupuesto_comun_obliga_a_elegir) {
  // Sin este techo se acababan poniendo los seis a 100 y los EVs dejaban de
  // ser una decision: entrenar era solo cuestion de insistir.
  Pet p;
  makeLuchador(p, 4, 30);
  p.trHp = p.trAtk = p.trDef = p.trSpa = p.trSpd = p.trSpe = 0;
  CHECK_EQ((int)p.gainEV(EV_ATK, 100), EV_STAT_CAP);
  CHECK_EQ((int)p.gainEV(EV_SPE, 100), EV_STAT_CAP);
  CHECK_EQ((int)p.evTotal(), EV_TOTAL_CAP);
  CHECK_MSG(p.gainEV(EV_DEF, 20) == 0, "gastado el presupuesto, no entra mas");
  CHECK_EQ((int)p.trDef, 0);
}

TEST(ev, el_presupuesto_reparte_lo_que_queda) {
  Pet p;
  makeLuchador(p, 4, 30);
  p.trHp = p.trAtk = p.trDef = p.trSpa = p.trSpd = p.trSpe = 0;
  p.gainEV(EV_ATK, 100);
  p.gainEV(EV_SPE, 93);
  CHECK_EQ((int)p.gainEV(EV_DEF, 40), 7);   // solo los 7 que caben en el total
  CHECK_EQ((int)p.evTotal(), EV_TOTAL_CAP);
}

TEST(ev, una_partida_vieja_por_encima_del_techo_no_pierde_nada) {
  // Las primeras partidas no tenian techo comun. Recortarles los EVs al
  // cargar seria bajarles los stats sin avisar: se quedan como estan y
  // simplemente dejan de crecer.
  Pet p;
  makeLuchador(p, 4, 30);
  p.trHp = p.trAtk = p.trDef = p.trSpa = p.trSpd = p.trSpe = 90;
  CHECK_EQ((int)p.evTotal(), 540);
  CHECK_EQ((int)p.gainEV(EV_ATK, 5), 0);
  CHECK_EQ((int)p.trAtk, 90);   // nadie le ha quitado nada
}

TEST(ev, una_partida_de_tres_hereda_en_vez_de_perder) {
  // Al pasar de 3 a 6, el especial usaba el campo de su pareja fisica. Si al
  // cargar se pusiera 0, al jugador le bajaria el ataque especial de golpe.
  mockNvsReset();
  mockSetMillis(0);
  {
    Pet viejo;
    makePet(viejo, 4);
    viejo.geneAtk = 108;
    viejo.geneDef = 94;
    viejo.trAtk = 60;
    viejo.trDef = 30;
    viejo.clean();
  }
  {
    Preferences px;                 // se le quitan las claves nuevas
    px.begin("tamapoke", false);
    px.remove("gspa");
    px.remove("gspd");
    px.remove("ghp");
    px.remove("tspa");
    px.remove("tspd");
    px.remove("thp");
    px.end();
  }
  Pet q;
  q.begin();
  CHECK_EQ((int)q.geneSpa, 108);    // hereda del fisico, como estaba
  CHECK_EQ((int)q.geneSpd, 94);
  CHECK_EQ((int)q.trSpa, 60);
  CHECK_EQ((int)q.trSpd, 30);
}

TEST(ev, un_huevo_no_entrena) {
  Pet p;
  mockNvsReset();
  mockSetMillis(0);
  p.begin();
  if (p.awaitingStarter()) p.chooseStarter(4);
  CHECK_EQ((int)p.gainEV(SG_ATK, 10), 0);
}
// ------------------------------------------------------- equipo y banco
// El ACTIVO es el propio Pet: el que tiene barras, animacion y suciedad. El
// equipo guarda a los que no salen y el banco a los congelados.

static StoredMon unMon(int16_t dex, uint16_t nivel) {
  StoredMon m{};
  m.species = dex;
  m.exp = Pet::expForLevel(nivel);
  m.geneAtk = m.geneDef = m.geneSpe = 100;
  m.moves[0] = MOVE_STRUGGLE;
  m.ppLeft[0] = 5;
  m.hpLeft = 50;
  m.bond = 10;
  return m;
}

// Los del banquillo NO evolucionaban nunca: ganaban experiencia y subian
// de nivel, pero al llegar al nivel de su evolucion no pasaba nada, para
// siempre. Un Weedle en el equipo se quedaba Weedle aunque llegara al 50.
TEST(equipo, un_suplente_con_nivel_de_sobra_puede_evolucionar) {
  Pet p;
  makePet(p, 4);
  StoredMon w = unMon(13, 10);          // Weedle nv.10; evoluciona al 9
  CHECK(p.addToTeam(w));
  uint8_t slot = 0xFF;
  for (uint8_t i = 0; i < TEAM_SLOTS; i++)
    if (i != p.mainSlot && p.team[i].species == 13) slot = i;
  CHECK_MSG(slot != 0xFF, "el Weedle no entro en el equipo");

  CHECK_MSG(p.canEvolveReserve(slot),
            "con nivel de sobra tendria que poder evolucionar");
  CHECK_MSG(p.evolveReserve(slot), "no evoluciono");
  CHECK_EQ((int)p.team[slot].species, 14);        // Kakuna
  CHECK_MSG(p.team[slot].hpLeft > 0, "quedo con 0 PS, o sea desmayado");
  CHECK_MSG(!p.canEvolveReserve(slot),
            "Kakuna evoluciona al 10, pero este esta justo en el 10: "
            "si la tabla cambia, que se vea");
}

// O suplente que evolui ganha a ficha, como o titular. Nao ganhava: o
// Torchic do usuario virou Combusken no banco e o Combusken ficou "???".
TEST(equipo, o_suplente_que_evolui_entra_na_pokedex) {
  Pet p;
  makePet(p, 4);
  CHECK(p.addToTeam(unMon(13, 10)));    // Weedle nv.10 -> Kakuna
  uint8_t slot = 0xFF;
  for (uint8_t i = 0; i < TEAM_SLOTS; i++)
    if (i != p.mainSlot && p.team[i].species == 13) slot = i;
  CHECK(slot != 0xFF);
  CHECK_MSG(!p.isRegistered(14), "premissa: o Kakuna comeca sem ficha");
  CHECK(p.evolveReserve(slot));
  CHECK_EQ((int)p.team[slot].species, 14);
  CHECK_MSG(p.isRegistered(14), "evoluiu no banco e nao entrou na Pokedex");
}

// Autocura no boot: quem voce TEM (time, banco, times das regioes) esta na
// Pokedex, mesmo que o dexreg tenha se perdido. Foi o que a NVS cheia fez.
TEST(equipo, o_boot_registra_quem_voce_tem) {
  mockNvsReset();
  Pet p;
  makePet(p, 4);
  StoredMon h = unMon(187, 12);         // Hoppip na caixa aberta
  StoredMon t = unMon(152, 20);         // Chikorita num hueco do time
  p.bank[0] = h;
  uint8_t hueco = 0xFF;
  for (uint8_t i = 0; i < TEAM_SLOTS && hueco == 0xFF; i++)
    if (!p.team[i].used()) hueco = i;
  CHECK(hueco != 0xFF);
  p.team[hueco] = t;                    // posto a mao, como fazia o PARTY
  CHECK_MSG(!p.isRegistered(187) && !p.isRegistered(152),
            "premissa: postos a mao, sem ficha");
  p.saveNow();
  Pet q;
  q.begin();
  CHECK_MSG(q.bank[0].species == 187 && q.team[hueco].species == 152,
            "premissa: os dois tinham de voltar do save");
  CHECK_MSG(q.isRegistered(187), "o Hoppip do banco voltou sem ficha");
  CHECK_MSG(q.isRegistered(152), "a Chikorita do time voltou sem ficha");
}

// Y NO antes de tiempo, ni al principal por esta puerta.
TEST(equipo, sin_nivel_no_evoluciona_y_el_principal_no_entra_por_aqui) {
  Pet p;
  makePet(p, 4);
  StoredMon w = unMon(13, 3);           // Weedle nv.3: le faltan seis
  CHECK(p.addToTeam(w));
  uint8_t slot = 0xFF;
  for (uint8_t i = 0; i < TEAM_SLOTS; i++)
    if (i != p.mainSlot && p.team[i].species == 13) slot = i;
  CHECK(slot != 0xFF);
  CHECK_MSG(!p.canEvolveReserve(slot), "evoluciono sin tener el nivel");
  CHECK_MSG(!p.evolveReserve(slot), "y encima lo hizo");
  CHECK_EQ((int)p.team[slot].species, 13);
  CHECK_MSG(!p.canEvolveReserve(p.mainSlot),
            "el principal tiene su propio boton, no este");
}

// Los legendarios despues de acabar una region. Se MIDE la tirada, que es
// la unica forma de saber si "3%" es de verdad un 3%: la cuenta reparte
// milesimas por tramos y un fallo de un indice ahi no se ve leyendo.
TEST(salvajes, los_legendarios_no_salen_hasta_acabar_la_region) {
  Pet p;
  makePet(p, 4);
  p.badges = 0;
  for (int i = 0; i < 2000; i++)
    CHECK_MSG(p.rollLegendary(13) == 0,
              "salio un legendario sin tener las insignias");
}

TEST(salvajes, acabada_la_region_los_legendarios_salen_en_su_porcentaje) {
  Pet p;
  makePet(p, 4);
  p.badges = GYM_COUNT;
  const int N = 40000;
  int veces = 0;
  for (int i = 0; i < N; i++)
    if (p.rollLegendary(13)) veces++;
  // Leva 72: UMA tirada de LEG_PM_TOTAL por mil (o dono escolheu 3%).
  // Se comprueba la FRANJA, no el numero exacto: es una tirada aleatoria.
  int pm = veces * 1000 / N;
  CHECK_MSG(pm >= LEG_PM_TOTAL - 5,
            "casi nunca sale un legendario: " + std::to_string(pm) + " por mil");
  CHECK_MSG(pm <= LEG_PM_TOTAL + 5,
            "salen demasiados legendarios: " + std::to_string(pm) + " por mil");
}

TEST(salvajes, o_treinador_do_aviso_NUNCA_traz_lendario) {
  // A queixa: "ainda aparecem lendarios nos treinadores aleatorios".
  // Regiao terminada (e onde os lendarios entram) e mede-se o EFEITO: o
  // bicho que o aviso automatico poe no wildDex.
  Pet p;
  makePet(p, 4);
  p.badges = GYM_COUNT;
  // Premissa: nesse estado o sorteio de SELVAGEM da lendario, ou o teste
  // passaria sem provar nada.
  int lendSelvagem = 0;
  for (int i = 0; i < 10000; i++) {
    int16_t d = p.pickWildSpecies(13);
    if (d > 0 && DEX_TBL[d].rarity == R_LEGENDARIO) lendSelvagem++;
  }
  CHECK_MSG(lendSelvagem > 0, "premissa: o selvagem tinha de sortear lendario");

  int avisos = 0, lendTreinador = 0;
  for (int i = 0; i < 40000 && avisos < 6000; i++) {
    p.wildDex = 0; p.wildTtl = 0; p.wildGap = 0; p.wildWait = 200;
    p.rollWild(13);
    if (p.wildDex <= 0) continue;
    avisos++;
    CHECK(p.wildTrainer >= 0);
    if (DEX_TBL[p.wildDex].rarity == R_LEGENDARIO) lendTreinador++;
  }
  CHECK_MSG(avisos > 1000, "premissa: o aviso tem de sair; saiu " +
                               std::to_string(avisos) + " vezes");
  CHECK_MSG(lendTreinador == 0, std::to_string(lendTreinador) +
                                    " treinadores com lendario em " +
                                    std::to_string(avisos));
  // E os seguintes do mesmo treinador saem do mesmo modo.
  int lendSeguinte = 0;
  for (int i = 0; i < 10000; i++) {
    int16_t d = p.pickWildSpecies(13, true);
    if (d > 0 && DEX_TBL[d].rarity == R_LEGENDARIO) lendSeguinte++;
  }
  CHECK_MSG(lendSeguinte == 0, std::to_string(lendSeguinte) +
                                   " lendarios no sorteio de treinador");
}

TEST(equipo, el_recien_nacido_ocupa_su_hueco) {
  Pet p;
  makePet(p, 4);
  CHECK_EQ((int)p.teamCount(), 1);
  CHECK_EQ((int)p.mainSlot, 0);
  CHECK_EQ((int)p.team[0].species, 4);
  CHECK_EQ((int)p.bankCount(), 0);
}

TEST(equipo, caben_seis_y_no_mas) {
  Pet p;
  makePet(p, 4);
  for (int i = 0; i < 5; i++) CHECK(p.addToTeam(unMon(1, 10)));
  CHECK_EQ((int)p.teamCount(), TEAM_SLOTS);
  CHECK_MSG(!p.addToTeam(unMon(1, 10)), "el septimo no entra");
}

TEST(equipo, cambiar_de_titular_conserva_a_los_dos) {
  Pet p;
  makeLuchador(p, 4, 30);
  p.bond = 80;
  p.trAtk = 40;
  StoredMon otro = unMon(7, 12);      // Squirtle
  otro.bond = 5;
  CHECK(p.addToTeam(otro));

  CHECK(p.setMain(1));
  CHECK_EQ((int)p.speciesId, 7);
  CHECK_EQ((int)p.level(), 12);
  CHECK_MSG((int)p.bond == 5, "el vinculo es de CADA UNO, no del entrenador");
  CHECK_EQ((int)p.mainSlot, 1);

  // el que salio sigue ahi, entero
  CHECK_EQ((int)p.team[0].species, 4);
  CHECK_EQ((int)Pet::levelOf(p.team[0]), 30);
  CHECK_EQ((int)p.team[0].bond, 80);
  CHECK_EQ((int)p.team[0].trAtk, 40);

  // y se puede volver
  CHECK(p.setMain(0));
  CHECK_EQ((int)p.speciesId, 4);
  CHECK_EQ((int)p.bond, 80);
  CHECK_EQ((int)p.trAtk, 40);
}

TEST(equipo, no_se_cambia_a_un_hueco_vacio_ni_a_uno_mismo) {
  Pet p;
  makePet(p, 4);
  CHECK(!p.setMain(3));            // vacio
  CHECK(!p.setMain(0));            // ya es el activo
  CHECK(!p.setMain(200));          // fuera de rango
}

TEST(banco, guardar_y_recuperar) {
  Pet p;
  makePet(p, 4);
  // Nivel 5 e nao 12: com 0 insignias o teto pra sacar do banco e o
  // nivel do primeiro lider + 3, e um 12 passa dele. O teto e novo;
  // o que este teste prova (guardar e recuperar) nao mudou.
  CHECK(p.addToTeam(unMon(7, 5)));
  CHECK_EQ((int)p.teamCount(), 2);

  CHECK(p.toBank(1));
  CHECK_EQ((int)p.teamCount(), 1);
  CHECK_EQ((int)p.bankCount(), 1);
  CHECK_EQ((int)p.bank[0].species, 7);

  CHECK(p.fromBank(0));
  CHECK_EQ((int)p.teamCount(), 2);
  CHECK_EQ((int)p.bankCount(), 0);
}

TEST(regioes, o_cadeado_nao_deixa_sacar_quem_passa_do_teto) {
  // O banco e compartilhado entre as regioes. Sem isto, o time de Kanto
  // inteiro entrava em Johto pela porta da caixa e o teto da regiao nao
  // servia pra nada —— que era exatamente a queixa ("nosso time forte
  // demais versus pokemons de comeco de johto").
  Pet p;
  makePet(p, 4);
  StoredMon veterano = unMon(7, 45);
  CHECK(p.addToBank(veterano));
  CHECK_MSG(p.bankLocked(p.bank[0]), "um nivel 45 passa do teto de 0 insignias");
  CHECK_MSG(!p.fromBank(0), "e por isso NAO sai da caixa");
  CHECK_EQ((int)p.bankCount(), 1);
}

TEST(regioes, terminar_a_regiao_libera_o_banco_inteiro) {
  Pet p;
  makePet(p, 4);
  CHECK(p.addToBank(unMon(7, 45)));
  CHECK(p.bankLocked(p.bank[0]));
  // com todas as insignias da regiao o teto se levanta de vez
  p.badges = regionAt(p.region).count;
  p.badgesIn[p.region] = p.badges;
  CHECK_MSG(!p.bankLocked(p.bank[0]),
            "regiao terminada libera o banco todo");
  CHECK(p.fromBank(0));
}

TEST(regioes, o_teto_do_banco_NAO_e_o_de_combate) {
  // Sao dois tetos diferentes e misturar os dois foi o erro que eu quase
  // cometi: o de combate pega o MAIOR entre as regioes de proposito,
  // senao viajar viraria punicao.
  Pet p;
  makePet(p, 4);
  p.maxRegion = 1;
  p.badges = regionAt(0).count;      // Kanto terminada
  p.badgesIn[0] = p.badges;
  CHECK(p.setZone(1));               // vai pra Johto
  CHECK_MSG(p.levelCap() >= 60,
            "o teto de COMBATE continua alto: Kanto esta feita");
  CHECK_MSG(p.bankCapHere() < 20,
            "o de SACAR DO BANCO e o de Johto com 0 insignias");
}

// Quantos bichos ha no banco aberto e no equipo, para comparar.
static uint8_t contaEquipo(const Pet &p) {
  uint8_t n = 0;
  for (uint8_t i = 0; i < TEAM_SLOTS; i++) if (p.team[i].used()) n++;
  return n;
}

TEST(regioes, o_save_do_formato_de_ONTEM_converte_sem_perder_nada) {
  // E o caminho que roda NO PRIMEIRO BOOT do dono com o firmware das
  // chaves por regiao: o save tem o bloco unico "teams" e nao tem
  // tm0..tm8. Nenhum teste passava por aqui —— e foi este arranque
  // que pos a placa em loop.
  Pet p;
  makePet(p, 4);
  p.maxRegion = 1;
  p.badges = regionAt(0).count;
  p.badgesIn[0] = p.badges;
  for (uint8_t i = 1; i < TEAM_SLOTS; i++)
    CHECK(p.addToTeam(unMon((int16_t)(150 + i), 50)));
  int16_t kanto[TEAM_SLOTS];
  for (uint8_t i = 0; i < TEAM_SLOTS; i++) kanto[i] = p.team[i].species;
  p.saveNow();
  {
    Preferences pr;
    pr.begin("tamapoke", false);
    for (uint8_t r = 0; r < REGION_COUNT; r++) {
      char k[8];
      snprintf(k, sizeof(k), "tm%u", (unsigned)r);
      pr.remove(k);
    }
    pr.remove("tmreg");
    // O save() de hoje ja nao escreve o bloco unico; o de ontem escrevia.
    pr.putBytes("teams", p.teamsIn, sizeof(p.teamsIn));
    pr.end();
  }
  svApagaDir();   // e ontem as regioes nao iam para o SD
  Pet q;
  mockSerialSilence(true);
  q.begin();
  mockSerialSilence(false);
  CHECK_EQ((int)q.region, 0);
  for (uint8_t i = 0; i < TEAM_SLOTS; i++)
    CHECK_MSG(q.team[i].species == kanto[i] && q.teamsIn[0][i].species == kanto[i],
              "slot " + std::to_string(i) + " de Kanto mudou na conversao");
  Preferences pr;
  pr.begin("tamapoke", true);
  CHECK_MSG(pr.getBytesLength("tm0") > 0 || svExiste("reg00.bin"), "a conversao nao gravou Kanto");
  CHECK_MSG(pr.getBytesLength("teams") > 0, "a conversao apagou o bloco velho");
  pr.end();
}

TEST(regioes, um_bloco_com_o_carimbo_errado_e_RECUSADO) {
  // Cada regiao tem a sua chave (tm0..tm8) e cada bloco traz, no
  // primeiro byte, o numero da regiao a que pertence. Se um bloco for
  // parar na chave errada —— que e a forma como o time de uma regiao
  // acabou na outra —— ele tem de ser RECUSADO, nao carregado.
  //
  // Uma regiao vazia e um erro que se VE. O time de outra regiao
  // ocupando o lugar e um erro que nao se ve, e foi esse que comeu os
  // bichos do dono.
  Pet p;
  makePet(p, 4);
  p.maxRegion = 1;
  p.badges = regionAt(0).count;
  p.badgesIn[0] = p.badges;
  for (uint8_t i = 1; i < TEAM_SLOTS; i++)
    CHECK(p.addToTeam(unMon((int16_t)(150 + i), 50)));
  p.saveNow();

  // Agora poe-se o bloco de KANTO na chave de JOHTO, com o carimbo de
  // Kanto que ele ja traz. E exatamente o acidente contra o qual o
  // carimbo existe.
  {
    // As regioes agora vivem no SD: o acidente e o arquivo de Kanto no lugar
    // do de Johto (o carimbo de Kanto vai dentro dele).
    CHECK_MSG(svExiste("reg00.bin"), "premissa: Kanto tem que estar no SD");
    std::error_code ec;
    std::filesystem::copy_file(svCaminho("reg00.bin"), svCaminho("reg01.bin"),
                               std::filesystem::copy_options::overwrite_existing, ec);
    CHECK_MSG(!ec, "premissa: copiar o arquivo");
  }

  Pet q;
  mockSerialSilence(true);
  q.begin();
  mockSerialSilence(false);

  uint8_t emJohto = 0;
  for (uint8_t i = 0; i < TEAM_SLOTS; i++)
    if (q.teamsIn[1][i].used()) emJohto++;
  CHECK_MSG(emJohto == 0,
            "o bloco de Kanto foi aceito como time de Johto: " +
                std::to_string(emJohto) + " bichos");
}

TEST(regioes, se_a_regiao_se_perde_manda_o_CARIMBO_do_time) {
  // A HIPOTESE DO DONO, e estava certa: "percebi que estava em johto
  // com o time de johto, e aparentemente foi a REGIAO que reapareceu
  // como kanto".
  //
  // Enquanto so a variavel `region` decidir onde o time e arquivado,
  // um arranque em que ela volte errada arquiva o time no lugar
  // errado —— por cima do que la estava, e em silencio. Foi assim que
  // o time de Kanto dele (cinco bichos de nivel 50) virou po.
  //
  // Agora o time traz consigo a regiao a que pertence. Aqui estraga-se
  // de proposito a chave da regiao e exige-se que o carimbo salve.
  Pet p;
  makePet(p, 4);
  p.maxRegion = 1;
  p.badges = regionAt(0).count;
  p.badgesIn[0] = p.badges;

  // Um time de Kanto reconhecivel, que NAO pode ser tocado.
  for (uint8_t i = 1; i < TEAM_SLOTS; i++)
    CHECK(p.addToTeam(unMon((int16_t)(150 + i), 50)));
  int16_t kanto[TEAM_SLOTS];
  for (uint8_t i = 0; i < TEAM_SLOTS; i++) kanto[i] = p.team[i].species;

  // Vai a Johto e choca o inicial de la.
  CHECK(p.setZone(1));
  p.chooseStarter(regionAt(1).starters[0]);
  p.eggTap(); p.eggTap(); p.eggTap();
  p.saveNow();
  const int16_t deJohto = p.speciesId;

  // --- O ESTRAGO: a chave da regiao volta a dizer Kanto ---
  {
    Preferences pr;
    pr.begin("tamapoke", false);
    pr.putUChar("zone", 0);
    pr.putUChar("regn", 0);
    pr.end();
  }

  // --- reinicia ---
  Pet q;
  q.begin();

  CHECK_MSG(q.region == 1,
            "o carimbo tinha que repor Johto, e a regiao veio #" +
                std::to_string((int)q.region));
  CHECK_MSG(q.speciesId == deJohto,
            "e o titular tem que continuar o de Johto");

  // E o time de Kanto, intacto —— que e o que de facto se perdeu.
  for (uint8_t i = 0; i < TEAM_SLOTS; i++)
    CHECK_MSG(q.teamsIn[0][i].species == kanto[i],
              "o slot " + std::to_string(i) + " de Kanto era #" +
                  std::to_string(kanto[i]) + " e ficou #" +
                  std::to_string(q.teamsIn[0][i].species));
}

TEST(regioes, ir_e_voltar_com_REBOOT_nao_mexe_no_equipo_nem_no_banco) {
  // A sequencia do dono: time forte em Kanto, vai a Johto, volta —— e
  // com o aparelho a reiniciar pelo meio, que e quando a migracao e a
  // leitura do save correm. No aparelho dele o time de Kanto acabou
  // cheio de bichos do BANCO.
  Pet p;
  makePet(p, 4);
  p.maxRegion = 1;
  p.badges = regionAt(0).count;
  p.badgesIn[0] = p.badges;

  // Um time de Kanto reconhecivel.
  for (uint8_t i = 1; i < TEAM_SLOTS; i++)
    CHECK(p.addToTeam(unMon((int16_t)(150 + i), 50)));
  // E um banco com outros, bem diferentes.
  for (uint8_t i = 0; i < 8; i++)
    CHECK(p.addToBank(unMon((int16_t)(19 + i), 5)));
  p.saveNow();

  const uint8_t equipoAntes = contaEquipo(p);
  const uint8_t bancoAntes = p.bankCount();
  int16_t kanto[TEAM_SLOTS];
  for (uint8_t i = 0; i < TEAM_SLOTS; i++) kanto[i] = p.team[i].species;

  // O titular anda SEM gravar: so o setZone o pode levar a chave de
  // Kanto antes de sair.
  p.exp = Pet::expForLevel(33);
  const int16_t titKanto = p.speciesId;
  const uint8_t slotKanto = p.mainSlot;

  // --- vai a Johto, choca o inicial de la ---
  CHECK(p.setZone(1));
  p.chooseStarter(regionAt(1).starters[0]);
  p.eggTap(); p.eggTap(); p.eggTap();
  p.saveNow();

  // --- REINICIA o aparelho ---
  Pet q;
  q.begin();
  CHECK_MSG(q.region == 1, "depois do reboot tem que continuar em Johto, "
                           "e nao em #" + std::to_string((int)q.region));

  CHECK_MSG(q.teamsIn[0][slotKanto].species == titKanto &&
            Pet::levelOf(q.teamsIn[0][slotKanto]) == 33,
            "o que o titular andou em Kanto antes de viajar se perdeu "
            "no reboot");

  // --- volta a Kanto ---
  CHECK(q.setZone(0));
  CHECK_MSG(q.region == 0, "tem que estar em Kanto");

  uint8_t doBanco = 0;
  for (uint8_t i = 0; i < TEAM_SLOTS; i++) {
    if (!q.team[i].used()) continue;
    if (q.team[i].species >= 19 && q.team[i].species <= 26) doBanco++;
  }
  CHECK_MSG(doBanco == 0,
            std::to_string(doBanco) + " bichos do BANCO apareceram no "
            "equipo de Kanto");
  CHECK_MSG(contaEquipo(q) == equipoAntes,
            "o equipo de Kanto tinha " + std::to_string(equipoAntes) +
                " e voltou com " + std::to_string(contaEquipo(q)));
  CHECK_MSG(q.bankCount() == bancoAntes,
            "o banco tinha " + std::to_string(bancoAntes) +
                " e ficou com " + std::to_string(q.bankCount()));
  for (uint8_t i = 0; i < TEAM_SLOTS; i++)
    CHECK_MSG(q.team[i].species == kanto[i],
              "o slot " + std::to_string(i) + " de Kanto era #" +
                  std::to_string(kanto[i]) + " e voltou #" +
                  std::to_string(q.team[i].species));
}

TEST(regioes, viajar_NAO_leva_o_bicho_para_a_outra_regiao) {
  // ISTO ACONTECEU NA PARTIDA DO DONO: ele voltou de Johto para Kanto
  // e o Totodile —— o inicial que nascera em Johto —— apareceu no time
  // de Kanto.
  //
  // O caminho: sair de uma regiao COM UM OVO na mao. storeActive()
  // esvazia o slot do titular quando ele e ovo, entao o slot guardado
  // daquela regiao fica vazio. Na volta, o
  // `else if (team[mainSlot].used())` nao entrava, o cargaDelHueco era
  // pulado, o bicho da outra regiao continuava carregado —— e o save()
  // logo abaixo o gravava NESTA. Viajar movia o bicho de lugar.
  //
  // Os testes que eu tinha nunca pegaram porque em todos havia alguem
  // no slot de destino. So quebra quando esta vazio.
  Pet p;
  makePet(p, 4);
  CHECK(p.addToTeam(unMon(7, 12)));      // alguem no slot 1
  p.maxRegion = 1;
  p.badges = regionAt(0).count;
  p.badgesIn[0] = p.badges;

  // Sai de Kanto com um OVO: o slot 0 fica vazio ao guardar.
  p.newEgg();
  CHECK_MSG(p.isEgg(), "premissa: tem que sair com ovo na mao");
  CHECK(p.setZone(1));
  CHECK_MSG(!p.teamsIn[0][0].used(),
            "premissa: o slot guardado de Kanto tem que ficar vazio");

  // Em Johto, choca o inicial de la.
  p.chooseStarter(regionAt(1).starters[0]);
  p.eggTap(); p.eggTap(); p.eggTap();
  CHECK_MSG(!p.isEgg(), "premissa: o inicial de Johto tem que chocar");
  const int16_t deJohto = p.speciesId;

  // E volta para Kanto.
  CHECK(p.setZone(0));
  bool estaEmKanto = false;
  for (uint8_t i = 0; i < TEAM_SLOTS; i++)
    if (p.team[i].species == deJohto) estaEmKanto = true;
  CHECK_MSG(!estaEmKanto,
            "o inicial de Johto foi parar no time de Kanto");
  CHECK_MSG(p.speciesId != deJohto,
            "e continuou sendo o titular em Kanto");
  CHECK_MSG(p.speciesId == 7,
            "o titular de Kanto tem que ser quem estava la: veio #" +
                std::to_string((int)p.speciesId));
}

TEST(regioes, o_teto_nunca_fica_abaixo_do_titular) {
  // A regra crua prendia um bicho criado em CASA: em Kanto com 0
  // insignias o teto fica perto de 11, entao guardar um nivel 12 na
  // caixa era perde-lo. O piso conserta isso.
  Pet p;
  makePet(p, 4);
  p.exp = Pet::expForLevel(40);          // titular 40, ainda sem insignia
  CHECK(p.addToBank(unMon(7, 38)));
  CHECK_MSG(!p.bankLocked(p.bank[0]),
            "nao passa do titular, entao tem que sair da caixa");
  CHECK(p.fromBank(0));
}

TEST(regioes, o_piso_do_titular_NAO_solta_o_veterano_de_fora) {
  // O ponto todo da coisa: em Johto o titular e um inicial recem
  // chocado, entao o piso nao serve de porta pro nivel 50 de Kanto.
  Pet p;
  makePet(p, 4);
  p.maxRegion = 1;
  p.badges = regionAt(0).count;
  p.badgesIn[0] = p.badges;
  CHECK(p.setZone(1));                   // Johto: ovo do inicial
  p.chooseStarter(regionAt(1).starters[0]);
  p.eggTap(); p.eggTap(); p.eggTap();
  CHECK_MSG(!p.isEgg(), "premissa: o inicial de Johto tem que chocar");
  CHECK_MSG(p.level() < 20, "premissa: e ele e novinho");

  CHECK(p.addToBank(unMon(7, 50)));
  CHECK_MSG(p.bankLocked(p.bank[0]),
            "o veterano de Kanto continua barrado em Johto");
  CHECK_MSG(!p.fromBank(0), "e nao sai da caixa");
}

// Ajudante: esta especie esta no time?
static bool noTime(const Pet &p, int16_t dex) {
  for (uint8_t i = 0; i < TEAM_SLOTS; i++)
    if (p.team[i].species == dex) return true;
  return false;
}

TEST(regioes, viajar_troca_o_time_e_devolve_na_volta) {
  // CUIDADO com o que este teste mede. A primeira versao so checava se
  // o bicho de Kanto continuava no array depois de ir e voltar —— e a
  // sabotagem mostrou que isso passa SEM a troca, porque sem ela o
  // array nunca e mexido e o dado sobrevive de graca.
  //
  // Entao agora se poe um bicho que SO existe em Johto e se exige que
  // ele NAO venha junto na volta. Isso so passa se os dois times forem
  // mesmo separados.
  Pet p;
  makePet(p, 4);
  CHECK(p.addToTeam(unMon(7, 5)));          // Squirtle, de Kanto
  p.maxRegion = 1;
  p.badges = regionAt(0).count;
  p.badgesIn[0] = p.badges;

  CHECK(p.setZone(1));
  p.chooseStarter(regionAt(1).starters[0]);
  p.eggTap(); p.eggTap(); p.eggTap();
  CHECK_MSG(!noTime(p, 7), "o Squirtle de Kanto NAO pode estar em Johto");
  CHECK(p.addToTeam(unMon(129, 5)));        // Magikarp, so de Johto
  CHECK(noTime(p, 129));

  CHECK(p.setZone(0));
  CHECK_MSG(noTime(p, 7), "voltando, o time de Kanto volta");
  CHECK_MSG(!noTime(p, 129), "e o de Johto NAO vem junto");
  CHECK_MSG(p.badges == regionAt(0).count,
            "e as insignias de Kanto voltam inteiras");
}

TEST(regioes, chegar_numa_regiao_nova_da_ovo_do_inicial_dela) {
  Pet p;
  makePet(p, 4);
  p.maxRegion = 1;
  p.badges = regionAt(0).count;
  p.badgesIn[0] = p.badges;
  CHECK(!p.regionVisited(1));
  CHECK(p.setZone(1));
  CHECK_MSG(p.regionVisited(1), "pisar marca a regiao");
  CHECK_MSG(p.isEgg(), "chega-se com um ovo na mao");
  CHECK_MSG(p.awaitingStarter(), "e ele deixa escolher entre os tres");
  // e os tres sao os de JOHTO, nao os de Kanto
  CHECK_EQ((int)regionAt(1).starters[0], 152);
  CHECK_EQ((int)regionAt(1).starters[1], 155);
  CHECK_EQ((int)regionAt(1).starters[2], 158);
}

TEST(regioes, o_titular_fica_na_regiao_de_tras) {
  // "o titular e meio que substituido pelo ovo - e o titular fica la no
  // outro time da regiao anterior - se voltarmos, ele volta como titular"
  Pet p;
  makePet(p, 4);
  int16_t titular = p.speciesId;
  p.maxRegion = 1;
  p.badges = regionAt(0).count;
  p.badgesIn[0] = p.badges;

  CHECK(p.setZone(1));
  CHECK_EQ((int)p.teamsIn[0][0].species, (int)titular);

  CHECK(p.setZone(0));
  CHECK_MSG(!p.isEgg(), "voltando, voce volta a cuidar dele");
  CHECK_EQ((int)p.speciesId, (int)titular);
}

TEST(regioes, so_se_ganha_o_ovo_UMA_vez_por_regiao) {
  Pet p;
  makePet(p, 4);
  p.maxRegion = 1;
  p.badges = regionAt(0).count;
  p.badgesIn[0] = p.badges;
  CHECK(p.setZone(1));
  p.chooseStarter(regionAt(1).starters[0]);
  p.eggTap(); p.eggTap(); p.eggTap();
  CHECK(!p.isEgg());
  CHECK(p.setZone(0));
  CHECK(p.setZone(1));
  CHECK_MSG(!p.awaitingStarter(), "a segunda vez nao da ovo nenhum");
  CHECK_MSG(!p.isEgg(), "e devolve o bicho que ficou em Johto");
}

TEST(regioes, um_save_velho_poe_seu_time_na_regiao_onde_esta) {
  // A migracao. E segura por construcao: a chave "team" continua sendo a
  // fonte e nao se apaga, entao se a escrita nova falhar o boot seguinte
  // refaz tudo. Nada velho e destruido —— que foi o que deu errado na
  // migracao do banco e custou trinta bichos de verdade.
  Pet p;
  makePet(p, 4);
  CHECK(p.addToTeam(unMon(7, 5)));
  p.saveNow();

  // um save de antes dos times por regiao nao tem essas chaves
  Preferences pr;
  pr.begin("tamapoke", false);
  pr.remove("teams");
  pr.remove("mslin");
  pr.remove("vmask");
  // e nem as chaves por regiao, que vieram depois: sem tira-las o
  // begin() acha-as e nunca passa pela migracao que este teste cobra.
  pr.remove("teamsmig");
  for (uint8_t r = 0; r < REGION_COUNT; r++) {
    char k[8];
    snprintf(k, sizeof(k), "tm%u", (unsigned)r);
    pr.remove(k);
  }
  pr.end();

  Pet q;
  q.begin();
  CHECK_EQ((int)q.region, 0);
  CHECK_MSG(q.teamsIn[0][1].species == 7,
            "o time que existia vai pra regiao onde o jogador esta");
  CHECK_MSG(q.regionVisited(0), "e so essa conta como pisada");
  CHECK_MSG(!q.regionVisited(1), "Johto continua virgem: tem ovo esperando");
}

TEST(banco, el_titular_no_se_puede_guardar) {
  // si no, el juego se quedaria sin bicho en pantalla
  Pet p;
  makePet(p, 4);
  CHECK_MSG(!p.toBank(p.mainSlot), "primero hay que pasar el relevo");
  CHECK_EQ((int)p.teamCount(), 1);
}

TEST(banco, no_se_saca_de_un_hueco_vacio) {
  Pet p;
  makePet(p, 4);
  CHECK(!p.fromBank(0));
  CHECK(!p.fromBank(200));
}

TEST(banco, con_el_equipo_lleno_no_se_puede_sacar) {
  Pet p;
  makePet(p, 4);
  for (int i = 0; i < 5; i++) p.addToTeam(unMon(1, 5));
  p.toBank(1);                        // hace hueco y llena el banco con uno
  p.addToTeam(unMon(2, 5));           // vuelve a llenar el equipo
  CHECK_EQ((int)p.teamCount(), TEAM_SLOTS);
  CHECK_MSG(!p.fromBank(0), "no cabe");
}

TEST(equipo, los_que_no_salen_ganan_algo_de_experiencia) {
  Pet p;
  makeLuchador(p, 4, 20);
  StoredMon suplente = unMon(7, 5);
  CHECK(p.addToTeam(suplente));
  uint32_t antes = p.team[1].exp;
  uint32_t mia = p.exp;

  p.teamShareExp(400);
  CHECK_MSG(p.team[1].exp > antes, "el suplente no puede quedarse clavado");
  CHECK_MSG(p.team[1].exp - antes < 400, "pero gana menos que el titular");
  CHECK_MSG(p.exp == mia, "al titular se la da gainExp, no esto");
}

TEST(equipo, sobrevive_a_un_reinicio) {
  Pet p;
  makeLuchador(p, 4, 25);
  StoredMon otro = unMon(7, 12);
  otro.bond = 33;
  p.addToTeam(otro);
  p.addToTeam(unMon(1, 8));
  p.toBank(2);
  p.clean();                          // vuelca

  Pet q;
  q.begin();
  CHECK_EQ((int)q.teamCount(), 2);
  CHECK_EQ((int)q.bankCount(), 1);
  CHECK_EQ((int)q.team[1].species, 7);
  CHECK_EQ((int)q.team[1].bond, 33);
  CHECK_EQ((int)q.bank[0].species, 1);
  CHECK_EQ((int)q.speciesId, 4);
  CHECK_EQ((int)q.level(), 25);
}

TEST(equipo, el_tamano_guardado_no_se_dispara) {
  // Son 6 + 60 huecos en la NVS, que tiene 20 KB en total. Si el struct
  // engorda, esto avisa antes de que el guardado deje de caber.
  //
  // 48 -> 52 al pasar `ability` a uint16_t (el Anil tiene 304
  // habilidades y el indice se salia de un byte en Galar), y 52 -> 56 al
  // anadir `form` (las regionales: un Vulpix de Alola es un Vulpix).
  // 66 x 56 = 3696 B, el 18% de la NVS: el margen sigue de sobra.
  CHECK(sizeof(StoredMon) <= 56);
  CHECK((sizeof(StoredMon) * (TEAM_SLOTS + BANK_SLOTS)) < 4096);
}

TEST(equipo, perder_no_cambia_de_quien_cuidas) {
  // El PRINCIPAL y QUIEN PELEA son cosas distintas: caer en combate no puede
  // pasarle el testigo a otro. Cambiar de principal es una decision que se
  // toma en la pagina de equipo, con su boton, y en ningun otro sitio.
  Pet p;
  makeLuchador(p, 4, 30);
  StoredMon otro = unMon(7, 12);
  p.addToTeam(otro);
  uint8_t antes = p.mainSlot;
  int16_t especie = p.speciesId;

  p.afterBattle(0);                    // se desmaya
  CHECK_EQ((int)p.mainSlot, (int)antes);
  CHECK_EQ((int)p.speciesId, (int)especie);
  CHECK_MSG(p.sleeping, "se desmaya y duerme, pero sigue siendo el tuyo");
}

TEST(equipo, cambiar_de_principal_es_deliberado) {
  // setMain es la UNICA via, y la pantalla la cuelga de un boton aparte
  // (el circulo) y no del cuerpo de la fila.
  Pet p;
  makeLuchador(p, 4, 30);
  p.addToTeam(unMon(7, 12));
  CHECK_EQ((int)p.mainSlot, 0);
  p.feedBerry(0);                      // cuidarlo no cambia nada
  p.afterBattle(p.maxHp() / 2);        // pelear tampoco
  CHECK_EQ((int)p.mainSlot, 0);
  CHECK(p.setMain(1));                 // solo esto
  CHECK_EQ((int)p.mainSlot, 1);
}

// --- los golpes de un SUPLENTE ---------------------------------------------
// La pagina de golpes ya no es solo la del titular: desde la lista del equipo
// se abre la de cualquiera. Antes habia que hacerlo titular para mirarlo, y
// eso cuesta el vinculo del que lo era.

// Un suplente de verdad, con su aprendizaje, y no el unMon() de MOVE_STRUGGLE.
static uint8_t ponSuplente(Pet &p, int16_t dex, uint16_t nivel) {
  StoredMon m{};
  m.species = dex;
  m.exp = Pet::expForLevel(nivel);
  m.hpLeft = 50;
  p.addToTeam(m);
  for (uint8_t i = 0; i < TEAM_SLOTS; i++)
    if (p.team[i].species == dex && i != p.mainSlot) return i;
  return 0xFF;
}

TEST(golpes_suplente, sin_elegir_se_ven_los_de_su_nivel) {
  // `moves[]` de un capturado viene vacio: lo que de verdad sacaria al campo
  // es su aprendizaje. Si la pantalla leyera el array a secas, un suplente
  // recien pillado apareceria sin ningun golpe.
  Pet p;
  makeLuchador(p, 4, 30);
  uint8_t s = ponSuplente(p, 7, 20);          // Squirtle nv 20
  CHECK(s < TEAM_SLOTS);
  CHECK_EQ((int)p.team[s].moves[0], 0);       // el array esta vacio...

  uint16_t mv[4];
  uint8_t pp[4];
  CHECK(p.teamMoves(s, mv, pp));
  uint16_t esperado[4];
  uint8_t n = movesAtLevel(7, 20, esperado);
  CHECK(n > 0);
  for (uint8_t i = 0; i < n; i++) {
    CHECK_EQ((int)mv[i], (int)esperado[i]);   // ...pero se ven los suyos
    CHECK_EQ((int)pp[i], (int)movePP(esperado[i]));
  }
}

TEST(golpes_suplente, cambiar_uno_no_borra_los_otros_tres) {
  // Al tocar los golpes por primera vez hay que CONGELAR lo que tenia: su
  // `moves[]` esta vacio hasta ese momento. Sin congelar, el primer cambio lo
  // dejaria con un solo golpe (o con ninguno) al salir al campo.
  Pet p;
  makeLuchador(p, 4, 30);
  uint8_t s = ponSuplente(p, 7, 20);           // nv 20: los cuatro llenos
  uint16_t antes[4];
  uint8_t pp[4];
  p.teamMoves(s, antes, pp);
  for (uint8_t i = 0; i < 4; i++) CHECK(antes[i] != 0);

  // uno fuera y otro dentro, que es el gesto real de la pantalla
  CHECK(p.teamUnequipMove(s, 1));
  CHECK(p.team[s].flags & SM_MOVES_PICKED);    // ya manda el jugador
  uint16_t lista[40];
  uint8_t n = movesKnown(7, 20, lista, 40);
  uint16_t mete = 0;
  for (uint8_t i = 0; i < n && !mete; i++) {
    bool ya = false;
    for (uint8_t k = 0; k < 4; k++) if (antes[k] == lista[i]) ya = true;
    if (!ya) mete = lista[i];
  }
  CHECK_MSG(mete != 0, "a nivel 20 conoce mas de cuatro");
  CHECK(p.teamEquipMove(s, mete));

  uint16_t ahora[4];
  p.teamMoves(s, ahora, pp);
  CHECK_EQ((int)ahora[0], (int)antes[0]);      // los otros tres, intactos
  CHECK_EQ((int)ahora[2], (int)antes[2]);
  CHECK_EQ((int)ahora[3], (int)antes[3]);
  CHECK_EQ((int)ahora[1], (int)mete);          // y el nuevo en el hueco
  CHECK_EQ((int)pp[1], (int)movePP(mete));
}

TEST(golpes_suplente, soltar_deja_el_hueco_y_se_guarda) {
  Pet p;
  makeLuchador(p, 4, 30);
  uint8_t s = ponSuplente(p, 7, 20);
  uint16_t mv[4];
  uint8_t pp[4];
  p.teamMoves(s, mv, pp);
  uint16_t fuera = mv[0];
  CHECK(fuera != 0);

  CHECK(p.teamUnequipMove(s, 0));
  p.teamMoves(s, mv, pp);
  CHECK_EQ((int)mv[0], 0);
  CHECK_EQ((int)pp[0], 0);
  // y el resto sigue donde estaba: soltar no recoloca
  CHECK(p.teamEquipMove(s, fuera));            // vuelve al primer hueco libre
  p.teamMoves(s, mv, pp);
  CHECK_EQ((int)mv[0], (int)fuera);
  CHECK_EQ((int)pp[0], (int)movePP(fuera));
}

TEST(golpes_suplente, el_mismo_golpe_no_entra_dos_veces) {
  Pet p;
  makeLuchador(p, 4, 30);
  uint8_t s = ponSuplente(p, 7, 20);
  uint16_t mv[4];
  uint8_t pp[4];
  p.teamMoves(s, mv, pp);
  CHECK(!p.teamEquipMove(s, mv[0]));
}

TEST(golpes_suplente, el_titular_no_pasa_por_aqui) {
  // Su copia de team[mainSlot] solo se pone al dia en storeActive(), asi que
  // editarla se perderia sin avisar. Las tres tienen que negarse.
  Pet p;
  makeLuchador(p, 4, 30);
  uint16_t mv[4];
  uint8_t pp[4];
  CHECK(!p.teamMoves(p.mainSlot, mv, pp));
  CHECK(!p.teamEquipMove(p.mainSlot, MOVE_STRUGGLE));
  CHECK(!p.teamUnequipMove(p.mainSlot, 0));
  CHECK(!p.teamMoves(TEAM_SLOTS, mv, pp));     // y un hueco que no existe
}

TEST(golpes_suplente, un_hueco_vacio_no_tiene_golpes) {
  Pet p;
  makeLuchador(p, 4, 30);
  uint8_t libre = (uint8_t)(p.mainSlot == 0 ? 1 : 0);
  CHECK(!p.team[libre].used());
  uint16_t mv[4];
  uint8_t pp[4];
  CHECK(!p.teamMoves(libre, mv, pp));
  CHECK(!p.teamEquipMove(libre, MOVE_STRUGGLE));
}

TEST(golpes_suplente, lo_elegido_sobrevive_al_guardado) {
  // Es lo que fallaba con el titular: elegir y no grabar. Aqui el equipo va
  // en el blob de la NVS, asi que se comprueba el viaje de ida y vuelta.
  Pet p;
  makeLuchador(p, 4, 30);
  uint8_t s = ponSuplente(p, 7, 20);
  uint16_t mv[4];
  uint8_t pp[4];
  p.teamMoves(s, mv, pp);
  CHECK(p.teamUnequipMove(s, 1));   // teamUnequipMove ya graba

  Pet q;                            // encender otra vez
  q.begin();
  uint16_t mv2[4];
  uint8_t pp2[4];
  CHECK(q.teamMoves(s, mv2, pp2));
  CHECK_EQ((int)mv2[1], 0);
  CHECK_EQ((int)mv2[0], (int)mv[0]);
  CHECK(q.team[s].flags & SM_MOVES_PICKED);
}

TEST(golpes_suplente, si_cambia_la_tabla_de_golpes_se_rehace_todo) {
  // MOVE_TBL va ORDENADA POR NOMBRE: un golpe nuevo se mete en medio y corre
  // los indices de todos los de detras. Lo guardado como "SURF" pasaria a ser
  // otro golpe sin que nadie lo note (al entrar Johto fueron 46 golpes nuevos
  // y se movio media tabla). Al cargar con otra tabla se suelta la eleccion y
  // cada uno vuelve a su aprendizaje.
  Pet p;
  makeLuchador(p, 4, 30);
  uint8_t s = ponSuplente(p, 7, 20);
  CHECK(p.teamUnequipMove(s, 1));          // elige: pone las dos banderas
  CHECK(p.unequipMove(0));
  p.clean();                               // vuelca a la NVS

  {
    // el guardado dice que se hizo con OTRA tabla de golpes
    Preferences px;
    px.begin("tamapoke", false);
    px.putUShort("mvn", (uint16_t)(MOVE_COUNT + 7));
    px.end();
  }
  Pet q;
  q.begin();
  CHECK_MSG(!q.movesPicked, "la eleccion del titular se suelta");
  CHECK_MSG(!(q.team[s].flags & SM_MOVES_PICKED), "y la de los suplentes");
  // y nadie se queda sin golpes: ppSync() los repone del aprendizaje
  uint8_t n = 0;
  for (uint8_t i = 0; i < 4; i++) if (q.ppMove[i]) n++;
  CHECK_MSG(n > 0, "no puede quedarse sin ningun golpe");
  uint16_t mv[4];
  uint8_t pp[4];
  CHECK(q.teamMoves(s, mv, pp));
  CHECK(mv[0] != 0);
}

TEST(golpes_suplente, con_la_misma_tabla_no_se_toca_nada) {
  Pet p;
  makeLuchador(p, 4, 30);
  uint8_t s = ponSuplente(p, 7, 20);
  CHECK(p.teamUnequipMove(s, 1));
  p.clean();
  Pet q;
  q.begin();
  CHECK(q.team[s].flags & SM_MOVES_PICKED);
  uint16_t mv[4];
  uint8_t pp[4];
  CHECK(q.teamMoves(s, mv, pp));
  CHECK_EQ((int)mv[1], 0);
}

// La orden de MOVE_TBL esta congelada desde MOVE_COUNT_FIJO: lo nuevo va
// al final. Una partida guardada con la tabla de 725 (antes de la segunda
// leva) tiene los MISMOS indices que hoy, y tirarle la eleccion seria
// quitarle sin motivo las MT que enseno a mano.
TEST(golpes_suplente, la_tabla_que_solo_crecio_al_final_no_se_toca) {
  CHECK_MSG(MOVE_COUNT_FIJO < MOVE_COUNT,
            "premissa: hay golpes nuevos despues de la orden fija");
  Pet p;
  makeLuchador(p, 4, 30);
  uint8_t s = ponSuplente(p, 7, 20);
  CHECK(p.teamUnequipMove(s, 1));
  CHECK(p.unequipMove(0));
  p.clean();
  {
    Preferences px;
    px.begin("tamapoke", false);
    px.putUShort("mvn", (uint16_t)MOVE_COUNT_FIJO);
    px.end();
  }
  Pet q;
  q.begin();
  CHECK_MSG(q.movesPicked, "la eleccion del titular se queda");
  CHECK_MSG(q.team[s].flags & SM_MOVES_PICKED, "y la de los suplentes");
  uint16_t mv[4];
  uint8_t pp[4];
  CHECK(q.teamMoves(s, mv, pp));
  CHECK_EQ((int)mv[1], 0);
}

TEST(bolsa, las_vitaminas_suben_los_EVs) {
  Pet p;
  makeLuchador(p, 4, 30);
  // una por stat: con tres EVs, Proteina y Calcio habrian hecho lo mismo
  uint8_t prot = 0, calc = 0;
  for (uint8_t i = 1; i < ITEM_COUNT; i++) {
    if (ITEM_TBL[i].fx != IT_VITAMIN) continue;
    if (ITEM_TBL[i].arg == EV_ATK) prot = i;
    if (ITEM_TBL[i].arg == EV_SPA) calc = i;
  }
  CHECK(prot && calc && prot != calc);

  p.trAtk = p.trSpa = 0;
  p.takeItem(prot);
  CHECK(p.useItem(prot));
  CHECK_EQ((int)p.trAtk, 10);
  CHECK_MSG((int)p.trSpa == 0, "Proteina y Calcio ya no son el mismo objeto");

  p.takeItem(calc);
  CHECK(p.useItem(calc));
  CHECK_EQ((int)p.trSpa, 10);
  CHECK_EQ((int)p.trAtk, 10);
}

TEST(bolsa, la_vitamina_no_se_gasta_con_el_EV_al_tope) {
  Pet p;
  makeLuchador(p, 4, 30);
  uint8_t carbos = 0;
  for (uint8_t i = 1; i < ITEM_COUNT; i++)
    if (ITEM_TBL[i].fx == IT_VITAMIN && ITEM_TBL[i].arg == EV_SPE) carbos = i;
  CHECK(carbos != 0);
  p.trSpe = 100;
  p.takeItem(carbos);
  CHECK_MSG(!p.useItem(carbos), "al tope no haria nada: no se tira");
  CHECK_EQ((int)p.bag[carbos], 1);
}

TEST(bolsa, hay_una_vitamina_por_cada_stat) {
  bool visto[EV_N] = { false, false, false, false, false, false };
  for (uint8_t i = 1; i < ITEM_COUNT; i++)
    if (ITEM_TBL[i].fx == IT_VITAMIN && ITEM_TBL[i].arg < EV_N)
      visto[ITEM_TBL[i].arg] = true;
  for (uint8_t k = 0; k < EV_N; k++)
    CHECK_MSG(visto[k], "falta la vitamina de algun stat");
}
// ---------------------------------------------------- la energia como portero
// Es lo que ata el combate y el entrenamiento al ciclo de cuidado: cansan,
// dormir repone, y sin dormir no se hace ninguna de las dos.

TEST(energia, sin_fuelle_no_se_pelea) {
  Pet p;
  makeLuchador(p, 4, 30);
  p.energy = 100;
  CHECK(p.canBattle());
  p.energy = BATTLE_ENERGY_MIN;
  CHECK(p.canBattle());                       // justo en el umbral, si
  p.energy = BATTLE_ENERGY_MIN - 1;
  CHECK(!p.canBattle());
  CHECK_STREQ(p.whyNotBattle(), "too tired");
}

TEST(energia, sin_fuelle_no_se_entrena) {
  Pet p;
  makeLuchador(p, 4, 30);
  p.energy = TRAIN_ENERGY_MIN;
  CHECK(p.canTrain());
  p.energy = TRAIN_ENERGY_MIN - 1;
  CHECK(!p.canTrain());
}

TEST(energia, entrenar_cansa_mas_que_pelear_una_vez) {
  CHECK(TRAIN_ENERGY_COST > BATTLE_ENERGY_COST);
  CHECK(TRAIN_ENERGY_MIN > BATTLE_ENERGY_MIN);
}

TEST(energia, durmiendo_no_se_hace_ninguna_de_las_dos) {
  Pet p;
  makeLuchador(p, 4, 30);
  p.energy = 100;
  p.toggleLight();
  CHECK(p.sleeping);
  CHECK(!p.canBattle());
  CHECK(!p.canTrain());
  CHECK_STREQ(p.whyNotBattle(), "fast asleep");
}

TEST(energia, el_motivo_se_puede_contar) {
  // Dejar un boton muerto sin decir por que es de las cosas que mas confunden.
  Pet p;
  makeLuchador(p, 4, 30);
  p.energy = 100;
  CHECK_STREQ(p.whyNotBattle(), "");
  p.afterBattle(1);                           // queda en los huesos
  if (p.sleeping) p.toggleLight();
  p.energy = 100;
  CHECK_STREQ(p.whyNotBattle(), "too hurt");
}

TEST(energia, pelear_y_entrenar_acaban_cerrando_la_puerta) {
  // El ciclo completo: se pelea hasta quedarse sin fuelle y hay que dormir.
  Pet p;
  makeLuchador(p, 4, 30);
  p.energy = 100;
  int combates = 0;
  while (p.canBattle() && combates < 50) {
    p.afterBattle(p.maxHp());                 // gana sin un rasguno
    combates++;
  }
  CHECK_MSG(combates > 3, "tampoco puede cerrarse a la primera");
  CHECK_MSG(combates < 20, "pero no se puede moler sin fin");
  CHECK(!p.canBattle());

  p.toggleLight();                            // a dormir
  advance(p, 120);
  if (p.sleeping) p.toggleLight();
  CHECK_MSG(p.canBattle(), "dormir tiene que devolver la posibilidad de pelear");
}

// ---------------------------------------- lo que llega por el acelerometro
// El driver (imu.cpp) no se puede probar sin la placa, y los gestos tienen
// su propia bateria en test_gestos.cpp. Lo que se prueba aqui es la otra
// mitad: que el gesto, cuando llega, le haga al bicho lo que debe.

TEST(gestos_bicho, un_zarandeo_le_quita_alegria_pero_no_lo_hunde) {
  Pet p;
  makePet(p, 143);
  p.joy = 50;
  p.scare();
  CHECK_MSG(p.joy < 50, "zarandearlo tiene que molestarle");
  // y no se pasa de cero por abajo, que es el fallo clasico de un uint8_t
  p.joy = 1;
  p.scare();
  CHECK_MSG(p.joy == 0, "con 1 de alegria baja a 0, no da la vuelta a 255");
}

TEST(gestos_bicho, durmiendo_no_se_le_asusta) {
  Pet p;
  makePet(p, 143);
  p.joy = 50;
  p.sleeping = true;
  p.scare();
  CHECK_MSG(p.joy == 50, "dormido ya esta quieto: un meneo no le asusta");
}

TEST(gestos_bicho, boca_abajo_lo_acuesta_y_dos_tapas_lo_levantan) {
  Pet p;
  makePet(p, 143);
  CHECK_MSG(!p.sleeping, "empieza despierto");
  CHECK_MSG(p.restDown(), "boca abajo lo acuesta");
  CHECK_MSG(p.sleeping, "y queda dormido");
  CHECK_MSG(!p.restDown(), "ya dormido, no se acuesta dos veces");
  CHECK_MSG(p.wakeUp(), "dos tapas lo levantan");
  CHECK_MSG(!p.sleeping, "y queda despierto");
  CHECK_MSG(!p.wakeUp(), "despierto, no hay a quien levantar");
}

TEST(gestos_bicho, a_un_desmayado_NO_se_le_despierta_a_tapas) {
  // El test que de verdad importa. Dormir es como se cura (ver faint()),
  // asi que dos golpecitos sin querer no pueden cortarle la cura. Por eso
  // wakeUp() mira restByGesture y no solo sleeping.
  Pet p;
  makePet(p, 143);
  p.sleeping = true;          // como lo deja un desmayo
  p.restByGesture = false;    // NO lo acosto el jugador
  CHECK_MSG(!p.wakeUp(),
            "un desmayado no se despierta a tapas: dormir es su cura");
  CHECK_MSG(p.sleeping, "y sigue durmiendo");
}

TEST(gestos_bicho, andar_quema_peso_y_anima) {
  Pet p;
  makePet(p, 143);
  p.weight = 40;
  p.joy = 50;
  p.walk(PASOS_TRAMO * 5);      // cinco tramos
  CHECK_MSG(p.weight < 40, "andar quema peso");
  CHECK_MSG(p.joy > 50, "y anima");
  CHECK_MSG(p.stepsToday == PASOS_TRAMO * 5, "los pasos del dia se guardan");
}

TEST(gestos_bicho, los_pasos_se_cobran_por_tramos_y_el_resto_se_guarda) {
  // Si se cobraran por paso, cien pasos darian cien puntos de alegria y
  // el bicho estaria siempre al maximo. Y si el resto se tirara, andar en
  // tandas cortas no contaria nunca.
  Pet p;
  makePet(p, 143);
  p.weight = 50;
  uint8_t antes = p.weight;
  for (int i = 0; i < PASOS_TRAMO - 1; i++) p.walk(1);
  CHECK_MSG(p.weight == antes,
            "un tramo incompleto todavia no cobra");
  p.walk(1);                    // el que completa el tramo
  CHECK_MSG(p.weight == antes - 1, "y al completarlo, cobra UNA vez");
}

TEST(gestos_bicho, durmiendo_no_se_anda) {
  Pet p;
  makePet(p, 143);
  p.weight = 40;
  p.sleeping = true;
  p.walk(PASOS_TRAMO * 3);
  CHECK_MSG(p.weight == 40, "dormido en el bolsillo no hace ejercicio");
}

TEST(gestos_bicho, una_caricia_a_tapas_es_la_MISMA_que_tocar_la_pantalla) {
  // Lo importante del enganche: no hay dos sistemas de mimos. La tapa
  // llama a caress(), asi que respeta el tope diario de vinculo y
  // registra el cuidado igual que el dedo.
  Pet a, b;
  makePet(a, 143);
  makePet(b, 143);
  a.joy = b.joy = 50;
  a.caress();                    // "el dedo"
  b.caress();                    // "la tapa" — es la misma llamada
  CHECK_MSG(a.joy == b.joy, "la misma accion da el mismo resultado");
  CHECK_MSG(a.bond == b.bond, "y el mismo vinculo");
  // y el tope diario sigue valiendo: no se farmea a tapas
  Pet c;
  makePet(c, 143);
  // El tope es de GANANCIA diaria, no de valor: el bicho ya nace con
  // vinculo. Mi primera version comparo el valor absoluto y dio en rojo
  // con razon — el codigo estaba bien y la afirmacion mal.
  uint8_t antes = c.bond;
  for (int i = 0; i < 100; i++) c.caress();
  CHECK_MSG(c.bond - antes <= 20,
            "cien caricias no pueden dar mas de 20 de vinculo al dia; "
            "dieron " + std::to_string((int)(c.bond - antes)));
  CHECK_MSG(c.bond > antes, "pero algo si dan");
}

// "o heal de poke center nao ta curando os PPs dos ataques (ao menos do time)"
TEST(centro, curar_repone_los_PP_del_equipo_y_de_la_caja) {
  Pet p;
  makePet(p, 4);
  setLevel(p, 20);
  p.hpLeft = p.maxHp();
  p.ppRestoreAll();
  const uint16_t mv = p.ppMove[0];
  CHECK_MSG(mv != 0 && movePP(mv) > 1, "premisa: el titular tiene un golpe con PP");
  auto vacio = [&](StoredMon &m, int16_t esp) {
    m.species = esp;
    m.exp = Pet::expForLevel(12);
    m.hpLeft = hpAtLevelForm(esp, 12, m.trHp, m.form);   // PS llenos: solo faltan PP
    m.moves[0] = mv;
    m.ppLeft[0] = 0;
  };
  uint8_t hueco = p.mainSlot == 1 ? 2 : 1;
  vacio(p.team[hueco], 16);
  vacio(p.bank[3], 25);
  CHECK_MSG(p.needsHealing(), "sin PP en el equipo, la enfermera tiene que tener trabajo");
  uint8_t n = p.healAll();
  CHECK_EQ((int)p.team[hueco].ppLeft[0], (int)movePP(mv));
  CHECK_EQ((int)p.bank[3].ppLeft[0], (int)movePP(mv));
  CHECK_EQ((int)n, 2);                       // el cartel los cuenta
  CHECK_EQ((int)p.team[hueco].ppLeft[1], 0); // un hueco sin golpe sigue vacio
  CHECK_MSG(!p.needsHealing(), "despues de curar no puede quedar nada");
}

// =====================================================================
// LA AVENTURA: lo que se gana sale del reloj, con limite. "se nao eu deixo
// desligado uma semana, e o pokemon volta lvl 100".
static void conEquipoParaAventura(Pet &p) {
  makePet(p, 4);
  setLevel(p, 20);
  p.badges = 8;
  p.ppRestoreAll();
  uint8_t h = p.mainSlot == 1 ? 2 : 1;
  p.team[h].species = 16;
  p.team[h].exp = Pet::expForLevel(20);
  p.bank[2].species = 25;
  p.bank[2].exp = Pet::expForLevel(20);
}
#define AV_T0 1760000000u

TEST(aventura, ya_desde_el_primer_minuto_da_algo_y_antes_nada) {
  // "pode ter exp desde o primeiro minuto... pouca mas pode"
  Pet p;
  conEquipoParaAventura(p);
  CHECK(p.aventuraComeca(AV_T0, 77));
  uint32_t e0 = p.exp;
  AvResumo r;
  p.aventuraFim(AV_T0 + 30, r);          // medio minuto: aun nada
  CHECK_EQ((int)r.nMons, 0);
  CHECK_EQ((long)p.exp, (long)e0);
  CHECK_MSG(!p.aventuraAtiva(), "acabada, tiene que dejar de estar activa");
  // un minuto: poco, pero se nota, tambien a nivel bajo
  Pet q;
  conEquipoParaAventura(q);
  setLevel(q, 2);
  q.aventuraComeca(AV_T0, 77);
  uint32_t e1 = q.exp;
  q.aventuraFim(AV_T0 + 60, r);
  CHECK_MSG(q.exp > e1, "un minuto no dio nada");
  CHECK_EQ((int)r.nItens, 0);            // objetos, solo con tiempo
  CHECK_EQ((int)q.level(), 2);
}

TEST(aventura, salen_varios_de_la_caja_y_no_siempre_los_mismos) {
  // "nao precisa ser sempre os mesmos, pode ser os pokemons do banco"
  Pet p;
  conEquipoParaAventura(p);
  for (uint8_t i = 0; i < BANK_SLOTS && i < 20; i++) {
    p.bank[i].species = (int16_t)(100 + i);
    p.bank[i].exp = Pet::expForLevel(10);
  }
  AvParticipa e[AV_MAX_ELENCO];
  bool visto[20] = { false };
  for (uint32_t s = 1; s <= 40; s++) {
    p.avSeed = s;
    uint8_t n = p.aventuraElenco(e), caja = 0;
    for (uint8_t i = 0; i < n; i++)
      if (e[i].origen == 2) { caja++; visto[e[i].dex - 100] = true; }
    CHECK_EQ((int)caja, AV_DE_LA_CAJA);
  }
  int distintos = 0;
  for (bool v : visto) if (v) distintos++;
  CHECK_MSG(distintos >= 18, "en 40 aventuras solo salieron " + std::to_string(distintos) + " de 20");
}

TEST(aventura, una_semana_rinde_lo_mismo_que_ocho_horas) {
  Pet a, b;
  conEquipoParaAventura(a);
  conEquipoParaAventura(b);
  a.aventuraComeca(AV_T0, 77);
  b.aventuraComeca(AV_T0, 77);
  uint32_t e0 = a.exp;
  AvResumo ra, rb;
  a.aventuraFim(AV_T0 + 8 * 3600, ra);
  b.aventuraFim(AV_T0 + 7 * 24 * 3600, rb);
  CHECK_EQ((long)a.exp, (long)b.exp);
  CHECK_EQ((int)ra.minutos, AV_TETO_MIN);
  CHECK_EQ((int)rb.minutos, AV_TETO_MIN);
  CHECK_MSG(a.exp > e0, "ocho horas tienen que dar algo");
  // y el techo es de verdad pequeno: a nivel 20, menos de dos niveles
  CHECK_MSG(a.level() <= 21, "ocho horas subieron a nivel " + std::to_string(a.level()));
  CHECK_EQ((int)ra.nItens, (int)rb.nItens);
  CHECK_MSG(ra.nItens <= AV_MAX_ITENS, "mas objetos que el maximo");
}

TEST(aventura, rinde_cada_vez_menos_con_el_tiempo) {
  Pet a, b;
  conEquipoParaAventura(a);
  conEquipoParaAventura(b);
  a.aventuraComeca(AV_T0, 5);
  b.aventuraComeca(AV_T0, 5);
  uint32_t e0 = a.exp;
  AvResumo r;
  a.aventuraFim(AV_T0 + 2 * 3600, r);
  b.aventuraFim(AV_T0 + 4 * 3600, r);
  uint32_t dos = a.exp - e0, cuatro = b.exp - e0;
  CHECK_MSG(cuatro > dos && cuatro < dos * 2,
            "2h " + std::to_string(dos) + " 4h " + std::to_string(cuatro));
}

TEST(aventura, reparte_titular_equipo_y_caja_y_cobra_una_vez) {
  Pet p;
  conEquipoParaAventura(p);
  p.aventuraComeca(AV_T0, 9);
  uint8_t h = p.mainSlot == 1 ? 2 : 1;
  uint32_t eTit = p.exp, eEq = p.team[h].exp, eCaja = p.bank[2].exp;
  AvResumo r;
  p.aventuraFim(AV_T0 + 2 * 3600, r);
  CHECK_EQ((int)r.nMons, 3);
  uint32_t dT = p.exp - eTit, dE = p.team[h].exp - eEq, dC = p.bank[2].exp - eCaja;
  CHECK_MSG(dT > 0, "el titular no gano");
  CHECK_EQ((long)dE, (long)dT / 2);
  CHECK_EQ((long)dC, (long)dT / 4);
  CHECK_EQ((long)r.mons[0].exp, (long)dT);
  // otra vez: nada
  uint32_t ya = p.exp;
  AvResumo r2;
  p.aventuraFim(AV_T0 + 9 * 3600, r2);
  CHECK_EQ((long)p.exp, (long)ya);
  CHECK_EQ((int)r2.nMons, 0);
}

TEST(aventura, no_pasa_del_techo_de_nivel_de_la_region) {
  Pet p;
  conEquipoParaAventura(p);
  p.badges = 0;
  uint16_t tope = p.levelCap();
  setLevel(p, (uint16_t)(tope - 1));
  p.exp = Pet::expForLevel(tope) - 1;
  p.aventuraComeca(AV_T0, 3);
  AvResumo r;
  p.aventuraFim(AV_T0 + 8 * 3600, r);
  CHECK_MSG(p.exp <= Pet::expForLevel(tope), "se paso del techo");
  // y el que ya esta por encima no gana ni pierde
  Pet q;
  conEquipoParaAventura(q);
  q.badges = 0;
  setLevel(q, (uint16_t)(q.levelCap() + 5));
  uint32_t e = q.exp;
  q.aventuraComeca(AV_T0, 3);
  q.aventuraFim(AV_T0 + 8 * 3600, r);
  CHECK_EQ((long)q.exp, (long)e);
}

TEST(aventura, los_objetos_son_los_que_la_cena_dijo_y_llegan_a_la_mochila) {
  Pet p;
  conEquipoParaAventura(p);
  for (uint16_t i = 0; i < ITEM_COUNT; i++) p.bag[i] = 0;
  p.aventuraComeca(AV_T0, 1234);
  // lo que se promete ANTES de cobrar
  uint8_t dicho[AV_MAX_ITENS], n = 0;
  uint16_t rolos = p.aventuraRolos(AV_T0 + 8 * 3600);
  CHECK_EQ((int)rolos, AV_TETO_MIN / AV_ROLO_MIN);
  for (uint16_t i = 0; i < rolos && n < AV_MAX_ITENS; i++) {
    uint8_t id = p.aventuraItem(i);
    if (id) dicho[n++] = id;
  }
  CHECK_MSG(n > 0, "premisa: con esta semilla sale algo");
  AvResumo r;
  p.aventuraFim(AV_T0 + 8 * 3600, r);
  CHECK_EQ((int)r.nItens, (int)n);
  for (uint8_t i = 0; i < n; i++) CHECK_EQ((int)r.itens[i], (int)dicho[i]);
  int enBolsa = 0;
  for (uint16_t i = 0; i < ITEM_COUNT; i++) enBolsa += p.bag[i];
  CHECK_EQ(enBolsa, (int)n);
}

TEST(aventura, a_veces_sale_una_piedra_mega) {
  Pet p;
  conEquipoParaAventura(p);
  int mega = 0, nada = 0, tot = 0;
  for (uint32_t s = 1; s <= 400; s++) {
    p.avSeed = s;
    for (uint16_t i = 0; i < 12; i++) {
      uint8_t id = p.aventuraItem(i);
      tot++;
      if (!id) nada++;
      else {
        CHECK_MSG(ITEM_TBL[id].pocket != BP_KEY, "salio un objeto clave");
        if (ITEM_TBL[id].fx == IT_MEGA) mega++;
      }
    }
  }
  CHECK_MSG(mega > tot / 200 && mega < tot / 20, "mega " + std::to_string(mega) + "/" + std::to_string(tot));
  CHECK_MSG(nada > tot / 4 && nada < tot / 2, "nada " + std::to_string(nada) + "/" + std::to_string(tot));
}

TEST(aventura, de_aventura_no_pasa_hambre_ni_descuidos_ni_se_escapa) {
  Pet p;
  conEquipoParaAventura(p);
  p.fullness = p.joy = p.hygiene = 60;
  p.setClock(AV_T0);          // o `seen` guardado: sem ele o syncClock nao conta nada
  p.aventuraComeca(AV_T0, 3);
  uint8_t care = p.careMistakes;
  p.update((uint32_t)PET_TICK_MS * 2000);
  CHECK_EQ((int)p.fullness, 60);
  CHECK_EQ((int)p.joy, 60);
  CHECK_EQ((int)p.hygiene, 60);
  CHECK_EQ((int)p.careMistakes, (int)care);
  CHECK_MSG(!p.canRunawayNow(), "se puede escapar estando de aventura");
  // y la vuelta con el aparato apagado tampoco le cobra el tiempo
  // Sem aventura, o mesmo tempo cobra (a premissa: senao isto nao mede nada)
  Pet q;
  conEquipoParaAventura(q);
  q.fullness = q.joy = q.hygiene = 60;
  q.setClock(AV_T0);
  q.syncClock(AV_T0 + 3 * 24 * 3600);
  CHECK_MSG(q.fullness < 60, "premissa: tres dias fora dao fome");
  Pet r;
  conEquipoParaAventura(r);
  r.fullness = r.joy = r.hygiene = 60;
  r.setClock(AV_T0);
  r.aventuraComeca(AV_T0, 3);
  r.ageMinutes = 1000;
  uint32_t idade = r.ageMinutes;
  r.syncClock(AV_T0 + 3 * 24 * 3600);
  CHECK_EQ((int)r.fullness, 60);
  // e nem envelhece enquanto esta fora
  CHECK_EQ((long)r.ageMinutes, (long)idade);
}

TEST(aventura, sobrevive_a_apagar_y_no_se_abre_con_un_huevo) {
  Pet p;
  conEquipoParaAventura(p);
  CHECK(p.aventuraComeca(AV_T0, 42));
  p.saveNow();
  Pet q;
  q.begin();
  CHECK_MSG(q.aventuraAtiva(), "se perdio al apagar");
  CHECK_EQ((long)q.avDesde, (long)AV_T0);
  CHECK_EQ((long)q.avSeed, 42L);
  AvResumo r;
  q.aventuraFim(AV_T0 + 3600, r);
  Pet z;
  z.begin();
  CHECK_MSG(!z.aventuraAtiva(), "cobrada, no puede volver al encender");
  CHECK_MSG(!p.aventuraComeca(0, 1), "sin reloj no empieza");
}

// Pausar ("GUARDAR") con la aventura en marcha: el tiempo sigue contando para
// ella, y la marca de pausa se gasta en esa vuelta. Si sobrara, el proximo
// apagado normal no cobraria nada.
TEST(aventura, la_pausa_no_sobra_para_despues_de_la_aventura) {
  Pet p;
  conEquipoParaAventura(p);
  p.fullness = p.joy = p.hygiene = 80;
  p.setClock(AV_T0);
  p.aventuraComeca(AV_T0, 5);
  p.guardar(AV_T0 + 60);                     // pausar y apagar
  p.syncClock(AV_T0 + 86400);                // un dia despues
  CHECK_EQ((int)p.fullness, 80);             // de aventura: sin hambre
  CHECK_EQ((long)p.aventuraMinutos(AV_T0 + 86400), (long)AV_TETO_MIN);   // y conto
  AvResumo r;
  p.aventuraFim(AV_T0 + 86400, r);
  CHECK_MSG(r.mons[0].exp > 0, "premisa: la aventura pago");
  // y ahora un apagado NORMAL de dos dias: tiene que cobrar
  p.syncClock(AV_T0 + 3 * 86400);
  CHECK_MSG(p.fullness < 80, "la marca de pausa sobro y se comio dos dias");
}


// 2026-11-08: a NVS encheu em Hoenn, nada mais se gravou e o reinicio levou a
// regiao inteira. Num save em que os blocos grandes nao mudam, so falham as
// escritas pequenas, e essas ninguem olhava. A sentinela, por ultimo, olha.
TEST(guardado, a_sentinela_ve_a_nvs_cheia_mesmo_sem_bloco_grande) {
  Pet p;
  makePet(p, 25);
  p.saveNow();
  CHECK_MSG(!p.guardadoFallo, "premissa: com sitio, grava");
  mockNvsRecusaEscalares(true);
  p.fullness = (uint8_t)(p.fullness > 50 ? 10 : 90);   // muda so um byte
  p.saveNow();
  mockNvsRecusaEscalares(false);
  CHECK_MSG(p.guardadoFallo, "so as escritas pequenas falharam e o save nao se enteirou");
  p.saveNow();
  CHECK_MSG(!p.guardadoFallo, "com sitio de novo, a falha nao some");
}

// O bloco unico "teams" (2,8 KB) repetia tm0..tm8 e foi o que encheu a NVS. O
// save nao o escreve mais, e o boot apaga o que sobrou SO com as nove chaves boas.
static bool temChave(const char *k) {
  Preferences pr;
  pr.begin("tamapoke", true);
  bool r = pr.isKey(k);
  pr.end();
  return r;
}

TEST(regioes, o_bloco_teams_sai_e_nao_volta) {
  Pet p;
  makePet(p, 4);
  for (uint8_t i = 1; i < TEAM_SLOTS; i++) CHECK(p.addToTeam(unMon((int16_t)(150 + i), 50)));
  p.saveNow();
  CHECK_MSG(!temChave("teams"), "o save ainda escreve o bloco unico");
  {
    Preferences pr;   // o que o firmware de ontem deixou na NVS
    pr.begin("tamapoke", false);
    pr.putBytes("teams", p.teamsIn, sizeof(p.teamsIn));
    pr.putBytes("teamsbk", p.teamsIn, sizeof(p.teamsIn));
    pr.end();
  }
  Pet q;
  mockSerialSilence(true);
  q.begin();
  mockSerialSilence(false);
  CHECK_MSG(!temChave("teams"), "com as nove chaves boas, o bloco velho ficou");
  CHECK_MSG(!temChave("teamsbk"), "a copia velha ficou");
  for (uint8_t i = 0; i < TEAM_SLOTS; i++)
    CHECK_MSG(q.team[i].species == p.team[i].species, "o time mudou no slot " + std::to_string(i));
  q.saveNow();
  CHECK_MSG(!temChave("teams"), "voltou no save seguinte");
}

TEST(regioes, com_uma_chave_ruim_o_bloco_teams_fica) {
  // (chave que FALTA nao conta: e regiao nunca pisada)
  Pet p;
  makePet(p, 4);
  p.saveNow();
  {
    Preferences pr;
    pr.begin("tamapoke", false);
    pr.putBytes("teams", p.teamsIn, sizeof(p.teamsIn));
    uint8_t ruim[2 + sizeof(StoredMon) * TEAM_SLOTS] = { 7 };   // carimbo errado
    pr.putBytes("tm0", ruim, sizeof(ruim));
    uint8_t boa[2 + sizeof(StoredMon) * TEAM_SLOTS] = { 1 };    // Johto, vazia mas certa
    pr.putBytes("tm1", boa, sizeof(boa));
    pr.end();
  }
  Pet q;
  mockSerialSilence(true);
  q.begin();
  mockSerialSilence(false);
  CHECK_MSG(temChave("teams"), "apagou o bloco velho com uma chave faltando");
}


// ------------------------------------------------ as regioes no SD (leva 44)
// "bola um jeito desta info da regiao ficar no SD" —— depois de a NVS cheia
// ter levado Hoenn.

// Um Pet em Johto com Kanto feito: dois times de verdade.
static void doisTimes(Pet &p) {
  sdDePruebas();
  makePet(p, 4);
  for (uint8_t i = 1; i < TEAM_SLOTS; i++) CHECK(p.addToTeam(unMon((int16_t)(150 + i), 50)));
  p.maxRegion = 1;
  p.badges = regionAt(0).count;
  p.badgesIn[0] = p.badges;
  p.saveNow();
  CHECK(p.setZone(1));                       // Johto: ovo do inicial
}

TEST(regioes, as_regioes_vivem_no_sd_e_saem_da_nvs) {
  Pet p;
  doisTimes(p);
  CHECK_MSG(svExiste("reg00.bin") && svExiste("reg01.bin"), "as regioes nao foram para o SD");
  CHECK_MSG(svExiste("estado.bin"), "o estado nao foi para o SD");
  CHECK_MSG(!temChave("tm0") && !temChave("tm1"), "a copia da NVS ficou (e ela e o que enche)");
  // O save do dono HOJE: as nove chaves ainda na NVS. O boot leva-as para o
  // SD e so entao as apaga.
  {
    uint8_t buf[2 + sizeof(StoredMon) * TEAM_SLOTS];
    buf[0] = 0;
    buf[1] = p.mainSlotIn[0];
    memcpy(buf + 2, p.teamsIn[0], sizeof(StoredMon) * TEAM_SLOTS);
    Preferences pr;
    pr.begin("tamapoke", false);
    pr.putBytes("tm0", buf, sizeof(buf));
    pr.end();
  }
  svApagaDir();
  Pet q;
  mockSerialSilence(true);
  q.begin();
  mockSerialSilence(false);
  CHECK_EQ((int)q.region, 1);
  for (uint8_t i = 0; i < TEAM_SLOTS; i++)
    CHECK_MSG(q.teamsIn[0][i].species == p.teamsIn[0][i].species, "Kanto mudou no slot " + std::to_string(i));
  CHECK_MSG(svExiste("reg00.bin"), "o boot nao levou Kanto da NVS para o SD");
  CHECK_MSG(!temChave("tm0"), "o boot levou Kanto para o SD e deixou a chave na NVS");
}

// O CASO DE HOENN: a NVS para de gravar, o SD nao. No boot o SD e mais novo
// e manda: a regiao, as insignias e o time voltam.
TEST(regioes, com_a_nvs_parada_o_sd_traz_a_regiao_de_volta) {
  Pet p;
  doisTimes(p);
  const int16_t titularJohto = p.speciesId;
  mockNvsRecusaEscalares(true);              // a NVS cheia: nada pequeno grava...
  mockNvsTopeBlob(1);                        // ...nem grande
  p.maxRegion = 2;
  p.badges = regionAt(1).count;
  p.badgesIn[1] = p.badges;
  CHECK(p.setZone(2));                       // Hoenn, so na RAM e no SD
  CHECK_MSG(p.guardadoFallo, "premissa: a NVS recusou");
  const int16_t titularHoenn = p.speciesId;
  CHECK_MSG(titularHoenn != titularJohto || p.isEgg(), "premissa: o titular mudou");
  mockNvsRecusaEscalares(false);
  mockNvsTopeBlob(0);
  Pet q;                                     // o reinicio
  mockSerialSilence(true);
  q.begin();
  mockSerialSilence(false);
  CHECK_MSG(q.sdManda, "o boot nao viu que o SD era mais novo");
  CHECK_EQ((int)q.region, 2);
  CHECK_EQ((int)q.maxRegion, 2);
  CHECK_MSG(q.regionVisited(2), "Hoenn nao conta como pisada");
  CHECK_EQ((int)q.badgesIn[1], (int)regionAt(1).count);
  CHECK_EQ((int)q.speciesId, (int)titularHoenn);
  for (uint8_t i = 0; i < TEAM_SLOTS; i++)
    CHECK_MSG(q.teamsIn[0][i].species == p.teamsIn[0][i].species, "Kanto mudou");
}

// Com a NVS boa, ela manda (o SD so empata): nada de "voltar no tempo".
TEST(regioes, com_a_nvs_boa_o_sd_nao_manda) {
  Pet p;
  doisTimes(p);
  Pet q;
  mockSerialSilence(true);
  q.begin();
  mockSerialSilence(false);
  CHECK_MSG(!q.sdManda, "o SD mandou com a NVS em dia");
  CHECK_EQ((int)q.region, 1);
}

// Sem SD, grava na NVS como antes, e le de volta.
TEST(regioes, sem_sd_as_regioes_ficam_na_nvs) {
  Pet p;
  mockNvsReset();
  mockSdRoot("build/sd-que-no-esta/ni-de-lejos");
  makePet(p, 4);
  p.saveNow();
  CHECK_MSG(temChave("tm0"), "sem SD, Kanto nao foi para a NVS");
  Pet q;
  mockSerialSilence(true);
  q.begin();
  mockSerialSilence(false);
  CHECK_EQ((int)q.teamsIn[0][0].species, (int)p.teamsIn[0][0].species);
  sdDePruebas();
}

// Um arquivo com um byte trocado nao se carrega: o CRC o recusa, e vale o .new.
TEST(regioes, arquivo_corrompido_e_recusado) {
  Pet p;
  doisTimes(p);
  {
    std::error_code ec;
    std::filesystem::copy_file(svCaminho("reg00.bin"), svCaminho("reg00.new"),
                               std::filesystem::copy_options::overwrite_existing, ec);
    FILE *f = fopen(svCaminho("reg00.bin").c_str(), "r+b");
    CHECK_MSG(f, "premissa: abrir o arquivo");
    fseek(f, 40, SEEK_SET);
    fputc(0x5a, f);
    fclose(f);
  }
  Pet q;
  mockSerialSilence(true);
  q.begin();
  mockSerialSilence(false);
  for (uint8_t i = 0; i < TEAM_SLOTS; i++)
    CHECK_MSG(q.teamsIn[0][i].species == p.teamsIn[0][i].species, "o .new nao salvou Kanto");
  // e sem o .new, Kanto fica vazio (um erro que se ve) em vez de lixo
  std::filesystem::remove(svCaminho("reg00.new"));
  Pet r;
  mockSerialSilence(true);
  r.begin();
  mockSerialSilence(false);
  uint8_t n = 0;
  for (uint8_t i = 0; i < TEAM_SLOTS; i++) if (r.teamsIn[0][i].used()) n++;
  CHECK_EQ((int)n, 0);
}

// Uma regiao pisada cujo time nao se le: nao se viaja (o ovo gravaria por cima).
TEST(regioes, sem_ler_o_time_nao_se_viaja) {
  Pet p;
  doisTimes(p);
  for (uint8_t i = 0; i < TEAM_SLOTS; i++) p.teamsIn[0][i].species = 0;   // memoria vazia
  svApagaDir();                                                           // e SD sem nada
  CHECK_MSG(!p.setZone(0), "viajou para Kanto sem conseguir ler o time");
  CHECK_EQ((int)p.region, 1);
}

// Partida nova: o /save da velha sai.
TEST(regioes, partida_nova_apaga_o_save_do_sd) {
  Pet p;
  doisTimes(p);
  // Os da partida velha em JOHTO: a nova comeca em Kanto e nao regrava
  // Johto, entao so o apagar os tira de la.
  for (uint8_t i = 0; i < TEAM_SLOTS; i++)
    if (!p.team[i].used()) p.team[i] = unMon((int16_t)(151 + i % 5), 30);
  p.saveNow();
  CHECK_MSG(svExiste("reg01.bin"), "premissa");
  {
    Preferences pr;
    pr.begin("tamapoke", false);
    pr.clear();
    pr.end();
  }
  Pet q;
  mockSerialSilence(true);
  q.begin();
  mockSerialSilence(false);
  // A partida nova ja grava o proprio save; o que nao pode e voltar a VELHA.
  Pet r;
  mockSerialSilence(true);
  r.begin();
  mockSerialSilence(false);
  for (uint8_t reg = 0; reg < REGION_COUNT; reg++)
    for (uint8_t i = 0; i < TEAM_SLOTS; i++)
      CHECK_MSG(r.teamsIn[reg][i].species < 151 || r.teamsIn[reg][i].species > 155,
                "um bicho da partida velha voltou na regiao " + std::to_string(reg));
  CHECK_EQ((int)r.region, 0);
}


// ---- leva 72: VISTOS na Pokedex e a CACADA ----
TEST(vistos72, visto_nao_e_registrado_e_o_registrado_conta_como_visto) {
  Pet p;
  makePet(p, 4);
  CHECK_MSG(!p.isSeen(25), "premissa: o Pikachu ainda nao apareceu");
  p.markSeen(25);
  CHECK_MSG(p.isSeen(25), "markSeen nao marcou");
  CHECK_MSG(!p.isRegistered(25), "VER nao pode registrar: registrar e ter");
  CHECK_MSG(p.isSeen(4), "o que voce teve tambem conta como visto");
}

TEST(vistos72, vistos_e_cacada_sobrevivem_ao_reinicio_e_o_borrado_os_limpa) {
  Pet a;
  makePet(a, 4);
  a.markSeen(25);
  a.setCaca(25);
  CHECK_MSG(a.savePending(), "cacar nao marcou o save pendente");
  a.flushSave();
  Pet b;
  b.begin();
  CHECK_MSG(b.isSeen(25), "o visto se perdeu ao reiniciar");
  CHECK_MSG(!b.isRegistered(25), "o visto voltou como registrado");
  CHECK_EQ(b.cacaDex, (int16_t)25);
  a.factoryReset();
  Pet c;
  c.begin();
  CHECK_MSG(!c.isSeen(25), "o borrado de fabrica deixou o visto");
  CHECK_EQ(c.cacaDex, (int16_t)0);
}

// Quantas das buscas no mato trazem o 16 (Pidgey), com ou sem cacada.
static double fracaoCacada72(int16_t caca, int16_t alvo) {
  randomSeed(72);
  Pet p;
  makePet(p, 4);
  setStats(p, 90, 90, 90, 90);
  p.markSeen(alvo);
  if (caca) p.setCaca(caca);
  int saiu = 0, alvoN = 0;
  for (int i = 0; i < 4000; i++) {
    p.wildDex = 0;
    p.wildTtl = 0;
    p.huntUntil = 0;
    if (!p.hunt(12, 1000)) continue;
    saiu++;
    if (p.wildDex == alvo) alvoN++;
  }
  return saiu ? (double)alvoN / saiu : -1.0;
}

TEST(vistos72, a_cacada_traz_o_cacado_uma_de_cada_quatro_buscas) {
  Pet q;
  makePet(q, 4);
  CHECK_MSG(q.wildWeight(16, 12) > 0, "premissa: o Pidgey tem de poder sair no mato");
  double sem = fracaoCacada72(0, 16), com = fracaoCacada72(16, 16);
  // esperado: com = 1/4 + 3/4 * sem
  double esperado = 0.25 + 0.75 * sem;
  CHECK_MSG(sem >= 0.0 && sem < 0.15, "premissa: sem cacada o Pidgey e so mais um: " +
            std::to_string(sem));
  CHECK_MSG(com > esperado - 0.04 && com < esperado + 0.04,
            "a cacada nao puxa 1 em 4: sem " + std::to_string(sem) + ", com " +
            std::to_string(com) + ", esperado " + std::to_string(esperado));
}

TEST(vistos72, o_que_nao_sai_no_mato_nao_pode_ser_cacado_nem_aparece) {
  Pet q;
  makePet(q, 4);
  q.badges = 0;
  q.markSeen(150);
  CHECK_MSG(!q.podeCacar(150, 12), "sem as 8 insignias o lendario nao se caca");
  CHECK_MSG(q.podeCacar(16, 12) == false, "premissa: sem ver o Pidgey nao se caca");
  q.markSeen(16);
  CHECK_MSG(q.podeCacar(16, 12), "o Pidgey visto tem de poder ser cacado");
  // e mesmo forcando a cacada (um save velho, uma viagem), o Mewtwo nao sai
  // sem as insignias (fracaoCacada72 cria o bicho com badges 0)
  double f = fracaoCacada72(150, 150);
  CHECK_MSG(f == 0.0, "o Mewtwo cacado saiu no mato: " + std::to_string(f));
  // e um COMUM que nao sai ali (o Treecko e de Hoenn, e o bicho esta em
  // Kanto): o caminho da cacada dos comuns e outro, e tambem tem de olhar
  CHECK_MSG(q.wildWeight(252, 12) == 0, "premissa: o Treecko nao sai no mato de Kanto");
  q.markSeen(252);
  CHECK_MSG(!q.podeCacar(252, 12), "o Treecko de Hoenn ganhou cacada em Kanto");
  double t = fracaoCacada72(252, 252);
  CHECK_MSG(t == 0.0, "o Treecko cacado saiu no mato de Kanto: " + std::to_string(t));
}

TEST(vistos72, capturar_o_cacado_acaba_a_cacada_e_outro_nao) {
  Pet p;
  makePet(p, 4);
  p.markSeen(16);
  p.setCaca(16);
  CHECK_EQ(p.cacaDex, (int16_t)16);
  p.catchMon(19, 5, false, 0, 0);
  CHECK_MSG(p.cacaDex == 16, "capturar OUTRO acabou com a cacada");
  p.catchMon(16, 5, false, 0, 0);
  CHECK_MSG(p.cacaDex == 0, "capturar o cacado nao acabou a cacada");
}

// O passeio da aventura sorteia com pickWildSpecies: a cacada e so do
// mato (o dono escolheu). Com a mesma semente, a sequencia tem de ser a
// MESMA com e sem cacada ligada.
TEST(vistos72, a_cacada_nao_mexe_no_sorteio_da_aventura) {
  std::vector<int16_t> sem, com;
  for (int k = 0; k < 2; k++) {
    randomSeed(5);
    Pet p;
    makePet(p, 4);
    p.markSeen(16);
    if (k) p.setCaca(16);
    for (int i = 0; i < 500; i++) (k ? com : sem).push_back(p.pickWildSpecies(12));
  }
  CHECK_MSG(sem == com, "a cacada mudou o sorteio da aventura");
}

// O teto antigo somava por ordem de dex e cortava: so 6 dos 103 tinham
// chance. Com todas as regioes liberadas, TODOS tem de sair alguma vez.
TEST(vistos72, todos_os_lendarios_liberados_tem_chance_no_mato) {
  randomSeed(7272);
  Pet p;
  makePet(p, 4);
  p.region = DEX_REGIONS - 1;
  p.badges = GYM_COUNT;
  static uint16_t vezes[DEX_COUNT + 1];
  memset(vezes, 0, sizeof(vezes));
  for (long i = 0; i < 600000; i++) {
    int16_t d = p.rollLegendary(13);
    if (d > 0) vezes[d]++;
  }
  int liberados = 0, nunca = 0;
  std::string quem;
  for (int16_t d = 1; d <= DEX_COUNT; d++) {
    if (!p.lendarioPodeSair(d)) continue;
    liberados++;
    if (!vezes[d]) { nunca++; if (quem.size() < 80) quem += std::string(DEX_TBL[d].name) + " "; }
  }
  CHECK_MSG(liberados > 90, "premissa: com tudo liberado sao uns 100: " + std::to_string(liberados));
  CHECK_MSG(nunca == 0, std::to_string(nunca) + " lendarios nunca sairam: " + quem);
}

// Quantas buscas trazem o lendario `alvo`, cacando ele, com `insignias`.
static double fracaoLendaria72(int16_t alvo, uint8_t insignias) {
  randomSeed(1072);
  Pet p;
  makePet(p, 4);
  setStats(p, 90, 90, 90, 90);
  p.badges = insignias;
  p.markSeen(alvo);
  p.setCaca(alvo);
  int saiu = 0, n = 0;
  for (int i = 0; i < 6000; i++) {
    p.wildDex = 0;
    p.wildTtl = 0;
    p.huntUntil = 0;
    if (!p.hunt(12, 1000)) continue;
    saiu++;
    if (p.wildDex == alvo) n++;
  }
  return saiu ? (double)n / saiu : -1.0;
}

TEST(vistos72, cacar_um_lendario_traz_ele_uma_de_cada_dez_e_so_com_as_insignias) {
  Pet q;
  makePet(q, 4);
  q.badges = GYM_COUNT;
  q.markSeen(144);
  CHECK_MSG(q.podeCacar(144, 12), "com a regiao feita o Articuno visto tem de poder ser cacado");
  double com = fracaoLendaria72(144, GYM_COUNT);
  CHECK_MSG(com > 0.08 && com < 0.14,
            "cacando o Articuno ele nao sai 1 em 10: " + std::to_string(com));
  double sem = fracaoLendaria72(144, 0);
  CHECK_MSG(sem == 0.0, "sem as insignias o Articuno cacado saiu: " + std::to_string(sem));
}


// Leva 81: um prazo VELHO nao reacende. O coracao da ultima caricia fica
// guardado; com a placa ligada direto, 24,8 dias depois a resta de 32 bits
// dava a volta e o coracao (e a comida, a evolucao...) acendia sozinho por
// mais 24,8 dias, tapando o balao do pedido. E o relogio dos testes, que
// volta a 0 antes de cada um, fazia o mesmo: era a falha do teste do globo.
TEST(relogio81, um_prazo_velho_nao_reacende) {
  Pet p;
  makePet(p, 4);
  mockAdvanceMillis(30000000UL);   // 8 h ligado
  p.caress();
  CHECK_MSG(p.showHeart(), "premissa: o carinho acende o coracao");
  mockAdvanceMillis(HEART_MS + 10);
  CHECK_MSG(!p.showHeart(), "premissa: o coracao apaga sozinho");

  // a placa ligada direto: 2^31 ms depois a conta da a volta
  mockAdvanceMillis(0x80000000UL);
  CHECK_MSG(!p.showHeart(), "24,8 dias depois o coracao velho acendeu sozinho");

  // o relogio que volta para tras (o dos testes): o prazo fica 8 h no futuro
  mockSetMillis(0);
  CHECK_MSG(!p.showHeart(), "com o relogio de volta a 0 o coracao velho acendeu");

  // e um prazo de verdade continua valendo depois disso tudo
  p.caress();
  CHECK_MSG(p.showHeart(), "um carinho novo deixou de acender o coracao");
}
