// La "SD" como una carpeta del PC. Vive APARTE del resto del shim para
// que la use tambien la bateria de tests: asi sdmon.cpp se compila TAL
// CUAL en los dos sitios y lo que se ejercita es su parseo de verdad,
// no una copia en Python que puede entender el formato de otra manera.
//
// Es justo el fallo que se escondio mas tiempo en este proyecto:
// PmdMon::load recibia el numero de dex en uint8_t y desde Hoenn pedia
// el fichero equivocado. tools/auditoria.py no podia verlo — comprueba
// que los ficheros esten y se puedan leer, no que el FIRMWARE sepa
// pedirlos. Para eso hay que correr el firmware contra la tarjeta, y
// para eso esta este fichero.
#include <Arduino.h>
#include <FS.h>
#include <SD_MMC.h>

#include <algorithm>
#include <filesystem>
#include <string>
#include <sys/stat.h>
#include <vector>

namespace fs = std::filesystem;

SDMMCSim SD_MMC;
static std::string g_root = "../tools/sdcard";
void mockSdRoot(const char *p) { g_root = p; }
const char *mockSdRootGet() { return g_root.c_str(); }

static std::string full(const char *path) {
  std::string p = path ? path : "";
  if (!p.empty() && p[0] == '/') p.erase(0, 1);
  return g_root + "/" + p;
}

File SDMMCSim::open(const char *path, const char *mode) {
  std::string p = full(path);
  if (fs::is_directory(p)) {
    std::vector<std::string> hijos;
    for (auto &e : fs::directory_iterator(p)) hijos.push_back(e.path().string());
    std::sort(hijos.begin(), hijos.end());
    return File(std::move(hijos));
  }
  FILE *f = fopen(p.c_str(), mode);
  if (!f) return File();
  fseek(f, 0, SEEK_END);
  long sz = ftell(f);
  fseek(f, 0, SEEK_SET);
  std::string nom = p.substr(p.find_last_of("/\\") + 1);
  return File(f, sz < 0 ? 0 : (size_t)sz, nom);
}

File File::openNextFile() {
  if (!_dir || !_hijos || _i >= _hijos->size()) return File();
  std::string p = (*_hijos)[_i++];
  FILE *f = fopen(p.c_str(), "rb");
  if (!f) return File();
  fseek(f, 0, SEEK_END);
  long sz = ftell(f);
  fseek(f, 0, SEEK_SET);
  std::string nom = p.substr(p.find_last_of("/\\") + 1);
  return File(f, sz < 0 ? 0 : (size_t)sz, nom);
}

bool SDMMCSim::exists(const char *path) {
  struct stat st;
  return stat(full(path).c_str(), &st) == 0;
}

bool SDMMCSim::mkdir(const char *path) {
#ifdef _WIN32
  return ::mkdir(full(path).c_str()) == 0;
#else
  return ::mkdir(full(path).c_str(), 0777) == 0;
#endif
}

bool SDMMCSim::remove(const char *path) {
  return ::remove(full(path).c_str()) == 0;
}

bool SDMMCSim::rename(const char *de, const char *para) {
  return ::rename(full(de).c_str(), full(para).c_str()) == 0;
}
