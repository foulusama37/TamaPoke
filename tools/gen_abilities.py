#!/usr/bin/env python3
"""Genera abilities.h: las habilidades de la Pokedex y quien tiene cual.

Se emiten TODAS las que pueden salir (86 normales + 67 ocultas), aunque solo
una parte tenga mecanica: que un Rattata tenga Fuga y aqui no sirva de nada es
mejor que fingir que no la tiene. Las que no estan implementadas salen con
AB_NONE y se ven en la ficha igual.

Los nombres en ingles salen de la traduccion del propio Anil
(Data/messages_english_game.dat, MessageTypes indice 10). Las mecanicas van a
mano: en Essentials las habilidades son handlers de Ruby, no un FunctionCode
declarativo como los golpes.

  python3 tools/gen_abilities.py
"""
import os
import re
import sys
import unicodedata

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import marshal_lite  # noqa: E402
from dex_data import DEX  # noqa: E402
from dex_range import N_DEX  # noqa: E402

ANIL = r"C:\Users\foulu\Downloads\Pokemon Anil V4.13"
DAT = os.path.join(ANIL, "Data", "messages_english_game.dat")
PBS = os.path.join(ANIL, "PBS")
RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DST = os.path.join(RAIZ, "abilities.h")

MT_HAB = 10    # nombres de habilidad
MT_DESC = 11   # y sus descripciones (el .dat las guarda en tipos distintos)
SLUG = {d[1].upper(): d[0] for d in DEX}
SLUG["NIDORANFE"] = 29
SLUG["NIDORANMA"] = 32

# id del PBS -> (efecto, argumento). Lo que no este aqui sale AB_NONE: existe,
# se ve, y no hace nada. Elegidas por lo que cambia un combate de uno contra
# uno; lo de equipo, huida y mapa aqui no significa nada.
MECANICA = {
    # --- el cielo ---
    "SWIFTSWIM":    ("AB_SPEED_WEATHER", "WE_RAIN"),
    "CHLOROPHYLL":  ("AB_SPEED_WEATHER", "WE_SUN"),
    "SANDRUSH":     ("AB_SPEED_WEATHER", "WE_SAND"),
    "SLUSHRUSH":    ("AB_SPEED_WEATHER", "WE_HAIL"),
    "SANDVEIL":     ("AB_EVA_WEATHER",   "WE_SAND"),
    "SNOWCLOAK":    ("AB_EVA_WEATHER",   "WE_HAIL"),
    "RAINDISH":     ("AB_HEAL_WEATHER",  "WE_RAIN"),
    "ICEBODY":      ("AB_HEAL_WEATHER",  "WE_HAIL"),
    "SANDFORCE":    ("AB_NONE",          0),
    "DRIZZLE":      ("AB_SET_WEATHER",   "WE_RAIN"),
    "DROUGHT":      ("AB_SET_WEATHER",   "WE_SUN"),
    "SANDSTREAM":   ("AB_SET_WEATHER",   "WE_SAND"),
    "SNOWWARNING":  ("AB_SET_WEATHER",   "WE_HAIL"),
    # --- inmunidades y absorciones por tipo ---
    "LEVITATE":     ("AB_IMMUNE_TYPE",   "T_GROUND"),
    "FLASHFIRE":    ("AB_ABSORB_BOOST",  "T_FIRE"),
    "WATERABSORB":  ("AB_ABSORB_HEAL",   "T_WATER"),
    "VOLTABSORB":   ("AB_ABSORB_HEAL",   "T_ELEC"),
    "LIGHTNINGROD": ("AB_ABSORB_BOOST",  "T_ELEC"),
    "STORMDRAIN":   ("AB_ABSORB_BOOST",  "T_WATER"),
    "MOTORDRIVE":   ("AB_ABSORB_SPEED",  "T_ELEC"),
    "SAPSIPPER":    ("AB_ABSORB_ATK",    "T_GRASS"),
    "DRYSKIN":      ("AB_ABSORB_HEAL",   "T_WATER"),
    # STEAMVEIL es del MEGA ARCANINE que el Anil se invento, asi que no
    # hay juego con el que comparar: la descripcion de su PBS es la
    # especificacion. Son DOS cosas (sol al entrar, y el agua que se le
    # evapora encima) y la segunda encaja tal cual en las absorciones
    # por tipo, asi que el arg es el tipo que absorbe y el sol va aparte.
    "STEAMVEIL":    ("AB_STEAM_VEIL",    "T_WATER"),
    # --- la tanda por impacto: las que mas especies llevan ---
    # Las tres de abajo son del Anil y su descripcion del PBS es la
    # unica especificacion: CAMORRISTA sube las patadas un 20%,
    # SILVANO el dano de CUALQUIER tipo un 30% en campo de hierba, e
    # ILLUMINATE (aqui) baja la punteria del rival al entrar.
    "SOUNDPROOF":   ("AB_SOUNDPROOF",    0),
    "BULLETPROOF":  ("AB_BULLETPROOF",   0),
    "STRONGJAW":    ("AB_BITE_BOOST",    0),
    "CAMORRISTA":   ("AB_KICK_BOOST",    0),
    "SILVANO":      ("AB_FORESTRY",      0),
    "SUPERLUCK":    ("AB_CRIT_UP",       0),
    "SKILLLINK":    ("AB_SKILL_LINK",    0),
    "BEASTBOOST":   ("AB_BEAST_BOOST",   0),
    "JUSTIFIED":    ("AB_JUSTIFIED",     0),
    "QUICKFEET":    ("AB_QUICK_FEET",    0),
    "EARLYBIRD":    ("AB_EARLY_BIRD",    0),
    "ILLUMINATE":   ("AB_ILLUMINATE",    0),
    "CONTRARY":     ("AB_CONTRARY",      0),
    "POISONTOUCH":  ("AB_TOUCH_STATUS",  "ST_POISON"),
    # BURNTOUCH es del Anil y es el mismo toque con otro estado, asi
    # que AB_POISON_TOUCH paso a AB_TOUCH_STATUS con el estado en el
    # arg. Un codigo menos y una habilidad mas.
    "BURNTOUCH":    ("AB_TOUCH_STATUS",  "ST_BURN"),
    "CURSEDBODY":   ("AB_CURSED_BODY",   0),
    # --- segunda tanda por impacto ---
    # PODERGELIDO es del Anil: hielo y velocidad +50% con nieve.
    "PROTOSYNTHESIS": ("AB_PARADOX",     "WE_SUN"),
    "QUARKDRIVE":   ("AB_PARADOX_TR",    "TR_ELEC"),
    "HARVEST":      ("AB_HARVEST",       0),
    "NEUROFORCE":   ("AB_NEURO_FORCE",   0),
    "MOODY":        ("AB_MOODY",         0),
    "HEAVYMETAL":   ("AB_HEAVY_METAL",   0),
    "CORROSION":    ("AB_CORROSION",     0),
    "PODERGELIDO":  ("AB_GLACIAL",       0),
    "STENCH":       ("AB_STENCH",        0),
    "CLOUDNINE":    ("AB_CLOUD_NINE",    0),
    "ANGERPOINT":   ("AB_ANGER_POINT",   0),
    "FOREWARN":     ("AB_FOREWARN",      0),
    "SHARPNESS":    ("AB_SLICE_BOOST",   0),
    "PROTEAN":      ("AB_PROTEAN",       0),
    "SWEETVEIL":    ("AB_SWEET_VEIL",    0),
    "FLUFFY":       ("AB_FLUFFY",        0),
    "SOLARPOWER":   ("AB_SOLAR_POWER",   0),
    "BERSERK":      ("AB_BERSERK",       0),
    # --- tercera tanda: las que dejan a una especie SIN NINGUNA ---
    # Salen de tools/sin_habilidad.py, que lista los bichos cuyas TRES
    # ranuras (las dos normales y la oculta) estan sin hacer. Ahi no hay
    # una habilidad que desbloquee a veinte: son de firma, una por
    # bicho, asi que lo que se busca es compartir mecanica donde la
    # ficha del PBS dice literalmente lo mismo.
    #
    # Cuatro salen GRATIS, reusando codigo que ya existia:
    "AIRLOCK":      ("AB_CLOUD_NINE",     0),
    "FULLMETALBODY": ("AB_CLEAR_BODY",    0),
    "SHADOWSHIELD": ("AB_MULTISCALE",     0),
    "SOULHEART":    ("AB_MOXIE",          "SG_SPA"),
    # Los que cambian el TIPO de sus golpes normales y le suben un 20%.
    "PIXILATE":     ("AB_TYPE_SWAP",      "T_FAIRY"),
    "REFRIGERATE":  ("AB_TYPE_SWAP",      "T_ICE"),
    "AERILATE":     ("AB_TYPE_SWAP",      "T_FLYING"),
    "GALVANIZE":    ("AB_TYPE_SWAP",      "T_ELEC"),
    "NORMALIZE":    ("AB_NORMALIZE",      0),
    # TINTINEO es del Anil: campanada al equipo al entrar, y sus golpes
    # de sonido pasan a psiquicos con un 20% mas.
    "TINTINEO":     ("AB_HALLOWED",       0),
    # Un tipo mas fuerte para TODOS (las auras) o solo para el suyo.
    "FAIRYAURA":    ("AB_AURA",           "T_FAIRY"),
    "DARKAURA":     ("AB_AURA",           "T_DARK"),
    "STEELWORKER":  ("AB_TYPE_X15",       "T_STEEL"),
    "DRAGONSMAW":   ("AB_TYPE_X15",       "T_DRAGON"),
    "TRANSISTOR":   ("AB_TYPE_X13",       "T_ELEC"),
    "MEGALAUNCHER": ("AB_PULSE_BOOST",    0),
    # Los cuatro Tesoros Funestos: el mismo codigo, distinto stat.
    "TABLETSOFRUIN": ("AB_RUIN",          "SG_ATK"),
    "SWORDOFRUIN":  ("AB_RUIN",           "SG_DEF"),
    "VESSELOFRUIN": ("AB_RUIN",           "SG_SPA"),
    "BEADSOFRUIN":  ("AB_RUIN",           "SG_SPD"),
    # Mitades de dano
    "FURCOAT":      ("AB_HALVE_CLASS",    1),
    "ICESCALES":    ("AB_HALVE_CLASS",    0),
    "HEATPROOF":    ("AB_HALVE_TYPE",     "T_FIRE"),
    "PRISMARMOR":   ("AB_ANTI_SE",        0),
    "WONDERGUARD":  ("AB_WONDER_GUARD",   0),
    # Al entrar
    "INTREPIDSWORD": ("AB_ENTRY_STAGE",   "SG_ATK"),
    "DAUNTLESSSHIELD": ("AB_ENTRY_STAGE", "SG_DEF"),
    "DOWNLOAD":     ("AB_DOWNLOAD",       0),
    "IMPOSTER":     ("AB_IMPOSTER",       0),
    # Lo demas, una por una
    "TRUANT":       ("AB_TRUANT",         0),
    "SLOWSTART":    ("AB_SLOW_START",     0),
    "DEFEATIST":    ("AB_DEFEATIST",      0),
    "WONDERSKIN":   ("AB_WONDER_SKIN",    0),
    "MYCELIUMMIGHT": ("AB_MYCELIUM",      0),
    "MUMMY":        ("AB_MUMMY",          0),
    "WANDERINGSPIRIT": ("AB_SPIRIT_SWAP", 0),
    "BADDREAMS":    ("AB_BAD_DREAMS",     0),
    "VICTORYSTAR":  ("AB_ACC_UP",         10),
    "GOODASGOLD":   ("AB_GOOD_AS_GOLD",   0),
    "GUARDDOG":     ("AB_GUARD_DOG",      0),
    "MAGNETPULL":   ("AB_TRAP_TYPE",      "T_STEEL"),
    "COMATOSE":     ("AB_COMATOSE",       0),
    "TOXICCHAIN":   ("AB_TOXIC_CHAIN",    0),
    "POISONPUPPETEER": ("AB_POISON_PUPPET", 0),
    "WIMPOUT":      ("AB_WIMP_OUT",       0),
    "EMERGENCYEXIT": ("AB_WIMP_OUT",      0),
    # Las tres ultimas que no piden un sistema de FORMAS nuevo.
    "UNSEENFIST":   ("AB_UNSEEN_FIST",   0),
    "ORICHALCUMPULSE": ("AB_ORICHALCUM", 0),
    "HADRONENGINE": ("AB_HADRON",        0),
    # --- las "de forma" ---
    # Lo que decia aqui ("no tienen forma alternativa en el PBS del
    # Anil") era FALSO: lo miraba en forms.h, que solo trae las
    # regionales. El PBS si las trae (CASTFORM,1..3, AEGISLASH,1...) y
    # desde la leva 70 viven en formas_combate.h, con su dibujo.
    "ZENMODE":      ("AB_ZEN_MODE",      0),
    "FORECAST":     ("AB_FORECAST",      0),
    "MULTITYPE":    ("AB_TYPE_FROM_ITEM", 0),
    "RKSSYSTEM":    ("AB_TYPE_FROM_ITEM", 0),
    "FLOWERGIFT":   ("AB_SUN_BOOST",     0),
    "STANCECHANGE": ("AB_STANCE",        0),
    "POWERCONSTRUCT": ("AB_HP_FORM",     1),
    "SHIELDSDOWN":  ("AB_HP_FORM",       1),
    "SCHOOLING":    ("AB_HP_FORM",       0),
    "ZEROTOHERO":   ("AB_HERO",          0),
    "TERASHIFT":    ("AB_TERA_SHIFT",    0),
    "DISGUISE":     ("AB_DISGUISE",      0),
    "GULPMISSILE":  ("AB_GULP",          0),
    "HUNGERSWITCH": ("AB_HUNGER",        0),
    "ILLUSION":     ("AB_ILLUSION",      0),
    # BAILARIN lo habia metido yo en el cajon de "solo con compañero"
    # y estaba MAL: su ficha dice "otro Pokemon presente en el
    # combate", y en un uno contra uno ese otro es el RIVAL. Copia
    # sus bailes, y eso si se puede hacer.
    "DANCER":       ("AB_DANCER",        0),
    # BATERIA y PUNTO FUERTE potencian a los ALIADOS, y aqui no hay
    # companero en la pista... pero SI hay banquillo. El usuario lo
    # resolvio como lo que tiene sentido en este motor: lo que dejan
    # al SIGUIENTE que entre. Bateria, sus golpes especiales; Punto
    # Fuerte, todos (es lo que dice su ficha: "los movimientos").
    "BATTERY":      ("AB_RELAY_BOOST",   1),
    "POWERSPOT":    ("AB_RELAY_BOOST",   2),
    # CURA, con la misma lectura: cura el estado al SIGUIENTE que
    # entre. Su ficha dice "los problemas de estado de un companero",
    # y en este motor el companero es el del banquillo.
    "HEALER":       ("AB_RELAY_CURE",    0),
    # COMANDANTE: en los juegos el Tatsugiri se mete en la boca de un
    # Dondozo aliado y es EL DONDOZO el que gana +2 en todo; el
    # Tatsugiri se va del campo. Aqui no hay aliado, asi que tomarlo
    # a la letra lo dejaria sin poder actuar NUNCA: peor que no tener
    # habilidad. Lo que si dice su ficha es "para tomar el control",
    # y sin segundo cuerpo el que manda es el propio Tatsugiri.
    #
    # El +2 va solo a lo que el M.Tatsugiri USA (135 de especial y 92
    # de velocidad). Darle tambien las defensas seria regalarle el
    # aguante que no tiene -68 de PS- y romper el equilibrio: el
    # +2 del juego real lo recibe un Dondozo, que es un muro que no
    # ataca, no un atacante especial.
    "COMMANDER":    ("AB_COMMANDER",     0),
    # --- las de las MEGAS ---
    # 30 de las 127 megas ganaban una habilidad que no hacia nada, y
    # megaevolucionar es el momento grande del combate. Cinco salen
    # GRATIS porque ya existia su mecanica con otro nombre:
    "FILTER":       ("AB_ANTI_SE",       0),
    "SOLIDROCK":    ("AB_ANTI_SE",       0),
    "LIBERO":       ("AB_PROTEAN",       0),
    "SOBRECARGA":   ("AB_PINCH_TYPE",    "T_ELEC"),
    "SHADOWTAG":    ("AB_TRAP_TYPE",     0),
    # ...y el resto, por lo que mas megas arreglan:
    "TOUGHCLAWS":   ("AB_CONTACT_BOOST", 0),
    "MAGICBOUNCE":  ("AB_MAGIC_BOUNCE",  0),
    "TRACE":        ("AB_TRACE",         0),
    "PARENTALBOND": ("AB_PARENTAL",      0),
    "PERISHBODY":   ("AB_PERISH_BODY",   0),
    "WINDRIDER":    ("AB_WIND_RIDER",    0),
    "DELTASTREAM":  ("AB_DELTA_STREAM",  0),
    "QUEENLYMAJESTY": ("AB_QUEENLY",     0),
    "PUNKROCK":     ("AB_PUNK_ROCK",     0),
    "THERMALEXCHANGE": ("AB_THERMAL",    0),
    "STURDYROOT":   ("AB_DEEP_ROOT",     0),
    # Los cuatro suelos de entrada, que son el Drought del terreno.
    "GRASSYSURGE":  ("AB_SET_TERRAIN",   "TR_GRASS"),
    "ELECTRICSURGE": ("AB_SET_TERRAIN",  "TR_ELEC"),
    "MISTYSURGE":   ("AB_SET_TERRAIN",   "TR_MISTY"),
    "PSYCHICSURGE": ("AB_SET_TERRAIN",   "TR_PSY"),
    "THICKFAT":     ("AB_HALVE_FIRE_ICE", 0),
    # --- al tocarle ---
    "POISONPOINT":  ("AB_CONTACT_STATUS", "ST_POISON"),
    "STATIC":       ("AB_CONTACT_STATUS", "ST_PARALYSIS"),
    "FLAMEBODY":    ("AB_CONTACT_STATUS", "ST_BURN"),
    "EFFECTSPORE":  ("AB_CONTACT_STATUS", "ST_SLEEP"),
    "CUTECHARM":    ("AB_CONTACT_CONFUSE", 0),
    "ROUGHSKIN":    ("AB_CONTACT_HURT",  8),
    "IRONBARBS":    ("AB_CONTACT_HURT",  8),
    # --- potencia ---
    "OVERGROW":     ("AB_PINCH_TYPE",    "T_GRASS"),
    "BLAZE":        ("AB_PINCH_TYPE",    "T_FIRE"),
    "TORRENT":      ("AB_PINCH_TYPE",    "T_WATER"),
    "SWARM":        ("AB_PINCH_TYPE",    "T_BUG"),
    "TECHNICIAN":   ("AB_TECHNICIAN",    60),
    "GUTS":         ("AB_GUTS",          0),
    "HUGEPOWER":    ("AB_HUGE_POWER",    0),
    "PUREPOWER":    ("AB_HUGE_POWER",    0),
    "ADAPTABILITY": ("AB_ADAPTABILITY",  0),
    "SHEERFORCE":   ("AB_SHEER_FORCE",   0),
    "IRONFIST":     ("AB_PUNCH_BOOST",   0),
    "HUSTLE":       ("AB_HUSTLE",        0),
    "TINTEDLENS":   ("AB_TINTED_LENS",   0),
    "SNIPER":       ("AB_SNIPER",        0),
    "RIVALRY":      ("AB_RIVALRY",       0),
    "ROCKHEAD":     ("AB_NO_RECOIL",     0),
    # ACOMETIDA es del Anil, no de los juegos: "el primer turno que
    # sale a combatir, +50% de Velocidad y +20% de Ataque".
    "ACOMETIDA":    ("AB_BLITZ",         0),
    # --- defensa ---
    "SHELLARMOR":   ("AB_NO_CRIT",       0),
    "BATTLEARMOR":  ("AB_NO_CRIT",       0),
    "STURDY":       ("AB_STURDY",        0),
    "MULTISCALE":   ("AB_MULTISCALE",    0),
    "SHIELDDUST":   ("AB_NO_SECONDARY",  0),
    "MARVELSCALE":  ("AB_MARVELSCALE",   0),
    "WEAKARMOR":    ("AB_WEAK_ARMOR",    0),
    "OVERCOAT":     ("AB_OVERCOAT",      0),
    "INFILTRATOR":  ("AB_INFILTRATOR",   0),
    "MOLDBREAKER":  ("AB_MOLD_BREAKER",  0),
    "TURBOBLAZE":   ("AB_MOLD_BREAKER",  0),
    "TERAVOLT":     ("AB_MOLD_BREAKER",  0),
    # --- estados ---
    "IMMUNITY":     ("AB_IMMUNE_STATUS", "ST_POISON"),
    "LIMBER":       ("AB_IMMUNE_STATUS", "ST_PARALYSIS"),
    "INSOMNIA":     ("AB_IMMUNE_STATUS", "ST_SLEEP"),
    "VITALSPIRIT":  ("AB_IMMUNE_STATUS", "ST_SLEEP"),
    "WATERVEIL":    ("AB_IMMUNE_STATUS", "ST_BURN"),
    "MAGMAARMOR":   ("AB_IMMUNE_STATUS", "ST_FREEZE"),
    "OBLIVIOUS":    ("AB_NO_CONFUSE",    0),
    "OWNTEMPO":     ("AB_NO_CONFUSE",    0),
    "INNERFOCUS":   ("AB_NO_FLINCH",     0),
    "SHEDSKIN":     ("AB_SHED_SKIN",     30),
    "NATURALCURE":  ("AB_SHED_SKIN",     100),
    "SYNCHRONIZE":  ("AB_SYNCHRONIZE",   0),
    "POISONHEAL":   ("AB_POISON_HEAL",   0),
    "HYDRATION":    ("AB_CURE_WEATHER",  "AB_WE_RAIN"),
    "ICEFACE":      ("AB_ICE_FACE",      0),
    "REGENERATOR":  ("AB_REGENERATOR",   0),
    # --- al entrar, y varios ---
    "INTIMIDATE":   ("AB_INTIMIDATE",    0),
    "PRESSURE":     ("AB_PRESSURE",      0),
    "KEENEYE":      ("AB_KEEN_EYE",      0),
    "HYPERCUTTER":  ("AB_NO_LOWER_ATK",  0),
    "BIGPECKS":     ("AB_NO_LOWER_DEF",  0),
    "CLEARBODY":    ("AB_CLEAR_BODY",    0),
    "WHITESMOKE":   ("AB_CLEAR_BODY",    0),
    "SPEEDBOOST":   ("AB_SPEED_BOOST",   0),
    "COMPOUNDEYES": ("AB_COMPOUND_EYES", 30),
    "NOGUARD":      ("AB_NO_GUARD",      0),
    "SERENEGRACE":  ("AB_SERENE_GRACE",  0),
    "PRANKSTER":    ("AB_PRANKSTER",     0),
    "UNNERVE":      ("AB_UNNERVE",       0),
    "ASONE":        ("AB_UNNERVE",       0),
    "RUNAWAY":      ("AB_RUN_AWAY",      0),
    "FRISK":        ("AB_FRISK",         0),
    # --- tercera tanda ---
    "RECKLESS":     ("AB_RECKLESS",      0),
    "RATTLED":      ("AB_RATTLED",       0),
    "MOXIE":        ("AB_MOXIE",         "SG_ATK"),
    "CHILLINGNEIGH": ("AB_MOXIE",        "SG_ATK"),
    "GRIMNEIGH":    ("AB_MOXIE",         "SG_SPA"),
    "DEFIANT":      ("AB_SPITE_STAT",    "SG_ATK"),
    "COMPETITIVE":  ("AB_SPITE_STAT",    "SG_SPA"),
    "DAMP":         ("AB_DAMP",          0),
    "UNAWARE":      ("AB_UNAWARE",       0),
    "SANDFORCE":    ("AB_SAND_FORCE",    0),
    "SCRAPPY":      ("AB_SCRAPPY",       0),
    "MAGICGUARD":   ("AB_MAGIC_GUARD",   0),
    "GOOEY":        ("AB_GOOEY",         0),
    "TANGLINGHAIR": ("AB_GOOEY",         0),
    "STEADFAST":    ("AB_STEADFAST",     0),
    "ANALYTIC":     ("AB_ANALYTIC",      0),
    "UNBURDEN":     ("AB_UNBURDEN",      0),
    # TELEPATHY en los juegos esquiva el golpe de tu COMPANERO, y aqui
    # no hay combates dobles. Decision del usuario: que valga al
    # RELEVAR. Lee el movimiento del rival y entra sin comerse el
    # golpe gratis que normalmente te llevas al cambiar.
    "TELEPATHY":    ("AB_TELEPATHY",     0),
    # Glotoneria: adelanta la baya de 1/4 a 1/2 de vida. Durante un
    # tiempo se quedo fuera porque el umbral de aqui YA era 1/2 y no
    # tenia nada que adelantar; entro bajando el umbral general al
    # cuarto, que ademas es el de los juegos. Ver battle.cpp.
    "GLUTTONY":     ("AB_GLUTTONY",      0),
    # --- cuarta tanda ---
    # Pickup aqui NO es la del combate: es la de la hierba, que es la
    # que todo el mundo recuerda. De vez en cuando aparece con algo en
    # la boca y va a la bolsa. Vive en pet.cpp, fuera del combate.
    "PICKUP":       ("AB_PICKUP",        0),
    "PICKPOCKET":   ("AB_PICKPOCKET",    0),
    "MAGICIAN":     ("AB_PICKPOCKET",    0),
    "ANTICIPATION": ("AB_ANTICIPATION",  0),
    "LEAFGUARD":    ("AB_GUARD_WEATHER", "AB_WE_SUN"),
    # VENTOSAS: Rugido, Remolino, Cola Dragao e Llave Giro nao a tiram
    # da pista (so conta com banco: a batalha entre placas).
    "SUCTIONCUPS":  ("AB_SUCTION_CUPS",  0),
}


def ascii_plano(s):
    s = s.replace("\u2019", "'").replace("\u2013", "-")
    s = unicodedata.normalize("NFKD", s)
    s = "".join(c for c in s if not unicodedata.combining(c))
    return "".join(c for c in s if 32 <= ord(c) < 127).strip()


def secciones(path):
    out, cur, dic = [], None, {}
    for ln in open(path, encoding="utf-8", errors="replace"):
        ln = ln.strip()
        if not ln or ln.startswith("#"):
            continue
        m = re.match(r"^\[(.+)\]$", ln)
        if m:
            if cur:
                out.append((cur, dic))
            cur, dic = m.group(1), {}
            continue
        if "=" in ln and cur:
            k, v = ln.split("=", 1)
            dic[k.strip()] = v.strip()
    if cur:
        out.append((cur, dic))
    return out


def catalogo():
    """({dex: (normales, ocultas)}, [ids en el orden de ABIL_TBL]).

    Vive fuera de main() porque gen_megas.py necesita EXACTAMENTE el mismo
    orden para saber en que fila cayo cada habilidad. Lo intento leyendo
    los nombres del abilities.h generado y no valio: ahi estan los nombres
    TRADUCIDOS (SOBRECARGA sale con su nombre ingles) y la busqueda por
    nombre no encontraba nada. El indice se calcula, no se adivina — es el
    mismo fallo que ya mordio en gen_evos con "Sweet Cream".
    """
    porDex = {}
    for nombre, d in secciones(os.path.join(PBS, "pokemon.txt")):
        dex = SLUG.get(nombre.split(",")[0].upper())
        if not dex or dex > N_DEX or dex in porDex:
            continue
        ab = [a.strip() for a in d.get("Abilities", "").split(",") if a.strip()]
        oc = [a.strip() for a in
              d.get("HiddenAbilities", d.get("HiddenAbility", "")).split(",")
              if a.strip()]
        porDex[dex] = (ab, oc)

    # Y las de las MEGAS y de las FORMAS, que casi siempre son otra
    # distinta de la del bicho normal (Charizard X pasa a Dura Garra, el
    # Meowth de Galar a Uñas Fuertes). Van en el mismo catalogo porque
    # megas.h y forms.h indexan esta misma tabla; si no estuvieran, esos
    # bichos saldrian sin habilidad y no se notaria hasta jugarlos.
    deFormas = set()

    def mete(cab, d):
        base = SLUG.get(cab.split(",")[0].upper())
        if not base or base > N_DEX:
            return
        deFormas.update(a.strip() for a in d.get("Abilities", "").split(",")
                        if a.strip())

    for cab, d in secciones(os.path.join(PBS, "pokemon_forms.txt")):
        fn = d.get("FormName", "")
        if "Mega" in fn or "Primal" in fn:
            mete(cab, d)
    # el MISMO filtro que usa gen_forms, para no tener dos criterios
    import anil_pbs
    for cab, d in anil_pbs.formas_incluidas():
        mete(cab, d)

    return porDex, sorted({a for ab, oc in porDex.values() for a in ab + oc}
                          | deFormas)


# Las descripciones que el .dat del Anil NO traduce. Igual que en
# gen_moves.py: la traduccion va por la frase entera y una frase que no
# este en el diccionario se embarcaba en español, en silencio y en la
# ficha de la habilidad. Si el Anil la traduce algun dia, esta entrada
# sobra; mientras, auditoria.py vigila que no se cuele ninguna.
DESC_MANO = {
    "SPEEDBOOST": "Raises the Pokemon's Speed by one stage at the end of "
                  "each turn.",
}

# Palabras de funcion que no existen como palabra suelta en ingles. Hacen
# falta porque "falta en el diccionario" NO es lo mismo que "sale en
# español": las habilidades propias del Anil (Steam Veil y cia) ya traen
# su descripcion EN INGLES en el PBS, asi que no estan en el diccionario
# y no hay nada que arreglar. Lo que se busca es texto español que se
# quedo sin traducir, no cualquier hueco.
ES_PALABRAS = (" el ", " la ", " los ", " las ", " del ", " que ", " con ",
               " su ", " sus ", " se ", " lo ", " al ", " un ", " una ",
               " por ", " para ", "usuario", "objetivo", " puede ")


def parece_espanol(s):
    b = " " + (s or "").lower() + " "
    return any(p in b for p in ES_PALABRAS)


def indices():
    """{ID interno del PBS: fila en ABIL_TBL}. 0 = ninguna."""
    return {a: i + 1 for i, a in enumerate(catalogo()[1])}


def main():
    hab = {n: d for n, d in secciones(os.path.join(PBS, "abilities.txt"))}
    dicc = marshal_lite.load(DAT)[MT_HAB]
    dicc = {k.decode("utf-8", "replace"): v.decode("utf-8", "replace")
            for k, v in dicc.items()
            if isinstance(k, bytes) and isinstance(v, bytes)}

    porDex, usadas = catalogo()
    idx = {a: i + 1 for i, a in enumerate(usadas)}   # 0 = ninguna

    sinMec = [a for a in usadas if a not in MECANICA]
    print(f"{len(usadas)} habilidades entre las {N_DEX}")
    print(f"  con mecanica: {len(usadas) - len(sinMec)}")
    print(f"  solo el nombre: {len(sinMec)}")

    largo = max(len(ascii_plano(dicc.get(hab.get(a, {}).get('Name', a), a)))
                for a in usadas)
    out = [
        "// GENERADO por tools/gen_abilities.py -- no editar a mano\n",
        "// Se emiten TODAS las que pueden salir en la dex, aunque solo una parte\n",
        "// tenga mecanica: que un Rattata tenga Fuga y aqui no sirva de nada es\n",
        "// mejor que fingir que no la tiene. Los nombres vienen de la traduccion\n",
        "// inglesa del propio Anil; las mecanicas estan escritas a mano.\n",
        "#pragma once\n#include <stdint.h>\n",
        '#include "dex.h"      // T_*\n',
        '#include "btstate.h"  // ST_*\n\n',
        "// El cielo se declara en battle.h, que llega despues. Se repite el enum\n"
        "// aqui no: se usan los numeros por nombre desde alli.\n"
        "enum : uint8_t { AB_WE_NONE = 0, AB_WE_SUN, AB_WE_RAIN, AB_WE_SAND,\n"
        "                 AB_WE_HAIL };\n\n",
        "enum : uint8_t {\n",
        "  AB_NONE = 0,\n",
        "  AB_SPEED_WEATHER,   // arg = clima: velocidad x2 mientras dure\n",
        "  AB_EVA_WEATHER,     // arg = clima: +1 de evasion\n",
        "  AB_HEAL_WEATHER,    // arg = clima: se cura 1/16 por turno\n",
        "  AB_SET_WEATHER,     // arg = clima: lo pone al entrar\n",
        "  AB_IMMUNE_TYPE,     // arg = T_*: ese tipo no le hace nada\n",
        "  AB_ABSORB_HEAL,     // arg = T_*: en vez de dano, se cura\n",
        "  AB_ABSORB_BOOST,    // arg = T_*: en vez de dano, +1 at. especial\n",
        "  AB_ABSORB_SPEED,    // arg = T_*: en vez de dano, +1 velocidad\n",
        "  AB_ABSORB_ATK,      // arg = T_*: en vez de dano, +1 ataque\n",
        "  AB_STEAM_VEIL,      // arg = T_*: lo evapora (+1 evasion) y saca el sol\n",
        "  AB_SOUNDPROOF,       // los golpes de SONIDO no le entran\n",
        "  AB_BULLETPROOF,      // ni los de bomba, bola o canon\n",
        "  AB_BITE_BOOST,       // +50% a los mordiscos (MV_BITE)\n",
        "  AB_KICK_BOOST,       // +20% a las patadas (del Anil)\n",
        "  AB_FORESTRY,         // +30% a TODO sobre hierba (del Anil)\n",
        "  AB_CRIT_UP,          // +1 al escalon de critico\n",
        "  AB_SKILL_LINK,       // los multigolpe siempre al maximo\n",
        "  AB_BEAST_BOOST,      // al tumbar, +1 a su stat MAS ALTO\n",
        "  AB_JUSTIFIED,        // un golpe siniestro le sube el ataque\n",
        "  AB_QUICK_FEET,       // +50% de velocidad si esta mal, sin el corte\n",
        "  AB_EARLY_BIRD,       // duerme la mitad de turnos\n",
        "  AB_ILLUMINATE,       // al entrar, -1 a la punteria del rival\n",
        "  AB_CONTRARY,         // los cambios de stat se le invierten\n",
        "  AB_TOUCH_STATUS,     // arg = ST_*: 30% de ponerlo a quien TOCA\n",
        "  AB_CURSED_BODY,      // 30% de anular el golpe que le acaba de dar\n",
        "  AB_SET_TERRAIN,      // arg = TR_*: pone ese suelo al entrar\n",
        "  AB_PARADOX,          // arg = clima: con el, +30% a su stat mas alto\n",
        "  AB_PARADOX_TR,       // arg = TR_*: lo mismo, pero con suelo\n",
        "  AB_HARVEST,          // al cerrar el turno, recupera la baya comida\n",
        "  AB_NEURO_FORCE,      // +25% a los golpes que le van bien\n",
        "  AB_MOODY,            // cada turno, +2 a un stat al azar y -1 a otro\n",
        "  AB_HEAVY_METAL,      // pesa el doble para los golpes de peso\n",
        "  AB_CORROSION,        // envenena hasta a los de acero y veneno\n",
        "  AB_GLACIAL,          // con nieve, +50% al hielo y a la velocidad\n",
        "  AB_STENCH,           // 10% de amedrentar con cualquier golpe\n",
        "  AB_CLOUD_NINE,       // el cielo deja de contar para los dos\n",
        "  AB_ANGER_POINT,      // un critico le pone el ataque al maximo\n",
        "  AB_FOREWARN,         // al entrar, ve el golpe mas fuerte del rival\n",
        "  AB_SLICE_BOOST,      // +50% a los cortantes (MV_SLICE)\n",
        "  AB_PROTEAN,          // se vuelve del tipo del golpe que va a usar\n",
        "  AB_SWEET_VEIL,       // no le duermen, ni el a si mismo\n",
        "  AB_FLUFFY,           // la mitad del contacto, el doble del fuego\n",
        "  AB_SOLAR_POWER,      // al sol pega mas y se quema un octavo\n",
        "  AB_BERSERK,          // bajar de media vida le sube el especial\n",
        "  AB_TYPE_SWAP,        // arg = T_*: sus normales pasan a ese tipo, +20%\n",
        "  AB_NORMALIZE,        // TODOS sus golpes pasan a normal, +20%\n",
        "  AB_HALLOWED,         // del Anil: campanada al entrar y sonido psiquico\n",
        "  AB_AURA,             // arg = T_*: ese tipo pega 1/3 mas, para los DOS\n",
        "  AB_TYPE_X15,         // arg = T_*: sus golpes de ese tipo, +50%\n",
        "  AB_TYPE_X13,         // arg = T_*: ...+30%\n",
        "  AB_PULSE_BOOST,      // +50% a los de pulso y aura (MV_PULSE)\n",
        "  AB_RUIN,             // arg = SG_*: baja ese stat del OTRO un 25%\n",
        "  AB_HALVE_CLASS,      // arg 1 = la mitad del fisico, 0 = del especial\n",
        "  AB_HALVE_TYPE,       // arg = T_*: la mitad de ese tipo\n",
        "  AB_ANTI_SE,          // un cuarto menos de lo que le va mal\n",
        "  AB_WONDER_GUARD,     // solo le entra lo supereficaz\n",
        "  AB_ENTRY_STAGE,      // arg = SG_*: +1 la PRIMERA vez que sale\n",
        "  AB_DOWNLOAD,         // al entrar, +1 al ataque que le vaya mejor\n",
        "  AB_IMPOSTER,         // al entrar se transforma en el de enfrente\n",
        "  AB_TRUANT,           // un turno si y otro no\n",
        "  AB_SLOW_START,       // cinco turnos a mitad de ataque y velocidad\n",
        "  AB_DEFEATIST,        // por debajo de media vida, mitad de ataques\n",
        "  AB_WONDER_SKIN,      // los golpes de estado le llegan al 50%\n",
        "  AB_MYCELIUM,         // sus golpes de estado van ultimos y no los paran\n",
        "  AB_MUMMY,            // al que le toca le pega SU habilidad\n",
        "  AB_SPIRIT_SWAP,      // al que le toca le cambia la habilidad\n",
        "  AB_BAD_DREAMS,       // al rival dormido le roe un octavo por turno\n",
        "  AB_ACC_UP,           // arg = % de punteria extra para el\n",
        "  AB_GOOD_AS_GOLD,     // los golpes de ESTADO no le entran\n",
        "  AB_GUARD_DOG,        // Intimidacion le sube el ataque, y no le echan\n",
        "  AB_TRAP_TYPE,        // arg = T_*: al rival de ese tipo no le deja irse\n",
        "  AB_COMATOSE,         // duerme siempre: ningun estado le entra\n",
        "  AB_TOXIC_CHAIN,      // 30% de entoxicar con cualquier golpe\n",
        "  AB_POISON_PUPPET,    // al que envenena, tambien le confunde\n",
        "  AB_WIMP_OUT,         // por debajo de media vida, se larga\n",
        "  AB_UNSEEN_FIST,      // sus golpes de contacto pasan el Protect\n",
        "  AB_ORICHALCUM,       // saca el sol al entrar Y crece con el\n",
        "  AB_HADRON,           // idem con el campo electrico\n",
        "  AB_FORECAST,         // su tipo sigue al cielo (agua, fuego, hielo)\n",
        "  AB_TYPE_FROM_ITEM,   // su tipo es el de la tabla que lleve\n",
        "  AB_SUN_BOOST,        // al sol, +50% de ataque y defensa especial\n",
        "  AB_STANCE,           // pegando se afila; de apoyo, se escuda\n",
        "  AB_HP_FORM,          // arg 1 = crece bajo media vida; 0 = sobre un cuarto\n",
        "  AB_HERO,             // al retirarse se vuelve heroe, y vuelve crecido\n",
        "  AB_TERA_SHIFT,       // al entrar, +1 a su stat mas alto\n",
        "  AB_DISGUISE,         // el primer golpe le quita un octavo y nada mas\n",
        "  AB_GULP,             // tras un golpe de agua, al que le toca le duele\n",
        "  AB_HUNGER,           // alterna cada turno y le cambia el tipo al golpe\n",
        "  AB_ILLUSION,         // entra con la cara de otro hasta que le peguen\n",
        "  AB_DANCER,           // copia el baile que use el rival\n",
        "  AB_RELAY_BOOST,      // arg 1 = deja al siguiente +30% especial, 2 = todo\n",
        "  AB_RELAY_CURE,       // al retirarse, le cura el estado al siguiente\n",
        "  AB_COMMANDER,        // al entrar, +2 de especial y de velocidad\n",
        "  AB_CONTACT_BOOST,    // +1/3 a los golpes de contacto\n",
        "  AB_MAGIC_BOUNCE,     // le devuelve al rival sus golpes de estado\n",
        "  AB_TRACE,            // al entrar le copia la habilidad al rival\n",
        "  AB_PARENTAL,         // pega DOS veces, la segunda a un cuarto\n",
        "  AB_PERISH_BODY,      // quien le toca cae en tres turnos, y el tambien\n",
        "  AB_WIND_RIDER,       // el viento no le toca y encima le crece\n",
        "  AB_DELTA_STREAM,     // al volador se le quitan sus debilidades\n",
        "  AB_QUEENLY,          // al rival no le salen los golpes de prioridad\n",
        "  AB_PUNK_ROCK,        // el sonido pega +30% y le entra a la mitad\n",
        "  AB_THERMAL,          // el fuego le sube el ataque y no se quema\n",
        "  AB_DEEP_ROOT,        // lo que drena, drena mas (del Anil)\n",
        "  AB_HALVE_FIRE_ICE,  // fuego y hielo le hacen la mitad\n",
        "  AB_CONTACT_STATUS,  // arg = ST_*: 30% al que le pegue de cerca\n",
        "  AB_CONTACT_CONFUSE, // 30% de confundir al que le pegue de cerca\n",
        "  AB_CONTACT_HURT,    // arg = divisor: le devuelve parte al que toca\n",
        "  AB_PINCH_TYPE,      // arg = T_*: +50% a ese tipo por debajo de 1/3\n",
        "  AB_TECHNICIAN,      // arg = potencia: +50% a los golpes flojos\n",
        "  AB_GUTS,            // +50% de ataque si esta mal de estado\n",
        "  AB_HUGE_POWER,      // ataque x2\n",
        "  AB_ADAPTABILITY,    // el STAB pasa de x1,5 a x2\n",
        "  AB_NO_CRIT,         // no le entran criticos\n",
        "  AB_STURDY,          // estando entero, aguanta un golpe mortal\n",
        "  AB_MULTISCALE,      // estando entero, recibe la mitad\n",
        "  AB_NO_SECONDARY,    // los efectos secundarios no le entran\n",
        "  AB_MARVELSCALE,     // +50% de defensa si esta mal de estado\n",
        "  AB_IMMUNE_STATUS,   // arg = ST_* que no le entra\n",
        "  AB_NO_CONFUSE,\n",
        "  AB_NO_FLINCH,\n",
        "  AB_SHED_SKIN,       // arg = % de curarse el estado al cerrar el turno\n",
        "  AB_SYNCHRONIZE,     // le pasa el estado al que se lo puso\n",
        "  AB_POISON_HEAL,     // el veneno le cura en vez de hacerle dano\n",
        "  AB_INTIMIDATE,      // al entrar, -1 al ataque del rival\n",
        "  AB_PRESSURE,        // al rival se le va el doble de PP\n",
        "  AB_KEEN_EYE,        // no le bajan la punteria\n",
        "  AB_NO_LOWER_ATK,\n",
        "  AB_NO_LOWER_DEF,\n",
        "  AB_CLEAR_BODY,      // no le bajan NINGUN stat\n",
        "  AB_SPEED_BOOST,     // +1 de velocidad al cerrar cada turno\n",
        "  AB_COMPOUND_EYES,   // arg = % de punteria extra\n",
        "  AB_NO_GUARD,        // ni el falla ni le fallan\n",
        "  AB_SERENE_GRACE,    // los efectos secundarios salen el doble\n",
        "  // --- segunda tanda (2026-10-04) ---------------------------\n",
        "  // Hasta aqui habia 67 habilidades con mecanica de 304, y las\n",
        "  // otras 237 salian en la ficha sin hacer nada. Estas son las\n",
        "  // que mas especies llevaban encima.\n",
        "  AB_SHEER_FORCE,     // +30% a los golpes con efecto, y sin efecto\n",
        "  AB_PUNCH_BOOST,     // +20% a los golpes de puno (MV_PUNCH)\n",
        "  AB_HUSTLE,          // +50% de ataque fisico, -20% de punteria\n",
        "  AB_TINTED_LENS,     // x2 cuando el tipo le va mal\n",
        "  AB_SNIPER,          // sus criticos pegan la mitad mas\n",
        "  AB_RIVALRY,         // +25% al del mismo sexo, -25% al otro\n",
        "  AB_NO_RECOIL,       // el retroceso no le hace nada\n",
        "  AB_BLITZ,           // 1er turno fuera: +50% vel, +20% atq (del Anil)\n",
        "  AB_WEAK_ARMOR,      // al recibir un fisico: -1 defensa, +2 velocidad\n",
        "  AB_OVERCOAT,        // ni el clima ni los polvos le tocan\n",
        "  AB_INFILTRATOR,     // se salta Reflejo, Pantalla Luz y Velo Sagrado\n",
        "  AB_MOLD_BREAKER,    // la habilidad del rival no cuenta\n",
        "  AB_CURE_WEATHER,    // arg = clima: con ese cielo se cura el estado\n",
        "  AB_REGENERATOR,     // al salir del combate recupera un tercio\n",
        "  AB_PRANKSTER,       // sus golpes de apoyo salen primero\n",
        "  AB_UNNERVE,         // el rival no puede comerse su baya\n",
        "  AB_RUN_AWAY,        // de un salvaje se escapa siempre\n",
        "  AB_FRISK,           // al entrar, ve que lleva el rival\n",
        "  AB_TELEPATHY,       // al relevar, entra sin comerse el golpe\n",
        "  // --- tercera tanda ------------------------------------------\n",
        "  AB_RECKLESS,        // +20% a los golpes con retroceso\n",
        "  AB_RATTLED,         // +1 velocidad si le pegan bicho/siniestro/fantasma\n",
        "  AB_MOXIE,           // arg = SG_*: +1 de ese stat al tumbar a alguien\n",
        "  AB_SPITE_STAT,      // arg = SG_*: +2 si el RIVAL le baja un stat\n",
        "  AB_DAMP,            // nadie se autodestruye delante de el\n",
        "  AB_UNAWARE,         // se salta los stages del otro\n",
        "  AB_SAND_FORCE,      // +30% a roca/tierra/acero con tormenta de arena\n",
        "  AB_SCRAPPY,         // normal y lucha SI le entran al fantasma\n",
        "  AB_MAGIC_GUARD,     // solo le hace dano lo que le pegan de frente\n",
        "  AB_GOOEY,           // al que le toca, -1 de velocidad\n",
        "  AB_STEADFAST,       // +1 de velocidad cada vez que le amilanan\n",
        "  AB_ANALYTIC,        // +30% si pega el ULTIMO\n",
        "  AB_UNBURDEN,        // sin objeto encima, el doble de velocidad\n",
        "  AB_GLUTTONY,        // la baya le salta a la mitad, no al cuarto\n",
        "  // --- cuarta tanda -------------------------------------------\n",
        "  AB_PICKUP,          // FUERA del combate: de vez en cuando trae algo\n",
        "  AB_PICKPOCKET,      // al que le toca sin manos libres, le roba\n",
        "  AB_ANTICIPATION,    // al entrar, avisa si el otro trae algo peligroso\n",
        "  AB_GUARD_WEATHER,   // arg = clima: con ese cielo no le entran estados\n",
        "  AB_SUCTION_CUPS,    // no la sacan de la pista a empujones\n",
        "  // --- quinta tanda (leva 70: las formas de combate) ----------\n",
        "  AB_ZEN_MODE,        // Darmanitan: a media vida, Modo Daruma\n",
        "  AB_ICE_FACE,        // Eiscue: el primer fisico lo para el hielo\n",
        "};\n\n",
        "struct AbilityEntry {\n"
        "  const char *name;\n"
        "  uint8_t fx;   // AB_*\n"
        "  uint8_t arg;  // significa una cosa distinta en cada efecto\n"
        "};\n\n",
        f"#define ABIL_COUNT {len(usadas) + 1}\n",
        "// El indice de habilidad es uint16_t EN TODAS PARTES (ABIL_OF,\n"
        "// Pet::ability, StoredMon::ability, Combatant::ability). Era\n"
        "// uint8_t y aguantaba de sobra con 198, pero el PBS del Anil tiene\n"
        "// 293 habilidades entre las 1025 especies y el numero 256 llega en\n"
        "// ZACIAN (#888, Galar). Eso NO habria roto el build: la habilidad\n"
        "// 260 se guarda como la 4 y el bicho sale con otra, en silencio,\n"
        "// que es el fallo que mas veces se ha repetido en este proyecto\n"
        "// (evolvesTo, el indice de golpe...). Se arreglo de una vez en vez\n"
        "// de esperar a tropezarse.\n"
        "static_assert(ABIL_COUNT <= 65535, \"el indice de habilidad es uint16_t\");\n\n",
        "static const AbilityEntry ABIL_TBL[] = {\n",
        '  { "-", AB_NONE, 0 },\n',
    ]
    for a in usadas:
        nom_es = hab.get(a, {}).get("Name", a)
        nom = ascii_plano(dicc.get(nom_es, nom_es))
        fx, arg = MECANICA.get(a, ("AB_NONE", 0))
        # el clima se nombra con el enum propio de este fichero
        if isinstance(arg, str) and arg.startswith("WE_"):
            arg = "AB_" + arg
        out.append(f'  {{ "{nom}",{" " * (largo - len(nom))} {fx}, {arg} }},\n')
    out.append("};\n\n")

    # QUE HACE cada habilidad. Sin esto la ficha ensena un nombre suelto
    # ("Blaze") y no hay forma de saber que significa sin salir a buscarlo.
    # Mismo camino que los nombres: el PBS trae la descripcion en espanol y
    # la traduccion del propio Anil (MessageTypes indice MT_DESC) la pasa a
    # ingles, con la frase entera como clave.
    desc = marshal_lite.load(DAT)[MT_DESC]
    desc = {k.decode("utf-8", "replace") if isinstance(k, bytes) else k:
            v.decode("utf-8", "replace") if isinstance(v, bytes) else v
            for k, v in desc.items()}
    out.append("// Que hace cada habilidad, indexado igual que ABIL_TBL.\n"
               "static const char *const ABIL_DESC[ABIL_COUNT] = {\n")
    out.append('  "",\n')
    faltan = 0
    sinTraducir = []
    for a in usadas:
        es = hab.get(a, {}).get("Description", "")
        if a in DESC_MANO:
            en = ascii_plano(DESC_MANO[a])
        else:
            en = ascii_plano(desc.get(es, es))
            # `desc.get(es, es)` cae al ESPAÑOL cuando falta la entrada, y
            # lo hacia en silencio: Impulso (Speed Boost) salia en la ficha
            # en español. Mismo fallo, y mismo arreglo, que en las
            # descripciones de los golpes (ver gen_moves.py).
            if es and desc.get(es, es) == es and parece_espanol(es):
                sinTraducir.append(a)
        if not en:
            faltan += 1
        out.append('  "' + en.replace("\\", "\\\\").replace('"', '\\"') + '",\n')
    out.append("};\n\n")
    if faltan:
        print(f"  OJO: {faltan} habilidades sin descripcion")
    if sinTraducir:
        print(f"  OJO: {len(sinTraducir)} descripciones que el .dat del Anil "
              f"no traduce y saldrian en ESPAÑOL: "
              f"{', '.join(sinTraducir[:10])}")
        print("       añadelas a DESC_MANO en este fichero")

    out.append("// Las que puede tener cada especie: [0] y [1] normales, [2] la\n"
               "// oculta. 0 = no tiene esa ranura.\n")
    out.append(f"static const uint16_t ABIL_OF[{N_DEX + 1}][3] = {{\n")
    out.append("  { 0, 0, 0 },\n")
    for d in range(1, N_DEX + 1):
        ab, oc = porDex.get(d, ([], []))
        a1 = idx.get(ab[0], 0) if len(ab) > 0 else 0
        a2 = idx.get(ab[1], 0) if len(ab) > 1 else 0
        a3 = idx.get(oc[0], 0) if len(oc) > 0 else 0
        out.append(f"  {{ {a1:3}, {a2:3}, {a3:3} }},  // {d}\n")
    out.append("};\n\n")
    out.append("static inline const AbilityEntry &abilAt(uint16_t i) {\n"
               "  return ABIL_TBL[i < ABIL_COUNT ? i : 0];\n"
               "}\n")

    with open(DST, "w", encoding="utf-8") as f:
        f.write("".join(out))
    print(f"guardado {DST}")
    print(f"  sin mecanica: {', '.join(sinMec[:14])}...")


if __name__ == "__main__":
    main()
