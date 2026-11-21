#!/usr/bin/env python3
"""Que EFECTO de golpe y que HABILIDAD no prueba nadie.

La pregunta del usuario era: "que os efeitos de golpes e habilidades
estejam funcionando (matematicamente/programaticamente)". Para eso hay
que saber primero cuales se ejecutan alguna vez, y eso no se puede
responder marcando a mano que test cubre que — marcar es opinar, y la
marca se queda puesta cuando el test deja de cubrirlo.

Asi que lo cuenta el compilador. `make cover` compila la bateria con
--coverage; gcc anota cuantas veces paso por cada linea, y aqui se
cruzan esos contadores con las lineas donde battle.cpp decide cada
efecto. Una linea `case FX_DRAIN:` con cero pasadas significa que en
496 tests nadie drena vida. Eso no es una sospecha, es el contador.

Lo que SI prueba: que el efecto se ejecuta y, por tanto, que algun test
llega hasta el. Lo que NO prueba: que el numero que sale sea el bueno
— eso lo dicen los CHECK_EQ de ese test. Por eso el informe separa los
efectos que no se ejecutan (agujero seguro) de los que se ejecutan
(hay que fiarse del test que los toca).

  python3 tools/cobertura.py            # compila, corre y resume
  python3 tools/cobertura.py --sin-compilar
"""
import io
import os
import re
import subprocess
import sys

AQUI = os.path.dirname(os.path.abspath(__file__))
RAIZ = os.path.dirname(AQUI)
TEST = os.path.join(RAIZ, "test")
COV = os.path.join(TEST, "build", "cov")

# Los ficheros con logica de efectos y de donde sale cada familia.
FUENTES = ["battle.cpp", "pet.cpp"]

# Todo lo que se escribio a mano, para poder decir CUANTO esta bajo
# prueba y cuanto no. La pregunta "¿lo probaste todo?" merece un numero,
# no una impresion.
TODO_EL_CODIGO = ["pet.cpp", "battle.cpp", "i18n.cpp", "sdmon.cpp",
                  "audio.cpp", "rtcbat.cpp", "TamaPoke.ino",
                  "battle_ui.ino", "capture.ino", "intro.ino"]
# Los que entran en la bateria (ver test/Makefile).
EN_LA_BATERIA = ["pet.cpp", "battle.cpp", "i18n.cpp", "sdmon.cpp",
                 "TamaPoke.ino", "battle_ui.ino", "capture.ino",
                 "intro.ino"]
# Los cuatro .ino se compilan PEGADOS en un solo fichero, como hace el
# IDE de Arduino, asi que gcov da un numero para los cuatro juntos.
INOS = ["TamaPoke.ino", "battle_ui.ino", "capture.ino", "intro.ino"]
SKETCH_GCDA = "tamapoke_uicov-sketch.gcda"
SKETCH_GCOV = "sketch.cpp.gcov"

# Como se reconoce que una linea DECIDE un efecto. battle.cpp usa TRES
# formas y hay que leer las tres, porque leer solo los `case` daba 32
# falsos positivos: los 28 cambios de stat (SX_LOWER_ATK..SX_RAISE2_EVA)
# no tienen case ninguno, se despachan por RANGO y se indexa una tabla
# con la resta. Darlos por no implementados era mentira.
# El lado IZQUIERDO de la comparacion no se mira, y hay un motivo: el
# patron decia `abil() == AB_X`, y en cuanto la habilidad del rival paso
# por una variable (`defAb`, que es lo que hizo falta para Mold Breaker)
# cuatro codigos desaparecieron del informe. No fallo nada — eso es lo
# malo: salieron de la lista de "probados" y no entraron en la de "sin
# probar". Lo canto la comprobacion de que la cuenta cuadre.
UNO = re.compile(
    r"\bcase\s+((?:FX|SX|AB)_[A-Z0-9_]+)\s*:"
    r"|[!=]=\s*((?:FX|SX|AB)_[A-Z0-9_]+)\b")

# `m.sx >= SX_LOWER_ATK && m.sx <= SX_LOWER_EVA` cubre TODO el tramo.
RANGO = re.compile(
    r">=\s*((?:FX|SX|AB)_[A-Z0-9_]+)\s*&&[^&|]*?<=\s*((?:FX|SX|AB)_[A-Z0-9_]+)")

# Codigos que existen en el enum pero que NINGUN golpe ni especie lleva:
# no se les puede pedir cobertura, no hay con que dispararlos.
NO_SE_USAN = {"FX_NONE", "SX_NONE", "AB_NONE"}


def corre(cmd, cwd):
    return subprocess.run(cmd, cwd=cwd, capture_output=True, text=True,
                          shell=False)


def orden_enums():
    """{codigo: su valor} leyendo los enum en el orden en que se declaran.

    Hace falta para los rangos: sin saber que entre SX_LOWER_ATK y
    SX_LOWER_EVA hay siete, no se puede decir a cuantos cubre una sola
    linea.
    """
    out = {}
    for fich, pref in (("moves.h", ("FX_", "SX_")),
                       ("abilities.h", ("AB_",))):
        txt = io.open(os.path.join(RAIZ, fich), encoding="utf-8",
                      errors="replace").read()
        for m in re.finditer(r"enum\s*(?::\s*\w+\s*)?\{(.*?)\};", txt, re.S):
            n = 0
            for linea in m.group(1).splitlines():
                linea = re.sub(r"//.*", "", linea)
                for item in linea.split(","):
                    item = item.strip()
                    if not item:
                        continue
                    mm = re.match(r"^([A-Z][A-Z0-9_]*)\s*(?:=\s*(\d+))?$",
                                  item)
                    if not mm:
                        continue
                    if mm.group(2) is not None:
                        n = int(mm.group(2))
                    if mm.group(1).startswith(pref):
                        out[mm.group(1)] = n
                    n += 1
    return out


def usados_de_verdad():
    """{FX_x, SX_x, AB_x} que alguna fila de las tablas lleva encima.

    Pedir cobertura de un codigo que ningun golpe usa seria pedir lo
    imposible; y al reves, un codigo que SI usa un golpe y que no se
    ejecuta nunca es un agujero de verdad.
    """
    out = set()
    mv = io.open(os.path.join(RAIZ, "moves.h"), encoding="utf-8",
                 errors="replace").read()
    cuerpo = mv[mv.index("MOVE_TBL"):]
    for m in re.finditer(r"\b(FX_[A-Z0-9_]+|SX_[A-Z0-9_]+)\b", cuerpo):
        out.add(m.group(1))
    ab = io.open(os.path.join(RAIZ, "abilities.h"), encoding="utf-8",
                 errors="replace").read()
    cuerpo = ab[ab.index("ABIL_TBL"):]
    # SOLO la segunda columna. La tercera es el ARGUMENTO, y ahi viven
    # los AB_WE_* (Drought es { "Drought", AB_SET_WEATHER, AB_WE_SUN }):
    # AB_WE_SUN no es un efecto que despachar, es "sol". Cogiendo la
    # fila entera salian cuatro "habilidades sin implementar" que no
    # existen.
    for m in re.finditer(r'^\s*\{\s*"[^"]*"\s*,\s*(AB_[A-Z0-9_]+)\s*,',
                         cuerpo, re.M):
        out.add(m.group(1))
    return out - NO_SE_USAN


def gcov(fuente):
    """{numero de linea: veces que se ejecuto} para un .cpp.

    El .gcda se llama por el BINARIO, no por la fuente:
    build/tamapoke_tests_cov-battle.gcda. Y se borra el .gcov viejo
    antes de pedir el nuevo: leer sin querer el de la corrida anterior
    daria una cobertura perfecta y falsa, que es el tipo de mentira que
    este proyecto ya ha pagado varias veces.
    """
    f = os.path.join(TEST, fuente + ".gcov")
    if os.path.exists(f):
        os.remove(f)
    base = os.path.splitext(os.path.basename(fuente))[0]
    gcda = os.path.join("build", f"tamapoke_tests_cov-{base}.gcda")
    if not os.path.exists(os.path.join(TEST, gcda)):
        return None
    corre(["gcov", "-r", gcda], TEST)
    if not os.path.exists(f):
        return None
    cuentas = {}
    for ln in io.open(f, encoding="utf-8", errors="replace"):
        partes = ln.split(":", 2)
        if len(partes) < 3:
            continue
        veces, num = partes[0].strip(), partes[1].strip()
        if not num.isdigit():
            continue
        if veces in ("-",):
            continue
        cuentas[int(num)] = 0 if veces in ("#####", "=====") else \
            int(re.sub(r"[^0-9]", "", veces) or 0)
    return cuentas


def gcov_sketch():
    """Contadores del sketch (los cuatro .ino pegados), si los hay.

    Los deja `make uicover`, que es una bateria APARTE: el sketch
    necesita los shims del simulador y chocan con los de test/shim.
    """
    f = os.path.join(TEST, SKETCH_GCOV)
    if os.path.exists(f):
        os.remove(f)
    gcda = os.path.join("build", SKETCH_GCDA)
    if not os.path.exists(os.path.join(TEST, gcda)):
        return None
    corre(["gcov", "-r", gcda], TEST)
    if not os.path.exists(f):
        return None
    cuentas = {}
    for ln in io.open(f, encoding="utf-8", errors="replace"):
        partes = ln.split(":", 2)
        if len(partes) < 3:
            continue
        veces, num = partes[0].strip(), partes[1].strip()
        if not num.isdigit() or veces == "-":
            continue
        cuentas[int(num)] = 0 if veces in ("#####", "=====") else \
            int(re.sub(r"[^0-9]", "", veces) or 0)
    return cuentas


def lineas_de(fich):
    p = os.path.join(RAIZ, fich)
    if not os.path.exists(p):
        return 0
    return sum(1 for _ in io.open(p, encoding="utf-8", errors="replace"))


def cuanto_del_total():
    """Cuanto del codigo escrito a mano pasa por la bateria, y cuanto no.

    Sin esto, "¿esta todo probado?" se contesta de memoria, y de memoria
    se contesta que si. Los .ino son la mitad del proyecto y no tienen
    ni un test: lo que los ejercita es el simulador del escritorio, que
    no afirma nada, solo dibuja.
    """
    print("--- cuanto del codigo pasa por la bateria ---")
    dentro = fuera = 0
    sueltos = []
    for f in TODO_EL_CODIGO:
        n = lineas_de(f)
        if f in EN_LA_BATERIA:
            dentro += n
        else:
            fuera += n
            sueltos.append((n, f))
    tot = dentro + fuera
    if not tot:
        print("  no encuentro las fuentes")
        return
    print(f"  en la bateria: {dentro} lineas ({100 * dentro // tot}% del "
          f"codigo a mano)")
    for f in EN_LA_BATERIA:
        if f in INOS:
            continue
        c = gcov(f)
        if not c:
            continue
        t = sum(1 for v in c.values() if v > 0)
        print(f"     {f:<14s} {100 * t // len(c) if c else 0}% de sus "
              f"{len(c)} lineas ejecutables")
    c = gcov_sketch()
    if c:
        t = sum(1 for v in c.values() if v > 0)
        print(f"     los 4 .ino    {100 * t // len(c)}% de sus {len(c)} "
              f"lineas ejecutables (van PEGADOS en un fichero, como en "
              f"el IDE, asi que gcov los cuenta juntos)")
    else:
        print("     los 4 .ino    SIN MEDIR: falta `make uicover`")
    if sueltos:
        print(f"  SIN tests: {fuera} lineas ({100 * fuera // tot}%) — "
              f"puro driver (I2S, I2C, PMU), no hay nada que afirmar "
              f"sin la placa:")
        for n, f in sorted(sueltos, reverse=True):
            print(f"     {f:<14s} {n}")
    print()


def main():
    if "--sin-compilar" not in sys.argv:
        make = "mingw32-make" if os.name == "nt" else "make"
        # Son DOS baterias con contadores: la de la logica y la de la
        # interfaz, que vive en su propio binario porque el sketch
        # necesita los shims del simulador.
        for objetivo, que in (("cover", "logica"), ("uicover", "interfaz")):
            print(f"compilando y corriendo la bateria de {que}...")
            r = corre([make, objetivo], TEST)
            if r.returncode != 0:
                print(r.stdout[-3000:])
                print(r.stderr[-3000:])
                raise SystemExit(f"no compila `make {objetivo}`")
            lineas = [x for x in r.stdout.strip().splitlines() if "tests," in x]
            print("  " + (lineas[-1] if lineas else "(sin salida)"))

    cuanto_del_total()
    usa = usados_de_verdad()
    valores = orden_enums()
    faltan = usa - set(valores)
    if faltan:
        raise SystemExit("no encuentro en ningun enum: " +
                         ", ".join(sorted(faltan)))
    print(f"\n{len(usa)} codigos de efecto los lleva alguna fila de las "
          f"tablas")

    fríos, calientes, sin_sitio, sin_contador = {}, {}, set(usa), {}
    for fuente in FUENTES:
        cuentas = gcov(fuente)
        if cuentas is None:
            raise SystemExit(f"no hay contadores de {fuente}: ¿corrio "
                             f"`make cover`?")
        texto = io.open(os.path.join(RAIZ, fuente), encoding="utf-8",
                        errors="replace").read().splitlines()
        for i, linea in enumerate(texto, 1):
            codigos = set()
            for m in UNO.finditer(linea):
                codigos.add(next(g for g in m.groups() if g))
            for m in RANGO.finditer(linea):
                lo, hi = valores.get(m.group(1)), valores.get(m.group(2))
                fam = m.group(1).split("_")[0]
                if lo is None or hi is None or lo > hi:
                    continue
                codigos |= {c for c, v in valores.items()
                            if c.startswith(fam + "_") and lo <= v <= hi}
            for cod in codigos:
                if cod in NO_SE_USAN:
                    continue
                sin_sitio.discard(cod)
                veces = cuentas.get(i)
                if veces is None:
                    # Linea sin contador: un `case X:` pelado que cae en
                    # el siguiente no genera codigo propio. Se apunta,
                    # porque si TODAS las lineas de un codigo son asi se
                    # quedaria fuera del informe sin que nadie lo note —
                    # y un codigo que no sale ni como probado ni como sin
                    # probar es el peor de los tres sitios donde estar.
                    sin_contador.setdefault(cod, f"{fuente}:{i}")
                    continue
                if veces > 0:
                    calientes[cod] = calientes.get(cod, 0) + veces
                    fríos.pop(cod, None)
                elif cod not in calientes:
                    fríos[cod] = f"{fuente}:{i}"

    def fam(c):
        return c.split("_")[0]

    print()
    for f, como in (("FX", "efectos sobre el dano"),
                    ("SX", "efectos de estado"),
                    ("AB", "habilidades")):
        cal = sorted(c for c in calientes if fam(c) == f)
        fri = sorted(c for c in fríos if fam(c) == f)
        tot = len(cal) + len(fri)
        if not tot:
            continue
        pct = 100 * len(cal) // tot
        print(f"{como}: {len(cal)}/{tot} se ejecutan en la bateria ({pct}%)")
        if fri:
            print("   NADIE los prueba: " + ", ".join(fri))

    # Un codigo que las tablas usan y que no tiene NI UNA linea que lo
    # mire es peor que uno sin test: es un efecto que el juego cree
    # tener y que no esta implementado en ninguna parte.
    if sin_sitio:
        print(f"\n{len(sin_sitio)} codigos que algun golpe o especie lleva y "
              f"que NO se miran en ningun sitio del codigo:")
        for c in sorted(sin_sitio):
            print(f"   {c}")

    # Los que no caen ni en probados ni en sin probar: sus unicas lineas
    # no generan codigo. No son un fallo, pero TIENEN que salir, o el
    # informe cuadraria dando por cubierto algo que no ha mirado.
    perdidos = sorted(set(sin_contador) - set(calientes) - set(fríos))
    if perdidos:
        print(f"\n{len(perdidos)} sin contador propio (su `case` cae en el "
              f"siguiente; se cubren con el de al lado):")
        for c in perdidos:
            print(f"   {c}  {sin_contador[c]}")

    # Los binarios instrumentados, FUERA. No hacen falta una vez leidos
    # los contadores, y un .exe recien compilado, sin firmar, que al
    # salir escribe dos docenas de ficheros en su propia carpeta es un
    # candidato de libro a que un antivirus lo marque por heuristica.
    # Le paso al usuario con tamapoke_uicov.exe y Kaspersky. Borrarlos
    # no arregla el aviso de aquella vez, pero quita el motivo de que
    # vuelva a pasar.
    for b in ("tamapoke_tests_cov", "tamapoke_uicov"):
        for ext in ("", ".exe"):
            p = os.path.join(TEST, "build", b + ext)
            if os.path.exists(p):
                try:
                    os.remove(p)
                except OSError:
                    pass

    # Que TODO codigo de las tablas haya acabado en alguna columna. El
    # informe puede tener de mas (codigo que despacha un efecto que hoy
    # no lleva ningun golpe, lo cual no estorba), pero no puede tener
    # de menos: un codigo que se evapora entre las tres listas se leeria
    # como cubierto sin que nadie lo haya mirado.
    mirados = set(calientes) | set(fríos) | set(perdidos)
    huerfanos = sorted(usa - mirados)
    if huerfanos:
        print(f"\nLA CUENTA NO SALE: estos {len(huerfanos)} estan en las "
              f"tablas y no salen en ninguna columna de arriba: "
              + ", ".join(huerfanos))
        return 1
    extra = sorted(mirados - usa)
    if extra:
        print(f"\n{len(extra)} efectos que el codigo sabe hacer y que hoy no "
              f"lleva ningun golpe ni especie: " + ", ".join(extra))

    fallo = bool(fríos) or bool(sin_sitio)
    print()
    if fallo:
        print("hay efectos sin probar (arriba)")
    else:
        print("todos los efectos que alguna fila usa se ejecutan en la "
              "bateria")
    return 1 if fallo else 0


if __name__ == "__main__":
    sys.exit(main())
