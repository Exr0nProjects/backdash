#include <SDL2/SDL.h>
#include <X11/Xlib.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <cassert>
#include <csignal>
#include <chrono>
#include <thread>
#include <deque>
#include <array>
#include <memory>
#include <iterator>
#include <iostream>
#include <algorithm>
#include <functional>
// TODO: auto-get height https://stackoverflow.com/questions/33393528/how-to-get-screen-size-in-sdl

// config
const int WIDTH = 3000;
const int HEIGHT = 1920;

const auto FRAME_PERIOD = std::chrono::milliseconds(10);
const double SPED = 0.05*FRAME_PERIOD.count();
//const double SPED = 0.01*FRAME_PERIOD.count();

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
    double x, y, w, h;
    // constructors
    Renderable(double x, double y, double w, double h): x(x), y(y), w(w), h(h) {}
    // methods
    virtual void render(SDL_Renderer* renderer, double speed, color_t color)
    {
        std::cerr << "Attempted to render empty renderable!" << std::endl;
    }
    bool completed() const { return x + w < -10; };
    bool started() const { return x < WIDTH + 10; };
};

class Triangle : public Renderable
{
public:
    // constructors
    Triangle(double x, double y, double w, double h): Renderable(x, y, w, h) {}
    // methods
    void render(SDL_Renderer* renderer, double speed, color_t color)
    {
        filledTrigonRGBA(renderer, x, y, x+w/2, y-h, x+w, y, color[0], color[1], color[2], color[3]);
        x = (x - speed);
    }
};

class Layer
{
public:
    double speed, bottom, scale, spacing;
    color_t color;
    std::function<Renderable*(double, double)> gen;
    std::deque<std::unique_ptr<Renderable> > assets;    // invariant: sorted by x
    // constructors
    Layer(double speed, double bottom, double scale, color_t color,
            double spacing, std::function<Renderable*(double, double)> generator):
        speed(speed), bottom(bottom), scale(scale), color(color),
        spacing(spacing), gen(generator) {}
    Layer(double speed, double bottom, double scale, color_t color):
        Layer(speed, bottom, scale, color, 0., nullptr) {}
    Layer(): Layer(0, 0, 0, {0xff, 0, 0, 0}) {};
    // methods
    void render(SDL_Renderer* renderer)
    {
        // pop queue
        while (assets.size() && assets.front()->completed()) assets.pop_front();
        // draw
        for (auto &asset : assets)
            asset->render(renderer, speed, color);
        // push queue
        if (gen)
            while (!assets.size() || assets.back()->started())
                assert(assets.size() < 1e6),    // TODO: this is unsafe
                assets.emplace_back(gen((assets.size()?assets.back()->x:-10) + spacing, bottom));
    }
};

void sigintHandler(int sig)
{
    printf("\nDying a horrible death..\n");
    exit(sig);
}

int main()
{
    signal(SIGINT, sigintHandler);  // otherwise this doesn't respond to C-c for some reason
    // construct procedural layers
    std::array<Layer, NUM_LAYERS> layers;
    for (int i=1; i<=NUM_LAYERS; ++i)
    {
        // set procedural constants
        const double spacing = 200 * i - 160;
        const double bottom = 1200;
        const int height = 90 * i;
        const int height_noise = 0*i;
        const int width = spacing * 1.30;
        const int width_noise = 20*i;
        layers[i-1] = Layer(SPED*(NUM_LAYERS-i+1), bottom, 1200., layer_colors[i-1], spacing,
            [=](double x, double y) -> Renderable* { return new Triangle(
                x, y, width+rng(-width_noise, width_noise), height+rng(-height_noise, height_noise)
            ); });
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

