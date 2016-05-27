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
static CSaruGame::GameObject * g_gobs[GOBS];

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


bool init () {

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL failed to initialize.  %s\n", SDL_GetError());
        return false;
    }

	g_timer.UpdateFrequency();
	g_timer.Reset();
	g_timer.SetPaused(false);

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

	// Prepare gobs
	for (unsigned i = 0; i < GOBS; ++i) {
		g_gobs[i] = new CSaruGame::GameObject;
	}

	// Prepare "level"
	{
		CSaruJson::JsonParser jsonParser;
		std::FILE *           levelFile = std::fopen("levels/test.json", "rt");
		SDL_assert_release(levelFile);

		CSaruContainer::DataMap                 levelDm;
		CSaruJson::JsonParserCallbackForDataMap levelParserCallback(levelDm.GetMutator());

		if (!jsonParser.ParseEntireFile(
			levelFile,
			nullptr /* parser uses its own buffer */,
			0       /* size of buffer we gave it */,
			&levelParserCallback
		)) {
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load level json!\n");
		}

		CSaruContainer::DataMapReader levelReader = levelDm.GetReader();
		// root -> data
		levelReader.ToFirstChild();
		SDL_assert_release(!std::strcmp(levelReader.ReadName(), "data"));
		if (!std::strcmp(levelReader.ReadName(), "data")) {
			CSaruContainer::DataMapReaderSimple simpleReader(levelReader);
			// data -> array of data objects or first data object
			simpleReader.ToFirstChild();

			// for each data object
			for (; simpleReader.IsValid(); simpleReader.ToNextSibling()) {
				// ignore if it's not a GameObject
				if (simpleReader.String("type") != "GameObject")
					continue;

				int id = simpleReader.Int("id");

				simpleReader.ToChild("attributes");
				if (simpleReader.ToChild("transform")) {
					if (simpleReader.EnterArray("position")) {
						CSaruContainer::DataMapReader posReader = simpleReader.GetReader();
						int x = posReader.ReadIntWalk();
						int y = posReader.ReadIntWalk();
						int z = posReader.ReadIntWalk();
						//SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "pos: |%d, %d, %d|", x, y, z);
						g_gobs[id]->GetTransform().SetPosition(x, y, z);
						simpleReader.ExitArray();
					}

					float rot = simpleReader.Float("rotation", 0.0f);
					if (rot != 0.0f) {
						g_gobs[id]->GetTransform().SetRotation(rot);
					}

					simpleReader.ToParent();
				}

				if (simpleReader.ToChild("components")) {
					if (simpleReader.EnterArray("array")) {
						while (simpleReader.IsValid()) {
							// (now on an unnamed component object)
							if (simpleReader.String("module_name") == "CSaruBase") {
								std::string typeName = simpleReader.String("type_name");
								if (typeName == "GocSpriteSimple") {
									static unsigned s_GocSpriteSimpleId = 1;
									g_gobs[id]->AddComponent(new CSaruGame::GocSpriteSimple(s_GocSpriteSimpleId++));
									if (simpleReader.ToChild("properties")) {
										auto goc = g_gobs[id]->GetGoc<CSaruGame::GocSpriteSimple>();
										// src_rect
										{
											if (simpleReader.EnterArray("src_rect")) {
												CSaruContainer::DataMapReader posReader = simpleReader.GetReader();
												int x = posReader.ReadIntWalk();
												int y = posReader.ReadIntWalk();
												int w = posReader.ReadIntWalk();
												int h = posReader.ReadIntWalk();
												goc->GetSrcRect() = SDL_Rect{x, y, w, h};
												simpleReader.ExitArray();
											}
											else
												simpleReader.ToParent();
										}

										// flip
										{
											std::string flip = simpleReader.String("flip", "none");
											if (flip == "horizontal")
												goc->SetFlip(SDL_FLIP_HORIZONTAL);
											else if (flip == "vertical")
												goc->SetFlip(SDL_FLIP_VERTICAL);
											else if (flip == "horizontal,vertical")
												goc->SetFlip(SDL_RendererFlip(SDL_FLIP_HORIZONTAL | SDL_FLIP_VERTICAL));
											else
												goc->SetFlip(SDL_FLIP_NONE);
										}
									}
									else
										simpleReader.ToParent();
									simpleReader.ToParent();
								}
								else if (typeName == "GocGobjRotator") {
									static unsigned s_GocGobjRotatorId = 1;
									g_gobs[id]->AddComponent(new CSaruGame::GocGobjRotator(s_GocGobjRotatorId++));
									if (simpleReader.ToChild("properties")) {
										auto goc = g_gobs[id]->GetGoc<CSaruGame::GocGobjRotator>();
										goc->SetRadiansPerSecond(simpleReader.Float("rads_per_sec", 10.0f));
									}
									else
										simpleReader.ToParent();
									simpleReader.ToParent();
								}
							}

							simpleReader.ToNextSibling();
						}
						simpleReader.ExitArray();
					}
					else
						simpleReader.ToParent();
				}
				else
					simpleReader.ToParent();
				simpleReader.ToParent();

				simpleReader.ToParent();
			}
		}
	}

	// animating
	{
		g_gobs[GOB_ANIMATING]->AddComponent(new CSaruGame::GocSrcRectAnimator(1));


		auto rectAnimator = g_gobs[GOB_ANIMATING]->GetGoc<CSaruGame::GocSrcRectAnimator>();
		rectAnimator->SetTargetRect(
			&g_gobs[GOB_ANIMATING]->GetGoc<CSaruGame::GocSpriteSimple>()->GetSrcRect()
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
		auto * spriteGoc = g_gobs[i]->GetGoc<CSaruGame::GocSpriteSimple>();
		SDL_assert_release(spriteGoc);
		if (!spriteGoc->LoadTextureFromFile(g_renderer, "kenney/platformer_redux/spritesheet_players.png"))
			return false;
		//spriteGoc->GetSrcRect() = SDL_Rect{ 512, 1280, 128, 256 };

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
    SDL_Quit();

}


int main (int argc, char ** argv) {

    unused(argc);
    unused(argv);

    // initialize and load
    if (!init())
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

					case SDLK_w: {
						for (unsigned i = GOB_PLAIN; i < GOBS; ++i) {
							g_gobs[i]->GetGoc<CSaruGame::GocSpriteSimple>()->GetTexture()->SetAlpha(0xFF);
						}
					} break;

					case SDLK_s: {
						for (unsigned i = GOB_PLAIN; i < GOBS; ++i) {
							g_gobs[i]->GetGoc<CSaruGame::GocSpriteSimple>()->GetTexture()->SetAlpha(0x7F);
						}
					} break;

                    default:         g_testRectIndex = 0; break;
                }
            }
        }

		// Update all game objects.
		const float dt = g_timer.GetMsDelta() / 1000.0f;
		for (unsigned i = 0; i < GOBS; ++i) {
			g_gobs[i]->Update(dt);
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
			g_gobs[i]->Render();

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

