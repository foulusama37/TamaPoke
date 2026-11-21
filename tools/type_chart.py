"""Tabla de efectividad de tipos.

Fuente de verdad, escrita a mano: aqui se decide como pelea el juego.
Solo se listan los casos que NO son x1. Multiplicadores como enteros x100
(0, 50, 100, 200) para no meter coma flotante en el firmware.

Son 18 tipos, no 15. La razon: los golpes salen del PBS de Pokemon Anil, que
llega hasta gen 9, y ahi hay CRUNCH (siniestro), IRONHEAD (acero) y PLAYROUGH
(hada). Contando solo los que las 151 aprenden al subir de nivel ya aparecen
los 18. Ademas van a entrar mas generaciones pronto, asi que no tiene sentido
montar una tabla que habria que rehacer.

Si algun dia se quiere el juego "gen 1 puro", poner MODERNO = False: se aplican
las cuatro diferencias historicas de abajo. Los tipos siniestro/acero/hada
siguen existiendo en la tabla (hacen falta para que los indices no bailen),
simplemente no los usa nadie si los golpes tambien se filtran a gen 1.
"""

MODERNO = True

TIPOS = ['none', 'normal', 'fighting', 'flying', 'poison', 'ground', 'rock',
         'bug', 'ghost', 'fire', 'water', 'grass', 'electric', 'psychic',
         'ice', 'dragon', 'dark', 'steel', 'fairy']

# atacante: {defensor: multiplicador x100}. Tabla moderna (gen 6 en adelante).
CHART = {
    'normal':   {'rock': 50, 'ghost': 0, 'steel': 50},
    'fighting': {'normal': 200, 'rock': 200, 'steel': 200, 'ice': 200, 'dark': 200,
                 'poison': 50, 'flying': 50, 'psychic': 50, 'bug': 50, 'fairy': 50,
                 'ghost': 0},
    'flying':   {'fighting': 200, 'bug': 200, 'grass': 200,
                 'rock': 50, 'steel': 50, 'electric': 50},
    'poison':   {'grass': 200, 'fairy': 200,
                 'poison': 50, 'ground': 50, 'rock': 50, 'ghost': 50, 'steel': 0},
    'ground':   {'poison': 200, 'rock': 200, 'steel': 200, 'fire': 200, 'electric': 200,
                 'grass': 50, 'bug': 50, 'flying': 0},
    'rock':     {'flying': 200, 'bug': 200, 'fire': 200, 'ice': 200,
                 'fighting': 50, 'ground': 50, 'steel': 50},
    'bug':      {'grass': 200, 'psychic': 200, 'dark': 200,
                 'fighting': 50, 'flying': 50, 'poison': 50, 'ghost': 50,
                 'steel': 50, 'fire': 50, 'fairy': 50},
    'ghost':    {'ghost': 200, 'psychic': 200, 'dark': 50, 'normal': 0},
    'fire':     {'bug': 200, 'steel': 200, 'grass': 200, 'ice': 200,
                 'rock': 50, 'fire': 50, 'water': 50, 'dragon': 50},
    'water':    {'ground': 200, 'rock': 200, 'fire': 200,
                 'water': 50, 'grass': 50, 'dragon': 50},
    'grass':    {'ground': 200, 'rock': 200, 'water': 200,
                 'flying': 50, 'poison': 50, 'bug': 50, 'steel': 50,
                 'fire': 50, 'grass': 50, 'dragon': 50},
    'electric': {'flying': 200, 'water': 200,
                 'grass': 50, 'electric': 50, 'dragon': 50, 'ground': 0},
    'psychic':  {'fighting': 200, 'poison': 200,
                 'steel': 50, 'psychic': 50, 'dark': 0},
    'ice':      {'flying': 200, 'ground': 200, 'grass': 200, 'dragon': 200,
                 'steel': 50, 'fire': 50, 'water': 50, 'ice': 50},
    'dragon':   {'dragon': 200, 'steel': 50, 'fairy': 0},
    'dark':     {'ghost': 200, 'psychic': 200,
                 'fighting': 50, 'dark': 50, 'fairy': 50},
    # ACERO. Esta fila NO EXISTIA: la tabla se escribio a mano y se salto
    # la clave entera, asi que mult() caia en su `.get(atacante, {})` y
    # devolvia x1 contra los dieciocho tipos. Ningun golpe de acero era
    # supereficaz nunca —— ni contra hada, ni contra hielo, ni contra
    # roca —— y tampoco se lo resistia nadie.
    #
    # No se ve jugando: un Cabeza de Hierro que no multiplica parece un
    # golpe flojo, no un juego roto. Salio al pintar la tabla de
    # debilidades en la ficha, donde a un Aurorus (roca/hielo) le
    # faltaba el x4 de acero. Lo comprueba tools/chequea_tipos.py contra
    # PBS/types.txt del Anil, casilla a casilla: las otras 316 estaban
    # bien, asi que era esta fila y nada mas.
    'steel':    {'rock': 200, 'ice': 200, 'fairy': 200,
                 'fire': 50, 'water': 50, 'electric': 50, 'steel': 50},
    'fairy':    {'fighting': 200, 'dragon': 200, 'dark': 200,
                 'poison': 50, 'steel': 50, 'fire': 50},
}

# Lo que cambio de gen 1 a hoy entre los 15 tipos que ya existian entonces.
# El de fantasma contra psiquico era un BUG del juego, no una decision: hacia
# cero y por eso los psiquicos eran intocables en gen 1.
DIFERENCIAS_GEN1 = {
    ('bug', 'poison'): 200,     # hoy x0,5
    ('poison', 'bug'): 200,     # hoy x1
    ('ice', 'fire'): 100,       # el fuego no resistia el hielo
    ('ghost', 'psychic'): 0,    # el bug famoso
}


# Nombre corto para el chip de la tarjeta de golpe y color de cada tipo. Los
# nombres se cortan a 5 chars porque el chip vive dentro de una tarjeta de
# 257 px que ya lleva flechas: FIGHTING no cabe, FIGHT si.
ETIQUETAS = {
    'none': '---',     'normal': 'NORMAL', 'fighting': 'FIGHT', 'flying': 'FLY',
    'poison': 'POISON', 'ground': 'GROUND', 'rock': 'ROCK',     'bug': 'BUG',
    'ghost': 'GHOST',  'fire': 'FIRE',     'water': 'WATER',    'grass': 'GRASS',
    'electric': 'ELEC', 'psychic': 'PSY',  'ice': 'ICE',        'dragon': 'DRAGON',
    'dark': 'DARK',    'steel': 'STEEL',   'fairy': 'FAIRY',
}
COLORES = {
    'none': '#8a8a8a',     'normal': '#a8a878', 'fighting': '#c03028',
    'flying': '#a890f0',   'poison': '#a040a0', 'ground': '#e0c068',
    'rock': '#b8a038',     'bug': '#a8b820',    'ghost': '#705898',
    'fire': '#f08030',     'water': '#6890f0',  'grass': '#78c850',
    'electric': '#f8d030', 'psychic': '#f85888', 'ice': '#98d8d8',
    'dragon': '#7038f8',   'dark': '#705848',   'steel': '#b8b8d0',
    'fairy': '#ee99ac',
}


def mult(atacante, defensor):
    """multiplicador x100 de un tipo contra otro"""
    if atacante == 'none' or defensor == 'none':
        return 100
    if not MODERNO and (atacante, defensor) in DIFERENCIAS_GEN1:
        return DIFERENCIAS_GEN1[(atacante, defensor)]
    return CHART.get(atacante, {}).get(defensor, 100)


def matriz():
    """[atacante][defensor] en el orden de TIPOS, listo para emitir a C"""
    return [[mult(a, d) for d in TIPOS] for a in TIPOS]
