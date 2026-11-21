#include "../../imu.h"
// Implementacion de los shims + sustitutos de rtcbat.cpp y audio.cpp (que son
// puro driver: I2C, I2S, PMU). Todo lo demas del firmware se compila de verdad.
#include <Arduino.h>
#include <FS.h>
#include <Preferences.h>
#include <SD_MMC.h>
#include <TouchDrvCSTXXX.hpp>
#include <Wire.h>

#include <algorithm>
#include <deque>
#include <filesystem>
#include <map>
#include <string>
#include <sys/stat.h>
#include <vector>

namespace fs = std::filesystem;

#include "../../audio.h"
#include "../../rtcbat.h"

// ---------------------------------------------------------------- reloj
static uint32_t g_ms = 0;
void mockSetMillis(uint32_t ms) { g_ms = ms; }
void mockAdvanceMillis(uint32_t ms) { g_ms += ms; }
uint32_t millis() { return g_ms; }

// ---------------------------------------------------------------- aleatorio
static uint32_t g_seed = 12345;
void mockSeedRandom(uint32_t s) { g_seed = s ? s : 1; }
static uint32_t nextRand() {
  g_seed = g_seed * 1664525u + 1013904223u;
  // Y SE MEZCLA. Un LCG de modulo 2^32 tiene los bits bajos casi quietos
  // (el bit k repite cada 2^(k+1)), asi que devolver `g_seed >> 1` y
  // hacer luego `% N` con N potencia de dos lee justo la parte muerta.
  // Medido: con un multiplo de 256 llamadas entre dos tiradas —— lo que
  // pasa cuando el sorteo recorre la Pokedex —— random(128) devolvia
  // SIEMPRE el mismo numero, y un shiny de 1/128 salia cero veces en
  // 4000 intentos. El test que lo vigilaba pasaba por casualidad.
  //
  // lowbias32: barato y reparte todos los bits.
  uint32_t x = g_seed;
  x ^= x >> 16;
  x *= 0x7feb352du;
  x ^= x >> 15;
  x *= 0x846ca68bu;
  x ^= x >> 16;
  return x >> 1;
}
// Dado forzado: lo usa la bateria de tests para probar un efecto del
// 10% sin depender de la suerte. -1 = no forzado.
static long g_forzado = -1;
void mockForceRandom(long v) { g_forzado = v; }
void mockClearForcedRandom() { g_forzado = -1; }

long random(long howbig) {
  if (g_forzado >= 0) return howbig <= 0 ? 0 : (g_forzado % howbig);
  return howbig <= 0 ? 0 : (long)(nextRand() % (uint32_t)howbig);
}
long random(long howsmall, long howbig) {
  return howbig <= howsmall ? howsmall : howsmall + random(howbig - howsmall);
}

// ---------------------------------------------------------------- serie
SerialSim Serial;
EspSim ESP;
static std::deque<std::string> g_in;
void mockSerialPush(const std::string &l) { g_in.push_back(l); }
// El firmware habla mucho por el puerto serie y en una bateria de
// tests eso tapa los fallos. No afecta al simulador, que lo deja en
// false.
static bool g_callado = false;
void mockSerialSilence(bool q) { g_callado = q; }
bool mockSerialIsSilent() { return g_callado; }
int SerialSim::available() { return g_in.empty() ? 0 : (int)g_in.front().size() + 1; }
String SerialSim::readStringUntil(char) {
  if (g_in.empty()) return String();
  String s(g_in.front());
  g_in.pop_front();
  return s;
}

// ---------------------------------------------------------------- tactil
static int16_t g_tx = 0, g_ty = 0;
static bool g_down = false;
static void (*g_isr)() = nullptr;

void attachInterrupt(int, void (*fn)(), int) { g_isr = fn; }
void detachInterrupt(int) { g_isr = nullptr; }

// El CST9217 real pulsa INT mientras hay datos; aqui se replica llamando a la
// ISR en cada vuelta del loop mientras el dedo siga apoyado. Tambien se dispara
// al levantarlo, para que el firmware vea el final del toque.
void mockTouchTick() { if (g_down && g_isr) g_isr(); }
void mockTouchSet(int16_t x, int16_t y, bool down) {
  g_tx = x; g_ty = y; g_down = down;
  if (g_isr) g_isr();
}
uint8_t TouchDrvCST92xx::getPoint(int16_t *x, int16_t *y, uint8_t) {
  if (!g_down) return 0;
  if (x) *x = g_tx;
  if (y) *y = g_ty;
  return 1;
}

// La "SD" se mudo a shim/fs.cpp: la bateria de tests la compila tambien,
// para poder correr sdmon.cpp de verdad contra la tarjeta.

// ---------------------------------------------------------------- RTC / bateria
// El RTC del simulador avanza con el reloj falso, partiendo de una epoca que
// se puede fijar: asi se pueden ver los cielos de amanecer/noche sin esperar.
static uint32_t g_epoch0 = 1758700000;  // ~2025-09-24 12:00 UTC
static uint32_t g_ms0 = 0;
void mockSetEpoch(uint32_t e) { g_epoch0 = e; g_ms0 = millis(); }
bool rtcBegin() { return true; }
uint32_t rtcEpoch() { return g_epoch0 + (millis() - g_ms0) / 1000; }
void rtcSetEpoch(uint32_t e) { mockSetEpoch(e); }

bool batBegin() { return true; }
void pmuEnablePanel() {}
int batPercent() { return 77; }
bool batCharging() { return false; }
int batMillivolts() { return 3920; }
bool usbPresent() { return true; }
void pwrSetup() {}
bool pwrShortPressed() { return false; }
bool pwrKeyHeld() { return false; }
bool movePlay(uint16_t mv) { printf("[golpe] #%u\n", (unsigned)mv); return true; }
void audioMoveTable(uint16_t, const char *(*)(uint16_t)) {}
int8_t audioMoveFirma() { return -1; }
void pwrOff() {}

// ---------------------------------------------------------------- Wire / NVS
TwoWire Wire;

static std::map<std::string, std::vector<uint8_t>> g_nvs;
const char *mockSdRootGet();
// Zerar a NVS e partida nova: o /save do SD falso vai junto, senao o boot do
// teste seguinte acharia o estado do anterior "mais novo" e o carregaria.
void mockNvsReset() {
  g_nvs.clear();
  std::error_code ec;
  std::filesystem::remove_all(std::string(mockSdRootGet()) + "/save", ec);
}

// La NVS a un FICHERO, para que el simulador se acuerde entre una
// ejecucion y la siguiente.
//
// Sin esto, en el PC cada arranque empieza por el profesor Oak: la
// "memoria" vive en este mapa y el mapa muere con el proceso. En la
// placa no pasa, porque ahi Preferences es flash de verdad. Pero
// significaba que en el PC NO se podian probar a mano las cosas que
// dependen de que algo quede guardado: la intro que solo sale una vez,
// la racha de dias, el tiempo que paso con el aparato apagado.
//
// El formato es a proposito tonto: clave, largo, bytes. No es la NVS
// de verdad y no tiene por que parecerse — solo tiene que ir y venir.
void mockNvsSave(const char *path) {
  FILE *f = fopen(path, "wb");
  if (!f) return;
  for (const auto &kv : g_nvs) {
    uint32_t nk = (uint32_t)kv.first.size(), nv = (uint32_t)kv.second.size();
    fwrite(&nk, 4, 1, f);
    fwrite(kv.first.data(), 1, nk, f);
    fwrite(&nv, 4, 1, f);
    if (nv) fwrite(kv.second.data(), 1, nv, f);
  }
  fclose(f);
}

bool mockNvsLoad(const char *path) {
  FILE *f = fopen(path, "rb");
  if (!f) return false;
  g_nvs.clear();
  uint32_t nk = 0, nv = 0;
  while (fread(&nk, 4, 1, f) == 1) {
    // Un fichero a medio escribir no puede dejar el simulador en un
    // estado raro: se corta y se sigue con lo que haya entrado.
    if (nk > 4096) break;
    std::string k(nk, '\0');
    if (nk && fread(&k[0], 1, nk, f) != nk) break;
    if (fread(&nv, 4, 1, f) != 1 || nv > (1u << 20)) break;
    std::vector<uint8_t> v(nv);
    if (nv && fread(v.data(), 1, nv, f) != nv) break;
    g_nvs[k] = std::move(v);
  }
  fclose(f);
  return true;
}
bool Preferences::begin(const char *name, bool readOnly) {
  ns_ = name ? name : "";
  ro_ = readOnly;
  open_ = true;
  return true;
}
bool Preferences::clear() {
  if (!open_ || ro_) return false;
  for (auto it = g_nvs.begin(); it != g_nvs.end();)
    it = (it->first.rfind(ns_ + "/", 0) == 0) ? g_nvs.erase(it) : ++it;
  return true;
}
bool Preferences::isKey(const char *k) { return g_nvs.count(ns_ + "/" + k) > 0; }
bool Preferences::remove(const char *k) {
  return !ro_ && g_nvs.erase(ns_ + "/" + k) > 0;
}
// A NVS cheia: recusa tudo, como a da placa em Hoenn (2026-11-08).
static bool g_nvsRecusa = false;
void mockNvsRecusa(bool r) { g_nvsRecusa = r; }
size_t Preferences::put(const char *k, const void *v, size_t n) {
  if (!open_ || ro_ || g_nvsRecusa) return 0;
  auto &b = g_nvs[ns_ + "/" + k];
  b.assign((const uint8_t *)v, (const uint8_t *)v + n);
  return n;
}
size_t Preferences::putString(const char *k, const char *v) {
  return put(k, v, v ? strlen(v) + 1 : 1);
}
size_t Preferences::getBytes(const char *k, void *buf, size_t maxLen) {
  auto it = g_nvs.find(ns_ + "/" + k);
  if (it == g_nvs.end()) return 0;
  size_t n = it->second.size() < maxLen ? it->second.size() : maxLen;
  memcpy(buf, it->second.data(), n);
  return n;
}
size_t Preferences::getBytesLength(const char *k) {
  auto it = g_nvs.find(ns_ + "/" + k);
  return it == g_nvs.end() ? 0 : it->second.size();
}
size_t Preferences::getString(const char *k, char *buf, size_t maxLen) {
  if (!maxLen) return 0;
  size_t n = getBytes(k, buf, maxLen);
  buf[n ? (n < maxLen ? n - 1 : maxLen - 1) : 0] = '\0';
  return n;
}

// ---------------------------------------------------------------- audio
// Sin I2S: se registra el efecto para poder comprobar en el log que suena lo
// que toca (por ejemplo que comer dispara SFX_EAT).
static bool g_audio = true;
void audioBegin() {}
// El ACELEROMETRO. imu.cpp habla por I2C con el QMI8658, asi que en el PC
// no existe: aqui se contesta "no hay sensor" y el juego sigue igual, que
// es exactamente lo que pasa en una placa donde el chip no responde.
//
// Los GESTOS si se prueban, pero en su propia bateria (test_gestos.cpp):
// LectorGestos no toca hardware y se le dan muestras a mano. La division
// es la misma que con el audio y el RTC.
// El lector USB es solo de la placa: aqui la "tarjeta" es una carpeta.
bool usbMscBegin() { return false; }
bool usbMscPrestar(bool) { return false; }
bool usbMscPrestada() { return false; }
uint32_t usbMscEscritos() { return 0; }
bool usbMscExpulsada() { return false; }
bool usbMscSerialCommand(const String &) { return false; }

bool imuBegin() { return false; }
bool imuPresente() { return false; }
void imuTick() {}
Gestos imuGestos() { return Gestos(); }
float imuModulo() { return 1.0f; }

// Cuantas veces ha sonado ALGO. El barrido de toques de test_ui.cpp lo
// mira para saber si un toque acerto a un blanco aunque no cambie nada
// (el pitido de "no se puede" tambien es una respuesta).
static void apunta(const char *k, const char *v);   // definida mas abajo
static uint32_t g_sonidos = 0;
uint32_t mockSonidos() { return g_sonidos; }

void sfxPlay(uint8_t id) {
  g_sonidos++;
  // Esta tabla TIENE que crecer con el enum de audio.h. No lo hizo al
  // anadir los avisos y los golpes, y el simulador se cayo con un
  // SIGSEGV dentro de printf: id valia 12 y aqui solo habia 10 nombres,
  // asi que N[id] era un puntero cualquiera y strlen lo siguio. El
  // static_assert de abajo es para que la proxima vez no compile en vez
  // de reventar en marcha.
  static const char *N[] = {"TAP", "EAT", "PLAY", "HEART", "HATCH",
                            "EVOLVE", "MEDAL", "DENY", "BYE", "LEVEL",
                            "WILD", "TRAINER",
                            "HIT_WEAK", "HIT", "HIT_SUPER",
                            "BALL_THROW", "BALL_HIT", "BALL_DROP",
                            "BALL_SHAKE", "BALL_CLICK", "BALL_OPEN"};
  static_assert(sizeof(N) / sizeof(N[0]) == SFX_COUNT,
                "falta el nombre de algun efecto en el shim");
  if (g_audio && id < SFX_COUNT) {
    printf("[sfx] %s\n", N[id]);
    apunta("sfx", N[id]);
  }
}
static uint8_t g_vol = AUDIO_VOL_MAX;
void audioSetVolume(uint8_t v) {
  g_vol = v > AUDIO_VOL_MAX ? AUDIO_VOL_MAX : v;
  g_audio = g_vol > 0;
  printf("[vol] %u\n", (unsigned)g_vol);
}
uint8_t audioVolume() { return g_vol; }
void audioSetEnabled(bool on) { audioSetVolume(on ? AUDIO_VOL_MAX : 0); }
bool audioEnabled() { return g_audio; }
void audioSetSleeping(bool) {}
// La musica y los gritos tampoco suenan aqui, pero se registran: en el
// simulador el log es lo unico que dice si el combate pidio la pista del
// lider o la del salvaje.
static int g_mus = -1;
static std::string g_audioLog;
const char *mockAudioLog() { return g_audioLog.c_str(); }
void mockAudioLogClear() { g_audioLog.clear(); }
static void apunta(const char *k, const char *v) {
  g_audioLog += k; g_audioLog += ':'; g_audioLog += v; g_audioLog += ' ';
}
void musicPlay(uint8_t id) {
  static const char *N[] = {"WILD","TRAINER","GYM","ELITE","CHAMP","TITLE",
                            "HOME1","HOME2","HOME3","HOME4","HOME5","NIGHT",
                            "EVOLVE"};
  // Se paso de 6 a 12 pistas y esta tabla se quedo en 6: el printf leia un
  // puntero de la nada y el simulador petaba. Es el mismo agujero de tabla
  // contra enum que ya nos mordio con los stages.
  static_assert(sizeof(N) / sizeof(N[0]) == MUS_COUNT,
                "falta el nombre de alguna pista");
  if (!g_audio || id >= MUS_COUNT || g_mus == (int)id) return;
  g_mus = (int)id;
  printf("[bgm] %s\n", N[id]);
  apunta("bgm", N[id]);
}
void musicStop() {
  if (g_mus >= 0) { printf("[bgm] stop\n"); apunta("bgm", "stop"); }
  g_mus = -1;
}
bool jinglePlay(uint8_t id) {
  g_sonidos++;
  static const char *N[] = {"WIN_WILD","WIN_TRAINER","WIN_BIG","CAUGHT",
                            "EVOLVE","BADGE","HATCH","HEAL","MEGA"};
  // El static_assert que le faltaba. La tabla de musica de aqui arriba lo
  // tiene; esta no, y al entrar la fanfarria del centro Pokemon (la octava)
  // el indice 7 leyo un hueco que el compilador rellena con basura: el
  // simulador reventaba dentro de strlen. En la placa no se habria notado
  // —— alli el nombre no existe, se abre /snd/jNN.raw ——, o sea que era un
  // fallo que solo salia en el banco de pruebas, que es el peor sitio.
  static_assert(sizeof(N) / sizeof(N[0]) == JIN_COUNT,
                "falta el nombre de alguna fanfarria");
  // Devuelve si SONO, igual que en la placa: alli es false sin tarjeta, y
  // quien llama cae al pitido sintetizado.
  if (!g_audio || id >= JIN_COUNT) return false;
  printf("[jin] %s\n", N[id]);
  apunta("jin", N[id]);
  return true;
}
bool cryPlay(uint16_t dex) {
  g_sonidos++;
  if (g_audio) {
    printf("[cry] #%u\n", (unsigned)dex);
    apunta("cry", ("#" + std::to_string(dex)).c_str());
  }
  return true;
}
// En el simulador nada suena de verdad, asi que nunca esta ocupado: la
// musica de casa vuelve en cuanto se cierra el combate.
bool audioOneShotBusy() { return false; }
