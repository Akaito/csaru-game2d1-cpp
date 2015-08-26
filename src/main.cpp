// NOTE: Much of this early code is taken from Lazy Foo's SDL tutorials at http://lazyfoo.net/tutorials/SDL/

#include <SDL.h>
#include <SDL_image.h>
#include <stdio.h>

#include <csaru-core-cpp.h>

static const int  s_screenWidth       = 640;
static const int  s_screenHeight      = 480;
static const char s_testImageSource[] = "kenney/platformer_redux/spritesheet_ground.png";

static SDL_Window *   g_window        = nullptr;
static SDL_Renderer * g_renderer      = nullptr;
static SDL_Texture *  g_testTexture   = nullptr;

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

	// Create main window.
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

	// Create renderer for main window.
	g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_ACCELERATED);
	if (!g_renderer) {
		printf("SDL failed to create renderer.  %s\n", SDL_GetError());
		return false;
	}

	// Set color used when clearing.
	SDL_SetRenderDrawColor(g_renderer, 0x3F, 0x00, 0x3F, 0xFF);

	// Initialize SDL_image extension.
	int imgFlags = IMG_INIT_PNG;
	if (!(IMG_Init(imgFlags) & imgFlags)) {
		printf("SDL_image failed to initialize.  %s\n", IMG_GetError());
		return false;
	}

    return true;

}


SDL_Texture * loadTexture (const char * path) {

	SDL_Texture * result      = nullptr;
	SDL_Surface * tempSurface = IMG_Load(path);
	if (!tempSurface) {
		printf("SDL_image failed to load {%s}.  %s\n", path, IMG_GetError());
		return nullptr;
	}

	result = SDL_CreateTextureFromSurface(g_renderer, tempSurface);
	if (!result) {
		printf("SDL failed to create a texture from surface for {%s}.  %s\n", path, SDL_GetError());
		SDL_FreeSurface(tempSurface);
		return nullptr;
	}

	return result;

}


bool loadMedia () {

	g_testTexture = loadTexture(s_testImageSource);
	if (!g_testTexture) {
		printf("Failed to load texture.\n");
		return false;
	}

    return true;

}


void close () {

	// Free loaded data.
    SDL_DestroyTexture(g_testTexture);
    g_testTexture = nullptr;

	// Destroy window.
	SDL_DestroyRenderer(g_renderer);
    SDL_DestroyWindow(g_window);
    g_renderer = nullptr;
    g_window   = nullptr;

	IMG_Quit();
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
		SDL_RenderClear(g_renderer);
		SDL_RenderCopy(
			g_renderer,
			g_testTexture,
			&s_testRects[g_testRectIndex] /* srcRect */,
			nullptr /* destRect */
		);
		SDL_RenderCopy(
			g_renderer,
			g_testTexture,
			&s_testRects[g_testRectIndex] /* srcRect */,
			&s_testRects[0] /* destRect */
		);
		SDL_RenderPresent(g_renderer);
    }

    // wait
    //SDL_Delay(2000);

    // quit
    close();
    return 0;

}
