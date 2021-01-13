#include <SDL2/SDL.h>
#include <X11/Xlib.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <sys/ioctl.h>
#include <unistd.h> // https://stackoverflow.com/a/23369919
#include "deps/simplexnoise.h"
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
// TODO: smooth generation breaks when display turned off, etc

// config
const int WIDTH = 3000; const int HEIGHT = 1920;
//const int WIDTH = 3840; const int HEIGHT = 1080;

const auto FRAME_PERIOD = std::chrono::milliseconds(10);
const double SPED = 0.01*FRAME_PERIOD.count();
//const double SPED = 0.3*FRAME_PERIOD.count();
const double NOISE_TIME_SCALE = 0.0002*SPED;
const double TERRAIN_AMPLITUDE = 40;
const double TERRAIN_DEVIATION = 50;

const int CENTER_X = 800;
const int CENTER_Y = 600;
const int RADIUS = 70;
const int LAYERS = 7;
const unsigned long SUNCOLOR = 0xee339900;
typedef std::array<unsigned char, 4> color_t;

const size_t NUM_LAYERS = 6;
const size_t NUM_BG_COLORS = 2;
const color_t COLORS[] = {
    { 50,  90,  240, 0xff },    // foreground layers
    { 97,  155, 255, 0xff },
    { 109, 178, 255, 0xff },
    { 128, 187, 255, 0xff },
    { 145, 213, 255, 0xff },
    { 173, 219, 253, 0xff },
    {  64,  87, 250, 0xff },    // background gradient top
    { 200, 109, 202, 0xff },    // background gradient bot
    {0xff,0xee,0x20, 0xff }     // sun
};

const auto SHIFT = std::chrono::seconds(1577865600); // 50 years
int rng(int l, int r)
{ return rand() % (r-l+1) + l; }

inline unsigned collapse_color(const color_t &c)
{ return c[0] << 24 | c[1] << 16 | c[2] << 8 | c[3]; }

static SDL_Renderer * renderer;

class Renderable
{
public:
    double x, y, w, h;
    // constructors
    Renderable(double x, double y, double w, double h): x(x), y(y), w(w), h(h) {}
    // methods
    virtual void render(double speed, const color_t& color)
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
    void render(double speed, const color_t& color)
    {
        filledTrigonRGBA(renderer, x, y, x+w/2, y-h, x+w, y, color[0], color[1], color[2], color[3]);
        boxRGBA(renderer, x, y, x+w, HEIGHT, color[0], color[1], color[2], color[3]);
        x = (x - speed);
    }
};

class PolyTerrain : public Renderable
{
	double h1, h2;
public:
	PolyTerrain(double x, double y, double w, double h1, double h2): Renderable(x, y, w, std::max(h1, h2)), h1(h1), h2(h2)
	{
		if (h1 > h2) std::swap(h1, h2); // maintain h1 < h2
	}
	void render(double speed, const color_t& color)
	{
		filledTrigonRGBA(renderer, x, y-h1, x, y-h2, x+w, y-h1, color[0], color[1], color[2], color[3]);
		boxRGBA(renderer, x, y-h1, x+w, HEIGHT, color[0], color[1], color[2], color[3]);
	}
};

class Layer
{
public:
    double speed, bottom, scale, spacing;
    color_t color;
    std::function<Renderable*(double, double, double)> gen;
    std::deque<std::unique_ptr<Renderable> > assets;    // invariant: sorted by x
    // constructors
    Layer(double speed, double bottom, double scale, color_t color,
            double spacing, std::function<Renderable*(double, double, double)> generator):
        speed(speed), bottom(bottom), scale(scale), color(color),
        spacing(spacing), gen(generator) {}
    Layer(double speed, double bottom, double scale, color_t color):
        Layer(speed, bottom, scale, color, 0., nullptr) {}
    Layer(): Layer(0, 0, 0, {0xff, 0, 0, 0}) {};
    // methods
    void render(double time)
    {
        // pop queue
        while (assets.size() && assets.front()->completed()) assets.pop_front();
        // draw
        for (auto &asset : assets)
            asset->render(speed, color);
        // push queue
        if (gen)
            while (!assets.size() || assets.back()->started())
                assets.emplace_back(gen(time, (assets.size()?assets.back()->x:-10) + spacing, bottom));
    }
};


void renderCelestialBody(double x, double y)
{
    for (int i=0; i<LAYERS; ++i)
        filledCircleRGBA(renderer, CENTER_X, CENTER_Y, RADIUS*i,
                COLORS[NUM_LAYERS+NUM_BG_COLORS][0],
                COLORS[NUM_LAYERS+NUM_BG_COLORS][1],
                COLORS[NUM_LAYERS+NUM_BG_COLORS][2], 0x30*i/LAYERS);
    filledCircleRGBA(renderer, CENTER_X, CENTER_Y, RADIUS/3, 0xff, 0xff, 0xdd, 0xff);
}

void renderCelestialBodyByTime()
{
    //using namespace std::chrono; // https://stackoverflow.com/a/52895441
    //std::cout << std::chrono::current_zone()->get_info(system_clock::now()) << std::endl;

    renderCelestialBody(CENTER_X, CENTER_Y);    // TODO: day night cycle, move with the time
}

void quit(int sig)
{
    SDL_DestroyRenderer(renderer);
    SDL_Quit();
    exit(sig);
}

int main()
{
    signal(SIGINT, quit);  // otherwise this doesn't respond to C-c for some reason

    // construct procedural layers
    std::array<Layer, NUM_LAYERS> layers;
    for (int i=1; i<=NUM_LAYERS; ++i)
    {
        // set procedural constants
        const double spacing = 220 * i - 190;
        const double bottom = 1400-40*i;
        const int height = 100 * i;
        const int height_noise = 0*i;
        const int width = spacing * 1.50;
        const int width_noise = 10*i;
        if (i < 4 || i == NUM_LAYERS)
			layers[i-1] = Layer(SPED*(NUM_LAYERS-i+1), bottom, 0., COLORS[i-1], spacing,
				[=](double time, double x, double y) -> Renderable* { return new Triangle(
					x, y+SNoise::noise(time)*(TERRAIN_AMPLITUDE+TERRAIN_DEVIATION*(NUM_LAYERS-i)), width+rng(-width_noise, width_noise), height+rng(-height_noise, height_noise)
				); });
        else
			layers[i-1] = Layer(SPED*(NUM_LAYERS-i+1), bottom, 0., COLORS[i-1], spacing,
				[=](double time, double x, double y) -> Renderable* { return new PolyTerrain(
					x, y+SNoise::noise(time)*(TERRAIN_AMPLITUDE+TERRAIN_DEVIATION*(NUM_LAYERS-i)), spacing, height
				); });
    }

    // init window
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s", SDL_GetError());
        return 3;
    }
    const auto x11d = XOpenDisplay(NULL);
    const Window x11w = RootWindow(x11d, DefaultScreen(x11d));
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *window = SDL_CreateWindowFrom((void*)x11w);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);    // global so clean up func can access it
    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, WIDTH, HEIGHT);
    SDL_EnableScreenSaver();                            // allow display to sleep while running: https://stackoverflow.com/a/39917503

    // init background texture
    const SDL_Rect center = {1, 1, 1, NUM_BG_COLORS};   // the center mask of the background to display
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY,"1");     // low res texture based gradient: https://stackoverflow.com/a/42234816
    SDL_Texture * background = SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGBA8888,
    SDL_TEXTUREACCESS_STREAMING,3,NUM_BG_COLORS+2);
    {
        int bg_byte_width = 4;
        uint32_t * bgpixels;
        SDL_LockTexture(background,NULL,(void**)(&bgpixels),&bg_byte_width);
        for (int i=0; i<3; ++i) bgpixels[i] = collapse_color(COLORS[NUM_LAYERS]),               // fill top band
            bgpixels[3*NUM_BG_COLORS+i] = collapse_color(COLORS[NUM_LAYERS+NUM_BG_COLORS-1]);   // fill bot band
        for (int j=0; j<NUM_BG_COLORS; ++j) for (int i=0; i<3; ++i)                             // fill center (visible area + sides)
            bgpixels[(j+1)*3+i] = collapse_color(COLORS[NUM_LAYERS+j]);
        SDL_UnlockTexture(background);
    }

    // event loop
    float i=0;
    for (SDL_Event event = {}; event.type != SDL_QUIT; SDL_PollEvent(&event))
    {
        // calculate current time synced noise 'location'
        const auto now = (std::chrono::system_clock::now()-SHIFT).time_since_epoch();
        const auto shift = std::chrono::duration_cast<std::chrono::milliseconds>(now);
        const double cur = (double)(shift.count())*NOISE_TIME_SCALE;

        // draw background gradient to clear screen: https://stackoverflow.com/a/42234816
        SDL_SetRenderTarget(renderer, texture);
        SDL_RenderCopy(renderer, background, &center, NULL);

        // draw dynamic elements
        renderCelestialBodyByTime();
        for (auto it=layers.rbegin(); it != layers.rend(); ++it)
            it->render(cur);

        // draw to the screen
        SDL_SetRenderTarget(renderer, NULL);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
        std::this_thread::sleep_for(FRAME_PERIOD);
    }

    // clean up
    quit(0);
}
