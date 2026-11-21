#pragma once
#include <stdint.h>

// Los GESTOS, separados del driver a proposito.
//
// imu.cpp habla con el QMI8658 por I2C y eso no se puede comprobar sin la
// placa — igual que audio.cpp y rtcbat.cpp, que cobertura.py cuenta como
// "puro driver". Pero lo que DECIDE (esto es un meneo o un bache?, van dos
// tapas o una?, esta boca abajo?) son cuentas sobre numeros, y eso si se
// puede probar: se le dan muestras a mano y se mira que conteste.
//
// Por eso el driver solo trae tres floats y un reloj, y aqui vive todo lo
// que piensa. Si manana cambia el sensor, esto no se toca.

// Un meneo util no es "se movio": es que la aceleracion se salga de la
// gravedad con fuerza y VARIE. Dejar la consola en una mesa que vibra da
// un modulo distinto de 1g pero constante, y eso no es menear.
#define GESTO_MENEO_G      1.7f   // g de pico para contar un meneo
#define GESTO_MENEO_MS      120   // no se cuentan dos meneos mas juntos
#define GESTO_SUSTO_G      3.2f   // de aqui para arriba, es un zarandeo

// Las TAPAS las detecta el propio chip (tiene detector de toque y de
// doble toque en hardware), asi que aqui solo se cuenta el ritmo: varias
// seguidas son caricias, dos justas son "despierta".
#define GESTO_CARICIA_N       3   // tapas seguidas para que cuente como mimo
#define GESTO_CARICIA_MS   1200   // ventana entre tapas de una misma tanda

// LANZAMIENTO. La consola va en una carcasa de Pokebola, asi que el
// gesto natural es tirarla: un empujon fuerte y, justo despues, un
// frenazo. Un meneo va y vuelve; un lanzamiento acelera y PARA. La
// diferencia esta en lo que pasa DESPUES del pico, no en el pico.
#define GESTO_TIRO_G       2.2f   // el empujon
#define GESTO_TIRO_QUIETO  0.35f  // y lo quieto que se queda al frenar
#define GESTO_TIRO_MS       400   // cuanto se espera el frenazo

// Boca abajo: la gravedad apunta al reves en Z. Se pide un margen para que
// dejarla de canto en una mesa no cuente.
#define GESTO_BOCABAJO_G  -0.75f
#define GESTO_BOCABAJO_MS  2000   // y que aguante asi dos segundos

// Un paso es un pico suave y ritmico, nada que ver con un meneo: la gente
// no sacude el bolsillo, lo balancea.
#define GESTO_PASO_G       1.25f
#define GESTO_PASO_MIN_MS   250   // mas rapido que esto no es andar
#define GESTO_PASO_MAX_MS  2000   // mas lento, tampoco

// Lo que el juego pregunta. Cada uno se CONSUME al leerlo (vuelve a
// false), asi que una tapa no se cobra dos veces.
struct Gestos {
  bool meneo = false;        // un meneo util (minijuego, captura)
  bool tiro = false;         // un LANZAMIENTO: empujon y frenazo
  bool susto = false;        // un zarandeo brusco: al bicho no le gusta
  bool caricia = false;      // varias tapas seguidas = mimo
  bool dosTapas = false;     // dos justas = despierta
  bool bocabajo = false;     // lleva un rato boca abajo = a dormir
  uint16_t pasos = 0;        // pasos nuevos desde la ultima lectura
  // Cuanta fuerza llevo el ultimo meneo, 0..255. Lo pide la captura: no
  // es lo mismo un golpecito que una sacudida de verdad.
  uint8_t fuerza = 0;
};

// La maquina de estados. Se le dan muestras (x,y,z en g y el reloj en ms)
// y los eventos del chip, y contesta con los gestos. No toca hardware: por
// eso entra en la bateria de pruebas.
class LectorGestos {
 public:
  void reset();
  // Una muestra del acelerometro. `ms` es millis().
  void muestra(float x, float y, float z, uint32_t ms);
  // El chip dice que hubo una tapa (1) o una doble (2). 0 = nada.
  void tapa(uint8_t cuantas, uint32_t ms);
  // Lee y CONSUME los gestos pendientes.
  Gestos leer();

  // Para los tests y para la pantalla de depuracion.
  float ultimoModulo() const { return modulo_; }
  uint8_t tapasSeguidas() const { return tapasN_; }

 private:
  Gestos pend_;
  float modulo_ = 1.0f;
  uint32_t ultMeneo_ = 0;
  uint32_t ultTapa_ = 0;
  uint8_t tapasN_ = 0;
  uint32_t bocabajoDesde_ = 0;
  bool bocabajoAvisado_ = false;
  uint32_t ultPaso_ = 0;
  // El lanzamiento se decide DESPUES del pico: se apunta cuando se
  // empuja y se confirma si llega el frenazo a tiempo.
  uint32_t empujonEn_ = 0;
  bool subiendo_ = false;
};
