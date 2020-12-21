#include <SDL2/SDL.h>
#include <X11/Xlib.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <vector>
#include <chrono>
#include <thread>
#include <array>
#include <memory>
#include <iterator>
#include <iostream>
#include <algorithm>
// TODO: auto-get height https://stackoverflow.com/questions/33393528/how-to-get-screen-size-in-sdl

// config
const int WIDTH = 3000;
const int HEIGHT = 1920;

const auto FRAME_PERIOD = std::chrono::milliseconds(10);
const float SPED = 0.01*FRAME_PERIOD.count();

const int CENTER_X = 800;
const int CENTER_Y = 600;
const int RADIUS = 70;
const int LAYERS = 7;
const unsigned long SUNCOLOR = 0xee339900;
typedef std::array<unsigned char, 4> color_t;

const size_t NUM_LAYERS = 6;
const color_t layer_colors[NUM_LAYERS] = {
    { 50,  90,  240, 0xff },
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
    float speed, scale;
    color_t color;
    std::vector<std::unique_ptr<Renderable> > assets;
    // constructors
    Layer(): speed(0), scale(0), color({0xff, 0, 0, 0}), assets() {};
    Layer(float speed, float scale, color_t color): speed(speed), scale(scale), color(color) {}
    // methods
    void render(SDL_Renderer* renderer)
    {
        for (auto &asset : assets)
            asset->render(renderer, speed, color);
    }
};

int main()
{
    // construct procedural layers
    std::array<Layer, NUM_LAYERS> layers;
    for (int i=1; i<=NUM_LAYERS; ++i)
    {
        layers[i-1] = { SPED*(NUM_LAYERS-i+1), 1200, layer_colors[i-1] };
        const int spacing = 200 * i - 160;
        const int bottom = 1200;
        const int height = 90 * i;
        const int height_noise = 0*i;
        const int width = spacing * 1.30;
        const int width_noise = 20*i;
        for (int j=-5; j< WIDTH / spacing + 5; ++j)
            layers[i-1].assets.emplace_back(new Triangle(spacing*j, bottom, width+rng(-width_noise, width_noise), height+rng(-height_noise, height_noise)));
    }

    // initialize window
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
    SDL_EnableScreenSaver();    // https://stackoverflow.com/a/39917503

    // event loop
    for (SDL_Event event = {}; event.type != SDL_QUIT; SDL_PollEvent(&event))
    {
        // clear, I guess?
        SDL_SetRenderTarget(renderer, texture);
        SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0x00);
        SDL_RenderClear(renderer);
        // draw the scene
        for (auto it=layers.rbegin(); it != layers.rend(); ++it) it->render(renderer);
        // draw sky
        for (int i=0; i<LAYERS; ++i)
            filledCircleRGBA(renderer, CENTER_X, CENTER_Y, RADIUS*i, 0xff, 0xee, 0x20, 0x40*i/LAYERS);
        // draw to the screen?
        SDL_SetRenderTarget(renderer, NULL);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
        std::this_thread::sleep_for(FRAME_PERIOD);
    }

    // clean up
    SDL_DestroyRenderer(renderer);
    SDL_Quit();
}

