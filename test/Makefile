# Tests de la logica del firmware en el PC (sin placa).
#   make        -> compila y ejecuta
#   make FILTER=evolve  -> solo los tests cuyo nombre contenga "evolve"
CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O1 -g -Wall -Wextra -Wno-unused-parameter \
            -fno-omit-frame-pointer
LDFLAGS  ?=
SRC_DIR  := ..
BUILD    := build
BIN      := $(BUILD)/tamapoke_tests

# sdmon.cpp entra con el shim de ficheros del SIMULADOR (desktop/shim/fs.cpp,
# la "SD" es tools/sdcard): asi los sprites se abren con EL CODIGO DEL
# FIRMWARE. tools/auditoria.py comprueba que los ficheros estan y se dejan
# leer, pero no que el firmware sepa pedirlos, y ese fue justo el fallo que
# mas tiempo paso escondido (PmdMon::load con el dex en uint8_t: desde Hoenn
# pedia otro fichero, y habia uno valido al otro lado).
SOURCES := main.cpp shim/shim.cpp test_pet.cpp test_dex.cpp test_i18n.cpp \
           test_battle.cpp test_quiz.cpp test_sd.cpp test_gauntlet.cpp test_gestos.cpp test_troca.cpp test_duelo.cpp test_pvp.cpp \
           $(SRC_DIR)/pet.cpp $(SRC_DIR)/i18n.cpp $(SRC_DIR)/battle.cpp \
           $(SRC_DIR)/sdmon.cpp $(SRC_DIR)/gestos.cpp $(SRC_DIR)/troca.cpp $(SRC_DIR)/duelo.cpp $(SRC_DIR)/pvp.cpp \
           $(SRC_DIR)/desktop/shim/fs.cpp

.PHONY: all run clean asan
all: run

# -I shim va PRIMERO: Arduino.h y Preferences.h son los de aqui, y de
# desktop/shim solo se cogen FS.h y SD_MMC.h, que no existen en el nuestro.
INCS := -I shim -I $(SRC_DIR)/desktop/shim

$(BIN): $(SOURCES) framework.h shim/Arduino.h shim/Preferences.h
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) $(INCS) -o $@ $(SOURCES) $(LDFLAGS)

run: $(BIN)
	@./$(BIN) $(FILTER)

# misma suite bajo AddressSanitizer + UBSan: caza desbordes e indices malos.
#
# OJO EN WINDOWS: w64devkit NO trae libasan ni libubsan, asi que esto
# falla al enlazar ("cannot find -lasan"). No es un fallo del codigo.
# Alli se usa `make ubtrap`, que da la mitad de la red sin pedir
# bibliotecas.
asan: CXXFLAGS += -fsanitize=address,undefined -fno-sanitize-recover=all
asan: BIN := $(BUILD)/tamapoke_tests_asan
asan:
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) $(INCS) -o $(BUILD)/tamapoke_tests_asan $(SOURCES) $(LDFLAGS)
	@./$(BUILD)/tamapoke_tests_asan $(FILTER)

# UBSan SIN su biblioteca: en vez de imprimir un informe, el programa se
# para en seco (SIGILL) en la primera operacion con comportamiento
# indefinido. Se pierde el mensaje bonito y se gana poder correrlo en
# Windows, que es donde se desarrolla esto.
#
# Lo que caza y los tests normales no: desbordes de enteros con signo,
# desplazamientos fuera de rango, punteros desalineados y meter en un
# entero un float que no cabe. El gauntlet (4000 combates al azar) es
# justo el que mas provecho le saca.
#
# -D_GLIBCXX_ASSERTIONS ademas comprueba los indices de la libreria
# estandar, que aqui son los de los std::vector de los tests.
ubtrap: CXXFLAGS := -std=c++17 -O1 -g -Wall -Wextra -Wno-unused-parameter \
                    -fsanitize=undefined -fsanitize-undefined-trap-on-error \
                    -D_GLIBCXX_ASSERTIONS
ubtrap:
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) $(INCS) -o $(BUILD)/tamapoke_tests_ub $(SOURCES) $(LDFLAGS)
	@./$(BUILD)/tamapoke_tests_ub $(FILTER)

# La misma suite CONTANDO QUE LINEAS SE EJECUTAN. Lo usa
# tools/cobertura.py para decir que efecto de golpe o que habilidad no
# llega a probar nadie, sin que yo tenga que marcar a mano quien cubre
# que, que es justo la parte en la que me equivocaria.
#
# -O0 a proposito: con optimizacion, gcc funde y reordena lineas y el
# contador deja de corresponderse con el `case` que uno lee.
cover: CXXFLAGS := -std=c++17 -O0 -g -Wall -Wextra -Wno-unused-parameter \
                   --coverage
cover: LDFLAGS += --coverage
cover:
	@mkdir -p $(BUILD)/cov
	$(CXX) $(CXXFLAGS) $(INCS) -o $(BUILD)/tamapoke_tests_cov $(SOURCES) $(LDFLAGS)
	@./$(BUILD)/tamapoke_tests_cov $(FILTER) > $(BUILD)/cov/run.log || true
	@tail -3 $(BUILD)/cov/run.log

# ------------------------------------------------------- la INTERFAZ
#
# Binario APARTE porque el sketch necesita los shims del simulador
# (pantalla, tactil, Wire) y esos chocan con los de test/shim: los dos
# definen millis(), random() y Serial. Separando no hay que elegir, y
# cada bateria usa el shim que le va.
#
# Esto es lo que mete los .ino bajo prueba: eran dos tercios del codigo
# escrito a mano sin un solo assert.
UIBIN    := $(BUILD)/tamapoke_uitests
SKETCH   := ../desktop/build/sketch.cpp
DSHIM    := ../desktop/shim
UISOURCES := main.cpp test_ui.cpp $(SKETCH) ../desktop/guion.cpp \
             $(DSHIM)/shim.cpp $(DSHIM)/fs.cpp $(DSHIM)/gfx.cpp \
             $(SRC_DIR)/pet.cpp $(SRC_DIR)/i18n.cpp $(SRC_DIR)/battle.cpp \
             $(SRC_DIR)/sdmon.cpp $(SRC_DIR)/troca.cpp $(SRC_DIR)/troca_radio.cpp $(SRC_DIR)/duelo.cpp $(SRC_DIR)/pvp.cpp

# El .ino no es C++ valido tal cual: le faltan los prototipos que mete
# el IDE. Los pone el mismo generador que usa el simulador.
$(SKETCH): $(wildcard $(SRC_DIR)/*.ino) ../desktop/tools/gen_sketch.py
	@cd ../desktop && python tools/gen_sketch.py

# COMPILAR sin correr. Lo pide tools/sabotaje_items.py, que construye y
# despues lanza UN teste suelto. Antes no existia, y pedirle a make un
# objetivo que no conoce pero que YA EXISTE como fichero no da error: se
# queda callado. Entonces se mide el binario viejo creyendo que es el
# nuevo —— ya me paso dos veces en esta sesion.
#
# Recompila SIEMPRE, a proposito: las cabeceras no estan en la lista de
# dependencias, asi que lo incremental se saltaria builds que hacen
# falta. Tarda, pero no miente.
.PHONY: uibin ui
uibin: $(SKETCH)
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) -I $(DSHIM) -I $(SRC_DIR) -o $(UIBIN) $(UISOURCES) $(LDFLAGS)

ui: uibin
	@./$(UIBIN) $(FILTER)

# Lo mismo contando lineas, para que tools/cobertura.py pueda decir
# cuanto de los .ino se recorre de verdad. Sin esto, "los .ino no
# tienen tests" se contesta de memoria.
uicover: CXXFLAGS := -std=c++17 -O0 -g -Wall -Wextra -Wno-unused-parameter \
                     --coverage
uicover: LDFLAGS += --coverage
uicover: $(SKETCH)
	@mkdir -p $(BUILD)/cov
	$(CXX) $(CXXFLAGS) -I $(DSHIM) -I $(SRC_DIR) -o $(BUILD)/tamapoke_uicov \
	  $(UISOURCES) $(LDFLAGS)
	@./$(BUILD)/tamapoke_uicov $(FILTER) > $(BUILD)/cov/ui.log || true
	@tail -2 $(BUILD)/cov/ui.log

clean:
	rm -rf $(BUILD) *.gcda *.gcno *.gcov
