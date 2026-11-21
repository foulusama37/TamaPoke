#!/usr/bin/env python3
"""Genera formas_combate.h: las formas que se ganan y se pierden EN el combate.

No son bichos distintos, como las regionales de forms.h: un Castform al
sol sigue siendo un Castform, y al acabar el combate vuelve a su forma.
Por eso NO van en FORM_TBL. FORM_TBL esta en el save (dexFormReg, un bit
por fila), en la Pokedex y en la evolucion (formAfterEvolve conserva el
numero de forma); meter aqui una forma de combate la haria aparecer en
esos tres sitios.

Sus sprites van por NUMERO DE DEX Y DE FORMA, no por fila de una tabla:
/mons/c0351_1.bin (y cs0351_1.bin el shiny). Asi, que la tabla crezca no
pinta jamas el dibujo del vecino.

  python tools/pack_anil.py combate     # primero los sprites
  python tools/gen_formas_combate.py    # luego la tabla (mide su tinta)
"""
import os
import sys

AQUI = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, AQUI)
import anil_pbs                        # noqa: E402
import tinta                           # noqa: E402
from dex_sizes import px_para, AREA_A, AREA_EXP, PX_MIN, PX_MAX  # noqa: E402
from type_chart import TIPOS as ORDEN  # noqa: E402

# (especie, forma) del PBS del Anil. Cada una la dispara una regla de
# Battler_ChangeSelf / Move_Usage del Anil (ver battle.cpp).
FORMAS = [
    ("CASTFORM", 1), ("CASTFORM", 2), ("CASTFORM", 3),   # Predicción: sol, lluvia, nieve
    ("CHERRIM", 1),                                      # Don Floral: al sol
    ("DARMANITAN", 1), ("DARMANITAN", 3),                # Modo Daruma (normal y de Galar)
    ("AEGISLASH", 1),                                    # Cambio Táctico: Filo
    ("WISHIWASHI", 1),                                   # Banco
    ("MINIOR", 7),                                       # Escudo Limitado: núcleo
    ("MIMIKYU", 1),                                      # Disfraz roto
    ("EISCUE", 1),                                       # Cara de Hielo rota
    ("CRAMORANT", 1), ("CRAMORANT", 2),                  # Tragamisil: con Arrokuda / con Pikachu
    ("MORPEKO", 1),                                      # Mas que Mono: voraz
    ("ZYGARDE", 2),                                      # Agrupamiento: completo
]

C_TIPO = {t: "T_" + t.upper() for t in ORDEN}
C_TIPO["fighting"] = "T_FIGHT"
C_TIPO["electric"] = "T_ELEC"

MONS = os.path.join(AQUI, "sdcard", "mons")
DST = os.path.join(os.path.dirname(AQUI), "formas_combate.h")


def fichero(base, forma, shiny=False):
    return "c%s%04d_%d.bin" % ("s" if shiny else "", base, forma)


def main():
    num = anil_pbs.numeros()
    fichas = {n: c for n, _s, c in anil_pbs.filas()}
    bloques = {anil_pbs.clave(cab): c for cab, c in anil_pbs.bloques(anil_pbs.FORMS)}
    filas, faltan = [], []
    for esp, forma in FORMAS:
        base = num[anil_pbs.clave(esp)]
        c = bloques.get(anil_pbs.clave("%s,%d" % (esp, forma)))
        if c is None:
            raise SystemExit("el PBS del Anil no trae [%s,%d]" % (esp, forma))
        c = dict(fichas[base], **c)        # la forma solo declara lo que cambia
        st = anil_pbs.stats(c)
        t1, t2 = anil_pbs.tipos(c)
        ruta = os.path.join(MONS, fichero(base, forma))
        if os.path.exists(ruta):
            px = tinta.px_de_area(tinta.area_idle(ruta), AREA_A, AREA_EXP,
                                  PX_MIN, PX_MAX)
        else:
            px = px_para(anil_pbs.altura_dm(c) / 10.0)
            faltan.append(fichero(base, forma))
        filas.append((base, forma, st, t1, t2, px, esp, c.get("FormName", "")))

    out = [
        "// GENERADO por tools/gen_formas_combate.py desde pokemon_forms.txt del Anil.",
        "// No editar.",
        "//",
        "// Las formas que se ganan y se pierden DENTRO del combate (Castform al",
        "// sol, Aegislash Filo, Darmanitan Daruma...). No van en FORM_TBL porque",
        "// esa tabla esta en el save, en la Pokedex y en la evolucion. El sprite",
        "// es /mons/c<dex 4 cifras>_<forma>.bin: por dex y forma, nunca por fila.",
        "#pragma once",
        "#include <stdint.h>",
        '#include "dex.h"',
        "",
        "struct BattleForm {",
        "  int16_t base;",
        "  uint8_t form;       // el numero de forma del Anil",
        "  uint8_t bHp, bAtk, bDef, bSpA, bSpD, bSpe;",
        "  uint8_t type1, type2;",
        "  uint8_t screenSz;   // alto de cuerpo, como DEX_TBL[].screenSz",
        "};",
        "",
        "#define BFORM_COUNT %d" % len(filas),
        "static const BattleForm BFORM_TBL[BFORM_COUNT] = {",
    ]
    for base, forma, st, t1, t2, px, esp, fn in filas:
        out.append(
            "  { %4d, %2d, %3d, %3d, %3d, %3d, %3d, %3d, %-9s, %-9s, %3d },  // %s: %s"
            % (base, forma, st[0], st[1], st[2], st[3], st[4], st[5],
               C_TIPO[t1], C_TIPO[t2] if t2 else "T_NONE", px, esp, fn))
    out += [
        "};",
        "",
        "// La fila de esa forma de combate de esa especie, o -1.",
        "static inline int8_t bformRow(int16_t dex, uint8_t form) {",
        "  for (uint8_t i = 0; i < BFORM_COUNT; i++)",
        "    if (BFORM_TBL[i].base == dex && BFORM_TBL[i].form == form)",
        "      return (int8_t)i;",
        "  return -1;",
        "}",
        "",
    ]
    open(DST, "w", encoding="utf-8", newline="\n").write("\n".join(out))
    print("formas_combate.h: %d formas" % len(filas))
    if faltan:
        print("SIN SPRITE (alto por la altura del PBS): " + ", ".join(faltan))


if __name__ == "__main__":
    main()
