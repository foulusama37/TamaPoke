// ================================================================ TROCAS
// A tela de troca entre duas placas. O protocolo esta em troca.h (testado
// no PC com ar ruim e reinicios); aqui so se desenha, se toca e se ligam
// os ganchos ao radio (troca_radio) e ao save (Pet::troca*).
//
// Abre pelo botao TROCAR da pagina do time. O radio so liga com esta tela
// aberta.
uint8_t trocaSlot = 0xFF;    // o meu em oferta (Pet::trocaMeu), 0xFF = nenhum
bool trocaSemRadio = false;  // o Wi-Fi nao ligou: so mostra o aviso
// A lista para escolher quem sai: aberta?, no banco?, e quanto rolou.
bool trocaEscolhe = false;
bool trocaBanco = false;
uint8_t trocaRola = 0;
// Ao MUDAR de tela (lista fecha, o outro confirmou, chegou...), os toques
// ficam surdos um instante. O painel as vezes solta e aperta de novo no
// meio de um toque: o segundo caia na tela nova, e o VOLTAR da lista fica
// exatamente em cima do TROCAR do resumo —— escolher virava confirmar.
#define TR_TRAVA_MS 350
// De que caixa saiu a oferta do banco (o slot so diz o hueco). Espiar
// outra caixa na lista abre ela; ao fechar a lista volta a esta, senao o
// preparar compararia o hueco certo da caixa errada.
uint8_t trocaCaixa = 0;
uint32_t trocaTravaAte = 0;
// A cena da evolucao ao chegar (so quando chega evoluido).
#define TR_EVO_MS 3200
uint32_t trocaFeitoEm = 0;
bool trocaEvoSom = false;

// ---- BATALHA entre placas (duelo.h). O pareamento e o da troca: tocar
// numa placa da lista pergunta TROCAR ou BATALHAR; quem convida escolhe as
// regras. Pareados, a troca passa a vez ao Duelo (TE_DUELO) e cada um
// escolhe quem leva. NADA aqui escreve no save.
Duelo duelo;
uint8_t duPasso = 0;          // na lista: 0 nada, 1 trocar ou batalhar?, 2 as regras
uint8_t duViz[6] = { 0 };     // a placa tocada
int16_t duVizDex = 0;
uint8_t duVizNv = 0;
uint8_t duFormato = DF_3V3, duItens = DI_PROIBIDO;   // a ultima escolha fica
uint8_t duOrdem[DUELO_MAX];   // os huecos do time escolhidos, na ordem
uint8_t duNEsc = 0;

static void duEnviar(void *, const uint8_t *mac, const uint8_t *b, int n) {
  radioEnvia(mac, b, n);
}

static uint8_t duNoTime() {
  uint8_t k = 0;
  for (uint8_t i = 0; i < TEAM_SLOTS; i++) if (pet.team[i].used()) k++;
  return k;
}

// O registro que vai pelo ar. O titular vive no Pet: fotoTitular() e o
// que storeActive() gravaria, mas sem gravar nem sincronizar nada.
static StoredMon duRegistro(uint8_t slot) {
  return slot == pet.mainSlot ? pet.fotoTitular() : pet.team[slot];
}

static int8_t duPosicao(uint8_t slot) {
  for (uint8_t i = 0; i < duNEsc; i++) if (duOrdem[i] == slot) return (int8_t)i;
  return -1;
}

// "3v3" e "ITENS DA BOLSA: ATE 3", para o convite e a escolha
static const char *duNomeFormato(uint8_t f) {
  return f == DF_1V1 ? "1v1" : f == DF_3V3 ? "3v3" : "6v6";
}
static StrId duNomeItens(uint8_t i) {
  return i == DI_LIVRE ? S_DU_LIVRE : i == DI_ATE3 ? S_DU_ATE3 : S_DU_NENHUM;
}

// Pareados para batalhar: o Duelo assume. Para o 6v6 o time ja vem todo
// escolhido, na ordem do time (tocar desmarca, para mudar quem abre).
static void duComeca() {
  troca.entregaDuelo();
  duelo.g.enviar = duEnviar;
  duelo.comeca(troca.eu, troca.outro, troca.sessao, troca.anfitriao, troca.modo);
  duNEsc = 0;
  if (duelo.n >= DUELO_MAX)
    for (uint8_t i = 0; i < TEAM_SLOTS && duNEsc < duelo.n; i++)
      if (pet.team[i].used()) duOrdem[duNEsc++] = i;
}

// ---- os ganchos
static void trEnviar(void *, const uint8_t *mac, const uint8_t *b, int n) {
  radioEnvia(mac, b, n);
}
static bool trPreparar(void *, TrocaDiario &d) { return pet.trocaPreparar(d); }
static bool trAplicar(void *, const TrocaDiario &d) {
  bool ok = pet.trocaAplicar(d);
  if (ok) galleryDirty = true;
  return ok;
}
static void trDescartar(void *, const TrocaDiario &d) { pet.trocaDescartar(d); }

// alguem (do time ou da caixa aberta) pode sair?
static bool trocaAlguem() {
  for (uint8_t i = 0; i < TEAM_SLOTS; i++)
    if (pet.trocaPodeOferecer(i)) return true;
  for (uint8_t i = 0; i < BANK_SLOTS; i++)
    if (pet.trocaPodeOferecer((uint8_t)(TROCA_BANCO | i))) return true;
  return false;
}

// O que a lista mostra, na ordem: o time inteiro (o titular aparece, mas
// apagado: e bom ver que ele esta la e porque nao sai) ou os da caixa.
static uint8_t trocaLista(uint8_t *out) {
  uint8_t n = 0;
  if (!trocaBanco) {
    for (uint8_t i = 0; i < TEAM_SLOTS; i++)
      if (pet.team[i].used()) out[n++] = i;
  } else {
    for (uint8_t i = 0; i < BANK_SLOTS; i++)
      if (pet.bank[i].used()) out[n++] = (uint8_t)(TROCA_BANCO | i);
  }
  return n;
}
#define TR_LISTA_MAX (BANK_SLOTS > TEAM_SLOTS ? BANK_SLOTS : TEAM_SLOTS)

void trocaAbre() {
  if (pet.isEgg() || pet.ceremony) { sfxPlay(SFX_DENY); return; }
  troca.g.ctx = nullptr;
  troca.g.enviar = trEnviar;
  troca.g.preparar = trPreparar;
  troca.g.aplicar = trAplicar;
  troca.g.descartar = trDescartar;
  troca.titular = pet.speciesId;
  troca.forma = pet.form;
  troca.aparencia = (uint8_t)((pet.shiny ? 1 : 0) | (pet.playerGirl ? 2 : 0));
  troca.nivel = (uint8_t)(pet.level() > 255 ? 255 : pet.level());
  trocaSemRadio = !radioLiga();
  uint8_t mac[6];
  radioMac(mac);
  // Um diario de antes: PREPARADO retoma (a outra placa tem de aparecer
  // para resolver); os outros so ficam lembrados, para responder a ela.
  TrocaDiario d;
  if (Pet::trocaLeDiario(d)) {
    if (d.estado == TD_PREPARADO) troca.retoma(d);
    else troca.lembra(d.sessao, d.estado);
  }
  troca.comeca(mac);
  trocaSlot = 0xFF;   // ninguem escolhido: quem escolhe e o jogador
  trocaEscolhe = false;
  trocaBanco = false;
  trocaRola = 0;
  duPasso = 0;
  duNEsc = 0;
  duelo = Duelo();
  trocaOpen = true;
  sfxPlay(SFX_TAP);
}

// ---- o combate pela rede (leva 59). Os ganchos do battle_ui apontam
// para o Duelo so enquanto o combate dura (pvpFecha os desliga).
uint8_t duResultado = 0xFF;   // 0 venci, 1 perdi, 2 empate; 0xFF = sem resultado
static void duEnviaDecisao(const DueloDecisao &d, uint32_t h) { duelo.decide(d, h); }
static void duDesistiu() { duelo.sair(); }
// Este combate comecou e ainda nao foi para o placar.
static bool duContar = false;
static void duAcabouCombate() {
  pvpAoAcabar = nullptr;
  if (!pvpCortado) duResultado = pvpVenceu == 0xFF ? 2 : pvpVenceu == pvpEu ? 0 : 1;
  duelo.acabou();
}
// Os dois times chegaram: o combate comeca. O time do ANFITRIAO e o lado 0
// nas duas placas; a semente sai da sessao (igual nas duas).
static void duJoga() {
  const uint8_t(*r0)[TROCA_MON] = duelo.anfitriao ? duelo.meu : duelo.dele;
  const uint8_t(*r1)[TROCA_MON] = duelo.anfitriao ? duelo.dele : duelo.meu;
  duResultado = 0xFF;
  pvpEnvia = duEnviaDecisao;
  pvpAoDesistir = duDesistiu;
  pvpAoAcabar = duAcabouCombate;
  duContar = true;
  if (!battleStartPvp(r0, r1, duelo.n, duelo.meuLado(), dueloSemente(duelo.sessao),
                      duelo.regras)) {
    duContar = false;
    pvpEnvia = nullptr;
    pvpAoDesistir = nullptr;
    pvpAoAcabar = nullptr;
    duelo.sair();
  }
}

void trocaFecha() {
  // no meio de uma batalha: avisa que saiu (o combate nao valia nada)
  // fechar a tela no meio do combate e desistir: vai para o placar
  if (duContar && duelo.estado == DU_BATALHA) pet.contaDuelo(1);
  duContar = false;
  if (duelo.estado == DU_TIME || duelo.estado == DU_BATALHA) duelo.sair();
  duPasso = 0;
  troca.para();   // antes de preparar avisa o outro; preparado, segue pendente
  radioDesliga();
  trocaOpen = false;
  trocaEscolhe = false;
}

// chegou evoluido? (a cena roda e o OK espera ela acabar)
static bool trocaEvoAnda() {
  if (troca.estado != TE_FEITO) return false;
  StoredMon c, v;
  memcpy(&c, troca.diario.chega, sizeof(c));
  memcpy(&v, troca.diario.veio, sizeof(v));
  return v.species != c.species;
}
static bool trocaEvoRodando() {
  return trocaEvoAnda() && millis() - trocaFeitoEm < TR_EVO_MS;
}

// do loop(): o que chegou pelo ar, e os reenvios
void trocaLoop() {
  if (!trocaOpen) return;
  uint32_t agora = millis();
  radioTick(agora);
  static uint8_t buf[256];
  for (int k = 0; k < 8; k++) {
    int n = radioRecebe(buf, sizeof(buf));
    if (n <= 0) break;
    troca.recebe(buf, n, agora);
    duelo.recebe(buf, n, agora);   // cada um so aceita os seus (magia)
  }
  troca.tick(agora);
  if (troca.podeDuelar()) duComeca();
  duelo.tick(agora);
  if (duelo.prontos() && !bt.open) duJoga();
  if (bt.open && bt.pvp) {
    // chegou a jogada da outra (ou o NADA dela): o ponto pode fechar
    if (duelo.temAsDuas() && pvpChegou(duelo.doLado((uint8_t)(duelo.meuLado() ^ 1))))
      duelo.avanca();
    // o Duelo acabou por fora: a outra saiu, sumiu, ou as duas divergiram
    if (duelo.estado == DU_FIM && !pvpAcabou) {
      const char *m = duelo.motivo == DM_ELE_SAIU ? "The other board gave up!"
                    : duelo.motivo == DM_DESSINCRONIA ? "The boards fell out of sync!"
                    : "Lost the other board...";
      pvpInterrompe(m, duelo.motivo == DM_ELE_SAIU ? duelo.meuLado() : 0xFF);
    }
  }
  // O PLACAR, uma vez por combate: o resultado, ou quem desistiu. Sumir e
  // divergir nao contam (a queda nao tira nada de ninguem).
  if (duContar && duelo.estado == DU_FIM) {
    duContar = false;
    uint8_t r = duelo.motivo == DM_FIM ? duResultado
              : duelo.motivo == DM_SAI ? 1
              : duelo.motivo == DM_ELE_SAIU ? 0 : 0xFF;
    if (r <= 2) pet.contaDuelo(r);
  }
  // a placa escolhida sumiu da lista: volta a lista
  if (duPasso && troca.estado == TE_PROCURA) {
    bool esta = false;
    for (uint8_t i = 0; i < troca.nViz; i++)
      if (!memcmp(troca.viz[i].mac, duViz, 6)) esta = true;
    if (!esta) duPasso = 0;
  }
  // Pareou (por qualquer lado): abre a lista para ESCOLHER quem sai. Antes
  // oferecia o primeiro do time sozinha, e o dono nao achava como mudar.
  static uint8_t antes = TE_FORA;
  if (troca.estado == TE_ESCOLHA && antes != TE_ESCOLHA && !troca.tenhoMeu) {
    trocaSlot = 0xFF;
    trocaEscolhe = true;
    trocaBanco = false;
    trocaRola = 0;
  }
  if (troca.estado != TE_ESCOLHA) trocaEscolhe = false;
  antes = troca.estado;
  // fora da lista, a caixa aberta e a da oferta (ver trocaCaixa)
  if (!trocaEscolhe && trocaSlot != 0xFF && (trocaSlot & TROCA_BANCO) &&
      pet.boxOpen != trocaCaixa && troca.estado <= TE_ACEITEI)
    pet.boxGo(trocaCaixa);
  // mudou a tela: trava os toques um instante
  static uint16_t chave = 0xFFFF;
  uint16_t k = (uint16_t)(troca.estado | (trocaEscolhe ? 0x100 : 0) |
                          (duPasso << 9) | (duelo.estado << 11) |
                          (duelo.tenhoMeu ? 0x2000 : 0));
  if (k != chave) {
    chave = k;
    trocaTravaAte = agora + TR_TRAVA_MS;
    if (troca.estado == TE_FEITO) {
      trocaFeitoEm = agora;
      trocaEvoSom = false;
    }
  }
  if (trocaEvoAnda() && !trocaEvoSom && agora - trocaFeitoEm >= TR_EVO_MS) {
    trocaEvoSom = true;
    sfxPlay(SFX_EVOLVE);
  }
}

// volta a procurar (depois de cancelar um convite)
static void trRecomeca() {
  uint8_t mac[6];
  memcpy(mac, troca.eu, 6);
  troca.estado = TE_PROCURA;   // cancelar() deixou CANCELADO
  troca.comeca(mac);
}

// ---- desenho
#define TR_BT_Y 316
#define TR_BT_H 44
#define TR_BT_W 128
#define TR_BT_LX 98
#define TR_BT_RX 240
#define TR_ROW_Y0 112
#define TR_ROW_DY 46

static void trBotao(int x, int y, int w, const char *t, bool ativo) {
  gfx->fillRoundRect(x, y, w, TR_BT_H, 12,
                     ativo ? C565(0x2c, 0x4a, 0x6e) : CARD_DIM);
  gfx->drawRoundRect(x, y, w, TR_BT_H, 12, ativo ? UI_INK : CARD_SOFT);
  gfx->setTextColor(ativo ? UI_WHITE : CARD_SOFT);
  int sz = textW(t, 2) <= w - 12 ? 2 : 1;
  setSize(sz);
  setCur(x + (w - textW(t, sz)) / 2, y + (TR_BT_H - 8 * sz) / 2);
  printT(t);
}

static bool trDentro(int x, int y, int bx, int by, int w) {
  return x >= bx && x <= bx + w && y >= by && y <= by + TR_BT_H;
}

// "PLACA 51D8": os dois ultimos bytes do MAC, que e o que se le na etiqueta
static void trNomePlaca(const uint8_t *mac, char *dst, int cap) {
  snprintf(dst, cap, "%s %02X%02X", T(S_TR_PLACA), mac[4], mac[5]);
}

static void trLinha(const char *s, int y, uint16_t cor, uint8_t sz = 1) {
  gfx->setTextColor(cor);
  setSize(sz);
  setCur(centerX(s, sz), y);
  printT(s);
}

// o placar das batalhas entre placas (vitorias, derrotas, empates)
static void duPlacarLinha(int y) {
  char tmp[64];
  snprintf(tmp, sizeof tmp, T(S_DU_PLACAR), (unsigned)pet.duV, (unsigned)pet.duD,
           (unsigned)pet.duE);
  trLinha(tmp, y, CARD_SOFT);
}

static void trMiniGrande(int16_t dex, int cy, int caja = 64) {
  const uint8_t *th = thumbs.get(dex);
  if (th) drawThumbFit(th, CX, cy, caja, false);
}
// leva 69: na chegada (e na cena da evolucao) o icone vai a 3x
#define TR_EVO_CAIXA 120
#define TR_EVO_CY 140

// A lista de escolha. Leva 69: era uma lista de linhas de 38 com o icone
// a 28 px ("os icones ficam bem pequenos"); agora e uma GRADE 3x2 com o
// icone a 2x (80 px), seis por pagina tambem no banco; as setas viram a
// pagina. Nada nesta tela fecha por um toque errado: so o VOLTAR sai dela.
#define TRL_TAB_Y 62
// no banco a barra da caixa fica entre as abas e a grade
#define TRL_Y0 (trocaBanco ? 130 : 102)
#define TRL_CX_Y 98
#define TRL_FILAS 6          // celulas por pagina
#define TRL_BT_Y 352
// a celula (troca e duelo): 112x106, passo 120x110, centrada em CX
#define TRG_X0 57
#define TRG_W 112
#define TRG_H 106
#define TRG_DX 120
#define TRG_DY 110
#define TRL_UP_X 70
#define TRL_DN_X 326
#define TRL_SETA_W 70
#define TRL_VOLTA_X 158
#define TRL_VOLTA_W 150

static int trgX(uint8_t k) { return TRG_X0 + (k % 3) * TRG_DX; }
static int trgY(uint8_t k, int y0) { return y0 + (k / 3) * TRG_DY; }

// Qual celula o dedo pegou: a faixa inteira, sem vaos (o vao de 8 entre
// as celulas vai para a da esquerda / a de cima). -1 = fora da grade.
static int trgToque(int x, int y, int y0) {
  if (x < TRG_X0 - 6 || x > TRG_X0 + 2 * TRG_DX + TRG_W + 6) return -1;
  if (y < y0 - 2 || y >= y0 - 2 + 2 * TRG_DY) return -1;
  int c = (x - (TRG_X0 - 6)) / TRG_DX;
  if (c > 2) c = 2;
  int r = (y - (y0 - 2)) / TRG_DY;
  return r * 3 + c;
}

// Uma celula da grade: o icone a 2x, o nome e o nivel embaixo, e o que
// leva (verde) ao lado do nivel. Os sinais da linha antiga continuam:
// estrela do shiny (canto direito), cadeado do banco trancado (canto
// esquerdo, e a celula apaga).
static void drawMonCelula(const StoredMon &m, int x, int y, bool activo,
                          bool trancado = false, uint8_t objeto = 0xFF) {
  if (activo) gfx->fillRoundRect(x, y, TRG_W, TRG_H, 10, CARD_DIM);
  gfx->drawRoundRect(x, y, TRG_W, TRG_H, 10, activo ? CARD_INK : CARD_DIM);
  if (!m.used()) return;
  const uint8_t *th = thumbs.get(m.species);
  if (th) drawThumbFit(th, x + TRG_W / 2, y + 42, 80, false);
  if (m.flags & SM_SHINY) drawShinyStar(ART_SHINY_S, x + TRG_W - 14, y + 14);
  const char *nm = dexName(m.species);
  gfx->setTextColor(CARD_INK);
  setSize(1);
  setCur(x + (TRG_W - (int)textW(nm, 1)) / 2, y + 84);
  printT(nm);
  char lv[10], it[24] = "";
  snprintf(lv, sizeof lv, "Lv%u", Pet::levelOf(m));
  uint8_t leva = objeto == 0xFF ? m.item : objeto;
  if (leva && leva < ITEM_COUNT) {
    int cabe = (TRG_W - 8) / LATIN_ADV - (int)strlen(lv) - 1;
    if (cabe > 0) snprintf(it, sizeof it, "%.*s", cabe, ITEM_TBL[leva].name);
  }
  int w = (int)textW(lv, 1) + (it[0] ? LATIN_ADV + (int)textW(it, 1) : 0);
  int lx = x + (TRG_W - w) / 2;
  gfx->setTextColor(CARD_SOFT);
  setCur(lx, y + 95);
  printT(lv);
  if (it[0]) {
    gfx->setTextColor(UI_BAR_OK);
    setCur(lx + (int)textW(lv, 1) + LATIN_ADV, y + 95);
    printT(it);
  }
  if (trancado) {
    int cx = x + 16, cy = y + 14;
    uint16_t cor = C565(0x8a, 0x93, 0xb0);
    gfx->fillRoundRect(cx - 8, cy, 16, 13, 3, cor);
    gfx->drawFastVLine(cx - 5, cy - 6, 6, cor);
    gfx->drawFastVLine(cx - 4, cy - 6, 6, cor);
    gfx->drawFastVLine(cx + 4, cy - 6, 6, cor);
    gfx->drawFastVLine(cx + 5, cy - 6, 6, cor);
    gfx->drawFastHLine(cx - 5, cy - 7, 11, cor);
    gfx->drawFastHLine(cx - 5, cy - 6, 11, cor);
    for (int yy = y + 2; yy < y + TRG_H - 1; yy += 3)
      gfx->drawFastHLine(x + 2, yy, TRG_W - 4, CARD_DIM);
  }
}

static void trSeta(int x, bool cima, bool ativa) {
  gfx->fillRoundRect(x, TRL_BT_Y, TRL_SETA_W, TR_BT_H, 12, ativa ? C565(0x2c, 0x4a, 0x6e) : CARD_DIM);
  gfx->drawRoundRect(x, TRL_BT_Y, TRL_SETA_W, TR_BT_H, 12, ativa ? UI_INK : CARD_SOFT);
  int cx = x + TRL_SETA_W / 2, cy = TRL_BT_Y + TR_BT_H / 2;
  uint16_t c = ativa ? UI_WHITE : CARD_SOFT;
  if (cima) gfx->fillTriangle(cx - 10, cy + 6, cx + 10, cy + 6, cx, cy - 7, c);
  else      gfx->fillTriangle(cx - 10, cy - 6, cx + 10, cy - 6, cx, cy + 7, c);
}

static void renderTrocaLista() {
  trLinha(T(S_TR_DA), 30, CARD_INK, 2);
  for (uint8_t b = 0; b < 2; b++) {
    int x = 116 + b * 118;
    bool act = (b == (trocaBanco ? 1 : 0));
    const char *t = T(b ? S_CARD_BANK : S_CARD_TEAM);
    gfx->fillRoundRect(x, TRL_TAB_Y, 112, 30, 8, act ? CARD_INK : CARD_PAPER);
    gfx->drawRoundRect(x, TRL_TAB_Y, 112, 30, 8, CARD_INK);
    gfx->setTextColor(act ? CARD_PAPER : CARD_INK);
    setSize(1);
    setCur(x + 56 - (int)strlen(t) * 3, TRL_TAB_Y + 11);
    printT(t);
  }
  uint8_t l[TR_LISTA_MAX];
  uint8_t n = trocaLista(l);
  if (trocaBanco) {
    // a barra da caixa, como a do cartao: "BOX 3/32  12/30" e as setas
    char bar[32];
    snprintf(bar, sizeof(bar), "BOX %u/%u   %u/%u", (unsigned)(pet.boxOpen + 1),
             (unsigned)BOX_COUNT, (unsigned)n, (unsigned)BANK_SLOTS);
    gfx->setTextColor(CARD_INK);
    setSize(2);
    setCur(centerX(bar, 2), TRL_CX_Y + 6);
    printT(bar);
    int cy = TRL_CX_Y + 13;
    gfx->fillTriangle(70, cy, 86, cy - 10, 86, cy + 10, CARD_INK);
    gfx->fillTriangle(370, cy, 354, cy - 10, 354, cy + 10, CARD_INK);
  }
  if (trocaRola + TRL_FILAS > n) trocaRola = n > TRL_FILAS ? (uint8_t)(n - TRL_FILAS) : 0;
  if (!n) trLinha("- empty -", 200, CARD_SOFT);
  for (uint8_t k = 0; k < TRL_FILAS && trocaRola + k < n; k++) {
    uint8_t slot = l[trocaRola + k];
    const StoredMon *m = pet.trocaMeu(slot);
    int x = trgX(k), y = trgY(k, TRL_Y0);
    drawMonCelula(*m, x, y, slot == trocaSlot, (slot & TROCA_BANCO) && pet.bankLocked(*m));
    // o titular: o circulo cheio da ficha, no canto da celula
    if (!(slot & TROCA_BANCO) && slot == pet.mainSlot)
      gfx->fillCircle(x + 14, y + 14, 7, UI_BAR_WARN);
  }
  bool rola = n > TRL_FILAS;
  trSeta(TRL_UP_X, true, rola && trocaRola > 0);
  trSeta(TRL_DN_X, false, rola && trocaRola + TRL_FILAS < n);
  trBotao(TRL_VOLTA_X, TRL_BT_Y, TRL_VOLTA_W, T(S_TR_VOLTAR), true);
}

// ---- as telas da batalha
#define DU_OP_W 96
#define DU_OP_X0 81
#define DU_OP_DX 104
#define DU_FMT_Y 114
#define DU_ITM_Y 190
#define DU_CH_Y 260         // TROCAR / BATALHAR
#define DU_VOLTA_Y 340
#define DUE_Y0 96           // a grade do time na escolha (no alto o vidro redondo corta)
#define DUE_BT_Y 346

// um botao de opcao: escolhido = cheio; sem poder = apagado
static void duOpcao(int x, int y, const char *t, bool escolhido, bool pode) {
  uint16_t fundo = !pode ? CARD_DIM : escolhido ? CARD_INK : CARD_PAPER;
  gfx->fillRoundRect(x, y, DU_OP_W, TR_BT_H, 10, fundo);
  gfx->drawRoundRect(x, y, DU_OP_W, TR_BT_H, 10, pode ? CARD_INK : CARD_SOFT);
  gfx->setTextColor(!pode ? CARD_SOFT : escolhido ? CARD_PAPER : CARD_INK);
  int sz = textW(t, 2) <= DU_OP_W - 10 ? 2 : 1;
  setSize(sz);
  setCur(x + (DU_OP_W - textW(t, sz)) / 2, y + (TR_BT_H - 8 * sz) / 2);
  printT(t);
}

static void duLinhaRegras(uint8_t regras, int y) {
  char tmp[72];
  snprintf(tmp, sizeof tmp, "%s  -  %s: %s", duNomeFormato(DR_FORMATO(regras)),
           T(S_DU_ITENS), T(duNomeItens(DR_ITENS(regras))));
  trLinha(tmp, y, CARD_INK);
}

// uma fila de miniaturas (o time de um lado), centrada
static void duFila(const uint8_t regs[][TROCA_MON], uint8_t n, int cy) {
  int passo = n > 3 ? 62 : 80;
  int x0 = CX - passo * (n - 1) / 2;
  for (uint8_t i = 0; i < n; i++) {
    StoredMon m;
    memcpy(&m, regs[i], sizeof(m));
    const uint8_t *th = thumbs.get(m.species);
    if (th) drawThumbFit(th, x0 + i * passo, cy, 52, false);
  }
}

static void renderDuConvite() {
  char tmp[64];
  if (duVizDex > 0) trMiniGrande(duVizDex, 120);
  trNomePlaca(duViz, tmp, sizeof tmp);
  trLinha(tmp, 168, CARD_INK, 2);
  if (duVizDex > 0) {
    snprintf(tmp, sizeof tmp, "%s  Lv%u", dexName(duVizDex), duVizNv);
    trLinha(tmp, 194, CARD_SOFT);
  }
  trLinha(T(S_DU_OQUE), 226, CARD_INK);
  trBotao(TR_BT_LX, DU_CH_Y, TR_BT_W, T(S_TROCA), trocaAlguem());
  trBotao(TR_BT_RX, DU_CH_Y, TR_BT_W, T(S_DU_BATALHAR), duNoTime() > 0);
  duPlacarLinha(DU_CH_Y + TR_BT_H + 12);
  trBotao(CX - 64, DU_VOLTA_Y, 128, T(S_TR_VOLTAR), true);
}

static void renderDuRegras() {
  char tmp[64];
  trLinha(T(S_DU_BATALHAR), 34, CARD_INK, 3);
  trNomePlaca(duViz, tmp, sizeof tmp);
  trLinha(tmp, 72, CARD_SOFT);
  trLinha(T(S_DU_FORMATO), DU_FMT_Y - 14, CARD_SOFT);
  uint8_t tem = duNoTime();
  for (uint8_t f = 0; f < 3; f++)
    duOpcao(DU_OP_X0 + f * DU_OP_DX, DU_FMT_Y, duNomeFormato(f), f == duFormato,
            duTamanho(duRegras(f, 0)) <= tem);
  trLinha(T(S_DU_ITENS), DU_ITM_Y - 14, CARD_SOFT);
  static const uint8_t ORD[3] = { DI_PROIBIDO, DI_ATE3, DI_LIVRE };
  for (uint8_t k = 0; k < 3; k++)
    duOpcao(DU_OP_X0 + k * DU_OP_DX, DU_ITM_Y, T(duNomeItens(ORD[k])),
            ORD[k] == duItens, true);
  trLinha(T(S_DU_NIVEL), 254, CARD_SOFT);
  trBotao(TR_BT_LX, TR_BT_Y, TR_BT_W, T(S_DU_CONVIDAR), true);
  trBotao(TR_BT_RX, TR_BT_Y, TR_BT_W, T(S_TR_VOLTAR), true);
}

static void renderDuelo() {
  char tmp[64];
  uint8_t n = duelo.n;
  switch (duelo.estado) {
    case DU_TIME:
      if (!duelo.tenhoMeu) {
        snprintf(tmp, sizeof tmp, T(S_DU_ESCOLHA), (unsigned)n);
        trLinha(tmp, 40, CARD_INK, 2);
        duLinhaRegras(duelo.regras, 66);
        if (duNoTime() < n) {
          snprintf(tmp, sizeof tmp, T(S_DU_FALTA), (unsigned)n);
          trLinha(tmp, 190, UI_BAR_WARN);
        } else {
          uint8_t k = 0;
          for (uint8_t i = 0; i < TEAM_SLOTS; i++) {
            if (!pet.team[i].used()) continue;
            int x = trgX(k), y = trgY(k, DUE_Y0);
            int8_t pos = duPosicao(i);
            StoredMon m = duRegistro(i);
            drawMonCelula(m, x, y, pos >= 0, false);
            if (pos >= 0) {
              // a ordem em que vao sair: o 1 abre o combate
              // (print direto: o printT recorta ao vidro e empurrava o
              // numero para fora da bolinha)
              gfx->fillCircle(x + 16, y + 16, 11, CARD_INK);
              gfx->setTextColor(CARD_PAPER);
              setSize(2);
              gfx->setCursor(x + 11, y + 9);
              gfx->print((char)('1' + pos));
            }
            k++;
          }
        }
        trBotao(TR_BT_LX, DUE_BT_Y, TR_BT_W, T(S_DU_PRONTO), duNEsc == n);
        trBotao(TR_BT_RX, DUE_BT_Y, TR_BT_W, T(S_TR_CANCELAR), true);
      } else {
        trLinha(T(S_DU_BATALHAR), 34, CARD_INK, 3);
        duLinhaRegras(duelo.regras, 76);
        duFila(duelo.meu, n, 150);
        trLinha(T(S_DU_ESPERA), 210, CARD_SOFT);
        int p = (int)((millis() / 400) % 4);
        snprintf(tmp, sizeof tmp, "%.*s", p, "...");
        trLinha(tmp, 232, CARD_INK, 2);
        trBotao(CX - 64, DUE_BT_Y, 128, T(S_TR_CANCELAR), true);
      }
      break;
    case DU_BATALHA:
      // Os dois times, frente a frente: aparece so ate o combate abrir (ou
      // se ele nao abriu).
      duLinhaRegras(duelo.regras, 40);
      duFila(duelo.dele, n, 110);
      trLinha("VS", 160, CARD_INK, 3);
      duFila(duelo.meu, n, 230);
      trBotao(CX - 64, DUE_BT_Y, 128, T(S_TR_FECHAR), true);
      break;
    case DU_FIM: {
      if (duelo.motivo == DM_FIM && duResultado <= 2) {
        StrId r = duResultado == 0 ? S_DU_VENCEU : duResultado == 1 ? S_DU_PERDEU : S_DU_EMPATE;
        trLinha(T(S_DU_BATALHAR), 34, CARD_INK, 3);
        trLinha(T(r), 190, duResultado == 0 ? UI_BAR_OK : CARD_INK, 3);
        trLinha(T(S_DU_NIVEL), 250, CARD_SOFT);
        duPlacarLinha(280);
        trBotao(CX - 64, DUE_BT_Y, 128, "OK", true);
        break;
      }
      StrId mot = duelo.motivo == DM_SAI ? S_DU_DESISTI
                : duelo.motivo == DM_ELE_SAIU ? S_TR_DESISTIU
                : duelo.motivo == DM_DESSINCRONIA ? S_DU_DESSINC : S_TR_SUMIU;
      // o mesmo jeito da tela do resultado: titulo e a frase grande
      trLinha(T(S_DU_BATALHAR), 34, CARD_INK, 3);
      trLinha(T(mot), 196, UI_BAR_WARN, 2);
      trLinha(T(S_DU_NIVEL), 250, CARD_SOFT);
      duPlacarLinha(280);
      trBotao(CX - 64, DUE_BT_Y, 128, "OK", true);
      break;
    }
    default:
      break;
  }
}

void renderTroca() {
  gfx->fillScreen(CARD_PAPER);
  if (!trocaSemRadio && troca.estado == TE_DUELO) {
    renderDuelo();
    gfx->flush();
    return;
  }
  if (!trocaSemRadio && troca.estado == TE_PROCURA && duPasso) {
    if (duPasso == 1) renderDuConvite();
    else renderDuRegras();
    gfx->flush();
    return;
  }
  if (!trocaSemRadio && troca.estado == TE_ESCOLHA && trocaEscolhe) {
    renderTrocaLista();
    gfx->flush();
    return;
  }
  // O titulo: CONECTAR enquanto procura (dali sai troca OU batalha); o
  // convite diz o que e; dai em diante, so a troca passa por aqui.
  StrId titulo = S_TROCA;
  if (trocaSemRadio || troca.estado == TE_PROCURA) titulo = S_CONECTAR;
  else if ((troca.estado == TE_CONVIDEI || troca.estado == TE_CONVIDADO) && troca.modo)
    titulo = S_DU_BATALHAR;
  trLinha(T(titulo), 34, CARD_INK, 3);
  char tmp[64];
  if (trocaSemRadio) {
    trLinha(T(S_TR_SEMRADIO), 200, UI_BAR_BAD);
    trBotao(CX - 64, TR_BT_Y, 128, T(S_TR_FECHAR), true);
    gfx->flush();
    return;
  }
  switch (troca.estado) {
    case TE_PROCURA: {
      trLinha(T(S_TR_PROCURA), 76, CARD_SOFT);
      if (troca.nViz == 0) {
        // os pontinhos andam: e o sinal de que a tela nao travou
        int n = (int)((millis() / 400) % 4);
        snprintf(tmp, sizeof tmp, "%.*s", n, "...");
        trLinha(tmp, 200, CARD_INK, 3);
      }
      for (uint8_t i = 0; i < troca.nViz && i < 4; i++) {
        const TrocaVizinho &v = troca.viz[i];
        int y = TR_ROW_Y0 + i * TR_ROW_DY;
        gfx->drawRoundRect(58, y, 324, 40, 8, CARD_INK);
        const uint8_t *th = thumbs.get(v.titular);
        if (th) drawThumbFit(th, 80, y + 20, 30, false);
        gfx->setTextColor(CARD_INK);
        setSize(2);
        trNomePlaca(v.mac, tmp, sizeof tmp);
        setCur(104, y + 4);
        printT(tmp);
        setSize(1);
        gfx->setTextColor(CARD_SOFT);
        snprintf(tmp, sizeof tmp, "%s  Lv%u", dexName(v.titular), v.nivel);
        setCur(104, y + 26);
        printT(tmp);
      }
      if (!trocaAlguem())
        trLinha(T(S_TR_NINGUEM), 296, UI_BAR_WARN);
      trBotao(CX - 64, TR_BT_Y + 24, 128, T(S_TR_FECHAR), true);
      break;
    }
    case TE_CONVIDEI:
      trNomePlaca(troca.outro, tmp, sizeof tmp);
      trLinha(tmp, 150, CARD_INK, 2);
      trLinha(T(S_TR_ESPERA), 190, CARD_SOFT);
      if (troca.modo) duLinhaRegras(troca.modo, 220);
      trBotao(CX - 64, TR_BT_Y, 128, T(S_TR_CANCELAR), true);
      break;
    case TE_CONVIDADO: {
      int16_t dex = 0;
      for (uint8_t i = 0; i < troca.nViz; i++)
        if (!memcmp(troca.viz[i].mac, troca.outro, 6)) dex = troca.viz[i].titular;
      if (dex > 0) trMiniGrande(dex, 150);
      trNomePlaca(troca.outro, tmp, sizeof tmp);
      trLinha(tmp, 210, CARD_INK, 2);
      bool pode = true;
      if (troca.modo) {
        trLinha(T(S_DU_QUER), 240, CARD_SOFT);
        duLinhaRegras(troca.modo, 262);
        uint8_t n = duTamanho(troca.modo);
        if (duNoTime() < n) {
          snprintf(tmp, sizeof tmp, T(S_DU_FALTA), (unsigned)n);
          trLinha(tmp, 284, UI_BAR_WARN);
          pode = false;
        } else {
          trLinha(T(S_DU_NIVEL), 284, CARD_SOFT);
        }
      } else {
        trLinha(T(S_TR_QUER), 240, CARD_SOFT);
      }
      trBotao(TR_BT_LX, TR_BT_Y, TR_BT_W, T(S_YES), pode);
      trBotao(TR_BT_RX, TR_BT_Y, TR_BT_W, T(S_NO), true);
      break;
    }
    case TE_ESCOLHA:
    case TE_ACEITEI: {
      trLinha(T(S_TR_DA), 82, CARD_SOFT);
      if (troca.tenhoMeu && trocaSlot != 0xFF) {
        drawMonRow(*pet.trocaMeu(trocaSlot), 96, true);
        if (troca.estado == TE_ESCOLHA) trLinha(T(S_TR_OUTRO), 134, CARD_SOFT);
      } else if (troca.estado == TE_ESCOLHA) {
        trBotao(CX - 100, 92, 200, T(S_TR_ESCOLHER), true);
      }
      trLinha(T(S_TR_RECEBE), 150, CARD_SOFT);
      if (troca.tenhoDele) {
        StoredMon d;
        memcpy(&d, troca.dele, sizeof(d));
        drawMonRow(d, 164, false);
        // o que vai acontecer quando chegar: evolui? vai para o banco?
        StoredMon c = Pet::trocaChegada(d);
        if (c.species != d.species) {
          snprintf(tmp, sizeof tmp, "%s %s!", T(S_TR_EVOLUI), dexName(c.species));
          trLinha(tmp, 212, UI_BAR_OK);
        }
        if (Pet::levelOf(c) > pet.bankCapHere()) trLinha(T(S_TR_BANCO), 230, UI_BAR_WARN);
      } else {
        trLinha(T(S_TR_ESCOLHENDO), 178, CARD_SOFT);
      }
      if (troca.estado == TE_ACEITEI) {
        trLinha(T(S_TR_ESPERA2), 268, CARD_INK);
        trBotao(CX - 64, TR_BT_Y, 128, T(S_TR_CANCELAR), true);
      } else {
        if (troca.eleAceitou) trLinha(T(S_TR_CONFIRMOU), 268, UI_BAR_OK);
        trBotao(TR_BT_LX, TR_BT_Y, TR_BT_W, T(S_TROCA), troca.podeAceitar());
        trBotao(TR_BT_RX, TR_BT_Y, TR_BT_W, T(S_TR_CANCELAR), true);
      }
      break;
    }
    case TE_PREPARADO:
      trLinha(T(S_TR_TROCANDO), 190, CARD_INK);
      trLinha(T(S_TR_PERTO), 212, CARD_SOFT);
      trBotao(CX - 64, TR_BT_Y + 24, 128, T(S_TR_FECHAR), true);
      break;
    case TE_FEITO: {
      StoredMon c, v;
      memcpy(&c, troca.diario.chega, sizeof(c));
      memcpy(&v, troca.diario.veio, sizeof(v));
      if (trocaEvoRodando()) {
        // Como nos jogos: o de antes e o de depois se alternam, cada vez
        // mais rapido (t^2), e no fim fica o novo (com o som).
        uint32_t t = millis() - trocaFeitoEm;
        bool novo = ((t * t) / 180000u) & 1u;
        if (t > TR_EVO_MS - 250) {
          gfx->fillCircle(CX, TR_EVO_CY, 64 + (int)(TR_EVO_MS - t) / 10, UI_WHITE);
          novo = true;
        }
        trMiniGrande(novo ? c.species : v.species, TR_EVO_CY, TR_EVO_CAIXA);
        snprintf(tmp, sizeof tmp, T(S_TR_EVOLUINDO), dexName(v.species));
        trLinha(tmp, 210, CARD_INK, 2);
        break;   // sem OK: a cena nao se pula sem querer
      }
      trMiniGrande(c.species, TR_EVO_CY, TR_EVO_CAIXA);
      snprintf(tmp, sizeof tmp, "%s %s", dexName(c.species), T(S_TR_CHEGOU));
      trLinha(tmp, 210, CARD_INK, 2);
      if (v.species != c.species) {
        snprintf(tmp, sizeof tmp, "%s %s %s!", dexName(v.species), T(S_TR_EVOLUI),
                 dexName(c.species));
        trLinha(tmp, 240, UI_BAR_OK);
      }
      trBotao(CX - 64, TR_BT_Y, 128, "OK", true);
      break;
    }
    case TE_CANCELADO: {
      static const StrId MOT[] = { S_TR_DESISTIU, S_TR_DESISTI, S_TR_SD,
                                   S_TR_SUMIU, S_TR_DESISTIU };
      trLinha(T(MOT[troca.motivo < 5 ? troca.motivo : 0]), 200, UI_BAR_WARN);
      trBotao(CX - 64, TR_BT_Y, 128, "OK", true);
      break;
    }
    default:
      break;
  }
  gfx->flush();
}

// ---- toques
// toque na lista de escolha
static void trocaListaTap(int x, int y) {
  // as abas (a faixa de toque e maior que o desenho)
  if (y >= TRL_TAB_Y - 8 && y <= (trocaBanco ? TRL_CX_Y - 2 : TRL_TAB_Y + 38) &&
      x >= 104 && x <= 362) {
    bool b = x >= 233;
    if (b != trocaBanco) { trocaBanco = b; trocaRola = 0; }
    sfxPlay(SFX_TAP);
    return;
  }
  // as setas da caixa: a proxima (ou anterior) que tem alguem. Para
  // trocar, caixa vazia nao interessa, e sao 32.
  if (trocaBanco && y >= TRL_CX_Y - 1 && y < TRL_Y0 - 2 && (x <= 170 || x >= 270)) {
    int passo = x <= 170 ? -1 : 1;
    for (int k = 1; k < BOX_COUNT; k++) {
      uint8_t c = (uint8_t)((pet.boxOpen + passo * k + BOX_COUNT * 2) % BOX_COUNT);
      if (!pet.boxCount(c)) continue;
      if (pet.boxGo(c)) { trocaRola = 0; sfxPlay(SFX_TAP); }
      else sfxPlay(SFX_DENY);
      return;
    }
    sfxPlay(SFX_DENY);   // nenhuma outra caixa tem alguem
    return;
  }
  uint8_t l[TR_LISTA_MAX];
  uint8_t n = trocaLista(l);
  // as celulas: a faixa inteira, sem vaos (aqui nada fecha)
  int cel = trgToque(x, y, TRL_Y0);
  if (cel >= 0) {
    uint8_t k = (uint8_t)cel;
    if (trocaRola + k >= n) return;
    uint8_t slot = l[trocaRola + k];
    if (!pet.trocaPodeOferecer(slot)) { sfxPlay(SFX_DENY); return; }
    trocaSlot = slot;
    trocaCaixa = pet.boxOpen;
    troca.oferecer(slot, (const uint8_t *)pet.trocaMeu(slot));
    trocaEscolhe = false;
    sfxPlay(SFX_TAP);
    return;
  }
  if (y >= TRL_BT_Y - 8 && y <= TRL_BT_Y + TR_BT_H + 14) {
    if (x >= TRL_UP_X - 10 && x <= TRL_UP_X + TRL_SETA_W + 8) {
      if (trocaRola > 0) { trocaRola = (uint8_t)(trocaRola > TRL_FILAS ? trocaRola - TRL_FILAS : 0); sfxPlay(SFX_TAP); }
      else sfxPlay(SFX_DENY);
    } else if (x >= TRL_DN_X - 8 && x <= TRL_DN_X + TRL_SETA_W + 10) {
      if (trocaRola + TRL_FILAS < n) { trocaRola = (uint8_t)(trocaRola + TRL_FILAS); sfxPlay(SFX_TAP); }
      else sfxPlay(SFX_DENY);
    } else if (x >= TRL_VOLTA_X - 9 && x <= TRL_VOLTA_X + TRL_VOLTA_W + 9) {
      trocaEscolhe = false;
      sfxPlay(SFX_TAP);
    }
  }
}

static uint32_t duSessaoNova() {
  return ((uint32_t)random(0x7FFFFFFF) << 1) ^ millis();
}

// a tela "trocar ou batalhar?"
static void duConviteTap(int x, int y) {
  if (trDentro(x, y, TR_BT_LX, DU_CH_Y, TR_BT_W)) {
    if (!trocaAlguem()) { sfxPlay(SFX_DENY); return; }
    duPasso = 0;
    troca.convidar(duViz, duSessaoNova());
    sfxPlay(SFX_TAP);
  } else if (trDentro(x, y, TR_BT_RX, DU_CH_Y, TR_BT_W)) {
    uint8_t tem = duNoTime();
    if (!tem) { sfxPlay(SFX_DENY); return; }
    // o formato lembrado pode ser grande demais para o time de agora
    while (duFormato > DF_1V1 && duTamanho(duRegras(duFormato, 0)) > tem) duFormato--;
    duPasso = 2;
    sfxPlay(SFX_TAP);
  } else if (trDentro(x, y, CX - 64, DU_VOLTA_Y, 128)) {
    duPasso = 0;
    sfxPlay(SFX_TAP);
  }
}

// as regras: as faixas de toque das opcoes vao ate meio caminho da vizinha
static void duRegrasTap(int x, int y) {
  int col = -1;
  if (x >= DU_OP_X0 - 12 && x < DU_OP_X0 + 3 * DU_OP_DX + 4)
    col = (x - (DU_OP_X0 - 12)) / DU_OP_DX;
  if (col > 2) col = 2;
  if (col >= 0 && y >= DU_FMT_Y - 18 && y <= DU_FMT_Y + TR_BT_H + 8) {
    if (duTamanho(duRegras((uint8_t)col, 0)) > duNoTime()) { sfxPlay(SFX_DENY); return; }
    duFormato = (uint8_t)col;
    sfxPlay(SFX_TAP);
    return;
  }
  if (col >= 0 && y >= DU_ITM_Y - 18 && y <= DU_ITM_Y + TR_BT_H + 8) {
    static const uint8_t ORD[3] = { DI_PROIBIDO, DI_ATE3, DI_LIVRE };
    duItens = ORD[col];
    sfxPlay(SFX_TAP);
    return;
  }
  if (trDentro(x, y, TR_BT_LX, TR_BT_Y, TR_BT_W)) {
    duPasso = 0;
    troca.convidar(duViz, duSessaoNova(), duRegras(duFormato, duItens));
    sfxPlay(SFX_TAP);
  } else if (trDentro(x, y, TR_BT_RX, TR_BT_Y, TR_BT_W)) {
    duPasso = 1;
    sfxPlay(SFX_TAP);
  }
}

// a batalha pareada: escolher o time, esperar, o fim
static void dueloTap(int x, int y) {
  switch (duelo.estado) {
    case DU_TIME:
      if (!duelo.tenhoMeu) {
        // as celulas: a faixa inteira, como na lista da troca
        int cel = trgToque(x, y, DUE_Y0);
        if (duNoTime() >= duelo.n && cel >= 0) {
          uint8_t fila = (uint8_t)cel, k = 0;
          for (uint8_t i = 0; i < TEAM_SLOTS; i++) {
            if (!pet.team[i].used()) continue;
            if (k++ != fila) continue;
            int8_t pos = duPosicao(i);
            if (pos >= 0) {
              // desmarca: os de depois sobem um
              for (uint8_t j = (uint8_t)pos; j + 1 < duNEsc; j++) duOrdem[j] = duOrdem[j + 1];
              duNEsc--;
              sfxPlay(SFX_TAP);
            } else if (duNEsc < duelo.n) {
              duOrdem[duNEsc++] = i;
              sfxPlay(SFX_TAP);
            } else {
              sfxPlay(SFX_DENY);
            }
            return;
          }
          return;
        }
        if (trDentro(x, y, TR_BT_LX, DUE_BT_Y, TR_BT_W)) {
          if (duNEsc != duelo.n) { sfxPlay(SFX_DENY); return; }
          uint8_t regs[DUELO_MAX][TROCA_MON];
          for (uint8_t i = 0; i < duNEsc; i++) {
            StoredMon m = duRegistro(duOrdem[i]);
            memcpy(regs[i], &m, TROCA_MON);
          }
          duelo.meuTime(regs, duNEsc);
          sfxPlay(SFX_TAP);
        } else if (trDentro(x, y, TR_BT_RX, DUE_BT_Y, TR_BT_W)) {
          duelo.sair();
          trRecomeca();
        }
      } else if (trDentro(x, y, CX - 64, DUE_BT_Y, 128)) {
        duelo.sair();
        trRecomeca();
      }
      break;
    case DU_BATALHA:
      if (trDentro(x, y, CX - 64, DUE_BT_Y, 128)) {
        duelo.sair();
        trRecomeca();
      }
      break;
    case DU_FIM:
      if (trDentro(x, y, CX - 64, DUE_BT_Y, 128)) trRecomeca();
      break;
    default:
      break;
  }
}

void trocaTap(int x, int y) {
  // a tela acabou de mudar: este toque era do dedo de antes
  if ((int32_t)(millis() - trocaTravaAte) < 0) return;
  if (trocaEvoRodando()) return;
  if (!trocaSemRadio && troca.estado == TE_DUELO) {
    dueloTap(x, y);
    return;
  }
  if (!trocaSemRadio && troca.estado == TE_PROCURA && duPasso) {
    if (duPasso == 1) duConviteTap(x, y);
    else duRegrasTap(x, y);
    return;
  }
  if (!trocaSemRadio && troca.estado == TE_ESCOLHA && trocaEscolhe) {
    trocaListaTap(x, y);
    return;
  }
  if (trocaSemRadio) {
    if (trDentro(x, y, CX - 64, TR_BT_Y, 128)) trocaFecha();
    return;
  }
  switch (troca.estado) {
    case TE_PROCURA:
      for (uint8_t i = 0; i < troca.nViz && i < 4; i++) {
        int yy = TR_ROW_Y0 + i * TR_ROW_DY;
        if (y >= yy && y <= yy + 40 && x >= 58 && x <= 382) {
          // trocar ou batalhar? (antes convidava para trocar direto)
          memcpy(duViz, troca.viz[i].mac, 6);
          duVizDex = troca.viz[i].titular;
          duVizNv = troca.viz[i].nivel;
          duPasso = 1;
          sfxPlay(SFX_TAP);
          return;
        }
      }
      if (trDentro(x, y, CX - 64, TR_BT_Y + 24, 128)) trocaFecha();
      break;
    case TE_CONVIDEI:
      if (trDentro(x, y, CX - 64, TR_BT_Y, 128)) {
        troca.cancelar();
        trRecomeca();
      }
      break;
    case TE_CONVIDADO:
      if (trDentro(x, y, TR_BT_LX, TR_BT_Y, TR_BT_W)) {
        if (troca.modo && duNoTime() < duTamanho(troca.modo)) { sfxPlay(SFX_DENY); return; }
        troca.aceitarConvite();
        sfxPlay(SFX_TAP);
      } else if (trDentro(x, y, TR_BT_RX, TR_BT_Y, TR_BT_W)) {
        troca.cancelar();
        trRecomeca();
      }
      break;
    case TE_ESCOLHA:
      // tocar no meu (ou no ESCOLHER): abre a lista
      if (y >= 84 && y <= 144 && x >= 48 && x <= 392) {
        trocaEscolhe = true;
        trocaBanco = trocaSlot != 0xFF && (trocaSlot & TROCA_BANCO);
        trocaRola = 0;
        sfxPlay(SFX_TAP);
        return;
      }
      if (trDentro(x, y, TR_BT_LX, TR_BT_Y, TR_BT_W)) {
        if (troca.podeAceitar()) { troca.aceitar(); sfxPlay(SFX_TAP); }
        else sfxPlay(SFX_DENY);
      } else if (trDentro(x, y, TR_BT_RX, TR_BT_Y, TR_BT_W)) {
        troca.cancelar();
      }
      break;
    case TE_ACEITEI:
      if (trDentro(x, y, CX - 64, TR_BT_Y, 128)) troca.cancelar();
      break;
    case TE_PREPARADO:
      if (trDentro(x, y, CX - 64, TR_BT_Y + 24, 128)) trocaFecha();
      break;
    case TE_FEITO:
    case TE_CANCELADO:
      if (trDentro(x, y, CX - 64, TR_BT_Y, 128)) trocaFecha();
      break;
    default:
      break;
  }
}
