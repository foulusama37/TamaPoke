#!/usr/bin/env python3
"""Baja los retratos de lider que el Anil NO tiene, de Bulbagarden Archives.

El Anil solo trae a los de Kanto (LIDER1..8, ALTOMANDO1..4, CAMPEON) mas los
de Hoenn y las revanchas. Para Johto no hay nada, ni en Unbreakable Ties ni en
Reloaded: se comprobo mirando las tres carpetas de Trainers.

La fuente la dio el usuario:
  https://archives.bulbagarden.net/wiki/Category:Trainer_sprites

Se cogen los de HGSS a proposito: son pixel art de la misma epoca y el mismo
tamano que los del Anil (80..100 px de ancho, 17..32 colores), asi que en la
pantalla no se nota de donde salio cada uno. Los de gen 5 en adelante son mas
grandes y con otra paleta, y se verian pegados.

Se guardan en tools/leaders_johto/ y de ahi los lee gen_art.py. El fichero
descargado NO se toca a mano: si hay que cambiar uno, se cambia aqui y se
vuelve a bajar.

  python3 tools/fetch_leaders.py           # los que falten
  python3 tools/fetch_leaders.py --force   # todos otra vez
"""
import json
import os
import subprocess
import sys
import time
import urllib.parse

AQUI = os.path.dirname(os.path.abspath(__file__))
API = "https://archives.bulbagarden.net/w/api.php"
UA = "TamaPoke-asset-fetch/1.0 (hobby handheld; contact via github.com/socquique/TamaPoke)"

# Que falta por region, y con que numero de hueco entra. El ORDEN y los
# numeros son los de gyms.h: el indice del fichero ES el indice del lider,
# y por eso Hoenn empieza en el 8 (los ocho primeros los pone el Anil con
# sus LIDER1HOENN..LIDER8HOENN, y ahi no hay nada que bajar).
#
# Los de HGSS se cogen a proposito: pixel art de la misma epoca y el mismo
# tamano que los del Anil una vez reducidos (80 px).
#
# De Hoenn solo hay los de RUBI/ZAFIRO, que son de GBA y vienen a 64. No
# hay version HGSS ni ORAS de esos cinco en Archives (se busco por la API
# antes de conformarse). Se quedan un 20% mas bajos que los ocho lideres
# que pone el Anil; prefiero eso a reescalar 64->80, que no es un multiplo
# y deja el pixel art emborronado.
JUEGOS = {"johto": "HGSS", "hoenn": "RS", "sinnoh": "DP", "unova": "BW"}

# Como se llama el fichero en Archives. Hasta la gen 5 todos siguen el
# mismo molde, "Spr <JUEGO> <Nombre>.png", que es el sprite de combate.
#
# De la gen 6 en adelante ESO NO EXISTE: XY pasa a modelos 3D y de los
# lideres solo quedan los retratos VS, que son dibujo plano de MEDIO
# CUERPO a 512x256. Se probaron al lado de los de Johto (el apano esta en
# _claude_scratch/kalos/) y no pegan: puesta a 80 px de alto, la cara de
# Viola sale tres veces mas grande que un entrenador ENTERO de HGSS, y
# parecen de dos juegos distintos.
#
# Los de OVERWORLD si pegan: cuerpo entero, mismo encuadre y proporciones
# parecidas. Son lo que se usa de Kalos en adelante.
PATRON = {r: "Spr %s {n}" % j for r, j in JUEGOS.items()}
PATRON["kalos"] = "{n} XY OD"
# Galar no tiene NI overworld: Espada/Escudo es 3D del todo. Quedaban el
# retrato VS (medio cuerpo, el mismo problema que en Kalos) y el muneco de
# Masters, que es cuerpo entero. Procesados a 80 px y puestos al lado de
# los del Anil, el de Masters se lee y el VS no (_claude_scratch/galar/).
PATRON["galar"] = "Spr Masters {n}"
PATRON["paldea"] = "Spr Masters {n}"   # misma razon: gen 9 tampoco tiene 2D
FALTAN = {
    "johto": [(i, n) for i, n in enumerate([
        "Falkner", "Bugsy", "Whitney", "Morty", "Chuck", "Jasmine", "Pryce",
        "Clair", "Will", "Koga", "Bruno", "Karen", "Lance"])],
    # Del Alto Mando de Hoenn el Anil no trae ninguno.
    "hoenn": list(enumerate(
        ["Sidney", "Phoebe", "Glacia", "Drake", "Steven"], start=8)),
    # De Sinnoh no trae NADA (su Cintia es de su propia historia, no la
    # campeona de Sinnoh), asi que los trece.
    "sinnoh": list(enumerate([
        "Roark", "Gardenia", "Maylene", "Crasher Wake", "Fantina", "Byron",
        "Candice", "Volkner", "Aaron", "Bertha", "Flint", "Lucian",
        "Cynthia"])),
    # Teselia. Los de BW son mas grandes que los de HGSS/DP (son de pantalla
    # de DS a otra escala); se comprueba el tamano al generar el arte.
    "unova": list(enumerate([
        "Cilan", "Lenora", "Burgh", "Elesa", "Clay", "Skyla", "Brycen",
        "Drayden", "Shauntal", "Grimsley", "Caitlin", "Marshal", "Alder"])),
    "kalos": list(enumerate([
        "Viola", "Grant", "Korrina", "Ramos", "Clemont", "Valerie",
        "Olympia", "Wulfric", "Malva", "Siebold", "Wikstrom", "Drasna",
        "Diantha"])),
    # Galar tiene DIEZ lideres contando los dos que cambian de version
    # (Bea/Allister y Gordie/Melony), asi que entran los diez y los tres
    # ultimos huecos son la Copa de Campeones: Marnie, Hop y Lionel.
    # Ofelia no esta en Masters (se busco), y la unica imagen suya es el
    # retrato VS de medio cuerpo, que ya se descarto en Kalos. En su hueco
    # va BEDE, que es justamente quien le quita el gimnasio de hada en la
    # historia postliga: no es un relleno, es su sucesor.
    "galar": list(enumerate([
        "Milo", "Nessa", "Kabu", "Bea", "Allister", "Bede", "Gordie",
        "Melony", "Piers", "Raihan", "Marnie", "Hop", "Leon"])),
    # Paldea. Cuatro lideres (Katy, Kofu, Ryme y Tulip) NO estan en
    # Masters, y de ellos solo hay retrato VS de medio cuerpo, que es lo
    # que ya se descarto dos veces. En sus huecos van personajes de la
    # MISMA region que si estan y que tambien se pelean en los juegos:
    # Arven, Clavell (el director), Penny y los dos de la Academia
    # Arandano. Larry sale una vez, no dos como en el juego.
    "paldea": list(enumerate([
        "Arven", "Brassius", "Iono", "Clavell", "Larry", "Penny",
        "Carmine", "Grusha", "Rika", "Poppy", "Drayton", "Hassel",
        "Geeta"])),
}


# Se baja con curl y no con urllib por la misma razon que gen_names.py: el
# almacen de certificados que trae Python en esta maquina esta caducado y
# urllib se cae con CERTIFICATE_VERIFY_FAILED, mientras que curl usa el de
# Windows y pasa. Desactivar la verificacion no es la respuesta.
def baja(url, binario=False):
    r = subprocess.run(
        ["curl", "-sL", "--max-time", "60", "-A", UA, url],
        capture_output=True,
        **({} if binario else {"text": True, "encoding": "utf-8", "errors": "replace"}))
    if r.returncode != 0:
        raise SystemExit(f"curl fallo ({r.returncode}) en {url}")
    return r.stdout


def pide(params, intentos=4):
    """Consulta a la API. Reintenta: de vez en cuando devuelve HTML.

    Con una region son trece llamadas y no se nota; con ocho son mas de
    cien y Archives contesta alguna vez con una pagina de error o un
    corte. Sin reintento, la descarga entera se cae por una.
    """
    params = dict(params, format="json")
    url = API + "?" + urllib.parse.urlencode(params)
    for n in range(intentos):
        txt = baja(url)
        try:
            return json.loads(txt)
        except json.JSONDecodeError:
            if n == intentos - 1:
                raise SystemExit(
                    f"Archives no devuelve JSON despues de {intentos} "
                    f"intentos:\n  {url}\n  {txt[:200]!r}")
            time.sleep(2 * (n + 1))
    return {}


def url_de(titulo):
    """URL real del fichero, o None. MediaWiki la da en imageinfo."""
    d = pide({"action": "query", "titles": titulo,
              "prop": "imageinfo", "iiprop": "url"})
    for pag in d.get("query", {}).get("pages", {}).values():
        info = pag.get("imageinfo")
        if info:
            return info[0]["url"]
    return None


def main():
    forzar = "--force" in sys.argv
    solo = next((a for a in sys.argv[1:] if a in FALTAN), None)
    faltan, puestos = [], 0
    for region, lista in FALTAN.items():
        if solo and region != solo:
            continue
        out = os.path.join(AQUI, f"leaders_{region}")
        os.makedirs(out, exist_ok=True)
        patron = PATRON[region]
        print(f"\n{region.upper()} ({patron.format(n='...')}) -> {out}")
        for i, nombre in lista:
            destino = os.path.join(out, f"{i:02d}_{nombre.upper()}.png")
            if os.path.exists(destino) and not forzar:
                print(f"  {i:2d} {nombre:<9} ya esta")
                puestos += 1
                continue
            url = url_de("File:" + patron.format(n=nombre) + ".png")
            if not url:
                print(f"  {i:2d} {nombre:<9} !! no esta en Archives")
                faltan.append(f"{region}/{nombre}")
                continue
            datos = baja(url, binario=True)
            if not datos.startswith(b"\x89PNG"):
                print(f"  {i:2d} {nombre:<9} !! lo que llego no es un PNG")
                faltan.append(f"{region}/{nombre}")
                continue
            with open(destino, "wb") as f:
                f.write(datos)
            puestos += 1
            print(f"  {i:2d} {nombre:<9} {len(datos) // 1024} KB  "
                  f"{os.path.basename(destino)}")
            time.sleep(0.3)   # no se machaca el servidor de nadie
    if faltan:
        print(f"\nFALTAN: {', '.join(faltan)}")
        return 1
    print(f"\n{puestos} retratos")
    return 0


if __name__ == "__main__":
    sys.exit(main())
