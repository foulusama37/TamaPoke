#!/usr/bin/env python3
"""Las especies que se quedan SIN NINGUNA habilidad que funcione.

cuanto_hace.py da el numero; esto da la lista y, lo que importa, QUE
habilidades les faltan. Un bicho con tres ranuras (dos normales y la
oculta) solo aparece aqui si NINGUNA de las tres hace nada: si la oculta
funcionara, ya no estaria sin habilidad.

La salida esta ordenada por lo que mas desbloquea: implementar la
habilidad de arriba quita de la lista a todas las especies que la
llevan. Es la misma idea que cuanto_hace.py pero mirando a los bichos
que se quedan a cero, no al catalogo entero.

  python3 tools/sin_habilidad.py        # el resumen
  python3 tools/sin_habilidad.py -v     # y la lista de especies
"""
import collections
import io
import os
import re
import sys

AQUI = os.path.dirname(os.path.abspath(__file__))
RAIZ = os.path.dirname(AQUI)

VERBOSO = "-v" in sys.argv


# Habilidades cuyo efecto SOLO existe teniendo COMPAÑERO en la pista.
# Aqui los combates son de uno contra uno, asi que no estan "sin
# hacer": no hay donde hacerlas. Va en una lista aparte y VISIBLE para
# que el numero de especies a cero pueda llegar a cero de verdad, igual
# que se hizo con los golpes en sin_efecto.py.
#
# La lista ADELGAZO, y eso es lo interesante: aqui estaban Bateria,
# Punto Fuerte y Bailarin, y las tres se podian hacer. El Bailarin
# porque su ficha dice "otro Pokemon presente en el combate" y en un
# 1v1 ese otro es el RIVAL (lo lei mal). Las otras dos porque "aliado"
# en este motor no es un companero en la pista: es EL SIGUIENTE QUE
# ENTRA del banquillo, y de eso si hay. Antes de meter algo aqui, leer
# la ficha dos veces.
SOLO_COMPANERO = ("Healer", "Hospitality")

# Las que piden un sistema de FORMAS en mitad del combate, que el
# motor todavia no tiene: megaEvolve() cambia de forma UNA vez y atado
# a una piedra, no de ida y vuelta segun el clima o los PS. No es que
# falten estas habilidades: falta la pieza de abajo. Se listan para que
# el numero diga QUE hace falta y no solo cuanto.
PIDEN_FORMAS = ("Forecast", "Flower Gift", "Multitype", "Stance Change",
                "Power Construct", "Schooling", "Shields Down",
                "Disguise", "Gulp Missile", "Hunger Switch",
                "Zero to Hero", "Tera Shift", "RKS System", "Illusion")


def lee(n):
    return io.open(os.path.join(RAIZ, n), encoding="utf-8",
                   errors="replace").read()


def main():
    ab = lee("abilities.h")
    # nombre y mecanica de cada fila
    cuerpo = ab[ab.index("ABIL_TBL"):]
    filas = re.findall(r'\{\s*"([^"]*)",\s*(AB_\w+),', cuerpo)
    if not filas:
        raise SystemExit("no leo ABIL_TBL: cambio el formato de la tabla?")
    nombre = {i: n for i, (n, _f) in enumerate(filas)}
    mec = {i: f for i, (_n, f) in enumerate(filas)}

    # las tres ranuras de cada especie
    de = ab[ab.index("ABIL_OF"):]
    porEspecie = [tuple(int(x) for x in m) for m in
                  re.findall(r"\{\s*(\d+),\s*(\d+),\s*(\d+)\s*\}", de)]
    if len(porEspecie) < 2:
        raise SystemExit("no leo ABIL_OF")

    dex = lee("dex.h")
    nombres = re.findall(r'^\s*\{\s*"([^"]+)",', dex[dex.index("DEX_TBL"):],
                         re.M)

    sin = []
    for d in range(1, len(porEspecie)):
        suyas = [a for a in porEspecie[d] if a]
        if not suyas:
            continue                      # sin ninguna ranura: otro problema
        if any(mec.get(a, "AB_NONE") != "AB_NONE" for a in suyas):
            continue
        sin.append((d, suyas))

    # Se parten en tres: las que de verdad faltan, las que no caben en
    # un uno contra uno y las que esperan al sistema de formas.
    def clase(suyas):
        nn = [nombre.get(a, "") for a in suyas]
        if all(x in SOLO_COMPANERO for x in nn):
            return "companero"
        if all(x in PIDEN_FORMAS or x in SOLO_COMPANERO for x in nn):
            return "formas"
        return "faltan"

    grupos = {"faltan": [], "companero": [], "formas": []}
    for d, suyas in sin:
        grupos[clase(suyas)].append((d, suyas))

    print(f"{len(sin)} especies no tienen NINGUNA de sus habilidades hecha")
    print("(se miran las tres ranuras: las dos normales y la oculta)")
    print(f"   {len(grupos['faltan']):3d} por hacer de verdad")
    print(f"   {len(grupos['companero']):3d} su habilidad solo existe con COMPAÑERO: en un 1v1 no hay donde")
    print(f"   {len(grupos['formas']):3d} esperan el sistema de FORMAS en mitad del combate")
    print()
    for k, titulo in (("companero", "solo con compañero"),
                      ("formas", "piden el sistema de formas")):
        if grupos[k]:
            print(f"  {titulo}:")
            for d, suyas in grupos[k]:
                nom = nombres[d - 1] if d - 1 < len(nombres) else f"#{d}"
                print(f"     {d:4d} {nom:<14s} " +
                      ", ".join(nombre.get(a, chr(63)) for a in suyas))
            print()
    sin = grupos["faltan"]

    # que habilidad desbloquea a mas de ellas
    cuantas = collections.Counter()
    for _d, suyas in sin:
        for a in set(suyas):
            cuantas[a] += 1
    print("lo que mas desbloquearia, por especies que se quitarian de la lista:")
    for a, n in cuantas.most_common(25):
        print(f"   {n:3d}  {nombre.get(a, '?'):<18s} (fila {a})")
    if len(cuantas) > 25:
        print(f"   ... y {len(cuantas) - 25} habilidades mas")

    if VERBOSO:
        print("\nlas especies, una a una:")
        for d, suyas in sin:
            nom = nombres[d - 1] if d - 1 < len(nombres) else f"#{d}"
            print(f"   {d:4d} {nom:<14s} " +
                  ", ".join(nombre.get(a, '?') for a in suyas))

    # LAS MEGAS, que es un agujero aparte: megaEvolve() le CAMBIA la
    # habilidad al bicho, asi que una mega puede ganar una que no haga
    # nada — y megaevolucionar es el momento grande del combate. Habia 30
    # de 127 asi y no lo miraba nadie.
    mg = lee("megas.h")
    cuerpoM = mg[mg.index("MEGA_TBL"):]
    filasM = re.findall(
        r"\{\s*(-?\d+),\s*\w+\s*,(?:\s*\d+,){8}\s*T_\w+\s*,"
        r"\s*T_\w+\s*,\s*(\d+),\s*\d+,\s*\"([^\"]+)\"", cuerpoM)
    if not filasM:
        mal = "no leo MEGA_TBL: cambio el formato?"
        print("  OJO:", mal)
    else:
        mudas = [(nm, nombre.get(int(h), "(ninguna)"))
                 for _b, h, nm in filasM
                 if int(h) == 0 or mec.get(int(h), "AB_NONE") == "AB_NONE"]
        print(f"{len(filasM)} megas; {len(mudas)} ganan una habilidad que "
              f"NO hace nada")
        for nm, hb in mudas:
            print(f"   {nm:<16s} {hb}")
        print()

    # Cuantas se arreglarian con las CINCO de arriba: es el numero que
    # decide si vale la pena otra tanda o si hay que buscar otra via.
    top5 = [a for a, _ in cuantas.most_common(5)]
    arregla = sum(1 for _d, suyas in sin if any(a in top5 for a in suyas))
    print(f"\ncon las 5 de arriba se quedarian {len(sin) - arregla} "
          f"especies sin ninguna")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
