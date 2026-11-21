#!/usr/bin/env python3
"""La fuente de TITULOS: "power clear bold" del Anil (la de los juegos de DS).

A diferencia de gen_font.py (que imita la 5x7 con avance fijo para no mover
ninguna caja), esta es PROPORCIONAL y solo la usan los carteles grandes
(evolucion, medalla, insignia), que miden su texto con tituloW() y no con
textW(). Lleva los 224 caracteres de 32 a 255 en CP437, que es como estan
codificadas las cadenas del firmware ("\\240" = a con tilde), asi que los
acentos del portugues y del espanol salen.

  python3 tools/gen_font_titulo.py            # 16 px
  python3 tools/gen_font_titulo.py --px 13 --ver
"""
import argparse
import io
import os

from PIL import Image, ImageDraw, ImageFont

AQUI = os.path.dirname(os.path.abspath(__file__))
RAIZ = os.path.dirname(AQUI)
ANIL = r"C:\Users\foulu\Downloads\Pokemon Anil V4.13"
TTF = os.path.join(ANIL, "Fonts", "power clear bold.ttf")
DST = os.path.join(RAIZ, "font_titulo.h")
MUESTRA = r"C:\Users\foulu\Downloads\digisprites\fuente_titulo_muestra.png"
PRIMERO, ULTIMO = 32, 255


# 176..179 son los del TIL en i18n.h (CH_ATIL...): la 5x7 los compone con
# una '~' encima; aqui van como letra propia.
TIL = {176: "ã", 177: "õ", 178: "Ã", 179: "Õ"}


def caracter(b):
    return TIL.get(b) or bytes([b]).decode("cp437")


def glifos(ttf, px, umbral):
    f = ImageFont.truetype(ttf, px)
    asc, _ = f.getmetrics()
    lienzo = px * 3
    out = []
    for c in range(PRIMERO, ULTIMO + 1):
        ch = caracter(c)
        adv = int(round(f.getlength(ch)))
        im = Image.new("L", (lienzo, lienzo), 0)
        ImageDraw.Draw(im).text((px, px), ch, font=f, fill=255)
        im = im.point(lambda v: 255 if v >= umbral else 0)
        caja = im.getbbox()
        if caja is None:
            out.append((c, 0, 0, max(adv, 1), 0, 0, []))
            continue
        x0, y0, x1, y1 = caja
        w, h = x1 - x0, y1 - y0
        px_ = im.crop(caja).load()
        bits = [1 if px_[x, y] else 0 for y in range(h) for x in range(w)]
        # un pixel de aire entre letras: la bold a tamano nativo se pega
        out.append((c, w, h, max(adv, w + (x0 - px)) + 1, x0 - px, y0 - (px + asc), bits))
    return out


def empaqueta(gs):
    datos, offs = [], []
    for g in gs:
        bits = g[6]
        offs.append(len(datos))
        acc = n = 0
        for b in bits:
            acc = (acc << 1) | b
            n += 1
            if n == 8:
                datos.append(acc)
                acc = n = 0
        if n:
            datos.append(acc << (8 - n))
    return datos, offs


def escribe(gs, datos, offs, px, ttf):
    altos = [g[2] for g in gs if g[2]]
    L = ["// GENERADO por tools/gen_font_titulo.py - no editar a mano.",
         "// %s a %d px, PROPORCIONAL, CP437 %d..%d." % (os.path.basename(ttf), px, PRIMERO, ULTIMO),
         "// Solo para carteles: se mide con tituloW(), no con textW().",
         "#pragma once", "#include <stdint.h>", "",
         "static const uint8_t FONT_TITULO_BM[] = {"]
    for i in range(0, len(datos), 16):
        L.append("  " + " ".join("0x%02X," % b for b in datos[i:i + 16]))
    L += ["};", "", "static const GFXglyph FONT_TITULO_GL[] = {"]
    for (c, w, h, adv, xo, yo, _b), off in zip(gs, offs):
        L.append("  { %5d, %2d, %2d, %2d, %3d, %4d },   // %d" % (off, w, h, adv, xo, yo, c))
    L += ["};", "",
          "static const GFXfont FONT_TITULO = {",
          "  (uint8_t *)FONT_TITULO_BM, (GFXglyph *)FONT_TITULO_GL,",
          "  %d, %d, %d };" % (PRIMERO, ULTIMO, max(altos) + 2),
          "// Lo que sube la letra mas alta desde la linea base (para centrar).",
          "#define FONT_TITULO_ASC %d" % max(-g[5] for g in gs if g[2]), ""]
    io.open(DST, "w", encoding="utf-8", newline="\n").write("\n".join(L))


def muestra(gs, esc=3):
    txt = "Evoluir? Manter forma  MEDALHA!  Ins\x82gnia  BULBASAUR Lv.16  \xa0\x87\xa2"
    por = {g[0]: g for g in gs}
    W = sum(por[ord(ch)][3] for ch in txt) * esc + 20
    im = Image.new("RGB", (W, 30 * esc), (248, 248, 240))
    d = ImageDraw.Draw(im)
    x, base = 10, 22 * esc
    for ch in txt:
        c, w, h, adv, xo, yo, bits = por[ord(ch)]
        for yy in range(h):
            for xx in range(w):
                if bits[yy * w + xx]:
                    px0, py0 = x + (xo + xx) * esc, base + (yo + yy) * esc
                    d.rectangle([px0, py0, px0 + esc - 1, py0 + esc - 1], fill=(40, 40, 56))
        x += adv * esc
    im.save(MUESTRA)
    return MUESTRA


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--ttf", default=TTF)
    ap.add_argument("--px", type=int, default=16)
    ap.add_argument("--umbral", type=int, default=128)
    ap.add_argument("--ver", action="store_true")
    a = ap.parse_args()
    gs = glifos(a.ttf, a.px, a.umbral)
    datos, offs = empaqueta(gs)
    escribe(gs, datos, offs, a.px, a.ttf)
    print("%d glifos, %d bytes -> %s" % (len(gs), len(datos), DST))
    if a.ver:
        print("muestra ->", muestra(gs))


if __name__ == "__main__":
    main()
