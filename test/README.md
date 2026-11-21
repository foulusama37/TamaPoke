# Tests

Bateria de tests que corre **en el PC, sin placa**: compila la logica pura del
firmware (`pet.cpp`, `i18n.cpp`, `dex.h`) contra unos shims de Arduino y la
ejecuta con un reloj y un `random()` falsos, asi que los resultados son
deterministas y tardan menos de un segundo.

```bash
./test/run_tests.sh           # lo de siempre (~10 s)
./test/run_tests.sh --asan    # ademas con AddressSanitizer + UBSan
./test/run_tests.sh --todo    # ademas lo lento: sprites de la SD y cobertura
```

O por partes:

```bash
cd test && make               # solo la logica del firmware
cd test && make FILTER=evolve # solo los tests con "evolve" en el nombre
cd test && make asan          # con sanitizers
python3 test/test_tools.py    # solo las herramientas de tools/
```

En Windows no hay `make` ni `g++` en el PATH; estan en
`C:\Users\foulu\Downloads\_toolchain\` (ver el README de la raiz).

## Las tres preguntas, y quien contesta cada una

Son preguntas distintas y hace falta una herramienta para cada una. La
primera columna es lo que uno se pregunta de verdad.

| La pregunta | Quien la contesta |
|---|---|
| ¿el codigo hace lo que digo que hace? | los tests de aqui (`make`) |
| ¿aguanta jugando de verdad? | `test_gauntlet.cpp`: 4000 combates al azar |
| ¿los DATOS son los del Anil? | `tools/contraste.py` |
| ¿esta todo en la tarjeta y la abre EL FIRMWARE? | `test_sd.cpp` |
| ¿los ficheros de la tarjeta estan bien formados? | `tools/auditoria.py` |
| ¿que efecto de golpe no prueba NADIE? | `tools/cobertura.py` |
| ¿se cae alguna pantalla? | `tools/simulacro.py`: las 86 sesiones |
| ¿hay comportamiento indefinido? | `make ubtrap` |
| ¿el dibujo es el que le toca a ese bicho? | `tools/hoja_contacto.py` (lo mira un ojo) |
| ¿y todo esto sabe fallar? | `tools/sabotaje.py` |

La ultima fila es la importante. En este proyecto el fallo que mas se
ha repetido no es una cuenta mal hecha: es una comprobacion que dejo de
comprobar y siguio diciendo "sin fallos". `sabotaje.py` mete un error
conocido en un `.h` y exige que salte.

## Cuanto queda sin probar, y por que

`tools/cobertura.py` lo dice con numeros en cada corrida. Hoy: **96%
del codigo escrito a mano entra en la bateria** — pet.cpp 96% de sus
lineas, battle.cpp 95%, i18n.cpp 100%, sdmon.cpp 61%, y **los cuatro
`.ino` al 71%**, que hasta hace nada eran un 0% redondo.

Lo que saco a los `.ino` de la oscuridad no fue escribir tests de
pixeles (eso cuesta caro y se rompe con cualquier retoque): fue meter
el sketch en un binario propio y pasarle **los 86 guiones que el
simulador ya tenia**, con los invariantes puestos despues de cada
comando. Esos guiones llevaban meses recorriendo todas las pantallas y
lo unico que hacian era dibujar PNG que miraba un ojo cuando se
acordaba.

Queda fuera un 3%: `audio.cpp` (I2S) y `rtcbat.cpp` (I2C/PMU). Son
driver puro y sin la placa delante no hay nada que afirmar de ellos.

## En Windows

No hay `make` ni `g++` en el PATH; estan en
`C:\Users\foulu\Downloads\_toolchain\`. Y **w64devkit no trae libasan
ni libubsan**, asi que `make asan` no enlaza: alli se usa
`make ubtrap`, que caza el comportamiento indefinido sin bibliotecas
(se para en seco en vez de imprimir un informe).

## Que se cubre

| Fichero | Que comprueba |
|---|---|
| `test_pet.cpp` | huevo y eleccion de inicial, comida, tick de estadisticas, sueno, descuidos, niveles, evolucion (incluida la rama de Eevee), formas regionales, stats de combate, entrenamiento y gimnasio, racha y vinculo, medallas, ceremonias, progresion offline y guardado en NVS |
| `test_sd.cpp` | **la tarjeta leida por el firmware**: las 1025 especies, 127 megas y 61 formas abiertas con `PmdMon::load`, los shinys, las animaciones propias de los golpes (`/fx/fx.bin`), los fondos y las miniaturas. No es lo mismo que `auditoria.py`: alli el parseador es Python, aqui es el del aparato |
| `test_gauntlet.cpp` | **la maquina jugando**: 4000 combates al azar entre las 1025 y una vida entera del bicho minuto a minuto. No afirma que pase nada concreto; afirma lo que nunca puede pasar (vida fuera de rango, stage fuera de -6..+6, un golpe que cura al rival, un combate que no termina) |
| `test_battle.cpp` | la formula de dano, criticos y precision, los estados (y que un GOLPE los provoque, no solo `applyStatus`), los stages, clima y terreno, los golpes de varios turnos, objetos, habilidades, obediencia, megas y captura |
| `test_dex.cpp` | integridad de la Pokedex: nombres ASCII, cadenas evolutivas sin bucles, toda especie alcanzable, ninguna linea imposible de completar, limites de los buffers de la UI |
| `test_i18n.cpp` | los 6 idiomas completos, sin acentos (la fuente GFX es ASCII), y con los mismos `%u`/`%s` que le pasa el sketch en cada llamada |
| `test_tools.py` | los scripts de `tools/` compilan, los datos de la Pokedex son coherentes y `dex.h` sigue coincidiendo con lo que genera `tools/gen_dex.py` |

## Como esta montado

- `shim/Arduino.h` + `shim/Preferences.h` + `shim/shim.cpp`: lo minimo de Arduino
  que usan `pet.cpp` e `i18n.cpp`. El reloj (`millis()`) y `random()` son falsos
  y controlables; `Preferences` es un mapa en memoria que imita la NVS, asi que
  se puede simular un reinicio creando otro `Pet` y llamando a `begin()`.
- `framework.h` + `main.cpp`: micro framework propio, sin dependencias.
- Los tests marcados con `TEST_KNOWN_ISSUE` describen el comportamiento
  **correcto** de algo que hoy falla: no rompen la bateria, salen listados
  aparte, y avisan si algun dia empiezan a pasar (para poder quitar la marca).

## Que NO se cubre

Todo lo que necesita la placa: pantalla y tactil (`TamaPoke.ino`), audio
(`audio.cpp`), tarjeta SD (`sdmon.cpp`) y RTC/bateria (`rtcbat.cpp`). El
`.ino` tampoco se compila aqui: hace falta `arduino-cli` con el core de ESP32.
