#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Que hay DE VERDAD dentro de SpriteCollab, y que nos sirve.

SpriteCollab (PMDCollab) es la coleccion de sprites y retratos al estilo
Mystery Dungeon. No es un monton de PNG sueltos: cada bicho es una
carpeta con un `AnimData.xml` que dice que animaciones tiene, de cuantos
fotogramas, a que velocidad y con que punto de apoyo —— o sea, lo mismo
que nosotros necesitamos para dibujarlo.

Lo importante para TamaPoke:

  - los sprites van en OCHO direcciones (las filas de cada hoja), que es
    mas de lo que gastamos, pero la fila 0 (de frente) y la 4 (de
    espaldas) son justo las dos vistas del combate;
  - hay animaciones que NO tenemos y que son puro tamagotchi: Sleep,
    Eat, Laugh, Hurt, Idle;
  - los retratos traen una emocion por fichero (Normal, Happy, Pain,
    Angry, Sad...), que es lo que le falta a la pantalla de casa.

Esto NO copia nada: recorre el arbol y dice que hay, cuanto pesa y que
cobertura tenemos contra nuestra Pokedex, para decidir con datos.

    python tools/catalogo_pmd.py G:/SpriteCollab/repo
"""
import collections
import io
import os
import re
import sys
import xml.etree.ElementTree as ET

AQUI = os.path.dirname(os.path.abspath(__file__))
RAIZ = os.path.dirname(AQUI)


def dex_del_puerto():
    """Los numeros de la Pokedex que embarcamos, sacados de dex.h."""
    p = os.path.join(RAIZ, "dex.h")
    if not os.path.exists(p):
        return None
    txt = io.open(p, encoding="utf-8", errors="replace").read()
    m = re.search(r"#define\s+DEX_COUNT\s+(\d+)", txt)
    return int(m.group(1)) if m else None


def lee_animdata(ruta):
    """[(nombre, fotogramas, ancho, alto)] de un AnimData.xml."""
    try:
        raiz = ET.parse(ruta).getroot()
    except Exception:                                 # noqa: BLE001
        return []
    out = []
    for an in raiz.iter("Anim"):
        nom = (an.findtext("Name") or "").strip()
        if not nom:
            continue
        dur = an.find("Durations")
        n = len(list(dur)) if dur is not None else 0
        w = an.findtext("FrameWidth")
        h = an.findtext("FrameHeight")
        out.append((nom, n, int(w) if w else 0, int(h) if h else 0))
    return out


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__.strip().splitlines()[-1])
    repo = sys.argv[1]
    dir_sprite = os.path.join(repo, "sprite")
    dir_retrato = os.path.join(repo, "portrait")
    if not os.path.isdir(dir_sprite):
        sys.exit("no veo %s —— ¿termino la descarga?" % dir_sprite)

    # --- los bichos que hay -------------------------------------------
    # El arbol es sprite/NNNN/[forma/]...; el numero de cuatro cifras es
    # el de la Pokedex nacional.
    conSprite = set()
    conRetrato = set()
    formas = collections.Counter()
    for base, destino in ((dir_sprite, conSprite), (dir_retrato, conRetrato)):
        if not os.path.isdir(base):
            continue
        for nom in os.listdir(base):
            if re.fullmatch(r"\d{4}", nom):
                destino.add(int(nom))
                sub = os.path.join(base, nom)
                extra = [d for d in os.listdir(sub)
                         if os.path.isdir(os.path.join(sub, d))]
                if base is dir_sprite:
                    formas[int(nom)] = len(extra)

    print("bichos con sprite : %d" % len(conSprite))
    print("bichos con retrato: %d" % len(conRetrato))
    n = dex_del_puerto()
    if n:
        dentro = [d for d in conSprite if 1 <= d <= n]
        print("de nuestros %d de la Pokedex, con sprite: %d (%.0f%%)"
              % (n, len(dentro), 100.0 * len(dentro) / n))
        faltan = sorted(set(range(1, n + 1)) - conSprite)
        print("sin sprite: %d%s" % (len(faltan),
                                    ("  p.ej. " + str(faltan[:12])) if faltan else ""))

    # --- que animaciones trae -----------------------------------------
    cuenta = collections.Counter()
    medidas = collections.defaultdict(list)
    leidos = 0
    for dirpath, _d, files in os.walk(dir_sprite):
        if "AnimData.xml" not in files:
            continue
        leidos += 1
        for nom, nfr, w, h in lee_animdata(os.path.join(dirpath, "AnimData.xml")):
            cuenta[nom] += 1
            if w and h:
                medidas[nom].append((w, h))

    print()
    print("animaciones (de %d carpetas con AnimData.xml):" % leidos)
    print("%-16s %7s  %s" % ("animacion", "bichos", "medida tipica"))
    for nom, c in cuenta.most_common(30):
        ms = medidas[nom]
        if ms:
            w = sorted(m[0] for m in ms)[len(ms) // 2]
            h = sorted(m[1] for m in ms)[len(ms) // 2]
            med = "%dx%d" % (w, h)
        else:
            med = "?"
        print("%-16s %7d  %s" % (nom, c, med))

    # --- retratos: que emociones --------------------------------------
    emos = collections.Counter()
    if os.path.isdir(dir_retrato):
        for dirpath, _d, files in os.walk(dir_retrato):
            for f in files:
                if f.lower().endswith(".png"):
                    emos[os.path.splitext(f)[0]] += 1
    if emos:
        print()
        print("emociones de retrato (top 24):")
        for e, c in emos.most_common(24):
            print("  %-18s %5d" % (e, c))

    # --- peso ----------------------------------------------------------
    def pesa(d):
        t = 0
        for dp, _dd, ff in os.walk(d):
            for f in ff:
                try:
                    t += os.path.getsize(os.path.join(dp, f))
                except OSError:
                    pass
        return t

    print()
    print("peso sprite : %.0f MB" % (pesa(dir_sprite) / 1048576.0))
    if os.path.isdir(dir_retrato):
        print("peso retrato: %.0f MB" % (pesa(dir_retrato) / 1048576.0))


if __name__ == "__main__":
    main()
