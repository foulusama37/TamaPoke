// Minimo de Arduino para compilar el firmware en el PC.
// No intenta ser completo: solo lo que TamaPoke.ino, pet.cpp, i18n.cpp y
// sdmon.cpp usan de verdad. El reloj es falso y controlable, que es lo que
// permite simular horas de juego en milisegundos.
#pragma once
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <cmath>
#include <string>

#ifndef PI
#define PI 3.1415926535897932384626433832795
#endif
#define PROGMEM
#define IRAM_ATTR
#define pgm_read_byte(a) (*(const uint8_t *)(a))
#define pgm_read_word(a) (*(const uint16_t *)(a))
#define pgm_read_pointer(a) (*(void *const *)(a))
#define F(x) (x)

// GPIO e interrupciones: en el simulador no hay pines, pero el firmware los
// configura en setup() y el tactil se engancha a una ISR.
#define INPUT 0
#define OUTPUT 1
#define INPUT_PULLUP 2
#define LOW 0
#define HIGH 1
#define RISING 1
#define FALLING 2
#define CHANGE 3
inline void pinMode(int, int) {}
inline void digitalWrite(int, int) {}
inline int digitalRead(int) { return 1; }
inline int digitalPinToInterrupt(int p) { return p; }
// El firmware solo lee el tactil cuando la ISR del pin INT levanta su bandera,
// asi que la ISR tiene que dispararse de verdad o los toques no llegan nunca.
// Se guarda el handler y el simulador lo llama mientras haya dedo en pantalla.
void attachInterrupt(int pin, void (*fn)(), int mode);
void detachInterrupt(int pin);
void mockTouchTick();  // dispara la ISR si el dedo sigue apoyado

using std::max;
using std::min;

// ---------------------------------------------------------------- reloj falso
void mockSetMillis(uint32_t ms);
void mockAdvanceMillis(uint32_t ms);
// Lo que se pidio sonar (musica, fanfarrias, gritos), en orden, separado
// por espacios: "bgm:EVOLVE cry:#4 jin:EVOLVE". Para probar el ORDEN.
const char *mockAudioLog();
void mockAudioLogClear();
uint32_t millis();
inline void delay(uint32_t ms) { mockAdvanceMillis(ms); }
inline void yield() {}

// ---------------------------------------------------------------- aleatorio
// LCG propio: reproducible entre ejecuciones, para que una sesion grabada se
// repita igual y se puedan comparar capturas.
void mockSeedRandom(uint32_t s);
long random(long howbig);
long random(long howsmall, long howbig);
inline void randomSeed(unsigned long s) { mockSeedRandom((uint32_t)s); }

// Los tres de abajo los pide la bateria de tests, que desde que mete el
// sketch en un binario propio (test/test_ui.cpp) usa ESTOS shims y no
// los suyos. Al simulador no le estorban y asi no hay dos versiones de
// un mock diciendo cosas distintas.
//
// Forzar el dado sirve para los tests que no quieren depender de la
// suerte: un efecto del 10% se prueba poniendo el dado a 0.
void mockForceRandom(long value);
void mockClearForcedRandom();
void mockSerialSilence(bool quiet);
bool mockSerialIsSilent();

inline long constrain(long x, long a, long b) { return x < a ? a : (x > b ? b : x); }
inline long mapv(long x, long i1, long i2, long o1, long o2) {
  return (x - i1) * (o2 - o1) / (i2 - i1) + o1;
}
#define map mapv

// ---------------------------------------------------------------- String
// Lo comparte con el shim de la bateria de tests (ver el fichero).
#include "arduino_string.h"

// ---------------------------------------------------------------- Print
class Print {
public:
  virtual size_t write(uint8_t) = 0;
  virtual size_t write(const uint8_t *b, size_t n) {
    size_t k = 0;
    while (n--) k += write(*b++);
    return k;
  }
  size_t print(const char *s) { return s ? write((const uint8_t *)s, strlen(s)) : 0; }
  size_t print(const String &s) { return print(s.c_str()); }
  size_t print(char c) { return write((uint8_t)c); }
  size_t print(int v) { char b[24]; snprintf(b, sizeof(b), "%d", v); return print(b); }
  size_t print(unsigned v) { char b[24]; snprintf(b, sizeof(b), "%u", v); return print(b); }
  size_t println(const char *s = "") { size_t k = print(s); k += print("\n"); return k; }
};

// ---------------------------------------------------------------- Serial
// La consola serie del firmware (STATS, SPEC, LVL...) sigue funcionando: el
// simulador le pasa lineas por stdin, asi que los comandos de depuracion valen
// igual que con la placa enchufada.
void mockSerialPush(const std::string &line);

class SerialSim : public Print {
public:
  void begin(unsigned long) {}
  void setRxBufferSize(int) {}
  void flush() { fflush(stdout); }
  void setTxTimeoutMs(int) {}
  size_t write(uint8_t c) override {
    if (!mockSerialIsSilent()) fputc(c, stdout);
    return 1;
  }
  void printf(const char *f, ...) {
    if (mockSerialIsSilent()) return;
    va_list a; va_start(a, f); vprintf(f, a); va_end(a);
  }
  int available();
  String readStringUntil(char t);
  void setTimeout(unsigned long) {}
  // el simulador no transfiere ficheros por "USB": PUT corta por timeout, que
  // es exactamente lo que hace la placa si el emisor se calla
  size_t readBytes(uint8_t *, size_t) { return 0; }
};
extern SerialSim Serial;

// ---------------------------------------------------------------- PSRAM
inline void *ps_malloc(size_t n) { return malloc(n); }
inline void *ps_calloc(size_t n, size_t s) { return calloc(n, s); }
inline uint32_t ESP_getFreeHeap() { return 200000; }

struct EspSim {
  uint32_t getFreeHeap() { return 200000; }
  uint32_t getFreePsram() { return 4000000; }
  uint32_t getMinFreeHeap() { return 180000; }
  void restart() {}
};
extern EspSim ESP;

// El motivo del ultimo arranque. En la placa lo da el IDF; aqui el
// simulador siempre 'se acaba de encender', que es la verdad: no hay
// panicos ni brownouts que contar en un PC.
enum esp_reset_reason_t {
  ESP_RST_UNKNOWN = 0, ESP_RST_POWERON, ESP_RST_EXT, ESP_RST_SW,
  ESP_RST_PANIC, ESP_RST_INT_WDT, ESP_RST_TASK_WDT, ESP_RST_WDT,
  ESP_RST_DEEPSLEEP, ESP_RST_BROWNOUT, ESP_RST_SDIO,
};
inline esp_reset_reason_t esp_reset_reason() { return ESP_RST_POWERON; }

inline uint32_t esp_get_free_heap_size() { return 200000; }
