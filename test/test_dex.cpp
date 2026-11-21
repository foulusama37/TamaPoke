// Integridad de la tabla de la Pokedex (dex.h, generada por tools/gen_dex.py).
#include "framework.h"
#include "shim/Arduino.h"
#include "../dex.h"
#include "../i18n.h"
#include "../moves.h"
#include "../pet.h"
#include "../stones.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <set>

static const int N = DEX_COUNT;

// ---------------------------------------------------------------- golpes
TEST(golpes, la_tabla_es_coherente) {
  for (int i = 0; i < MOVE_COUNT; i++) {
    const MoveEntry &m = MOVE_TBL[i];
    CHECK(m.name != nullptr);
    CHECK_RANGE((int)m.type, 1, (int)T_COUNT - 1);
    CHECK(m.pp > 0);
    CHECK(m.phys == 0 || m.phys == 1);
    // Potencia 0 = golpe de apoyo, y entonces TIENE que hacer algo: si no,
    // seria una tarjeta que al pulsarla no pasa nada. El generador los
    // descarta, pero mas vale comprobarlo aqui que descubrirlo jugando.
    if (m.power == 0)
      CHECK_MSG(m.fx != FX_NONE || m.sx != SX_NONE,
                "golpe sin potencia y sin efecto: no hace nada");
  }
}

TEST(golpes, hay_golpes_de_apoyo_y_no_son_mayoria) {
  int apoyo = 0;
  for (int i = 0; i < MOVE_COUNT; i++)
    if (MOVE_TBL[i].power == 0) apoyo++;
  CHECK(apoyo > 40);                       // la categoria existe de verdad
  CHECK(apoyo * 2 < MOVE_COUNT);           // pero el combate sigue siendo pegarse
}

TEST(golpes, los_nombres_caben_en_la_tarjeta_sin_rodar) {
  // La tarjeta de golpe deja ~200 px utiles y la fuente ocupa 6 px por char a
  // escala 1, 12 a escala 2. El nombre mas largo hoy es STOMPINGTANTRUM (15),
  // que son 180 px: entra. Si algun dia entra uno mas largo este test lo avisa
  // ANTES de que haya que recurrir a rodar el texto, que es justo lo que no
  // queremos en una pantalla de este tamano.
  for (int i = 0; i < MOVE_COUNT; i++) {
    size_t n = strlen(MOVE_TBL[i].name);
    CHECK(n >= 3 && n <= 16);
    CHECK((int)(n * 12) <= 200);
    for (size_t k = 0; k < n; k++) {
      char c = MOVE_TBL[i].name[k];
      CHECK((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'));
    }
  }
}

TEST(golpes, fisico_y_especial_ya_no_van_atados_al_tipo) {
  // En gen 1 la categoria salia del TIPO: todo lo normal era fisico y todo lo
  // de fuego especial. Desde gen 4 es por golpe, y estos son golpes modernos,
  // asi que tiene que haber tipos con golpes de las dos clases.
  bool fis[T_COUNT] = { false }, esp[T_COUNT] = { false };
  for (int i = 0; i < MOVE_COUNT; i++)
    (MOVE_TBL[i].phys ? fis : esp)[MOVE_TBL[i].type] = true;
  int mixtos = 0;
  for (int t = 1; t < T_COUNT; t++) if (fis[t] && esp[t]) mixtos++;
  CHECK(mixtos >= 8);
}

TEST(golpes, hay_golpes_de_los_tipos_modernos) {
  // si el filtro de gen 1 volviera por accidente, estos desaparecerian
  bool dark = false, steel = false, fairy = false;
  for (int i = 0; i < MOVE_COUNT; i++) {
    if (MOVE_TBL[i].type == T_DARK) dark = true;
    if (MOVE_TBL[i].type == T_STEEL) steel = true;
    if (MOVE_TBL[i].type == T_FAIRY) fairy = true;
  }
  CHECK(dark); CHECK(steel); CHECK(fairy);
}

TEST(golpes, el_indice_de_learnsets_es_coherente) {
  CHECK_EQ((int)LEARN_OFS[0], 0);
  for (int d = 1; d <= DEX_COUNT; d++) {
    CHECK(LEARN_OFS[d] >= LEARN_OFS[d - 1]);    // monotono
    CHECK(LEARN_OFS[d] > LEARN_OFS[d - 1]);     // nadie se queda sin nada
    for (uint16_t i = LEARN_OFS[d - 1]; i < LEARN_OFS[d]; i++) {
      CHECK_RANGE((int)LEARN_MV[i], 0, MOVE_COUNT - 1);
      if (i > LEARN_OFS[d - 1])
        CHECK(LEARN_LVL[i] >= LEARN_LVL[i - 1]);  // ordenado por nivel
    }
  }
  CHECK_EQ((int)LEARN_OFS[DEX_COUNT], (int)(sizeof(LEARN_LVL) / sizeof(LEARN_LVL[0])));
  CHECK_EQ((int)LEARN_OFS[DEX_COUNT], (int)(sizeof(LEARN_MV) / sizeof(LEARN_MV[0])));
}

TEST(golpes, toda_especie_sabe_algo_a_cualquier_nivel) {
  uint16_t mv[4];
  for (int d = 1; d <= DEX_COUNT; d++) {
    for (uint16_t lv : { (uint16_t)1, (uint16_t)5, (uint16_t)25, (uint16_t)100, (uint16_t)999 }) {
      uint8_t n = movesAtLevel(d, lv, mv);
      CHECK_RANGE((int)n, 1, 4);
      for (uint8_t k = 0; k < n; k++) CHECK_RANGE((int)mv[k], 0, MOVE_COUNT - 1);
    }
  }
}

TEST(golpes, nadie_embarca_sabiendo_solo_forcejeo) {
  // Este test usaba al Ditto como ejemplo de "sin nada que hacer dano" y
  // comprobaba que le quedaba Forcejeo. El ejemplo era en realidad un
  // FALLO: Transformacion es lo unico que aprende y se caia de la tabla,
  // asi que el bicho no podia pelear. Ahora se comprueba lo contrario, y
  // para TODAS las especies, que es lo que habria avisado a tiempo.
  uint16_t mv[4];
  int mudos = 0;
  for (int16_t d = 1; d <= DEX_COUNT; d++) {
    uint8_t n = movesAtLevel(d, 100, mv);
    if (n == 1 && mv[0] == MOVE_STRUGGLE) mudos++;
  }
  CHECK_MSG(mudos == 0,
            std::string("hay ") + std::to_string(mudos) +
            " especies que a nivel 100 solo saben Forcejeo");
  // Y Forcejeo sigue siendo la red: existe, es normal y no se gasta.
  CHECK_EQ((int)MOVE_TBL[MOVE_STRUGGLE].type, (int)T_NORMAL);
  CHECK_EQ((int)movePP(MOVE_STRUGGLE), 255);
}

TEST(golpes, el_pp_es_el_del_pbs_tal_cual) {
  // Se abandono la conversion a "usos por combate": PP es un termino que no
  // hay que explicar, y como ahora atraviesa los combates, los numeros de
  // siempre si limitan algo.
  for (int i = 0; i < MOVE_COUNT; i++) {
    if (i == MOVE_STRUGGLE) continue;
    CHECK_EQ((int)movePP(i), (int)MOVE_TBL[i].pp);
    CHECK_RANGE((int)movePP(i), 1, 64);
  }
  CHECK_EQ((int)movePP(MOVE_COUNT), 5);      // fuera de rango, sin reventar
  CHECK_EQ((int)movePP(MOVE_COUNT + 500), 5);
}

TEST(golpes, ningun_golpe_se_queda_en_un_solo_uso) {
  // Con un unico uso, a nivel alto (donde no tumbas de un golpe) y sobre todo
  // en 3x3 o 6x6, el golpe fuerte seria inservible: lo gastas en el primer
  // rival y te quedas sin el.
  for (int i = 0; i < MOVE_COUNT; i++) CHECK(movePP(i) >= 2);
}

TEST(golpes, el_forcejeo_no_se_gasta) {
  // es el ultimo recurso: si tuviera tope, Ditto se quedaria sin poder actuar
  CHECK_EQ((int)movePP(MOVE_STRUGGLE), 255);
}

TEST(golpes, se_aprenden_mas_al_subir_de_nivel) {
  uint16_t a[4], b[4];
  uint8_t n1 = movesAtLevel(6, 1, a);     // Charizard recien salido
  uint8_t n2 = movesAtLevel(6, 60, b);
  CHECK(n2 >= n1);
  CHECK_EQ((int)n2, 4);                   // a nivel alto ya tiene los cuatro
}

TEST(golpes, no_se_repiten_en_los_cuatro) {
  uint16_t mv[4];
  for (int d = 1; d <= DEX_COUNT; d++) {
    uint8_t n = movesAtLevel(d, 100, mv);
    for (uint8_t i = 0; i < n; i++)
      for (uint8_t j = i + 1; j < n; j++) CHECK(mv[i] != mv[j]);
  }
}

TEST(golpes, fuera_de_rango_devuelve_forcejeo) {
  uint16_t mv[4];
  CHECK_EQ((int)movesAtLevel(0, 10, mv), 1);
  CHECK_EQ((int)mv[0], MOVE_STRUGGLE);
  CHECK_EQ((int)movesAtLevel(DEX_COUNT + 1, 10, mv), 1);
  CHECK_EQ((int)movesAtLevel(-3, 10, mv), 1);
}

TEST(golpes, el_tipo_del_golpe_funciona_con_la_matriz) {
  // un golpe de agua contra Charizard (fuego/volador) tiene que ser x2
  for (int i = 0; i < MOVE_COUNT; i++) {
    if (MOVE_TBL[i].type != T_WATER) continue;
    CHECK_EQ((int)typeEffectVs(MOVE_TBL[i].type, 6), 200);
    break;
  }
}

// ---------------------------------------------------------------- tipos
TEST(tipos, la_matriz_es_cuadrada_y_solo_tiene_valores_validos) {
  CHECK_EQ((int)(sizeof(TYPE_CHART) / sizeof(TYPE_CHART[0])), (int)T_COUNT);
  for (int a = 0; a < T_COUNT; a++) {
    CHECK_EQ((int)(sizeof(TYPE_CHART[a]) / sizeof(TYPE_CHART[a][0])), (int)T_COUNT);
    for (int d = 0; d < T_COUNT; d++) {
      uint8_t v = TYPE_CHART[a][d];
      CHECK(v == 0 || v == 50 || v == 100 || v == 200);
    }
  }
}

TEST(tipos, T_NONE_es_neutro_en_los_dos_lados) {
  for (int t = 0; t < T_COUNT; t++) {
    CHECK_EQ((int)TYPE_CHART[T_NONE][t], 100);
    CHECK_EQ((int)TYPE_CHART[t][T_NONE], 100);
  }
}

TEST(tipos, el_triangulo_clasico) {
  CHECK_EQ((int)typeEffect(T_WATER, T_FIRE, T_NONE), 200);
  CHECK_EQ((int)typeEffect(T_FIRE, T_GRASS, T_NONE), 200);
  CHECK_EQ((int)typeEffect(T_GRASS, T_WATER, T_NONE), 200);
  CHECK_EQ((int)typeEffect(T_FIRE, T_WATER, T_NONE), 50);
  CHECK_EQ((int)typeEffect(T_GRASS, T_FIRE, T_NONE), 50);
  CHECK_EQ((int)typeEffect(T_WATER, T_GRASS, T_NONE), 50);
}

TEST(tipos, las_inmunidades) {
  CHECK_EQ((int)typeEffect(T_ELEC, T_GROUND, T_NONE), 0);   // tierra ignora el rayo
  CHECK_EQ((int)typeEffect(T_NORMAL, T_GHOST, T_NONE), 0);
  CHECK_EQ((int)typeEffect(T_FIGHT, T_GHOST, T_NONE), 0);
  CHECK_EQ((int)typeEffect(T_GROUND, T_FLYING, T_NONE), 0);
}

TEST(tipos, los_dos_tipos_se_multiplican) {
  // Gyarados es agua/volador: el rayo pega x2 por cada mitad = x4
  CHECK_EQ((int)typeEffectVs(T_ELEC, 130), 400);
  // Charizard fuego/volador: roca x2 y x2 = x4, el clasico
  CHECK_EQ((int)typeEffectVs(T_ROCK, 6), 400);
  // Dragonite dragon/volador: hielo x2 y x2
  CHECK_EQ((int)typeEffectVs(T_ICE, 149), 400);
  // ...y tierra no le hace NADA a Charizard aunque el fuego sea debil a tierra:
  // volador anula, y 200 * 0 = 0
  CHECK_EQ((int)typeEffectVs(T_GROUND, 6), 0);
  // Parasect bicho/planta contra fuego: x2 por cada mitad
  CHECK_EQ((int)typeEffectVs(T_FIRE, 47), 400);
}

TEST(tipos, un_cuarto_cuando_las_dos_mitades_resisten) {
  // Bulbasaur planta/veneno: el agua la resisten planta (x0,5) y veneno (x1)
  CHECK_EQ((int)typeEffectVs(T_WATER, 1), 50);
  // Gengar fantasma/veneno contra veneno: x0,5 * x0,5 = x0,25
  CHECK_EQ((int)typeEffectVs(T_POISON, 94), 25);
}

TEST(tipos, la_tabla_es_la_moderna_no_la_de_gen1) {
  // estas cuatro son justo las que cambiaron de gen 1 a hoy; si alguien pone
  // MODERNO = False en tools/type_chart.py, este test lo cantara
  CHECK_EQ((int)typeEffect(T_POISON, T_BUG, T_NONE), 100);  // en gen 1 era 200
  CHECK_EQ((int)typeEffect(T_BUG, T_POISON, T_NONE), 50);   // en gen 1 era 200
  CHECK_EQ((int)typeEffect(T_ICE, T_FIRE, T_NONE), 50);     // en gen 1 era 100
  CHECK_EQ((int)typeEffect(T_GHOST, T_PSYCHIC, T_NONE), 200);  // en gen 1, 0 por un bug
}

TEST(tipos, los_tres_tipos_nuevos_funcionan) {
  CHECK_EQ((int)typeEffect(T_DARK, T_PSYCHIC, T_NONE), 200);
  CHECK_EQ((int)typeEffect(T_PSYCHIC, T_DARK, T_NONE), 0);     // siniestro es inmune
  CHECK_EQ((int)typeEffect(T_FAIRY, T_DRAGON, T_NONE), 200);
  CHECK_EQ((int)typeEffect(T_DRAGON, T_FAIRY, T_NONE), 0);     // hada es inmune
  CHECK_EQ((int)typeEffect(T_POISON, T_STEEL, T_NONE), 0);     // acero es inmune
  CHECK_EQ((int)typeEffect(T_FIGHT, T_STEEL, T_NONE), 200);
}

TEST(tipos, toda_especie_tiene_tipo1_valido_y_type2_distinto) {
  int dobles = 0;
  for (int d = 1; d <= DEX_COUNT; d++) {
    const DexEntry &e = DEX_TBL[d];
    CHECK_RANGE((int)e.type1, 1, (int)T_COUNT - 1);   // type1 nunca es T_NONE
    CHECK_RANGE((int)e.type2, 0, (int)T_COUNT - 1);
    if (e.type2) {
      CHECK(e.type1 != e.type2);                      // nada de "fuego/fuego"
      dobles++;
    }
  }
  // Una PROPORCION y no una cifra exacta: lo que esto vigila es que
  // gen_types.py no se quede a medias y deje a media Pokedex de tipo
  // unico, y para eso no hace falta saber el numero de memoria. El 116
  // que habia aqui habia que corregirlo en cada region (67 con Kanto,
  // 116 con Johto, 182 con Hoenn), que es ruido, no cobertura.
  CHECK_RANGE(dobles, DEX_COUNT * 35 / 100, DEX_COUNT * 60 / 100);
}

TEST(tipos, la_tipificacion_es_la_moderna) {
  // Estas 7 cambiaron de tipo despues de gen 1. Se usa el tipo de HOY porque
  // el juego tiene golpes de hada y de acero: dejar a Clefairy como normal
  // mientras existe PLAYROUGH no tendria sentido.
  CHECK_EQ((int)DEX_TBL[35].type1, (int)T_FAIRY);    // Clefairy
  CHECK_EQ((int)DEX_TBL[36].type1, (int)T_FAIRY);    // Clefable
  CHECK_EQ((int)DEX_TBL[39].type2, (int)T_FAIRY);    // Jigglypuff, normal/hada
  CHECK_EQ((int)DEX_TBL[81].type1, (int)T_ELEC);     // Magnemite, electrico/acero
  CHECK_EQ((int)DEX_TBL[81].type2, (int)T_STEEL);
  CHECK_EQ((int)DEX_TBL[122].type2, (int)T_FAIRY);   // Mr. Mime, psiquico/hada
}

TEST(tipos, hay_volador_que_el_tipo_viejo_de_dex_data_no_tenia) {
  CHECK_EQ((int)DEX_TBL[6].type2, (int)T_FLYING);    // Charizard
  CHECK_EQ((int)DEX_TBL[16].type2, (int)T_FLYING);   // Pidgey
  CHECK_EQ((int)DEX_TBL[130].type2, (int)T_FLYING);  // Gyarados
  CHECK_EQ((int)DEX_TBL[145].type2, (int)T_FLYING);  // Zapdos
}

TEST(tipos, fuera_de_rango_no_revienta) {
  CHECK_EQ((int)typeEffect(T_COUNT, T_FIRE, T_NONE), 100);
  CHECK_EQ((int)typeEffect(T_FIRE, T_COUNT, T_NONE), 100);
  CHECK_EQ((int)typeEffectVs(T_FIRE, 0), 100);
  CHECK_EQ((int)typeEffectVs(T_FIRE, DEX_COUNT + 1), 100);
  CHECK_EQ((int)typeEffectVs(T_FIRE, -5), 100);
}

TEST(dex, la_tabla_tiene_152_entradas) {
  CHECK_EQ((int)(sizeof(DEX_TBL) / sizeof(DEX_TBL[0])), N + 1);
  CHECK_EQ((int)(sizeof(DEX_NAME_FR) / sizeof(DEX_NAME_FR[0])), N + 1);
  CHECK_EQ((int)(sizeof(DEX_NAME_DE) / sizeof(DEX_NAME_DE[0])), N + 1);
}

TEST(dex, todos_los_nombres_son_ascii_en_mayusculas) {
  for (int d = 1; d <= N; d++) {
    const char *n = DEX_TBL[d].name;
    CHECK_MSG(n != NULL && n[0] != 0, "entrada sin nombre");
    if (!n || !n[0]) continue;
    for (const char *c = n; *c; c++) {
      // la fuente GFX del firmware es ASCII y sin minusculas
      CHECK_MSG((unsigned char)*c < 128, std::string("caracter no ASCII en ") + n);
      CHECK_MSG(!islower((unsigned char)*c), std::string("minuscula en ") + n);
    }
  }
}

TEST(dex, los_nombres_traducidos_tambien_son_ascii) {
  const char *const *tables[2] = { DEX_NAME_FR, DEX_NAME_DE };
  for (int t = 0; t < 2; t++) {
    for (int d = 1; d <= N; d++) {
      const char *n = tables[t][d];
      if (!n) continue;  // sin nombre propio: cae al de DEX_TBL
      CHECK_MSG(n[0] != 0, "nombre traducido vacio");
      for (const char *c = n; *c; c++) {
        CHECK_MSG((unsigned char)*c < 128, std::string("caracter no ASCII en ") + n);
        CHECK_MSG(!islower((unsigned char)*c), std::string("minuscula en ") + n);
      }
    }
  }
}

// La cabecera de la ficha es "%s%s Lv.%u": ningun nombre en ningun idioma
// puede desbordarla, o se cortaria en pantalla.
//
// El tamano sale de DEX_HEAD_BUF (dex.h), que lo calcula gen_dex.py del
// nombre mas largo que hay de verdad. Estaba a mano aqui (26) y en los
// dos sitios que la pintan (28 y 32), tres numeros que tenian que cuadrar
// entre si: con Kalos los nombres japoneses de seis silabas son 18 bytes
// en UTF-8 y se pasaron de 26.
TEST(dex, ningun_nombre_desborda_la_cabecera) {
  char head[DEX_HEAD_BUF];
  for (int lang = 0; lang < LANG_COUNT; lang++) {
    gLang = (Lang)lang;
    for (int d = 1; d <= N; d++) {
      int need = snprintf(head, sizeof(head), T(S_NAME_FMT), "*", dexName(d), 255);
      CHECK_MSG(need >= 0 && need < (int)sizeof(head),
                std::string("cabecera cortada: ") + dexName(d));
    }
  }
  gLang = LANG_DEFAULT;
}

TEST(dex, las_evoluciones_apuntan_a_entradas_validas) {
  for (int d = 1; d <= N; d++) {
    uint8_t to = DEX_TBL[d].evolvesTo;
    CHECK_MSG(to <= N, std::string("evolvesTo fuera de rango en ") + DEX_TBL[d].name);
    CHECK_MSG(to != d, std::string("se evoluciona a si mismo: ") + DEX_TBL[d].name);
  }
}

TEST(dex, evolucionar_y_nivel_van_de_la_mano) {
  // Apuntar a alguien sin decir cuando, o decir cuando sin apuntar a
  // nadie, deja una evolucion que no ocurre jamas. El "cuando" son DOS
  // cosas desde que los niveles salen del PBS del Anil: un nivel, o un
  // objeto. Pikachu, Eevee, Gloom y once mas tienen destino y nivel 0
  // porque el Anil no les da ningun camino por nivel, solo la piedra.
  for (int d = 1; d <= N; d++) {
    bool evolves = DEX_TBL[d].evolvesTo != 0;
    bool comoSea = DEX_TBL[d].evolveLevel > 0 || evoHasItem(d);
    CHECK_MSG(evolves == comoSea,
              std::string("evolvesTo y evolveLevel no cuadran en ") + DEX_TBL[d].name);
  }
}

TEST(dex, ninguna_cadena_evolutiva_hace_bucle) {
  for (int d = 1; d <= N; d++) {
    std::set<int> seen;
    int cur = d;
    int guard = 0;
    while (cur >= 1 && cur <= N && DEX_TBL[cur].evolvesTo != 0 && guard++ < N) {
      CHECK_MSG(seen.insert(cur).second, std::string("bucle evolutivo en ") + DEX_TBL[d].name);
      if (!seen.count(cur)) break;
      cur = DEX_TBL[cur].evolvesTo;
    }
    CHECK_MSG(guard < 6, std::string("cadena demasiado larga desde ") + DEX_TBL[d].name);
  }
}

TEST(dex, los_niveles_de_evolucion_suben_a_lo_largo_de_la_cadena) {
  for (int d = 1; d <= N; d++) {
    // Los escalones por OBJETO no llevan nivel (valen 0) y no cuentan: lo
    // que se mira es que los que SI tienen nivel no vayan hacia atras.
    // Sin esto, Gloom (0, por piedra) rompia la cadena de Oddish (21).
    int cur = d, prev = 0, guard = 0;
    while (DEX_TBL[cur].evolvesTo != 0 && guard++ < 6) {
      if (DEX_TBL[cur].evolveLevel) {
        CHECK_MSG(DEX_TBL[cur].evolveLevel > prev,
                  std::string("nivel de evolucion no creciente en ") + DEX_TBL[cur].name);
        prev = DEX_TBL[cur].evolveLevel;
      }
      cur = DEX_TBL[cur].evolvesTo;
    }
  }
}

// una especie que solo sale por evolucion tiene que ser alcanzable desde algun
// huevo, o seria imposible de registrar en la pokedex
TEST(dex, toda_especie_es_alcanzable) {
  // Se sigue `evolvesTo` Y la tabla de piedras: desde Johto hay especies
  // que son la SEGUNDA rama de alguien (Bellossom, Politoed, Espeon,
  // Umbreon) y `evolvesTo`, que guarda un solo destino, no las nombra.
  // Mirando solo ese campo el test las daria por inalcanzables aunque la
  // piedra llegue a ellas sin problema.
  bool reach[DEX_COUNT + 1] = { false };
  for (int d = 1; d <= N; d++)
    if (DEX_TBL[d].rarity != R_EVO) reach[d] = true;  // sale de huevo

  // cierre: se repite hasta que no se marque a nadie nuevo
  for (int vuelta = 0; vuelta < 8; vuelta++) {
    bool cambio = false;
    auto marca = [&](int d) {
      if (d >= 1 && d <= N && !reach[d]) { reach[d] = true; cambio = true; }
    };
    for (int d = 1; d <= N; d++) {
      if (!reach[d]) continue;
      marca(DEX_TBL[d].evolvesTo);
      // La rama del MACHO: `evolvesTo` guarda la de la hembra, asi que
      // sin mirar aqui Mothim parecia inalcanzable (y estuvo puesto en
      // la hierba de apano hasta que hubo sexo).
      marca(dexMaleBranch(d));
      for (uint8_t i = 0; i < STONE_EVO_COUNT; i++)
        if (STONE_EVO[i].from == d) marca(STONE_EVO[i].to);
    }
    if (!cambio) break;
  }
  for (int d = 1; d <= N; d++)
    CHECK_MSG(reach[d], std::string("inalcanzable: ") + DEX_TBL[d].name);
}

// Y al reves: lo que SOLO sale evolucionando no puede salir tambien de un
// huevo, o la piedra no habria servido para nada.
TEST(dex, las_ramas_de_piedra_no_salen_tambien_de_huevo) {
  for (uint8_t i = 0; i < STONE_EVO_COUNT; i++) {
    int16_t to = STONE_EVO[i].to;
    // Salvo los que estan en ALSO_WILD, que es una decision tomada: una
    // Chansey sale en la hierba desde siempre y ADEMAS la da un Happiny
    // con la Piedra Oval, igual que en los juegos. El objeto no sobra por
    // eso: es el camino corto, no el unico.
    if (dexAlsoWild(to)) continue;
    CHECK_MSG(DEX_TBL[to].rarity == R_EVO,
              std::string(DEX_TBL[to].name) + " se consigue con piedra Y de huevo");
  }
}

// con la despedida a los 3 dias (nivel 72), toda linea tiene que poder llegar a
// su forma final antes de que la mascota cumpla el ciclo
TEST(dex, toda_linea_se_completa_antes_de_la_despedida) {
  const uint32_t maxLevel = FAREWELL_AGE_MIN / MINUTES_PER_LEVEL;
  for (int d = 1; d <= N; d++) {
    if (DEX_TBL[d].rarity == R_EVO) continue;
    int cur = d, guard = 0;
    uint32_t need = 0;
    while (DEX_TBL[cur].evolvesTo != 0 && guard++ < 6) {
      if (DEX_TBL[cur].evolveLevel > need) need = DEX_TBL[cur].evolveLevel;
      cur = DEX_TBL[cur].evolvesTo;
    }
    CHECK_MSG(need <= maxLevel,
              std::string("la linea de ") + DEX_TBL[d].name + " no llega a forma final a tiempo");
  }
}

TEST(dex, las_stats_base_son_plausibles) {
  for (int d = 1; d <= N; d++) {
    const DexEntry &e = DEX_TBL[d];
    CHECK_MSG(e.bHp > 0 && e.bAtk > 0 && e.bDef > 0 && e.bSpe > 0,
              std::string("stat base a cero en ") + e.name);
    CHECK_MSG(e.biome <= 5, std::string("bioma desconocido en ") + e.name);
    CHECK_MSG(e.rarity <= R_LEGENDARIO, std::string("rareza desconocida en ") + e.name);
  }
}

TEST(dex, la_rama_de_eevee_cuadra_con_el_codigo) {
  CHECK_EQ((int)DEX_TBL[DEX_EEVEE].rarity, (int)R_COMUN);
  CHECK_RANGE((int)DEX_TBL[DEX_EEVEE].evolvesTo, 134, 136);
  for (int d = 134; d <= 136; d++) {
    CHECK_EQ((int)DEX_TBL[d].rarity, (int)R_EVO);
    CHECK_EQ((int)DEX_TBL[d].evolvesTo, 0);
  }
}

TEST(dex, los_iniciales_clasicos_son_validos) {
  CHECK_EQ((int)(sizeof(CLASSIC_DEX) / sizeof(CLASSIC_DEX[0])), NUM_CLASSIC_DEX);
  for (int i = 0; i < NUM_CLASSIC_DEX; i++) {
    int16_t d = CLASSIC_DEX[i];
    CHECK_RANGE(d, (int16_t)1, (int16_t)151);
    CHECK_MSG(DEX_TBL[d].rarity != R_EVO, "un inicial no puede ser una forma evolucionada");
  }
}

TEST(dex, hay_al_menos_una_especie_de_cada_rareza) {
  int count[4] = { 0, 0, 0, 0 };
  for (int d = 1; d <= N; d++) count[DEX_TBL[d].rarity]++;
  for (int r = 0; r <= R_LEGENDARIO; r++)
    CHECK_MSG(count[r] > 0, "hay una rareza sin especies: el sorteo del huevo se quedaria sin candidatos");
}

// Esto comprobaba que ningun tier pasara de 80 candidatos, porque
// pickEggSpecies los juntaba en un `int16_t cand[80]`. Ese array ya no
// existe (ahora sortea por reservorio, sin tope), asi que la cuenta de 80
// no vigilaba nada: lo unico que hacia era fallar cada vez que entra una
// region. Lo que SI hay que vigilar es la consecuencia que tenia aquel
// tope, que es que las especies del final de la dex no salieran nunca.
TEST(dex, el_sorteo_del_huevo_llega_al_final_de_la_dex) {
  const int16_t ULTIMA = DEX_REGION_END[DEX_REGIONS - 2];  // fin de la penultima
  bool lejos = false;
  for (int d = ULTIMA + 1; d <= N && !lejos; d++)
    if (DEX_TBL[d].rarity != R_EVO) lejos = true;
  CHECK_MSG(lejos, "la ultima region no aporta ni un candidato a huevo");
  // La otra mitad (que el sorteo LLEGUE hasta ahi) esta en test_pet.cpp,
  // que es donde vive makePet.
}

TEST(dex, dexName_cae_al_nombre_base_si_no_hay_traduccion) {
  gLang = LANG_ES;
  CHECK_STREQ(dexName(1), DEX_TBL[1].name);
  gLang = LANG_FR;
  for (int d = 1; d <= N; d++) {
    const char *want = DEX_NAME_FR[d] ? DEX_NAME_FR[d] : DEX_TBL[d].name;
    CHECK_STREQ(dexName(d), want);
  }
  gLang = LANG_DE;
  for (int d = 1; d <= N; d++) {
    const char *want = DEX_NAME_DE[d] ? DEX_NAME_DE[d] : DEX_TBL[d].name;
    CHECK_STREQ(dexName(d), want);
  }
  gLang = LANG_DEFAULT;
}

// EVOLUCIONAR NO PUEDE HACERTE MAS PEQUENO.
//
// Reportado jugando: "parece q ele desevoluiu, de tao pequeno que ficou".
// Medido entonces: 55 evoluciones salian en pantalla MAS PEQUENAS que su
// forma anterior —— Pidgeotto 147 px -> Pidgeot 128, Haunter -> Gengar,
// Golbat -> Crobat, Weedle -> Kakuna.
//
// La causa es que el tamano sale del alto del SPRITE del Anil, y el alto
// del sprite depende de la POSE: Pidgeotto esta dibujado volando (116 px
// de lienzo) y Pidgeot posado (68). Se mide la pose, no el bicho.
//
// Esto no comprueba centimetros —— discutir si un Gengar mide mas que un
// Haunter no lleva a ningun sitio ——, comprueba lo UNICO que el jugador da
// por hecho sin pensarlo: que al evolucionar se ve mas grande. El arreglo
// vive en tools/gen_sizes.py y se puede volver a perder en cuanto entre
// otra region con otras poses.
// El invariante "evolucionar no te hace mas pequeno" VIVE EN OTRO SITIO:
// test_sd.cpp, que carga los sprites de verdad.
//
// Aqui estuvo, comparando DEX_TBL[].screenSz, y dejo de valer cuando
// screenSz paso de ser un alto a ser una magnitud
// (alto^0,35 * sqrt(area)^0,65). Dos bichos con la misma magnitud y
// distinta forma ocupan distinto sitio: Pidgeotto tiene magnitud 130 y
// Pidgeot 126, y aun asi Pidgeot ocupa MAS, porque es mas ancho. El test
// marcaba ocho parejas buenas como malas —— o sea que empujaba a
// estropear datos correctos. Un test que afirma una premisa falsa es
// peor que no tenerlo.

TEST(dex, dexName_fuera_de_rango_no_revienta) {
  for (int lang = 0; lang < LANG_COUNT; lang++) {
    gLang = (Lang)lang;
    CHECK_STREQ(dexName(0), DEX_TBL[0].name);
    CHECK_STREQ(dexName(-1), DEX_TBL[0].name);
    CHECK_STREQ(dexName(DEX_COUNT + 1), DEX_TBL[0].name);
    CHECK_STREQ(dexName(30000), DEX_TBL[0].name);
  }
  gLang = LANG_DEFAULT;
}
