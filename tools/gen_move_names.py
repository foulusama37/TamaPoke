#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Nomes OFICIAIS dos golpes por idioma (PokeAPI) -> move_names.h.

    python3 tools/gen_move_names.py           # usa o cache em tools/downloads/pokeapi_moves
    python3 tools/gen_move_names.py --baixa   # baixa o que faltar no cache

MOVE_TBL[].name (moves.h) e o IDENTIFICADOR do golpe: carrega a animacao,
escolhe o som, o motor compara ("SURF", "THUNDERWAVE"). Nao se mexe nele. O
que vai para a TELA sai de moveName(), deste arquivo.

Como os Pokemon (gen_names.py): latinos em MAIUSCULAS e sem acento (a fonte
CP437 nao tem maiusculas acentuadas, e assim o golpe fica com a cara do nome
do Pokemon); japones em kana (ja-Hrkt, com os digitos largos virando ASCII:
a fonte tem os dois e o ASCII e mais estreito); coreano como vem. Portugues:
a PokeAPI nao tem; vai o ingles, que e o que os jogos usaram ate SV.

ASSINATURA: cada linha guarda tambem o id do golpe (MOVE_NAME_ID). Uma tabela
indexada pela linha de outra toca o VIZINHO calada quando a outra cresce; o
teste confere MOVE_NAME_ID[i] == MOVE_TBL[i].name para todas as linhas.
"""
import json
import os
import re
import subprocess
import sys
import time
import unicodedata

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CACHE = os.path.join(RAIZ, "tools", "downloads", "pokeapi_moves")
LINGUAS = ["ES", "EN", "FR", "DE", "IT", "PT", "JA", "KO"]
API = {"ES": "es", "EN": "en", "FR": "fr", "DE": "de", "IT": "it", "JA": "ja-hrkt", "KO": "ko"}
APELIDO = {"VISEGRIP": "vice-grip"}   # o id do Anil difere do slug da PokeAPI
# Golpes do Anil que nao existem nos jogos: nomes escritos aqui, a partir
# do PBS do Anil (Name/Description). ATRAPAMOSCAS se chama "Atrapabicho" la
# (mordida de planta boa contra Inseto); DESLIZAMIENTO e de GELO: "se desliza
# con un esqui y aumenta la Velocidad" -- nao e deslizamento de terra (a
# primeira versao disse Landslide/Frana e o italiano batia com o Rock Slide).
PROPRIOS = {
    "ATRAPAMOSCAS": {"ES": "ATRAPABICHO", "EN": "BUG TRAP", "FR": "PIEGE-INSECTE",
                     "DE": "KAEFERFALLE", "IT": "ACCHIAPPAINSETTI", "PT": "PEGA-INSETO",
                     "JA": "むしとり", "KO": "벌레잡이"},
    "DESLIZAMIENTO": {"ES": "DESLIZAMIENTO", "EN": "SKI SLIDE", "FR": "GLISSADE",
                      "DE": "SKIFAHRT", "IT": "SCIATA", "PT": "ESQUIADA",
                      "JA": "スキースライド", "KO": "스키활강"},
    "ESCALOFRIO": {"ES": "ESCALOFRIO", "EN": "SHIVER", "FR": "FRISSON",
                   "DE": "SCHAUDER", "IT": "BRIVIDO", "PT": "CALAFRIO",
                   "JA": "さむけ", "KO": "오한"},
    "TRIPLEHIT": {"ES": "TRIPLE GOLPE", "EN": "TRIPLE HIT", "FR": "TRIPLE COUP",
                  "DE": "DREIFACHHIEB", "IT": "TRIPLO COLPO", "PT": "GOLPE TRIPLO",
                  "JA": "トリプルヒット", "KO": "트리플히트"},
}


def latino(s):
    s = s.replace("’", "'").replace("‘", "'").replace(" ", " ")
    # ligaduras que a fonte nao tem: escrevem-se separadas (Coeur, Aetherium)
    for a, b in (("Œ", "OE"), ("œ", "oe"), ("Æ", "AE"), ("æ", "ae")):
        s = s.replace(a, b)
    s = unicodedata.normalize("NFD", s)
    s = "".join(c for c in s if unicodedata.category(c) != "Mn")
    s = s.upper()
    if any(ord(c) >= 128 for c in s):
        raise SystemExit("sobrou caractere fora do ASCII: %r" % s)
    return s


def ids_de_moves_h():
    h = open(os.path.join(RAIZ, "moves.h"), encoding="utf-8").read()
    ini = h.index("static const MoveEntry MOVE_TBL[MOVE_COUNT] = {")
    fim = h.index("};", ini)
    ids = re.findall(r'^\s*\{ "([A-Z0-9]+)",', h[ini:fim], re.M)
    n = int(re.search(r"#define MOVE_COUNT (\d+)", h).group(1))
    if len(ids) != n:
        raise SystemExit("li %d golpes e MOVE_COUNT e %d" % (len(ids), n))
    return ids


def slugs():
    lista = json.load(open(os.path.join(CACHE, "lista.json"), encoding="utf-8"))["results"]
    d = {}
    for m in lista:
        d.setdefault(m["name"].replace("-", "").upper(), m["name"])
    return d


def baixa(slug):
    p = os.path.join(CACHE, slug + ".json")
    for t in range(4):
        r = subprocess.run(["curl", "-s", "--max-time", "25", "-A", "Mozilla/5.0",
                            "https://pokeapi.co/api/v2/move/%s/" % slug],
                           capture_output=True, text=True, encoding="utf-8", errors="replace")
        if r.returncode == 0 and r.stdout.strip().startswith("{"):
            d = json.loads(r.stdout)
            json.dump({"names": d["names"], "id": d["id"]}, open(p, "w", encoding="utf-8"),
                      ensure_ascii=False)
            return
        time.sleep(1 + t)
    raise SystemExit("nao baixou " + slug)


def nomes(slug):
    d = json.load(open(os.path.join(CACHE, slug + ".json"), encoding="utf-8"))
    return {n["language"]["name"]: n["name"] for n in d["names"]}


def c_str(s):
    return '"' + s.replace("\\", "\\\\").replace('"', '\\"') + '"'


def main():
    ids = ids_de_moves_h()
    sl = slugs()
    tab = {lg: [] for lg in LINGUAS}
    sem = []
    for gid in ids:
        if gid in PROPRIOS:
            for lg in LINGUAS:
                tab[lg].append(PROPRIOS[gid][lg])
            continue
        slug = APELIDO.get(gid) or sl.get(gid)
        if not slug:
            sem.append(gid)
            for lg in LINGUAS:
                tab[lg].append(gid)
            continue
        if not os.path.exists(os.path.join(CACHE, slug + ".json")):
            if "--baixa" not in sys.argv:
                raise SystemExit("falta o cache de %s: rode com --baixa" % slug)
            baixa(slug)
        n = nomes(slug)
        en = latino(n["en"])
        for lg in LINGUAS:
            if lg == "PT":
                tab[lg].append(en)
            elif lg == "JA":
                tab[lg].append(unicodedata.normalize("NFKC", n.get("ja-hrkt", n["en"])))
            elif lg == "KO":
                tab[lg].append(n.get("ko", n["en"]))
            else:
                tab[lg].append(latino(n.get(API[lg], n["en"])))
    if sem:
        raise SystemExit("golpes sem nome (ponha em APELIDO ou PROPRIOS): " + " ".join(sem))

    out = ["// GERADO por tools/gen_move_names.py (PokeAPI) - nao editar a mao.",
           "// O nome do golpe NA TELA, por idioma. MOVE_TBL[].name e o id: nao mostrar.",
           "#pragma once",
           '#include "moves.h"',
           '#include "i18n.h"',
           "",
           "// assinatura: o id de cada linha (a bateria confere com MOVE_TBL)",
           "static const char *const MOVE_NAME_ID[MOVE_COUNT] = {"]
    out += ["  %s," % c_str(g) for g in ids]
    out.append("};")
    for lg in LINGUAS:
        if lg == "PT":
            continue          # PT usa o EN (a PokeAPI nao tem portugues)
        out.append("")
        out.append("static const char *const MOVE_NAME_%s[MOVE_COUNT] = {" % lg)
        out += ["  %s," % c_str(s) for s in tab[lg]]
        out.append("};")
    out += ["",
            "static inline const char *moveName(uint16_t mv) {",
            "  if (mv >= MOVE_COUNT) return MOVE_TBL[0].name;",
            "  switch (gLang) {",
            "    case LANG_ES: return MOVE_NAME_ES[mv];",
            "    case LANG_FR: return MOVE_NAME_FR[mv];",
            "    case LANG_DE: return MOVE_NAME_DE[mv];",
            "    case LANG_IT: return MOVE_NAME_IT[mv];",
            "    case LANG_JA: return MOVE_NAME_JA[mv];",
            "    case LANG_KO: return MOVE_NAME_KO[mv];",
            "    default:      return MOVE_NAME_EN[mv];   // EN e PT",
            "  }",
            "}",
            ""]
    open(os.path.join(RAIZ, "move_names.h"), "w", encoding="utf-8", newline="\n").write("\n".join(out))
    print("move_names.h: %d golpes x %d idiomas" % (len(ids), len(LINGUAS)))


if __name__ == "__main__":
    main()
