// NOTE: Much of this early code is taken from Lazy Foo's SDL tutorials at http://lazyfoo.net/tutorials/SDL/

#include <SDL.h>
#include <SDL_image.h>
#include <stdio.h>

#include <csaru-core-cpp.h>

static const int  s_screenWidth       = 640;
static const int  s_screenHeight      = 480;
static const char s_testImageSource[] = "kenney/platformer_redux/spritesheet_ground.png";

static SDL_Window * g_window     = nullptr;
static SDL_Surface * g_windowSurface   = nullptr;
static SDL_Surface * g_testImage = nullptr;


bool init () {

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL failed to initialize.  %s\n", SDL_GetError());
        return false;
    }

    g_window = SDL_CreateWindow(
        "game2d1",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        s_screenWidth,
        s_screenHeight,
        SDL_WINDOW_SHOWN
    );
    if (!g_window) {
        printf("SDL failed to create a window.  %s\n", SDL_GetError());
        return false;
    }

    g_windowSurface = SDL_GetWindowSurface(g_window);

    return true;

}


bool loadMedia () {

    g_testImage = IMG_Load(s_testImageSource);
    if (!g_testImage) {
        printf("Failed to load image at {%s}.  %s\n", s_testImageSource, SDL_GetError());
        return false;
    }

    return true;

}


void close () {

    SDL_FreeSurface(g_testImage);
    g_testImage = nullptr;

    SDL_DestroyWindow(g_window);
    g_windowSurface = nullptr;
    g_window        = nullptr;

    SDL_Quit();

}


int main (int argc, char ** argv) {

    ref(argc);
    ref(argv);

    if (!init())
        return 1;
    if (!loadMedia())
        return 1;

    SDL_FillRect(g_windowSurface, nullptr /* rect */, SDL_MapRGB(g_windowSurface->format, 0x00, 0x00, 0x3F));
    SDL_BlitSurface(g_testImage, nullptr /* srcRect */, g_windowSurface, nullptr /* destRect */);
    SDL_UpdateWindowSurface(g_window);

    SDL_Delay(2000);

    close();
    return 0;

}
