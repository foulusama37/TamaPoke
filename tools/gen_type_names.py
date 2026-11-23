#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Nomes OFICIAIS dos 18 tipos por idioma -> tools/type_names.py (dados).

    python3 tools/gen_type_names.py            # usa o cache tools/downloads/pokeapi_types
    python3 tools/gen_type_names.py --baixa    # baixa o que faltar

Quem le e o gen_dex.py, que emite a tabela TYPE_NAME_L no dex.h e o
typeName() por idioma. Ingles fica com os rotulos CURTOS de sempre (FIGHT,
ELEC, PSY: o chip do golpe foi medido com eles); os outros levam o nome
oficial, em maiusculas com os acentos que a fonte TEM (É Ç Ä Ö Ü Ñ, e
Ã Õ pelos bytes do jogo); os outros caem (cp437_up.py). Portugues: a PokeAPI nao tem; vao
os nomes oficiais brasileiros, escritos aqui. JA em kana, KO como vem.
"""
import json
import os
import subprocess
import sys
import unicodedata

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from cp437_up import maiuscula

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CACHE = os.path.join(RAIZ, "tools", "downloads", "pokeapi_types")
# na ordem dos T_* (T_NONE fica de fora)
SLUGS = ["normal", "fighting", "flying", "poison", "ground", "rock", "bug", "ghost", "fire",
         "water", "grass", "electric", "psychic", "ice", "dragon", "dark", "steel", "fairy"]
EN_CURTO = ["NORMAL", "FIGHT", "FLY", "POISON", "GROUND", "ROCK", "BUG", "GHOST", "FIRE",
            "WATER", "GRASS", "ELEC", "PSY", "ICE", "DRAGON", "DARK", "STEEL", "FAIRY"]
PT = ["Normal", "Lutador", "Voador", "Venenoso", "Terrestre", "Pedra", "Inseto", "Fantasma",
      "Fogo", "Água", "Planta", "Elétrico", "Psíquico", "Gelo", "Dragão", "Sombrio", "Aço", "Fada"]
API = {"ES": "es", "FR": "fr", "DE": "de", "IT": "it", "JA": "ja-hrkt", "KO": "ko"}
LINGUAS = ["ES", "EN", "FR", "DE", "IT", "PT", "JA", "KO"]


def latino(s):
    return maiuscula(s)   # KÄFER, DRAGÃO: as letras que a fonte tem


def nomes(slug):
    p = os.path.join(CACHE, slug + ".json")
    if not os.path.exists(p):
        if "--baixa" not in sys.argv:
            raise SystemExit("falta o cache de %s: rode com --baixa" % slug)
        os.makedirs(CACHE, exist_ok=True)
        r = subprocess.run(["curl", "-s", "--max-time", "25", "-A", "Mozilla/5.0",
                            "https://pokeapi.co/api/v2/type/%s/" % slug],
                           capture_output=True, text=True, encoding="utf-8", errors="replace")
        json.dump({"names": json.loads(r.stdout)["names"]}, open(p, "w", encoding="utf-8"),
                  ensure_ascii=False)
    d = json.load(open(p, encoding="utf-8"))
    return {n["language"]["name"]: n["name"] for n in d["names"]}


def main():
    tab = {lg: ["---"] for lg in LINGUAS}
    for i, slug in enumerate(SLUGS):
        n = nomes(slug)
        for lg in LINGUAS:
            if lg == "EN":
                tab[lg].append(EN_CURTO[i])
            elif lg == "PT":
                tab[lg].append(latino(PT[i]))
            elif lg in ("JA", "KO"):
                tab[lg].append(unicodedata.normalize("NFKC", n[API[lg]]))
            else:
                tab[lg].append(latino(n[API[lg]]))
    p = os.path.join(RAIZ, "tools", "type_names.py")
    with open(p, "w", encoding="utf-8", newline="\n") as f:
        f.write("# -*- coding: utf-8 -*-\n")
        f.write('"""GERADO por tools/gen_type_names.py - nao editar a mao.\n\n')
        f.write("Nome do tipo por idioma, na ordem dos T_* (o 0 e T_NONE).\n")
        f.write('Ordem dos idiomas = a do enum Lang (ES EN FR DE IT PT JA KO)."""\n\n')
        f.write("TYPE_NAMES = {\n")
        for lg in LINGUAS:
            f.write("    %r: %r,\n" % (lg, tab[lg]))
        f.write("}\n")
    print("guardado", os.path.normpath(p))


if __name__ == "__main__":
    main()
