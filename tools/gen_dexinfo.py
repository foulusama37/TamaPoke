#!/usr/bin/env python3
"""Genera dex_text.h: la entrada de Pokedex de cada especie, en ingles.

La fuente es la TRADUCCION del propio proyecto Anil V4.13. Hacen falta los
dos ficheros a la vez, y este es el detalle que no es obvio:

  - `PBS/pokemon.txt` dice QUE entrada le toca a cada bicho, pero en ESPANOL
    (el PBS nunca se tradujo).
  - `Data/messages_english_game.dat` es un volcado de Marshal cuyo indice 3
    es un hash **{texto original en espanol => texto en ingles}**. No esta
    indexado por numero de Pokedex: la clave es la frase entera.

Asi que el camino es PBS -> espanol -> diccionario -> ingles. De paso eso
hace la asignacion a prueba de bombas, porque quien decide de quien es cada
descripcion es el propio PBS y no un orden supuesto.

  python3 tools/gen_dexinfo.py
"""
import os
import re
import sys
import unicodedata

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import marshal_lite  # noqa: E402
import texto_pt  # noqa: E402
from dex_data import DEX  # noqa: E402
from pt_dex import FICHAS as PT_FICHAS  # noqa: E402
from dex_range import N_DEX  # noqa: E402

ANIL = r"C:\Users\foulu\Downloads\Pokemon Anil V4.13"
DAT = os.path.join(ANIL, "Data", "messages_english_game.dat")
PBS = os.path.join(ANIL, "PBS", "pokemon.txt")
RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DST = os.path.join(RAIZ, "dex_text.h")

MT_ENTRADAS = 3
N = N_DEX

SLUG = {d[1].upper(): d[0] for d in DEX}
SLUG["NIDORANFE"] = 29   # el PBS los llama NIDORANfE / NIDORANmA
SLUG["NIDORANMA"] = 32


def ascii_plano(s):
    """La fuente del firmware es la 5x7 de siempre: solo ASCII imprimible.

    'Pokémon' pasa a 'Pokemon' y las comillas tipograficas a las rectas. Se
    hace aqui y no en la pantalla para que lo que se ve sea exactamente lo
    que hay en el fichero.
    """
    s = (s.replace("\u2019", "'").replace("\u2018", "'")
          .replace("\u201c", '"').replace("\u201d", '"')
          .replace("\u2013", "-").replace("\u2014", "-")
          .replace("\u2026", "..."))
    s = unicodedata.normalize("NFKD", s)
    s = "".join(c for c in s if not unicodedata.combining(c))
    s = s.replace("\n", " ").replace("\r", " ")
    s = re.sub(r"\s+", " ", s).strip()
    return "".join(c for c in s if 32 <= ord(c) < 127)


def nombres_de_dex_h():
    """Los nombres que ya usa el firmware, para poder cotejar el resultado."""
    txt = open(os.path.join(RAIZ, "dex.h"), encoding="utf-8").read()
    return re.findall(r'\{\s*"([A-Z0-9.\'\- ]+)"', txt)


def pokedex_en_espanol():
    """dex -> la frase en espanol del PBS, que es la clave del diccionario."""
    out, cur = {}, None
    for ln in open(PBS, encoding="utf-8", errors="replace"):
        ln = ln.strip()
        m = re.match(r"^\[(.+)\]$", ln)
        if m:
            cur = SLUG.get(m.group(1).split(",")[0].upper())
            continue
        if cur and cur <= N and ln.startswith("Pokedex") and "=" in ln:
            out.setdefault(cur, ln.split("=", 1)[1].strip())
    return out


def main():
    for p in (DAT, PBS):
        if not os.path.exists(p):
            raise SystemExit(f"no encuentro {p}")
    dicc = marshal_lite.load(DAT)[MT_ENTRADAS]
    # el hash viene con claves en bytes: se pasa a str una vez y ya
    dicc = {k.decode("utf-8", "replace"): v.decode("utf-8", "replace")
            for k, v in dicc.items()
            if isinstance(k, bytes) and isinstance(v, bytes)}
    es = pokedex_en_espanol()

    dexh = nombres_de_dex_h()
    textos, sin_es, sin_en = [], [], []
    # el espanol y el portugues de la MISMA vuelta, para que no puedan
    # desalinearse del ingles
    textos_es, textos_pt = [], []
    for dex in range(1, N + 1):
        frase = es.get(dex)
        textos_es.append(frase or "")
        textos_pt.append(PT_FICHAS.get(dex, ""))
        if not frase:
            sin_es.append(dex)
            textos.append("")
            continue
        ing = dicc.get(frase)
        if ing is None:
            sin_en.append(f"{dex} {dexh[dex] if dex < len(dexh) else '?'}")
            textos.append(ascii_plano(frase))   # mejor en espanol que en blanco
            continue
        textos.append(ascii_plano(ing))

    if sin_es:
        print(f"  sin entrada en el PBS: {sin_es}")
    if sin_en:
        print(f"  {len(sin_en)} sin traducir (se quedan en espanol): {sin_en[:8]}")

    vacios = [i + 1 for i, t in enumerate(textos) if not t]
    total = sum(len(t) + 1 for t in textos)

    out = [
        "// GENERADO por tools/gen_dexinfo.py -- no editar a mano\n",
        "// Entradas de Pokedex, sacadas de la traduccion inglesa del\n",
        "// propio Anil V4.13 (Data/messages_english_game.dat). Solo ASCII: la\n",
        "// fuente del firmware es la 5x7 de siempre.\n",
        "//\n",
        f"// {N} entradas, {total} bytes de texto. Van en FLASH porque son 17 KB de\n",
        "// los 2,1 MB libres; cuando entren las otras generaciones (1027 entradas,\n",
        "// ~120 KB) toca moverlas a la SD, al lado de los sprites.\n",
        "#pragma once\n\n",
        "// indice 0 sin usar, para poder escribir DEX_TEXT[dex]\n",
        "static const char *const DEX_TEXT[] = {\n",
        '  "",\n',
    ]
    for t in textos:
        out.append('  "%s",\n' % t.replace("\\", "\\\\").replace('"', '\\"'))
    out.append("};\n\n")

    # ---------------- lo mismo en espanol y en portugues ----------------
    # El espanol NO pasa por ascii_plano(): los acentos van a los bytes de
    # la fuente (CP437 mas los codigos del til portugues, ver texto_pt.py),
    # que es lo que la pantalla sabe dibujar. El ingles si sigue pelado,
    # porque no tiene ninguno que perder.
    for nom, filas, quien in (("ES", textos_es, "el PBS del Anil"),
                              ("PT", textos_pt, "tools/pt_dex.py")):
        malas = []
        for i, f in enumerate(filas):
            if not f:
                continue
            try:
                # el espanol viene del PBS y no se puede reescribir
                texto_pt.codifica(f, "dex %d" % (i + 1), importado=(nom == "ES"))
            except texto_pt.NoCabe as e:
                malas.append(str(e))
        if malas:
            print(f"  {len(malas)} fichas {nom} con algun caracter que la "
                  f"fuente no tiene:")
            for m in malas[:12]:
                print(f"    {m}")
            raise SystemExit(1)
        hechas = sum(1 for f in filas if f)
        out.append(f"// Lo mismo en {nom}, de {quien}. Indexado igual.\n")
        if nom == "PT":
            out.append("// nullptr = sin traducir: la pantalla cae al ingles.\n")
        out.append(f"static const char *const DEX_TEXT_{nom}[] = {{\n")
        out.append('  "",\n')
        for f in filas:
            out.append("  nullptr,\n" if not f
                       else '  "%s",\n'
                       % texto_pt.codifica(f, importado=(nom == "ES")))
        out.append("};\n")
        print(f"  {nom}: {hechas}/{len(filas)} fichas "
              f"({sum(len(f) for f in filas) / 1024:.1f} KB)")
        if nom == "ES" and texto_pt.pelados:
            print(f"    {len(texto_pt.pelados)} mayusculas sin glifo, "
                  f"peladas: {'; '.join(texto_pt.pelados[:6])}")

    # un numero de pokedex que no existe es una errata que nadie ve
    sobran = sorted(d for d in PT_FICHAS if not 1 <= d <= N)
    if sobran:
        print(f"  OJO: {len(sobran)} numeros en pt_dex.py fuera de 1..{N}: "
              f"{sobran[:8]}")

    with open(DST, "w", encoding="utf-8") as f:
        f.write("".join(out))

    largos = sorted(len(t) for t in textos)
    print(f"guardado {DST}")
    print(f"  {N} entradas, {total} bytes ({total/1024:.1f} KB)")
    print(f"  caracteres: min {largos[0]}  mediana {largos[len(largos)//2]}  max {largos[-1]}")
    if vacios:
        print(f"  SIN TEXTO: {vacios}")
    for d in (1, 25, 151, N):
        print(f"  [{d}] {dexh[d]}: {textos[d-1][:64]}...")


if __name__ == "__main__":
    main()
