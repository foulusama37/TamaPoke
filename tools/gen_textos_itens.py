#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Nomes e descricoes de ITENS e HABILIDADES por idioma -> item_names.h e
abil_names.h (leva 78).

    python3 tools/gen_textos_itens.py

De onde vem cada coisa:
  EN    o que ja estava (items.h / abilities.h: a traducao do proprio Anil)
  ES    o PBS do Anil (nome e descricao), que e o original
  PT    descricao dos itens: items.h (tools/pt_items.py); o resto:
        tools/textos_itens/tr_PT.json
  FR DE IT JA KO
        NOMES: os oficiais (PokeAPI, cache em tools/downloads/pokeapi_csv);
        os que o Anil inventou (megas novas, Blitz...) e as DESCRICOES vem
        de tools/textos_itens/tr_XX.json, traduzidas DO TEXTO DO ANIL -- nao
        do oficial: a descricao e a especificacao da mecanica, e o Anil
        muda algumas (a PokeAPI tambem erra: em XY a descricao espanhola
        dos Restos e a do Lenco de Seda).

ITEM_TBL[].name e ABIL_TBL[].name continuam sendo o ID (serial, motor,
REPO das piedras). O que vai para a TELA sai de itemName()/abilName().

ASSINATURA: cada tabela guarda o nome ingles da linha (ITEM_NAME_SIG /
ABIL_NAME_SIG); a bateria confere com ITEM_TBL/ABIL_TBL linha a linha. Uma
tabela indexada pela linha de outra le o VIZINHO calada quando a outra
cresce.
"""
import csv
import json
import os
import re
import sys
import unicodedata

AQUI = os.path.dirname(os.path.abspath(__file__))
RAIZ = os.path.dirname(AQUI)
sys.path.insert(0, AQUI)
import gen_items          # noqa: E402
import gen_abilities      # noqa: E402
import texto_pt           # noqa: E402

CSV = os.path.join(AQUI, "downloads", "pokeapi_csv")
TR = os.path.join(AQUI, "textos_itens")
LINGUAS = ["ES", "EN", "FR", "DE", "IT", "PT", "JA", "KO"]
PKAPI = {"5": "FR", "6": "DE", "8": "IT", "1": "JA", "3": "KO"}   # ja-hrkt: kana
COMILLA = chr(34)
BARRA = chr(92)
# --rascunho: sem as traducoes ainda (so para compilar); cai no ingles
RASCUNHO = "--rascunho" in sys.argv
LIGADURAS = {"Œ": "OE", "œ": "oe"}


def chave(s):
    return re.sub(r"[^a-z0-9]", "", s.lower())


def c_utf8(s):
    return COMILLA + s.replace(BARRA, BARRA + BARRA).replace(COMILLA, BARRA + COMILLA) + COMILLA


def latino(s, onde):
    for a, b in LIGADURAS.items():
        s = s.replace(a, b)
    return COMILLA + texto_pt.codifica(s, onde, importado=True) + COMILLA


def cjk(s):
    s = unicodedata.normalize("NFKC", s).replace("\n", " ")
    return c_utf8(re.sub(r" +", " ", s).strip())


def literal(lg, s, onde):
    if s is None:
        return "nullptr"
    s = re.sub(r"\s+", " ", s.replace("­", "")).strip()
    return cjk(s) if lg in ("JA", "KO") else latino(s, onde)


def oficiais(idcsv, nomcsv, idk):
    ids = {}
    for r in csv.DictReader(open(os.path.join(CSV, idcsv), encoding="utf-8")):
        ids.setdefault(chave(r["identifier"]), r["id"])
    nm = {}
    for r in csv.DictReader(open(os.path.join(CSV, nomcsv), encoding="utf-8")):
        lg = PKAPI.get(r["local_language_id"])
        if lg:
            nm.setdefault(r[idk], {})[lg] = r["name"]
    return ids, nm


PAT = re.compile(COMILLA + "((?:[^" + COMILLA + BARRA + BARRA + "]|" + BARRA + BARRA + ".)*)" + COMILLA)


def nomes_tbl(h, marca):
    i = h.index(marca)
    return [texto_pt.descodifica(x) for x in
            re.findall(r"^\s*\{ " + COMILLA + "((?:[^" + COMILLA + "])*)" + COMILLA + ",",
                       h[i:h.index("\n};", i)], re.M)]


def descs_tbl(h, marca):
    i = h.index(marca)
    corpo = h[h.index("{", i) + 1:h.index("\n};", i)]
    out = []
    for ln in corpo.split("\n"):
        ln = ln.strip()
        if not ln or ln.startswith("//"):
            continue
        if ln.startswith("nullptr"):
            out.append(None)
            continue
        m = PAT.search(ln)
        if m:
            out.append(texto_pt.descodifica(m.group(1)))
    return out


def carrega_tr():
    tr = {}
    for lg in ("FR", "DE", "IT", "PT", "JA", "KO"):
        p = os.path.join(TR, "tr_%s.json" % lg)
        if RASCUNHO and not os.path.exists(p):
            tr[lg] = {"itens": {}, "habil": {}}
            continue
        tr[lg] = json.load(open(p, encoding="utf-8"))
    return tr


def emite(arq, pref, nomes, descs, sig, fn_nome, fn_desc, cnt, tipo_id, descs_externas):
    """nomes/descs: {lg: [texto por linha]} (descs pode faltar lg -> usa externa)"""
    out = ["// GERADO por tools/gen_textos_itens.py - nao editar a mao.",
           "// Nome e descricao NA TELA, por idioma. %s_TBL[].name e o id." % pref,
           "#pragma once",
           '#include "i18n.h"',
           '#include "%s"' % ("items.h" if pref == "ITEM" else "abilities.h"),
           "",
           "// assinatura: o nome ingles de cada linha (a bateria confere com %s_TBL)" % pref,
           "static const char *const %s_NAME_SIG[%s] = {" % (pref, cnt)]
    out += ["  %s," % latino(x, "sig") for x in sig]
    out.append("};")
    for lg in LINGUAS:
        if lg == "EN":
            continue
        out += ["", "static const char *const %s_NAME_%s[%s] = {" % (pref, lg, cnt)]
        out += ["  %s," % literal(lg, x, "%s nome %s %d" % (pref, lg, i)) for i, x in enumerate(nomes[lg])]
        out.append("};")
    for lg in LINGUAS:
        if lg in descs_externas:
            continue
        out += ["", "// nullptr = sem texto: cai no ingles",
                "static const char *const %s_DESC_%s[%s] = {" % (pref, lg, cnt)]
        out += ["  %s," % literal(lg, x, "%s desc %s %d" % (pref, lg, i)) for i, x in enumerate(descs[lg])]
        out.append("};")

    def tab(kind, lg):
        if kind == "DESC" and lg in descs_externas:
            return descs_externas[lg]
        if kind == "NAME" and lg == "EN":
            return None
        return "%s_%s_%s" % (pref, kind, lg)

    out += ["", "static inline const char *%s(%s i) {" % (fn_nome, tipo_id),
            "  if (i >= %s) i = 0;" % cnt,
            "  switch (gLang) {"]
    for lg in LINGUAS:
        if lg == "EN":
            continue
        out.append("    case LANG_%s: return %s[i];" % (lg, tab("NAME", lg)))
    out += ["    default: return %s_TBL[i].name;" % pref, "  }", "}", "",
            "static inline const char *%s(%s i) {" % (fn_desc, tipo_id),
            "  if (i >= %s) return \"\";" % cnt,
            "  const char *s = nullptr;",
            "  switch (gLang) {"]
    for lg in LINGUAS:
        if lg == "EN":
            continue
        out.append("    case LANG_%s: s = %s[i]; break;" % (lg, tab("DESC", lg)))
    out += ["    default: break;", "  }",
            "  return s ? s : %s[i];" % tab("DESC", "EN"), "}", ""]
    open(os.path.join(RAIZ, arq), "w", encoding="utf-8", newline="\n").write("\n".join(out))


def main():
    tr = carrega_tr()

    # ---------------- itens
    h = open(os.path.join(RAIZ, "items.h"), encoding="utf-8").read()
    en_n = nomes_tbl(h, "static const ItemEntry ITEM_TBL[] = {")
    idx = sorted(gen_items.indices().items(), key=lambda x: x[1])
    assert [i for _, i in idx] == list(range(1, len(en_n))), "items.h fora de ordem com gen_items"
    es_n = gen_items.nombres_es()
    ids, nm = oficiais("items.csv", "item_names.csv", "item_id")
    nomes = {lg: ["-"] for lg in LINGUAS}
    descs = {lg: [None] for lg in LINGUAS}
    falta = []
    for ident, i in idx:
        of = nm.get(ids.get(chave(ident)), {})
        for lg in LINGUAS:
            if lg == "EN":
                nomes[lg].append(en_n[i])
            elif lg == "ES":
                nomes[lg].append(es_n.get(ident) or en_n[i])
            else:
                t = tr[lg]["itens"].get(ident, {})
                n = t.get("nome") if lg == "PT" else (of.get(lg) or t.get("nome"))
                if not n:
                    falta.append("item %s %s" % (lg, ident))
                    n = en_n[i]
                nomes[lg].append(n)
                if lg != "PT":
                    descs[lg].append(t.get("desc"))
    if falta and not RASCUNHO:
        raise SystemExit("falta nome: " + ", ".join(falta[:20]))
    emite("item_names.h", "ITEM", nomes, descs, en_n, "itemName", "itemDesc",
          "ITEM_COUNT", "uint8_t",
          {"EN": "ITEM_DESC", "ES": "ITEM_DESC_ES", "PT": "ITEM_DESC_PT"})
    print("item_names.h: %d itens x %d idiomas" % (len(en_n), len(LINGUAS)))

    # ---------------- habilidades
    h = open(os.path.join(RAIZ, "abilities.h"), encoding="utf-8").read()
    en_n = nomes_tbl(h, "static const AbilityEntry ABIL_TBL[] = {")
    idx = sorted(gen_abilities.indices().items(), key=lambda x: x[1])
    assert [i for _, i in idx] == list(range(1, len(en_n))), "abilities.h fora de ordem"
    hab = {n: d for n, d in gen_abilities.secciones(os.path.join(gen_abilities.PBS, "abilities.txt"))}
    ids, nm = oficiais("abilities.csv", "ability_names.csv", "ability_id")
    nomes = {lg: ["-"] for lg in LINGUAS}
    descs = {lg: [None] for lg in LINGUAS}
    for ident, i in idx:
        of = nm.get(ids.get(chave(ident)), {})
        for lg in LINGUAS:
            if lg == "EN":
                nomes[lg].append(en_n[i])
            elif lg == "ES":
                nomes[lg].append(hab.get(ident, {}).get("Name") or en_n[i])
                descs[lg].append(hab.get(ident, {}).get("Description") or None)
            else:
                t = tr[lg]["habil"].get(ident, {})
                n = t.get("nome") if lg == "PT" else (of.get(lg) or t.get("nome"))
                if not n:
                    falta.append("habil %s %s" % (lg, ident))
                    n = en_n[i]
                nomes[lg].append(n)
                descs[lg].append(t.get("desc"))
    if falta and not RASCUNHO:
        raise SystemExit("falta nome: " + ", ".join(falta[:20]))
    emite("abil_names.h", "ABIL", nomes, descs, en_n, "abilName", "abilDesc",
          "ABIL_COUNT", "uint16_t", {"EN": "ABIL_DESC"})
    print("abil_names.h: %d habilidades x %d idiomas" % (len(en_n), len(LINGUAS)))
    if texto_pt.pelados:
        print("  %d maiusculas sem glifo perderam o acento (ex.: %s)" % (len(texto_pt.pelados), texto_pt.pelados[0]))


if __name__ == "__main__":
    main()
