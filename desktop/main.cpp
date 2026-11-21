// Driver del simulador: llama a setup()/loop() del firmware de verdad y va
// volcando la pantalla a PPM. Los toques y el paso del tiempo vienen de un
// guion en texto, para poder repetir una sesion identica y comparar capturas.
//
//   tamapoke_sim --script sesion.txt --out frames --every 3
//
// Comandos del guion (uno por linea, # = comentario):
//   wait <ms>            deja correr el tiempo
//   tap <x> <y>          toque corto
//   hold <x> <y> <ms>    mantener pulsado
//   swipe <x0> <y0> <x1> <y1> <ms>
//   serial <linea>       manda un comando a la consola serie (SPEC, LVL, ...)
//   epoch <unix>         fija la hora del RTC (para ver amanecer/noche)
//   shot <nombre>        guarda el frame actual con ese nombre
//
// Opciones:
//   --nvs <fichero>   recuerda la partida entre ejecuciones. Sin esto,
//                     en el PC cada arranque empieza por el profesor
//                     Oak: la "memoria" vive en RAM y muere con el
//                     proceso. En la placa no pasa — alli es flash.
//   --borrar          arranca de cero aunque haya fichero de memoria
#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <Preferences.h>     // mockNvsLoad/Save: la memoria a disco
#include <TouchDrvCSTXXX.hpp>

// El interprete de guiones vive aparte: lo comparte la bateria de
// tests de interfaz, que pasa los mismos 86 ficheros con las
// comprobaciones puestas.
#include "guion.h"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

void setup();
void loop();
// El bicho vive en el sketch; aqui solo hace falta para poder volcar su
// guardado pendiente antes de cerrar (ver el final de main).
#include "../pet.h"
extern Pet pet;
void mockSdRoot(const char *path);
void mockSetEpoch(uint32_t e);

static std::string g_out = "frames";
static int g_every = 3;      // guarda 1 de cada N flush()
static uint32_t g_saved = 0;
static std::string g_forceName;

// el firmware llama a gfx->flush() al final de cada frame
void simOnFlush(Arduino_Canvas *c) {
  char path[512];
  if (!g_forceName.empty()) {
    snprintf(path, sizeof(path), "%s/%s.ppm", g_out.c_str(), g_forceName.c_str());
    c->savePPM(path);
    g_forceName.clear();
    return;
  }
  if (g_every <= 0) return;
  if ((c->frames() - 1) % (uint32_t)g_every) return;
  snprintf(path, sizeof(path), "%s/f%05u.ppm", g_out.c_str(), g_saved++);
  c->savePPM(path);
}

// Avanza el firmware: cada vuelta son ~16 ms de reloj falso, parecido al ritmo
// real de refresco. No se puede llamar a loop() sin avanzar millis() porque el
// planificador del bicho y las animaciones se mueven con el reloj.
static void run(uint32_t ms) {
  const uint32_t PASO = 16;
  for (uint32_t t = 0; t < ms; t += PASO) {
    mockAdvanceMillis(PASO);
    mockTouchTick();
    loop();
  }
}

int main(int argc, char **argv) {
  std::string script;
  std::string sd = "../tools/sdcard";
  // La "memoria" del aparato. Por omision NO se guarda, porque los 86
  // guiones de la bateria tienen que arrancar siempre igual: un guion
  // que dependa de lo que dejo el anterior deja de ser repetible.
  // Para jugar a mano se pasa --nvs y entonces si se acuerda.
  std::string nvs;
  bool borrar = false;
  for (int i = 1; i < argc; i++) {
    std::string a = argv[i];
    auto val = [&](const char *def) { return (i + 1 < argc) ? argv[++i] : def; };
    if (a == "--script") script = val("");
    else if (a == "--out") g_out = val("frames");
    else if (a == "--every") g_every = atoi(val("3"));
    else if (a == "--sd") sd = val("../tools/sdcard");
    else if (a == "--nvs") nvs = val("build/nvs.bin");
    else if (a == "--borrar") borrar = true;
  }
  mockSdRoot(sd.c_str());
  if (!nvs.empty() && !borrar && mockNvsLoad(nvs.c_str()))
    printf("[sim] memoria cargada de %s\n", nvs.c_str());

#ifdef _WIN32
  std::string mk = "mkdir \"" + g_out + "\" 2>nul";
#else
  std::string mk = "mkdir -p '" + g_out + "'";
#endif
  if (system(mk.c_str())) { /* ya existia */ }

  printf("[sim] SD = %s\n[sim] frames -> %s (1 de cada %d)\n", sd.c_str(), g_out.c_str(), g_every);
  mockSetMillis(0);
  setup();

  if (script.empty()) {
    run(20000);  // sin guion: 20 s de vida del bicho
  } else if (!corriendoGuion(script, run,
                             [](const char *n) { g_forceName = n; })) {
    fprintf(stderr, "no puedo abrir %s\n", script.c_str());
    return 1;
  }
  // Lo ultimo: dejar la memoria en disco, para que el proximo arranque
  // siga donde este se quedo (como la placa, que no se olvida).
  if (!nvs.empty()) {
    // ANTES de volcar: el juego no graba en cada cambio (escribir en
    // flash congela los dos nucleos casi un segundo), sino cuando algo
    // lo marca. Cerrar el simulador es un apagado limpio, asi que se
    // guarda SIN preguntar: con flushSave() se escribia solo lo que
    // estuviera marcado y se perdia, por ejemplo, todo lo hecho con
    // los comandos de depuracion. En la placa esto no hace falta,
    // porque el loop sigue y el autoguardado acaba pasando.
    pet.saveNow();
    mockNvsSave(nvs.c_str());
    printf("[sim] memoria guardada en %s\n", nvs.c_str());
  }
  printf("[sim] %u frames guardados\n", g_saved);
  return 0;
}
