#!/usr/bin/env python3
"""female.h: que especies tienen sprite de HEMBRA en el Anil.

En el Anil hay 101 sprites de frente con la hembra aparte (y 103 shiny),
llamados <SLUG>_female.png. Sin esta tabla el aparato tendria dos malas
opciones: no usarlos nunca (y entonces no sirven de nada) o intentar
abrir /mons/hNNN.bin en cada carga y comerse un fallo de SD por cada
bicho que no la tiene, 924 de 1025 veces.

Es un mapa de bits, como el de las MT y el de los efectos propios: un bit
por especie, 129 bytes para las 1025. La fuente es la CARPETA del Anil,
que es quien sabe de verdad si el dibujo existe; auditoria.py comprueba
luego que lo que dice el bit este de verdad en la tarjeta.

  python3 tools/gen_female.py
"""
import io
import os
import sys

AQUI = os.path.dirname(os.path.abspath(__file__))
RAIZ = os.path.dirname(AQUI)
sys.path.insert(0, AQUI)

from dex_data import DEX            # noqa: E402
from dex_range import N_DEX         # noqa: E402

# La carpeta del Anil. MISMA forma de encontrarla que pack_anil.py y con
# la misma variable de entorno: dos maneras distintas de buscar lo mismo
# es una que se queda desfasada.
ANIL = os.environ.get(
    'ANIL_GFX',
    r"C:\Users\foulu\Downloads\Pokemon Anil V4.13\Graphics\Pokemon")

# Mismos alias que el empaquetador: los Nidoran se llaman distinto en las
# carpetas normales y en las shiny.
ALIAS = {
    29: ['NIDORANF', 'NIDORANfE'],
    32: ['NIDORANM', 'NIDORANmA'],
}
NOMBRE = {num: ALIAS.get(num, [slug.upper()]) for num, slug, disp, *_ in DEX}


def tiene(dexnum, sub):
    for n in NOMBRE.get(dexnum, []):
        if os.path.exists(os.path.join(ANIL, sub, n + '_female.png')):
            return True
    return False


def main():
    if not os.path.isdir(ANIL):
        raise SystemExit(f'no encuentro la carpeta del Anil: {ANIL}')

    normal, shiny = [], []
    for d in range(1, N_DEX + 1):
        if tiene(d, 'Front'):
            normal.append(d)
        if tiene(d, 'Front shiny'):
            shiny.append(d)

    # Solo se marca la que tiene las DOS: el aparato pide hNNN o hsNNN
    # segun el shiny, y marcar una especie que solo tenga una mitad seria
    # pedir un fichero que no existe en cuanto saliera un shiny.
    conAmbas = sorted(set(normal) & set(shiny))
    soloUna = sorted(set(normal) ^ set(shiny))

    nbytes = (N_DEX + 8) // 8
    bits = bytearray(nbytes)
    for d in conAmbas:
        bits[d >> 3] |= 1 << (d & 7)

    out = [
        '#pragma once\n#include <stdint.h>\n#include "dex.h"\n\n',
        '// GENERADO por tools/gen_female.py - no editar a mano\n',
        '//\n',
        '// Que especies traen sprite de HEMBRA aparte en el Anil. Un bit\n',
        '// por especie: sin esto el aparato intentaria abrir el fichero de\n',
        '// la hembra en CADA carga y se comeria un fallo de SD en las 924\n',
        '// especies que no la tienen.\n',
        '//\n',
        '// Solo se marca la que tiene frente normal Y shiny: el combate\n',
        '// pide hNNN o hsNNN segun el shiny, y marcar media variante seria\n',
        '// pedir un fichero que no existe en cuanto saliera uno shiny.\n',
        f'#define FEMALE_BYTES {nbytes}\n',
        f'#define FEMALE_COUNT {len(conAmbas)}\n\n',
        f'static const uint8_t FEMALE_BITS[FEMALE_BYTES] = {{\n',
    ]
    for i in range(0, nbytes, 12):
        out.append('  ' + ' '.join(f'0x{b:02X},' for b in bits[i:i + 12]) + '\n')
    out.append('};\n\n')
    out.append('// Tiene esa especie dibujo de hembra propio?\n'
               'static inline bool hasFemale(int16_t dex) {\n'
               '  if (dex < 1 || dex > DEX_COUNT) return false;\n'
               '  return (FEMALE_BITS[dex >> 3] >> (dex & 7)) & 1;\n'
               '}\n')

    dst = os.path.join(RAIZ, 'female.h')
    io.open(dst, 'w', encoding='utf-8', newline='\n').write(''.join(out))
    print(f'guardado {dst}')
    print(f'  {len(conAmbas)} especies con hembra (frente normal Y shiny)')
    print(f'  {nbytes} bytes de mapa')
    if soloUna:
        print(f'  OJO: {len(soloUna)} tienen la hembra en UNA sola carpeta y '
              f'se quedan fuera: {soloUna[:12]}')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
