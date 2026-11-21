// A tabela de MT ANTIGA (304 MT), escrita a mao a partir do tms.h de
// 2026-10-24 e mantida so para MIGRAR saves. Nao e gerada e nao muda.
//
// Aquele tms.h foi feito contra uma tabela de 718 golpes e ficou para tras
// quando moves.h passou a 725: TM_MOVE guarda INDICES, entao cada MT
// apontava para outro golpe (a de AERIALACE ensinava ACUPRESSURE). Os
// numeros abaixo sao exatamente os que o firmware usou, lidos contra o
// moves.h de 725 golpes —— ou seja, o golpe que o jogador VIA e aprendia.
// Pet::load usa isto para levar cada MT que voce tinha para a MT nova do
// MESMO golpe, em vez de zerar a colecao.
#pragma once
#include <stdint.h>

#define TM_COUNT_V304 304
#define TM_BYTES_V304 38
static const uint16_t TM_MOVE_V304[TM_COUNT_V304] = {
  5, 6, 7, 9, 10, 11, 12, 14, 20, 25, 29, 30,
  34, 42, 44, 48, 52, 54, 55, 57, 58, 65, 68, 69,
  70, 71, 72, 75, 76, 77, 78, 80, 84, 85, 86, 87,
  88, 90, 99, 104, 105, 110, 113, 114, 117, 118, 121, 122,
  123, 124, 126, 128, 129, 134, 137, 139, 142, 148, 149, 150,
  152, 153, 157, 160, 161, 162, 163, 165, 167, 168, 170, 171,
  172, 173, 174, 176, 177, 180, 182, 183, 184, 185, 188, 191,
  192, 195, 198, 200, 201, 208, 209, 211, 212, 218, 219, 221,
  222, 223, 226, 227, 229, 231, 232, 233, 236, 239, 240, 242,
  246, 247, 250, 251, 257, 259, 260, 266, 268, 271, 272, 276,
  281, 282, 283, 284, 285, 286, 288, 293, 294, 296, 300, 303,
  304, 306, 308, 310, 311, 312, 313, 315, 318, 319, 320, 330,
  333, 334, 338, 339, 341, 346, 347, 350, 351, 356, 359, 362,
  370, 371, 372, 375, 376, 378, 390, 391, 399, 400, 401, 404,
  406, 407, 411, 421, 424, 426, 427, 430, 431, 433, 435, 437,
  440, 441, 444, 447, 448, 449, 451, 454, 456, 457, 461, 462,
  464, 465, 466, 468, 469, 471, 485, 488, 491, 492, 493, 494,
  495, 497, 498, 499, 500, 502, 503, 504, 505, 506, 508, 511,
  512, 513, 517, 521, 522, 524, 525, 526, 527, 529, 531, 532,
  533, 535, 536, 537, 539, 547, 548, 550, 554, 556, 563, 564,
  565, 566, 571, 573, 574, 576, 577, 578, 587, 592, 597, 600,
  601, 602, 606, 608, 609, 612, 614, 615, 618, 619, 621, 622,
  623, 625, 626, 630, 632, 634, 639, 641, 642, 645, 650, 651,
  655, 656, 657, 660, 661, 663, 666, 667, 668, 671, 672, 680,
  681, 682, 683, 685, 689, 690, 692, 694, 699, 700, 704, 705,
  709, 710, 713, 716,
};

// Os indices acima valem contra ESTA tabela de golpes. Se moves.h mudar
// de tamanho, eles deixam de dizer que golpe era: Pet::load entao nao
// migra (e zera as MT, como fazia antes).
#define TM_V304_MOVE_COUNT 725
