// O protocolo das trocas (troca.h), com duas placas de mentira ligadas por um
// "ar" que perde pacotes, e reinicios no meio. A regra que importa: no fim,
// OU as duas aplicaram OU nenhuma. Uma so = um bicho sumiu ou duplicou.
#include <deque>
#include <vector>

#include "framework.h"
#include "../troca.h"

namespace {

struct Pacote {
  int para;   // 0 ou 1
  std::vector<uint8_t> b;
};

struct Lado;
struct Ar {
  std::deque<Pacote> fila;
  uint32_t semente = 1;
  int perdaPct = 0;
  Lado *lado[2] = { nullptr, nullptr };
  uint32_t rnd() {
    semente = semente * 1103515245u + 12345u;
    return (semente >> 16) & 0x7FFF;
  }
};

struct Lado {
  int id = 0;
  Ar *ar = nullptr;
  Troca t;
  // o "SD"
  bool temDiario = false;
  TrocaDiario sd;
  bool sdFalha = false;
  int aplicacoes = 0;     // quantas vezes o save mudou de verdade
  int descartes = 0;
  bool desligado = false;
};

uint8_t MAC[2][6] = { { 0x44, 0xbd, 0x8d, 0x61, 0x51, 0xd8 },
                      { 0x44, 0xbd, 0x8d, 0x60, 0x0b, 0x2c } };

void enviar(void *ctx, const uint8_t *mac, const uint8_t *buf, int n) {
  Lado *l = (Lado *)ctx;
  for (int k = 0; k < 2; k++) {
    if (k == l->id) continue;
    bool todos = mac[0] == 0xFF;
    if (!todos && memcmp(mac, MAC[k], 6) != 0) continue;
    if ((int)(l->ar->rnd() % 100) < l->ar->perdaPct) continue;
    l->ar->fila.push_back({ k, std::vector<uint8_t>(buf, buf + n) });
  }
}

bool preparar(void *ctx, TrocaDiario &d) {
  Lado *l = (Lado *)ctx;
  if (l->sdFalha) return false;
  trocaSelaDiario(d);
  l->sd = d;
  l->temDiario = true;
  return true;
}

bool aplicar(void *ctx, const TrocaDiario &d) {
  Lado *l = (Lado *)ctx;
  // idempotente como o do Pet: so muda o save se o diario ainda diz PREPARADO
  if (l->temDiario && l->sd.sessao == d.sessao && l->sd.estado == TD_PREPARADO) {
    l->sd.estado = TD_APLICADO;
    trocaSelaDiario(l->sd);
    l->aplicacoes++;
  }
  return true;
}

void descartar(void *ctx, const TrocaDiario &d) {
  Lado *l = (Lado *)ctx;
  if (l->temDiario && l->sd.sessao == d.sessao && l->sd.estado == TD_PREPARADO) {
    l->sd.estado = TD_DESCARTADO;
    trocaSelaDiario(l->sd);
    l->descartes++;
  }
}

void liga(Lado &l, int id, Ar &ar) {
  l.id = id;
  l.ar = &ar;
  ar.lado[id] = &l;
  l.t = Troca();
  l.t.g.ctx = &l;
  l.t.g.enviar = enviar;
  l.t.g.preparar = preparar;
  l.t.g.aplicar = aplicar;
  l.t.g.descartar = descartar;
  l.t.titular = id ? 25 : 7;
  l.t.comeca(MAC[id]);
  l.desligado = false;
}

// Reinicio: tudo da RAM some; o que fica e o "SD". Como o firmware faz no
// boot: diario PREPARADO -> retoma; APLICADO/DESCARTADO -> so lembra.
void reinicia(Lado &l, Ar &ar) {
  bool tem = l.temDiario;
  TrocaDiario d = l.sd;
  liga(l, l.id, ar);
  if (tem && trocaDiarioValido(d)) {
    if (d.estado == TD_PREPARADO) l.t.retoma(d);
    else l.t.lembra(d.sessao, d.estado);
  }
}

void mon(uint8_t *b, int16_t especie, uint8_t extra) {
  StoredMon m;
  m.species = especie;
  m.exp = 1000 + extra;
  memcpy(b, &m, sizeof(m));
}

// roda o ar ate esvaziar ou acabar o tempo
void roda(Ar &ar, Lado &a, Lado &b, uint32_t &agora, uint32_t ms) {
  uint32_t fim = agora + ms;
  while (agora < fim) {
    agora += 20;
    if (!a.desligado) a.t.tick(agora);
    if (!b.desligado) b.t.tick(agora);
    for (int k = 0; k < 64 && !ar.fila.empty(); k++) {
      Pacote p = ar.fila.front();
      ar.fila.pop_front();
      Lado *l = ar.lado[p.para];
      if (!l->desligado) l->t.recebe(p.b.data(), (int)p.b.size(), agora);
    }
  }
}

// convite + ofertas + os dois aceitam
void pareia(Ar &ar, Lado &a, Lado &b, uint32_t &agora) {
  roda(ar, a, b, agora, 1200);   // OLA
  a.t.convidar(MAC[1], 0xC0FFEE);
  roda(ar, a, b, agora, 600);
  b.t.aceitarConvite();
  roda(ar, a, b, agora, 600);
  uint8_t m[TROCA_MON];
  mon(m, 64, 1);    // Kadabra
  a.t.oferecer(2, m);
  mon(m, 95, 2);    // Onix
  b.t.oferecer(1, m);
  roda(ar, a, b, agora, 900);
}

}  // namespace

TEST(troca, o_caminho_feliz_aplica_UMA_vez_dos_dois_lados) {
  Ar ar;
  Lado a, b;
  liga(a, 0, ar);
  liga(b, 1, ar);
  uint32_t agora = 0;
  pareia(ar, a, b, agora);
  CHECK_EQ((int)a.t.estado, (int)TE_ESCOLHA);
  CHECK(a.t.podeAceitar());
  CHECK(b.t.podeAceitar());
  a.t.aceitar();
  roda(ar, a, b, agora, 600);
  // so um aceitou: ninguem prepara
  CHECK(!a.temDiario);
  CHECK(!b.temDiario);
  b.t.aceitar();
  roda(ar, a, b, agora, 2000);
  CHECK_EQ((int)a.t.estado, (int)TE_FEITO);
  CHECK_EQ((int)b.t.estado, (int)TE_FEITO);
  CHECK_EQ(a.aplicacoes, 1);
  CHECK_EQ(b.aplicacoes, 1);
  // cada um ficou com o do outro
  StoredMon ca, cb;
  memcpy(&ca, a.sd.chega, sizeof(ca));
  memcpy(&cb, b.sd.chega, sizeof(cb));
  CHECK_EQ(ca.species, 95);
  CHECK_EQ(cb.species, 64);
  CHECK_EQ((int)a.sd.slot, 2);
  CHECK_EQ((int)b.sd.slot, 1);
}

TEST(troca, mudar_a_oferta_depois_de_aceitar_nao_vale) {
  Ar ar;
  Lado a, b;
  liga(a, 0, ar);
  liga(b, 1, ar);
  uint32_t agora = 0;
  pareia(ar, a, b, agora);
  a.t.aceitar();
  uint8_t m[TROCA_MON];
  mon(m, 150, 9);
  a.t.oferecer(3, m);           // tarde: ignorado
  CHECK_EQ((int)a.t.estado, (int)TE_ACEITEI);
  b.t.aceitar();
  roda(ar, a, b, agora, 2000);
  StoredMon cb;
  memcpy(&cb, b.sd.chega, sizeof(cb));
  CHECK_EQ(cb.species, 64);     // o Kadabra combinado, nao o Mewtwo
}

TEST(troca, quem_nao_preparou_cancela_e_o_outro_descarta) {
  Ar ar;
  Lado a, b;
  liga(a, 0, ar);
  liga(b, 1, ar);
  uint32_t agora = 0;
  pareia(ar, a, b, agora);
  b.sdFalha = true;   // o SD do B nao grava: ele nao pode preparar
  a.t.aceitar();
  b.t.aceitar();
  roda(ar, a, b, agora, 3000);
  CHECK_EQ(a.aplicacoes, 0);
  CHECK_EQ(b.aplicacoes, 0);
  CHECK_EQ((int)b.t.estado, (int)TE_CANCELADO);
  CHECK_EQ((int)a.t.estado, (int)TE_CANCELADO);
  if (a.temDiario) CHECK_EQ((int)a.sd.estado, (int)TD_DESCARTADO);
}

TEST(troca, convites_cruzados_viram_um_par_so) {
  Ar ar;
  Lado a, b;
  liga(a, 0, ar);
  liga(b, 1, ar);
  uint32_t agora = 0;
  roda(ar, a, b, agora, 1200);
  CHECK_EQ((int)a.t.nViz, 1);
  a.t.convidar(MAC[1], 500);
  b.t.convidar(MAC[0], 300);   // antes de ouvir o convite do A
  roda(ar, a, b, agora, 1500);
  CHECK_EQ((int)a.t.estado, (int)TE_ESCOLHA);
  CHECK_EQ((int)b.t.estado, (int)TE_ESCOLHA);
  CHECK_EQ(a.t.sessao, 300u);
  CHECK_EQ(b.t.sessao, 300u);
}

// O teste grande: ar ruim e reinicio de uma placa num momento sorteado.
// Depois as duas voltam a se encontrar (tela aberta, ar bom) e tem de
// terminar igual.
TEST(troca, ar_ruim_e_reinicio_no_meio_nunca_deixam_so_um_lado_aplicado) {
  int ambos = 0, nenhum = 0;
  for (uint32_t s = 1; s <= 400; s++) {
    Ar ar;
    ar.semente = s * 2654435761u;
    Lado a, b;
    liga(a, 0, ar);
    liga(b, 1, ar);
    uint32_t agora = 0;
    pareia(ar, a, b, agora);
    ar.perdaPct = 20 + (int)(s % 50);
    a.t.aceitar();
    b.t.aceitar();
    // corre um pedaco e derruba alguem num instante qualquer
    Lado &v = (s & 1) ? a : b;
    roda(ar, a, b, agora, 20 * (1 + ar.rnd() % 40));
    v.desligado = true;
    ar.fila.clear();
    roda(ar, a, b, agora, 20 * (1 + ar.rnd() % 400));
    // volta: reinicia (da RAM so sobra o SD) e reabre a tela de troca
    reinicia(v, ar);
    Lado &o = (s & 1) ? b : a;
    // o que nao caiu tambem pode ter fechado a tela e voltado
    if (s % 3 == 0) {
      o.t.para();
      reinicia(o, ar);
    }
    ar.perdaPct = 0;
    roda(ar, a, b, agora, 6000);
    CHECK(a.aplicacoes <= 1);
    CHECK(b.aplicacoes <= 1);
    if (a.aplicacoes != b.aplicacoes) {
      char msg[96];
      snprintf(msg, sizeof msg, "semente %u: A aplicou %d, B aplicou %d", (unsigned)s,
               a.aplicacoes, b.aplicacoes);
      CHECK_MSG(false, msg);
      return;
    }
    // e ninguem fica preso esperando
    CHECK(a.t.estado != TE_PREPARADO);
    CHECK(b.t.estado != TE_PREPARADO);
    (a.aplicacoes ? ambos : nenhum)++;
  }
  // os dois desfechos tem de aparecer, senao o teste nao exercita nada
  CHECK(ambos > 20);
  CHECK(nenhum > 20);
}

// ---------------------------------------------------------------- o save
// Agora com o Pet de verdade: o que entra, onde entra, e que reaplicar
// (reinicio entre o save e o diario) nao da o bicho duas vezes.
#include <filesystem>
#include "../pet.h"

void mockNvsReset();
void mockSetMillis(uint32_t);
void mockSdRoot(const char *);

namespace {

uint8_t itemPorNome(const char *n) {
  for (uint8_t i = 0; i < ITEM_COUNT; i++)
    if (strcmp(ITEM_TBL[i].name, n) == 0) return i;
  return 0;
}

StoredMon bicho(int16_t dex, uint16_t nivel, uint8_t item = 0) {
  StoredMon m{};
  m.species = dex;
  m.exp = Pet::expForLevel(nivel);
  m.moves[0] = MOVE_STRUGGLE;
  m.ppLeft[0] = 5;
  m.hpLeft = 30;
  m.item = item;
  return m;
}

// Devolve a "tarjeta" de sempre ao sair: os outros testes leem dela.
struct RaizSd {
  ~RaizSd() { mockSdRoot("../tools/sdcard"); }
};

void petNovo(Pet &p) {
  // Uma pasta NOVA por teste, e o remove_all que nao lanca: no Windows
  // apagar a pasta logo depois de usada as vezes falha (arquivo ainda
  // preso) e a excecao derrubava o binario inteiro, num teste qualquer.
  static int n = 0;
  static char raiz[48];
  snprintf(raiz, sizeof raiz, "build/sd_troca%d", n++);
  std::error_code ec;
  std::filesystem::remove_all(raiz, ec);
  std::filesystem::create_directories(raiz, ec);
  mockSdRoot(raiz);
  mockNvsReset();
  mockSetMillis(0);
  p.begin();
  if (p.awaitingStarter()) p.chooseStarter(1);
  p.eggTap();
  p.eggTap();
  p.eggTap();
}

// um diario como o protocolo monta: o meu do slot, o dele pelo ar
TrocaDiario diarioDe(Pet &p, uint8_t slot, const StoredMon &dele, uint32_t s) {
  TrocaDiario d;
  memset(&d, 0, sizeof(d));
  d.sessao = s;
  d.slot = slot;
  memcpy(d.sai, p.trocaMeu(slot), TROCA_MON);
  memcpy(d.veio, &dele, TROCA_MON);
  memcpy(d.chega, &dele, TROCA_MON);
  return d;
}

uint8_t slotCom(Pet &p, int16_t dex) {
  for (uint8_t i = 0; i < TEAM_SLOTS; i++)
    if (p.team[i].used() && p.team[i].species == dex) return i;
  return 0xFF;
}

int contaEmTudo(Pet &p, int16_t dex) {
  int n = 0;
  for (uint8_t i = 0; i < TEAM_SLOTS; i++) n += p.team[i].species == dex;
  for (uint8_t i = 0; i < BANK_SLOTS; i++) n += p.bank[i].species == dex;
  return n;
}

}  // namespace

TEST(troca, chegar_evolui_como_no_anil) {
  const uint8_t coat = itemPorNome("Metal Coat");
  const uint8_t eterna = itemPorNome("Everstone");
  const uint8_t restos = itemPorNome("Leftovers");
  CHECK_MSG(coat && eterna && restos, "premissa: os tres objetos existem");
  // sem objeto: Kadabra -> Alakazam
  CHECK_EQ(Pet::trocaChegada(bicho(64, 30)).species, 65);
  // com o objeto certo: Onix -> Steelix, e o Metal Coat se gasta
  StoredMon s = Pet::trocaChegada(bicho(95, 30, coat));
  CHECK_EQ(s.species, 208);
  CHECK_EQ((int)s.item, 0);
  // Onix sem nada, ou com outro objeto: continua Onix (e com o objeto)
  CHECK_EQ(Pet::trocaChegada(bicho(95, 30)).species, 95);
  StoredMon r = Pet::trocaChegada(bicho(95, 30, restos));
  CHECK_EQ(r.species, 95);
  CHECK_EQ((int)r.item, (int)restos);
  // Everstone segura os dois tipos
  CHECK_EQ(Pet::trocaChegada(bicho(64, 30, eterna)).species, 64);
  // quem nao evolui por troca: igual, byte a byte
  StoredMon p = bicho(25, 30);
  StoredMon q = Pet::trocaChegada(p);
  CHECK(memcmp(&p, &q, sizeof(p)) == 0);
}

TEST(troca, aplicar_poe_no_lugar_do_meu_registra_e_nao_duplica) {
  RaizSd raiz;
  Pet p;
  petNovo(p);
  CHECK(p.addToTeam(bicho(16, 5)));          // Pidgey: o que sai
  uint8_t slot = slotCom(p, 16);
  CHECK(slot != 0xFF && slot != p.mainSlot);
  CHECK(p.trocaPodeOferecer(slot));
  CHECK(!p.trocaPodeOferecer(p.mainSlot));   // o titular nao sai
  CHECK(!p.isRegistered(65));
  TrocaDiario d = diarioDe(p, slot, bicho(64, 6), 77);
  CHECK(p.trocaPreparar(d));
  StoredMon c;
  memcpy(&c, d.chega, sizeof(c));
  CHECK_EQ(c.species, 65);                   // ja vem Alakazam no diario
  TrocaDiario lido;
  CHECK(Pet::trocaLeDiario(lido));
  CHECK_EQ((int)lido.estado, (int)TD_PREPARADO);
  CHECK(p.trocaAplicar(d));
  CHECK_EQ(p.team[slot].species, 65);
  CHECK_EQ(contaEmTudo(p, 16), 0);
  CHECK(p.isRegistered(65));
  CHECK(p.isRegistered(64));                 // viu o de antes tambem
  CHECK(p.trocaAplicar(d));                  // de novo: nada muda
  CHECK_EQ(contaEmTudo(p, 65), 1);
  CHECK(Pet::trocaLeDiario(lido));
  CHECK_EQ((int)lido.estado, (int)TD_APLICADO);
}

// Reinicio DEPOIS do save e ANTES de marcar o diario: no boot o diario ainda
// diz PREPARADO e o protocolo vai mandar aplicar de novo.
TEST(troca, reiniciar_entre_o_save_e_o_diario_nao_da_o_bicho_duas_vezes) {
  RaizSd raiz;
  Pet p;
  petNovo(p);
  CHECK(p.addToTeam(bicho(16, 5)));
  uint8_t slot = slotCom(p, 16);
  TrocaDiario d = diarioDe(p, slot, bicho(25, 6), 991);
  CHECK(p.trocaPreparar(d));
  CHECK(p.trocaAplicar(d));
  // volta o diario para PREPARADO, como se o reinicio tivesse sido antes
  CHECK(Pet::trocaGravaDiario(d));
  Pet q;
  q.begin();
  CHECK_EQ(q.trocaUltima, 991u);
  CHECK_EQ(contaEmTudo(q, 25), 1);
  CHECK(q.trocaAplicar(d));
  CHECK_EQ(contaEmTudo(q, 25), 1);
  CHECK_EQ(contaEmTudo(q, 16), 0);
}

TEST(troca, acima_do_teto_daqui_chega_no_banco_com_cadeado) {
  RaizSd raiz;
  Pet p;
  petNovo(p);
  CHECK(p.addToTeam(bicho(16, 5)));
  uint8_t slot = slotCom(p, 16);
  CHECK_MSG(p.bankCapHere() < 60, "premissa: Kanto sem insignias tem teto baixo");
  TrocaDiario d = diarioDe(p, slot, bicho(149, 60), 5);   // Dragonite nv60
  CHECK(p.trocaPreparar(d));
  CHECK(p.trocaAplicar(d));
  CHECK(!p.team[slot].used());               // nao entrou no time
  int noBanco = -1;
  for (uint8_t i = 0; i < BANK_SLOTS; i++)
    if (p.bank[i].species == 149) noBanco = i;
  CHECK(noBanco >= 0);
  if (noBanco >= 0) CHECK(p.bankLocked(p.bank[noBanco]));
  CHECK_EQ(contaEmTudo(p, 16), 0);
}

TEST(troca, preparar_recusa_se_o_meu_mudou) {
  RaizSd raiz;
  Pet p;
  petNovo(p);
  CHECK(p.addToTeam(bicho(16, 5)));
  uint8_t slot = slotCom(p, 16);
  TrocaDiario d = diarioDe(p, slot, bicho(25, 6), 8);
  p.team[slot].exp += 10;                    // ganhou exp depois de oferecer
  CHECK(!p.trocaPreparar(d));
  TrocaDiario t = diarioDe(p, p.mainSlot, bicho(25, 6), 9);
  CHECK(!p.trocaPreparar(t));                // titular: nunca
}

// Leva 49: tambem sai do BANCO (a caixa aberta). Quem chega fica no mesmo
// hueco da caixa; o time nao muda.
TEST(troca, do_banco_sai_da_caixa_e_quem_chega_fica_no_mesmo_hueco) {
  RaizSd raiz;
  Pet p;
  petNovo(p);
  p.bank[3] = bicho(16, 5);                  // Pidgey na caixa
  StoredMon timeAntes[TEAM_SLOTS];
  memcpy(timeAntes, p.team, sizeof(timeAntes));
  const uint8_t slot = (uint8_t)(TROCA_BANCO | 3);
  CHECK(p.trocaPodeOferecer(slot));
  CHECK(!p.trocaPodeOferecer((uint8_t)(TROCA_BANCO | 4)));             // vazio
  CHECK(!p.trocaPodeOferecer((uint8_t)(TROCA_BANCO | (BANK_SLOTS + 1))));  // fora
  TrocaDiario d = diarioDe(p, slot, bicho(64, 6), 4242);
  CHECK(p.trocaPreparar(d));
  CHECK_EQ((int)d.rsv, (int)p.boxOpen);
  CHECK(p.trocaAplicar(d));
  CHECK_EQ(p.bank[3].species, 65);           // Kadabra chegou Alakazam, no 3
  CHECK_EQ(contaEmTudo(p, 16), 0);
  CHECK(memcmp(timeAntes, p.team, sizeof(timeAntes)) == 0);
  CHECK(p.trocaAplicar(d));                  // de novo: nada muda
  CHECK_EQ(contaEmTudo(p, 65), 1);
}

// Reiniciou com a troca pendente e OUTRA caixa aberta: o aplicar volta a
// caixa de onde saiu, e a outra fica como estava.
TEST(troca, do_banco_com_outra_caixa_aberta_volta_a_caixa_certa) {
  RaizSd raiz;
  Pet p;
  petNovo(p);
  CHECK_EQ((int)p.boxOpen, 0);
  p.bank[2] = bicho(16, 5);
  const uint8_t slot = (uint8_t)(TROCA_BANCO | 2);
  TrocaDiario d = diarioDe(p, slot, bicho(25, 6), 777);
  CHECK(p.trocaPreparar(d));
  CHECK(p.boxGo(1));
  p.bank[0] = bicho(19, 4);                  // Rattata na caixa 1
  CHECK(p.trocaAplicar(d));
  CHECK_EQ((int)p.boxOpen, 0);
  CHECK_EQ(p.bank[2].species, 25);
  CHECK_EQ(contaEmTudo(p, 16), 0);
  CHECK(p.boxGo(1));
  CHECK_EQ(p.bank[0].species, 19);           // a caixa 1 intacta
  CHECK_EQ(contaEmTudo(p, 25), 0);
}

// O placar das batalhas entre placas e a unica coisa que elas gravam: tem
// de sobreviver a um reinicio, e um resultado que nao existe nao conta.
TEST(troca, o_placar_das_batalhas_sobrevive_ao_reinicio) {
  RaizSd raiz;
  Pet p;
  petNovo(p);
  p.contaDuelo(0);
  p.contaDuelo(0);
  p.contaDuelo(1);
  p.contaDuelo(2);
  p.contaDuelo(0xFF);          // queda / nada: nao conta
  CHECK_EQ((int)p.duV, 2);
  CHECK_EQ((int)p.duD, 1);
  CHECK_EQ((int)p.duE, 1);
  Pet q;
  q.begin();
  CHECK_EQ((int)q.duV, 2);
  CHECK_EQ((int)q.duD, 1);
  CHECK_EQ((int)q.duE, 1);
}
