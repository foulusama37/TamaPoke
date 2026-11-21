#pragma once
#include <stdint.h>
#include "gestos.h"

// El acelerometro QMI8658 de la placa. Seis ejes, por I2C, en el mismo bus
// que el tactil y el RTC.
//
// Esto es SOLO el driver: lee el chip y le pasa las muestras al
// LectorGestos, que es quien decide. La division es a proposito — el I2C
// no se puede comprobar sin la placa (como audio.cpp y rtcbat.cpp), pero
// los gestos si, y estan probados en test_gestos.cpp.
//
// El chip trae detector de toque y de DOBLE toque en hardware, asi que
// las tapas no se adivinan mirando picos: las dice el sensor.

bool imuBegin();        // false si el chip no responde (el juego sigue)
bool imuPresente();
void imuTick();         // del bucle principal; barato, no bloquea
Gestos imuGestos();     // lee y CONSUME los gestos pendientes

// Para la pantalla de depuracion.
float imuModulo();
