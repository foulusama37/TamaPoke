// La tarjeta, LEIDA POR EL FIRMWARE.
//
// Por que hace falta aparte de tools/auditoria.py: la auditoria abre los
// .bin con un parseador escrito en Python y dice si estan bien formados.
// Eso no prueba que el APARATO sepa pedirlos ni que su parseador los
// entienda igual. Son dos cosas distintas, y la diferencia costo cara:
//
//   PmdMon::load recibia el numero de dex en uint8_t. Desde Hoenn el
//   numero daba la vuelta EN SILENCIO y la especie 300 abria
//   /mons/p044.bin, que es un fichero perfectamente valido de otro
//   bicho. La auditoria lo daba por bueno — y tenia razon, el fichero
//   estaba bien. Lo que estaba mal era quien lo pedia.
//
// Aqui se compila sdmon.cpp DE VERDAD, con el shim de ficheros del
// simulador apuntando a tools/sdcard, y se le piden las 1025 especies,
// las 127 megas y las 61 formas una a una.
#include "framework.h"
#include "shim/Arduino.h"
#include <FS.h>        // mockSdRoot: la "SD" es una carpeta del PC
#include <SD_MMC.h>
#include "../sdmon.h"
#include "../dex.h"
#include "../megas.h"
#include "../forms.h"
#include "../female.h"   // hasFemale, FEMALE_COUNT
#include "../moves.h"
#include "../art.h"      // fxHasOwn: que golpes tienen animacion propia
#include "../retratos.h" // POR_ESPECIES / POR_RANURAS / os estados
#include "../bgs.h"
#include <algorithm>
#include <vector>
#include <filesystem>
#include <string>

// La carpeta que hace de tarjeta. El Makefile corre desde test/.
static const char *RAIZ_SD = "../tools/sdcard";

// Monta la "tarjeta" igual que el arranque del aparato. Hay que llamar
// a sdBegin(): sin el, `sdReady` es false y loadFile() devuelve false
// ANTES de mirar nada. Eso hacia que el test de "una fila que no existe
// no carga" pasara por el motivo equivocado — pasaba porque no cargaba
// NADA. Un test verde que no prueba lo que dice es peor que uno rojo.
static bool hayTarjeta() {
  mockSdRoot(RAIZ_SD);
  if (!SD_MMC.exists("/mons")) return false;
  if (!sdReady) {
    mockSerialSilence(true);
    sdBegin();
    mockSerialSilence(false);
  }
  return sdReady;
}

// Un PmdMon cargado y descargado en el acto: son ~300 KB cada uno y
// aqui se abren mil.
struct Prestado {
  PmdMon m;
  ~Prestado() { m.unload(); }
};

// ---------------------------------------------------- OS RETRATOS
// Carregados pelo codigo do firmware, do arquivo de verdade.

TEST(retratos, el_fichero_esta_y_lo_lee_el_codigo_del_firmware) {
  if (!hayTarjeta()) { CHECK_MSG(false, "sem tarjeta"); return; }
  porEspeciesEsperadas = POR_ESPECIES;
  porRanurasEsperadas = POR_RANURAS;
  porVersionEsperada = POR_VERSION;
  int cargados = 0;
  for (uint16_t d = 1; d <= DEX_COUNT; d += 37) {
    if (!porMon.load(d)) continue;
    cargados++;
    const Art *a = porMon.cara(POR_NORMAL);
    CHECK_MSG(a != nullptr, "o estado normal tem que existir sempre");
    CHECK_MSG(a->w > 8 && a->h > 8,
              "retrato minusculo em #" + std::to_string(d) + ": " +
                  std::to_string(a->w) + "x" + std::to_string(a->h));
    CHECK_MSG(a->rleLen > 0, "retrato sem dados em #" + std::to_string(d));
  }
  porMon.unload();
  CHECK_MSG(cargados > 20, "quase nada carregou: " + std::to_string(cargados));
}

TEST(retratos, cada_especie_carga_LA_SUYA_y_no_la_del_vecino) {
  // O medo de sempre com arquivo indexado por numero. Se o indice
  // estivesse deslocado, cada bicho mostraria a cara do vizinho e nada
  // avisaria —— so se percebe comparando os dados.
  if (!hayTarjeta()) { CHECK_MSG(false, "sem tarjeta"); return; }
  porEspeciesEsperadas = POR_ESPECIES;
  porRanurasEsperadas = POR_RANURAS;
  porVersionEsperada = POR_VERSION;
  int comparados = 0, iguais = 0;
  for (uint16_t d = 1; d + 1 <= DEX_COUNT; d += 53) {
    if (!porMon.load(d)) continue;
    const Art *a = porMon.cara(POR_NORMAL);
    if (!a) continue;
    uint16_t la = a->rleLen;
    std::vector<uint8_t> copia(a->rle, a->rle + la);
    if (!porMon.load((uint16_t)(d + 1))) continue;
    const Art *b = porMon.cara(POR_NORMAL);
    if (!b) continue;
    comparados++;
    if (b->rleLen == la &&
        std::equal(copia.begin(), copia.end(), b->rle))
      iguais++;
  }
  porMon.unload();
  CHECK_MSG(comparados > 10, "nao deu pra comparar: " +
                                 std::to_string(comparados));
  CHECK_MSG(iguais == 0,
            "dois vizinhos trouxeram O MESMO retrato " +
                std::to_string(iguais) + " vez(es): o indice esta torto");
}

TEST(retratos, los_estados_NO_son_todos_la_misma_cara) {
  // ESTE e o teste que vale. Se o mapa de estados nao funcionasse, cada
  // bicho teria uma cara so e os seis estados apontariam pra ela: a
  // maquinaria inteira —— empacotar seis, resolver a emocao, o indice
  // por estado —— seria enfeite, e tudo continuaria "verde".
  if (!hayTarjeta()) { CHECK_MSG(false, "sem tarjeta"); return; }
  porEspeciesEsperadas = POR_ESPECIES;
  porRanurasEsperadas = POR_RANURAS;
  porVersionEsperada = POR_VERSION;
  int mirados = 0, conVarias = 0, sujoDistinto = 0;
  for (uint16_t d = 1; d <= DEX_COUNT; d += 11) {
    if (!porMon.load(d)) continue;
    mirados++;
    if (porMon.n > 1) conVarias++;
    const Art *norm = porMon.cara(POR_NORMAL);
    const Art *sujo = porMon.cara(POR_SUJO);
    if (norm && sujo && norm != sujo) sujoDistinto++;
  }
  porMon.unload();
  CHECK_MSG(mirados > 50, "poucos bichos: " + std::to_string(mirados));
  CHECK_MSG(conVarias > mirados / 2,
            "quase ninguem tem mais de uma cara (" +
                std::to_string(conVarias) + " de " + std::to_string(mirados) +
                "): o empacotador nao esta resolvendo as emocoes");
  CHECK_MSG(sujoDistinto > mirados / 3,
            "o estado SUJO quase nunca difere do normal (" +
                std::to_string(sujoDistinto) + " de " +
                std::to_string(mirados) + ")");
}

TEST(retratos, la_linea_leida_es_la_de_ESA_especie) {
  // O teste de cima ("nao e a do vizinho") so compara vizinhos entre
  // si, e isso NAO basta: um indice deslocado uma casa, de forma
  // uniforme, faz cada bicho carregar o do vizinho e os vizinhos
  // continuam diferentes um do outro. Passaria verde.
  //
  // Aqui a verdade vem de FORA do leitor: POR_MUESTRA sai de quem
  // escreveu o arquivo (tools/gen_retratos.py) e diz, pra doze
  // especies espalhadas, quantas caras distintas elas tem e quanto
  // mede o RLE da cara normal. Deslocar uma casa derruba na hora.
  if (!hayTarjeta()) { CHECK_MSG(false, "sem tarjeta"); return; }
  porEspeciesEsperadas = POR_ESPECIES;
  porRanurasEsperadas = POR_RANURAS;
  porVersionEsperada = POR_VERSION;
  for (uint8_t i = 0; i < POR_MUESTRAS; i++) {
    uint16_t d = POR_MUESTRA[i].dex;
    CHECK_MSG(porMon.load(d), "nao carregou #" + std::to_string(d));
    CHECK_MSG(porMon.n == POR_MUESTRA[i].caras,
              "#" + std::to_string(d) + " tem " + std::to_string(porMon.n) +
                  " caras e o gerador escreveu " +
                  std::to_string(POR_MUESTRA[i].caras));
    const Art *a = porMon.cara(POR_NORMAL);
    CHECK_MSG(a != nullptr, "#" + std::to_string(d) + " sem cara normal");
    CHECK_MSG(a->rleLen == POR_MUESTRA[i].rle,
              "#" + std::to_string(d) + ": RLE de " +
                  std::to_string(a->rleLen) + " e o gerador escreveu " +
                  std::to_string(POR_MUESTRA[i].rle));
  }
  porMon.unload();
}

TEST(retratos, un_fichero_de_otra_version_se_rechaza_ENTERO) {
  // A assinatura. Recusar tudo e a falha BOA: fica-se sem retrato, que
  // se ve e se conserta, em vez de mostrar a cara de outro bicho.
  if (!hayTarjeta()) { CHECK_MSG(false, "sem tarjeta"); return; }
  porMon.unload();
  porEspeciesEsperadas = POR_ESPECIES + 1;      // como um cartao velho
  porRanurasEsperadas = POR_RANURAS;
  porVersionEsperada = POR_VERSION;
  mockSerialSilence(true);
  bool colou = porMon.load(25);
  mockSerialSilence(false);
  CHECK_MSG(!colou, "um arquivo de outra contagem TEM que ser recusado");

  porEspeciesEsperadas = POR_ESPECIES;
  porRanurasEsperadas = (uint8_t)(POR_RANURAS + 1);   // outro numero de estados
  mockSerialSilence(true);
  colou = porMon.load(25);
  mockSerialSilence(false);
  CHECK_MSG(!colou, "mudar os estados sem recopiar tambem tem que recusar");

  porEspeciesEsperadas = POR_ESPECIES;          // e com a firma certa, carrega
  porRanurasEsperadas = POR_RANURAS;
  porVersionEsperada = POR_VERSION;
  CHECK_MSG(porMon.load(25), "com a firma certa tem que carregar");
  porMon.unload();
}

TEST(retratos, el_shiny_tiene_su_PROPIO_retrato) {
  // O dono perguntou DUAS vezes por que os shiny nao usavam o retrato
  // deles. Usavam o normal porque eu so lia a pasta raiz. No
  // SpriteCollab o shiny da forma base vive em <dex>/0000/0001 e tem a
  // mesma silhueta com outras cores —— 1023 das 1025 especies tem.
  if (!hayTarjeta()) { CHECK_MSG(false, "sem tarjeta"); return; }
  porEspeciesEsperadas = POR_ESPECIES;
  porRanurasEsperadas = POR_RANURAS;
  porVersionEsperada = POR_VERSION;

  int comparados = 0, iguais = 0;
  for (uint16_t d = 1; d <= DEX_COUNT; d += 61) {
    if (!porMon.load(d, false)) continue;
    const Art *a = porMon.cara(POR_NORMAL);
    if (!a) continue;
    // A PALETA, nao o RLE. O RLE guarda indices de paleta, e um shiny e
    // o mesmo desenho recolorido: os indices sao legitimamente iguais e
    // o que muda sao as CORES. Comparando o RLE o teste acusava "o
    // shiny veio igual" em cima de dados corretos.
    std::vector<uint16_t> pnorm;
    for (uint8_t k = 0; k < 64 && a->pal[k]; k++) pnorm.push_back(a->pal[k]);

    if (!porMon.load(d, true)) continue;
    CHECK_MSG(porMon.shiny, "pediu shiny e veio o normal em #" +
                                std::to_string(d));
    const Art *b = porMon.cara(POR_NORMAL);
    if (!b) continue;
    comparados++;
    std::vector<uint16_t> pshi;
    for (uint8_t k = 0; k < 64 && b->pal[k]; k++) pshi.push_back(b->pal[k]);
    if (pnorm == pshi) iguais++;
  }
  porMon.unload();
  CHECK_MSG(comparados > 8, "poucos comparados: " + std::to_string(comparados));
  CHECK_MSG(iguais == 0,
            "o shiny veio IGUAL ao normal em " + std::to_string(iguais) +
                " de " + std::to_string(comparados) +
                ": nao esta lendo a pasta do shiny");
}

// As cores de uma cara (a paleta; o RLE e o mesmo desenho num shiny).
static std::vector<uint16_t> coresDe(const Art *a) {
  std::vector<uint16_t> v;
  for (uint8_t k = 0; a && k < 64 && a->pal[k]; k++) v.push_back(a->pal[k]);
  return v;
}

TEST(retratos, a_MEGA_ARCANINE_tem_cara_propria_e_shiny) {
  // O dono aprovou o retrato da Mega Arcanine (feito em cima do do
  // Arcanine) e pediu a shiny. Mede-se o EFEITO: a cara carregada para a
  // mega e OUTRA que a da especie, a shiny e outra que a normal, e um
  // pedido da forma errada nao carrega nada.
  if (!hayTarjeta()) { CHECK_MSG(false, "sem tarjeta"); return; }
  porEspeciesEsperadas = POR_ESPECIES;
  porRanurasEsperadas = POR_RANURAS;
  porVersionEsperada = POR_VERSION;
  int16_t fila = -1;
  for (int16_t i = 0; i < (int16_t)MEGA_COUNT; i++)
    if (MEGA_TBL[i].base == 59) fila = i;
  CHECK_MSG(fila >= 0, "premissa: a Mega Arcanine esta na tabela");
  if (fila < 0) return;
  const uint8_t forma = MEGA_TBL[fila].form;

  CHECK(porMon.load(59, false));
  std::vector<uint16_t> especie = coresDe(porMon.cara(POR_NORMAL));
  CHECK_MSG(porMon.loadMega(59, forma, false) && porMon.mega == forma && !porMon.shiny,
            "a mega normal nao carrega");
  std::vector<uint16_t> mega = coresDe(porMon.cara(POR_NORMAL));
  std::vector<uint16_t> festa = coresDe(porMon.cara(POR_FESTA));
  CHECK_MSG(porMon.loadMega(59, forma, true) && porMon.shiny, "a mega shiny nao carrega");
  std::vector<uint16_t> megaSh = coresDe(porMon.cara(POR_NORMAL));
  CHECK_MSG(!mega.empty() && mega != especie, "a mega veio com a cara do Arcanine");
  CHECK_MSG(!megaSh.empty() && megaSh != mega, "a mega shiny veio igual a normal");
  CHECK_MSG(!festa.empty() && festa != mega, "a cara de festa e a mesma da normal");
  // forma errada e especie errada: recusados, e nada fica carregado
  CHECK(!porMon.loadMega(59, (uint8_t)(forma + 1), false));
  CHECK(!porMon.blob);
  CHECK(!porMon.loadMega(58, forma, false));
  // e o retrato da especie volta normalmente depois
  CHECK(porMon.load(59, false) && !porMon.mega);
  porMon.unload();
}

TEST(retratos, un_cartao_da_VERSAO_antiga_se_recusa) {
  // A versao 1 tinha METADE das entradas na tabela de deslocamentos
  // (uma por especie, nao duas). Lido como se fosse esta, cada bicho
  // pegaria o deslocamento do vizinho —— e em silencio, que e o pior
  // jeito de errar.
  if (!hayTarjeta()) { CHECK_MSG(false, "sem tarjeta"); return; }
  porMon.unload();
  porEspeciesEsperadas = POR_ESPECIES;
  porRanurasEsperadas = POR_RANURAS;
  porVersionEsperada = (uint8_t)(POR_VERSION + 1);
  mockSerialSilence(true);
  bool colou = porMon.load(25, false);
  mockSerialSilence(false);
  CHECK_MSG(!colou, "um arquivo de outra VERSAO tem que ser recusado");

  porVersionEsperada = POR_VERSION;
  CHECK_MSG(porMon.load(25, false), "com a versao certa tem que carregar");
  porMon.unload();
}

TEST(retratos, un_numero_imposible_no_carga_nada_y_no_revienta) {
  if (!hayTarjeta()) { CHECK_MSG(false, "sem tarjeta"); return; }
  porEspeciesEsperadas = POR_ESPECIES;
  porRanurasEsperadas = POR_RANURAS;
  porVersionEsperada = POR_VERSION;
  CHECK(!porMon.load(0));
  CHECK(!porMon.load(DEX_COUNT + 500));
  CHECK(porMon.cara(POR_NORMAL) == nullptr);
  CHECK(porMon.cara(200) == nullptr);
  porMon.unload();
}

TEST(tarjeta, esta_donde_creemos) {
  CHECK_MSG(hayTarjeta(),
            "no encuentro tools/sdcard/mons: el resto de este fichero "
            "no probaria nada");
}

TEST(tarjeta, las_1025_se_cargan_con_el_codigo_del_firmware) {
  if (!hayTarjeta()) return;
  mockSerialSilence(true);
  int malas = 0;
  int16_t primera = 0;
  for (int16_t d = 1; d <= DEX_COUNT; d++) {
    Prestado p;
    if (!p.m.load(d)) {
      if (!primera) primera = d;
      malas++;
    }
  }
  mockSerialSilence(false);
  CHECK_MSG(malas == 0,
            std::string("no cargan ") + std::to_string(malas) +
            " de " + std::to_string((int)DEX_COUNT) +
            "; la primera es la " + std::to_string((int)primera));
}

TEST(tarjeta, cada_especie_carga_LA_SUYA_y_no_la_del_vecino) {
  // El fallo del uint8_t no era "no carga": era "carga otra". Un
  // contador de exitos no lo habria visto nunca. Lo que si lo ve es
  // que dos numeros distintos den el MISMO dibujo.
  //
  // Se comparan la paleta y las medidas de la accion de reposo, que es
  // lo que distingue a un bicho de otro sin tener que guardar 675 MB.
  if (!hayTarjeta()) return;
  mockSerialSilence(true);
  struct Huella { uint16_t pal0, palN, w, h, frames; };
  // 300 y 44 son el par exacto que se cruzaba; el resto es un barrido
  // por toda la dex para que no quede un tramo sin mirar.
  const int16_t MIRAR[] = { 44, 300, 1, 255, 256, 257, 511, 512, 513,
                            767, 768, 1024, DEX_COUNT };
  Huella h[sizeof(MIRAR) / sizeof(MIRAR[0])] = {};
  size_t n = 0;
  for (int16_t d : MIRAR) {
    Prestado p;
    CHECK_MSG(p.m.load(d), std::string("no carga la ") + std::to_string(d));
    const PmdAct &a = p.m.acts[PMD_IDLE];
    h[n++] = { p.m.pal[0], p.m.palCount, a.w, a.h, a.frames };
  }
  mockSerialSilence(false);
  int iguales = 0;
  for (size_t i = 0; i < n; i++)
    for (size_t j = i + 1; j < n; j++)
      if (!memcmp(&h[i], &h[j], sizeof(Huella))) iguales++;
  CHECK_MSG(iguales == 0,
            std::string("hay ") + std::to_string(iguales) +
            " pares de especies lejanas con el mismo sprite: el numero "
            "de dex esta dando la vuelta otra vez");
}

TEST(tarjeta, las_98_hembras_se_cargan_con_el_codigo_del_firmware) {
  // female.h dice con un bit quien trae dibujo propio y el firmware se
  // fia de ese bit. Aqui se comprueba con EL LECTOR DEL FIRMWARE, no con
  // un parseador de Python: que las 98 abran, normales y shiny.
  if (!hayTarjeta()) return;
  mockSerialSilence(true);
  int marcadas = 0, fallos = 0;
  std::string primerFallo;
  for (int16_t d = 1; d <= DEX_COUNT; d++) {
    if (!hasFemale(d)) continue;
    marcadas++;
    for (bool sh : { false, true }) {
      Prestado p;
      if (!p.m.load(d, sh, true)) {
        fallos++;
        if (primerFallo.empty())
          primerFallo = std::to_string(d) + (sh ? " shiny" : "");
      }
    }
  }
  mockSerialSilence(false);
  CHECK_MSG(marcadas == FEMALE_COUNT,
            std::string("female.h dice ") + std::to_string(FEMALE_COUNT) +
            " y el bitmap da " + std::to_string(marcadas));
  CHECK_MSG(fallos == 0, std::string("no cargan ") +
            std::to_string(fallos) + " hembras, la primera la " + primerFallo);
}

TEST(tarjeta, la_hembra_es_un_dibujo_DISTINTO_del_macho) {
  // Lo que de verdad importa. Si el fichero de la hembra fuese una copia
  // del macho, los 98 cargarian igual y el contador de exitos diria que
  // todo esta perfecto — y no se veria nada nuevo en la pantalla. Es el
  // mismo fallo que el de "carga otra": un exito no es una prueba.
  if (!hayTarjeta()) return;
  mockSerialSilence(true);
  int comparadas = 0, iguales = 0;
  std::string ejemplo;
  for (int16_t d = 1; d <= DEX_COUNT && comparadas < 20; d++) {
    if (!hasFemale(d)) continue;
    uint16_t pm = 0, pn = 0, w = 0, h = 0;
    {
      Prestado p;
      if (!p.m.load(d, false, false)) continue;
      pm = p.m.pal[0]; pn = p.m.palCount;
      w = p.m.acts[PMD_IDLE].w; h = p.m.acts[PMD_IDLE].h;
    }
    Prestado q;
    if (!q.m.load(d, false, true)) continue;
    comparadas++;
    if (q.m.pal[0] == pm && q.m.palCount == pn &&
        q.m.acts[PMD_IDLE].w == w && q.m.acts[PMD_IDLE].h == h) {
      iguales++;
      if (ejemplo.empty()) ejemplo = std::to_string(d);
    }
  }
  mockSerialSilence(false);
  CHECK_MSG(comparadas >= 10, "no se compararon suficientes hembras");
  // Alguna puede coincidir de verdad (hay hembras que solo cambian dos
  // pixeles), pero no TODAS: si coinciden todas, se empaqueto el macho.
  CHECK_MSG(iguales < comparadas,
            std::string("las ") + std::to_string(comparadas) +
            " hembras miradas dan el MISMO dibujo que el macho (p. ej. la " +
            ejemplo + "): se empaqueto el fichero equivocado");
}

TEST(tarjeta, sin_dibujo_de_hembra_cae_al_normal_y_no_se_queda_sin_bicho) {
  // Pedir la hembra de una especie que no la tiene no puede dejar el
  // combate sin dibujo: cae al normal. 924 especies estan en ese caso.
  if (!hayTarjeta()) return;
  mockSerialSilence(true);
  int mirados = 0;
  for (int16_t d = 1; d <= DEX_COUNT && mirados < 25; d++) {
    if (hasFemale(d)) continue;
    mirados++;
    Prestado p;
    CHECK_MSG(p.m.load(d, false, true),
              std::string("la ") + std::to_string(d) +
              " no tiene hembra y pedirla tiene que caer al normal");
  }
  mockSerialSilence(false);
  CHECK(mirados >= 20);
}

TEST(tarjeta, un_numero_imposible_no_carga_nada_y_no_revienta) {
  if (!hayTarjeta()) return;
  mockSerialSilence(true);
  for (int16_t d : { (int16_t)0, (int16_t)-1, (int16_t)-300,
                     (int16_t)(DEX_COUNT + 1), (int16_t)30000 }) {
    Prestado p;
    CHECK_MSG(!p.m.load(d),
              std::string("la ") + std::to_string(d) + " no deberia cargar");
    CHECK(!p.m.loaded);
  }
  mockSerialSilence(false);
}

TEST(tarjeta, todas_las_megas_y_formas_se_cargan_por_su_fila) {
  if (!hayTarjeta()) return;
  mockSerialSilence(true);
  int malas = 0;
  for (int16_t i = 0; i < (int16_t)MEGA_COUNT; i++) {
    Prestado p;
    if (!p.m.loadMega(i)) malas++;
  }
  CHECK_MSG(malas == 0, std::string("no cargan ") + std::to_string(malas) +
                        " megas de " + std::to_string((int)MEGA_COUNT));
  malas = 0;
  for (int16_t i = 0; i < (int16_t)FORM_COUNT; i++) {
    Prestado p;
    if (!p.m.loadForm(i)) malas++;
  }
  mockSerialSilence(false);
  CHECK_MSG(malas == 0, std::string("no cargan ") + std::to_string(malas) +
                        " formas de " + std::to_string((int)FORM_COUNT));
}

TEST(tarjeta, la_mega_de_un_shiny_sale_SHINY) {
  // A queixa: "o gengar shiny ao virar mega gengar, segue com a cor
  // normal". Mede-se o EFEITO: a paleta carregada para a mega shiny tem
  // de ser outra que a da mega normal, em todas as filas.
  if (!hayTarjeta()) return;
  mockSerialSilence(true);
  int iguais = 0, falham = 0;
  int16_t primeira = -1, gengar = -1;
  for (int16_t i = 0; i < (int16_t)MEGA_COUNT; i++) {
    if (gengar < 0 && MEGA_TBL[i].base == 94) gengar = i;
    Prestado n, s;
    if (!n.m.loadMega(i, false) || !s.m.loadMega(i, true)) { falham++; continue; }
    if (n.m.palCount == s.m.palCount &&
        !memcmp(n.m.pal, s.m.pal, n.m.palCount * sizeof(uint16_t))) {
      iguais++;
      if (primeira < 0) primeira = i;
    }
  }
  mockSerialSilence(false);
  CHECK_MSG(falham == 0, std::to_string(falham) + " megas nao carregam");
  CHECK_MSG(gengar >= 0, "premissa: a Mega Gengar esta na tabela");
  CHECK_MSG(iguais <= 2, std::to_string(iguais) + " megas shiny com a paleta "
            "da normal (a primeira, fila " + std::to_string((int)primeira) + ")");
  if (gengar >= 0) {
    Prestado n, s;
    mockSerialSilence(true);
    bool ok = n.m.loadMega(gengar, false) && s.m.loadMega(gengar, true);
    mockSerialSilence(false);
    CHECK(ok);
    CHECK_MSG(ok && (n.m.palCount != s.m.palCount ||
                     memcmp(n.m.pal, s.m.pal, n.m.palCount * 2)),
              "a Mega Gengar shiny saiu com a cor da normal");
  }
}

// Copia um arquivo do cartao (a pasta do PC) para outro nome, opcionalmente
// cortado em `corta` bytes a menos.
static bool copiaNoCartao(const char *de, const char *para, long corta = 0) {
  std::string a = std::string(RAIZ_SD) + de, b = std::string(RAIZ_SD) + para;
  FILE *fi = fopen(a.c_str(), "rb");
  if (!fi) return false;
  std::vector<uint8_t> v;
  int ch;
  while ((ch = fgetc(fi)) != EOF) v.push_back((uint8_t)ch);
  fclose(fi);
  if (corta > 0 && (long)v.size() > corta) v.resize(v.size() - corta);
  FILE *fo = fopen(b.c_str(), "wb");
  if (!fo) return false;
  fwrite(v.data(), 1, v.size(), fo);
  fclose(fo);
  return true;
}

TEST(tarjeta, a_animacao_do_anil_carrega_e_recusa_a_de_OUTRO_golpe) {
  // O arquivo diz o nome do golpe no cabecalho; o que nao casar e recusado
  // (indice por linha de tabela ja tocou a animacao do vizinho).
  if (!hayTarjeta()) return;
  uint16_t tackle = 0;
  for (uint16_t m = 0; m < MOVE_COUNT; m++)
    if (!strcmp(MOVE_TBL[m].name, "TACKLE")) tackle = m;
  CHECK(tackle);
  mockSerialSilence(true);
  SdAnim a;
  // Premissa: a de verdade carrega, dos dois lados.
  CHECK_MSG(a.load(tackle, "TACKLE", false) && a.nq > 0, "p_TACKLE nao carrega");
  CHECK_MSG(a.load(tackle, "TACKLE", true) && a.rival, "f_TACKLE nao carrega");
  // O conteudo do TACKLE com o nome de outro golpe: recusado.
  CHECK(copiaNoCartao("/anim/p_TACKLE.bin", "/anim/p_ZZTESTE.bin"));
  CHECK_MSG(!a.load(tackle, "ZZTESTE", false),
            "aceitou a animacao do TACKLE no lugar de outro golpe");
  CHECK(!a.blob);
  // Cortado no meio de uma copia: recusado, e nada fica meio carregado.
  // Corta-se o PROPRIO p_TACKLE (com o nome certo), senao a recusa viria
  // do nome e nao do corte; a copia de seguranca o devolve no fim.
  CHECK(copiaNoCartao("/anim/p_TACKLE.bin", "/anim/p_TACKLE.bak"));
  CHECK(copiaNoCartao("/anim/p_TACKLE.bak", "/anim/p_TACKLE.bin", 7));
  CHECK_MSG(!a.load(tackle, "TACKLE", false), "aceitou um arquivo cortado");
  CHECK(!a.blob);
  CHECK(copiaNoCartao("/anim/p_TACKLE.bak", "/anim/p_TACKLE.bin"));
  CHECK_MSG(a.load(tackle, "TACKLE", false), "o p_TACKLE nao voltou inteiro");
  remove((std::string(RAIZ_SD) + "/anim/p_ZZTESTE.bin").c_str());
  remove((std::string(RAIZ_SD) + "/anim/p_TACKLE.bak").c_str());
  a.unload();
  mockSerialSilence(false);
}

TEST(tarjeta, o_clima_carrega_e_recusa_o_de_OUTRO_clima) {
  // /anim/w_<n>.bin diz no cabecalho de que clima e (o WE_* de battle.h:
  // 2 chuva, 3 areia); um arquivo copiado por cima de outro e recusado.
  if (!hayTarjeta()) return;
  mockSerialSilence(true);
  SdClima c;
  // Premissa: os de verdade carregam, cada um com o que e seu.
  CHECK_MSG(c.load(3) && c.nq > 0 && c.figura(),
            "w_3.bin (a areia) nao carrega com a sua figura de tela cheia");
  CHECK_MSG(c.load(2) && c.ni > 0 && !c.figura(),
            "w_2.bin (a chuva) nao carrega com as suas particulas");
  // A areia no lugar da chuva: recusada.
  CHECK(copiaNoCartao("/anim/w_2.bin", "/anim/w_2.bak"));
  CHECK(copiaNoCartao("/anim/w_3.bin", "/anim/w_2.bin"));
  c.unload();      // senao load(2) responde "ja e esse" sem ler o arquivo
  CHECK_MSG(!c.load(2), "aceitou a areia no lugar da chuva");
  CHECK(!c.blob);
  // Cortado no meio de uma copia: recusado, e nada fica meio carregado.
  CHECK(copiaNoCartao("/anim/w_2.bak", "/anim/w_2.bin", 9));
  CHECK_MSG(!c.load(2), "aceitou um arquivo cortado");
  CHECK(!c.blob);
  CHECK(copiaNoCartao("/anim/w_2.bak", "/anim/w_2.bin"));
  CHECK_MSG(c.load(2), "o w_2.bin nao voltou inteiro");
  remove((std::string(RAIZ_SD) + "/anim/w_2.bak").c_str());
  c.unload();
  mockSerialSilence(false);
}

TEST(tarjeta, una_fila_que_no_existe_no_carga) {
  if (!hayTarjeta()) return;
  mockSerialSilence(true);
  Prestado a, b;
  CHECK(!a.m.loadMega((int16_t)MEGA_COUNT + 50));
  CHECK(!b.m.loadForm((int16_t)FORM_COUNT + 50));
  mockSerialSilence(false);
}

// La silueta del primer frame, reducida a 32x32. Sirve para comparar
// dos sprites sin que importe su tamano.
static void siluetaDe(const PmdMon &m, bool out[32 * 32]) {
  const PmdAct &a = m.acts[PMD_IDLE];
  for (int i = 0; i < 32 * 32; i++) out[i] = false;
  if (!a.data || !a.w || !a.h) return;
  for (int y = 0; y < 32; y++)
    for (int x = 0; x < 32; x++)
      out[y * 32 + x] = a.data[(y * a.h / 32) * a.w + (x * a.w / 32)] != 0xFF;
}

static int difiereSilueta(const bool a[32 * 32], const bool b[32 * 32]) {
  int n = 0;
  for (int i = 0; i < 32 * 32; i++) if (a[i] != b[i]) n++;
  return n * 100 / (32 * 32);
}

TEST(tarjeta, el_shiny_es_el_mismo_bicho_de_otro_color) {
  // Un shiny es el MISMO dibujo repintado. Comparar las MEDIDAS no
  // vale: 188 de las 1025 bailan, porque el Anil guarda el shiny como
  // una lamina aparte, a veces con otro numero de poses, y el recorte
  // al contenido cae donde cae. Los seis que mas bailaban los mire uno
  // a uno y son el bicho correcto.
  //
  // Asi que se compara la SILUETA. Y aqui va lo que de verdad hay que
  // saber, medido y no supuesto: comparando cada bicho con su shiny, la
  // diferencia va de 0% a 33% (mediana 0); comparandolo con un bicho
  // cualquiera, de 10% a 53%. **Las dos nubes se solapan**, asi que por
  // especie esto NO distingue un fichero equivocado de un shiny con
  // otra pose. Por eso hay dos comprobaciones distintas:
  //
  //   por especie  solo un desastre (>45%); hoy el peor va por 33
  //   en conjunto  la MEDIANA y el p95, que es lo que si separa: si
  //                alguien desplazara los shinys una fila, la mediana
  //                saltaria de 0 a ~29 en el acto
  if (!hayTarjeta()) return;
  mockSerialSilence(true);
  static bool sa[32 * 32], sb[32 * 32];
  std::vector<int> difs;
  int sinShiny = 0, iguales = 0, desastres = 0;
  int16_t peor = 0, palIgual = 0;
  for (int16_t d = 1; d <= DEX_COUNT; d++) {
    Prestado n, s;
    if (!n.m.load(d, false)) continue;
    if (!s.m.load(d, true)) { sinShiny++; continue; }
    siluetaDe(n.m, sa);
    siluetaDe(s.m, sb);
    int dif = difiereSilueta(sa, sb);
    difs.push_back(dif);
    if (dif > 45) { desastres++; if (!peor) peor = d; }
    if (n.m.palCount == s.m.palCount &&
        !memcmp(n.m.pal, s.m.pal, n.m.palCount * sizeof(uint16_t))) {
      iguales++;
      if (!palIgual) palIgual = d;
    }
  }
  mockSerialSilence(false);
  CHECK_MSG(sinShiny == 0, std::to_string(sinShiny) +
                           " especies sin sprite shiny");
  CHECK_MSG(desastres == 0, std::to_string(desastres) +
                            " shinys que no se parecen en nada a los suyos "
                            "(el primero, el " + std::to_string((int)peor) + ")");
  // Minior es el unico con la paleta clavada, y es CORRECTO: en forma
  // meteorito el shiny solo cambia el nucleo, y el nucleo no se ve.
  CHECK_MSG(iguales <= 1,
            std::to_string(iguales) + " shinys con la paleta identica a la "
            "suya (el primero, el " + std::to_string((int)palIgual) +
            "); solo Minior tiene derecho");

  // Y que la medida SIRVA para algo. Si `siluetaDe` se rompiera y
  // devolviera siempre lo mismo, todo lo de arriba daria 0% y el test
  // pasaria sin mirar nada — es el fallo que mas veces se ha repetido
  // en este proyecto. Asi que se comparan dos bichos que no tienen
  // nada que ver y se exige que la medida lo note.
  {
    Prestado x, y;
    mockSerialSilence(true);
    bool ok = x.m.load(143) && y.m.load(129);   // Snorlax vs Magikarp
    mockSerialSilence(false);
    CHECK(ok);
    if (ok) {
      siluetaDe(x.m, sa);
      siluetaDe(y.m, sb);
      CHECK_MSG(difiereSilueta(sa, sb) > 15,
                "la medida de silueta no distingue un Snorlax de un "
                "Magikarp: no esta midiendo nada");
    }
  }

  std::sort(difs.begin(), difs.end());
  CHECK(!difs.empty());
  int mediana = difs[difs.size() / 2];
  int p95 = difs[(size_t)(difs.size() * 95 / 100)];
  CHECK_MSG(mediana <= 2, std::string("la mediana de diferencia con su "
            "shiny es ") + std::to_string(mediana) + "%; en una tarjeta "
            "bien empaquetada es 0, y ~29 si estan desplazados");
  CHECK_MSG(p95 <= 15, std::string("el p95 es ") + std::to_string(p95) +
                       "%, y deberia rondar el 7");
}

TEST(tarjeta, el_sprite_de_espalda_esta_donde_lo_buscamos) {
  // El de espaldas vive en el hueco PMD_SLEEP: fue donde cupo. Si
  // alguien cambia el empaquetador y lo mueve, el combate pintaria al
  // bicho de frente contra si mismo y nadie se enteraria por un test
  // de ficheros.
  if (!hayTarjeta()) return;
  mockSerialSilence(true);
  int sinEspalda = 0;
  for (int16_t d = 1; d <= DEX_COUNT; d += 53) {
    Prestado p;
    if (!p.m.load(d)) continue;
    if (!p.m.has(PMD_SLEEP)) sinEspalda++;
  }
  mockSerialSilence(false);
  CHECK_MSG(sinEspalda == 0, std::string(std::to_string(sinEspalda)) +
                             " sin el hueco de espaldas (PMD_SLEEP)");
}

TEST(tarjeta, todas_traen_LAS_MISMAS_acciones) {
  // Las cinco que trae el acervo del Anil: reposo, los dos pasos, el
  // de espaldas (metido en el hueco de dormir) y el de dano.
  //
  // PMD_ATTACK **no existe** y es correcto que no exista: los battlers
  // de RPG Maker no tienen pose de atacar, y el combate la resuelve
  // moviendo el sprite, no cambiando de accion. Lo escribo aqui porque
  // mi primera version de este test la exigia y fallaba en las 1025 —
  // la respuesta era arreglar el test, no el acervo.
  //
  // Lo que si importa es que sean las MISMAS en todas: una especie a la
  // que le falte una se queda tiesa en esa pantalla, y es el tipo de
  // cosa que solo se ve jugando con ese bicho concreto.
  if (!hayTarjeta()) return;
  mockSerialSilence(true);
  const uint8_t PIDE[] = { PMD_IDLE, PMD_WALKL, PMD_WALKR, PMD_SLEEP,
                           PMD_HURT };
  int falta = 0;
  int16_t primera = 0;
  uint8_t cualFalta = 0;
  for (int16_t d = 1; d <= DEX_COUNT; d++) {
    Prestado p;
    if (!p.m.load(d)) continue;
    for (uint8_t a : PIDE) {
      if (!p.m.has(a)) {
        if (!primera) { primera = d; cualFalta = a; }
        falta++;
      }
    }
  }
  mockSerialSilence(false);
  CHECK_MSG(falta == 0, std::to_string(falta) +
                        " acciones que faltan; la primera es la accion " +
                        std::to_string((int)cualFalta) + " de la especie " +
                        std::to_string((int)primera));
}

TEST(tarjeta, ningun_frame_apunta_fuera_de_su_blob) {
  // El aparato indexa `data + frame * w * h` sin volver a comprobar.
  // Si el cabezal mintiera sobre las medidas, pintaria memoria de al
  // lado — y en la placa eso no es un test que falla, es una pantalla
  // con basura o un reinicio.
  if (!hayTarjeta()) return;
  mockSerialSilence(true);
  int malos = 0;
  for (int16_t d = 1; d <= DEX_COUNT; d += 7) {
    Prestado p;
    if (!p.m.load(d)) continue;
    for (uint8_t a = 0; a < PMD_NACTS; a++) {
      const PmdAct &ac = p.m.acts[a];
      if (!ac.frames) continue;
      if (!ac.data || !ac.w || !ac.h) { malos++; continue; }
      if (ac.base > ac.h) malos++;            // el suelo, fuera del dibujo
      if (ac.footR < ac.footL || ac.footR > ac.w) malos++;
      // TOCAR de verdad el ultimo pixel del ultimo frame: si el blob se
      // quedo corto, es aqui donde se nota (y bajo sanitizers, salta).
      const uint8_t *fin = ac.data +
          (uint32_t)(ac.frames - 1) * ac.w * ac.h + (ac.w * ac.h - 1);
      volatile uint8_t ultimo = *fin;
      (void)ultimo;
      if (ac.liftAt((uint8_t)(ac.frames - 1)) > ac.h) malos++;
    }
  }
  mockSerialSilence(false);
  CHECK_MSG(malos == 0, std::to_string(malos) +
                        " acciones con medidas que no cuadran con su blob");
}

TEST(tarjeta, la_animacion_propia_de_cada_golpe_se_carga) {
  // El bitmap FX_HAS_OWN de art.h dice que golpes tienen hoja propia en
  // /fx/fx.bin. tools/auditoria.py comprueba que el offset no apunte
  // fuera del fichero; esto comprueba otra cosa: que SdFx::load la
  // abra y monte sus fotogramas. Son las "animaciones de los golpes"
  // y hasta ahora no las tocaba ningun test.
  if (!hayTarjeta()) return;
  mockSerialSilence(true);
  int conPropia = 0, fallan = 0, vacias = 0;
  uint16_t primera = 0;
  for (uint16_t mv = 0; mv < MOVE_COUNT; mv++) {
    if (!fxHasOwn(mv)) continue;
    conPropia++;
    if (!fxMove.load(mv)) {
      if (!primera) primera = mv;
      fallan++;
      continue;
    }
    // cargada: tiene que traer al menos un fotograma con dibujo
    bool algo = false;
    for (uint8_t i = 0; i < fxMove.n && i < SDFX_MAX_FRAMES; i++)
      if (fxMove.has[i]) algo = true;
    if (!algo) { vacias++; if (!primera) primera = mv; }
  }
  fxMove.unload();
  mockSerialSilence(false);
  CHECK_MSG(conPropia > 100, std::to_string(conPropia) +
                             " golpes con animacion propia: esperaba "
                             "cientos, asi que no se esta mirando nada");
  CHECK_MSG(fallan == 0, std::to_string(fallan) + " de " +
                         std::to_string(conPropia) +
                         " animaciones no cargan (la primera, el golpe " +
                         std::to_string((int)primera) + " " +
                         (primera < MOVE_COUNT ? MOVE_TBL[primera].name : "?") +
                         ")");
  CHECK_MSG(vacias == 0, std::to_string(vacias) +
                         " animaciones que cargan y no traen ni un "
                         "fotograma con dibujo");
}

TEST(tarjeta, un_golpe_sin_animacion_propia_no_inventa_una) {
  if (!hayTarjeta()) return;
  mockSerialSilence(true);
  int colados = 0;
  for (uint16_t mv = 0; mv < MOVE_COUNT && colados == 0; mv++) {
    if (fxHasOwn(mv)) continue;
    if (fxMove.load(mv)) colados++;
  }
  fxMove.unload();
  mockSerialSilence(false);
  CHECK_MSG(colados == 0, "un golpe que NO tiene hoja propia se cargo "
                          "igual: estaria pintando la del vecino");
}

TEST(tarjeta, estan_todos_los_fondos_que_el_juego_nombra) {
  // No hay un BG_COUNT: los ids los reparten las tablas de bgs.h. Asi
  // que se pide EXACTAMENTE lo que esas tablas nombran — ni un id
  // inventado ni uno de menos. Un fondo que falte es una pantalla
  // negra detras del bicho, y solo en ese bioma.
  if (!hayTarjeta()) return;
  mockSerialSilence(true);
  bool pedido[256] = { false };
  for (uint8_t i = 0; i < GYM_TOTAL; i++)
    pedido[KANTO_BG[i]] = true;
  for (uint8_t b = 0; b < 6; b++) {
    const BgSet &s = BIOME_BGS[b];
    for (uint8_t i = 0; i < s.n; i++)
      pedido[s.ids[i]] = true;
  }
  for (const BgSet *s : { &NIGHT_BGS, &CAVE_BGS, &CITY_BGS })
    for (uint8_t i = 0; i < s->n; i++)
      pedido[s->ids[i]] = true;
  pedido[BG_LAB] = true;

  int cuantos = 0, fallan = 0, primero = -1;
  for (int n = 0; n < 256; n++) {
    if (!pedido[n]) continue;
    cuantos++;
    if (!bgImg.load((uint8_t)n) || !bgImg.w || !bgImg.h || !bgImg.pix) {
      fallan++;
      if (primero < 0) primero = n;
    }
  }
  bgImg.unload();
  mockSerialSilence(false);
  CHECK_MSG(cuantos > 20, std::to_string(cuantos) +
                          " fondos pedidos: esperaba unos 40, asi que "
                          "las tablas de bgs.h no se estan leyendo");
  CHECK_MSG(fallan == 0, std::to_string(fallan) + " de " +
                         std::to_string(cuantos) + " fondos no cargan "
                         "(el primero, el " + std::to_string(primero) + ")");
}

TEST(tarjeta, las_miniaturas_cubren_la_dex_entera) {
  if (!hayTarjeta()) return;
  mockSerialSilence(true);
  SdThumbs t;
  bool ok = t.load();
  mockSerialSilence(false);
  CHECK_MSG(ok, "no se carga thumbs.bin");
  if (!ok) return;
  CHECK_MSG(t.count >= DEX_COUNT,
            std::string("thumbs.bin trae ") + std::to_string((int)t.count) +
            " y la dex tiene " + std::to_string((int)DEX_COUNT));
  int sin = 0;
  for (int16_t d = 1; d <= DEX_COUNT; d++)
    if (!t.get(d)) sin++;
  CHECK_MSG(sin == 0, std::string(std::to_string(sin)) +
                      " especies sin miniatura en la galeria");
  t.unload();
}

// ---------------------------------------------------------------------
// La tarjeta PRESTADA al PC (modo USB-MSC).
//
// Con la pokebola cerrada, la unica via para actualizar la tarjeta es
// que el aparato se ensene al PC como un lector. Mientras el PC la
// tiene, el firmware no puede abrir NADA: dos escritores sobre el mismo
// FAT lo rompen, y ni leer es seguro, porque el PC puede estar
// reescribiendo la tabla justo debajo.
//
// Aqui no se prueba el USB -- eso es la placa. Se prueba la PUERTA:
// sdUsable(), que es lo unico que separa las dos cosas.
// ---------------------------------------------------------------------

// Devuelve la tarjeta pase lo que pase. Sin esto, un CHECK que falle a
// media prueba dejaba sdPrestada puesta y todos los tests de debajo
// fallaban por un motivo que no era el suyo.
struct EnManosDelPC {
  EnManosDelPC() { sdPrestada = true; }
  ~EnManosDelPC() { sdPrestada = false; }
};

TEST(usb, prestada_no_se_carga_ningun_sprite) {
  if (!hayTarjeta()) return;
  mockSerialSilence(true);
  // Primero, que SIN prestarla si carga. Sin esta mitad, lo de abajo
  // pasaria aunque el sprite no existiera.
  bool libre = false, presa = false, vuelve = false;
  { Prestado a; libre = a.m.load(25, false, false); }
  { EnManosDelPC pc; Prestado b; presa = b.m.load(25, false, false); }
  { Prestado c; vuelve = c.m.load(25, false, false); }
  mockSerialSilence(false);
  CHECK_MSG(libre, "PIKACHU no carga ni con la tarjeta libre");
  CHECK_MSG(!presa, "se cargo un sprite con la tarjeta en manos del PC");
  CHECK_MSG(vuelve, "la tarjeta no vuelve a servir despues de devolverla");
}

TEST(usb, prestada_no_se_releen_las_miniaturas_ni_los_fondos) {
  if (!hayTarjeta()) return;
  mockSerialSilence(true);
  SdThumbs t;
  bool libre = t.load();
  t.unload();
  bool thumbsPresa, bgPresa;
  {
    EnManosDelPC pc;
    SdThumbs u;
    thumbsPresa = u.load();
    SdBg b;
    bgPresa = b.load(KANTO_BG[0]);
    u.unload();
    b.unload();
  }
  mockSerialSilence(false);
  CHECK_MSG(libre, "thumbs.bin no carga ni con la tarjeta libre");
  CHECK_MSG(!thumbsPresa, "se leyo thumbs.bin con la tarjeta prestada");
  CHECK_MSG(!bgPresa, "se leyo un fondo con la tarjeta prestada");
}

TEST(usb, la_consola_tampoco_toca_la_tarjeta_prestada) {
  if (!hayTarjeta()) return;
  mockSerialSilence(true);
  EnManosDelPC pc;
  // LS y SDINFO son las dos que se colaban: abrian la tarjeta sin mirar
  // nada, y la guarda vieja no lo veia porque el sdReady que las
  // salvaba estaba en OTRA rama de la misma funcion. Siguen
  // respondiendo (la orden es suya) pero ya no abren nada.
  bool ls = sdSerialCommand(String("LS"));
  bool info = sdSerialCommand(String("SDINFO"));
  mockSerialSilence(false);
  CHECK_MSG(ls, "LS dejo de contestar");
  CHECK_MSG(info, "SDINFO dejo de contestar");
  CHECK_MSG(!sdUsable(), "la consola devolvio la tarjeta por su cuenta");
}

TEST(usb, la_puerta_son_dos_condiciones_no_una) {
  // sdUsable() = sdReady && !sdPrestada. Una version que solo mirase el
  // prestamo dejaria pasar a un aparato sin tarjeta ninguna, que es el
  // caso normal cuando el juego tira de los sprites de la flash.
  bool antes = sdReady;
  sdReady = false;
  sdPrestada = false;
  CHECK_MSG(!sdUsable(), "sdUsable() abierta sin tarjeta montada");
  sdReady = true;
  { EnManosDelPC pc; CHECK_MSG(!sdUsable(), "sdUsable() abierta estando prestada"); }
  CHECK_MSG(sdUsable(), "sdUsable() cerrada con tarjeta y sin prestamo");
  sdReady = antes;
}

// EVOLUCIONAR NO PUEDE HACERTE OCUPAR MENOS SITIO.
//
// Se reporto dos veces y de dos formas distintas: "pidgey evolui pro
// pidgeotto mas parece q ele desevoluiu" y, ya con la tabla nueva,
// "wiglytuff ta menor que o jiglypuff".
//
// Lo segundo es el caso que ensena por que esto se mide AQUI y no sobre
// la tabla: Wigglytuff sale 18 px MAS ALTO que su Jigglypuff y ocupa la
// MITAD de area, porque es estirado y el otro es una bola. Cualquier
// comprobacion que mire alturas —— o magnitudes —— lo da por bueno. Hay
// que mirar el SITIO, y para eso hacen falta los sprites.
//
// Los dos .bin se cargan con PmdMon, o sea con el parser del firmware, y
// `mag` sale de sdmon.cpp. La unica parte replicada son las dos lineas
// de escala de drawPmdActM; la autoridad es tools/audita_tamanos.py, que
// mide las tres vistas.
TEST(tarjeta, ninguna_evolucion_ocupa_menos_sitio_que_su_forma_anterior) {
  if (!hayTarjeta()) return;
  // el mismo recorte de alto que aplica el firmware en casa
  const float ALTO_MAX = 190.0f;
  auto sitio = [&](PmdMon &m, int16_t d) -> float {
    const PmdAct &a = m.acts[PMD_IDLE];
    if (!a.frames || a.mag <= 0.0f) return 0.0f;
    float quiere = (float)DEX_TBL[d].screenSz;
    int s = (int)(quiere / a.mag + 0.5f);
    if (s < 1) s = 1;
    if (s > 5) s = 5;
    while (s > 1 && a.h * s > 250) s--;
    float k = quiere / (a.mag * s);
    if (k < 0.45f) k = 0.45f;
    if (k > 1.9f) k = 1.9f;
    if (a.inkH() * s * k > ALTO_MAX) k = ALTO_MAX / (float)(a.inkH() * s);
    float px = s * k;
    return a.inkArea * px * px;
  };

  int encogen = 0;
  std::string primeros;
  for (int16_t d = 1; d <= DEX_COUNT; d++) {
    int16_t evo = DEX_TBL[d].evolvesTo;
    if (!evo || evo > DEX_COUNT || evo == d) continue;
    PmdMon a, b;
    if (!a.load(d) || !b.load(evo)) { a.unload(); b.unload(); continue; }
    float sa = sitio(a, d), sb = sitio(b, evo);
    a.unload();
    b.unload();
    if (sa <= 0.0f || sb <= 0.0f) continue;
    // 2% de margen: la escala entera obliga a redondear
    if (sb < sa * 0.98f) {
      encogen++;
      if (encogen <= 6)
        primeros += "\n    " + std::string(DEX_TBL[d].name) + " " +
                    std::to_string((int)(sa + 0.5f)) + " px2 -> " +
                    std::string(DEX_TBL[evo].name) + " " +
                    std::to_string((int)(sb + 0.5f)) + " px2";
    }
  }
  // GIMMIGHOUL -> GHOLDENGO no tiene arreglo y esta medido: el sprite de
  // Gholdengo es el mas estirado del plantel (2,03 de alto por ancho) y
  // ya toca el techo de alto de la pantalla, asi que ocupar mas solo
  // seria posible saliendose por arriba. Se deja pasar UNO, con nombre,
  // en vez de subir el margen hasta que no se queje nadie.
  CHECK_MSG(encogen <= 1,
            std::to_string(encogen) + " evoluciones ocupan menos sitio que "
            "su forma anterior:" + primeros);
}

// O pacote de som: /snd/golpes.txt tem de bater FILA A FILA com MOVE_TBL,
// ou cada golpe toca o som do vizinho. O de verdade bate; um com duas
// filas trocadas, ou com outro MOVE_COUNT, nao.
static const char *nomeDoGolpe(uint16_t i) { return MOVE_TBL[i].name; }

TEST(tarjeta, o_pacote_de_som_confere_a_tabela_de_golpes) {
  if (!hayTarjeta()) return;
  CHECK_MSG(sdFirmaGolpes(MOVE_COUNT, nomeDoGolpe),
            "o golpes.txt do cartao nao bate com moves.h: recorra pack_audio.py");

  // Uma copia com DUAS filas trocadas e outra com o MOVE_COUNT errado.
  FILE *src = fopen("../tools/sdcard/snd/golpes.txt", "rb");
  CHECK_MSG(src, "premissa: o golpes.txt existe");
  if (!src) return;
  std::string txt;
  char b[4096];
  size_t n;
  while ((n = fread(b, 1, sizeof(b), src)) > 0) txt.append(b, n);
  fclose(src);
  const std::string a = std::string(" ") + MOVE_TBL[317].name + " ",
                    z = std::string(" ") + MOVE_TBL[318].name + " ";
  size_t pa = txt.find("\n317" + a), pz = txt.find("\n318" + z);
  CHECK_MSG(pa != std::string::npos && pz != std::string::npos,
            "premissa: achei as filas 317 e 318");
  if (pa == std::string::npos || pz == std::string::npos) return;
  std::string trocado = txt;
  trocado.replace(pz + 4, z.size(), a);
  trocado.replace(pa + 4, a.size(), z);
  std::string contagem = txt;
  size_t pc = contagem.find("MOVE_COUNT ");
  contagem.replace(pc, 11 + std::to_string(MOVE_COUNT).size(),
                   "MOVE_COUNT " + std::to_string(MOVE_COUNT - 1));

  bool r[2];
  const std::string *casos[2] = { &trocado, &contagem };
  for (int k = 0; k < 2; k++) {
    std::filesystem::create_directories("build/sd_firma/snd");
    FILE *o = fopen("build/sd_firma/snd/golpes.txt", "wb");
    fwrite(casos[k]->data(), 1, casos[k]->size(), o);
    fclose(o);
    mockSdRoot("build/sd_firma");
    r[k] = sdFirmaGolpes(MOVE_COUNT, nomeDoGolpe);
    mockSdRoot(RAIZ_SD);
  }
  CHECK_MSG(!r[0], "duas filas trocadas passaram pela assinatura");
  CHECK_MSG(!r[1], "o MOVE_COUNT errado passou pela assinatura");
}

// As insignias com cor (Hoenn a Paldea) e as fitas das conquistas sairam
// do flash para o cartao. Todas tem de carregar; e um arquivo cuja entrada
// diz OUTRO indice (a tabela correu uma casa) tem de ser recusado.
TEST(tarjeta, insignias_e_fitas_vem_do_cartao_com_assinatura) {
  if (!hayTarjeta()) return;
  int ins = 0;
  for (uint8_t r = 2; r < 9; r++)
    for (uint8_t i = 0; i < 8; i++)
      if (insigniasSd.get(r, i)) ins++;
  CHECK_EQ(ins, 56);
  int fit = 0;
  for (uint8_t i = 0; i < 30; i++)
    if (fitasSd.get(0, i)) fit++;
  CHECK_EQ(fit, 30);
  const Art *a = insigniasSd.get(5, 3);
  CHECK_MSG(a && a->w > 8 && a->h > 8 && a->rleLen > 0, "a insignia veio vazia");
  CHECK_MSG(!insigniasSd.get(1, 0), "Johto nao esta no cartao (e mascara do flash)");

  // A copia adulterada: a primeira entrada de Kalos (regiao 5) diz que e
  // a numero 1.
  FILE *src = fopen("../tools/sdcard/por/insignias.bin", "rb");
  CHECK_MSG(src, "premissa: o insignias.bin existe");
  if (!src) return;
  std::vector<uint8_t> d;
  int c;
  while ((c = fgetc(src)) != EOF) d.push_back((uint8_t)c);
  fclose(src);
  uint32_t off;
  memcpy(&off, &d[8 + 4 * (8 * (5 - 2) + 0)], 4);
  CHECK_MSG(off && off + 1 < d.size() && d[off] == 5 && d[off + 1] == 0,
            "premissa: a entrada diz regiao 5, numero 0");
  d[off + 1] = 1;
  std::filesystem::create_directories("build/sd_tira/por");
  FILE *o = fopen("build/sd_tira/por/insignias.bin", "wb");
  fwrite(d.data(), 1, d.size(), o);
  fclose(o);
  SdTira teste("/por/insignias.bin", "TPIN");
  mockSdRoot("build/sd_tira");
  bool carregou = teste.load(5);
  mockSdRoot(RAIZ_SD);
  CHECK_MSG(!carregou, "a entrada com o indice errado foi aceita");
  teste.unload();
}
