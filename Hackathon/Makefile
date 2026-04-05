export QNX_HOST = C:/Users/yoy91/qnx800/host/win64/x86_64
export QNX_TARGET = C:/Users/yoy91/qnx800/target/qnx
CC = $(QNX_HOST)/usr/bin/qcc
TARGET_ARCH = -Vgcc_ntox86_64
CFLAGS = -Wall -O1 $(TARGET_ARCH) -I$(QNX_TARGET)/usr/include -Isrc/include
LDFLAGS = -L$(QNX_TARGET)/x86_64/usr/lib -L$(QNX_TARGET)/x86_64/lib
LIBS_GAME = -lscreen -lsocket -lm
LIBS_PROC  = -lsocket
BIN = bin
OBJ = bin/obj

all: dirs $(BIN)/game_main $(BIN)/engine_proc $(BIN)/reactor_proc

dirs:
	mkdir -p $(BIN) $(OBJ)

$(OBJ)/game_main.o: src/core/game_main.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ)/game_tasks.o: src/core/game_tasks.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ)/ui_engine.o: src/drivers/ui_engine.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ)/controller_input.o: src/drivers/controller_input.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN)/game_main: $(OBJ)/game_main.o $(OBJ)/game_tasks.o $(OBJ)/ui_engine.o $(OBJ)/controller_input.o
	$(CC) $(TARGET_ARCH) $^ -o $@ $(LDFLAGS) $(LIBS_GAME)

$(BIN)/engine_proc: src/core/engine_proc.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS) $(LIBS_PROC)

$(BIN)/reactor_proc: src/core/reactor_proc.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS) $(LIBS_PROC)

deploy: all
	mkdir -p deploy
	cp $(BIN)/game_main $(BIN)/engine_proc $(BIN)/reactor_proc deploy/
	cp scripts/autorun_game.sh deploy/autorun.sh
	@echo "=== Deploy ready ==="

clean:
	rm -rf $(BIN) deploy

.PHONY: all dirs clean deploy
