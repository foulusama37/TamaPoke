# -*- coding: utf-8 -*-
"""Que letras tem DE VERDADE as fontes CJK do firmware (leva 79).

Arduino_GFX nao desenha nada quando falta o glifo (nem avanca o cursor):
uma letra fora da fonte some da tela sem aviso. Este modulo le o array
u8g2_font_unifont_t_* direto de u8g2_fonts.c (o mesmo que vai para a
placa) e devolve o conjunto de codepoints, percorrendo o formato u8g2:

  cabecalho de 23 bytes; bytes 21-22 = offset da parte unicode (>= 0x100)
  parte < 0x100: [cod 1 byte][salto 1 byte]... ate salto 0
  parte unicode: tabela de busca [offset 2][cod 2]... ate cod 0xFFFF,
                 depois [cod 2 bytes BE][salto 1 byte]... ate cod 0

Uso: python tools/glifos_cjk.py   -> confere todas as cadeias JA/KO dos .h
"""
import os
import re
import sys

LIB = os.path.expanduser(
    r"~\OneDrive\Documentos\Arduino\libraries\U8g2\src\clib\u8g2_fonts.c")
RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

_cache = {}


def _bytes_do_array(nome):
    src = open(LIB, "rb").read().decode("latin-1")
    i = src.index("const uint8_t " + nome + "[")
    # o array tem ';' DENTRO das cadeias: le literal a literal ate o ';' de fora
    lits = []
    rx = re.compile(r'\s*"((?:[^"\\]|\\.)*)"')
    p = src.index("=", i) + 1
    while True:
        m = rx.match(src, p)
        if not m:
            break
        lits.append(m.group(1))
        p = m.end()
    assert src[p:].lstrip().startswith(";"), nome
    out = bytearray()
    for lit in lits:
        k = 0
        while k < len(lit):
            c = lit[k]
            if c != "\\":
                out.append(ord(c))
                k += 1
                continue
            n = lit[k + 1]
            if n in "01234567":
                m = re.match(r"[0-7]{1,3}", lit[k + 1:]).group(0)
                out.append(int(m, 8))
                k += 1 + len(m)
            elif n == "x":
                m = re.match(r"[0-9a-fA-F]+", lit[k + 2:]).group(0)
                out.append(int(m, 16))
                k += 2 + len(m)
            else:
                out.append({"n": 10, "t": 9, "r": 13, "\\": 92, '"': 34,
                            "'": 39, "?": 63, "a": 7, "b": 8, "f": 12,
                            "v": 11}[n])
                k += 2
    out.append(0)  # o NUL implicito da cadeia C conta no tamanho do array
    return bytes(out)


def glifos(nome):
    if nome in _cache:
        return _cache[nome]
    b = _bytes_do_array(nome)
    cods = set()
    p = 23
    while True:
        cod, salto = b[p], b[p + 1]
        if salto == 0:
            break
        cods.add(cod)
        p += salto
    uni = 23 + (b[21] << 8 | b[22])
    # a tabela de busca: o 1o offset (relativo a 'uni') salta a propria tabela
    # e cai no 1o glifo (u8g2_font_get_glyph_data); os glifos sao contiguos
    p = uni + (b[uni] << 8 | b[uni + 1])
    while True:
        cod = b[p] << 8 | b[p + 1]
        if cod == 0:
            break
        cods.add(cod)
        p += b[p + 2]
    _cache[nome] = cods
    return cods


JA = "u8g2_font_unifont_t_japanese3"
KO = "u8g2_font_unifont_t_korean2"


def faltam(texto, nome):
    g = glifos(nome)
    return sorted({ch for ch in texto if ord(ch) >= 0x80 and ord(ch) not in g})


def tabelas_cjk(caminho):
    """(nome da tabela, idioma, [cadeias]) das tabelas *_JA / *_KO de um .h"""
    s = open(caminho, encoding="utf-8").read()
    for m in re.finditer(r"static const char \*const (\w+_(JA|KO))\[[^\]]*\] = \{(.*?)\n\};", s, re.S):
        cads = [eval('"' + c + '"').encode("latin-1").decode("utf-8")
                if False else _desc(c) for c in re.findall(r'"((?:[^"\\]|\\.)*)"', m.group(3))]
        yield m.group(1), m.group(2), cads


def _desc(c):
    # literal C (UTF-8 cru + escapes octais) -> str
    out = bytearray()
    k = 0
    while k < len(c):
        if c[k] == "\\" and c[k + 1] in "01234567":
            m = re.match(r"[0-7]{1,3}", c[k + 1:]).group(0)
            out.append(int(m, 8))
            k += 1 + len(m)
        elif c[k] == "\\":
            out.append({"n": 10, "\\": 92, '"': 34}[c[k + 1]])
            k += 2
        else:
            out += c[k].encode("utf-8")
            k += 1
    return out.decode("utf-8")


def main():
    ja, ko = glifos(JA), glifos(KO)
    print(f"japanese3: {len(ja)} glifos   korean2: {len(ko)} glifos")
    ruins = 0
    for h in sorted(os.listdir(RAIZ)):
        if not h.endswith(".h"):
            continue
        for tab, lg, cads in tabelas_cjk(os.path.join(RAIZ, h)):
            fonte = JA if lg == "JA" else KO
            falta = {}
            for i, c in enumerate(cads):
                for ch in faltam(c, fonte):
                    falta.setdefault(ch, []).append(i)
            if falta:
                ruins += len(falta)
                ex = ", ".join(f"{ch}(U+{ord(ch):04X}) x{len(v)}" for ch, v in sorted(falta.items())[:12])
                print(f"  {h} {tab}: {len(falta)} letras SEM glifo: {ex}")
            else:
                print(f"  {h} {tab}: {len(cads)} cadeias, todas as letras na fonte")
    sys.exit(1 if ruins else 0)


if __name__ == "__main__":
    main()
