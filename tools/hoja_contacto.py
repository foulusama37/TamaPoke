#!/usr/bin/env python3
"""Hojas de contacto: TODO lo que hay, en PNG, para mirarlo de un vistazo.

La auditoria dice si un sprite se puede leer. No dice si es el sprite que
le toca a ese bicho — eso solo lo ve un ojo, y mirar 1025 de uno en uno no
lo hace nadie. Asi que se sacan en hojas de 8x6 con el nombre debajo, y en
diez minutos se repasan todas.

Lo que se dibuja es lo que HAY EN LA TARJETA, descomprimido con el mismo
formato que lee el aparato — no el PNG del Anil. Si el empaquetador se
equivoco de fichero, aqui se ve.

  python3 tools/hoja_contacto.py              # los 1025, por region
  python3 tools/hoja_contacto.py megas        # las 128
  python3 tools/hoja_contacto.py formas       # las 63
  python3 tools/hoja_contacto.py shiny        # para comparar colores
"""
import io
import os
import struct
import sys

from PIL import Image, ImageDraw

AQUI = os.path.dirname(os.path.abspath(__file__))
RAIZ = os.path.dirname(AQUI)
sys.path.insert(0, AQUI)
from dex_data import DEX              # noqa: E402
from dex_range import N_DEX, REGIONES  # noqa: E402

MONS = os.path.join(AQUI, "sdcard", "mons")
OUT = os.path.join(AQUI, "hojas")
NOMBRE = {d[0]: d[2] for d in DEX}

COLS, FILAS = 8, 6
CELDA_W, CELDA_H = 112, 118
FONDO = (24, 28, 36)
TEXTO = (210, 214, 220)
AVISO = (232, 96, 96)


def primer_frame(path):
    """El primer fotograma del TPK2, como lo leeria el aparato.

    Se decodifica aqui y no se usa el PNG del Anil A PROPOSITO: lo que
    hay que revisar es lo que se grabo en la tarjeta. Si el empaquetador
    cogio el fichero de otro bicho, mirando el PNG no se veria.
    """
    try:
        d = io.open(path, "rb").read()
    except OSError:
        return None
    if len(d) < 7 or d[:4] != b"TPK2":
        return None
    nacts, npal = struct.unpack_from("<BH", d, 4)
    pal = [struct.unpack_from("<H", d, 7 + i * 2)[0] for i in range(npal)]
    off = 7 + npal * 2
    for _a in range(nacts):
        if off + 4 > len(d):
            return None
        _aid, w, h, nf = struct.unpack_from("<BBBB", d, off)
        off += 4 + nf * 2
        if w == 0 or h == 0 or off + w * h > len(d):
            return None
        im = Image.new("RGBA", (w, h), (0, 0, 0, 0))
        px = im.load()
        for y in range(h):
            fila = d[off + y * w: off + (y + 1) * w]
            for x, idx in enumerate(fila):
                if idx == 0xFF or idx >= npal:
                    continue
                c = pal[idx]
                px[x, y] = (((c >> 11) & 31) * 255 // 31,
                            ((c >> 5) & 63) * 255 // 63,
                            (c & 31) * 255 // 31, 255)
        return im
    return None


def hoja(items, titulo, dst):
    """items: [(ruta, etiqueta)]"""
    paginas = []
    for p0 in range(0, len(items), COLS * FILAS):
        trozo = items[p0:p0 + COLS * FILAS]
        img = Image.new("RGB", (COLS * CELDA_W, FILAS * CELDA_H + 22), FONDO)
        dib = ImageDraw.Draw(img)
        dib.text((6, 6), f"{titulo}  {p0 + 1}-{p0 + len(trozo)}", fill=TEXTO)
        for i, (ruta, etq) in enumerate(trozo):
            cx = (i % COLS) * CELDA_W
            cy = (i // COLS) * CELDA_H + 22
            fr = primer_frame(ruta)
            if fr is None:
                dib.rectangle([cx + 4, cy + 4, cx + CELDA_W - 6,
                               cy + CELDA_H - 20], outline=AVISO)
                dib.text((cx + 8, cy + 40), "SIN SPRITE", fill=AVISO)
            else:
                # cabe en la celda sin deformarse
                k = min((CELDA_W - 10) / fr.width, (CELDA_H - 26) / fr.height)
                if k < 1:
                    fr = fr.resize((max(1, int(fr.width * k)),
                                    max(1, int(fr.height * k))),
                                   Image.NEAREST)
                img.paste(fr, (cx + (CELDA_W - fr.width) // 2,
                               cy + (CELDA_H - 20 - fr.height) // 2), fr)
            dib.text((cx + 4, cy + CELDA_H - 16), etq[:17], fill=TEXTO)
        paginas.append(img)

    os.makedirs(OUT, exist_ok=True)
    for i, p in enumerate(paginas):
        f = os.path.join(OUT, f"{dst}_{i + 1:02d}.png")
        p.save(f)
    print(f"  {titulo}: {len(items)} en {len(paginas)} hojas -> "
          f"{dst}_01..{len(paginas):02d}.png")


def filas_tabla(fichero, cte):
    """[(fila, base)] de megas.h / forms.h, por orden de fila."""
    import re
    f = os.path.join(RAIZ, fichero)
    if not os.path.exists(f):
        return []
    txt = io.open(f, encoding="utf-8", errors="replace").read()
    cuerpo = txt[txt.index(cte.replace("_COUNT", "_TBL")):]
    out = []
    for i, m in enumerate(re.finditer(
            r'^\s*\{\s*(\d+),.*?"([^"]*)" \}', cuerpo, re.M)):
        out.append((i, int(m.group(1)), m.group(2)))
    return out


def main():
    que = sys.argv[1] if len(sys.argv) > 1 else "dex"

    if que in ("dex", "shiny"):
        pre = "ps" if que == "shiny" else "p"
        ini = 1
        for nom, fin in REGIONES:
            items = [(os.path.join(MONS, f"{pre}{d:03d}.bin"),
                      f"{d} {NOMBRE.get(d, '?')}")
                     for d in range(ini, min(fin, N_DEX) + 1)]
            hoja(items, f"{nom}{' SHINY' if que == 'shiny' else ''}",
                 f"{que}_{nom.lower()}")
            ini = fin + 1
    elif que == "megas":
        fs = filas_tabla("megas.h", "MEGA_COUNT")
        hoja([(os.path.join(MONS, f"m{i:03d}.bin"), n) for i, _b, n in fs],
             "MEGAS", "megas")
    elif que == "formas":
        fs = filas_tabla("forms.h", "FORM_COUNT")
        hoja([(os.path.join(MONS, f"v{i:03d}.bin"), n) for i, _b, n in fs],
             "FORMAS", "formas")
    else:
        raise SystemExit(__doc__)
    print(f"\nhojas en {OUT}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
