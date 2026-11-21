#!/usr/bin/env python3
"""Genera moves.h: golpes y learnsets, desde el PBS de Pokemon Anil.

De donde sale cada cosa:
  - Pokemon Anil V4.13/PBS/moves.txt   -> tipo, potencia, PP y categoria
  - Pokemon Anil V4.13/PBS/pokemon.txt -> los learnsets (nivel, golpe)

Se cogen TODOS los golpes, no solo los de gen 1. El PBS de Anil llega hasta
gen 9 y van a entrar mas generaciones pronto, asi que filtrar a gen 1 solo
obligaria a rehacer esto. Por eso la tabla de tipos tiene 18 y no 15: entre los
que las 151 aprenden al subir de nivel ya salen siniestro, acero y hada.

El unico filtro que queda es el de golpes que hacen DANO: un combate simple no
sabe que hacer con HARDEN ni con TRANSFORM. Ditto se queda sin ninguno (solo
aprende Transform) y le toca FORCEJEO, que es lo que hace el juego original
cuando no queda ningun golpe usable.

Fisico o especial sale del campo Category del PBS. En gen 1 lo decidia el TIPO
del golpe, pero desde gen 4 es por golpe, y estos son golpes modernos.

  python3 tools/gen_moves.py
"""
import collections
import io
import os
import re
import sys
import unicodedata

AQUI = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, AQUI)
import marshal_lite
import texto_pt
from dex_data import DEX
from pt_golpes import GOLPES as PT_GOLPES
from dex_range import N_DEX
from type_chart import TIPOS as GEN1_TIPOS

PBS = os.environ.get(
    'ANIL_PBS', r"C:\Users\foulu\Downloads\Pokemon Anil V4.13\PBS")
DST = os.path.join(AQUI, '..', 'moves.h')

# En gen 1 fisico/especial lo decidia el TIPO; desde gen 4 es POR GOLPE, y como
# aqui se usan golpes modernos se respeta el campo Category del PBS.
# PBS -> enum del firmware
C_TIPO = {t.upper(): 'T_' + t.upper() for t in GEN1_TIPOS if t != 'none'}
C_TIPO['FIGHTING'] = 'T_FIGHT'
C_TIPO['ELECTRIC'] = 'T_ELEC'

SLUG = {d[1].upper(): d[0] for d in DEX}
SLUG['NIDORANFE'] = 29   # el PBS los llama NIDORANfE / NIDORANmA, igual que
SLUG['NIDORANMA'] = 32   # las carpetas de sprites shiny de Anil
# El mas largo hoy es STOMPINGTANTRUM (15 chars): 180 px a escala 2, y la
# tarjeta de golpe tiene 200 utiles. Si algo pasa de aqui habria que rodar el
# texto, que es justo lo que no queremos en esta pantalla.
MAX_NOMBRE = 16

# MessageTypes del .dat del Anil: el 6 son las descripciones de golpe. Es un
# diccionario {frase en espanol -> frase en ingles}, no una lista indexada.
MT_DESC = 6
DAT = os.path.join(os.path.dirname(PBS), 'Data', 'messages_english_game.dat')


def ascii_plano(s):
    """La fuente del firmware es la 5x7 de siempre: solo ASCII imprimible.

    'Pokemon' pierde el acento y las comillas tipograficas pasan a rectas.
    Se hace aqui y no en la pantalla para que lo que se ve sea exactamente
    lo que hay en el fichero.
    """
    s = (s.replace('’', "'").replace('‘', "'")
          .replace('“', '"').replace('”', '"')
          .replace('–', '-').replace('—', '-')
          .replace('…', '...'))
    s = unicodedata.normalize('NFKD', s)
    s = ''.join(c for c in s if not unicodedata.combining(c))
    s = s.replace('\n', ' ').replace('\r', ' ')
    s = re.sub(r'\s+', ' ', s).strip()
    return ''.join(c for c in s if 32 <= ord(c) < 127)


def cadena_c(s):
    return '"' + s.replace('\\', '\\\\').replace('"', '\\"') + '"'


def traducciones(indice):
    d = marshal_lite.load(DAT)[indice]
    return {k.decode('utf-8', 'replace') if isinstance(k, bytes) else k:
            v.decode('utf-8', 'replace') if isinstance(v, bytes) else v
            for k, v in d.items()}


def secciones(path):
    out, cur, dic = [], None, {}
    for ln in open(path, encoding='utf-8', errors='replace'):
        ln = ln.strip()
        if not ln or ln.startswith('#'):
            continue
        m = re.match(r'^\[(.+)\]$', ln)
        if m:
            if cur:
                out.append((cur, dic))
            cur, dic = m.group(1), {}
            continue
        if '=' in ln and cur:
            k, v = ln.split('=', 1)
            dic[k.strip()] = v.strip()
    if cur:
        out.append((cur, dic))
    return out


# De los 337 golpes de dano, 271 traen algun efecto extra en el PBS. Aqui se
# traducen SOLO los que no necesitan estado nuevo: son cuentas sobre el dano que
# ya se ha calculado, asi que salen gratis. Los demas (quemar, paralizar, bajar
# stats, confundir...) piden un byte de estado por combatiente y una fase de UI
# para que el jugador los vea, y van en la siguiente tanda: de momento esos
# golpes se comportan como dano puro, que es lo que hacian hasta ahora.
FX = ['FX_NONE', 'FX_DRAIN', 'FX_DRAIN_SLEEP', 'FX_PHOTON',
      'FX_DESTINY_BOND', 'FX_GRUDGE', 'FX_FINAL_GAMBIT', 'FX_MEMENTO',
      'FX_HEALING_WISH', 'FX_LUNAR_DANCE',
      'FX_RECOIL_2', 'FX_RECOIL_3', 'FX_RECOIL_4',
      'FX_MULTI_2', 'FX_MULTI_3', 'FX_MULTI_2_5']


# Los PP que se embarcan. Casi siempre son los del PBS tal cual; la
# excepcion vive AQUI, escrita, en vez de escondida en un clamp.
#
# El PBS trae dos golpes de UN solo uso, SKETCH y REVIVALBLESSING, y en
# los juegos originales eso esta compensado: Esquema es PERMANENTE, lo
# usas una vez en la vida del bicho y te quedas el golpe. Nuestro
# Esquema NO lo es -- la copia dura el combate, porque el equipo guarda
# los golpes por nivel y no por bicho -- asi que con un uso el Smeargle
# seria un bicho de un solo acto y nada mas.
#
# "Ningun golpe de un solo uso" es una decision de ESTE juego y tiene su
# test (ningun_golpe_se_queda_en_un_solo_uso). El sitio honrado para la
# excepcion es el generador, no debilitar el test.
PP_PROPIO = {'SKETCH': 5}


def pp_de(nombre, d):
    pbs = int(d.get('TotalPP', 0) or 0)
    mio = PP_PROPIO.get(nombre)
    if mio is None:
        return pbs
    print('  PP propios: %s %d -> %d (ver PP_PROPIO)' % (nombre, pbs, mio))
    return mio


def chance_de(d):
    """El EffectChance del PBS, con el 101 de los colmillos acotado.

    Los tres unicos golpes del PBS que pasan de 100 son los de DOS efectos
    (Colmillo Igneo, Hielo y Rayo): el Anil escribe 101 para decir "van dos
    del 10". Tal cual, el motor hace `random(100) < 101`, que es siempre, y
    Colmillo Igneo quemaba cada turno.

    Se acota SOLO lo que pasa de 100. Un 100 de verdad se respeta: Nuzzle
    paraliza siempre y tiene potencia, y bajarlo seria romper un golpe que
    estaba bien.
    """
    c = int(d.get('EffectChance', 0) or 0)
    return 10 if c > 100 else c


# Los que se reconocen por el nombre ENTERO del FunctionCode. Van los
# primeros y por igualdad, no por substring: 'DoublePowerIfTargetNotActed'
# contiene 'DoublePowerIfTarget...' y media docena de reglas de abajo
# podrian pillarlo antes de tiempo.
FX_EXACTO = {
    # La segunda leva: los seis que estaban fuera por necesitar cada uno
    # un campo de combate que no existia. Exactos y no por `in`, porque
    # 'UserMakeSubstitute' tambien es el principio del de Shed Tail.
    'StartSlowerBattlersActFirst': 'FX_TRICKROOM',
    'UserMakeSubstitute': 'FX_SUBSTITUTE',
    'HealUserPositionNextTurn': 'FX_WISH',
    'StartPerishCountsForAllBattlers': 'FX_PERISH',
    'StartTargetCannotUseItem': 'FX_EMBARGO',
    'DisableTargetHealingMoves': 'FX_HEALBLOCK',
    # La tercera: los que pegaban como un golpe normal y el Anil les da
    # algo mas (tools/audita_golpes_anil.py los encontro).
    # La cuarta: los que caian en un efecto generico y perdian media
    # mecanica (Fire Fang sin amedrentar, Tri Ataque que solo congelaba,
    # Rompecoraza que solo se bajaba las defensas, Bostezo que dormia en
    # el acto). Se vieron comparando CADA FunctionCode compuesto.
    'SleepTargetNextTurn': 'FX_YAWN',
    'LowerUserDefSpDef1RaiseUserAtkSpAtkSpd2': 'FX_SHELLSMASH',
    'ParalyzeBurnOrFreezeTarget': 'FX_TRIATTACK',
    'PoisonParalyzeOrSleepTarget': 'FX_DIRECLAW',
    'BurnFlinchTarget': 'FX_FANG',
    'FreezeFlinchTarget': 'FX_FANG',
    'ParalyzeFlinchTarget': 'FX_FANG',
    'PoisonTargetLowerTargetSpeed1': 'FX_TOXICTHREAD',
    'RaiseUserAtkSpAtk1Or2InSun': 'FX_GROWTH',
    'FreezeTargetSuperEffectiveAgainstWater': 'FX_FREEZEDRY',
    'RecoilHalfOfTotalHP': 'FX_RECOIL_HP',
    # Bastion Final: los cinco +1 una sola vez, y ya no se releva.
    'RaiseUserMainStats1TrapUserInBattle': 'FX_NORETREAT',
    'AddMoneyGainedFromBattle': 'FX_PAYDAY',
    'BurnAttackerBeforeUserActs': 'FX_BEAKBLAST',
    'UsedAfterUserTakesPhysicalDamage': 'FX_SHELLTRAP',
    'MultiTurnAttackBideThenReturnDoubleDamage': 'FX_BIDE',
    'HitOncePerUserTeamMember': 'FX_BEATUP',
    'TypeAndPowerDependOnUserBerry': 'FX_NATURALGIFT',
    'EffectDependsOnEnvironment': 'FX_SECRETPOWER',
    'DoublePowerIfTargetNotActed': 'FX_X2_IF_NOT_ACTED',
    'DoublePowerIfTargetParalyzedCureTarget': 'FX_X2_IF_PARA',
    'DoublePowerAfterFusionBolt': 'FX_AFTER_BOLT',
    'DoublePowerAfterFusionFlare': 'FX_AFTER_FLARE',
    'IncreasePowerSuperEffective': 'FX_X133_IF_SUPER',
    'IncreasePowerInSunWeather': 'FX_X15_SUN',
    'IncreasePowerInElectricTerrain': 'FX_X15_ELEC_TR',
    # La parte de "a todos los rivales" es de combate doble y aqui no
    # significa nada; la del campo psiquico si.
    'HitsAllFoesAndPowersUpInPsychicTerrain': 'FX_X15_PSY_TR',
    'IncreasePowerEachTimeHit': 'FX_RAGE_FIST',
    'IncreasePowerEachFaintedAlly': 'FX_LAST_RESPECTS',
    'RandomlyDealsDoubleDamage': 'FX_FICKLE',
    'PowerLowerWithUserHappiness': 'FX_BY_BOND_INV',
    'RandomlyDamageOrHealTarget': 'FX_PRESENT',
    'HitTenTimes': 'FX_MULTI_10',
    'SuperEffectiveAgainstBug': 'FX_SUPER_VS_BUG',
    'EffectivenessIncludesFlyingType': 'FX_ALSO_FLYING',
    # --- los que no tienen un tipo fijo ---
    'TypeIsUserFirstType': 'FX_TYPE_MINE1',
    'TypeIsUserSecondType': 'FX_TYPE_MINE2',
    'TypeAndPowerDependOnTerrain': 'FX_TYPE_TERRAIN',
    'NormalMovesBecomeElectric': 'FX_NORMAL_IS_ELEC',
    # Juicio, Multiataque y Tecno Shock NO se mapean a proposito: cambian
    # de tipo con la placa / el disco / el cartucho, y esos objetos no
    # existen en este juego. Sin el objeto se quedan en normal, que es lo
    # mismo que hacen en los juegos. Darles un FX que no hace nada seria
    # mentirle al verificador de cobertura.
    # --- los que quitan el terreno ---
    'RemoveTerrain': 'FX_WIPE_TERRAIN',
    'RemoveTerrainIceSpinner': 'FX_WIPE_TERRAIN',
    # --- y los que se gastan a si mismos ---
    'UserLosesFireType': 'FX_LOSE_FIRE',
    'UserLosesElectricType': 'FX_LOSE_ELEC',
    'IgnoreTargetAbility': 'FX_IGNORE_ABIL',
    # El propio nombre del codigo trae las DOS mitades, y el mapa se
    # quedaba con la primera: la habilidad del rival seguia contando.
    'CategoryDependsOnHigherDamageIgnoreTargetAbility': 'FX_PHOTON',
    # La familia del SELF_KO. Siete codigos que caian en el mismo efecto
    # "el que pega cae", y dos de ellos no tumban a nadie.
    'AttackerFaintsIfUserFaints': 'FX_DESTINY_BOND',
    'SetAttackerMovePPTo0IfUserFaints': 'FX_GRUDGE',
    'UserFaintsFixedDamageUserHP': 'FX_FINAL_GAMBIT',
    'UserFaintsLowerTargetAtkSpAtk2': 'FX_MEMENTO',
    'UserFaintsHealAndCureReplacement': 'FX_HEALING_WISH',
    'UserFaintsHealAndCureReplacementRestorePP': 'FX_LUNAR_DANCE',
    'TerapagosCategoryDependsOnHigherDamage': 'FX_BEST_CATEGORY',
    'StartSaltCureTarget': 'FX_SALT_CURE',
    'CureTargetBurn': 'FX_CURE_BURN',
    'UserVulnerableUntilNextAction': 'FX_EXPOSED',
    'CantSelectConsecutiveTurns': 'FX_NO_REPEAT',
    'DamageTargetAddLeechSeedToFoeSide': 'FX_HIT_AND_SEED',
    'UserStealTargetPositiveStatStages': 'FX_STEAL_STAGES',
}


def efecto(code):
    if code in FX_EXACTO:
        return FX_EXACTO[code]
    c = code or ''
    # --- los que le hacen algo al rival sin pegarle ---
    # Casi todos estaban FUERA de la tabla por no tener ni dano ni
    # efecto modelado. Varios no se podian hacer hasta ahora: atrapar y
    # echar del combate no significan nada si no hay ni huida ni
    # relevos, y las dos cosas entraron hace poco.
    # Los dos golpes COPIA. Sin ellos, el Ditto y el Smeargle embarcaban
    # sabiendo solo Forcejeo: es lo UNICO que aprenden, y el filtro de
    # "golpe de apoyo sin efecto" se los llevaba por delante. No es un
    # golpe que no hace nada, es un bicho que no puede pelear.
    if 'TransformUserIntoTarget' in c:
        return 'FX_TRANSFORM'
    if 'ReplaceMoveWithTargetLastMoveUsed' in c:
        return 'FX_SKETCH'
    if 'DisableTargetStatusMoves' in c:
        return 'FX_TAUNT'
    if 'NegateTargetEvasionStatStageAndGhostImmunity' in c:
        return 'FX_FORESIGHT'
    if 'TrapTargetInBattle' in c:
        return 'FX_TRAP'
    if 'SwitchOutTargetStatusMove' in c:
        return 'FX_ROAR'
    if 'StartUserSideImmunityToStatStageLowering' in c:
        return 'FX_MIST'
    if 'CureUserPartyStatus' in c:
        return 'FX_HEAL_BELL'
    if 'HitsTargetInSkyGroundsTarget' in c:
        return 'FX_SMACK_DOWN'
    # --- los que dependen del combate entero ---
    if 'PursueSwitchingFoe' in c:
        return 'FX_PURSUIT'
    if 'SwitchOutUserDamagingMove' in c:
        return 'FX_UTURN'
    if 'FailsIfUserHasUnusedMove' in c:
        return 'FX_LAST_RESORT'
    if 'PowerHigherWithConsecutiveUseOnUserSide' in c:
        return 'FX_ECHO'
    if 'DoublePowerIfAllyFaintedLastTurn' in c:
        return 'FX_RETALIATE'
    # --- cuentas sobre el dano que no piden NADA nuevo ---
    # Estos solo miran algo que ya estaba en la ficha de los dos
    # combatientes. Estaban sin hacer por olvido, no por imposibles.
    if 'NegateTargetAbility' in c:
        return 'FX_GASTRO'
    if 'SetTargetAbilityToUserAbility' in c:
        return 'FX_ENTRAIN'
    if 'DisableTargetMovesKnownByUser' in c:
        return 'FX_IMPRISON'
    # --- la tanda que el typeSet y los bandos hicieron posible ---
    # EMPAPAR estaba fuera por falta de poder pisar el tipo, y eso lo
    # trajo Mutatipo. Las demas piden un contador de bando o de bicho,
    # que es lo que ya tienen las pantallas y las trampas.
    if 'SetTargetTypesToWater' in c:
        return 'FX_SOAK'
    if 'SetTargetAbilityToInsomnia' in c:
        return 'FX_WORRY_SEED'
    if 'StartPreventCriticalHitsAgainstUserSide' in c:
        return 'FX_LUCKY_CHANT'
    if 'AttractTarget' in c:
        return 'FX_ATTRACT'
    if 'StartUserAirborne' in c:
        return 'FX_MAGNET_RISE'
    if 'ProtectUserSideFromPriorityMoves' in c:
        return 'FX_QUICK_GUARD'
    # --- la tanda que el RELEVO hizo posible ---
    # Todos estos estaban FUERA de la tabla y no por capricho: un golpe
    # que pone una trampa en el lado del rival no significa nada si nadie
    # entra nunca, y pasar los stats al siguiente tampoco si no hay
    # siguiente. Con banquillo y relevos ya hay donde pasar.
    if 'SwitchOutUserPassOnEffects' in c:
        return 'FX_BATON'
    if 'StartUserSideDoubleSpeed' in c:
        return 'FX_TAILWIND'
    if 'AddStealthRocksToFoeSide' in c:
        return 'FX_ROCKS'
    if 'AddToxicSpikesToFoeSide' in c:
        return 'FX_TSPIKES'
    if 'AddStickyWebToFoeSide' in c:
        return 'FX_WEB'
    if 'AddSpikesToFoeSide' in c:
        return 'FX_SPIKES'
    if 'StartWeakenFireMoves' in c:
        return 'FX_WATERSPORT'
    if 'StartWeakenElectricMoves' in c:
        return 'FX_MUDSPORT'
    if 'StartHealUserEachTurnTrapUserInBattle' in c:
        return 'FX_INGRAIN'
    if 'UserTargetAverageHP' in c:
        return 'FX_PAINSPLIT'
    if 'UserTargetSwapItems' in c:
        return 'FX_TRICK'
    if 'HealUserAndAlliesQuarterOfTotalHP' in c:
        return 'FX_HEAL_QUARTER'
    if 'DisableTargetUsingSameMoveConsecutively' in c:
        return 'FX_TORMENT'
    if 'UseLastMoveUsed' in c:
        return 'FX_COPYCAT'
    # --- los ultimos sueltos, todos cuentas sobre lo que ya hay ---
    if 'FailsUnlessTargetSharesTypeWithUser' in c:
        return 'FX_SAME_TYPE'
    if 'FailsIfNotUserFirstTurn' in c:
        return 'FX_FIRST_TURN'
    if 'PowerHigherWithUserHP' in c:
        return 'FX_POW_HIGH_HP'
    if 'UserLosesHalfOfTotalHP' in c:
        return 'FX_COST_HALF'
    if 'TrapUserAndTargetInBattle' in c:
        return 'FX_TRAP_BOTH'
    # --- la tanda que cierra los golpes de dano que faltaban ---
    if 'FixedDamage20' in c:
        return 'FX_FIXED_20'
    if 'FailsIfTargetHasNoItem' in c:
        return 'FX_POLTERGEIST'
    if 'TypeDependsOnUserIVs' in c:
        return 'FX_HIDDEN_POWER'
    if 'TypeAndPowerDependOnWeather' in c:
        return 'FX_WEATHER_BALL'
    if 'PowerHigherWithTargetPositiveStatStages' in c:
        return 'FX_BY_TARGET_STAGES'
    if 'HigherPriorityInMistyTerrain' in c:
        return 'FX_PRIO_MISTY'
    if 'SwitchOutTargetDamagingMove' in c:
        return 'FX_DRAGONTAIL'
    if 'DisableTargetSoundMoves' in c:
        return 'FX_GAG'
    if 'FailsIfUserNotConsumedBerry' in c:
        return 'FX_BELCH'
    if 'DoublePowerIfUserLastMoveFailed' in c:
        return 'FX_TANTRUM'
    if 'AttackTwoTurnsLater' in c:
        return 'FX_FUTURE'
    # El trio del deposito: uno llena, uno lo escupe y uno se lo traga.
    # Van juntos porque sin los tres el deposito no sirve de nada.
    if 'UserAddStockpileRaiseDefSpDef1' in c:
        return 'FX_STOCKPILE'
    if 'PowerDependsOnUserStockpile' in c:
        return 'FX_SPIT_UP'
    if 'HealUserDependingOnUserStockpile' in c:
        return 'FX_SWALLOW'
    # Los que castigan al que se escondio. Magnitud va DELANTE porque su
    # codigo contiene el del Terremoto pelado.
    if 'RandomPowerDoublePowerIfTargetUnderground' in c:
        return 'FX_MAGNITUDE'
    if 'DoublePowerIfTargetUnderground' in c:
        return 'FX_X2_IF_UNDER'
    if 'DoublePowerIfTargetInSky' in c:
        return 'FX_X2_IF_SKY'
    # TORBELLINO es BindTargetDoublePowerIfTargetUnderwater: lo suyo es
    # AGARRAR, y el doble bajo el agua es la propina. Al poner este check
    # sin la excusa del Bind se quedo sin agarrar a nadie, cambiando un
    # efecto por otro peor. Un golpe no puede perder nada al ganar algo.
    if 'DoublePowerIfTargetUnderwater' in c and 'Bind' not in c:
        return 'FX_X2_IF_WATER'
    # 'TargetInSky' a secas y no 'HitsTargetInSky': el TRUENO es
    # ParalyzeTargetAlwaysHitsInRainHits*DecreasesInSun*TargetInSky, o
    # sea que la subcadena larga NO casa —— "DecreasesInSun" se mete por
    # medio. Se quedaba en FX_NONE y era el unico de los seis que el Anil
    # manda al cielo (hitsFlyingTargets?) y aqui fallaba contra uno
    # volando.
    #
    # Ampliarlo es seguro: en el PBS hay SEIS codigos con 'TargetInSky' y
    # los seis alcanzan el cielo. Los dos que ademas doblan potencia
    # (Tornado, Ciclon) ya se fueron arriba con DoublePowerIfTargetInSky,
    # y el Vuelo es TwoTurnAttackInvulnerableInSky, que no lleva 'Target'.
    if 'TargetInSky' in c:
        return 'FX_HITS_SKY'
    if 'DoublePowerIfTargetPoisoned' in c:
        return 'FX_X2_IF_POISON'
    if 'IgnoreTargetDefSpDefEvaStatStages' in c:
        return 'FX_IGNORE_STAGES'
    if 'UseTargetAttackInsteadOfUserAttack' in c:
        return 'FX_FOUL_PLAY'
    if 'DoublePowerIfUserLostHPThisTurn' in c:
        return 'FX_X2_IF_I_HURT'
    if 'PowerHigherWithTargetHP' in c:
        return 'FX_BY_TARGET_HP'
    if 'FixedDamageHalfTargetHP' in c:
        return 'FX_HALF_HP'
    if 'DoublePowerInElectricTerrain' in c:
        return 'FX_X2_ELEC_TERR'
    if 'DoublePowerIfUserPoisonedBurnedParalyzed' in c:
        return 'FX_FACADE'
    if 'CounterDamagePlusHalf' in c:
        return 'FX_METAL_BURST'
    if 'FailsIfUserDamagedThisTurn' in c:
        return 'FX_FOCUS_PUNCH'
    if 'DoublePowerIfTargetAsleepCureTarget' in c:
        return 'FX_WAKE_SLAP'
    if 'HigherPriorityInGrassyTerrain' in c:
        return 'FX_PRIO_GRASS'
    if 'StartRaiseUserAtk1WhenDamaged' in c:
        return 'FX_RAGE'
    # Alboroto va DELANTE de los otros MultiTurnAttack: comparte con
    # Enfado el encadenado, pero ademas no deja dormir a nadie.
    if 'MultiTurnAttackPreventSleeping' in c:
        return 'FX_UPROAR'
    # --- lo que paso antes en el turno ---
    # Estos no se podian hacer hasta que el combate supo QUIEN se habia
    # movido ya (Battle::movedThisTurn, que entro con Analytic).
    if 'FailsIfTargetActed' in c:
        return 'FX_ONLY_IF_FIRST'
    if 'DoublePowerIfTargetActed' in c:
        return 'FX_X2_IF_ACTED'
    if 'DoublePowerIfTargetLostHPThisTurn' in c:
        return 'FX_X2_IF_HURT'
    if 'DoublePowerIfTargetHPLessThanHalf' in c:
        return 'FX_X2_IF_HALF'
    # --- los que crecen ---
    if 'MultiTurnAttackPowersUpEachTurn' in c:
        return 'FX_ROLLOUT'
    # Rizo Defensa no es solo +1 defensa: deja al bicho ENROSCADO, y eso
    # DOBLA Rodar y Bola Hielo (el Anil le suma un escalon al 2**shift).
    # La subida de defensa sigue viniendo por el sx, que es otro campo.
    if 'RaiseUserDefense1CurlUpUser' in c:
        return 'FX_CURL'
    if 'PowerHigherWithConsecutiveUse' in c:
        return 'FX_RAMP'
    if 'PowerHigherWithUserHappiness' in c:
        return 'FX_BY_BOND'
    if 'PowerHigherWithUserHeavierThanTarget' in c:
        return 'FX_BY_WEIGHT_REL'
    if 'PowerHigherWithTargetWeight' in c:
        return 'FX_BY_WEIGHT'
    # --- objetos del rival ---
    if 'UserConsumeTargetBerry' in c:
        return 'FX_EAT_BERRY'
    if 'DestroyTargetBerryOrGem' in c:
        return 'FX_BURN_BERRY'
    # --- varios ---
    if 'LowerUserAtkDef1' in c:
        return 'FX_SELF_ATKDEF'
    if 'RemoveUserBindingAndEntryHazards' in c:
        return 'FX_SPIN'
    # --- objetos ---
    # Entran con la tanda que le dio objetos a los salvajes: hasta
    # entonces no tenian nada que robar ni que tirar.
    if 'RemoveTargetItem' in c:
        return 'FX_KNOCK_OFF'
    if 'UserTakesTargetItem' in c:
        return 'FX_STEAL'
    if 'DoublePowerIfUserHasNoItem' in c:
        return 'FX_NO_ITEM_X2'
    if 'ThrowUserItemAtTarget' in c:
        return 'FX_FLING'
    # --- drenaje y retroceso ---
    if 'HealUserByThreeQuarters' in c:
        return 'FX_DRAIN_3_4'
    # Comesuenos antes que el drenaje normal: su codigo CONTIENE el del
    # drenaje ('...IfTargetAsleep'), asi que si va despues no se ve nunca.
    if 'HealUserByHalfOfDamageDoneIfTargetAsleep' in c:
        return 'FX_DRAIN_SLEEP'
    if 'HealUserByHalfOfDamageDone' in c or 'AbsorbHealth' in c:
        return 'FX_DRAIN'
    if 'RecoilHalf' in c:
        return 'FX_RECOIL_2'
    if 'RecoilThird' in c:
        return 'FX_RECOIL_3'
    if 'RecoilQuarter' in c:
        return 'FX_RECOIL_4'
    # --- multigolpe ---
    if 'HitTwoToFiveTimes' in c:
        return 'FX_MULTI_2_5'
    # Triple Kick e Triple Hit: 1x, 2x e 3x a potencia, e a precisao se
    # tira a CADA impacto (HitThreeTimesPowersUpWithEachHit*). Antes caiam
    # no FX_MULTI_3 comum: tres de 10 que nunca falhavam depois do 1o.
    if 'HitThreeTimesPowersUpWithEachHit' in c:
        return 'FX_MULTI_3_UP'
    if 'HitThreeTimes' in c:
        return 'FX_MULTI_3'
    if 'HitTwoTimes' in c:
        return 'FX_MULTI_2'
    # --- variantes de la formula de dano: no piden estado nuevo ---
    if 'OHKO' in c:
        return 'FX_OHKO'
    if 'FixedDamageUserLevelRandom' in c:
        return 'FX_FIXED_LVL_RND'
    if 'FixedDamageUserLevel' in c:
        return 'FX_FIXED_LVL'
    if 'FixedDamage40' in c:
        return 'FX_FIXED_40'
    if 'PowerLowerWithUserHP' in c:
        return 'FX_POW_LOW_HP'
    if 'PowerHigherWithUserFasterThanTarget' in c:
        return 'FX_POW_SPEED'
    if 'PowerHigherWithTargetFasterThanUser' in c:
        return 'FX_POW_SLOW'
    if 'PowerHigherWithUserPositiveStatStages' in c:
        return 'FX_POW_STAGES'
    if 'PowerHigherWithLessPP' in c:
        return 'FX_POW_LESS_PP'
    if 'DoublePowerIfTargetStatusProblem' in c:
        return 'FX_X2_IF_STATUS'
    if 'AlwaysCriticalHit' in c:
        return 'FX_ALWAYS_CRIT'
    if 'UseTargetDefenseInsteadOfTargetSpDef' in c:
        return 'FX_VS_DEF'
    if 'UserFaintsExplosive' in c or 'UserFaints' in c:
        return 'FX_SELF_KO'
    if 'CrashDamageIfFails' in c:
        return 'FX_CRASH'
    # --- los que duran varios turnos: piden un par de bytes de estado ---
    if 'BindTarget' in c:
        return 'FX_BIND'
    if 'MultiTurnAttackConfuseUserAtEnd' in c:
        return 'FX_LOCK'
    if 'AttackAndSkipNextTurn' in c:
        return 'FX_RECHARGE'
    # Los tres escondites. Van DELANTE del TwoTurnAttack generico, que es
    # el que los recogia a todos: Excavar y Vuelo eran Rayo Solar con otro
    # nombre, sin la mitad que importa (mientras cargas no te alcanzan).
    # Y sin eso, Terremoto, Tornado y Surf (142 huecos de especie entre los
    # tres) no tenian a que mirar para pegar el doble.
    if 'TwoTurnAttackInvulnerableUnderground' in c:
        return 'FX_CHARGE_DIG'
    if 'TwoTurnAttackInvulnerableUnderwater' in c:
        return 'FX_CHARGE_DIVE'
    if 'TwoTurnAttackInvulnerableInSky' in c:
        return 'FX_CHARGE_FLY'
    # Golpe Umbrio y Golpe Fantasma: se esfuman un turno. Sin esto caian
    # en el TwoTurnAttack generico y se les podia pegar (y capturar)
    # mientras no estaban.
    if 'TwoTurnAttackInvulnerableRemoveProtections' in c:
        return 'FX_CHARGE_GONE'
    if 'TwoTurnAttack' in c:
        return 'FX_CHARGE'
    # --- clima: cambia las reglas del combate entero, no solo este golpe ---
    if 'StartSunWeather' in c:
        return 'FX_W_SUN'
    if 'StartRainWeather' in c:
        return 'FX_W_RAIN'
    if 'StartSandstormWeather' in c:
        return 'FX_W_SAND'
    if 'StartHailWeather' in c:
        return 'FX_W_HAIL'
    # --- terreno: como el clima pero pegado al SUELO (no afecta a quien vuela)
    if 'StartElectricTerrain' in c:
        return 'FX_T_ELEC'
    if 'StartGrassyTerrain' in c:
        return 'FX_T_GRASS'
    if 'StartMistyTerrain' in c:
        return 'FX_T_MISTY'
    if 'StartPsychicTerrain' in c:
        return 'FX_T_PSY'
    # --- cura ---
    if 'HealUserDependingOnWeather' in c:
        return 'FX_HEAL_WEATHER'     # Synthesis: mucho al sol, poco con lluvia
    if 'HealUserFullyAndFallAsleep' in c:
        return 'FX_REST'
    # PURIFICACION va ANTES del HealUserHalfOfTotalHP generico: su clase
    # es CureTargetStatusHealUserHalfOfTotalHP y la subcadena casa, asi
    # que colgada despues se quedaba en media cura y sin curar al otro
    # —— que es la mitad que le da nombre al golpe.
    if 'CureTargetStatus' in c and 'HealUser' in c:
        return 'FX_PURIFY'
    if 'HealUserByTargetAttack' in c:
        return 'FX_SAP_ATK'
    # RESPIRO, tambien ANTES del generico: su clase es
    # HealUserHalfOfTotalHPLoseFlyingTypeThisTurn y la subcadena casa,
    # asi que se quedaba en media cura —— sin la mitad que le da el
    # nombre, que es perder el VOLADOR durante el turno.
    if 'LoseFlyingTypeThisTurn' in c:
        return 'FX_ROOST'
    if 'HealUserHalfOfTotalHP' in c or 'HealTargetHalfOfTotalHP' in c:
        return 'FX_HEAL_HALF'
    # --- los que cobran o cambian algo DEL QUE LOS USA ---
    if 'LoseThirdOfTotalHP' in c:
        return 'FX_CLANG_SOUL'
    if 'LowerUserWeight' in c:
        return 'FX_AUTOTOMIZE'
    if 'RemoveHazardsSubstitutes' in c:
        return 'FX_TIDYUP'
    # DESPEJAR: ademas del -1 de evasion (que hace el sx), BARRE —— las
    # trampas de los dos lados, las pantallas del lado del otro y, desde
    # la gen 8, el terreno. Sin eso era un "le bajo la evasion" y ya.
    if 'RemoveSideEffects' in c:
        return 'FX_DEFOG'
    # --- estado de bando: dura varios turnos y protege al que lo pone ---
    if 'WeakenPhysicalDamageAgainstUserSide' in c:
        return 'FX_REFLECT'
    if 'WeakenSpecialDamageAgainstUserSide' in c:
        return 'FX_LIGHTSCREEN'
    if 'UserSideImmunityToInflictedStatus' in c:
        return 'FX_SAFEGUARD'
    if 'RemoveScreens' in c:
        return 'FX_BREAK_SCREENS'
    # --- este turno ---
    if 'ProtectUser' in c and 'Side' not in c:
        return 'FX_PROTECT'
    if 'UserEnduresFaintingThisTurn' in c:
        return 'FX_ENDURE'
    if 'RemoveProtections' in c:
        return 'FX_FEINT'
    # --- manipular lo que hay puesto ---
    if 'ResetAllBattlersStatStages' in c:
        return 'FX_HAZE'
    if 'ResetTargetStatStages' in c:
        return 'FX_CLEAR_TARGET'
    if 'UserCopyTargetStatStages' in c:
        return 'FX_PSYCHUP'
    if 'MaxUserAttackLoseHalfOfTotalHP' in c:
        return 'FX_BELLYDRUM'
    # --- criticos ---
    if 'RaiseUserCriticalHitRate2' in c:
        return 'FX_FOCUSENERGY'
    if 'EnsureNextCriticalHit' in c:
        return 'FX_LASERFOCUS'
    if 'EnsureNextMoveAlwaysHits' in c:
        return 'FX_MINDREADER'
    # --- que va royendo turno a turno ---
    if 'StartLeechSeedTarget' in c:
        return 'FX_LEECHSEED'
    if 'StartHealUserEachTurn' in c and 'Trap' not in c:
        return 'FX_AQUARING'
    # MALDICION, que son DOS golpes en uno segun el tipo del que lo usa.
    # El nombre de la clase del Anil lo dice entero:
    # CurseTargetOrLowerUserSpd1RaiseUserAtkDef1.
    if 'CurseTarget' in c:
        return 'FX_CURSE'
    # --- los que estaban FUERA de la tabla por no tener efecto ---
    #
    # Un golpe de apoyo sin fx ni sx se descarta (ver main()), asi que
    # estos seis no existian: ni en la tarjeta ni en el learnset. Lo
    # trajo el dueno —— "desses golpes q citou, tem alguns q a mecanica
    # funciona, pq diabos nao estao?" —— y tenia razon: ninguno de los
    # seis necesita combates dobles, que era mi excusa.
    if 'CureUserBurnPoisonParalysis' in c:
        return 'FX_REFRESH'
    if 'RestoreUserConsumedItem' in c:
        return 'FX_RECYCLE'
    if 'RaiseTargetRandomStat2' in c:
        return 'FX_ACUPRESSURE'
    if 'LowerPoisonedTarget' in c:
        return 'FX_VENOM_DRENCH'
    if 'HealUserDependingOnSandstorm' in c:
        return 'FX_SHORE_UP'
    if 'NegateTargetEvasionStatStageAndDarkImmunity' in c:
        return 'FX_MIRACLE_EYE'
    # --- tocar los golpes del rival ---
    if 'DisableTargetLastMoveUsed' in c:
        return 'FX_DISABLE'
    if 'DisableTargetUsingDifferentMove' in c:
        return 'FX_ENCORE'
    if 'LowerPPOfTargetLastMoveBy4' in c:
        return 'FX_SPITE'
    # --- formula ---
    if 'CounterPhysicalDamage' in c:
        return 'FX_COUNTER'
    if 'CounterSpecialDamage' in c:
        return 'FX_MIRRORCOAT'
    if 'LowerTargetHPToUserHP' in c:
        return 'FX_ENDEAVOR'
    if 'CannotMakeTargetFaint' in c:
        return 'FX_FALSESWIPE'
    if 'UseUserDefenseInsteadOfUserAttack' in c:
        return 'FX_BODYPRESS'
    if 'DoesNothing' in c:
        return 'FX_SPLASH'      # Splash: no hace nada, y ESA es la gracia
    return 'FX_NONE'


# El efecto de ESTADO va en un campo aparte del de dano, porque hay golpes que
# llevan los dos: FLAREBLITZ es RecoilThirdOfDamageDealtBurnTarget, o sea
# retroceso Y quemadura. Con un solo campo habria que elegir.
SX = ['SX_NONE', 'SX_BURN', 'SX_POISON', 'SX_BADPOISON', 'SX_PARALYZE',
      'SX_SLEEP', 'SX_FREEZE', 'SX_CONFUSE', 'SX_FLINCH',
      'SX_LOWER_ATK', 'SX_LOWER_DEF', 'SX_LOWER_SPA', 'SX_LOWER_SPD',
      'SX_LOWER_SPE', 'SX_LOWER_ACC',
      'SX_RAISE_ATK', 'SX_RAISE_DEF', 'SX_RAISE_SPA', 'SX_RAISE_SPD',
      'SX_RAISE_SPE',
      'SX_RAISE_SPASPDSPE', 'SX_RAISE3_SPA', 'SX_RAISE2_SPASPDSPE',
      'SX_RAISE3_ATK_KO', 'SX_LOWER2_SPA_ATR', 'SX_RAISE_SPASPD_CURA',
      'SX_CONFUSE_ATK2', 'SX_CONFUSE_SPA1']

# Un golpe de apoyo que sube UN escalon casi no se nota; Swords Dance sube DOS
# y por eso vale la pena gastar el turno. Mientras solo habia efectos
# secundarios de golpes de dano daba igual, pero ahora que entran los golpes
# de potencia 0 la diferencia es justo el sentido del golpe.
GRADO2 = {
    'Attack': 'ATK', 'Defense': 'DEF', 'SpAtk': 'SPA',
    'SpDef': 'SPD', 'Speed': 'SPE', 'Accuracy': 'ACC', 'Evasion': 'EVA',
}

# Los que tocan DOS stats a la vez. Sin esto, Dragon Dance seria medio Dragon
# Dance y Calm Mind no tendria ningun sentido.
COMBOS = [
    ('RaiseUserAtkDef1', 'SX_RAISE_ATKDEF'),
    ('RaiseUserAtkSpd1', 'SX_RAISE_ATKSPE'),
    ('RaiseUserAtkSpAtk1', 'SX_RAISE_ATKSPA'),
    ('RaiseUserSpAtkSpDef1', 'SX_RAISE_SPASPD'),
    ('RaiseUserDefSpDef1', 'SX_RAISE_DEFSPD'),
    ('RaiseUserSpeedAtk1', 'SX_RAISE_ATKSPE'),
    # Afilagarras: ataque Y punteria. Lo saben 49 especies y sin esta
    # linea el golpe entero se quedaba fuera de la tabla.
    ('RaiseUserAtkAcc1', 'SX_RAISE_ATKACC'),
    # Rugido de Guerra y Ojos Llorosos: le bajan al RIVAL los dos
    # ataques. Son 27 huecos de especie y estaban fuera de la tabla
    # porque no habia un sx que tocara dos stats DEL OTRO.
    ('LowerTargetAtkSpAtk1', 'SX_LOWER_ATKSPA'),
    # HALAGO: -1 ataque y -1 defensa al rival. Estaba FUERA de la
    # tabla entera —— un golpe de apoyo sin efecto modelado se
    # descarta —— y lo aprenden 17 especies.
    ('LowerTargetAtkDef1', 'SX_LOWER_ATKDEF'),
    # ENROSQUE: ataque, defensa Y punteria. Son 20 huecos de especie
    # y estaba fuera de la tabla por no haber un sx de TRES stats.
    ('RaiseUserAtkDefAcc1', 'SX_RAISE_ATKDEFACC'),
]


# Las descripciones que el .dat del Anil NO traduce. Su traduccion va por
# la frase entera, y si una frase del PBS no esta en el diccionario el
# generador se quedaba con la española — en silencio, y en la tarjeta del
# golpe. Dos de las cuatro eran PLACAJE y GOLPE CUERPO, que se ven en el
# primer combate de la partida.
#
# Se escriben a mano, como ya se hacia con STRUGGLE. Si el Anil las
# traduce algun dia, sobran y se borran; mientras, auditoria.py vigila
# que no vuelva a colarse ninguna.
# Palabras de funcion que no existen como palabra suelta en ingles; ver
# parece_espanol() mas abajo.
ES_PALABRAS = (" el ", " la ", " los ", " las ", " del ", " que ", " con ",
               " su ", " sus ", " se ", " lo ", " al ", " un ", " una ",
               " por ", " para ", "usuario", "objetivo", " puede ")


def parece_espanol(s):
    b = " " + (s or "").lower() + " "
    return any(p in b for p in ES_PALABRAS)


DESC_MANO = {
    'TACKLE': 'A physical attack in which the user charges the target.',
    'BODYSLAM': 'The user drops onto the target with its full body weight. '
                'It may leave the target with paralysis.',
    'FIREBLAST': 'The target is attacked with an intense blast of fire. '
                 'It may also leave the target with a burn.',
    'QUICKGUARD': 'The user and its allies are protected from priority moves. It may fail if used in a row.',
    'HIGHHORSEPOWER': 'The user fiercely attacks the target using its '
                      'entire body.',
}


def estado(code):
    c = code or ''
    # VAN LOS PRIMEROS, y no por capricho: la cascada de abajo busca
    # SUBCADENAS, y 'ConfuseTarget' esta dentro de
    # 'RaiseTargetAttack2ConfuseTarget'. Puesto despues, Contoneo salia
    # como un Rayo Confuso y perdia el +2 —— que es la mitad que lo hace
    # una jugada y no un regalo.
    EXACTOS = {
        # Escama Disparo: o preco/premio vem DEPOIS de todos os impactos
        # (pbEffectAfterAllHits), sem EffectChance. Antes nao fazia nada.
        'HitTwoToFiveTimesRaiseUserSpd1LowerUserDef1': 'SX_SELF_SCALE',
        'RaiseUserSpAtkSpDefSpd1': 'SX_RAISE_SPASPDSPE',
        'TwoTurnAttackRaiseUserSpAtkSpDefSpd2': 'SX_RAISE2_SPASPDSPE',
        'RaiseUserSpAtk3': 'SX_RAISE3_SPA',
        'RaiseUserAttack3IfTargetFaints': 'SX_RAISE3_ATK_KO',
        'LowerTargetSpAtk2IfCanAttract': 'SX_LOWER2_SPA_ATR',
        'RaiseUserSpAtkSpDef1CureStatus': 'SX_RAISE_SPASPD_CURA',
        'RaiseTargetAttack2ConfuseTarget': 'SX_CONFUSE_ATK2',
        'RaiseTargetSpAtk1ConfuseTarget': 'SX_CONFUSE_SPA1',
        # MALDICION. Su clase lleva 'RaiseUserAtkDef1' DENTRO, asi que la
        # cascada de abajo le daba SX_RAISE_ATKDEF —— y entonces un
        # FANTASMA, que con este golpe no sube nada, se subia ataque y
        # defensa igual. Los tres stages (y la mitad de PS que paga el
        # fantasma) los reparte FX_CURSE, que es el unico que sabe de
        # que tipo es el que lo esta usando.
        'CurseTargetOrLowerUserSpd1RaiseUserAtkDef1': 'SX_NONE',
        # ESTRIDENCIA. Misma historia: lleva 'RaiseUserMainStats1' y la
        # cascada le daba SX_RAISE_ALL, o sea los cinco +1 GRATIS. El
        # golpe cuesta un tercio de la vida y falla si no te llega, y
        # eso no cabe en un sx —— lo reparte entero FX_CLANG_SOUL.
        'RaiseUserMainStats1LoseThirdOfTotalHP': 'SX_NONE',
        # BASTION FINAL. Lo mismo: con SX_RAISE_ALL subia los cinco cada
        # turno aunque el golpe "fallase". Lo reparte FX_NORETREAT.
        'RaiseUserMainStats1TrapUserInBattle': 'SX_NONE',
    }
    if c in EXACTOS:
        return EXACTOS[c]
    # Bajadas que se aplica el que PEGA (Overheat baja su propio ataque
    # especial dos escalones, Close Combat su defensa). Van primero porque
    # comparten palabras con las del rival.
    if 'LowerUserSpAtk2' in c:
        return 'SX_SELF_SPA2'
    # El precio de los golpes muy gordos del Anil. Van ANTES que los de
    # abajo porque 'LowerUserDefSpDefSpd1' contiene 'LowerUserDefSpDef1',
    # y 'LowerUserSpeed2' contiene... nada, pero el orden se mantiene por
    # el mismo motivo: estas cadenas se contienen unas a otras.
    if 'LowerUserDefSpDefSpd1' in c:
        return 'SX_SELF_DEFSPDSPE'     # Fuego Explosivo (V-create)
    if 'LowerUserSpeed2' in c:
        return 'SX_SELF_SPE2'          # Giro Acero (Spin Out)
    if 'LowerUserDefense1' in c:
        return 'SX_SELF_DEF'           # Escamas Ruidosas
    if 'LowerUserSpAtk1' in c:
        return 'SX_SELF_SPA'           # Lluvia Monedas
    if 'LowerUserDefSpDef1' in c:
        return 'SX_SELF_DEFSPD'
    if 'LowerUserSpeed1' in c:
        return 'SX_SELF_SPE'
    if 'LowerUserAttack1' in c or 'LowerUserAttackDefense1' in c:
        return 'SX_SELF_ATK'
    if 'RaiseUserMainStats1' in c:
        return 'SX_RAISE_ALL'
    # el orden importa: BadPoison antes que Poison, y las bajadas antes que
    # las subidas porque algunos codigos mencionan las dos
    if 'BadPoison' in c or 'ToxicTarget' in c:
        return 'SX_BADPOISON'
    if 'PoisonTarget' in c:
        return 'SX_POISON'
    if 'BurnTarget' in c:
        return 'SX_BURN'
    if 'ParalyzeTarget' in c:
        return 'SX_PARALYZE'
    if 'SleepTarget' in c:
        return 'SX_SLEEP'
    if 'FreezeTarget' in c:
        return 'SX_FREEZE'
    if 'ConfuseTarget' in c:
        return 'SX_CONFUSE'
    # LOS COLMILLOS. Colmillo Igneo, Colmillo Hielo y Colmillo Rayo traen
    # `BurnFlinchTarget` / `FreezeFlinchTarget` / `ParalyzeFlinchTarget`:
    # DOS efectos del 10% cada uno. Aqui solo cabe uno, y se queda el
    # ESTADO y no el amedrentamiento, porque el estado es lo que distingue
    # a los tres entre si —— con el flinch, los tres eran el mismo golpe
    # con otro color.
    #
    # Van ANTES de la regla general: 'FreezeFlinchTarget' no contiene
    # 'FreezeTarget' (Freeze + Flinch + Target), asi que se escapaba por
    # arriba y caia en el FlinchTarget de abajo.
    if 'BurnFlinchTarget' in c:
        return 'SX_BURN'
    if 'FreezeFlinchTarget' in c:
        return 'SX_FREEZE'
    if 'ParalyzeFlinchTarget' in c:
        return 'SX_PARALYZE'
    if 'FlinchTarget' in c:
        return 'SX_FLINCH'
    for pat, sx in COMBOS:
        if pat in c:
            return sx
    # dos escalones antes que uno: 'RaiseUserAttack2' contiene 'RaiseUserAttack'
    for stat, suf in GRADO2.items():
        if f'LowerTarget{stat}2' in c:
            return 'SX_LOWER2_' + suf
        if f'RaiseUser{stat}2' in c:
            return 'SX_RAISE2_' + suf
    for stat, suf in GRADO2.items():
        if f'LowerTarget{stat}' in c:
            return 'SX_LOWER_' + suf
    for stat, suf in GRADO2.items():
        if f'RaiseUser{stat}' in c:
            return 'SX_RAISE_' + suf
    # AULLIDO es 'RaiseTargetAttack1' aunque se lo lance a si mismo: en el
    # PBS el "objetivo" de un golpe de apoyo propio es el. Sin esta linea
    # se quedaba fuera de la tabla por no tener ningun efecto, y lo saben
    # 24 especies. El sx de subir ya se aplica al que PEGA, asi que vale
    # tal cual.
    for stat, suf in GRADO2.items():
        if f'RaiseTarget{stat}1' in c:
            return 'SX_RAISE_' + suf
    return 'SX_NONE'


# La evasion no existia porque hasta ahora no habia golpes que la tocaran: era
# un stat sin nadie que lo usara. Con Minimize, Double Team y Sweet Scent
# dentro ya tiene sentido, y ademas hace que Sand Attack signifique algo.


# Las banderas del PBS que aqui significan algo. Son un byte por golpe
# (667 de flash) y pagan de sobra:
#
#   Contact   antes se aproximaba con "es fisico", y no es lo mismo:
#             Rayo Burbuja es especial y toca, Bomba Lodo es especial y
#             no. De esto cuelgan Static, Rough Skin y companía.
#   Punching  sin ella no hay forma de hacer Iron Fist
#   Powder    Overcoat (y los tipos Planta) no tragan polvos
#   Sound     para Soundproof, cuando toque
#   Biting    para Strong Jaw, idem
MV_FLAGS = [('Contact', 'MV_CONTACT'), ('Punching', 'MV_PUNCH'),
            ('Powder', 'MV_POWDER'), ('Sound', 'MV_SOUND'),
            ('Biting', 'MV_BITE'), ('Bomb', 'MV_BOMB'),
            ('Kicking', 'MV_KICK'), ('Slicing', 'MV_SLICE'),
            ('Pulse', 'MV_PULSE'), ('Dance', 'MV_DANCE'),
            ('Wind', 'MV_WIND'),
            # NAO e do campo Flags: e "tem EffectChance no PBS", o
            # `addlEffect > 0` do Anil. E o que decide o Sheer Force (o +30%
            # e o que ele apaga). O `chance` da tabela nao serve: o gerador
            # poe 100 nos golpes de auto-baixa (Overheat, Close Combat), que
            # no Anil NAO tem efeito adicional.
            ('<EffectChance>', 'MV_ADDL')]

# El campo `flags` de MoveEntry es de ANCHO FIJO, asi que la bandera
# numero 9 no 'no entra': entra y se pierde el bit de arriba, en
# silencio y en las 708 filas. Con Pulse se llenaba el byte justo, asi
# que el campo paso a uint16_t y esto lo vigila. Si algun dia hacen
# falta mas de 16, hay que ensanchar el campo Y mirar quien lo lee.
assert len(MV_FLAGS) <= 16, (
    '%d banderas no caben en el uint16_t de MoveEntry.flags'
    % len(MV_FLAGS))


def banderas(d):
    """El byte de banderas de un golpe, leido del campo Flags del PBS."""
    suyas = {x.strip() for x in d.get('Flags', '').split(',') if x.strip()}
    v = 0
    for i, (pbs, _c) in enumerate(MV_FLAGS):
        if pbs in suyas:
            v |= 1 << i
        if pbs == '<EffectChance>' and int(d.get('EffectChance', 0) or 0) > 0:
            v |= 1 << i
    return v


def main():
    crudos = {}
    for nombre, d in secciones(os.path.join(PBS, 'moves.txt')):
        crudos[nombre] = dict(tipo=d.get('Type', '?'),
                              cat=d.get('Category', 'Physical'),
                              pot=int(d.get('Power', 0) or 0),
                              pp=pp_de(nombre, d),
                              # ACCURACY 0 EN EL PBS = NUNCA FALLA.
                              # Es como Essentials marca a Swift, Golpe
                              # Aereo, Esfera Aural y otros 216: cero no
                              # es "no acierta nunca", es "no se tira el
                              # dado". Aqui se convertia a 100, y con eso
                              # los 219 pasaban a fallar en cuanto el
                              # rival subia la evasion o a ti te bajaban
                              # la punteria —— justo lo unico que esos
                              # golpes existen para ignorar.
                              #
                              # El motor ya lo tenia puesto: computeHit
                              # salta la tirada con `MOVE_TBL[mv].acc > 0`.
                              # La informacion se perdia AQUI, un paso
                              # antes, y por eso la guarda del motor no
                              # habia servido nunca de nada.
                              #
                              # Un campo que falta se nota; un campo que
                              # llega con otro valor, no.
                              acc=int(d.get('Accuracy', 100) or 0),
                              # LA PRIORIDAD. Estaba en el PBS desde el
                              # principio (55 golpes la traen) y no llegaba
                              # a la placa: Ataque Rapido era un golpe
                              # normal de 40 y el turno lo decidia solo la
                              # velocidad, asi que el golpe cuya razon de
                              # existir es salir primero no salia primero.
                              prio=int(d.get('Priority', 0) or 0),
                              fx=efecto(d.get('FunctionCode', '')),
                              sx=estado(d.get('FunctionCode', '')),
                              # EL 101 DE LOS COLMILLOS. Los tres unicos
                              # golpes del PBS con EffectChance > 100 son
                              # los de dos efectos: el Anil lo escribe asi
                              # para decir "van dos del 10". Tal cual, el
                              # motor hace `random(100) < 101`, que es
                              # SIEMPRE: Colmillo Igneo amedrentaba cada
                              # turno y el rival no se movia nunca.
                              ch=chance_de(d),
                              fl=banderas(d),
                              desc=d.get('Description', '').strip())

    aprende = collections.defaultdict(list)
    for nombre, d in secciones(os.path.join(PBS, 'pokemon.txt')):
        dex = SLUG.get(nombre.split(',')[0].upper())
        if not dex or dex > N_DEX:
            continue
        partes = [p.strip() for p in d.get('Moves', '').split(',')]
        for i in range(0, len(partes) - 1, 2):
            try:
                lv = int(partes[i])
            except ValueError:
                continue
            mv = partes[i + 1]
            m = crudos.get(mv)
            if not m:
                continue
            if m['tipo'] not in C_TIPO:
                continue
            # Los de potencia 0 (Swords Dance, Sunny Day, Recover...) SI entran:
            # antes se descartaban y con ellos se iba la categoria entera de
            # golpes de apoyo. Pero solo si hacen algo que este implementado; si
            # no, seria un golpe en la tarjeta que al usarlo no pasa nada.
            if m['pot'] <= 0 and m['fx'] == 'FX_NONE' and m['sx'] == 'SX_NONE':
                continue
            aprende[dex].append((lv, mv))

    usados = sorted({mv for v in aprende.values() for _, mv in v})
    for mv in usados:
        if len(mv) > MAX_NOMBRE:
            raise SystemExit(f'{mv} tiene {len(mv)} chars y no cabe en la tarjeta')

    # LA ORDEN ESTA CONGELADA. Antes la tabla era STRUGGLE + todo por
    # nombre, y cada golpe nuevo se metia en medio y corria el indice de
    # todos los de detras: las partidas, las cajas del cartao, los
    # equipos archivados, los gNNN.raw y el fx.bin apuntaban de golpe al
    # vecino, y la unica defensa era tirar los movesets elegidos. Ahora
    # las filas de tools/orden_golpes.txt no se mueven nunca y lo nuevo
    # va AL FINAL (por nombre entre si).
    fijos = [l.strip() for l in io.open(
        os.path.join(os.path.dirname(os.path.abspath(__file__)), 'orden_golpes.txt'),
        encoding='utf-8') if l.strip() and not l.startswith('#')]
    assert fijos[0] == 'STRUGGLE', 'la fila 0 de orden_golpes.txt es STRUGGLE'
    perdidos = [mv for mv in fijos if mv not in crudos and mv != 'STRUGGLE']
    if perdidos:
        raise SystemExit('orden_golpes.txt nombra golpes que ya no estan en el '
                         'PBS: %s. Borrarlos correria la tabla: hay que '
                         'decidir que pasa con esas filas' % perdidos)
    sueltos = [mv for mv in fijos[1:] if mv not in usados]
    if sueltos:
        print('  %d golpes de la orden fija ya no los aprende nadie; se '
              'quedan en su fila: %s' % (len(sueltos), ', '.join(sueltos)))
    nuevos = [mv for mv in usados if mv not in set(fijos)]
    if nuevos:
        print('  %d golpes nuevos, al final de la tabla: %s'
              % (len(nuevos), ', '.join(nuevos)))
    tabla = fijos + nuevos                 # indice 0 = el recurso de siempre
    MOVE_COUNT_FIJO = len(fijos)
    idx = {mv: i for i, mv in enumerate(tabla)}
    # Forcejeo toca: es darse de cabezazos contra el rival.
    crudos['STRUGGLE'] = dict(tipo='NORMAL', cat='Physical', pot=50, pp=1, acc=100,
                              fx='FX_NONE', sx='SX_NONE', ch=0, fl=1, prio=0)

    # Y se CUENTAN, para que el dia que alguien vuelva a aplanar el cero
    # salga en el resumen en vez de en una partida.
    nunca = sum(1 for mv in tabla if crudos[mv].get('acc', 100) == 0)
    assert nunca > 100, (
        'solo %d golpes marcados "nunca falla" y el PBS trae 219: alguien '
        'convirtio el cero de Accuracy en 100 otra vez' % nunca)

    huerfanos = [d for d in range(1, N_DEX + 1) if not aprende[d]]
    for d in huerfanos:
        aprende[d] = [(1, 'STRUGGLE')]

    out = ['#pragma once\n#include <stdint.h>\n#include "dex.h"  // T_* y DEX_COUNT\n\n',
           '// GENERADO por tools/gen_moves.py - no editar a mano\n',
           '// Golpes del PBS de Pokemon Anil (llega hasta gen 9), filtrados solo a\n',
           '// los que hacen dano. Son 18 tipos: ver tools/type_chart.py.\n\n',
           'struct MoveEntry {\n'
           '  const char *name;  // ASCII en mayusculas, 16 chars como mucho\n'
           '  uint8_t type;      // T_*\n'
           '  uint8_t power;\n'
           '  uint8_t pp;        // PP maximos, tal cual vienen del PBS\n'
           '  uint8_t phys;      // 1 fisico, 0 especial (campo Category del PBS)\n'
           '  uint8_t fx;        // FX_*: efecto sobre el dano (drenaje, retroceso...)\n'
           '  uint8_t sx;        // SX_*: efecto de estado sobre el rival o sobre uno\n'
           '  uint8_t chance;    // % de que sx ocurra (EffectChance del PBS)\n'
           '  uint8_t acc;       // precision, 0..100\n'
           '  int8_t prio;       // escalon de prioridad del PBS (-7..+5)\n'
           '  uint16_t flags;    // MV_*: las banderas del PBS que usamos\n'
           '};\n\n',
           '// Banderas del golpe (campo Flags del PBS). CONTACT se sacaba\n'
           '// antes de "es fisico", y no es lo mismo: Rayo Burbuja es\n'
           '// especial y toca, Bomba Lodo es especial y no toca.\n'
           '// uint16_t y no uint8_t: con NUEVE banderas, 1 << 8 ya no cabe\n'
           '// en un byte y el enum se habria quedado en 0 sin decir nada.\n'
           '// Ver la guarda de MV_FLAGS en tools/gen_moves.py.\n'
           'enum : uint16_t {\n'
           + ''.join(f'  {c} = 1 << {i},\n'
                     for i, (_p, c) in enumerate(MV_FLAGS))
           + '};\n\n',
           '// Efectos sobre el DANO. Todos son cuentas sobre lo ya calculado o\n'
           '// variantes de la formula: ninguno necesita estado entre turnos.\n'
           'enum : uint8_t {\n'
           '  FX_NONE = 0,\n'
           '  FX_DRAIN,      // cura al que pega la mitad de lo que hizo (Giga Drain)\n'
           '  FX_DRAIN_3_4,  // ...tres cuartos (Draining Kiss)\n'
           '  FX_RECOIL_2,   // el que pega se lleva la mitad\n'
           '  FX_RECOIL_3,   // ...un tercio (Double-Edge)\n'
           '  FX_RECOIL_4,   // ...un cuarto (Take Down)\n'
           '  FX_MULTI_2,    // pega dos veces\n'
           '  FX_MULTI_3,\n'
           '  FX_MULTI_2_5,  // de dos a cinco, como siempre\n'
           '  FX_OHKO,          // fulmina de un golpe; la precision ya es bajisima\n'
           '  FX_FIXED_LVL,     // dano = nivel del que pega (Seismic Toss)\n'
           '  FX_FIXED_LVL_RND, // nivel x 0,5..1,5 (Psywave)\n'
           '  FX_FIXED_40,      // 40 clavados (Dragon Rage)\n'
           '  FX_POW_LOW_HP,    // cuanto menos PS le quedan, mas pega (Flail)\n'
           '  FX_POW_SPEED,     // mas rapido que el rival, mas pega (Electro Ball)\n'
           '  FX_POW_SLOW,      // al reves (Gyro Ball)\n'
           '  FX_POW_STAGES,    // suma por cada stage suyo en positivo (Stored Power)\n'
           '  FX_POW_LESS_PP,   // cuantos menos usos le quedan, mas pega (Trump Card)\n'
           '  // --- los que le hacen algo al rival sin pegarle ---\n'
           '  FX_TAUNT,         // 3 turnos sin poder usar golpes de apoyo\n'
           '  FX_FORESIGHT,     // le quita la evasion y la inmunidad fantasma\n'
           '  FX_TRAP,          // no se puede ir: ni huir ni relevar\n'
           '  FX_ROAR,          // le echa del combate (a un salvaje, se acaba)\n'
           '  FX_MIST,          // 5 turnos sin que le bajen ningun stat\n'
           '  FX_HEAL_BELL,     // quita el estado a TODO el equipo\n'
           '  FX_SMACK_DOWN,    // le baja al suelo: deja de ser volador\n'
           '  // --- los que dependen del combate entero ---\n'
           '  FX_PURSUIT,       // el doble al que se va\n'
           '  FX_UTURN,         // pega y se cambia\n'
           '  FX_LAST_RESORT,   // falla si le queda algun golpe sin usar\n'
           '  FX_ECHO,          // crece si se repite, aunque cambies de bicho\n'
           '  FX_RETALIATE,     // el doble si acaba de caer uno de los tuyos\n'
           '  FX_GASTRO,        // le anula la habilidad (Bilis)\n'
           '  FX_ENTRAIN,       // le pone LA SUYA (Danza Amiga)\n'
           '  FX_IMPRISON,      // le bloquea los golpes que el tambien sabe\n'
           '  FX_SOAK,          // le vuelve de tipo AGUA\n'
           '  FX_WORRY_SEED,    // le cambia la habilidad por Insomnio\n'
           '  FX_LUCKY_CHANT,   // 5 turnos sin criticos contra su bando\n'
           '  FX_ATTRACT,       // enamorado: la mitad de las veces no se mueve\n'
           '  FX_MAGNET_RISE,   // levita 5 turnos: tierra no le entra\n'
           '  FX_QUICK_GUARD,   // este turno, nada de golpes de prioridad\n'
           '  // --- lo que el banquillo hizo posible ---\n'
           '  FX_BATON,         // se cambia Y le pasa los stats al siguiente\n'
           '  FX_TAILWIND,      // su bando va al doble de velocidad 4 turnos\n'
           '  FX_ROCKS,         // piedras en el lado del rival: pican al entrar\n'
           '  FX_SPIKES,        // puas: igual, y se pueden apilar\n'
           '  FX_TSPIKES,       // puas toxicas: envenenan al que entra\n'
           '  FX_WEB,           // red viscosa: le baja la velocidad al entrar\n'
           '  FX_WATERSPORT,    // el fuego pega la mitad mientras dure\n'
           '  FX_MUDSPORT,      // lo mismo con lo electrico\n'
           '  FX_INGRAIN,       // se cura cada turno pero ya no se puede ir\n'
           '  FX_PAINSPLIT,     // se reparten los PS a medias\n'
           '  FX_TRICK,         // se cambian el objeto\n'
           '  FX_HEAL_QUARTER,  // se cura un cuarto (Gota Vital)\n'
           '  FX_TORMENT,       // no le deja repetir golpe\n'
           '  FX_COPYCAT,       // usa el ultimo golpe que se vio\n'
           '  FX_SAME_TYPE,     // falla si no comparten tipo (Sincrorruido)\n'
           '  FX_FIRST_TURN,    // solo el primer turno que sale (Escaramuza)\n'
           '  FX_POW_HIGH_HP,   // cuanto mas entero este EL QUE PEGA, mas\n'
           '  FX_COST_HALF,     // le cuesta la mitad de su vida (Metalaser)\n'
           '  FX_TRAP_BOTH,     // ninguno de los dos se va (Presa Maxilar)\n'
           '  // --- la tanda que cierra los de dano ---\n'
           '  FX_FIXED_20,      // 20 clavados (Bomba Sonica)\n'
           '  FX_POLTERGEIST,   // falla si el rival va sin objeto\n'
           '  FX_HIDDEN_POWER,  // su TIPO sale de los genes del que pega\n'
           '  FX_WEATHER_BALL,  // tipo y potencia segun el cielo\n'
           '  FX_BY_TARGET_STAGES, // suma por cada subida del RIVAL (Castigo)\n'
           '  FX_PRIO_MISTY,    // va primero si hay campo de niebla\n'
           '  FX_DRAGONTAIL,    // pega Y le echa del combate\n'
           '  FX_GAG,           // le calla: 3 turnos sin golpes de sonido\n'
           '  FX_BELCH,         // falla si no se ha comido una baya\n'
           '  FX_TANTRUM,       // el doble si el golpe anterior fallo\n'
           '  FX_FUTURE,        // pega DENTRO de dos turnos\n'
           '  FX_STOCKPILE,     // llena el deposito y sube def y def esp\n'
           '  FX_SPIT_UP,       // lo escupe: potencia segun lo acumulado\n'
           '  FX_SWALLOW,       // se lo traga: cura segun lo acumulado\n'
           '  // --- los tres escondites y lo que los castiga ---\n'
           '  FX_CHARGE_DIG,    // se mete bajo tierra y al otro turno sale\n'
           '  FX_CHARGE_FLY,    // se sube al cielo\n'
           '  FX_CHARGE_DIVE,   // se hunde\n'
           '  FX_X2_IF_UNDER,   // el doble al que esta cavando (Terremoto)\n'
           '  FX_X2_IF_SKY,     // el doble al que esta volando (Tornado)\n'
           '  FX_X2_IF_WATER,   // el doble al que esta buceando (Surf)\n'
           '  FX_HITS_SKY,      // alcanza al que vuela, sin doblar (Gancho Alto)\n'
           '  FX_MAGNITUDE,     // potencia al azar, y el doble si esta cavando\n'
           '  // --- cuentas que solo miran la ficha de los dos ---\n'
           '  FX_X2_IF_POISON,  // el doble al envenenado (Carga Toxica)\n'
           '  FX_IGNORE_STAGES, // pega como si el rival no se hubiera subido nada\n'
           '  FX_FOUL_PLAY,     // pega con el ATAQUE DEL RIVAL (Juego Sucio)\n'
           '  FX_X2_IF_I_HURT,  // el doble si a MI ya me pegaron este turno\n'
           '  FX_BY_TARGET_HP,  // cuanto mas entero este el rival, mas (Estrujon)\n'
           '  FX_HALF_HP,       // le deja a la mitad, sin formula (Superdiente)\n'
           '  FX_X2_ELEC_TERR,  // el doble con campo electrico (Alto Voltaje)\n'
           '  FX_FACADE,        // el doble si el que pega esta mal (Fachada)\n'
           '  FX_METAL_BURST,   // devuelve vez y media lo recibido, del tipo que sea\n'
           '  FX_FOCUS_PUNCH,   // falla si le tocaron este turno (Puno Certero)\n'
           '  FX_WAKE_SLAP,     // el doble al dormido, y le despierta (Espabila)\n'
           '  FX_PRIO_GRASS,    // va primero si hay campo de hierba (Fitoimpulso)\n'
           '  FX_RAGE,          // cada golpe que recibe le sube el ataque (Furia)\n'
           '  FX_UPROAR,        // encadena y NADIE puede dormir (Alboroto)\n'
           '  // --- los que miran que paso ANTES en el turno ---\n'
           '  FX_ONLY_IF_FIRST, // falla si el rival ya se movio (Golpe Bajo)\n'
           '  FX_X2_IF_ACTED,   // el doble si ya se movio (Vendetta)\n'
           '  FX_X2_IF_NOT_ACTED, // ...y estos al reves (Electropico, Fauces)\n'
           '  FX_X2_IF_PARA,    // el doble al paralizado, y le cura (Estimulo)\n'
           '  FX_AFTER_BOLT,    // el doble tras Rayo Fusion (Llama Fusion)\n'
           '  FX_AFTER_FLARE,   // el doble tras Llama Fusion (Rayo Fusion)\n'
           '  FX_X133_IF_SUPER, // un tercio mas si es muy eficaz (Justo Castigo)\n'
           '  FX_X15_SUN,       // mitad mas con sol (Vapor Hidro)\n'
           '  FX_X15_ELEC_TR,   // mitad mas en campo electrico (Psicohoja)\n'
           '  FX_X15_PSY_TR,    // mitad mas en campo psiquico (Vasto Impacto)\n'
           '  FX_RAGE_FIST,     // +50 por cada golpe recibido (Punorrabia)\n'
           '  FX_LAST_RESPECTS, // +50 por cada companero caido (Ultimo Adios)\n'
           '  FX_FICKLE,        // a veces el doble, y se nota (Rayo Umbrio)\n'
           '  FX_BY_BOND_INV,   // cuanto MENOS te quiera, mas (Frustracion)\n'
           '  FX_PRESENT,       // regalo: o pega, o cura al rival\n'
           '  FX_MULTI_10,      // diez impactos (Bomba Demografica)\n'
           '  FX_SUPER_VS_BUG,  // muy eficaz contra bicho pase lo que pase\n'
           '  FX_ALSO_FLYING,   // la eficacia cuenta tambien volador (Plancha)\n'
           '  // --- los que no tienen un tipo fijo ---\n'
           '  FX_TYPE_MINE1,    // del tipo del que pega (Danza Despertar)\n'
           '  FX_TYPE_MINE2,    // de su SEGUNDO tipo (Garrote Hiedra)\n'
           '  FX_TYPE_TERRAIN,  // del terreno que haya (Campo de Pulsos)\n'
           '  FX_NORMAL_IS_ELEC,// los normales pasan a electricos (Punos Plasma)\n'
           '  // --- los que tocan el campo o a uno mismo ---\n'
           '  FX_WIPE_TERRAIN,  // borra el terreno (Bola Acero, Trompo Hielo)\n'
           '  FX_LOSE_FIRE,     // se queda sin su tipo fuego (Ultimo Fuego)\n'
           '  FX_LOSE_ELEC,     // ...sin el electrico (Electrochoque)\n'
           '  FX_IGNORE_ABIL,   // la habilidad del rival no cuenta (Rayo Umbrio Lunar)\n'
           '  FX_BEST_CATEGORY, // pega por donde mas duela (Tera Estallido)\n'
           '  FX_PHOTON,        // ...y ademas la habilidad del rival no cuenta\n'
           '                    // (Geiser Foton: su FunctionCode lo dice)\n'
           '  FX_DRAIN_SLEEP,   // drena, pero solo a quien duerme (Comesuenos)\n'
           '  // La familia del SELF_KO, que estaba entera en un solo caso.\n'
           '  // Siete codigos distintos del PBS y un unico efecto: dos de\n'
           '  // ellos (Lazo Destino y Rencor) ni siquiera tumban a quien los\n'
           '  // usa —— solo MARCAN, por si cae despues.\n'
           '  FX_DESTINY_BOND,  // si caigo este turno, cae quien me tumbo\n'
           '  FX_GRUDGE,        // si caigo, el golpe que me tumbo pierde los PP\n'
           '  FX_FINAL_GAMBIT,  // caigo y hago tanto dano como PS me quedaban\n'
           '  FX_MEMENTO,       // caigo y le dejo el ataque y el at.esp a -2\n'
           '  FX_HEALING_WISH,  // caigo y el que entre llega entero y sano\n'
           '  FX_LUNAR_DANCE,   // ...y ademas con los PP llenos\n'
           '  FX_SALT_CURE,     // le roe cada turno (Salazon)\n'
           '  FX_CURE_BURN,     // le apaga la quemadura (Aria Burbuja)\n'
           '  FX_EXPOSED,       // hasta tu turno te pegan el doble (Asalto Espada)\n'
           '  FX_NO_REPEAT,     // no se puede usar dos veces seguidas (Martillo Giga)\n'
           '  FX_HIT_AND_SEED,  // pega Y planta la semilla (Bombsemilla)\n'
           '  FX_STEAL_STAGES,  // le roba lo que se haya subido (Robo Espectral)\n'
           '  FX_X2_IF_HURT,    // el doble si ya le pegaron (Buena Baza)\n'
           '  FX_X2_IF_HALF,    // el doble si esta por debajo de la mitad\n'
           '  // --- los que suben solos ---\n'
           '  FX_RAMP,          // cada vez que se repite, mas (Corte Furia)\n'
           '  FX_ROLLOUT,       // idem pero obligando a seguir (Rodar)\n'
           '  FX_CURL,          // Rizo Defensa: ademas DOBLA Rodar/Bola Hielo\n'
           '  FX_BY_BOND,       // tanto como te quiera (Retribucion)\n'
           '  FX_BY_WEIGHT,     // segun lo que pese el RIVAL (Patada Baja)\n'
           '  FX_BY_WEIGHT_REL, // segun cuanto mas peses tu (Cuerpo Pesado)\n'
           '  // --- los que tocan objetos ---\n'
           '  FX_EAT_BERRY,     // se come su baya y le hace efecto (Picadura)\n'
           '  FX_BURN_BERRY,    // se la quema sin comersela (Calcinacion)\n'
           '  // --- varios ---\n'
           '  FX_SELF_ATKDEF,   // se baja ataque y defensa (Fuerza Bruta)\n'
           '  FX_SPIN,          // se suelta de lo que le agarraba (Giro Rapido)\n'
           '  FX_KNOCK_OFF,     // +50% si lleva algo, y se lo tira\n'
           '  FX_STEAL,         // se lo queda, si tiene las manos libres\n'
           '  FX_NO_ITEM_X2,    // el doble si el que pega va sin nada\n'
           '  FX_FLING,         // tira lo que lleva: potencia segun que sea\n'
           '  FX_X2_IF_STATUS,  // el doble si el rival esta mal (Hex)\n'
           '  FX_ALWAYS_CRIT,   // critico garantizado (Storm Throw)\n'
           '  FX_VS_DEF,        // especial, pero contra la defensa FISICA (Psyshock)\n'
           '  FX_SELF_KO,       // el que pega cae (Explosion)\n'
           '  FX_CRASH,         // si falla, se estrella y se lleva la mitad (Hi Jump Kick)\n'
           '  // los de varios turnos, los unicos que guardan estado\n'
           '  FX_BIND,          // atrapa 4-5 turnos y va royendo (Bind, Fire Spin)\n'
           '  FX_LOCK,          // obliga a repetir 2-3 turnos y luego confunde (Outrage)\n'
           '  FX_RECHARGE,      // el turno siguiente se pierde (Hyper Beam)\n'
           '  FX_CHARGE,        // un turno cargando y al otro suelta (Solar Beam)\n'
           '  // Clima: no tocan a nadie, cambian las reglas del combate entero.\n'
           '  FX_W_SUN,         // Sunny Day\n'
           '  FX_W_RAIN,        // Rain Dance\n'
           '  FX_W_SAND,        // Sandstorm\n'
           '  FX_W_HAIL,        // Snowscape\n'
           '  // Terreno: como el clima, pero solo alcanza a quien pisa el suelo.\n'
           '  FX_T_ELEC,        // Electric Terrain\n'
           '  FX_T_GRASS,       // Grassy Terrain\n'
           '  FX_T_MISTY,       // Misty Terrain\n'
           '  FX_T_PSY,         // Psychic Terrain\n'
           '  // Cura\n'
           '  FX_HEAL_HALF,     // la mitad de los PS maximos (Recover, Soft-Boiled)\n'
           '  FX_HEAL_WEATHER,  // depende del cielo (Synthesis, Moonlight)\n'
           '  FX_REST,          // se cura del todo y se duerme dos turnos\n'
           '  // Pantallas: duran cinco turnos y protegen a QUIEN LAS PONE\n'
           '  FX_REFLECT,       // la mitad del dano fisico\n'
           '  FX_LIGHTSCREEN,   // la mitad del especial\n'
           '  FX_SAFEGUARD,     // nada de estados\n'
           '  FX_BREAK_SCREENS, // Brick Break: las rompe antes de pegar\n'
           '  // Solo este turno\n'
           '  FX_PROTECT,       // no le entra nada; encadenarlo falla cada vez mas\n'
           '  FX_ENDURE,        // aguanta con 1 PS pase lo que pase\n'
           '  FX_FEINT,         // atraviesa el Protect\n'
           '  // Mover lo que ya hay puesto\n'
           '  FX_HAZE,          // borra TODOS los stages, los suyos tambien\n'
           '  FX_CLEAR_TARGET,  // borra solo los del rival (Clear Smog)\n'
           '  FX_PSYCHUP,       // se copia los del rival\n'
           '  FX_BELLYDRUM,     // ataque al maximo a cambio de medio deposito\n'
           '  // Criticos y punteria\n'
           '  FX_FOCUSENERGY,   // +2 al escalon de critico\n'
           '  FX_LASERFOCUS,    // el siguiente es critico seguro\n'
           '  FX_MINDREADER,    // el siguiente no falla\n'
           '  // Que va royendo (o curando) turno a turno\n'
           '  FX_LEECHSEED,     // le chupa un octavo y se lo queda el que lo puso\n'
           '  FX_AQUARING,      // se cura un dieciseisavo cada turno\n'
           '  FX_CURSE,         // fantasma: medio PS por maldecir. Si no, stats\n'
           '  // Los que CAMBIAN algo del que los usa, o curan segun el otro\n'
           '  FX_PURIFY,        // le quita el estado AL OTRO y se cura la mitad\n'
           '  FX_SAP_ATK,       // se cura lo que valga el ataque del otro, y se lo baja\n'
           '  FX_CLANG_SOUL,    // +1 a los cinco, y paga un tercio de sus PS\n'
           '  FX_AUTOTOMIZE,    // se quita 100 kg de encima (Patada Baja, Cuerpo Pesado)\n'
           '  FX_TIDYUP,        // barre las trampas de los DOS lados\n'
           '  FX_ROOST,         // media cura, y pierde el VOLADOR este turno\n'
           '  FX_DEFOG,         // barre trampas, pantallas y terreno\n'
           '  // Los que estaban fuera de la tabla por no tener efecto\n'
           '  FX_REFRESH,       // se quita quemadura, veneno o paralisis\n'
           '  FX_RECYCLE,       // recupera la baya que se comio\n'
           '  FX_ACUPRESSURE,   // +2 a un stat suyo al azar\n'
           '  FX_VENOM_DRENCH,  // al ENVENENADO: -1 ataque, especial y velocidad\n'
           '  FX_SHORE_UP,      // media cura, dos tercios con tormenta de arena\n'
           '  FX_MIRACLE_EYE,   // le anula la evasion y el siniestro deja de ser intocable\n'
           '  // Tocar los golpes del rival\n'
           '  FX_DISABLE,       // le bloquea el ultimo golpe unos turnos\n'
           '  FX_ENCORE,        // le OBLIGA a repetir el ultimo\n'
           '  FX_SPITE,         // le quita 4 PP al ultimo que uso\n'
           '  // Variantes de la formula\n'
           '  FX_COUNTER,       // devuelve el doble del dano fisico recibido\n'
           '  FX_MIRRORCOAT,    // idem con el especial\n'
           '  FX_ENDEAVOR,      // le deja los PS que tenga el que pega\n'
           '  FX_FALSESWIPE,    // nunca tumba: deja siempre 1 PS\n'
           '  FX_BODYPRESS,     // pega con la DEFENSA en vez de con el ataque\n'
           '  FX_SPLASH,        // no hace nada, y esa es exactamente la gracia\n'
           '  // Los que COPIAN al de enfrente\n'
           '  FX_TRANSFORM,     // se vuelve el otro: especie, stats y golpes\n'
           '  FX_SKETCH,        // se queda con el ultimo golpe que uso el otro\n'
           '  // El cuarto escondite: Golpe Umbrio y Golpe Fantasma. Va al final\n'
           '  // para no correr los numeros de los demas.\n'
           '  FX_CHARGE_GONE,   // desaparece: nada le alcanza ese turno\n'
           '  // La segunda leva, al final por lo mismo.\n'
           '  FX_TRICKROOM,     // 5 turnos: el mas lento va primero\n'
           '  FX_SUBSTITUTE,    // un cuarto de PS en un muneco que recibe por el\n'
           '  FX_WISH,          // al turno siguiente, la mitad de PS al de ese lado\n'
           '  FX_PERISH,        // todos los que lo oyen caen en 3 turnos\n'
           '  FX_EMBARGO,       // 5 turnos sin que su objeto haga nada\n'
           '  FX_HEALBLOCK,     // 5 turnos sin poder curarse\n'
           '  // La tercera leva, al final por lo mismo.\n'
           '  FX_PAYDAY,        // monedas por el suelo (aqui no hay dinero: la frase)\n'
           '  FX_BEAKBLAST,     // calienta el pico: quien le toque esa ronda se quema\n'
           '  FX_SHELLTRAP,     // solo sale si esa ronda le pegaron un fisico\n'
           '  FX_BIDE,          // aguanta dos turnos y devuelve el doble\n'
           '  FX_BEATUP,        // un golpe por cada sano del equipo\n'
           '  FX_NATURALGIFT,   // tipo y potencia de la baya, que se gasta\n'
           '  FX_SECRETPOWER,   // 30% de un efecto que depende del suelo\n'
           '  // La cuarta leva, al final por lo mismo.\n'
           '  FX_YAWN,          // adormece: cae dormido al cerrar el turno siguiente\n'
           '  FX_SHELLSMASH,    // -1 Def y Def Esp, +2 At, At Esp y Vel\n'
           '  FX_TRIATTACK,     // quema, congela o paraliza, a suertes\n'
           '  FX_DIRECLAW,      // envenena, paraliza o duerme, a suertes\n'
           '  FX_FANG,          // su estado Y amedrentar, con dados aparte\n'
           '  FX_TOXICTHREAD,   // envenena y -1 Velocidad\n'
           '  FX_GROWTH,        // +1 At y At Esp, +2 con sol\n'
           '  FX_FREEZEDRY,     // el agua le es superefectiva\n'
           '  FX_RECOIL_HP,     // cuesta la mitad de los PS MAXIMOS\n'
           '  FX_NORETREAT,     // los cinco +1 UNA vez, y ya no se releva\n'
           '  FX_MULTI_3_UP,    // tres impactos de 1x, 2x y 3x, precision en cada uno (Triple Kick)\n'
           '};\n\n',
           '// Efectos de ESTADO. Van en un campo aparte del de dano porque hay\n'
           '// golpes con los dos: FLAREBLITZ tiene retroceso Y quema.\n'
           'enum : uint8_t {\n'
           '  SX_NONE = 0,\n'
           '  SX_BURN, SX_POISON, SX_BADPOISON, SX_PARALYZE, SX_SLEEP, SX_FREEZE,\n'
           '  SX_CONFUSE, SX_FLINCH,\n'
           '  SX_LOWER_ATK, SX_LOWER_DEF, SX_LOWER_SPA, SX_LOWER_SPD,\n'
           '  SX_LOWER_SPE, SX_LOWER_ACC, SX_LOWER_EVA,\n'
           '  // Los cuatro bloques (bajar/subir, x1/x2) tienen que llevar los\n'
           '  // MISMOS siete stats y en el mismo orden: battle.cpp los indexa\n'
           '  // restando el primero de cada bloque. Falto SX_RAISE_ACC una vez y\n'
           '  // Minimize acabo subiendo la punteria en vez de la evasion.\n'
           '  SX_RAISE_ATK, SX_RAISE_DEF, SX_RAISE_SPA, SX_RAISE_SPD, SX_RAISE_SPE,\n'
           '  SX_RAISE_ACC, SX_RAISE_EVA,\n'
           '  // los que se aplica el que PEGA, como precio de un golpe potente\n'
           '  SX_SELF_SPA2, SX_SELF_DEFSPD, SX_SELF_SPE, SX_SELF_ATK, SX_RAISE_ALL,\n'
           '  // el precio de los golpes mas gordos, uno por stat\n'
           '  SX_SELF_DEF, SX_SELF_SPA, SX_SELF_SPE2, SX_SELF_DEFSPDSPE,\n'
           '  // De DOS escalones. Un golpe de apoyo que sube uno solo casi no se\n'
           '  // nota; Swords Dance sube dos y por eso vale gastar el turno.\n'
           '  SX_LOWER2_ATK, SX_LOWER2_DEF, SX_LOWER2_SPA, SX_LOWER2_SPD,\n'
           '  SX_LOWER2_SPE, SX_LOWER2_ACC, SX_LOWER2_EVA,\n'
           '  SX_RAISE2_ATK, SX_RAISE2_DEF, SX_RAISE2_SPA, SX_RAISE2_SPD,\n'
           '  SX_RAISE2_SPE, SX_RAISE2_ACC, SX_RAISE2_EVA,\n'
           '  // dos stats de golpe (Dragon Dance, Calm Mind, Bulk Up)\n'
           '  SX_RAISE_ATKDEF, SX_RAISE_ATKSPE, SX_RAISE_ATKSPA,\n'
           '  SX_RAISE_SPASPD, SX_RAISE_DEFSPD, SX_RAISE_ATKACC,\n'
           '  // y dos del RIVAL de golpe (Rugido de Guerra, Ojos Llorosos)\n'
           '  SX_LOWER_ATKSPA,\n'
           '  SX_LOWER_ATKDEF,\n'
           '  // y TRES del que pega de golpe (Enrosque)\n'
           '  SX_RAISE_ATKDEFACC,\n'
           '  // Los que el mapa resumia de menos. El FunctionCode del PBS\n'
           '  // de Anil dice exactamente lo que hacen; antes se quedaban\n'
           '  // en el pariente mas cercano y perdian la mitad.\n'
           '  SX_RAISE_SPASPDSPE,    // Danza Aleteo: at.esp, def.esp y vel +1\n'
           '  SX_RAISE3_SPA,         // Luz Cola: at.esp +3, no +1\n'
           '  SX_RAISE2_SPASPDSPE,   // Geocontrol: los tres +2, tras cargar\n'
           '  SX_RAISE3_ATK_KO,      // Aguijon Letal: +3, y SOLO si lo tumba\n'
           '  SX_LOWER2_SPA_ATR,     // Seduccion: solo si podria atraerle\n'
           '  SX_RAISE_SPASPD_CURA,  // Animo: ademas le quita el estado\n'
           '  SX_CONFUSE_ATK2,       // Contoneo: le sube el ataque Y le marea\n'
           '  SX_CONFUSE_SPA1,       // Camelo: le sube el at.esp Y le marea\n'
           '  SX_SELF_SCALE,         // Escama Disparo: el que pega +1 Vel y -1 Def\n'
           '};\n\n',
           f'#define MOVE_COUNT {len(tabla)}\n',
           '// Las primeras MOVE_COUNT_FIJO filas tienen la orden congelada\n',
           '// (tools/orden_golpes.txt): una partida guardada con una tabla de\n',
           '// entre MOVE_COUNT_FIJO y MOVE_COUNT golpes sigue valiendo tal cual.\n',
           f'#define MOVE_COUNT_FIJO {MOVE_COUNT_FIJO}\n',
           '#define MOVE_STRUGGLE 0  // para quien no tiene ningun golpe de dano\n\n',
           'static const MoveEntry MOVE_TBL[MOVE_COUNT] = {\n']
    SX_ADICIONAL = {'SX_BURN', 'SX_POISON', 'SX_BADPOISON', 'SX_PARALYZE',
                    'SX_SLEEP', 'SX_FREEZE', 'SX_CONFUSE', 'SX_FLINCH'}
    for mv in tabla:
        m = crudos[mv]
        fl = m.get('fl', 0)
        if m['pot'] and not m['ch'] and m['sx'] in SX_ADICIONAL:
            m = dict(m, sx='SX_NONE')
        nom = ' | '.join(c for i, (_p, c) in enumerate(MV_FLAGS)
                         if fl & (1 << i)) or '0'
        out.append(f'  {{ "{mv}", {C_TIPO[m["tipo"]]}, {m["pot"]}, {m["pp"]}, '
                   f'{1 if m["cat"] == "Physical" else 0}, {m["fx"]}, '
                   f'{m["sx"]}, {m["ch"] if m["ch"] else (100 if m["sx"] != "SX_NONE" else 0)}, '
                   f'{min(100, m["acc"])}, '
                   f'{m.get("prio", 0)}, {nom} }},\n')
    # El ayudante va DETRAS de la tabla: delante no compila, porque
    # nombra MOVE_TBL y MOVE_COUNT.
    out.append('};\n\n'
               '// ¿Lleva el golpe esa bandera? Fuera de rango, no.\n'
               'static inline bool moveHas(uint16_t mv, uint16_t f) {\n'
               '  return mv < MOVE_COUNT && (MOVE_TBL[mv].flags & f);\n'
               '}\n\n')

    # QUE HACE cada golpe, en ingles. Sin esto la pantalla de golpes es una
    # lista de nombres y numeros: el jugador no tiene forma de saber que hace
    # Sweet Scent sin probarlo en un combate de verdad.
    #
    # Mismo camino que las entradas de Pokedex (gen_dexinfo.py): el PBS trae
    # la descripcion en ESPANOL y la traduccion del propio Anil
    # (MessageTypes indice 6) la pasa a ingles. La clave es la frase entera,
    # no un numero, asi que lo que decide es el PBS y no un orden supuesto.
    dicc = traducciones(MT_DESC)
    out.append('// Que hace cada golpe, indexado igual que MOVE_TBL.\n'
               '// Sale del PBS del Anil traducido con su propio .dat.\n'
               'static const char *const MOVE_DESC[MOVE_COUNT] = {\n')
    faltan = 0
    sinTraducir = []
    filas_es, filas_pt = [], []
    for mv in tabla:
        # STRUGGLE se sintetiza aparte y no viene del PBS, pero es de los
        # que mas se ven (sale cuando no queda nada), asi que se escribe a
        # mano en vez de dejarlo en blanco.
        es = crudos[mv].get('desc', '')
        if mv in DESC_MANO:
            en = DESC_MANO[mv]
        else:
            en = dicc.get(es, es)
            # `dicc.get(es, es)` cae al ESPAÑOL cuando falta la entrada, y
            # lo hacia en silencio: Placaje y Golpe Cuerpo (de los golpes
            # que mas se ven en todo el juego) salian en la tarjeta en
            # español. Un fallback no puede ser invisible, asi que se
            # apunta y se canta al final.
            # "Falta en el diccionario" NO es lo mismo que "sale en
            # español": los golpes propios del Anil ya traen su texto EN
            # INGLES en el PBS, asi que no estan en el diccionario y no
            # hay nada que arreglar. Lo que se busca es texto español sin
            # traducir. Sin este filtro saltaba Steam Veil, que estaba
            # bien, y se habria "arreglado" lo que no estaba roto.
            if es and en == es and parece_espanol(es):
                sinTraducir.append(mv)
        if mv == 'STRUGGLE' and not en:
            en = 'Used when no moves are left. It also hurts the user.'
        if not en:
            faltan += 1
        out.append(f'  {cadena_c(ascii_plano(en))},\n')
        # el espanol y el portugues de ESTA fila, recogidos aqui mismo
        if mv == 'STRUGGLE' and not es:
            es = 'Se usa cuando no quedan golpes. Tambien hiere al usuario.'
        filas_es.append(es)
        filas_pt.append(PT_GOLPES.get(mv, ''))
    out.append('};\n\n')
    if faltan:
        print(f"  OJO: {faltan} golpes sin descripcion")
    if sinTraducir:
        print(f"  OJO: {len(sinTraducir)} descripciones que el .dat del Anil "
              f"no traduce y saldrian en ESPAÑOL: "
              f"{', '.join(sinTraducir[:10])}")
        print(f"       añadelas a DESC_MANO en este fichero")

    # ---------------- el mismo texto en español y en portugués ----------------
    # Salen del MISMO bucle de arriba, no de una segunda pasada: por eso no
    # pueden desalinearse de MOVE_TBL aunque la tabla crezca.
    #
    # El español es el campo Description del PBS tal cual -- es de donde el
    # inglés se traduce, así que no cuesta nada. El portugués está escrito a
    # mano en tools/pt_golpes.py, indexado por slug.
    #
    # Los acentos NO pasan por ascii_plano(): van a los bytes de la fuente
    # (CP437 más los dos códigos del til, ver texto_pt.py). El inglés sí
    # sigue pelado, porque no tiene ninguno que perder.
    for nom, filas, quien in (('ES', filas_es, 'el PBS'),
                              ('PT', filas_pt, 'tools/pt_golpes.py')):
        hechas = sum(1 for f in filas if f)
        # El slug esta aqui mismo (las filas salen del bucle de arriba, en
        # el orden de `tabla`), asi que el error puede decir QUE golpe es
        # en vez de solo que caracter. Sin esto hay que buscarlo a mano.
        malas = []
        for slug, f in zip(tabla, filas):
            if not f:
                continue
            try:
                texto_pt.codifica(f, slug)
            except texto_pt.NoCabe as e:
                malas.append(str(e))
        if malas:
            print(f'  {len(malas)} descripciones {nom} con algun caracter '
                  f'que la fuente no tiene:')
            for m in malas:
                print(f'    {m}')
            raise SystemExit(1)
        out.append(f'// Lo mismo en {nom}, de {quien}. Misma fila que MOVE_TBL.\n')
        if nom == 'PT':
            out.append('// nullptr = sin traducir todavia: moveDesc() cae al ingles.\n')
        out.append(f'static const char *const MOVE_DESC_{nom}[MOVE_COUNT] = {{\n')
        for f in filas:
            if not f:
                out.append('  nullptr,\n')
                continue
            out.append(f'  "{texto_pt.codifica(f)}",\n')
        out.append('};\n\n')
        print(f'  {nom}: {hechas}/{len(filas)} descripciones '
              f'({sum(len(f) for f in filas) / 1024:.1f} KB)')

    # un slug de pt_golpes.py que no exista es una errata que se ve en
    # cuanto se mira, pero solo si alguien mira
    sobran = sorted(set(PT_GOLPES) - set(tabla))
    if sobran:
        print(f'  OJO: {len(sobran)} slugs en pt_golpes.py que no son '
              f'ningun golpe: {", ".join(sobran[:8])}')

    # pares (nivel, indice) seguidos, con un indice de inicio por especie
    # Convencion del PBS de Essentials: -1 = "lo aprende AL EVOLUCIONAR" y
    # 0 = "ya lo sabe al nacer". Los dos significan "lo tiene desde el
    # principio de esta forma", asi que valen como nivel 1. Sin esto se cuelan
    # niveles negativos en una tabla de uint8_t.
    datos, ofs = [], [0]
    for d in range(1, N_DEX + 1):
        for lv, mv in sorted((max(1, min(lv, 255)), mv) for lv, mv in aprende[d]):
            datos.append((lv, idx[mv]))
        ofs.append(len(datos))
    out.append('// Nivel e indice de golpe en DOS arrays paralelos, de la especie 1 a la\n'
               '// una detras de otra. LEARN_OFS[d-1]..LEARN_OFS[d] delimita los de la especie d.\n'
               '//\n'
               '// Por que separados y no pares en un solo uint8_t: hay mas de 255 golpes\n'
               '// (y con las siguientes generaciones seran muchos mas), asi que el indice\n'
               '// necesita uint16_t. El nivel cabe de sobra en uint8_t, y mezclarlos\n'
               '// gastaria un byte por par para nada.\n')
    out.append(f'static const uint8_t LEARN_LVL[{len(datos)}] = {{\n')
    for i in range(0, len(datos), 16):
        out.append('  ' + ' '.join(f'{lv:3d},' for lv, _ in datos[i:i + 16]) + '\n')
    out.append('};\n')
    out.append(f'static const uint16_t LEARN_MV[{len(datos)}] = {{\n')
    for i in range(0, len(datos), 12):
        out.append('  ' + ' '.join(f'{ix:3d},' for _, ix in datos[i:i + 12]) + '\n')
    out.append('};\n')
    out.append('static const uint16_t LEARN_OFS[DEX_COUNT + 1] = {\n')
    for i in range(0, len(ofs), 12):
        out.append('  ' + ' '.join(f'{v},' for v in ofs[i:i + 12]) + '\n')
    out.append('};\n\n')
    out.append(
        '// Los hasta 4 ultimos golpes que la especie ya sabe a ese nivel, como en\n'
        '// el juego: se aprenden en orden y los viejos se caen. Devuelve cuantos.\n'
        '// out4 es uint16_t porque los indices pasan de 255.\n'
        'static inline uint8_t movesAtLevel(int16_t dex, uint16_t level, uint16_t *out4) {\n'
        '  if (dex < 1 || dex > DEX_COUNT) { out4[0] = MOVE_STRUGGLE; return 1; }\n'
        '  uint16_t a = LEARN_OFS[dex - 1], b = LEARN_OFS[dex];\n'
        '  uint8_t n = 0;\n'
        '  for (uint16_t i = a; i < b; i++) {\n'
        '    if (LEARN_LVL[i] > level) break;  // la lista va ordenada por nivel\n'
        '    uint16_t mv = LEARN_MV[i];\n'
        '    bool rep = false;\n'
        '    for (uint8_t k = 0; k < n; k++) if (out4[k] == mv) rep = true;\n'
        '    if (rep) continue;\n'
        '    if (n < 4) out4[n++] = mv;\n'
        '    else { out4[0] = out4[1]; out4[1] = out4[2]; out4[2] = out4[3]; out4[3] = mv; }\n'
        '  }\n'
        '  if (!n) { out4[0] = MOVE_STRUGGLE; n = 1; }  // por debajo del primer nivel\n'
        '  return n;\n'
        '}\n\n'
        '// TODOS los que ya sabe, no solo los cuatro ultimos. Es la lista de la\n'
        '// pantalla donde se eligen: a nivel 50 un bicho conoce 17 de media y\n'
        '// hasta 31, asi que quedarse con los ultimos cuatro tira mucho a la\n'
        '// basura. Devuelve cuantos ha metido, en orden de aprendizaje.\n'
        'static inline uint8_t movesKnown(int16_t dex, uint16_t level,\n'
        '                                 uint16_t *out, uint8_t max) {\n'
        '  if (dex < 1 || dex > DEX_COUNT || !max) return 0;\n'
        '  uint16_t a = LEARN_OFS[dex - 1], b = LEARN_OFS[dex];\n'
        '  uint8_t n = 0;\n'
        '  for (uint16_t i = a; i < b && n < max; i++) {\n'
        '    if (LEARN_LVL[i] > level) break;\n'
        '    uint16_t mv = LEARN_MV[i];\n'
        '    bool rep = false;\n'
        '    for (uint8_t k = 0; k < n; k++) if (out[k] == mv) rep = true;\n'
        '    if (!rep) out[n++] = mv;\n'
        '  }\n'
        '  if (!n) { out[0] = MOVE_STRUGGLE; n = 1; }\n'
        '  return n;\n'
        '}\n\n'
        '// PP de verdad, con los numeros de siempre.\n'
        '//\n'
        '// Antes esto convertia el PP a "usos por combate" (2 a 6), porque con los\n'
        '// PP reales y restauracion automatica un Tackle de 35 no se agota nunca\n'
        '// dentro de un combate y el numero sobra. Pero PP es un termino que todo\n'
        '// el mundo entiende y que no hay que explicar en una pantalla de 466 px,\n'
        '// y la tension del PP nunca estuvo en el combate suelto: esta en la\n'
        '// TRAVESIA hasta el sitio donde se recupera. Aqui ese sitio es el ciclo\n'
        '// de cuidado del bicho -dormir y las bayas-, asi que el PP atraviesa los\n'
        '// combates y cuidar al bicho es lo que te deja seguir peleando.\n'
        'static inline uint8_t movePP(uint16_t mv) {\n'
        '  if (mv >= MOVE_COUNT) return 5;\n'
        '  if (mv == MOVE_STRUGGLE) return 255;  // el ultimo recurso no se gasta\n'
        '  uint8_t p = MOVE_TBL[mv].pp;\n'
        '  return p ? p : 5;\n'
        '}\n')

    with open(DST, 'w', encoding='utf-8') as f:
        f.write(''.join(out))

    peso = sum(len(m) + 1 for m in tabla) + len(tabla) * 8 + len(datos) * 2 + len(ofs) * 2
    print(f'guardado {os.path.normpath(DST)}')
    print(f'  {len(tabla)} golpes (1 es STRUGGLE), {len(datos)} pares de learnset')
    print(f'  sin ningun golpe de dano: {[d for d in huerfanos]}')
    print(f'  nombre mas largo: {max(len(m) for m in tabla)} chars')
    print(f'  peso estimado: ~{peso / 1024:.1f} KB de flash')


if __name__ == '__main__':
    main()
