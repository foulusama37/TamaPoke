# Simulador de escritorio: compila el firmware DE VERDAD (TamaPoke.ino,
# pet.cpp, i18n.cpp, sdmon.cpp) contra shims de hardware y vuelca la pantalla
# a imagenes. Lo unico sustituido son los drivers: pantalla, tactil, RTC/PMU,
# audio y SD. Toda la logica y todo el dibujo son los de la placa.
#
#   make            compila
#   make run        compila y corre el guion de ejemplo
#   make gif        convierte los frames en un GIF (necesita Pillow)
CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O1 -g -Wall -Wextra -Wno-unused-parameter
SRC      := ..
BUILD    := build
BIN      := $(BUILD)/tamapoke_sim

SKETCH   := $(BUILD)/sketch.cpp
# shim/fs.cpp (la "SD" sobre una carpeta) salio de shim.cpp para que la
# compile tambien test/: alli sdmon.cpp entro en la bateria y abre los
# sprites con el codigo del firmware. Una sola implementacion.
SOURCES  := main.cpp guion.cpp shim/shim.cpp shim/fs.cpp shim/gfx.cpp $(SKETCH) \
            $(SRC)/pet.cpp $(SRC)/i18n.cpp $(SRC)/sdmon.cpp $(SRC)/battle.cpp $(SRC)/troca.cpp $(SRC)/troca_radio.cpp $(SRC)/duelo.cpp $(SRC)/pvp.cpp
INCLUDES := -I shim -I $(SRC)

SCRIPT ?= sesion.txt
OUTDIR ?= $(BUILD)/frames
EVERY  ?= 3
SD     ?= $(SRC)/tools/sdcard

WINBIN   := $(BUILD)/tamapoke_win.exe
WINSRC   := win/main_win.cpp guion.cpp shim/shim.cpp shim/fs.cpp shim/gfx.cpp \
            $(SKETCH) \
            $(SRC)/pet.cpp $(SRC)/i18n.cpp $(SRC)/sdmon.cpp $(SRC)/battle.cpp $(SRC)/troca.cpp $(SRC)/troca_radio.cpp $(SRC)/duelo.cpp $(SRC)/pvp.cpp

.PHONY: all run gif win jugar clean

all: $(BIN)

# el .ino no es C++ valido tal cual: le faltan los prototipos que inserta el IDE
$(SKETCH): $(wildcard $(SRC)/*.ino) tools/gen_sketch.py
	@mkdir -p $(BUILD)
	python tools/gen_sketch.py

# as fontes CJK de verdade (leva 79), copiadas da biblioteca U8g2; nao vao no git
shim/u8g2_cjk_dados.inc: ../tools/gen_u8g2_cjk_sim.py ../tools/glifos_cjk.py
	python ../tools/gen_u8g2_cjk_sim.py

$(BIN): $(SOURCES) $(wildcard shim/*.h) $(SRC)/pet.h $(SRC)/sdmon.h shim/u8g2_cjk_dados.inc
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $(SOURCES)

run: $(BIN)
	./$(BIN) --script $(SCRIPT) --out $(OUTDIR) --every $(EVERY) --sd $(SD)

gif: run
	python tools/ppm2gif.py $(OUTDIR)

# ventana nativa: el firmware corriendo con el raton de dedo (solo Windows)
win: $(WINBIN)

$(WINBIN): $(WINSRC) $(wildcard shim/*.h) $(SRC)/pet.h $(SRC)/sdmon.h shim/u8g2_cjk_dados.inc
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $(WINSRC) -lgdi32 -luser32

jugar: $(WINBIN)
	./$(WINBIN) --sd $(SD) --zoom 1

clean:
	rm -rf $(BUILD)
