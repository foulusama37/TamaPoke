// Protocolo da batalha entre placas: ver duelo.h. Nada de Arduino aqui.
#include "duelo.h"

static_assert(sizeof(DueloPacote) <= 250, "ESP-NOW manda no maximo 250 bytes");

#define DUELO_REENVIO_MS 250
#define DUELO_VIVO_MS 1000
// Pensar num golpe pode levar o tempo que for, mas o outro manda VIVO a
// cada segundo: 20 s de silencio e a placa dele que se foi (desligou,
// afastou-se). O combate nao vale nada, entao nada se perde.
#define DUELO_SUMIU_MS 20000
#define DUELO_SAI_ECO_MS 1500

static bool igualMac(const uint8_t *a, const uint8_t *b) { return memcmp(a, b, 6) == 0; }

uint32_t dueloHashTime(const uint8_t mons[][TROCA_MON], uint8_t n, uint32_t sessao) {
  uint32_t h = 2166136261u ^ sessao ^ ((uint32_t)n << 24);
  for (uint8_t i = 0; i < n; i++)
    for (int j = 0; j < TROCA_MON; j++) h = (h ^ mons[i][j]) * 16777619u;
  return h ? h : 1;
}

uint32_t dueloSemente(uint32_t sessao) {
  uint32_t x = sessao * 2654435761u + 0x7F4A7C15u;
  x ^= x >> 15;
  x *= 2246822519u;
  x ^= x >> 13;
  return x ? x : 1;
}

void Duelo::comeca(const uint8_t meuMac[6], const uint8_t outroMac[6], uint32_t s,
                   bool souAnfitriao, uint8_t r) {
  DueloGanchos gg = g;
  *this = Duelo();
  g = gg;            // os ganchos ficam
  memcpy(eu, meuMac, 6);
  memcpy(outro, outroMac, 6);
  sessao = s;
  anfitriao = souAnfitriao;
  regras = r;
  n = duTamanho(r);
  estado = DU_TIME;
}

void Duelo::meuTime(const uint8_t mons[][TROCA_MON], uint8_t quantos) {
  if (estado != DU_TIME || tenhoMeu || quantos != n) return;
  for (uint8_t i = 0; i < n; i++) memcpy(meu[i], mons[i], TROCA_MON);
  tenhoMeu = true;
  ultimoEnvio = 0;
}

void Duelo::manda(uint8_t tipo, uint8_t idx, uint16_t kk, uint32_t hash,
                  const DueloDecisao *d, const uint8_t *mon) {
  if (!g.enviar) return;
  DueloPacote p;
  memset((void *)&p, 0, sizeof(p));   // zera tambem o que o crc cobre
  p.magia = DUELO_MAGIA;
  p.ver = DUELO_VER;
  p.tipo = tipo;
  p.idx = idx;
  p.n = n;
  memcpy(p.de, eu, 6);
  memcpy(p.para, outro, 6);
  p.sessao = sessao;
  p.k = kk;
  p.hash = hash;
  if (d) p.dec = *d;
  if (mon) memcpy(p.mon, mon, TROCA_MON);
  p.crc = trocaCrc(&p, (int)offsetof(DueloPacote, crc));
  g.enviar(g.ctx, outro, (const uint8_t *)&p, (int)sizeof(p));
}

void Duelo::decide(const DueloDecisao &d, uint32_t hash) {
  if (estado != DU_BATALHA || temMinha) return;
  minha = d;
  hashMinha = hash;
  temMinha = true;
  manda(DP_DECISAO, 0, k, hashMinha, &minha, nullptr);
  ultimoEnvio = agoraUltimo;
  confere();
}

void Duelo::confere() {
  if (temMinha && temDele && hashMinha != hashDela) {
    estado = DU_FIM;
    motivo = DM_DESSINCRONIA;
  }
}

void Duelo::avanca() {
  if (!temAsDuas()) return;
  anterior = minha;
  hashAnterior = hashMinha;
  temAnterior = true;
  temMinha = temDele = false;
  k++;
}

void Duelo::acabou() {
  if (estado == DU_BATALHA) {
    estado = DU_FIM;
    motivo = DM_FIM;
  }
}

void Duelo::sair() {
  if (estado == DU_FORA) return;
  if (estado != DU_FIM) {
    estado = DU_FIM;
    motivo = DM_SAI;
  }
  saiEm = agoraUltimo ? agoraUltimo : 1;
  manda(DP_SAI, 0, k, 0, nullptr, nullptr);
}

void Duelo::recebe(const uint8_t *buf, int len, uint32_t agora) {
  agoraUltimo = agora;
  if (estado == DU_FORA || len != (int)sizeof(DueloPacote)) return;
  DueloPacote p;
  memcpy(&p, buf, sizeof(p));
  if (p.magia != DUELO_MAGIA || p.ver != DUELO_VER) return;
  if (p.crc != trocaCrc(&p, (int)offsetof(DueloPacote, crc))) return;
  if (!igualMac(p.de, outro) || !igualMac(p.para, eu) || p.sessao != sessao) return;
  ultimoDele = agora;

  if (p.tipo == DP_SAI) {
    if (estado != DU_FIM) {
      estado = DU_FIM;
      motivo = DM_ELE_SAIU;
    }
    return;
  }
  if (estado == DU_FIM) {
    // Ja acabou aqui. Se fui eu que sai, ele pode nao ter ouvido.
    if (motivo == DM_SAI) manda(DP_SAI, 0, k, 0, nullptr, nullptr);
    // Acabou (ou divergimos) com a decisao que fechou: ele so chega ao
    // mesmo fim com a minha, e a primeira pode ter caido. Sem isto ele
    // ficava esperando ate me dar por sumido.
    if (p.tipo == DP_DECISAO) {
      if (p.k == k && temMinha) manda(DP_DECISAO, 0, k, hashMinha, &minha, nullptr);
      else if (p.k + 1 == k && temAnterior)
        manda(DP_DECISAO, 0, (uint16_t)(k - 1), hashAnterior, &anterior, nullptr);
    }
    return;
  }

  switch (p.tipo) {
    case DP_TIME:
      if (p.n != n || p.idx >= n) break;
      if (estado == DU_TIME) {
        memcpy(dele[p.idx], p.mon, TROCA_MON);
        chegou |= (uint8_t)(1u << p.idx);
      }
      // Tenho o dele inteiro: confirmo (de novo, se ele ainda manda).
      if (tenhoDele()) manda(DP_TIMEOK, 0, 0, dueloHashTime(dele, n, sessao), nullptr, nullptr);
      break;
    case DP_TIMEOK:
      if (tenhoMeu && p.hash == dueloHashTime(meu, n, sessao)) eleTemMeu = true;
      break;
    case DP_DECISAO:
      if (estado != DU_BATALHA) {
        // Ele ja comecou: entao tem o meu time e eu, se tenho o dele, posso
        // comecar tambem (o TIMEOK dele pode ter caido).
        if (estado == DU_TIME && tenhoMeu && tenhoDele()) {
          eleTemMeu = true;
          estado = DU_BATALHA;
        } else {
          break;
        }
      }
      if (p.k == k) {
        if (!temDele) {
          dela = p.dec;
          hashDela = p.hash;
          temDele = true;
          confere();
        }
      } else if (p.k + 1 == k && temAnterior) {
        // ficou um atras: nao ouviu a minha do ponto anterior
        manda(DP_DECISAO, 0, (uint16_t)(k - 1), hashAnterior, &anterior, nullptr);
      }
      break;
    default:
      break;
  }
  if (estado == DU_TIME && tenhoMeu && tenhoDele() && eleTemMeu) estado = DU_BATALHA;
}

void Duelo::tick(uint32_t agora) {
  agoraUltimo = agora;
  if (estado == DU_FORA) return;
  if (estado == DU_FIM) {
    // Fui eu que sai: repito o aviso um pouco, que o primeiro pode cair.
    if (motivo == DM_SAI && saiEm && agora - saiEm < DUELO_SAI_ECO_MS &&
        agora - ultimoEnvio >= DUELO_REENVIO_MS) {
      ultimoEnvio = agora;
      manda(DP_SAI, 0, k, 0, nullptr, nullptr);
    }
    return;
  }
  if (ultimoDele == 0) ultimoDele = agora;
  if (agora - ultimoDele > DUELO_SUMIU_MS) {
    estado = DU_FIM;
    motivo = DM_SUMIU;
    return;
  }
  if (estado == DU_TIME && tenhoMeu && tenhoDele() && eleTemMeu) estado = DU_BATALHA;
  if (agora - ultimoEnvio >= DUELO_REENVIO_MS) {
    ultimoEnvio = agora;
    if (estado == DU_TIME) {
      // os meus, um por vez, ate ele confirmar; e a confirmacao do dele
      if (tenhoMeu && !eleTemMeu) {
        manda(DP_TIME, proximoTime, 0, 0, nullptr, meu[proximoTime]);
        proximoTime = (uint8_t)((proximoTime + 1) % n);
      }
      if (tenhoDele()) manda(DP_TIMEOK, 0, 0, dueloHashTime(dele, n, sessao), nullptr, nullptr);
    } else if (temMinha && !temDele) {
      manda(DP_DECISAO, 0, k, hashMinha, &minha, nullptr);
    }
  }
  if (agora - ultimoVivo >= DUELO_VIVO_MS) {
    ultimoVivo = agora;
    manda(DP_VIVO, 0, k, 0, nullptr, nullptr);
  }
}
