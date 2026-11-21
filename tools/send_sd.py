#!/usr/bin/env python3
"""Envia los ficheros empaquetados a la SD de la placa por USB.

  python3 tools/send_sd.py                  # envia tools/sdcard/mons/*.bin
  python3 tools/send_sd.py --dir bg         # los fondos de combate
  python3 tools/send_sd.py --port COM7
  python3 tools/send_sd.py --ls             # lista lo que hay en la SD
  python3 tools/send_sd.py --dir snd --diff # solo lo que la placa tiene distinto
  python3 tools/send_sd.py --usb on         # la tarjeta aparece en el PC
  python3 tools/send_sd.py --usb off        # se la devuelve al bicho

Con --usb on el aparato se ensena al PC como un lector de tarjetas y la
microSD sale en el explorador. Copiar ahi es mucho mas rapido que PUT y no
hay que abrir la carcasa -- que es el punto, porque esto va a vivir dentro
de una pokebola cerrada con solo el USB-C fuera. Al terminar, EXPULSA la
unidad desde el PC: es lo unico que garantiza que vacio su cache, y el
bicho lo nota y se reinicia solo para releer la tarjeta.

PUT (lo de abajo) sigue valiendo para retocar un sprite suelto; para tandas
grandes es lento.
"""
import argparse
import glob
import os
import sys
import time
import zlib
import serial

def find_port():
    # macOS/Linux primero; en Windows los puertos son COMn y hay que
    # preguntarselos a pyserial.
    ports = glob.glob('/dev/cu.usbmodem*') or glob.glob('/dev/ttyACM*')
    if not ports:
        try:
            from serial.tools import list_ports
            ports = [p.device for p in list_ports.comports()]
        except ImportError:
            ports = []
    if not ports:
        sys.exit("no encuentro la placa; pasa --port")
    return ports[0]

def wait_line(ser, expect, timeout=10):
    end = time.time() + timeout
    while time.time() < end:
        line = ser.readline().decode(errors='replace').strip()
        if not line:
            continue
        print(f"  placa: {line}")
        if line == expect:
            return True
        if line == 'ERR':
            return False
    return False

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--port')
    ap.add_argument('--ls', action='store_true')
    ap.add_argument('--usb', choices=('on', 'off'),
                    help='presta la tarjeta al PC (o la devuelve)')
    ap.add_argument('--dir', default='mons', help='carpeta de tools/sdcard a enviar')
    ap.add_argument('--diff', action='store_true',
                    help='pide SUMS a la placa y manda solo lo que falta o difiere')
    ap.add_argument('--seco', action='store_true',
                    help='con --diff: solo dice que mandaria')
    args = ap.parse_args()

    port = args.port or find_port()
    print(f"puerto {port}")
    ser = serial.Serial(port, 115200, timeout=1)
    time.sleep(1.5)
    ser.reset_input_buffer()

    if args.usb:
        ser.write(b'USB ON\n' if args.usb == 'on' else b'USB OFF\n')
        wait_line(ser, 'DONE', 5)
        if args.usb == 'on':
            print("la tarjeta deberia salir ahora en el explorador.")
            print("al terminar, EXPULSALA desde el PC en vez de --usb off:")
            print("es lo unico que vacia su cache de escritura, y el bicho")
            print("lo nota y se reinicia solo para releerla.")
        return

    if args.ls:
        ser.write(b'LS\n')
        wait_line(ser, 'DONE', 5)
        return

    carpeta = args.dir.strip('/')
    files = sorted(glob.glob(os.path.join(os.path.dirname(__file__), 'sdcard', carpeta, '*.bin')))
    if not files and not args.diff:
        sys.exit(f"no hay .bin en sdcard/{carpeta}; ejecuta antes el pack_* que toque")

    if args.diff:
        # Todo lo de la carpeta (en /snd hay .raw y golpes.txt, no .bin),
        # y se compara tamano Y crc: el tamano solo no basta, muchos sonidos
        # de golpe miden lo mismo que el de la fila de al lado.
        files = sorted(f for f in glob.glob(os.path.join(
            os.path.dirname(__file__), 'sdcard', carpeta, '*')) if os.path.isfile(f))
        ser.write(f"SUMS /{carpeta}\n".encode())
        placa = {}
        end = time.time() + 600
        while time.time() < end:
            ln = ser.readline().decode(errors='replace').strip()
            if ln == 'DONE':
                break
            if ln in ('ERR', 'PRESTADA'):
                sys.exit(f"la placa dijo {ln}")
            p = ln.split()
            if len(p) == 3 and len(p[2]) == 8:
                placa[os.path.basename(p[0])] = (int(p[1]), int(p[2], 16))
        else:
            sys.exit("SUMS no termino")
        dif = []
        for f in files:
            d = open(f, 'rb').read()
            if placa.get(os.path.basename(f)) != (len(d), zlib.crc32(d) & 0xFFFFFFFF):
                dif.append(f)
        sobra = sorted(set(placa) - {os.path.basename(f) for f in files})
        print(f"{len(placa)} en la placa, {len(files)} aqui, {len(dif)} distintos")
        for f in dif[:40]:
            print("  ", os.path.basename(f), placa.get(os.path.basename(f), 'FALTA'))
        if sobra:
            print(f"  {len(sobra)} sobran en la placa: {' '.join(sobra[:20])}")
        files = dif
        if args.seco:
            return

    recusas = 0
    for path in files:
        size = os.path.getsize(path)
        name = f"{carpeta}/{os.path.basename(path)}"
        print(f"-> {name} ({size/1024:.0f} KB)")
        ser.write(f"PUT {name} {size}\n".encode())
        if not wait_line(ser, 'OK', 5):
            print("   la placa no acepto el PUT, sigo con el siguiente")
            recusas += 1
            # Todos seguidos e que a placa nao aceita esta pasta (lista branca
            # do PUT em sdmon.cpp): nao adianta tentar os outros 1990.
            if recusas >= 5 and recusas == files.index(path) + 1:
                sys.exit("la placa rechazo todos: mira la lista de PUT en sdmon.cpp")
            continue
        t0 = time.time()
        ok = True
        with open(path, 'rb') as f:
            while chunk := f.read(2048):
                ser.write(chunk)
                # espera el ack '#' del bloque
                ack = ''
                while ack not in ('#', 'ERR'):
                    ack = ser.readline().decode(errors='replace').strip()
                    if ack == '':
                        ok = False
                        break
                if not ok or ack == 'ERR':
                    ok = False
                    break
        if ok and wait_line(ser, 'DONE', 30):
            kbs = size / 1024 / max(0.01, time.time() - t0)
            print(f"   ok ({kbs:.0f} KB/s)")
        else:
            print("   fallo la transferencia")
    if recusas:
        sys.exit(f"{recusas} de {len(files)} rechazados por la placa")
    print("listo")

if __name__ == '__main__':
    main()
