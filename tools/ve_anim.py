"""Le um /anim/*.bin (TPAN, ver pack_anim.py) e desenha os quadros numa
folha, com as mesmas contas que o firmware faz. Serve para separar um
defeito do gerador de um do tocador.

    python tools/ve_anim.py tools/sdcard/anim/p_FLAMETHROWER.bin [saida.png]
"""
import math
import os
import struct
import sys

from PIL import Image, ImageDraw

JOG, RIV = (145, 242), (325, 128)


def le(caminho):
    b = open(caminho, "rb").read()
    assert b[:4] == b"TPAN", "assinatura"
    ver, lado, n = b[4], chr(b[5]), b[6]
    nome = b[7:7 + n].decode()
    p = 7 + n
    nq, ni, _ = struct.unpack_from("<HBB", b, p)
    p += 4
    imgs = []
    for _ in range(ni):
        w, h, ox, oy, npal, _z = struct.unpack_from("<BBbbBB", b, p)
        p += 6
        pal = struct.unpack_from("<%dH" % npal, b, p)
        p += 2 * npal
        alfa = b[p:p + npal]
        p += npal
        idx = b[p:p + w * h]
        p += w * h
        im = Image.new("RGBA", (w, h))
        px = []
        for i in idx:
            c = pal[i]
            px.append((((c >> 11) & 31) * 255 // 31, ((c >> 5) & 63) * 255 // 63,
                       (c & 31) * 255 // 31, alfa[i]))
        im.putdata(px)
        imgs.append((im, ox, oy))
    quadros = []
    for _ in range(nq):
        nc = b[p]
        fundo, frente = tuple(b[p + 1:p + 5]), tuple(b[p + 5:p + 9])
        p += 9
        cels = []
        for _c in range(nc):
            cels.append(struct.unpack_from("<bBhhHHhBB4B4h", b, p))
            p += 26
        quadros.append((fundo, frente, cels))
    assert p == len(b), "sobrou ou faltou: %d de %d" % (p, len(b))
    return nome, lado, imgs, quadros


def plano(tela, cor):
    r, g, b, w = cor
    if not w:
        return
    tela.alpha_composite(Image.new("RGBA", tela.size, (r, g, b, w)))


def cel(tela, imgs, c):
    img, camada, x, y, zx, zy, ang, op, fl, *resto = c
    im, ox, oy = imgs[img]
    # recoloca o recorte no lugar dentro de um quadrado centrado
    lado = max(abs(ox), abs(oy), im.width + ox, im.height + oy) * 2 + 2
    base = Image.new("RGBA", (lado, lado))
    base.paste(im, (lado // 2 + ox, lado // 2 + oy))
    if fl & 1:
        base = base.transpose(Image.FLIP_LEFT_RIGHT)
    nw, nh = max(1, round(lado * zx / 100)), max(1, round(lado * zy / 100))
    base = base.resize((nw, nh), Image.NEAREST)
    if ang % 360:
        base = base.rotate(ang, expand=True)
    if op < 255:
        a = base.getchannel("A").point(lambda v: v * op // 255)
        base.putalpha(a)
    tela.alpha_composite(base, (int(x - base.width / 2), int(y - base.height / 2)))


def bicho(tela, rotulo, centro, c):
    if c is None:
        return
    _img, _cam, dx, dy, zx, zy, _a, op, _f, *_r = c
    if op == 0:
        return
    w, h = 60 * zx // 100, 60 * zy // 100
    x, y = centro[0] + dx, centro[1] + dy
    d = ImageDraw.Draw(tela)
    d.ellipse((x - w / 2, y - h / 2, x + w / 2, y + h / 2),
              fill=(90, 150, 230, op) if rotulo == "J" else (230, 120, 90, op))
    d.text((x - 4, y - 5), rotulo, (0, 0, 0, 255))


def main():
    nome, lado, imgs, quadros = le(sys.argv[1])
    saida = sys.argv[2] if len(sys.argv) > 2 else os.path.join(
        os.path.expanduser("~"), "Downloads", "digisprites", "anim_tpan",
        "%s_%s.png" % (lado, nome))
    os.makedirs(os.path.dirname(saida), exist_ok=True)
    cols, t = 6, 233
    rows = (len(quadros) + cols - 1) // cols
    folha = Image.new("RGB", (cols * t, rows * t), (20, 20, 20))
    for k, (fundo, frente, cels) in enumerate(quadros):
        tela = Image.new("RGBA", (466, 466), (150, 176, 150, 255))
        plano(tela, fundo)
        jog = next((c for c in cels if c[0] == -1), None)
        riv = next((c for c in cels if c[0] == -2), None)
        for cam in (0, 1, 2):
            for c in cels:
                if c[0] >= 0 and c[1] == cam:
                    cel(tela, imgs, c)
            if cam == 0:
                bicho(tela, "R", RIV, riv)
            if cam == 1:
                bicho(tela, "J", JOG, jog)
        plano(tela, frente)
        folha.paste(tela.convert("RGB").resize((t, t)), ((k % cols) * t, (k // cols) * t))
        ImageDraw.Draw(folha).text(((k % cols) * t + 3, (k // cols) * t + 2), str(k), (255, 255, 0))
    folha.save(saida)
    print(saida, len(quadros), "quadros", len(imgs), "imagens")


if __name__ == "__main__":
    main()
