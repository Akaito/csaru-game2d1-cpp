#include <SDL.h>
#include <stdio.h>

#include <csaru-core-cpp.h>

static const int s_screenWidth = 640;
static const int s_screenHeight = 480;

int main (int argc, char ** argv) {

    ref(argc);
    ref(argv);

    if (const int sdl_code = SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL failed to initialize.  %s\n", SDL_GetError());
        return sdl_code;
    }

    SDL_Window * window = SDL_CreateWindow(
        "game2d1",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        s_screenWidth,
        s_screenHeight,
        SDL_WINDOW_SHOWN
    );
    if (!window) {
        printf("SDL failed to create a window.  %s\n", SDL_GetError());
        return 1;
    }

    SDL_Surface * surface = SDL_GetWindowSurface(window);

    SDL_FillRect(surface, nullptr /* rect */, SDL_MapRGB(surface->format, 0x00, 0x00, 0x3F));
    SDL_UpdateWindowSurface(window);

    SDL_Delay(2000);

    SDL_Quit();
    return 0;

}
