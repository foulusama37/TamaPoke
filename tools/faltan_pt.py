#!/usr/bin/env python3
"""Que golpes faltan por traducir, con su espanol al lado.

El espanol del PBS es la FUENTE, no el ingles: el ingles del .dat del
Anil ya es una traduccion de ese espanol, asi que traducir desde el
ingles seria pasar por dos filtros. Ademas el espanol es mas corto, que
es lo que conviene en una tarjeta de 25 columnas.

Se saca por lotes para poder ir traduciendo y comprobando: cada lote se
mete en tools/pt_golpes.py y `python tools/gen_moves.py` dice cuantas van.

  python3 tools/faltan_pt.py            # cuantas faltan
  python3 tools/faltan_pt.py --lote 0   # los primeros 120, en TSV
  python3 tools/faltan_pt.py --lote 1 --tam 150
"""
import argparse
import io
import os
import re
import sys

AQUI = os.path.dirname(os.path.abspath(__file__))
RAIZ = os.path.dirname(AQUI)
sys.path.insert(0, AQUI)
from pt_golpes import GOLPES  # noqa: E402

PBS = os.environ.get(
    "ANIL_PBS", r"C:\Users\foulu\Downloads\Pokemon Anil V4.13\PBS")
COMILLA = chr(34)


def descripciones_pbs():
    """slug -> Description del PBS (en espanol)."""
    out, cur = {}, None
    for ln in io.open(os.path.join(PBS, "moves.txt"), encoding="utf-8",
                      errors="replace"):
        ln = ln.strip()
        m = re.match(r"^\[(.+)\]$", ln)
        if m:
            cur = m.group(1)
            continue
        if cur and ln.startswith("Description"):
            out[cur] = ln.split("=", 1)[1].strip()
    return out


def embarcados():
    """Los slugs de MOVE_TBL, en orden de fila."""
    s = io.open(os.path.join(RAIZ, "moves.h"), encoding="utf-8",
                errors="surrogateescape").read()
    i = s.index("static const MoveEntry MOVE_TBL[MOVE_COUNT]")
    j = s.index("};", i)
    return re.findall(r"^\s*\{\s*" + COMILLA + "([^" + COMILLA + "]*)"
                      + COMILLA, s[i:j], re.M)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--lote", type=int, default=None)
    ap.add_argument("--tam", type=int, default=120)
    a = ap.parse_args()

    pbs = descripciones_pbs()
    faltan = [m for m in embarcados() if m not in GOLPES]
    print("%d traducidos, %d faltan de %d"
          % (len(GOLPES), len(faltan), len(embarcados())), file=sys.stderr)
    if a.lote is None:
        print("%d lotes de %d" % ((len(faltan) + a.tam - 1) // a.tam, a.tam),
              file=sys.stderr)
        return 0

    trozo = faltan[a.lote * a.tam:(a.lote + 1) * a.tam]
    sin = 0
    for m in trozo:
        es = pbs.get(m, "")
        if not es:
            sin += 1
        print("%s\t%s" % (m, es))
    if sin:
        print("(%d sin espanol en el PBS)" % sin, file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
