#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Saca de /fx/fx.bin la animacion de un golpe, para MIRARLA.

Es el unico modo de comprobar que lo que va a la tarjeta es lo que uno
cree: gen_art.py dice cuantos fotogramas emitio, pero no si son los del
golpe que toca ni si el RLE los devuelve bien. Esto lo decodifica igual
que lo hace la placa (SdFx::load) y lo guarda en un PNG.

    python tools/ve_fxbin.py FLAREBLITZ EMBER
"""
import io
import os
import re
import struct
import sys

from PIL import Image, ImageDraw

AQUI = os.path.dirname(os.path.abspath(__file__))
RAIZ = os.path.dirname(AQUI)
BIN = os.path.join(AQUI, "sdcard", "fx", "fx.bin")
TRANSP = 0xFF


def nombres():
    txt = io.open(os.path.join(RAIZ, "moves.h"), encoding="utf-8",
                  errors="replace").read()
    return re.findall(r'^\s*\{\s*"([A-Z0-9]+)",\s*T_', txt, flags=re.M)


def descomprime(rle, w, h):
    """El RLE de art.h: pares (cuenta, valor)."""
    px = bytearray()
    i = 0
    while i + 1 < len(rle) and len(px) < w * h:
        px += bytes([rle[i + 1]]) * rle[i]
        i += 2
    px += bytes([TRANSP]) * max(0, w * h - len(px))
    return px[:w * h]


def lee(mi):
    d = open(BIN, "rb").read()
    assert d[:4] == b"TPFX", "cabecera mala"
    n_golpes, _ = struct.unpack("<HH", d[4:8])
    if mi >= n_golpes:
        return None
    off = struct.unpack("<I", d[8 + 4 * mi:12 + 4 * mi])[0]
    if not off:
        return None
    n, esc, _ = struct.unpack("<BBH", d[off:off + 4])
    p = off + 4
    cab = []
    for _ in range(n):
        w, h, npal, _z, nrle, _z2 = struct.unpack("<BBBBHH", d[p:p + 8])
        cab.append((w, h, npal, nrle))
        p += 8
    pals = []
    for (w, h, npal, nrle) in cab:
        pal = list(struct.unpack("<%dH" % npal, d[p:p + 2 * npal])) if npal else []
        p += 2 * npal
        pals.append(pal)
    fotos = []
    for (w, h, npal, nrle), pal in zip(cab, pals):
        rle = d[p:p + nrle]
        p += nrle
        if not w or not h:
            fotos.append(None)
            continue
        idx = descomprime(rle, w, h)
        im = Image.new("RGBA", (w, h), (0, 0, 0, 0))
        px = im.load()
        for y in range(h):
            for x in range(w):
                v = idx[y * w + x]
                if v == TRANSP or v >= len(pal):
                    continue
                c = pal[v]
                px[x, y] = (((c >> 11) << 3) & 0xF8, ((c >> 5) << 2) & 0xFC,
                            (c << 3) & 0xF8, 255)
        fotos.append(im)
    return esc, fotos


def main():
    nom = nombres()
    que = [a.upper() for a in sys.argv[1:]] or ["FLAREBLITZ"]
    filas = []
    for q in que:
        if q not in nom:
            print(q, "no esta en moves.h")
            continue
        r = lee(nom.index(q))
        if r is None:
            print(q, "-> NO tiene animacion propia en fx.bin")
            continue
        esc, fotos = r
        print("%s -> %d fotogramas, escala %d, %s" %
              (q, len(fotos), esc,
               "x".join(str(v) for v in (fotos[0].size if fotos[0] else (0, 0)))))
        filas.append((q, fotos))
    if not filas:
        return
    L = 120
    ancho = max(len(f) for _, f in filas)
    out = Image.new("RGB", (170 + ancho * L, len(filas) * L), (28, 28, 34))
    d = ImageDraw.Draw(out)
    for fi, (q, fotos) in enumerate(filas):
        d.text((6, fi * L + 6), q, (240, 220, 120))
        d.text((6, fi * L + 22), "desde fx.bin", (160, 160, 160))
        for i, im in enumerate(fotos):
            bg = Image.new("RGB", (L, L), (16, 16, 20))
            if im is not None:
                s = im.resize((L, L), Image.NEAREST)
                bg.paste(s, (0, 0), s)
            out.paste(bg, (170 + i * L, fi * L))
    p = os.path.join(RAIZ, "_claude_scratch", "fxbin.png")
    out.save(p)
    print(p)


if __name__ == "__main__":
    main()
