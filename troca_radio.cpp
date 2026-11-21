// Radio das trocas: ver troca_radio.h.
#include "troca_radio.h"

#include <string.h>

#ifdef ESP_PLATFORM
// ================================================================ placa
#include <WiFi.h>
#include <esp_mac.h>
#include <esp_now.h>
#include <esp_wifi.h>

#define FILA 8
#define TAM 256
// O callback do ESP-NOW roda na tarefa do Wi-Fi: so copia para uma fila e
// o loop() le de la. Nada de mexer no protocolo fora do loop.
struct Recebido {
  int n;
  uint8_t b[TAM];
};
static Recebido fila[FILA];
static volatile uint8_t cab = 0, cauda = 0;
static portMUX_TYPE trava = portMUX_INITIALIZER_UNLOCKED;
static bool ligado = false;

static void aoReceber(const esp_now_recv_info_t *, const uint8_t *d, int n) {
  if (n <= 0 || n > TAM) return;
  portENTER_CRITICAL(&trava);
  uint8_t prox = (uint8_t)((cab + 1) % FILA);
  if (prox != cauda) {   // cheia: descarta (o protocolo reenvia)
    memcpy(fila[cab].b, d, n);
    fila[cab].n = n;
    cab = prox;
  }
  portEXIT_CRITICAL(&trava);
}

static void garantePar(const uint8_t *mac) {
  if (esp_now_is_peer_exist(mac)) return;
  esp_now_peer_info_t p;
  memset(&p, 0, sizeof(p));
  memcpy(p.peer_addr, mac, 6);
  p.channel = 1;
  p.ifidx = WIFI_IF_STA;
  p.encrypt = false;
  esp_now_add_peer(&p);
}

bool radioLiga() {
  if (ligado) return true;
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
  if (esp_now_init() != ESP_OK) {
    WiFi.mode(WIFI_OFF);
    return false;
  }
  esp_now_register_recv_cb(aoReceber);
  static const uint8_t TODOS[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
  garantePar(TODOS);
  cab = cauda = 0;
  ligado = true;
  return true;
}

void radioDesliga() {
  if (!ligado) return;
  esp_now_deinit();
  WiFi.mode(WIFI_OFF);
  ligado = false;
}

bool radioLigado() { return ligado; }

void radioMac(uint8_t mac[6]) { esp_read_mac(mac, ESP_MAC_WIFI_STA); }

void radioEnvia(const uint8_t *mac, const uint8_t *buf, int n) {
  if (!ligado) return;
  garantePar(mac);
  esp_now_send(mac, buf, (size_t)n);
}

int radioRecebe(uint8_t *buf, int cap) {
  int n = 0;
  portENTER_CRITICAL(&trava);
  if (cauda != cab) {
    n = fila[cauda].n <= cap ? fila[cauda].n : 0;
    if (n) memcpy(buf, fila[cauda].b, n);
    cauda = (uint8_t)((cauda + 1) % FILA);
  }
  portEXIT_CRITICAL(&trava);
  return n;
}

void radioTick(uint32_t) {}

#else
// ================================================================ PC
// A placa fantasma: um segundo protocolo inteiro, com ganchos que so
// fingem gravar. Responde ao convite e oferece um Kadabra nv25.
#include <deque>
#include <vector>

#include "duelo.h"
#include "troca.h"

static const uint8_t EU[6] = { 0x02, 0x00, 0x00, 0x00, 0x00, 0x01 };
static const uint8_t ELA[6] = { 0x02, 0x00, 0x00, 0x00, 0x00, 0x02 };
static bool ligado = false;
static std::deque<std::vector<uint8_t>> paraMim;
static Troca fantasma;
static Duelo fduelo;   // a batalha: aceita e manda o time dela
static TrocaDiario fdiario;
static uint32_t relogio = 0;
static bool fCala = false;        // testes: a fantasma some (nao manda nada)
static bool fHashErrado = false;  // testes: ela ve outro combate

// O combate e o da tela (o mesmo processo): ela le o ponto aberto la.
bool pvpFantasmaJoga(uint8_t lado, uint16_t k, DueloDecisao &d, uint32_t &hash);

void radioFantasmaCala(bool s) { fCala = s; }
void radioFantasmaHashErrado(bool s) { fHashErrado = s; }
// Ela convida para batalhar (e anfitria): a tela daqui e a convidada.
void radioFantasmaConvida(uint8_t regras) {
  if (!ligado) return;
  static uint32_t s = 0x7A11u;
  s = s * 1103515245u + 12345u;
  fantasma.convidar(EU, s | 1u, regras);
}

static void fEnvia(void *, const uint8_t *, const uint8_t *b, int n) {
  if (ligado && !fCala) paraMim.emplace_back(b, b + n);
}
static bool fPrepara(void *, TrocaDiario &d) {
  trocaSelaDiario(d);
  fdiario = d;
  return true;
}
static bool fAplica(void *, const TrocaDiario &) { return true; }

bool radioLiga() {
  ligado = true;
  paraMim.clear();
  fantasma = Troca();
  fantasma.g.enviar = fEnvia;
  fantasma.g.preparar = fPrepara;
  fantasma.g.aplicar = fAplica;
  fantasma.titular = 64;   // Kadabra
  fantasma.nivel = 25;
  fantasma.comeca(ELA);
  fduelo = Duelo();
  fduelo.g.enviar = fEnvia;
  fCala = false;
  fHashErrado = false;
  fantasma.tick(relogio);
  return true;
}

void radioDesliga() {
  ligado = false;
  paraMim.clear();
}

bool radioLigado() { return ligado; }

void radioMac(uint8_t mac[6]) { memcpy(mac, EU, 6); }

void radioEnvia(const uint8_t *, const uint8_t *buf, int n) {
  if (!ligado) return;
  fantasma.recebe(buf, n, relogio);
  fduelo.recebe(buf, n, relogio);
}

int radioRecebe(uint8_t *buf, int cap) {
  if (paraMim.empty()) return 0;
  std::vector<uint8_t> p = paraMim.front();
  paraMim.pop_front();
  if ((int)p.size() > cap) return 0;
  memcpy(buf, p.data(), p.size());
  return (int)p.size();
}

void radioTick(uint32_t agora) {
  if (!ligado) return;
  relogio = agora;
  fantasma.tick(agora);
  // a fantasma e boazinha: aceita o convite, oferece e aceita o par
  if (fantasma.estado == TE_CONVIDADO) fantasma.aceitarConvite();
  // Convite para batalhar: passa ao Duelo e manda o time dela (Kadabra,
  // Machoke, Graveler, Haunter, Golem, Alakazam: os seis do PC).
  if (fantasma.podeDuelar()) {
    fantasma.entregaDuelo();
    fduelo.comeca(ELA, fantasma.outro, fantasma.sessao, fantasma.anfitriao, fantasma.modo);
    static const int16_t ESP[6] = { 64, 67, 75, 93, 76, 65 };
    uint8_t regs[DUELO_MAX][TROCA_MON];
    for (uint8_t i = 0; i < fduelo.n; i++) {
      StoredMon m;
      m.species = ESP[i];
      m.exp = 15625;
      m.hpLeft = 60;
      memcpy(regs[i], &m, TROCA_MON);
    }
    fduelo.meuTime(regs, fduelo.n);
  }
  fduelo.tick(agora);
  // No combate: joga pelo lado dela quando o ponto aberto na tela e o dela.
  if (fduelo.estado == DU_BATALHA) {
    DueloDecisao d;
    uint32_t h = 0;
    if (!fduelo.jaDecidi() && pvpFantasmaJoga(fduelo.meuLado(), fduelo.ponto(), d, h))
      fduelo.decide(d, fHashErrado ? (h ^ 0x5A5A5A5Au) : h);
    if (fduelo.temAsDuas()) fduelo.avanca();
  }
  // acabou (o outro saiu ou sumiu): volta a procurar, como o OK da tela
  if (fantasma.estado == TE_DUELO && fduelo.estado == DU_FIM) {
    fantasma.estado = TE_PROCURA;
    fantasma.comeca(ELA);
  }
  if (fantasma.estado == TE_ESCOLHA && !fantasma.tenhoMeu && !fantasma.modo) {
    StoredMon m;
    m.species = 64;
    m.exp = 15625;   // nv25 no crescimento medio
    m.hpLeft = 60;
    m.moves[0] = 93;
    m.ppLeft[0] = 25;
    fantasma.oferecer(1, (const uint8_t *)&m);
  }
  if (fantasma.podeAceitar()) fantasma.aceitar();
}
#endif
