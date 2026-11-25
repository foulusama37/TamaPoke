# -*- coding: utf-8 -*-
"""As fontes CJK de VERDADE para o simulador (leva 79).

O simulador desenhava o japones e o coreano com a fonte latina, byte a
byte: a previa saia com lixo no lugar das letras e os testes mediam o
texto com 6 px por BYTE (um kana = 18 px a talla 2; na placa sao 16).

Isto copia os dois arrays que vao para a placa (u8g2_font_unifont_t_
japanese3 e _korean2, de U8g2/src/clib/u8g2_fonts.c, a mesma biblioteca
que o arduino-cli compila) para desktop/shim/u8g2_cjk_dados.inc, que o
shim de Arduino_GFX decodifica com o MESMO codigo da biblioteca.

O .inc nao vai no git (a fonte e da U8g2/unifont, com a licenca dela):
os Makefiles do simulador e dos testes o geram quando falta.

    python tools/gen_u8g2_cjk_sim.py
"""
import os
import sys

AQUI = os.path.dirname(os.path.abspath(__file__))
RAIZ = os.path.dirname(AQUI)
sys.path.insert(0, AQUI)
import glifos_cjk  # noqa: E402

DST = os.path.join(RAIZ, "desktop", "shim", "u8g2_cjk_dados.inc")


def main():
    partes = ["// GERADO por tools/gen_u8g2_cjk_sim.py a partir de " + glifos_cjk.LIB.replace("\\", "/"),
              "// - nao editar, nao vai no git.", ""]
    for nome in (glifos_cjk.JA, glifos_cjk.KO):
        b = glifos_cjk._bytes_do_array(nome)
        partes.append("const uint8_t %s[%d] = {" % (nome, len(b)))
        for i in range(0, len(b), 24):
            partes.append("  " + ",".join(str(x) for x in b[i:i + 24]) + ",")
        partes.append("};")
        partes.append("")
    tmp = DST + ".tmp"
    with open(tmp, "w", encoding="ascii", newline="\n") as f:
        f.write("\n".join(partes))
    os.replace(tmp, DST)
    print(DST, os.path.getsize(DST), "bytes")


if __name__ == "__main__":
    main()
