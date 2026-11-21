#!/usr/bin/env python3
"""Convierte los PPM del simulador en PNG sueltos y un GIF de la sesion.

  python tools/ppm2gif.py build/frames
"""
import glob
import os
import sys

from PIL import Image


def main():
    d = sys.argv[1] if len(sys.argv) > 1 else 'build/frames'
    ms = int(sys.argv[2]) if len(sys.argv) > 2 else 90

    # las capturas con nombre (shot) salen como PNG sueltos
    nombradas = [p for p in sorted(glob.glob(os.path.join(d, '*.ppm')))
                 if not os.path.basename(p).startswith('f')]
    for p in nombradas:
        png = p[:-4] + '.png'
        Image.open(p).save(png)
        print('  ', os.path.basename(png))

    seq = sorted(glob.glob(os.path.join(d, 'f*.ppm')))
    if seq:
        fr = [Image.open(p).convert('P', palette=Image.ADAPTIVE) for p in seq]
        out = os.path.join(d, 'sesion.gif')
        fr[0].save(out, save_all=True, append_images=fr[1:], duration=ms, loop=0)
        print(f'   {os.path.basename(out)}: {len(fr)} frames')

    # hoja de contacto con las capturas nombradas
    if nombradas:
        ims = [Image.open(p) for p in nombradas]
        cols = 5
        rows = (len(ims) + cols - 1) // cols
        w, h = ims[0].size
        esc = 3
        sheet = Image.new('RGB', (w * cols // esc, (h // esc + 16) * rows), (24, 26, 32))
        from PIL import ImageDraw
        dr = ImageDraw.Draw(sheet)
        for i, (p, im) in enumerate(zip(nombradas, ims)):
            th = im.resize((w // esc, h // esc), Image.LANCZOS)
            x, y = (i % cols) * (w // esc), (i // cols) * (h // esc + 16)
            sheet.paste(th, (x, y + 14))
            dr.text((x + 3, y + 2), os.path.basename(p)[:-4], fill=(235, 235, 240))
        sheet.save(os.path.join(d, 'contacto.png'))
        print('   contacto.png', sheet.size)


if __name__ == '__main__':
    main()
