#pragma once
#include <FS.h>

// Las rutas llegan como String desde la consola serie y como const char* desde
// el codigo: se aceptan las dos para que sdmon.cpp compile sin tocarlo.
class SDMMCSim {
public:
  void setPins(int, int, int) {}
  bool begin(const char * = "/sdcard", bool = true, bool = false) { return true; }
  File open(const char *path, const char *mode = FILE_READ);
  File open(const String &path, const char *mode = FILE_READ) { return open(path.c_str(), mode); }
  bool exists(const char *path);
  bool exists(const String &path) { return exists(path.c_str()); }
  bool mkdir(const char *path);
  bool mkdir(const String &path) { return mkdir(path.c_str()); }
  bool remove(const char *path);
  bool remove(const String &path) { return remove(path.c_str()); }
  bool rename(const char *de, const char *para);
  uint64_t cardSize() { return 8ULL * 1024 * 1024 * 1024; }
  uint64_t totalBytes() { return 8ULL * 1024 * 1024 * 1024; }
  uint64_t usedBytes() { return 64ULL * 1024 * 1024; }
};
extern SDMMCSim SD_MMC;
