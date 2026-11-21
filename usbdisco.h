#pragma once
#include <Arduino.h>

// OJO CON EL NOMBRE DEL FICHERO. Esto se llamaba usbmsc.h, y en Windows
// el sistema de ficheros no distingue mayusculas: el <USBMSC.h> del
// nucleo resolvia a ESTE fichero, que ya estaba incluido, y el #pragma
// once lo dejaba en nada. El resultado era "USBMSC does not name a
// type" con el MSC perfectamente activado en el sdkconfig. No le pongas
// a un fichero de aqui el nombre de una cabecera del nucleo.

// ---------------------------------------------------------------------
// La tarjeta por el USB-C, sin abrir la carcasa.
//
// El bicho va a vivir dentro de una pokebola cerrada: lo unico que
// asoma es el USB-C. Sacar la microSD para actualizar los sprites no es
// una molestia de una vez, es LA via de actualizacion para siempre, y
// por eso esto existe.
//
// Como funciona: el ESP32-S3 se presenta al PC como un LECTOR DE
// TARJETAS (USB Mass Storage). El objeto USBMSC se declara global a
// proposito -- su constructor llama a tinyusb_enable_interface(), y eso
// tiene que pasar ANTES de que el nucleo arranque el USB, cosa que hace
// en app_main(), antes de setup() (cores/esp32/main.cpp). Declararlo
// dentro de una funcion llegaria tarde y el PC no veria la unidad.
//
// Y nace VACIO: media_present empieza en false, asi que el PC ve un
// lector sin tarjeta dentro. Solo cuando se presta de verdad aparece la
// unidad. Eso resuelve el problema de los dos escritores sin tener que
// reiniciar para entrar: ver sdUsable() en sdmon.h.
//
// Velocidad MEDIDA con la placa delante: 0,25 MB/s. (Yo habia estimado
// ~1 MB/s por el Full Speed del USB; la estimacion estaba 4 veces por
// encima.) El cuello no es el USB: SDMMCFS::writeRAW escribe UN sector
// por llamada (disk_write(..., 1)), y cada bloque de 512 bytes paga
// entero el tiempo de programacion interno de la tarjeta, ~2 ms. 512
// sectores por segundo son justo los 0,25 MB/s que se miden.
//
// En la practica: los ~910 MB de la tarjeta entera son cerca de UNA
// HORA; cambiar unos cuantos sprites, segundos. Para el uso real
// (retocar sprites sin abrir la carcasa) da igual.
//
// Se podria multiplicar por 3 o 4 escribiendo los 8 sectores que trae
// cada bufer de TinyUSB en una sola transaccion (sdmmc_write_sectors, o
// disk_write con count>1; _card y _pdrv son protected en SDMMCFS, se
// llega con un using). No esta hecho: es el camino de ESCRITURA de la
// tarjeta y un fallo ahi corrompe el sistema de ficheros en silencio.
// ---------------------------------------------------------------------

// Prepara el lector (tamanos y callbacks). Va en setup(), DESPUES de
// sdBegin(), porque necesita saber cuantos sectores tiene la tarjeta.
// Sin tarjeta no hace nada y devuelve false.
bool usbMscBegin();

// Mete (true) o saca (false) "la tarjeta" del lector virtual. Prestarla
// pone sdPrestada: a partir de ahi el firmware no abre un solo fichero.
// Devuelve false si no hay tarjeta o si usbMscBegin() no cuajo.
bool usbMscPrestar(bool prestar);

// true mientras el PC tiene la tarjeta.
bool usbMscPrestada();

// Cuantos sectores ha ESCRITO el PC desde que se presto. Sirve para
// saber si hace falta reiniciar al devolverla (si no escribio nada, lo
// que hubiera en memoria sigue valiendo).
uint32_t usbMscEscritos();

// true si el PC hizo "expulsar". Es la senal limpia de que ya termino.
bool usbMscExpulsada();

// Ordenes de consola: "USB", "USB ON", "USB OFF". true si la maneja.
bool usbMscSerialCommand(const String &line);
