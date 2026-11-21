#!/usr/bin/env bash
# Ejecuta toda la bateria (logica del firmware + herramientas + datos).
#   ./test/run_tests.sh          lo de siempre
#   ./test/run_tests.sh --asan   ademas con AddressSanitizer + UBSan
#   ./test/run_tests.sh --todo   ademas lo lento: sprites y cobertura
set -uo pipefail
cd "$(dirname "$0")"
RAIZ=".."

fail=0
modo="${1:-}"

echo "== logica del firmware (pet.cpp / i18n.cpp / dex.h / battle.cpp) =="
make --no-print-directory run || fail=1

# La INTERFAZ, en su propio binario: el sketch necesita los shims del
# simulador (pantalla, tactil, Wire) y chocan con los de test/shim.
# Pasa los 86 guiones de desktop/ con los invariantes puestos, que es
# lo que saco los .ino del 0% de cobertura.
echo
echo "== la interfaz: el sketch y los 86 guiones (make ui) =="
make --no-print-directory ui || fail=1

if [[ "$modo" == "--asan" ]]; then
  echo
  echo "== la misma bateria con AddressSanitizer + UBSan =="
  make --no-print-directory asan || fail=1
fi

# UBSan sin su biblioteca: es lo unico que corre en Windows, donde
# w64devkit no trae libasan ni libubsan. Se para en seco en la primera
# operacion indefinida. Barato (una compilacion mas) y le saca mucho
# partido al gauntlet, que son 4000 combates al azar.
if [[ "$modo" == "--todo" || "$modo" == "--ub" ]]; then
  echo
  echo "== la misma bateria cazando comportamiento indefinido =="
  make --no-print-directory ubtrap || fail=1
fi

echo
echo "== herramientas y ficheros generados (tools/) =="
python3 test_tools.py || fail=1

# Los DATOS contra el PBS del Anil: que cada bicho tenga SUS golpes, SUS
# habilidades y SU evolucion. Los tests de arriba comprueban que las
# tablas son coherentes; esto comprueba que dicen la verdad.
echo
echo "== los 1025 contra el PBS del Anil (tools/contraste.py) =="
python3 "$RAIZ/tools/contraste.py" || fail=1

# Y que los dos verificadores de datos sigan sabiendo fallar. Sin esto,
# un "sin fallos" no significa nada: ya ha pasado que un patron dejara
# de casar y el informe saliera limpio sin mirar una sola especie.
echo
echo "== los verificadores saben fallar (tools/sabotaje.py) =="
python3 "$RAIZ/tools/sabotaje.py" || fail=1

if [[ "$modo" == "--todo" ]]; then
  echo
  echo "== integridad de la tarjeta (tools/auditoria.py --hondo) =="
  python3 "$RAIZ/tools/auditoria.py" --hondo || fail=1
  # Las 86 sesiones del simulador: es la unica red que hay sobre los
  # .ino, que son dos tercios del codigo a mano. No mira pixeles, mira
  # que ninguna pantalla se caiga.
  echo
  echo "== las 86 sesiones del simulador (tools/simulacro.py) =="
  python3 "$RAIZ/tools/simulacro.py" || fail=1
  echo
  echo "== que efectos y habilidades no prueba nadie (cobertura) =="
  python3 "$RAIZ/tools/cobertura.py" || fail=1
fi

echo
if [[ $fail -eq 0 ]]; then
  echo "TODO OK"
else
  echo "HAY FALLOS"
fi
exit $fail
