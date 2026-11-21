#!/usr/bin/env python3
"""Genera evos.h: que objeto pide cada evolucion, SEGUN EL ANIL.

stones.h estaba escrito a mano y se invento la mitad: un "Linking Cord"
generico para todo lo que en los juegos es intercambio, cuando el PBS del
Anil dice el objeto exacto ("Evolutions = STEELIX,TradeItem,METALCOAT").
Tambien se invento que Espeon y Umbreon salian por piedra, cuando el Anil
dice amistad de dia / de noche.

Metodos del PBS y que hacemos con cada uno:

  Item / TradeItem / DayHoldItem  -> objeto (es lo que emite este fichero).
      Los tres acaban en lo mismo aqui: usas el objeto desde la bolsa. No
      hay con quien intercambiar en un cacharro sin radio, y "llevarlo
      puesto de dia" pediria un reloj dentro de evolve().
  Trade (sin objeto)              -> el Cable (EVOI_CABLE), que es el
      sustituto del intercambio que ya existia y para esos SI vale.
  Level                           -> ya esta en DexEntry.evolvesTo/Level.
  Happiness / HappinessDay/Night  -> todavia no: haria falta meter el
      vinculo (y la hora) en evolve(). Se listan al generar para que se vea
      lo que falta, en vez de quedar callado.
  AttackGreater / DefenseGreater / AtkDefEqual (Tyrogue) -> tampoco.

  python3 tools/gen_evos.py
"""
import io
import os
import re
import sys
from collections import Counter

AQUI = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, AQUI)
import anil_pbs                   # noqa: E402
from dex_data import DEX          # noqa: E402
from dex_range import N_DEX       # noqa: E402

DST = os.path.join(os.path.dirname(AQUI), "evos.h")
ITEMS_H = os.path.join(os.path.dirname(AQUI), "items.h")

SLUG = {d[1].upper(): d[0] for d in DEX}
SLUG["NIDORANFE"] = 29
SLUG["NIDORANMA"] = 32
NOMBRE = {d[0]: d[2] for d in DEX}

# ItemFemale / ItemMale van aqui y no fuera: aqui no hay sexo, asi que el
# objeto funciona y punto. Dejarlos fuera hacia que Froslass (Snorunt +
# Piedra Alba, pero solo hembra en los juegos) no se consiguiera de ninguna
# manera, porque ademas esta en EVO_ONLY.
CON_OBJETO = ("Item", "TradeItem", "DayHoldItem", "NightHoldItem", "HoldItem",
              "ItemFemale", "ItemMale")

# Los que ADEMAS del objeto piden un sexo. Ya hay sexo en el juego, asi
# que se respeta: Snorunt con la Piedra Alba da Froslass solo si es
# hembra (si es macho, la piedra no hace nada, como en los juegos).
#   0 = da igual, 1 = hembra, 2 = macho
SEXO_DE = {"ItemFemale": 1, "ItemMale": 2}
SIN_NADA = ("Trade",)


def clave(s):
    return re.sub(r"[^A-Z0-9]", "", s.upper())


def items_nuestros():
    """{CLAVE_INTERNA_DEL_PBS: constante del arg} de los objetos de evolucion.

    Se lee la tabla de gen_items.py y NO el items.h generado. Parecia lo
    mismo y no lo es: items.h lleva el nombre de PANTALLA, que sale de la
    traduccion del propio Anil, y ahi WHIPPEDDREAM se llama "Sweet Cream".
    Casando por el nombre de pantalla, el objeto de Slurpuff existia pero
    la evolucion no lo encontraba, y el aviso decia que faltaba un objeto
    que estaba puesto.
    """
    from gen_items import LISTA, USABLES
    # Igual que lo monta gen_items.main(): LISTA son los de llevar (cuatro
    # campos, bolsillo siempre BP_HELD) y USABLES los de usar (cinco, con
    # el bolsillo delante). Las piedras estan en USABLES.
    todos = [(i, "BP_HELD", f, a, c) for i, f, a, c in LISTA] + list(USABLES)
    return {clave(interno): arg
            for interno, bolsillo, fx, arg, *_ in todos
            if bolsillo == "BP_EVO" and fx == "IT_STONE"}


def lee_pbs():
    """{dex: [(destino, metodo, arg)]} limitado a nuestra dex.

    El parseo de secciones y la comprobacion de que el fichero va en orden
    nacional viven en anil_pbs.py; aqui solo se interpreta Evolutions.
    """
    out = {}
    for num, _slug, c in anil_pbs.filas():
        campos = [x.strip() for x in c.get("Evolutions", "").split(",")]
        for i in range(0, max(0, len(campos) - 2), 3):
            destino = SLUG.get(clave(campos[i]))
            if destino and destino <= N_DEX:
                out.setdefault(num, []).append(
                    (destino, campos[i + 1], campos[i + 2]))
    return out


# Lo que el Anil pide con metodos que aqui todavia no existen, puesto a un
# objeto EQUIVALENTE para que la especie sea alcanzable. Va aparte y con su
# nombre para que se vea que es un apano, no el dato: el dia que evolve()
# sepa del vinculo y de la hora, esto se borra.
#
#   Espeon  = HappinessDay    -> la del sol
#   Umbreon = HappinessNight  -> la de la luna (Moon Stone, T_FAIRY)
APANOS = [
    (133, "EVOI_SUN", 196, "HappinessDay"),
    (133, "T_FAIRY", 197, "HappinessNight"),
]


def solo_objeto(evos):
    """Los que NO tienen ningun camino por nivel segun el Anil.

    Hace falta fuera: su `evolveLevel` tiene que ser 0 o subirian solos y el
    objeto no serviria de nada. Kadabra, Machoke, Graveler, Haunter y Eevee
    estaban con un nivel inventado en dex_data.py.
    """
    out = set()
    for d, lista in evos.items():
        if any(m == "Level" for _, m, _ in lista):
            continue
        if any(m in CON_OBJETO or m in SIN_NADA for _, m, _ in lista):
            out.add(d)
    return out


def main():
    evos = lee_pbs()
    mios = items_nuestros()
    filas, faltan_obj, sin_modelar = [], Counter(), Counter()
    vistos = set()
    for d in sorted(evos):
        for destino, metodo, arg in evos[d]:
            if metodo in SIN_NADA:
                k = (d, "EVOI_CABLE", destino)
                if k not in vistos:
                    vistos.add(k)
                    filas.append((d, "EVOI_CABLE", destino, metodo, "", 0, 1))
                continue
            if metodo not in CON_OBJETO:
                sin_modelar[metodo] += 1
                continue
            cte = mios.get(clave(arg))
            if not cte:
                faltan_obj[arg] += 1
                continue
            k = (d, cte, destino)
            if k not in vistos:
                vistos.add(k)
                filas.append((d, cte, destino, metodo, arg,
                              SEXO_DE.get(metodo, 0),
                              2 if metodo == "TradeItem" else 0))

    for d, cte, destino, metodo in APANOS:
        if destino <= N_DEX and (d, cte, destino) not in vistos:
            vistos.add((d, cte, destino))
            filas.append((d, cte, destino, metodo + " (apano)", "", 0, 0))
    filas.sort(key=lambda f: (f[0], f[2]))

    solos = sorted(solo_objeto(evos))

    out = [
        "// GENERADO por tools/gen_evos.py desde el PBS del Anil - no editar.",
        "//",
        "// Que OBJETO pide cada evolucion. Antes esto estaba a mano en",
        "// stones.h y se invento la mitad: un \"cable\" generico donde el Anil",
        "// dice Metal Coat, Dragon Scale o King's Rock, y piedras para Espeon",
        "// y Umbreon donde dice amistad.",
        "//",
        "// Item, TradeItem y DayHoldItem acaban todos aqui: en un cacharro sin",
        "// radio no hay con quien intercambiar, asi que el objeto se USA desde",
        "// la bolsa. El Cable sigue existiendo para los que se cambian SIN",
        "// objeto (Alakazam, Machamp, Golem, Gengar).",
        "#pragma once",
        "#include <stdint.h>",
        '#include "items.h"',
        "",
        "struct EvoItem {",
        "  int16_t from;",
        "  uint8_t item;    // EVOI_*: el `arg` del objeto en ITEM_TBL",
        "  int16_t to;",
        "  uint8_t sexo;    // 0 cualquiera, 1 solo hembra, 2 solo macho",
        "  // Se ela vem de TROCA no Anil: 1 = Trade (sem objeto, o Cabo),",
        "  // 2 = TradeItem (chegar segurando o objeto). Com ESP-NOW a troca",
        "  // de verdade evolui por isto; o objeto da bolsa continua valendo.",
        "  uint8_t troca;",
        "};",
        "",
        f"#define EVO_ITEM_COUNT {len(filas)}",
        f"static const EvoItem EVO_ITEM[EVO_ITEM_COUNT] = {{",
    ]
    for d, cte, destino, metodo, arg, sexo, troca in filas:
        out.append(f"  {{ {d:3d}, {cte:<17s}, {destino:3d}, {sexo}, {troca} }},"
                   f"  // {NOMBRE.get(d, d)} -> {NOMBRE.get(destino, destino)}"
                   f" ({metodo}{', ' + arg if arg else ''})")
    out += [
        "};",
        "",
        "// Si a este le sirve algun objeto para evolucionar.",
        "static inline bool evoHasItem(int16_t dex) {",
        "  for (uint8_t i = 0; i < EVO_ITEM_COUNT; i++)",
        "    if (EVO_ITEM[i].from == dex) return true;",
        "  return false;",
        "}",
        "",
        "// En que se convierte con ESE objeto. 0 = en nada.",
        "//",
        "// `hembra` hace falta porque hay evoluciones que ademas piden",
        "// sexo: Snorunt con la Piedra Alba da Froslass, pero solo si es",
        "// hembra. A un macho la piedra no le hace nada, como en los",
        "// juegos. Por omision `true` para que quien no sepa de sexos (los",
        "// tests viejos, la ficha) siga viendo el destino.",
        "static inline int16_t evoItemTarget(int16_t dex, uint8_t item,",
        "                                    bool hembra = true) {",
        "  for (uint8_t i = 0; i < EVO_ITEM_COUNT; i++) {",
        "    if (EVO_ITEM[i].from != dex || EVO_ITEM[i].item != item) continue;",
        "    if (EVO_ITEM[i].sexo == 1 && !hembra) continue;",
        "    if (EVO_ITEM[i].sexo == 2 && hembra) continue;",
        "    return EVO_ITEM[i].to;",
        "  }",
        "  return 0;",
        "}",
        "",
        "// Em que vira ao CHEGAR numa troca, segurando `item` (o `arg` do",
        "// objeto, 0xFF = nada). Primeiro a de objeto (Onix com Metal Coat),",
        "// depois a sem objeto (Kadabra). 0 = nao evolui.",
        "static inline int16_t evoTrocaTarget(int16_t dex, uint8_t item,",
        "                                     bool hembra = true) {",
        "  for (uint8_t i = 0; i < EVO_ITEM_COUNT; i++) {",
        "    const EvoItem &e = EVO_ITEM[i];",
        "    if (e.from != dex || e.troca != 2 || e.item != item) continue;",
        "    if (e.sexo == 1 && !hembra) continue;",
        "    if (e.sexo == 2 && hembra) continue;",
        "    return e.to;",
        "  }",
        "  for (uint8_t i = 0; i < EVO_ITEM_COUNT; i++)",
        "    if (EVO_ITEM[i].from == dex && EVO_ITEM[i].troca == 1)",
        "      return EVO_ITEM[i].to;",
        "  return 0;",
        "}",
        "",
        "// Se o objeto (`arg`) e de troca para alguem: e o que faz a bolsa",
        "// perguntar Usar / Segurar.",
        "static inline bool evoItemDeTroca(uint8_t item) {",
        "  for (uint8_t i = 0; i < EVO_ITEM_COUNT; i++)",
        "    if (EVO_ITEM[i].troca == 2 && EVO_ITEM[i].item == item) return true;",
        "  return false;",
        "}",
        "",
        "// Que objeto le hace falta, para poder decirselo al jugador.",
        "// 0xFF = ninguno; 0xFE = varios (y entonces elige el jugador).",
        "#define EVO_ITEM_MANY 0xFE",
        "static inline uint8_t evoItemFor(int16_t dex) {",
        "  uint8_t visto = 0xFF;",
        "  for (uint8_t i = 0; i < EVO_ITEM_COUNT; i++) {",
        "    if (EVO_ITEM[i].from != dex) continue;",
        "    if (visto != 0xFF && visto != EVO_ITEM[i].item) return EVO_ITEM_MANY;",
        "    visto = EVO_ITEM[i].item;",
        "  }",
        "  return visto;",
        "}",
        "",
        "// Los que SOLO salen por objeto (el Anil no les da ningun camino por",
        "// nivel). gen_dex.py les pone evolveLevel = 0 a partir de esta misma",
        "// lectura del PBS, para que no suban solos y el objeto sirva de algo:",
        "//   " + ", ".join(NOMBRE.get(d, str(d)) for d in solos),
        "",
    ]
    with io.open(DST, "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(out))

    print(f"{len(filas)} evoluciones por objeto -> {os.path.normpath(DST)}")
    print("\nsolo por objeto (gen_dex les pone evolveLevel = 0):")
    print("   " + ", ".join(NOMBRE.get(d, str(d)) for d in solos))
    if faltan_obj:
        print("\nobjetos que el Anil pide y no tenemos en items.h:")
        for k, v in faltan_obj.most_common():
            print(f"   {k:<16s} {v}")
    if sin_modelar:
        print("\nmetodos que el Anil usa y aqui TODAVIA no se modelan:")
        for k, v in sin_modelar.most_common():
            print(f"   {k:<16s} {v}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
