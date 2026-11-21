// Protocolo das trocas: ver troca.h. Nada de Arduino aqui de proposito.
#include "troca.h"

static_assert(sizeof(StoredMon) == TROCA_MON,
              "TROCA_MON tem de ser o tamanho do StoredMon");
static_assert(sizeof(TrocaPacote) <= 250, "ESP-NOW manda no maximo 250 bytes");

static const uint8_t TODOS[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
#define TROCA_REENVIO_MS 300
#define TROCA_OLA_MS 500
#define TROCA_VIZ_VELHO_MS 3000
#define TROCA_SUMIU_MS 8000
#define TROCA_FEITO_ECO_MS 3000

uint32_t trocaCrc(const void *p, int n) {
  const uint8_t *b = (const uint8_t *)p;
  uint32_t c = 0xFFFFFFFFu;
  for (int i = 0; i < n; i++) {
    c ^= b[i];
    for (int k = 0; k < 8; k++) c = (c >> 1) ^ (0xEDB88320u & (0u - (c & 1)));
  }
  return ~c;
}

uint32_t trocaHash(const uint8_t *mon, uint32_t sessao) {
  uint32_t h = 2166136261u ^ sessao;
  for (int i = 0; i < TROCA_MON; i++) h = (h ^ mon[i]) * 16777619u;
  return h ? h : 1;   // 0 fica para "nao tenho"
}

void trocaSelaDiario(TrocaDiario &d) {
  d.magia = TROCA_MAGIA;
  d.ver = TROCA_VER;
  d.crc = trocaCrc(&d, (int)offsetof(TrocaDiario, crc));
}

bool trocaDiarioValido(const TrocaDiario &d) {
  return d.magia == TROCA_MAGIA && d.ver == TROCA_VER &&
         d.crc == trocaCrc(&d, (int)offsetof(TrocaDiario, crc));
}

static bool igualMac(const uint8_t *a, const uint8_t *b) {
  return memcmp(a, b, 6) == 0;
}

void Troca::resetaPar() {
  tenhoMeu = tenhoDele = false;
  eleAceitou = elePronto = false;
  memset(meu, 0, sizeof(meu));
  memset(dele, 0, sizeof(dele));
}

void Troca::comeca(const uint8_t mac[6]) {
  memcpy(eu, mac, 6);
  // Uma troca PREPARADA nao se larga por reabrir a tela: continua.
  if (estado == TE_PREPARADO) return;
  estado = TE_PROCURA;
  nViz = 0;
  sessao = 0;
  motivo = 0;
  memset(outro, 0, 6);
  resetaPar();
}

void Troca::para() {
  if (estado == TE_PREPARADO) return;   // o diario segue valendo
  if (estado >= TE_CONVIDEI && estado <= TE_ACEITEI) manda(TP_CANCELA, outro);
  estado = TE_FORA;
}

void Troca::convidar(const uint8_t mac[6], uint32_t sessaoNova, uint8_t modoNovo) {
  if (estado != TE_PROCURA) return;
  memcpy(outro, mac, 6);
  modo = modoNovo;
  anfitriao = true;
  sessao = sessaoNova ? sessaoNova : 1;
  resetaPar();
  estado = TE_CONVIDEI;
  ultimoEnvio = 0;
  ultimoDele = agoraUltimo;
  manda(TP_CONVITE, outro);
}

void Troca::aceitarConvite() {
  if (estado != TE_CONVIDADO) return;
  estado = TE_ESCOLHA;
  ultimoDele = agoraUltimo;
  manda(TP_SIM, outro);
}

void Troca::oferecer(uint8_t slot, const uint8_t mon[TROCA_MON]) {
  // So ANTES de aceitar. Depois de aceito o outro pode ja ter gravado o
  // diario com este par; mudar aqui deixava as duas presas (ele esperando
  // PRONTO do par velho, eu oferecendo outro). Depois de aceitar: cancelar.
  if (estado != TE_ESCOLHA) return;
  memcpy(meu, mon, TROCA_MON);
  slotMeu = slot;
  tenhoMeu = true;
  eleAceitou = false;   // o aceite dele era do par velho
  manda(TP_OFERTA, outro);
}

void Troca::aceitar() {
  if (!podeAceitar()) return;
  estado = TE_ACEITEI;
  manda(TP_ACEITO, outro);
  tentaPreparar();
}

void Troca::cancelar() {
  if (!podeCancelar()) return;
  if (estado >= TE_CONVIDEI && estado <= TE_ACEITEI) manda(TP_CANCELA, outro);
  if (estado != TE_FORA) {
    estado = TE_CANCELADO;
    motivo = 1;
  }
}

void Troca::retoma(const TrocaDiario &d) {
  if (!trocaDiarioValido(d) || d.estado != TD_PREPARADO) return;
  diario = d;
  sessao = d.sessao;
  memcpy(outro, d.outro, 6);
  memcpy(meu, d.sai, TROCA_MON);
  memcpy(dele, d.veio, TROCA_MON);
  slotMeu = d.slot;
  tenhoMeu = tenhoDele = true;
  eleAceitou = true;
  elePronto = false;
  estado = TE_PREPARADO;
}

void Troca::lembra(uint32_t s, uint8_t est) {
  for (int i = 0; i < 8; i++)
    if (feitas[i] == s) { feitasEstado[i] = est; return; }
  memmove(feitas + 1, feitas, 7 * sizeof(feitas[0]));
  memmove(feitasEstado + 1, feitasEstado, 7);
  feitas[0] = s;
  feitasEstado[0] = est;
}

int8_t Troca::jaTermineiSessao(uint32_t s) const {
  for (int i = 0; i < 8; i++)
    if (s && feitas[i] == s) return (int8_t)i;
  return -1;
}

bool Troca::deleEsperado(const TrocaPacote &p) const {
  return tenhoMeu && tenhoDele && p.hashMeu == trocaHash(dele, sessao) &&
         p.hashSeu == trocaHash(meu, sessao);
}

void Troca::mandaCom(uint8_t tipo, const uint8_t *para, uint32_t s) {
  if (!g.enviar) return;
  TrocaPacote p;
  memset(&p, 0, sizeof(p));
  p.magia = TROCA_MAGIA;
  p.ver = TROCA_VER;
  p.tipo = tipo;
  p.aparencia = aparencia;
  p.forma = forma;
  memcpy(p.de, eu, 6);
  memcpy(p.para, para, 6);
  p.sessao = s;
  p.hashMeu = tenhoMeu ? trocaHash(meu, s) : 0;
  p.hashSeu = tenhoDele ? trocaHash(dele, s) : 0;
  p.titular = titular;
  p.nivel = nivel;
  // As regras viajam em todos, e com elas se EU sou o anfitriao: o SIM de
  // quem cruzou o convite e ficou com o dele tem de me dizer que perdi,
  // senao, com sessoes iguais, as duas ficavam anfitrias.
  p.rsv = (uint8_t)(modo | (anfitriao ? TROCA_SOU_ANFITRIAO : 0));
  if (tipo == TP_OFERTA) memcpy(p.mon, meu, TROCA_MON);
  p.crc = trocaCrc(&p, (int)offsetof(TrocaPacote, crc));
  g.enviar(g.ctx, para, (const uint8_t *)&p, (int)sizeof(p));
}

void Troca::manda(uint8_t tipo, const uint8_t *para) {
  mandaCom(tipo, para, sessao);
}

void Troca::tentaPreparar() {
  if (estado != TE_ACEITEI || !eleAceitou) return;
  memset(&diario, 0, sizeof(diario));
  diario.sessao = sessao;
  diario.estado = TD_PREPARADO;
  diario.slot = slotMeu;
  memcpy(diario.outro, outro, 6);
  memcpy(diario.sai, meu, TROCA_MON);
  memcpy(diario.veio, dele, TROCA_MON);
  memcpy(diario.chega, dele, TROCA_MON);   // o gancho evolui, se for o caso
  bool ok = g.preparar && g.preparar(g.ctx, diario);
  if (!ok) {
    // Sem diario nao ha troca: quem nao preparou pode desistir, e o outro
    // descarta o dele ao saber.
    estado = TE_CANCELADO;
    motivo = 2;
    manda(TP_CANCELA, outro);
    return;
  }
  estado = TE_PREPARADO;
  manda(TP_PRONTO, outro);
  if (elePronto) aplica();
}

void Troca::aplica() {
  if (estado != TE_PREPARADO) return;
  if (!g.aplicar || !g.aplicar(g.ctx, diario)) return;   // tenta de novo depois
  estado = TE_FEITO;
  feitoEm = agoraUltimo;
  lembra(sessao, TD_APLICADO);
  manda(TP_FEITO, outro);
}

void Troca::recebe(const uint8_t *buf, int n, uint32_t agora) {
  agoraUltimo = agora;
  if (estado == TE_FORA || n != (int)sizeof(TrocaPacote)) return;
  TrocaPacote p;
  memcpy(&p, buf, sizeof(p));
  if (p.magia != TROCA_MAGIA || p.ver != TROCA_VER) return;
  if (p.crc != trocaCrc(&p, (int)offsetof(TrocaPacote, crc))) return;
  if (igualMac(p.de, eu)) return;
  if (!igualMac(p.para, eu) && !igualMac(p.para, TODOS)) return;

  if (p.tipo == TP_OLA) {
    int i = 0;
    while (i < nViz && !igualMac(viz[i].mac, p.de)) i++;
    if (i == nViz) {
      if (nViz >= TROCA_VIZINHOS) return;
      nViz++;
    }
    memcpy(viz[i].mac, p.de, 6);
    viz[i].titular = p.titular;
    viz[i].forma = p.forma;
    viz[i].aparencia = p.aparencia;
    viz[i].nivel = p.nivel;
    viz[i].visto = agora;
    return;
  }

  const bool doOutro = igualMac(p.de, outro);
  const bool minha = doOutro && p.sessao == sessao && sessao != 0;

  if (p.tipo == TP_CONVITE) {
    if (estado == TE_PROCURA) {
      memcpy(outro, p.de, 6);
      sessao = p.sessao;
      modo = (uint8_t)(p.rsv & ~TROCA_SOU_ANFITRIAO);
      anfitriao = false;
      resetaPar();
      estado = TE_CONVIDADO;
      ultimoDele = agora;
    } else if (estado == TE_CONVIDEI && doOutro) {
      // Os dois tocaram um no outro ao mesmo tempo: fica a menor sessao,
      // que as duas calculam igual, e ja estamos pareados.
      // Com a sessao vem o convite: se a dele ganha, valem as regras dele
      // e ele e o anfitriao. Empate de sessao: decide o MAC menor.
      if (p.sessao < sessao || (p.sessao == sessao && memcmp(p.de, eu, 6) < 0)) {
        sessao = p.sessao;
        modo = (uint8_t)(p.rsv & ~TROCA_SOU_ANFITRIAO);
        anfitriao = false;
      }
      estado = TE_ESCOLHA;
      ultimoDele = agora;
      manda(TP_SIM, outro);
    } else if (minha && ((estado >= TE_ESCOLHA && estado <= TE_ACEITEI) ||
                         estado == TE_DUELO)) {
      manda(TP_SIM, outro);   // ele nao ouviu o meu SIM
    } else if (!minha && !(estado == TE_CONVIDADO && doOutro)) {
      mandaCom(TP_CANCELA, p.de, p.sessao);   // ocupado com outra
    }
    return;
  }

  // PRONTO de uma sessao que nao e a de agora: alguem retomando depois de
  // reiniciar. Ou eu ja terminei essa (respondo o que fiz) ou nunca
  // preparei (CANCELA: ele descarta).
  if (!minha) {
    if (p.tipo == TP_PRONTO) {
      int8_t k = jaTermineiSessao(p.sessao);
      if (k >= 0 && feitasEstado[k] == TD_APLICADO) mandaCom(TP_FEITO, p.de, p.sessao);
      else mandaCom(TP_CANCELA, p.de, p.sessao);
    }
    // SIM ao meu convite (com a sessao que ele adotou, se cruzou)
    if (p.tipo == TP_SIM && estado == TE_CONVIDEI && doOutro) {
      // cruzamos e ele ficou com a sessao DELE: o convite dele vale
      if (p.sessao != sessao || (p.rsv & TROCA_SOU_ANFITRIAO)) {
        modo = (uint8_t)(p.rsv & ~TROCA_SOU_ANFITRIAO);
        anfitriao = false;
      }
      sessao = p.sessao;
      estado = TE_ESCOLHA;
      ultimoDele = agora;
    }
    return;
  }

  ultimoDele = agora;
  // Desisti, mas ele insiste: repete o CANCELA (o primeiro pode ter caido),
  // senao ele, se ja preparou, fica esperando para sempre.
  if (estado == TE_CANCELADO) {
    if (p.tipo != TP_CANCELA) manda(TP_CANCELA, outro);
    return;
  }
  switch (p.tipo) {
    case TP_SIM:
      if (estado == TE_CONVIDEI) {
        // cruzamos com a MESMA sessao e ele ficou anfitriao (MAC menor)
        if (p.rsv & TROCA_SOU_ANFITRIAO) {
          modo = (uint8_t)(p.rsv & ~TROCA_SOU_ANFITRIAO);
          anfitriao = false;
        }
        estado = TE_ESCOLHA;
      }
      break;
    case TP_OFERTA:
      if (estado != TE_ESCOLHA && estado != TE_ACEITEI) break;
      if (!tenhoDele || memcmp(dele, p.mon, TROCA_MON) != 0) {
        memcpy(dele, p.mon, TROCA_MON);
        tenhoDele = true;
        eleAceitou = false;
        if (estado == TE_ACEITEI) estado = TE_ESCOLHA;   // mudou: reconfirmar
      }
      break;
    case TP_ACEITO:
      if (estado == TE_ESCOLHA || estado == TE_ACEITEI) {
        eleAceitou = deleEsperado(p);
        tentaPreparar();
      } else if (estado == TE_PREPARADO || estado == TE_FEITO) {
        manda(TP_PRONTO, outro);   // ele nao ouviu o meu
      }
      break;
    case TP_PRONTO:
      if (estado == TE_ACEITEI) {
        if (deleEsperado(p)) {
          eleAceitou = elePronto = true;
          tentaPreparar();
        }
      } else if (estado == TE_PREPARADO) {
        elePronto = true;
        aplica();
      } else if (estado == TE_FEITO) {
        manda(TP_PRONTO, outro);
        manda(TP_FEITO, outro);
      }
      break;
    case TP_FEITO:
      // ele aplicou, entao ele tinha o meu PRONTO: eu estou preparado
      if (estado == TE_PREPARADO) {
        elePronto = true;
        aplica();
      }
      break;
    case TP_CANCELA:
      if (estado >= TE_CONVIDEI && estado <= TE_ACEITEI) {
        estado = TE_CANCELADO;
        motivo = 0;
      } else if (estado == TE_PREPARADO) {
        // Ele nunca preparou (ou desistiu antes): nao vai aplicar, e eu
        // so aplicaria com o PRONTO dele. Nada muda; o diario e descartado.
        if (g.descartar) g.descartar(g.ctx, diario);
        lembra(sessao, TD_DESCARTADO);
        estado = TE_CANCELADO;
        motivo = 4;
      }
      break;
  }
}

void Troca::tick(uint32_t agora) {
  agoraUltimo = agora;
  if (estado == TE_FORA || estado == TE_CANCELADO) return;
  // vizinhos que pararam de anunciar saem da lista
  for (int i = 0; i < nViz;) {
    if (agora - viz[i].visto > TROCA_VIZ_VELHO_MS) {
      viz[i] = viz[nViz - 1];
      nViz--;
    } else {
      i++;
    }
  }
  if (estado == TE_PROCURA || estado == TE_CONVIDADO || estado == TE_CONVIDEI) {
    if (agora - ultimoOla >= TROCA_OLA_MS) {
      ultimoOla = agora;
      mandaCom(TP_OLA, TODOS, 0);
    }
  }
  // Sumiu antes de preparar: desiste. Depois de preparar nao ha prazo (o
  // diario espera ate as placas se encontrarem de novo).
  if (estado >= TE_CONVIDEI && estado <= TE_ACEITEI &&
      agora - ultimoDele > TROCA_SUMIU_MS) {
    estado = TE_CANCELADO;
    motivo = 3;
    return;
  }
  if (agora - ultimoEnvio < TROCA_REENVIO_MS) return;
  ultimoEnvio = agora;
  switch (estado) {
    case TE_CONVIDEI: manda(TP_CONVITE, outro); break;
    case TE_ESCOLHA: manda(tenhoMeu ? TP_OFERTA : TP_SIM, outro); break;
    case TE_ACEITEI:
      manda(TP_OFERTA, outro);
      manda(TP_ACEITO, outro);
      break;
    case TE_PREPARADO: manda(TP_PRONTO, outro); break;
    case TE_FEITO:
      if (agora - feitoEm < TROCA_FEITO_ECO_MS) manda(TP_FEITO, outro);
      break;
    default: break;
  }
}
