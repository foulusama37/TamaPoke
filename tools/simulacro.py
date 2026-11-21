#!/usr/bin/env python3
"""Pasa TODAS las sesiones del simulador y exige que ninguna se caiga.

Por que existe: los .ino son dos tercios del codigo escrito a mano y no
tienen ni un test. Son pantalla y toques, y afirmar sobre un pixel es
caro y se rompe con cualquier retoque de diseno. Pero hay algo que si
se puede afirmar barato y que vale mucho: **que no se caen**.

En desktop/ hay 86 guiones de sesion escritos a lo largo del proyecto
(elegir inicial, pelear, abrir la ficha, la bolsa, el gimnasio,
capturar, la Pokedex...). Cada uno recorre pantallas de verdad, con el
firmware de verdad, contra la tarjeta de verdad. Pasarlos todos y mirar
el codigo de salida convierte 7500 lineas sin red en 7500 lineas con
una red fina: no dice que se vean bien, dice que no revientan.

Lo que NO hace, y conviene tenerlo claro: no mira ni un pixel. Que la
pantalla sea la correcta lo ve un ojo en los PNG de desktop/frames_*.

  python3 tools/simulacro.py              # todas
  python3 tools/simulacro.py combate      # las que llevan eso en el nombre
  python3 tools/simulacro.py --sin-compilar
"""
import glob
import io
import os
import shutil
import subprocess
import sys
import tempfile

AQUI = os.path.dirname(os.path.abspath(__file__))
RAIZ = os.path.dirname(AQUI)
DESK = os.path.join(RAIZ, "desktop")
BIN = os.path.join(DESK, "build",
                   "tamapoke_sim.exe" if os.name == "nt" else "tamapoke_sim")
SD = os.path.join(RAIZ, "tools", "sdcard")


def compila():
    make = "mingw32-make" if os.name == "nt" else "make"
    r = subprocess.run([make], cwd=DESK, capture_output=True, text=True)
    if r.returncode != 0:
        print(r.stdout[-3000:])
        print(r.stderr[-3000:])
        raise SystemExit("el simulador no compila")


def main():
    filtro = next((a for a in sys.argv[1:] if not a.startswith("-")), "")
    if "--sin-compilar" not in sys.argv:
        print("compilando el simulador...")
        compila()
    if not os.path.exists(BIN):
        raise SystemExit(f"no esta {BIN}")

    guiones = sorted(glob.glob(os.path.join(DESK, "*.txt")))
    guiones = [g for g in guiones if filtro in os.path.basename(g)]
    if not guiones:
        raise SystemExit("ninguna sesion con ese nombre")

    tmp = tempfile.mkdtemp(prefix="simulacro_")
    bien, mal = 0, []
    for g in guiones:
        nom = os.path.basename(g)
        salida = os.path.join(tmp, nom[:-4])
        os.makedirs(salida, exist_ok=True)
        try:
            r = subprocess.run(
                [BIN, "--script", g, "--out", salida, "--every", "40",
                 "--sd", SD],
                capture_output=True, text=True, timeout=180, cwd=DESK)
        except subprocess.TimeoutExpired:
            mal.append(f"{nom}: se quedo colgada (mas de 3 min)")
            continue
        if r.returncode != 0:
            # Un fallo de verdad: se cayo a mitad de la sesion.
            cola = (r.stdout + r.stderr).strip().splitlines()
            mal.append(f"{nom}: salio con codigo {r.returncode}" +
                       (f" — {cola[-1][:120]}" if cola else ""))
            continue
        # Y que haya PINTADO algo: una sesion que arranca y se va sin
        # dibujar un fotograma "pasa" sin haber recorrido nada.
        if not os.listdir(salida):
            mal.append(f"{nom}: acabo bien y no pinto ni un fotograma")
            continue
        bien += 1

    shutil.rmtree(tmp, ignore_errors=True)
    print()
    for m in mal:
        print(f"  !  {m}")
    print(f"\n{bien} sesiones del simulador pasan, {len(mal)} no "
          f"(de {len(guiones)})")
    return 1 if mal else 0


if __name__ == "__main__":
    sys.exit(main())
