// Sistema de archivos falso sobre stdio: la "SD" es una carpeta del PC
// (por defecto tools/sdcard). Asi sdmon.cpp se compila TAL CUAL y el parseo
// de TPK2 que se ejercita es el de verdad, no una copia.
#pragma once
#include <Arduino.h>

#include <memory>
#include <string>
#include <vector>

#define FILE_READ "rb"
#define FILE_WRITE "wb"
#define FILE_APPEND "ab"

void mockSdRoot(const char *path);
const char *mockSdRootGet();

class File {
public:
  File() {}
  File(FILE *f, size_t sz, const std::string &nombre) : _f(f), _sz(sz), _name(nombre) {}
  // constructor de "directorio": guarda las entradas para openNextFile()
  explicit File(std::vector<std::string> hijos)
      : _dir(true), _hijos(std::make_shared<std::vector<std::string>>(std::move(hijos))) {}

  explicit operator bool() const { return _f != nullptr || _dir; }
  bool operator!() const { return !(_f != nullptr || _dir); }
  size_t size() const { return _sz; }
  const char *name() const { return _name.c_str(); }
  bool isDirectory() const { return _dir; }
  size_t read(uint8_t *buf, size_t n) { return _f ? fread(buf, 1, n, _f) : 0; }
  // Lo tiene el File de verdad, y hace falta para /fx/fx.bin: ese fichero
  // es un indice mas 396 registros y se salta directo al que toca, en vez
  // de leer un mega para quedarse con dos kilos.
  bool seek(uint32_t pos) { return _f && fseek(_f, (long)pos, SEEK_SET) == 0; }
  uint32_t position() const { return _f ? (uint32_t)ftell(_f) : 0; }
  size_t write(const uint8_t *buf, size_t n) { return _f ? fwrite(buf, 1, n, _f) : 0; }
  void close() { if (_f) { fclose(_f); _f = nullptr; } }
  File openNextFile();

private:
  FILE *_f = nullptr;
  size_t _sz = 0;
  std::string _name;
  bool _dir = false;
  std::shared_ptr<std::vector<std::string>> _hijos;
  size_t _i = 0;
};
