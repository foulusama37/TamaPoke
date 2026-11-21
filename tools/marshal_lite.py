#!/usr/bin/env python3
"""Lector minimo de Ruby Marshal 4.8, lo justo para los .dat de Essentials.

Los textos traducidos de Pokemon Anil no estan en el PBS (que sigue en
espanol): viven en `Data/messages_english_game.dat`, que es un volcado de
Marshal con el array de MessageTypes. Python no trae lector de Marshal y no
hay Ruby instalado, asi que aqui va uno que cubre solo lo que aparece en esos
ficheros: arrays, cadenas, enteros, simbolos, nil y booleanos.

Despues se le anadieron los dos tipos que hacian falta para
`Data/PkmnAnimations.rxdata`, que es donde el Anil guarda COMO se anima cada
golpe: 'o' (objeto con sus variables de instancia, o sea RPG::Animation y
RPG::Animation::Frame) y 'u' (clase que se serializa ella misma, que aqui es
RPG::Table — la rejilla de numeros con los fotogramas).

NO es un lector completo de Marshal: no hay floats, bignums, structs ni
modulos. Si aparece un tipo que no conoce, avisa con la posicion en vez de
devolver basura en silencio.
"""


class ListaConIvars(list):
    """Una subclase de Array de Ruby: el contenido va en `@array` y el resto
    de variables de instancia (el nombre de la animacion, su hoja) van al
    lado. Se devuelve una lista de verdad para que el codigo de siempre la
    recorra sin enterarse, con las ivars colgadas en `.iv`.
    """

    iv = {}


class Objeto:
    """Un objeto Ruby cualquiera: su clase y sus variables de instancia.

    Se guardan SIN el arroba (`@name` -> `name`) porque asi se leen igual
    que un atributo de Python, que es como se usan aguas abajo.
    """

    def __init__(self, clase):
        self.clase = clase
        self.iv = {}

    def __getattr__(self, k):
        try:
            return self.iv[k]
        except KeyError:
            raise AttributeError(k)

    def __repr__(self):
        return "<%s %s>" % (self.clase, sorted(self.iv))


class Tabla:
    """RPG::Table: la rejilla de enteros de RPG Maker XP.

    Se vuelca con _dump, asi que en el fichero es una ristra de bytes: cinco
    enteros de 32 bits (dimensiones, x, y, z y cuantos elementos) y detras
    los datos como enteros de 16 bits CON signo. Lo de 16 bits importa: en
    cell_data hay posiciones negativas, y leerlas sin signo pondria los
    fotogramas a 65000 pixeles del centro.
    """

    def __init__(self, datos):
        import struct
        self.dim, self.xs, self.ys, self.zs, n = struct.unpack_from("<5i", datos, 0)
        self.v = list(struct.unpack_from("<%dh" % n, datos, 20))

    def __getitem__(self, ij):
        i, j = ij
        return self.v[i + j * self.xs]

    def __repr__(self):
        return "<Tabla %dx%dx%d>" % (self.xs, self.ys, self.zs)


class Marshal:
    def __init__(self, data):
        self.d = data
        self.p = 0
        self.symbols = []   # tabla de simbolos, para las referencias ';'
        self.objects = []   # tabla de objetos, para las referencias '@'

    # ---------------------------------------------------------------- bajo nivel
    def byte(self):
        b = self.d[self.p]
        self.p += 1
        return b

    def long(self):
        """El entero de longitud variable de Marshal (el mismo para tamanos)."""
        c = self.byte()
        if c > 127:
            c -= 256           # a con signo
        if c == 0:
            return 0
        if c > 0:
            if c > 4:
                return c - 5   # 5..127 son 0..122 metidos en el propio byte
            n = 0
            for i in range(c):
                n |= self.byte() << (8 * i)
            return n
        if c < -4:
            return c + 5       # -128..-5 son -123..0
        n = -1
        for i in range(-c):
            n = (n & ~(0xFF << (8 * i))) | (self.byte() << (8 * i))
        return n

    def raw(self):
        n = self.long()
        s = self.d[self.p:self.p + n]
        self.p += n
        return s

    # ---------------------------------------------------------------- valores
    def value(self):
        t = chr(self.byte())
        if t == "[":
            n = self.long()
            out = []
            self.objects.append(out)     # se registra ANTES de llenarlo
            for _ in range(n):
                out.append(self.value())
            return out
        if t in "{}":
            # Los .dat traducidos guardan cada tipo de mensaje como un hash
            # {indice => texto}, no como un array denso: las entradas vacias
            # simplemente no estan.
            n = self.long()
            out = {}
            self.objects.append(out)
            for _ in range(n):
                k = self.value()
                out[k] = self.value()
            if t == "}":
                out["__default__"] = self.value()
            return out
        if t == '"':
            s = self.raw()
            self.objects.append(s)
            return s
        if t == "I":
            # cadena (u otro) con variables de instancia: la de siempre es :E,
            # que solo dice si la codificacion es UTF-8
            v = self.value()
            ivars = {}
            for _ in range(self.long()):
                k = self.value()
                ivars[str(k).lstrip("@")] = self.value()
            # OJO: las ivars NO siempre sobran. En una cadena la unica que
            # hay es :E (la codificacion) y se puede tirar, pero Essentials
            # guarda sus clases propias como SUBCLASES DE ARRAY que dejan el
            # array base VACIO y meten el contenido en `@array`
            # (PBAnimations, PBAnimation). Tirando las ivars, PkmnAnimations
            # se leia entero sin dar error y devolvia una lista de cero
            # elementos: el fallo mas caro de todos, el que no se queja.
            if isinstance(v, list) and not v and isinstance(ivars.get("array"), list):
                out = ListaConIvars(ivars["array"])
                out.iv = ivars
                return out
            return v
        if t == ":":
            s = self.raw().decode("ascii", "replace")
            self.symbols.append(s)
            return s
        if t == ";":
            return self.symbols[self.long()]
        if t == "@":
            return self.objects[self.long()]
        if t == "f":
            # float: va como TEXTO ("1.5", "inf", "nan"). En las animaciones
            # son los tiempos de los timings.
            b = self.raw().decode("ascii", "replace")
            v = {"inf": float("inf"), "-inf": float("-inf"),
                 "nan": float("nan")}.get(b)
            if v is None:
                v = float(b)
            self.objects.append(v)
            return v
        if t == "C":
            # subclase de un tipo basico: el simbolo de la clase y detras el
            # valor normal. PkmnAnimations es un Array de una clase propia,
            # y lo unico que nos importa es el array de dentro.
            self.value()                 # el nombre de la clase, que da igual
            return self.value()
        if t == "o":
            # objeto: el simbolo de la clase, cuantas ivars, y los pares.
            # Se registra ANTES de llenarlo, como el array: RPG::Animation
            # se referencia a si mismo por la tabla de objetos.
            clase = self.value()
            o = Objeto(clase)
            self.objects.append(o)
            for _ in range(self.long()):
                k = self.value()
                o.iv[k.lstrip("@")] = self.value()
            return o
        if t == "u":
            # clase que se serializa ella misma (_dump). La unica que sale
            # en los .rxdata que leemos es RPG::Table.
            clase = self.value()
            datos = self.raw()
            o = Tabla(datos) if clase.endswith("Table") else datos
            self.objects.append(o)
            return o
        if t == "i":
            return self.long()
        if t == "0":
            return None
        if t == "T":
            return True
        if t == "F":
            return False
        raise ValueError(f"tipo de Marshal no soportado {t!r} en el byte {self.p - 1}")


def load(path):
    d = open(path, "rb").read()
    if d[:2] != b"\x04\x08":
        raise ValueError("no empieza por la cabecera de Marshal 4.8")
    m = Marshal(d)
    m.p = 2
    return m.value()


if __name__ == "__main__":
    import sys
    v = load(sys.argv[1])
    print(f"raiz: {type(v).__name__}, {len(v)} elementos")
    for i, x in enumerate(v):
        if isinstance(x, (list, dict)):
            vals = list(x.values()) if isinstance(x, dict) else x
            textos = [s for s in vals if isinstance(s, bytes) and s]
            muestra = textos[0].decode("utf-8", "replace")[:66] if textos else ""
            print(f"  [{i:2}] {type(x).__name__:5} {len(vals):5} entradas, "
                  f"{len(textos):5} con texto  {muestra}")
