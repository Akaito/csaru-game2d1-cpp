// NOTE: Much of this early code is taken from Lazy Foo's SDL tutorials at http://lazyfoo.net/tutorials/SDL/

#include <SDL.h>
#include <SDL_image.h>
#include <stdio.h>

#include <csaru-core-cpp.h>

static const int  s_screenWidth       = 640;
static const int  s_screenHeight      = 480;
static const char s_testImageSource[] = "kenney/platformer_redux/spritesheet_ground.png";

static SDL_Window *  g_window        = nullptr;
static SDL_Surface * g_windowSurface = nullptr;
static SDL_Surface * g_testImage     = nullptr;

static SDL_Rect s_testRects[] = {
    {   0,   0, 128, 128 },
    { 128,   0, 128, 128 },
    { 256,   0, 128, 128 },
    {   0, 128, 128, 128 },
    { 128, 128, 128, 128 },
};
static unsigned g_testRectIndex = 0;


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

    SDL_Surface * tempSurface = nullptr;

    tempSurface = IMG_Load(s_testImageSource);
    if (!tempSurface) {
        printf("Failed to load image at {%s}.  %s\n", s_testImageSource, SDL_GetError());
        return false;
    }
    //g_testImage = tempSurface;
    //return true;

    g_testImage = SDL_ConvertSurface(tempSurface, g_windowSurface->format, 0 /* flags */);
    if (!g_testImage) {
        printf("Failed to convert and optimize loaded image format from {%s}.  %s\n", s_testImageSource, SDL_GetError());
        return false;
    }

    SDL_FreeSurface(tempSurface);
    tempSurface = nullptr;

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

    // initialize and load
    if (!init())
        return 1;
    if (!loadMedia())
        return 1;

    bool readyToQuit = false;
    SDL_Event e;

    while (!readyToQuit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                readyToQuit = true;
            }
            else if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                    case SDLK_ESCAPE: readyToQuit = true; break;

                    case SDLK_UP:    g_testRectIndex = 1; break;
                    case SDLK_DOWN:  g_testRectIndex = 2; break;
                    case SDLK_LEFT:  g_testRectIndex = 3; break;
                    case SDLK_RIGHT: g_testRectIndex = 4; break;

                    default:         g_testRectIndex = 0; break;
                }
            }
        }

        // draw
        SDL_FillRect(g_windowSurface, nullptr /* rect */, SDL_MapRGB(g_windowSurface->format, 0x00, 0x00, 0x3F));
        SDL_Rect windRect = {0, 0, s_screenWidth, s_screenHeight};
        SDL_BlitScaled(g_testImage, &s_testRects[g_testRectIndex], g_windowSurface, &windRect);
        SDL_BlitSurface(g_testImage, &s_testRects[g_testRectIndex], g_windowSurface, nullptr /* destRect */);
        SDL_UpdateWindowSurface(g_window);
    }

    // wait
    //SDL_Delay(2000);

    // quit
    close();
    return 0;

}
