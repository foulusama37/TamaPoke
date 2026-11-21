#!/usr/bin/env python3
"""Genera items.h: los objetos que un bicho puede LLEVAR ENCIMA.

El PBS tiene 889 objetos y aqui casi ninguno significa nada: no hay tienda, ni
mochila, ni mapa, ni cambio de Pokemon. Asi que la lista va A MANO, elegida
por lo que cambia un combate de uno contra uno, y de la traduccion del propio
Anil solo se saca el NOMBRE en ingles (el PBS sigue en espanol; el ingles esta
en Data/messages_english_game.dat, indice 7).

Las mecanicas estan escritas aqui, no sacadas del FunctionCode: los objetos de
Essentials se implementan con handlers de Ruby, no con un codigo declarativo
como los golpes.

  python3 tools/gen_items.py
"""
import os
import re
import sys
import unicodedata

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import marshal_lite  # noqa: E402
import texto_pt  # noqa: E402
from pt_items import OBJETOS as PT_OBJETOS  # noqa: E402

ANIL = r"C:\Users\foulu\Downloads\Pokemon Anil V4.13"
DAT = os.path.join(ANIL, "Data", "messages_english_game.dat")
PBS = os.path.join(ANIL, "PBS", "items.txt")
RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DST = os.path.join(RAIZ, "items.h")

MT_NOMBRES_OBJ = 7
# La 9 son las DESCRIPCIONES de objeto (espanol -> ingles), 904 entradas.
# Se encontro mirando las 32 tablas del .dat, no adivinando: la 7 son los
# nombres y la 8 los plurales, asi que la 9 cae justo detras.
MT_DESC_OBJ = 9

# Los bolsillos son los NUESTROS, no los del PBS. El PBS mete Restos, piedras
# de evolucion y repelentes todos en el bolsillo 1, que aqui no sirve de nada:
# lo que importa es si un objeto se LLEVA encima, se USA, o es una bola.
BOLSILLOS = ["BP_HELD", "BP_BATTLE", "BP_BALL", "BP_EVO", "BP_KEY"]

# (id del PBS, bolsillo, efecto, argumento, comentario)
# El argumento significa una cosa distinta en cada efecto; va explicado en el
# enum que se emite mas abajo.
LISTA = [
    ("LEFTOVERS",    "IT_LEFTOVERS",  0,  "un dieciseisavo por turno"),
    ("SITRUSBERRY",  "IT_BERRY_HEAL", 25, "se la come a media vida: un cuarto"),
    # ARANJA cura DIEZ PS, no un porcentaje. En el Anil son dos reglas
    # distintas y se ven una al lado de la otra en
    # 034_Other battle code/010_Battle_ItemEffects.rb:
    #   SITRUSBERRY  amt = battler.totalhp / 4
    #   ORANBERRY    amt = 10
    # Aqui estaba como un 12% del maximo, seguramente arrastrada por la
    # Zidra, que si es una fraccion. La diferencia no es pequena: a un
    # bicho de nivel 50 con 150 PS le curaba 18 en vez de 10, y a uno de
    # nivel 5 le curaba dos. Una baya de 20 monedas no puede escalar.
    ("ORANBERRY",    "IT_BERRY_FIX",  10, "la version barata: diez PS fijos"),
    ("LUMBERRY",     "IT_BERRY_CURE", 0,  "se come el estado que sea"),
    ("FOCUSSASH",    "IT_FOCUS_SASH", 0,  "aguanta un golpe mortal estando entero"),
    ("LIFEORB",      "IT_LIFE_ORB",   30, "+30% de dano, y un 10% de tu vida"),
    ("CHOICEBAND",   "IT_CHOICE",     1,  "+50% fisico, pero atado a un golpe"),
    ("CHOICESPECS",  "IT_CHOICE",     0,  "idem con el especial"),
    ("QUICKCLAW",    "IT_QUICK_CLAW", 20, "20% de salir primero pase lo que pase"),
    ("SCOPELENS",    "IT_SCOPE_LENS", 0,  "+1 al escalon de critico"),
    ("EVERSTONE",    "IT_EVERSTONE",  0,  "no evoluciona mientras lo lleve"),
    # Potenciadores de tipo: un solo efecto con el tipo como argumento, en vez
    # de dieciocho efectos iguales.
    ("CHARCOAL",     "IT_TYPE_BOOST", "T_FIRE",    "+20% al fuego"),
    ("MYSTICWATER",  "IT_TYPE_BOOST", "T_WATER",   "+20% al agua"),
    ("MIRACLESEED",  "IT_TYPE_BOOST", "T_GRASS",   "+20% a la planta"),
    ("MAGNET",       "IT_TYPE_BOOST", "T_ELEC",    "+20% al electrico"),
    ("TWISTEDSPOON", "IT_TYPE_BOOST", "T_PSYCHIC", "+20% al psiquico"),
    ("NEVERMELTICE", "IT_TYPE_BOOST", "T_ICE",     "+20% al hielo"),
    ("BLACKBELT",    "IT_TYPE_BOOST", "T_FIGHT",   "+20% a la lucha"),
    ("SHARPBEAK",    "IT_TYPE_BOOST", "T_FLYING",  "+20% al volador"),
    ("POISONBARB",   "IT_TYPE_BOOST", "T_POISON",  "+20% al veneno"),
    ("SOFTSAND",     "IT_TYPE_BOOST", "T_GROUND",  "+20% a la tierra"),
    ("HARDSTONE",    "IT_TYPE_BOOST", "T_ROCK",    "+20% a la roca"),
    ("SILVERPOWDER", "IT_TYPE_BOOST", "T_BUG",     "+20% al bicho"),
    ("SPELLTAG",     "IT_TYPE_BOOST", "T_GHOST",   "+20% al fantasma"),
    ("DRAGONFANG",   "IT_TYPE_BOOST", "T_DRAGON",  "+20% al dragon"),
    ("BLACKGLASSES", "IT_TYPE_BOOST", "T_DARK",    "+20% al siniestro"),
    # (La Metal Coat se fue al bolsillo de EVOLUCION: el PBS del Anil la pide
    #  para Steelix y Scizor, y un objeto solo puede tener un efecto. Pierde
    #  el +20% al acero, que es lo de menos: hay otros potenciadores.)
    ("SILKSCARF",    "IT_TYPE_BOOST", "T_NORMAL",  "+20% al normal"),
]

# Los que NO se llevan encima: se usan. Van aparte porque el bolsillo cambia
# lo que se puede hacer con ellos, no solo donde salen en la lista.
# Las megas que el Anil deja SIN piedra, con una inventada a proposito.
# Hoy es una sola:
#   Rayquaza    en los juegos megaevoluciona sabiendo Ascenso Draco, no
#               con piedra. Aqui no hay forma de pedir "sabe este golpe"
#               como condicion de objeto, asi que se le da la suya.
# El nombre sigue el patron de las demas (-ITE / -NITE).
#
# FUERA desde 2026-10-04, por decision del usuario: el "Mewtwo Armadura"
# (forma 5). No es una mega de los juegos, el Anil no le da piedra, y
# sobre todo **no tiene sprite de espalda en ningun proyecto** — se
# dibujaba cayendo al Mewtwo normal, asi que de espaldas era otro bicho.
# Ademas sus 400 de PS base lo ponian un 46% por encima de Blissey. No
# volver a meterlo sin que aparezca el sprite.
MEGAS_SIN_PIEDRA = {
    ("RAYQUAZA", 1): "RAYQUAZITE",
}

# ...y como no existe en el Anil, tampoco existe su nombre traducido: el
# resto de objetos lo saca del PBS y del .dat del usuario, y este hay
# que traerlo escrito. Doce caracteres como todo lo demas.
NOMBRE_INVENTADO = {
    "RAYQUAZITE":  "Rayquazite",
    # A Pokedex nao e objeto no PBS do Anil (la e o menu).
    "POKEDEX":     "Pokedex",
}

# Descricao dos que nao tem uma no PBS.
DESC_INVENTADA = {
    "POKEDEX": "A high-tech encyclopedia that records every Pokemon "
               "you have seen and caught.",
}

# CHAVE, de enfeite: ficam sempre no bolso de itens-chave (ver
# enEstaLista no pet.cpp). No FIM da tabela, depois das megas: o save
# guarda a bolsa por indice, e nada que ja existe pode mudar de numero.
CHAVES = [
    ("POKEDEX",   "BP_KEY", "IT_KEY", 0, "enfeite"),
    ("MEGARING",  "BP_KEY", "IT_KEY", 0, "enfeite"),
]

# OS PREMIOS DO QUIZ (leva 68). No FIM de tudo, pelo mesmo motivo das
# chaves: o save guarda a bolsa por indice. O SHINYZADOR e a Pocion
# Brillante do Anil, mas aqui faz OUTRA coisa (o dono pediu): deixa
# shiny o titular, em vez do proximo selvagem. Por isso a descricao vem
# de DESC_PROPRIA e nao do PBS.
PREMIOS = [
    ("RARECANDY",  "BP_BATTLE", "IT_RARECANDY", 0, "+1 nivel"),
    ("SHINYZADOR", "BP_BATTLE", "IT_SHINY",     0, "o titular fica shiny"),
]

# Descricao propria (ingles, espanhol) de quem faz aqui outra coisa que
# no Anil. O portugues vai em pt_items.py, como os outros.
DESC_PROPRIA = {
    "SHINYZADOR": ("A sparkling potion. The Pokemon that drinks it becomes "
                   "a shiny Pokemon for good.",
                   "Una pocion brillante. El Pokemon que la bebe se vuelve "
                   "variocolor para siempre."),
}

# AS OUTRAS BOLAS. Tambem no FIM, pelo mesmo motivo. Nao entram nos
# escaloes (BALL_ID): o multiplicador delas DEPENDE do combate (turno,
# hora, nivel, especie) e quem o calcula e ballMult() no capture.ino, a
# partir do BALLK_* que vai no `arg`. A ordem desta lista e a ordem das
# artes de arremesso (BALL_ALL / ART_BALL), depois das quatro de escalao.
BOLAS_EXTRA = [
    ("QUICKBALL",   "BP_BALL", "IT_BALLX", "BALLK_QUICK",   "x5 no 1o turno"),
    ("TIMERBALL",   "BP_BALL", "IT_BALLX", "BALLK_TIMER",   "melhora a cada turno"),
    ("DUSKBALL",    "BP_BALL", "IT_BALLX", "BALLK_DUSK",    "x3 de noite"),
    ("NETBALL",     "BP_BALL", "IT_BALLX", "BALLK_NET",     "x3,5 inseto/agua"),
    ("NESTBALL",    "BP_BALL", "IT_BALLX", "BALLK_NEST",    "melhor em nivel baixo"),
    ("REPEATBALL",  "BP_BALL", "IT_BALLX", "BALLK_REPEAT",  "x3,5 se ja capturou"),
    ("LEVELBALL",   "BP_BALL", "IT_BALLX", "BALLK_LEVEL",   "pelo seu nivel"),
    ("FASTBALL",    "BP_BALL", "IT_BALLX", "BALLK_FAST",    "x4 nos rapidos"),
    ("HEAVYBALL",   "BP_BALL", "IT_BALLX", "BALLK_HEAVY",   "melhor nos pesados"),
    ("MOONBALL",    "BP_BALL", "IT_BALLX", "BALLK_MOON",    "x4 nos da Moon Stone"),
    ("DREAMBALL",   "BP_BALL", "IT_BALLX", "BALLK_DREAM",   "x4 no adormecido"),
    ("FRIENDBALL",  "BP_BALL", "IT_BALLX", "BALLK_FRIEND",  "chega com vinculo alto"),
    ("LUXURYBALL",  "BP_BALL", "IT_BALLX", "BALLK_LUXURY",  "o vinculo cresce mais"),
    ("PREMIERBALL", "BP_BALL", "IT_BALLX", "BALLK_PREMIER", "comemorativa"),
]

# NOMBRES QUE EL ANIL TRADUCE MAL. Su .dat manda para los otros 211,
# pero aqui se le lleva la contraria con el motivo escrito al lado.
#
# FOCUSSASH se llama "Banda Aguante" en el PBS y el diccionario del Anil
# la pasa a ingles como "Focus Band" —— que es OTRO objeto suyo ("Cinta
# Aguante", FOCUSBAND) y con otra regla: la Band es un 10% de aguantar
# CUALQUIER golpe mortal, y la Sash aguanta uno solo, estando entero.
# Nosotros llevamos la SASH (ver LISTA, IT_FOCUS_SASH), asi que el
# jugador estaba leyendo el nombre del objeto que no tiene en la mano.
NOMBRE_CORREGIDO = {
    "FOCUSSASH":   "Focus Sash",
}


def piedra_de(cab, c):
    """La piedra de una forma mega: la del Anil, o la inventada."""
    p = (c.get("MegaStone") or "").strip()
    if p:
        return p
    partes = cab.split(",")
    especie = partes[0].strip().upper()
    forma = int(partes[1]) if len(partes) > 1 and partes[1].strip().isdigit() else 0
    return MEGAS_SIN_PIEDRA.get((especie, forma))


def piedras_mega():
    """Las piedras mega, sacadas del pokemon_forms.txt del Anil.

    A mano no: son mas de ciento veinte y cada una existe porque una forma
    la nombra, asi que la lista sale de donde esta el dato. El `arg` es el
    indice de la piedra, que es lo que megas.h usa para saber cual es cual
    (MEGA_<CLAVE>), y el bolsillo es BP_HELD porque una piedra mega se
    LLEVA puesta, no se usa desde la bolsa.

    Las dos que el Anil deja sin piedra las pone MEGAS_SIN_PIEDRA.
    """
    import anil_pbs
    vistas, out = {}, []
    for cab, c in anil_pbs.bloques(anil_pbs.FORMS):
        fn = c.get("FormName", "")
        if "Mega" not in fn and "Primal" not in fn:
            continue
        p = piedra_de(cab, c)
        if not p:
            continue
        k = re.sub(r"[^A-Z0-9]", "", p.upper())
        if k in vistas:
            continue
        vistas[k] = True
        out.append((p, "BP_HELD", "IT_MEGA", f"MEGA_{k}", fn))
    return out


USABLES = [
    # (id, bolsillo, efecto, arg, comentario)
    ("POTION",       "BP_BATTLE", "IT_POTION",   20,  "cura 20 PS"),
    ("SUPERPOTION",  "BP_BATTLE", "IT_POTION",   60,  "cura 60"),
    ("HYPERPOTION",  "BP_BATTLE", "IT_POTION",  120,  "cura 120"),
    ("MAXPOTION",    "BP_BATTLE", "IT_POTION",  255,  "255 = del todo"),
    ("FULLRESTORE",  "BP_BATTLE", "IT_FULLREST", 0,   "PS al maximo y sin estado"),
    ("REVIVE",       "BP_BATTLE", "IT_REVIVE",  50,   "levanta al desmayado a media vida"),
    ("MAXREVIVE",    "BP_BATTLE", "IT_REVIVE", 100,   "...y entero"),
    ("ANTIDOTE",     "BP_BATTLE", "IT_CURE",    "ST_POISON",    "quita el veneno"),
    ("PARALYZEHEAL", "BP_BATTLE", "IT_CURE",    "ST_PARALYSIS", "quita la paralisis"),
    ("AWAKENING",    "BP_BATTLE", "IT_CURE",    "ST_SLEEP",     "despierta"),
    ("BURNHEAL",     "BP_BATTLE", "IT_CURE",    "ST_BURN",      "quita la quemadura"),
    ("ICEHEAL",      "BP_BATTLE", "IT_CURE",    "ST_FREEZE",    "descongela"),
    ("FULLHEAL",     "BP_BATTLE", "IT_CURE",    0,    "0 = el que sea"),
    ("ETHER",        "BP_BATTLE", "IT_ETHER",   10,   "10 PP a un golpe"),
    ("MAXETHER",     "BP_BATTLE", "IT_ETHER",  255,   "un golpe al maximo"),
    ("ELIXIR",       "BP_BATTLE", "IT_ELIXIR",  10,   "10 PP a los cuatro"),
    ("MAXELIXIR",    "BP_BATTLE", "IT_ELIXIR", 255,   "los cuatro al maximo"),
    ("XATTACK",      "BP_BATTLE", "IT_XSTAT",  "SG_ATK", "+1 ataque en combate"),
    ("XDEFENSE",     "BP_BATTLE", "IT_XSTAT",  "SG_DEF", "+1 defensa"),
    ("XSPEED",       "BP_BATTLE", "IT_XSTAT",  "SG_SPE", "+1 velocidad"),
    ("XSPATK",       "BP_BATTLE", "IT_XSTAT",  "SG_SPA", "+1 at. especial"),
    ("DIREHIT",      "BP_BATTLE", "IT_XCRIT",   0,   "+2 al escalon de critico"),
    # Vitaminas: el camino rapido y caro a los EVs. El lento y gratis es cazar
    # a la especie que entrena lo que quieres. Una por stat, y por eso hubo que
    # partir los EVs en seis: con tres, Proteina y Calcio eran el mismo objeto.
    ("HPUP",         "BP_BATTLE", "IT_VITAMIN", "EV_HP",  "+10 EV de PS"),
    ("PROTEIN",      "BP_BATTLE", "IT_VITAMIN", "EV_ATK", "+10 EV de ataque"),
    ("IRON",         "BP_BATTLE", "IT_VITAMIN", "EV_DEF", "+10 EV de defensa"),
    ("CALCIUM",      "BP_BATTLE", "IT_VITAMIN", "EV_SPA", "+10 EV de at. especial"),
    ("ZINC",         "BP_BATTLE", "IT_VITAMIN", "EV_SPD", "+10 EV de def. especial"),
    ("CARBOS",       "BP_BATTLE", "IT_VITAMIN", "EV_SPE", "+10 EV de velocidad"),
    # Bolas. Todavia no hacen nada: no hay captura. Estan para que el bolsillo
    # exista de verdad y para no tener que tocar el formato cuando la haya.
    ("POKEBALL",     "BP_BALL", "IT_BALL",  10, "la de siempre"),
    ("GREATBALL",    "BP_BALL", "IT_BALL",  15, "x1,5"),
    ("ULTRABALL",    "BP_BALL", "IT_BALL",  20, "x2"),
    ("MASTERBALL",   "BP_BALL", "IT_BALL", 255, "no falla"),
    # Piedras: quien evoluciona con piedra NO evoluciona por nivel (ver
    # stones.h). El `arg` es la CLAVE de la piedra, y es por ahi por donde la
    # busca la tabla de evoluciones, nunca por indice. Casi todas usan su
    # tipo porque se leia solo; la Sun Stone no tiene tipo propio y usa
    # T_COUNT, que por definicion no choca con ninguno.
    ("FIRESTONE",    "BP_EVO", "IT_STONE", "T_FIRE",    ""),
    ("WATERSTONE",   "BP_EVO", "IT_STONE", "T_WATER",   ""),
    ("THUNDERSTONE", "BP_EVO", "IT_STONE", "T_ELEC",    ""),
    ("LEAFSTONE",    "BP_EVO", "IT_STONE", "T_GRASS",   ""),
    ("MOONSTONE",    "BP_EVO", "IT_STONE", "T_FAIRY",   ""),
    # Compartia clave con la Fire Stone, asi que hacia lo mismo y nunca se
    # la nombraba (la busqueda por clave encontraba antes a la otra). Con
    # Johto si tiene trabajo: Gloom->Bellossom y Sunkern->Sunflora.
    ("SUNSTONE",     "BP_EVO", "IT_STONE", "EVOI_SUN",  "la del sol"),
    # El Cable Link hace de "intercambio" para los que en los juegos solo
    # evolucionan cambiandolos de consola SIN objeto (Alakazam, Machamp,
    # Golem, Gengar). Aqui no hay con quien intercambiar, asi que el cable
    # ES el intercambio.
    ("LINKINGCORD",  "BP_EVO", "IT_STONE", "EVOI_CABLE", "hace de intercambio"),
    # Los que SI llevan objeto tienen el SUYO, como en el Anil: su
    # PBS/pokemon.txt dice "TradeItem,METALCOAT", no "cable". Usar el cable
    # para todos era inventarselo. La Metal Coat ya estaba como objeto de
    # llevar (+20% al acero); estas tres faltaban.
    ("KINGSROCK",    "BP_EVO", "IT_STONE", "EVOI_KINGSROCK",   "Politoed, Slowking"),
    ("DRAGONSCALE",  "BP_EVO", "IT_STONE", "EVOI_DRAGONSCALE", "Kingdra"),
    ("UPGRADE",      "BP_EVO", "IT_STONE", "EVOI_UPGRADE",     "Porygon2"),
    ("METALCOAT",    "BP_EVO", "IT_STONE", "EVOI_METALCOAT",   "Steelix, Scizor"),
    # Los tres de Hoenn. La Escama Abisal no es un adorno: Clamperl guarda
    # UN solo destino (Huntail, que ademas sube por nivel) y sin objeto la
    # otra rama se queda sin nadie que la apunte, o sea Gorebyss no sale de
    # ningun sitio.
    ("PRISMSCALE",   "BP_EVO", "IT_STONE", "EVOI_PRISMSCALE",  "Milotic"),
    ("DEEPSEATOOTH", "BP_EVO", "IT_STONE", "EVOI_SEATOOTH",    "Huntail"),
    ("DEEPSEASCALE", "BP_EVO", "IT_STONE", "EVOI_SEASCALE",    "Gorebyss"),
    # Los DIEZ de Sinnoh. Esa generacion casi no evoluciona por nivel: se
    # apoya en objetos, y sin ellos la mitad de la region no se completa.
    # Las tres piedras nuevas (Brillante, Noche, Alba) y la de Hielo son
    # piedras de verdad; las otras seis son cacharros de un solo bicho.
    ("SHINYSTONE",   "BP_EVO", "IT_STONE", "EVOI_SHINY",     "Togekiss, Roserade"),
    ("DUSKSTONE",    "BP_EVO", "IT_STONE", "EVOI_DUSK",      "Honchkrow, Mismagius"),
    ("DAWNSTONE",    "BP_EVO", "IT_STONE", "EVOI_DAWN",      "Gallade, Froslass"),
    ("ICESTONE",     "BP_EVO", "IT_STONE", "EVOI_ICESTONE",  "Glaceon"),
    ("PROTECTOR",    "BP_EVO", "IT_STONE", "EVOI_PROTECTOR", "Rhyperior"),
    ("ELECTIRIZER",  "BP_EVO", "IT_STONE", "EVOI_ELECTIRIZ", "Electivire"),
    ("MAGMARIZER",   "BP_EVO", "IT_STONE", "EVOI_MAGMARIZ",  "Magmortar"),
    ("DUBIOUSDISC",  "BP_EVO", "IT_STONE", "EVOI_DUBIOUS",   "Porygon-Z"),
    ("REAPERCLOTH",  "BP_EVO", "IT_STONE", "EVOI_REAPER",    "Dusknoir"),
    ("OVALSTONE",    "BP_EVO", "IT_STONE", "EVOI_OVAL",      "Chansey"),
    # Kalos solo pide dos, los de Aromatisse y Slurpuff (que en los juegos
    # son intercambio llevando el objeto; aqui, como todos, se usan).
    ("SACHET",       "BP_EVO", "IT_STONE", "EVOI_SACHET",    "Aromatisse"),
    ("WHIPPEDDREAM", "BP_EVO", "IT_STONE", "EVOI_NATA",      "Slurpuff"),
    # Los tres de Galar. Los dos pergaminos son los que parten a Urshifu
    # en sus dos estilos; sin ellos, Kubfu no llega a ninguna parte.
    ("BLACKAUGURITE",   "BP_EVO", "IT_STONE", "EVOI_AUGURITA", "Kleavor"),
    ("SCROLLOFDARKNESS","BP_EVO", "IT_STONE", "EVOI_ROLLOSCU", "Urshifu oscuro"),
    ("SCROLLOFWATERS",  "BP_EVO", "IT_STONE", "EVOI_ROLLOAGU", "Urshifu de agua"),
    # Clave: no se gastan y no se tiran.
    ("TOWNMAP",      "BP_KEY", "IT_KEY", 0, "de momento, un recuerdo"),
]


def ascii_plano(s):
    s = s.replace("\u2019", "'").replace("\u2013", "-")
    s = unicodedata.normalize("NFKD", s)
    s = "".join(c for c in s if not unicodedata.combining(c))
    return "".join(c for c in s if 32 <= ord(c) < 127).strip()


def nombres_es():
    """id del PBS -> nombre en espanol (la clave del diccionario traducido)."""
    out, cur = {}, None
    for ln in open(PBS, encoding="utf-8", errors="replace"):
        ln = ln.strip()
        m = re.match(r"^\[(.+)\]$", ln)
        if m:
            cur = m.group(1).split(",")[0]
            continue
        if cur and ln.startswith("Name") and not ln.startswith("NamePlural"):
            out.setdefault(cur, ln.split("=", 1)[1].strip())
    return out


# EN QUIEN SE USA CADA OBJETO DENTRO DEL COMBATE. No se decide aqui: lo
# dice el propio PBS en `BattleUse`, y es el dato que faltaba para que
# un Revivir se pudiera usar.
#
# Lo trajo el dueno jugando: "revive esta quebrado, nao tem como usar na
# batalha pois nao deixa eu escolher o pokemon". Y tenia razon —— la
# bolsa aplicaba TODO al que estaba en pista, asi que un Revivir no
# tenia a quien levantar: el de pista, por definicion, esta en pie.
#
#   OnPokemon  hay que ELEGIR a cual del equipo (pociones, curas,
#              revivires, elixires). Es la mayoria.
#   OnBattler  al que esta en pista y punto (las X y el Mas Psique):
#              suben escalones, y los escalones son del combate.
#   OnMove     elegir bicho y golpe (los Eteres). Aqui se elige bicho y
#              el golpe lo pone el juego —— el mas gastado ——, que es
#              una pantalla menos en una placa que se toca con el dedo.
#   OnFoe      las bolas, que ya tienen su propio camino.
#   (nada)     no se puede usar en combate (las vitaminas).
USO_EN_COMBATE = {
    "OnPokemon": "IU_POKEMON",
    "OnMove":    "IU_POKEMON",
    "OnBattler": "IU_BATTLER",
    "OnFoe":     "IU_FOE",
    "Direct":    "IU_BATTLER",
}


def usos_en_combate():
    """id del PBS -> IU_*, leido de `BattleUse`."""
    out, cur = {}, None
    for ln in open(PBS, encoding="utf-8", errors="replace"):
        ln = ln.strip()
        m = re.match(r"^\[(.+)\]$", ln)
        if m:
            cur = m.group(1).split(",")[0]
            continue
        if cur and ln.startswith("BattleUse"):
            v = ln.split("=", 1)[1].strip()
            out.setdefault(cur, USO_EN_COMBATE.get(v, "IU_NONE"))
    return out


# Pinta de espanol: si una descripcion las tiene DESPUES de traducir, es
# que no estaba en el diccionario y se embarcaria en espanol. Ya paso con
# los golpes (cuatro, incluido TACKLE, el primero que ve cualquiera) y
# por eso aqui se avisa a gritos en vez de dejarlo pasar.
PINTA_ES = (" el ", " la ", " los ", " las ", " del ", " que ", " para ",
            " con ", "cion", "Pokemon que", "aumenta", "Restaura")


def traduce_desc(ident, desc_es, dicc, espanol):
    es = desc_es.get(ident)
    if not es:
        return ""
    en = dicc.get(es, es)
    if en is es or en == es:
        if any(t in " " + en + " " for t in PINTA_ES):
            espanol.append(ident)
    return ascii_plano(en)


def descripciones_es():
    """id del PBS -> descripcion en espanol (la clave del diccionario)."""
    out, cur = {}, None
    for ln in open(PBS, encoding="utf-8", errors="replace"):
        ln = ln.strip()
        m = re.match(r"^\[(.+)\]$", ln)
        if m:
            cur = m.group(1).split(",")[0]
            continue
        if cur and ln.startswith("Description"):
            out.setdefault(cur, ln.split("=", 1)[1].strip())
    return out


def orden():
    """Los identificadores del PBS, EN EL ORDEN en que se emiten.

    Vive aparte porque main() lo necesita para escribir la tabla y
    gen_dex para saber que indice tiene cada objeto (los WildItem del
    Anil vienen por su clave del PBS, no por numero). Construirlo dos
    veces seria la forma segura de que un dia dejaran de coincidir.
    """
    return ([i for i, _f, _a, _c in LISTA]
            + [u[0] for u in USABLES]
            + [p[0] for p in piedras_mega()]
            + [c[0] for c in CHAVES]
            + [b[0] for b in BOLAS_EXTRA]
            + [p[0] for p in PREMIOS])


def indices():
    """{clave del PBS: indice en ITEM_TBL}. El 0 es "nada".

    Aplica EL MISMO filtro que main(): lo que no tiene nombre en el PBS
    no se emite, y por tanto no ocupa indice. Saltarselo aqui correria
    todos los numeros a partir del primer hueco — y el sintoma seria un
    Pikachu salvaje llevando una Poke Ball.
    """
    es = nombres_es()
    salen = [i for i in orden()
             if es.get(i) is not None or i in NOMBRE_INVENTADO]
    return {ident: i + 1 for i, ident in enumerate(salen)}


def main():
    _tablas = marshal_lite.load(DAT)
    dicc = _tablas[MT_NOMBRES_OBJ]
    dicc_desc = {k.decode("utf-8", "replace"): v.decode("utf-8", "replace")
                 for k, v in _tablas[MT_DESC_OBJ].items()
                 if isinstance(k, bytes) and isinstance(v, bytes)}
    desc_es = descripciones_es()
    dicc = {k.decode("utf-8", "replace"): v.decode("utf-8", "replace")
            for k, v in dicc.items()
            if isinstance(k, bytes) and isinstance(v, bytes)}
    es = nombres_es()
    usos = usos_en_combate()

    todos = ([(i, "BP_HELD", f, a, c) for i, f, a, c in LISTA] + USABLES
             + piedras_mega() + CHAVES + BOLAS_EXTRA + PREMIOS)
    # Que el orden de aqui sea EL MISMO que el de orden(), que es de
    # donde gen_dex saca los indices. Si un dia se anade algo a una de
    # las dos listas y no a la otra, aqui se para.
    assert [t[0] for t in todos] == orden(), \
        "orden() y main() ya no emiten los objetos en el mismo orden"
    filas, sin_traducir, faltan, espanol = [], [], [], []
    for ident, bolsillo, fx, arg, com in todos:
        nom_es = es.get(ident)
        if nom_es is None and ident in NOMBRE_INVENTADO:
            # Las dos piedras que no existen en el Anil porque las pusimos
            # nosotros: no hay nombre que buscar, lo traemos escrito.
            filas.append((NOMBRE_INVENTADO[ident], bolsillo, fx, arg, com,
                          traduce_desc(ident, desc_es, dicc_desc, espanol)
                          or DESC_INVENTADA.get(ident, ""),
                          ident, usos.get(ident, "IU_NONE")))
            continue
        if nom_es is None:
            faltan.append(ident)
            continue
        nom = NOMBRE_CORREGIDO.get(ident) or dicc.get(nom_es, nom_es)
        if nom_es not in dicc and ident not in NOMBRE_CORREGIDO:
            sin_traducir.append(ident)
        filas.append((ascii_plano(nom), bolsillo, fx, arg, com,
                      DESC_PROPRIA[ident][0] if ident in DESC_PROPRIA
                      else traduce_desc(ident, desc_es, dicc_desc, espanol),
                      # chave e enfeite: nunca entra na bolsa de combate
                      ident, "IU_NONE" if bolsillo == "BP_KEY"
                      else usos.get(ident, "IU_NONE")))
    if faltan:
        print(f"  no estan en el PBS y se saltan: {faltan}")

    if sin_traducir:
        print(f"  sin traducir, se quedan en espanol: {sin_traducir}")

    largo = max(len(f[0]) for f in filas)
    out = [
        "// GENERADO por tools/gen_items.py -- no editar a mano\n",
        "// La bolsa. La lista va a mano (el PBS tiene 889 objetos y aqui casi\n",
        "// ninguno significa nada: no hay tienda, ni mapa, ni equipo); de la\n",
        "// traduccion del propio Anil solo sale el nombre en ingles.\n",
        "//\n",
        "// Los bolsillos son los NUESTROS: el PBS mete Restos, piedras y\n",
        "// repelentes todos en el mismo, y aqui lo que importa es si un objeto se\n",
        "// LLEVA encima, se USA, o es una bola.\n",
        "#pragma once\n#include <stdint.h>\n",
        "#include \"dex.h\"      // T_*\n",
        "#include \"btstate.h\"  // ST_* y SG_*: un Antidoto necesita nombrarlos\n\n",
        "enum : uint8_t {\n  " + ", ".join(BOLSILLOS) + ", BP_COUNT,\n};\n\n",
        "enum : uint8_t {\n",
        "  IT_NONE = 0,\n",
        "  IT_LEFTOVERS,   // cura un dieciseisavo al cerrar el turno\n",
        "  IT_BERRY_HEAL,  // arg = % del maximo; se come sola por debajo de la mitad\n",
        "  IT_BERRY_FIX,   // igual, pero arg = PS FIJOS (la Aranja cura 10, no un %)\n",
        "  IT_BERRY_CURE,  // se come sola y quita el estado que haya\n",
        "  IT_FOCUS_SASH,  // estando ENTERO, aguanta un golpe mortal con 1 PS\n",
        "  IT_LIFE_ORB,    // arg = % de dano extra; cuesta un 10% de tu vida\n",
        "  IT_CHOICE,      // +50% (arg: 1 fisico, 0 especial) atado al primer golpe\n",
        "  IT_QUICK_CLAW,  // arg = % de salir primero aunque seas mas lento\n",
        "  IT_SCOPE_LENS,  // +1 al escalon de critico\n",
        "  IT_EVERSTONE,   // no evoluciona mientras lo lleve\n",
        "  IT_TYPE_BOOST,  // arg = T_*: +20% a los golpes de ese tipo\n",
        "  // --- los que se USAN, no se llevan ---\n",
        "  IT_POTION,      // arg = PS que cura (255 = del todo)\n",
        "  IT_FULLREST,    // PS al maximo y sin estado\n",
        "  IT_REVIVE,      // arg = % con el que levanta a un desmayado\n",
        "  IT_CURE,        // arg = ST_* que quita (0 = el que sea)\n",
        "  IT_ETHER,       // arg = PP a UN golpe (255 = al maximo)\n",
        "  IT_ELIXIR,      // idem a los cuatro\n",
        "  IT_XSTAT,       // arg = SG_*: +1 a ese stage, solo en combate\n",
        "  IT_XCRIT,       // +2 al escalon de critico\n",
        "  IT_BALL,        // arg = multiplicador x10. Aun no hace nada: no hay captura\n",
        "  IT_STONE,       // piedra de evolucion. Aqui se evoluciona por nivel, aun no hace nada\n",
        "  IT_KEY,         // no se gasta y no se tira\n",
        "  IT_VITAMIN,     // arg = EV_*: +10 a ese EV, hasta el tope\n"
        "  IT_MEGA,        // piedra mega. arg = MEGA_*: cual de las 121 es\n",
        "  IT_BALLX,       // bola de condicao. arg = BALLK_*: ballMult() decide\n",
        "  IT_RARECANDY,   // +1 nivel ao titular\n",
        "  IT_SHINY,       // o titular fica shiny (premio raro do quiz)\n",
        "};\n\n",
        "// QUE bola de condicao e (o `arg` de IT_BALLX).\n"
        "enum : uint8_t {\n  BALLK_NONE = 0,\n"
        + "".join(f"  {b[3]},\n" for b in BOLAS_EXTRA)
        + "};\n\n",
        "// EN QUIEN se usa dentro del combate. Sale del `BattleUse` del PBS\n"
        "// del Anil, no de una lista escrita a mano: es el dato que decide\n"
        "// si la bolsa tiene que preguntar a cual del equipo.\n"
        "enum : uint8_t {\n"
        "  IU_NONE = 0,   // en combate no se puede usar (las vitaminas)\n"
        "  IU_BATTLER,    // al que esta en pista, sin preguntar (las X)\n"
        "  IU_POKEMON,    // hay que ELEGIR a cual del equipo (pociones, revivir)\n"
        "  IU_FOE,        // contra el rival: las bolas, que van por su camino\n"
        "};\n\n",
        "// La CLAVE de cada objeto de evolucion (el `arg` de IT_STONE). Las\n"
        "// piedras usan su tipo porque se leia solo; los que no son de ningun\n"
        "// tipo van detras de T_COUNT, que por definicion no choca con uno.\n"
        "// Los nombres salen del PBS del Anil, que dice objeto por objeto cual\n"
        "// pide cada evolucion (\"TradeItem,METALCOAT\"): antes habia un solo\n"
        "// \"cable\" generico para todos, que era inventarselo.\n"
        "#define EVOI_CABLE       (T_NONE)\n"
        "#define EVOI_SUN         (T_COUNT)\n"
        "#define EVOI_KINGSROCK   (T_COUNT + 1)\n"
        "#define EVOI_DRAGONSCALE (T_COUNT + 2)\n"
        "#define EVOI_UPGRADE     (T_COUNT + 3)\n"
        "#define EVOI_METALCOAT   (T_COUNT + 4)\n"
        "#define EVOI_PRISMSCALE  (T_COUNT + 5)\n"
        "#define EVOI_SEATOOTH    (T_COUNT + 6)\n"
        "#define EVOI_SEASCALE    (T_COUNT + 7)\n"
        "#define EVOI_SHINY       (T_COUNT + 8)\n"
        "#define EVOI_DUSK        (T_COUNT + 9)\n"
        "#define EVOI_DAWN        (T_COUNT + 10)\n"
        "#define EVOI_ICESTONE    (T_COUNT + 11)\n"
        "#define EVOI_PROTECTOR   (T_COUNT + 12)\n"
        "#define EVOI_ELECTIRIZ   (T_COUNT + 13)\n"
        "#define EVOI_MAGMARIZ    (T_COUNT + 14)\n"
        "#define EVOI_DUBIOUS     (T_COUNT + 15)\n"
        "#define EVOI_REAPER      (T_COUNT + 16)\n"
        "#define EVOI_OVAL        (T_COUNT + 17)\n"
        "#define EVOI_SACHET      (T_COUNT + 18)\n"
        "#define EVOI_NATA        (T_COUNT + 19)\n"
        "#define EVOI_AUGURITA    (T_COUNT + 20)\n"
        "#define EVOI_ROLLOSCU    (T_COUNT + 21)\n"
        "#define EVOI_ROLLOAGU    (T_COUNT + 22)\n\n",
        "// Las piedras MEGA. Son 121 y van numeradas de corrido, no detras\n"
        "// de T_COUNT como las de evolucion: estas no comparten espacio con\n"
        "// nada porque su efecto (IT_MEGA) es solo suyo. El numero es lo que\n"
        "// megas.h guarda en MegaForm::item para saber cual es cual.\n"
        + "".join(f"#define MEGA_{re.sub(r'[^A-Z0-9]', '', p.upper()):<18s} {i}\n"
                  for i, (p, _b, _f, _a, _c) in enumerate(piedras_mega()))
        + "\n",
        "struct ItemEntry {\n"
        "  const char *name;  // ASCII, de la traduccion inglesa del propio Anil\n"
        "  uint8_t pocket;    // BP_*\n"
        "  uint8_t fx;        // IT_*\n"
        "  uint8_t arg;       // significa una cosa distinta en cada efecto\n"
        "  uint8_t use;       // IU_*: EN QUIEN se usa dentro del combate\n"
        "};\n\n",
        f"#define ITEM_COUNT {len(filas) + 1}\n\n",
        "static const ItemEntry ITEM_TBL[] = {\n",
        '  { "-", BP_HELD, IT_NONE, 0, IU_NONE },  // 0 = nada / hueco vacio\n',
    ]
    for nom, bolsillo, fx, arg, com, _d, ident, uso in filas:
        a = arg if isinstance(arg, str) else str(arg)
        # El id del PBS, escrito en la linea. No es decoracion: los
        # iconos del Anil se llaman asi (BLACKAUGURITE.png) y no por el
        # nombre de pantalla (Black Mineral), y sin esto gen_art.py no
        # los encuentra. Lo sabiamos aqui y no lo deciamos.
        coma = f'  // pbs:{ident}' + (f' {com}' if com else '')
        out.append(f'  {{ "{nom}",{" " * (largo - len(nom))} {bolsillo + ",":<11}'
                   f'{fx}, {a}, {uso} }},{coma}\n')
    out.append("};\n\n")

    # Las DESCRIPCIONES, en una tabla aparte y no dentro de ItemEntry:
    # asi la ficha de 4 bytes por objeto no engorda para los sitios que
    # solo quieren el nombre o el efecto. Salen del diccionario ingles
    # del propio Anil, el mismo de donde salen los nombres.
    out.append("// Que hace cada objeto, en ingles del propio Anil.\n")
    out.append("static const char *const ITEM_DESC[ITEM_COUNT] = {\n")
    out.append('  "",\n')
    for _n, _b, _f, _a, _c, d, _id, _u in filas:
        out.append('  "%s",\n' % d.replace('"', "'"))
    out.append("};\n\n")

    # ---------------- lo mismo en español y en portugués ----------------
    # El español es la Description del PBS tal cual (de donde el inglés se
    # traduce) y el portugués está escrito a mano en tools/pt_items.py,
    # indexado por el IDENT del PBS —— no por la fila, que se mueve cada
    # vez que se añade un objeto a la lista de arriba.
    for nom_tabla, fuente, quien in (
            ("ES", lambda i: DESC_PROPRIA[i][1] if i in DESC_PROPRIA
             else desc_es.get(i, ""), "el PBS del Anil"),
            ("PT", lambda i: PT_OBJETOS.get(i, ""), "tools/pt_items.py")):
        textos = [fuente(f[6]) for f in filas]
        malas = []
        for f, t in zip(filas, textos):
            if not t:
                continue
            try:
                texto_pt.codifica(t, f[6], importado=(nom_tabla == "ES"))
            except texto_pt.NoCabe as e:
                malas.append(str(e))
        if malas:
            print(f"  {len(malas)} descripciones {nom_tabla} con algun "
                  f"caracter que la fuente no tiene:")
            for m in malas[:12]:
                print(f"    {m}")
            raise SystemExit(1)
        hechas = sum(1 for t in textos if t)
        out.append(f"// Lo mismo en {nom_tabla}, de {quien}. Misma fila.\n")
        if nom_tabla == "PT":
            out.append("// nullptr = sin traducir: la pantalla cae al ingles.\n")
        out.append(f"static const char *const ITEM_DESC_{nom_tabla}"
                   f"[ITEM_COUNT] = {{\n")
        out.append('  "",\n')
        for t in textos:
            out.append("  nullptr,\n" if not t
                       else '  "%s",\n'
                       % texto_pt.codifica(t, importado=(nom_tabla == "ES")))
        out.append("};\n\n")
        print(f"  {nom_tabla}: {hechas}/{len(textos)} descripciones "
              f"({sum(len(t) for t in textos) / 1024:.1f} KB)")

    sobran = sorted(set(PT_OBJETOS) - {f[6] for f in filas})
    if sobran:
        print(f"  OJO: {len(sobran)} idents en pt_items.py que no son ningun "
              f"objeto embarcado: {', '.join(sobran[:8])}")

    out.append("static inline const ItemEntry &itemAt(uint8_t i) {\n"
               "  return ITEM_TBL[i < ITEM_COUNT ? i : 0];\n"
               "}\n\n"
               "static const char *const POCKET_NAME[BP_COUNT] = {\n"
               '  "Held", "Battle", "Balls", "Stones", "Key",\n'
               "};\n\n")

    # Las bolas, ordenadas de la mas floja a la mas fuerte. Quien REPARTE
    # bolas (el cuidado diario, el gimnasio) las nombra por escalon y no por
    # indice: si manana entra una bola nueva en medio, esta tabla se
    # reordena sola y no hay que ir a buscar numeros a mano por el codigo.
    bolas = sorted(((i + 1, n, arg)
                    for i, (n, b, fx, arg, _, _d, _id, _u) in enumerate(filas)
                    if b == "BP_BALL" and fx == "IT_BALL"),
                   key=lambda t: (t[2] if isinstance(t[2], int) else 0))
    out.append("// Las bolas de la mas floja a la mas fuerte (por su "
               "multiplicador).\n"
               "// Se nombran por ESCALON, nunca por indice: asi meter una "
               "bola nueva\n// no obliga a tocar el codigo que las reparte.\n")
    out.append(f"#define BALL_TIERS {len(bolas)}\n")
    out.append("static const uint8_t BALL_ID[BALL_TIERS] = { " +
               ", ".join(str(i) for i, _, _ in bolas) + " };  // " +
               ", ".join(n for _, n, _ in bolas) + "\n")
    # TODAS as bolas: as de escalao e depois as de condicao, na ordem de
    # BOLAS_EXTRA. E a ordem das artes de arremesso (gen_art.py le a
    # mesma lista), e a ficha do bicho usa o icone de qualquer uma.
    extra = [i + 1 for i, f in enumerate(filas) if f[2] == "IT_BALLX"]
    out.append("// Todas as bolas, na ordem das artes de arremesso (ART_BALL).\n")
    out.append(f"#define BALL_ALL_COUNT {len(bolas) + len(extra)}\n")
    out.append("static const uint8_t BALL_ALL[BALL_ALL_COUNT] = { " +
               ", ".join(str(i) for i in [b[0] for b in bolas] + extra) + " };\n")

    with open(DST, "w", encoding="utf-8") as f:
        f.write("".join(out))
    import collections as _c
    porBolsillo = _c.Counter(f[1] for f in filas)
    if espanol:
        print("  !! %d descripciones SIN TRADUCIR (irian en espanol): %s"
              % (len(espanol), ", ".join(espanol[:8])))
    sin_desc = sum(1 for f in filas if not f[5])
    print(f"guardado {DST}: {len(filas)} objetos, "
          f"{len(filas) - sin_desc} con descripcion")
    for b in BOLSILLOS:
        print(f"  {b:<10} {porBolsillo.get(b, 0):3}")


if __name__ == "__main__":
    main()
