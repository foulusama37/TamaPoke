#include "imu.h"
#include <Arduino.h>
#include <Wire.h>
// ImuDrv.hpp y NO SensorQMI8658.hpp: ese de arriba es el envoltorio
// VIEJO que la propia libreria avisa por pantalla que va a desaparecer
// ("Please replace with a new driver"), y con la placa delante resulto
// que su begin() entra en PANICO -- el aparato se quedaba reiniciando
// cada 3,7 s. El chip contesta perfectamente en 0x6B; el roto estaba en
// el envoltorio. Ojo: los dos ficheros se llaman SensorQMI8658.hpp (uno
// en src/, el nuevo en src/sensor/imu/qmi8658/), asi que hay que entrar
// por ImuDrv.hpp para coger el bueno.
#include <ImuDrv.hpp>
#include "pin_config.h"

// El QMI8658 vive en el MISMO bus I2C que el tactil (CST9217) y el RTC
// (PCF85063), asi que aqui no se llama a Wire.begin(): lo hizo ya quien
// arranco primero. Llamarlo otra vez reinicia el bus y deja el tactil
// muerto hasta el siguiente arranque.
static SensorQMI8658 imu;
static LectorGestos lector;
static bool ok = false;

// Cada cuanto se lee. A 50 Hz sobra para todo lo que se mira (un meneo
// dura 200 ms, un paso 600) y no se come el bucle principal, que tiene
// que repintar 466x466.
static const uint32_t IMU_MS = 20;

// Gravedad en m/s2. Convierte lo que da el driver a las G en las que
// estan escritos los umbrales de gestos.h.
static const float G_POR_MS2 = 9.80665f;
static uint32_t ultLeido = 0;

// Quien contesta en esa direccion del bus. Es una pregunta de Wire, no
// del driver: no toca ningun registro del chip ni deja nada a medias.
static bool respondeEnI2C(uint8_t dir) {
  Wire.beginTransmission(dir);
  return Wire.endTransmission() == 0;
}

bool imuBegin() {
  // PRIMERO se pregunta al bus, y solo DESPUES se llama a begin(), una
  // sola vez y a la direccion que contesto.
  //
  // Antes esto era `begin(L) || begin(H)`, y ese "ahorra una tarde" costo
  // la tarde: con la placa delante, el aparato entraba en bucle de
  // reinicios (~3,7 s) por PANICO dentro de imuBegin(), antes de que
  // saliera ninguno de sus dos mensajes. Llamar a begin() por segunda vez
  // sobre un objeto que se quedo a medias en el primer intento no esta
  // previsto por el driver. Sondear es lo que habia que hacer desde el
  // principio: ademas distingue "no hay chip" de "hay chip y no arranca",
  // que son dos averias distintas.
  Serial.println("[imu] sondeando el bus");
  uint8_t dir = 0;
  if (respondeEnI2C(QMI8658_L_SLAVE_ADDRESS)) dir = QMI8658_L_SLAVE_ADDRESS;
  else if (respondeEnI2C(QMI8658_H_SLAVE_ADDRESS)) dir = QMI8658_H_SLAVE_ADDRESS;
  if (!dir) {
    Serial.println("QMI8658 no contesta en el bus I2C (gestos off)");
    return false;
  }
  Serial.printf("[imu] contesta en 0x%02X, arrancando\n", dir);

  ok = imu.begin(Wire, dir, IIC_SDA, IIC_SCL);
  if (!ok) {
    Serial.println("QMI8658 contesta pero no arranca (gestos off)");
    return false;
  }
  Serial.printf("QMI8658 id=0x%02X\n", imu.getChipID());

  // +-8g: un meneo de verdad pasa de 3g y con +-4g se saturaria justo
  // donde hace falta medir la fuerza. 125 Hz da margen sobre los 50 a
  // los que se lee, para que el filtro del chip tenga con que trabajar.
  imu.configAccel(AccelFullScaleRange::FS_8G, 125.0f,
                  SensorQMI8658::LpfMode::MODE_0);
  imu.enableAccel();

  // El GIROSCOPIO se queda APAGADO. No hace falta para nada de lo que se
  // mira (meneo, tapas, postura, pasos) y encenderlo cuesta bateria en
  // un aparato que vive de una celda de 3,7 V.
  imu.disableGyro();

  // Y el detector de TAPAS del propio chip. Los numeros son los del
  // ejemplo de Waveshare; la ventana de doble toque es la que separa
  // "dos tapas" de "dos tapas sueltas".
  // La firma de verdad lleva OCHO parametros y los cuatro ultimos son
  // floats (los umbrales), no bytes. Me invente una de siete y el
  // compilador me paro: por eso los numeros salen del ejemplo oficial
  // de la libreria y no de mi cabeza.
  //   prioridad, ventana de pico, ventana de tapa, ventana de doble tapa,
  //   alpha, gamma, umbral de pico, umbral de direccion
  // configTapDefault y no seis numeros a mano: la libreria trae unos
  // valores sensatos y yo no tengo con que medir cual es mejor hasta
  // tener el aparato cerrado en la mano. Z_GT_X_GT_Y porque las tapas
  // llegan por la cara de la pantalla, que es el eje Z.
  imu.configTapDefault(SensorQMI8658::TapPriority::Z_GT_X_GT_Y);
  imu.enableTap();

  lector.reset();
  ultLeido = millis();
  return true;
}

bool imuPresente() { return ok; }

void imuTick() {
  if (!ok) return;
  uint32_t ahora = millis();
  if (ahora - ultLeido < IMU_MS) return;
  ultLeido = ahora;

  // OJO CON LAS UNIDADES. El driver nuevo entrega metros por segundo al
  // cuadrado; el viejo daba G. Todos los umbrales de gestos.h estan en G
  // (GESTO_BOCABAJO_G vale -0,75: es gravedad, no m/s2), asi que sin esta
  // division cada gesto saltaria con la decima parte del movimiento y el
  // bicho se creeria zarandeado estando quieto.
  AccelerometerData a;
  if (imu.readAccel(a)) {
    lector.muestra(a.mps2.x / G_POR_MS2, a.mps2.y / G_POR_MS2,
                   a.mps2.z / G_POR_MS2, ahora);
  }

  // Las tapas las cuenta el chip; aqui solo se recoge el aviso. Hay que
  // preguntar en cada vuelta: el estado se borra al leerlo.
  switch (imu.getTapStatus()) {
    case TapEvent::SINGLE: lector.tapa(1, ahora); break;
    case TapEvent::DOUBLE: lector.tapa(2, ahora); break;
    default: break;
  }
}

Gestos imuGestos() {
  if (!ok) return Gestos();
  return lector.leer();
}

float imuModulo() { return lector.ultimoModulo(); }
