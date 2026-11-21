// Los entrenadores de a pie: los que te cruzas entre gimnasio y gimnasio.
//
// Por que existen, si ya hay salvajes: un salvaje siempre es lo mismo (sale
// de la hierba, lo cazas o lo ignoras). Un entrenador da VARIEDAD sin
// contenido nuevo — pega mas fuerte, paga mejor y no se puede capturar —, y
// es lo que hace que el camino hasta el gimnasio no sea una sola cosa
// repetida treinta veces.
//
// Los sprites son del Anil (mismo orden que ART_TRAINER en art.h). Ocho
// llegan: son sabor, no catalogo.
#pragma once
#include <stdint.h>

struct TrainerEntry {
  const char *name;   // nombre propio corto: NO se traduce
  // CUANTOS bichos saca. El nivel es el mismo que el tuyo: la dificultad
  // esta en que son varios seguidos y tu vas gastando PS y PP, no en que
  // pegue mas fuerte. Un rival de nivel superior se siente injusto; tres
  // rivales de tu nivel se sienten como un combate de verdad.
  uint8_t team;
};

#define TRAINER_COUNT 8
#define TRAINER_TEAM_MAX 3
static const TrainerEntry TRAINER_TBL[TRAINER_COUNT] = {
  { "BUG CATCHER", 1 },
  { "GAMER",       2 },
  { "SWIMMER",     2 },
  { "BIKER",       2 },
  { "SCIENTIST",   2 },
  { "HIKER",       3 },
  { "FISHERMAN",   2 },
  { "ROCKET",      3 },
};

static inline const TrainerEntry &trainerAt(uint8_t i) {
  return TRAINER_TBL[i < TRAINER_COUNT ? i : 0];
}
