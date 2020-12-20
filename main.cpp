#include <SDL2/SDL.h>
#include <X11/Xlib.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <vector>
#include <algorithm>

// TODO: auto-get height https://stackoverflow.com/questions/33393528/how-to-get-screen-size-in-sdl
const int WIDTH = 3000;
const int HEIGHT = 1920;

const int CENTER_X = 800;
const int CENTER_Y = 600;
const int RADIUS = 2;
const int LAYERS = 9;
const unsigned long SUNCOLOR = 0xee339900;
typedef unsigned char color_t;

int rng(int l, int r)
{ return rand() % (r-l+1) + l; }

class Renderable
{
public:
    float x, y, w, h;
    Renderable(float x, float y, float w, float h): x(x), y(y), w(w), h(h) {}
    virtual void render(SDL_Renderer* renderer, float speed, color_t r, color_t g, color_t b) = 0;
};

class Triangle : public Renderable
{
public:
    Triangle(float x, float y, float w, float h): Renderable(x, y, w, h) {}
    void render(SDL_Renderer* renderer, float speed, color_t r, color_t g, color_t b)
    {
        filledTrigonRGBA(renderer, x, y, x+w/2, y-h, x+w, y, r, g, b, 0xff);
        x = (x - speed);
        if (x+w < 0) x += WIDTH+w;   // TODO sketchy mod
    }
};

int main()
{
    std::vector<Triangle> layer;
    for (int i=0; i<40; ++i)
        //layer.emplace_back(300*i, 1000, 700+rng(-100, 100), 400+rng(-200, 200), 0x44, 0x33, 0x33);
        layer.emplace_back((float)300*i, (float)1000, (float)700+rng(-100, 100), (float)400+rng(-200, 200));
    for (int i=0; i<90; ++i)
        //layer.emplace_back(50*i, 1000, 70+rng(-20, 20), 120+rng(-20, 20), 0x05, 0x55, 0x05);
        layer.emplace_back((float)50*i, (float)1000, (float)70+rng(-20, 20), (float)120+rng(-20, 20));
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
        // draw the scene

        for (auto &tri : layer)
            tri.render(renderer, 0.1, 0x32, 0x6c, 0xcc);
        // draw sky
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

