#!/usr/bin/env python3
"""Que golpes se quedan SIN su efecto, y por que.

Esta herramienta existe porque yo llevaba varias respuestas diciendo
"220 solo pegan, y es lo correcto: en el PBS tampoco hacen otra cosa"
sin haberlo comprobado nunca. El usuario insistio en que todo tiene que
funcionar y tenia razon en desconfiar: la frase era una suposicion.

Se separan tres grupos, porque son tres problemas distintos:

  1. EN LA TABLA Y SIN EFECTO, con FunctionCode None en el PBS.
     Estos si estan bien: el Anil tampoco les da nada.

  2. EN LA TABLA Y SIN EFECTO, pero el PBS SI les da un FunctionCode.
     Estos estan MAL: el golpe existe, se puede usar, y hace menos de
     lo que deberia sin que nada lo diga.

  3. FUERA DE LA TABLA. gen_moves los descarta por no tener ni dano ni
     efecto modelado. Cada uno lleva su FunctionCode para poder decidir
     si de verdad no pinta nada aqui o si es que falta implementarlo.

  python3 tools/sin_efecto.py
  python3 tools/sin_efecto.py -v    # lista los golpes, no solo el codigo
"""
import collections
import io
import os
import re
import sys

AQUI = os.path.dirname(os.path.abspath(__file__))
RAIZ = os.path.dirname(AQUI)
sys.path.insert(0, AQUI)
import anil_pbs                        # noqa: E402

VERBOSO = "-v" in sys.argv

# Trozos de FunctionCode cuyo efecto SOLO existe en un combate de varios:
# un compañero al que potenciar o al que pegar sin querer, o varios
# rivales a los que alcanzar de golpe. Aqui los combates son de uno contra
# uno, asi que estos golpes ya hacen todo lo que pueden hacer: su dano.
#
# Esto no es una excusa para no hacerlos: es que no hay donde hacerlos. Va
# en una lista aparte y VISIBLE para que el recuento de "golpes a medias"
# pueda llegar a cero de verdad en vez de quedarse clavado en un numero
# que nadie puede bajar. Si algun dia entran los combates dobles, se
# borra esta lista y vuelven todos a la cuenta.
SOLO_DOBLES = (
    "Ally",                 # Refuerzo, Pirotecnia, Canon, Danza Amiga...
    "AllFoes",              # Vasta Fuerza y los que barren el campo
    "CannotBeRedirected",   # no hay a quien redirigir el golpe
    "HitOncePerUserTeamMember",   # Paliza: cuenta el equipo EN LA PISTA
    "ProtectUserSideFromMultiTargetDamagingMoves",   # Vasta Guardia
)


def clave(s):
    return re.sub(r"[^A-Z0-9]", "", s.upper())


def main():
    mv = io.open(os.path.join(RAIZ, "moves.h"), encoding="utf-8",
                 errors="replace").read()
    cuerpo = mv[mv.index("MOVE_TBL"):]
    filas = re.findall(
        r'^\s*\{\s*"([^"]+)",\s*T_\w+,\s*(\d+),\s*\d+,\s*\d+,\s*'
        r'(\w+),\s*(\w+)\s*,', cuerpo, re.M)
    if not filas:
        raise SystemExit("no leo MOVE_TBL: esto no esta mirando nada")
    enTabla = {clave(n): (int(p), fx, sx) for n, p, fx, sx in filas}

    pbs = {}
    for cab, c in anil_pbs.bloques(
            os.path.join(anil_pbs.ANIL, "PBS", "moves.txt")):
        pbs[clave(cab)] = c

    # Cuantas especies aprenden cada golpe: un golpe roto que no sabe
    # nadie importa mucho menos que uno que saben ochenta.
    cuantas = collections.Counter()
    for _d, _s, c in anil_pbs.filas():
        bruto = [x.strip() for x in c.get("Moves", "").split(",")]
        for i in range(1, len(bruto), 2):
            if bruto[i]:
                cuantas[clave(bruto[i])] += 1

    bien, mal, fuera, solos = [], [], [], []
    for k, c in pbs.items():
        fc = (c.get("FunctionCode") or "None").strip()
        nom = c.get("Name", k)
        n = cuantas.get(k, 0)
        if k in enTabla:
            _pot, fx, sx = enTabla[k]
            if fx != "FX_NONE" or sx != "SX_NONE":
                continue                      # tiene efecto: nada que ver
            if fc in ("None", ""):
                bien.append((n, k, nom, fc))
            elif any(p in fc for p in SOLO_DOBLES):
                solos.append((n, k, nom, fc))
            else:
                mal.append((n, k, nom, fc))
        else:
            if n:                             # lo aprende alguien
                # La misma regla que arriba: si el efecto solo existe
                # teniendo compañero, no esta "a medias", esta fuera de
                # lo que un uno contra uno puede ser.
                (solos if any(p in fc for p in SOLO_DOBLES)
                 else fuera).append((n, k, nom, fc))

    bien.sort(reverse=True)
    mal.sort(reverse=True)
    fuera.sort(reverse=True)
    solos.sort(reverse=True)

    print(f"{len(enTabla)} golpes en la tabla\n")
    print(f"BIEN: {len(bien)} sin efecto, y el PBS tampoco les da ninguno")
    print(f"MAL:  {len(mal)} en la tabla, usables, y el PBS SI les da un "
          f"efecto que aqui no pasa")
    for n, _k, nom, fc in (mal if VERBOSO else mal[:25]):
        print(f"   {n:4d} especies  {nom:<18s} {fc}")
    if not VERBOSO and len(mal) > 25:
        print(f"   ... y {len(mal) - 25} mas (con -v salen todos)")

    # Esta lista NO es una excusa: es que aqui los combates son de uno
    # contra uno, y un efecto que solo existe teniendo compañero o varios
    # rivales no tiene donde pasar. Dejarlos contados en MAL haria que el
    # numero nunca pudiera bajar a cero y dejaria de significar nada.
    # Si algun dia hay combates dobles, se borra esta lista y vuelven.
    print(f"\nNO APLICA en 1 contra 1: {len(solos)} (el efecto necesita "
          f"compañero o varios rivales)")
    for n, _k, nom, fc in solos:
        print(f"   {n:4d} especies  {nom:<18s} {fc}")

    print(f"\nFUERA de la tabla: {len(fuera)} que alguien aprende y aqui "
          f"no existen")
    porCodigo = collections.Counter()
    for n, _k, _nom, fc in fuera:
        porCodigo[fc] += n
    for fc, n in porCodigo.most_common(25 if not VERBOSO else 999):
        cuales = [x[2] for x in fuera if x[3] == fc]
        print(f"   {n:5d} huecos de especie  {fc}")
        print(f"          {', '.join(cuales[:6])}"
              + (" ..." if len(cuales) > 6 else ""))
    return 0


if __name__ == "__main__":
    sys.exit(main())
