#include "gestos.h"
#include <math.h>

void LectorGestos::reset() {
  pend_ = Gestos();
  modulo_ = 1.0f;
  ultMeneo_ = ultTapa_ = ultPaso_ = 0;
  empujonEn_ = 0;
  tapasN_ = 0;
  bocabajoDesde_ = 0;
  bocabajoAvisado_ = false;
  subiendo_ = false;
}

void LectorGestos::muestra(float x, float y, float z, uint32_t ms) {
  // El MODULO de la aceleracion. Quieto vale 1 (la gravedad); el signo de
  // cada eje da la postura y el modulo da el movimiento, asi que las dos
  // preguntas salen de la misma muestra.
  modulo_ = sqrtf(x * x + y * y + z * z);

  // --- MENEO y SUSTO -------------------------------------------------
  // Se mira el pico, no la media: menear es un golpe seco de fuerza y
  // promediarlo lo borra. El tiempo muerto evita que un solo meneo se
  // cuente como cinco, que es lo que pasa a 100 muestras por segundo.
  if (modulo_ >= GESTO_MENEO_G && ms - ultMeneo_ >= GESTO_MENEO_MS) {
    ultMeneo_ = ms;
    pend_.meneo = true;
    // La fuerza se reparte entre el umbral de meneo y el de susto, para
    // que la captura pueda premiar un meneo de verdad frente a un roce.
    float t = (modulo_ - GESTO_MENEO_G) / (GESTO_SUSTO_G - GESTO_MENEO_G);
    if (t < 0) t = 0;
    if (t > 1) t = 1;
    uint8_t f = (uint8_t)(t * 255.0f);
    if (f > pend_.fuerza) pend_.fuerza = f;   // se guarda el mas fuerte
    if (modulo_ >= GESTO_SUSTO_G) pend_.susto = true;
  }

  // --- LANZAMIENTO ---------------------------------------------------
  // Un meneo va y vuelve; un lanzamiento acelera y PARA. Asi que no se
  // decide en el pico (los dos tienen pico) sino en lo que pasa justo
  // despues: si se queda quieta, la tiraste.
  if (modulo_ >= GESTO_TIRO_G) {
    empujonEn_ = ms;
  } else if (empujonEn_ && ms - empujonEn_ <= GESTO_TIRO_MS) {
    // El frenazo: casi sin aceleracion propia, solo la gravedad.
    if (fabsf(modulo_ - 1.0f) <= GESTO_TIRO_QUIETO) {
      pend_.tiro = true;
      empujonEn_ = 0;
    }
  } else if (empujonEn_) {
    empujonEn_ = 0;             // se le paso el tiempo: era un meneo
  }

  // --- BOCA ABAJO ----------------------------------------------------
  // No basta con que Z sea negativa un instante: hay que aguantar, o
  // darle la vuelta para mirar la pantalla contaria como acostarlo.
  if (z <= GESTO_BOCABAJO_G) {
    if (!bocabajoDesde_) bocabajoDesde_ = ms;
    if (!bocabajoAvisado_ && ms - bocabajoDesde_ >= GESTO_BOCABAJO_MS) {
      pend_.bocabajo = true;
      bocabajoAvisado_ = true;
    }
  } else {
    bocabajoDesde_ = 0;
    bocabajoAvisado_ = false;   // al darle la vuelta, se puede volver a avisar
  }

  // --- PASOS ---------------------------------------------------------
  // Andar es un vaiven RITMICO y suave. Se cuenta un paso al cruzar el
  // umbral hacia arriba, y solo si el ritmo es de persona: mas rapido es
  // menear y mas lento es que la dejaste quieta.
  if (modulo_ >= GESTO_PASO_G && modulo_ < GESTO_MENEO_G) {
    if (!subiendo_) {
      subiendo_ = true;
      uint32_t d = ms - ultPaso_;
      if (ultPaso_ && d >= GESTO_PASO_MIN_MS && d <= GESTO_PASO_MAX_MS &&
          pend_.pasos < 0xFFFF)
        pend_.pasos++;
      ultPaso_ = ms;
    }
  } else if (modulo_ < GESTO_PASO_G) {
    subiendo_ = false;
  }
}

void LectorGestos::tapa(uint8_t cuantas, uint32_t ms) {
  if (!cuantas) return;
  // El chip ya distingue una tapa de dos. Lo que no sabe es el RITMO: si
  // las tapas se encadenan, el jugador esta acariciando, no despertando.
  if (ms - ultTapa_ > GESTO_CARICIA_MS) tapasN_ = 0;
  ultTapa_ = ms;
  tapasN_ = (uint8_t)(tapasN_ + cuantas);

  if (cuantas >= 2 && tapasN_ <= 2) {
    // Dos justas y a secas: despertar. Si vienen mas detras, la tanda
    // pasa a ser caricia y esto se queda en el camino, que es lo que
    // hace que las dos cosas no se pisen.
    pend_.dosTapas = true;
  }
  if (tapasN_ >= GESTO_CARICIA_N) {
    pend_.caricia = true;
    pend_.dosTapas = false;   // acariciar NO es despertar
  }
}

Gestos LectorGestos::leer() {
  Gestos g = pend_;
  // Se consume todo menos el ritmo de tapas, que tiene que sobrevivir
  // entre lecturas para que una caricia larga siga contando.
  pend_ = Gestos();
  return g;
}
