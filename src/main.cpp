// NOTE: Much of this early code is taken from Lazy Foo's SDL tutorials at http://lazyfoo.net/tutorials/SDL/

#include <cstring>

#ifdef WIN32
#	include <SDL.h>
#	include <SDL_image.h>
#	include <SDL_ttf.h>
#	include <SDL_opengl.h>
#	undef main
#else
#	include <SDL2/SDL.h>
#	include <SDL2/SDL_image.h>
#	include <SDL2/SDL_ttf.h>
#	include <SDL2/SDL_opengl.h>
#endif

#include <physfs.h>

#include <cml/cml.h>

#include <csaru-core-cpp/csaru-core-cpp.hpp>
#include <csaru-uuid-cpp/csaru-uuid-cpp.hpp>
#include <csaru-datamap-cpp/csaru-datamap-cpp.hpp>
#include <csaru-json-cpp/csaru-json-cpp.hpp>
#include <csaru-game2dlib-cpp/csaru-game2dlib-cpp.hpp>

#include "SceneAlienTest.hpp"


static const int  s_screenWidth       = 1366;
static const int  s_screenHeight      = 768;
static const char s_testImageSource[] = "kenney/platformer_redux/spritesheet_ground.png";

static CSaruGame::Timer g_timer;

static CSaruGame::GameScene * g_scene = nullptr;

static unsigned         g_frameCounter = 0;
static SDL_Window *     g_window       = nullptr;
static SDL_GLContext    g_glContext    = nullptr;
static SDL_Renderer *   g_renderer     = nullptr;

static SDL_Rect s_viewportRects[] = {
	{                 0,                  0, s_screenWidth / 2, s_screenHeight / 2 },
	{ s_screenWidth / 2,                  0, s_screenWidth / 2, s_screenHeight / 2 },
	{                 0, s_screenHeight / 2, s_screenWidth,     s_screenHeight / 2 },
};


//======================================================================
bool init (int argc, char * argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL failed to initialize.  %s\n", SDL_GetError());
        return false;
    }

	g_timer.UpdateFrequency(); // uses SDL; must come after SDL_Init
	g_timer.Reset();
	g_timer.SetPaused(false);

	// Setup PhysicsFS
	{
		const int physicsFsInitResult = PHYSFS_init(argv[0]);
		SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "argv[0]: %s", argv[0]);
		if (!physicsFsInitResult) {
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, PHYSFS_getLastError());
			SDL_assert_release(physicsFsInitResult);
		}

		const int physicsFsConfigResult = PHYSFS_setSaneConfig(
			"codesaru"          /* organization */,
			"csaru-game2d1-cpp" /* application name */,
			"zip"               /* archive extension (case-insensitive) */,
			0                   /* include CD-ROM dirs */,
			1                   /* archives first */
		);
		if (!physicsFsConfigResult) {
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, PHYSFS_getLastError());
			SDL_assert_release(physicsFsConfigResult);
		}

		SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Base Dir: %s", PHYSFS_getBaseDir());
		SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "User Dir: %s", PHYSFS_getUserDir());
		SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Write Dir: %s", PHYSFS_getWriteDir());
	}

	// Prepare some flags before making the main window.
	SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 0); // 0: force software, 1: force hardware, neither: either
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	//SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);

	// Create main window.
    g_window = SDL_CreateWindow(
        "game2d1",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        s_screenWidth,
        s_screenHeight,
        SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL // | SDL_WINDOW_RESIZABLE
    );
    if (!g_window) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL failed to create a window.  %s\n", SDL_GetError());
        return false;
    }

	// Prepare OpenGL.
	g_glContext = SDL_GL_CreateContext(g_window);

	// Create renderer for main window.
	g_renderer = SDL_CreateRenderer(
		g_window,
		-1 /* rendering driver index; -1 use first available renderer */,
		SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
	);
	if (!g_renderer) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL failed to create renderer.  %s\n", SDL_GetError());
		return false;
	}

	// Set color used when clearing.
	SDL_SetRenderDrawColor(g_renderer, 0x34, 0xE3, 0xFF, 0xFF);

	// Initialize SDL_image extension.
	int imgFlags = IMG_INIT_PNG;
	if (!(IMG_Init(imgFlags) & imgFlags)) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_image failed to initialize.  %s\n", IMG_GetError());
		return false;
	}

	// Initialize SDL_ttf
	if (TTF_Init() == -1) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_ttf could not initialize!  %s\n", TTF_GetError());
		return false;
	}

	g_scene = new SceneAlienTest;
	g_scene->Init(argc, argv);

	// perf/timer testing
	g_timer.Advance();
	SDL_LogInfo(
		SDL_LOG_CATEGORY_APPLICATION,
		"game2d1 init took %u ticks (%u Ms)",
		g_timer.GetTicks(),
		g_timer.GetMs()
	);

    return true;
}


//======================================================================
bool loadMedia () {
	g_timer.Reset();

	bool success = g_scene->Load(g_renderer);

	g_timer.Advance();
	SDL_LogInfo(
		SDL_LOG_CATEGORY_APPLICATION,
		"game2d1 load took %u ticks (%u Ms)",
		g_timer.GetTicks(),
		g_timer.GetMs()
	);

    return success;
}


//======================================================================
void close () {
	g_scene->Unload();

	// Unload OpenGL.
	SDL_GL_DeleteContext(g_glContext);

	// Destroy window.
	SDL_DestroyRenderer(g_renderer);
    SDL_DestroyWindow(g_window);
    g_renderer = nullptr;
    g_window   = nullptr;

	TTF_Quit();
	IMG_Quit();

	const int physfsDeinitResult = PHYSFS_deinit();
	SDL_assert(physfsDeinitResult);

    SDL_Quit();
}


//======================================================================
int main (int argc, char ** argv) {
    unused(argc);
    unused(argv);

	g_scene = new SceneAlienTest();

    // initialize and load
    if (!init(argc, argv)) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to init.");
        return 1;
	}
    if (!loadMedia()) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to loadMedia. %s", SDL_GetError());
        return 1;
	}

	g_timer.Reset();

	SDL_Event e;
    while (!g_scene->ShouldQuit()) {
		while (SDL_PollEvent(&e))
			g_scene->OnSdlEvent(e);

		const float dt = g_timer.GetMsDelta() / 1000.0f;
		g_scene->Update(dt);
		g_scene->Render(g_renderer, s_screenWidth, s_screenHeight);

		SDL_RenderPresent(g_renderer);
		SDL_GL_SwapWindow(g_window);

		++g_frameCounter; // Just for debugging.
		g_timer.Advance();
    }

    // quit
    close();
    return 0;

}

