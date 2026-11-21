#!/usr/bin/env python3
"""Convierte una TTF en una fuente GFXfont para Arduino_GFX.

Por que
-------
La UI dibuja con la 5x7 clasica de Adafruit, que es cuadrada y seca; los
numeros sobre todo (niveles, PS, PP, el numero de la Pokedex) se leen como
una calculadora. El Anil trae las fuentes de los juegos en `Fonts/`, y
"power clear" es justo la redonda de la era DS.

El avance es FIJO
-----------------
`textW()` del firmware mide el texto clasico como `strlen * 6 * talla` y
toda la pantalla esta centrada con esa cuenta. Emitiendo la fuente con
xAdvance = 6 para todos los glifos, la medida sigue siendo exacta al pixel
y no se mueve ni una caja. Lo unico que cambia es el DIBUJO de cada letra.
Power clear es estrecha de por si (~5,5 px por caracter a 12 px), asi que
seis es su sitio natural y no hay que apretarla.

La altura si cambia, y por eso se controla con `--alto`: a talla 2 la
clasica ocupa 14 px y las cajas de texto estan hechas para eso. Una fuente
de 12 px se iria a 24 y se pisarian las lineas.

  python3 tools/gen_font.py                 # power clear, 10 px
  python3 tools/gen_font.py --px 9 --ver    # y saca una muestra en PNG
"""
import argparse
import io
import os

from PIL import Image, ImageDraw, ImageFont

AQUI = os.path.dirname(os.path.abspath(__file__))
RAIZ = os.path.dirname(AQUI)
ANIL = r"C:\Users\foulu\Downloads\Pokemon Anil V4.13"
TTF = os.path.join(ANIL, "Fonts", "power clear.ttf")
DST = os.path.join(RAIZ, "font_redonda.h")
MUESTRA = r"C:\Users\foulu\Downloads\digisprites\fuente_muestra.png"

PRIMERO, ULTIMO = 32, 126     # ASCII imprimible, como la clasica
AVANCE = 6                    # el de la 5x7; se puede subir con --avance


def glifos(ttf, px, umbral):
    """[(ch, w, h, xoff, yoff, bits)] midiendo cada letra por separado."""
    f = ImageFont.truetype(ttf, px)
    # El ascenso de la fuente marca donde cae la linea base dentro del
    # lienzo donde se dibuja; hace falta para los yOffset, que Adafruit
    # cuenta DESDE la linea base hacia arriba (negativos).
    asc, _des = f.getmetrics()
    lienzo = px * 3
    out = []
    for c in range(PRIMERO, ULTIMO + 1):
        ch = chr(c)
        im = Image.new("L", (lienzo, lienzo), 0)
        ImageDraw.Draw(im).text((px, px), ch, font=f, fill=255)
        im = im.point(lambda v: 255 if v >= umbral else 0)
        caja = im.getbbox()
        if caja is None:                       # el espacio y poco mas
            out.append((ch, 0, 0, 0, 0, []))
            continue
        x0, y0, x1, y1 = caja
        w, h = x1 - x0, y1 - y0
        if w > AVANCE:                         # no cabe: se aprieta
            im = im.crop((x0, y0, x1, y1)).resize((AVANCE, h), Image.LANCZOS)
            im = im.point(lambda v: 255 if v >= 110 else 0)
            w, x0 = AVANCE, x0 + (w - AVANCE) // 2
        else:
            im = im.crop((x0, y0, x1, y1))
        px_ = im.load()
        bits = [1 if px_[x, y] else 0 for y in range(h) for x in range(w)]
        # cursor en (px, px + asc) = linea base; el glifo arranca en (x0, y0)
        out.append((ch, w, h, x0 - px, y0 - (px + asc), bits))
    return out


def empaqueta(gs):
    """Los bits de todos los glifos, seguidos y a 8 por byte (MSB primero)."""
    datos, offs, acc, nbits = [], [], 0, 0
    for (_ch, _w, _h, _xo, _yo, bits) in gs:
        offs.append(len(datos) + (1 if nbits else 0) - (1 if nbits else 0))
        # el offset es en BYTES y cada glifo empieza en byte nuevo
        if nbits:
            datos.append(acc << (8 - nbits))
            acc, nbits = 0, 0
        offs[-1] = len(datos)
        for b in bits:
            acc = (acc << 1) | b
            nbits += 1
            if nbits == 8:
                datos.append(acc)
                acc, nbits = 0, 0
    if nbits:
        datos.append(acc << (8 - nbits))
    return datos, offs


def escribe(gs, datos, offs, px, alto_linea, ttf):
    L = []
    L.append("// GENERADO por tools/gen_font.py - no editar a mano.")
    L.append("//")
    L.append("// %s a %d px, avance FIJO de %d px." % (os.path.basename(ttf), px, AVANCE))
    L.append("// El avance es el mismo que el de la 5x7 clasica a proposito:")
    L.append("// textW() mide `strlen * 6 * talla` y toda la pantalla esta")
    L.append("// centrada con esa cuenta, asi que cambia el dibujo de cada")
    L.append("// letra y NO se mueve ninguna caja.")
    L.append("#pragma once")
    L.append("#include <stdint.h>")
    L.append("")
    L.append("static const uint8_t FONT_REDONDA_BM[] = {")
    for i in range(0, len(datos), 16):
        L.append("  " + " ".join("0x%02X," % b for b in datos[i:i + 16]))
    L.append("};")
    L.append("")
    L.append("static const GFXglyph FONT_REDONDA_GL[] = {")
    for (ch, w, h, xo, yo, _b), off in zip(gs, offs):
        nom = ch if ch.isalnum() else "0x%02X" % ord(ch)
        L.append("  { %5d, %2d, %2d, %2d, %3d, %4d },   // '%s'"
                 % (off, w, h, AVANCE, xo, yo, nom))
    L.append("};")
    L.append("")
    L.append("static const GFXfont FONT_REDONDA = {")
    L.append("  (uint8_t *)FONT_REDONDA_BM, (GFXglyph *)FONT_REDONDA_GL,")
    L.append("  %d, %d, %d };" % (PRIMERO, ULTIMO, alto_linea))
    L.append("")
    io.open(DST, "w", encoding="utf-8", newline="\n").write("\n".join(L))
    return len(datos)


def muestra(gs, px):
    """Un PNG con el abecedario y unos numeros, para mirarlo."""
    txt = "LV 25  HP 118/118  N.0123456789  POKEDEX"
    esc = 4
    im = Image.new("RGB", (len(txt) * AVANCE * esc + 20, 30 * esc), (18, 20, 28))
    d = ImageDraw.Draw(im)
    por = {g[0]: g for g in gs}
    x = 10
    base = 20 * esc
    for ch in txt:
        g = por.get(ch)
        if g:
            _c, w, h, xo, yo, bits = g
            for yy in range(h):
                for xx in range(w):
                    if bits[yy * w + xx]:
                        px0 = x + (xo + xx) * esc
                        py0 = base + (yo + yy) * esc
                        d.rectangle([px0, py0, px0 + esc - 1, py0 + esc - 1],
                                    fill=(240, 240, 250))
        x += AVANCE * esc
    os.makedirs(os.path.dirname(MUESTRA), exist_ok=True)
    im.save(MUESTRA)
    return MUESTRA


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--ttf", default=TTF)
    ap.add_argument("--px", type=int, default=10)
    ap.add_argument("--umbral", type=int, default=110)
    ap.add_argument("--avance", type=int, default=AVANCE)
    ap.add_argument("--ver", action="store_true")
    a = ap.parse_args()

    globals()["AVANCE"] = a.avance
    gs = glifos(a.ttf, a.px, a.umbral)
    altos = [g[2] for g in gs if g[2]]
    anchos = [g[1] for g in gs if g[1]]
    datos, offs = empaqueta(gs)
    alto_linea = max(altos) + 2 if altos else a.px
    n = escribe(gs, datos, offs, a.px, alto_linea, a.ttf)
    print("%s a %d px: %d glifos, alto %d..%d, ancho %d..%d, %d bytes"
          % (os.path.basename(a.ttf), a.px, len(gs), min(altos), max(altos),
             min(anchos), max(anchos), n))
    print("->", DST)
    if a.ver:
        print("muestra ->", muestra(gs, a.px))


# Guardado: este modulo tambien se importa para comparar fuentes, y sin
# esto la importacion volvia a generar el .h con los valores por defecto.
if __name__ == "__main__":
    main()
