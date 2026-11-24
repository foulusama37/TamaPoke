// A AVENTURA: para quando o jogo fica parado.
//
// O titular sai por ai com os sprites do SpriteCollab (/av/, ver
// tools/pack_aventura.py), e quem nao tem sprite la (44 especies, a maioria
// das megas, as formas) entra com o sprite padrao do jogo, no MESMO tamanho:
// a escala sai do corpo de cada um (avAlvo), nao do arquivo.
//
// O que se ganha NAO sai desta tela: e a conta do relogio em Pet::aventuraFim,
// e vale com o aparelho desligado. Mas a cena so mostra o que e verdade: quem
// entra para brincar e quem sai no resumo (Pet::aventuraElenco), e os itens
// achados sao os que vao para a mochila (Pet::aventuraItem).
//
// Cada trecho de AV_SEG_MS e um evento, tirado da semente e do numero do
// trecho, sem roteiro guardado.
enum : uint8_t { AV_ANDA, AV_ANIMA, AV_AMIGO, AV_LUTA, AV_ACHA, AV_SONECA, AV_MEGA, AV_EVENTOS };
#define AV_SEG_MS 7000
#define AV_CHAO 330
#define AV_ENTRA_MS 1400       // quanto o outro leva para entrar e para sair
#define AV_BTN_X 158           // o botao de acabar
#define AV_BTN_Y 392
#define AV_BTN_W 150
#define AV_BTN_H 40
#define AV_LINHAS 5            // linhas por pagina do resumo
#define AV_ZOOM 4.0f           // ver avAlvo
#define AV_RAZAO_PADRAO 0.243f

bool avOpen = false;
int8_t avFixo = -1;            // o teste escolhe o evento; -1 = sorteado
int8_t avFixoClima = -1;       // o teste escolhe o clima
AvResumo avRes;
static uint8_t avFase = 0;     // 0 a cena, 1 o resumo
static uint8_t avPagina = 0;
static uint32_t avAt = 0, avN = 0xFFFFFFFF, avUltBg = 0xFFFFFFFF;
static uint8_t avEv = AV_ANDA, avItem = 0, avItensVistos = 0, avAnimEu = PMD_IDLE, avAnimOutro = PMD_IDLE;
static int16_t avMegaFila = -1;
static bool avEuPronto = false;
static AvAtor avEu, avOutro, avMega;
// Onde cada um fica quando estao os dois: o grande abre espaco.
static int avXEu = 136, avXOutro = 330;
static int16_t avUltimoAmigo = 0;

static uint32_t avRnd(uint32_t n, uint32_t k) {
  uint32_t x = pet.avSeed * 2654435761u + n * 40503u + k * 2166136261u;
  x ^= x >> 15;
  x *= 2246822519u;
  x ^= x >> 13;
  return x;
}

// O que ele pode fazer quando esta so se divertindo: o que tiver, menos o que
// so faz sentido num momento certo (apanhar, dormir) e o andar.
static void avMontaPool(AvAtor &a) {
  a.nPool = 0;
  for (uint8_t i = 0; i < PMD_NACTS; i++) {
    if (!a.m.has(i) || i == PMD_IDLE || i == PMD_WALKL || i == PMD_WALKR ||
        i == PMD_SLEEP || i == PMD_HURT)
      continue;
    a.pool[a.nPool++] = i;
  }
}

// O sprite do PMD; sem ele, o padrao do jogo.
static bool avCarrega(AvAtor &a, int16_t dex, bool shiny, bool femea, uint8_t forma) {
  a.dex = dex;
  a.real = false;
  if (forma == 0 && a.m.loadAv(dex, shiny)) {
    a.real = true;
  } else {
    int16_t fila = formRow(dex, forma);
    bool ok = fila >= 0 ? a.m.loadForm(fila) : a.m.load(dex, shiny, femea);
    if (!ok) return false;
  }
  avMontaPool(a);
  return true;
}

static int16_t avMegaDoTitular() {
  const ItemEntry &it = itemAt(pet.heldItem);
  return it.fx == IT_MEGA ? megaFormOf(pet.speciesId, it.arg, pet.form) : (int16_t)-1;
}

static float avAlvo(const AvAtor &a);
static bool avCarregaMega(int16_t fila) {
  const MegaForm &f = MEGA_TBL[fila];
  avMega.dex = f.base;
  avMega.alvoFixo = 0;
  avMega.real = avMega.m.loadAvMega(f.base, f.form);
  if (!avMega.real && !avMega.m.loadMega(fila, pet.shiny)) return false;
  // Sem sprite da aventura, a mega vem do desenho do JOGO, e a tabela de
  // alturas so conhece a forma normal: o Mega Blaziken saia do tamanho do
  // Blaziken encaixado num quadro esticado pelas chamas, ou seja, MENOR que
  // ele. Agora: o tamanho do normal na cena vezes o quanto a mega e maior
  // que o normal no desenho do jogo (os dois do mesmo artista e escala).
  if (!avMega.real) {
    static PmdMon normal;
    if (normal.load(f.base, false, false)) {
      const float r = (float)avMega.m.acts[PMD_IDLE].inkH() /
                      (float)normal.acts[PMD_IDLE].inkH();
      normal.unload();
      const float base = (avEu.dex == f.base) ? avAlvo(avEu)
                         : altoPedido(f.base, HOME_K) * AV_RAZAO_PADRAO * AV_ZOOM;
      avMega.alvoFixo = base * r;
    }
  }
  avMontaPool(avMega);
  return true;
}

// O proximo item achado que ainda nao apareceu, ate onde o relogio ja chegou.
static uint8_t avProximoItem() {
  uint32_t e = rtcEpoch();
  uint16_t rolos = pet.aventuraRolos(e);
  uint8_t achados = 0;
  for (uint16_t i = 0; i < rolos; i++) {
    uint8_t id = pet.aventuraItem(i);
    if (!id) continue;
    if (achados++ == avItensVistos) return id;
    if (achados >= AV_MAX_ITENS) break;
  }
  return 0;
}

static void avPrepara(uint32_t n) {
  avN = n;
  // o cenario muda de vez em quando, nao a cada evento
  uint32_t qualBg = n / 8;
  if (qualBg != avUltBg) {
    avUltBg = qualBg;
    static const uint8_t LUGARES[] = { BG_FIELD, BG_FOREST, BG_BEACH, BG_MOUNTAIN, BG_SNOW, BG_CAVE, BG_CITY };
    bgImg.load(LUGARES[avRnd(qualBg, 7) % sizeof(LUGARES)]);
    // O clima e so enfeite: metade das vezes nenhum. Usa o mesmo desenho da
    // batalha, que olha bt.b.weather; com o combate fechado ninguem mais le.
    const uint8_t w = (uint8_t)(avRnd(qualBg, 8) % 8);
    bt.b.weather = (avFixoClima >= 0) ? (uint8_t)avFixoClima : (w <= WE_HAIL ? w : (uint8_t)WE_NONE);
  }
  if (!avEuPronto || avEu.dex != pet.speciesId)
    avEuPronto = avCarrega(avEu, pet.speciesId, pet.shiny, pet.female, pet.form);
  avEv = avFixo >= 0 ? (uint8_t)avFixo : (uint8_t)(avRnd(n, 1) % AV_EVENTOS);
  if (avEv == AV_MEGA) {
    avMegaFila = avMegaDoTitular();
    if (avMegaFila < 0 || !avCarregaMega(avMegaFila)) avEv = AV_ANIMA;
  }
  if (avEv == AV_AMIGO) {
    AvParticipa elenco[AV_MAX_ELENCO];
    uint8_t q = pet.aventuraElenco(elenco);
    bool ok = false;
    if (q > 1) {
      uint8_t nCaixa = 0;
      for (uint8_t i = 1; i < q; i++) if (elenco[i].origen == 2) nCaixa++;
      const uint8_t nTime = (uint8_t)(q - 1 - nCaixa);
      // Metade das vezes da caixa, quando ha: se nao, os seis do time
      // apareceriam sempre, e os da caixa quase nunca.
      bool daCaixa = nCaixa && (!nTime || (avRnd(n, 4) & 1));
      uint8_t ini = daCaixa ? (uint8_t)(1 + nTime) : (uint8_t)1;
      uint8_t tot = daCaixa ? nCaixa : nTime;
      uint8_t k = (uint8_t)(avRnd(n, 3) % tot);
      if (tot > 1 && elenco[ini + k].dex == avUltimoAmigo) k = (uint8_t)((k + 1) % tot);
      const AvParticipa &p = elenco[ini + k];
      avUltimoAmigo = p.dex;
      ok = avCarrega(avOutro, p.dex, p.shiny, false, 0);
    }
    if (!ok) avEv = AV_ANIMA;
  }
  if (avEv == AV_LUTA) {
    uint8_t hora = (uint8_t)(pet.lastSeenEpoch ? (pet.lastSeenEpoch / 3600) % 24 : 13);
    int16_t d = pet.pickWildSpecies(hora);
    if (d < 1 || !avCarrega(avOutro, d, false, false, 0)) avEv = AV_ANIMA;
    // Leva 72: o selvagem do passeio fica VISTO (so se apareceu de fato).
    else pet.markSeen(d);
  }
  if (avEv == AV_ACHA) {
    avItem = avProximoItem();
    if (avItem && ART_ITEM[avItem]) avItensVistos++;
    else avEv = AV_ANIMA;
  }
  if (avEv == AV_AMIGO || avEv == AV_LUTA) {
    avXEu = 136;
    avXOutro = 330;
    if (avXEu - avMeiaLargura(avEu) < 24) avXEu = 24 + avMeiaLargura(avEu);
    if (avXOutro + avMeiaLargura(avOutro) > 442) avXOutro = 442 - avMeiaLargura(avOutro);
  }
  avAnimEu = avEu.nPool ? avEu.pool[avRnd(n, 20) % avEu.nPool] : (uint8_t)PMD_IDLE;
  avAnimOutro = avOutro.nPool ? avOutro.pool[avRnd(n, 21) % avOutro.nPool] : (uint8_t)PMD_IDLE;
}

// A altura do corpo na cena. O MESMO zoom para todo sprite do PMD: eles ja vem
// na proporcao certa uns dos outros (Gallade 34 px, Minun 22, Guzzlord 51). Nada
// de piso: foi um piso que deixou um Minun do tamanho de um Gallade.
//
// Quem cai no sprite padrao nao tem como ser medido assim (o desenho e outro), e
// usa a tabela do jogo convertida para a mesma escala pela razao medida no
// conjunto (leva35.py).
#define AV_ALTO_MAX 190.0f
#define AV_LARGO_MAX 300.0f
static float avAlvo(const AvAtor &a) {
  const PmdAct &idle = a.m.acts[PMD_IDLE];
  float alvo = a.alvoFixo > 0 ? a.alvoFixo
             : a.real ? idle.inkH() * AV_ZOOM
                      : altoPedido(a.dex, HOME_K) * AV_RAZAO_PADRAO * AV_ZOOM;
  if (alvo > AV_ALTO_MAX) alvo = AV_ALTO_MAX;
  if (idle.inkH() && idle.inkR > idle.inkL) {
    // A TINTA, nao a caixa: a caixa e simetrica em volta do centro, e a cauda
    // do Guzzlord, que vai para um lado so, espremia ele a metade.
    float largura = alvo * (idle.inkR - idle.inkL + 1) / idle.inkH();
    if (largura > AV_LARGO_MAX) alvo = alvo * AV_LARGO_MAX / largura;
  }
  return alvo;
}

// Metade da largura dele na tela.
// Metade da largura para o LADO QUE MAIS PASSA do centro (para caber na tela).
static int avMeiaLargura(const AvAtor &a) {
  const PmdAct &idle = a.m.acts[PMD_IDLE];
  if (!idle.inkH() || idle.inkR <= idle.inkL) return 40;
  const int meio = idle.w / 2;
  const int esq = meio - idle.inkL, dir = idle.inkR + 1 - meio;
  return (int)(avAlvo(a) * (esq > dir ? esq : dir) / idle.inkH());
}

// `direita` = para onde olha. O sprite do PMD olha para a direita de nascenca
// e o padrao para a esquerda: o que nao for de nascenca, sai espelhado.
// `avPasso`: com dois em cena, primeiro passam as SOMBRAS dos dois e so depois os
// corpos. Desenhando um inteiro de cada vez, a sombra do da esquerda, que se
// inclina para a direita, caia por cima do outro.
static uint8_t avPasso = 0;    // 0 tudo, 1 so sombra, 2 so corpo
static void avBicho(AvAtor &a, uint8_t act, bool direita, int x, uint32_t t, float dx = 0) {
  PmdMon &m = a.m;
  if (act == PMD_WALKL || act == PMD_WALKR) {
    if (a.real) act = PMD_WALKR;
    else act = direita ? PMD_WALKR : PMD_WALKL;
  }
  if (!m.has(act)) act = PMD_IDLE;
  if (!m.has(act)) return;
  const bool andando = !a.real && (act == PMD_WALKL || act == PMD_WALKR);
  const uint8_t ink = m.acts[PMD_IDLE].inkH();
  // A mesma escala para TODAS as acoes do bicho: a do Idle. Medindo cada acao,
  // ele mudaria de tamanho ao passar de andar para atacar.
  PetFx fx;
  fx.sx = fx.sy = ink ? avAlvo(a) / ink : 1.0f;
  gEspelho = andando ? false : (a.real != direita);
  if (avPasso != 2) {
    gSombra.on = !gSombra.desligada;
    gSombra.num = 5;
    gSombra.den = 9;
    if (gSombra.on) drawPmdActM(m, act, x, AV_CHAO, t, true, false, 1, &fx, 0);
    gSombra.on = false;
  }
  if (avPasso != 1) {
    fx.dx = dx;
    drawPmdActM(m, act, x, AV_CHAO, t, true, false, 1, &fx, 0);
  }
  gEspelho = false;
}

static void avBalao(int x, uint8_t estado, int dx = 0) {
  if (porMon.dex != (uint16_t)pet.speciesId || porMon.shiny != pet.shiny || porMon.mega) return;
  const Art *c = porMon.cara(estado);
  if (!c) return;
  const int w = c->w * 2 + 12, h = c->h * 2 + 12;
  const int topo = AV_CHAO - (int)avAlvo(avEu);
  int y = topo - h - 8;
  if (y < 56) y = 56;
  // Sem caber em cima da cabeca, vai para o lado: em cima da cara, nao.
  if (!dx && y + h > topo - 40) dx = avMeiaLargura(avEu) + w / 2 + 4;
  int bx = x + dx - w / 2;
  if (bx < 62) bx = 62;
  if (bx + w > 404) bx = 404 - w;
  gfx->fillRoundRect(bx - 2, y - 2, w + 4, h + 4, 12, INK_K);
  gfx->fillRoundRect(bx, y, w, h, 10, UI_WHITE);
  drawArt(*c, bx + w / 2, y + 6 + c->h * 2, 2);
}

// O simbolo sobe devagar e some: nota, coracao, raiva, "!"...
static void avSimbolo(const Art &a, int x, int y, uint32_t u, uint32_t dura) {
  const uint32_t f = u % dura;
  if (f > dura * 3 / 5) return;
  drawArt(a, x, y - (int)(f / 40), 1);
}

// Ondinhas de som saindo de (x,y) para o lado de `dir`: tres arcos que abrem.
static void avOndas(int x, int y, int dir, uint32_t u) {
  uint16_t *fb = gfx->getFramebuffer();
  if (!fb) return;
  for (int k = 0; k < 3; k++) {
    const int r = 14 + (int)((u / 12 + k * 14) % 42);
    for (int g = -35; g <= 35; g += 3) {
      const float th = g * (float)PI / 180.0f;
      const int px = x + dir * (int)(cosf(th) * r), py = y + (int)(sinf(th) * r);
      if (px >= 1 && py >= 1 && px < LCD_WIDTH - 1 && py < LCD_HEIGHT - 1)
        gfx->fillRect(px, py, 2, 2, UI_WHITE);
    }
  }
}

// Para o teste dos gigantes: a altura e a largura que este bicho ocupa.
void aventuraMede(int16_t dex, int *alto, int *largo) {
  static AvAtor t;
  *alto = *largo = 0;
  if (!avCarrega(t, dex, false, false, 0)) return;
  const PmdAct &i = t.m.acts[PMD_IDLE];
  float k = i.inkH() ? avAlvo(t) / i.inkH() : 1.0f;
  *alto = (int)(i.inkH() * k + 0.5f);
  *largo = (int)((i.inkR - i.inkL + 1) * k + 0.5f);
  t.m.unload();
}

// Igual, para a MEGA da fila `fila` (o evento AV_MEGA). Sem isto, o
// tamanho das 121 megas na cena nao tinha quem o olhasse.
void aventuraMedeMega(int16_t fila, int *alto, int *largo, bool *real) {
  *alto = *largo = 0;
  *real = false;
  avEuPronto = false;
  if (!avCarrega(avEu, MEGA_TBL[fila].base, false, false, 0)) return;
  if (!avCarregaMega(fila)) return;
  const PmdAct &i = avMega.m.acts[PMD_IDLE];
  float k = i.inkH() ? avAlvo(avMega) / i.inkH() : 1.0f;
  *alto = (int)(i.inkH() * k + 0.5f);
  *largo = (int)((i.inkR - i.inkL + 1) * k + 0.5f);
  *real = avMega.real;
  avMega.m.unload();
}

void aventuraAbre() {
  avOpen = true;
  avFase = 0;
  avAt = millis();
  avN = 0xFFFFFFFF;
  avUltBg = 0xFFFFFFFF;
  avItensVistos = 0;
  avEuPronto = false;
}

// Do botao do mapa. Se ja ha uma aventura em curso, so volta a ela.
void aventuraComeca() {
  if (!pet.aventuraAtiva() &&
      !pet.aventuraComeca(rtcEpoch(), (uint32_t)random(0x7FFFFFFF) + millis()))
    return;
  aventuraAbre();
}

static void avSolta() {
  avEu.m.unload();
  avOutro.m.unload();
  avMega.m.unload();
  avEuPronto = false;
}

// true se o toque era da aventura (o resto do jogo nao o ve).
bool aventuraTap(int16_t x, int16_t y) {
  if (avFase == 0) {
    if (x >= AV_BTN_X - 12 && x <= AV_BTN_X + AV_BTN_W + 12 &&
        y >= AV_BTN_Y - 12 && y <= AV_BTN_Y + AV_BTN_H + 12) {
      pet.aventuraFim(rtcEpoch(), avRes);
      avSolta();
      bt.b.weather = WE_NONE;      // o enfeite nao fica para a batalha
      avFase = 1;
      avPagina = 0;
      sfxPlay(SFX_TAP);
    }
    // Fora do botao nao faz nada. Quem deixa isto rodando na mesa nao pode
    // acabar a aventura por encostar na tela.
    return true;
  }
  // o resumo: um toque em qualquer lugar passa de pagina
  uint8_t linhas = (uint8_t)(avRes.nMons + avRes.nItens);
  uint8_t paginas = (uint8_t)((linhas + AV_LINHAS - 1) / AV_LINHAS);
  if (paginas == 0) paginas = 1;
  sfxPlay(SFX_TAP);
  if (avPagina + 1 < paginas) avPagina++;
  else avOpen = false;
  return true;
}

static void avTempo(char *s, size_t n, uint32_t min) {
  if (min >= 60) snprintf(s, n, "%uh%02u", (unsigned)(min / 60), (unsigned)(min % 60));
  else snprintf(s, n, "%u min", (unsigned)min);
}

static void renderResumo() {
  drawDexSkin();
  gfx->setTextColor(CARD_INK);
  setSize(3);
  setCur(centerX(T(S_AVENTURA), 3), 44);
  printT(T(S_AVENTURA));
  char t[48], l[12];
  avTempo(l, sizeof(l), avRes.minutos);
  snprintf(t, sizeof(t), T(S_AV_FORA_FMT), l);
  gfx->setTextColor(CARD_SOFT);
  setSize(2);
  setCur(centerX(t, 2), 76);
  printT(t);

  if (!avRes.nMons && !avRes.nItens) {
    const char *nada = T(S_AV_CURTO);
    gfx->setTextColor(CARD_INK);
    setCur(centerX(nada, 2), 200);
    printT(nada);
  }
  const uint8_t ini = (uint8_t)(avPagina * AV_LINHAS);
  for (uint8_t k = 0; k < AV_LINHAS; k++) {
    const uint8_t i = (uint8_t)(ini + k);
    const int y = 112 + k * 50;
    if (i < avRes.nMons) {
      const AvParticipa &p = avRes.mons[i];
      const uint8_t *th = thumbs.get(p.dex);
      (void)th;
      gfx->setTextColor(CARD_INK);
      setSize(2);
      setCur(70, y + 4);
      printT(dexName(p.dex));
      char e[40];
      if (p.subiu) snprintf(e, sizeof(e), T(S_AV_SUBIU_FMT), (unsigned long)p.exp);
      else snprintf(e, sizeof(e), "+%lu EXP", (unsigned long)p.exp);
      gfx->setTextColor(p.subiu ? UI_BAR_OK : CARD_SOFT);
      setSize(1);
      setCur(70, y + 26);
      printT(e);
    } else if (i < avRes.nMons + avRes.nItens) {
      const uint8_t id = avRes.itens[i - avRes.nMons];
      if (ART_ITEM[id]) drawArt(*ART_ITEM[id], 88, y + 34, 1);
      gfx->setTextColor(CARD_INK);
      setSize(2);
      setCur(116, y + 10);
      printT(itemName(id));
    }
  }
  const uint8_t linhas = (uint8_t)(avRes.nMons + avRes.nItens);
  const uint8_t paginas = linhas ? (uint8_t)((linhas + AV_LINHAS - 1) / AV_LINHAS) : 1;
  gfx->setTextColor(CARD_SOFT);
  setSize(1);
  if (paginas > 1) snprintf(t, sizeof(t), T(S_AV_PROX_FMT), avPagina + 1, paginas);
  else snprintf(t, sizeof(t), "%s", T(S_BACK));
  setCur(centerX(t, 1), 408);
  printT(t);
}

void renderAventura() {
  if (avFase == 1) {
    renderResumo();
    return;
  }
  const uint32_t now = millis(), t = now - avAt;
  const uint32_t n = t / AV_SEG_MS, u = t % AV_SEG_MS;
  if (n != avN) avPrepara(n);
  // A mesma musica de casa; no escuro o loop() a cala.
  if (dimStage == 0) homeMusic(gNight);
  avisoDeSave();
  drawBattleScene(now);
  if (!avEuPronto) return;
  btClimaAtras(now);
  const float vai = (float)u / AV_SEG_MS;
  char txt[40];
  txt[0] = 0;
  switch (avEv) {
    case AV_ANDA: {
      const bool ida = vai < 0.5f;
      const float p = ida ? vai * 2 : (1 - vai) * 2;
      // de ponta a ponta, sem sair do circulo por grande que seja
      const int ml = avMeiaLargura(avEu);
      const int xa = ml + 40 > 96 ? ml + 40 : 96, xb = 426 - ml < 370 ? 426 - ml : 370;
      avBicho(avEu, PMD_WALKR, ida, xb > xa ? xa + (int)(p * (xb - xa)) : CX, u);
      break;
    }
    case AV_ANIMA:
      avBicho(avEu, avAnimEu, avRnd(avN, 22) & 1, CX, u);
      avBalao(CX, POR_FESTA, avMeiaLargura(avEu) + 54);
      {
        const int topo = AV_CHAO - (int)avAlvo(avEu);
        switch (avRnd(avN, 30) % 3) {
          case 0: avSimbolo(ART_NOTA, CX - 74, topo + 20, u, 2400); break;
          case 1: avSimbolo(ART_CORACAO, CX - 74, topo + 20, u, 2400); break;
          default: avOndas(CX + avMeiaLargura(avEu) / 2, topo + 40, 1, u); break;
        }
      }
      break;
    case AV_AMIGO:
    case AV_LUTA: {
      const bool luta = avEv == AV_LUTA;
      // o outro entra pela direita e sai por onde veio
      int xo = avXOutro;
      uint8_t ao = PMD_IDLE;
      bool oDir = false;
      if (u < AV_ENTRA_MS) {
        xo = 440 - (int)(110.0f * u / AV_ENTRA_MS);
        ao = PMD_WALKL;
      } else if (u > AV_SEG_MS - AV_ENTRA_MS) {
        xo = avXOutro + (int)(110.0f * (u - (AV_SEG_MS - AV_ENTRA_MS)) / AV_ENTRA_MS);
        ao = PMD_WALKR;
        oDir = true;
      }
      const bool meio = u >= AV_ENTRA_MS && u <= AV_SEG_MS - AV_ENTRA_MS;
      uint8_t ae = PMD_IDLE;
      float dxe = 0, dxo = 0;
      bool impacto = false;
      if (meio && !luta) {
        ae = avAnimEu;
        ao = avAnimOutro;
      } else if (meio) {
        // um de cada vez: 800 ms bate, o outro apanha
        const uint32_t v = u - AV_ENTRA_MS;
        const uint32_t w = v % 800;
        const bool minha = (v / 800) % 2 == 0;
        const float golpe = sinf(w / 800.0f * (float)PI);
        impacto = w > 300 && w < 500;
        if (minha) {
          ae = PMD_ATTACK;
          ao = PMD_HURT;
          dxe = golpe * 50;
          dxo = golpe * 12;
        } else {
          ao = PMD_ATTACK;
          ae = PMD_HURT;
          dxo = -golpe * 50;
          dxe = -golpe * 12;
        }
      }
      avPasso = 1;
      avBicho(avOutro, ao, oDir, xo, u, dxo);
      avBicho(avEu, ae, true, avXEu, u, dxe);
      avPasso = 2;
      avBicho(avOutro, ao, oDir, xo, u, dxo);
      avBicho(avEu, ae, true, avXEu, u, dxe);
      avPasso = 0;
      if (impacto) {
        const bool minha = ((u - AV_ENTRA_MS) / 800) % 2 == 0;
        const int ix = minha ? xo - 24 : avXEu + 24, iy = AV_CHAO - 50;
        gfx->fillTriangle(ix - 18, iy, ix + 18, iy, ix, iy - 18, UI_WHITE);
        gfx->fillTriangle(ix - 18, iy, ix + 18, iy, ix, iy + 18, UI_WHITE);
        gfx->fillCircle(ix, iy, 7, UI_BAR_WARN);
      }
      // A raiva em quem apanhou, ate a proxima pancada.
      if (meio && luta) {
        const uint32_t v = u - AV_ENTRA_MS;
        const bool minha = (v / 800) % 2 == 0;
        if (v % 800 > 300) {
          const int rx = minha ? xo + 30 : avXEu + 30;
          const AvAtor &q = minha ? avOutro : avEu;
          drawArt(ART_RAIVA, rx, AV_CHAO - (int)avAlvo(q) + 30, 1);
        }
      }
      if (meio && !luta) {
        // brincando: nota ou coracao, um em cima de cada
        const Art &sim = (avRnd(avN, 50) & 1) ? ART_NOTA : ART_CORACAO;
        avSimbolo(sim, avXEu + 40, AV_CHAO - (int)avAlvo(avEu) + 20, u, 2000);
        avSimbolo(sim, xo, AV_CHAO - (int)avAlvo(avOutro) + 10, u + 900, 2000);
      }
      if (u < AV_ENTRA_MS && luta) drawArt(ART_PRONTO, avXEu + 40, AV_CHAO - (int)avAlvo(avEu), 1);
      avBalao(avXEu, luta ? POR_PRONTO : POR_FESTA);
      snprintf(txt, sizeof(txt), T(luta ? S_AV_SELVAGEM_FMT : S_AV_ENTROU_FMT), dexName(avOutro.dex));
      break;
    }
    case AV_ACHA: {
      avBicho(avEu, u < 2600 ? (uint8_t)PMD_IDLE : avAnimEu, true, CX, u);
      if (u < 1200) drawArt(ART_PRONTO, CX - 60, AV_CHAO - (int)avAlvo(avEu) + 20, 1);
      if (u >= 1200) {
        const float sobe = u < 2200 ? (u - 1200) / 1000.0f : 1.0f;
        drawArt(*ART_ITEM[avItem], CX, AV_CHAO - (int)avAlvo(avEu) - (int)(sobe * 24), 3);
      }
      if (u >= 2600) {
        avBalao(CX, POR_FESTA, avMeiaLargura(avEu) + 54);
        snprintf(txt, sizeof(txt), T(S_AV_ACHOU_FMT), itemName(avItem));
        avSimbolo(ART_CORACAO, CX - 74, AV_CHAO - (int)avAlvo(avEu) + 30, u, 2400);
      }
      break;
    }
    case AV_SONECA:
      avBicho(avEu, PMD_SLEEP, true, CX, u);
      avBalao(CX, POR_SONO, -(avMeiaLargura(avEu) + 54));
      for (int k = 0; k < 2; k++) {
        const uint32_t f = (u + k * 900) % 1800;
        const int zy = AV_CHAO - (int)(avAlvo(avEu) * 0.6f) - (int)(f / 26);
        drawArt(ART_ZZZ, CX + 50 + k * 22 + (int)(f / 110), zy, 1);
      }
      break;
    case AV_MEGA: {
      const bool mega = u >= 1800 && u < 5600;
      if (mega) avBicho(avMega, avMega.nPool ? avMega.pool[avRnd(avN, 23) % avMega.nPool] : (uint8_t)PMD_IDLE, true, CX, u);
      else      avBicho(avEu, PMD_IDLE, true, CX, u);
      const int cy = AV_CHAO - 70;
      if (u < 1800) {
        for (int k = 0; k < 3; k++) {
          int r = 24 + (int)((u + k * 420) % 1260) / 7;
          gfx->drawCircle(CX, cy, r, k & 1 ? C565(0x4a, 0x8d, 0xd4) : C565(0xd4, 0x52, 0x7e));
          gfx->drawCircle(CX, cy, r - 1, k & 1 ? C565(0x4a, 0x8d, 0xd4) : C565(0xd4, 0x52, 0x7e));
        }
      }
      // o clarao, na troca de sprite
      for (uint32_t troca : { (uint32_t)1800, (uint32_t)5600 }) {
        int32_t d = (int32_t)u - (int32_t)troca;
        if (d > -220 && d < 220) gfx->fillCircle(CX, cy, 150 - abs(d) * 150 / 220, UI_WHITE);
      }
      if (mega) snprintf(txt, sizeof(txt), "%s!", MEGA_TBL[avMegaFila].name);
      break;
    }
  }
  btClimaFrente(now);
  if (txt[0]) printOutlined(centerX(txt, 2), 362, txt, 2, UI_WHITE);

  // quanto ja passou, e o botao de acabar
  char tempo[28], l[12];
  uint32_t e = rtcEpoch();
  avTempo(l, sizeof(l), pet.aventuraMinutos(e));
  if (pet.aventuraMinutos(e) >= AV_TETO_MIN) snprintf(tempo, sizeof(tempo), "%s  (max)", l);
  else snprintf(tempo, sizeof(tempo), "%s", l);
  printOutlined(centerX(tempo, 2), 30, tempo, 2, UI_WHITE);
  gfx->fillRoundRect(AV_BTN_X, AV_BTN_Y, AV_BTN_W, AV_BTN_H, 12, UI_WHITE);
  gfx->drawRoundRect(AV_BTN_X, AV_BTN_Y, AV_BTN_W, AV_BTN_H, 12, INK_K);
  gfx->setTextColor(UI_INK);
  setSize(2);
  setCur(AV_BTN_X + (AV_BTN_W - textW(T(S_AV_TERMINAR), 2)) / 2, AV_BTN_Y + 13);
  printT(T(S_AV_TERMINAR));
}
