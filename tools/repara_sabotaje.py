#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Devolve ao codigo os defeitos que uma sabotagem deixou para tras.

A lista de sabotagens guarda os DOIS textos —— o bom e o do defeito ——,
entao desfazer e trocar um pelo outro. Nao ha adivinhacao.

So mexe no que verifica_sabotaje.py apontou como sujo: arquivo onde o
texto do defeito esta e o bom nao.

    python tools/repara_sabotaje.py          (diz o que faria)
    python tools/repara_sabotaje.py --faz    (faz)
"""
import io
import os
import sys

AQUI = os.path.dirname(os.path.abspath(__file__))
RAIZ = os.path.dirname(AQUI)
sys.path.insert(0, AQUI)

import sabotaje_held                                  # noqa: E402


def main():
    faz = "--faz" in sys.argv
    n = 0
    for arq, bueno, malo, teste, desc in sabotaje_held.SABOTAJES:
        ruta = os.path.join(RAIZ, arq)
        s = io.open(ruta, encoding="utf-8", errors="surrogateescape").read()
        if s.count(bueno) or not s.count(malo):
            continue
        if s.count(malo) != 1:
            print("PULO %s (%s): o texto do defeito aparece %d vezes"
                  % (arq, teste, s.count(malo)))
            continue
        n += 1
        print("%s %s: %s" % ("reparo" if faz else "repararia", arq, desc))
        if faz:
            io.open(ruta, "w", encoding="utf-8", errors="surrogateescape",
                    newline="\n").write(s.replace(malo, bueno))
    if not n:
        print("nada sujo.")
    elif not faz:
        print()
        print("%d para reparar. Corre outra vez com --faz." % n)
    return 0


if __name__ == "__main__":
    sys.exit(main())
