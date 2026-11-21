// O time de cada lado na batalha entre placas (pvp.h) e o motor com banco
// DOS DOIS lados. Duas coisas que importam:
//   1. as duas placas montam os mesmos combatentes dos mesmos registros,
//      no nivel 50, sem ler nada do save;
//   2. um combate inteiro, com quedas, Ida e Volta, Rugido e Cola Dragao,
//      da o MESMO combate nas duas placas, hash a hash.
#include "framework.h"
#include "shim/Arduino.h"
#include "../pvp.h"
#include "../pet.h"
#include "../forms.h"
#include "../megas.h"

namespace {

uint16_t golpe(const char *n) {
  for (uint16_t i = 0; i < MOVE_COUNT; i++)
    if (!strcmp(MOVE_TBL[i].name, n)) return i;
  return MOVE_STRUGGLE;
}

uint16_t habil(const char *n) {
  for (uint16_t i = 0; i < ABIL_COUNT; i++)
    if (!strcmp(ABIL_TBL[i].name, n)) return i;
  return 0;
}

// Uma ficha como as do save: com os PS e os PP do dia a dia (feridos de
// proposito: no combate entre placas entra-se cheio).
StoredMon ficha(int16_t sp, uint16_t nivel, const char *g0 = nullptr,
                const char *g1 = nullptr, const char *g2 = nullptr,
                const char *g3 = nullptr) {
  StoredMon m;
  m.species = sp;
  m.exp = Pet::expForLevel(nivel);
  const char *gs[4] = { g0, g1, g2, g3 };
  if (g0) {
    m.flags |= SM_MOVES_PICKED;
    for (uint8_t i = 0; i < 4; i++) {
      m.moves[i] = gs[i] ? golpe(gs[i]) : 0;
      CHECK_MSG(!gs[i] || m.moves[i] != MOVE_STRUGGLE, "golpe com nome errado");
      m.ppLeft[i] = 1;
    }
  }
  m.hpLeft = 1;
  const uint16_t *a = ABIL_OF[sp];
  m.ability = a[0] ? a[0] : a[1];
  return m;
}

void montaLados(PvpLado l[2], const StoredMon *a, uint8_t na,
                const StoredMon *b, uint8_t nb) {
  uint8_t ra[DUELO_MAX][TROCA_MON], rb[DUELO_MAX][TROCA_MON];
  for (uint8_t i = 0; i < na; i++) memcpy(ra[i], &a[i], TROCA_MON);
  for (uint8_t i = 0; i < nb; i++) memcpy(rb[i], &b[i], TROCA_MON);
  CHECK(pvpMonta(l[0], ra, na));
  CHECK(pvpMonta(l[1], rb, nb));
}

}  // namespace

TEST(pvp, monta_no_nivel_50_com_o_que_e_dele) {
  StoredMon alto = ficha(59, 80, "FLAMETHROWER", "ROAR", nullptr, "TACKLE");
  alto.geneAtk = 200;
  alto.trAtk = 120;
  alto.flags |= SM_SHINY | SM_FEMALE | SM_GENDER_SET;
  alto.item = 5;
  Combatant c = pvpCombatente(alto);
  CHECK_EQ(c.level, 50);
  CHECK_EQ(c.hp, c.hpMax);
  CHECK_EQ(c.status, ST_NONE);
  CHECK(c.shiny);
  CHECK_EQ(c.sexo, 1);
  CHECK_EQ(c.item, 5);
  CHECK_EQ(c.ability, alto.ability);
  // os escolhidos, compactados, com os PP CHEIOS (a ficha tinha 1)
  CHECK_EQ(c.nMoves, 3);
  CHECK_EQ(c.moves[0], golpe("FLAMETHROWER"));
  CHECK_EQ(c.moves[1], golpe("ROAR"));
  CHECK_EQ(c.moves[2], golpe("TACKLE"));
  for (uint8_t i = 0; i < 3; i++) CHECK_EQ(c.usesLeft[i], movePP(c.moves[i]));
  // os genes e o treino contam
  StoredMon liso = alto;
  liso.geneAtk = 100;
  liso.trAtk = 0;
  CHECK_MSG(pvpCombatente(liso).atkStatRaw() < c.atkStatRaw(),
            "os genes e o treino do bicho nao entraram no combate");

  // Os NAO escolhidos sao os do SEU nivel: subir a 50 nao ensina nada.
  StoredMon baixo = ficha(1, 12);
  Combatant d = pvpCombatente(baixo);
  CHECK_EQ(d.level, 50);
  uint16_t e12[4] = { 0, 0, 0, 0 }, e50[4] = { 0, 0, 0, 0 };
  uint8_t n12 = movesAtLevel(1, 12, e12), n50 = movesAtLevel(1, 50, e50);
  CHECK_MSG(n12 != n50 || memcmp(e12, e50, sizeof e12) != 0,
            "premissa: no 50 o Bulbasaur saberia outros golpes");
  CHECK_EQ(d.nMoves, n12);
  for (uint8_t i = 0; i < n12; i++) {
    CHECK_EQ(d.moves[i], e12[i]);
    CHECK_EQ(d.usesLeft[i], movePP(e12[i]));
  }

  // A FORMA: o Meowth de Galar tem outros PS base que o de Kanto.
  uint8_t f = 0;
  for (uint8_t k = 1; k < 8 && !f; k++)
    if (formRow(52, k) >= 0 && hpAtLevelForm(52, 50, 0, k) != hpAtLevelForm(52, 50, 0, 0)) f = k;
  CHECK_MSG(f, "premissa: um Meowth de forma com outros PS");
  StoredMon mw = ficha(52, 30);
  mw.form = f;
  Combatant cm = pvpCombatente(mw);
  CHECK_EQ(cm.form, f);
  CHECK_EQ(cm.hpMax, hpAtLevelForm(52, 50, 0, f));

  // sem sexo, mesmo com o bit de femea
  StoredMon mag = ficha(81, 30);
  mag.flags |= SM_FEMALE;
  CHECK_EQ(pvpCombatente(mag).sexo, 2);

  // e um registro que nao serve nao monta
  PvpLado l;
  uint8_t r[1][TROCA_MON];
  StoredMon vazio;
  memcpy(r[0], &vazio, TROCA_MON);
  CHECK(!pvpMonta(l, r, 1));
}

TEST(pvp, rival_com_banco_cai_e_o_combate_segue_sem_banco_acaba) {
  StoredMon a[1] = { ficha(131, 50, "TACKLE") };
  StoredMon b[2] = { ficha(134, 50, "TACKLE"), ficha(135, 50, "TACKLE") };
  PvpLado l[2];
  montaLados(l, a, 1, b, 2);
  Battle bt;
  pvpComeca(bt, l);
  CHECK(bt.isTrainer);
  CHECK(!bt.playerReserves);
  CHECK(bt.foeReserves);
  bt.side[BT_FOE].hp = 0;
  bt.knockOut(BT_FOE);
  CHECK_MSG(!bt.over, "o rival ainda tinha banco");
  CHECK(!l[1].podeEntrar(0) || l[1].ativo == 0);
  CHECK(pvpEntra(bt, l, BT_FOE, 1, false));
  CHECK_EQ(bt.side[BT_FOE].dex, 135);
  CHECK_EQ(l[1].caidos(), 1);
  CHECK(!l[1].podeEntrar(0));           // o caido nao volta
  CHECK(!bt.foeReserves);
  bt.side[BT_FOE].hp = 0;
  bt.knockOut(BT_FOE);
  CHECK(bt.over);
  CHECK_EQ(bt.winner, BT_PLAYER);
}

TEST(pvp, relevo_guarda_ps_estado_pp_e_mega_e_desfaz_o_que_e_da_pista) {
  int fila = -1;
  for (uint8_t i = 0; i < MEGA_COUNT && fila < 0; i++)
    if (MEGA_TBL[i].fromForm == 0 && MEGA_TBL[i].ability) fila = i;
  CHECK(fila >= 0);
  int16_t sp = MEGA_TBL[fila].base;
  StoredMon a[2] = { ficha(sp, 50, "TACKLE", "SWORDSDANCE"), ficha(131, 50, "TACKLE") };
  StoredMon b[2] = { ficha(132, 50, "TRANSFORM"), ficha(134, 50, "TACKLE") };
  PvpLado l[2];
  montaLados(l, a, 2, b, 2);
  Battle bt;
  pvpComeca(bt, l);

  Combatant &yo = bt.side[BT_PLAYER];
  CHECK(yo.megaEvolve((uint8_t)fila));
  yo.hp = (uint16_t)(yo.hpMax / 3);
  yo.status = ST_SLEEP;
  yo.statusTurns = 2;
  yo.usesLeft[0] = 7;
  yo.ability = habil("Mold Breaker");     // um Skill Swap, digamos
  yo.stage[SG_ATK] = 2;
  const uint16_t hpYo = yo.hp;

  Combatant &el = bt.side[BT_FOE];
  el.transformInto(yo);
  CHECK(el.transformed);
  el.usesLeft[0] = 1;                     // gastou os PP COPIADOS
  el.status = ST_BADPOISON;
  el.statusTurns = 4;
  el.hp = (uint16_t)(el.hpMax - 10);
  const uint16_t hpEl = el.hp;

  CHECK(pvpEntra(bt, l, BT_PLAYER, 1, true));
  CHECK(pvpEntra(bt, l, BT_FOE, 1, true));

  const Combatant &g = l[0].mon[0];
  CHECK_MSG(g.isMega(), "a mega fica ate ao fim do combate");
  CHECK_EQ(g.hp, hpYo);
  CHECK_EQ(g.status, ST_SLEEP);
  CHECK_EQ(g.statusTurns, 2);             // o sono continua a contar
  CHECK_EQ(g.usesLeft[0], 7);
  CHECK_EQ(g.ability, MEGA_TBL[fila].ability);   // o Skill Swap desfaz-se
  CHECK_EQ(g.stage[SG_ATK], 0);

  const Combatant &d = l[1].mon[0];
  CHECK_MSG(!d.transformed && d.asDex == 132, "o Ditto voltou transformado");
  CHECK_EQ(d.moves[0], golpe("TRANSFORM"));
  CHECK_EQ(d.usesLeft[0], movePP(golpe("TRANSFORM")));
  CHECK_EQ(d.ability, b[0].ability);
  CHECK_EQ(d.status, ST_BADPOISON);
  CHECK_EQ(d.statusTurns, 0);             // o Toxico recomeca, como no Anil
  CHECK_EQ(d.hp, hpEl);

  // e e assim que volta a pista
  CHECK(pvpEntra(bt, l, BT_PLAYER, 0, true));
  CHECK(bt.side[BT_PLAYER].isMega());
  CHECK_EQ(bt.side[BT_PLAYER].hp, hpYo);
  CHECK_EQ(bt.side[BT_PLAYER].status, ST_SLEEP);
}

TEST(pvp, ida_e_volta_relevo_e_huida_pedem_relevo_dos_dois_lados_so_com_banco) {
  for (int banco = 0; banco < 2; banco++) {
    StoredMon a[2] = { ficha(123, 50, "UTURN", "BATONPASS", "TACKLE"), ficha(131, 50, "TACKLE") };
    StoredMon b[2] = { ficha(123, 50, "UTURN", "BATONPASS"), ficha(134, 50, "TACKLE") };
    PvpLado l[2];
    montaLados(l, a, (uint8_t)(banco ? 2 : 1), b, (uint8_t)(banco ? 2 : 1));
    Battle bt;
    pvpComeca(bt, l);
    for (uint8_t s = 0; s < 2; s++) bt.side[s].hp = bt.side[s].hpMax = 900;

    bt.useMove(BT_FOE, 0);
    CHECK_EQ(bt.querTrocar[BT_FOE], banco == 1);
    CHECK(!bt.wantsSwitch);               // o do jogador nao se mexe
    bt.useMove(BT_PLAYER, 0);
    CHECK_EQ(bt.querTrocar[BT_PLAYER], banco == 1);
    CHECK(bt.wantsSwitch);                // como sempre foi

    bt.querTrocar[BT_FOE] = false;
    HitResult r = bt.useMove(BT_FOE, 1);  // Relevo
    CHECK_EQ(r.failed, banco == 0);
    CHECK_EQ(bt.querTrocar[BT_FOE], banco == 1);

    // HUIDA (Wimp Out) do lado do rival
    StoredMon c[2] = { ficha(768, 50, "TACKLE"), ficha(134, 50, "TACKLE") };
    c[0].ability = habil("Wimp Out");
    montaLados(l, a, 1, c, (uint8_t)(banco ? 2 : 1));
    pvpComeca(bt, l);
    bt.side[BT_FOE].hpMax = 900;
    bt.side[BT_FOE].hp = 451;
    bt.side[BT_PLAYER].sureHit = true;
    bt.useMove(BT_PLAYER, 2);             // Tackle
    CHECK_MSG(bt.side[BT_FOE].hp * 2 <= 900, "premissa: o Tackle tinha de o passar da metade");
    CHECK_EQ(bt.querTrocar[BT_FOE], banco == 1);
  }
}

TEST(pvp, rugido_arrasta_e_falha_contra_ventosas_perro_guardian_e_arraigo) {
  struct Caso { const char *hab; bool rompe, raiz, banco, arrasta; } casos[] = {
    { nullptr,        false, false, true,  true  },
    { "Suction Cups", false, false, true,  false },
    { "Guard Dog",    false, false, true,  false },
    { "Suction Cups", true,  false, true,  true  },   // Rompemoldes
    { nullptr,        false, true,  true,  false },   // Arraigo
    { nullptr,        false, false, false, false },   // ninguem para entrar
  };
  for (const Caso &k : casos) {
    StoredMon a[1] = { ficha(59, 50, "ROAR") };
    StoredMon b[2] = { ficha(134, 50, "TACKLE"), ficha(135, 50, "TACKLE") };
    PvpLado l[2];
    montaLados(l, a, 1, b, (uint8_t)(k.banco ? 2 : 1));
    Battle bt;
    pvpComeca(bt, l);
    if (k.hab) bt.side[BT_FOE].ability = habil(k.hab);
    if (k.rompe) bt.side[BT_PLAYER].ability = habil("Mold Breaker");
    bt.side[BT_FOE].ingrained = k.raiz;
    HitResult r = bt.useMove(BT_PLAYER, 0);
    CHECK_EQ(bt.arrastado[BT_FOE], k.arrasta);
    CHECK_EQ(r.failed, !k.arrasta);
    CHECK_MSG(!bt.over, "contra um time, Rugido nunca acaba o combate");
  }
}

TEST(pvp, rugido_e_cola_dragao_contra_salvaje_seguem_o_anil) {
  // Rugido: um salvaje de nivel MAIOR nao foge, nem um que se agarra.
  for (int caso = 0; caso < 3; caso++) {
    Combatant a, d;
    a.init(59, 30);
    a.moves[0] = golpe("ROAR"); a.usesLeft[0] = 9; a.nMoves = 1;
    d.init(134, caso == 0 ? 40 : 20);
    Battle bt;
    bt.start(a, d);
    if (caso == 2) bt.side[BT_FOE].ingrained = true;
    HitResult r = bt.useMove(BT_PLAYER, 0);
    CHECK_EQ(r.failed, caso != 1);
    CHECK_EQ(bt.over, caso == 1);
    CHECK_EQ(bt.fled, caso == 1);
  }
  // Cola Dragao: igual de nivel, e o boneco para-a.
  for (int caso = 0; caso < 3; caso++) {
    Combatant a, d;
    a.init(149, 30);
    a.moves[0] = golpe("DRAGONTAIL"); a.usesLeft[0] = 9; a.nMoves = 1;
    d.init(134, caso == 0 ? 40 : 20);
    Battle bt;
    bt.start(a, d);
    bt.side[BT_FOE].hp = bt.side[BT_FOE].hpMax = 900;
    bt.side[BT_PLAYER].sureHit = true;
    if (caso == 2) bt.side[BT_FOE].subHp = 800;
    HitResult r = bt.useMove(BT_PLAYER, 0);
    CHECK_MSG(r.damage > 0, "premissa: a Cola Dragao tinha de acertar");
    CHECK_EQ(bt.over, caso == 1);
  }
}

TEST(pvp, cola_dragao_arrasta_mas_nao_pelo_boneco_nem_quem_se_agarra) {
  for (int caso = 0; caso < 4; caso++) {
    StoredMon a[1] = { ficha(149, 50, "DRAGONTAIL") };
    StoredMon b[2] = { ficha(134, 50, "TACKLE"), ficha(135, 50, "TACKLE") };
    PvpLado l[2];
    montaLados(l, a, 1, b, (uint8_t)(caso == 3 ? 1 : 2));
    Battle bt;
    pvpComeca(bt, l);
    bt.side[BT_FOE].hp = bt.side[BT_FOE].hpMax = 900;
    bt.side[BT_PLAYER].sureHit = true;
    if (caso == 1) bt.side[BT_FOE].subHp = 800;
    if (caso == 2) bt.side[BT_FOE].ability = habil("Suction Cups");
    HitResult r = bt.useMove(BT_PLAYER, 0);
    CHECK_MSG(r.damage > 0, "premissa: a Cola Dragao tinha de acertar");
    CHECK_EQ(bt.arrastado[BT_FOE], caso == 0);
    CHECK(!bt.over);
  }
}

TEST(pvp, o_arrastado_e_sorteado_igual_nas_duas_placas_e_so_entre_quem_pode) {
  StoredMon a[6] = { ficha(131, 50, "TACKLE"), ficha(134, 50, "TACKLE"), ficha(135, 50, "TACKLE"),
                     ficha(59, 50, "TACKLE"), ficha(94, 50, "TACKLE"), ficha(149, 50, "TACKLE") };
  PvpLado l[2];
  montaLados(l, a, 6, a, 6);
  l[0].mon[2].hp = 0;
  l[0].mon[4].hp = 0;
  int8_t s1[60], s2[60];
  btRngSemente(77);
  for (int i = 0; i < 60; i++) s1[i] = pvpSorteia(l[0]);
  btRngSemente(77);
  for (int i = 0; i < 60; i++) { random(1000); s2[i] = pvpSorteia(l[0]); random(3); }
  int visto[6] = { 0 };
  bool iguais = true;
  for (int i = 0; i < 60; i++) {
    iguais = iguais && s1[i] == s2[i];
    if (s1[i] >= 0) visto[s1[i]]++;
  }
  CHECK_MSG(iguais, "as duas placas sortearam diferente");
  CHECK_EQ(visto[0] + visto[2] + visto[4], 0);   // o da pista e os caidos, nunca
  CHECK_MSG(visto[1] && visto[3] && visto[5], "o sorteio nao passou por todos os que podem");
  l[0].mon[1].hp = l[0].mon[3].hp = l[0].mon[5].hp = 0;
  CHECK_EQ(pvpSorteia(l[0]), -1);
  btRngDesliga();
}

TEST(pvp, itens_da_bolsa_seguem_a_regra_do_convite) {
  PvpLado l;
  CHECK(!pvpPodeUsarItem(duRegras(DF_3V3, DI_PROIBIDO), l));
  CHECK(pvpPodeUsarItem(duRegras(DF_3V3, DI_ATE3), l));
  l.itensUsados = 3;
  CHECK(!pvpPodeUsarItem(duRegras(DF_3V3, DI_ATE3), l));
  CHECK(pvpPodeUsarItem(duRegras(DF_3V3, DI_LIVRE), l));
}

TEST(pvp, jogo_de_sempre_rival_sem_banco_continua_igual) {
  Combatant a, d;
  a.init(123, 50);
  d.init(123, 50);
  d.moves[0] = golpe("UTURN"); d.usesLeft[0] = 9; d.nMoves = 1;
  Battle bt;
  bt.isTrainer = true;
  bt.playerReserves = true;
  bt.start(a, d);
  bt.side[BT_PLAYER].hp = bt.side[BT_PLAYER].hpMax = 900;
  bt.useMove(BT_FOE, 0);
  CHECK(!bt.querTrocar[BT_FOE]);
  CHECK(!bt.wantsSwitch);
  bt.side[BT_PLAYER].hp = 0;
  bt.knockOut(BT_PLAYER);
  CHECK(!bt.over);                         // o jogador tem banco
  bt.side[BT_FOE].hp = 0;
  bt.knockOut(BT_FOE);
  CHECK(bt.over);                          // o rival nao: acaba
}

// ---------------------------------------------------------------------
// Um combate INTEIRO entre placas, 3 contra 3, pelo motor. As escolhas
// sao fixas (giram pelos quatro golpes); os relevos seguem o Anil:
//  - Ida e Volta / Relevo: entra o primeiro que pode, logo a seguir
//  - Rugido / Cola Dragao: entra um sorteado, e o arrastado perde a vez
//  - quem cai: substituido no FIM da ronda
namespace {
struct ResumoPvp {
  int turnos = 0, relevosQueda[2] = { 0, 0 }, arrastos = 0, idas = 0;
  uint8_t vencedor = 0xFF, caidos[2] = { 0, 0 };
  bool vioSono = false, vioConfusao = false, vioToxico = false;
};

int combatePvp(uint32_t semente, bool ruido, uint32_t *hashes, int max, ResumoPvp &s) {
  StoredMon a[3] = { ficha(131, 50, "THUNDER", "HYPNOSIS", "SCALD", "UTURN"),
                     ficha(135, 50, "THUNDER", "TOXIC", "SHADOWBALL", "BATONPASS"),
                     ficha(59, 50, "FLAMETHROWER", "ROAR", "CONFUSERAY", "TACKLE") };
  StoredMon b[3] = { ficha(134, 50, "SCALD", "CONFUSERAY", "ROAR", "THUNDER"),
                     ficha(94, 50, "SHADOWBALL", "HYPNOSIS", "TOXIC", "THUNDER"),
                     ficha(149, 50, "DRAGONTAIL", "FLAMETHROWER", "THUNDER", "UTURN") };
  PvpLado l[2];
  montaLados(l, a, 3, b, 3);
  s = ResumoPvp();
  btRngSemente(semente);
  Battle bt;
  pvpComeca(bt, l);
  auto primeiro = [&](uint8_t lado) -> int8_t {
    for (uint8_t i = 0; i < l[lado].n; i++) if (l[lado].podeEntrar(i)) return (int8_t)i;
    return -1;
  };
  // atende os pedidos do motor; true se `lado` mudou de bicho
  auto atende = [&](uint8_t lado) -> bool {
    if (bt.over) return false;
    if (bt.arrastado[lado]) {
      bt.arrastado[lado] = false;
      int8_t i = pvpSorteia(l[lado]);
      if (i >= 0 && pvpEntra(bt, l, lado, (uint8_t)i, false)) { s.arrastos++; return true; }
    }
    if (bt.querTrocar[lado]) {
      bt.querTrocar[lado] = false;
      int8_t i = primeiro(lado);
      if (i >= 0 && !bt.side[lado].fainted() && pvpEntra(bt, l, lado, (uint8_t)i, true)) {
        s.idas++;
        return true;
      }
    }
    return false;
  };
  auto olha = [&]() {
    for (uint8_t lado = 0; lado < 2; lado++) {
      s.vioSono = s.vioSono || bt.side[lado].status == ST_SLEEP;
      s.vioToxico = s.vioToxico || bt.side[lado].status == ST_BADPOISON;
      s.vioConfusao = s.vioConfusao || bt.side[lado].confuseTurns > 0;
    }
  };
  int n = 0;
  while (!bt.over && n < max) {
    uint8_t sl[2];
    for (uint8_t lado = 0; lado < 2; lado++) {
      uint8_t nm = bt.side[lado].nMoves ? bt.side[lado].nMoves : 1;
      sl[lado] = (uint8_t)((n + lado * 2) % nm);
    }
    if (ruido) random(1000);
    bt.inicioDeRonda(sl[0], sl[1]);
    uint8_t q1 = bt.movesFirst(BT_PLAYER, sl[0], sl[1]) ? BT_PLAYER : BT_FOE;
    uint8_t q2 = (uint8_t)(q1 ^ 1);
    bt.useMove(q1, sl[q1]);
    olha();
    if (ruido) { random(7); random(300); }
    atende(q1);
    bool perdeuAVez = atende(q2);
    if (!bt.over && !perdeuAVez && !bt.side[q2].fainted()) {
      bt.useMove(q2, sl[q2]);
      olha();
      atende(q2);
      atende(q1);
    }
    if (ruido) random(50);
    for (uint8_t lado = 0; lado < 2; lado++)
      if (!bt.over && !bt.side[lado].fainted()) bt.endTurn(lado);
    for (uint8_t lado = 0; lado < 2; lado++) {
      s.vioSono = s.vioSono || bt.side[lado].status == ST_SLEEP;
      s.vioToxico = s.vioToxico || bt.side[lado].status == ST_BADPOISON;
      s.vioConfusao = s.vioConfusao || bt.side[lado].confuseTurns > 0;
    }
    for (uint8_t lado = 0; lado < 2; lado++) {
      if (bt.over || !bt.side[lado].fainted()) continue;
      int8_t i = primeiro(lado);
      if (i >= 0 && pvpEntra(bt, l, lado, (uint8_t)i, false)) s.relevosQueda[lado]++;
    }
    hashes[n++] = pvpHash(bt, l);
  }
  // o da pista tambem conta nos caidos
  for (uint8_t lado = 0; lado < 2; lado++) {
    pvpGuarda(bt, l[lado], lado);
    s.caidos[lado] = l[lado].caidos();
  }
  s.vencedor = bt.over ? bt.winner : 0xFF;
  s.turnos = n;
  btRngDesliga();
  return n;
}
}  // namespace

TEST(pvp, combate_3v3_inteiro_e_o_mesmo_nas_duas_placas_com_quedas_e_relevos) {
  static uint32_t h1[300], h2[300], h3[300];
  ResumoPvp r1, r2, r3;
  randomSeed(3);
  int n1 = combatePvp(0xC0FFEE, false, h1, 300, r1);
  randomSeed(1234);
  int n2 = combatePvp(0xC0FFEE, true, h2, 300, r2);
  CHECK_EQ(n1, n2);
  bool iguais = n1 == n2;
  for (int i = 0; i < n1 && i < n2; i++) iguais = iguais && h1[i] == h2[i];
  CHECK_MSG(iguais, "as duas placas divergiram");
  CHECK_EQ(r1.vencedor, r2.vencedor);

  // e foi um combate de verdade, que passou por tudo
  CHECK_MSG(r1.vencedor <= 1, "o combate nao acabou");
  uint8_t perdedor = (uint8_t)(r1.vencedor ^ 1);
  CHECK_EQ(r1.caidos[perdedor], 3);
  CHECK_MSG(r1.relevosQueda[perdedor] >= 1, "ninguem entrou depois de uma queda");
  CHECK_MSG(r1.idas >= 1, "nenhuma Ida e Volta/Relevo trocou de bicho");
  CHECK_MSG(r1.arrastos >= 1, "nenhum Rugido/Cola Dragao arrastou ninguem");
  printf("    [3v3] turnos %d, quedas %d/%d, idas %d, arrastos %d, venceu %d, sono %d conf %d tox %d\n",
         r1.turnos, r1.relevosQueda[0], r1.relevosQueda[1], r1.idas, r1.arrastos,
         (int)r1.vencedor, (int)r1.vioSono, (int)r1.vioConfusao, (int)r1.vioToxico);
  CHECK_MSG(r1.vioSono, "o combate tinha de passar por sono");
  CHECK_MSG(r1.vioConfusao, "o combate tinha de passar por confusao");
  CHECK_MSG(r1.vioToxico, "o combate tinha de passar por toxico");

  // o dado manda de verdade
  int n3 = combatePvp(0xF00D, false, h3, 300, r3);
  bool outro = n3 != n1;
  for (int i = 0; i < n1 && i < n3; i++) outro = outro || h1[i] != h3[i];
  CHECK_MSG(outro, "outra semente deu o mesmo combate");
  CHECK(!btRngLigado());
}

// Leva 70. ILUSAO entre placas: a cara do ULTIMO do lado que ainda pode
// lutar (pbLastInTeam do Anil). As duas placas fazem a mesma conta.
TEST(pvp, a_ilusao_mostra_o_ultimo_que_ainda_luta) {
  StoredMon a[1] = { ficha(143, 50, "TACKLE") };
  StoredMon b[3] = { ficha(571, 50, "TACKLE"), ficha(25, 50, "TACKLE"),
                     ficha(6, 50, "TACKLE") };
  PvpLado l[2];
  montaLados(l, a, 1, b, 3);
  Battle bt;
  pvpComeca(bt, l);
  CHECK_MSG(bt.side[BT_FOE].abil() == AB_ILLUSION, "premissa: Zoroark com Ilusao");
  CHECK_MSG(bt.side[BT_FOE].illusion, "entra com a cara de outro");
  CHECK_EQ(bt.side[BT_FOE].pareceDex(), 6);
  CHECK_MSG(!bt.side[BT_PLAYER].illusion, "o Snorlax nao tem ilusao nenhuma");
  // o Charizard cai; o Zoroark sai e volta: agora o ultimo de pe e o Pikachu
  l[1].mon[2].hp = 0;
  CHECK(pvpEntra(bt, l, BT_FOE, 1, true));
  CHECK_MSG(!bt.side[BT_FOE].illusion, "o Pikachu entra com a cara dele");
  CHECK(pvpEntra(bt, l, BT_FOE, 0, true));
  CHECK_MSG(bt.side[BT_FOE].illusion, "o Zoroark volta disfarcado");
  CHECK_EQ(bt.side[BT_FOE].pareceDex(), 25);
}
