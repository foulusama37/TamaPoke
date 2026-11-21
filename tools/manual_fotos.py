"""Converte as fotos do simulador (prev_man_*.ppm, tiradas pelo teste
fotos_manual de test_ui.cpp) em PNG redondos para docs/manual/img/.

A tela e um AMOLED redondo de 466x466: o canto fora do circulo nao existe
na placa, entao vira transparente, com uma borda fina como a da caixa.

Uso: python tools/manual_fotos.py [--src _claude_scratch] [--dst docs/manual/img]
"""
import argparse
import glob
import os

from PIL import Image, ImageDraw

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def redonda(src, dst, lado=360):
    im = Image.open(src).convert("RGBA")
    w, h = im.size
    esc = 4  # mascara supersampleada: borda lisa
    m = Image.new("L", (w * esc, h * esc), 0)
    ImageDraw.Draw(m).ellipse((0, 0, w * esc - 1, h * esc - 1), fill=255)
    im.putalpha(m.resize((w, h), Image.LANCZOS))
    # aro escuro de 6 px, como a moldura da Poke Ball
    aro = Image.new("RGBA", (w * esc, h * esc), (0, 0, 0, 0))
    ImageDraw.Draw(aro).ellipse((0, 0, w * esc - 1, h * esc - 1),
                                outline=(40, 40, 48, 255), width=6 * esc)
    im = Image.alpha_composite(im, aro.resize((w, h), Image.LANCZOS))
    im = im.resize((lado, lado), Image.LANCZOS)
    im.save(dst, optimize=True)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--src", default=os.path.join(RAIZ, "_claude_scratch"))
    ap.add_argument("--dst", default=os.path.join(RAIZ, "docs", "manual", "img"))
    a = ap.parse_args()
    os.makedirs(a.dst, exist_ok=True)
    n = 0
    for p in sorted(glob.glob(os.path.join(a.src, "prev_man_*.ppm"))):
        nome = os.path.basename(p)[len("prev_man_"):-4] + ".png"
        redonda(p, os.path.join(a.dst, nome))
        n += 1
    print(n, "fotos em", a.dst)


if __name__ == "__main__":
    main()
