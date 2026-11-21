#!/usr/bin/env python3
"""Compara el learnset del port con el del Anil, especie a especie.

Nace de una sospecha del dueno jugando: "peguei um mewtwo nv 45... que
golpe ele ja deveria ter? blastoise lvl 45 tb... to com a sensacao que
ta faltando golpes".

La LOGICA de elegir (los cuatro ultimos aprendidos hasta ese nivel) ya
estaba bien; lo que habia que mirar es el DATO. Aqui se reconstruye la
lista del PBS (Moves = nivel,GOLPE,nivel,GOLPE...) y se pone al lado de
LEARN_LVL/LEARN_MV de moves.h.

  python tools/chequea_learnset.py             # el resumen
  python tools/chequea_learnset.py MEWTWO      # uno, con los dos lados
"""
import io
import os
import re
import sys

AQUI = os.path.dirname(os.path.abspath(__file__))
RAIZ = os.path.dirname(AQUI)
PBS = r"C:\Users\foulu\Downloads\Pokemon Anil V4.13\PBS\pokemon.txt"


def lee_moves_h():
    """(nombres, {dex: [(nivel, golpe)]})"""
    s = io.open(os.path.join(RAIZ, "moves.h"), encoding="utf-8",
                errors="replace").read()
    i = s.index("MOVE_TBL[MOVE_COUNT] = {")
    nombres = re.findall(r'\{\s*"([A-Z0-9]+)"', s[i:])

    def vector(nombre):
        # La DECLARACION, no la primera vez que aparece el nombre: a
        # LEARN_OFS lo nombra un comentario 2500 lineas antes, y leyendo
        # desde ahi se cogian los numeros del array de al lado. Salio
        # Mewtwo con el learnset de Bulbasaur.
        j = re.search(r"static const \w+ " + nombre + r"\[", s).start()
        k = s.index("};", j)
        cuerpo = s[s.index("{", j) + 1:k]
        return [int(x) for x in re.findall(r"\b\d+\b", cuerpo)]

    ofs = vector("LEARN_OFS")
    lvl = vector("LEARN_LVL")
    mvs = vector("LEARN_MV")
    por_dex = {}
    for d in range(1, len(ofs)):
        a, b = ofs[d - 1], ofs[d]
        por_dex[d] = [(lvl[i], nombres[mvs[i]]) for i in range(a, b)]
    return nombres, por_dex


def lee_pbs():
    """{NOMBRE: [(nivel, GOLPE)]}, solo las formas base."""
    out = {}
    cur = None
    for ln in io.open(PBS, encoding="utf-8", errors="replace"):
        ln = ln.strip()
        m = re.match(r"^\[(.+)\]$", ln)
        if m:
            cab = m.group(1)
            cur = cab if "," not in cab else None   # las formas, aparte
            continue
        if cur and ln.startswith("Moves"):
            v = ln.split("=", 1)[1].strip().split(",")
            pares = []
            for i in range(0, len(v) - 1, 2):
                try:
                    pares.append((int(v[i].strip()), v[i + 1].strip().upper()))
                except ValueError:
                    pass
            out.setdefault(cur, pares)
    return out


def nombres_dex():
    s = io.open(os.path.join(RAIZ, "dex.h"), encoding="utf-8",
                errors="replace").read()
    i = s.index("DEX_TBL[DEX_COUNT + 1] = {")
    # El "?" del hueco 0 ENTRA en la lista: sin el, todo se corre una
    # casilla y Mewtwo sale con el learnset de Dragonite. Me paso, y por
    # un momento parecio un fallo del firmware.
    return re.findall(r'\{\s*"([^"]+)"', s[i:])


def main():
    _n, mio = lee_moves_h()
    suyo = lee_pbs()
    dexn = nombres_dex()

    uno = sys.argv[1].upper() if len(sys.argv) > 1 else None
    faltan_total = 0
    peores = []
    for d, lista in sorted(mio.items()):
        if d >= len(dexn):
            continue
        nom = dexn[d].upper().replace(" ", "").replace(".", "").replace("-", "")
        cand = [k for k in suyo if k.replace("_", "") == nom]
        if not cand:
            continue
        pbs = suyo[cand[0]]
        # Solo los golpes que NUESTRA tabla conoce: el PBS tiene golpes
        # que el port no embarca, y eso no es un fallo del learnset.
        conocidos = set(g for _l, g in lista)
        todos = set(g for _l, g in pbs)
        falta = [(l, g) for l, g in pbs if g not in conocidos]
        if uno and nom == uno.replace(" ", ""):
            print("== %s ==" % dexn[d])
            print("  PBS (%d):" % len(pbs))
            print("   ", ", ".join("%d:%s" % p for p in pbs))
            print("  port (%d):" % len(lista))
            print("   ", ", ".join("%d:%s" % p for p in lista))
            if falta:
                print("  NO ESTAN en el port:")
                print("   ", ", ".join("%d:%s" % p for p in falta))
            return 0
        if falta:
            faltan_total += len(falta)
            peores.append((len(falta), dexn[d], falta))

    if uno:
        print("no encontre", uno)
        return 1
    print("%d especies con golpes de nivel que el port no embarca, "
          "%d huecos en total" % (len(peores), faltan_total))
    for n, nom, falta in sorted(peores, reverse=True)[:15]:
        print("  %-14s %2d: %s" % (nom, n,
              ", ".join("%d:%s" % p for p in falta[:6])))
    return 0


if __name__ == "__main__":
    sys.exit(main())
