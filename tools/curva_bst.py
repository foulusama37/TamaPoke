#!/usr/bin/env python3
"""Recalcula WILD_BST_CAP: los techos de BST que abre cada insignia.

Son PERCENTILES del catalogo, no una rampa. La rampa lineal se probo y
estaba mal: la BST esta amontonada entre 300 y 500, asi que "+45 por
insignia" abria 24 -> 53 -> 65 -> 83 -> 102 -> 138 y las tres ultimas
insignias ya no daban nada.

Hay que volver a correrlo cuando cambia el catalogo o la fuente de stats.
Los numeros de pet.h eran de cuando habia 151 especies con los stats
OFICIALES; ahora son 386 con los del Anil, que sube a 140 de ellas.

Se cuenta POR REGION, no sobre las 386: desde 2026-10-03 la hierba no
adelanta regiones (wildWeight), asi que lo que vive un jugador de Kanto
con dos insignias es el percentil de KANTO, no el del catalogo entero. Un
techo sacado de las 386 dejaria las primeras insignias de Kanto abriendo
cuatro bichos.

  python3 tools/curva_bst.py
"""
import os
import sys

AQUI = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, AQUI)
from dex_data import DEX, LEGENDARY     # noqa: E402
from dex_range import REGIONES          # noqa: E402
from dex_stats import BASE_STATS        # noqa: E402

GYM_COUNT = 8


def main():
    # Los legendarios tienen su propia puerta (las ocho insignias), asi que
    # no deben estirar los percentiles de los siete primeros tramos.
    por_region = {}
    ini = 1
    for nombre, fin in REGIONES:
        bst = sorted(sum(BASE_STATS[n]) for n in range(ini, fin + 1)
                     if n not in LEGENDARY)
        por_region[nombre] = bst
        ini = fin + 1

    todos = sorted(sum(BASE_STATS[d[0]]) for d in DEX if d[0] not in LEGENDARY)

    print(f"{len(todos)} especies no legendarias, BST de {todos[0]} a {todos[-1]}")
    for nombre, bst in por_region.items():
        print(f"  {nombre:<6} {len(bst):3d}  min {bst[0]}  mediana "
              f"{bst[len(bst) // 2]}  max {bst[-1]}")

    # La octava LEVANTA el techo del todo (si no, habria que afinar el
    # numero hasta que Mewtwo pasara raspando, que es un apano).
    print("\npercentiles por region (las 7 primeras insignias):")
    caps = {}
    for nombre, bst in por_region.items():
        c = []
        for i in range(1, GYM_COUNT):
            v = bst[min(len(bst) - 1, len(bst) * i // (GYM_COUNT + 1))]
            c.append(v)
        c.append(9999)
        caps[nombre] = c
        print(f"  {nombre:<6} " + ", ".join(f"{v}" for v in c[:-1]))

    # UNA tabla POR REGION, y no una sola para las tres. Se probo lo otro:
    # cogiendo el maximo de cada escalon, la segunda insignia de Johto
    # abria CINCO especies, porque Johto casi no tiene nada entre 280 y 318
    # y el escalon lo marcaba Kanto. Cada region tiene su reparto y cada una
    # merece el suyo; la tabla vive en `Region` (gyms.h), que es donde ya
    # estan las otras cosas que cambian de region en region.
    print("\npara gyms.h (el campo bstCap de cada Region):")
    for nombre, c in caps.items():
        print(f"static const uint16_t BST_{nombre}[GYM_COUNT] = {{ "
              + ", ".join(str(v) for v in c) + " };")

    print("\ncuantas abre cada insignia:")
    for nombre, bst in por_region.items():
        abiertas = [sum(1 for b in bst if b <= t) for t in caps[nombre]]
        saltos = [abiertas[i] - abiertas[i - 1] for i in range(1, GYM_COUNT)]
        print(f"  {nombre:<6} " + " ".join(f"{a:3d}" for a in abiertas) +
              "   (+" + " +".join(str(s) for s in saltos) + ")")
        if min(saltos) < 6:
            print(f"     OJO: la insignia {saltos.index(min(saltos)) + 2} "
                  f"solo abre {min(saltos)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
