#!/usr/bin/env python3
"""Cruza la tabla de tipos del port con la del Anil.

El Anil describe cada tipo por su lado DEFENSOR (PBS/types.txt:
Weaknesses / Resistances / Immunities), asi que la fila de ataque de X
se reconstruye buscando quien nombra a X en cada lista. Aqui se compara
casilla a casilla con tools/type_chart.py, que es de donde sale
TYPE_CHART en dex.h.

Nacio porque la fila entera de ACERO estaba a x1: la tabla del port se
escribio a mano y sencillamente le faltaba la clave 'steel'. Un fallo
asi no se ve jugando —— un Iron Head que no es supereficaz contra un
hada parece un golpe flojo, no un juego roto —— y por eso hace falta
compararlo contra la fuente en vez de leerlo.

  python tools/chequea_tipos.py [ruta del PBS]
"""
import os
import sys

AQUI = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, AQUI)
import type_chart  # noqa: E402

PBS = sys.argv[1] if len(sys.argv) > 1 else \
    r"C:\Users\foulu\Downloads\Pokemon Anil V4.13\PBS\types.txt"


def lee_anil(path):
    """{tipo: (debilidades, resistencias, inmunidades)} en minusculas."""
    datos = {}
    actual = None
    pseudo = set()
    for linea in open(path, encoding="utf-8", errors="replace"):
        linea = linea.strip()
        if linea.startswith("[") and linea.endswith("]"):
            actual = linea[1:-1].lower()
            datos[actual] = {"weak": [], "res": [], "imm": []}
        elif actual and "=" in linea:
            k, v = [p.strip() for p in linea.split("=", 1)]
            campos = [x.strip().lower() for x in v.split(",") if x.strip()]
            if k == "Weaknesses":
                datos[actual]["weak"] = campos
            elif k == "Resistances":
                datos[actual]["res"] = campos
            elif k == "Immunities":
                datos[actual]["imm"] = campos
            elif k == "IsPseudoType" and v.lower() == "true":
                pseudo.add(actual)
    for p in pseudo:
        datos.pop(p, None)
    return datos


def main():
    if not os.path.exists(PBS):
        print("no encuentro", PBS)
        return 1
    anil = lee_anil(PBS)
    tipos = [t for t in type_chart.TIPOS if t != "none"]

    faltan = [t for t in tipos if t not in anil]
    if faltan:
        print("tipos que el PBS no tiene:", faltan)

    fallos = []
    for atacante in tipos:
        fila = type_chart.CHART.get(atacante)
        if fila is None:
            fallos.append("%s: NO TIENE FILA en type_chart.py (todo x1)"
                          % atacante.upper())
            fila = {}
        for defensor in tipos:
            if defensor not in anil:
                continue
            d = anil[defensor]
            if atacante in d["imm"]:
                debe = 0
            elif atacante in d["weak"]:
                debe = 200
            elif atacante in d["res"]:
                debe = 50
            else:
                debe = 100
            hay = fila.get(defensor, 100)
            if hay != debe:
                fallos.append("  %-9s -> %-9s  port x%-4s  Anil x%s"
                              % (atacante, defensor, hay / 100.0, debe / 100.0))

    if not fallos:
        print("la tabla de tipos coincide con el Anil en las %d casillas"
              % (len(tipos) ** 2))
        return 0
    print("\n".join(fallos))
    print("\n%d casillas distintas de %d" % (len(fallos), len(tipos) ** 2))
    return 1


if __name__ == "__main__":
    sys.exit(main())
