// NVS en memoria (misma idea que test/shim/Preferences.h, copiada aparte para
// que el simulador no arrastre el shim.cpp de los tests, que tambien define
// millis/random/Serial y chocaria).
#pragma once
#include <stddef.h>
#include <stdint.h>

#include <map>
#include <string>
#include <vector>

class Preferences {
public:
  // Stub: en el PC no hay particion que llenar.
  size_t freeEntries() { return 1000; }
  bool begin(const char *name, bool readOnly = false);
  void end() { open_ = false; }
  bool clear();
  bool isKey(const char *k);
  bool remove(const char *k);

  size_t putBool(const char *k, bool v) { return put(k, &v, sizeof(v)); }
  size_t putUChar(const char *k, uint8_t v) { return put(k, &v, sizeof(v)); }
  size_t putChar(const char *k, int8_t v) { return put(k, &v, sizeof(v)); }
  size_t putShort(const char *k, int16_t v) { return put(k, &v, sizeof(v)); }
  size_t putUShort(const char *k, uint16_t v) { return put(k, &v, sizeof(v)); }
  size_t putInt(const char *k, int32_t v) { return put(k, &v, sizeof(v)); }
  size_t putUInt(const char *k, uint32_t v) { return put(k, &v, sizeof(v)); }
  size_t putBytes(const char *k, const void *v, size_t n) { return put(k, v, n); }
  size_t putString(const char *k, const char *v);

  bool getBool(const char *k, bool d = false) { return get(k, d); }
  uint8_t getUChar(const char *k, uint8_t d = 0) { return get(k, d); }
  int8_t getChar(const char *k, int8_t d = 0) { return get(k, d); }
  int16_t getShort(const char *k, int16_t d = 0) { return get(k, d); }
  uint16_t getUShort(const char *k, uint16_t d = 0) { return get(k, d); }
  int32_t getInt(const char *k, int32_t d = 0) { return get(k, d); }
  uint32_t getUInt(const char *k, uint32_t d = 0) { return get(k, d); }
  size_t getBytes(const char *k, void *buf, size_t maxLen);
  // Existe na Preferences de verdade; o DIAG precisa dela para dizer
  // quantos bytes cada chave tem mesmo.
  size_t getBytesLength(const char *k);
  size_t getString(const char *k, char *buf, size_t maxLen);

private:
  size_t put(const char *k, const void *v, size_t n);
  template <typename T>
  T get(const char *k, T def) {
    T out = def;
    return getBytes(k, &out, sizeof(T)) == sizeof(T) ? out : def;
  }
  std::string ns_;
  bool open_ = false, ro_ = false;
};

void mockNvsReset();
// Recusa toda escrita (a NVS cheia). Para testar o aviso e a nova tentativa.
void mockNvsRecusa(bool recusa);
// La NVS a disco y de vuelta: sin esto, en el PC cada arranque empieza
// por el profesor Oak, porque la "memoria" vive en RAM y muere con el
// proceso. En la placa no hace falta: alli Preferences es flash.
void mockNvsSave(const char *path);
bool mockNvsLoad(const char *path);
