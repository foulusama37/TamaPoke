# -*- coding: utf-8 -*-
"""Hasta donde llega la Pokedex. UN solo sitio.

Estaba repetido como `range(1, 152)` en gen_names, gen_stats, gen_types y
gen_sizes, mas el 151 escrito a mano en los mensajes. Anadir una generacion
obligaba a tocar los cuatro y acordarse de los cuatro.

  151 = Kanto
  251 = + Johto
  386 = + Hoenn
  493 = + Sinnoh
  649 = + Teselia
  721 = + Kalos
  809 = + Alola
  905 = + Galar
 1025 = + Paldea (la dex completa)
"""

N_DEX = 1025

# Donde acaba cada region, en orden. Era un par de constantes sueltas
# (N_KANTO, y el 151 a mano en gen_dex); con la tercera region ya no vale
# contar de dos en dos. El firmware lo recibe como DEX_REGION_END[].
REGIONES = [
    ("KANTO", 151),
    ("JOHTO", 251),
    ("HOENN", 386),
    ("SINNOH", 493),
    ("UNOVA", 649),
    ("KALOS", 721),
    ("ALOLA", 809),
    ("GALAR", 905),
    ("PALDEA", 1025),
]

assert REGIONES[-1][1] == N_DEX, 'la ultima region tiene que acabar en N_DEX'

# Compatibilidad con lo que ya lo usaba por nombre.
N_KANTO = REGIONES[0][1]
N_JOHTO = REGIONES[1][1]
