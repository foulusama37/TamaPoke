// El interprete de los guiones de sesion (desktop/*.txt).
//
// Vive aparte porque lo usan DOS programas: el simulador, que vuelca
// fotogramas a disco, y la bateria de tests de interfaz
// (test/test_ui.cpp), que pasa los mismos 86 guiones pero con las
// comprobaciones puestas.
//
// Reutilizarlos es lo que hace que los .ino dejen de estar a oscuras:
// esos guiones ya recorren elegir inicial, pelear, la ficha, la bolsa,
// el gimnasio, capturar y la Pokedex, y estaban escritos desde hace
// meses sin que nada mirase si algo se rompia por el camino.
#pragma once
#include <stdint.h>
#include <functional>
#include <string>

// Qué hacer entre comando y comando. La bateria de tests mete aqui sus
// comprobaciones; el simulador no pone nada.
using TrasCadaPaso = std::function<void(const std::string &linea)>;

// Corre un guion. `avanza(ms)` lo pone quien llama, porque el simulador
// y los tests cuentan los fotogramas de forma distinta.
// Devuelve false si el fichero no se puede abrir.
bool corriendoGuion(const std::string &path,
                    const std::function<void(uint32_t)> &avanza,
                    const std::function<void(const char *)> &captura,
                    const TrasCadaPaso &despues = nullptr);
