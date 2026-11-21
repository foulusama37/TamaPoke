#!/usr/bin/env python3
"""Las animaciones de golpe DEL ANIL, montadas como fotogramas.

Por que existe
--------------
Hasta ahora el generador cogia una hoja de Graphics/Animations y se quedaba
con "las primeras N celdas no vacias". Eso no es la animacion: una hoja como
`PRAS- Slash.png` son TRECE animaciones distintas pegadas en una rejilla, y
otras traen al final el cartel de "Ripped and Composed By..." del que la
extrajo. Scratch salia con tres fotogramas de ese cartel: letras sueltas en
mitad del combate.

Data/PkmnAnimations.rxdata dice exactamente que celda va en cada fotograma,
donde, con que zoom, giro, espejo y opacidad. Leyendolo no hace falta
adivinar NADA: ni el tamano de celda, ni cuales valen, ni cuantas coger.

El formato
----------
Cada animacion es `PBAnimation`, una subclase de Array de Ruby que deja el
array base VACIO y guarda el contenido en la ivar `@array` (ojo con esto:
leyendo solo el array base, el fichero se parsea entero sin dar error y
devuelve cero animaciones). Dentro hay una lista de fotogramas, y cada
fotograma una lista de celdas.

Los indices de una celda salen de `class AnimFrame` del script
BattleAnimationPlayer del propio Anil, no de suponerlos:

    X=0  Y=1  ZOOMX=2  ANGLE=3  MIRROR=4  BLENDTYPE=5
    VISIBLE=6  PATTERN=7  OPACITY=8  ZOOMY=11

Y el recorte de la hoja, de `pbSpriteSetAnimFrame` del mismo script:

    src_rect = ((pattern % 5) * 192, (pattern / 5) * 192, 192, 192)

o sea rejilla de CINCO columnas de 192 px, siempre, sea cual sea el ancho
de la imagen. `pattern` negativo no es un dibujo de la hoja: -1 es el sprite
del que ataca y -2 el del que recibe. Los dos se saltan — esos bichos los
pinta el reloj por su cuenta.
"""
import os

from PIL import Image

# Los indices de AnimFrame (BattleAnimationPlayer del Anil)
X, Y, ZOOMX, ANGLE, MIRROR, BLENDTYPE, VISIBLE, PATTERN, OPACITY = range(9)
ZOOMY = 11

LADO = 192          # el lado de celda que usa pbSpriteSetAnimFrame
COLS = 5            # ...y sus cinco columnas, fijas

# Battle::Scene del Anil (script Battle_Scene). El golpe se monta centrado
# en el punto donde esta el que lo RECIBE, que es donde el reloj lo pinta.
FOCO_ALVO = (384, 96)

# El lienzo donde se montan los fotogramas, en coordenadas del juego. Tiene
# que dar de si: una celda de 192 con zoom 300 ocupa 576, y las hay que se
# salen de la pantalla a proposito.
LIENZO = 768


def _texto(v):
    return v.decode("utf-8", "replace") if isinstance(v, bytes) else str(v or "")


def carga(ruta):
    """{NOMBRE_DE_GOLPE: animacion} leyendo PkmnAnimations.rxdata.

    Se queda con los bloques `Move:X`. `OppMove:X` es la version de cuando
    el golpe lo tira el rival y solo cambia de lado, asi que vale de
    respaldo si falta la otra.
    """
    import marshal_lite

    todas = marshal_lite.load(ruta)
    propias, ajenas = {}, {}
    for a in todas:
        if not a:
            continue
        nombre = _texto(a.iv.get("name"))
        if ":" not in nombre:
            continue
        clase, _, golpe = nombre.partition(":")
        golpe = golpe.strip().upper()
        if clase == "Move":
            propias[golpe] = a
        elif clase == "OppMove":
            ajenas[golpe] = a
    for k, v in ajenas.items():
        propias.setdefault(k, v)
    return propias


def sonido_de(anim):
    """El fichero de Audio/SE/Anim que suena al tirar el golpe, o "".

    Sale de los `timing` de la animacion, que es la misma tabla de la que
    salen los fotogramas: cada uno dice en que fotograma pasa algo, y los
    de tipo 0 son "suena este SE" (lo hace `playTiming` del Anil, que
    llama a pbSEPlay("Anim/" + name)). Se coge el PRIMERO: es el golpe en
    si; los siguientes suelen ser la reaccion.
    """
    for t in (anim.iv.get("timing") or []):
        iv = getattr(t, "iv", None)
        if iv is None or iv.get("timingType", 0) != 0:
            continue
        nom = _texto(iv.get("name"))
        if nom:
            return nom
    return ""


def sonidos_de(anim):
    """TODOS los SE del golpe: [(fotograma, fichero, volumen, tono)], en orden.

    `sonido_de` coge solo el primero de la lista, y la lista NO va en orden
    de tiempo: en Thunderbolt el primero es el del fotograma 14 y se perdia
    el trueno del 1. En Mud Bomb el primero es el disparo (todo grave, por
    debajo de 500 Hz: en el altavoz de la placa no se oye nada) y el
    chapoteo del fotograma 12, que es lo que se oye, se quedaba fuera.
    """
    out = []
    for t in (anim.iv.get("timing") or []):
        iv = getattr(t, "iv", None)
        if iv is None or iv.get("timingType", 0) != 0:
            continue
        nom = _texto(iv.get("name"))
        if nom:
            out.append((int(iv.get("frame", 0) or 0), nom,
                        int(iv.get("volume", 100) or 0),
                        int(iv.get("pitch", 100) or 100)))
    out.sort(key=lambda s: s[0])
    return out


def hoja_de(anim):
    return _texto(anim.iv.get("graphic"))


def _gira_tono(im, hue):
    """El `hue` de la animacion, que es un giro de tono de 0 a 359."""
    if not hue:
        return im
    rgb = im.convert("RGB").convert("HSV")
    h, s, v = rgb.split()
    h = h.point(lambda p: (p + hue * 255 // 360) % 256)
    out = Image.merge("HSV", (h, s, v)).convert("RGB").convert("RGBA")
    out.putalpha(im.getchannel("A"))
    return out


class Hojas:
    """Las hojas de Graphics/Animations, leidas una sola vez.

    Son 704 ficheros y algunos pesan lo suyo; sin esto el montaje relee la
    misma hoja una vez por fotograma.
    """

    def __init__(self, carpeta):
        self.carpeta = carpeta
        self._cache = {}

    def get(self, nombre, hue=0):
        clave = (nombre, hue)
        if clave in self._cache:
            return self._cache[clave]
        ruta = os.path.join(self.carpeta, nombre)
        im = None
        if nombre and os.path.exists(ruta):
            try:
                im = Image.open(ruta).convert("RGBA")
                im = _gira_tono(im, hue)
            except Exception:                      # noqa: BLE001
                im = None
        self._cache[clave] = im
        return im


def _pinta_celda(lienzo, hoja, cel):
    """Una celda sobre el lienzo, con su zoom, giro, espejo y opacidad."""
    pat = cel[PATTERN]
    if pat is None or pat < 0:
        return False                 # -1 y -2 son los bichos, no la animacion
    if len(cel) > VISIBLE and cel[VISIBLE] == 0:
        return False
    fila, col = pat // COLS, pat % COLS
    if (col + 1) * LADO > hoja.width or (fila + 1) * LADO > hoja.height:
        return False                 # patron fuera de la hoja: se ignora
    trozo = hoja.crop((col * LADO, fila * LADO,
                       (col + 1) * LADO, (fila + 1) * LADO))
    if not trozo.getbbox():
        return False                 # celda vacia

    zx = cel[ZOOMX] if cel[ZOOMX] else 100
    zy = cel[ZOOMY] if len(cel) > ZOOMY and cel[ZOOMY] else zx
    if zx != 100 or zy != 100:
        nw = max(1, round(LADO * zx / 100.0))
        nh = max(1, round(LADO * zy / 100.0))
        if nw > LIENZO * 2 or nh > LIENZO * 2:
            return False             # un zoom absurdo: no cabe ni centrado
        trozo = trozo.resize((nw, nh), Image.LANCZOS)
    if cel[MIRROR]:
        trozo = trozo.transpose(Image.FLIP_LEFT_RIGHT)
    if cel[ANGLE]:
        # RMXP gira en sentido CONTRARIO a las agujas, igual que PIL con
        # angulo positivo; `expand` para no cortarle las esquinas.
        trozo = trozo.rotate(cel[ANGLE], resample=Image.BICUBIC, expand=True)

    op = cel[OPACITY] if len(cel) > OPACITY and cel[OPACITY] is not None else 255
    op = max(0, min(255, op))
    if op == 0:
        return False
    if op != 255:
        alfa = trozo.getchannel("A").point(lambda p: p * op // 255)
        trozo.putalpha(alfa)

    # El sprite se ancla por su CENTRO (ox = ancho/2, oy = alto/2).
    cx = cel[X] - FOCO_ALVO[0] + LIENZO // 2
    cy = cel[Y] - FOCO_ALVO[1] + LIENZO // 2
    px = int(round(cx - trozo.width / 2.0))
    py = int(round(cy - trozo.height / 2.0))

    capa = Image.new("RGBA", lienzo.size, (0, 0, 0, 0))
    capa.paste(trozo, (px, py))
    if len(cel) > BLENDTYPE and cel[BLENDTYPE] == 1:
        # blend_type 1 del RGSS es ADITIVO: es lo que hace que un destello se
        # vea como luz y no como una mancha opaca encima. Se suma el color y
        # se queda el alfa mas alto de los dos. Lo transparente convierte a
        # negro, que sumando no aporta nada, que es justo lo que se quiere.
        from PIL import ImageChops
        rgb = ImageChops.add(lienzo.convert("RGB"), capa.convert("RGB"))
        alf = ImageChops.lighter(lienzo.getchannel("A"), capa.getchannel("A"))
        # OJO: `lienzo` es el de quien llama, asi que se vuelca DENTRO. Con
        # un `lienzo = ...` la suma se quedaria en esta funcion.
        lienzo.paste(Image.merge("RGBA", (*rgb.split(), alf)), (0, 0))
    else:
        lienzo.alpha_composite(capa)
    return True


def cuadros(anim, hojas, max_lado=150, max_cuadros=12):
    """Los fotogramas de la animacion, ya recortados y escalados.

    Devuelve (lista_de_imagenes, escala) o (None, 0) si no sale nada.

    Los fotogramas se recortan TODOS a la MISMA caja — la union de lo que
    pinta cada uno — y no cada uno a la suya. El reloj centra cada
    fotograma en el mismo punto, asi que recortandolos por separado el
    movimiento se pierde: un golpe que cruza la pantalla se quedaba
    temblando en el sitio.
    """
    nombre = hoja_de(anim)
    hoja = hojas.get(nombre, anim.iv.get("hue") or 0)
    if hoja is None:
        return None, 0

    brutos = []
    for fr in anim:
        lienzo = Image.new("RGBA", (LIENZO, LIENZO), (0, 0, 0, 0))
        algo = False
        for cel in fr:
            if not cel:
                continue
            if _pinta_celda(lienzo, hoja, cel):
                algo = True
        brutos.append(lienzo if algo else None)

    # fuera los vacios de los extremos; los de en medio se quedan, que ahi
    # el parpadeo ES la animacion
    i, j = 0, len(brutos)
    while i < j and brutos[i] is None:
        i += 1
    while j > i and brutos[j - 1] is None:
        j -= 1
    brutos = brutos[i:j]
    if not brutos:
        return None, 0

    caja = None
    for im in brutos:
        if im is None:
            continue
        b = im.getbbox()
        if not b:
            continue
        caja = b if caja is None else (min(caja[0], b[0]), min(caja[1], b[1]),
                                       max(caja[2], b[2]), max(caja[3], b[3]))
    if not caja:
        return None, 0

    return _monta(brutos, caja, max_lado, max_cuadros)


def celdas(hojas, nombre, ini, fin, hue=0, max_lado=150, max_cuadros=12):
    """Fotogramas sacados A MANO de un tramo de celdas de una hoja.

    Para los golpes cuya animacion del Anil vive en el SPRITE y no en la
    hoja: alli no hay nada que copiar. La rejilla es la de siempre, cinco
    columnas de 192, asi que la celda `p` esta en ((p%5)*192, (p//5)*192).
    """
    hoja = hojas.get(nombre, hue)
    if hoja is None:
        return None, 0
    brutos = []
    for p in range(ini, fin):
        x, y = (p % 5) * LADO, (p // 5) * LADO
        if x + LADO > hoja.width or y + LADO > hoja.height:
            break
        cel = hoja.crop((x, y, x + LADO, y + LADO))
        brutos.append(cel if cel.getbbox() else None)
    if not any(b is not None for b in brutos):
        return None, 0
    caja = None
    for im in brutos:
        if im is None:
            continue
        b = im.getbbox()
        if not b:
            continue
        caja = b if caja is None else (min(caja[0], b[0]), min(caja[1], b[1]),
                                       max(caja[2], b[2]), max(caja[3], b[3]))
    if not caja:
        return None, 0
    return _monta(brutos, caja, max_lado, max_cuadros)


def _monta(brutos, caja, max_lado, max_cuadros):
    """Recorta todos los fotogramas a la MISMA caja y los escala."""
    # Si son mas de los que caben, se reparten a lo largo de la animacion en
    # vez de cortar por el final: quedarse con los 12 primeros de 40 deja el
    # golpe a medias, y repartiendo se ve entero aunque mas a saltos.
    if len(brutos) > max_cuadros:
        paso = len(brutos) / float(max_cuadros)
        brutos = [brutos[min(len(brutos) - 1, int(k * paso))]
                  for k in range(max_cuadros)]

    an, al = caja[2] - caja[0], caja[3] - caja[1]
    div = 1
    while max(an, al) // div > max_lado:
        div += 1

    salida = []
    for im in brutos:
        if im is None:
            salida.append(None)
            continue
        rec = im.crop(caja)
        if div > 1:
            rec = rec.resize((max(1, an // div), max(1, al // div)), Image.LANCZOS)
        salida.append(rec)
    return salida, div
