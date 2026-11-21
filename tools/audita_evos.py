#!/usr/bin/env python3
"""Compara TODAS las evoluciones del Anil con las que de verdad hay en el juego.

No mira si el generador hizo lo que queria: mira el RESULTADO —— dex.h y
evos.h, que es lo que la placa lee —— contra el PBS, especie por especie.

Lo que comprueba, por metodo:

  Level...        dex.h tiene que llevar al MISMO destino y al MISMO nivel
  Item / Trade+objeto / HoldItem
                  evos.h tiene que tener (de, objeto, a); sin radio no hay
                  intercambio, asi que el objeto se usa desde la bolsa
  Trade sin objeto
                  le toca el Cable (EVOI_CABLE)
  lo demas        (amistad, un golpe aprendido, un sitio del mapa...) no
                  tiene mecanica en el reloj: se LISTA, para saber cuales
                  son y decidir, en vez de que se pierdan en silencio

  python3 tools/audita_evos.py          # resumen
  python3 tools/audita_evos.py -v       # y el detalle de cada fallo
"""
import io
import os
import re
import sys

AQUI = os.path.dirname(os.path.abspath(__file__))
RAIZ = os.path.dirname(AQUI)
sys.path.insert(0, AQUI)
import anil_pbs

# Metodos del PBS que acaban siendo "sube de nivel" en el reloj.
POR_NIVEL = {"Level", "LevelDay", "LevelNight", "LevelMale", "LevelFemale",
             "LevelRain", "LevelDarkInParty", "LevelUpholdItem"}
# Los que acaban pidiendo un OBJETO (el intercambio no existe aqui).
POR_OBJETO = {"Item", "ItemMale", "ItemFemale", "ItemDay", "ItemNight",
              "TradeItem", "DayHoldItem", "NightHoldItem", "HoldItem"}
POR_CABLE = {"Trade"}


def dex_tabla():
    """{dex: (nombre, evolvesTo, evolveLevel)} leyendo dex.h."""
    txt = io.open(os.path.join(RAIZ, "dex.h"), encoding="utf-8",
                  errors="replace").read()
    ini = txt.index("DEX_TBL[")
    out, n = {}, 0
    for m in re.finditer(r'^\s*\{\s*"([A-Z0-9\'\.\- ]+)",\s*(-?\d+),\s*(-?\d+),',
                         txt[ini:], re.M):
        n += 1
        out[n] = (m.group(1), int(m.group(2)), int(m.group(3)))
    return out


def evos_item():
    """{(de, a)} y {de: [(objeto, a)]} leyendo evos.h."""
    txt = io.open(os.path.join(RAIZ, "evos.h"), encoding="utf-8",
                  errors="replace").read()
    pares, por_de = set(), {}
    for m in re.finditer(r'^\s*\{\s*(\d+),\s*([A-Za-z_0-9]+)\s*,\s*(\d+),\s*(\d+)\s*\}',
                         txt, re.M):
        de, obj, a = int(m.group(1)), m.group(2), int(m.group(3))
        pares.add((de, a))
        por_de.setdefault(de, []).append((obj, a))
    return pares, por_de


def ramas_dex():
    """Las dos tablas de dex.h que el resto del auditor no veia:

      DEX_SEXADA  en que evoluciona el MACHO (Burmy -> Mothim)
      EVO_RAMA    las ramas que decide un stat, el azar o un extra
                  (Tyrogue, Wurmple, Nincada+Shedinja)

    Sin esto el auditor daba cuatro fallos que no lo eran: el juego SI los
    resuelve, solo que no por `evolvesTo`.
    """
    txt = io.open(os.path.join(RAIZ, "dex.h"), encoding="utf-8",
                  errors="replace").read()
    sex, ramas = set(), set()
    m = re.search(r"DEX_SEXADA\[[^\]]*\]\s*=\s*\{(.*?)\};", txt, re.S)
    if m:
        for a, b in re.findall(r"\{\s*(\d+)\s*,\s*(\d+)\s*\}", m.group(1)):
            sex.add((int(a), int(b)))
    m = re.search(r"EVO_RAMA\[[^\]]*\]\s*=\s*\{(.*?)\};", txt, re.S)
    if m:
        for a, _c, b in re.findall(
                r"\{\s*(\d+)\s*,\s*([A-Z_]+)\s*,\s*(\d+)\s*\}", m.group(1)):
            ramas.add((int(a), int(b)))
    return sex | ramas


def evoluciones_pbs():
    """[(dex, nombre, destino_nombre, metodo, parametro)] del PBS."""
    out = []
    for n, slug, c in anil_pbs.filas():
        campo = c.get("Evolutions", "").strip()
        if not campo:
            continue
        trozos = [t.strip() for t in campo.split(",")]
        # van de tres en tres: DESTINO, Metodo, Parametro (puede ir vacio)
        for i in range(0, len(trozos) - 1, 3):
            destino = trozos[i]
            metodo = trozos[i + 1] if i + 1 < len(trozos) else ""
            param = trozos[i + 2] if i + 2 < len(trozos) else ""
            if destino and metodo:
                out.append((n, slug, destino, metodo, param))
    return out


def main():
    verbose = "-v" in sys.argv
    dex = dex_tabla()
    # Los nombres se comparan SIN puntuacion: nuestra dex escribe
    # "PORYGON-Z", "MR. RIME" y "JANGMO-O" y el PBS "PORYGONZ", "MRRIME",
    # "JANGMOO". Son el mismo bicho y no vale contarlos como fallo.
    norm = lambda t: re.sub(r"[^A-Z0-9]", "", t.upper())
    por_nombre = {norm(v[0]): k for k, v in dex.items()}
    pares_item, por_de = evos_item()
    pares_rama = ramas_dex()

    bien = mal = 0
    fallos, sin_mecanica = [], []
    por_metodo = {}

    for n, slug, destino, metodo, param in evoluciones_pbs():
        por_metodo[metodo] = por_metodo.get(metodo, 0) + 1
        dst = por_nombre.get(norm(destino))
        if dst is None:
            fallos.append((slug, destino, metodo, param,
                           "el destino no esta en nuestra dex"))
            mal += 1
            continue

        if metodo in POR_NIVEL:
            _nom, to, lv = dex[n]
            if to != dst and (n, dst) in pares_rama:
                bien += 1          # va por DEX_SEXADA o por EVO_RAMA
            elif to != dst:
                fallos.append((slug, destino, metodo, param,
                               "dex.h lleva a %d y no a %d" % (to, dst)))
                mal += 1
            elif param.isdigit() and lv != int(param):
                fallos.append((slug, destino, metodo, param,
                               "dex.h dice nivel %d y el Anil %s" % (lv, param)))
                mal += 1
            else:
                bien += 1
        elif metodo in POR_OBJETO:
            if (n, dst) in pares_item:
                bien += 1
            else:
                fallos.append((slug, destino, metodo, param,
                               "no esta en evos.h"))
                mal += 1
        elif metodo in POR_CABLE:
            objs = [o for o, a in por_de.get(n, []) if a == dst]
            if any(o == "EVOI_CABLE" for o in objs):
                bien += 1
            elif objs:
                fallos.append((slug, destino, metodo, param,
                               "va por %s y el Anil dice intercambio" % objs[0]))
                mal += 1
            else:
                fallos.append((slug, destino, metodo, param, "no esta en evos.h"))
                mal += 1
        else:
            # ni nivel, ni objeto, ni cable: no hay mecanica para esto
            _nom, to, lv = dex[n]
            tiene_otra = ((to == dst and lv > 0) or (n, dst) in pares_item
                          or (n, dst) in pares_rama)
            if tiene_otra:
                bien += 1          # se llega por otro camino, que vale
            else:
                sin_mecanica.append((slug, destino, metodo, param))

    print("evoluciones del Anil revisadas: %d" % (bien + mal + len(sin_mecanica)))
    print("  correctas            %d" % bien)
    print("  MAL                  %d" % mal)
    print("  sin mecanica aqui    %d" % len(sin_mecanica))
    print()
    print("por metodo en el PBS:")
    for k in sorted(por_metodo, key=lambda x: -por_metodo[x]):
        print("  %-18s %4d" % (k, por_metodo[k]))

    if fallos:
        print()
        print("FALLOS (%d):" % len(fallos))
        for f in (fallos if verbose else fallos[:25]):
            print("  %-14s -> %-14s %-14s %-12s %s" % f)
        if not verbose and len(fallos) > 25:
            print("  ... y %d mas (-v para verlos)" % (len(fallos) - 25))

    if sin_mecanica:
        print()
        print("SIN MECANICA en el reloj (%d) —— no hay forma de evolucionarlos:"
              % len(sin_mecanica))
        agrupado = {}
        for slug, destino, metodo, param in sin_mecanica:
            agrupado.setdefault(metodo, []).append(slug)
        for metodo in sorted(agrupado, key=lambda x: -len(agrupado[x])):
            ls = agrupado[metodo]
            print("  %-20s %3d  %s%s" % (metodo, len(ls), ", ".join(ls[:6]),
                                         " ..." if len(ls) > 6 else ""))
    return 1 if mal else 0


sys.exit(main())
