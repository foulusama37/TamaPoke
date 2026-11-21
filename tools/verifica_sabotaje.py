#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""A arvore esta limpa, ou sobrou defeito de uma sabotagem?

A sabotagem injeta um defeito, compila, roda e restaura. Se morrer no
meio —— ou se alguem editar o arquivo enquanto ela corre —— o defeito
FICA. E ai tudo mente: os testes caem por um motivo que nao existe no
codigo de verdade, e o proprio sabotador passa a dizer "o texto a
sabotear aparece 0 vezes", porque o que ele procura (o codigo bom) ja
nao esta la.

Aconteceu: ficou um `if (false) side[who].roosted = false;` em
battle.cpp e so se descobriu porque quatro testes sem relacao nenhuma
passaram a cair.

Isto nao conserta: so DIZ. Para cada entrada da lista, confere se o
texto bom esta no arquivo e se o texto do defeito nao esta.

    python tools/verifica_sabotaje.py
"""
import io
import os
import sys

AQUI = os.path.dirname(os.path.abspath(__file__))
RAIZ = os.path.dirname(AQUI)
sys.path.insert(0, AQUI)

import sabotaje_held                                  # noqa: E402


def main():
    sucios = []
    raros = []
    for entrada in sabotaje_held.SABOTAJES:
        arq, bueno, malo, teste, desc = entrada
        ruta = os.path.join(RAIZ, arq)
        s = io.open(ruta, encoding="utf-8", errors="surrogateescape").read()
        tem_bom = s.count(bueno)
        tem_mau = s.count(malo)
        if tem_bom == 1:
            continue                      # limpo
        if tem_mau >= 1 and tem_bom == 0:
            sucios.append((arq, teste, desc, malo.splitlines()[0][:70]))
        else:
            # nem o bom nem o mau: a ancora envelheceu com uma edicao
            raros.append((arq, teste, tem_bom, tem_mau))

    if sucios:
        print("DEFEITO DE SABOTAGEM AINDA NO CODIGO (%d):" % len(sucios))
        for arq, teste, desc, linha in sucios:
            print("  %s: %s" % (arq, desc))
            print("      %s" % linha)
            print("      (o teste que cobra isto e %s)" % teste)
    if raros:
        print()
        print("ANCORAS QUE NAO CASAM (%d) —— nao provam nada:" % len(raros))
        for arq, teste, b, m in raros:
            print("  %-12s %-50s bom=%d mau=%d" % (arq, teste, b, m))

    if not sucios and not raros:
        print("arvore limpa: as %d entradas casam."
              % len(sabotaje_held.SABOTAJES))
        return 0
    return 1


if __name__ == "__main__":
    sys.exit(main())
