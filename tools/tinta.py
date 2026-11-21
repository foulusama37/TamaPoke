#!/usr/bin/env python3
"""El AREA DE TINTA de un sprite TPK2, y como se traduce a un tamano.

Lo usan gen_sizes.py (la Pokedex), gen_megas.py y gen_forms.py, y los tres
tienen que usar la MISMA calibracion o una mega acaba dibujada en otra
escala que su forma base. Por eso la calibracion (A y EXP) la calcula
gen_sizes.py sobre las 1025 y la deja escrita en dex_sizes.py, y los otros
dos la importan en vez de calcular la suya.

POR QUE EL AREA, despues de probar las otras tres:

  altura del lienzo   depende de la POSE. Pidgeotto esta dibujado volando
                      (116 px) y Pidgeot posado (68): evolucionar encogia.
  altura de la ficha  miente con todo lo largo. Dratini "mide" 1,8 m
                      —— mas que un Charizard —— porque son 1,8 m de
                      serpiente de 3,3 kg.
  peso de la ficha    miente en los dos sentidos, y lo dijo el usuario con
                      dos contraejemplos: "tem pokemon pequeno q eh pesado,
                      e pokemon grande q e leve". Cosmoem son 0,1 m y 999
                      kg; Gastly 1,3 m y 0,1 kg, y por eso el Gastly salia
                      "quase invisivel na tela".

El area de tinta es lo que el artista DIBUJO, que es lo que el jugador ya
conoce, y a diferencia de la altura aguanta la pose: un pajaro con las alas
abiertas es alto y ancho, pero la tinta de su cuerpo es la que es. La raiz
cuadrada la pasa de area a medida lineal, que es lo que se escala.
"""
import math
import os
import struct

PMD_IDLE = 0
HUECO = 0xFF      # el indice transparente de TPK2 (el 0 es un color mas)


def area_idle(ruta):
    """Pixeles con tinta del primer frame de la accion IDLE, o None."""
    if not os.path.exists(ruta):
        return None
    d = open(ruta, 'rb').read()
    if len(d) < 7 or d[:4] != b'TPK2':
        return None
    nActs = d[4]
    palCount = struct.unpack('<H', d[5:7])[0]
    p = 7 + palCount * 2
    for _ in range(nActs):
        if p + 4 > len(d):
            return None
        idd, w, h, nf = d[p], d[p + 1], d[p + 2], d[p + 3]
        p += 4 + nf * 2
        if idd == PMD_IDLE:
            fr = d[p:p + w * h]
            if len(fr) < w * h:
                return None
            return sum(1 for b in fr if b != HUECO)
        p += w * h * nf
    return None


def px_de_area(area, A, EXP, LO, HI):
    """El area de tinta llevada a un alto de cuerpo en pixeles."""
    if not area:
        return None
    v = A * (math.sqrt(area) ** EXP)
    return int(max(LO, min(HI, round(v))))

def alto_idle(ruta):
    """Alto del CUERPO (base - top) del IDLE, mirando todos los
    frames, igual que sdmon.cpp. None si no se puede leer."""
    if not os.path.exists(ruta):
        return None
    d = open(ruta, 'rb').read()
    if len(d) < 7 or d[:4] != b'TPK2':
        return None
    nActs = d[4]
    palCount = struct.unpack('<H', d[5:7])[0]
    p = 7 + palCount * 2
    for _ in range(nActs):
        if p + 4 > len(d):
            return None
        idd, w, h, nf = d[p], d[p + 1], d[p + 2], d[p + 3]
        p += 4 + nf * 2
        if idd == PMD_IDLE:
            base, top = 1, h
            for f in range(nf):
                fr = d[p + f * w * h:p + (f + 1) * w * h]
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
            return (base - top) if base > top else (h or 1)
        p += w * h * nf
    return None

# ---------------------------------------------------------------------
# LA CUENTA DEL FIRMWARE, replicada una sola vez.
#
# Tiene que seguir a escalaDibujo/factorTinta (TamaPoke.ino) y a lo que
# PmdAct calcula al cargar (sdmon.cpp). Quien la copie en otro sitio se
# quedara viejo: ya paso dos veces en este proyecto.

B_AREA = 0.65          # el mismo PMD_B_AREA de sdmon.h


def magnitud(alto_tinta, area_tinta, b=B_AREA):
    """Lo que se normaliza: ni el alto ni el area, una mezcla.

    Normalizando el ALTO, un sprite estirado ocupa menos SITIO que uno
    redondo del mismo alto —— y el sitio es lo que el ojo lee. Normalizando
    el AREA, los estirados se salen por arriba de la pantalla. La mezcla
    con b=0,65 deja la lista de bichos estirados bien y solo 29 contra el
    techo.
    """
    if not alto_tinta or not area_tinta:
        return 1.0
    return (alto_tinta ** (1.0 - b)) * (math.sqrt(area_tinta) ** b)


def escala_firmware(mag, alto_tinta, lienzo_h, quiere, alto_max=0, max_s=5):
    """(s entera, k fraccionaria, si se topo) como las calcula el firmware."""
    if quiere < 1 or mag <= 0:
        return 1, 1.0, False
    s = int(quiere / mag + 0.5)
    s = 1 if s < 1 else (max_s if s > max_s else s)
    while s > 1 and lienzo_h * s > 250:
        s -= 1
    k = quiere / (mag * s)
    topado = k < 0.45 or k > 1.9
    k = 0.45 if k < 0.45 else (1.9 if k > 1.9 else k)
    # y que quepa de alto
    recortado = False
    if alto_max and alto_tinta * s * k > alto_max:
        k = alto_max / float(alto_tinta * s)
        recortado = True
    return s, k, (topado or recortado)
