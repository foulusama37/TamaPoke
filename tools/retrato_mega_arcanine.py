# Retrato da Mega Arcanine: as DIFERENCAS da Mega aplicadas em cima do
# retrato do Arcanine do SpriteCollab (pixel art de verdade, mesmo estilo
# dos outros), como o dono pediu:
#   pelo branco/creme, mascara dourada, olho azul, boca rosa, listras
#   laranja; chifre INCLINADO para a frente; juba arredondada (os espinhos
#   viram tufos) e as linhas de cachinhos da Mega por cima do pelo.
#
# O vermelho do Arcanine faz TRES papeis (mascara, boca, listras do
# pescoco); quem decide e a MANCHA conectada, pela altura media dela.
import os
import sys

from PIL import Image

ARC = "G:/SpriteCollab/repo/portrait/0059/"
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "retratos_extra", "m059_2")

PELO = {  # tons do Arcanine -> do branco da Mega (sombra fria, lavanda)
    (255, 231, 159): (255, 253, 246),
    (247, 215, 143): (246, 243, 236),
    (239, 191, 119): (228, 224, 222),
    (215, 167, 95): (204, 198, 204),
    (191, 135, 63): (168, 160, 174),
    (127, 95, 55): (118, 106, 126),
}
PELO_NOVO = set(PELO.values())
CONTORNO = {(79, 71, 55): (82, 66, 74), (55, 55, 39): (52, 42, 50)}
ESC = (52, 42, 50)
LINHA = (150, 140, 166)      # o traco dos cachinhos: sombra fria do pelo
CLARO, ESCURO = (199, 63, 15), (135, 23, 31)
MASCARA = {CLARO: (240, 182, 52), ESCURO: (128, 72, 16)}
BOCA = {CLARO: (226, 96, 112), ESCURO: (150, 40, 62)}
LISTRA = {CLARO: (240, 150, 60), ESCURO: (186, 96, 34)}

# Por cara: onde nasce o chifre, os pixels da iris e onde vao os
# cachinhos. Medidos olhando cada retrato ampliado.
CARAS = {
    "Normal": dict(
        chifre=(22, 10),
        iris=[((20, 15), (96, 156, 238)), ((21, 16), (96, 156, 238)),
              ((20, 16), (40, 82, 192)), ((20, 17), (40, 82, 192)),
              ((21, 17), (40, 82, 192))],
        cachos=[(14, 6), (27, 7), (3, 22), (2, 29), (6, 34), (34, 26), (31, 32)]),
    "Joyous": dict(
        chifre=(22, 11),     # meio da testa, acima da mascara
        iris=[],
        cachos=[(10, 6), (26, 6), (3, 22), (2, 29), (6, 34), (34, 30)]),
}


def manchas(im):
    """Manchas conectadas (4-vizinhos) de vermelho: {pixel: id}, {id: [pixels]}."""
    VR = (CLARO, ESCURO)
    de, todas = {}, {}
    for y in range(40):
        for x in range(40):
            if im.getpixel((x, y))[:3] in VR and (x, y) not in de:
                k = len(todas)
                todas[k] = []
                pilha = [(x, y)]
                de[(x, y)] = k
                while pilha:
                    a, b = pilha.pop()
                    todas[k].append((a, b))
                    for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                        q = (a + dx, b + dy)
                        if (0 <= q[0] < 40 and 0 <= q[1] < 40 and q not in de
                                and im.getpixel(q)[:3] in VR):
                            de[q] = k
                            pilha.append(q)
    return de, todas


def repinta(nome):
    im = Image.open(ARC + nome + ".png").convert("RGBA")
    out = im.copy()
    de, todas = manchas(im)
    papel = {}
    for k, px in todas.items():
        ym = sum(p[1] for p in px) / len(px)
        papel[k] = LISTRA if ym >= 28 else (BOCA if ym >= 20 else MASCARA)
    for y in range(40):
        for x in range(40):
            c = im.getpixel((x, y))[:3]
            if c in PELO:
                n = PELO[c]
            elif c in CONTORNO:
                n = CONTORNO[c]
            elif (x, y) in de:
                n = papel[de[(x, y)]][c]
            else:
                continue                   # fundo e brilho do olho ficam
            out.putpixel((x, y), n + (255,))
    return out, im


def eh_fundo(orig, x, y):
    """Fundo = o que no ORIGINAL nao era bicho (o degrade e o D da borda)."""
    if not (0 <= x < 40 and 0 <= y < 40):
        return True
    c = orig.getpixel((x, y))[:3]
    return c in ((127, 199, 215), (151, 207, 199), (231, 247, 191), (103, 135, 127))


def arredonda(im, orig, ate_y=13, voltas=3):
    """Os ESPINHOS da juba viram tufo: na parte de cima, pixel do bicho com
    3+ vizinhos de fundo e uma ponta —— vira fundo. Depois o contorno e
    refeito: pelo encostado no fundo ganha a linha escura."""
    fundo = {(x, y) for y in range(40) for x in range(40) if eh_fundo(orig, x, y)}
    for _ in range(voltas):
        tirar = []
        for y in range(ate_y):
            for x in range(40):
                if (x, y) in fundo:
                    continue
                viz = sum(((x + dx, y + dy) in fundo or not (0 <= x + dx < 40 and 0 <= y + dy < 40))
                          for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)))
                if viz >= 3:
                    tirar.append((x, y))
        for p in tirar:
            fundo.add(p)
            im.putpixel(p, orig_fundo(orig, p))
    # contorno novo onde o pelo ficou exposto
    for y in range(ate_y + 1):
        for x in range(40):
            if (x, y) in fundo:
                continue
            if im.getpixel((x, y))[:3] in PELO_NOVO and any(
                    (x + dx, y + dy) in fundo for dx, dy in ((1, 0), (-1, 0), (0, -1))):
                im.putpixel((x, y), ESC + (255,))


def tufos(im, orig, ate_y=14, r=3, passo=5):
    """O contorno de CIMA vira uma fileira de tufos redondos, como a juba
    de nuvem da Mega. Mede-se a altura do bicho em cada coluna, suaviza-se
    (mediana de 9) e a forma nova e a uniao de circulos de raio r encostados
    nessa curva. O que ficar fora vira fundo; o que ficar dentro vira pelo;
    e a borda ganha o contorno escuro."""
    topo = []
    for x in range(40):
        y0 = next((y for y in range(40) if not eh_fundo(orig, x, y)), 40)
        topo.append(y0)
    suave = []
    for x in range(40):
        jan = sorted(topo[max(0, x - 4):x + 5])
        suave.append(jan[len(jan) // 2] + 1)
    centros = [(x, suave[x] + r - 1) for x in range(1, 40, passo) if suave[x] < ate_y + r]
    def dentro(x, y):
        if y >= ate_y:
            return not eh_fundo(orig, x, y)
        if y > suave[x] + r:
            return not eh_fundo(orig, x, y) or True
        return any((x - cx) ** 2 + (y - cy) ** 2 <= r * r + 1 for cx, cy in centros)
    forma = {(x, y) for y in range(ate_y) for x in range(40) if dentro(x, y) and topo[x] < 40}
    for y in range(ate_y):
        for x in range(40):
            era_bicho = not eh_fundo(orig, x, y)
            if (x, y) in forma and not era_bicho:
                im.putpixel((x, y), (246, 243, 236, 255))
            elif (x, y) not in forma and (era_bicho or
                                          orig.getpixel((x, y))[:3] == BORDA_FUNDO):
                # (a borda escura do fundo original tambem sai: sem o
                # espinho que ela contornava, sobrava solta no ar)
                im.putpixel((x, y), orig_fundo(orig, (x, y)))
    for (x, y) in forma:
        if any((x + dx, y + dy) not in forma and y + dy < ate_y and 0 <= x + dx < 40 and y + dy >= 0
               for dx, dy in ((1, 0), (-1, 0), (0, -1))):
            im.putpixel((x, y), ESC + (255,))


BORDA_FUNDO = (103, 135, 127)


def borda_olho(im, orig, iris):
    """Contorno ESCURO em volta do olho: tudo que encosta (8 vizinhos) no
    branco do olho ou na iris e e mascara/pelo vira linha escura."""
    olho = {p for p, _c in iris}
    olho |= {(x, y) for y in range(40) for x in range(40)
             if orig.getpixel((x, y))[:3] == (255, 255, 255)
             and y < 22}                    # o branco do olho, nao a presa
    claros = set(MASCARA.values()) | PELO_NOVO
    for (x, y) in olho:
        for dx in (-1, 0, 1):
            for dy in (-1, 0, 1):
                q = (x + dx, y + dy)
                if q in olho or not (0 <= q[0] < 40 and 0 <= q[1] < 40):
                    continue
                if im.getpixel(q)[:3] in claros:
                    im.putpixel(q, ESC + (255,))


def barbinha(im, orig, lado, y0=21, y1=40, larg=13, r=3, passo=5):
    """A barbinha da LATERAL (lado -1 esquerda, +1 direita) em tufos
    redondos —— o mesmo que `tufos` faz no topo, deitado: mede-se onde o
    bicho comeca em cada linha (vindo daquele lado), suaviza, e a borda
    vira uma fileira de circulos. Por dentro os fios espetados do Arcanine
    viram SOMBRA (nao pelo chapado) e os cachos entram por cima."""
    def borda(y):
        xs = range(40) if lado < 0 else range(39, -1, -1)
        return next((x for x in xs if not eh_fundo(orig, x, y)), None)
    bruto = [borda(y) for y in range(40)]
    vale = [b if b is not None else (40 if lado < 0 else -1) for b in bruto]
    suave = []
    for y in range(40):
        jan = sorted(vale[max(0, y - 4):y + 5])
        suave.append(jan[len(jan) // 2] - lado)        # um pixel para dentro
    if lado < 0:
        regiao = lambda x: x < larg
        centros = [(suave[y] + r - 1, y) for y in range(y0, y1, passo) if suave[y] < larg]
        alem = lambda x, y: x > suave[y] + r
    else:
        regiao = lambda x: x >= 40 - larg
        centros = [(suave[y] - r + 1, y) for y in range(y0, y1, passo) if suave[y] >= 40 - larg]
        alem = lambda x, y: x < suave[y] - r
    forma = set()
    for y in range(y0, y1):
        for x in range(40):
            if regiao(x) and (alem(x, y) or any((x - cx) ** 2 + (y - cy) ** 2 <= r * r + 1
                                                 for cx, cy in centros)):
                forma.add((x, y))
    for y in range(y0, y1):
        for x in range(40):
            if not regiao(x):
                continue
            era = not eh_fundo(orig, x, y)
            if (x, y) in forma and not era:
                im.putpixel((x, y), (228, 224, 222, 255))
            elif (x, y) not in forma and (era or orig.getpixel((x, y))[:3] == BORDA_FUNDO):
                im.putpixel((x, y), orig_fundo(orig, (x, y)))
    fios = set(CONTORNO.values()) | {(118, 106, 126)}
    for (x, y) in forma:
        # So os fios PERTO da borda de fora (ate 3 px): mais para dentro
        # ja e o rosto —— o contorno da boca e do focinho clareava junto.
        perto = abs(x - suave[y]) <= 3
        if perto and im.getpixel((x, y))[:3] in fios:
            im.putpixel((x, y), (204, 198, 204, 255))      # sombra, nao chapado
    # contorno so onde o tufo encosta no FUNDO
    for (x, y) in forma:
        viz = [(x + lado, y), (x, y - 1), (x, y + 1)]
        if any(0 <= qx < 40 and y0 <= qy < y1 and regiao(qx) and (qx, qy) not in forma
               for qx, qy in viz):
            im.putpixel((x, y), ESC + (255,))


def orig_fundo(orig, p):
    # a cor do degrade naquela altura: pega o fundo mais proximo na linha
    # (sem a borda escura, que nao e degrade)
    x, y = p
    for d in range(1, 40):
        for q in ((x - d, y), (x + d, y), (x, y - d)):
            if (0 <= q[0] < 40 and 0 <= q[1] < 40 and eh_fundo(orig, *q)
                    and orig.getpixel(q)[:3] != BORDA_FUNDO):
                return orig.getpixel(q)
    return (127, 199, 215, 255)


# O cachinho da Mega, 4x4: uma espiral aberta.
CACHO = [".###.",
         "#...#",
         "#.#.#",
         "#..#.",
         ".##.."]


def cachos(im, onde):
    for cx, cy in onde:
        for j, lin in enumerate(CACHO):
            for i, ch in enumerate(lin):
                x, y = cx + i, cy + j
                if ch == "#" and 0 <= x < 40 and 0 <= y < 40 and \
                        im.getpixel((x, y))[:3] in PELO_NOVO:
                    im.putpixel((x, y), LINHA + (255,))


def chifre(im, bx, by, alto=8):
    """Cone INCLINADO para a frente (para o lado do focinho, a direita):
    sobe e anda um pixel para a frente a cada dois. 3 px de base na metade
    de baixo, 1 na de cima; dourado passando a rosa na ponta."""
    cores = [(240, 182, 52), (246, 160, 92), (238, 128, 142)]
    pts = []
    for k in range(alto):
        y = by - k
        x = bx + k               # diagonal REGULAR de 45 graus: o passo
                                 # irregular de antes deixava o chifre torto
        meia = 1 if k < alto // 2 else 0
        cor = cores[min(2, k * 3 // alto)] + (255,)
        for xx in range(x - meia, x + meia + 1):
            im.putpixel((xx, y), cor)
        pts.append((x, y, meia))
    # contorno escuro em volta do que foi pintado
    pintado = {(x + d, y) for x, y, m in pts for d in range(-m, m + 1)}
    for (x, y) in list(pintado):
        for dx, dy in ((1, 0), (-1, 0), (0, -1), (1, -1), (-1, -1)):
            q = (x + dx, y + dy)
            if q not in pintado and 0 <= q[0] < 40 and 0 <= q[1] < 40 and q[1] <= by:
                im.putpixel(q, ESC + (255,))
    im.putpixel((bx - 1, by - 1), (255, 236, 150, 255))   # brilho


# A SHINY: a troca que o PROPRIO Anil faz entre Front/ARCANINE_2.png e
# Front shiny/ARCANINE_2.png (medida pixel a pixel): o branco fica, o
# dourado vira vermelho-alaranjado, o laranja vira vermelho, o rosa vira
# magenta e o azul do olho puxa para o ciano.
SHINY = {
    (240, 182, 52): (240, 56, 30),     # mascara clara
    (128, 72, 16): (128, 16, 10),      # mascara escura
    (240, 150, 60): (236, 52, 34),     # listra clara
    (186, 96, 34): (176, 24, 18),      # listra escura
    (246, 160, 92): (246, 110, 60),    # chifre, meio
    (238, 128, 142): (238, 112, 214),  # chifre, ponta
    (255, 236, 150): (255, 184, 127),  # brilho do chifre
    (226, 96, 112): (226, 96, 186),    # boca clara
    (150, 40, 62): (150, 40, 112),     # boca escura
    (96, 156, 238): (96, 214, 238),    # iris clara
    (40, 82, 192): (36, 150, 214),     # iris escura
}


def shiny(im):
    out = im.copy()
    for y in range(im.height):
        for x in range(im.width):
            c = im.getpixel((x, y))
            if c[:3] in SHINY:
                out.putpixel((x, y), SHINY[c[:3]] + (c[3],))
    return out


if __name__ == "__main__":
    nome = sys.argv[1] if len(sys.argv) > 1 else "Normal"
    im, orig = repinta(nome)
    cfg = CARAS.get(nome)
    if cfg:
        tufos(im, orig)
        barbinha(im, orig, -1)
        barbinha(im, orig, +1, y0=27)     # abaixo do queixo
        cachos(im, cfg["cachos"])
        for (x, y), c in cfg["iris"]:
            im.putpixel((x, y), c + (255,))
        if cfg["iris"]:
            borda_olho(im, orig, cfg["iris"])
        chifre(im, *cfg["chifre"], alto=int(os.environ.get("ALTO", "8")))
    os.makedirs(os.path.join(OUT, "shiny"), exist_ok=True)
    im.save(os.path.join(OUT, "%s.png" % nome))
    shiny(im).save(os.path.join(OUT, "shiny", "%s.png" % nome))
    print("ok", nome)
