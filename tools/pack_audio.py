#!/usr/bin/env python3
"""Empaquetador de sonido: gritos, musica de combate y fanfarrias.

El Anil trae 111 BGM, 38 fanfarrias y 1167 gritos, todo en .ogg. La placa no
va a descodificar Vorbis, asi que se convierte AQUI a PCM crudo con el mismo
ritmo de muestreo que usa audio.cpp (16 kHz), mono y 16 bits:

  char[4] "TPKS"
  u32 muestras
  u16 hz
  u16 flags       (1 = pensado para repetirse)
  int16 pcm[muestras]      mono, little endian

Mono y 16 kHz no es por gusto: el altavoz es uno, el I2S ya esta abierto a
16 kHz para los pitidos y a ese ritmo un grito ocupa 34 KB en vez de 190.

Sale en tools/sdcard/snd/:
  cNNN.raw   grito de la especie NNN (1..151)
  mNN.raw    musica (se repite)
  jNN.raw    fanfarria (suena una vez)

  python3 tools/pack_audio.py            # todo
  python3 tools/pack_audio.py cries      # solo los gritos
  python3 tools/pack_audio.py music
"""
import io
import os
import re
import struct
import sys

import numpy as np
import soundfile as sf

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from dex_range import N_DEX

ANIL = r"C:\Users\foulu\Downloads\Pokemon Anil V4.13"
AUDIO = os.path.join(ANIL, "Audio")
CRIES = os.path.join(AUDIO, "SE", "Cries")
DEX = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "dex.h")
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "sdcard", "snd")

HZ = 16000
# El pico al que se normaliza cada clip. 18000 de 32767 deja aire para que
# musica y efecto suenen a la vez sin recortar (el mezclador de audio.cpp
# suma y satura), y queda por encima de los 5000 de los pitidos cuadrados.
PICO = 18000
PICO_MUS = 11000        # la musica va por debajo: es fondo, no protagonista

# Musica. Se corta a MUS_SEG segundos y se cruza el final con el principio,
# porque estas pistas tienen intro y bucle y cortar en seco chasquea.
MUS_SEG = 30
MUS_CRUCE = 0.35        # segundos de cruce

MUSICA = [
    (0, "BGM/Salvaje.ogg"),          # combate salvaje
    (1, "BGM/Entrenador.ogg"),       # entrenador
    (2, "BGM/Gimnasio.ogg"),         # lider de gimnasio
    (3, "BGM/Elite Four.ogg"),       # Alto Mando
    (4, "BGM/BatallaCampeon.ogg"),   # campeon
    (5, "BGM/Titulo.ogg"),           # portada
    # 6..11: la pantalla de casa. VARIAS, como los paisajes: es la que esta
    # siempre encendida y con una sola cansa en una tarde. Son temas de
    # pueblo y de ruta a proposito — tranquilos, de fondo, no de combate.
    (6, "BGM/PuebloPaleta.ogg"),
    (7, "BGM/Ruta 1.ogg"),
    (8, "BGM/Ruta 3.ogg"),
    (9, "BGM/PuebloMarengo.ogg"),
    (10, "BGM/BosqueVerde.ogg"),
    (11, "BGM/PuebloLavanda.ogg"),   # la de noche
    # 12: la de EVOLUCION, la que suena en bucle mientras el bicho cambia
    # (pbEvolution: pbBGMPlay("Evolution")). Antes la escena era muda y
    # solo sonaba la fanfarria de exito.
    (12, "BGM/Evolution.ogg"),
]

# Fanfarrias: suenan enteras y una sola vez, asi que no se cortan.
# Los EFECTOS, en el orden del enum de audio.h (SFX_TAP..SFX_LEVEL).
#
# Hasta ahora eran pitidos de onda cuadrada sintetizados al vuelo.
# Funcionaban -- y siguen ahi, como red para cuando no hay tarjeta --
# pero no sonaban a Pokemon, que era la queja. Estos salen del Anil.
#
# Cuatro son evidentes (el cursor, el zumbido de "no", subir de nivel,
# cerrar menu). Los otros seis son ELECCION MIA y estan marcados: el
# Anil no tiene un sonido de "comer" ni de "mimo", asi que se busco el
# que mas se le parece. Cambiar cualquiera es cambiar una linea.
EFECTOS = [
    (0, "SE/GUI sel cursor.ogg"),      # SFX_TAP     tocar / boton
    # El bicho MASTICANDO, no el "has usado un objeto": Bug Bite es el
    # golpe que en los juegos hace que el rival se coma su baya, asi que
    # su sonido ES el crujido de baya. Antes estuvo "Use item in party",
    # que es el de curar, y el dueno lo dijo: suena a recuperar PS, no a
    # comer.
    # Crunch: es el mas largo de los tres mordiscos que trae el Anil
    # (0,89 s contra 0,41). NO hay un sonido de "comer baya" en su
    # biblioteca -- se busco en todo Audio/ -- asi que esto es lo mas
    # cerca que hay. Si tampoco vale, los otros dos son
    # "PRSFX- Bite.wav" y "PRSFX- Bug Bite.wav".
    (1, "SE/Anim/PRSFX- Crunch.wav"),  # SFX_EAT     comer
    (2, "SE/Mining ping.ogg"),         # SFX_PLAY    punto      (elegido)
    # El brillito de shiny sonaba raro para un mimo (es de "has
    # encontrado algo raro", no de carino). Este es corto y calido.
    # Sweet Scent parecia el tematico pero dura 3 segundos: para algo
    # que se repite acariciando, es una eternidad.
    (3, "SE/GUI naming confirm.ogg"),  # SFX_HEART   mimo
    (4, "SE/Battle catch click.ogg"),  # SFX_HATCH   eclosion   (elegido)
    (5, "SE/Vs flash.ogg"),            # SFX_EVOLVE  evolucion  (elegido)
    (6, "SE/Pkmn exp full.ogg"),       # SFX_MEDAL   hito       (elegido)
    (7, "SE/GUI sel buzzer.ogg"),      # SFX_DENY    no se puede
    (8, "SE/GUI menu close.ogg"),      # SFX_BYE     despedida
    (9, "SE/Pkmn level up.ogg"),       # SFX_LEVEL   sube nivel
    # Cut.ogg es el sonido de cortar hierba: es LA hierba moviendose.
    # Exclaim.ogg es el "!" de cuando alguien te ve.
    # VIENTO y no Cut.ogg: Cut es el golpe de cortar, y sonaba a
    # golpe. Wind1 es el unico sonido de aire corto que trae el Anil
    # (1,04 s) y es lo mas parecido a hierba moviendose que hay.
    (10, "SE/Anim/Wind1.ogg"),         # SFX_WILD    sale un salvaje
    (11, "SE/Exclaim.ogg"),            # SFX_TRAINER te ve un entrenador
    (12, "SE/Battle damage weak.ogg"),   # SFX_HIT_WEAK
    (13, "SE/Battle damage normal.ogg"), # SFX_HIT
    (14, "SE/Battle damage super.ogg"),  # SFX_HIT_SUPER
    # La pokebola: los que toca el Anil en PokeballThrowCapture.
    (15, "SE/Battle throw.ogg"),         # SFX_BALL_THROW
    (16, "SE/Battle ball hit.ogg"),      # SFX_BALL_HIT
    (17, "SE/Battle ball drop.ogg"),     # SFX_BALL_DROP
    (18, "SE/Battle ball shake.ogg"),    # SFX_BALL_SHAKE
    (19, "SE/Battle catch click.ogg"),   # SFX_BALL_CLICK
    (20, "SE/Battle recall.ogg"),        # SFX_BALL_OPEN
]

JINGLES = [
    (0, "BGM/VictoriaSalvaje.ogg"),
    (1, "BGM/VictoriaEntrenador.ogg"),
    (2, "BGM/Victoria Importante.ogg"),
    (3, "ME/Battle capture success.ogg"),
    (4, "ME/Evolution success.ogg"),
    (5, "ME/Badge get.ogg"),
    (6, "ME/Egg get.ogg"),
    # El centro Pokemon. Es la fanfarria que todo el mundo reconoce sin
    # mirar la pantalla, que es justo lo que tiene que hacer un aviso que
    # aparece solo cada tantos minutos.
    (7, "ME/Pkmn healing.ogg"),
    # La MEGA. El "1" del Anil NO vale: es byte a byte el mismo fichero
    # que "Pulse Evolution1", un zumbido plano de 1,92 s (RMS 0,06 de
    # punta a punta, sin ataque). El "2" es el estallido de verdad: 0,61
    # de RMS en el primer decimo y 1,37 s de cola.
    (8, "SE/PRSFX- Mega Evolution2.wav"),
]
JIN_MAX_SEG = 8         # una fanfarria mas larga que esto es una cancion

# Los dos unicos nombres que no casan solos: en el Anil llevan el sexo
# pegado al nombre y en nuestra dex es " H" / " M".
A_MANO = {"NIDORANH": "NIDORANfE", "NIDORANM": "NIDORANmA"}


def normaliza(s):
    return re.sub(r"[^A-Z0-9]", "", s.upper())


def dex_nombres():
    """{numero: nombre} leyendo las filas de DEX_TBL de dex.h."""
    txt = io.open(DEX, encoding="utf-8", errors="replace").read()
    filas = re.findall(r'\{\s*"([^"]+)",[^\n]*//\s*(\d+)\s', txt)
    return {int(n): nombre for nombre, n in filas if 1 <= int(n) <= N_DEX}


def a_pcm(ruta, pico, seg=None, cruce=0.0):
    """Lee un .ogg/.wav y devuelve int16 mono a HZ, normalizado a `pico`."""
    d, sr = sf.read(ruta, dtype="float32", always_2d=True)
    m = d.mean(axis=1)                       # a mono: el altavoz es uno
    if seg:
        m = m[: int(sr * seg)]
    # remuestreo lineal. Es suficiente: bajamos de 44/48 kHz a 16 y lo que
    # se pierde arriba no lo da ni el altavoz de la placa.
    n = int(round(len(m) * HZ / sr))
    if n <= 0:
        return None
    m = np.interp(np.linspace(0, len(m) - 1, n), np.arange(len(m)), m)
    if cruce > 0 and n > int(HZ * cruce) * 2:
        # cruza el final con el principio para que el bucle no chasquee
        c = int(HZ * cruce)
        rampa = np.linspace(0.0, 1.0, c)
        m[:c] = m[:c] * rampa + m[-c:] * (1.0 - rampa)
        m = m[:-c]
    top = float(np.max(np.abs(m))) or 1.0
    return np.clip(m / top * pico, -32768, 32767).astype("<i2")


# EL SONIDO DE UN GOLPE, MONTADO COMO LO TOCA EL ANIL. La animacion trae
# una lista de SE, cada uno en su fotograma, con su volumen y su tono
# (playTiming). Antes se cogia uno solo y se perdia el resto: el Mud Bomb
# sonaba solo al disparo, que es un retumbo por debajo de 500 Hz, y el
# altavoz de la placa —— uno pequeno, que por debajo de ~300 Hz no mueve
# aire —— lo dejaba en silencio. El chapoteo, que si se oye, iba despues.
FPS_ANIM = 20           # el mismo ritmo al que battle_ui pinta los fotogramas
GOLPE_MIN = 0.9         # lo que duraba antes cualquier golpe
GOLPE_TOPE = 1.6        # y mas de esto ya pisa el mensaje del turno
# Lo que el altavoz no da. Fuera ese grave, el pico ya no lo marca un
# retumbo que no se oye, y al normalizar sube lo que si se oye.
CORTE_GRAVE = 250
# Volumen OIDO (rms de la ventana de 100 ms mas fuerte, ya sin grave) por
# debajo del cual se sube el golpe, con un limitador suave para no
# recortar. Es el percentil 10 de los golpes que ya sonaban bien.
OIDO_MIN = 4000
OIDO_SUBE_MAX = 2.5


def _lee_mono(ruta, tono=100):
    """El fichero a mono y a HZ, con el tono de RPG Maker: 105 = 5% mas
    rapido y mas agudo, que es lo que hace cambiar el ritmo."""
    d, sr = sf.read(ruta, dtype="float32", always_2d=True)
    m = d.mean(axis=1)
    n = int(round(len(m) * HZ / sr * 100.0 / max(tono, 1)))
    if n <= 0:
        return None
    return np.interp(np.linspace(0, len(m) - 1, n), np.arange(len(m)), m)


def _oido(y):
    w = HZ // 10
    if len(y) < w:
        return float(np.sqrt(np.mean(y ** 2)))
    return float(np.sqrt(np.convolve(y ** 2, np.ones(w) / w, "valid").max()))


def compone_golpe(carpeta, sons, nq):
    from scipy.signal import butter, sosfiltfilt
    # Se cuenta desde el PRIMER SE, no desde el fotograma 0: la placa
    # arranca el sonido al empezar el golpe, y Torch Song no suena hasta
    # el fotograma 41 —— contado desde 0, los 1,6 s de tope eran silencio.
    fr0 = min(s[0] for s in sons)
    partes = []
    for fr, fich, vol, tono in sons:
        x = _lee_mono(os.path.join(carpeta, fich), tono)
        if x is not None:
            partes.append((int((fr - fr0) * HZ / FPS_ANIM), x * (vol / 100.0)))
    if not partes:
        return None
    tope = min(GOLPE_TOPE, max(GOLPE_MIN, (nq - fr0) / FPS_ANIM + 0.3))
    n = min(int(tope * HZ), max(o + len(x) for o, x in partes))
    mix = np.zeros(n)
    for o, x in partes:
        if o < n:
            trozo = x[: n - o]
            mix[o:o + len(trozo)] += trozo
    if max(o + len(x) for o, x in partes) > n:     # cortado: que no chasquee
        c = min(n, HZ * 8 // 100)
        mix[-c:] *= np.linspace(1.0, 0.0, c)
    if n > 30:
        mix = sosfiltfilt(butter(4, CORTE_GRAVE, "highpass", fs=HZ,
                                 output="sos"), mix)
    top = float(np.max(np.abs(mix))) or 1.0
    y = mix / top * PICO
    o = _oido(y)
    if 0 < o < OIDO_MIN:
        g = min(OIDO_MIN / o, OIDO_SUBE_MAX)
        y = PICO * np.tanh(y * g / PICO)
    return np.clip(np.round(y), -32768, 32767).astype("<i2")


def guarda(nombre, pcm, loop):
    os.makedirs(OUT, exist_ok=True)
    ruta = os.path.join(OUT, nombre)
    with open(ruta, "wb") as f:
        f.write(b"TPKS")
        f.write(struct.pack("<IHH", len(pcm), HZ, 1 if loop else 0))
        f.write(pcm.tobytes())
    return 8 + pcm.nbytes


def haz_cries():
    disp = {f[:-4].upper(): f for f in os.listdir(CRIES) if f.lower().endswith(".ogg")}
    total = n = 0
    for num, nombre in sorted(dex_nombres().items()):
        k = normaliza(nombre)
        fich = disp.get(A_MANO.get(k, k).upper()) or disp.get(k)
        if not fich:
            print("  !! sin grito: %3d %s" % (num, nombre))
            continue
        pcm = a_pcm(os.path.join(CRIES, fich), PICO)
        if pcm is None:
            continue
        total += guarda("c%03d.raw" % num, pcm, False)
        n += 1
    print("  %d gritos, %d KB" % (n, total // 1024))
    return total


def haz_lista(lista, pref, pico, seg, cruce, loop):
    total = 0
    for idn, rel in lista:
        ruta = os.path.join(AUDIO, rel.replace("/", os.sep))
        if not os.path.exists(ruta):
            print("  !! falta", rel)
            continue
        pcm = a_pcm(ruta, pico, seg, cruce)
        if pcm is None:
            continue
        b = guarda("%s%02d.raw" % (pref, idn), pcm, loop)
        total += b
        print("  %s%02d %-34s %4d KB  %.1fs" %
              (pref, idn, os.path.basename(rel), b // 1024, len(pcm) / HZ))
    return total


# Los golpes suenan con el SE que les pone el ANIL, no con un pitido
# sintetizado. El nombre del fichero sale de los `timing` de la animacion
# (Data/PkmnAnimations.rxdata), igual que los fotogramas: ahi esta escrito
# que SE toca cada golpe y en que fotograma.
#
# Se escribe un fichero POR GOLPE (g000.raw..) y no uno por sonido con una
# tabla de indices: 611 ficheros distintos se reparten entre 703 golpes, y
# duplicar los repetidos cuesta unos pocos MB en una tarjeta de 58 GB. A
# cambio el firmware solo tiene que abrir "g%03u.raw" y no cargar ninguna
# tabla. El prefijo es `g` y no `m` porque la musica ya usa `m`.
def haz_golpes():
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    import fxanim

    ANIM_SE = os.path.join(AUDIO, "SE", "Anim")
    if not os.path.isdir(ANIM_SE):
        print("  !! no esta", ANIM_SE)
        return 0
    disp = {f.lower(): f for f in os.listdir(ANIM_SE)}

    txt = io.open(os.path.join(os.path.dirname(os.path.dirname(
        os.path.abspath(__file__))), "moves.h"),
        encoding="utf-8", errors="replace").read()
    nombres = re.findall(r'^\s*\{\s*"([A-Z0-9]+)",\s*T_', txt, flags=re.M)
    anims = fxanim.carga(os.path.join(ANIL, "Data", "PkmnAnimations.rxdata"))

    cache = {}          # fichero -> pcm ya convertido (612 fuentes, 704 usos)
    fuente = [None] * len(nombres)    # fila de MOVE_TBL -> fichero de origen
    total = n_ok = n_sin = 0
    from pack_anim import EMPRESTADA
    for i, nom in enumerate(nombres):
        # El golpe que toma prestada la animacion de otro (Triple Hit la
        # del Triple Kick) toma tambien su sonido: se ve una patada, que
        # suene a patada.
        a = anims.get(EMPRESTADA.get(nom, nom))
        if not a:
            n_sin += 1
            continue
        sons = []
        for fr, se, vol, tono in fxanim.sonidos_de(a):
            base = os.path.splitext(se)[0].lower()
            fich = (disp.get(se.lower()) or disp.get(base + ".ogg")
                    or disp.get(base + ".wav") or disp.get(base + ".mp3"))
            if fich and vol > 0:
                sons.append((fr, fich, vol, tono))
        if not sons:
            n_sin += 1
            continue
        nq = len(a.iv.get("array") or [])
        clave = (tuple(sons), nq)
        if clave not in cache:
            cache[clave] = compone_golpe(ANIM_SE, sons, nq)
        pcm = cache[clave]
        if pcm is None:
            n_sin += 1
            continue
        fuente[i] = " + ".join(s[1] for s in sons)
        total += guarda("g%03d.raw" % i, pcm, False)
        n_ok += 1

    # LO QUE SOBRA. `guarda` escribe pero no borra, y un gNNN.raw de una
    # tabla anterior no deja al golpe en silencio: le da OTRO sonido,
    # porque el firmware toca cualquier fichero que encuentre con ese
    # numero (movePlay solo mira si existe). Asi se quedaron ROAR,
    # MIRRORMOVE, IVYCUDGEL, SECRETPOWER y TRIPLEHIT —— que el Anil deja
    # sin sonido —— tocando el del golpe siguiente en vez del pitido.
    sobran = []
    for f in sorted(os.listdir(OUT)):
        m = re.fullmatch(r"g(\d{3})\.raw", f)
        if not m:
            continue
        i = int(m.group(1))
        if i >= len(nombres) or not fuente[i]:
            os.remove(os.path.join(OUT, f))
            sobran.append(f)

    # LA FIRMA del paquete: la tabla contra la que se empaqueto, fila a
    # fila. Sin esto, 703 ficheros numerados no dicen de que epoca son, y
    # meter un golpe en medio de moves.h los corre todos una casa sin que
    # nada se queje. Paso de verdad: entro ION DELUGE en la fila 317 y
    # desde ahi cada golpe tocaba el sonido del siguiente —— el Lick
    # sonaba a cura. tools/auditoria.py compara estos nombres con moves.h.
    with io.open(os.path.join(OUT, "golpes.txt"), "w", encoding="utf-8",
                 newline="\n") as f:
        f.write("# el sonido de cada golpe, una fila por fila de MOVE_TBL.\n")
        f.write("# lo escribe tools/pack_audio.py; lo comprueba\n")
        f.write("# tools/auditoria.py (seccion sonidos) contra moves.h.\n")
        f.write("# `-` = el Anil no le pone sonido: NO tiene que haber\n")
        f.write("# fichero, o tocaria el de otro golpe.\n")
        f.write("MOVE_COUNT %d\n" % len(nombres))
        for i, nom in enumerate(nombres):
            f.write("%d %s %s\n" % (i, nom, fuente[i] or "-"))

    print("  %d golpes con su sonido (%d fuentes distintas), %d sin el, %d KB"
          % (n_ok, len(cache), n_sin, total // 1024))
    if sobran:
        print("  %d de sobra, borrados (tocaban el sonido de otro golpe): %s"
              % (len(sobran), ", ".join(sobran)))
    print("  firma en golpes.txt, %d filas" % len(nombres))
    return total


def main():
    que = [a.lower() for a in sys.argv[1:]] or ["cries", "music", "jingles",
                                                "sfx", "golpes"]
    total = 0
    if "cries" in que:
        print("gritos:")
        total += haz_cries()
    if "music" in que:
        print("musica:")
        total += haz_lista(MUSICA, "m", PICO_MUS, MUS_SEG, MUS_CRUCE, True)
    if "jingles" in que:
        print("fanfarrias:")
        total += haz_lista(JINGLES, "j", PICO, JIN_MAX_SEG, 0.0, False)
    if "sfx" in que:
        print("efectos:")
        # Medio segundo de tope: un efecto es un golpe seco. Si se deja
        # entero, el de subir de nivel dura dos segundos y pisa al
        # siguiente toque.
        total += haz_lista(EFECTOS, "s", PICO, 0.9, 0.0, False)
    if "golpes" in que:
        print("golpes:")
        total += haz_golpes()
    print("%d KB en %s" % (total // 1024, OUT))


if __name__ == "__main__":
    main()
