// Shim de la API Preferences (NVS de ESP32) en memoria.
// Comparte un unico almacen global entre instancias, igual que la NVS real,
// para poder testear guardado/carga y persistencia entre "arranques".
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string>

class Preferences {
public:
  // Stub: en el PC no hay particion que llenar.
  size_t freeEntries() { return 1000; }
  bool begin(const char *name, bool readOnly = false);
  void end();
  bool clear();
  bool isKey(const char *key);
  bool remove(const char *key);

  size_t putBool(const char *k, bool v);
  size_t putUChar(const char *k, uint8_t v);
  size_t putChar(const char *k, int8_t v);
  size_t putShort(const char *k, int16_t v);
  size_t putUShort(const char *k, uint16_t v);
  size_t putInt(const char *k, int32_t v);
  size_t putUInt(const char *k, uint32_t v);
  size_t putBytes(const char *k, const void *v, size_t len);
  size_t putString(const char *k, const char *v);

  bool getBool(const char *k, bool def = false);
  uint8_t getUChar(const char *k, uint8_t def = 0);
  int8_t getChar(const char *k, int8_t def = 0);
  int16_t getShort(const char *k, int16_t def = 0);
  uint16_t getUShort(const char *k, uint16_t def = 0);
  int32_t getInt(const char *k, int32_t def = 0);
  uint32_t getUInt(const char *k, uint32_t def = 0);
  size_t getBytes(const char *k, void *buf, size_t maxLen);
  // Existe na Preferences de verdade; faltava aqui e o DIAG nao
  // compilava nos testes.
  size_t getBytesLength(const char *k);
  size_t getString(const char *k, char *buf, size_t maxLen);

private:
  std::string ns_;
  bool open_ = false;
  bool ro_ = false;
};

// helpers de test: borra toda la "NVS" simulada
void mockNvsReset();
// Tope de tamano para putBytes: a partir de aqui devuelve 0, como hace la
// NVS de verdad cuando la particion no tiene sitio. 0 = sin tope.
// Hace falta para poder PROBAR el fallo de escritura: sin esto no hay
// forma de que falle y el test seria un test de que no pasa nada.
void mockNvsTopeBlob(size_t tope);
// Recusa as escritas de escalares (putUChar & cia.), deixando os blobs. E o caso
// da NVS cheia num save em que os blocos grandes nao mudaram: a NVS real nao
// regrava um blob identico, entao so falha o que mudou, e isso e pequeno.
void mockNvsRecusaEscalares(bool recusa);
size_t mockNvsKeyCount(const char *ns);
