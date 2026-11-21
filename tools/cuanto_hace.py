#!/usr/bin/env python3
"""¿Cuantos golpes y cuantas habilidades HACEN algo de verdad?

Esta herramienta existe por una pregunta del usuario que yo habia
estado contestando de lado: "habilidad y golpes, ¿esta todo
funcionando?". Yo respondia con cobertura — "el 100% de los efectos se
ejecuta en la bateria" — y eso contesta otra cosa. Que un efecto se
EJECUTE no dice que exista: un golpe sin efecto modelado tambien se
"ejecuta", hace su dano y ya.

Lo que hay que separar son tres cosas distintas, y aqui se cuentan las
tres por separado:

  1. ESTA EN LA TABLA     el golpe/la habilidad existe y se puede usar
  2. HACE ALGO            tiene un efecto modelado, no solo un nombre
  3. ALGUIEN LO COMPRUEBA hay un test que afirma el NUMERO que sale

Las tres son numeros, no impresiones.

  python3 tools/cuanto_hace.py
"""
import io
import os
import re
import sys

AQUI = os.path.dirname(os.path.abspath(__file__))
RAIZ = os.path.dirname(AQUI)
sys.path.insert(0, AQUI)
import anil_pbs                        # noqa: E402
from dex_range import N_DEX            # noqa: E402


def lee(n):
    return io.open(os.path.join(RAIZ, n), encoding="utf-8",
                   errors="replace").read()


def pide(txt, patron, que):
    m = re.search(patron, txt, re.S)
    if not m:
        raise SystemExit(f"no encuentro {que}: esto no mira nada")
    return m.group(1)


def golpes():
    mv = lee("moves.h")
    cuerpo = pide(mv, r"MOVE_TBL\[[^\]]*\] = \{(.*?)\n\};", "MOVE_TBL")
    # Sin atarse al numero de campos: la fila crece con el tiempo.
    filas = re.findall(
        r'^\s*\{\s*"([^"]+)",\s*T_\w+,\s*(\d+),\s*\d+,\s*\d+,\s*'
        r'(\w+),\s*(\w+)\s*,', cuerpo, re.M)
    if not filas:
        raise SystemExit("no leo ni una fila de MOVE_TBL")
    return filas            # (nombre, potencia, fx, sx)


def habilidades():
    ab = lee("abilities.h")
    cuerpo = pide(ab, r"ABIL_TBL\[[^\]]*\] = \{(.*?)\n\};", "ABIL_TBL")
    filas = re.findall(r'^\s*\{\s*"([^"]*)"\s*,\s*(AB_[A-Z0-9_]+)\s*,',
                       cuerpo, re.M)
    if not filas:
        raise SystemExit("no leo ni una fila de ABIL_TBL")
    de = pide(ab, r"ABIL_OF\[[^\]]*\]\[3\] = \{(.*?)\n\};", "ABIL_OF")
    porEspecie = [tuple(int(x) for x in m) for m in
                  re.findall(r"\{\s*(\d+),\s*(\d+),\s*(\d+)\s*\}", de)]
    return filas, porEspecie


def main():
    # ---------------------------------------------------------- golpes
    g = golpes()
    conEfecto = [x for x in g if x[2] != "FX_NONE" or x[3] != "SX_NONE"]
    soloDano = [x for x in g if x[2] == "FX_NONE" and x[3] == "SX_NONE"
                and int(x[1]) > 0]
    nada = [x for x in g if x[2] == "FX_NONE" and x[3] == "SX_NONE"
            and int(x[1]) == 0]
    print(f"GOLPES: {len(g)} en la tabla")
    print(f"  {len(conEfecto):4d}  con efecto modelado "
          f"({100 * len(conEfecto) // len(g)}%)")
    print(f"  {len(soloDano):4d}  solo pegan, y es lo correcto: en el PBS "
          f"tampoco hacen otra cosa")
    if nada:
        print(f"  {len(nada):4d}  SIN potencia Y SIN efecto: no hacen NADA")
        for n, _p, _f, _s in nada[:10]:
            print(f"          {n}")

    # Los que el PBS tiene y la tabla no. Se descartan porque no harian
    # nada aqui, pero conviene saber CUANTOS y que son.
    enTabla = {re.sub(r"[^A-Z0-9]", "", n.upper()) for n, _p, _f, _s in g}
    fuera = {}
    for _d, _s, c in anil_pbs.filas():
        bruto = [x.strip() for x in c.get("Moves", "").split(",")]
        for i in range(1, len(bruto), 2):
            k = re.sub(r"[^A-Z0-9]", "", bruto[i].upper())
            if k and k not in enTabla:
                fuera[k] = fuera.get(k, 0) + 1
    print(f"  {len(fuera):4d}  golpes del PBS que NO entran (no harian nada "
          f"aqui: cambio de equipo, terrenos, objetos...)")

    # ----------------------------------------------------- habilidades
    tbl, porEspecie = habilidades()
    conMec = [n for n, fx in tbl if fx != "AB_NONE"]
    soloNombre = [n for n, fx in tbl if fx == "AB_NONE" and n not in ("-",)]
    print(f"\nHABILIDADES: {len(tbl) - 1} en el catalogo")
    print(f"  {len(conMec):4d}  con mecanica ({100 * len(conMec) // (len(tbl) - 1)}%)")
    print(f"  {len(soloNombre):4d}  solo el nombre: salen en la ficha y no "
          f"hacen nada todavia")

    # Y lo que de verdad importa: cuantos BICHOS se quedan sin ninguna
    # habilidad que funcione. Un 30% de habilidades implementadas suena
    # mal; si cubren a las especies que de verdad se juegan, suena de
    # otra forma. El numero decide.
    fx = {i: f for i, (_n, f) in enumerate(tbl)}
    sinNinguna = conAlguna = 0
    for d in range(1, min(N_DEX, len(porEspecie) - 1) + 1):
        suyas = [a for a in porEspecie[d] if a]
        if any(fx.get(a, "AB_NONE") != "AB_NONE" for a in suyas):
            conAlguna += 1
        else:
            sinNinguna += 1
    print(f"  de las {N_DEX} especies, {conAlguna} tienen al menos una "
          f"habilidad que hace algo y {sinNinguna} ninguna")

    # Y cuales faltan MAS. Una habilidad sin mecanica que la lleva un
    # bicho no importa; una que llevan ochenta, si. Esto es la lista de
    # por donde seguir, ordenada por lo que de verdad se nota jugando.
    cuantos = {}
    for d in range(1, min(N_DEX, len(porEspecie) - 1) + 1):
        for a in porEspecie[d]:
            if a and fx.get(a, "AB_NONE") == "AB_NONE":
                cuantos[a] = cuantos.get(a, 0) + 1
    top = sorted(cuantos.items(), key=lambda x: -x[1])[:20]
    print("\n  las que mas se notarian, por cuantas especies las llevan:")
    for a, n in top:
        print(f"     {n:4d}  {tbl[a][0]}")
    cubiertas = sum(n for _a, n in
                    [(a, c) for a, c in cuantos.items()])
    print(f"\n  (en total, {cubiertas} huecos de habilidad por especie "
          f"estan sin mecanica)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
