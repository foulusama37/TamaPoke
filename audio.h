#pragma once
#include <stdint.h>

// Efectos de sonido del juego (cola, no bloqueante). El orden coincide con la
// tabla SFX de audio.cpp.
enum Sfx : uint8_t {
  SFX_TAP = 0,  // tocar / boton
  SFX_EAT,      // comer
  SFX_PLAY,     // punto del minijuego / golpe
  SFX_HEART,    // le gusta / mimo
  SFX_HATCH,    // eclosion
  SFX_EVOLVE,   // evolucion
  SFX_MEDAL,    // medalla / hito
  SFX_DENY,     // accion no permitida
  SFX_BYE,      // despedida
  SFX_LEVEL,    // sube de nivel
  // El aviso de que hay alguien en la hierba. Son DOS y no uno: saber
  // si te espera un bicho o un entrenador cambia si vas o no, y el
  // dibujo ya lo distingue (hierba meciendose contra bocadillo de "!").
  // Distinguirlo tambien de oido es lo que deja enterarte sin mirar.
  SFX_WILD,     // sale un salvaje
  SFX_TRAINER,  // te ve un entrenador
  // El GOLPE, segun lo que haya hecho. Tres y no uno: en los juegos el
  // "no le hace nada", el normal y el "es muy eficaz" suenan distinto, y
  // es la forma mas rapida de enterarte de si acertaste el tipo -- antes
  // de leer el mensaje. Hasta ahora el combate tocaba el pitido del
  // MENU, que es lo que sonaba al pulsar un boton: no decia nada.
  SFX_HIT_WEAK,   // poco eficaz (x0,25 y x0,5)
  SFX_HIT,        // normal
  SFX_HIT_SUPER,  // muy eficaz (x2 y x4)
  // LA POKEBOLA, con los sonidos del Anil (PokeballThrowCapture): hasta
  // ahora volaba, se abria y se cerraba con el pitido del menu.
  SFX_BALL_THROW,   // sale de la mano
  SFX_BALL_HIT,     // llega y se abre
  SFX_BALL_DROP,    // cae al suelo, ya cerrada
  SFX_BALL_SHAKE,   // cada vaiven del forcejeo
  SFX_BALL_CLICK,   // se cierra del todo: atrapado
  SFX_BALL_OPEN,    // se abre y se escapa
  SFX_COUNT
};

// ---------------------------------------------------------------- muestras
//
// Lo de arriba son pitidos de onda cuadrada sintetizados al vuelo. Esto es
// audio de VERDAD: musica de combate, fanfarrias y los gritos de los 151,
// sacados del Anil y convertidos a PCM mono de 16 kHz (tools/pack_audio.py).
// Viven en /snd de la SD y se cargan a la PSRAM; sin tarjeta no suena nada
// de esto y los pitidos siguen funcionando igual, que es lo que permite que
// el cacharro arranque sin tarjeta.
enum : uint8_t {
  MUS_WILD = 0, MUS_TRAINER, MUS_GYM, MUS_ELITE, MUS_CHAMP, MUS_TITLE,
  // La pantalla de casa tiene VARIAS y se van turnando, por lo mismo que
  // los paisajes: es la que esta siempre encendida. La ultima es la de
  // noche. Quien elige es MUS_HOME + n, con n < MUS_HOME_COUNT.
  MUS_HOME, MUS_HOME_COUNT = 5, MUS_HOME_NIGHT = MUS_HOME + MUS_HOME_COUNT,
  MUS_EVOLVE = MUS_HOME_NIGHT + 1,   // la escena de evolucion
  MUS_COUNT = MUS_EVOLVE + 1
};
enum : uint8_t {
  JIN_WIN_WILD = 0, JIN_WIN_TRAINER, JIN_WIN_BIG, JIN_CAUGHT, JIN_EVOLVE,
  JIN_BADGE, JIN_HATCH, JIN_HEAL,
  // La MEGA. Sale de "PRSFX- Mega Evolution2.wav" del Anil y no del 1:
  // el 1 es byte a byte el mismo fichero que "Pulse Evolution1" -- un
  // zumbido plano de 1,9 s sin ataque ninguno (RMS 0,06 de punta a
  // punta). El 2 es el estallido: 0,61 en el primer decimo y 1,37 s de
  // cola, que es lo que dura la escena.
  JIN_MEGA,
  JIN_COUNT
};

void audioBegin();          // init ES8311 + I2S + amplificador + tarea de audio
void sfxPlay(uint8_t id);   // encola un efecto (no bloquea el loop)
// Musica de fondo, en bucle. Pedir la que ya suena NO la reinicia, asi que
// se puede llamar desde el bucle de dibujo sin pensarlo.
void musicPlay(uint8_t id);
void musicStop();
// Una vez, por encima de la musica. false si no sono (sin tarjeta no hay
// fanfarrias), para poder caer al pitido sintetizado.
bool jinglePlay(uint8_t id);
bool cryPlay(uint16_t dex);    // el grito de una especie; false = no sono
// A tabela de golpes contra a qual conferir /snd/golpes.txt: quantos sao
// e o nome de cada um. O audio.cpp nao inclui moves.h (seria uma segunda
// copia da tabela no flash), entao o sketch entrega isto no setup().
void audioMoveTable(uint16_t count, const char *(*nome)(uint16_t));
// O pacote de som bate com a tabela? -1 ainda nao se sabe (sem cartao ou
// nao se tentou), 0 nao bate, 1 bate. Para os testes e o comando serie.
int8_t audioMoveFirma();

// El sonido PROPIO de un golpe (/snd/gNNN.raw). Devuelve false si ese
// golpe no tiene el suyo en la tarjeta, para poder caer al pitido. El
// nombre del fichero lo decide el Anil: sale de los `timing` de su
// animacion (ver tools/pack_audio.py, haz_golpes).
bool movePlay(uint16_t mv);
// Suena algo de una vez (fanfarria o grito)? La pantalla de casa lo mira
// para no arrancar su musica ENCIMA de la fanfarria de victoria, que es
// justo el momento en que uno quiere oirla.
bool audioOneShotBusy();
// Volumen en cuatro escalones (0 = mudo). Va por hardware, en el registro
// de volumen del DAC: bajarlo escalando las muestras seria tirar bits.
#define AUDIO_VOL_MAX 3
void audioSetVolume(uint8_t lvl);
uint8_t audioVolume();
void audioSetEnabled(bool on);   // atajo: 0 o el ultimo volumen que tenias
bool audioEnabled();
void audioSetSleeping(bool sleeping);  // dormida: amplificador apagado
