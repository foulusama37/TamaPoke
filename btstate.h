#pragma once
#include <stdint.h>

// Los estados y los stages de combate, en un sitio al que pueden llegar los
// dos que los necesitan: battle.h (que los usa) e items.h (que los nombra en
// su tabla: un Antidoto tiene que decir ST_POISON). items.h es GENERADO y
// battle.h lo incluye, asi que no puede ir al reves sin montar un ciclo.

// Solo puede haber UNO a la vez, como en los juegos: si ya estas quemado no
// te pueden envenenar encima. La confusion va aparte porque en los juegos si
// se acumula con el estado principal.
enum : uint8_t { ST_NONE = 0, ST_BURN, ST_POISON, ST_BADPOISON,
                 ST_PARALYSIS, ST_SLEEP, ST_FREEZE };

// Los dos "deportes" que debilitan un tipo entero mientras duran.
enum : uint8_t { SP_WATER = 1 << 0, SP_MUD = 1 << 1 };

// El SUELO del combate. Vive aqui, con el resto del estado, porque lo
// miran tanto battle.h como abilities.h (los cuatro "Surge" lo ponen al
// entrar) y abilities.h no puede incluir battle.h.
enum : uint8_t { TR_NONE = 0, TR_ELEC, TR_GRASS, TR_MISTY, TR_PSY };

// Donde se mete el que esta cargando Excavar, Vuelo o Buceo. Mientras
// este ahi, casi nada le alcanza; lo que si le pilla, le pega el doble.
// HID_GONE es Golpe Umbrio / Golpe Fantasma: no esta en NINGUN sitio, y
// no hay golpe que le llegue.
enum : uint8_t { HID_NONE = 0, HID_UNDER, HID_SKY, HID_WATER, HID_GONE };

// El ENTORNO del combate (el `environment` del Anil), para Poder Secreto.
// EN_MISTY y EN_PSY no son entornos sino los dos terrenos que el golpe
// mira antes; van en el mismo enum para que el switch sea uno.
enum : uint8_t { EN_NONE = 0, EN_GRASS, EN_WATER, EN_SAND, EN_CAVE,
                 EN_SNOW, EN_VOLCANO, EN_MISTY, EN_PSY };

// indices de stage[]. La evasion se anadio cuando entraron Minimize, Double
// Team y Sweet Scent: antes era un stat que nadie tocaba.
enum : uint8_t { SG_ATK = 0, SG_DEF, SG_SPA, SG_SPD, SG_SPE, SG_ACC, SG_EVA,
                 SG_COUNT };

// Indice de IV/EV. Son SEIS y a diferencia de SG_* incluyen los PS, porque un
// EV de PS existe aunque ningun golpe pueda subir los PS en combate. Vive
// aqui y no en party.h porque la tabla de objetos lo nombra (una Proteina es
// IT_VITAMIN EV_ATK) y items.h se genera antes que todo lo demas.
enum : uint8_t { EV_HP = 0, EV_ATK, EV_DEF, EV_SPA, EV_SPD, EV_SPE, EV_N };
