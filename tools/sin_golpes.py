#!/usr/bin/env python3
"""Especies que se quedan SIN NINGUN golpe embarcado, y por que.

El Ditto lo delato con la placa en la mano: capturado, su menu de golpes
solo tenia Forcejeo. La causa no es un fallo suelto, es una regla con un
borde sin guardia. gen_moves.py descarta del aprendizaje los golpes de
apoyo cuyo efecto no esta implementado:

    if m['pot'] <= 0 and m['fx'] == 'FX_NONE' and m['sx'] == 'SX_NONE':
        continue

La regla es buena -- un golpe en la tarjeta que al usarlo no hace nada es
peor que no tenerlo. Lo que nadie comprobo es que a una especie se le
pueden caer TODOS. El Ditto solo aprende Transformacion; Transformacion
no tiene efecto implementado; luego el Ditto no aprende nada y solo sabe
Forcejeo. Eso no es "un golpe que no hace nada": es un bicho que no
puede pelear.

Mide sobre lo que SE EMBARCA (LEARN_OFS de moves.h), no sobre el PBS: lo
que importa es lo que el aparato tiene, no lo que podria tener. Para
cada especie vacia va a buscar al PBS que le quedo fuera, que es la
lista de la compra.
"""
import io
import os
import re
import sys

AQUI = os.path.dirname(os.path.abspath(__file__))
RAIZ = os.path.dirname(AQUI)
sys.path.insert(0, AQUI)
import anil_pbs  # noqa: E402


def lee(n):
    return io.open(os.path.join(RAIZ, n), encoding="utf-8",
                   errors="replace").read()


def arreglo(txt, nombre):
    m = re.search(r"\b%s\[[^\]]*\]\s*=\s*\{(.*?)\};" % nombre, txt, re.S)
    if not m:
        sys.exit("no encuentro %s" % nombre)
    return [int(x) for x in re.findall(r"-?\d+", m.group(1))]


def main():
    mv = lee("moves.h")
    ofs = arreglo(mv, "LEARN_OFS")
    en_tabla = set(re.findall(r'\{\s*"([A-Z0-9_]+)"', mv))
    if not en_tabla:
        sys.exit("no encuentro los nombres de MOVE_TBL")

    # el PBS, para poder decir QUE le falta a cada una
    dex_de = anil_pbs.numeros()
    aprende = {}
    for cab, campos in anil_pbs.bloques():
        d = dex_de.get(anil_pbs.clave(cab))
        if not d:
            continue
        partes = [p.strip() for p in campos.get("Moves", "").split(",")]
        aprende[d] = [partes[i + 1] for i in range(0, len(partes) - 1, 2)
                      if partes[i].isdigit()]

    vacias, falta = [], {}
    for d in range(1, len(ofs)):
        suyos = aprende.get(d, [])
        fuera = sorted({g for g in suyos if g not in en_tabla})
        if ofs[d] - ofs[d - 1] == 0:
            vacias.append((d, len(suyos), fuera))
        for g in fuera:
            falta[g] = falta.get(g, 0) + 1

    if vacias:
        print("%d especies SIN NINGUN golpe (solo saben Forcejeo):"
              % len(vacias))
        for d, n, fuera in vacias:
            print("  #%-4d el PBS le da %-2d, fuera de tabla: %s"
                  % (d, n, ", ".join(fuera) or "(ninguno: mirar el PBS)"))
    else:
        print("ninguna especie se queda sin golpes")

    print()
    print("golpes del PBS fuera de la tabla, por cuantas especies tocan:")
    for g, n in sorted(falta.items(), key=lambda kv: -kv[1])[:20]:
        print("  %-22s %4d especies" % (g, n))
    print("  (%d golpes distintos fuera en total)" % len(falta))
    return 1 if vacias else 0


if __name__ == "__main__":
    sys.exit(main())
