// El cuestionario de Oak (quiz.h).
//
// Por que este fichero es el que importa: las preguntas se MONTAN sobre
// las tablas, no estan escritas a mano, asi que la respuesta buena no
// puede estar mal. Lo que si puede pasar —— y no se ve jugando hasta que
// te pasa —— es que una opcion MALA sea tambien buena:
//
//   un Charizard es fuego Y VOLADOR. Si "volador" sale de senuelo, el
//   aparato te dice que has fallado habiendo acertado.
//
// Asi que aqui no se comprueba que el generador "funcione": se vuelve a
// resolver cada pregunta contra las tablas y se exige que haya UNA sola
// respuesta buena. Si manana alguien anade una plantilla, esto la pilla
// sin tocar nada, porque el bucle recorre QZ_KIND_COUNT.
#include "framework.h"
#include "shim/Arduino.h"
#include "../dex.h"
#include "../moves.h"
#include "../quiz.h"
#include <string.h>
#include <string>

// Cuantas de cada clase. 4000 por clase llega de sobra para que salgan
// los bichos de dos tipos y los golpes compartidos.
static const int RONDAS = 4000;

static const char *nom(const QuizQ &q, int i) {
  return q.opt[i] ? q.opt[i] : "(null)";
}

// Lo que DEBERIA valer cada opcion, resuelto otra vez desde las tablas.
// Devuelve true si esa opcion es una respuesta buena.
static bool esBuena(const QuizQ &q, const char *op) {
  switch (q.kind) {
    case QZ_TYPE_OF: {
      const DexEntry &e = DEX_TBL[q.subjDex];
      // los DOS tipos cuentan como buenos
      return strcmp(op, typeName(e.type1)) == 0 ||
             (e.type2 != T_NONE && strcmp(op, typeName(e.type2)) == 0);
    }
    case QZ_EVOLVES: {
      uint16_t evo = DEX_TBL[q.subjDex].evolvesTo;
      return evo && strcmp(op, DEX_TBL[evo].name) == 0;
    }
    case QZ_MOVE_TYPE: {
      for (int m = 0; m < MOVE_COUNT; m++)
        if (strcmp(MOVE_TBL[m].name, q.subject) == 0)
          return strcmp(op, typeName(MOVE_TBL[m].type)) == 0;
      return false;
    }
    case QZ_LEARNS: {
      for (int m = 0; m < MOVE_COUNT; m++)
        if (strcmp(MOVE_TBL[m].name, op) == 0 && qzAprende(q.subjDex, m))
          return true;
      return false;
    }
    case QZ_WHO:
      // la buena es SU nombre; otra especie con el mismo nombre tambien
      // lo seria, y por eso el generador las descarta
      return strcmp(op, DEX_TBL[q.subjDex].name) == 0;
    default:
      return false;   // QZ_HEAVIER se mira aparte (es comparativa)
  }
}

TEST(quiz, cada_pregunta_tiene_UNA_sola_respuesta_buena) {
  for (uint8_t k = 0; k < QZ_KIND_COUNT; k++) {
    if (k == QZ_HEAVIER) continue;          // tiene su propio test
    int hechas = 0;
    for (int s = 0; s < RONDAS; s++) {
      randomSeed(1000 + s);
      QuizQ q;
      if (!quizUna(q, k)) continue;
      hechas++;
      CHECK_MSG(q.right < QUIZ_OPTS,
                std::string("clase ") + std::to_string(k) + ": indice malo");
      // la que marca como buena, lo es
      CHECK_MSG(esBuena(q, nom(q, q.right)),
                std::string("clase ") + std::to_string(k) + " sobre '" +
                (q.subject ? q.subject : "?") + "': dice que la buena es '" +
                nom(q, q.right) + "' y no lo es");
      // y NINGUNA otra lo es
      for (int i = 0; i < QUIZ_OPTS; i++) {
        if (i == q.right) continue;
        CHECK_MSG(!esBuena(q, nom(q, i)),
                  std::string("clase ") + std::to_string(k) + " sobre '" +
                  (q.subject ? q.subject : "?") + "': ofrece '" + nom(q, i) +
                  "' como fallo y TAMBIEN es correcta");
      }
    }
    CHECK_MSG(hechas > RONDAS / 10,
              std::string("la clase ") + std::to_string(k) +
              " casi nunca consigue montar pregunta: " +
              std::to_string(hechas) + " de " + std::to_string(RONDAS));
  }
}

TEST(quiz, las_tres_opciones_son_distintas_y_existen) {
  for (uint8_t k = 0; k < QZ_KIND_COUNT; k++) {
    for (int s = 0; s < RONDAS; s++) {
      randomSeed(50000 + s);
      QuizQ q;
      if (!quizUna(q, k)) continue;
      for (int i = 0; i < QUIZ_OPTS; i++) {
        CHECK_MSG(q.opt[i] != nullptr,
                  std::string("clase ") + std::to_string(k) + ": opcion vacia");
        CHECK(q.opt[i][0] != 0);
        for (int j = i + 1; j < QUIZ_OPTS; j++)
          CHECK_MSG(strcmp(q.opt[i], q.opt[j]) != 0,
                    std::string("clase ") + std::to_string(k) +
                    ": dos botones dicen lo mismo ('" + q.opt[i] + "')");
      }
    }
  }
}

// El peso es comparativo, asi que se comprueba aparte: la buena tiene que
// ser la MAS pesada, y con holgura —— si no, es una moneda al aire.
TEST(quiz, el_que_pesa_mas_pesa_mas_y_se_nota) {
  int hechas = 0;
  for (int s = 0; s < RONDAS; s++) {
    randomSeed(90000 + s);
    QuizQ q;
    if (!quizUna(q, QZ_HEAVIER)) continue;
    hechas++;
    uint16_t pesos[QUIZ_OPTS] = { 0, 0, 0 };
    for (int i = 0; i < QUIZ_OPTS; i++)
      for (int d = 1; d <= DEX_COUNT; d++)
        if (strcmp(DEX_TBL[d].name, q.opt[i]) == 0) {
          pesos[i] = DEX_TBL[d].weight10;
          break;
        }
    for (int i = 0; i < QUIZ_OPTS; i++) {
      CHECK_MSG(pesos[i] > 0, std::string("'") + nom(q, i) +
                "' no esta en la tabla o no pesa nada");
      if (i == q.right) continue;
      CHECK_MSG(pesos[q.right] >= pesos[i] * 2,
                std::string("dice que '") + nom(q, q.right) + "' (" +
                std::to_string(pesos[q.right]) + ") pesa mas que '" +
                nom(q, i) + "' (" + std::to_string(pesos[i]) +
                ") y no se nota lo bastante");
    }
  }
  CHECK_MSG(hechas > RONDAS / 10,
            std::string("casi nunca sale: ") + std::to_string(hechas));
}

// Leva 68. "Quem e esse Pokemon?": el enunciado NO puede llevar el
// nombre (seria la respuesta), y la silueta solo cuando se pide.
TEST(quiz, quien_es_no_dice_el_nombre_y_la_silueta_es_a_peticion) {
  int conSil = 0;
  for (int s = 0; s < 500; s++) {
    randomSeed(77000 + s);
    QuizQ q;
    CHECK(quizUna(q, QZ_WHO));
    CHECK_MSG(q.subject == nullptr, "el enunciado lleva el nombre");
    CHECK(q.subjDex >= 1 && q.subjDex <= DEX_COUNT);
    CHECK_MSG(!q.silueta, "en la ronda normal sale a color");
    QuizQ r;
    qzQuienEs(r, true);
    conSil += r.silueta;
    // y la plantilla siguiente no hereda la silueta
    QuizQ t = r;
    CHECK(quizUna(t, QZ_TYPE_OF));
    CHECK(!t.silueta);
  }
  CHECK_EQ(conSil, 500);
}

// quizSiguiente() es la que usa el aparato: tiene que devolver SIEMPRE
// una pregunta. Si un dia una plantilla se rompe, que no se quede el
// Profesor mudo en pantalla.
TEST(quiz, siempre_sale_una_pregunta) {
  for (int s = 0; s < 3000; s++) {
    randomSeed(7000 + s);
    QuizQ q;
    CHECK_MSG(quizSiguiente(q), std::string("semilla ") + std::to_string(s) +
              ": no ha salido ninguna pregunta");
    CHECK(q.opt[q.right] != nullptr);
  }
}

// Y que no encadene dos de la misma clase: una ronda de cinco "de que
// tipo es" seguidas se siente a chapuza.
TEST(quiz, no_repite_la_clase_de_la_anterior) {
  for (int s = 0; s < 2000; s++) {
    randomSeed(31000 + s);
    QuizQ a, b;
    if (!quizSiguiente(a)) continue;
    if (!quizSiguiente(b, a.kind)) continue;
    CHECK_MSG(a.kind != b.kind,
              std::string("dos seguidas de la clase ") +
              std::to_string(a.kind));
  }
}
