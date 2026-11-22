// Consistencia de las traducciones (i18n.cpp).
#include "framework.h"
#include "shim/Arduino.h"
#include "shim/Preferences.h"
#include "../i18n.h"
#include "../pet.h"  // MED_COUNT
#include <ctype.h>
#include <string.h>
#include <vector>

static const char *LANG_NAME[LANG_COUNT] = { "ES", "EN", "FR", "DE", "IT", "PT", "JA", "KO" };

// extrae la firma de los especificadores de printf de una cadena: "%lud" -> "lu"
static std::vector<std::string> specs(const char *s) {
  std::vector<std::string> out;
  const char *p = s;
  while (*p) {
    if (*p != '%') { p++; continue; }
    p++;
    if (*p == '%') { p++; continue; }   // %% es un literal
    std::string sig;
    while (*p && strchr("-+ #0", *p)) p++;             // banderas
    while (*p && isdigit((unsigned char)*p)) p++;      // ancho
    if (*p == '.') { p++; while (*p && isdigit((unsigned char)*p)) p++; }
    while (*p && strchr("lhzjt", *p)) { sig += *p; p++; }  // modificador
    if (!*p) { out.push_back(sig + "<incompleto>"); break; }
    sig += *p++;
    out.push_back(sig);
  }
  return out;
}

static std::string joinSpecs(const std::vector<std::string> &v) {
  std::string s;
  for (size_t i = 0; i < v.size(); i++) { if (i) s += ","; s += "%" + v[i]; }
  return s.empty() ? "(sin formato)" : s;
}

// firma esperada de cada cadena con formato, sacada de las llamadas del sketch
struct ExpectedFmt { StrId id; const char *sig; };
static const ExpectedFmt EXPECTED[] = {
  // leva 75: mensagens de combate
  { S_BM_ABILITY, "%s,%s" },
  { S_BM_SENDS, "%s,%s" },
  { S_BM_WILD_APPEARED, "%s" },
  { S_BM_GO, "%s" },
  { S_BM_GO_2, "%s,%s" },
  { S_BM_GOT, "%s" },
  { S_BM_GOT_A_HEALTHY_GIFT, "%s" },
  { S_BM_WAS_CURED_OF_PARALYSIS, "%s" },
  { S_BM_WAS_BURNED_BY_THE, "%s" },
  { S_BM_UNLEASHED_ITS_ENERGY, "%s" },
  { S_BM_SUBSTITUTE_FADED, "%s" },
  { S_BM_IT_DOESN_T_AFFECT, "%s" },
  { S_BM_HIT_TIMES, "%u" },
  { S_BM_WAS_BURNED, "%s" },
  { S_BM_WAS_POISONED, "%s" },
  { S_BM_IS_PARALYZED, "%s" },
  { S_BM_FELL_ASLEEP, "%s" },
  { S_BM_WAS_FROZEN, "%s" },
  { S_BM_IS_FAST_ASLEEP, "%s" },
  { S_BM_IS_FROZEN_SOLID, "%s" },
  { S_BM_CAN_T_MOVE, "%s" },
  { S_BM_FLINCHED, "%s" },
  { S_BM_IS_CONFUSED, "%s" },
  { S_BM_MUST_RECHARGE, "%s" },
  { S_BM_WON_T_LISTEN, "%s" },
  { S_BM_IS_LOAFING_AROUND, "%s" },
  { S_BM_IS_IN_LOVE, "%s" },
  { S_BM_IGNORES_YOU, "%s" },
  { S_BM_DID_INSTEAD, "%s,%s" },
  { S_BM_USED, "%s,%s" },
  { S_BM_FLEW_UP_HIGH, "%s" },
  { S_BM_SPRANG_UP, "%s" },
  { S_BM_BURROWED_ITS_WAY_UNDER, "%s" },
  { S_BM_HID_UNDERWATER, "%s" },
  { S_BM_VANISHED_INSTANTLY, "%s" },
  { S_BM_TOOK_IN_SUNLIGHT, "%s" },
  { S_BM_WHIPPED_UP_A_WHIRLWIND, "%s" },
  { S_BM_BECAME_CLOAKED_IN_A, "%s" },
  { S_BM_TUCKED_IN_ITS_HEAD, "%s" },
  { S_BM_IS_OVERFLOWING_WITH_SPACE, "%s" },
  { S_BM_IS_ABSORBING_POWER, "%s" },
  { S_BM_ABSORBED_ELECTRICITY, "%s" },
  { S_BM_FORESAW_AN_ATTACK, "%s" },
  { S_BM_CHOSE_DOOM_DESIRE_AS, "%s" },
  { S_BM_HURT_ITSELF, "%s" },
  { S_BM_CAN_T_USE_IT, "%s" },
  { S_BM_IS_PREVENTED_FROM_HEALING, "%s" },
  { S_BM_TRANSFORMED_INTO, "%s,%s" },
  { S_BM_MEGA_EVOLVED, "%s" },
  { S_BM_FOE_MEGA_EVOLVED, "%s" },
  { S_BM_MAGNITUDE, "%u" },
  { S_BM_IS_CHARGING_UP, "%s" },
  { S_BM_IS_STORING_ENERGY, "%s" },
  { S_BM_PROTECTED_ITSELF, "%s" },
  { S_BM_MOVE_IS_DISABLED, "%s" },
  { S_BM_MISSED, "%s" },
  { S_BM_REGAINED_HEALTH, "%s" },
  { S_BM_DRAINED_HEALTH, "%s" },
  { S_BM_IS_COVERED, "%s" },
  { S_BM_BRACED_ITSELF, "%s" },
  { S_BM_DUG_IN, "%s" },
  { S_BM_STATS_RESET, "%s" },
  { S_BM_COPIED_THE_STATS, "%s" },
  { S_BM_IS_FIRED_UP, "%s" },
  { S_BM_IS_FOCUSED, "%s" },
  { S_BM_TOOK_AIM, "%s" },
  { S_BM_WAS_SEEDED, "%s" },
  { S_BM_CURSED, "%s,%s" },
  { S_BM_WAS_PURIFIED, "%s" },
  { S_BM_BECAME_NIMBLE, "%s" },
  { S_BM_LANDED_TO_REST, "%s" },
  { S_BM_SHOOK_IT_OFF, "%s" },
  { S_BM_GOT_ITS_BERRY_BACK, "%s" },
  { S_BM_WAS_IDENTIFIED, "%s" },
  { S_BM_IS_BEING_SALTED, "%s" },
  { S_BM_MADE_A_VEIL, "%s" },
  { S_BM_WAS_DISABLED, "%s" },
  { S_BM_GOT_AN_ENCORE, "%s" },
  { S_BM_LOST_SOME_PP, "%s" },
  { S_BM_IS_TAKING_YOU_DOWN, "%s" },
  { S_BM_BEARS_A_GRUDGE, "%s" },
  { S_BM_SKETCHED_MOVE, "%s,%s" },
  { S_BM_FELL_FOR_THE_TAUNT, "%s" },
  { S_BM_WAS_TORMENTED, "%s" },
  { S_BM_SEALED_ITS_MOVES, "%s" },
  { S_BM_CAN_T_ESCAPE, "%s" },
  { S_BM_WAS_BLOWN_AWAY, "%s" },
  { S_BM_WAS_DRAGGED_OUT, "%s" },
  { S_BM_IS_SHROUDED_IN_MIST, "%s" },
  { S_BM_ABILITY_WAS_SUPPRESSED, "%s" },
  { S_BM_GOT_A_NEW_ABILITY, "%s" },
  { S_BM_PASSED_IT_ON, "%s" },
  { S_BM_GREW_DROWSY, "%s" },
  { S_BM_CAN_NO_LONGER_ESCAPE, "%s" },
  { S_BM_PUT_IN_A_SUBSTITUTE, "%s" },
  { S_BM_MADE_A_WISH, "%s" },
  { S_BM_CAN_T_USE_ITEMS, "%s" },
  { S_BM_WAS_PREVENTED_FROM_HEALING, "%s" },
  { S_BM_ROCKS_FLOAT_AROUND, "%s" },
  { S_BM_SPIKES_LITTER_SIDE, "%s" },
  { S_BM_BECAME_WATER_TYPE, "%s" },
  { S_BM_CAN_T_SLEEP_NOW, "%s" },
  { S_BM_FELL_IN_LOVE, "%s" },
  { S_BM_IS_FLOATING, "%s" },
  { S_BM_IS_BRACED_FOR_SPEED, "%s" },
  { S_BM_SWITCHED_ITEMS, "%s" },
  { S_BM_STOLE, "%s,%s" },
  { S_BM_LOST, "%s,%s" },
  { S_BM_KEPT_GOING_AND_CRASHED, "%s" },
  { S_BM_TOOK_RECOIL, "%s" },
  { S_BM_BECAME_CONFUSED, "%s" },
  { S_BM_ENCORE_ENDED, "%s" },
  { S_BM_WISH_CAME_TRUE, "%s" },
  { S_BM_PERISH_COUNT_FELL_TO, "%s,%u" },
  { S_BM_TOOK_THE_FUTURE_SIGHT, "%s" },
  { S_BM_IS_HURT_BY, "%s,%s" },
  { S_BM_RESTORED_HP, "%s,%s" },
  { S_BM_RESTORED_HP_2, "%s" },
  { S_BM_FAINTED, "%s" },
  { S_BM_CAN_T_GET_AWAY, "%s" },
  { S_BM_FOE_SENDS, "%s" },
  { S_BM_USED_ON, "%s,%s" },
  { S_BM_FOE_USED_ON, "%s,%s" },
  { S_BM_THREW_A, "%s" },
  { S_BM_WAS_CAUGHT, "%s,%s" },
  { S_BM_USED_2, "%s" },
  { S_BM_STAT_UP, "%s,%s" },
  { S_BM_STAT_DOWN, "%s,%s" },
  { S_POKEDEX_FMT,     "%u,%u" },   // vistos y TOTAL: el total dejo de ser 151
  { S_GYM_CHAMP,       "%s" },      // la region: ya no es siempre Kanto
  { S_NAME_FMT,        "%s,%s,%u" },
  { S_RELEASE_FMT,     "%s" },
  { S_GYM_BUDGET_FMT,  "%u,%u" },
  { S_GYM_WINS_FMT,    "%u,%u" },
  { S_GYM_GAIN_FMT,    "%s,%u" },
  { S_RECORD_FMT,      "%u" },
  { S_SCORE_FMT,       "%u" },
  { S_STREAK_DAYS_FMT, "%u" },
  { S_STREAK_FMT,      "%u,%u" },
  { S_INFO_FMT,        "%s,%lu" },
  { S_MEDALS_FMT,      "%d,%d" },
  { S_QUIZ_Q_TYPE,     "%s" },
  { S_QUIZ_Q_EVO,      "%s" },
  { S_QUIZ_Q_MOVE,     "%s" },
  { S_QUIZ_Q_LEARN,    "%s" },
  { S_QUIZ_SCORE_FMT,  "%u,%u" },
  { S_PRIZE_FMT,       "%s" },
  { S_REC_FMT,         "%u" },
  { S_LVL_FMT,         "%u" },
  { S_NEXT_LVL_FMT,    "%u,%u" },
  { S_EVO_IN_FMT,      "%u" },
  { S_MISTAKES_FMT,    "%u" },
  { S_FAREWELL_BTN,    "%s" },
  { S_RUNAWAY_BTN,     "%s" },
  { S_CAP_LV_FMT,      "%u" },
  { S_CAP_FMT,         "%u" },
  { S_EXP_NEXT_FMT,    "%lu,%u" },
  { S_TR_EVOLUINDO,    "%s" },      // troca_ui: o nome de quem evolui
  { S_BAG_DARA,        "%s" },      // o nome do objeto
  { S_DU_ESCOLHA,      "%u" },      // quantos escolher
  { S_DU_FALTA,        "%u" },
  { S_DU_PLACAR,       "%u,%u,%u" },   // vitorias, derrotas, empates
  { S_BADGE_FROM,      "%s" },      // o lider (leva 71)
  { S_BADGE_TM,        "%s" },      // o golpe da MT
  // leva 73: os textos de tela que estavam fixos em ingles
  { S_CURADOS_FMT,     "%u" },
  { S_REV_SUB_FMT,     "%s,%u" },
  { S_MAPA_VENCE_FMT,  "%s" },
  { S_IR_PARA_FMT,     "%s" },
  { S_PRECISA_FMT,     "%s" },
  { S_OVO_FMT,         "%s" },
  { S_OUTROS_EXP_FMT,  "%lu" },
  { S_EM_QUEM_FMT,     "%s" },
  { S_RESTAM_FMT,      "%u" },
  { S_AV_FORA_FMT,     "%s" },
  { S_AV_SUBIU_FMT,    "%lu" },
  { S_AV_PROX_FMT,     "%u,%u" },
  { S_AV_SELVAGEM_FMT, "%s" },
  { S_AV_ENTROU_FMT,   "%s" },
  { S_AV_ACHOU_FMT,    "%s" },
};
static const int NUM_EXPECTED = (int)(sizeof(EXPECTED) / sizeof(EXPECTED[0]));

static const char *expectedSig(int id) {
  for (int i = 0; i < NUM_EXPECTED; i++)
    if ((int)EXPECTED[i].id == id) return EXPECTED[i].sig;
  return "(sin formato)";
}

TEST(i18n, ningun_idioma_tiene_huecos) {
  for (int lang = 0; lang < LANG_COUNT; lang++) {
    gLang = (Lang)lang;
    for (int id = 0; id < STR_COUNT; id++) {
      const char *s = T((StrId)id);
      char where[64];
      snprintf(where, sizeof(where), "[%s][id %d]", LANG_NAME[lang], id);
      CHECK_MSG(s != NULL, where);
      if (s) CHECK_MSG(s[0] != 0, std::string("cadena vacia ") + where);
    }
  }
  gLang = LANG_DEFAULT;
}

// La fuente del firmware (glcdfont.h de Arduino_GFX) es una tabla CP437 de 256
// glifos, asi que SI admite acentos, pero cada uno tiene que ocupar UN byte: la
// UI centra con strlen(t)*6, y un caracter UTF-8 de dos bytes descuadraria la
// etiqueta. Este test no prohibe los bytes altos, prohibe el UTF-8.
TEST(i18n, los_acentos_van_en_un_solo_byte_no_en_utf8) {
  // secuencias de arranque de UTF-8 multibyte: 0xC2-0xDF, 0xE0-0xEF, 0xF0-0xF4
  auto arranqueUtf8 = [](unsigned char c) {
    return (c >= 0xC2 && c <= 0xDF) || (c >= 0xE0 && c <= 0xEF) || (c >= 0xF0 && c <= 0xF4);
  };
  for (int lang = 0; lang < LANG_COUNT; lang++) {
    if (LANG_IS_CJK(lang)) continue;  // los CJK van en UTF-8 a proposito, con fuente U8g2
    gLang = (Lang)lang;
    for (int id = 0; id < STR_COUNT; id++) {
      const char *s = T((StrId)id);
      if (!s) continue;
      for (const char *c = s; *c; c++) {
        unsigned char u = (unsigned char)*c;
        if (u < 128) continue;                 // ASCII: siempre vale
        if (!arranqueUtf8(u)) continue;        // byte alto suelto: es CP437, vale
        // un arranque UTF-8 solo es sospechoso si le sigue un byte de
        // continuacion (0x80-0xBF): entonces son dos glifos donde deberia ir uno
        unsigned char sig = (unsigned char)*(c + 1);
        char msg[200];
        snprintf(msg, sizeof(msg),
                 "parece UTF-8 (0x%02X 0x%02X) en [%s][id %d]: los acentos van en octal, "
                 "un byte por caracter, o se descuadra el centrado", u, sig, LANG_NAME[lang], id);
        CHECK_MSG(!(sig >= 0x80 && sig <= 0xBF), msg);
      }
    }
  }
  gLang = LANG_DEFAULT;
}

// un %u de mas o de menos en una traduccion es una lectura de basura de la pila
TEST(i18n, los_formatos_coinciden_en_todos_los_idiomas) {
  for (int id = 0; id < STR_COUNT; id++) {
    gLang = LANG_EN;
    std::vector<std::string> ref = specs(T((StrId)id));
    for (int lang = 0; lang < LANG_COUNT; lang++) {
      gLang = (Lang)lang;
      std::vector<std::string> got = specs(T((StrId)id));
      char msg[256];
      snprintf(msg, sizeof(msg), "[%s][id %d] \"%s\": %s en vez de %s (EN)", LANG_NAME[lang], id,
               T((StrId)id), joinSpecs(got).c_str(), joinSpecs(ref).c_str());
      CHECK_MSG(got == ref, msg);
    }
  }
  gLang = LANG_DEFAULT;
}

// ...y ademas tienen que cuadrar con lo que pasa el sketch en cada llamada
TEST(i18n, los_formatos_cuadran_con_las_llamadas_del_sketch) {
  for (int lang = 0; lang < LANG_COUNT; lang++) {
    gLang = (Lang)lang;
    for (int id = 0; id < STR_COUNT; id++) {
      std::string got = joinSpecs(specs(T((StrId)id)));
      const char *want = expectedSig(id);
      char msg[256];
      snprintf(msg, sizeof(msg), "[%s][id %d] \"%s\": %s, se esperaba %s", LANG_NAME[lang], id,
               T((StrId)id), got.c_str(), want);
      CHECK_MSG(got == want, msg);
    }
  }
  gLang = LANG_DEFAULT;
}

TEST(i18n, las_medallas_estan_en_todos_los_idiomas) {
  for (int lang = 0; lang < LANG_COUNT; lang++) {
    gLang = (Lang)lang;
    // los topes de longitud van en BYTES, que solo equivalen a caracteres con la
    // fuente CP437. En CJK cada kana ocupa 3 bytes y el limite real es de anchura
    // en pantalla, que aqui no se puede medir: eso se comprueba en la placa.
    const bool cjk = LANG_IS_CJK(lang);
    for (int m = 0; m < MED_COUNT; m++) {
      const char *n = medalName(m), *l = medalLabel(m), *d = medalDesc(m);
      char where[64];
      snprintf(where, sizeof(where), "[%s][medalla %d]", LANG_NAME[lang], m);
      CHECK_MSG(n && n[0], std::string("medalName vacio ") + where);
      CHECK_MSG(l && l[0], std::string("medalLabel vacio ") + where);
      CHECK_MSG(d && d[0], std::string("medalDesc vacio ") + where);
      // la etiqueta corta va en una casilla de la cuadricula de medallas
      if (!cjk && l) CHECK_MSG(strlen(l) <= 6, std::string("etiqueta demasiado larga ") + where + ": " + l);
      if (!cjk && n) CHECK_MSG(strlen(n) <= 12, std::string("nombre demasiado largo ") + where + ": " + n);
      if (!cjk && d) CHECK_MSG(strlen(d) <= 16, std::string("descripcion demasiado larga ") + where + ": " + d);
    }
  }
  gLang = LANG_DEFAULT;
}

// UN BYTE, UN GLIFO en las medallas.
//
// Antes esto pedia ASCII puro y no decia por que. El porque venia de
// creer que la fuente no tenia acentos, que es falso: glcdfont.h es una
// tabla CP437 entera. Con esa regla el espanol se quedaba sin la tilde de
// VINCULO y el portugues escribia LACO en vez de LACO con cedilla.
//
// Lo que si se rompe de verdad es la MEDIDA. Las tres tablas se dibujan
// con printT() y se miden con strlen()*6 -- ahi arriba hay tres
// comprobaciones de largo contra la casilla de la cuadricula --, asi que
// un caracter en UTF-8 serian dos bytes para un solo dibujo y todas esas
// cuentas saldrian mal.
//
// La frontera: los acentos de CP437 que se usan van de 0200 a 0264
// (c-cedilla, e, a, n~, y los dos codigos del til portugues). El UTF-8
// empieza a partir de 0302. Cortar en 0300 deja pasar la fuente y sigue
// cazando el UTF-8 pegado por error.
TEST(i18n, en_las_medallas_un_byte_es_un_glifo) {
  for (int lang = 0; lang < LANG_COUNT; lang++) {
    if (LANG_IS_CJK(lang)) continue;  // los CJK van en UTF-8 con fuente U8g2
    gLang = (Lang)lang;
    for (int m = 0; m < MED_COUNT; m++) {
      const char *tab[3] = { medalName(m), medalLabel(m), medalDesc(m) };
      for (int t = 0; t < 3; t++)
        for (const char *c = tab[t]; c && *c; c++)
          CHECK_MSG((unsigned char)*c < 0xC0,
                    std::string("UTF-8 en una medalla (dos bytes para un "
                                "glifo, y el largo se mide con strlen): ") +
                    tab[t]);
    }
  }
  gLang = LANG_DEFAULT;
}

TEST(i18n, el_idioma_se_guarda_y_se_recupera) {
  mockNvsReset();
  setLang(LANG_DE);
  CHECK_EQ((int)gLang, (int)LANG_DE);
  gLang = LANG_ES;
  loadLang();
  CHECK_EQ((int)gLang, (int)LANG_DE);
  gLang = LANG_DEFAULT;
}

TEST(i18n, sin_ajuste_guardado_arranca_en_el_idioma_por_defecto) {
  mockNvsReset();
  gLang = LANG_IT;
  loadLang();
  CHECK_EQ((int)gLang, (int)LANG_DEFAULT);
  gLang = LANG_DEFAULT;
}

TEST(i18n, un_idioma_invalido_en_la_nvs_no_rompe_nada) {
  mockNvsReset();
  Preferences p;
  p.begin("tamapoke", false);
  p.putUChar("lang", 200);  // valor corrupto
  p.end();
  gLang = LANG_ES;
  loadLang();
  CHECK_EQ((int)gLang, (int)LANG_DEFAULT);
  gLang = LANG_DEFAULT;
}

TEST(i18n, setLang_rechaza_valores_fuera_de_rango) {
  mockNvsReset();
  setLang(LANG_FR);
  setLang((Lang)LANG_COUNT);
  CHECK_EQ((int)gLang, (int)LANG_FR);
  setLang((Lang)250);
  CHECK_EQ((int)gLang, (int)LANG_FR);
  gLang = LANG_DEFAULT;
}
