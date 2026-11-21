#!/usr/bin/env python3
"""El codepage del firmware, de ida y de vuelta.

La pantalla dibuja con la 5x7 clasica de Arduino_GFX, que es una tabla
CP437 completa: un byte por caracter. Por eso las traducciones no van en
UTF-8 sino con escapes octales, y por eso centrar con strlen()*6 sigue
valiendo. Lo que falta en CP437 y el portugues necesita son a~ y o~, que
tienen dos codigos propios (ver i18n.h) y los pinta printT().

  codifica("evolucao")  -> 'evolu\\347\\261o'   (ejemplo: c-cedilla + o~)
  descodifica(...)      -> el texto de vuelta, para poder LEER lo embarcado

Rechaza en vez de estropear
---------------------------
Un caracter que la fuente no tiene se puede resolver de tres maneras:
dibujarlo mal, quitarle el acento en silencio, o avisar. Las dos primeras
ya pasaron: las cadenas PT de hoy dicen "NAO" y "EVOLUCAO" porque alguien
(yo) quito los acentos y no lo apunto en ningun sitio. Asi que avisa.

Las MAYUSCULAS A~/O~ se rechazan aparte, con su motivo: no es que falten
en la fuente, es que no CABEN. La 'A' y la 'O' de la 5x7 ya ocupan la fila
0 de la celda y el til no tiene donde ponerse sin meterse en la linea de
encima. Si una frase las necesita hay que escribirla de otra forma
("Evolucao" en vez de "EVOLUCAO"), que es decision de traductor, no algo
que deba decidir un script por su cuenta.

  python3 tools/texto_pt.py            # lee las cadenas PT de i18n.cpp
  python3 tools/texto_pt.py --todas    # los ocho idiomas
"""
import argparse
import io
import os
import re
import sys

AQUI = os.path.dirname(os.path.abspath(__file__))
RAIZ = os.path.dirname(AQUI)

COMILLA = chr(34)
BARRA = chr(92)
ESC = BARRA + BARRA

# Los dos codigos que printT() dibuja a mano. Caen en los huecos de dibujo
# de cajas de CP437 (0260 = 0xB0, 0261 = 0xB1), que este proyecto no usa.
A_TIL = 0xB0
O_TIL = 0xB1
A_TIL_MAY = 0xB2
O_TIL_MAY = 0xB3
PROPIOS = {"ã": A_TIL, "õ": O_TIL,
           "Ã": A_TIL_MAY, "Õ": O_TIL_MAY}
# Las MAYUSCULAS llevan el til FUERA de la celda, tres filas mas arriba:
# vale en un boton o un titulo y no en medio de un parrafo, donde se
# metria en la linea de encima. Se avisa de cada una para poder mirarla.
AVISAN = {"Ã": "A~", "Õ": "O~"}
avisados = []

# Lo que se arregla sin preguntar: tipografia que no aporta nada en una
# pantalla de 5x7 y que CP437 no tiene.
APANOS = {
    "’": "'", "‘": "'", "“": COMILLA, "”": COMILLA,
    "–": "-", "—": "-", "…": "...", " ": " ",
}


class NoCabe(Exception):
    pass


# Mayusculas acentuadas que CP437 no tiene, con la letra a la que caen
# cuando el texto es IMPORTADO. En espanol quitarle el acento a una
# mayuscula es lo normal; en lo que escribo yo no se permite.
SIN_GLIFO_MAY = {
    "\u00c1": "A", "\u00c0": "A", "\u00c2": "A", "\u00c4": "A",
    "\u00cd": "I", "\u00cc": "I", "\u00ce": "I", "\u00cf": "I",
    "\u00d3": "O", "\u00d2": "O", "\u00d4": "O", "\u00d6": "O",
    "\u00da": "U", "\u00d9": "U", "\u00db": "U", "\u00dc": "U",
    "\u00d1": "N",
}
pelados = []


def codifica(texto, donde="", importado=False):
    """Texto en UTF-8 -> cadena C con escapes octales de un byte.

    `importado` es para el texto que viene del PBS del Anil y no se puede
    reescribir: a una mayuscula acentuada sin glifo se le quita el acento
    y se apunta en `pelados`. Para lo escrito a mano se deja en False, que
    es lo que obliga a escribirlo bien.
    """
    for k, v in APANOS.items():
        texto = texto.replace(k, v)
    out = []
    for ch in texto:
        if ch in AVISAN:
            aviso = ("%s'%s' (%s) lleva el til FUERA de la celda, tres filas "
                     "mas arriba: mira la captura si no es un boton o un "
                     "titulo" % (donde and donde + ": ", ch, AVISAN[ch]))
            if aviso not in avisados:
                avisados.append(aviso)
        if ch in PROPIOS:
            out.append(PROPIOS[ch])
            continue
        if ch == BARRA:
            out.append(ord(BARRA))
            continue
        try:
            b = ch.encode("cp437")
        except UnicodeEncodeError:
            if importado and ch in SIN_GLIFO_MAY:
                aviso = "%s'%s' -> '%s'" % (donde and donde + ": ", ch,
                                            SIN_GLIFO_MAY[ch])
                if aviso not in pelados:
                    pelados.append(aviso)
                out.append(ord(SIN_GLIFO_MAY[ch]))
                continue
            raise NoCabe(
                "%s'%s' (U+%04X) no esta en CP437 y no tiene codigo propio"
                % (donde and donde + ": ", ch, ord(ch)))
        out.append(b[0])

    # Ya en bytes: a cadena C. Solo se escapan los que lo necesitan, para
    # que el .cpp siga siendo legible ("Est\240 feliz" y no todo en octal).
    s = []
    for b in out:
        if b == ord(COMILLA):
            s.append(BARRA + COMILLA)
        elif b == ord(BARRA):
            s.append(ESC)
        elif 32 <= b < 127:
            s.append(chr(b))
        else:
            s.append(BARRA + "%03o" % b)
    return "".join(s)


# ----------------------------------------------------------- de vuelta
OCTAL = re.compile(re.escape(BARRA) + r"([0-7]{1,3})")


def descodifica(cadena):
    """Cadena C del firmware -> texto legible, para poder auditarlo."""
    # primero los escapes que no son octales
    cadena = cadena.replace(ESC, "\x00BARRA\x00")
    cadena = cadena.replace(BARRA + COMILLA, COMILLA)
    cadena = cadena.replace(BARRA + "n", "\n").replace(BARRA + "t", "\t")
    trozos = []
    pos = 0
    for m in OCTAL.finditer(cadena):
        trozos.append(cadena[pos:m.start()].encode("latin-1", "replace"))
        trozos.append(bytes([int(m.group(1), 8) & 0xFF]))
        pos = m.end()
    trozos.append(cadena[pos:].encode("latin-1", "replace"))
    crudo = b"".join(trozos)

    out = []
    for b in crudo:
        if b == A_TIL:
            out.append("ã")
        elif b == O_TIL:
            out.append("õ")
        elif b == A_TIL_MAY:
            out.append("Ã")
        elif b == O_TIL_MAY:
            out.append("Õ")
        else:
            out.append(bytes([b]).decode("cp437", "replace"))
    return "".join(out).replace("\x00BARRA\x00", BARRA)


# ------------------------------------------- leer las tablas de i18n.cpp
PAT = re.compile(COMILLA + "((?:[^" + COMILLA + ESC + "]|" + ESC + ".)*)"
                 + COMILLA)
IDIOMAS = ["ES", "EN", "FR", "DE", "IT", "PT", "JA", "KO"]


def bloques():
    """{idioma: [frases tal cual estan en el .cpp]}."""
    s = io.open(os.path.join(RAIZ, "i18n.cpp"), encoding="utf-8",
                errors="surrogateescape").read()
    i = s.index("static const char *const STRINGS")
    out = {}
    for n, idi in enumerate(IDIOMAS):
        marca = "// ---------------- %s ----------------" % idi
        a = s.index(marca, i)
        sig = (s.index("// ---------------- %s ----" % IDIOMAS[n + 1], a)
               if n + 1 < len(IDIOMAS) else s.index("};", a))
        out[idi] = PAT.findall(s[a:sig])
    return out


# Frases donde falta un acento que la fuente SI tiene. No se puede
# detectar sola (hay palabras sin acento de verdad), asi que esto es una
# lista de lo que hay que mirar: palabras portuguesas que casi siempre
# llevan acento o til.
SOSPECHOSAS = [
    "NAO", "Nao", "nao", "EVOLUCAO", "Evolucao", "evolucao", "GINASIO",
    "Ginasio", "ginasio", "FOLEGO", "Folego", "folego", "OTIMO", "Otimo",
    "otimo", "MEDIO", "ACAO", "Acao", "acao", "ORGAO", "SAUDE", "Saude",
    "ATENCAO", "Atencao", "POCAO", "Pocao", "pocao", "MUSICA", "Musica",
    "PROXIMO", "Proximo", "proximo", "ULTIMO", "Ultimo", "ultimo",
    "SONO", "Sono",
]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--todas", action="store_true",
                    help="los ocho idiomas, no solo el portugues")
    a = ap.parse_args()

    bl = bloques()
    quienes = IDIOMAS if a.todas else ["PT"]
    malas = 0
    for idi in quienes:
        fr = bl[idi]
        print("---- %s (%d frases) ----" % (idi, len(fr)))
        for n, t in enumerate(fr):
            d = descodifica(t)
            marca = ""
            if idi == "PT":
                for sosp in SOSPECHOSAS:
                    if re.search(r"\b" + sosp + r"\b", d):
                        marca = "  <-- falta acento?"
                        malas += 1
                        break
            print("%3d  %s%s" % (n, d, marca))
    if not a.todas:
        print()
        print("%d frases PT con pinta de faltarle el acento" % malas)
    return 0


if __name__ == "__main__":
    sys.exit(main())
