// El String de Arduino sobre std::string. Vive aparte porque lo usan
// DOS shims: el del simulador (desktop/shim/Arduino.h) y el de la
// bateria de tests (test/shim/Arduino.h), que desde que sdmon.cpp entro
// en la bateria tambien lo necesita.
//
// Una copia en cada sitio es exactamente como se cuelan las diferencias
// de comportamiento entre "lo que prueba el test" y "lo que corre el
// simulador", y entonces ninguno de los dos dice nada del aparato.
#pragma once
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>

class String : public std::string {
public:
  String() {}
  String(const char *s) : std::string(s ? s : "") {}
  String(const std::string &s) : std::string(s) {}
  String(int v) { char b[24]; snprintf(b, sizeof(b), "%d", v); assign(b); }
  String(unsigned v) { char b[24]; snprintf(b, sizeof(b), "%u", v); assign(b); }
  int toInt() const { return atoi(c_str()); }
  void trim() {
    size_t a = find_first_not_of(" \t\r\n");
    size_t b = find_last_not_of(" \t\r\n");
    if (a == npos) { clear(); return; }
    assign(substr(a, b - a + 1));
  }
  String substring(size_t a) const { return a <= size() ? String(substr(a)) : String(); }
  String substring(size_t a, size_t b) const {
    if (a >= size()) return String();
    return String(substr(a, b > a ? b - a : 0));
  }
  bool startsWith(const char *p) const { return rfind(p, 0) == 0; }
  bool equalsIgnoreCase(const char *o) const {
    std::string a = *this, b = o ? o : "";
    for (auto &c : a) c = (char)toupper((unsigned char)c);
    for (auto &c : b) c = (char)toupper((unsigned char)c);
    return a == b;
  }
  void toUpperCase() { for (auto &c : *this) c = (char)toupper((unsigned char)c); }
  int indexOf(char c) const { size_t p = find(c); return p == npos ? -1 : (int)p; }
  int indexOf(const char *s) const { size_t p = find(s); return p == npos ? -1 : (int)p; }
  int lastIndexOf(char c) const { size_t p = rfind(c); return p == npos ? -1 : (int)p; }
  unsigned length() const { return (unsigned)size(); }
  const char *cstr() const { return c_str(); }
};
