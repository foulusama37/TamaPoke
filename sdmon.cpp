#include <math.h>
#include "sdmon.h"
#include "female.h"   // hasFemale()
#include "pin_config.h"
#include <FS.h>
#include <SD_MMC.h>

uint16_t fxGolpesEsperados = 0;   // lo rellena el sketch con MOVE_COUNT
bool sdReady = false;
bool sdPrestada = false;
bool sdDirty = false;
SdThumbs thumbs;

// OJO con el tipo: esto era `uint8_t dexNum` y aguanto mientras la dex
// cabia en un byte. Desde Hoenn NO cabe, y el numero daba la vuelta en
// silencio: el 300 pedia /mons/p044.bin y salia otro bicho. No reventaba
// nada — habia un fichero valido en el otro extremo — y el barrido no lo
// veia porque el solo mira que los ficheros ESTEN en la tarjeta, no que
// el firmware sepa pedirlos.
bool PmdMon::load(int16_t dexNum, bool shiny, bool hembra) {
  // La HEMBRA primero, si esa especie trae dibujo propio. Si el fichero
  // no esta (una tarjeta a medio empaquetar), se cae al normal en vez de
  // quedarse sin bicho: un sprite que falta no puede dejar el combate
  // sin dibujo.
  if (hembra && hasFemale(dexNum) && loadFile(dexNum, shiny, -1, 'h'))
    return true;
  return loadFile(dexNum, shiny, -1, 'p');
}

// Una MEGA no tiene numero de dex: su fichero es mNNN.bin, donde NNN es
// la fila de MEGA_TBL. Comparte todo lo demas con un sprite normal, asi
// que comparte tambien el lector.
bool PmdMon::loadMega(int16_t fila, bool shiny) {
  return loadFile(0, shiny, fila, 'm');
}

// Una FORMA regional: /mons/vNNN.bin, NNN = la fila de FORM_TBL.
bool PmdMon::loadForm(int16_t fila) {
  return loadFile(0, false, fila, 'v');
}

bool PmdMon::loadBatalha(int16_t dex, uint8_t forma, bool shiny) {
  if (dex < 1) return false;
  char r[28];
  snprintf(r, sizeof(r), "/mons/c%s%04d_%u.bin", shiny ? "s" : "", dex,
           (unsigned)forma);
  if (loadFile(dex, false, -1, 'p', r)) return true;
  if (!shiny) return false;
  snprintf(r, sizeof(r), "/mons/c%04d_%u.bin", dex, (unsigned)forma);
  return loadFile(dex, false, -1, 'p', r);
}

bool PmdMon::loadAv(int16_t dex, bool shiny) {
  if (dex < 1) return false;
  char r[24];
  snprintf(r, sizeof(r), "/av/%c%04d.bin", shiny ? 's' : 'p', dex);
  if (loadFile(dex, false, -1, 'p', r)) return true;
  if (!shiny) return false;
  snprintf(r, sizeof(r), "/av/p%04d.bin", dex);    // sem shiny la: o normal
  return loadFile(dex, false, -1, 'p', r);
}

bool PmdMon::loadAvMega(int16_t base, uint8_t forma) {
  char r[24];
  snprintf(r, sizeof(r), "/av/m%d_%u.bin", base, (unsigned)forma);
  return loadFile(base, false, -1, 'p', r);
}

bool PmdMon::loadFile(int16_t dexNum, bool shiny, int16_t extra, char pref, const char *rota) {
  unload();
  if (!sdUsable()) return false;
  if (extra < 0 && dexNum < 1) return false;

  char path[28];
  if (extra >= 0)
    // So as megas tem shiny (msNNN.bin); as formas chegam com shiny=false.
    snprintf(path, sizeof(path), "/mons/%c%s%03d.bin", pref,
             shiny ? "s" : "", extra);
  else
    // El prefijo decide la variante: 'p' el normal, 'h' la hembra.
    snprintf(path, sizeof(path), "/mons/%c%s%03d.bin", pref,
             shiny ? "s" : "", dexNum);
  if (rota) snprintf(path, sizeof(path), "%s", rota);
  File f = SD_MMC.open(path, FILE_READ);
  if (!f && shiny && !rota) {  // sin shiny PMD: usa el normal
    if (extra >= 0)
      snprintf(path, sizeof(path), "/mons/%c%03d.bin", pref, extra);
    else
      snprintf(path, sizeof(path), "/mons/p%03d.bin", dexNum);
    f = SD_MMC.open(path, FILE_READ);
  }
  if (!f) return false;

  uint32_t size = f.size();
  if (size < 7 || size > 3UL * 1024 * 1024) { f.close(); return false; }
  blob = (uint8_t *)ps_malloc(size);
  if (!blob || f.read(blob, size) != size || memcmp(blob, "TPK2", 4) != 0) {
    if (blob) { free(blob); blob = nullptr; }
    f.close();
    return false;
  }
  f.close();

  uint8_t nActs = blob[4];
  memcpy(&palCount, blob + 5, 2);
  if (palCount > 256 || (uint32_t)7 + palCount * 2 > size) { unload(); return false; }
  memcpy(pal, blob + 7, palCount * 2);

  const uint8_t *p = blob + 7 + palCount * 2;
  const uint8_t *end = blob + size;
  for (uint8_t i = 0; i < nActs && p + 4 <= end; i++) {
    uint8_t id = p[0], w = p[1], h = p[2], nf = p[3];
    p += 4;
    // Ya no hay tope de frames: las duraciones se apuntan en el blob en vez de
    // copiarse a un array fijo (ver PmdAct en sdmon.h). El unico limite real
    // es nf <= 255 porque el campo del fichero es un u8, y el tamano total.
    if (id >= PMD_NACTS) { unload(); return false; }
    // valida que las duraciones y los frames caben en el blob (fichero truncado)
    uint32_t bytes = (uint32_t)nf * 2 + (uint32_t)w * h * nf;
    if (w == 0 || h == 0 || nf == 0 || p + bytes > end) { unload(); return false; }
    PmdAct &a = acts[id];
    a.w = w;
    a.h = h;
    a.frames = nf;
    a.msRaw = p;
    p += (uint32_t)nf * 2;
    a.data = p;
    p += (uint32_t)w * h * nf;
    // fila mas baja con contenido en cualquier frame: anclar por los pies
    uint8_t base = 1, bajo = 0;
    for (uint8_t f = 0; f < nf; f++) {
      const uint8_t *fr = a.data + (uint32_t)f * w * h;
      for (int r = h - 1; r >= 0; r--) {
        bool any = false;
        for (int c = 0; c < w && !any; c++)
          if (fr[r * w + c] != 0xFF) any = true;
        if (any) { if (r + 1 > base) { base = r + 1; bajo = f; } break; }
      }
    }
    a.base = base;

    // Y la mas ALTA, por el mismo criterio: con las dos sale el alto del
    // CUERPO (inkH), que es con lo que se da el tamano en pantalla. Se mira
    // en todos los frames, como `base`, para que el bicho no cambie de
    // tamano a mitad de la animacion.
    uint8_t top = base ? (uint8_t)(base - 1) : 0;
    for (uint8_t f = 0; f < nf; f++) {
      const uint8_t *fr = a.data + (uint32_t)f * w * h;
      for (int r = 0; r < h; r++) {
        bool any = false;
        for (int c = 0; c < w && !any; c++)
          if (fr[r * w + c] != 0xFF) any = true;
        if (any) { if (r < top) top = (uint8_t)r; break; }
      }
    }
    a.top = top;

    // E de lado a lado, tambem em todos os quadros.
    uint8_t esq = w, dir = 0;
    for (uint8_t f = 0; f < nf; f++) {
      const uint8_t *fr = a.data + (uint32_t)f * w * h;
      for (int r = 0; r < h; r++)
        for (int c = 0; c < w; c++)
          if (fr[r * w + c] != 0xFF) {
            if (c < esq) esq = (uint8_t)c;
            if (c + 1 > dir) dir = (uint8_t)(c + 1);
          }
    }
    a.inkL = dir > esq ? esq : 0;
    a.inkR = dir > esq ? dir : w;

    // El AREA de tinta del primer frame, y la magnitud que sale de
    // ella. Del PRIMER frame y no de todos: el generador la mide
    // igual, y lo que importa es que los dos midan LO MISMO.
    uint32_t tinta = 0;
    for (uint32_t i = 0; i < (uint32_t)w * h; i++)
      if (a.data[i] != 0xFF) tinta++;
    a.inkArea = (uint16_t)(tinta > 65535 ? 65535 : tinta);
    a.mag = powf((float)a.inkH(), 1.0f - PMD_B_AREA) *
            powf(sqrtf(tinta ? (float)tinta : 1.0f), PMD_B_AREA);
    if (!(a.mag > 0.0f)) a.mag = 1.0f;   // NaN o 0: no dividir por eso

    // Huella: el ancho del CUERPO en el frame que apoya, no el del lienzo ni
    // el del punto de contacto. El lienzo trae padding vacio (Aerodactyl: 93
    // px de lienzo para un bicho que no los ocupa) y el punto de contacto es
    // demasiado literal (Beedrill apoya en 6 px: una sombra de 6 px no se ve,
    // y sin sombra visible el bicho parece flotar todavia mas). Lo que ancla
    // a un personaje es una sombra del tamano de SU CUERPO.
    const uint8_t *fb = a.data + (uint32_t)bajo * w * h;
    int cL = w, cR = -1;
    for (int r = 0; r < h; r++) {
      for (int c = 0; c < w; c++) {
        if (fb[r * w + c] == 0xFF) continue;
        if (c < cL) cL = c;
        if (c > cR) cR = c;
      }
    }
    if (cR < 0) { cL = 0; cR = w ? w - 1 : 0; }   // por si acaso: lienzo entero
    a.footL = (uint8_t)cL;
    a.footR = (uint8_t)cR;

    // Y EL APOYO DE VERDAD: solo la fila que toca el suelo. El ancho de
    // arriba es el del cuerpo (a proposito: una sombra del tamano del
    // punto de contacto no se ve), pero el CENTRO tiene que ser este, o
    // un ala abierta se lleva la mancha lejos del pie. Lugia apoya en
    // las columnas 11..16 de 156 y la sombra se iba a la 78.
    int pL = w, pR = -1;
    if (base > 0 && base <= h) {
      const uint8_t *fila = fb + (uint32_t)(base - 1) * w;
      for (int c = 0; c < w; c++) {
        if (fila[c] == 0xFF) continue;
        if (c < pL) pL = c;
        if (c > pR) pR = c;
      }
    }
    if (pR < 0) { pL = cL; pR = cR; }   // sin fila de apoyo: el cuerpo
    a.pieL = (uint8_t)pL;
    a.pieR = (uint8_t)pR;
  }
  loaded = true;
  Serial.printf("cargado %s (%u KB)\n", path, size / 1024);
  return true;
}

void PmdMon::unload() {
  if (blob) {
    free(blob);
    blob = nullptr;
  }
  for (auto &a : acts) {
    a.w = a.h = a.frames = a.base = a.footL = a.footR = a.top = 0;
    a.inkL = a.inkR = 0;
    a.inkArea = 0;
    a.mag = 1.0f;
    // los dos apuntan DENTRO del blob que se acaba de liberar: dejarlos
    // colgando seria un puntero muerto esperando a que alguien lo siga
    a.data = a.msRaw = nullptr;
  }
  loaded = false;
}

void SdThumbs::unload() {
  if (data) { free(data); data = nullptr; }
  loaded = false;
  count = 0;
  size = 0;
}

bool SdThumbs::load() {
  unload();  // recargar sin fugar el blob anterior
  if (!sdUsable()) return false;
  File f = SD_MMC.open("/mons/thumbs.bin", FILE_READ);
  if (!f) {
    Serial.println("sin thumbs.bin (galeria sin miniaturas)");
    return false;
  }
  uint32_t sz = f.size();
  // acota el tamano: evita un ps_malloc absurdo con un archivo corrupto
  if (sz < 10 || sz > 2UL * 1024 * 1024) {
    Serial.println("thumbs.bin invalido (tamano)");
    f.close();
    return false;
  }
  data = (uint8_t *)ps_malloc(sz);
  if (!data || f.read(data, sz) != sz || memcmp(data, "TPTH", 4) != 0) {
    Serial.println("thumbs.bin invalido");
    if (data) { free(data); data = nullptr; }
    f.close();
    return false;
  }
  f.close();
  memcpy(&count, data + 4, 2);
  // la tabla de offsets (uno por especie) debe caber entera en lo leido
  if ((uint32_t)6 + 4UL * count > sz) {
    Serial.println("thumbs.bin invalido (tabla de offsets)");
    unload();
    return false;
  }
  size = sz;
  loaded = true;
  Serial.printf("miniaturas cargadas: %u (%u KB)\n", count, sz / 1024);
  return true;
}

const uint8_t *SdThumbs::get(int16_t dex) const {
  if (!loaded || dex < 1 || dex > count) return nullptr;
  uint32_t off;
  memcpy(&off, data + 6 + 4 * (dex - 1), 4);
  // el offset viene del fichero sin comprobar: sin esto, un thumbs.bin truncado
  // o corrupto hacia leer fuera de la reserva de PSRAM
  if (off > size - 3) return nullptr;  // la cabecera w,h,palCount debe caber (size >= 10 siempre)
  uint32_t need = 3 + (uint32_t)data[off + 2] * 2 + (uint32_t)data[off] * data[off + 1];
  if (need > size || off > size - need) return nullptr;  // y el blob entero tambien
  return data + off;
}

// ------------------------------------------- animacion propia de un golpe

SdFx fxMove;

bool sdFirmaGolpes(uint16_t n, const char *(*nome)(uint16_t)) {
  if (!n || !nome || !sdUsable()) return false;
  File f = SD_MMC.open("/snd/golpes.txt", FILE_READ);
  if (!f) return false;
  // Linha a linha, lida em blocos: "MOVE_COUNT N" e "<fila> <NOME> <fonte>".
  char ln[96], buf[256];
  size_t k = 0;
  int32_t declarado = -1;
  uint16_t filas = 0;
  bool ok = true, fimArq = false;
  while (ok && !fimArq) {
    int lidos = f.read((uint8_t *)buf, sizeof(buf));
    if (lidos <= 0) { fimArq = true; lidos = 1; buf[0] = 10; }   // fecha a ultima linha
    for (int b = 0; ok && b < lidos; b++) {
      char c = buf[b];
      if (c != 10) {                       // 10 = fim de linha
        if (c != 13 && k < sizeof(ln) - 1) ln[k++] = c;
        continue;
      }
      ln[k] = 0;
      k = 0;
      if (!ln[0] || ln[0] == '#') continue;
      if (!strncmp(ln, "MOVE_COUNT ", 11)) { declarado = atol(ln + 11); continue; }
      char *esp = strchr(ln, ' ');
      if (!esp) { ok = false; break; }
      long fila = atol(ln);
      char *nom = esp + 1;
      char *fin = strchr(nom, ' ');
      if (fin) *fin = 0;
      if (fila != filas || fila >= n || strcmp(nom, nome((uint16_t)fila))) {
        ok = false;
        break;
      }
      filas++;
    }
  }
  f.close();
  return ok && declarado == n && filas == n;
}

SdPor porMon;
SdTira insigniasSd("/por/insignias.bin", "TPIN");
SdTira fitasSd("/por/fitas.bin", "TPFT");

void SdTira::unload() {
  if (blob) free(blob);
  blob = nullptr;
  grupo = 0xFF;
  for (uint8_t i = 0; i < SDTIRA_MAX; i++) tem[i] = false;
}

bool SdTira::load(uint8_t g) {
  if (grupo == g && blob) return true;
  // Tentou e falhou ha pouco: nao rele o cartao a cada quadro.
  if (falhou == g && millis() - falhouEm < 5000) return false;
  unload();
  if (!sdUsable()) return false;
  File f = SD_MMC.open(path, FILE_READ);
  uint8_t cab[8];
  bool ok = f && f.read(cab, 8) == 8 && memcmp(cab, magia, 4) == 0 &&
            cab[4] == 1 && g >= cab[5] && g < cab[5] + cab[6] &&
            cab[7] && cab[7] <= SDTIRA_MAX;
  const uint8_t K = ok ? cab[7] : 0;
  uint32_t offs[SDTIRA_MAX] = { 0 };
  ok = ok && f.seek(8 + 4UL * K * (g - cab[5])) &&
       f.read((uint8_t *)offs, 4 * K) == (int)(4 * K);
  // Quanto cabe: cada cabecalho lido duas vezes (o tamanho, depois o
  // desenho). Sao no maximo 32 de 8 bytes: barato.
  uint32_t total = 0, tam[SDTIRA_MAX] = { 0 };
  for (uint8_t i = 0; ok && i < K; i++) {
    if (!offs[i]) continue;
    uint8_t e[8];
    uint16_t np, nr;
    if (!f.seek(offs[i]) || f.read(e, 8) != 8) { ok = false; break; }
    memcpy(&np, e + 4, 2);
    memcpy(&nr, e + 6, 2);
    // A ASSINATURA: a entrada diz de que grupo e de que indice e.
    if (e[0] != g || e[1] != i || !e[2] || !e[3] || np > 256 || !nr) { ok = false; break; }
    tam[i] = 2UL * np + nr;
    total += (tam[i] + 1) & ~1UL;
  }
  if (ok && total) blob = (uint8_t *)ps_malloc(total);
  ok = ok && blob;
  uint32_t p = 0;
  for (uint8_t i = 0; ok && i < K; i++) {
    if (!offs[i]) continue;
    uint8_t e[8];
    uint16_t np, nr;
    if (!f.seek(offs[i]) || f.read(e, 8) != 8 ||
        f.read(blob + p, tam[i]) != (int)tam[i]) { ok = false; break; }
    memcpy(&np, e + 4, 2);
    memcpy(&nr, e + 6, 2);
    arts[i].w = e[2];
    arts[i].h = e[3];
    arts[i].pal = (const uint16_t *)(blob + p);
    arts[i].rle = blob + p + 2UL * np;
    arts[i].rleLen = nr;
    tem[i] = true;
    p += (tam[i] + 1) & ~1UL;
  }
  if (f) f.close();
  if (!ok) {
    unload();
    falhou = g;
    falhouEm = millis();
    return false;
  }
  grupo = g;
  return true;
}
uint16_t porEspeciesEsperadas = 0;
uint8_t porRanurasEsperadas = 0;
uint8_t porVersionEsperada = 0;

void SdPor::unload() {
  if (blob) { free(blob); blob = nullptr; }
  dex = 0;
  mega = 0;
  shiny = false;
  n = 0;
  ranuras = 0;
}

// UM bloco de retratos (o de uma especie no por.bin, ou o de uma mega
// no seu arquivo), lido da posicao atual de `f`.
static bool porLeBloco(SdPor &p, File &f, uint8_t ran) {
  uint8_t *&blob = p.blob;
  uint8_t *mapa = p.mapa;
  Art *caras = p.caras;
  auto unload = [&]() { p.unload(); };
  uint8_t nu = 0;
  if (f.read(&nu, 1) != 1 || !nu || nu > SDPOR_MAX) { return false; }
  if (f.read(mapa, ran) != ran) { return false; }
  // o gerador enche ate par, pra paleta cair em endereco par
  if ((1 + ran) % 2) {
    uint8_t z;
    if (f.read(&z, 1) != 1) { return false; }
  }

  uint8_t cabs[SDPOR_MAX * 8];
  if (f.read(cabs, nu * 8) != nu * 8) { return false; }
  uint32_t bytesPal = 0, bytesRle = 0;
  for (uint8_t i = 0; i < nu; i++) {
    bytesPal += (uint32_t)cabs[i * 8 + 2] * 2;
    uint16_t rl;
    memcpy(&rl, cabs + i * 8 + 4, 2);
    bytesRle += rl;
  }
  uint32_t total = bytesPal + bytesRle;
  // Teto de sanidade: um cabecalho corrompido nao pode pedir meio mega.
  // Seis caras de 40x40 dao uns 9 KB, entao 128 KB e folga enorme.
  if (!total || total > 128UL * 1024) { return false; }

  blob = (uint8_t *)ps_malloc(total);
  if (!blob || f.read(blob, total) != (int)total) { unload(); return false; }

  // As paletas vao todas na frente (e por isso caem em endereco par, que
  // e o que deixa le-las como u16 sem copiar); os RLE atras.
  uint32_t po = 0, ro = bytesPal;
  for (uint8_t i = 0; i < nu; i++) {
    uint8_t w = cabs[i * 8], hh = cabs[i * 8 + 1], pn = cabs[i * 8 + 2];
    uint16_t rl;
    memcpy(&rl, cabs + i * 8 + 4, 2);
    caras[i].w = w;
    caras[i].h = hh;
    caras[i].pal = (const uint16_t *)(blob + po);
    caras[i].rle = blob + ro;
    caras[i].rleLen = rl;
    po += (uint32_t)pn * 2;
    ro += rl;
  }
  p.n = nu;
  p.ranuras = ran;
  return true;
}

bool SdPor::load(uint16_t d, bool sh) {
  if (dex == d && shiny == sh && !mega && blob) return true;   // ja e esse
  unload();
  if (!d || !sdUsable()) return false;

  File f = SD_MMC.open("/por/por.bin", FILE_READ);
  if (!f) return false;

  uint8_t cab[8];
  if (f.read(cab, 8) != 8 || memcmp(cab, "TPPR", 4) != 0) { f.close(); return false; }
  uint16_t cuenta;
  memcpy(&cuenta, cab + 4, 2);
  uint8_t ran = cab[6];
  uint8_t ver = cab[7];
  // A FIRMA, exata. Recusar o arquivo inteiro e a falha BOA: fica-se
  // sem retrato, que se nota e se conserta, em vez de mostrar a cara de
  // outro bicho, que nao se nota nunca.
  if ((porEspeciesEsperadas && cuenta != porEspeciesEsperadas) ||
      (porRanurasEsperadas && ran != porRanurasEsperadas) ||
      (porVersionEsperada && ver != porVersionEsperada)) {
    static bool dicho = false;
    if (!dicho) {
      dicho = true;
      Serial.printf("/por/por.bin e v%u de %u especies x %u estados e aqui "
                    "sao v%u de %u x %u: sem retratos ate recopiar\n",
                    (unsigned)ver, (unsigned)cuenta, (unsigned)ran,
                    (unsigned)porVersionEsperada,
                    (unsigned)porEspeciesEsperadas,
                    (unsigned)porRanurasEsperadas);
    }
    f.close();
    return false;
  }
  if (d > cuenta || !ran || ran > SDPOR_MAX) { f.close(); return false; }

  // Dois deslocamentos por especie: normal e shiny, nesta ordem.
  uint32_t off = 0;
  uint32_t ranura = (uint32_t)d * 2 + (sh ? 1 : 0);
  if (!f.seek(8 + 4UL * ranura) || f.read((uint8_t *)&off, 4) != 4) {
    f.close();
    return false;
  }
  // Sem retrato shiny proprio (duas especies das 1025), cai no normal:
  // melhor a cara certa na cor errada do que cara nenhuma.
  if (!off && sh) {
    if (!f.seek(8 + 4UL * ((uint32_t)d * 2)) ||
        f.read((uint8_t *)&off, 4) != 4) {
      f.close();
      return false;
    }
    sh = false;
  }
  if (!off) { f.close(); return false; }   // esse bicho nao tem retrato

  if (!f.seek(off)) { f.close(); return false; }
  bool ok = porLeBloco(*this, f, ran);
  f.close();
  if (!ok) { unload(); return false; }
  dex = d;
  shiny = sh;
  mega = 0;
  return true;
}

bool SdPor::loadMega(uint16_t base, uint8_t forma, bool sh) {
  if (dex == base && mega == forma && shiny == sh && blob) return true;   // ja e esse
  unload();
  if (!base || !forma || !sdUsable()) return false;
  char path[24];
  snprintf(path, sizeof(path), "/por/m%03u_%u.bin", (unsigned)base, (unsigned)forma);
  File f = SD_MMC.open(path, FILE_READ);
  if (!f) return false;
  uint8_t cab[18];
  uint16_t d;
  uint32_t offs[2];
  bool ok = f.read(cab, 18) == 18 && memcmp(cab, "TPPM", 4) == 0 && cab[4] == 1;
  memcpy(&d, cab + 6, 2);
  memcpy(offs, cab + 10, 8);
  uint32_t off = offs[sh ? 1 : 0];
  // o arquivo tem de ser DESSA mega, e com os estados que o firmware conhece
  ok = ok && cab[5] == forma && d == base && cab[8] && off &&
       (!porRanurasEsperadas || cab[8] == porRanurasEsperadas) &&
       f.seek(off) && porLeBloco(*this, f, cab[8]);
  f.close();
  if (!ok) { unload(); return false; }
  dex = base;
  shiny = sh;
  mega = forma;
  return true;
}

void SdFx::unload() {
  if (blob) { free(blob); blob = nullptr; }
  move = 0xFFFF;
  n = 0;
  s = 1;
}

SdAnim animMove;

void SdAnim::unload() {
  if (blob) free(blob);
  blob = nullptr;
  tam = 0;
  nq = 0;
  ni = 0;
  livre = 0;
  hit = 0;
  move = 0xFFFF;
}

bool SdAnim::load(uint16_t mv, const char *nome, bool deRival, uint8_t hit_) {
  if (blob && move == mv && rival == deRival && hit == hit_) return true;   // ja e essa
  if (!sdUsable() || !nome || !nome[0]) { unload(); return false; }
  // O nome que o arquivo tem de trazer: "<GOLPE>" ou "<GOLPE>.<hit>".
  char quer[40];
  if (hit_) snprintf(quer, sizeof(quer), "%s.%u", nome, (unsigned)hit_);
  else snprintf(quer, sizeof(quer), "%s", nome);
  nome = quer;
  char path[56];
  snprintf(path, sizeof(path), "/anim/%c_%s.bin", deRival ? 'f' : 'p', nome);
  // A maioria dos golpes nao tem variante (o "hit 2" do Anil e copia do
  // 1): sem o arquivo, a carregada fica, e quem chamou volta a de sempre
  // sem reler nada do SD.
  if (hit_ && !SD_MMC.exists(path)) return false;
  unload();
  File f = SD_MMC.open(path, FILE_READ);
  if (!f) return false;
  uint32_t sz = (uint32_t)f.size();
  if (sz < 16 || sz > SDANIM_MAX_B) { f.close(); return false; }
  uint8_t *b = (uint8_t *)ps_malloc(sz);
  if (!b) { f.close(); return false; }
  bool lido = (uint32_t)f.read(b, sz) == sz;
  f.close();
  if (!lido) { free(b); return false; }

  // Tudo conferido contra o tamanho ANTES de aceitar: um arquivo cortado
  // no meio de uma copia nao pode mandar o desenho ler fora do blob.
  uint32_t p = 0;
  auto falha = [&]() { free(b); return false; };
  if (memcmp(b, "TPAN", 4) != 0 || b[4] != 1) return falha();
  if (b[5] != (deRival ? 'f' : 'p')) return falha();
  uint8_t n = b[6];
  if (7u + n + 4 > sz || strlen(nome) != n || memcmp(b + 7, nome, n) != 0)
    return falha();                 // o arquivo e de OUTRO golpe
  p = 7 + n;
  uint16_t q;
  memcpy(&q, b + p, 2);
  uint8_t ni_ = b[p + 2];
  uint8_t livre_ = b[p + 3];
  p += 4;
  if (!q || q > SDANIM_MAX_Q || ni_ > SDANIM_MAX_IMG || livre_ > 3) return falha();
  for (uint8_t i = 0; i < ni_; i++) {
    if (p + 6 > sz) return falha();
    img[i] = p;
    uint32_t w = b[p], h = b[p + 1], np = b[p + 4];
    if (!w || !h || !np) return falha();
    p += 6 + np * 3 + w * h;
    if (p > sz) return falha();
  }
  for (uint16_t i = 0; i < q; i++) {
    if (p + 9 > sz) return falha();
    quad[i] = p;
    p += 9 + (uint32_t)b[p] * SDANIM_CEL_B;
    if (p > sz) return falha();
  }
  if (p != sz) return falha();      // sobrou ou faltou: nao e TPAN v1
  blob = b;
  tam = sz;
  nq = q;
  ni = ni_;
  livre = livre_;
  move = mv;
  rival = deRival;
  hit = hit_;
  return true;
}

SdClima animClima;

void SdClima::unload() {
  if (blob) free(blob);
  blob = nullptr;
  nq = 0;
  ni = 0;
  fig = 0;
  clima = 0;
}

bool SdClima::load(uint8_t qual) {
  if (blob && clima == qual) return true;    // ja e esse
  unload();
  if (!sdUsable() || !qual) return false;
  char path[32];
  snprintf(path, sizeof(path), "/anim/w_%u.bin", (unsigned)qual);
  File f = SD_MMC.open(path, FILE_READ);
  if (!f) return false;
  uint32_t sz = (uint32_t)f.size();
  if (sz < 16 || sz > SDCLIMA_MAX_B) { f.close(); return false; }
  uint8_t *b = (uint8_t *)ps_malloc(sz);
  if (!b) { f.close(); return false; }
  bool lido = (uint32_t)f.read(b, sz) == sz;
  f.close();
  if (!lido) { free(b); return false; }

  // Tudo conferido contra o tamanho ANTES de aceitar, como no SdAnim.
  auto falha = [&]() { free(b); return false; };
  if (memcmp(b, "TPCL", 4) != 0 || b[4] != 1) return falha();
  if (b[5] != qual) return falha();          // o arquivo e de OUTRO clima
  uint16_t q;
  memcpy(&q, b + 6, 2);
  uint8_t ni_ = b[8], tem = b[9];
  uint32_t p = 10;
  if (!q || q > SDCLIMA_MAX_Q || ni_ > SDCLIMA_MAX_IMG || tem > 1) return falha();
  for (uint8_t i = 0; i < ni_; i++) {
    if (p + 6 > sz) return falha();
    img[i] = p;
    uint32_t w = b[p], h = b[p + 1], np = b[p + 4];
    if (!w || !h || !np) return falha();
    p += 6 + np * 3 + w * h;
    if (p > sz) return falha();
  }
  uint32_t fig_ = 0;
  if (tem) {
    if (p + 6 > sz) return falha();
    uint16_t w, h;
    memcpy(&w, b + p, 2);
    memcpy(&h, b + p + 2, 2);
    uint32_t np = b[p + 4];
    if (!w || !h || !np) return falha();
    fig_ = p;
    p += 6 + np * 3 + (uint32_t)w * h;
    if (p > sz) return falha();
  }
  for (uint16_t i = 0; i < q; i++) {
    if (p + SDCLIMA_QUAD_B > sz) return falha();
    quad[i] = p;
    p += SDCLIMA_QUAD_B + (uint32_t)b[p] * SDANIM_CEL_B;
    if (p > sz) return falha();
  }
  if (p != sz) return falha();               // sobrou ou faltou: nao e TPCL v1
  blob = b;
  nq = q;
  ni = ni_;
  fig = fig_;
  clima = qual;
  return true;
}

bool SdFx::load(uint16_t mv) {
  if (move == mv && blob) return true;    // ya es ese: no se relee
  unload();
  if (!sdUsable()) return false;

  File f = SD_MMC.open("/fx/fx.bin", FILE_READ);
  if (!f) return false;

  uint8_t cab[8];
  uint16_t cuenta;
  if (f.read(cab, 8) != 8 || memcmp(cab, "TPFX", 4) != 0) { f.close(); return false; }
  memcpy(&cuenta, cab + 4, 2);
  // La cuenta tiene que ser EXACTAMENTE la de la tabla embarcada, no
  // solo "mayor que el indice". fx.bin se indexa por numero de golpe, y
  // ese numero sale del orden alfabetico de la tabla: al anadir dos
  // golpes (Transformacion y Esquema) TODOS los de despues se corrieron
  // una casilla. Con la tarjeta vieja, Latigo Cepa sacaba la animacion
  // de otro golpe -- un puñetazo -- y nada lo decia.
  //
  // Rechazarla entera es el fallo BUENO: cada golpe se cae a la
  // animacion de su tipo, que es generica pero correcta, en vez de
  // ensenar la de otro. Y el aviso dice que hay que recopiar el fichero.
  if (fxGolpesEsperados && cuenta != fxGolpesEsperados) {
    static bool dicho = false;
    if (!dicho) {
      dicho = true;
      Serial.printf("/fx/fx.bin es de %u golpes y la tabla tiene %u: "
                    "animaciones por tipo hasta que se recopie\n",
                    (unsigned)cuenta, (unsigned)fxGolpesEsperados);
    }
    f.close();
    return false;
  }
  if (mv >= cuenta) { f.close(); return false; }

  uint32_t off = 0;
  if (!f.seek(8 + 4UL * mv) || f.read((uint8_t *)&off, 4) != 4) { f.close(); return false; }
  if (!off) { f.close(); return false; }   // ese golpe no tiene animacion

  // La cabecera del registro dice cuanto ocupa todo: se lee de una.
  uint8_t h[4];
  if (!f.seek(off) || f.read(h, 4) != 4) { f.close(); return false; }
  uint8_t nf = h[0], esc = h[1];
  if (!nf || nf > SDFX_MAX_FRAMES) { f.close(); return false; }

  uint8_t cabs[SDFX_MAX_FRAMES * 8];
  if (f.read(cabs, nf * 8) != nf * 8) { f.close(); return false; }
  uint32_t bytesPal = 0, bytesRle = 0;
  for (uint8_t i = 0; i < nf; i++) {
    bytesPal += (uint32_t)cabs[i * 8 + 2] * 2;
    uint16_t rl;
    memcpy(&rl, cabs + i * 8 + 4, 2);
    bytesRle += rl;
  }
  uint32_t total = bytesPal + bytesRle;
  // Tope de cordura: una cabecera corrupta no puede pedir medio mega.
  // NO es el limite de verdad de lo que cabe —— el blob va a PSRAM, que
  // son 8 MB ——, asi que lo unico que decide cuanto puede pesar una
  // animacion es el TIEMPO DE LECTURA de la tarjeta (ver
  // FX_MOVE_MAX_B en tools/gen_art.py).
  //
  // Estaba en 64 KB, por debajo del tope del generador, y eso es una
  // trampa: el generador daba por embarcada una hoja que la placa
  // rechazaba al leerla, y el golpe se caia al efecto de su tipo sin
  // que nada lo dijera. Se deja CON MARGEN sobre el del generador (128
  // KB) para que el que manda sea uno solo y sea el de alla.
  if (!total || total > 192UL * 1024) { f.close(); return false; }

  blob = (uint8_t *)ps_malloc(total);
  if (!blob || f.read(blob, total) != (int)total) { unload(); f.close(); return false; }
  f.close();

  // Las paletas van todas delante (y por eso caen en direccion par, que es
  // lo que permite leerlas como u16 sin copiarlas); los RLE detras.
  uint32_t po = 0, ro = bytesPal;
  for (uint8_t i = 0; i < nf; i++) {
    uint8_t w = cabs[i * 8], hh = cabs[i * 8 + 1], pn = cabs[i * 8 + 2];
    uint16_t rl;
    memcpy(&rl, cabs + i * 8 + 4, 2);
    has[i] = (w != 0 && hh != 0 && rl != 0);
    frames[i].w = w;
    frames[i].h = hh;
    frames[i].pal = (const uint16_t *)(blob + po);
    frames[i].rle = blob + ro;
    frames[i].rleLen = rl;
    po += (uint32_t)pn * 2;
    ro += rl;
  }
  n = nf;
  s = esc ? esc : 1;
  move = mv;
  return true;
}

// ---------------------------------------------------------- fondo de combate

SdBg bgImg;
SdBg mapImg;   // el mapa de region que se esta mirando, si hay alguno

void SdBg::unload() {
  if (pix) { free(pix); pix = nullptr; }
  loaded = false;
  id = -1;
  w = h = 0;
}

bool SdBg::load(uint8_t n, const char *dir) {
  if (loaded && id == (int16_t)n) return true;   // ya es ese: no se relee
  unload();
  if (!sdUsable()) return false;

  char path[24];
  snprintf(path, sizeof(path), "/%s/%02u.bin", dir, n);
  File f = SD_MMC.open(path, FILE_READ);
  if (!f) return false;                          // sin fondo se pinta el cielo

  uint8_t cab[8];
  if (f.read(cab, 8) != 8 || memcmp(cab, "TPKB", 4) != 0) { f.close(); return false; }
  uint16_t fw, fh;
  memcpy(&fw, cab + 4, 2);
  memcpy(&fh, cab + 6, 2);
  // acota: un fichero corrupto no puede pedir una reserva absurda
  if (fw == 0 || fh == 0 || fw > 640 || fh > 640) { f.close(); return false; }
  uint32_t bytes = (uint32_t)fw * fh * 2;
  if (f.size() < 8 + bytes) { f.close(); return false; }

  pix = (uint16_t *)ps_malloc(bytes);
  if (!pix) { Serial.println("sin PSRAM para el fondo"); f.close(); return false; }
  // Por trozos y no de una: 434 KB de golpe se atraganta en algunas tarjetas.
  uint32_t leido = 0;
  while (leido < bytes) {
    uint32_t trozo = bytes - leido;
    if (trozo > 32768) trozo = 32768;
    if (f.read((uint8_t *)pix + leido, trozo) != (int)trozo) {
      unload();
      f.close();
      return false;
    }
    leido += trozo;
  }
  f.close();

  w = fw;
  h = fh;
  id = (int16_t)n;
  loaded = true;
  return true;
}

bool sdBegin() {
  SD_MMC.setPins(SDMMC_CLK, SDMMC_CMD, SDMMC_DATA);
  // NUNCA formatar sozinho: o cartao guarda as caixas e as regioes. Uma
  // leitura ruim num boot apagaria tudo. Sem montar, fica sem SD e avisa.
  sdReady = SD_MMC.begin("/sdcard", true /* modo 1-bit */, false /* NAO formata */);
  if (sdReady) {
    Serial.printf("SD montada: %llu MB\n", SD_MMC.cardSize() / (1024ULL * 1024ULL));
    SD_MMC.mkdir("/mons");
    SD_MMC.mkdir("/bg");
  } else {
    Serial.println("SD no detectada (el juego usa los sprites de flash)");
  }
  return sdReady;
}

// int16_t por lo mismo que PmdMon::load: con mas de 255 especies un byte
// da la vuelta y se carga el sprite de otro bicho sin avisar.
bool SdMon::load(int16_t dexNum, bool shiny) {
  unload();
  if (!sdUsable() || dexNum < 1) return false;

  char path[24];
  snprintf(path, sizeof(path), "/mons/%s%03d.bin", shiny ? "s" : "", dexNum);
  File f = SD_MMC.open(path, FILE_READ);
  if (!f && shiny) {  // sin variante shiny: usa la normal
    snprintf(path, sizeof(path), "/mons/%03d.bin", dexNum);
    f = SD_MMC.open(path, FILE_READ);
  }
  if (!f) {
    Serial.printf("no existe %s\n", path);
    return false;
  }

  char magic[4];
  uint16_t header[4];
  if (f.read((uint8_t *)magic, 4) != 4 || memcmp(magic, "TPK1", 4) != 0 ||
      f.read((uint8_t *)header, 8) != 8) {
    f.close();
    return false;
  }
  w = header[0];
  h = header[1];
  frames = header[2];
  frameMs = header[3];
  // acota dimensiones: evita size desbordado o absurdo con archivo corrupto
  if (f.read((uint8_t *)&palCount, 2) != 2 || palCount > 256 ||
      w == 0 || w > 256 || h == 0 || h > 256 || frames == 0 || frames > 64) {
    f.close();
    return false;
  }
  if (f.read((uint8_t *)pal, palCount * 2) != palCount * 2) {
    f.close();
    return false;
  }

  uint32_t size = (uint32_t)w * h * frames;
  data = (uint8_t *)ps_malloc(size);
  if (!data) {
    Serial.println("sin PSRAM para el sprite");
    f.close();
    return false;
  }
  uint32_t got = f.read(data, size);
  f.close();
  if (got != size) {
    Serial.printf("%s truncado (%u de %u)\n", path, got, size);
    unload();
    return false;
  }

  // zoom entero para que el bicho mida ~200 px de alto en pantalla
  scale = 200 / h;
  if (scale < 2) scale = 2;
  if (scale > 5) scale = 5;

  Serial.printf("cargado %s: %ux%u x%u frames @%ums, escala %u\n",
                path, w, h, frames, frameMs, scale);
  loaded = true;
  return true;
}

void SdMon::unload() {
  if (data) {
    free(data);
    data = nullptr;
  }
  loaded = false;
}

// ---------------------------------------------------------------------------
// Protocolo de carga por USB (para llenar la SD sin sacarla de la placa):
//   PUT <ruta> <bytes>\n  + datos crudos   -> "OK" ... "DONE"
//   LS\n                                   -> listado de /mons
// Usar con tools/send_sd.py
// ---------------------------------------------------------------------------

bool sdSerialCommand(const String &line) {
  if (line.startsWith("PUT ")) {
    int sp = line.lastIndexOf(' ');
    String path = line.substring(4, sp);
    uint32_t size = line.substring(sp + 1).toInt();
    // 64 MB e nao 4: o /fx/fx.bin tem 11,6 MB e o /por/por.bin 8,4,
    // e com o teto antigo os dois davam ERR —— a transferencia por
    // serie nao podia leva-los, e so se percebia porque a placa
    // respondia ERR sem dizer por que.
    if (!sdUsable() || size == 0 || size > 64UL * 1024 * 1024) {
      Serial.println("ERR");
      return true;
    }
    if (!path.startsWith("/")) path = "/" + path;
    // acota la escritura a /mons/ y /bg/: la ruta llega tal cual de la linea
    // serie, sin sanear, asi que un PUT manipulado podria escribir en cualquier
    // sitio de la tarjeta. Los clientes reales (tools/send_sd.py y
    // web/index.html) ya mandan nombres con uno de esos dos prefijos.
    //
    // /fx/ e /por/ entraram depois: sao as outras duas pastas que o
    // gerador escreve (as animacoes de golpe e os retratos). Sem elas
    // na lista, a unica forma de poe-las no cartao era emprestar o
    // cartao ao PC, que e justamente o caminho que o dono pediu para
    // nao usar. /map/ faltava: a placa nova (2026-11-13) ficou sem os mapas
    // das regioes, que renderMap le de /map/NN.bin (SdBg com dir "map").
    if ((!path.startsWith("/mons/") && !path.startsWith("/bg/") &&
         !path.startsWith("/fx/") && !path.startsWith("/por/") &&
         !path.startsWith("/anim/") && !path.startsWith("/snd/") &&
         !path.startsWith("/av/") && !path.startsWith("/map/")) ||
        path.indexOf("..") >= 0) {
      Serial.println("ERR");
      return true;
    }
    // FILE_WRITE ANADE al final si el fichero ya existe, asi que reintentar uno
    // que quedo a medias lo alargaba en vez de reemplazarlo: quedaba un sprite
    // corrupto y mas grande que el original. Importa mas desde que el instalador
    // reanuda transferencias cortadas, porque reintenta justo los que fallaron.
    // A PASTA, se ainda nao existir. Vai depois da lista branca de
    // proposito: so se cria o que a lista ja permitiu.
    //
    // Sem isto o PUT nunca conseguia inaugurar um diretorio —— o FAT
    // nao cria pasta intermediaria sozinho e o open() falhava. Nao se
    // notava porque /mons e /bg estavam no cartao desde sempre; apareceu
    // com /por, que era novo, e a placa so respondia ERR.
    int barra = path.lastIndexOf('/');
    if (barra > 0) {
      String dir = path.substring(0, barra);
      if (!SD_MMC.exists(dir)) SD_MMC.mkdir(dir);
    }
    if (SD_MMC.exists(path)) SD_MMC.remove(path);
    File f = SD_MMC.open(path, FILE_WRITE);
    if (!f) {
      Serial.println("ERR");
      return true;
    }
    Serial.println("OK");
    static uint8_t buf[2048];
    uint32_t remaining = size;
    Serial.setTimeout(5000);
    while (remaining > 0) {
      size_t want = remaining > sizeof(buf) ? sizeof(buf) : remaining;
      size_t n = Serial.readBytes(buf, want);
      if (n == 0) break;  // timeout
      // sin mirar el retorno, una tarjeta llena o con fallo de escritura daba
      // DONE igualmente y dejaba el fichero truncado en la SD: justo la entrada
      // corrupta contra la que hay que protegerse luego al cargar el sprite
      if (f.write(buf, n) != n) break;
      remaining -= n;
      Serial.println("#");  // ack: listo para el siguiente bloque
    }
    f.close();
    Serial.setTimeout(1000);
    sdDirty = (remaining == 0);
    Serial.println(remaining == 0 ? "DONE" : "ERR");
    return true;
  } else if (line == "SDINFO") {  // diagnostico remoto de "no me reconoce la SD"
    // Sin esta guarda, SDINFO y LS leian la tarjeta aunque estuviera
    // prestada al PC. Son ordenes de depuracion, pero llegan por el
    // MISMO cable USB que en ese momento la esta escribiendo.
    if (!sdUsable()) {
      Serial.println(sdPrestada ? "PRESTADA" : "ERR");
      return true;
    }
    Serial.printf("total=%llu used=%llu\n", SD_MMC.totalBytes(), SD_MMC.usedBytes());
    Serial.println("DONE");
    return true;
  } else if (line.startsWith("SUMS ")) {
    // Nome, tamanho e CRC32 de cada ficheiro de UMA pasta do cartao. So
    // le. Existe para comparar o cartao da placa com tools/sdcard sem o
    // emprestar ao PC: o `LS` so via /mons e so o tamanho, e um som de
    // golpe da tabela velha tem o mesmo tamanho que o novo muitas vezes.
    if (!sdUsable()) {
      Serial.println(sdPrestada ? "PRESTADA" : "ERR");
      return true;
    }
    String dirp = line.substring(5);
    dirp.trim();
    if (!dirp.startsWith("/")) dirp = "/" + dirp;
    File dir = SD_MMC.open(dirp);
    if (dir) {
      static uint8_t buf[4096];
      File e;
      while ((e = dir.openNextFile())) {
        uint32_t crc = 0xFFFFFFFFu;
        if (!e.isDirectory()) {
          int n;
          while ((n = e.read(buf, sizeof(buf))) > 0)
            for (int k = 0; k < n; k++) {
              crc ^= buf[k];
              for (int b = 0; b < 8; b++) crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1)));
            }
        }
        Serial.printf("%s %u %08x\n", e.name(), (uint32_t)e.size(), (unsigned)~crc);
        e.close();
      }
      dir.close();
    }
    Serial.println("DONE");
    return true;
  } else if (line == "LS") {
    if (!sdUsable()) {
      Serial.println(sdPrestada ? "PRESTADA" : "ERR");
      return true;
    }
    File dir = SD_MMC.open("/mons");
    if (dir) {
      File e;
      while ((e = dir.openNextFile())) {
        Serial.printf("%s %u\n", e.name(), (uint32_t)e.size());
        e.close();
      }
      dir.close();
    }
    Serial.println("DONE");
    return true;
  }
  return false;
}
