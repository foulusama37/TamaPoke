"""Baixa as FITAS (ribbons) do PokeSprite para tools/fitas/<n>.png, uma
por conquista, na ordem de MED_* do pet.h (e depois uma por regiao, na
ordem das regioes).

    python -I tools/baixa_fitas.py

A escolha de qual fita e de qual conquista esta aqui, com o motivo.
"""
import os
import sys
import urllib.request

AQUI = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(AQUI, "fitas")
URL = "https://raw.githubusercontent.com/msikma/pokesprite/master/misc/ribbon/%s.png"
UA = {"User-Agent": "Mozilla/5.0"}

# indice = bit de MED_* ; depois MED_REGION0 + regiao
FITAS = [
    "training-ribbon",            # 0  LV10
    "effort-ribbon",              # 1  LV25
    "expert-battler-ribbon",      # 2  LV50
    "smile-ribbon",               # 3  BERRY: descobriu a baga favorita
    "alert-ribbon",               # 4  STREAK7
    "best-friends-ribbon",        # 5  BOND
    "footprint-ribbon",           # 6  FINAL: forma final
    "relax-ribbon",               # 7  FIT
    "legend-ribbon",              # 8  LV100
    "record-ribbon",              # 9  STREAK30
    "land-ribbon",                # 10 DEX50
    "sky-ribbon",                 # 11 DEX150
    "earth-ribbon",               # 12 DEX500
    "classic-ribbon",             # 13 CAUGHT100
    "winning-ribbon",             # 14 WINS100
    "victory-ribbon",             # 15 WINS500
    "gorgeous-ribbon",            # 16 SHINY
    "shock-ribbon",               # 17 MEGA1
    "royal-ribbon",               # 18 MEGA10
    "battle-memory-ribbon-gold",  # 19 REMATCH
    "festival-ribbon",            # 20 TEAM6
    # as regioes: a fita de campeao de cada uma; Johto, Unova e Paldea
    # nao tem a sua no PokeSprite e levam uma de campeao generica
    "champion-ribbon",            # Kanto
    "regional-champion-ribbon",   # Johto
    "hoenn-champion-ribbon",      # Hoenn
    "sinnoh-champion-ribbon",     # Sinnoh
    "battle-champion-ribbon",     # Unova
    "kalos-champion-ribbon",      # Kalos
    "alola-champion-ribbon",      # Alola
    "gen8/galar-champion-ribbon", # Galar
    "national-champion-ribbon",   # Paldea
]


def main():
    os.makedirs(OUT, exist_ok=True)
    falta = []
    for i, nome in enumerate(FITAS):
        try:
            with urllib.request.urlopen(urllib.request.Request(URL % nome, headers=UA),
                                        timeout=30) as r:
                d = r.read()
        except Exception as e:                           # noqa: BLE001
            falta.append((i, nome, str(e)))
            continue
        if not d.startswith(bytes([0x89]) + b"PNG"):
            falta.append((i, nome, "nao e PNG"))
            continue
        open(os.path.join(OUT, "%d.png" % i), "wb").write(d)
    print("faltam:", falta)


if __name__ == "__main__":
    sys.exit(main())
