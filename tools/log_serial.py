#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Grava o que a placa diz pela serial num arquivo.

Faz falta porque a serial nao guarda nada: o que a placa imprime
enquanto ninguem escuta some. O tempo de leitura das animacoes
(`fx <golpe> <ms>`) so aparece durante um combate, entao sem isto
e preciso estar olhando na hora exata.

    python tools/log_serial.py --port COM6 --seg 600
"""
import argparse
import os
import sys
import time

import serial

AQUI = os.path.dirname(os.path.abspath(__file__))
RAIZ = os.path.dirname(AQUI)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--port', default='COM6')
    ap.add_argument('--seg', type=int, default=600)
    ap.add_argument('--out', default=os.path.join(RAIZ, '_claude_scratch',
                                                  'serial.log'))
    a = ap.parse_args()

    os.makedirs(os.path.dirname(a.out), exist_ok=True)
    try:
        ser = serial.Serial(a.port, 115200, timeout=1)
    except Exception as e:                            # noqa: BLE001
        sys.exit('nao abriu %s: %s' % (a.port, e))
    ser.dtr = True
    ser.rts = True

    t0 = time.time()
    n = 0
    with open(a.out, 'w', encoding='utf-8', newline='\n') as f:
        while time.time() - t0 < a.seg:
            try:
                linha = ser.readline().decode('utf-8', 'replace').strip()
            except Exception:                         # noqa: BLE001
                break
            if not linha:
                continue
            f.write(linha + '\n')
            f.flush()
            n += 1
    ser.close()
    print('%d linhas em %s' % (n, a.out))


if __name__ == '__main__':
    main()
