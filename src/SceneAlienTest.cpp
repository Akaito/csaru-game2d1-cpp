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

#include <csaru-datamap-cpp/csaru-datamap-cpp.hpp>
#include <csaru-json-cpp/csaru-json-cpp.hpp>

#include "SceneAlienTest.hpp"

enum Gobs {
	GOB_PLAIN = 0,
	GOB_COLOR_MOD,
	GOB_ROTATED,
	GOB_ROTATED_FLIP,
	GOB_ROTATING,
	GOB_ANIMATING,
	GOBS
};


//======================================================================
bool SceneAlienTest::Init (int argc, const char * const argv[]) {

	unused(argc);
	unused(argv);

	// Arbitrary ObjectDbTable test
	m_db.Init();
	CSaruUuid::Uuid testUuidA{{{5, 42}}};
	CSaruUuid::Uuid testUuidB{{{8493, 42}}};
	SDL_assert_release(m_db.CreateNow(testUuidA).generation);
	SDL_assert_release(m_db.Find(CSaruGame::FindStyle::FAIL, testUuidA).generation);
	SDL_assert_release(m_db.Find(CSaruGame::FindStyle::CREATE_NOW, testUuidB).generation);
	SDL_assert_release(m_db.Find(CSaruGame::FindStyle::FAIL, testUuidB).generation);

	// Set color used when clearing.
	// TODO
	//SDL_SetRenderDrawColor(g_renderer, 0x3F, 0xff, 0x3F, 0xFF);

	// Prepare "level"
	//LoadLevelStuff("levels/test/03_Assets.json");
	if (!LoadLevelStuff("levels/test/05_GameObjects.json"))
		return false;
	if (!LoadLevelStuff("levels/test/07_GameObjectComponents.json"))
		return false;

	// animating
	{
		CSaruGame::GameObject * gob = m_level.GetGameObject(GOB_ANIMATING);
		SDL_assert_release(gob);
		gob->AddComponent(new CSaruGame::GocSrcRectAnimator(1));

		auto rectAnimator = gob->GetGoc<CSaruGame::GocSrcRectAnimator>();
		rectAnimator->SetTargetRect(
			&gob->GetGoc<CSaruGame::GocSpriteSimple>()->GetSrcRect()
		);
		rectAnimator->SetAnimation(&m_yellowAlienWalkAnim);
	}

    return true;
}


//======================================================================
bool SceneAlienTest::Load (SDL_Renderer * renderer) {
	// load background texture
	{
		SDL_assert_release(PHYSFS_exists("testImage.png"));
		SDL_RWops * rwOps = CSaruGame::AllocRwOpsPhysFs("testImage.png", 'r');
		SDL_assert_release(rwOps);
		if (!m_bgTexture.Load(renderer, rwOps, false, 0x00, 0x00, 0x00)) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load bg image!\n");
			return false;
		}
	}

	// load GocSpriteSimple textures
	for (unsigned i = GOB_PLAIN; i < GOBS; ++i) {
		CSaruGame::GameObject * gob = m_level.GetGameObject(i);
		SDL_assert_release(gob);
		auto * spriteGoc = gob->GetGoc<CSaruGame::GocSpriteSimple>();
		SDL_assert_release(spriteGoc);
		SDL_RWops * rwOps = CSaruGame::AllocRwOpsPhysFs("kenney/platformer_redux/spritesheet_players.png", 'r');
		SDL_assert_release(rwOps);
		if (!spriteGoc->LoadTexture(renderer, rwOps)) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "GocSpriteSimple failed to load texture. %s", SDL_GetError());
			return false;
		}

		switch (i) {
			case GOB_COLOR_MOD: {
				spriteGoc->GetTexture()->SetColor(0xFF, 0x00, 0x00);
			} break;
		}
	}

	// Open the font
	{
		SDL_RWops * rwOps = CSaruGame::AllocRwOpsPhysFs("ubuntu-font-family/UbuntuMono-B.ttf", 'r');
		m_font = TTF_OpenFontRW(rwOps, 1 /* SDL_RWclose for me */, 28);
		if (!m_font) {
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load ttf font!  SDL_ttf Error: %s\n", TTF_GetError());
			return false;
		}

		// Render text
		SDL_Color textColor = { 255, 255, 255, 255 }; // TODO : Was 255 the right 'a' value?
		if (!m_textTexture.LoadFromRenderedText(renderer, m_font, "The quick brown fox jumps over the lazy dog", textColor)) {
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to render text texture!\n");
			return false;
		}
	}

	// setup animation(s)
	m_yellowAlienWalkAnim.SetSrcRect(0, SDL_Rect{ 128, 1024, 128, 256 });
	m_yellowAlienWalkAnim.SetSrcRect(1, SDL_Rect{ 128,  768, 128, 256 });
	m_yellowAlienWalkAnim.SetFrameDuration(0, 250);
	m_yellowAlienWalkAnim.SetFrameDuration(1, 250);

	return true;
}


//======================================================================
void SceneAlienTest::Unload () {
	// Free loaded data.
	m_textTexture.Free();
	m_bgTexture.Free();

	// Free font data.
	TTF_CloseFont(m_font);
	m_font = nullptr;
}


//======================================================================
void SceneAlienTest::Update (float dtms) {
	// Update all game objects.
	for (unsigned i = 0; i < GOBS; ++i) {
		m_level.GetGameObject(i)->Update(dtms);
	}
}


//======================================================================
void SceneAlienTest::OnSdlEvent (const SDL_Event & e) {
	if (e.type == SDL_QUIT) {
		m_shouldQuit = true;
	}
	else if (e.type == SDL_KEYDOWN) {
		switch (e.key.keysym.sym) {
			case SDLK_ESCAPE: m_shouldQuit = true; break;

			case SDLK_p: {
				for (unsigned i = GOB_PLAIN; i < GOBS; ++i) {
					auto trans = m_level.GetGameObject(i)->GetTransform();
					SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Pos: %f, %f", trans.GetPosition()[0], trans.GetPosition()[1]);
				}
			} break;

			case SDLK_w: {
				for (unsigned i = GOB_PLAIN; i < GOBS; ++i) {
					m_level.GetGameObject(i)->GetGoc<CSaruGame::GocSpriteSimple>()->GetTexture()->SetAlpha(0xFF);
				}
			} break;

			case SDLK_s: {
				for (unsigned i = GOB_PLAIN; i < GOBS; ++i) {
					m_level.GetGameObject(i)->GetGoc<CSaruGame::GocSpriteSimple>()->GetTexture()->SetAlpha(0x7F);
				}
			} break;
		}
	}
}


//======================================================================
void SceneAlienTest::Render (SDL_Renderer * renderer, int screenWidth, int screenHeight) {
	// Clear the renderer to prepare for drawing.
	glClearColor(0.33f, 0.40f, 0.27f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	// Render prep
	SDL_RenderSetViewport(renderer, nullptr);
	m_bgTexture.Render(
		renderer,
		screenWidth  / 2 - m_bgTexture.GetWidth() / 2,
		screenHeight / 2 - m_bgTexture.GetHeight() / 2
	);

	// Render game objects
	for (unsigned i = 0; i < GOBS; ++i)
		m_level.GetGameObject(i)->Render();

	// Test text rendering.
	m_textTexture.Render(
		renderer,
		(screenWidth  - m_textTexture.GetWidth())  / 2,
		(screenHeight - m_textTexture.GetHeight()) / 2
	);
}


//======================================================================
bool SceneAlienTest::LoadLevelStuff (const char * filepath) {
	CSaruJson::JsonParser jsonParser;
	PHYSFS_File *         levelFileFs = PHYSFS_openRead(filepath);
	if (!levelFileFs) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, PHYSFS_getLastError());
		SDL_assert(levelFileFs);
		return false;
	}

	CSaruDataMap::DataMap                 levelDm;
	CSaruJson::JsonParserCallbackForDataMap levelParserCallback(levelDm.GetMutator());

	const PHYSFS_sint64 bufObjCount = static_cast<PHYSFS_sint64>(CSaruCore::GetSystemPageSize());
	char *              buf         = new char[bufObjCount];
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
			return false;
		}

		jsonParser.ParseBuffer(buf, bufObjCount, &levelParserCallback);
	}

	// Load level assets.
	CSaruDataMap::DataMapReader levelReader = levelDm.GetReader();
	m_level.AcceptDataMap(levelReader);

	delete [] buf;
	if (levelFileFs)
		PHYSFS_close(levelFileFs);

	for (unsigned i = 0; i < GOBS; ++i) {
		SDL_assert_release(m_level.GetGameObject(i));
	}

	return true;
}

