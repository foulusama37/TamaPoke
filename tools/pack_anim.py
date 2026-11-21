"""Empacota as animacoes de golpe DO ANIL, quadro a quadro, para /anim/.

    python tools/pack_anim.py                 # todos os golpes de moves.h
    python tools/pack_anim.py FLAMETHROWER    # so esses

O dono: "sinto as animacoes dos ataques bem bobinhas e erradas... nao
conseguimos reproduzir algo como e no anil para cada golpe?". O fx.bin
antigo (gen_art.py) achatava cada animacao numa figurinha so, colada no
rival, em 12 quadros e 620 ms. Aqui sai a animacao INTEIRA, com as regras
do proprio Essentials (Scripts.rxdata #232 BattleAnimationPlayer e #193
Scene_PlayAnimations, lidos e conferidos):

  * 20 quadros por segundo, um por 50 ms;
  * a cel i de um quadro manda no sprite i; o 0 e o bicho de quem USA, o 1
    o de quem RECEBE (padrao -1/-2); do 2 em diante, particulas;
  * foco 1 = relativo ao alvo, 2 = ao usuario, 3 = a linha usuario->alvo
    (transformPoint, espelhando se a linha vira ao contrario), 4 = tela;
  * z: bicho do jogador 50, do rival 45, particulas por prioridade;
  * planos de fundo (z5) e de frente (z85), cor ou figura, com as
    interpolacoes dos timings 2/4.

TODA a geometria e resolvida AQUI, para os dois lados: p_<GOLPE>.bin
(voce usa) e f_<GOLPE>.bin (o rival usa; com o OppMove: do Anil quando
existe, que e o que o jogo faz). A placa nao sabe o que e um foco: so
desenha cada cel onde o arquivo diz.

VARIANTES (hitNum): p_<GOLPE>.<k>.bin e a animacao do Anil que vem k
posicoes depois da do golpe (move2anim.dat + k), que e o que o Anil toca
no impacto k+1 (pbProcessMoveHit passa hitNum = i) e, com k = 1, no turno
de CARGA (TwoTurnMove). So se empacota se ela for mesmo do golpe: nome com
"hit" (impacto) ou "charg" (carga); se a vizinha e outro golpe, o firmware
cai na de sempre. O nome no cabecalho vira "<GOLPE>.<k>".

O arquivo leva o NOME do golpe e o lado no cabecalho, e o firmware recusa
o que nao casar: nada de indice por linha de tabela, que ja fez o jogo
tocar a animacao do vizinho quando a tabela cresceu.

Formato TPAN v1 (little endian):
  "TPAN" u8 versao(1) u8 lado('p'|'f') u8 n  char nome[n]
  u16 nQuadros  u8 nImagens  u8 livre
     (livre: bit0 = o bicho do JOGADOR nao entra nesta animacao, bit1 = o
      do RIVAL. Desenha-se como se nao houvesse animacao. Ver modo_alvo.)
  imagem x nImagens:
     u8 w  u8 h  i8 ox  i8 oy  u8 nPal  u8 0
     u16 pal565[nPal]  u8 alfa[nPal]  u8 idx[w*h]
     (ox,oy = canto de cima a esquerda do recorte em relacao ao CENTRO da
      celula de 192, ja na escala da tela: as particulas vem pre-escaladas
      por K, entao zoom 100 = 1:1)
  quadro x nQuadros:
     u8 nCels  u8 fundo[4](r,g,b,peso)  u8 frente[4]
     cel x nCels (26 bytes):
        i8 img   (>=0 imagem; -1 = o bicho do JOGADOR; -2 = o do RIVAL)
        u8 camada (0 atras dos dois, 1 entre o rival e o jogador, 2 na frente)
        i16 x, y (particula: centro na tela; bicho: deslocamento do repouso)
        u16 zx, zy (%)   i16 angulo (graus, anti-horario)
        u8 opacidade     u8 flags (bit0 espelho, bits1-2 mistura 0/1/2)
        u8 cor[4] (r,g,b,a: puxa para essa cor)   i16 tom[4] (r,g,b,cinza)
"""
import io
import os
import struct
import sys

from PIL import Image

AQUI = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, AQUI)
import anil_anim as A  # noqa: E402

OUT = os.path.join(AQUI, "sdcard", "anim")

# O layout do TamaPoke (battle_ui.ino). Centros = os pontos que o combate
# ja usava para o efeito (chao - 70 / chao - 50).
BT_FOE_CX, BT_GROUND_FOE = 325, 178
BT_MINE_CX, BT_GROUND_MINE = 145, 312
TP_JOGADOR = (BT_MINE_CX, BT_GROUND_MINE - 70)
TP_RIVAL = (BT_FOE_CX, BT_GROUND_FOE - 50)
# Escala Anil -> TamaPoke: os bichos do Anil tem ~192 px, os nossos no
# combate ate ~137. As particulas saem pre-escaladas por isto.
K = 0.70
CEL = 192
Z_JOGADOR, Z_RIVAL = 50, 45


# ------------------------------------------------------------ geometria
def _eixo(x1, y1, x2, y2, px, py):
    dx, dy = x2 - x1, y2 - y1
    return (0.0 if dx == 0 else (px - x1) / dx,
            0.0 if dy == 0 else (py - y1) / dy)


def transform_point(src, dst, px, py):
    tx, ty = _eixo(*src, px, py)
    x3, y3, x4, y4 = dst
    return x3 + tx * (x4 - x3), y3 + ty * (y4 - y3)


def invertida(s0, s1, d0, d1):
    if s0 == s1:
        return False
    return (d0 > d1) if s0 < s1 else (d0 < d1)


# A TELA do Essentials (512x384) sobre o nosso circulo de 466: pela altura,
# a partir do centro. So para os PAINEIS de fundo (ver eh_painel).
S_TELA = 466 / 384.0


def eh_painel(sheet, pat):
    """Uma celula que preenche a casa de 192 quase inteira e um PAINEL de
    fundo (as linhas de velocidade e o espaco do Seismic Toss), nao uma
    particula. Painel vai pelo mapa da tela inteira, para cobrir o
    circulo; particula vai alinhada aos bichos. O zoom nao separa os dois
    (os paineis estao em 100, como qualquer particula)."""
    k = (id(sheet), pat)
    if k not in _paineis:
        c, r = pat % 5, pat // 5
        ok = False
        if (c + 1) * CEL <= sheet.width and (r + 1) * CEL <= sheet.height:
            cel = sheet.crop((c * CEL, r * CEL, (c + 1) * CEL, (r + 1) * CEL))
            bb = cel.getchannel("A").point(lambda a: 255 if a > 8 else 0).getbbox()
            ok = bool(bb) and bb[2] - bb[0] >= 176 and bb[3] - bb[1] >= 176
        _paineis[k] = ok
    return _paineis[k]


_paineis = {}


def _tela(x, y):
    # foco 4 (tela): o mesmo mapa da linha, que preserva a geometria
    # usuario->alvo e extrapola para os pontos fora dela.
    return transform_point((A.FOCUSUSER[0], A.FOCUSUSER[1],
                            A.FOCUSTARGET[0], A.FOCUSTARGET[1]),
                           (TP_JOGADOR[0], TP_JOGADOR[1], TP_RIVAL[0], TP_RIVAL[1]),
                           x, y)


# ------------------------------------------------------------ imagens
_folhas = {}


def _gira_matiz(im, hue):
    if not hue:
        return im
    hsv = im.convert("RGB").convert("HSV")
    h, s, v = hsv.split()
    h = h.point(lambda p: (p + hue * 255 // 360) % 256)
    out = Image.merge("HSV", (h, s, v)).convert("RGB").convert("RGBA")
    out.putalpha(im.getchannel("A"))
    return out


def folha(nome, hue=0):
    k = (nome, hue)
    if k not in _folhas:
        p = os.path.join(A.SHEETS, nome or "")
        # O Anil cita 105 folhas SEM extensao ("PRAS- Surf", "016-Ice01"):
        # o RGSS as acha assim mesmo. Aqui nao achava, e 72 golpes nossos
        # (Psychic, Surf, Rest, Bulk Up...) saiam sem nenhuma particula.
        if nome and not os.path.exists(p) and os.path.exists(p + ".png"):
            p += ".png"
        _folhas[k] = (_gira_matiz(Image.open(p).convert("RGBA"), hue)
                      if nome and os.path.exists(p) else None)
    return _folhas[k]


def rgb565(r, g, b):
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)


def empacota_celula(sheet, pat):
    """A celula `pat` da folha, escalada por K, recortada ao conteudo e com
    paleta RGBA de ate 255 cores. None se vazia ou fora da folha."""
    c, r = pat % 5, pat // 5
    if (c + 1) * CEL > sheet.width or (r + 1) * CEL > sheet.height:
        return None
    cel = sheet.crop((c * CEL, r * CEL, (c + 1) * CEL, (r + 1) * CEL))
    lado = round(CEL * K)
    cel = cel.resize((lado, lado), Image.LANCZOS)
    caixa = cel.getchannel("A").point(lambda a: 255 if a > 8 else 0).getbbox()
    if not caixa:
        return None
    rec = cel.crop(caixa)
    w, h = rec.size
    ox, oy = caixa[0] - lado // 2, caixa[1] - lado // 2
    # Paleta RGBA: o alfa por pixel e o que faz os brilhos e as bordas
    # macias. Alfa abaixo de 8 vira transparente de vez.
    q = rec.quantize(colors=255, method=Image.Quantize.FASTOCTREE)
    pal = q.getpalette("RGBA")[:255 * 4] if hasattr(q, "getpalette") else None
    idx = list(q.getdata())
    n = max(idx) + 1
    if pal is None or len(pal) < n * 4:
        raise RuntimeError("paleta RGBA indisponivel")
    p565, palfa = [], []
    for i in range(n):
        rr, gg, bb, aa = pal[i * 4:i * 4 + 4]
        p565.append(rgb565(rr, gg, bb))
        palfa.append(0 if aa <= 8 else aa)
    cab = struct.pack("<BBbbBB", w, h, ox, oy, n, 0)
    return (cab + struct.pack("<%dH" % n, *p565) + bytes(palfa) + bytes(idx))


# ------------------------------------------------------------ planos
class Plano:
    def __init__(self):
        self.figura = None      # cor media (r,g,b,alfa medio) da figura, ou None
        self.ox = self.oy = 0
        self.opacity = 0
        self.color = (0, 0, 0, 0)


_medias = {}


def media_da_figura(nome):
    """A figura de fundo vira a COR MEDIA dela: as 99 figuras somam 19 Mpx
    e nao cabem; a cor do fundo (escurecer, o azul do espaco) e o que se le."""
    if nome not in _medias:
        p = os.path.join(A.SHEETS, nome)
        if not os.path.exists(p):
            _medias[nome] = None
        else:
            im = Image.open(p).convert("RGBA")
            im.thumbnail((64, 64))
            px = list(im.getdata())
            sa = sum(a for *_c, a in px) or 1
            r = sum(c[0] * c[3] for c in px) / sa
            g = sum(c[1] * c[3] for c in px) / sa
            b = sum(c[2] * c[3] for c in px) / sa
            _medias[nome] = (r, g, b, sa / len(px))
    return _medias[nome]


def timings_do_quadro(tms, q, bgG, bgC, foG, foC, oldbg, oldfo):
    """Copia fiel de pbAnimation (timings 1..4), incluindo o tropeco de
    precedencia do Ruby que deixa fixa a opacidade de um plano de COR
    durante 2/4 (`oldbg[2] || 0 + ...` devolve oldbg[2])."""
    def interp(t, G, C, old):
        frac = (q - t["frame"]) / float(t["duration"])
        r, gg, b, a = t["rgba"]
        if G.figura is None:
            if t["opacity"] is not None:
                C.opacity = old[2] if old[2] is not None else t["opacity"] * frac
            oc = old[3] if old[3] is not None else (0, 0, 0, 0)
            C.color = tuple((oc[k] + (v - oc[k]) * frac) if v is not None else oc[k]
                            for k, v in enumerate((r, gg, b, a)))
        else:
            if t["opacity"] is not None and old[2] is not None:
                G.opacity = old[2] + (t["opacity"] - old[2]) * frac
            oc = old[3] if old[3] is not None else (0, 0, 0, 0)
            G.color = tuple((oc[k] + (v - oc[k]) * frac) if v is not None else oc[k]
                            for k, v in enumerate((r, gg, b, a)))

    for t in tms:
        if not t["duration"] or t["duration"] <= 0:
            continue
        if t["frame"] + t["duration"] < q or t["frame"] >= q:
            continue
        if t["type"] == 2:
            interp(t, bgG, bgC, oldbg)
        elif t["type"] == 4:
            interp(t, foG, foC, oldfo)
    for t in tms:
        if t["frame"] != q:
            continue
        ty = t["type"]
        if ty in (1, 3):
            G, C = (bgG, bgC) if ty == 1 else (foG, foC)
            r, gg, b, a = (v or 0 for v in t["rgba"])
            if t["name"]:
                G.figura = media_da_figura(t["name"])
                G.color = (r, gg, b, a)
                G.opacity = t["opacity"] or 0
                C.opacity = 0
            else:
                G.figura = None
                G.opacity = 0
                C.color = (r, gg, b, a)
                C.opacity = t["opacity"] or 0
        elif ty in (2, 4):
            G, C, old = (bgG, bgC, oldbg) if ty == 2 else (foG, foC, oldfo)
            if G.figura is None:
                old[:] = [0, 0, C.opacity or 0, C.color]
            else:
                old[:] = [G.ox or 0, G.oy or 0, G.opacity or 0, G.color]


def plano_em_um(C, G):
    """Os dois planos (cor e figura) numa so mistura (r,g,b,peso 0..255).
    O de cor puxa para cor*alfa com peso opacidade; o da figura, para a
    cor media (tingida) com peso alfa_medio*opacidade. Duas misturas
    seguidas de cor constante dao exatamente uma so."""
    camadas = []
    if C.opacity and C.opacity > 0:
        r, g, b, a = (v or 0 for v in C.color)
        k = a / 255.0
        camadas.append(((r * k, g * k, b * k), min(1.0, C.opacity / 255.0)))
    if G.figura is not None and G.opacity and G.opacity > 0:
        r, g, b, am = G.figura
        cr, cg, cb, ca = (v or 0 for v in G.color)
        if ca:
            r, g, b = (r + (cr - r) * ca / 255.0, g + (cg - g) * ca / 255.0,
                       b + (cb - b) * ca / 255.0)
        camadas.append(((r, g, b), min(1.0, am / 255.0 * G.opacity / 255.0)))
    if not camadas:
        return (0, 0, 0, 0)
    cor, w = camadas[0]
    for c2, w2 in camadas[1:]:
        wt = 1 - (1 - w) * (1 - w2)
        if wt <= 0:
            continue
        cor = tuple((cor[i] * w * (1 - w2) + c2[i] * w2) / wt for i in range(3))
        w = wt
    return tuple(max(0, min(255, round(v))) for v in cor) + (max(0, min(255, round(w * 255))),)


# ------------------------------------------------------------ uma animacao
# Os golpes que o Anil nao anima, com a animacao de um parecido. O NOME
# no cabecalho continua sendo o do golpe de verdade (empacota escreve
# `golpe`), entao a assinatura do firmware segue valendo.
EMPRESTADA = {
    "ATRAPAMOSCAS": "SNAPTRAP",      # mordida de planta que prende
    "DESLIZAMIENTO": "ICESPINNER",   # investida de gelo que acelera
    "ESCALOFRIO": "POWDERSNOW",      # o frio que pode congelar
    "IVYCUDGEL": "WOODHAMMER",       # a clava de madeira da Ogerpon
    "TRIPLEHIT": "TRIPLEKICK",       # tres golpes seguidos
}


# A NUVEM DE BRIGA. No Play Rough o bicho corre ate o alvo e os dois somem
# numa nuvem de onde saltam coracoes e estrelas. Nos dados do Anil o sprite
# de quem ataca continua la, parado em cima do alvo (cel 0, opacidade 255):
# quem o esconde e a nuvem, que no Anil mede 208x175 px. Aqui os bichos de
# costas chegam a 215 px e a nuvem (escalada por K) fica em 146x122: o bicho
# sobrava por todos os lados. O dono: "deveria ficar so a fumaca brigando,
# esta ficando o sprite do pokemon tambem".
#
# Entao, NESTES golpes, enquanto quem ataca esta parado em cima do alvo e ha
# nuvem na frente, ele nao se desenha. Volta a aparecer quando sai andando.
NUVEM_DE_BRIGA = {"PLAYROUGH"}
NUVEM_PERTO = 30      # px do Anil entre a cel do usuario e o foco do alvo
NUVEM_MIN = 3         # particulas na frente para contar como nuvem


# O ALVO de cada golpe no PBS do Anil. pbAnimation passa `targets[0]`
# como alvo da animacao, e pbFindTargets (script 170) diz quem e:
#   * User / UserOrNearAlly / UserAndAllies -> o proprio usuario. A cel 1
#     (o bicho de quem recebe) e o MESMO sprite que a cel 0: as duas mexem
#     em quem usa, e o rival nem entra.
#   * AllBattlers -> battlers[0], o do jogador: o proprio se o jogador usa.
#   * UserSide / FoeSide / BothSides / AllAllies / NearAlly -> nenhum
#     alvo: targetSprite e nil, a cel 1 se pula (`next if !sprite`) e o
#     rival fica como esta.
# Antes a cel 1 ia SEMPRE para o rival: Swords Dance, Reflect, Rain Dance
# etc. mexiam/tingiam/escondiam o bicho de quem nada tinha a ver.
ALVO_PROPRIO = {"User", "UserOrNearAlly", "UserAndAllies"}
ALVO_NENHUM = {"UserSide", "FoeSide", "BothSides", "AllAllies", "NearAlly"}
_alvos = {}


def alvos_pbs():
    if not _alvos:
        atual = None
        p = os.path.join(A.ANIL, "PBS", "moves.txt")
        for ln in io.open(p, encoding="utf-8-sig", errors="replace"):
            ln = ln.strip()
            if ln.startswith("[") and ln.endswith("]"):
                atual = ln[1:-1].strip()
            elif atual and ln.startswith("Target") and "=" in ln:
                _alvos[atual] = ln.split("=", 1)[1].strip()
    return _alvos


def modo_alvo(golpe, rival):
    """'normal', 'proprio' ou 'nenhum' para o golpe usado (nao o da
    animacao emprestada: o alvo e do golpe que se usa)."""
    # Struggle nao esta no PBS: e a classe Battle::Move::Struggle, com
    # @target = :NearOther fixo no script.
    t = alvos_pbs().get(golpe, "NearOther" if golpe == "STRUGGLE" else None)
    if t is None:
        raise KeyError("golpe sem Target no PBS do Anil: " + golpe)
    if t in ALVO_PROPRIO or (t == "AllBattlers" and not rival):
        return "proprio"
    if t in ALVO_NENHUM:
        return "nenhum"
    return "normal"


_lista = []
_m2a = []


def _indices():
    if not _lista:
        import marshal_lite
        _lista.extend(marshal_lite.load(A.RXDATA))
        _m2a.extend(marshal_lite.load(os.path.join(A.ANIL, "Data", "move2anim.dat")))
    return _lista, _m2a


def variante(golpe, rival, k):
    """(anim, trocada) do hitNum k, ou (None, False). Mesma escolha de
    pbFindMoveAnimDetails: o rival usa o OppMove: se existe (trocada),
    senao o Move:."""
    lista, m2a = _indices()
    g = EMPRESTADA.get(golpe, golpe)
    i, trocada = None, False
    if rival and g in m2a[1]:
        i, trocada = m2a[1][g], True
    elif g in m2a[0]:
        i = m2a[0][g]
    if i is None or i + k >= len(lista) or lista[i + k] is None:
        return None, False
    a = lista[i + k]
    nome = A.txt(a.iv.get("name"))
    baixo = nome.lower()
    if nome.startswith(("Move:", "OppMove:")):
        return None, False
    if nome.startswith("Common:") and not (k == 1 and "charg" in baixo):
        return None, False
    if "hit" not in baixo and not (k == 1 and "charg" in baixo):
        return None, False
    return a, trocada


def acha(todas, golpe, rival):
    """pbFindMoveAnimDetails: o jogador usa Move:X; o rival tenta OppMove:X
    (tocada com usuario e alvo trocados) e senao Move:X."""
    golpe = EMPRESTADA.get(golpe, golpe)
    if not rival:
        return todas.get("Move:" + golpe), False
    a = todas.get("OppMove:" + golpe)
    if a is not None:
        return a, True
    return todas.get("Move:" + golpe), False


def _i16(v):
    return max(-32768, min(32767, int(round(v or 0))))


def _u8(v):
    return max(0, min(255, int(round(v or 0))))


def empacota(todas, golpe, rival, hitk=0):
    if hitk:
        anim, trocada = variante(golpe, rival, hitk)
    else:
        anim, trocada = acha(todas, golpe, rival)
    if anim is None:
        return None
    sheet = folha(A.txt(anim.iv.get("graphic")), anim.iv.get("hue") or 0)
    tms = A.timings(anim)

    # Quem e quem NA TELA. O jogador esta sempre embaixo (z50) e o rival
    # em cima (z45); muda so quem usa.
    if not rival:
        usu, alv = ("J", TP_JOGADOR, Z_JOGADOR), ("R", TP_RIVAL, Z_RIVAL)
    else:
        usu, alv = ("R", TP_RIVAL, Z_RIVAL), ("J", TP_JOGADOR, Z_JOGADOR)
    real = usu        # quem usa de verdade, na tela
    if trocada:   # pbAnimationCore(anim, target, user, true)
        usu, alv = alv, usu
    modo = modo_alvo(golpe, rival)
    linha = (usu, alv)
    if modo == "proprio":
        # alvo = o proprio: os dois papeis sao o mesmo sprite, e a linha
        # usuario->alvo vira um ponto (transformPoint leva tudo a ele).
        usu, alv = real, real
        linha = (real, real)
    elif modo == "nenhum":
        # O papel sem bicho: getSpriteCenter(nil) = (0,0) do Anil, e a
        # linha cai no usuario (oldTargetX = oldUserX quando nao ha alvo).
        vazio = (None, _tela(0, 0), None)
        if trocada:   # o papel de "usuario" e o nil
            usu, alv = vazio, real
            linha = (vazio, real)
        else:
            usu, alv = real, vazio
            linha = (real, real)
    livre = 0
    if modo != "normal":
        livre = 2 if real[0] == "J" else 1   # o OUTRO lado fica de fora
    src = (A.FOCUSUSER[0], A.FOCUSUSER[1], A.FOCUSTARGET[0], A.FOCUSTARGET[1])
    dst = (linha[0][1][0], linha[0][1][1], linha[1][1][0], linha[1][1][1])
    inv = invertida(src[0], src[2], dst[0], dst[2])

    imagens, mapa = [], {}

    def imagem(pat):
        if pat not in mapa:
            b = empacota_celula(sheet, pat) if sheet is not None else None
            if b is None or len(imagens) >= 255:
                mapa[pat] = None
            else:
                mapa[pat] = len(imagens)
                imagens.append(b)
        return mapa[pat]

    bgG, bgC, foG, foC = Plano(), Plano(), Plano(), Plano()
    oldbg, oldfo = [None] * 4, [None] * 4
    quadros = []
    for q, fr in enumerate(anim):
        cels = []
        em_cima = False     # quem usa esta parado em cima do alvo
        for i, cel in enumerate(fr):
            if cel is None or i >= 60:
                continue
            pat = A.g(cel, A.PATTERN)
            if pat is None or A.g(cel, A.VISIBLE, 1) != 1:
                continue
            x, y = A.g(cel, A.X, 0), A.g(cel, A.Y, 0)
            if pat == -1 and ((x - A.FOCUSTARGET[0]) ** 2 +
                              (y - A.FOCUSTARGET[1]) ** 2) ** 0.5 < NUVEM_PERTO:
                em_cima = True
            espelho = bool(A.g(cel, A.MIRROR, 0))
            foc = A.g(cel, A.FOCUS, 4)
            if foc == 1:
                x = alv[1][0] + (x - A.FOCUSTARGET[0]) * K
                y = alv[1][1] + (y - A.FOCUSTARGET[1]) * K
            elif foc == 2:
                x = usu[1][0] + (x - A.FOCUSUSER[0]) * K
                y = usu[1][1] + (y - A.FOCUSUSER[1]) * K
            elif foc == 3:
                x, y = transform_point(src, dst, x, y)
                if inv and pat >= 0:
                    espelho = not espelho
            else:
                painel = pat >= 0 and sheet is not None and eh_painel(sheet, pat)
                if painel:
                    x = 233 + (x - 256) * S_TELA
                    y = 233 + (y - 192) * S_TELA
                else:
                    x, y = _tela(x, y)
            # O BICHO (padrao -1 = o de quem usa, -2 = o de quem recebe),
            # escrito pelo lado da TELA: -1 jogador, -2 rival.
            if pat in (-1, -2):
                quem = usu if pat == -1 else alv
                if quem[0] is None:
                    continue          # papel sem sprite: o Anil pula a cel
                img = -1 if quem[0] == "J" else -2
                repouso = quem[1]
                x, y = x - repouso[0], y - repouso[1]
                z = quem[2]
            elif pat >= 0:
                img = imagem(pat)
                if img is None:
                    continue
                pri = A.g(cel, A.PRIORITY, 1)
                fz = alv[2] if foc == 1 else usu[2] if foc == 2 else None
                if pri == 0:
                    z = 10
                elif pri == 2:
                    z = fz - 1 if fz is not None else 20
                elif pri == 3:
                    z = fz + 1 if fz is not None else 80
                else:
                    z = 80
            else:
                continue
            camada = 0 if z < Z_RIVAL else (1 if z < Z_JOGADOR else 2)
            if img < 0:
                camada = 0   # o bicho nao e particula: a camada nao se usa
            flags = (1 if espelho else 0) | ((_u8(A.g(cel, A.BLENDTYPE, 0)) & 3) << 1)
            cor = [_u8(A.g(cel, k, 0)) for k in (A.COLORRED, A.COLORGREEN,
                                                 A.COLORBLUE, A.COLORALPHA)]
            tom = [_i16(A.g(cel, k, 0)) for k in (A.TONERED, A.TONEGREEN,
                                                  A.TONEBLUE, A.TONEGRAY)]
            # O painel cresce do tamanho das particulas (K) para o da tela.
            # (+2%: os paineis se sobrepoem e somem as emendas de 1 px)
            fzoom = (S_TELA / K * 1.02) if (foc not in (1, 2, 3) and pat >= 0 and
                                  sheet is not None and eh_painel(sheet, pat)) else 1.0
            cels.append((z, i, struct.pack(
                "<bBhhHHhBB4B4h", img, camada, _i16(x), _i16(y),
                max(0, min(65535, int((A.g(cel, A.ZOOMX, 100) or 0) * fzoom))),
                max(0, min(65535, int((A.g(cel, A.ZOOMY, 100) or 0) * fzoom))),
                _i16(A.g(cel, A.ANGLE, 0)), _u8(A.g(cel, A.OPACITY, 255)),
                flags, *cor, *tom)))
        # Alvo = o proprio: a cel 0 e a 1 sao o MESMO sprite, e a 1 se
        # aplica depois (vence). Fica so a ultima cel de bicho por lado.
        if modo == "proprio":
            visto, unicas = set(), []
            for c in reversed(cels):
                im = struct.unpack_from("<b", c[2])[0]
                if im < 0:
                    if im in visto:
                        continue
                    visto.add(im)
                unicas.append(c)
            cels = unicas[::-1]
        if golpe in NUVEM_DE_BRIGA and em_cima:
            de_quem_usa = -1 if usu[0] == "J" else -2
            nuvem = sum(1 for c in cels if struct.unpack_from("<b", c[2])[0] >= 0)
            if nuvem >= NUVEM_MIN:
                for k, c in enumerate(cels):
                    if struct.unpack_from("<b", c[2])[0] == de_quem_usa:
                        b = bytearray(c[2])
                        b[12] = 0                      # a opacidade da cel
                        cels[k] = (c[0], c[1], bytes(b))
        timings_do_quadro(tms, q, bgG, bgC, foG, foC, oldbg, oldfo)
        cels.sort(key=lambda c: (c[0], c[1]))
        fundo, frente = plano_em_um(bgC, bgG), plano_em_um(foC, foG)
        quadros.append(struct.pack("<B4B4B", len(cels), *fundo, *frente)
                       + b"".join(c[2] for c in cels))

    if not quadros:
        return None
    nome = (golpe if not hitk else "%s.%d" % (golpe, hitk)).encode("ascii")
    cab = (b"TPAN" + struct.pack("<BBB", 1, ord("f" if rival else "p"), len(nome))
           + nome + struct.pack("<HBB", len(quadros), len(imagens), livre))
    return cab + b"".join(imagens) + b"".join(quadros)


def main():
    pedidos = [a.upper() for a in sys.argv[1:]]
    golpes = pedidos or A.moves_h_names()
    todas = A.load_all()
    os.makedirs(OUT, exist_ok=True)
    feitos, sem, total, maior = 0, [], 0, (0, "")
    variantes = 0
    for g in golpes:
        for rival in (False, True):
            b = empacota(todas, g, rival)
            if b is None:
                if not rival:
                    sem.append(g)
                continue
            nome = ("f_" if rival else "p_") + g + ".bin"
            with open(os.path.join(OUT, nome), "wb") as fh:
                fh.write(b)
            feitos += 1
            total += len(b)
            if len(b) > maior[0]:
                maior = (len(b), nome)
            # os impactos seguintes e a carga (hitNum 1..). A maioria dos
            # "X hit N" do Anil e COPIA do hit 1 (74 de 107): essas nao se
            # gravam, o firmware cai na de sempre sem reler o SD.
            for k in range(1, 10):
                nv = ("f_" if rival else "p_") + g + ".%d.bin" % k
                if os.path.exists(os.path.join(OUT, nv)):
                    os.remove(os.path.join(OUT, nv))
            for k in range(1, 10):
                bv = empacota(todas, g, rival, k)
                if bv is None:
                    break
                nv = ("f_" if rival else "p_") + g + ".%d.bin" % k
                if bv[7 + bv[6]:] == b[7 + b[6]:]:
                    continue
                with open(os.path.join(OUT, nv), "wb") as fh:
                    fh.write(bv)
                feitos += 1
                variantes += 1
                total += len(bv)
    print("%d arquivos (%d variantes de impacto/carga), %.1f MB, maior %s (%d KB)" % (
        feitos, variantes, total / 1e6, maior[1], maior[0] // 1024))
    if sem:
        print("sem animacao no Anil (%d): %s" % (len(sem), " ".join(sem)))


if __name__ == "__main__":
    main()
