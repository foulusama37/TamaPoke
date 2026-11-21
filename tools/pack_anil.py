#!/usr/bin/env python3
"""Empaquetador de los battlers de Pokemon Anil al formato TPK2.

Alternativa a pack_pmd.py: en vez de los sprites chibi de PMD SpriteCollab
(que solo llegan hasta la gen 4), usa los battlers animados de Anil, que
cubren 1647 especies y se ven bastante mejor. A cambio solo hay DOS angulos
-frente y espalda- en lugar de las 12 acciones de PMD, asi que:

  Idle   <- Front
  WalkL  <- Front tal cual   (los battlers gen 5 miran a la IZQUIERDA)
  WalkR  <- Front espejado
  Sleep  <- BACK: de espaldas no se le ven los ojos, que es justo el problema
            que tiene fingir que duerme con un sprite que te esta mirando
  Hurt   <- Front

Limites del formato que impone sdmon.cpp y que aqui se respetan:
  - nf > 24 por accion  -> el firmware RECHAZA el fichero entero
  - palCount > 256      -> idem; ademas el indice 0xFF es "transparente",
                           asi que solo hay 255 colores utiles
  - tamano > 3 MB       -> idem
  - w, h son u8         -> maximo 255 px

Dos detalles de alineacion, porque drawPmdActM() hace
`x0 = cx - a.w*s/2` y `y0 = groundY - a.base*s`:
  - el recorte tiene que ser SIMETRICO respecto al centro del lienzo
    original, o el bicho da un salto lateral al cambiar de accion;
  - la altura no importa, porque `base` (la fila mas baja con contenido) la
    recalcula el firmware al cargar.

  python3 tools/pack_anil.py            # los 151, normal + shiny
  python3 tools/pack_anil.py 1 4 7      # dex concretos
  python3 tools/pack_anil.py normal 25  # solo normales
"""
import os
import re
import struct
import sys

import numpy as np
from PIL import Image
from scipy import ndimage

from dex_data import DEX
from dex_range import N_DEX

ANIL = os.environ.get(
    'ANIL_GFX',
    r"C:\Users\foulu\Downloads\Pokemon Anil V4.13\Graphics\Pokemon")
OUT = os.path.join(os.path.dirname(__file__), 'sdcard', 'mons')
CORRIGE = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'correcciones')

PMD_IDLE, PMD_WALKL, PMD_WALKR, PMD_SLEEP, PMD_HURT = 0, 1, 2, 3, 5
# Ya no hay tope de 24: sdmon.cpp apunta las duraciones en el blob en vez de
# copiarlas a un array fijo. El limite que queda es nf <= 255 (el campo del
# fichero es un u8) y los 3 MB por fichero que valida el firmware.
MAX_FRAMES = 255
ALPHA_T = 128

# (id, carpeta, num. de frames, duracion total del bucle en ms, espejar)
# Con el tope fuera se puede subir el Idle, que es lo que se ve el 95% del
# tiempo. El dedupe de reduce_frames() recorta solo si la tira original no da
# para tantos, asi que pedir de mas no infla el fichero sin motivo.
PLAN = [
    (PMD_IDLE,  'Front', 40, 2600, False),
    (PMD_WALKL, 'Front', 14,  900, False),
    (PMD_WALKR, 'Front', 14,  900, True),
    (PMD_SLEEP, 'Back',   8, 3600, False),
    (PMD_HURT,  'Front',  8,  700, False),
]

# El fichero del Anil se busca por el SLUG en mayusculas, no por el nombre
# de pantalla. El de pantalla lleva espacios y guiones ("MIME JR",
# "PORYGON-Z", "MR. MIME", "HO-OH") y el Anil no: MIMEJR.png, PORYGONZ.png.
# Mientras fueron tres excepciones se llevaron a mano aqui, pero con
# Sinnoh aparecieron dos mas y se quedaron SIN SPRITE sin que nada se
# cayera: el empaquetador lo decia al final, entre cientos de lineas. El
# slug ya viene normalizado de dex_data.py, asi que es la clave buena.
#
# El ALIAS queda para lo unico que el slug no puede saber: que el Anil NO
# llama igual a un bicho en las carpetas normales y en las shiny. Los
# Nidoran son NIDORANF/NIDORANM en Front y Back, y NIDORANfE/NIDORANmA en
# las shiny; por eso es una LISTA de candidatos y no un nombre.
ALIAS = {
    29:  ['NIDORANF', 'NIDORANfE'],
    32:  ['NIDORANM', 'NIDORANmA'],
}
NOMBRE = {num: ALIAS.get(num, [slug.upper()]) for num, slug, disp, *_ in DEX}


def etiqueta(dexnum):
    return NOMBRE.get(dexnum, ['?'])[0]


def rgb565(r, g, b):
    return (r >> 3) << 11 | (g >> 2) << 5 | (b >> 3)


# O pedaco mais largo que ainda conta como ponta que vazou (o Koffing tem
# fumaca de 23 px; a menor peca inteira encontrada, a mao do Iron Hands, 29).
VAZA_MAX = 24


def fragmentos(quadro):
    """[(x0, x1, y0, y1, lado)] dos pedacos que vazaram de um quadro RGBA.

    `lado` = +1: colado na borda ESQUERDA, e continuacao do que sai pela
    direita (vai para x + W); -1 o contrario."""
    a = np.asarray(quadro.getchannel("A")) >= ALPHA_T
    H, W = a.shape
    if not a.any():
        return []
    lab, n = ndimage.label(a, structure=np.ones((3, 3), bool))
    if n < 2:
        return []
    tam = ndimage.sum(a, lab, range(1, n + 1))
    maior = int(np.argmax(tam)) + 1
    out = []
    for k, sl in enumerate(ndimage.find_objects(lab), start=1):
        if k == maior:
            continue
        ys, xs = sl
        # E vazamento quando o pedaco CONTINUA a arte da borda oposta:
        # as linhas em que ele encosta na borda sao (quase) as mesmas em
        # que algo encosta do outro lado, como um corte. E e PEQUENO: as
        # maos do Iron Hands e as asas do Iron Moth tambem encostam nas
        # duas bordas, mas sao pecas inteiras (29 a 51 px), nao pontas.
        if xs.stop - xs.start > VAZA_MAX:
            continue
        for borda, oposta, lado in ((0, W - 1, +1), (W - 1, 0, -1)):
            if not (xs.start == 0 if borda == 0 else xs.stop == W):
                continue
            meu = lab[:, borda] == k
            outro = a[:, oposta]
            outro = outro | np.roll(outro, 1) | np.roll(outro, -1)
            if meu.sum() and (meu & outro).sum() >= 0.5 * meu.sum():
                out.append((xs.start, xs.stop, ys.start, ys.stop, lado, k))
                break
    return out



def desenrola(frames):
    """Devolve cada ponta que vazou para o lado de onde saiu.

    Algumas tiras do Anil tem a arte mais larga que a celula: o que passa
    da borda direita reaparece colado na esquerda do mesmo quadro (o
    Shiftry de costas, com a ponta da folha sozinha do outro lado do
    corpo). Aqui a ponta volta para depois da borda certa. Para isso a
    tira inteira ganha a MESMA margem dos dois lados: todos os quadros
    continuam do mesmo tamanho e o centro nao sai do lugar, que e do que
    caja_simetrica() depende.
    """
    achados = [fragmentos(f) for f in frames]
    pad = max((x1 - x0 for fs in achados for x0, x1, *_ in fs), default=0)
    if not pad:
        return frames
    W, H = frames[0].size
    out = []
    for f, fs in zip(frames, achados):
        arr = np.zeros((H, W + 2 * pad, 4), np.uint8)
        src = np.asarray(f).copy()
        if fs:
            a = np.asarray(f.getchannel('A')) >= ALPHA_T
            lab, _n = ndimage.label(a, structure=np.ones((3, 3), bool))
            for _x0, _x1, _y0, _y1, lado, k in fs:
                ys, xs = np.nonzero(lab == k)
                # tudo o que for desse pedaco, inclusive a borda meio
                # transparente que o limiar deixou de fora
                arr[ys, xs + pad + lado * W] = src[ys, xs]
                src[ys, xs] = 0
        arr[:, pad:pad + W] = np.where(arr[:, pad:pad + W, 3:4] > 0,
                                       arr[:, pad:pad + W], src)
        out.append(Image.fromarray(arr, 'RGBA'))
    return out


def tira(carpeta, candidatos, shiny, desenrolar=True):
    sub = carpeta + (' shiny' if shiny else '')
    path = None
    for nombre in candidatos:
        # correcciones/ gana al Anil: dibujos que el propio Anil trae mal
        # (el EISCUE_1 shiny venia con el cubo de hielo; leva 70).
        for raiz in (CORRIGE, ANIL):
            p = os.path.join(raiz, sub, nombre + '.png')
            if os.path.exists(p):
                path = p
                break
        if path:
            break
    if path is None:
        return None
    im = Image.open(path).convert('RGBA')
    fw = im.height                      # las tiras son cuadradas: N frames de HxH
    n = im.width // fw
    if n < 1:
        return None
    fr = [im.crop((i * fw, 0, (i + 1) * fw, im.height)) for i in range(n)]
    return desenrola(fr) if desenrolar else fr


def reduce_frames(frames, objetivo):
    """Agrupa frames consecutivos IGUALES (las tiras repiten mucho) y luego
    funde los grupos mas cortos hasta bajar de `objetivo`. Devuelve
    [(imagen, peso)], donde el peso es cuantos frames originales representa."""
    grupos = []
    for f in frames:
        clave = f.tobytes()
        if grupos and grupos[-1][0] == clave:
            grupos[-1][2] += 1
        else:
            grupos.append([clave, f, 1])
    while len(grupos) > objetivo:
        i = min(range(len(grupos)), key=lambda k: grupos[k][2])
        vecino = i + 1 if i == 0 else i - 1
        grupos[vecino][2] += grupos[i][2]
        grupos.pop(i)
    return [(g[1], g[2]) for g in grupos]


def caja_simetrica(imgs):
    """bbox del contenido, expandida para quedar centrada en el lienzo original
    (si no, el bicho se desplaza lateralmente al cambiar de accion)"""
    W = imgs[0].width
    x0, y0, x1, y1 = W, imgs[0].height, -1, -1
    for im in imgs:
        b = im.getchannel('A').point(lambda v: 255 if v >= ALPHA_T else 0).getbbox()
        if not b:
            continue
        x0, y0 = min(x0, b[0]), min(y0, b[1])
        x1, y1 = max(x1, b[2]), max(y1, b[3])
    if x1 < 0:
        return (0, 0, W, imgs[0].height)
    centro = W / 2.0
    media = max(centro - x0, x1 - centro)
    nx0 = max(0, int(centro - media))
    nx1 = min(W, int(centro + media + 0.5))
    return (nx0, y0, nx1, y1)


def pack(dexnum, shiny=False, candidatos=None, salida=None):
    """Empaqueta un bicho. `candidatos`/`salida` solo para las megas.

    Una megaevolucion no tiene numero de dex: es la forma N de una
    especie, su PNG se llama <SLUG>_<N>.png y su .bin sale como mNNN.bin
    (NNN = la fila de MEGA_TBL). Lo demas es identico, asi que en vez de
    un segundo empaquetador se le pasan el origen y el destino.
    """
    if candidatos is None:
        candidatos = NOMBRE.get(dexnum)
    if not candidatos:
        raise RuntimeError(f'dex {dexnum} no esta en dex_data')

    colmap, pal = {}, []

    def indexa(im):
        out = bytearray()
        for px in im.getdata():
            if px[3] < ALPHA_T:
                out.append(0xFF)
                continue
            k = px[:3]
            if k not in colmap:
                if len(pal) >= 255:
                    k2 = min(colmap, key=lambda c: sum((a - b) ** 2 for a, b in zip(c, k)))
                    colmap[k] = colmap[k2]
                else:
                    colmap[k] = len(pal)
                    pal.append(k)
            out.append(colmap[k])
        return out

    packed = []
    for aid, carpeta, nmax, total_ms, espejo in PLAN:
        nmax = min(nmax, MAX_FRAMES)
        # Desenrolada primeiro; se com as pontas no lugar o quadro passa
        # dos 255 px que o formato aceita (o Corviknight Gigantamax: 248
        # de celula, 282 desenrolado), fica como o Anil o desenhou.
        for desenrolar in (True, False):
            fr = tira(carpeta, candidatos, shiny, desenrolar)
            if not fr:
                break
            grupos = reduce_frames(fr, nmax)
            imgs = [g[0] for g in grupos]
            pesos = [g[1] for g in grupos]
            caja = caja_simetrica(imgs)
            imgs = [im.crop(caja) for im in imgs]
            if imgs[0].width <= 255:
                break
        if not fr:
            continue
        if espejo:
            imgs = [im.transpose(Image.FLIP_LEFT_RIGHT) for im in imgs]
        w, h = imgs[0].size
        if w > 255 or h > 255:
            raise RuntimeError(f'{candidatos[0]} {carpeta}: {w}x{h} no cabe en u8')
        # reparte total_ms segun el peso de cada grupo, con suelo de 40 ms
        suma = sum(pesos)
        ms = [max(40, round(total_ms * p / suma)) for p in pesos]
        data = bytearray()
        for im in imgs:
            data += indexa(im)
        packed.append((aid, w, h, len(imgs), ms, bytes(data)))

    if not any(p[0] == PMD_IDLE for p in packed):
        raise RuntimeError('sin Idle (falta el Front)')

    os.makedirs(OUT, exist_ok=True)
    nombre_bin = salida or f'p{"s" if shiny else ""}{dexnum:03d}.bin'
    path = os.path.join(OUT, nombre_bin)
    with open(path, 'wb') as f:
        f.write(b'TPK2')
        f.write(struct.pack('<BH', len(packed), len(pal)))
        for r, g, b in pal:
            f.write(struct.pack('<H', rgb565(r, g, b)))
        for aid, w, h, nf, ms, data in packed:
            f.write(struct.pack('<4B', aid, w, h, nf))
            f.write(struct.pack(f'<{nf}H', *ms))
            f.write(data)
    kb = os.path.getsize(path) / 1024
    if kb > 3072:
        raise RuntimeError(f'{kb:.0f} KB: pasa del limite de 3 MB de sdmon.cpp')
    acts = ' '.join(f'{a}:{n}f' for a, _, _, n, _, _ in packed)
    print(f"  -> {nombre_bin}  {len(pal)} colores  {kb:.0f} KB  [{acts}]")
    return kb


def filas_de(fichero, tabla):
    """[(fila, base, form)] leyendo una tabla del .h generado.

    Del .h y no del PBS: lo que importa es que el NUMERO DE FILA coincida
    con el que el firmware usa para indexar, y eso solo lo sabe el
    fichero que el firmware compila.
    """
    import io
    raiz = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    h = os.path.join(raiz, fichero)
    if not os.path.exists(h):
        return []
    txt = io.open(h, encoding='utf-8', errors='replace').read()
    if tabla not in txt:
        return []
    cuerpo = txt[txt.index(tabla):]
    out = []
    for i, m in enumerate(re.finditer(
            r'^\s*\{\s*(\d+),\s*(?:\w+\s*,\s*)?(\d+),', cuerpo, re.M)):
        out.append((i, int(m.group(1)), int(m.group(2))))
    return out


def filas_mega():
    return filas_de('megas.h', 'MEGA_TBL[MEGA_COUNT]')


if __name__ == '__main__':
    args = sys.argv[1:]
    # Un argumento que no se reconoce PARA el script. Pedi "--solo-megas"
    # (que no existe; la palabra es "megas"), el script lo tomo por basura,
    # empaqueto las 1025 especies durante veinte minutos y NO toco las
    # megas — que era lo unico que hacia falta. Se quedaron desfasadas una
    # fila y el aparato habria pintado la mega equivocada desde Mewtwo en
    # adelante. Un argumento ignorado en silencio es una orden perdida.
    desconocidos = [a for a in args
                    if a not in ('normal', 'megas', 'hembras', 'combate')
                    and not a.isdigit()]
    if desconocidos:
        raise SystemExit(
            "no se que es " + ", ".join(repr(a) for a in desconocidos) +
            "\nuso: pack_anil.py [normal] [megas] [hembras] [combate] [numero ...]")
    solo_normal = 'normal' in args
    solo_megas = 'megas' in args
    solo_hembras = 'hembras' in args
    solo_combate = 'combate' in args
    nums = ([] if (solo_megas or solo_hembras or solo_combate) else
            ([int(a) for a in args if a.isdigit()] or list(range(1, N_DEX + 1))))
    fallos, total = [], 0.0
    for n in nums:
        for sh in ([False] if solo_normal else [False, True]):
            try:
                print(f"#{n:03d} {etiqueta(n)}{' shiny' if sh else ''}")
                total += pack(n, sh)
            except Exception as e:
                print(f"  FALLO: {e}")
                fallos.append((n, sh, str(e)))

    # Las megas van siempre: son 122 ficheros y el combate los pide por
    # fila de MEGA_TBL. Com shiny desde 2026-11-01.
    if (not nums and not solo_hembras and not solo_combate) or solo_megas:
        # Megas y formas se empaquetan igual: <SLUG>_<forma>.png a un
        # fichero por FILA de su tabla. Si la forma no tiene sprite en
        # alguna carpeta, cae al de la especie — hace falta para el
        # Mewtwo Armadura, que tiene frente pero no espalda en ningun
        # proyecto, y de espaldas es un Mewtwo con placas.
        for etq, pref, fs in (("mega", "m", filas_mega()),
                              ("forma", "v", filas_de("forms.h",
                                                      "FORM_TBL[FORM_COUNT]"))):
            # As megas levam shiny (msNNN.bin): o dono viu o Gengar shiny
            # virar uma Mega Gengar da cor comum.
            for fila, base, form in fs:
                s = NOMBRE.get(base, ["?"])[0]
                cand = [f'{s}_{form}', s]
                for sh in ((False, True) if pref == 'm' else (False,)):
                    nome = f'{pref}{"s" if sh else ""}{fila:03d}.bin'
                    try:
                        print(f"{etq} {fila:03d} {etiqueta(base)} forma {form}"
                              f"{' shiny' if sh else ''}")
                        total += pack(base, sh, cand, nome)
                    except Exception as e:
                        print(f"  FALLO: {e}")
                        fallos.append((nome, sh, str(e)))

    # Las HEMBRAS: 98 especies traen su dibujo aparte en el Anil. Van
    # por NUMERO DE DEX y no por fila de una tabla, porque hay como mucho
    # una por especie. El fichero es h/hsNNN.bin y el aparato solo lo
    # pide si el bit de female.h dice que existe (ver gen_female.py), que
    # es lo que evita 924 fallos de SD por combate.
    # Las formas de COMBATE (Castform al sol, Aegislash Filo...): por dex
    # y forma, /mons/c<dex>_<forma>.bin, nunca por fila de una tabla. Sin
    # caer a la especie: un sprite que falta tiene que verse como fallo,
    # no como un Castform normal pintado encima del de sol.
    if (not nums and not solo_hembras and not solo_megas) or solo_combate:
        from gen_formas_combate import FORMAS, fichero
        num = {re.sub(r'[^A-Z0-9]', '', v[0].upper()): k for k, v in NOMBRE.items()}
        for esp, form in FORMAS:
            base = num[re.sub(r'[^A-Z0-9]', '', esp)]
            slug = NOMBRE.get(base, ["?"])[0]
            for sh in (False, True):
                nome = fichero(base, form, sh)
                try:
                    print(f"combate {etiqueta(base)} forma {form}{' shiny' if sh else ''}")
                    total += pack(base, sh, [f'{slug}_{form}'], nome)
                except Exception as e:
                    print(f"  FALLO: {e}")
                    fallos.append((nome, sh, str(e)))

    if (not nums and not solo_combate) or solo_hembras:
        fem = open(os.path.join(os.path.dirname(__file__), '..', 'female.h'),
                   encoding='utf-8').read()
        bits = [int(x, 16) for x in re.findall(r'0x([0-9A-F]{2})', fem)]
        for d in range(1, len(bits) * 8):
            if not (bits[d >> 3] >> (d & 7)) & 1:
                continue
            slug = NOMBRE.get(d, ["?"])[0]
            for sh in (False, True):
                pref = 'hs' if sh else 'h'
                try:
                    print(f"hembra {etiqueta(d)}{' shiny' if sh else ''}")
                    total += pack(d, sh, [slug + '_female'],
                                  f'{pref}{d:03d}.bin')
                except Exception as e:
                    print(f"  FALLO: {e}")
                    fallos.append((f'{pref}{d}', sh, str(e)))

    print(f"\ntotal escrito: {total/1024:.1f} MB")
    print(f"FALLOS: {fallos}" if fallos else 'TODOS EMPAQUETADOS')

