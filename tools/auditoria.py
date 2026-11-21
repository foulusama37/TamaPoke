#!/usr/bin/env python3
"""Auditoria EXHAUSTIVA: todo bicho, todo golpe, todo sprite.

Por que existe, dicho por el usuario: "nao tenho condicoes de checar
todos os golpes e todos os pokemons, mas cada vez que olho, acho um
erro". Tiene razon, y la respuesta no es prometer mejor: es que la
maquina mire las 1025 una a una y diga lo que no cuadra.

Lo que mira, y lo que NO:

  SI, aqui              una maquina puede decidirlo solo
    que el .bin de cada bicho EXISTA y se pueda LEER entero (no vale
      con que el fichero este: el barrido viejo solo miraba eso)
    que cada especie tenga golpes utiles a varios niveles
    que cada especie tenga habilidad, grito, miniatura y ficha
    que cada evolucion llegue a su forma final antes del nivel tope
    que cada mega y cada forma tengan su sprite, su piedra y su tabla
    que el efecto de un golpe sea el que el Anil le asigna
    que el SONIDO de cada golpe sea el suyo y no el del de al lado

  NO, eso va en las hojas de contacto (tools/hoja_contacto.py)
    si el dibujo es el que le toca a ese bicho. Eso solo lo ve un ojo,
    y por eso lo otro se saca en PNG para mirarlo de un vistazo.

  python3 tools/auditoria.py            # lo rapido (~1 min)
  python3 tools/auditoria.py --hondo    # ademas descomprime cada sprite
  python3 tools/auditoria.py --seccion sprites
"""
import io
import os
import re
import struct
import sys

AQUI = os.path.dirname(os.path.abspath(__file__))
RAIZ = os.path.dirname(AQUI)
sys.path.insert(0, AQUI)
from dex_data import DEX              # noqa: E402
from dex_range import N_DEX, REGIONES  # noqa: E402

SD = os.path.join(AQUI, "sdcard")
MONS = os.path.join(SD, "mons")
NOMBRE = {d[0]: d[2] for d in DEX}

fallos, avisos = [], []


def mal(sec, msg):
    fallos.append((sec, msg))


def ojo(sec, msg):
    avisos.append((sec, msg))


def lee(n):
    return io.open(os.path.join(RAIZ, n), encoding="utf-8",
                   errors="replace").read()


def constante(txt, nom):
    m = re.search(r"#define %s\s+(\d+)" % nom, txt)
    return int(m.group(1)) if m else None


def pide(sec, txt, patron, que):
    """Busca algo que TIENE que estar. Si no esta, es un fallo.

    Esta es la pieza que hace que la auditoria valga de algo. Un patron
    que deja de casar (porque la tabla cambio de forma) no da error: da
    CERO, y cero se lee como "todo bien". Ya paso aqui mismo — el patron
    del aprendizaje decia `LEARN_OFS[]` y la tabla se declara
    `LEARN_OFS[DEX_COUNT + 1]`, asi que informaba de "0 sin golpes" sin
    haber mirado una sola especie.
    """
    m = re.search(patron, txt, re.S)
    if not m:
        mal(sec, f"NO ENCUENTRO {que}: esta comprobacion no ha mirado nada")
        return None
    return m.group(1)


# ------------------------------------------------------------ sprites
def lee_tpk2(path, hondo):
    """Valida un .bin de sprite. Devuelve (ok, motivo, nFrames).

    No basta con que el fichero este: puede estar truncado, tener el
    numero de acciones mal o apuntar fuera de si mismo, y entonces el
    aparato lo rechaza o pinta basura. El firmware hace estas mismas
    comprobaciones en PmdMon::load.
    """
    try:
        d = io.open(path, "rb").read()
    except OSError as e:
        return False, f"no se puede leer ({e.strerror})", 0
    if len(d) < 7:
        return False, "vacio o truncado", 0
    if d[:4] != b"TPK2":
        return False, f"cabecera {d[:4]!r}, no TPK2", 0
    nacts, npal = struct.unpack_from("<BH", d, 4)
    if not 1 <= nacts <= 12:
        return False, f"{nacts} acciones, fuera de 1..12", 0
    if npal > 256:
        return False, f"{npal} colores, el formato aguanta 256", 0
    off = 7 + npal * 2
    total = 0
    for a in range(nacts):
        if off + 6 > len(d):
            return False, f"se acaba en la accion {a}", total
        aid, w, h, nf = struct.unpack_from("<BBBB", d, off)[0:4]
        off += 4
        if off + nf * 2 > len(d):
            return False, f"accion {aid}: faltan tiempos", total
        off += nf * 2
        px = w * h * nf
        if off + px > len(d):
            return False, (f"accion {aid}: pide {px} px y quedan "
                           f"{len(d) - off}"), total
        if hondo:
            # tocarlos de verdad: un indice fuera de la paleta pinta mal
            trozo = d[off:off + px]
            peor = max(trozo) if trozo else 0
            if peor != 0xFF and peor >= npal:
                return False, (f"accion {aid}: indice {peor} con "
                               f"{npal} colores"), total
        off += px
        total += nf
    return True, "", total


def sec_sprites(hondo):
    print("--- sprites de la tarjeta ---")
    n = malos = 0
    for d in range(1, N_DEX + 1):
        for pre in ("p", "ps"):
            f = os.path.join(MONS, f"{pre}{d:03d}.bin")
            if not os.path.exists(f):
                mal("sprites", f"{d} {NOMBRE.get(d, '?')}: falta {pre}{d:03d}.bin")
                malos += 1
                continue
            ok, por, _nf = lee_tpk2(f, hondo)
            n += 1
            if not ok:
                mal("sprites", f"{d} {NOMBRE.get(d, '?')} ({pre}): {por}")
                malos += 1
    print(f"  {n} ficheros leidos, {malos} con problema")

    # Las HEMBRAS: female.h dice con un bit quien trae dibujo propio, y
    # el aparato se fia de ese bit para pedir el fichero. Si el bit dice
    # que si y el fichero no esta, el combate se queda sin dibujo — asi
    # que lo que hay que comprobar es justamente que el bit y la tarjeta
    # digan lo mismo. En los dos sentidos: un fichero que nadie va a
    # pedir son 600 KB tirados.
    fem = lee("female.h")
    bits = [int(x, 16) for x in re.findall(r"0x([0-9A-F]{2})", fem)]
    dice = {d for d in range(1, N_DEX + 1)
            if d >> 3 < len(bits) and (bits[d >> 3] >> (d & 7)) & 1}
    hay = set()
    nf = malasf = 0
    for d in sorted(dice):
        for pre in ("h", "hs"):
            f = os.path.join(MONS, f"{pre}{d:03d}.bin")
            if not os.path.exists(f):
                mal("hembras", f"{d} {NOMBRE.get(d, '?')}: female.h dice que "
                               f"tiene hembra y falta {pre}{d:03d}.bin")
                malasf += 1
                continue
            ok, por, _x = lee_tpk2(f, hondo)
            nf += 1
            if not ok:
                mal("hembras", f"{d} {NOMBRE.get(d, '?')} ({pre}): {por}")
                malasf += 1
    for f in os.listdir(MONS) if os.path.isdir(MONS) else []:
        m = re.match(r"^hs?(\d{3})\.bin$", f)
        if m:
            hay.add(int(m.group(1)))
    sobran = sorted(hay - dice)
    if sobran:
        mal("hembras", f"{len(sobran)} hembras empaquetadas que female.h no "
                       f"marca y nadie va a pedir: {sobran[:8]}")
    print(f"  hembras: {len(dice)} marcadas, {nf} ficheros leidos, "
          f"{malasf} con problema")

    # megas y formas, por fila de su tabla
    for etq, pref, fich, tabla in (
            ("megas", "m", "megas.h", "MEGA_TBL[MEGA_COUNT]"),
            ("formas", "v", "forms.h", "FORM_TBL[FORM_COUNT]")):
        if not os.path.exists(os.path.join(RAIZ, fich)):
            continue
        txt = lee(fich)
        cuantas = constante(txt, "MEGA_COUNT" if pref == "m" else "FORM_COUNT")
        falta = 0
        for i in range(cuantas or 0):
            f = os.path.join(MONS, f"{pref}{i:03d}.bin")
            if not os.path.exists(f):
                mal(etq, f"fila {i}: falta {pref}{i:03d}.bin")
                falta += 1
                continue
            ok, por, _nf = lee_tpk2(f, hondo)
            if not ok:
                mal(etq, f"fila {i}: {por}")
                falta += 1
        print(f"  {cuantas} {etq}, {falta} con problema")
        duplicados(etq, pref, txt, "MEGA_TBL" if pref == "m" else "FORM_TBL",
                   cuantas or 0)


def duplicados(etq, pref, txt, tabla, cuantas):
    """Dos filas con el MISMO dibujo byte a byte.

    No siempre es un fallo nuestro: las dos megas de Toxtricity comparten
    sprite porque el PROPIO Anil trae TOXTRICITY_2.png y _3.png
    identicos. Pero si un dia el empaquetador se equivoca de fichero,
    asi se ve — y de paso queda apuntado lo que ya se sabe, para no
    volver a investigarlo desde cero.
    """
    import hashlib
    nombres = re.findall(r'^\s*\{\s*\d+,.*?"([^"]*)" \}',
                         txt[txt.index(tabla + "["):], re.M)
    h = {}
    for i in range(cuantas):
        f = os.path.join(MONS, f"{pref}{i:03d}.bin")
        if not os.path.exists(f):
            continue
        k = hashlib.md5(io.open(f, "rb").read()).hexdigest()
        nom = nombres[i] if i < len(nombres) else str(i)
        if k in h:
            ojo(etq, f"{h[k]} y {nom} llevan el MISMO dibujo")
        else:
            h[k] = nom


# ------------------------------------------------------------- golpes
def sec_golpes():
    print("--- golpes ---")
    mv = lee("moves.h")
    n = constante(mv, "MOVE_COUNT")
    bloque = mv[mv.index("MOVE_TBL"):]
    # LA FILA ENTERA, Y CON NOMBRE.
    #
    # Antes esto cogia los cinco primeros campos y los desempaquetaba como
    # `nom, tipo, pot, pp, acc`. El quinto NO es la precision: es `phys`
    # (1 fisico / 0 especial). O sea que "393 que no fallan nunca" queria
    # decir "393 especiales", y el `if acc > 100` de debajo no podia
    # saltar jamas porque leia un campo que solo vale 0 o 1.
    #
    # Un verificador que mide la columna de al lado no avisa de nada y
    # encima da confianza. Se nombran los grupos para que anadir un campo
    # —— como acaba de pasar con `prio` —— no vuelva a correrlos todos.
    filas = re.findall(
        r'^\s*\{\s*"(?P<nom>[^"]+)",\s*(?P<tipo>T_\w+),\s*(?P<pot>\d+),'
        r'\s*(?P<pp>\d+),\s*(?P<phys>\d+),\s*(?P<fx>FX_\w+),'
        r'\s*(?P<sx>SX_\w+),\s*(?P<ch>\d+),\s*(?P<acc>\d+),'
        r'\s*(?P<prio>-?\d+),', bloque, re.M)
    if not filas:
        mal("golpes", "NO LEO MOVE_TBL: esta comprobacion no mira nada")
        return
    if len(filas) != n:
        mal("golpes", f"MOVE_TBL tiene {len(filas)} filas y MOVE_COUNT {n}")
    sinPot = sinAcc = conPrio = 0
    for nom, tipo, pot, pp, phys, fx, sx, ch, acc, prio in filas:
        if not nom:
            mal("golpes", "golpe sin nombre")
        if len(nom) > 15:
            mal("golpes", f"{nom}: {len(nom)} chars, la carta da para 15")
        if int(pp) == 0:
            mal("golpes", f"{nom}: 0 PP, no se puede usar nunca")
        if int(pot) == 0:
            sinPot += 1
        # acc 0 = NUNCA FALLA (asi lo marca el PBS). No es un error: es el
        # dato. Lo que si lo seria es pasarse de 100.
        if int(acc) == 0:
            sinAcc += 1
        if int(acc) > 100:
            mal("golpes", f"{nom}: precision {acc}")
        if int(ch) > 100:
            mal("golpes", f"{nom}: efecto al {ch}%")
        if not -7 <= int(prio) <= 5:
            mal("golpes", f"{nom}: prioridad {prio} fuera de la escala")
        if int(prio) != 0:
            conPrio += 1
    # Los dos numeros que valdria la pena ver bajar sin querer: si alguno
    # se va a cero, es que un campo dejo de llegar del PBS.
    # Los numeros son de LO EMBARCADO (717 de los ~900 del PBS), medidos
    # el 2026-10-23: 142 que nunca fallan y 40 con prioridad. El tope esta
    # holgado a proposito —— esto no vigila el dato exacto, vigila que el
    # campo siga LLEGANDO. Los dos han estado a cero sin que nadie se
    # enterase: la precision porque el generador aplanaba el 0 a 100, y la
    # prioridad porque el campo no existia.
    if sinAcc < 100:
        mal("golpes", f"solo {sinAcc} golpes que nunca fallan y eran 142: "
                      "alguien volvio a aplanar el Accuracy 0 del PBS")
    if conPrio < 30:
        mal("golpes", f"solo {conPrio} golpes con prioridad y eran 40: "
                      "el campo Priority dejo de llegar")
    print(f"  {len(filas)} golpes; {sinPot} sin potencia (de apoyo), "
          f"{sinAcc} que no fallan nunca, {conPrio} con prioridad")

    # Que NINGUNA descripcion salga en español. El PBS del Anil esta en
    # español y se traduce con el .dat del propio Anil, pero la
    # traduccion va por la FRASE ENTERA: si falta una, el generador se
    # quedaba con la española y la embarcaba. Pasaron cuatro, y dos eran
    # PLACAJE y GOLPE CUERPO — los golpes del primer combate de la
    # partida. Se ve en la tarjeta del golpe y no lo cantaba nada.
    #
    # Las palabras son de funcion y van entre espacios a proposito: son
    # las que no existen como palabra suelta en ingles.
    ES = (" el ", " la ", " los ", " las ", " del ", " que ", " con ",
          " su ", " sus ", " se ", " lo ", " al ", " un ", " una ",
          " por ", " para ", "usuario", "objetivo", " puede ")
    desc = re.search(r"MOVE_DESC\[MOVE_COUNT\] = \{(.*?)\n\};", mv, re.S)
    if not desc:
        mal("golpes", "no encuentro MOVE_DESC")
    else:
        enEs = [ln.strip() for ln in desc.group(1).splitlines()
                if any(p in ln.lower() for p in ES)]
        if enEs:
            mal("golpes", f"{len(enEs)} descripciones embarcadas en ESPAÑOL "
                          f"(el .dat del Anil no las traduce; van a mano en "
                          f"DESC_MANO de gen_moves.py): {enEs[0]}")
        print(f"  descripciones: {len(desc.group(1).splitlines()) - 1} "
              f"revisadas, 0 en español")

    # Y lo mismo con las HABILIDADES, por el mismo camino y el mismo
    # fallback: Impulso (Speed Boost) salia en la ficha en español.
    abh = lee("abilities.h")
    dab = re.search(r"ABIL_DESC\[[^\]]*\] = \{(.*?)\n\};", abh, re.S)
    if not dab:
        mal("habilidades", "no encuentro ABIL_DESC")
    else:
        enEs = [ln.strip() for ln in dab.group(1).splitlines()
                if any(p in ln.lower() for p in ES)]
        if enEs:
            mal("habilidades",
                f"{len(enEs)} descripciones embarcadas en ESPAÑOL (van a "
                f"mano en DESC_MANO de gen_abilities.py): {enEs[0]}")
        print(f"  habilidades: {len(dab.group(1).splitlines()) - 1} "
              f"descripciones revisadas, 0 en español")

    # Que la PANTALLA sepa contar todo lo que el motor marca. El motor
    # pone `r.sideFx` justo para que la pantalla diga que paso; si no
    # tiene ese caso, el efecto se aplica EN SILENCIO y el jugador ve
    # una trampa puesta sin que nadie se lo diga, o un golpe que
    # parece no haber hecho nada. Eran 19 de 34 asi, y la lista es un
    # switch: un switch se olvida. Esto es la red.
    bcpp = lee("battle.cpp")
    ponen = set(re.findall(r"r\.sideFx = (FX_\w+)", bcpp))
    for m in re.finditer(
            r"case (FX_\w+):(.*?)(?=\n      case |\n    \}|\n  \})",
            bcpp, re.S):
        if "r.sideFx" in m.group(2):
            ponen.add(m.group(1))
    uicpp = lee("battle_ui.ino")
    tro = uicpp[uicpp.index("bt.last.sideFx"):]
    tro = tro[:tro.index("default:")]
    cuenta = set(re.findall(r"case (FX_\w+):", tro))
    mudos = sorted(ponen - cuenta)
    if mudos:
        mal("pantalla", f"{len(mudos)} efectos se aplican sin que la "
                        f"pantalla los cuente: {', '.join(mudos[:6])}")
    print(f"  avisos: {len(ponen)} efectos que el motor marca, "
          f"{len(ponen & cuenta)} contados por la pantalla")

    # el EFECTO de cada golpe: que el que tiene hoja propia la tenga de
    # verdad en /fx/fx.bin, y que el tope del bitmap no deje golpes fuera
    art = lee("art.h")
    tope = constante(art, "FX_MOVE_COUNT")
    if tope is None:
        mal("golpes", "no encuentro FX_MOVE_COUNT")
    elif tope != n:
        mal("golpes", f"FX_MOVE_COUNT {tope} y MOVE_COUNT {n}: los golpes "
                      f"de {tope} en adelante no pueden tener efecto propio")

    # Lo MISMO con las MT, y aqui es peor: TM_MOVE guarda indices de golpe,
    # asi que una tms.h vieja no deja a nadie sin animacion — ensena otro
    # golpe. Paso de verdad (286 MT contra la tabla vieja de 667) porque
    # gen_tms.py no estaba en gen_todo.py. Esta es la red.
    tms = lee("tms.h")
    topeTm = constante(tms, "TM_MOVE_COUNT")
    if topeTm is None:
        mal("MT", "no encuentro TM_MOVE_COUNT: tms.h es de antes de la firma")
    elif topeTm != n:
        mal("MT", f"TM_MOVE_COUNT {topeTm} y MOVE_COUNT {n}: tms.h se hizo "
                  f"contra otra tabla y sus indices apuntan a otros golpes "
                  f"(corre tools/gen_tms.py)")
    fx = os.path.join(SD, "fx", "fx.bin")
    if not os.path.exists(fx):
        mal("golpes", "no esta /fx/fx.bin")
        return
    d = io.open(fx, "rb").read()
    if d[:4] != b"TPFX":
        mal("golpes", f"fx.bin empieza por {d[:4]!r}")
        return
    ng = struct.unpack_from("<H", d, 4)[0]
    offs = struct.unpack_from(f"<{ng}I", d, 8)
    bits = re.search(r"FX_HAS_OWN\[\] = \{(.*?)\};", art, re.S)
    tiene = set()
    if bits:
        by = [int(x, 0) for x in re.findall(r"0x[0-9A-Fa-f]+|\d+",
                                            bits.group(1))]
        for i in range(min(tope or 0, len(by) * 8)):
            if by[i >> 3] & (1 << (i & 7)):
                tiene.add(i)
    rotos = 0
    for i in sorted(tiene):
        if i >= ng or offs[i] == 0:
            mal("golpes", f"golpe {i} dice tener efecto y fx.bin no lo trae")
            rotos += 1
        elif offs[i] >= len(d):
            mal("golpes", f"golpe {i}: su efecto apunta fuera del fichero")
            rotos += 1
    print(f"  {len(tiene)} con efecto propio, {rotos} mal apuntados")


# -------------------------------------------------------- sonidos
def sec_sonidos():
    """Que el sonido de cada golpe sea EL SUYO.

    Los gNNN.raw se llaman por la FILA de MOVE_TBL, asi que meter un
    golpe en medio de moves.h los corre todos una casa. Paso: entro ION
    DELUGE en la fila 317, gen_art.py se volvio a correr y
    pack_audio.py no, y desde ahi cada golpe tocaba el sonido del
    siguiente —— 386 de 703. El usuario lo oyo antes que yo: "licktung
    deu um lick no meu pokemon e fez som de um tipo de cura".

    No se vuelve a descodificar el audio (son 1480 ficheros y minutos);
    se compara la FIRMA que deja el empaquetador, fila a fila, con
    moves.h. Si los nombres cuadran, los numeros son de esta tabla.
    """
    print("--- sonidos ---")
    man = os.path.join(SD, "snd", "golpes.txt")
    mv = lee("moves.h")
    n = constante(mv, "MOVE_COUNT")
    cuerpo = mv[mv.index("MOVE_TBL[MOVE_COUNT]"):] if "MOVE_TBL[MOVE_COUNT]" in mv else ""
    nombres = re.findall(r'^\s*\{\s*"([A-Z0-9]+)",\s*T_', cuerpo, re.M)
    if not nombres:
        mal("sonidos", "NO LEO los nombres de MOVE_TBL: esto no mira nada")
        return
    if not os.path.exists(man):
        mal("sonidos", "no esta snd/golpes.txt: el paquete de sonido no "
                       "dice contra que tabla se hizo, asi que no se puede "
                       "saber si cada golpe toca el suyo (corre "
                       "tools/pack_audio.py golpes)")
        return

    filas, tope = {}, None
    for ln in io.open(man, encoding="utf-8", errors="replace"):
        ln = ln.strip()
        if not ln or ln.startswith("#"):
            continue
        if ln.startswith("MOVE_COUNT"):
            tope = int(ln.split()[1])
            continue
        p = ln.split(None, 2)
        if len(p) == 3:
            filas[int(p[0])] = (p[1], p[2])

    if tope != n:
        mal("sonidos", f"golpes.txt se hizo contra {tope} golpes y MOVE_COUNT "
                       f"es {n}: los sonidos de la fila que cambio en "
                       f"adelante son de otro golpe (corre "
                       f"tools/pack_audio.py golpes)")
        return

    # fila a fila, y NOMBRANDO al primero que no cuadra: "386 mal" no
    # dice donde mirar, "LICK toca el de LEER" si.
    corridos = []
    for i, nom in enumerate(nombres):
        f = filas.get(i)
        if not f:
            corridos.append(f"{nom} (fila {i}) no esta en golpes.txt")
        elif f[0] != nom:
            corridos.append(f"fila {i} es {nom} y el sonido empaquetado es "
                            f"el de {f[0]}")
    if corridos:
        mal("sonidos", f"{len(corridos)} golpes tocan el sonido de otro: "
                       + "; ".join(corridos[:4]) +
                       " (corre tools/pack_audio.py golpes)")
        return

    # y que el fichero este donde la firma dice, y NO este donde dice que
    # no: movePlay toca cualquier gNNN.raw que exista, asi que uno de
    # sobra no es basura, es el sonido de otro golpe.
    faltan, sobran = [], []
    for i, nom in enumerate(nombres):
        hay = os.path.exists(os.path.join(SD, "snd", "g%03d.raw" % i))
        tiene = filas[i][1] != "-"
        if tiene and not hay:
            faltan.append(nom)
        elif not tiene and hay:
            sobran.append(nom)
    for f in sorted(os.listdir(os.path.join(SD, "snd"))):
        m = re.fullmatch(r"g(\d{3})\.raw", f)
        if m and int(m.group(1)) >= n:
            sobran.append(f"{f} (fuera de la tabla)")
    if faltan:
        mal("sonidos", f"{len(faltan)} golpes con sonido en la firma y sin "
                       f"fichero: {', '.join(faltan[:6])}")
    if sobran:
        mal("sonidos", f"{len(sobran)} gNNN.raw de sobra; el Anil no les "
                       f"pone sonido, asi que tocan el de otro golpe en vez "
                       f"del pitido: {', '.join(sobran[:6])}")
    con = sum(1 for i in range(n) if filas[i][1] != "-")
    print(f"  {n} golpes en la firma, {con} con sonido propio, "
          f"{n - con} al pitido")

    # QUE SE OIGA. Tener fichero no es sonar: el Mud Bomb lo tenia y en la
    # placa no se oia nada, porque era un retumbo por debajo de 500 Hz y
    # el altavoz no baja tanto. Se mide lo que queda por ENCIMA de 500 Hz,
    # en la ventana de 100 ms mas fuerte, y se compara con la mediana.
    try:
        import numpy as np
    except ImportError:
        aviso("sonidos", "sin numpy no mido si los golpes se oyen")
        return
    oido = {}
    for i, nom in enumerate(nombres):
        ruta = os.path.join(SD, "snd", "g%03d.raw" % i)
        if not os.path.exists(ruta):
            continue
        d = open(ruta, "rb").read()
        k = int.from_bytes(d[4:8], "little")
        x = np.frombuffer(d[12:12 + 2 * k], dtype="<i2").astype(float)
        X = np.fft.rfft(x)
        X[np.fft.rfftfreq(len(x), 1 / 16000) < 500] = 0
        y = np.fft.irfft(X, len(x))
        w = 1600
        e = (np.convolve(y ** 2, np.ones(w) / w, "valid").max()
             if len(y) >= w else (y ** 2).mean())
        oido[nom] = float(e) ** 0.5
    if oido:
        med = float(np.median(list(oido.values())))
        mudos = sorted((v, nom) for nom, v in oido.items() if v < med / 3)
        if mudos:
            mal("sonidos", f"{len(mudos)} golpes casi no se oyen en el "
                           f"altavoz (por encima de 500 Hz, < 1/3 de la "
                           f"mediana): " + ", ".join(nom for _, nom in mudos[:8])
                           + " (todo grave: mira compone_golpe en pack_audio)")
        print(f"  lo oido por encima de 500 Hz: el mas flojo "
              f"{min(oido.values()) / med:.2f} de la mediana")


# -------------------------------------------------- especies y tablas
def sec_especies():
    print("--- especies ---")
    dex = lee("dex.h")
    ab = lee("abilities.h")
    mv = lee("moves.h")

    # aprendizaje: que nadie se quede sin golpes.
    #
    # El patron llevaba `LEARN_OFS[]` y la tabla se declara
    # `LEARN_OFS[DEX_COUNT + 1]`, asi que no casaba y el resultado era
    # "0 sin golpes" — tranquilizador y vacio. Por eso `pide()`: si una
    # comprobacion no encuentra sus datos, eso es un FALLO y no un cero.
    nums = pide("especies", mv, r"LEARN_OFS\[[^\]]*\] = \{(.*?)\};",
                "la tabla de aprendizaje")
    if nums is not None:
        nums = [int(x) for x in re.findall(r"\d+", nums)]
        if len(nums) < N_DEX:
            mal("especies", f"LEARN_OFS trae {len(nums)} offsets y hacen "
                            f"falta {N_DEX + 1}")
        sinGolpes = [d for d in range(1, min(N_DEX, len(nums) - 1) + 1)
                     if nums[d] == nums[d - 1]]
        if sinGolpes:
            mal("especies", f"{len(sinGolpes)} sin NINGUN golpe: "
                            + ", ".join(NOMBRE.get(x, str(x))
                                        for x in sinGolpes[:10]))
        print(f"  aprendizaje: {len(nums)} offsets, "
              f"{len(sinGolpes)} especies sin golpes")

    # habilidades
    cuerpo = pide("especies", ab, r"ABIL_OF\[[^\]]*\]\[3\] = \{(.*?)\n\};",
                  "la tabla de habilidades por especie")
    if cuerpo is not None:
        filas = re.findall(r"\{\s*(\d+),\s*(\d+),\s*(\d+)\s*\}", cuerpo)
        if len(filas) <= N_DEX:
            mal("especies", f"ABIL_OF trae {len(filas)} filas y hacen falta "
                            f"{N_DEX + 1}")
        sinHab = [i for i, f in enumerate(filas)
                  if i and i <= N_DEX and f[0] == "0" and f[1] == "0"]
        if sinHab:
            mal("especies", f"{len(sinHab)} sin habilidad: "
                            + ", ".join(NOMBRE.get(x, str(x))
                                        for x in sinHab[:10]))
        print(f"  habilidades: {len(filas)} filas, {len(sinHab)} sin ninguna")

    # gritos y miniaturas
    snd = os.path.join(SD, "snd")
    sinGrito = [d for d in range(1, N_DEX + 1)
                if not os.path.exists(os.path.join(snd, f"c{d:03d}.raw"))
                or os.path.getsize(os.path.join(snd, f"c{d:03d}.raw")) < 256]
    if sinGrito:
        mal("especies", f"{len(sinGrito)} sin grito (o vacio): "
                        + ", ".join(NOMBRE.get(x, str(x))
                                    for x in sinGrito[:10]))
    print(f"  gritos: {N_DEX - len(sinGrito)}/{N_DEX}")


# -------------------------------------------------------- evoluciones
def sec_evoluciones():
    print("--- evoluciones ---")
    dex = lee("dex.h")
    bloque = dex[dex.index("DEX_TBL[DEX_COUNT + 1]"):]
    filas = re.findall(
        r'^\s*\{ "([^"]*)", (\d+), (\d+),', bloque, re.M)
    if len(filas) <= N_DEX:
        mal("evoluciones", f"solo leo {len(filas)} filas de DEX_TBL y hacen "
                           f"falta {N_DEX + 1}: no se mira nada")
        return
    evo = {i: (int(e), int(l)) for i, (_n, e, l) in enumerate(filas)}

    # que la cadena entera quepa antes del nivel de la despedida
    TOPE = 72
    for d in range(1, N_DEX + 1):
        cur, lvl, pasos = d, 0, 0
        while cur in evo and evo[cur][0] and pasos < 6:
            e, l = evo[cur]
            if l > lvl:
                lvl = l
            cur, pasos = e, pasos + 1
        if lvl > TOPE:
            ojo("evoluciones",
                f"{NOMBRE.get(d, d)}: su linea pide nivel {lvl} y la "
                f"despedida llega al {TOPE}")
    print(f"  {N_DEX} lineas recorridas")


# ----------------------------------------------- megas y formas (datos)
def sec_megas_formas():
    print("--- megas y formas ---")
    for fich, tabla, cte in (("megas.h", "MEGA_TBL", "MEGA_COUNT"),
                             ("forms.h", "FORM_TBL", "FORM_COUNT")):
        if not os.path.exists(os.path.join(RAIZ, fich)):
            continue
        txt = lee(fich)
        n = constante(txt, cte)
        cuerpo = txt[txt.index(tabla + "["):]
        filas = re.findall(r'^\s*\{\s*(\d+),.*?"([^"]*)" \}', cuerpo, re.M)
        if len(filas) != n:
            mal(fich, f"{tabla} tiene {len(filas)} filas y {cte} dice {n}")
        vistos = {}
        for base, nom in filas:
            if not 1 <= int(base) <= N_DEX:
                mal(fich, f"{nom}: base {base} fuera de la dex")
            if len(nom) > 12:
                mal(fich, f"{nom}: {len(nom)} chars, la ficha da para 12")
            # Dos filas con el MISMO rotulo. Los tres Tauros de Paldea
            # se llamaban los tres "P.TAUROS": el aparato los pintaba
            # distintos y los nombraba igual, y la auditoria no tenia
            # nada que decir porque cada uno por separado estaba bien.
            # Lo canto la hoja de contacto. Esto es para que no haga
            # falta un ojo la proxima vez.
            if nom in vistos:
                mal(fich, f"{nom!r}: lo llevan las filas {vistos[nom]} y "
                          f"{len(vistos)}; en pantalla no se distinguen")
            else:
                vistos[nom] = len(vistos)
        print(f"  {fich}: {len(filas)} filas, {len(vistos)} nombres distintos")
    sec_formas_region()


def sec_formas_region():
    """Que toda forma con region sea ALCANZABLE, y que el grupo no pase de 100.

    formForRegion acumula las probabilidades del grupo y devuelve la
    primera que pasa la tirada. Dos consecuencias que hay que vigilar:
    una forma con chance 0 no sale JAMAS, y si el grupo suma mas de 100
    las ultimas tampoco, porque la tirada nunca llega tan alto.
    """
    if not os.path.exists(os.path.join(RAIZ, "forms.h")):
        return
    txt = lee("forms.h")
    cuerpo = pide("formas", txt, r"FORM_TBL\[FORM_COUNT\] = \{(.*?)\n\};",
                  "la tabla de formas")
    if cuerpo is None:
        return
    filas = re.findall(
        r'^\s*\{\s*(\d+),\s*(\d+),\s*(FORM_NO_REGION|\d+)\s*,\s*(\d+),'
        r'.*?"([^"]*)" \}', cuerpo, re.M)
    if len(filas) != constante(txt, "FORM_COUNT"):
        mal("formas", f"leo {len(filas)} filas de FORM_TBL y FORM_COUNT dice "
                      f"{constante(txt, 'FORM_COUNT')}")
        return
    grupos = {}
    for base, _f, reg, ch, nom in filas:
        if reg == "FORM_NO_REGION":
            continue
        if int(ch) == 0:
            mal("formas", f"{nom}: probabilidad 0, no sale nunca")
        grupos.setdefault((base, reg), []).append((int(ch), nom))
    for (base, reg), v in sorted(grupos.items()):
        suma = sum(c for c, _n in v)
        if suma > 100:
            mal("formas", f"dex {base} en la region {reg}: sus formas suman "
                          f"{suma}%; las ultimas no salen nunca "
                          + ", ".join(n for _c, n in v))
    print(f"  formas por region: {len(grupos)} grupos, "
          f"{sum(1 for v in grupos.values() if len(v) > 1)} con varias "
          f"variedades")


# --------------------------------------------------------------------
# La tarjeta prestada al PC (modo USB-MSC).
#
# Mientras el PC la tiene, el firmware no puede abrir un solo fichero:
# dos escritores sobre el mismo FAT lo rompen. La puerta unica es
# sdUsable() (sdmon.h). Esta guarda existe porque la version anterior
# -- "todo el mundo mira sdReady" -- era FALSA: "SDINFO" y "LS" leian la
# tarjeta sin mirar nada, y la funcion que los contiene SI mencionaba
# sdReady, en otra rama. Mirar la funcion entera no vale; hay que mirar
# la RAMA.
# --------------------------------------------------------------------
import re as _re

# Operaciones que tocan el sistema de ficheros. readRAW/writeRAW no
# estan: son justo las que el modo USB usa a proposito.
SD_OPS = ("open", "exists", "remove", "mkdir", "rmdir", "totalBytes",
          "usedBytes", "cardSize", "numSectors", "sectorSize")

# Donde se permite tocar la tarjeta sin preguntar por sdUsable():
#   sdBegin   - la monta; ahi sdReady es el RESULTADO, no una guarda
#   usbdisco  - es el duenno del modo prestado, trabaja por sectores
SD_LIBRES = ("bool sdBegin()",)


def _ramas(lineas):
    """Devuelve, por cada linea, donde empieza su rama: el ultimo inicio
    de funcion o el ultimo 'else if'/'else' que haya por encima. Eso es
    lo que hay que mirar, no la funcion entera."""
    ini, corte = [], 0
    for i, l in enumerate(lineas):
        t = l.strip()
        if l and l[0] not in " \t#/}" and t.endswith("{"):
            corte = i                      # funcion nueva
        elif t.startswith("} else") or t.startswith("else if") \
                or t.startswith("else {"):
            corte = i                      # rama nueva
        ini.append(corte)
    return ini


def sec_tarjeta_prestada():
    sec = "usb"

    # 1) la puerta existe y es lo que dice ser
    h = lee("sdmon.h")
    if "bool sdPrestada;" not in h:
        mal(sec, "sdmon.h sin sdPrestada: el modo USB no puede cerrar la puerta")
    if not _re.search(r"inline bool sdUsable\(\)\s*\{\s*return sdReady "
                      r"&& !sdPrestada;\s*\}", h):
        mal(sec, "sdUsable() no es 'sdReady && !sdPrestada': la guarda miente")

    # 2) ninguna operacion de ficheros se cuela sin pasar por ella
    for f in ("sdmon.cpp", "audio.cpp"):
        lineas = lee(f).split("\n")
        ini = _ramas(lineas)
        for n, l in enumerate(lineas):
            if not _re.search(r"SD_MMC\.(%s)\(" % "|".join(SD_OPS), l):
                continue
            rama = "\n".join(lineas[ini[n]:n])
            if any(x in lineas[ini[n]] for x in SD_LIBRES):
                continue
            if "sdUsable()" not in rama:
                mal(sec, "%s:%d toca la tarjeta sin sdUsable(): %s"
                    % (f, n + 1, l.strip()[:60]))

    # 3) el modulo USB existe y esta enganchado
    u = lee("usbdisco.cpp")
    for q, por in (("ARDUINO_USB_MODE", "el aviso de FQBN equivocado"),
                   ("static USBMSC msc;", "el objeto global que registra la interfaz"),
                   ("sdPrestada = true;", "cerrar la puerta al prestar")):
        if q not in u:
            mal(sec, "usbdisco.cpp sin %s (%s)" % (q, por))
    # el orden importa: cerrar la puerta ANTES de ensenar la unidad
    i_cerrar = u.find("sdPrestada = true;")
    i_montar = u.find("msc.mediaPresent(true);")
    if i_cerrar < 0 or i_montar < 0 or i_cerrar > i_montar:
        mal(sec, "usbdisco.cpp presta la tarjeta antes de cerrarle la puerta al "
                 "firmware: queda una ventana con dos escritores")
    # El fx.bin de la TARJETA, contra la tabla de golpes embarcada. Se
    # indexa por numero de golpe, asi que una tabla con dos golpes mas
    # lo invalida ENTERO: cada animacion pasa a ser la del golpe de al
    # lado. Paso de verdad y se vio jugando ("Latigo Cepa saca un
    # puñetazo"), no aqui, porque aqui nadie lo miraba.
    fx = os.path.join(SD, "fx", "fx.bin")
    if os.path.exists(fx):
        with io.open(fx, "rb") as fh:
            cab = fh.read(8)
        if len(cab) == 8 and cab[:4] == b"TPFX":
            n_fx = cab[4] | (cab[5] << 8)
            n_mv = constante(lee("moves.h"), "MOVE_COUNT")
            if n_mv and n_fx != n_mv:
                mal(sec, "la tarjeta trae fx.bin de %d golpes y la tabla "
                         "tiene %d: hay que volver a copiarlo" % (n_fx, n_mv))
        else:
            mal(sec, "fx.bin de la tarjeta sin cabecera TPFX")

    ino = lee("TamaPoke.ino")
    for q, por in (("usbMscBegin();", "preparar el lector en setup()"),
                   ("usbMscSerialCommand(line)", "las ordenes USB ON/OFF"),
                   ("usbMscExpulsada()", "recuperar la tarjeta al expulsarla")):
        if q not in ino:
            mal(sec, "TamaPoke.ino sin %s (%s)" % (q, por))


def sec_animaciones():
    """Que las animaciones de la tarjeta esten ENTERAS.

    Dos cosas que ya fallaron sin que nada avisara:

    - 72 golpes (Psychic, Surf, Rest, Bulk Up...) iban SIN PARTICULAS: el
      Anil cita la hoja sin extension ("PRAS- Surf") y pack_anim.py no la
      encontraba. El fichero salia igual, con el bicho moviendose y nada
      mas. Aqui: golpe cuya animacion del Anil usa celdas y cuyo fichero
      no trae ni una imagen.
    - El clima no tenia animacion ninguna. Los cuatro w_<n>.bin tienen
      que estar, decir de que clima son, y traer lo suyo: la figura de
      pantalla entera (sol y arena) o las particulas (lluvia y granizo).
    """
    import struct
    print("--- animaciones ---")
    carpeta = os.path.join(SD, "anim")
    if not os.path.isdir(carpeta):
        mal("animaciones", "no esta la carpeta anim/ de la tarjeta")
        return
    for n, nom, figura in ((1, "sol", True), (2, "lluvia", False),
                           (3, "arena", True), (4, "granizo", False)):
        p = os.path.join(carpeta, "w_%d.bin" % n)
        if not os.path.exists(p):
            mal("animaciones", "falta anim/w_%d.bin (el clima: %s): "
                "python tools/pack_clima.py" % (n, nom))
            continue
        b = open(p, "rb").read()
        if b[:4] != b"TPCL" or b[4] != 1 or b[5] != n:
            mal("animaciones", "anim/w_%d.bin no es el TPCL de ese clima" % n)
            continue
        nq, ni, tem = struct.unpack_from("<HBB", b, 6)
        if figura and not tem:
            mal("animaciones", "el clima %s va sin su figura de pantalla entera" % nom)
        if not figura and not ni:
            mal("animaciones", "el clima %s va sin particulas" % nom)
        if not nq:
            mal("animaciones", "el clima %s no tiene fotogramas" % nom)

    # El NOMBRE de la cabecera tiene que ser el del fichero: el firmware
    # (SdAnim::load) rechaza en silencio el que no coincide y el golpe se
    # queda sin animacion. Leva 63: una variable `k` de pack_anim pisada
    # dentro de un bucle grabo PLAYROUGH como "PLAYROUGH.21".
    malnom = []
    for f in sorted(os.listdir(carpeta)):
        if not (f.startswith(("p_", "f_")) and f.endswith(".bin")):
            continue
        b = open(os.path.join(carpeta, f), "rb").read(64)
        if len(b) < 8 or b[:4] != b"TPAN" or b[5:6] != f[:1].encode():
            malnom.append(f + " (cabecera)")
            continue
        if b[7:7 + b[6]] != f[2:-4].encode():
            malnom.append("%s dice %r" % (f, b[7:7 + b[6]].decode("ascii", "replace")))
    for x in malnom[:10]:
        mal("animaciones", "anim/%s: el firmware no la acepta" % x)
    if len(malnom) > 10:
        mal("animaciones", "... y %d mas con la cabecera mal" % (len(malnom) - 10))

    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    try:
        import anil_anim as AA
        todas = AA.load_all()
    except Exception as e:      # sin el Anil en el disco no se puede comparar
        ojo("animaciones", "no se pudo leer PkmnAnimations del Anil (%s): "
            "no se miran las particulas de los golpes" % e)
        return
    mirados = vacios = 0
    for nombre in AA.moves_h_names():
        anim = todas.get("Move:" + nombre)
        p = os.path.join(carpeta, "p_%s.bin" % nombre)
        if anim is None or not os.path.exists(p):
            continue
        usa = False
        for fr in anim:
            for cel in fr:
                if cel is None:
                    continue
                pat = AA.g(cel, AA.PATTERN)
                if pat is not None and pat >= 0 and AA.g(cel, AA.VISIBLE, 1) == 1:
                    usa = True
                    break
            if usa:
                break
        if not usa:
            continue
        b = open(p, "rb").read(64)
        if b[:4] != b"TPAN":
            mal("animaciones", "anim/p_%s.bin no es un TPAN" % nombre)
            continue
        ni = b[7 + b[6] + 2]
        mirados += 1
        if ni == 0:
            vacios += 1
            mal("animaciones", "%s: el Anil la anima con particulas y el "
                "fichero no trae ninguna (python tools/pack_anim.py %s)"
                % (nombre, nombre))
    # LA NUBE DE PELEA (Play Rough): el que ataca no se dibuja mientras
    # esta dentro de la nube, y si antes de entrar y al salir. Reportado:
    # "deveria ficar so a fumaca brigando, esta ficando o sprite tambem".
    import pack_anim as PA
    import ve_anim as VA
    for nombre in sorted(PA.NUVEM_DE_BRIGA):
        for lado, quien in (("p", -1), ("f", -2)):
            p = os.path.join(carpeta, "%s_%s.bin" % (lado, nombre))
            if not os.path.exists(p):
                mal("animaciones", "falta anim/%s_%s.bin" % (lado, nombre))
                continue
            _n, _l, _i, quadros = VA.le(p)
            ops = [next((c[7] for c in q[2] if c[0] == quien), 0) for q in quadros]
            ocultos = sum(1 for o in ops if not o)
            if not ops[0] or not ops[-1]:
                mal("animaciones", "%s_%s: el que ataca no esta al empezar o "
                    "al acabar" % (lado, nombre))
            if ocultos < len(ops) // 2:
                mal("animaciones", "%s_%s: el que ataca se queda a la vista "
                    "dentro de la nube (%d de %d fotogramas oculto)"
                    % (lado, nombre, ocultos, len(ops)))
    if not mirados:
        mal("animaciones", "NO SE MIRO ningun golpe: esto no comprueba nada")
    else:
        print("  %d golpes con particulas en el Anil, %d sin ellas en la tarjeta"
              % (mirados, vacios))


def sec_lideres():
    """Hoenn: a linha n do gyms.h e o LIDERnHOENN do Anil (o retrato n e dele),
    e o time sai do time dele. Escapou uma vez: ROXANNE com a cara do Norman."""
    sec = "lideres"
    pbs = os.path.join(r"C:\Users\foulu\Downloads\Pokemon Anil V4.13", "PBS", "trainers.txt")
    if not os.path.exists(pbs):
        ojo(sec, "sem o PBS do Anil V4.13: nao deu para conferir")
        return
    t = io.open(pbs, encoding="utf-8").read()
    NOME = {"Norman": "NORMAN", "Candela": "FLANNERY", "Alana": "WINONA",
            "Erico": "WATTSON", "Petra": "ROXANNE", "Marcial": "BRAWLY",
            "Plubio": "JUAN", "Vito y Leti": "TATE&LIZA"}
    POR_NOME = {d[2]: d[0] for d in DEX}
    g = lee("gyms.h")
    bloco = g[g.index("HOENN_GYMS[GYM_TOTAL] = {"):]
    linhas = re.findall(r'\{ "([^"]+)",\s*"[^"]+",\s*\w+,\s*\{\s*(\d+),\s*(\d+),\s*(\d+)\s*\}', bloco)[:8]
    if len(linhas) != 8:
        mal(sec, "nao achei as 8 linhas de Hoenn no gyms.h")
        return
    for n in range(1, 9):
        m = re.search(r"^\[LIDER%dHOENN,([^\]]+)\]\n(.*?)(?=^#-|^\[)" % n, t, re.M | re.S)
        if not m:
            mal(sec, "LIDER%dHOENN sumiu do PBS" % n)
            continue
        quem = NOME.get(m.group(1).strip(), "?" + m.group(1))
        nome, a, b, c = linhas[n - 1]
        if nome != quem:
            mal(sec, "Hoenn %d: o gyms.h diz %s, o retrato e de %s" % (n, nome, quem))
        time = [(POR_NOME.get(x), int(nv)) for x, nv in
                re.findall(r"^Pokemon = (\w+),(\d+)", m.group(2), re.M)]
        cabe = [(d, nv) for d, nv in time if d and d <= 386]
        if not cabe:
            mal(sec, "%s: nenhum bicho ate a 386" % quem)
            continue
        alto = max(nv for _, nv in cabe)
        az = [d for d, nv in cabe if nv == alto][-1]
        if int(c) != az:
            mal(sec, "%s: o AS devia ser #%d (o mais alto do Anil), esta #%s" % (quem, az, c))
        for x in (a, b):
            if int(x) and int(x) not in [d for d, _ in cabe]:
                mal(sec, "%s: #%s nao esta no time dele no Anil" % (quem, x))


def main():
    hondo = "--hondo" in sys.argv
    solo = None
    if "--seccion" in sys.argv:
        solo = sys.argv[sys.argv.index("--seccion") + 1]

    secs = {
        "especies": sec_especies,
        "golpes": sec_golpes,
        "sonidos": sec_sonidos,
        "animaciones": sec_animaciones,
        "evoluciones": sec_evoluciones,
        "megas": sec_megas_formas,
        "sprites": lambda: sec_sprites(hondo),
        "usb": sec_tarjeta_prestada,
        "lideres": sec_lideres,
    }
    for nom, fn in secs.items():
        if solo and nom != solo:
            continue
        fn()

    print()
    if avisos:
        print(f"--- {len(avisos)} AVISOS ---")
        for s, m in avisos[:25]:
            print(f"  ? [{s}] {m}")
        if len(avisos) > 25:
            print(f"  ... y {len(avisos) - 25} mas")
    if fallos:
        print(f"--- {len(fallos)} FALLOS ---")
        for s, m in fallos[:40]:
            print(f"  ! [{s}] {m}")
        if len(fallos) > 40:
            print(f"  ... y {len(fallos) - 40} mas")
        return 1
    print("sin fallos")
    return 0


if __name__ == "__main__":
    sys.exit(main())
