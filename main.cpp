#include <SDL2/SDL.h>
#include <X11/Xlib.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <vector>
#include <algorithm>

const int WIDTH = 3000;
const int HEIGHT = 1920;

const int CENTER_X = 800;
const int CENTER_Y = 600;
const int RADIUS = 2;
const int LAYERS = 9;
const unsigned long SUNCOLOR = 0xee339900;

int rng(int l, int r)
{ return rand() % (r-l+1) + l; }

struct Triangle
{
    float x, y, w, h, s;
    int cr, cg, cb;
    void render(SDL_Renderer* renderer)
    {
        filledTrigonRGBA(renderer, x, y, x+w/2, y-h, x+w, y, cr, cg, cb, 0xff);
        x = (x - s);
        if (x+w < 0) x += WIDTH+w;   // TODO sketchy mod
    }
};

int main()
{
    std::vector<Triangle> layer;
    for (int i=0; i<40; ++i)
        layer.emplace_back(300*i, 1000, 700+rng(-100, 100), 400+rng(-200, 200), 0.1, 0x44, 0x33, 0x33);
    for (int i=0; i<90; ++i)
        layer.emplace_back(50*i, 1000, 70+rng(-20, 20), 120+rng(-20, 20), 0.4, 0x05, 0x55, 0x05);
    //for (int i=1; i<=10; ++i)
    //    layer.emplace_back(10, 1000, 50, 90);

    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s", SDL_GetError());
        return 3;
    }

    const auto x11d = XOpenDisplay(NULL);
    const Window x11w = RootWindow(x11d, DefaultScreen(x11d));
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *window = SDL_CreateWindowFrom((void*)x11w);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, WIDTH, HEIGHT);

    for (SDL_Event event = {}; event.type != SDL_QUIT; SDL_PollEvent(&event))
    {
        // clear, I guess?
        SDL_SetRenderTarget(renderer, texture);
        SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0x00);
        SDL_RenderClear(renderer);
        // draw triangles
        for (auto &tri : layer)
            tri.render(renderer);
        // draw circles
        for (int i=0; i<LAYERS; ++i)
            filledCircleRGBA(renderer, CENTER_X, CENTER_Y, RADIUS<<i, 0xff, 0xee, 0x20, 0x40*i/LAYERS);
        // draw to the screen?
        SDL_SetRenderTarget(renderer, NULL);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
    }
    SDL_DestroyRenderer(renderer);
    SDL_Quit();
}

