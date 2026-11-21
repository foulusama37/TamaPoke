// El cuestionario del Profesor Oak.
//
// Las preguntas NO estan escritas a mano: se montan en el momento sobre las
// MISMAS tablas con las que el aparato juega (DEX_TBL, MOVE_TBL, LEARN_*).
// Es una decision, y tiene dos motivos.
//
//   1. La respuesta buena no puede estar mal. Si el Charizard de la tabla
//      es de fuego, la pregunta "de que tipo es Charizard" se contesta sola
//      con el mismo dato que usa el combate. Una lista de trivia escrita a
//      mano habria que creersela: nadie puede repasar mil respuestas, y un
//      error ahi se descubre jugando, que es tarde.
//   2. No ocupa flash y no se repite. 1025 especies y 718 golpes dan
//      decenas de miles de preguntas distintas sin una sola cadena nueva.
//
// Lo que SI hay que vigilar —— y lo vigila test_quiz.cpp —— es que las
// opciones MALAS sean de verdad malas: un bicho de dos tipos tiene DOS
// respuestas buenas, y ofrecer la segunda como fallo es exactamente el
// error que una lista a mano no comete y un generador comete solo.
#pragma once
#include <stdint.h>
#include "dex.h"
#include "moves.h"

#define QUIZ_OPTS 3   // caben tres botones en la pantalla redonda
#define QUIZ_LEN  5   // preguntas por ronda

enum : uint8_t {
  QZ_TYPE_OF = 0,   // de que tipo es <bicho>
  QZ_EVOLVES,       // en que evoluciona <bicho>
  QZ_MOVE_TYPE,     // de que tipo es el golpe <golpe>
  QZ_HEAVIER,       // cual de los tres pesa mas
  QZ_LEARNS,        // cual de estos golpes aprende <bicho>
  QZ_WHO,           // quien es ese Pokemon (su dibujo; tres nombres)
  QZ_KIND_COUNT,
};

// Las opciones APUNTAN a las tablas (TYPE_NAME, DEX_TBL[].name,
// MOVE_TBL[].name); no se copia texto. Son const char* de flash y viven
// mas que la pregunta.
struct QuizQ {
  uint8_t kind = QZ_TYPE_OF;
  const char *subject = nullptr;   // el nombre que va en el enunciado
  int16_t subjDex = 0;             // especie del enunciado (0 = no hay)
  const char *opt[QUIZ_OPTS] = { nullptr, nullptr, nullptr };
  uint8_t right = 0;               // cual de las tres es la buena
  bool silueta = false;            // QZ_WHO: el dibujo todo en negro
};

// ------------------------------------------------------------- utiles

// Si la especie aprende ese golpe A CUALQUIER nivel. Mira la lista
// entera, no los cuatro de su nivel: la pregunta es "lo aprende", no
// "lo lleva puesto".
static inline bool qzAprende(int16_t dex, uint16_t mv) {
  if (dex < 1 || dex > DEX_COUNT) return false;
  for (uint16_t i = LEARN_OFS[dex - 1]; i < LEARN_OFS[dex]; i++)
    if (LEARN_MV[i] == mv) return true;
  return false;
}

static inline uint16_t qzCuantosAprende(int16_t dex) {
  if (dex < 1 || dex > DEX_COUNT) return 0;
  return (uint16_t)(LEARN_OFS[dex] - LEARN_OFS[dex - 1]);
}

static inline bool qzYaEsta(const QuizQ &q, uint8_t n, const char *s) {
  for (uint8_t i = 0; i < n; i++)
    if (q.opt[i] && s && strcmp(q.opt[i], s) == 0) return true;
  return false;
}

// Mezcla las tres y se lleva consigo cual era la buena. Sin esto la
// respuesta correcta seria siempre el primer boton.
static inline void qzBaraja(QuizQ &q) {
  for (int i = QUIZ_OPTS - 1; i > 0; i--) {
    int j = (int)random(i + 1);
    const char *t = q.opt[i];
    q.opt[i] = q.opt[j];
    q.opt[j] = t;
    if (q.right == i) q.right = (uint8_t)j;
    else if (q.right == j) q.right = (uint8_t)i;
  }
}

// ------------------------------------------------------- cada plantilla
//
// Todas devuelven false si no han podido montar una pregunta limpia, y
// quien llama prueba con otra. Preferimos rendirnos a soltar una pregunta
// con dos respuestas buenas.

static inline bool qzTipoDe(QuizQ &q) {
  int16_t d = (int16_t)(1 + random(DEX_COUNT));
  const DexEntry &e = DEX_TBL[d];
  if (e.type1 == T_NONE) return false;
  q.kind = QZ_TYPE_OF;
  q.subject = e.name;
  q.subjDex = d;
  q.opt[0] = typeName(e.type1);
  q.right = 0;
  // El segundo tipo NO vale como fallo: seria una respuesta buena
  // marcada como mala. Es el error que mas facil se cuela aqui.
  for (uint8_t n = 1; n < QUIZ_OPTS; ) {
    uint8_t t = (uint8_t)(1 + random(T_COUNT - 1));
    if (t == e.type1 || t == e.type2) continue;
    const char *s = typeName(t);
    if (qzYaEsta(q, n, s)) continue;
    q.opt[n++] = s;
  }
  return true;
}

static inline bool qzEvolucionaEn(QuizQ &q) {
  int16_t d = (int16_t)(1 + random(DEX_COUNT));
  uint16_t evo = DEX_TBL[d].evolvesTo;
  if (!evo || evo > DEX_COUNT) return false;
  q.kind = QZ_EVOLVES;
  q.subject = DEX_TBL[d].name;
  q.subjDex = d;
  q.opt[0] = DEX_TBL[evo].name;
  q.right = 0;
  for (uint8_t n = 1; n < QUIZ_OPTS; ) {
    int16_t o = (int16_t)(1 + random(DEX_COUNT));
    if (o == (int16_t)evo || o == d) continue;
    const char *s = DEX_TBL[o].name;
    if (qzYaEsta(q, n, s)) continue;
    q.opt[n++] = s;
  }
  return true;
}

static inline bool qzTipoDeGolpe(QuizQ &q) {
  // desde 1: el 0 es Forcejeo, que no es un golpe que nadie "sepa".
  uint16_t m = (uint16_t)(1 + random(MOVE_COUNT - 1));
  const MoveEntry &mv = MOVE_TBL[m];
  if (mv.type == T_NONE) return false;
  q.kind = QZ_MOVE_TYPE;
  q.subject = mv.name;
  q.subjDex = 0;
  q.opt[0] = typeName(mv.type);
  q.right = 0;
  for (uint8_t n = 1; n < QUIZ_OPTS; ) {
    uint8_t t = (uint8_t)(1 + random(T_COUNT - 1));
    if (t == mv.type) continue;
    const char *s = typeName(t);
    if (qzYaEsta(q, n, s)) continue;
    q.opt[n++] = s;
  }
  return true;
}

static inline bool qzPesaMas(QuizQ &q) {
  int16_t d[QUIZ_OPTS];
  for (uint8_t n = 0; n < QUIZ_OPTS; ) {
    int16_t c = (int16_t)(1 + random(DEX_COUNT));
    if (!DEX_TBL[c].weight10) continue;     // sin peso en la tabla
    bool rep = false;
    for (uint8_t k = 0; k < n; k++)
      if (d[k] == c || strcmp(DEX_TBL[d[k]].name, DEX_TBL[c].name) == 0)
        rep = true;
    if (rep) continue;
    d[n++] = c;
  }
  uint8_t may = 0;
  for (uint8_t i = 1; i < QUIZ_OPTS; i++)
    if (DEX_TBL[d[i]].weight10 > DEX_TBL[d[may]].weight10) may = i;
  // Que se note. Con 12 kg contra 13 esto no es una pregunta de cultura,
  // es una moneda al aire: se pide que el mayor pese AL MENOS el doble
  // que cualquier otro.
  for (uint8_t i = 0; i < QUIZ_OPTS; i++)
    if (i != may && DEX_TBL[d[i]].weight10 * 2 > DEX_TBL[d[may]].weight10)
      return false;
  q.kind = QZ_HEAVIER;
  q.subject = nullptr;          // el enunciado no lleva nombre
  q.subjDex = 0;
  for (uint8_t i = 0; i < QUIZ_OPTS; i++) q.opt[i] = DEX_TBL[d[i]].name;
  q.right = may;
  return true;
}

static inline bool qzQueAprende(QuizQ &q) {
  int16_t d = (int16_t)(1 + random(DEX_COUNT));
  uint16_t n = qzCuantosAprende(d);
  if (!n) return false;
  uint16_t bueno = LEARN_MV[LEARN_OFS[d - 1] + random(n)];
  if (bueno >= MOVE_COUNT) return false;
  q.kind = QZ_LEARNS;
  q.subject = DEX_TBL[d].name;
  q.subjDex = d;
  q.opt[0] = MOVE_TBL[bueno].name;
  q.right = 0;
  for (uint8_t k = 1; k < QUIZ_OPTS; ) {
    uint16_t m = (uint16_t)(1 + random(MOVE_COUNT - 1));
    if (qzAprende(d, m)) continue;          // tambien lo aprende: no vale
    const char *s = MOVE_TBL[m].name;
    if (qzYaEsta(q, k, s)) continue;
    q.opt[k++] = s;
  }
  return true;
}

// QUIEN ES ESE POKEMON. El enunciado NO lleva el nombre (es la pregunta):
// subject queda en nullptr y la pantalla pinta el dibujo de subjDex. Las
// dos malas son otras especies con OTRO nombre.
static inline bool qzQuienEs(QuizQ &q, bool silueta) {
  int16_t d = (int16_t)(1 + random(DEX_COUNT));
  q.kind = QZ_WHO;
  q.subject = nullptr;
  q.subjDex = d;
  q.silueta = silueta;
  q.opt[0] = DEX_TBL[d].name;
  q.right = 0;
  for (uint8_t n = 1; n < QUIZ_OPTS; ) {
    int16_t o = (int16_t)(1 + random(DEX_COUNT));
    if (o == d) continue;
    const char *s = DEX_TBL[o].name;
    if (qzYaEsta(q, n, s)) continue;
    q.opt[n++] = s;
  }
  return true;
}

// --------------------------------------------------------------- montar

// Una pregunta de la clase que se pida. Devuelve false si esa clase no ha
// podido montarla (el que llama reintenta).
static inline bool quizUna(QuizQ &q, uint8_t kind) {
  q.silueta = false;
  switch (kind) {
    case QZ_TYPE_OF:   if (!qzTipoDe(q)) return false; break;
    case QZ_EVOLVES:   if (!qzEvolucionaEn(q)) return false; break;
    case QZ_MOVE_TYPE: if (!qzTipoDeGolpe(q)) return false; break;
    case QZ_HEAVIER:   if (!qzPesaMas(q)) return false; break;
    case QZ_LEARNS:    if (!qzQueAprende(q)) return false; break;
    case QZ_WHO:       if (!qzQuienEs(q, false)) return false; break;
    default: return false;
  }
  qzBaraja(q);
  return true;
}

// Una pregunta cualquiera. `evita` es la clase de la anterior, para no
// encadenar dos iguales; 0xFF = da igual.
static inline bool quizSiguiente(QuizQ &q, uint8_t evita = 0xFF) {
  for (int intento = 0; intento < 64; intento++) {
    uint8_t k = (uint8_t)random(QZ_KIND_COUNT);
    if (k == evita && QZ_KIND_COUNT > 1) continue;
    if (quizUna(q, k)) return true;
  }
  // Ultimo recurso: el tipo de un golpe sale SIEMPRE (los 718 tienen
  // tipo). Si ni eso, es que las tablas estan rotas y lo dira un test.
  return quizUna(q, QZ_MOVE_TYPE);
}
