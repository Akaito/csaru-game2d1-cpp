// NOTE: Much of this early code is taken from Lazy Foo's SDL tutorials at http://lazyfoo.net/tutorials/SDL/

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

#include <stdio.h>

#include <csaru-core-cpp/csaru-core-cpp.h>
#include <csaru-game2dlib-cpp/csaru-game2dlib-cpp.h>


/*
class TextureWrapper {
private:
	SDL_Texture * m_texture;
	unsigned      m_width;
	unsigned      m_height;

public:
	TextureWrapper ();
	~TextureWrapper ();

	bool LoadFromFile (
		const char * path,
		bool         colorKeying = false,
		uint8_t      r           = 0xFF,
		uint8_t      g           = 0x00,
		uint8_t      b           = 0xFF
	);

	bool LoadFromRenderedText (const char * textureText, SDL_Color textColor);

	void Free ();

	void SetColor (uint8_t r, uint8_t g, uint8_t b);
	void SetAlpha (uint8_t a);
	void SetBlendMode (SDL_BlendMode blendMode);

	void Render (unsigned x, unsigned y, const SDL_Rect * srcRect = nullptr, double rotDegrees = 0.0, const SDL_Point * rotCenter = nullptr, SDL_RendererFlip flip = SDL_FLIP_NONE) const;

	unsigned GetWidth () const   { return m_width;  }
	unsigned GetHeight () const  { return m_height; }
};
*/


static const int  s_screenWidth       = 640;
static const int  s_screenHeight      = 480;
static const char s_testImageSource[] = "kenney/platformer_redux/spritesheet_ground.png";

static unsigned       g_frameCounter = 0;
static SDL_Window *   g_window       = nullptr;
static SDL_Renderer * g_renderer     = nullptr;
static TTF_Font *     g_font         = nullptr;
//static TextureWrapper g_textTexture;
static CSaru2d::Texture g_bgTexture;
static CSaru2d::Texture g_fgTexture;

static SDL_Rect s_testRects[] = {
    {   0,   0, 128, 128 },
    { 128,   0, 128, 128 },
    { 256,   0, 128, 128 },
    {   0, 128, 128, 128 },
    { 128, 128, 128, 128 },
};
static unsigned g_testRectIndex = 0;

static SDL_Rect s_animRects[] = {
    { 128,1024, 128, 256 },
    { 128, 768, 128, 256 },
};

static SDL_Rect s_viewportRects[] = {
	{                 0,                  0, s_screenWidth / 2, s_screenHeight / 2 },
	{ s_screenWidth / 2,                  0, s_screenWidth / 2, s_screenHeight / 2 },
	{                 0, s_screenHeight / 2, s_screenWidth,     s_screenHeight / 2 },
};


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
	g_renderer = SDL_CreateRenderer(
		g_window,
		-1 /* rendering driver index; -1 use first available renderer */,
		SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
	);
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

	// Initialize SDL_ttf
	if (TTF_Init() == -1) {
		printf("SDL_ttf could not initialize!  %s\n", TTF_GetError());
		return false;
	}

    return true;

}


bool loadMedia () {

	if (!g_bgTexture.LoadFromFile(g_renderer, "testImage.png", false, 0x00, 0x00, 0x00)) {
		printf("Failed to load bg image!\n");
		return false;
	}

	if (!g_fgTexture.LoadFromFile(g_renderer, "kenney/platformer_redux/spritesheet_players.png")) {
		printf("Failed to load bg image!\n");
		return false;
	}
	g_fgTexture.SetBlendMode(SDL_BLENDMODE_BLEND);

	// Open the font
	{
		g_font = TTF_OpenFont("ubuntu-font-family/UbuntuMono-B.ttf", 28);
		if (!g_font) {
			printf("Failed to load ttf font!  SDL_ttf Error: %s\n", TTF_GetError());
			return false;
		}

		// Render text
		/*
		SDL_Color textColor = { 255, 255, 255 };
		if (!g_textTexture.LoadFromRenderedText("The quick brown fox jumps over the lazy dog", textColor)) {
			printf("Failed to render text texture!\n");
			return false;
		}
		*/
	}

    return true;

}


void close () {

	// Free loaded data.
	//g_textTexture.Free();
	g_fgTexture.Free();
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

					case SDLK_w: g_fgTexture.SetAlpha(0xFF); break;
					case SDLK_s: g_fgTexture.SetAlpha(0x7F); break;

                    default:         g_testRectIndex = 0; break;
                }
            }
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

		SDL_RenderSetViewport(g_renderer, nullptr);
		g_bgTexture.Render(g_renderer, 0, 0);
		// Test src clip rect in TextureWrapper class.
		SDL_Rect srcRect = { 512, 1280, 128, 256 };
		g_fgTexture.SetColor(0xFF, 0xFF, 0xFF);
		g_fgTexture.Render(g_renderer, 100, 40, &srcRect);

		// Test animation.
		const unsigned animFrame = (g_frameCounter / 16) % arrsize(s_animRects);
		g_fgTexture.Render(g_renderer, 100 - srcRect.w, 40, &s_animRects[animFrame]);

		// Rotation and flipping
		g_fgTexture.Render(g_renderer, 100 - srcRect.w, 40 + srcRect.h, &srcRect, 45.0 /* rotDegrees */);
		g_fgTexture.Render(g_renderer, 100            , 40 + srcRect.h, &srcRect, 45.0, nullptr /* rotCenter */, SDL_FLIP_HORIZONTAL);
		g_fgTexture.Render(g_renderer, 100 + srcRect.w, 40 + srcRect.h, &srcRect, 45.0 + 1.0 * g_frameCounter, nullptr /* rotCenter */, SDL_RendererFlip(SDL_FLIP_HORIZONTAL | SDL_FLIP_VERTICAL));

		// Test color modulation.
		g_fgTexture.SetColor(0xFF, 0x00, 0x00);
		g_fgTexture.Render(g_renderer, 100 + srcRect.w, 40, &srcRect);

		// Test text rendering.
		/*
		g_textTexture.Render(
			(s_screenWidth  - g_textTexture.GetWidth())  / 2,
			(s_screenHeight - g_textTexture.GetHeight()) / 2
		);
		*/

		SDL_RenderPresent(g_renderer);

		++g_frameCounter;
    }

    // wait
    //SDL_Delay(2000);

    // quit
    close();
    return 0;

}


/*
bool TextureWrapper::LoadFromRenderedText (const char * textureText, SDL_Color textColor) {
	// Get rid of pre-existing texture (if any)
	Free();

	// Render text surface
	SDL_Surface * textSurface = TTF_RenderText_Solid(g_font, textureText, textColor);
	if (!textSurface) {
		printf("Unable to render text surface! SDL_ttf Error: %s\n", TTF_GetError());
		return false;
	}

	// Create texture from surface pixels
	m_texture = SDL_CreateTextureFromSurface(g_renderer, textSurface);
	if (!m_texture) {
		printf("Unable to create texture from rendered text! SDL Error: %s\n", SDL_GetError());
		return false;
	}

	// Get image dimensions
	m_width  = textSurface->w;
	m_height = textSurface->h;

	// Get rid of old surface
	SDL_FreeSurface(textSurface);
	return true;
}
*/

