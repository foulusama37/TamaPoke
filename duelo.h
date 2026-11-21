#pragma once
// BATALHA entre duas placas: o PROTOCOLO, puro como o da troca (troca.h).
// Nao sabe de radio, de tela nem do combate: troca bytes e guarda escolhas.
//
// Combinado com o dono (2026-11-14): quem convida escolhe as regras (1v1,
// 3v3 ou 6v6; itens da mochila proibidos, livres ou ate 3), todos lutam no
// nivel 50 e o save NAO muda.
//
// Como as duas chegam ao mesmo combate: LOCKSTEP. Quem convidou e o lado 0
// nas DUAS placas e o convidado o lado 1; as duas simulam o mesmo combate
// com o mesmo dado (btRngSemente com a semente da sessao) e so trocam as
// ESCOLHAS. A convidada so desenha espelhada. Simular espelhado nao serve:
// o motor trata o lado 0 diferente em varios sitios (ordem do fim de turno,
// moeda do empate) e as placas divergiriam sem ninguem notar.
//
// O fluxo:
//   o pareamento e o da troca (OLA/CONVITE/SIM, com as regras no CONVITE)
//   TIME     -- cada uma manda os seus, um por pacote (6 x 52 nao cabem nos
//               250 bytes do ESP-NOW), ate o outro confirmar com o hash
//   BATALHA  -- ponto de decisao k: as duas mandam a sua (ou NADA, quando so
//               a outra decide) com a impressao digital do combate; quando
//               tenho as duas, avanco. Hash diferente = DESSINCRONIA.
//   FIM      -- acabou, alguem desistiu/saiu, o outro sumiu ou divergiu.
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "troca.h"   // TROCA_MON, trocaCrc

#define DUELO_MAGIA 0x55445054u   // "TPDU"
#define DUELO_VER 1
#define DUELO_MAX 6

// As regras num byte, que viaja no CONVITE da troca (0 = e uma troca).
#define DR_BATALHA 0x80
#define DR_FORMATO(r) ((r) & 3)          // 0 = 1v1, 1 = 3v3, 2 = 6v6
#define DR_ITENS(r) (((r) >> 2) & 3)     // 0 proibido, 1 livre, 2 ate 3
enum : uint8_t { DF_1V1 = 0, DF_3V3 = 1, DF_6V6 = 2 };
enum : uint8_t { DI_PROIBIDO = 0, DI_LIVRE = 1, DI_ATE3 = 2 };
static inline uint8_t duRegras(uint8_t formato, uint8_t itens) {
  return (uint8_t)(DR_BATALHA | (formato & 3) | ((itens & 3) << 2));
}
static inline uint8_t duTamanho(uint8_t regras) {
  uint8_t f = DR_FORMATO(regras);
  return f == DF_1V1 ? 1 : f == DF_3V3 ? 3 : 6;
}
static inline bool duRegrasValidas(uint8_t r) {
  return (r & DR_BATALHA) && DR_FORMATO(r) <= DF_6V6 && DR_ITENS(r) <= DI_ATE3 &&
         (r & 0x70) == 0;
}

enum : uint8_t {
  DP_TIME = 1,   // um dos meus: idx de n
  DP_TIMEOK,     // recebi o teu time inteiro (hash = o que recebi)
  DP_DECISAO,    // a minha escolha no ponto k (com o hash do combate)
  DP_VIVO,       // ainda estou aqui (pensando)
  DP_SAI,        // fui embora
};

// Uma escolha. Pequena de proposito: e o que viaja a cada turno.
enum : uint8_t {
  DD_NADA = 0,    // neste ponto nao me cabe decidir (so o outro)
  DD_GOLPE,       // arg = hueco do golpe, extra bit0 = mega
  DD_TROCA,       // arg = indice no MEU time (0..n-1)
  DD_ITEM,        // arg = objeto, extra = indice no meu time
  DD_DESISTE,
};
struct DueloDecisao {
  uint8_t tipo = DD_NADA, arg = 0, extra = 0, rsv = 0;
  bool operator==(const DueloDecisao &o) const {
    return tipo == o.tipo && arg == o.arg && extra == o.extra;
  }
};

struct DueloPacote {
  uint32_t magia;
  uint8_t ver, tipo, idx, n;
  uint8_t de[6], para[6];
  uint32_t sessao;
  uint16_t k;            // ponto de decisao
  uint8_t rsv[2];
  uint32_t hash;         // TIME: hash do time inteiro; DECISAO: do combate
  DueloDecisao dec;
  uint8_t mon[TROCA_MON];
  uint32_t crc;
};

enum DueloEstado : uint8_t {
  DU_FORA = 0,
  DU_TIME,       // trocando os times
  DU_BATALHA,
  DU_FIM,
};
// por que acabou
enum : uint8_t {
  DM_NADA = 0,
  DM_SAI,          // eu sai
  DM_ELE_SAIU,
  DM_SUMIU,        // o outro parou de responder
  DM_DESSINCRONIA,
  DM_FIM,          // o combate acabou normalmente (quem venceu, diz a tela)
};

uint32_t dueloHashTime(const uint8_t mons[][TROCA_MON], uint8_t n, uint32_t sessao);
// A semente do dado, igual nas duas: sai da sessao.
uint32_t dueloSemente(uint32_t sessao);

struct DueloGanchos {
  void *ctx = nullptr;
  void (*enviar)(void *ctx, const uint8_t *mac, const uint8_t *buf, int n) = nullptr;
};

class Duelo {
 public:
  DueloGanchos g;
  DueloEstado estado = DU_FORA;
  uint8_t motivo = DM_NADA;
  uint8_t eu[6] = { 0 }, outro[6] = { 0 };
  uint32_t sessao = 0;
  bool anfitriao = false;   // quem convidou: o lado 0
  uint8_t regras = 0;

  // os times: o meu (escolhido aqui) e o dele (chegando pelo ar)
  uint8_t n = 0;                          // quantos cada um leva
  uint8_t meu[DUELO_MAX][TROCA_MON];
  bool tenhoMeu = false;
  uint8_t dele[DUELO_MAX][TROCA_MON];
  uint8_t chegou = 0;                     // bit i = tenho o dele[i]
  bool eleTemMeu = false;                 // ele confirmou o meu time inteiro

  // Pareados pela troca: comeca a fase do time (ainda sem o meu).
  void comeca(const uint8_t meuMac[6], const uint8_t outroMac[6], uint32_t s,
              bool souAnfitriao, uint8_t r);
  // O jogador escolheu os seus (n deles, na ordem em que vao sair).
  void meuTime(const uint8_t mons[][TROCA_MON], uint8_t quantos);
  bool tenhoDele() const { return n && chegou == (uint8_t)((1u << n) - 1); }
  // As duas tem os dois times: pode comecar o combate.
  bool prontos() const { return estado == DU_BATALHA; }
  // Lado do combate de cada um (o meu e o dele), canonico.
  uint8_t meuLado() const { return anfitriao ? 0 : 1; }

  // Ponto de decisao k (comeca em 0 e cresce de um em um). `hash` = btHash
  // do combate NESTE ponto, para conferir com o do outro.
  void decide(const DueloDecisao &d, uint32_t hash);
  bool jaDecidi() const { return temMinha; }
  bool temAsDuas() const { return estado == DU_BATALHA && temMinha && temDele; }
  // As duas do ponto atual, por LADO do combate (0 = do anfitriao).
  DueloDecisao doLado(uint8_t lado) const {
    return lado == meuLado() ? minha : dela;
  }
  // Consumidas as duas: passa ao ponto seguinte.
  void avanca();
  uint16_t ponto() const { return k; }

  void acabou();      // o combate terminou normalmente
  void sair();        // o jogador foi embora (desiste e avisa)
  void recebe(const uint8_t *buf, int len, uint32_t agora);
  void tick(uint32_t agora);

 private:
  uint16_t k = 0;
  DueloDecisao minha, dela;
  uint32_t hashMinha = 0, hashDela = 0;
  bool temMinha = false, temDele = false;
  // a do ponto anterior, para quem ficou um atras (nao ouviu a minha)
  DueloDecisao anterior;
  uint32_t hashAnterior = 0;
  bool temAnterior = false;
  uint32_t agoraUltimo = 0, ultimoDele = 0, ultimoEnvio = 0, ultimoVivo = 0;
  uint32_t saiEm = 0;
  uint8_t proximoTime = 0;
  void manda(uint8_t tipo, uint8_t idx, uint16_t kk, uint32_t hash,
             const DueloDecisao *d, const uint8_t *mon);
  void confere();
};
