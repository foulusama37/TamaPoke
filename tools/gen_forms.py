#!/usr/bin/env python3
"""Genera forms.h: las FORMAS alternativas, desde pokemon_forms.txt.

Una forma no es una especie: comparte numero de dex con la suya y cambia
tipos, stats, habilidad y dibujo. Un Vulpix de Alola es un Vulpix.

QUE ENTRA, y por que no todo:

  regionales   Alola, Galar, Hisui y Paldea. Son 58 y son el grupo que
               mas sentido tiene aqui: el juego YA va por regiones, asi
               que un Vulpix cogido en Alola sale de Alola solo. Ademas
               encadenan — el Vulpix de Alola da un Ninetales de Alola —
               con una regla y sin datos extra: al evolucionar se
               conserva el numero de forma si el destino lo tiene.

  con mega     las cuatro que se quedaron fuera de megas.h por compartir
               piedra con la forma base (Toxtricity, Tatsugiri, Magearna).
               Eran el motivo de todo esto.

  FUERA quedan:
  - las de estado de combate (Aegislash Filo, Wishiwashi Banco, Darmanitan
    Daruma, Minior, Castform). Esas cambian DENTRO del combate segun lo
    que pasa, que es otra mecanica y no un bicho distinto.
  - los 18 tipos de Arceus y los 18 de Silvally: aqui no hay Tablas ni
    Discos que los cambien, asi que serian 36 filas muertas.
  - las puramente cosmeticas (Vivillon, Furfrou, las letras de Unown):
    178 formas que no tocan el combate.

  python3 tools/gen_forms.py
"""
import io
import os
import re
import sys

AQUI = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, AQUI)
import anil_pbs                        # noqa: E402
from dex_data import DEX               # noqa: E402
from dex_range import N_DEX, REGIONES  # noqa: E402
import tinta                           # noqa: E402
from dex_sizes import px_para, AREA_A, AREA_EXP, PX_MIN, PX_MAX  # noqa: E402
from type_chart import TIPOS as ORDEN  # noqa: E402

# El tamano sale del AREA DE TINTA del sprite, igual que la Pokedex y
# con la MISMA calibracion (AREA_A/AREA_EXP, que calcula gen_sizes.py
# sobre las 1025). Si cada tabla calculara la suya sobre su propio
# plantel, una mega saldria en otra escala que su forma base.
# px_para(metros) queda de ultimo recurso, por si falta el sprite.
def _px_sprite(fila, pref):
    a = tinta.area_idle(os.path.join(
        AQUI, 'sdcard', 'mons', '%c%03d.bin' % (pref, fila)))
    return tinta.px_de_area(a, AREA_A, AREA_EXP, PX_MIN, PX_MAX)


DST = os.path.join(os.path.dirname(AQUI), "forms.h")

NOMBRE = {d[0]: d[2] for d in DEX}
SLUG = {d[0]: d[1] for d in DEX}

C_TIPO = {t: "T_" + t.upper() for t in ORDEN}
C_TIPO["fighting"] = "T_FIGHT"
C_TIPO["electric"] = "T_ELEC"

# Como se llama cada region en el FormName del Anil, a que indice de
# REGIONES va, y con que PROBABILIDAD sale alli.
#
# El 100 es lo normal: en Alola, un Vulpix ES de Alola, no hay otro. Pero
# HISUI no es una region del juego — es la Sinnoh antigua. Mandar sus
# formas a Sinnoh al 100% hacia que alli TODO Growlithe, Voltorb y Zorua
# fuera hisuiano, que es falso de dos maneras: ni son de Sinnoh ni
# sustituyen al normal. Al 12% son lo que deben ser, una rareza que
# aparece en la tierra donde existio.
REGIONALES = {
    "Alola":  ("ALOLA",  100),
    "Galar":  ("GALAR",  100),
    "Paldea": ("PALDEA", 100),
    "Hisui":  ("SINNOH",  12),
}
IDX_REGION = {n: i for i, (n, _f) in enumerate(REGIONES)}

# Las formas que NO son regionales pero entran igual porque tienen una
# mega propia: sin ellas, esas cuatro megas no se pueden alcanzar.
CON_MEGA = {("TOXTRICITY", 1), ("TATSUGIRI", 1), ("TATSUGIRI", 2),
            ("MAGEARNA", 1)}

SIN_REGION = 0xFF


def clave(s):
    return re.sub(r"[^A-Z0-9]", "", s.upper())


# Las que NO son regionales: su FormName esta en castellano y ademas no
# dice de quien es ("Forma Grave" a secas). Se nombran como las megas,
# con la inicial de la forma delante del bicho. La letra sale de
# anil_pbs porque gen_megas usa la misma para sus megas.
RENOMBRA = {k: (v + "." + NOMBRE.get(k[0], "?"))[:12]
            for k, v in anil_pbs.LETRA_FORMA.items()}


def sin_tildes(s):
    """Quita las tildes SIN comerse la letra.

    `[^A-Z0-9 ]` borraba la vocal entera y "Languida" salia "LNGUID".
    """
    import unicodedata
    return "".join(c for c in unicodedata.normalize("NFD", s)
                   if unicodedata.category(c) != "Mn")


# Cuando una region trae VARIAS variedades del mismo bicho, el nombre de
# la region no basta para distinguirlas. Tauros de Paldea son tres, y los
# tres se llamaban "P.TAUROS": en la hoja de contacto salian tres filas
# seguidas con el mismo rotulo y tres dibujos distintos.
VARIEDAD = {"COMBATIENTE": "CMB", "ARDIENTE": "BLZ", "ACUATICA": "AQU"}


def variedad(fn):
    """El "(Combatiente)" de un FormName, en tres letras, o "".

    Si aparece una variedad que no esta en la tabla se cogen sus tres
    primeras letras y se avisa: mejor un rotulo feo que dos iguales.
    """
    m = re.search(r"\(([^)]+)\)", fn)
    if not m:
        return ""
    v = re.sub(r"[^A-Z0-9]", "", sin_tildes(m.group(1)).upper())
    if v not in VARIEDAD:
        print(f"   ojo: variedad sin abreviatura propia: {m.group(1)!r}")
    return VARIEDAD.get(v, v[:3])


def nombre_corto(fn, base, forma=0):
    """"Forma Alola" sobre VULPIX -> "A.VULPIX", <= 12 chars."""
    # Prefijo de DOS letras y no el nombre entero de la region: "ALOLAN
    # VULPIX" son trece y la ficha da para doce, asi que salia "ALOLAN
    # VULPI" — se perdia justo el nombre del bicho. Con "A.VULPIX" caben
    # los 63 y se lee igual. Es el mismo criterio que el "M." de las megas.
    if (base, forma) in RENOMBRA:
        return RENOMBRA[(base, forma)]
    for es in REGIONALES:
        if fn.startswith("Forma " + es):
            p = {"Alola": "A.", "Galar": "G.", "Hisui": "H.",
                 "Paldea": "P."}[es]
            var = variedad(fn)
            if var:
                # 8 + " " + 3 = los 12 justos
                return (p + NOMBRE.get(base, "?"))[:8].strip() + " " + var
            return (p + NOMBRE.get(base, "?"))[:12]
    n = re.sub(r"[^A-Z0-9 ]", "", sin_tildes(fn).upper())
    return n[:12].strip()


def main():
    num = anil_pbs.numeros()
    fichas = {n: c for n, _s, c in anil_pbs.filas()}

    # El filtro vive en anil_pbs: gen_abilities necesita el mismo para
    # meter las habilidades de estas formas en el catalogo.
    incluidas = anil_pbs.formas_incluidas()
    todas = [1 for cab, c in anil_pbs.bloques(anil_pbs.FORMS)
             if "Mega" not in c.get("FormName", "")
             and "Primal" not in c.get("FormName", "")]
    saltadas = len(todas) - len(incluidas)

    filas = []
    for cab, c in incluidas:
        fn = c.get("FormName", "")
        partes = cab.split(",")
        base = num.get(clave(partes[0]))
        if not base or base > N_DEX:
            continue
        forma = int(partes[1]) if len(partes) > 1 and partes[1].isdigit() else 0

        # `startswith` y no `in`: "Forma Gorra Alola" es un GORRO de
        # Pikachu, no un Pikachu de Alola (que no existe). Con `in`
        # entraba, y solo se vio en la hoja de contacto.
        region, prob = None, 100
        for es, (destino, p) in REGIONALES.items():
            if fn.startswith("Forma " + es):
                region, prob = IDX_REGION[destino], p
                break

        c = dict(fichas[base], **c)      # la forma solo declara lo que cambia
        st = anil_pbs.stats(c)
        t1, t2 = anil_pbs.tipos(c)
        hab = (c.get("Abilities", "").split(",") or [""])[0].strip()
        filas.append((base, forma, fn,
                      region if region is not None else SIN_REGION,
                      st, t1, t2, hab, anil_pbs.altura_dm(c), prob))

    filas.sort(key=lambda f: (f[0], f[1]))

    # Repartir la probabilidad cuando una region trae VARIAS variedades
    # del mismo bicho. Paldea tiene tres Tauros, y los tres pedian el
    # 100%: formForRegion devuelve la primera que casa, asi que el de
    # Combate salia siempre y los otros dos eran filas muertas — el
    # sprite estaba en la tarjeta y no habia manera de encontrarlo.
    # Lo que suma el grupo se mantiene (Hisui sigue al 12 entre todas).
    grupos = {}
    for i, f in enumerate(filas):
        if f[3] != SIN_REGION:
            grupos.setdefault((f[0], f[3]), []).append(i)
    repartidas = 0
    for (_b, _r), idxs in grupos.items():
        if len(idxs) < 2:
            continue
        total = filas[idxs[0]][9]
        trozo = total // len(idxs)
        for k, i in enumerate(idxs):
            # el ultimo se queda el resto, para que el grupo sume justo
            p = total - trozo * (len(idxs) - 1) if k == len(idxs) - 1 else trozo
            filas[i] = filas[i][:9] + (p,)
        repartidas += len(idxs)

    from gen_abilities import indices
    abil = {clave(k): v for k, v in indices().items()}
    faltaHab = set()

    out = [
        "// GENERADO por tools/gen_forms.py desde pokemon_forms.txt del Anil.",
        "// No editar.",
        "//",
        "// Una FORMA comparte numero de dex con su especie y cambia tipos,",
        "// stats, habilidad y dibujo. Un Vulpix de Alola es un Vulpix.",
        "//",
        "// Las regionales se asignan por DONDE SALE el bicho: en Alola, un",
        "// Vulpix sale de Alola. Y encadenan solas, porque al evolucionar se",
        "// conserva el numero de forma si el destino lo tiene (formOf).",
        "#pragma once",
        "#include <stdint.h>",
        '#include "dex.h"',
        "",
        "#define FORM_NO_REGION 0xFF",
        "",
        "struct FormVariant {",
        "  int16_t base;",
        "  uint8_t form;      // el sprite es <slug>_<form> en la tarjeta",
        "  uint8_t region;    // en que region sale, o FORM_NO_REGION",
        "  uint8_t chance;    // con que probabilidad, en % (ver abajo)",
        "  uint16_t bHp;",
        "  uint8_t bAtk, bDef, bSpA, bSpD, bSpe;",
        "  uint8_t type1, type2;",
        "  uint16_t ability;",
        "  uint8_t screenSz;",
        "  const char *name;",
        "};",
        "",
        f"#define FORM_COUNT {len(filas)}",
        "static const FormVariant FORM_TBL[FORM_COUNT] = {",
    ]
    sinSprite = []
    for _fila, (base, forma, fn, region, st, t1, t2, hab, alto, prob) in enumerate(filas):
        iab = abil.get(clave(hab), 0)
        if hab and not iab:
            faltaHab.add(hab)
        reg = "FORM_NO_REGION" if region == SIN_REGION else str(region)
        px = _px_sprite(_fila, 'v')
        if px is None:
            px = px_para(alto / 10.0)
            sinSprite.append(fn)
        out.append(
            f'  {{ {base:4d}, {forma:2d}, {reg:<14s}, {prob:3d}, '
            f'{st[0]:3d}, {st[1]:3d}, {st[2]:3d}, {st[3]:3d}, {st[4]:3d}, '
            f'{st[5]:3d}, {C_TIPO[t1]:<10s}, '
            f'{C_TIPO[t2] if t2 else "T_NONE":<10s}, {iab:3d}, '
            f'{px:3d}, "{nombre_corto(fn, base, forma)}" }},'
            f'  // {NOMBRE.get(base, base)}: {fn}')
    out += [
        "};",
        "",
        "// La fila de esa forma de esa especie, o -1.",
        "static inline int16_t formRow(int16_t dex, uint8_t form) {",
        "  if (!form) return -1;            // 0 = la forma normal",
        "  for (uint16_t i = 0; i < FORM_COUNT; i++)",
        "    if (FORM_TBL[i].base == dex && FORM_TBL[i].form == form)",
        "      return (int16_t)i;",
        "  return -1;",
        "}",
        "",
        "// Que forma le toca a esta especie en esta region (0 = la normal).",
        "//",
        "// `roll` es una tirada 0..99 y hace falta porque no todas las",
        "// formas son seguras: en Alola un Vulpix ES de Alola siempre, pero",
        "// HISUI no es una region del juego — es la Sinnoh antigua — y sus",
        "// formas salen alli solo de vez en cuando. Sin la tirada, en",
        "// Sinnoh no habria un solo Growlithe normal.",
        "//",
        "// Las probabilidades se ACUMULAN porque una region puede traer",
        "// varias variedades del mismo bicho: Paldea tiene tres Tauros, a",
        "// 33/33/34. Volviendo en la primera que casaba salia siempre el",
        "// mismo y los otros dos no existian para el jugador.",
        "//",
        "// Quien no quiera tirar (la ficha, los tests) pasa 0 y se lleva la",
        "// primera forma que exista.",
        "static inline uint8_t formForRegion(int16_t dex, uint8_t region,",
        "                                    uint8_t roll = 0) {",
        "  uint16_t acc = 0;",
        "  for (uint16_t i = 0; i < FORM_COUNT; i++) {",
        "    if (FORM_TBL[i].base != dex || FORM_TBL[i].region != region)",
        "      continue;",
        "    acc += FORM_TBL[i].chance;",
        "    if (roll < acc) return FORM_TBL[i].form;",
        "  }",
        "  return 0;",
        "}",
        "",
        "// Al evolucionar se CONSERVA la forma si el destino la tiene: asi",
        "// un Vulpix de Alola da un Ninetales de Alola sin una tabla de",
        "// parejas. Si no la tiene, vuelve a la normal.",
        "static inline uint8_t formAfterEvolve(int16_t to, uint8_t form) {",
        "  return formRow(to, form) >= 0 ? form : 0;",
        "}",
        "",
        "// El nombre que hay que PINTAR: el de la forma si la tiene, y si",
        "// no el de la especie en el idioma activo. Un sitio solo, para",
        "// que no haya pantallas que digan \"VULPIX\" sobre un sprite de",
        "// Alola. (El de la mega lo resuelve battle_ui, que es el unico",
        "// sitio donde una mega existe.)",
        "static inline const char *dexNameForm(int16_t dex, uint8_t form) {",
        "  int16_t f = formRow(dex, form);",
        "  return f >= 0 ? FORM_TBL[f].name : dexName(dex);",
        "}",
        "",
        "// Los PS contando la forma. Vive AQUI y no en dex.h porque forms.h",
        "// necesita los T_* y el DEX_COUNT de dex.h: si dex.h mirara",
        "// FORM_TBL, los dos se incluirian en circulo.",
        "static inline uint16_t hpAtLevelForm(int16_t dex, uint16_t level,",
        "                                     uint8_t trHp, uint8_t form) {",
        "  int16_t f = formRow(dex, form);",
        "  if (f < 0) return hpAtLevel(dex, level, trHp);",
        "  return hpAtLevelBase(FORM_TBL[f].bHp, level, trHp);",
        "}",
        "",
        "// El stat BASE de una especie contando la forma. `cual` va en el",
        "// orden de BASE_STATS: 0 PS, 1 Atq, 2 Def, 3 AtqEsp, 4 DefEsp, 5 Vel.",
        "static inline uint16_t baseStatForm(int16_t dex, uint8_t form,",
        "                                    uint8_t cual) {",
        "  int16_t f = formRow(dex, form);",
        "  if (f >= 0) {",
        "    const FormVariant &v = FORM_TBL[f];",
        "    switch (cual) {",
        "      case 0: return v.bHp;",
        "      case 1: return v.bAtk;",
        "      case 2: return v.bDef;",
        "      case 3: return v.bSpA;",
        "      case 4: return v.bSpD;",
        "      default: return v.bSpe;",
        "    }",
        "  }",
        "  if (dex < 1 || dex > DEX_COUNT) return 1;",
        "  const DexEntry &d = DEX_TBL[dex];",
        "  switch (cual) {",
        "    case 0: return d.bHp;",
        "    case 1: return d.bAtk;",
        "    case 2: return d.bDef;",
        "    case 3: return d.bSpA;",
        "    case 4: return d.bSpD;",
        "    default: return d.bSpe;",
        "  }",
        "}",
        "",
    ]

    io.open(DST, "w", encoding="utf-8", newline="\n").write("\n".join(out) + "\n")
    print(f"{len(filas)} formas -> {os.path.normpath(DST)}")
    if repartidas:
        print(f"   {repartidas} con la probabilidad repartida (varias "
              f"variedades del mismo bicho en la misma region)")
    porReg = {}
    for f in filas:
        porReg[f[3]] = porReg.get(f[3], 0) + 1
    for r, n in sorted(porReg.items()):
        nom = "sin region (las de mega)" if r == SIN_REGION else REGIONES[r][0]
        print(f"   {n:3d}  {nom}")
    print(f"\n{saltadas} formas fuera (cosmeticas, de estado de combate, "
          f"los tipos de Arceus y Silvally)")
    if faltaHab:
        print("habilidades que no estan en abilities.h: "
              + ", ".join(sorted(faltaHab)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
