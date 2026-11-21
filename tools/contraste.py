#!/usr/bin/env python3
"""Contraste: lo que SE EMBARCA contra el PBS del Anil, bicho a bicho.

La auditoria responde "esto esta completo y se puede leer". Esta
responde otra pregunta, que es la que el usuario hizo: **¿es lo que
tiene que ser?** Que Charizard aprenda los golpes de Charizard al nivel
que toca, que evolucione a quien toca, que tenga las habilidades que
el Anil le da, y que su velocidad sea su velocidad.

COMO lo hace, que es lo unico que la hace valer:

  - lee los .h COMO TEXTO, tal y como estan en el disco. No llama a los
    generadores. Si un .h se quedo viejo, o si el generador se cayo a la
    mitad, aqui se ve; regenerando y comparando, no.

  - vuelve a leer el PBS del Anil por su cuenta y rehace la cuenta. El
    unico codigo compartido es el lector de secciones (anil_pbs), que es
    la ENTRADA — no la transformacion.

  - y como eso solo prueba que la COPIA es fiel, no que la LECTURA del
    formato sea correcta, hay un bloque de VERDAD A MANO (`A_MANO`):
    hechos comprobados uno a uno contra el juego. Es lo que habria
    cazado el lio del BaseStats, donde la velocidad va cuarta y leerlo
    de corrido deja numeros perfectamente plausibles en el sitio
    equivocado.

  python3 tools/contraste.py            # todo
  python3 tools/contraste.py golpes     # una sola seccion
  python3 tools/contraste.py -v         # lista cada diferencia
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

fallos = []
VERBOSO = "-v" in sys.argv


def mal(sec, msg):
    fallos.append((sec, msg))


def lee(n):
    return io.open(os.path.join(RAIZ, n), encoding="utf-8",
                   errors="replace").read()


def pide(sec, txt, patron, que):
    """Lo mismo que en auditoria.py, y por el mismo motivo.

    Un patron que deja de casar devuelve CERO diferencias, y cero se lee
    como "todo bien". Si lo que hace falta no esta, es un fallo.
    """
    m = re.search(patron, txt, re.S)
    if not m:
        mal(sec, f"NO ENCUENTRO {que}: esta comprobacion no ha mirado nada")
        return None
    return m.group(1)


def clave(s):
    return re.sub(r"[^A-Z0-9]", "", s.upper())


# --------------------------------------------------------- verdad a mano
#
# Hechos comprobados UNO A UNO, a mano, contra el juego y el PBS. No
# salen de ningun generador: existen para pillar el caso en que yo
# entienda mal el formato y lo copie mal pero coherentemente, que es el
# fallo que ninguna comparacion automatica puede ver.
#
# OJO, y esto me paso escribiendo este mismo fichero: la verdad de
# referencia es **el PBS del Anil leido a ojo**, NO los stats oficiales
# de la serie. El Anil esta rebalanceado. La primera version de esta
# tabla la copie de los juegos y canto cuatro fallos que no existian:
# el Pikachu del Anil tiene 30 de Defensa y 40 de DefEsp, no 40 y 50.
# Si estos numeros hay que tocarlos, se tocan abriendo pokemon.txt.
#
# El orden es (PS, Atq, Def, AtqEsp, DefEsp, Vel) — el NUESTRO. El PBS
# los escribe HP,Atk,Def,VELOCIDAD,SpA,SpD, y esta tabla existe sobre
# todo por eso: es lo unico que prueba que la velocidad sale del cuarto
# hueco y no del cuarto campo que uno esperaria.
A_MANO_STATS = {
    #                                   linea del PBS, transcrita a ojo
    1:   (45, 49, 49, 65, 65, 45),      # Bulbasaur  45,49,49,45,65,65
    6:   (78, 84, 78, 109, 85, 100),    # Charizard  78,84,78,100,109,85
    25:  (35, 55, 30, 50, 40, 90),      # Pikachu    35,55,30,90,50,40
    143: (160, 110, 65, 65, 110, 30),   # Snorlax    160,110,65,30,65,110
    150: (110, 110, 100, 154, 130, 130),  # Mewtwo   110,110,100,130,154,130
    291: (61, 90, 45, 50, 50, 160),     # Ninjask    61,90,45,160,50,50
    488: (120, 70, 110, 75, 120, 85),   # Cresselia  120,70,110,85,75,120
}

# (dex, nivel, GOLPE) que TIENE que estar en el aprendizaje.
# Golpes cuyos PP se APARTAN del PBS a proposito. Uno solo, y con
# motivo: el PBS le da 1 PP a Esquema porque en los juegos originales
# Esquema es PERMANENTE -- lo usas una vez en la vida del bicho y te
# quedas el golpe. Aqui la copia dura el combate (el equipo guarda los
# golpes por nivel, no por bicho), asi que con un uso el Smeargle seria
# un bicho de un solo acto. La regla "ningun golpe de un solo uso" es
# decision de este juego y tiene su test.
#
# La excepcion se escribe en DOS sitios y eso es a proposito: aqui, para
# que el contraste no grite, y en PP_PROPIO de gen_moves.py, que es
# quien lo hace y lo anuncia por pantalla al generar.
PP_A_PROPOSITO = {"SKETCH"}

A_MANO_GOLPES = [
    (1, 1, "TACKLE"),
    (4, 1, "SCRATCH"),
    (25, 1, "THUNDERSHOCK"),
    (129, 1, "SPLASH"),        # Magikarp y su unico golpe de salida
    (143, 1, "TACKLE"),
]

# (dex, el `evolvesTo` de dex.h, el `evolveLevel`).
#
# CUIDADO con lo que significa cada campo, que tambien me equivoque:
# `evolvesTo` es a quien evoluciona, para la cadena de la Pokedex y para
# el camino del objeto; `evolveLevel` 0 quiere decir "no por nivel", y
# entonces el unico camino es la piedra (lo decide `onlyByItem` en
# pet.cpp). Por eso Eevee pone (134, 0) y no (0, 0): apunta a Vaporeon
# pero no sube solo.
A_MANO_EVO = {
    1: (2, 16), 2: (3, 32), 4: (5, 16), 5: (6, 36),
    129: (130, 20),            # Magikarp -> Gyarados, por nivel
    133: (134, 0),             # Eevee: apunta a Vaporeon, pero solo con piedra
    25:  (26, 0),              # Pikachu: Piedra Trueno, nunca por nivel
    132: (0, 0),               # Ditto no evoluciona
    149: (0, 0),               # Dragonite es final
}

# (dex, habilidad que tiene). Nombre tal cual lo pinta la ficha.
A_MANO_HAB = [
    (25, "Static"),
    (143, "Immunity"),
    (6, "Blaze"),
    (130, "Intimidate"),
]


def sec_a_mano():
    """La verdad escrita a mano contra lo que se embarca."""
    print("--- verdad comprobada a mano ---")
    dex = lee("dex.h")
    filas = tabla_dex(dex)
    if filas is None:
        return
    n = 0
    for d, esperado in A_MANO_STATS.items():
        if d not in filas:
            mal("a mano", f"dex {d} no esta en DEX_TBL")
            continue
        suyo = filas[d]["stats"]
        if suyo != esperado:
            mal("a mano", f"{filas[d]['nombre']}: stats {suyo}, "
                          f"comprobado a mano {esperado}")
        n += 1
    for d, (quien, lvl) in A_MANO_EVO.items():
        if d not in filas:
            continue
        suyo = (filas[d]["evo"], filas[d]["lvl"])
        if suyo != (quien, lvl):
            mal("a mano", f"{filas[d]['nombre']}: evoluciona {suyo}, "
                          f"comprobado a mano {(quien, lvl)}")
        n += 1

    apren = aprendizaje()
    nombres = nombres_golpes()
    if apren is not None:
        for d, lvl, golpe in A_MANO_GOLPES:
            pares = apren.get(d, [])
            if (lvl, clave(golpe)) not in {(l, clave(nombres[i]))
                                           for l, i in pares
                                           if i < len(nombres)}:
                mal("a mano", f"{filas[d]['nombre']} no aprende {golpe} "
                              f"al nivel {lvl}, y lo comprobe a mano")
            n += 1

    hab = habilidades_de()
    if hab is not None:
        for d, nom in A_MANO_HAB:
            if clave(nom) not in {clave(x) for x in hab.get(d, [])}:
                mal("a mano", f"{filas[d]['nombre']} no tiene {nom} "
                              f"(tiene {hab.get(d)}), y lo comprobe a mano")
            n += 1
    print(f"  {n} hechos comprobados a mano")


# ------------------------------------------------------ lectura de los .h
def tabla_dex(dex=None):
    """{num: {nombre, evo, lvl, stats, tipos, fem}} leido de dex.h."""
    dex = dex or lee("dex.h")
    cuerpo = pide("dex", dex, r"DEX_TBL\[DEX_COUNT \+ 1\] = \{(.*?)\n\};",
                  "la tabla DEX_TBL")
    if cuerpo is None:
        return None
    # El patron NO cuenta campos hasta los tipos: la fila crece (acaba
    # de ganar el PESO) y atarse al numero exacto hace que esto deje de
    # leer nada al primer campo nuevo. Ya paso dos veces, aqui y en
    # MOVE_TBL. Lo que se ancla es lo que se va a usar: el nombre y los
    # tres primeros numeros al principio, y los tipos y el sexo al
    # final, que son los ultimos de la fila.
    out = {}
    for i, m in enumerate(re.finditer(
            r'^\s*\{ "([^"]*)", (\d+), (\d+), (\w+), 0x[0-9A-Fa-f]+, '
            r'(\d+), (\d+), (\d+), (\d+), (\d+), (\d+), '
            r'.*?(T_\w+), (T_\w+), (\d+) \}', cuerpo, re.M)):
        g = m.groups()
        out[i] = {"nombre": g[0], "evo": int(g[1]), "lvl": int(g[2]),
                  "stats": tuple(int(x) for x in g[4:10]),
                  "tipos": (g[10], g[11]), "fem": int(g[12])}
    if len(out) <= N_DEX:
        mal("dex", f"leo {len(out)} filas de DEX_TBL y hacen falta "
                   f"{N_DEX + 1}: no se compara nada")
        return None
    return out


def nombres_golpes():
    """[nombre] por indice de MOVE_TBL."""
    mv = lee("moves.h")
    cuerpo = mv[mv.index("MOVE_TBL"):]
    return re.findall(r'^\s*\{\s*"([^"]+)",\s*T_', cuerpo, re.M)


def aprendizaje():
    """{dex: [(nivel, indice de golpe)]} leido de moves.h."""
    mv = lee("moves.h")
    ofs = pide("golpes", mv, r"LEARN_OFS\[[^\]]*\] = \{(.*?)\};",
               "LEARN_OFS")
    lvl = pide("golpes", mv, r"LEARN_LVL\[[^\]]*\] = \{(.*?)\};",
               "LEARN_LVL")
    idx = pide("golpes", mv, r"LEARN_MV\[[^\]]*\] = \{(.*?)\};",
               "LEARN_MV")
    if ofs is None or lvl is None or idx is None:
        return None
    ofs = [int(x) for x in re.findall(r"\d+", ofs)]
    lvl = [int(x) for x in re.findall(r"\d+", lvl)]
    idx = [int(x) for x in re.findall(r"\d+", idx)]
    if len(lvl) != len(idx):
        mal("golpes", f"LEARN_LVL trae {len(lvl)} y LEARN_IDX {len(idx)}")
        return None
    return {d: list(zip(lvl[ofs[d - 1]:ofs[d]], idx[ofs[d - 1]:ofs[d]]))
            for d in range(1, min(N_DEX, len(ofs) - 1) + 1)}


def habilidades_de(crudo=False):
    """{dex: [nombres]} leido de abilities.h. `crudo` devuelve los indices."""
    ab = lee("abilities.h")
    tbl = pide("habilidades", ab, r"ABIL_TBL\[[^\]]*\] = \{(.*?)\n\};",
               "ABIL_TBL")
    de = pide("habilidades", ab, r"ABIL_OF\[[^\]]*\]\[3\] = \{(.*?)\n\};",
              "ABIL_OF")
    if tbl is None or de is None:
        return None
    noms = re.findall(r'^\s*\{\s*"([^"]*)"', tbl, re.M)
    out = {}
    for i, m in enumerate(re.finditer(
            r"\{\s*(\d+),\s*(\d+),\s*(\d+)\s*\}", de)):
        if crudo:
            out[i] = [int(x) for x in m.groups()]
        else:
            out[i] = [noms[int(x)] for x in m.groups()
                      if int(x) and int(x) < len(noms)]
    return out


def evos_item():
    """{(desde, hasta)} de evos.h — las que piden objeto."""
    ev = lee("evos.h")
    cuerpo = pide("evoluciones", ev, r"EVO_ITEM\[[^\]]*\] = \{(.*?)\n\};",
                  "EVO_ITEM")
    if cuerpo is None:
        return set()
    return {(int(a), int(b)) for a, b in re.findall(
        r"^\s*\{\s*(\d+),\s*\w+\s*,\s*(\d+),", cuerpo, re.M)}


def evos_rama():
    """{(desde, hasta)} de las OTRAS tablas de dex.h.

    Hay TRES sitios donde vive una evolucion, no dos, y mirar solo los
    dos primeros me hizo informar de un fallo que no existia: dije que
    un Burmy macho acababa de Wormadam cuando DEX_SEXADA ya lo mandaba
    a Mothim desde hacia tiempo. Un verificador que se inventa fallos
    gasta la misma confianza que uno que se los calla.

      DexEntry.evolvesTo   la rama por omision
      EVO_ITEM (evos.h)    las que piden un objeto
      DEX_SEXADA           la del macho, cuando el sexo parte la linea
      EVO_RAMA             las demas (stats, azar, el Shedinja de regalo)
    """
    dex = lee("dex.h")
    out = set()
    cuerpo = pide("evoluciones", dex,
                  r"DEX_SEXADA\[[^\]]*\] = \{(.*?)\};", "DEX_SEXADA")
    if cuerpo is not None:
        out |= {(int(a), int(b)) for a, b in
                re.findall(r"\{\s*(\d+),\s*(\d+)\s*\}", cuerpo)
                if int(a) and int(b)}
    cuerpo = pide("evoluciones", dex,
                  r"EVO_RAMA\[[^\]]*\] = \{(.*?)\n\};", "EVO_RAMA")
    if cuerpo is not None:
        out |= {(int(a), int(b)) for a, b in
                re.findall(r"\{\s*(\d+),\s*\w+\s*,\s*(\d+)\s*\}", cuerpo)
                if int(a) and int(b)}
    return out


# ------------------------------------------------------- las comparaciones
def cuenta(sec, titulo, difs, cuantos):
    """Informa de una comparacion. `cuantos` es lo que SI se miro."""
    if cuantos == 0:
        mal(sec, f"{titulo}: no se comparo ni una especie")
        return
    print(f"  {titulo}: {cuantos} comparadas, {len(difs)} distintas")
    for d in difs[:40 if VERBOSO else 6]:
        mal(sec, d)
    if len(difs) > (40 if VERBOSO else 6):
        mal(sec, f"{titulo}: ...y {len(difs) - (40 if VERBOSO else 6)} mas "
                 f"(con -v salen 40)")


def sec_stats():
    print("--- stats, tipos y sexo ---")
    filas = tabla_dex()
    if filas is None:
        return
    T = {"fighting": "T_FIGHT", "electric": "T_ELEC"}
    ds, dt, dg = [], [], []
    n = 0
    for num, _slug, c in anil_pbs.filas():
        if num not in filas:
            continue
        f = filas[num]
        n += 1
        if anil_pbs.stats(c) != f["stats"]:
            ds.append(f"{f['nombre']}: dex.h {f['stats']} / PBS "
                      f"{anil_pbs.stats(c)}")
        t1, t2 = anil_pbs.tipos(c)
        esp = (T.get(t1, "T_" + t1.upper()),
               T.get(t2, "T_" + t2.upper()) if t2 else "T_NONE")
        if esp != f["tipos"]:
            dt.append(f"{f['nombre']}: dex.h {f['tipos']} / PBS {esp}")
        if anil_pbs.fem_pct(c) != f["fem"]:
            dg.append(f"{f['nombre']}: dex.h {f['fem']}% hembra / PBS "
                      f"{anil_pbs.fem_pct(c)}%")
    cuenta("stats", "stats base", ds, n)
    cuenta("tipos", "tipos", dt, n)
    cuenta("sexo", "reparto de sexo", dg, n)


def sec_habilidades():
    """Que cada especie tenga LAS SUYAS, sin comparar nombres.

    Comparar el nombre de pantalla con la clave del PBS no vale: el
    catalogo va en INGLES (sale de la traduccion del propio usuario) y
    el Anil nombra varias en castellano — ACOMETIDA es "Blitz",
    CAMORRISTA es "Striker". Haciendolo asi salian 53 "diferencias" que
    no lo eran, y las de verdad se habrian perdido entre ellas.

    Lo que si se puede comprobar sin saber traducir, y es lo que de
    hecho se rompe, es que la correspondencia sea una BIYECCION: la
    misma clave del PBS tiene que dar SIEMPRE el mismo indice, y dos
    claves distintas nunca el mismo. Un desplazamiento de una fila en
    ABIL_OF, o una habilidad duplicada en el catalogo, rompe eso en las
    1025 a la vez. Y aparte se exige que el NUMERO de habilidades por
    especie cuadre, que es lo que pilla una que se cayo.
    """
    print("--- habilidades ---")
    hab = habilidades_de(crudo=True)
    nombres = habilidades_de()
    filas = tabla_dex()
    if hab is None or filas is None:
        return
    porClave, porIndice = {}, {}
    difs, n = [], 0
    for num, _slug, c in anil_pbs.filas():
        if num not in hab:
            continue
        n += 1
        normales = [clave(a) for a in c.get("Abilities", "").split(",")
                    if a.strip()]
        ocultas = [clave(a) for a in c.get("HiddenAbilities", "").split(",")
                   if a.strip()]
        # ABIL_OF es {normal 1, normal 2, oculta}; el PBS puede dar una
        # sola normal y entonces el segundo hueco va a 0.
        esperadas = [normales[0] if len(normales) > 0 else None,
                     normales[1] if len(normales) > 1 else None,
                     ocultas[0] if ocultas else None]
        suyas = hab[num]
        for hueco, (k, i) in enumerate(zip(esperadas, suyas)):
            if k is None and i != 0:
                difs.append(f"{filas[num]['nombre']}: el hueco {hueco} tiene "
                            f"la habilidad {i} y el PBS no le da ninguna")
                continue
            if k is not None and i == 0:
                difs.append(f"{filas[num]['nombre']}: el PBS le da {k} en el "
                            f"hueco {hueco} y abilities.h lo deja vacio")
                continue
            if k is None:
                continue
            # OJO al leer estos dos: el conflicto se ve en la SEGUNDA
            # especie que use la clave, no en la que esta mal. Si se
            # cambia la habilidad de Bulbasaur, quien lo canta es
            # Ivysaur, que es el siguiente en tenerla. Por eso el
            # mensaje nombra a las dos: una de ellas es la rota.
            if k in porClave and porClave[k][0] != i:
                difs.append(f"{k} vale {porClave[k][0]} en "
                            f"{porClave[k][1]} y {i} en "
                            f"{filas[num]['nombre']}: una de las dos esta mal")
            porClave.setdefault(k, (i, filas[num]["nombre"]))
            if i in porIndice and porIndice[i][0] != k:
                difs.append(f"el indice {i} lo usan {porIndice[i][0]} "
                            f"({porIndice[i][1]}) y {k} "
                            f"({filas[num]['nombre']})")
            porIndice.setdefault(i, (k, filas[num]["nombre"]))
    cuenta("habilidades", "habilidades por especie", difs, n)
    print(f"  {len(porClave)} claves del PBS, cada una con un indice propio "
          f"y estable en las {n}")
    # La biyeccion dice que la correspondencia es coherente, no QUE
    # habilidad es cada una. Eso lo fija A_MANO_HAB, mirando la ficha.


def sec_golpes():
    print("--- golpes aprendidos ---")
    apren = aprendizaje()
    filas = tabla_dex()
    if apren is None or filas is None:
        return
    noms = nombres_golpes()
    porNombre = {clave(x): i for i, x in enumerate(noms)}

    difs, n, fuera = [], 0, {}
    for num, _slug, c in anil_pbs.filas():
        if num not in apren:
            continue
        n += 1
        bruto = [x.strip() for x in c.get("Moves", "").split(",")]
        quiere = set()
        for i in range(0, len(bruto) - 1, 2):
            # El PBS de Essentials usa -1 ("lo aprende AL EVOLUCIONAR") y
            # 0 ("ya lo sabe al nacer"); gen_moves los guarda los dos como
            # nivel 1, que es lo que significan aqui. Mi primer filtro era
            # `isdigit()`, que a "-1" le dice que no es un numero y lo
            # tiraba: salian 227 especies "con un golpe de mas" que lo
            # unico que tenian era un golpe de evolucion.
            try:
                lvl = max(1, int(bruto[i]))
            except ValueError:
                continue
            k = clave(bruto[i + 1])
            if k not in porNombre:
                # Golpe que NO esta en la tabla: gen_moves descarta los
                # que no harian nada (ni dano, ni fx, ni sx). Es
                # legitimo, pero se CUENTA y se nombra: si la lista
                # crece de golpe, es que se cayo algo que si valia.
                fuera[k] = fuera.get(k, 0) + 1
                continue
            quiere.add((lvl, porNombre[k]))
        # Forcejeo es el indice 0 de la tabla y el recurso de siempre:
        # gen_moves se lo da a quien se quedaria sin NINGUN golpe de
        # dano (Ditto, que solo tiene Transform; Smeargle, que solo
        # tiene Sketch). No esta en el PBS porque no se aprende.
        tiene = {p for p in apren[num] if p[1] != 0}
        if tiene != quiere:
            f, s = quiere - tiene, tiene - quiere
            det = []
            if f:
                det.append("le falta " + ", ".join(
                    f"{noms[i]}@{l}" for l, i in sorted(f)[:4]))
            if s:
                det.append("le sobra " + ", ".join(
                    f"{noms[i]}@{l}" for l, i in sorted(s)[:4]))
            difs.append(f"{filas[num]['nombre']}: " + "; ".join(det))
    cuenta("golpes", "aprendizaje por especie", difs, n)
    if fuera:
        top = sorted(fuera.items(), key=lambda x: -x[1])[:8]
        print(f"  {len(fuera)} golpes del PBS fuera de la tabla "
              f"(no harian nada): " + ", ".join(f"{k}x{v}" for k, v in top))


# Metodos del PBS que NO son por nivel. Quien solo tenga de estos no
# debe subir solo: su `evolveLevel` va a 0 y el camino es la piedra
# (`onlyByItem` en pet.cpp). La lista esta escrita entera a proposito:
# un metodo nuevo del Anil que no este aqui salta como desconocido en
# vez de colarse como si fuera por nivel.
NO_POR_NIVEL = {
    "Item", "ItemMale", "ItemFemale", "TradeItem", "Trade", "TradeSpecies",
    "DayHoldItem", "NightHoldItem", "Happiness", "HappinessDay",
    "HappinessNight", "AttackGreater", "DefenseGreater", "AtkDefEqual",
    "Silcoon", "Cascoon", "Ninjask", "Shedinja",
}

# Especies cuya evolucion en dex.h la eligio una persona, no el PBS.
# Son las que ramifican sin que ninguna rama sea "la" principal; el
# campo de dex.h solo decide cual ensena la ficha. Ver EVO_A_MANO en
# gen_filas.py, donde esta el porque de cada una.
EVO_ELEGIDA_A_MANO = {
    133: 134,   # Eevee -> Vaporeon en la ficha; las siete van en EEVEE_BRANCH
    236: 237,   # Tyrogue -> Hitmontop, el unico que SOLO sale de ahi
}

# Ramas que el PBS tiene y el juego NO.
#
# Esta VACIA desde 2026-10-04, y esa es la noticia. Habia cinco:
#
#   Burmy -> Mothim      NUNCA estuvo rota. La que estaba rota era esta
#                        herramienta, que miraba dex.h y evos.h y no
#                        DEX_SEXADA, que lo resolvia desde hacia tiempo.
#                        Informe de un fallo inventado, que gasta la
#                        misma confianza que callarse uno de verdad.
#   Tyrogue -> Lee/Chan  ahora salen comparando su Ataque y su Defensa
#   Wurmple -> Cascoon   ahora sale, a suertes pero fija por bicho
#   Nincada -> Shedinja  ahora se deja de propina si hay hueco
#
# El diccionario se queda, y con el la comprobacion de que no le sobre
# ninguna entrada: el Anil puede traer un metodo nuevo manana.
RAMAS_PERDIDAS = {}


def sec_atributos_golpes():
    """Que cada golpe tenga SU tipo, SU potencia, SUS PP y SU precision.

    Este hueco lo encontre contestando a "¿los golpes funcionan?": se
    comprobaba QUIEN aprende cada golpe (sec_golpes) y nadie comprobaba
    que el golpe en si fuera el del Anil. Un Lanzallamas con 40 de
    potencia habria pasado las 1025 comparaciones de aprendizaje sin
    que nada dijera nada.
    """
    print("--- los golpes, uno a uno ---")
    mv = lee("moves.h")
    cuerpo = pide("golpes", mv, r"MOVE_TBL\[[^\]]*\] = \{(.*?)\n\};",
                  "MOVE_TBL")
    if cuerpo is None:
        return
    # El `.*?` del final NO es pereza: la fila crece (acaba de ganar un
    # byte de banderas) y atarse al numero exacto de campos hace que
    # esto deje de leer nada al primer campo nuevo. Paso justo asi, y
    # lo canto el guardia de "cero filas" — que es para lo que esta.
    filas = re.findall(
        r'^\s*\{\s*"([^"]+)",\s*(T_\w+),\s*(\d+),\s*(\d+),\s*(\d+),\s*'
        r'\w+,\s*\w+,\s*\d+,\s*(\d+),\s*(-?\d+)\s*[,}]', cuerpo, re.M)
    if not filas:
        mal("golpes", "no leo ni una fila de MOVE_TBL")
        return

    # El PBS de golpes, leido por nuestra cuenta.
    pbs = {}
    for cab, c in anil_pbs.bloques(
            os.path.join(anil_pbs.ANIL, "PBS", "moves.txt")):
        pbs[clave(cab)] = c

    T = {"fighting": "T_FIGHT", "electric": "T_ELEC"}
    dt, dp, dpp, da, dc, dpr = [], [], [], [], [], []
    n = 0
    for nom, tipo, pot, pp, fis, acc, prio in filas:
        c = pbs.get(clave(nom))
        if c is None:
            # STRUGGLE se lo inventa gen_moves: no esta en el PBS y es
            # correcto que no este (no se aprende, es el recurso).
            if clave(nom) != "STRUGGLE":
                mal("golpes", f"{nom} no esta en el PBS del Anil")
            continue
        n += 1
        t = c.get("Type", "").strip().lower()
        esp = T.get(t, "T_" + t.upper())
        if esp != tipo:
            dt.append(f"{nom}: moves.h {tipo} / PBS {esp}")
        # El PBS deja Power en 1 (o en 0) en los que se calculan en
        # codigo, asi que esos no se comparan por numero.
        pPbs = int(re.sub(r"[^0-9]", "", c.get("Power", "0")) or 0)
        if pPbs > 1 and int(pot) != pPbs:
            dp.append(f"{nom}: moves.h {pot} de potencia / PBS {pPbs}")
        ppPbs = int(re.sub(r"[^0-9]", "", c.get("TotalPP", "0")) or 0)
        if ppPbs and int(pp) != ppPbs and nom not in PP_A_PROPOSITO:
            dpp.append(f"{nom}: moves.h {pp} PP / PBS {ppPbs}")
        # En Essentials, Accuracy = 0 quiere decir "NO FALLA NUNCA"
        # (Aerial Ace, Aura Sphere, Swift y los 100 de apoyo). AQUI SE
        # GUARDA IGUAL, un cero, y el motor salta la tirada con
        # `MOVE_TBL[mv].acc > 0`.
        #
        # Hasta el 2026-10-23 se guardaba como 100 "porque random(100) <
        # 100 es siempre cierto". No lo es: deja de serlo en cuanto hay
        # evasion o punteria de por medio, que es justo lo que esos
        # golpes existen para ignorar. El comentario de antes llamaba a
        # eso una traduccion; era una perdida de informacion, y los 219
        # golpes del PBS que nunca fallan fallaban.
        aPbs = int(re.sub(r"[^0-9]", "", c.get("Accuracy", "0")) or 0)
        if aPbs == 0:
            if int(acc) != 0:
                da.append(f"{nom}: el PBS dice que no falla nunca "
                          f"(Accuracy 0) y moves.h pone {acc} en vez de 0")
        elif int(acc) != aPbs:
            da.append(f"{nom}: moves.h {acc} de precision / PBS {aPbs}")
        # LA PRIORIDAD. Es el campo que acaba de entrar, y el que mas
        # facil es perder otra vez: no se ve en ninguna pantalla y un
        # golpe sin su escalon sigue funcionando —— solo sale en el
        # momento equivocado del turno.
        pPbs = int(c.get("Priority", "0") or 0)
        if int(prio) != pPbs:
            dpr.append(f"{nom}: moves.h prioridad {prio} / PBS {pPbs}")
        cat = c.get("Category", "").strip().lower()
        if cat in ("physical", "special"):
            esperado = "1" if cat == "physical" else "0"
            if fis != esperado:
                dc.append(f"{nom}: moves.h {'fisico' if fis == '1' else 'especial'}"
                          f" / PBS {cat}")
    cuenta("golpes", "tipo del golpe", dt, n)
    cuenta("golpes", "potencia", dp, n)
    cuenta("golpes", "PP", dpp, n)
    cuenta("golpes", "precision", da, n)
    cuenta("golpes", "prioridad", dpr, n)
    cuenta("golpes", "fisico o especial", dc, n)

    # Las BANDERAS. Entraron para Iron Fist y para que "de contacto"
    # dejara de adivinarse con "es fisico", asi que mas vale que digan
    # lo que dice el PBS.
    nombres = {"MV_CONTACT": "Contact", "MV_PUNCH": "Punching",
               "MV_POWDER": "Powder", "MV_SOUND": "Sound",
               "MV_BITE": "Biting"}
    # El `-?` NO es decoracion: al entrar la columna de prioridad, los
    # golpes de escalon negativo (Contraataque -5, Venganza -4, Rugido -6)
    # dejaron de casar y el verificador se quedo mirando 705 de 716 sin
    # que la diferencia significara nada.
    conBand = re.findall(
        r'^\s*\{\s*"([^"]+)",.*?,\s*-?\d+,\s*([A-Z_0-9| ]+)\s*\}',
        cuerpo, re.M)
    if len(conBand) < n:
        mal("golpes", f"leo las banderas de {len(conBand)} golpes y hay {n}")
        return
    df, nb = [], 0
    for nom, bandas in conBand:
        c = pbs.get(clave(nom))
        if c is None:
            continue
        nb += 1
        suyas = {x.strip() for x in bandas.split("|") if x.strip() != "0"}
        delPbs = {x.strip() for x in c.get("Flags", "").split(",")}
        for cte, pbsNom in nombres.items():
            tiene, deberia = cte in suyas, pbsNom in delPbs
            if tiene != deberia:
                df.append(f"{nom}: {cte} {'puesta' if tiene else 'sin poner'}"
                          f" y el PBS dice que {'si' if deberia else 'no'}")
    cuenta("golpes", "banderas (contacto, puno, polvo...)", df, nb)


def sec_evoluciones():
    """Que todo destino del PBS se pueda ALCANZAR de alguna forma.

    Esto es lo que de verdad importa y no se veia: no si el numero de
    dex.h coincide con el del PBS, sino si queda algun bicho al que no
    haya manera de llegar jugando. dex.h guarda UNA evolucion por
    especie y evos.h las de objeto; cuando el Anil ramifica (Tyrogue da
    tres, Wurmple dos), las ramas que no caben ahi desaparecen sin que
    nada se queje — el bicho sigue en la Pokedex y es inalcanzable.
    """
    print("--- evoluciones ---")
    filas = tabla_dex()
    if filas is None:
        return
    # Los TRES sitios donde puede vivir una evolucion, no dos.
    porItem = evos_item() | evos_rama()
    num = anil_pbs.numeros()
    nivel, sueltas, raros, conocidas, n = [], [], set(), set(), 0
    for d, _slug, c in anil_pbs.filas():
        if d not in filas:
            continue
        n += 1
        bruto = [x.strip() for x in c.get("Evolutions", "").split(",")]
        porNivel, destinos = None, []
        for i in range(0, len(bruto) - 2, 3):
            destino = num.get(clave(bruto[i]))
            metodo, arg = bruto[i + 1], bruto[i + 2]
            if not destino or destino > N_DEX:
                continue
            destinos.append((destino, metodo))
            if metodo == "Level" and arg.isdigit():
                if porNivel is None:
                    porNivel = (destino, int(arg))
            elif metodo not in NO_POR_NIVEL and not metodo.startswith("Level"):
                raros.add(metodo)
        # La AMISTAD se convierte en un nivel: aqui no hay un contador
        # de carino que el jugador pueda ver subir, asi que gen_filas la
        # traduce segun lo FUERTE QUE SEA EL DESTINO (15/25/35 por BST).
        # Se rehace la cuenta en vez de importarla, que es el sentido de
        # este fichero.
        # Las ramas (AttackGreater, Cascoon, LevelMale...) tambien pasan
        # a un nivel: traen el suyo en el argumento. dex.h se queda con
        # una, y cual es se comprueba abajo, en las ramas perdidas.
        if porNivel is None:
            for i in range(0, len(bruto) - 2, 3):
                destino = num.get(clave(bruto[i]))
                if (destino and destino <= N_DEX and bruto[i + 2].isdigit()
                        and destino == filas[d]["evo"]):
                    porNivel = (destino, int(bruto[i + 2]))
                    break
        if porNivel is None:
            for destino, metodo in destinos:
                if metodo.startswith("Happiness"):
                    bst = sum(filas[destino]["stats"]) if destino in filas else 0
                    porNivel = (destino,
                                15 if bst < 350 else (25 if bst < 500 else 35))
                    break
        if d in EVO_ELEGIDA_A_MANO:
            # Eevee tiene siete ramas y ninguna es "la" suya: el campo de
            # dex.h solo decide cual ensena la ficha, y las siete las
            # lleva EEVEE_BRANCH en pet.cpp. Se comprueba que siga siendo
            # la elegida y no se compara con el PBS, que diria otra.
            if filas[d]["evo"] != EVO_ELEGIDA_A_MANO[d]:
                nivel.append(f"{filas[d]['nombre']}: la ficha deberia "
                             f"ensenar {EVO_ELEGIDA_A_MANO[d]} y ensena "
                             f"{filas[d]['evo']}")
        elif porNivel:
            if (filas[d]["evo"], filas[d]["lvl"]) != porNivel:
                nivel.append(f"{filas[d]['nombre']}: dex.h a "
                             f"{filas[d]['evo']} al nivel {filas[d]['lvl']}, "
                             f"el PBS a {porNivel[0]} al {porNivel[1]}")
        elif filas[d]["lvl"]:
            nivel.append(f"{filas[d]['nombre']}: dex.h lo sube solo al nivel "
                         f"{filas[d]['lvl']} y el PBS no tiene ninguna por "
                         f"nivel")
        # y que a cada destino se pueda llegar EVOLUCIONANDO
        for destino, metodo in destinos:
            if filas[d]["evo"] == destino or (d, destino) in porItem:
                continue
            if (d, destino) in RAMAS_PERDIDAS:
                conocidas.add((d, destino))
                continue
            sueltas.append(
                f"{filas[d]['nombre']} -> "
                f"{filas.get(destino, {}).get('nombre', destino)} "
                f"({metodo}): el PBS la tiene y aqui no hay forma de "
                f"llegar evolucionando")
    cuenta("evoluciones", "evolucion por nivel", nivel, n)
    cuenta("evoluciones", "ramas del PBS que llegan", sueltas, n)
    print(f"  {len(conocidas)} ramas perdidas a proposito (ver "
          f"RAMAS_PERDIDAS en este fichero)")
    olvidadas = set(RAMAS_PERDIDAS) - conocidas
    if olvidadas:
        # Una excepcion que ya no hace falta es tan mala como una que
        # falta: deja de proteger nada y nadie se entera.
        mal("evoluciones", "RAMAS_PERDIDAS tiene ramas que el PBS ya no "
                           "pide: " + ", ".join(str(x) for x in olvidadas))
    if raros:
        mal("evoluciones", "metodos del PBS que no conozco (ni por nivel ni "
                           "en NO_POR_NIVEL): " + ", ".join(sorted(raros)))


def main():
    secs = {
        "amano": sec_a_mano,
        "stats": sec_stats,
        "habilidades": sec_habilidades,
        "golpes": sec_golpes,
        "atributos": sec_atributos_golpes,
        "evoluciones": sec_evoluciones,
    }
    pedidas = [a for a in sys.argv[1:] if not a.startswith("-")]
    if pedidas and any(p not in secs for p in pedidas):
        raise SystemExit("secciones: " + ", ".join(secs))
    for nom, fn in secs.items():
        if pedidas and nom not in pedidas:
            continue
        fn()

    print()
    if fallos:
        print(f"--- {len(fallos)} PROBLEMAS ---")
        for s, m in fallos[:60]:
            print(f"  ! [{s}] {m}")
        if len(fallos) > 60:
            print(f"  ... y {len(fallos) - 60} mas")
        return 1
    print("lo que se embarca coincide con el PBS del Anil")
    return 0


if __name__ == "__main__":
    sys.exit(main())
