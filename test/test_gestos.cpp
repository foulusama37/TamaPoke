// Los GESTOS del acelerometro.
//
// El driver (imu.cpp) habla por I2C y sin la placa no hay nada que
// afirmar, igual que audio.cpp y rtcbat.cpp. Pero lo que DECIDE — esto es
// un meneo o un bache?, van dos tapas o cinco?, esta boca abajo o solo de
// canto? — son cuentas sobre numeros, y eso si se prueba: se le dan
// muestras a mano y se mira que conteste.
//
// Por eso gestos.cpp no toca hardware. Si el dia que llegue la placa un
// gesto no sale, el fallo estara en el driver o en los umbrales, no en la
// logica — y eso es la mitad del trabajo de buscarlo.
#include "framework.h"
#include "shim/Arduino.h"
#include "../gestos.h"

// Una tanda de muestras quietas, para separar gestos en los tests.
static void quieto(LectorGestos &g, uint32_t &ms, int cuantas = 10) {
  for (int i = 0; i < cuantas; i++) {
    g.muestra(0.0f, 0.0f, 1.0f, ms);
    ms += 10;
  }
}

TEST(gestos, quieto_no_es_ningun_gesto) {
  LectorGestos g;
  uint32_t ms = 1000;
  quieto(g, ms, 200);          // dos segundos en la mesa
  Gestos r = g.leer();
  CHECK_MSG(!r.meneo, "una consola quieta no se esta meneando");
  CHECK_MSG(!r.susto, "ni asustando a nadie");
  CHECK_MSG(!r.caricia && !r.dosTapas, "ni acariciando");
  CHECK_MSG(!r.bocabajo, "boca arriba no es boca abajo");
  CHECK_MSG(r.pasos == 0, "y nadie ha andado");
}

TEST(gestos, una_mesa_que_vibra_no_cuenta_como_meneo) {
  // El fallo facil: mirar "se movio" en vez de "se movio FUERTE". Una
  // consola en una mesa con un ventilador da un modulo distinto de 1g
  // pero pequeño, y eso no puede dar puntos de minijuego gratis.
  LectorGestos g;
  uint32_t ms = 1000;
  for (int i = 0; i < 300; i++) {
    float t = (i % 2) ? 1.08f : 0.94f;   // tembleque de +-8%
    g.muestra(0.0f, 0.0f, t, ms);
    ms += 10;
  }
  Gestos r = g.leer();
  CHECK_MSG(!r.meneo, "un tembleque no es un meneo");
  CHECK_MSG(r.pasos == 0, "ni un paso");
}

TEST(gestos, un_meneo_de_verdad_se_cuenta_una_sola_vez) {
  // A 100 muestras por segundo, un meneo que dura 200 ms son VEINTE
  // muestras por encima del umbral. Sin tiempo muerto se contaria veinte
  // veces y el minijuego se ganaria con un solo sacudon.
  LectorGestos g;
  uint32_t ms = 1000;
  quieto(g, ms);
  for (int i = 0; i < 20; i++) {       // 200 ms de meneo continuo
    g.muestra(0.0f, 0.0f, 2.0f, ms);
    ms += 10;
  }
  quieto(g, ms);
  Gestos r = g.leer();
  CHECK_MSG(r.meneo, "eso es un meneo");
  // dos meneos separados SI son dos
  LectorGestos g2;
  uint32_t t2 = 1000;
  int cuenta = 0;
  for (int k = 0; k < 2; k++) {
    g2.muestra(0.0f, 0.0f, 2.0f, t2);
    if (g2.leer().meneo) cuenta++;
    t2 += GESTO_MENEO_MS + 50;
  }
  CHECK_MSG(cuenta == 2, "dos meneos separados son dos, no uno");
}

TEST(gestos, la_fuerza_del_meneo_distingue_el_roce_del_sacudon) {
  // Lo pide la captura: menear mas fuerte tiene que valer mas, o menear
  // seria un boton y no un gesto.
  LectorGestos flojo, fuerte;
  flojo.muestra(0.0f, 0.0f, GESTO_MENEO_G + 0.05f, 1000);
  fuerte.muestra(0.0f, 0.0f, GESTO_SUSTO_G - 0.05f, 1000);
  uint8_t ff = flojo.leer().fuerza, fz = fuerte.leer().fuerza;
  CHECK_MSG(ff < fz, "un sacudon tiene que puntuar mas que un roce: " +
            std::to_string((int)ff) + " contra " + std::to_string((int)fz));
  CHECK_MSG(ff < 60, "el roce justo en el umbral puntua poco");
  CHECK_MSG(fz > 190, "y el sacudon, mucho");
}

TEST(gestos, zarandear_demasiado_fuerte_asusta) {
  // El preciо de menear. Sin esto, menear seria gratis y el jugador
  // sacudiria la consola todo el dia sin consecuencia.
  LectorGestos g;
  uint32_t ms = 1000;
  g.muestra(0.0f, 0.0f, 2.0f, ms);          // meneo normal
  Gestos r1 = g.leer();
  CHECK_MSG(r1.meneo && !r1.susto, "un meneo normal no asusta");
  ms += GESTO_MENEO_MS + 50;
  g.muestra(0.0f, 0.0f, GESTO_SUSTO_G + 0.5f, ms);
  Gestos r2 = g.leer();
  CHECK_MSG(r2.meneo && r2.susto, "un zarandeo brusco si");
}

TEST(gestos, dos_tapas_despiertan_y_muchas_acarician) {
  // Las dos cosas que pidio el usuario, y lo importante es que NO se
  // pisen: una caricia empieza por dos tapas.
  LectorGestos g;
  g.tapa(2, 1000);                   // dos justas
  Gestos r = g.leer();
  CHECK_MSG(r.dosTapas, "dos tapas despiertan");
  CHECK_MSG(!r.caricia, "dos no son una caricia");

  LectorGestos g2;
  uint32_t ms = 1000;
  for (int i = 0; i < 5; i++) {      // cinco seguidas
    g2.tapa(1, ms);
    ms += 200;
  }
  Gestos r2 = g2.leer();
  CHECK_MSG(r2.caricia, "varias seguidas son una caricia");
  CHECK_MSG(!r2.dosTapas,
            "y acariciar NO puede contar tambien como despertar: seria "
            "despertarle cada vez que le haces un mimo");
}

TEST(gestos, las_tapas_sueltas_no_se_acumulan_para_siempre) {
  // Tres tapas en tres minutos no son una caricia. Sin la ventana, el
  // contador subiria durante todo el dia y cualquier golpecito acabaria
  // dando mimos gratis.
  LectorGestos g;
  uint32_t ms = 1000;
  for (int i = 0; i < 4; i++) {
    g.tapa(1, ms);
    ms += GESTO_CARICIA_MS + 500;    // muy espaciadas
    CHECK_MSG(!g.leer().caricia,
              "una tapa cada dos segundos no es acariciar");
  }
  CHECK_MSG(g.tapasSeguidas() <= 1, "el contador tiene que reiniciarse");
}

TEST(gestos, un_lanzamiento_es_empujon_Y_frenazo) {
  // Lo pide la carcasa de Pokebola: tirarla es el gesto natural. Y lo
  // que lo distingue de menear NO es el pico (los dos tienen pico) sino
  // lo que pasa despues: un meneo va y vuelve, un tiro acelera y PARA.
  LectorGestos g;
  uint32_t ms = 1000;
  quieto(g, ms);
  g.muestra(0.0f, 0.0f, 2.6f, ms);     // el empujon
  ms += 80;
  for (int i = 0; i < 10; i++) {       // y se queda quieta: frenazo
    g.muestra(0.0f, 0.0f, 1.0f, ms);
    ms += 20;
  }
  Gestos r = g.leer();
  CHECK_MSG(r.tiro, "empujon y frenazo es un lanzamiento");
}

TEST(gestos, menear_NO_es_lanzar) {
  // El test que de verdad importa. Si menear contara como tiro, el
  // jugador tiraria la bola sin querer cada vez que juega al minijuego.
  LectorGestos g;
  uint32_t ms = 1000;
  quieto(g, ms);
  int tiros = 0;
  for (int i = 0; i < 15; i++) {
    // menear: va y VUELVE, sin quedarse quieta en ningun momento
    g.muestra(0.0f, 0.0f, 2.6f, ms);
    ms += 60;
    g.muestra(0.0f, 0.0f, -1.6f, ms);  // el rebote del vaiven
    ms += 60;
    if (g.leer().tiro) tiros++;
  }
  CHECK_MSG(tiros == 0, "menear se conto como lanzamiento " +
            std::to_string(tiros) + " veces: el minijuego tiraria la bola "
            "sin querer");
}

TEST(gestos, un_empujon_sin_frenazo_a_tiempo_no_es_tiro) {
  // Si el frenazo llega tarde, fue un meneo largo y no un tiro.
  LectorGestos g;
  uint32_t ms = 1000;
  quieto(g, ms);
  g.muestra(0.0f, 0.0f, 2.6f, ms);
  ms += GESTO_TIRO_MS + 200;           // se le pasa el tiempo
  g.muestra(0.0f, 0.0f, 1.0f, ms);
  CHECK_MSG(!g.leer().tiro, "el frenazo tiene que llegar a tiempo");
}

TEST(gestos, boca_abajo_hay_que_aguantarlo) {
  LectorGestos g;
  uint32_t ms = 1000;
  // darle la vuelta un instante (para mirar algo) no es acostarlo
  for (int i = 0; i < 50; i++) {
    g.muestra(0.0f, 0.0f, -1.0f, ms);
    ms += 10;
  }
  CHECK_MSG(!g.leer().bocabajo,
            "medio segundo boca abajo no es ponerlo a dormir");
  // aguantarlo si
  for (int i = 0; i < 250; i++) {
    g.muestra(0.0f, 0.0f, -1.0f, ms);
    ms += 10;
  }
  CHECK_MSG(g.leer().bocabajo, "dos segundos y medio, si");
}

TEST(gestos, boca_abajo_no_avisa_dos_veces_seguidas) {
  // Si avisara en cada muestra, el bicho se dormiria cien veces por
  // segundo mientras la consola esta en la mesa del reves.
  LectorGestos g;
  uint32_t ms = 1000;
  int avisos = 0;
  for (int i = 0; i < 600; i++) {
    g.muestra(0.0f, 0.0f, -1.0f, ms);
    ms += 10;
    if (g.leer().bocabajo) avisos++;
  }
  CHECK_MSG(avisos == 1, "tiene que avisar UNA vez, no " +
            std::to_string(avisos));
}

TEST(gestos, de_canto_en_la_mesa_no_es_boca_abajo) {
  LectorGestos g;
  uint32_t ms = 1000;
  for (int i = 0; i < 600; i++) {
    g.muestra(1.0f, 0.0f, 0.0f, ms);   // de lado: la gravedad va en X
    ms += 10;
  }
  CHECK_MSG(!g.leer().bocabajo, "de canto no esta boca abajo");
}

TEST(gestos, andar_cuenta_pasos_y_menear_no) {
  // Andar es un vaiven ritmico y suave; menear es un golpe seco. Si los
  // pasos contaran los meneos, sacudir la consola daria el ejercicio de
  // una caminata.
  LectorGestos g;
  uint32_t ms = 1000;
  for (int p = 0; p < 20; p++) {       // veinte zancadas de 600 ms
    g.muestra(0.0f, 0.0f, 1.35f, ms);  // el pico del paso
    ms += 100;
    g.muestra(0.0f, 0.0f, 1.0f, ms);   // y baja
    ms += 500;
  }
  Gestos r = g.leer();
  CHECK_MSG(r.pasos >= 15, "veinte zancadas tienen que dar pasos, dio " +
            std::to_string((int)r.pasos));
  CHECK_MSG(!r.meneo, "andar no es menear");

  LectorGestos g2;
  uint32_t t2 = 1000;
  for (int i = 0; i < 20; i++) {       // sacudones
    g2.muestra(0.0f, 0.0f, 2.5f, t2);
    t2 += GESTO_MENEO_MS + 20;
    g2.muestra(0.0f, 0.0f, 1.0f, t2);
    t2 += 30;
  }
  Gestos r2 = g2.leer();
  CHECK_MSG(r2.meneo, "eso son meneos");
  CHECK_MSG(r2.pasos == 0, "y menear NO puede contar como andar: seria "
            "hacer ejercicio sacudiendo la consola en el sofa");
}

TEST(gestos, los_pasos_se_consumen_al_leerlos) {
  // Si no se consumieran, el juego cobraria los mismos pasos en cada
  // vuelta del bucle principal.
  LectorGestos g;
  uint32_t ms = 1000;
  for (int p = 0; p < 6; p++) {
    g.muestra(0.0f, 0.0f, 1.35f, ms);
    ms += 100;
    g.muestra(0.0f, 0.0f, 1.0f, ms);
    ms += 500;
  }
  CHECK(g.leer().pasos > 0);
  CHECK_MSG(g.leer().pasos == 0, "leidos una vez, no se vuelven a cobrar");
}

TEST(gestos, un_reset_lo_deja_todo_a_cero) {
  LectorGestos g;
  uint32_t ms = 1000;
  g.muestra(0.0f, 0.0f, 3.0f, ms);
  g.tapa(1, ms);
  g.reset();
  Gestos r = g.leer();
  CHECK(!r.meneo && !r.susto && !r.caricia && !r.dosTapas);
  CHECK(r.pasos == 0 && r.fuerza == 0);
  CHECK(g.tapasSeguidas() == 0);
}
