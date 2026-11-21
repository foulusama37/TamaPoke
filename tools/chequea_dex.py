#!/usr/bin/env python3
"""Que hay que meter en ALSO_WILD y en EVO_ONLY al anadir una region.

`DexEntry.evolvesTo` guarda UN solo destino, y de ahi salen dos agujeros
que no avisan de nada y que hay que mirar cada vez que entra una region:

  ALSO_WILD  La rareza se deduce de "le apunta alguien". Cuando llega un
             bebe, su forma adulta pasa a R_EVO y DESAPARECE de la hierba
             y de los huevos. Paso con Pichu y Pikachu en Johto, y con
             Azurill y Marill en Hoenn. En los juegos tampoco hay que
             criar un Munchlax para encontrarse un Snorlax.

  EVO_ONLY   Al reves: quien es la SEGUNDA rama de alguien no tiene a
             nadie que le apunte (el hueco lo ocupa la primera), asi que
             se le toma por forma base y sale en la hierba — y la piedra
             que hacia falta para conseguirlo no sirve para nada.

Esto los saca comparando el PBS del Anil (que si sabe de todas las ramas)
con lo que dex_data.py declara.

  python3 tools/chequea_dex.py
"""
import os
import sys

AQUI = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, AQUI)
import anil_pbs                                        # noqa: E402
from dex_data import DEX, ALSO_WILD, EVO_ONLY, RARE, LEGENDARY   # noqa: E402
from dex_range import N_DEX, REGIONES                  # noqa: E402

NOMBRE = {d[0]: d[2] for d in DEX}
APUNTA = {d[0]: d[4] for d in DEX}        # lo que dex_data dice que evoluciona a


def region_de(n):
    for nombre, fin in REGIONES:
        if n <= fin:
            return nombre
    return "?"


def main():
    num = anil_pbs.numeros()
    # {origen: [destinos]} segun el Anil, dentro de nuestra dex
    ramas = {}
    for n, _slug, c in anil_pbs.filas():
        campos = [x.strip() for x in c.get("Evolutions", "").split(",")]
        for i in range(0, max(0, len(campos) - 2), 3):
            d = num.get(anil_pbs.clave(campos[i])) if campos[i] else None
            if d and d <= N_DEX:
                ramas.setdefault(n, []).append(d)

    # quien recibe un apuntador de dex_data
    apuntados = {v for v in APUNTA.values() if v}
    # quien es evolucion de alguien SEGUN EL ANIL
    es_evolucion = {d for lista in ramas.values() for d in lista}

    # --- 1. los que el Anil dice que son evolucion y nadie apunta ---
    huerfanos = sorted(es_evolucion - apuntados - EVO_ONLY)
    print(f"--- {len(huerfanos)} ramas sin nadie que las apunte "
          f"(candidatas a EVO_ONLY) ---")
    for d in huerfanos:
        padres = [p for p, l in ramas.items() if d in l]
        quien = ", ".join(f"{NOMBRE.get(p, p)}->{NOMBRE.get(APUNTA.get(p, 0), '-')}"
                          for p in padres)
        print(f"  {d:3d} {NOMBRE.get(d, '?'):<12s} [{region_de(d)}]  "
              f"el hueco de su padre lo ocupa: {quien}")

    # --- 2. los que pasan a ser evolucion de un bebe nuevo ---
    # Son los que YA estaban (region anterior) y de pronto alguien les
    # apunta. Se detecta por: le apunta alguien de numero MAYOR que el.
    print(f"\n--- formas adultas a las que ahora apunta un bebe "
          f"(candidatas a ALSO_WILD) ---")
    n = 0
    for padre, hijo in sorted(APUNTA.items()):
        if hijo and padre > hijo and hijo not in ALSO_WILD:
            n += 1
            print(f"  {hijo:3d} {NOMBRE.get(hijo, '?'):<12s} [{region_de(hijo)}]"
                  f"  <- {NOMBRE.get(padre, padre)} ({padre}) [{region_de(padre)}]")
    if not n:
        print("  (ninguna)")

    # --- 3. las ramas de Eevee, que el codigo lleva en una tabla aparte ---
    eevee = sorted(ramas.get(133, []))
    print(f"\n--- las {len(eevee)} ramas de Eevee (EEVEE_BRANCH en gen_dex.py) ---")
    print("  " + ", ".join(f"{d} {NOMBRE.get(d, '?')}" for d in eevee))

    # --- 4. legendarios y raros que nadie ha clasificado ---
    sinclase = [d[0] for d in DEX
                if d[0] > REGIONES[-2][1]
                and d[0] not in LEGENDARY and d[0] not in RARE
                and d[0] not in es_evolucion and d[0] not in EVO_ONLY]
    print(f"\n--- {len(sinclase)} de la region nueva se quedan COMUNES ---")
    print("  " + ", ".join(NOMBRE.get(d, str(d)) for d in sinclase))
    return 0


if __name__ == "__main__":
    sys.exit(main())
