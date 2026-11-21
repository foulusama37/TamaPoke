#pragma once
#include <Arduino.h>

// RTC PCF85063: hora persistente mientras la placa tenga alimentacion
bool rtcBegin();
uint32_t rtcEpoch();             // segundos unix; 0 si el RTC no es valido
void rtcSetEpoch(uint32_t e);

// PMU AXP2101: estado de la bateria
bool batBegin();
void pmuEnablePanel();           // enciende BLDO1 (OLED VDD 3.3V); llamar antes de gfx->begin()
int batPercent();                // 0-100, -1 si no hay bateria conectada
bool batCharging();
// Fija el alvo (4,2 V), la corriente de carga (500 mA para la celda
// de 1100 mAh) y el limite de entrada. El AXP2101 carga solo al
// enchufar el USB, pero con lo que tenga puesto: esto no lo deja al
// azar. Lo llama pwrSetup().
void pwrChargeSetup();
// Saca por el serie lo que el chip dice DE VERDAD de la carga.
void pwrChargeReport();
int batMillivolts();               // tension de la bateria en mV, 0 si no hay lectura
bool usbPresent();

// boton PWR del AXP2101: pulsacion larga 4s = apagado fisico (RTC sigue vivo);
// la pulsacion corta la captura el firmware (pantalla on/off)
void pwrSetup();
bool pwrShortPressed();  // sondear en el loop
// El boton esta apretado AHORA. Sale de los dos IRQ de FLANCO del
// AXP2101 (negativo = apretar, positivo = soltar), no de leer un pin:
// el PWR no llega al ESP32, solo al PMU.
bool pwrKeyHeld();
// Corte por software, el mismo que hace el chip al aguantar el boton.
// Volcar lo que haya que volcar ANTES: detras de esto no corre nada.
void pwrOff();
