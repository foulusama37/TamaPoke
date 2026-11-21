#!/usr/bin/env python3
"""Que SITIO ocupa cada bicho, de verdad, en las tres vistas.

Tres, no una: casa, el tuyo de espaldas y el rival de frente. Cada una
tiene su factor y su hueco, y durante mucho tiempo solo se auditaba la de
casa —— que es justo donde NO estaba el fallo que el usuario reportaba.

Y mide AREA, no alto. Medir el alto fue el error que trajo hasta aqui:
Wigglytuff sale 18 px mas alto que su Jigglypuff y ocupa la MITAD de
area, asi que evolucionar parecia encoger y este script decia que todo
iba bien. Lo que el ojo lee es el sitio ocupado.

La cuenta del firmware no se copia aqui: vive en tinta.py y la usan
tambien las hojas de contacto. Copiada en tres sitios se quedo vieja dos
veces.

    python3 tools/audita_tamanos.py          # solo los fallos
    python3 tools/audita_tamanos.py -v       # toda la lista
"""
import math
import os
import re
import struct
import sys

AQUI = os.path.dirname(os.path.abspath(__file__))
RAIZ = os.path.dirname(AQUI)
sys.path.insert(0, AQUI)
import tinta

MONS = os.path.join(AQUI, 'sdcard', 'mons')
PMD_IDLE, PMD_SLEEP = 0, 3

# Los factores y los huecos del firmware. Si alguien los cambia alli y no
# aqui, comprueba_constantes() se pone rojo.
K_CASA, K_ESPALDAS, K_FRENTE = 1.00, 1.05, 0.72
ALTO_CASA, ALTO_ESPALDAS, ALTO_FRENTE = 190, 215, 137

VISTAS = (('casa', K_CASA, ALTO_CASA, PMD_IDLE),
          ('espaldas', K_ESPALDAS, ALTO_ESPALDAS, PMD_SLEEP),
          ('frente', K_FRENTE, ALTO_FRENTE, PMD_IDLE))


def acciones(dex):
    """{id: (lienzo_h, alto_tinta, area_tinta)} de ese .bin."""
    ruta = os.path.join(MONS, 'p%03d.bin' % dex)
    if not os.path.exists(ruta):
        return None
    d = open(ruta, 'rb').read()
    if len(d) < 7 or d[:4] != b'TPK2':
        return None
    nActs = d[4]
    palCount = struct.unpack('<H', d[5:7])[0]
    p = 7 + palCount * 2
    out = {}
    for _ in range(nActs):
        if p + 4 > len(d):
            break
        idd, w, h, nf = d[p], d[p + 1], d[p + 2], d[p + 3]
        p += 4 + nf * 2
        fr0 = d[p:p + w * h]
        if len(fr0) < w * h:
            break
        base, top = 1, h
        for f in range(nf):
            fr = d[p + f * w * h:p + (f + 1) * w * h]
            if len(fr) < w * h:
                break
            for r in range(h - 1, -1, -1):
                if any(b != tinta.HUECO for b in fr[r * w:(r + 1) * w]):
                    base = max(base, r + 1)
                    break
            for r in range(h):
                if any(b != tinta.HUECO for b in fr[r * w:(r + 1) * w]):
                    top = min(top, r)
                    break
        alto = (base - top) if base > top else (h or 1)
        area = sum(1 for b in fr0 if b != tinta.HUECO)
        out[idd] = (h, alto, area or 1)
        p += w * h * nf
    return out


def lee_dex():
    s = open(os.path.join(RAIZ, 'dex.h'), encoding='utf-8',
             errors='replace').read()
    i = s.index('DEX_TBL[DEX_COUNT + 1]')
    filas = re.findall(
        r'^  \{ "([^"]*)", (\d+), \d+, \w+, 0x[0-9A-Fa-f]+, '
        r'\d+, \d+, \d+, \d+, \d+, \d+, \d+, (\d+),', s[i:], re.M)
    return [(nom, int(evo), int(sz)) for nom, evo, sz in filas]


def comprueba_constantes():
    """Que este script, el firmware y el generador usen los MISMOS
    numeros. Un auditor con constantes viejas da por bueno lo que no ha
    mirado, y aqui ya ha pasado."""
    mal = []
    bu = open(os.path.join(RAIZ, 'battle_ui.ino'), encoding='utf-8',
              errors='replace').read()
    ino = open(os.path.join(RAIZ, 'TamaPoke.ino'), encoding='utf-8',
               errors='replace').read()
    sd = open(os.path.join(RAIZ, 'sdmon.h'), encoding='utf-8',
              errors='replace').read()
    for src, nombre, valor in ((bu, 'BT_BACK_K', K_ESPALDAS),
                               (bu, 'BT_FOE_K', K_FRENTE),
                               (ino, 'HOME_K', K_CASA),
                               (sd, 'PMD_B_AREA', tinta.B_AREA)):
        m = re.search(r'#define\s+%s\s+([\d.]+)f?' % nombre, src)
        if not m:
            mal.append('%s no esta en el fuente' % nombre)
        elif abs(float(m.group(1)) - valor) > 1e-6:
            mal.append('%s: firmware %s, auditor %s'
                       % (nombre, m.group(1), valor))
    m = re.search(r'#define\s+HOME_ALTO_MAX\s+(\d+)', ino)
    if not m or int(m.group(1)) != ALTO_CASA:
        mal.append('HOME_ALTO_MAX: firmware %s, auditor %d'
                   % (m.group(1) if m else '?', ALTO_CASA))
    try:
        from dex_sizes import B_AREA as GEN_B
        if abs(GEN_B - tinta.B_AREA) > 1e-6:
            mal.append('B_AREA: gen_sizes %s, auditor %s'
                       % (GEN_B, tinta.B_AREA))
    except Exception:                                   # noqa: BLE001
        mal.append('dex_sizes.py no exporta B_AREA')
    # Y el fallo que costo cinco intentos: el combate tiene que pintar
    # diciendo QUE especie pinta, o el rival se escala con el jugador.
    if not re.search(r'drawPmdActM\(m, act,.*&fx, 0\)', bu):
        mal.append('battle_ui.ino dibuja sin pasar dex=0: el rival vuelve '
                   'a escalarse con el screenSz del jugador')
    return mal


def main():
    verboso = '-v' in sys.argv
    dex = lee_dex()
    sitio = {}        # dex -> {vista: area dibujada}
    altos = {}
    topados, desbordan = [], []

    for n in range(1, len(dex)):
        acts = acciones(n)
        if not acts or PMD_IDLE not in acts:
            continue
        sz = dex[n][2]
        fila, filaAlto = {}, {}
        for nombre, factor, altoMax, act in VISTAS:
            a = acts.get(act, acts[PMD_IDLE])
            lienzo, alto, area = a
            mag = tinta.magnitud(alto, area)
            s, k, topado = tinta.escala_firmware(
                mag, alto, lienzo, sz * factor, altoMax)
            px = s * k
            fila[nombre] = area * px * px
            filaAlto[nombre] = alto * px
            if topado:
                topados.append((n, dex[n][0], nombre, round(alto * px)))
            if alto * px > altoMax + 1:
                desbordan.append((n, dex[n][0], nombre,
                                  round(alto * px), altoMax))
        sitio[n] = fila
        altos[n] = filaAlto

    # EVOLUCIONAR NO PUEDE HACERTE OCUPAR MENOS SITIO
    encogen = []
    for n in range(1, len(dex)):
        nom, evo, _sz = dex[n]
        if not evo or evo >= len(dex) or n not in sitio or evo not in sitio:
            continue
        a, b = sitio[n]['casa'], sitio[evo]['casa']
        if b < a * 0.98:
            encogen.append((nom, round(math.sqrt(a)), dex[evo][0],
                            round(math.sqrt(b))))

    if verboso:
        for n in sorted(sitio):
            f, al = sitio[n], altos[n]
            print('  %4d %-14s casa %3d px de lado (alto %3d)  '
                  'tuyo %3d  rival %3d'
                  % (n, dex[n][0], round(math.sqrt(f['casa'])),
                     round(al['casa']), round(math.sqrt(f['espaldas'])),
                     round(math.sqrt(f['frente']))))

    print('%d bichos medidos' % len(sitio))
    for nombre, _f, _am, _a in VISTAS:
        lados = sorted(math.sqrt(f[nombre]) for f in sitio.values())
        alt = sorted(a[nombre] for a in altos.values())
        print('  %-9s lado %3d..%3d (mediana %3d)   alto %3d..%3d'
              % (nombre, lados[0], lados[-1], lados[len(lados) // 2],
                 alt[0], alt[-1]))

    fallos = comprueba_constantes()
    if fallos:
        print('\nDESCUADRE CON EL FIRMWARE:')
        for f in fallos:
            print('  ' + f)

    if topados:
        print('\n%d con la escala RECORTADA (salen a otro tamano que el '
              'pedido, casi siempre por no caber de alto):' % len(topados))
        for n, nom, vista, px in topados[:15]:
            print('  %4d %-14s %-9s alto %3d' % (n, nom, vista, px))
        if len(topados) > 15:
            print('  ...y %d mas' % (len(topados) - 15))

    if desbordan:
        print('\n%d SE SALEN DEL HUECO:' % len(desbordan))
        for n, nom, vista, px, hueco in desbordan[:15]:
            print('  %4d %-14s %-9s %3d px en %3d' % (n, nom, vista, px, hueco))

    print('\n%d EVOLUCIONES QUE OCUPAN MENOS QUE SU FORMA ANTERIOR:'
          % len(encogen))
    for na, la, nb, lb in encogen:
        print('  %-14s lado %3d  ->  %-14s lado %3d   (%+d)'
              % (na, la, nb, lb, lb - la))

    # Los recortados NO son un fallo por si mismos: son los sprites mas
    # estirados del plantel, y quien les pone el limite es la pantalla, no
    # la tabla. Lo que SI seria un fallo es que fueran muchos, porque
    # entonces la banda estaria fuera de sitio y el recorte volveria a
    # aplastar a media Pokedex contra el mismo alto —— que es de donde se
    # viene. Se mira POR VISTA y en porcentaje: un total a dedo sobre las
    # tres no responde a esa pregunta.
    malo = len(encogen) + len(desbordan) + len(fallos)
    LIMITE = 0.08
    porVista = {}
    for _n, _nom, vista, _px in topados:
        porVista[vista] = porVista.get(vista, 0) + 1
    if topados:
        print('\n  recortados por vista: %s'
              % ', '.join('%s %d (%.1f%%)'
                          % (v_, c, 100.0 * c / len(sitio))
                          for v_, c in sorted(porVista.items())))
    for v_, c in porVista.items():
        if c > LIMITE * len(sitio):
            print('  DEMASIADOS en "%s": %d de %d (mas del %.0f%%). La banda '
                  'se fue de sitio y el recorte vuelve a aplastar.'
                  % (v_, c, len(sitio), 100 * LIMITE))
            malo += 1
    print('\n%s' % ('sin fallos' if not malo else '%d PROBLEMAS' % malo))
    return 1 if malo else 0


if __name__ == '__main__':
    sys.exit(main())
