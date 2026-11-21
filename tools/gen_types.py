#!/usr/bin/env python3
"""La tipificacion DEL ANIL, a tools/dex_types.py.

Ojo: dex_data.py ya tiene un campo de "tipo", pero no sirve para combatir.
Es una simplificacion para elegir color e icono de bioma: son 17 claves, NO
existe volador, y cada especie tiene una sola. Asi, Charizard figura como
fuego a secas y Gyarados como agua. Para pelear hacen falta los de verdad,
con el segundo incluido, que son estos.

Salian de PokeAPI. Ahora salen del PBS del Anil, por lo mismo que los stats
(ver gen_stats.py): su autor reparte segundos tipos a mano y son 13 — Arbok
pasa a veneno/siniestro, Feraligatr a agua/dragon, los tres iniciales de
Johto ganan uno. Tener los stats de un sitio y los tipos de otro no tendria
sentido.

Con esto desaparece de paso toda la historia de `past_types`: se uso para
intentar la tipificacion de gen 1, pero el juego lleva golpes y tabla
MODERNOS desde el principio (ver type_chart.py), asi que la de gen 1 nunca
llego a emitirse. El Anil tambien es moderno.

  python3 tools/gen_types.py
"""
import io
import os
import sys

AQUI = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, AQUI)
import anil_pbs                   # noqa: E402
from dex_range import N_DEX       # noqa: E402
from type_chart import TIPOS as ORDEN   # el orden sale de la tabla de efectividad

DST = os.path.join(AQUI, 'dex_types.py')


def main():
    antes = {}
    try:
        from dex_types import TIPOS as antes
    except Exception:
        pass

    filas = {}
    for n, slug, c in anil_pbs.filas():
        t1, t2 = anil_pbs.tipos(c)
        for t in (t1, t2):
            if t and t not in ORDEN:
                raise SystemExit(f'tipo que no esta en type_chart: {t} (#{n} {slug})')
        filas[n] = (t1, t2)

    with io.open(DST, 'w', encoding='utf-8', newline='\n') as f:
        f.write('"""GENERADO por tools/gen_types.py desde el PBS del Anil.\n\n'
                'No editar a mano. Son los tipos del ANIL, que reparte algunos\n'
                'segundos tipos por su cuenta (Arbok veneno/siniestro...).\n"""\n\n')
        f.write('ORDEN = %r\n\n' % (ORDEN,))
        f.write('# dex: (tipo1, tipo2 o None)\nTIPOS = {\n')
        for n in range(1, N_DEX + 1):
            t1, t2 = filas[n]
            f.write(f'    {n}: ({t1!r}, {t2!r}),\n')
        f.write('}\n')
    print(f'escrito {DST}')

    difs = [(n, antes[n], filas[n]) for n in filas
            if n in antes and antes[n] != filas[n]]
    if difs:
        print(f'\n{len(difs)} especies cambian de tipo:')
        for n, a, b in difs:
            viejo = '/'.join(x for x in a if x)
            nuevo = '/'.join(x for x in b if x)
            print(f'   #{n:3d} {viejo:<20} -> {nuevo}')
    dobles = sum(1 for t1, t2 in filas.values() if t2)
    print(f'\n{dobles} de {N_DEX} tienen dos tipos')
    return 0


if __name__ == '__main__':
    sys.exit(main())
