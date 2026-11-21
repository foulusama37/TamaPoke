#pragma once
#include <stdint.h>

// Un Pokemon GUARDADO: lo justo para reconstruirlo, sin nada de lo que solo
// tiene sentido mientras esta en pantalla.
//
// El bicho ACTIVO no vive aqui: vive en el propio Pet, con sus barras, su
// animacion y su suciedad. Esto es para los que estan en el equipo pero no
// salen, y para los del banco. Al cambiar de activo se vuelca el Pet a su
// hueco y se carga el otro encima.
//
// El orden de los campos NO es casual: de mas ancho a mas estrecho, para que
// el compilador no meta relleno. Son 42 bytes utiles y 44 con la alineacion;
// 6 de equipo + 60 de banco = 2,9 KB, que caben de sobra en los 20 KB de NVS
// como un solo blob (un blob por bicho se comeria la particion en cabeceras).
// Los valores por defecto NO son decoracion: sin ellos los arrays de equipo y
// banco arrancan con basura de la pila y `used()` da true en huecos vacios —
// un Pet recien creado aparecia con 3 en el equipo y 33 en el banco.
struct StoredMon {
  uint32_t exp = 1;
  int16_t species = 0;    // 0 = hueco vacio
  uint16_t moves[4] = { 0, 0, 0, 0 };
  uint16_t hpLeft = 0;
  uint8_t ppLeft[4] = { 0, 0, 0, 0 };
  // Los IVs y los EVs, SEIS de cada uno. Eran tres, con el especial
  // compartiendo campo con su pareja fisica; asi no habia forma de criar a
  // un bicho especial ni de que Proteina y Calcio hicieran cosas distintas.
  uint8_t geneHp = 100, geneAtk = 100, geneDef = 100;
  uint8_t geneSpa = 100, geneSpd = 100, geneSpe = 100;
  uint8_t trHp = 0, trAtk = 0, trDef = 0;
  uint8_t trSpa = 0, trSpd = 0, trSpe = 0;
  // uint16_t y no uint8_t: el Anil tiene 293 habilidades entre las 1025
  // especies, asi que el indice pasa de 255 en Galar. En un byte no
  // rompia el build — la habilidad 260 se grababa como la 4 y el bicho
  // salia con otra, en silencio, que es el fallo de siempre de este
  // proyecto. Cuesta 4 bytes por hueco (48 -> 52) por el relleno.
  uint16_t ability = 0;
  // La FORMA. 0 = la de siempre; si no, la fila de FORM_TBL dice sus
  // tipos, stats, habilidad y sprite. Un Vulpix de Alola es un Vulpix
  // con form=1, no otra especie, asi que `species` no cambia.
  uint8_t form = 0;
  uint8_t item = 0;       // lo que lleva encima; vuelve a la bolsa al soltarlo
  uint8_t bond = 0;       // el vinculo es de CADA UNO: por eso cambiar de
                          // titular tiene precio y el equipo no son seis
                          // piezas intercambiables
  uint8_t flags = 0;
  char nick[12] = { 0 };
  // Em QUE BOLA foi capturado (indice de ITEM_TBL). Vai DEPOIS do nick e
  // nao junto dos outros bytes: aqui estavam 2 bytes de alinhamento (50 ->
  // 52), entao o registro NAO cresce e as caixas da SD continuam a ler-se.
  // Num registro velho este byte e lixo do alinhamento: so vale com
  // SM_BALL_SET em `flags` (ver ballDe()).
  uint8_t ball = 0;

  bool used() const { return species > 0; }
};
static_assert(sizeof(StoredMon) == 52,
              "StoredMon mudou de tamanho: as caixas da SD e o time na NVS "
              "seriam lidos desalinhados");

// SM_FEMALE va en `flags` y no en un campo propio porque el byte ya
// estaba y le sobraban seis bits: StoredMon esta en 52 y cada byte que
// crece son 52 mas en la NVS.
//
// SM_GENDER_SET distingue "macho" de "todavia sin sortear": sin el, una
// partida vieja (o un bicho sin sexo) no se puede diferenciar de uno que
// salio macho, y se le volveria a tirar el dado en cada carga.
enum : uint8_t {
  SM_SHINY = 1 << 0,
  SM_MOVES_PICKED = 1 << 1,
  SM_FEMALE = 1 << 2,
  SM_GENDER_SET = 1 << 3,
  SM_BALL_SET = 1 << 4,   // `ball` foi escrito de verdade (nao e lixo velho)
};

// A bola de um bicho guardado, ou 0 se nao se sabe (registro de antes das
// bolas, ovo, inicial): quem desenha usa a Poke Ball nesse caso.
static inline uint8_t ballDe(const StoredMon &m) {
  return (m.flags & SM_BALL_SET) ? m.ball : 0;
}

#define TEAM_SLOTS 6
// UNA CAJA, del tamano de las de los juegos. Antes eran 60 y era el banco
// entero, porque vivia en una sola entrada de la NVS y ahi no cabe mas.
// Ahora las cajas estan en la SD y solo la ABIERTA ocupa RAM.
#define BANK_SLOTS 30
#define BOX_COUNT 32          // 32 x 30 = 960 huecos
// Cabecera de /boxes/boxNN.bin. Lleva el sizeof de StoredMon a proposito:
// esta estructura YA crecio una vez (48 -> 52, cuando la habilidad paso a
// dos bytes) y un fichero de registros de tamano fijo que cambia de
// tamano se lee entero desplazado, sin que nada se queje. Si no cuadra,
// la caja se ignora en vez de leerse torcida.
#define BOX_MAGIC 0x58425054UL   // 'TPBX'
#define BOX_VERSION 1
struct BoxHeader {
  uint32_t magic;
  uint8_t version;
  uint8_t monSize;     // sizeof(StoredMon) con el que se escribio
  uint8_t slots;       // BANK_SLOTS con el que se escribio
  uint8_t box;         // que caja es, para cazar un fichero renombrado
  uint32_t reservado[2];
};
// EV_* (indice de IV/EV) vive en btstate.h: la tabla de objetos tambien lo
// nombra y items.h se genera antes que esto.
