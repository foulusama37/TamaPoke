#!/usr/bin/env python3
"""Regenera TODO en el orden correcto.

Existe por un fallo real: al meter Hoenn se corrio gen_art.py antes que
gen_moves.py, y como gen_art lee moves.h para saber cuantos golpes hay, se
quedo con los 483 de antes. Resultado: 33 golpes (Water Gun, Waterfall,
Volt Tackle, X-Scissor, Wing Attack...) perdieron su animacion y volvian al
efecto generico de su tipo. Nada se cayo, nada aviso, y solo se vio porque
el barrido comparo FX_MOVE_COUNT con MOVE_COUNT.

Las dependencias que importan:
  dex_range.py          -> de donde sale N_DEX; se edita a mano
  anil_pbs.py           -> el lector del PBS del Anil, con la comprobacion
                           de que el fichero va en orden de dex nacional
  gen_filas.py          -> escribe las filas nuevas en dex_data.py (PBS)
  gen_stats/types/      -> tablas por especie; LEEN dex_range, no dex.h
    sizes/names
  gen_items.py          -> items.h, que define los EVOI_*
  gen_evos.py           -> evos.h; necesita items.h e dex_data.py
  gen_dex.py            -> dex.h; necesita TODAS las de arriba
  gen_moves.py          -> moves.h
  gen_tms.py            -> tms.h; LEE moves.h y guarda INDICES de golpe, asi
                           que quedarse atras no es perder una animacion:
                           es que las MT ensenan OTRO golpe. Paso justo eso
                           (286 MT apuntando a la tabla vieja de 667) por no
                           estar en esta lista. Ahora tms.h ademas se firma
                           con el MOVE_COUNT con el que se hizo, y
                           auditoria.py lo compara.
  gen_abilities.py      -> abilities.h
  gen_megas.py          -> megas.h; necesita items.h Y abilities.h
  gen_dexinfo.py        -> las fichas
  gen_art.py            -> art.h y /fx/fx.bin; LEE moves.h (va el ultimo)

  python3 tools/gen_todo.py            # todo menos lo que pide red
  python3 tools/gen_todo.py --red      # tambien stats y nombres (PokeAPI)
  python3 tools/gen_todo.py --filas 252 386
"""
import os
import subprocess
import sys
import time

AQUI = os.path.dirname(os.path.abspath(__file__))

# (script, necesita_red). El ORDEN de esta lista es la respuesta al fallo
# de arriba: no se toca sin mirar las dependencias del docstring.
PASOS = [
    # stats, tipos y alturas salen del PBS del Anil desde 2026-10-03: ya no
    # piden red. El unico que la sigue pidiendo es gen_names (los nombres
    # traducidos, que el PBS no trae).
    ("gen_stats.py", False),
    ("gen_types.py", False),
    ("gen_sizes.py", False),
    ("gen_names.py", True),
    ("gen_items.py", False),
    ("gen_evos.py", False),
    ("gen_dex.py", False),
    ("gen_moves.py", False),
    ("gen_tms.py", False),      # DESPUES de moves.h: guarda indices de golpe
    ("gen_abilities.py", False),
    # DESPUES de items y de abilities: una mega indexa la tabla de
    # habilidades y nombra su piedra, que salen de esos dos. Las formas,
    # por lo mismo.
    ("gen_forms.py", False),
    ("gen_megas.py", False),
    ("gen_dexinfo.py", False),
    ("gen_art.py", False),      # el ultimo: lee moves.h
]


def corre(script, *args):
    t0 = time.time()
    r = subprocess.run([sys.executable, os.path.join(AQUI, script), *args],
                       capture_output=True, text=True,
                       encoding="utf-8", errors="replace")
    tag = "ok " if r.returncode == 0 else "FALLO"
    print(f"  {tag} {script:<18s} {time.time() - t0:5.1f}s")
    if r.returncode != 0:
        print((r.stdout or "") + (r.stderr or ""))
    return r.returncode


def main():
    red = "--red" in sys.argv
    if "--filas" in sys.argv:
        i = sys.argv.index("--filas")
        desde, hasta = sys.argv[i + 1], sys.argv[i + 2]
        print(f"filas {desde}-{hasta} desde el PBS del Anil")
        if corre("gen_filas.py", desde, hasta, "-w"):
            return 1

    for script, pide_red in PASOS:
        if pide_red and not red:
            print(f"  --   {script:<18s} (pide red; --red para correrlo)")
            continue
        if corre(script):
            print(f"\nparado en {script}: lo de detras NO se ha regenerado, "
                  f"asi que hay ficheros de dos epocas mezclados.")
            return 1
    # Decir "empaquetar la tarjeta" a secas no basto: moves.h cambio de
    # 717 a 718 golpes, gen_art.py (que esta aqui) rehizo fx.bin y
    # pack_audio.py (que no esta) no se corrio, asi que 386 golpes
    # tocaban el sonido del siguiente. Los empaquetadores no entran en
    # esta lista porque tardan y piden el Anil entero, pero se NOMBRAN,
    # y tools/auditoria.py comprueba que se hayan corrido.
    print("\nahora: tests (test/), compilar, y empaquetar la tarjeta.")
    print("  si MOVE_COUNT cambio:  python tools/pack_audio.py golpes")
    print("     (los gNNN.raw van por fila de MOVE_TBL; sin rehacerlos")
    print("      cada golpe toca el sonido del de al lado)")
    print("  si megas.h/forms.h cambiaron:  python tools/pack_anil.py megas")
    print("  y luego:  python tools/auditoria.py   <- lo comprueba")
    return 0


if __name__ == "__main__":
    sys.exit(main())
