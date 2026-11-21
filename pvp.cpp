// O time de cada lado na batalha entre placas: ver pvp.h.
#include "pvp.h"
#include "pet.h"     // Pet::levelOf (estatica: nao toca em Pet nenhum)
#include "moves.h"

Combatant pvpCombatente(const StoredMon &m) {
  Combatant c;
  if (!m.used() || m.species > DEX_COUNT) return c;
  // Os genes e o treino ANTES de init(), como em battleStart: init() ja
  // deixa os stats feitos com eles.
  c.geneHp = m.geneHp; c.geneAtk = m.geneAtk; c.geneDef = m.geneDef;
  c.geneSpa = m.geneSpa; c.geneSpd = m.geneSpd; c.geneSpe = m.geneSpe;
  c.trHp = m.trHp; c.trAtk = m.trAtk; c.trDef = m.trDef;
  c.trSpa = m.trSpa; c.trSpd = m.trSpd; c.trSpe = m.trSpe;
  c.bond = m.bond;
  // A FORMA vai no init: um Meowth de Galar nao tem os PS do de Kanto.
  c.init(m.species, PVP_NIVEL, (m.flags & SM_SHINY) != 0, m.form);
  if (m.ability) {
    c.ability = m.ability;
  } else {
    const uint16_t *a = ABIL_OF[m.species];
    c.ability = a[0] ? a[0] : a[1];
  }
  c.item = m.item;
  // O sexo e o da ficha (Rivalry, Atraccion). Os sem sexo ficam em 2.
  c.sexo = DEX_TBL[m.species].femPct == GENDERLESS
               ? 2 : ((m.flags & SM_FEMALE) ? 1 : 0);
  // OS GOLPES SAO OS DELE, com os PP cheios. Os que o dono escolheu,
  // tal e qual; senao, os que ele sabe no SEU nivel —— subir para 50
  // nao ensina golpe nenhum, e descer de 70 nao faz esquecer.
  uint16_t mv[BT_MOVES] = { 0, 0, 0, 0 };
  uint8_t n = 0;
  if (m.flags & SM_MOVES_PICKED) {
    for (uint8_t i = 0; i < 4; i++)
      if (m.moves[i] && m.moves[i] < MOVE_COUNT) mv[n++] = m.moves[i];
  } else {
    n = movesAtLevel(c.asDex, Pet::levelOf(m), mv);
  }
  for (uint8_t i = 0; i < BT_MOVES; i++) {
    c.moves[i] = i < n ? mv[i] : 0;
    c.usesLeft[i] = i < n ? movePP(mv[i]) : 0;
  }
  c.nMoves = n;
  return c;
}

uint8_t PvpLado::reservas() const {
  uint8_t k = 0;
  for (uint8_t i = 0; i < n; i++) if (podeEntrar(i)) k++;
  return k;
}

uint8_t PvpLado::caidos() const {
  uint8_t k = 0;
  for (uint8_t i = 0; i < n; i++) if (mon[i].valid() && mon[i].hp == 0) k++;
  return k;
}

bool pvpMonta(PvpLado &l, const uint8_t regs[][TROCA_MON], uint8_t n) {
  l = PvpLado();
  if (n < 1 || n > DUELO_MAX) return false;
  for (uint8_t i = 0; i < n; i++) {
    memcpy(&l.reg[i], regs[i], TROCA_MON);
    if (!l.reg[i].used() || l.reg[i].species > DEX_COUNT) return false;
    l.mon[i] = pvpCombatente(l.reg[i]);
    if (!l.mon[i].valid()) return false;
  }
  l.n = n;
  return true;
}

void pvpSincroniza(Battle &b, const PvpLado l[2]) {
  b.playerReserves = l[BT_PLAYER].reservas() > 0;
  b.foeReserves = l[BT_FOE].reservas() > 0;
}

// ILUSION (leva 70): a cara do ULTIMO do lado que ainda pode lutar, se
// nao for o proprio que entra (pbLastInTeam do Anil). As duas placas
// fazem a mesma conta com os mesmos registros.
static void pvpIlusao(const PvpLado &l, uint8_t entra, Combatant &c) {
  c.ilusDex = 0;
  c.ilusForm = 0;
  c.ilusShiny = false;
  for (int8_t j = (int8_t)l.n - 1; j >= 0; j--) {
    if (!l.mon[j].valid() || l.mon[j].hp == 0) continue;
    if ((uint8_t)j == entra) return;
    c.ilusDex = l.reg[j].species;
    c.ilusForm = l.reg[j].form;
    c.ilusShiny = (l.reg[j].flags & SM_SHINY) != 0;
    return;
  }
}

void pvpComeca(Battle &b, PvpLado l[2]) {
  for (uint8_t s = 0; s < 2; s++) {
    l[s].ativo = 0;
    l[s].itensUsados = 0;
  }
  // Sem fuga, sem captura, Rugido nao acaba o combate: e um combate
  // contra um treinador, que e o que o outro e.
  b.isTrainer = true;
  pvpSincroniza(b, l);
  Combatant a = l[BT_PLAYER].mon[0], f = l[BT_FOE].mon[0];
  pvpIlusao(l[BT_PLAYER], 0, a);
  pvpIlusao(l[BT_FOE], 0, f);
  b.start(a, f);
}

// O que sobrevive a um relevo, e SO isso. O combatente em pista muda em
// coisas que no Anil se desfazem ao sair (Transform, Skill Swap, Worry
// Seed, Soak, Mimic, as formas de Illusion e companhia), entao nao se
// guarda o combatente inteiro: reconstroi-se do registro e trazem-se os
// PS, o estado (o contador do Toxico volta ao inicio, como o
// PBEffects::Toxic do Anil), os PP de cada golpe SEU, o objeto, a mega
// (que fica ate ao fim) e as marcas que sao do bicho e nao da pista.
void pvpGuarda(const Battle &b, PvpLado &l, uint8_t lado) {
  if (lado > 1 || l.ativo >= l.n) return;
  const Combatant &c = b.side[lado];
  const Combatant &antes = l.mon[l.ativo];
  Combatant x = pvpCombatente(l.reg[l.ativo]);
  // Os PP: primeiro os que tinha ao entrar, depois os da pista por ID do
  // golpe. Um Transform (golpes copiados a 5 PP) ou um Mimic nao gastam
  // os dele, e esses ficam como estavam.
  for (uint8_t i = 0; i < x.nMoves; i++) {
    for (uint8_t j = 0; j < antes.nMoves; j++)
      if (antes.moves[j] == x.moves[i]) { x.usesLeft[i] = antes.usesLeft[j]; break; }
    if (c.transformed) continue;
    for (uint8_t j = 0; j < c.nMoves && j < BT_MOVES; j++)
      if (c.moves[j] == x.moves[i]) { x.usesLeft[i] = c.usesLeft[j]; break; }
  }
  if (c.isMega()) x.megaEvolve(c.megaIdx);
  x.hp = c.hp > x.hpMax ? x.hpMax : c.hp;
  x.status = c.status;
  x.statusTurns = c.status == ST_BADPOISON ? 0 : c.statusTurns;
  x.item = c.item;
  x.itemComido = c.itemComido;
  x.ateBerry = c.ateBerry;
  x.lastBerry = c.lastBerry;
  x.hero = c.hero;
  x.entryDone = c.entryDone;
  l.mon[l.ativo] = x;
}

bool pvpEntra(Battle &b, PvpLado l[2], uint8_t lado, uint8_t i,
              bool porGusto) {
  if (lado > 1) return false;
  PvpLado &me = l[lado];
  if (!me.podeEntrar(i)) return false;
  // Regenerator, De Cero a Heroe, Bateria: so para quem SAI de pe. O Anil
  // chama pbAbilitiesOnSwitchOut apenas `if !battler.fainted?`.
  if (!b.side[lado].fainted()) b.regenOnSwitch(lado);
  pvpGuarda(b, me, lado);
  me.ativo = i;
  Combatant c = me.mon[i];
  c.alliesFallen = me.caidos();
  pvpIlusao(me, i, c);
  b.sendOut(lado, c, porGusto);
  b.querTrocar[lado] = false;
  b.arrastado[lado] = false;
  pvpSincroniza(b, l);
  return true;
}

int8_t pvpSorteia(const PvpLado &l) {
  uint8_t k = l.reservas();
  if (!k) return -1;
  uint8_t alvo = (uint8_t)btRand(k);
  for (uint8_t i = 0; i < l.n; i++)
    if (l.podeEntrar(i) && alvo-- == 0) return (int8_t)i;
  return -1;
}

static void hMix(uint32_t &h, uint32_t v) {
  for (uint8_t i = 0; i < 4; i++) { h ^= (v >> (8 * i)) & 0xFF; h *= 16777619u; }
}

uint32_t pvpHash(const Battle &b, const PvpLado l[2]) {
  uint32_t h = btHash(b);
  for (uint8_t s = 0; s < 2; s++) {
    hMix(h, (uint32_t)l[s].n | ((uint32_t)l[s].ativo << 8) |
                ((uint32_t)l[s].itensUsados << 16));
    for (uint8_t i = 0; i < l[s].n; i++) {
      const Combatant &c = l[s].mon[i];
      hMix(h, (uint32_t)c.hp | ((uint32_t)c.hpMax << 16));
      hMix(h, (uint32_t)c.status | ((uint32_t)c.statusTurns << 8) |
                  ((uint32_t)c.item << 16) | ((uint32_t)c.megaIdx << 24));
      for (uint8_t k = 0; k < BT_MOVES; k++) hMix(h, c.usesLeft[k]);
    }
  }
  return h;
}

bool pvpPodeUsarItem(uint8_t regras, const PvpLado &l) {
  switch (DR_ITENS(regras)) {
    case DI_LIVRE: return true;
    case DI_ATE3:  return l.itensUsados < 3;
    default:       return false;
  }
}
