#!/usr/bin/env python3
"""Genera art.h: todo el pixel art que va en FLASH, sacado del Anil.

Tres grupos, un solo formato y un solo decodificador (`drawArt`):

  - Los retratos de la intro (profesor y los dos entrenadores).
  - El huevo, con su tira de 5 frames de grietas. El huevo que habia era un
    borron dibujado a mano en species.h y no parecia de Pokemon; las grietas
    eran dos listas de pixeles sueltos (CRACK1/CRACK2).
  - Los 67 iconos de objeto. Los 67 estan en el Anil: el nombre de fichero es
    el nombre interno del PBS, o sea nuestro nombre de pantalla en mayusculas
    y sin espacios. Ninguno pasa de 15 colores.

Por que en flash y no en la SD: la intro es lo primero que se ve al montar
el cacharro (no puede ser un error de tarjeta) y los iconos de la bolsa son
pequenos y se piden de golpe al pintar una lista, que con la SD seria un
tiron. Los bichos, que son 100 veces mas grandes, siguen en la SD.

Formato: pares RLE (repeticiones 1..255, indice de paleta), 0xFF =
transparente. Con pixel art de pocos colores y mucho fondo vacio baja mucho.
"""
import glob
import io
import os
import re
import struct
import sys

from PIL import Image, ImageEnhance, ImageFilter

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
# Que hoja usa cada golpe SEGUN EL ANIL (tools/gen_fxmap.py lo saca de
# Data/PkmnAnimations.rxdata). Es el dato de verdad; antes se adivinaba.
from fx_map import FX_MAP  # noqa: E402
import fxanim  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ANIL = r"C:\Users\foulu\Downloads\Pokemon Anil V4.13\Graphics"
TRAINERS = os.path.join(ANIL, "Trainers")
EGGS = os.path.join(ANIL, "Pokemon", "Eggs")
ITEMS = os.path.join(ANIL, "Items")

RETRATOS = [
    ("ART_OAK",  os.path.join(TRAINERS, "OAK.png")),
    ("ART_BOY",  os.path.join(TRAINERS, "POKEMONTRAINER_RojoNeutro.png")),
    ("ART_GIRL", os.path.join(TRAINERS, "POKEMONTRAINER_HojaNeutra.png")),
]

UI = os.path.join(ANIL, "UI", "Trainer Card")

# Los trece desafios, en el orden de GYM_TBL (gyms.h). El octavo es GIOVANNI
# y no LIDER8: el octavo lider del Anil (Urano) es de su propia historia, no
# de Kanto. GIOVANNI1 es su primer combate, el del gimnasio de Viridian.
LIDERES = ([f"LIDER{i}.png" for i in range(1, 8)] + ["GIOVANNI1.png"] +
           [f"ALTOMANDO{i}.png" for i in range(1, 5)] + ["CAMPEON.png"])

# Los de JOHTO no estan en el Anil (su octavo lider es Urano y de ahi para
# arriba va el Alto Mando de Kanto), ni en Unbreakable Ties ni en Reloaded.
# Se bajan de Bulbagarden Archives con tools/fetch_leaders.py, que es quien
# fija el orden; aqui solo se leen los ficheros ya numerados.
#
LIDERES_JOHTO = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                             "leaders_johto")
LIDERES_HOENN = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                             "leaders_hoenn")
LIDERES_SINNOH = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                              "leaders_sinnoh")
LIDERES_UNOVA = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                             "leaders_unova")
LIDERES_KALOS = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                             "leaders_kalos")

# ALOLA no se baja de Archives: esta en el OTRO juego del usuario. Las
# diecisiete caras de la region estan en Unbreakable Ties como
# SUMMERRESORT_*, a 160 px y en pixel art de Essentials, o sea el mismo
# formato que las del Anil. El prefijo es del evento de ese juego; la ropa
# es la normal, no banadores (se abrieron para comprobarlo).
#
# En Alola no hay gimnasios: hay capitanes de prueba y kahunas. El orden
# es el de la aventura, y los cinco ultimos hacen de Alto Mando.
LIDERES_GALAR = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                             "leaders_galar")
LIDERES_PALDEA = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                              "leaders_paldea")

UT_TRAINERS = (r"C:\Users\foulu\Downloads\Pokemon Unbreakable Ties 4.9 (ENG)"
               r"\Pokemon Unbreakable Ties 4.9 (ENG)\Graphics\Trainers")
ALOLA_CARAS = ["ILIMA", "HALA", "LANA", "KIAWE", "MALLOW", "OLIVIA",
               "SOPHOCLES", "ACEROLA", "NANU", "MINA", "HAPU", "MOLAYNE",
               "KUKUI"]

# TODOS los retratos acaban con 80 px de alto, y como llegar ahi sale del
# TAMANO DE ORIGEN y no de una lista a mano. Antes era un bool "reducir a
# la mitad" que valia mientras hubo dos clases de fichero; con cinco
# regiones son cuatro tamanos distintos (160 del Anil, 80 de HGSS/DP/BW,
# 64 de RS, 256 de los overworld de XY) y el bool ya no decia nada.
RETRATO_PX = 80


# El ESTILO GBA que el usuario le dio a los graficos del Anil: borde negro
# y mas color. No es una suposicion — se midio comparando Graphics/ contra
# Graphics_ORIGINAL_BACKUP/ en seis retratos (_claude_scratch/borde*.py):
#
#   borde       3 px por FUERA de la silueta, negro puro. La dilatacion del
#               alfa cuadra con la de ahora al 0,0% en los seis.
#   saturacion  ImageEnhance.Color a ~1,6 (sale entre 1,5 y 1,7 segun la
#               imagen). La luminancia no se mueve.
#
# Lo que viene del Anil YA lo trae. Lo de Archives y lo de Unbreakable Ties
# no, y al lado se nota: el de al lado tiene borde y color y este no.
GBA_SAT = 1.6
GBA_BORDE_160 = 3      # a tamano del Anil
GBA_BORDE_80 = 1       # ya reducido. Con 2 el borde pesa mas que el suyo


def estilo_gba(im, grosor, sat=GBA_SAT):
    """Sube el color y rodea la silueta con un borde negro de `grosor`."""
    col = ImageEnhance.Color(im.convert("RGB")).enhance(sat).convert("RGBA")
    col.putalpha(im.getchannel("A"))
    if grosor <= 0:
        return col
    d = im.getchannel("A").point(lambda v: 255 if v >= 8 else 0)
    for _ in range(grosor):
        d = d.filter(ImageFilter.MaxFilter(3))
    out = Image.new("RGBA", im.size, (0, 0, 0, 0))
    out.paste((0, 0, 0, 255), (0, 0), d)
    out.alpha_composite(col)
    return out


def a_retrato(im, del_anil):
    """Deja la imagen en RETRATO_PX de alto y con el estilo del Anil.

    `del_anil` dice si el fichero ya trae el borde y el color: los del
    propio juego si, los de fuera no. El borde se pone ANTES de reducir
    cuando hay que reducir, para que acabe con el mismo grosor aparente
    que el de los del Anil (3 px a 160 = 1,5 a 80).
    """
    h = im.height
    if h == RETRATO_PX:
        # HGSS, DP, BW: ya vienen al tamano bueno.
        return im if del_anil else estilo_gba(im, GBA_BORDE_80)
    if h < RETRATO_PX:
        # Los de RUBI/ZAFIRO son de 64. Subirlos a 80 no es un multiplo y
        # deja el pixel art sucio: se quedan como estan, un 20% mas bajos.
        return im if del_anil else estilo_gba(im, GBA_BORDE_80)
    if h % RETRATO_PX == 0:
        if not del_anil:
            im = estilo_gba(im, GBA_BORDE_160 if h == RETRATO_PX * 2 else 0)
        return mitad(im) if h == RETRATO_PX * 2 else im.resize(
            (im.width * RETRATO_PX // h, RETRATO_PX), Image.NEAREST)
    # No es multiplo (los overworld de XY vienen a 256 con el dibujo
    # ocupando casi todo): recortar al contenido y bajar con LANCZOS. Aqui
    # si compensa, porque el original no es pixel art de 1:1. El borde va
    # DESPUES de reducir, para que salga limpio y no emborronado.
    im = im.crop(im.getbbox())
    im = im.resize((max(1, im.width * RETRATO_PX // im.height), RETRATO_PX),
                   Image.LANCZOS)
    return im if del_anil else estilo_gba(im, GBA_BORDE_80)


def _anil(*nombres):
    """Retratos del propio Anil: ya traen el estilo GBA."""
    return [(os.path.join(TRAINERS, n), True) for n in nombres]


def _fuera(carpeta):
    """Retratos de Archives: hay que ponerles el estilo."""
    return [(p, False) for p in sorted(glob.glob(os.path.join(carpeta, "*.png")))]


# De Hoenn el Anil SI tiene los ocho de gimnasio (LIDER1HOENN..LIDER8HOENN);
# del Alto Mando, ninguno, asi que esos cinco se bajan igual que Johto.
REGION_RETRATOS = [
    ("KANTO", _anil(*LIDERES)),
    ("JOHTO", _fuera(LIDERES_JOHTO)),
    # El campeon de Hoenn es MAXIMO, que es como el Anil llama a Maximo /
    # Steven. Estaba bajado de RUBI (64 px) al lado de ocho lideres del
    # Anil a 80: el del Anil es el mismo personaje, con el mismo tamano, la
    # misma paleta y el estilo ya puesto.
    ("HOENN", _anil(*[f"LIDER{i}HOENN.png" for i in range(1, 9)])
              + _fuera(LIDERES_HOENN)[:4]
              + _anil("MAXIMO.png")),
    # Y la campeona de Sinnoh es CINTIA, que TAMBIEN esta en el Anil. Se
    # habia dado por hecho que era un personaje de su propia historia sin
    # abrir el fichero; es Cintia, rubia de abrigo negro, inconfundible.
    ("SINNOH", _fuera(LIDERES_SINNOH)[:12] + _anil("CINTIA.png")),
    ("UNOVA", _fuera(LIDERES_UNOVA)),
    # Kalos son los de OVERWORLD y no los de combate: de la gen 6 en
    # adelante no hay sprite de combate, solo el retrato VS de medio
    # cuerpo, que al lado de un entrenador entero de HGSS parece de otro
    # juego (ver fetch_leaders.py).
    ("KALOS", _fuera(LIDERES_KALOS)),
    # Unbreakable Ties es del usuario pero NO lleva el estilo GBA que le
    # puso al Anil, asi que estos pasan por estilo_gba() como los de
    # Archives: van con del_anil = False.
    ("ALOLA", [(os.path.join(UT_TRAINERS, f"SUMMERRESORT_{n}.png"), False)
               for n in ALOLA_CARAS]),
    ("GALAR", _fuera(LIDERES_GALAR)),
    ("PALDEA", _fuera(LIDERES_PALDEA)),
]

# Los treinadores de a pie que se cruzan entre gimnasio y gimnasio. Ocho
# llegan: son variedad, no contenido, y cada uno son ~3,5 KB de flash.
# Mismo orden que TRAINER_TBL en trainers.h.
ENTRENADORES = [
    "CAZABICHOS.png",   # bug catcher
    "JUGON.png",        # gamer
    "NADADOR.png",      # swimmer
    "MOTORISTA.png",    # biker
    "CIENTIFICA.png",   # scientist
    "MONTANERO.png",    # hiker
    "PESCADOR.png",     # fisherman
    "ROCKETO.png",      # Team Rocket
]

# Iconos de la pantalla principal. Van aparte de ART_ITEM porque no todos
# son objetos de nuestra bolsa y porque aqui el orden lo manda la UI.
# Antes eran mapas de caracteres dibujados a mano y se notaba: una pokeball
# cuadrada, una gota que no parecia agua.
UI_ICONS = [
    ("ART_UI_BALL",  "Items/POKEBALL.png"),
    ("ART_UI_FOOD",  "Items/ORANBERRY.png"),
    ("ART_UI_BATH",  "Items/FRESHWATER.png"),
    # El selector de comida. Las tres bayas son las del juego, elegidas por
    # COLOR (Pet::feedBerry las llama roja, azul y verde) y no por su efecto
    # en los juegos, que aqui no existe: la azul es la misma ORANBERRY del
    # boton. Antes eran tres borrones de 16x16 dibujados a mano.
    ("ART_FEED_RED",   "Items/CHERIBERRY.png"),
    ("ART_FEED_GREEN", "Items/LUMBERRY.png"),
    ("ART_FEED_CANDY", "Items/CASTELIACONE.png"),   # o doce: sorvete (o Rare Candy confundia; escolha do dono)
]

# EL SELLO DE LA MEGAEVOLUCION. Pictures/megaevolucion.png es el orbe de
# 512x384 con el dibujo centrado en (127,63)-(384,320): el arcoiris con la
# helice grabada, que es el simbolo de verdad y no un rombo pintado a mano.
#
# Dos medidas, porque son dos usos y encoger mal estropea el dibujo: la
# chica va en el boton (radio 27) y la grande la amplia la escena.
#
# Hay que CUANTIZAR: el orbe es un degradado y `indexa` guarda un color por
# tono, con 0xFF reservado, asi que a pelo se pasa de 254. Con 48 el
# arcoiris sigue leyendose (comprobado en hoja de contacto) y el RLE
# comprime mucho mejor, porque las bandas planas son tiradas largas.
MEGA_PNG = "Pictures/megaevolucion.png"
MEGA_CAJA = (127, 63, 384, 320)
MEGA_COLORES = 48
MEGA_LADOS = [("ART_MEGA", 36), ("ART_MEGA_BIG", 88)]

# Los simbolos de tipo salen de UI/types.png (19 filas de 64x28). Se coge
# SOLO el icono de la izquierda: el texto de la chapa esta en espanol
# ("SEA" por agua, "AIR" por volador) y el nuestro va en ingles, asi que la
# etiqueta la seguimos poniendo nosotros y de aqui sale el dibujo.
TYPES_PNG = "UI/types.png"
TYPE_ROW_H = 28
TYPE_ICON_W = 26

# La marca de shiny. Cinco destellos de 192x192 en fila; el 1 es la estrella
# de cuatro puntas, que es la que se reconoce. Los otros son un brillo fino,
# un estallido y dos rayas.
SHINY_PNG = "Animations/Shiny.png"
SHINY_FRAME = 1

# La bola que vuela en la captura. NO vale el icono de la bolsa (Items/*.png):
# ese esta dibujado para una lista, de tres cuartos y con su brillo de icono.
# Battle animations/ball_*.png son las que el propio juego lanza: rejilla de
# 8x2 celdas de 32, y la celda 0 es la bola cerrada de frente.
BALL_PNG = "Battle animations/ball_%s.png"
# La MISMA bola, abierta: 32x64, la tapa arriba (filas 0..32) y la base
# debajo (32..49), ya separadas. Es la que faltaba para que la captura no
# fuera una bola cerrada quieta mientras el bicho seguia plantado fuera.
BALL_OPEN_PNG = "Battle animations/ball_%s_open.png"
BALL_OPEN_H = 50       # medido: la parte con dibujo de las dos mitades
BALL_CELL = 32
BALL_OFF = 16          # el desfase vertical de la rejilla (medido, ver abajo)
BALL_SPIN = 8          # las 8 celdas de la fila 0 son la bola GIRANDO
# As quatro de escalao e depois as de condicao, na ordem de BALL_ALL
# (items.h): gen_items.py e quem decide essa ordem, aqui so se le.
from gen_items import BOLAS_EXTRA  # noqa: E402
BALL_NAMES = (["POKEBALL", "GREATBALL", "ULTRABALL", "MASTERBALL"]
              + [b[0] for b in BOLAS_EXTRA])

# ---------------------------------------------------------------- efectos
#
# Un efecto por TIPO y no por golpe: son 437 golpes y 18 tipos, y lo que el
# jugador tiene que leer en medio segundo es "esto fue fuego", no cual de los
# doce golpes de fuego era.
#
# Las hojas son de Graphics/Animations, rejillas de celdas de 192. ELEGIDAS
# MIRANDOLAS una a una (_claude_scratch/fx.py); dos trampas que costaron tres
# rondas: muchas hojas PRAS guardan las PALETAS de recolor en los ultimos
# fotogramas, asi que muestrear salteado ensena bolitas de colores en vez de
# la animacion; y algunas empiezan con un fotograma que no es del efecto (el
# 0 de water3 es una elipse verde, el de Night Daze un circulo). De ahi
# `desde` y `cuantos`.
#
# El indice de la lista es nuestro T_* (dex.h), empezando por T_NONE.
FX_CELL = 192
FX_SHEETS = [
    (None, 0, 0),                                  # T_NONE: sin efecto
    ("PRAS- Vice Grip.png", 0, 5),                 # NORMAL: un tajo amarillo
    ("PRAS- Force Palm.png", 0, 4),                # FIGHT: palma y onda
    ("PRAS- Aeroblast.png", 4, 8),                 # FLYING: se forma un remolino
    ("poison.png", 0, 6),                          # POISON
    ("PRAS- Earth Power.png", 0, 5),               # GROUND: erupcion
    ("RockClimbEX.png", 0, 8),                     # ROCK: piedras que crecen
    ("PRAS- Electroweb.png", 0, 5),                # BUG: la telarana
    ("ghost1.png", 0, 6),                          # GHOST
    ("Flames.png", 0, 3),                          # FIRE (los otros 2 van vacios)
    ("water3.png", 1, 6),                          # WATER: el 0 es una elipse verde
    ("grass2.png", 0, 6),                          # GRASS
    ("electric1.png", 0, 6),                       # ELEC
    ("PRAS- Psybeam.png", 0, 4),                   # PSYCHIC: anillos
    ("PRAS- Frost Breath.png", 0, 6),              # ICE
    ("PRAS- Dragonbreath.png", 0, 3),              # DRAGON
    ("PRAS- Night Daze.png", 1, 3),                # DARK: el 0 es un circulo suelto
    ("PRAS- Iron Head.png", 0, 2),                 # STEEL: impacto blanco
    ("PRAS- Moonblast.png", 0, 2),                 # FAIRY
]
# Los golpes que NO pegan no pueden usar el efecto de su tipo: Tail Whip es
# de tipo normal y salia el tajo de Vice Grip, o sea que bajar la defensa se
# veia como un aranazo. Estas cinco van por lo que el golpe HACE.
#
# Ojo: los de clima y terreno, y los que meten estado (Thunder Wave, Sleep
# Powder, Toxic), se quedan con el de su TIPO a proposito ???????? ahi el tipo es
# justo lo que cuenta, y el rayo de Thunder Wave es lo que uno espera ver.
FX_SUPPORT = [
    ("PRAS- Bulk Up.png", 0, 5),          # 0 subir stats
    ("PRAS- Baby Doll Eyes.png", 0, 6),   # 1 bajar stats del rival
    ("PRAS- Aromatherapy.png", 0, 5),     # 2 curarse
    ("PRAS- Crafty Shield.png", 0, 5),    # 3 protegerse / pantallas
    ("PRAS- Defog.png", 0, 6),            # 4 lo demas que no pega
]
FX_SUP_NOM = ["RAISE", "LOWER", "HEAL", "GUARD", "OTHER"]

# --- efecto PROPIO de cada golpe -------------------------------------------
#
# Lo de arriba (tipo y categoria) es el respaldo. Encima va, cuando existe,
# la animacion del golpe concreto: el Anil trae 704 hojas en Animations y
# 141 de nuestros 441 golpes tienen la suya con nombre reconocible.
#
# MEDIDO antes de decidir: 775 fotogramas = 354 KB, a 469 bytes por
# fotograma. Con 1,87 MB de flash libres cabe de sobra, asi que van a FLASH
# y no a la tarjeta: asi no hay que cargar nada al usar el golpe y siguen
# funcionando sin SD. (Los fondos y el sonido si van al SD porque son
# fotografia y musica, que es otro orden de magnitud.)
#
# OJO: esto lee moves.h, asi que gen_art.py hay que correrlo DESPUES de
# gen_moves.py. Si el orden de los golpes cambia y no se regenera, el
# indice apunta al golpe de al lado.
MOVES_H = os.path.join(ROOT, "moves.h")
FX_MOVE_CEL = 12         # tope de fotogramas por golpe (SDFX_MAX_FRAMES)
# Tope de bytes por golpe. Subido de 12000 a 18000 MIDIENDO: con 12000 se
# caian mapeos a mano buenos (015-Fire01 pide 17 KB, Water pulse 13) y el
# golpe volvia al efecto generico sin decirlo. A 18000 entran esos y se
# siguen quedando fuera los de verdad caros (Surf 36 KB, Earth1 28, fire
# blast 25), que son hojas casi fotograficas: cuatro de esas se comian
# 100 KB y el respaldo por tipo ya es correcto y ademas se mueve.
# Lo que puede ocupar la animacion de UN golpe en /fx/fx.bin.
#
# El tope no es el espacio (la tarjeta va sobrada): es el TIEMPO DE LECTURA.
# Medido en la placa, la SD da 0,18-0,25 MB/s.
#
# EL NUMERO VIEJO (40 KB) SALIA DE UNA CUENTA MAL HECHA, y la escribi yo:
# decia que "la pausa del anuncio da 850 ms de cobertura", pero la lectura
# estaba en btResolve(), que corre JUSTO DESPUES de esa pausa. Lo que
# cubria de verdad eran los ~200 ms que quedan hasta la embestida —— y de
# ahi los 40 KB. Por esa cuenta se quedaban fuera 51 golpes, entre ellos
# Crunch, Rock Slide, Surf, Psychic y Play Rough, que se veian con el
# efecto generico de su tipo. Lo trajo el dueno: "varios golpes ainda com
# animacoes genericas ou sem mto sentido".
#
# Ahora la hoja se pide en btLaunch y la trae battleTick MIENTRAS se lee
# la frase, asi que la cobertura es de verdad: 850 ms menos los ~150 de
# dejar el anuncio pintado = 700 ms. Al extremo LENTO de lo medido
# (0,18 MB/s) eso son 126 KB, asi que el tope se pone en 120.
#
# Es una cuenta, no una medida: battleTick imprime por el puerto serie lo
# que tarda cada lectura ("fx <golpe> <KB> en <ms>"), para poder
# comprobarlo en la placa en vez de volver a suponerlo.
# 120000 -> 128000: con 120 se quedaba fuera PRAS- Bite.png por CINCO
# KB (122), y de esa hoja viven Mordisco y Triturar, que los aprenden
# un monton. 122 KB a 0,18 MB/s son 678 ms y la cobertura son 700: entra
# justo, y por eso la medida del puerto serie importa aqui de verdad.
FX_MOVE_MAX_B = 128000

# Mapeos A MANO: golpes cuya hoja existe con OTRO nombre. El casamiento
# automatico solo pilla los que se llaman igual, y quedaban fuera justo los
# que mas se ven jugando (Tackle lo aprenden 51 de las 151).
#
# Se eligen mirando la hoja, no adivinando por el nombre: "Scratch + Shadow
# Claw" tiene un solo fotograma util y "Acid Downpour" es el fondo rosa de
# un movimiento Z a pantalla completa. Los descartados se quedan con el
# efecto de su tipo, que ya es correcto.
#
# Varios golpes pueden compartir hoja: un golpe cuerpo a cuerpo es un
# impacto, y Tackle, Pound y Quick Attack se ven igual en los juegos.
FX_A_MANO = {
    # Aqui hubo ~45 mapeos puestos ADIVINANDO que hoja pegaba con cada
    # golpe. Se quitaron: Data/PkmnAnimations del Anil ya dice la hoja de
    # CADA golpe (FX_MAP) y acierta mucho mas — Vine Whip es una liana
    # chasqueando, Dragon Claw una garra, Absorb una semilla. Como
    # FX_A_MANO gana a FX_MAP, dejarlos era tapar el dato bueno con una
    # suposicion.
    #
    # Lo de abajo son EXCEPCIONES comprobadas en el art.h generado.
    #
    # A estos el Anil los manda a una capa `_BG` (lavado a pantalla
    # completa), que no sirve: nosotros pintamos UNA capa sobre el bicho.
    "PSYCHIC": "PRAS- Psystrike.png",
    "FUTURESIGHT": "PRAS- Psystrike.png",
    "CONFUSION": "PRAS- Psycho Boost.png",
    "PSYSHOCK": "PRAS- Psycho Boost.png",
    "EXPLOSION": "PRAS- Explosions.png",
    "SELFDESTRUCT": "PRAS- Explosions.png",
    # Y este porque "PRAS- Grass.png" son PETALOS ROSAS cuatro fotogramas y
    # la hoja verde solo sale al final: peor que el remolino de hojas.
    "RAZORLEAF": "PRAS- Leaf Tornado.png",
    "TACKLE": "Tackle_B.png",
    "POUND": "Tackle_B.png",
    "QUICKATTACK": "Tackle_B.png",
    "TAKEDOWN": "Tackle_B.png",
    "BODYSLAM": "Tackle_B.png",
    "SCRATCH": "scratchbattle.png",
    "FURYATTACK": "scratchbattle.png",
    "FURYSWIPES": "scratchbattle.png",
    "ABSORB": "PRAS- Absorption.png",
    "MEGADRAIN": "PRAS- Absorption.png",
    "WRAP": "PRAS- Wrapping.png",
    "VINEWHIP": "PRAS- Wrapping.png",
    "POISONPOWDER": "PRAS- Powders.png",
    "SLEEPPOWDER": "PRAS- Powder.png",
    "STUNSPORE": "PRAS- Spore.png",
    "WATERGUN": "Water pulse.png",
    "BUBBLEBEAM": "Water pulse.png",
    "BUBBLE": "Water pulse.png",
    # segunda tanda: los mas aprendidos que quedaban, y los que perdieron
    # su hoja al descartar los fondos (Earthquake pintaba la pantalla).
    "ANCIENTPOWER": "Ancient-PowerFilesheet.png",
    "EARTHQUAKE": "Earth1.png",
    "MAGNITUDE": "Earth1.png",
    "BULLDOZE": "Earth1.png",
    "HARDEN": "PRAS- Sharpen.png",
    "DEFENSECURL": "PRAS- Sharpen.png",
    "WITHDRAW": "PRAS- Sharpen.png",
    "SLAM": "Sword5.png",
    "THRASH": "Sword5.png",
    "CUT": "Sword5.png",
    "PECK": "PRAS- Fly.png",
    "WINGATTACK": "PRAS- Fly.png",
    "HYPERFANG": "PRAS- Bite.png",
    "ZENHEADBUTT": "Tackle_B.png",
    "PURSUIT": "Tackle_B.png",
    "ASSURANCE": "Tackle_B.png",
    "KNOCKOFF": "Tackle_B.png",
    "SUCKERPUNCH": "Tackle_B.png",
    "DOUBLEEDGE": "Tackle_B.png",
    "DOUBLEKICK": "Tackle_B.png",
    # (PRAS- Rock.png queda fuera: mide 960x1144, que no es rejilla de 192.
    #  Roca ya tiene su efecto de tipo, que son piedras.)
}

# Golpes cuya animacion del Anil NO se puede copiar porque no esta en la
# hoja: el Anil la cuenta moviendo el SPRITE del bicho. Nosotros no
# coreografiamos sprites, asi que de la animacion solo sale la decoracion
# —— y queda una chispita quieta.
#
# Lo trajo el dueno: "flarebitz ta com animacao generica". Y tiene razon
# por partida doble: la llamita de Flare Blitz es la MISMA que usan otros
# ocho golpes de fuego de la hoja PRAS- Fire.png, asi que no solo es
# pobre, es que no se distingue de un Ascuas.
#
# Para estos se elige a mano una hoja que cuente lo mismo y SE DICE EL
# TRAMO de celdas, porque las hojas traen varias animaciones cosidas:
# Firebird.png empieza con cuatro rayitas y un disco rojo, y el ave de
# fuego no arranca hasta la celda 10. Sin el tramo saldrian las rayitas.
#
# (nombre del golpe) -> (fichero, primera celda, una pasada la ultima)
FX_CELDAS_A_MANO = {
    "FLAREBLITZ": ("Firebird.png", 10, 19),
}

# Golpes cuya hoja existe pero se ve PEOR que el respaldo de su tipo. Casi
# siempre porque la hoja es solo una capa suelta — los puntitos de carga de
# un rayo cuyo haz vive en el "_BG" que descartamos — y queda un efecto de
# cuatro pixeles. Mejor el del tipo, que al menos es una animacion entera.
#
# Comprobado mirando las hojas de contacto (_claude_scratch/verfx.py).
FX_PEOR_QUE_EL_TIPO = {
    "FLASHCANNON",   # puntitos de carga; el haz esta en el BG
    "GIGADRAIN",     # idem
    "METEORBEAM",    # idem
    "SCALESHOT",     # una escama en cinco colores: tira de paletas encubierta
    # A folha dele e CENARIO: planeta, campo de estrelas, a Terra. No
    # Anil sao 67 quadros em que o sprite faz tudo e isso e so o
    # fundo; sozinho vira um retalho de ceu por cima da batalha. O
    # impacto generico de Lutador diz muito mais.
    "SEISMICTOSS",
}


def _norm_fx(s):
    return re.sub(r"[^A-Z0-9]", "", s.upper())


def _sin_prefijo(k):
    for p in ("PRAS", "GEN8", "GEN9", "CUSTOM", "GEN"):
        if k.startswith(p):
            return k[len(p):]
    return k


def lado_celda(im):
    """El tamano de celda de una hoja de animacion.

    Casi todas son rejillas de 192, pero NO todas: Flash Cannon es 768x384
    con celdas de 384 y Decorate es 960x960 de una sola celda. Leyendolas a
    192 se emite un CUARTO del dibujo, que es lo que hacia que Flash Cannon,
    Giga Drain y Meteor Beam salieran como una bola blanca sosa — era el
    centro recortado de un efecto mas grande.

    Regla: si el ancho es multiplo del alto, la hoja es una fila de celdas
    cuadradas y el lado es el alto. Si no, es una rejilla de 192.
    """
    if im.height and im.width % im.height == 0:
        return im.height
    return FX_CELL


def reduce_colores(im, tope=24):
    """Baja el numero de colores SIN tocar la transparencia.

    El formato indexado se queda en 254 colores (0xFF es el transparente) y
    algunas hojas de efecto son casi fotograficas: Earth1 trae 649 y por eso
    se caia.

    El tope no es 254 sino 24, y eso NO es por el formato: es por el RLE.
    Con 200 colores casi no hay tiradas que repetir y cuatro golpes costaban
    137 KB; a 24 comprimen como el resto del arte y una explosion sigue
    leyendose igual. Medido, no supuesto.

    Se separa el alfa a proposito: PIL cuantiza mal el RGBA y deja halos.
    """
    alfa = im.getchannel("A")
    rgb = im.convert("RGB").quantize(colors=tope, method=Image.MEDIANCUT)
    out = rgb.convert("RGB").convert("RGBA")
    out.putalpha(alfa)
    return out


def _firma(cel):
    """(caja de contenido, pixeles opacos). Vacio -> None."""
    caja = recorte(cel)
    if caja[2] < 0:
        return None
    px = cel.load()
    n = sum(1 for y in range(cel.height) for x in range(cel.width)
            if px[x, y][3] >= 8)
    return (caja, n)


def quita_vacios(celdas):
    """Quita los fotogramas EN BLANCO del principio y del final.

    Muchas hojas del Anil son una CAPA de una animacion compuesta y traen el
    dibujo en una celda sola, con el resto de la rejilla vacia: `Tackle_B` es
    768x192 (cuatro celdas) y solo la primera tiene algo. Como la lista se
    emitia entera, el golpe pintaba un fotograma y se quedaba tres cuartos de
    la ventana sin dibujar nada — un parpadeo. Y afectaba a los golpes mas
    usados del juego (Tackle, Pound, Quick Attack, Body Slam, Take Down...).

    Los huecos DE EN MEDIO se respetan: ahi el parpadeo es la animacion (un
    destello que va y viene). Solo se recortan los extremos.
    """
    lleno = [recorte(c)[2] >= 0 for c in celdas]
    if not any(lleno):
        return celdas                      # toda vacia: que lo vea quien llama
    i = lleno.index(True)
    j = len(lleno) - 1 - lleno[::-1].index(True)
    return celdas[i:j + 1]


def quita_paletas(celdas):
    """Recorta las variantes de color pegadas al principio o al final.

    Muchas hojas traen el efecto y, cosidas, las versiones recoloreadas del
    mismo dibujo. Las hay por detras (Drain Punch: un pu??o y luego cuatro
    bolas naranjas) y por delante (Magical Leaf: cuatro circulos de colores
    y la hoja al final). La hoja entera no es una tira de paletas ??? el
    fotograma bueno la salva ???, asi que `es_tira_de_paletas` no la ve.

    Se corta una tanda de 3 o mas con silueta identica en cualquiera de los
    dos extremos, y nunca se deja menos de 2 fotogramas.
    """
    firmas = [_firma(c) for c in celdas]
    n = len(celdas)
    ini, fin = 0, n

    j = n - 1
    while j > 0 and firmas[j] is not None and firmas[j] == firmas[j - 1]:
        j -= 1
    if n - j >= 3 and j >= 2:
        fin = j

    i = 0
    while i + 1 < fin and firmas[i] is not None and firmas[i] == firmas[i + 1]:
        i += 1
    if i + 1 >= 3 and fin - (i + 1) >= 2:
        ini = i + 1

    return celdas[ini:fin]


def es_tira_de_paletas(celdas):
    """True si la hoja no es una animacion sino la MISMA forma recoloreada.

    Muchas hojas PRAS guardan asi las variantes de color: cinco circulos
    identicos en cinco tonos. Emitirlas da un efecto que es una bola lisa
    quieta ???????? peor que el generico del tipo, que al menos se mueve. Se
    reconocen porque la silueta no cambia: misma caja y practicamente el
    mismo numero de pixeles opacos en todos los fotogramas.

    Se descubrio con HYDROPUMP: salio una pelota verde azulada.
    """
    firmas = []
    for cel in celdas:
        caja = recorte(cel)
        if caja[2] < 0:
            continue
        px = cel.load()
        n = sum(1 for y in range(cel.height) for x in range(cel.width)
                if px[x, y][3] >= 8)
        firmas.append((caja, n))
    if len(firmas) < 2:
        return False
    caja0, n0 = firmas[0]
    if not n0:
        return False
    for caja, n in firmas[1:]:
        if caja != caja0:
            return False
        if abs(n - n0) * 100 > n0 * 5:      # mas de un 5% de diferencia
            return False
    return True


def golpes_con_animacion():
    """[(indice en MOVE_TBL, NOMBRE)] de los golpes que el Anil anima.

    Sustituye a golpes_con_hoja(), que casaba el nombre del golpe con el
    nombre de un fichero PNG y acertaba a medias. Data/PkmnAnimations dice
    golpe a golpe cual es su animacion, asi que aqui solo hay que cruzar
    con nuestra tabla.
    """
    with io.open(MOVES_H, encoding="utf-8", errors="replace") as f:
        txt = f.read()
    nombres = re.findall(r'^\s*\{\s*"([A-Z0-9]+)",\s*T_', txt, flags=re.M)
    # ANIL apunta a Graphics/, asi que Data/ cuelga del padre.
    raiz = os.path.dirname(ANIL.rstrip("\/"))
    anims = fxanim.carga(os.path.join(raiz, "Data", "PkmnAnimations.rxdata"))
    pares = [(i, nom) for i, nom in enumerate(nombres) if nom in anims]
    return pares, anims, len(nombres)


def golpes_con_hoja():
    """[(indice en MOVE_TBL, fichero)] de los golpes que tienen animacion."""
    with io.open(MOVES_H, encoding="utf-8", errors="replace") as f:
        txt = f.read()
    nombres = re.findall(r'^\s*\{\s*"([A-Z0-9]+)",\s*T_', txt, flags=re.M)

    # Las hojas vienen por CAPAS: "X.png" es el efecto, "X BG.png" es el
    # lavado de fondo a pantalla completa y "X FG.png" el primer plano.
    #
    # Coger el BG es un error que se ve enseguida: Earthquake, Psychic,
    # Thunder y Surf pintaban una mancha de color sobre todo el combate en
    # vez del golpe. Eran 40 de 208. Asi que el BG se DESCARTA ???????? el golpe
    # cae en el efecto de su tipo, que es correcto y ademas se mueve ???????? y
    # entre los demas gana el nombre limpio.
    #   0 = nombre limpio (el efecto), 1 = FG, 2 = variante/copia
    def rango(k):
        if k.endswith(("BG", "BGOPP")):
            return None                   # fuera
        for suf, r in (("FGOPP", 1), ("FG", 1), ("COPIE", 2), ("COPY", 2),
                       ("OLD", 2), ("OPP", 2)):
            if k.endswith(suf):
                return (r, k[: -len(suf)])
        return (0, k)

    carpeta = os.path.join(ANIL, "Animations")
    mejor = {}
    for fich in sorted(os.listdir(carpeta)):
        if not fich.lower().endswith(".png"):
            continue
        r = rango(_norm_fx(fich[:-4]))
        if r is None:
            continue
        prio, k = r
        for clave in (k, _sin_prefijo(k)):
            if clave and (clave not in mejor or prio < mejor[clave][0]):
                mejor[clave] = (prio, fich)
    hojas = {k: f for k, (_, f) in mejor.items()}
    carpeta = os.path.join(ANIL, "Animations")

    # LO QUE DICE EL ANIL gana al casamiento por nombre. Data/PkmnAnimations
    # declara, golpe a golpe, que hojas usa su animacion (tools/gen_fxmap.py
    # las saca). Mientras no se leyo, un golpe cuya hoja no se llamaba como
    # el caia en el efecto generico de su tipo: Razor Leaf sacaba lianas del
    # suelo cuando el Anil ya decia "PRAS- Grass.png".
    #
    # De la lista de capas se coge la PRIMERA que sirva: `rango()` descarta
    # los BG (lavados a pantalla completa, que es lo que rompia 40 golpes) y
    # prefiere la capa limpia sobre el FG.
    for golpe, capas in FX_MAP.items():
        if golpe in hojas:
            continue                       # ya lo tenia por nombre exacto
        for fich in capas:
            if not os.path.exists(os.path.join(carpeta, fich)):
                continue
            r = rango(_norm_fx(fich[:-4]))
            if r is None:
                continue                   # BG: no sirve de capa unica
            hojas[golpe] = fich
            break

    # y los mapeos a mano ganan a todo: son los que ya se miraron uno a uno
    for golpe, fich in FX_A_MANO.items():
        if not os.path.exists(os.path.join(carpeta, fich)):
            raise SystemExit(f"FX_A_MANO: no existe {fich} (para {golpe})")
        hojas[golpe] = fich

    out = []
    for i, n in enumerate(nombres):
        k = _norm_fx(n)
        if k in hojas and k not in FX_PEOR_QUE_EL_TIPO:
            out.append((i, hojas[k]))
    return out, len(nombres)

# Nombres solo para el comentario del .h, en el mismo orden.
FX_TIPOS = [
    "NONE", "NORMAL", "FIGHT", "FLYING", "POISON", "GROUND", "ROCK", "BUG",
    "GHOST", "FIRE", "WATER", "GRASS", "ELEC", "PSYCHIC", "ICE", "DRAGON",
    "DARK", "STEEL", "FAIRY",
]

# El tipo de cada uno de los ocho gimnasios, para tenir su insignia.
# T_ROCK, T_WATER, T_ELECTRIC, T_GRASS, T_POISON, T_PSYCHIC, T_FIRE, T_GROUND
BADGE_TYPE = [6, 10, 12, 11, 4, 13, 9, 5]

TRANSP = 0xFF
CRACK_FRAMES = 5   # 000_cracks.png son 5 fotogramas de 160x160 en fila


def rgb565(r, g, b):
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def recorte(im):
    px = im.load()
    x0, y0, x1, y1 = im.width, im.height, -1, -1
    for y in range(im.height):
        for x in range(im.width):
            if px[x, y][3] >= 8:
                x0, y0 = min(x0, x), min(y0, y)
                x1, y1 = max(x1, x), max(y1, y)
    return x0, y0, x1, y1


def sin_fondo(im):
    """Separa el simbolo de su cuadro de color. Devuelve (imagen, fondo).

    Las chapas del Anil son dos colores planos: el cuadro del tipo y el
    dibujo en blanco. Si se deja el cuadro, dentro de nuestra chapa queda un
    recuadro de otro tono; y si se tira el color, se pierde justo la paleta
    que hace legible el dibujo blanco. Asi que se quita el cuadro de la
    imagen y se devuelve su color para pintar la chapa con el.

    El fondo es el color opaco mas repetido: las esquinas no valen porque
    son transparentes (la chapa tiene las puntas redondeadas).
    """
    im = im.copy()
    px = im.load()
    cuenta = {}
    for y in range(im.height):
        for x in range(im.width):
            p = px[x, y]
            if p[3] >= 8:
                cuenta[p[:3]] = cuenta.get(p[:3], 0) + 1
    if not cuenta:
        return im, None
    fondo = max(cuenta, key=cuenta.get)
    for y in range(im.height):
        for x in range(im.width):
            if px[x, y][:3] == fondo:
                px[x, y] = (0, 0, 0, 0)
    return im, fondo


def mitad(im):
    """Encoge a la mitad exacta con NEAREST, ANTES de recortar.

    Se comparo contra LANCZOS ajustando a la caja: LANCZOS sale borroso y
    dispara los colores de 15 a ~500 (que ya no cabrian en el formato sin
    cuantizar y volver a bandear). NEAREST desde el lienzo de 48 es una
    decimacion 2:1 limpia, no inventa un solo color, y mantiene el aspecto
    de pixel art del resto de la interfaz. Los iconos quedan en 13..22 px,
    que entran de sobra en una fila de lista de 34.
    """
    return im.resize((im.width // 2, im.height // 2), Image.NEAREST)


def indexa(im, caja=None):
    """(w, h, paleta, indices) recortado al contenido (o a `caja`)."""
    px = im.load()
    x0, y0, x1, y1 = caja if caja else recorte(im)
    if x1 < 0:
        raise SystemExit("imagen entera transparente")
    pal, idx = [], []
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            r, g, b, a = px[x, y]
            if a < 8:
                idx.append(TRANSP)
                continue
            c = rgb565(r, g, b)
            if c not in pal:
                pal.append(c)
            idx.append(pal.index(c))
    if len(pal) >= TRANSP:
        raise SystemExit(f"{len(pal)} colores, y 0xFF esta reservado")
    return x1 - x0 + 1, y1 - y0 + 1, pal, idx


def comprimir(idx):
    out, i = [], 0
    while i < len(idx):
        v, n = idx[i], 1
        while i + n < len(idx) and idx[i + n] == v and n < 255:
            n += 1
        out += [n, v]
        i += n
    return out


def tabla(nombre, vals, ancho, hexa):
    ls = [f"static const {'uint16_t' if hexa else 'uint8_t'} {nombre}[] = {{"]
    for i in range(0, len(vals), ancho):
        t = vals[i:i + ancho]
        ls.append("  " + " ".join(f"0x{v:04X}," if hexa else f"{v}," for v in t))
    ls.append("};")
    return "\n".join(ls)


# Cuando esto es una lista, `pieza` apunta ahi lo que emite ADEMAS de
# escribirlo. Sirve para sacar las animaciones por golpe a la tarjeta sin
# duplicar el procesado de imagen: se empaquetan igual y luego se eligen.
CAPTURA = None

# /fx/fx.bin — las animaciones POR GOLPE, fuera de flash.
#
#   char[4] "TPFX" | u16 nGolpes | u16 pad
#   u32 offset[nGolpes]        (0 = ese golpe no tiene animacion propia)
#   --- por golpe, en `offset` ---
#   u8 n, u8 s, u16 pad
#   n x { u8 w, u8 h, u8 palN, u8 pad, u16 rleLen, u16 pad }   (8 B, alineado)
#   todas las paletas seguidas (u16)
#   todos los RLE seguidos (u8)
#
# El reparto cabecera / paletas / RLE no es estetico: la paleta se lee como
# u16 directamente del blob en PSRAM, asi que tiene que caer en direccion
# PAR. Mezclandola con los bytes del RLE no caeria.
FX_SD_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                         "sdcard", "fx")


def escribe_fx_sd(fxSD, n_golpes):
    import struct
    os.makedirs(FX_SD_DIR, exist_ok=True)
    cuerpos = {}
    for mi, (n, esc, piezas) in fxSD.items():
        cab = bytearray()
        pals = bytearray()
        rles = bytearray()
        # `piezas` solo trae los fotogramas que PINTAN; los vacios no estan,
        # asi que se reconstruye la lista entera con huecos w=0.
        usadas = list(piezas)
        k = 0
        for _ in range(n):
            if k < len(usadas):
                _nom, w, h, pal, rle = usadas[k]
                k += 1
            else:
                w = h = 0
                pal, rle = [], []
            cab += struct.pack("<BBBBHH", w, h, len(pal), 0, len(rle), 0)
            for c in pal:
                pals += struct.pack("<H", c)
            rles += bytes(rle)
        cuerpos[mi] = struct.pack("<BBH", n, esc, 0) + bytes(cab) + bytes(pals) + bytes(rles)

    cabecera = 8 + 4 * n_golpes
    offs, blob, cur = [0] * n_golpes, bytearray(), cabecera
    for mi in sorted(cuerpos):
        offs[mi] = cur
        blob += cuerpos[mi]
        cur += len(cuerpos[mi])
    out = bytearray(b"TPFX")
    out += struct.pack("<HH", n_golpes, 0)
    for o in offs:
        out += struct.pack("<I", o)
    out += blob
    with open(os.path.join(FX_SD_DIR, "fx.bin"), "wb") as f:
        f.write(out)
    return len(out)


def pieza(nombre, w, h, pal, idx, partes):
    """Emite una entrada Art y devuelve los bytes que ocupa."""
    rle = comprimir(idx)
    if CAPTURA is not None:
        CAPTURA.append((nombre, w, h, list(pal), list(rle)))
    partes.append(tabla(f"{nombre}_PAL", pal, 8, True))
    partes.append(tabla(f"{nombre}_RLE", rle, 16, False))
    partes.append(f"static const Art {nombre} = {{ {w}, {h}, "
                  f"{nombre}_PAL, {nombre}_RLE, {len(rle)} }};")
    partes.append("")
    return len(rle) + len(pal) * 2


def escreve_tira(nome, magia, primeiro, grupos, por_grupo, entradas):
    """Um arquivo SdTira (ver sdmon.h) em tools/sdcard/por/<nome>.

    `entradas` = {(grupo, indice): (w, h, pal, rle)}. Cada entrada leva o
    SEU grupo e o SEU indice antes do desenho —— dado indexado por linha
    precisa de assinatura, ou a tabela crescendo desenha a vizinha."""
    cab = bytearray(magia.encode()) + bytes([1, primeiro, grupos, por_grupo])
    corpo = bytearray()
    base = len(cab) + 4 * grupos * por_grupo
    offs = []
    for g in range(primeiro, primeiro + grupos):
        for i in range(por_grupo):
            e = entradas.get((g, i))
            if not e:
                offs.append(0)
                continue
            if len(corpo) % 2:
                corpo.append(0)
            offs.append(base + len(corpo))
            w, h, pal, rle = e
            corpo += bytes([g, i, w, h])
            corpo += struct.pack("<HH", len(pal), len(rle))
            corpo += struct.pack("<%dH" % len(pal), *pal)
            corpo += bytes(rle)
    out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "sdcard", "por")
    os.makedirs(out, exist_ok=True)
    dados = bytes(cab) + struct.pack("<%dI" % len(offs), *offs) + bytes(corpo)
    with open(os.path.join(out, nome), "wb") as fh:
        fh.write(dados)
    return len(dados) / 1024


def escreve_insignias(no_cartao):
    """/por/insignias.bin: as insignias com cor, grupo = regiao (2..8)."""
    return escreve_tira("insignias.bin", "TPIN", 2, 7, 8, no_cartao)


def sello_mega(lado):
    """El orbe de la mega a `lado` px, cuantizado a MEGA_COLORES."""
    im = Image.open(os.path.join(ANIL, MEGA_PNG.replace("/", os.sep)))
    im = im.convert("RGBA").crop(MEGA_CAJA).resize((lado, lado), Image.LANCZOS)
    # El alfa se guarda y se vuelve a poner: quantize() trabaja en RGB y si
    # se le da el alfa dentro se lleva el borde por delante.
    alfa = im.split()[3]
    q = im.convert("RGB").quantize(colors=MEGA_COLORES,
                                   method=Image.MEDIANCUT).convert("RGB")
    q.putalpha(alfa)
    return q


def colores_de_tipo():
    """TYPE_COLOR de dex.h, en RGB de 8 bits. Una sola fuente de verdad: si
    manana se retoca el color de un tipo, las insignias cambian con el."""
    with open(os.path.join(ROOT, "dex.h"), encoding="utf-8") as fh:
        txt = fh.read()
    m = re.search(r"TYPE_COLOR\[T_COUNT\]\s*=\s*\{(.*?)\}\s*;", txt, re.S)
    if not m:
        sys.exit("no encuentro TYPE_COLOR en dex.h")
    out = []
    for h in re.findall(r"0x([0-9A-Fa-f]{4})", m.group(1)):
        v = int(h, 16)
        r5, g6, b5 = (v >> 11) & 31, (v >> 5) & 63, v & 31
        out.append(((r5 * 255) // 31, (g6 * 255) // 63, (b5 * 255) // 31))
    return out


def tine(im, rgb):
    """Recolorea una silueta con un color, conservando su sombreado.

    Las insignias del Anil vienen en crema (son mascaras que la ficha de
    entrenador tine). Se multiplica el brillo de cada pixel por el color del
    tipo del gimnasio: sale la forma real de Kanto con el color que le toca,
    sin guardar ocho paletas a mano.
    """
    px = im.load()
    for y in range(im.height):
        for x in range(im.width):
            r, g, b, a = px[x, y]
            if a < 8:
                continue
            luma = (r * 30 + g * 59 + b * 11) // 100
            px[x, y] = (rgb[0] * luma // 255, rgb[1] * luma // 255,
                        rgb[2] * luma // 255, a)
    return im


def nombres_de_items():
    """Los nombres de pantalla de items.h, en el orden de la tabla."""
    with open(os.path.join(ROOT, "items.h"), encoding="utf-8") as fh:
        txt = fh.read()
    return re.findall(r'\{\s*"([^"]+)"\s*,\s*BP_', txt)


# Objetos que no existen en el Anil (los inventamos) y a los que se les
# presta el icono de otro. Clave: el id que se buscaria; valor: el que se
# usa de verdad.
#
#   RAYQUAZITE -> SCEPTILITE   verde y de una mega de tipo DRAGON, que es
#                              lo mas cerca que hay de un Rayquaza.
PRESTADOS = {
    "RAYQUAZITE": "SCEPTILITE",
}


def claves_pbs_de_items():
    """El id INTERNO del PBS de cada objeto, cuando se puede saber.

    Los iconos del Anil se llaman por el id del PBS (ARCANINITE.png), no
    por el nombre de pantalla (Arcanite), y por eso 43 megapiedras salian
    "sin icono" teniendolo delante. De las piedras el id esta en items.h,
    en el `arg`: IT_MEGA con MEGA_ARCANINITE. Del resto no hay de donde
    sacarlo aqui, y se devuelve cadena vacia —— esos siguen buscandose
    por el nombre de pantalla, que para ellos funciona.
    """
    with open(os.path.join(ROOT, "items.h"), encoding="utf-8") as fh:
        txt = fh.read()
    out = []
    for m in re.finditer(r'\{\s*"([^"]+)"\s*,\s*BP_[A-Z]+\s*,\s*'
                         r'([A-Z_0-9]+)\s*,\s*([A-Z_0-9]+)\s*'
                         # campos de mais (o IU_* que veio depois): sem
                         # isto a linha nao casava e a piedra ficava sem
                         # id —— a Arcanite saia sem icone com o PNG ali.
                         r'(?:,\s*[A-Z_0-9]+\s*)*\},?'
                         r'(?:\s*//\s*pbs:([A-Z_0-9]+))?', txt):
        efecto, arg, pbs = m.group(2), m.group(3), m.group(4)
        if pbs:
            out.append(pbs)
        elif efecto == "IT_MEGA" and arg.startswith("MEGA_"):
            out.append(arg[5:])
        else:
            out.append("")
    return out


# --------------------------------------------------------------- marcas
# Las dos MARCAS del aviso de la hierba, sacadas de las animaciones del
# Anil (celdas de 192x192, 5 por fila, que es el formato del RPG Maker).
#
#   hierba       Animations/grass.png, fila 0 y 1: las hojitas que saltan
#                al andar por la hierba alta. Es el gesto que cualquiera
#                reconoce como "ahi se mueve algo".
#   exclamacion  Animations/029-Emotion01.png, fila 1: el bocadillo con
#                el "!" creciendo, el de toda la vida cuando un
#                entrenador te ve.
#
# Antes las dos eran un "!" escrito con la fuente del sistema. Funcionaba
# y se entendia, pero no parecia de Pokemon, que era justo la queja.
MARCA_CELDA = 192

# (nombre, fichero, QUE celdas, alto que se quiere en pantalla)
#
# Las celdas van ESCRITAS una a una y no como "desde la N coge M", y hay
# motivo: en grass.png se alternan hojas sueltas pequenas con medias
# lunas grandes, y la hoja de animacion las compone a pares. Puestas en
# fila como fotogramas, el dibujo parpadeaba entre grande y pequeno en
# vez de mecerse. Las medias lunas (1, 5, 6, 7) SI se mecen: son la
# misma forma en angulos distintos.
# OS BALOES DA REACAO. Tudo sai do Anil; nada foi desenhado aqui.
#
# (nome, caminho dentro de Graphics, lado da celula, celulas, altura
#  que queremos na tela)
#
# As celulas vao escritas uma a uma com o que SAO ao lado, porque o
# indice sozinho nao diz nada e um erro aqui poe a cara errada no
# estado errado sem avisar. Conferidas olhando as folhas.
GLOBOS = [
    # O Z do sono. A folha tem um Z so; quem repete em tres tamanhos
    # subindo e o desenho, nao o arte.
    ("ART_ZZZ",    "Animations/PRAS- Sleep Talk.png",        192, [0], 34),
    # A cruz vermelha de raiva —— o "jogo da velha" que ele descreveu.
    # Folha de 48x48 inteira, uma celula so.
    ("ART_RAIVA",  "Battle animations/safari_anger.png",      48, [0], 28),
    # A cara de boca aberta, pra fome. Celula 4 do Follower_Emote2.
    #
    # Aqui eu tinha posto a celula 0 do Follower_Emote1 achando que era
    # uma gota de suor. NAO e: medindo as celulas, a 0 da 28x10 e a 4 da
    # 29x11 —— sao os quadros ESMAGADOS do balao nascendo, que essas
    # folhas animam (esmagado -> crescendo -> cheio, os cheios sao a 3 e
    # a 7). No Emote2 e no Emote3 todas as celulas dao 30x32, ou seja
    # todas sao glifos inteiros.
    #
    # E a celula 4 do Emote2 e uma cara FELIZ de boca aberta —— o dono
    # viu: "a reclamacao da fome parece estar com uma onomatopeia
    # feliz". Agora a 6 do Emote3: cara triste com a boca azul, babando.
    ("ART_FOME",   "Animations/Follower_Emote3.png",         192, [6], 32),
    # A nuvem roxa de sujeira, pro bicho sujo. Celula 0 do Emote3.
    ("ART_SUJEIRA","Animations/Follower_Emote3.png",         192, [0], 32),
    # A nota musical verde, pra comemorar. Celula 0 do Follower_Emote2.
    ("ART_NOTA",   "Animations/Follower_Emote2.png",         192, [0], 32),
    # As reticencias, pro bicho sem forcas. Celula 4 do Follower_Emote3.
    ("ART_PONTOS", "Animations/Follower_Emote3.png",         192, [4], 32),
    # O "!" cheio, pra quando ele se decide e entra no matinho. A
    # celula 7 e a do glifo inteiro: as 0 e 4 dessa folha sao os
    # quadros esmagados do balao nascendo (28x10, 29x11).
    ("ART_PRONTO", "Animations/Follower_Emote1.png",         192, [7], 34),
    # O coracao, para quando ele acabou de comer ou de ganhar carinho.
    # Celula 8 do Follower_Emote2 (todas as dez dessa folha sao glifos
    # inteiros, 30x32 — conferido medindo).
    ("ART_CORACAO","Animations/Follower_Emote2.png",         192, [8], 32),
]


MARCAS = [
    ("ART_EXCLAMA", "029-Emotion01.png", [5, 6, 7], 46),
]


# La HIERBA ALTA, la de verdad: el tile del tileset por el que se anda y
# donde salen los bichos. No es una animacion, es UN tile -- en los
# juegos la hierba esta quieta y lo que se mueve es el efecto de
# farfallar cuando algo pasa. Aqui se mece en el dibujado (un pixel a un
# lado y a otro), que es animar arte de verdad y no inventarla.
#
# Antes aqui habia cuatro medias lunas de Animations/grass.png. Son el
# efecto de HOJAS saltando, no hierba, y el dueno lo dijo sin rodeos.
# Fila 0, columna 4 del tileset: el tile de HIERBA ALTA, el macizo de
# hojas por el que se anda y donde salen los bichos. (La fila 1 es el
# mismo en verde oscuro, por si algun dia hace falta de noche.)
#
# Antes aqui estuvo el tile de la fila 3, que son briznas sueltas de
# adorno, y antes de eso cuatro medias lunas de Animations/grass.png,
# que son HOJAS SALTANDO. Ninguna de las dos es la hierba del juego.
HIERBA_TILE = ('Exterior1.png', (128, 0, 160, 32), 46)


# Los iconos de la interfaz, sacados del Anil en vez de dibujados a mano
# con circulos y rectangulos en el .ino. Los de antes eran eso, y se notaba.
#
#   (nombre, carpeta, fichero, ancho destino, tinte RGB o None, colores)
#
# BLACKSLUDGE es el unico monton con remolino que hay en todo el juego, que
# es exactamente la silueta de una caca; va TENIDO de marron (tine() lo usa
# ya para las insignias: multiplica el brillo por el color y conserva el
# sombreado). menuQuit es una bandera blanca, que para "huir" se entiende
# sola, y menuPokemon es la pokebola del menu.
# (nombre, carpeta, fichero, ancho final, tinte, colores[, recorte])
#
# El RECORTE es opcional y va en pixeles del fichero original. Hace falta
# para los que no son un icono suelto: enfermera.png es una hoja de
# personaje de RPG Maker (4x4 cuadros de 60x64) y sin acotar antes, el
# getbbox de abajo encuadraria las dieciseis poses de golpe.
ICONOS = [
    ('ART_POOP',  'Items',      'BLACKSLUDGE.png', 30, (0x86, 0x52, 0x26), 10),
    ('ART_BAG',   'UI/Bag',     'bag_1.png',       34, None,               14),
    ('ART_SWAP',  'Icons',      'menuPokemon.png', 32, None,               12),
    ('ART_FLEE',  'Icons',      'menuQuit.png',    32, None,               10),
    # El centro Pokemon de este cacharro es ELLA. No hay ningun grafico de
    # centro Pokemon en el Anil (se busco: en Icons solo estan los del menu),
    # pero la enfermera se reconoce al instante y ademas es lo que se toca.
    # Cuadro de abajo-izquierda de la hoja: de frente y quieta.
    ('ART_NURSE', 'Characters', 'enfermera.png',   40, None,               16,
     (0, 0, 60, 64)),
    # El Profesor, que es quien saca el cuestionario. Mismo recorte que la
    # enfermera (de frente y quieto), solo que su hoja es de 64 y no de 60.
    # Sale del Anil y NO se dibuja por codigo: un personaje hecho a base de
    # circulos ya se intento una vez y quedo feo.
    ('ART_OAKMINI', 'Characters', 'oak.png',       40, None,               16,
     (0, 0, 64, 64)),
    # Y el mapa del pueblo, que es literalmente el objeto que hace esto en
    # los juegos.
    ('ART_MAP',   'Items',      'TOWNMAP.png',     34, None,               14),
]


def iconos(partes, resumen):
    total = 0
    for fila in ICONOS:
        nom, carp, fich, ancho, tinte, cols = fila[:6]
        recorte = fila[6] if len(fila) > 6 else None
        ruta = os.path.join(ANIL, carp.replace('/', os.sep), fich)
        if not os.path.exists(ruta):
            resumen.append('  !! falta %s (%s)' % (fich, nom))
            continue
        im = Image.open(ruta).convert('RGBA')
        if recorte:
            im = im.crop(recorte)
        b = im.getbbox()
        if not b:
            resumen.append('  !! %s esta vacio' % fich)
            continue
        rec = im.crop(b)
        if tinte:
            rec = tine(rec, tinte)
        esc = ancho / float(rec.width)
        nw = ancho
        nh = max(1, int(rec.height * esc + 0.5))
        rec = rec.resize((nw, nh), Image.LANCZOS)
        # el alfa se vuelve binario: el blit del reloj no mezcla
        px = rec.load()
        for y in range(nh):
            for x in range(nw):
                r, g, bl, a = px[x, y]
                px[x, y] = (r, g, bl, 0) if a < 96 else (r, g, bl, 255)
        rec = reduce_colores(rec, cols)
        w, h, pal, idx = indexa(rec, (0, 0, nw - 1, nh - 1))
        total += pieza(nom, w, h, pal, idx, partes)
        partes.append('')
        resumen.append('  %-10s %-20s %dx%d' % (nom, fich, w, h))
    return total


def hierba(partes, resumen):
    fich, caja, ancho = HIERBA_TILE
    im = Image.open(os.path.join(ANIL, 'Tilesets', fich)).convert('RGBA')
    rec = im.crop(caja)
    b = rec.getbbox()
    if not b:
        raise SystemExit('el tile de hierba esta vacio: %s %s' % (fich, caja))
    rec = rec.crop(b)
    esc = ancho / float(rec.width)
    nw = ancho
    nh = max(1, int(rec.height * esc + 0.5))
    rec = rec.resize((nw, nh), Image.LANCZOS)
    px = rec.load()
    for y in range(nh):
        for x in range(nw):
            r, g, bl, a = px[x, y]
            px[x, y] = (r, g, bl, 0) if a < 96 else (r, g, bl, 255)
    rec = reduce_colores(rec, 10)
    w, h, pal, idx = indexa(rec, (0, 0, nw - 1, nh - 1))
    n = pieza('ART_HIERBA', w, h, pal, idx, partes)
    partes.append('')
    resumen.append('  ART_HIERBA   tile del tileset  %dx%d' % (w, h))
    return n


def marcas(partes, resumen):
    """Emite las tiras de fotogramas de las dos marcas. Devuelve bytes."""
    total = 0
    for nombre, rel, lado, cuales, alto in GLOBOS:
        hoja = Image.open(os.path.join(ANIL, *rel.split("/"))).convert("RGBA")
        cols = max(1, hoja.width // lado)
        celdas = []
        for k in cuales:
            cx, cy = (k % cols) * lado, (k // cols) * lado
            cel = hoja.crop((cx, cy, cx + lado, cy + lado))
            if not cel.getbbox():
                raise SystemExit("%s: a celula %d esta vazia" % (rel, k))
            celdas.append(cel)
        caja = None
        for c in celdas:
            b = c.getbbox()
            caja = b if caja is None else (min(caja[0], b[0]), min(caja[1], b[1]),
                                           max(caja[2], b[2]), max(caja[3], b[3]))
        rec = [c.crop(caja) for c in celdas]
        an = caja[2] - caja[0]
        al = caja[3] - caja[1]
        if al > alto:
            an = max(1, an * alto // al)
            rec = [c.resize((an, alto), Image.LANCZOS) for c in rec]
        w, h, pal, idx = indexa(rec[0])
        pieza(nombre, w, h, pal, idx, partes)
        resumen.append("  %-11s %-38s %dx%d" % (nombre, rel.split("/")[-1], w, h))

    for nombre, fich, cuales, alto in MARCAS:
        hoja = Image.open(os.path.join(ANIL, "Animations", fich)).convert("RGBA")
        cols = hoja.width // MARCA_CELDA
        celdas = []
        for k in cuales:
            cx, cy = (k % cols) * MARCA_CELDA, (k // cols) * MARCA_CELDA
            cel = hoja.crop((cx, cy, cx + MARCA_CELDA, cy + MARCA_CELDA))
            if not cel.getbbox():
                raise SystemExit("%s: la celda %d esta vacia" % (fich, k))
            celdas.append(cel)

        # UNA caja para todos los fotogramas, no una por fotograma: si
        # cada uno se recortara por su cuenta, el dibujo saltaria de sitio
        # al animar. Es el mismo truco que las grietas del huevo.
        caja = None
        for c in celdas:
            b = c.getbbox()
            if not b:
                continue
            caja = b if caja is None else (min(caja[0], b[0]), min(caja[1], b[1]),
                                           max(caja[2], b[2]), max(caja[3], b[3]))
        if caja is None:
            raise SystemExit(f"{fich}: los fotogramas {desde}..{desde+cuantas-1} "
                             "estan vacios")

        ancho = caja[2] - caja[0]
        alto0 = caja[3] - caja[1]
        esc = alto / float(alto0)
        nw, nh = max(1, int(ancho * esc + 0.5)), max(1, int(alto0 * esc + 0.5))

        refs = []
        for f, c in enumerate(celdas):
            rec = c.crop(caja).resize((nw, nh), Image.LANCZOS)
            # LANCZOS deja un halo de pixeles casi transparentes en el
            # borde; sin limpiarlo, indexa() los toma por color y la
            # paleta se llena de basura gris.
            px = rec.load()
            for y in range(nh):
                for x in range(nw):
                    r, g, b, a = px[x, y]
                    px[x, y] = (r, g, b, 0) if a < 96 else (r, g, b, 255)
            rec = reduce_colores(rec, 14)
            # El lienzo ENTERO, no el contenido: indexa() recorta por su
            # cuenta si no se le da caja, y entonces cada fotograma sale
            # con su propio tamano y la animacion salta de sitio. Aqui
            # ya estan todos recortados a la MISMA caja comun.
            w, h, pal, idx = indexa(rec, (0, 0, nw - 1, nh - 1))   # caja INCLUSIVA, como recorte()
            n = f"{nombre}{f}"
            total += pieza(n, w, h, pal, idx, partes)
            refs.append("&" + n)

        partes.append(f"#define {nombre}_N {len(refs)}")
        partes.append(f"static const Art *const {nombre}[{nombre}_N] = {{")
        partes.append("  " + " ".join(r + "," for r in refs))
        partes.append("};")
        partes.append("")
        resumen.append(f"  {nombre:12s} {len(refs)} frames  {nw}x{nh}")
    return total


def main():
    if not os.path.isdir(ANIL):
        sys.exit(f"no encuentro los graficos del Anil en {ANIL}")

    partes = [
        "// GENERADO por tools/gen_art.py - no editar a mano.",
        "// Pixel art en flash sacado del Anil: retratos de la intro, el huevo",
        "// con sus grietas, y los iconos de los objetos.",
        "#pragma once",
        "#include <stdint.h>",
        "",
        "// `Art` y `FxAnim` viven en art_def.h, no aqui: sdmon.cpp tambien",
        "// los necesita (carga las animaciones de golpe de la tarjeta) y no",
        "// puede incluir este fichero sin duplicar el arte entero en flash.",
        '#include "art_def.h"',
        "",
    ]
    total = 0
    resumen = []

    # --- retratos de la intro ---
    for nombre, ruta in RETRATOS:
        w, h, pal, idx = indexa(Image.open(ruta).convert("RGBA"))
        total += pieza(nombre, w, h, pal, idx, partes)
        resumen.append(f"  {nombre:10s} {w:3d}x{h:3d} {len(pal):3d} col")

    # --- el huevo y sus grietas ---
    # Las grietas se recortan con la MISMA caja que el huevo, no con la suya:
    # asi los frames se superponen encima sin tener que guardar desplazamientos.
    huevo = Image.open(os.path.join(EGGS, "000.png")).convert("RGBA")
    caja = recorte(huevo)
    w, h, pal, idx = indexa(huevo, caja)
    total += pieza("ART_EGG", w, h, pal, idx, partes)
    resumen.append(f"  ART_EGG    {w:3d}x{h:3d} {len(pal):3d} col")

    tira = Image.open(os.path.join(EGGS, "000_cracks.png")).convert("RGBA")
    paso = tira.width // CRACK_FRAMES
    nombres_cr = []
    for f in range(CRACK_FRAMES):
        rec = tira.crop((f * paso, 0, (f + 1) * paso, tira.height))
        cw, ch, cpal, cidx = indexa(rec, caja)
        n = f"ART_CRACK{f}"
        total += pieza(n, cw, ch, cpal, cidx, partes)
        nombres_cr.append(n)
    partes.append("// Las grietas comparten recorte con el huevo: se pintan")
    partes.append("// encima en la misma posicion, sin desplazamientos.")
    partes.append(f"#define ART_CRACK_FRAMES {CRACK_FRAMES}")
    partes.append("static const Art *const ART_CRACKS[ART_CRACK_FRAMES] = { " +
                  ", ".join("&" + n for n in nombres_cr) + " };")
    partes.append("")
    resumen.append(f"  ART_CRACK0..{CRACK_FRAMES - 1} ({paso}x{tira.height} cada uno)")

    # --- iconos de objeto ---
    disponibles = {f[:-4].upper(): f for f in os.listdir(ITEMS)
                   if f.lower().endswith(".png")}
    nombres = nombres_de_items()
    claves_pbs = claves_pbs_de_items()
    refs, faltan, bytes_items = [], [], 0
    for i, n in enumerate(nombres):
        # Primero por el id INTERNO del PBS, que es como se llaman los
        # ficheros del Anil (ARCANINITE.png), y solo despues por el
        # nombre de pantalla (Arcanite). Al reves, 43 megapiedras se
        # daban por "sin icono" con el dibujo delante.
        pbs = claves_pbs[i] if i < len(claves_pbs) else ""
        clave = re.sub(r"[^A-Z0-9]", "", (pbs or n).upper())
        if clave not in disponibles:
            clave = re.sub(r"[^A-Z0-9]", "", n.upper())
        if clave not in disponibles and clave in PRESTADOS:
            # No es que falte el dibujo: es que el OBJETO no existe en el
            # Anil y lo pusimos nosotros (ver MEGAS_SIN_PIEDRA en
            # gen_items.py). No hay RAYQUAZITE.png que buscar, asi que se
            # presta el de la piedra mas parecida y queda dicho aqui.
            clave = PRESTADOS[clave]
        if clave == "POKEDEX" and clave not in disponibles:
            # Nao e objeto no Anil: o desenho e o do menu.
            disponibles[clave] = os.path.join("..", "Icons", "menuPokedex.png")
        if n == "-" or clave not in disponibles:
            if n != "-":
                faltan.append(n)
            refs.append("nullptr")
            continue
        ruta = os.path.join(ITEMS, disponibles[clave])
        w, h, pal, idx = indexa(mitad(Image.open(ruta).convert("RGBA")))
        b = pieza(f"ART_IT{i}", w, h, pal, idx, partes)
        total += b
        bytes_items += b
        refs.append(f"&ART_IT{i}")
    partes.append("// Indexado por el mismo indice que ITEM_TBL. nullptr = sin icono.")
    partes.append(f"static const Art *const ART_ITEM[{len(refs)}] = {{")
    for i in range(0, len(refs), 6):
        partes.append("  " + " ".join(r + "," for r in refs[i:i + 6]))
    partes.append("};")
    partes.append("")
    resumen.append(f"  {len(nombres) - 1 - len(faltan)}/{len(nombres) - 1} iconos de objeto"
                   f" ({bytes_items} B)")
    if faltan:
        resumen.append("  SIN ICONO: " + ", ".join(faltan))

    # --- retratos de lideres, Alto Mando y campeon ---
    # Todos a RETRATO_PX de alto: enteros, los del Anil ocupan 160 px y en
    # la pantalla del gimnasio conviven con el nombre, el bicho y el boton.
    # Una fila por region, en el orden de REGIONS (gyms.h). Como llegar a
    # los 80 lo decide a_retrato() mirando el original.
    filas, nGym = [], len(LIDERES)
    for region, fuentes in REGION_RETRATOS:
        fila = []
        for ruta, del_anil in fuentes:
            if not os.path.exists(ruta):
                fila = []
                break
            im = a_retrato(Image.open(ruta).convert("RGBA"), del_anil)
            # Los overworld de XY son render, no pixel art: traen miles de
            # colores y el formato indexado solo aguanta 254.
            if len(set(im.getdata())) > 254:
                im = reduce_colores(im, 24)
            w, h, pal, idx = indexa(im)
            c = f"ART_L{len(filas)}_{len(fila)}"
            total += pieza(c, w, h, pal, idx, partes)
            fila.append("&" + c)
        if fila and len(fila) != nGym:
            raise SystemExit(f"{region} trae {len(fila)} retratos y la escalera "
                             f"son {nGym}: corre tools/fetch_leaders.py")
        if not fila:
            # Sin los ficheros se repite la primera region y la pantalla al
            # menos no se cae; el aviso sale aqui, no jugando.
            resumen.append(f"  !! sin retratos de {region}: corre "
                           f"tools/fetch_leaders.py")
            fila = list(filas[0]) if filas else []
        filas.append(fila)

    partes.append("// Un retrato por desafio. Las FILAS van en el orden de")
    partes.append("// REGIONS y las columnas en el de los gimnasios de cada una")
    partes.append("// (gyms.h). Los que no son del Anil: ver fetch_leaders.py.")
    partes.append(f"static const Art *const ART_LEADER[{len(filas)}][{nGym}] = {{")
    for fila in filas:
        partes.append("  {")
        for i in range(0, len(fila), 5):
            partes.append("    " + " ".join(r + "," for r in fila[i:i + 5]))
        partes.append("  },")
    partes.append("};")
    partes.append("")
    resumen.append(f"  {nGym} retratos de lider x{len(filas)} regiones")

    # --- entrenadores de a pie ---
    trefs = []
    for n in ENTRENADORES:
        ruta = os.path.join(TRAINERS, n)
        w, h, pal, idx = indexa(mitad(Image.open(ruta).convert("RGBA")))
        c = f"ART_T{len(trefs)}"
        total += pieza(c, w, h, pal, idx, partes)
        trefs.append("&" + c)
    partes.append("// Mismo orden que TRAINER_TBL en trainers.h.")
    partes.append(f"static const Art *const ART_TRAINER[{len(trefs)}] = {{")
    for i in range(0, len(trefs), 4):
        partes.append("  " + " ".join(r + "," for r in trefs[i:i + 4]))
    partes.append("};")
    partes.append("")
    resumen.append(f"  {len(trefs)} entrenadores")

    # --- iconos de la pantalla principal ---
    for nombre, rel in UI_ICONS:
        ruta = os.path.join(ANIL, rel.replace("/", os.sep))
        w, h, pal, idx = indexa(Image.open(ruta).convert("RGBA"))
        total += pieza(nombre, w, h, pal, idx, partes)
    resumen.append(f"  {len(UI_ICONS)} iconos de pantalla")

    # --- el sello de la megaevolucion ---
    for nombre, lado in MEGA_LADOS:
        w, h, pal, idx = indexa(sello_mega(lado))
        total += pieza(nombre, w, h, pal, idx, partes)
    resumen.append("  sello de la mega " +
                   " ".join(f"{l}x{l}" for _, l in MEGA_LADOS))

    # --- estrella de shiny ---
    # Animations/Shiny.png son 5 destellos de 192x192 en fila; el 1 es la
    # estrella limpia de cuatro puntas, que es la marca de siempre. Se encoge
    # a la mitad con NEAREST por lo mismo que los iconos de objeto (LANCZOS
    # emborrona y multiplica los colores). Queda en 20x28.
    #
    # Hacen falta DOS tamanos. La grande (20x28) va junto a un nombre a
    # tamano 3; en la chapa de PS del combate, que mide 30 por dentro y lleva
    # la barra a media altura, se sale por arriba y cruza la barra (visto en
    # captura). La pequena es otra mitad.
    sh = Image.open(os.path.join(ANIL, SHINY_PNG.replace("/", os.sep))).convert("RGBA")
    cel = sh.width // 5
    est = mitad(sh.crop((SHINY_FRAME * cel, 0, (SHINY_FRAME + 1) * cel, cel)))
    w, h, pal, idx = indexa(est)
    total += pieza("ART_SHINY", w, h, pal, idx, partes)
    w2, h2, pal2, idx2 = indexa(mitad(est))
    total += pieza("ART_SHINY_S", w2, h2, pal2, idx2, partes)
    resumen.append(f"  estrella de shiny {w}x{h} y {w2}x{h2}")

    # Y los CINCO a tamano completo, para el destello de entrada al combate
    # (el de los juegos). Aqui si interesan grandes: van encima del bicho,
    # que se dibuja a escala 5.
    srefs, sdim = [], []
    for i in range(5):
        cw, ch, cpal, cidx = indexa(sh.crop((i * cel, 0, (i + 1) * cel, cel)))
        c = f"ART_SPARK{i}"
        total += pieza(c, cw, ch, cpal, cidx, partes)
        srefs.append("&" + c)
        sdim.append(f"{cw}x{ch}")
    partes.append("// Los cinco fotogramas del destello, en orden.")
    partes.append(f"#define ART_SPARK_FRAMES {len(srefs)}")
    partes.append(f"static const Art *const ART_SPARK[{len(srefs)}] = {{")
    partes.append("  " + " ".join(r + "," for r in srefs))
    partes.append("};")
    partes.append("")
    resumen.append("  destello de entrada " + " ".join(sdim))

    # --- las bolas de la captura, en el orden de BALL_ALL (items.h) ---
    filas_bola, orefs = [], []
    for nombre in BALL_NAMES:
        ruta = os.path.join(ANIL, BALL_PNG.replace("/", os.sep) % nombre)
        if not os.path.exists(ruta):
            raise SystemExit(f"falta {ruta}")
        b = Image.open(ruta).convert("RGBA")
        # La bola NO esta alineada a la rejilla: la mitad de arriba cae en las
        # filas 16..31 de la primera fila de celdas y la de abajo en las 0..15
        # de la segunda. Recortando la celda tal cual salia media bola (32x16).
        #
        # Y la fila 0 son OCHO celdas: la misma bola girando. Antes se emitia
        # solo la celda 0, asi que la bola llegaba volando sin girar.
        giro = []
        for k in range(BALL_SPIN):
            cel = b.crop((k * BALL_CELL, BALL_OFF,
                          (k + 1) * BALL_CELL, BALL_OFF + BALL_CELL))
            w, h, pal, idx = indexa(cel)
            c = f"ART_BALL_{nombre}_{k}"
            total += pieza(c, w, h, pal, idx, partes)
            giro.append("&" + c)
        partes.append(f"static const Art *const ART_BALL_{nombre}[] = {{")
        partes.append("  " + " ".join(r + "," for r in giro))
        partes.append("};")
        partes.append("")
        filas_bola.append(f"ART_BALL_{nombre}")

        # La abierta, de una pieza: la tapa y la base ya vienen separadas.
        rabierta = os.path.join(ANIL, BALL_OPEN_PNG.replace("/", os.sep) % nombre)
        if not os.path.exists(rabierta):
            raise SystemExit(f"falta {rabierta}")
        o = Image.open(rabierta).convert("RGBA")
        w, h, pal, idx = indexa(o.crop((0, 0, BALL_CELL, BALL_OPEN_H)))
        c = f"ART_BALLOPEN_{nombre}"
        total += pieza(c, w, h, pal, idx, partes)
        orefs.append("&" + c)

    partes.append("// Todas las bolas, en el MISMO orden que BALL_ALL de items.h.")
    partes.append("// Cada una son sus OCHO fotogramas de giro (fila 0 de la hoja).")
    partes.append(f"static const Art *const *const ART_BALL[{len(filas_bola)}] = {{")
    partes.append("  " + " ".join(n + "," for n in filas_bola))
    partes.append("};")
    partes.append(f"#define BALL_SPIN_FRAMES {BALL_SPIN}")
    partes.append("// Y abiertas, para el momento en que se traga al bicho.")
    partes.append(f"static const Art *const ART_BALL_OPEN[{len(orefs)}] = {{")
    partes.append("  " + " ".join(r + "," for r in orefs))
    partes.append("};")
    partes.append("")
    resumen.append(f"  {len(filas_bola)} bolas x{BALL_SPIN} de giro + {len(orefs)} abiertas")

    # --- efectos de golpe ---
    # A la mitad (192 -> 96): a tamano completo un solo fotograma ocupaba mas
    # que todos los iconos de objeto juntos, y 96 px ya cubre al bicho, que se
    # dibuja como mucho a 170 de alto.
    estado = {"total": 0, "vacios": 0}

    def empaqueta_fx(hojas, pref, partes, reducir=False):
        """Devuelve [(lista, nFotogramas, escala)] para una tanda de hojas."""
        salida = []
        for i, (hoja, desde, cuantos) in enumerate(hojas):
            if not hoja:
                salida.append(("nullptr", 0, 1))
                continue
            ruta = os.path.join(ANIL, "Animations", hoja)
            if not os.path.exists(ruta):
                raise SystemExit(f"falta la hoja de efecto {hoja}")
            im = Image.open(ruta).convert("RGBA")
            lado = lado_celda(im)
            cols = im.width // lado
            celdas = []
            for k in range(desde, desde + cuantos):
                c, r = k % cols, k // cols
                cel = im.crop((c * lado, r * lado, (c + 1) * lado, (r + 1) * lado))
                # las celdas grandes se encogen mas, para que todas acaben
                # cerca de los 96 px con los que se diseno el dibujo
                while cel.width > FX_CELL:
                    cel = mitad(cel)
                cel = mitad(cel)
                celdas.append(reduce_colores(cel) if reducir else cel)
            # el efecto delante y las variantes de color pegadas detras
            celdas = quita_paletas(celdas)
            celdas = quita_vacios(celdas)
            # Caja UNION de todos los fotogramas, no una por fotograma.
            # Recortando cada uno a su contenido se pierde DONDE estaba dentro
            # de la celda y la animacion deja de moverse: un remolino que sube
            # quedaria centrado siempre y pareceria latir en el sitio. El hueco
            # transparente de mas es justo lo que el RLE comprime bien.
            x0, y0, x1, y1 = FX_CELL, FX_CELL, -1, -1
            for cel in celdas:
                a, b, c2, d2 = recorte(cel)
                if c2 < 0:
                    continue
                x0, y0, x1, y1 = min(x0, a), min(y0, b), max(x1, c2), max(y1, d2)
            if x1 < 0:
                salida.append(("nullptr", 0, 1))
                continue
            caja = (x0, y0, x1, y1)
            refs = []
            for cel in celdas:
                if recorte(cel)[2] < 0:
                    estado["vacios"] += 1
                    refs.append("nullptr")   # vacio, pero cuenta para el ritmo
                    continue
                w, h, pal, idx = indexa(cel, caja)
                nom = f"ART_{pref}{i}_{len(refs)}"
                estado["total"] += pieza(nom, w, h, pal, idx, partes)
                refs.append("&" + nom)
            # Una escala por ANIMACION (no por fotograma, que bailaria): los
            # dibujos dentro de la celda son pequenos y a escala 1 se pierden
            # encima de un bicho de ~100 px de alto.
            esc = max(1, min(3, round(96 / (y1 - y0 + 1))))
            lista = f"ART_{pref}{i}"
            partes.append(f"static const Art *const {lista}[{len(refs)}] = {{")
            partes.append("  " + " ".join(r + "," for r in refs))
            partes.append("};")
            partes.append("")
            salida.append((lista, len(refs), esc))
        return salida

    def empaqueta_fx_imgs(imgs, pref, partes, reducir=False):
        """Como empaqueta_fx, pero sobre fotogramas YA montados.

        fxanim los devuelve compuestos (varias celdas por fotograma, con su
        zoom, giro, espejo y opacidad), recortados TODOS a la misma caja y
        ya escalados, asi que aqui no hay nada que recortar ni adivinar:
        solo indexar, comprimir y emitir.
        """
        refs = []
        for im in imgs:
            if im is None or not im.getbbox():
                estado["vacios"] += 1
                refs.append("nullptr")   # vacio, pero cuenta para el ritmo
                continue
            uso = reduce_colores(im) if reducir else im
            # caja ENTERA: ya viene recortada en comun por fxanim, y
            # recortarla otra vez aqui, fotograma a fotograma, es justo lo
            # que mataba el movimiento.
            w, h, pal, idx = indexa(uso, (0, 0, uso.width - 1, uso.height - 1))
            nom = f"ART_{pref}{len(refs)}"
            estado["total"] += pieza(nom, w, h, pal, idx, partes)
            refs.append("&" + nom)
        if not any(r != "nullptr" for r in refs):
            return None
        # Una escala por ANIMACION. fxanim ya las deja a tamano de pantalla,
        # asi que casi todas van a 1; solo se dobla lo que salio muy chico
        # (un arañazo de 58 px encima de un bicho de 100 no se ve).
        alto = max((im.height for im in imgs if im is not None), default=1)
        ancho = max((im.width for im in imgs if im is not None), default=1)
        esc = 2 if max(ancho, alto) <= 70 else 1
        lista = f"ART_{pref}L"
        partes.append(f"static const Art *const {lista}[{len(refs)}] = {{")
        partes.append("  " + " ".join(r + "," for r in refs))
        partes.append("};")
        partes.append("")
        return (lista, len(refs), esc)

    frefs = empaqueta_fx(FX_SHEETS, "FX", partes)
    partes.append("// Efecto de golpe por TIPO (indice = nuestro T_* de dex.h).")
    partes.append("// Las hojas salen de Graphics/Animations del Anil; ver")
    partes.append("// FX_SHEETS en tools/gen_art.py para cual es cada una.")
    # `FxAnim` tambien esta en art_def.h (lo necesita sdmon.cpp)
    partes.append(f"static const FxAnim FX_OF_TYPE[{len(frefs)}] = {{")
    for i, (lista, n, esc) in enumerate(frefs):
        partes.append(f"  {{ {lista}, {n}, {esc} }},   // {FX_TIPOS[i]}")
    partes.append("};")
    partes.append("")

    srefs = empaqueta_fx(FX_SUPPORT, "FS", partes)
    partes.append("// Efecto de los golpes que NO pegan, por lo que HACEN. Un")
    partes.append("// Tail Whip es de tipo normal, y con el efecto del tipo se")
    partes.append("// veia el tajo de Vice Grip: bajar la defensa parecia un")
    partes.append("// aranazo. El orden es el de FXS_* en battle_ui.ino.")
    partes.append(f"#define FX_SUP_COUNT {len(srefs)}")
    partes.append(f"static const FxAnim FX_SUP[{len(srefs)}] = {{")
    for i, (lista, n, esc) in enumerate(srefs):
        partes.append(f"  {{ {lista}, {n}, {esc} }},   // {FX_SUP_NOM[i]}")
    partes.append("};")
    partes.append("")

    # --- el efecto PROPIO de cada golpe, TAL CUAL LO ANIMA EL ANIL ---
    #
    # Antes esto cogia la hoja que le tocaba al golpe y se quedaba con "las
    # primeras N celdas no vacias". No es la animacion: una hoja como
    # `PRAS- Slash.png` son TRECE animaciones pegadas en una rejilla, y
    # algunas traen al final el cartel de "Ripped and Composed By..." del
    # que las extrajo (Scratch salia con tres fotogramas de ese cartel).
    #
    # Ahora se lee Data/PkmnAnimations.rxdata, que dice que celda va en
    # cada fotograma, donde, con que zoom, giro, espejo y opacidad. Se
    # acabaron el tamano de celda adivinado, el recorte de celdas vacias,
    # el detector de tiras de paletas y los ~47 mapeos a mano.
    pares, anims, n_golpes = golpes_con_animacion()
    hojas_fx = fxanim.Hojas(os.path.join(ANIL, "Animations"))
    porGolpe = {}
    fxSD = {}        # mi -> (nFotogramas, escala, [(nombre,w,h,pal,rle)])
    rangos = []      # los trozos de `partes` que se van a la tarjeta
    n_paleta = 0
    n_reducidos = 0
    n_caros = 0
    n_peores = 0
    caros = []
    for mi, nom_golpe in pares:
        anim = anims[nom_golpe]
        fich = fxanim.hoja_de(anim) or "(sin hoja)"
        # A lista de "pior que o efeito do tipo" vale AQUI tambem. Ela
        # so era consultada em golpes_con_hoja(), que e o caminho morto
        # —— estava escrita e sem efeito nenhum desde que se passou a
        # ler Data/PkmnAnimations.
        if nom_golpe in FX_PEOR_QUE_EL_TIPO:
            n_peores += 1
            continue
        try:
            a_mano = FX_CELDAS_A_MANO.get(nom_golpe)
            if a_mano:
                imgs, _div = fxanim.celdas(hojas_fx, a_mano[0], a_mano[1],
                                           a_mano[2],
                                           max_cuadros=FX_MOVE_CEL)
                if not imgs:
                    raise SystemExit(
                        "FX_CELDAS_A_MANO: %s no da fotogramas (%r %d..%d)"
                        % (nom_golpe, a_mano[0], a_mano[1], a_mano[2]))
                fich = a_mano[0]
            else:
                imgs, _div = fxanim.cuadros(anim, hojas_fx,
                                            max_cuadros=FX_MOVE_CEL)
        except SystemExit:
            raise
        except Exception:                        # noqa: BLE001
            continue
        if not imgs:
            continue
        marca, bytes0 = len(partes), estado["total"]
        globals()["CAPTURA"] = []
        try:
            r = empaqueta_fx_imgs(imgs, f"MV{mi}_", partes)
        except SystemExit:
            # Mas de 254 colores. En vez de renunciar al efecto se cuantiza
            # (ver reduce_colores) y se reintenta: una explosion a 200
            # colores no se distingue, y perderla si.
            del partes[marca:]
            estado["total"] = bytes0
            globals()["CAPTURA"] = []
            try:
                r = empaqueta_fx_imgs(imgs, f"MV{mi}_", partes, reducir=True)
                n_reducidos += 1
            except SystemExit:
                del partes[marca:]
                estado["total"] = bytes0
                continue
        r = [r] if r else None
        # Techo por golpe. El respaldo por tipo ya es correcto y se mueve,
        # asi que una hoja que cuesta media docena de golpes normales no
        # compensa: SHELLSMASH pedia 23 KB el solo. Sin este tope, cuatro
        # hojas casi fotograficas se comian 100 KB.
        coste = estado["total"] - bytes0
        if coste > FX_MOVE_MAX_B:
            del partes[marca:]
            estado["total"] = bytes0
            n_caros += 1
            # Se dice CUAL se cae y por cuanto: un mapeo a mano que no entra
            # por el tope se quedaba en silencio, y desde fuera parecia que
            # el mapeo estaba mal escrito.
            caros.append((coste, fich))
            continue
        if r and r[0][0] != "nullptr":
            porGolpe[mi] = r[0]
            # Las animaciones POR GOLPE van a la tarjeta, no a flash: son el
            # 90% de art.h (1,2 MB de 1,3) y solo hace falta la del golpe que
            # se acaba de lanzar. Las de TIPO y las de apoyo se quedan en
            # flash, que son 23 y asi el combate nunca se queda sin efecto
            # aunque no haya SD.
            fxSD[mi] = (r[0][1], r[0][2], list(CAPTURA))
            # El TROZO entero que acaba de emitirse, para borrarlo luego del
            # .h: filtrar por "ART_MV" linea a linea dejaba los `};` sueltos,
            # porque la lista y su cierre son elementos distintos.
            rangos.append((marca, len(partes)))
    globals()["CAPTURA"] = None
    # Las animaciones por golpe salen del .h y se van a /fx/fx.bin. En flash
    # solo queda un bitmap de "este golpe tiene la suya", para que el combate
    # sepa si merece la pena pedirsela a la tarjeta.
    #
    # Lo que se borra de `partes` son las piezas ART_MV*, que ya estan
    # capturadas en fxSD. Las de tipo y las de apoyo NO se tocan.
    bytes_sd = escribe_fx_sd(fxSD, n_golpes)
    for ini, fin in sorted(rangos, reverse=True):
        del partes[ini:fin]       # al reves, para no mover los de delante
    estado["total"] = 0           # ya no pesan en flash
    partes.append("// El efecto PROPIO de cada golpe vive en la TARJETA")
    partes.append("// (/fx/fx.bin, lo escribe este mismo generador): son el 90%")
    partes.append("// de los datos de arte y solo hace falta el del golpe que se")
    partes.append("// acaba de lanzar. En flash queda solo quien TIENE uno.")
    partes.append("// Los de TIPO y los de apoyo si se quedan aqui: son 23 y")
    partes.append("// hacen que el combate nunca se quede sin efecto sin SD.")
    partes.append(f"#define FX_MOVE_COUNT {n_golpes}")
    bits = [0] * ((n_golpes + 7) // 8)
    for i in porGolpe:
        bits[i >> 3] |= 1 << (i & 7)
    partes.append(tabla("FX_HAS_OWN", bits, 16, False))
    partes.append("static inline bool fxHasOwn(uint16_t mv) {")
    partes.append("  return mv < FX_MOVE_COUNT && (FX_HAS_OWN[mv >> 3] & (1 << (mv & 7)));")
    partes.append("}")
    partes.append("")
    resumen.append(f"  /fx/fx.bin: {bytes_sd // 1024} KB, {len(fxSD)} golpes")
    if n_peores:
        resumen.append(f"  {n_peores} mandados ao efecto del TIPO a proposito"
                       " (su hoja se ve peor que el generico)")

    total += estado["total"]
    hechos = sum(1 for lista, _, _ in frefs + srefs if lista != "nullptr")
    marcos = sum(n for _, n, _ in frefs + srefs)
    marcos += sum(n for _, n, _ in porGolpe.values())
    resumen.append(f"  {hechos} efectos genericos ({len(srefs)} de apoyo)"
                   f" + {len(porGolpe)} por golpe, {marcos} fotogramas"
                   + (f" ({estado['vacios']} vacios)" if estado["vacios"] else "")
                   + (f", {n_paleta} tiras de paleta descartadas" if n_paleta else "")
                   + (f", {n_reducidos} cuantizados" if n_reducidos else "")
                   + (f", {n_caros} demasiado caros" if n_caros else ""))
    for coste, fich in sorted(caros, reverse=True)[:10]:
        resumen.append(f"    caro ({coste // 1024} KB, tope "
                       f"{FX_MOVE_MAX_B // 1024}): {fich}")

    # --- simbolos de tipo (solo el dibujo, sin el texto en espanol) ---
    tp = Image.open(os.path.join(ANIL, TYPES_PNG.replace("/", os.sep))).convert("RGBA")
    filas = tp.height // TYPE_ROW_H
    trefs, tbg = [], []
    for i in range(filas):
        cel = tp.crop((0, i * TYPE_ROW_H, TYPE_ICON_W, (i + 1) * TYPE_ROW_H))
        if not [1 for p in cel.getdata() if p[3] >= 8]:
            trefs.append("nullptr")       # la fila vacia de la hoja
            tbg.append(0)
            continue
        cel, fondo = sin_fondo(cel)
        tbg.append(rgb565(*fondo) if fondo else 0)
        if not [1 for p in cel.getdata() if p[3] >= 8]:
            trefs.append("nullptr")       # cuadro liso sin dibujo (la fila azul)
            continue
        w, h, pal, idx = indexa(cel)
        c = f"ART_TY{len(trefs)}"
        total += pieza(c, w, h, pal, idx, partes)
        trefs.append("&" + c)
    partes.append("// Simbolo de cada tipo, en el MISMO orden que la hoja del")
    partes.append("// Anil. La traduccion de ese orden a nuestros T_* vive en")
    partes.append("// TYPE_ART de dex.h; aqui solo esta el dibujo.")
    partes.append(f"#define ART_TYPE_ROWS {len(trefs)}")
    partes.append(f"static const Art *const ART_TYPE_SHEET[{len(trefs)}] = {{")
    for i in range(0, len(trefs), 5):
        partes.append("  " + " ".join(r + "," for r in trefs[i:i + 5]))
    partes.append("};")
    partes.append("// El color del cuadro que llevaba cada simbolo detras. El")
    partes.append("// dibujo es blanco y esta pensado para leerse encima de ESTE")
    partes.append("// color, asi que la chapa se pinta con el.")
    partes.append(f"static const uint16_t ART_TYPE_BG[{len(tbg)}] = {{")
    for i in range(0, len(tbg), 8):
        partes.append("  " + " ".join(f"0x{v:04X}," for v in tbg[i:i + 8]))
    partes.append("};")
    # La hoja del Anil va en SU orden (el de los juegos en espanol) y el
    # nuestro es otro: acero esta en medio alli y al final aqui, y el fuego
    # arranca despues de un hueco vacio. Esta lista es esa traduccion, con
    # el indice = nuestro T_*.
    #   T_NONE, NORMAL, FIGHT, FLY, POISON, GROUND, ROCK, BUG, GHOST,
    #   FIRE, WATER, GRASS, ELEC, PSYCHIC, ICE, DRAGON, DARK, STEEL, FAIRY
    FILA_DE_TIPO = [-1, 0, 1, 2, 3, 4, 5, 6, 7, 10, 11, 12, 13, 14, 15,
                    16, 17, 8, 18]
    partes.append("")
    partes.append("// Fila de la hoja que le toca a cada T_* nuestro.")
    partes.append("static inline const Art *typeArt(uint8_t t) {")
    partes.append("  static const int8_t FILA[] = { " +
                  ", ".join(str(v) for v in FILA_DE_TIPO) + " };")
    partes.append("  if (t >= sizeof(FILA) / sizeof(FILA[0])) return nullptr;")
    partes.append("  int8_t f = FILA[t];")
    partes.append("  return (f < 0 || f >= ART_TYPE_ROWS) ? nullptr : ART_TYPE_SHEET[f];")
    partes.append("}")
    partes.append("")
    partes.append("// El color de chapa que acompana a ese simbolo. 0 = no hay.")
    partes.append("static inline uint16_t typeArtBg(uint8_t t) {")
    partes.append("  static const int8_t FILA[] = { " +
                  ", ".join(str(v) for v in FILA_DE_TIPO) + " };")
    partes.append("  if (t >= sizeof(FILA) / sizeof(FILA[0])) return 0;")
    partes.append("  int8_t f = FILA[t];")
    partes.append("  return (f < 0 || f >= ART_TYPE_ROWS) ? 0 : ART_TYPE_BG[f];")
    partes.append("}")
    partes.append("")
    resumen.append(f"  {sum(1 for r in trefs if r != 'nullptr')} simbolos de tipo")

    # --- insignias: MASCARAS em cinza, uma tira por regiao ---
    # O firmware as tinge na hora de desenhar com o tipo do ginasio DAQUELA
    # regiao (drawBadge). Antes vinham ja tingidas com os tipos de Kanto, e
    # o dono viu "as insignias de johto estao iguais as de kanto".
    #   Kanto: UI/Trainer Card/badges0.png (48 px, creme)
    #   Johto: UI/Trainer Card/icon_badges.png, 2a fileira (32 px, cinza)
    # As outras regioes ainda nao tem desenho (nem o Anil nem o PokeSprite
    # tem): usam a de Kanto, tingida com o tipo certo.
    fontes = [
        ("K", Image.open(os.path.join(UI, "badges0.png")).convert("RGBA"), 48, 0),
        ("J", Image.open(os.path.join(UI, "icon_badges.png")).convert("RGBA"), 32, 32),
    ]
    linhas, npals = [], []
    for letra, tira, lado, y0 in fontes:
        refs, ns = [], []
        for i in range(8):
            cel = tira.crop((i * lado, y0, (i + 1) * lado, y0 + lado))
            w, h, pal, idx = indexa(cel)
            c = f"ART_BADGE_{letra}{i}"
            total += pieza(c, w, h, pal, idx, partes)
            refs.append("&" + c)
            ns.append(str(len(pal)))
        linhas.append("  { " + ", ".join(refs) + " },")
        npals.append("  { " + ", ".join(ns) + " },")
    # Hoenn a Paldea: as insignias de verdade, COM COR (nao se tingem),
    # baixadas por tools/baixa_insignias.py. Reduzidas para caber em 40 px;
    # as pequenas (os Cristais Z de Alola, 32 px de pixel art) sao
    # recortadas e ampliadas x2 no vizinho mais proximo.
    #
    # NAO vao para o art.h: sao 56 e passavam de 70 KB de flash, com o
    # flash a 92%. Vao para /por/insignias.bin no cartao (escreve_insignias)
    # e o firmware carrega a regiao que vai desenhar. Aqui ficam nullptr.
    pasta = os.path.join(os.path.dirname(os.path.abspath(__file__)), "insignias")
    cores = 0
    no_cartao = {}
    for reg in range(2, 9):
        refs, ns = [], []
        for i in range(8):
            p = os.path.join(pasta, "%d_%d.png" % (reg, i))
            if not os.path.exists(p):
                refs.append("nullptr")
                ns.append("0")
                continue
            im = Image.open(p).convert("RGBA")
            bb = im.getchannel("A").point(lambda v: 255 if v > 24 else 0).getbbox()
            if bb:
                im = im.crop(bb)
            lado = max(im.size)
            if lado <= 24:
                im = im.resize((im.width * 2, im.height * 2), Image.NEAREST)
            elif lado > 40:
                k = 40.0 / lado
                im = im.resize((max(1, round(im.width * k)), max(1, round(im.height * k))),
                               Image.LANCZOS)
            a = im.getchannel("A").point(lambda v: 255 if v > 110 else 0)
            im.putalpha(a)
            im = reduce_colores(im, 48)
            w, h, pal, idx = indexa(im)
            no_cartao[(reg, i)] = (w, h, list(pal), list(comprimir(idx)))
            refs.append("nullptr")
            ns.append("0")
            cores += 1
        linhas.append("  { " + ", ".join(refs) + " },")
        npals.append("  { " + ", ".join(ns) + " },")
    partes.append("#define ART_BADGE_REGIOES 9")
    partes.append("// [regiao][ginasio]. Kanto e Johto sao MASCARAS (o firmware tinge com")
    partes.append("// o tipo do ginasio); de Hoenn em diante, com cor. nullptr = sem")
    partes.append("// desenho: cai na mascara de Kanto tingida.")
    partes.append("static const Art *const ART_BADGE_REG[ART_BADGE_REGIOES][8] = {")
    partes += linhas
    partes.append("};")
    partes.append("static const uint8_t ART_BADGE_NPAL[ART_BADGE_REGIOES][8] = {")
    partes += npals
    partes.append("};")
    partes.append("static const bool ART_BADGE_MASCARA[ART_BADGE_REGIOES] = {")
    partes.append("  true, true, false, false, false, false, false, false, false };")
    partes.append("// De ART_BADGE_SD_PRIMEIRA em diante as insignias vem do cartao")
    partes.append("// (/por/insignias.bin, SdInsignias em sdmon.h).")
    partes.append("#define ART_BADGE_SD_PRIMEIRA 2")
    partes.append("")
    kb = escreve_insignias(no_cartao)
    resumen.append(f"  16 mascaras + {cores} insignias com cor no cartao ({kb:.1f} KB)")

    # --- as FITAS das conquistas (tools/baixa_fitas.py), no cartao ---
    # Uma por conquista, na ordem de MED_* e depois das regioes. Ate 36 px:
    # cabem na pastilha de 44 da pagina de conquistas.
    pasta = os.path.join(os.path.dirname(os.path.abspath(__file__)), "fitas")
    fitas = {}
    for i in range(32):
        p = os.path.join(pasta, "%d.png" % i)
        if not os.path.exists(p):
            continue
        im = Image.open(p).convert("RGBA")
        bb = im.getchannel("A").point(lambda v: 255 if v > 24 else 0).getbbox()
        if bb:
            im = im.crop(bb)
        if max(im.size) > 36:
            k = 36.0 / max(im.size)
            im = im.resize((max(1, round(im.width * k)), max(1, round(im.height * k))),
                           Image.LANCZOS)
        im.putalpha(im.getchannel("A").point(lambda v: 255 if v > 110 else 0))
        im = reduce_colores(im, 48)
        w, h, pal, idx = indexa(im)
        fitas[(0, i)] = (w, h, list(pal), list(comprimir(idx)))
    kb = escreve_tira("fitas.bin", "TPFT", 0, 1, 32, fitas)
    resumen.append(f"  {len(fitas)} fitas de conquista no cartao ({kb:.1f} KB)")

    # --- las marcas del aviso de la hierba ---
    total += marcas(partes, resumen)
    total += hierba(partes, resumen)
    total += iconos(partes, resumen)

    with open(os.path.join(ROOT, "art.h"), "w", encoding="utf-8", newline="\n") as fh:
        fh.write("\n".join(partes))
    print("\n".join(resumen))
    print(f"art.h: {total} bytes de datos en flash")


if __name__ == "__main__":
    main()
