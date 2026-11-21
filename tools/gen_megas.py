#!/usr/bin/env python3
"""Genera megas.h: las megaevoluciones, desde pokemon_forms.txt del Anil.

Una mega NO es una especie: no tiene numero de dex. Es una FORMA que el
bicho toma dentro del combate mientras lleva su piedra, y de la que vuelve
al acabar. Por eso no entra en DEX_TBL y vive en su propia tabla.

Lo que cambia al megaevolucionar, y lo que por tanto hay que guardar:
  stats base   los SEIS, PS incluidos. En los juegos oficiales una mega
                nunca toca los PS, pero el Anil SI: lo hace en varias, y
                entre sus "megas" hay versiones Dynamax, que por lo que
                son cambian la vida. Se comprobo al generar en vez de
                darlo por supuesto, y la suposicion era falsa.
                Consecuencia: al megaevolucionar cambia el maximo de PS y
                la vida actual tiene que escalar con el (ver battle.cpp),
                o seria curarse gratis o caer en el acto.
  tipos        Charizard X pasa de fuego/volador a fuego/dragon
  habilidad    casi siempre otra distinta
  alto         sale de la altura de la forma, como el resto
  sprite       otro fichero en la tarjeta (SLUG_<forma>.bin)

Las que NO entran:
  - las que el Anil deja sin MegaStone. Hoy solo Mega Rayquaza, que en
    los juegos megaevoluciona sabiendo Ascenso Draco en vez de con
    piedra, y que por eso lleva una inventada en gen_items.
  - el "Mewtwo Armadura" (forma 5), fuera desde 2026-10-04: no tiene
    sprite de espalda en ningun proyecto y tenia 400 de PS base.

  python3 tools/gen_megas.py
"""
import io
import os
import re
import sys

AQUI = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, AQUI)
import anil_pbs                        # noqa: E402
from dex_data import DEX               # noqa: E402
from dex_range import N_DEX            # noqa: E402
from type_chart import TIPOS as ORDEN  # noqa: E402
import tinta                           # noqa: E402
from dex_sizes import px_para, AREA_A, AREA_EXP, PX_MIN, PX_MAX

# El tamano sale del AREA DE TINTA del sprite, igual que la Pokedex y
# con la MISMA calibracion (AREA_A/AREA_EXP, que calcula gen_sizes.py
# sobre las 1025). Si cada tabla calculara la suya sobre su propio
# plantel, una mega saldria en otra escala que su forma base.
# px_para(metros) queda de ultimo recurso, por si falta el sprite.
def _px_sprite(fila, pref):
    a = tinta.area_idle(os.path.join(
        AQUI, 'sdcard', 'mons', '%c%03d.bin' % (pref, fila)))
    return tinta.px_de_area(a, AREA_A, AREA_EXP, PX_MIN, PX_MAX)


DST = os.path.join(os.path.dirname(AQUI), "megas.h")

SLUG = {d[0]: d[1] for d in DEX}
NOMBRE = {d[0]: d[2] for d in DEX}

C_TIPO = {t: "T_" + t.upper() for t in ORDEN}
C_TIPO["fighting"] = "T_FIGHT"
C_TIPO["electric"] = "T_ELEC"


def clave(s):
    return re.sub(r"[^A-Z0-9]", "", s.upper())


def indices_de_habilidad():
    """{CLAVE INTERNA: fila en ABIL_TBL}, calculada por gen_abilities.

    Por la clave INTERNA del PBS, no por el nombre. Se intento leyendo los
    nombres del abilities.h generado y fallo: alli estan TRADUCIDOS, asi
    que SOBRECARGA (una habilidad propia del Anil que solo usan megas) no
    se encontraba. Mismo fallo que en gen_evos con "Sweet Cream".
    """
    from gen_abilities import indices
    return {clave(k): v for k, v in indices().items()}


def piedras_de_items():
    """{CLAVE_INTERNA: constante del arg} de las piedras mega de items.h.

    Por la clave INTERNA del PBS y no por el nombre de pantalla: es el
    mismo fallo que ya mordio en gen_evos, donde WHIPPEDDREAM se llama
    "Sweet Cream" y la evolucion no encontraba un objeto que existia.
    """
    from gen_items import piedras_mega
    return {clave(i): a for i, _b, _fx, a, *_ in piedras_mega()}


def piedra_de(cab, c):
    from gen_items import piedra_de as _p
    return _p(cab, c)


# Nombres de forma que el Anil dejo en castellano o que no dicen de quien
# son. La tabla entera va en ingles y el nombre se pinta en la ficha.
#
# Esta vacia desde que salio el Mewtwo Armadura (2026-10-04). Se queda
# porque el caso vuelve en cuanto el Anil traiga otra forma con nombre
# castellano, y porque las megas por forma ya usan LETRA_FORMA.
RENOMBRA = {}


def nombre_corto(fn, base, form=0, desde=0):
    """"Mega Charizard X" -> "MEGA CHARIZARD X", recortado a 12.

    No se guarda el nombre entero: la ficha da para doce caracteres y
    "MEGA " ya se come cinco. Para los largos se deja el "M." delante,
    que se lee igual y cabe.

    `desde` es la forma de la que SALE la mega, y hace falta porque hay
    especies con varias megas que el Anil llama a todas igual: las tres
    de Tatsugiri comparten FormName, piedra y stats, y lo unico que las
    separa es de que forma vienen. Se les pega la letra de esa forma,
    la misma que lleva la forma en forms.h.
    """
    if (base, form) in RENOMBRA:
        return RENOMBRA[(base, form)]
    n = fn.upper().replace("PRIMAL", "MEGA")
    n = re.sub(r"[^A-Z0-9 ]", "", n)
    letra = anil_pbs.LETRA_FORMA.get((base, desde))
    if letra:
        n += " " + letra
    if len(n) <= 12:
        return n
    if n.startswith("MEGA "):
        n = "M." + n[5:]
    if len(n) <= 12:
        return n
    # Se recorta por el MEDIO, no por el final: el sufijo es lo que
    # distingue a Mega Charizard X de la Y, y cortando a doce por detras
    # los dos salian "M.CHARIZARD" y en pantalla eran el mismo bicho.
    trozos = n.split()
    if len(trozos) > 1 and len(trozos[-1]) <= 2:
        cola = " " + trozos[-1]
        return (" ".join(trozos[:-1]))[:12 - len(cola)] + cola
    return n[:12].strip()


def main():
    num = anil_pbs.numeros()
    filas, sinPiedra, pisHp = [], [], []
    base_stats, fichas = {}, {}
    for n, _slug, c in anil_pbs.filas():
        base_stats[n] = anil_pbs.stats(c)
        fichas[n] = c

    for cab, c in anil_pbs.bloques(anil_pbs.FORMS):
        fn = c.get("FormName", "")
        if "Mega" not in fn and "Primal" not in fn:
            continue
        partes = cab.split(",")
        base = num.get(clave(partes[0]))
        if not base or base > N_DEX:
            continue
        # La piedra la resuelve gen_items, que es quien sabe tambien de
        # las dos inventadas (Rayquaza y el Mewtwo Armadura, que el Anil
        # deja sin ninguna).
        piedra = piedra_de(cab, c)
        if not piedra:
            sinPiedra.append((base, fn))
            continue

        # Una forma del Anil solo declara lo que CAMBIA; lo demas se
        # hereda de la especie. Mega Gengar no repite sus tipos porque
        # son los mismos, y leer el bloque a pelo revienta con un
        # KeyError o, peor, deja la mega sin tipos.
        c = dict(fichas[base], **c)

        st = anil_pbs.stats(c)
        if st[0] != base_stats[base][0]:
            pisHp.append((base, fn, base_stats[base][0], st[0]))
        t1, t2 = anil_pbs.tipos(c)
        hab = (c.get("Abilities", "").split(",") or [""])[0].strip()
        alto = anil_pbs.altura_dm(c)
        # De que FORMA sale esta mega. El PBS lo dice con UnmegaForm, que
        # es justo lo que faltaba: Toxtricity tiene dos megas con la misma
        # piedra, una desde la forma normal y otra desde la Grave, y sin
        # este campo no habia como distinguirlas (se tiraba una).
        # UnmegaForm NEGATIVO (-2) en tres: Meowstic, Zygarde y Alcremie,
        # que son bichos de muchas formas. Significa "al deshacer la mega
        # vuelve a la que fuera", o sea que la mega NO esta atada a una
        # forma concreta: vale para cualquiera.
        desde = int(c.get("UnmegaForm", "0") or 0)
        if desde < 0:
            desde = 0xFF            # MEGA_ANY_FORM
        filas.append((base, int(partes[1]), fn, piedra, st, t1, t2, hab,
                      alto, desde))

    filas.sort(key=lambda f: (f[0], f[1]))

    # Una especie puede tener VARIAS megas con la MISMA piedra: son las
    # megas de sus otras formas (Toxtricity tiene Agudo y Grave,
    # Tatsugiri tres, Magearna la de color original). Lo que las
    # distingue es de QUE FORMA salen (UnmegaForm), asi que la clave
    # lleva las tres cosas. Antes, sin la forma, cuatro filas se caian
    # por "repetidas" cuando no lo eran.
    vistas, unicas, repes = set(), [], []
    for f in filas:
        k = (f[0], f[9], f[3])
        if k in vistas:
            repes.append(f)
            continue
        vistas.add(k)
        unicas.append(f)
    filas = unicas

    out = [
        "// GENERADO por tools/gen_megas.py desde pokemon_forms.txt del Anil.",
        "// No editar.",
        "//",
        "// Una mega no es una especie: no tiene numero de dex. Es una forma",
        "// que se toma DENTRO del combate llevando su piedra, y de la que se",
        "// vuelve al acabar, asi que no entra en DEX_TBL.",
        "//",
        "// OJO: aqui los PS SI cambian. En los juegos oficiales una mega",
        "// nunca los toca, pero el Anil lo hace en varias (entre sus megas",
        "// hay versiones Dynamax). Por eso bHp esta en la tabla y por eso",
        "// megaEvolve() escala la vida actual con el maximo nuevo.",
        "#pragma once",
        "#include <stdint.h>",
        '#include "dex.h"',
        '#include "items.h"',
        "",
        "// Una mega que no esta atada a una forma concreta: Meowstic,",
        "// Zygarde y Alcremie, que tienen muchas y vuelven a la que fueran.",
        "#define MEGA_ANY_FORM 0xFF",
        "",
        "struct MegaForm {",
        "  int16_t base;      // numero de dex del que se transforma",
        "  uint8_t item;      // MEGA_* : el `arg` de su piedra en ITEM_TBL",
        "  uint8_t form;      // numero de forma en el Anil: el sprite es",
        "                     // <slug>_<form>.bin en la tarjeta",
        "  // De QUE forma sale (0 = de la normal, MEGA_ANY_FORM = de",
        "  // cualquiera). Lo dice UnmegaForm en el PBS, y es lo unico que",
        "  // distingue las dos megas de Toxtricity, que piden la misma",
        "  // piedra: una sale del normal y otra del Grave.",
        "  uint8_t fromForm;",
        "  // bHp es uint16_t y los otros cinco uint8_t. Hoy el mayor es el",
        "  // Mega Snorlax con 160, asi que cabria en un byte; se queda",
        "  // ancho porque el Anil mete versiones Dynamax entre sus megas y",
        "  // ya hubo una (el Mewtwo Armadura) con 400 de PS base. Pasarlo a",
        "  // byte ahorra 127 bytes de flash y vuelve a romperse solo.",
        "  uint16_t bHp;",
        "  uint8_t bAtk, bDef, bSpA, bSpD, bSpe;",
        "  uint8_t type1, type2;",
        "  uint16_t ability;",
        "  uint8_t screenSz;   // alto en pantalla, como DexEntry",
        "  const char *name;  // 12 caracteres como mucho, igual que la dex",
        "};",
        "",
        f"#define MEGA_COUNT {len(filas)}",
        "static const MegaForm MEGA_TBL[MEGA_COUNT] = {",
    ]

    abil = indices_de_habilidad()
    piedras = piedras_de_items()
    faltaHab, faltaPiedra = set(), set()
    sinSprite = []
    for _fila, (base, form, fn, piedra, st, t1, t2, hab, alto, desde) in enumerate(filas):
        iab = abil.get(clave(hab), 0)
        if hab and not iab:
            faltaHab.add(hab)
        cte = piedras.get(clave(piedra))
        if not cte:
            faltaPiedra.add(piedra)
            cte = "0"
        px = _px_sprite(_fila, 'm')
        if px is None:
            px = px_para(alto / 10.0)
            sinSprite.append(fn)
        out.append(
            f'  {{ {base:4d}, {cte:<16s}, {form:2d}, {desde:2d}, '
            f'{st[0]:3d}, {st[1]:3d}, {st[2]:3d}, {st[3]:3d}, {st[4]:3d}, '
            f'{st[5]:3d}, '
            f'{C_TIPO[t1]:<10s}, {C_TIPO[t2] if t2 else "T_NONE":<10s}, '
            f'{iab:3d}, {px:3d}, "{nombre_corto(fn, base, form, desde)}" }},'
            f'  // {NOMBRE.get(base, base)}')
    out += [
        "};",
        "",
        "// La mega de una especie con ESA piedra, o -1.",
        "//",
        "// Hacen falta las tres cosas: Charizard tiene dos megas y las",
        "// separa la PIEDRA; Toxtricity tiene dos con la misma piedra y",
        "// las separa la FORMA de la que sale (la normal o la Grave).",
        "static inline int16_t megaFormOf(int16_t dex, uint8_t item,",
        "                                 uint8_t form = 0) {",
        "  for (uint16_t i = 0; i < MEGA_COUNT; i++)",
        "    if (MEGA_TBL[i].base == dex && MEGA_TBL[i].item == item &&",
        "        (MEGA_TBL[i].fromForm == form ||",
        "         MEGA_TBL[i].fromForm == MEGA_ANY_FORM))",
        "      return (int16_t)i;",
        "  return -1;",
        "}",
        "",
        "// Si a esta especie le sirve alguna piedra, para poder decirlo en",
        "// la ficha sin tener que saber cual lleva encima.",
        "static inline bool hasMega(int16_t dex) {",
        "  for (uint16_t i = 0; i < MEGA_COUNT; i++)",
        "    if (MEGA_TBL[i].base == dex) return true;",
        "  return false;",
        "}",
        "",
    ]
    print(f"{len(filas)} megaevoluciones")
    if sinPiedra:
        print(f"\n{len(sinPiedra)} fuera por no tener piedra en el Anil:")
        for b, fn in sinPiedra:
            print(f"   {NOMBRE.get(b, b)}: {fn}")
    if repes:
        print(f"\n{len(repes)} fuera por compartir piedra con otra mega del")
        print("mismo bicho (son megas de sus OTRAS formas, que aqui no hay):")
        for f in repes:
            print(f"   {NOMBRE.get(f[0], f[0])}: {f[2]}")
    if pisHp:
        print(f"\n{len(pisHp)} cambian los PS base (el Anil lo hace; los")
        print("juegos oficiales no). megaEvolve() escala la vida actual:")
        for b, fn, a, d in pisHp[:6]:
            print(f"   {NOMBRE.get(b, b)} {fn}: {a} -> {d}")
        if len(pisHp) > 6:
            print(f"   ... y {len(pisHp) - 6} mas")
    if faltaHab:
        print(f"\nhabilidades que no estan en abilities.h: "
              + ", ".join(sorted(faltaHab)))
    if faltaPiedra:
        print(f"\npiedras que no estan en items.h: "
              + ", ".join(sorted(faltaPiedra)))

    io.open(DST, "w", encoding="utf-8", newline="\n").write("\n".join(out) + "\n")
    print(f"\nescrito {os.path.normpath(DST)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
