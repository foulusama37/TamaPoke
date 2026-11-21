#!/usr/bin/env python3
"""Empaquetador de los mapas de region.

Coge los nueve posters que dejo el usuario en _claude_scratch/mapas y los
deja en /map/NN.bin, el MISMO formato TPKB que los fondos de combate:

  char[4] "TPKB"
  u16 w, u16 h
  u16 pix[w*h]            RGB565, little endian, fila a fila

El numero es la region: 00 Kanto, 01 Johto... el mismo indice que
`dexRegionOf()` y que REGIONS[] en gyms.h. Si se cambia el orden de uno hay
que cambiarlo en los tres sitios.

Los originales son 1200x1200 con el poster enmarcado flotando sobre blanco.
Lo que se guarda es SOLO el poster: el blanco de alrededor no es parte del
dibujo y en una pantalla redonda sobre fondo oscuro se veria como un
recuadro blanco gigante.

El tamano de destino sale de la pantalla, y MEDIDO, no a ojo: el poster no
va centrado en el circulo (debajo tienen que caber las flechas y el nombre),
asi que sus esquinas de arriba quedan mas lejos del centro que las de abajo.
Con el centro en (233, 196) y una lamina de 320x240, la esquina mas lejana
cae a 224 px del centro y el radio es 233: entra con el marco incluido. Con
344x258 —— el maximo teorico si fuera centrado —— se salia por arriba.
Cada mapa pesa ~150 KB y se carga de uno en uno, igual que los fondos.

  python3 tools/pack_maps.py          # los nueve
  python3 tools/pack_maps.py kanto    # solo ese
"""
import os
import struct
import sys

from PIL import Image

AQUI = os.path.dirname(os.path.abspath(__file__))
ORIG = os.path.join(os.path.dirname(AQUI), "_claude_scratch", "mapas")
OUT = os.path.join(AQUI, "sdcard", "map")
MAX_W, MAX_H = 320, 240

# El indice ES la region. No se reordena.
MAPAS = [
    "kanto", "johto", "hoenn", "sinnoh", "unova",
    "kalos", "alola", "galar", "paldea",
]


def rgb565(r, g, b):
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def quita_el_blanco(im, umbral=242):
    """El recuadro del poster dentro de la hoja blanca.

    Se mira fila a fila y columna a columna: una linea cuenta como "hay
    dibujo" en cuanto alguno de sus pixeles baja del umbral. Con getbbox()
    sobre un alfa no valdria —— estos JPG no tienen alfa —— y con un recorte
    fijo tampoco, porque los nueve posters no estan colocados igual.
    """
    g = im.convert("L")
    w, h = g.size
    px = g.load()
    # un paso de 2 px basta y va cuatro veces mas rapido en 1200x1200
    filas = [y for y in range(0, h, 2)
             if any(px[x, y] < umbral for x in range(0, w, 2))]
    cols = [x for x in range(0, w, 2)
            if any(px[x, y] < umbral for y in range(0, h, 2))]
    if not filas or not cols:
        return im
    return im.crop((cols[0], filas[0], cols[-1] + 1, filas[-1] + 1))


def cabe(im, mw, mh):
    """Escala para que entre ENTERO. Un mapa recortado no sirve de nada."""
    e = min(mw / im.width, mh / im.height)
    return im.resize((max(1, round(im.width * e)), max(1, round(im.height * e))),
                     Image.LANCZOS)


def empaqueta(idn, nombre):
    ruta = os.path.join(ORIG, nombre + ".jpg")
    if not os.path.exists(ruta):
        print(f"  !! falta {nombre}.jpg")
        return 0
    im = cabe(quita_el_blanco(Image.open(ruta).convert("RGB")), MAX_W, MAX_H)
    w, h = im.size
    px = im.load()
    buf = bytearray(w * h * 2)
    i = 0
    for y in range(h):
        for x in range(w):
            r, g, b = px[x, y]
            v = rgb565(r, g, b)
            buf[i] = v & 0xFF
            buf[i + 1] = v >> 8
            i += 2
    os.makedirs(OUT, exist_ok=True)
    with open(os.path.join(OUT, f"{idn:02d}.bin"), "wb") as f:
        f.write(b"TPKB")
        f.write(struct.pack("<HH", w, h))
        f.write(buf)
    n = 8 + len(buf)
    print(f"  {idn} {nombre:8s} {w}x{h}  {n // 1024} KB")
    return n


def main():
    filtro = [a.lower() for a in sys.argv[1:]]
    total = 0
    for idn, nombre in enumerate(MAPAS):
        if filtro and nombre not in filtro:
            continue
        total += empaqueta(idn, nombre)
    print(f"{total // 1024} KB en {OUT}")


if __name__ == "__main__":
    main()
