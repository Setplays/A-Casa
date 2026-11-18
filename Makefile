CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Wpedantic
CPPFLAGS = -Isrc
LDLIBS = -lglut -lGLU -lGL -lm
ifeq ($(OS),Windows_NT)
LDLIBS = -lfreeglut -lopengl32 -lglu32 -lm
EXE = .exe
endif
.PHONY: all test test-zombies test-grapple test-integration clean
all: a_casa$(EXE)
a_casa$(EXE): src/main.c src/zombie_render.c src/grapple_render.c src/game.c src/world.c src/zombies.c src/grapple.c src/game.h src/zombies.h src/zombie_render.h src/grapple.h src/grapple_render.h
	$(CC) $(CPPFLAGS) $(CFLAGS) src/main.c src/zombie_render.c src/grapple_render.c src/game.c src/world.c src/zombies.c src/grapple.c -o $@ $(LDLIBS)
test: test-grapple test-zombies tests/test_game.c src/game.c src/world.c src/game.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -UNDEBUG tests/test_game.c src/game.c src/world.c -o test_game$(EXE) -lm
	./test_game$(EXE)
clean:
	$(RM) a_casa a_casa.exe test_game test_game.exe test_zombies test_zombies.exe test_integration test_integration.exe test_grapple test_grapple.exe

test-zombies: tests/test_zombies.c src/game.c src/world.c src/zombies.c src/grapple.c src/zombies.h src/game.h src/grapple.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -UNDEBUG tests/test_zombies.c src/game.c src/world.c src/zombies.c src/grapple.c -o test_zombies$(EXE) -lm
	./test_zombies$(EXE)

test-integration: tests/test_integration.c src/main.c src/zombie_render.c src/grapple_render.c src/game.c src/world.c src/zombies.c src/grapple.c src/game.h src/zombies.h src/zombie_render.h src/grapple.h src/grapple_render.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -UNDEBUG tests/test_integration.c src/zombie_render.c src/grapple_render.c src/game.c src/world.c src/zombies.c src/grapple.c -o test_integration$(EXE) $(LDLIBS)
	./test_integration$(EXE)

test-grapple: tests/test_grapple.c src/game.c src/world.c src/zombies.c src/grapple.c src/game.h src/zombies.h src/grapple.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -UNDEBUG tests/test_grapple.c src/game.c src/world.c src/zombies.c src/grapple.c -o test_grapple$(EXE) -lm
	./test_grapple$(EXE)
