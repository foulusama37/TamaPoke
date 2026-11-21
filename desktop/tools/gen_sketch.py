#!/usr/bin/env python3
"""Convierte TamaPoke.ino en un .cpp compilable fuera del IDE de Arduino.

El IDE inserta solo los prototipos de las funciones antes de compilar; el .ino
se apoya en eso (drawMap se usa 130 lineas antes de definirse, por ejemplo).
Aqui se hace lo mismo a mano: se buscan las definiciones de funcion a nivel
superior y se inserta el bloque de prototipos JUSTO ANTES de la primera, que es
donde ya estan declarados todos los tipos y globales que puedan aparecer en las
firmas (PetFx, PetNeed, PmdMon...).
"""
import os
import re
import sys

import glob

AQUI = os.path.dirname(os.path.abspath(__file__))
RAIZ = os.path.normpath(os.path.join(AQUI, '..', '..'))
INO = os.path.join(RAIZ, 'TamaPoke.ino')
OUT = os.path.normpath(os.path.join(AQUI, '..', 'build', 'sketch.cpp'))


def extras():
    """Los demas .ino de la carpeta, en orden alfabetico.

    El IDE de Arduino pega TODOS los .ino del sketch en un solo fichero: primero
    el que da nombre a la carpeta y luego el resto por orden. Aqui se hace igual,
    porque battle_ui.ino usa helpers de TamaPoke.ino (drawPmdActM, tintEllipse,
    el canvas gfx...) y solo compila si van juntos.
    """
    todos = sorted(glob.glob(os.path.join(RAIZ, '*.ino')))
    return [p for p in todos if os.path.abspath(p) != os.path.abspath(INO)]

# tipo(s) + nombre + (args) + { , empezando en la columna 0.
# El no-codicioso hasta un espacio/*/& deja el nombre en el grupo 2, asi que
# cubre "const char *eggMsg(" igual que "int centerX(". Y no se exige que la
# llave cierre la linea, porque hay definiciones de una sola linea
# ("int centerX(...) { return ...; }").
# OJO: DEF pide el ')' y la '{' en la MISMA linea, asi que una firma
# partida en dos no recibe prototipo. Casi siempre da igual (la funcion
# esta declarada en un .h, o definida antes de usarse), pero si ademas se
# llama antes de definirse, el error sale lejos: "no se declaro en este
# ambito", en la primera llamada. Paso con drawWrapped al anadirle un
# parametro. La regla, entonces: en los .ino, las firmas de una linea.
#
# Hubo aqui un aviso automatico y se quito: daba falsos positivos con
# funciones que compilan de sobra y, cuando se afino, dejo de ver el caso
# de verdad. Un comprobador que no pilla lo que dice pillar es peor que
# no tenerlo, porque se confia en el.
DEF = re.compile(r'^([A-Za-z_][\w:*&<>,\s]*?[\s*&])([A-Za-z_]\w*)\s*\(([^;{)]*)\)\s*\{')
NO_SON_TIPO = {'if', 'for', 'while', 'switch', 'else', 'return', 'do', 'case', 'struct',
               'class', 'enum', 'union', 'namespace', 'template'}


def lee(path):
    """El .ino, SIN la marca de orden de bytes.

    Un editor de Windows (o un Set-Content de PowerShell) le pone un BOM
    al fichero. En el primero da igual, pero aqui se PEGAN uno detras de
    otro como hace el IDE de Arduino, y el BOM del segundo cae en mitad
    del .cpp: gcc dice "does not name a type" en una linea en blanco y
    atar eso con "guarde el fichero" lleva un rato. Paso de verdad con
    battle_ui.ino, y el firmware seguia compilando porque arduino-cli
    compila cada .ino por separado: solo se rompia el simulador.
    """
    return open(path, encoding='utf-8-sig').read()


def main():
    src = lee(INO).split('\n')
    for extra in extras():
        src.append(f'\n// ---- {os.path.basename(extra)} (pegado como hace el IDE) ----')
        src += lee(extra).split('\n')

    protos, primera = [], None
    for i, ln in enumerate(src):
        m = DEF.match(ln)
        if not m:
            continue
        ret, nombre, args = m.group(1), m.group(2), m.group(3)
        if ret.strip().split()[0] in NO_SON_TIPO:
            continue
        if nombre in ('setup', 'loop'):
            continue
        if primera is None:
            primera = i
        # ret conserva el espacio o el '*' final, asi que no se pega al nombre
        protos.append(f'{ret}{nombre}({args});')

    if primera is None:
        sys.exit('no se encontro ninguna definicion de funcion')

    bloque = ['', '// --- prototipos generados por desktop/tools/gen_sketch.py ---']
    # los que ya estan declarados a mano en el .ino se repiten sin problema:
    # en C++ redeclarar identico es legal. Los que llevan argumento por defecto
    # NO se pueden repetir, asi que se saltan.
    for p in protos:
        if '=' in p.split('(', 1)[1]:
            continue
        bloque.append(p)
    bloque.append('// --- fin de los prototipos ---')
    bloque.append('')

    salida = src[:primera] + bloque + src[primera:]
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(OUT, 'w', encoding='utf-8') as f:
        f.write('\n'.join(salida))
    print(f'{OUT}: {len(protos)} funciones, prototipos en la linea {primera}')


if __name__ == '__main__':
    main()
