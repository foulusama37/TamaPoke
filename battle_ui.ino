// Pantalla de combate. Va como .ino aparte para que el sketch principal la
// compile junta y pueda usar sus helpers de dibujo (drawPmdActM, tintEllipse,
// drawMap, el canvas gfx...) sin tener que exportarlos a un .cpp.

BattleUi bt;
PmdMon foePmd;           // sprite del rival; el mio ya esta en `pmd`
int16_t foePmdFor = -1;  // que especie hay cargada, para no recargar cada frame
// La ultima clave que se INTENTO cargar, acertando o no. Va aparte de
// foePmdFor para que un sprite que falta en la tarjeta no se reintente
// en cada fotograma: btSyncFoeSprite() corre en el render. Ver el
// comentario largo de esa funcion.
int16_t foePmdTried = -1;
// Las mismas dos, del lado del jugador. Arriba del todo porque se
// usan en el cierre del combate, que esta mucho antes que su funcion:
// declararlas junto a ella daba "no se declaro en este ambito".
int16_t myPmdFor = -1;
int16_t myPmdTried = -1;

// PERSPECTIVA: lo unico que el combate anade al tamano de la especie. El
// tuyo esta delante y se ve algo mas grande; el rival esta al fondo.
//
// La razon entre los dos (0,69) es lo que se lee como distancia. Llego a
// estar en 0,75, que casi no se notaba, y en 0,64/0,85 CON TOPES DUROS por
// rol, que aplastaba a 475 de 1025 rivales contra el mismo numero. Estos
// dos estan elegidos para que la banda de la tabla (64..185 px de cuerpo)
// quepa entera sin recortar a nadie:
//
//   185 * 1,05 = 194 px  y de espaldas hay 217 (suelo 312, chapa en y=95)
//   185 * 0,72 = 133 px  y de frente hay 137 (circulo, en x=325)
#define BT_BACK_K 1.05f
// Onde o bicho de costas pode ir de lado a lado: da borda (4) ate uma
// folga antes da placa de PS (que comeca em x=268). Com 264 o Shiftry
// cabia e mesmo assim a folha encostava na placa ("muito grande a
// largura"); 256 deixa ar e toca em 32 das 1025, todas largas, e so no
// que falta. Ele e desenhado com o centro do LIENZO em x=145, entao o que
// manda e quanto a tinta passa de cada lado.
#define BT_BACK_X0 4
#define BT_BACK_X1 256
#define BT_FOE_K  0.72f

// Lo que el combate PIDE y lo que de verdad SALE, de cada lado, en
// pixeles de alto de cuerpo. Indexado por BT_PLAYER / BT_FOE.
//
// Son dos y no uno a proposito. El fallo que esto vigila —— que el
// dibujante anadiera un tamano por su cuenta, el del bicho del jugador,
// encima del que el combate ya habia calculado —— se ve comparando estos
// dos numeros EN EL MISMO fotograma. Mi primer intento de test comparaba
// dos combates distintos y se quedo verde con el fallo puesto a
// proposito, porque el estado del bicho entre combates no era el que yo
// creia. Un invariante local no tiene ese problema.
int16_t btPedido[2] = {0, 0};
// En MAGNITUD, la misma unidad que btPedido: comparar un pedido en
// magnitud con un resultado en alto daba fallos inventados.
int16_t btAltoDibujado[2] = {0, 0};
// De onde ate onde vai a TINTA do bicho de costas na horizontal, com a
// escala ja decidida (sem a respiracao). O teste da largura le isto.
int16_t btLadosCostas[2] = {0, 0};
// Y el alto de verdad en pixeles, para mirarlo con ojos (comando TALLA).
int16_t btAltoReal[2] = {0, 0};
// En que X se acaba de pintar la bola de la ENTRADA de cada lado. Por el
// mismo motivo que los tres de arriba: es la unica forma de afirmar
// sobre lo que se dibujo sin ponerse a leer pixeles de un sprite que
// cambia con cualquier retoque. Lo mira el test de que cada bola viene
// del lado de SU entrenador. -1 = este lado no esta sacando a nadie.
int16_t btBolaX[2] = {-1, -1};

#define BT_CARD_Y0 334
#define BT_CARD_Y1 420
#define BT_GROUND_FOE 178
#define BT_GROUND_MINE 312

// A animacao do Anil tocando: o btDrawSide pergunta aqui. Sem cel no
// quadro o bicho SOME (e o Essentials: so se mostra o sprite que tem cel).
static bool gAnimOn = false;
static const uint8_t *gAnimSpr[2] = { nullptr, nullptr };
#define BT_INTRO_MS 1600

// ======================================================== BATALHA ENTRE PLACAS
// O estado do combate PvP (o resto mora no fim deste ficheiro). bt.b e
// SEMPRE canonico: o anfitriao e o lado 0 nas duas placas.
PvpLado pvpL[2];
uint8_t pvpRegras = 0;
uint8_t pvpEu = BT_PLAYER;          // o meu lado no combate
// O DESENHO: o meu lado sempre EMBAIXO. Entre placas o combate segue
// canonico (o anfitriao e o lado 0 nas DUAS placas) e so a tela troca:
// a convidada pinta o lado 1 embaixo. Fora do PvP e o de sempre.
// Tudo o que DESENHA pergunta a estes dois; o que SIMULA, nunca.
static inline uint8_t btAbajo() { return bt.pvp ? pvpEu : (uint8_t)BT_PLAYER; }
static inline uint8_t btArriba() { return (uint8_t)(btAbajo() ^ 1); }
uint8_t pvpPonto = PT_NADA;         // o ponto de decisao aberto
uint8_t pvpPrecisa = 0;             // bit s: o lado s decide neste ponto
uint8_t pvpTem = 0;                 // bit s: ja decidiu
DueloDecisao pvpQuer[2];
uint32_t pvpAbertoEm = 0;           // quando abriu (os 60 s)
uint16_t pvpPontos = 0;             // quantos pontos ja se fecharam
// A fila do turno: megas, trocas e golpes, na ordem em que acontecem.
// `quem` e quem estava na pista ao entrar na fila: se saiu (Rugido) ou
// caiu antes da vez dele, a acao cai.
enum : uint8_t { PA_MEGA = 1, PA_TROCA, PA_GOLPE, PA_REPOE, PA_ITEM };
// `alvo`: no PA_ITEM, o indice do time em quem o objeto vai.
struct PvpAcao { uint8_t lado, tipo, arg, quem, alvo; };
PvpAcao pvpFila[6];
uint8_t pvpN = 0, pvpI = 0;
bool pvpAcabou = false;
uint8_t pvpVenceu = 0xFF;           // o lado que ganhou (0xFF = ninguem/empate)
// Ganchos para quem esta por fora (a rede, os testes): avisado quando
// um ponto abre e quando o combate acaba.
void (*pvpAoAbrir)() = nullptr;
void (*pvpAoAcabar)() = nullptr;
// PELA REDE (troca_ui): a minha jogada sai por aqui, com o hash do ponto.
// Nulo = as duas jogadas chegam por pvpDecide (os testes, sem radio).
void (*pvpEnvia)(const DueloDecisao &d, uint32_t hash) = nullptr;
// Desistir pela tela, na rede: quem avisa a outra placa e a troca_ui.
void (*pvpAoDesistir)() = nullptr;
bool pvpRede = false;          // a jogada do outro lado chega pelo radio
bool pvpRedeOk = false;        // a rede ja trouxe as duas do ponto aberto
uint32_t pvpHashPonto = 0;     // pvpHash ao abrir o ponto (vai com a jogada)
uint32_t pvpDesistirAte = 0;   // 1o toque na bandeira: o 2o ate aqui confirma
bool pvpCortado = false;       // acabou sem resultado (sumiu, divergiu, saiu)
// A BOLSA entre placas e uma COPIA da minha, tirada ao comecar: gastar
// aqui nao mexe no save (regra do user). A da outra placa eu nao vejo:
// a jogada dela diz o objeto, e as duas aplicam igual.
uint8_t pvpBolsa[ITEM_COUNT];
bool pvpItemServe(uint8_t lado, uint8_t id, uint8_t idx);
bool pvpBolsaPode();
bool pvpBolsaNaRegra();
void pvpUsaItem(uint8_t id, uint8_t idx);
bool btItemNoCombatente(Combatant &c, uint8_t id, bool aplicar);
// 60 s por escolha, os ultimos 10 contando na tela (decidido com o dono).
#define PVP_TEMPO_MS 60000u
#define PVP_AVISO_MS 10000u
bool pvpChegou(const DueloDecisao &dele);
void pvpInterrompe(const char *msg, uint8_t vence);
bool pvpFantasmaJoga(uint8_t lado, uint16_t k, DueloDecisao &d, uint32_t &hash);
uint32_t pvpRestaMs();
static void pvpTalvezExecuta();
static void pvpRelogioTick(uint32_t now);
bool pvpSegue();
void pvpDecide(uint8_t lado, const DueloDecisao &d);
static bool pvpMeioDoTurno();
static void pvpDesmaio();
static void pvpFecha();
static void pvpAbreTurno();
static void pvpEsperaTick(uint32_t now);
static void pvpPartyTap(int16_t x, int16_t y);
bool battleStartPvp(const uint8_t r0[][TROCA_MON], const uint8_t r1[][TROCA_MON],
                    uint8_t n, uint8_t eu, uint32_t semente, uint8_t regras);

// ancho util del circulo a esa altura (radio 233, centro 233,233)
static int btHalf(int y) {
  int dy = y - 233;
  int v = 233 * 233 - dy * dy;
  return v > 0 ? (int)sqrtf((float)v) : 0;
}
// el ancho que cabe en TODA la franja [y0,y1], con margen para el bisel
static int btBandW(int y0, int y1) {
  int m = btHalf(y0) < btHalf(y1) ? btHalf(y0) : btHalf(y1);
  return (m - 10) * 2;
}

// ------------------------------------------------------------------ fondo
//
// Que escenario toca. La tabla vive en bgs.h, que comparten esta pantalla y
// la de casa. El fichero puede no estar: `SdBg::load` devuelve false y se
// pinta el cielo de siempre, asi que esto nunca es obligatorio.
// Aqui SI se sortea: el escenario se elige una vez, al empezar, y luego se
// queda quieto todo el combate. Dos peleas seguidas contra Geodudes en el
// mismo sitio es justo lo que se queria evitar.
static int8_t bgForWild(int16_t dex) {
  if (dex < 1 || dex > DEX_COUNT) return BG_FIELD;
  return (int8_t)bgPick(bgSetOfBiome(DEX_TBL[dex].biome), (uint32_t)random(1000));
}

// Pone en el combatiente los golpes que el JUGADOR eligio, no los que le
// tocarian por nivel.
//
// Esto faltaba y era un agujero de verdad: `Combatant::init()` rellena
// `moves[]` con los cuatro ultimos del learnset, y lo unico que se copiaba
// de `Pet::ppMove[]` eran los PP â€” y solo cuando el golpe coincidia. O sea
// que elegir golpes en el menu no cambiaba nada en combate: salian siempre
// los de serie. (Reportado jugando.)
//
// Si el jugador NO ha elegido (`picked` false) manda el learnset, que es lo
// que `ppSync()` ya deja en ppMove, asi que solo hay que llevarse los PP.
static void btAplicaGolpes(Combatant &c, const uint16_t *mv,
                           const uint8_t *pp, bool picked) {
  if (!picked) {
    for (uint8_t i = 0; i < c.nMoves && i < 4; i++)
      if (mv[i] == c.moves[i]) c.usesLeft[i] = pp[i];
    return;
  }
  uint8_t n = 0;
  for (uint8_t i = 0; i < 4; i++) {
    if (!mv[i] || mv[i] >= MOVE_COUNT) continue;
    c.moves[n] = mv[i];
    c.usesLeft[n] = pp[i];
    n++;
  }
  // Dejarse sin ninguno es una eleccion valida en el menu; en combate eso es
  // forcejear, y de eso ya se encarga `anyUsable()`.
  c.nMoves = n;
}

// ---------------------------------------------------------------- arranque
// Sirve este bicho para uno de los TRES de la casa, a ese nivel?
static uint8_t gymTrPeso(int16_t d, uint8_t tipo, uint16_t tope) {
  const DexEntry &e = DEX_TBL[d];
  if (e.type1 != tipo && e.type2 != tipo) return 0;
  // NUNCA un legendario. `wildWeight` los suelta en cuanto tienes las
  // ocho insignias, y por ahi salia un don nadie de la casa sacando un
  // ARTICUNO —— que es lo que lo delato a la primera.
  if (e.rarity == R_LEGENDARIO) return 0;
  uint16_t bst = (uint16_t)e.bHp + e.bAtk + e.bDef + e.bSpA + e.bSpD + e.bSpe;
  return bst <= tope ? 1 : 0;
}

// Un bicho de la casa: del TIPO del gimnasio. Tiene regla PROPIA y ya no
// la de la hierba, que es de donde venian los dos problemas:
//
//   - de CUALQUIER region. `wildWeight` solo deja ver hasta la tuya, y
//     el charco de un tipo dentro de UNA region es minusculo: en un
//     gimnasio de hielo en Kanto casi no hay de donde elegir, asi que
//     salia siempre el mismo —— y ese era el legendario. Aqui no hay
//     mapa que respetar: son el equipo de un entrenador, no lo que se
//     cruza por la hierba.
//   - sin legendarios, nunca.
//   - y con un techo de fuerza atado a SU nivel, que es lo que separa al
//     primero de Brock (nivel 2) del tercero de Giovanni (42).
int16_t gymTrainerSpecies(uint8_t tipo, uint8_t nivel) {
  // 300 de base y +5 por nivel: a nivel 2 deja fuera casi todo lo
  // evolucionado, a nivel 42 entra un Nidoking de 505.
  uint16_t tope = (uint16_t)(300 + (uint16_t)nivel * 5);
  // Tres intentos subiendo el techo: hay tipos cuyo bicho mas flojo ya
  // pega fuerte (dragon, acero), y ahi vale mas sacar uno duro que no
  // sacar ninguno y caer en el reparto de la hierba.
  for (uint8_t intento = 0; intento < 3; intento++) {
    uint32_t total = 0;
    for (int16_t d = 1; d <= DEX_COUNT; d++) total += gymTrPeso(d, tipo, tope);
    if (total) {
      uint32_t r = (uint32_t)random((long)total);
      for (int16_t d = 1; d <= DEX_COUNT; d++) {
        uint32_t w = gymTrPeso(d, tipo, tope);
        if (r < w) return (int16_t)d;
        r -= w;
      }
    }
    tope = (uint16_t)(tope + 120);
  }
  return 0;
}

// El nivel al que sale el hueco `i` de un desafio. En revancha, TODOS
// al nivel del campeon de la region: un Brock de nivel 8 no es un
// entrenamiento, es una pausa.
static uint16_t btGymLevel(const GymEntry &g, uint8_t i) {
  if (!bt.rematch) return gymLevelAt(g, i);
  return gymChampLevel(pet.region);
}

// La piedra que le vale a esa especie, o IT_NONE.
static uint8_t piedraMegaDe(int16_t dex) {
  for (uint16_t i = 1; i < ITEM_COUNT; i++)
    if (ITEM_TBL[i].fx == IT_MEGA && megaFormOf(dex, ITEM_TBL[i].arg) >= 0)
      return (uint8_t)i;
  return IT_NONE;
}

// A QUIEN del equipo del lider le toca la piedra: el ULTIMO que pueda
// usarla, que casi siempre es el as. Si el as no tiene mega —— y 74 de
// los 117 lideres estan en ese caso —— la lleva el ultimo que si, en vez
// de quedarse ese gimnasio entero sin ella.
static int8_t gymMegaSlot(const GymEntry &g) {
  for (int8_t i = (int8_t)gymTeamSize(g) - 1; i >= 0; i--) {
    int16_t d = gymTeamAt(g, (uint8_t)i);
    if (d >= 1 && piedraMegaDe(d) != IT_NONE) return i;
  }
  return -1;
}

// Lo que lleva encima el rival que sale AHORA. `hueco` es su sitio en el
// equipo del lider (0 para todo lo demas).
//
// Esto antes solo se hacia en battleStart, asi que el segundo y el
// tercero de un lider salian con las manos VACIAS. Y como el as es el
// ultimo, la piedra del lider no se dio nunca: la condicion de alli
// ("queda uno") es falsa el primer turno de un lider de tres, que son
// todos. La mega del lider existia en el codigo y no habia pasado una
// sola vez.
static uint8_t btFoeItem(int16_t dex, int8_t gym, uint8_t hueco) {
  if (gym < 0) return rollWildItem(dex);
  // En REVANCHA la piedra entra siempre: el desafio ya esta superado y
  // lo que se busca es pelea, no una escalera de dificultad.
  if (bt.rematch || gym >= MEGA_DESDE_GIMNASIO) {
    const GymEntry &g = gymAt(pet.region, (uint8_t)gym);
    int8_t conPiedra = gymMegaSlot(g);
    if (conPiedra >= 0 && (uint8_t)conPiedra == hueco) return piedraMegaDe(dex);
  }
  return IT_NONE;
}

// El bicho del camino que saca el entrenador: el mismo reparto que la
// hierba (bioma, hora, insignias), o el tipo del gimnasio si es uno de
// los tres de la casa. Lo usan btNextFoe y la Ilusion (que necesita saber
// quien es el ULTIMO antes de que salga).
static void btSorteaDelCamino(int16_t &d, uint16_t &nv) {
  uint8_t hora = (uint8_t)(pet.lastSeenEpoch ? (pet.lastSeenEpoch / 3600) % 24 : 13);
  if (bt.gymTr >= 0) {
    const GymEntry &g = pet.gymHere();
    nv = gymTrainerLevel(g, (uint8_t)bt.gymTr);
    d = gymTrainerSpecies(g.type, (uint8_t)nv);
    if (d < 1) d = pet.pickWildSpecies(hora, true);
  } else {
    d = pet.pickWildSpecies(hora, true);
    nv = pet.wildLevelFor(d);
  }
}

// ILUSION (leva 70): la cara del ULTIMO del equipo del rival, si quedan
// mas detras del que sale (pbLastInTeam del Anil). Un salvaje no tiene
// equipo. El de un lider esta escrito; el de un entrenador del camino se
// sortea AQUI y se apunta, y btNextFoe lo saca de verdad cuando llegue:
// si no, la ilusion seria de un bicho que nunca aparece.
static void btIlusionRival(Combatant &c) {
  c.ilusDex = 0;
  c.ilusForm = 0;
  c.ilusShiny = false;
  if (bt.pvp || bt.foesLeft <= 1) return;
  if (bt.gym >= 0) {
    const GymEntry &g = gymAt(pet.region, (uint8_t)bt.gym);
    c.ilusDex = gymTeamAt(g, (uint8_t)(gymTeamSize(g) - 1));
    return;
  }
  if (bt.foeUltimo < 1) btSorteaDelCamino(bt.foeUltimo, bt.foeUltimoNv);
  c.ilusDex = bt.foeUltimo > 0 ? bt.foeUltimo : 0;
}

// Y la del jugador: el ultimo hueco del equipo que aun puede luchar. Si
// ese es el mismo que entra, no hay ilusion.
static void btIlusionMia(Combatant &c, uint8_t entra) {
  c.ilusDex = 0;
  c.ilusForm = 0;
  c.ilusShiny = false;
  if (bt.pvp) return;
  for (int8_t i = TEAM_SLOTS - 1; i >= 0; i--) {
    bool principal = (uint8_t)i == pet.mainSlot;
    const StoredMon &m = pet.team[i];
    bool vivo = principal ? (!pet.isEgg() && pet.hpLeft > 0)
                          : (m.used() && m.hpLeft > 0);
    if (!vivo) continue;
    if ((uint8_t)i == entra) return;
    c.ilusDex = principal ? pet.speciesId : m.species;
    c.ilusForm = principal ? pet.form : m.form;
    c.ilusShiny = principal ? pet.shiny : (m.flags & SM_SHINY) != 0;
    return;
  }
}

void battleStart(int16_t foeDex, uint16_t foeLevel, int8_t gym,
                 bool foeShiny, int8_t trainer, int8_t gymTr,
                 bool revancha) {
  if (pet.isEgg() || foeDex < 1 || foeDex > DEX_COUNT) return;
  if (!pet.canBattle()) return;   // muy tocado: primero hay que cuidarlo
  bt.rematch = revancha && gym >= 0;   // solo un desafio se revancha
  bt.insigniaDada = false;
  bt.gym = gym;                   // -1 = salvaje; si no, indice en GYM_TBL
  bt.trainer = trainer;           // -1 = hierba; si no, quien te reta
  bt.gymTr = trainer >= 0 ? gymTr : -1;   // y si es de la casa, cual
  // De un entrenador no se huye: ni de un gimnasio ni del que te para
  // en el camino. El boton ni sale.
  bt.b.isTrainer = (gym >= 0 || trainer >= 0);
  bt.b.fleeTries = 0;
  bt.b.fled = false;
  bt.expWon = 0;
  bt.lvlWon = 0;
  bt.lvlAntes = 0;
  bt.winDex = 0;
  bt.winSlot = 0xFF;
  bt.foeIdx = 0;
  bt.foeUltimo = 0;     // el ultimo del camino aun no se ha sorteado
  bt.foeUltimoNv = 0;
  bt.foesLeft = gym >= 0 ? gymTeamSize(gymAt(pet.region, (uint8_t)gym))
              : trainer >= 0 ? trainerAt((uint8_t)trainer).team : 1;
  // El escenario se elige AQUI y no en cada fotograma: el equipo de un lider
  // cambia de bicho entre asaltos y el fondo tiene que quedarse quieto.
  bt.bg = gym >= 0 ? (int8_t)KANTO_BG[gym < GYM_TOTAL ? gym : GYM_TOTAL - 1]
        : trainer >= 0 ? (int8_t)bgPick(CITY_BGS, (uint32_t)random(1000))
        : bgForWild(foeDex);
  bgImg.load((uint8_t)bt.bg);   // si no hay fichero, se pinta el cielo

  Combatant mio, rival;
  // Los genes y el entrenamiento ANTES de init(): el bicho criado pelea con los
  // suyos, no con los de serie, y init() ya deja los stats calculados.
  mio.geneHp = pet.geneHp; mio.geneAtk = pet.geneAtk; mio.geneDef = pet.geneDef;
  mio.geneSpa = pet.geneSpa; mio.geneSpd = pet.geneSpd; mio.geneSpe = pet.geneSpe;
  mio.trHp = pet.trHp; mio.trAtk = pet.trAtk; mio.trDef = pet.trDef;
  mio.trSpa = pet.trSpa; mio.trSpd = pet.trSpd; mio.trSpe = pet.trSpe;
  // El vinculo viaja al combate: es lo unico que decide si te hace caso.
  mio.bond = pet.bond;
  // PELEA al techo de la region, no a su nivel real: conserva nivel y
  // EXP, pero un Charizard de 70 no arrasa el primer gimnasio de una
  // region nueva. Fuera del combate sigue siendo de 70.
  mio.init(pet.speciesId, pet.battleLevel(), pet.shiny, pet.form);
  // El salvaje sale con la forma de ESTA region: en Alola, un Vulpix de
  // Alola. Es lo mismo que hace Pet::catchMon al guardarlo.
  rival.init(foeDex, foeLevel, foeShiny,
             formForRegion(foeDex, pet.region, (uint8_t)random(100)));
  // El sexo: el tuyo lo traes de casa, el del salvaje se sortea ahora.
  // Lo pide Rivalry, que pega mas al del mismo sexo.
  mio.sexo = (pet.speciesId >= 1 && pet.speciesId <= DEX_COUNT &&
              DEX_TBL[pet.speciesId].femPct == 255)
             ? 2 : (pet.female ? 1 : 0);
  rival.rollSexo();
  mio.alliesFallen = btCaidos();   // lo que ya habia caido antes de empezar

  // La habilidad. La del bicho salio en su huevo; la del salvaje se sortea
  // aqui entre las suyas, para que dos Geodude no sean el mismo Geodude.
  mio.ability = pet.ability;
  {
    const uint16_t *a = ABIL_OF[foeDex];
    rival.ability = (a[1] && random(2)) ? a[1] : (a[0] ? a[0] : a[1]);
  }

  // Lo que lleva el RIVAL.
  //
  // Un salvaje SI lleva cosas: el Anil lo dice especie por especie
  // (WildItemCommon/Uncommon/Rare) y hasta ahora no se usaba. Sin
  // esto, Frisk, Pickpocket, Anticipation, Ladron y Desarme solo
  // tenian con que trabajar frente a un lider — y encima quitaba la
  // unica forma de conseguir objetos peleando en la hierba.
  // El PRIMERO que saca el lider es el hueco 0; de los demas se encarga
  // btNextFoe con la misma funcion, que es lo que faltaba.
  rival.item = btFoeItem(foeDex, gym, 0);

  // Los PP NO se rellenan al empezar: vienen de donde los dejo el combate
  // anterior. Eso es todo lo que hace que el numero signifique algo; si se
  // restaurasen solos, un Tackle de 35 no se agotaria nunca. Se recuperan
  // durmiendo y con bayas (ver Pet::ppRestore).
  pet.ppSync();
  btAplicaGolpes(mio, pet.ppMove, pet.ppLeft, pet.movesPicked);

  // Y los PS igual: se entra con los que se tengan, no con la barra llena.
  pet.hpSync();
  if (pet.hpLeft > 0 && pet.hpLeft <= mio.hpMax) mio.hp = pet.hpLeft;

  // Lo que lleve encima entra al combate con el. Las bayas se GASTAN, y de
  // eso se encarga battleClose() mirando si volvio con las manos vacias.
  mio.item = pet.heldItem;
  for (uint8_t i = 0; i < 6; i++)
    bt.itemIni[i] = bt.itemAhora[i] =
        btIsMain(i) ? pet.heldItem : pet.team[i].item;
  bt.itemGastado = 0;
  bt.foeItemIni = rival.item;

  btIlusionMia(mio, pet.mainSlot);
  btIlusionRival(rival);
  bt.b.start(mio, rival);
  // La musica dice contra QUIEN peleas antes de que lo leas: es la mitad de
  // lo que hace que un lider se sienta distinto de una rata del camino.
  musicPlay(gym >= GYM_BADGES + 4 ? MUS_CHAMP
          : gym >= GYM_BADGES     ? MUS_ELITE
          : gym >= 0              ? MUS_GYM
          : trainer >= 0          ? MUS_TRAINER
                                  : MUS_WILD);
  cryPlay((uint16_t)foeDex);
  bt.mySlot = pet.mainSlot;
  bt.partyScroll = 0;
  bt.mustSwitch = false;
  btSyncReserves();
  bt.open = true;
  bt.phase = BP_INTRO;
  bt.until = millis() + BT_INTRO_MS;
  bt.startedAt = millis();
  bt.fxType = T_NONE;
  bt.fxSup = 0xFF;
  bt.fxMove = 0;
  bt.sel = 0;
  bt.introAbil = 0;
  bt.secondPending = false;
  bt.itemPick = 0;          // nadie entra con un objeto a medio usar
  bt.megaSide = BT_PLAYER;
  // El escondite de Excavar/Vuelo/Buceo es del combate anterior: sin
  // esto, el que entra al siguiente se pasa 260 ms "volviendo" de un
  // agujero en el que nunca estuvo.
  for (uint8_t s = 0; s < 2; s++) {
    bt.hidWas[s] = bt.hidKind[s] = 0;
    bt.hidAt[s] = 0;
    bt.cargaAte[s] = 0;
    bt.cargaAnim[s] = false;
  }
  // A animacao do combate anterior nao entra neste: `animFim` segura TUDO
  // (o corte do Drought e da carga do Fly), e um valor velho congelava a
  // abertura do proximo combate ate ele passar.
  bt.anim = false;
  bt.animFim = 0;
  // El estado de la CAPTURA tambien se reinicia. `foeInBall` se quedaba
  // puesto al atrapar a uno, y como lo unico que lo apagaba era tirar otra
  // bola, el rival del combate SIGUIENTE nacia invisible y no aparecia
  // hasta que le tirabas una. `caught` y los toques, por la misma razon.
  bt.foeInBall = false;
  bt.caught = false;
  bt.taps = 0;
  bt.wobble = 0;
  bt.ballId = 0;
  // En un gimnasio del primer escalon el bicho SI obedece: perder la
  // insignia por una tirada de dados seria rabia, no dificultad.
  bt.b.strictOrders = (gym >= 0 && gym < 1);
  if (gym >= 0)
    snprintf(bt.msg, sizeof(bt.msg), "%s sends %s!", gymAt(pet.region, (uint8_t)gym).leader,
             dexName(bt.b.side[BT_FOE].pareceDex()));
  else if (trainer >= 0)
    snprintf(bt.msg, sizeof(bt.msg), "%s sends %s!",
             trainerAt((uint8_t)trainer).name, dexName(bt.b.side[BT_FOE].pareceDex()));
  else
    // Ya no lleva asterisco: la estrella sale en la chapa de PS (btHpBar), que
    // es donde esta en los juegos y, al contrario que el anuncio, se queda ahi
    // todo el combate.
    snprintf(bt.msg, sizeof(bt.msg), "Wild %s appeared!", dexName(foeDex));

  // La cache va por especie Y por si es shiny: mirando solo la especie,
  // pelear contra un Pikachu normal y luego contra uno shiny reutilizaba el
  // sprite del primero y el shiny salia del color de siempre.
  // La clave era `foeDex + 1000` para el shiny, y con 1025 especies eso
  // CHOCA: un Pikachu shiny daba 1025, que es Pecharunt de verdad, y el
  // siguiente combate reutilizaba el sprite equivocado. Se desplaza por
  // encima de la dex entera, que es lo unico que no puede colisionar.
  int16_t quiero = rival.shiny ? (int16_t)(foeDex + DEX_COUNT + 1) : foeDex;
  // Y la FORMA tambien va en la clave, o un Meowth de Galar reutilizaria
  // el sprite del de Kanto del combate anterior.
  int16_t rfila = formRow(foeDex, rival.form);
  if (rfila >= 0) quiero = (int16_t)(-2 - rfila);
  if (foePmdFor != quiero) {
    foePmd.unload();
    // El sexo decide el dibujo en las 98 especies que traen hembra
    // propia. Al rival se lo sorteo rollSexo() al aparecer.
    bool ok = rfila >= 0 ? foePmd.loadForm(rfila)
                         : foePmd.load(foeDex, rival.shiny,
                                       rival.sexo == 1);
    foePmdFor = ok ? quiero : -1;
    Serial.printf("sprite rival: dex=%d shiny=%d cargado=%d\n",
                  (int)foeDex, (int)rival.shiny, (int)ok);
  }
  sfxPlay(SFX_TAP);
}

// La REVANCHA de un desafio ya superado. Arranca por el primero de su
// equipo, como el desafio de verdad —— lo que cambia lo pone `bt.rematch`:
// todos al nivel del campeon, piedra mega siempre y mejor botin.
void rematchStart(uint8_t gym) {
  if (gym >= pet.gymCount()) return;
  const GymEntry &g = gymAt(pet.region, gym);
  int16_t d = gymTeamAt(g, 0);
  if (d < 1) return;
  battleStart(d, gymChampLevel(pet.region), (int8_t)gym, false, -1, -1, true);
}

// Los PP gastados se devuelven al bicho en cuanto se gastan, no al cerrar:
// si se corta la luz a mitad de combate no se regalan los golpes ya usados.
// Devuelve los PP gastados a su hueco. Busca por ID de golpe y no por
// indice: con los golpes elegidos por el jugador, los huecos vacios se
// compactan al entrar al combate y el indice ya no cuadra.
static void btGuardaGolpes(const Combatant &c, const uint16_t *mv, uint8_t *pp) {
  for (uint8_t i = 0; i < 4; i++) {
    if (!mv[i]) continue;
    for (uint8_t j = 0; j < c.nMoves && j < 4; j++)
      if (c.moves[j] == mv[i]) { pp[i] = c.usesLeft[j]; break; }
  }
}

static void btSavePP() {
  if (bt.pvp) return;   // entre placas os PP sao do combate, nao do save
  btGuardaGolpes(bt.b.side[BT_PLAYER], pet.ppMove, pet.ppLeft);
}

// ------------------------------------------------------------- relevos
//
// El bicho que sale a pista puede ser el PRINCIPAL (que vive en el propio
// Pet, con sus barras) o cualquier otro del equipo (que vive en su
// StoredMon). `bt.mySlot` dice cual esta fuera; al entrar y al salir hay que
// volcar los PS y los PP a su sitio, o el combate no deja rastro.

static bool btIsMain(uint8_t slot) { return slot == pet.mainSlot; }
static_assert(TEAM_SLOTS == 6, "BattleUi::itemIni/itemAhora van a 6 huecos");

// Un combatiente a partir de un hueco del equipo. Es el mismo montaje que
// hace battleStart con el Pet, pero leyendo del StoredMon.
static Combatant btFromSlot(uint8_t slot) {
  Combatant c;
  const StoredMon &m = pet.team[slot];
  c.geneHp = m.geneHp; c.geneAtk = m.geneAtk; c.geneDef = m.geneDef;
  c.geneSpa = m.geneSpa; c.geneSpd = m.geneSpd; c.geneSpe = m.geneSpe;
  c.trHp = m.trHp; c.trAtk = m.trAtk; c.trDef = m.trDef;
  c.trSpa = m.trSpa; c.trSpd = m.trSpd; c.trSpe = m.trSpe;
  c.bond = m.bond;
  // Al techo de la region, igual que el principal: si no, meter al suplente
  // de nivel 70 en el primer gimnasio saltaria el cap por la puerta de atras.
  uint16_t nv = Pet::levelOf(m);
  uint16_t tope = pet.levelCap();
  c.init(m.species, nv > tope ? tope : nv, (m.flags & SM_SHINY) != 0);
  c.ability = m.ability;
  c.item = m.item;
  // los suyos, si su dueno los eligio (mismo criterio que el principal)
  btAplicaGolpes(c, m.moves, m.ppLeft, (m.flags & SM_MOVES_PICKED) != 0);
  if (m.hpLeft > 0 && m.hpLeft <= c.hpMax) c.hp = m.hpLeft;
  return c;
}

// Guarda lo que le quedo al que sale de pista, en el sitio que le toque.
static void btStoreActive() {
  if (bt.pvp) return;   // entre placas nada volta ao time
  const Combatant &c = bt.b.side[BT_PLAYER];
  // El objeto, lo primero y para los dos casos. Lo que lleva AHORA se
  // apunta para si vuelve a salir; lo unico que se pierde de verdad es lo
  // que se gasto y era SUYO (comerse una baya robada no le quita la suya).
  if (bt.mySlot < 6) {
    uint8_t bit = (uint8_t)(1u << bt.mySlot);
    bt.itemAhora[bt.mySlot] = c.item;
    if (c.itemComido != IT_NONE && c.itemComido == bt.itemIni[bt.mySlot])
      bt.itemGastado |= bit;
    if (c.item != IT_NONE && c.item == bt.itemIni[bt.mySlot])
      bt.itemGastado &= (uint8_t)~bit;      // Reciclaje o Cosecha se lo devolvieron
  }
  if (btIsMain(bt.mySlot)) {
    btSavePP();
    pet.hpLeft = c.hp;
    return;
  }
  StoredMon &m = pet.team[bt.mySlot];
  // Recortado a SU maximo, que es el de su forma normal. Siete megas de
  // la tabla suben los PS base (el Anil mete versiones Dynamax entre las
  // suyas: Pikachu 35->55, Eevee 55->65), asi que un suplente que
  // megaevolucione y acabe entero vuelve con mas vida de la que le cabe.
  // El principal ya lo recortaba Pet::afterBattle; aqui faltaba.
  uint16_t tope = hpAtLevelForm(m.species, Pet::levelOf(m), m.trHp, m.form);
  m.hpLeft = (tope && c.hp > tope) ? tope : c.hp;
  btGuardaGolpes(c, m.moves, m.ppLeft);
  // El objeto ya NO se escribe aqui: `m.item = c.item` hacia que un Desarme
  // del rival le borrase el objeto al suplente para siempre. Lo que se
  // gasto de verdad lo cobra battleClose(), mirando bt.itemGastado.
}

// Puede salir ese hueco? Tiene que existir, tener PS y no ser el que ya esta.
static bool btCanSend(uint8_t slot) {
  if (bt.pvp) return pvpL[pvpEu].podeEntrar(slot);
  if (slot >= TEAM_SLOTS || slot == bt.mySlot) return false;
  if (btIsMain(slot)) return !pet.isEgg() && pet.hpLeft > 0;
  const StoredMon &m = pet.team[slot];
  return m.used() && m.hpLeft > 0;
}

static uint8_t btReserveCount() {
  uint8_t n = 0;
  for (uint8_t i = 0; i < TEAM_SLOTS; i++) if (btCanSend(i)) n++;
  return n;
}

// Mantiene al dia lo que el motor necesita saber del banquillo: con relevos,
// caer no acaba el combate (ver Battle::knockOut).
static void btSyncReserves() {
  if (bt.pvp) { pvpSincroniza(bt.b, pvpL); return; }
  bt.b.playerReserves = btReserveCount() > 0;
}

// Cuantos del equipo estan tumbados AHORA. Lo pide Ultimo Adios, que pega
// +50 por cada companero caido: es el golpe que convierte ir perdiendo en
// una amenaza, y sin esta cuenta se queda en 50 de potencia para siempre.
//
// Vive aqui y no en el motor porque el motor no conoce el equipo —— solo
// ve a los dos que estan en pista.
static uint8_t btCaidos() {
  if (bt.pvp) return pvpL[pvpEu].caidos();
  uint8_t n = 0;
  for (uint8_t i = 0; i < TEAM_SLOTS; i++)
    if (pet.team[i].used() && pet.team[i].hpLeft == 0) n++;
  return n;
}

static void btSwitchTo(uint8_t slot, bool porGusto = false) {
  if (bt.pvp || !btCanSend(slot)) return;   // entre placas: pvpTrocaAgora
  // Regenerator ANTES de guardar: cura al que se va, y btStoreActive
  // se lleva ya los PS buenos a su ficha. Al reves la cura se perderia.
  bool regenero = bt.b.regenOnSwitch(BT_PLAYER) > 0;
  btStoreActive();
  bt.mySlot = slot;
  Combatant c = btIsMain(slot) ? bt.b.side[BT_PLAYER] : btFromSlot(slot);
  if (btIsMain(slot)) {
    // el principal se reconstruye del Pet, que es donde vive de verdad
    c = Combatant();
    c.geneHp = pet.geneHp; c.geneAtk = pet.geneAtk; c.geneDef = pet.geneDef;
    c.geneSpa = pet.geneSpa; c.geneSpd = pet.geneSpd; c.geneSpe = pet.geneSpe;
    c.trHp = pet.trHp; c.trAtk = pet.trAtk; c.trDef = pet.trDef;
    c.trSpa = pet.trSpa; c.trSpd = pet.trSpd; c.trSpe = pet.trSpe;
    c.bond = pet.bond;
    c.init(pet.speciesId, pet.battleLevel(), pet.shiny);
    c.ability = pet.ability;
    pet.ppSync();
    btAplicaGolpes(c, pet.ppMove, pet.ppLeft, pet.movesPicked);
    pet.hpSync();
    if (pet.hpLeft > 0 && pet.hpLeft <= c.hpMax) c.hp = pet.hpLeft;
  }
  // Sale con lo que lleve EN ESTE COMBATE, no con lo de la ficha: si le
  // tiraron el objeto y vuelve a entrar, sigue sin el.
  c.item = slot < 6 ? bt.itemAhora[slot] : (uint8_t)IT_NONE;
  c.alliesFallen = btCaidos();
  btIlusionMia(c, slot);
  bt.b.sendOut(BT_PLAYER, c, porGusto);
  if (regenero) snprintf(bt.msg, sizeof(bt.msg), "It regained health!");
  btSyncReserves();
  // el sprite y el grito tambien cambian
  {
    int16_t fr = formRow(c.dex, c.form);
    if (fr >= 0) pmd.loadForm(fr);
    else pmd.load(c.dex, c.shiny, c.sexo == 1);
  }
  cryPlay((uint16_t)c.dex);
  snprintf(bt.msg, sizeof(bt.msg), "Go, %s!", dexName(c.pareceDex()));
  bt.sendAt = millis();                // la bola, en btSendoutFx
  bt.sendSide = BT_PLAYER;
  // Entrou com Drought & cia: a animacao do campo depois da bola abrir.
  if (btAnimDoCampo(BT_PLAYER, millis() + 900))
    snprintf(bt.msg, sizeof(bt.msg), "Go, %s! (%s)", dexName(c.pareceDex()),
             abilAt(bt.b.startAbil[BT_PLAYER]).name);
}

// O GOLPE cuja animacao do Anil representa uma habilidade que muda o
// TERRENO ao entrar, ou nullptr.
//
// As de CLIMA (Drought, Sand Stream...) ja nao passam por aqui. Tocavam o
// golpe equivalente (o Sunny Day), e nao e isso o que o Anil faz: uma
// habilidade de clima chama pbStartWeather, que mostra a animacao DO
// CLIMA —— a mesma que agora fica em laco por cima do combate
// (btClimaQuadro). Basta o clima mudar para ela aparecer.
static const char *btGolpeDoCampo(uint16_t habil) {
  if (!habil) return nullptr;
  const AbilityEntry &a = abilAt(habil);
  switch (a.fx) {
    case AB_SET_TERRAIN:
      return a.arg == TR_ELEC ? "ELECTRICTERRAIN" : a.arg == TR_GRASS ? "GRASSYTERRAIN"
           : a.arg == TR_MISTY ? "MISTYTERRAIN" : a.arg == TR_PSY ? "PSYCHICTERRAIN"
           : nullptr;
    case AB_HADRON: return "ELECTRICTERRAIN";
    default: return nullptr;
  }
}

// Se a habilidade que ACABOU de disparar no `lado` muda o campo, carrega a
// animacao do golpe equivalente (do lado de quem tem a habilidade) e a
// marca para comecar em `inicio`. Devolve quanto ela dura (0 = nenhuma).
static uint32_t btAnimDoCampo(uint8_t lado, uint32_t inicio) {
  const char *g = btGolpeDoCampo(bt.b.startAbil[lado]);
  if (!g) return 0;
  uint16_t mv = 0;
  for (uint16_t m = 1; m < MOVE_COUNT; m++)
    if (!strcmp(MOVE_TBL[m].name, g)) { mv = m; break; }
  if (!mv || !animMove.load(mv, MOVE_TBL[mv].name, lado != btAbajo())) return 0;
  bt.anim = true;
  bt.fxAt = inicio;
  bt.animFim = inicio + (uint32_t)animMove.nq * 50;
  return (uint32_t)animMove.nq * 50;
}

// El premio por TUMBAR A UNO. Va aparte del cierre del combate porque un
// entrenador saca varios: cada uno que cae paga lo suyo, como en los juegos.
// No es static: la bateria lo llama para tumbar rivales con OTRO bicho en
// pista y mirar a quien da la victoria.
void btFoeDefeated() {
  if (bt.pvp) return;   // entre placas nao ha EXP nem EV
  int16_t caido = bt.b.side[BT_FOE].dex;
  uint32_t e = Pet::expFromFoe(caido, bt.b.side[BT_FOE].level);
  // A QUIEN ESTA EN PISTA, que puede no ser el principal. Antes iba
  // siempre al principal: peleando con un suplente, el que se lo
  // curraba no se llevaba nada y el que miraba desde el banquillo se
  // llevaba la experiencia Y el EV. Es justo lo contrario de lo que
  // uno espera al cambiar de bicho para no estropearle el reparto.
  // Se apunta ANTES de sumar: el nivel de partida es el del primer rival
  // que cae, no el del ultimo.
  //
  // E se quem derruba este NAO e quem derrubou os anteriores (o primeiro
  // caiu e o seguinte entrou e venceu), o vencedor passa a ser ELE: a tela
  // de vitoria mostrava o que tinha desmaiado. A EXP e os niveis da tela
  // recomecam com ele, porque sao os que ELE ganhou; os do anterior ja
  // foram entregues a quem os ganhou.
  if (!bt.expWon || bt.winSlot != bt.mySlot) {
    bt.lvlAntes = btIsMain(bt.mySlot) ? pet.level()
                                      : Pet::levelOf(pet.team[bt.mySlot]);
    bt.winDex = bt.b.side[BT_PLAYER].dex;
    bt.winSlot = bt.mySlot;
    bt.expWon = 0;
    bt.lvlWon = 0;
  }
  bt.expWon += e;
  uint8_t subio = pet.gainExpOf(bt.mySlot, e);
  bt.lvlWon = (uint8_t)(bt.lvlWon + subio);
  // Los EVs salen de A QUIEN derrotas: cada especie entrena aquello en lo
  // que destaca. Es lo que convierte elegir el encuentro en entrenar.
  uint8_t ev = Pet::evYieldOf(caido);
  pet.gainEVOf(bt.mySlot, ev, 1);
  // El resto del equipo se lleva su parte: poco, pero que no se queden
  // clavados mientras el titular sube y cambiar de activo deje de ser opcion.
  pet.teamShareExp(e, bt.mySlot);
  Serial.printf("+%u exp (nv.%u)  +1 EV %s%s\n", e, pet.level(),
                Pet::evName(ev), subio ? "  SUBIO DE NIVEL" : "");
}

// Saca el siguiente bicho del entrenador. Se elige AQUI y no al empezar
// porque asi sale del mismo reparto que la hierba (bioma, hora, insignias)
// sin tener que guardar una lista de tres especies en ninguna parte.
// No es static para que la bateria pueda recorrer el equipo de un lider
// y mirar QUIEN sale con la piedra mega: por el camino normal eso son
// tres combates ganados, y la piedra se la lleva el ultimo.
void btNextFoe() {
  int16_t d;
  uint16_t nv;
  bt.foeIdx++;
  if (bt.gym >= 0) {
    // Un lider tiene equipo FIJO, con su as el ultimo.
    const GymEntry &g = gymAt(pet.region, (uint8_t)bt.gym);
    d = gymTeamAt(g, bt.foeIdx);
    nv = btGymLevel(g, bt.foeIdx);
  } else if (bt.foesLeft == 1 && bt.foeUltimo > 0) {
    // El ultimo ya lo sorteo una Ilusion: tiene que ser ESE.
    d = bt.foeUltimo;
    nv = bt.foeUltimoNv;
  } else {
    // El de un entrenador sale del mismo reparto que la hierba (bioma,
    // hora, insignias), asi no hay que guardar una lista en ninguna parte;
    // los TRES de la casa sacan del tipo del gimnasio y pelean a SU nivel
    // (son el porton del lider, y un porton que se adapta a ti no es un
    // porton). Ver btSorteaDelCamino.
    btSorteaDelCamino(d, nv);
  }
  if (d < 1) { bt.foesLeft = 0; bt.phase = BP_END; return; }
  Combatant nuevo;
  nuevo.init(d, nv);
  const uint16_t *a = ABIL_OF[d];
  nuevo.ability = (a[1] && random(2)) ? a[1] : (a[0] ? a[0] : a[1]);
  // Lo que lleva encima. Esto FALTABA: el segundo y el tercero de un
  // lider salian sin nada, y con ellos se iba la piedra mega —— que le
  // toca justamente al ultimo.
  nuevo.item = btFoeItem(d, bt.gym, (uint8_t)bt.foeIdx);
  btIlusionRival(nuevo);
  // Por sendOut, como o seu: e ele que dispara as habilidades de entrada.
  // Com a atribuicao direta, um segundo bicho com Drought entrava e o sol
  // nao saia.
  bt.b.sendOut(BT_FOE, nuevo, false);
  cryPlay((uint16_t)d);
  bt.sendAt = millis();                // la bola, en btSendoutFx
  bt.sendSide = BT_FOE;
  bt.b.over = false;
  bt.b.winner = 0xFF;
  bt.secondPending = false;
  // NO con BP_SAY. BP_SAY no es "ensena esta frase": es "X uso Y!", y su
  // paso del reloj llama a btResolve(), que TIRA el golpe que siguiera
  // engatillado en bt.acting/bt.pendingSlot. Como aqui la frase es "el
  // entrenador saca al siguiente", lo que salia a los 1200 ms era el
  // ultimo golpe del jugador cayendo encima del bicho recien entrado:
  // el ataque de regalo por cada relevo.
  //
  // Ya lo di por arreglado una vez matando bt.secondPending, y era cierto
  // pero era OTRA fuga con el mismo sintoma; esta sobrevivio porque el
  // golpe no venia del pendiente, venia de la fase.
  //
  // Lo correcto es el mismo patron que btSwitchTo usa para su "Go, X!":
  // BP_MSG con la cola de avisos saltada (extraStep 9) y el desgaste de
  // fin de turno ya dado por cobrado (endStep 2), que desemboca en
  // BP_CHOOSE. O sea: se lee la frase y te toca elegir.
  bt.phase = BP_MSG;
  bt.extraStep = 9;
  bt.endStep = 2;
  bt.until = millis() + 1200;
  snprintf(bt.msg, sizeof(bt.msg), "%s sends %s!",
           bt.gym >= 0 ? gymAt(pet.region, (uint8_t)bt.gym).leader
                       : trainerAt((uint8_t)bt.trainer).name,
           dexName(d));
  {
    // Entrou com Drought & cia: a animacao do campo depois da bola, e a
    // frase espera por ela.
    uint32_t dura = btAnimDoCampo(BT_FOE, millis() + 900);
    if (dura && millis() + 900 + dura > bt.until) bt.until = millis() + 900 + dura;
  }
  // El sprite del nuevo. Sin esto se quedaba el del anterior en pantalla.
  foePmd.unload();
  // Aqui el rival sale de la tabla del entrenador y todavia no se le
  // ha sorteado el sexo, asi que va con el dibujo normal.
  foePmdFor = foePmd.load(d, false) ? d : -1;
  sfxPlay(SFX_TAP);
}

// Lo que te llevas al ganar. Devuelve true si hay algo que ENSENAR.
//
// Son dos cosas distintas y las dos tienen que verse:
//   1. lo que llevaba encima el salvaje — ahora que los salvajes
//      llevan cosas, dejarlo caer al suelo seria raro
//   2. lo que suelta el combate (rollBattleDrop), que ya existia pero
//      solo salia por el puerto serie: el jugador nunca se enteraba
// Lo que ganas se lee UNA vez y no se puede volver atras, asi que el
// tiempo lo manda el que lee, no el que escribe: 1500 ms daba para verlo
// pasar, no para leerlo.
#define BT_LOOT_MS 2600
static bool btCobrarBotin() {
  bt.loot[0] = bt.loot[1] = 0;
  bt.lootShown = 0;
  if (bt.b.winner != BT_PLAYER) return false;

  // 1. Lo que llevaba. Solo de un SALVAJE: lo del lider es suyo, y
  // quitarle la piedra mega al ganarle seria cobrarse dos veces.
  uint8_t suyo = bt.b.side[BT_FOE].item;
  // Solo si sigue llevando LO SUYO: tras un Trapicheo lo que lleva es tuyo
  // (y eso ya vuelve solo), y regalarlo aqui lo duplicaria.
  if (bt.gym < 0 && bt.trainer < 0 && suyo != IT_NONE &&
      suyo == bt.foeItemIni && pet.bag[suyo] < 99) {
    pet.bag[suyo]++;
    bt.loot[0] = suyo;
  }
  // 2. Y lo que suelte el combate.
  uint8_t extra = pet.rollBattleDrop(bt.trainer >= 0 || bt.gym >= 0);
  if (extra) bt.loot[bt.loot[0] ? 1 : 0] = extra;
  // La REVANCHA paga mejor: es lo unico que queda por hacer despues de
  // coronarse, y sin premio seria pegarse por deporte. Se tira UNA vez
  // mas, en el hueco que quede libre —— no se duplica el que ya salio.
  if (bt.rematch && (!bt.loot[0] || !bt.loot[1])) {
    uint8_t otro = pet.rollBattleDrop(true);
    if (otro) bt.loot[bt.loot[0] ? 1 : 0] = otro;
  }

  if (!bt.loot[0]) return false;
  snprintf(bt.msg, sizeof(bt.msg), "Got %s!", itemAt(bt.loot[0]).name);
  bt.until = millis() + BT_LOOT_MS;
  return true;
}

static void battleClose() {
  if (bt.pvp) { pvpFecha(); return; }
  // Quien se acaba de capturar, apuntado ANTES de tocar nada: de aqui para
  // abajo el combatiente del rival se da por perdido.
  int16_t capturadoDex = bt.caught ? bt.b.side[BT_FOE].dex : 0;
  // Lo primero: volcar al que quedo en pista a SU sitio (el Pet si es el
  // principal, su StoredMon si es un suplente). Sin esto, pelear con un
  // suplente no dejaba rastro y volvia entero.
  btStoreActive();
  bool mainFuera = btIsMain(bt.mySlot);
  // Los objetos: se pierde SOLO lo que se gasto (baya, Banda Focus...), sea
  // del principal o de un suplente y este o no en pista al acabar. Lo que
  // le quitaron vuelve, que es lo que hace el Anil al cerrar el combate
  // (151_Battle_StartAndEnd.rb:510). Antes se miraba "volvio con las manos
  // vacias", y un Desarme del rival borraba el objeto para siempre.
  for (uint8_t i = 0; i < 6; i++) {
    if (!(bt.itemGastado & (1u << i))) continue;
    if (btIsMain(i)) pet.heldItem = 0;
    else pet.team[i].item = 0;
  }
  bt.itemGastado = 0;
  // afterBattle cobra la energia, guarda los PS y, si cayo, lo desmaya. Los
  // PS que le importan son los del PRINCIPAL, este o no en pista: si se pasa
  // los del suplente, el que crias sale herido de un combate que no peleo.
  pet.hpSync();
  pet.afterBattle(pet.hpLeft);
  if (bt.b.winner == BT_PLAYER) {
    // (la experiencia y los EVs ya los cobro btFoeDefeated con cada caida;
    //  el BOTIN lo cobro btCobrarBotin al entrar en BP_LOOT, que es donde
    //  se puede ENSENAR — antes se cobraba aqui y solo salia por serie)
    for (uint8_t i = 0; i < 2; i++)
      if (bt.loot[i]) Serial.printf("encontro: %s\n", itemAt(bt.loot[i]).name);
    // Y la progresion por Kanto: un salvaje suma al porton, un lider da la
    // insignia (y con ella, mas especies sueltas por ahi).
    if (bt.gym >= 0) {
      // La REVANCHA no es progresion —— winGym se va de vacio con la
      // region hecha ——, pero SI es un logro: es lo unico que queda por
      // ganar una vez coronado.
      if (bt.rematch) pet.winRematch();
      // Se ja foi dada na tela da insignia, nao se da de novo.
      if (!bt.insigniaDada) {
        pet.winGym();
        // La insignia pisa a la fanfarria de victoria: es la que importa.
        if (bt.gym < GYM_BADGES) jinglePlay(JIN_BADGE);
      }
      Serial.printf("DESAFIO SUPERADO: %s (%u/%u)  premio: %u x %s\n",
                    gymAt(pet.region, (uint8_t)bt.gym).leader,
                    pet.badges, pet.gymCount(), pet.lastBallCount,
                    itemAt(pet.lastBallId).name);
    } else {
      pet.winWild();
      // Ganar a un entrenador paga una bola de propina, encima de lo que
      // suelte el combate. Es lo que compensa aguantar a varios seguidos y
      // que su bicho no se pueda capturar.
      if (bt.trainer >= 0) {
        pet.giveBall(pet.badges >= 4 ? 1 : 0);
        Serial.printf("propina: %s\n", itemAt(pet.lastBallId).name);
      }
      // Y si era uno de los TRES de la casa, sube el porton del lider.
      // Va DESPUES de winWild(), que ya apunto la victoria: winGymTrainer
      // solo mueve el porton.
      if (bt.gymTr >= 0) {
        pet.winGymTrainer();
        Serial.printf("gimnasio: %u/%u de la casa\n",
                      pet.gymTrainers, GYM_TRAINERS);
      }
    }
  }
  // El boton de cazar se enfria DESDE QUE SALES, no desde que lo pulsas:
  // si contara desde el toque, un combate largo lo dejaria listo antes de
  // acabarlo y se podrian encadenar sin pausa.
  pet.huntUntil = millis() + HUNT_COOLDOWN_MS;
  bt.gym = -1;
  bt.trainer = -1;
  bt.gymTr = -1;
  bt.open = false;
  bt.bg = -1;
  bt.mustSwitch = false;
  foePmd.unload();
  foePmdFor = -1;
  foePmdTried = -1;
  myPmdFor = -1;
  myPmdTried = -1;
  // El sprite de CASA no es de fiar despues de un combate. `pmd` es uno
  // solo y el combate lo reutiliza por tres sitios distintos: el relevo
  // (btSwitchTo), la transformacion (btSyncMySprite) y la mega
  // (capture.ino). De esos tres solo el de la transformacion invalidaba
  // monFor, asi que al cambiar de bicho en mitad de la pelea volvias a
  // casa con el SUPLENTE pintado: el principal no cambia al relevar — y
  // no tiene que cambiar, lo dice pet.h —, o sea que monFor seguia
  // diciendo "lo que hay en pmd es el principal" y ensureMon() se creia
  // el cache y no recargaba nunca.
  //
  // Se invalida AQUI y no en cada uno de los tres: el que escribe en pmd
  // no tiene por que acordarse de esto, y el proximo que se anada
  // tampoco. La regla es una sola — se acabo el combate, el sprite de
  // casa se vuelve a leer — y ensureMon() lo restaura en la vuelta
  // siguiente del bucle.
  monFor = -2;
  // 434 KB de PSRAM que fuera del combate no pinta nadie, y que en el proximo
  // combate probablemente sea otro escenario.
  bgImg.unload();

  // CAPTURASTE ALGO: la Pokedex se abre por su ficha. Hasta ahora, atrapar
  // un bicho nuevo se contaba con una linea de texto que se iba sola y el
  // bicho desaparecia dentro de la caja; para verlo habia que acordarse de
  // abrir la galeria y buscarlo a mano entre 1025. Lo que uno quiere
  // despues de atrapar algo es MIRARLO.
  //
  // Va aqui, lo ultimo, y no en BP_CAUGHT: en medio del combate todavia
  // quedan el resumen de la victoria y el botin por contar.
  if (bt.caught) openDexEntry(capturadoDex);
}

// ---------------------------------------------------------------- fases
// Texto de la efectividad, con el mismo criterio que los juegos: solo se
// comenta cuando NO es neutro, porque si no el combate es un muro de texto.
static void btEffectMsg(const HitResult &r, const char *quien) {
  // Los tres golpes que hacen algo RARO van los primeros: lo que han hecho
  // no se parece a "muy eficaz" y se perderia detras de esa frase.
  //
  // Esto es la otra mitad de implementarlos. Un efecto que el motor hace y
  // la pantalla no cuenta es, desde el sofa, un efecto que no existe —— ya
  // paso con el multigolpe, que acertaba las cinco veces en silencio.
  if (r.presentHeal) {
    snprintf(bt.msg, sizeof(bt.msg), "%s got a healthy gift!", quien);
    return;
  }
  if (r.curedTarget) {
    snprintf(bt.msg, sizeof(bt.msg), "%s was cured of paralysis!", quien);
    return;
  }
  if (r.fickleBig) {
    snprintf(bt.msg, sizeof(bt.msg), "It got serious!");
    return;
  }
  if (r.stolenStages) {
    snprintf(bt.msg, sizeof(bt.msg), "Stole the boosts!");
    return;
  }
  if (r.lostType) {
    snprintf(bt.msg, sizeof(bt.msg), "It burned out its own type!");
    return;
  }
  // SUSTITUTO: el golpe no le llego al bicho. Sin decirlo, ver que la
  // barra no baja parece un fallo del juego.
  if (r.beakBurn) {
    snprintf(bt.msg, sizeof(bt.msg), "%s was burned by the hot beak!",
             dexName(bt.b.side[bt.acting].pareceDex()));
    return;
  }
  if (r.bideRelease && r.damage) {
    snprintf(bt.msg, sizeof(bt.msg), "%s unleashed its energy!",
             dexName(bt.b.side[bt.acting].pareceDex()));
    return;
  }
  if (r.subBroke) {
    snprintf(bt.msg, sizeof(bt.msg), "%s's substitute faded!", quien);
    return;
  }
  if (r.subHit) {
    snprintf(bt.msg, sizeof(bt.msg), "The substitute took damage!");
    return;
  }
  if (r.effect == 0)       snprintf(bt.msg, sizeof(bt.msg), "It doesn't affect %s", quien);
  else if (r.effect > 100) snprintf(bt.msg, sizeof(bt.msg), "It's super effective!");
  else if (r.effect < 100) snprintf(bt.msg, sizeof(bt.msg), "It's not very effective");
  else if (r.crit)         snprintf(bt.msg, sizeof(bt.msg), "A critical hit!");
  else if (r.coins)        snprintf(bt.msg, sizeof(bt.msg), "Coins scattered everywhere!");
  else                     bt.msg[0] = 0;
  // MULTIGOLPE. El motor SI pega las dos (o las cinco) veces —— lo hace un
  // bucle sobre `golpes` y lo deja en r.hits ——, pero nada lo decia: una
  // animacion, un mensaje y el dano ya sumado. Desde fuera, Doble Patada
  // parecia acertar una sola vez.
  if (r.hits > 1) {
    char tmp[sizeof(bt.msg)];
    if (bt.msg[0]) {
      snprintf(tmp, sizeof(tmp), "%s x%u", bt.msg, (unsigned)r.hits);
      snprintf(bt.msg, sizeof(bt.msg), "%s", tmp);
    } else {
      snprintf(bt.msg, sizeof(bt.msg), "Hit %u times!", (unsigned)r.hits);
    }
  }
}

// Lo que acaba de pasar con los estados. Se cuenta DESPUES de la efectividad
// porque "muy eficaz" es lo primero que el jugador quiere leer.
static bool btStatusMsg(const HitResult &r) {
  const char *yo = dexName(bt.b.side[bt.acting].pareceDex());
  const char *el = dexName(bt.b.side[bt.acting ^ 1].pareceDex());
  switch (r.gotStatus) {
    case ST_BURN:      snprintf(bt.msg, sizeof(bt.msg), "%s was burned!", el); return true;
    case ST_POISON:
    case ST_BADPOISON: snprintf(bt.msg, sizeof(bt.msg), "%s was poisoned!", el); return true;
    case ST_PARALYSIS: snprintf(bt.msg, sizeof(bt.msg), "%s is paralyzed!", el); return true;
    case ST_SLEEP:     snprintf(bt.msg, sizeof(bt.msg), "%s fell asleep!", el); return true;
    case ST_FREEZE:    snprintf(bt.msg, sizeof(bt.msg), "%s was frozen!", el); return true;
    default: break;
  }
  if (r.stageWhich < SG_COUNT) {
    // OJO: uno por cada SG_*. Cuando entro la evasion esta tabla se quedo en
    // seis, el septimo hueco lo rellena el compilador con nullptr y Sweet
    // Scent anunciaba "<null> fell". El static_assert de abajo es el que
    // impide que vuelva a pasar en silencio: al anadir un stage, el build
    // rompe aqui en vez de la pantalla en mitad de un combate.
    static const char *const NOM[] = { "Attack", "Defense", "Sp.Atk",
                                       "Sp.Def", "Speed", "Accuracy",
                                       "Evasion" };
    static_assert(sizeof(NOM) / sizeof(NOM[0]) == SG_COUNT,
                  "falta el nombre de algun stage");
    snprintf(bt.msg, sizeof(bt.msg), "%s's %s %s!",
             r.stageSelf ? yo : el, NOM[r.stageWhich],
             r.stageDelta > 0 ? "rose" : "fell");
    return true;
  }
  return false;
}

// Por que no se movio. Se dice ANTES de anunciar el golpe, porque es lo que
// pasa antes: el bicho dormido no usa Arañazo y luego se duerme —— esta
// dormido y por eso no usa Arañazo. Lo de antes salia al reves ("X uso
// Arañazo!", el fogonazo, y de postre "X esta dormido"), que es exactamente
// el orden contrario al de la causa.
static bool btBlockedMsg(uint8_t corte) {
  const char *yo = dexName(bt.b.side[bt.acting].pareceDex());
  switch (corte) {
    case BLK_SLEEP:     snprintf(bt.msg, sizeof(bt.msg), "%s is fast asleep!", yo); return true;
    case BLK_FREEZE:    snprintf(bt.msg, sizeof(bt.msg), "%s is frozen solid!", yo); return true;
    case BLK_PARALYSIS: snprintf(bt.msg, sizeof(bt.msg), "%s can't move!", yo); return true;
    case BLK_FLINCH:    snprintf(bt.msg, sizeof(bt.msg), "%s flinched!", yo); return true;
    case BLK_CONFUSED:  snprintf(bt.msg, sizeof(bt.msg), "%s is confused!", yo); return true;
    case BLK_RECHARGE:  snprintf(bt.msg, sizeof(bt.msg), "%s must recharge!", yo); return true;
    case BLK_DISOBEY:   snprintf(bt.msg, sizeof(bt.msg), "%s won't listen!", yo); return true;
    case BLK_TRUANT:    snprintf(bt.msg, sizeof(bt.msg), "%s is loafing around!", yo); return true;
    case BLK_ATTRACT:   snprintf(bt.msg, sizeof(bt.msg), "%s is in love!", yo); return true;
    default: return false;
  }
}

// Anuncia el golpe ANTES de resolverlo. Sin esto no hay manera de saber que
// acaba de hacer el rival: se ve el sacudon y el numero baja, pero no por que.
static void btLaunch(uint8_t who, uint8_t slot) {
  bt.acting = who;
  Combatant &att = bt.b.side[who];
  // Hay que anunciar el golpe que useMove() va a sacar DE VERDAD, o la
  // caja miente. Aqui se miraban solo dos de las cuatro cosas que pueden
  // cambiar el hueco —— faltaban Encore y la Choice —— asi que con un
  // Encore encima la caja decia el golpe elegido, la animacion salia de
  // su tipo y el dano era de otro: desde la pantalla, Encore no hacia
  // nada. Ahora lo pregunta al motor, que es quien lo decide.
  slot = bt.b.realSlot(who, slot);

  // Y aqui se tira el dado de la obediencia, UNA vez, antes de anunciar: si
  // se tirara dentro de useMove() la caja anunciaria un golpe y saldria otro.
  Order ord = bt.b.giveOrder(who, slot);
  slot = ord.slot;

  // Y AHORA, antes de escribir nada en la caja: se mueve o no? El dado se
  // tira una sola vez —— peekBlock() lo guarda y useMove() usa ese mismo ——
  // asi que preguntarlo aqui no cambia el combate, solo el orden en que se
  // cuenta. Si no se mueve, no hay golpe que anunciar ni animacion que
  // pintar: la unica cosa que ha pasado es el estado, y se dice sola.
  uint8_t corte = att.peekBlock();
  if (corte != BLK_NONE) {
    bt.pendingSlot = slot;
    btBlockedMsg(corte);
    bt.phase = BP_SAY;             // al acabar la frase, btResolve() cierra
    bt.until = millis() + 1000;
    return;
  }

  uint16_t mv = (!att.forcejea && slot < att.nMoves && att.usesLeft[slot] > 0)
                    ? att.moves[slot] : MOVE_STRUGGLE;
  // 10 del nombre mas largo + " used " + 15 del golpe mas largo + "!" = 32
  // chars = 192 px a escala 1, y la caja tiene ~290: entra sin rodar texto.
  if (ord.kind == OB_IGNORE)
    snprintf(bt.msg, sizeof(bt.msg), "%s ignores you...", dexName(att.pareceDex()));
  else if (ord.kind == OB_OTHER)
    snprintf(bt.msg, sizeof(bt.msg), "%s did %s instead!", dexName(att.pareceDex()),
             MOVE_TBL[mv < MOVE_COUNT ? mv : MOVE_STRUGGLE].name);
  else
    snprintf(bt.msg, sizeof(bt.msg), "%s used %s!", dexName(att.pareceDex()),
             MOVE_TBL[mv < MOVE_COUNT ? mv : MOVE_STRUGGLE].name);
  bt.pendingSlot = slot;
  bt.phase = BP_SAY;
  bt.until = millis() + 850;
  // Y se APUNTA la hoja, para traerla mientras se lee la frase. Se pide
  // aunque el golpe acabe fallando: lo caro es la lectura, y si falla
  // basta con no pintarla (btResolve pone bt.fxMove a 0).
  bt.fxPend = (mv < MOVE_COUNT && fxHasOwn(mv)) ? mv : 0;
  bt.fxPendAt = millis();
  // A do Anil, inteira, para QUEM usa: o rival tem a sua (OppMove:).
  bt.animPend = (mv < MOVE_COUNT) ? mv : 0;
  bt.animPendRival = (who != btAbajo());
}

// ---- que animacion le toca a un golpe que NO pega ----
//
// El efecto por TIPO vale para los que hacen dano, pero con los de apoyo
// mentia: Tail Whip es de tipo normal y salia el tajo de Vice Grip, o sea
// que bajar la defensa se veia como un aranazo.
//
// Dos categorias se quedan con el tipo A PROPOSITO, porque ahi el tipo SI
// es lo que cuenta: el clima y el terreno (Sunny Day es fuego y sale la
// llama, Rain Dance es agua) y los que meten estado (Thunder Wave saca el
// rayo, Sleep Powder las esporas, Toxic la nube morada).
//
// Devuelve el indice en FX_SUP, o 0xFF si hay que usar el del tipo.
#define FXS_RAISE 0
#define FXS_LOWER 1
#define FXS_HEAL  2
#define FXS_GUARD 3
#define FXS_OTHER 4

static uint8_t btSupportFx(uint16_t mv) {
  const MoveEntry &m = MOVE_TBL[mv];
  if (m.power) return 0xFF;            // si pega, manda el tipo
  switch (m.fx) {
    case FX_W_SUN: case FX_W_RAIN: case FX_W_SAND: case FX_W_HAIL:
    case FX_T_ELEC: case FX_T_GRASS: case FX_T_MISTY: case FX_T_PSY:
      return 0xFF;                     // el tipo es justo lo que se espera ver
    case FX_HEAL_HALF: case FX_HEAL_WEATHER: case FX_REST:
    case FX_AQUARING: case FX_LEECHSEED:
      return FXS_HEAL;
    case FX_PROTECT: case FX_ENDURE: case FX_REFLECT: case FX_LIGHTSCREEN:
    case FX_SAFEGUARD:
      return FXS_GUARD;
    default: break;
  }
  // Los estados tambien se quedan con su tipo; lo que cambia de verdad son
  // los que solo mueven stats.
  if (m.sx >= SX_BURN && m.sx <= SX_FLINCH) return 0xFF;
  if (m.sx >= SX_LOWER_ATK && m.sx <= SX_LOWER_EVA) return FXS_LOWER;
  if (m.sx >= SX_LOWER2_ATK && m.sx <= SX_LOWER2_EVA) return FXS_LOWER;
  if (m.sx >= SX_RAISE_ATK && m.sx <= SX_RAISE_EVA) return FXS_RAISE;
  if (m.sx >= SX_SELF_SPA2 && m.sx <= SX_RAISE_ALL) return FXS_RAISE;
  if (m.sx >= SX_RAISE2_ATK && m.sx <= SX_RAISE2_EVA) return FXS_RAISE;
  if (m.sx >= SX_RAISE_ATKDEF && m.sx <= SX_RAISE_DEFSPD) return FXS_RAISE;
  // Los que entraron al leer bien los codigos del PBS. Van por nombre y
  // no por rango por lo mismo que sxEsPropio en battle.cpp: un rango con
  // el enum creciendo por el final se rompe sin avisar.
  if (m.sx == SX_RAISE_ATKACC || m.sx == SX_RAISE_SPASPDSPE ||
      m.sx == SX_RAISE2_SPASPDSPE || m.sx == SX_RAISE3_SPA ||
      m.sx == SX_RAISE_SPASPD_CURA || m.sx == SX_RAISE3_ATK_KO)
    return FXS_RAISE;
  if (m.sx == SX_LOWER2_SPA_ATR) return FXS_LOWER;
  if (m.fx == FX_BELLYDRUM || m.fx == FX_FOCUSENERGY) return FXS_RAISE;
  if (m.fx == FX_HAZE || m.fx == FX_CLEAR_TARGET) return FXS_LOWER;
  return FXS_OTHER;
}

// resuelve de verdad el golpe que se acaba de anunciar
// Un golpe que se usa SOBRE UNO MISMO: su animacion va encima del que lo
// tira, no del que tiene enfrente. La lista sale de los FX_ que el propio
// combate ya trata como "esto es mio": barreras, preparacion, curaciones y
// las subidas de stat propias.
static bool btFxSobreUnoMismo(uint16_t mv, const HitResult &r) {
  if (r.noDamage && r.stageSelf) return true;      // lo de siempre
  if (mv >= MOVE_COUNT) return false;
  switch (MOVE_TBL[mv].fx) {
    case FX_REFLECT:
    case FX_LIGHTSCREEN:
    case FX_SAFEGUARD:
    case FX_PROTECT:
    case FX_ENDURE:
    case FX_FOCUSENERGY:
    case FX_LASERFOCUS:
    case FX_MINDREADER:
    case FX_AQUARING:
    case FX_MIST:
    case FX_IMPRISON:
    case FX_HEAL_BELL:
    case FX_PSYCHUP:
    case FX_TRANSFORM:
      return true;
    default:
      return false;
  }
}

// La frase de CADA golpe de carga, la del pbChargingTurnMessage del Anil.
// Antes era siempre "is charging up!": Vuelo y Excavar no decian a donde
// se iba el bicho, que es lo que explica el turno. La usan el turno de
// carga (BP_HIT) y la carga rapida del sol/lluvia (BP_CARGA).
static const char *btFraseCarga(uint16_t g) {
  static const struct { const char *g, *f; } CARGA[] = {
    { "FLY", "%s flew up high!" },
    { "BOUNCE", "%s sprang up!" },
    { "DIG", "%s burrowed its way under the ground!" },
    { "DIVE", "%s hid underwater!" },
    { "PHANTOMFORCE", "%s vanished instantly!" },
    { "SHADOWFORCE", "%s vanished instantly!" },
    { "SOLARBEAM", "%s took in sunlight!" },
    { "SOLARBLADE", "%s took in sunlight!" },
    { "RAZORWIND", "%s whipped up a whirlwind!" },
    { "SKYATTACK", "%s became cloaked in a harsh light!" },
    { "SKULLBASH", "%s tucked in its head!" },
    { "METEORBEAM", "%s is overflowing with space power!" },
    { "GEOMANCY", "%s is absorbing power!" },
    { "ELECTROSHOT", "%s absorbed electricity!" },
    { "FUTURESIGHT", "%s foresaw an attack!" },
    { "DOOMDESIRE", "%s chose Doom Desire as its destiny!" },
  };
  if (g >= MOVE_COUNT) return nullptr;
  for (const auto &c : CARGA)
    if (!strcmp(MOVE_TBL[g].name, c.g)) return c.f;
  return nullptr;
}

// El sonido DEL GOLPE (ver el comentario de arriba, en btResolve).
static void btSomDoGolpe(uint16_t mvFx, const Combatant &att) {
  if (!movePlay(mvFx) &&
      !(moveHas(mvFx, MV_SOUND) && cryPlay((uint16_t)att.dex))) {
    if (bt.last.noDamage || bt.last.missed) sfxPlay(SFX_PLAY);
    else if (bt.last.effect < 100)          sfxPlay(SFX_HIT_WEAK);
    else if (bt.last.effect > 100)          sfxPlay(SFX_HIT_SUPER);
    else                                    sfxPlay(SFX_HIT);
  }
}

static void btResolve() {
  // el golpe ANTES de resolverlo: useMove consume el PP y puede dejar el
  // slot en otro sitio, y lo que el efecto necesita es que se tiro
  const Combatant &att = bt.b.side[bt.acting];
  uint16_t mv = (!att.forcejea && bt.pendingSlot < att.nMoves)
                    ? att.moves[bt.pendingSlot] : MOVE_STRUGGLE;
  // A vida do alvo ANTES: o que ele perdeu de verdade sai daqui, nao de
  // last.damage, que passa da vida dele num golpe que derruba e nao conta
  // Sturdy, Focus Sash nem a baya que ele comeu.
  const uint16_t vidaAntes = bt.b.side[bt.acting ^ 1].hp;
  bt.last = bt.b.useMove(bt.acting, bt.pendingSlot);
  const uint16_t vidaDepois = bt.b.side[bt.acting ^ 1].hp;
  const uint16_t perdeu = vidaAntes > vidaDepois ? (uint16_t)(vidaAntes - vidaDepois) : 0;
  if (bt.acting == BT_PLAYER) btSavePP();
  // Ladron contra un salvaje: el objeto va a la mochila en el acto.
  if (!bt.pvp && bt.last.itemToBag && bt.last.itemStolen != IT_NONE)
    pet.takeItem(bt.last.itemStolen);

  // No se movio, y eso YA lo dijo btLaunch antes de que empezara el turno.
  // Aqui no se vuelve a decir ni se pasa por BP_ATTACK/BP_HIT: esos dos son
  // 740 ms de animacion de un golpe que no existe.
  //
  // La confusion si deja una segunda linea, porque ahi SI baja la vida y un
  // numero que baja sin frase parece un fallo.
  bt.anim = false;
  if (bt.last.blocked != BLK_NONE) {
    bt.fxMove = 0;
    bt.fxType = (uint8_t)T_NONE;
    bt.fxSup = 0xFF;
    if (bt.last.blocked == BLK_CONFUSED)
      snprintf(bt.msg, sizeof(bt.msg), "%s hurt itself!",
               dexName(bt.b.side[bt.acting].pareceDex()));
    // Mofa y Anticura puestas DESPUES de elegir: el golpe se anuncia y
    // no sale. Sin frase, parecia que el juego se lo comia.
    else if (bt.last.blocked == BLK_TAUNT)
      snprintf(bt.msg, sizeof(bt.msg), "%s can't use it after the taunt!",
               dexName(bt.b.side[bt.acting].pareceDex()));
    else if (bt.last.blocked == BLK_HEALBLOCK)
      snprintf(bt.msg, sizeof(bt.msg), "%s is prevented from healing!",
               dexName(bt.b.side[bt.acting].pareceDex()));
    else
      bt.msg[0] = 0;
    bt.extraStep = 9;              // sin cola de drenaje/estado
    bt.phase = BP_MSG;
    bt.until = millis() + (bt.msg[0] ? 900 : 0);
    return;
  }

  // Sin dibujo si no entro: un fogonazo encima de un "it missed!" dice lo
  // contrario de lo que pone la caja.
  bool entro = !bt.last.missed && !bt.last.blocked && !bt.last.failed
               && !bt.last.blockedByProtect && !bt.last.charging;
  // Turno de carga: o sumico espera a frase e a animacao (BP_HIT acerta a
  // hora; 5 s e so o teto, se algo cortar o caminho).
  if (bt.last.charging) bt.cargaAte[bt.acting] = millis() + 5000;
  uint16_t mvFx = (mv < MOVE_COUNT) ? mv : MOVE_STRUGGLE;
  bt.fxType = entro ? MOVE_TBL[mvFx].type : (uint8_t)T_NONE;
  bt.fxSup = entro ? btSupportFx(mvFx) : 0xFF;
  bt.fxMove = entro ? mvFx : 0;
  // La hoja YA esta cargada: la pidio btLaunch y la trajo battleTick
  // mientras se leia el anuncio. Aqui solo se decide si se PINTA, que
  // es gratis. Si la lectura fallo (sin tarjeta, o fichero corto),
  // fxMove.move no casa con bt.fxMove y btMoveFx cae al efecto del
  // tipo, igual que siempre.
  //
  // Leerla aqui era lo que obligaba a dejar 51 golpes fuera por caros:
  // el hueco entre el anuncio y la embestida son ~200 ms, y a 0,2 MB/s
  // eso son 40 KB. Durante la frase hay 850.
  if (bt.animPend && bt.animPend == bt.fxMove) {   // no dio tiempo: ahora
    animMove.load(bt.animPend, MOVE_TBL[bt.animPend].name, bt.animPendRival);
    bt.animPend = 0;
  }
  // Toca a do Anil se a carregada e ESTE golpe e deste lado.
  bt.anim = entro && animMove.blob && animMove.move == mvFx &&
            animMove.rival == (bt.acting != btAbajo());
  if (!bt.anim && bt.fxPend && bt.fxPend == bt.fxMove) {   // no dio tiempo: ahora
    fxMove.load(bt.fxPend);
    bt.fxPend = 0;
  }
  // Un golpe que se sube a si mismo (Swords Dance) se pinta encima de QUIEN
  // lo usa; todo lo demas, encima del que lo recibe.
  // De QUIEN es la animacion. Miraba solo `stageSelf`, que es "me subo un
  // stat": con eso, Reflejo, Pantalla Luz, Foco Energia, Protección y
  // compania —— que no tocan ningun stat —— se pintaban encima del RIVAL,
  // como si se las hubieras hecho a el.
  bt.fxSide = btFxSobreUnoMismo(mvFx, bt.last) ? bt.acting
                                               : (uint8_t)(bt.acting ^ 1);
  bt.fxAt = millis();

  bt.msg[0] = 0;
  bt.phase = BP_ATTACK;
  bt.until = millis() + 320;
  // A animacao do Anil dura o que ela dura: 20 quadros por segundo.
  if (bt.anim && (uint32_t)animMove.nq * 50 > 320)
    bt.until = millis() + (uint32_t)animMove.nq * 50;
  // A barra cai quando a animacao comeca. So no MULTIGOLPE fica um resto
  // guardado (o que os outros impactos vao tirar): hpPend nunca passa do que
  // o alvo perdeu, entao a barra so pode descer. Um impacto so: hpPend = 0.
  bt.golpeN = 0;
  bt.hpPend = (entro && bt.last.hits > 1 && perdeu > bt.last.hitDmg[0])
                  ? (uint16_t)(perdeu - bt.last.hitDmg[0]) : 0;
  // El sonido del GOLPE, por eficacia. Antes eran SFX_PLAY y SFX_TAP,
  // o sea los pitidos del menu: pegar sonaba igual que tocar un boton.
  // `effect` viene x100 (0, 25, 50, 100, 200, 400).
  // Un golpe de APOYO no pega: sonaba a golpe normal y parecia que
  // habia hecho dano. Lo mismo el que falla. Los tres sonidos de dano
  // son solo para cuando hay dano de verdad.
  // El sonido DEL GOLPE, el que le pone el Anil. Es el mismo dato del que
  // salen los fotogramas: la animacion de cada golpe dice que SE suena.
  // Los tres pitidos por eficacia eran sintetizados — ondas cuadradas — y
  // sonaban a consola de juguete al lado de lo que se ve en pantalla.
  //
  // Si ese golpe no tiene el suyo en la tarjeta (14 de 717 no lo tienen, y
  // sin tarjeta ninguno), se cae a los pitidos de antes, que ademas
  // distinguen la eficacia. La eficacia la sigue diciendo la caja de
  // texto, asi que no se pierde nada por el camino.
  //
  // Growl, Roar, Howl, Hyper Voice y Disarming Voice: en el Anil su SE no
  // tiene nombre, que en Essentials quiere decir "el grito del que lo
  // usa". Asi que es el grito, no un pitido.
  // Carga rapida (Rayo Solar al sol): antes del golpe, la frase y la
  // animacion de la carga. El sonido y la barra esperan a BP_CARGA.
  // Magnitud: "Magnitude 7!" entre el anuncio y el golpe, por el mismo
  // camino (pbOnStartUse del Anil; sale aunque luego falle).
  if (bt.last.cargaRapida || bt.last.magnitud) {
    bt.cargaMv = mvFx;
    bt.cargaHpPend = bt.hpPend;
    bt.hpPend = perdeu;          // la barra no baja durante la frase
    bt.cargaAnimOk = bt.anim;
    bt.anim = false;
    bt.cargaPasso = 0;
    bt.phase = BP_CARGA;
    bt.until = millis();
    return;
  }
  btSomDoGolpe(mvFx, att);
}

// Caiu e ninguem vai por ele? Entre placas, quando os DOIS lados repoem
// (um caiu, o outro deu U-turn), o caido espera a vez na fila: sem isto o
// BP_MSG do primeiro relevo o via no chao, abria outro desmaio e o dono
// tinha de escolher DUAS vezes quem entra.
static bool btCaidoAgora(uint8_t s) {
  if (!bt.b.side[s].fainted()) return false;
  if (bt.pvp)
    for (uint8_t i = pvpI; i < pvpN; i++)
      if (pvpFila[i].lado == s && pvpFila[i].tipo == PA_REPOE) return false;
  return true;
}

void battleTick() {
  if (!bt.open) return;
  uint32_t now = millis();
  // La bolsa y la captura tienen sus propias fases (capture.ino) y
  // algunas NO caducan por reloj (la bolsa espera al jugador), asi que
  // se consultan antes del corte por tiempo.
  // Entre placas o relogio corre com a bolsa aberta tambem: a bolsa
  // congela o combate, nao os 60 s (e ela fecha quando eles acabam).
  if (bt.pvp && bt.phase == BP_BAG) pvpRelogioTick(now);
  if (btCaptureTick(now)) return;
  // A animacao de campo (Drought...) ainda tocando: nada avanca.
  if ((int32_t)(now - bt.animFim) < 0) return;
  // LA HOJA DEL GOLPE, mientras se lee el anuncio. Va ANTES del corte
  // por tiempo a proposito: esto tiene que correr DURANTE la frase, no
  // al acabarla.
  //
  // Los 150 ms de espera son un repintado: el combate va a unos 8 por
  // segundo, asi que con eso el "X uso Y!" ya esta en pantalla cuando
  // empieza la lectura. Sin ellos la pausa caeria entre el toque y la
  // frase, y eso se lee como un mando que no responde.
  // A animacao DO ANIL primeiro: se ela veio, a velha nao faz falta.
  if (bt.animPend && (int32_t)(now - bt.fxPendAt) >= 150) {
    // A anterior JA acabou (o corte do animFim e logo acima), mas bt.anim
    // e bt.fxAt ainda sao dela. Sem isto, a nova entrava no lugar da velha
    // com o relogio velho e o fim dela tocava DURANTE a frase: no Explosion,
    // a tela branca antes de explodir. Quem liga de novo e o golpe (abaixo,
    // quando entra) ou btAnimDoCampo.
    bt.anim = false;
    uint32_t t0 = millis();
    bool ok = animMove.load(bt.animPend, MOVE_TBL[bt.animPend].name,
                            bt.animPendRival);
    Serial.printf("anim %s%s %s %lu ms\n", bt.animPendRival ? "f_" : "p_",
                  MOVE_TBL[bt.animPend].name, ok ? "ok" : "--",
                  (unsigned long)(millis() - t0));
    if (ok && bt.fxPend == bt.animPend) bt.fxPend = 0;
    bt.animPend = 0;
  }
  if (bt.fxPend && (int32_t)(now - bt.fxPendAt) >= 150) {
    uint32_t t0 = millis();
    fxMove.load(bt.fxPend);
    // SE MIDE, y se dice. El tope de tools/gen_art.py (120 KB) sale de
    // una CUENTA —— 700 ms de cobertura por 0,18 MB/s ——, no de un
    // cronometro, y la cuenta anterior ya estuvo mal una vez. Con esto
    // basta con jugar un combate con el puerto abierto para saber si
    // el numero aguanta.
    Serial.printf("fx %s %lu ms\n", MOVE_TBL[bt.fxPend].name,
                  (unsigned long)(millis() - t0));
    bt.fxPend = 0;
  }
  if ((int32_t)(now - bt.until) < 0) return;  // la fase sigue corriendo

  // EL GOLPE QUE NO SE ELIGE. La segunda mitad de Vuelo, Excavar, Buceo
  // o Rayo Solar, la recarga de Hiperrayo y la racha de Enfado o Rodar:
  // el motor ya sabe que golpe sale (realSlot), pero la pantalla se
  // paraba en el menu y obligaba a tocar el golpe OTRA vez, como si se
  // pudiera elegir. En el Anil ese turno sale solo, y aqui tambien.
  // Entre placas: chegou ao menu, abre o ponto do turno (as duas placas
  // chegam aqui no mesmo ponto do combate).
  if (bt.pvp && bt.phase == BP_CHOOSE && pvpPonto == PT_NADA && !pvpAcabou)
    pvpAbreTurno();
  if (bt.pvp) pvpRelogioTick(now);
  if (bt.pvp && bt.phase == BP_CHOOSE && pvpPonto == PT_TURNO) {
    const Combatant &yo = bt.b.side[pvpEu];
    if (!yo.fainted() && (yo.lockTurns || yo.recharging ||
                          yo.chargingMove != 0xFFFF || yo.bideTurns)) {
      DueloDecisao d;
      d.tipo = DD_GOLPE;
      d.arg = bt.b.realSlot(pvpEu, bt.sel);
      pvpDecide(pvpEu, d);
      return;
    }
    if (!yo.fainted() && yo.encoreTurns > 0 && yo.encoreSlot < yo.nMoves)
      bt.sel = yo.encoreSlot;
  }
  if (bt.phase == BP_CHOOSE && !bt.mustSwitch && !bt.pvp) {
    const Combatant &yo = bt.b.side[BT_PLAYER];
    if (!yo.fainted() &&
        (yo.lockTurns || yo.recharging || yo.chargingMove != 0xFFFF ||
         yo.bideTurns)) {
      bt.sel = bt.b.realSlot(BT_PLAYER, bt.sel);
      bt.msg[0] = 0;
      btCommitTurn();
      return;
    }
    // ENCORE: aqui no sale solo (bolsa, equipo y huir siguen valiendo, como
    // en el Anil), pero el golpe ya esta decidido: la tarjeta se clava en
    // el. Antes ensenaba el que tuvieras elegido con un "OFF" y, al tocar,
    // salia otro.
    if (!yo.fainted() && yo.encoreTurns > 0 && yo.encoreSlot < yo.nMoves)
      bt.sel = yo.encoreSlot;
  }

  switch (bt.phase) {
    case BP_INTRO:
      // Intimidate y Drought saltan al salir, antes del primer turno. Si no
      // se cuentan aqui, el combate empieza con el ataque ya bajado y parece
      // un error del juego.
      while (bt.introAbil < 2) {
        uint8_t s = bt.introAbil++;
        if (!bt.b.startAbil[s]) continue;
        // La Imposter se cuenta aparte. Con el texto generico salia
        // "Bulbasaur's Imposter!", porque el nombre se saca de asDex y
        // la habilidad ACABA de cambiarlo: el jugador veia aparecer un
        // Ditto y acto seguido un mensaje de otro bicho, sin que nada
        // dijera que se habia transformado. El nombre de verdad del que
        // copia vive en `dex`.
        if (abilAt(bt.b.startAbil[s]).fx == AB_IMPOSTER)
          snprintf(bt.msg, sizeof(bt.msg), "%s transformed into %s!",
                   dexName(bt.b.side[s].dex),
                   dexName(bt.b.side[s ^ 1].pareceDex()));
        else
          snprintf(bt.msg, sizeof(bt.msg), "%s's %s!",
                   dexName(bt.b.side[s].pareceDex()), abilAt(bt.b.startAbil[s]).name);
        bt.until = now + 1000;
        // E o clima/terreno que ela trouxe, animado (o sol do Drought...).
        uint32_t dura = btAnimDoCampo(s, now);
        if (dura > 1000) bt.until = now + dura;
        return;                     // seguimos en BP_INTRO para el siguiente
      }
      bt.phase = BP_CHOOSE;
      bt.msg[0] = 0;
      break;

    case BP_SAY:
      btResolve();
      break;

    case BP_MEGA: {
      // La escena ya paso: AHORA se transforma, para que el cambio de
      // sprite caiga con el final de los anillos y no antes.
      uint8_t lado = bt.megaSide;
      bt.megaRevelaLado = lado;          // y desde aqui vuelve del blanco
      bt.megaRevelaAt = now;
      if (bt.b.doMega(lado)) {
        const Combatant &me = bt.b.side[lado];
        if (lado == btAbajo()) {
          pmd.loadMega(me.megaIdx, me.shiny);
        } else {
          foePmd.loadMega(me.megaIdx, me.shiny);
          foePmdFor = -1;        // la cache ya no vale: el sprite es otro
          foePmdTried = -1;
        }
        // A la Pokedex. Una mega dura lo que el combate y no se queda en
        // el equipo, asi que este es el UNICO momento en que se puede
        // apuntar que existe. La del rival tambien: en los juegos la
        // Pokedex registra lo que VES, y es la unica via de llegar a las
        // megas de bichos cuya piedra no te ha tocado nunca.
        if (!bt.pvp) pet.registerMega(me.megaIdx);
        snprintf(bt.msg, sizeof(bt.msg),
                 lado == btAbajo() ? "%s MEGA EVOLVED!" : "Foe %s MEGA EVOLVED!",
                 dexName(me.dex));
      }
      if (lado != BT_PLAYER || bt.pvp) {
        // Entre placas as DUAS caem no meio do turno (na fila).
        // La suya cae EN MITAD del turno: ya hay golpe elegido, asi que
        // de aqui no se vuelve al menu —— se lee la frase y se sigue.
        bt.megaSide = BT_PLAYER;
        bt.phase = BP_MEGA_SAY;
        bt.until = now + 1100;
        break;
      }
      // No gasta el turno: en los juegos se megaevoluciona Y se ataca.
      // Mismo camino que el relevo para su "Go, X!" —— BP_MSG con la cola
      // de avisos saltada, que desemboca en BP_CHOOSE.
      bt.phase = BP_MSG;
      bt.extraStep = 9;
      bt.endStep = 2;
      bt.until = now + 1100;
      break;
    }

    case BP_MEGA_SAY:
      // Se leyo la mega del rival; el turno sigue donde lo dejo el toque.
      bt.msg[0] = 0;
      if (bt.pvp) {
        // a fila segue pelo caminho de sempre (BP_MSG -> pvpSegue)
        bt.phase = BP_MSG;
        bt.extraStep = 9;
        bt.secondPending = pvpI < pvpN;
        bt.until = now;
        break;
      }
      btCommitTurn();
      break;

    case BP_CARGA: {
      // CARGA RAPIDA: pbQuickChargingMove del Anil —— la frase de la
      // carga, su animacion (hitNum 1), lo que la carga sube, y el golpe.
      const Combatant &cq = bt.b.side[bt.acting];
      const uint16_t g = bt.cargaMv;
      const bool riv = bt.acting != btAbajo();
      if (bt.cargaPasso == 0) {
        bt.cargaPasso = 1;
        if (bt.last.magnitud) {
          snprintf(bt.msg, sizeof(bt.msg), "Magnitude %u!", (unsigned)bt.last.magnitud);
          bt.until = now + 900;
          break;
        }
        const char *f = btFraseCarga(g);
        snprintf(bt.msg, sizeof(bt.msg), f ? f : "%s is charging up!",
                 dexName(cq.pareceDex()));
        bt.until = now + 900;
        if (g < MOVE_COUNT && animMove.load(g, MOVE_TBL[g].name, riv, 1)) {
          bt.anim = true;
          bt.fxAt = now;
          bt.animFim = now + (uint32_t)animMove.nq * 50;
          if ((int32_t)(bt.animFim - bt.until) > 0) bt.until = bt.animFim;
        }
        break;
      }
      bt.anim = false;
      if (bt.cargaPasso == 1) {
        bt.cargaPasso = 2;
        // Electrorrayo con lluvia: "Sp.Atk rose!" ANTES del golpe, que
        // es cuando sube. Se borra para que la cola no lo repita.
        if (bt.last.stageWhich < SG_COUNT && bt.last.stageSelf) {
          const uint8_t st = bt.last.gotStatus;
          bt.last.gotStatus = 0;
          btStatusMsg(bt.last);
          bt.last.gotStatus = st;
          bt.last.stageWhich = 0xFF;
          bt.until = now + 900;
          break;
        }
      }
      // El golpe, como si viniera de btResolve.
      bt.msg[0] = 0;
      if (bt.cargaAnimOk && g < MOVE_COUNT &&
          animMove.load(g, MOVE_TBL[g].name, riv, 0))
        bt.anim = true;
      bt.fxAt = now;
      bt.hpPend = bt.cargaHpPend;
      bt.phase = BP_ATTACK;
      bt.until = now + 320;
      if (bt.anim && (uint32_t)animMove.nq * 50 > 320)
        bt.until = now + (uint32_t)animMove.nq * 50;
      btSomDoGolpe(g, cq);
      break;
    }

    case BP_ATTACK:
      bt.phase = BP_HIT;
      bt.until = now + 420;
      break;

    case BP_HIT: {
      // El "no se movio" ya no llega hasta aqui: lo cuenta btLaunch antes de
      // anunciar y btResolve corta el turno sin pasar por el golpe.
      // Turno de carga: no hay golpe que enseÃ±ar todavia, solo el aviso.
      if (bt.last.charging) {
        // La frase de CADA golpe, la del pbChargingTurnMessage del Anil.
        // Antes era siempre "is charging up!": Vuelo y Excavar no decian
        // a donde se iba el bicho, que es lo que explica el turno.
        const char *frase = bt.last.bideStore ? "%s is storing energy!"
                                              : "%s is charging up!";
        const Combatant &cq = bt.b.side[bt.acting];
        uint16_t g = cq.chargingMove != 0xFFFF ? cq.chargingMove : cq.lastMove;
        if (!bt.last.bideStore && btFraseCarga(g)) frase = btFraseCarga(g);
        snprintf(bt.msg, sizeof(bt.msg), frase,
                 dexName(bt.b.side[bt.acting].pareceDex()));
        bt.phase = BP_MSG;
        // Cabezazo e Rayo Meteorico sobem um stat NA CARGA: o passo 2 da
        // fila (btStatusMsg) diz "X's Defense rose!" depois da frase.
        bt.extraStep = bt.last.stageWhich < SG_COUNT ? 2 : 9;
        bt.until = now + 900;
        // A ANIMACAO DE CARGA do Anil (TwoTurnMove: hitNum 1 no turno de
        // carga): "Fly charging", "Common:Dig charging"... Junto com a
        // frase, e o bicho so some quando ela acaba.
        bt.cargaAte[bt.acting] = now;
        bt.cargaAnim[bt.acting] = false;
        if (!bt.last.bideStore && g < MOVE_COUNT &&
            animMove.load(g, MOVE_TBL[g].name, bt.acting != btAbajo(), 1)) {
          bt.anim = true;
          bt.fxAt = now;
          bt.animFim = now + (uint32_t)animMove.nq * 50;
          bt.cargaAte[bt.acting] = bt.animFim;
          bt.cargaAnim[bt.acting] = true;
          if ((int32_t)(bt.animFim - bt.until) > 0) bt.until = bt.animFim;
        }
        break;
      }
      if (bt.last.blockedByProtect) {
        snprintf(bt.msg, sizeof(bt.msg), "%s protected itself!",
                 dexName(bt.b.side[bt.acting ^ 1].pareceDex()));
        bt.phase = BP_MSG;
        // Si se estrello contra el escudo, el paso 1 es quien lo cuenta.
        bt.extraStep = bt.last.crashed ? 1 : 9;
        bt.until = now + 900;
        break;
      }
      if (bt.last.failed) {
        // "But it failed!" a secas es la frase que no explica nada. Si lo
        // que paso fue la Anulacion, se dice.
        const Combatant &q = bt.b.side[bt.acting];
        if (q.disableTurns > 0 && bt.pendingSlot == q.disabledSlot)
          snprintf(bt.msg, sizeof(bt.msg), "%s's move is disabled!",
                   dexName(q.pareceDex()));
        else if (bt.pendingSlot < q.nMoves && !bt.last.struggled &&
                 MOVE_TBL[q.moves[bt.pendingSlot]].fx == FX_SPLASH)
          snprintf(bt.msg, sizeof(bt.msg), "But nothing happened!");
        else
          snprintf(bt.msg, sizeof(bt.msg), "But it failed!");
        bt.phase = BP_MSG;
        bt.extraStep = 9;
        bt.until = now + 900;
        break;
      }
      if (bt.last.missed) {
        snprintf(bt.msg, sizeof(bt.msg), "%s missed!",
                 dexName(bt.b.side[bt.acting].pareceDex()));
        bt.phase = BP_MSG;
        bt.extraStep = bt.last.crashed ? 1 : 9;
        bt.until = now + 800;
        break;
      }
      // MULTIGOLPE: quedan impactos. Se vuelve a tocar la animacion (el
      // Anil hace pbShowAnimation por cada hit) y la barra baja otro
      // trozo al llegar a BP_HIT. "Hit N times!" sale al acabar el ultimo.
      if (bt.golpeN + 1 < bt.last.hits && bt.last.damage) {
        bt.golpeN++;
        // O proximo pedaco da barra cai junto com esta repeticao; o ultimo
        // leva o que sobrar.
        {
          uint16_t d = bt.golpeN < HIT_MAX ? bt.last.hitDmg[bt.golpeN] : 0;
          bt.hpPend = d < bt.hpPend ? (uint16_t)(bt.hpPend - d) : 0;
          if (bt.golpeN + 1 >= bt.last.hits) bt.hpPend = 0;
        }
        bt.fxAt = now;
        bt.phase = BP_ATTACK;
        bt.until = now + 320;
        // O impacto golpeN do Anil e a animacao golpeN posicoes depois
        // ("Bullet Seed hit 2", "hit 3"...). Sem ela, a de sempre.
        if (bt.anim) {
          const uint16_t mvA = animMove.move;
          const bool riv = animMove.rival;
          if (mvA < MOVE_COUNT &&
              !animMove.load(mvA, MOVE_TBL[mvA].name, riv, bt.golpeN))
            animMove.load(mvA, MOVE_TBL[mvA].name, riv, 0);
          bt.anim = animMove.blob != nullptr;
        }
        if (bt.anim && (uint32_t)animMove.nq * 50 > 320)
          bt.until = now + (uint32_t)animMove.nq * 50;
        uint16_t mvFx = bt.fxMove ? bt.fxMove : (uint16_t)MOVE_STRUGGLE;
        if (!movePlay(mvFx))
          sfxPlay(bt.last.effect < 100 ? SFX_HIT_WEAK
                  : bt.last.effect > 100 ? SFX_HIT_SUPER : SFX_HIT);
        break;
      }
      const char *quien = dexName(bt.b.side[bt.acting ^ 1].pareceDex());
      btEffectMsg(bt.last, quien);
      bt.extraStep = 0;
      bt.phase = BP_MSG;
      bt.until = now + (bt.msg[0] ? 900 : 0);
      break;
    }

    case BP_MSG:
      // Cola de avisos posteriores al golpe, uno por paso: drenaje, retroceso,
      // estado y por fin el desgaste de veneno/quemadura al cerrar el turno.
      if (bt.extraStep == 0) {
        bt.extraStep = 1;
        // La habilidad va la PRIMERA de la cola: explica lo que viene detras.
        // Un formato unico vale para las 56, porque el efecto ya lo cuenta el
        // mensaje siguiente ("Static!" y luego "X is paralyzed!").
        if (bt.last.abilFired) {
          uint8_t lado = bt.last.abilOfDef ? (bt.acting ^ 1) : bt.acting;
          snprintf(bt.msg, sizeof(bt.msg), "%s's %s!",
                   dexName(bt.b.side[lado].pareceDex()), abilAt(bt.last.abilFired).name);
          bt.until = now + 900;
          break;
        }
        if (bt.last.weatherSet) {
          static const char *CIELO[] = { "", "The sunlight turned harsh!",
                                         "It started to rain!",
                                         "A sandstorm kicked up!",
                                         "It started to hail!" };
          snprintf(bt.msg, sizeof(bt.msg), "%s", CIELO[bt.last.weatherSet]);
          bt.until = now + 900;
          break;
        }
        if (bt.last.terrainSet) {
          static const char *SUELO[] = { "", "Electricity ran on the ground!",
                                         "Grass grew on the ground!",
                                         "Mist swirled on the ground!",
                                         "The ground got weird!" };
          static_assert(sizeof(SUELO) / sizeof(SUELO[0]) == TR_PSY + 1,
                        "falta el aviso de algun terreno");
          snprintf(bt.msg, sizeof(bt.msg), "%s", SUELO[bt.last.terrainSet]);
          bt.until = now + 900;
          break;
        }
        if (bt.last.healed) {
          // drenar es quitarselo al otro; curarse es otra cosa y se cuenta
          // distinto, o parece que Recover le hace dano al rival
          snprintf(bt.msg, sizeof(bt.msg),
                   bt.last.noDamage ? "%s regained health!" : "%s drained health!",
                   dexName(bt.b.side[bt.acting].pareceDex()));
          bt.until = now + 900;
          break;
        }
        if (bt.last.sideFx) {
          const char *yo = dexName(bt.b.side[bt.acting].pareceDex());
          const char *el = dexName(bt.b.side[bt.acting ^ 1].pareceDex());
          switch (bt.last.sideFx) {
            case FX_REFLECT:     snprintf(bt.msg, sizeof(bt.msg), "Reflect went up!"); break;
            case FX_LIGHTSCREEN: snprintf(bt.msg, sizeof(bt.msg), "Light Screen went up!"); break;
            case FX_SAFEGUARD:   snprintf(bt.msg, sizeof(bt.msg), "%s is covered!", yo); break;
            case FX_PROTECT:     snprintf(bt.msg, sizeof(bt.msg), "%s braced itself!", yo); break;
            case FX_ENDURE:      snprintf(bt.msg, sizeof(bt.msg), "%s dug in!", yo); break;
            case FX_HAZE:        snprintf(bt.msg, sizeof(bt.msg), "All stats went back!"); break;
            case FX_CLEAR_TARGET: snprintf(bt.msg, sizeof(bt.msg), "%s's stats reset!", el); break;
            case FX_PSYCHUP:     snprintf(bt.msg, sizeof(bt.msg), "%s copied the stats!", yo); break;
            case FX_FOCUSENERGY: snprintf(bt.msg, sizeof(bt.msg), "%s is fired up!", yo); break;
            case FX_LASERFOCUS:  snprintf(bt.msg, sizeof(bt.msg), "%s is focused!", yo); break;
            case FX_MINDREADER:  snprintf(bt.msg, sizeof(bt.msg), "%s took aim!", yo); break;
            case FX_LEECHSEED:   snprintf(bt.msg, sizeof(bt.msg), "%s was seeded!", el); break;
            // Solo la pone un FANTASMA, y le cuesta media vida. Con los
            // dos nombres (15 chars el mas largo) no cabe nada mas en
            // los 44 de bt.msg: lo que paga se lee en su barra de PS.
            case FX_CURSE:       snprintf(bt.msg, sizeof(bt.msg), "%s cursed %s!", yo, el); break;
            // PURIFICACION le quita el estado AL OTRO, que es lo que
            // nadie se espera de un golpe que ademas te cura: sin la
            // frase parece que solo se curo quien lo uso.
            case FX_PURIFY:      snprintf(bt.msg, sizeof(bt.msg), "%s was purified!", el); break;
            // AUTOTOMIA y LIMPIEZA cambian algo que no se ve en ninguna
            // barra —— el peso y las trampas del campo ——, asi que sin
            // frase parecen un golpe que no hizo nada.
            case FX_AUTOTOMIZE:  snprintf(bt.msg, sizeof(bt.msg), "%s became nimble!", yo); break;
            case FX_TIDYUP:      snprintf(bt.msg, sizeof(bt.msg), "The field was tidied up!"); break;
            // RESPIRO: lo que se ve es la cura, y lo que DECIDE el
            // turno es el tipo que acaba de perder. Sin la frase, el
            // Terremoto que de pronto le alcanza parece un fallo.
            case FX_ROOST:       snprintf(bt.msg, sizeof(bt.msg), "%s landed to rest!", yo); break;
            case FX_DEFOG:       snprintf(bt.msg, sizeof(bt.msg), "The field was cleared!"); break;
            // Los tres del lote que estaba fuera de la tabla y que
            // no cambian ninguna barra: sin frase parecen un turno
            // tirado a la basura.
            case FX_REFRESH:     snprintf(bt.msg, sizeof(bt.msg), "%s shook it off!", yo); break;
            case FX_RECYCLE:     snprintf(bt.msg, sizeof(bt.msg), "%s got its berry back!", yo); break;
            case FX_MIRACLE_EYE: snprintf(bt.msg, sizeof(bt.msg), "%s was identified!", el); break;
            case FX_HIT_AND_SEED: snprintf(bt.msg, sizeof(bt.msg), "%s was seeded!", el); break;
            case FX_SALT_CURE:   snprintf(bt.msg, sizeof(bt.msg), "%s is being salted!", el); break;
            case FX_AQUARING:    snprintf(bt.msg, sizeof(bt.msg), "%s made a veil!", yo); break;
            case FX_DISABLE:     snprintf(bt.msg, sizeof(bt.msg), "%s was disabled!", el); break;
            case FX_ENCORE:      snprintf(bt.msg, sizeof(bt.msg), "%s got an encore!", el); break;
            case FX_SPITE:       snprintf(bt.msg, sizeof(bt.msg), "%s lost some PP!", el); break;
            // LAZO DESTINO y RENCOR: los dos MARCAN y no pasa nada
            // visible hasta que el marcado cae. Sin la frase, usarlos
            // parece gastar el turno en nada —— que es justo lo que
            // canto la guarda de auditoria.py al implementarlos.
            case FX_DESTINY_BOND:
              snprintf(bt.msg, sizeof(bt.msg), "%s is taking you down!", yo);
              break;
            case FX_GRUDGE:
              snprintf(bt.msg, sizeof(bt.msg), "%s bears a grudge!", yo);
              break;
            // Los dos que COPIAN. Sin estas lineas la guarda de
            // auditoria.py salto en cuanto se implementaron, y con
            // razon: el Ditto se transformaba y en pantalla no pasaba
            // nada, que es exactamente como se ve un golpe que fallo.
            case FX_TRANSFORM:
              // `yo` NO sirve aqui: sale de asDex, que la transformacion
              // acaba de cambiar, y diria "Bulbasaur se transformo en
              // Bulbasaur". El nombre de verdad del que copia vive en
              // `dex`, que es justo para esto que esta separado.
              snprintf(bt.msg, sizeof(bt.msg), "%s transformed into %s!",
                       dexName(bt.b.side[bt.acting].dex), el);
              break;
            case FX_SKETCH:
              snprintf(bt.msg, sizeof(bt.msg), "%s sketched %s's move!", yo, el);
              break;
            // Los diecinueve que se aplicaban EN SILENCIO. Los encontre
            // comparando los FX que el motor marca con los que esta
            // lista sabia contar: eran 34 contra 15. Una trampa que se
            // pone sin que nadie lo diga parece un golpe que no hizo
            // nada, y el jugador no tiene forma de enterarse.
            case FX_TAUNT:       snprintf(bt.msg, sizeof(bt.msg), "%s fell for the taunt!", el); break;
            case FX_TORMENT:     snprintf(bt.msg, sizeof(bt.msg), "%s was tormented!", el); break;
            case FX_IMPRISON:    snprintf(bt.msg, sizeof(bt.msg), "%s sealed its moves!", yo); break;
            case FX_FORESIGHT:   snprintf(bt.msg, sizeof(bt.msg), "%s was identified!", el); break;
            case FX_TRAP:        snprintf(bt.msg, sizeof(bt.msg), "%s can't escape!", el); break;
            case FX_ROAR:        snprintf(bt.msg, sizeof(bt.msg), "%s was blown away!", el); break;
            case FX_DRAGONTAIL:  snprintf(bt.msg, sizeof(bt.msg), "%s was dragged out!", el); break;
            case FX_MIST:        snprintf(bt.msg, sizeof(bt.msg), "%s is shrouded in mist!", yo); break;
            case FX_HEAL_BELL:   snprintf(bt.msg, sizeof(bt.msg), "A bell chimed!"); break;
            case FX_GASTRO:      snprintf(bt.msg, sizeof(bt.msg), "%s's ability was suppressed!", el); break;
            case FX_ENTRAIN:     snprintf(bt.msg, sizeof(bt.msg), "%s got a new ability!", el); break;
            case FX_BATON:       snprintf(bt.msg, sizeof(bt.msg), "%s passed it on!", yo); break;
            case FX_TAILWIND:    snprintf(bt.msg, sizeof(bt.msg), "A tailwind blew!"); break;
            case FX_TRICKROOM:
              snprintf(bt.msg, sizeof(bt.msg), bt.b.trickRoom
                       ? "The dimensions were twisted!"
                       : "The twisted dimensions returned to normal!");
              break;
            case FX_YAWN:        snprintf(bt.msg, sizeof(bt.msg), "%s grew drowsy!", el); break;
            case FX_NORETREAT:   snprintf(bt.msg, sizeof(bt.msg), "%s can no longer escape!", yo); break;
            case FX_SUBSTITUTE:  snprintf(bt.msg, sizeof(bt.msg), "%s put in a substitute!", yo); break;
            case FX_WISH:        snprintf(bt.msg, sizeof(bt.msg), "%s made a wish!", yo); break;
            case FX_PERISH:      snprintf(bt.msg, sizeof(bt.msg), "All who heard the song will faint in 3 turns!"); break;
            case FX_EMBARGO:     snprintf(bt.msg, sizeof(bt.msg), "%s can't use items anymore!", el); break;
            case FX_HEALBLOCK:   snprintf(bt.msg, sizeof(bt.msg), "%s was prevented from healing!", el); break;
            case FX_WATERSPORT:  snprintf(bt.msg, sizeof(bt.msg), "Fire's power weakened!"); break;
            case FX_MUDSPORT:    snprintf(bt.msg, sizeof(bt.msg), "Electricity weakened!"); break;
            case FX_ROCKS:       snprintf(bt.msg, sizeof(bt.msg), "Rocks float around %s!", el); break;
            case FX_SPIKES:      snprintf(bt.msg, sizeof(bt.msg), "Spikes litter %s's side!", el); break;
            case FX_TSPIKES:     snprintf(bt.msg, sizeof(bt.msg), "Toxic spikes were laid!"); break;
            case FX_WEB:         snprintf(bt.msg, sizeof(bt.msg), "A sticky web was laid!"); break;
            case FX_SOAK:        snprintf(bt.msg, sizeof(bt.msg), "%s became Water-type!", el); break;
            case FX_WORRY_SEED:  snprintf(bt.msg, sizeof(bt.msg), "%s can't sleep now!", el); break;
            case FX_LUCKY_CHANT: snprintf(bt.msg, sizeof(bt.msg), "No more critical hits!"); break;
            case FX_ATTRACT:     snprintf(bt.msg, sizeof(bt.msg), "%s fell in love!", el); break;
            case FX_MAGNET_RISE: snprintf(bt.msg, sizeof(bt.msg), "%s is floating!", yo); break;
            case FX_QUICK_GUARD: snprintf(bt.msg, sizeof(bt.msg), "%s is braced for speed!", yo); break;
            default:             bt.msg[0] = 0; break;
          }
          if (bt.msg[0]) { bt.until = now + 900; break; }
        }
      }
      if (bt.extraStep == 1) {
        // LOS OBJETOS que cambian de manos. El motor lo marcaba desde hace
        // tiempo y la pantalla no decia nada: te quedabas sin objeto sin
        // saber cuando. Se queda en el paso 1 hasta haberlo dicho.
        if (bt.last.itemSwap) {
          bt.last.itemSwap = false;
          bt.last.itemStolen = bt.last.itemLost = IT_NONE;
          snprintf(bt.msg, sizeof(bt.msg), "%s switched items!",
                   dexName(bt.b.side[bt.acting].pareceDex()));
          bt.until = now + 900;
          break;
        }
        if (bt.last.itemStolen != IT_NONE) {
          snprintf(bt.msg, sizeof(bt.msg), "%s stole %s!",
                   dexName(bt.b.side[bt.acting].pareceDex()),
                   itemAt(bt.last.itemStolen).name);
          bt.last.itemStolen = IT_NONE;
          bt.until = now + 900;
          break;
        }
        if (bt.last.itemLost != IT_NONE) {
          // Desarme se lo tira al otro; Lanzamiento tira el propio.
          uint8_t de = bt.last.itemLostSelf ? bt.acting : (uint8_t)(bt.acting ^ 1);
          snprintf(bt.msg, sizeof(bt.msg), "%s lost %s!",
                   dexName(bt.b.side[de].pareceDex()),
                   itemAt(bt.last.itemLost).name);
          bt.last.itemLost = IT_NONE;
          bt.until = now + 900;
          break;
        }
        bt.extraStep = 2;
        if (bt.last.recoil) {
          // "¡X ha fallado y se ha caido al suelo!" del Anil.
          snprintf(bt.msg, sizeof(bt.msg),
                   bt.last.crashed ? "%s kept going and crashed!"
                                   : "%s took recoil!",
                   dexName(bt.b.side[bt.acting].pareceDex()));
          bt.until = now + 900;
          break;
        }
      }
      if (bt.extraStep == 2) {
        bt.extraStep = 3;
        if (btStatusMsg(bt.last)) { bt.until = now + 900; break; }
      }
      if (bt.extraStep == 3) {
        bt.extraStep = 4;
        if (bt.last.trapped) {
          snprintf(bt.msg, sizeof(bt.msg), "%s can't escape!",
                   dexName(bt.b.side[bt.acting ^ 1].pareceDex()));
          bt.until = now + 900;
          break;
        }
      }
      if (bt.extraStep == 4) {
        bt.extraStep = 9;
        if (bt.last.selfConfused) {
          snprintf(bt.msg, sizeof(bt.msg), "%s became confused!",
                   dexName(bt.b.side[bt.acting].pareceDex()));
          bt.until = now + 900;
          break;
        }
      }
      bt.msg[0] = 0;
      // Cayo el tuyo pero te queda banquillo: knockOut() dejo `over` en
      // false a proposito, y aqui hay que ir igualmente al desmayo â€” de ahi
      // sale la lista de relevos en vez del fin del combate.
      if (bt.b.over || btCaidoAgora(BT_PLAYER) || btCaidoAgora(BT_FOE)) {
        // El de ENFRENTE tambien cuenta, y faltaba. Contra un salvaje no
        // se notaba: al tumbarlo, knockOut() pone `over` y entra por la
        // primera condicion. Pero un ENTRENADOR con banquillo deja
        // `over` en false a proposito -- para poder sacar al siguiente --
        // asi que su bicho caia, esto no lo miraba, y el turno seguia
        // con el segundo golpe pendiente... contra el que acababa de
        // entrar. Un ataque de regalo por cada relevo.
        //
        // El pendiente se tira: su objetivo ya no esta en el campo.
        bt.secondPending = false;
        bt.phase = BP_FAINT;
        bt.until = now + 700;
        bt.fainter = bt.b.side[BT_PLAYER].fainted() ? BT_PLAYER : BT_FOE;
        sfxPlay(SFX_DENY);
      } else if (bt.pvp && pvpMeioDoTurno()) {
        // U-turn/Baton Pass pediram relevo, ou Rugido arrastou: ja tratado
      } else if (bt.secondPending) {
        bt.secondPending = false;
        // le toca al que no ha pegado todavia este turno
        if (bt.pvp) { if (!pvpSegue()) bt.until = now; }
        else btLaunch(bt.acting ^ 1, bt.acting == BT_PLAYER ? bt.foeSlot : bt.sel);
      } else if (bt.b.yawnDurmio != 0xFF) {
        // Estos dos avisos tienen paso propio: dentro del paso de endTurn(),
        // su `break` se comia la frase del veneno de ese mismo bicho.
        snprintf(bt.msg, sizeof(bt.msg), "%s fell asleep!",
                 dexName(bt.b.side[bt.b.yawnDurmio].pareceDex()));
        bt.b.yawnDurmio = 0xFF;
        bt.until = now + 900;
      } else if (bt.b.encoreFin != 0xFF) {
        snprintf(bt.msg, sizeof(bt.msg), "%s's encore ended!",
                 dexName(bt.b.side[bt.b.encoreFin].pareceDex()));
        bt.b.encoreFin = 0xFF;
        bt.until = now + 900;
      } else if (bt.endStep < 2) {
        // Los dos ya se han movido: ahora cobran el veneno y la quemadura, uno
        // por uno para que se vea de quien es cada mordisco.
        uint8_t who = bt.endStep;
        bt.endStep++;
        // hay que mirar el agarre y el cielo ANTES: endTurn() descuenta los dos
        bool atrapado = bt.b.side[who].bindTurns > 0;
        uint8_t cielo = bt.b.weather;
        uint16_t d = bt.b.endTurn(who);
        if (bt.b.endAbil) {
          // Shed Skin y Speed Boost: si no se anuncian, el estado desaparece
          // o la velocidad sube sin que nadie sepa por que
          snprintf(bt.msg, sizeof(bt.msg), "%s's %s!",
                   dexName(bt.b.side[who].pareceDex()), abilAt(bt.b.endAbil).name);
          bt.until = now + 900;
          break;
        }
        // PREMONICION: el golpe que salio hace dos turnos acaba de caer.
        // Va ANTES del desgaste porque es lo mas raro de ver: un bicho que
        // pierde PS sin que nadie le haya pegado parece un fallo del juego.
        // DESEO cumplido y CUENTA del Canto Mortal: como la Premonicion,
        // cosas que pasan sin que nadie pegue y que hay que decir.
        if (bt.b.wishHealOn == who && bt.b.wishHealAmt) {
          snprintf(bt.msg, sizeof(bt.msg), "%s's wish came true!",
                   dexName(bt.b.side[who].pareceDex()));
          bt.b.wishHealAmt = 0;
          bt.until = now + 900;
          break;
        }
        if (bt.b.perishTick == who && !bt.b.side[who].fainted()) {
          snprintf(bt.msg, sizeof(bt.msg), "%s's perish count fell to %u!",
                   dexName(bt.b.side[who].pareceDex()), (unsigned)bt.b.side[who].perish + 1u);  // o Anil conta 3, 2, 1
          bt.b.perishTick = 0xFF;
          bt.until = now + 900;
          break;
        }
        if (bt.b.futureHitOn != 0xFF && bt.b.futureHitDmg) {
          snprintf(bt.msg, sizeof(bt.msg), "%s took the Future Sight attack!",
                   dexName(bt.b.side[bt.b.futureHitOn].pareceDex()));
          bt.b.futureHitDmg = 0;        // contado ya
          bt.until = now + 900;
          break;
        }
        if (d) {
          const Combatant &c = bt.b.side[who];
          // el orden importa: sin esto, el desgaste de la tormenta de arena
          // se anunciaba como veneno, que es lo primero que caia en el else
          const char *culpa =
              atrapado                 ? "the hold"
            : c.cursed                 ? "the curse"
            : c.status == ST_BURN      ? "its burn"
            : c.status != ST_NONE      ? "poison"
            : c.leechSeed              ? "the seed"
            : cielo == WE_SAND         ? "the sandstorm"
            : cielo == WE_HAIL         ? "the hail"
                                       : "the weather";
          snprintf(bt.msg, sizeof(bt.msg), "%s is hurt by %s", dexName(c.pareceDex()), culpa);
          bt.until = now + 900;
        } else if (bt.b.endHeal) {
          // LO QUE CURA, que hasta ahora no lo decia nadie: los Restos
          // curaban en silencio y a nivel bajo son uno o dos PS, asi que
          // desde fuera el objeto parecia roto. El dueno lo dio por roto.
          //
          // Va en el ramo de "nada que contar" A PROPOSITO: delante del
          // desgaste, un bicho envenenado CON Restos perderia la frase
          // del veneno, que es la que de verdad hace falta. Asi la cura
          // se anuncia justo cuando es lo unico que paso.
          const Combatant &q = bt.b.side[who];
          if (bt.b.endHealItem)
            snprintf(bt.msg, sizeof(bt.msg), "%s's %s restored HP!",
                     dexName(q.pareceDex()), itemAt(bt.b.endHealItem).name);
          else
            snprintf(bt.msg, sizeof(bt.msg), "%s restored HP!",
                     dexName(q.pareceDex()));
          bt.until = now + 900;
        } else {
          bt.until = now;   // nada que contar: al siguiente en el proximo tick
        }
      } else {
        bt.endStep = 0;
        bt.phase = BP_CHOOSE;
      }
      break;

    case BP_FAINT:
      if (bt.pvp) { pvpDesmaio(); break; }
      snprintf(bt.msg, sizeof(bt.msg), "%s fainted!", dexName(bt.b.side[bt.fainter].pareceDex()));
      bt.until = now + 1800;
      if (bt.fainter == BT_FOE) {
        btFoeDefeated();       // cada uno que cae paga lo suyo
        if (bt.foesLeft) bt.foesLeft--;
        // Un entrenador saca al siguiente: eso es lo que sube el liston,
        // no que sus bichos sean de mas nivel. Tu llegas al segundo con los
        // PS y los PP que te quedaron del primero.
        if (bt.foesLeft > 0) { btNextFoe(); break; }
      } else if (!bt.b.over && btReserveCount() > 0) {
        // El tuyo cayo y te queda equipo: no se ha acabado nada, toca
        // decidir a quien sacas. Es el otro lado del relevo â€” el que no se
        // elige â€” y por eso la lista no se puede cerrar.
        btStoreActive();            // deja sus 0 PS guardados donde toca
        bt.mustSwitch = true;
        bt.partyScroll = 0;
        bt.phase = BP_PARTY;
        sfxPlay(SFX_DENY);
        break;
      }
      bt.phase = btCobrarBotin() ? BP_LOOT
               : (bt.b.winner == BT_PLAYER ? BP_WIN : BP_END);
      sfxPlay(bt.b.winner == BT_PLAYER ? SFX_MEDAL : SFX_BYE);
      // Ganar tiene fanfarria, y no la misma: la de un lider no puede sonar
      // igual que la de una rata del camino. Perder solo corta la musica.
      musicStop();
      if (bt.b.winner == BT_PLAYER)
        jinglePlay(bt.gym >= 0 ? JIN_WIN_BIG
                 : bt.trainer >= 0 ? JIN_WIN_TRAINER : JIN_WIN_WILD);
      break;

    case BP_LOOT:
      // Un objeto por pantalla, con su tiempo para leerlo. Cuando se
      // acaban, se cierra.
      if (millis() < bt.until) break;
      bt.lootShown++;
      if (bt.lootShown < 2 && bt.loot[bt.lootShown]) {
        snprintf(bt.msg, sizeof(bt.msg), "Got %s!",
                 itemAt(bt.loot[bt.lootShown]).name);
        bt.until = millis() + BT_LOOT_MS;
      } else {
        bt.msg[0] = 0;
        // Del objeto al resumen; si perdiste, directo al final.
        bt.phase = (bt.b.winner == BT_PLAYER) ? BP_WIN : BP_END;
        bt.until = millis() + 300;
      }
      break;

    case BP_WIN:
      // Se queda hasta que el jugador toque. Ponerle reloj seria repetir
      // el fallo que esto viene a arreglar: la pantalla que cuenta lo que
      // has ganado no puede irse sola antes de que la leas.
      break;

    case BP_END:
      battleClose();
      break;

    case BP_ESPERA:
      if (bt.pvp) pvpEsperaTick(now);
      break;

    default:
      break;
  }
}

// ---------------------------------------------------------------- dibujo
// Chip de estado, junto al nombre. En la barra no cabe mas que UNA etiqueta,
// asi que en vez de recortar informacion va ROTANDO entre todo lo que este
// activo: el estado principal y cada stage que no sea cero. Un bicho quemado
// con el ataque subido enseÃ±a BRN, luego ATK+, y vuelta a empezar.
static void btStatusChip(int x, int y, const Combatant &c) {
  const char *etiq[8];
  uint16_t col[8];
  uint8_t n = 0;

  // Los colores son los de UI/statuses.png del Anil, medidos uno a uno. La
  // hoja en si no sirve (el texto esta en espanol: DOR, QUE, ENV), pero la
  // paleta y la forma de la pastilla si, y es lo que hace que se reconozca.
  switch (c.status) {
    case ST_BURN:      etiq[n] = "BRN"; col[n++] = C565(0xe0, 0x58, 0x48); break;
    case ST_POISON:    etiq[n] = "PSN"; col[n++] = C565(0xc0, 0x40, 0xc8); break;
    case ST_BADPOISON: etiq[n] = "TOX"; col[n++] = C565(0xa0, 0x48, 0xc8); break;
    case ST_PARALYSIS: etiq[n] = "PAR"; col[n++] = C565(0xe8, 0xa8, 0x00); break;
    case ST_SLEEP:     etiq[n] = "SLP"; col[n++] = C565(0x68, 0x70, 0x60); break;
    case ST_FREEZE:    etiq[n] = "FRZ"; col[n++] = C565(0x30, 0xc8, 0xc8); break;
    default: break;
  }
  if (c.confuseTurns) { etiq[n] = "CNF"; col[n++] = C565(0x00, 0x98, 0x98); }

  // Uno por cada SG_*, y el static_assert lo vigila: esta tabla tenia el
  // mismo agujero que la de los mensajes y con la evasion tocada habria
  // impreso basura en el chip.
  static const char *const NOM[] = { "ATK", "DEF", "SPA", "SPD", "SPE",
                                     "ACC", "EVA" };
  static_assert(sizeof(NOM) / sizeof(NOM[0]) == SG_COUNT,
                "falta la etiqueta de algun stage");
  char buf[SG_COUNT][6];
  for (uint8_t i = 0; i < SG_COUNT && n < 8; i++) {
    if (!c.stage[i]) continue;
    snprintf(buf[i], sizeof(buf[i]), "%s%c", NOM[i], c.stage[i] > 0 ? '+' : '-');
    etiq[n] = buf[i];
    col[n++] = c.stage[i] > 0 ? UI_BAR_OK : UI_BAR_BAD;
  }
  if (!n) return;

  uint8_t i = (uint8_t)((millis() / 1200) % n);   // una vuelta cada 1,2 s
  int w = (int)strlen(etiq[i]) * LATIN_ADV + 10;
  // La pastilla del Anil tiene un LABIO mas oscuro abajo: es lo que la hace
  // parecer un boton en vez de un rectangulo de color.
  gfx->fillRoundRect(x, y, w, 15, 5, darken565(col[i]));
  gfx->fillRoundRect(x, y, w, 12, 5, col[i]);
  gfx->setTextSize(1);
  gfx->setTextColor(UI_WHITE);
  gfx->setCursor(x + 5, y + 3);
  gfx->print(etiq[i]);
  gfx->setTextColor(INK_K);
}

// Cuanto le falta al que ESTA EN PISTA para subir, 0..1.
//
// No vale `pet.levelProgress()` a secas: peleando con un suplente, la
// barra contaria los puntos del principal, que es justo el que no esta
// ganando nada. La cuenta es la misma que la de Pet::levelProgress().
static float btExpProgress() {
  if (btIsMain(bt.mySlot)) return pet.levelProgress();
  const StoredMon &m = pet.team[bt.mySlot];
  uint16_t lv = Pet::levelOf(m);
  if (lv >= LEVEL_CAP) return 1.0f;
  uint32_t a = Pet::expForLevel(lv), b = Pet::expForLevel(lv + 1);
  return (b > a && m.exp >= a) ? (float)(m.exp - a) / (float)(b - a) : 0.0f;
}

// `conExp` solo para el tuyo: la experiencia del rival no es asunto de
// nadie, y la chapa de arriba ya va pegada al borde del circulo.
// La bolita de "este ya lo tienes". Dibujada a mano y no con
// ART_UI_BALL: esa arte mide 36x36 y la chapa entera tiene 36 de alto.
static void btMarcaCazado(int cx, int cy) {
  const int r = 6;
  gfx->fillCircle(cx, cy, r, UI_WHITE);
  // la mitad de arriba, roja
  for (int dy = -r; dy <= 0; dy++) {
    int dx = (int)sqrtf((float)(r * r - dy * dy));
    gfx->fillRect(cx - dx, cy + dy, dx * 2 + 1, 1, UI_BAR_BAD);
  }
  gfx->drawCircle(cx, cy, r, INK_K);
  gfx->fillRect(cx - r, cy, r * 2 + 1, 1, INK_K);
  gfx->fillCircle(cx, cy, 2, UI_WHITE);
  gfx->drawCircle(cx, cy, 2, INK_K);
}

static void btHpBar(int y, const Combatant &c, int lado, bool conExp = false,
                    bool cazado = false) {
  const int w = 182;
  int media = btHalf(y - 6) < btHalf(y + 34) ? btHalf(y - 6) : btHalf(y + 34);
  int x = (lado < 0) ? (233 - media + 10) : (233 + media - 10 - w);

  // Con barra de EXP la chapa crece 9 px: la de PS acaba en y+26 y el
  // papel blanco en y+29, o sea que no habia sitio debajo.
  int alto = conExp ? 45 : 36;
  gfx->fillRoundRect(x - 4, y - 4, w + 8, alto, 9, INK_K);
  gfx->fillRoundRect(x - 1, y - 1, w + 2, alto - 6, 7, UI_WHITE);
  gfx->setTextColor(INK_K);
  gfx->setTextSize(1);
  // La estrella de shiny va en la chapa, como en los juegos, y no en el texto
  // del anuncio: el anuncio pasa y la chapa se queda ahi todo el combate.
  // Solo por la especie de VERDAD: un Ditto transformado en un shiny no lo es.
  int nx = x + 8;
  if (c.shiny && c.asDex == c.dex) {
    drawShinyStar(ART_SHINY_S, x + 7 + ART_SHINY_S.w / 2, y + 9);
    nx = x + 8 + ART_SHINY_S.w + 2;
  }
  // Y la bolita de "este ya lo tienes", al lado de la estrella: las dos
  // dicen algo de ESTE bicho y las dos se quedan toda la pelea, al reves
  // que el anuncio, que pasa.
  if (cazado) {
    btMarcaCazado(nx + 6, y + 9);
    nx += 15;
  }
  gfx->setCursor(nx, y + 5);
  // El nombre que toca: la MEGA manda, luego la FORMA, y si no la
  // especie. Decia siempre el de la especie, asi que un Charizard X se
  // anunciaba como "CHARIZARD" y un Vulpix de Alola como "VULPIX" — el
  // sprite y los tipos cambiaban y la chapa no se enteraba.
  gfx->print(c.isMega() ? MEGA_TBL[c.megaIdx].name
                        : dexNameForm(c.pareceDex(), c.pareceForm()));
  char lv[10];
  snprintf(lv, sizeof(lv), "Lv%u", (unsigned)c.level);
  gfx->setCursor(x + w - 8 - (int)strlen(lv) * LATIN_ADV, y + 5);
  gfx->print(lv);

  gfx->fillRect(x + 8, y + 18, w - 16, 8, UI_TRACK);
  // Los PS que se ENSENAN: al que esta recibiendo el golpe se le suma lo
  // que aun no ha "caido" (bt.hpPend), asi la barra baja con cada impacto
  // y no de golpe al anunciar el golpe.
  uint32_t hpV = c.hp;
  if (bt.hpPend && &c == &bt.b.side[bt.acting ^ 1] &&
      (bt.phase == BP_ATTACK || bt.phase == BP_HIT || bt.phase == BP_CARGA)) {
    hpV = (uint32_t)c.hp + bt.hpPend;
    if (hpV > c.hpMax) hpV = c.hpMax;
  }
  int lleno = c.hpMax ? (int)((uint32_t)(w - 16) * hpV / c.hpMax) : 0;
  uint16_t col = (hpV * 2 > c.hpMax) ? UI_BAR_OK
                 : (hpV * 5 > c.hpMax) ? UI_BAR_WARN : UI_BAR_BAD;
  if (lleno > 0) gfx->fillRect(x + 8, y + 18, lleno, 8, col);

  // La EXPERIENCIA, debajo de los PS y mas fina, como en los juegos.
  //
  // Existia solo en una subpagina de la ficha, a la que hay que llegar
  // deslizando: jugando no se veia NUNCA cuanto faltaba para subir. Y es
  // de lo poco que avanza en cada combate, asi que no verlo es no ver lo
  // que estas consiguiendo. Azul y no verde para que no se confunda de un
  // vistazo con la de PS, que es la que importa mientras peleas.
  if (conExp) {
    int ey = y + 29, eh = 5;
    gfx->fillRect(x + 8, ey, w - 16, eh, UI_TRACK);
    int ancho = (int)((w - 16) * btExpProgress());
    if (ancho > 0) gfx->fillRect(x + 8, ey, ancho, eh, C565(0x3c, 0xa8, 0xe0));
  }

  // El chip va ANTES del nivel, no encima: la barra mide 182 y con el nombre
  // (60 como mucho), el chip (28) y el "Lv61" (24) queda sitio de sobra, pero
  // solo si cada cosa tiene el suyo.
  btStatusChip(x + w - 78, y + 3, c);
}

static void btDrawSide(PmdMon &m, const Combatant &c, int cx, int ground,
                       bool back, float extra, float hundir,
                       float tragado = 0.0f, float volar = 0.0f) {
  // De los cuatro portazos, los que son "ya no esta en el campo" dejan
  // dicho que no se pinto nada (alto 0). `tragado` NO: la bola de la
  // captura se coloca a la altura del bicho que acaba de tragarse, y
  // borrarla la mandaria a la constante de reserva. `m.loaded` tampoco,
  // que es "falta el fichero" y no "se escondio".
  const uint8_t QUIEN = back ? BT_PLAYER : BT_FOE;
  if (!m.loaded) return;
  if (hundir >= 1.0f) { btAltoDibujado[QUIEN] = 0; return; }  // ya se fue
  if (tragado >= 1.0f) return; // dentro de la bola: no esta en el campo
  // VUELO: se fue por ARRIBA, y tampoco esta.
  if (volar >= 1.0f) { btAltoDibujado[QUIEN] = 0; return; }
  uint8_t act = back ? PMD_SLEEP : PMD_IDLE;   // la vista de espaldas vive en SLEEP
  if (!m.has(act)) act = PMD_IDLE;

  PetFx fx;
  // PERSPECTIVA, y SOLO perspectiva.
  //
  // DEX_TBL[].screenSz es el alto de CUERPO que pide la especie en la
  // pantalla de casa (gen_sizes.py). Aqui lo unico que se anade es que el
  // tuyo esta delante y el rival al fondo. Nada mas: ni normalizar, ni
  // topes por rol, ni correcciones.
  //
  // Los topes por rol SI estaban, y hacian dano: el del rival (120 px)
  // aplastaba a 475 de las 1025 especies contra el mismo numero, asi que
  // la mitad del juego salia exactamente igual de grande —— un Caterpie
  // como un Onix, y un Wailord mas pequeno que los dos. Ahora la banda de
  // la tabla (64..185) ya esta elegida para que quepa con estos factores,
  // y tools/audita_tamanos.py lo comprueba especie a especie en vez de
  // recortar en caliente.
  //
  //   de espaldas  suelo 312, tope por la chapa de PS del rival (y=95):
  //                185 * 1,05 = 194 px de los 217 que hay
  //   de frente    suelo 178, tope por el circulo: en x=325 caben 137 y
  //                185 * 0,72 = 133
  const PmdAct &ac = m.acts[act];
  // Leva 70: el alto de lo que se VE. Un Wishiwashi en banco no mide lo
  // que el pez solo, y una Ilusion mide lo que el bicho que imita.
  uint8_t altoVisto = (c.bform >= 0 && !c.transformed) ? BFORM_TBL[c.bform].screenSz
                    : DEX_TBL[c.pareceDex()].screenSz;
  float quiere = altoVisto * (back ? BT_BACK_K : BT_FOE_K);
  // `ac` y no el idle: de espaldas se dibuja PMD_SLEEP, cuyo lienzo difiere
  // del idle en 904 de las 1025 especies. Escalando con el idle, Dondozo
  // pedia 145 px y salia a 214, y Arboliva pedia 180 y salia a 124.
  uint8_t s = escalaDibujo(m, ac, 5, quiere);
  // El hueco de cada lado, en alto. De espaldas el limite es la chapa de
  // PS del rival (acaba en y=95) sobre un suelo en 312; de frente, el
  // circulo de la pantalla, que en x=325 deja 137 px.
  float k = factorTinta(ac, s, quiere, back ? 215 : 137);
  // E QUE CAIBA DE LARGO. So o de costas: e o que fica entre a borda e a
  // placa. Encolhe so o que falta (1,5% a mais pela respiracao, que
  // estica o X), e quem cabe nao muda nada.
  if (back && ac.inkR > ac.inkL) {
    float meio = ac.w * 0.5f;
    float dir = (ac.inkR - meio) * s * 1.015f;   // px de tinta a direita do centro, k=1
    float esq = (meio - ac.inkL) * s * 1.015f;
    if (dir > 0 && cx + dir * k > BT_BACK_X1) k = (BT_BACK_X1 - cx) / dir;
    if (esq > 0 && cx - esq * k < BT_BACK_X0) k = (cx - BT_BACK_X0) / esq;
  }
  if (back) {
    btLadosCostas[0] = (int16_t)(cx - (ac.w * 0.5f - ac.inkL) * s * k);
    btLadosCostas[1] = (int16_t)(cx + (ac.inkR - ac.w * 0.5f) * s * k);
  }
  fx.sx = fx.sy = k;

  // respiracion de fondo, y encima lo que pida la fase
  float r = sinf(cyclePhase(millis(), 3000));
  fx.sy *= 1.0f - 0.02f * r;
  fx.sx *= 1.0f + 0.015f * r;
  fx.dx += extra;

  // Lo esta tragando la bola: se encoge hacia ella. No hay alfa en el blit,
  // asi que no puede desvanecerse â€” se encoge, que ademas es lo que hacen
  // los juegos. Sin esto el bicho se quedaba entero y quieto al lado de su
  // propia bola, que es justo lo que delataba que no pasaba nada.
  if (tragado > 0.0f) {
    float q = 1.0f - tragado;
    fx.sx *= q;
    fx.sy *= q;
    fx.dy += tragado * 26.0f;   // y baja hacia la boca de la bola
  }

  // Al caer se hunde DENTRO del suelo: baja, y todo lo que pase de la linea
  // del suelo deja de dibujarse. Antes se pintaba entero y se repintaba la
  // plataforma encima, pero la plataforma es una elipse y el bicho salia por
  // debajo de ella: parecia que atravesaba el suelo en vez de meterse en el.
  // Se aplasta un poco por el camino para que no parezca un ascensor.
  if (hundir > 0.0f) {
    fx.dy += hundir * (ac.inkH() * s * k + 24);
    fx.sy *= 1.0f - 0.25f * hundir;
    fx.sx *= 1.0f + 0.10f * hundir;
    fx.clipY = (int16_t)ground;
  }

  // VUELO y REBOTE: ese se va por ARRIBA. Mismo papel que `hundir` pero
  // en el otro sentido —— sube y se encoge hasta no estar ——, y no se
  // recorta contra el suelo porque no va hacia el. Sin esto el motor lo
  // daba por fuera del campo (Combatant::hiding) mientras el sprite
  // seguia plantado en su sitio: el golpe del rival lo atravesaba.
  if (volar > 0.0f) {
    fx.dy -= volar * 190.0f;
    float q = 1.0f - 0.70f * volar;
    fx.sx *= q;
    fx.sy *= q;
  }

  // A ANIMACAO DO ANIL manda no bicho: desloca, estica, apaga e tinge.
  // Menos no lado que ela deixa LIVRE (golpe em si mesmo ou sem alvo): no
  // Anil esse sprite nem entra no tocador, entao fica como esta.
  if (gAnimOn && !(animMove.livre & (QUIEN == BT_PLAYER ? 1 : 2))) {
    const uint8_t *ce = gAnimSpr[QUIEN];
    if (!ce) return;               // sem cel neste quadro: nao aparece
    int16_t dx, dy, tom[4];
    uint16_t zx, zy;
    memcpy(&dx, ce + 2, 2);
    memcpy(&dy, ce + 4, 2);
    memcpy(&zx, ce + 6, 2);
    memcpy(&zy, ce + 8, 2);
    memcpy(tom, ce + 18, 8);
    if (!ce[12] || !zx || !zy) return;   // opacidade ou zoom zero
    fx.dx += dx;
    fx.dy += dy;
    fx.sx *= zx / 100.0f;
    fx.sy *= zy / 100.0f;
    gTinta.op = ce[12];
    memcpy(gTinta.cor, ce + 14, 4);
    memcpy(gTinta.tom, tom, 8);
    gTinta.on = gTinta.op < 255 || gTinta.cor[3] || tom[0] || tom[1] ||
                tom[2] || tom[3];
  }

  // Sombra atada a la HUELLA y al frame que se esta viendo.
  //
  // La version anterior sacaba el ancho del lienzo (0,28 x w) y lo dejaba
  // clavado en el suelo. Dos cosas fallaban a la vez: Aerodactyl ocupa 93 px
  // de lienzo y apoya en 7, asi que tenia una mancha trece veces mas ancha
  // que su punto de apoyo; y como `base` es el minimo de TODA la accion, en
  // los battlers que flotan el bicho se despega hasta 26 filas del suelo (x5
  // de escala = media pantalla) mientras la sombra se quedaba abajo.
  //
  // Ahora la sombra sigue en el suelo -que es donde va una sombra-, pero se
  // ENSANCHA y se ACLARA con la altura. Eso es lo que convierte "esta mal
  // alineado" en "esta volando": el ojo lee la distancia en la sombra.
  float px = s * k;   // pixeles de pantalla por pixel de sprite

  float vuelo = ac.liftAt(pmdFrameAt(ac, millis(), true)) * px / 40.0f;
  if (vuelo > 1.0f) vuelo = 1.0f;

  int cuerpo = (int)((ac.footR - ac.footL + 1) * px);
  // algo mas estrecha que el cuerpo (la luz viene de arriba), y se ensancha al
  // subir; nunca por debajo de 10 px o deja de leerse como sombra
  // La sombra se va con el que se va, por abajo o por arriba: una sombra
  // sola en el suelo seria justo el adorno que delataba el fallo.
  int sw = (int)(cuerpo * (0.72f + 0.40f * vuelo) * (1.0f - hundir) *
                 (1.0f - volar));
  if (sw < 10 && hundir < 0.5f && volar < 0.5f) sw = 10;
  // El cuerpo no tiene por que caer en el centro del lienzo —— y el
  // APOYO tampoco cae en el centro del cuerpo. Se centra donde el
  // bicho toca el suelo, no en el medio de su tinta: con lo segundo,
  // 819 de las 1025 especies tenian la mancha a 3 px o mas del pie
  // (mediana 12, Lugia 97). El ANCHO si sale del cuerpo, a proposito.
  int sx = cx + (int)((ac.centroPie() - ac.w * 0.5f) * px);
  // tintEllipse escala el pixel por num/den: cuanto MENOR el cociente, mas
  // oscuro. Apoyado va oscuro y en el aire claro, no al reves.
  uint8_t dens = (uint8_t)(5.0f + 3.0f * vuelo);
  // A SILHUETA, como na casa. Some junto com o bicho que sai (hundir /
  // volar), que era o que o `sw` fazia com a elipse.
  (void)sx;
  if (sw > 0 && hundir < 0.5f && volar < 0.5f && !gSombra.desligada) {
    gSombra.on = true;
    gSombra.num = dens;
    gSombra.den = 10;
    drawPmdActM(m, act, cx, ground, millis(), true, false, (uint8_t)s, &fx, 0);
    gSombra.on = false;
  }

  // Los dos ultimos argumentos son el arreglo del fallo que trajo el
  // usuario: "quando aumenta o pokemon do lado do player, deixa o
  // adversario gigante".
  //
  //   maxS = s   el techo de escala es la escala que acabamos de calcular,
  //              asi que drawPmdActM dibuja EXACTAMENTE a `s`.
  //   dex  = 0   "el tamano ya se lo he dado yo". Sin esto, el valor por
  //              defecto es -1 = pet.speciesId, y el sprite del RIVAL
  //              acababa escalado por el screenSz del bicho del JUGADOR:
  //              el mismo Caterpie salia pequeno con un Caterpie delante,
  //              mediano con un Dratini y enorme con un Pidgeotto.
  //              Medido en el simulador, tres capturas del mismo rival.
  // LA MEGA, BLANCA: sube a blanco mientras dura la escena y baja del
  // blanco ya con el sprite nuevo (btMegaBlanco). Va encima de lo que
  // pusiera la animacion del golpe, que en la escena no corre.
  float blanco = btMegaBlanco(back ? btAbajo() : btArriba(), millis());
  if (blanco > 0.0f) {
    gTinta.on = true;
    gTinta.op = 255;
    gTinta.cor[0] = gTinta.cor[1] = gTinta.cor[2] = 255;
    gTinta.cor[3] = (uint8_t)(blanco * 255.0f + 0.5f);
    for (int k = 0; k < 4; k++) gTinta.tom[k] = 0;
  }
  drawPmdActM(m, act, cx, ground, millis(), true, false, (uint8_t)s, &fx, 0);
  gTinta.on = false;
  // Se copia DESPUES de pintar y de lo que escribio el dibujante: es la
  // unica medida que incluye lo que drawPmdActM anade por su cuenta.
  btPedido[back ? BT_PLAYER : BT_FOE] = (int16_t)(quiere + 0.5f);
  btAltoDibujado[back ? BT_PLAYER : BT_FOE] = ultimaMagPintada;
  btAltoReal[back ? BT_PLAYER : BT_FOE] = ultimoAltoPintado;
}

static void btMoveCard() {
  const Combatant &me = bt.b.side[btAbajo()];
  int w = btBandW(BT_CARD_Y0, BT_CARD_Y1);
  if (w > 272) w = 272;
  int x0 = 233 - w / 2, ym = (BT_CARD_Y0 + BT_CARD_Y1) / 2;

  drawWindow(x0, BT_CARD_Y0, w, BT_CARD_Y1 - BT_CARD_Y0, 15);

  // flechas: apagadas en los extremos, para que se vea que ya no hay mas
  uint16_t cIzq = bt.sel > 0 ? INK_K : UI_TRACK;
  uint16_t cDer = (bt.sel + 1 < me.nMoves) ? INK_K : UI_TRACK;
  gfx->fillTriangle(x0 + 34, ym - 14, x0 + 34, ym + 14, x0 + 14, ym, cIzq);
  gfx->fillTriangle(x0 + w - 34, ym - 14, x0 + w - 34, ym + 14, x0 + w - 14, ym, cDer);

  const bool sinGolpe = bt.b.debeForcejear(btAbajo());
  uint16_t mv = (!sinGolpe && bt.sel < me.nMoves) ? me.moves[bt.sel]
                                                  : MOVE_STRUGGLE;
  const MoveEntry &m = MOVE_TBL[mv];

  gfx->setTextSize(2);
  gfx->setTextColor(INK_K);
  int tw = (int)strlen(m.name) * 12;
  gfx->setCursor(233 - tw / 2, BT_CARD_Y0 + 12);
  gfx->print(m.name);

  // chip del tipo: el mismo de la Pokedex, con simbolo. Cabe de +32 a +58,
  // justo entre el nombre (acaba en +28) y la linea de POW (+60).
  int cw = (int)strlen(typeName(m.type)) * 6 + 42;
  drawTypeChip(233 - cw / 2, BT_CARD_Y0 + 32, cw, 26, m.type, 1);

  char izq[16], der[16];
  // Un golpe de apoyo no tiene potencia que enseÃ±ar, y "POW 0" se lee como
  // "este golpe esta roto" en vez de "este golpe hace otra cosa".
  if (m.power == 0) snprintf(izq, sizeof(izq), "SUPPORT");
  else              snprintf(izq, sizeof(izq), "POW %u", (unsigned)m.power);
  gfx->setTextColor(UI_INK);
  gfx->setCursor(233 - 62, BT_CARD_Y0 + 60);
  gfx->print(izq);

  // PP con los numeros de siempre, y en rojo cuando queda poco: ahora que el
  // PP atraviesa los combates hay que poder ver de un vistazo si se entra a
  // pelear con el tanque vacio.
  uint8_t us = (bt.sel < me.nMoves) ? me.usesLeft[bt.sel] : 0;
  uint8_t tope = movePP(mv);
  // ANULACION. El motor la tenia entera desde hace tiempo —— bloquea el
  // hueco cuatro turnos y tiene sus tests ——, pero no habia UNA SOLA linea
  // en pantalla que lo dijera: se podia elegir el golpe anulado, perder el
  // turno y leer "But it failed!" sin saber por que. Un mecanismo que el
  // jugador no puede ver no es una mecanica, es un bug con tests.
  const Combatant &yo = bt.b.side[btAbajo()];
  // Aqui solo se miraba Anular, asi que Mofa, Mordaza, Tormento, Sellar y
  // el Martillo Giga dejaban el hueco inservible SIN que nada en pantalla
  // lo dijera —— justo lo que el comentario de arriba dice que no puede
  // pasar. Con Anular se sigue enseñando la cuenta atras, que es lo unico
  // de los seis que tiene reloj.
  bool anulado = !sinGolpe && yo.disableTurns > 0 && bt.sel == yo.disabledSlot;
  bool cerrado = !sinGolpe && bt.b.slotBloqueado(btAbajo(), bt.sel) &&
                 yo.usesLeft[bt.sel];
  if (anulado) {
    snprintf(der, sizeof(der), "OFF %u", (unsigned)yo.disableTurns);
    gfx->setTextColor(UI_BAR_BAD);
  } else if (cerrado) {
    snprintf(der, sizeof(der), "OFF");
    gfx->setTextColor(UI_BAR_BAD);
  } else if (mv == MOVE_STRUGGLE) {
    snprintf(der, sizeof(der), "PP --");
    gfx->setTextColor(UI_INK);
  } else {
    snprintf(der, sizeof(der), "PP %u/%u", (unsigned)us, (unsigned)tope);
    gfx->setTextColor(us == 0 ? UI_BAR_BAD : (us * 4 <= tope ? UI_BAR_WARN : UI_INK));
  }
  gfx->setCursor(233 + 58 - (int)strlen(der) * 3, BT_CARD_Y0 + 60);
  gfx->print(der);

  // puntitos: cuantos golpes hay y cual esta elegido
  for (uint8_t i = 0; i < me.nMoves; i++) {
    int px = 233 - (me.nMoves - 1) * 8 + i * 16;
    // El anulado se ve ROJO aunque no sea el elegido: asi se sabe cual es
    // sin tener que pasar por los cuatro con las flechas.
    bool off = bt.b.slotBloqueado(btAbajo(), i) && yo.usesLeft[i];
    gfx->fillCircle(px, BT_CARD_Y1 - 13, i == bt.sel ? 4 : 3,
                    off ? UI_BAR_BAD : (i == bt.sel ? INK_K : UI_TRACK));
  }
}

// La caja que cuenta TODO lo que pasa en el combate: quien pego, que fue
// superefectivo y, al ganar, que te llevas. Iba siempre a talla 1, o sea
// seis pixeles de ancho por letra en una pantalla de 466: justo el sitio
// donde hay que poder leer sin acercar el ojo.
//
// Ahora la talla la elige el texto, no yo. Talla 2 en una linea si cabe,
// talla 2 en dos lineas si no, y talla 1 solo si el mensaje es largo de
// verdad (con msg[44] eso hoy no llega a pasar, pero si manana crece el
// buffer la caja no se rompe: se encoge la letra).
//
// La caja crece HACIA ARRIBA y el borde de abajo se queda en 396. Es lo
// que manda la pantalla redonda: a y=396 la cuerda deja 332 px de ancho y
// mas abajo se estrecha rapido, asi que bajar el suelo seria estrechar la
// caja. Subir el techo no cuesta nada.
static void btTextBox(const char *s) {
  int w = btBandW(330, 396);
  if (w > 300) w = 300;
  int largo = (int)strlen(s);
  uint8_t talla = 2;
  int cols = (w - 24) / (LATIN_ADV * 2);   // talla 2
  int lineas = largo > cols ? 2 : 1;
  if (largo > cols * 2) {          // ni en dos lineas: se baja la letra
    talla = 1;
    cols = (w - 24) / LATIN_ADV;
    lineas = largo > cols ? 2 : 1;
  }
  int lh = talla == 2 ? 20 : 12;
  int h = 22 + lineas * lh;
  int y = 396 - h;
  drawWindow(233 - w / 2, y, w, h, 13);
  gfx->setTextColor(INK_K);
  drawWrapped(s, y + 11, cols, lh, lineas, talla);
}

// Destello de shiny al salir a la pista, como en los juegos. Tres estallidos
// escalonados alrededor del bicho durante la entrada. No es un adorno: es el
// AVISO de que te ha salido uno raro, y llega antes de que te pongas a
// pegarle â€” la estrella de la chapa se ve si la buscas, esto no se pierde.
#define SPARK_MS 110
#define SPARK_LAST 380                                  // retardo del ultimo
#define SPARK_TOTAL_MS (SPARK_LAST + SPARK_MS * ART_SPARK_FRAMES)

static void btShinySpark(uint32_t t, int cx, int cy) {
  // dx, dy y el retardo de cada estallido, en ms
  static const int16_t B[3][3] = { { -34, 12, 0 }, { 30, -32, 190 },
                                   { -6, -64, SPARK_LAST } };
  for (auto &b : B) {
    int32_t e = (int32_t)t - b[2];
    if (e < 0) continue;
    uint32_t f = (uint32_t)e / SPARK_MS;
    if (f >= ART_SPARK_FRAMES) continue;
    const Art *a = ART_SPARK[f];
    if (a) drawArt(*a, cx + b[0], cy + b[1] + a->h / 2, 1);
  }
}

// El efecto del golpe, encima del bicho al que le toca. Uno por TIPO: en
// medio segundo lo que hay que leer es "esto fue fuego", no cual de los doce
// golpes de fuego era, y 437 animaciones no caben en ninguna parte.
// El efecto dura lo mismo sea cual sea el tipo (el golpe se ve durante
// BP_ATTACK + BP_HIT, 740 ms), asi que el paso sale de cuantos fotogramas
// tenga la hoja: las de 3 no pueden pasar en un parpadeo y las de 8 no pueden
// quedarse a medias.
#define FX_WINDOW_MS 620

static_assert(sizeof(FX_OF_TYPE) / sizeof(FX_OF_TYPE[0]) == T_COUNT,
              "FX_OF_TYPE se indexa por T_*: tiene que tener T_COUNT filas");

// ---- a animacao do Anil (TPAN) ----
//
// O quadro que toca agora, ou nullptr. 20 por segundo, como no Essentials.
static const uint8_t *btAnimQuadro(uint32_t now) {
  if (!bt.anim || !animMove.blob) return nullptr;
  uint32_t q = (now - bt.fxAt) / 50;
  return q < animMove.nq ? animMove.quadro((uint16_t)q) : nullptr;
}

// Um plano de cor sobre a tela inteira (escurecer, o azul do espaco).
static void btAnimPlano(const uint8_t *p) {
  if (!p[3]) return;
  uint16_t *fb = gfx->getFramebuffer();
  if (!fb) return;
  uint16_t c = C565(p[0], p[1], p[2]);
  for (uint32_t i = 0; i < (uint32_t)LCD_WIDTH * LCD_HEIGHT; i++)
    misturaPixel(fb + i, c, p[3], 0);
}

// Uma particula: zoom, giro, espelho, opacidade, mistura e tingimento,
// amostrando a imagem de tras pra frente (cada pixel da tela pergunta de
// que pixel da imagem ele vem). Vizinho mais proximo: e pixel art.
//
// `im` e a imagem da cel: quem chama sabe de que arquivo ela sai (o do
// golpe ou o do clima).
static void btAnimCelDe(const uint8_t *c, const uint8_t *im) {
  if (!im) return;
  int16_t x, y, ang, tom[4];
  uint16_t zx, zy;
  memcpy(&x, c + 2, 2);
  memcpy(&y, c + 4, 2);
  memcpy(&zx, c + 6, 2);
  memcpy(&zy, c + 8, 2);
  memcpy(&ang, c + 10, 2);
  memcpy(tom, c + 18, 8);
  uint8_t op = c[12], fl = c[13];
  const uint8_t *cor = c + 14;
  if (!op || !zx || !zy) return;
  const int w = im[0], h = im[1], ox = (int8_t)im[2], oy = (int8_t)im[3];
  const int np = im[4];
  const uint8_t *alfa = im + 6 + np * 2;
  const uint8_t *idx = alfa + np;
  static uint16_t pal[256];
  bool tingir = cor[3] || tom[0] || tom[1] || tom[2] || tom[3];
  for (int i = 0; i < np; i++) {
    uint16_t p;
    memcpy(&p, im + 6 + i * 2, 2);
    pal[i] = tingir ? tingeCor(p, cor, tom) : p;
  }
  const bool esp = fl & 1;
  const uint8_t modo = (fl >> 1) & 3;
  const float th = ang * (float)PI / 180.0f, cs = cosf(th), sn = sinf(th);
  const float kx = zx / 100.0f, ky = zy / 100.0f;
  // Caixa na tela: os quatro cantos do recorte, levados para la.
  float xMin = 1e9f, xMax = -1e9f, yMin = 1e9f, yMax = -1e9f;
  for (int k = 0; k < 4; k++) {
    float sx = (k & 1) ? ox + w : ox, sy = (k & 2) ? oy + h : oy;
    float u = (esp ? -sx : sx) * kx, v = sy * ky;
    float X = x + u * cs + v * sn, Y = y - u * sn + v * cs;
    if (X < xMin) xMin = X;
    if (X > xMax) xMax = X;
    if (Y < yMin) yMin = Y;
    if (Y > yMax) yMax = Y;
  }
  int x0 = (int)floorf(xMin), x1 = (int)ceilf(xMax);
  int y0 = (int)floorf(yMin), y1 = (int)ceilf(yMax);
  if (x0 < 0) x0 = 0;
  if (y0 < 0) y0 = 0;
  if (x1 > LCD_WIDTH - 1) x1 = LCD_WIDTH - 1;
  if (y1 > LCD_HEIGHT - 1) y1 = LCD_HEIGHT - 1;
  if (x0 > x1 || y0 > y1) return;
  uint16_t *fb = gfx->getFramebuffer();
  if (!fb) return;
  // Inverso: (dX,dY) -> u = dX*cs - dY*sn ; v = dX*sn + dY*cs, e depois
  // desfaz o zoom e o espelho. Por coluna anda-se um passo constante.
  const float pu = cs / kx * (esp ? -1.0f : 1.0f), pv = sn / ky;
  for (int Y = y0; Y <= y1; Y++) {
    float dX = x0 + 0.5f - x, dY = Y + 0.5f - y;
    float su = (dX * cs - dY * sn) / kx * (esp ? -1.0f : 1.0f) - ox;
    float sv = (dX * sn + dY * cs) / ky - oy;
    uint16_t *linha = fb + (uint32_t)Y * LCD_WIDTH;
    for (int X = x0; X <= x1; X++, su += pu, sv += pv) {
      int cx = (int)floorf(su), cy = (int)floorf(sv);
      if ((unsigned)cx >= (unsigned)w || (unsigned)cy >= (unsigned)h) continue;
      uint8_t i = idx[cy * w + cx];
      uint8_t a = alfa[i];
      if (!a) continue;
      if (op < 255) a = (uint8_t)(a * op / 255);
      if (a == 255 && modo == 0) linha[X] = pal[i];
      else misturaPixel(linha + X, pal[i], a, modo);
    }
  }
}

static void btAnimCel(const uint8_t *c) {
  int8_t qual = (int8_t)c[0];
  if (qual < 0) return;
  btAnimCelDe(c, animMove.imagem((uint8_t)qual));
}

// As particulas de uma camada (0 atras dos dois, 1 entre, 2 na frente).
static void btAnimCamada(const uint8_t *q, uint8_t camada) {
  const uint8_t n = q[0];
  const uint8_t *c = q + 9;
  for (uint8_t i = 0; i < n; i++, c += SDANIM_CEL_B)
    if ((int8_t)c[0] >= 0 && c[1] == camada) btAnimCel(c);
}

// ---- O CLIMA, em laco por cima do combate ----
//
// No Anil o clima nao e so o golpe que o chama: enquanto dura, a animacao
// dele (Common:Sandstorm, a areia atravessando a tela) toca SEM PARAR ——
// plugin "Continuous Weather Animations": pbStartWeather a arranca e
// pbUpdateWeather a recomeca quando acaba. Aqui: muda `bt.b.weather`,
// carrega-se o arquivo desse clima e o quadro sai do relogio, em laco.
static uint8_t gClimaVisto = 0;     // o clima para o qual ja se carregou
static uint32_t gClimaAt = 0;       // quando ele comecou
static const uint8_t *btClimaQuadro(uint32_t now) {
  const uint8_t w = bt.b.weather;
  if (w != gClimaVisto) {
    gClimaVisto = w;
    gClimaAt = now;
    if (w) animClima.load(w);
    else animClima.unload();
  }
  if (!w || !animClima.blob || animClima.clima != w) return nullptr;
  return animClima.quadro((uint16_t)(((now - gClimaAt) / 50) % animClima.nq));
}

// A aventura pede o mesmo clima por cima da cena (aventura.ino).
static void btClimaCamada(const uint8_t *q, uint8_t camada);
static void btClimaFigura(const uint8_t *q);
void btClimaAtras(uint32_t now) {
  const uint8_t *q = btClimaQuadro(now);
  if (q) {
    btAnimPlano(q + 1);
    btClimaCamada(q, 0);
  }
}
void btClimaFrente(uint32_t now) {
  const uint8_t *q = btClimaQuadro(now);
  if (q) {
    btClimaCamada(q, 2);
    btClimaFigura(q);
    btAnimPlano(q + 5);
  }
}

static void btClimaCamada(const uint8_t *q, uint8_t camada) {
  const uint8_t n = q[0];
  const uint8_t *c = q + SDCLIMA_QUAD_B;
  for (uint8_t i = 0; i < n; i++, c += SDANIM_CEL_B)
    if ((int8_t)c[0] >= 0 && c[1] == camada)
      btAnimCelDe(c, animClima.imagem(c[0]));
}

// A FIGURA de frente: um ladrilho a meia resolucao (um pixel dele = 2x2
// da tela) que cobre o vidro inteiro e rola. So dentro do circulo: fora
// dele nao ha tela, e sao 46 mil misturas a menos por quadro.
static void btClimaFigura(const uint8_t *q) {
  const uint8_t op = q[9];
  const uint8_t *f = animClima.figura();
  if (!op || !f) return;
  uint16_t w, h, fx, fy;
  memcpy(&w, f, 2);
  memcpy(&h, f + 2, 2);
  memcpy(&fx, q + 10, 2);
  memcpy(&fy, q + 12, 2);
  const int np = f[4];
  const uint8_t *alfa = f + 6 + np * 2;
  const uint8_t *idx = alfa + np;
  // A paleta ja ABERTA para a mistura: o 565 espalhado em 32 bits (R e B
  // num extremo, G no outro, com 5 bits de folga entre eles) e o alfa em
  // 5 bits (0..32). Assim cada pixel e: d*(32-a) + s*a, >> 5, e mascara.
  // Sem isto eram tres divisoes por 255 e sete deslocamentos por pixel,
  // vezes 170 mil pixeis por quadro: o combate ficava "meio lento" com
  // sol ou areia.
  static uint32_t cor32[256];
  static uint8_t a5[256];
  for (int i = 0; i < 256; i++) a5[i] = 0;
  for (int i = 0; i < np; i++) {
    uint16_t c;
    memcpy(&c, f + 6 + i * 2, 2);
    cor32[i] = ((uint32_t)c | ((uint32_t)c << 16)) & 0x07E0F81Fu;
    a5[i] = (uint8_t)(((uint32_t)alfa[i] * op * 32 + 255 * 127) / (255 * 255));
  }
  uint16_t *fb = gfx->getFramebuffer();
  if (!fb || !w || !h) return;
  const int R = LCD_WIDTH / 2;
  for (int Y = 0; Y < LCD_HEIGHT; Y++) {
    int dy = Y - R;
    int meio = (int)sqrtf((float)(R * R - dy * dy));
    int x0 = R - meio, x1 = R + meio;
    if (x0 < 0) x0 = 0;
    if (x1 > LCD_WIDTH - 1) x1 = LCD_WIDTH - 1;
    const uint8_t *fila = idx + (uint32_t)(((uint32_t)(Y >> 1) + fy) % h) * w;
    uint32_t tx = ((uint32_t)(x0 >> 1) + fx) % w;
    uint16_t *linha = fb + (uint32_t)Y * LCD_WIDTH;
    for (int X = x0; X <= x1; X++) {
      const uint8_t i = fila[tx];
      const uint32_t a = a5[i];
      if (a) {
        const uint32_t d = linha[X];
        const uint32_t d32 = (d | (d << 16)) & 0x07E0F81Fu;
        const uint32_t m = ((d32 * (32 - a) + cor32[i] * a) >> 5) & 0x07E0F81Fu;
        linha[X] = (uint16_t)(m | (m >> 16));
      }
      if ((X & 1) && ++tx >= w) tx = 0;
    }
  }
}

// A cel do BICHO de um lado neste quadro (-1 o jogador, -2 o rival).
static const uint8_t *btAnimBicho(const uint8_t *q, uint8_t lado) {
  const int8_t quer = (lado == BT_PLAYER) ? -1 : -2;
  const uint8_t *c = q + 9;
  for (uint8_t i = 0; i < q[0]; i++, c += SDANIM_CEL_B)
    if ((int8_t)c[0] == quer) return c;
  return nullptr;
}


static void btMoveFx(uint32_t now) {
  if (bt.anim) return;            // a do Anil ja esta desenhada
  // Tres escalones, de lo concreto a lo general: la animacion DEL GOLPE si
  // el Anil tiene su hoja (140 de 441), si no la de su categoria cuando no
  // pega, y si no la de su tipo. Asi Hydro Pump se ve distinto de Surf sin
  // que ningun golpe se quede sin nada.
  // La animacion DEL GOLPE vive en la tarjeta (SdFx): son 1 MB y en flash
  // no caben. Si esta cargada manda ella; si no hay SD, o ese golpe no
  // tiene la suya, se cae a la de su categoria (cuando no pega) y luego a
  // la de su tipo, que siguen en flash. Asi nunca se queda sin efecto.
  uint8_t nF = 0, escala = 1;
  const Art *propio = nullptr;    // se resuelve abajo, fotograma a fotograma
  bool deTarjeta = (bt.fxMove && fxMove.move == bt.fxMove && fxMove.n);
  const FxAnim *sel = nullptr;
  if (deTarjeta) {
    nF = fxMove.n;
    escala = fxMove.s;
  } else if (bt.fxType != T_NONE && bt.fxType < T_COUNT) {
    // El TIPO antes que la eficacia. Estaba al reves y por eso Latigo
    // Cepa -- que no trae animacion propia en el Anil -- salia como un
    // puñetazo: se caia al estallido generico de "ha sido eficaz" en vez
    // de al efecto de planta.
    //
    // Lo de la eficacia tenia sentido cuando era la unica pista de si
    // acertabas el tipo. Ya no: desde que cada golpe suena distinto
    // segun el dano (SFX_HIT_WEAK / HIT / HIT_SUPER), esa informacion
    // llega por el oido y la pantalla puede dedicarse al sabor.
    sel = &FX_OF_TYPE[bt.fxType];
  } else if (bt.fxSup < FX_SUP_COUNT) {
    sel = &FX_SUP[bt.fxSup];
  }
  if (!deTarjeta) {
    if (!sel || !sel->n || !sel->f) return;
    nF = sel->n;
    escala = sel->s;
  }
  uint32_t paso = FX_WINDOW_MS / nF;
  uint32_t f = (now - bt.fxAt) / (paso ? paso : 1);
  if (f >= nF) return;
  if (deTarjeta) {
    if (!fxMove.has[f]) return;   // fotograma vacio de la hoja: cuenta, no pinta
    propio = &fxMove.frames[f];
  } else {
    propio = sel->f[f];
  }
  const Art *a = propio;
  if (!a) return;                 // fotograma vacio de la hoja: cuenta, no pinta
  bool foe = (bt.fxSide != btAbajo());
  int cx = foe ? BT_FOE_CX : 145;
  int cy = (foe ? BT_GROUND_FOE - 50 : BT_GROUND_MINE - 70);
  uint8_t s = escala ? escala : 1;
  drawArt(*a, cx, cy + (a->h * s) / 2, s);
}

// El estado del CAMPO: el cielo y el suelo, con los turnos que les quedan.
// No habia ninguno â€” ni para el clima, que lleva tiempo puesto â€”, y sin el
// no hay forma de saber si el sol sigue arriba o si conviene volver a tirar
// Sunny Day. Va debajo de la chapa del rival, que es el hueco que queda
// entre las dos barras.
static void btFieldChip(int x, int y, const char *s, uint8_t turnos, uint16_t col) {
  int w = (int)strlen(s) * LATIN_ADV + 26;
  gfx->fillRoundRect(x, y, w, 17, 5, darken565(col));
  gfx->fillRoundRect(x, y, w, 14, 5, col);
  gfx->setTextSize(1);
  gfx->setTextColor(UI_WHITE);
  gfx->setCursor(x + 6, y + 4);
  gfx->print(s);
  char n[4];
  snprintf(n, sizeof(n), "%u", (unsigned)turnos);
  gfx->setCursor(x + w - 12, y + 4);
  gfx->print(n);
}

static void btFieldChips() {
  static const char *const CIELO[] = { "", "SUN", "RAIN", "SAND", "HAIL" };
  static const uint16_t CCOL[] = { 0, C565(0xe8, 0xa8, 0x00), C565(0x40, 0x88, 0xd0),
                                   C565(0xb0, 0x90, 0x50), C565(0x70, 0xc8, 0xe0) };
  static const char *const SUELO[] = { "", "ELEC", "GRASS", "MIST", "PSY" };
  static const uint16_t SCOL[] = { 0, C565(0xe8, 0xd0, 0x30), C565(0x50, 0xb0, 0x50),
                                   C565(0xd0, 0x90, 0xd8), C565(0xe0, 0x60, 0xa0) };
  static_assert(sizeof(CIELO) / sizeof(CIELO[0]) == WE_HAIL + 1, "falta un cielo");
  static_assert(sizeof(SUELO) / sizeof(SUELO[0]) == TR_PSY + 1, "falta un suelo");

  int x = 66;
  if (bt.b.weather) {
    btFieldChip(x, 106, CIELO[bt.b.weather], bt.b.weatherTurns, CCOL[bt.b.weather]);
    x += (int)strlen(CIELO[bt.b.weather]) * LATIN_ADV + 32;
  }
  if (bt.b.terrain)
    btFieldChip(x, 106, SUELO[bt.b.terrain], bt.b.terrainTurns, SCOL[bt.b.terrain]);
}

// ---------------------------------------------------------- pantalla de relevo
//
// El boton de equipo es hermano del de la bolsa y vive justo debajo. Cuando
// el relevo es OBLIGATORIO (cayo el tuyo y queda banquillo) la lista no se
// puede cerrar tocando fuera: ahi no hay nada que elegir salvo a quien sacar.
// Estaba en (400, 232) y se comia la chapa de PS del jugador, que vive
// en la DERECHA de y=228 a y=262 (x 272..454). Se solapaban casi
// enteros. Ahora va a la izquierda, debajo del de huir (50, 252), con
// 10 px de aire entre los dos circulos; por ahi no pasa ni la chapa ni
// el bicho, que se dibuja centrado en x=145.
#define BPTY_BTN_X 50
#define BPTY_BTN_Y 316
#define BPTY_BTN_R 27
#define BPTY_Y 128
#define BPTY_H 44
#define BPTY_X 66
#define BPTY_W 334
// El panel de la lista (el drawWindow de btRenderParty): fuera de el,
// y con foso, es "fuera".
#define BPTY_PAN_X0 56
#define BPTY_PAN_X1 410
#define BPTY_PAN_Y0 96
#define BPTY_PAN_Y1 396

void btDrawPartyButton() {
  if (btReserveCount() == 0) return;     // sin banquillo no hay boton
  gfx->fillCircle(BPTY_BTN_X, BPTY_BTN_Y, BPTY_BTN_R, UI_WHITE);
  gfx->drawCircle(BPTY_BTN_X, BPTY_BTN_Y, BPTY_BTN_R, INK_K);
  // La pokebola del menu del Anil. Eran dos circulos y dos rectangulos
  // redondeados haciendo de "siluetas", y parecian dos pastillas.
  drawArt(ART_SWAP, BPTY_BTN_X, BPTY_BTN_Y + ART_SWAP.h / 2, 1);
}

// Una fila por hueco del equipo, con miniatura, nivel y barra de PS. El que
// esta en pista y los caidos salen apagados y no responden.
static void btPartyRow(uint8_t slot, int y) {
  int16_t esp;
  uint16_t nv, hp, tope;
  bool puede, caido, fuera, vazio;
  if (bt.pvp) {
    // Entre placas a lista e a do COMBATE: os que eu levei, no 50, com o
    // que lhes resta. O que esta na pista vive no motor, nao na ficha.
    const PvpLado &l = pvpL[pvpEu];
    vazio = slot >= l.n;
    esp = vazio ? 0 : l.reg[slot].species;
    const Combatant &c = (!vazio && slot == l.ativo) ? bt.b.side[pvpEu] : l.mon[vazio ? 0 : slot];
    nv = PVP_NIVEL;
    hp = c.hp;
    tope = c.hpMax;
    // com um objeto na mao acende quem ele ajudaria (o Revive: o caido)
    puede = !vazio && (bt.itemPick ? btItemSirveEn(bt.itemPick, slot) : btCanSend(slot));
    caido = (hp == 0);
    fuera = !vazio && slot == l.ativo && !caido;
  } else {
    bool main = btIsMain(slot);
    esp = main ? pet.speciesId : pet.team[slot].species;
    vazio = main ? pet.isEgg() : !pet.team[slot].used();
    nv = vazio ? 0 : main ? pet.level() : Pet::levelOf(pet.team[slot]);
    hp = vazio ? 0 : main ? pet.hpLeft : pet.team[slot].hpLeft;
    tope = vazio ? 0 : hpAtLevel(esp, nv, main ? pet.trHp : pet.team[slot].trHp);
    // Con un objeto en la mano, lo que decide si la fila responde no es
    // "puede salir a pelear" sino "a este le haria algo": un Revivir va
    // justamente al CAIDO, que es el que btCanSend apaga.
    puede = !vazio && (bt.itemPick ? btItemSirveEn(bt.itemPick, slot)
                                   : btCanSend(slot));
    // "En pista" solo mientras aguante: el que acaba de caer sigue siendo
    // `mySlot` hasta que se elige el relevo, y marcarlo como si estuviera
    // peleando es justo lo contrario de lo que pasa.
    caido = (hp == 0);
    fuera = (slot == bt.mySlot) && !caido;
  }
  if (vazio) {
    gfx->drawRoundRect(BPTY_X, y, BPTY_W, BPTY_H - 6, 8, UI_TRACK);
    gfx->setTextColor(UI_TRACK);
    setSize(1);
    setCur(BPTY_X + 14, y + 16);
    printT(T(S_VAZIO));
    return;
  }
  // Con un objeto en la mano, estar EN PISTA no vuelve a encender la
  // fila: una Pocion sobre uno con la vida llena no hace nada, y el
  // realce verde del que esta peleando decia que si. Se queda el fondo
  // (es informacion: ese es el que esta fuera) y se apaga la letra.
  bool resalta = fuera && (!bt.itemPick || puede);

  gfx->fillRoundRect(BPTY_X, y, BPTY_W, BPTY_H - 6, 8,
                     fuera ? C565(0xd8, 0xe4, 0xc8) : UI_WHITE);
  gfx->drawRoundRect(BPTY_X, y, BPTY_W, BPTY_H - 6, 8, puede ? UI_INK : UI_TRACK);
  const uint8_t *th = thumbs.get(esp);
  if (th) drawThumbFit(th, BPTY_X + 24, y + (BPTY_H - 6) / 2, 30, !puede && !resalta);
  gfx->setTextColor(puede || resalta ? UI_INK : UI_TRACK);
  setSize(2);
  setCur(BPTY_X + 46, y + 5);
  printT(dexName(esp));
  char lv[10];
  snprintf(lv, sizeof(lv), "Lv%u", (unsigned)nv);
  setSize(1);
  setCur(BPTY_X + BPTY_W - 12 - (int)strlen(lv) * LATIN_ADV, y + 7);
  printT(lv);
  // barra de PS, que es lo que de verdad decide a quien sacas
  int bw = BPTY_W - 60;
  uint32_t pct = tope ? (uint32_t)hp * 100 / tope : 0;
  if (pct > 100) pct = 100;
  gfx->fillRect(BPTY_X + 46, y + 25, bw, 6, UI_TRACK);
  int fw = (int)(bw * pct / 100);
  if (fw > 0)
    gfx->fillRect(BPTY_X + 46, y + 25, fw, 6,
                  pct > 50 ? UI_BAR_OK : pct > 20 ? UI_BAR_WARN : UI_BAR_BAD);
  if (fuera || caido) {
    gfx->setTextColor(caido ? UI_BAR_BAD : C565(0x2e, 0x7d, 0x32));
    setSize(1);
    setCur(BPTY_X + 46, y + 33);
    printT(caido ? T(S_DESMAIOU) : T(S_EM_CAMPO));
  }
}

// El RESUMEN de la victoria. Lo que avanza de verdad en el juego —— la
// experiencia, el nivel, los stats —— solo salia por el puerto serie, y la
// pantalla volvia a casa en segundo y medio.
//
// Habla del que PELEO, no del principal: con un suplente en pista, los
// stats del titular no son los que acaban de cambiar.
// A tela da INSIGNIA: a insignia grande, o nome, de quem, a MT e o premio.
void btRenderBadge() {
  drawWindow(50, 84, 366, 300, 16);
  const uint8_t reg = pet.region, i = bt.insigniaIdx;
  const bool cristal = (reg == 6);          // Alola: Cristal Z, nao insignia
  // leva 71: a letra de DS e o texto no idioma (era "NEW BADGE!" fixo)
  const char *t = T(cristal ? S_BADGE_ZCRYSTAL : S_BADGE_NEW);
  printTituloC(t, 102, tituloTalla(t, 3, 300), C565(0xc0, 0x50, 0x10),
               C565(0xf0, 0xd8, 0x90));
  // um brilho dourado atras da insignia, a respirar
  int rb = 46 + (int)(3 * sinf(millis() * 0.004f));
  gfx->fillCircle(233, 178, rb, C565(0xfc, 0xec, 0xb8));
  gfx->fillCircle(233, 178, rb - 10, C565(0xfe, 0xf6, 0xd8));
  drawBadge(reg, i, 233, 178, 2);
  const char *nome = badgeName(reg, i);
  printTituloC(nome, 230, tituloTalla(nome, 2, 300), UI_INK, C565(0xc8, 0xc0, 0xa8));
  char l[48];
  snprintf(l, sizeof(l), T(S_BADGE_FROM), gymAt(reg, i).leader);
  printTituloC(l, 256, 1, C565(0x60, 0x60, 0x78), C565(0xd8, 0xd8, 0xe0));
  int y = 282;
  if (pet.lastTM >= 0) {
    snprintf(l, sizeof(l), T(S_BADGE_TM), MOVE_TBL[tmMove((uint16_t)pet.lastTM)].name);
    printTituloC(l, y, tituloTalla(l, 2, 300), C565(0x30, 0x58, 0xb0),
                 C565(0xc8, 0xd0, 0xe8));
    y += 30;
  }
  if (pet.lastBallId) {
    snprintf(l, sizeof(l), "+%u %s", (unsigned)pet.lastBallCount,
             itemAt(pet.lastBallId).name);
    printTituloC(l, y, tituloTalla(l, 2, 300), C565(0x28, 0x80, 0x40),
                 C565(0xc8, 0xe0, 0xc0));
  }
}

void btRenderWin() {
  drawWindow(50, 84, 366, 300, 16);
  gfx->setTextColor(UI_INK);
  setSize(3);
  const char *t = T(S_VITORIA);
  setCur(centerX(t, 3), 100);
  printT(t);

  int16_t dex = bt.winDex ? bt.winDex : bt.b.side[BT_PLAYER].dex;
  const uint8_t *th = thumbs.get(dex);
  if (th) drawThumbFit(th, 92, 150, 34, false);

  // A CARA COMEMORANDO, na metade direita, que esta vazia.
  // O vencedor pode ser shiny, e tem retrato proprio.
  const bool ganhadorShiny =
      btIsMain(bt.mySlot) ? pet.shiny
                          : (pet.team[bt.mySlot].flags & SM_SHINY) != 0;
  // Venceu MEGAEVOLUIDO: a cara da mega, se ela foi feita, na cor dele
  // (normal ou shiny). Sem a versao certa cai na cara da especie —— cor
  // errada num shiny e pior que nao mostrar a mega.
  const Combatant &eu = bt.b.side[BT_PLAYER];
  const uint8_t festaMega =
      (eu.isMega() && MEGA_TBL[eu.megaIdx].base == dex) ? MEGA_TBL[eu.megaIdx].form : 0;
  if (dex > 0 && festaMega &&
      !(porMon.dex == (uint16_t)dex && porMon.mega == festaMega &&
        porMon.shiny == ganhadorShiny))
    porMon.loadMega((uint16_t)dex, festaMega, ganhadorShiny);
  const bool temMega = festaMega && porMon.dex == (uint16_t)dex &&
                       porMon.mega == festaMega && porMon.shiny == ganhadorShiny &&
                       porMon.blob;
  if (dex > 0 && !temMega && (porMon.dex != (uint16_t)dex ||
                              porMon.shiny != ganhadorShiny || porMon.mega))
    porMon.load((uint16_t)dex, ganhadorShiny);
  const Art *festa = (porMon.dex == (uint16_t)dex &&
                      (temMega || (!porMon.mega && porMon.shiny == ganhadorShiny)))
                         ? porMon.cara(POR_FESTA)
                         : nullptr;
  if (festa) {
    // NO VAZIO DO MEIO, nao no canto de cima a direita.
    //
    // Estava em (338, 150) e ficava debaixo do "Lv.N" —— que e alinhado
    // a direita terminando em x=366, na mesma linha do nome —— e ainda
    // encostava na barra de EXP (y=186). Renderizei a tela para olhar,
    // em vez de escolher outro canto no escuro.
    //
    // Daqui para baixo a janela esta vazia ate o "tap to continue", que
    // e onde a cara cabe inteira e com folga. drawArt ancora pelo pe.
    // NO CANTO DE CIMA A DIREITA, que o "Lv.N" acabou de liberar ao
    // mudar para dentro da barra.
    //
    // NAO vai no meio da janela: ali parece vazio numa vitoria sem
    // premio, mas e onde entram o "Lv.A -> Lv.B" e os seis stats quando
    // se sobe de nivel. Eu tinha posto a cara la depois de olhar uma
    // renderizacao sem subida e sem item —— teria tapado os dois.
    drawArt(*festa, 348, 178, 2);
    // E a nota, no ombro dela: `Joyous` so cobre 72% das especies,
    // entao em uma de cada quatro a cara aqui e a neutra e quem diz
    // que isto e uma comemoracao e ela.
    // No ombro ESQUERDO dela e nao no canto de cima a direita: ali
    // a nota encostava na moldura arredondada da janela e saia
    // cortada. E EM CIMA, como no balao da casa (la o simbolo fica no
    // alto do balao): a cara vai de y=98 a 178; o pe da nota em 124
    // a poe em 92..124, dentro da janela (topo em 84).
    drawArt(ART_NOTA, 326, 124, 1);
  }
  setSize(2);
  setCur(124, 142);
  printT(dexName(dex));

  char l[40];
  // la experiencia, que es el premio de todos los combates
  snprintf(l, sizeof(l), "+%lu EXP", (unsigned long)bt.expWon);
  gfx->setTextColor(C565(0x3c, 0xa8, 0xe0));
  setSize(2);
  setCur(124, 166);
  printT(l);

  // El nivel y lo que falta para el siguiente. Sin esto, un combate sin
  // subida ni objeto dejaba la ventana casi vacia —— "+33 EXP" y nada mas ——
  // y no decia lo unico que uno quiere saber: cuanto falta.
  {
    uint16_t lv = btIsMain(bt.mySlot) ? pet.level()
                                      : Pet::levelOf(pet.team[bt.mySlot]);
    // O "Lv.N" vai DENTRO da barra, nao na linha do nome.
    //
    // Na linha do nome ele ficava alinhado a direita terminando em
    // x=366 —— exatamente onde cabia a cara comemorando, e uma tapava
    // o outro. Dentro da barra ele nao disputa espaco com nada, e a
    // barra engrossa de 10 para 22 para ele caber com folga.
    snprintf(l, sizeof(l), "Lv.%u", (unsigned)lv);
    int bx = 86, bw = 300, by = 182, bh = 22;
    gfx->fillRoundRect(bx, by, bw, bh, 7, UI_TRACK);
    int fw = (int)((bw - 2) * btExpProgress());
    if (fw > 0)
      gfx->fillRoundRect(bx + 1, by + 1, fw, bh - 2, 6, C565(0x3c, 0xa8, 0xe0));
    gfx->setTextColor(UI_INK);
    setSize(1);
    setCur(bx + 10, by + 8);
    printT(l);
  }

  // Y lo que se llevan los del banquillo. Es poco (una cuarta parte) pero
  // existe, y sin decirlo parecia que no ganaban nada por estar ahi.
  if (btReserveCount() > 0 || pet.teamCount() > 1) {
    snprintf(l, sizeof(l), T(S_OUTROS_EXP_FMT),
             (unsigned long)(bt.expWon / 4 ? bt.expWon / 4 : 1));
    gfx->setTextColor(UI_TRACK);
    setSize(1);
    setCur(124, 212);
    printT(l);
  }

  // 232 e nao 206: a barra ficou mais grossa e o "all others" desceu
  // com ela. Daqui para baixo vem o que SO as vezes aparece —— os
  // itens e, se subiu de nivel, o aviso e os seis stats.
  int y = 232;
  // lo que se haya recogido por el camino
  for (uint8_t i = 0; i < 2; i++) {
    if (!bt.loot[i]) continue;
    gfx->setTextColor(UI_INK);
    setSize(1);
    if (ART_ITEM[bt.loot[i]])
      drawArt(*ART_ITEM[bt.loot[i]], 86, y + 6 + ART_ITEM[bt.loot[i]]->h / 2, 1);
    setCur(108, y + 2);
    printT(T(S_GANHOU));
    setCur(108 + textW(T(S_GANHOU), 1), y + 2);
    printT(itemAt(bt.loot[i]).name);
    y += 22;
  }

  if (bt.lvlWon) {
    uint16_t ahora = (uint16_t)(bt.lvlAntes + bt.lvlWon);
    snprintf(l, sizeof(l), "Lv.%u -> Lv.%u", (unsigned)bt.lvlAntes,
             (unsigned)ahora);
    gfx->setTextColor(UI_BAR_OK);
    setSize(2);
    setCur(centerX(l, 2), y + 4);
    printT(l);
    y += 30;

    // Y COMO QUEDO. Es lo que uno quiere mirar justo despues de subir, y
    // hasta ahora habia que irse a la ficha a buscarlo.
    const Combatant &c = bt.b.side[BT_PLAYER];
    static const char *ET[6] = { "HP", "ATK", "DEF", "SPA", "SPD", "SPE" };
    uint16_t vs[6] = { c.hpMax, c.atkStat(), c.defStat(),
                       c.spAtkStat(), c.spDefStat(), c.speStat() };
    gfx->setTextColor(UI_INK);
    setSize(1);
    for (uint8_t i = 0; i < 6; i++) {
      int cx = 100 + (i % 3) * 112;
      int cy = y + (i / 3) * 20;
      snprintf(l, sizeof(l), "%s %u", ET[i], (unsigned)vs[i]);
      setCur(cx, cy);
      printT(l);
    }
    y += 44;
  }

  gfx->setTextColor(UI_TRACK);
  setSize(1);
  const char *pie = T(S_INTRO_TAP);
  setCur(centerX(pie, 1), 360);
  printT(pie);
}

void btRenderParty() {
  drawWindow(BPTY_PAN_X0, BPTY_PAN_Y0, BPTY_PAN_X1 - BPTY_PAN_X0,
             BPTY_PAN_Y1 - BPTY_PAN_Y0, 16);
  gfx->setTextColor(UI_INK);
  setSize(2);
  // El titulo DICE lo que se esta haciendo. Con un objeto en la mano la
  // misma lista significa otra cosa, y sin decirlo el jugador cree que
  // va a relevar —— que es justo el toque que no quiere dar.
  char tit[48];
  if (bt.itemPick)
    snprintf(tit, sizeof(tit), T(S_EM_QUEM_FMT), ITEM_TBL[bt.itemPick].name);
  else
    snprintf(tit, sizeof(tit), "%s", bt.mustSwitch ? T(S_QUEM_PROXIMO) : T(S_CARD_TEAM));
  uint8_t tt = 2;
  while (tt > 1 && textW(tit, tt) > 330) tt--;
  setSize(tt);
  setCur(centerX(tit, tt), 106 + (2 - tt) * 4);
  printT(tit);
  // LOS SEIS. Eran cinco con un `partyScroll` que nadie movia nunca
  // (solo se pone a 0, en tres sitios), asi que el sexto del equipo no
  // se podia ver NI tocar: estaba fuera del juego. Y caben: seis filas
  // de 44 desde y=128 acaban en 392, donde la cuerda del circulo da de
  // x=63 a x=403 y la lista ocupa de 66 a 400.
  for (uint8_t i = 0; i < TEAM_SLOTS; i++)
    btPartyRow(i, BPTY_Y + i * BPTY_H);
  // FUERA de la ventana. Estaba en y=372, que cae DENTRO de la sexta fila
  // (128 + 5*44 = 348, y la fila mide 38): el aviso se pintaba encima de
  // la barra de PS del sexto del equipo. La ventana acaba en y=396, asi
  // que el texto se va debajo, y con contorno para que se lea sobre el
  // escenario en vez de sobre el papel de la ficha.
  const char *pie = bt.itemPick   ? T(S_FORA_BOLSA)
                  : bt.mustSwitch ? T(S_MANDE_ALGUEM)
                                  : T(S_FORA_VOLTAR);
  printOutlined(centerX(pie, 1), 404, pie, 1, UI_WHITE);
}

// true si se comio el toque
bool btPartyTap(int16_t x, int16_t y) {
  if (bt.phase != BP_PARTY) return false;
  // Con un objeto en la mano la lista NO releva: aplica. Va antes de
  // todo lo demas porque las reglas del relevo (agarrado, caido, en
  // pista) no pintan nada aqui —— a un caido se le da un Revivir
  // precisamente porque esta caido.
  if (bt.itemPick) return btItemPickTap(x, y);
  // Los mismos seis que se dibujan, o el sexto se veria y no responderia.
  for (uint8_t i = 0; i < TEAM_SLOTS; i++) {
    int ry = BPTY_Y + i * BPTY_H;
    if (x < BPTY_X || x > BPTY_X + BPTY_W || y < ry || y > ry + BPTY_H - 6) continue;
    if (!btCanSend(i)) { sfxPlay(SFX_DENY); return true; }
    // Agarrado o atrapado no se cambia por gusto. El relevo FORZOSO
    // (mustSwitch: el tuyo ya cayo) sigue saliendo, que si no el combate
    // se quedaria clavado.
    if (!bt.mustSwitch && !bt.b.canSwitchOut(BT_PLAYER)) {
      sfxPlay(SFX_DENY);
      snprintf(bt.msg, sizeof(bt.msg), "%s can't get away!",
               dexName(bt.b.side[BT_PLAYER].pareceDex()));
      bt.extraStep = 9;
      bt.phase = BP_MSG;
      bt.until = millis() + 900;
      return true;
    }
    bool gratis = bt.mustSwitch;   // el tuyo ya cayo: el relevo no se paga
    // Si el relevo es por gusto, el que entra lo lleva marcado: es la
    // unica situacion en la que Persecucion pega el doble.
    btSwitchTo(i, !gratis);
    sfxPlay(SFX_TAP);
    bt.mustSwitch = false;
    // El "Go, X!" que dejo btSwitchTo se lee en BP_MSG; de ahi, o se vuelve
    // a elegir (relevo obligatorio, ya pagado con la caida) o pega el rival
    // (relevo voluntario: cuesta el turno, que es lo que impide encadenar
    // cambios gratis).
    bt.extraStep = 9;          // sin avisos de drenaje ni de estado
    bt.phase = BP_MSG;
    bt.until = millis() + 900;
    if (gratis) {
      bt.secondPending = false;
      bt.endStep = 2;          // el desgaste de fin de turno ya se cobro
    } else {
      bt.foeSlot = bt.b.pickFoeMove();
      bt.b.inicioDeRondaRival(bt.foeSlot);   // el jugador gasto la ronda en otra cosa
      bt.acting = BT_PLAYER;   // "yo" ya gaste el turno cambiando
      bt.secondPending = true; // asi BP_MSG lanza al rival con foeSlot
      bt.endStep = 0;
    }
    return true;
  }
  if (bt.mustSwitch) return true;    // fuera de la lista no se sale
  // Cerrar es tocar FUERA DEL PANEL, y lejos. Antes era "fuera de las
  // filas": el hueco de 6 px entre dos filas, o un pelo al lado de la
  // lista, devolvia al combate.
  if (lejosDe(x, y, BPTY_PAN_X0, BPTY_PAN_Y0, BPTY_PAN_X1, BPTY_PAN_Y1))
    bt.phase = BP_CHOOSE;
  return true;
}

// El sprite del rival sigue a asDex, NO a la especie con la que empezo el
// combate.
//
// Lo delato un Ditto con la placa en la mano. Su Imposter le copia al
// de enfrente nada mas entrar: el nombre y los golpes ya eran los del
// Bulbasaur del jugador, pero el dibujo seguia siendo el del Ditto,
// porque la clave de la cache se armaba con el parametro `foeDex` de
// battleStart(), que no cambia nunca. Y salia ADEMAS gigante, que era
// el mismo fallo visto de otra forma: btDrawSide saca el tamano de
// DEX_TBL[asDex].screenSz (la altura que se QUIERE, ya la del Bulbasaur)
// dividida por la altura del sprite CARGADO (todavia la del Ditto, que
// es un pegote de 0,3 m). Mezclar las dos especies en una division da
// una escala absurda.
//
// Por eso se comprueba en el RENDER y no en un evento: asi vale para la
// Imposter al entrar, para el golpe Transformacion y para lo que venga,
// sin tener que acordarse de llamar a nadie. La mega tiene su propio
// camino (loadMega) y aqui se deja en paz.

// Y lo mismo del lado de ACA. Hasta ahora solo se sincronizaba el
// rival, asi que un Ditto TUYO se transformaba -- copiaba tipos, stats
// y golpes -- y seguia dibujado como Ditto. El dueno lo pregunto
// directamente ("nao sei se copia o sprite do pokemon adversario") y
// tenia razon en dudar: de su lado no lo copiaba.
//
// Solo actua si esta TRANSFORMADO. Sin transformar, de `pmd` se encarga
// ensureMon() con la especie del bicho, y pisarselo seria pelearse con
// el. Al tocarlo se invalida `monFor` para que ensureMon lo devuelva a
// su sitio en cuanto se acabe el combate.
// Leva 70: lo que se VE no es siempre la especie. La clave de la cache:
// -1 = el dibujo de siempre; 20000+ = la cara de la Ilusion; 30000+ = una
// forma de combate (Castform al sol, Aegislash Filo...). Por encima de
// la dex entera y de las filas de forma (-2 - fila), para no chocar.
static int16_t btClaveVisual(const Combatant &c) {
  if (c.transformed) return -1;
  if (c.illusion && c.ilusDex > 0)
    return (int16_t)(20000 + c.ilusDex * 2 + (c.ilusShiny ? 1 : 0));
  if (c.bform >= 0) return (int16_t)(30000 + c.bform * 2 + (c.shiny ? 1 : 0));
  return -1;
}

static bool btCargaVisual(PmdMon &m, const Combatant &c) {
  if (c.illusion && c.ilusDex > 0) {
    int16_t fr = formRow(c.ilusDex, c.ilusForm);
    return fr >= 0 ? m.loadForm(fr) : m.load(c.ilusDex, c.ilusShiny, false);
  }
  if (c.bform >= 0)
    return m.loadBatalha(BFORM_TBL[c.bform].base, BFORM_TBL[c.bform].form, c.shiny);
  return false;
}

// El dibujo de siempre de quien esta en pista (su forma regional si la
// tiene). Para cuando la forma de combate o la Ilusion se van.
static bool btCargaPropio(PmdMon &m, const Combatant &c) {
  int16_t fr = formRow(c.dex, c.form);
  return fr >= 0 ? m.loadForm(fr) : m.load(c.dex, c.shiny, c.sexo == 1);
}

static void btSyncMySprite() {
  const Combatant &c = bt.b.side[btAbajo()];
  if (!c.valid() || c.isMega()) return;
  int16_t v = btClaveVisual(c);
  if (!c.transformed && v < 0) {
    // Ni forma ni ilusion. Si la ultima vez puse yo otra cosa, se vuelve
    // a su dibujo (la cascara rota no se arregla, pero el Daruma si).
    if (myPmdFor >= 20000 || myPmdTried >= 20000) {
      pmd.unload();
      btCargaPropio(pmd, c);
      myPmdFor = myPmdTried = -1;
    }
    return;
  }
  int16_t quiero = c.transformed ? c.asDex : v;
  if (myPmdFor == quiero || myPmdTried == quiero) return;
  myPmdTried = quiero;
  pmd.unload();
  bool ok = c.transformed ? pmd.load(c.asDex, c.shiny, c.sexo == 1)
                          : btCargaVisual(pmd, c);
  myPmdFor = ok ? quiero : -1;
  // SIN `monFor = -2` aqui (leva 70). ensureMon() corre en CADA vuelta del
  // bucle, tambien en combate: con -2 recargaba el bicho de casa en la
  // vuelta siguiente y la transformacion (o la forma) duraba un fotograma.
  // Al cerrar el combate ya se invalida monFor (ver battleClose).
  Serial.printf("mi sprite -> dex=%d (%s) cargado=%d\n",
                (int)c.asDex, dexName(c.pareceDex()), (int)ok);
}

static void btSyncFoeSprite() {
  const Combatant &c = bt.b.side[btArriba()];
  // Leva 72: o que esta do lado de la fica VISTO na Pokedex. Aqui e nao no
  // battleStart porque e o unico ponto por onde passam todos (selvagem,
  // treinador, o 2o e 3o do lider, a outra placa). Com Ilusao, o que se ve
  // e o disfarce. markSeen so faz algo na primeira vez.
  if (c.valid()) pet.markSeen(c.illusion && c.ilusDex > 0 ? c.ilusDex : c.dex);
  if (!c.valid() || c.isMega()) return;
  int16_t quiero = c.shiny ? (int16_t)(c.asDex + DEX_COUNT + 1) : c.asDex;
  int16_t fila = formRow(c.asDex, c.form);
  if (fila >= 0) quiero = (int16_t)(-2 - fila);
  int16_t v = btClaveVisual(c);
  if (v >= 0) quiero = v;
  if (foePmdFor == quiero) return;
  // Se apunta lo INTENTADO y no solo lo logrado: si el fichero no esta en
  // la tarjeta, sin esto se reintentaria la lectura en cada fotograma.
  if (foePmdTried == quiero) return;
  foePmdTried = quiero;
  foePmd.unload();
  bool ok = v >= 0     ? btCargaVisual(foePmd, c)
          : fila >= 0 ? foePmd.loadForm(fila)
                      : foePmd.load(c.asDex, c.shiny, c.sexo == 1);
  foePmdFor = ok ? quiero : -1;
  Serial.printf("sprite rival -> dex=%d (%s) cargado=%d\n",
                (int)c.asDex, dexName(c.pareceDex()), (int)ok);
}

// Quanto demorou o ultimo quadro do combate e, dele, a figura do clima.
// Le-se pela serial com CLIMA: foi assim que se mediu o "meio lento".
uint32_t gQuadroMs = 0, gClimaMs = 0;

void renderBattle() {
  const uint32_t t0 = millis();
  drawBattleScene(millis());   // cielo + las dos plataformas

  // el que pega se lanza hacia delante; el que recibe se sacude
  float dMine = 0, dFoe = 0;
  // A animacao do Anil, se houver: ela mesma move os bichos, entao a
  // estocada de sempre fica de fora.
  const uint8_t *aq = btAnimQuadro(millis());
  if (bt.phase == BP_ATTACK && !bt.anim) {
    float u = 1.0f - (float)((int32_t)bt.until - (int32_t)millis()) / 320.0f;
    float lunge = sinf(u * (float)PI) * 16.0f;
    if (bt.acting == btAbajo()) dMine = lunge; else dFoe = -lunge;
  } else if (bt.phase == BP_HIT && bt.last.damage) {
    float sac = ((millis() / 60) % 2) ? 5.0f : -5.0f;
    if (bt.acting == btAbajo()) dFoe = sac; else dMine = sac;
  }

  // cuanto lleva hundido el que cayo, 0..1
  // El temblor de la mega: el bicho vibra mientras dura la escena.
  if (bt.phase == BP_MEGA) {
    uint32_t t = millis() - bt.megaAt;
    // Del lado al que le toca. Estaba clavado en el mio porque el rival
    // no llegaba a esta fase: megaevolucionaba en el acto, sin escena.
    (bt.megaSide == btAbajo() ? dMine : dFoe) +=
        ((t / 40) % 2 ? 3.0f : -3.0f);
  }
  float hMine = 0, hFoe = 0;
  if (bt.phase == BP_FAINT) {
    float u = 1.0f - (float)((int32_t)bt.until - (int32_t)millis()) / 700.0f;
    if (u < 0) u = 0;
    if (u > 1) u = 1;
    (bt.fainter == btAbajo() ? hMine : hFoe) = u;
  } else if (bt.phase == BP_END || bt.phase == BP_LOOT ||
             bt.phase == BP_PARTY || bt.phase == BP_ESPERA) {
    // (BP_ESPERA: entre placas, esperando a outra ou dizendo o resultado)
    // El que ya cayo se queda hundido en TODO lo que viene despues.
    // Estaba atado a BP_FAINT y BP_END, y entre esas dos esta BP_LOOT —— la
    // pantalla del objeto que te llevas ——, asi que al ganar con premio el
    // rival volvia a aparecer entero mientras leias lo que habias cogido.
    //
    // Y se miran los DOS lados: el `? :` de antes hundia uno solo, asi que
    // si caian a la vez (retroceso, veneno) uno se quedaba de pie.
    if (bt.b.side[btAbajo()].fainted()) hMine = 1.0f;
    if (bt.b.side[btArriba()].fainted()) hFoe = 1.0f;
  }

  // El que entra sube de la plataforma: es el mismo parametro con el que
  // se hunde el que cae, usado al reves. Asi la entrada no necesita un
  // escalado de sprites que el dibujante no tiene.
  {
    float uM = btSendoutU(btAbajo(), millis());
    float uF = btSendoutU(btArriba(), millis());
    // solo la segunda mitad: antes de eso la bola todavia esta cerrada
    if (uM >= 0) hMine = uM < 0.5f ? 1.0f : 1.0f - (uM - 0.5f) / 0.5f;
    if (uF >= 0) hFoe = uF < 0.5f ? 1.0f : 1.0f - (uF - 0.5f) / 0.5f;
  }

  // EXCAVAR, VUELO y BUCEO. El motor saca al bicho del campo durante un
  // turno (Combatant::hiding) y hasta ahora el dibujante no lo miraba:
  // el sprite se quedaba a la vista mientras los golpes le pasaban por
  // encima, que es como se ve un juego roto.
  //
  // No hace falta ningun aviso desde la logica: basta con mirar `hiding`
  // en cada fotograma y apuntar CUANDO cambio. Asi vale igual para el
  // turno de carga (se esconde), para el de salida (vuelve) y para
  // cualquier cosa que lo saque de ahi —— un relevo, un Smack Down ——
  // sin tener que acordarse de llamar a nadie.
  float volMine = 0, volFoe = 0;
  for (uint8_t s = 0; s < 2; s++) {
    uint8_t ahora = bt.b.side[s].hiding;
    // carregando: ainda nao sumiu (ver cargaAte)
    if ((int32_t)(millis() - bt.cargaAte[s]) < 0 && ahora != HID_NONE &&
        bt.hidWas[s] == HID_NONE)
      continue;
    if (ahora != bt.hidWas[s]) {
      bt.hidWas[s] = ahora;
      bt.hidAt[s] = millis();
      // La CLASE se guarda aparte: al volver, `hiding` ya es HID_NONE y
      // sin esto no habria forma de saber si sale de un agujero o baja
      // del cielo.
      if (ahora != HID_NONE) bt.hidKind[s] = ahora;
      if (ahora != HID_NONE && bt.cargaAnim[s])
        bt.hidAt[s] = millis() - BT_HIDE_MS;   // a animacao ja o levou
      bt.cargaAnim[s] = false;
    }
    if (!bt.hidKind[s]) continue;
    float u = (float)(millis() - bt.hidAt[s]) / (float)BT_HIDE_MS;
    if (u > 1.0f) u = 1.0f;
    if (ahora == HID_NONE) {
      u = 1.0f - u;                      // vuelve deshaciendo el camino
      if (u <= 0.0f) { bt.hidKind[s] = HID_NONE; continue; }
    }
    if (bt.hidKind[s] == HID_SKY) {
      (s == btAbajo() ? volMine : volFoe) = u;
    } else {
      // Bajo tierra y bajo el agua se van para ABAJO, que es justo lo
      // que ya sabe hacer `hundir` (y encima recorta contra el suelo).
      float &h = (s == btAbajo() ? hMine : hFoe);
      if (u > h) h = u;
    }
  }

  btSyncFoeSprite();
  btSyncMySprite();
  // Ordem do Essentials: plano de fundo (z5), particulas de tras, rival
  // (z45), as do meio, jogador (z50), as da frente, plano da frente (z85).
  gAnimOn = (aq != nullptr);
  // O CLIMA abraca tudo: o plano de fundo dele vai antes do do golpe, e
  // a frente dele (as gotas, a areia) depois da do golpe. As chapas de PS
  // e os botoes vem mais abaixo e ficam por cima, como no Anil.
  const uint8_t *cq = btClimaQuadro(millis());
  if (cq) {
    btAnimPlano(cq + 1);
    btClimaCamada(cq, 0);
  }
  if (aq) {
    gAnimSpr[BT_PLAYER] = btAnimBicho(aq, BT_PLAYER);
    gAnimSpr[BT_FOE] = btAnimBicho(aq, BT_FOE);
    btAnimPlano(aq + 1);
    btAnimCamada(aq, 0);
  }
  btDrawSide(foePmd, bt.b.side[btArriba()], BT_FOE_CX, BT_GROUND_FOE, false, dFoe, hFoe,
             btSuckProgress(millis()), volFoe);
  if (aq) btAnimCamada(aq, 1);
  btDrawSide(pmd, bt.b.side[btAbajo()], 145, BT_GROUND_MINE, true, dMine, hMine,
             0.0f, volMine);
  if (aq) {
    btAnimCamada(aq, 2);
    btAnimPlano(aq + 5);
  }
  if (cq) {
    const uint32_t tc = millis();
    btClimaCamada(cq, 2);
    btClimaFigura(cq);
    btAnimPlano(cq + 5);
    gClimaMs = millis() - tc;
  } else {
    gClimaMs = 0;
  }
  gAnimOn = false;

  btSendoutFx(millis());       // la bola del que entra, delante del bicho

  // Va DESPUES de los bichos y ANTES de las chapas: el destello envuelve al
  // bicho, pero no puede taparle la barra de PS.
  btMoveFx(millis());
  btMegaFx(millis());          // los anillos de la mega, si toca

  uint32_t desde = millis() - bt.startedAt;
  if (desde < SPARK_TOTAL_MS) {
    if (bt.b.side[btArriba()].shiny) btShinySpark(desde, 347, BT_GROUND_FOE - 54);
    if (bt.b.side[btAbajo()].shiny) btShinySpark(desde, 145, BT_GROUND_MINE - 76);
  }

  // La bolita solo contra un SALVAJE: el bicho de un entrenador no se
  // captura, asi que decir "ya lo tienes" ahi no serviria de nada.
  bool yaLoTengo = !bt.pvp && (bt.trainer < 0 && bt.gym < 0) &&
                   pet.isRegistered(bt.b.side[BT_FOE].dex);
  btHpBar(68, bt.b.side[btArriba()], -1, false, yaLoTengo);
  btHpBar(228, bt.b.side[btAbajo()], +1, !bt.pvp);
  btFieldChips();

  // La bola se ve mientras vuela, mientras traga, mientras forcejea y
  // cuando ya esta cerrada con el dentro.
  if (bt.phase == BP_THROW || bt.phase == BP_SUCK ||
      bt.phase == BP_WOBBLE || bt.phase == BP_CAUGHT)
    btDrawBall(millis());
  if (bt.msg[0]) btTextBox(bt.msg);
  else if (bt.phase == BP_CHOOSE) btMoveCard();
  if (bt.phase == BP_CHOOSE) {
    if (!bt.pvp || pvpBolsaNaRegra()) btDrawBagButton();
    btDrawPartyButton();
    btDrawMegaButton();
    btDrawRunButton();
  }
  if (bt.phase == BP_BAG) btRenderBag();
  if (bt.phase == BP_PARTY) btRenderParty();
  if (bt.phase == BP_WIN) btRenderWin();
  if (bt.phase == BP_BADGE) btRenderBadge();
  if (bt.pvp) btDrawRelogioPvp();
  gQuadroMs = millis() - t0;
}

// Cuanto lleva de entrada el bicho que acaba de salir de la bola, 0..1.
// Devuelve -1 si ese lado no esta entrando ahora mismo.
static float btSendoutU(uint8_t lado, uint32_t now) {
  if (!bt.sendAt || bt.sendSide != lado) return -1.0f;
  uint32_t t = now - bt.sendAt;
  if (t >= BT_SENDOUT_MS) return -1.0f;
  return (float)t / (float)BT_SENDOUT_MS;
}

// La bola que suelta al que entra. Va DESPUES de los bichos: la bola se
// ve delante, y el bicho asoma por detras mientras ella se abre.
//
// Primera mitad cerrada y cayendo hacia la plataforma; segunda mitad
// abierta y desvaneciendose, que es cuando el bicho ya esta subiendo.
static void btSendoutFx(uint32_t now) {
  for (uint8_t lado = 0; lado < 2; lado++) {
    float u = btSendoutU(lado, now);
    btBolaX[lado] = -1;
    if (u < 0) continue;
    bool mio = (lado == btAbajo());
    int cx = mio ? 145 : BT_FOE_CX;
    int suelo = mio ? BT_GROUND_MINE : BT_GROUND_FOE;
    // cae desde arriba, con un poco de arco hacia dentro
    float caida = u < 0.5f ? (u / 0.5f) : 1.0f;
    int y = (int)(suelo - 120 + caida * 110);
    // DESDE DONDE LA TIRAN, que es lo que estaba al reves. Cada bola sale
    // de su entrenador: el jugador esta FUERA DE PANTALLA POR LA
    // IZQUIERDA (su bicho se pinta de espaldas en x=145, y el que lo
    // manda esta detras y a su lado), y el rival por la DERECHA. Asi que
    // la tuya entra de izquierda a derecha y la suya de derecha a
    // izquierda. El signo de antes mandaba las dos hacia dentro desde el
    // centro, o sea cada una desde el lado del otro.
    int x = cx + (int)((mio ? -40 : 40) * (1.0f - caida));
    // O SEU sai da bola em que foi capturado; o rival e os que nasceram
    // aqui, da Poke Ball de sempre.
    uint8_t tier = 0;
    if (mio && bt.mySlot < TEAM_SLOTS) {
      uint8_t b = btIsMain(bt.mySlot) ? pet.ballId : ballDe(pet.team[bt.mySlot]);
      tier = ballArte(b);
    }
    const Art *bola = (u < 0.5f)
        ? ART_BALL[tier][(uint8_t)(u * 16.0f) % BALL_SPIN_FRAMES]
        : ART_BALL_OPEN[tier];
    btBolaX[lado] = (int16_t)x;
    drawArt(*bola, x, y + bola->h, 1);
  }
}

// ---------------------------------------------------------------- toque
void battleTap(int16_t x, int16_t y) {
  // El resumen se va cuando TU quieras, no cuando se acabe un reloj.
  if (bt.phase == BP_WIN) {
    // Venceu um LIDER (nao a Elite 4, nao a revanche): antes de sair, a
    // tela da insignia. winGym roda AQUI para ela poder contar a MT e o
    // premio, e o battleClose fica sabendo (insigniaDada).
    if (bt.gym >= 0 && bt.gym < GYM_BADGES && !bt.rematch &&
        bt.b.winner == BT_PLAYER && !bt.insigniaDada) {
      pet.winGym();
      bt.insigniaDada = true;
      bt.insigniaIdx = pet.badges ? (uint8_t)(pet.badges - 1) : 0;
      bt.phase = BP_BADGE;
      bt.until = millis();
      jinglePlay(JIN_BADGE);
      return;
    }
    bt.phase = BP_END;
    bt.until = millis();
    sfxPlay(SFX_TAP);
    return;
  }
  if (bt.phase == BP_BADGE) {
    bt.phase = BP_END;
    bt.until = millis();
    sfxPlay(SFX_TAP);
    return;
  }
  if (bt.pvp && bt.phase == BP_PARTY) { pvpPartyTap(x, y); return; }
  if (btPartyTap(x, y)) return;        // la lista de relevos, si esta abierta
  // A bandeira armada vem ANTES: tocar fora dela (na bolsa tambem) so desarma.
  if (bt.pvp && pvpDesistirTap(x, y)) return;   // a bandeira (2 toques)
  if ((!bt.pvp || pvpBolsaNaRegra()) && btCaptureTap(x, y)) return;   // bolsa, bola y forcejeo
  if (bt.phase != BP_CHOOSE) return;   // durante la animacion no se acepta nada
  if (btMegaTap(x, y)) return;         // megaevolucionar, antes de elegir golpe
  if (btRunTap(x, y)) return;          // huir, si es un salvaje

  // el boton de equipo, hermano del de la bolsa
  if (btReserveCount() > 0) {
    int dx = x - BPTY_BTN_X, dy = y - BPTY_BTN_Y;
    if (dx * dx + dy * dy <= (BPTY_BTN_R + 8) * (BPTY_BTN_R + 8)) {
      bt.phase = BP_PARTY;
      bt.partyScroll = 0;
      sfxPlay(SFX_TAP);
      return;
    }
  }

  const Combatant &me = bt.b.side[btAbajo()];
  if (y < BT_CARD_Y0 || y > BT_CARD_Y1) return;

  int w = btBandW(BT_CARD_Y0, BT_CARD_Y1);
  if (w > 272) w = 272;
  int x0 = 233 - w / 2;

  if (x < x0 + 52) {                       // flecha izquierda
    if (bt.sel > 0) { bt.sel--; sfxPlay(SFX_TAP); }
    return;
  }
  if (x > x0 + w - 52) {                   // flecha derecha
    if (bt.sel + 1 < me.nMoves) { bt.sel++; sfxPlay(SFX_TAP); }
    return;
  }

  // Entre placas o toque so DECIDE: o turno corre quando chegar a jogada
  // da outra placa (pvpDecide). A mega vai junto, marcada antes.
  if (bt.pvp) {
    DueloDecisao d;
    d.tipo = DD_GOLPE;
    d.arg = bt.sel;
    d.extra = bt.pvpMega ? 1 : 0;
    sfxPlay(SFX_TAP);
    pvpDecide(pvpEu, d);
    return;
  }
  // El rival megaevoluciona AQUI, antes de que se decida el orden: la
  // mega cambia la velocidad, asi que hacerlo despues seria mover la
  // porteria a mitad del turno.
  //
  // Y ahora con ESCENA. Antes se llamaba a doMega() en esta misma linea:
  // el bicho cambiaba de sprite entre dos fotogramas, sin anillos, sin
  // sello y sin sonido. La transformacion mas gorda del combate duraba
  // cero cuando la hacia el de enfrente, y el dueno lo vio enseguida.
  // La escena acaba en BP_MEGA_SAY, que es quien sigue el turno.
  if (bt.b.canMega(BT_FOE) >= 0) {
    bt.phase = BP_MEGA;
    bt.megaSide = BT_FOE;
    bt.megaAt = millis();
    bt.until = bt.megaAt + BT_MEGA_MS;
    bt.msg[0] = 0;
    if (!jinglePlay(JIN_MEGA)) sfxPlay(SFX_MEDAL);
    return;
  }

  btCommitTurn();
}

// El turno se lanza DE VERDAD: se decide el orden y sale el primero; el
// segundo va solo cuando acabe la animacion de este.
//
// Esta aparte de battleTap porque hay dos caminos hasta aqui: tocar la
// tarjeta de golpe, y salir de la escena de la mega del rival (que se
// mete por medio entre las dos cosas).
// El blanco de la mega, 0..1. Durante la escena (BT_MEGA_MS) sube del
// 15% al 70% del tiempo y se queda en blanco puro; al transformarse (el
// sprite ya es el nuevo) baja a cero en BT_MEGA_REVELA_MS. Es el
// pbFlashInOut del Anil: 0,7 s hacia el blanco y 0,4 s de vuelta.
#define BT_MEGA_REVELA_MS 400
float btMegaBlanco(uint8_t lado, uint32_t now) {
  if (!bt.open) return 0.0f;
  if (bt.phase == BP_MEGA && bt.megaSide == lado) {
    float t = (float)(now - bt.megaAt) / (float)BT_MEGA_MS;
    if (t <= 0.15f) return 0.0f;
    if (t >= 0.70f) return 1.0f;
    return (t - 0.15f) / 0.55f;
  }
  if (bt.megaRevelaLado == lado) {
    uint32_t d = now - bt.megaRevelaAt;
    if (d < BT_MEGA_REVELA_MS) return 1.0f - (float)d / BT_MEGA_REVELA_MS;
  }
  return 0.0f;
}

// El escenario en el idioma del Anil: el `environment` que mira Poder
// Secreto. Hierba (pradera y bosque), arena (playa), roca (montana),
// cueva, nieve, volcan; ciudad, noche, gimnasios y laboratorio no son
// ninguno, como un edificio en el Anil.
static uint8_t bgEntorno(int8_t bg) {
  auto en = [bg](const uint8_t *ids, uint8_t n) {
    for (uint8_t i = 0; i < n; i++) if (ids[i] == (uint8_t)bg) return true;
    return false;
  };
  if (bg < 0) return EN_NONE;
  if (en(BGS_FIELD, sizeof(BGS_FIELD)) || en(BGS_FOREST, sizeof(BGS_FOREST)))
    return EN_GRASS;
  if (en(BGS_BEACH, sizeof(BGS_BEACH)) || en(BGS_MOUNT, sizeof(BGS_MOUNT)))
    return EN_SAND;
  if (en(BGS_CAVE, sizeof(BGS_CAVE))) return EN_CAVE;
  if (en(BGS_SNOW, sizeof(BGS_SNOW))) return EN_SNOW;
  if (en(BGS_VOLC, sizeof(BGS_VOLC))) return EN_VOLCANO;
  return EN_NONE;
}

// PALIZA pega una vez por cada uno del equipo que puede (en pie y sin
// estado), con el ataque base de CADA uno. El motor no ve el banquillo;
// se lo cuenta esto. Del rival solo se conoce al que esta: los demas de
// un entrenador se sortean al salir, asi que cuentan con el mismo ataque.
static void btCuentaEquipos() {
  uint8_t atk[6], n = 0;
  const Combatant &yo = bt.b.side[BT_PLAYER];
  if (!yo.fainted() && yo.status == ST_NONE) atk[n++] = DEX_TBL[yo.asDex].bAtk;
  for (uint8_t i = 0; i < TEAM_SLOTS && n < 6; i++)
    if (btCanSend(i)) atk[n++] = DEX_TBL[pet.team[i].species].bAtk;
  bt.b.ponEquipo(BT_PLAYER, atk, n);
  const Combatant &el = bt.b.side[BT_FOE];
  n = 0;
  uint8_t quedan = bt.foesLeft ? bt.foesLeft : 1;
  for (uint8_t i = 0; i < quedan && n < 6; i++) {
    if (i == 0 && (el.fainted() || el.status != ST_NONE)) continue;
    atk[n++] = DEX_TBL[el.asDex].bAtk;
  }
  bt.b.ponEquipo(BT_FOE, atk, n);
}

void btCommitTurn() {
  bt.foeSlot = bt.b.pickFoeMove();
  bt.b.entorno = bgEntorno(bt.bg);
  btCuentaEquipos();
  // Pico Canon y Coparazon se preparan ANTES de que nadie se mueva.
  bt.b.inicioDeRonda(bt.sel, bt.foeSlot);
  // Los dos huecos van al orden porque Prankster adelanta los golpes
  // de APOYO, y para saber si el que sale es de apoyo hay que saber
  // cual es. Sin ellos el orden lo decide la velocidad, como siempre.
  bool yoPrimero = bt.b.movesFirst(BT_PLAYER, bt.sel, bt.foeSlot);
  bt.secondPending = true;
  btLaunch(yoPrimero ? BT_PLAYER : BT_FOE,
           yoPrimero ? bt.sel : bt.foeSlot);
}


// ================================================== BATALHA ENTRE PLACAS (PvP)
// O combate e o MESMO de sempre, passo a passo, nas duas placas: o que
// muda e de onde vem a jogada de cada lado e o que nao se pode gravar.
// O desenho esta em PENDENCIAS.md (leva 57/58).
//
//   ponto de decisao  ->  as duas jogadas na mesa  ->  a FILA do turno
//
// Um PONTO abre no menu (os dois lados decidem) ou depois de um desmaio /
// U-turn (so quem repoe). Fechado, pvpExecuta monta a fila: megas e
// trocas primeiro, por velocidade, e os golpes por fim, na ordem do
// movesFirst (decidida so na hora, depois das trocas: quem entrou e
// outro bicho com outra velocidade). Cada acao sai pelo caminho de
// sempre (btLaunch, BP_MSG...) e, no lugar do "golpe do outro"
// (secondPending), BP_MSG pede a proxima a pvpSegue.

bool pvpDisseFim = false;

static void pvpAbre(uint8_t ponto, uint8_t precisa) {
  pvpPonto = ponto;
  pvpPrecisa = precisa;
  pvpTem = 0;
  pvpRedeOk = false;
  pvpDesistirAte = 0;
  pvpAbertoEm = millis();
  bt.pvpMega = false;
  // A impressao digital do combate NESTE ponto: vai com a minha jogada e
  // o Duelo confere com a da outra placa (diferente = dessincronia).
  pvpHashPonto = pvpHash(bt.b, pvpL);
  if (ponto == PT_REPOE) {
    if (precisa & (1u << pvpEu)) {
      bt.mustSwitch = true;
      bt.partyScroll = 0;
      bt.phase = BP_PARTY;
    } else {
      bt.mustSwitch = false;
      bt.phase = BP_ESPERA;
      if (!bt.msg[0]) snprintf(bt.msg, sizeof(bt.msg), "Waiting for the other board...");
    }
  } else {
    bt.mustSwitch = false;
    bt.phase = BP_CHOOSE;
  }
  // Nao me cabe decidir (so a outra repoe): digo NADA ja, senao a outra
  // espera por mim e o Duelo nao passa ao ponto seguinte.
  if (pvpRede && pvpEnvia && !(precisa & (1u << pvpEu))) {
    DueloDecisao nada;
    pvpEnvia(nada, pvpHashPonto);
  }
  if (pvpAoAbrir) pvpAoAbrir();
}

static void pvpAbreTurno() {
  bt.msg[0] = 0;
  pvpAbre(PT_TURNO, 3);
}

// PALIZA: o ataque base dos que podem pegar, de cada lado, do banco do
// COMBATE (nao do time da casa).
static void pvpCuentaEquipos() {
  for (uint8_t s = 0; s < 2; s++) {
    uint8_t atk[6], n = 0;
    const Combatant &c = bt.b.side[s];
    if (!c.fainted() && c.status == ST_NONE) atk[n++] = DEX_TBL[c.asDex].bAtk;
    for (uint8_t i = 0; i < pvpL[s].n && n < 6; i++)
      if (pvpL[s].podeEntrar(i) && pvpL[s].mon[i].status == ST_NONE)
        atk[n++] = DEX_TBL[pvpL[s].reg[i].species].bAtk;
    bt.b.ponEquipo(s, atk, n);
  }
}

static void pvpCarregaMeu(const Combatant &c) {
  int16_t fr = formRow(c.dex, c.form);
  if (fr >= 0) pmd.loadForm(fr);
  else pmd.load(c.dex, c.shiny, c.sexo == 1);
  myPmdFor = -1;
  myPmdTried = -1;
}

// Poe `i` na pista do `lado`, com a frase e o sprite. A fase fica em
// BP_MSG (a frase), e dali a fila segue.
static bool pvpTrocaAgora(uint8_t lado, uint8_t i, bool porGusto) {
  if (!pvpEntra(bt.b, pvpL, lado, i, porGusto)) return false;
  const Combatant &c = bt.b.side[lado];
  if (lado == pvpEu) {           // o meu (de costas, embaixo)
    pvpCarregaMeu(c);
  } else {
    foePmdFor = -1;      // btSyncFoeSprite carrega o novo no proximo quadro
    foePmdTried = -1;
  }
  cryPlay((uint16_t)c.dex);
  if (lado == pvpEu) snprintf(bt.msg, sizeof(bt.msg), "Go, %s!", dexName(c.pareceDex()));
  else snprintf(bt.msg, sizeof(bt.msg), "Foe sends %s!", dexName(c.pareceDex()));
  bt.sendAt = millis();
  bt.sendSide = lado;
  bt.phase = BP_MSG;
  bt.extraStep = 9;
  bt.until = millis() + 900;
  {
    uint32_t dura = btAnimDoCampo(lado, millis() + 900);
    if (dura && millis() + 900 + dura > bt.until) bt.until = millis() + 900 + dura;
  }
  bt.secondPending = pvpI < pvpN;
  return true;
}

// A proxima acao da fila. true se pos alguma coisa a correr.
bool pvpSegue() {
  while (pvpI < pvpN) {
    PvpAcao a = pvpFila[pvpI++];
    if (pvpL[a.lado].ativo != a.quem) continue;
    // quem caiu nao age; o RELEVO e justamente de quem caiu
    if (a.tipo != PA_REPOE && bt.b.side[a.lado].fainted()) continue;
    switch (a.tipo) {
      case PA_MEGA:
        if (bt.b.canMega(a.lado) < 0) continue;
        bt.phase = BP_MEGA;
        bt.megaSide = a.lado;
        bt.megaAt = millis();
        bt.until = bt.megaAt + BT_MEGA_MS;
        bt.msg[0] = 0;
        if (!jinglePlay(JIN_MEGA)) sfxPlay(SFX_MEDAL);
        return true;
      case PA_TROCA:
        if (!bt.b.canSwitchOut(a.lado) || !pvpL[a.lado].podeEntrar(a.arg)) continue;
        if (pvpTrocaAgora(a.lado, a.arg, true)) return true;
        continue;
      case PA_ITEM: {
        // As duas placas aplicam o MESMO objeto no MESMO alvo. Se ja nao
        // serve (ou a regra acabou), cai sem gastar, como uma troca que
        // ja nao pode.
        uint8_t lado = a.lado, id = a.arg, idx = a.alvo;
        if (!pvpPodeUsarItem(pvpRegras, pvpL[lado]) || !pvpItemServe(lado, id, idx)) continue;
        PvpLado &l = pvpL[lado];
        bool naPista = idx == l.ativo;
        Combatant &c = naPista ? bt.b.side[lado] : l.mon[idx];
        btItemNoCombatente(c, id, true);
        l.itensUsados++;
        if (lado == pvpEu && pvpBolsa[id]) pvpBolsa[id]--;
        pvpSincroniza(bt.b, pvpL);      // um Revive devolve alguem ao banco
        const char *quem = dexName(naPista ? c.asDex : (int16_t)l.reg[idx].species);
        snprintf(bt.msg, sizeof(bt.msg), lado == pvpEu ? "Used %s on %s!" : "Foe used %s on %s!",
                 ITEM_TBL[id].name, quem);
        sfxPlay(SFX_EAT);
        bt.phase = BP_MSG;
        bt.extraStep = 9;
        bt.until = millis() + 1200;
        bt.secondPending = pvpI < pvpN;
        return true;
      }
      case PA_REPOE:
        // depois de cair ou de um U-turn: sai sempre, e nao e "por gusto"
        if (!pvpL[a.lado].podeEntrar(a.arg)) continue;
        if (pvpTrocaAgora(a.lado, a.arg, false)) return true;
        continue;
      case PA_GOLPE: {
        // Os dois golpes: a ordem sai AGORA, com quem esta na pista agora.
        if (pvpI < pvpN && pvpFila[pvpI].tipo == PA_GOLPE &&
            pvpL[pvpFila[pvpI].lado].ativo == pvpFila[pvpI].quem &&
            !bt.b.side[pvpFila[pvpI].lado].fainted()) {
          PvpAcao b2 = pvpFila[pvpI];
          if (!bt.b.movesFirst(a.lado, a.arg, b2.arg)) {
            pvpFila[pvpI] = a;
            a = b2;
          }
        }
        bt.secondPending = pvpI < pvpN;
        btLaunch(a.lado, a.arg);
        return true;
      }
      default:
        continue;
    }
  }
  bt.secondPending = false;
  return false;
}

// O fim: quem ganhou (0xFF = empate). A frase do desmaio fica no ar um
// pouco; depois BP_ESPERA diz o resultado e fecha (ver o tick).
static void pvpFim(uint8_t vence) {
  pvpAcabou = true;
  pvpVenceu = vence;
  pvpPonto = PT_NADA;
  pvpN = pvpI = 0;
  bt.secondPending = false;
  bt.mustSwitch = false;
  pvpDisseFim = false;
  bt.phase = BP_ESPERA;
  bt.until = millis() + (bt.msg[0] ? 1800 : 0);
}

// As duas jogadas estao na mesa: o turno (ou os relevos) comecam.
static void pvpExecuta() {
  uint8_t ponto = pvpPonto;
  pvpPonto = PT_NADA;
  pvpPontos++;
  bt.msg[0] = 0;
  bt.mustSwitch = false;
  if (ponto == PT_REPOE) {
    // Os relevos entram NA FRENTE do que sobrou da fila: depois de um
    // U-turn, o golpe do outro ainda vem.
    PvpAcao novas[2];
    uint8_t k = 0;
    for (uint8_t s = 0; s < 2; s++)
      if (pvpPrecisa & (1u << s))
        novas[k++] = { s, PA_REPOE, pvpQuer[s].arg, pvpL[s].ativo, 0 };
    for (int j = (int)pvpN - 1; j >= (int)pvpI; j--) pvpFila[j + k] = pvpFila[j];
    for (uint8_t j = 0; j < k; j++) pvpFila[pvpI + j] = novas[j];
    pvpN = (uint8_t)(pvpN + k);
    if (pvpSegue()) return;
    bt.phase = BP_MSG;
    bt.extraStep = 9;
    bt.secondPending = pvpI < pvpN;
    bt.until = millis();
    return;
  }
  // ---- o turno. Desistir acaba ja.
  bool d0 = pvpQuer[0].tipo == DD_DESISTE, d1 = pvpQuer[1].tipo == DD_DESISTE;
  if (d0 || d1) {
    snprintf(bt.msg, sizeof(bt.msg), d0 && d1 ? "Both gave up!"
             : ((d0 ? 0 : 1) == pvpEu) ? "You gave up..." : "The other board gave up!");
    pvpFim(d0 && d1 ? 0xFF : d0 ? 1 : 0);
    return;
  }
  pvpN = pvpI = 0;
  bool usa[2];
  uint8_t slot[2];
  for (uint8_t s = 0; s < 2; s++) {
    usa[s] = pvpQuer[s].tipo == DD_GOLPE;
    slot[s] = usa[s] ? pvpQuer[s].arg : 0;
  }
  // Megas e trocas: o mais rapido primeiro. So se tira a ordem se ha duas
  // (o empate gasta o dado, e as duas placas gastam igual).
  bool mega[2], troca[2], item[2];
  for (uint8_t s = 0; s < 2; s++) {
    mega[s] = usa[s] && (pvpQuer[s].extra & 1) && bt.b.canMega(s) >= 0;
    troca[s] = pvpQuer[s].tipo == DD_TROCA && bt.b.canSwitchOut(s) &&
               pvpL[s].podeEntrar(pvpQuer[s].arg);
    item[s] = pvpQuer[s].tipo == DD_ITEM;
  }
  uint8_t pri = 0;
  if ((mega[0] && mega[1]) || (troca[0] && troca[1]) || (item[0] && item[1]))
    pri = bt.b.movesFirst(BT_PLAYER) ? 0 : 1;
  // A ordem do Anil (pbAttackPhase): trocas, objetos, megas e golpes.
  // Antes a mega vinha antes da troca.
  for (uint8_t o = 0; o < 2; o++) {
    uint8_t s = o ? (uint8_t)(pri ^ 1) : pri;
    if (troca[s]) pvpFila[pvpN++] = { s, PA_TROCA, pvpQuer[s].arg, pvpL[s].ativo, 0 };
  }
  for (uint8_t o = 0; o < 2; o++) {
    uint8_t s = o ? (uint8_t)(pri ^ 1) : pri;
    if (item[s])
      pvpFila[pvpN++] = { s, PA_ITEM, pvpQuer[s].arg, pvpL[s].ativo, pvpQuer[s].extra };
  }
  for (uint8_t o = 0; o < 2; o++) {
    uint8_t s = o ? (uint8_t)(pri ^ 1) : pri;
    if (mega[s]) pvpFila[pvpN++] = { s, PA_MEGA, 0, pvpL[s].ativo, 0 };
  }
  for (uint8_t s = 0; s < 2; s++)
    if (usa[s]) pvpFila[pvpN++] = { s, PA_GOLPE, slot[s], pvpL[s].ativo, 0 };
  bt.b.inicioDeRondaDos(usa[0], slot[0], usa[1], slot[1]);
  bt.b.entorno = bgEntorno(bt.bg);
  pvpCuentaEquipos();
  bt.endStep = 0;
  bt.extraStep = 9;
  if (!pvpSegue()) {
    bt.phase = BP_MSG;
    bt.until = millis();
  }
}

void pvpDecide(uint8_t lado, const DueloDecisao &d) {
  if (lado > 1 || pvpPonto == PT_NADA || pvpAcabou) return;
  uint8_t bit = (uint8_t)(1u << lado);
  if (!(pvpPrecisa & bit) || (pvpTem & bit)) return;
  pvpQuer[lado] = d;
  pvpTem |= bit;
  if (lado == pvpEu && pvpRede && pvpEnvia) pvpEnvia(d, pvpHashPonto);
  if (lado == pvpEu) pvpDesistirAte = 0;
  pvpTalvezExecuta();
}

// O ponto fecha quando os lados que decidem decidiram E, pela rede, o
// Duelo trouxe as duas (mesmo quando a da outra e NADA): assim as duas
// placas passam juntas de ponto e o hash de cada um e conferido.
static void pvpTalvezExecuta() {
  if ((pvpTem & pvpPrecisa) != pvpPrecisa || (pvpRede && !pvpRedeOk)) {
    uint8_t bit = (uint8_t)(1u << pvpEu);
    bool minhaFeita = !(pvpPrecisa & bit) || (pvpTem & bit);
    if (minhaFeita && bt.phase != BP_ESPERA) {
      bt.phase = BP_ESPERA;
      snprintf(bt.msg, sizeof(bt.msg), "Waiting for the other board...");
    }
    return;
  }
  pvpExecuta();
}

// A troca_ui: o Duelo tem as duas do ponto aberto. true = consumi (ela
// avanca o Duelo). A minha ja esta na mesa: o Duelo so as tem se eu mandei.
bool pvpChegou(const DueloDecisao &dele) {
  if (!bt.open || !bt.pvp || !pvpRede || pvpPonto == PT_NADA || pvpAcabou || pvpRedeOk)
    return false;
  uint8_t o = (uint8_t)(pvpEu ^ 1), bit = (uint8_t)(1u << o);
  pvpRedeOk = true;
  if (pvpPrecisa & bit) {
    if (dele.tipo == DD_NADA) {
      // ela acha que nao decide aqui: as duas ja nao veem o mesmo combate
      pvpInterrompe("The boards fell out of sync!", 0xFF);
      return true;
    }
    pvpQuer[o] = dele;
    pvpTem |= bit;
  }
  pvpTalvezExecuta();
  return true;
}

// Acabou sem combate acabado: a outra sumiu, saiu, divergiu, ou eu desisti.
// A frase e a que vem; nada de "You won!" depois.
void pvpInterrompe(const char *msg, uint8_t vence) {
  if (!bt.open || !bt.pvp || pvpAcabou) return;
  snprintf(bt.msg, sizeof(bt.msg), "%s", msg);
  pvpFim(vence);
  pvpCortado = true;
  pvpDisseFim = true;
  bt.until = millis() + 2500;
}

// Quanto falta para a placa escolher por mim (0xFFFFFFFF = nao estou
// escolhendo agora).
uint32_t pvpRestaMs() {
  if (!bt.open || !bt.pvp || pvpPonto == PT_NADA || pvpAcabou) return 0xFFFFFFFFu;
  uint8_t bit = (uint8_t)(1u << pvpEu);
  if (!(pvpPrecisa & bit) || (pvpTem & bit)) return 0xFFFFFFFFu;
  uint32_t foi = millis() - pvpAbertoEm;
  return foi >= PVP_TEMPO_MS ? 0 : PVP_TEMPO_MS - foi;
}

// O que a placa escolhe quando o tempo acaba: o golpe marcado (ou o
// primeiro com PP), ou o primeiro do banco que pode entrar.
static DueloDecisao pvpEscolhaSozinha() {
  DueloDecisao d;
  if (pvpPonto == PT_REPOE) {
    d.tipo = DD_TROCA;
    for (uint8_t i = 0; i < pvpL[pvpEu].n; i++)
      if (pvpL[pvpEu].podeEntrar(i)) { d.arg = i; break; }
    return d;
  }
  const Combatant &c = bt.b.side[pvpEu];
  uint8_t s = bt.sel < c.nMoves ? bt.sel : 0;
  if (c.nMoves && c.usesLeft[s] == 0)
    for (uint8_t i = 0; i < c.nMoves; i++)
      if (c.usesLeft[i] > 0) { s = i; break; }
  d.tipo = DD_GOLPE;
  d.arg = bt.b.realSlot(pvpEu, s);
  return d;
}

static void pvpRelogioTick(uint32_t now) {
  // a bandeira armada e nao confirmada desarma sozinha
  if (pvpDesistirAte && (int32_t)(now - pvpDesistirAte) >= 0) {
    pvpDesistirAte = 0;
    if (bt.phase == BP_CHOOSE) bt.msg[0] = 0;
  }
  if (pvpRestaMs() != 0) return;
  DueloDecisao d = pvpEscolhaSozinha();
  bt.mustSwitch = false;
  if ((bt.phase == BP_PARTY || bt.phase == BP_BAG) && pvpPonto == PT_TURNO)
    bt.phase = BP_CHOOSE;
  bt.itemPick = 0;
  sfxPlay(SFX_DENY);
  pvpDecide(pvpEu, d);
}

// A placa fantasma do PC (troca_radio.cpp) joga pelo outro lado lendo o
// ponto aberto AQUI (e o mesmo processo). `k` = o ponto em que ela esta:
// so joga se for o mesmo que esta aberto na tela.
bool pvpFantasmaJoga(uint8_t lado, uint16_t k, DueloDecisao &d, uint32_t &hash) {
  if (!bt.open || !bt.pvp || pvpPonto == PT_NADA || pvpAcabou || lado > 1) return false;
  if (pvpPontos != k) return false;
  hash = pvpHashPonto;
  d = DueloDecisao();
  if (!(pvpPrecisa & (1u << lado))) return true;      // NADA
  if (pvpPonto == PT_REPOE) {
    d.tipo = DD_TROCA;
    for (uint8_t i = 0; i < pvpL[lado].n; i++)
      if (pvpL[lado].podeEntrar(i)) { d.arg = i; break; }
    return true;
  }
  const Combatant &c = bt.b.side[lado];
  d.tipo = DD_GOLPE;
  d.arg = bt.b.realSlot(lado, c.nMoves ? (uint8_t)random(c.nMoves) : 0);
  return true;
}

// A BOLSA ENTRE PLACAS. So o bolso de cura e das X (nada de bolas), e a
// regra do convite: proibida, ate 3 por combate ou livre. `idx` = o
// alvo no time do lado; as X (IU_BATTLER) so no que esta na pista.
bool pvpItemServe(uint8_t lado, uint8_t id, uint8_t idx) {
  if (lado > 1 || id == 0 || id >= ITEM_COUNT) return false;
  const ItemEntry &it = ITEM_TBL[id];
  if (it.pocket != BP_BATTLE || (it.use != IU_BATTLER && it.use != IU_POKEMON)) return false;
  const PvpLado &l = pvpL[lado];
  if (idx >= l.n || !l.reg[idx].used()) return false;
  if (it.use == IU_BATTLER && idx != l.ativo) return false;
  // so pergunta (aplicar = false): nao escreve nada
  Combatant &c = const_cast<Combatant &>(idx == l.ativo ? bt.b.side[lado] : l.mon[idx]);
  return btItemNoCombatente(c, id, false);
}

bool pvpBolsaNaRegra() { return bt.pvp && DR_ITENS(pvpRegras) != DI_PROIBIDO; }

// Posso abrir a bolsa agora: o ponto e o do turno, ainda nao escolhi, e
// a regra deixa (no "ate 3", sobra algum).
bool pvpBolsaPode() {
  if (!pvpBolsaNaRegra() || !bt.open || pvpAcabou || pvpPonto != PT_TURNO) return false;
  uint8_t bit = (uint8_t)(1u << pvpEu);
  if (!(pvpPrecisa & bit) || (pvpTem & bit)) return false;
  return pvpPodeUsarItem(pvpRegras, pvpL[pvpEu]);
}

// A minha jogada de objeto: vai pela rede como DD_ITEM e so se aplica
// quando o turno roda (nas duas placas, no mesmo lugar da fila).
void pvpUsaItem(uint8_t id, uint8_t idx) {
  if (!pvpBolsaPode() || id >= ITEM_COUNT || !pvpBolsa[id] || !pvpItemServe(pvpEu, id, idx)) {
    sfxPlay(SFX_DENY);
    return;
  }
  DueloDecisao d;
  d.tipo = DD_ITEM;
  d.arg = id;
  d.extra = idx;
  bt.itemPick = 0;
  bt.phase = BP_CHOOSE;
  bt.msg[0] = 0;
  sfxPlay(SFX_TAP);
  pvpDecide(pvpEu, d);
}

// Depois de cada golpe (no lugar do "agora o outro"): alguem foi
// arrastado (Rugido) ou pediu relevo (U-turn, Baton Pass, Wimp Out)?
static bool pvpMeioDoTurno() {
  for (uint8_t s = 0; s < 2; s++) {
    if (!bt.b.arrastado[s]) continue;
    bt.b.arrastado[s] = false;
    int8_t i = pvpSorteia(pvpL[s]);
    if (i >= 0 && pvpTrocaAgora(s, (uint8_t)i, false)) {
      snprintf(bt.msg, sizeof(bt.msg), "%s was dragged out!",
               dexName(bt.b.side[s].pareceDex()));
      return true;
    }
  }
  uint8_t precisa = 0;
  for (uint8_t s = 0; s < 2; s++) {
    if (!bt.b.querTrocar[s]) continue;
    bt.b.querTrocar[s] = false;
    if (pvpL[s].reservas()) precisa |= (uint8_t)(1u << s);
  }
  if (!precisa) return false;
  bt.msg[0] = 0;
  pvpAbre(PT_REPOE, precisa);
  return true;
}

// BP_FAINT entre placas. Caiu alguem: ou acabou, ou quem tem banco repoe
// (os dois, se cairam os dois). O resto da fila cai com eles, como no
// jogo de sempre, e o desgaste do fim do turno tambem (endStep 2).
static void pvpDesmaio() {
  bool c0 = bt.b.side[0].fainted(), c1 = bt.b.side[1].fainted();
  snprintf(bt.msg, sizeof(bt.msg), "%s fainted!", dexName(bt.b.side[bt.fainter].pareceDex()));
  bt.until = millis() + 1800;
  pvpN = pvpI = 0;
  bt.secondPending = false;
  bt.b.arrastado[0] = bt.b.arrastado[1] = false;
  // Os DOIS lados sem ninguem de pe (Explosion, recuo, Destiny Bond...):
  // EMPATE, como no Anil (pbJudge -> pbDecisionOnDraw = 5). O motor do
  // jogo normal da a vitoria a quem recebeu o golpe; aqui nao.
  if (c0 && c1 && !pvpL[0].reservas() && !pvpL[1].reservas()) {
    pvpFim(0xFF);
    return;
  }
  if (bt.b.over) {
    pvpFim(bt.b.winner <= 1 ? bt.b.winner : 0xFF);
    return;
  }
  uint8_t precisa = 0;
  for (uint8_t s = 0; s < 2; s++) {
    bool caiu = s ? c1 : c0;
    if (caiu && pvpL[s].reservas()) precisa |= (uint8_t)(1u << s);
    if (bt.b.querTrocar[s]) {
      bt.b.querTrocar[s] = false;
      if (!caiu && pvpL[s].reservas()) precisa |= (uint8_t)(1u << s);
    }
  }
  if (!precisa) {
    // caiu sem banco mas o motor nao deu o fim: decide quem esta de pe
    pvpFim(c0 && !c1 ? 1 : c1 && !c0 ? 0 : 0xFF);
    return;
  }
  bt.endStep = 2;
  pvpAbre(PT_REPOE, precisa);
}

static void pvpPartyTap(int16_t x, int16_t y) {
  if (bt.itemPick) {
    // com um objeto na mao: tocar um do time usa nele
    for (uint8_t i = 0; i < TEAM_SLOTS; i++) {
      int ry = BPTY_Y + i * BPTY_H;
      if (x < BPTY_X || x > BPTY_X + BPTY_W || y < ry || y > ry + BPTY_H - 6) continue;
      pvpUsaItem(bt.itemPick, i);
      return;
    }
    if (lejosDe(x, y, BPTY_PAN_X0, BPTY_PAN_Y0, BPTY_PAN_X1, BPTY_PAN_Y1)) {
      bt.itemPick = 0;
      bt.phase = BP_BAG;
    }
    return;
  }
  for (uint8_t i = 0; i < TEAM_SLOTS; i++) {
    int ry = BPTY_Y + i * BPTY_H;
    if (x < BPTY_X || x > BPTY_X + BPTY_W || y < ry || y > ry + BPTY_H - 6) continue;
    if (!btCanSend(i)) { sfxPlay(SFX_DENY); return; }
    // Por gusto, agarrado nao sai. O relevo obrigatorio sai sempre.
    if (pvpPonto == PT_TURNO && !bt.b.canSwitchOut(pvpEu)) { sfxPlay(SFX_DENY); return; }
    if (pvpPonto == PT_NADA) return;
    DueloDecisao d;
    d.tipo = DD_TROCA;
    d.arg = i;
    sfxPlay(SFX_TAP);
    bt.mustSwitch = false;
    if (pvpPonto == PT_TURNO) bt.phase = BP_CHOOSE;
    pvpDecide(pvpEu, d);
    return;
  }
  if (bt.mustSwitch) return;    // tem de mandar alguem
  if (lejosDe(x, y, BPTY_PAN_X0, BPTY_PAN_Y0, BPTY_PAN_X1, BPTY_PAN_Y1))
    bt.phase = BP_CHOOSE;
}

// BP_ESPERA no tick: so faz alguma coisa quando o combate acabou (a
// frase do resultado e depois fechar). Esperar a outra placa e so ficar.
static void pvpEsperaTick(uint32_t now) {
  if (!pvpAcabou || (int32_t)(now - bt.until) < 0) return;
  if (!pvpDisseFim) {
    pvpDisseFim = true;
    snprintf(bt.msg, sizeof(bt.msg), pvpVenceu == 0xFF ? "It's a draw!"
             : pvpVenceu == pvpEu ? "You won!" : "You lost...");
    musicStop();
    if (pvpVenceu == pvpEu) jinglePlay(JIN_WIN_TRAINER);
    else sfxPlay(SFX_BYE);
    bt.until = now + 2500;
    return;
  }
  bt.phase = BP_END;
  bt.until = now;
}

static void pvpFecha() {
  btRngDesliga();
  bt.open = false;
  bt.pvp = false;
  bt.pvpMega = false;
  bt.bg = -1;
  bt.mustSwitch = false;
  pvpPonto = PT_NADA;
  foePmd.unload();
  foePmdFor = -1;
  foePmdTried = -1;
  myPmdFor = -1;
  myPmdTried = -1;
  monFor = -2;          // o sprite de casa volta no proximo ensureMon
  bgImg.unload();
  pvpRede = false;
  pvpEnvia = nullptr;
  pvpAoDesistir = nullptr;
  pvpDesistirAte = 0;
  if (pvpAoAcabar) pvpAoAcabar();
}

// Os ultimos 10 s da minha escolha, contando no alto. Fora disso, nada:
// um relogio sempre a vista apressa quem nem esta perto do fim.
void btDrawRelogioPvp() {
  uint32_t r = pvpRestaMs();
  if (r > PVP_AVISO_MS) return;
  unsigned s = (unsigned)((r + 999) / 1000);
  const int cx = 233, cy = 40;
  gfx->fillCircle(cx, cy, 19, s <= 3 ? UI_BAR_BAD : UI_BAR_WARN);
  gfx->drawCircle(cx, cy, 19, INK_K);
  char t[4];
  snprintf(t, sizeof(t), "%u", s);
  // print direto: o printT recorta ao vidro redondo
  gfx->setTextColor(UI_WHITE);
  setSize(2);
  gfx->setCursor(cx - (int)strlen(t) * 6 + 1, cy - 7);
  gfx->print(t);
}

// Arranca o combate entre placas. r0 e o time do ANFITRIAO (lado 0) e r1
// o do convidado, nas duas placas; `eu` diz qual dos dois sou eu.
bool battleStartPvp(const uint8_t r0[][TROCA_MON], const uint8_t r1[][TROCA_MON],
                    uint8_t n, uint8_t eu, uint32_t semente, uint8_t regras) {
  if (bt.open || eu > 1) return false;
  if (!pvpMonta(pvpL[0], r0, n) || !pvpMonta(pvpL[1], r1, n)) return false;
  btRngSemente(semente);
  bt.pvp = true;
  bt.pvpMega = false;
  pvpEu = eu;
  pvpRegras = regras;
  pvpPonto = PT_NADA;
  pvpPrecisa = pvpTem = 0;
  pvpN = pvpI = 0;
  pvpAcabou = false;
  pvpDisseFim = false;
  pvpVenceu = 0xFF;
  pvpPontos = 0;
  pvpRede = pvpEnvia != nullptr;
  pvpRedeOk = false;
  pvpCortado = false;
  pvpDesistirAte = 0;
  pvpHashPonto = 0;
  memcpy(pvpBolsa, pet.bag, sizeof(pvpBolsa));   // a copia: o save fica
  bt.itemPick = 0;
  bt.rematch = false;
  bt.insigniaDada = false;
  bt.gym = -1;
  bt.trainer = -1;
  bt.gymTr = -1;
  bt.expWon = 0;
  bt.lvlWon = 0;
  bt.lvlAntes = 0;
  bt.winDex = 0;
  bt.winSlot = 0xFF;
  bt.foeIdx = 0;
  bt.foesLeft = 1;
  // O cenario sai da semente: o entorno (Camuflaje, Poder Secreto) tem de
  // ser o mesmo nas duas placas.
  bt.bg = (int8_t)bgPick(CITY_BGS, semente % 1000u);
  bgImg.load((uint8_t)bt.bg);
  pvpComeca(bt.b, pvpL);
  bt.b.fleeTries = 0;
  bt.b.fled = false;
  // Entre placas ninguem desobedece (decisao do dono, e assim no Anil:
  // batalha de link nao tira o dado da obediencia). O vinculo continua
  // valendo no resto (Retribucion, Frustracion).
  bt.b.strictOrders = true;
  bt.mySlot = 0;
  bt.partyScroll = 0;
  bt.mustSwitch = false;
  for (uint8_t i = 0; i < 6; i++) bt.itemIni[i] = bt.itemAhora[i] = 0;
  bt.itemGastado = 0;
  bt.foeItemIni = 0;
  bt.open = true;
  bt.phase = BP_INTRO;
  bt.until = millis() + BT_INTRO_MS;
  bt.startedAt = millis();
  bt.fxType = T_NONE;
  bt.fxSup = 0xFF;
  bt.fxMove = 0;
  bt.sel = 0;
  bt.introAbil = 0;
  bt.secondPending = false;
  bt.itemPick = 0;
  bt.megaSide = BT_PLAYER;
  for (uint8_t s = 0; s < 2; s++) {
    bt.hidWas[s] = bt.hidKind[s] = 0;
    bt.hidAt[s] = 0;
    bt.cargaAte[s] = 0;
    bt.cargaAnim[s] = false;
  }
  // A animacao do combate anterior nao entra neste: `animFim` segura TUDO
  // (o corte do Drought e da carga do Fly), e um valor velho congelava a
  // abertura do proximo combate ate ele passar.
  bt.anim = false;
  bt.animFim = 0;
  bt.foeInBall = false;
  bt.caught = false;
  bt.taps = 0;
  bt.wobble = 0;
  bt.ballId = 0;
  snprintf(bt.msg, sizeof(bt.msg), "Go, %s!", dexName(bt.b.side[pvpEu].pareceDex()));
  pvpCarregaMeu(bt.b.side[pvpEu]);
  foePmd.unload();
  foePmdFor = -1;
  foePmdTried = -1;
  musicPlay(MUS_TRAINER);
  cryPlay((uint16_t)bt.b.side[pvpEu ^ 1].dex);
  sfxPlay(SFX_TAP);
  return true;
}
