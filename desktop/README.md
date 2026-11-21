# Simulador de escritorio

Corre el firmware **de verdad** en el PC y vuelca la pantalla a imagenes. No es
una maqueta: `TamaPoke.ino`, `pet.cpp`, `i18n.cpp` y `sdmon.cpp` se compilan tal
cual. Lo unico sustituido son los drivers.

| | |
|---|---|
| Real | toda la logica, todo el dibujo, el parseo de TPK2, la consola serie, la NVS |
| Simulado | pantalla (framebuffer propio), tactil, RTC/PMU, audio, SD (una carpeta) |

## Uso

```
make                 # compila
make run             # corre sesion.txt y guarda los frames
make gif             # ademas convierte a PNG + GIF (necesita Pillow)

make run SCRIPT=otra.txt EVERY=2 OUTDIR=build/otra
```

Requiere `g++` y `make` (en Windows vale w64devkit) y Python con Pillow para el
paso de las imagenes.

## El guion de sesion

Un comando por linea, `#` es comentario:

```
wait <ms>                       deja correr el tiempo
tap <x> <y>                     toque corto
hold <x> <y> <ms>               mantener pulsado (p. ej. el dialogo de soltar)
swipe <x0> <y0> <x1> <y1> <ms>  gesto
serial <linea>                  comando de la consola (SPEC 133, LVL 12, EGGS...)
epoch <unix>                    fija la hora del RTC, para ver amanecer/noche
shot <nombre>                   guarda el frame actual como <nombre>.ppm
```

Como el reloj es falso y `random()` es un LCG con semilla fija, **la misma
sesion da exactamente los mismos frames**: se pueden comparar capturas entre
cambios para ver que se movio.

### Las sesiones que hay

| Guion | Que enseña |
|---|---|
| `sesion.txt` | el recorrido normal: huevo, cuidados, evolucion |
| `combate.txt` | un combate de principio a fin y la tarjeta de golpes |
| `estados.txt` | quemadura, veneno, sueño y compania |
| `chip.txt` | el chip que rota entre el estado y los stages |
| `efeitos.txt` | drenaje, retroceso, multigolpe |
| `multiturno.txt` | agarre, furia, recarga y carga (los de varios turnos) |

### Coordenadas utiles

Pantalla redonda de 466x466, centro en (233, 233).

- Filas del inicial: y = 145 / 223 / 301 (x de 70 a 396)
- Arco de botones (lado 52): comer (140,390), jugar (202,404), luz (264,404), bano (326,390)
- El bicho: suelo en y=304, pasea entre x=150 y x=325

## Como esta montado

- `shim/` — Arduino, Print, String, Preferences (NVS en memoria), Wire, FS/SD_MMC
  (sobre una carpeta), tactil, y un `Arduino_GFX_Library.h` propio.
- `shim/glcdfont.h` — la tabla de la fuente 5x7 **extraida de la libreria real**,
  para que el texto caiga en los mismos pixeles que en la placa.
- `tools/gen_sketch.py` — el `.ino` no es C++ valido tal cual: el IDE de Arduino
  le inserta los prototipos antes de compilar. Esto hace lo mismo.
- `main.cpp` — llama a `setup()`/`loop()` avanzando el reloj 16 ms por vuelta.

### Por que no se reusa el Arduino_GFX real

Su `Arduino_DataBus.h` arrastra headers del ESP-IDF (`soc/*`, `hal/*`,
`esp_lcd_*`), y como el include va entre comillas, `Arduino_GFX.cpp` siempre
encuentra el suyo antes que cualquier stub. Salia mas barato reimplementar los
~20 primitivos que usa el firmware.

### Detalle que costo encontrar

El firmware solo lee el tactil cuando la ISR del pin INT levanta su bandera
(`gTouchIrq`). Con un `attachInterrupt` vacio los toques no llegaban nunca y la
sesion se quedaba clavada en la pantalla del inicial. El shim guarda el handler
y lo dispara mientras haya dedo apoyado, que es lo que hace el CST9217.

## Lo que NO cubre

Audio (solo imprime el efecto en el log), transferencia de ficheros por USB
(`PUT` corta por timeout), y el comportamiento electrico del AXP2101. Para eso
hace falta la placa.
