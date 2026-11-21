"""Retratos de FORMAS MEGA, um arquivo por forma: /por/m<DEX>_<FORMA>.bin.

    python tools/gen_retratos_mega.py

O SpriteCollab nao tem retrato de mega; os que existem foram feitos aqui
(tools/retrato_mega_arcanine.py, a Mega Arcanine que o dono aprovou) e
moram em tools/retratos_extra/m<DEX>_<FORMA>/<Emocao>.png.

Arquivo proprio e nao uma vaga a mais no por.bin: aquele e indexado por
especie e tem assinatura; esticar a tabela mudaria tudo o que ja esta no
cartao. Aqui a identidade e (dex base, numero da forma no Anil) —— o mesmo
par que o MEGA_TBL guarda —— e o firmware confere os dois ao abrir.

Formato TPPM v1:
  "TPPM" u8 versao(1) u8 forma u16 dex u8 ranuras u8 0
  u32 offNormal  u32 offShiny (0 = sem shiny; a subpasta shiny/)
  e cada bloco e igual ao de uma especie no por.bin (gen_retratos):
  u8 nUnicas, u8 mapa[ranuras], pad ate par, nUnicas x (u8 w, u8 h,
  u8 nPal, u8 0, u16 rle, u16 0), paletas, RLEs.
"""
import os
import re
import struct
import sys

AQUI = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, AQUI)
import gen_retratos as G  # noqa: E402

EXTRA = os.path.join(AQUI, "retratos_extra")


def bloco(pasta):
    rutas, mapa = [], []
    for _nome, prefs in G.ESTADOS:
        r, _emo = G.resolve([pasta], prefs)
        if r is None:
            mapa.append(0xFF)
            continue
        if r not in rutas:
            rutas.append(r)
        mapa.append(rutas.index(r))
    if not rutas:
        return None
    mapa = [0 if m == 0xFF else m for m in mapa]
    piezas = [G.pedazo(r) for r in rutas]
    cab = bytearray([len(piezas)]) + bytes(mapa)
    if len(cab) % 2:
        cab.append(0)
    pals, rles = bytearray(), bytearray()
    for (w, h, pal, rle) in piezas:
        cab += struct.pack("<BBBBHH", w, h, len(pal), 0, len(rle), 0)
        for c in pal:
            pals += struct.pack("<H", c)
        rles += bytes(rle)
    return bytes(cab) + bytes(pals) + bytes(rles)


def main():
    os.makedirs(G.SALIDA, exist_ok=True)
    feitos = 0
    for nome in sorted(os.listdir(EXTRA)):
        m = re.fullmatch(r"m(\d{3})_(\d+)", nome)
        if not m:
            continue
        dex, forma = int(m.group(1)), int(m.group(2))
        b = bloco(os.path.join(EXTRA, nome))
        if b is None:
            continue
        pasta_sh = os.path.join(EXTRA, nome, "shiny")
        bs = bloco(pasta_sh) if os.path.isdir(pasta_sh) else None
        cab = 4 + 6 + 8
        off_n, off_s = cab, (cab + len(b)) if bs else 0
        out = (b"TPPM" + struct.pack("<BBHBB", 1, forma, dex, len(G.ESTADOS), 0)
               + struct.pack("<II", off_n, off_s) + b + (bs or b""))
        with open(os.path.join(G.SALIDA, nome + ".bin"), "wb") as fh:
            fh.write(out)
        feitos += 1
        print("%s.bin  %d bytes" % (nome, len(out)))
    print("%d retratos de mega" % feitos)


if __name__ == "__main__":
    main()
