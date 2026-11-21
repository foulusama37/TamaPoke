#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Que golpes se ven POBRES aunque tengan su animacion del Anil.

Por que hace falta
------------------
El dueno ha traido tres veces la misma queja ("play rough", "fire
blast", "flarebitz ta com animacao generica") y las tres veces el golpe
SI tenia su animacion del Anil. El problema es otro: el Anil cuenta
muchos golpes moviendo el SPRITE del bicho (una embestida, un salto) y
deja en la hoja solo una decoracion. Nosotros no coreografiamos
sprites, asi que de esa animacion solo copiamos la decoracion —— y sale
una chispita quieta.

Esto los ordena por lo poco que se ven, con tres medidas:

  tinta   el % del fotograma que llega a pintarse, en el mejor
          fotograma. Una llamita de 20x20 sobre un lienzo de 150 es un
          2%: eso no se ve en una pantalla de 466.
  mueve   cuanto cambia el dibujo entre fotogramas. 0 = una estampa
          quieta.
  comparte cuantos OTROS golpes salen de la misma hoja. Alto no es
          malo por si solo (un golpe de contacto es un impacto), pero
          un golpe estrella que comparte con 27 no se distingue.

No arregla nada: dice donde mirar. La correccion es a mano, mirando la
hoja, y vive en FX_CELDAS_A_MANO de gen_art.py.
"""
import collections
import io
import os
import re
import sys

AQUI = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, AQUI)

import fxanim                                        # noqa: E402
import gen_art                                       # noqa: E402


def mide(imgs):
    """(tinta %, movimiento %) de una lista de fotogramas."""
    tinta = 0.0
    prev = None
    mueve = 0.0
    n = 0
    for im in imgs:
        if im is None:
            continue
        a = im.getchannel("A")
        px = a.load()
        w, h = a.size
        op = 0
        for y in range(0, h, 2):
            for x in range(0, w, 2):
                if px[x, y] > 40:
                    op += 1
        total = max(1, (w + 1) // 2 * ((h + 1) // 2))
        tinta = max(tinta, 100.0 * op / total)
        if prev is not None:
            dif = 0
            pw, ph = prev.size
            for y in range(0, min(h, ph), 4):
                for x in range(0, min(w, pw), 4):
                    if (px[x, y] > 40) != (prev.load()[x, y] > 40):
                        dif += 1
            mueve += 100.0 * dif / max(1, (min(w, pw) // 4) * (min(h, ph) // 4))
            n += 1
        prev = a
    return tinta, (mueve / n if n else 0.0)


def main():
    pares, anims, _n = gen_art.golpes_con_animacion()
    hojas = fxanim.Hojas(os.path.join(gen_art.ANIL, "Animations"))

    txt = io.open(gen_art.MOVES_H, encoding="utf-8", errors="replace").read()
    nombres = re.findall(r'^\s*\{\s*"([A-Z0-9]+)",\s*T_', txt, flags=re.M)
    # solo los que se usan de verdad: los que alguien aprende
    aprendidos = set()
    for m in re.finditer(r'^\s*(\d+),', txt, flags=re.M):
        pass

    porHoja = collections.Counter()
    filas = []
    for mi, nom in pares:
        anim = anims[nom]
        fich = fxanim.hoja_de(anim) or "(sin hoja)"
        porHoja[fich] += 1
        try:
            imgs, _d = fxanim.cuadros(anim, hojas, max_cuadros=gen_art.FX_MOVE_CEL)
        except Exception:                            # noqa: BLE001
            continue
        if not imgs:
            continue
        t, mv = mide(imgs)
        filas.append((nom, fich, t, mv))

    filas.sort(key=lambda f: (f[2], f[3]))
    tope = int(sys.argv[1]) if len(sys.argv) > 1 else 40
    print("los %d que menos se ven (de %d con animacion):" % (tope, len(filas)))
    print("%-16s %6s %6s %5s  %s" % ("golpe", "tinta", "mueve", "comp", "hoja"))
    for nom, fich, t, mv in filas[:tope]:
        print("%-16s %5.1f%% %5.1f%% %5d  %s" % (nom, t, mv, porHoja[fich], fich))

    print()
    print("hojas mas compartidas:")
    for fich, n in porHoja.most_common(12):
        print("  %3d  %s" % (n, fich))


if __name__ == "__main__":
    main()
