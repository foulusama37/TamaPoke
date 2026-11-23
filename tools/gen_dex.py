#!/usr/bin/env python3
"""Genera dex.h (tabla de la Pokedex para el firmware) desde dex_data.py.

  python3 tools/gen_dex.py
"""
import io
import os
import re
import sys

sys.path.insert(0, os.path.dirname(__file__))
from dex_data import (DEX, TYPE_ACCENTS, CLASSIC, RARE, LEGENDARY,
                      ALSO_WILD, EVO_ONLY)
from dex_range import N_DEX, REGIONES
from dex_stats import BASE_STATS
from dex_names import LOCAL_NAMES
from dex_sizes import ALTURA_DM, PANTALLA_PX
from dex_types import TIPOS as TIPOS_ESPECIE
from type_chart import COLORES, ETIQUETAS, TIPOS as ORDEN_TIPOS, matriz
from gen_evos import lee_pbs, solo_objeto

# Metodos del PBS que son una RAMA de la evolucion por nivel, y con que
# condicion se toma cada una.
#
# Los que NO estan aqui es porque ya los guarda `evolvesTo`, que es la
# rama por omision: "Silcoon" (Wurmple), "Ninjask" (Nincada) y
# "AtkDefEqual" (Tyrogue -> Hitmontop). Meterlos tambien daria dos
# caminos para lo mismo.
COND_RAMA = {
    "AttackGreater":  "RAMA_ATK_MAYOR",
    "DefenseGreater": "RAMA_DEF_MAYOR",
    "Cascoon":        "RAMA_AZAR",
    "Shedinja":       "RAMA_EXTRA",
}


def rgb565(hexcol):
    r, g, b = int(hexcol[1:3], 16), int(hexcol[3:5], 16), int(hexcol[5:7], 16)
    return (r >> 3) << 11 | (g >> 2) << 5 | (b >> 3)


# bioma de fondo por tipo (la luz la pone la hora real del RTC)
# 0 PRADERA, 1 PLAYA, 2 BOSQUE, 3 VOLCAN, 4 MONTANA, 5 NIEVE
TYPE_BIOME = {
    'agua': 1, 'planta': 2, 'bicho': 2, 'fuego': 3,
    'roca': 4, 'tierra': 4, 'dragon': 1, 'hielo': 5,  # los dragones gen1 (Dratini) viven en el agua
    'normal': 0, 'electrico': 0, 'lucha': 0, 'veneno': 0,
    'psiquico': 0, 'fantasma': 0,
    # Johto. El acero sale de la montana (Steelix es un Onix y Skarmory
    # anida en acantilados) y los siniestros y las hadas, de pradera.
    'acero': 4, 'siniestro': 0, 'hada': 0,
}

# excepciones por dex# (el tipo no basta)
BIOME_OVERRIDE = {
    138: 1, 139: 1, 140: 1, 141: 1,   # fosiles marinos roca/agua -> playa
    # Johto
    185: 2,   # Sudowoodo es un arbol: bosque, no montana
    191: 0, 192: 0,   # Sunkern y Sunflora son de campo abierto, no de bosque
    204: 2, 205: 2,   # Pineco y Forretress cuelgan de los arboles
    213: 4,   # Shuckle vive entre rocas
    220: 5, 221: 5,   # Swinub y Piloswine ya son de hielo, pero por si acaso
    222: 1, 226: 1,   # Corsola y Mantine, coral y mar abierto -> playa
    246: 4, 247: 4, 248: 4,  # la linea de Larvitar sale de la montana
}


def parte_fija_del_formato(sid="S_NAME_FMT"):
    """Bytes que ocupa `sid` sin contar sus %s/%u, en el peor idioma.

    Se saca igual que en test/test_tools.py: los IDs del enum de i18n.h
    (quitando los comentarios antes, que si mencionan un S_ALGO descuadran
    la tabla) y las filas de i18n.cpp.
    """
    import test_i18n_formats as fmt
    raiz = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

    def leer(n):
        return io.open(os.path.join(raiz, n), encoding="utf-8").read()

    cuerpo = re.search(r"enum\s+StrId[^{]*\{(.*?)\}", leer("i18n.h"), re.S).group(1)
    ids = re.findall(r"\b(S_[A-Z0-9_]+)\b", re.sub(r"//[^\n]*", "", cuerpo))
    filas = fmt.extract_table(leer("i18n.cpp"), "STRINGS", len(fmt.LANGS))
    if len(filas[0]) != len(ids) or sid not in ids:
        raise SystemExit(f"no cuadro {sid} con i18n.cpp; "
                         f"{len(ids)} ids y {len(filas[0])} columnas")
    k = ids.index(sid)
    return max(len(re.sub(r"%[a-z]", "", f[k][1:-1]).encode("utf-8"))
               for f in filas)


def ramas_de_eevee():
    """Los destinos de Eevee segun el PBS, dentro de nuestra dex."""
    import anil_pbs
    num = anil_pbs.numeros()
    c = anil_pbs.filas(133)[132][2]
    campos = [x.strip() for x in c.get("Evolutions", "").split(",")]
    out = []
    for i in range(0, max(0, len(campos) - 2), 3):
        d = num.get(anil_pbs.clave(campos[i])) if campos[i] else None
        if d and d <= N_DEX and d not in out:
            out.append(d)
    return sorted(out)


def main():
    ramas_eevee = ramas_de_eevee()
    out = []
    out.append("#pragma once\n#include <stdint.h>\n#include \"i18n.h\"  // gLang\n\n")
    out.append("// GENERADO por tools/gen_dex.py desde tools/dex_data.py - no editar\n\n")
    assert len(DEX) == N_DEX, f'dex_data tiene {len(DEX)} filas y N_DEX dice {N_DEX}'
    assert [d[0] for d in DEX] == list(range(1, N_DEX + 1)), 'faltan o sobran numeros de dex'
    out.append(f"#define DEX_COUNT {N_DEX}\n")
    # Donde acaba cada region. Era un DEX_KANTO 151 escrito a mano aqui, que
    # con la segunda region ya mentia por omision y con la tercera habria
    # hecho falta un DEX_JOHTO al lado. Sale de dex_range.REGIONES.
    out.append(f"#define DEX_REGIONS {len(REGIONES)}\n")
    out.append("static const int16_t DEX_REGION_END[DEX_REGIONS] = { " +
               ", ".join(str(f) for _, f in REGIONES) + " };\n")
    out.append("#define DEX_KANTO " + str(REGIONES[0][1]) +
               "  // compatibilidad: donde acaba la primera\n")
    out.append(
        "// En que region cae una especie (0 = la primera). Para el techo de\n"
        "// nivel y para que la hierba de cada region saque lo suyo.\n"
        "static inline uint8_t dexRegionOf(int16_t dex) {\n"
        "  for (uint8_t r = 0; r < DEX_REGIONS; r++)\n"
        "    if (dex <= DEX_REGION_END[r]) return r;\n"
        "  return DEX_REGIONS - 1;\n"
        "}\n")
    out.append("// Bytes del bitmap de \"ya lo criaste\". Sale de DEX_COUNT para que\n"
               "// anadir una region no deje el mapa corto en silencio.\n"
               "#define DEX_REG_BYTES ((DEX_COUNT + 7) / 8)\n")
    # Cuanto ocupa la cabecera "%s%s Lv.%u" de la ficha en el PEOR idioma.
    # Estaba a mano en TRES sitios con TRES valores (26 en el test, 28 en
    # la pantalla de casa, 32 en la ficha) y con Kalos los nombres
    # japoneses de seis silabas (18 bytes en UTF-8) pasaron de 26: el
    # nombre se habria cortado en pantalla y solo lo vio el test.
    masLargo = max(
        [len(n.encode("utf-8")) for por in LOCAL_NAMES.values()
         for n in por.values()] +
        [len(d[2]) for d in DEX] + [12])      # 12 = el apodo
    fijo = parte_fija_del_formato()
    out.append(
        "// Para la cabecera \"%s%s Lv.%u\" de la ficha: el nombre mas largo\n"
        f"// de cualquier idioma ({masLargo} bytes en UTF-8), mas el prefijo,\n"
        f"// la parte fija del formato en el peor idioma ({fijo} bytes), el\n"
        "// nivel y el NUL. Sale de aqui para que los sitios que la pintan\n"
        "// no puedan quedarse cortos al entrar una generacion con nombres\n"
        "// mas largos.\n"
        "//\n"
        "// El nivel cuenta CINCO cifras y no tres: es la misma cuenta que\n"
        "// hace el test de buffers (test/test_tools.py), que da por hecho\n"
        "// que un %u puede traer un uint16_t entero. Dos cuentas distintas\n"
        "// para el mismo buffer es justo lo que este #define vino a evitar.\n"
        "//\n"
        "// La parte fija se MIDE y no se supone: se dio por hecho que eran\n"
        "// los cuatro bytes de \" Lv.\" en los ocho idiomas, y en japones y\n"
        "// coreano son mas, asi que el buffer se quedaba corto justo en los\n"
        "// dos idiomas donde los nombres ya son los mas largos.\n"
        f"#define DEX_HEAD_BUF {masLargo + 1 + fijo + 5 + 1}\n")
    # Las que se parten POR SEXO. En el Anil es solo Burmy (hembra ->
    # Wormadam, macho -> Mothim), pero sale del PBS y no escrita a mano:
    # si manana el Anil le hace lo mismo a otro, entra sola.
    #
    # El campo `evolvesTo` guarda un destino, asi que ahi esta el de la
    # HEMBRA y aqui el del macho. Sin esto, Mothim no salia de ningun
    # sitio y se tapaba poniendolo en la hierba.
    sexadas = []
    ramas = []        # (desde, condicion, destino)
    # Silcoon/Ninjask NO entran: son el destino que ya guarda
    # `evolvesTo`, o sea la rama por omision. Aqui solo van las OTRAS.
    # AtkDefEqual tampoco, por lo mismo (Tyrogue va a Hitmontop).
    try:
        for d, lista in lee_pbs().items():
            for destino, metodo, arg in lista:
                if destino > N_DEX:
                    continue
                if metodo == "LevelMale":
                    sexadas.append((d, destino))
                elif metodo in COND_RAMA:
                    ramas.append((d, COND_RAMA[metodo], destino))
    except OSError:
        pass
    out.append(
        "// Las que se parten por SEXO: el macho va a otra cosa que la\n"
        "// hembra, y `evolvesTo` solo guarda una (la de la hembra).\n"
        f"#define DEX_SEXADAS {len(sexadas)}\n"
        "struct DexSexada { int16_t from, male; };\n"
        "static const DexSexada DEX_SEXADA[DEX_SEXADAS ? DEX_SEXADAS : 1] = { "
        + ", ".join(f"{{ {a}, {b} }}" for a, b in sorted(sexadas))
        + (" " if sexadas else "{ 0, 0 } ") + "};\n"
        "// En que evoluciona un MACHO de esta especie, o 0 si da igual.\n"
        "static inline int16_t dexMaleBranch(int16_t dex) {\n"
        "  for (uint8_t i = 0; i < DEX_SEXADAS; i++)\n"
        "    if (DEX_SEXADA[i].from == dex) return DEX_SEXADA[i].male;\n"
        "  return 0;\n"
        "}\n\n")

    # Las OTRAS ramas de nivel: las que no decide el sexo ni un objeto.
    #
    # `evolvesTo` guarda UNA, asi que las demas no existian y el bicho
    # del otro lado solo salia en la hierba. Son pocas y cada una tiene
    # su regla, que es la de los juegos:
    #
    #   Tyrogue   mira su Ataque contra su Defensa
    #   Wurmple   lo echa a suertes, pero UNA VEZ: la tirada sale de los
    #             genes del bicho, que no cambian, para que mirar la
    #             ficha dos veces no de dos respuestas
    #   Nincada   da Ninjask Y ADEMAS deja un Shedinja, si hay hueco en
    #             el equipo (como en los juegos, que pedia una Poke Ball
    #             libre)
    out.append(
        "// Ramas de nivel que no decide ni el sexo ni un objeto. Salen\n"
        "// del PBS: `evolvesTo` solo guarda una y el resto se perdian.\n"
        "enum : uint8_t {\n"
        "  RAMA_ATK_MAYOR = 0,  // Tyrogue con mas Ataque que Defensa\n"
        "  RAMA_DEF_MAYOR,      // ...al reves\n"
        "  RAMA_IGUALES,        // ...empatados\n"
        "  RAMA_AZAR,           // Wurmple: a suertes, pero fijo por bicho\n"
        "  RAMA_EXTRA,          // Nincada: ADEMAS de la normal (Shedinja)\n"
        "};\n"
        "struct EvoRama { int16_t from; uint8_t cond; int16_t to; };\n"
        f"#define EVO_RAMAS {len(ramas)}\n"
        "static const EvoRama EVO_RAMA[EVO_RAMAS ? EVO_RAMAS : 1] = {\n"
        + "".join(f"  {{ {a:4d}, {c:<14s}, {b:4d} }},\n"
                  for a, c, b in sorted(ramas))
        + ("" if ramas else "  { 0, RAMA_AZAR, 0 },\n")
        + "};\n\n")
    # Lo que lleva encima un SALVAJE. El Anil lo trae por especie
    # (WildItemCommon/Uncommon/Rare, en 366 de las 1025) y hasta ahora
    # no se usaba: el comentario de battle_ui decia "un salvaje nunca
    # lleva nada", y con eso Frisk, Pickpocket y Anticipation solo
    # valian contra un lider de gimnasio.
    #
    # Las probabilidades son las de los juegos: 50 / 5 / 1 por ciento.
    # Solo entran los objetos que existen en NUESTRA bolsa (211 de los
    # 889 del PBS); el resto se queda a 0, que es "nada".
    import anil_pbs
    from gen_items import indices as item_idx
    idx = item_idx()
    wild = {}
    sinObjeto = set()
    try:
        for d, _slug, c in anil_pbs.filas():
            tres = []
            for campo in ("WildItemCommon", "WildItemUncommon",
                          "WildItemRare"):
                k = (c.get(campo) or "").strip().upper()
                if not k:
                    tres.append(0)
                    continue
                if k not in idx:
                    sinObjeto.add(k)
                    tres.append(0)
                    continue
                tres.append(idx[k])
            if any(tres):
                wild[d] = tres
    except OSError:
        pass
    out.append(
        "// Lo que lleva encima un SALVAJE, por especie. Sale del PBS\n"
        "// (WildItemCommon/Uncommon/Rare) y las probabilidades son las\n"
        "// de los juegos: 50%, 5% y 1%. Un 0 es \"nada\".\n"
        "//\n"
        "// Sin esto, Frisk, Pickpocket, Anticipation, Ladron y Desarme\n"
        "// solo tenian con que trabajar frente a un lider de gimnasio.\n"
        "struct WildItem { int16_t dex; uint8_t comun, raro, rarisimo; };\n"
        f"#define WILD_ITEM_COUNT {len(wild)}\n"
        "static const WildItem WILD_ITEM[WILD_ITEM_COUNT ? WILD_ITEM_COUNT : 1] = {\n"
        + "".join(f"  {{ {d:4d}, {a:3d}, {b:3d}, {c:3d} }},\n"
                  for d, (a, b, c) in sorted(wild.items()))
        + ("" if wild else "  { 0, 0, 0, 0 },\n")
        + "};\n\n"
        "// Tira el dado: que lleva encima ESTE salvaje (0 = nada).\n"
        "static inline uint8_t rollWildItem(int16_t dex) {\n"
        "  for (uint16_t i = 0; i < WILD_ITEM_COUNT; i++) {\n"
        "    if (WILD_ITEM[i].dex != dex) continue;\n"
        "    uint8_t t = (uint8_t)random(100);\n"
        "    if (t < 1 && WILD_ITEM[i].rarisimo) return WILD_ITEM[i].rarisimo;\n"
        "    if (t < 6 && WILD_ITEM[i].raro) return WILD_ITEM[i].raro;\n"
        "    if (t < 56) return WILD_ITEM[i].comun;\n"
        "    return 0;\n"
        "  }\n"
        "  return 0;\n"
        "}\n\n")
    print(f"objetos de salvaje: {len(wild)} especies llevan algo; "
          f"{len(sinObjeto)} claves del PBS no estan en nuestra bolsa")

    out.append("#define DEX_EEVEE 133\n")
    out.append(
        "// Las ramas de Eevee. Van en una tabla y no como un rango 134..136\n"
        "// porque con Johto dejaron de ser seguidas: Espeon y Umbreon estan\n"
        "// en los 190 y con Sinnoh llegaron Leafeon y Glaceon en los 470.\n"
        "// Quien recorra la linea evolutiva tiene que mirar aqui.\n"
        "//\n"
        "// La lista sale del PBS del Anil y NO esta escrita a mano: estaba, y\n"
        "// se quedo en cinco cuando entraron las dos de Sinnoh.\n"
        f"static const int16_t EEVEE_BRANCH[] = {{ {', '.join(str(d) for d in ramas_eevee)} }};\n"
        "#define EEVEE_BRANCHES (sizeof(EEVEE_BRANCH) / sizeof(EEVEE_BRANCH[0]))\n\n")
    # --- tipos de combate ---------------------------------------------------
    C_TIPO = {t: 'T_' + t.upper() for t in ORDEN_TIPOS}
    C_TIPO['fighting'] = 'T_FIGHT'
    C_TIPO['electric'] = 'T_ELEC'
    out.append(
        "// Tipos de GEN 1 para el combate. OJO: el campo 'tipo' de dex_data.py\n"
        "// NO sirve para esto: son 14, no existe volador y cada especie tiene uno\n"
        "// solo, porque se uso para elegir color de UI y bioma de fondo. Estos\n"
        "// salen de PokeAPI con la tipificacion de gen 1 (gen_types.py).\n"
        "enum : uint8_t {\n"
        "  " + ", ".join(C_TIPO[t] for t in ORDEN_TIPOS) + ", T_COUNT\n"
        "};\n\n")
    out.append(
        "// Efectividad x100 (0, 50, 100, 200) de [atacante][defensor].\n"
        "// Se conservan las rarezas de gen 1 (veneno<->bicho x2, hielo->fuego x1),\n"
        "// pero NO el bug de fantasma contra psiquico, que hacia cero y dejaba a\n"
        "// los psiquicos intocables. Ver tools/type_chart.py para cambiarlo.\n"
        "static const uint8_t TYPE_CHART[T_COUNT][T_COUNT] = {\n")
    m = matriz()
    for i, fila in enumerate(m):
        out.append("  { " + ", ".join(f"{v:3d}" for v in fila) + f" }},  // {ORDEN_TIPOS[i]}\n")
    out.append("};\n\n")
    out.append(
        "// Multiplicador x100 de un golpe de tipo `atk` contra un bicho de tipos\n"
        "// d1/d2. Con dos tipos se multiplican, asi que sale 0, 25, 50, 100, 200 o 400.\n"
        "static inline uint16_t typeEffect(uint8_t atk, uint8_t d1, uint8_t d2) {\n"
        "  if (atk >= T_COUNT || d1 >= T_COUNT) return 100;\n"
        "  uint16_t e = TYPE_CHART[atk][d1];\n"
        "  if (d2 && d2 < T_COUNT) e = (uint16_t)(e * TYPE_CHART[atk][d2] / 100);\n"
        "  return e;\n"
        "}\n\n")
    out.append(
        "// Etiqueta corta y color de cada tipo, para el chip de la tarjeta de\n"
        "// golpe. Cortas a proposito: la tarjeta mide 257 px y ya lleva flechas,\n"
        "// asi que FIGHTING no cabe pero FIGHT si.\n"
        "static const char *const TYPE_NAME[T_COUNT] = {\n")
    for t in ORDEN_TIPOS:
        out.append(f'  "{ETIQUETAS[t]}",\n')
    out.append("};\n")
    out.append("static const uint16_t TYPE_COLOR[T_COUNT] = {\n")
    for t in ORDEN_TIPOS:
        out.append(f'  0x{rgb565(COLORES[t]):04X},  // {t}\n')
    out.append("};\n")
    # O nome do tipo NA TELA, por idioma (tools/gen_type_names.py). Ingles
    # segue com as etiquetas curtas de cima; os outros, o nome oficial.
    from type_names import TYPE_NAMES
    ling = ["ES", "EN", "FR", "DE", "IT", "PT", "JA", "KO"]
    if TYPE_NAMES["EN"] != [ETIQUETAS[t] for t in ORDEN_TIPOS]:
        raise SystemExit("type_names.py EN difere das ETIQUETAS: rode gen_type_names.py")
    out.append(
        "// por idioma, na ordem do enum Lang (leva 77: o quiz perguntava em\n"
        "// espanhol e respondia FIRE)\n"
        "static_assert(LANG_ES == 0 && LANG_EN == 1 && LANG_FR == 2 && LANG_DE == 3 &&\n"
        "              LANG_IT == 4 && LANG_PT == 5 && LANG_JA == 6 && LANG_KO == 7,\n"
        "              \"TYPE_NAME_L segue a ordem do enum Lang\");\n"
        "static const char *const TYPE_NAME_L[LANG_COUNT][T_COUNT] = {\n")
    from cp437_up import literal_c
    for lg in ling:
        lit = (lambda s: '"%s"' % s) if lg in ("JA", "KO") else literal_c
        out.append("  { " + ", ".join(lit(s) for s in TYPE_NAMES[lg]) + " },  // " + lg + "\n")
    out.append("};\n")
    out.append(
        "static inline const char *typeName(uint8_t t) {\n"
        "  const uint8_t l = gLang < LANG_COUNT ? (uint8_t)gLang : (uint8_t)LANG_EN;\n"
        "  return TYPE_NAME_L[l][t < T_COUNT ? t : T_NONE];\n"
        "}\n"
        "static inline uint16_t typeColor(uint8_t t) {\n"
        "  return t < T_COUNT ? TYPE_COLOR[t] : TYPE_COLOR[T_NONE];\n"
        "}\n\n")

    out.append(
        "// rareza: 0 = solo por evolucion, 1 = comun, 2 = raro, 3 = legendario\n"
        "enum : uint8_t { R_EVO = 0, R_COMUN, R_RARO, R_LEGENDARIO };\n\n"
        "struct DexEntry {\n"
        "  const char *name;\n"
        "  // uint16_t y no uint8_t: el numero de dex se pasa de 255 en\n"
        "  // cuanto entre la gen 2, y entonces las evoluciones altas\n"
        "  // apuntarian a cualquier sitio en silencio. Cuesta 151 bytes de\n"
        "  // flash y quita un bloqueo de encima.\n"
        "  uint16_t evolvesTo;   // numero de dex, 0 = forma final\n"
        "  uint8_t evolveLevel;\n"
        "  uint8_t rarity;       // sale de huevo si > 0\n"
        "  uint16_t accent;      // color RGB565 del tipo para la UI\n"
        "  // los SEIS stats base. Antes solo habia cuatro (faltaban los especiales)\n"
        "  // y sin ellos no se puede calcular el dano de medio catalogo de golpes.\n"
        "  uint8_t bHp, bAtk, bDef, bSpA, bSpD, bSpe;\n"
        "  uint8_t biome;        // 0 pradera 1 playa 2 bosque 3 volcan 4 montana 5 nieve\n"
        "  // Alto en pantalla, segun la altura real Y LA DENSIDAD del sprite\n"
        "  // (gen_sizes.py). uint16_t y no uint8_t: la correccion por densidad\n"
        "  // tiene que pasar de 255 en los sprites muy vacios. Pidgeotto esta\n"
        "  // dibujado VOLANDO —— 84x116 de lienzo con solo el 30% de tinta ——\n"
        "  // y necesita 270 para que su CABEZA se vea del tamano de la de\n"
        "  // Pidgey; con el byte se topaba en 255 justo donde hacia falta.\n"
        "  // Son 1025 bytes mas de flash sobre 3 MB.\n"
        "  uint16_t screenSz;\n"
        "  // El PESO, en decenas de kilo (asi un Groudon de 950 kg cabe\n"
        "  // en un byte). Lo piden dos golpes y no se podian hacer sin\n"
        "  // el: Patada Baja pega segun lo que pese el rival y Cuerpo\n"
        "  // Pesado segun cuanto mas peses tu. Sale del PBS.\n"
        "  uint8_t weight10;\n"
        "  uint8_t type1, type2; // tipos de GEN 1 para combatir; type2 = T_NONE si es puro\n"
        "  // Probabilidad de salir HEMBRA, en %. 255 = SIN SEXO, que no es\n"
        "  // lo mismo que 0 (0 es \"siempre macho\"). Sale del GenderRatio\n"
        "  // del Anil. Hace falta para las evoluciones que piden sexo\n"
        "  // (Snorunt da Froslass solo si es hembra, Burmy se parte en\n"
        "  // Wormadam y Mothim) y para elegir el sprite de hembra.\n"
        "  uint8_t femPct;\n"
        "};\n\n"
        "// Sin sexo: ni macho ni hembra. Son 155 (Magnemite, Voltorb, los\n"
        "// legendarios...), asi que no es un caso raro.\n"
        "#define GENDERLESS 255\n\n")
    # Formas base = las que no son evolucion de nadie. Las ramas de Eevee si
    # lo son aunque nadie las apunte (`evolvesTo` guarda un solo destino), y
    # eso mismo le pasa a las segundas ramas de Johto: van en EVO_ONLY.
    # ALSO_WILD es el caso contrario, y gana: con los bebes de Johto, Pikachu
    # pasaria a ser "evolucion de Pichu" y desapareceria de la hierba.
    evolved = ({d[4] for d in DEX if d[4]} | set(ramas_eevee) | EVO_ONLY) - ALSO_WILD
    out.append(
        "// Los que son evolucion de alguien Y ADEMAS salen por su cuenta.\n"
        "// Es una decision, no un descuido: en los juegos tampoco hay que\n"
        "// criar un Pichu para encontrarse un Pikachu, ni un Happiny para\n"
        "// ver una Chansey. Se emite para que los tests puedan distinguir\n"
        "// esto de un agujero de verdad (ALSO_WILD en dex_data.py).\n"
        f"#define DEX_ALSO_WILD_COUNT {len(ALSO_WILD)}\n"
        "static const int16_t DEX_ALSO_WILD[DEX_ALSO_WILD_COUNT] = { "
        + ", ".join(str(d) for d in sorted(ALSO_WILD)) + " };\n"
        "static inline bool dexAlsoWild(int16_t dex) {\n"
        "  for (uint8_t i = 0; i < DEX_ALSO_WILD_COUNT; i++)\n"
        "    if (DEX_ALSO_WILD[i] == dex) return true;\n"
        "  return false;\n"
        "}\n\n")
    assert {196, 197} <= evolved, 'las ramas nuevas de Eevee tienen que ser solo-evolucion'
    # El nivel de evolucion sale del PBS del Anil, no de dex_data.py. Estaba
    # a mano y con cifras inventadas: Onix a 30 cuando el Anil dice 42,
    # Kadabra a 40 cuando dice 45, y Eevee con un nivel que no tiene — el
    # Anil solo le da objetos y amistad, asi que subia solo y la piedra no
    # pintaba nada. `solos` son los que no tienen NINGUN camino por nivel y
    # por eso se quedan a 0.
    # El reparto de sexo, tambien del PBS. Si no se puede leer, todo el
    # mundo a 50/50, que es el caso de 722 de las 1025.
    import anil_pbs
    try:
        fem = {n: anil_pbs.fem_pct(c) for n, _s, c in anil_pbs.filas()}
    except Exception:
        fem = {d[0]: 50 for d in DEX}

    nivel_pbs, solos = {}, set()
    try:
        pbs = lee_pbs()
        solos = solo_objeto(pbs)
        for d, lista in pbs.items():
            for destino, metodo, arg in lista:
                if metodo == "Level" and arg.isdigit():
                    nivel_pbs[(d, destino)] = int(arg)
    except OSError as e:
        print(f"AVISO: sin el PBS del Anil ({e.strerror}); los niveles de\n"
              "       evolucion se quedan como esten en dex_data.py.")

    rarities = []
    # El PESO, en decenas de kilo. Lo piden Patada Baja (pega segun lo
    # que pese el rival) y Cuerpo Pesado (segun cuanto mas peses tu), y
    # sin el no se podian hacer. Groudon pesa 950 kg = 95, asi que cabe
    # en un byte de sobra.
    peso = {}
    try:
        for d, _slug, c in anil_pbs.filas():
            kg = float(c.get("Weight", "1") or 1)
            v = int(round(kg / 10.0))
            peso[d] = 1 if v < 1 else (255 if v > 255 else v)
    except (OSError, ValueError):
        pass

    out.append("static const DexEntry DEX_TBL[DEX_COUNT + 1] = {\n")
    out.append('  { "?", 0, 0, 0, 0x2946, 50, 50, 50, 50, 50, 50, 0, 120, 1, T_NONE, T_NONE, 255 },  // 0: sin usar (255 = GENDERLESS)\n')
    cambios = []
    for num, slug, display, typ, evo, lvl in DEX:
        if evo:
            nuevo = 0 if num in solos else nivel_pbs.get((num, evo), lvl)
            if nuevo != lvl:
                cambios.append((display, lvl, nuevo))
                lvl = nuevo
        acc = rgb565(TYPE_ACCENTS[typ])
        if num in evolved:
            rar = 'R_EVO'
        elif num in LEGENDARY:
            rar = 'R_LEGENDARIO'
        elif num in RARE:
            rar = 'R_RARO'
        else:
            rar = 'R_COMUN'
        rarities.append(rar)
        hp, atk, df, spa, spd, vel = BASE_STATS[num]
        bio = BIOME_OVERRIDE.get(num, TYPE_BIOME[typ])
        out.append(f'  {{ "{display}", {evo}, {lvl}, {rar}, 0x{acc:04X}, {hp}, {atk}, {df}, {spa}, {spd}, {vel}, {bio}, {PANTALLA_PX[num]}, {peso.get(num, 1)}, {C_TIPO[TIPOS_ESPECIE[num][0]]}, {C_TIPO[TIPOS_ESPECIE[num][1]] if TIPOS_ESPECIE[num][1] else "T_NONE"}, {fem[num]} }},  // {num} {typ} {ALTURA_DM[num]/10:.1f}m\n')
    out.append("};\n\n")
    out.append(
        "static inline bool dexGenderless(int16_t dex) {\n"
        "  return dex >= 1 && dex <= DEX_COUNT &&\n"
        "         DEX_TBL[dex].femPct == GENDERLESS;\n"
        "}\n\n")
    out.append(
        "// PS a un nivel. UNA sola cuenta para los CUATRO sitios que la hacian\n"
        "// por su cuenta (Combatant::init, Pet::maxHp, la fila del equipo y la\n"
        "// barra del combate), que ya se habian despegado: la del combate se\n"
        "// olvidaba del entrenamiento.\n"
        "//\n"
        "// Era `bHp * 2 + nivel * 2 + 10`, que escala con la BASE y casi nada\n"
        "// con el NIVEL. El dano si escala con el nivel, asi que las dos cosas\n"
        "// iban en escalas distintas: a nivel 11 un Snorlax tenia 352 PS cuando\n"
        "// en los juegos tiene 59, y tumbar a cualquiera costaba de 3 a 6 veces\n"
        "// mas turnos. Por eso hasta un golpe muy eficaz parecia hacer\n"
        "// cosquillas: el dano estaba BIEN, el deposito estaba inflado.\n"
        "//\n"
        "// Ahora es la de los juegos, con nuestro entrenamiento (trHp, 0..100)\n"
        "// en el sitio de los EVs. Al bajar el tope, `Pet::hpSync()` recorta lo\n"
        "// que sobre, asi que una partida vieja entra sin tocar nada.\n"
        "// La cuenta, a partir del PS BASE. Existe aparte porque una\n"
        "// megaevolucion no tiene numero de dex y su bHp sale de MEGA_TBL:\n"
        "// sin esto habria que copiar la formula en battle.cpp, que es\n"
        "// justo lo que este fichero vino a evitar.\n"
        "// bHp en uint16_t: una mega puede pasar de 255 (el Mewtwo\n"
        "// Armadura del Anil tiene 400 de PS base).\n"
        "static inline uint16_t hpAtLevelBase(uint16_t bHp, uint16_t level,\n"
        "                                     uint8_t trHp) {\n"
        "  uint32_t v = ((uint32_t)bHp * 2 + trHp / 4) * level / 100\n"
        "             + level + 10;\n"
        "  return (uint16_t)(v > 65535 ? 65535 : (v ? v : 1));\n"
        "}\n\n"
        "// Para una FORMA (un Meowth de Galar no tiene los mismos PS base)\n"
        "// esta hpAtLevelForm() en forms.h. No puede estar aqui: forms.h\n"
        "// necesita los T_* y el DEX_COUNT de este fichero, asi que si este\n"
        "// mirara FORM_TBL los dos se incluirian en circulo.\n"
        "static inline uint16_t hpAtLevel(int16_t dex, uint16_t level, uint8_t trHp) {\n"
        "  if (dex < 1 || dex > DEX_COUNT) return 1;\n"
        "  return hpAtLevelBase(DEX_TBL[dex].bHp, level, trHp);\n"
        "}\n\n"
        "// Los otros CINCO stats, tambien como en los juegos. Estaba duplicada\n"
        "// en battle.cpp y pet.cpp (las dos `calcStat`, identicas) y era\n"
        "// `base * gene / 100 + nivel + tr`: casi no crecia con el nivel. Un\n"
        "// Charizard tenia Ataque 85 a nivel 1 y 134 a nivel 50, cuando en los\n"
        "// juegos son 7 y 104 — subir de nivel apenas se notaba en la ficha.\n"
        "//\n"
        "// No se veia en el dano porque A/D se cancela: atacante y defensor\n"
        "// usaban la misma cuenta mala. Lo que arregla esto es el TACTO de\n"
        "// subir de nivel y que los numeros de la ficha signifiquen algo.\n"
        "//\n"
        "// `gene` (90..110) hace de IV y `tr` (0..100) de EV, en los mismos\n"
        "// sitios que ocupan en la formula de verdad.\n"
        "static inline uint16_t statAtLevel(uint8_t base, uint8_t gene,\n"
        "                                   uint16_t level, uint8_t tr) {\n"
        "  uint32_t v = ((uint32_t)base * 2 * gene / 100 + tr / 4) * level / 100 + 5;\n"
        "  return (uint16_t)(v > 65535 ? 65535 : (v ? v : 1));\n"
        "}\n\n"
        "// atajo: efectividad de un golpe contra una especie de la Pokedex\n"
        "static inline uint16_t typeEffectVs(uint8_t atk, int16_t dex) {\n"
        "  if (dex < 1 || dex > DEX_COUNT) return 100;\n"
        "  return typeEffect(atk, DEX_TBL[dex].type1, DEX_TBL[dex].type2);\n"
        "}\n\n"
        "// La forma BASE de la linea evolutiva: sube por la cadena hasta el\n"
        "// que no tiene padre. Un Charizard deja un huevo de CHARMANDER, no\n"
        "// de Charizard: de la otra forma la despedida regalaba una forma\n"
        "// final de nivel 1 y el linaje se saltaba entero el criar.\n"
        "//\n"
        "// La tabla solo apunta hacia DELANTE (evolvesTo), asi que hay que\n"
        "// buscar quien apunta a este. Es un barrido de la tabla entera y se\n"
        "// llama una vez por huevo, asi que no importa.\n"
        "static inline int16_t baseFormOf(int16_t dex) {\n"
        "  if (dex < 1 || dex > DEX_COUNT) return dex;\n"
        "  for (int salto = 0; salto < 3; salto++) {  // ninguna cadena pasa de 3\n"
        "    int16_t padre = 0;\n"
        "    for (int16_t d = 1; d <= DEX_COUNT && !padre; d++)\n"
        "      if (DEX_TBL[d].evolvesTo == dex) padre = d;\n"
        "    if (!padre) break;\n"
        "    dex = padre;\n"
        "  }\n"
        "  return dex;\n"
        "}\n\n")

    # Solo FR y DE tienen nombre propio en gen 1; ES/IT/PT usan el ingles.
    out.append(
        "// Nombres oficiales por idioma. FR y DE son los unicos latinos que difieren\n"
        "// del ingles en gen 1 (ES/IT/PT usan el de DEX_TBL); JA y KO van en UTF-8,\n"
        "// que se pinta con la fuente U8g2. nullptr = sin nombre propio.\n")
    for lg in ('fr', 'de', 'ja', 'ko'):
        out.append(f"static const char *const DEX_NAME_{lg.upper()}[DEX_COUNT + 1] = {{\n")
        fila = []
        for num in range(0, N_DEX + 1):
            nm = LOCAL_NAMES.get(num, {}).get(lg)
            fila.append(f'"{nm}"' if nm else 'nullptr')
            if len(fila) == 4:
                out.append("  " + ", ".join(fila) + ",\n")
                fila = []
        if fila:
            out.append("  " + ", ".join(fila) + ",\n")
        out.append("};\n\n")
    out.append(
        "// Nombre de la especie en el idioma activo (cae al de DEX_TBL si ese\n"
        "// idioma no tiene nombre propio para ella).\n"
        "static inline const char *dexName(int16_t dex) {\n"
        "  if (dex < 1 || dex > DEX_COUNT) return DEX_TBL[0].name;\n"
        "  // Nidoran: a fonte classica (CP437) TEM femea e macho nos codigos 12\n"
        "  // e 11 (octal 014 e 013), e o Arduino_GFX so trata a quebra de linha e\n"
        "  // o retorno como controle: os dois se desenham. Nas latinas sai o\n"
        "  // simbolo em vez do H/M do espanhol; JA/KO (fontes U8g2 sem eles)\n"
        "  // ficam com as tabelas deles (F/M e am/su). DEX_TBL segue com o nome\n"
        "  // de sempre: as ferramentas leem dex.h e casam por ele.\n"
        "  if (!LANG_IS_CJK(gLang) && (dex == 29 || dex == 32))\n"
        "    return dex == 29 ? \"NIDORAN\\014\" : \"NIDORAN\\013\";\n"
        "  const char *n = (gLang == LANG_FR)   ? DEX_NAME_FR[dex]\n"
        "                  : (gLang == LANG_DE) ? DEX_NAME_DE[dex]\n"
        "                  : (gLang == LANG_JA) ? DEX_NAME_JA[dex]\n"
        "                  : (gLang == LANG_KO) ? DEX_NAME_KO[dex]\n"
        "                                       : nullptr;\n"
        "  return n ? n : DEX_TBL[dex].name;\n"
        "}\n\n")

    out.append("// el primer huevo de la partida: iniciales clasicos\n")
    out.append("static const int16_t CLASSIC_DEX[] = { %s };\n" % ", ".join(map(str, CLASSIC)))
    out.append(f"#define NUM_CLASSIC_DEX {len(CLASSIC)}\n")
    from collections import Counter
    c = Counter(rarities)
    print(f"bases: {c['R_COMUN']} comunes, {c['R_RARO']} raras, {c['R_LEGENDARIO']} legendarias, {c['R_EVO']} solo-evolucion")

    # encoding explicito: los nombres JA/KO van en UTF-8 y en Windows el
    # open() por defecto es cp1252, que revienta a medio escribir y deja el
    # dex.h truncado. En Linux (que es donde corre el CI) no se notaba.
    path = os.path.join(os.path.dirname(__file__), '..', 'dex.h')
    with open(path, 'w', encoding='utf-8') as f:
        f.write(''.join(out))
    print(f"guardado {os.path.normpath(path)} ({len(DEX)} especies)")
    if cambios:
        print(f"\n{len(cambios)} niveles de evolucion corregidos desde el PBS:")
        for nom, antes, ahora in cambios:
            print(f"   {nom:<12s} {antes:3d} -> {ahora:3d}"
                  f"{'   (solo por objeto)' if ahora == 0 else ''}")


if __name__ == '__main__':
    main()
