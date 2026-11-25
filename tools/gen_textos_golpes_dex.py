#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Descricoes de GOLPE e fichas de POKEDEX em FR/DE/IT/JA/KO -> move_descs.h
e dex_text_l.h (leva 79).

    python3 tools/gen_textos_golpes_dex.py [--rascunho]

ES, EN e PT continuam onde estavam (moves.h e dex_text.h). Aqui entram os
outros cinco:

  golpes  tools/textos_golpes/tr_XX.json, traduzidos DO TEXTO DO ANIL (a
          descricao e a especificacao da mecanica no Anil, nao a oficial)
  dex     tools/textos_dex/dex_oficial.json (tools/dex_oficial.py): a ficha
          oficial do MESMO jogo de onde o Anil tirou a espanhola

MOVE_DESC_SIG guarda o ID de cada linha de MOVE_TBL: a bateria confere linha
a linha (uma tabela indexada pela linha de outra le o VIZINHO calada quando a
outra cresce). A Pokedex vai pelo numero, que e a identidade do bicho.

--rascunho: sem as traducoes ainda (so para compilar); o que falta vira
nullptr e moveDesc() cai no ingles.
"""
import json
import os
import re
import sys

AQUI = os.path.dirname(os.path.abspath(__file__))
RAIZ = os.path.dirname(AQUI)
sys.path.insert(0, AQUI)
import texto_pt      # noqa: E402
import glifos_cjk    # noqa: E402
from gen_textos_itens import latino, cjk, COMILLA  # noqa: E402

LINGUAS = ["FR", "DE", "IT", "JA", "KO"]
RASCUNHO = "--rascunho" in sys.argv
SEXO = {"♀": "\x0c", "♂": "\x0b"}   # ♀ ♂ = codigos 12 e 11 da fonte classica


def ids_dos_golpes():
    h = open(os.path.join(RAIZ, "moves.h"), encoding="utf-8").read()
    i = h.index("static const MoveEntry MOVE_TBL[MOVE_COUNT] = {")
    return re.findall(r'^\s*\{ "([^"]*)",', h[i:h.index("\n};", i)], re.M)


def literal(lg, s, onde):
    if s is None:
        return "nullptr"
    if lg in ("JA", "KO"):
        falta = glifos_cjk.faltam(s, glifos_cjk.JA if lg == "JA" else glifos_cjk.KO)
        if falta:
            sys.exit("%s: a fonte da placa nao tem %s" % (onde, " ".join("U+%04X" % ord(c) for c in falta)))
        return cjk(s)
    for a, b in SEXO.items():
        s = s.replace(a, b)
    # importado=False: uma letra sem glifo PARA o gerador em vez de perder o acento calada
    return COMILLA + texto_pt.codifica(s, onde) + COMILLA


def golpes(ids):
    tr = {}
    for lg in LINGUAS:
        p = os.path.join(AQUI, "textos_golpes", "tr_%s.json" % lg)
        if os.path.exists(p):
            tr[lg] = json.load(open(p, encoding="utf-8"))["golpes"]
        elif RASCUNHO:
            tr[lg] = {}
        else:
            sys.exit("falta " + p)
        sobra = set(tr[lg]) - set(ids)
        if sobra:
            sys.exit("tr_%s.json: chaves que nao sao golpe: %s" % (lg, sorted(sobra)[:10]))
        falta = [k for k in ids if not tr[lg].get(k)]
        if falta and not RASCUNHO:
            sys.exit("tr_%s.json: faltam %d golpes (%s...)" % (lg, len(falta), falta[:5]))
    L = []
    L.append("// GERADO por tools/gen_textos_golpes_dex.py - nao editar a mao.")
    L.append("// O que faz cada golpe em FR/DE/IT/JA/KO, traduzido do texto do Anil")
    L.append("// (tools/textos_golpes/tr_XX.json). ES/EN/PT estao em moves.h.")
    L.append("#pragma once")
    L.append('#include "moves.h"')
    L.append("")
    L.append("// ASSINATURA: o ID de cada linha de MOVE_TBL (a bateria confere)")
    L.append("static const char *const MOVE_DESC_SIG[MOVE_COUNT] = {")
    L += ['  "%s",' % k for k in ids]
    L.append("};")
    for lg in LINGUAS:
        L.append("")
        L.append("static const char *const MOVE_DESC_%s[MOVE_COUNT] = {" % lg)
        for k in ids:
            L.append("  %s,  // %s" % (literal(lg, tr[lg].get(k) or None, "golpe %s %s" % (lg, k)), k))
        L.append("};")
    L.append("")
    return "\n".join(L)


def dex():
    d = json.load(open(os.path.join(AQUI, "textos_dex", "dex_oficial.json"), encoding="utf-8"))
    L = []
    L.append("// GERADO por tools/gen_textos_golpes_dex.py - nao editar a mao.")
    L.append("// Fichas de Pokedex oficiais em FR/DE/IT/JA/KO, da mesma versao de jogo")
    L.append("// que a ficha espanhola do Anil (tools/dex_oficial.py). Indice = numero.")
    L.append("#pragma once")
    for lg in LINGUAS:
        t = d[lg]
        L.append("")
        L.append("static const char *const DEX_TEXT_%s[%d] = {" % (lg, len(t)))
        L.append('  "",')
        for n in range(1, len(t)):
            L.append("  %s,  // %d" % (literal(lg, t[n], "dex %s %d" % (lg, n)), n))
        L.append("};")
    L.append("")
    return "\n".join(L), len(d["FR"])


def main():
    ids = ids_dos_golpes()
    g = golpes(ids)
    x, n = dex()
    for nome, txt in (("move_descs.h", g), ("dex_text_l.h", x)):
        with open(os.path.join(RAIZ, nome), "w", encoding="utf-8", newline="\n") as f:
            f.write(txt)
    print("move_descs.h: %d golpes x %d idiomas%s | dex_text_l.h: %d fichas" %
          (len(ids), len(LINGUAS), " (RASCUNHO)" if RASCUNHO else "", n - 1))
    for a in texto_pt.avisados:
        print("  aviso:", a)


if __name__ == "__main__":
    main()
