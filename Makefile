
debug: main.cpp
	g++ -std=c++20 $(sdl2-config --cflags --libs) -lSDL2_gfx -lX11 main.cpp -o auto 

