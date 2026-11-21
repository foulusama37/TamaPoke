#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Quien dibujo el retrato de cada bicho -> creditos.h

La licencia de SpriteCollab es CC BY-NC 4.0: no comercial Y con credito.
El credito es POR ESPECIE (tracker.json trae el autor de cada retrato),
asi que el sitio donde se pone es la Pokedex, que es la unica pantalla
que ya va por especie.

En tracker.json los autores vienen como ID de Discord
(`<@!1038291154032136192>`); credit_names.txt los traduce a nombre
legible. Los 132 que nos tocan traducen todos, asi que no hace falta
ensenar un ID a nadie.

Cuanto cuesta: 132 nombres distintos (~3 KB de texto) y hasta TRES
autores por especie, asi que tres indices de un byte por bicho. Unos
6 KB de flash en total.

    python tools/gen_creditos.py G:/SpriteCollab/repo
"""
import csv
import io
import json
import os
import re
import sys

AQUI = os.path.dirname(os.path.abspath(__file__))
RAIZ = os.path.dirname(AQUI)
MAX_AUT = 3          # el maximo que hay de verdad; medido, no supuesto


def dex_count():
    txt = io.open(os.path.join(RAIZ, "dex.h"), encoding="utf-8",
                  errors="replace").read()
    m = re.search(r"#define\s+DEX_COUNT\s+(\d+)", txt)
    if not m:
        sys.exit("no encuentro DEX_COUNT en dex.h")
    return int(m.group(1))


def legibles(repo):
    """ID de Discord (o nombre) -> nombre legible."""
    mapa = {}
    p = os.path.join(repo, "credit_names.txt")
    with io.open(p, encoding="utf-8") as f:
        r = csv.reader(f, delimiter="\t")
        next(r, None)
        for fila in r:
            if len(fila) < 2:
                continue
            nombre, ident = fila[0].strip(), fila[1].strip()
            if not nombre:
                # 42 filas vienen SIN nombre: el autor nunca corrio
                # `!register`. No se le puede tirar —— no acreditar es lo
                # unico que la licencia no deja —— asi que se le acredita
                # por el ID, que es lo que hace el propio repo.
                if ident:
                    mapa[ident] = "Discord " + _solo_digitos(ident)
                continue
            if ident:
                mapa[ident] = nombre
            mapa[nombre] = nombre
    return mapa


def _solo_digitos(ident):
    d = "".join(ch for ch in ident if ch.isdigit())
    return d or ident


def c_str(s):
    out = s.replace("\\", "\\\\").replace('"', '\\"')
    # El puerto pinta con su propia fuente: fuera lo que no sea ASCII.
    return "".join(ch if 32 <= ord(ch) < 127 else "?" for ch in out)


def main():
    if len(sys.argv) < 2:
        sys.exit("uso: python tools/gen_creditos.py <carpeta del repo>")
    repo = sys.argv[1]
    n = dex_count()
    mapa = legibles(repo)
    tr = json.load(io.open(os.path.join(repo, "tracker.json"),
                           encoding="utf-8"))

    autores = []         # nombre legible -> indice
    indice = {}
    porEspecie = [[] for _ in range(n + 1)]
    sin = []
    for k, e in tr.items():
        if not k.isdigit():
            continue
        d = int(k)
        if not (1 <= d <= n):
            continue
        c = e.get("portrait_credit") or {}
        crudos = [c.get("primary")] + list(c.get("secondary") or [])
        for t in crudos:
            if not t:
                continue
            nom = mapa.get(t)
            if nom is None:
                sin.append((d, t))
                continue
            if nom not in indice:
                indice[nom] = len(autores)
                autores.append(nom)
            if indice[nom] not in porEspecie[d]:
                porEspecie[d].append(indice[nom])
    if sin:
        print("OJO: %d autores sin nombre legible, p.ej. %s" % (len(sin), sin[:3]))
    if len(autores) > 254:
        sys.exit("mas de 254 autores: el indice ya no cabe en un byte")

    vacios = [d for d in range(1, n + 1) if not porEspecie[d]]
    hondo = max(len(v) for v in porEspecie)
    if hondo > MAX_AUT:
        sys.exit("hay una especie con %d autores y la tabla guarda %d"
                 % (hondo, MAX_AUT))

    L = []
    L.append("// GENERADO por tools/gen_creditos.py - no editar a mano.")
    L.append("//")
    L.append("// Quien dibujo el retrato de cada bicho. La licencia de")
    L.append("// SpriteCollab (CC BY-NC 4.0) pide credito, y el credito es por")
    L.append("// ESPECIE, asi que se ensena en la ficha de la Pokedex y no en")
    L.append("// una lista aparte que no abre nadie.")
    L.append("//")
    L.append("// 0xFF = hueco. La especie %d es la que mas autores tiene (%d)."
             % (max(range(1, n + 1), key=lambda d: len(porEspecie[d])), hondo))
    L.append("")
    L.append("#define CREDITO_AUTORES %d" % len(autores))
    L.append("#define CREDITO_MAX %d" % MAX_AUT)
    L.append("")
    L.append("static const char *const CREDITO_NOMBRE[CREDITO_AUTORES] = {")
    for a in autores:
        L.append('  "%s",' % c_str(a))
    L.append("};")
    L.append("")
    L.append("// [especie][hueco]; la especie 0 no existe y va a 0xFF.")
    L.append("static const uint8_t CREDITO_DE[%d][CREDITO_MAX] = {" % (n + 1))
    for d in range(n + 1):
        v = (porEspecie[d] + [0xFF] * MAX_AUT)[:MAX_AUT]
        L.append("  { %s }," % ", ".join(str(x) for x in v))
    L.append("};")
    L.append("")
    L.append("// La fuente, que los nombres sueltos no dicen de donde salen.")
    L.append('#define CREDITO_FUENTE "SpriteCollab (PMDCollab) - CC BY-NC 4.0"')
    L.append("")

    p = os.path.join(RAIZ, "creditos.h")
    io.open(p, "w", encoding="utf-8", newline="\n").write("\n".join(L))
    print("guardado", p)
    print("  %d autores, %d especies sin credito" % (len(autores), len(vacios)))
    print("  hasta %d autores por especie" % hondo)
    print("  ~%d B de nombres + %d B de indices"
          % (sum(len(a) + 1 for a in autores), (n + 1) * MAX_AUT))


if __name__ == "__main__":
    main()
