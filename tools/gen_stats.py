#!/usr/bin/env python3
"""Los SEIS stats base, DEL ANIL, a tools/dex_stats.py.

Salian de PokeAPI, o sea los oficiales. Ahora salen del PBS del Anil
(decision del usuario, 2026-10-03): este cacharro es el companero de SU
juego, y alli hay un reequilibrado hecho a mano que toca **93 de las 251**
que ya teniamos, casi siempre subiendo a los flojos — Butterfree pasa de
90/80/70 a 100/100/90, Paras de 35 a 65 de PS, Beedrill sube ataque y
velocidad. Mezclar las dos fuentes dejaria media Pokedex con un criterio y
media con otro.

El cambio no es solo cosmetico: la BST manda en que sale en la hierba
(WILD_BST_CAP) y el stat base mas alto decide que EV suelta cada especie.

OJO con el orden: el PBS escribe BaseStats como HP,Atk,Def,VEL,SpA,SpD.
Leerlo de corrido mete la velocidad en el hueco del ataque especial y los
numeros siguen pareciendo razonables, que es lo peligroso. Lo desenreda
anil_pbs.stats().

  python3 tools/gen_stats.py
"""
import io
import os
import sys

AQUI = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, AQUI)
import anil_pbs                   # noqa: E402
from dex_range import N_DEX       # noqa: E402

DST = os.path.join(AQUI, 'dex_stats.py')
NOMBRES = ('hp', 'atk', 'def', 'spa', 'spd', 'vel')


def main():
    antes = {}
    try:
        from dex_stats import BASE_STATS as antes
    except Exception:
        pass

    filas = {n: anil_pbs.stats(c) for n, _slug, c in anil_pbs.filas()}

    with io.open(DST, 'w', encoding='utf-8', newline='\n') as f:
        f.write('# GENERADO por tools/gen_stats.py desde el PBS del Anil - no editar\n')
        f.write('# num -> (hp, atk, def, spatk, spdef, vel)\n')
        f.write('#\n')
        f.write('# Son los del ANIL, no los oficiales: el reequilibrado de su autor\n')
        f.write('# sube a los flojos, y este aparato acompana a ESE juego.\n')
        f.write('BASE_STATS = {\n')
        for n in range(1, N_DEX + 1):
            f.write(f'    {n}: {filas[n]},\n')
        f.write('}\n')
    print(f'escrito {DST} ({N_DEX} especies)')

    difs = [(n, antes[n], filas[n]) for n in filas
            if n in antes and antes[n] != filas[n]]
    if difs:
        print(f'\n{len(difs)} especies cambian de stats:')
        for n, a, b in difs[:20]:
            d = ' '.join(f'{NOMBRES[i]}{b[i] - a[i]:+d}'
                         for i in range(6) if a[i] != b[i])
            print(f'   #{n:3d} {d}')
        if len(difs) > 20:
            print(f'   ... y {len(difs) - 20} mas')
        subenBst = sum(1 for _, a, b in difs if sum(b) > sum(a))
        print(f'   de esas, {subenBst} suben de BST y {len(difs) - subenBst} bajan')
    return 0


if __name__ == '__main__':
    sys.exit(main())
