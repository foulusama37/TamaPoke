#!/usr/bin/env python3
"""Mete las cadenas pt-BR de tools/pt_interfaz.py en i18n.cpp.

Antes de tocar nada comprueba dos cosas contra lo que ya hay, porque son
las dos formas en que esto puede romper en silencio:

  1. que sean EXACTAMENTE las mismas 121, en el mismo orden. Una de menos
     y todas las de despues se corren una posicion: cada etiqueta sale en
     el sitio de la siguiente y ninguna da error.
  2. que cada cadena conserve sus %s/%u/%d. Un printf con un formato de
     menos no avisa, lee basura de la pila.

  python3 tools/pon_pt.py --ver     # ensena lo que cambiaria y no escribe
  python3 tools/pon_pt.py
"""
import argparse
import io
import os
import re
import sys

AQUI = os.path.dirname(os.path.abspath(__file__))
RAIZ = os.path.dirname(AQUI)
sys.path.insert(0, AQUI)
import texto_pt  # noqa: E402
from pt_interfaz import UI  # noqa: E402

FMT = re.compile(r"%[-+ #0-9.]*(?:l|ll|h|hh)?[diouxXeEfgGcsp%]")


def firma(s):
    return [m.group(0) for m in FMT.finditer(s) if m.group(0) != "%%"]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--ver", action="store_true", help="no escribe nada")
    a = ap.parse_args()

    viejas = [texto_pt.descodifica(t) for t in texto_pt.bloques()["PT"]]
    if len(UI) != len(viejas):
        print("SON %d y tienen que ser %d. Las que faltan corren TODAS las "
              "demas una posicion y no da ningun error."
              % (len(UI), len(viejas)))
        # ensenar donde se desalinean, que es lo unico util aqui
        for i in range(min(len(UI), len(viejas))):
            if firma(UI[i]) != firma(viejas[i]):
                print("  se despegan en la %d: vieja %r / nueva %r"
                      % (i, viejas[i], UI[i]))
                break
        return 1

    malas = 0
    for i, (nueva, vieja) in enumerate(zip(UI, viejas)):
        if firma(nueva) != firma(vieja):
            print("  %3d FORMATO: %r tenia %s y ahora %s"
                  % (i, vieja, firma(vieja) or "ninguno",
                     firma(nueva) or "ninguno"))
            malas += 1
    if malas:
        print("%d cadenas con la firma cambiada: no se escribe nada" % malas)
        return 1

    cambiadas = [(i, v, n) for i, (n, v) in enumerate(zip(UI, viejas))
                 if n != v]
    print("%d de %d cadenas cambian" % (len(cambiadas), len(UI)))
    if a.ver:
        for i, v, n in cambiadas:
            print("  %3d  %s" % (i, v))
            print("       %s" % n)
        return 0

    # ---- a bytes de la fuente, cuatro por linea para que se pueda leer
    codificadas = [texto_pt.codifica(t, "ui[%d]" % i)
                   for i, t in enumerate(UI)]
    lineas = []
    for i in range(0, len(codificadas), 4):
        lineas.append("    " + " ".join('"%s",' % c
                                        for c in codificadas[i:i + 4]))
    cuerpo = "\n".join(lineas)

    P = os.path.join(RAIZ, "i18n.cpp")
    s = io.open(P, encoding="utf-8", errors="surrogateescape").read()
    ini = s.index("// ---------------- PT ----------------")
    abre = s.index("{", ini)
    cierra = s.index("\n  },", abre)
    s = s[:abre + 1] + "\n" + cuerpo + s[cierra:]
    io.open(P, "w", encoding="utf-8", errors="surrogateescape",
            newline="\n").write(s)
    print("i18n.cpp: bloque PT reescrito en pt-BR")
    for av in texto_pt.avisados:
        print("  aviso: %s" % av)
    return 0


if __name__ == "__main__":
    sys.exit(main())
