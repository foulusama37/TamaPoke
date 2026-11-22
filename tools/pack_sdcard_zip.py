# -*- coding: utf-8 -*-
"""Monta o pacote do cartao SD para o Release do GitHub.

O zip tem exatamente as pastas que o firmware le do cartao, a partir de
tools/sdcard/ (o que ja foi validado e mandado para as placas). Extrair na
RAIZ do cartao.

NUNCA entram: save/, boxes/, troca/ -- sao o save do jogador. Extrair o
pacote num cartao em uso apagaria o jogo de quem esta atualizando.

Depois de escrever, o zip e reaberto e conferido: cada arquivo de origem
tem de estar la, com o mesmo tamanho e o mesmo CRC.

    python tools/pack_sdcard_zip.py            # -> dist/TamaPoke-sdcard-vX.YY.zip
"""
import os
import re
import sys
import zipfile
import zlib

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SD = os.path.join(RAIZ, "tools", "sdcard")
PASTAS = ["mons", "por", "snd", "av", "fx", "anim", "map", "bg"]   # o que o firmware le
PROIBIDAS = {"save", "boxes", "troca"}                               # o save do jogador
assert not PROIBIDAS & set(PASTAS)

ver = re.search(r'#define\s+FW_VERSION\s+"([^"]+)"',
                open(os.path.join(RAIZ, "TamaPoke.ino"), encoding="utf-8").read())
if not ver:
    sys.exit("nao achei FW_VERSION no TamaPoke.ino")
ver = ver.group(1)
dist = os.path.join(RAIZ, "dist")
os.makedirs(dist, exist_ok=True)
saida = os.path.join(dist, "TamaPoke-sdcard-v%s.zip" % ver)


def crc(p):
    c = 0
    with open(p, "rb") as f:
        for bloco in iter(lambda: f.read(1 << 20), b""):
            c = zlib.crc32(bloco, c)
    return c & 0xFFFFFFFF


origem = {}
for pasta in PASTAS:
    base = os.path.join(SD, pasta)
    if not os.path.isdir(base):
        sys.exit("falta tools/sdcard/%s: rode o pack_* dessa pasta antes" % pasta)
    n = 0
    for dirpath, _, nomes in os.walk(base):
        for nome in nomes:
            p = os.path.join(dirpath, nome)
            arc = os.path.relpath(p, SD).replace(os.sep, "/")
            origem[arc] = p
            n += 1
    if n == 0:
        sys.exit("tools/sdcard/%s esta vazia" % pasta)
    print("  %-5s %5d arquivos" % (pasta, n))

with zipfile.ZipFile(saida, "w", zipfile.ZIP_DEFLATED, compresslevel=6) as z:
    for arc in sorted(origem):
        z.write(origem[arc], arc)
    z.writestr("LEIA-ME.txt",
               "TamaPoke v%s - cartao SD\n\n"
               "Extraia TUDO na RAIZ do cartao microSD (as pastas mons, snd, av...\n"
               "ficam direto no cartao, nao dentro de outra pasta).\n"
               "O seu save NAO esta aqui e nao e tocado.\n\n"
               "Extract EVERYTHING to the ROOT of the microSD card. Your save is not\n"
               "in this package and is left untouched.\n" % ver)

# confere: reabrir e bater tudo
erros = 0
with zipfile.ZipFile(saida) as z:
    info = {i.filename: i for i in z.infolist()}
    for arc, p in origem.items():
        i = info.get(arc)
        if i is None or i.file_size != os.path.getsize(p) or i.CRC != crc(p):
            erros += 1
            print("ERRO:", arc)
    for nome in info:
        if nome.split("/")[0] in PROIBIDAS:
            erros += 1
            print("ERRO: pasta de save no pacote:", nome)
    if z.testzip() is not None:
        erros += 1
        print("ERRO: zip corrompido")
if erros:
    os.remove(saida)
    sys.exit("%d erros: pacote APAGADO" % erros)
print("%s: %d arquivos, %.0f MB (conferido)" % (os.path.relpath(saida, RAIZ), len(origem),
                                               os.path.getsize(saida) / 1e6))
