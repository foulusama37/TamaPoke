# -*- coding: utf-8 -*-
"""Le o QR do manual como uma camera leria: abre a tela que o teste
manual74 desenhou (_claude_scratch/prev_qr74_*.ppm) e decodifica com o
leitor do OpenCV. O endereco tem de ser o do manual, no idioma certo.

    ./build/tamapoke_uitests manual74     (em test/)
    python tools/confere_qr.py
"""
import os
import sys
import cv2
import numpy as np

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BASE = "https://github.com/foulusama37/TamaPoke/blob/main/docs/manual/"
ESPERA = {"pt": BASE + "LEIAME.md", "en": BASE + "README.md"}

falhas = 0
for lg, url in ESPERA.items():
    ppm = os.path.join(RAIZ, "_claude_scratch", "prev_qr74_%s.ppm" % lg)
    img = cv2.imread(ppm)
    if img is None:
        print("%s: falta %s (rode o teste manual74)" % (lg, ppm))
        falhas += 1
        continue
    # como uma foto: um pouco menor e borrada, nao o pixel perfeito
    foto = cv2.GaussianBlur(cv2.resize(img, (300, 300), interpolation=cv2.INTER_AREA), (3, 3), 0)
    for nome, im in (("tela", img), ("foto", foto)):
        txt, _, _ = cv2.QRCodeDetector().detectAndDecode(im)
        ok = txt == url
        print("%s %s: %s %s" % (lg, nome, "OK " if ok else "ERRO", txt or "(nao leu)"))
        falhas += not ok
sys.exit(1 if falhas else 0)
