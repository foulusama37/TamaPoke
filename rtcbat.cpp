#include "rtcbat.h"
#include "pin_config.h"  // define XPOWERS_CHIP_AXP2101
#include <Wire.h>
#include <time.h>
#include <SensorPCF85063.hpp>
#include <XPowersLib.h>

static SensorPCF85063 rtc;
static XPowersPMU pmu;
static bool rtcOk = false;
static bool pmuOk = false;

bool rtcBegin() {
  rtcOk = rtc.begin(Wire, IIC_SDA, IIC_SCL);
  if (!rtcOk) Serial.println("PCF85063 no detectado");
  return rtcOk;
}

uint32_t rtcEpoch() {
  if (!rtcOk) return 0;
  RTC_DateTime t = rtc.getDateTime();
  if (t.getYear() < 2025 || t.getYear() > 2120) return 0;  // sin hora valida
  struct tm tmv = {};
  tmv.tm_year = t.getYear() - 1900;
  tmv.tm_mon = t.getMonth() - 1;
  tmv.tm_mday = t.getDay();
  tmv.tm_hour = t.getHour();
  tmv.tm_min = t.getMinute();
  tmv.tm_sec = t.getSecond();
  time_t e = mktime(&tmv);  // TZ por defecto = UTC, consistente con gmtime_r
  return e > 0 ? (uint32_t)e : 0;
}

void rtcSetEpoch(uint32_t e) {
  if (!rtcOk) return;
  time_t tt = e;
  struct tm tmv;
  gmtime_r(&tt, &tmv);
  rtc.setDateTime(RTC_DateTime(tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday,
                               tmv.tm_hour, tmv.tm_min, tmv.tm_sec));
}

bool batBegin() {
  pmuOk = pmu.begin(Wire, AXP2101_SLAVE_ADDRESS, IIC_SDA, IIC_SCL);
  if (!pmuOk) Serial.println("AXP2101 no detectado");
  return pmuOk;
}

// Enciende la alimentacion de la AMOLED. En la Waveshare 1.75 el panel (OLED VDD)
// cuelga del rail BLDO1 a 3.3V del AXP2101. El firmware daba por hecho que estaba
// encendido; si el PMU se resetea (drenaje total), BLDO1 queda OFF y la pantalla
// se ve negra aunque el resto funcione. Hay que llamarla ANTES de gfx->begin().
void pmuEnablePanel() {
  if (!pmu.begin(Wire, AXP2101_SLAVE_ADDRESS, IIC_SDA, IIC_SCL)) {
    Serial.println("AXP2101 no detectado (pmuEnablePanel)");
    return;
  }
  pmu.setBLDO1Voltage(3300);   // OLED VDD
  pmu.enableBLDO1();
}

// el estado de energia (I2C) se cachea ~2 s: leerlo en cada frame del loop
// metia trafico I2C inutil y podia oscilar (parpadeo de brillo)
static uint32_t powerCacheT = 0;
static int cachedPct = -1;
static bool cachedCharging = false, cachedUsb = true;

static void refreshPower() {
  uint32_t now = millis();
  if (powerCacheT && now - powerCacheT < 2000) return;
  powerCacheT = now ? now : 1;
  if (!pmuOk) { cachedPct = -1; cachedCharging = false; cachedUsb = true; return; }
  cachedPct = pmu.isBatteryConnect() ? pmu.getBatteryPercent() : -1;
  cachedCharging = pmu.isCharging();
  cachedUsb = pmu.isVbusIn();
}

int batPercent() { refreshPower(); return cachedPct; }
bool batCharging() { refreshPower(); return cachedCharging; }
int batMillivolts() { return pmuOk ? (int)pmu.getBattVoltage() : 0; }
bool usbPresent() { refreshPower(); return cachedUsb; }

// La CARGA de la bateria. El AXP2101 la hace en hardware -- enchufar el
// USB ya carga, sin codigo -- pero con lo que tenga configurado en ese
// momento, y eso no es de fiar: un PMU que se resetea por descarga total
// vuelve a sus valores de arranque. Ya mordio una vez en este proyecto
// (de ahi pmuEnablePanel), asi que aqui se FIJA.
//
// La celda es de 1100 mAh:
//   - alvo 4,2 V. Una celda de "3,7 V" llena esta a 4,2; el 3,7 es la
//     media de la descarga, no el tope. Poner 4,35 o 4,4 la estropea.
//   - 500 mA de carga = 0,45C. Llena en ~2,5 h y no calienta. 1000 mA
//     (0,9C) lo aguantaria, pero calienta y acorta la vida de la celda.
//   - el limite de ENTRADA es lo que mas importa enchufado al PC: un
//     puerto USB 2.0 garantiza 500 mA EN TOTAL, y la placa ya chupa lo
//     suyo con el AMOLED encendido. Pidiendo 500 de carga MAS el gasto
//     de la placa se pasa del presupuesto del puerto y el AXP2101 acaba
//     recortando solo. Con 900 mA de limite el chip reparte: alimenta la
//     placa primero y carga con lo que sobre.
void pwrChargeSetup() {
  if (!pmuOk) return;
  pmu.setChargeTargetVoltage(XPOWERS_AXP2101_CHG_VOL_4V2);
  pmu.setChargerConstantCurr(XPOWERS_AXP2101_CHG_CUR_500MA);
  pmu.setVbusCurrentLimit(XPOWERS_AXP2101_VBUS_CUR_LIM_900MA);
  // No hay enableCharge(): la carga de la bateria principal esta SIEMPRE
  // activa en el AXP2101, se configura y ya. (El enableCharge que trae la
  // libreria es el de la pila de boton del RTC, otra cosa.) Me lo invente
  // y el compilador me paro.
}

// Lo que el chip dice DE VERDAD, no lo que yo creo haberle puesto. Sale
// por el puerto serie al arrancar: si algun dia carga raro, esto dice si
// es la configuracion o es otra cosa, sin tener que adivinar.
void pwrChargeReport() {
  if (!pmuOk) { Serial.println("PMU: no detectado"); return; }
  static const uint16_t CUR[] = { 0, 0, 0, 0, 100, 125, 150, 175, 200,
                                  300, 400, 500, 600, 700, 800, 900, 1000 };
  static const char *VOL[] = { "?", "4.0V", "4.1V", "4.2V", "4.35V", "4.4V" };
  uint8_t c = pmu.getChargerConstantCurr();
  uint8_t t = pmu.getChargeTargetVoltage();
  Serial.printf(
      "PMU: usb=%d cargando=%d bateria=%d%% %dmV | carga %umA, alvo %s\n",
      pmu.isVbusIn() ? 1 : 0, pmu.isCharging() ? 1 : 0,
      pmu.isBatteryConnect() ? pmu.getBatteryPercent() : -1,
      (int)pmu.getBattVoltage(),
      c < sizeof(CUR) / sizeof(CUR[0]) ? CUR[c] : 0,
      t < sizeof(VOL) / sizeof(VOL[0]) ? VOL[t] : "?");
}

void pwrSetup() {
  if (!pmuOk) return;
  pwrChargeSetup();
  pwrChargeReport();
  // SEIS segundos y no cuatro. El corte lo sigue haciendo el chip por su
  // cuenta — el PWR no llega al ESP32 —, asi que es la unica red de
  // seguridad si el firmware se cuelga. Pero ahora el firmware pregunta a
  // los 700 ms si quieres GUARDAR, y con cuatro segundos la pregunta se
  // quedaba sin tiempo de contestar. Aguantando igual se apaga: 6 s.
  pmu.setPowerKeyPressOffTime(XPOWERS_POWEROFF_6S);
  pmu.disableIRQ(XPOWERS_AXP2101_ALL_IRQ);
  // Los dos flancos ademas del toque corto: con ellos el firmware sabe que
  // el boton SIGUE apretado, que es lo que no se podia saber antes.
  pmu.enableIRQ(XPOWERS_AXP2101_PKEY_SHORT_IRQ |
                XPOWERS_AXP2101_PKEY_NEGATIVE_IRQ |
                XPOWERS_AXP2101_PKEY_POSITIVE_IRQ);
  pmu.clearIrqStatus();
}

// UNA lectura del estado para los tres sucesos. Antes cada funcion leia y
// limpiaba por su cuenta, y con tres eso se come los sucesos de las otras
// dos: el que limpia primero borra lo que las demas no han visto.
static bool evCorto = false;      // hubo toque corto sin recoger
static bool teclaAbajo = false;   // el boton esta apretado
static uint32_t abajoDesde = 0;   // cuando se apreto (ms)

static void pwrSondea() {
  if (!pmuOk) return;
  pmu.getIrqStatus();
  if (pmu.isPekeyShortPressIrq()) evCorto = true;
  // PWRON es activo a nivel BAJO: el flanco negativo es apretar.
  if (pmu.isPekeyNegativeIrq()) { teclaAbajo = true; abajoDesde = millis(); }
  if (pmu.isPekeyPositiveIrq()) teclaAbajo = false;
  pmu.clearIrqStatus();
  // Red de seguridad por si me equivoco de polaridad o se pierde el flanco
  // de soltar: a los 8 s el chip ya habria cortado la corriente hace dos,
  // asi que si seguimos vivos con la tecla "abajo" es que no lo esta. Sin
  // esto, un flanco perdido dejaria la pregunta de guardar pegada en
  // pantalla para siempre.
  if (teclaAbajo && millis() - abajoDesde > 8000) teclaAbajo = false;
}

bool pwrShortPressed() {
  pwrSondea();
  bool hit = evCorto;
  evCorto = false;
  return hit;
}

bool pwrKeyHeld() {
  pwrSondea();
  return teclaAbajo;
}

void pwrOff() {
  if (!pmuOk) return;
  Serial.println("apagando");
  Serial.flush();
  pmu.shutdown();
}
