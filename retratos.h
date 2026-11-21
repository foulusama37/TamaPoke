// GERADO por tools/gen_retratos.py - nao editar a mao.
//
// Os retratos vivem no cartao (/por/por.bin): sao 40x40 por
// estado e por especie, o que nao cabe em flash. Aqui fica so
// a ASSINATURA, que o firmware exige que bata antes de ler ——
// um arquivo indexado por especie mostra o vizinho em silencio
// quando a tabela cresce, que foi o que aconteceu com os sons.

#define POR_ESPECIES 1025
#define POR_RANURAS 7
#define POR_VERSION 2

// Impressao digital de 12 especies, escrita por quem GEROU o
// arquivo. O teste cobra do leitor que batam: e o que pega um
// indice deslocado, que a contagem sozinha nao pega.
#define POR_MUESTRAS 12
static const struct { uint16_t dex; uint8_t caras; uint16_t rle; }
    POR_MUESTRA[POR_MUESTRAS] = {
  { 1, 6, 1534 },
  { 86, 6, 1344 },
  { 171, 6, 1218 },
  { 256, 6, 1150 },
  { 341, 6, 1712 },
  { 426, 6, 1578 },
  { 511, 6, 1584 },
  { 596, 1, 1688 },
  { 681, 1, 1784 },
  { 766, 1, 1680 },
  { 851, 1, 1524 },
  { 936, 1, 1404 },
};

enum {
  POR_NORMAL = 0,
  POR_FOME = 1,
  POR_SONO = 2,
  POR_SEM_PS = 3,
  POR_SUJO = 4,
  POR_FESTA = 5,
  POR_PRONTO = 6,
};
