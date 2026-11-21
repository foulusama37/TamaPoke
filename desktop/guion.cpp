#include "guion.h"

#include <Arduino.h>

#include <fstream>
#include <sstream>

void mockTouchSet(int16_t x, int16_t y, bool down);
void mockSetEpoch(uint32_t e);
void radioFantasmaConvida(uint8_t regras);   // troca_radio.cpp (PC)

bool corriendoGuion(const std::string &path,
                    const std::function<void(uint32_t)> &avanza,
                    const std::function<void(const char *)> &captura,
                    const TrasCadaPaso &despues) {
  std::ifstream in(path);
  if (!in) return false;
  std::string ln;
  while (std::getline(in, ln)) {
    if (ln.empty() || ln[0] == '#') continue;
    std::istringstream s(ln);
    std::string cmd;
    s >> cmd;
    if (cmd == "wait") {
      uint32_t ms = 0;
      s >> ms;
      avanza(ms);
    } else if (cmd == "tap") {
      int x = 0, y = 0;
      s >> x >> y;
      mockTouchSet((int16_t)x, (int16_t)y, true);
      avanza(120);
      mockTouchSet((int16_t)x, (int16_t)y, false);
      avanza(160);
    } else if (cmd == "hold") {
      int x = 0, y = 0;
      uint32_t ms = 0;
      s >> x >> y >> ms;
      mockTouchSet((int16_t)x, (int16_t)y, true);
      avanza(ms);
      mockTouchSet((int16_t)x, (int16_t)y, false);
      avanza(160);
    } else if (cmd == "swipe") {
      int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
      uint32_t ms = 0;
      s >> x0 >> y0 >> x1 >> y1 >> ms;
      const int N = 10;
      for (int k = 0; k <= N; k++) {
        mockTouchSet((int16_t)(x0 + (x1 - x0) * k / N),
                     (int16_t)(y0 + (y1 - y0) * k / N), true);
        avanza(ms / N);
      }
      mockTouchSet((int16_t)x1, (int16_t)y1, false);
      avanza(200);
    } else if (cmd == "serial") {
      std::string resto;
      std::getline(s, resto);
      if (!resto.empty() && resto[0] == ' ') resto.erase(0, 1);
      mockSerialPush(resto);
      avanza(200);
    } else if (cmd == "epoch") {
      uint32_t e = 0;
      s >> e;
      mockSetEpoch(e);
      avanza(50);
    } else if (cmd == "convida") {
      // a placa fantasma convida para batalhar (regras = o byte do convite)
      int r = 0;
      s >> r;
      radioFantasmaConvida((uint8_t)r);
      avanza(50);
    } else if (cmd == "shot") {
      std::string n;
      s >> n;
      if (captura) captura(n.c_str());
      avanza(20);
    } else {
      // Un comando que no se reconoce NO se ignora en silencio: un
      // guion con una errata pasaria entero sin hacer la mitad de lo
      // que dice, y eso en una bateria de tests se lee como "todo
      // bien". Se avisa por la salida de error.
      fprintf(stderr, "[guion] %s: comando desconocido '%s'\n",
              path.c_str(), cmd.c_str());
    }
    if (despues) despues(ln);
  }
  return true;
}
