# -*- coding: utf-8 -*-
"""Gera qr_manual.h: o QR code que leva ao manual (docs/manual no GitHub).

Dois QR: o do LEIAME (portugues) e o do README (ingles, para os outros
idiomas). Os dois enderecos tem o mesmo tamanho, entao saem do mesmo
tamanho de grade; o script para se nao for assim.

Cada linha da grade e um bitmap (bit 7 do primeiro byte = modulo da
esquerda), 1 = modulo escuro. A zona de silencio NAO esta aqui: quem
desenha poe a margem branca.

    pip install segno
    python tools/gen_qr_manual.py
"""
import os
import segno

BASE = "https://github.com/foulusama37/TamaPoke/blob/main/docs/manual/"
URLS = [("QR_MANUAL_PT", BASE + "LEIAME.md"), ("QR_MANUAL_EN", BASE + "README.md")]
RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

grades = []
for nome, url in URLS:
    q = segno.make(url, error="m", micro=False, boost_error=False)
    m = [[1 if c else 0 for c in row] for row in q.matrix]
    grades.append((nome, url, q.version, m))
n = len(grades[0][3])
assert all(len(g[3]) == n for g in grades), "os dois QR tem de ter o mesmo tamanho"
bpl = (n + 7) // 8

out = ["// GERADO por tools/gen_qr_manual.py -- nao editar a mao.",
       "// O QR dos ajustes: leva ao manual em docs/manual do repo.",
       "#pragma once",
       "#include <stdint.h>",
       "#define QR_MANUAL_N %d          // modulos por lado (sem a margem)" % n,
       "#define QR_MANUAL_BPL %d        // bytes por linha" % bpl, ""]
for nome, url, ver, m in grades:
    out.append("// %s  (versao %d, correcao M)" % (url, ver))
    out.append("static const uint8_t %s[QR_MANUAL_N * QR_MANUAL_BPL] = {" % nome)
    for row in m:
        bs = []
        for k in range(bpl):
            b = 0
            for j in range(8):
                c = k * 8 + j
                if c < n and row[c]:
                    b |= 0x80 >> j
            bs.append("0x%02x" % b)
        out.append("  " + ", ".join(bs) + ",")
    out.append("};")
    out.append("")
open(os.path.join(RAIZ, "qr_manual.h"), "w", encoding="utf-8", newline="\n").write("\n".join(out))
print("qr_manual.h: %d x %d" % (n, n))
