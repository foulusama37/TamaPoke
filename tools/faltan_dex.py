#!/usr/bin/env python3
"""Que fichas de Pokedex faltan por traducir, con su espanol al lado.

Mismo reparto que tools/faltan_pt.py para los golpes: la fuente es el
ESPANOL del PBS (el ingles del .dat ya es traduccion suya), y sale por
lotes para poder ir metiendolos en tools/pt_dex.py y comprobar con
`python tools/gen_dexinfo.py` cuantas van.

  python3 tools/faltan_dex.py            # cuantas faltan
  python3 tools/faltan_dex.py --lote 0   # los primeros, en TSV
"""
import argparse
import os
import sys

AQUI = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, AQUI)
import gen_dexinfo  # noqa: E402
from pt_dex import FICHAS  # noqa: E402
from dex_range import N_DEX  # noqa: E402


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--lote", type=int, default=None)
    ap.add_argument("--tam", type=int, default=115)
    a = ap.parse_args()

    es = gen_dexinfo.pokedex_en_espanol()
    nombres = gen_dexinfo.nombres_de_dex_h()
    faltan = [d for d in range(1, N_DEX + 1) if d not in FICHAS]
    print("%d traducidas, %d faltan de %d"
          % (len(FICHAS), len(faltan), N_DEX), file=sys.stderr)
    if a.lote is None:
        print("%d lotes de %d" % ((len(faltan) + a.tam - 1) // a.tam, a.tam),
              file=sys.stderr)
        return 0

    for d in faltan[a.lote * a.tam:(a.lote + 1) * a.tam]:
        nom = nombres[d] if d < len(nombres) else "?"
        print("%d\t%s\t%s" % (d, nom, es.get(d, "")))
    return 0


if __name__ == "__main__":
    sys.exit(main())
