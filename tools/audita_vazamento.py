"""Acha os sprites do Anil cuja arte e MAIOR que a celula da tira: o que
passa de uma borda reaparece colado na borda oposta do mesmo quadro (a
folha do Shiftry de costas: a ponta da folha direita aparecia sozinha do
lado esquerdo). pack_anil.py desenrola isso com `desenrola()`; este
script so conta quem tem o problema, antes e depois.

    python tools/audita_vazamento.py            # todas as especies
    python tools/audita_vazamento.py 275        # uma
    python tools/audita_vazamento.py --depois   # o que SOBRA apos desenrolar
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import pack_anil  # noqa: E402
from pack_anil import fragmentos  # noqa: E402


def main(argv):
    depois = '--depois' in argv
    argv = [x for x in argv if x != '--depois']
    nums = [int(x) for x in argv] or [d[0] for d in pack_anil.DEX]
    achados = []
    for num in nums:
        cand = pack_anil.NOMBRE.get(num)
        if not cand:
            continue
        for carpeta in ('Front', 'Back'):
            for shiny in (False, True):
                fr = pack_anil.tira(carpeta, cand, shiny, desenrolar=depois)
                if not fr:
                    continue
                q = sum(1 for f in fr if fragmentos(f))
                if q:
                    larg = max(x1 - x0 for f in fr for x0, x1, *_ in fragmentos(f))
                    achados.append((num, cand[0], carpeta + (' shiny' if shiny else ''),
                                    q, len(fr), larg))
    for num, nome, c, q, n, larg in achados:
        print(f"  {num:4d} {nome:14s} {c:12s} {q:3d}/{n:3d} quadros  pedaco ate {larg} px")
    print(f"{len(achados)} tiras com vazamento")


if __name__ == '__main__':
    main(sys.argv[1:])
