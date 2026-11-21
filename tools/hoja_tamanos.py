#!/usr/bin/env python3
"""Hojas de contacto con el TAMANO REAL de cada bicho en pantalla.

Para que? Porque comprobar 1025 especies una a una dentro del juego no es
comprobar nada: "vai ser dificil eu conferir um a um dentro do jogo".

Esto NO es un dibujo aproximado. Repite la cuenta del firmware
(escalaDibujo / factorTinta en TamaPoke.ino, y BT_BACK_K / BT_FOE_K en
battle_ui.ino) sobre el MISMO fichero TPK2 que lee la placa, y pega el
sprite con su misma ancla (los pies en la linea de suelo). El modo
`--validar` lo comprueba contra capturas de verdad del simulador: una hoja
que mienta es peor que no tenerla.

    python3 tools/hoja_tamanos.py --validar      # contrasta con el simulador
    python3 tools/hoja_tamanos.py                # genera las hojas
    python3 tools/hoja_tamanos.py --orden tamano # ordenadas por tamano
"""
import argparse
import io
import math
import os
import re
import struct
import sys

from PIL import Image, ImageDraw

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import tinta

AQUI = os.path.dirname(os.path.abspath(__file__))
RAIZ = os.path.dirname(AQUI)
MONS = os.path.join(AQUI, 'sdcard', 'mons')
SALIDA = r'C:\Users\foulu\Downloads\tamapoke_tamanos'

PMD_IDLE, PMD_SLEEP = 0, 3
HUECO = 0xFF

# Los factores del firmware. Si alguien los toca alli y no aqui, la hoja
# dibuja una cosa y la placa otra: por eso se comprueban contra el fuente.
K_CASA, K_ESPALDAS, K_FRENTE = 1.00, 1.05, 0.72
# el hueco de alto de cada vista, igual que en el firmware
ALTO_CASA, ALTO_ESPALDAS, ALTO_FRENTE = 190, 215, 137
# Lineas de suelo, de TamaPoke.ino (PET_GROUND) y battle_ui.ino.
SUELO_CASA, SUELO_TUYO, SUELO_RIVAL = 304, 312, 178


# --------------------------------------------------------------- TPK2
def lee_tpk2(ruta):
    """{id: (w, h, pixeles_frame0, base, top)} con la paleta ya en RGB."""
    if not os.path.exists(ruta):
        return None, None
    d = open(ruta, 'rb').read()
    if len(d) < 7 or d[:4] != b'TPK2':
        return None, None
    nActs = d[4]
    palCount = struct.unpack('<H', d[5:7])[0]
    pal = []
    for i in range(palCount):
        v = struct.unpack('<H', d[7 + i * 2:9 + i * 2])[0]
        r = (v >> 11) & 0x1F
        g = (v >> 5) & 0x3F
        b = v & 0x1F
        pal.append(((r * 255 + 15) // 31, (g * 255 + 31) // 63,
                    (b * 255 + 15) // 31))
    p = 7 + palCount * 2
    acts = {}
    for _ in range(nActs):
        if p + 4 > len(d):
            break
        idd, w, h, nf = d[p], d[p + 1], d[p + 2], d[p + 3]
        p += 4 + nf * 2
        blob = d[p:p + w * h * nf]
        p += w * h * nf
        if len(blob) < w * h:
            continue
        # base y top: las mismas filas que calcula sdmon.cpp, sobre TODOS
        # los frames (si no, el bicho cambiaria de tamano al animarse)
        base, top = 1, h
        for f in range(nf):
            fr = blob[f * w * h:(f + 1) * w * h]
            if len(fr) < w * h:
                break
            for r in range(h - 1, -1, -1):
                if any(b != HUECO for b in fr[r * w:(r + 1) * w]):
                    base = max(base, r + 1)
                    break
            for r in range(h):
                if any(b != HUECO for b in fr[r * w:(r + 1) * w]):
                    top = min(top, r)
                    break
        acts[idd] = (w, h, blob[:w * h], base, top if top < h else 0)
    return acts, pal


def ink_h(a):
    _w, h, _fr, base, top = a
    return (base - top) if base > top else (h or 1)


# ------------------------------------------------- la cuenta del firmware
def escala(a, quiere, maxS=5, alto_max=0):
    """(s, k) delegando en tinta.escala_firmware().

    Antes esta cuenta estaba copiada aqui y en audita_tamanos.py. Al
    cambiar el esquema de normalizar el ALTO a normalizar una mezcla con
    el AREA, las dos copias se quedaron viejas y los dos verificadores
    pasaron a dar verde sobre otra cosa. Una copia, un sitio.
    """
    _w, h, _fr, _base, _top = a
    s, k, _topado = tinta.escala_firmware(
        mag_de(a), ink_h(a), h, quiere, alto_max, maxS)
    return s, k


def area_de(a):
    """Pixeles con tinta del frame que se dibuja."""
    w, h, fr, _base, _top = a
    return sum(1 for b in fr if b != HUECO) or 1


def mag_de(a):
    return tinta.magnitud(ink_h(a), area_de(a))


def pinta(dst, a, pal, cx, suelo, quiere, alto_max=0):
    """Pega el sprite escalado, anclado por los pies. Devuelve el LADO
    equivalente de lo dibujado (la raiz del area), que es lo que el ojo
    lee —— y no el alto, que fue el error que costo toda esta serie."""
    w, h, fr, base, _top = a
    s, k = escala(a, quiere, 5, alto_max)
    px = s * k
    fullW = max(1, int(w * px + 0.5))
    fullH = max(1, int(h * px + 0.5))
    img = Image.new('RGBA', (w, h), (0, 0, 0, 0))
    po = img.load()
    for y in range(h):
        for x in range(w):
            i = fr[y * w + x]
            if i == HUECO or i >= len(pal):
                continue
            c = pal[i]
            po[x, y] = (c[0], c[1], c[2], 255)
    img = img.resize((fullW, fullH), Image.NEAREST)
    x0 = int(cx - fullW / 2)
    y0 = int(suelo - base * px + 0.5)
    dst.alpha_composite(img, (x0, y0))
    return math.sqrt(area_de(a)) * px


# ------------------------------------------------------------- la dex
def lee_dex():
    s = open(os.path.join(RAIZ, 'dex.h'), encoding='utf-8',
             errors='replace').read()
    i = s.index('DEX_TBL[DEX_COUNT + 1]')
    filas = re.findall(
        r'^  \{ "([^"]*)", (\d+), \d+, \w+, 0x[0-9A-Fa-f]+, '
        r'\d+, \d+, \d+, \d+, \d+, \d+, \d+, (\d+),', s[i:], re.M)
    return [(n, int(e), int(sh)) for n, e, sh in filas]


def lee_tabla(fichero, marca):
    """[(nombre, screenSz)] de megas.h o forms.h, en orden de fila —— que
    es como se llaman sus .bin (mNNN / vNNN)."""
    ruta = os.path.join(RAIZ, fichero)
    if not os.path.exists(ruta):
        return []
    s_ = open(ruta, encoding='utf-8', errors='replace').read()
    i = s_.index(marca)
    # la fila acaba en `..., <screenSz>, "<nombre>" },`
    return [(nom, int(sz)) for sz, nom in
            re.findall(r',\s*(\d+), "([^"]{1,14})" \},', s_[i:])]


def comprueba_constantes():
    """Que los factores de aqui sigan siendo los del firmware."""
    mal = []
    bu = open(os.path.join(RAIZ, 'battle_ui.ino'), encoding='utf-8',
              errors='replace').read()
    ino = open(os.path.join(RAIZ, 'TamaPoke.ino'), encoding='utf-8',
               errors='replace').read()
    for src, nombre, valor in ((bu, 'BT_BACK_K', K_ESPALDAS),
                               (bu, 'BT_FOE_K', K_FRENTE),
                               (ino, 'HOME_K', K_CASA)):
        m = re.search(r'#define\s+%s\s+([\d.]+)f' % nombre, src)
        if not m:
            mal.append('%s no esta en el fuente' % nombre)
        elif abs(float(m.group(1)) - valor) > 1e-6:
            mal.append('%s: firmware %s, hoja %s'
                       % (nombre, m.group(1), valor))
    return mal


# ------------------------------------------------------------- celdas
CELDA_W, CELDA_H = 232, 250
FONDO_CASA = (96, 150, 86)
FONDO_COMB = (74, 120, 150)


def celda_casa(a, pal, dex, nombre, screenSz):
    im = Image.new('RGBA', (CELDA_W, CELDA_H), FONDO_CASA + (255,))
    d = ImageDraw.Draw(im)
    # el suelo, a escala: la celda es la mitad de la pantalla de 466
    suelo = int(SUELO_CASA * CELDA_H / 466.0) + 48
    d.line([(8, suelo), (CELDA_W - 8, suelo)], fill=(60, 100, 56), width=2)
    lado = pinta(im, a, pal, CELDA_W // 2, suelo,
                 screenSz * K_CASA, ALTO_CASA)
    d.text((5, 3), '%d %s' % (dex, nombre), fill=(255, 255, 255))
    d.text((5, CELDA_H - 14), '%d px de lado' % round(lado),
           fill=(230, 230, 180))
    return im


def celda_combate(ai, pal_i, asl, pal_s, dex, nombre, screenSz):
    """El rival de frente y el mismo bicho de espaldas, como en combate."""
    im = Image.new('RGBA', (CELDA_W, CELDA_H), FONDO_COMB + (255,))
    d = ImageDraw.Draw(im)
    esc = CELDA_H / 466.0
    sr = int(SUELO_RIVAL * esc) + 30
    st = int(SUELO_TUYO * esc) + 30
    d.line([(CELDA_W // 2, sr), (CELDA_W - 6, sr)], fill=(56, 92, 116), width=2)
    d.line([(6, st), (CELDA_W // 2, st)], fill=(56, 92, 116), width=2)
    hr = pinta(im, ai, pal_i, int(CELDA_W * 0.70), sr,
               screenSz * K_FRENTE, ALTO_FRENTE)
    ht = pinta(im, asl, pal_s, int(CELDA_W * 0.30), st,
               screenSz * K_ESPALDAS, ALTO_ESPALDAS)
    d.text((5, 3), '%d %s' % (dex, nombre), fill=(255, 255, 255))
    d.text((5, CELDA_H - 14), 'tuyo %d  rival %d px' % (round(ht), round(hr)),
           fill=(220, 230, 255))
    return im


def hoja(celdas, cols, titulo):
    filas = (len(celdas) + cols - 1) // cols
    im = Image.new('RGB', (cols * CELDA_W, filas * CELDA_H + 24), (24, 24, 28))
    d = ImageDraw.Draw(im)
    d.text((8, 7), titulo, fill=(240, 240, 240))
    for i, c in enumerate(celdas):
        im.paste(c.convert('RGB'),
                 ((i % cols) * CELDA_W, 24 + (i // cols) * CELDA_H))
    return im


# ----------------------------------------------------------- validacion
def validar(log):
    """Compara la hoja con lo que el FIRMWARE dice que pinta.

    El log sale de correr el simulador con un guion que alterna
    `SPEC <n>` y `TALLA` (tools/ no lo genera: lo hace quien valida).

    La primera version de esto no preguntaba al firmware: restaba el
    fondo de una captura. Daba 194 px para TODAS las especies —— el
    alto exacto de la banda entera —— porque el escenario es un
    paisaje y no un color liso, asi que la resta marcaba todas las
    filas. Un validador que mide otra cosa da por bueno cualquier
    cosa, y en este proyecto ya ha pasado dos veces.
    """
    if not os.path.exists(log):
        print("no existe %s" % log)
        print("hazlo asi, desde desktop/:")
        print("  un guion con `serial SPEC <n>` + `serial TALLA` por")
        print("  especie, y guarda la salida en un fichero")
        return 1
    dex = lee_dex()
    nombre = None
    filas = []
    for linea in io.open(log, encoding="utf-8", errors="replace"):
        m = re.match(r"especie #(\d+) (.+)", linea.strip())
        if m:
            nombre = int(m.group(1))
            continue
        m = re.match(r"pintado=(\d+) pedido=(\d+)", linea.strip())
        if m and nombre:
            filas.append((nombre, int(m.group(1)), int(m.group(2))))
            nombre = None
    if not filas:
        print("el log no trae ninguna pareja SPEC/TALLA")
        return 1

    print("%-13s %6s %7s %7s   %s"
          % ("especie", "hoja", "pedido", "pintado", "nota"))
    malPedido = malPintado = 0
    for n, pintado, pedido in filas:
        acts, _pal = lee_tpk2(os.path.join(MONS, "p%03d.bin" % n))
        if not acts or PMD_IDLE not in acts:
            continue
        a = acts[PMD_IDLE]
        s_, k = escala(a, dex[n][2] * K_CASA)
        mio = ink_h(a) * s_ * k
        nota = ""
        # 1) la hoja tiene que pedir lo mismo que el firmware
        if abs(mio - pedido) > max(2, pedido * 0.03):
            nota = "LA HOJA NO PIDE LO MISMO"
            malPedido += 1
        # 2) y el firmware tiene que pintar lo que pide. El margen es
        #    del 12% porque el bicho de casa ANDA y SALTA: petFxFor
        #    deforma hasta +-10% y la captura cae donde cae.
        elif abs(pintado - pedido) > max(3, pedido * 0.12):
            nota = "el firmware no pinta lo que pide"
            malPintado += 1
        print("%-13s %6d %7d %7d   %s"
              % (dex[n][0], round(mio), pedido, pintado, nota))

    print("")
    print("%d especies comparadas" % len(filas))
    print("%d donde la hoja no coincide con el firmware" % malPedido)
    print("%d donde el firmware no pinta lo que pide" % malPintado)
    print("OK" if not (malPedido or malPintado) else "HAY DESCUADRES")
    return 1 if (malPedido or malPintado) else 0

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--validar', action='store_true')
    ap.add_argument('--log', default=os.path.join(RAIZ, 'desktop', 'build', 'talla.txt'))
    ap.add_argument('--orden', choices=('dex', 'tamano'), default='dex')
    ap.add_argument('--cols', type=int, default=8)
    ap.add_argument('--que', choices=('base', 'megas', 'formas'),
                    default='base',
                    help='base = las 1025; megas y formas tienen su propia tabla y sus propios .bin')
    ap.add_argument('--por-hoja', type=int, default=96)
    args = ap.parse_args()

    mal = comprueba_constantes()
    if mal:
        print('DESCUADRE CON EL FIRMWARE:')
        for m in mal:
            print('  ' + m)
        return 1

    if args.validar:
        return validar(args.log)

    os.makedirs(SALIDA, exist_ok=True)
    if args.que == 'base':
        dex = lee_dex()
        filas = [(n, dex[n][0], dex[n][2], 'p') for n in range(1, len(dex))]
    elif args.que == 'megas':
        filas = [(i, nom, sz, 'm') for i, (nom, sz)
                 in enumerate(lee_tabla('megas.h', 'MEGA_TBL[MEGA_COUNT]'))]
    else:
        filas = [(i, nom, sz, 'v') for i, (nom, sz)
                 in enumerate(lee_tabla('forms.h', 'FORM_TBL[FORM_COUNT]'))]
    if not filas:
        print('no hay filas que dibujar para "%s"' % args.que)
        return 1
    if args.orden == 'tamano':
        filas.sort(key=lambda f: f[2])

    casa, comb = [], []
    for n, nom, sz, pref in filas:
        acts, pal = lee_tpk2(os.path.join(MONS, '%c%03d.bin' % (pref, n)))
        if not acts or PMD_IDLE not in acts:
            continue
        ai = acts[PMD_IDLE]
        asl = acts.get(PMD_SLEEP, ai)
        casa.append(celda_casa(ai, pal, n, nom, sz))
        comb.append(celda_combate(ai, pal, asl, pal, n, nom, sz))

    hechas = []
    for etiqueta, celdas in (('casa', casa), ('combate', comb)):
        for i in range(0, len(celdas), args.por_hoja):
            trozo = celdas[i:i + args.por_hoja]
            num = i // args.por_hoja + 1
            tot = (len(celdas) + args.por_hoja - 1) // args.por_hoja
            tit = ('%s %s — hoja %d/%d (orden: %s)'
                   % (etiqueta.upper(), args.que, num, tot, args.orden))
            im = hoja(trozo, args.cols, tit)
            sufijo = '' if args.que == 'base' else '_' + args.que
            ruta = os.path.join(
                SALIDA, '%s%s_%s_%02d.png'
                % (etiqueta, sufijo, args.orden, num))
            im.save(ruta)
            hechas.append(ruta)
    print('%d bichos, %d hojas en %s' % (len(casa), len(hechas), SALIDA))
    return 0


if __name__ == '__main__':
    sys.exit(main())
