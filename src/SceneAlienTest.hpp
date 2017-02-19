#pragma once
#include <csaru-uuid-cpp/csaru-uuid-cpp.hpp>
#include <csaru-game2dlib-cpp/csaru-game2dlib-cpp.hpp>

class SceneAlienTest : public CSaruGame::GameScene {
private:
	CSaruGame::ObjectDbTable<CSaruUuid::Uuid, CSaruGame::GocSpriteSimple> m_db;
	CSaruGame::Level m_level;
	bool             m_shouldQuit = false;

	TTF_Font *                m_font = nullptr;
	CSaru2d::Texture          m_textTexture;
	CSaru2d::Texture          m_bgTexture;
	CSaru2d::TextureAnimation m_yellowAlienWalkAnim;

	// TODO : Move to GameScene?
	struct KeyboardState {
		static const int s_keyArraySize = 512;
		uint8_t          keyArray[s_keyArraySize];
	};
	KeyboardState m_keyboardFrames[2]; // current frame (0), and last (1)

private:
	bool LoadLevelStuff (const char * filepath);

public: // CSaruGame:Scene
	bool Init (int argc, const char * const argv[]) override;
	bool Load (SDL_Renderer *) override;
	void Unload () override;
	void Update (float dtms) override;
	void Render (SDL_Renderer *, int screenWidth, int screenHeight) override;
	void OnSdlEvent (const SDL_Event &) override;
	bool ShouldQuit () override        { return m_shouldQuit; }
};

