# -*- coding: utf-8 -*-
"""Maiusculas para a fonte da placa (CP437) e literal C com os bytes dela.

A fonte classica tem as maiusculas acentuadas E Ç Ä Ö Ü Ñ (e o jogo poe
A~/O~ nos bytes 0262/0263, ver i18n.h). As que NAO tem (Á Í Ó Ú À È Ê...)
perdem o acento: e o que se escreve num letreiro sem a letra. Assim o
alemao sai KÄFER e nao KAFER, o portugues DRAGÃO, o frances ÉLECTRIK.
"""
import unicodedata

TEM = set("ÉÇÄÖÜÑÃÕ")
TIL = {"Ã": 0o262, "Õ": 0o263}
LIGADURA = {"Œ": "OE", "Æ": "AE", "ß": "SS"}


def maiuscula(s):
    s = s.replace("’", "'").replace("‘", "'").replace(" ", " ")
    s = s.upper()
    for a, b in LIGADURA.items():
        s = s.replace(a, b)
    out = []
    for c in s:
        if ord(c) < 128 or c in TEM:
            out.append(c)
            continue
        base = "".join(x for x in unicodedata.normalize("NFD", c) if unicodedata.category(x) != "Mn")
        if not base or any(ord(x) >= 128 for x in base):
            raise SystemExit("letra sem saida na fonte: %r em %r" % (c, s))
        out.append(base)
    return "".join(out)


def literal_c(s):
    """Literal C: ASCII como esta, o resto em octal com o byte da fonte."""
    out = ['"']
    for c in s:
        if c in '"\\':
            out.append("\\" + c)
        elif ord(c) < 128:
            out.append(c)
        else:
            b = TIL[c] if c in TIL else c.encode("cp437")[0]
            out.append("\\%03o" % b)
    out.append('"')
    return "".join(out)
