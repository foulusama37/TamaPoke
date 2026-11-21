#!/usr/bin/env python3
"""Escribe las filas de DEX (dex_data.py) de un tramo, desde el PBS del Anil.

Las 251 primeras se escribieron A MANO y salio caro: 39 niveles de
evolucion inventados, Eevee subiendo por nivel cuando el Anil solo le da
objetos, y nombres de slug a ojo. Hoenn son 135 mas, asi que se generan.

Lo que saca de cada seccion del PBS:
  slug / nombre   del propio nombre de la seccion y de `Name`
  tipo (color)    de `Types`, el primero
  evoluciona a    de `Evolutions`, el primer destino que caiga en el tramo
  nivel           del mismo sitio, y 0 si no hay ninguno por nivel
                  (gen_dex.py vuelve a leerlo de aqui de todas formas)

El PBS esta en orden de dex NACIONAL: eso no se supone, se comprueba
contra las especies que ya tenemos antes de emitir nada.

  python3 tools/gen_filas.py 252 386        # las imprime
  python3 tools/gen_filas.py 252 386 -w     # las mete en dex_data.py
"""
import io
import os
import re
import sys

AQUI = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, AQUI)
import anil_pbs                          # noqa: E402

# dex_data.py es lo que ESTE script escribe, asi que no puede ser un
# requisito para arrancar: si sale roto (paso con el apostrofo de
# Sirfetch'd), el unico que puede arreglarlo seria el unico que no
# arranca. Lo mismo que ya paso con comprueba_orden y len(DEX).
try:
    from dex_data import TYPE_ACCENTS    # noqa: E402
except Exception as e:                   # noqa: BLE001
    print(f"AVISO: dex_data.py no se puede leer ({e.__class__.__name__}); "
          f"se regenera igual, pero sin comprobar colores ni orden.")
    TYPE_ACCENTS = None

DST = os.path.join(AQUI, "dex_data.py")

# El PBS escribe los tipos en ingles; TYPE_ACCENTS (solo color de UI) va en
# castellano. VOLADOR no esta: ninguna especie es volador puro, asi que el
# color sale siempre del otro tipo, y por eso se mira la lista entera.
COLOR = {
    "NORMAL": "normal", "FIRE": "fuego", "WATER": "agua", "GRASS": "planta",
    "ELECTRIC": "electrico", "ICE": "hielo", "FIGHTING": "lucha",
    "POISON": "veneno", "GROUND": "tierra", "PSYCHIC": "psiquico",
    "BUG": "bicho", "ROCK": "roca", "GHOST": "fantasma", "DRAGON": "dragon",
    "DARK": "siniestro", "STEEL": "acero", "FAIRY": "hada",
}

# A que nivel suben los que el Anil hace evolucionar por AMISTAD, mientras
# evolve() no sepa del vinculo.
#
# Un numero fijo no vale. Con 15 para todos (que es lo que llevaban los
# bebes de Johto) un Golbat se convertia en Crobat nada mas salir de la
# hierba, porque Zubat ya evoluciona al 22. El nivel sale de LO FUERTE QUE
# ES EL DESTINO, que es lo mismo que separa a un bebe de una forma final:
#   Pichu->Pikachu (320)   15        Togepi->Togetic (405)    25
#   Azurill->Marill (250)  15        Golbat->Crobat (535)     35
# Ademas asi la cadena sube siempre (Happiny 25 -> Chansey 35), que es lo
# que pide el test de niveles crecientes.
def amistad_lvl(bst):
    return 15 if bst < 350 else (25 if bst < 500 else 35)

# Nombres que no se pueden copiar tal cual del PBS. La fuente es CP437 y
# el nombre se pinta en MAYUSCULAS: los simbolos de genero de los Nidoran
# no estan, y saldria un hueco o un garabato.
A_PANTALLA = {
    "NIDORAN♀": "NIDORAN H", "NIDORAN♂": "NIDORAN M",
    "FARFETCH'D": "FARFETCHD", "SIRFETCH'D": "SIRFETCHD",
    "MIME JR.": "MIME JR",
    # La fuente tiene las minusculas acentuadas de CP437 pero NO las
    # mayusculas salvo la E, y el nombre se pinta en mayusculas.
    "FLABÉBÉ": "FLABEBE",
    # Lo que el Anil dejo en CASTELLANO: Codigo Cero y los veinte Pokemon
    # paradoja de Paldea. No se buscaron de uno en uno — los canta el
    # propio generador comparando el `Name` del PBS con el nombre de la
    # seccion, que si es el ingles (GREATTUSK, IRONHANDS).
    "CÓDIGO CERO": "TYPE NULL",
    "COLMILARGO": "GREAT TUSK",      "COLAGRITO": "SCREAM TAIL",
    "FURIOSETA": "BRUTE BONNET",     "MELENALETEO": "FLUTTER MANE",
    "REPTALADA": "SLITHER WING",     "PELARENA": "SANDY SHOCKS",
    "FERRODADA": "IRON TREADS",      "FERROSACO": "IRON BUNDLE",
    "FERROPALMAS": "IRON HANDS",     "FERROCUELLO": "IRON JUGULIS",
    "FERROPOLILLA": "IRON MOTH",     "FERROPÚAS": "IRON THORNS",
    "BRAMALUNA": "ROARING MOON",     "FERROPALADÍN": "IRON VALIANT",
    "ONDULAGUA": "WALKING WAKE",     "FERROVERDOR": "IRON LEAVES",
    "FLAMARIETE": "GOUGING FIRE",    "ELECTROFURIA": "RAGING BOLT",
    "FERROMOLE": "IRON BOULDER",     "FERROTESTA": "IRON CROWN",
}

# El detector compara el nombre YA pasado por A_PANTALLA con el interno,
# asi que las traducciones de arriba dejan de cantar solas. Los dos
# Nidoran son la excepcion de verdad: su nombre de pantalla es NIDORAN H /
# NIDORAN M (la fuente no tiene los simbolos de genero) y nunca va a
# parecerse a NIDORANfE / NIDORANmA.
_VISTOS_DISTINTOS = {29, 32}

# El slug es el identificador interno y lo usan los empaquetadores de la
# tarjeta. Los dos Nidoran se llamaban asi desde el principio y cambiarlos
# por el nombre del PBS (nidoranfe/nidoranma) no arregla nada y rompe lo
# que ya este empaquetado.
SLUG_A_MANO = {29: "nidoranf", 32: "nidoranm"}

# Evoluciones donde el PBS lista PRIMERO una que aqui no queremos. El campo
# guarda UN destino, asi que de las ramas hay que elegir, y estas dos se
# eligieron mirando:
#   133 Eevee     -> Vaporeon, no Espeon. Las siete ramas las lleva el
#                   codigo en EEVEE_BRANCH; el campo solo decide cual
#                   ensena la ficha, y Vaporeon es la primera de siempre.
#   236 Tyrogue   -> Hitmontop, no Hitmonlee. Hitmonlee y Hitmonchan ya
#                   existen por su cuenta desde Kanto; Hitmontop es el que
#                   SOLO sale de aqui.
EVO_A_MANO = {133: 134, 236: 237}


clave = anil_pbs.clave


def fila(n, slug, c, desde, hasta):
    nombre = c.get("Name", slug).upper()
    nombre = A_PANTALLA.get(nombre, nombre)
    # Se comprueba AQUI y no tres pasos mas abajo: la fuente es CP437 y la
    # caja de la ficha da para 12 caracteres. Flabebe entro con tildes y
    # solo lo canto el barrido, cuando ya estaba todo generado.
    # El `Name` del PBS NO siempre esta en ingles: el Anil dejo en
    # castellano a Codigo Cero y a los 24 Pokemon paradoja de Paldea. Dos
    # de ellos llevan tilde y los cazo el filtro de ASCII, pero
    # "Colmilargo" o "Ferropalmas" son ASCII perfectos y se habrian
    # colado. El nombre de la SECCION si es el ingles sin espacios
    # (GREATTUSK, IRONHANDS), asi que comparando los dos se ve solo.
    if clave(nombre) != clave(slug) and n not in _VISTOS_DISTINTOS:
        raise SystemExit(
            f"#{n} {slug}: el PBS lo llama {nombre!r}, que no es su nombre "
            f"interno; si es una traduccion, pon el ingles en A_PANTALLA")
    if not nombre.isascii():
        raise SystemExit(f"#{n} {slug}: el nombre {nombre!r} no es ASCII; "
                         f"anadelo a A_PANTALLA en gen_filas.py")
    if len(nombre) > 12:
        raise SystemExit(f"#{n} {slug}: {nombre!r} son {len(nombre)} chars y "
                         f"la ficha da para 12; acortalo en A_PANTALLA")

    tipos = [t.strip() for t in c["Types"].split(",")]
    color = next((COLOR[t] for t in tipos if t in COLOR), "normal")
    assert TYPE_ACCENTS is None or color in TYPE_ACCENTS, \
        f"{slug}: color {color} sin entrada en TYPE_ACCENTS"

    evo, lvl, otraVia, amistad = 0, 0, False, 0
    campos = [x.strip() for x in c.get("Evolutions", "").split(",")]
    for i in range(0, max(0, len(campos) - 2), 3):
        destino, metodo, arg = campos[i], campos[i + 1], campos[i + 2]
        if not destino:
            continue
        num = NUM.get(clave(destino))
        if num is None or num > hasta:
            continue          # de una generacion que todavia no tenemos
        if evo == 0:
            evo = num
        # Un nivel es un nivel aunque el metodo se llame de otra forma: el
        # Anil parte a Wurmple y a Nincada en dos con metodos a medida
        # ("SILCOON,Silcoon,7"), que es una RAMA al mismo nivel, no otra
        # cosa. Mirar solo por el nombre "Level" los dejaba sin nivel y
        # con destino, que es una evolucion que no ocurre nunca.
        if arg.isdigit():
            evo, lvl = num, int(arg)
            break             # el camino por nivel manda sobre los demas
        if metodo.startswith("Happiness"):
            if amistad == 0:
                amistad = num
        else:
            otraVia = True    # objeto o intercambio: eso ya tiene camino

    # La amistad solo se convierte en nivel si NO hay otra via. Si no,
    # Eevee (nueve objetos y dos amistades) acababa apuntando a Espeon en
    # vez de a Vaporeon, porque la amistad se leia la ultima.
    if lvl == 0 and amistad and not otraVia:
        evo = amistad
        lvl = amistad_lvl(sum(anil_pbs.stats(FICHA[amistad])))

    if n in EVO_A_MANO:
        evo = EVO_A_MANO[n]
    return (n, SLUG_A_MANO.get(n, slug.lower()), nombre, color, evo, lvl)


def main():
    if len(sys.argv) < 3:
        raise SystemExit(__doc__)
    desde, hasta = int(sys.argv[1]), int(sys.argv[2])
    escribir = "-w" in sys.argv

    # Si dex_data no se pudo leer no hay contra que comparar el orden; se
    # sigue, porque lo que viene ahora es justo reescribirlo.
    b = anil_pbs.comprueba_orden() if TYPE_ACCENTS else anil_pbs.bloques()
    global NUM, FICHA
    NUM = anil_pbs.numeros()
    # La ficha de CUALQUIER numero, para poder mirar la BST del destino de
    # una evolucion por amistad sin depender de dex_stats.py (que se genera
    # DESPUES que esto cuando entra una region nueva).
    FICHA = {i + 1: c for i, (_s, c) in enumerate(b)}

    filas = [fila(n, b[n - 1][0], b[n - 1][1], desde, hasta)
             for n in range(desde, hasta + 1)]

    texto = "".join(
        f"    ({n}, '{slug}', '{nombre}', '{color}', {evo}, {lvl}),\n"
        for n, slug, nombre, color, evo, lvl in filas)

    if not escribir:
        print(texto, end="")
        sin_evo = [f[2] for f in filas if f[4] == 0]
        print(f"\n{len(filas)} filas, {len(filas) - len(sin_evo)} con evolucion")
        return 0

    # Se RECONSTRUYE la lista entera: se tiran las filas cuyo numero cae en
    # el tramo y se meten las nuevas en su sitio. Antes esto buscaba una
    # marca de comentario y, si no estaba, insertaba al final — asi que al
    # regenerar 1-493 se quedaron las viejas DEBAJO y la lista acabo con
    # 986 filas y un segundo Bulbasaur en el hueco 494. El fallo lo canto
    # anil_pbs.comprueba_orden(), que para eso esta.
    src = io.open(DST, encoding="utf-8").read()
    ini = src.index("\nDEX = [") + len("\nDEX = [\n")
    fin = src.index("\n]\n", ini) + 1

    queda = []
    for ln in src[ini:fin].splitlines(True):
        m = re.match(r"\s*\((\d+),", ln)
        if m and desde <= int(m.group(1)) <= hasta:
            continue                      # la reemplaza una nueva
        if "generado por gen_filas.py" in ln:
            continue                      # marca vieja de otro tramo
        queda.append(ln)

    # repr() y no comillas a mano: Sirfetch'd lleva apostrofo y rompia el
    # dex_data.py generado con un SyntaxError. Que lo escape Python, que
    # para eso sabe.
    nuevas = {n: f"    ({n}, {s!r}, {nom!r}, {col!r}, {e}, {l}),\n"
              for n, s, nom, col, e, l in filas}
    salida, puestas = [], False
    for ln in queda:
        m = re.match(r"\s*\((\d+),", ln)
        if not puestas and m and int(m.group(1)) > hasta:
            salida.extend(nuevas[n] for n in sorted(nuevas))
            puestas = True
        salida.append(ln)
    if not puestas:
        salida.extend(nuevas[n] for n in sorted(nuevas))

    src = src[:ini] + "".join(salida) + src[fin:]
    io.open(DST, "w", encoding="utf-8", newline="\n").write(src)
    print(f"{len(filas)} filas ({desde}-{hasta}) metidas en "
          f"{os.path.normpath(DST)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
