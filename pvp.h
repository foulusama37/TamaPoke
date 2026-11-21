#pragma once
// O TIME de cada lado na batalha entre placas.
//
// Nada de Pet nem de tela aqui, e de proposito: as duas placas montam OS
// DOIS LADOS a partir dos MESMOS registros (os 52 bytes de StoredMon que
// o Duelo trocou) e da mesma semente. O meu time tambem sai do registro,
// nao do Pet vivo: se uma placa montasse o seu de um sitio e a outra de
// outro, bastaria um campo a mais para as duas verem combates diferentes.
//
// O combate NAO MEXE no save: tudo o que acontece aqui fica nestas
// copias. Ninguem escreve de volta em pet.team.
#include <stdint.h>
#include "battle.h"
#include "party.h"
#include "duelo.h"

// Todo o mundo no nivel 50, para cima ou para baixo (regra do user).
#define PVP_NIVEL 50

// Um combatente do registro, como entra numa batalha entre placas:
// nivel 50, PS e PP cheios, sem estado; os genes, o treino, a forma, o
// sexo, a habilidade, o objeto e os golpes sao os dele.
Combatant pvpCombatente(const StoredMon &m);

struct PvpLado {
  uint8_t n = 0;                 // quantos levou (1, 3 ou 6)
  uint8_t ativo = 0;             // qual esta na pista
  StoredMon reg[DUELO_MAX];      // como chegou: e daqui que se reconstroi
  Combatant mon[DUELO_MAX];      // o que lhe resta NESTE combate
  uint8_t itensUsados = 0;       // da bolsa (a regra "ate 3")

  bool podeEntrar(uint8_t i) const {
    return i < n && i != ativo && mon[i].valid() && mon[i].hp > 0;
  }
  uint8_t reservas() const;      // quantos podem entrar agora
  uint8_t caidos() const;        // quantos ja cairam (Ultimo Adios)
};

// Monta um lado com os registros NA ORDEM em que vieram (o primeiro abre).
// false se algum nao serve (vazio, ovo, especie fora da dex).
bool pvpMonta(PvpLado &l, const uint8_t regs[][TROCA_MON], uint8_t n);

// Arranca o combate: os primeiros de cada lado, banco dos dois lados,
// sem fuga nem captura (isTrainer). `l[0]` e o anfitriao (BT_PLAYER).
void pvpComeca(Battle &b, PvpLado l[2]);

// Guarda o que esta na pista de `lado` na ficha dele, SO com o que
// sobrevive a um relevo (ver pvp.cpp).
void pvpGuarda(const Battle &b, PvpLado &l, uint8_t lado);

// Relevo: guarda o que sai e poe `i` na pista. false se `i` nao pode.
bool pvpEntra(Battle &b, PvpLado l[2], uint8_t lado, uint8_t i,
              bool porGusto);

// Quem entra arrastado (Rugido, Cola Dragao): um sorteado entre os que
// podem, com o dado do combate. -1 se ninguem pode.
int8_t pvpSorteia(const PvpLado &l);

// Mantem `playerReserves`/`foeReserves` em dia: e o que diz ao motor se
// cair acaba o combate.
void pvpSincroniza(Battle &b, const PvpLado l[2]);

// A impressao digital: a do motor e a do banco dos dois lados.
uint32_t pvpHash(const Battle &b, const PvpLado l[2]);

// A regra dos itens da bolsa. Os que levam encima nao contam aqui.
bool pvpPodeUsarItem(uint8_t regras, const PvpLado &l);
