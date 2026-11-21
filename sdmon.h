#pragma once
#include <Arduino.h>
#include "art_def.h"   // struct Art: lo usa SdFx
#include "dex.h"       // DEX_COUNT, para la comprobacion de abajo

// El numero de dex viaja en int16_t y NO en uint8_t. Estuvo en uint8_t
// desde el principio y aguanto mientras la Pokedex cabia en un byte; con
// Hoenn dejo de caber y el numero daba la vuelta EN SILENCIO: la especie
// 300 pedia /mons/p044.bin, habia un fichero valido al otro lado y salia
// otro bicho. No lo vio el barrido porque el solo comprueba que los
// ficheros esten en la tarjeta, no que el firmware sepa pedirlos.
static_assert(DEX_COUNT <= 32767, "el numero de dex ya no cabe en int16_t");

// Sprite animado TPK1 (formato heredado, camino de respaldo). El proyecto usa
// PMD/TPK2 (PmdMon) para todo; esta ruta queda inactiva si no hay NNN.bin en la SD.
// Los datos indexados viven en PSRAM; la paleta es RGB565.
struct SdMon {
  bool loaded = false;
  uint16_t w = 0, h = 0, frames = 0, frameMs = 100;
  uint8_t scale = 2;       // factor de zoom entero al dibujar
  uint16_t palCount = 0;
  uint16_t pal[256];
  uint8_t *data = nullptr;  // frames * w * h indices (0xFF = transparente)

  bool load(int16_t dexNum, bool shiny = false);
  void unload();
};

// acciones de los sprites PMD (formato TPK2)
enum : uint8_t {
  PMD_IDLE = 0, PMD_WALKL, PMD_WALKR, PMD_SLEEP, PMD_EAT, PMD_HURT,
  PMD_ATTACK, PMD_POSE, PMD_HOP, PMD_NOD, PMD_BREATH, PMD_SIT,
  // Da 12 em diante so nos sprites da aventura (/av/): as outras animacoes
  // que a especie tiver, sem nome, para a cena sortear.
  PMD_NACTS = 48
};

// Cuanto se hace caso al AREA frente al ALTO al dar tamano. Lo comparte
// tools/gen_sizes.py, que calibra la tabla con el MISMO numero; si los dos
// se separan, la tabla pide una cosa y el dibujante hace otra.
// tools/audita_tamanos.py lo comprueba.
#define PMD_B_AREA 0.65f

struct PmdAct {
  uint8_t w = 0, h = 0, frames = 0;
  uint8_t base = 0;  // fila+1 del pixel mas bajo (anclar por los pies, no el lienzo)
  // Huella: las columnas que de verdad apoyan, medidas en el frame que llega
  // mas abajo. El ancho del LIENZO no sirve para la sombra: Aerodactyl ocupa
  // 93 px de lienzo y apoya en 7, asi que una sombra sacada del lienzo le sale
  // trece veces mas ancha que el punto de apoyo y se ve despegada.
  uint8_t footL = 0, footR = 0;
  // Y DONDE APOYA DE VERDAD: las columnas con tinta en la fila que
  // toca el suelo. footL/footR dan el ANCHO de la sombra (el del
  // cuerpo, para que se vea); esto da su CENTRO. Son dos cosas
  // distintas y usar la primera para las dos es lo que dejaba la
  // mancha lejos del pie en 819 de las 1025 especies.
  uint8_t pieL = 0, pieR = 0;
  // El centro de apoyo, en pixeles de sprite desde el borde.
  float centroPie() const {
    return (pieR >= pieL) ? (pieL + pieR) * 0.5f
                          : (footL + footR) * 0.5f;
  }
  // Fila mas ALTA con tinta, en cualquier frame. Con `base` (la mas baja)
  // da el alto del CUERPO, que es lo unico que el ojo lee.
  //
  // El LIENZO no sirve para dar tamano, y costo cinco intentos aprenderlo.
  // El sprite de Pidgeotto mide 116 px de lienzo pero esta dibujado VOLANDO:
  // solo el 30% es tinta, el resto son los huecos entre las alas abiertas,
  // que cuentan en el lienzo y no se ven. Igualando lienzos, su CUERPO sale
  // mas pequeno que el de un Pidgey — que es exactamente lo que se reporto
  // ("pidgey maior que pidgeotto") y lo que ni la altura, ni la caja, ni el
  // area, ni la erosion de la silueta supieron ver. Midiendo tinta la
  // pregunta ni se plantea.
  uint8_t top = 0;
  // E as colunas com tinta em qualquer quadro: a primeira e a seguinte a
  // ultima. Com elas o combate sabe quanto o bicho ocupa de LARGO, que o
  // tamanho por alto/area nao diz: Shiftry e Swanna, de costas, cabiam de
  // alto e entravam na placa de PS com a folha/a asa.
  uint8_t inkL = 0, inkR = 0;
  // Alto del CUERPO en pixeles de sprite. Nunca 0: quien divide por esto no
  // tiene que comprobarlo.
  uint8_t inkH() const {
    return (base > top) ? (uint8_t)(base - top) : (h ? h : 1);
  }
  // Pixeles con tinta del primer frame, y la MAGNITUD que se normaliza
  // para dar tamano. Las dos las calcula sdmon.cpp al cargar.
  //
  // Por que una magnitud y no el alto a secas: normalizando el ALTO, un
  // sprite estirado ocupa menos sitio que uno redondo del mismo alto, y
  // el ojo lee el SITIO. Se reporto con una lista entera de bichos altos
  // y estrechos —— Blacephalon, Kingdra, Pidgeotto, Beedrill, Fearow,
  // Vespiquen —— que "habia que aumentar", y con Snom, redondo, que
  // habia que bajar. Medido: los de la lista tenian un aspecto medio de
  // 1,40 contra 1,10 del plantel, y Wigglytuff (1,82) salia con la MITAD
  // de area dibujada que su Jigglypuff (1,27) pese a ser 18 px mas alto
  // —— o sea que evolucionar encogia, y el auditor no lo veia porque
  // comparaba alturas.
  //
  //   mag = inkH^(1-B) * sqrt(area)^B      con B = PMD_B_AREA
  //
  // B=0 seria normalizar el alto (de donde venimos) y B=1 el area pura,
  // que arregla la forma pero manda 178 especies contra el techo de la
  // pantalla. 0,65 arregla la lista dejando solo 31 en el techo.
  uint16_t inkArea = 0;
  float mag = 1.0f;
  // Duraciones: se APUNTA al blob en PSRAM, no se copian. Antes esto era un
  // `uint16_t ms[24]` y de ahi salia el tope de 24 frames por accion, que no
  // tiene nada que ver con el hardware. Ademas el array fijo ocupaba 48 bytes
  // por accion x 12 acciones x 3 PmdMon = 1,7 KB de RAM siempre reservados,
  // llenos o no. Apuntando no hay tope y no se gasta nada.
  const uint8_t *msRaw = nullptr;
  const uint8_t *data = nullptr;  // frames * w * h en el blob

  // Los u16 del fichero no tienen por que caer alineados a 2 bytes dentro del
  // blob, asi que se montan a mano en vez de castear a uint16_t*.
  uint16_t ms(uint8_t i) const {
    if (!msRaw || i >= frames) return 100;
    uint16_t v = (uint16_t)(msRaw[i * 2] | (msRaw[i * 2 + 1] << 8));
    return v ? v : 100;  // nunca 0: pmdFrameAt() giraria sin avanzar
  }

  // Cuantas filas por encima del suelo esta ESTE frame. `base` es el minimo de
  // toda la accion, asi que solo el frame que llega mas abajo da 0: en los
  // demas el bicho esta en el aire, y en algunos battlers (Aerodactyl, Mew,
  // Gyarados) son 10-26 filas, que escaladas x5 es media pantalla. Sin esto la
  // sombra se queda clavada en el suelo mientras el bicho sube y el resultado
  // es justo lo que parece: que flota.
  uint8_t liftAt(uint8_t fi) const {
    if (!data || fi >= frames || !w || !h) return 0;
    const uint8_t *fr = data + (uint32_t)fi * w * h;
    for (int r = base - 1; r >= 0; r--) {
      for (int c = 0; c < w; c++)
        if (fr[r * w + c] != 0xFF) return (uint8_t)(base - 1 - r);
    }
    return base;
  }
};

// sprite PMD multi-accion cargado de la SD a PSRAM
struct PmdMon {
  bool loaded = false;
  uint16_t palCount = 0;
  uint16_t pal[256];
  uint8_t *blob = nullptr;
  PmdAct acts[PMD_NACTS];

  // `hembra` = usar el dibujo propio de la hembra, si esa especie lo
  // trae (98 lo traen; lo dice hasFemale() de female.h). Si no lo trae o
  // si el fichero falla, cae al normal: una hembra sin dibujo propio se
  // dibuja como el macho, que es lo que hace el juego.
  bool load(int16_t dexNum, bool shiny = false, bool hembra = false);
  // Una MEGA y una FORMA no tienen numero de dex: se piden por su fila
  // de MEGA_TBL / FORM_TBL, que es como se llaman sus ficheros en la
  // tarjeta (mNNN.bin y vNNN.bin).
  // `shiny`: tenta o msNNN.bin; sem ele, cai no mNNN.bin normal.
  bool loadMega(int16_t fila, bool shiny = false);
  bool loadForm(int16_t fila);
  // Una forma de COMBATE (Castform al sol...): /mons/c<dex>_<forma>.bin,
  // por dex y forma y nunca por fila. Sin shiny propio, cae al normal.
  bool loadBatalha(int16_t dex, uint8_t forma, bool shiny);
  // Os da aventura (/av/, tools/pack_aventura.py). false = essa nao tem la, e
  // quem chama cai no sprite padrao.
  bool loadAv(int16_t dex, bool shiny);
  bool loadAvMega(int16_t base, uint8_t forma);
  void unload();
  bool has(uint8_t a) const { return loaded && a < PMD_NACTS && acts[a].frames > 0; }

 private:
  // `extra` < 0: un bicho normal. Si no, la fila de la tabla y `pref`
  // dice de cual ('m' megas, 'v' formas).
  // `rota`: abre esse arquivo, sem montar o nome nem cair em outro.
  bool loadFile(int16_t dexNum, bool shiny, int16_t extra, char pref = 'm',
                const char *rota = nullptr);
};

// Quem esta em cena na aventura (aventura.ino). Aqui e nao la porque o gerador
// de prototipos do .ino corre antes de qualquer struct que esteja nele.
struct AvAtor {
  PmdMon m;
  int16_t dex = 0;
  bool real = false;           // true = veio de /av/ (olha para a direita)
  float alvoFixo = 0;          // >0: a altura ja vem decidida (a mega sem /av/)
  uint8_t pool[PMD_NACTS];     // as animacoes que ele pode sortear
  uint8_t nPool = 0;
};

// miniaturas de la galeria (thumbs.bin entero en PSRAM)
struct SdThumbs {
  bool loaded = false;
  uint8_t *data = nullptr;
  uint16_t count = 0;
  uint32_t size = 0;  // bytes leidos: acota los offsets del fichero
  bool load();
  void unload();  // libera el blob: recargar tras recibir un thumbs.bin nuevo
  const uint8_t *get(int16_t dex) const;  // blob: w,h,palCount,pal[],idx[]
};
extern SdThumbs thumbs;

// Fondo de combate (/bg/NN.bin, formato TPKB: "TPKB", u16 w, u16 h, RGB565).
// Va en crudo porque el combate repinta el fondo ENTERO en cada fotograma:
// tiene que ser un memcpy por fila desde la PSRAM, no una descompresion. Solo
// hay uno cargado a la vez (434 KB); pedir otro id suelta el anterior.
struct SdBg {
  bool loaded = false;
  int16_t id = -1;
  uint16_t w = 0, h = 0;
  uint16_t *pix = nullptr;

  // `dir` es la carpeta de la tarjeta sin barras: "bg" para los escenarios
  // de combate, "map" para los mapas de region. El formato es el mismo, asi
  // que lo unico que cambia entre los dos es de donde se lee y en que
  // instancia se guarda —— y como cada instancia tiene su propio buffer, el
  // mapa no echa de la PSRAM al fondo del combate ni al reves.
  bool load(uint8_t n, const char *dir = "bg");   // ya cargado ese id: no hace nada
  void unload();
};
extern SdBg bgImg;
// Los mapas de region (/map/NN.bin, el indice ES la region). Aparte de
// bgImg porque se miran desde la pantalla de casa, y hacerlos compartir
// buffer obligaria a releer los 434 KB del escenario cada vez que se cierra
// el mapa. Son 172 KB y solo esta cargado el que se esta mirando.
extern SdBg mapImg;

// La animacion PROPIA de un golpe (/fx/fx.bin, formato TPFX; lo escribe
// tools/gen_art.py, que documenta el formato). Estaban las 396 en flash y
// eran 1,2 MB de los 1,3 de art.h: con Johto dentro y Hoenn por venir, eso
// no cabe. En la tarjeta solo hace falta la del golpe que se acaba de
// lanzar, y son ~2,5 KB de media.
//
// Las de TIPO y las de apoyo siguen en flash (son 23): asi un combate sin
// tarjeta sigue teniendo efectos, solo que los genericos.
#define SDFX_MAX_FRAMES 12
struct SdFx {
  uint16_t move = 0xFFFF;   // el golpe que hay cargado; 0xFFFF = ninguno
  uint8_t n = 0, s = 1;
  Art frames[SDFX_MAX_FRAMES];
  bool has[SDFX_MAX_FRAMES] = { false };   // false = fotograma vacio
  uint8_t *blob = nullptr;

  bool load(uint16_t mv);   // no relee si ya es ese golpe
  void unload();
};
extern SdFx fxMove;

// A animacao DO ANIL de um golpe, inteira (/anim/p_<GOLPE>.bin quando voce
// usa, f_<GOLPE>.bin quando o rival usa; formato TPAN, documentado em
// tools/pack_anim.py, que e quem escreve). Toda a geometria vem resolvida
// do PC: aqui so se le e se aponta para cada imagem e cada quadro.
//
// O arquivo diz o NOME do golpe e o lado, e o que nao casar e recusado:
// nada de indice por linha de tabela, que ja fez o jogo tocar a
// animacao do vizinho quando a tabela cresceu.
#define SDANIM_MAX_Q 160       // a mais longa do Anil tem 95 quadros
#define SDANIM_MAX_IMG 96      // a que mais usa, 30 celulas
#define SDANIM_MAX_B (768UL * 1024)
#define SDANIM_CEL_B 26        // bytes de uma cel no arquivo
struct SdAnim {
  uint16_t move = 0xFFFF;      // 0xFFFF = nenhuma
  bool rival = false;          // f_ (o rival usa) ou p_ (voce usa)
  uint8_t hit = 0;             // hitNum do Anil: 0 o golpe, k o impacto k+1 / a carga
  uint16_t nq = 0;
  uint8_t ni = 0;
  // bit0: o bicho do JOGADOR (embaixo) nao entra nesta animacao; bit1: o do
  // RIVAL. Golpe em si mesmo ou sem alvo: o Anil nao mexe no outro.
  uint8_t livre = 0;
  uint8_t *blob = nullptr;
  uint32_t tam = 0;
  uint32_t img[SDANIM_MAX_IMG];
  uint32_t quad[SDANIM_MAX_Q];

  // `nome` = MOVE_TBL[mv].name; o arquivo tem de dizer o mesmo.
  // `hit` > 0: a variante <GOLPE>.<hit> (impacto seguinte ou carga).
  bool load(uint16_t mv, const char *nome, bool deRival, uint8_t hit = 0);
  void unload();
  const uint8_t *quadro(uint16_t q) const { return (blob && q < nq) ? blob + quad[q] : nullptr; }
  const uint8_t *imagem(uint8_t i) const { return (blob && i < ni) ? blob + img[i] : nullptr; }
};
extern SdAnim animMove;

// O CLIMA (/anim/w_<n>.bin, formato TPCL; quem escreve e documenta e
// tools/pack_clima.py). E a animacao Common:Sun/Rain/Sandstorm/Hail do
// Anil, que la toca em laco por cima do combate enquanto o clima dura.
// Tem o que as dos golpes nao tem: uma FIGURA de frente, em tela cheia,
// que rola (a areia). `n` e o WE_* de battle.h, e o arquivo o repete no
// cabecalho: um w_3.bin copiado por cima do w_1.bin e recusado.
#define SDCLIMA_MAX_Q 96       // a mais longa (o sol) tem 40 quadros
#define SDCLIMA_MAX_IMG 16
#define SDCLIMA_MAX_B (256UL * 1024)
#define SDCLIMA_QUAD_B 14      // o cabecalho de um quadro, antes das cels
struct SdClima {
  uint8_t clima = 0;           // o WE_* carregado; 0 = nenhum
  uint16_t nq = 0;
  uint8_t ni = 0;
  uint8_t *blob = nullptr;
  uint32_t fig = 0;            // onde comeca a figura no blob; 0 = nao tem
  uint32_t img[SDCLIMA_MAX_IMG];
  uint32_t quad[SDCLIMA_MAX_Q];

  bool load(uint8_t qual);
  void unload();
  const uint8_t *quadro(uint16_t q) const { return (blob && q < nq) ? blob + quad[q] : nullptr; }
  const uint8_t *imagem(uint8_t i) const { return (blob && i < ni) ? blob + img[i] : nullptr; }
  const uint8_t *figura() const { return (blob && fig) ? blob + fig : nullptr; }
};
extern SdClima animClima;

// Os RETRATOS de um bicho (/por/por.bin, formato TPPR; quem escreve e
// tools/gen_retratos.py, que documenta o formato). Sao caras de 40x40
// do SpriteCollab, uma por ESTADO: normal, com fome, com sono, sem PS,
// sujo e comemorando.
//
// Vao pro cartao pelo mesmo motivo das animacoes de golpe: 1025
// especies por seis estados sao 7 MB, que em flash nao cabem. E como so
// faz falta o do bicho que esta na tela, le-se um.
//
// A escolha de QUAL cara serve pra cada estado ja foi feita no PC (a
// especifica, senao Sigh, senao Normal): aqui nao se sabe o que e um
// "Sigh", so se pede o estado.
#define SDPOR_MAX 8        // POR_RANURAS sao 6; a folga e caso cresca
// Desenhos pequenos em GRUPOS, num arquivo do cartao: as insignias com
// cor (um grupo de 8 por regiao, Hoenn a Paldea) e as fitas das
// conquistas (um grupo so, de 32). Um grupo carregado por vez.
//
//   "XXXX" (magia), ver 1, primeiro grupo, quantos grupos, por grupo
//   u32 deslocamento[grupo][por grupo]      0 = sem desenho
//   entrada: u8 grupo, u8 indice, w, h, u16 cores, u16 bytes RLE, ...
//
// Cada entrada diz de que grupo e indice e: e a assinatura que impede
// um arquivo de outra versao de desenhar a insignia (ou fita) vizinha.
// Sem cartao, ou arquivo errado, get() devolve nullptr e quem desenha
// tem a sua reserva.
#define SDTIRA_MAX 32
struct SdTira {
  const char *path;
  char magia[5];
  uint8_t grupo = 0xFF;    // carregado; 0xFF = nenhum
  Art arts[SDTIRA_MAX];
  bool tem[SDTIRA_MAX] = { false };
  uint8_t *blob = nullptr;
  uint8_t falhou = 0xFF;   // o grupo que falhou ha pouco, e quando
  uint32_t falhouEm = 0;
  SdTira(const char *p, const char *m) : path(p) { memcpy(magia, m, 5); }
  bool load(uint8_t g);    // nao rele se ja e esse
  void unload();
  const Art *get(uint8_t g, uint8_t i) {
    if (i >= SDTIRA_MAX || !load(g)) return nullptr;
    return tem[i] ? &arts[i] : nullptr;
  }
};
extern SdTira insigniasSd;   // /por/insignias.bin, grupo = regiao
extern SdTira fitasSd;       // /por/fitas.bin, grupo 0, indice = conquista

// Confere /snd/golpes.txt (pack_audio.py) contra a tabela de golpes do
// firmware, FILA A FILA: o MOVE_COUNT declarado e o nome de cada fila.
// true so se tudo bate. Vive aqui e nao no audio.cpp para poder ser
// testada (a bateria compila o sdmon, nao o audio).
bool sdFirmaGolpes(uint16_t n, const char *(*nome)(uint16_t));

struct SdPor {
  uint16_t dex = 0;        // 0 = nenhum carregado
  bool shiny = false;      // qual das duas versoes esta carregada
  uint8_t n = 0;           // quantas caras DISTINTAS traz
  uint8_t ranuras = 0;
  uint8_t mapa[SDPOR_MAX] = { 0 };   // estado -> qual das n
  Art caras[SDPOR_MAX];
  uint8_t *blob = nullptr;

  // Um shiny tem retrato PROPRIO no SpriteCollab (mesma silhueta,
  // outra cor): 1023 das 1025 especies tem o seu. Por isso o
  // arquivo traz dois deslocamentos por especie e aqui se escolhe.
  bool load(uint16_t d, bool sh = false);   // nao rele se ja e esse
  // O retrato de uma FORMA MEGA (/por/m<DEX>_<FORMA>.bin), so as que
  // foram feitas a mao (o SpriteCollab nao tem mega). `base` e `forma`
  // sao os do MEGA_TBL, e o arquivo tem de dizer os dois.
  // `sh`: a versao shiny; se o arquivo nao tiver, falha (quem chama cai
  // no retrato shiny da especie, que ao menos tem a cor certa).
  bool loadMega(uint16_t base, uint8_t forma, bool sh = false);
  uint8_t mega = 0;        // forma mega carregada; 0 = a da especie
  void unload();
  // A cara de um estado, ou nullptr. NUNCA devolve a do vizinho: se o
  // estado passa da conta, nao devolve nada.
  const Art *cara(uint8_t estado) const {
    if (!blob || estado >= ranuras) return nullptr;
    uint8_t k = mapa[estado];
    return k < n ? &caras[k] : nullptr;
  }
};
extern SdPor porMon;
// A FIRMA que /por/por.bin tem que trazer. Quem poe e o sketch ao
// arrancar, igual ao fxGolpesEsperados. Nao e enfeite: o arquivo e
// indexado por numero de especie, e foi exatamente assim que os sons
// dos golpes passaram a tocar o do vizinho em silencio quando a tabela
// cresceu.
extern uint16_t porEspeciesEsperadas;
extern uint8_t porRanurasEsperadas;
// A VERSAO do formato. Um cartao da versao 1 tinha METADE das
// entradas na tabela de deslocamentos: lido como se fosse desta,
// cada bicho pegaria o deslocamento do vizinho.
extern uint8_t porVersionEsperada;

bool sdBegin();                 // monta la SD (SDMMC 1-bit), true si hay tarjeta
bool sdSerialCommand(const String &line);  // PUT/LS por USB; true si la maneja
// Cuantos golpes tiene la tabla embarcada. Lo pone el sketch al
// arrancar y lo usa SdFx::load para rechazar un /fx/fx.bin de otra
// version. No se incluye moves.h aqui a proposito: son ~50 KB de tablas
// y meterlas en esta unidad de compilacion las duplicaria en flash.
extern uint16_t fxGolpesEsperados;

extern bool sdReady;
// La tarjeta esta PRESTADA al PC (modo USB: el ordenador la ve como un
// lector de tarjetas y manda el escribirla). Mientras lo este, el
// firmware no puede tocarla: dos escritores sobre el mismo FAT lo
// rompen, y ni siquiera LEER es seguro, porque el PC puede estar
// reescribiendo la tabla justo debajo.
extern bool sdPrestada;
// La unica pregunta que vale antes de abrir nada en la tarjeta. Hay una
// guarda en tools/auditoria.py que comprueba que ningun SD_MMC.* se cuela
// sin pasar por aqui: "SDINFO" y "LS" ya se habian colado una vez.
inline bool sdUsable() { return sdReady && !sdPrestada; }
extern bool sdDirty;  // true tras recibir archivos: recargar sprite
