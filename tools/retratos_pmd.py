#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Los RETRATOS de SpriteCollab, medidos contra lo que necesitamos.

El plan del dueno no es cambiar los sprites —— los de combate y los de
casa se quedan —— sino usar los retratos para las REACCIONES: hambre,
sueno, sin PS, enfadado porque esta sucio o desatendido, y celebrando
en la pantalla de experiencia y premios.

Asi que lo que importa no es cuantos ficheros hay, sino:

  - cuantas de NUESTRAS especies tienen cada emocion en su forma base
    (una emocion que solo cubre la mitad de la Pokedex no sirve de
    reaccion: el resto se quedaria mudo);
  - cuanto mide un retrato, para saber si cabe y a que escala;
  - cuanto pesaria embarcarlos.

Solo mira y cuenta. No copia nada.

    python tools/retratos_pmd.py G:/SpriteCollab/repo
"""
import collections
import io
import os
import re
import sys

AQUI = os.path.dirname(os.path.abspath(__file__))
RAIZ = os.path.dirname(AQUI)

# Lo que queremos decir en pantalla -> que emocion lo dice.
# Es una PROPUESTA: se confirma mirando los ficheros.
QUEREMOS = [
    ("hambre",            "Sigh"),
    ("sueno",             "Dizzy"),
    ("sin PS / dolorido", "Pain"),
    ("sucio / desatendido", "Angry"),
    ("celebrando (exp)",  "Joyous"),
    ("contento",          "Happy"),
    ("pokedex / quieto",  "Normal"),
    ("triste",            "Sad"),
    ("asustado",          "Surprised"),
    ("decidido (combate)", "Determined"),
]


def dex_count():
    p = os.path.join(RAIZ, "dex.h")
    txt = io.open(p, encoding="utf-8", errors="replace").read()
    m = re.search(r"#define\s+DEX_COUNT\s+(\d+)", txt)
    return int(m.group(1))


def main():
    if len(sys.argv) < 2:
        sys.exit("uso: python tools/retratos_pmd.py <carpeta del repo>")
    base = os.path.join(sys.argv[1], "portrait")
    if not os.path.isdir(base):
        sys.exit("no veo " + base)

    n = dex_count()
    # Forma BASE = los PNG que cuelgan directamente de portrait/NNNN/.
    porEmocion = collections.Counter()
    tam = collections.Counter()
    peso = 0
    especies = set()
    for nom in os.listdir(base):
        if not re.fullmatch(r"\d{4}", nom):
            continue
        d = int(nom)
        if not (1 <= d <= n):
            continue
        carpeta = os.path.join(base, nom)
        hay = False
        for f in os.listdir(carpeta):
            if not f.lower().endswith(".png"):
                continue
            emo = os.path.splitext(f)[0]
            if emo.endswith("^"):
                continue          # la variante espejada no es otra emocion
            porEmocion[emo] += 1
            hay = True
            ruta = os.path.join(carpeta, f)
            peso += os.path.getsize(ruta)
            try:
                from PIL import Image
                with Image.open(ruta) as im:
                    tam[im.size] += 1
            except Exception:                         # noqa: BLE001
                pass
        if hay:
            especies.add(d)

    print("especies de NUESTRA Pokedex con retrato base: %d de %d (%.0f%%)"
          % (len(especies), n, 100.0 * len(especies) / n))
    print()
    print("medidas de los ficheros:")
    for s, c in tam.most_common(6):
        print("  %-12s %6d" % ("%dx%d" % s, c))
    print()
    print("peso de los retratos de forma base: %.0f MB" % (peso / 1048576.0))
    print()
    print("cobertura por emocion (sobre %d especies):" % n)
    print("%-22s %-14s %7s  %s" % ("para que", "emocion", "bichos", "cobertura"))
    for que, emo in QUEREMOS:
        c = porEmocion.get(emo, 0)
        print("%-22s %-14s %7d  %3.0f%%" % (que, emo, c, 100.0 * c / n))
    print()
    print("TODAS las emociones que existen, por cobertura:")
    for emo, c in porEmocion.most_common():
        if c < n * 0.02:
            continue
        print("  %-18s %6d  %3.0f%%" % (emo, c, 100.0 * c / n))


if __name__ == "__main__":
    main()
