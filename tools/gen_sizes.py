#!/usr/bin/env python3
"""El tamano de cada especie en pantalla, a tools/dex_sizes.py.

QUE ES EL NUMERO QUE SALE DE AQUI
---------------------------------
`PANTALLA_PX[dex]` es el alto del CUERPO del bicho, en pixeles de la
pantalla de casa. Del cuerpo: de la TINTA. No del lienzo del sprite, que
trae huecos y padding y no es lo que nadie ve. El firmware (escalaDibujo /
factorTinta en TamaPoke.ino) llega a ese alto con la escala entera que
toque, y el combate le aplica encima su perspectiva.

Y POR QUE ASI, QUE COSTO CINCO INTENTOS
---------------------------------------
Los cinco ajustaron algo que no era lo que el ojo lee:

  1. `170 / alto_del_lienzo` en el firmware: normalizaba. Un Eevee salia
     tan alto como un Charizard.
  2. el alto del sprite del Anil: depende de la POSE. Pidgeotto esta
     dibujado volando (116 px de lienzo) y Pidgeot posado (68), asi que
     evolucionar encogia; 55 evoluciones salian mas pequenas que su forma
     anterior.
  3. una correccion por densidad de tinta encima de (2): acertaba de
     media y no por bicho, y seguia siendo un parche sobre el lienzo.
  4. topes de altura por rol en el combate: aplastaban a 475 de 1025
     rivales contra el mismo numero, con lo que media Pokedex salia
     exactamente igual de grande.
  5. y el de verdad, que no estaba en esta tabla: el combate dibujaba sin
     decir QUE especie pintaba, asi que el rival se escalaba con el
     screenH del bicho del JUGADOR. Agrandar el tuyo agrandaba al rival,
     y por eso ningun retoque de estos numeros podia arreglarlo.

LA FUENTE: EL AREA DE TINTA DEL SPRITE
--------------------------------------
Ni la altura ni el peso de la ficha. Las dos mienten, y en direcciones
contrarias:

  altura   Dratini "mide" 1,8 m —— mas que un Charizard —— porque son
           1,8 m de serpiente de 3,3 kg. Onix (8,8 m) y Wailord (14,5 m)
           topan los dos arriba y empatan con cualquier otro grandote.
  peso     lo probe y lo tumbo el usuario con dos contraejemplos: "tem
           pokemon pequeno q eh pesado, e pokemon grande q e leve".
           Cosmoem son 0,1 m y 999 kg; Gastly 1,3 m y 0,1 kg, y con el
           peso el Gastly caia al suelo de la banda —— "meu gastly ficou
           quase invisivel na tela".

Lo que el ojo lee es el DIBUJO. Y del dibujo, el AREA de tinta: a
diferencia del alto, aguanta la pose. Un Pidgeotto volando con las alas
abiertas es alto y ancho, pero la tinta de su cuerpo es la que es —— y la
pose era justo lo que rompia el intento (2). La raiz cuadrada pasa de
area a medida lineal, que es lo que se escala, y el exponente de encima
comprime el rango para que quepa en la banda.

    px = A * (sqrt(pixeles_de_tinta) ** EXP)

Medido sobre las 1025 con EXP 0,60: ninguna se sale de la banda.

Y SE APLICA COMO SITIO, NO COMO ALTO
------------------------------------
Esto es lo ultimo que faltaba, y lo trajo una lista del usuario:
"charizard, beedril, pidgeotto, fearow, raichu, machoke, machamp,
executtor, totodile, vespiqueen - aumentar / wiglytuff ta menor que o
jiglypuff, kinkdra mto pequeno, blacephalon - aumentar bastante / snom -
diminuir".

Lo que tienen en comun los de "aumentar": son ALTOS Y ESTRECHOS. El
unico de "diminuir", Snom, es redondo. Medido: aspecto alto/ancho 1,40
de media contra 1,10 del plantel, y Blacephalon —— el que pedia "aumentar
bastante" —— es el mas estirado de todos (2,10) y el que menos area
dibujaba (22% de la mediana).

El motivo: se normalizaba el ALTO de tinta. A igual alto, un sprite
estirado ocupa mucho menos SITIO que uno redondo, y el sitio es lo que el
ojo lee. Wigglytuff lo ensena de una: sale 18 px MAS ALTO que su
Jigglypuff y con la MITAD de area, asi que evolucionar parecia encoger
—— y audita_tamanos.py no lo veia, porque comparaba alturas.

Asi que lo que se normaliza es una mezcla de los dos:

    magnitud = alto_tinta^(1-B) * sqrt(area_tinta)^B      B = 0,65

B=0 es normalizar el alto (de donde venimos) y B=1 el area pura, que
arregla la forma pero manda 178 especies contra el techo de la pantalla
—— aplastarlas todas al mismo alto es justo el fallo del que se venia.
Con 0,65 quedan 31. El firmware usa el MISMO B (PMD_B_AREA en sdmon.h) y
audita_tamanos.py comprueba que no se separen.

GANANCIA: la lista eran 12 "aumentar" contra 1 "diminuir", o sea que
ademas de repartir distinto habia que subir. 1,18 deja a los 12 creciendo
(el que menos, x1,09; Blacephalon x1,44) y a Snom bajando a x0,90.

  python3 tools/gen_sizes.py
"""
import math
import os
import sys

AQUI = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, AQUI)
import anil_pbs
import tinta
from dex_range import N_DEX

DST = os.path.join(AQUI, 'dex_sizes.py')


# LA BANDA, y sale de la geometria de la pantalla, no de probar numeros.
#
#   casa      el bicho vive entre la cabecera (acaba en y~105) y la linea
#             de suelo (PET_GROUND = 304): 199 px. El techo es 185.
#   combate   de espaldas hay 217 px (suelo 312, chapa de PS del rival en
#             y=95) y 185*1,05 = 194. De frente hay 137 (el circulo, en
#             x=325) y 185*0,72 = 133. Ver BT_BACK_K / BT_FOE_K.
#
# EL SUELO sube a 88 y no es cosmetico: con 64 el usuario reporto que
# "meu gastly ficou quase invisivel na tela de cuidados". En la pantalla
# de cuidados hay UN bicho y nada con que compararlo, asi que su tamano
# absoluto es lo unico que se lee; 64 px de cuerpo en una pantalla de 466
# es una mancha.
#
# Con el area de tinta como fuente, subir el suelo ya casi no cuesta:
# UNA especie cae debajo (con el peso eran 88). El suelo esta para que
# nadie desaparezca, no para repartir la banda.
# LA BANDA, en unidades de MAGNITUD (no de alto: ver el docstring).
#
# 74..170 no es a ojo y no se recalcula aqui: salio de medir el plantel
# una vez. Se tomo la magnitud que cada especie ocupaba con el esquema
# anterior (que normalizaba el alto), se subio la mediana un 18% —— la
# lista del usuario eran 12 "aumentar" contra 1 "diminuir" —— y se abrio
# la banda a +-42% alrededor de esa mediana. Con eso:
#
#   los 12 de "aumentar" crecen entre x1,09 y x1,44
#   Snom baja a x0,90
#   Wigglytuff pasa por fin a su Jigglypuff
#   31 de 1025 llegan al techo de alto de la pantalla
#
# Esta ESCRITO y no calculado a proposito. La version anterior lo sacaba
# de la tabla ya generada, o sea de su propia salida: la segunda pasada
# tomaba como referencia lo que acababa de escribir y la banda se movia
# sola (74..170 -> 65..150 -> ...). Un generador tiene que dar lo mismo
# cada vez que se corre.
PX_MIN, PX_MAX = 74, 170
# Cuanto se hace caso al area frente al alto. TIENE que ser el mismo que
# PMD_B_AREA en sdmon.h.
B_AREA = 0.65
# El alto util de la pantalla de casa, para contar cuantos tocan techo.
ALTO_MAX_CASA = 190
# La compresion sobre la raiz del area. Con 1,0 (sin comprimir) 139 de
# las 1025 se salen de la banda; con 0,60 no se sale ninguna y el leque
# sigue siendo de 1,9x, que se lee de sobra.
EXP = 0.60
# Donde estan los sprites ya empaquetados para la tarjeta.
MONS = os.path.join(AQUI, 'sdcard', 'mons')


def _peso_kg(campos):
    try:
        return max(0.1, float(campos.get('Weight', '10') or 10))
    except ValueError:
        return 10.0


def main():
    alturas = {n: anil_pbs.altura_dm(c) for n, _slug, c in anil_pbs.filas()}
    pesos = {n: _peso_kg(c) for n, _slug, c in anil_pbs.filas()}

    # A se fija para que la MEDIANA del plantel caiga en el centro
    # geometrico de la banda. Asi la banda se reparte sola y no hay que
    # recalibrar a mano cada vez que entra una region nueva.
    areas, tintas = {}, {}
    for n in range(1, N_DEX + 1):
        ruta = os.path.join(MONS, 'p%03d.bin' % n)
        a = tinta.area_idle(ruta)
        t = tinta.alto_idle(ruta)
        if a and t:
            areas[n] = a
            tintas[n] = float(t)
            # Lo que esa especie ocupa AHORA, para poder expresar el
            # cambio como una ganancia y no como un numero absoluto.
    if not areas:
        raise SystemExit('no hay sprites en %s: empaqueta la tarjeta antes'
                         % MONS)

    # A se fija para que la MEDIANA del plantel caiga en el centro
    # geometrico de la banda. Asi la banda se reparte sola y no hay que
    # recalibrar a mano cada vez que entra una region nueva.
    crudos = {n: math.sqrt(a) for n, a in areas.items()}
    med = sorted(crudos.values())[len(crudos) // 2]

    # La MAGNITUD de cada sprite: la misma mezcla que hace el firmware.
    mag = {n: (tintas[n] ** (1.0 - B_AREA)) * (crudos[n] ** B_AREA)
           for n in areas}

    LO, HI = float(PX_MIN), float(PX_MAX)
    A = math.sqrt(LO * HI) / (med ** EXP)

    px = {}
    arriba = abajo = 0
    for n, c in crudos.items():
        v = A * (c ** EXP)
        if v > HI:
            arriba += 1
        if v < LO:
            abajo += 1
        px[n] = int(max(LO, min(HI, round(v))))
    PX_LO, PX_HI = int(round(LO)), int(round(HI))
    faltan = [n for n in range(1, N_DEX + 1) if n not in px]
    for n in faltan:                      # sin sprite: al centro, y se avisa
        px[n] = int(round(math.sqrt(PX_MIN * PX_MAX)))
    techo = sum(1 for n in areas
                if tintas[n] * (px[n] / mag[n]) > ALTO_MAX_CASA)
    print('  %d sprites medidos; banda de magnitud %d..%d'
          % (len(areas), PX_LO, PX_HI))
    print('  %d topan arriba, %d abajo (%.0f%% intactos); '
          '%d llegan al techo de alto'
          % (arriba, abajo,
             100.0 * (len(crudos) - arriba - abajo) / len(crudos), techo))
    if faltan:
        print('  AVISO: %d sin sprite, puestos al centro de la banda: %s'
              % (len(faltan), faltan[:10]))

    # ------------------------------------------------------------------
    # EVOLUCIONAR NO PUEDE HACERTE MAS PEQUENO.
    #
    # Se reporto asi: "pidgey evolui pro pidgeotto mas parece q ele
    # desevoluiu". Con el peso esto ya casi no pasa (evolucionar engorda),
    # pero "casi" no sirve: hay parejas que adelgazan en la ficha
    # (Shedinja pesa 1,2 g) y el jugador no perdona ver encoger al suyo
    # justo en el momento en que crece.
    #
    # Se pasa varias veces porque las cadenas son de tres: subir al del
    # medio puede dejar corto al ultimo.
    # En PORCENTAJE de area, no en pixeles de magnitud: lo que se exige
    # es que la evolucion OCUPE mas sitio, y un 3% es lo menos que se ve.
    MARGEN = 3
    nums = anil_pbs.numeros()
    evoDe = {}
    for i, (_cab, c) in enumerate(anil_pbs.bloques()):
        origen = i + 1
        if origen > N_DEX:
            continue
        campos = [x.strip() for x in c.get('Evolutions', '').split(',')]
        for j in range(0, len(campos) - 2, 3):
            destino = nums.get(anil_pbs.clave(campos[j]))
            if destino and destino <= N_DEX and destino != origen:
                evoDe[destino] = origen

    def area_dibujada(n):
        """Lo que esa especie OCUPA en pantalla, con el recorte de alto
        ya aplicado. Es lo que hay que comparar: la magnitud no vale,
        porque dos bichos con la misma magnitud y distinta forma ocupan
        distinto sitio —— que es justo el fallo que trajo aqui."""
        e = px[n] / mag[n]
        if tintas[n] * e > ALTO_MAX_CASA:
            e = ALTO_MAX_CASA / tintas[n]
        return areas[n] * e * e

    subidos = 0
    for _vuelta in range(10):
        cambio = False
        for destino, origen in sorted(evoDe.items()):
            if origen not in px or destino not in px:
                continue
            falta = area_dibujada(origen) * (1.0 + MARGEN / 100.0)
            tengo = area_dibujada(destino)
            if tengo >= falta or tengo <= 0:
                continue
            # El area va con el CUADRADO de la escala, y la escala con la
            # magnitud: para multiplicar el area por r hay que multiplicar
            # la magnitud por raiz de r. Se salta directo en vez de
            # tantear de uno en uno —— tanteando no llegaba y el
            # generador acababa diciendo que los que faltaban topaban
            # contra la pantalla, que era FALSO en cuatro de cinco.
            quiero = int(math.ceil(px[destino] * math.sqrt(falta / tengo)))
            nuevo = min(PX_MAX, max(px[destino] + 1, quiero))
            if nuevo == px[destino]:
                continue
            antes = tengo
            px[destino] = nuevo
            if area_dibujada(destino) <= antes:
                px[destino] = nuevo - 1 if nuevo > antes else nuevo
                # si ni subiendo mejora, es que el recorte de alto manda
                px[destino] = min(nuevo, PX_MAX)
                continue
            cambio = True
            subidos += 1
        if not cambio:
            break

    quedan = [(n, evoDe[n]) for n in evoDe
              if n in px and evoDe[n] in px
              and area_dibujada(n) < area_dibujada(evoDe[n]) * 0.98]
    print('  %d subidas para que ninguna evolucion encoja EN AREA' % subidos)
    if quedan:
        # Se dice POR QUE, y comprobandolo, no suponiendolo: o la magnitud
        # ya esta al maximo de la banda, o el sprite toca el techo de alto
        # de la pantalla. Si no es ninguna de las dos, es un fallo de esta
        # pasada y hay que verlo, no taparlo con una frase.
        porTecho = porBanda = raro = 0
        for n_, _o in quedan:
            e = px[n_] / mag[n_]
            if tintas[n_] * e > ALTO_MAX_CASA:
                porTecho += 1
            elif px[n_] >= PX_MAX:
                porBanda += 1
            else:
                raro += 1
        print('  %d evoluciones siguen encogiendo en area: %d topan contra '
              'el alto de la pantalla, %d contra el techo de la banda'
              % (len(quedan), porTecho, porBanda))
        if raro:
            print('  OJO: %d sin explicacion —— esta pasada se quedo corta'
                  % raro)

    # La curva de respaldo por ALTURA. Solo la usan una mega o una forma
    # cuyo sprite falte en la tarjeta: con el sprite delante se mide su
    # area, igual que aqui, usando la calibracion que se escribe abajo.
    # Que las tres tablas compartan calibracion no es cosmetico —— si cada
    # generador calculara la suya sobre su propio plantel, una mega
    # saldria en otra escala que su forma base.
    REF_M, REF_PX, EXP_M = 1.7, 128.0, 0.30

    with open(DST, 'w', encoding='utf-8', newline='\n') as f:
        f.write('"""GENERADO por tools/gen_sizes.py.\n\n'
                'No editar a mano.\n\n'
                'PANTALLA_PX[dex] = la MAGNITUD que el bicho tiene que'
                ' ocupar en la pantalla de casa:'
                ' alto_tinta^(1-B) * sqrt(area_tinta)^B, con'
                ' B = B_AREA. No es un alto: un sprite estirado y uno'
                ' redondo con la misma magnitud ocupan el mismo SITIO'
                ' y distinto alto, que es lo que el ojo lee. Ver el'
                ' docstring de gen_sizes.py.\n"""\n\n')
        # La banda DE VERDAD, la que se acaba de usar: PX_MIN/PX_MAX
        # son solo el punto de partida y la ganancia los mueve.
        f.write(f'PX_MIN, PX_MAX = {PX_LO}, {PX_HI}\n')
        f.write(f'B_AREA = {B_AREA!r}\n')
        f.write(f'REF_M, REF_PX, EXP = {REF_M}, {REF_PX}, {EXP_M}\n\n')
        # La calibracion del area, para que gen_megas.py y
        # gen_forms.py midan SUS sprites con esta misma regla.
        f.write('# calibracion del area (la comparten las tres tablas)\n')
        f.write(f'AREA_A, AREA_EXP = {A!r}, {EXP!r}\n\n')
        f.write('# dex: altura en decimetros y peso en kg. Se guardan\n'
                '# porque la ficha los ENSENA, pero NO dan el tamano.\n'
                'ALTURA_DM = {\n')
        for n in range(1, N_DEX + 1):
            f.write(f'    {n}: {alturas[n]},\n')
        f.write('}\n\n')
        f.write('PESO_KG = {\n')
        for n in range(1, N_DEX + 1):
            f.write(f'    {n}: {pesos.get(n, 10.0)},\n')
        f.write('}\n\n')
        f.write('def px_para(metros):\n'
                '    """Ultimo recurso por ALTURA, para una mega o\n'
                '    forma cuyo sprite falte. Con sprite se mide el\n'
                '    area, que es la regla de verdad."""\n'
                '    v = REF_PX * (metros / REF_M) ** EXP\n'
                '    return int(max(PX_MIN, min(PX_MAX, round(v))))\n\n')
        f.write('PANTALLA_PX = {\n')
        for n in range(1, N_DEX + 1):
            f.write(f'    {n}: {px.get(n, PX_MIN)},\n')
        f.write('}\n')

    print(f'escrito {DST}')
    for n, nombre in ((50, 'DIGLETT'), (10, 'CATERPIE'), (16, 'PIDGEY'),
                      (25, 'PIKACHU'), (92, 'GASTLY'), (147, 'DRATINI'),
                      (17, 'PIDGEOTTO'), (95, 'ONIX'), (6, 'CHARIZARD'),
                      (321, 'WAILORD')):
        if n in px:
            print('  %-10s %3d px  (area %d, %.1f m, %.1f kg)'
                  % (nombre, px[n], areas.get(n, 0), alturas[n] / 10.0,
                     pesos[n]))


if __name__ == '__main__':
    main()
