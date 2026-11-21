#pragma once
// TROCAS entre duas placas (ESP-NOW). Este arquivo e o PROTOCOLO, puro: nao
// sabe de radio, de SD nem do Pet. Quem usa da os ganchos (enviar, preparar,
// aplicar, descartar) e chama recebe() / tick(). Assim os testes do PC ligam
// duas maquinas destas por uma fila que perde pacotes e conferem que nenhum
// bicho some e nenhum se duplica.
//
// O fluxo (2026-11-13, combinado com o dono):
//   PROCURA   -- as duas anunciam (OLA) quem sao e o titular
//   CONVITE   -- uma toca na outra; a outra aceita (SIM)
//   ESCOLHA   -- cada uma oferece um bicho do time (OFERTA), ve o do outro
//   ACEITO    -- as duas confirmam ESTE par (hash dos dois bichos)
//   PREPARADO -- cada uma grava o DIARIO no SD (quem sai, quem chega) e so
//                entao manda PRONTO
//   FEITO     -- com o proprio diario gravado E o PRONTO do outro, aplica.
//
// Por que e seguro: so se aplica tendo o PRONTO do outro, e o PRONTO so sai
// depois do diario dele gravado. Entao, se eu apliquei, ele tem diario e vai
// aplicar quando nos falarmos de novo (mesmo depois de desligar). E quem ainda
// nao preparou pode cancelar; quem preparou NAO pode, so o outro (que nao
// preparou) cancela, e ai nenhum dos dois aplica.
//
// Depois de reiniciar com um diario PREPARADO, retoma(): fica mandando PRONTO
// ao outro. Ele responde PRONTO (preparado tambem: os dois aplicam), FEITO (ja
// aplicou: eu aplico) ou CANCELA (nunca preparou: eu descarto).
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "party.h"

#define TROCA_MAGIA 0x52545054u   // "TPTR"
#define TROCA_VER 1
#define TROCA_MON 52              // sizeof(StoredMon), conferido em troca.cpp
#define TROCA_VIZINHOS 6
// no byte das regras (rsv do pacote): quem manda e o anfitriao
#define TROCA_SOU_ANFITRIAO 0x40

enum : uint8_t {
  TP_OLA = 1,     // broadcast: estou na tela de troca (titular, aparencia)
  TP_CONVITE,     // quero trocar com voce (sessao nova)
  TP_SIM,         // aceito o convite
  TP_OFERTA,      // ofereco este bicho (mon)
  TP_ACEITO,      // aceito o par (hashMeu = o meu, hashSeu = o seu)
  TP_PRONTO,      // diario gravado
  TP_FEITO,       // apliquei
  TP_CANCELA,     // desisti (ou: nao conheco essa sessao)
};

// Copiado byte a byte: as duas placas rodam o mesmo firmware. A versao
// recusa um pacote de um firmware com outro formato.
struct TrocaPacote {
  uint32_t magia;
  uint8_t ver, tipo, aparencia, forma;   // aparencia: bit0 shiny, bit1 menina
  uint8_t de[6], para[6];
  uint32_t sessao;
  uint32_t hashMeu, hashSeu;
  int16_t titular;
  uint8_t nivel, rsv;
  uint8_t mon[TROCA_MON];
  uint32_t crc;
};

// O que vai para o SD antes de aplicar. `chega` e o bicho como ENTRA aqui
// (ja evoluido pela troca, se for o caso): calcula-se uma vez, no preparar,
// para que reaplicar depois de um reinicio de exatamente o mesmo bicho.
enum : uint8_t { TD_PREPARADO = 1, TD_APLICADO = 2, TD_DESCARTADO = 3 };
struct TrocaDiario {
  uint32_t magia;
  uint32_t sessao;
  uint8_t ver, estado, slot, rsv;        // slot: de onde sai o meu (time)
  uint8_t outro[6];
  uint8_t rsv2[2];
  uint8_t sai[TROCA_MON];                // o meu, como estava
  uint8_t veio[TROCA_MON];               // o dele, como chegou pelo ar
  uint8_t chega[TROCA_MON];              // o dele, como fica aqui
  uint32_t crc;
};

uint32_t trocaCrc(const void *p, int n);
uint32_t trocaHash(const uint8_t *mon, uint32_t sessao);
void trocaSelaDiario(TrocaDiario &d);
bool trocaDiarioValido(const TrocaDiario &d);

enum TrocaEstado : uint8_t {
  TE_FORA = 0,     // radio desligado / tela fechada
  TE_PROCURA,      // anunciando e listando vizinhos
  TE_CONVIDEI,     // mandei convite, esperando SIM
  TE_CONVIDADO,    // recebi convite, esperando o jogador dizer sim
  TE_ESCOLHA,      // pareados: escolhendo o bicho
  TE_ACEITEI,      // confirmei o par, esperando o ACEITO dele
  TE_PREPARADO,    // diario gravado, esperando o PRONTO dele
  TE_FEITO,        // aplicado
  TE_CANCELADO,    // alguem desistiu (ou o SD nao gravou)
  // Pareados para BATALHAR (modo != 0): daqui em diante quem fala e o
  // Duelo (duelo.h); a troca so responde a um CONVITE repetido.
  TE_DUELO,
};

struct TrocaVizinho {
  uint8_t mac[6];
  int16_t titular;
  uint8_t forma, aparencia, nivel;
  uint32_t visto;   // ms do ultimo OLA
};

struct TrocaGanchos {
  void *ctx = nullptr;
  // manda n bytes a mac (FF..FF = todos)
  void (*enviar)(void *ctx, const uint8_t *mac, const uint8_t *buf, int n) = nullptr;
  // preenche d.chega e grava o diario PREPARADO; false = nao gravou (cancela)
  bool (*preparar)(void *ctx, TrocaDiario &d) = nullptr;
  // aplica no save e marca o diario APLICADO; tem de ser idempotente
  bool (*aplicar)(void *ctx, const TrocaDiario &d) = nullptr;
  // o outro nunca preparou: marca o diario DESCARTADO (nada muda no save)
  void (*descartar)(void *ctx, const TrocaDiario &d) = nullptr;
};

class Troca {
 public:
  TrocaGanchos g;
  TrocaEstado estado = TE_FORA;
  uint8_t eu[6] = { 0 };
  uint8_t outro[6] = { 0 };
  uint32_t sessao = 0;
  // o que eu anuncio no OLA
  int16_t titular = 0;
  uint8_t forma = 0, aparencia = 0, nivel = 0;
  // O CONVITE pode ser para BATALHAR: `modo` sao as regras (duelo.h,
  // DR_*), 0 = troca. Quem convidou e o `anfitriao` (lado 0 do combate)
  // e e dele que valem as regras. Se os dois convidarem ao mesmo tempo,
  // fica a sessao menor e com ela o convite (e as regras) de quem a criou.
  uint8_t modo = 0;
  bool anfitriao = false;

  TrocaVizinho viz[TROCA_VIZINHOS];
  uint8_t nViz = 0;

  bool tenhoMeu = false, tenhoDele = false;
  uint8_t slotMeu = 0;
  uint8_t meu[TROCA_MON], dele[TROCA_MON];
  bool eleAceitou = false;   // e para ESTE par
  bool elePronto = false;
  TrocaDiario diario;        // valido de PREPARADO em diante

  // por que cancelou (para a tela): 0 o outro desistiu, 1 eu, 2 o SD
  // nao gravou, 3 o outro sumiu antes de preparar, 4 descartada
  uint8_t motivo = 0;

  void comeca(const uint8_t mac[6]);
  void para();               // fecha a tela: so antes de preparar
  void convidar(const uint8_t mac[6], uint32_t sessaoNova, uint8_t modoNovo = 0);
  // Pareados para batalhar: a troca se cala e o Duelo assume.
  bool podeDuelar() const { return estado == TE_ESCOLHA && modo != 0; }
  void entregaDuelo() { if (podeDuelar()) estado = TE_DUELO; }
  void aceitarConvite();
  void oferecer(uint8_t slot, const uint8_t mon[TROCA_MON]);
  bool podeAceitar() const { return estado == TE_ESCOLHA && tenhoMeu && tenhoDele; }
  void aceitar();
  void cancelar();           // o jogador desistiu (so antes de preparar)
  bool podeCancelar() const { return estado < TE_PREPARADO || estado == TE_CANCELADO; }
  // retoma um diario PREPARADO lido do SD apos um reinicio
  void retoma(const TrocaDiario &d);
  void recebe(const uint8_t *buf, int n, uint32_t agora);
  void tick(uint32_t agora);

  // Sessoes que eu ja terminei (aplicadas ou descartadas): para responder
  // a quem volta perguntando. Quem usa enche ao carregar do SD.
  uint32_t feitas[8] = { 0 };
  uint8_t feitasEstado[8] = { 0 };
  void lembra(uint32_t s, uint8_t est);

 private:
  uint32_t ultimoEnvio = 0;
  uint32_t ultimoOla = 0;
  uint32_t ultimoDele = 0;   // ultimo pacote valido do outro
  uint32_t agoraUltimo = 0;  // o relogio do ultimo recebe()/tick()
  uint32_t feitoEm = 0;
  void manda(uint8_t tipo, const uint8_t *para);
  void mandaCom(uint8_t tipo, const uint8_t *para, uint32_t s);
  void resetaPar();
  void tentaPreparar();
  void aplica();
  bool deleEsperado(const TrocaPacote &p) const;
  int8_t jaTermineiSessao(uint32_t s) const;
};
