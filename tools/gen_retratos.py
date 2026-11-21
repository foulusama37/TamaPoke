#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Os retratos do SpriteCollab -> /por/por.bin

Pra que servem (decisao do dono): NAO pra trocar os sprites, que ficam
como estao. Sao as REACOES —— o bicho com fome, com sono, sem PS, bravo
porque esta sujo ou sem atencao —— mais a cara comemorando na tela de
experiencia e a cara na Pokedex.

O buraco e a cobertura: so o `Normal` existe pras 1025 especies; as
outras quinze emocoes ficam entre 70% e 78%. Entao cada ESTADO cai pro
`Sigh` e, se nem esse existir, pro `Normal` —— decisao dele: "qualquer
coisa usa o sigh mesmo". Como a onomatopeia (o Z, a cruz vermelha, o
balao) vai em TODOS, nenhum bicho fica mudo mesmo com a cara neutra.

A resolucao e feita AQUI, nao no firmware: o cartao ja entrega a cara
certa pra cada estado. O firmware nao sabe o que e um "Sigh".

Formato, espelhando o do /fx/fx.bin:

    "TPPR"              4
    uint16 especies     tem que bater com DEX_COUNT, senao o firmware
                        recusa o arquivo inteiro (ver abaixo)
    uint8  ranuras      quantos estados; idem
    uint8  0
    uint32 off[especies + 1]    0 = sem retrato

  e por especie:
    uint8  nUnicas
    uint8  mapa[ranuras]        qual imagem unica usa cada estado
    uint8  relleno ate par
    por imagem unica: uint8 w, h, nPal, 0; uint16 nRle, 0
    depois todas as paletas, depois todos os RLE

A ASSINATURA nao e enfeite. Isto e indexado por numero de especie, e e
exatamente o tipo de arquivo que, quando a tabela cresce, passa a
mostrar o vizinho em silencio —— foi o que aconteceu com os sons dos
golpes. Por isso vai a contagem no cabecalho e o firmware exige que
bata EXATAMENTE.

    python tools/gen_retratos.py G:/SpriteCollab/repo
"""
import collections
import io
import os
import re
import struct
import sys

from PIL import Image, ImageChops

AQUI = os.path.dirname(os.path.abspath(__file__))
RAIZ = os.path.dirname(AQUI)
sys.path.insert(0, AQUI)

import gen_art                                       # noqa: E402

SALIDA = os.path.join(AQUI, "sdcard", "por")

# Os estados, NESTA ordem. O firmware usa o indice; mexer na ordem sem
# regerar o cartao mostra a cara errada —— por isso a contagem vai na
# assinatura.
#
# A emocao de cada um e uma escolha, nao um dado: o SpriteCollab nao tem
# "fome" nem "sono". `Sigh` e um suspiro e `Dizzy` e tonto; os dois
# servem de aproximacao, e quem carrega o significado de verdade e a
# onomatopeia desenhada por cima.
ESTADOS = [
    ("POR_NORMAL", ["Normal"]),                     # Pokedex, e o ultimo recurso
    ("POR_FOME",   ["Sigh", "Worried"]),
    # Sigh, nao Dizzy: foi o combinado ("qualquer coisa usa o sigh
    # mesmo"). Quem carrega o recado de sono e o Z desenhado por cima.
    ("POR_SONO",   ["Sigh"]),
    ("POR_SEM_PS", ["Pain", "Teary-Eyed"]),
    ("POR_SUJO",   ["Angry", "Shouting"]),
    ("POR_FESTA",  ["Joyous", "Happy"]),
    # Ao entrar no matinho: o bicho se decide e vai. Substitui o
    # aviso de "!" no alto da tela, que mandava o olhar pra longe
    # dele bem na hora em que ele e o assunto.
    ("POR_PRONTO", ["Determined", "Shouting"]),
]
# A rede de seguranca, depois das preferidas de cada estado.
REDE = ["Sigh", "Normal"]


def dex_count():
    txt = io.open(os.path.join(RAIZ, "dex.h"), encoding="utf-8",
                  errors="replace").read()
    m = re.search(r"#define\s+DEX_COUNT\s+(\d+)", txt)
    if not m:
        sys.exit("nao acho DEX_COUNT em dex.h")
    return int(m.group(1))


def _mesmo_png(a, b):
    try:
        return ImageChops.difference(Image.open(a).convert('RGB'),
                                     Image.open(b).convert('RGB')).getbbox() is None
    except Exception:                                 # noqa: BLE001
        return False


def pastas_da_especie(carpeta):
    """(pastas para o NORMAL, pastas para o SHINY), varrendo a arvore
    INTEIRA. Cada pasta e julgada pelo seu Normal.png: igual ao da raiz
    serve ao normal, igual ao de 0000/0001 serve ao shiny, o resto
    (formas, fantasias) nao serve."""
    raiz_n = os.path.join(carpeta, 'Normal.png')
    sh0 = os.path.join(carpeta, '0000', '0001')
    sh_n = os.path.join(sh0, 'Normal.png')
    normal = [carpeta]
    shiny = [sh0] if os.path.exists(sh_n) else []
    for dp, _dd, ff in sorted(os.walk(carpeta)):
        if dp in (carpeta, sh0) or 'Normal.png' not in ff:
            continue
        n = os.path.join(dp, 'Normal.png')
        if os.path.exists(raiz_n) and _mesmo_png(n, raiz_n):
            normal.append(dp)
        elif shiny and _mesmo_png(n, sh_n):
            shiny.append(dp)
    return normal, shiny


def pasta_shiny(carpeta):
    """A pasta do shiny da forma base: <dex>/0000/0001."""
    p = os.path.join(carpeta, '0000', '0001')
    return p if os.path.isdir(p) else None


def pastas_de(carpeta):
    """A raiz da especie mais as subpastas que sao O MESMO desenho.

    O SpriteCollab guarda as formas em subpastas numeradas, e as vezes
    uma delas tem muito mais emocoes que a raiz (o Gengar tem 6 na raiz
    e 20 na 0003). Mas a maioria das subpastas e OUTRO desenho ——
    shiny, forma regional, fantasia ——, e tomar emprestada a cara de la
    poria um bicho de outra aparencia na reacao sem que nada avisasse.

    O teste e exato: so entra a subpasta cujo Normal.png seja igual
    pixel a pixel ao da raiz. Medido nas 400 primeiras especies, isso
    separa 24 subpastas aproveitaveis de 86 que nao sao.
    """
    out = [carpeta]
    raiz = os.path.join(carpeta, "Normal.png")
    if not os.path.exists(raiz):
        return out
    try:
        base = Image.open(raiz).convert("RGB")
    except Exception:                                 # noqa: BLE001
        return out
    for sub in sorted(os.listdir(carpeta)):
        p = os.path.join(carpeta, sub)
        if not os.path.isdir(p):
            continue
        n = os.path.join(p, "Normal.png")
        if not os.path.exists(n):
            continue
        try:
            if ImageChops.difference(base, Image.open(n).convert("RGB")).getbbox() is None:
                out.append(p)
        except Exception:                             # noqa: BLE001
            pass
    return out


def resolve(pastas, prefs):
    """O primeiro arquivo que existir, entre as preferidas e a rede.

    Procura cada emocao em TODAS as pastas boas antes de passar para a
    proxima preferencia: vale mais o `Determined` de uma subpasta com o
    mesmo desenho do que o `Sigh` da raiz.
    """
    for emo in list(prefs) + REDE:
        for carpeta in pastas:
            p = os.path.join(carpeta, emo + ".png")
            if os.path.exists(p):
                return p, emo
    return None, None


def pedazo(ruta):
    """(w, h, paleta, rle) de um retrato."""
    im = Image.open(ruta).convert("RGBA")
    try:
        w, h, pal, idx = gen_art.indexa(im)
    except SystemExit:
        im = gen_art.reduce_colores(im, 60)
        w, h, pal, idx = gen_art.indexa(im)
    if len(pal) > 254:
        im = gen_art.reduce_colores(im, 60)
        w, h, pal, idx = gen_art.indexa(im)
    return w, h, pal, gen_art.comprimir(idx)


def main():
    if len(sys.argv) < 2:
        sys.exit("uso: python tools/gen_retratos.py <carpeta do repo>")
    base = os.path.join(sys.argv[1], "portrait")
    if not os.path.isdir(base):
        sys.exit("nao vejo " + base)

    n = dex_count()
    ranuras = len(ESTADOS)
    cuerpos = {}
    usos = collections.Counter()
    sin = []
    firmas = {}
    caros = []

    for d in range(1, n + 1):
        carpeta = os.path.join(base, "%04d" % d)
        if not os.path.isdir(carpeta):
            sin.append(d)
            continue
        # resolve cada estado e deduplica por ARQUIVO: a maioria das
        # especies so tem `Normal`, entao os seis estados viram um so.
        # NAO chamar de `base`: esse nome ja e a raiz dos retratos la
        # em cima, e sombrea-lo rebentava o join da volta seguinte.
        pastasBase, pastasSh = pastas_da_especie(carpeta)
        # o shiny usa as pastas DELE, achadas na arvore inteira;
        # nunca cai nas do normal, que poriam a cor errada
        variantes = [("", pastasBase)]
        if pastasSh:
            variantes.append(("sh", pastasSh))

        for sufixo, pastas in variantes:
            rutas, mapa = [], []
            for _nome, prefs in ESTADOS:
                r, emo = resolve(pastas, prefs)
                if r is None:
                    mapa.append(0xFF)
                    continue
                if not sufixo:
                    usos[emo] += 1
                if r not in rutas:
                    rutas.append(r)
                mapa.append(rutas.index(r))
            if not rutas:
                if not sufixo:
                    sin.append(d)
                continue
            # quem nao resolveu aponta pro Normal, que e o indice 0 sempre
            mapa = [0 if m == 0xFF else m for m in mapa]

            piezas = [pedazo(r) for r in rutas]
            cab = bytearray()
            cab.append(len(piezas))
            cab += bytes(mapa)
            if len(cab) % 2:
                cab.append(0)
            pals, rles = bytearray(), bytearray()
            for (w, h, pal, rle) in piezas:
                cab += struct.pack("<BBBBHH", w, h, len(pal), 0, len(rle), 0)
                for c in pal:
                    pals += struct.pack("<H", c)
                rles += bytes(rle)
            cuerpo = bytes(cab) + bytes(pals) + bytes(rles)
            if len(cuerpo) > 24000:
                caros.append((len(cuerpo), d))
            cuerpos[(d, 1 if sufixo else 0)] = cuerpo
            if not sufixo:
                firmas[d] = (len(piezas), len(piezas[mapa[0]][3]))

    # DOIS deslocamentos por especie: normal e shiny. Indice d*2 + shiny.
    # Assim a placa le so o que vai usar.
    cabecera = 8 + 4 * (n + 1) * 2
    offs = [0] * ((n + 1) * 2)
    blob = bytearray()
    cur = cabecera
    for clave in sorted(cuerpos):
        d, sh = clave
        offs[d * 2 + sh] = cur
        blob += cuerpos[clave]
        cur += len(cuerpos[clave])

    out = bytearray(b"TPPR")
    # O 2 e a VERSAO do formato. Um cartao da versao 1 tem de ser
    # recusado, nao lido como se fosse desta: la a tabela tinha metade
    # das entradas e cada bicho leria o deslocamento do vizinho.
    out += struct.pack("<HBB", n, ranuras, 2)
    for o in offs:
        out += struct.pack("<I", o)
    out += blob

    os.makedirs(SALIDA, exist_ok=True)
    with open(os.path.join(SALIDA, "por.bin"), "wb") as f:
        f.write(out)

    # e o cabecalho com a assinatura que o firmware confere
    L = []
    L.append("// GERADO por tools/gen_retratos.py - nao editar a mao.")
    L.append("//")
    L.append("// Os retratos vivem no cartao (/por/por.bin): sao 40x40 por")
    L.append("// estado e por especie, o que nao cabe em flash. Aqui fica so")
    L.append("// a ASSINATURA, que o firmware exige que bata antes de ler ——")
    L.append("// um arquivo indexado por especie mostra o vizinho em silencio")
    L.append("// quando a tabela cresce, que foi o que aconteceu com os sons.")
    L.append("")
    L.append("#define POR_ESPECIES %d" % n)
    L.append("#define POR_RANURAS %d" % ranuras)
    L.append("#define POR_VERSION 2")
    L.append("")
    # A IMPRESSAO DIGITAL. Nao e a contagem do cabecalho (essa ja esta
    # em POR_ESPECIES): e um dado POR LINHA, que e o unico jeito de
    # provar que a linha lida e a da especie pedida. Com so a contagem,
    # um indice deslocado em uma casa passa despercebido.
    espalhadas = [d for d in sorted(firmas)][::max(1, len(firmas) // 12)][:12]
    L.append("// Impressao digital de %d especies, escrita por quem GEROU o" % len(espalhadas))
    L.append("// arquivo. O teste cobra do leitor que batam: e o que pega um")
    L.append("// indice deslocado, que a contagem sozinha nao pega.")
    L.append("#define POR_MUESTRAS %d" % len(espalhadas))
    L.append("static const struct { uint16_t dex; uint8_t caras; uint16_t rle; }")
    L.append("    POR_MUESTRA[POR_MUESTRAS] = {")
    for d in espalhadas:
        L.append("  { %d, %d, %d }," % (d, firmas[d][0], firmas[d][1]))
    L.append("};")
    L.append("")
    L.append("enum {")
    for i, (nome, _p) in enumerate(ESTADOS):
        L.append("  %s = %d," % (nome, i))
    L.append("};")
    L.append("")
    io.open(os.path.join(RAIZ, "retratos.h"), "w", encoding="utf-8",
            newline="\n").write("\n".join(L))

    nsh = sum(1 for k in cuerpos if k[1])
    print("/por/por.bin: %.1f MB, %d normais + %d shiny"
          % (len(out) / 1048576.0, len(cuerpos) - nsh, nsh))
    print("sem retrato: %d%s" % (len(sin), ("  " + str(sin[:8])) if sin else ""))
    print("retratos.h: POR_ESPECIES=%d POR_RANURAS=%d" % (n, ranuras))
    print()
    print("de onde saiu a cara de cada estado:")
    for emo, c in usos.most_common():
        print("  %-12s %5d" % (emo, c))
    if caros:
        caros.sort(reverse=True)
        print()
        print("os mais pesados: %s" % caros[:5])


if __name__ == "__main__":
    main()
