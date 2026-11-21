#pragma once
#include <Arduino.h>

// Idiomas soportados. La fuente 5x7 de Arduino_GFX es una tabla CP437 completa
// (256 glifos), asi que SI hay acentos, dieresis y enes: van como byte suelto en
// octal (\240 a, \244 n~, \202 e, \204 a"...), nunca en UTF-8, porque la UI centra
// con strlen(t)*6 y un caracter de dos bytes descuadraria las etiquetas.
// No estan en CP437 las mayusculas acentuadas salvo E (\220). Tampoco a~/o~,
// que el portugues necesita en casi cada frase ("nao", "acao", "evolucao"):
// para esas dos hay codigos propios en los huecos de dibujo de cajas, y
// printT() las dibuja poniendo la '~' de la fuente encima de la letra.
// Medido: la '~' ocupa las filas 0..2 de la celda 5x7 y la 'a'/'o' las
// 2..6, asi que el til cabe dentro y el avance sigue siendo 6 px.
// Las MAYUSCULAS no caben en la celda (la 'A' usa las siete filas), asi que
// su til va TRES FILAS MAS ARRIBA, fuera de ella. Vale en un boton o un
// titulo -- "NAO", "EVOLUCAO", "CAMPEAO", que es donde salen -- y no vale en
// medio de un parrafo, donde se metria en la linea de encima. texto_pt.py
// AVISA de cada una para poder mirar la captura.
#define CH_ATIL  '\260'   // a~
#define CH_OTIL  '\261'   // o~
#define CH_ATILM '\262'   // A~
#define CH_OTILM '\263'   // O~
enum Lang : uint8_t { LANG_ES = 0, LANG_EN, LANG_FR, LANG_DE, LANG_IT, LANG_PT,
                     LANG_JA, LANG_KO, LANG_COUNT };
// idiomas que necesitan una fuente U8g2 y texto UTF-8 en vez de la tabla CP437
#define LANG_IS_CJK(l) ((l) == LANG_JA || (l) == LANG_KO)
#define LANG_DEFAULT LANG_EN  // idioma por defecto: ingles

extern Lang gLang;  // idioma activo (definido en i18n.cpp)

// IDs de cadena. El orden debe coincidir con la tabla STRINGS de i18n.cpp.
enum StrId : uint16_t {
  // estado del bicho (statusMsg)
  S_EVOLVING, S_EATING, S_LIKES, S_HUNGRY, S_NEEDS_BATH,
  S_EXHAUSTED, S_SAD, S_CHUBBY, S_IS_SHINY, S_HAPPY,
  // ceremonias de despedida
  S_FAREWELL, S_RUNAWAY, S_GOODBYE,
  // huevo
  S_EGG_HDR, S_EGG_LEGEND, S_EGG_RARE, S_EGG_TOUCH, S_EGG_MOVES, S_EGG_ALMOST,
  // formatos compartidos
  // "POKEDEX %u/%u" (vistos, total). El total era 151 escrito dentro del
  // literal; al entrar Johto habia que cambiarlo en las ocho traducciones y
  // una se habria quedado atras sin que nadie lo viera.
  S_POKEDEX_FMT,
  S_NAME_FMT,      // "%s%s Nv.%u"
  // dialogo soltar
  S_RELEASE_FMT, S_YES, S_NO,
  // gimnasio: saco de tipos
  S_GYM_PICK, S_NEW_RECORD, S_RECORD_FMT, S_GYM_HINT,
  S_SCORE_FMT, S_GYM_SUPER, S_GYM_OK, S_GYM_BAD,
  S_GYM_BUDGET_FMT, S_GYM_GAIN_FMT, S_GYM_FULL, S_GYM_TIRED,
  // reloj / ajustes
  S_SET_TIME, S_HOUR, S_MIN, S_CLOCK_CANCEL, S_LANG_LABEL,
  // celebracion
  S_MEDAL_BANNER, S_GREAT, S_STREAK_DAYS_FMT,
  // ficha: perfil
  S_STREAK_FMT, S_VIN, S_BERRY_UNK, S_BERRY_RED, S_BERRY_BLUE, S_BERRY_GREEN,
  S_INFO_FMT, S_RENAME_HINT, S_DISOBEY_LBL,
  // ficha: combate
  // S_STAT_SPE es la VELOCIDAD (no confundir con S_STAT_SPD, que es la
  // defensa especial): son los mismos cinco nombres que usa Pet::evName.
  S_BATTLE, S_STAT_ATK, S_STAT_DEF, S_STAT_SPA, S_STAT_SPD, S_STAT_SPE,
  S_STAT_WGT, S_TRAIN_BTN,
  // ficha: medallas
  S_MEDALS_FMT, S_BACK,
  // teclado y galeria
  S_NAME, S_DETAIL_BACK,
  // barras
  S_BAR_FOOD, S_BAR_JOY, S_BAR_ENE, S_BAR_HYG,
  // marcador en vivo del gimnasio
  S_REC_FMT,
  // ficha: pagina de progreso
  S_PROGRESS, S_LVL_FMT, S_NEXT_LVL_FMT, S_EVO_LABEL, S_FINAL_FORM,
  S_EVO_READY, S_EVO_BLOCKED, S_EVO_IN_FMT, S_MISTAKES_FMT,
  // interruptor de sonido (ajustes)
  S_SND_ON, S_SND_OFF,
  // gimnasios de Kanto
  S_GYMS, S_GYM_WINS_FMT, S_GYM_GO, S_GYM_NOT_YET, S_GYM_CHAMP,
  // ficha de entrenador: el titulo y las tres cuentas que ensena
  S_CARD, S_CARD_WINS, S_CARD_CAUGHT, S_CARD_BADGES,
  // intro de primera partida y reseteo de fabrica
  S_INTRO_SUB, S_INTRO_BASED, S_INTRO_TAP,
  S_OAK_1, S_OAK_2, S_OAK_3, S_OAK_4,
  S_ASK_GENDER, S_BOY, S_GIRL,
  S_RESET, S_RESET_ASK1, S_RESET_ASK2,
  S_EVO_TAP,        // texto del boton de evolucion
  S_FAREWELL_BTN,   // texto del boton de despedida (lleva el nombre: "%s ...")
  S_RUNAWAY_BTN,    // texto del boton de escapada por abandono (final triste)
  // dialogos de decision (evolucionar/mantener, despedirse/quedaros)
  S_EVO_Q, S_EVO_KEEP, S_FAR_Q, S_FAR_GO, S_FAR_STAY,
  S_CHOOSE_STARTER,  // titulo de la eleccion del inicial (primera vez)
  S_NO_SPRITES, S_LOAD_SPRITES,  // aviso cuando falta el sprite en la SD
  // El cuestionario del Profesor. Los NOMBRES (bicho, golpe, tipo) no se
  // traducen: salen de DEX_TBL y MOVE_TBL, igual que en la ficha.
  S_QUIZ_TITLE, S_QUIZ_Q_TYPE, S_QUIZ_Q_EVO, S_QUIZ_Q_MOVE,
  S_QUIZ_Q_HEAVY, S_QUIZ_Q_LEARN, S_QUIZ_RIGHT, S_QUIZ_WRONG,
  S_QUIZ_SCORE_FMT,
  S_PRIZE_FMT,      // lo que te dan: tambien lo usa la medalla
  S_QUIZ_NOPRIZE,
  // La pagina de progreso. Estaban a pelo en el sketch y salian en
  // ingles en los ocho idiomas.
  S_CAP_LV_FMT, S_CAP_FMT, S_EXP_NEXT_FMT,
  // Titulos de la ficha. Tambien estaban a pelo en el sketch.
  S_CARD_MOVES, S_CARD_BAG, S_CARD_TEAM, S_CARD_BANK,
  S_SAVE_FAIL,      // a faixa vermelha: o ultimo save nao gravou
  S_MAP_MEDALS,     // botao do mapa que abre as medalhas
  // a tela de troca entre placas (troca_ui.ino)
  S_TROCA,
  S_TR_PLACA,
  S_TR_SEMRADIO,
  S_TR_FECHAR,
  S_TR_PROCURA,
  S_TR_NINGUEM,
  S_TR_ESPERA,
  S_TR_CANCELAR,
  S_TR_QUER,
  S_TR_DA,
  S_TR_RECEBE,
  S_TR_EVOLUI,
  S_TR_BANCO,
  S_TR_ESCOLHENDO,
  S_TR_CONFIRMOU,
  S_TR_TROCANDO,
  S_TR_PERTO,
  S_TR_CHEGOU,
  S_TR_DESISTIU,
  S_TR_DESISTI,
  S_TR_SD,
  S_TR_SUMIU,
  S_TR_ESCOLHER,
  S_TR_VOLTAR,
  S_TR_OUTRO,
  S_TR_ESPERA2,
  S_TR_EVOLUINDO,
  S_BAG_USAR,
  S_BAG_DAR,
  S_BAG_DARA,
  S_DU_BATALHAR,
  S_DU_OQUE,
  S_DU_FORMATO,
  S_DU_ITENS,
  S_DU_NENHUM,
  S_DU_LIVRE,
  S_DU_ATE3,
  S_DU_NIVEL,
  S_DU_CONVIDAR,
  S_DU_QUER,
  S_DU_ESCOLHA,
  S_DU_FALTA,
  S_DU_PRONTO,
  S_DU_ESPERA,
  S_CONECTAR,
  S_DU_VENCEU,
  S_DU_PERDEU,
  S_DU_EMPATE,
  S_DU_DESSINC,
  S_DU_DESISTI,
  S_DU_PLACAR,
  // la escapada pide confirmacion: pregunta, dejarlo ir, cuidarlo
  S_RUN_Q, S_RUN_GO, S_RUN_STAY,
  S_QUIZ_Q_WHO,     // quem e esse Pokemon? (o quiz com o dibujo)
  // a tela da insignia (leva 71): estava fixa em ingles
  S_BADGE_NEW, S_BADGE_ZCRYSTAL, S_BADGE_FROM, S_BADGE_TM,
  // a cacada da Pokedex (leva 72): o botao, parar, o aviso e o 'nao sai'
  S_DEX_CACAR, S_DEX_PARAR, S_DEX_CACANDO, S_DEX_NAO_SAI,
  // textos de tela que estavam fixos em ingles no sketch (leva 73)
  S_APA_Q, S_APA_PAUSA, S_APA_PAUSA_SUB, S_APA_DESL,
  S_APA_DESL_SUB, S_APA_PIE, S_APA_PAUSADO, S_APA_TCHAU,
  S_APA_ESPERA, S_CURADOS, S_CURADOS_FMT, S_REVANCHA,
  S_REV_SUB_FMT, S_MAPA_VENCE_FMT, S_MAPA_VIAJAR, S_FORA_VOLTAR,
  S_HABILIDADE, S_TOQUE_FECHAR, S_IR_PARA_FMT, S_INSIG_FICAM,
  S_UMA_PEDRA, S_PRECISA_FMT, S_TOQUE_GOLPE, S_ORD_BOLSA,
  S_ORD_AZ, S_ORD_TIPO, S_VAZIO, S_EQUIPADO,
  S_TIRAR, S_SEGURAR, S_EQUIPE_DICA, S_OVO_FMT,
  S_DESMAIOU, S_EM_CAMPO, S_VITORIA, S_OUTROS_EXP_FMT,
  S_GANHOU, S_EM_QUEM_FMT, S_QUEM_PROXIMO, S_FORA_BOLSA,
  S_MANDE_ALGUEM, S_RESTAM_FMT, S_LIVRE_SAVE, S_BOLAS,
  S_CURA, S_NADA_AQUI, S_AVENTURA, S_AV_FORA_FMT,
  S_AV_CURTO, S_AV_SUBIU_FMT, S_AV_PROX_FMT, S_AV_SELVAGEM_FMT,
  S_AV_ENTROU_FMT, S_AV_ACHOU_FMT, S_AV_TERMINAR,
  // as abas da mochila (o POCKET_NAME de items.h fica para a serial)
  S_BOLSO_0, S_BOLSO_1, S_BOLSO_2, S_BOLSO_3, S_BOLSO_4,
  STR_COUNT
};

const char *T(StrId id);       // texto en el idioma activo
const char *medalName(int i);  // banner de medalla (MED_COUNT)
const char *medalLabel(int i); // etiqueta corta de medalla
const char *medalDesc(int i);  // descripcion larga de medalla

void loadLang();             // lee el idioma de NVS (llamar en setup)
void setLang(Lang l);        // cambia y persiste el idioma
