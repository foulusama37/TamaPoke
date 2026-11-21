// Shim minimo de Arduino.h para ejecutar la logica del firmware en el host.
// Cubre lo que usan pet.cpp, i18n.cpp, battle.cpp y sdmon.cpp. El reloj es
// falso y controlable para que los tests sean deterministas.
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// String y ps_malloc los pide sdmon.cpp, que entro en la bateria para
// poder cargar los sprites CON EL CODIGO DEL FIRMWARE y no con una
// copia en Python. El String es el mismo fichero que usa el simulador:
// dos copias del mismo tipo es como se cuela una diferencia entre lo
// que prueba el test y lo que corre el aparato.
#include "../../desktop/shim/arduino_string.h"

// En el aparato esto reserva en la PSRAM de 8 MB; aqui es malloc y ya.
static inline void *ps_malloc(size_t n) { return malloc(n); }

// ---- reloj falso -----------------------------------------------------------
uint32_t millis();
void mockSetMillis(uint32_t ms);
void mockAdvanceMillis(uint32_t ms);

// ---- random determinista (misma semantica que Arduino: [0, howbig) ) -------
long random(long howbig);
long random(long howsmall, long howbig);
void randomSeed(unsigned long seed);
// fuerza el siguiente/los siguientes random() a un valor fijo (para tests)
void mockForceRandom(long value);   // todos los random() devuelven value
void mockClearForcedRandom();

// ---- Serial ----------------------------------------------------------------
struct MockSerial {
  int printf(const char *fmt, ...);
  void print(const char *s);
  void println(const char *s);
  void begin(unsigned long);
  // Los pide sdSerialCommand() en sdmon.cpp (el protocolo de
  // tools/send_sd.py). Aqui no hay puerto serie: leer siempre devuelve
  // cero bytes, que es lo que el codigo tiene que saber aguantar.
  void setTimeout(unsigned long) {}
  size_t readBytes(uint8_t *, size_t) { return 0; }
};
extern MockSerial Serial;
void mockSerialSilence(bool quiet);

// ---- utilidades ------------------------------------------------------------
template <class T> static inline T min(T a, T b) { return a < b ? a : b; }
template <class T> static inline T max(T a, T b) { return a > b ? a : b; }
