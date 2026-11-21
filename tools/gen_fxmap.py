#!/usr/bin/env python3
"""Saca de PkmnAnimations.rxdata QUE hoja usa cada golpe, segun el Anil.

Esto existia desde el principio y no lo estabamos usando: las animaciones de
combate de Essentials NO son los PNG sueltos de Graphics/Animations, son
objetos RPG::Animation guardados aqui, y cada uno dice como se llama
("Move:RAZORLEAF") y que hoja usa ("PRAS- Grass.png").

Mientras no se leyo, el generador casaba golpe con hoja POR EL NOMBRE del
fichero y lo que no casaba se quedaba con el efecto generico de su tipo:
Razor Leaf sacaba lianas del suelo cuando el Anil ya decia que su hoja es
"PRAS- Grass.png". Adivinar cuando el dato esta escrito es el error.

No hace falta parsear Marshal entero (hay objetos Ruby, y marshal_lite solo
cubre los tipos basicos): las cadenas van con su longitud delante y basta
recorrerlas en orden. De cada bloque "Move:X" se coge el PRIMER .png que
aparece antes del siguiente bloque.

  python3 tools/gen_fxmap.py          # escribe tools/fx_map.py
  python3 tools/gen_fxmap.py RAZORLEAF  # consulta uno
"""
import io
import os
import re
import sys

AQUI = os.path.dirname(os.path.abspath(__file__))
ANIL = r"C:\Users\foulu\Downloads\Pokemon Anil V4.13"
RX = os.path.join(ANIL, "Data", "PkmnAnimations.rxdata")
DST = os.path.join(AQUI, "fx_map.py")

# Los tres prefijos que marcan el principio de una animacion.
MARCA = re.compile(r"^(Move|OppMove|Common):(.+)$")


def cadenas(datos):
    """Las cadenas Marshal del blob, en orden. ('"' + longitud + bytes)."""
    out = []
    i, n = 0, len(datos)
    while i < n - 2:
        if datos[i] == 0x22:
            j = i + 1
            b = datos[j]
            if b == 0:
                largo, j = 0, j + 1
            elif 0x06 <= b <= 0x7A:
                largo, j = b - 5, j + 1
            elif b == 0x01:
                largo, j = datos[j + 1], j + 2
            elif b == 0x02:
                largo = datos[j + 1] | (datos[j + 2] << 8)
                j += 3
            else:
                i += 1
                continue
            if 0 < largo < 200 and j + largo <= n:
                s = datos[j:j + largo]
                if all(32 <= c < 127 for c in s):
                    out.append(s.decode("ascii"))
                    i = j + largo
                    continue
        i += 1
    return out


def extrae():
    """{NOMBREGOLPE: [hojas.png]} en el orden en que las usa el Anil.

    Se guardan TODAS, no solo la primera: una animacion de Essentials
    compone varias capas y la primera suele ser el fondo. Solar Beam empieza
    por "PRAS- Flash FG.png" y Confusion por "PRAS- Confusion BG.png", que
    son lavados a pantalla completa — inservibles para nosotros, que
    pintamos UNA capa encima del bicho. Quien elige es gen_art, que ya sabe
    descartar BG/FG.
    """
    cs = cadenas(open(RX, "rb").read())
    mapa = {}
    actual = None
    for s in cs:
        m = MARCA.match(s)
        if m:
            # "OppMove" es la misma animacion vista desde el otro lado: vale
            # igual, y se acumula en la misma entrada.
            tipo, nombre = m.group(1), m.group(2).strip().upper()
            actual = None if tipo == "Common" else re.sub(r"[^A-Z0-9]", "", nombre)
            continue
        if actual and s.lower().endswith(".png"):
            hojas = mapa.setdefault(actual, [])
            if s not in hojas:
                hojas.append(s)
    return mapa


def main():
    mapa = extrae()
    if len(sys.argv) > 1:
        for a in sys.argv[1:]:
            k = re.sub(r"[^A-Z0-9]", "", a.upper())
            print(f"  {a}: {mapa.get(k, '(no esta)')}")
        return 0

    carpeta = os.path.join(ANIL, "Graphics", "Animations")
    hay = {f.lower() for f in os.listdir(carpeta)}
    existe = sum(1 for hs in mapa.values() if any(h.lower() in hay for h in hs))
    with io.open(DST, "w", encoding="utf-8", newline="\n") as f:
        f.write("# -*- coding: utf-8 -*-\n")
        f.write('"""GENERADO por tools/gen_fxmap.py - no editar a mano.\n\n')
        f.write("Que hoja de Graphics/Animations usa cada golpe, SEGUN EL ANIL\n")
        f.write("(Data/PkmnAnimations.rxdata). Es el dato de verdad: antes se\n")
        f.write("adivinaba casando el nombre del golpe con el del fichero.\n")
        f.write('"""\n\n')
        f.write("FX_MAP = {\n")
        for k in sorted(mapa):
            f.write(f"    {k!r}: {mapa[k]!r},\n")
        f.write("}\n")
    print(f"{len(mapa)} golpes con hoja declarada; {existe} de esas hojas existen")
    print(f"escrito {DST}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
