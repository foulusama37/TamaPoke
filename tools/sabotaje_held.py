#!/usr/bin/env python3
"""Sabotagem das correcoes de OBJETOS EQUIPADOS.

`tools/sabotaje.py` prova os verificadores de dados (auditoria,
contraste). Isto prova as correcoes do MOTOR: desfaz cada uma, corre o
teste que a cobre e exige que fique VERMELHO.

Um teste que nunca falhou nao provou nada, e nesta mesma sessao dois
testes meus passaram com a correcao desfeita porque mediam a si mesmos.
Por isso cada entrada diz tambem QUE teste tem de cair: se cair outro,
ou se nenhum cair, isto avisa.

  python tools/sabotaje_held.py
"""
import io
import os
import re
import shutil
import subprocess
import sys
import tempfile

AQUI = os.path.dirname(os.path.abspath(__file__))
RAIZ = os.path.dirname(AQUI)
TEST = os.path.join(RAIZ, "test")

# (fichero, texto a buscar, con que se troca, teste que tem de cair, nome)
SABOTAJES = [
    # Levas 7, 8 y 9: lo que quedaba de la auditoria contra el Anil.
    ("battle.cpp",
     '  att.lastFailed = (r.failed && !nada) || r.missed ||\n                   (r.effect == 0 && !r.blockedByProtect);',
     '  att.lastFailed = (r.failed && !nada) || r.missed;',
     'pataleta_dobla_tras_un_golpe_que_no_afecto',
     'L7: que no le afecte ya no cuenta como fallo'),

    ("battle.cpp",
     '    // quien salto con una Patada Salto Alta se estrella igual.\n    seEstrella();',
     '    // quien salto con una Patada Salto Alta se estrella igual.\n    att.lastFailed = true;\n    seEstrella();',
     'pataleta_NO_dobla_si_lo_que_paso_fue_un_protect',
     'L7: el Protect vuelve a contar como fallo'),

    ("battle.cpp",
     '  if (who <= 1) side[who].lastRoundFailed = side[who].lastFailed;\n',
     '',
     'pataleta_dobla_tras_un_golpe_que_no_afecto',
     'L7: el fallo de la ronda no se copia al cerrar'),

    ("battle.cpp",
     '  yo.lastFailed = false;      // usar un objeto no es fallar un golpe\n',
     '',
     'pataleta_mira_la_ronda_anterior_y_usar_un_objeto_la_limpia',
     'L7: usar un objeto ya no limpia el fallo'),

    ("battle.cpp",
     '  att.lastFailed = (r.failed && !nada) || r.missed ||',
     '  att.lastFailed = r.failed || r.missed ||',
     'salpicadura_no_cuenta_como_fallo_para_pataleta',
     'L7: Salpicadura vuelve a contar como fallo'),

    ("battle.cpp",
     "      r.hits = 0;      // fallar no es 'pegar una vez y hacer cero'\n      att.lastFailed = true;\n",
     "      r.hits = 0;      // fallar no es 'pegar una vez y hacer cero'\n",
     'pataleta_dobla_tras_fallar_la_punteria',
     'L7: fallar la punteria ya no cuenta'),

    ("battle.cpp",
     '    // quien salto con una Patada Salto Alta se estrella igual.\n    seEstrella();\n',
     '    // quien salto con una Patada Salto Alta se estrella igual.\n',
     'patada_salto_alta_se_estrella_contra_un_protect',
     'L7: contra un Protect ya no se estrella'),

    ("battle.cpp",
     '  if (!r.noDamage && r.effect == 0 && !r.failed) seEstrella();\n',
     '',
     'patada_salto_alta_se_estrella_contra_un_fantasma',
     'L7: contra un inmune ya no se estrella'),

    ("battle.cpp",
     '      seEstrella();             // saltar contra quien no esta tambien duele\n',
     '',
     'patada_salto_alta_se_estrella_si_el_rival_esta_escondido',
     'L7: contra un escondido ya no se estrella'),

    ("battle.cpp",
     '    if (att.abil() == AB_MAGIC_GUARD || att.fainted()) return;',
     '    if (att.fainted()) return;',
     'muro_magico_no_se_estrella',
     'L7: Muro Magico se estrella'),

    ("battle.cpp",
     '  r.hits = hechos;',
     '  r.hits = golpes;',
     'los_impactos_que_se_cuentan_son_los_que_entraron',
     'L7: se cuentan los impactos previstos y no los dados'),

    ("battle.cpp",
     '      uint32_t p = 20 + 20 * (uint32_t)subidas;\n      pow = (uint16_t)p;',
     '      uint32_t p = 20 + 20 * (uint32_t)subidas;\n      pow = (uint16_t)(p > 250 ? 250 : p);',
     'poder_reserva_no_tiene_tope',
     'L7: Poder Reserva vuelve a tener tope'),

    ("battle.cpp",
     '    m.power = (uint8_t)(t < 40 ? 40 : t < 70 ? 80 : 120);',
     '    m.power = (uint8_t)(t < 30 ? 40 : t < 60 ? 80 : 120);',
     'regalo_reparte_como_el_anil_40_30_10_20',
     'L7: el dado del Regalo vuelve a 30/30/20'),

    ("battle.cpp",
     '    if (t >= 80) {\n      r.presentHeal = true;',
     '    if (t >= 90) {\n      r.presentHeal = true;',
     'regalo_reparte_como_el_anil_40_30_10_20',
     'L7: el Regalo cura la mitad de veces'),

    ("battle.cpp",
     '      r.noDamage = true;\n      r.effect = 100;\n      return r;\n    }\n    m.power = (uint8_t)',
     '      r.noDamage = true;\n    }\n    m.power = (uint8_t)',
     'regalo_reparte_como_el_anil_40_30_10_20',
     'L7: el Regalo que cura vuelve a mirar los tipos'),

    ("battle.cpp",
     '          if (def.fainted() || def.hp >= def.hpMax || def.healBlockTurns) {',
     '          if (def.fainted()) {',
     'regalo_cura_un_cuarto_y_falla_si_no_hay_que_curar',
     'L7: el Regalo ya no falla contra uno entero o con Anticura'),

    ("battle.cpp",
     '        if (att.noRetreat) { r.failed = true; break; }\n',
     '',
     'bastion_final_sube_una_vez_y_ya_no_se_releva',
     'L7: Bastion Final se puede repetir'),

    ("battle.cpp",
     '  if (yo.bindTurns || yo.trapped2 || yo.noRetreat) return false;\n  const Combatant &otro = side[quien ^ 1];',
     '  if (yo.bindTurns || yo.trapped2) return false;\n  const Combatant &otro = side[quien ^ 1];',
     'bastion_final_sube_una_vez_y_ya_no_se_releva',
     'L7: con Bastion Final se puede relevar'),

    ("battle.cpp",
     '  if (yo.bindTurns || yo.trapped2 || yo.noRetreat) return false;\n  // IMAN: al rival del tipo que diga su arg no le deja escapar.',
     '  if (yo.bindTurns || yo.trapped2) return false;\n  // IMAN: al rival del tipo que diga su arg no le deja escapar.',
     'bastion_final_sube_una_vez_y_ya_no_se_releva',
     'L7: con Bastion Final se puede huir'),

    ("battle.cpp",
     '        if (!(att.bindTurns || att.trapped2)) {\n          att.noRetreat = true;',
     '        if (true) {\n          att.noRetreat = true;',
     'bastion_final_atrapado_por_otra_cosa_no_queda_marcado',
     'L7: Bastion Final marca aunque ya estuviera atrapado'),

    ("battle.cpp",
     '  yo.lastFailed = false;      // usar un objeto no es fallar un golpe\n  preparaLado(*this, BT_FOE, slotRival);',
     '  yo.lastFailed = false;      // usar un objeto no es fallar un golpe',
     'en_la_ronda_del_objeto_el_rival_tambien_forcejea_si_no_tiene_golpe',
     'L7: en la ronda del objeto el rival no prepara la suya'),

    ("battle.cpp",
     '  return megaFormOf(c.dex, it.arg, c.form) >= 0;\n}\n\nHitResult computeHit(',
     '  return false;\n}\n\nHitResult computeHit(',
     'desarme_no_le_quita_la_piedra_mega_a_su_dueno',
     'L8: la piedra del que aun no megaevoluciono se puede quitar'),

    ("battle.cpp",
     '  if (c.isMega()) return MEGA_TBL[c.megaIdx].item == it.arg;',
     '  if (c.isMega()) return false;',
     'desarme_no_le_quita_la_piedra_mega_a_su_dueno',
     'L8: la piedra del megaevolucionado se puede quitar'),

    ("battle.cpp",
     '      if (def.item != IT_NONE && !objetoIntocable(def, def.item))\n        pow = (uint16_t)(pow * 3 / 2);',
     '      if (def.item != IT_NONE)\n        pow = (uint16_t)(pow * 3 / 2);',
     'desarme_solo_pega_mas_si_lo_que_lleva_se_puede_tirar',
     'L8: Desarme pega mas contra una piedra intocable'),

    ("battle.cpp",
     '    if (fxi == FX_KNOCK_OFF && def.item != IT_NONE && !r.subHit &&\n        !pegaSalvaje && !objetoIntocable(def, def.item)) {',
     '    if (fxi == FX_KNOCK_OFF && def.item != IT_NONE && !r.subHit &&\n        !objetoIntocable(def, def.item)) {',
     'un_salvaje_no_quita_ni_roba_ni_trapichea',
     'L8: un salvaje tira objetos con Desarme'),

    ("battle.cpp",
     '    if (fxi == FX_KNOCK_OFF && def.item != IT_NONE && !r.subHit &&\n        !pegaSalvaje && !objetoIntocable(def, def.item)) {',
     '    if (fxi == FX_KNOCK_OFF && def.item != IT_NONE &&\n        !pegaSalvaje && !objetoIntocable(def, def.item)) {',
     'tras_el_muneco_no_se_quita_nada',
     'L8: Desarme quita el objeto a traves del muneco'),

    ("battle.cpp",
     '               !objetoIntocable(att, def.item) &&\n               (att.item == IT_NONE || recibeSalvaje)) {',
     '               (att.item == IT_NONE || recibeSalvaje)) {',
     'ladron_y_trapicheo_tampoco_tocan_la_piedra',
     'L8: Ladron roba la piedra que seria la suya'),

    ("battle.cpp",
     '               !pegaSalvaje && !objetoIntocable(def, def.item) &&',
     '               !pegaSalvaje &&',
     'ladron_y_trapicheo_tampoco_tocan_la_piedra',
     'L8: Ladron le roba la piedra a su dueno'),

    ("battle.cpp",
     '      if (recibeSalvaje) r.itemToBag = true;\n      else att.item = def.item;',
     '      att.item = def.item;',
     'ladron_a_un_salvaje_va_a_la_mochila',
     'L8: lo robado a un salvaje se queda en la mano'),

    ("battle.cpp",
     '               (att.item == IT_NONE || recibeSalvaje)) {',
     '               (att.item == IT_NONE)) {',
     'ladron_a_un_salvaje_va_a_la_mochila',
     'L8: con las manos ocupadas no se roba a un salvaje'),

    ("battle.cpp",
     '            objetoIntocable(def, def.item) || objetoIntocable(def, att.item) ||\n            objetoIntocable(att, att.item) || objetoIntocable(att, def.item)) {',
     '            false) {',
     'ladron_y_trapicheo_tampoco_tocan_la_piedra',
     'L8: Trapicheo cambia piedras intocables'),

    ("battle.cpp",
     '        if ((from == BT_FOE && !isTrainer) ||\n            objetoIntocable(def, def.item)',
     '        if (false ||\n            objetoIntocable(def, def.item)',
     'un_salvaje_no_quita_ni_roba_ni_trapichea',
     'L8: un salvaje trapichea'),

    ("battle.cpp",
     '            !(from == BT_PLAYER && !isTrainer) &&\n',
     '',
     'un_salvaje_con_hurto_no_le_quita_el_objeto_a_quien_le_toca',
     'L8: un salvaje hurta'),

    ("battle.cpp",
     '      c.lastBerry = c.item;    // cual era, para que Cosecha la devuelva\n      c.perderObjeto(true);',
     '      c.lastBerry = c.item;    // cual era, para que Cosecha la devuelva\n      c.perderObjeto();',
     'lo_que_se_gasta_queda_apuntado_y_lo_que_le_quitan_no',
     'L8: comerse la baya no queda apuntado como gasto'),

    ("battle.cpp",
     '      HitResult f = computeHit(fuera ? futureDe[q] : side[q], victima, mvF,',
     '      HitResult f = computeHit(futureDe[q], victima, mvF,',
     'premonicion_pega_con_los_stats_de_cuando_cae',
     'L9: la Premonicion no usa al que sigue en pista'),

    ("battle.cpp",
     '    for (uint8_t i = 0; i < SG_COUNT; i++) futureDe[quien].stage[i] = 0;\n',
     '',
     'premonicion_del_que_se_fue_pega_sin_sus_subidas',
     'L9: la foto del que se fue conserva sus subidas'),

    ("battle.cpp",
     '    futureCaido[quien] = side[quien].fainted();',
     '    futureCaido[quien] = false;',
     'premonicion_no_llega_si_el_que_la_lanzo_cayo',
     'L9: la Premonicion llega aunque el que la lanzo cayera'),

    ("battle.cpp",
     '      if (f.effect == 0 || !f.damage) continue;     // no le afecta\n      uint16_t golpe = f.damage;',
     '      uint16_t golpe = f.damage ? f.damage : 1;',
     'premonicion_no_le_hace_nada_al_siniestro_que_entro_despues',
     'L9: la Premonicion pega algo al inmune'),

    # Leva 6: sin golpe que elegir = Forcejeo (pbAutoChooseMove).
    ("battle.cpp",
     '    return slotBloqueado(quien, c.encoreSlot);\n',
     '    return false;\n',
     'encore_sobre_un_golpe_anulado_es_forcejeo_y_no_turno_perdido',
     'Encore sobre un golpe cerrado vuelve a perder el turno'),

    ("battle.cpp",
     '  for (uint8_t i = 0; i < c.nMoves && i < BT_MOVES; i++)\n    if (!slotBloqueado(quien, i)) return false;\n  return true;\n}',
     '  for (uint8_t i = 0; i < c.nMoves && i < BT_MOVES; i++)\n    if (!slotBloqueado(quien, i)) return false;\n  return false;\n}',
     'sin_encore_solo_forcejea_si_no_le_queda_NINGUNO',
     'con todo cerrado ya no forcejea'),

    ("battle.cpp",
     '  if (c.lockTurns || c.recharging || c.chargingMove != 0xFFFF || c.bideTurns)\n    return false;\n  // Con Encore solo cuenta ESE golpe',
     '  // Con Encore solo cuenta ESE golpe',
     'a_medio_cargar_o_enfurecido_no_hay_forcejeo',
     'forcejea a medio Enfado o cargando'),

    ("battle.cpp",
     '  if (!forcejea) {\n  // Disable: ese hueco',
     '  if (true) {\n  // Disable: ese hueco',
     'encore_sobre_un_golpe_anulado_es_forcejeo_y_no_turno_perdido',
     'las puertas vuelven a cortar el Forcejeo'),

    ("battle.cpp",
     '  const bool forcejea = att.forcejea && forzado == 0xFFFF;',
     '  const bool forcejea = debeForcejear(from) && forzado == 0xFFFF;',
     'lo_que_le_cierran_a_mitad_de_ronda_falla_y_no_forcejea',
     'el Forcejeo se decide al moverse y no al elegir'),

    ("battle.cpp",
     '  att.lastSlot = (mv == MOVE_STRUGGLE) ? (uint8_t)0xFF : slot;',
     '  att.lastSlot = slot;',
     'forcejeo_no_cuenta_como_ultimo_golpe_para_encore_ni_anular',
     'Forcejeo vuelve a contar como ultimo golpe'),

    ("battle.cpp",
     '        def.encoreSalta = !movedThisTurn[from ^ 1] &&\n                          slotBloqueado(from ^ 1, def.encoreSlot);\n',
     '        def.encoreSalta = false;\n',
     'encore_a_mitad_de_ronda_sobre_un_golpe_cerrado_espera_a_la_siguiente',
     'el Encore a mitad de ronda le come el turno'),

    ("battle.cpp",
     '  if (who <= 1) side[who].encoreSalta = false;\n',
     '',
     'cuando_se_le_abre_el_golpe_el_encore_vuelve_a_obligar',
     'la espera del Encore no se borra al cerrar la ronda'),

    ("battle.cpp",
     '  if (objeto(att).fx == IT_CHOICE && att.choiceSlot == 0xFF &&\n      mv != MOVE_STRUGGLE)',
     '  if (objeto(att).fx == IT_CHOICE && att.choiceSlot == 0xFF)',
     'forcejeo_no_ata_la_choice',
     'Forcejeo ata la Choice'),

    ("battle.cpp",
     '  if (yawnDurmio == quien) yawnDurmio = 0xFF;\n  if (encoreFin == quien) encoreFin = 0xFF;\n',
     '',
     'el_aviso_de_encore_o_bostezo_no_se_le_cuelga_al_que_entra',
     'el aviso se le cuelga al que entra'),

    ("battle.cpp",
     '  if (forcejea) att.noRepetir = 0;   // uso otro golpe: el Martillo vuelve\n',
     '',
     'encore_sobre_el_martillo_giga_alterna_con_forcejeo',
     'tras el Forcejeo el Martillo sigue cerrado'),

    ("battle.cpp",
     '      mv != MOVE_STRUGGLE && random(100) < 30) {',
     '      random(100) < 30) {',
     'forcejeo_no_dispara_cuerpo_maldito_ni_rencor_ni_cuenta_para_ultima_baza',
     'Cuerpo Maldito anula un hueco por un Forcejeo'),

    ("battle.cpp",
     '      if (def.grudge && slot < BT_MOVES && mv != MOVE_STRUGGLE) {',
     '      if (def.grudge && slot < BT_MOVES) {',
     'forcejeo_no_dispara_cuerpo_maldito_ni_rencor_ni_cuenta_para_ultima_baza',
     'Rencor vacia un hueco por un Forcejeo'),

    ("battle.cpp",
     '  if (slot < 8 && mv != MOVE_STRUGGLE) att.usedMask |= (uint8_t)(1 << slot);',
     '  if (slot < 8) att.usedMask |= (uint8_t)(1 << slot);',
     'forcejeo_no_dispara_cuerpo_maldito_ni_rencor_ni_cuenta_para_ultima_baza',
     'Forcejeo cuenta como hueco usado para Ultima Baza'),

    # Leva 4: mecanicas del Anil (Encore, Bostezo, duraciones).
    ("battle.cpp",
     '        def.encoreTurns = 4;\n',
     '        def.encoreTurns = 3;\n',
     'encore_repite_4_rondas_si_el_rival_aun_no_se_movia',
     'Encore vuelve a 3'),

    ("battle.cpp",
     '      (--c.encoreTurns == 0 ||\n       (c.encoreSlot < BT_MOVES && !c.usesLeft[c.encoreSlot]))) {',
     '      (--c.encoreTurns == 0)) {',
     'encore_se_acaba_al_gastar_los_pp_del_golpe',
     'Encore no se acaba sin PP'),

    ("battle.cpp",
     '            !def.usesLeft[def.lastSlot] ||\n            def.moves[def.lastSlot] >= MOVE_COUNT ||',
     '            def.moves[def.lastSlot] >= MOVE_COUNT ||',
     'encore_falla_como_en_el_anil',
     'Encore entra sobre un golpe sin PP'),

    ("battle.cpp",
     '          if (fxU == FX_ENCORE || fxU == FX_SKETCH || fxU == FX_TRANSFORM ||\n              fxU == FX_COPYCAT) {',
     '          if (fxU == FX_ENCORE) {',
     'encore_falla_como_en_el_anil',
     'lista negra del Encore recortada'),

    ("battle.cpp",
     '    case FX_ENCORE: case FX_TAUNT: case FX_DISABLE: case FX_TORMENT:\n    case FX_ATTRACT: case FX_FORESIGHT:',
     '    case FX_TAUNT: case FX_DISABLE: case FX_TORMENT:\n    case FX_ATTRACT: case FX_FORESIGHT:',
     'encore_atraviesa_el_muneco_y_el_relevo_lo_borra',
     'el muneco para el Encore'),

    ("battle.cpp",
     '        def.yawn = 2;\n',
     '        def.yawn = 1;\n',
     'bostezo_duerme_al_cerrar_la_ronda_SIGUIENTE',
     'Bostezo duerme en la misma ronda'),

    ("battle.cpp",
     '    } else if (m.fx == FX_YAWN) {\n',
     '    } else if (false) {\n',
     'bostezo_duerme_al_cerrar_la_ronda_SIGUIENTE',
     'el sx de Bostezo duerme en el acto'),

    ("battle.cpp",
     '  if (c.yawn && --c.yawn == 0 && !c.fainted() &&\n      c.applyStatus(ST_SLEEP, terrain, weather, uproar > 0))',
     '  if (c.yawn && --c.yawn == 0 && !c.fainted() &&\n      (c.status = ST_SLEEP, true))',
     'bostezo_se_pierde_si_al_final_ya_no_puede_dormir_o_si_releva',
     'Bostezo pisa otro estado'),

    ("battle.cpp",
     '  yo.yawn = 0;            // relevarse tambien libra del Bostezo\n',
     '',
     'bostezo_se_pierde_si_al_final_ya_no_puede_dormir_o_si_releva',
     'relevarse no libra del Bostezo'),

    ("battle.cpp",
     '        def.tauntTurns = 4;\n',
     '        def.tauntTurns = 3;\n',
     'las_duraciones_son_las_del_anil',
     'Mofa vuelve a 3'),

    ("battle.cpp",
     '        def.disableTurns = 5;           // Disable = 5 en el Anil\n',
     '        def.disableTurns = 4;\n',
     'las_duraciones_son_las_del_anil',
     'Disable vuelve a 4'),

    ("battle.cpp",
     '    def.gagTurns = 2;\n',
     '    def.gagTurns = 3;\n',
     'las_duraciones_son_las_del_anil',
     'Golpe Mordaza vuelve a 3'),

    ("battle.cpp",
     '    att.disableTurns = 3;   // Cuerpo Maldito: Disable = 3 en el Anil\n',
     '    att.disableTurns = 4;\n',
     'cuerpo_maldito_anula_3_rondas',
     'Cuerpo Maldito vuelve a 4'),

    ("battle.cpp",
     '          att.lockTurns = 2;            // Uproar = 3 usos, fijo, en el Anil\n',
     '          att.lockTurns = (uint8_t)(1 + random(2));\n',
     'alboroto_son_3_usos_fijos',
     'Alboroto vuelve al azar'),

    ("battle.cpp",
     '    if (confuseTurns && random(3) == 0) return BLK_CONFUSED;  // se golpea\n',
     '    if (random(3) == 0) return BLK_CONFUSED;  // se golpea\n',
     'confusion_dura_2_a_5_y_al_llegar_a_cero_ya_no_se_golpea',
     'la ultima vuelta aun tira el dado'),

    ("battle.cpp",
     '    futureTurns[from] = 3;    // FutureSightCounter = 3: cae al 3er cierre\n',
     '    futureTurns[from] = 2;\n',
     'premonicion_llega_dos_turnos_despues',
     'Premonicion vuelve a 2'),

    ("battle.cpp",
     '        sxTiro = TRI[random(3)];\n',
     '        sxTiro = TRI[0];\n',
     'tri_ataque_reparte_quemadura_hielo_y_paralisis',
     'Tri Ataque solo quema'),

    ("battle.cpp",
     '        sxTiro = GARRA[random(3)];\n',
     '        sxTiro = GARRA[0];\n',
     'garra_funesta_reparte_veneno_paralisis_y_sueno',
     'Garra Funesta solo envenena'),

    ("battle.cpp",
     '  if (r.damage && !r.subHit && mv < MOVE_COUNT && MOVE_TBL[mv].fx == FX_FANG &&\n',
     '  if (false && r.damage && !r.subHit && mv < MOVE_COUNT && MOVE_TBL[mv].fx == FX_FANG &&\n',
     'colmillo_tira_dos_dados_estado_y_amedrentar',
     'los colmillos no amedrentan'),

    ("battle.cpp",
     '        att.applyStage(SG_ATK, +2);\n        att.applyStage(SG_SPA, +2);\n        att.applyStage(SG_SPE, +2);\n',
     '        att.applyStage(SG_ATK, +2);\n        att.applyStage(SG_SPA, +2);\n',
     'rompecoraza_baja_defensas_y_sube_lo_demas',
     'Rompecoraza no sube velocidad'),

    ("battle.cpp",
     '        if (def.applyStage(SG_SPE, -1)) r.stageWhich = SG_SPE;\n        break;\n      case FX_GROWTH:',
     '        break;\n      case FX_GROWTH:',
     'hilo_venenoso_envenena_y_frena',
     'Hilo Venenoso no frena'),

    ("battle.cpp",
     '        if (weather == WE_SUN) {\n          att.applyStage(SG_ATK, +1);\n          att.applyStage(SG_SPA, +1);\n        }\n',
     '',
     'desarrollo_sube_el_doble_con_sol',
     'Desarrollo no sube mas con sol'),

    ("battle.cpp",
     '  if (m.fx == FX_FREEZEDRY) {\n',
     '  if (false) {\n',
     'liofilizacion_es_superefectiva_contra_el_agua_por_tipo',
     'Liofilizacion sin agua'),

    ("battle.cpp",
     '        r.recoil = (uint16_t)((att.hpMax + 1) / 2);   // (totalhp / 2.0).ceil\n',
     '        r.recoil = (uint16_t)(att.hpMax / 2);\n',
     'clorofotazo_cuesta_la_mitad_de_los_ps_maximos_redondeando_arriba',
     'Clorofotazo redondea abajo'),

    ("battle.cpp",
     '      if (strcmp(m.name, "THUNDERWAVE") == 0 &&\n          typeEffect(m.type, def.type1(), def.type2()) == 0) {\n',
     '      if (typeEffect(m.type, def.type1(), def.type2()) == 0) {\n',
     'en_el_anil_el_apoyo_no_mira_la_tabla_de_tipos',
     'el apoyo vuelve a mirar los tipos'),

    ("battle.cpp",
     '      if (strcmp(m.name, "THUNDERWAVE") == 0 &&\n          typeEffect(m.type, def.type1(), def.type2()) == 0) {\n',
     '      if (false) {\n',
     'onda_trueno_es_la_excepcion_y_no_le_entra_a_tierra',
     'Onda Trueno entra en tierra'),

    ("battle.cpp",
     "    const uint16_t BERRY_AT = 2;",
     "    const uint16_t BERRY_AT = 4;",
     "la_baya_se_come_sola_a_MEDIA_vida_y_desaparece",
     "las bayas vuelven a saltar a un cuarto"),

    ("battle.cpp",
     "  if (!over && r.damage && objeto(att).fx == IT_LIFE_ORB &&\n"
     "      att.abil() != AB_MAGIC_GUARD) {",
     "  if (!over && r.damage && objeto(att).fx == IT_LIFE_ORB) {",
     "life_orb_no_se_lo_cobra_a_guardia_magica",
     "el orbe vuelve a cobrarle a Guardia Magica"),

    ("battle.cpp",
     "  if (side[a].abil() == AB_UNBURDEN && side[a].soltoAlgo &&\n"
     "      side[a].item == IT_NONE) sa *= 2;",
     "  if (side[a].abil() == AB_UNBURDEN &&\n"
     "      side[a].item == IT_NONE) sa *= 2;",
     # El que mide la VELOCIDAD. El otro teste de Liviano solo mira la
     # bandera `soltoAlgo`, asi que sobrevive a este sabotaje: por eso
     # no sirve como prueba de este arreglo.
     "unburden_corre_el_doble_SOLO_tras_largar_algo",
     "Liviano vuelve a valer para el que entra vacio"),

    ("gyms.h",
     "  uint8_t baja = (uint8_t)((GYM_TRAINERS - (i < GYM_TRAINERS ? i : 0)) * 2);\n"
     "  return g.level > baja ? (uint8_t)(g.level - baja) : 1;",
     "  (void)i;\n"
     "  return g.level > 2 ? (uint8_t)(g.level - 2) : 1;",
     "los_tres_de_la_casa_pelean_por_debajo_del_lider",
     "los tres de la casa, todos al mismo nivel"),

    ("pet.h",
     "    return !allGymsDone() && gymTrainers >= GYM_TRAINERS;",
     "    return !allGymsDone();",
     "el_porton_son_los_tres_de_la_casa",
     "el porton del gimnasio deja de existir"),

    # ------------------------------------------------- MALDICION
    # Eran DOS golpes metidos en uno y estaba puesto como un
    # SX_RAISE_ATKDEF pelado. Lo pregunto el dueno: "curse em fantasma
    # deveria corta a energia nao?".

    # La mitad de FANTASMA, que no existia: un Gengar se subia ataque y
    # defensa en vez de pagar media vida y maldecir.
    ("battle.cpp",
     "        if (att.type1() == T_GHOST || att.type2() == T_GHOST) {",
     "        if (false) {",
     "un_fantasma_paga_media_vida_y_maldice_al_otro",
     "un fantasma vuelve a subirse los stats en vez de maldecir"),

    # El PRECIO del no-fantasma. Sin el, Maldicion es una Danza Dragon
    # de regalo.
    ("battle.cpp",
     "          bool algo = att.applyStage(SG_SPE, -1);",
     "          bool algo = false;",
     "sin_ser_fantasma_baja_velocidad_y_sube_ataque_y_defensa",
     "Maldicion deja de costar velocidad"),

    # El mordisco de cada turno, que es para lo que sirve maldecir.
    ("battle.cpp",
     "  if (c.cursed && !c.fainted()) {",
     "  if (false) {",
     "el_maldito_pierde_un_cuarto_al_cerrar_cada_turno",
     "la maldicion deja de roer al cerrar el turno"),

    # Y que no se pueda echar dos veces: si no, el fantasma paga media
    # vida otra vez por nada.
    ("battle.cpp",
     "          if (def.cursed || def.fainted()) { r.failed = true; break; }",
     "          if (def.fainted()) { r.failed = true; break; }",
     "dos_veces_al_mismo_falla",
     "maldecir al ya maldito vuelve a cobrar media vida"),

    # ------------------------------------------- la TABLA DE TIPOS
    # La fila entera del ACERO estaba a x1: tools/type_chart.py se
    # escribio a mano y se salto la clave 'steel'. Aqui se sabotea el
    # dex.h generado, que es lo que lee el combate.
    ("dex.h",
     "  { 100, 100, 100, 100, 100, 100, 200, 100, 100,  50,  50, 100,  50, 100, 200, 100, 100,  50, 200 },  // steel",
     "  { 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },  // steel",
     "el_acero_pega_como_en_el_Anil",
     "el acero vuelve a ser x1 contra los dieciocho"),

    # Y el barrido, que es el que habria pillado el fallo sin saber que
    # buscar: ninguna fila puede estar entera a x1.
    ("dex.h",
     "  { 100, 100, 100, 200, 100,   0, 100, 100, 100, 100, 200,  50,  50, 100, 100,  50, 100, 100, 100 },  // electric",
     "  { 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },  // electric",
     "ningun_tipo_pega_x1_contra_TODOS",
     "otra fila entera a x1, a ver si el barrido la ve"),

    # ----------------------------- los que CAMBIAN segun quien los usa
    # Salieron de tools/golpes_que_dependen.py. Ninguno se veia roto:
    # todos hacian algo, y lo que hacian era creible.

    # FANTASMA que se va: dos sitios, uno para huir y otro para relevar.
    # Se sabotean por separado porque son dos puertas distintas.
    ("battle.cpp",
     "  if (!yo.valid() || yo.fainted()) return false;\n"
     "  if (esFantasma(yo)) return true;          // ver esFantasma()",
     "  if (!yo.valid() || yo.fainted()) return false;",
     "un_fantasma_se_va_aunque_lo_tengan_agarrado",
     "el fantasma vuelve a quedarse preso del agarre"),

    ("battle.cpp",
     "    return false;\n"
     "  if (esFantasma(yo)) return true;          // ver esFantasma()",
     "    return false;\n"
     "  if (false) return true;                    // ver esFantasma()",
     "un_fantasma_se_va_aunque_lo_tengan_agarrado",
     "el fantasma vuelve a no poder relevar"),

    # TOXICO de un veneno: con 90 de precision falla uno de cada diez.
    ("battle.cpp",
     "      (att.type1() == T_POISON || att.type2() == T_POISON))\n"
     "    nuncaFalla = true;",
     "      (att.type1() == T_POISON || att.type2() == T_POISON))\n"
     "    nuncaFalla = nuncaFalla;",
     "de_un_veneno_no_falla_NUNCA",
     "el Toxico de un veneno vuelve a poder fallar"),

    # DRENAJE DE FUERZA: la cura sale del ataque DEL OTRO.
    ("battle.cpp",
     "        uint16_t cura = def.atkStatRaw();",
     "        uint16_t cura = 0;",
     "drenaje_de_fuerza_cura_LO_QUE_VALE_el_ataque_del_otro",
     "Drenaje de Fuerza deja de curar y vuelve a ser solo un -1"),

    # ESTRIDENCIA: los cinco +1 vuelven a ser gratis.
    ("battle.cpp",
     "        att.hp = (uint16_t)(att.hp - precio);",
     "        (void)precio;",
     "los_cinco_mas_uno_SE_PAGAN_con_un_tercio",
     "Estridencia deja de cobrar el tercio de vida"),

    # AUTOTOMIA: sin quitar peso, Patada Baja pega igual.
    ("battle.cpp",
     "        if (att.ligero < 255) att.ligero++;",
     "        (void)0;",
     "quitarse_peso_abarata_la_patada_baja",
     "Autotomia deja de quitar peso de verdad"),

    # LIMPIEZA: las trampas se quedan donde estaban.
    ("battle.cpp",
     "      case FX_TIDYUP:\n"
     "        for (uint8_t s = 0; s < 2; s++) {\n"
     "          rocks[s] = spikes[s] = tspikes[s] = web[s] = 0;\n"
     "        }",
     "      case FX_TIDYUP:",
     "tidy_up_barre_las_trampas_de_LOS_DOS_lados",
     "Limpieza deja de barrer las trampas"),

    # PURIFICACION: vuelve a ser un Recuperacion que no limpia a nadie.
    ("battle.cpp",
     "        if (def.status == ST_NONE) { r.failed = true; break; }\n"
     "        def.status = ST_NONE;\n"
     "        def.statusTurns = 0;",
     "        if (false) { r.failed = true; break; }",
     "le_quita_el_estado_AL_OTRO_y_se_cura",
     "Purificacion deja de limpiar al otro"),

    # ------------------------------------------- del barrido de OBJETOS
    # Los 212 estan y solo el Mapa no hace nada (a proposito), pero tres
    # numeros no eran los del Anil.

    # ARANJA: diez PS fijos, no un porcentaje del maximo.
    ("battle.cpp",
     "  if (it.fx == IT_BERRY_FIX) return it.arg;           // PS fijos",
     "  if (it.fx == IT_BERRY_FIX) return (uint16_t)((uint32_t)hpMax * 12 / 100);",
     "la_aranja_cura_DIEZ_PS_y_no_un_porcentaje",
     "la Aranja vuelve a curar un 12% y a escalar con el nivel"),

    # ...y la ZIDRA sigue siendo una fraccion: si no, "arreglar" la
    # Aranja poniendo las dos fijas pasaria por bueno.
    ("battle.cpp",
     "  return (uint16_t)((uint32_t)hpMax * it.arg / 100);  // una fraccion",
     "  return it.arg;",
     "la_zidra_SI_escala_con_la_vida",
     "las dos bayas pasan a curar un numero fijo"),

    # LA BAYA QUE LIMPIA: tambien la confusion, y tambien salta por ella.
    ("battle.cpp",
     "               (c.status != ST_NONE || c.confuseTurns > 0)) {",
     "               (c.status != ST_NONE)) {",
     "la_que_cura_estados_quita_tambien_la_CONFUSION",
     "la baya vuelve a no mirar la confusion"),

    # Y el NOMBRE del objeto, que decia el de otro. Se sabotea el
    # items.h GENERADO y no tools/gen_items.py: el make no vuelve a
    # correr el generador, asi que tocar la fuente no cambiaria el
    # binario y el teste se quedaria verde sin probar nada.
    ("items.h",
     '  { "Focus Sash",         BP_HELD,   IT_FOCUS_SASH, 0, IU_NONE },',
     '  { "Focus Band",         BP_HELD,   IT_FOCUS_SASH, 0, IU_NONE },',
     "la_banda_se_llama_por_lo_que_HACE",
     "vuelve el nombre mal traducido del Anil"),

    # ------------------------------------------- RESPIRO y DESPEJAR
    # Los dos que la varredura marco como medio hechos: curaban o
    # bajaban algo, y se dejaban fuera la mitad que les da nombre.

    # RESPIRO: pierde el VOLADOR mientras dure el turno.
    ("battle.cpp",
     "        att.roosted = true;",
     "        (void)0;",
     "posado_le_ENTRA_el_terremoto_que_volando_no_le_llega",
     "Respiro vuelve a ser media cura sin precio"),

    # ...y lo recupera al cerrarlo, no para siempre.
    ("battle.cpp",
     "  if (who <= 1) side[who].roosted = false;",
     "  if (false) side[who].roosted = false;",
     "al_cerrar_el_turno_vuelve_a_volar",
     "el Respiro se queda puesto para siempre"),

    # Y el volador PURO pasa a normal, no a sin-tipo.
    ("battle.h",
     "    return (b == T_NONE || b == T_FLYING) ? (uint8_t)T_NORMAL : b;",
     "    return b;",
     "un_volador_PURO_se_queda_en_normal",
     "un volador puro se queda SIN tipo en vez de normal"),

    # DESPEJAR: barre las trampas de los dos lados.
    ("battle.cpp",
     "        uint8_t suyo = (uint8_t)(from ^ 1);\n"
     "        for (uint8_t s = 0; s < 2; s++) {\n"
     "          rocks[s] = spikes[s] = tspikes[s] = web[s] = 0;\n"
     "        }",
     "        uint8_t suyo = (uint8_t)(from ^ 1);",
     "barre_las_trampas_de_LOS_DOS_lados",
     "Despejar deja de barrer las trampas"),

    # ...y las pantallas del otro y el terreno.
    ("battle.cpp",
     "        for (uint8_t i = 0; i < PA_COUNT; i++) pantalla[suyo][i] = 0;\n"
     "        espejaPantallas(suyo);\n"
     "        terrain = TR_NONE;\n"
     "        terrainTurns = 0;",
     "        (void)suyo;",
     "tira_las_pantallas_DEL_OTRO_y_el_terreno",
     "Despejar deja las pantallas y el terreno donde estaban"),

    # -------------------- los que estaban FUERA de la tabla
    # Un golpe de apoyo sin efecto modelado lo descarta el generador, asi
    # que estos no existian ni en la tarjeta ni en el learnset. Sabotear
    # el EFECTO los devuelve a ese limbo, y el primer teste lo canta.

    ("battle.cpp",
     "          def.applyStage(SG_DEF, -1, true);\n"
     "          if (def.applyStage(SG_ATK, -1, true)) {",
     "          if (def.applyStage(SG_ATK, -1, true)) {",
     "halago_baja_ataque_Y_defensa",
     "Halago deja de bajar la defensa"),

    ("battle.cpp",
     "        att.status = ST_NONE;\n"
     "        att.statusTurns = 0;\n"
     "        r.sideFx = m.fx;\n"
     "        break;\n"
     "\n"
     "      // RECICLAJE",
     "        r.sideFx = m.fx;\n"
     "        break;\n"
     "\n"
     "      // RECICLAJE",
     "refresco_quita_las_TRES_y_no_el_sueno",
     "Refresco deja de quitar el estado"),

    ("battle.cpp",
     "        if (att.item != IT_NONE || !att.lastBerry) { r.failed = true; break; }\n"
     "        att.item = att.lastBerry;",
     "        if (att.item != IT_NONE || !att.lastBerry) { r.failed = true; break; }\n"
     "        att.item = IT_NONE;",
     "reciclaje_devuelve_la_baya_que_se_comio",
     "Reciclaje no devuelve nada"),

    ("battle.cpp",
     "        uint8_t cual = puede[random(n)];\n"
     "        att.applyStage(cual, +2);",
     "        uint8_t cual = puede[random(n)];\n"
     "        att.applyStage(cual, +1);",
     "acupresion_sube_DOS_a_uno_solo",
     "Acupresion sube UN escalon en vez de dos"),

    ("battle.cpp",
     "        if (def.status != ST_POISON && def.status != ST_BADPOISON) {\n"
     "          r.failed = true;\n"
     "          break;\n"
     "        }",
     "        if (false) { r.failed = true; break; }",
     "lodo_venenoso_solo_le_entra_al_envenenado",
     "Lodo Venenoso le entra a cualquiera"),

    ("battle.cpp",
     "        uint32_t cura = (weather == WE_SAND) ? (uint32_t)att.hpMax * 2 / 3",
     "        uint32_t cura = false ? (uint32_t)att.hpMax * 2 / 3",
     "refugio_cura_MAS_con_tormenta_de_arena",
     "Refugio deja de aprovechar la tormenta de arena"),

    ("battle.cpp",
     "  if (r.effect == 0 && def.mirado && m.type == T_PSYCHIC &&",
     "  if (false && def.mirado && m.type == T_PSYCHIC &&",
     "ojo_de_mente_le_mete_lo_psiquico_al_siniestro",
     "el Ojo de Mente deja de tumbar la inmunidad del siniestro"),
    # --------------------------------------------- RODAR / CORTE FURIA
    # "depois veja a mecanica do golpe Rollout, acho q esta errado".
    # Estaba mal de cinco maneras; cada una lleva su sabotaje.

    # 1. El candado. El comentario decia "obliga a seguir" y no habia
    #    una sola linea que lo hiciera.
    ("battle.cpp",
     "        if (att.lockTurns == 0) {\n"
     "          att.lockTurns = 4;\n"
     "          att.lockSlot = slot;\n"
     "        } else {\n"
     "          --att.lockTurns;\n"
     "        }\n"
     "        break;",
     "        break;",
     "rodar_ata_cuatro_turnos_y_no_cobra_usos",
     "Rodar vuelve a ser todo premio y ningun precio"),

    # 2. El x16 clavado: con la racha en vez del candado, el quinto
    #    turno no bajaba nunca.
    ("battle.cpp",
     "      uint8_t n = (att.lockTurns > 0 && att.lockTurns <= 4)\n"
     "                      ? (uint8_t)(5 - att.lockTurns) : 0;",
     "      uint8_t n = (att.rachaDe == mv) ? att.rachaMove : 0;\n"
     "      if (n > 4) n = 4;",
     "rodar_sube_hasta_x16_y_vuelve_a_empezar",
     "la cadena de Rodar se queda clavada en el x16"),

    # 3. Rizo Defensa.
    ("battle.cpp",
     "      if (att.curled) n++;",
     "      if (false) n++;",
     "el_rizo_defensa_dobla_rodar",
     "Rizo Defensa deja de doblar Rodar"),

    ("battle.cpp",
     "      case FX_CURL:\n"
     "        att.curled = true;\n"
     "        break;",
     "      case FX_CURL:\n"
     "        break;",
     "rizo_defensa_deja_al_bicho_enroscado",
     "el golpe vuelve a ser solo +1 de defensa"),

    # 4. El tope de Corte Furia: 160, no 250.
    ("battle.cpp",
     "      while ((uint32_t)(m.power << tope) < 160 && tope < 8) tope++;",
     "      tope = 4;",
     "corte_furia_se_para_en_los_160",
     "Corte Furia vuelve a doblar hasta el x16"),

    # 5. Fallar rompe la cadena. El bloque del final de useMove no se
    #    alcanza por el camino del fallo de punteria.
    ("battle.cpp",
     "      // conservaba el multiplicador.\n"
     "      rompeCadena(from, mv, r);",
     "      // conservaba el multiplicador.",
     "rodar_fallado_suelta_el_candado",
     "fallar vuelve a dejar al bicho atado al golpe"),
    # 6. Atado es atado: tampoco se releva.
    ("battle.cpp",
     "  if (yo.lockTurns || yo.recharging || yo.chargingMove != 0xFFFF ||\n"
     "      yo.bideTurns)\n"
     "    return false;",
     "  if (false)\n"
     "    return false;",
     "atado_a_un_golpe_no_se_puede_relevar",
     "la cadena de Rodar vuelve a cortarse gratis relevando"),
    # ------------------------------------- TIMES POR REGIAO
    # Chegar em Johto com o time de Kanto pronto atropelava o
    # Falkner. Cada regiao tem o seu time; o banco e compartilhado
    # mas com cadeado em quem passa do teto DAQUELA regiao.
    ("pet.cpp",
     "  if (bankLocked(bank[bankSlot])) return false;",
     "  if (false) return false;",
     "o_cadeado_nao_deixa_sacar_quem_passa_do_teto",
     "o banco volta a soltar qualquer um em qualquer regiao"),
    ("pet.h",
     "    if (regionDone(region)) return LEVEL_CAP;\n"
     "    const Region &g = regionAt(region);",
     "    if (true) return LEVEL_CAP;\n"
     "    const Region &g = regionAt(region);",
     "o_teto_do_banco_NAO_e_o_de_combate",
     "o teto do banco deixa de olhar a regiao e libera tudo"),
    ("pet.h",
     "    for (uint8_t i = 0; i < TEAM_SLOTS; i++) team[i] = teamsIn[r][i];",
     "    (void)0;",
     "viajar_troca_o_time_e_devolve_na_volta",
     "viajar deixa de trazer o time da regiao de destino"),
    ("pet.h",
     "    // estragou nas duas vezes anteriores.\n"
     "    guardaTimeDe(region);",
     "    // estragou nas duas vezes anteriores.",
     "ir_e_voltar_com_REBOOT_nao_mexe_no_equipo_nem_no_banco",
     "sair de uma regiao deixa de gravar a chave dela"),
    ("pet.h",
     "      visitedMask |= (uint16_t)(1u << r);\n"
     "      llegadaARegion();",
     "      visitedMask |= (uint16_t)(1u << r);",
     "chegar_numa_regiao_nova_da_ovo_do_inicial_dela",
     "chegar numa regiao nova nao da mais o ovo do inicial"),
    ("pet.cpp",
     "    if (vazia) {\n"
     "      for (uint8_t i = 0; i < TEAM_SLOTS; i++) teamsIn[regAhora][i] = team[i];",
     "    if (false) {\n"
     "      for (uint8_t i = 0; i < TEAM_SLOTS; i++) teamsIn[regAhora][i] = team[i];",
     "um_save_velho_poe_seu_time_na_regiao_onde_esta",
     "a migracao marca TODAS as regioes como ja pisadas"),
    # O piso do titular: sem ele a regra prende um bicho criado em casa.
    ("pet.h",
     "    if (!isEgg()) {\n"
     "      uint16_t meu = level();\n"
     "      if (meu > teto) teto = meu;\n"
     "    }",
     "    if (false) {\n"
     "      uint16_t meu = level();\n"
     "      if (meu > teto) teto = meu;\n"
     "    }",
     "o_teto_nunca_fica_abaixo_do_titular",
     "o teto volta a prender um bicho criado na propria regiao"),
    # ------------------------------------- RETRATOS (SpriteCollab)
    # O gerador e o leitor foram escritos por mim um depois do outro;
    # ate estes testes a unica evidencia de que concordam era compilar.
    ("sdmon.cpp",
     "      (porVersionEsperada && ver != porVersionEsperada)) {",
     "      (false)) {",
     "un_cartao_da_VERSAO_antiga_se_recusa",
     "um cartao de outra versao volta a ser aceito"),

    ("sdmon.h",
     "    uint8_t k = mapa[estado];",
     "    uint8_t k = 0;",
     "los_estados_NO_son_todos_la_misma_cara",
     "todos os estados voltam a apontar pra mesma cara"),

    ("sdmon.cpp",
     "  uint32_t ranura = (uint32_t)d * 2 + (sh ? 1 : 0);",
     "  uint32_t ranura = (uint32_t)(d + 1) * 2 + (sh ? 1 : 0);",
     "la_linea_leida_es_la_de_ESA_especie",
     "o indice dos retratos se desloca uma casa"),
    # O shiny tem retrato proprio; a versao do formato tem de bater.
    ("sdmon.cpp",
     "  uint32_t ranura = (uint32_t)d * 2 + (sh ? 1 : 0);",
     "  uint32_t ranura = (uint32_t)d * 2;",
     "el_shiny_tiene_su_PROPIO_retrato",
     "o shiny volta a usar o retrato do normal"),
    # A mega de um shiny sai shiny (msNNN.bin).
    ("sdmon.cpp",
     "  return loadFile(0, shiny, fila, 'm');",
     "  return loadFile(0, false, fila, 'm');",
     "la_mega_de_un_shiny_sale_SHINY",
     "a mega do shiny volta a sair da cor normal"),
    # Treinador nunca traz lendario; lendario so pela rolagem propria.
    ("pet.cpp",
     "  int16_t d = pickWildSpecies(hour, true);",
     "  int16_t d = pickWildSpecies(hour);",
     "o_treinador_do_aviso_NUNCA_traz_lendario",
     "o aviso automatico volta a sortear lendario"),
    ("pet.cpp",
     "  if (e.rarity == R_LEGENDARIO) return 0;",
     "  if (e.rarity == R_LEGENDARIO && badges < GYM_COUNT) return 0;",
     "los_legendarios_son_el_remate",
     "os lendarios voltam a entrar no peso comum com as oito"),
    # Os itens-chave de enfeite estao sempre no bolso.
    ("pet.cpp",
     "  if (ITEM_TBL[i].fx == IT_KEY) return true;",
     "  if (false) return true;",
     "solo_se_ve_lo_que_se_tiene_y_en_su_bolsillo",
     "o bolso de itens-chave volta a ficar vazio"),
    # A animacao do Anil confere o nome do golpe no cabecalho.
    ("sdmon.cpp",
     "  if (7u + n + 4 > sz || strlen(nome) != n || memcmp(b + 7, nome, n) != 0)",
     "  if (7u + n + 4 > sz)",
     "a_animacao_do_anil_carrega_e_recusa_a_de_OUTRO_golpe",
     "a animacao de outro golpe volta a ser aceita"),
    # O retrato da mega escolhe a versao (normal/shiny) pelo deslocamento.
    ("sdmon.cpp",
     "  uint32_t off = offs[sh ? 1 : 0];",
     "  uint32_t off = offs[0];",
     "a_MEGA_ARCANINE_tem_cara_propria_e_shiny",
     "a mega shiny volta a mostrar a cara normal"),

    ("sdmon.cpp",
     "      (porVersionEsperada && ver != porVersionEsperada)) {",
     "      (false)) {",
     "un_cartao_da_VERSAO_antiga_se_recusa",
     "um cartao de formato antigo volta a ser aceito"),
    # Viajar nao pode LEVAR o bicho para a outra regiao.
    ("pet.h",
     "      uint8_t hueco = TEAM_SLOTS;\n"
     "      if (team[mainSlot].used()) {\n"
     "        hueco = mainSlot;\n"
     "      } else {\n"
     "        for (uint8_t i = 0; i < TEAM_SLOTS; i++)\n"
     "          if (team[i].used()) { hueco = i; break; }\n"
     "      }",
     "      uint8_t hueco = team[mainSlot].used() ? mainSlot : TEAM_SLOTS;",
     "viajar_NAO_leva_o_bicho_para_a_outra_regiao",
     "o slot vazio volta a deixar o bicho da outra regiao carregado"),
    # O carimbo do time: se a regiao se perder, e ele que manda.
    ("pet.cpp",
     "  if (teamRegion != region) {",
     "  if (false) {",
     "se_a_regiao_se_perde_manda_o_CARIMBO_do_time",
     "a regiao perdida volta a arquivar o time no lugar errado"),

    # E o carimbo de cada chave de regiao.
    ("pet.cpp",
     "  if (buf[0] != r) {",
     "  if (false) {",
     "um_bloco_com_o_carimbo_errado_e_RECUSADO",
     "um bloco com o carimbo de outra regiao volta a ser aceito"),
    ('sdmon.cpp', '  if (b[5] != qual) return falha();          // o arquivo e de OUTRO clima\n', '', 'o_clima_carrega_e_recusa_o_de_OUTRO_clima', 'L16: aceita o arquivo de clima de OUTRO clima'),
    ('battle.cpp', '    if (i < HIT_MAX) r.hitDmg[i] = h.damage;   // (el `r = h` de arriba lo deja a cero)\n', '', 'el_multigolpe_dice_cuanto_pego_cada_impacto', 'L21: el motor no apunta el dano de cada impacto'),
    ('pet.cpp', '        m.ppLeft[k] = lleno;\n', '', 'curar_repone_los_PP_del_equipo_y_de_la_caja', 'L23: la enfermera no repone los PP del equipo'),
    ('pet.cpp', '      if (m.moves[k] && m.ppLeft[k] < movePP(m.moves[k])) return true;\n', '      ;\n', 'curar_repone_los_PP_del_equipo_y_de_la_caja', 'L23: sin PP en el equipo no viene la enfermera'),
    ('pet.cpp', '        if (m.ppLeft[k] < lleno) algo = true;\n', '', 'curar_repone_los_PP_del_equipo_y_de_la_caja', 'L23: el cartel no cuenta a quien solo le faltaban PP'),
    ('pet.cpp', '  return m > AV_TETO_MIN ? AV_TETO_MIN : m;', '  return m;', 'una_semana_rinde_lo_mismo_que_ocho_horas', 'L25: sem teto, uma semana rende uma semana'),
    ('pet.cpp', '    uint32_t ef = r.minutos <= AV_ENTERO_MIN ? r.minutos\n                                             : AV_ENTERO_MIN + (r.minutos - AV_ENTERO_MIN) / 2;', '    uint32_t ef = r.minutos;', 'rinde_cada_vez_menos_con_el_tiempo', 'L25: o rendimento nao cai com o tempo'),
    ('pet.cpp', '  if (avDesde) return;\n', '', 'de_aventura_no_pasa_hambre_ni_descuidos_ni_se_escapa', 'L25: de aventura passa fome'),
    ('pet.cpp', '  if (avDesde) {\n    guardado = false;\n    save();\n    return;\n  }\n', '', 'de_aventura_no_pasa_hambre_ni_descuidos_ni_se_escapa', 'L25: desligado cobra o tempo da fome'),
    ('pet.cpp', '  avDesde = 0;\n  save();\n}\n\nvoid Pet::tick() {', '  save();\n}\n\nvoid Pet::tick() {', 'reparte_titular_equipo_y_caja_y_cobra_una_vez', 'L25: cobra duas vezes'),
    ('pet.cpp', 'p.origen == 1 ? 50 : 25', '100', 'reparte_titular_equipo_y_caja_y_cobra_una_vez', 'L25: time e caixa ganham como o titular'),
    ('pet.cpp', '  if (e > techo - expAhora) e = techo - expAhora;\n', '', 'no_pasa_del_techo_de_nivel_de_la_region', 'L25: passa do teto (chegando nele)'),
    ('pet.cpp', '      r.itens[r.nItens++] = id;', '      r.itens[r.nItens++] = (uint8_t)(id == 1 ? 2 : 1);', 'los_objetos_son_los_que_la_cena_dijo_y_llegan_a_la_mochila', 'L25: o resumo diz outro objeto'),
    ('pet.cpp', '      bag[id]++;\n      r.itens', '      r.itens', 'los_objetos_son_los_que_la_cena_dijo_y_llegan_a_la_mochila', 'L25: o objeto nao chega na mochila'),
    ('pet.cpp', '  prefs.putUInt("avd", avDesde);\n', '', 'sobrevive_a_apagar_y_no_se_abre_con_un_huevo', 'L25: apagar perde a aventura'),
    ('pet.cpp', '  else { bolsillo = BP_HELD; mega = true; }', '  else { return 0; }', 'a_veces_sale_una_piedra_mega', 'L25: nunca sai mega'),
    ('pet.cpp', '  if (!e && minEf) e = 1;      // un minuto a nivel bajo redondea a cero: poco, pero algo\n', '', 'ya_desde_el_primer_minuto_da_algo_y_antes_nada', 'L35: um minuto em nivel baixo arredonda para zero'),
    ('pet.h', '#define AV_DE_LA_CAJA 6 ', '#define AV_DE_LA_CAJA 1 ', 'salen_varios_de_la_caja_y_no_siempre_los_mismos', 'L35: quase nao sai ninguem da caixa'),
    ('pet.h', '#define AV_MIN_MIN 1 ', '#define AV_MIN_MIN 5 ', 'ya_desde_el_primer_minuto_da_algo_y_antes_nada', 'L35: o minimo volta a cinco minutos'),
    ('pet.cpp', '    guardado = false;\n    save();\n    return;\n  }\n  // Y AQUI si:', '    save();\n    return;\n  }\n  // Y AQUI si:', 'la_pausa_no_sobra_para_despues_de_la_aventura', 'L42: a pausa sobra depois da aventura'),
    ('pet.cpp', '  if (prefs.putUInt("svn", seqSave) != sizeof(uint32_t)) {', '  if (prefs.putUInt("svn", seqSave) != sizeof(uint32_t) && false) {', 'a_sentinela_ve_a_nvs_cheia_mesmo_sem_bloco_grande', 'L43: sem sentinela'),
    ('pet.cpp', '  if (algumaChave && !chavesRuins) {', '  if (false) {', 'o_bloco_teams_sai_e_nao_volta', 'L43: o bloco velho nao sai'),
    ('pet.cpp', '  if (algumaChave && !chavesRuins) {', '  if (algumaChave) {', 'com_uma_chave_ruim_o_bloco_teams_fica', 'L43: apaga com chave faltando'),
    ('pet.cpp', '  sdManda = temEstadoSd && estSd.seq > seqNvs;', '  sdManda = false;', 'com_a_nvs_parada_o_sd_traz_a_regiao_de_volta', 'L44: o SD nunca manda'),
    ('pet.cpp', '  sdManda = temEstadoSd && estSd.seq > seqNvs;', '  sdManda = temEstadoSd && estSd.seq >= seqNvs;', 'com_a_nvs_boa_o_sd_nao_manda', 'L44: o SD manda empatado'),
    ('pet.cpp', '    if (prefs.isKey(k)) prefs.remove(k);\n    return;', '    return;', 'as_regioes_vivem_no_sd_e_saem_da_nvs', 'L44: a chave da NVS fica'),
    ('pet.cpp', '            f.read(dados, n) == (int)n && svCrc(dados, n) == h.crc;', '            f.read(dados, n) == (int)n;', 'arquivo_corrompido_e_recusado', 'L44: sem CRC'),
    ('pet.h', '      if (!algum) return false;', '      (void)algum;', 'sem_ler_o_time_nao_se_viaja', 'L44: viaja sem time'),
    ('pet.cpp', '    svApagaTudo();\n    prefs.putBool("init", true);', '    prefs.putBool("init", true);', 'partida_nova_apaga_o_save_do_sd', 'L44: partida nova herda o SD'),
    ('pet.cpp', '  // Sem SD (ou com ele recusando): a NVS, como antes.\n  guarda(k, buf, sizeof(buf));', '  // Sem SD (ou com ele recusando): a NVS, como antes.', 'sem_sd_as_regioes_ficam_na_nvs', 'L44: sem SD nao grava'),
]


# En Windows el binario lleva .exe y "./build/..." no lo resuelve
# subprocess: hay que darle la ruta entera.
BIN = os.path.join(TEST, "build", "tamapoke_tests")
if not os.path.exists(BIN) and os.path.exists(BIN + ".exe"):
    BIN += ".exe"

# Y `make` se llama distinto segun el toolchain.
MAKE = shutil.which("mingw32-make") or shutil.which("make")


def corre(nombre):
    """Corre la bateria filtrada y devuelve (ok, salida)."""
    r = subprocess.run([BIN, nombre], cwd=TEST,
                       capture_output=True, text=True)
    return r.returncode == 0, r.stdout + r.stderr


def construye():
    if not MAKE:
        return False, "no encuentro mingw32-make ni make en el PATH"
    r = subprocess.run([MAKE, "-s", "build/tamapoke_tests"],
                       cwd=TEST, capture_output=True, text=True)
    if r.returncode != 0:
        # el Makefile puede no tener ese objetivo: cae al de siempre
        r = subprocess.run([MAKE, "-s"], cwd=TEST,
                           capture_output=True, text=True)
    return r.returncode == 0, r.stdout + r.stderr


def main():
    fallos = []
    tmp = tempfile.mkdtemp(prefix="sabhold-")
    print("copias en", tmp)
    for fichero, buscar, cambiar, teste, nombre in SABOTAJES:
        ruta = os.path.join(RAIZ, fichero)
        orig = io.open(ruta, encoding="utf-8", errors="surrogateescape").read()
        if orig.count(buscar) != 1:
            fallos.append("%s: el texto a sabotear aparece %d veces"
                          % (nombre, orig.count(buscar)))
            continue
        shutil.copy2(ruta, os.path.join(tmp, os.path.basename(fichero)))
        io.open(ruta, "w", encoding="utf-8", errors="surrogateescape",
                newline="\n").write(orig.replace(buscar, cambiar))
        try:
            okc, salida = construye()
            if not okc:
                fallos.append("%s: no compila sabotado\n%s" % (nombre, salida[-800:]))
                continue
            verde, salida = corre(teste)
            if verde:
                fallos.append("%s: el teste %s SIGUIO VERDE con el fallo puesto"
                              % (nombre, teste))
                print("  ROJO ESPERADO, VERDE OBTENIDO:", nombre)
            else:
                print("  ok  cae %s  (%s)" % (teste, nombre))
        finally:
            io.open(ruta, "w", encoding="utf-8", errors="surrogateescape",
                    newline="\n").write(orig)

    okc, salida = construye()
    if not okc:
        fallos.append("no vuelve a compilar al restaurar\n" + salida[-800:])

    if fallos:
        print("\nFALLOS:")
        for f in fallos:
            print(" -", f)
        sys.exit(1)
    print("\n%d/%d: cada arreglo tiene un teste que cae sin el."
          % (len(SABOTAJES), len(SABOTAJES)))


if __name__ == "__main__":
    main()
