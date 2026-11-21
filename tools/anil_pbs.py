# -*- coding: utf-8 -*-
"""Lectura del PBS del Anil. UN solo sitio.

El mismo parseador de secciones estaba copiado en gen_evos.py, en
gen_filas.py y en dos scripts del scratch, cada copia con sus manias. Y la
comprobacion de que el fichero va en orden de dex NACIONAL estaba en uno
solo, que es justo la que no puede faltar: de ella depende que el numero
251 sea Celebi y no otro.

El PBS es la fuente de la Pokedex entera desde 2026-10-03 (decision del
usuario): numeros, nombres, tipos, stats base, alturas y evoluciones. Lo
unico que sigue pidiendo red son los nombres traducidos (gen_names.py).

OJO con dos cosas del formato de Essentials:

  BaseStats va HP,Atk,Def,VELOCIDAD,SpA,SpD        <- la velocidad va 4a
  los dos Nidoran se llaman NIDORANfE / NIDORANmA
"""
import io
import os
import re
import sys

AQUI = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, AQUI)
from dex_range import N_DEX      # noqa: E402

ANIL = r"C:\Users\foulu\Downloads\Pokemon Anil V4.13"
PBS = os.path.join(ANIL, "PBS", "pokemon.txt")
FORMS = os.path.join(ANIL, "PBS", "pokemon_forms.txt")

# El PBS escribe los tipos en mayusculas y con el nombre largo; dex_types.py
# los guarda como PokeAPI, que es de donde salieron.
T_PBS = {"FIGHTING": "fighting", "ELECTRIC": "electric"}

_cache = {}


def clave(s):
    """Normaliza un nombre para comparar: solo A-Z y digitos."""
    return re.sub(r"[^A-Z0-9]", "", s.upper())


def bloques(path=None):
    """[(cabecera, {campo: valor})] en el orden del fichero."""
    path = path or PBS
    if path in _cache:
        return _cache[path]
    out, cab, campos = [], None, {}
    for ln in io.open(path, encoding="utf-8", errors="replace"):
        m = re.match(r"^\[(.+?)\]\s*$", ln.strip())
        if m:
            if cab is not None:
                out.append((cab, campos))
            cab, campos = m.group(1).strip(), {}
            continue
        if cab is not None and "=" in ln:
            k, v = ln.split("=", 1)
            campos[k.strip()] = v.strip()
    if cab is not None:
        out.append((cab, campos))
    _cache[path] = out
    return out


def comprueba_orden(b=None):
    """Que el PBS vaya en dex NACIONAL. Se comprueba, no se supone.

    Si algun dia el Anil reordena su pokemon.txt, TODO lo que sale de aqui
    (stats, tipos, alturas, evoluciones) se desplazaria un hueco y nadie se
    enteraria hasta ver a un Charizard de tipo agua.
    """
    from dex_data import DEX
    b = b or bloques()
    if len(b) < N_DEX:
        raise SystemExit(f"el PBS tiene {len(b)} especies y la dex pide {N_DEX}")
    # Solo las N_DEX primeras, no `len(DEX)`: que la lista tenga el largo
    # que toca lo comprueba gen_dex.py con su propio assert. Mirar la lista
    # entera aqui hacia que un dex_data.py roto bloqueara al unico que
    # puede arreglarlo (gen_filas), que es lo que paso al regenerar 1-493.
    for i, d in enumerate(DEX[:N_DEX]):
        suyo, mio = clave(b[i][0]), clave(d[1])
        if suyo in ("NIDORANFE", "NIDORANMA"):
            suyo = suyo[:-1]
        if suyo != mio:
            raise SystemExit(
                f"el PBS NO va en orden nacional: en el hueco {i + 1} nosotros "
                f"tenemos {mio} y el {suyo}. No se genera nada.")
    return b


def numeros():
    """{CLAVE: numero de dex} de TODO el PBS, no solo de nuestro tramo."""
    return {clave(s): i + 1 for i, (s, _) in enumerate(bloques())}


def stats(c):
    """(hp, atk, def, spa, spd, vel) desde el BaseStats de una seccion.

    El PBS los escribe HP,Atk,Def,VEL,SpA,SpD (orden de Essentials). Leerlos
    de corrido deja la velocidad en el hueco del ataque especial, que es un
    fallo que no se ve: los numeros siguen siendo plausibles.
    """
    hp, atk, df, vel, spa, spd = [int(x) for x in c["BaseStats"].split(",")]
    return (hp, atk, df, spa, spd, vel)


def tipos(c):
    """(tipo1, tipo2 o None) en minusculas, como PokeAPI."""
    t = [T_PBS.get(x.strip(), x.strip().lower()) for x in c["Types"].split(",")]
    return (t[0], t[1] if len(t) > 1 else None)


# Como llama Essentials a cada reparto de sexo, en % de HEMBRAS.
# 255 = sin sexo, que no es lo mismo que 0 (0 es "siempre macho").
GENERO = {
    "AlwaysMale": 0, "FemaleOneEighth": 12, "Female25Percent": 25,
    "Female50Percent": 50, "Female75Percent": 75,
    "FemaleSevenEighths": 87, "AlwaysFemale": 100, "Genderless": 255,
}


def fem_pct(c):
    """% de probabilidad de salir HEMBRA. 255 = sin sexo."""
    g = c.get("GenderRatio", "Female50Percent").strip()
    if g not in GENERO:
        raise SystemExit(f"GenderRatio que no conozco: {g!r}")
    return GENERO[g]


def altura_dm(c):
    """Altura en decimetros (el PBS la da en metros)."""
    return int(round(float(c["Height"]) * 10))


# Que formas del Anil entran en el juego. Vive aqui, y no en gen_forms,
# porque gen_abilities necesita el MISMO filtro para meter sus habilidades
# en el catalogo — y si gen_abilities importara gen_forms habria un ciclo
# (gen_forms ya importa gen_abilities para saber los indices).
#
# Entran las REGIONALES y las cuatro que tienen mega propia. El por que
# de dejar fuera las demas esta en gen_forms.py.
FORMAS_REGION = {"Alola", "Galar", "Hisui", "Paldea"}


def es_regional(fn):
    """¿Es una forma REGIONAL, o solo lleva el nombre de la region?

    Tiene que EMPEZAR por "Forma <Region>". Buscar la region en
    cualquier parte del nombre metia los gorros de Pikachu: su forma 13
    se llama "Forma Gorra Alola" y entraba como si fuera un Pikachu de
    Alola, que no existe. Lo canto la hoja de contacto, no la auditoria
    — un sprite de Pikachu con gorra se lee perfectamente, solo que no
    es lo que dice ser.
    """
    return any(fn.startswith("Forma " + r) for r in FORMAS_REGION)
FORMAS_CON_MEGA = {("TOXTRICITY", 1), ("TATSUGIRI", 1), ("TATSUGIRI", 2),
                   ("MAGEARNA", 1)}

# La letra con que se distingue cada una de esas formas en pantalla, por
# (numero de dex, numero de forma). El Anil las dejo en castellano y sin
# decir de quien son ("Forma Grave" a secas), asi que el rotulo hay que
# ponerlo aqui.
#
# Vive en este modulo y no en gen_forms porque gen_megas necesita LA
# MISMA letra: la mega de la Toxtricity Grave y la de la normal piden la
# misma piedra y tienen los mismos stats, de modo que sin la letra las
# dos se llamaban "M.TOXTRICITY" y en la ficha eran la misma. Teniendo
# cada generador su tabla, arreglar una dejaba la otra como estaba — que
# es exactamente lo que paso.
LETRA_FORMA = {
    (849, 1): "L",   # Toxtricity Grave     / Low Key
    (978, 1): "D",   # Tatsugiri Languida   / Droopy
    (978, 2): "S",   # Tatsugiri Recta      / Stretchy
    (801, 1): "O",   # Magearna Color Vetusta / Original Color
}


def formas_incluidas():
    """[(cabecera, campos)] de las formas (NO megas) que si se modelan."""
    out = []
    for cab, c in bloques(FORMS):
        fn = c.get("FormName", "")
        if "Mega" in fn or "Primal" in fn:
            continue
        partes = cab.split(",")
        forma = int(partes[1]) if len(partes) > 1 and partes[1].isdigit() else 0
        if es_regional(fn) or (clave(partes[0]), forma) in {
                (clave(a), b) for a, b in FORMAS_CON_MEGA}:
            out.append((cab, c))
    return out


def filas(hasta=None):
    """[(numero, slug, campos)] de 1 a `hasta` (por omision, N_DEX)."""
    b = comprueba_orden()
    hasta = hasta or N_DEX
    return [(i + 1, b[i][0], b[i][1]) for i in range(hasta)]
