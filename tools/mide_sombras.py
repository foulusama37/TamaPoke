#!/usr/bin/env python3
"""Cuanto se le DESPEGA la sombra a cada bicho, medido en pixeles.

El dueno: "as sombras no chao... mtos pokemon a sombra nao casa (pé
voando, sensacao que estao flutuando)".

Como se dibuja hoy (battle_ui.ino, btDrawSide):
  - el sprite se ancla por `base`, que es la fila mas baja CON TINTA de
    TODA la accion (si no, el bicho cambiaria de altura al animarse);
  - la sombra se pinta SIEMPRE en el suelo, y se ensancha y aclara con
    `liftAt(frame)`, las filas que ESE frame esta por encima de `base`.

O sea: en el frame que llega mas abajo, los pies tocan; en los demas el
bicho esta en el aire A PROPOSITO. El problema aparece cuando el bicho
NUNCA baja —— cuando todos los frames de su reposo estan altos ——,
porque entonces no es un salto: es que flota siempre.

Esto mide, por especie, el lift de CADA frame del reposo (PMD_IDLE) en
pixeles de PANTALLA, usando la misma escala que el combate. Lo que hay
que mirar es el MINIMO: si el minimo es 0, en algun momento apoya y el
ojo lo lee como un salto. Si el minimo es grande, flota siempre.

  python tools/mide_sombras.py              # el resumen
  python tools/mide_sombras.py --lista 40   # los 40 peores
"""
import io
import os
import re
import struct
import sys

AQUI = os.path.dirname(os.path.abspath(__file__))
RAIZ = os.path.dirname(AQUI)
SD = os.path.join(AQUI, "sdcard", "mons")
HUECO = 0xFF
PMD_IDLE = 0

# Los mismos de battle_ui.ino
BT_BACK_K = 1.05
BT_FOE_K = 0.72


def lee_acts(ruta):
    """{id: (w, h, nf, blob, base)}"""
    if not os.path.exists(ruta):
        return None
    d = open(ruta, "rb").read()
    if len(d) < 7 or d[:4] != b"TPK2":
        return None
    nActs = d[4]
    palCount = struct.unpack("<H", d[5:7])[0]
    p = 7 + palCount * 2
    acts = {}
    for _ in range(nActs):
        if p + 4 > len(d):
            break
        idd, w, h, nf = d[p], d[p + 1], d[p + 2], d[p + 3]
        p += 4 + nf * 2
        blob = d[p:p + w * h * nf]
        p += w * h * nf
        if len(blob) < w * h:
            continue
        base = 1
        for f in range(nf):
            fr = blob[f * w * h:(f + 1) * w * h]
            if len(fr) < w * h:
                break
            for r in range(h - 1, -1, -1):
                if any(b != HUECO for b in fr[r * w:(r + 1) * w]):
                    base = max(base, r + 1)
                    break
        acts[idd] = (w, h, nf, blob, base)
    return acts


def lift_de(a, f):
    """Filas que ESE frame esta por encima de `base` (igual que liftAt)."""
    w, h, nf, blob, base = a
    if f >= nf:
        return 0
    fr = blob[f * w * h:(f + 1) * w * h]
    if len(fr) < w * h:
        return 0
    for r in range(base - 1, -1, -1):
        if any(b != HUECO for b in fr[r * w:(r + 1) * w]):
            return base - 1 - r
    return base


def screen_sz():
    """dex -> screenSz de DEX_TBL (el alto de cuerpo que pide la especie)."""
    s = io.open(os.path.join(RAIZ, "dex.h"), encoding="utf-8",
                errors="replace").read()
    i = s.index("DEX_TBL[DEX_COUNT + 1] = {")
    j = s.index("};", i)
    out = []
    for m in re.finditer(r"\{[^}]*\}", s[i:j]):
        campos = [c.strip() for c in m.group(0)[1:-1].split(",")]
        # screenSz es el campo 13 (ver DexEntry en dex.h): tras los seis
        # stats base, rarity, accent, catch... se cuenta desde el final
        # porque los tipos y el femPct si son fijos.
        try:
            out.append(int(campos[12]))
        except (ValueError, IndexError):
            out.append(0)
    return out


def main():
    lista = 0
    if "--lista" in sys.argv:
        lista = int(sys.argv[sys.argv.index("--lista") + 1])
    sz = screen_sz()
    peores = []
    mirados = 0
    for dex in range(1, len(sz)):
        ruta = os.path.join(SD, "p%03d.bin" % dex)
        acts = lee_acts(ruta)
        if not acts or PMD_IDLE not in acts:
            continue
        a = acts[PMD_IDLE]
        w, h, nf, _blob, base = a
        quiere = sz[dex] * BT_BACK_K
        alto = base - 0
        # pixeles de pantalla por pixel de sprite, como escalaDibujo
        px = (quiere / max(1, _ink_h(a))) if _ink_h(a) else 1.0
        lifts = [lift_de(a, f) for f in range(nf)]
        if not lifts:
            continue
        mirados += 1
        minimo = min(lifts) * px
        maximo = max(lifts) * px
        if minimo >= 2.0:
            peores.append((minimo, maximo, dex, nf))

    print("mirados %d sprites de reposo" % mirados)
    print("  %d NUNCA apoyan: su frame mas bajo ya esta en el aire"
          % len(peores))
    if peores:
        import statistics
        ms = [p[0] for p in peores]
        print("  despegue minimo: mediana %.1f px, peor %.1f px"
              % (statistics.median(ms), max(ms)))
    if lista:
        nombres = _nombres()
        print()
        print("  los peores (despegue SIEMPRE presente, en px de pantalla):")
        for mn, mx, dex, nf in sorted(peores, reverse=True)[:lista]:
            print("    %-14s min %5.1f  max %5.1f  (%d frames)"
                  % (nombres[dex] if dex < len(nombres) else dex, mn, mx, nf))
    return 0


def _ink_h(a):
    w, h, nf, blob, base = a
    top = h
    for f in range(nf):
        fr = blob[f * w * h:(f + 1) * w * h]
        if len(fr) < w * h:
            break
        for r in range(h):
            if any(b != HUECO for b in fr[r * w:(r + 1) * w]):
                top = min(top, r)
                break
    return (base - top) if base > top else (h or 1)


def _nombres():
    s = io.open(os.path.join(RAIZ, "dex.h"), encoding="utf-8",
                errors="replace").read()
    i = s.index("DEX_TBL[DEX_COUNT + 1] = {")
    return re.findall(r'\{\s*"([^"]+)"', s[i:])


if __name__ == "__main__":
    sys.exit(main())
