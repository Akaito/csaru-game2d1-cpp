// NOTE: Much of this early code is taken from Lazy Foo's SDL tutorials at http://lazyfoo.net/tutorials/SDL/

#include <cstring>

#ifdef WIN32
#	include <SDL.h>
#	include <SDL_image.h>
#	include <SDL_ttf.h>
#	undef main
#else
#	include <SDL2/SDL.h>
#	include <SDL2/SDL_image.h>
#	include <SDL2/SDL_ttf.h>
#endif

#include <physfs.h>

#include <cml/cml.h>

#include <csaru-core-cpp/csaru-core-cpp.h>
#include <csaru-container-cpp/csaru-container-cpp.h>
#include <csaru-json-cpp/csaru-json-cpp.h>
#include <csaru-game2dlib-cpp/csaru-game2dlib-cpp.h>


static const int  s_screenWidth       = 1366;
static const int  s_screenHeight      = 768;
static const char s_testImageSource[] = "kenney/platformer_redux/spritesheet_ground.png";

static CSaruGame::Timer g_timer;

static unsigned         g_frameCounter = 0;
static SDL_Window *     g_window       = nullptr;
static SDL_Renderer *   g_renderer     = nullptr;
static TTF_Font *       g_font         = nullptr;
static CSaru2d::Texture g_textTexture;
static CSaru2d::Texture g_bgTexture;

enum Gobs {
	GOB_PLAIN = 0,
	GOB_COLOR_MOD,
	GOB_ROTATED,
	GOB_ROTATED_FLIP,
	GOB_ROTATING,
	GOB_ANIMATING,
	GOBS
};
static CSaruGame::Level g_level;

static SDL_Rect s_testRects[] = {
    {   0,   0, 128, 128 },
    { 128,   0, 128, 128 },
    { 256,   0, 128, 128 },
    {   0, 128, 128, 128 },
    { 128, 128, 128, 128 },
};
static unsigned g_testRectIndex = 0;

static CSaru2d::TextureAnimation g_yellowAlienWalkAnim;

static SDL_Rect s_viewportRects[] = {
	{                 0,                  0, s_screenWidth / 2, s_screenHeight / 2 },
	{ s_screenWidth / 2,                  0, s_screenWidth / 2, s_screenHeight / 2 },
	{                 0, s_screenHeight / 2, s_screenWidth,     s_screenHeight / 2 },
};


void LoadLevelStuff (const char * filepath) {
	CSaruJson::JsonParser jsonParser;
	PHYSFS_File *         levelFileFs = PHYSFS_openRead(filepath);
	if (!levelFileFs) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, PHYSFS_getLastError());
		SDL_assert(levelFileFs);
		return;
	}

	CSaruContainer::DataMap                 levelDm;
	CSaruJson::JsonParserCallbackForDataMap levelParserCallback(levelDm.GetMutator());

	const std::size_t bufObjCount = CSaruCore::GetSystemPageSize();
	char *            buf         = new char[bufObjCount];
	while (!PHYSFS_eof(levelFileFs)) {
		PHYSFS_sint64 physFsReadResult = PHYSFS_read(
			levelFileFs,
			buf,
			sizeof(char),
			bufObjCount
		);
		// Handle errors reading file.
		if (physFsReadResult < bufObjCount && !PHYSFS_eof(levelFileFs)) {
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, PHYSFS_getLastError());
			SDL_assert_release(physFsReadResult);

			delete [] buf;
			PHYSFS_close(levelFileFs);
			return;
		}

		jsonParser.ParseBuffer(buf, bufObjCount, &levelParserCallback);
	}

	// Load level assets.
	CSaruContainer::DataMapReader levelReader = levelDm.GetReader();
	g_level.AcceptDataMap(levelReader);

	delete [] buf;
	if (levelFileFs)
		PHYSFS_close(levelFileFs);

	for (unsigned i = 0; i < GOBS; ++i) {
		SDL_assert_release(g_level.GetGameObject(i));
	}
}


bool init (const char * argv0) {

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL failed to initialize.  %s\n", SDL_GetError());
        return false;
    }

	g_timer.UpdateFrequency(); // uses SDL; must come after SDL_Init
	g_timer.Reset();
	g_timer.SetPaused(false);

	// Setup PhysicsFS
	{
		const int physicsFsInitResult = PHYSFS_init(argv0);
		SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "argv0: %s", argv0);
		if (!physicsFsInitResult) {
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, PHYSFS_getLastError());
			SDL_assert_release(physicsFsInitResult);
		}

		const int physicsFsConfigResult = PHYSFS_setSaneConfig(
			"codesaru" /* organization */,
			argv0      /* application name */,
			"zip"      /* archive extension (case-insensitive) */,
			0          /* include CD-ROM dirs */,
			1          /* archives first */
		);
		if (!physicsFsConfigResult) {
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, PHYSFS_getLastError());
			SDL_assert_release(physicsFsConfigResult);
		}

		SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Base Dir: %s", PHYSFS_getBaseDir());
		SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "User Dir: %s", PHYSFS_getUserDir());
		SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Write Dir: %s", PHYSFS_getWriteDir());

		SDL_assert_release(PHYSFS_exists("testImage.png"));
		//SDL_assert_release(PHYSFS_exists("bogus"));
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
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL failed to create a window.  %s\n", SDL_GetError());
        return false;
    }

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
	SDL_SetRenderDrawColor(g_renderer, 0x3F, 0x00, 0x3F, 0xFF);

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

	// Prepare "level"
	//LoadLevelStuff("levels/test/03_Assets.json");
	LoadLevelStuff("levels/test/05_GameObjects.json");
	LoadLevelStuff("levels/test/07_GameObjectComponents.json");

	// animating
	{
		CSaruGame::GameObject * gob = g_level.GetGameObject(GOB_ANIMATING);
		SDL_assert_release(gob);
		gob->AddComponent(new CSaruGame::GocSrcRectAnimator(1));

		auto rectAnimator = gob->GetGoc<CSaruGame::GocSrcRectAnimator>();
		rectAnimator->SetTargetRect(
			&gob->GetGoc<CSaruGame::GocSpriteSimple>()->GetSrcRect()
		);
		rectAnimator->SetAnimation(&g_yellowAlienWalkAnim);
	}

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


bool loadMedia () {

	g_timer.Reset();

	if (!g_bgTexture.LoadFromFile(g_renderer, "testImage.png", false, 0x00, 0x00, 0x00)) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load bg image!\n");
		return false;
	}

	// load GocSpriteSimple textures
	for (unsigned i = GOB_PLAIN; i < GOBS; ++i) {
		CSaruGame::GameObject * gob = g_level.GetGameObject(i);
		SDL_assert_release(gob);
		auto * spriteGoc = gob->GetGoc<CSaruGame::GocSpriteSimple>();
		SDL_assert_release(spriteGoc);
		if (!spriteGoc->LoadTextureFromFile(g_renderer, "kenney/platformer_redux/spritesheet_players.png"))
			return false;

		switch (i) {
			case GOB_COLOR_MOD: {
				spriteGoc->GetTexture()->SetColor(0xFF, 0x00, 0x00);
			} break;
		}
	}

	// Open the font
	{
		g_font = TTF_OpenFont("ubuntu-font-family/UbuntuMono-B.ttf", 28);
		if (!g_font) {
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load ttf font!  SDL_ttf Error: %s\n", TTF_GetError());
			return false;
		}

		// Render text
		SDL_Color textColor = { 255, 255, 255 };
		if (!g_textTexture.LoadFromRenderedText(g_renderer, g_font, "The quick brown fox jumps over the lazy dog", textColor)) {
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to render text texture!\n");
			return false;
		}
	}

	// setup animation(s)
	g_yellowAlienWalkAnim.SetSrcRect(0, SDL_Rect{ 128, 1024, 128, 256 });
	g_yellowAlienWalkAnim.SetSrcRect(1, SDL_Rect{ 128,  768, 128, 256 });
	g_yellowAlienWalkAnim.SetFrameDuration(0, 250);
	g_yellowAlienWalkAnim.SetFrameDuration(1, 250);

	g_timer.Advance();
	SDL_LogInfo(
		SDL_LOG_CATEGORY_APPLICATION,
		"game2d1 load took %u ticks (%u Ms)",
		g_timer.GetTicks(),
		g_timer.GetMs()
	);

    return true;

}


void close () {

	// Free loaded data.
	g_textTexture.Free();
	g_bgTexture.Free();

	// Free font data.
	TTF_CloseFont(g_font);
	g_font = nullptr;

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


int main (int argc, char ** argv) {

    unused(argc);
    unused(argv);

    // initialize and load
    if (!init(argv[0]))
        return 1;
    if (!loadMedia())
        return 1;

    bool readyToQuit = false;
    SDL_Event e;

	g_timer.Reset();

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

					case SDLK_p: {
						for (unsigned i = GOB_PLAIN; i < GOBS; ++i) {
							auto trans = g_level.GetGameObject(i)->GetTransform();
							SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Pos: %f, %f", trans.GetPosition()[0], trans.GetPosition()[1]);
						}
					} break;

					case SDLK_w: {
						for (unsigned i = GOB_PLAIN; i < GOBS; ++i) {
							g_level.GetGameObject(i)->GetGoc<CSaruGame::GocSpriteSimple>()->GetTexture()->SetAlpha(0xFF);
						}
					} break;

					case SDLK_s: {
						for (unsigned i = GOB_PLAIN; i < GOBS; ++i) {
							g_level.GetGameObject(i)->GetGoc<CSaruGame::GocSpriteSimple>()->GetTexture()->SetAlpha(0x7F);
						}
					} break;

                    default:         g_testRectIndex = 0; break;
                }
            }
        }

		// Update all game objects.
		const float dt = g_timer.GetMsDelta() / 1000.0f;
		for (unsigned i = 0; i < GOBS; ++i) {
			g_level.GetGameObject(i)->Update(dt);
		}

		// Clear the renderer to prepare for drawing.
		SDL_SetRenderDrawColor(g_renderer, 0x3F, 0x00, 0x3F, 0xFF);
		SDL_RenderClear(g_renderer);

#if 0
		for (unsigned viewportIndex = 0;  viewportIndex < arrsize(s_viewportRects);  ++viewportIndex) {
			SDL_RenderSetViewport(g_renderer, &s_viewportRects[viewportIndex]);
			// Draw Texture.
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

			// Draw primitive.
			SDL_Rect testPrimitiveRect = { s_screenWidth / 8 * 6, s_screenHeight / 8 * 6, s_screenWidth / 8, s_screenHeight / 8 };
			SDL_SetRenderDrawColor(g_renderer, 0x00, 0x40, 0xFF, 0xFF);
			SDL_RenderFillRect(g_renderer, &testPrimitiveRect);
			// Draw outline primitive.
			testPrimitiveRect.x -= 50;
			testPrimitiveRect.y -= 50;
			testPrimitiveRect.w += 50;
			testPrimitiveRect.h += 50;
			SDL_SetRenderDrawColor(g_renderer, 0x00, 0xFF, 0xFF, 0x40);
			SDL_RenderDrawRect(g_renderer, &testPrimitiveRect);
			// Draw line.
			SDL_SetRenderDrawColor(g_renderer, 0x00, 0xFF, 0x40, 0xFF);
			SDL_RenderDrawLine(g_renderer, s_screenWidth / 4, s_screenHeight / 2, s_screenWidth / 4 * 3, s_screenHeight / 2);
			// Draw points.
			SDL_SetRenderDrawColor(g_renderer, 0xFF, 0x00, 0xFF, 0xFF);
			for (unsigned i = s_screenHeight / 4;  i < s_screenHeight / 4 * 3;  i += 10) {
				SDL_RenderDrawPoint(g_renderer, s_screenWidth / 2, i);
			}
		} // end for each viewport
#endif

		// Render prep
		SDL_RenderSetViewport(g_renderer, nullptr);
		g_bgTexture.Render(
			g_renderer,
			s_screenWidth  / 2 - g_bgTexture.GetWidth() / 2,
			s_screenHeight / 2 - g_bgTexture.GetHeight() / 2
		);
		// Test src clip rect in TextureWrapper class.
		SDL_Rect srcRect = { 512, 1280, 128, 256 };

		// Render game objects
		for (unsigned i = 0; i < GOBS; ++i)
			g_level.GetGameObject(i)->Render();

		// Test text rendering.
		g_textTexture.Render(
			g_renderer,
			(s_screenWidth  - g_textTexture.GetWidth())  / 2,
			(s_screenHeight - g_textTexture.GetHeight()) / 2
		);

		SDL_RenderPresent(g_renderer);

		++g_frameCounter;
		g_timer.Advance();
    }

    // wait
    //SDL_Delay(2000);

    // quit
    close();
    return 0;

}

