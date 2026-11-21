#!/usr/bin/env python3
"""Cruza CADA golpe de moves.h com o FunctionCode que o Anil lhe da.

A pergunta e uma so: o Anil faz alguma coisa com este golpe que o nosso
motor NAO faz? Sai uma lista por FunctionCode, com quantos golpes o usam
e o que gen_moves fez dele (fx/sx). Os que caem em FX_NONE + SX_NONE com
um codigo que nao e de "so dano" sao efeito do Anil jogado fora.

  python tools/audita_golpes_anil.py            # resumo
  python tools/audita_golpes_anil.py --todos    # cada golpe
"""
import collections
import io
import os
import re
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PBS = r"C:\Users\foulu\Downloads\Pokemon Anil V4.13\PBS\moves.txt"

# Codigos que so dao dano: nada a fazer alem de pegar.
SO_DANO = {"", "None"}


def pbs():
    out, cur = {}, None
    for ln in io.open(PBS, encoding="utf-8-sig", errors="replace"):
        ln = ln.strip()
        m = re.match(r"^\[([A-Z0-9]+)\]$", ln)
        if m:
            cur = out.setdefault(m.group(1), {})
            continue
        if cur is not None and "=" in ln:
            k, v = ln.split("=", 1)
            cur[k.strip()] = v.strip()
    return out


def tabla():
    t = io.open(os.path.join(RAIZ, "moves.h"), encoding="utf-8").read()
    c = t[t.index("MOVE_TBL[MOVE_COUNT]"):]
    return re.findall(r'^\s*\{\s*"([A-Z0-9]+)",\s*T_\w+,\s*(\d+),\s*\d+,\s*\d+,'
                      r'\s*(FX_\w+),\s*(SX_\w+)', c, re.M)


def main():
    p = pbs()
    porcod = collections.defaultdict(list)
    for nom, pot, fx, sx in tabla():
        if nom == "STRUGGLE":
            continue
        cod = p.get(nom, {}).get("FunctionCode", "?")
        porcod[cod].append((nom, int(pot), fx, sx))
    ignorados = []
    for cod, gs in sorted(porcod.items()):
        fxs = collections.Counter((fx, sx) for _, _, fx, sx in gs)
        nada = all(fx == "FX_NONE" and sx == "SX_NONE" for _, _, fx, sx in gs)
        if nada and cod not in SO_DANO:
            ignorados.append((cod, gs))
        if "--todos" in sys.argv:
            print(f"{cod:45s} {len(gs):3d}  " +
                  ", ".join(f"{fx}/{sx}" for (fx, sx) in fxs))
    print(f"\n{len(ignorados)} FunctionCodes do Anil que o motor IGNORA "
          f"(FX_NONE + SX_NONE): {sum(len(g) for _, g in ignorados)} golpes")
    for cod, gs in ignorados:
        print(f"  {cod:45s} " + ", ".join(n for n, *_ in gs))


if __name__ == "__main__":
    main()
