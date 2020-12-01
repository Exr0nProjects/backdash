#include <SDL2/SDL.h>
#include <X11/Xlib.h>

//struct Textures
//{
//    SDL_Texture** texture;
//    size_t size;
//};
//
//struct View
//{
//    int speed;
//    Textures textures;
//    SDL_Rect* rect;
//    View* next;
//};
//
//struct Video
//{
//    Display* x11d;
//    SDL_Window* window;
//    SDL_Renderer* renderer;
//};
//
//static Video Setup()
//{
//    Video self;
//    self.x11d = XOpenDisplay(NULL);
//    const Window x11w = RootWindow(self.x11d, DefaultScreen(self.x11d));
//    SDL_Init(SDL_INIT_VIDEO);
//    self.window = SDL_CreateWindowFrom((void*)x11w);
//    self.renderer = SDL_CreateRenderer(self.window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
//    return self;
//}
//
//int main()
//{
//    //Video video = Setup();
//    //auto display = XOpenDisplay(NULL);
//}

int main()
{

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
    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 1024, 768);



    SDL_Rect r;
    r.w = 200;
    r.h = 150;
    r.x = 100;
    r.y = 100;
    for (SDL_Event event = {}; event.type != SDL_QUIT; SDL_PollEvent(&event))
    {
        r.x = (r.x + 1) % 1080;
        // clear, I guess?
        SDL_SetRenderTarget(renderer, texture);
        SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0x00);
        SDL_RenderClear(renderer);
        // draw the rect
        SDL_RenderDrawRect(renderer,&r);
        SDL_SetRenderDrawColor(renderer, 0xFF, 0x00, 0x00, 0x00);
        SDL_RenderFillRect(renderer, &r);
        // draw to the screen?
        SDL_SetRenderTarget(renderer, NULL);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
    }
    SDL_DestroyRenderer(renderer);
    SDL_Quit();
}

