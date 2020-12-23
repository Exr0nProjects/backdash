main: debug
	./auto

debug: main.cpp deps/simplexnoise.cpp
	clang++ -std=c++20 $(shell sdl2-config --cflags --libs) -lSDL2_gfx -lX11 main.cpp -o auto

build: main.cpp deps/simplexnoise.cpp
	clang++ -O2 -std=c++20 $(shell sdl2-config --cflags --libs) -lSDL2_gfx -lX11 main.cpp -o backdash

