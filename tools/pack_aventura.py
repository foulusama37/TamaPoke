#!/usr/bin/env python3
"""Os sprites PMD (SpriteCollab) da AVENTURA -> tools/sdcard/av/

    python tools/pack_aventura.py                 # todos
    python tools/pack_aventura.py 59 94 6         # so esses
    python tools/pack_aventura.py --amostra       # mede o peso em 40 especies

Le o clone local do SpriteCollab (REPO abaixo; o mesmo de gen_retratos.py).
NAO mexe em /mons: casa e combate seguem com os battlers do Anil.

Mesmo formato TPK2 de /mons, entao PmdMon le sem mudar nada. So as acoes
mudam de sentido, e todas olham para a DIREITA (fila 2 da folha): quem olha
para a esquerda se desenha espelhado, e o cartao leva metade. Da 12 em diante
vai tudo o mais que a especie tiver (menos a lista FORA), e a cena sorteia.

    PMD_IDLE   Idle       PMD_HURT    Hurt
    PMD_WALKR  Walk       PMD_ATTACK  Attack
    PMD_SLEEP  Sleep      PMD_HOP     Hop
    PMD_POSE   Pose ou Charge
    PMD_BREATH SpAttack ou Shoot
    PMD_SIT    Rotate     (a danca)
    PMD_EAT    Eat

Arquivos, sem indice de linha de tabela nenhum (o nome ja e a identidade):
    /av/p<dex 4 digitos>.bin          o normal
    /av/s<dex 4 digitos>.bin          o shiny
    /av/m<dex>_<forma>.bin            a mega: `base` e `forma` do MEGA_TBL
Quem nao tem arquivo cai no sprite padrao do jogo (aventura.ino).
"""
import io
import json
import os
import re
import struct
import sys
import xml.etree.ElementTree as ET

from PIL import Image

AQUI = os.path.dirname(os.path.abspath(__file__))
REPO = r'G:/SpriteCollab/repo'
OUT = os.path.join(AQUI, 'sdcard', 'av')
DIR_DIREITA, DIR_FRENTE = 2, 0
ALFA = 128
MAX_Q = 24        # quadros por acao
MIN_MS = 40

# (id da acao em sdmon.h, nomes na ordem de preferencia, fila da folha)
ACOES = [
    (0, ('Idle',), DIR_DIREITA),
    (2, ('Walk',), DIR_DIREITA),
    (3, ('Sleep', 'EventSleep', 'Laying'), DIR_FRENTE),
    (4, ('Eat',), DIR_DIREITA),
    (5, ('Hurt', 'Cringe'), DIR_DIREITA),
    (6, ('Attack', 'Strike', 'Swing'), DIR_DIREITA),
    (7, ('Pose', 'Charge'), DIR_DIREITA),
    (8, ('Hop',), DIR_DIREITA),
    (10, ('SpAttack', 'Shoot'), DIR_DIREITA),
    (11, ('Rotate', 'Twirl'), DIR_DIREITA),
]


# O que NAO entra no sorteio. Sao de cena do jogo original e fora dela nao fazem
# sentido: `Head` e so a cabeca (saindo de uma parede), `Sink` afunda no chao,
# `Pull` e `HitGround` sao de cutscene, `Ricochet` vira uma bola, `Wiggle` e
# so a cauda. Os `Special` cada autor usou como quis.
FORA = {'Head', 'Sink', 'Pull', 'HitGround', 'Faint', 'Fainted', 'Ricochet',
        'Wiggle', 'Special0', 'Special1', 'Special2', 'Pain', 'Cringe',
        'EventSleep', 'Laying'}
PRIMEIRO_EXTRA = 12    # as acoes fixas vao ate PMD_SIT (11)
MAX_ACOES = 48         # PMD_NACTS em sdmon.h


def rgb565(r, g, b):
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)


def le_anims(pasta):
    raiz = ET.parse(os.path.join(pasta, 'AnimData.xml')).getroot()
    crus = {a.findtext('Name'): a for a in raiz.find('Anims')}
    res = {}
    for nome, a in crus.items():
        alvo = a.findtext('CopyOf')
        b = crus.get(alvo) if alvo else a
        if b is None:
            continue
        arq = os.path.join(pasta, (alvo or nome) + '-Anim.png')
        if os.path.exists(arq):
            res[nome] = (arq, int(b.findtext('FrameWidth')), int(b.findtext('FrameHeight')),
                         [int(d.text) for d in b.find('Durations')])
    return res


def empacota(pasta, saida):
    """Devolve os bytes escritos, ou 0 se a pasta nao serve."""
    if not os.path.exists(os.path.join(pasta, 'AnimData.xml')):
        return 0
    anims = le_anims(pasta)
    if 'Idle' not in anims:
        return 0
    cores, pal, feitos = {}, [], []
    # As fixas primeiro (a cena pede por elas), depois TODAS as outras que ele
    # tiver: quem tem muitas varia mais, quem tem poucas faz as que tem. A
    # mesma folha nao entra duas vezes (CopyOf).
    fila_acoes = []
    folhas = set()
    for aid, nomes, fila in ACOES:
        nome = next((n for n in nomes if n in anims), None)
        if nome is not None:
            fila_acoes.append((aid, nome, fila))
            folhas.add(anims[nome][0])
    prox = PRIMEIRO_EXTRA
    for nome in sorted(anims):
        if nome in FORA or nome == 'Walk' or anims[nome][0] in folhas or prox >= MAX_ACOES:
            continue
        folhas.add(anims[nome][0])
        fila_acoes.append((prox, nome, DIR_DIREITA))
        prox += 1
    for aid, nome, fila in fila_acoes:
        arq, fw, fh, durs = anims[nome]
        if fw > 255 or fh > 255:
            continue
        im = Image.open(arq).convert('RGBA')
        linhas = im.height // fh
        r = fila if linhas > fila else 0
        nq = min(len(durs), im.width // fw, MAX_Q)
        # So a parte comum com tinta: as celulas do PMD sao folgadas, e cortar
        # as colunas vazias reduz muito o arquivo sem mudar o desenho.
        faixa = im.crop((0, r * fh, nq * fw, (r + 1) * fh))
        caixa = None
        for i in range(nq):
            bb = faixa.crop((i * fw, 0, (i + 1) * fw, fh)).getchannel('A').point(
                lambda a: 255 if a >= ALFA else 0).getbbox()
            if bb:
                caixa = bb if caixa is None else (min(caixa[0], bb[0]), min(caixa[1], bb[1]),
                                                  max(caixa[2], bb[2]), max(caixa[3], bb[3]))
        if caixa is None:
            continue
        # horizontal simetrico em volta do centro, para o espelho nao deslocar
        meio = fw // 2
        raio = max(meio - caixa[0], caixa[2] - meio)
        x0, x1 = max(0, meio - raio), min(fw, meio + raio)
        y0, y1 = caixa[1], caixa[3]
        w, h = x1 - x0, y1 - y0
        dados = bytearray()
        for i in range(nq):
            q = faixa.crop((i * fw + x0, y0, i * fw + x1, y1))
            for px in q.getdata():
                if px[3] < ALFA:
                    dados.append(0xFF)
                    continue
                k = px[:3]
                if k not in cores:
                    if len(pal) >= 255:
                        perto = min(pal, key=lambda c: sum((a - b) ** 2 for a, b in zip(c, k)))
                        cores[k] = cores[perto]
                    else:
                        cores[k] = len(pal)
                        pal.append(k)
                dados.append(cores[k])
        ms = [max(MIN_MS, round(d * 1000 / 60)) for d in durs[:nq]]
        feitos.append((aid, w, h, nq, ms, bytes(dados)))
    if not any(f[0] == 0 for f in feitos):
        return 0
    b = io.BytesIO()
    b.write(b'TPK2')
    b.write(struct.pack('<BH', len(feitos), len(pal)))
    for c in pal:
        b.write(struct.pack('<H', rgb565(*c)))
    for aid, w, h, nq, ms, dados in feitos:
        b.write(struct.pack('<4B', aid, w, h, nq))
        b.write(struct.pack('<%dH' % nq, *ms))
        b.write(dados)
    os.makedirs(OUT, exist_ok=True)
    with open(saida, 'wb') as f:
        f.write(b.getvalue())
    return len(b.getvalue())


def megas():
    """(base, forma, sufixo do nome) do MEGA_TBL, lido de megas.h."""
    s = io.open(os.path.join(AQUI, '..', 'megas.h'), encoding='utf-8', errors='replace').read()
    tb = s[s.index('MEGA_TBL['):s.index('};', s.index('MEGA_TBL['))]
    res = []
    for m in re.finditer(r'^\s*\{\s*(\d+),\s*\w+\s*,\s*(\d+),.*?"([^"]+)"', tb, re.M):
        nome = m.group(3).split()
        res.append((int(m.group(1)), int(m.group(2)), nome[-1] if len(nome) > 1 and len(nome[-1]) == 1 else ''))
    return res


def main():
    args = sys.argv[1:]
    amostra = '--amostra' in args
    pedidos = [int(a) for a in args if a.isdigit()]
    so_megas = 'megas' in args
    faixa = [] if so_megas else (pedidos or (list(range(1, 1026, 25)) if amostra else list(range(1, 1026))))
    tot = n = sh = 0
    sem = []
    maior = (0, '')
    for d in faixa:
        pasta = os.path.join(REPO, 'sprite', '%04d' % d)
        b = empacota(pasta, os.path.join(OUT, 'p%04d.bin' % d))
        if not b:
            sem.append(d)
            continue
        tot += b
        n += 1
        if b > maior[0]:
            maior = (b, 'p%04d' % d)
        b2 = empacota(os.path.join(pasta, '0000', '0001'), os.path.join(OUT, 's%04d.bin' % d))
        if b2:
            tot += b2
            sh += 1
    print('%d especies, %d shiny, %.1f MB (maior %s, %d KB); media %d KB' % (
        n, sh, tot / 1e6, maior[1], maior[0] // 1024, tot // max(n, 1) // 1024))
    print('sem sprite: %d %s' % (len(sem), sem[:50]))
    if amostra:
        return
    arvore = json.load(io.open(os.path.join(REPO, 'tracker.json'), encoding='utf-8'))
    feitas, faltam, peso = 0, [], 0
    for base, forma, suf in megas():
        if pedidos and base not in pedidos:
            continue
        sub = arvore.get('%04d' % base, {}).get('subgroups', {})
        quer = 'mega_' + suf.lower() if suf else 'mega'
        k = next((k for k, v in sub.items() if (v.get('name') or '').lower() == quer), None)
        b = 0
        if k:
            b = empacota(os.path.join(REPO, 'sprite', '%04d' % base, k),
                         os.path.join(OUT, 'm%d_%d.bin' % (base, forma)))
        if b:
            feitas += 1
            peso += b
        else:
            faltam.append('%d/%d' % (base, forma))
    print('megas com sprite: %d (%.1f MB); sem: %d' % (feitas, peso / 1e6, len(faltam)))


if __name__ == '__main__':
    main()
