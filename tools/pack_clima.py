"""Empacota as animacoes de CLIMA do Anil para /anim/w_<n>.bin (TPCL).

    python tools/pack_clima.py                    # as quatro, em tools/sdcard/anim
    python tools/pack_clima.py --out <pasta>      # noutra pasta
    python tools/pack_clima.py --previa <pasta>   # e uma folha de quadros de cada

O dono: "a animacao do sandstorm esta errada, deveria ser na tela toda nao?".
Estava: so tocava a animacao do GOLPE (um redemoinho em volta de quem usa).
No Anil o clima tem a SUA animacao (Common:Sandstorm, Common:Rain...) e o
plugin "Continuous Weather Animations" a deixa tocando EM LACO, por cima do
combate inteiro, enquanto o clima durar
(Plugins/Continuous Weather Animations/001_ContinuousWeatherAnimations.rb:
pbStartWeather -> pbStartContinuousWeather, e pbUpdateWeather recomeca a
animacao quando ela acaba).

O que essas animacoes tem e as dos golpes nao: uma FIGURA de frente em
tela cheia que rola (a areia e PRAS- Sandstorm.png andando 499 px em 20
quadros; o sol e PRAS- Sunny Day.png). O pack_anim.py troca a figura pela
cor media dela, que para um golpe basta e para o clima e exatamente o que
nao pode. Por isso o clima tem formato proprio.

As contas dos planos sao as de playTiming (Scripts #232
BattleAnimationPlayer), com o mesmo `oldfo` compartilhado entre os tempos.

Formato TPCL v1 (little endian):
  "TPCL" u8 versao(1) u8 clima (WE_*: 1 sol, 2 chuva, 3 areia, 4 granizo)
  u16 nQuadros  u8 nImagens  u8 temFigura
  imagem x nImagens: igual a do TPAN (ver pack_anim.py)
     u8 w  u8 h  i8 ox  i8 oy  u8 nPal  u8 0  u16 pal565[nPal]  u8 alfa[nPal]
     u8 idx[w*h]
  figura (se temFigura): u16 w  u16 h  u8 nPal  u8 0
     u16 pal565[nPal]  u8 alfa[nPal]  u8 idx[w*h]
     E um LADRILHO: repete nas duas direcoes. Vem a MEIA resolucao da tela
     (um pixel da figura = 2x2 da tela).
  quadro x nQuadros:
     u8 nCels  u8 fundo[4](r,g,b,peso)  u8 frente[4]
     u8 figOp  u16 figX  u16 figY
        (o pixel (X,Y) da tela le a figura em ((X>>1)+figX) % w,
         ((Y>>1)+figY) % h, com peso alfa*figOp/255)
     cel x nCels: os mesmos 26 bytes do TPAN, sem bichos (img >= 0)
"""
import os
import struct
import sys

from PIL import Image

AQUI = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, AQUI)
import anil_anim as A  # noqa: E402
import pack_anim as P  # noqa: E402

OUT = os.path.join(AQUI, "sdcard", "anim")

# WE_* de battle.h -> a animacao de Scripts #122 BattleWeather
CLIMAS = {1: "Sun", 2: "Rain", 3: "Sandstorm", 4: "Hail"}

S = P.S_TELA            # a tela do Essentials (512x384) sobre o circulo de 466


def _raw_timings(anim):
    """Os tempos com os valores CRUS (None e 0 nao sao a mesma coisa para o
    playTiming: `if i.bgX` passa com 0 e nao com nil)."""
    res = []
    for t in (anim.iv.get("timing") or []):
        iv = t.iv
        res.append(dict(
            frame=iv.get("frame") or 0, type=iv.get("timingType") or 0,
            name=A.txt(iv.get("name")), bgX=iv.get("bgX"), bgY=iv.get("bgY"),
            opacity=iv.get("opacity"),
            duration=iv.get("duration") if iv.get("duration") is not None else 5,
            cor=(iv.get("colorRed"), iv.get("colorGreen"), iv.get("colorBlue"),
                 iv.get("colorAlpha"))))
    return res


class Figura:
    """O plano de frente com FIGURA (foGraphic), quadro a quadro."""

    def __init__(self):
        self.nome = None
        self.ox = self.oy = 0.0
        self.op = 0.0
        self.old = [None, None, None]

    def quadro(self, tms, q):
        for t in tms:                      # as interpolacoes em curso
            if t["type"] != 4 or not t["duration"] or t["duration"] <= 0:
                continue
            if t["frame"] + t["duration"] < q or t["frame"] >= q:
                continue
            if self.nome is None:
                continue                   # plano de COR: e do pack_anim
            fr = (q - t["frame"]) / float(t["duration"])
            o0, o1, o2 = (self.old[0] or 0), (self.old[1] or 0), self.old[2]
            if t["bgX"] is not None:
                self.ox = o0 - ((t["bgX"] - o0) * fr)
            if t["bgY"] is not None:
                self.oy = o1 - ((t["bgY"] - o1) * fr)
            if t["opacity"] is not None and o2 is not None:
                self.op = o2 + ((t["opacity"] - o2) * fr)
        for t in tms:                      # os que disparam neste quadro
            if t["frame"] != q:
                continue
            if t["type"] == 3:
                if t["name"]:
                    self.nome = t["name"]
                    self.ox = -(t["bgX"] or 0)
                    self.oy = -(t["bgY"] or 0)
                    self.op = t["opacity"] or 0
                else:
                    self.nome = None
                    self.op = 0
            elif t["type"] == 4 and self.nome is not None:
                self.old = [self.ox, self.oy, self.op]


def empacota_figura(nome):
    """A figura, do tamanho que ocupa na tela e a METADE da resolucao, com
    paleta RGBA. Devolve (bytes, w, h)."""
    p = os.path.join(A.SHEETS, nome)
    if not os.path.exists(p) and os.path.exists(p + ".png"):
        p += ".png"
    im = Image.open(p).convert("RGBA")
    w = max(1, round(im.width * S / 2))
    h = max(1, round(im.height * S / 2))
    im = im.resize((w, h), Image.LANCZOS)
    q = im.quantize(colors=255, method=Image.Quantize.FASTOCTREE)
    pal = q.getpalette("RGBA")
    idx = list(q.getdata())
    n = max(idx) + 1
    p565, palfa = [], []
    for i in range(n):
        r, g, b, a = pal[i * 4:i * 4 + 4]
        p565.append(P.rgb565(r, g, b))
        palfa.append(0 if a <= 8 else a)
    return (struct.pack("<HHBB", w, h, n, 0) + struct.pack("<%dH" % n, *p565)
            + bytes(palfa) + bytes(idx)), w, h


def empacota(todas, clima):
    anim = todas.get("Common:" + CLIMAS[clima])
    if anim is None:
        return None
    sheet = P.folha(A.txt(anim.iv.get("graphic")), anim.iv.get("hue") or 0)
    tms_cor = A.timings(anim)
    tms = _raw_timings(anim)

    imagens, mapa = [], {}

    def imagem(pat):
        if pat not in mapa:
            b = P.empacota_celula(sheet, pat) if sheet is not None else None
            mapa[pat] = None if b is None else len(imagens)
            if b is not None:
                imagens.append(b)
        return mapa[pat]

    bgG, bgC, foG, foC = P.Plano(), P.Plano(), P.Plano(), P.Plano()
    oldbg, oldfo = [None] * 4, [None] * 4
    fig = Figura()
    figuras = set()
    brutos = []
    for q, fr in enumerate(anim):
        cels = []
        for i, cel in enumerate(fr):
            if cel is None or i >= 60:
                continue
            pat = A.g(cel, A.PATTERN)
            if pat is None or pat < 0 or A.g(cel, A.VISIBLE, 1) != 1:
                continue                    # os bichos nao se mexem no clima
            img = imagem(pat)
            if img is None:
                continue
            # TUDO pelo mapa da tela inteira: o clima cobre o combate, nao
            # a linha entre os dois bichos. As particulas crescem na mesma
            # proporcao (vem pre-escaladas por K).
            x = 233 + (A.g(cel, A.X, 0) - 256) * S
            y = 233 + (A.g(cel, A.Y, 0) - 192) * S
            fz = S / P.K
            pri = A.g(cel, A.PRIORITY, 1)
            camada = 0 if pri == 0 else 2
            flags = (1 if A.g(cel, A.MIRROR, 0) else 0) | (
                (P._u8(A.g(cel, A.BLENDTYPE, 0)) & 3) << 1)
            cor = [P._u8(A.g(cel, k, 0)) for k in (A.COLORRED, A.COLORGREEN,
                                                   A.COLORBLUE, A.COLORALPHA)]
            tom = [P._i16(A.g(cel, k, 0)) for k in (A.TONERED, A.TONEGREEN,
                                                    A.TONEBLUE, A.TONEGRAY)]
            cels.append(struct.pack(
                "<bBhhHHhBB4B4h", img, camada, P._i16(x), P._i16(y),
                max(0, min(65535, int((A.g(cel, A.ZOOMX, 100) or 0) * fz))),
                max(0, min(65535, int((A.g(cel, A.ZOOMY, 100) or 0) * fz))),
                P._i16(A.g(cel, A.ANGLE, 0)), P._u8(A.g(cel, A.OPACITY, 255)),
                flags, *cor, *tom))
        P.timings_do_quadro(tms_cor, q, bgG, bgC, foG, foC, oldbg, oldfo)
        fig.quadro(tms, q)
        fundo = P.plano_em_um(bgC, bgG)
        # o plano de frente de COR segue valendo; o de figura vai aparte
        frente = P.plano_em_um(foC, P.Plano())
        if fig.nome:
            figuras.add(fig.nome)
        brutos.append((cels, fundo, frente, fig.nome, fig.ox, fig.oy, fig.op))

    if not brutos:
        return None
    if len(figuras) > 1:
        raise RuntimeError("%s troca de figura no meio: %s" % (CLIMAS[clima], figuras))
    figb, fw, fh = (b"", 1, 1)
    if figuras:
        figb, fw, fh = empacota_figura(next(iter(figuras)))
    quadros = []
    for cels, fundo, frente, nome, ox, oy, op in brutos:
        # O plano do Essentials mostra, no pixel (sx,sy) da tela DELE, o
        # pixel (sx+ox, sy+oy) da figura. O nosso (X,Y) e o (256+(X-233)/S,
        # 192+(Y-233)/S) dele; na figura a meia resolucao isso da
        # (X>>1) + figX, com figX = (256 + ox) * S/2 - 233/2.
        fx = int(round((256 + ox) * S / 2 - 116.5)) % fw
        fy = int(round((192 + oy) * S / 2 - 116.5)) % fh
        quadros.append(struct.pack("<B4B4BBHH", len(cels), *fundo, *frente,
                                   P._u8(op) if nome else 0, fx, fy)
                       + b"".join(cels))
    cab = b"TPCL" + struct.pack("<BBHBB", 1, clima, len(quadros), len(imagens),
                                1 if figuras else 0)
    return cab + b"".join(imagens) + figb + b"".join(quadros)


# ------------------------------------------------------------ a previa
def le(b):
    assert b[:4] == b"TPCL" and b[4] == 1
    clima = b[5]
    nq, ni, tem = struct.unpack_from("<HBB", b, 6)
    p = 10
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
        im.putdata([(((pal[i] >> 11) & 31) * 255 // 31, ((pal[i] >> 5) & 63) * 255 // 63,
                     (pal[i] & 31) * 255 // 31, alfa[i]) for i in idx])
        imgs.append((im, ox, oy))
    fig = None
    if tem:
        w, h, npal, _z = struct.unpack_from("<HHBB", b, p)
        p += 6
        pal = struct.unpack_from("<%dH" % npal, b, p)
        p += 2 * npal
        alfa = b[p:p + npal]
        p += npal
        idx = b[p:p + w * h]
        p += w * h
        fig = Image.new("RGBA", (w, h))
        fig.putdata([(((pal[i] >> 11) & 31) * 255 // 31, ((pal[i] >> 5) & 63) * 255 // 63,
                      (pal[i] & 31) * 255 // 31, alfa[i]) for i in idx])
    quadros = []
    for _ in range(nq):
        nc = b[p]
        fundo, frente = tuple(b[p + 1:p + 5]), tuple(b[p + 5:p + 9])
        op, fx, fy = struct.unpack_from("<BHH", b, p + 9)
        p += 14
        cels = []
        for _c in range(nc):
            cels.append(struct.unpack_from("<bBhhHHhBB4B4h", b, p))
            p += 26
        quadros.append((fundo, frente, op, fx, fy, cels))
    assert p == len(b), "sobrou ou faltou: %d de %d" % (p, len(b))
    return clima, imgs, fig, quadros


def pinta(base, imgs, fig, quadro):
    """Um quadro por cima de `base` (RGBA 466x466), com as contas da placa."""
    import ve_anim as V
    fundo, frente, op, fx, fy, cels = quadro
    tela = base.copy()
    V.plano(tela, fundo)
    for c in cels:
        V.cel(tela, imgs, c)
    if fig is not None and op:
        w, h = fig.size
        lad = Image.new("RGBA", (233 + w, 233 + h))
        for yy in range(0, 233 + h, h):
            for xx in range(0, 233 + w, w):
                lad.paste(fig, (xx, yy))
        rec = lad.crop((fx, fy, fx + 233, fy + 233)).resize((466, 466), Image.NEAREST)
        a = rec.getchannel("A").point(lambda v: v * op // 255)
        rec.putalpha(a)
        tela.alpha_composite(rec)
    V.plano(tela, frente)
    return tela


def main():
    args = sys.argv[1:]
    out, previa = OUT, None
    if "--out" in args:
        out = args[args.index("--out") + 1]
    if "--previa" in args:
        previa = args[args.index("--previa") + 1]
    todas = A.load_all()
    os.makedirs(out, exist_ok=True)
    for clima, nome in sorted(CLIMAS.items()):
        b = empacota(todas, clima)
        if b is None:
            print("SEM animacao: Common:" + nome)
            continue
        with open(os.path.join(out, "w_%u.bin" % clima), "wb") as fh:
            fh.write(b)
        _c, imgs, fig, quadros = le(b)
        print("w_%u.bin  %-10s %3d quadros, %2d imagens, figura %s, %d KB" % (
            clima, nome, len(quadros), len(imgs),
            ("%dx%d" % fig.size) if fig else "-", len(b) // 1024))
        if previa:
            os.makedirs(previa, exist_ok=True)
            base = Image.new("RGBA", (466, 466), (120, 170, 120, 255))
            cols = 6
            rows = (len(quadros) + cols - 1) // cols
            folha = Image.new("RGB", (cols * 233, rows * 233), (20, 20, 20))
            for k, qd in enumerate(quadros):
                t = pinta(base, imgs, fig, qd).resize((233, 233))
                folha.paste(t.convert("RGB"), ((k % cols) * 233, (k // cols) * 233))
            folha.save(os.path.join(previa, "clima_%s.png" % nome))


if __name__ == "__main__":
    main()
