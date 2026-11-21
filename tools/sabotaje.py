#!/usr/bin/env python3
"""Rompe a proposito y exige que las comprobaciones lo canten.

Esto es lo que separa un verificador de un adorno. `auditoria.py` y
`contraste.py` dicen "sin fallos"; esto demuestra que esa frase
significa algo, metiendo un error conocido en un .h y comprobando que
salta — y que salta nombrando al bicho, no con un numero suelto.

Lo escribo porque el fallo que mas veces se ha repetido en este
proyecto no es un calculo mal hecho: es una comprobacion que dejo de
comprobar. Un patron que no casa devuelve CERO diferencias, y cero se
lee igual que "todo bien". Un verificador que nunca ha fallado no ha
demostrado nada.

Cada sabotaje toca UNA linea, corre la herramienta, restaura el fichero
y mira el resultado. Si algo se queda a medias, el .h original esta en
la copia temporal que se imprime.

  python3 tools/sabotaje.py
"""
import io
import os
import shutil
import subprocess
import sys
import tempfile

AQUI = os.path.dirname(os.path.abspath(__file__))
RAIZ = os.path.dirname(AQUI)

# (herramienta, fichero, texto que hay que encontrar, con que se cambia,
#  a quien tiene que nombrar el fallo, como se llama el sabotaje)
#
# El texto a buscar es literal a proposito: si un generador cambia el
# formato de su tabla, este fichero deja de encontrar donde morder y
# avisa, en vez de dar por bueno un sabotaje que no llego a hacerse.
SABOTAJES = [
    # El sonido de cada golpe. Este es el sabotaje del fallo que lo
    # provoco: con la firma corrida una fila, el Lick tocaba el sonido
    # del golpe siguiente. Si la auditoria no lo canta, no hay nada que
    # impida que vuelva a pasar al meter un golpe nuevo.
    ("auditoria.py", "tools/sdcard/snd/golpes.txt",
     # La fila es la de LICK en la tabla CONGELADA (tools/orden_golpes.txt).
     # Era 344 hasta que el 2026-11-02 se reordeno la tabla: el ancla dejo
     # de casar y este sabotaje llevaba desde entonces sin hacerse.
     "345 LICK ", "345 LEER ",
     "LICK", "la firma del sonido corrida una fila"),

    ("contraste.py", "dex.h",
     '{ "PIKACHU", 26, 0, R_COMUN, 0xBCA1, 35, 55, 30,',
     '{ "PIKACHU", 26, 0, R_COMUN, 0xBCA1, 35, 55, 31,',
     "PIKACHU", "un stat base cambiado"),

    ("contraste.py", "dex.h",
     "T_GRASS, T_POISON, 50 },  // 1 planta",
     "T_FIRE, T_POISON, 50 },  // 1 planta",
     "BULBASAUR", "un tipo cambiado"),

    ("contraste.py", "dex.h", '{ "IVYSAUR", 3, 32,', '{ "IVYSAUR", 3, 33,',
     "IVYSAUR", "un nivel de evolucion cambiado"),

    ("contraste.py", "dex.h", '{ "CHARMELEON", 6, 36,',
     '{ "CHARMELEON", 7, 36,',
     "CHARMELEON", "una evolucion que apunta a otro bicho"),

    # El conflicto de una biyeccion se ve en la SEGUNDA especie que use
    # la clave, no en la que se toco: por eso aqui se espera IVYSAUR.
    ("contraste.py", "abilities.h",
     "{ 156,   0,  27 },  // 1", "{ 157,   0,  27 },  // 1",
     "IVYSAUR", "un indice de habilidad cambiado"),

    ("contraste.py", "abilities.h",
     "{ 156,   0,  27 },  // 2", "{ 156,   0,   0 },  // 2",
     "IVYSAUR", "una habilidad oculta borrada"),

    ("auditoria.py", "forms.h", '"A.RATTATA" },', '"A.RATICATE" },',
     "A.RATICATE", "dos formas con el mismo rotulo"),

    ("auditoria.py", "megas.h", '"M.VENUSAUR X" },', '"M.VENUSAUR XYZ" },',
     "M.VENUSAUR XYZ", "un nombre de mega que no cabe en la ficha"),

    # tms.h guarda INDICES de golpe, asi que quedarse atras de moves.h no
    # es perder una animacion: es que la MT ensena OTRO golpe. Paso de
    # verdad y en silencio, porque gen_tms.py no estaba en gen_todo.py.
    # Esto comprueba que la red nueva lo pilla.
    ("auditoria.py", "tms.h", "#define TM_MOVE_COUNT",
     "#define TM_MOVE_COUNT 1  //",
     "TM_MOVE_COUNT", "unas MT hechas contra otra tabla de golpes"),

    # El PBS del Anil esta en español y la traduccion va por la frase
    # entera: si falta una, el generador embarcaba la española. Pasaron
    # cuatro, y dos eran Placaje y Golpe Cuerpo. Esto comprueba que
    # ahora se canta.
    ("auditoria.py", "moves.h",
     '"A physical attack in which the user charges the target.",',
     '"Embiste con el cuerpo.",',
     # El fallo tiene que ENSEÑAR la frase culpable, no decir "hay una":
    # con 708 descripciones, "hay una en español" no sirve de nada.
     "Embiste", "una descripcion de golpe embarcada en espanol"),

    # La lista de avisos de la pantalla es un switch, y un switch se
    # olvida: habia 19 efectos de 34 que se aplicaban en silencio. Esto
    # comprueba que quitar un aviso se canta.
    ("auditoria.py", "battle_ui.ino",
     "case FX_TAILWIND:", "case FX_W_SUN: // quitado a proposito",
     "FX_TAILWIND", "un efecto que la pantalla deja de contar"),

    # female.h dice con un bit quien tiene dibujo de hembra, y el aparato
    # se fia de ese bit para pedir el fichero. Marcar a uno que no lo
    # tiene deja el combate sin dibujo, asi que el bit y la tarjeta
    # tienen que decir lo mismo. Se enciende el PRIMER byte del mapa,
    # que cubre las especies 0..7:
    # ninguna de ellas tiene hembra, asi que el bit miente y el fichero
    # h001.bin no existe. Es exactamente el fallo que importa.
    ("auditoria.py", "female.h",
     "  0x08, 0x10, 0x18, 0x06,",
     "  0xFF, 0x10, 0x18, 0x06,",
     "falta h001", "una hembra marcada que no esta en la tarjeta"),

    # --- la tarjeta prestada al PC (modo USB-MSC) ---
    # El primero es EL fallo que estaba de verdad en el codigo: "LS"
    # leia la tarjeta sin mirar nada, y una guarda que mirase la funcion
    # entera no lo veia, porque esa funcion SI nombra la puerta... en
    # otra rama. Por eso la guarda mira la RAMA.
    ("auditoria.py", "sdmon.cpp",
     '  } else if (line == "LS") {\n    if (!sdUsable()) {',
     '  } else if (line == "LS") {\n    if (false) {',
     "sin sdUsable", "una rama que lee la tarjeta prestada al PC"),

    ("auditoria.py", "sdmon.h",
     "return sdReady && !sdPrestada;", "return sdReady;",
     "la guarda miente", "sdUsable() que se olvida del prestamo"),

    ("auditoria.py", "usbdisco.cpp",
     "    sdPrestada = true;\n    escritos = 0;",
     "    escritos = 0;",
     "cerrarle la puerta", "prestar sin cerrarle la puerta al firmware"),

    ("auditoria.py", "TamaPoke.ino",
     "if (usbMscPrestada() && usbMscExpulsada()) {",
     "if (usbMscPrestada() && false) {",
     "usbMscExpulsada", "la tarjeta no se recupera al expulsarla"),

    # --- los dos golpes COPIA ---
    # El Ditto embarcaba sabiendo solo Forcejeo y nadie se entero hasta
    # que el dueno capturo uno con la placa en la mano y abrio su menu.
    # El filtro de gen_moves descarta los golpes de apoyo sin efecto
    # implementado -- regla buena -- pero a un bicho se le pueden caer
    # TODOS, y entonces no es un golpe que no hace nada: es un bicho que
    # no puede pelear. Quitando el efecto, el golpe vuelve a caerse.
    ("auditoria.py", "battle_ui.ino",
     "            case FX_TRANSFORM:", "            case FX_TRANSFORM_NO:",
     "sin que la pantalla", "una transformacion que no se cuenta en pantalla"),

    ("auditoria.py", "battle_ui.ino",
     "            case FX_SKETCH:", "            case FX_SKETCH_NO:",
     "sin que la pantalla", "un Esquema que no se cuenta en pantalla"),
]


def corre(tool):
    r = subprocess.run([sys.executable, os.path.join(AQUI, tool)],
                       capture_output=True, text=True, cwd=RAIZ)
    return r.returncode, r.stdout + r.stderr


def main():
    tools = []
    for s in SABOTAJES:
        if s[0] not in tools:
            tools.append(s[0])
    for t in tools:
        code, out = corre(t)
        if code != 0:
            print(f"{t} YA falla sin tocar nada: no se puede medir con el")
            print(out[-1500:])
            return 1
    print(f"de partida, {' y '.join(tools)} pasan\n")

    tmp = tempfile.mkdtemp(prefix="sabotaje_")
    bien, mal = 0, []
    for tool, fich, busca, pon, quien, que in SABOTAJES:
        p = os.path.join(RAIZ, fich)
        copia = os.path.join(tmp, fich.replace("/", os.sep))
        # subcarpetas: la firma del sonido vive en tools/sdcard/snd, no
        # en la raiz, y sin esto el shutil.copy2 no encuentra donde
        # guardar el original.
        os.makedirs(os.path.dirname(copia), exist_ok=True)
        shutil.copy2(p, copia)
        txt = io.open(p, encoding="utf-8").read()
        if busca not in txt:
            mal.append(f"{que}: no encuentro donde morder en {fich} "
                       f"(¿cambio el formato de la tabla?)")
            continue
        io.open(p, "w", encoding="utf-8", newline="\n").write(
            txt.replace(busca, pon, 1))
        code, out = corre(tool)
        shutil.copy2(copia, p)
        # copy2 devuelve la fecha VIEJA: un make que generara algo del fichero
        # saboteado lo creeria al dia y no lo rehace (paso con sketch.cpp).
        os.utime(p, None)
        if code == 0:
            mal.append(f"{que}: {tool} NO lo pilla")
        elif quien and quien not in out:
            mal.append(f"{que}: {tool} falla pero no nombra a {quien}")
        else:
            print(f"  ok  {que}  ({tool})")
            bien += 1

    for t in tools:
        code, _ = corre(t)
        if code != 0:
            print(f"\nOJO: {t} sigue fallando tras restaurar. Los originales "
                  f"estan en {tmp}")
            return 1
    shutil.rmtree(tmp, ignore_errors=True)

    print()
    for m in mal:
        print(f"  !   {m}")
    print(f"\n{bien} sabotajes pillados, {len(mal)} no")
    return 1 if mal else 0


if __name__ == "__main__":
    sys.exit(main())
