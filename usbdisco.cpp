#include "usbdisco.h"
#include "sdmon.h"
#include <FS.h>
#include <SD_MMC.h>

// Esto solo existe con el USB en modo TinyUSB (USBMode=default en el
// FQBN). Con "Hardware CDC and JTAG" el nucleo ni compila USBMSC.h, asi
// que el aviso sale aqui y no en forma de cuarenta errores del linker.
#if !defined(ARDUINO_USB_MODE) || ARDUINO_USB_MODE
#error "El MSC necesita USBMode=default (TinyUSB) en el FQBN. Ver CLAUDE.md."
#endif

#include <USB.h>
#include <USBMSC.h>

// GLOBAL a proposito: su constructor registra la interfaz MSC en el
// descriptor USB, y eso tiene que ocurrir antes de que app_main() llame
// a USB.begin(). Ver el comentario largo de usbdisco.h.
static USBMSC msc;

static bool listo = false;  // usbMscBegin() cuajo

// volatile porque estas tres cruzan de tarea: las escriben los callbacks
// de TinyUSB (su propia tarea) y las lee el bucle principal. Sin esto el
// compilador puede cachear "expulsada" en un registro y el bucle no ver
// nunca que el PC expulso la unidad.
static volatile bool prestada = false;   // el PC la tiene ahora
static volatile bool expulsada = false;  // el PC hizo "expulsar"
static volatile uint32_t escritos = 0;   // sectores que escribio el PC

// 512 y no un VLA con SD_MMC.sectorSize(): toda tarjeta SD usa 512, y un
// buffer de pila de tamano variable dentro de un callback de USB es
// justo donde no quiero una sorpresa. Si algun dia no fuera 512,
// usbMscBegin() se niega a arrancar en vez de corromper la tarjeta.
static const uint16_t SECTOR = 512;

static int32_t cbLeer(uint32_t lba, uint32_t offset, void *buffer, uint32_t bufsize) {
  (void)offset;
  if (!prestada) return -1;  // la tarjeta no esta prestada: no hay nada que leer
  for (uint32_t i = 0; i < bufsize / SECTOR; i++) {
    if (!SD_MMC.readRAW((uint8_t *)buffer + i * SECTOR, lba + i)) return -1;
  }
  return bufsize;
}

static int32_t cbEscribir(uint32_t lba, uint32_t offset, uint8_t *buffer, uint32_t bufsize) {
  (void)offset;
  if (!prestada) return -1;
  for (uint32_t i = 0; i < bufsize / SECTOR; i++) {
    // Copia a un buffer propio, como el ejemplo oficial
    // (SD_MMC/examples/SD2USBMSC): el DMA del SDMMC quiere el origen
    // alineado a palabra, y el buffer que llega de TinyUSB no lo
    // garantiza en un offset cualquiera. El aligned(4) es por lo mismo:
    // un array de uint8_t solo garantiza alineacion de 1.
    static_assert(SECTOR % 4 == 0, "el sector tiene que ser multiplo de 4");
    uint8_t bloque[SECTOR] __attribute__((aligned(4)));
    memcpy(bloque, buffer + i * SECTOR, SECTOR);
    if (!SD_MMC.writeRAW(bloque, lba + i)) return -1;
    escritos++;
  }
  return bufsize;
}

// El PC avisa de que expulso la unidad. Es la unica senal fiable de "ya
// he terminado y he vaciado lo que tenia en cache".
static bool cbArrancarParar(uint8_t power_condition, bool start, bool load_eject) {
  (void)power_condition;
  if (load_eject && !start) expulsada = true;
  return true;
}

bool usbMscBegin() {
  if (listo) return true;
  if (!sdReady) return false;
  if (SD_MMC.sectorSize() != SECTOR) {
    Serial.printf("USB-MSC: sector de %u bytes, esperaba %u -- desactivado\n",
                  (unsigned)SD_MMC.sectorSize(), (unsigned)SECTOR);
    return false;
  }
  msc.vendorID("TamaPoke");   // 8 caracteres como mucho
  msc.productID("microSD");   // 16
  msc.productRevision("1.0");  // 4
  msc.onRead(cbLeer);
  msc.onWrite(cbEscribir);
  msc.onStartStop(cbArrancarParar);
  msc.isWritable(true);
  msc.mediaPresent(false);  // lector vacio hasta que se preste
  listo = msc.begin(SD_MMC.numSectors(), SECTOR);
  // Idempotente (ESPUSB::begin se protege con _started), asi que llamarlo
  // aqui no molesta a lo que ya hizo app_main().
  USB.begin();
  Serial.printf("USB-MSC: %s (%u sectores, %llu MB)\n", listo ? "listo" : "FALLO",
                (unsigned)SD_MMC.numSectors(),
                SD_MMC.cardSize() / (1024ULL * 1024ULL));
  return listo;
}

bool usbMscPrestar(bool prestar) {
  if (!listo) return false;
  if (prestar == prestada) return true;
  if (prestar) {
    // El orden importa: primero se le cierra la puerta al firmware y
    // solo DESPUES aparece la unidad en el PC. Al reves habria una
    // ventana, corta pero real, en la que los dos escriben.
    sdPrestada = true;
    escritos = 0;
    expulsada = false;
    msc.mediaPresent(true);
  } else {
    msc.mediaPresent(false);
    sdPrestada = false;
    // Lo que hubiera leido de la tarjeta puede haber cambiado debajo.
    if (escritos) sdDirty = true;
  }
  prestada = prestar;
  return true;
}

bool usbMscPrestada() { return prestada; }
uint32_t usbMscEscritos() { return escritos; }
bool usbMscExpulsada() { return expulsada; }

// ---------------------------------------------------------------------
// Consola. Llega por el MISMO cable USB-C, asi que todo esto funciona
// con la pokebola cerrada:
//   USB       - en que estado esta
//   USB ON    - presta la tarjeta al PC (aparece la unidad)
//   USB OFF   - la recupera
// Lo normal es ni escribir "USB OFF": basta con expulsar la unidad en el
// PC, que es ademas lo unico que garantiza que vacio su cache. El bucle
// principal ve usbMscExpulsada() y la recupera solo.
// ---------------------------------------------------------------------
bool usbMscSerialCommand(const String &line) {
  if (line != "USB" && line != "USB ON" && line != "USB OFF") return false;
  if (!listo) {
    Serial.println(sdReady ? "USB-MSC no arranco" : "sin tarjeta");
    Serial.println("DONE");
    return true;
  }
  if (line == "USB ON") usbMscPrestar(true);
  else if (line == "USB OFF") usbMscPrestar(false);
  Serial.printf("USB-MSC %s, escritos %u sectores (%u KB)%s\n",
                prestada ? "PRESTADA al PC" : "en manos del firmware",
                (unsigned)escritos, (unsigned)(escritos / 2),
                expulsada ? ", expulsada" : "");
  Serial.println("DONE");
  return true;
}
