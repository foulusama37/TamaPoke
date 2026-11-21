#!/usr/bin/env python3
"""Genera /mons/thumbs.bin: miniaturas 40x40 de los 151 para la galeria.

Salen de los ICONOS de Anil (Graphics/Pokemon/Icons/NOMBRE.png), no de los
battlers. Dos razones:

  - Un icono esta DIBUJADO para 64 px: a 40x40 se lee. Un battler de 100 px
    reducido pierde la cara y queda una mancha.
  - La galeria era la unica pantalla que seguia con sprites PMD, porque su
    thumbs.bin se genero antes de cambiar los sprites a Anil.

El PNG es de 128x64 = dos frames de 64x64 (el balanceo tipico de Essentials).
Se coge solo el PRIMERO y la rejilla se queda quieta: son 16 bichos a la vez
en pantalla y, moviendose todos, aquello es un hormiguero. La animacion vive
en la vista de detalle, donde hay UN bicho y se carga el battler entero.

Formato TPTH (little-endian):

  char[4] "TPTH"
  uint16  count
  uint32  offset[count]    (desde el inicio del archivo, 1-based: offset[0]=dex 1)
  blobs:  u8 w, u8 h, u8 palCount, u16 pal[palCount], u8 data[w*h] (0xFF transp.)

  python3 tools/make_thumbs.py
"""
import os
import struct
import sys

from PIL import Image

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from dex_data import DEX
from dex_range import N_DEX

DIR = os.path.join(os.path.dirname(__file__), 'sdcard', 'mons')
ICONS = (r"C:\Users\foulu\Downloads\Pokemon Anil V4.13"
         r"\Graphics\Pokemon\Icons")
CELL = 40

NOMBRE = {d[0]: d[1].upper() for d in DEX}
NOMBRE[29] = 'NIDORANfE'   # el acervo de Anil los escribe asi, igual que el PBS
NOMBRE[32] = 'NIDORANmA'


def read_pmd_idle_frame0(path):
    """Frame 0 de la accion Idle (id 0 = vista frontal) de un sprite PMD TPK2."""
    with open(path, 'rb') as f:
        buf = f.read()
    if buf[:4] != b'TPK2':
        raise ValueError('magic TPK2')
    nacts = buf[4]
    (palcount,) = struct.unpack_from('<H', buf, 5)
    pal = list(struct.unpack_from(f'<{palcount}H', buf, 7))
    p = 7 + palcount * 2
    for _ in range(nacts):
        aid, w, h, nf = buf[p], buf[p + 1], buf[p + 2], buf[p + 3]
        p += 4 + nf * 2  # cabecera + ms[]
        if aid == 0:     # PMD_IDLE
            return w, h, pal, buf[p:p + w * h]  # frame 0
        p += w * h * nf
    raise ValueError('sin accion Idle (id 0)')


def shrink(w, h, pal, data):
    # escala a CELL x CELL con vecino mas cercano, conservando aspecto
    scale = min(CELL / w, CELL / h, 1.0)
    nw, nh = max(1, round(w * scale)), max(1, round(h * scale))
    out = bytearray()
    used = {}
    newpal = []
    for y in range(nh):
        sy = min(h - 1, int(y / scale)) if scale < 1 else y
        for x in range(nw):
            sx = min(w - 1, int(x / scale)) if scale < 1 else x
            idx = data[sy * w + sx]
            if idx == 0xFF:
                out.append(0xFF)
                continue
            c = pal[idx]
            if c not in used:
                used[c] = len(newpal)
                newpal.append(c)
            out.append(used[c])
    return nw, nh, newpal, bytes(out)


def read_anil_icon(dex):
    """Frame 0 del icono de Anil, recortado a lo que ocupa e indexado.

    Devuelve lo mismo que read_pmd_idle_frame0 (w, h, paleta, indices) para
    que shrink() no tenga que enterarse de nada.
    """
    nombre = NOMBRE.get(dex)
    if not nombre:
        raise ValueError(f'dex {dex} no esta en dex_data')
    path = os.path.join(ICONS, nombre + '.png')
    if not os.path.exists(path):
        raise FileNotFoundError(path)
    im = Image.open(path).convert('RGBA')
    W, H = im.size
    frames = W // H if H and W % H == 0 else 1
    im = im.crop((0, 0, W // frames, H))      # solo el primer frame
    bb = im.getbbox()                          # fuera el aire de alrededor
    if bb:
        im = im.crop(bb)
    w, h = im.size
    pal, used, data = [], {}, bytearray()
    for r, g, b, a in im.getdata():
        if a < 128:
            data.append(0xFF)                  # transparente
            continue
        c = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)   # RGB565
        if c not in used:
            used[c] = len(pal)
            pal.append(c)
        data.append(used[c])
    if len(pal) > 255:
        raise ValueError(f'dex {dex}: {len(pal)} colores, no cabe en TPK2')
    return w, h, pal, bytes(data)


def main():
    blobs = []
    for dex in range(1, N_DEX + 1):
        w, h, pal, data = read_anil_icon(dex)
        nw, nh, npal, ndata = shrink(w, h, pal, data)
        if len(npal) > 255:
            raise ValueError(f'{dex}: paleta {len(npal)}')
        blob = struct.pack('<3B', nw, nh, len(npal))
        blob += struct.pack(f'<{len(npal)}H', *npal)
        blob += ndata
        blobs.append(blob)

    head = 4 + 2 + 4 * N_DEX
    offsets, pos = [], head
    for b in blobs:
        offsets.append(pos)
        pos += len(b)

    out = os.path.join(DIR, 'thumbs.bin')
    with open(out, 'wb') as f:
        f.write(b'TPTH')
        f.write(struct.pack('<H', N_DEX))
        f.write(struct.pack(f'<{N_DEX}I', *offsets))
        for b in blobs:
            f.write(b)
    print(f"guardado {out}: {pos / 1024:.0f} KB, {len(blobs)} miniaturas")


if __name__ == '__main__':
    main()
