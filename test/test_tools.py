#!/usr/bin/env python3
"""Tests de las herramientas del taller (tools/) y de los ficheros generados.

  python3 test/test_tools.py
"""
import os
import re
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TOOLS = os.path.join(ROOT, 'tools')
sys.path.insert(0, TOOLS)


class TestToolsCompile(unittest.TestCase):
    def test_todos_los_scripts_compilan(self):
        scripts = sorted(f for f in os.listdir(TOOLS) if f.endswith('.py'))
        self.assertTrue(scripts, 'no se encontro ningun script en tools/')
        for name in scripts:
            with self.subTest(script=name):
                with open(os.path.join(TOOLS, name), encoding='utf-8') as fh:
                    src = fh.read()
                try:
                    compile(src, name, 'exec')
                except SyntaxError as e:
                    self.fail(f'{name} no compila: {e}')

    @unittest.skipIf(os.name == 'nt', 'en Windows os.access(X_OK) dice que si a todo')
    def test_los_scripts_ejecutables_declaran_shebang(self):
        for name in sorted(f for f in os.listdir(TOOLS) if f.endswith('.py')):
            path = os.path.join(TOOLS, name)
            if not os.access(path, os.X_OK):
                continue
            with open(path, encoding='utf-8') as fh:
                self.assertTrue(fh.readline().startswith('#!'),
                                f'{name} es ejecutable pero no tiene shebang')


class TestDexData(unittest.TestCase):
    """Invariantes de la fuente de datos de la Pokedex."""

    @classmethod
    def setUpClass(cls):
        from dex_data import (DEX, TYPE_ACCENTS, CLASSIC, RARE, LEGENDARY,
                              SLUGS, ALSO_WILD, EVO_ONLY)
        from dex_range import N_DEX
        from dex_stats import BASE_STATS
        from dex_names import LOCAL_NAMES
        cls.DEX, cls.ACCENTS, cls.CLASSIC = DEX, TYPE_ACCENTS, CLASSIC
        cls.RARE, cls.LEGENDARY, cls.SLUGS = RARE, LEGENDARY, SLUGS
        cls.ALSO_WILD, cls.EVO_ONLY, cls.N_DEX = ALSO_WILD, EVO_ONLY, N_DEX
        cls.STATS, cls.NAMES = BASE_STATS, LOCAL_NAMES
        cls.byNum = {row[0]: row for row in DEX}

    def test_estan_todas_las_especies_sin_huecos_ni_repetidos(self):
        nums = [row[0] for row in self.DEX]
        self.assertEqual(len(nums), self.N_DEX)
        self.assertEqual(sorted(nums), list(range(1, self.N_DEX + 1)))

    def test_los_slugs_son_unicos_y_en_minusculas(self):
        slugs = [row[1] for row in self.DEX]
        self.assertEqual(len(set(slugs)), len(slugs), 'hay slugs repetidos')
        for s in slugs:
            self.assertEqual(s, s.lower(), f'slug con mayusculas: {s}')
            self.assertTrue(s.isascii(), f'slug no ASCII: {s}')

    def test_los_nombres_de_pantalla_son_ascii_en_mayusculas(self):
        # la fuente GFX del firmware no tiene acentos ni minusculas
        for num, slug, display, *_ in self.DEX:
            self.assertTrue(display.isascii(), f'{num} {display} no es ASCII')
            self.assertEqual(display, display.upper(), f'{num} {display} lleva minusculas')
            self.assertLessEqual(len(display), 12, f'{num} {display} no cabe en pantalla')

    def _con_objeto(self):
        """Las especies a las que les sirve algun objeto para evolucionar.

        Sale del evos.h generado desde el PBS del Anil. Hace falta porque
        "evoluciona" ya no implica "tiene nivel": el Anil le da a Pikachu,
        a Eevee, a Gloom y a quince mas la piedra como UNICO camino, y
        entonces el nivel 0 es el dato, no un hueco.
        """
        ruta = os.path.join(ROOT, 'evos.h')
        if not os.path.exists(ruta):
            return set()
        with open(ruta, encoding='utf-8', errors='replace') as fh:
            # `\d+(?:,\s*\d+)*` y no un numero fijo de campos: EvoItem
            # gano el sexo (0/1/2) al entrar las evoluciones que lo piden,
            # y un patron con tres huecos dejo de encontrar nada — el test
            # no fallaba, se quedaba vigilando el vacio. Por eso ademas
            # hay un assert de que la lista NO esta vacia.
            return {int(m) for m in re.findall(
                r'^\s*\{\s*(\d+),\s*\w+\s*,\s*\d+(?:,\s*\d+)*\s*\}',
                fh.read(), re.M)}

    def test_las_evoluciones_apuntan_a_especies_reales(self):
        con_objeto = self._con_objeto()
        self.assertTrue(con_objeto, 'no leo evos.h: el test no vigilaria nada')
        for num, slug, display, tipo, evo, lvl in self.DEX:
            if evo:
                self.assertIn(evo, self.byNum, f'{display} evoluciona a {evo}, que no existe')
                self.assertNotEqual(evo, num, f'{display} evoluciona a si mismo')
                self.assertTrue(
                    lvl > 0 or num in con_objeto,
                    f'{display} evoluciona y no dice ni nivel ni objeto')
            else:
                self.assertEqual(lvl, 0, f'{display} es forma final pero tiene nivel {lvl}')

    def test_ninguna_cadena_evolutiva_hace_bucle(self):
        for num in self.byNum:
            seen, cur = set(), num
            while cur and self.byNum[cur][4]:
                self.assertNotIn(cur, seen, f'bucle evolutivo desde {num}')
                seen.add(cur)
                cur = self.byNum[cur][4]
                self.assertLessEqual(len(seen), 5, f'cadena demasiado larga desde {num}')

    def test_todos_los_tipos_tienen_color(self):
        for num, slug, display, tipo, *_ in self.DEX:
            self.assertIn(tipo, self.ACCENTS, f'{display}: tipo "{tipo}" sin color')

    def test_los_colores_son_hex_de_6_digitos(self):
        for tipo, col in self.ACCENTS.items():
            self.assertRegex(col, r'^#[0-9a-fA-F]{6}$', f'color raro en {tipo}: {col}')

    def _solo_por_evolucion(self):
        """Lo mismo que calcula gen_dex.py para la rareza R_EVO.

        No basta "alguien le apunta con evolvesTo": ALSO_WILD son los que
        ademas salen por su cuenta (Pikachu dejo de ser forma base al entrar
        Pichu) y EVO_ONLY los que no apunta nadie pero solo se consiguen
        evolucionando (las segundas ramas de piedra).
        """
        return ({row[4] for row in self.DEX if row[4]} | self.EVO_ONLY) - self.ALSO_WILD

    def test_las_rarezas_apuntan_a_formas_base(self):
        evolucionadas = self._solo_por_evolucion()
        for num in sorted(self.RARE | self.LEGENDARY):
            self.assertIn(num, self.byNum, f'rareza para una especie inexistente: {num}')
            self.assertNotIn(num, evolucionadas,
                             f'{self.byNum[num][2]} solo sale por evolucion: la rareza no le sirve')
        self.assertFalse(self.RARE & self.LEGENDARY, 'una especie no puede ser rara y legendaria')
        self.assertFalse(self.ALSO_WILD & self.EVO_ONLY,
                         'una especie no puede estar en las dos listas a la vez')
        for num in self.ALSO_WILD:
            self.assertIn(num, {row[4] for row in self.DEX if row[4]},
                          f'{self.byNum[num][2]} esta en ALSO_WILD sin ser evolucion de nadie')

    def test_los_iniciales_clasicos_existen(self):
        evolucionadas = self._solo_por_evolucion()
        for num in self.CLASSIC:
            self.assertIn(num, self.byNum, f'inicial inexistente: {num}')
            self.assertNotIn(num, evolucionadas, f'{self.byNum[num][2]} no puede ser inicial')

    def test_hay_stats_base_de_las_151(self):
        for num in self.byNum:
            self.assertIn(num, self.STATS, f'faltan stats de {self.byNum[num][2]}')
            # seis desde que el especial se partio en SPA/SPD; este test se
            # quedo desempaquetando cuatro y llevaba desde entonces sin correr
            fila = self.STATS[num]
            self.assertEqual(len(fila), 6, f'{self.byNum[num][2]}: no son 6 stats')
            for v in fila:
                self.assertTrue(0 < v <= 255, f'stat fuera de rango en {self.byNum[num][2]}: {v}')

    def test_los_nombres_traducidos_cuadran_con_su_fuente(self):
        # Los latinos se pintan con la fuente CP437 (un byte por caracter, sin
        # minusculas en los nombres); el japones y el coreano con una fuente
        # U8g2 y van en UTF-8, asi que ni son ASCII ni tienen mayusculas que
        # comprobar.
        LATINOS, CJK = ('fr', 'de'), ('ja', 'ko')
        for num, langs in self.NAMES.items():
            self.assertIn(num, self.byNum, f'nombre traducido de una especie inexistente: {num}')
            for lang, name in langs.items():
                self.assertIn(lang, LATINOS + CJK, f'idioma inesperado: {lang}')
                self.assertTrue(name, f'{num} {lang} vacio')
                if lang in LATINOS:
                    self.assertTrue(name.isascii(), f'{num} {lang} no es ASCII: {name}')
                    self.assertEqual(name, name.upper(), f'{num} {lang} lleva minusculas: {name}')
                    self.assertLessEqual(len(name), 12, f'{num} {lang} no cabe en pantalla: {name}')
                else:
                    # el limite util son CARACTERES, no bytes: en UTF-8 cada kana o silaba hangul ocupa 3
                    self.assertLessEqual(len(name), 8, f'{num} {lang} no cabe en pantalla: {name}')


class TestGeneratedFiles(unittest.TestCase):
    """dex.h esta generado: si no coincide con su generador, alguien lo edito a mano."""

    def test_gen_dex_reproduce_dex_h(self):
        tmp = tempfile.mkdtemp(prefix='tamapoke-gen-')
        try:
            shutil.copytree(TOOLS, os.path.join(tmp, 'tools'))
            # gen_dex lee tambien i18n.h/i18n.cpp, para medir cuanto ocupa
            # la cabecera de la ficha en el peor idioma (DEX_HEAD_BUF). Sin
            # copiarlos, el generador se cae aqui y el test diria que el
            # dex.h no se reproduce cuando lo que falta es el decorado.
            for n in ('i18n.h', 'i18n.cpp'):
                shutil.copy(os.path.join(ROOT, n), os.path.join(tmp, n))
            r = subprocess.run([sys.executable, os.path.join(tmp, 'tools', 'gen_dex.py')],
                               capture_output=True, text=True)
            self.assertEqual(r.returncode, 0, f'gen_dex.py fallo:\n{r.stderr}')
            generated = os.path.join(tmp, 'dex.h')
            self.assertTrue(os.path.exists(generated), 'gen_dex.py no escribio dex.h')
            with open(generated, encoding='utf-8') as a, \
                 open(os.path.join(ROOT, 'dex.h'), encoding='utf-8') as b:
                self.assertEqual(a.read(), b.read(),
                                 'dex.h no coincide con lo que genera tools/gen_dex.py '
                                 '(vuelve a generarlo en vez de editarlo a mano)')
        finally:
            shutil.rmtree(tmp, ignore_errors=True)

    def test_los_ficheros_generados_avisan_de_que_lo_son(self):
        for name in ('dex.h', 'species.h', 'art.h'):
            with open(os.path.join(ROOT, name), encoding='utf-8') as fh:
                head = fh.read(400)
            self.assertIn('GENERADO', head, f'{name} deberia decir que esta generado')


class TestSketchSanity(unittest.TestCase):
    """Comprobaciones baratas sobre el .ino que no necesitan compilarlo."""

    @classmethod
    def setUpClass(cls):
        with open(os.path.join(ROOT, 'TamaPoke.ino'), encoding='utf-8') as fh:
            cls.ino = fh.read()

    def test_el_sketch_no_lleva_caracteres_no_ascii_en_literales_de_pantalla(self):
        # los comentarios pueden llevar lo que quieran; los literales, no:
        # la fuente GFX es ASCII y un acento sale como basura
        for lineno, line in enumerate(self.ino.splitlines(), 1):
            code = line.split('//')[0]
            for chunk in code.split('"')[1::2]:
                self.assertTrue(chunk.isascii(),
                                f'TamaPoke.ino:{lineno}: literal no ASCII: {chunk!r}')

    def test_las_llaves_estan_equilibradas(self):
        self.assertEqual(self.ino.count('{'), self.ino.count('}'),
                         'llaves descompensadas en TamaPoke.ino')


class TestTablasPorEnum(unittest.TestCase):
    """Una tabla `X nombre[ALGO_COUNT] = { ... }` con menos elementos que el
    enum NO es un error de compilacion: C++ rellena el resto con ceros. En una
    tabla de `const char *` eso son punteros nulos, y el printf imprime
    "<null>" en mitad de la partida.

    Paso de verdad: al anadir la evasion, SG_COUNT paso de 6 a 7 y las dos
    tablas de nombres de stage se quedaron en seis. Sweet Scent anunciaba
    "<null> fell". Ahora las tablas llevan static_assert, y este test
    comprueba que NINGUNA tabla dimensionada por un *_COUNT se quede corta.
    """

    # (fichero, enum, cuantos vale de verdad). Los que no son listas de
    # literales (matrices, buffers de trabajo) se saltan solos mas abajo.
    CUENTAS = {'SG_COUNT': 7, 'EV_N': 6, 'BP_COUNT': 5, 'LANG_COUNT': 8}
    DECL = re.compile(
        r'\b(?:static\s+)?const\s+char\s*\*(?:\s*const)?\s*\w+\s*'
        r'\[\s*(\w*_(?:COUNT|N))\s*\]\s*=\s*\{(.*?)\}\s*;', re.S)

    def test_ninguna_tabla_de_nombres_se_queda_corta(self):
        fuentes = [f for f in os.listdir(ROOT)
                   if f.endswith(('.ino', '.h', '.cpp'))]
        revisadas = 0
        for nombre in sorted(fuentes):
            with open(os.path.join(ROOT, nombre), encoding='utf-8') as fh:
                src = fh.read()
            for m in self.DECL.finditer(src):
                enum, cuerpo = m.group(1), m.group(2)
                esperado = self.CUENTAS.get(enum)
                if esperado is None:
                    continue
                n = len(re.findall(r'"', cuerpo)) // 2
                self.assertEqual(
                    n, esperado,
                    f'{nombre}: tabla de {enum} con {n} literales y no '
                    f'{esperado}; los que faltan salen como "<null>"')
                revisadas += 1
        self.assertGreater(revisadas, 0, 'el patron ya no encuentra ninguna tabla')



class TestBuffersDeTexto(unittest.TestCase):
    """Cada snprintf(buf, sizeof(buf), T(S_x), ...) cabe en su buffer en TODOS los idiomas.

    Los buffers se dimensionaron cuando toda cadena era de un byte por
    caracter. En UTF-8 un kana o una silaba hangul ocupan 3, y snprintf corta
    en silencio: a veces se pierden cifras ("きろく 2" por "きろく 219") y a
    veces corta a mitad de un caracter, que la fuente ya no sabe dibujar. No lo
    avisa ni el compilador ni la pantalla, asi que se comprueba aqui con el
    peor caso de cada argumento segun su tipo:

      %u / %d  -> 5 cifras (todos los argumentos son de 16 bits o menos)
      %lu      -> 10 cifras
      %s       -> lo que declare ARGS_S; un %s sin declarar hace fallar el
                  test, para que quien lo anada piense cuanto puede ocupar
    """

    ARGS_S = {  # que puede llegar por cada %s, en orden
        'S_NAME_FMT': ('estrella', 'nombre_o_apodo'),
        'S_RELEASE_FMT': ('nombre',),
        'S_INFO_FMT': ('baya',),
        'S_FAREWELL_BTN': ('nombre_o_apodo',),
        'S_RUNAWAY_BTN': ('nombre_o_apodo',),
        'S_GYM_GAIN_FMT': ('ev',),
        'S_GYM_CHAMP': ('region',),
    }
    CIFRAS = {'u': 5, 'd': 5, 'lu': 10}
    ESPEC = re.compile(r'%0?\d*(lu|u|d|s)')

    @classmethod
    def setUpClass(cls):
        import test_i18n_formats as fmt
        from dex_data import DEX
        from dex_names import LOCAL_NAMES

        def leer(nombre):
            with open(os.path.join(ROOT, nombre), encoding='utf-8') as fh:
                return fh.read()

        cls.LANGS = fmt.LANGS
        cuerpo = re.search(r'enum\s+StrId[^{]*\{(.*?)\}', leer('i18n.h'), re.S).group(1)
        # Fuera los comentarios ANTES de buscar los IDs: un comentario que
        # mencione un S_ALGO (por ejemplo para avisar de que S_STAT_SPE es la
        # velocidad y no la defensa especial) se colaba como si fuera una
        # entrada mas del enum y descuadraba la tabla entera.
        cuerpo = re.sub(r'//[^\n]*', '', cuerpo)
        ids = re.findall(r'\b(S_[A-Z0-9_]+)\b', cuerpo)
        filas = fmt.extract_table(leer('i18n.cpp'), 'STRINGS', len(cls.LANGS))
        if len(filas[0]) != len(ids):
            raise AssertionError(
                f'el enum StrId tiene {len(ids)} entradas y cada idioma '
                f'{len(filas[0])}: falta anadir la cadena en i18n.cpp')
        cls.LIT = {sid: [fila[k] for fila in filas] for k, sid in enumerate(ids)}
        cls.ino = leer('TamaPoke.ino').splitlines()
        cls.APODO = int(re.search(r'char\s+nick\[(\d+)\]', leer('pet.h')).group(1)) - 1
        # El nombre de region sale de la tabla REGIONS de gyms.h, no de una
        # constante: al anadir una region larga el buffer tiene que avisar.
        regs = re.search(r'REGIONS\[REGION_COUNT\]\s*=\s*\{(.*?)\};',
                         leer('gyms.h'), re.S).group(1)
        cls.REGION_MAX = max(len(n) for n in re.findall(r'\{\s*"([^"]+)"', regs))
        # Los #define de los generados, para los buffers que ya no llevan
        # un numero a pelo (char head[DEX_HEAD_BUF]).
        cls.DEFINES = {}
        for f in ('dex.h', 'moves.h', 'abilities.h', 'items.h'):
            for nom, val in re.findall(r'#define (\w+)\s+(\d+)\s*$',
                                       leer(f), re.M):
                cls.DEFINES[nom] = val
        base = max(len(row[2].encode()) for row in DEX)
        cls.NOMBRE = {}
        for lang in cls.LANGS:
            propios = [n[lang.lower()] for n in LOCAL_NAMES.values() if lang.lower() in n]
            cls.NOMBRE[lang] = max([base] + [len(n.encode('utf-8')) for n in propios])

    @staticmethod
    def bytes_literal(literal):
        """Bytes que ocupa en memoria un literal C escrito como en el fuente."""
        cuerpo = literal[1:-1].encode('utf-8')
        n = i = 0
        while i < len(cuerpo):
            if cuerpo[i] == 0x5C:  # barra invertida: \ooo octal, o \n \" \\ ...
                j = i + 1
                while j < len(cuerpo) and j < i + 4 and 0x30 <= cuerpo[j] <= 0x37:
                    j += 1
                i = j if j > i + 1 else i + 2
            else:
                i += 1
            n += 1
        return n

    def peor_caso(self, sid, li):
        lang, lit = self.LANGS[li], self.LIT[sid][li]
        total = self.bytes_literal(lit) + 1  # + terminador
        pendientes = list(self.ARGS_S.get(sid, ()))
        for m in self.ESPEC.finditer(lit):
            total -= len(m.group(0))
            if m.group(1) != 's':
                total += self.CIFRAS[m.group(1)]
                continue
            self.assertTrue(pendientes, f'{sid}: tiene un %s sin declarar en ARGS_S')
            arg = pendientes.pop(0)
            if arg == 'estrella':
                total += 1
            elif arg == 'nombre':
                total += self.NOMBRE[lang]
            elif arg == 'nombre_o_apodo':
                total += max(self.NOMBRE[lang], self.APODO)
            elif arg == 'baya':
                total += max(self.bytes_literal(self.LIT[b][li]) for b in
                             ('S_BERRY_UNK', 'S_BERRY_RED', 'S_BERRY_BLUE', 'S_BERRY_GREEN'))
            elif arg == 'ev':
                # Pet::evName(): "HP"/"ATK"/... son ASCII fijos, sin traducir
                total += 3
            elif arg == 'region':
                # Region::name de gyms.h: ASCII fijo, sin traducir
                total += self.REGION_MAX
        return total

    def test_cada_texto_formateado_cabe_en_su_buffer_en_todos_los_idiomas(self):
        llamada = re.compile(r'snprintf\((\w+),\s*sizeof\(\1\),\s*T\((S_[A-Z0-9_]+)\)')
        revisadas = 0
        for n, linea in enumerate(self.ino, 1):
            m = llamada.search(linea.split('//')[0])
            if not m:
                continue
            var, sid = m.groups()
            # El tamano puede ser un numero o una CONSTANTE: la cabecera
            # de la ficha pasó a `char head[DEX_HEAD_BUF]` para que los
            # tres sitios que la pintan no pudieran separarse. Se resuelve
            # contra los #define de los generados.
            tam = next((d.group(1) for k in range(n - 2, -1, -1)
                        for d in [re.search(r'\bchar\s+' + var + r'\s*\[(\w+)\]',
                                            self.ino[k])]
                        if d), None)
            self.assertIsNotNone(tam, f'TamaPoke.ino:{n}: no encuentro la declaracion de {var}')
            if not tam.isdigit():
                self.assertIn(tam, self.DEFINES,
                              f'TamaPoke.ino:{n}: {var}[{tam}] y no se que vale {tam}')
                tam = self.DEFINES[tam]
            tam = int(tam)
            revisadas += 1
            for li, lang in enumerate(self.LANGS):
                with self.subTest(linea=n, id=sid, idioma=lang):
                    peor = self.peor_caso(sid, li)
                    self.assertLessEqual(
                        peor, tam,
                        f'TamaPoke.ino:{n}: {var}[{tam}] con {sid} en {lang} necesita {peor} bytes')
        self.assertGreater(revisadas, 10, 'el patron de snprintf ya no encuentra las llamadas')


class TestContadoresQueNoCaben(unittest.TestCase):
    """`for (uint8_t i = ...; i < ALGO_COUNT; i++)` con ALGO_COUNT > 255.

    La condicion nunca es falsa: el contador da la vuelta en 255 y el
    bucle NO TERMINA. El compilador lo avisa ("comparison is always true
    due to limited range of data type"), pero entre los cientos de avisos
    de los headers de Espressif no lo ve nadie; aqui es un fallo.

    Paso de verdad al entrar Galar: el Anil tiene 293 habilidades y la
    numero 256 es la de Zacian, asi que ABIL_COUNT se paso de 255 y dejo
    dos bucles colgados, uno en el comando de depuracion ABIL del .ino y
    otro en los propios tests.

    Es el fallo de siempre de este proyecto (evolvesTo era uint8_t, el
    indice de golpe tambien), pero visto desde el BUCLE y no desde el
    campo, que es donde no se ve venir.
    """

    GENERADOS = ('dex.h', 'moves.h', 'abilities.h', 'items.h', 'evos.h')
    BUCLE = re.compile(
        r'for\s*\(\s*(u?int8_t|char)\s+\w+\s*=[^;]+;\s*\w+\s*<\s*(\w+_COUNT)\b')

    def cuentas(self):
        out = {}
        for f in self.GENERADOS:
            ruta = os.path.join(ROOT, f)
            if not os.path.exists(ruta):
                continue
            with open(ruta, encoding='utf-8', errors='replace') as fh:
                for nom, val in re.findall(r'#define (\w+_COUNT)\s+(\d+)',
                                           fh.read()):
                    out[nom] = int(val)
        return out

    def test_ningun_bucle_de_un_byte_cuenta_mas_de_255(self):
        cuentas = self.cuentas()
        self.assertIn('ABIL_COUNT', cuentas, 'no leo los *_COUNT generados')
        malos = []
        for carpeta in (ROOT, os.path.join(ROOT, 'test')):
            for nombre in sorted(os.listdir(carpeta)):
                if not nombre.endswith(('.h', '.cpp', '.ino')):
                    continue
                with open(os.path.join(carpeta, nombre), encoding='utf-8',
                          errors='replace') as fh:
                    for n, linea in enumerate(fh, 1):
                        for tipo, cte in self.BUCLE.findall(linea.split('//')[0]):
                            if cuentas.get(cte, 0) > 255:
                                malos.append(f'{nombre}:{n}  {tipo} contra '
                                             f'{cte} (={cuentas[cte]})')
        self.assertEqual(malos, [], 'bucles que no terminarian:\n  '
                                    + '\n  '.join(malos))


if __name__ == '__main__':
    unittest.main(verbosity=2)
