# Pendências do TamaPoke

Lista na **ordem em que o dono pediu**, não na ordem em que é cômodo
fazer. Isso existe porque eu furei a fila uma vez: comecei os retratos
antes de terminar os times por região, que tinham sido pedidos antes.

Regra: terminar uma antes de começar a seguinte. "Terminar" aqui quer
dizer bateria verde **e** sabotagem provando cada correção.

---

## Fechadas (contexto)

- **Sombras** — centro do apoio medido na fileira que toca o chão.
  26/26 na sabotagem.
- **Rodar / Corte Furia** — cinco erros de mecânica mais dois achados no
  caminho (falhar não quebrava a corrente; dava pra fugir do cadeado
  trocando de bicho). 990/990 lógica, 58/58 UI, 9 sabotagens caem.
- **Flare Blitz** — caminho novo `FX_CELDAS_A_MANO` no gerador.
  Conferido decodificando o `fx.bin` como a placa lê.
- **Créditos do SpriteCollab** — `creditos.h`, 132 autores, 0 espécies
  sem crédito, 4,6 KB.
- **Leva 46 (2026-11-09, GRAVADA) — Pokédex com furos.** 9 espécies do
  dono sem ficha. `evolveReserve` não registrava (Combusken), `PARTY` não
  registrava (Hoenn refeito), e capturas com a NVS cheia perderam o dexreg
  (Hoppip, Taillow nas caixas). Conserto: registram os dois + autocura no
  load (`registraOsQueTem`: time, banco, teamsIn). 2 testes, sabotagem cai.
  Placa: boot curou 7, `REG 187 276` os 2 das caixas → 238. As caixas
  FECHADAS não entram na autocura (ler 32 arquivos no boot).
- **Leva 47 (2026-11-13, gravada na PLACA NOVA 44BD8D6151D8 às 20:10 + cartão conferido 100% contra o PC; NÃO gravada na placa do dono ainda).**
  (a) Vuelo/Bote/Excavar/Buceo com o último PP: a descida virava Forcejeo e
  o bicho ficava fora da tela. `!soltando` na regra do PP (battle.cpp).
  (b) GINÁSIOS e MEDALHAS saíram da ficha: botões no mapa da região atual
  (`cardSolo`, `CARD_SEQ` = 0 1 2 3 4 6 8). (c) `/map/` na lista branca
  do PUT (a placa nova ficou sem mapas). Testes + sabotagem nos três.
- **Placa NOVA (2026-11-13)**: MAC 44:bd:8d:61:51:d8, sobe na COM8
  (bootloader COM7), cartão de 8 GB, save novo. A do dono é a da COM6.
- **Leva 48 (2026-11-13) — TROCAS ESP-NOW, no PC.** Tabela de particoes
  nova (`partitions.csv`, app 6,25 MB, NVS no mesmo 0x9000; FQBN
  `PartitionScheme=custom`): o Wi-Fi custa 564 KB e so sobravam 277.
  Gravada na placa nova com save conferido (EQUIPO igual antes/depois).
  `troca.h/.cpp` = protocolo puro (convite, oferta, aceite, diario
  PREPARADO no SD, PRONTO/FEITO; so aplica com o PRONTO do outro).
  `Pet::troca*` = evolui ao chegar (gen_evos marca Trade/TradeItem),
  trava de nivel (acima do teto -> banco), Pokedex, `trcu` na NVS torna
  reaplicar idempotente. `troca_radio.cpp` = ESP-NOW canal 1 so com a
  tela aberta; no PC, placa fantasma. Tela em `troca_ui.ino`, botao
  TROCAR na pagina do time. 10 testes (400 sorteios de ar ruim +
  reinicio), 7 sabotagens caem. Firmware 3,44 MB.
  FALTA: teste real entre as duas placas; Usar/Segurar dos itens de troca;
  Karrablast/Shelmet (TradeSpecies).
  2026-11-14: gravada tambem na placa do dono (COM6) com tabela nova,
  NVS 2x + tabela velha em `_claude_scratch/nvsdump/*antes48*`; save igual.
  As placas se acharam na primeira tentativa.
- **Leva 49 (2026-11-14) — as duas queixas do primeiro teste real.**
  (1) TROCAR da pagina do time "6 tentativas": era 140x30 em y=350 e abaixo
  de y=352 a ficha FECHAVA. O mapa de toques nao via porque `trocaOpen` nao
  estava na huella (abrir a troca = "fechar"). Agora 200x40, toque +12 px,
  fechar so 18 px abaixo, "tap: back" desta pagina em 398.
  (2) A tela escolhia sozinha o 1o do time e "tocar na linha passa ao
  proximo" nao estava escrito em lugar nenhum; banco fora. Agora abre uma
  lista TIME|BANCO ao parear (nada oferecido sem o jogador); do banco,
  quem chega fica no mesmo hueco da caixa (diario guarda a caixa em `rsv`,
  `TROCA_BANCO|i`). Acima do teto nunca entra no time nem como ultimo
  recurso. Testes: 2 de Pet + 2 de tela + 3 telas no mapa de toques;
  5 sabotagens caem. Firmware 3.444.192 bytes. Gravada na placa do
  dono (COM6) em 2026-11-14, save igual antes/depois (NVS 2x em
  `nvsdump/backup_antes49_*`). Placa nova (COM8) gravada no mesmo dia, save
  igual (`nvsdump/placa2_antes49_*`). Troca real entre as duas: FUNCIONOU.
- **Leva 57 (2026-11-15) — CONECTAR, regras e escolha do time.** O botao
  da pagina do time virou CONECTAR (escolha do dono; EN CONNECT). Tocar
  numa placa pergunta TROCAR ou BATALHAR (a troca ganhou esse toque).
  Regras de quem convida (1v1/3v3/6v6 — apagado se o time nao chega —;
  itens NENHUM/ATE 3/LIVRES); o convidado ve as regras e o SIM apaga se
  o time dele e pequeno. Escolha na ordem (o 1 abre), 6v6 ja marcado;
  espera e "VS" provisorio. O titular vai por `Pet::fotoTitular()`
  (const: storeActive agora usa ela depois do sync). Fantasma do PC joga
  Duelo e volta a lista quando acaba. 3 testes de tela + 4 telas no mapa
  de toques; sabotagem (titular pelo hueco velho) cai. Tela 148/149
  (globo). NAO gravado. Decidido: **60 s por escolha**, contador nos
  ultimos 10 s, ao acabar a placa escolhe pelo jogador.
  Proximas levas (o combate de verdade):
  - 58: modo PvP no battle_ui SEM tocar no save (todos os pontos que
    gravam: btSavePP, btStoreActive, btFromSlot/btSwitchTo -> pvpEntra,
    btFoeDefeated/EXP, loot, registerMega, takeItem, spendItem/battleSave
    da bolsa, battleClose, captura/fuga fora). Teste: combate inteiro
    pela tela com o hash do save igual.
  - 59: jogada do outro lado pelo radio (no lugar do pickFoeMove), troca
    e item DO RIVAL (hoje a maquina nunca troca), espera + 60 s, relevo
    obrigatorio de um lado so (o outro manda NADA), arrastado/querTrocar.
  - 60: a convidada desenha espelhada (troca os lados so no desenho).
  - 61: placar V/D, fantasma jogando o combate no PC, duas placas reais.
  Desenho decidido (para nao re-pensar): bt.b SEMPRE canonico (anfitriao
  = lado 0) nas duas placas, e a tela de combate continua a passo a passo
  de hoje (btLaunch/btResolve/BP_MSG/endTurn por lado) —— a ordem dela e
  a mesma nas duas placas, entao o dado (btRand semeado) sai igual.
  Simular espelhado ou deixar o motor simetrico: NAO (o motor e a tela
  tem `for s=0..1` e BT_PLAYER que decidem a ordem do dado). PvP so muda:
  (a) a jogada do lado 1 vem de `pvpQuer[1]` e nao do pickFoeMove;
  (b) um turno vira uma FILA: trocas e itens antes (por velocidade),
  depois os golpes por movesFirst; no lugar do `secondPending` a fila
  roda a proxima acao; (c) desmaio/U-turn/Baton = ponto de decisao so do
  lado que repoe (o outro NADA); Roar/Dragon Tail = pvpSorteia na hora;
  (d) nada de pet.*: relevo por pvpEntra, sem EXP/loot/registerMega/
  takeItem/spendItem/afterBattle; (e) a convidada desenha trocando os
  lados SO no desenho (swap antes do render, swap de volta) e o sprite
  de cada lado vai para pmd/foePmd conforme quem e "eu".
- **Leva 58 (2026-11-15) — modo PvP no battle_ui, sem tocar no save.**
  `battleStartPvp(r0, r1, n, eu, semente, regras)`; pontos de decisao
  PT_TURNO/PT_REPOE -> `pvpDecide`; fila `pvpFila` (megas e trocas por
  velocidade, golpes por movesFirst na hora); desmaio -> `pvpDesmaio`
  (repoe so quem caiu; os dois se cairam os dois — `btCaidoAgora`);
  U-turn/Baton -> `pvpMeioDoTurno`; fecha por `pvpFecha` (dado proprio
  desligado). Ganchos fora do save: btSavePP, btStoreActive, btCanSend,
  btSyncReserves, btSwitchTo, btFoeDefeated, battleClose, takeItem,
  registerMega; sem EXP, bolsa, captura. 4 testes pvp_ui (3v3 inteiro pela
  tela com hash do save igual — 33 turnos, 4 quedas, 8 U-turn/Baton, 7
  trocas, sono/toxico/confusao vistos —; mesmo combate com outro ritmo e
  ruido = mesmos hashes; desistir; velocidade e prioridade). 6 sabotagens
  caem. Bateria: logica 1171/1171, tela 152/153 (globo), ubtrap 1171,
  simulador 0 erros, auditoria/contraste/sabotaje (19) ok, firmware
  3.562.519 bytes, RAM 130.852. NAO gravado.
  Licao: o sab58.py restaurou com copy2 e o sketch.cpp gerado com a
  sabotagem ficou "mais novo" — a bateria testou o codigo sabotado (2 PvP
  falsos). Agora sab58.py e tools/sabotaje.py fazem os.utime ao restaurar.
  Uma vez a bateria pendurou nos testes de logica (3 h de CPU); 7 rodadas
  depois, iguais e em 23–41 s: nao reproduz. Se voltar, cacar.
  Decidido pelo dono: **no PvP ninguem desobedece** (`strictOrders = true`
  no battleStartPvp; o vinculo segue valendo em Retribucion/Frustracion).
  O 3v3 de teste (vinculo 0 nos dois lados) conta "won't listen"/"ignores
  you"/"instead!" = 0; sabotagem (false) cai com 1005. Tela 152/153.
- **Leva 63 (2026-11-17) — animacoes como no Anil + botoes da bolsa. GRAVADA na placa do DONO em 2026-11-18 (a nova tambem, 2026-11-18).**
  (a) Explosion "ficava branco antes de explodir": a animacao do golpe
  seguinte era lida durante a frase e herdava o relogio da anterior (o FIM
  dela tocava antes). Vale para qualquer golpe. (b) Golpe em si mesmo
  (Iron Defense, Shell Smash...) e sem alvo (Rain Dance...): o alvo vem do
  `Target` do PBS (pbFindTargets); a cel 1 vai para o proprio usuario ou
  some. Byte `livre` no TPAN; 217 arquivos mudaram. Limite: Curse de tipo
  Fantasma anima sempre no usuario. (c) Botoes USE/HOLD | DAR da ficha do
  objeto: 30 -> 44 px, toque com folga em volta, "voltar" abaixo da ficha.
  (d) IMPACTO e CARGA: o impacto N toca "X hit N" (hitNum = i) e o turno de
  carga toca "Fly charging"/"Common:Dig charging" (hitNum 1); o bicho sai
  ja sumido, sem a nossa transicao por cima. 74 das 107 "hit N" do Anil
  sao copia do hit 1 e nao se gravam: ficam 33 variantes por lado
  (p_/f_<GOLPE>.<k>.bin), e SdAnim nao rele o SD quando a variante nao
  existe. Testes medem a tela (x medio da mao do Double Slap; caixa do
  jogador no Fly); 4 sabotagens caem. Dois testes velhos passavam por
  sorte e foram endurecidos (barra do Double Kick com 999 PS; janela de
  1 s do respirar; Play Rough do rival errava pelo sorteio). Achados no
  caminho: `bt.animFim` nao se zerava ao abrir combate (com a carga ele
  passou a segurar tudo; nos testes congelava 94 deles); um `k` pisado no
  pack_anim gravou PLAYROUGH com o nome "PLAYROUGH.21" (o firmware
  recusava) —— a auditoria agora confere o nome de cada anim/*.bin.
  Bateria 18/11: 1183/1183, tela 170/171 (so o globo de sempre), ubtrap
  1183, 19/19 sabotagens, firmware 3.571.635 B.
  Gravar: firmware + `send_sd --dir anim --diff`.
- leva 64 (GRAVADA com a 63 na placa do DONO em 2026-11-18; a nova tambem: COM8, NVS 2x em nvsdump/placa2_antes65_*, SD anim 1532=1532): conferido no codigo do Anil
  (TwoTurnMove, pbQuickChargingMove, MoveEffects_MultiHit). (a) Solar Beam/
  Blade ao sol: frase "took in sunlight!" + animacao de carga (.1) e o golpe
  no mesmo turno (fase nova BP_CARGA; barra so cai com o golpe). (b) Solar
  Beam/Blade pegam a METADE com chuva/areia/granizo. (c) Electro Shot com
  chuva sai num turno so. (d) Skull Bash/Meteor Beam/Electro Shot sobem
  Def/SpA NO TURNO DE CARGA (o golpe ja sai com a subida; fica mesmo se
  errar), nao mais como secundario depois do dano. 5 testes de logica + 1
  de tela; 7/7 sabotagens. Bateria: 1188/1188, tela 171/172 (globo),
  ubtrap 1188, 19/19, firmware 3.572.719 B.
  Dono (COM6): NVS 2x em nvsdump/backup_antes64_*, tabela conferida,
  EQUIPO/REGIONES iguais antes/depois (o Swampert de Hoenn fugiu ANTES,
  por descuido do user; confirmado por ele). SD: anim --diff, 1532 = 1532.
- leva 65 (GRAVADA na placa do DONO em 2026-11-18, NVS 2x em nvsdump/backup_antes65_*, EQUIPO/REGIONES iguais; a nova tambem: COM8, NVS 2x em nvsdump/placa2_antes65_*, SD anim 1532=1532): Magnitude como no Anil (pbOnStartUse). O nivel
  4..10 sai em useMove antes de acertar/Protect (HitResult.magnitud) e a
  tela diz "Magnitude N!" entre o anuncio e o golpe (via BP_CARGA), mesmo
  errando; o dano usa esse nivel. Chances 5/10/20/30/20/10/5 iguais. E
  ALEM do pedido: Grassy Terrain corta Earthquake/Magnitude/Bulldoze pela
  metade. 3 testes de logica + 1 de tela; 7/7 sabotagens (sab65.py).
  Bateria: 1191/1191, tela 172/173 (globo), ubtrap 1191, 19/19,
  firmware 3.572.987 B. Duelo entre placas: as DUAS precisam da 65 (o
  sorteio mudou de lugar na ordem dos aleatorios).
- levas 66-69 (66-69 GRAVADAS NA COM6 em 2026-11-19, backup_antes69_b.bin; COM8 FALTA). Bateria: logica 1197/1197, tela
  177/178 (so o globo), ubtrap 1197, sabotaje 19/19, firmware 3.576.399 B.
  (66) FUGA pede confirmacao: botao de escapada abre pergunta (choiceKind 3),
  FICAR em cima / DEIXAR IR embaixo; 600 ms surdos em TODA pergunta (o botao
  y176-234 pisa o 1o da pergunta y206-258: toque duplo confirmava). O bicho
  abandonado CHAMA (grito) no 1o minuto e a cada 15 (pet.tomaAvisoAbandono).
  O Swampert do dono fugiu assim (placa ligada horas). Sabotagens 7/7 + 1.
  (67) MEGA na aventura: sem /av/ saia do tamanho da tabela da forma normal
  num quadro esticado (M.Blaziken 122 px < Blaziken 128). Agora normal na cena
  x (tinta mega / tinta normal no desenho do jogo), teto 190. 82/85 maiores;
  as 41 com /av/ seguem a arte delas (M.Scrafty 88%). Teste + sabotagem.
  Doce da comida: CASTELIACONE (escolha do dono) no lugar do Rare Candy.
  (68) QUIZ: QZ_WHO "Quem e esse Pokemon?" (thumb 2x, colorido); com 4 de 4
  a 5a e sempre QZ_WHO em silhueta preta (revela ao responder). Premios:
  2 Super, 3 Hyper, 4 Max Potion, 5 = Rare Candy x2 ou 1/20 Shiny Potion.
  Itens NOVOS no FIM da tabela (228 RARECANDY +1 nivel, 229 SHINYZADOR =
  'Shiny Potion': deixa o titular shiny; no Anil a Pocion Brillante faz o
  proximo selvagem shiny -- mudado a pedido do dono). Fora da Pickup e do
  botin (soDoQuiz). Sabotagens 9/9.
  (69) Escolha na troca e no duelo em GRADE 3x2 (icone 2x, 80 px; 6 por pagina
  tambem no banco; setas/VOLTAR desceram para y352). Evolucao/chegada da troca
  a 3x (120 px). Teste mede a altura da tinta; sabotagens 4/4 (sab69.py).
  TESTE SUJO (nao e da 69): anim.cada_impacto_toca_a_SUA_animacao (#100) falha
  sempre que combate_ui.cada_bola_entra_desde_el_lado (#64) roda antes (TP_DESDE=64
  TP_ATE=100); a versao pre-69 falha igual. Achar o estado que o #64 deixa.
- leva 77 (GRAVADA nas duas placas em 2026-11-23; DONO e NOVA leram iguais antes/depois, so seq +1; caixas intactas): NOMES DOS TIPOS por idioma. tools/gen_type_names.py -> tools/type_names.py (PokeAPI; PT a mao com os nomes oficiais BR: LUTADOR, VENENOSO, DRAGÃO, AÇO...); gen_dex.py emite TYPE_NAME_L[LANG_COUNT][T_COUNT] e typeName() do idioma; EN fica com os rotulos curtos (FIGHT/ELEC/PSY). tools/cp437_up.py: maiusculas mantem ÉÇÄÖÜÑÃÕ (a fonte tem), resto perde acento; golpes regenerados com isso (~270 mudaram, sem repetidos). Chip do golpe no combate pela largura (textW), chip do saco do ginasio cresce (JA FAIRY = 80 px). Provas: logica 1212, ui 216/217 (so o globo), sab77 3/3 (uma pegou teste tautologico, corrigido p/ comparar com a tabela), firmware 3.720.531 B. NAO PROVADO: JA/KO na tela; o crescimento do chip so por formula.
- leva 78 (NAO GRAVADA ainda; espera OK da previa leva78_itens.png): NOMES E DESCRICOES DE ITENS E HABILIDADES nos 8 idiomas. tools/gen_textos_itens.py -> item_names.h/abil_names.h (nomes oficiais PokeAPI + PBS do Anil p/ ES; PT a mao: Poção, Restos, Intimidação; descricoes traduzidas do TEXTO DO ANIL por agentes, tools/textos_itens/tr_*.json; ITEM/ABIL_NAME_SIG = ingles linha a linha). itemName/abilName/itemDesc/abilDesc em todas as telas (mochila, ficha, cartao, combate, captura, aventura, troca, quiz, medalha); serial segue com ids. drawWrapped mede por largura e nao parte letra UTF-8 (latino quebra igual ao de antes, provado). Cuadro da habilidade PAGINA (24 col, 3 linhas); ficha do objeto 23 col; pe 'toque: voltar' da ficha a talla 1 (era cortado); mochila A-Z pelo nome do idioma. PT: tipos dentro das descricoes com o nome do chip (FOGO, nao FIRE) em itens/golpes/dex. Golpes em PT seguem com nome ingles (decisao, oferecida a troca). Provas: logica 1212, ui 224/225 (so o globo, antigo), sab78 8/8 (2 escaparam na 1a rodada: teste tautologico + japones sem espaco; reforcados), sab73 21/21 (ancora atualizada p/ itemName), firmware 4.081.619 B. NAO PROVADO: JA/KO na tela (simulador sem a fonte).
- PENDENTE: reacao.el_globo_ensena_la_onomatopeya_de_lo_que_pide falha so dentro da bateria (coracao fica aceso; algum teste anterior deixa a cerimonia/estado); passa sozinho. Ja falhava na leva 77.
- leva 76 (GRAVADA 2026-11-22 no DONO COM6 e na NOVA COM8, saves e CAJAS conferidos; no DONO 3 do time subiram +1 nivel entre a leitura de antes e a de depois e o seq andou 2 -- so ganho, causa NAO achada lendo o codigo (offline e dia-de-cuidado nao dao isso); perguntei ao dono se estava em aventura/batalha; na NOVA nao se repetiu): NOMES OFICIAIS DOS GOLPES por idioma (PokeAPI, tools/gen_move_names.py -> move_names.h; cache em tools/downloads/pokeapi_moves). MOVE_TBL[].name segue sendo o ID (animacao, som, motor); a tela usa moveName(). PT = ingles (a PokeAPI nao tem pt). Assinatura MOVE_NAME_ID linha a linha; nenhum nome repetido (o DESLIZAMIENTO saiu FRANA = Rock Slide na 1a versao: e de GELO, 'esqui'). Golpes do Anil com nome a mao a partir do PBS. printNaLargura() encolhe/corta com '..'; bt.msg 96 -> 128. QUIZ: passa sozinho (1,5 s acerto / 2,5 s erro; o premio espera toque) e mostra os nomes DO IDIOMA (Pokemon e golpe; antes eram os de base). Provas: logica 1212, ui 214/215 (so o globo), sab76 7/7. Limites: 74 nomes JA e 1 ES cortados no espaco 2x2 da ficha; JA/KO sem prova na tela. Ideia seguinte: traduzir os TIPOS (FIRE...) que agora destoam no quiz.
- leva 75 (GRAVADA 2026-11-22 na NOVA COM8 e no DONO COM6, saves e CAJAS conferidos antes/depois): MENSAGENS DE COMBATE NOS 8 IDIOMAS. 194 frases (combate, captura, troca; catalogo e traducoes em _claude_scratch/l75, valida75.py) com o texto oficial dos jogos onde existe. Tabelas de frase (carga, clima, terreno, atributos) viraram tabelas de StrId (T() na hora, senao ficavam no 1o idioma). bt.msg 44 -> 96 bytes; a caixa do combate parte pela LARGURA (btParteLinhas), nao por bytes. NIDORAN: simbolo femea/macho (CP437 14/13 octal) nas latinas via dexName (gen_dex.py); JA/KO seguem F/M e am/su (as fontes U8g2 nao tem, medido com _claude_scratch/l75/glifos_u8g2.py). CORRIGIDO: o estado que entra no que PEGA (Poison Point/Static/Flame Body do outro, Rest, Gulp Missile) era anunciado com o nome do alvo -> HitResult.statusSelf. Testes que vazavam o idioma (400 toques, 86 guioes) agora devolvem. Provas: logica 1209, ui 210/211 (so o globo), sab75 8/8, sab_nid 3/3, sab_lado 3/3. NAO PROVADO: JA/KO na tela de verdade. Proxima: leva 76 = nomes oficiais dos GOLPES por idioma (PokeAPI); os Pokemon ja sao (gen_names.py).
- leva 74 (GRAVADA 2026-11-22 na NOVA COM8 e no DONO COM6, saves e CAJAS conferidos antes/depois): O MANUAL NO JOGO. Repo PRIVADO github.com/foulusama37/TamaPoke (publicar quando acabarem as correcoes; ate la o QR so abre logado). Botao MANUAL no topo dos ajustes -> QR (LEIAME em PT, README nos outros); qualquer toque/deslize fecha so o QR. Carvalho ganhou a 5a fala (onde fica o manual). QR gerado por tools/gen_qr_manual.py (qr_manual.h; trocar o endereco = rodar de novo); tools/confere_qr.py le a tela com o OpenCV. Provas: manual74 3/3, sab74 11/11, confere_qr 4/4, logica 1209, ui 201/203 (globo e cada_impacto, velhas). NAO PROVADO: ler o QR com um celular de verdade na tela AMOLED; o texto JA/KO da 5a fala (so estimativa). A leva das ~190 mensagens de batalha + descricoes dos objetos + rodape 'toque: ..' da ficha do objeto passa a ser a 75.
- leva 73 (GRAVADA 2026-11-21 no DONO COM6 e na NOVA COM8, so o app; saves conferidos antes/depois): TELAS TRADUZIDAS + MANUAL. ~60 textos fixos em ingles (desligar/pausar, curados, mapa/revanche, ficha, mochila e abas, despedida/ovo, batalha: equipe/bolsa/vitoria, aventura) viraram T(S_...) nos 8 idiomas. MUDANCA ALEM DO PEDIDO: enum StrId passou de uint8_t a uint16_t (ja sao >255 textos). Manual em docs/manual (LEIAME.md pt-BR, README.md EN, 58 fotos por idioma; refazer: TAMA_MANUAL=pt|en ./build/tamapoke_uitests manual + python tools/manual_fotos.py). Provas: logica 1209/1209, ui 198/200 (so globo e cada_impacto, velhas), telas73 8/8 nos 8 idiomas, sab73 21/21, sabotaje 19, sab72 17/17. NAO PROVADO NO PC: largura real do japones/coreano (estimativa largCJK73) e a tela PAUSADO/TCHAU. Fica para a leva 75: ~190 mensagens de batalha ("X usou Y!") e as descricoes dos objetos (items.h, ainda em ingles); rodape cortado "toque: .." na ficha do objeto.
- leva 72 (GRAVADA 2026-11-21 na NOVA COM8 e no DONO COM6, so o app; saves conferidos antes/depois): POKEDEX VISTOS + CACADA.
  Visto (NVS "dexvis") = todo rival de batalha (btSyncFoeSprite: selvagem, treinador, lider, PvP tambem; Ilusao marca o disfarce)
  e o selvagem da aventura (AV_LUTA, SEM TESTE desse gancho). Saves antigos comecam sem vistos.
  Grade: colorido/cinza-azulado (DEX_VISTO)/escuro + anel laranja no cacado. Ficha do visto: bicho a cores, tipos, choro,
  sem texto da dex; botao CACAR/PARAR (S_DEX_CACAR/PARAR/CACANDO/NAO_SAI, 8 linguas).
  Cacada (NVS "cacad"): 1 de cada 4 buscas no mato (CACA_CHANCE), so se ele sai ali; lendario 1 de cada 10 (CACA_CHANCE_LEG)
  e so com lendarioPodeSair. Acaba ao capturar o cacado (equipe ou banco; cheio = continua). Nao mexe na aventura.
  LENDARIOS (bug antigo: so 6 de 103 tinham chance): agora 1 sorteio de 3% (LEG_PM_TOTAL=30) por busca, dividido entre
  os elegiveis (8 insignias da regiao atual; regioes atual e anteriores; regiao atual peso 4). AVISO: ~4x a taxa velha;
  em 11-02 o dono reclamou de lendario demais a ~12%. Se reclamar, baixar LEG_PM_TOTAL.
  Bateria: logica 1209/1209, UI 189/191 (globo e cada_impacto, antigas), ubtrap 1209, auditoria/contraste ok,
  sabotaje 19/19, sab72 17/17, firmware 3.585.643 B / RAM 133.292. cobertura: AB_GLUTTONY fora das colunas (ja na 71).
- leva 71 (GRAVADA 2026-11-21 no DONO COM6 e na NOVA COM8, /bg 128/128 nas duas; DONO: Rotom nv51 no banco = capturado pelo dono, confirmado): FUNDOS E CARTAZES.
  86 fundos novos (ids 42..127, tools/fondos_extra/, pack_bg.py FONDOS_EXTRA) tirados dos outros fangames do PC,
  repartidos nos BgSet de bgs.h (campo, praia, floresta, vulcao, montanha, neve, noite, caverna, cidade).
  Letra de titulo nova (font_titulo.h, gen_font_titulo.py --px 12, power clear bold) + janela/botoes novos em:
  evolucao (miniaturas + seta), despedida e fuga (alem do pedido), medalha (medalha desenhada), insignia
  (titulo, lider e linha da MT agora traduzidos: S_BADGE_NEW/ZCRYSTAL/FROM/TM). Nome da insignia e do golpe seguem em ingles.
  Bateria 2026-11-21: logica 1201/1201, UI 184/186 (globo e cada_impacto, antigas), ubtrap 1201, sabotaje 19/19,
  sab71 7/7, firmware 3.583.647 B / RAM 133.164. Teste carteles71 falhava so no lote: evolveUntil de teste anterior
  (millis volta a 0) -> o helper espera a evolucao acabar. tools/verifica_sabotaje.py quebra (entrada com malo vazio
  + ancoras velhas random->btRand em sabotaje_held): consertar.
- leva 70 (GRAVADA no DONO COM6 2026-11-20 + 30 sprites no SD das duas; placa NOVA COM8 tambem gravada 2026-11-20; Eiscue shiny corrigido em tools/correcciones/): FORMAS DE COMBATE. formas_combate.h (gen_formas_combate.py,
  15 formas do PBS do Anil: Castform 1-3, Cherrim, Darmanitan Daruma e Daruma de Galar,
  Aegislash Filo, Wishiwashi Banco, Minior nucleo, Mimikyu, Eiscue, Cramorant 1-2, Morpeko,
  Zygarde Completo). FORA de FORM_TBL (save/Pokedex/evolucao). Sprites /mons/c<dex>_<forma>.bin
  e cs... (pack_anil.py combate), 30 ficheiros NOVOS a mandar ao SD. Regras do Anil
  (Battler_ChangeSelf): so a especie muda; Aegislash so volta com Escudo Real; Cramorant so
  Surf/Buceo e escupe com qualquer golpe (1: -1 def, 2: paralisa); Zen Mode e Ice Face
  ganharam mecanica (AB_ZEN_MODE/AB_ICE_FACE no FIM do enum). ILUSAO: era a forma 1 = Zorua
  de HISUI; agora a cara do ultimo do equipo que luta (ilusDex), treinador do caminho
  sorteia o ultimo de antemao (bt.foeUltimo). Conserto vizinho: btSyncMySprite punha
  monFor=-2 e o ensureMon apagava o Ditto transformado do jogador no quadro seguinte.
  Bateria: logica 1201/1201, tela 180/182 (globo + cada_impacto, ja antes), sab70 18/19
  (a que escapou e impossivel por construcao), firmware 3.578.423 B.
  PENDENTE: OK do dono nas previas (formas_combate_previa.png, eiscue_shiny_proposta.png:
  o shiny da cara quebrada do proprio Anil vem COM o cubo). Tamanho do Banco no combate
  (altoVisto) sem teste. Desvio aceito: Disfarce/Cara de Hielo rearmam ao reentrar.
- **Leva 61 (2026-11-15) — placar V/D/E + gravada na placa do DONO.**
  `pet.duV/duD/duE` (NVS `duv`/`dud`/`due`), `Pet::contaDuelo` grava. Conta
  uma vez por combate que COMECOU: o resultado (DM_FIM), desistir (minha
  bandeira = derrota, a outra desistiu = vitoria, fechar a tela no meio =
  derrota); sumir/dessincronia NAO contam. Linha "vitorias N derrotas N
  empates N" (8 idiomas, S_DU_PLACAR) na tela do fim e no "trocar ou
  batalhar?". Testes: logica (o placar sobrevive ao reinicio) + 3 de rede
  (3v3 soma 1 so e nao segue somando; desistir = derrota; queda nao conta).
  4 sabotagens caem (save, contar 2x, queda conta, desistir nao conta).
  Logica 1172/1172 (i18n: S_DU_PLACAR registrado na tabela de formatos),
  tela 165/166 (globo). Firmware 3.570.608 bytes. **Gravada na COM6** com
  NVS 2x em `nvsdump/backup_antes61_*` + tabela conferida; EQUIPO/REGIONES
  iguais antes/depois (Hoenn, Blaziken/Swampert/Donphan/Ludicolo/
  Manectric/Whiscash). **Placa nova (COM8) gravada tambem** (NVS 2x em
  nvsdump/placa2_antes61_*, EQUIPO igual: Kanto, Squirtle/Scizor/Bellsprout/
  Dratini). O 1o toque de 1200 so reiniciou a placa nova (bootloader nao
  veio em 25 s, nada gravado); o 2o toque levou a COM7 em 0,6 s.
  **2026-11-15: teste real entre as duas placas FUNCIONOU** (user: "funcionou,
  sensacional").
  Licoes: compilar com `-e` (sem ele o .bin em build/ fica o velho; o
  grava61.py recusa .bin mais velho que as fontes); o logica pendurado da
  rodada b22mdomtd (PID vivo por horas) travava o link -> matar antes.
- **Leva 60 (2026-11-15) — a convidada desenha espelhado.** O combate
  segue canonico (anfitriao = lado 0 nas duas placas); so o DESENHO troca:
  `btAbajo()`/`btArriba()` (battle_ui.ino) dizem que lado vai embaixo
  (`pvpEu` no PvP, BT_PLAYER fora). Passaram a perguntar a eles: barras de
  PS, os dois sprites (btSyncMySprite/btSyncFoeSprite, pvpTrocaAgora,
  arranque), a carta de golpes e o toque nela, estocada/sacudida, mega
  (sprite, frase, branco, botao), desmaio/afundar, entrada da bola,
  Excavar/Vuelo, o lado do efeito e o "rival" das animacoes do Anil. Os
  arrays de tela (btPedido, gAnimSpr) continuam por posicao. A fantasma do
  PC agora CONVIDA (`radioFantasmaConvida`, e `convida <regras>` no guion
  do simulador). Testes: pvp_ui a convidada (barra medida em pixels: o meu
  a 10% = vermelho embaixo; sprite de cima; a carta decide pelo lado 1) e
  pvp_rede como convidada (3v3 inteiro pela tela, a cada vez minha confere
  sprite e barra, save intacto). 3 sabotagens caem (sab60.py). NAO gravado.
  Previa: digisprites/batalha_placas_leva60_convidada.png.
  Licao: `sed -i` do Git Bash converteu o test_ui.cpp (CRLF) para LF.
- **Leva 59b (2026-11-15) — a BOLSA entre placas.** Copia da minha bolsa
  (`pvpBolsa`) tirada ao comecar: gastar nao mexe no save. So o bolso de
  cura/X (sem bolas). Regra do convite: NENHUM esconde o botao; ATE 3 conta
  `itensUsados` (cabecalho "N of 3 left", botao apagado depois da 3a);
  LIVRES = o que a copia tiver. Pocao/Revive escolhem o alvo na lista do
  time (acende so quem o objeto ajudaria; o Revive levanta um CAIDO do
  banco a metade e ele volta a poder entrar); X so no da pista. Vira a
  jogada DD_ITEM (arg=objeto, extra=indice), vai pela rede, e as duas
  placas aplicam no turno com `btItemNoCombatente` (o efeito separado da
  bolsa do jogo normal, sem mudar o comportamento dele). "Used X on Y!" /
  "Foe used X on Y!". **MUDANCA ALEM DO PEDIDO:** a fila do turno agora segue
  o pbAttackPhase do Anil — trocas, objetos, megas, golpes (antes a mega
  vinha antes da troca). Achado e corrigido: com a bolsa aberta o relogio
  dos 60 s parava (btCaptureTick congelava o tick antes). Testes: 5 pvp_ui
  (proibida, ate 3 com save intacto, Revive, objeto do outro antes do
  golpe do mais rapido, tempo acaba com a bolsa aberta) + 1 pvp_rede
  (Super Potion pela rede). 9 sabotagens caem (sab59b.py). NAO gravado.
  Previa: digisprites/batalha_placas_leva59b_bolsa.png.
- **Leva 59 (2026-11-15) — o combate pela rede.** Os dois times chegam
  (DU_BATALHA) e a troca_ui abre `battleStartPvp` POR CIMA da troca (time
  do anfitriao = lado 0; semente `dueloSemente(sessao)`); desenho e toque
  vao ao combate enquanto `bt.open && bt.pvp`. Ganchos: `pvpEnvia` (minha
  jogada + hash do ponto -> `duelo.decide`), `pvpAoDesistir`, `pvpAoAcabar`
  (resultado + `duelo.acabou`). Quem nao decide num ponto manda NADA ja.
  Pela rede o ponto so fecha quando o Duelo tem AS DUAS (`pvpChegou`), mesmo
  a NADA dela: as duas placas passam juntas e o hash e conferido em todo
  ponto. Duelo acabou por fora (saiu/sumiu 20 s/dessincronia) ->
  `pvpInterrompe` com a frase, sem resultado, save intocado. Relogio: 60 s
  da MINHA escolha, os ultimos 10 contando num circulo no alto; acabou ->
  a placa escolhe (golpe marcado ou o 1o com PP; 1o do banco no relevo).
  Bandeira (a de fugir) = DESISTIR em 2 toques: o 1o arma (vermelha + "Give
  up? Tap the flag again."), tocar fora desarma sem escolher golpe
  escondido. Os dois lados sem ninguem = EMPATE (como o pbJudge do Anil).
  Tela de fim: titulo + VOCE VENCEU!/VOCE PERDEU/EMPATE ou "voce
  desistiu"/"a outra placa desistiu"/sumiu/"as placas se desencontraram"
  (5 textos novos x 8 idiomas). Fantasma do PC joga o lado dela lendo o
  ponto aberto (`pvpFantasmaJoga`), com chaves de teste (cala, hash errado).
  Testes: pvp_rede x4 (3v3 inteiro pela rede ate DM_FIM com NADA dos dois
  lados; 60 s + contagem medida na tela; a outra some; dessincronia), empate
  por Explosion, o convite agora termina desistindo pela bandeira. 8
  sabotagens caem. Logica 1171/1171; tela 157/158 (globo). NAO gravado.
  Prévia: digisprites/batalha_placas_leva59.png.
  Ficou para a proxima: a BOLSA no PvP (regras nenhum/livre/ate 3, virtual,
  DD_ITEM) — o botao segue escondido; a convidada desenhando espelhada (60);
  o placar V/D e as duas placas reais (61).
  Achado (jogo normal, nao mexido): quando os dois caem juntos e ninguem
  tem banco, o motor da a vitoria a quem RECEBEU o golpe (battle.cpp:3858);
  no Anil e empate (pbDecisionOnDraw = 5).
- **Leva 56 (2026-11-14) — o time de cada lado (pvp.h/.cpp) e banco dos
  dois lados no motor.** As duas placas montam os DOIS times dos registros
  de 52 bytes (nunca do Pet vivo), nivel 50, PS/PP cheios; golpes nao
  escolhidos = os do nivel REAL. Relevo reconstroi do registro e traz so
  PS, estado (Toxico recomeca), PP proprios, objeto, mega, hero/entryDone:
  Transform e Skill Swap se desfazem. Motor: `foeReserves`/`temReservas`,
  `querTrocar[2]` (Ida e Volta, Relevo, Huida) e `arrastado[2]` (Rugido,
  Cola Dragao, sorteado com btRand). Rugido/Cola Dragao seguem o Anil:
  falham contra Ventosas (AB_SUCTION_CUPS novo), Perro Guardian e Arraigo;
  **MUDA O JOGO NORMAL**: Rugido contra salvaje de nivel maior ou com
  Arraigo agora falha; Cola Dragao contra salvaje de nivel maior ou no
  boneco nao acaba o combate. 11 testes (test_pvp.cpp), combate 3v3
  inteiro igual hash a hash com ruido; 6 sabotagens caem (sab56.py).
  Bateria 1171/1171, UI 145/146 (globo), ubtrap 1171, desktop 0,
  sabotaje 19/19, contraste 0 diffs. Firmware 3.448.803 bytes. NAO gravada.
  - **Achados, sem mexer:** a tela NUNCA le `wantsSwitch` — Ida e Volta,
    Relevo e Huida nao trocam de bicho no jogo normal. Natural Cure =
    Shed Skin a 100% no fim do turno; no Anil cura AO SAIR. Mimikyu
    recupera o Disfraz a cada entrada (alEntrar poe `disguised` sempre);
    no Anil a forma quebrada fica. Ventosas/Remolino com salvaje que ruge
    no jogador: o Anil arrastaria o seu; aqui acaba o combate.
- **Leva 55 (2026-11-14) — protocolo da batalha (duelo.h/.cpp).** O
  pareamento e o da troca: o CONVITE leva as regras no `rsv` (DR_*, 0 =
  troca) e o bit TROCA_SOU_ANFITRIAO (sem ele, convites cruzados com a
  MESMA sessao deixavam as duas anfitrias); `entregaDuelo()` cala a troca
  (TE_DUELO). Duelo: times um bicho por pacote ate o TIMEOK com hash;
  lockstep por ponto de decisao k com btHash, reenvio, resposta a quem
  ficou um atras e depois do FIM, VIVO a cada 1 s, 20 s sem nada = sumiu,
  SAI com eco. 9 testes com ar perdendo 20-50% (300 pontos iguais nas
  duas); 3 sabotagens caem. NAO gravada (sem tela ainda).
- **Leva 54 (2026-11-14) — BATALHA ENTRE PLACAS, base.** Regras do dono:
  quem convida escolhe (1v1/3v3/6v6; itens de mochila proibido/livre/ate 3;
  seguraveis sempre); 1v1 e 3v3 escolhem do time; TODOS nivelados no 50;
  o save NAO muda (no maximo placar). Arquitetura: lockstep canonico (quem
  convida e o lado 0 nas duas placas; a convidada so DESENHA espelhada),
  trocam-se so as escolhas. Esta leva: o dado do motor (`btRand`, 47
  tiradas de battle.cpp; desligado = random() de sempre) e `btHash` para
  comparar a cada turno. Teste: duas "placas" simuladas (Lapras x Vaporeon,
  Thunder/Hypnosis/Confuse Ray...), uma gastando random() como a tela:
  mesmo combate; sabotagem (dado ignorando a semente) cai. NAO gravada
  (nada visivel ainda). Falta: convite+regras+escolha, time por lado no
  combate, protocolo de decisoes com seq/reenvio/timeout, vista espelhada,
  sem EXP/captura/fuga; fundo sincronizado (entorno muda Camuflagem).
- **Leva 53 (2026-11-14)**: GINASIO/MEDALHAS do mapa "dificil de apertar
  e ficar voltando": a faixa das setas (±40 em y=352) pegava x<112/x>354
  ate y=392 — errar o botao trocava de regiao (onde ele some). Botoes
  agora antes das setas, 112x44, toque x 96..233 | 233..370, y 362..424;
  setas param acima. Teste + sabotagem (codigo velho falha). O mapa de
  toques nao via porque trocar de regiao e "acao", nao "fechar".
- **Leva 52 (2026-11-14)**: a lista da troca so via a caixa ABERTA. Agora
  barra "BOX n/32" com setas que pulam caixas vazias; `trocaCaixa` lembra
  de onde saiu a oferta e o loop volta a ela fora da lista. Teste de tela
  + 2 sabotagens. Testes: `preparaElMundoT` fixa boxOpen=0 (um teste
  deixava a 6 aberta) e `todoCerradoT` zera `troca` (sessao repetida pelo
  random semeado). Gravada nas duas placas (2026-11-14), saves iguais.
  Os scripts grava49*.py agora so aceitam o bootloader pelo serial COM
  dois-pontos (a 1a tentativa pegou a COM6 ainda no app, falhou a leitura
  e parou sem gravar).
- **Leva 51 (2026-11-14) — SEGURAR/DAR.** Bolsa: objeto de troca abre a
  ficha com USAR (evolui o titular, como antes) e DAR; objetos de segurar
  ganham DAR ao lado do HOLD IT. DAR -> pagina do time "Dar X a quem?"
  (`darItemId`), tocar a fila = `Pet::darItem` (titular via holdItem; o
  mesmo de novo tira; passa de um para outro). O que alguem do time
  segura continua na bolsa (`quemLeva`). Nome do objeto pequeno na linha
  do bicho (time/banco/troca). 3 sabotagens caem. Gravada na COM6.
  Visto na previa e NAO consertado: na bolsa o "tap: back" pisa a ficha do
  objeto (ja era assim com os de segurar).
- **Leva 50 (2026-11-14)**: o dono viu "uma confirmou e a outra ja
  aceitou". O protocolo NAO aceita sozinho; duas causas na tela: (a) o
  painel as vezes repete o toque e o VOLTAR/ultimas linhas da lista caem
  em cima do TROCAR do resumo -> trava de 350 ms ao mudar de tela;
  (b) "ja confirmou" ambiguo -> "a outra placa confirmou: falta voce" e
  "esperando a outra placa confirmar". Cena de evolucao ao chegar (3,2 s,
  OK so depois). Comando `BANCO <dex> <nv> [objeto]` (escreve: caixa
  aberta, segurando objeto) para testar Scyther+Metal Coat. Itens ids:
  Everstone 11, Dragon Scale 68, Up-Grade 69, Metal Coat 70, Protector 78.
  Segurar so se poe no TITULAR (e o titular nao sai): caminho no jogo e
  segurar e depois trocar de titular -- ~~PENDENTE~~ RESOLVIDO na leva 51 (DAR na bolsa, qualquer um do time).
- **UI `reacao.el_globo_ensena_la_onomatopeya_de_lo_que_pide` FALHA na
  bateria inteira (sozinho passa)**: o coracao nao apaga. Provado que NAO e
  da leva 48 (copia com todo o codigo de antes falha igual). Estado deixado
  por um teste anterior; investigar.
- **Trocas ESP-NOW (planejado)**: decidido com o dono — trava de nível
  vale, item viaja junto, lendário/shiny trocáveis, depois batalha entre
  placas. Itens TradeItem (Metal Coat etc.) hoje são "pedra" (usar evolui
  na hora); na troca: perguntar Usar/Segurar e evoluir ao chegar
  segurando. Commit em duas fases com diário no SD; recebido registra na
  Pokédex.

---

## 1. Times por região — **FECHADO**

1000/1000 na lógica (10 testes novos), 59/59 na UI, 49/49 na sabotagem.

Dois achados da sabotagem, que valem mais que os verdes: o teste de
viajar estava FALSO (media sobrevivência do dado, não a troca — sem a
troca o array nunca é tocado), e o teste do cadeado media a aba errada
da página 4. Os dois refeitos.

O que entrou: trio de iniciais por região no `gyms.h`; `teamsIn[9][6]` +
`mainSlotIn[9]` + máscara de regiões pisadas, 2,8 KB na NVS (não no
cartão, porque a SD não está montada na hora da migração); `setZone()`
troca time + titular + insígnias; `cargaDelHueco()` extraído do
`setMain()`; ovo do inicial na primeira chegada; `bankCapHere()`
separado do `levelCap()`, com piso no nível do titular; cadeado
desenhado na caixa; tela de escolha do inicial agora olha a região.

Pedido antes do Rodar. O problema que originou: chegar em Johto com o
time de Kanto e atropelar os líderes do começo.

- 9 times de 6, um por região, guardados e trocados ao viajar.
- Ao chegar numa região pela primeira vez: ovo do inicial dela. O
  titular é substituído pelo ovo e **fica no time da região anterior**;
  voltando pra lá, ele volta a ser titular.
- Banco **compartilhado**, mas na região R só dá pra sacar quem estiver
  dentro do teto de R. Cadeadinho em cima de quem passa do teto.
- Vencer líderes de R vai liberando tetos maiores até liberar tudo.
- Voltar pra uma região terminada traz o time dela **e** o banco inteiro
  liberado.

**Cuidado 1:** são DOIS tetos diferentes. O `levelCap()` que já existe é
o de combate e pega o **maior** entre as regiões de propósito — baixá-lo
ao viajar seria punição pela porta dos fundos, e isso foi regra sua.
O teto novo é só pra **sacar do banco** e é o da região atual.

**Cuidado 2:** tem migração de save. Duas fases, conferir que o cartão
está montado antes, e não apagar o formato velho antes do novo estar
escrito E relido. Teste do caminho de falha, provado por sabotagem.

**Cuidado 3:** o save do dono já foi a Johto e voltou. Como não existe
registro dessa ida, na migração o time atual vai pra Kanto, Johto fica
vazia, e ele ganha o ovo quando voltar lá.

## GRAVADO em 2026-10-31

Firmware: 2 718 946 B de flash (86%, **427 KB livres**), 72 924 B de RAM
(22%, **249 KB livres**). Hash verificado.

Cartão: `/fx/fx.bin` (11,6 MB) e `/por/por.bin` (8,4 MB), os dois a
~233 KB/s pela serial. Sem `--usb on`.

Dois defeitos do PUT por serial apareceram só ao tentar: ele aceitava
apenas `/mons/` e `/bg/` e recusava acima de 4 MB; e **nunca conseguiu
criar uma pasta nova** (o FAT não cria diretório intermediário, então o
`/por` falhava no `open`). Os dois corrigidos. O segundo nunca tinha
aparecido porque `/mons` e `/bg` existiam desde sempre.

Correção de uma anotação errada minha: `--dir` **não** servia para tudo.
Servia para os arquivos pequenos. O `fx.bin` só estava no cartão porque
em algum momento foi montado como disco no PC.

### Falta conferir NO APARELHO

Nada disto dá para provar daqui:

- a cara aparece mesmo no balão, e o símbolo em cima dela
- o matinho entra no combate sem o "!" no alto
- o cadeadinho na caixa
- a migração dos times: time atual em Kanto, ovo ao chegar em Johto
- ~~o tempo de leitura do fx~~ — **MEDIDO em 2026-10-31**, ver abaixo.

### O teto do fx, medido (2026-10-31)

Um combate com o cabo ligado, via `tools/log_serial.py`:

| golpe | tamanho | tempo |
|---|---|---|
| TACKLE | 2 656 B | 9 ms |
| QUICKATTACK | 4 424 B | 9 ms |
| DIAMONDSTORM | 30 048 B | 25 ms |

Os dois pequenos batem num piso de ~9 ms de **custo fixo** (abrir,
procurar, ler cabeçalhos). O terceiro informa: 30 KB em ~17 ms de
transferência = **~1,7 MB/s**, contra os 0,18–0,25 MB/s que eu tinha
suposto. Sete a nove vezes mais rápido.

Com ~700 ms de cobertura cabem perto de 1,1 MB por animação, não 128 KB.
A maior embarcada hoje é CRUNCH com 125 668 B — raspando no teto, ou
seja há animações sendo descartadas por um número inventado.

**Lição:** esse teto já tinha sido "corrigido" três vezes por aritmética
(40 → 120 → 128 KB) e as três vezes era conta. A medida levou um combate.

Patch pronto em `_claude_scratch/p_teto_fx.py`: 512 KB no gerador e
640 KB no `SdFx::load`. Os dois juntos de propósito — são limites
independentes, e se o da placa ficar abaixo do do gerador, o gerador
embarca o que a placa recusa e o golpe cai no genérico em silêncio.

### Ainda sem explicação

O desenho de um quadro deu pico de **189–197 ms** (contra ~79 ms
normais), nas duas sessões de log. **Não é** a leitura do fx: a maior
medida foi 25 ms. É outra coisa.

## 2. Retratos do SpriteCollab

Créditos já feitos. Falta:

- empacotar os retratos pro cartão, indexados por espécie, **com
  assinatura** (senão lê o vizinho quando a tabela crescer).
- levar os balões do Anil pro `art.h`: o `Z` do `PRAS- Sleep Talk.png`,
  a cruz vermelha do `safari_anger.png`, e o `Follower_Emote1/2/3`.
- ligar em três telas: reação em casa (fome, sono, sem HP, sujo),
  comemoração na tela de exp/prêmios, e cara + crédito na Pokédex.
- mapa de emoção: específica → `Sigh` → `Normal`, com onomatopeia em
  todos. `Sigh` é a rede de segurança (decisão dele).
- os `SpecialN` não têm nome no `sprite_config.json`: cada autor usou
  como quis. Folhas de contato em `_claude_scratch/specials/` pra ele
  escolher a olho; o que marcar entra como exceção por espécie.

## 3. Animações: a coreografia do sprite

O que ele notou no Extreme Speed. No Anil são 35 quadros e **os 35
movem o sprite**; a folha só tem clarões de impacto. A gente emite os
clarões e joga a atuação fora — e como eles saem do `PRAS- Strike.png`,
que **84 golpes** compartilham, Extreme Speed, Quick Attack e Tackle
ficam idênticos.

Conserto: ler X/Y/zoom/espelho do sprite nos quadros (o `fxanim` já
decodifica, só ignora quando o `pattern` é negativo) e mover o nosso
sprite de verdade. Isso conserta os 84 de uma vez, em vez de mapear na
mão um por um.

Medida pra priorizar: `com_sprite` vs `com_celula` por golpe — diz
exatamente quem perdeu a identidade.

## 3b. ~~Ícones do PokeSprite — medalhas como ribbons~~ FEITO 2026-11-02

As 30 conquistas têm fita (`tools/baixa_fitas.py`, `/por/fitas.bin`); crédito no CREDITS.md.

https://msikma.github.io/pokesprite/overview/misc.html

As medalhas hoje são **texto dentro de caixa**. Ele quer que virem
ícones no estilo das ribbons. O PokeSprite tem o conjunto de ribbons,
marks e itens em pixel art pequena, que é exatamente a escala da nossa
tela.

Antes de usar: conferir a licença do repo (o código e as imagens têm
termos diferentes lá) e ver se exige crédito — se exigir, entra junto
com o do SpriteCollab.

Checar também o que mais de lá serve: ícones de item pra bolsa, marks,
e os ícones de bola.

## 4. O resto da varredura

- **94 espécies** chegam ao nível 45 com ≤1 golpe de dano (regra dos
  últimos 4). Levantado, sem decisão.
- ~~Treinadores "!" aleatórios sorteiam lendários~~ FEITO.
- ~~O firmware não confere o `golpes.txt`~~ FEITO 2026-11-02: `sdFirmaGolpes()`
  confere fila a fila; sem bater, nenhum golpe toca som próprio.
- ~~Segunda leva de golpes~~ FEITO 2026-11-02: Trick Room, Substitute,
  Wish, Perish Song, Embargo, Heal Block (filas 725-730, ordem congelada:
  tools/orden_golpes.txt e orden_mts.txt). Junto: Fly/Dig/Hyper Beam/
  Outrage saem sozinhos no 2o turno; sons montados com TODOS os SE do Anil.
- ~~Golpes cujo efeito do Anil o motor ignorava~~ FEITO 2026-11-02: Bide,
  Beak Blast, Shell Trap, Beat Up, Natural Gift, Secret Power (com entorno
  pelo cenario), Pay Day (so a frase: nao ha dinheiro). Encore/Choice apagam
  os outros golpes no menu; frases de carga do Anil (Fly, Dig...).
  Falta: a frase "comeca a aquecer o bico" ANTES da ronda (o Anil mostra);
  Judgment/Multi-Attack/Techno Blast so mudam de tipo com Placas/Memorias/
  Drives, que nao existem como item (ids de item sao uint8 e estao em 228).
- Evolucao com a musica do Anil (grito velho, BGM Evolution, grito novo,
  fanfarra) e Mega com fundido a branco: FEITO 2026-11-02.
- Medir na placa quanto demora a leitura do fx. O teto de 128 KB é
  conta, não medida — o `battleTick` já imprime o tempo pela serial.
- ~~Auditoria de golpes que falta~~: FEITA 2026-11-16, GRAVADA nas duas placas (leva 62).
  Já batiam com o Anil: Crash, Future Sight, Tantrum, Stored Power,
  Present, Encore, Knock Off/Trick/Thief com Mega Stone, e o +1 do
  Ancient Power continua mesmo se o rival cai. **Mudam o jogo normal:**
  - Sheer Force: agora só conta golpe com EffectChance (MV_ADDL). Antes
    Overheat/Close Combat ganhavam +30%, e Ancient Power/Flame Charge
    ganhavam +30% E a subida. Também tira Life Orb, Berserk, Pickpocket,
    Wimp Out, Anchor Shot, Salt Cure, Throat Chop, Sparkling Aria.
  - Rapid Spin: +1 Velocidade (gen 8+), faltava.
  - Multigolpe: 2-5 em 35/35/15/15% (era 37,5/37,5/12,5/12,5); Triple
    Kick/Triple Hit 1x/2x/3x de potencia e precisão a cada impacto (Bomba
    Demografica tambem; Skill Link pula); Triple Hit não amedronta mais
    (no Anil não tem EffectChance; aqui era SEMPRE); Scale Shot +1 Vel
    -1 Def; efeito adicional rola por impacto (Twineedle 36%, Double
    Iron Bash 51%).
  Testes: 11 novos, todos provados por sabotagem (12 sabotagens).

## 5. Gravar

Não feito desde as sombras. O `fx.bin` mudou: **11,6 MB a 115200 baud,
~18 minutos** de serial. Nada de `--usb on`.

---

## GRAVADO em 2026-11-04 — Encore, Yawn e durações como no Anil

Lote `leva4` (da sessão anterior, que parou com os testes rodando) mais o
que a conferência achou. 1082 lógica, 93 UI, ubtrap verde, 103/103
sabotagens do motor, 4/4 de tela, 19/19 de dados. Firmware 2.852.006 bytes.

- **Encore = 4 rodadas** contando a do uso (3 se o alvo já tinha agido).
  Falha como no Anil (sem PP, lista negra, Shell Trap), atravessa o
  Substituto, acaba se o golpe fica sem PP e avisa quando acaba.
- **Sem golpe para escolher = Struggle**, não turno perdido
  (`pbAutoChooseMove`): Encore sobre golpe com Disable/Taunt/Torment etc.,
  ou todos os golpes fechados. `Battle::debeForcejear()`, decidido em
  `inicioDeRonda()`. O que fecha no meio da rodada continua falhando.
- **Yawn** dorme no fim da rodada seguinte. Durações: Taunt 4, Disable 5,
  Cursed Body 3, Throat Chop 2, Uproar 3 fixos, confusão 2–5, Future Sight 3.
- **Golpe de apoio não olha a tabela de tipos** (só Thunder Wave): Confuse
  Ray pega em Normal, Sand Attack em Voador. Era o contrário.
- Tri Attack, Dire Claw, os Fangs, Shell Smash, Toxic Thread, Growth,
  Freeze-Dry e Chloroblast com a mecânica inteira.

### Ficou aberto

- Com Encore o Anil nem mostra o menu de golpes; aqui a carta aparece e
  qualquer toque solta o golpe preso. Resultado igual, tela diferente.
- A carta mostrando STRUGGLE quando não há golpe não tem teste de tela.
- `cobertura.py` sai com erro por `AB_GLUTTONY` (sem uso desde que a baya
  passou a saltar a meia vida para todos) e lista `SX_SELF_ATK`, que
  nenhum golpe leva. Os dois são anteriores a este lote.
- 4 slots do banco (10, 14, 15, 16) não entraram na comparação
  antes/depois da gravação: a serial perdeu as linhas. Os outros 31 batem.

### Ferramentas novas em `_claude_scratch/`

- `sab_par.py <dir> [N] [filtro]` — a sabotagem em N cópias, em paralelo,
  sem tocar na árvore (103 entradas em ~40 min com 10). Mede a linha de
  base num binário limpo antes. As cópias não levam o cartão: os testes de
  `sdmon` vão por `sab_sd.py`, na árvore real.
- `sab_ui.py` — a sabotagem dos testes de tela.

---

## GRAVADO em 2026-11-05 — resto da auditoria de golpes, itens e bolsa

1110 lógica (e o mesmo com ubtrap), 105 UI, 141/141 sabotagens do motor,
19/19 de tela, 19/19 de dados. Firmware 2.855.030 bytes (90%).

- **Stomping Tantrum** olha a RODADA anterior. Conta como falha: errar,
  não afetar (imune), "but it failed", não conseguir agir. NÃO conta:
  Protect, Splash, usar item/bola/trocar.
- **Queda (High Jump Kick etc.)** também contra Protect, imune e alvo
  escondido; Magic Guard não cai. A tela diz "kept going and crashed!".
- **Golpes múltiplos** dizem quantos acertos ENTRARAM.
- **Stored Power / Power Trip** sem teto (era 250).
- **Present**: 40/30/10/20 como no Anil; quando cura não olha tipos e
  falha se o alvo está cheio ou com Heal Block.
- **No Retreat**: uma vez só, e prende (nem troca nem foge).
- **Future Sight / Doom Desire**: o dano é calculado quando CAI, contra
  quem estiver lá; se quem lançou desmaiou, não chega.
- **Itens**: a pedra mega do próprio bicho não sai (Knock Off, Thief,
  Trick, Pickpocket). Selvagem não tira item. Roubar de selvagem vai
  para a mochila. O que TIRAM de você volta no fim da batalha; só se
  perde o que foi GASTO (berry, Focus Sash, Fling).
- **Encore na tela**: a carta fica presa no golpe do Encore.
- **Rodada em que o jogador não ataca** (item, bola, troca, fuga que
  falha): o rival agora prepara a rodada dele (`inicioDeRondaRival`).
- **Bolsa na batalha**: dois botões grandes de página, arrastar rola,
  só fecha tocando FORA do painel. (Reportado: "quase 100% das vezes a
  tela volta pra batalha".)
- `Battle::start()` reinicia no lugar, sem cópia de 800 bytes na pilha.

### Ficou aberto

- ~~Decisão do dono: berry gasta ou volta?~~ **DECIDIDO em 2026-11-05:
  a berry GASTA.** No Anil `RESTORE_HELD_ITEMS_AFTER_BATTLE = true` (lá
  até berry consumida volta), mas aqui fica como está: só volta o que
  TIRAM do bicho. Não igualar.
- Trick contra selvagem com as mãos vazias: no Anil o item fica com você
  para sempre; aqui some no fim da batalha.
- Os 6 pontos de `inicioDeRondaRival` na tela não têm teste de tela
  próprio (a função tem teste de motor).
- A carta mostrando STRUGGLE não tem teste de tela.
- `cobertura.py`: `AB_GLUTTONY` e `SX_SELF_ATK`, como antes.
- Nada disto foi jogado na placa: só testes e simulador.

---

## GRAVADO em 2026-11-05 (2) — toque, clima em tela cheia, sons da bola, Play Rough

1111 lógica (e com ubtrap), 118 UI, sabotagem: 142/142 do motor (árvore
limpa), 26/26 do toque, 12/12 de clima e sons, 1/1 do carregador de clima,
19/19 de dados. Firmware 2.857.162 bytes (90%). No cartão foram 6 sons e
111 animações.

- **O FOSO (toque)**: em nenhuma tela um toque que erra um alvo por 16 px
  ou menos fecha a tela. Valia só para a bolsa da batalha; o mapa de
  toques mostrou o mesmo defeito na lista de troca, na revanche, e em
  golpes / bolsa / equipe / banco / ginásio / medalhas da ficha, no mapa,
  na comida e na Pokédex. `UI_FOSO` e `lejosDe()` em battle_ui.h. As
  listas da ficha fecham no título ou no "tap: back", não no meio.
- **Revanche**: os mesmos dois botões de página da bolsa, e arrastar rola.
- **Medalhas**: a seta de página estava EM CIMA do "tap: back" (tocar
  "voltar" passava de página). Subiu para y=334.
- **Três defeitos que o mapa achou de passagem**: a página de golpes
  aceitava toque numa 6ª fila que não é desenhada (tocar o letreiro
  "tap a move..." trocava um golpe invisível); e a divisão inteira que
  trunca para zero fazia a faixa ACIMA das listas (golpes, equipe, banco)
  contar como a fila 0.
- **Clima em tela cheia e em laço**, como no Anil (plugin "Continuous
  Weather Animations" + Common:Sun/Rain/Sandstorm/Hail). `tools/pack_clima.py`
  -> `/anim/w_<n>.bin` (formato TPCL), `SdClima` em sdmon, `btClimaQuadro`
  em battle_ui.ino. A habilidade de clima (Drought...) já não toca o golpe
  Sunny Day: basta o clima mudar.
- **72 golpes iam sem partículas** (Surf, Rest, Bulk Up, Muddy Water...): o
  Anil cita 105 folhas sem ".png" e o pack_anim.py não as achava.
  Reempacotados; `auditoria.py --seccion animaciones` agora vigia.
- **Play Rough**: quem ataca não se desenha enquanto está dentro da nuvem
  (`NUVEM_DE_BRIGA` em pack_anim.py). É um desvio de propósito dos dados
  do Anil, onde o sprite fica lá e é a nuvem (maior) que o tapa.
- **Sons da Pokébola** do Anil: arremesso, chegada, queda, um toc por
  balanço, clique (pegou) ou abertura (escapou). SFX_BALL_* (s15..s20.raw).

### Ferramentas novas

- `TAMA_MAPA=1 ./build/tamapoke_uitests mapa` + `python
  _claude_scratch/mapa_toques.py <pasta>`: foto e mapa de toques de cada
  tela (vermelho = fecha). O teste `toques.fallar_por_un_pelo...` é a guarda.
- `python tools/pack_clima.py [--previa <pasta>]`.

### Ficou aberto

- **A pasta `Plugins/` do Anil NUNCA tinha sido lida.** Os scripts em
  `_claude_scratch/scripts` são só o Scripts.rxdata; o Deluxe Battle Kit,
  o Advanced AI, o clima contínuo e os Pokémon animados estão em
  `Pokemon Anil V4.13\Plugins\` (com fonte). A auditoria "igual ao Anil"
  feita até aqui não olhou nenhum deles. Falta varrer.
- No Anil, no fim de cada rodada com clima sai a frase ("La tormenta de
  arena arrecia"). Aqui não.
- O terreno (Electric Terrain...) continua tocando a animação do GOLPE na
  habilidade; no Anil é Common:ElectricTerrain etc.
- Sons de bola ao SOLTAR um Pokémon (Battle throw / recall no início e na
  troca): não feitos, só a captura.
- GRAVADO 2026-11-06: "o jogo ficou meio lento com o sunny day". A figura
  do clima passou a misturar com o truque do 565 em 32 bits e alfa de 5
  bits (sem divisoes; erro maximo 4/255). Comando serial `CLIMA [0..4]`
  poe clima num combate aberto e diz os ms do ultimo quadro e da figura.
  Ainda NAO medido na placa: o bicho estava dormindo e FIGHT recusa.
  Se seguir lento: desenhar a figura em metade das linhas, ou pular o
  quadro do clima enquanto toca a animacao de um golpe.
- Os roteiros de `desktop/*.txt` que fechavam uma tela tocando no meio
  agora não fecham (só `revancha.txt` foi ajustado). São só para prévia.
- Nada disto foi jogado na placa: testes, simulador e arquivos conferidos.

---

## GRAVADO em 2026-11-07 — multigolpe impacto a impacto

- **Multigolpe** ("se for acertar 3x, bater 3 vezes"): o motor apunta
  `hitDmg[]` por impacto; a tela toca a animacao uma vez por impacto e a
  barra do rival baixa um pedaco em cada BP_HIT (`bt.hpPend`), nao mais
  de golpe ao anunciar. Em TODOS os golpes de dano a barra agora cai no
  impacto, nao no anuncio. "Hit N times!" segue no fim.
- Sabotagem: 3/3 de tela, 1/1 do motor (sab_par). 1112 logica, 119 UI.
- Tempo de quadro medido na placa sem clima: 65 ms. Com sol ainda nao
  medido (`CLIMA` pela serial depois de uma batalha do dono).
- **2026-11-06, acidente**: mandei `RUN` pela serial para "fugir" de uma
  batalha de teste; e `pet.startRunaway()`. O Arcanine nv50 do dono foi
  embora; refeito com SPEC/LVL/ABIL (Intimidate), Arcanite e vinculo
  mantidos; IVs/EVs/golpes originais perdidos. Nunca mais um comando
  serial sem ler o handler.

### Corrigido em seguida (leva 22)

A leva 21 quebrou a barra: "aparece ate subindo antes de cair, com delay".
- SUBIA: hpPend era last.damage, que passa da vida num golpe que derruba
  (dano 342, vida 12: 48 > 1328 > 0). Reproduzido em 12 casos no codigo da
  leva 21. Agora sai da vida medida antes e depois do useMove.
- DELAY: eu tinha mudado TODOS os golpes para a barra cair no fim da
  animacao, sem ninguem pedir. Voltou a cair quando a animacao comeca; so o
  multigolpe reparte, um degrau por repeticao.

---

## GRAVADO em 2026-11-09 — regioes no SD (leva 44) e NVS sem o bloco teams (leva 43)

- /save/reg00..08.bin + estado.bin (seq do save), .new + releitura + rename, CRC.
  Boot: SD com seq maior que "svn" da NVS => manda o SD (o caso de Hoenn).
  tm0..tm8 sairam da NVS (livre 220 -> 420 entradas). SD nao formata mais sozinho.
- Ler sem escrever: `SUMS /save`, `DIAG` (seq, svn, sdManda, sdGravouEstado).
- ~~Falta: reconstruir Hoenn~~ — o dono confirmou em 2026-11-15 que esta tudo ok (Hoenn, 6 no time, 7 insignias).

## (historico) a NVS cheia que levou Hoenn

- Causa: NVS com 409 de 504 entradas; regravar o bloco "teams" (2,8 KB) pedia 92 de
  folga. Em Hoenn nada mais gravou, calado; o reinicio da gravacao levou a regiao.
  Dump cru: `_claude_scratch/nvsdump/` (parse_nvs.py). tm2 vazio, regn=1.
- Correcao: save nao escreve mais "teams" nem "teamsbk"; o boot apaga os dois quando
  nenhuma chave tmN existente estiver ruim. Sentinela "svn" no fim do save. Faixa
  vermelha "NAO SALVOU!" e nova tentativa 1 min depois da falha.
- A gravar so com autorizacao: o boot novo ESCREVE no save (apaga "teams").
- Depois: reconstruir Hoenn (time e banco lidos em eq_antes42.txt) com o "sim" do dono.

## GRAVADO em 2026-11-08 — aventura em silencio no escuro

- Na aventura a musica cala com a tela escurecida (90 s) ou apagada, e volta ao tocar.
- Um coracao pendente do bicho de casa nao segura mais a tela acesa durante a aventura.
- Pausar (GUARDAR) com aventura: o tempo conta pelo relogio (teto 8 h) e a marca de
  pausa se gasta na volta (antes sobrava e o proximo apagado normal nao cobrava fome).
- **PERDA**: o dono estava em Hoenn (Torchic nv14...) e nada disso estava gravado; o
  reinicio desta gravacao levou tudo. Ver a entrada de 2026-11-09.

## GRAVADO em 2026-11-07 (2) — aventura e tela do cuidado nova

- **Aventura** (`aventura.ino`, regras em `Pet::aventura*`): sprites do SpriteCollab em
  `/av/` (`tools/pack_aventura.py`, 311 MB no cartao), todas as animacoes menos a lista
  FORA. Tamanho = zoom unico sobre o sprite do PMD; sem sprite cai no padrao pela tabela
  screenSz com a razao medida. Rende pelo relogio, teto de 8 h, EXP desde 1 minuto,
  fome pausada. Numeros em pet.h (AV_*): SEM jogar ainda, podem mudar.
- **Tela do cuidado**: fundo da Pokedex, chapa inclinada, seis botoes meia-pokebola em
  passo igual no ANGULO. Botao novo: aventura (o mapa). Tirado do mapa.
- **Centro Pokemon** repoe os PP do time e da caixa aberta; a enfermeira vem por falta de PP.
- 1123 logica, 132 UI, auditoria limpa.

### Ficou aberto
- Medir na placa o custo da aventura com clima (`CLIMA` pela serial le o ultimo quadro).
- A cura e o elenco so veem a caixa aberta, nao as outras.
- Dormir repoe PP so do titular.
- Sobrou `/av/zz.bin` (1 byte, do teste de PUT): inofensivo.
- `--seccion animaciones` nao confere a lista FORA dentro dos arquivos.
