# -*- coding: utf-8 -*-
"""As fichas de Pokedex oficiais em FR/DE/IT/JA/KO (leva 79).

O Anil e espanhol (o ingles dele e de fa), e as fichas espanholas dele sao
as oficiais de algum jogo. Para cada especie escolhe-se a VERSAO da PokeAPI
que tem os 5 idiomas e cuja ficha espanhola mais se parece com a do Anil,
e tira-se dessa versao a ficha de cada idioma: o mesmo jogo, a mesma ficha.
(Em ~half das especies bate >= 85%; nas outras o Anil usa um jogo que a
PokeAPI nao tem, e fica a versao mais parecida.)

A Pokedex e lore, nao mecanica: aqui o texto oficial vale mais do que uma
traducao do espanhol do Anil.

Entrada: tools/downloads/pokeapi_csv/pokemon_species_flavor_text.csv
         (gitignored; baixa-se de github.com/PokeAPI/pokeapi data/v2/csv)
Saida:   tools/textos_dex/dex_oficial.json  (vai no git: o build nao
         precisa da PokeAPI)

Troca as letras que a fonte da placa nao tem (ver TROCAS), e o resto e
conferido por gen_textos_golpes_dex.py.
"""
import csv
import json
import os
import re
import sys
import unicodedata
from difflib import SequenceMatcher

AQUI = os.path.dirname(os.path.abspath(__file__))
RAIZ = os.path.dirname(AQUI)
sys.path.insert(0, AQUI)
import texto_pt  # noqa: E402

CSV = os.path.join(AQUI, "downloads", "pokeapi_csv", "pokemon_species_flavor_text.csv")
DST = os.path.join(AQUI, "textos_dex", "dex_oficial.json")
LG = {"FR": 5, "DE": 6, "IT": 8, "JA": 1, "KO": 3}   # JA = ja-hrkt (kana), como os nomes
ES = 7
LIT = re.compile(r'"((?:[^"\\]|\\.)*)"')

# o que a fonte da placa nao tem -> o que os jogos escrevem sem ela.
# ♀/♂ vao para os codigos 12/11 da fonte classica (CP437), como no nome do Nidoran.
TROCAS = {
    "FR": {"œ": "oe", "Œ": "OE", "À": "A", "È": "E"},
    "DE": {"„": '"', "℃": "°C"},
    "IT": {"È": "E'", "À": "A'"},
    "JA": {"体躯": "からだ"},   # 体躯 -> からだ (躯 nao esta na japanese3)
    "KO": {"“": '"', "”": '"'},
}
LATINO_SEXO = {"♀": "\x0c", "♂": "\x0b"}


def plano(s):
    s = unicodedata.normalize("NFKD", s.replace("­", ""))
    s = "".join(c for c in s if not unicodedata.combining(c))
    return re.sub(r"[^a-z0-9]+", " ", s.lower()).strip()


def limpa(s, lg):
    """o texto da PokeAPI vem com as quebras de linha da caixa do jogo"""
    s = s.replace("­\n", "").replace("­", "")
    s = s.replace("\x0c", " ").replace("\n", " ").replace("\r", " ").replace("　", " ")
    s = re.sub(r"\s+", " ", s).strip()
    for a, b in TROCAS[lg].items():
        s = s.replace(a, b)
    return s


def fichas_es_do_anil():
    h = open(os.path.join(RAIZ, "dex_text.h"), encoding="utf-8").read()
    i = h.index("static const char *const DEX_TEXT_ES[")
    return [texto_pt.descodifica(x) for x in LIT.findall(h[h.index("{", i):h.index("\n};", i)])]


def main():
    es = fichas_es_do_anil()
    ft = {}
    for r in csv.DictReader(open(CSV, encoding="utf-8")):
        ft.setdefault(int(r["species_id"]), {}).setdefault(int(r["version_id"]), {})[int(r["language_id"])] = r["flavor_text"]
    out = {lg: [None] for lg in LG}
    versao = [None]
    for sp in range(1, len(es)):
        vers = ft.get(sp, {})
        alvo = plano(es[sp])
        best, bv = -1.0, None
        for v in sorted(vers):
            t = vers[v]
            if not all(k in t for k in LG.values()):
                continue
            r = SequenceMatcher(None, alvo, plano(t[ES])).ratio() if ES in t else 0.0
            if r >= best:   # empate: a versao mais nova
                best, bv = r, v
        if bv is None:
            sys.exit("especie %d sem versao com os 5 idiomas" % sp)
        versao.append([bv, round(best, 2)])
        for lg, k in LG.items():
            out[lg].append(limpa(vers[bv][k], lg))
    out["_versao"] = versao
    os.makedirs(os.path.dirname(DST), exist_ok=True)
    with open(DST, "w", encoding="utf-8", newline="\n") as f:
        json.dump(out, f, ensure_ascii=False, indent=0)
    v = versao[1:]
    print("%d fichas; ficha espanhola igual (>= 85%%) em %d" % (len(v), sum(1 for x in v if x[1] >= .85)))


if __name__ == "__main__":
    main()
