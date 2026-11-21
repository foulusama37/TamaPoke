// O protocolo da batalha entre placas (duelo.h), com duas placas de mentira
// ligadas por um "ar" que perde pacotes. A regra que importa: as duas veem
// as MESMAS escolhas, na MESMA ordem, ponto a ponto. Uma escolha que so uma
// placa viu = dois combates diferentes, cada um com o seu vencedor.
#include <deque>
#include <vector>

#include "framework.h"
#include "../duelo.h"
#include "../troca.h"

namespace {

struct Placa;
struct ArD {
  struct P { int para; std::vector<uint8_t> b; };
  std::deque<P> fila;
  uint32_t semente = 1;
  int perdaPct = 0;
  Placa *lado[2] = { nullptr, nullptr };
  uint32_t rnd() {
    semente = semente * 1103515245u + 12345u;
    return (semente >> 16) & 0x7FFF;
  }
};

uint8_t MACD[2][6] = { { 0x44, 0xbd, 0x8d, 0x61, 0x51, 0xd8 },
                       { 0xb8, 0x1f, 0x3f, 0xc8, 0x74, 0x44 } };

struct Placa {
  int id = 0;
  ArD *ar = nullptr;
  Troca t;
  Duelo d;
  bool desligada = false;
};

void enviarD(void *ctx, const uint8_t *mac, const uint8_t *buf, int n) {
  Placa *l = (Placa *)ctx;
  if (l->desligada) return;
  for (int k = 0; k < 2; k++) {
    if (k == l->id) continue;
    if (mac[0] != 0xFF && memcmp(mac, MACD[k], 6) != 0) continue;
    if ((int)(l->ar->rnd() % 100) < l->ar->perdaPct) continue;
    l->ar->fila.push_back({ k, std::vector<uint8_t>(buf, buf + n) });
  }
}

void ligaD(Placa &l, int id, ArD &ar) {
  l.id = id;
  l.ar = &ar;
  ar.lado[id] = &l;
  l.t = Troca();
  l.t.g.ctx = &l;
  l.t.g.enviar = enviarD;
  l.d = Duelo();
  l.d.g.ctx = &l;
  l.d.g.enviar = enviarD;
}

uint32_t relogio = 1000;

// um passo de 10 ms: entrega o que esta no ar e faz o tick das duas
void passo(ArD &ar) {
  size_t n = ar.fila.size();
  for (size_t i = 0; i < n; i++) {
    ArD::P p = ar.fila.front();
    ar.fila.pop_front();
    Placa *l = ar.lado[p.para];
    if (l->desligada) continue;
    l->t.recebe(p.b.data(), (int)p.b.size(), relogio);
    l->d.recebe(p.b.data(), (int)p.b.size(), relogio);
  }
  for (int k = 0; k < 2; k++) {
    if (ar.lado[k]->desligada) continue;
    ar.lado[k]->t.tick(relogio);
    ar.lado[k]->d.tick(relogio);
  }
  relogio += 10;
}

void anda(ArD &ar, int passos) { for (int i = 0; i < passos; i++) passo(ar); }

// Um bicho de mentira com uma assinatura: o byte 0 e o dono, o 1 o indice.
void bicho(uint8_t *m, int dono, int i) {
  for (int j = 0; j < TROCA_MON; j++) m[j] = (uint8_t)(dono * 97 + i * 31 + j * 7);
}

// Pareia pela troca (0 convida com as regras) e entrega ao duelo.
void pareia(Placa &a, Placa &b, ArD &ar, uint8_t regras) {
  a.t.comeca(MACD[0]);
  b.t.comeca(MACD[1]);
  anda(ar, 60);
  a.t.convidar(MACD[1], 0x1234, regras);
  anda(ar, 30);
  b.t.aceitarConvite();
  for (int i = 0; i < 300 && !(a.t.podeDuelar() && b.t.podeDuelar()); i++) passo(ar);
  Placa *ps[2] = { &a, &b };
  for (Placa *p : ps) {
    p->d.comeca(MACD[p->id], MACD[p->id ^ 1], p->t.sessao, p->t.anfitriao, p->t.modo);
    p->t.entregaDuelo();
  }
}

void daTime(Placa &p) {
  uint8_t m[DUELO_MAX][TROCA_MON];
  for (uint8_t i = 0; i < p.d.n; i++) bicho(m[i], p.id, i);
  p.d.meuTime(m, p.d.n);
}

// A escolha que a placa `lado` faz no ponto k (deterministica, para conferir).
DueloDecisao escolha(int lado, uint16_t k) {
  DueloDecisao d;
  uint32_t x = (uint32_t)(k * 2654435761u) ^ (uint32_t)(lado * 0x9E37u);
  d.tipo = (uint8_t)(x % 4);         // NADA, GOLPE, TROCA, ITEM
  d.arg = (uint8_t)((x >> 8) % 4);
  d.extra = (uint8_t)((x >> 16) & 1);
  return d;
}

}  // namespace

TEST(duelo, regras_num_byte) {
  uint8_t r = duRegras(DF_3V3, DI_ATE3);
  CHECK(duRegrasValidas(r));
  CHECK_EQ((int)duTamanho(r), 3);
  CHECK_EQ((int)DR_ITENS(r), (int)DI_ATE3);
  CHECK_EQ((int)duTamanho(duRegras(DF_1V1, DI_LIVRE)), 1);
  CHECK_EQ((int)duTamanho(duRegras(DF_6V6, DI_PROIBIDO)), 6);
  CHECK(!duRegrasValidas(0));                 // 0 = e uma troca
  CHECK(!duRegrasValidas(DR_BATALHA | 3));    // formato que nao existe
  CHECK_EQ((int)sizeof(DueloPacote) <= 250, 1);
}

TEST(duelo, convite_leva_as_regras_e_so_quem_convida_e_anfitriao) {
  ArD ar;
  ar.perdaPct = 30;
  Placa a, b;
  ligaD(a, 0, ar);
  ligaD(b, 1, ar);
  uint8_t r = duRegras(DF_6V6, DI_LIVRE);
  pareia(a, b, ar, r);
  CHECK_EQ((int)a.t.estado, (int)TE_DUELO);
  CHECK_EQ((int)b.t.estado, (int)TE_DUELO);
  CHECK_EQ((int)a.t.modo, (int)r);
  CHECK_EQ((int)b.t.modo, (int)r);
  CHECK(a.t.anfitriao);
  CHECK(!b.t.anfitriao);
  CHECK_EQ(a.t.sessao, b.t.sessao);
}

TEST(duelo, convites_cruzados_concordam_em_regras_e_num_anfitriao) {
  // Os dois tocam um no outro ao mesmo tempo, cada um com as suas regras.
  for (uint32_t s = 1; s <= 40; s++) {
    ArD ar;
    ar.semente = s;
    ar.perdaPct = 20;
    Placa a, b;
    ligaD(a, 0, ar);
    ligaD(b, 1, ar);
    a.t.comeca(MACD[0]);
    b.t.comeca(MACD[1]);
    anda(ar, 60);
    uint32_t sa = 100 + (s * 37) % 50, sb = 100 + (s * 53) % 50;   // as vezes iguais
    a.t.convidar(MACD[1], sa, duRegras(DF_1V1, DI_PROIBIDO));
    b.t.convidar(MACD[0], sb, duRegras(DF_6V6, DI_ATE3));
    for (int i = 0; i < 400 && !(a.t.podeDuelar() && b.t.podeDuelar()); i++) passo(ar);
    CHECK_MSG(a.t.podeDuelar() && b.t.podeDuelar(), "nao parearam");
    CHECK_EQ(a.t.sessao, b.t.sessao);
    CHECK_EQ((int)a.t.modo, (int)b.t.modo);
    CHECK_MSG(a.t.anfitriao != b.t.anfitriao, "tem de haver UM anfitriao");
    // e as regras sao as de quem ficou anfitriao
    uint8_t esperado = a.t.anfitriao ? duRegras(DF_1V1, DI_PROIBIDO) : duRegras(DF_6V6, DI_ATE3);
    CHECK_EQ((int)a.t.modo, (int)esperado);
  }
}

TEST(duelo, os_times_chegam_inteiros_mesmo_perdendo_pacotes) {
  ArD ar;
  ar.perdaPct = 40;
  Placa a, b;
  ligaD(a, 0, ar);
  ligaD(b, 1, ar);
  pareia(a, b, ar, duRegras(DF_6V6, DI_LIVRE));
  // um escolhe antes, o outro bem depois: os do primeiro esperam
  daTime(a);
  anda(ar, 200);
  CHECK_EQ((int)a.d.estado, (int)DU_TIME);
  daTime(b);
  for (int i = 0; i < 2000 && !(a.d.prontos() && b.d.prontos()); i++) passo(ar);
  CHECK(a.d.prontos());
  CHECK(b.d.prontos());
  for (int i = 0; i < 6; i++) {
    uint8_t m[TROCA_MON];
    bicho(m, 1, i);
    CHECK_MSG(memcmp(a.d.dele[i], m, TROCA_MON) == 0, "o time do B chegou errado no A");
    bicho(m, 0, i);
    CHECK_MSG(memcmp(b.d.dele[i], m, TROCA_MON) == 0, "o time do A chegou errado no B");
  }
  CHECK_EQ((int)a.d.meuLado(), 0);
  CHECK_EQ((int)b.d.meuLado(), 1);
}

// O coracao: 300 pontos de decisao com o ar perdendo um terco, cada placa
// decidindo num tempo diferente. As duas tem de ver as mesmas escolhas.
TEST(duelo, as_duas_veem_as_mesmas_escolhas_ponto_a_ponto) {
  ArD ar;
  ar.perdaPct = 33;
  Placa a, b;
  ligaD(a, 0, ar);
  ligaD(b, 1, ar);
  pareia(a, b, ar, duRegras(DF_3V3, DI_ATE3));
  daTime(a);
  daTime(b);
  for (int i = 0; i < 2000 && !(a.d.prontos() && b.d.prontos()); i++) passo(ar);
  CHECK(a.d.prontos() && b.d.prontos());

  const int PONTOS = 300;
  std::vector<DueloDecisao> visto[2][2];   // [placa][lado]
  Placa *ps[2] = { &a, &b };
  uint32_t espera[2] = { 0, 0 };
  for (int it = 0; it < 200000; it++) {
    bool fim = true;
    for (int p = 0; p < 2; p++) {
      Placa &pl = *ps[p];
      uint16_t k = pl.d.ponto();
      if (k >= PONTOS) continue;
      fim = false;
      if (!pl.d.jaDecidi()) {
        // cada uma "pensa" um tempo diferente
        if (espera[p] == 0) espera[p] = relogio + ((k * 7 + p * 13) % 9) * 40;
        if (relogio >= espera[p]) {
          pl.d.decide(escolha(pl.d.meuLado(), k), 0xABC00000u + k);
          espera[p] = 0;
        }
      } else if (pl.d.temAsDuas()) {
        visto[p][0].push_back(pl.d.doLado(0));
        visto[p][1].push_back(pl.d.doLado(1));
        pl.d.avanca();
      }
    }
    if (fim) break;
    passo(ar);
  }
  CHECK_EQ((int)visto[0][0].size(), PONTOS);
  CHECK_EQ((int)visto[1][0].size(), PONTOS);
  bool iguais = true;
  for (int k = 0; k < PONTOS && k < (int)visto[0][0].size() && k < (int)visto[1][0].size(); k++) {
    for (int l = 0; l < 2; l++) {
      iguais = iguais && visto[0][l][k] == visto[1][l][k];
      iguais = iguais && visto[0][l][k] == escolha(l, (uint16_t)k);
    }
  }
  CHECK_MSG(iguais, "as placas viram escolhas diferentes");
  CHECK_EQ((int)a.d.estado, (int)DU_BATALHA);
  CHECK_EQ((int)b.d.estado, (int)DU_BATALHA);
}

TEST(duelo, combate_que_diverge_acaba_nas_duas) {
  ArD ar;
  ar.perdaPct = 30;
  Placa a, b;
  ligaD(a, 0, ar);
  ligaD(b, 1, ar);
  pareia(a, b, ar, duRegras(DF_1V1, DI_PROIBIDO));
  daTime(a);
  daTime(b);
  for (int i = 0; i < 2000 && !(a.d.prontos() && b.d.prontos()); i++) passo(ar);
  DueloDecisao g;
  g.tipo = DD_GOLPE;
  a.d.decide(g, 111);
  b.d.decide(g, 222);   // o combate do B ja nao e o do A
  for (int i = 0; i < 1000 && !(a.d.estado == DU_FIM && b.d.estado == DU_FIM); i++) passo(ar);
  CHECK_EQ((int)a.d.motivo, (int)DM_DESSINCRONIA);
  CHECK_EQ((int)b.d.motivo, (int)DM_DESSINCRONIA);
}

TEST(duelo, pensar_muito_nao_e_sumir_mas_desligar_e) {
  ArD ar;
  ar.perdaPct = 20;
  Placa a, b;
  ligaD(a, 0, ar);
  ligaD(b, 1, ar);
  pareia(a, b, ar, duRegras(DF_1V1, DI_LIVRE));
  daTime(a);
  daTime(b);
  for (int i = 0; i < 2000 && !(a.d.prontos() && b.d.prontos()); i++) passo(ar);
  DueloDecisao g;
  g.tipo = DD_GOLPE;
  a.d.decide(g, 5);
  anda(ar, 6000);   // o B pensa um minuto
  CHECK_EQ((int)a.d.estado, (int)DU_BATALHA);
  CHECK_EQ((int)b.d.estado, (int)DU_BATALHA);
  b.desligada = true;   // e agora a placa dele apaga
  anda(ar, 1500);       // 15 s: ainda espera
  CHECK_EQ((int)a.d.estado, (int)DU_BATALHA);
  anda(ar, 1000);       // 25 s
  CHECK_EQ((int)a.d.estado, (int)DU_FIM);
  CHECK_EQ((int)a.d.motivo, (int)DM_SUMIU);
}

TEST(duelo, quem_sai_avisa_mesmo_com_o_ar_ruim) {
  for (uint32_t s = 1; s <= 20; s++) {
    ArD ar;
    ar.semente = s;
    ar.perdaPct = 50;
    Placa a, b;
    ligaD(a, 0, ar);
    ligaD(b, 1, ar);
    pareia(a, b, ar, duRegras(DF_3V3, DI_LIVRE));
    daTime(a);
    daTime(b);
    for (int i = 0; i < 3000 && !(a.d.prontos() && b.d.prontos()); i++) passo(ar);
    b.d.sair();
    anda(ar, 300);   // 3 s: bem antes do prazo de sumir
    CHECK_EQ((int)b.d.motivo, (int)DM_SAI);
    CHECK_EQ((int)a.d.estado, (int)DU_FIM);
    CHECK_EQ((int)a.d.motivo, (int)DM_ELE_SAIU);
  }
}

TEST(duelo, a_ultima_decisao_chega_mesmo_depois_de_um_acabar) {
  // O A tem as duas do ultimo ponto e acaba; a dele para o B caiu. O B
  // tem de chegar ao mesmo fim, nao ficar esperando ate dar o A por sumido.
  for (uint32_t s = 1; s <= 20; s++) {
    ArD ar;
    ar.semente = s;
    ar.perdaPct = 40;
    Placa a, b;
    ligaD(a, 0, ar);
    ligaD(b, 1, ar);
    pareia(a, b, ar, duRegras(DF_1V1, DI_LIVRE));
    daTime(a);
    daTime(b);
    for (int i = 0; i < 3000 && !(a.d.prontos() && b.d.prontos()); i++) passo(ar);
    DueloDecisao g;
    g.tipo = DD_GOLPE;
    b.d.decide(g, 9);
    for (int i = 0; i < 200 && !a.d.temAsDuas(); i++) {
      if (!a.d.jaDecidi()) {
        ar.perdaPct = 100;   // a primeira do A se perde
        a.d.decide(g, 9);
        ar.perdaPct = 40;
      }
      passo(ar);
    }
    if (a.d.temAsDuas()) { a.d.avanca(); a.d.acabou(); }
    for (int i = 0; i < 1000 && !b.d.temAsDuas(); i++) passo(ar);
    CHECK_MSG(b.d.temAsDuas(), "o B nunca soube da ultima decisao do A");
  }
}
