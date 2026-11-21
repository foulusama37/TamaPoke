#!/usr/bin/env python3
"""Golpes cuyo efecto DEPENDE de quien los usa (o de donde).

Nace de lo que paso con MALDICION: el golpe tenia efecto modelado —— o
sea que tools/cuanto_hace.py lo daba por hecho —— pero era el efecto
EQUIVOCADO, porque en el Anil hace dos cosas distintas segun el tipo
del que lo usa y aqui solo se habia escrito una. Un golpe asi no se ve
roto: hace algo, y lo que hace es creible.

Lo que busca: clases de Battle::Move del Anil cuyo cuerpo mira al
USUARIO (su tipo, su especie, su forma, su objeto, su vida, su peso) o
al CAMPO (clima, terreno) para decidir QUE hace —— no solo cuanto pega.
Esas son las que pueden estar a medias sin que se note.

Para cada una dice que golpes la usan, de que categoria son y con que
efecto salieron en moves.h, para poder mirarlas una a una.

  python tools/golpes_que_dependen.py            # solo las de estado
  python tools/golpes_que_dependen.py --todas    # tambien las de dano
"""
import io
import os
import re
import sys

AQUI = os.path.dirname(os.path.abspath(__file__))
RAIZ = os.path.dirname(AQUI)

ANIL = r"C:\Users\foulu\Downloads\Pokemon Anil V4.13"
RUBY = os.path.join(ANIL, "Data", "Scripts")
PBS = os.path.join(ANIL, "PBS", "moves.txt")

# Lo que convierte a un golpe en "dos golpes en uno". No vale con mirar
# `user.` a secas: casi toda clase lo nombra para aplicarse a si misma.
# Estas son las preguntas que CAMBIAN el efecto.
SENALES = [
    (r"user\.pbHasType\?", "mira el TIPO del que lo usa"),
    (r"user\.isSpecies\?", "mira la ESPECIE"),
    (r"user\.form\b", "mira la FORMA"),
    (r"user\.(item|hasActiveItem)", "mira el OBJETO que lleva"),
    (r"def pbBaseType", "CAMBIA DE TIPO segun algo"),
    (r"def pbNumHits", "numero de golpes variable"),
    (r"@battle\.(pbWeather|field\.weather)", "mira el CLIMA"),
    (r"@battle\.field\.terrain", "mira el TERRENO"),
    (r"user\.pbWeight", "mira el PESO del que lo usa"),
    (r"user\.(hp|totalhp)\b", "mira la VIDA del que lo usa"),
    (r"user\.happiness", "mira la FELICIDAD"),
    (r"user\.pbOwnSide", "mira su BANDO"),
]


def clases_ruby():
    """{nombre de la clase: cuerpo}"""
    out = {}
    for raiz, _, ficheros in os.walk(RUBY):
        for f in ficheros:
            if not f.endswith(".rb"):
                continue
            txt = io.open(os.path.join(raiz, f), encoding="utf-8",
                          errors="replace").read()
            # las clases van una detras de otra en el mismo fichero
            trozos = re.split(r"^class Battle::Move::", txt, flags=re.M)
            for t in trozos[1:]:
                nombre = t.split(None, 1)[0].strip()
                out[nombre] = t
    return out


def moves_pbs():
    """[(nombre, categoria, function_code)]"""
    out = []
    nombre = cat = fc = None
    for linea in io.open(PBS, encoding="utf-8", errors="replace"):
        linea = linea.strip()
        if linea.startswith("[") and linea.endswith("]"):
            if nombre:
                out.append((nombre, cat, fc))
            nombre, cat, fc = linea[1:-1], None, None
        elif "=" in linea:
            k, v = [p.strip() for p in linea.split("=", 1)]
            if k == "Category":
                cat = v
            elif k == "FunctionCode":
                fc = v
    if nombre:
        out.append((nombre, cat, fc))
    return out


def efectos_del_port():
    """{NOMBRE: (fx, sx)} leido de moves.h"""
    out = {}
    txt = io.open(os.path.join(RAIZ, "moves.h"), encoding="utf-8",
                  errors="replace").read()
    for m in re.finditer(r'\{\s*"([A-Z0-9]+)",\s*\w+,\s*\d+,\s*\d+,\s*-?\d+,'
                         r'\s*(\w+),\s*(\w+),', txt):
        out[m.group(1)] = (m.group(2), m.group(3))
    return out


def main():
    todas = "--todas" in sys.argv
    if not os.path.isdir(RUBY):
        print("no encuentro los scripts del Anil en", RUBY)
        return 1
    clases = clases_ruby()
    port = efectos_del_port()

    porCodigo = {}
    for nombre, cat, fc in moves_pbs():
        if not fc:
            continue
        porCodigo.setdefault(fc, []).append((nombre, cat))

    filas = []
    for fc, usos in sorted(porCodigo.items()):
        cuerpo = clases.get(fc)
        if not cuerpo:
            continue
        motivos = [m for pat, m in SENALES if re.search(pat, cuerpo)]
        if not motivos:
            continue
        mios = [(n, c) for n, c in usos if n in port]
        if not mios:
            continue
        if not todas and not any(c == "Status" for _, c in mios):
            continue
        filas.append((fc, motivos, mios))

    print("%d clases del Anil cambian lo que HACEN segun quien las usa "
          "(o el campo), y tienen golpes nuestros:\n" % len(filas))
    for fc, motivos, mios in filas:
        print("  %s" % fc)
        print("      %s" % "; ".join(motivos))
        for n, c in mios:
            fx, sx = port[n]
            marca = "  <-- SIN EFECTO" if fx == "FX_NONE" and sx == "SX_NONE" else ""
            print("      %-16s %-8s %-22s %s%s" % (n, c, fx, sx, marca))
        print()
    return 0


if __name__ == "__main__":
    sys.exit(main())
