# TamaPoke — notas para Claude

Tamagotchi Pokemon en una Waveshare ESP32-S3-Touch-AMOLED-1.75 (466x466
redonda). Los datos (especies, golpes, habilidades, megas, formas, MT)
salen del PBS de **Pokemon Anil V4.13**, que esta rebalanceado: nunca
comparar stats ni efectos con los juegos oficiales, el PBS es la verdad.

## La toolchain NO esta en el PATH

Hay que exportarla en cada orden (la sesion no guarda el entorno):

```bash
export PATH="/c/Users/foulu/Downloads/_toolchain/w64devkit/bin:$PATH"
```

- `mingw32-make` y `g++` viven ahi. No hay `make`, es `mingw32-make`.
- arduino-cli: `/c/Users/foulu/Downloads/_toolchain/arduino/arduino-cli.exe`
- FQBN:
  `esp32:esp32:esp32s3:USBMode=default,CDCOnBoot=cdc,PSRAM=opi,FlashSize=16M,PartitionScheme=custom`
  `USBMode=default` es TinyUSB, y no es opcional: sin el, el nucleo ni
  compila `USBMSC.h` y `usbdisco.cpp` corta la compilacion con un `#error`.
  Es lo que deja al bicho ensenarse al PC como lector de tarjetas sin
  abrir la carcasa. El valor por defecto de la placa es el otro
  (`esp32s3.build.usb_mode=1`, "Hardware CDC and JTAG"), asi que hay que
  escribirlo SIEMPRE; omitirlo no da el valor de antes, da el `#error`.
- Particiones: `partitions.csv` de la carpeta del sketch (6,25 MB de app,
  NVS en 0x9000 igual que antes). `PartitionScheme=custom` solo sube el
  limite de tamano (que pasa a 16 MB: el arduino-cli YA NO avisa si la app
  pasa de 0x640000, mirar el tamano a mano). La primera vez en una placa
  con la tabla vieja hay que grabar TAMBIEN `0x8000 TamaPoke.ino.partitions.bin`.
  A cambio se pierde el JTAG por USB; el puerto serie sigue, ahora por
  TinyUSB CDC, y convive con la unidad de disco en el mismo cable.
- `mingw32-make asan` NO enlaza: w64devkit no trae libasan ni libubsan.
  El sustituto es `ubtrap` (`-fsanitize=undefined -fsanitize-undefined-trap-on-error`).

## La tarjeta por el USB-C

El bicho vive dentro de una pokebola cerrada: lo unico que asoma es el
USB-C. Sacar la microSD para actualizar sprites no es una molestia de una
vez, es la via de actualizacion para siempre.

- `usbdisco.cpp` presenta el ESP32-S3 como lector de tarjetas (USB MSC).
  El `static USBMSC msc` es global A PROPOSITO: su constructor registra
  la interfaz, y eso tiene que pasar antes de que `app_main()` llame a
  `USB.begin()`, o sea antes de `setup()`. Dentro de una funcion llega
  tarde y el PC no ve la unidad.
- Nace VACIO (`media_present = false`): el PC ve un lector sin tarjeta.
  Por eso no hace falta reiniciar para entrar en modo USB.
- Ordenes por consola, que llegan por el mismo cable: `USB`, `USB ON`,
  `USB OFF`. Lo normal es expulsar la unidad desde el PC: el bucle lo ve
  (`usbMscExpulsada()`), recupera la tarjeta y, si el PC escribio algo,
  guarda la partida y reinicia -- los indices en memoria (thumbs.bin, el
  sprite cargado) hablan de una tarjeta que ya no existe.
- **Dos escritores rompen el FAT.** La puerta unica es `sdUsable()`
  (`sdmon.h`) = `sdReady && !sdPrestada`. La seccion `usb` de
  `auditoria.py` comprueba que ningun `SD_MMC.*` se cuela sin pasar por
  ella, y mira la RAMA, no la funcion: la version que miraba la funcion
  entera daba por buenos `SDINFO` y `LS`, que leian la tarjeta sin
  mirar nada porque el `sdReady` que los salvaba estaba en otra rama.
- Velocidad **medida**: 0,25 MB/s (yo habia estimado ~1 MB/s por el Full
  Speed del USB y me pase cuatro veces). El cuello no es el USB:
  `SDMMCFS::writeRAW` hace `disk_write(..., 1)`, un sector por llamada, y
  cada 512 bytes paga el tiempo de programacion de la tarjeta (~2 ms).
  La tarjeta entera (~910 MB) es cerca de UNA HORA; unos sprites,
  segundos. Para el uso real da igual, y por eso sigue asi: batir los 8
  sectores de cada bufer de TinyUSB en una transaccion lo multiplicaria
  por 3 o 4, pero es el camino de ESCRITURA y un fallo ahi corrompe la
  tarjeta en silencio.

## La bateria completa, en orden

Esta en `/bateria`. Si se corre a mano, este es el orden y el porque:

```bash
export PATH="/c/Users/foulu/Downloads/_toolchain/w64devkit/bin:$PATH"
cd test && mingw32-make            # logica  (~800 tests, incluye 4000 combates)
mingw32-make ui                    # interfaz (el sketch con shims)
mingw32-make ubtrap                # lo mismo con el sanitizador de UB
cd ../desktop && mingw32-make      # el simulador de escritorio
cd .. && python tools/auditoria.py   # la tarjeta y los .h generados
python tools/contraste.py            # lo embarcado contra el PBS del Anil
python tools/sabotaje.py             # rompe a proposito: las guardas valen?
python tools/cobertura.py            # cada efecto se EJECUTA en la bateria?
/c/Users/foulu/Downloads/_toolchain/arduino/arduino-cli.exe compile \
  --fqbn "<FQBN de arriba>" .        # el firmware de verdad
```

El compilado del firmware tarda varios minutos: lanzarlo en segundo plano
y **no dar por bueno** lo que no haya terminado.

## Los .h generados NO se editan a mano

`dex.h dex_text.h items.h moves.h abilities.h art.h evos.h megas.h
forms.h tms.h female.h species.h`

Cada uno tiene su generador en `tools/`. **El orden importa** y esta en el
docstring de `tools/gen_todo.py`: `gen_art.py` y `gen_tms.py` LEEN
`moves.h`, asi que van detras de `gen_moves.py`. Si se quedan atras:

- `art.h` viejo = golpes sin animacion propia
- `tms.h` viejo = **las MT ensenan OTRO golpe** (guarda INDICES)

Las dos cosas pasaron de verdad. `auditoria.py` las vigila comparando
`FX_MOVE_COUNT` y `TM_MOVE_COUNT` con `MOVE_COUNT`.

## Las animaciones de golpe salen de PkmnAnimations, no de las hojas

`/fx/fx.bin` lo escribe `gen_art.py` leyendo
`Data/PkmnAnimations.rxdata` del Anil con `tools/fxanim.py`. Ese fichero
dice, golpe a golpe, QUE celda va en cada fotograma, donde, con que zoom,
giro, espejo y opacidad. **No se adivina nada.**

Antes se cogia la hoja PNG que le tocaba al golpe y se usaban "las
primeras N celdas no vacias". Eso no es la animacion:

- `PRAS- Slash.png` son TRECE animaciones distintas en una rejilla
- varias hojas traen al final el cartel de "Ripped and Composed By..."
  del que las extrajo — Scratch salia con tres fotogramas de ese cartel,
  letras sueltas en mitad del combate

Lo que hay que saber si se toca:

- la rejilla es **5 columnas de 192 px SIEMPRE**, sea cual sea el ancho
  del PNG (`pbSpriteSetAnimFrame`, script BattleAnimationPlayer)
- los indices de una celda estan en `class AnimFrame` del mismo script;
  `PATTERN` es el **7**, no el 0 (el 0 es X)
- `pattern` negativo no es un dibujo: -1 es el que ataca y -2 el que
  recibe. Los pintamos nosotros, asi que se saltan
- `PBAnimations` es una subclase de Array que deja el array base VACIO y
  guarda todo en la ivar `@array`. Leyendo solo el array base el fichero
  se parsea entero **sin dar error** y devuelve cero animaciones
- los fotogramas se recortan TODOS a la MISMA caja (la union). El reloj
  centra cada fotograma en el mismo punto: recortando cada uno a lo suyo
  el movimiento desaparece y una roca que vuela se queda temblando

El tope por golpe (`FX_MOVE_MAX_B`) no es por espacio — la tarjeta va
sobrada — sino por TIEMPO DE LECTURA: la SD da 0,18-0,25 MB/s medidos y
la animacion se lee justo antes de pintarla, con 850 ms de pausa del
anuncio como toda cobertura.

## Trampas que ya costaron tiempo

- **`pack_anil.py` sin argumentos reempaqueta los 1025 sprites: 20
  minutos.** Para una parte: `pack_anil.py megas` / `hembras` / `<numero>`.
  Un argumento que no reconoce lo canta (antes lo ignoraba en silencio y
  dejo las megas desfasadas una fila).
- **El heredoc de bash se come las barras.** `\\n` dentro de `<<'EOF'`
  colapsa y parte las cadenas C/C++ generadas, a veces sin error. Para
  codigo con barras: usar la herramienta de escritura, o construir la
  barra con `chr(92)`.
- **Leer la firma real antes de escribir contra ella.** En una sesion me
  invente `pet.addJoy()`, `pet.andar()`, `uiCorazon()`, `Pet::reset()` y
  un `configTap` de siete parametros (son ocho). Todas eran un `grep` de
  distancia. Los tests usan `makePet(p, dex)`, no `reset()`.
- **Insertar codigo por script cae en la llave equivocada.** Paso dos
  veces (el Disfraz dentro de la rama del golpe mortal; la cuenta del
  Cuerpo Mortal dentro de un `switch`, detras de un `break`). Despues de
  un parche por script, LEER el resultado en su contexto.
- **Nunca "arreglar" una cadena if/else con `if (false)`.** Lo hice y
  desactive el Sturdy, la Focus Sash y el Endure enteros.
- **No le pongas a un fichero de aqui el nombre de una cabecera del
  nucleo.** Esto se llamo `usbmsc.h`, y como Windows no distingue
  mayusculas y la carpeta del sketch va primera en la ruta de includes,
  el `#include <USBMSC.h>` de dentro resolvia a SI MISMO; el `#pragma
  once` lo dejaba en nada y el error salia como `'USBMSC' does not name
  a type`, con el MSC perfectamente activado. Ahora es `usbdisco.h`.
  Cuando un tipo "no existe" aunque sus macros esten puestas, sospecha
  del include antes que de la configuracion.
- **Renombrar un .cpp deja su .o viejo en la cache de arduino-cli.** No
  borra los huerfanos, asi que el `objs.a` sigue trayendo el objeto del
  fichero que ya no existe y TODAS sus funciones salen por duplicado
  ("multiple definition of ..."). Tras renombrar o borrar una fuente:

  ```bash
  D=~/AppData/Local/arduino/sketches/<hash-del-sketch>
  rm -f "$D/sketch/<viejo>."* "$D/sketch/objs.a"
  ```

  El hash sale en el propio mensaje del linker. Borrar la carpeta entera
  tambien vale, pero recompila el nucleo entero (minutos).

## Como se mide aqui

El fallo que mas se repite en este proyecto no es un calculo mal hecho:
es **una comprobacion que deja de comprobar**. Un patron que no casa
devuelve cero diferencias, y cero se lee igual que "todo bien". De ahi
las herramientas: antes de decidir, medir; antes de creer una
comprobacion, romperla a proposito.

- `tools/sin_efecto.py` — golpes de la tabla sin su efecto del PBS
- `tools/cuanto_hace.py` — cuanto del catalogo hace algo de verdad
- `tools/sin_habilidad.py` — especies (y MEGAS) que se quedan sin ninguna
  habilidad que funcione
- `tools/cobertura.py` — que cada codigo de efecto se EJECUTE en la
  bateria, con una cuenta que cuadra: si un codigo no sale en ninguna
  columna, lo dice
- `tools/sabotaje.py` — mete un error conocido en un `.h` y exige que la
  comprobacion lo cante, nombrando al culpable. **Si se anade una guarda,
  se anade su sabotaje.**

## Lo que no se puede probar sin la placa

`audio.cpp` (I2S/ES8311), `rtcbat.cpp` (RTC/PMU) e `imu.cpp`
(I2C/QMI8658) son puro driver. El patron del proyecto es sacar las
DECISIONES a un modulo sin hardware y probar ese: `gestos.cpp` decide que
es un meneo y que es un paso, y tiene su bateria; `imu.cpp` solo lee el
chip. Los shims de escritorio estan en `desktop/shim/shim.cpp` y
`test/shim/shim.cpp` — al anadir un driver hay que anadir su shim o el
simulador y la bateria de interfaz no enlazan.

## Ficheros temporales

`_claude_scratch/` para recortes y scripts de un rato. Nunca en la raiz.
