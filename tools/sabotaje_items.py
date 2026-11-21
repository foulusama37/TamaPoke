#!/usr/bin/env python3
"""Sabotagem das correcoes da BOLSA EM COMBATE.

Irma de `sabotaje_held.py`, mas para os testes de INTERFACE: as pocoes,
as X e as curas aplicam-se sobre o combatente em pista, e isso so se ve
tocando na bolsa. Desfaz cada correcao, corre o teste que a cobre e
exige que fique VERMELHO.

Um teste que nunca falhou nao provou nada. Nesta mesma sessao o build
que devia dar vermelho morreu a meio por eu editar os ficheiros com o
make a correr, e os testes nasceram ja em cima do codigo corrigido ——
sem isto eu nao teria nenhuma prova de que apanham o defeito.

  python tools/sabotaje_items.py
"""
import io
import os
import shutil
import subprocess
import sys
import tempfile

AQUI = os.path.dirname(os.path.abspath(__file__))
RAIZ = os.path.dirname(AQUI)
TEST = os.path.join(RAIZ, "test")

# (ficheiro, texto a procurar, com que se troca, teste que tem de cair, nome)
SABOTAJES = [
    ("capture.ino",
     "  if (slot == bt.mySlot) return btItemEnPista(id, aplicar);",
     "  if (false) return btItemEnPista(id, aplicar);",
     "una_pocion_cura_al_que_esta_EN_PISTA",
     "el objeto vuelve a caer sobre lo GUARDADO y no sobre el que pelea"),

    ("capture.ino",
     "      sirvio = c.applyStage(it.arg, 2);",
     "      sirvio = c.applyStage(it.arg, 1);",
     "una_X_Attack_sube_DOS_escalones",
     "as X voltam a subir um so escalao"),

    ("capture.ino",
     "        if (c.status == ST_NONE && c.confuseTurns == 0) break;\n"
     "        if (!aplicar) { sirvio = true; break; }\n"
     "        c.confuseTurns = 0;",
     "        if (c.status == ST_NONE) break;",
     "una_Full_Heal_quita_el_estado_Y_la_confusion",
     "a Full Heal deixa de tirar a confusao"),

    # A linha que REMATAVA o estrago: copiava a vida do principal por cima
    # da do suplente. Aqui poe-se de volta, com a correcao no sitio.
    ("capture.ino",
     "    btItemEnSlot(id, i, true);",
     "    btItemEnSlot(id, i, true);\n"
     "    bt.b.side[BT_PLAYER].hp = pet.hpLeft;",
     "con_un_suplente_fuera_la_pocion_no_toca_al_principal",
     "vuelve a copiar la vida del principal encima del suplente"),

    # ---------------------------------------- a MEGA na Pokedex
    # O cabo que vai do combate a ficha. E o unico que responde ao que
    # foi pedido: os outros dois testes chamam registerMega a mao.
    ("battle_ui.ino",
     "        pet.registerMega(me.megaIdx);",
     "        (void)0;",
     "megaevolucionar_en_combate_deja_la_mega_en_la_ficha",
     "megaevoluir deixa de avisar a Pokedex"),

    # A lista de versoes da ficha.
    ("TamaPoke.ino",
     "  for (uint16_t i = 0; i < MEGA_COUNT && galleryNVers < GVER_MAX; i++)\n"
     "    if (MEGA_TBL[i].base == dex && pet.isMegaRegistered((int16_t)i))\n"
     "      galleryVers[galleryNVers++] = (int16_t)(GVER_MEGA0 - (int16_t)i);",
     "  (void)0;",
     "una_mega_vista_se_apunta_y_sale_como_version",
     "a ficha deixa de oferecer a mega como versao"),

    # E que a habilidade seja a DELA e nao a da especie base.
    ("TamaPoke.ino",
     "  if (mg >= 0 && MEGA_TBL[mg].ability) { out[0] = MEGA_TBL[mg].ability; return 1; }",
     "  if (false) { out[0] = 0; return 1; }",
     "la_ficha_de_una_mega_ensena_SU_habilidad",
     "a ficha volta a mostrar a habilidade da especie base"),

    # ---------------------------------------- os TRES da casa
    # O ARTICUNO. O teto de forca NAO chega: COSMOG tem 200 de BST e
    # entra no teto de um treinador de nivel 2.
    ("battle_ui.ino",
     "  if (e.rarity == R_LEGENDARIO) return 0;",
     "  if (false) return 0;",
     "los_de_la_casa_no_sacan_legendarios_NUNCA",
     "os tres da casa voltam a poder sacar lendarios"),

    # O teto atado ao nivel: sem ele, o primeiro do Brock e o terceiro
    # do Giovanni sacam a mesma coisa.
    ("battle_ui.ino",
     "  uint16_t tope = (uint16_t)(300 + (uint16_t)nivel * 5);",
     "  uint16_t tope = 600; (void)nivel;",
     "los_de_la_casa_pegan_segun_SU_nivel",
     "a forca do bicho deixa de depender do nivel do treinador"),

    # A pedra do lider, que nunca se deu: o AS e o ULTIMO, e so se
    # repartia item no battleStart.
    ("battle_ui.ino",
     "  nuevo.item = btFoeItem(d, bt.gym, (uint8_t)bt.foeIdx);",
     "  (void)0;",
     "el_lider_saca_su_piedra_mega_con_el_que_la_lleva",
     "o segundo e o terceiro do lider voltam a sair de maos vazias"),

    # A mega so da Elite 4 para cima.
    ("gyms.h",
     "#define MEGA_DESDE_GIMNASIO 8",
     "#define MEGA_DESDE_GIMNASIO 4",
     "un_lider_normal_NO_megaevoluciona",
     "a mega volta a chegar aos lideres normais"),

    # ---------------------------------------- a REVANCHA
    # O nivel: sem isto a revancha e bater outra vez num Brock de nivel 8.
    ("battle_ui.ino",
     "  if (!bt.rematch) return gymLevelAt(g, i);\n"
     "  return gymChampLevel(pet.region);",
     "  (void)0;\n"
     "  return gymLevelAt(g, i);",
     "todos_pelean_al_nivel_del_campeon_y_con_mega",
     "a revancha deixa de nivelar pelo campeao"),

    # E a guarda que impede a revancha de dar insignia outra vez.
    ("pet.cpp",
     "void Pet::winGym() {\n  if (allGymsDone()) return;",
     "void Pet::winGym() {\n  if (false) return;",
     "ganarla_no_da_otra_insignia",
     "ganhar com a regiao feita volta a dar insignia"),

    # ------------------------------------- o que SE VE no combate
    # Tres coisas que o dono apanhou a jogar e que nenhum teste olhava:
    # todos os de combate afirmavam sobre PS e sobre fases, nenhum sobre
    # ONDE e SE cada coisa se desenhou.

    # EXCAVAR / VUELO / BUCEO: o motor tira o bicho do campo
    # (Combatant::hiding) e o desenhador nao olhava para isso. Sabota-se
    # a LEITURA do escondite, que e a raiz —— assim cai tanto a metade
    # do ceu como a da terra.
    ("battle_ui.ino",
     "    uint8_t ahora = bt.b.side[s].hiding;",
     "    uint8_t ahora = HID_NONE;",
     "el_que_vuela_DESAPARECE_de_la_pantalla",
     "o sprite volta a ficar na tela enquanto esta escondido"),

    # DE QUE LADO VEM CADA BOLA. Um sinal trocado, e cada bola entrava
    # pelo lado do treinador do outro.
    ("battle_ui.ino",
     "    int x = cx + (int)((mio ? -40 : 40) * (1.0f - caida));",
     "    int x = cx + (int)((mio ? 40 : -40) * (1.0f - caida));",
     "cada_bola_entra_desde_el_lado_de_su_entrenador",
     "as bolas voltam a entrar trocadas de lado"),

    # A MEGA DO RIVAL, que era instantanea: um doMega() na mesma linha
    # do toque, sem aneis, sem selo e sem som.
    ("battle_ui.ino",
     "  if (bt.b.canMega(BT_FOE) >= 0) {",
     "  if (false) {",
     "la_mega_del_rival_no_es_instantanea",
     "a mega do rival deixa de ter cena"),

    # ------------------------------------------------- os TIPOS
    # "nenhum lugar mostra o tipo do pokemon que temos, nem na pokedex e
    # nem no menu". A chapa existia; nenhuma tela do bicho a usava.
    ("TamaPoke.ino",
     "    drawTypesOf(233 - (2 * 104 + 10) / 2, 264, 104, 28, 10, false, t1, t2, 1);",
     "    (void)t1; (void)t2;",
     "la_pagina_de_combate_dice_LOS_DOS_tipos",
     "a pagina de combate volta a nao dizer o tipo"),

    ("TamaPoke.ino",
     "      drawTypesOf(26, 164, 108, 28, 6, true, t1, t2, 1);",
     "      (void)t1; (void)t2;",
     "la_ficha_de_la_pokedex_dice_el_tipo",
     "a ficha da Pokedex volta a nao dizer o tipo"),

    # E que seja o da VERSAO que se olha: uma mega troca de tipo.
    ("TamaPoke.ino",
     "      if (mg >= 0)     { t1 = MEGA_TBL[mg].type1; t2 = MEGA_TBL[mg].type2; }",
     "      if (false)       { t1 = T_NONE; t2 = T_NONE; }",
     "una_mega_ensena_SU_tipo_y_no_el_de_la_base",
     "a mega volta a mostrar o tipo da especie base"),

    # E que de um bicho sem registar NAO se diga nada: a chapa seria um
    # chivatazo do que ainda nao encontraste.
    ("TamaPoke.ino",
     "    if (reg) {\n      int16_t q = galleryVers[galleryVer];",
     "    if (true) {\n      int16_t q = galleryVers[galleryVer];",
     "de_uno_sin_registrar_NO_se_dice_el_tipo",
     "a ficha passa a chivar o tipo de quem ainda nao criaste"),

    # ------------------------------- ELEGIR EN QUIEN se usa el objeto
    # "revive esta quebrado, nao tem como usar na batalha pois nao deixa
    # eu escolher o pokemon".

    # La puerta: los objetos que el PBS marca OnPokemon abren la lista.
    ("capture.ino",
     "      } else if (ITEM_TBL[id].use == IU_POKEMON) {",
     "      } else if (false) {",
     "un_revive_abre_la_lista_en_vez_de_aplicarse_solo",
     "la bolsa vuelve a aplicarlo todo al de pista"),

    # Y que la lista, con objeto en la mano, APLIQUE en vez de relevar.
    ("battle_ui.ino",
     "  if (bt.itemPick) return btItemPickTap(x, y);",
     "  if (false) return btItemPickTap(x, y);",
     "un_revive_abre_la_lista_en_vez_de_aplicarse_solo",
     "tocar un hueco con el objeto en la mano vuelve a relevar"),

    # Que no se gaste en quien no le sirve.
    ("capture.ino",
     "    if (!btItemEnSlot(id, i, false)) { sfxPlay(SFX_DENY); return true; }",
     "    if (false) { sfxPlay(SFX_DENY); return true; }",
     "la_lista_apaga_a_quien_el_objeto_no_le_haria_nada",
     "se puede gastar un Revivir en uno que esta en pie"),

    # Y que un Revivir mire si el otro esta CAIDO.
    ("capture.ino",
     "      if (vida != 0) break;            // solo a los caidos",
     "      if (false) break;",
     "la_lista_apaga_a_quien_el_objeto_no_le_haria_nada",
     "el Revivir deja de mirar si el otro esta caido"),

    # ------------------------------------------------- LA SOMBRA
    # "mtos pokemon a sombra nao casa (pé voando)". Se centraba en el
    # medio de la TINTA y un ala abierta se la llevaba lejos del pie:
    # 819 de 1025 especies a 3 px o mas, mediana 12, Lugia 97.

    # El centro, que es lo que se arreglo.
    ("battle_ui.ino",
     "  int sx = cx + (int)((ac.centroPie() - ac.w * 0.5f) * px);",
     "  int sx = cx + (int)(((ac.footL + ac.footR) * 0.5f - ac.w * 0.5f) * px);",
     "cae_bajo_los_PIES_y_no_bajo_el_ala",
     "la sombra vuelve a centrarse en el medio de la tinta"),

    # Y la MEDIDA del apoyo: si se vuelve a sacar del lienzo entero, el
    # centro queda bien calculado sobre un dato malo.
    ("sdmon.cpp",
     "      const uint8_t *fila = fb + (uint32_t)(base - 1) * w;",
     "      const uint8_t *fila = fb;",
     "el_apoyo_se_mide_en_la_fila_que_toca",
     "el apoyo se mide en la fila de ARRIBA en vez de la que toca"),
]

# Em Windows o binario leva .exe e "./build/..." nao resolve no subprocess.
BIN = os.path.join(TEST, "build", "tamapoke_uitests")
if not os.path.exists(BIN) and os.path.exists(BIN + ".exe"):
    BIN += ".exe"

MAKE = shutil.which("mingw32-make") or shutil.which("make")


def corre(nombre):
    r = subprocess.run([BIN, nombre], cwd=TEST, capture_output=True, text=True)
    return r.returncode == 0, r.stdout + r.stderr


def construye():
    if not MAKE:
        return False, "nao encontro mingw32-make nem make no PATH"
    # `uibin`, que COMPILA sem correr. Nao se pede "build/tamapoke_uitests":
    # esse caminho nao e um alvo, e o make, vendo o ficheiro ja ali, cala-se
    # e devolve 0 —— ficava-se a medir o binario velho.
    r = subprocess.run([MAKE, "-s", "uibin"],
                       cwd=TEST, capture_output=True, text=True)
    return r.returncode == 0, r.stdout + r.stderr


def escribe(ruta, texto):
    """Escreve E atrasa o mtime para tras nao ficar: `shutil.copy2` ja me
    deixou uma vez com o objeto sabotado mais novo que o fonte restaurado,
    e o make nao recompilou —— eu estava a medir o binario errado a
    acreditar que era o corrigido."""
    io.open(ruta, "w", encoding="utf-8", errors="surrogateescape",
            newline="\n").write(texto)
    os.utime(ruta, None)


def main():
    if not os.path.exists(BIN):
        print("nao ha binario de UI: corre `make ui` primeiro")
        sys.exit(1)
    fallos = []
    tmp = tempfile.mkdtemp(prefix="sabitems-")
    print("copias em", tmp)
    for fichero, buscar, cambiar, teste, nombre in SABOTAJES:
        ruta = os.path.join(RAIZ, fichero)
        orig = io.open(ruta, encoding="utf-8", errors="surrogateescape").read()
        if orig.count(buscar) != 1:
            fallos.append("%s: o texto a sabotar aparece %d vezes"
                          % (nombre, orig.count(buscar)))
            continue
        shutil.copy2(ruta, os.path.join(tmp, os.path.basename(fichero)))
        escribe(ruta, orig.replace(buscar, cambiar))
        try:
            okc, salida = construye()
            if not okc:
                fallos.append("%s: nao compila sabotado\n%s"
                              % (nombre, salida[-900:]))
                continue
            verde, salida = corre(teste)
            if verde:
                fallos.append("%s: o teste %s FICOU VERDE com o defeito posto"
                              % (nombre, teste))
                print("  VERMELHO ESPERADO, VERDE OBTIDO:", nombre)
            else:
                print("  ok  cai %s  (%s)" % (teste, nombre))
        finally:
            escribe(ruta, orig)

    okc, salida = construye()
    if not okc:
        fallos.append("nao volta a compilar ao restaurar\n" + salida[-900:])

    if fallos:
        print("\nFALHAS:")
        for f in fallos:
            print(" -", f)
        sys.exit(1)
    print("\n%d/%d: cada correcao tem um teste que cai sem ela."
          % (len(SABOTAJES), len(SABOTAJES)))


if __name__ == "__main__":
    main()
