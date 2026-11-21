#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Hojas de contacto de los retratos Special0..3 de SpriteCollab.

Las 16 emociones con nombre (Normal, Happy, Pain...) se pueden cablear
solas: el nombre dice que son. Los cuatro SpecialN NO: `sprite_config.json`
los deja sin nombre y cada autor los llena como quiere. Arcanine usa el
Special0 para dormir, otro lo usa para una pose, otro para un gesto de su
forma regional. O sea que no hay dato que leer —— hay que MIRARLOS.

Esto saca una hoja de contacto por slot, con el numero de la Pokedex y el
nombre debajo, para poder decidir uno a uno cual nos sirve y para que.

    python tools/hoja_specials.py G:/SpriteCollab/repo
    (deja las hojas en _claude_scratch/specials/)
"""
import io
import json
import os
import re
import sys

from PIL import Image, ImageDraw

AQUI = os.path.dirname(os.path.abspath(__file__))
RAIZ = os.path.dirname(AQUI)
CEL = 86
COLS = 14
POR_HOJA = COLS * 7


def main():
    if len(sys.argv) < 2:
        sys.exit("uso: python tools/hoja_specials.py <carpeta del repo>")
    repo = sys.argv[1]
    base = os.path.join(repo, "portrait")
    if not os.path.isdir(base):
        sys.exit("no veo " + base)

    nombres = {}
    try:
        tr = json.load(io.open(os.path.join(repo, "tracker.json"),
                               encoding="utf-8"))
        for k, e in tr.items():
            if k.isdigit():
                nombres[int(k)] = (e.get("name") or "?")[:11]
    except Exception:                                 # noqa: BLE001
        pass

    por = {}
    for f in sorted(os.listdir(base)):
        if not re.fullmatch(r"\d{4}", f):
            continue
        n = int(f)
        for g in os.listdir(os.path.join(base, f)):
            m = re.fullmatch(r"(Special[0-3])\.png", g)
            if m:
                por.setdefault(m.group(1), []).append(n)

    salida = os.path.join(RAIZ, "_claude_scratch", "specials")
    os.makedirs(salida, exist_ok=True)
    for slot in sorted(por):
        lista = sorted(por[slot])
        for parte in range((len(lista) + POR_HOJA - 1) // POR_HOJA):
            sub = lista[parte * POR_HOJA:(parte + 1) * POR_HOJA]
            filas = (len(sub) + COLS - 1) // COLS
            out = Image.new("RGB", (COLS * CEL, filas * CEL + 20),
                            (238, 238, 243))
            d = ImageDraw.Draw(out)
            d.text((4, 4), "%s  (%d de %d)" % (slot, len(sub), len(lista)),
                   (20, 20, 20))
            for i, n in enumerate(sub):
                p = os.path.join(base, "%04d" % n, slot + ".png")
                bg = Image.new("RGB", (CEL, CEL), (255, 255, 255))
                try:
                    im = Image.open(p).convert("RGBA")
                    im = im.resize((CEL - 16, CEL - 16), Image.NEAREST)
                    bg.paste(im, (8, 10), im)
                except Exception:                     # noqa: BLE001
                    pass
                dd = ImageDraw.Draw(bg)
                dd.text((2, 0), str(n), (190, 50, 50))
                dd.text((2, CEL - 11), nombres.get(n, ""), (70, 70, 90))
                out.paste(bg, ((i % COLS) * CEL, 20 + (i // COLS) * CEL))
            p = os.path.join(salida, "%s_%d.png" % (slot, parte))
            out.save(p)
            print(p)


if __name__ == "__main__":
    main()
