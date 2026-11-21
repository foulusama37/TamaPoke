#include "audio.h"
#include "dex.h"        // DEX_COUNT: hasta donde llegan los gritos
#include "pin_config.h"
#include "sdmon.h"      // sdReady: sin tarjeta no hay musica ni gritos
#include <Arduino.h>
#include <Wire.h>
#include <ESP_I2S.h>
#include <Preferences.h>
#include <FS.h>
#include <SD_MMC.h>

// ---------------------------------------------------------------------------
// Audio del TamaPoke: códec ES8311 (DAC -> amplificador PA -> altavoz) por I2S.
// Init del ES8311 portado del driver oficial de Espressif (esp-bsp), fijado a
// MCLK=4.096MHz (256*fs), 16kHz, 16-bit, esclavo I2S.
//
// Hay DOS fuentes de sonido y una tarea que las mezcla:
//   - pitidos de onda cuadrada sintetizados al vuelo (los de siempre), que
//     no necesitan tarjeta y por eso son los unicos que suenan sin ella;
//   - PCM de la SD (musica en bucle, fanfarrias y los gritos de los 151),
//     cargado a la PSRAM por tools/pack_audio.py.
// La tarea suma las dos y satura. Antes escribia al I2S dentro del propio
// generador de tonos, que bloqueaba y hacia imposible solapar nada.
// ---------------------------------------------------------------------------

#define ES8311_ADDR 0x18
#define SAMPLE_RATE 16000

static I2SClass i2s;
static bool gReady = false;
static bool gOn = true;
static bool gSleeping = false;
static QueueHandle_t gQ = nullptr;

// El NS4150B tarda bastante mas de 8 ms en estabilizarse tras cada apagado, asi
// que encenderlo justo antes de cada efecto se comia los cortos: el jingle de
// arranque (440 ms) se oia y los pitidos de 35 ms no. Se deja encendido mientras
// haya sonido y la mascota este despierta, como en el ejemplo verificado de
// Waveshare, y solo se apaga al dormir o al silenciar.
static void updateAmplifierPower() {
  // gReady evita encenderlo en una placa donde el codec no arranco: ahi el
  // amplificador solo aportaria siseo, porque no va a sonar nada.
  digitalWrite(PA, (gReady && gOn && !gSleeping) ? HIGH : LOW);
}

// ---- I2C del códec ----
static bool esW(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(ES8311_ADDR);
  Wire.write(reg);
  Wire.write(val);
  return Wire.endTransmission() == 0;
}

// ---- volumen ----
//
// Va por HARDWARE, en el registro 0x32 del ES8311 (volumen del DAC, pasos
// de medio dB con 0xBF ~ 0 dB). Escalar las muestras a mano seria tirar
// bits: a volumen bajo un grito de 16 bits se quedaria en 12.
static const uint8_t VOL_REG[AUDIO_VOL_MAX + 1] = { 0x00, 0xA0, 0xB0, 0xBF };
static uint8_t gVol = AUDIO_VOL_MAX;

static void applyVolume() {
  if (gReady) esW(0x32, VOL_REG[gVol <= AUDIO_VOL_MAX ? gVol : AUDIO_VOL_MAX]);
}

static uint8_t esR(uint8_t reg) {
  Wire.beginTransmission(ES8311_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom(ES8311_ADDR, 1);
  return Wire.available() ? Wire.read() : 0;
}

// Secuencia de init VERIFICADA EN ESTA PLACA (proyecto PlaneRadar2.0, misma
// Waveshare 1.75). Clave: reloj DERIVADO DEL BCLK (reg01=0xBF, sin MCLK externo)
// y referencia interna que alimenta el DAC (reg44=0x58); sin esos dos el codec
// respondia por I2C pero no salia audio. 16kHz, 16-bit, esclavo I2S.
static bool es8311Init() {
  Wire.beginTransmission(ES8311_ADDR);
  if (Wire.endTransmission() != 0) return false;

  // open()
  esW(0x0D, 0xFA); esW(0x44, 0x08); esW(0x44, 0x08);  // power up + quirk de 1a escritura
  esW(0x01, 0x30); esW(0x02, 0x00); esW(0x03, 0x10); esW(0x16, 0x24);
  esW(0x04, 0x10); esW(0x05, 0x00); esW(0x0B, 0x00); esW(0x0C, 0x00);
  esW(0x10, 0x1F); esW(0x11, 0x7F);
  esW(0x00, 0x80); esW(0x00, 0x80);                   // reset clock, esclavo
  esW(0x01, 0xBF);                                    // clk src = BCLK (sin MCLK externo)
  { uint8_t r = esR(0x06); r &= ~0x20; esW(0x06, r); }  // SCLK no invertido
  esW(0x13, 0x10); esW(0x1B, 0x0A); esW(0x1C, 0x6A);
  esW(0x44, 0x58);                                    // referencia interna -> alimenta el DAC

  // config_sample(): BCLK*8 = DIG_MCLK
  esW(0x02, 0x18); esW(0x05, 0x00); esW(0x03, 0x10); esW(0x04, 0x20);
  { uint8_t r = esR(0x07); r &= 0xC0; esW(0x07, r); }
  esW(0x08, 0xFF);
  { uint8_t r = esR(0x06); r &= 0xE0; r |= 0x03; esW(0x06, r); }  // bclk_div=4

  // formato I2S 16-bit
  esW(0x09, 0x0C); esW(0x0A, 0x0C);

  // start() DAC esclavo
  esW(0x00, 0x80); esW(0x01, 0xBF); esW(0x09, 0x0C); esW(0x0A, 0x0C);
  esW(0x17, 0xBF); esW(0x0E, 0x02); esW(0x12, 0x00); esW(0x14, 0x1A);
  esW(0x0D, 0x01); esW(0x15, 0x40); esW(0x37, 0x08); esW(0x45, 0x00);

  // volumen + unmute
  esW(0x32, 0xBF);                                    // volumen DAC ~0 dB
  { uint8_t r = esR(0x31); r &= 0x9F; esW(0x31, r); }  // unmute
  return true;
}

// ---- sintetizador de tono cuadrado ----
struct Note { uint16_t f, ms; };

static const Note N_TAP[]    = {{880, 35}};
static const Note N_EAT[]    = {{660, 45}, {0, 12}, {660, 45}};
static const Note N_PLAY[]   = {{784, 45}, {988, 60}};
static const Note N_HEART[]  = {{1047, 55}, {1319, 90}};
static const Note N_HATCH[]  = {{523, 80}, {659, 80}, {784, 110}, {1047, 170}};
static const Note N_EVOLVE[] = {{523, 80}, {659, 80}, {784, 80}, {1047, 90}, {1319, 230}};
static const Note N_MEDAL[]  = {{784, 70}, {0, 25}, {784, 70}, {0, 25}, {1047, 200}};
static const Note N_DENY[]   = {{300, 110}, {200, 170}};
static const Note N_BYE[]    = {{784, 150}, {659, 150}, {523, 280}};
static const Note N_LEVEL[]  = {{784, 70}, {1047, 130}};
// Sin tarjeta siguen siendo pitidos. El del salvaje sube (algo aparecio);
// el del entrenador son dos golpes iguales y secos, como el "!" de los
// juegos, para que no se confundan de oido.
static const Note N_WILD[]   = {{659, 60}, {880, 90}};
static const Note N_TRAIN[]  = {{988, 70}, {0, 40}, {988, 140}};
// Los golpes, sin tarjeta: grave y corto el flojo, seco el normal, y
// dos notas subiendo el eficaz. Se distinguen de oido, que es para lo
// que estan.
static const Note N_HITW[]   = {{196, 70}};
static const Note N_HIT[]    = {{294, 60}, {220, 50}};
static const Note N_HITS[]   = {{392, 50}, {523, 50}, {659, 90}};
// La pokebola, sin tarjeta: un silbido que sube al tirarla, un golpe al
// abrirse, dos botes, un toc por vaiven, el clic y el "se abrio".
static const Note N_BTHROW[] = {{523, 40}, {784, 40}, {1047, 60}};
static const Note N_BHIT[]   = {{330, 50}, {165, 70}};
static const Note N_BDROP[]  = {{196, 50}, {0, 60}, {147, 50}};
static const Note N_BSHAKE[] = {{440, 35}};
static const Note N_BCLICK[] = {{1319, 30}, {0, 20}, {1568, 60}};
static const Note N_BOPEN[]  = {{392, 60}, {262, 110}};

struct SfxDef { const Note *n; uint8_t len; };
static const SfxDef SFX[SFX_COUNT] = {
  {N_TAP, 1}, {N_EAT, 3}, {N_PLAY, 2}, {N_HEART, 2}, {N_HATCH, 4},
  {N_EVOLVE, 5}, {N_MEDAL, 5}, {N_DENY, 2}, {N_BYE, 3}, {N_LEVEL, 2},
  {N_WILD, 2}, {N_TRAIN, 3},
  {N_HITW, 1}, {N_HIT, 2}, {N_HITS, 3},
  {N_BTHROW, 3}, {N_BHIT, 2}, {N_BDROP, 3}, {N_BSHAKE, 1}, {N_BCLICK, 3},
  {N_BOPEN, 2},
};
static_assert(sizeof(SFX) / sizeof(SFX[0]) == SFX_COUNT,
              "falta el pitido de respaldo de algun efecto");

#define BLOQUE 256
static int16_t buf[BLOQUE * 2];  // estéreo intercalado (L=R)

// ---- pitido cuadrado, ahora como GENERADOR ----
//
// Antes playTone() escribia al I2S el mismo, en bucle y bloqueando. Ya no
// puede: con musica de fondo hay que SUMAR las dos cosas muestra a muestra,
// asi que el pitido pasa a dar una muestra cada vez y quien escribe al I2S
// es el mezclador.
static const SfxDef *gSfx = nullptr;
static uint8_t gSfxNota = 0;
static int gSfxHechas = 0, gSfxTotal = 0, gSfxFase = 0, gSfxMedio = 0;
static bool gSfxAlta = true;

static void sfxNota(uint8_t i) {
  const Note &n = gSfx->n[i];
  gSfxTotal = SAMPLE_RATE * n.ms / 1000;
  gSfxMedio = n.f ? (SAMPLE_RATE / (2 * n.f)) : 0;
  gSfxHechas = 0;
  gSfxFase = 0;
  gSfxAlta = true;
}


static int16_t sfxNext() {
  if (!gSfx) return 0;
  if (gSfxHechas >= gSfxTotal) {
    if (++gSfxNota >= gSfx->len) { gSfx = nullptr; return 0; }
    sfxNota(gSfxNota);
  }
  int16_t s = 0;
  if (gSfxMedio) {
    const int16_t amp = 5000;
    s = gSfxAlta ? amp : -amp;
    // rampas de ataque y caida: sin ellas cada nota chasquea
    if (gSfxHechas < 64) s = (int16_t)(s * gSfxHechas / 64);
    else if (gSfxHechas > gSfxTotal - 96)
      s = (int16_t)(s * (gSfxTotal - gSfxHechas) / 96);
    if (++gSfxFase >= gSfxMedio) { gSfxFase = 0; gSfxAlta = !gSfxAlta; }
  }
  gSfxHechas++;
  return s;
}

// ---- muestras de la SD (musica, fanfarrias, gritos) ----
struct Pcm {
  int16_t *s = nullptr;
  uint32_t n = 0, pos = 0;
  bool loop = false, on = false;
  int8_t id = -1;         // que esta cargado, para no releerlo
};
static Pcm gMus, gUno;    // fondo en bucle, y el de una vez (fanfarria o grito)

// Los EFECTOS del Anil, ya en memoria. Antes todo efecto era un pitido
// de onda cuadrada sintetizado al vuelo: sonaba, pero no sonaba a
// Pokemon. Ahora si hay muestra se toca la muestra, y si no (sin
// tarjeta, o con una a medio llenar) sigue el pitido -- el aparato tiene
// que hacer ruido sin tarjeta, que es una promesa vieja de este
// proyecto.
//
// Se cargan TODOS al arrancar, desde el hilo principal, y se quedan en
// PSRAM: son 204 KB entre los diez. Cargarlos al vuelo seria leer la SD
// dentro de la tarea de audio, que es justo lo que pcmLoad prohibe (la
// tarjeta tarda decimas y ahi dentro eso es silencio cortado).
static int16_t *gEfeDat[SFX_COUNT] = { nullptr };
static uint32_t gEfeLen[SFX_COUNT] = { 0 };
static Pcm gEfe;          // el que suena ahora; NO es dueno de su buffer
static SemaphoreHandle_t gMix = nullptr;

static void sfxStart(uint8_t id) {
  // Con muestra, suena la muestra. gEfe NO es dueno del buffer: apunta a
  // uno de los que se cargaron al arrancar, asi que aqui no se reserva
  // ni se libera nada. Solo se mueve el cabezal, y con el mezclador
  // parado, porque esto corre en la tarea de audio igual que el.
  if (id < SFX_COUNT && gEfeDat[id]) {
    if (gMix) xSemaphoreTake(gMix, portMAX_DELAY);
    gEfe.s = gEfeDat[id];
    gEfe.n = gEfeLen[id];
    gEfe.pos = 0;
    gEfe.loop = false;
    gEfe.on = true;
    if (gMix) xSemaphoreGive(gMix);
    gSfx = nullptr;        // y el pitido se calla: sonarian los dos
    return;
  }
  gSfx = &SFX[id];
  gSfxNota = 0;
  sfxNota(0);
}

static int16_t pcmNext(Pcm &p) {
  if (!p.on || !p.s || p.pos >= p.n) { p.on = p.loop && p.s; p.pos = 0; return 0; }
  int16_t v = p.s[p.pos++];
  if (p.pos >= p.n) {
    if (p.loop) p.pos = 0;
    else p.on = false;
  }
  return v;
}

// Lee /snd/<nombre> a la PSRAM. Se hace desde el hilo que LLAMA (el bucle
// principal), nunca desde la tarea de audio: la SD tarda decimas y ahi
// dentro eso son decimas de silencio cortado.
static bool pcmLoad(const char *nombre, Pcm &p, int8_t id, bool loop) {
  if (!sdUsable()) return false;
  char ruta[28];
  snprintf(ruta, sizeof(ruta), "/snd/%s", nombre);
  File f = SD_MMC.open(ruta, FILE_READ);
  if (!f) return false;
  uint8_t cab[12];
  if (f.read(cab, 12) != 12 || memcmp(cab, "TPKS", 4) != 0) { f.close(); return false; }
  uint32_t n;
  uint16_t hz;
  memcpy(&n, cab + 4, 4);
  memcpy(&hz, cab + 8, 2);
  // acota: un fichero corrupto no puede pedir una reserva absurda
  if (!n || n > 2UL * 1024 * 1024 || hz != SAMPLE_RATE) { f.close(); return false; }
  int16_t *nuevo = (int16_t *)ps_malloc(n * 2);
  if (!nuevo) { f.close(); return false; }
  uint32_t leido = 0, bytes = n * 2;
  while (leido < bytes) {
    uint32_t t = bytes - leido;
    if (t > 16384) t = 16384;
    if (f.read((uint8_t *)nuevo + leido, t) != (int)t) {
      free(nuevo);
      f.close();
      return false;
    }
    leido += t;
  }
  f.close();

  // El cambio se publica con el mezclador parado: si no, la tarea puede
  // estar leyendo el buffer viejo justo cuando se libera.
  if (gMix) xSemaphoreTake(gMix, portMAX_DELAY);
  int16_t *viejo = p.s;
  p.s = nuevo;
  p.n = n;
  p.pos = 0;
  p.loop = loop;
  p.on = true;
  p.id = id;
  if (gMix) xSemaphoreGive(gMix);
  if (viejo) free(viejo);
  return true;
}

static void audioTask(void *) {
  for (;;) {
    if (!gReady) { vTaskDelay(pdMS_TO_TICKS(50)); continue; }
    uint8_t id;
    if (!gSfx && xQueueReceive(gQ, &id, 0) && id < SFX_COUNT) sfxStart(id);
    if (!gOn || gSleeping) {
      gSfx = nullptr;
      gMus.on = gUno.on = gEfe.on = false;
      vTaskDelay(pdMS_TO_TICKS(30));
      continue;
    }
    // gEfe tambien cuenta: sin el en esta condicion la tarea se iba a
    // dormir con una muestra sonando y no la mezclaba nunca. Un canal
    // nuevo hay que anadirlo en los TRES sitios -- aqui, al silenciar y
    // en la suma -- o calla en silencio.
    if (!gSfx && !gMus.on && !gUno.on && !gEfe.on) { vTaskDelay(pdMS_TO_TICKS(10)); continue; }

    xSemaphoreTake(gMix, portMAX_DELAY);
    for (int i = 0; i < BLOQUE; i++) {
      int32_t s = sfxNext();
      s += pcmNext(gMus);
      s += pcmNext(gUno);
      s += pcmNext(gEfe);
      if (s > 32767) s = 32767;
      else if (s < -32768) s = -32768;
      buf[i * 2] = buf[i * 2 + 1] = (int16_t)s;
    }
    xSemaphoreGive(gMix);
    i2s.write((uint8_t *)buf, BLOQUE * 4);
  }
}

void musicPlay(uint8_t id) {
  if (!gReady || !gOn || id >= MUS_COUNT) return;
  if (gMus.id == (int8_t)id && gMus.s) { gMus.on = true; return; }  // ya es esa
  char n[10];
  snprintf(n, sizeof(n), "m%02u.raw", id);
  pcmLoad(n, gMus, (int8_t)id, true);
}

void musicStop() {
  if (gMix) xSemaphoreTake(gMix, portMAX_DELAY);
  gMus.on = false;
  if (gMix) xSemaphoreGive(gMix);
}

bool jinglePlay(uint8_t id) {
  if (!gReady || !gOn || id >= JIN_COUNT) return false;
  char n[10];
  snprintf(n, sizeof(n), "j%02u.raw", id);
  return pcmLoad(n, gUno, -1, false);
}

bool cryPlay(uint16_t dex) {
  if (!gReady || !gOn || dex < 1 || dex > DEX_COUNT) return false;
  char n[12];
  snprintf(n, sizeof(n), "c%03u.raw", dex);
  return pcmLoad(n, gUno, -1, false);
}

// LA FIRMA DEL PAQUETE DE SONIDO. Los gNNN.raw van por FILA de MOVE_TBL,
// y la tabla esta ordenada por nombre: un golpe nuevo en medio corre los
// numeros de todos los de detras. Paso: entro ION DELUGE en la fila 317 y
// desde ahi cada golpe tocaba el sonido del siguiente. pack_audio.py
// escribe golpes.txt (fila, nombre); aqui se compara FILA A FILA con la
// tabla del firmware, una vez. Si no casa, ningun golpe toca su sonido
// propio —— cae en el generico, que se nota y se arregla recopiando —— en
// vez de tocar el del vecino, que no se nota nunca.
static uint16_t gTablaN = 0;
static const char *(*gTablaNome)(uint16_t) = nullptr;
static int8_t gFirma = -1;

void audioMoveTable(uint16_t count, const char *(*nome)(uint16_t)) {
  gTablaN = count;
  gTablaNome = nome;
  gFirma = -1;
}

int8_t audioMoveFirma() { return gFirma; }

static bool firmaGolpes() {
  if (gFirma >= 0) return gFirma == 1;
  if (!gTablaNome || !gTablaN || !sdUsable()) return false;   // se intenta luego
  bool ok = sdFirmaGolpes(gTablaN, gTablaNome);
  gFirma = ok ? 1 : 0;
  if (!ok)
    Serial.printf("/snd/golpes.txt NAO bate com a tabela (%u golpes): sem som "
                  "proprio de golpe ate recopiar o pacote\n", (unsigned)gTablaN);
  return ok;
}

bool movePlay(uint16_t mv) {
  if (!gReady || !gOn) return false;
  if (!firmaGolpes()) return false;
  char n[12];
  snprintf(n, sizeof(n), "g%03u.raw", (unsigned)mv);
  return pcmLoad(n, gUno, -1, false);
}

bool audioOneShotBusy() { return gUno.on; }

void audioBegin() {
  // I2S primero: arranca el MCLK que necesita el códec para engancharse
  pinMode(PA, OUTPUT);
  digitalWrite(PA, LOW);   // hasta saber si el sonido esta activado

  i2s.setPins(I2S_BCK_IO, I2S_WS_IO, I2S_DO_IO, I2S_DI_IO, I2S_MCK_IO);
  if (!i2s.begin(I2S_MODE_STD, SAMPLE_RATE, I2S_DATA_BIT_WIDTH_16BIT,
                 I2S_SLOT_MODE_STEREO, I2S_STD_SLOT_BOTH)) {
    Serial.println("I2S begin fallo");
    return;
  }
  if (!es8311Init()) { Serial.println("ES8311 no responde (audio off)"); return; }

  Preferences p;
  p.begin("tamapoke", true);
  gOn = p.getBool("snd", true);
  // Partida anterior al volumen: si el sonido estaba puesto, al maximo.
  gVol = p.getUChar("vol", gOn ? AUDIO_VOL_MAX : 0);
  p.end();
  if (gVol > AUDIO_VOL_MAX) gVol = AUDIO_VOL_MAX;
  gOn = (gVol > 0);

  gReady = true;
  applyVolume();
  updateAmplifierPower();
  // Los efectos, a PSRAM, ANTES de crear la tarea de audio: asi nadie
  // esta leyendo los buffers mientras se llenan (no hace falta el mutex
  // porque todavia no hay con quien competir) y el primer sonido que se
  // oye ya es la muestra y no el pitido.
  {
    Pcm tmp;
    uint8_t hay = 0;
    for (uint8_t i = 0; i < SFX_COUNT; i++) {
      char nom[12];
      snprintf(nom, sizeof(nom), "s%02u.raw", (unsigned)i);
      // a cero antes de cada vuelta: pcmLoad libera el buffer anterior
      // del Pcm que le pasas, y los anteriores ya son del array.
      tmp.s = nullptr;
      tmp.on = false;
      if (!pcmLoad(nom, tmp, -1, false)) continue;
      gEfeDat[i] = tmp.s;
      gEfeLen[i] = tmp.n;
      tmp.s = nullptr;   // el dueno pasa a ser el array
      hay++;
    }
    gEfe.on = false;
    Serial.printf("efectos: %u de %u en memoria%s\n", hay,
                  (unsigned)SFX_COUNT,
                  hay ? "" : " (sin tarjeta: pitidos de siempre)");
  }

  gQ = xQueueCreate(8, sizeof(uint8_t));
  gMix = xSemaphoreCreateMutex();
  xTaskCreatePinnedToCore(audioTask, "audio", 4096, nullptr, 1, nullptr, 0);
  sfxPlay(SFX_HATCH);  // jingle de arranque (confirma que suena)
}

void sfxPlay(uint8_t id) {
  if (gReady && gOn && gQ) xQueueSend(gQ, &id, 0);  // descarta si la cola esta llena
}

void audioSetVolume(uint8_t lvl) {
  if (lvl > AUDIO_VOL_MAX) lvl = AUDIO_VOL_MAX;
  gVol = lvl;
  gOn = (lvl > 0);
  applyVolume();
  updateAmplifierPower();
  Preferences p;
  p.begin("tamapoke", false);
  p.putBool("snd", gOn);      // la clave vieja se mantiene: saves antiguos
  p.putUChar("vol", gVol);
  p.end();
}
uint8_t audioVolume() { return gVol; }

void audioSetEnabled(bool on) {
  // Apagar recuerda a que volumen estabas: volver a encender no te devuelve
  // al maximo de golpe.
  static uint8_t ultimo = AUDIO_VOL_MAX;
  if (!on && gVol) ultimo = gVol;
  audioSetVolume(on ? (ultimo ? ultimo : AUDIO_VOL_MAX) : 0);
}
bool audioEnabled() { return gOn; }

void audioSetSleeping(bool sleeping) {
  if (gSleeping == sleeping) return;
  gSleeping = sleeping;
  updateAmplifierPower();  // durmiendo no hay efectos: amp apagado, sin consumo
}
