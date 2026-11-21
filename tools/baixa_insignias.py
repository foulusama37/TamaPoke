"""Baixa as insignias das regioes 2..8 (Hoenn a Paldea) para
tools/insignias/<regiao>_<i>.png, na ordem dos ginasios do gyms.h.

    python tools/baixa_insignias.py

Kanto e Johto vem do proprio Anil (gen_art.py). Daqui em diante:
  * WikiDex (a Bulbapedia bloqueia acesso automatico com o Cloudflare);
  * Alola nao tem insignias (sao provas): os Cristais Z de cada capitao,
    do PokeSprite;
  * Paldea: o nosso time tem Arven e Penny, que nao sao lideres —— ficam
    com as da Equipe Star.
"""
import os
import re
import sys
import urllib.parse
import urllib.request

AQUI = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(AQUI, "insignias")
UA = {"User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) Chrome/124"}
PAGINA = "https://www.wikidex.net/wiki/Medalla"
Z = "https://raw.githubusercontent.com/msikma/pokesprite/master/items/z-crystals/%s--bag.png"

# regiao -> as 8, na ordem de gymAt(regiao, i). "z:" = Cristal Z do PokeSprite.
TABELA = {
    # Hoenn na ordem do ANIL (Norman primeiro), a mesma do gyms.h.
    2: ["Equilibrio", "Calor", "Pluma", "Dinamo", "Piedra", "Puño", "Lluvia", "Mente"],
    3: ["Lignito", "Bosque", "Adoquín", "Ciénaga", "Reliquia", "Mina", "Carámbano", "Faro"],
    # Unova de Black/White. O Freeze em espanhol e "Candelizo" (procurar
    # por "Hielo" achava a de Galar e o Brycen ficava sem). A Legend
    # "_NB" e o MESMO desenho em miniatura de 30x54: fica a grande.
    4: ["Trío", "Base", "Élitro", "Voltio", "Temblor", "Jet", "Candelizo", "Leyenda"],
    5: ["Insecto", "Muro", "Lid", "Hoja", "Voltaje", "Hada", "Psique", "Iceberg"],
    6: ["z:normalium-z", "z:fightinium-z", "z:waterium-z", "z:firium-z",
        "z:grassium-z", "z:rockium-z", "z:electrium-z", "z:ghostium-z"],
    7: ["Planta_EpEc", "Agua_EpEc", "Fuego_EpEc", "Lucha_EpEc",
        "Fantasma_EpEc", "Hada_EpEc", "Roca_EpEc", "Hielo_EpEc"],
    8: ["Star_Siniestro", "de_Gimnasio_Planta", "de_Gimnasio_Eléctrico",
        "de_Gimnasio_Psíquico", "de_Gimnasio_Normal", "Star_Hada",
        "de_Gimnasio_Fantasma", "de_Gimnasio_Hielo"],
}


def baixa(url):
    with urllib.request.urlopen(urllib.request.Request(url, headers=UA), timeout=30) as r:
        return r.read()


def originais(html):
    """nome do arquivo -> URL do ORIGINAL (as miniaturas viram o original)."""
    out = {}
    for u in re.findall(r'https://images\.wikidexcdn\.net/mwuploads/wikidex/[^" ]+?\.png', html):
        # miniatura: .../thumb/a/ab/latest/ts/NOME.png/105px-NOME.png —— o
        # regex ja para no primeiro .png, entao basta tirar o "/thumb"
        if "/thumb/" in u:
            u = u.replace("/thumb/", "/", 1)
        nome = urllib.parse.unquote(u.rsplit("/", 1)[1])
        out.setdefault(nome, u)
    return out


def main():
    os.makedirs(OUT, exist_ok=True)
    html = baixa(PAGINA).decode("utf-8", "replace")
    mapa = originais(html)
    # a Legend de Black/White so esta na pagina dela
    mapa.update({k: v for k, v in originais(baixa(PAGINA + "_Leyenda").decode(
        "utf-8", "replace")).items() if k not in mapa})
    faltam = []
    for reg, nomes in TABELA.items():
        for i, n in enumerate(nomes):
            dst = os.path.join(OUT, "%d_%d.png" % (reg, i))
            if n is None:
                faltam.append((reg, i, "-"))
                continue
            if n.startswith("z:"):
                url = Z % n[2:]
            else:
                url = mapa.get("Medalla_%s.png" % n)
                if not url:
                    faltam.append((reg, i, n))
                    continue
            try:
                dados = baixa(url)
            except Exception as e:                       # noqa: BLE001
                faltam.append((reg, i, "%s (%s)" % (n, e)))
                continue
            if not dados.startswith(b"\x89PNG"):
                faltam.append((reg, i, n + " (nao e PNG)"))
                continue
            open(dst, "wb").write(dados)
    print("faltam:", faltam)


if __name__ == "__main__":
    sys.exit(main())
