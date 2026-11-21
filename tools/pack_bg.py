#!/usr/bin/env python3
"""Empaquetador de fondos de combate.

Coge los battlebacks del Anil (512x384) y los deja en /bg/NN.bin listos para
volcarlos de la SD a la pantalla sin descomprimir nada:

  char[4] "TPKB"
  u16 w, u16 h
  u16 pix[w*h]            RGB565, little endian, fila a fila

Son 466x466 = 434 KB por fondo. Va en crudo y no comprimido a proposito: el
combate repinta el fondo ENTERO en cada fotograma, asi que tiene que ser un
memcpy por fila desde la PSRAM. Un RLE habria que descomprimirlo igualmente a
un buffer del mismo tamano, y en una foto (no pixel art) apenas comprime.

El recorte es "cubrir": se escala hasta tapar los 466x466 y se corta lo que
sobra por los lados. Los battlebacks son 4:3 y la pantalla es cuadrada; si se
ajustara por dentro quedarian franjas arriba y abajo, y justo arriba es donde
va la barra de vida del rival.

  python3 tools/pack_bg.py            # todos los de la lista
  python3 tools/pack_bg.py pradera    # solo ese
"""
import os
import struct
import sys

from PIL import Image

ANIL = r"C:\Users\foulu\Downloads\Pokemon Anil V4.13\Graphics"
BACKS = os.path.join(ANIL, "Battlebacks")
OUT = os.path.join(os.path.dirname(__file__), "sdcard", "bg")
SIDE = 466

# (id, fichero del Anil). El id es el numero del .bin y el que usa BG_* en
# battle_ui: una vez publicado NO se cambia, o los ficheros de una SD vieja
# salen en el sitio de otro.
# Elegidos MIRANDOLOS, no por el nombre: los del Anil estan en espanol y
# enganan ("Monte_bg" es un prado, no una montana; "BosqueLila" es lila de
# verdad y salia un bosque morado en la pantalla de casa). El contacto de los
# candidatos se saca con _claude_scratch/hoja_cand.py, y el de lo YA empaquetado
# — que es lo que de verdad se ve en la placa — con _claude_scratch/hoja_bins.py.
FONDOS = [
    # 0..8: los biomas de la dex (DexEntry.biome) + los dos comodines
    (0,  "Pradera_bg.png"),          # pradera, y el de por defecto
    (1,  "Bosque_bg.png"),           # bosque
    (2,  "Cueva_bg.png"),            # roca/cueva
    (3,  "Playa_bg.png"),            # playa
    (4,  "Ciudad_bg.png"),           # ciudad: entrenadores
    (5,  "ZonaEscarpada_bg.png"),    # montana
    (6,  "Noche_bg.png"),            # noche (sin usar todavia)
    (7,  "Volcan_bg.png"),           # volcan
    (8,  "CimaNevada_bg.png"),       # nieve
    # 9..19: una casa por lider, en el orden de KANTO_GYMS
    (9,  "Agua_bg.png"),             # MISTY
    (10, "GimnasioSurge_bg.png"),    # SURGE
    (11, "PraderaFlores_bg.png"),    # ERIKA
    (12, "InteriorOscuro_bg.png"),   # KOGA
    (13, "GimnasioSabrina_bg.png"),  # SABRINA
    (14, "BaseVillano_bg.png"),      # GIOVANNI, que es el jefe de Rocket
    (15, "CuevaHielo_bg.png"),       # LORELEI
    (16, "Templo_bg.png"),           # BRUNO
    (17, "TorrePokemon_bg.png"),     # AGATHA
    (18, "Castillo_bg.png"),         # LANCE
    (19, "Campeon_bg.png"),          # AZUL
    # 20..40: VARIANTES. Un solo paisaje por bioma cansa enseguida, y la
    # pantalla de casa esta siempre encendida. Los ids nuevos van al final
    # para no mover los de arriba, que ya estan en tarjetas.
    (20, "Pradera2_bg.png"),
    (21, "Pradera3_bg.png"),
    (22, "PraderaFlores_bg.png"),    # (tambien es la casa de ERIKA, id 11)
    (23, "Playa2_bg.png"),
    (24, "water_celeste_bg.png"),
    (25, "BosqueArcoiris_bg.png"),
    (26, "Monte_bg.png"),            # era BosqueLila: lila de verdad, no bosque
    (27, "Otono_bg.png"),
    (28, "CuevaRoja_bg.png"),
    (29, "PraderaMontania_bg.png"),
    (30, "PraderaMontania2_bg.png"),
    (31, "Acantilado_bg.png"),
    (32, "snow_bg.png"),
    (33, "Helado_bg.png"),
    (34, "CuevaNieve_bg.png"),
    (35, "Nocturno_bg.png"),
    (36, "Espacio_bg.png"),          # era PraderaMontaniaNoche: magenta entero
    (37, "CuevaAzul_bg.png"),
    (38, "CuevaOscura_bg.png"),
    (39, "Pueblo_bg.png"),
    (40, "CaminoBici_bg.png"),
    # El laboratorio: la intro del profesor, la eleccion de inicial y el
    # reloj/ajustes, que hasta ahora eran un circulo beige.
    (41, "Laboratorio_bg.png"),
]

# 42..: de OTROS fangames del PC (leva 71), copiados a tools/fondos_extra
# para no depender de sus carpetas. Elegidos mirandolos, uno a uno, entre
# 520 sin repetir (_claude_scratch/bg71). Mismo pacto: el id no se cambia.
EXTRA = os.path.join(os.path.dirname(os.path.abspath(__file__)), "fondos_extra")
FONDOS_EXTRA = [
    (42, "UT_forest.png"),                     # pradera
    (43, "Rebirth_Forest10_bg.png"),           # pradera
    (44, "Rebirth_Forest13_bg.png"),           # pradera
    (45, "Rebirth_10.png"),                    # pradera
    (46, "Rebirth_9.png"),                     # pradera
    (47, "Awakening_battlebgFieldMountainNight.png"), # pradera
    (48, "UT_prado_bg.png"),                   # pradera
    (49, "Vanguard_Courtyard_bg.png"),         # pradera
    (50, "Realidea_battlebgField.png"),        # pradera
    (51, "Realidea_battlebgPradera.png"),      # pradera
    (52, "UT_cima_tue_tue_bg.png"),            # pradera
    (53, "UT_escuela_bg.png"),                 # pradera
    (54, "UT_jardin_palacio_2_bg.png"),        # pradera
    (55, "UT_montana_2_bg.png"),               # pradera
    (56, "Rebirth_water3_bg.png"),             # praia
    (57, "Rebirth_cave44_bg.png"),             # praia
    (58, "Awakening_battlebgSand.png"),        # praia
    (59, "UT_summer_bg.png"),                  # praia
    (60, "Realidea_battlebgPlaya.png"),        # praia
    (61, "UT_isladelsur_bg.png"),              # praia
    (62, "Rejuv_battlebgBeach.png"),           # praia
    (63, "Rejuv_battlebgBeachEve.png"),        # praia
    (64, "Rebirth_14.png"),                    # floresta
    (65, "Rebirth_cave47_bg.png"),             # floresta
    (66, "Awakening_battlebgFieldNighta.png"), # floresta
    (67, "UT_jungle3_bg.png"),                 # floresta
    (68, "UT_forest_2_bg.png"),                # floresta
    (69, "Realidea_battlebgBosque.png"),       # floresta
    (70, "UT_Sakura_bg.png"),                  # floresta
    (71, "Rejuv_battlebgForest.png"),          # floresta
    (72, "Rejuv_battlebgTime.png"),            # floresta
    (73, "UT_granja_bg.png"),                  # floresta
    (74, "Realidea_battlebgotonyo.png"),       # floresta
    (75, "Cosmo_jardim_bg.png"),               # floresta
    (76, "Awakening_battlebgNina.png"),        # vulcao
    (77, "UT_cave9_bg.png"),                   # vulcao
    (78, "Realidea_battlebgVolcan.png"),       # vulcao
    (79, "Rejuv_battlebgVolcano.png"),         # vulcao
    (80, "Rebirth_rocky1_bg.png"),             # montanha
    (81, "Rebirth_rocky_bg.png"),              # montanha
    (82, "Rebirth_cave45_bg.png"),             # montanha
    (83, "UT_mountain_bg.png"),                # montanha
    (84, "Rebirth_sand1_bg.png"),              # montanha
    (85, "Rebirth_sand2_bg.png"),              # montanha
    (86, "Rebirth_sand3_bg.png"),              # montanha
    (87, "Awakening_battlebgDesierto.png"),    # montanha
    (88, "Realidea_battlebgDesierto.png"),     # montanha
    (89, "Vanguard_desert_bg.png"),            # montanha
    (90, "Reminiscencia_battlebgBosqueNieve.png"), # neve
    (91, "Rebirth_ice_bg.png"),                # neve
    (92, "Rebirth_cave43_bg.png"),             # neve
    (93, "Awakening_a.png"),                   # neve
    (94, "UT_nieve.png"),                      # neve
    (95, "UT_snow_bg.png"),                    # neve
    (96, "UT_granja_night_bg.png"),            # noite
    (97, "UT_ruta1_night_bg.png"),             # noite
    (98, "Realidea_battlebgDesiertoNight.png"), # noite
    (99, "Realidea_battlebgFieldNight.png"),   # noite
    (100, "Realidea_battlebgSara.png"),         # noite
    (101, "UT_cima_sendero_bg.png"),            # noite
    (102, "UT_isladelsur_night_bg.png"),        # noite
    (103, "UT_jardin_palacio_2_night_bg.png"),  # noite
    (104, "UT_montana_2_night_bg.png"),         # noite
    (105, "Rebirth_cave12B_bg.png"),            # caverna
    (106, "Rebirth_cave12C_bg.png"),            # caverna
    (107, "Rebirth_cave1_ice_bg.png"),          # caverna
    (108, "Awakening_battlebgCave.png"),        # caverna
    (109, "UT_cave4_bg.png"),                   # caverna
    (110, "UT_cave5_bg.png"),                   # caverna
    (111, "UT_cave6_bg.png"),                   # caverna
    (112, "Vanguard_cave_bg.png"),              # caverna
    (113, "Realidea_battlebgSableye1.png"),     # caverna
    (114, "Vanguard_gemstone_bg.png"),          # caverna
    (115, "Rejuv_battlebgAmethystCave.png"),    # caverna
    (116, "Rejuv_battlebgIcy.png"),             # caverna
    (117, "Rebirth_city12_bg.png"),             # cidade
    (118, "Rebirth_city19_bg.png"),             # cidade
    (119, "Rebirth_city20_bg.png"),             # cidade
    (120, "Rebirth_city_bg.png"),               # cidade
    (121, "Rebirth_Forest5_bg.png"),            # cidade
    (122, "Rebirth_cave48_bg.png"),             # cidade
    (123, "Rebirth_cave49_bg.png"),             # cidade
    (124, "Rebirth_cave52_bg.png"),             # cidade
    (125, "Realidea_battlebgPuente.png"),       # cidade
    (126, "UT_ciudad_bg.png"),                  # cidade
    (127, "UT_ciudad_eve_bg.png"),              # cidade
]


def rgb565(r, g, b):
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def cubrir(im, lado):
    """Escala para tapar el cuadrado y recorta centrado."""
    e = max(lado / im.width, lado / im.height)
    n = im.resize((max(lado, round(im.width * e)), max(lado, round(im.height * e))),
                  Image.LANCZOS)
    x = (n.width - lado) // 2
    y = (n.height - lado) // 2
    return n.crop((x, y, x + lado, y + lado))


def empaqueta(idn, nombre, carpeta=None):
    ruta = os.path.join(carpeta or BACKS, nombre)
    if not os.path.exists(ruta):
        print(f"  !! falta {nombre}")
        return 0
    im = cubrir(Image.open(ruta).convert("RGB"), SIDE)
    px = im.load()
    buf = bytearray(SIDE * SIDE * 2)
    i = 0
    for y in range(SIDE):
        for x in range(SIDE):
            r, g, b = px[x, y]
            v = rgb565(r, g, b)
            buf[i] = v & 0xFF
            buf[i + 1] = v >> 8
            i += 2
    os.makedirs(OUT, exist_ok=True)
    destino = os.path.join(OUT, f"{idn:02d}.bin")
    with open(destino, "wb") as f:
        f.write(b"TPKB")
        f.write(struct.pack("<HH", SIDE, SIDE))
        f.write(buf)
    n = 8 + len(buf)
    print(f"  {idn:2d} {nombre:26s} {n // 1024} KB")
    return n


def main():
    filtro = [a.lower() for a in sys.argv[1:]]
    total = 0
    for idn, nombre in FONDOS:
        if filtro and not any(f in nombre.lower() for f in filtro):
            continue
        total += empaqueta(idn, nombre)
    for idn, nombre in FONDOS_EXTRA:
        if filtro and not any(f in nombre.lower() for f in filtro):
            continue
        total += empaqueta(idn, nombre, EXTRA)
    print(f"{total // 1024} KB en {OUT}")


if __name__ == "__main__":
    main()
