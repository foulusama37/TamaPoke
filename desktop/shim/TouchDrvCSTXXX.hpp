// Tactil falso: el simulador le inyecta toques desde el guion de la sesion.
#pragma once
#include <Arduino.h>
#include <Wire.h>

void mockTouchSet(int16_t x, int16_t y, bool down);

class TouchDrvCST92xx {
public:
  bool begin(TwoWire &, uint8_t, int, int) { return true; }
  void reset() {}
  void setPins(int, int) {}
  void setMaxCoordinates(int16_t, int16_t) {}
  void setMirrorXY(bool, bool) {}
  uint8_t getPoint(int16_t *x, int16_t *y, uint8_t n = 1);
};
