#!/usr/bin/env python3
"""Genera tms.h: las MT, sacadas del TutorMoves del Anil.

Idea del usuario: los golpes que un bicho puede aprender pero nunca
aprende solo, repartidos como premio. El Anil ya lo trae todo —
`TutorMoves` esta en 1021 de las 1025 especies, con una mediana de 57
golpes utiles por bicho — asi que no hay que inventarse ninguna lista.

POR QUE NO SON OBJETOS DE LA BOLSA, que era lo primero que uno piensa:
los identificadores de objeto son `uint8_t` en TODAS partes
(Combatant::item, StoredMon::item, Pet::heldItem, bag[]). Con 211
objetos ya en la bolsa, meter 286 MT pasaria de 255 y cada id daria la
vuelta en silencio — el fallo de siempre de este proyecto. Ensanchar
todo eso a uint16_t es un cambio grande y arriesgado para nada: una MT
no se lleva encima ni se tira, asi que no necesita ser un objeto.

Lo que son: un numero, una tabla que dice a que golpe corresponde, y
dos mapas de bits. Uno por especie (que puede aprender) y otro por
partida (cuales tienes). 286 bits son 36 bytes.

  python3 tools/gen_tms.py
"""
import io
import os
import re
import sys

AQUI = os.path.dirname(os.path.abspath(__file__))
RAIZ = os.path.dirname(AQUI)
sys.path.insert(0, AQUI)
import anil_pbs                        # noqa: E402
from dex_range import N_DEX            # noqa: E402

DST = os.path.join(RAIZ, "tms.h")


def clave(s):
    return re.sub(r"[^A-Z0-9]", "", s.upper())


def main():
    # Los golpes que YA existen en el juego. Una MT de un golpe que no
    # esta en la tabla no serviria de nada: se descartan y se cuentan.
    mv = io.open(os.path.join(RAIZ, "moves.h"), encoding="utf-8",
                 errors="replace").read()
    cuerpo = mv[mv.index("MOVE_TBL"):]
    nombres = re.findall(r'^\s*\{\s*"([^"]+)",', cuerpo, re.M)
    if not nombres:
        raise SystemExit("no leo MOVE_TBL: gen_moves tiene que ir antes")
    idx = {clave(n): i for i, n in enumerate(nombres)}

    porEspecie = {}
    fuera = set()
    todos = set()
    for d, _slug, c in anil_pbs.filas():
        suyos = set()
        for t in (c.get("TutorMoves") or "").split(","):
            k = clave(t)
            if not k:
                continue
            if k not in idx:
                fuera.add(k)
                continue
            suyos.add(k)
            todos.add(k)
        if suyos:
            porEspecie[d] = suyos

    # El NUMERO de cada MT esta CONGELADO en tools/orden_mts.txt. Antes era
    # el orden alfabetico, y una MT nueva en medio renumeraba todas las de
    # detras: la partida guardada pasaba a tener otras, y la unica defensa
    # era tirarlas todas (ver Pet::load). Lo nuevo va AL FINAL.
    fijas = [l.strip() for l in io.open(os.path.join(os.path.dirname(
        os.path.abspath(__file__)), "orden_mts.txt"), encoding="utf-8")
        if l.strip() and not l.startswith("#")]
    perdidas = [k for k in fijas if k not in idx]
    if perdidas:
        raise SystemExit("orden_mts.txt nombra MT cuyo golpe ya no esta en "
                         "moves.h: %s. Quitarlas correria las demas" % perdidas)
    nuevas = sorted(todos - set(fijas))
    if nuevas:
        print("  %d MT nuevas, al final: %s" % (len(nuevas), ", ".join(nuevas)))
    orden = fijas + nuevas
    tm = {k: i for i, k in enumerate(orden)}
    bytesPorEspecie = (len(orden) + 7) // 8

    out = [
        "// GENERADO por tools/gen_tms.py desde el TutorMoves del Anil.",
        "// No editar.",
        "//",
        "// Una MT no es un objeto de la bolsa: los ids de objeto son",
        "// uint8_t y con 211 ya ocupados, 286 MT mas pasarian de 255 y",
        "// cada id daria la vuelta en silencio. Y tampoco hace falta:",
        "// una MT no se lleva encima ni se tira.",
        "//",
        "// Son un numero y dos mapas de bits: TM_OF dice que puede",
        "// aprender cada especie, y Pet::tms cuales tienes tu.",
        "#pragma once",
        "#include <stdint.h>",
        '#include "dex.h"',
        '#include "moves.h"',
        "",
        f"#define TM_COUNT {len(orden)}",
        f"#define TM_BYTES {bytesPorEspecie}",
        "// Las primeras TM_COUNT_FIJO tienen el numero congelado: una partida",
        "// guardada con entre TM_COUNT_FIJO y TM_COUNT MT se lee tal cual.",
        f"#define TM_COUNT_FIJO {len(fijas)}",
        "",
        "// Con cuantos golpes tenia la tabla cuando se hizo esto. TM_MOVE",
        "// guarda INDICES, asi que si moves.h crece y esto no se rehace,",
        "// las MT no pierden una animacion: ensenan OTRO golpe, y en",
        "// silencio. Paso (286 MT contra la tabla vieja de 667) porque",
        "// gen_tms.py no estaba en gen_todo.py. auditoria.py lo compara",
        "// con MOVE_COUNT, igual que hace con FX_MOVE_COUNT de art.h.",
        f"#define TM_MOVE_COUNT {len(nombres)}",
        "",
        "// A que golpe corresponde cada MT.",
        "static const uint16_t TM_MOVE[TM_COUNT] = {",
    ]
    fila = []
    for k in orden:
        fila.append(str(idx[k]))
        if len(fila) == 12:
            out.append("  " + ", ".join(fila) + ",")
            fila = []
    if fila:
        out.append("  " + ", ".join(fila) + ",")
    out.append("};")
    out.append("")

    # El mapa por especie. 1025 x 36 bytes = 36 KB de flash, que es lo
    # que cuesta que cada bicho tenga SU lista y no una comun.
    out.append("// Que MT puede aprender cada especie. Un bit por MT.")
    out.append("static const uint8_t TM_OF[DEX_COUNT + 1][TM_BYTES] = {")
    for d in range(0, N_DEX + 1):
        bits = bytearray(bytesPorEspecie)
        for k in porEspecie.get(d, ()):  # el 0 no tiene ninguna
            i = tm[k]
            bits[i >> 3] |= 1 << (i & 7)
        out.append("  { " + ", ".join(str(b) for b in bits) + " },")
    out.append("};")
    out.append("")
    out.extend([
        "// ¿Puede ESTA especie aprender ESA MT?",
        "static inline bool tmFits(int16_t dex, uint16_t tm) {",
        "  if (dex < 1 || dex > DEX_COUNT || tm >= TM_COUNT) return false;",
        "  return (TM_OF[dex][tm >> 3] >> (tm & 7)) & 1;",
        "}",
        "",
        "// El golpe de una MT, o MOVE_STRUGGLE si el numero no existe.",
        "static inline uint16_t tmMove(uint16_t tm) {",
        "  return tm < TM_COUNT ? TM_MOVE[tm] : (uint16_t)MOVE_STRUGGLE;",
        "}",
        "",
        "// Cuantas MT puede aprender esta especie (para la pantalla).",
        "static inline uint16_t tmFitCount(int16_t dex) {",
        "  if (dex < 1 || dex > DEX_COUNT) return 0;",
        "  uint16_t n = 0;",
        "  for (uint16_t i = 0; i < TM_COUNT; i++)",
        "    if ((TM_OF[dex][i >> 3] >> (i & 7)) & 1) n++;",
        "  return n;",
        "}",
        "",
    ])

    io.open(DST, "w", encoding="utf-8", newline="\n").write("\n".join(out) + "\n")
    cuantas = [len(v) for v in porEspecie.values()]
    cuantas.sort()
    print(f"{len(orden)} MT -> {os.path.normpath(DST)}")
    print(f"  {len(porEspecie)} especies con alguna; por especie: "
          f"min {cuantas[0]}, mediana {cuantas[len(cuantas) // 2]}, "
          f"max {cuantas[-1]}")
    print(f"  {len(fuera)} golpes del TutorMoves que no estan en la tabla "
          f"y se quedan fuera")
    print(f"  peso: {len(orden) * 2} B la tabla + "
          f"{(N_DEX + 1) * bytesPorEspecie} B el mapa por especie")
    return 0


if __name__ == "__main__":
    sys.exit(main())
