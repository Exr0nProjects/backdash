#include <SDL2/SDL.h>
#include <X11/Xlib.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <vector>
#include <chrono>
#include <thread>
#include <array>
#include <memory>
#include <iostream>
#include <algorithm>

// TODO: auto-get height https://stackoverflow.com/questions/33393528/how-to-get-screen-size-in-sdl
const int WIDTH = 3000;
const int HEIGHT = 1920;

const int CENTER_X = 800;
const int CENTER_Y = 600;
const int RADIUS = 70;
const int LAYERS = 7;
const unsigned long SUNCOLOR = 0xee339900;
typedef std::array<unsigned char, 4> color_t;

const size_t NUM_LAYERS = 6;
const color_t layer_colors[NUM_LAYERS] = {
    { 33,  88,  197, 0xff },
    { 97,  155, 255, 0xff },
    { 109, 178, 255, 0xff },
    { 128, 187, 255, 0xff },
    { 145, 213, 255, 0xff },
    { 173, 219, 253, 0xff },
};

int rng(int l, int r)
{ return rand() % (r-l+1) + l; }

class Renderable
{
public:
    float x, y, w, h;
    // constructors
    Renderable(float x, float y, float w, float h): x(x), y(y), w(w), h(h) {}
    // methods
    virtual void render(SDL_Renderer* renderer, float speed, color_t color) = 0;
};

class Triangle : public Renderable
{
public:
    // constructors
    Triangle(float x, float y, float w, float h): Renderable(x, y, w, h) {}
    // methods
    void render(SDL_Renderer* renderer, float speed, color_t color)
    {
        filledTrigonRGBA(renderer, x, y, x+w/2, y-h, x+w, y, color[0], color[1], color[2], color[3]);
        x = (x - speed);
        if (x+w < 0) x += WIDTH+w;   // TODO sketchy mod
    }
};

class Layer
{
public:
    float speed, ypos;
    color_t color;
    std::vector<std::unique_ptr<Renderable> > assets;
    // constructors
    Layer(): speed(0), ypos(0), color({0xff, 0, 0, 0}), assets() {};
    Layer(float speed, float ypos, color_t color): speed(speed), ypos(ypos), color(color) {}
    // methods
    void render(SDL_Renderer* renderer)
    {
        for (auto &asset : assets)
            asset->render(renderer, speed, color);
    }
};

int main()
{
    //std::array<Layer, NUM_LAYERS> layers = {{ {0.1, 1000, {0x44, 0x33, 0x33, 0xff} }, { 0.4, 1000, {0x05, 0x55, 0x05, 0xff} } }};
    std::array<Layer, NUM_LAYERS> layers;
    for (int i=0; i<NUM_LAYERS; ++i)
        layers[i] = { 0.1*(i+1), 800, layer_colors[i] };
    for (int i=0; i<40; ++i)
        layers[0].assets.emplace_back(new Triangle(300*i, 1200, 700+rng(-100, 100), 400+rng(-200, 200)));
        //layers[0].assets.push_back(std::make_unique<Triangle>(100*i, 1200, 700+rng(-100, 100), 400+rng(-200, 200)) );
    for (int i=0; i<90; ++i)
        layers[1].assets.emplace_back( new Triangle(50*i, 1200, 70+rng(-20, 20), 120+rng(-20, 20)) );

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
        for (auto &layer : layers) layer.render(renderer);
        //for (auto &tri : layer)
        //    tri.render(renderer, 0.1, 0x32, 0x6c, 0xcc);
        // draw sky
        for (int i=0; i<LAYERS; ++i)
            filledCircleRGBA(renderer, CENTER_X, CENTER_Y, RADIUS*i, 0xff, 0xee, 0x20, 0x40*i/LAYERS);
        // draw to the screen?
        SDL_SetRenderTarget(renderer, NULL);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(100ms);
    }
    SDL_DestroyRenderer(renderer);
    SDL_Quit();
}

